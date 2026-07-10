/*
 * harness/diff/dbf_diff/test_ndx_multilevel.c -- host oracle for the multi-level
 *   (3+ level) .ndx B-tree: bulk INDEX ON build past the 2-level ceiling
 *   (bead initech-h5vg).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc is OK here. Same seed test_assert.h
 * harness idiom as test_ndx_build.c / test_ndx_seek.c (CHECK / TEST_HARNESS /
 * TEST_SUMMARY, non-zero exit on any failed check). Links the engine codec
 * (ndx.c) + value + rt + pal_host ONLY -- NO evaluator/dbf: the per-record key
 * bytes are supplied by a synthetic in-memory key provider, so this leg is
 * independent of eval.c and grades the codec directly.
 *
 * WHY THIS EXISTS (Law 1 / Law 2 -- ground truth + independent oracle):
 *   ndx_build used to cap the tree at 2 levels: once (nleaf-1) > keys_per_page a
 *   single root branch could not index all the leaves, and ndx_build returned
 *   -NDX_ERR_PAGE_OVF (the honest fail-loud, Rule 2). Real dBASE III+ builds a
 *   tree of arbitrary depth (nested branch levels). This oracle forces a genuine
 *   3-level tree and asserts the build succeeds AND is a STRUCTURALLY VALID
 *   B-tree.
 *
 * PROVENANCE OF THE GRADING (Law 1 honesty):
 *   There is NO minted 3-level golden .ndx in the corpus. The largest pristine/
 *   minted golden is BIGIDX.NDX (16 pages, 2-level: 14 leaves + 1 root; mint-
 *   results-003). The multi-level ("BIGXA"/ndx-B5) real-dBASE mint is PLANNED but
 *   NOT YET MINTED (re/mint-formats.md ndx-B5; loud-skipped in the differential).
 *   So a 3-level tree CANNOT be graded byte-exact against real dBASE. Instead
 *   this oracle grades against the FORMAT INVARIANTS that ndx.md ss5 states and
 *   that every 2-level golden was byte-verified to obey:
 *     - B-tree ordering: in-order traversal is strictly ascending; count == nrec
 *       (ndx.md ss5 "keys ascending ... in-order yields a fully sorted sequence").
 *     - HIGH-key separator: in a branch node, separator[i] == the MAX key of the
 *       subtree rooted at child_page[i] (ndx.md ss5 "Branch separator = HIGH key
 *       of subtree"); trailing child holds keys > the last separator (ss3.2).
 *     - Balanced depth: every leaf is at the same depth (a real B-tree invariant;
 *       the read paths ndx_seek/ndx_inorder assume it).
 *     - Root reachable; every branch node has >= 1 separator (so the reader's
 *       branch-vs-leaf test -- "first live entry has child_page != 0" -- fires).
 *     - Point lookups (ndx_seek) resolve the correct recno for present keys and
 *       report not-found for absent keys, across all 3 levels.
 *   The 2-level byte-exact goldens remain graded by test_ndx_build.c; the 2-level
 *   build path is unchanged, so those goldens are the regression guard that the
 *   refactor did not perturb the minted-exact case.
 *
 * MUTATION (Rule 6): built with -DNDX_MUTATE_NO_ROOT_SPLIT, ndx_build reverts to
 *   the OLD ceiling and returns -NDX_ERR_PAGE_OVF for a tree that needs 3 levels.
 *   The build-success check then goes RED (exit non-zero) -> the mutant gate
 *   passes, proving this oracle actually exercises the new multi-level path.
 *
 * KEYS_PER_PAGE ARITHMETIC (stated per the task; ndx_format.h derived formulas):
 *   char key, key_length = 20  ->  group_length = ceil4(20 + 8) = 28
 *                              ->  keys_per_page = (512 - 4) / 28 = 508/28 = 18.
 *   A 2-level tree tops out at (kpp + 1) = 19 leaves (kpp separators + 1 trailing
 *   child). Forcing >= 20 leaves needs a 3rd level.
 *   nrec = 343 -> nleaf = ceil(343 / 18) = 20 leaves (19*18 = 342 < 343).
 *   (nleaf - 1) = 19 > kpp = 18  ->  the OLD code returned -NDX_ERR_PAGE_OVF.
 *   Level shape (children/branch = kpp+1 = 19): L1 = ceil(20/19) = 2 branch
 *   nodes; L2 = ceil(2/19) = 1 root. Depth = 3 (root -> branch -> leaf), 24 pages
 *   (1 header + 20 leaves + 2 branches + 1 root), root_page = 23.
 *   [key_length 20 mirrors the minted BIGIDX (kpp 18); ndx.md ss2 formulas.]
 *
 * ASCII-clean (Rule 12). No host paths / timestamps baked into artifacts (Rule 11).
 *
 * Compile (self-verify recipe):
 *   ENG="os/samir/fs/ndx.c os/samir/core/value.c os/samir/core/rt.c"
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *     -Iseed -Ios/samir/include -Ispec \
 *     -o /tmp/test_ndx_multilevel harness/diff/dbf_diff/test_ndx_multilevel.c \
 *     $ENG os/samir/pal/pal_host.c
 *   /tmp/test_ndx_multilevel
 *   # mutant (must exit non-zero):
 *   cc ... -DNDX_MUTATE_NO_ROOT_SPLIT ... -o /tmp/test_ndx_multilevel_mut ...
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/ndx.md ss1/ss3/ss3.1/ss3.2 (geometry,
 *     node/group/trailing child), ss5 (ordering + HIGH-key separator + search).
 *   - ../dbase3-decomp/re/mint-results-003.md (bulk build: 100% L->R leaf pack,
 *     remainder last; N keys / N+1 children; 2-level BIGIDX minted).
 *   - ../dbase3-decomp/re/mint-formats.md ndx-B5 (the multi-level "BIGXA" mint --
 *     PLANNED / not yet minted; this oracle grades structure, not those bytes).
 *   - spec/samir/ndx_format.h (LOCKED offsets + derived formulas).
 *   - os/samir/include/samir/ndx.h (ndx_build / ndx_open / ndx_read_node / seek).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test_assert.h"        /* seed/, on -Iseed */
