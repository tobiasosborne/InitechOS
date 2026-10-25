/*
 * os/samir/include/samir/ovl.h -- the SAMIR overlay manager interface
 * (InitechBase overlays Phase B, epic initech-5etom; design:
 * docs/design/samir-overlays.md).
 *
 * THE ARTIFACT (Law 3): freestanding C, <stdint.h> + samir/ headers only.
 *
 * MODEL (TP5 / dBASE style, one fixed slot):
 *   - Cold command code is compiled into OVERLAY SECTIONS with SAMIR_OVL_TEXT
 *     (function attribute). On the Milton build (SAMIR_OVERLAYS defined) the
 *     linker script (os/samir/boot/samir.ld) links every overlay at the same
 *     address -- the slot, a fixed region of SAMIR's own image window -- and
 *     tools/samir_ovl_pack.c moves them out of SAMIR.EXE into SAMIR.OVL.
 *   - Resident code reaches overlaid code ONLY through samir_ovl_call(): the
 *     manager makes sure the overlay is in the slot (reading SAMIR.OVL through
 *     the PAL, as DBASE.EXE reads DBASE.OVL) and calls the overlay's entry,
 *     whose address it takes from the slot header (SAMIR_OVL_HEADER). Resident
 *     code never names an overlay symbol: the link fails if it does
 *     (NOCROSSREFS_TO in samir.ld), and overlay-to-overlay references fail
 *     the link too (OVERLAY ... NOCROSSREFS).
 *   - Overlay code calls only resident code, and never resident code that can
 *     reach samir_ovl_call (no returning into an evicted overlay): the packer
 *     proves it on the call graph (--manager), and the manager refuses a swap
 *     while an overlay is running (Rule 2, defence in depth).
 *   - The host build (no SAMIR_OVERLAYS: every host gate) links everything
 *     directly; SAMIR_OVL_CALL is then a plain call of the same entry
 *     function. Same code, same behaviour; only the loading mechanism is
 *     target-only (ADR-0009 DEC-03: no target-only BEHAVIOUR). The manager's
 *     pure parts (CRC, header/directory checks, relocation) are host-tested.
 *
 * ASCII-clean (Rule 12). No timestamps (Rule 11).
 */
#ifndef INITECH_SAMIR_OVL_H
#define INITECH_SAMIR_OVL_H

#include <stdint.h>
#include "samir/interp.h"
#include "samir/pal.h"

/* Overlay ids == the SAMIR.OVL directory order (tools/samir_ovl_pack.c emits
 * them in this order; the slot header carries the id and the manager checks). */
enum {
    SAMIR_OVL_FILE   = 0,   /* cmd/mutate.c: TYPE RENAME ERASE COPY RUN ! SORT
                               TOTAL INDEX REINDEX, APPEND FROM, PACK, ZAP;
                               fs/ndx.c: ndx_build, ndx_rebuild             */
    SAMIR_OVL_REPORT = 1,   /* cmd/report.c: REPORT FORM, LABEL FORM            */
    SAMIR_OVL_STATUS = 2,   /* cmd/query.c: LIST/DISPLAY STATUS|MEMORY          */
    SAMIR_OVL_SET    = 3,   /* cmd/set.c: the SET sub-option parser             */
    SAMIR_OVL_MEMFILE = 4,  /* cmd/flow.c: SAVE TO / RESTORE FROM               */
    SAMIR_OVL_COUNT  = 5
};

/* An overlay entry: `k` is the resident dispatcher's verb index (each entry
 * documents its own k values); the result is exactly what the resident command
 * hook returns for that verb (CMD_OK / a negative interp error). */
typedef int (*samir_ovl_entry_fn)(int k, xb_interp *ip, const char *args, int *ec);

/* The ONLY resident -> overlay path. */
int samir_ovl_call(unsigned id, int k, xb_interp *ip, const char *args, int *ec);

/* What a resident dispatcher writes. Milton: through the manager (the entry
 * `fn` is never named by resident code; the manager takes it from the slot
 * header). Host: a direct call of the same entry -- no manager, no table, so a
 * host gate that links mutate.c without report.c still links. */
