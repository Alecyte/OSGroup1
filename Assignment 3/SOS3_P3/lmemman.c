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
	//sys_printf("yo dawg\n");
	// TODO: see background material on what this function should
	// do. High-level objectives are:
	// 1) calculate the number of frames necessary for program code,
	//    user-mode stack and kernel-mode stack

	/*uint32_t kernelPages = 4096/4096;//1KB
	uint32_t userPages = 12288/4096;//12KB/4KB
	uint32_t codePages;
	if(code_size<4096)
	{
		codePages = 1;
	}
	else
	{
		codePages = code_size/4096;
	}
	 
	// 2) allocate frames for above
	
	//uint32_t n_pages, uint32_t base, PDE *page_directory, uint32_t mode
	//sys_printf("before alloc kern\n");
	alloc_kernel_pages(kernelPages);
	//sys_printf("successzzzz\n");
	//READ+WRITE
	//sys_printf("code_size %d\n",code_size);
	//process code @ zero - SOS internals
	uint32_t retCode = (uint32_t)alloc_user_pages(codePages, 0, k_page_directory, PDE_READ_WRITE);
	sys_printf("retCode : %d\n", retCode);
	if(retCode == NULL){
		sys_printf("retCode false\n");
		return FALSE;
	}
	else{
		sys_printf("The code number is : %d\n", retCode);
	}	
	
	//sys_printf("retCode is: %d\n", retCode);
	//sys_printf("codePages is: %d\n",codePages);
	//stack starts @ 
	//0xBFBFC000
	uint32_t retStack = (uint32_t)alloc_user_pages(userPages, 0xBFBFC000, k_page_directory, PDE_READ_WRITE);
	if(retStack == NULL){
		return FALSE;
	}
	else{
		sys_printf("The retStack number is : %d\n", retStack);
	}
	uint32_t kerStack = (uint32_t)alloc_user_pages(1, 0xBFBFF000, k_page_directory, PDE_READ_WRITE);
	if(kerStack == NULL){
		return FALSE;
	}
	else{
		sys_printf("The kerStack number is : %d\n", kerStack);
	}
//	init_kernel_pages();

	p->mem.start_code = 0x0;
	//p->mem.start_code = codePages * 4096;
	p->mem.start_brk = codePages * 4096;
	p->mem.brk = p->mem.start_brk;
	p->mem.start_stack = 0xBFBFEFFF;
	p->mem.page_directory = k_page_directory;*/
	//alloc_frames(kernelStack, KERNEL_ALLOC);
	//alloc_frames(userStack, USER_ALLOC);
	//alloc_frames(code, USER_ALLOC);

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

	// TODO: uncomment following line when you start working in 
	//        this function
	/*alloc_kernel_pages(1);
	alloc_user_pages(4,0,k_page_directory, PDE_READ_WRITE);

	uint32_t kernelPages = 4096/4096;//1KB
	uint32_t userPages = 12288/4096;//12KB/4KB
	uint32_t codePages;
	if(code_size<4096)
	{
		codePages = 1;
	}
	else
	{
		codePages = code_size/4096;
	}


	p->mem.start_code = 0x0;
	p->mem.start_brk = codePages * 4096;
	p->mem.brk = p->mem.start_brk;
	p->mem.start_stack =(4096*3);
	p->mem.page_directory = k_page_directory;
	*/
	//alloc_kernel_pages();


	uint32_t codePages;
	if(code_size < 4096){
		codePages = 1;
	}
	else{
		codePages = 1 + code_size/4096;
	}

	//I CANT GET SOMETHING CORRECT
	//THINK THE ISSUE IS SETTING UP THE CORRECT LOCATION OF THE 0xC0000000 thing in page tables.
	//WE NEED TO PUT THE LOCATION OF PAGE TABLES IN SOME ADDRESS SPACE HERE.
	//CANNOT FIGURE OUT WHERE
	//THE REST SHOULD BE REALLY CLOSE TO CORRECT

	sys_printf("We are allocating a kernel page\n");

	PDE *myPDE = (PDE*)alloc_kernel_pages(1);
	//uint32_t n_pages, uint32_t base, PDE *page_directory, uint32_t mode
	sys_printf("The location of the PDE should be here %x\n", myPDE);
	//sys_printf("The location of the tempPDE is :  %x\n", tempPDE);

	myPDE[768] = ((uint32_t)pages_768-KERNEL_BASE) | PDE_PRESENT | PDE_READ_WRITE;

	uint32_t codeRet = (uint32_t)alloc_user_pages(1, 0x0, myPDE, PTE_READ_WRITE);

	uint32_t stackRet = (uint32_t)alloc_user_pages(4, 0xBFBFC000, myPDE, PTE_READ_WRITE);

	p->mem.start_code = 0x0;
	p->mem.end_code = codePages * 4096;
	p->mem.start_brk = p->mem.start_code;
	p->mem.brk = p->mem.start_code;
	p->mem.start_stack = 0xBFBFF000;
	p->mem.page_directory = myPDE;
	return TRUE;
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
// Returns NULL on failure, or beginning logical address
// (i.e. base) on success; we also allocate frames for page tables 
// if necessary
void *alloc_user_pages(uint32_t n_pages, uint32_t base, PDE *page_directory, uint32_t mode) { 
	// some sanity check
	uint32_t initBase = base; //sanity check - see if the parameter base was initially zero
	if (base & 0x00000FFF != 0 || 			// base not 4KB aligned
	    base >= KERNEL_BASE ||			// base encroaching on kernel address space
	    (KERNEL_BASE - base)/4096 < n_pages ||	// some pages on kernel address space
	    n_pages == 0)
	{
	    sys_printf("first null in alloc user\n"); 
	    return NULL; 
	}

	int i;
	sys_printf("we are getting past the first null\n");
	// allocate frames for the requested pages
	uint32_t user_frames = (uint32_t)alloc_frames(n_pages, USER_ALLOC);
	if (user_frames==NULL) {
		sys_printf("second null in alloc user\n");
		return NULL;
	}

	sys_printf("we are getting past the second null\n");
	// how many new page tables we may need; some may be returned
	uint32_t n_pde = n_pages / 1024; // one page table maps 1024 pages 
	if (n_pages % 1024 != 0) n_pde++;
	
	// allocate frames for the new page tables
	uint32_t pt_frames = (uint32_t)alloc_frames(n_pde, KERNEL_ALLOC);
	uint32_t pt_frames_used = 0; // we will track how many are used
	if (pt_frames == NULL) {
		dealloc_frames((void *)user_frames,n_pages);	
		sys_printf("third null in allo user\n");	
		return NULL;
	}

	// set up page directory and page tables
	PTE *l_pages; 
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
	
		
	// fill-zero the memory area
	zero_out_pages((void *)base, n_pages);
	sys_printf("base in alloc %d\n",base);
	if(initBase == 0)
	{

		base = 1;
	}

	return (void *)base; 
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


