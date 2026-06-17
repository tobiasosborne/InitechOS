/*
 * os/samir/include/samir/dbt.h -- SAMIR (InitechBase) .dbt memo file contract.
 *                                   Step S2.1: III+ memo READ.
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib). Includes ONLY <stdint.h> plus the engine headers. No libc, no
 * int 0x21 -- all OS contact is through the PAL vtable.
 *
 * dBASE III PLUS 1.1 ONLY (docs/plans/SAMIR-implementation-plan.md Sec 2.C):
 * The .dbt memo file for a .dbf with version byte 0x83. The dBASE IV .dbt
 * (version 0x8B with FF FF 08 00 per-block header) is OUT OF SCOPE for M6;
 * dbt_open rejects the 0x8B dialect with DBT_ERR_BAD_VERSION (fail loud,
 * Rule 2). The FoxPro .fpt format is out of scope entirely.
 *
 * S2.1 scope (READ only; write is S2.2):
 *   - dbt_open  : open a .dbt via the PAL; read and validate the block-0
 *                 header; return an opaque handle.
 *   - dbt_close : release arena memory and the PAL handle.
 *   - dbt_read  : read one memo by block number; decode the terminator;
 *                 return a pointer to the text bytes and the length.
 *
 * S2.1 / S2.2 boundary:
 *   This header exposes no write primitives. Callers that need to append a
 *   memo (S2.2) will use dbt_append (defined in S2.2). The block-0 next-free
 *   pointer is read (to validate the file geometry) but never written here.
 *
 * File geometry (dbt.md sec 2, verified against TOURS/TRAVEL/UNIVERS.DBT):
 *   - Fixed 512-byte blocks, numbered from 0.  Block n is at file offset n*512.
 *   - Block 0 is the header: 4-byte LE uint32 next-free-block at offset 0;
 *     remaining 508 bytes are unstructured / leftover garbage in III+ 1.1.
 *   - Memo blocks (1+) carry raw text starting at byte 0 of the block.
 *     No per-block header exists in III+ (that is a dBASE IV addition).
 *   - A memo ends at the FIRST occurrence of 0x1A 0x1A (two consecutive
 *     SUB / Ctrl-Z bytes). Everything after the terminator in the block is
 *     unused garbage.
 *   - Tolerance (S2.1 contract): a lone trailing 0x1A at the very end of the
 *     file (as found in TOURS.DBT and UNIVERS.DBT, both 513 bytes = 1 full
 *     block + one stray byte) is NOT treated as a memo block; it is ignored.
 *   - The M-field pointer (10-byte right-justified ASCII decimal in the .dbf
 *     record) is decoded by the caller (dec_parse from rt.h) before calling
 *     dbt_read; this codec accepts the block number directly.
 *
 * Endianness (dbt.md sec 3, RESOLVED against byte dump):
 *   The block-0 next-free pointer is LITTLE-ENDIAN in III+ 1.1.
 *   [verified: TOURS (01 00 00 00 -> 1), TRAVEL (03 00 00 00 -> 3),
 *    UNIVERS (01 00 00 00 -> 1); big-endian readings are absurd for file sizes]
 *
 * Fail loud (CLAUDE.md Rule 2): every structural violation (file too short,
 * bad version dialect, I/O error) is returned as a distinct dbt_err code.
 * A silently-wrong memo would corrupt every memo-field display built on top.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths / nondeterminism (Rule 11).
 * Freestanding-legal (Law 3): only <stdint.h> and samir/ headers.
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/dbt.md:
 *     sec 2   (block model: 512-byte blocks, block n at n*512)
 *     sec 2.1 (block 0: uint32 LE next-free @0x00; remaining 508 = garbage)
 *     sec 2.2 (memo blocks: raw text, no III+ per-block header)
 *     sec 3   (endianness RESOLVED: little-endian for III+)
 *     sec 4   (M-field: 10-byte right-justified ASCII decimal; 0 = no memo)
 *     sec 4.1 (TRAVEL rec1 -> block 1 -> offset 0x200; worked example)
 *     sec 5   (0x1A 0x1A terminator; position table for TRAVEL/TOURS goldens)
 *     sec 6   (append-only: block-0 tail + final-block tail = garbage)
 *     sec 8   (reader/writer recipe: confirm 0x83; trim+atoi M-field; seek n*512;
 *              read until 0x1A 0x1A)
 *   - Verification section of dbt.md (byte-level re-derivation 2026-06-16):
 *     TRAVEL.DBT: block-1 terminator @file-offset 0x27F (block-rel 127);
 *                 block-2 terminator @file-offset 0x455 (block-rel 85);
 *                 block-2 soft-break (0x8D 0x0A) at block-rel 57.
 *     TOURS.DBT:  next-free=1; no live memo blocks; stray 0x1A at byte[512].
 *   - docs/plans/SAMIR-implementation-plan.md S2.1 contract + Phase 2 header.
 *   - os/samir/include/samir/pal.h  (open/read/seek/close vtable).
 *   - os/samir/include/samir/rt.h   (dec_parse: M-field block-number decoder).
 */

