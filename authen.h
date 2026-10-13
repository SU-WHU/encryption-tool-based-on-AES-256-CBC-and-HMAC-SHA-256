#ifndef AUTHEN_H
#define AUTHEN_H
#include <stddef.h> 
int compute_mac(
	const unsigned char key[32],
	const unsigned char *data,//from ByteBuffer
	size_t data_len,//from ByteBuffer
	unsigned char tag[32]
);
#endif