#include "samir/ndx.h"          /* os/samir/include/, on -Ios/samir/include */
#include "samir/ndx_format.h"   /* NDX_* constants, on -Ispec */
#include "samir/value.h"        /* xb_val, xb_c */
#include "samir/rt.h"           /* rt_memcpy, rt_memcmp, rt_memset */

TEST_HARNESS();

/* pal_host.c surface (not in a header; same pattern as the sibling tests). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* ----------------------------------------------------------------------- */
/* Test geometry (see the KEYS_PER_PAGE arithmetic in the header block).    */
/* ----------------------------------------------------------------------- */
#define ML_KEYLEN   20u          /* char key width -> kpp 18, cap 19 */
#define ML_NREC     343u         /* -> 20 leaves -> forces a 3-level tree */
#define ML_KPP      18u          /* (512-4)/ceil4(20+8) = 508/28 */

/* Synthetic key provider: recno r in 1..NREC gets value (NREC - r), rendered as
 * a fixed-width zero-padded decimal string in ML_KEYLEN bytes. Reversed vs recno
 * order so the build MUST sort; all keys distinct. After the ascending sort the
 * in-order sequence is value 0 (recno NREC), value 1 (recno NREC-1), ...,
 * value NREC-1 (recno 1). */
static void render_key(uint32_t value, uint8_t *out /* ML_KEYLEN bytes */)
{
    char dec[32];
    int  n, i, pad;
    n = snprintf(dec, sizeof(dec), "%010u", value);   /* 10 digits is plenty */
    /* Left-justify into an ML_KEYLEN field, space-padded on the right, matching
     * the on-disk char layout (ndx.md ss4.1). We instead zero-pad on the LEFT of
     * the numeric field so ASCII byte order == numeric order; then space-pad the
     * field to key width. */
    pad = (int)ML_KEYLEN - n;
    for (i = 0; i < (int)ML_KEYLEN; i++) out[i] = (uint8_t)' ';
    if (pad < 0) pad = 0;
    for (i = 0; i < n && (pad + i) < (int)ML_KEYLEN; i++)
        out[pad + i] = (uint8_t)dec[i];
    /* Left region [0,pad) is the zero fill so shorter numbers sort first. */
    for (i = 0; i < pad; i++) out[i] = (uint8_t)'0';
}

