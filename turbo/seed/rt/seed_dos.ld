/*
 * seed_dos.ld -- flat .COM link script for seed-compiled InitechDOS programs.
 *
 * beads: initech-ogxv (B8 -- the fileio.pas ON-InitechDOS oracle leg).
 * Ref:   os/samir/boot/samir.ld (THE PRECEDENT, mirrored here: org at
 *        PROGRAM_IMAGE 0x00040100 per spec/memory_map.h; entry stub linked
 *        first so _start is the first emitted byte -- the loader inspects no
 *        header, the first byte of the image IS the entry; NOLOAD .bss with
 *        __bss_start/__bss_end for the crt0 zeroing, DEC-05 rationale);
 *        seed/rt/seed.ld (the bare-metal sibling this deliberately does NOT
 *        modify -- two runtime targets, one compiled program body).
 *        CLAUDE.md Rule 11 (no timestamps) / Rule 12 (ASCII).
 */

ENTRY(_start)

SECTIONS
{
    . = 0x00040100;

    /* start_dos.o is linked first -> _start is the first emitted byte at
     * 0x40100, the address the loader JMPs to (authentic .COM model). */
    .text : {
        *(.text._start)
        *(.text*)
    }

    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }

    /* NOLOAD: address space but no file bytes; start_dos.asm zeroes
     * [__bss_start, __bss_end) at entry (the samir.ld DEC-05 rationale). */
    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
