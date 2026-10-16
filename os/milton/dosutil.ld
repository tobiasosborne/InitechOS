/* ADR-0003 DEC-08 flat COM, spec/memory_map.h. No host metadata. */
ENTRY(_start)
SECTIONS {
  . = 0x40100;
  .text : { *(.start) *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  __bss_start = .;
  .bss : { *(.bss*) *(COMMON) }
  __bss_end = .;
  ASSERT(__bss_end < 0x50000, "DOS utility exceeds COM image window")
  /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) }
}
