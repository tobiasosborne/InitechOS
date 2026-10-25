/* samir.ld -- link script for SAMIR (InitechBase): the resident load module of
 * SAMIR.EXE plus the overlays of SAMIR.OVL, in one ELF that
 * tools/samir_ovl_pack.c splits.
 *
 * beads: initech-ap5g (the freestanding link toolchain); initech-5etom.1
 *        (overlays Phase B; docs/design/samir-overlays.md).
 *        ADR-0009 DEC-01/02/03/05 (soft-float; pinned libgcc helpers; MZ +
 *        overlay as the authentic packaging; BSS zeroed by the crt0, .bss
 *        NOLOAD). ADR-0003 DEC-08a (InitechMZ: flat module, u32 relocations
 *        to which the loader adds the load base).
 *
 * Linked at ORG 0 (with -q, so every relocation survives into the ELF): the
 * InitechMZ loader adds the load base B (PROGRAM_IMAGE, 0x40100, for a top-level
 * EXEC) to each relocated dword of the resident module, and the overlay manager
 * (os/samir/core/ovl.c) adds the same B to each relocated dword of an overlay
 * it reads into the slot. One factor serves both because the slot sits at a
 * fixed offset from B inside SAMIR's own image window.
 *
 * Layout (run-time addresses = B + the link addresses below):
 *   [0, load_end)              resident .text/.rodata/.data -> SAMIR.EXE module
 *   [__ovl_slot, __ovl_slot_end)  THE SLOT: every overlay is linked here
 *                              (OVERLAY: the region is as long as the largest
 *                              overlay) -> each one goes to SAMIR.OVL
 *   [__bss_start, __bss_end)   .bss, zeroed by samir_crt0
 *   [_samir_heap_tail, +3952)  the PAL heap's first region
 *   ... up to load_end + 64 KiB: the loader's BSS reserve; the AH=48h arena
 *   (the rest of the PAL heap, os/samir/pal/pal_milton.c) starts above it.
 * The slot and .bss are NOLOAD: they live in the BSS reserve that the loader
 * keeps free above the module (spec/memory_map.h PROGRAM_BSS_RESERVE).
 *
 * Section discipline (os/samir/include/samir/ovl.h): overlay code is
 * ".text.0ovl_<name>", its jump tables ".rodata.0ovl_<name>", its slot header
 * ".rodata.0ovl_<name>.0hdr" (KEEP, first); its mergeable literals
 * (".rodata.0ovl_<name>.str*" / ".cst*") stay resident (see .rodata). A
 * function name never starts with a digit, so the resident patterns
 * ".text.[!0]*" / ".rodata.[!0]*" never capture an overlay input section.
 * An input section no pattern names would become an orphan output section;
 * tools/samir_ovl_pack.c refuses any non-empty allocated section other than
 * .text/.rodata/.data/.bss and the named overlays.
 *
 * Static checks (the link FAILS on a violation):
 *   - OVERLAY ... NOCROSSREFS: no overlay refers to another overlay.
 *   - NOCROSSREFS_TO(<overlay> <resident sections>): no resident section refers
 *     to an overlay (the manager reaches an overlay only through the slot
 *     header it reads at run time, never through a symbol).
 *
 * Reproducible (Rule 11): no timestamps. ASCII-clean (Rule 12).
 */

ENTRY(_start)

SECTIONS
{
    . = 0;
    __samir_base = .;

    /* crt0.o is linked first -> _start is the first emitted byte (MZ entry
     * e_cs:e_ip = 0:0, DEC-08a.2). */
    .text : {
        *(.text._start)
        *(.text .text.[!0]*)
    }

    /* Mergeable strings and constants of overlay functions stay RESIDENT:
     * with one LTO partition GCC emits an identical literal once, in the
     * section of whichever function used it first, so an overlay's string
     * section can hold a literal resident code also uses (the link proved it:
     * NOCROSSREFS_TO refused "TO"/"FILE" from mutate_cmd_hook). Only the code
     * and its jump tables move. */
    .rodata : {
        *(.rodata .rodata.[!0]*)
        *(.rodata.0ovl_*.str* .rodata.0ovl_*.cst*)
    }
    .data   : {
        *(.data .data.*)
        . = ALIGN(16);
        __ovl_slot = .;           /* resident symbol naming the slot start */
    }

    _samir_load_end = .;
    ASSERT(_samir_load_end == __ovl_slot, "slot must follow the module")

    OVERLAY : NOCROSSREFS
    {
        .ovl_file {
            KEEP(*(.rodata.0ovl_file.0hdr))
            *(.text.0ovl_file)
            *(.rodata.0ovl_file)
        }
        .ovl_report {
            KEEP(*(.rodata.0ovl_report.0hdr))
            *(.text.0ovl_report)
            *(.rodata.0ovl_report)
        }
        .ovl_status {
            KEEP(*(.rodata.0ovl_status.0hdr))
            *(.text.0ovl_status)
            *(.rodata.0ovl_status)
        }
        .ovl_set {
            KEEP(*(.rodata.0ovl_set.0hdr))
            *(.text.0ovl_set)
            *(.rodata.0ovl_set)
        }
        .ovl_memfile {
            KEEP(*(.rodata.0ovl_memfile.0hdr))
            *(.text.0ovl_memfile)
            *(.rodata.0ovl_memfile)
        }
    }

    . = ALIGN(16);
    __ovl_slot_end = .;

    /* .bss is NOLOAD: the module carries no zero-fill; samir_crt0 zeroes
     * [__bss_start, __bss_end) at entry (DEC-05). */
    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    _samir_image_end = .;
    /* The PAL heap's first region (3952 B, pal_milton.c
     * PAL_MILTON_REGION1_BYTES) lives in the reserve above .bss. */
    . = ALIGN(16);
    _samir_heap_tail = .;

    /* Slot + .bss + heap region 1 must fit the loader's BSS reserve above the
     * module (spec/memory_map.h PROGRAM_BSS_RESERVE = 64 KiB), or the AH=48h
     * arena the loader places above it would overlap them (ADR-0009 DEC-04). */
    ASSERT(_samir_heap_tail + 3952 - _samir_load_end <= 0x10000,
           "SAMIR slot + BSS + PAL region 1 exceed the loader's 64 KiB BSS reserve")
    /* The rest of the PAL heap is one AH=48h block of 2057 paragraphs
     * (0x8090 B: pal_milton.c PAL_MILTON_HEAP_PARAS) plus its MCB and
     * paragraph rounding (0x20); it must end below ENV_BLOCK (0x6F000) when the
     * module is loaded at PROGRAM_IMAGE (0x40100). */
    ASSERT(_samir_load_end + 0x10000 + 0x8090 + 0x20 <= 0x6F000 - 0x40100,
           "SAMIR resident module leaves insufficient PAL memory")
    /* Nothing may sit above the slot at a lower address than the overlays'. */
    ASSERT(__ovl_slot_end > __ovl_slot, "no overlays linked")

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}

NOCROSSREFS_TO(.ovl_file    .text .rodata .data .bss)
NOCROSSREFS_TO(.ovl_report  .text .rodata .data .bss)
NOCROSSREFS_TO(.ovl_status  .text .rodata .data .bss)
NOCROSSREFS_TO(.ovl_set     .text .rodata .data .bss)
NOCROSSREFS_TO(.ovl_memfile .text .rodata .data .bss)
