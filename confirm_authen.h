#ifndef CONFIRM_AUTHEN_H
#define CONFIRM_AUTHEN_H
#include <stddef.h>
int verify_mac(
	const unsigned char key[32],
	const unsigned char *data,//from ByteBuffer
	size_t data_len,//from ByteBuffer
	const unsigned char tag[32]
);
#endif
