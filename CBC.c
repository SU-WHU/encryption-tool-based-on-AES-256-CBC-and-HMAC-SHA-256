#include <string.h>
#include "CBC.h"
int CBC(
	const unsigned char prev_text[16],//check before input
	unsigned char pres_text[16],
	int pres_len
){
	if(pres_len < 0) return 0;
	if(prev_text == NULL) return 1;
	if(pres_text == NULL) return 2;
	if(pres_len > 16) return 3;
	if(pres_len == 16){
		for(int i=0;i<16;i++){
			pres_text[i]=((prev_text[i])^(pres_text[i]));
		}
		return 4;
	} 
	else {
		for(int i=pres_len; i<16;i++){
			pres_text[i]=(0x10-pres_len);
		}//padding
		for(int i=0;i<16;i++){
			pres_text[i]=((prev_text[i])^(pres_text[i]));
		}
		return 5;
	}
}
