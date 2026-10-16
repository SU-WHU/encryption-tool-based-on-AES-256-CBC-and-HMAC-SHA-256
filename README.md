# Secure File Encryption and Authentication

Student: Ziyu Su · Student ID: 2810330 · Email ID: ig25560

## Environment

This tool is developed on Windows 11, Inter i9-13900H CPU. Using Dev-C++ 1991 as a IDE. Openssl libs are from Msys2 UCRT64 at the version 3.0.22. The tool has adaptation to Linux system, tested on Ubuntu 24.04.3 LTS on WSL.

## Contents

- `*.c`, `*.h`: C implementation and module interfaces.
- `Makefile`: GNU Make build for Linux.
- `Secure File Encryption and Authentication.docx`: report.

## Dependencies

The application requires Linux, a C11 compiler (tested with GCC 13.3.0), GNU Make, the C/POSIX runtime and OpenSSL **3.0.x** development headers and `libcrypto` (tested on Ubuntu with OpenSSL 3.0.13). Ubuntu packages: `build-essential`, `libssl-dev`.

## Build and run

Open a Linux terminal in this directory:

```sh
make
./coursework encrypt input.txt encrypted.enc
./coursework decrypt encrypted.enc restored.txt
```

Enter the same password when prompted. Passwords can also be supplied as one line on standard input. The accepted length is 1–200 bytes. However, input the password using keyboard is highly recommended, this can prevent password from seeing or recording. Output files must not already exist. The decryption output must have the same final extension as the original input (`.gz` for `archive.tar.gz`); extension matching is case-sensitive.

## Implementation status

| Requirement | Current implementation |
| --- | --- |
| AES-256-CBC and padding | CBC chaining and PKCS#7 padding/unpadding are implemented in project code. OpenSSL supplies raw AES-256 ECB blocks with EVP padding disabled. |
| Password-based key derivation | PBKDF2-HMAC-SHA256, 1,200,000 iterations, 16-byte salt, 64-byte result split into a 32-byte encryption key and a 32-byte MAC key. |
| Fresh salt and IV | Separate 16-byte values generated with `RAND_bytes()` on each encryption. |
| Encrypt-then-MAC | HMAC-SHA256 covers the complete header and ciphertext. Tags use constant-time comparison. Verification precedes AES decryption; output opens after decryption and padding validation. |
| File format | Magic, version, salt, IV, extension length, extension, ciphertext and 32-byte tag. KDF parameters are implicit in version 1 rather than stored as separate numeric fields. |
| Error handling | Rejects wrong passwords, tampering, malformed files, invalid padding, invalid arguments and existing outputs; reports failure with a nonzero exit status. |

## File format and limits

```text
magic 00 11 22 33 (4) | version 01 (1) | salt (16) | IV (16)
| extension length (1) | extension (0–62) | padded ciphertext | HMAC (32)
```

Lengths are bytes. This package interprets version 1 as PBKDF2-HMAC-SHA256 with 1,200,000 iterations. Older local code used the same version with 600,000 iterations, so its encrypted files are incompatible.

Plaintext input is limited to 209,715,200 bytes (200 MiB); encrypted input is limited to 209,715,400 bytes. Whole-file buffering increases memory use. Linux non-terminal input currently attempts terminal configuration and can print warnings; terminal error/EOF restoration also needs attention.

## Further information and Expansion

I also commit the project onto GitHub (In a private repo before the deadline). More detailed information and new functions may add into it. Also, there are some auto test tools in it. Here's the link https://github.com/SU-WHU/encryption-tool-based-on-AES-256-CBC-and-HMAC-SHA-256.git
