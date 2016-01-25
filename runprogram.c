////////////////////////////////////////////////////////
// Loads program and executes it in user mode
//

#include "kernel_only.h"

//kris commit
//version 2
extern GDT_DESCRIPTOR gdt[6];	// from startup.S
extern TSS_STRUCTURE TSS;	// from systemcalls.c

PCB console;	// PCB of the console (==kernel)
PCB user_program; // PCB of running program
PCB *current_process; // should point to user_program before control transfer

/*** Sequential execution of a program ***/
// Loads n_sector number of sectors
// starting from sector LBA in disk and executes; 
// kernel blocks until this program finishes, a.k.a.
// single-tasking system
void run(uint32_t LBA, uint32_t n_sectors) {
	uint8_t *load_base = NULL;
	uint32_t bytes_needed;

	// request memory: we need n_sectors*512 bytes for program code;
	// we will ask for 16KB more for the user-mode and kernel-mode stacks
	bytes_needed = n_sectors*512+16384;
	load_base = (uint8_t *)alloc_memory(bytes_needed);
	if (load_base == NULL) {
		puts("run: Not enough memory.\n");
		return;
	}

	// load the program into memory
	if (!load_disk_to_memory(LBA,n_sectors,load_base)) {
		puts("run: Load error.\n");
		return;
	}
 	
    	// Save some state information about the console in its PCB
    	// so that we return back here; the PCB of the console is defined
    	// in the beginning of this file; the PCB structure is defined in
    	// kernel_only.h.
    puts("hey\n");
	// TODO: Save flags in the console's PCB
	
	/* console.cpu.eflags = current_process->cpu.eflags; */
	
	asm volatile("pushfl\n");	
	asm volatile("popl %0\n": "=r"(console.cpu.eflags));
	
	// TODO: Save current stack pointers (ESP and EBP) in the console's PCB

	/*console.cpu.esp = current_process->cpu.esp;
	console.cpu.ebp = current_process->cpu.ebp;*/
	asm volatile ("movl %%esp,%0" : "=r"(console.cpu.esp));
	asm volatile("movl %%ebp,%0" : "=r"(console.cpu.ebp));


	
	

	// save resume point: we will resume at forward label 1 (below)
	asm volatile ("movl $1f,%0" : "=r"(console.cpu.eip));
	
	// fill data in the PCB for the user program (defined in the
    	// beginning of this file)
    	// TODO: fill into user_program the following values
    	//          a) memory base
    	//          b) memory limit
    	//          c) stack segment (SS)
    	//          d) stack pointer (ESP): stack should begin from (end of process space - 4KB)
    	//          e) code segment (CS)
    	//          f) instruction pointer (EIP)
    	//          g) flags (EFLAGS)

	user_program.memory_base = *load_base;
	user_program.memory_limit = bytes_needed;

	asm volatile ("movl %%ss,%0" : "=r"(user_program.cpu.ss));
	user_program.cpu.esp = (*load_base + bytes_needed) - 4096;
    asm volatile ("movl %%cs,%0" : "=r"(user_program.cpu.cs));
    asm volatile ("1: movl $1b, %0" : "=r" (user_program.cpu.eip));
    asm volatile ("pushfl\n");
    asm volatile ("popl %0\n": "=r"(user_program.cpu.eflags));

	/*user_program.memory_base = current_process->memory_base;
	user_program.memory_limit = current_process->memory_limit;
	user_program.cpu.ss = current_process->cpu.ss;
	user_program.cpu.esp = current_process->cpu.esp;
	user_program.cpu.cs = current_process->cpu.cs;
	user_program.cpu.cs = current_process->cpu.eip;
	user_program.cpu.cs = current_process->cpu.eflags;*/

	current_process = &user_program;
	switch_to_user_process(current_process);
	
	// all programs execute interrupt 0xFF upon termination
	// this is where execution will resume after user program ends
	// !!BACK IN KERNEL MODE!!
	asm volatile ("1:\n");	
}

/*** Load the user program to memory ***/
bool load_disk_to_memory(uint32_t LBA, uint32_t n_sectors, uint8_t *mem) {
	uint8_t status;
	uint16_t read_count = 0;

	// read up to 256 sectors at a time
	for (;n_sectors>0;) {
		read_count = (n_sectors>=256?256:n_sectors);

		status = read_disk(LBA,(read_count==256?0:read_count),mem);
		
		if (status == DISK_ERROR_LBA_OUTSIDE_RANGE
			|| status == DISK_ERROR_SECTORCOUNT_TOO_BIG) 
			return FALSE;

		else if (status == DISK_ERROR || status == DISK_ERROR_ERR
				|| status == DISK_ERROR_DF) 
			return FALSE;

		n_sectors -= read_count;
		LBA += read_count;
		mem += 512*read_count;
	}

	return TRUE;
}

