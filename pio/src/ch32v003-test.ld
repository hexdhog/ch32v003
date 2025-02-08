ENTRY(start);

PROVIDE(__stack_size = 256);

MEMORY {
	FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 16K
	RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 2K
}

SECTIONS {
	.init : {
		. = ALIGN(4);
		PROVIDE(__start = .);
		KEEP(*(SORT_NONE(.init)));
		. = ALIGN(4);
	} >FLASH AT>FLASH

	.text : {
		. = ALIGN(4);
		*(.text);
		*(.text.*);
		*(.rodata);
		*(.rodata.*);
		. = ALIGN(4);
	} >FLASH AT>FLASH

	.data : {
		. = ALIGN(4);
		*(.data .data.*)
		PROVIDE(__data_start = .);
		. = ALIGN(8);
		/* since gp is used to access globals within +/-2KB and total RAM size is 2KB
		 * we can just set it to the base of the .data section */
		PROVIDE(__global_pointer$ = .);
		. = ALIGN(8);
		PROVIDE(__data_end = .);
	} >RAM AT>FLASH

	.stack ORIGIN(RAM) + LENGTH(RAM) - __stack_size : {
		PROVIDE(__heap_end = .);
		. = ALIGN(8);
		PROVIDE(__stack_start = .);
		. = . + __stack_size;
		PROVIDE(__stack_end = .);
	} >RAM
}
