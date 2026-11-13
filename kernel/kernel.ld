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
        /* initech-tode; round2 size-policy Sec 4 lever C. Firmware jumps to
         * _start; interrupt delivery enters assembly without a C call edge.
         * Keep the complete entry/ISR stubs, including their relocations. */
        KEEP(*kstart.o(.text*))
        KEEP(*(.text._start))
        KEEP(*isr.o(.text*))
        *(.text*)
    }

    .rodata : {
        /* idt.c: the exception-vector table is an interrupt root. */
        KEEP(*(.rodata.g_exc_stubs))
        *(.rodata*)
    }
    .data   : { *(.data*) }

    .bss : {
        KEEP(*(.bss.g_idt .bss.g_idt_ptr))
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
 * image, including NOBITS .bss, at or below KERNEL_IMAGE_CEIL (0x00580000 ==
 * TRANSIENT_BASE): above it sit the transient COMMAND.COM window and, at the
 * runway top [0x005F0000,0x00600000), the kernel stack (bead initech-xu97;
 * spec/memory_map.h; the Makefile fails at parse time if this literal and the
 * header's KERNEL_IMAGE_CEIL disagree). */
ASSERT(_kernel_start == KERNEL_BASE && ADDR(.text) == KERNEL_BASE,
       "kernel floor mismatch -- .text/_kernel_start must equal KERNEL_BASE (0x00500000)")
ASSERT(_kernel_end <= 0x00580000,
       "kernel image busts KERNEL_IMAGE_CEIL (0x00580000, transient base; kernel stack above) -- shrink the kernel or re-ratify the runway sub-windows; bump KERNEL_SECTORS too if loaded bytes grew")
