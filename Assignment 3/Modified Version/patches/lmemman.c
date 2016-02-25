////////////////////////////////////////////////////////
// The NAIVE Logical Memory Manager
// 
// Divides memory into 4KB pages and allocates from them

#include "kernel_only.h"

// kernel page directory will be placed at frame 257
PDE *k_page_directory = (PDE *)(0xC0101000); 
// page table entries for the 768th page directory entry will be placed
// at frame 258; 768th entry corresponds to virtual address range
// 3GB to 3GB+4MB-1 (0xC0000000 to 0xC03FFFFF)
PTE *pages_768 = (PTE *)(0xC0102000); 

/*** Initialize logical memory for a process ***/
// Allocates physical memory and sets up page tables;
// we need to allocate memory to hold the program code and
// data, the stack, the page directory, and the required
// page tables
// called by runprogram.c; this function does not load the 
// program from disk to memory (done in scheduler.c)

bool init_logical_memory(PCB *p, uint32_t code_size) {

	// TODO: see background material on what this function should
	// do. High-level objectives are:
	// 1) calculate the number of frames necessary for program code,
	//    user-mode stack and kernel-mode stack
	
	// 2) allocate frames for above
	// 3) determine beginning physical address of program code,
	//    user-mode stack and kernel-mode stack
	// 4) calculate the number of frames necessary for page directory
	//    and page tables
	// 5) allocate frames for above
	// 6) set up page directory and page tables
	// 7) set mem struct in PCB: start_code, end_code, start_stack,
	//    start_brk, brk, and page_directory
	// Return value: TRUE if everything goes well; FALSE if allocation
	//     of frames failed (you should dealloc any frames that may
	//     have already been allocated before returning)

	// TODO: comment following line when you start working in 
	//        this function

	return FALSE;
}

/*** Initialize kernel's page directory and table ***/
void init_kernel_pages(void) {
	uint32_t i;

	// set up kernel page directory (users cannot touch this)
	for (i=0; i<1024; i++) k_page_directory[i] = 0;
	k_page_directory[768] = ((uint32_t)pages_768-KERNEL_BASE) | PDE_PRESENT | PDE_READ_WRITE;

	// map virtual (0xC0000000--0xC03FFFFF) to physical (0--0x3FFFFF)
	for (i=0; i<1024; i++) 
		pages_768[i] = (i*4096) | PTE_PRESENT | PTE_READ_WRITE | PTE_GLOBAL;

	// load page directory
	load_CR3((uint32_t)k_page_directory-KERNEL_BASE);
}

/*** Load CR3 with page directory ***/
void load_CR3(uint32_t pd) {
	asm volatile ("movl %0, %%eax\n": :"m"(pd));
	asm volatile ("movl %eax, %cr3\n");
}

/*** Allocate logical memory for kernel***/
// Allocates pages for kernel and returns logical address of allocated memory
// Note: Kernel uses 0xC0000000 to 0xC0400000 for now
void *alloc_kernel_pages(uint32_t n_pages) { 
	uint32_t p_alloc_base; // physical address of allocated memory
	uint32_t l_alloc_base; // logical address of allocated memory
	int i;

	p_alloc_base = (uint32_t)alloc_frames(n_pages, KERNEL_ALLOC); 
	if (p_alloc_base==NULL) return NULL;

	// Note: page table update is not necessary since first 4MB is already
	// mapped and kernel allocation is always from first 4MB

	// adding KERNEL_BASE converts address to logical when allocation 
	// of kernel is from the first 4MB (see alloc_frames)
	l_alloc_base = p_alloc_base + KERNEL_BASE;

	// fill-zero the memory area
	zero_out_pages((void *)l_alloc_base, n_pages);

	return (void *)l_alloc_base; 
}

