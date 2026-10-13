#ifndef KEY_DERIVATION_H
#define KEY_DERIVATION_H
#include <stddef.h>
int kdf(
	const unsigned char *user_key,
	size_t user_key_len,
	unsigned char *key4MAC,
	unsigned char *key4Encrypt,
	const unsigned char *salt
);
#endif
