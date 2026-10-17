/*
 * harness/diff/dbf_diff/test_ndx_incremental.c -- host oracle for the
 *   MULTI-LEVEL .ndx B-tree: bulk INDEX ON layout at every depth and the
 *   one-key-at-a-time insert / REPLACE policy, graded BYTE-EXACT (normalized)
 *   against real dBASE III PLUS 1.1 output (bead initech-7tj3).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc is OK here. seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY, non-zero exit on any failure).
 * Links the engine codec only (ndx.c + value + rt + pal_host); keys come from
 * a synthetic provider that renders exactly the keys the mint scripts stored.
 *
 * THE GOLDENS (Law 2 -- independent, never our artifact): minted 2026-10-17
 * by DBASE.EXE 1.1 under dosbox-x from ../dbase3-decomp/mint/work/M006A..D.PRG
 * (re/mint-results-006.md s2). Every key is a zero-padded decimal string.
 *   A00            INDEX ON K (C20, kpp 18) over 343 records, K = 10*recno:
 *                  the real 3-level layout (root p22 = [branch p20, leaf p21]).
 *   I01..I17       then APPEND one record at a time with the index open,
 *   J01..J19, K01    keys 15,25..175 / 180j+5 / 99990 (recnos 344..380).
 *   D01..D18       then REPLACE K of recnos 37..54 with 90000+10j.
 *   B001..B130     INDEX ON K (C100, kpp 4) over N records, N = 1..130.
 *   U000..U130     an EMPTY C100 index, then APPEND 10*n (n = 1..130).
 *   R000..R130     an EMPTY C100 index, then APPEND (53n mod 131)*10+5.
 *   M001..M060     then REPLACE K of recno n (n = 1..60) with 900000+10n.
 *   E01..E08       then REPLACE the keys 475 485 505 515 15 295 325 355 with
 *                  910000+10j (empties a 1-separator branch, the leftmost
 *                  leaf and a trailing subtree).
 *
 * NORMALIZED BYTE COMPARE (the meaningful bytes; ndx.md ss1 writer-garbage
 * note -- genuine III+ leaves stale heap bytes everywhere else):
 *   - file size, and the header root_page, total_pages, key_length,
 *     keys_per_page, key_type, group_length, unique flag, key_expr to its NUL;
 *   - every page REACHABLE from the golden root (walked by THIS file's own
 *     raw-byte reader, not ndx.c's): entry_count, every live group's
 *     child_page + dbf_recno + key bytes, and a branch's trailing child.
 *   Unreachable pages (emptied by deletes, never reused) are not compared.
 * Plus a behavioral leg: ndx_inorder over our file visits exactly the minted
 * key count in ascending order after every REPLACE step (E02 holds a branch
 * with 0 separators -- its trailing child sits in group 0).
 *
 * MUTANTS (Rule 6; Makefile test-ndx-incremental-mutant, each RED by name):
 *   NDX_MUTATE_LEAF_SPLIT_LOW        leaf split keeps (kpp+1)/2 on the left
 *   NDX_MUTATE_BULK_NO_ELIDE         a 1-child pending node becomes a branch
 *   NDX_MUTATE_DELETE_NO_SEP_UPDATE  a deleted HIGH key leaves a stale separator
 *   NDX_MUTATE_ZERO_SEP_LEAF         a 0-separator branch read as an empty leaf
 *
 * ASCII-clean (Rule 12). Builds land in build/ndx_incr/ (Rule 11: no host
 * path is written into an artifact). Goldens are opened read-only.
 */

#include "../../oracle_policy.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "test_assert.h"
#include "samir/ndx.h"
#include "samir/ndx_format.h"
#include "samir/value.h"
#include "samir/rt.h"

TEST_HARNESS();

struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

static const char *g_base;
static char        g_msg[512];
static int         g_first_red_printed;

/* ---- keys ------------------------------------------------------------- */
static void render(uint32_t v, uint16_t w, uint8_t *out)
{
    char dec[16];
    int  n = snprintf(dec, sizeof dec, "%u", (unsigned)v);
    int  i;
    memset(out, '0', w);
    for (i = 0; i < n; i++)
        out[w - (uint16_t)n + (uint16_t)i] = (uint8_t)dec[i];
}

struct bulk_ctx { uint16_t w; const uint32_t *vals; };
static int bulk_key(void *user, uint32_t recno, uint8_t *out, uint16_t len)
{
    struct bulk_ctx *c = (struct bulk_ctx *)user;
    if (len != c->w) return 1;
    render(c->vals[recno - 1u], len, out);
    return 0;
}

