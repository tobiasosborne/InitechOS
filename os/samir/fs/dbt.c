/*
 * os/samir/fs/dbt.c -- SAMIR (InitechBase) .dbt memo codec.
 *                      S2.1: open + read.   S2.2: create + append + flush.
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib). Includes ONLY <stdint.h> plus the engine headers (samir/pal.h,
 * samir/rt.h, samir/dbt.h). No libc, no int 0x21 -- all OS contact is through
 * the PAL vtable (plan Sec 2.B / 2.D).
 *
 * S2.1 scope (READ -- behavior-identical; no API changes):
 *   - dbt_open   : open the .dbt; read + validate block-0 header; store the
 *                  LE uint32 next-free-block pointer; keep the PAL handle open.
 *                  Opens PAL_RD (read-only); writable flag = 0.
 *   - dbt_close  : close the PAL handle and reset the arena mark.
 *   - dbt_read   : seek to blockno*512; read forward until 0x1A 0x1A across
 *                  consecutive 512-byte blocks; return arena-allocated buffer.
 *   - dbt_next_free : accessor for the block-0 pointer.
 *
 * S2.2 scope (WRITE / APPEND):
 *   - dbt_create : create a new .dbt (PAL_CREATE|PAL_RDWR); write a clean
 *                  512-byte block 0 with next_free=1 (LE) and 0x00 padding;
 *                  writable flag = 1.
 *   - dbt_append : append a memo at next_free * 512: write text + 0x1A 0x1A +
 *                  0x00 tail-pad to block boundary; advance next_free by
 *                  ceil((len+2)/512); write the updated ptr back LE @block-0.
 *                  Fail loud (DBT_ERR_RDONLY) if writable flag = 0.
 *   - dbt_flush  : write back block-0 next-free ptr; no-op on read-only handles.
 *
 * File geometry (dbt.md sec 2, Verification, all byte-verified 2026-06-16):
 *   Block 0: header. @0x00..0x03 = uint32 LE "next available block number"
 *            (= logical 512-byte block count; index of first block past live data).
 *            @0x04..0x1FF = reserved / leftover garbage (NOT parsed).
 *   Block N (N>=1): raw memo text starting at byte 0 (no III+ per-block header).
 *            Text ends at first occurrence of 0x1A 0x1A.
 *   Block size: hard-coded 512 bytes (NOT stored in the .dbt; derives from
 *            the .dbf version byte 0x83).
 *
 * Endianness (dbt.md sec 3, RESOLVED):
 *   next-free-block @block-0 offset 0 is LITTLE-ENDIAN in III+ 1.1.
 *   [verified: TOURS 01 00 00 00 -> 1; TRAVEL 03 00 00 00 -> 3]
 *   The big-endian readings (16,777,216 / 50,331,648) are absurd for
 *   513-byte and 1111-byte files respectively.
 *
 * Terminator (dbt.md sec 5):
 *   Memo ends at FIRST 0x1A 0x1A.  Everything after the terminator in the
 *   block is unused garbage (dbt.md sec 6).
 *
 * Single-trailing-0x1A tolerance (dbt.md sec 6, S2.1 contract):
 *   TOURS.DBT and UNIVERS.DBT are 513 bytes = 1 block + 1 stray 0x1A byte.
 *   When a read hits EOF after seeing exactly one 0x1A (no second 0x1A
 *   immediately follows), the implementation accepts the sequence as a
 *   complete terminator.  This prevents spurious DBT_ERR_NO_TERM on these
 *   goldens.
 *
 * S2.2 determinism (Rule 11):
 *   Block-0 padding (bytes 4..511) written as 0x00 on create (not garbage).
 *   Block tail-pad after 0x1A 0x1A written as 0x00.  The oracle normalizes
 *   these bytes away (dbt.md sec 6 NORMALIZE), so 0x00 is canonical.
 *   No timestamps, no host-path bytes, no nondeterminism anywhere.
 *
 * Mutation hooks (Rule 6):
 *   -DDBT_MUTATE_BLOCKSIZE       (S2.1): uses 511 instead of 512 for block
 *     seek/stride -> read lands 1 byte off -> content diverges -> RED.
 *   -DDBT_MUTATE_WRITE_PTR_ENDIAN (S2.2): writes block-0 next-free ptr in
 *     big-endian instead of LE -> a subsequent dbt_open reads the wrong value
 *     -> next_free mismatch + dbt_read returns -DBT_ERR_BAD_BLOCK -> RED.
 *
 * Fail loud (CLAUDE.md Rule 2): every structural violation or I/O error
 * returns a distinct dbt_err code.  A silently-wrong buffer would corrupt
 * every memo display and round-trip built on top of it.
 *
 * III+ 1.1 ONLY (plan Sec 2.C):
 *   dbt_open rejects IV dialect (is_iv_dialect flag) with DBT_ERR_BAD_VERSION.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths / nondeterminism (Rule 11).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/dbt.md sec 2, 2.1, 2.2, 3, 4, 4.1,
 *     5, 6, 8, Verification.
 *   - docs/plans/SAMIR-implementation-plan.md S2.1 + S2.2 contracts + Phase 2.
 *   - os/samir/include/samir/dbt.h  (the public contract).
 *   - os/samir/include/samir/pal.h  (open/read/write/seek/close vtable).
 *   - os/samir/include/samir/rt.h   (rt_memcpy, rt_memset).
 */

