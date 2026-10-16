
#include "decryption.h"
#include "AES_Decrypt.h"
#include "CBC.h"
#include "confirm_authen.h"
#include "input_mod.h"
#include "key_derivation.h"
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int decryption(
	const unsigned char *key,
	const size_t key_len, 
	const char *path,
	const char *output_path,
	ByteBuffer target
) {
	if(key == NULL) return 0;
	if(path == NULL) return 0;
	if(output_path == NULL) return 0;
	if(target.data != NULL || target.len != 0 || target.capacity != 0) return 0;
	//check if input is NULL
	
	
	
	size_t header_len = 0;
	size_t extension_len = 0; 
	unsigned char header[100];//the header of file
	unsigned char division[16];
	unsigned char salt[16];
	unsigned char InitVector[16];
	unsigned char buffer_output[16];
	unsigned char previous[16];
	unsigned char k_mac[32];
	unsigned char k_enc[32];
	unsigned char tag[32];
	unsigned char *new_data = NULL;
	ByteBuffer result = {0};
	
	if(read_file(path, &target, 1) != 7) goto cleanup;
	
	if (target.len < 38) goto cleanup;

	extension_len = target.data[37];
	if (extension_len > 62) goto cleanup;

	header_len = 38 + extension_len;
	if (target.len < header_len + 16 + 32) goto cleanup;
    //check availability of the header
	size_t cipher_len = target.len - header_len - 32;
	if (cipher_len % 16 != 0) goto cleanup;//check ciphertext length
	
	memcpy(tag, target.data + target.len - 32, 32);
	
	for(int i=0; i<38;i++){
		header[i] = target.data[i];
	}//read the fixed header
	if(header[0] != 0x00 || header[1] != 0x11|| header[2] != 0x22||
	header[3] != 0x33 || header[4] != 0x01) goto cleanup;
	
	memcpy(salt, header + 5, 16);
	memcpy(InitVector, header+21, 16);
	header_len = 38 + extension_len;
	//find the extension of the file
	if(kdf(key, key_len, k_mac, k_enc, salt) != 5) goto cleanup;
	if(verify_mac(k_mac, target.data, target.len - 32, tag) != 5) goto cleanup;
	
	const char *filename = output_path;
	const char *slash = strrchr(output_path, '/');
	const char *backslash = strrchr(output_path, '\\');

	if (slash != NULL)
    	filename = slash + 1;

	if (backslash != NULL && backslash + 1 > filename)
    	filename = backslash + 1;


	if (*filename == '\0') {
    	fprintf(stderr, "Output filename is empty.\n");
    	goto cleanup;
	}
	//find the filename in output 
	const char *dot = strrchr(filename, '.');
	const char *output_extension = "";
	size_t output_extension_len = 0;
	//locate the extension
	if (dot != NULL && dot != filename && dot[1] != '\0') {
    	output_extension = dot;
    	output_extension_len = strlen(dot);
	}

	const unsigned char *stored_extension = target.data + 38;

	if (output_extension_len != extension_len ||
    	memcmp(output_extension, stored_extension, extension_len) != 0) {
    	fprintf(stderr,"Output extension does not match the stored extension.\n");
    	goto cleanup;
	}
	//preventing from attackes by changing extension
	new_data = malloc(target.len - header_len - 32);
	if(new_data == NULL) goto cleanup;
	result.data = new_data;
	

	unsigned int position = header_len;
	while(target.len - position >= 48){
		memcpy(division, target.data+position, 16);
		
		if(position == header_len){
			memcpy(previous, InitVector, 16);
			if(DecByAES(k_enc, division, buffer_output) != 1) goto cleanup;
			if(CBC(previous, buffer_output, 16) != 4) goto cleanup;
			memcpy(result.data + result.len, buffer_output, 16);
			result.len += 16;
			position += 16;
		}//first block
		else {
			memcpy(previous, target.data+position-16, 16);
			if(DecByAES(k_enc, division, buffer_output) != 1) goto cleanup;
			if(CBC(previous, buffer_output, 16) != 4) goto cleanup;
			memcpy(result.data + result.len, buffer_output, 16);
			result.len += 16;
			position += 16;
		}//blocks in the middle
	}
	
	unsigned int padding = result.data[result.len - 1];
	if(padding < 1 || padding >16 || padding > result.len) goto cleanup;
	for(size_t i = 0; i<padding ; i++){
		if(result.data[result.len - 1 - i] != padding) goto cleanup;
	}
	result.len -= padding;
	//unpadding the text
	
	FILE *file = fopen(output_path, "wbx");
	if(file == NULL) {
		fprintf(stderr, "Can't generate output.enc: the file already exists or can't write into directory\n");
		goto cleanup;
	}
	
	size_t written = fwrite(result.data, 1, result.len, file);
	int close_status = fclose(file);
	file = NULL;

	if (written != result.len || close_status != 0) {
    	remove(output_path);
    	//fprintf(stderr, "Writing error: no enough space, content tampered or file error\n");
    	goto cleanup;
	}
	free(target.data);
	free(result.data);
	OPENSSL_cleanse(k_mac, 32);
	OPENSSL_cleanse(k_enc, 32);
	OPENSSL_cleanse(salt, 16);
	OPENSSL_cleanse(InitVector, 16);
	//reuse the encryption code
	return 7;
	cleanup:
		free(target.data);
		free(result.data);
		OPENSSL_cleanse(k_mac, 32);
		OPENSSL_cleanse(k_enc, 32);
		OPENSSL_cleanse(salt, 16);
		OPENSSL_cleanse(InitVector, 16);
		return 6;
}
