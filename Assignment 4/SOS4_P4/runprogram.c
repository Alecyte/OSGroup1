////////////////////////////////////////////////////////
// Loads program and executes it in user mode
//

#include "kernel_only.h"


extern PCB *current_process; // from scheduler.c
extern PDE *k_page_directory; // from lmemman.c

uint32_t next_pid = 0;

/*** Parallel execution of a program ***/
// Loads n_sector number of sectors
// starting from sector LBA in disk and adds PCB to ready queue; 
// control returns to console, a.k.a. multi-tasking system;
// programs run as background processes (blocks forever if getc is used)

/** DO NOT UNCOMMENT THE FOLLOWING ***/
/*
void run(uint32_t LBA, uint32_t n_sectors) {
	// you implemented this in a previous assignment
	...

	user_program->mutex.wait_on = -1; // not waiting on any mutex
	user_program->semaphore.wait_on = -1; // not waiting on any semaphore
	user_program->shared_memory.created = FALSE; // no shared memory objects yet

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

