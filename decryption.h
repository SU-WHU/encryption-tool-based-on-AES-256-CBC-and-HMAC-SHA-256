#ifndef DECRYPTION_H
#define DECRYPTION_H
#include "input_mod.h"
int decryption(
	const unsigned char *key,
	const size_t key_len, 
	const char *path,
	const char *output_path,
	ByteBuffer target
);
#endif