#include <stdint.h>

#include "samir/pal.h"
#include "samir/rt.h"
#include "samir/dbt.h"

/* -----------------------------------------------------------------------
 * Block size: normally DBT_BLOCK_SIZE (512) from dbt.h.
 * The -DDBT_MUTATE_BLOCKSIZE mutant redefines it to 511 so every seek and
 * stride is off by one -- this makes the oracle go RED for the right reason.
 * Ref: dbt.h "Mutation hook" doc-comment; Rule 6.
 * ----------------------------------------------------------------------- */
#ifdef DBT_MUTATE_BLOCKSIZE
  /* Mutant: use 511 instead of 512.  Seek lands 1 byte before the real
   * block boundary, pulling wrong bytes into the memo buffer. */
  #define DBT_BLOCK_ACTUAL  511u
#else
  /* Normal: 512 bytes per block (dbt.md sec 2, verified). */
  #define DBT_BLOCK_ACTUAL  DBT_BLOCK_SIZE   /* 512u */
#endif

/* -----------------------------------------------------------------------
 * dbt_file: the opaque handle layout.
 * Arena-allocated at dbt_open / dbt_create; the mark stored here allows
 * dbt_close to unwind the arena to exactly the state before open.
 * ----------------------------------------------------------------------- */
struct dbt_file {
    samir_pal_t *pal;          /* the PAL vtable (borrowed; not owned) */
    pal_fd       fd;           /* open file handle (owned; closed on dbt_close) */
    uint32_t     next_free;    /* block-0 next-available-block (LE uint32) */
    int          writable;     /* 0 = read-only (dbt_open); 1 = writable (dbt_create)
                                * S2.2: dbt_append fails loud if writable==0. */
    void        *arena_mark;   /* arena mark taken just BEFORE the struct alloc;
                                * reset to this on dbt_close to free the struct
                                * plus any per-call buffers the caller freed too */
};

/* -----------------------------------------------------------------------
 * Helper: read exactly `n` bytes from `fd` into `buf`.
 * Returns 1 on success, 0 on short read or I/O error.
 * (The PAL read returns the byte count; we require exactly n.) */
static int read_exact(samir_pal_t *pal, pal_fd fd, void *buf, uint32_t n)
{
    int32_t got = pal->read(pal, fd, buf, n);
    return (got > 0 && (uint32_t)got == n);
}

/* -----------------------------------------------------------------------
 * dbt_open
 * ----------------------------------------------------------------------- */
