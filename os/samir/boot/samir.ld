/* samir.ld -- flat binary link script for the SAMIR (InitechBase) .COM image.
 *
 * beads: initech-ap5g (SAMIR Milton integration -- the freestanding link
 *        toolchain). ADR-0009 DEC-01/DEC-02/DEC-05 (soft-float; pinned libgcc
 *        helpers; BSS-zeroing in a SAMIR crt0, .bss NOLOAD).
 *
 * Ref (Law 1):
 *   - spec/memory_map.h: PROGRAM_IMAGE = 0x00030100 is the flat .COM load /
 *     entry address (PSP at PROGRAM_BASE 0x30000, +0x100 image offset). The
 *     loader (os/milton/loader.c) copies the image to PROGRAM_IMAGE and JMPs to
 *     it with ESP = PROGRAM_STACK_TOP (0x6FFFC).
 *   - os/milton/kernel.ld: THE PRECEDENT -- an ELF-linked flat binary (ld -T
 *     then objcopy -O binary), entry stub first, .text/.rodata/.data loaded.
 *     This script mirrors it at org 0x30100 and adds the NOLOAD .bss + the
 *     __bss_start/__bss_end symbols the SAMIR crt0 needs to zero BSS (DEC-05:
 *     a flat .COM does not carry .bss in its file, and neither the loader nor
 *     kstart.asm zeroes it; the crt0 must materialize it -- relying on emulator
 *     RAM-reset is a latent Rule-3 bug).
 *
 * Reproducible (Rule 11): no timestamps; the raw .COM is emitted via
 * `objcopy -O binary`. ASCII-clean (Rule 12).
 *
 * Layout: samir_crt0.o is linked FIRST on the ld command line, so its .text
 * (which begins with _start) is emitted first -- _start lands at 0x00030100,
 * the address the loader JMPs to. The loader does NOT inspect any header; the
 * first byte of the image IS the entry point (authentic .COM model).
 */

ENTRY(_start)

SECTIONS
{
    . = 0x00030100;

    /* crt0.o is linked first -> _start is the first emitted byte at 0x30100. */
    .text : {
        *(.text._start)     /* be explicit: the entry stub leads the image */
        *(.text*)
    }

    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }

    /* .bss is NOLOAD: it occupies address space (so .data/.bss do not overlap
     * the program's runtime BSS) but objcopy -O binary emits NO zero-fill for
     * it -- the .COM file stays text+rodata+data sized, and samir_crt0 zeroes
     * [__bss_start, __bss_end) at entry (DEC-05). */
    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }

    _samir_image_end = .;

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
