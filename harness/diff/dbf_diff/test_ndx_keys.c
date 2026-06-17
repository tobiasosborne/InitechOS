/*
 * harness/diff/dbf_diff/test_ndx_keys.c -- host oracle for S4.2: ndx_key_decode
 *                                           + ndx_key_cmp (key decode + collation).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc is OK here. Mirrors test_ndx_parse.c:
 * test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY), pal_host_make,
 * loud-SKIP on absent goldens (printed, exit 0), non-zero exit on failure.
 *
 * Three tiers (plan Sec 2.A):
 *   Tier 0 (committed, operator-free): Tier-0 manifest values from ndx.md ss4.1,
 *     ss4.2, re/mint-results-001.md, and the Verification byte-dumps. These run
 *     even if the golden files are absent (uses the real golden through the PAL
 *     when present, loud-skips when absent).
 *   Tier 1 (golden-diff): corpus goldens for TOURDATE (date/JDN), CNAMES (char),
 *     NCOST (numeric with negatives; in mint/work). Verifies:
 *       - ndx_key_decode: correct xb_val type and value for each key_type.
 *       - ndx_key_cmp: in-order traversal of all leaf entries yields a non-
 *         decreasing sequence (ascending B-tree collation invariant).
 *
 * Mutation (Rule 6 / plan S4.2):
 *   Built with -DNDX_MUTATE_KEY_SIGNFLIP: the sign-flip transform in ndx_key_cmp
 *   inverts the comparison for negative doubles (XOR high bit of byte 7), causing
 *   the NCOST leaf ordering assertion (and the decoded-value assertion for -123.45)
 *   to go RED. Exit code non-zero -> the mutant gate passes.
 *   This is the canonical S4.2 mutation: it implements the hypothesis that
 *   re/mint-results-001.md was specifically minted to disprove.
 *
 * Goldens base is argv[1] (default "../dbase3-decomp").
 * NCOST.NDX is at <base>/mint/work/NCOST.NDX (minted 2026-06-16).
 * Pristine corpus goldens at:
 *   <base>/goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Compile (canonical self-verify recipe from the task brief):
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *     -Iseed -Ios/samir/include -Ispec \
 *     -o /tmp/test_ndx_keys \
 *     harness/diff/dbf_diff/test_ndx_keys.c \
 *     os/samir/fs/ndx.c os/samir/core/rt.c os/samir/core/value.c \
 *     os/samir/pal/pal_host.c
 *   /tmp/test_ndx_keys ../dbase3-decomp
 *
 * Mutant (must exit non-zero):
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *     -DNDX_MUTATE_KEY_SIGNFLIP \
 *     -Iseed -Ios/samir/include -Ispec \
 *     -o /tmp/test_ndx_keys_mut \
 *     harness/diff/dbf_diff/test_ndx_keys.c \
 *     os/samir/fs/ndx.c os/samir/core/rt.c os/samir/core/value.c \
 *     os/samir/pal/pal_host.c
 *   /tmp/test_ndx_keys_mut ../dbase3-decomp
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/ndx.md  ss4.1 (char), ss4.2 (numeric/
 *     date: raw LE double, NO sign-flip, arithmetic compare), ss6 (collation).
 *   - ../dbase3-decomp/re/mint-results-001.md -- THE authority for numeric key
 *     encoding. Table: -123.45->cd cc cc cc cc dc 5e c0; -1.0->00..f0 bf;
 *     0.0->all zeros; 279.0->00..70 71 40. [VERIFIED minted NCOST.NDX 2026-06-16]
 *   - docs/plans/SAMIR-implementation-plan.md S4.2 contract, Sec 3.3.
 *   - os/samir/include/samir/ndx.h (API under test).
 *   - os/samir/include/samir/value.h (xb_val, XB_C, XB_N).
 *   - os/samir/include/samir/rt.h (jdn_from_ymd for JDN assertions).
 *   - seed/test_assert.h (harness idiom: CHECK / TEST_HARNESS / TEST_SUMMARY).
 */

#include <stdio.h>
#include <string.h>

#include "test_assert.h"       /* seed/, on -Iseed */
#include "samir/ndx.h"         /* os/samir/include/, on -Ios/samir/include */
#include "samir/rt.h"          /* jdn_from_ymd */

TEST_HARNESS();

/* pal_host.c surface (not in a header; same pattern as test_ndx_parse.c). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* -----------------------------------------------------------------------
 * Path helpers (mirrored from test_ndx_parse.c)
 * ----------------------------------------------------------------------- */

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