static uint32_t value_for_recno(uint32_t recno)   /* recno 1..NREC */
{
    return ML_NREC - recno;                        /* reversed permutation */
}

static int ml_get_key(void *user, uint32_t recno, uint8_t *key_out,
                      uint16_t key_len)
{
    (void)user;
    if (key_len != (uint16_t)ML_KEYLEN) return 1;
    if (recno < 1u || recno > ML_NREC) return 1;
    render_key(value_for_recno(recno), key_out);
    return 0;
}

/* ----------------------------------------------------------------------- */
/* Recursive structural validator (independent of ndx_seek/ndx_inorder).    */
/* Reads pages with ndx_read_node; asserts the HIGH-key + balanced-depth     */
/* invariants (ndx.md ss5). Returns 0 on OK, non-0 on structural violation.  */
/* On OK, *maxkey_out <- the MAX key of the subtree, *height_out <- subtree   */
/* height (leaves = 1).                                                       */
/* ----------------------------------------------------------------------- */
static int validate_subtree(ndx_index *idx, uint32_t page_no, uint32_t depth,
                            uint8_t *maxkey_out /* ML_KEYLEN */,
                            uint32_t *height_out, const char *label)
{
    ndx_node_t *node = NULL;
    uint32_t    kl   = (uint32_t)ndx_key_length(idx);
    uint32_t    cnt, i;
    int         is_branch;
    char        msg[192];
    int         rc_ok = 0;

    /* Local copies so we can free the node before recursing (arena is a bump
     * stack; a child alloc must sit above the freed parent mark). */
    uint32_t child_pg[64];
    uint8_t  sepkey[64][ML_KEYLEN];
    uint32_t trail_child;

    if (depth == 0u) { CHECK(0, "validate: depth budget exhausted (cycle?)"); return 1; }

    if (ndx_read_node(idx, page_no, &node) != NDX_OK || !node) {
        snprintf(msg, sizeof(msg), "%s: read node page %u", label, page_no);
        CHECK(0, msg);
        return 1;
    }
    cnt       = (uint32_t)node->entry_count;
    is_branch = (cnt > 0u && node->entries[0].child_page != 0u);

    if (is_branch) {
        if (cnt > 64u) { CHECK(0, "validate: branch fanout > test cap"); ndx_node_free(idx, node); return 1; }
        for (i = 0u; i < cnt; i++) {
            child_pg[i] = node->entries[i].child_page;
            rt_memcpy(sepkey[i], node->entries[i].key_data, kl);
        }
        trail_child = node->trail_child;
    }
    ndx_node_free(idx, node);
    node = NULL;

    if (!is_branch) {
        /* Leaf: entries must be strictly ascending; MAX = last entry key. */
        uint8_t prev[ML_KEYLEN];
        uint8_t cur[ML_KEYLEN];
        int have_prev = 0;
        /* Re-read to walk entries (we freed above to keep it simple). */
        if (ndx_read_node(idx, page_no, &node) != NDX_OK || !node) {
            CHECK(0, "validate: re-read leaf"); return 1;
        }
        cnt = (uint32_t)node->entry_count;
        for (i = 0u; i < cnt; i++) {
            rt_memcpy(cur, node->entries[i].key_data, kl);
            if (have_prev) {
                if (rt_memcmp(prev, cur, kl) > 0) {
                    snprintf(msg, sizeof(msg),
                             "%s: leaf page %u not ascending at entry %u",
                             label, page_no, i);
                    CHECK(0, msg); rc_ok = 1;
                }
            }
            rt_memcpy(prev, cur, kl);
            have_prev = 1;
        }
        if (cnt == 0u) { CHECK(0, "validate: empty leaf in a >1-record index"); rc_ok = 1; }
        rt_memcpy(maxkey_out, prev, kl);
        *height_out = 1u;
        ndx_node_free(idx, node);
        return rc_ok;
    }

    /* Branch: every child (sep[0..cnt-1].child + trailing) must have the SAME
     * height; separator[i] MUST equal the MAX key of child_pg[i] (HIGH-key
     * invariant, ndx.md ss5); the branch's own MAX = MAX of the trailing child. */
    {
        uint32_t child_h, first_h = 0u;
        uint8_t  child_max[ML_KEYLEN];
        int      first = 1;

        if (cnt < 1u) { CHECK(0, "validate: branch with 0 separators (reader would misread as leaf)"); return 1; }

        for (i = 0u; i < cnt; i++) {
            if (validate_subtree(idx, child_pg[i], depth - 1u, child_max,
                                 &child_h, label) != 0)
                return 1;
            if (first) { first_h = child_h; first = 0; }
            else if (child_h != first_h) {
                snprintf(msg, sizeof(msg),
                         "%s: unbalanced depth at branch page %u child %u "
                         "(h=%u vs %u)", label, page_no, i, child_h, first_h);
                CHECK(0, msg); return 1;
            }
            /* HIGH-key separator: separator[i] == MAX of child_pg[i]. */
            if (rt_memcmp(sepkey[i], child_max, kl) != 0) {
                snprintf(msg, sizeof(msg),
                         "%s: HIGH-key separator mismatch at branch page %u "
                         "sep %u", label, page_no, i);
                CHECK(0, msg); return 1;
            }
        }
        /* Trailing child. */
        if (trail_child == 0u) { CHECK(0, "validate: branch missing trailing child"); return 1; }
        if (validate_subtree(idx, trail_child, depth - 1u, child_max,
                             &child_h, label) != 0)
            return 1;
        if (child_h != first_h) {
            snprintf(msg, sizeof(msg),
                     "%s: unbalanced trailing child at branch page %u", label,
                     page_no);
            CHECK(0, msg); return 1;
        }
        rt_memcpy(maxkey_out, child_max, kl);   /* branch MAX = trailing MAX */
        *height_out = first_h + 1u;
        return 0;
    }
}

