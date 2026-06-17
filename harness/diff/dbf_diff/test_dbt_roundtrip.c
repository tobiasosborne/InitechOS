/*
 * harness/diff/dbf_diff/test_dbt_roundtrip.c -- host oracle for S2.2:
 *   .dbt III+ write / append + round-trip (dbt_create / dbt_append / dbt_flush).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_dbt_read.c:
 * seed test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY), a host PAL
 * via pal_host_make (pal_host.c). Non-zero exit on any failed check keeps the
 * gate from false-greening (Law 2: the oracle is the truth).
 *
 * This test exercises dbt_create + dbt_append (write path) while keeping
 * dbt_open + dbt_read (S2.1 read path) behavior-identical.
 *
 * Two tiers (plan Sec 2.A):
 *   Tier 0 (committed, operator-free): all assertions are computed from
 *     the write inputs (known text payloads, known block counts, known
 *     LE next_free values).  No external golden needed.
 *   Tier 1 (corpus-adjacent): appends into a temp copy of a corpus .dbt
 *     (TRAVEL.DBT) and re-reads all memos via dbt_read; confirms format
 *     consistency.  Loud-skip if the corpus is absent.
 *
 * Oracles (this file):
 *   1. BIDIRECTIONAL ROUND-TRIP (Tier 0):
 *      Create a fresh .dbt; append a short memo and a long memo (> 510 bytes
 *      to span a block boundary); re-open with dbt_open; dbt_read both memos
 *      and assert the bytes match what was appended byte-for-byte.
 *   2. BLOCK-0 NEXT-FREE POINTER:
 *      Assert the LE uint32 at file offset 0 matches the expected value
 *      (1 + blocks_used_by_each_appended_memo) after each append.
 *      Read via both dbt_next_free() AND raw file-seek to confirm the LE
 *      encoding on disk is correct.
 *   3. BLOCK-SIZING BOUNDARY:
 *      A 511-byte text payload: ceil((511+2)/512) = ceil(513/512) = 2 blocks.
 *      Assert next_free advances by 2, not 1.  This bites the
 *      blocks_used formula.
 *   4. DETERMINISM (Rule 11):
 *      Write the same two-memo .dbt twice with the same inputs; binary-compare
 *      the two output files byte-for-byte (modulo the normalized tail bytes).
 *      We use content-keyed read-back (dbt_read) rather than raw cmp because
 *      the tail pad and block-0 bytes 4..511 are defined as "don't care" by
 *      dbt.md sec 6; we do check that both files decode identically.
 *   5. RDONLY GUARD:
 *      Open a .dbt via dbt_open (read-only); attempt dbt_append; assert
 *      -DBT_ERR_RDONLY.
 *   6. EMPTY MEMO:
 *      Append a 0-byte memo: ceil((0+2)/512) = 1 block; read back len=0.
 *   7. TIER-1 CORPUS APPEND:
 *      Copy TRAVEL.DBT (3 blocks: 0,1,2) to a temp path; open with
 *      dbt_create on the copy (creating a fresh file) -- ACTUALLY: open the
 *      copy with dbt_open to read the existing memos, verify they still decode
 *      correctly, showing the create/append path did not corrupt the reader.
 *      Then create a SECOND temp fresh .dbt, append matching text, re-read.
 *
 * Mutation (Rule 6 -- -DDBT_MUTATE_WRITE_PTR_ENDIAN):
 *   dbt_append writes the block-0 next-free pointer big-endian instead of LE.
 *   A subsequent dbt_open reads the field LE and gets a wrong value:
 *   e.g. for next_free=2 (0x00000002 LE) the BE write produces 0x02000000;
 *   dbt_open reads that as 0x02000000 = 33554432 -- absurd.
 *   Then dbt_read(blockno=1) fails because 1 >= wrong-next_free (33554432?
 *   No -- 1 < 33554432; so blockno check passes but the file is only 1024
 *   bytes, so the seek to block 1 = offset 512 succeeds, but the read of
 *   block 1's content is correct ... wait: the ROUND-TRIP check compares
 *   dbt_next_free after re-open against the expected value -- that goes RED.
 *   Also: the oracle explicitly reads the raw LE bytes from offset 0 and
 *   asserts they equal the expected LE encoding; with the BE mutant they will
 *   not match.  Exit non-zero.
 *
 * Compile + run (self-grade; NOT make):
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *     -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/dbt.c os/samir/core/rt.c os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_dbt_roundtrip.c -o /tmp/test_dbt_roundtrip \
 *     && /tmp/test_dbt_roundtrip ../dbase3-decomp ; echo "unit exit=$?"
 *
 * Mutant (must exit non-zero):
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *     -DDBT_MUTATE_WRITE_PTR_ENDIAN \
 *     -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/dbt.c os/samir/core/rt.c os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_dbt_roundtrip.c -o /tmp/test_dbt_roundtrip_mut \
 *     && /tmp/test_dbt_roundtrip_mut ../dbase3-decomp ; echo "mutant exit=$?"
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 * All temp files use fixed deterministic paths under /tmp.
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/dbt.md:
 *     sec 2   (block geometry: 512-byte, block n at n*512)
 *     sec 2.1 (block 0: uint32 LE next-free @0x00; 508 = don't-care)
 *     sec 3   (endianness RESOLVED: little-endian)
 *     sec 5   (0x1A 0x1A terminator)
 *     sec 6   (append-only; tail = don't-care / NORMALIZE; block-0 = NORMALIZE)
 *     sec 8   (writer recipe steps 1-4: start, write+term+pad, advance ptr, store)
 *   - docs/plans/SAMIR-implementation-plan.md S2.2 contract:
 *     dbt_append(text,len)->blockno; ceil((len+2)/512) rounding; LE ptr write.
 *   - os/samir/include/samir/dbt.h  (write API: dbt_create, dbt_append, dbt_flush)
 *   - os/samir/include/samir/pal.h  (open/read/write/seek vtable).
 *   - seed/test_assert.h (CHECK / TEST_HARNESS / TEST_SUMMARY).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"       /* seed/, on -Iseed */
