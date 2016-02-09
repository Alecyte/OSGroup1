OUTPUT_FORMAT("binary")

SECTIONS
{	
	kernel_start = 0x10000; /* execution time address of first instruction */
	. = kernel_start;

	.start_module : { startup.o(.text)
		. = ALIGN (1024 + kernel_start); }

	.all_text : { *(.text) }
	.all_rodata : { *(.rodata) }
	.all_data : { *(.data) 
		kernel_end = .; }
 	

	ASSERT(kernel_end - kernel_start <= 512K, "Kernel image too big!")

	
}
	
