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
	disable_interrupts();

	//check if empty	
		if(processq_next == NULL) //&& processq_next->prev_PCB == NULL)
		{
			processq_next = p;
			//processq_next->next_PCB = p;

			p->next_PCB = p;
			p->prev_PCB = p;
			
			
		}
		else
		{
			//make tempHead
			PCB *temp = processq_next;
			p->next_PCB = temp;
			p->prev_PCB = temp->prev_PCB;
			p->prev_PCB->next_PCB = p;
			temp->prev_PCB = p;

	//		while(temp->next_PCB != processq_next)
	//		{
	//			temp = temp->next_PCB;	
	//		}	
	//		p->prev_PCB = temp;
	//		temp->next_PCB = p;

		}
	//next prev to p
		//

	// TODO: add process p to the queue of processes, always 
	// maintained as a circular doubly linked list;
	// processq_next always points to the next user process
	// that will get the time quanta;
	// if the process queue is non-empty, p should be added immediately
	// before processq_next
	// For details, read assignment background material

	enable_interrupts();

	return p;		
}

/*** Remove a TERMINATED process from process queue ***/
// Returns pointer to the next process in process queue
PCB *remove_from_processq(PCB *p) {
	// TODO: remove process p from the process queue
	//if(processq_next == NULL)
	//{
	//	return NULL;
	//}
	//else
	{


	// TODO: free the memory used by process p's image
	//PCB *temp = processq_next;
	// while(temp->next_PCB != p)
	 {
	 //	temp = temp->next_PCB;	

	 }
	PCB *tempAfterP = p->next_PCB;
	tempAfterP->prev_PCB = p->prev_PCB; //p->next_PCB;
	tempAfterP->prev_PCB->prev_PCB->next_PCB = tempAfterP;
	p->next_PCB = NULL;
	p->prev_PCB = NULL;

	//dealloc_memory(tempAfterP);
	dealloc_memory((void*)p->memory_base); //not a pointer itself
	dealloc_memory(p);
	// TODO: free the memory used by the PCB
	
	//COME BAXK AND FREE P
	//!!!!!!!!!!!!!!!!!!
		// TODO: return pointer to next process in list
	return tempAfterP;
	}
}


/*** Schedule a process ***/
// This function is called whenever a scheduling decision is needed,
// such as when the timer interrupts, or the current process is done
void schedule_something() { // no interruption when here

	// TODO: see assignment background material on what this function should do 
	//if(state.TERMINATED)

	PCB *temp = processq_next;
			while(temp->next_PCB != processq_next)
			{
				if(temp->state == TERMINATED)
				{
					temp = remove_from_processq(temp);

				}
				temp = temp->next_PCB;

			}
			temp = processq_next;
			//uint32_t gimmeTime;
				//PCB *temp = processq_next;
			while(temp->next_PCB != processq_next)
			{
				if(temp->state == WAITING)
				{
					if(get_epochs()> temp->sleep_end)
					{
						temp->state = READY;

					}
					

				}
				temp = temp->next_PCB;

			}	

			temp = processq_next;
			//uint32_t gimmeTime;
				//PCB *temp = processq_next;
			while(temp->next_PCB != processq_next)
			{
				if(temp->state == READY)
				{
					temp->state = RUNNING;
					current_process = temp;
					switch_to_user_process(temp);
				}
				temp = temp->next_PCB;
			}	
	// TODO: comment the following when you start working on this function
	switch_to_kernel_process(&console);
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