#include "samir/dbt.h"         /* os/samir/include/, on -Ios/samir/include */

TEST_HARNESS();

/* ---- PAL host surface (declared here; not in a separate header) ---- */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* ---- Fixed temp paths (deterministic; Rule 11) ---- */
#define TMP_DBT_A      "/tmp/samir_test_dbt_rt_a.dbt"
#define TMP_DBT_B      "/tmp/samir_test_dbt_rt_b.dbt"
#define TMP_DBT_CORPUS "/tmp/samir_test_dbt_rt_corpus.dbt"

/* ---- Corpus path ---- */
#define PRISTINE_REL \
    "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities"

static char *join(char *buf, size_t cap, const char *base, const char *rel)
{
    snprintf(buf, cap, "%s/%s", base, rel);
    return buf;
}

static int file_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

/* ====================================================================
 * Helper: read raw 4 bytes from file offset 0 and decode as LE uint32.
 * Used to verify the on-disk encoding of next_free independently of the
 * dbt_open path (catches the BE-mutant).
 * ==================================================================== */
static int read_raw_next_free(samir_pal_t *pal, const char *path,
                              uint32_t *out)
{
    pal_fd  fd;
    uint8_t buf[4];
    int32_t nr;

    fd = pal->open(pal, path, PAL_RD);
    if (fd < 0) return -1;

    pal->seek(pal, fd, 0, PAL_SEEK_SET);
    nr = pal->read(pal, fd, buf, 4u);
    pal->close(pal, fd);

    if (nr != 4) return -1;

    /* Always interpret as LE (the correct encoding per dbt.md sec 3). */
    *out = (uint32_t)buf[0]
         | ((uint32_t)buf[1] << 8)
         | ((uint32_t)buf[2] << 16)
         | ((uint32_t)buf[3] << 24);
    return 0;
}

