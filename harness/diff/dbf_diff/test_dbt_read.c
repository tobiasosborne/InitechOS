/*
 * harness/diff/dbf_diff/test_dbt_read.c -- host oracle for dbt_open + dbt_read
 *                                           (step S2.1).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc is OK here. Mirrors test_ndx_parse.c:
 * seed test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY), a host PAL
 * via pal_host_make (pal_host.c). Non-zero exit on any failed check keeps the
 * gate from false-greening (Law 2: the oracle is the truth).
 *
 * Two tiers (plan Sec 2.A):
 *   Tier 0 (committed, operator-free): expected memo content, block geometry,
 *     and terminator positions transcribed directly from dbt.md sec 4.1, sec 5,
 *     and the Verification section (all byte-verified 2026-06-16 against the
 *     golden fixtures). Each assertion is cited to the source.
 *   Tier 1 (golden-diff): opens TRAVEL.DBT and TOURS.DBT from the corpus;
 *     asserts the decoded text and block geometry match the Tier-0 manifest.
 *     Loud-skip if the golden is absent (never a silent pass).
 *
 * Mutation (Rule 6): built with -DDBT_MUTATE_BLOCKSIZE, dbt.c seeks blocks at
 * stride 511 instead of 512.  The seek to block 1 lands at offset 511 instead
 * of 512; the bytes pulled into the memo buffer are shifted by one relative to
 * the actual content.  The content-prefix check (first 4 bytes of the TRAVEL
 * block-1 memo = 0D 0A 50 61 "..Pa") goes RED because the mutant reads one byte
 * earlier (0x0D-shifted), and the length check also diverges.  Exit non-zero.
 *
 * Goldens base is argv[1] (default "../dbase3-decomp"). Goldens path:
 *   <base>/goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/
 *
 * Compile + run (self-grade; NOT make):
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *     -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/dbt.c os/samir/core/rt.c os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_dbt_read.c -o /tmp/test_dbt_read \
 *     && /tmp/test_dbt_read ../dbase3-decomp
 *
 * Mutant (must exit non-zero):
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *     -DDBT_MUTATE_BLOCKSIZE \
 *     -Iseed -Ios/samir/include -Ispec \
 *     os/samir/fs/dbt.c os/samir/core/rt.c os/samir/pal/pal_host.c \
 *     harness/diff/dbf_diff/test_dbt_read.c -o /tmp/test_dbt_read_mut \
 *     && /tmp/test_dbt_read_mut ../dbase3-decomp; echo "exit=$?"
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/dbt.md:
 *     sec 2   (block model: 512-byte blocks, block n at n*512)
 *     sec 2.1 (block 0: uint32 LE next-free @0x00; 508 = garbage)
 *     sec 3   (endianness RESOLVED: little-endian)
 *     sec 4   (M-field: 10-byte right-justified ASCII decimal; 0 = no memo)
 *     sec 4.1 (TRAVEL rec1 -> block 1 -> worked example, text content)
 *     sec 5   (terminator table: TRAVEL block-1 @block-rel 127,
 *              block-2 @block-rel 85; TOURS block-0 leftover @block-rel 103)
 *     Verification (byte-verified claims used for Tier-0 constants)
 *   - docs/plans/SAMIR-implementation-plan.md S2.1 oracle contract.
 *   - os/samir/include/samir/dbt.h (API under test).
 *   - seed/test_assert.h (harness idiom).
 */

#include <stdio.h>
#include <string.h>

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

/* ---- Constants ---- */
#define PRISTINE_REL \
    "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities"

/* Convenience: build a full path from base + relative path. */
static char *join(char *buf, size_t cap, const char *base, const char *rel)
{
    snprintf(buf, cap, "%s/%s", base, rel);
    return buf;
}

/* True if the file is readable. */
static int file_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

/* ====================================================================
 * Tier-0 manifest constants (operator-free; sourced from dbt.md)
 *
 * These values are byte-verified from the dbt.md Verification section
 * (2026-06-16) against TOURS.DBT and TRAVEL.DBT.  Every constant is cited
 * to the source below.
 * ==================================================================== */

