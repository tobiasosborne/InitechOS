/*
 * os/ibase/include/ibase/ovl.h -- the IBASE overlay manager interface
 * (InitechBase overlays Phase B, epic initech-5etom; design:
 * docs/design/ibase-overlays.md).
 *
 * THE ARTIFACT (CLAUDE.md, The constitution): freestanding C, <stdint.h> + ibase/ headers only.
 *
 * MODEL (TP5 / dBASE style, one fixed slot):
 *   - Cold command code is compiled into OVERLAY SECTIONS with IBASE_OVL_TEXT
 *     (function attribute). On the InitechDOS build (IBASE_OVERLAYS defined) the
 *     linker script (os/ibase/boot/ibase.ld) links every overlay at the same
 *     address -- the slot, a fixed region of IBASE's own image window -- and
 *     tools/ibase_ovl_pack.c moves them out of INITBASE.EXE into INITBASE.OVL.
 *   - Resident code reaches overlaid code ONLY through ibase_ovl_call(): the
 *     manager makes sure the overlay is in the slot (reading INITBASE.OVL through
 *     the PAL, as DBASE.EXE reads DBASE.OVL) and calls the overlay's entry,
 *     whose address it takes from the slot header (IBASE_OVL_HEADER). Resident
 *     code never names an overlay symbol: the link fails if it does
 *     (NOCROSSREFS_TO in ibase.ld), and overlay-to-overlay references fail
 *     the link too (OVERLAY ... NOCROSSREFS).
 *   - Overlay code calls only resident code, and never resident code that can
 *     reach ibase_ovl_call (no returning into an evicted overlay): the packer
 *     proves it on the call graph (--manager), and the manager refuses a swap
 *     while an overlay is running (CLAUDE.md, The constitution, defence in depth).
 *   - The host build (no IBASE_OVERLAYS: every host gate) links everything
 *     directly; IBASE_OVL_CALL is then a plain call of the same entry
 *     function. Same code, same behaviour; only the loading mechanism is
 *     target-only (ADR-0009 DEC-03: no target-only BEHAVIOUR). The manager's
 *     pure parts (CRC, header/directory checks, relocation) are host-tested.
 *
 * ASCII-clean (CLAUDE.md, The constitution). No timestamps (CLAUDE.md, The constitution).
 */
#ifndef INITECH_IBASE_OVL_H
#define INITECH_IBASE_OVL_H

#include <stdint.h>
#include "ibase/interp.h"
#include "ibase/pal.h"

/* Overlay ids == the INITBASE.OVL directory order (tools/ibase_ovl_pack.c emits
 * them in this order; the slot header carries the id and the manager checks). */
enum {
    IBASE_OVL_FILE   = 0,   /* cmd/mutate.c: TYPE RENAME ERASE COPY RUN ! SORT
                               TOTAL INDEX REINDEX, APPEND FROM, PACK, ZAP;
                               fs/ndx.c: ndx_build, ndx_rebuild             */
    IBASE_OVL_REPORT = 1,   /* cmd/report.c: REPORT FORM, LABEL FORM            */
    IBASE_OVL_STATUS = 2,   /* cmd/query.c: LIST/DISPLAY STATUS|MEMORY          */
    IBASE_OVL_SET    = 3,   /* cmd/set.c: the SET sub-option parser             */
    IBASE_OVL_MEMFILE = 4,  /* cmd/flow.c: SAVE TO / RESTORE FROM               */
    IBASE_OVL_FORMS  = 5,   /* cmd/query.c: @...TO, @...CLEAR, bare @, CLEAR    */
    IBASE_OVL_COUNT  = 6
};

/* An overlay entry: `k` is the resident dispatcher's verb index (each entry
 * documents its own k values); the result is exactly what the resident command
 * hook returns for that verb (CMD_OK / a negative interp error). */
typedef int (*ibase_ovl_entry_fn)(int k, xb_interp *ip, const char *args, int *ec);

/* The ONLY resident -> overlay path. */
int ibase_ovl_call(unsigned id, int k, xb_interp *ip, const char *args, int *ec);

/* What a resident dispatcher writes. InitechDOS: through the manager (the entry
 * `fn` is never named by resident code; the manager takes it from the slot
 * header). Host: a direct call of the same entry -- no manager, no table, so a
 * host gate that links mutate.c without report.c still links. */