/* ---- the independent raw-byte compare --------------------------------- */
static long slurp(const char *path, uint8_t **out)
{
    FILE *f = fopen(path, "rb");
    long  n;
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    *out = (uint8_t *)malloc((size_t)(n > 0 ? n : 1));
    if (!*out || fread(*out, 1, (size_t)n, f) != (size_t)n) { fclose(f); return -1; }
    fclose(f);
    return n;
}
static uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

static int walk(const uint8_t *g, const uint8_t *o, long size, uint32_t page,
                uint32_t kl, uint32_t gl, uint32_t budget, char *why, size_t wn)
{
    const uint8_t *gp, *op;
    uint32_t n, s;
    if (budget == 0u) { snprintf(why, wn, "golden tree deeper than its page count"); return 1; }
    if ((long)(page + 1u) * 512L > size) { snprintf(why, wn, "page %u outside the file", (unsigned)page); return 1; }
    gp = g + page * 512u;
    op = o + page * 512u;
    n = le16(gp);
    if (le16(op) != n) { snprintf(why, wn, "page %u entry_count gold %u ours %u", (unsigned)page, (unsigned)n, (unsigned)le16(op)); return 1; }
    if (le32(gp + 4) == 0u) {               /* leaf */
        for (s = 0u; s < n; s++) {
            const uint8_t *a = gp + 4u + s * gl, *b = op + 4u + s * gl;
            if (le32(b) != 0u) { snprintf(why, wn, "leaf page %u slot %u ours has child %u", (unsigned)page, (unsigned)s, (unsigned)le32(b)); return 1; }
            if (le32(a + 4) != le32(b + 4)) { snprintf(why, wn, "leaf page %u slot %u recno gold %u ours %u", (unsigned)page, (unsigned)s, (unsigned)le32(a + 4), (unsigned)le32(b + 4)); return 1; }
            if (memcmp(a + 8, b + 8, kl) != 0) { snprintf(why, wn, "leaf page %u slot %u key differs", (unsigned)page, (unsigned)s); return 1; }
        }
        return 0;
    }
    for (s = 0u; s <= n; s++) {              /* branch: n separators + trailing */
        const uint8_t *a = gp + 4u + s * gl, *b = op + 4u + s * gl;
        if (le32(a) != le32(b)) { snprintf(why, wn, "branch page %u slot %u child gold %u ours %u", (unsigned)page, (unsigned)s, (unsigned)le32(a), (unsigned)le32(b)); return 1; }
        if (s < n && memcmp(a + 8, b + 8, kl) != 0) { snprintf(why, wn, "branch page %u separator %u differs", (unsigned)page, (unsigned)s); return 1; }
        if (walk(g, o, size, le32(a), kl, gl, budget - 1u, why, wn)) return 1;
    }
    return 0;
}

/* Grade our file OURS against golden <base>/mint/work/GOLD; LABEL names it. */
static void grade(const char *ours, const char *gold, const char *label)
{
    char     gpath[1024], why[256];
    uint8_t *g = NULL, *o = NULL;
    long     gn, on;
    int      bad = 0;
    snprintf(gpath, sizeof gpath, "%s/mint/work/%s", g_base, gold);
    gn = slurp(gpath, &g);
    if (gn < 0) oracle_failure("golden %s unreadable (the M006 mint is incomplete)\n", gpath);
    on = slurp(ours, &o);
    why[0] = '\0';
    if (on != gn) { snprintf(why, sizeof why, "file size gold %ld ours %ld", gn, on); bad = 1; }
    else if (memcmp(g, o, 8) != 0) { snprintf(why, sizeof why, "header root/total gold %u/%u ours %u/%u", (unsigned)le32(g), (unsigned)le32(g + 4), (unsigned)le32(o), (unsigned)le32(o + 4)); bad = 1; }
    else if (memcmp(g + 12, o + 12, 8) != 0 || memcmp(g + 22, o + 22, 2) != 0) { snprintf(why, sizeof why, "header geometry/unique differs"); bad = 1; }
    else if (strncmp((const char *)g + 24, (const char *)o + 24, 100) != 0) { snprintf(why, sizeof why, "key_expr gold '%.20s' ours '%.20s'", (const char *)g + 24, (const char *)o + 24); bad = 1; }
    else bad = walk(g, o, gn, le32(g), le16(g + 12), le16(g + 18), le32(g + 4), why, sizeof why);
    snprintf(g_msg, sizeof g_msg, "%s: normalized byte-exact vs real dBASE %s%s%s", label, gold, bad ? " -- " : "", why);
    if (bad && !g_first_red_printed) { printf("FIRST RED %s\n", g_msg); g_first_red_printed = 1; }
    CHECK(!bad, g_msg);
    free(g);
    free(o);
}

