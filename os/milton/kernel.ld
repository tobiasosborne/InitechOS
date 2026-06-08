/* kernel.ld -- flat binary link script for the InitechDOS C kernel.
 *
 * beads: initech-d00.
 * Ref: PRD Sec 5 (flat binary kernel; no paging/VM: link addr == load addr ==
 *      physical addr); ADR-0003 DEC-08 (flat binary handed off by stage2);
 *      docs/research/boot-to-text-ground-truth.md Sec 3.1-3.2 (link at
 *      0x00010000; _start first). CLAUDE.md Rule 11 (reproducible -- no
 *      timestamps; the Makefile emits a raw binary via --oformat binary).
 *
 * The kernel is loaded by stage2 (INT 13h) at physical 0x00010000 and entered
 * by a far jump to that address, so _start MUST be the first byte emitted.
 */

ENTRY(_start)

SECTIONS
{
    . = 0x00010000;

    /* kstart.o is linked first on the ld command line, so its .text (which
     * begins with _start) is emitted first -- _start lands at 0x00010000. */
    .text : {
        *(.text*)
    }

    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }

    .bss : {
        *(.bss*)
        *(COMMON)
    }

    _kernel_end = .;

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
