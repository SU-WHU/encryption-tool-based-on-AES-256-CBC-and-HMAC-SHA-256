#include "authen.h"
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <stddef.h>
int compute_mac(
	const unsigned char key[32],
	const unsigned char *data,//from ByteBuffer
	size_t data_len,//from ByteBuffer
	unsigned char tag[32]
){
	unsigned int tag_len = 0;
	
	if(key == NULL) return 0;
	if(tag == NULL) return 1;
	if(data == NULL && data_len != 0) return 2;
	if(HMAC(
	EVP_sha256(),
	key,
	32,
	data,
	data_len,
	tag,
	&tag_len
	) == NULL) return 3; // compute HMAC for whole file
	
	return 4;
}
