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

	switch (current_process->cpu.eax) {
		case SYSCALL_GETC: break; //_0x94_getc();  we only allow background processes here
		case SYSCALL_PRINTF: _0x94_printf();  break;
		case SYSCALL_SLEEP: _0x94_sleep(); break;
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

	// verify that parameter addresses are in process memory range
	if (current_process->cpu.ebx > current_process->memory_limit ||
		current_process->cpu.ecx > current_process->memory_limit) {
		current_process->cpu.edx = 0; // means error
		return;
	}

	_printf(format,args,current_process->memory_base);
	current_process->cpu.edx = 1; // success

	current_process->state = READY;
	
}

/*** Make process sleep ***/
void _0x94_sleep(void) {
	uint32_t tts = current_process->cpu.ebx;
	current_process->sleep_end = get_epochs() + tts/get_epoch_length(); // each epoch is 10ms long	
	current_process->state = WAITING;
}
