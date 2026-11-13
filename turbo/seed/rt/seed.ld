/*
 * seed.ld -- linker script for seed-compiled freestanding binaries.
 *
 * beads: initech-znb ("Step B of the InitechOS seed cross-compiler").
 * Ref:   Multiboot1 spec (the multiboot header must lie within the first
 *        8 KiB of the file and be 4-byte aligned); PRD Sec 6.7 / Sec 8.
 *        Adapted from harness/emu/fixtures/multiboot.ld (the seed keeps its
 *        OWN copy under seed/rt/ rather than editing the harness fixtures).
 *        CLAUDE.md Rule 11 (reproducible: no timestamps/host paths) / Rule 12.
 *
 * QEMU's `-kernel` multiboot loader lands the image in 32-bit protected mode
 * with A20 on and flat segments. We link at 1 MiB (0x100000), the
 * conventional multiboot load address, and force .multiboot_header first so
 * the magic is found early. The runtime (start.asm) provides _start; codegen
 * provides pas_main.
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
