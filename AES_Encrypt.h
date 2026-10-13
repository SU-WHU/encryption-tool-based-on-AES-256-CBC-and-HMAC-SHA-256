#ifndef AES_ENCRYPT_H
#define AES_ENCRYPT_H

int EncByAES(
    const unsigned char key[32],
    const unsigned char input[16],
    unsigned char output[16]
);

#endif 