/*
 * TRAVEL.DBT geometry (dbt.md sec 3 table, Verification claim 1):
 *   bytes @0x00: 03 00 00 00 -> LE = 3 -> next_free = 3.
 *   File size = 1111 bytes; blocks 0, 1, 2 are live; next free = 3. [verified]
 */
#define TRAVEL_NEXT_FREE  3u

/*
 * TRAVEL.DBT block-1 content length (dbt.md sec 5 / 4.1):
 *   Terminator 0x1A 0x1A at block-relative offset 127 means the memo content
 *   is bytes [0..126] of block 1 = 127 bytes.
 *   [verified: dbt.md sec 5 "TRAVEL block 1: block-rel 127"; sec 4.1 worked example]
 */
#define TRAVEL_BLOCK1_LEN 127u

/*
 * TRAVEL.DBT block-1 content prefix (dbt.md sec 4.1 worked example, Verification):
 *   The first 4 bytes of the TRAVEL block-1 memo text are 0D 0A 50 61 ("\r\nPa").
 *   [verified: dbt.md sec 4.1 "\r\nPaid on 7/15/85..."; Verification claim 5]
 *
 * Full text (dbt.md sec 4.1, Verification claim 5):
 *   "\r\nPaid on 7/15/85    Visa 9999-111-999-999\r\n\r\n
 *    Name on the Card: Claire M. Buckman\r\n\r\n
 *    Expiration Date: 5/86\r\n\r\nApproval: JK \r\n\r\n"
 */
#define TRAVEL_BLOCK1_B0  0x0Du   /* \r  (CR) */
#define TRAVEL_BLOCK1_B1  0x0Au   /* \n  (LF) */
#define TRAVEL_BLOCK1_B2  0x50u   /* 'P' */
#define TRAVEL_BLOCK1_B3  0x61u   /* 'a' */

/* Verify the text ends with the expected ASCII suffix (last 6 bytes):
 * "K \r\n\r\n" = 4B 20 0D 0A 0D 0A.
 * [verified: dbt.md sec 4.1 "Approval: JK \r\n\r\n"] */
#define TRAVEL_BLOCK1_TAIL_OFF  121u  /* offset within content = len-6 */
#define TRAVEL_BLOCK1_T0  0x4Bu  /* 'K' */
#define TRAVEL_BLOCK1_T1  0x20u  /* ' ' */
#define TRAVEL_BLOCK1_T2  0x0Du  /* \r */
#define TRAVEL_BLOCK1_T3  0x0Au  /* \n */
#define TRAVEL_BLOCK1_T4  0x0Du  /* \r */
#define TRAVEL_BLOCK1_T5  0x0Au  /* \n */

/*
 * TRAVEL.DBT block-2 content length (dbt.md sec 5, Verification):
 *   Terminator at block-relative offset 85 -> content = bytes [0..84] = 85 bytes.
 *   [verified: dbt.md sec 5 "TRAVEL block 2: block-rel 85"]
 */
#define TRAVEL_BLOCK2_LEN 85u

/*
 * TRAVEL.DBT block-2 first 4 bytes (Verification claim 3, byte dump):
 *   Block 2 starts at file offset 0x400.
 *   dbt.md Verification: "No III+ per-block header (III-vs-IV delta). TRAVEL
 *   block 2 @0x400 = 0d 0a 'Paymen...' -- raw text, byte 0."
 *   [verified: dbt.md Verification claim 3]
 */
#define TRAVEL_BLOCK2_B0  0x0Du   /* \r */
#define TRAVEL_BLOCK2_B1  0x0Au   /* \n */
#define TRAVEL_BLOCK2_B2  0x50u   /* 'P' */
#define TRAVEL_BLOCK2_B3  0x61u   /* 'a' */

/*
 * TRAVEL.DBT block-2 soft-break location (dbt.md sec 2.2, Verification claim 6):
 *   0x8D 0x0A at block-relative offset 57.
 *   [verified: dbt.md Verification claim 6 "TRAVEL block 2 contains ... 0d 0a
 *    (hard) and 8d 0a (soft, the char at block-rel 57)"]
 */