/* ----------------------------------------------------------------------- */
/* In-order collector (reuses the real ndx_inorder under test).             */
/* ----------------------------------------------------------------------- */
typedef struct {
    uint32_t n;
    int      overflow;
    uint32_t kl;
    uint8_t  keys[ML_NREC * ML_KEYLEN];
    uint32_t recnos[ML_NREC];
} io_collect;

static int io_visit(void *ctx, const uint8_t *key_data, uint32_t recno)
{
    io_collect *c = (io_collect *)ctx;
    if (c->n >= ML_NREC) { c->overflow = 1; return 0; }
    rt_memcpy(c->keys + c->n * c->kl, key_data, c->kl);
    c->recnos[c->n] = recno;
    c->n++;
    return 0;
}

/* ----------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    struct pal_host_cfg cfg;
    samir_pal_t *pal;
    const char  *tmpdir = (argc >= 2) ? argv[1] : "/tmp";
    char         ndx_path[512];
    int          rc;
    ndx_index   *idx = NULL;
    char         msg[192];

    (void)argv;

    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy   = 26u;
    cfg.date_mm   = 7u;
    cfg.date_dd   = 10u;
    cfg.heap_size = 4u * 1024u * 1024u;   /* build buffers + node reads */
    pal = pal_host_make(cfg);
    if (!pal) { fprintf(stderr, "FATAL: pal_host_make failed\n"); return 2; }

    snprintf(ndx_path, sizeof(ndx_path), "%s/initech_ndx_ml.ndx", tmpdir);
    remove(ndx_path);

    printf("test-ndx-multilevel: 3-level .ndx bulk build (initech-h5vg)\n");
    printf("  key_length=%u kpp=%u nrec=%u -> nleaf=20 -> 3 levels\n",
           ML_KEYLEN, ML_KPP, ML_NREC);

    /* ---- (1) BUILD: the RED-first leg. Old code / mutant: -NDX_ERR_PAGE_OVF. */
    rc = ndx_build(pal, ndx_path, (uint16_t)NDX_KEY_TYPE_CHAR,
                   (uint16_t)ML_KEYLEN, "MLKEY ", ML_NREC, ml_get_key, NULL);
    snprintf(msg, sizeof(msg),
             "ndx_build 3-level succeeds (rc=%d; -8 == PAGE_OVF is the OLD "
             "2-level ceiling / mutant)", rc);
    CHECK(rc == NDX_OK, msg);
    if (rc != NDX_OK) { pal_host_free(pal); return TEST_SUMMARY("test-ndx-multilevel"); }

    /* ---- (2) OPEN + header geometry. */
    rc = ndx_open(pal, ndx_path, &idx);
    CHECK(rc == NDX_OK && idx != NULL, "ndx_open rebuilt 3-level index");
    if (rc != NDX_OK || !idx) { pal_host_free(pal); return TEST_SUMMARY("test-ndx-multilevel"); }

    CHECK(ndx_key_length(idx) == (uint16_t)ML_KEYLEN, "key_length == 20");
    CHECK(ndx_keys_per_page(idx) == (uint16_t)ML_KPP, "keys_per_page == 18");
    {
        uint32_t total = ndx_total_pages(idx);
        uint32_t root  = ndx_root_page(idx);
        snprintf(msg, sizeof(msg), "root_page (%u) in [1,total=%u)", root, total);
        CHECK(root >= 1u && root < total, msg);
        /* nleaf=20 -> 1 hdr + 20 leaves + 2 L1 branches + 1 root = 24 pages. */
        snprintf(msg, sizeof(msg), "total_pages == 24 (got %u)", total);
        CHECK(total == 24u, msg);
        snprintf(msg, sizeof(msg), "root_page == 23 (got %u)", root);
        CHECK(root == 23u, msg);
    }

    /* ---- (3) STRUCTURAL validity: HIGH-key + balanced depth + >=3 levels. */
    {
        uint8_t  rootmax[ML_KEYLEN];
        uint32_t height = 0u;
        int      vrc = validate_subtree(idx, ndx_root_page(idx),
                                        ndx_total_pages(idx), rootmax, &height,
                                        "struct");
        CHECK(vrc == 0, "structural validity (HIGH-key separators + balanced depth)");
        snprintf(msg, sizeof(msg),
                 "tree is genuinely multi-level: height %u >= 3", height);
        CHECK(height >= 3u, msg);
    }

    /* ---- (4) IN-ORDER round-trip: strictly ascending, count == nrec, and the
     * (value,recno) mapping is exactly the reversed-permutation expectation. */
    {
        static io_collect io;
        uint32_t i;
        int asc_ok = 1, map_ok = 1;
        memset(&io, 0, sizeof(io));
        io.kl = (uint32_t)ndx_key_length(idx);
        rc = ndx_inorder(idx, io_visit, &io);
        CHECK(rc == NDX_OK, "ndx_inorder over 3-level tree");
        CHECK(io.overflow == 0, "in-order collector did not overflow");
        snprintf(msg, sizeof(msg), "in-order count == nrec (%u vs %u)",
                 io.n, ML_NREC);
        CHECK(io.n == ML_NREC, msg);

        for (i = 1u; i < io.n; i++)
            if (rt_memcmp(io.keys + (i - 1u) * io.kl,
                          io.keys + i * io.kl, io.kl) >= 0) { asc_ok = 0; break; }
        CHECK(asc_ok, "in-order keys strictly ascending across all leaves");

        /* Expected: in-order position i has value i (recno NREC - i). */
        for (i = 0u; i < io.n && map_ok; i++) {
            uint8_t want[ML_KEYLEN];
            render_key(i, want);
            if (rt_memcmp(io.keys + i * io.kl, want, io.kl) != 0) { map_ok = 0; break; }
            if (io.recnos[i] != (ML_NREC - i)) { map_ok = 0; break; }
        }
        CHECK(map_ok, "in-order (key,recno) == reversed-permutation expectation");
    }

    /* ---- (5) POINT LOOKUPS (ndx_seek) across all levels. */
    {
        uint32_t samples[] = { 1u, 2u, 18u, 19u, 20u, 100u, 171u, 172u,
                               342u, 343u };
        uint32_t si;
        int all_found = 1, all_recno = 1;
        for (si = 0u; si < sizeof(samples) / sizeof(samples[0]); si++) {
            uint32_t recno = samples[si];
            uint32_t value = value_for_recno(recno);
            uint8_t  kbuf[ML_KEYLEN];
            xb_val   kv;
            uint32_t got_recno = 0u;
            int      found = 0;
            render_key(value, kbuf);
            kv = xb_c((const char *)kbuf, (uint16_t)ML_KEYLEN);
            rc = ndx_seek(idx, &kv, /*set_exact=*/1, &got_recno, &found);
            if (rc != NDX_OK || !found) { all_found = 0; }
            if (got_recno != recno)     { all_recno = 0; }
        }
        CHECK(all_found, "ndx_seek finds every sampled present key (all 3 levels)");
        CHECK(all_recno, "ndx_seek resolves the correct recno for present keys");

        /* Absent key: a value above every stored value -> not found. */
        {
            uint8_t  kbuf[ML_KEYLEN];
            xb_val   kv;
            uint32_t got_recno = 7u;
            int      found = 1;
            render_key(999999u, kbuf);
            kv = xb_c((const char *)kbuf, (uint16_t)ML_KEYLEN);
            rc = ndx_seek(idx, &kv, 1, &got_recno, &found);
            CHECK(rc == NDX_OK && found == 0, "ndx_seek reports absent key as not-found (EOF)");
        }
    }

    /* ---- (6) HONEST FAIL-LOUD on the scoped-out incremental paths (Rule 2).
     * Multi-level incremental insert/delete are NOT implemented (unminted split
     * policy; see the worklog). On a >=3-level tree these MUST fail loud rather
     * than corrupt a branch page. Assert the guard returns -NDX_ERR_NOROOM. */
    ndx_close(idx);
    idx = NULL;
    {
        ndx_index *widx = NULL;
        rc = ndx_open_rw(pal, ndx_path, &widx);
        CHECK(rc == NDX_OK && widx != NULL, "ndx_open_rw 3-level index");
        if (rc == NDX_OK && widx) {
            uint8_t  kbuf[ML_KEYLEN];
            render_key(500u, kbuf);   /* a fresh interior value */
            rc = ndx_insert_key(widx, kbuf, ML_NREC + 1u);
            snprintf(msg, sizeof(msg),
                     "ndx_insert_key on 3-level tree fails loud NOROOM (rc=%d, "
                     "want -%d)", rc, NDX_ERR_NOROOM);
            CHECK(rc == -NDX_ERR_NOROOM, msg);

            /* delete an existing key -- must also fail loud, never misreport. */
            render_key(value_for_recno(171u), kbuf);
            rc = ndx_delete_key(widx, kbuf, 171u);
            snprintf(msg, sizeof(msg),
                     "ndx_delete_key on 3-level tree fails loud NOROOM (rc=%d, "
                     "want -%d)", rc, NDX_ERR_NOROOM);
            CHECK(rc == -NDX_ERR_NOROOM, msg);
            ndx_close(widx);
        }
    }

    remove(ndx_path);
    pal_host_free(pal);
    return TEST_SUMMARY("test-ndx-multilevel");
}
