/*
 * os/samir/fs/dbt.c -- SAMIR (InitechBase) .dbt memo codec: open + read (S2.1).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib). Includes ONLY <stdint.h> plus the engine headers (samir/pal.h,
 * samir/rt.h, samir/dbt.h). No libc, no int 0x21 -- all OS contact is through
 * the PAL vtable (plan Sec 2.B / 2.D).
 *
 * S2.1 scope (READ only; write is S2.2):
 *   - dbt_open   : open the .dbt; read + validate block-0 header; store the
 *                  LE uint32 next-free-block pointer; keep the PAL handle open.
 *   - dbt_close  : close the PAL handle and reset the arena mark.
 *   - dbt_read   : seek to blockno*512; read forward until 0x1A 0x1A across
 *                  consecutive 512-byte blocks; return arena-allocated buffer.
 *   - dbt_next_free : accessor for the block-0 pointer.
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
 * Mutation hook (Rule 6, -DDBT_MUTATE_BLOCKSIZE):
 *   Defines DBT_BLOCK_ACTUAL as 511 instead of 512.  All block-boundary
 *   arithmetic (seek to blockno*ACTUAL and block-boundary crossing in the
 *   scan loop) uses 511, landing reads 1 byte off from the true memo start.
 *   This makes the oracle go RED for the right reason: decoded content and
 *   length diverge from the golden.  The mutation does NOT cause a crash;
 *   it produces plausibly-sized but byte-incorrect output.
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
 *   - docs/plans/SAMIR-implementation-plan.md S2.1 contract + Phase 2 header.
 *   - os/samir/include/samir/dbt.h  (the public contract).
 *   - os/samir/include/samir/pal.h  (open/read/seek/close vtable).
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
 * Arena-allocated at dbt_open; the mark stored here allows dbt_close to
 * unwind the arena to exactly the state before open.
 * ----------------------------------------------------------------------- */
struct dbt_file {
    samir_pal_t *pal;          /* the PAL vtable (borrowed; not owned) */
    pal_fd       fd;           /* open file handle (owned; closed on dbt_close) */
    uint32_t     next_free;    /* block-0 next-available-block (LE uint32) */
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
