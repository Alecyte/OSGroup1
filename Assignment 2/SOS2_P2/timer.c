////////////////////////////////////////////////////////
// Everything about the Programmable Interval Timer (PIT)
//
//

#include "kernel_only.h"

extern PCB *current_process; // from scheduler.c
extern PCB console;

uint32_t elapsed_epoch;

/*** The timer (IRQ0) handler ***/
// We will save the state to current process' PCB,
// update the display clock, change the state of the current 
// process, and call the scheduler
asm(	"handler_timer_entry: \n"
	// CPU would have already pushed these in order:
	// [SS, ESP](only if not in Ring 0), EFLAGS, CS and EIP
	"pushal\n" // push all general purpose registers
	// save stack pointer
	"movl %esp, %ecx\n" 
	"jmp timer_interrupt_handler\n"
);
__attribute__((fastcall)) void timer_interrupt_handler() {
	// reload stack pointer (discards C function prologue)	
	asm volatile ("movl %ecx, %esp\n");

	// must reset the segment selectors before
	// accessing any kernel data
	asm volatile ("movl $0x10, %eax\n"
		      "movl %eax, %ds\n"
		      "movl %eax, %es\n"
		      "movl %eax, %fs\n"
		      "movl %eax, %gs\n");	

	if (current_process == &console) { // interrupted process was the console (i.e. in Ring 0)
		asm volatile ("movl %%esp, %0\n": "=r"(current_process->cpu.edi));
		asm volatile ("movl 4(%%esp), %0\n": "=r"(current_process->cpu.esi));
		asm volatile ("movl 8(%%esp), %0\n": "=r"(current_process->cpu.ebp));
		asm volatile ("movl 12(%%esp), %0\n": "=r"(current_process->cpu.esp));
		asm volatile ("movl 16(%%esp), %0\n": "=r"(current_process->cpu.ebx));
		asm volatile ("movl 20(%%esp), %0\n": "=r"(current_process->cpu.edx));
		asm volatile ("movl 24(%%esp), %0\n": "=r"(current_process->cpu.ecx));
		asm volatile ("movl 28(%%esp), %0\n": "=r"(current_process->cpu.eax));
		asm volatile ("movl 32(%%esp), %0\n": "=r"(current_process->cpu.eip));
		asm volatile ("movl 36(%%esp), %0\n": "=r"(current_process->cpu.cs));
		asm volatile ("movl 40(%%esp), %0\n": "=r"(current_process->cpu.eflags));
		// account for EFLAGS, CS and EIP pushed by CPU
		// adding 12 (=3*4) bytes ensures stack pointer is pointing at same location
		// as before the interrrupts
		current_process->cpu.esp += 12; 
	}
	else { // a regular Ring 3 user process was interrupted
		asm volatile ("movl %%esp, %0\n": "=r"(current_process->cpu.edi));
		asm volatile ("movl 4(%%esp), %0\n": "=r"(current_process->cpu.esi));
		asm volatile ("movl 8(%%esp), %0\n": "=r"(current_process->cpu.ebp));
		asm volatile ("movl 16(%%esp), %0\n": "=r"(current_process->cpu.ebx));
		asm volatile ("movl 20(%%esp), %0\n": "=r"(current_process->cpu.edx));
		asm volatile ("movl 24(%%esp), %0\n": "=r"(current_process->cpu.ecx));
		asm volatile ("movl 28(%%esp), %0\n": "=r"(current_process->cpu.eax));
		asm volatile ("movl 32(%%esp), %0\n": "=r"(current_process->cpu.eip));
		asm volatile ("movl 36(%%esp), %0\n": "=r"(current_process->cpu.cs));
		asm volatile ("movl 40(%%esp), %0\n": "=r"(current_process->cpu.eflags));
		asm volatile ("movl 44(%%esp), %0\n": "=r"(current_process->cpu.esp));
		asm volatile ("movl 48(%%esp), %0\n": "=r"(current_process->cpu.ss));
	}
 

	if (current_process->state == RUNNING) current_process->state = READY;

	elapsed_epoch++; // each epoch is 10ms long

	update_display_time();

	// the PIC masks interrupts when they are being serviced;
	// notify the PIC that interrupt has been serviced,
	// otherwise the interrupt will be ignored in future
	port_write_byte(0x20,0x20);

	// invoke scheduler
	schedule_something();
}

/*** Returns number of milliseconds since start ****/
uint32_t get_uptime() {
	return elapsed_epoch*get_epoch_length(); // each epoch is 10ms long
}

/*** Returns number of epochs since start ****/
uint32_t get_epochs() {
	return elapsed_epoch; // each epoch is 10ms long
}

/*** Return duration of one epoch in milliseconds ***/
uint32_t get_epoch_length() {
	return 10; // each epoch is 10ms long
}

/*** Initialize timer ***/
void init_timer() {
	// register timer handler
	// timer generates IRQ0, which is mapped to interrupt 32 (see setup_PIC)
	install_interrupt_handler(32,handler_timer_entry,0x0008,0x8E);

	elapsed_epoch = 0;

	// setup timer to go off every 10ms
	// The PIT works at a fequency of 1193182 Hz; we want a timer interrupt
	// every 10 milliseonds; a divider of 11931 gives us 100 pulses per
	// second, i.e. one pulse (interrupt) every 10 milliseconds
	uint16_t divider = (uint16_t)11931; 

	// use counter 0 in mode 2 (rate generator)
	port_write_byte(0x43,0x34);

	// set the timer count
	port_write_byte(0x40,(divider & 0xFF)); // LSBs
	port_write_byte(0x40,(divider >> 8) & 0xFF); //MSBs
}