#ifndef INITECH_SAMIR_DBT_H
#define INITECH_SAMIR_DBT_H

#include <stdint.h>

#include "samir/pal.h"

/* -----------------------------------------------------------------------
 * Constants (dbt.md sec 2, verified)
 * ----------------------------------------------------------------------- */

/*
 * DBT_BLOCK_SIZE: the fixed 512-byte block size for III+ .dbt files.
 * NOT stored in the file; hard-coded because the .dbf version byte is 0x83.
 * Ref: dbt.md sec 2 "hard-coded 512 in III+"; cl-db3 db3.lisp:368-369.
 * [verified: TRAVEL.DBT block boundaries at 0x000/0x200/0x400]
 */
#define DBT_BLOCK_SIZE   512u

/*
 * DBT_TERM1 / DBT_TERM2: the two-byte terminator that ends every III+ memo.
 * A memo ends at the FIRST occurrence of {0x1A, 0x1A} from its starting block.
 * Ref: dbt.md sec 5 "III+ memo ends at first 0x1A 0x1A"; cl-db3 db3.lisp:396-400.
 * [verified: TRAVEL.DBT block-1 @0x27F, block-2 @0x455]
 */
#define DBT_TERM1        ((uint8_t)0x1Au)
#define DBT_TERM2        ((uint8_t)0x1Au)

/*
 * DBT_HDR_NEXT_OFF: byte offset of the next-free-block uint32 LE in block 0.
 * Ref: dbt.md sec 2.1 "off 0..3 = next-available block (uint32 LE)".
 * [verified: TOURS @0x00 = 01 00 00 00 -> 1; TRAVEL @0x00 = 03 00 00 00 -> 3]
 */
#define DBT_HDR_NEXT_OFF 0u

/* -----------------------------------------------------------------------
 * Error codes (fail loud; Rule 2)
 * dbt_open / dbt_read return DBT_OK (0) on success or a NEGATED dbt_err
 * on failure (return -DBT_ERR_IO, etc.).  Callers compare symbolically.
 * ----------------------------------------------------------------------- */
typedef enum {
    DBT_OK              = 0,   /* success */
    DBT_ERR_IO          = 1,   /* PAL open/read/seek failure */
    DBT_ERR_NOMEM       = 2,   /* PAL arena exhausted (alloc returned NULL) */
    DBT_ERR_SHORT       = 3,   /* file shorter than one 512-byte block */
    DBT_ERR_BAD_VERSION = 4,   /* called with a 0x8B (IV) .dbt; III+ only */
    DBT_ERR_BAD_BLOCK   = 5,   /* blockno == 0 (the header; not a memo block),
                                 * or blockno >= next_free (out of range) */
    DBT_ERR_NO_TERM     = 6    /* 0x1A 0x1A terminator not found within the
                                 * file (malformed / truncated file) */
} dbt_err;

/* -----------------------------------------------------------------------
 * Opaque memo-file handle
 * The full layout lives in os/samir/fs/dbt.c.  Callers reach parsed values
 * only through the accessors below.
 * ----------------------------------------------------------------------- */
typedef struct dbt_file dbt_file;

/* -----------------------------------------------------------------------
 * dbt_open: open the .dbt named `name` through `pal`.
 *
 * Reads and validates block 0:
 *   - File must be at least DBT_BLOCK_SIZE bytes (else DBT_ERR_SHORT).
 *   - The next-free-block pointer (uint32 LE @block-0 offset 0) is decoded
 *     and stored.  A value of 0 is pathological (block 0 is always the
 *     header); the function does not reject it explicitly -- a subsequent
 *     dbt_read with blockno >= next_free will fail loud (DBT_ERR_BAD_BLOCK).
 *   - The PAL handle stays OPEN for later dbt_read calls.
 *
 * The caller is responsible for confirming that the companion .dbf has
 * version byte 0x83 (has_memo set) before calling dbt_open.  dbt_open does
 * not read the .dbf; it trusts the caller.  Pass `is_iv_dialect=1` to have
 * dbt_open return DBT_ERR_BAD_VERSION immediately (reject IV 0x8B callers;
 * the flag lets the caller enforce the III+ constraint without dbt.c needing
 * to read the .dbf).
 *
 * On success returns DBT_OK and *out is a non-NULL handle.
 * On any failure returns -(dbt_err) and *out is NULL; the PAL handle is closed.
 *
 * Ref: dbt.md sec 2.1, sec 3, sec 8 (reader recipe step 1).
 * ----------------------------------------------------------------------- */
