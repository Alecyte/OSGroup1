////////////////////////////////////////////////////////
// A Round-Robin Scheduler with 50% share to console
// 
// Process queue is maintained as a doubly linked list.
// Eventually processes should be on different queues based 
//       on their state.

#include "kernel_only.h"

extern GDT_DESCRIPTOR gdt[6];	// from startup.S
extern TSS_STRUCTURE TSS;	// from systemcalls.c

PCB console;	// PCB of the console (==kernel)
PCB *current_process; // the currently running process
PCB *processq_next = NULL; // the next user program to run

void init_scheduler() {
	current_process = &console; // the first process is the console
}

/*** Add process to process queue ***/
// Returns pointer to added process
PCB *add_to_processq(PCB *p) {
	PCB *backSide = processq_next->prev_PCB;
	disable_interrupts();

	// TODO: add process p to the queue of processes, always 
	// maintained as a circular doubly linked list;
	// processq_next always points to the next user process
	// that will get the time quanta;
	// if the process queue is non-empty, p should be added immediately
	// before processq_next
	// For details, read assignment background material
	if (processq_next == NULL) {
		processq_next = p;
		p->next_PCB = p;
		p->prev_PCB = p;
	}
	else {
		p->next_PCB = processq_next;
		processq_next->prev_PCB = p;
		p->prev_PCB = backSide;
		backSide->next_PCB = p;
	}
	p->state = READY;

	enable_interrupts();

	return p;		
}

/*** Remove a TERMINATED process from process queue ***/
// Returns pointer to the next process in process queue
PCB *remove_from_processq(PCB *p) {
	// TODO: remove process p from the process queue
	PCB *frontSide;
	PCB *backSide;
	PCB *value;
	frontSide = p->next_PCB;
	backSide = p->prev_PCB;

	if (frontSide == p) {
		processq_next = NULL;
		frontSide = NULL;
		backSide = NULL;
		value = NULL;	
	}
	else if (processq_next == p) {
		processq_next = frontSide;
		value = processq_next;		
	}
	else {
		frontSide->prev_PCB = backSide;
		backSide->next_PCB = frontSide;
		value = frontSide;
	}

	// TODO: free the memory used by process p's image
	dealloc_memory((uint32_t*)p->memory_base);

	// TODO: free the memory used by the PCB
	dealloc_memory(p);	

	// TODO: return pointer to next process in list
	return value;
}

/*** Schedule a process ***/
// This function is called whenever a scheduling decision is needed,
// such as when the timer interrupts, or the current process is done
void schedule_something() { // no interruption when here
	PCB *p = processq_next;

	// TODO: see assignment background material on what this function should do
	do {
		// 1. clean up processes that are in the TERMINATED state
		if ((p->state == WAITING) && (p->sleep_end <= get_epochs())) {
			// and wake up processes that are in the sleep state
			// 2. change the state of processes to READY if current time epoch is larger or equal to the one set in sleep_end
			p->state = READY;	
		}	
		else if (p->state == TERMINATED) {
			p = remove_from_processq(p);
		}
		
		p = p->next_PCB;	

	} while (p != processq_next);		
	
	if (current_process == &console) {
		if (processq_next == NULL) {
			current_process = &console;
			switch_to_kernel_process(&console);
		} 
		else {
			p = processq_next;
			do {
				if (processq_next->state == READY) {
					PCB *temp = processq_next;
					current_process = processq_next;
					current_process->state = RUNNING;
					switch_to_user_process(temp);
					break;
				}		
				processq_next = processq_next->next_PCB;

			} while (processq_next != p);		

			if (processq_next == p) {
				current_process = &console;
				switch_to_kernel_process(&console);
			}
		}
	}

	// TODO: comment the following when you start working on this function
	else {
		current_process = &console;
		switch_to_kernel_process(&console);
	}
}

/*** Switch to kernel process described by the PCB ***/
// We will use the "fastcall" keyword to force GCC to pass 
// the pointer in register ECX;
// process switched to is a kernel process; so no ring change
__attribute__((fastcall)) void switch_to_kernel_process(PCB *p)  {

	// load CPU state from process PCB
	asm volatile ("movl %0, %%edi\n": :"m"(p->cpu.edi));
	asm volatile ("movl %0, %%esi\n": :"m"(p->cpu.esi));
	asm volatile ("movl %0, %%eax\n": :"m"(p->cpu.eax));
	asm volatile ("movl %0, %%ebx\n": :"m"(p->cpu.ebx));
	asm volatile ("movl %0, %%edx\n": :"m"(p->cpu.edx));
	asm volatile ("movl %0, %%ebp\n": :"m"(p->cpu.ebp));
	asm volatile ("movl %0, %%esp\n": :"m"(p->cpu.esp));

	// switching within the same ring; IRET requires the following in stack (see IRET details)
	asm volatile ("pushl %0\n"::"m"(p->cpu.eflags));
	asm volatile ("pushl %0\n": :"m"(p->cpu.cs));
	asm volatile ("pushl %0\n": :"m"(p->cpu.eip));

	// this should be the last one to be copied
	asm volatile ("movl %0, %%ecx\n": :"m"(p->cpu.ecx));

	// issue IRET; see IRET details
	asm volatile("sti\n"); // interrupts cleared in timer/syscall handler
	asm volatile("iretl\n"); // this completes the timer/syscall interrupt
}

/*** Switch to user process described by the PCB ***/
/*** DO NOT UNCOMMENT ***/
/*
__attribute__((fastcall)) void switch_to_user_process(PCB *p) {

	// You implemented this in the previous assignment
	...

}*/