/* -----------------------------------------------------------------------
 * Helper: in-order traverse, collect leaf key_data pointers into an array.
 *
 * Traverses the B-tree rooted at root_page; for each leaf entry copies
 * the raw key_data bytes (key_length bytes) into successive slots of
 * KEY_BUF (pre-allocated: max_entries * key_length bytes).
 *
 * Returns the number of leaf entries found, or -1 on ndx_read_node error.
 * Stops early if count reaches MAX_ENTRIES.
 *
 * Ref: ndx.md ss8 step 3 (in-order traversal).
 * ----------------------------------------------------------------------- */
static int collect_leaf_keys(ndx_index *idx, uint32_t page_no,
                             uint8_t *key_buf, int max_entries,
                             int *count)
{
    ndx_node_t *node = (ndx_node_t *)0;
    int         rc;
    uint32_t    i;
    uint32_t    kl = (uint32_t)ndx_key_length(idx);

    rc = ndx_read_node(idx, page_no, &node);
    if (rc != NDX_OK) return -1;

    for (i = 0u; i < (uint32_t)node->entry_count && *count < max_entries; i++) {
        if (node->entries[i].child_page != 0u) {
            /* Branch: recurse into child before emitting this separator.
             * Ref: ndx.md ss8 step 3 "if child_page != 0 recurse into it
             * before emitting the entry's key (branch)". */
            uint32_t child = node->entries[i].child_page;
            ndx_node_free(idx, node);
            node = (ndx_node_t *)0;
            rc = collect_leaf_keys(idx, child, key_buf, max_entries, count);
            if (rc < 0) return -1;
            /* Re-read current node (arena was freed for child). */
            rc = ndx_read_node(idx, page_no, &node);
            if (rc != NDX_OK) return -1;
        } else {
            /* Leaf entry: copy raw key bytes. */
            if (*count < max_entries) {
                rt_memcpy(key_buf + (uint32_t)(*count) * kl,
                          node->entries[i].key_data, kl);
                (*count)++;
            }
        }
    }
    /* Trailing (rightmost) child in branch nodes.
     * Ref: ndx.md ss8 step 3 "after the last entry, if internal, recurse
     * into the trailing child pointer". */
    if (node && node->trail_child != 0u) {
        uint32_t trail = node->trail_child;
        ndx_node_free(idx, node);
        node = (ndx_node_t *)0;
        rc = collect_leaf_keys(idx, trail, key_buf, max_entries, count);
        if (rc < 0) return -1;
    } else if (node) {
        ndx_node_free(idx, node);
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * check_decode_char: ndx_key_decode on a char key produces XB_C with
 * the raw bytes (space-padded CP437) in the caller buffer.
 *
 * Test case: CNAMES.NDX root page, first entry (key = "Collins ... Sara ...").
 * [ndx.md Verification "CNAMES root entry0 'Collins...Sara' == leaf page1 last
 * live key"; ndx.md ss4.1 "space-padded ASCII (OEM/CP437)"]
 * ----------------------------------------------------------------------- */
static void check_decode_char(samir_pal_t *pal, const char *base)
{
    char        path[1024];
    ndx_index  *idx  = (ndx_index *)0;
    ndx_node_t *node = (ndx_node_t *)0;
    int         rc;
    char        msg[256];
    char        keybuf[64];     /* key_length=40 for CNAMES; 64 is ample */
    xb_val      val;

    join(path, sizeof(path), base, PRISTINE_REL "/CNAMES.NDX");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): CNAMES.NDX absent (char decode check)\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0,
          "CNAMES decode: ndx_open");
    if (rc != NDX_OK || !idx) return;

    /* root_page = 6 per ndx.md ss2.2 CNAMES byte dump. */
    rc = ndx_read_node(idx, 6u, &node);
    CHECK(rc == NDX_OK && node != (ndx_node_t *)0,
          "CNAMES decode: ndx_read_node(6)");
    if (rc != NDX_OK || !node) { ndx_close(idx); return; }

    /* Decode first entry (branch; key_data is still valid per ss3.1). */
    CHECK(node->entry_count >= 1u, "CNAMES decode: entry_count >= 1");
    if (node->entry_count >= 1u) {
        memset(keybuf, 0, sizeof(keybuf));
        rc = ndx_key_decode(idx, node->entries[0].key_data, keybuf, &val);
        snprintf(msg, sizeof(msg),
                 "CNAMES decode: ndx_key_decode returns NDX_OK (rc=%d)", rc);
        CHECK(rc == NDX_OK, msg);

        /* Produces XB_C (key_type == 0).
         * Ref: ndx.h ndx_key_decode contract "char -> XB_C". */
        snprintf(msg, sizeof(msg),
                 "CNAMES decode: val.t == XB_C (got %d)", (int)val.t);
        CHECK(val.t == XB_C, msg);

        /* p points to keybuf; len = key_length = 40.
         * Ref: ndx.h "OUT->u.c.p is set to BUF ... len = ndx_key_length(idx)". */
        snprintf(msg, sizeof(msg),
                 "CNAMES decode: val.u.c.len == 40 (got %u)", val.u.c.len);
        CHECK(val.u.c.len == 40u, msg);

        CHECK(val.u.c.p == keybuf, "CNAMES decode: val.u.c.p == keybuf");

        /* First 7 bytes spell "Collins".
         * [ndx.md Verification "CNAMES root entry0 'Collins...Sara'"] */
        CHECK(keybuf[0] == 'C', "CNAMES decode: keybuf[0]=='C'");
        CHECK(keybuf[1] == 'o', "CNAMES decode: keybuf[1]=='o'");
        CHECK(keybuf[2] == 'l', "CNAMES decode: keybuf[2]=='l'");
        CHECK(keybuf[3] == 'l', "CNAMES decode: keybuf[3]=='l'");
        CHECK(keybuf[4] == 'i', "CNAMES decode: keybuf[4]=='i'");
        CHECK(keybuf[5] == 'n', "CNAMES decode: keybuf[5]=='n'");
        CHECK(keybuf[6] == 's', "CNAMES decode: keybuf[6]=='s'");
    }

    ndx_node_free(idx, node);
    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * check_decode_date: ndx_key_decode on TOURDATE (key_type=1, D field).
 *
 * First leaf entry: DEPARTURE = 1985-08-05, JDN = 2446283.
 * Key bytes: 00 00 00 80 E5 A9 42 41.
 * Decoded double must equal 2446283.0.
 * [ndx.md ss4.2 / re/mint-results-001.md "Date keys = JDN as 8-byte double";
 *  Verification "19850805->JDN 2446283->'00 00 00 80 E5 A9 42 41' (match)"]
 *
 * The result is XB_N (not XB_D): the N-vs-D distinction is NOT in the .ndx
 * header (ndx.h S4.2->S5 boundary note). The caller checks that the double
 * matches the expected JDN value.
 * ----------------------------------------------------------------------- */
static void check_decode_date(samir_pal_t *pal, const char *base)
{
    char        path[1024];
    ndx_index  *idx  = (ndx_index *)0;
    ndx_node_t *node = (ndx_node_t *)0;
    int         rc;
    char        msg[256];
    xb_val      val;
    /* JDN(1985-08-05) = 2446283 [ndx.md ss4.2 / rt.h jdn_from_ymd verified]. */
    int32_t     expected_jdn;
    double      expected_dbl;

    join(path, sizeof(path), base, PRISTINE_REL "/TOURDATE.NDX");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): TOURDATE.NDX absent (date decode check)\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0,
          "TOURDATE decode: ndx_open");
    if (rc != NDX_OK || !idx) return;

    /* Compute expected JDN using rt.h jdn_from_ymd (same formula as rt.c).
     * [rt.h jdn_from_ymd: verified against minted TOURDATE.NDX 2446283 = 1985-08-05] */
    expected_jdn = jdn_from_ymd(1985, 8, 5);
    expected_dbl = (double)expected_jdn;
    snprintf(msg, sizeof(msg),
             "TOURDATE decode: jdn_from_ymd(1985,8,5)==2446283 (got %d)",
             expected_jdn);
    CHECK(expected_jdn == 2446283, msg);

    /* root_page=1 [ndx.md Verification "TOURDATE root=1, total=2"]. */
    rc = ndx_read_node(idx, 1u, &node);
    CHECK(rc == NDX_OK && node != (ndx_node_t *)0,
          "TOURDATE decode: ndx_read_node(1)");
    if (rc != NDX_OK || !node) { ndx_close(idx); return; }

    CHECK(node->entry_count == 30u, "TOURDATE decode: entry_count==30");
    if (node->entry_count >= 1u) {
        rc = ndx_key_decode(idx, node->entries[0].key_data, (char *)0, &val);
        snprintf(msg, sizeof(msg),
                 "TOURDATE decode: ndx_key_decode rc==NDX_OK (got %d)", rc);
        CHECK(rc == NDX_OK, msg);

        /* Produces XB_N (key_type==1; N-vs-D boundary: always XB_N here).
         * Ref: ndx.h S4.2->S5 boundary note; plan Sec 3.3. */
        snprintf(msg, sizeof(msg),
                 "TOURDATE decode: val.t == XB_N (got %d)", (int)val.t);
        CHECK(val.t == XB_N, msg);

        /* Double value must equal JDN 2446283.0.
         * [ndx.md ss4.2 "TOURDATE leaf entry 0 key_data = 00 00 00 80 E5 A9 42 41"
         *  = JDN 2446283.0. Ref: Verification "19850805->2446283->00 00 00 80 E5 A9
         *  42 41 (match)".] */
        snprintf(msg, sizeof(msg),
                 "TOURDATE decode: val.u.n == 2446283.0 (got %.1f)", val.u.n);
        CHECK(val.u.n == expected_dbl, msg);

        /* Additional check: second entry JDN 2446293 = 1985-08-15.
         * [ndx.md Verification "19850907->2446316->00 00 00 00 F6 A9 42 41"
         *  is entry[3]; entry[1] is 1985-08-15 = JDN 2446293 from Python dump.] */
        {
            xb_val val2;
            int32_t jdn2 = jdn_from_ymd(1985, 8, 15);
            rc = ndx_key_decode(idx, node->entries[1].key_data, (char *)0, &val2);
            CHECK(rc == NDX_OK, "TOURDATE decode entry[1]: rc==NDX_OK");
            CHECK(val2.t == XB_N, "TOURDATE decode entry[1]: XB_N");
            snprintf(msg, sizeof(msg),
                     "TOURDATE decode entry[1]: double==%.1f (JDN 1985-08-15, got %.1f)",
                     (double)jdn2, val2.u.n);
            CHECK(val2.u.n == (double)jdn2, msg);
        }
    }

    ndx_node_free(idx, node);
    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * check_decode_numeric: ndx_key_decode on NCOST.NDX (numeric, with negatives).
 *
 * NCOST.NDX was minted 2026-06-16 by re/mint-results-001.md to settle the
 * sign-flip question. It contains negatives and decimals: -123.45, -1.0, 0.0,
 * 279.0 (and originals from NTEST.DBF).
 *
 * Canonical bytes from re/mint-results-001.md:
 *   -123.45  cd cc cc cc cc dc 5e c0
 *   -1.0     00 00 00 00 00 00 f0 bf
 *   0.0      00 00 00 00 00 00 00 00
 *   279.0    00 00 00 00 00 70 71 40
 *
 * [re/mint-results-001.md "Numeric keys are stored as raw little-endian
 *  IEEE-754 doubles -- NOT a sign-flipped / order-preserving bit transform."
 *  VERIFIED minted NCOST.NDX 2026-06-16]
 * ----------------------------------------------------------------------- */
static void check_decode_numeric(samir_pal_t *pal, const char *base)
{
    char        path[1024];
    ndx_index  *idx  = (ndx_index *)0;
    ndx_node_t *node = (ndx_node_t *)0;
    int         rc;
    char        msg[256];
    xb_val      val;
    /* Tier-0 manifest from re/mint-results-001.md. */
    static const uint8_t bytes_neg123[8] = {
        0xCDu, 0xCCu, 0xCCu, 0xCCu, 0xCCu, 0xDCu, 0x5Eu, 0xC0u
    };
    static const uint8_t bytes_neg1[8] = {
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xF0u, 0xBFu
    };
    static const uint8_t bytes_zero[8] = {
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u
    };
    static const uint8_t bytes_279[8] = {
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x70u, 0x71u, 0x40u
    };

    /* NCOST.NDX is in mint/work, not in the pristine corpus.
     * [re/mint-results-001.md: "Minted NCOST.NDX = INDEX ON UNITCOST"] */
    join(path, sizeof(path), base, "mint/work/NCOST.NDX");
    if (!file_exists(path)) {
        fprintf(stderr,
                "  SKIP (LOUD): mint/work/NCOST.NDX absent -- "
                "pass corpus base as argv[1]\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0,
          "NCOST decode: ndx_open");
    if (rc != NDX_OK || !idx) return;

    /* key_type must be 1 (numeric). */
    snprintf(msg, sizeof(msg),
             "NCOST decode: key_type==1 (got %u)", ndx_key_type(idx));
    CHECK(ndx_key_type(idx) == 1u, msg);
    /* key_length must be 8 (IEEE double).
     * [ndx.md ss4.2 "key_length==8, group_length==16"] */
    snprintf(msg, sizeof(msg),
             "NCOST decode: key_length==8 (got %u)", ndx_key_length(idx));
    CHECK(ndx_key_length(idx) == 8u, msg);

    /* Tier-0: decode the four canonical byte patterns directly (no file read
     * needed -- validates the decode logic independently of file I/O). */
    {
        /* -123.45: cd cc cc cc cc dc 5e c0
         * [re/mint-results-001.md table; VERIFIED minted NCOST.NDX 2026-06-16] */
        rc = ndx_key_decode(idx, bytes_neg123, (char *)0, &val);
        CHECK(rc == NDX_OK, "NCOST Tier-0: ndx_key_decode(-123.45) rc==NDX_OK");
        CHECK(val.t == XB_N, "NCOST Tier-0: -123.45 -> XB_N");
        snprintf(msg, sizeof(msg),
                 "NCOST Tier-0: -123.45 decoded correctly (got %.6f)", val.u.n);
        /* Allow exact double equality: the LE bytes directly represent the double. */
        CHECK(val.u.n == -123.45, msg);

        /* -1.0: 00 00 00 00 00 00 f0 bf */
        rc = ndx_key_decode(idx, bytes_neg1, (char *)0, &val);
        CHECK(rc == NDX_OK, "NCOST Tier-0: ndx_key_decode(-1.0) rc==NDX_OK");
        snprintf(msg, sizeof(msg),
                 "NCOST Tier-0: -1.0 decoded correctly (got %.6f)", val.u.n);
        CHECK(val.u.n == -1.0, msg);

        /* 0.0: all zeros */
        rc = ndx_key_decode(idx, bytes_zero, (char *)0, &val);
        CHECK(rc == NDX_OK, "NCOST Tier-0: ndx_key_decode(0.0) rc==NDX_OK");
        snprintf(msg, sizeof(msg),
                 "NCOST Tier-0: 0.0 decoded correctly (got %.6f)", val.u.n);
        CHECK(val.u.n == 0.0, msg);

        /* 279.0: 00 00 00 00 00 70 71 40 */
        rc = ndx_key_decode(idx, bytes_279, (char *)0, &val);
        CHECK(rc == NDX_OK, "NCOST Tier-0: ndx_key_decode(279.0) rc==NDX_OK");
        snprintf(msg, sizeof(msg),
                 "NCOST Tier-0: 279.0 decoded correctly (got %.6f)", val.u.n);
        CHECK(val.u.n == 279.0, msg);
    }

    /* Tier-0 collation: ndx_key_cmp on the four canonical patterns.
     * True numeric order: -123.45 < -1.0 < 0.0 < 279.0.
     * [re/mint-results-001.md "dBASE compares numeric keys ARITHMETICALLY...
     *  the leaf is in true numeric order (-123.45 < -1 < 0 < 279) even though
     *  raw IEEE doubles do not byte-sort that way." VERIFIED minted NCOST.NDX] */
    {
        int cmp;
        cmp = ndx_key_cmp(idx, bytes_neg123, bytes_neg1);
        snprintf(msg, sizeof(msg),
                 "NCOST Tier-0 cmp: -123.45 < -1.0 (got %d, want <0)", cmp);
        CHECK(cmp < 0, msg);

        cmp = ndx_key_cmp(idx, bytes_neg1, bytes_zero);
        snprintf(msg, sizeof(msg),
                 "NCOST Tier-0 cmp: -1.0 < 0.0 (got %d, want <0)", cmp);
        CHECK(cmp < 0, msg);

        cmp = ndx_key_cmp(idx, bytes_zero, bytes_279);
        snprintf(msg, sizeof(msg),
                 "NCOST Tier-0 cmp: 0.0 < 279.0 (got %d, want <0)", cmp);
        CHECK(cmp < 0, msg);

        cmp = ndx_key_cmp(idx, bytes_neg123, bytes_neg123);
        snprintf(msg, sizeof(msg),
                 "NCOST Tier-0 cmp: -123.45 == -123.45 (got %d, want 0)", cmp);
        CHECK(cmp == 0, msg);

        cmp = ndx_key_cmp(idx, bytes_279, bytes_neg123);
        snprintf(msg, sizeof(msg),
                 "NCOST Tier-0 cmp: 279.0 > -123.45 (got %d, want >0)", cmp);
        CHECK(cmp > 0, msg);
    }

    /* Tier-1: read page 1 (first leaf) of NCOST and verify the first three
     * entries are in ascending numeric order. Python dump confirmed:
     *   entry[0] recno=31 double=-123.45  bytes: cd cc cc cc cc dc 5e c0
     *   entry[1] recno=32 double=-1.0     bytes: 00 00 00 00 00 00 f0 bf
     *   entry[2] recno=33 double=0.0      bytes: 00 00 00 00 00 00 00 00
     *   entry[3] recno=2  double=279.0    bytes: 00 00 00 00 00 70 71 40
     * [Python dump verified against re/mint-results-001.md table.] */
    {
        /* NCOST root_page = 3 (branch); leaf page 1 contains the negatives. */
        rc = ndx_read_node(idx, 1u, &node);
        if (rc != NDX_OK) {
            fprintf(stderr,
                    "  SKIP (LOUD): NCOST.NDX page 1 read failed (rc=%d)\n", rc);
            ndx_close(idx);
            return;
        }
        CHECK(rc == NDX_OK && node != (ndx_node_t *)0, "NCOST Tier-1: read page 1");

        snprintf(msg, sizeof(msg),
                 "NCOST Tier-1: page 1 entry_count >= 4 (got %u)",
                 node->entry_count);
        CHECK(node->entry_count >= 4u, msg);

        if (node->entry_count >= 4u) {
            xb_val v0, v1, v2, v3;
            int c01, c12, c23;

            /* Decode first 4 entries. */
            rc  = ndx_key_decode(idx, node->entries[0].key_data, (char *)0, &v0);
            rc |= ndx_key_decode(idx, node->entries[1].key_data, (char *)0, &v1);
            rc |= ndx_key_decode(idx, node->entries[2].key_data, (char *)0, &v2);
            rc |= ndx_key_decode(idx, node->entries[3].key_data, (char *)0, &v3);
            CHECK(rc == NDX_OK, "NCOST Tier-1: decode first 4 entries");

            /* Values must match the minted table.
             * [re/mint-results-001.md: -123.45; -1.0; 0.0; 279.0] */
            snprintf(msg, sizeof(msg),
                     "NCOST Tier-1: entry[0]==-123.45 (got %.6f)", v0.u.n);
            CHECK(v0.u.n == -123.45, msg);
            snprintf(msg, sizeof(msg),
                     "NCOST Tier-1: entry[1]==-1.0 (got %.6f)", v1.u.n);
            CHECK(v1.u.n == -1.0, msg);
            snprintf(msg, sizeof(msg),
                     "NCOST Tier-1: entry[2]==0.0 (got %.6f)", v2.u.n);
            CHECK(v2.u.n == 0.0, msg);
            snprintf(msg, sizeof(msg),
                     "NCOST Tier-1: entry[3]==279.0 (got %.6f)", v3.u.n);
            CHECK(v3.u.n == 279.0, msg);

            /* ndx_key_cmp must agree with arithmetic order.
             * This is the PRIMARY mutant detector: under NDX_MUTATE_KEY_SIGNFLIP
             * the sign-flip inverts comparison for negatives -> c01/c12 go wrong. */
            c01 = ndx_key_cmp(idx,
                               node->entries[0].key_data,
                               node->entries[1].key_data);
            c12 = ndx_key_cmp(idx,
                               node->entries[1].key_data,
                               node->entries[2].key_data);
            c23 = ndx_key_cmp(idx,
                               node->entries[2].key_data,
                               node->entries[3].key_data);
            snprintf(msg, sizeof(msg),
                     "NCOST Tier-1: entry[0] < entry[1] via ndx_key_cmp (got %d)", c01);
            CHECK(c01 < 0, msg);
            snprintf(msg, sizeof(msg),
                     "NCOST Tier-1: entry[1] < entry[2] via ndx_key_cmp (got %d)", c12);
            CHECK(c12 < 0, msg);
            snprintf(msg, sizeof(msg),
                     "NCOST Tier-1: entry[2] < entry[3] via ndx_key_cmp (got %d)", c23);
            CHECK(c23 < 0, msg);
        }

        ndx_node_free(idx, node);
    }

    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * check_cmp_char: ndx_key_cmp on CNAMES.NDX char keys.
 *
 * Full in-order traversal must yield a non-decreasing sequence.
 * [ndx.md Verification: "full in-order traversal of CNAMES yields 49 leaf
 *  recnos (1..49) in sorted key order, first 'Adams Nathan', last 'Zambini
 *  Rick'". ndx.md ss6 "unsigned byte value (ASCII/OEM CP437)."]
 * ----------------------------------------------------------------------- */
#define MAX_CNAMES_ENTRIES 256

static void check_cmp_char(samir_pal_t *pal, const char *base)
{
    char        path[1024];
    ndx_index  *idx  = (ndx_index *)0;
    int         rc;
    char        msg[256];
    int         count = 0;
    uint32_t    kl;
    /* 256 entries * 40 bytes each = 10 KB; on the stack (host test). */
    uint8_t     keys[MAX_CNAMES_ENTRIES * 40];
    int         i;

    join(path, sizeof(path), base, PRISTINE_REL "/CNAMES.NDX");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): CNAMES.NDX absent (char cmp check)\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0, "CNAMES cmp: ndx_open");
    if (rc != NDX_OK || !idx) return;

    kl = (uint32_t)ndx_key_length(idx);
    CHECK(kl == 40u, "CNAMES cmp: key_length==40");

    rc = collect_leaf_keys(idx, ndx_root_page(idx), keys,
                           MAX_CNAMES_ENTRIES, &count);
    CHECK(rc == 0, "CNAMES cmp: collect_leaf_keys succeeded");

    snprintf(msg, sizeof(msg),
             "CNAMES cmp: 49 leaf entries (got %d)", count);
    CHECK(count == 49, msg);

    /* Verify ascending order via ndx_key_cmp.
     * [ndx.md Verification "full in-order traversal yields 49 leaf entries in
     *  sorted key order". ndx.md ss6 "unsigned byte value, left-to-right".] */
    for (i = 0; i + 1 < count; i++) {
        int cmp = ndx_key_cmp(idx,
                               keys + (uint32_t)i * kl,
                               keys + ((uint32_t)i + 1u) * kl);
        if (cmp > 0) {
            snprintf(msg, sizeof(msg),
                     "CNAMES cmp: entry[%d] <= entry[%d] (ndx_key_cmp=%d, UNSORTED)",
                     i, i + 1, cmp);
            CHECK(0, msg);   /* Force failure to report the pair. */
        }
    }

    /* Spot-check: "Adams" (first) < "Zambini" (last) in byte order. */
    if (count >= 49) {
        int cmp = ndx_key_cmp(idx, keys, keys + 48u * kl);
        snprintf(msg, sizeof(msg),
                 "CNAMES cmp: Adams < Zambini (ndx_key_cmp=%d, want <0)", cmp);
        CHECK(cmp < 0, msg);
    }

    /* Spot-check: "DeBello" < "Dean" (verifies unsigned byte order: 'B'(0x42)
     * < 'a'(0x61)). We search the collected keys for each prefix.
     * [ndx.md ss6 Verification "DeBello(0x42) sorts before Dean(0x61)"] */
    {
        int debello_i = -1, dean_i = -1;
        for (i = 0; i < count; i++) {
            const uint8_t *k = keys + (uint32_t)i * kl;
            /* "DeBello" starts with "DeBello" (7 chars). */
            if (k[0]=='D' && k[1]=='e' && k[2]=='B')
                debello_i = i;
            /* "Dean" starts with "Dean". */
            if (k[0]=='D' && k[1]=='e' && k[2]=='a')
                dean_i = i;
        }
        if (debello_i >= 0 && dean_i >= 0) {
            int cmp = ndx_key_cmp(idx,
                                   keys + (uint32_t)debello_i * kl,
                                   keys + (uint32_t)dean_i * kl);
            snprintf(msg, sizeof(msg),
                     "CNAMES cmp: DeBello < Dean (unsigned byte order; "
                     "ndx_key_cmp=%d, want <0)", cmp);
            CHECK(cmp < 0, msg);
        } else {
            fprintf(stderr,
                    "  INFO: DeBello (%d) or Dean (%d) not found in CNAMES "
                    "(may not be in this fixture version)\n",
                    debello_i, dean_i);
        }
    }

    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * check_cmp_date: ndx_key_cmp on TOURDATE leaf (30 entries, type-1).
 *
 * Full leaf traversal must be non-decreasing (ascending JDN order).
 * [ndx.md Verification "TOURDATE JDNs monotonic non-decreasing". ndx.md ss5
 *  "keys are stored in ascending order by the collation in section 6".]
 * ----------------------------------------------------------------------- */
#define MAX_TOURDATE_ENTRIES 64

static void check_cmp_date(samir_pal_t *pal, const char *base)
{
    char        path[1024];
    ndx_index  *idx  = (ndx_index *)0;
    int         rc;
    char        msg[256];
    int         count = 0;
    uint8_t     keys[MAX_TOURDATE_ENTRIES * 8];  /* key_length == 8 */
    int         i;

    join(path, sizeof(path), base, PRISTINE_REL "/TOURDATE.NDX");
    if (!file_exists(path)) {
        fprintf(stderr,
                "  SKIP (LOUD): TOURDATE.NDX absent (date cmp check)\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0, "TOURDATE cmp: ndx_open");
    if (rc != NDX_OK || !idx) return;

    rc = collect_leaf_keys(idx, ndx_root_page(idx), keys,
                           MAX_TOURDATE_ENTRIES, &count);
    CHECK(rc == 0, "TOURDATE cmp: collect_leaf_keys succeeded");

    snprintf(msg, sizeof(msg),
             "TOURDATE cmp: 30 leaf entries (got %d)", count);
    CHECK(count == 30, msg);

    /* Non-decreasing order (duplicates are allowed: ndx.md ss5).
     * [ndx.md Verification "recnos 9,10 both JDN 2446363 -- equal dates."] */
    for (i = 0; i + 1 < count; i++) {
        int cmp = ndx_key_cmp(idx,
                               keys + (uint32_t)i * 8u,
                               keys + ((uint32_t)i + 1u) * 8u);
        if (cmp > 0) {
            snprintf(msg, sizeof(msg),
                     "TOURDATE cmp: entry[%d] <= entry[%d] "
                     "(ndx_key_cmp=%d, UNSORTED)", i, i + 1, cmp);
            CHECK(0, msg);
        }
    }

    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * check_cmp_numeric_leaf: full NCOST leaf ordering via ndx_key_cmp.
 *
 * Leaf page 1 of NCOST (31 entries). All must be non-decreasing.
 * This is the comprehensive numeric ordering check (Tier-1).
 * ----------------------------------------------------------------------- */
#define MAX_NCOST_LEAF_ENTRIES 64

static void check_cmp_numeric_leaf(samir_pal_t *pal, const char *base)
{
    char        path[1024];
    ndx_index  *idx  = (ndx_index *)0;
    ndx_node_t *node = (ndx_node_t *)0;
    int         rc;
    char        msg[256];
    int         i;

    join(path, sizeof(path), base, "mint/work/NCOST.NDX");
    if (!file_exists(path)) {
        fprintf(stderr,
                "  SKIP (LOUD): mint/work/NCOST.NDX absent "
                "(numeric leaf ordering check)\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0,
          "NCOST leaf cmp: ndx_open");
    if (rc != NDX_OK || !idx) return;

    /* Page 1 is the first leaf (confirmed by Python dump: 31 entries, child=0). */
    rc = ndx_read_node(idx, 1u, &node);
    CHECK(rc == NDX_OK && node != (ndx_node_t *)0,
          "NCOST leaf cmp: ndx_read_node(1)");
    if (rc != NDX_OK || !node) { ndx_close(idx); return; }

    snprintf(msg, sizeof(msg),
             "NCOST leaf cmp: page 1 entry_count >= 2 (got %u)",
             node->entry_count);
    CHECK(node->entry_count >= 2u, msg);

    /* Non-decreasing check over all live entries.
     * Under NDX_MUTATE_KEY_SIGNFLIP this will fail for the negative entries:
     * entries[0]=-123.45 and entries[1]=-1.0 would compare reversed -> RED.
     * [re/mint-results-001.md VERIFIED: -123.45 < -1 < 0 < 279 in NCOST leaf.] */
    for (i = 0; i + 1 < (int)node->entry_count; i++) {
        int cmp = ndx_key_cmp(idx,
                               node->entries[i].key_data,
                               node->entries[i + 1].key_data);
        if (cmp > 0) {
            snprintf(msg, sizeof(msg),
                     "NCOST leaf cmp: entry[%d] <= entry[%d] "
                     "(ndx_key_cmp=%d, UNSORTED -- mutant?)", i, i + 1, cmp);
            CHECK(0, msg);
        }
    }

    ndx_node_free(idx, node);
    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    const char *base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    struct pal_host_cfg cfg;
    samir_pal_t *pal;
    char path[1024];
    int any_present = 0;

    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy  = 99;
    cfg.date_mm  = 12;
    cfg.date_dd  = 31;
    /* Arena: collect_leaf_keys holds one node at a time plus ndx_index.
     * CNAMES: ndx_index~200B + node for KL=40/KPP=10 ~800B. Keys array is on
     * the C stack (host test). NCOST: similar. 128 KB is ample. */
    cfg.heap_size = 128u * 1024u;

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    /* Check whether any goldens are present. */
    if (file_exists(join(path, sizeof(path), base,
                         PRISTINE_REL "/TOURDATE.NDX")))
        any_present = 1;
    if (file_exists(join(path, sizeof(path), base,
                         "mint/work/NCOST.NDX")))
        any_present = 1;

    if (!any_present) {
        fprintf(stderr,
                "  SKIP (LOUD): no goldens found under base '%s'\n"
                "               pass the corpus base as argv[1]\n",
                base);
    }

    /* ---- S4.2 decode checks ---- */
    check_decode_char(pal, base);
    check_decode_date(pal, base);
    check_decode_numeric(pal, base);   /* Tier-0 manifest + Tier-1 leaf */

    /* ---- S4.2 collation (ndx_key_cmp) checks ---- */
    check_cmp_char(pal, base);         /* CNAMES: unsigned byte order */
    check_cmp_date(pal, base);         /* TOURDATE: ascending JDN */
    check_cmp_numeric_leaf(pal, base); /* NCOST: arithmetic; primary mutant */

    pal_host_free(pal);
    return TEST_SUMMARY("test-ndx-keys");
}
