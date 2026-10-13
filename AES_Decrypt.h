#ifndef AES_DECRYPT_H
#define AES_DECRYPT_H

int DecByAES(
	const unsigned char key[32],
	const unsigned char input[16],
	unsigned char output[16]
	//The availability should be checked while input
);
#endif
