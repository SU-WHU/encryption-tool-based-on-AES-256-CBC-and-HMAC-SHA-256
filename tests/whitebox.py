#!/usr/bin/env python3
"""Build fresh sources and test format v1. Python standard library only.
Requires GCC, OpenSSL development files, and the openssl command-line program.
All fixtures and binaries are isolated in a new temporary directory.
"""
import argparse
import hashlib
import hmac
import json
import os
from pathlib import Path
import random
import shutil
import subprocess
import sys
import tempfile
import time

PROJECT = Path(__file__).resolve().parents[1]
SOURCES = ['AES_Encrypt.c', 'AES_Decrypt.c', 'CBC.c', 'input_mod.c',
           'key_derivation.c', 'authen.c', 'confirm_authen.c', 'encryption.c', 'decryption.c']
PASSWORD = b'whitebox-test-only-password'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='gcc')
    parser.add_argument('--openssl', default='openssl', help='OpenSSL CLI path')
    parser.add_argument('--openssl-prefix', type=Path)
    parser.add_argument('--openssl-libdir', type=Path)
    parser.add_argument('--report', type=Path, default=PROJECT / 'tests' / 'last-report.json')
    parser.add_argument('--keep', action='store_true', help='Keep generated fixtures for debugging')
    parser.add_argument('--large', action='store_true', help='Also test rejection above the 200 MiB limits')
    parser.add_argument('--sanitize', action='store_true', help='Enable ASan/UBSan if supported by GCC runtime')
    parser.add_argument('--timeout', type=float, default=60)
    args = parser.parse_args()
    run_root = PROJECT.parent / 'output' / 'whitebox'
    run_root.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='run-', dir=run_root))
    records = []
    exit_errors = []
    env = os.environ.copy()
    cc = shutil.which(args.cc) or args.cc
    openssl = shutil.which(args.openssl) or args.openssl
    env['PATH'] = str(Path(cc).resolve().parent) + os.pathsep + env.get('PATH', '')
    if args.openssl_prefix:
        env['PATH'] = str(args.openssl_prefix.resolve() / 'bin') + os.pathsep + env['PATH']
    if os.name == 'nt':
        import ctypes
        ctypes.windll.kernel32.SetErrorMode(0x8003)  # Do not hang on a crash dialog.

    def run(command, data=None):
        return subprocess.run([str(x) for x in command], input=data, capture_output=True,
                              cwd=work, env=env, timeout=args.timeout)

    def require(condition, detail):
        if not condition:
            raise AssertionError(detail)

    def case(name, function):
        start = time.monotonic()
        try:
            function()
            item = dict(name=name, status='PASS')
        except Exception as exc:
            item = dict(name=name, status='FAIL', detail=str(exc))
        item['seconds'] = round(time.monotonic() - start, 3)
        records.append(item)
        print(item['status'], name, item.get('detail', ''), flush=True)

    suffix = '.exe' if os.name == 'nt' else ''
    exe, modules = work / ('coursework' + suffix), work / ('modules' + suffix)

    def build():
        flags = ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Wpedantic', '-I', str(PROJECT)]
        if args.sanitize:
            flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        libs = []
        if args.openssl_prefix:
            prefix = args.openssl_prefix.resolve()
            flags += ['-I', str(prefix / 'include')]
            libdir = args.openssl_libdir or (prefix / 'lib')
            if not args.openssl_libdir and not libdir.exists():
                libdir = prefix / 'lib64'
            libs += ['-L', str(libdir.resolve())]
        elif args.openssl_libdir:
            libs += ['-L', str(args.openssl_libdir.resolve())]
        libs += ['-lcrypto']
        if os.name == 'nt':
            libs += ['-lws2_32', '-lgdi32', '-ladvapi32', '-lcrypt32', '-luser32', '-lbcrypt']
        for entry, destination in [(PROJECT / 'main.c', exe), (PROJECT / 'tests/module_tests.c', modules)]:
            command = [cc] + flags + [str(entry)] + [str(PROJECT / s) for s in SOURCES] + ['-o', str(destination)] + libs
            result = run(command)
            (work / (destination.stem + '-build.log')).write_bytes(result.stdout + result.stderr)
            require(result.returncode == 0, result.stderr.decode(errors='replace'))
        result = run([openssl, 'version'])
        require(result.returncode == 0, 'OpenSSL CLI unavailable')

    def invoke(op, source, destination, password=PASSWORD):
        result = run([exe, op, source, destination], password + b'\n')
        result.cli = True
        return result

    def success(result):
        if getattr(result, 'cli', False):
            require(result.returncode in (0, 1), f'process crash or abnormal exit: {result.returncode}')
            require(b'file saved to:' in result.stdout, f'CLI failed: {result.stderr.decode(errors="replace")}')
            if result.returncode != 0:
                exit_errors.append('successful operation returned ' + str(result.returncode))
            return
        require(result.returncode == 0, f'exit={result.returncode}: {result.stderr.decode(errors="replace")}')

    def reject(result):
        # Check functional rejection separately from the exit-code contract so that
        # one CLI regression does not hide the rest of the cryptographic tests.
        require(result.returncode in (0, 1), f'crash or abnormal exit: {result.returncode}')
        require(bool(result.stderr) and b'file saved to:' not in result.stdout,
                'no failure diagnostic, or operation reported success')
        if result.returncode != 1:
            exit_errors.append('rejected operation returned ' + str(result.returncode))

    def module_suite():
        result = run([modules])
        text = result.stdout.decode(errors='replace')
        (work / 'module-results.txt').write_text(text, encoding='utf-8')
        for line in text.splitlines():
            if line.startswith(('PASS ', 'FAIL ')):
                records.append(dict(name='module: ' + line[5:], status=line[:4]))
        success(result)

    def aes(data, key, iv, decrypt=False, nopad=False):
        command = [openssl, 'enc', '-aes-256-cbc', '-K', key.hex(), '-iv', iv.hex()]
        if decrypt:
            command.append('-d')
        if nopad:
            command.append('-nopad')
        result = run(command, data)
        success(result)
        return result.stdout

    def roundtrip(n, extension='.txt', password=PASSWORD):
        folder = work / ('round-' + str(len(records)))
        folder.mkdir()
        source, encrypted, restored = folder / ('input' + extension), folder / 'file.enc', folder / ('output' + extension)
        plain = bytes(i % 256 for i in range(n))
        source.write_bytes(plain)
        success(invoke('encrypt', source, encrypted, password))
        blob = encrypted.read_bytes()
        require(blob[:5] == b'\x00\x11\x22\x33\x01', 'wrong magic/version')
        header_len = 38 + blob[37]
        expected_extension = ('.' + extension.rsplit('.', 1)[1]) if extension else ''
        require(blob[38:header_len] == expected_extension.encode(), 'wrong stored extension')
        require(len(blob) == header_len + (n // 16 + 1) * 16 + 32, 'wrong padded length')
        keys = hashlib.pbkdf2_hmac('sha256', password, blob[5:21], 1200000, 64)
        require(hmac.compare_digest(hmac.digest(keys[32:], blob[:-32], 'sha256'), blob[-32:]), 'independent HMAC mismatch')
        require(aes(blob[header_len:-32], keys[:32], blob[21:37], decrypt=True) == plain, 'OpenSSL reference decryption mismatch')
        success(invoke('decrypt', encrypted, restored, password))
        require(restored.read_bytes() == plain, 'roundtrip mismatch')
        require(source.read_bytes() == plain, 'input changed')

    salt, iv = bytes(range(16)), bytes(range(16, 32))
    keys = None

    def fixture(data=b'known independent plaintext', extension=b'.txt', raw=False):
        body = b'\x00\x11\x22\x33\x01' + salt + iv + bytes([len(extension)]) + extension
        body += aes(data, keys[:32], iv, nopad=raw)
        return body + hmac.digest(keys[32:], body, 'sha256')

    def negative(blob, password=PASSWORD, extension='.txt'):
        source, destination = work / 'negative.enc', work / ('negative-output' + extension)
        require(not destination.exists(), 'previous rejected test left output behind')
        source.write_bytes(blob)
        reject(invoke('decrypt', source, destination, password))
        require(not destination.exists(), 'rejected ciphertext created output')
        require(source.read_bytes() == blob, 'rejected ciphertext changed input')

    try:
        case('fresh build and OpenSSL CLI', build)
        if records[-1]['status'] != 'PASS':
            raise RuntimeError('Build prerequisite failed; dependent tests not run')
        case('module suite', module_suite)
        for n in [0, 1, 15, 16, 17, 31, 32, 33, 4095, 4096, 4097]:
            case(f'roundtrip and independent reference: {n} bytes', lambda n=n: roundtrip(n))
        for extension in ['', '.tar.gz', '.' + 'x' * 61]:
            case(f'extension boundary: {extension!r}', lambda e=extension: roundtrip(17, e))
        case('password maximum 200 bytes', lambda: roundtrip(16, password=b'x' * 200))
        keys = hashlib.pbkdf2_hmac('sha256', PASSWORD, salt, 1200000, 64)
        valid = fixture()

        def independent_input():
            source, output = work / 'reference.enc', work / 'reference.txt'
            source.write_bytes(valid)
            success(invoke('decrypt', source, output))
            require(output.read_bytes() == b'known independent plaintext', 'reference input mismatch')
        case('decrypt independently generated authenticated fixture', independent_input)
        case('wrong password', lambda: negative(valid, b'wrong'))
        case('output extension mismatch', lambda: negative(valid, extension='.png'))
        for label, index in [('magic',0), ('version',4), ('salt',5), ('IV',21), ('extension length',37), ('extension',38), ('first ciphertext block',42), ('last ciphertext block',len(valid)-33), ('tag',len(valid)-1)]:
            altered = bytearray(valid); altered[index] ^= 1
            case('tamper ' + label, lambda b=bytes(altered): negative(b))
        for length in [0, 1, 37, 38, 41, 42, 57, len(valid)-33, len(valid)-1]:
            case(f'truncated to {length} bytes', lambda n=length: negative(valid[:n]))
        case('appended byte', lambda: negative(valid + b'x'))
        case('appended block', lambda: negative(valid + b'x' * 16))
        body = valid[:42]
        case('authenticated but no ciphertext', lambda: negative(body + hmac.digest(keys[32:], body, 'sha256')))
        for label, padded in [('zero', b'A'*15+b'\0'), ('too large', b'A'*15+b'\x11'), ('inconsistent', b'A'*14+b'\x01\x02')]:
            case('valid MAC but invalid padding: ' + label, lambda p=padded: negative(fixture(p, raw=True)))
        rng = random.Random(25560)
        for i in range(8):
            data = bytes(rng.randrange(256) for _ in range(rng.randrange(0, 512)))
            case(f'deterministic malformed input {i}', lambda b=data: negative(b))

        def cli(arguments, data=b''):
            reject(run([exe] + arguments, data))
        for arguments in [[], ['encrypt'], ['encrypt','a','b','extra'], ['invalid','a','b'], ['encrypt','','b'], ['decrypt','a','']]:
            case('CLI arguments ' + repr(arguments), lambda a=arguments: cli(a))
        source = work / 'small.txt'; source.write_bytes(b'unchanged')
        for label, password_input in [('empty', b'\n'), ('EOF', b''), ('201 bytes', b'x'*201+b'\n')]:
            def bad_password(data=password_input):
                output = work / 'password-rejected.enc'
                cli(['encrypt', str(source), str(output)], data)
                require(not output.exists(), 'invalid password created output')
            case('password input ' + label, bad_password)

        def protected(op, same=False):
            src = work / ('protected.txt' if op=='encrypt' else 'protected.enc')
            src.write_bytes(b'KEEP' if op=='encrypt' else valid)
            dst = src if same else work / ('existing.enc' if op=='encrypt' else 'existing.txt')
            if not same:
                dst.write_bytes(b'KEEP')
            before_src, before_dst = src.read_bytes(), dst.read_bytes()
            reject(invoke(op, src, dst))
            require(src.read_bytes()==before_src and dst.read_bytes()==before_dst, 'existing file overwritten')
        for op in ['encrypt', 'decrypt']:
            case(op + ' existing output preserved', lambda op=op: protected(op))
            case(op + ' same input/output preserved', lambda op=op: protected(op, True))

        def missing_input():
            dst = work / 'missing-output.enc'
            reject(invoke('encrypt', work / 'does-not-exist', dst))
            require(not dst.exists(), 'missing input created output')
        case('missing input', missing_input)
        case('missing output directory', lambda: reject(invoke('encrypt', source, work / 'absent/output.enc')))
        def long_extension():
            src = work / ('input.' + 'x'*62); src.write_bytes(b'x')
            dst = work / 'long-extension.enc'
            reject(invoke('encrypt', src, dst))
            require(not dst.exists(), 'oversized extension accepted')
        case('63-byte extension rejected', long_extension)

        def randomization():
            blobs = []
            for i in range(3):
                dst = work / f'random-{i}.enc'
                success(invoke('encrypt', source, dst)); blobs.append(dst.read_bytes())
            require(len({b[5:21] for b in blobs})==3, 'repeated salt')
            require(len({b[21:37] for b in blobs})==3, 'repeated IV')
        case('repeated encryption uses fresh salt and IV (smoke test)', randomization)
        if args.large:
            for op, limit in [('encrypt',209715200), ('decrypt',209715400)]:
                def oversized(op=op, limit=limit):
                    src, dst = work / 'large-input', work / 'large-output'
                    with src.open('wb') as stream:
                        stream.truncate(limit+1)
                    reject(invoke(op,src,dst))
                    require(not dst.exists(), 'oversized input created output')
                case(op + ' exceeds file size limit', oversized)
        case('CLI exit-code contract: success=0, failure=1',
             lambda: require(not exit_errors, f'{len(exit_errors)} incorrect exit codes; examples: {exit_errors[:3]}'))
    except Exception as exc:
        records.append(dict(name='suite infrastructure', status='FAIL', detail=str(exc)))
        print('FAIL', exc, flush=True)
    finally:
        failed = sum(r['status']=='FAIL' for r in records)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(dict(project=str(PROJECT), work=str(work),
            kept=args.keep, failed=failed, passed=len(records)-failed, cases=records), indent=2), encoding='utf-8')
        print(f'{len(records)-failed} PASS, {failed} FAIL; report: {args.report}', flush=True)
        if args.keep:
            print('Fixtures and build logs:', work)
        else:
            shutil.rmtree(work)
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
