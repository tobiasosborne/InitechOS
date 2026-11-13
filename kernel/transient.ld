/* transient.ld -- link script for the TRANSIENT COMMAND.COM (bead
 * initech-3p1a). The interpreter objects (command.o env.o batch.o, the SAME
 * objects the shell kernel links) are placed at TRANSIENT_BASE
 * (spec/memory_map.h) behind transient_hdr.o, and resolve the kernel calls
 * they make against the running kernel ELF given with ld --just-symbols.
 * Flat binary via objcopy -O binary: .text/.rodata/.data are on disk; .bss is
 * zeroed by the kernel after the load (transient_hdr_t.bss_end). No
 * timestamps (Rule 11).
 */
ENTRY(transient_hdr)

TRANSIENT_BASE = 0x00580000;
TRANSIENT_CEIL = 0x005C0000;

SECTIONS
{
    . = TRANSIENT_BASE;
    .text : {
        KEEP(*(.transient_hdr))
        *(.text*)
    }
    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }
    _tr_load_end = .;
    .bss : {
        *(.bss*)
        *(COMMON)
    }
    _tr_bss_end = .;

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}

ASSERT(ADDR(.text) == TRANSIENT_BASE && transient_hdr == TRANSIENT_BASE,
       "transient header must sit at TRANSIENT_BASE")
ASSERT(_tr_bss_end <= TRANSIENT_CEIL,
       "the transient COMMAND.COM busts TRANSIENT_CEIL (spec/memory_map.h)")
