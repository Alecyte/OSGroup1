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
    
	// TODO: Save flags in the console's PCB
	console.cpu.eflags = current_process->cpu.eflags;
	// TODO: Save current stack pointers (ESP and EBP) in the console's PCB
	console.cpu.ESP = current_process->cpu.ESP;
	console.cpu.EBP = current_process->cpu.EBP;

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
	TSS.esp0 = p->cpu.esp0;

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
	uint32_t tempLimit = (p->cpu.memory_limit << 16);
	gdt[3].limit_0_15 = (tempLimit >> 16);
	uint8_t flagMask = (0x0F);
	uint8_t tempFlag = ((flagMask & p->flag) << 4);
	uint32_t limitMask2 = (0xF0000u);
	uint8_t tempLimit2 = ((limitMask2 & p->limit) >> 16);
	gdt[3].limit_and_flag = (tempFlag | tempLimit2) ;
	gdt[3].access_byte = p->access_byte;

	uint32_t temo = (p->memory_base << 16);
	gdt[4].base_0_15 = (temo >> 16 );
	uint32_t baseMask2 = (0x00FF0000u);
	gdt[4].base_16_23 = ((p->memory_base & baseMask2) >> 16);
	uint32_t baseMask3 = (0xFF000000u);
	gdt[4].base_24_31 = ((p->memory_base & baseMask3) >> 24);
	uint32_t tempLimit = (p->cpu.memory_limit << 16);
	gdt[4].limit_0_15 = (tempLimit >> 16);
	uint8_t flagMask = (0x0F);
	uint8_t tempFlag = ((flagMask & p->flag) << 4);
	uint32_t limitMask2 = (0xF0000u);
	uint8_t tempLimit2 = ((limitMask2 & p->limit) >> 16);
	gdt[4].limit_and_flag = (tempFlag | tempLimit2) ;
	gdt[4].access_byte = p->access_byte;

	// TODO: load EDI, ESI, EAX, EBX, EDX, EBP with values from
    	// process p's PCB
	current_process->cpu.EDI = p->cpu.EDI;
	current_process->cpu.ESI = p->cpu.ESI;
	current_process->cpu.EAX = p->cpu.EAX;
	current_process->cpu.EBX = p->cpu.EBX;
	current_process->cpu.EDX = p->cpu.EDX;
	current_process->cpu.EBP = p->cpu.EBP;

    
	// TODO: Push into stack the following values from process p's PCB: SS,
    	// ESP, EFLAGS, CS, EIP (in this order)

	// TODO: load ECX with value from process p's PCB

	// TODO: load ES, DS, FS, GS registers with user data segment selector


	// TODO: execute the IRETL instruction

}



