#include "AES_Decrypt.h"
#include <openssl/evp.h>
#include <string.h>
int DecByAES(
	const unsigned char key[32],
	const unsigned char input[16],
	unsigned char output[16]
	//The availability should be checked while input
){
	unsigned char decrypted[32];//As a buffer, prevent from overflow
	int decrypted_len = 0;
	unsigned char f_output[16];
	int final_len = 0;
	EVP_CIPHER_CTX *ctx = NULL;
	if(key == NULL || input == NULL || output == NULL) goto cleanup;//check if the pointer right
	
	ctx = EVP_CIPHER_CTX_new();
	if(ctx == NULL) goto cleanup;
	if(EVP_DecryptInit_ex(ctx, EVP_aes_256_ecb(), NULL, key, NULL) != 1) goto cleanup;//Initiate Encryption
	if(EVP_CIPHER_CTX_set_padding(ctx, 0) != 1) goto cleanup;
	if(EVP_DecryptUpdate(ctx, decrypted, &decrypted_len, input, 16) != 1) goto cleanup;
	if(EVP_DecryptFinal_ex(ctx, f_output, &final_len) != 1) goto cleanup;
	if(decrypted_len != 16 || final_len != 0) goto cleanup;
	
	memcpy(output, decrypted, 16);
	EVP_CIPHER_CTX_free(ctx);
	
	return 1;
	
	cleanup:
		EVP_CIPHER_CTX_free(ctx);
		return 0;
}
