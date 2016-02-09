#include "../lib.h"



int main(void) {
	char c = 'B';
	int i;
	for (i=0; i<50; i++) {
		printf("%c",c);	
		sleep(1000);		
	}
}



