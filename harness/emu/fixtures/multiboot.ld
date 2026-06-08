/*
 * multiboot.ld -- linker script for the harness self-test fixtures.
 *
 * beads: initech-f2s. Ref: Multiboot1 spec (the multiboot header must lie
 * within the first 8 KiB of the file and be 4-byte aligned); PRD Sec 8.
 *
 * QEMU's `-kernel` multiboot loader lands the image in 32-bit protected
 * mode with A20 on and flat segments, so the fixtures need no real boot.
 * We link at 1 MiB (0x100000), the conventional multiboot load address,
 * and force .multiboot_header first so the magic is found early.
 *
 * Reproducible: no timestamps/host paths (CLAUDE.md Rule 11). ASCII-only.
 */

ENTRY(_start)

SECTIONS
{
    . = 0x00100000;

    .text : ALIGN(4)
    {
        *(.multiboot_header)
        *(.text)
    }

    .rodata : ALIGN(4) { *(.rodata*) }
    .data   : ALIGN(4) { *(.data*) }
    .bss    : ALIGN(4) { *(.bss*) *(COMMON) }

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
