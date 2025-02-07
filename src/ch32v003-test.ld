ENTRY(start);

PROVIDE(__stack_size = 256);

MEMORY {
	FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 16K
	RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 2K
}

SECTIONS {
	.text : {
		. = ALIGN(4);
		*(.text);
		*(.text.*);
		*(.rodata);
		*(.rodata.*);
		. = ALIGN(4);
	} >FLASH AT>FLASH
}
