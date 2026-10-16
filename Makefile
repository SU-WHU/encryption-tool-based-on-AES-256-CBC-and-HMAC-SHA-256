# Ubuntu / GCC / system OpenSSL 3.0.x (libssl-dev).
# Run from this directory: make -f Makefil
.DEFAULT_GOAL := all
.DELETE_ON_ERROR:

SHELL := /bin/sh
CC = gcc
CFLAGS ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic
CPPFLAGS += -I.
LDLIBS += -lcrypto

TARGET := coursework
BUILD_DIR := build-ubuntu

SOURCES := main.c AES_Encrypt.c AES_Decrypt.c CBC.c input_mod.c \
           key_derivation.c authen.c confirm_authen.c encryption.c decryption.c
OBJECTS := $(SOURCES:%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS) Makefile
	$(CC) $(LDFLAGS) -o "$@" $(OBJECTS) $(LDLIBS)

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c "$<" -o "$@"

$(BUILD_DIR):
	mkdir -p "$@"

clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET)

-include $(DEPS)
