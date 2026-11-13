/* Ref: ADR-0003 DEC-08; spec/memory_map.h PROGRAM_BSS_RESERVE and arena. */
ENTRY(_start)
SECTIONS {
 . = 0x40100;
 .text : { *(.text._start) *(.text*) }
 .rodata : { *(.rodata*) }
 .data : { *(.data*) . = ALIGN(16); BYTE(0); }
 __load_end = .;
 .bss (NOLOAD) : { __bss_start = .; *(.bss*) *(COMMON) __bss_end = .; }
 ASSERT(__bss_end - __load_end <= 0x10000, "EDLIN BSS exceeds loader reserve")
 ASSERT(__load_end + 0x10000 + 32 < 0x6f000, "EDLIN has no MCB arena")
 /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
