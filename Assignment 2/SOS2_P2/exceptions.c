////////////////////////////////////////////////////////
// Functions to set up exception handlers 
// (very much like system call handlers)
//

#include "kernel_only.h"

extern PCB *current_process;		// from scheduler.c

/*** The all purpose exception handler ***/
// Simply kills the current process and schedules something
// Same as our 0xFF syscall handler
void default_exception_handler(void) {
	// must reset the segment selectors before
	// accessing any kernel data
	asm volatile ("movl $0x10, %eax\n"
		      "movl %eax, %ds\n"
		      "movl %eax, %es\n"
		      "movl %eax, %fs\n"
		      "movl %eax, %gs\n");
	
	// Why this weird way of printing? 
	puts("\n");
	set_color(WHITE,RED);
	puts(" OUCHH! Fatal exception. ");
	set_color(LIGHT_GRAY,BLACK);
	puts("\n");

	current_process->state = TERMINATED;
	schedule_something();
}


/*** Initialize exception handlers ***/
void init_exceptions(void) {
	int i;

	// exception handlers are set up as interrupt gates (cli automatic)
	for (i=0; i<32; i++) // all of these are exceptions
		install_interrupt_handler(i,default_exception_handler,0x0008,0x8E);
}
