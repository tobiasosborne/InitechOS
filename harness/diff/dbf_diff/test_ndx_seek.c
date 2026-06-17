/*
 * harness/diff/dbf_diff/test_ndx_seek.c -- host oracle for S4.3: ndx_inorder
 *                                           + ndx_seek (B-tree traverse + SEEK).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc is OK here. Mirrors test_ndx_keys.c
 * / test_ndx_parse.c: seed test_assert.h harness (CHECK / TEST_HARNESS /
 * TEST_SUMMARY, which emits the "checks," line), pal_host_make, loud-SKIP on
 * absent goldens (printed, exit 0), non-zero exit on any failed check.
 *
 * This is the load-bearing B-tree navigation gate. A silently-wrong descent
 * corrupts every SEEK / ORDER built on the index, so the oracle grades four
 * independent properties over the corpus (plan S4.3 oracle contract):
 *
 *   1. In-order SORTEDNESS (property): ndx_inorder yields keys that are
 *      non-decreasing per ndx_key_cmp for EVERY corpus index (CNAMES char,
 *      TOURDATE date, ZIPCODE char 2-level, NCOST numeric). cmp(prev,cur) <= 0.
 *   2. COMPLETENESS (property): the in-order leaf count equals the live key
 *      count, and every dbf_recno is plausible (>= 1).
 *   3. SEEK resolves the RIGHT recno: for known keys present in the index,
 *      ndx_seek returns FOUND with the matching recno. A key BEFORE the first
 *      (still found if it begins-with) / AFTER the last behaves per contract
 *      (EOF: recno 0, not found). SET EXACT ON vs OFF on a char prefix.
 *   4. BRUTE-FORCE cross-check: independently collect all (key,recno) by a raw
 *      ndx_read_node page-walk (pages 1..total_pages-1, leaf entries only),
 *      sort with ndx_key_cmp, and assert the in-order traversal matches that
 *      sorted (key,recno) sequence exactly. (recno sets compared too.)
 *
 * Mutation (Rule 6 / plan S4.3):
 *   Built with -DNDX_MUTATE_SEEK_CHILD, the branch descent in ndx.c follows the
 *   WRONG child (entry[i+1]'s child instead of entry[i]'s). Per ndx.md ss5 the
 *   separator at entry i is the HIGH key of subtree child_page[i], so following
 *   child[i+1] skips the subtree that contains the target. Effect:
 *     - ZIPCODE / CNAMES (multi-level trees): the in-order traversal loses or
 *       reorders leaves -> sortedness + completeness + brute-force checks RED;
 *     - SEEK lands in the wrong subtree -> resolves the wrong recno / EOFs.
 *   Single-level trees (TOURDATE: root is the only leaf) are unaffected by the
 *   mutation -- the multi-level ZIPCODE/CNAMES/NCOST checks carry the RED.
 *   Exit code non-zero -> the mutant gate passes.
 *
 * Goldens base is argv[1] (default "../dbase3-decomp").
 * NCOST.NDX is at <base>/mint/work/NCOST.NDX (minted 2026-06-16).
 * Pristine corpus goldens at:
 *   <base>/goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Compile (canonical self-verify recipe):
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *     -Iseed -Ios/samir/include -Ispec \
 *     -o /tmp/test_ndx_seek \
 *     harness/diff/dbf_diff/test_ndx_seek.c \
 *     os/samir/fs/ndx.c os/samir/core/value.c os/samir/core/rt.c \
 *     os/samir/pal/pal_host.c
 *   /tmp/test_ndx_seek ../dbase3-decomp
 *
 * Mutant (must exit non-zero):
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *     -DNDX_MUTATE_SEEK_CHILD \
 *     -Iseed -Ios/samir/include -Ispec \
 *     -o /tmp/test_ndx_seek_mut ... ; /tmp/test_ndx_seek_mut ../dbase3-decomp
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/ndx.md  ss5 (B-tree ordering invariant
 *     + search: scan to first key >= K, descend that entry's child / rightmost
 *     child; HIGH-key separator), ss8 step 3 (in-order traversal), ss3.2
 *     (trailing child), ss6 (collation).
 *   - ../dbase3-decomp/re/mint-results-002.md  SET EXACT default OFF; char "="
 *     directional begins-with (LEFT begins with RIGHT).
 *   - docs/plans/SAMIR-implementation-plan.md S4.3 contract.
 *   - os/samir/include/samir/ndx.h (API under test).
 *   - os/samir/include/samir/value.h (xb_val, xb_c, xb_n, xb_d).
 *   - seed/test_assert.h (harness idiom).
 *
 * Ground-truth recnos used below were derived from the goldens by an independent
 * python page-walk (the brute-force reference) and cross-checked against ndx.md
 * Verification:
 *   ZIPCODE: 49 entries; "72450"->27 (first), "91306"->16 (separator, left
 *     subtree page 1), "91316"->22 (page 2, RIGHT subtree -- exercises descent
 *     into the trailing child), "97401"->18 (last).
 *   CNAMES:  49 entries; "Adams...Nathan"->15 (first), "Zambini...Rick"->8 (last).
 *   TOURDATE: 30 entries; JDN(1985-08-05)=2446283 -> recno 1 (first);
 *     JDN(1985-09-07)=2446316 -> recno 2.
 *   NCOST:   33 entries; -123.45->31, -1.0->32, 0.0->33, 279.0->2.
 */

