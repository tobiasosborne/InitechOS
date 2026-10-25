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
 * Lane 1 has ONE section (COLD: the JTC mat + About This Computer, DEC-14-10).
 * A second section gets its own output
 * section here, its own directory row in overlay_hdr.asm, and its own
 * budget ASSERT (DEC-14-9(e)).
 */
ENTRY(ovl_hdr)

OVERLAY_BASE = 0x005C0000;
OVERLAY_CEIL = 0x005E0000;
/* DEC-14-9(e) per-section budget: section COLD (~2 KiB today).
 * Raising it is a deliberate act with an issue (Rule 8 spirit). */
OVL_COLD_BUDGET = 0x1000;

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
