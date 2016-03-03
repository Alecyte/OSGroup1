////////////////////////////////////////////////////////
// The NAIVE Physical Memory Manager
// 
// Divides memory into 4KB frames and allocates from them

#include "kernel_only.h"

// We will use a memory bitmap to track used memory in
// multiples of 4KB frames; bitmap will be placed at 1MB mark;
// 64MB (max allowed memory) will require 2KB for bimtap
// memory from 0x100000 to 0x1007FF should not be given to user
uint8_t *mem_bitmap = (uint8_t *)0xC0100000; // 0x100000 is frame 256
uint16_t mem_bitmap_size;

extern uint64_t total_memory;

uint32_t total_frames; // max 16384 (*4KB = 64MB)


/*** Initialize physical memory manager ***/
void init_physical_memory_manager(void) {
	uint32_t i;

	total_frames = total_memory/4; // frame size is 4KB
	mem_bitmap_size = total_frames/8; // each bitmap byte can track 8 frames

	// initialize memory bitmap (0 occupied; 1 available)
	for (i=0; i<mem_bitmap_size; i++) mem_bitmap[i]=0xFF;
	// everything upto 1MB + 12KB is considered under use
	for (i=0; i<32; i++) mem_bitmap[i]=0; // 32*8*4KB = 1MB
	mem_bitmap[32] = 0x1F; // frames 256, 257 and 258 occupied (see lmemman.c)

}

/*** Allocate frames from user memory***/
// Finds contiguous frames of memory to fit n_frames (each 4KB)
// Returns NULL if unable to find; otherwise first frame address
// Use mode = KERNEL_ALLOC to allocation from first 4MB; mode = 
// USER_ALLOC otherwise
void *alloc_frames(uint32_t n_frames, bool mode) {
	int i;
	uint32_t *alloc_base = NULL; // memory address to return

	if (mode==KERNEL_ALLOC && n_frames > (total_frames-264)) return NULL;
	if (mode==USER_ALLOC && n_frames > (total_frames-1024)) return NULL;

	uint32_t start_frame;
	if (mode==KERNEL_ALLOC)
		start_frame = find_frames(n_frames,264,1023); // kernel memory always from first 4MB
	else 
		start_frame = find_frames(n_frames,1024,total_frames); //starting from 4MB mark


	if (start_frame != 0) {
		alloc_base = (uint32_t *)(start_frame*4096);
		// update memory bitmap
		modify_bitmap(start_frame,n_frames,0);
	}
	
	return (void *)alloc_base;
}

/*** Finds n_frames of free contiguous memory ***/
// Returns frame number of found memory; 0 otherwise
uint32_t find_frames(uint32_t n_frames, uint32_t from, uint32_t to) {
	if (n_frames == 0) return 0;

	uint32_t start_frame = 0;
	uint32_t found_frames = 0;
	uint32_t i=from/8, j, k=to/8;

	while (i < k) {

		if (mem_bitmap[i] == 0) { // all zeros mean none available here
			i++;
			continue;
		}
		
		for (j=0; j<8; j++) {
			if ((mem_bitmap[i] << j) & 0x80) {
				found_frames++;
				if (start_frame==0) start_frame = i*8 + j;			
			}		
			else { // start looking again
				found_frames = 0;
				start_frame = 0;
			}

			if (found_frames == n_frames) return start_frame;
		}
		i++;
	}

	// looked through entire bitmap without success
	return 0;
}

/*** Set/Unset memory bitmap ***/
// Sets/unsets the memory bitmap, starting at start_frame and continues
// for n_frames
// set (1) - frame becomes available
// unset (0) - frames becomes unavailable
void modify_bitmap(uint32_t start_frame, uint32_t n_frames, bool set) {
	uint32_t i = start_frame/8;
	uint8_t j = start_frame%8;

	// update memory bitmap
	while (n_frames > 0) {
		if (set) 
			mem_bitmap[i] |= ((uint32_t)1 << (8-j-1)); //make jth bit one
		else
			mem_bitmap[i] &= ~((uint32_t)1 << (8-j-1)); //make jth bit zero
		n_frames--;
		j++;

		if (j==8) { // move on to next 8 bits
			j=0;
			i++;
		}
	}
}

/*** Deallocate memory ***/
// Deallocate n_frames frames; first frame is the one
// corrsponding to physical address <loc>
void dealloc_frames(void *loc, uint32_t n_frames) {
	uint32_t start_frame = ((uint32_t)loc)/4096; // address to frame number
	modify_bitmap(start_frame, n_frames, 1);
}


/*** Number of frames required for given bytes ***/
uint32_t bytes_to_frames(uint32_t count) {
	uint32_t n_frames = count/4096; // number of 4KB frames
	if (count % 4096 != 0) n_frames++;
	
	return n_frames;
}

/*** Return number of bytes free ***/
uint32_t count_free_memory() {
	int i,j;
	uint32_t total = 0;
	for (i=0; i<mem_bitmap_size; i++) {
		if (mem_bitmap[i]==0xFF) total+=8*4096; 
		else {
			for (j=0; j<8; j++) 
				total += 4096* ((mem_bitmap[i]>>j) & 0x01);
		}
	}
	return total;				
}