#include <stdio.h>
#include <string.h>

#include "test_assert.h"       /* seed/, on -Iseed */
#include "samir/ndx.h"         /* os/samir/include/, on -Ios/samir/include */
#include "samir/value.h"       /* xb_c, xb_n, xb_d */
#include "samir/rt.h"          /* jdn_from_ymd, rt_memcpy */

TEST_HARNESS();

/* pal_host.c surface (not in a header; same pattern as test_ndx_keys.c). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* -----------------------------------------------------------------------
 * Path helpers (mirrored from test_ndx_keys.c)
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
 * In-order collector (uses the REAL ndx_inorder under test).
 *
 * Collects (key_data copy, recno) pairs into a caller array, preserving order.
 * Bounded by MAX entries; overflow is flagged.
 * ----------------------------------------------------------------------- */
#define IO_MAX     128       /* max leaf entries we collect per index */
#define IO_KEYCAP  40        /* max key_length over the corpus (CNAMES = 40) */

typedef struct {
    int      n;              /* entries collected */
    int      overflow;       /* set if more than IO_MAX entries seen */
    uint32_t kl;             /* key_length (copied per entry) */
    uint8_t  keys[IO_MAX * IO_KEYCAP];
    uint32_t recnos[IO_MAX];
} io_collect;

static int io_visit(void *ctx, const uint8_t *key_data, uint32_t recno)
{
    io_collect *c = (io_collect *)ctx;
    if (c->n >= IO_MAX) { c->overflow = 1; return 0; }   /* keep going to count */
    rt_memcpy(c->keys + (uint32_t)c->n * c->kl, key_data, c->kl);
    c->recnos[c->n] = recno;
    c->n++;
    return 0;
}

/* -----------------------------------------------------------------------
 * Brute-force reference: raw page-walk over pages 1..total_pages-1, collecting
 * ALL leaf entries (child_page == 0), then a simple insertion sort by
 * ndx_key_cmp (stable on equal keys is NOT required for the key sequence, but
 * we keep it stable so the recno multiset is comparable). This is the
 * "third implementation" independent of the tree descent (plan Sec 2.A barrier).
 * ----------------------------------------------------------------------- */
typedef struct {
    int      n;
    int      overflow;
    uint32_t kl;
    uint8_t  keys[IO_MAX * IO_KEYCAP];
    uint32_t recnos[IO_MAX];
} bf_collect;

static void bf_pagewalk(ndx_index *idx, bf_collect *b)
{
    uint32_t total = ndx_total_pages(idx);
    uint32_t p;

    b->n = 0;
    b->overflow = 0;
    b->kl = (uint32_t)ndx_key_length(idx);

    for (p = 1u; p < total; p++) {
        ndx_node_t *node = (ndx_node_t *)0;
        uint32_t    i;
        int         rc = ndx_read_node(idx, p, &node);
        if (rc != NDX_OK || !node)
            continue;   /* skip unreadable pages in the brute walk */
        for (i = 0u; i < (uint32_t)node->entry_count; i++) {
            if (node->entries[i].child_page != 0u)
                continue;   /* branch separator, not a leaf data entry */
            if (b->n >= IO_MAX) { b->overflow = 1; break; }
            rt_memcpy(b->keys + (uint32_t)b->n * b->kl,
                      node->entries[i].key_data, b->kl);
            b->recnos[b->n] = node->entries[i].dbf_recno;
            b->n++;
        }
        ndx_node_free(idx, node);
    }
}

