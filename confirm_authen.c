#include "confirm_authen.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/crypto.h>
int verify_mac(
	const unsigned char key[32],
	const unsigned char *data,//from ByteBuffer
	size_t data_len,//from ByteBuffer
	const unsigned char tag[32]
){
	unsigned int tag_len = 0;
	unsigned char ver_tag[32];
	if(key == NULL) return 0;
	if(tag == NULL) return 1;
	if(data == NULL && data_len != 0) return 2;
	if(HMAC(
	EVP_sha256(),
	key,
	32,
	data,
	data_len,
	ver_tag,
	&tag_len
	) == NULL) return 3; // compute HMAC for whole file
	if (tag_len != sizeof(ver_tag)) return 3;
	if(CRYPTO_memcmp(ver_tag, tag, 32) != 0) return 4;
	return 5;
}