/*** Switch to process described by the PCB ***/
// We will use the "fastcall" keyword to force GCC to pass 
// the pointer in register ECX
__attribute__((fastcall)) void switch_to_user_process(PCB *p) {
	// update TSS to tell where the kernel-mode stack begins; 
	// we will use the last 4KB of the process address space
	TSS.ss0 = 0x10; // must be kernel data segment with RPL=0
    	// TODO: set TSS.esp0 
	TSS.esp0 = p->cpu.esp;

	// set up GDT entries 3 and 4
	// TODO: set user GDT code/data segment to base = p->memory_base,
	// limit = p->memory_limit, flag, and access byte (see kernel_only.h
    	// for definition of the GDT structure)

	uint32_t temo = (p->memory_base << 16);
	gdt[3].base_0_15 = (temo >> 16 );
	uint32_t baseMask2 = (0x00FF0000u);
	gdt[3].base_16_23 = ((p->memory_base & baseMask2) >> 16);
	uint32_t baseMask3 = (0xFF000000u);
	gdt[3].base_24_31 = ((p->memory_base & baseMask3) >> 24);
	uint32_t tempLimit = (p->memory_limit << 16);
	gdt[3].limit_0_15 = (tempLimit >> 16);
	//uint8_t flagMask = (0x0F);
	//uint8_t tempFlag = ((flagMask & p->flag) << 4);
	uint32_t limitMask2 = (0xF0000u);
	uint8_t tempLimit2 = ((limitMask2 & p->memory_limit) >> 16);
	gdt[3].limit_and_flag = (0xC| tempLimit2) ;
	gdt[3].access_byte = (0xFA);


	uint32_t temoRE = (p->memory_base << 16);
	gdt[4].base_0_15 = (temoRE >> 16 );
	uint32_t baseMask2RE = (0x00FF0000u);
	gdt[4].base_16_23 = ((p->memory_base & baseMask2RE) >> 16);
	uint32_t baseMask3RE = (0xFF000000u);
	gdt[4].base_24_31 = ((p->memory_base & baseMask3RE) >> 24);
	uint32_t tempLimitRE = (p->memory_limit << 16);
	gdt[4].limit_0_15 = (tempLimitRE >> 16);
	//uint8_t flagMaskRE = (0x0F);
	//uint8_t tempFlagRE = ((flagMaskRE & p->flag) << 4);
	uint32_t limitMask2RE = (0xF0000u);

	uint8_t tempLimit2RE = ((limitMask2RE & p->memory_limit) >> 16);
	gdt[4].limit_and_flag = (0xC | tempLimit2RE) ;
	gdt[4].access_byte = (0xF2);


//print what we expect to be in there & check what is
	//sys_printf ->prints to console
	//include lib.h
	//print gdt entries as well
	//what's up with the new line?


	// TODO: load EDI, ESI, EAX, EBX, EDX, EBP with values from
    	// process p's PCB
	asm volatile ("movl %0, %%edi\n": :"m"(p->cpu.edi));
	asm volatile ("movl %0, %%esi\n": :"m"(p->cpu.esi));
	asm volatile ("movl %0, %%eax\n": :"m"(p->cpu.eax));
	asm volatile ("movl %0, %%ebx\n": :"m"(p->cpu.ebx));
	asm volatile ("movl %0, %%edx\n": :"m"(p->cpu.edx));
	asm volatile ("movl %0, %%ebp\n": :"m"(p->cpu.ebp));


    
	// TODO: Push into stack the following values from process p's PCB: 
    	// SS, ESP, EFLAGS, CS, EIP (in this order)
	asm volatile ("pushl %0\n": :"m"(p->cpu.ss));
	asm volatile ("pushl %0\n": :"m"(p->cpu.esp));
	asm volatile ("pushl %0\n": :"m"(p->cpu.eflags));
	asm volatile ("pushl %0\n": :"m"(p->cpu.cs));
	asm volatile ("pushl %0\n": :"m"(p->cpu.eip));


	// TODO: load ECX with value from process p's PCB
	asm volatile ("movl %0, %%ecx\n": :"m"(p->cpu.ecx));

		// TODO: execute the IRETL instruction (was last)

	asm volatile ("IRETL\n");

	// TODO: load ES, DS, FS, GS registers with user data segment selector
	asm volatile ("movw %0, %%es\n": :"m"(TSS.ss0));
	asm volatile ("movw %0, %%ds\n": :"m"(TSS.ss0));
	asm volatile ("movw %0, %%fs\n": :"m"(TSS.ss0));
	asm volatile ("movw %0, %%gs\n": :"m"(TSS.ss0));





}