#ifdef IBASE_OVERLAYS
#define IBASE_OVL_CALL(id, fn, k, ip, args, ec) ibase_ovl_call((id), (k), (ip), (args), (ec))
#else
#define IBASE_OVL_CALL(id, fn, k, ip, args, ec) fn((k), (ip), (args), (ec))
#endif

#ifdef IBASE_OVERLAYS
/* The packer turns ".text.0ovl_<name>" / ".rodata.0ovl_<name>*" into overlay
 * <name> (ibase.ld). The leading digit cannot begin a function name, so the
 * resident patterns ".text.[!0]*" never capture an overlay section. GCC puts a
 * function's jump tables and string literals in ".rodata.0ovl_<name>*" when
 * its text section is ".text.0ovl_<name>" (-ffunction-sections
 * -fdata-sections), so they travel with it. */
#define IBASE_OVL_TEXT(name) __attribute__((section(".text.0ovl_" #name)))
#define IBASE_OVL_DATA(name) __attribute__((section(".rodata.0ovl_" #name)))

typedef struct {
    uint32_t           magic;   /* IBASE_OVL_MAGIC (spec/ibase/ovl_format.h) */
    uint32_t           id;
    ibase_ovl_entry_fn entry;
} ibase_ovl_slot_hdr;

/* Placed FIRST in the overlay (ibase.ld KEEPs ".rodata.0ovl_<name>.0hdr"). */
#define IBASE_OVL_HEADER(name, ovl_id, fn)                                     \
    static const ibase_ovl_slot_hdr ibase_ovl_hdr_##name                       \
        __attribute__((used, section(".rodata.0ovl_" #name ".0hdr"))) =        \
        { 0x4C564F53u, (ovl_id), (fn) }

/* InitechDOS startup: open INITBASE.OVL and verify it completely (header, stamp,
 * slot length, overlay count, CRC of the whole file) before the first dot
 * prompt, prompting "Insert System Disk ..." until it is right or the operator
 * aborts. Returns 0 when ready, -1 when the operator aborted (Ctrl-C / end of
 * input). The file stays open for the session (PRG3 R4-4: "the overlay file
 * remains open while running"). */
int ibase_ovl_open(ibase_pal_t *pal);
#else
#define IBASE_OVL_TEXT(name)
#define IBASE_OVL_DATA(name)
#define IBASE_OVL_HEADER(name, ovl_id, fn) \
    typedef int ibase_ovl_hdr_unused_##name
#endif

/* ---- Pure helpers (no I/O; the host oracle test_ibase_ovl.c drives them) - */

/* CRC-32 (spec/ibase/ovl_format.h): crc = ibase_ovl_crc32(0, ...) for a fresh
 * run; pass the previous result to continue. */
uint32_t ibase_ovl_crc32(uint32_t crc, const void *buf, uint32_t n);

/* Validate a 32-byte INITBASE.OVL file header against what this resident was
 * linked with. Returns IBASE_OVL_OK or a reason. */
enum {
    IBASE_OVL_OK      = 0,
    IBASE_OVL_MISSING = 1,   /* not found / unreadable / short           */
    IBASE_OVL_FOREIGN = 2,   /* signature, version, stamp, slot, count   */
    IBASE_OVL_CORRUPT = 3    /* CRC mismatch, directory out of range     */
};
int ibase_ovl_check_header(const uint8_t *hdr, uint32_t stamp, uint32_t slot_len,
                           uint32_t count);

/* Validate one 16-byte directory entry against the slot and the file length;
 * on success store offset/len/nrel/crc. */
int ibase_ovl_check_dir(const uint8_t *ent, uint32_t slot_len, uint32_t file_len,
                        uint32_t *off, uint32_t *len, uint32_t *nrel, uint32_t *crc);

/* Add `base` to the u32 at slot+rel[i] for each of the n offsets. Every offset
 * is bounds-checked BEFORE any write (CLAUDE.md, The constitution); returns 0, or -1 (nothing
 * written) when an offset runs past `len`. */
int ibase_ovl_relocate(uint8_t *slot, uint32_t len, const uint8_t *rel,
                       uint32_t n, uint32_t base);

#endif /* INITECH_IBASE_OVL_H */
