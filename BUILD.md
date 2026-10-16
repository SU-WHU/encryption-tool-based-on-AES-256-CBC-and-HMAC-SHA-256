# Build and run

The Makefile targets Linux with GCC, GNU Make and OpenSSL 3.0.x development headers and libcrypto. On Ubuntu, the required packages are `build-essential` and `libssl-dev`.

```sh
make
./filecrypt encrypt input.txt encrypted.enc
./filecrypt decrypt encrypted.enc restored.txt
make clean
```

The output must not already exist. Enter the same password for encryption and decryption. The restored filename must retain the original final extension.

The C sources also contain Windows branches. The Linux Makefile is not a native Windows Makefile. Windows compilation and automated testing commands are documented in `tests/README.md`; the test harness builds its own executable directly with GCC.

See `README.md` for implementation status, dependencies and known limitations.
