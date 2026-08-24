/* kernel.ld -- flat binary link script for the InitechDOS C kernel.
 *
 * beads: initech-d00.
 * Ref: PRD Sec 5 (flat binary kernel; no paging/VM: link addr == load addr ==
 *      physical addr); ADR-0003 DEC-08 (flat binary handed off by stage2);
 *      docs/research/boot-to-text-ground-truth.md Sec 3.1-3.2 (link == load ==
 *      physical; _start first), superseded in placement by
 *      docs/design/kernel-runway-relocation.md K1/K2 (bead initech-tdnl.29):
 *      KERNEL_BASE=0x00500000. CLAUDE.md Rule 11 (reproducible -- no
 *      timestamps; the Makefile emits a raw binary via --oformat binary).
 *
 * stage2 reads through INT 13h into the low bounce, copies the padded window to
 * physical 0x00500000 after PE, and far-jumps there. _start MUST be first.
 */

ENTRY(_start)

KERNEL_BASE = 0x00500000;

SECTIONS
{
    . = KERNEL_BASE;
    _kernel_start = .;

    /* kstart.o is linked first on the ld command line, so its .text (which
     * begins with _start) is emitted first -- _start lands at KERNEL_BASE. */
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

/* High-runway link hygiene (docs/design/kernel-runway-relocation.md K2; bead
 * initech-tdnl.29). The first ASSERT is the portable top-level equivalent of
 * ASSERT(. == KERNEL_BASE) immediately after the dot assignment: a stray base
 * or section alignment must fail at link time. The second keeps the entire
 * image, including NOBITS .bss, below the exclusive 0x00600000 ceiling. */
ASSERT(_kernel_start == KERNEL_BASE && ADDR(.text) == KERNEL_BASE,
       "kernel floor mismatch -- .text/_kernel_start must equal KERNEL_BASE (0x00500000)")
ASSERT(_kernel_end < 0x00600000,
       "kernel image busts KERNEL_CEIL (0x00600000) -- shrink the kernel or extend the ratified runway; bump KERNEL_SECTORS too if loaded bytes grew")