#define TRAVEL_BLOCK2_SOFTBREAK_OFF  57u
#define TRAVEL_BLOCK2_SOFTBREAK_B0   0x8Du   /* soft-wrap (0x0D | 0x80) */
#define TRAVEL_BLOCK2_SOFTBREAK_B1   0x0Au   /* LF */

/*
 * TOURS.DBT geometry (dbt.md sec 3 table, Verification claim 1):
 *   bytes @0x00: 01 00 00 00 -> LE = 1 -> next_free = 1.
 *   All 30 TOURS records have M-field pointer = "0" -> no live memos.
 *   File size = 513 bytes; only block 0 is live; next_free = 1. [verified]
 *
 * [verified: dbt.md sec 3 table "TOURS: 01 00 00 00 -> LE=1";
 *  Verification claim 7 "TOURS NOTE:M = '         0' for all 30 records"]
 */
#define TOURS_NEXT_FREE   1u

/* ====================================================================
 * check_travel_dbt: open TRAVEL.DBT and verify block geometry + content.
 * ==================================================================== */
static void check_travel_dbt(samir_pal_t *pal, const char *base)
{
    char      path[1024];
    char      msg[256];
    dbt_file *f = (dbt_file *)0;
    int       rc;
    uint8_t  *buf;
    uint32_t  blen;

    join(path, sizeof(path), base, PRISTINE_REL "/TRAVEL.DBT");
    if (!file_exists(path)) {
        fprintf(stderr,
                "  SKIP (LOUD): TRAVEL.DBT absent: %s\n"
                "               pass the corpus base as argv[1]\n", path);
        return;
    }

    /* --- dbt_open (not IV dialect) --- */
    rc = dbt_open(pal, path, 0 /*is_iv_dialect*/, &f);
    snprintf(msg, sizeof(msg), "TRAVEL: dbt_open succeeds (rc=%d)", rc);
    CHECK(rc == DBT_OK && f != (dbt_file *)0, msg);
    if (rc != DBT_OK || !f) return;

    /* --- block-0 next-free pointer ---
     * Expected: 3 (LE reading of 03 00 00 00).
     * Ref: dbt.md sec 3 table; Verification claim 1. */
    snprintf(msg, sizeof(msg),
             "TRAVEL: next_free == %u (got %u)",
             TRAVEL_NEXT_FREE, dbt_next_free(f));
    CHECK(dbt_next_free(f) == TRAVEL_NEXT_FREE, msg);

    /* --- Read block 1 ---
     * TRAVEL NOTES field of rec 1 = "         1" -> blockno = 1.
     * [verified: dbt.md sec 4.1 "TRAVEL.DBF record 1, NOTES bytes = ...31 ('1')"]
     * Expected text length: 127 bytes (terminator at block-rel 127).
     * Ref: dbt.md sec 5, Verification claim 4. */
    buf  = (uint8_t *)0;
    blen = 0u;
    rc = dbt_read(f, 1u, &buf, &blen);
    snprintf(msg, sizeof(msg), "TRAVEL block1: dbt_read(1) succeeds (rc=%d)", rc);
    CHECK(rc == DBT_OK, msg);
    if (rc != DBT_OK) goto close_travel;

    /* Length check: 127 bytes of memo text before the 0x1A 0x1A terminator.
     * Ref: dbt.md sec 5 "TRAVEL block 1: block-rel 127". */
    snprintf(msg, sizeof(msg),
             "TRAVEL block1: len == %u (got %u)", TRAVEL_BLOCK1_LEN, blen);
    CHECK(blen == TRAVEL_BLOCK1_LEN, msg);

    if (blen >= 4u && buf) {
        /* Content prefix: \r\n P a (0D 0A 50 61).
         * Ref: dbt.md sec 4.1 worked example; Verification claim 5. */
        snprintf(msg, sizeof(msg),
                 "TRAVEL block1: buf[0] == 0x0D (got 0x%02X)", (unsigned)buf[0]);
        CHECK(buf[0] == TRAVEL_BLOCK1_B0, msg);

        snprintf(msg, sizeof(msg),
                 "TRAVEL block1: buf[1] == 0x0A (got 0x%02X)", (unsigned)buf[1]);
        CHECK(buf[1] == TRAVEL_BLOCK1_B1, msg);

        snprintf(msg, sizeof(msg),
                 "TRAVEL block1: buf[2] == 'P' (got 0x%02X)", (unsigned)buf[2]);
        CHECK(buf[2] == TRAVEL_BLOCK1_B2, msg);

        snprintf(msg, sizeof(msg),
                 "TRAVEL block1: buf[3] == 'a' (got 0x%02X)", (unsigned)buf[3]);
        CHECK(buf[3] == TRAVEL_BLOCK1_B3, msg);
    }

    if (blen == TRAVEL_BLOCK1_LEN && buf) {
        /* Tail bytes (last 6): "K \r\n\r\n" = 4B 20 0D 0A 0D 0A.
         * Ref: dbt.md sec 4.1 "Approval: JK \r\n\r\n". */
        snprintf(msg, sizeof(msg),
                 "TRAVEL block1 tail[0] == 'K' (got 0x%02X)",
                 (unsigned)buf[TRAVEL_BLOCK1_TAIL_OFF + 0u]);
        CHECK(buf[TRAVEL_BLOCK1_TAIL_OFF + 0u] == TRAVEL_BLOCK1_T0, msg);

        snprintf(msg, sizeof(msg),
                 "TRAVEL block1 tail[1] == ' ' (got 0x%02X)",
                 (unsigned)buf[TRAVEL_BLOCK1_TAIL_OFF + 1u]);
        CHECK(buf[TRAVEL_BLOCK1_TAIL_OFF + 1u] == TRAVEL_BLOCK1_T1, msg);

        snprintf(msg, sizeof(msg),
                 "TRAVEL block1 tail[2] == 0x0D (got 0x%02X)",
                 (unsigned)buf[TRAVEL_BLOCK1_TAIL_OFF + 2u]);
        CHECK(buf[TRAVEL_BLOCK1_TAIL_OFF + 2u] == TRAVEL_BLOCK1_T2, msg);

        snprintf(msg, sizeof(msg),
                 "TRAVEL block1 tail[5] == 0x0A (got 0x%02X)",
                 (unsigned)buf[TRAVEL_BLOCK1_TAIL_OFF + 5u]);
        CHECK(buf[TRAVEL_BLOCK1_TAIL_OFF + 5u] == TRAVEL_BLOCK1_T5, msg);
    }

    /* --- Read block 2 ---
     * TRAVEL NOTES field of rec 2 = "         2" -> blockno = 2.
     * [verified: dbt.md sec 4.1 "rec2='2'"]
     * Expected length: 85 bytes; terminator at block-rel 85.
     * Ref: dbt.md sec 5; Verification claim 4. */
    buf  = (uint8_t *)0;
    blen = 0u;
    rc = dbt_read(f, 2u, &buf, &blen);
    snprintf(msg, sizeof(msg), "TRAVEL block2: dbt_read(2) succeeds (rc=%d)", rc);
    CHECK(rc == DBT_OK, msg);
    if (rc != DBT_OK) goto close_travel;

    /* Length check: 85 bytes.
     * Ref: dbt.md sec 5 "TRAVEL block 2: block-rel 85". */
    snprintf(msg, sizeof(msg),
             "TRAVEL block2: len == %u (got %u)", TRAVEL_BLOCK2_LEN, blen);
    CHECK(blen == TRAVEL_BLOCK2_LEN, msg);

    if (blen >= 4u && buf) {
        /* Prefix: \r\n P a (0D 0A 50 61).
         * Ref: dbt.md Verification claim 3 "TRAVEL block 2 @0x400 = 0d 0a 'Paymen...'". */
        snprintf(msg, sizeof(msg),
                 "TRAVEL block2: buf[0] == 0x0D (got 0x%02X)", (unsigned)buf[0]);
        CHECK(buf[0] == TRAVEL_BLOCK2_B0, msg);

        snprintf(msg, sizeof(msg),
                 "TRAVEL block2: buf[1] == 0x0A (got 0x%02X)", (unsigned)buf[1]);
        CHECK(buf[1] == TRAVEL_BLOCK2_B1, msg);

        snprintf(msg, sizeof(msg),
                 "TRAVEL block2: buf[2] == 'P' (got 0x%02X)", (unsigned)buf[2]);
        CHECK(buf[2] == TRAVEL_BLOCK2_B2, msg);

        snprintf(msg, sizeof(msg),
                 "TRAVEL block2: buf[3] == 'a' (got 0x%02X)", (unsigned)buf[3]);
        CHECK(buf[3] == TRAVEL_BLOCK2_B3, msg);
    }

    /* Soft-break (0x8D 0x0A) at block-relative offset 57.
     * Ref: dbt.md sec 2.2 (soft-wrap 0x8D), Verification claim 6.
     * [verified: python byte-check on TRAVEL.DBT confirms 0x8D 0x0A at
     *  block2-rel 57, file offset 0x400+57=0x439] */
    if (blen > TRAVEL_BLOCK2_SOFTBREAK_OFF + 1u && buf) {
        snprintf(msg, sizeof(msg),
                 "TRAVEL block2: soft-break buf[57] == 0x8D (got 0x%02X)",
                 (unsigned)buf[TRAVEL_BLOCK2_SOFTBREAK_OFF]);
        CHECK(buf[TRAVEL_BLOCK2_SOFTBREAK_OFF] == TRAVEL_BLOCK2_SOFTBREAK_B0,
              msg);

        snprintf(msg, sizeof(msg),
                 "TRAVEL block2: soft-break buf[58] == 0x0A (got 0x%02X)",
                 (unsigned)buf[TRAVEL_BLOCK2_SOFTBREAK_OFF + 1u]);
        CHECK(buf[TRAVEL_BLOCK2_SOFTBREAK_OFF + 1u] == TRAVEL_BLOCK2_SOFTBREAK_B1,
              msg);
    }

    /* --- Reject blockno 0 (no memo sentinel) ---
     * Ref: dbt.md sec 4 "block 0 or blank = no memo"; sec 2 "block 0 is header". */
    rc = dbt_read(f, 0u, &buf, &blen);
    CHECK(rc == -DBT_ERR_BAD_BLOCK,
          "TRAVEL: dbt_read(0) == -DBT_ERR_BAD_BLOCK (header block rejected)");

    /* --- Reject blockno >= next_free ---
     * next_free = 3; blockno 3 is one past the last live block. */
    rc = dbt_read(f, TRAVEL_NEXT_FREE, &buf, &blen);
    CHECK(rc == -DBT_ERR_BAD_BLOCK,
          "TRAVEL: dbt_read(next_free) == -DBT_ERR_BAD_BLOCK (out-of-range)");

close_travel:
    rc = dbt_close(f);
    CHECK(rc == DBT_OK, "TRAVEL: dbt_close succeeds");
}