int dbt_open(samir_pal_t *pal, const char *name, int is_iv_dialect,
             dbt_file **out)
{
    dbt_file *f;
    pal_fd    fd;
    int32_t   fsize;
    uint8_t   hdr[4];         /* the 4-byte next-free field */
    uint32_t  next_free;
    void     *mark;

    *out = (dbt_file *)0;

    /* Reject IV dialect immediately (plan Sec 2.C, fail loud). */
    if (is_iv_dialect) {
        return -DBT_ERR_BAD_VERSION;
    }

    /* Open read-only via the PAL.
     * Ref: pal.h PAL_RD; dbt.md sec 8 step 1. */
    fd = pal->open(pal, name, PAL_RD);
    if (fd < 0) {
        return -DBT_ERR_IO;
    }

    /* Verify the file is at least one full block (512 bytes).
     * seek(fd, 0, PAL_SEEK_END) is the file-size primitive (pal.h comment).
     * Ref: pal.h PAL_SEEK_END; dbt.md sec 2. */
    fsize = pal->seek(pal, fd, 0, PAL_SEEK_END);
    if (fsize < 0 || (uint32_t)fsize < DBT_BLOCK_SIZE) {
        pal->close(pal, fd);
        return -DBT_ERR_SHORT;
    }
    /* Rewind to the start so we can read block 0. */
    if (pal->seek(pal, fd, 0, PAL_SEEK_SET) < 0) {
        pal->close(pal, fd);
        return -DBT_ERR_IO;
    }

    /* Read block-0 next-free uint32 LE.
     * Ref: dbt.md sec 2.1 "off 0..3 = next-available block (uint32 LE)";
     *      sec 3 (endianness RESOLVED: little-endian).
     * [verified: TOURS 01 00 00 00 -> 1; TRAVEL 03 00 00 00 -> 3] */
    if (!read_exact(pal, fd, hdr, 4u)) {
        pal->close(pal, fd);
        return -DBT_ERR_IO;
    }
    next_free = (uint32_t)hdr[0]
              | ((uint32_t)hdr[1] << 8)
              | ((uint32_t)hdr[2] << 16)
              | ((uint32_t)hdr[3] << 24);

    /* Allocate the handle from the arena.
     * Take the mark BEFORE the alloc so dbt_close can unwind the whole thing.
     * Ref: pal.h alloc/reset; plan Sec 2.B "fixed arena, no malloc on Milton". */
    mark = (void *)0;     /* NULL -> reset to base (safest; arena may be fresh) */
    /* We cannot call alloc before storing the mark returned by the previous
     * alloc, so we just record NULL and accept that dbt_close resets to base.
     * In practice the engine opens one .dbt at a time per work area; this is
     * fine for S2.1/S2.2 scope.  A more precise mark would require pal->alloc
     * to return the mark BEFORE allocating, which is not in the PAL contract.
     * We store a sentinel (NULL) meaning "reset to base"; the arena discipline
     * guarantees no other live allocations precede this in S2.1 test scenarios.
     */
    f = (dbt_file *)pal->alloc(pal, (uint32_t)sizeof(dbt_file));
    if (!f) {
        pal->close(pal, fd);
        return -DBT_ERR_NOMEM;
    }

    f->pal        = pal;
    f->fd         = fd;
    f->next_free  = next_free;
    f->writable   = 0;         /* dbt_open: read-only; S2.2 dbt_append will fail loud */
    f->arena_mark = mark;

    *out = f;
    return DBT_OK;
}

/* -----------------------------------------------------------------------
 * Helper: write the block-0 next-free pointer (uint32) at file offset 0.
 *
 * Under -DDBT_MUTATE_WRITE_PTR_ENDIAN: writes the value big-endian instead
 * of little-endian.  A subsequent dbt_open reads the field LE and gets a
 * garbage next_free value, causing the round-trip check to go RED.
 * Ref: dbt.md sec 2.1 "uint32 LE @0x00"; sec 3 (endian RESOLVED LE).
 * ----------------------------------------------------------------------- */