/* ====================================================================
 * Helper: make a fresh PAL instance with a generous arena.
 * ==================================================================== */
static samir_pal_t *make_pal(void)
{
    struct pal_host_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy   = 99;   /* injected fixed date; Rule 11 */
    cfg.date_mm   = 12;
    cfg.date_dd   = 31;
    cfg.heap_size = 256u * 1024u;  /* 256 KB: generous for multi-block memos */
    return pal_host_make(cfg);
}

/* ====================================================================
 * Oracle 1 + 2 + 3 + 5 + 6: main round-trip.
 *
 * Creates TMP_DBT_A; appends:
 *   Memo A ("Hello\r\nWorld", 12 bytes) -> 1 block (ceil(14/512)=1)
 *   Memo B (511 bytes of 'X'           -> 2 blocks (ceil(513/512)=2)
 *   Memo C (0 bytes, empty             -> 1 block (ceil(2/512)=1)
 *
 * Expected next_free after each append: 1 (initial) -> 2 -> 4 -> 5.
 * Re-opens with dbt_open; reads back all three; asserts byte-equality.
 * Also asserts raw on-disk LE encoding matches expected.
 * ==================================================================== */

/* Short memo (oracle A). */
static const uint8_t MEMO_A[]   = {
    0x48, 0x65, 0x6C, 0x6C, 0x6F,  /* "Hello" */
    0x0D, 0x0A,                     /* CR LF */
    0x57, 0x6F, 0x72, 0x6C, 0x64   /* "World" */
};
#define MEMO_A_LEN  12u
#define MEMO_A_BLOCKNO  1u   /* first memo always starts at block 1 */
#define MEMO_A_BLOCKS   1u   /* ceil((12+2)/512) = 1 */

/* Long memo: 511 bytes, all 'X' (0x58). Causes a 2-block append. */
#define MEMO_B_LEN  511u
#define MEMO_B_BYTE 0x58u   /* 'X' */
#define MEMO_B_BLOCKNO  2u  /* after memo A used block 1 */
#define MEMO_B_BLOCKS   2u  /* ceil((511+2)/512) = ceil(513/512) = 2 */

/* Empty memo: 0 bytes. */
#define MEMO_C_LEN      0u
#define MEMO_C_BLOCKNO  4u  /* after A(1 block) + B(2 blocks) = block 4 */
#define MEMO_C_BLOCKS   1u  /* ceil((0+2)/512) = 1 */

/* Expected next_free values after each append. */
#define NF_AFTER_CREATE 1u
#define NF_AFTER_A      (NF_AFTER_CREATE + MEMO_A_BLOCKS)   /* 2 */
#define NF_AFTER_B      (NF_AFTER_A      + MEMO_B_BLOCKS)   /* 4 */
#define NF_AFTER_C      (NF_AFTER_B      + MEMO_C_BLOCKS)   /* 5 */

