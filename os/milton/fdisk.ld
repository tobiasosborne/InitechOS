/* DEC-08 flat COM and spec/memory_map.h, same loader reserve as EDLIN. */
ENTRY(_start)
SECTIONS {
 . = 0x40100;
 .text : { *(.text._start) *(.text*) }
 .rodata : { *(.rodata*) }
 .data : { *(.data*) . = ALIGN(16); BYTE(0); }
 __load_end = .;
 .bss (NOLOAD) : { __bss_start = .; *(.bss*) *(COMMON) __bss_end = .; }
 ASSERT(__bss_end - __load_end <= 0x10000, "FDISK BSS exceeds loader reserve")
 /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
