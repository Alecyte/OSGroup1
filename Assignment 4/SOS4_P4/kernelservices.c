////////////////////////////////////////////////////////
// Services provided by kernel via 0x94 system call
// 

#include "kernel_only.h"

extern PCB *current_process;

/*** Process the 0x94 system call ***/
// Context of calling process is in current_process
// EAX always has the system call number
void execute_0x94(void) {	
	current_process->state = WAITING;

	// TODO: lookup from array of function pointers
	switch (current_process->cpu.eax) {
		case SYSCALL_GETC: break; //_0x94_getc();  we only allow background processes here
		case SYSCALL_PRINTF: _0x94_printf();  break;
		case SYSCALL_SLEEP: _0x94_sleep(); break;
		case SYSCALL_MUTEX_CREATE: _0x94_mutex_create(); break;
		case SYSCALL_MUTEX_DESTROY: _0x94_mutex_destroy(); break;
		case SYSCALL_MUTEX_LOCK: _0x94_mutex_lock(); break;
		case SYSCALL_MUTEX_UNLOCK: _0x94_mutex_unlock(); break;
		case SYSCALL_SEM_CREATE: _0x94_semaphore_create(); break;
		case SYSCALL_SEM_DESTROY: _0x94_semaphore_destroy(); break;
		case SYSCALL_SEM_UP: _0x94_semaphore_up(); break;
		case SYSCALL_SEM_DOWN: _0x94_semaphore_down(); break;
		case SYSCALL_SHM_CREATE: _0x94_shm_create(); break;
		case SYSCALL_SHM_ATTACH: _0x94_shm_attach(); break;
		case SYSCALL_SHM_DETACH: _0x94_shm_detach(); break;
	}
}

/*** Return the keyboard scan code and SHIFT/CAPSLOCK status ***/
void _0x94_getc(void) {
	char key;

	enable_interrupts();  // okay to have other interrupts while waiting for key entry
	key = sys_getc();
	disable_interrupts();

	// returned in EDX register
	current_process->cpu.edx = (uint32_t)key;

	current_process->state = READY;
}

/*** Print formatted output for the user ***/
void _0x94_printf(void) {
	char *format = (char *)current_process->cpu.ebx;
	va_list args = (va_list)current_process->cpu.ecx;

	_printf(format,args,0);

	current_process->cpu.edx = 1; // success

	current_process->state = READY;
}

/*** Make process sleep ***/
void _0x94_sleep(void) {
	uint32_t tts = current_process->cpu.ebx;
	current_process->sleep_end = get_epochs() + tts/get_epoch_length(); // each epoch is 10ms long	
	current_process->state = WAITING;
}

/*** Create a mutex ***/
void _0x94_mutex_create(void) {
	current_process->cpu.edx = mutex_create(current_process); // return value
	current_process->state = READY;
}

/** Destroy a mutex ***/
void _0x94_mutex_destroy(void) {
	uint8_t key = (uint8_t)current_process->cpu.ebx;
	mutex_destroy(key,current_process);
	
	current_process->state = READY;
}

/*** Obtain lock on a mutex ***/
void _0x94_mutex_lock(void) {
	uint8_t key = (uint8_t)current_process->cpu.ebx;

	if (mutex_lock(key,current_process)) // lock obtained
		current_process->state = READY;
}

/*** Unlock a mutex ***/
void _0x94_mutex_unlock(void) {
	uint8_t key = (uint8_t)current_process->cpu.ebx;
	current_process->cpu.edx = mutex_unlock(key,current_process); // return value

	current_process->state = READY;
}

/*** Create a sempahore ***/
void _0x94_semaphore_create(void) {
	uint8_t init_value = (uint8_t)current_process->cpu.ebx;
	current_process->cpu.edx = semaphore_create(init_value, current_process); // return value

	current_process->state = READY;
}

/** Destroy a semaphore ***/
void _0x94_semaphore_destroy(void) {
	uint8_t key = (uint8_t)current_process->cpu.ebx;
	semaphore_destroy(key,current_process);
	
	current_process->state = READY;
}

/*** UP operation on a semaphore ***/
void _0x94_semaphore_up(void) {
	uint8_t key = (uint8_t)current_process->cpu.ebx;
	semaphore_up(key,current_process);
	
	current_process->state = READY;
}

/*** DOWN operation on a semaphore ***/
void _0x94_semaphore_down(void) {
	uint8_t key = (uint8_t)current_process->cpu.ebx;

	if (semaphore_down(key,current_process)) // obtained
		current_process->state = READY;
}

/*** Create shared memory area ***/
void _0x94_shm_create(void) {
	uint8_t key = (uint8_t)current_process->cpu.ebx;
	uint32_t size = (uint32_t)current_process->cpu.ecx;

	current_process->cpu.edx = (uint32_t) shm_create(key, size, current_process); // return value

	current_process->state = READY;
}

/*** Attach to a shared memory area ***/
void _0x94_shm_attach(void) {
	uint8_t key = (uint8_t)current_process->cpu.ebx;
	uint32_t mode = (uint32_t)current_process->cpu.ecx;

	current_process->cpu.edx = (uint32_t) shm_attach(key, mode, current_process); // return value

	current_process->state = READY;
}

/*** Detach from a shared memory area ***/
void _0x94_shm_detach(void) {
	shm_detach(current_process);
	
	current_process->state = READY;
}