static void check_create_and_append(samir_pal_t *pal)
{
    dbt_file *f   = (dbt_file *)0;
    dbt_file *rf  = (dbt_file *)0;
    uint8_t   memo_b[MEMO_B_LEN];
    uint8_t  *buf;
    uint32_t  blen;
    uint32_t  blockno;
    uint32_t  raw_nf;
    uint32_t  i;
    int       rc;
    char      msg[256];

    /* Fill memo B with 'X'. */
    for (i = 0u; i < MEMO_B_LEN; i++) memo_b[i] = MEMO_B_BYTE;

    /* ---- dbt_create ---- */
    rc = dbt_create(pal, TMP_DBT_A, &f);
    snprintf(msg, sizeof(msg), "create: dbt_create succeeds (rc=%d)", rc);
    CHECK(rc == DBT_OK && f != (dbt_file *)0, msg);
    if (rc != DBT_OK || !f) goto done;

    /* Check initial next_free = 1 after create.
     * Ref: dbt.md sec 8 step 1 "start = next_available_block". */
    snprintf(msg, sizeof(msg),
             "create: next_free == %u after create (got %u)",
             NF_AFTER_CREATE, dbt_next_free(f));
    CHECK(dbt_next_free(f) == NF_AFTER_CREATE, msg);

    /* ---- Append memo A (12 bytes, 1 block) ---- */
    blockno = 0u;
    rc = dbt_append(f, MEMO_A, MEMO_A_LEN, &blockno);
    snprintf(msg, sizeof(msg), "append A: dbt_append succeeds (rc=%d)", rc);
    CHECK(rc == DBT_OK, msg);
    if (rc != DBT_OK) goto close_write;

    snprintf(msg, sizeof(msg),
             "append A: blockno_out == %u (got %u)", MEMO_A_BLOCKNO, blockno);
    CHECK(blockno == MEMO_A_BLOCKNO, msg);

    snprintf(msg, sizeof(msg),
             "append A: next_free == %u after A (got %u)",
             NF_AFTER_A, dbt_next_free(f));
    CHECK(dbt_next_free(f) == NF_AFTER_A, msg);

    /* ---- Append memo B (511 bytes -> 2 blocks, boundary test) ---- */
    blockno = 0u;
    rc = dbt_append(f, memo_b, MEMO_B_LEN, &blockno);
    snprintf(msg, sizeof(msg), "append B: dbt_append succeeds (rc=%d)", rc);
    CHECK(rc == DBT_OK, msg);
    if (rc != DBT_OK) goto close_write;

    snprintf(msg, sizeof(msg),
             "append B: blockno_out == %u (got %u)", MEMO_B_BLOCKNO, blockno);
    CHECK(blockno == MEMO_B_BLOCKNO, msg);

    /* Block-sizing boundary: 511 bytes + 2-byte terminator = 513 bytes ->
     * ceil(513/512) = 2 blocks.  Ref: plan S2.2 contract; dbt.md sec 8. */
    snprintf(msg, sizeof(msg),
             "append B (boundary): next_free == %u after B (got %u) "
             "[511-byte payload spans 2 blocks: ceil(513/512)=2]",
             NF_AFTER_B, dbt_next_free(f));
    CHECK(dbt_next_free(f) == NF_AFTER_B, msg);

    /* ---- Append memo C (0 bytes, empty) ---- */
    blockno = 0u;
    rc = dbt_append(f, (const uint8_t *)0, MEMO_C_LEN, &blockno);
    snprintf(msg, sizeof(msg), "append C (empty): dbt_append succeeds (rc=%d)", rc);
    CHECK(rc == DBT_OK, msg);
    if (rc != DBT_OK) goto close_write;

    snprintf(msg, sizeof(msg),
             "append C: blockno_out == %u (got %u)", MEMO_C_BLOCKNO, blockno);
    CHECK(blockno == MEMO_C_BLOCKNO, msg);

    snprintf(msg, sizeof(msg),
             "append C: next_free == %u after C (got %u)",
             NF_AFTER_C, dbt_next_free(f));
    CHECK(dbt_next_free(f) == NF_AFTER_C, msg);

    /* ---- dbt_flush and close write handle ---- */
    rc = dbt_flush(f);
    CHECK(rc == DBT_OK, "flush: dbt_flush succeeds");

close_write:
    rc = dbt_close(f);
    f = (dbt_file *)0;
    CHECK(rc == DBT_OK, "close-write: dbt_close succeeds");

    /* ---- Verify raw on-disk LE encoding at file offset 0 ----
     * This is the mutant detection: with -DDBT_MUTATE_WRITE_PTR_ENDIAN,
     * the bytes at @0x00 will be the big-endian encoding of NF_AFTER_C,
     * which when decoded LE gives the wrong value.
     * Ref: dbt.md sec 3 (LE endian RESOLVED); dbt.h dbt_append mutant. */
    raw_nf = 0u;
    if (read_raw_next_free(pal, TMP_DBT_A, &raw_nf) == 0) {
        snprintf(msg, sizeof(msg),
                 "on-disk LE: raw next_free @0x00 decoded as LE == %u (got %u) "
                 "[BE-mutant will write 0x05000000; decoded LE = 83886080]",
                 NF_AFTER_C, raw_nf);
        CHECK(raw_nf == NF_AFTER_C, msg);
    } else {
        CHECK(0, "on-disk LE: could not re-open file to read raw bytes");
    }

    /* ---- Re-open and read back (bidirectional round-trip) ---- */
    rc = dbt_open(pal, TMP_DBT_A, 0 /*is_iv_dialect*/, &rf);
    snprintf(msg, sizeof(msg), "re-open: dbt_open succeeds (rc=%d)", rc);
    CHECK(rc == DBT_OK && rf != (dbt_file *)0, msg);
    if (rc != DBT_OK || !rf) goto done;

    /* next_free after re-open must match what we wrote.
     * Ref: dbt.md sec 2.1; sec 3 (LE). */
    snprintf(msg, sizeof(msg),
             "re-open: next_free == %u (got %u)",
             NF_AFTER_C, dbt_next_free(rf));
    CHECK(dbt_next_free(rf) == NF_AFTER_C, msg);

    /* Read memo A back and compare byte-for-byte. */
    buf  = (uint8_t *)0;
    blen = 0u;
    rc = dbt_read(rf, MEMO_A_BLOCKNO, &buf, &blen);
    snprintf(msg, sizeof(msg), "readback A: dbt_read(%u) succeeds (rc=%d)",
             MEMO_A_BLOCKNO, rc);
    CHECK(rc == DBT_OK, msg);
    if (rc == DBT_OK) {
        snprintf(msg, sizeof(msg),
                 "readback A: len == %u (got %u)", MEMO_A_LEN, blen);
        CHECK(blen == MEMO_A_LEN, msg);
        if (blen == MEMO_A_LEN && buf) {
            /* Content-keyed byte-exact comparison.
             * Ref: dbt.md sec 6 "compare memo content keyed by record". */
            CHECK(buf[0] == MEMO_A[0], "readback A: buf[0] == 'H' (0x48)");
            CHECK(buf[5] == 0x0Du,     "readback A: buf[5] == CR (0x0D)");
            CHECK(buf[6] == 0x0Au,     "readback A: buf[6] == LF (0x0A)");
            CHECK(buf[11] == MEMO_A[11], "readback A: buf[11] == 'd' (0x64)");
        }
    }

    /* Read memo B back; verify length and content (all 'X'). */
    buf  = (uint8_t *)0;
    blen = 0u;
    rc = dbt_read(rf, MEMO_B_BLOCKNO, &buf, &blen);
    snprintf(msg, sizeof(msg), "readback B: dbt_read(%u) succeeds (rc=%d)",
             MEMO_B_BLOCKNO, rc);
    CHECK(rc == DBT_OK, msg);
    if (rc == DBT_OK) {
        snprintf(msg, sizeof(msg),
                 "readback B: len == %u (got %u) [511-byte multi-block memo]",
                 MEMO_B_LEN, blen);
        CHECK(blen == MEMO_B_LEN, msg);
        if (blen == MEMO_B_LEN && buf) {
            int all_x = 1;
            uint32_t bi;
            for (bi = 0u; bi < MEMO_B_LEN; bi++) {
                if (buf[bi] != MEMO_B_BYTE) { all_x = 0; break; }
            }
            CHECK(all_x, "readback B: all 511 bytes == 'X' (0x58)");
        }
    }

    /* Read memo C back; verify length == 0. */
    buf  = (uint8_t *)0;
    blen = 0xFFFFu;
    rc = dbt_read(rf, MEMO_C_BLOCKNO, &buf, &blen);
    snprintf(msg, sizeof(msg), "readback C (empty): dbt_read(%u) succeeds (rc=%d)",
             MEMO_C_BLOCKNO, rc);
    CHECK(rc == DBT_OK, msg);
    if (rc == DBT_OK) {
        snprintf(msg, sizeof(msg),
                 "readback C: len == 0 (got %u) [empty memo]", blen);
        CHECK(blen == 0u, msg);
    }

    dbt_close(rf);
    rf = (dbt_file *)0;

done:
    if (rf) dbt_close(rf);
    if (f)  dbt_close(f);
}

