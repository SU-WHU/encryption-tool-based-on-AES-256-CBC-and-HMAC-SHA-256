#ifndef INPUT_MOD_H
#define INPUT_MOD_H
#include <stddef.h>
typedef struct{
	unsigned char *data;
	size_t len;
	size_t capacity;
} ByteBuffer; 
int read_file(
	const char *path,
	ByteBuffer *output,
	const unsigned int op_type 
);
#endif
