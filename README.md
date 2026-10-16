# Secure File Encryption and Authentication

Student: Ziyu Su · Student ID: 2810330 · Email ID: ig25560

## Contents

- `*.c`, `*.h`: C implementation and module interfaces.
- `Makefile`: GNU Make build for Linux.
- `Secure File Encryption and Authentication.docx`: report.
- `tests/whitebox.py`, `tests/module_tests.c`: automated verification tools.
- `tests/last-report.json`: Windows test results; Linux results are recorded separately.
- `BUILD.md`: older Windows build notes. The current Makefile targets Linux; use the instructions below for this package.

## Dependencies

The application requires Linux, a C11 compiler (tested with GCC 13.3.0), GNU Make, the C/POSIX runtime and OpenSSL **3.0.x** development headers and `libcrypto` (tested on Ubuntu with OpenSSL 3.0.13). Ubuntu packages: `build-essential`, `libssl-dev`.

The test suite additionally requires Python 3 and the OpenSSL command-line utility. Python uses only its standard library. The utility is an independent test oracle; the C application does not invoke it. Sanitizer testing requires GCC AddressSanitizer and UndefinedBehaviorSanitizer runtimes.

## Build and run

Open a Linux terminal in this directory:

```sh
make
./coursework encrypt input.txt encrypted.enc
./coursework decrypt encrypted.enc restored.txt
```

Enter the same password when prompted. Passwords can also be supplied as one line on standard input. The accepted length is 1–200 bytes. Output files must not already exist. The decryption output must have the same final extension as the original input (`.gz` for `archive.tar.gz`); extension matching is case-sensitive.

The coursework brief requires an executable named `filecrypt`. The supplied Makefile currently defaults to `coursework`. Its existing variable can be overridden without changing source:

```sh
make TARGET=filecrypt
./filecrypt encrypt input.txt encrypted.enc
./filecrypt decrypt encrypted.enc restored.txt
```

`make clean` removes the default build products; use `make clean TARGET=filecrypt` for that target.

## Implementation status

| Requirement | Current implementation |
| --- | --- |
| C and OpenSSL | Implemented; Linux Makefile builds successfully. |
| AES-256-CBC and padding | CBC chaining and PKCS#7 padding/unpadding are implemented in project code. OpenSSL supplies raw AES-256 ECB blocks with EVP padding disabled. |
| Password-based key derivation | PBKDF2-HMAC-SHA256, 1,200,000 iterations, 16-byte salt, 64-byte result split into a 32-byte encryption key and a 32-byte MAC key. |
| Fresh salt and IV | Separate 16-byte values generated with `RAND_bytes()` on each encryption. |
| Encrypt-then-MAC | HMAC-SHA256 covers the complete header and ciphertext. Tags use constant-time comparison. Verification precedes AES decryption; output opens after decryption and padding validation. |
| File format | Magic, version, salt, IV, extension length, extension, ciphertext and 32-byte tag. KDF parameters are implicit in version 1 rather than stored as separate numeric fields. |
| Error handling | Rejects wrong passwords, tampering, malformed files, invalid padding, invalid arguments and existing outputs; reports failure with a nonzero exit status. |
| Report | Included unchanged. |
| Required executable name | Default target remains `coursework`; see override above. |

## File format and limits

```text
magic 00 11 22 33 (4) | version 01 (1) | salt (16) | IV (16)
| extension length (1) | extension (0–62) | padded ciphertext | HMAC (32)
```

Lengths are bytes. This package interprets version 1 as PBKDF2-HMAC-SHA256 with 1,200,000 iterations. Older local code used the same version with 600,000 iterations, so its encrypted files are incompatible.

Plaintext input is limited to 209,715,200 bytes (200 MiB); encrypted input is limited to 209,715,400 bytes. Whole-file buffering increases memory use. Linux non-terminal input currently attempts terminal configuration and can print warnings; terminal error/EOF restoration also needs attention. Keys and the password buffer are cleansed, but plaintext buffers are freed without explicit cleansing. These are remaining limitations.

## Tests

Run on Linux:

```sh
python3 tests/whitebox.py --large --keep --report tests/linux-report.json
python3 tests/whitebox.py --sanitize --large --keep --report tests/linux-sanitized-report.json
```

On 16 October 2026, Windows testing and Ubuntu ASan/UBSan testing each completed with **149 PASS, 0 FAIL**, including size-limit tests. The separate Ubuntu Makefile build and binary round trip also succeeded. An additional pseudo-terminal test found that EOF leaves echo disabled; this is a known defect even though the existing suite passes.

The suite compiles fresh sources and checks known answers, boundary lengths, independent OpenSSL/Python interoperability, tampering, wrong passwords, padding, malformed input, output preservation and size limits. Its build command is separate from Makefile verification. Test dependencies and Windows commands are in `tests/README.md`.

The packaged test oracle was updated from 600,000 to 1,200,000 iterations to match the current source and report. The C implementation and report were copied byte-for-byte from the parent directory on 16 October 2026. The previous contents of this folder were backed up outside this package.

## Preparation record

OpenAI Codex assisted with file organisation, this README, test execution and a separate formative assessment on 16 October 2026. The existing report's AI-use appendix describes an earlier editing session. The assessment brief limits AI use to its minimal category; this preparation record makes the additional assistance explicit and should be considered alongside that requirement before submission.
