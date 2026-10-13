#include <openssl/evp.h>
#include <openssl/hmac.h>
//#include <openssl/rand.h>
#include <string.h>
#include <stddef.h>
#include <openssl/crypto.h>
#include "key_derivation.h"
int kdf(
	const unsigned char *user_key,
	size_t user_key_len,
	unsigned char *key4MAC,
	unsigned char *key4Encrypt,
	const unsigned char *salt
) {
	if(salt == NULL) return -1;
	if(user_key == NULL) return 0;
	if(key4MAC == NULL) return 1;
	if(key4Encrypt == NULL) return 2;

	if(user_key_len > 200) return 3;
	
	//if(RAND_bytes(salt, 16) != 1) return 5;
	
	unsigned char out[64];
	if(PKCS5_PBKDF2_HMAC(
		(const char *)user_key,
		(int)user_key_len, 
		salt,
		16,
		600000,
		EVP_sha256(),
		64,
		out
	) != 1) {
		OPENSSL_cleanse(out, sizeof(out));
		return 4;
	}
	memcpy(key4Encrypt, out, 32);
	memcpy(key4MAC, out + 32, 32);
	OPENSSL_cleanse(out, sizeof(out));
	return 5;
}