/* ====================================================================
 * check_tours_dbt: open TOURS.DBT and verify geometry.
 *
 * TOURS.DBT has next_free=1 and NO live memo blocks (all 30 NOTES fields
 * point to block 0, meaning "no memo").  The file is 513 bytes: one full
 * 512-byte block plus a single stray 0x1A at offset 512.
 * ==================================================================== */
static void check_tours_dbt(samir_pal_t *pal, const char *base)
{
    char      path[1024];
    char      msg[256];
    dbt_file *f = (dbt_file *)0;
    uint8_t  *buf;
    uint32_t  blen;
    int       rc;

    join(path, sizeof(path), base, PRISTINE_REL "/TOURS.DBT");
    if (!file_exists(path)) {
        fprintf(stderr,
                "  SKIP (LOUD): TOURS.DBT absent: %s\n", path);
        return;
    }

    /* --- dbt_open --- */
    rc = dbt_open(pal, path, 0, &f);
    snprintf(msg, sizeof(msg), "TOURS: dbt_open succeeds (rc=%d)", rc);
    CHECK(rc == DBT_OK && f != (dbt_file *)0, msg);
    if (rc != DBT_OK || !f) return;

    /* --- next_free == 1 ---
     * Ref: dbt.md sec 3 "TOURS: 01 00 00 00 -> LE=1"; Verification claim 1. */
    snprintf(msg, sizeof(msg),
             "TOURS: next_free == %u (got %u)",
             TOURS_NEXT_FREE, dbt_next_free(f));
    CHECK(dbt_next_free(f) == TOURS_NEXT_FREE, msg);

    /* --- Any blockno >= next_free (=1) must be rejected ---
     * Since next_free=1, blockno 1 is already out of range.
     * (All records have "no memo" pointer -> caller never calls dbt_read.)
     * Ref: dbt.md sec 4 "block 0 = no memo"; S2.1 contract. */
    buf  = (uint8_t *)0;
    blen = 0u;
    rc = dbt_read(f, 1u, &buf, &blen);
    CHECK(rc == -DBT_ERR_BAD_BLOCK,
          "TOURS: dbt_read(1) == -DBT_ERR_BAD_BLOCK (next_free==1; 1 >= next_free)");

    rc = dbt_read(f, 0u, &buf, &blen);
    CHECK(rc == -DBT_ERR_BAD_BLOCK,
          "TOURS: dbt_read(0) == -DBT_ERR_BAD_BLOCK (header block)");

    rc = dbt_close(f);
    CHECK(rc == DBT_OK, "TOURS: dbt_close succeeds");
}

