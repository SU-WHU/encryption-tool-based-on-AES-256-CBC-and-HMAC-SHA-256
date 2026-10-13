#ifndef ENCRYPTION_H
#define ENCRYPTION_H
#include "input_mod.h"
int encryption(
	const unsigned char *key,
	const size_t key_len, 
	const char *path,
	const char *output_path,
	ByteBuffer target
);
#endif