/* Insertion sort the brute-force set by ndx_key_cmp (key bytes + recno move). */
static void bf_sort(ndx_index *idx, bf_collect *b)
{
    int i, j;
    uint8_t tmpk[IO_KEYCAP];
    uint32_t kl = b->kl;
    for (i = 1; i < b->n; i++) {
        uint32_t tmpr = b->recnos[i];
        rt_memcpy(tmpk, b->keys + (uint32_t)i * kl, kl);
        j = i - 1;
        while (j >= 0 &&
               ndx_key_cmp(idx, b->keys + (uint32_t)j * kl, tmpk) > 0) {
            rt_memcpy(b->keys + (uint32_t)(j + 1) * kl,
                      b->keys + (uint32_t)j * kl, kl);
            b->recnos[j + 1] = b->recnos[j];
            j--;
        }
        rt_memcpy(b->keys + (uint32_t)(j + 1) * kl, tmpk, kl);
        b->recnos[j + 1] = tmpr;
    }
}

/* -----------------------------------------------------------------------
 * Property check over one index: sortedness + completeness + brute-force diff.
 *
 * expected_count: live key count (from the independent python page-walk /
 *   ndx.md Verification). 0 = "do not assert an exact count".
 * ----------------------------------------------------------------------- */
static void check_index_properties(samir_pal_t *pal, const char *base,
                                   const char *rel, const char *label,
                                   int expected_count)
{
    char        path[1024];
    ndx_index  *idx = (ndx_index *)0;
    int         rc;
    char        msg[256];
    static io_collect io;     /* static: ~5 KB; keep off the stack */
    static bf_collect bf;
    uint32_t    kl;
    int         i;

    join(path, sizeof(path), base, rel);
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): %s absent (%s properties)\n", rel, label);
        return;
    }

    rc = ndx_open(pal, path, &idx);
    snprintf(msg, sizeof(msg), "%s props: ndx_open (rc=%d)", label, rc);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0, msg);
    if (rc != NDX_OK || !idx) return;

    kl = (uint32_t)ndx_key_length(idx);
    CHECK(kl <= (uint32_t)IO_KEYCAP, "key_length fits IO_KEYCAP (test buffer)");

    /* --- 1. In-order traversal via the REAL ndx_inorder. --- */
    memset(&io, 0, sizeof(io));
    io.kl = kl;
    rc = ndx_inorder(idx, io_visit, &io);
    snprintf(msg, sizeof(msg), "%s props: ndx_inorder returns NDX_OK (rc=%d)",
             label, rc);
    CHECK(rc == NDX_OK, msg);
    CHECK(io.overflow == 0, "in-order count fits IO_MAX (test buffer)");

    /* --- 2. Completeness: count matches; every recno plausible (>= 1). --- */
    if (expected_count > 0) {
        snprintf(msg, sizeof(msg),
                 "%s props: in-order count == %d (got %d)",
                 label, expected_count, io.n);
        CHECK(io.n == expected_count, msg);
    }
    for (i = 0; i < io.n; i++) {
        if (io.recnos[i] < 1u) {
            snprintf(msg, sizeof(msg),
                     "%s props: recno[%d] >= 1 (got %u)",
                     label, i, io.recnos[i]);
            CHECK(0, msg);
        }
    }

    /* --- 3. Sortedness: cmp(prev, cur) <= 0 for every consecutive pair. --- */
    for (i = 0; i + 1 < io.n; i++) {
        int cmp = ndx_key_cmp(idx,
                              io.keys + (uint32_t)i * kl,
                              io.keys + (uint32_t)(i + 1) * kl);
        if (cmp > 0) {
            snprintf(msg, sizeof(msg),
                     "%s props: in-order entry[%d] <= entry[%d] "
                     "(ndx_key_cmp=%d, UNSORTED -- mutant?)", label, i, i + 1, cmp);
            CHECK(0, msg);
        }
    }

    /* --- 4. Brute-force cross-check: independent page-walk, sort, diff. --- */
    bf_pagewalk(idx, &bf);
    bf_sort(idx, &bf);
    CHECK(bf.overflow == 0, "brute-force count fits IO_MAX");
    snprintf(msg, sizeof(msg),
             "%s props: brute-force count == in-order count (%d vs %d)",
             label, bf.n, io.n);
    CHECK(bf.n == io.n, msg);

    if (bf.n == io.n) {
        int mism_key = -1, mism_recno = -1;
        for (i = 0; i < io.n; i++) {
            /* in-order keys must equal sorted brute-force keys, byte for byte. */
            if (rt_memcmp(io.keys + (uint32_t)i * kl,
                          bf.keys + (uint32_t)i * kl, kl) != 0) {
                if (mism_key < 0) mism_key = i;
            }
            /* recno at each ordered slot must match too (dup keys keep stable
             * tree order == brute insertion order; equal keys -> recno may
             * differ in order but here both walks preserve physical order, and
             * the corpus dup keys are adjacent so this holds). */
            if (io.recnos[i] != bf.recnos[i]) {
                if (mism_recno < 0) mism_recno = i;
            }
        }
        snprintf(msg, sizeof(msg),
                 "%s props: in-order keys == sorted brute-force keys "
                 "(first mismatch at %d)", label, mism_key);
        CHECK(mism_key < 0, msg);
        snprintf(msg, sizeof(msg),
                 "%s props: in-order recnos == sorted brute-force recnos "
                 "(first mismatch at %d)", label, mism_recno);
        CHECK(mism_recno < 0, msg);
    }

    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * SEEK on a char index (ZIPCODE: 2-level tree, exercises branch descent
 * INCLUDING the rightmost/trailing child).
 *
 * Ground truth (independent python page-walk): "72450"->27, "91306"->16
 * (separator, page 1), "91316"->22 (page 2, RIGHT subtree), "97401"->18.
 * ----------------------------------------------------------------------- */