/* ====================================================================
 * check_iv_reject: dbt_open with is_iv_dialect=1 must return
 *                  -DBT_ERR_BAD_VERSION without touching the file.
 * (We reuse the TRAVEL path but pass is_iv_dialect=1; the file itself is
 * valid III+, but the caller asserts IV and we must reject fast.)
 * ==================================================================== */
static void check_iv_reject(samir_pal_t *pal, const char *base)
{
    char      path[1024];
    dbt_file *f = (dbt_file *)0;
    int       rc;

    join(path, sizeof(path), base, PRISTINE_REL "/TRAVEL.DBT");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): TRAVEL.DBT absent for IV-reject check\n");
        return;
    }

    rc = dbt_open(pal, path, 1 /*is_iv_dialect*/, &f);
    CHECK(rc == -DBT_ERR_BAD_VERSION,
          "IV-reject: dbt_open(is_iv_dialect=1) == -DBT_ERR_BAD_VERSION");
    CHECK(f == (dbt_file *)0,
          "IV-reject: *out is NULL on failure");
}

/* ====================================================================
 * main
 * ==================================================================== */
int main(int argc, char **argv)
{
    const char   *base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    struct pal_host_cfg cfg;
    samir_pal_t  *pal;
    char          path[1024];
    int           any_present = 0;

    /* Injected fixed date (Rule 11; today() not exercised in dbt_read). */
    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy  = 99;
    cfg.date_mm  = 12;
    cfg.date_dd  = 31;
    /* Arena: generous for the read buffers.
     * TRAVEL.DBT has two memos totalling ~212 bytes; the dbt_file struct is
     * small (~24 bytes).  Work buffers up to 512 bytes per dbt_read call.
     * 64 KB is ample for the whole test. */
    cfg.heap_size = 64u * 1024u;

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    /* Detect whether any goldens are present; print a loud skip if not. */
    join(path, sizeof(path), base, PRISTINE_REL "/TRAVEL.DBT");
    if (file_exists(path)) any_present = 1;
    join(path, sizeof(path), base, PRISTINE_REL "/TOURS.DBT");
    if (file_exists(path)) any_present = 1;

    if (!any_present) {
        fprintf(stderr,
                "  SKIP (LOUD): no goldens found under base '%s'\n"
                "               pass the corpus base as argv[1]\n"
                "               (Tier-0 constants cannot be exercised without "
                "the goldens)\n",
                base);
    }

    /* ---- Tier-0 + Tier-1 checks ---- */
    check_travel_dbt(pal, base);
    check_tours_dbt(pal, base);
    check_iv_reject(pal, base);

    pal_host_free(pal);
    return TEST_SUMMARY("test-dbt-read");
}
