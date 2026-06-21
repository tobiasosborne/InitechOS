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

/* B0 (beads initech-re30.1): fail-loud if the kernel image busts the
 * PROGRAM_BASE window. PROGRAM_BASE = 0x40000 (spec/memory_map.h; raised
 * 0x38000->0x40000 by beads initech-re30.2, the whole-map +0x8000 shift) -- keep
 * this literal in sync. A bust is a LINK error (Rule 2), not silent boot-time
 * corruption of the program-image arena. Top-level ASSERT (after the SECTIONS
 * block) is the portable GNU ld form -- a bare ASSERT inside SECTIONS is a
 * syntax error. NOTE (initech-re30.2): this raise spent the LAST conventional
 * gap (kernel stack now butts 0xA0000), so the next mitigation is no longer a
 * +0x8000 shift but a high-half / extended-memory kernel relocation. */
ASSERT(_kernel_end < 0x40000,
       "kernel image busts the PROGRAM_BASE window (0x40000) -- raise PROGRAM_BASE (high-half/ext-mem relocation; conventional gap exhausted) or shrink the kernel")
