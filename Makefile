# GNU Make + GCC. Run from this directory; paths supplied as variables use /.
# No dependency on the Dev-C++ project or Makefile.win.
.DEFAULT_GOAL := all
.DELETE_ON_ERROR:

CC = gcc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wpedantic
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?=
OPENSSL_PREFIX ?=
OPENSSL_LIBDIR ?= $(OPENSSL_PREFIX)/lib

ifeq ($(OS),Windows_NT)
SHELL := cmd.exe
.SHELLFLAGS := /C
TARGET := coursework.exe
# Needed by common static MinGW OpenSSL builds; configurable for other builds.
SYSTEM_LIBS ?= -lws2_32 -lgdi32 -ladvapi32 -lcrypt32 -luser32 -lbcrypt
else
TARGET := coursework
SYSTEM_LIBS ?=
endif

# lib64 layouts must set OPENSSL_LIBDIR as well. Without a prefix,
# use GCC's default search paths, or pass OPENSSL_CFLAGS/OPENSSL_LIBS.
ifneq ($(strip $(OPENSSL_PREFIX)),)
OPENSSL_CFLAGS ?= -I"$(OPENSSL_PREFIX)/include"
OPENSSL_LDFLAGS := -L"$(OPENSSL_LIBDIR)"
OPENSSL_LIBS ?= -lcrypto
else
OPENSSL_CFLAGS ?=
OPENSSL_LDFLAGS :=
OPENSSL_LIBS ?= -lcrypto
endif

SOURCES := main.c AES_Encrypt.c AES_Decrypt.c CBC.c input_mod.c \
           key_derivation.c authen.c confirm_authen.c encryption.c decryption.c
# Keep independent objects, so old IDE-generated .o files are never reused.
OBJECTS := $(SOURCES:.c=.portable.o)
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS) Makefile
	"$(CC)" $(LDFLAGS) $(OPENSSL_LDFLAGS) -o "$@" $(OBJECTS) $(OPENSSL_LIBS) $(SYSTEM_LIBS) $(LDLIBS)

%.portable.o: %.c Makefile
	"$(CC)" $(CPPFLAGS) $(OPENSSL_CFLAGS) $(CFLAGS) -MMD -MP -c "$<" -o "$@"

clean:
ifeq ($(OS),Windows_NT)
	@for %%f in ($(OBJECTS) $(DEPS) $(TARGET)) do @if exist "%%f" del /q "%%f"
else
	rm -f $(OBJECTS) $(DEPS) $(TARGET)
endif

-include $(DEPS)
