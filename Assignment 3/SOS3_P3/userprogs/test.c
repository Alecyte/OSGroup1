#include "../lib.h"

void show_table(int);

int main(void) {
	char c;
	printf("%s\n","THE MULTIPLIER");
	printf("%s","How about a number?");
	c = getc();
	printf("%c\n",c);
	show_table(c-48);	
}

void show_table(int n){
	int i;
	for (i = 1; i<=10; i++) 
		printf("%u X %u = %u\n",n,i,n*i);
}


