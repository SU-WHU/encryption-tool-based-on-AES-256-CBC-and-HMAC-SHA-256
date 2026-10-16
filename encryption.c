#include "encryption.h"
#include "AES_Encrypt.h"
#include "CBC.h"
#include "authen.h"
#include "input_mod.h"
#include "key_derivation.h"
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int encryption(
	const unsigned char *key,
	const size_t key_len, 
	const char *path,
	const char *output_path,
	ByteBuffer target
) {
	if(key == NULL) return 0;
	if(path == NULL) return 0;
	if(output_path == NULL) return 0;
	if(target.data != NULL || target.len != 0 || target.capacity != 0) return 1;
	//check if input is NULL
	
	unsigned int position = 0;
	size_t header_len = 0;
	const char *extension = "";
	size_t extension_len = 0;
	unsigned char header[100];//the header of file
	unsigned char division[16];
	unsigned char salt[16];
	unsigned char InitVector[16];
	unsigned char buffer_output[16];
	unsigned char k_mac[32];
	unsigned char k_enc[32];
	unsigned char tag[32];
	unsigned char *new_data = NULL;
	ByteBuffer result = {0};
	
	const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *filename = path;
    if(slash != NULL) filename = slash+1; 
	if(backslash != NULL && backslash + 1 > filename) filename = backslash+1;
	if(*filename == '\0') return 2;
	const char *dot = strrchr(filename, '.');
	//size_t len = strlen(dot);
	
	if(dot == NULL || dot == filename || dot[1] == '\0'){
		extension = "";
		extension_len = 0;
	}//find the last dot to confirm file type
	else{
		size_t len = strlen(dot);
		if(strlen(dot) > 62) return 3;
		extension = dot;
		extension_len = len;//compute the file type
	}
/*	size_t len = strlen(dot);
	if(len>62) return 3;
	extension = dot;
	extension_len = len;//compute the file type */
	
	if(RAND_bytes(salt, sizeof(salt)) != 1) return 4;
	if(RAND_bytes(InitVector, sizeof(InitVector)) != 1) return 5;
	
	header[0]=0x00;
	header[1]=0x11;
	header[2]=0x22;
	header[3]=0x33;
	header[4]=0x01;//version,represent the iteration of KDF
	memcpy(header+5, salt, 16);
	memcpy(header+21, InitVector, 16);
	header[37]=(unsigned char) extension_len;
	if(extension_len > 0) memcpy(header+38, extension, extension_len);
	header_len = 38 + extension_len;
	
	//generate header
	
	new_data = malloc(header_len);
	if(new_data == NULL) goto cleanup;
	result.data = new_data;
	memcpy(result.data, header, header_len);
	result.len = header_len;
	//Write the header to output
	
	if(kdf(key, key_len, k_mac, k_enc, salt) != 5) goto cleanup;

	if(read_file(path, &target, 2) != 7) goto cleanup;
	
	new_data = realloc(result.data, target.capacity+header_len+16);
	if(new_data == NULL) goto cleanup;
	result.data = new_data;
	 
	while(target.len - position >= 16){
		memcpy(division, target.data+position, 16);
		if(position == 0){
			if(CBC(InitVector, division, 16) != 4) goto cleanup;
			if(EncByAES(k_enc, division, buffer_output) != 1) goto cleanup;
			memcpy(result.data + result.len, buffer_output, 16);
			result.len += 16;
			position += 16;
		}//first block
		else {
			if(CBC(buffer_output, division, 16) != 4) goto cleanup;
			if(EncByAES(k_enc, division, buffer_output) != 1) goto cleanup;
			memcpy(result.data + result.len, buffer_output, 16);
			result.len += 16;
			position += 16;
		}//blocks in the middle
	}
	if(target.len - position < 16) {
		if(target.data != NULL)memcpy(division, target.data + position, target.len - position);
		else memset(division, 0, sizeof(division));
		if(position == 0) {
			if(CBC(InitVector, division, target.len - position) != 5) goto cleanup;
		}
		else {
			if(CBC(buffer_output, division, target.len - position) != 5) goto cleanup;
		}
		if(EncByAES(k_enc, division, buffer_output) != 1) goto cleanup;
		memcpy(result.data + result.len, buffer_output, 16);
		result.len += 16;
	}//the last block
	
	if(compute_mac(k_mac, result.data, result.len, tag) == 4){
		new_data = realloc(result.data, result.len + 32);
		if(new_data == NULL) goto cleanup;
		result.data = new_data;
		memcpy(result.data + result.len, tag, 32);
	
		result.len += 32;
	}
	else goto cleanup;
	
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
    	fprintf(stderr, "Writing error: no enough space, content tampered or file error\n");
    	goto cleanup;
	}
	free(target.data);
	free(result.data);
	OPENSSL_cleanse(k_mac, 32);
	OPENSSL_cleanse(k_enc, 32);
	OPENSSL_cleanse(salt, 16);
	OPENSSL_cleanse(InitVector, 16);
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