#ifdef SAMIR_OVERLAYS
#define SAMIR_OVL_CALL(id, fn, k, ip, args, ec) samir_ovl_call((id), (k), (ip), (args), (ec))
#else
#define SAMIR_OVL_CALL(id, fn, k, ip, args, ec) fn((k), (ip), (args), (ec))
#endif

#ifdef SAMIR_OVERLAYS
/* The packer turns ".text.0ovl_<name>" / ".rodata.0ovl_<name>*" into overlay
 * <name> (samir.ld). The leading digit cannot begin a function name, so the
 * resident patterns ".text.[!0]*" never capture an overlay section. GCC puts a
 * function's jump tables and string literals in ".rodata.0ovl_<name>*" when
 * its text section is ".text.0ovl_<name>" (-ffunction-sections
 * -fdata-sections), so they travel with it. */
#define SAMIR_OVL_TEXT(name) __attribute__((section(".text.0ovl_" #name)))
#define SAMIR_OVL_DATA(name) __attribute__((section(".rodata.0ovl_" #name)))

typedef struct {
    uint32_t           magic;   /* SAMIR_OVL_MAGIC (spec/samir/ovl_format.h) */
    uint32_t           id;
    samir_ovl_entry_fn entry;
} samir_ovl_slot_hdr;

/* Placed FIRST in the overlay (samir.ld KEEPs ".rodata.0ovl_<name>.0hdr"). */
#define SAMIR_OVL_HEADER(name, ovl_id, fn)                                     \
    static const samir_ovl_slot_hdr samir_ovl_hdr_##name                       \
        __attribute__((used, section(".rodata.0ovl_" #name ".0hdr"))) =        \
        { 0x4C564F53u, (ovl_id), (fn) }

/* Milton startup: open SAMIR.OVL and verify it completely (header, stamp,
 * slot length, overlay count, CRC of the whole file) before the first dot
 * prompt, prompting "Insert System Disk ..." until it is right or the operator
 * aborts. Returns 0 when ready, -1 when the operator aborted (Ctrl-C / end of
 * input). The file stays open for the session (PRG3 R4-4: "the overlay file
 * remains open while running"). */
int samir_ovl_open(samir_pal_t *pal);
#else
#define SAMIR_OVL_TEXT(name)
#define SAMIR_OVL_DATA(name)
#define SAMIR_OVL_HEADER(name, ovl_id, fn) \
    typedef int samir_ovl_hdr_unused_##name
#endif

/* ---- Pure helpers (no I/O; the host oracle test_samir_ovl.c drives them) - */

/* CRC-32 (spec/samir/ovl_format.h): crc = samir_ovl_crc32(0, ...) for a fresh
 * run; pass the previous result to continue. */
uint32_t samir_ovl_crc32(uint32_t crc, const void *buf, uint32_t n);

/* Validate a 32-byte SAMIR.OVL file header against what this resident was
 * linked with. Returns SAMIR_OVL_OK or a reason. */
enum {
    SAMIR_OVL_OK      = 0,
    SAMIR_OVL_MISSING = 1,   /* not found / unreadable / short           */
    SAMIR_OVL_FOREIGN = 2,   /* signature, version, stamp, slot, count   */
    SAMIR_OVL_CORRUPT = 3    /* CRC mismatch, directory out of range     */
};
int samir_ovl_check_header(const uint8_t *hdr, uint32_t stamp, uint32_t slot_len,
                           uint32_t count);

/* Validate one 16-byte directory entry against the slot and the file length;
 * on success store offset/len/nrel/crc. */
int samir_ovl_check_dir(const uint8_t *ent, uint32_t slot_len, uint32_t file_len,
                        uint32_t *off, uint32_t *len, uint32_t *nrel, uint32_t *crc);

/* Add `base` to the u32 at slot+rel[i] for each of the n offsets. Every offset
 * is bounds-checked BEFORE any write (Rule 2); returns 0, or -1 (nothing
 * written) when an offset runs past `len`. */
int samir_ovl_relocate(uint8_t *slot, uint32_t len, const uint8_t *rel,
                       uint32_t n, uint32_t base);

#endif /* INITECH_SAMIR_OVL_H */