int dbt_open(samir_pal_t *pal, const char *name, int is_iv_dialect,
             dbt_file **out);

/* -----------------------------------------------------------------------
 * dbt_close: close the PAL handle held by `f` and free arena memory.
 *
 * NULL is a no-op.  Returns DBT_OK or -(dbt_err) if the PAL close reported
 * a failure; the handle is freed either way.
 * ----------------------------------------------------------------------- */
int dbt_close(dbt_file *f);

/* -----------------------------------------------------------------------
 * dbt_next_free: return the next-free-block number from block-0.
 *
 * This equals the logical 512-byte block count of the file (the index of
 * the first block past live data).  Ref: dbt.md sec 2.1, sec 3.
 * ----------------------------------------------------------------------- */
uint32_t dbt_next_free(const dbt_file *f);

/* -----------------------------------------------------------------------
 * dbt_read: read the memo that starts at `blockno`.
 *
 * Seeks to blockno * DBT_BLOCK_SIZE in the .dbt and reads forward across
 * consecutive 512-byte blocks until 0x1A 0x1A is found.  The bytes BEFORE
 * the terminator are the memo text (CP437; 0x0D 0x0A = hard break,
 * 0x8D 0x0A = soft word-wrap break).
 *
 * Parameters:
 *   f       : open handle from dbt_open.
 *   blockno : starting block number (the decimal value from the .dbf M field).
 *             Must be > 0 (block 0 is the header) and < dbt_next_free(f).
 *             A block number of 0 or all-blank means "no memo" -- the CALLER
 *             must test for this before calling dbt_read (dbt_read returns
 *             -DBT_ERR_BAD_BLOCK for blockno == 0).
 *   buf_out : set to a PAL-arena-allocated buffer containing the raw memo
 *             bytes (NOT NUL-terminated; may contain 0x00 bytes in principle,
 *             though III+ memos are CP437 text).  The buffer is allocated
 *             from the arena on each call; callers that need stable storage
 *             must copy or call before the arena is reset.
 *   len_out : set to the number of bytes in *buf_out (text before the
 *             0x1A 0x1A terminator; does NOT include the terminator bytes).
 *
 * Return value: DBT_OK (0) on success, or a NEGATED dbt_err code:
 *   -DBT_ERR_BAD_BLOCK : blockno == 0 or blockno >= next_free.
 *   -DBT_ERR_IO        : PAL seek or read failure.
 *   -DBT_ERR_NOMEM     : arena exhausted allocating the output buffer.
 *   -DBT_ERR_NO_TERM   : 0x1A 0x1A not found before end of file (malformed).
 *
 * Single-trailing-0x1A tolerance (dbt.md sec 6, S2.1 contract):
 *   TOURS.DBT and UNIVERS.DBT are 513 bytes (one block + one stray 0x1A at
 *   offset 512).  When reading hits EOF after a lone 0x1A without a second
 *   0x1A immediately following, the implementation accepts it as a complete
 *   terminator (the second byte is implicit / stray).  This prevents a
 *   spurious DBT_ERR_NO_TERM on these goldens.
 *   Ref: dbt.md sec 6 "TOURS.DBT ... single stray 0x1A at offset 0x200";
 *   dbt.md sec 5 "TOURS block-0 leftover terminator @0x67 (block-rel 103)".
 *   Note: the stray 0x1A is in block 0 (the header), so it is never seen by
 *   dbt_read in TOURS.DBT's case (TOURS has no live memo blocks, blockno>0
 *   never reached).  The tolerance is kept for robustness against any file
 *   where a memo block exactly ends with a lone 0x1A at file-end.
 *
 * Mutation hook (Rule 6):
 *   -DDBT_MUTATE_BLOCKSIZE: uses 511 instead of 512 for all block boundary
 *   arithmetic (seeks and block-read strides). With this mutant, the seek to
 *   block 1 lands at offset 511 instead of 512; the read pulls bytes that are
 *   shifted by one position relative to the actual memo content. The decoded
 *   text diverges from the golden at every byte checked -> Tier-0 content
 *   checks go RED (length mismatch and text prefix mismatch). This genuinely
 *   bites the READ path because both seek and multi-block stride are affected.
 *
 * Ref: dbt.md sec 2 (block geometry), sec 4 (M-field pointer), sec 4.1
 *      (TRAVEL rec1 worked example), sec 5 (0x1A 0x1A terminator),
 *      sec 8 (reader recipe steps 2-4).
 * ----------------------------------------------------------------------- */
int dbt_read(dbt_file *f, uint32_t blockno,
             uint8_t **buf_out, uint32_t *len_out);

#endif /* INITECH_SAMIR_DBT_H */