/* ---- behavioral in-order ---------------------------------------------- */
struct io_ctx { uint32_t n; uint8_t prev[128]; uint16_t kl; int asc; };
static int io_visit(void *ctx, const uint8_t *key, uint32_t recno)
{
    struct io_ctx *c = (struct io_ctx *)ctx;
    (void)recno;
    if (c->n > 0u && memcmp(c->prev, key, c->kl) >= 0) c->asc = 0;
    memcpy(c->prev, key, c->kl);
    c->n++;
    return 0;
}
static void inorder_check(samir_pal_t *pal, const char *path, uint16_t kl,
                          uint32_t want, const char *label)
{
    ndx_index    *ix = NULL;
    struct io_ctx c;
    int           rc;
    memset(&c, 0, sizeof c);
    c.kl = kl;
    c.asc = 1;
    rc = ndx_open(pal, path, &ix);
    CHECK(rc == NDX_OK, "ndx_open for the in-order leg");
    if (rc != NDX_OK) return;
    rc = ndx_inorder(ix, io_visit, &c);
    snprintf(g_msg, sizeof g_msg, "%s in-order: rc %d, %u keys (want %u), ascending %d",
             label, rc, (unsigned)c.n, (unsigned)want, c.asc);
    if ((rc != NDX_OK || c.n != want || !c.asc) && !g_first_red_printed) { printf("FIRST RED %s\n", g_msg); g_first_red_printed = 1; }
    CHECK(rc == NDX_OK && c.n == want && c.asc, g_msg);
    ndx_close(ix);
}

/* ---- drivers ---------------------------------------------------------- */
static void build(samir_pal_t *pal, const char *path, uint16_t w,
                  const uint32_t *vals, uint32_t n)
{
    struct bulk_ctx c;
    int rc;
    c.w = w;
    c.vals = vals;
    rc = ndx_build(pal, path, NDX_KEY_TYPE_CHAR, w, "K ", n, bulk_key, &c);
    snprintf(g_msg, sizeof g_msg, "ndx_build %u keys (rc %d)", (unsigned)n, rc);
    CHECK(rc == NDX_OK, g_msg);
}
static ndx_index *open_rw(samir_pal_t *pal, const char *path)
{
    ndx_index *ix = NULL;
    int rc = ndx_open_rw(pal, path, &ix);
    CHECK(rc == NDX_OK && ix, "ndx_open_rw our index");
    return ix;
}
static void ins(ndx_index *ix, uint16_t w, uint32_t v, uint32_t recno)
{
    uint8_t k[128];
    int rc;
    render(v, w, k);
    rc = ndx_insert_key(ix, k, recno);
    snprintf(g_msg, sizeof g_msg, "ndx_insert_key %u recno %u (rc %d)", (unsigned)v, (unsigned)recno, rc);
    CHECK(rc == NDX_OK, g_msg);
}
static void mov(ndx_index *ix, uint16_t w, uint32_t from, uint32_t to, uint32_t recno)
{
    uint8_t a[128], b[128];
    int rc;
    render(from, w, a);
    render(to, w, b);
    rc = ndx_update_key(ix, a, b, recno);
    snprintf(g_msg, sizeof g_msg, "ndx_update_key %u -> %u recno %u (rc %d)", (unsigned)from, (unsigned)to, (unsigned)recno, rc);
    CHECK(rc == NDX_OK, g_msg);
}

#define OUT "build/ndx_incr"

/* Close (flush) our index, grade it, reopen it read-write: the mint scripts
 * also closed the index (USE) before every COPY FILE snapshot. */
static ndx_index *step(samir_pal_t *pal, ndx_index *ix, const char *path,
                       const char *gold)
{
    ndx_close(ix);
    grade(path, gold, gold);
    return open_rw(pal, path);
}

