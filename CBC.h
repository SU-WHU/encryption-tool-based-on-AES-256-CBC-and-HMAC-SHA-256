#ifndef CBC_H
#define CBC_H
int CBC(
	const unsigned char prev_text[16],//check before input
	unsigned char pres_text[16],
	int pres_len
);
#endif
