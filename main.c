#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include "encryption.h"
#include "decryption.h"
/* run this program using the console pauser or add your own getch, system("pause") or input loop */

int main(int argc, char *argv[]) {
	int exit_status = 0;
	if(argc != 4){
		fprintf(stderr, "usage: %s <encrypt|decrypt> <input path> <output path> \n", argv[0]);
		return exit_status;
	}
	const char *operation = argv[1];
	const char *input_path = argv[2];
	const char *output_path = argv[3];
	unsigned int op= 0;
	
	if (strcmp(operation, "decrypt") != 0 && strcmp(operation, "encrypt") != 0) {
        fprintf(stderr, "Unknown operation: %s\n", operation);
        return exit_status;
    }
    if(strcmp(operation, "decrypt") == 0) op=1;
    if(strcmp(operation, "encrypt") == 0) op=2;
    if (input_path[0] == '\0' || output_path[0] == '\0') {
        fprintf(stderr, "Input and output paths must not be empty.\n");
        return exit_status;
    }
    
    char password[202] = {0};//including \0 and enter
    
    
    printf("Password: ");
    fflush(stdout);

    if (fgets(password, sizeof(password), stdin) == NULL) {
        fprintf(stderr, "Failed to read password.\n");
        goto cleanup;
    }

    size_t password_len = strcspn(password, "\r\n");
    password[password_len] = '\0'; //remove enter from the end

    if (password_len == 0 || password_len > 200) {
        fprintf(stderr, "Password must be between 1-200 Bytes.\n");
        goto cleanup;
    }
	int status;	
    ByteBuffer target = {0};
	if(op == 1 ){
		status = decryption(
        	(const unsigned char *)password,
        	password_len,
        	input_path,
        	output_path,
        	target
    	);
	}
	if(op == 2){
		status = encryption(
        	(const unsigned char *)password,
        	password_len,
        	input_path,
        	output_path,
        	target
    	);
	}
    
	if (status != 7 && op == 1) {
        fprintf(stderr, "Decryption failed (code %d).\n", status);
        goto cleanup;
    }
    if (status != 7 && op == 2) {
        fprintf(stderr, "Encryption failed (code %d).\n", status);
        goto cleanup;
    }
	if(op == 1)printf("Decrypted file saved to: %s\n", output_path);
    if(op == 2)printf("Encrypted file saved to: %s\n", output_path);
    
    exit_status = 1;

cleanup:
    OPENSSL_cleanse(password, sizeof(password));
    return exit_status;
}