/*** Allocate logical memory for user ***/
// n_pages: the number of contiguous pages requested
// base: requested base address of first page; must be 4KB aligned;
//       all pages must fit before hitting KERNEL_BASE
// page_directory: logical base address of process page directory
// mode: page modes (READ ONLY or READ+WRITE)
// Returns FALSE on failure, TRUE on success
//
// We also allocate frames for page tables if necessary
//
// 2016-02-24: Fixed some bugs:
//   1. Return type changed to bool, void* return type failed if called with base = 0
//   2. Fixed calculation of n_pde
//   3. Removed init of user pages to 0 (this failed because they aren't mapped in the kernel space)
//   Requires corresponding update of declaration in kernel_only.h
bool alloc_user_pages(uint32_t n_pages, uint32_t base, PDE *page_directory, uint32_t mode) { 
	// some sanity check
	if (base & 0x00000FFF != 0 || 			// base not 4KB aligned
	    base >= KERNEL_BASE ||			// base encroaching on kernel address space
	    (KERNEL_BASE - base)/4096 < n_pages ||	// some pages on kernel address space
	    n_pages == 0) return FALSE; 

	int i;

	// allocate frames for the requested pages
	uint32_t user_frames = (uint32_t)alloc_frames(n_pages, USER_ALLOC);
	if (user_frames==NULL) return FALSE;

	// how many new page tables we may need; some may be returned
	uint32_t n_pde = (n_pages+1023) / 1024 + 1; // one page table maps 1024 pages 
	
	// allocate frames for the new page tables
	uint32_t pt_frames = (uint32_t)alloc_frames(n_pde, KERNEL_ALLOC);
	uint32_t pt_frames_used = 0; // we will track how many are used
	if (pt_frames == NULL) {
		dealloc_frames((void *)user_frames,n_pages);		
		return FALSE;
	}
	
	// set up page directory and page tables
	PTE *l_pages; \
	uint32_t pd_entry = base >> 22; // start from this page directory entry
	uint32_t pt_entry = (base >> 12) & 0x000003FF; // and this page table entry

	for (i=0; i<n_pages; i++) {
		// see if page directory entry is present
		if ((uint32_t)(page_directory[pd_entry] & PDE_PRESENT) == 0) { // first time use (create the entry)
			page_directory[pd_entry] = pt_frames | PDE_PRESENT | PDE_READ_WRITE | PDE_USER_SUPERVISOR;
			zero_out_pages((void *)(pt_frames + KERNEL_BASE), 1);
			pt_frames_used++;
			pt_frames += 4096; // one page table takes up 4KB = 4096 bytes
		}
		
		// logical address pointer of page table
		l_pages = (PTE *)((page_directory[pd_entry] & 0xFFFFF000) + KERNEL_BASE);

		// write page table entries; if a mapping already exists, then referred frame
		// is freed
		if ((uint32_t)(l_pages[pt_entry] & PTE_PRESENT) != 0) { // mapping already present
			dealloc_frames((void *)(l_pages[pt_entry] & 0xFFFFF000),1);
		}
		l_pages[pt_entry] = user_frames | mode | PTE_PRESENT | PTE_USER_SUPERVISOR;
		user_frames += 4096; // one page is 4KB

		pt_entry++;
		if (pt_entry == 1024) {  // time to move to next page directory entry
			pd_entry++;
			pt_entry = 0;
		}	
	}

	// return unused frames allocated for page tables
	if (pt_frames_used != n_pde)
		dealloc_frames((void *)pt_frames, (n_pde - pt_frames_used));
	
	return TRUE; 
}

/*** Deallocate one page ***/
// Deallocates the page corresponding to virtual address
// <loc>; p is the virtual address of page directory
void dealloc_page(void *loc, PDE *p) {
	uint32_t pd_entry = (uint32_t)loc >> 22; // top 10 bits
	uint32_t pt_entry = ((uint32_t)loc >> 12) & 0x000003FF; // next top 10 bits 
	int i;

	// obtain page table corresponding to page directory entry
	PTE *pt = (PTE *)(p[pd_entry] & 0xFFFFF000);
	pt = (PTE *)((uint32_t)pt + KERNEL_BASE); // converting to virtual address

	// deallocate the frame
	dealloc_frames((void *)(pt[pt_entry] & 0xFFFFF000), 1);

	// if user space address, then mark page table entry as not present
	if ((uint32_t)loc < KERNEL_BASE) 
		pt[pt_entry] = 0; 	
}

/*** Deallocate all pages ***/
// Traverses the page directory and deallocs all allocated
// pages; p is the virtual address of page directory
void dealloc_all_pages(PDE *p) {
	uint32_t pd_entry;
	uint32_t pt_entry;
	uint32_t i;
	uint32_t loc = 0;
	PTE *pt;

	while (loc < 0xC0000000) { // only freeing user area of virtual memory
		pd_entry = loc >> 22; // top 10 bits

		if (p[pd_entry] != 0) { // page directory entry exists
			pt = (PTE *)((p[pd_entry] & 0xFFFFF000) + KERNEL_BASE);
			for (i=0; i<1024; i++) { // walk through page table
				if (pt[i] == 0) continue;
				dealloc_page((void *)(loc + i*4096), p);
			}

			// dealloc page table space and mark page directory entry not present
			dealloc_frames((void *)(p[pd_entry] & 0xFFFFF000), 1);
			p[pd_entry] = 0;
		}
		
		loc += 0x400000; // move ahead 4MB; the next page table
	}
}

/*** Zero out pages ***/
// Ensure that page mappings exist before calling this function
void zero_out_pages(void *base, uint32_t n_pages) {
	int i=0;
	for (i=0; i<1024*n_pages; i++)
		*((uint32_t *)((uint32_t)base + i)) = 0;
}


