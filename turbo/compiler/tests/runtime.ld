/* Factory byte oracle: identical object order/alignment to seed_dos.ld. */
SECTIONS {
 . = 0;
 .text : { *(.text._start) *(.text*) } =0x90909090
 .rodata : { *(.rodata*) }
 /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
pas_main = 0;
__bss_start = 0;
__bss_end = 0;