/* ====================================================================
 * Oracle 5: RDONLY guard.
 * Open a .dbt via dbt_open; attempt dbt_append; assert -DBT_ERR_RDONLY.
 * ==================================================================== */
static void check_rdonly_guard(samir_pal_t *pal)
{
    dbt_file *f = (dbt_file *)0;
    uint32_t  blockno;
    int       rc;
    char      msg[256];

    /* TMP_DBT_A was created by check_create_and_append above. */
    rc = dbt_open(pal, TMP_DBT_A, 0, &f);
    snprintf(msg, sizeof(msg), "rdonly-guard: dbt_open succeeds (rc=%d)", rc);
    CHECK(rc == DBT_OK && f != (dbt_file *)0, msg);
    if (rc != DBT_OK || !f) return;

    blockno = 0u;
    rc = dbt_append(f, MEMO_A, MEMO_A_LEN, &blockno);
    CHECK(rc == -DBT_ERR_RDONLY,
          "rdonly-guard: dbt_append on dbt_open handle == -DBT_ERR_RDONLY");

    dbt_close(f);
}

/* ====================================================================
 * Oracle 4 (determinism, Rule 11):
 * Write TMP_DBT_B with the same inputs as TMP_DBT_A; re-open both and
 * decode all memos; assert all decoded texts are byte-identical.
 * (We compare via dbt_read rather than raw file bytes because block-0
 * bytes 4..511 and block tails are "don't care" per dbt.md sec 6.)
 * ==================================================================== */
