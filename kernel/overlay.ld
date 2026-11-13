/* overlay.ld -- link script for C:\FLAIR.OVL (ADR-0014 DEC-14-1/-2/-3; epic
 * initech-x8gsi lane 1). The transient's pattern (os/milton/transient.ld),
 * generalised: the generated header (overlay_hdr.o) sits at OVERLAY_BASE and
 * the section objects follow it, resolving every kernel call against ONE
 * kernel ELF given with ld --just-symbols (stub names stripped from that
 * copy, so the section's own definitions win). Flat binary via objcopy -O
 * binary; the factory packer (tools/flairovl.c) then fills identity and the
 * CRC-32 and pads to whole 512 B sectors (spec/overlay_format.h). The file
 * is its own memory image: file offset X loads at OVERLAY_BASE + X. .bss is
 * not on disk; the kernel zeroes [load + length, bss_end). No timestamps
 * (Rule 11).
 *
 * ONE section (COLD: JTC, About This Computer and cold Finder operations,
 * ADR-0014 DEC-14-10 lanes 1/2).
 * A second section gets its own output
 * section here, its own directory row in overlay_hdr.asm, and its own
 * budget ASSERT (DEC-14-9(e)).
 */
ENTRY(ovl_hdr)

OVERLAY_BASE = 0x005C0000;
OVERLAY_CEIL = 0x005E0000;
/* ADR-0014 lane 2: 4 KiB -> 32 KiB for Finder operation bodies and dialogs.
 * One section, within the fixed 128 KiB window; the window/guard stay locked.
 * DEC-14-9(e)'s exact-boundary mutant covers the enlarged section budget. */
OVL_COLD_BUDGET = 0x8000;

SECTIONS
{
    . = OVERLAY_BASE;
    .ovl_hdr : { KEEP(*(.ovl_hdr)) }
    _ovl_cold_start = .;
    .text   : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }
    _ovl_cold_load_end = .;
    .bss : {
        *(.bss*)
        *(COMMON)
    }
    _ovl_cold_bss_end = .;
    _ovl_image_len   = ABSOLUTE(_ovl_cold_load_end - OVERLAY_BASE);
    _ovl_cold_off     = ABSOLUTE(_ovl_cold_start - OVERLAY_BASE);
    _ovl_cold_len     = ABSOLUTE(_ovl_cold_load_end - _ovl_cold_start);

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}

ASSERT(ADDR(.ovl_hdr) == OVERLAY_BASE && ovl_hdr == OVERLAY_BASE,
       "FLAIR.OVL header must sit at OVERLAY_BASE")
ASSERT(_ovl_cold_bss_end - _ovl_cold_start <= OVL_COLD_BUDGET,
       "FLAIR.OVL section COLD busts its budget (DEC-14-9(e))")
ASSERT(_ovl_cold_bss_end <= OVERLAY_CEIL,
       "FLAIR.OVL busts OVERLAY_CEIL (spec/memory_map.h)")
