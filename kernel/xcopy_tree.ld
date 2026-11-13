/* ADR-0003 DEC-08 flat image; spec/memory_map.h PROGRAM_IMAGE. */
OUTPUT_FORMAT(elf32-i386)
ENTRY(start)
SECTIONS {
    . = 0x40100;
    .entry : { *(.entry) }
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    __bss_start = .;
    .bss : { *(.bss*) *(COMMON) }
    __bss_end = .;
    ASSERT(__bss_end < 0x6f000, "DOS utility exceeds flat program region")
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