int main(int argc, char **argv)
{
    struct pal_host_cfg cfg;
    samir_pal_t *pal;
    static uint32_t vals[400];
    char   probe[1024], lab[32], gold[32];
    uint32_t i, j, recno;
    ndx_index *ix;

    g_base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    snprintf(probe, sizeof probe, "%s/mint/work/E08.NDX", g_base);
    {
        FILE *f = fopen(probe, "rb");
        if (!f) {
            oracle_missing("DBASE3_DECOMP", "the M006 multi-level NDX mint is absent (%s)\n", probe);
            return 0;
        }
        fclose(f);
    }
    mkdir("build", 0777);
    mkdir(OUT, 0777);
    memset(&cfg, 0, sizeof cfg);
    cfg.date_yy = 86; cfg.date_mm = 7; cfg.date_dd = 24;
    cfg.heap_size = 1024u * 1024u;
    pal = pal_host_make(cfg);
    if (!pal) { fprintf(stderr, "FATAL: pal_host_make\n"); return 2; }

    /* (1) The bulk INDEX ON layout at every record count 1..130 (C100). */
    for (i = 0u; i < 130u; i++) vals[i] = 10u * (i + 1u);
    for (j = 1u; j <= 130u; j++) {
        snprintf(gold, sizeof gold, "B%03u.NDX", (unsigned)j);
        build(pal, OUT "/b.ndx", 100u, vals, j);
        grade(OUT "/b.ndx", gold, gold);
    }

    /* (2) A00 (C20, 3 levels) and the one-key-at-a-time C20 series. */
    for (i = 0u; i < 343u; i++) vals[i] = 10u * (i + 1u);
    build(pal, OUT "/a.ndx", 20u, vals, 343u);
    grade(OUT "/a.ndx", "A00.NDX", "A00");
    ix = open_rw(pal, OUT "/a.ndx");
    if (ix) {
        recno = 344u;
        for (j = 1u; j <= 17u; j++) {
            ins(ix, 20u, 10u * j + 5u, recno++);
            snprintf(gold, sizeof gold, "I%02u.NDX", (unsigned)j);
            if (!(ix = step(pal, ix, OUT "/a.ndx", gold))) break;
        }
        for (j = 1u; j <= 19u; j++) {
            ins(ix, 20u, 180u * j + 5u, recno++);
            snprintf(gold, sizeof gold, "J%02u.NDX", (unsigned)j);
            if (!(ix = step(pal, ix, OUT "/a.ndx", gold))) break;
        }
        if (ix) ins(ix, 20u, 99990u, recno++);
        if (ix) ix = step(pal, ix, OUT "/a.ndx", "K01.NDX");
        for (j = 1u; ix && j <= 18u; j++) {
            mov(ix, 20u, 10u * (36u + j), 90000u + 10u * j, 36u + j);
            snprintf(gold, sizeof gold, "D%02u.NDX", (unsigned)j);
            ix = step(pal, ix, OUT "/a.ndx", gold);
        }
        if (ix) ndx_close(ix);
        inorder_check(pal, OUT "/a.ndx", 20u, 380u, "D18");
    }

    /* (3) U: an empty C100 index grown by ascending APPENDs. */
    build(pal, OUT "/u.ndx", 100u, vals, 0u);
    grade(OUT "/u.ndx", "U000.NDX", "U000");
    ix = open_rw(pal, OUT "/u.ndx");
    if (ix) {
        for (j = 1u; j <= 130u; j++) {
            ins(ix, 100u, 10u * j, j);
            snprintf(gold, sizeof gold, "U%03u.NDX", (unsigned)j);
            if (!(ix = step(pal, ix, OUT "/u.ndx", gold))) break;
        }
        if (ix) ndx_close(ix);
    }

    /* (4) R: a permutation; M and E: key moves on the 4-level tree. */
    build(pal, OUT "/r.ndx", 100u, vals, 0u);
    grade(OUT "/r.ndx", "R000.NDX", "R000");
    ix = open_rw(pal, OUT "/r.ndx");
    if (ix) {
        static const uint32_t ev[8] = { 475u, 485u, 505u, 515u, 15u, 295u, 325u, 355u };
        for (j = 1u; j <= 130u; j++) {
            vals[j] = ((53u * j) % 131u) * 10u + 5u;
            ins(ix, 100u, vals[j], j);
            snprintf(gold, sizeof gold, "R%03u.NDX", (unsigned)j);
            if (!(ix = step(pal, ix, OUT "/r.ndx", gold))) break;
        }
        for (j = 1u; ix && j <= 60u; j++) {
            mov(ix, 100u, vals[j], 900000u + 10u * j, j);
            vals[j] = 900000u + 10u * j;
            snprintf(gold, sizeof gold, "M%03u.NDX", (unsigned)j);
            ix = step(pal, ix, OUT "/r.ndx", gold);
        }
        for (j = 1u; ix && j <= 8u; j++) {
            uint32_t r = 0u, k;
            for (k = 1u; k <= 130u; k++) if (vals[k] == ev[j - 1u]) r = k;
            CHECK(r != 0u, "E-series key located");
            mov(ix, 100u, ev[j - 1u], 910000u + 10u * j, r);
            vals[r] = 910000u + 10u * j;
            snprintf(gold, sizeof gold, "E%02u.NDX", (unsigned)j);
            snprintf(lab, sizeof lab, "E%02u", (unsigned)j);
            ndx_close(ix);
            grade(OUT "/r.ndx", gold, gold);
            inorder_check(pal, OUT "/r.ndx", 100u, 130u, lab);
            ix = open_rw(pal, OUT "/r.ndx");
            if (!ix) break;
        }
        if (ix) ndx_close(ix);
    }

    pal_host_free(pal);
    return TEST_SUMMARY("test-ndx-incremental");
}
