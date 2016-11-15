#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

int get_position(unsigned short x){
	int n = 1;
	if(x == 0)return -1;
	if((x >> 8) == 0){
		n = n + 8;
		x = x << 8;
	}
	if((x >> 12) == 0){
		n = n + 4;
		x = x << 4;
	}
	if((x >> 14) == 0){
		n = n + 2;
		x = x << 2;
	}
	if((x >> 15) == 0){
		n = n + 1;
		x = x << 1;
	} 
	n = n - (x >> 15);
	return 15 - n;
}

int main(int argc, char **argv){
	// printf("%d \n", get_position(0xf000) );
	short input = 0xf100;
	int flag = 1;
	int result = 0;

	if(input < 0)
		flag = -1;

	int position = get_position(0x1000) + 1;
	printf("position: %d\n", position);
	unsigned int abs_input = abs(0xf100);
	if(position < 8){
		result = abs_input << 8;
		
	}else if(position == 8){
		result = abs_input << 7;
	}else {
		result = abs_input << (16 - position);
	}
	printf("%04x\n", result * flag);
	return 0;
}