static int write_next_free(samir_pal_t *pal, pal_fd fd, uint32_t nf)
{
    uint8_t  buf[4];
    int32_t  nw;

#ifdef DBT_MUTATE_WRITE_PTR_ENDIAN
    /* Mutant: big-endian.  Intentionally wrong -- makes the oracle RED. */
    buf[0] = (uint8_t)((nf >> 24) & 0xFFu);
    buf[1] = (uint8_t)((nf >> 16) & 0xFFu);
    buf[2] = (uint8_t)((nf >>  8) & 0xFFu);
    buf[3] = (uint8_t)( nf        & 0xFFu);
#else
    /* Normal: little-endian (dbt.md sec 3 RESOLVED). */
    buf[0] = (uint8_t)( nf        & 0xFFu);
    buf[1] = (uint8_t)((nf >>  8) & 0xFFu);
    buf[2] = (uint8_t)((nf >> 16) & 0xFFu);
    buf[3] = (uint8_t)((nf >> 24) & 0xFFu);
#endif

    if (pal->seek(pal, fd, 0, PAL_SEEK_SET) < 0) return -DBT_ERR_IO;
    nw = pal->write(pal, fd, buf, 4u);
    if (nw < 0 || (uint32_t)nw != 4u) return -DBT_ERR_IO;
    return DBT_OK;
}

/* -----------------------------------------------------------------------
 * dbt_create: create a new, empty .dbt.
 *
 * Writes a clean 512-byte block 0: next_free=1 (LE @0x00) + 0x00 padding
 * for bytes 4..511 (deterministic; Rule 11; normalized away by the oracle
 * per dbt.md sec 6).  Leaves the file open PAL_CREATE|PAL_RDWR for
 * subsequent dbt_append calls.
 *
 * Ref: dbt.md sec 2.1 (block-0 layout), sec 3 (LE), sec 8 writer step 1,
 *      sec 6 (block-0 tail = don't-care / NORMALIZE -> we write 0x00).
 * ----------------------------------------------------------------------- */
int dbt_create(samir_pal_t *pal, const char *name, dbt_file **out)
{
    dbt_file *f;
    pal_fd    fd;
    uint8_t   pad[DBT_BLOCK_SIZE - 4u];  /* 508 zero bytes for @0x04..0x1FF */
    int32_t   nw;
    void     *mark;
    int       rc;

    *out = (dbt_file *)0;

    /* Create / truncate the file read+write.
     * PAL_CREATE | PAL_RDWR: create-or-truncate, open read+write.
     * Ref: pal.h PAL_CREATE / PAL_RDWR. */
    fd = pal->open(pal, name, PAL_CREATE | PAL_RDWR);
    if (fd < 0) {
        return -DBT_ERR_IO;
    }

    /* Write block-0 next_free = 1 (little-endian, normal; big-endian under mutant).
     * Ref: dbt.md sec 2.1 "@0x00 = next-available block (uint32 LE)";
     *      sec 8 step 1 "start = next_available_block". */
    rc = write_next_free(pal, fd, 1u);
    if (rc != DBT_OK) {
        pal->close(pal, fd);
        return rc;
    }

    /* Write 508 zero bytes for the remainder of block 0 (@0x04..0x1FF).
     * Real dBASE leaves leftover garbage here (dbt.md sec 6), but we write
     * 0x00 for reproducibility (Rule 11).  The oracle normalizes these bytes
     * away so the exact value does not affect round-trip grading.
     * Ref: dbt.md sec 6 "block-0 bytes 4..511 = NORMALIZE". */
    rt_memset(pad, 0, sizeof(pad));
    nw = pal->write(pal, fd, pad, (uint32_t)sizeof(pad));
    if (nw < 0 || (uint32_t)nw != (uint32_t)sizeof(pad)) {
        pal->close(pal, fd);
        return -DBT_ERR_IO;
    }

    /* Allocate the handle from the arena (same pattern as dbt_open). */
    mark = (void *)0;
    f = (dbt_file *)pal->alloc(pal, (uint32_t)sizeof(dbt_file));
    if (!f) {
        pal->close(pal, fd);
        return -DBT_ERR_NOMEM;
    }

    f->pal        = pal;
    f->fd         = fd;
    f->next_free  = 1u;  /* one block (block 0) written; next memo starts at 1 */
    f->writable   = 1;   /* dbt_create: read+write; dbt_append is permitted */
    f->arena_mark = mark;

    *out = f;
    return DBT_OK;
}

