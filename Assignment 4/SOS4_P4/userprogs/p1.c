#include "../lib.h"



int main(void) {
	char c = 'A';
	int i;

	for (i=0; i<5; i++) {
		sleep(5000);
		printf("%c",c);			
	}
}