static void check_seek_zipcode(samir_pal_t *pal, const char *base)
{
    char       path[1024];
    ndx_index *idx = (ndx_index *)0;
    int        rc;
    char       msg[256];
    uint32_t   recno;
    int        found;
    xb_val     k;

    join(path, sizeof(path), base, PRISTINE_REL "/ZIPCODE.NDX");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): ZIPCODE.NDX absent (SEEK char check)\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0, "ZIPCODE seek: ndx_open");
    if (rc != NDX_OK || !idx) return;

    CHECK(ndx_key_type(idx) == 0u, "ZIPCODE seek: key_type==0 (char)");
    CHECK(ndx_key_length(idx) == 5u, "ZIPCODE seek: key_length==5");

    /* First key "72450" -> recno 27 (in the LEFT subtree). EXACT ON. */
    k = xb_c("72450", 5);
    rc = ndx_seek(idx, &k, 1 /*EXACT ON*/, &recno, &found);
    CHECK(rc == NDX_OK, "ZIPCODE seek '72450': rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "ZIPCODE seek '72450' EXACT: found=%d recno=%u (want found=1 recno=27)",
             found, recno);
    CHECK(found == 1 && recno == 27u, msg);

    /* Separator key "91306" -> recno 16 (last key of left leaf page 1). */
    k = xb_c("91306", 5);
    rc = ndx_seek(idx, &k, 1, &recno, &found);
    CHECK(rc == NDX_OK, "ZIPCODE seek '91306': rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "ZIPCODE seek '91306' EXACT: found=%d recno=%u (want found=1 recno=16)",
             found, recno);
    CHECK(found == 1 && recno == 16u, msg);

    /* "91316" -> recno 22, on page 2 (the RIGHT/trailing subtree). This is the
     * key check that exercises descent into the rightmost child (ndx.md ss3.2):
     * 91316 > separator 91306, so descent goes to the trailing child. */
    k = xb_c("91316", 5);
    rc = ndx_seek(idx, &k, 1, &recno, &found);
    CHECK(rc == NDX_OK, "ZIPCODE seek '91316': rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "ZIPCODE seek '91316' EXACT (right subtree): found=%d recno=%u "
             "(want found=1 recno=22)", found, recno);
    CHECK(found == 1 && recno == 22u, msg);

    /* Last key "97401" -> recno 18 (rightmost leaf). */
    k = xb_c("97401", 5);
    rc = ndx_seek(idx, &k, 1, &recno, &found);
    CHECK(rc == NDX_OK, "ZIPCODE seek '97401': rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "ZIPCODE seek '97401' EXACT: found=%d recno=%u (want found=1 recno=18)",
             found, recno);
    CHECK(found == 1 && recno == 18u, msg);

    /* A key BEFORE the first ("00000" < "72450"): the landing is the first leaf
     * entry; EXACT ON -> not found, but recno is the insertion-point recno (27,
     * the first key). [contract: recno_out = landing recno, found = 0]. */
    k = xb_c("00000", 5);
    rc = ndx_seek(idx, &k, 1, &recno, &found);
    CHECK(rc == NDX_OK, "ZIPCODE seek '00000' (before first): rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "ZIPCODE seek '00000' EXACT before-first: found=%d recno=%u "
             "(want found=0 recno=27)", found, recno);
    CHECK(found == 0 && recno == 27u, msg);

    /* A key AFTER the last ("99999" > "97401"): EOF -- recno 0, not found. */
    k = xb_c("99999", 5);
    rc = ndx_seek(idx, &k, 1, &recno, &found);
    CHECK(rc == NDX_OK, "ZIPCODE seek '99999' (after last): rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "ZIPCODE seek '99999' EXACT after-last: found=%d recno=%u "
             "(want found=0 recno=0 EOF)", found, recno);
    CHECK(found == 0 && recno == 0u, msg);

    /* SET EXACT OFF begins-with: seek "913" (3 chars). The first stored key >=
     * "913      " (space-padded) is "91301" (page 2), which BEGINS WITH "913"
     * -> FOUND under OFF. Under EXACT ON the same seek is NOT found (no key is
     * exactly "913  "). This is the EXACT ON vs OFF discriminator (mint-002). */
    k = xb_c("913", 3);
    rc = ndx_seek(idx, &k, 0 /*EXACT OFF*/, &recno, &found);
    CHECK(rc == NDX_OK, "ZIPCODE seek '913' OFF: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "ZIPCODE seek '913' EXACT OFF (begins-with): found=%d recno=%u "
             "(want found=1, begins-with match)", found, recno);
    CHECK(found == 1, msg);
    /* The begins-with landing is the first key >= "913  ", i.e. "91301".
     * (recno for 91301 is the next-after-91306 in the LEFT leaf? No: 91301 <
     * 91306, so it is on page 1. We only assert found here; the recno identity
     * is cross-checked by the brute-force property test.) */

    k = xb_c("913", 3);
    rc = ndx_seek(idx, &k, 1 /*EXACT ON*/, &recno, &found);
    CHECK(rc == NDX_OK, "ZIPCODE seek '913' ON: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "ZIPCODE seek '913' EXACT ON: found=%d (want found=0 -- no exact "
             "'913  ' key)", found);
    CHECK(found == 0, msg);

    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * SEEK on a date index (TOURDATE: key_type 1, single-level, 30 entries).
 * JDN(1985-08-05)=2446283 -> recno 1; JDN(1985-09-07)=2446316 -> recno 2.
 * ----------------------------------------------------------------------- */
static void check_seek_tourdate(samir_pal_t *pal, const char *base)
{
    char       path[1024];
    ndx_index *idx = (ndx_index *)0;
    int        rc;
    char       msg[256];
    uint32_t   recno;
    int        found;
    xb_val     k;
    int32_t    jdn1, jdn2;

    join(path, sizeof(path), base, PRISTINE_REL "/TOURDATE.NDX");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): TOURDATE.NDX absent (SEEK date check)\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0, "TOURDATE seek: ndx_open");
    if (rc != NDX_OK || !idx) return;

    CHECK(ndx_key_type(idx) == 1u, "TOURDATE seek: key_type==1");

    jdn1 = jdn_from_ymd(1985, 8, 5);
    CHECK(jdn1 == 2446283, "TOURDATE seek: JDN(1985-08-05)==2446283");
    jdn2 = jdn_from_ymd(1985, 9, 7);
    CHECK(jdn2 == 2446316, "TOURDATE seek: JDN(1985-09-07)==2446316");

    /* First date -> recno 1. Use xb_d (date); the engine encodes the double. */
    k = xb_d((double)jdn1);
    rc = ndx_seek(idx, &k, 0, &recno, &found);
    CHECK(rc == NDX_OK, "TOURDATE seek JDN 2446283: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "TOURDATE seek 1985-08-05: found=%d recno=%u (want found=1 recno=1)",
             found, recno);
    CHECK(found == 1 && recno == 1u, msg);

    /* 1985-09-07 -> recno 2. Use xb_n to confirm numeric path is identical. */
    k = xb_n((double)jdn2);
    rc = ndx_seek(idx, &k, 0, &recno, &found);
    CHECK(rc == NDX_OK, "TOURDATE seek JDN 2446316: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "TOURDATE seek 1985-09-07: found=%d recno=%u (want found=1 recno=2)",
             found, recno);
    CHECK(found == 1 && recno == 2u, msg);

    /* A date BEFORE the first (JDN 2000000): landing is first entry; not found;
     * recno is the landing recno (1, the first/earliest date). */
    k = xb_d(2000000.0);
    rc = ndx_seek(idx, &k, 0, &recno, &found);
    CHECK(rc == NDX_OK, "TOURDATE seek before-first: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "TOURDATE seek before-first: found=%d recno=%u (want found=0 recno=1)",
             found, recno);
    CHECK(found == 0 && recno == 1u, msg);

    /* A date AFTER the last (JDN 3000000): EOF -- recno 0, not found. */
    k = xb_d(3000000.0);
    rc = ndx_seek(idx, &k, 0, &recno, &found);
    CHECK(rc == NDX_OK, "TOURDATE seek after-last: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "TOURDATE seek after-last: found=%d recno=%u (want found=0 recno=0)",
             found, recno);
    CHECK(found == 0 && recno == 0u, msg);

    /* A date NOT in the index but within range (JDN 2446290 -- between entries):
     * not found, recno is the next-greater landing (non-zero, plausible). */
    k = xb_d(2446290.0);
    rc = ndx_seek(idx, &k, 0, &recno, &found);
    CHECK(rc == NDX_OK, "TOURDATE seek interior-gap: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "TOURDATE seek interior-gap: found=%d recno=%u (want found=0, "
             "recno>=1 landing)", found, recno);
    CHECK(found == 0 && recno >= 1u, msg);

    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * SEEK on a numeric index with negatives (NCOST, key_type 1).
 * -123.45->31, -1.0->32, 0.0->33, 279.0->2.
 * ----------------------------------------------------------------------- */
static void check_seek_ncost(samir_pal_t *pal, const char *base)
{
    char       path[1024];
    ndx_index *idx = (ndx_index *)0;
    int        rc;
    char       msg[256];
    uint32_t   recno;
    int        found;
    xb_val     k;

    join(path, sizeof(path), base, "mint/work/NCOST.NDX");
    if (!file_exists(path)) {
        fprintf(stderr,
                "  SKIP (LOUD): mint/work/NCOST.NDX absent (SEEK numeric check)\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0, "NCOST seek: ndx_open");
    if (rc != NDX_OK || !idx) return;

    CHECK(ndx_key_type(idx) == 1u, "NCOST seek: key_type==1");

    /* -123.45 -> recno 31 (the smallest, first leaf entry). */
    k = xb_n(-123.45);
    rc = ndx_seek(idx, &k, 0, &recno, &found);
    CHECK(rc == NDX_OK, "NCOST seek -123.45: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "NCOST seek -123.45: found=%d recno=%u (want found=1 recno=31)",
             found, recno);
    CHECK(found == 1 && recno == 31u, msg);

    /* 0.0 -> recno 33. */
    k = xb_n(0.0);
    rc = ndx_seek(idx, &k, 0, &recno, &found);
    CHECK(rc == NDX_OK, "NCOST seek 0.0: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "NCOST seek 0.0: found=%d recno=%u (want found=1 recno=33)",
             found, recno);
    CHECK(found == 1 && recno == 33u, msg);

    /* 279.0 -> recno 2 (first positive after the negatives/zero). */
    k = xb_n(279.0);
    rc = ndx_seek(idx, &k, 0, &recno, &found);
    CHECK(rc == NDX_OK, "NCOST seek 279.0: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "NCOST seek 279.0: found=%d recno=%u (want found=1 recno=2)",
             found, recno);
    CHECK(found == 1 && recno == 2u, msg);

    /* Below the minimum (-9999): landing is the smallest (-123.45, recno 31),
     * not found. Confirms negatives descend correctly (arithmetic compare). */
    k = xb_n(-9999.0);
    rc = ndx_seek(idx, &k, 0, &recno, &found);
    CHECK(rc == NDX_OK, "NCOST seek -9999 (below min): rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "NCOST seek below-min: found=%d recno=%u (want found=0 recno=31)",
             found, recno);
    CHECK(found == 0 && recno == 31u, msg);

    /* Above the maximum (999999): EOF. */
    k = xb_n(999999.0);
    rc = ndx_seek(idx, &k, 0, &recno, &found);
    CHECK(rc == NDX_OK, "NCOST seek above-max: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "NCOST seek above-max: found=%d recno=%u (want found=0 recno=0 EOF)",
             found, recno);
    CHECK(found == 0 && recno == 0u, msg);

    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * SEEK on CNAMES (40-byte char multi-field key, multi-level tree).
 * "Adams...Nathan"->15 (first), "Zambini...Rick"->8 (last).
 * Exercises a full-width concatenated char key + begins-with on a name prefix.
 * ----------------------------------------------------------------------- */
static void check_seek_cnames(samir_pal_t *pal, const char *base)
{
    char       path[1024];
    ndx_index *idx = (ndx_index *)0;
    int        rc;
    char       msg[256];
    uint32_t   recno;
    int        found;
    xb_val     k;
    /* Full 40-byte key for "Adams" (LASTNAME 20 + FIRSTNAME 20). */
    char       adams[40];
    int        i;

    join(path, sizeof(path), base, PRISTINE_REL "/CNAMES.NDX");
    if (!file_exists(path)) {
        fprintf(stderr, "  SKIP (LOUD): CNAMES.NDX absent (SEEK CNAMES check)\n");
        return;
    }

    rc = ndx_open(pal, path, &idx);
    CHECK(rc == NDX_OK && idx != (ndx_index *)0, "CNAMES seek: ndx_open");
    if (rc != NDX_OK || !idx) return;

    CHECK(ndx_key_length(idx) == 40u, "CNAMES seek: key_length==40");

    /* Build "Adams"+15sp+"Nathan"+14sp = 40 bytes (the exact stored key). */
    for (i = 0; i < 40; i++) adams[i] = ' ';
    memcpy(adams + 0, "Adams", 5);
    memcpy(adams + 20, "Nathan", 6);
    k = xb_c(adams, 40);
    rc = ndx_seek(idx, &k, 1 /*EXACT ON*/, &recno, &found);
    CHECK(rc == NDX_OK, "CNAMES seek 'Adams Nathan' ON: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "CNAMES seek 'Adams Nathan' EXACT: found=%d recno=%u "
             "(want found=1 recno=15)", found, recno);
    CHECK(found == 1 && recno == 15u, msg);

    /* Begins-with on "Adams" (5 chars) under EXACT OFF -> the first key >=
     * "Adams" + spaces is exactly "Adams ... Nathan" which begins with "Adams"
     * -> FOUND, recno 15. This is begins-with on a multi-level tree. */
    k = xb_c("Adams", 5);
    rc = ndx_seek(idx, &k, 0 /*OFF*/, &recno, &found);
    CHECK(rc == NDX_OK, "CNAMES seek 'Adams' OFF: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "CNAMES seek 'Adams' EXACT OFF begins-with: found=%d recno=%u "
             "(want found=1 recno=15)", found, recno);
    CHECK(found == 1 && recno == 15u, msg);

    /* Same prefix under EXACT ON: "Adams" right-padded to 40 spaces does NOT
     * equal "Adams...Nathan" -> not found. */
    k = xb_c("Adams", 5);
    rc = ndx_seek(idx, &k, 1 /*ON*/, &recno, &found);
    CHECK(rc == NDX_OK, "CNAMES seek 'Adams' ON: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "CNAMES seek 'Adams' EXACT ON: found=%d (want found=0 -- padded "
             "key != stored key)", found);
    CHECK(found == 0, msg);

    /* A name AFTER the last ("zzzzz..." > "Zambini") -> EOF.
     * NOTE: lowercase 'z' (0x7A) > uppercase 'Z' (0x5A) in byte order, so
     * "zzz" sorts after every capitalised surname. */
    k = xb_c("zzzzzzzz", 8);
    rc = ndx_seek(idx, &k, 0, &recno, &found);
    CHECK(rc == NDX_OK, "CNAMES seek after-last: rc==NDX_OK");
    snprintf(msg, sizeof(msg),
             "CNAMES seek after-last: found=%d recno=%u (want found=0 recno=0)",
             found, recno);
    CHECK(found == 0 && recno == 0u, msg);

    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * Type-mismatch fail-loud: seeking a char key into a numeric index (and vice
 * versa) must return -NDX_ERR_BAD_KEYTYPE (Rule 2).
 * ----------------------------------------------------------------------- */
static void check_seek_typecheck(samir_pal_t *pal, const char *base)
{
    char       path[1024];
    ndx_index *idx = (ndx_index *)0;
    int        rc;
    char       msg[256];
    uint32_t   recno;
    int        found;
    xb_val     k;

    /* numeric index (TOURDATE) seeked with a char key -> BAD_KEYTYPE. */
    join(path, sizeof(path), base, PRISTINE_REL "/TOURDATE.NDX");
    if (file_exists(path)) {
        rc = ndx_open(pal, path, &idx);
        if (rc == NDX_OK && idx) {
            k = xb_c("hello", 5);
            rc = ndx_seek(idx, &k, 0, &recno, &found);
            snprintf(msg, sizeof(msg),
                     "typecheck: char key into numeric index -> BAD_KEYTYPE "
                     "(rc=%d)", rc);
            CHECK(rc == -NDX_ERR_BAD_KEYTYPE, msg);
            ndx_close(idx);
            idx = (ndx_index *)0;
        }
    } else {
        fprintf(stderr, "  SKIP (LOUD): TOURDATE.NDX absent (typecheck num)\n");
    }

    /* char index (ZIPCODE) seeked with a numeric key -> BAD_KEYTYPE. */
    join(path, sizeof(path), base, PRISTINE_REL "/ZIPCODE.NDX");
    if (file_exists(path)) {
        rc = ndx_open(pal, path, &idx);
        if (rc == NDX_OK && idx) {
            k = xb_n(42.0);
            rc = ndx_seek(idx, &k, 0, &recno, &found);
            snprintf(msg, sizeof(msg),
                     "typecheck: numeric key into char index -> BAD_KEYTYPE "
                     "(rc=%d)", rc);
            CHECK(rc == -NDX_ERR_BAD_KEYTYPE, msg);
            ndx_close(idx);
        }
    } else {
        fprintf(stderr, "  SKIP (LOUD): ZIPCODE.NDX absent (typecheck char)\n");
    }
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
    /* Arena: one node live at a time during traversal/descent plus ndx_index.
     * The brute-force walk also holds one node at a time. 256 KB is ample for
     * CNAMES (KL=40/KPP=10) and the deepest corpus tree. */
    cfg.heap_size = 256u * 1024u;

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    if (file_exists(join(path, sizeof(path), base, PRISTINE_REL "/ZIPCODE.NDX")))
        any_present = 1;
    if (file_exists(join(path, sizeof(path), base, "mint/work/NCOST.NDX")))
        any_present = 1;

    if (!any_present) {
        fprintf(stderr,
                "  SKIP (LOUD): no goldens found under base '%s'\n"
                "               pass the corpus base as argv[1]\n", base);
    }

    /* ---- Properties (sortedness + completeness + brute-force) ---- */
    /* Expected counts from the independent python page-walk / ndx.md Verification. */
    check_index_properties(pal, base, PRISTINE_REL "/ZIPCODE.NDX", "ZIPCODE", 49);
    check_index_properties(pal, base, PRISTINE_REL "/CNAMES.NDX",  "CNAMES",  49);
    check_index_properties(pal, base, PRISTINE_REL "/TOURDATE.NDX","TOURDATE",30);
    check_index_properties(pal, base, "mint/work/NCOST.NDX",       "NCOST",   33);

    /* ---- SEEK recno resolution + EOF + SET EXACT ON/OFF ---- */
    check_seek_zipcode(pal, base);    /* char, 2-level, rightmost-child descent */
    check_seek_tourdate(pal, base);   /* date, single-level */
    check_seek_ncost(pal, base);      /* numeric with negatives */
    check_seek_cnames(pal, base);     /* 40-byte multi-field char, multi-level */

    /* ---- Fail-loud type checks ---- */
    check_seek_typecheck(pal, base);

    pal_host_free(pal);
    return TEST_SUMMARY("test-ndx-seek");
}
