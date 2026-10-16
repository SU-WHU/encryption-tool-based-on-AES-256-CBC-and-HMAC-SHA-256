#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include "encryption.h"
#include "decryption.h"
#ifdef _WIN32
	#include <windows.h>
	#include <io.h>
#elif defined(__linux__)
	#include <termios.h>
	#include <unistd.h>
#else #error Unsupported platform
	#endif
//to use different method to disable echo
/* run this program using the console pauser or add your own getch, system("pause") or input loop */
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

/* 成功返回 1，失败返回 0；结果单位为秒 */
static int monotonic_seconds(double *seconds)
{
#ifdef _WIN32
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;

    if (!QueryPerformanceFrequency(&frequency) ||
        !QueryPerformanceCounter(&counter)) {
        return 0;
    }

    *seconds = (double)counter.QuadPart /
               (double)frequency.QuadPart;
#else
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    *seconds = (double)ts.tv_sec +
               (double)ts.tv_nsec / 1000000000.0;
#endif

    return 1;
}

int main(int argc, char *argv[]) {
	int exit_status = EXIT_FAILURE;
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
    
    
    
    #ifdef _WIN32
	
		HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
    	DWORD mode = 0;
    	if(!GetConsoleMode(hStdin, &mode)){
    		fprintf(stderr, "Fail to get current input mode. Will use default input mode\n");
		}
    	if(!SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT))){
    		fprintf(stderr, "Fail to disable echo. Will use default input mode\n");
		}
		printf("Password: ");
    	fflush(stdout);
		if (fgets(password, sizeof(password), stdin) == NULL) {
        fprintf(stderr, "Failed to read password.\n");
        goto cleanup;
    	}
    	if(!SetConsoleMode(hStdin, mode)){
    		fprintf(stderr, "Fail to refresh input mode, program will continue work...\n"); 
		}
	#elif defined(__linux__)
		    
		struct termios ori_termios;
		if(tcgetattr(STDIN_FILENO, &ori_termios) == -1){
			fprintf(stderr, "Fail to get original termios\n");
		}
		struct termios raw = ori_termios;
		raw.c_lflag &= ~(ECHO);
		if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1){
			fprintf(stderr, "Fail to disable echo.\n");
		}
		printf("Password: ");
    	fflush(stdout);
		if (fgets(password, sizeof(password), stdin) == NULL) {
        fprintf(stderr, "Failed to read password.\n");
        goto cleanup;
    	}
    	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ori_termios) == -1){
			fprintf(stderr, "Fail to refresh input mode, program will continue work...\n");
		}
	#else 
	#error Unsupported platform
	#endif
    /*if (fgets(password, sizeof(password), stdin) == NULL) {
        fprintf(stderr, "Failed to read password.\n");
        goto cleanup;
    }*/

	
	double start = 0.0;
double end = 0.0;
int timer_ok = monotonic_seconds(&start);
	
	
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
    
    if (timer_ok && monotonic_seconds(&end)) {
    fprintf(stderr, "%s elapsed: %.3f ms%s\n",
            op == 1 ? "Decryption" : "Encryption",
            (end - start) * 1000.0,
            status == 7 ? "" : " (failed)");
} else {
    fprintf(stderr, "Timing unavailable.\n");
}
    
    exit_status = EXIT_SUCCESS;
	 
cleanup:
    OPENSSL_cleanse(password, sizeof(password));
    return exit_status;
}