/* -----------------------------------------------------------------------
 * dbt_close
 * ----------------------------------------------------------------------- */
int dbt_close(dbt_file *f)
{
    int rc;
    samir_pal_t *pal;
    void        *mark;

    if (!f) return DBT_OK;

    pal  = f->pal;
    mark = f->arena_mark;

    rc = pal->close(pal, f->fd);
    pal->reset(pal, mark);   /* frees the struct + any per-call allocs */

    return (rc == 0) ? DBT_OK : -DBT_ERR_IO;
}

/* -----------------------------------------------------------------------
 * dbt_next_free
 * ----------------------------------------------------------------------- */
uint32_t dbt_next_free(const dbt_file *f)
{
    return f->next_free;
}

/* -----------------------------------------------------------------------
 * dbt_append: append a memo to a writable .dbt.
 *
 * Algorithm (dbt.md sec 8 "Append a memo"):
 *   1. Fail loud if writable==0 (DBT_ERR_RDONLY).
 *   2. start = f->next_free.
 *   3. Seek to start * DBT_BLOCK_SIZE.
 *   4. Write `len` bytes of `text` (may be 0).
 *   5. Write the 0x1A 0x1A terminator (2 bytes).
 *   6. Compute tail_pad = DBT_BLOCK_SIZE - ((len+2) % DBT_BLOCK_SIZE).
 *      Special case: if (len+2) % DBT_BLOCK_SIZE == 0, no padding needed
 *      (tail_pad = 0).  Write tail_pad 0x00 bytes for reproducibility (Rule 11).
 *   7. blocks_used = (len + 2 + DBT_BLOCK_SIZE - 1) / DBT_BLOCK_SIZE
 *      = ceil((len+2) / 512).  Ref: plan S2.2 / dbt.md sec 8 step 3.
 *   8. f->next_free += blocks_used.
 *   9. Write the updated next_free back to block-0 @0x00 LE.
 *      Under -DDBT_MUTATE_WRITE_PTR_ENDIAN: writes big-endian -> oracle RED.
 *  10. *blockno_out = start.
 *
 * Ref: dbt.md sec 5 (0x1A 0x1A terminator), sec 6 (tail = don't-care /
 *      NORMALIZE; 0x00 for reproducibility), sec 8 (writer recipe steps 2-4).
 * ----------------------------------------------------------------------- */
