////////////////////////////////////////////////////////
// Loads program and executes it in user mode
//

#include "kernel_only.h"


extern PCB *current_process; // from scheduler.c
uint32_t next_pid = 0;

/*** Parallel execution of a program ***/
/*** DO NOT UNCOMMENT ***/
// Loads n_sector number of sectors
// starting from sector LBA in disk and adds PCB to ready queue; 
// control returns to console, a.k.a. multi-tasking system
/*
void run(uint32_t LBA, uint32_t n_sectors) {
	uint8_t *load_base = NULL;
	uint32_t bytes_needed;
	PCB *user_program = NULL;

	// request memory to load program: we need n_sectors*512 bytes for program code;
	// we will ask for 16KB more for the bss, heap, stack, etc.
	bytes_needed = n_sectors*512+16384;
	load_base = (uint8_t *)alloc_memory(bytes_needed);
	if (load_base == NULL) {
		puts("run: Not enough memory.\n");
		return;
	}

	// request memory for PCB
	user_program = (PCB *)alloc_memory(sizeof(PCB));
	if (user_program == NULL) {
		dealloc_memory(load_base);
		puts("run: Not enough memory.\n");
		return;
	}
	
	// load the program into memory
	if (!load_disk_to_memory(LBA,n_sectors,load_base)) {
		puts("run: Load error.\n");
		return;
	}
 		
	// create PCB for user process
	// You implemented most of this in the previous assignment
	...
	// some more PCB stuff
	user_program->pid = next_pid++;
	user_program->state = NEW; // not yet ready to run
	user_program->sleep_end = 0; // used when process sleeps

	// add PCB to process queue and then return; process will start running when scheduled
	add_to_processq(user_program); // in scheduler.c
}
*/

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