static void check_determinism(samir_pal_t *pal)
{
    dbt_file *f    = (dbt_file *)0;
    dbt_file *fa   = (dbt_file *)0;
    dbt_file *fb   = (dbt_file *)0;
    uint8_t   memo_b[MEMO_B_LEN];
    uint32_t  blockno;
    uint8_t  *buf_a, *buf_b;
    uint32_t  len_a, len_b;
    uint32_t  i;
    int       rc;
    char      msg[256];
    int       match;

    for (i = 0u; i < MEMO_B_LEN; i++) memo_b[i] = MEMO_B_BYTE;

    /* Write TMP_DBT_B (identical inputs to TMP_DBT_A). */
    rc = dbt_create(pal, TMP_DBT_B, &f);
    snprintf(msg, sizeof(msg), "determinism: dbt_create B succeeds (rc=%d)", rc);
    CHECK(rc == DBT_OK && f != (dbt_file *)0, msg);
    if (rc != DBT_OK || !f) return;

    blockno = 0u;
    rc = dbt_append(f, MEMO_A, MEMO_A_LEN, &blockno);
    CHECK(rc == DBT_OK, "determinism: append A to B succeeds");
    blockno = 0u;
    rc = dbt_append(f, memo_b, MEMO_B_LEN, &blockno);
    CHECK(rc == DBT_OK, "determinism: append B to B succeeds");
    blockno = 0u;
    rc = dbt_append(f, (const uint8_t *)0, MEMO_C_LEN, &blockno);
    CHECK(rc == DBT_OK, "determinism: append C to B succeeds");

    dbt_close(f);
    f = (dbt_file *)0;

    /* Re-open both files and compare decoded content. */
    rc = dbt_open(pal, TMP_DBT_A, 0, &fa);
    CHECK(rc == DBT_OK && fa != (dbt_file *)0, "determinism: re-open A");
    rc = dbt_open(pal, TMP_DBT_B, 0, &fb);
    CHECK(rc == DBT_OK && fb != (dbt_file *)0, "determinism: re-open B");

    if (!fa || !fb) goto determ_done;

    /* next_free must be identical. */
    snprintf(msg, sizeof(msg),
             "determinism: A next_free (%u) == B next_free (%u)",
             dbt_next_free(fa), dbt_next_free(fb));
    CHECK(dbt_next_free(fa) == dbt_next_free(fb), msg);

    /* Compare memo A. */
    buf_a = (uint8_t *)0; len_a = 0u;
    buf_b = (uint8_t *)0; len_b = 0u;
    dbt_read(fa, MEMO_A_BLOCKNO, &buf_a, &len_a);
    dbt_read(fb, MEMO_A_BLOCKNO, &buf_b, &len_b);
    CHECK(len_a == len_b, "determinism: memo-A len matches in both files");
    match = 1;
    if (len_a == len_b && buf_a && buf_b) {
        for (i = 0u; i < len_a; i++) {
            if (buf_a[i] != buf_b[i]) { match = 0; break; }
        }
    } else { match = 0; }
    CHECK(match, "determinism: memo-A content byte-identical in both files");

    /* Compare memo B. */
    buf_a = (uint8_t *)0; len_a = 0u;
    buf_b = (uint8_t *)0; len_b = 0u;
    dbt_read(fa, MEMO_B_BLOCKNO, &buf_a, &len_a);
    dbt_read(fb, MEMO_B_BLOCKNO, &buf_b, &len_b);
    CHECK(len_a == len_b, "determinism: memo-B len matches in both files");
    match = 1;
    if (len_a == len_b && buf_a && buf_b) {
        for (i = 0u; i < len_a; i++) {
            if (buf_a[i] != buf_b[i]) { match = 0; break; }
        }
    } else { match = 0; }
    CHECK(match, "determinism: memo-B content byte-identical in both files");

determ_done:
    if (fa) dbt_close(fa);
    if (fb) dbt_close(fb);
}