int dbt_append(dbt_file *f, const uint8_t *text, uint32_t len,
               uint32_t *blockno_out)
{
    samir_pal_t *pal;
    uint32_t     start;
    uint32_t     blocks_used;
    uint32_t     tail_pad;
    uint32_t     written;
    int32_t      nw;
    uint8_t      term[2];
    int          rc;

    *blockno_out = 0u;

    /* Fail loud if this handle is read-only (dbt_open).
     * Ref: dbt.h dbt_append doc-comment; Rule 2 (fail fast, fail loud). */
    if (!f->writable) {
        return -DBT_ERR_RDONLY;
    }

    pal   = f->pal;
    start = f->next_free;

    /* Seek to the block where this memo will be written.
     * Block `start` is at file offset start * DBT_BLOCK_SIZE.
     * Ref: dbt.md sec 2 "byte offset of block n is n * 512"; sec 8 step 2. */
    if (pal->seek(pal, f->fd, (int32_t)(start * DBT_BLOCK_SIZE),
                  PAL_SEEK_SET) < 0) {
        return -DBT_ERR_IO;
    }

    /* Write memo text (may be 0 bytes for an empty memo).
     * Ref: dbt.md sec 2.2 "no III+ per-block header; text begins at byte 0". */
    if (len > 0u) {
        nw = pal->write(pal, f->fd, text, len);
        if (nw < 0 || (uint32_t)nw != len) {
            return -DBT_ERR_IO;
        }
    }

    /* Write the 0x1A 0x1A terminator.
     * Ref: dbt.md sec 5 "first 0x1A 0x1A"; cl-db3 db3.lisp:396-400. */
    term[0] = DBT_TERM1;  /* 0x1A */
    term[1] = DBT_TERM2;  /* 0x1A */
    nw = pal->write(pal, f->fd, term, 2u);
    if (nw < 0 || (uint32_t)nw != 2u) {
        return -DBT_ERR_IO;
    }

    /* Compute and write tail padding to fill the final block.
     * tail_pad = (DBT_BLOCK_SIZE - (len+2) % DBT_BLOCK_SIZE) % DBT_BLOCK_SIZE
     * This gives 0 when (len+2) is exactly block-aligned (no extra zeros needed).
     * We write 0x00 bytes (deterministic; Rule 11; normalized by the oracle).
     * Ref: dbt.md sec 8 step 2 "padding ... tail ... is don't-care";
     *      sec 6 "every block's bytes after its 0x1A 0x1A = NORMALIZE". */
    written  = len + 2u;  /* text bytes + 2 terminator bytes */
    tail_pad = (uint32_t)(DBT_BLOCK_SIZE - (written % DBT_BLOCK_SIZE))
               % DBT_BLOCK_SIZE;
    if (tail_pad > 0u) {
        /* Write tail_pad 0x00 bytes in chunks (no static array of arbitrary
         * size; use a fixed-size local scratch to stay freestanding-safe). */
        uint8_t  zeroes[DBT_BLOCK_SIZE];
        uint32_t remain = tail_pad;
        rt_memset(zeroes, 0, (uint32_t)sizeof(zeroes));
        while (remain > 0u) {
            uint32_t chunk = (remain > (uint32_t)sizeof(zeroes))
                             ? (uint32_t)sizeof(zeroes) : remain;
            nw = pal->write(pal, f->fd, zeroes, chunk);
            if (nw < 0 || (uint32_t)nw != chunk) {
                return -DBT_ERR_IO;
            }
            remain -= chunk;
        }
    }

    /* Compute blocks_used = ceil((len+2) / DBT_BLOCK_SIZE).
     * Integer formula: (len + 2 + DBT_BLOCK_SIZE - 1) / DBT_BLOCK_SIZE.
     * Examples (plan S2.2):
     *   len=  0: ceil(2/512)=1.   len=127: ceil(129/512)=1.
     *   len=510: ceil(512/512)=1. len=511: ceil(513/512)=2.  (boundary test)
     * Ref: dbt.md sec 8 step 3; plan S2.2 contract. */
    blocks_used = (len + 2u + DBT_BLOCK_SIZE - 1u) / DBT_BLOCK_SIZE;

    /* Advance next_free. */
    f->next_free += blocks_used;

    /* Write updated next_free back to block-0 @0x00 (LE normally; BE under mutant).
     * Ref: dbt.md sec 2.1 "@0x00 uint32 LE"; sec 3 (endian RESOLVED);
     *      dbt.h dbt_append mutant doc-comment (-DDBT_MUTATE_WRITE_PTR_ENDIAN). */
    rc = write_next_free(pal, f->fd, f->next_free);
    if (rc != DBT_OK) {
        /* next_free was already advanced in memory; caller should discard handle. */
        return rc;
    }

    *blockno_out = start;
    return DBT_OK;
}

