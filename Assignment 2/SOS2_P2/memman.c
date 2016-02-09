////////////////////////////////////////////////////////
// The NAIVE Memory Manager
// 
// Divides memory into 4KB frames and allocates from them

#include "kernel_only.h"

// we will use a memory bitmap to track used memory in
// multiples of 4KB frames; bitmap will be placed at 1MB mark;
// 64MB (max allowed memory) will require 2KB for bimtap, so
// memory from 0x100000 to 0x1007FF should not be given to user
uint8_t *mem_bitmap = (uint8_t *)0x100000; // 0x100000 is frame 256
uint16_t mem_bitmap_size;

extern uint64_t total_memory;

uint32_t total_frames; // max 16384 (*4KB = 64MB)

/*** Initialize memory manager ***/
void init_memory_manager(void) {
	int i;

	total_frames = total_memory/4; // frame size is 4KB
	mem_bitmap_size = total_frames/8; // each bitmap byte can track 8 frames

	// initialize memory bitmap (0 occupied; 1 available)
	for (i=0; i<mem_bitmap_size; i++) mem_bitmap[i]=0xFF;
	// we will keep aside the first 4MB
	for (i=0; i<128; i++) mem_bitmap[i]=0; // 128*8*4KB = 4MB
}

/*** Allocate count bytes of memory ***/
// Finds contiguous frames of memory to fit count
// bytes (in multiples of 4KB frames)
// Returns NULL if unable to find; otherwise found location + 4bytes
// Writes no. of allocated frames in first 4 bytes of found location
void *alloc_memory(uint32_t count) {
	int i;
	uint32_t *alloc_base = NULL; // memory address to return

	uint32_t n_frames = (count+4)/(4096); // number of 4KB frames
	if ((count+4) % 4096 != 0) n_frames++;

	if (n_frames > (total_frames-1024)) return NULL;

	uint32_t start_frame = find_frames(n_frames);

	if (start_frame != 0) {
		alloc_base = (uint32_t *)(start_frame*4096);

		// the first 4 bytes will store number of frames allocated;
		// will be used when de-allocating
		*alloc_base = n_frames; 
		alloc_base++;

		// update memory bitmap
		/*for (i=start_frame/8; n_frames > 0; i++) {
			if (n_frames >= 8) {
				mem_bitmap[i] = 0;
				n_frames -= 8;
			}
			else {
				mem_bitmap[i] &= (0xFF >> n_frames);
				n_frames = 0;
			}
		}*/
		modify_bitmap(start_frame,n_frames,0);

	}

	return (void *)alloc_base;
}

/*** Finds n_frames of free contiuguous memory ***/
// Returns frame number of found memory; 0 otherwise
uint32_t find_frames(uint32_t n_frames) {
	if (n_frames == 0) return 0;

	uint32_t start_frame = 0;
	uint32_t found_frames = 0;
	uint32_t i=1024/8, j; // 1024th frame is 4MB mark

	while (i < total_frames) {

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
// Number of frames to deallocate is written in the 4 bytes
// before the location loc
void dealloc_memory(void *loc) {
	int i;
	uint32_t *alloc_base = (uint32_t *)loc;
	
	uint32_t n_frames = *(alloc_base-1); // number of 4KB frames
	uint32_t start_frame = ((uint32_t)(alloc_base-1))/4096; 

	// update memory bitmap
	modify_bitmap(start_frame,n_frames,1);
}









