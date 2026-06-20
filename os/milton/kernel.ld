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
 * PROGRAM_BASE window. PROGRAM_BASE = 0x38000 (spec/memory_map.h) -- keep this
 * literal in sync. A bust is a LINK error (Rule 2), not silent boot-time
 * corruption of the program-image arena. Mitigation: raise PROGRAM_BASE +0x8000
 * (Rule-8 whole-map shift, initech-o0td/5pe precedent). Top-level ASSERT (after
 * the SECTIONS block) is the portable GNU ld form -- a bare ASSERT inside
 * SECTIONS is a syntax error. Headroom ~61 KiB at introduction
 * (_kernel_end=0x28890); Phase-3 FLAIR linking (M3.0) consumes it. */
ASSERT(_kernel_end < 0x38000,
       "kernel image busts the PROGRAM_BASE window (0x38000) -- raise PROGRAM_BASE +0x8000 (o0td precedent) or shrink the kernel")