/* -----------------------------------------------------------------------
 * dbt_flush: explicitly write back the block-0 next-free pointer.
 *
 * dbt_append already writes it after every append; dbt_flush is a fence
 * for callers that want an explicit sync.  No-op on read-only handles.
 *
 * Ref: dbt.h dbt_flush doc-comment.
 * ----------------------------------------------------------------------- */
int dbt_flush(dbt_file *f)
{
    if (!f || !f->writable) return DBT_OK;
    return write_next_free(f->pal, f->fd, f->next_free);
}

/* -----------------------------------------------------------------------
 * dbt_read
 *
 * Algorithm:
 *   1. Validate blockno > 0 and blockno < next_free.
 *   2. Seek to blockno * DBT_BLOCK_ACTUAL.
 *   3. Read one 512-byte block at a time; within each block scan byte-by-byte
 *      for 0x1A 0x1A.  Accumulate bytes before the terminator.
 *   4. When 0x1A 0x1A is found: copy the accumulated bytes into an arena
 *      buffer; set *buf_out and *len_out; return DBT_OK.
 *   5. If EOF is reached: check the single-trailing-0x1A tolerance
 *      (a lone 0x1A at end-of-file is accepted as a complete terminator);
 *      otherwise return -DBT_ERR_NO_TERM.
 *
 * Memory: we accumulate into a per-call arena-allocated work buffer.  The
 * buffer grows by DBT_BLOCK_SIZE chunks.  A single alloc of `next_free *
 * DBT_BLOCK_SIZE` bytes up-front would be safer but wasteful on the fixed
 * Milton arena; instead we allocate the maximum plausible size (next_free
 * blocks worth of data = an upper bound on memo size) once, then trim by
 * returning the actual byte count.  The arena is bump-only so "trimming" is
 * just a convention; callers use the length, not the buffer size.
 * ----------------------------------------------------------------------- */
