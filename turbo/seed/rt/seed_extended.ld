/* Extended InitechMZ module, linked at zero and at a paragraph-aligned probe
 * base for mzlink. Authority: kernel/include/initechmz_extended.h and the
 * InitechBase startup/reserve model. BSS is recorded in descriptor reserve. */
ENTRY(_start)

SECTIONS
{
    . = DEFINED(TURBO_LINK_BASE) ? TURBO_LINK_BASE : 0;

    /* Entry must be the first byte of the module. */
    .text : {
        *(.text._start)
        *(.text*)
    } =0x90909090

    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }

    /* Startup clears the reserved BSS tail. */
    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } =0x90909090

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    } =0x90909090
}
