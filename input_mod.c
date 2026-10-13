#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "input_mod.h"
int read_file(
	const char *path,
	ByteBuffer *output,
	const unsigned int op_type
) {
	size_t enc_limit = 209715200;
	size_t dec_limit = 209715400;
	size_t limit = 0; 
	//200MB, Allow most pictures or PDFs even videos
	//decrypt need more space
	if(op_type == 1) limit = dec_limit;
	else if(op_type == 2) limit = enc_limit;
	else return 0;
	FILE *file = NULL;
	ByteBuffer buffer = {0};
	unsigned char chunk[4096];
	size_t count;
	
	if(path == NULL) return 0;
	if(output == NULL) return 1;
	
	file = fopen(path, "rb");//Only read the file in binary
	if(file == NULL) return 2;
	
	while((count = fread(chunk, 1, sizeof(chunk), file)) > 0){
		
		if(count > limit - buffer.len) {
			if (file != NULL) {
        		fclose(file);
    		}//file too big

    		free(buffer.data);
   			return 3;
		}
		
		size_t required = buffer.len + count;
		
		if(required > buffer.capacity){
			size_t new_capacity = buffer.capacity;
			if(new_capacity == 0){
				new_capacity = limit < sizeof(chunk) ? limit : sizeof(chunk);
			}//Initialize the buffer
			
			while(new_capacity < required){
				if(new_capacity > limit/2) new_capacity = limit;
				else new_capacity *= 2;
			}//double the capacity to get more data
			
			unsigned char *new_data = realloc(buffer.data, new_capacity);
			
			if(new_data == NULL) {
				if (file != NULL) {
        			fclose(file);
    			}

    			free(buffer.data);
   				return 4;
			}
			
			buffer.data = new_data;
			buffer.capacity = new_capacity;
		}
		
		memcpy(buffer.data + buffer.len, chunk, count);//copy the new data to the end
		buffer.len += count;
	}
	
	if(ferror(file)){
		if (file != NULL) {
        	fclose(file);
    	}

    	free(buffer.data);
   		return 5;
		
	}//streaming input error
	
	int close = fclose(file);
	file = NULL;
	if(close != 0){
    	free(buffer.data);
   		return 6;
	}//error when closing the file
	
	*output = buffer;
	return 7;
}

