///////////////////////////////////////////////////////
// Shared Memory Implementation
// This implementation provides SHMEM_MAXNUMBER shared memory
// objects for use by user processes; the object number is specified 
// using an 8-bit number (called key), which restricts us to have up to
// 256 shared memory objects.
// Shared memory objects persist until the number of references to 
// it comes down to zero, when the space is deallocated
// TODO: Allow object creation using alphanumeric keys

#include "kernel_only.h"

SHMEM shm[SHMEM_MAXNUMBER];	// the shared memory objects; maximum 256 of them

/*** Initialize all shared memory objects ***/
void init_shared_memory() {
	int i;
	for (i=0; i<SHMEM_MAXNUMBER; i++) {
		shm[i].refs = 0;
		shm[i].base = NULL;
	}
}

/*** Create a shared memory object ***/
// Shared memory area will always start at logical 0x80000000 (SHM_BEGIN);
// a process can create only one shared memory object at a time with a
// maximum size of 4MB
// At least one process must create the shared memory before others
// can use it using the key
void  *shm_create(uint8_t key, uint32_t size, PCB *p) {
	// some sanity checks: size should not be zero; size should not be
	// more than 4MB; object should not be in use; process should not
	// have created another shared memory object
	if (size == 0 || size > 0x400000 || shm[key].refs != 0) return NULL; 
	if (p->shared_memory.created) return NULL; // already created one area; unlink from it first

	// how many pages does <size> bytes take
	uint32_t n_pages = size/4096;   
	if (size % 4096 != 0) n_pages++;

	// logical address pointer of page directory
	PDE *page_directory = (PDE *)((uint32_t)p->mem.page_directory + KERNEL_BASE);

	// allocate pages for user process; alloc_user_pages will update the page 
	// directory and page tables as necessary
	if (alloc_user_pages(n_pages, SHM_BEGIN, page_directory, PTE_READ_WRITE)==NULL)
		return NULL;

	
	// remember the start frame address of the allocated memory;
	// this will be used when other processes attach to this shared memory object
	uint32_t pd_entry = SHM_BEGIN >> 22; // 0x200
	uint32_t pt_entry = (SHM_BEGIN >> 12) & 0x000003FF; // 0x0  
	PTE *l_pages = (PTE *)((page_directory[pd_entry] & 0xFFFFF000) + KERNEL_BASE);
	shm[key].base = l_pages[pt_entry] & 0xFFFFF000;
	shm[key].size = size;

	shm[key].refs++;
	p->shared_memory.created = TRUE;
	p->shared_memory.key = key;

	return (void *)SHM_BEGIN; // return logical address of shared memory area start
}

/*** Attach to a shared memory area ***/
// A process can attach to an already created shared memory area using
// the key; mode is SHM_READ_ONLY or SHM_READ_WRITE
void *shm_attach(uint8_t key, uint32_t mode, PCB *p) {
	int i;
	
	if (shm[key].refs == 0) return NULL; // not yet created

	if (p->shared_memory.created) return NULL; // already attached to an object
	
	// size of shared memory in number of pages
	uint32_t n_pages = shm[key].size/4096;
	if (shm[key].size % 4096 != 0) n_pages++;

	// logical address pointer of page directory
	PDE *page_directory = (PDE *)((uint32_t)p->mem.page_directory + KERNEL_BASE);

	// paging entries corresponding to shared memory base
	uint32_t pd_entry = SHM_BEGIN >> 22; // 0x200
	uint32_t pt_entry = (SHM_BEGIN >> 12) & 0x000003FF; // 0x0  

	// allocate space for page table, if needed
	uint32_t pt_frame;
	if ((uint32_t)(page_directory[pd_entry] & PDE_PRESENT) == 0) { // entry not present
		// allocate space for page table and set entry
		if ((pt_frame = (uint32_t)alloc_frames(1, KERNEL_ALLOC)) == NULL)
			return NULL;

		page_directory[pd_entry] = pt_frame | PDE_PRESENT | PDE_READ_WRITE | PDE_USER_SUPERVISOR;
		zero_out_pages((void *)(pt_frame + KERNEL_BASE), 1); // just to be sure
	}			
		
	// logical address pointer of page table
	PTE *l_pages = (PTE *)((page_directory[pd_entry] & 0xFFFFF000) + KERNEL_BASE);
	
	// modify page table entries to point to shared frames
	// CAUTION: if page is already mapped, it will not be changed
	for (i=pt_entry; i<pt_entry+n_pages; i++) {
		if ((uint32_t)(l_pages[i] & PTE_PRESENT) == 0)
			l_pages[i] =  (shm[key].base + (i-pt_entry)*4096) | mode | PTE_PRESENT | PTE_USER_SUPERVISOR;
	}

	shm[key].refs++;
	p->shared_memory.created = TRUE;
	p->shared_memory.key = key;

	return (void *)SHM_BEGIN; // return logical address of shared memory area start
}

/***  Unlink from a shared memory area ***/
void shm_detach(PCB *p) {
	int i;
	if (p->shared_memory.created) { // only if process has attached to object
		shm[p->shared_memory.key].refs--;
		p->shared_memory.created = FALSE;
	
		// size of shared memory in number of pages
		uint32_t n_pages = shm[p->shared_memory.key].size/4096;
		if (shm[p->shared_memory.key].size % 4096 != 0) n_pages++;

		// logical address pointer of page directory
		PDE *page_directory = (PDE *)((uint32_t)p->mem.page_directory + KERNEL_BASE);

		// paging entries corresponding to shared memory base
		uint32_t pd_entry = SHM_BEGIN >> 22; // 0x200
		uint32_t pt_entry = (SHM_BEGIN >> 12) & 0x000003FF; // 0x0  

		// logical address pointer of page table
		PTE *l_pages = (PTE *)((page_directory[pd_entry] & 0xFFFFF000) + KERNEL_BASE);

		// remove page table entries;
		// frames get deallocated only after the reference count becomes zero
		for (i=pt_entry; i<pt_entry+n_pages; i++) {
			l_pages[i] =  0;
		}	

		// free space if no more references 
		if (shm[p->shared_memory.key].refs == 0) {
			dealloc_frames((void *)shm[p->shared_memory.key].base, n_pages);
		}
	}
}

/*** Free shared memory area ***/
// Space allocated to a shared memory object is deleted when
// the reference count becomes zero
void free_shared_memory(PCB *p) {
	if (p->shared_memory.created) { // if process attached to a shared memory object
		shm_detach(p);
	}
}

