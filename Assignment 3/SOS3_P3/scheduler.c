////////////////////////////////////////////////////////
// A Round-Robin Scheduler with 50% share to console
// 
// Process queue is maintained as a doubly linked list
// TODO: processes should be on different queues based 
//       on their state

#include "kernel_only.h"

extern GDT_DESCRIPTOR gdt[6];	// from startup.S
extern TSS_STRUCTURE TSS;	// from systemcalls.c

extern PDE *k_page_directory;	// from lmemman.c

PCB console;	// PCB of the console (==kernel)
PCB *current_process; // the currently running process
PCB *processq_next = NULL; // the next user program to run

void init_scheduler() {
	current_process = &console; // the first process is the console
}

/*** Add process to process queue ***/
// Returns pointer to added process

/** DO NOT UNCOMMENT THE FOLLOWING **/
/*
PCB *add_to_processq(PCB *p) {
	disable_interrupts();

	// you implemented this in a previous assignment
	...

	// NOTE: process is not yet READY to run since the program code
	// has not been loaded yet

	return p;		
}
*/

/*** Remove a TERMINATED process from process queue ***/
// Returns pointer to the next process in process queue

/** DO NOT UNCOMMENT THE FOLLOWING **/
/*
PCB *remove_from_processq(PCB *p) {
	PCB *ret;

	// you implemented this in a previous assignment
	...

	// free used pages
	dealloc_all_pages((PDE *)((uint32_t) p->mem.page_directory + KERNEL_BASE));
	// free page used to store PCB
	dealloc_page((void *)p,(PDE *)((uint32_t) p->mem.page_directory + KERNEL_BASE));
	// free frame used to store page directory
	dealloc_frames((void *)((uint32_t)p->mem.page_directory & 0xFFFFF000), 1);

	// load kernel page directory
	load_CR3((uint32_t)k_page_directory-KERNEL_BASE);

	return ret;
}
*/

/*** Schedule a process ***/
// Toggle between console and a user program;
// user program is chosen from the process queue in
// round-robin fashion

/** DO NOT UNCOMMENT THE FOLLOWING **/
/*
void schedule_something() { // no interruption when here
	
	...
	
	// load the program from disk if state=NEW
	if (processq_next->state == NEW) {
		load_CR3((uint32_t)processq_next->mem.page_directory);
		if (!load_disk_to_memory(processq_next->disk.LBA,processq_next->disk.n_sectors,0)) {
			sys_printf("run: Load error (%u,%u).\n",
					processq_next->disk.LBA,
					processq_next->disk.n_sectors);
			processq_next->state = TERMINATED;
		}
		else {
			processq_next->state = READY;
		}
	}

	...
	// you implemented this in a previous assignment
}
*/

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
// We will use the "fastcall" keyword to force GCC to pass 
// the pointer in register ECX
// a ring change will be necessary here

/** DO NOT UNCOMMENT THE FOLLOWING **/
/*
__attribute__((fastcall)) void switch_to_user_process(PCB *p) {

	// Note: user code and data GDTs already set up in startup.S
	// load process page table
	asm volatile ("movl %0, %%eax\n": :"m"((uint32_t)p->mem.page_directory));
	asm volatile ("movl %eax, %cr3\n");

	// VirtualBox nonsense: if we do not touch the TSS stack
	// VirtualBox crashes since it does not sync the page tables
	// corresponding to this address
	asm volatile ("movb $0, 0xBFBFFFFF\n");

	... 
	// you implemented this in a previous assignment
}
*/