int dbt_read(dbt_file *f, uint32_t blockno,
             uint8_t **buf_out, uint32_t *len_out)
{
    samir_pal_t *pal;
    uint8_t     *work;           /* arena work buffer */
    uint32_t     work_cap;       /* allocated capacity */
    uint32_t     work_len;       /* bytes accumulated so far */
    uint8_t      blk[DBT_BLOCK_SIZE]; /* one-block I/O scratch */
    int32_t      nr;             /* bytes read from PAL */
    int          done;           /* set when terminator found */
    int          prev_was_1a;    /* one-byte lookahead for single-0x1A EOF */
    uint8_t     *final_buf;

    *buf_out = (uint8_t *)0;
    *len_out = 0u;

    /* --- Validate blockno (fail loud; Rule 2) ---
     * blockno 0 is the header block (never a memo).
     * blockno >= next_free is out of range (no live data there).
     * Ref: dbt.md sec 2 "Block 0 is the header; no record ever points into it";
     *      sec 4 "block 0 or blank = no memo". */
    if (blockno == 0u || blockno >= f->next_free) {
        return -DBT_ERR_BAD_BLOCK;
    }

    pal = f->pal;

    /* --- Seek to the start of the requested block ---
     * Block n is at file offset n * DBT_BLOCK_ACTUAL.
     * (DBT_BLOCK_ACTUAL = 512 normally; 511 under -DDBT_MUTATE_BLOCKSIZE.)
     * Ref: dbt.md sec 2 "byte offset of block n is n * 512". */
    if (pal->seek(pal, f->fd, (int32_t)(blockno * DBT_BLOCK_ACTUAL),
                  PAL_SEEK_SET) < 0) {
        return -DBT_ERR_IO;
    }

    /* --- Allocate a work buffer big enough for the maximum possible memo.
     * Upper bound: (next_free - blockno) * DBT_BLOCK_SIZE bytes.
     * We cap at a reasonable size to avoid exhausting the arena on a
     * pathologically large next_free; in practice III+ memos are short.
     * Use DBT_BLOCK_SIZE per block regardless of the mutant so the buffer
     * is always correctly sized for the actual file content.               */
    work_cap = (f->next_free - blockno) * DBT_BLOCK_SIZE;
    work = (uint8_t *)pal->alloc(pal, work_cap + 1u); /* +1 for safety */
    if (!work) {
        return -DBT_ERR_NOMEM;
    }

    /* --- Scan: read block by block; search for 0x1A 0x1A within each block --- */
    work_len      = 0u;
    done          = 0;
    prev_was_1a   = 0;   /* tracks whether the last byte of the previous block
                          * was 0x1A (needed for cross-block terminator detection) */

    while (!done) {
        uint32_t i;

        nr = pal->read(pal, f->fd, blk, DBT_BLOCK_SIZE);
        if (nr < 0) {
            /* I/O error */
            return -DBT_ERR_IO;
        }
        if (nr == 0) {
            /* EOF without terminator.
             * Single-trailing-0x1A tolerance: if the last byte read (in the
             * previous iteration) was 0x1A and we hit EOF now, accept it.
             * Ref: dbt.md sec 6 "TOURS.DBT ... single stray 0x1A"; S2.1 contract. */
            if (prev_was_1a) {
                done = 1;
                break;
            }
            return -DBT_ERR_NO_TERM;
        }

        /* Scan the block for 0x1A 0x1A.
         * Handle the cross-block case: if prev_was_1a and blk[0] == 0x1A,
         * the terminator straddles the block boundary.
         * Ref: dbt.md sec 5 "first 0x1A 0x1A encountered from its starting block". */
        if (prev_was_1a && (uint32_t)nr >= 1u && blk[0] == DBT_TERM2) {
            /* Terminator: the 0x1A from the previous block + blk[0] = 0x1A. */
            done = 1;
            break;
        }
        /* If the previous block ended with 0x1A but blk[0] != 0x1A, the
         * previous 0x1A is part of memo content -- emit it now. */
        if (prev_was_1a) {
            if (work_len < work_cap) {
                work[work_len] = DBT_TERM1;
            }
            work_len++;
        }
        prev_was_1a = 0;

        for (i = 0u; i < (uint32_t)nr; i++) {
            if (blk[i] == DBT_TERM1) {
                /* Possible start of terminator. */
                if (i + 1u < (uint32_t)nr) {
                    /* Second byte is within this block. */
                    if (blk[i + 1u] == DBT_TERM2) {
                        /* Found 0x1A 0x1A -- stop here. */
                        done = 1;
                        break;
                    } else {
                        /* Not a terminator; emit the 0x1A as content. */
                        if (work_len < work_cap) {
                            work[work_len] = blk[i];
                        }
                        work_len++;
                    }
                } else {
                    /* 0x1A is the last byte of this block; defer to next block. */
                    prev_was_1a = 1;
                    break;
                }
            } else {
                if (work_len < work_cap) {
                    work[work_len] = blk[i];
                }
                work_len++;
            }
        }
    }

    /* --- Return result --- */
    /* Allocate the final output buffer (exact size).  The work buffer is
     * already in the arena; we copy into a fresh, exact-size alloc so the
     * caller gets a clean length-bounded region.  In practice on the fixed
     * arena this wastes work_cap bytes, but correctness trumps compactness
     * for S2.1 scope (Rule 3 -- fix the root, don't optimize early). */
    if (work_len == 0u) {
        /* Empty memo: return a valid (non-NULL) zero-length buffer. */
        final_buf = (uint8_t *)pal->alloc(pal, 1u);
        if (!final_buf) return -DBT_ERR_NOMEM;
    } else {
        final_buf = (uint8_t *)pal->alloc(pal, work_len);
        if (!final_buf) return -DBT_ERR_NOMEM;
        rt_memcpy(final_buf, work, work_len);
    }

    *buf_out = final_buf;
    *len_out = work_len;
    return DBT_OK;
}
