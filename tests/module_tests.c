#include <stdio.h>
#include <string.h>
#include "AES_Encrypt.h"
#include "AES_Decrypt.h"
#include "CBC.h"
#include "authen.h"
#include "confirm_authen.h"
#include "key_derivation.h"
#include "input_mod.h"

static int failures;
#define CHECK(name, expression) do { int wb_pass = !!(expression); \
    printf("%s %s\n", wb_pass ? "PASS" : "FAIL", name); failures += !wb_pass; } while (0)

int main(void) {
    /* NIST SP 800-38A F.1.5 AES-256, first block. */
    unsigned char key[32] = {
        0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
        0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4};
    unsigned char plain[16] = {0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a};
    unsigned char cipher[16] = {0xf3,0xee,0xd1,0xbd,0xb5,0xd2,0xa0,0x3c,0x06,0x4b,0x5a,0x7e,0x3d,0xb1,0x81,0xf8};
    unsigned char out[32] = {0}, prev[16] = {0}, block[16], tag[32];
    unsigned char expected[32] = {0xb0,0x34,0x4c,0x61,0xd8,0xdb,0x38,0x53,0x5c,0xa8,0xaf,0xce,0xaf,0x0b,0xf1,0x2b,0x88,0x1d,0xc2,0x00,0xc9,0x83,0x3d,0xa7,0x26,0xe9,0x37,0x6c,0x2e,0x32,0xcf,0xf7};
    ByteBuffer buffer = {0};
    CHECK("AES encrypt known answer", EncByAES(key, plain, out) == 1 && memcmp(out,cipher,16)==0);
    CHECK("AES decrypt known answer", DecByAES(key, cipher, out) == 1 && memcmp(out,plain,16)==0);
    CHECK("AES encrypt null key", EncByAES(NULL,plain,out)==0);
    CHECK("AES encrypt null input", EncByAES(key,NULL,out)==0);
    CHECK("AES encrypt null output", EncByAES(key,plain,NULL)==0);
    CHECK("AES decrypt null key", DecByAES(NULL,cipher,out)==0);
    CHECK("AES decrypt null input", DecByAES(key,NULL,out)==0);
    CHECK("AES decrypt null output", DecByAES(key,cipher,NULL)==0);
    for(int n=0;n<=16;n++) {
        char label[80]; memset(block,0xa5,16); memset(prev,0x5a,16);
        int status=CBC(prev,block,n), ok=status==(n==16?4:5);
        for(int i=0;i<16;i++) ok &= block[i]==(unsigned char)((i<n?0xa5:16-n)^0x5a);
        snprintf(label,sizeof(label),"CBC length %d and padding bytes",n); CHECK(label,ok);
    }
    memset(block,0xa5,16);
    CHECK("CBC negative length", CBC(prev,block,-1)==0 && block[0]==0xa5);
    CHECK("CBC excessive length", CBC(prev,block,17)==3 && block[0]==0xa5);
    CHECK("CBC null previous", CBC(NULL,block,16)==1);
    CHECK("CBC null current", CBC(prev,NULL,16)==2);
    /* RFC 4231 case 1: zero-extending its 20-byte key to 32 is HMAC-equivalent. */
    memset(key,0,32); memset(key,0x0b,20);
    CHECK("HMAC RFC 4231 known answer", compute_mac(key,(const unsigned char*)"Hi There",8,tag)==4 && memcmp(tag,expected,32)==0);
    CHECK("HMAC accepts correct tag", verify_mac(key,(const unsigned char*)"Hi There",8,tag)==5);
    for(int i=0;i<32;i++) {
        char label[80]; tag[i]^=1;
        snprintf(label,sizeof(label),"HMAC rejects changed tag byte %d",i);
        CHECK(label,verify_mac(key,(const unsigned char*)"Hi There",8,tag)==4); tag[i]^=1;
    }
    CHECK("HMAC null key", compute_mac(NULL,plain,16,tag)==0);
    CHECK("HMAC null data with length", compute_mac(key,NULL,1,tag)==2);
    CHECK("HMAC null output", compute_mac(key,plain,16,NULL)==1);
    CHECK("verify null key", verify_mac(NULL,plain,16,tag)==0);
    CHECK("verify null tag", verify_mac(key,plain,16,NULL)==1);
    CHECK("verify null data with length", verify_mac(key,NULL,1,tag)==2);
    CHECK("KDF null salt", kdf(plain,16,key,out,NULL)==-1);
    CHECK("KDF null password", kdf(NULL,0,key,out,prev)==0);
    CHECK("KDF null MAC output", kdf(plain,16,NULL,out,prev)==1);
    CHECK("KDF null AES output", kdf(plain,16,key,NULL,prev)==2);
    CHECK("KDF excessive password length", kdf(plain,201,key,out,prev)==3);
    CHECK("read null path", read_file(NULL,&buffer,1)==0);
    CHECK("read null output", read_file("unused",NULL,1)==1);
    CHECK("read invalid operation", read_file("unused",&buffer,3)==0);
    return failures ? 1 : 0;
}