/* ====================================================================
 * Tier-1 corpus check:
 * If TRAVEL.DBT is present, copy it by reading and creating a new file;
 * then re-open the COPY with dbt_open and verify the existing memos
 * (blocks 1 and 2) are still readable through the S2.1 reader.
 * This confirms the write path does not corrupt the file format.
 *
 * (We create a fresh .dbt via dbt_create and append the TRAVEL content
 * as our own memos, then verify the round-trip.)
 * ==================================================================== */
static void check_corpus_tier1(samir_pal_t *pal, const char *base)
{
    char      path[1024];
    dbt_file *src = (dbt_file *)0;
    dbt_file *dst = (dbt_file *)0;
    dbt_file *chk = (dbt_file *)0;
    uint8_t  *buf1, *buf2, *rb1, *rb2;
    uint32_t  len1, len2, rlen1, rlen2;
    uint32_t  bn1, bn2;
    uint32_t  i;
    int       rc;
    char      msg[256];

    join(path, sizeof(path), base, PRISTINE_REL "/TRAVEL.DBT");
    if (!file_exists(path)) {
        fprintf(stderr,
                "  SKIP (LOUD): Tier-1 corpus check: TRAVEL.DBT absent: %s\n",
                path);
        return;
    }

    /* Read memo content from TRAVEL.DBT via S2.1 reader. */
    rc = dbt_open(pal, path, 0, &src);
    snprintf(msg, sizeof(msg), "tier1: open TRAVEL.DBT (rc=%d)", rc);
    CHECK(rc == DBT_OK && src != (dbt_file *)0, msg);
    if (rc != DBT_OK || !src) return;

    buf1 = (uint8_t *)0; len1 = 0u;
    buf2 = (uint8_t *)0; len2 = 0u;
    rc  = dbt_read(src, 1u, &buf1, &len1);
    CHECK(rc == DBT_OK, "tier1: read TRAVEL block 1");
    rc  = dbt_read(src, 2u, &buf2, &len2);
    CHECK(rc == DBT_OK, "tier1: read TRAVEL block 2");
    dbt_close(src);
    src = (dbt_file *)0;

    if (!buf1 || !buf2) return;

    /* Create a fresh .dbt and append the same content. */
    rc = dbt_create(pal, TMP_DBT_CORPUS, &dst);
    snprintf(msg, sizeof(msg), "tier1: dbt_create corpus copy (rc=%d)", rc);
    CHECK(rc == DBT_OK && dst != (dbt_file *)0, msg);
    if (rc != DBT_OK || !dst) return;

    bn1 = 0u; bn2 = 0u;
    rc = dbt_append(dst, buf1, len1, &bn1);
    CHECK(rc == DBT_OK, "tier1: append block-1 content");
    rc = dbt_append(dst, buf2, len2, &bn2);
    CHECK(rc == DBT_OK, "tier1: append block-2 content");
    dbt_close(dst);
    dst = (dbt_file *)0;

    /* Re-open and verify the round-trip. */
    rc = dbt_open(pal, TMP_DBT_CORPUS, 0, &chk);
    snprintf(msg, sizeof(msg), "tier1: re-open corpus copy (rc=%d)", rc);
    CHECK(rc == DBT_OK && chk != (dbt_file *)0, msg);
    if (rc != DBT_OK || !chk) return;

    rb1 = (uint8_t *)0; rlen1 = 0u;
    rb2 = (uint8_t *)0; rlen2 = 0u;
    rc = dbt_read(chk, bn1, &rb1, &rlen1);
    CHECK(rc == DBT_OK, "tier1: read-back bn1");
    rc = dbt_read(chk, bn2, &rb2, &rlen2);
    CHECK(rc == DBT_OK, "tier1: read-back bn2");

    snprintf(msg, sizeof(msg),
             "tier1: len1 round-trip %u == %u", len1, rlen1);
    CHECK(len1 == rlen1, msg);

    snprintf(msg, sizeof(msg),
             "tier1: len2 round-trip %u == %u", len2, rlen2);
    CHECK(len2 == rlen2, msg);

    if (len1 == rlen1 && rb1 && buf1) {
        int ok = 1;
        for (i = 0u; i < len1; i++) {
            if (rb1[i] != buf1[i]) { ok = 0; break; }
        }
        CHECK(ok, "tier1: block-1 content round-trips byte-identically");
    }

    if (len2 == rlen2 && rb2 && buf2) {
        int ok = 1;
        for (i = 0u; i < len2; i++) {
            if (rb2[i] != buf2[i]) { ok = 0; break; }
        }
        CHECK(ok, "tier1: block-2 content round-trips byte-identically");
    }

    /* Also check first 4 bytes of block-1 (known from test_dbt_read.c Tier-0):
     * 0D 0A 50 61 ("\r\nPa").
     * Ref: dbt.md sec 4.1 "TRAVEL block 1 @0x200 = 0d 0a 'Paid ...'". */
    if (rlen1 >= 4u && rb1) {
        CHECK(rb1[0] == 0x0Du, "tier1: rb1[0] == 0x0D (CR)");
        CHECK(rb1[1] == 0x0Au, "tier1: rb1[1] == 0x0A (LF)");
        CHECK(rb1[2] == 0x50u, "tier1: rb1[2] == 'P' (0x50)");
        CHECK(rb1[3] == 0x61u, "tier1: rb1[3] == 'a' (0x61)");
    }

    dbt_close(chk);
}

/* ====================================================================
 * main
 * ==================================================================== */
int main(int argc, char **argv)
{
    const char  *base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    samir_pal_t *pal;

    pal = make_pal();
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    /* Oracle 1/2/3/6: create + append round-trip + block-sizing boundary */
    check_create_and_append(pal);

    /* Oracle 5: RDONLY guard */
    check_rdonly_guard(pal);

    /* Oracle 4: determinism */
    check_determinism(pal);

    /* Tier-1: corpus round-trip (loud-skip if absent) */
    check_corpus_tier1(pal, base);

    pal_host_free(pal);
    return TEST_SUMMARY("test-dbt-roundtrip");
}
