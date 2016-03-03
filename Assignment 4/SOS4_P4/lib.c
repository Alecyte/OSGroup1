////////////////////////////////////////////////////////
// Library functions 
// Ideally you would have different files e.g. string.c,
// stdio.c, etc. 
// We will dump a few into this single file
//
// These functions can also be used by a user process
// They should never use any function listed in 
// kernel_only.h

#include "lib.h"

/***************** String related functions ***************/ 

/*** Compare two strings ***/
int strcmp(const char *s1, const char *s2) {
	int ret = 0;

	while (!(ret = *(unsigned char *) s1 - *(unsigned char *) s2)
			&& *s2) ++s1, ++s2;

	if (ret < 0) ret = -1; // s1 < s2
	else if (ret > 0) ret = 1; // s1 > s2

  	return ret;
}

/*** Convert ascii string to integer ***/
int atoi(char *s) {
	uint32_t val = 0;
	int sign = 1;

	if (*s == '-') {
		sign = -1;
		s++;
	}

	while ('0'<= *s && *s <= '9') {
		val = val*10 + (*s-'0');
		s++;
	}
	return sign*val;
}


/***************** I/O related functions ***************/ 

/*** Read a character from the keyboard ***/
char getc(void) { // SYSTEM CALL
	uint32_t ret;
	
	asm volatile ("movl %0, %%eax\n": :"i" (SYSCALL_GETC)); // read keyboard function
	asm volatile ("int $0x94\n");
	asm volatile ("movl %%edx, %0\n": "=m" (ret));

	return (char)ret; 
}

/*** Formatted output (simple printf) ***/
// Formatting strings:
//	%c: character
//	%s: string
//	%x: byte as hex
//	%u: unsigned integer
//	%d: signed integer
//	%%: %
//	%<any_other_character>: <any_other_character>
int printf(const char *format, ...) { // SYSTEM CALL
	va_list args;
	uint32_t ret;

	va_start(args,format);
	asm volatile ("movl %0, %%ebx\n": :"m" (format));
	asm volatile ("movl %0, %%ecx\n": :"m" (args));
	asm volatile ("movl %0, %%eax\n": :"i" (SYSCALL_PRINTF)); // print function
	asm volatile ("int $0x94\n");
	asm volatile ("movl %%edx, %0\n": "=m" (ret));
	va_end(args);

	return ret; // 0 means error

}

/*** Sleep calling process for tts milliseconds ***/
void sleep(uint32_t tts) { // SYSTEM CALL
	asm volatile ("movl %0, %%ebx\n": :"m" (tts));
	asm volatile ("movl %0, %%eax\n": :"i" (SYSCALL_SLEEP)); // sleep function
	asm volatile ("int $0x94\n");
}

/*** Mutex functions ***/
mutex_t mcreate() { // SYSTEM CALL
	uint32_t ret;

	asm volatile ("movl %0, %%eax\n": :"i" (SYSCALL_MUTEX_CREATE)); // mutex create function
	asm volatile ("int $0x94\n");

	asm volatile ("movl %%edx, %0\n": "=m" (ret));
	return (mutex_t)ret; // 0 means unsuccessful
}

void mdestroy(mutex_t key) { // SYSTEM CALL
	asm volatile ("movl %0, %%ebx\n": :"m" (key));
	asm volatile ("movl %0, %%eax\n": :"i" (SYSCALL_MUTEX_DESTROY)); // mutex destroy function
	asm volatile ("int $0x94\n");
}

void mlock(mutex_t key) { // SYSTEM CALL
	asm volatile ("movl %0, %%ebx\n": :"m" (key));
	asm volatile ("movl %0, %%eax\n": :"i" (SYSCALL_MUTEX_LOCK)); // mutex lock function
	asm volatile ("int $0x94\n");
}

bool munlock(mutex_t key) { // SYSTEM CALL
	bool ret;

	asm volatile ("movl %0, %%ebx\n": :"m" (key));
	asm volatile ("movl %0, %%eax\n": :"i" (SYSCALL_MUTEX_UNLOCK)); // mutex unlock function
	asm volatile ("int $0x94\n");

	asm volatile ("movl %%edx, %0\n": "=m" (ret));
	return ret; // FALSE means unsuccessful
}

/*** Semaphore functions ***/
sem_t screate(uint8_t init_value) { // SYSTEM CALL
	uint32_t ret;

	asm volatile ("movl %0, %%ebx\n": :"m" (init_value));
	asm volatile ("movl %0, %%eax\n": :"i" (SYSCALL_SEM_CREATE)); // semaphore create function
	asm volatile ("int $0x94\n");

	asm volatile ("movl %%edx, %0\n": "=m" (ret));
	return (sem_t)ret; 
}

void sdestroy(sem_t key) { // SYSTEM CALL
	asm volatile ("movl %0, %%ebx\n": :"m" (key));
	asm volatile ("movl %0, %%eax\n": :"i" (SYSCALL_SEM_DESTROY)); // semaphore destroy function
	asm volatile ("int $0x94\n");
}

void sdown(sem_t key) { // SYSTEM CALL
	asm volatile ("movl %0, %%ebx\n": :"m" (key));
	asm volatile ("movl %0, %%eax\n": :"i" (SYSCALL_SEM_DOWN)); // semaphore down function
	asm volatile ("int $0x94\n");
}

void sup(sem_t key) { // SYSTEM CALL
	asm volatile ("movl %0, %%ebx\n": :"m" (key));
	asm volatile ("movl %0, %%eax\n": :"i" (SYSCALL_SEM_UP)); // semaphore up function
	asm volatile ("int $0x94\n");
}

/*** Shared memory functions ***/
void  *smcreate(uint8_t key, uint32_t size) { // SYSTEM CALL
	uint32_t ret;

	asm volatile ("movl %0, %%ebx\n": :"m" (key));
	asm volatile ("movl %0, %%ecx\n": :"m" (size));
	asm volatile ("movl %0, %%eax\n": :"i" (SYSCALL_SHM_CREATE)); // shared memory create function
	asm volatile ("int $0x94\n");

	asm volatile ("movl %%edx, %0\n": "=m" (ret));
	return (void *)ret; 
}

void  *smattach(uint8_t key, uint32_t mode) { // SYSTEM CALL
	uint32_t ret;

	asm volatile ("movl %0, %%ebx\n": :"m" (key));
	asm volatile ("movl %0, %%ecx\n": :"m" (mode));
	asm volatile ("movl %0, %%eax\n": :"i" (SYSCALL_SHM_ATTACH)); // shared memory attach function
	asm volatile ("int $0x94\n");

	asm volatile ("movl %%edx, %0\n": "=m" (ret));
	return (void *)ret; 
}

void  smdetach() { // SYSTEM CALL
	asm volatile ("movl %0, %%eax\n": :"i" (SYSCALL_SHM_DETACH)); // shared memory detach function
	asm volatile ("int $0x94\n"); 
}


