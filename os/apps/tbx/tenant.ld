/*
 * os/apps/tbx/tenant.ld -- the link of a disk-launched C application (factory
 * script for an artifact image; bead initech-w96l).
 *
 * Ref: ADR-0003-AMENDMENT-DEC-08a (InitechMZ: a FLAT 32-bit load module whose
 *      entry is its first byte, relocated by a table of absolute-dword sites);
 *      spec/toolbox_gate.h Sec 5 (imageBase / imageLen describe the LOADED
 *      MODULE), Sec 8 (the e_minalloc area after it is zeroed by the loader);
 *      Makefile tenant-exe-rules (links this twice, at TBX_BASE = 0 and at the
 *      probe base, and lets mzlink --reloc-diff derive the relocation sites).
 *
 * Layout: [ .text (tbx_entry first) | .rodata | .data ]  = the module, then
 *         .bss (NOLOAD) = the e_minalloc area. __image_len is ABSOLUTE so its
 *         value is the same at both link bases (it is a length, never
 *         relocated). Nothing here depends on the base except through
 *         absolute dwords, which is what lets the kernel load it anywhere.
 */
OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(tbx_entry)

SECTIONS
{
    . = TBX_BASE;
    __image_start = .;
    .text : {
        KEEP(*(.text.entry))
        *(.text .text.*)
    }
    .rodata : {
        *(.rodata .rodata.*)
    }
    .data : {
        *(.data .data.*)
        . = ALIGN(16);
    }
    __image_end = .;
    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss .bss.* COMMON)
        . = ALIGN(16);
        __bss_end = .;
    }
    __image_len = ABSOLUTE(__image_end - __image_start);
    __bss_len = ABSOLUTE(__bss_end - __image_end);
    /DISCARD/ : {
        *(.comment) *(.note .note.*) *(.eh_frame .eh_frame_hdr)
    }
}
