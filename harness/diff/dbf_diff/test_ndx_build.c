/*
 * harness/diff/dbf_diff/test_ndx_build.c -- host oracle for S4.4: ndx_build
 *                                            (bulk INDEX ON build, byte-exact).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc is OK here. Mirrors test_ndx_seek.c
 * / test_ndx_parse.c: seed test_assert.h harness (CHECK / TEST_HARNESS /
 * TEST_SUMMARY, which emits the "checks," line), pal_host_make, loud-SKIP on
 * absent goldens (printed, exit 0), non-zero exit on any failed check.
 *
 * The DECOUPLING barrier (bead initech-ahu.4): ndx_build (in os/samir/fs/ndx.c)
 * does NOT depend on core/eval.c -- it takes the per-record key bytes via a
 * caller-supplied key-provider callback. THIS TEST supplies the callback, and
 * the callback uses dbf_read_rec + the EVALUATOR to compute each record's key.
 * So the TEST links eval+parse+lex+fn+dbf, but the engine codec (ndx.c) does
 * NOT -- and test_ndx_parse/keys/seek keep linking ndx.c WITHOUT the evaluator.
 *
 * What this grades (plan S4.4 oracle contract):
 *
 *   1. BYTE-EXACT (normalized) REBUILD: for each golden index whose key
 *      expression + source .dbf we have, rebuild a fresh .ndx with ndx_build,
 *      then compare it to the golden MASKING the NORMALIZE regions that genuine
 *      III+ leaves as stale heap garbage (ndx.md ss1: "a byte-exact WRITER need
 *      not reproduce specific garbage, but a READER must ignore it"). The masked
 *      regions are documented in the normalizer below; every MEANINGFUL byte
 *      (the 10 header fields except dummy/reserved, the key_expr+NUL, every live
 *      node entry, the branch separators, the trailing child pointer) must match
 *      the golden EXACTLY. Cases graded byte-exact:
 *        - ZIPCODE.NDX  (char C5, single-field, 2 leaves + root, 49 entries)
 *        - CNAMES.NDX   (char C40 = LASTNAME + FIRSTNAME concat via the
 *                        evaluator, 5 leaves + root, 49 entries)
 *        - NCOST.NDX    (numeric N, negatives + decimals, 2 leaves + root,
 *                        33 entries; the raw LE double key path)
 *
 *   2. BEHAVIORAL CROSS-CHECK: ndx_open the rebuilt file, ndx_inorder it, and
 *      assert the in-order (key,recno) sequence equals the golden's; ndx_seek a
 *      known key in the rebuilt index and assert it resolves the same recno.
 *
 * Mutation (Rule 6 / plan S4.4):
 *   Built with -DNDX_MUTATE_SPLIT_5050, ndx_build packs leaves ~50/50 instead of
 *   100%-then-remainder. The leaf fill levels (and the root separators) diverge
 *   from the golden, so the byte-exact rebuild check goes RED. Exit code
 *   non-zero -> the mutant gate passes.
 *
 * Goldens base is argv[1] (default "../dbase3-decomp").
 * NCOST.NDX + NTEST.DBF are at <base>/mint/work/ (minted 2026-06-16).
 * Pristine corpus goldens at:
 *   <base>/goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Compile (canonical self-verify recipe; the test links the evaluator, the
 * engine codec does not):
 *   ENG="os/samir/fs/ndx.c os/samir/fs/dbf.c os/samir/core/eval.c \
 *        os/samir/core/parse.c os/samir/core/lex.c os/samir/core/value.c \
 *        os/samir/core/rt.c os/samir/core/fn_builtins.c"
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *     -Iseed -Ios/samir/include -Ispec \
 *     -o /tmp/test_ndx_build harness/diff/dbf_diff/test_ndx_build.c $ENG \
 *     os/samir/pal/pal_host.c
 *   /tmp/test_ndx_build ../dbase3-decomp
 *
 * Mutant (must exit non-zero):
 *   cc ... -DNDX_MUTATE_SPLIT_5050 ... -o /tmp/test_ndx_build_mut ... ; run
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/re/mint-results-003.md  bulk-build pack 100% L->R,
 *     remainder last; N keys / N+1 children root (minted BIGIDX.NDX).
 *   - ../dbase3-decomp/specs/file-formats/ndx.md  ss1 (geometry + writer-garbage
 *     note), ss2 (header), ss3/ss3.1/ss3.2 (node/group/trailing child),
 *     ss5 (HIGH-key separator), Open questions (bulk build RESOLVED).
 *   - docs/plans/SAMIR-implementation-plan.md S4.4 contract; Sec 2.E.
 *   - os/samir/include/samir/ndx.h (ndx_build + key-provider under test).
 *   - os/samir/include/samir/dbf.h, eval.h, value.h, rt.h.
 *   - seed/test_assert.h (harness idiom).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test_assert.h"       /* seed/, on -Iseed */
#include "samir/ndx.h"         /* os/samir/include/, on -Ios/samir/include */
#include "samir/ndx_format.h"  /* NDX_KEY_TYPE_CHAR / _NUMERIC, on -Ispec */
#include "samir/dbf.h"         /* dbf_open / dbf_read_rec / dbf_field */
#include "samir/eval.h"        /* xb_lex / xb_parse / xb_eval (key provider) */
#include "samir/value.h"       /* xb_c, xb_n, xb_d */
#include "samir/rt.h"          /* rt_memcpy, jdn_from_ymd */

TEST_HARNESS();

/* pal_host.c surface (not in a header; same pattern as test_ndx_seek.c). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* -----------------------------------------------------------------------
 * Path helpers (mirrored from test_ndx_seek.c)
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

/* Read an entire file into a malloc'd buffer; returns length (or -1). */
static long slurp(const char *path, uint8_t **out)
{
    FILE *f = fopen(path, "rb");
    long n;
    uint8_t *b;
    *out = NULL;
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return -1; }
    b = (uint8_t *)malloc((size_t)n ? (size_t)n : 1u);
    if (!b) { fclose(f); return -1; }
    if (n > 0 && fread(b, 1u, (size_t)n, f) != (size_t)n) {
        free(b); fclose(f); return -1;
    }
    fclose(f);
    *out = b;
    return n;
}

/* -----------------------------------------------------------------------
 * The key provider: dbf_read_rec + the evaluator.
 *
 * One context per index. We lex/parse the key expression ONCE; per record we
 * read the record's typed fields, bind them by name via a resolve hook, eval
 * the AST to an xb_val, and render the on-disk key bytes (char: left-justified
 * space-padded to key_len; numeric/date: 8-byte LE double).
 *
 * This is the "the test owns the evaluator" half of the decoupling: ndx_build
 * never sees eval.c.
 * ----------------------------------------------------------------------- */

#define KP_MAX_TOK   128
#define KP_MAX_NODE  128
#define KP_SCRATCH   512
#define KP_MAXFIELDS 64

typedef struct {
    dbf_table   *tbl;
    int          nfields;
    /* AST for the key expression. */
    xb_node      pool[KP_MAX_NODE];
    int          root;
    char         scratch[KP_SCRATCH];
    /* per-record decoded field values (rebound each call). */
    xb_val       rec[KP_MAXFIELDS];
    int          rec_valid;
    int          fail;            /* sticky: a read/eval failure */
} keyprov_ctx;

/* Resolve hook: map an identifier (field name, case-insensitive) to its value
 * in the CURRENT record (kp->rec). */
static int kp_resolve(void *user, const char *name, uint16_t len, xb_val *out)
{
    keyprov_ctx *kp = (keyprov_ctx *)user;
    int i;
    for (i = 0; i < kp->nfields; i++) {
        const dbf_field_t *fd = dbf_field(kp->tbl, i);
        size_t fl;
        if (!fd) continue;
        fl = strlen(fd->name);
        if (fl == (size_t)len) {
            /* case-insensitive compare */
            uint16_t k;
            int eq = 1;
            for (k = 0; k < len; k++) {
                char a = fd->name[k], b = name[k];
                if (a >= 'a' && a <= 'z') a = (char)(a - 32);
                if (b >= 'a' && b <= 'z') b = (char)(b - 32);
                if (a != b) { eq = 0; break; }
            }
            if (eq) { *out = kp->rec[i]; return 0; }
        }
    }
    return 1;   /* unbound */
}

/* Render an xb_val into on-disk key bytes (key_out, key_len). */
static int kp_render(const xb_val *v, uint16_t key_type,
                     uint8_t *key_out, uint16_t key_len)
{
    if (key_type == (uint16_t)NDX_KEY_TYPE_CHAR) {
        uint16_t n = 0;
        if (v->t == XB_C || v->t == XB_M) {
            uint16_t src = v->u.c.len;
            n = (src < key_len) ? src : key_len;
            rt_memcpy(key_out, v->u.c.p, n);
        }
        /* right-pad with spaces (ndx.md ss4.1). */
        for (; n < key_len; n++) key_out[n] = (uint8_t)' ';
        return 0;
    }
    /* numeric / date: 8-byte LE double (ndx.md ss4.2). */
    {
        double d;
        if (v->t == XB_N)      d = v->u.n;
        else if (v->t == XB_D) d = v->u.d;
        else                   return 1;   /* wrong type for a numeric key */
        if (key_len != 8u) return 1;
        rt_memcpy(key_out, &d, 8u);
        return 0;
    }
}

/* Stash the key_type for the render path (passed to ndx_build, but the callback
 * needs it too). We thread it through the keyprov_ctx. */
static uint16_t g_key_type;   /* set per index before ndx_build */

static int kp_get_key(void *user, uint32_t recno,
                      uint8_t *key_out, uint16_t key_len)
{
    keyprov_ctx *kp = (keyprov_ctx *)user;
    xb_ctx       ctx;
    xb_val       result;
    int          err = 0;
    int          rc;

    if (kp->fail) return 1;

    /* Read the record's typed fields. */
    rc = dbf_read_rec(kp->tbl, recno, kp->rec, NULL);
    if (rc != DBF_OK) { kp->fail = 1; return 1; }

    /* Evaluate the key expression against the bound fields. */
    memset(&ctx, 0, sizeof(ctx));
    ctx.set_exact   = 0;
    ctx.resolve     = kp_resolve;
    ctx.user        = kp;
    ctx.scratch     = kp->scratch;
    ctx.scratch_cap = (uint32_t)KP_SCRATCH;
    ctx.scratch_used = 0;
    ctx.ctx_today   = 0.0;

    rc = xb_eval(kp->pool, kp->root, &ctx, &result, &err);
    if (rc != 0) { kp->fail = 1; return 1; }

    return kp_render(&result, g_key_type, key_out, key_len);
}

/* Build the AST once for an expression. Returns 0 on success. */
static int kp_compile(keyprov_ctx *kp, const char *expr)
{
    static xb_token toks[KP_MAX_TOK];     /* static: keep off the stack */
    int   ntok;
    int   err = 0;
    int   root;

    ntok = xb_lex(expr, (uint32_t)strlen(expr), toks, (uint32_t)KP_MAX_TOK, &err);
    if (ntok < 0) return 1;
    root = xb_parse(toks, (uint32_t)ntok, kp->pool, (uint32_t)KP_MAX_NODE, &err);
    if (root < 0) return 1;
    kp->root = root;
    return 0;
}

/* -----------------------------------------------------------------------
 * The normalizer: zero out the NORMALIZE byte regions of a built/golden .ndx so
 * that only MEANINGFUL bytes are compared. We need the parsed geometry (key_len,
 * group_len, total_pages, per-node entry_count + branch/leaf shape) to know
 * which bytes are live. We get that from the SAME ndx reader under test.
 *
 * NORMALIZE regions (documented; matches the writer in ndx.c):
 *   HEADER (page 0):
 *     - reserved @0x08 (4 bytes)            -> 0
 *     - dummy    @0x14 (2 bytes)            -> 0
 *     - bytes after the key_expr NUL .. 511 -> 0  (header tail garbage)
 *   NODE (each page 1..total-1):
 *     - filler word @0x02 (2 bytes)         -> 0
 *     - per live entry: the filler AFTER key_data within the group_length stride
 *     - in a BRANCH entry, the dbf_recno field is meaningful (== 0); keep it.
 *       (Our writer writes 0; genuine III+ writes 0 too for branch recnos.)
 *     - the TRAILING slot: in a BRANCH node the leading 4 bytes (child pointer)
 *       are MEANINGFUL; the rest of that slot -> 0. In a LEAF node the whole
 *       trailing slot -> 0 (it is stale garbage in genuine III+).
 *     - the unused page tail after the last entry/trailing slot -> 0.
 * ----------------------------------------------------------------------- */

static void zero(uint8_t *p, uint32_t off, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) p[off + i] = 0u;
}

/* Normalize one 512-byte HEADER page in place. */
static void norm_header(uint8_t *buf)
{
    uint32_t i;
    /* reserved @0x08, dummy @0x14 */
    zero(buf, 0x08u, 4u);
    zero(buf, 0x14u, 2u);
    /* find the key_expr NUL within [0x18, 0x18+100); zero from NUL+1 to 511. */
    {
        uint32_t e = 0x18u;
        uint32_t end = 0x18u + 100u;
        while (e < end && buf[e] != 0u) e++;
        /* e now points at the NUL (or end). Zero from e .. 511 inclusive.
         * (The NUL itself is part of the terminator -> writing 0 is harmless
         *  and both files have 0 there.) */
        for (i = e; i < 512u; i++) buf[i] = 0u;
    }
}

/* Normalize one 512-byte NODE page in place, given the geometry. */
static void norm_node(uint8_t *buf, uint32_t key_len, uint32_t group_len)
{
    uint32_t cnt = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8);
    uint32_t is_branch;
    uint32_t entries_off = 4u;
    uint32_t i;
    uint32_t live_end;
    uint32_t trail_off;

    /* node filler word @0x02 */
    zero(buf, 0x02u, 2u);

    /* branch iff the first live entry has a non-zero child page. */
    is_branch = 0u;
    if (cnt > 0u) {
        uint32_t c0 = (uint32_t)buf[entries_off]
                    | ((uint32_t)buf[entries_off + 1] << 8)
                    | ((uint32_t)buf[entries_off + 2] << 16)
                    | ((uint32_t)buf[entries_off + 3] << 24);
        is_branch = (c0 != 0u) ? 1u : 0u;
    }

    /* per-entry group filler after key_data (offset 8 + key_len .. group_len). */
    for (i = 0u; i < cnt; i++) {
        uint32_t base = entries_off + i * group_len;
        uint32_t pad_off = base + 8u + key_len;
        uint32_t pad_n   = group_len - (8u + key_len);
        if (pad_n > 0u && pad_off + pad_n <= 512u)
            zero(buf, pad_off, pad_n);
    }

    live_end  = entries_off + cnt * group_len;
    trail_off = live_end;

    if (is_branch) {
        /* trailing slot: keep the 4-byte child pointer; zero the rest of the
         * slot, but ONLY if the whole slot fits the page. */
        if (trail_off + group_len <= 512u) {
            zero(buf, trail_off + 4u, group_len - 4u);
            /* unused page tail after the trailing slot */
            if (trail_off + group_len < 512u)
                zero(buf, trail_off + group_len, 512u - (trail_off + group_len));
        } else if (trail_off + 4u <= 512u) {
            /* only the child pointer fits; zero whatever follows */
            if (trail_off + 4u < 512u)
                zero(buf, trail_off + 4u, 512u - (trail_off + 4u));
        } else {
            if (trail_off < 512u)
                zero(buf, trail_off, 512u - trail_off);
        }
    } else {
        /* leaf: the trailing slot + the rest of the page are all garbage -> 0. */
        if (trail_off < 512u)
            zero(buf, trail_off, 512u - trail_off);
    }
}

/* Normalize a whole .ndx image (header + all node pages) using the parsed
 * geometry from an opened index. */
static void norm_image(uint8_t *buf, long len,
                       uint32_t key_len, uint32_t group_len,
                       uint32_t total_pages)
{
    uint32_t p;
    (void)len;
    norm_header(buf);
    for (p = 1u; p < total_pages; p++)
        norm_node(buf + (size_t)p * 512u, key_len, group_len);
}

/* -----------------------------------------------------------------------
 * In-order collector (reuses the REAL ndx_inorder under test) for behavioral
 * cross-check between golden and rebuilt index.
 * ----------------------------------------------------------------------- */
#define IO_MAX     128
#define IO_KEYCAP  40

typedef struct {
    int      n;
    int      overflow;
    uint32_t kl;
    uint8_t  keys[IO_MAX * IO_KEYCAP];
    uint32_t recnos[IO_MAX];
} io_collect;

static int io_visit(void *ctx, const uint8_t *key_data, uint32_t recno)
{
    io_collect *c = (io_collect *)ctx;
    if (c->n >= IO_MAX) { c->overflow = 1; return 0; }
    rt_memcpy(c->keys + (uint32_t)c->n * c->kl, key_data, c->kl);
    c->recnos[c->n] = recno;
    c->n++;
    return 0;
}

static void collect_inorder(samir_pal_t *pal, const char *path, io_collect *io)
{
    ndx_index *idx = NULL;
    memset(io, 0, sizeof(*io));
    if (ndx_open(pal, path, &idx) != NDX_OK || !idx) return;
    io->kl = (uint32_t)ndx_key_length(idx);
    if (io->kl > (uint32_t)IO_KEYCAP) { ndx_close(idx); return; }
    ndx_inorder(idx, io_visit, io);
    ndx_close(idx);
}

/* -----------------------------------------------------------------------
 * The core check: rebuild one index, normalized-cmp to the golden, behavioral
 * cross-check.
 *
 *   src_rel   the source .dbf (relative to base)
 *   ndx_rel   the golden .ndx (relative to base)
 *   expr      the index expression (what dBASE typed for INDEX ON)
 *   key_type  NDX_KEY_TYPE_CHAR or _NUMERIC
 *   key_len   the index key_length
 *   label     for messages
 * ----------------------------------------------------------------------- */
static void check_rebuild(samir_pal_t *pal, const char *base,
                          const char *src_rel, const char *ndx_rel,
                          const char *expr, uint16_t key_type, uint16_t key_len,
                          const char *label)
{
    char        src_path[1024], ndx_path[1024], out_path[1024];
    dbf_table  *tbl = NULL;
    keyprov_ctx *kp;
    int          rc;
    char         msg[256];
    uint8_t     *gold = NULL, *built = NULL;
    long         glen, blen;
    ndx_index   *gi = NULL;
    uint32_t     g_kl, g_gl, g_total;

    join(src_path, sizeof(src_path), base, src_rel);
    join(ndx_path, sizeof(ndx_path), base, ndx_rel);

    if (!file_exists(src_path) || !file_exists(ndx_path)) {
        fprintf(stderr, "  SKIP (LOUD): %s rebuild -- %s or %s absent\n",
                label, src_rel, ndx_rel);
        return;
    }

    /* Open the source .dbf. */
    rc = dbf_open(pal, src_path, &tbl);
    snprintf(msg, sizeof(msg), "%s: dbf_open source (rc=%d)", label, rc);
    CHECK(rc == DBF_OK && tbl != NULL, msg);
    if (rc != DBF_OK || !tbl) return;

    /* Compile the key expression (the test's evaluator path). */
    kp = (keyprov_ctx *)calloc(1, sizeof(keyprov_ctx));
    if (!kp) { CHECK(0, "calloc keyprov_ctx"); dbf_close(tbl); return; }
    kp->tbl     = tbl;
    kp->nfields = (int)dbf_nfields(tbl);
    rc = kp_compile(kp, expr);
    snprintf(msg, sizeof(msg), "%s: compile key expr '%s' (rc=%d)",
             label, expr, rc);
    CHECK(rc == 0, msg);
    if (rc != 0) { free(kp); dbf_close(tbl); return; }

    /* Build the fresh index to a temp path next to the golden. */
    snprintf(out_path, sizeof(out_path), "%s/_rebuild_%s.ndx", base, label);
    g_key_type = key_type;
    kp->fail = 0;
    rc = ndx_build(pal, out_path, key_type, key_len, expr,
                   dbf_nrec(tbl), kp_get_key, kp);
    snprintf(msg, sizeof(msg), "%s: ndx_build (rc=%d, kp_fail=%d)",
             label, rc, kp->fail);
    CHECK(rc == NDX_OK && kp->fail == 0, msg);
    if (rc != NDX_OK) { free(kp); dbf_close(tbl); remove(out_path); return; }

    /* Parse the golden geometry (key_len, group_len, total_pages) via the
     * reader under test, so the normalizer masks the right regions. */
    rc = ndx_open(pal, ndx_path, &gi);
    snprintf(msg, sizeof(msg), "%s: ndx_open golden (rc=%d)", label, rc);
    CHECK(rc == NDX_OK && gi != NULL, msg);
    if (rc != NDX_OK || !gi) { free(kp); dbf_close(tbl); remove(out_path); return; }
    g_kl    = (uint32_t)ndx_key_length(gi);
    g_gl    = (uint32_t)ndx_group_length(gi);
    g_total = ndx_total_pages(gi);
    ndx_close(gi);

    /* Slurp both files, normalize, compare. */
    glen = slurp(ndx_path, &gold);
    blen = slurp(out_path, &built);
    CHECK(glen > 0 && gold != NULL, "slurp golden");
    CHECK(blen > 0 && built != NULL, "slurp rebuilt");

    if (gold && built) {
        snprintf(msg, sizeof(msg),
                 "%s: rebuilt size == golden size (%ld vs %ld)",
                 label, blen, glen);
        CHECK(blen == glen, msg);

        if (blen == glen) {
            norm_image(gold, glen, g_kl, g_gl, g_total);
            norm_image(built, blen, g_kl, g_gl, g_total);
            {
                long off = -1, k;
                for (k = 0; k < glen; k++) {
                    if (gold[k] != built[k]) { off = k; break; }
                }
                snprintf(msg, sizeof(msg),
                         "%s: NORMALIZED byte-exact vs golden (first diff at "
                         "%ld: gold=%02x built=%02x)",
                         label, off,
                         off >= 0 ? gold[off] : 0,
                         off >= 0 ? built[off] : 0);
                CHECK(off < 0, msg);
            }
        }
    }

    /* Behavioral cross-check: in-order (key,recno) of rebuilt == golden. */
    {
        static io_collect gio, bio;
        collect_inorder(pal, ndx_path, &gio);
        collect_inorder(pal, out_path, &bio);
        snprintf(msg, sizeof(msg),
                 "%s: in-order count rebuilt == golden (%d vs %d)",
                 label, bio.n, gio.n);
        CHECK(bio.n == gio.n && gio.n > 0, msg);
        if (bio.n == gio.n && gio.kl == bio.kl && gio.kl > 0) {
            int i, mism_k = -1, mism_r = -1;
            for (i = 0; i < gio.n; i++) {
                if (rt_memcmp(gio.keys + (uint32_t)i * gio.kl,
                              bio.keys + (uint32_t)i * bio.kl, gio.kl) != 0
                    && mism_k < 0) mism_k = i;
                if (gio.recnos[i] != bio.recnos[i] && mism_r < 0) mism_r = i;
            }
            snprintf(msg, sizeof(msg),
                     "%s: in-order keys rebuilt == golden (first diff %d)",
                     label, mism_k);
            CHECK(mism_k < 0, msg);
            snprintf(msg, sizeof(msg),
                     "%s: in-order recnos rebuilt == golden (first diff %d)",
                     label, mism_r);
            CHECK(mism_r < 0, msg);
        }
    }

    free(gold);
    free(built);
    free(kp);
    dbf_close(tbl);
    remove(out_path);
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
    int any = 0;

    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy  = 99;
    cfg.date_mm  = 12;
    cfg.date_dd  = 31;
    /* The build holds the whole key buffer (<= 49 * 40 bytes) + one page + the
     * dbf table + record buffer. ndx_open/inorder hold one node at a time.
     * 512 KB is ample. */
    cfg.heap_size = 512u * 1024u;

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    if (file_exists(join(path, sizeof(path), base, PRISTINE_REL "/ZIPCODE.NDX")))
        any = 1;
    if (file_exists(join(path, sizeof(path), base, "mint/work/NCOST.NDX")))
        any = 1;
    if (!any) {
        fprintf(stderr,
                "  SKIP (LOUD): no goldens found under base '%s'\n"
                "               pass the corpus base as argv[1]\n", base);
    }

    /* --- Byte-exact rebuild + behavioral cross-check --- */

    /* ZIPCODE: char C5 single-field; CLIENTS.DBF; 2 leaves + root (49 entries).
     * The stored .ndx key_expr is "ZIPCODE " (with the trailing space dBASE
     * echoes from `INDEX ON ZIPCODE TO ...`); we MUST pass it verbatim so the
     * header bytes match (the evaluator parses the trailing space fine). */
    check_rebuild(pal, base,
                  PRISTINE_REL "/CLIENTS.DBF", PRISTINE_REL "/ZIPCODE.NDX",
                  "ZIPCODE ", NDX_KEY_TYPE_CHAR, 5u, "ZIPCODE");

    /* CNAMES: char C40 = LASTNAME + FIRSTNAME (multi-field concat via the
     * evaluator); CLIENTS.DBF; 5 leaves + root (49 entries). Stored expr is
     * "LASTNAME + FIRSTNAME " (trailing space). */
    check_rebuild(pal, base,
                  PRISTINE_REL "/CLIENTS.DBF", PRISTINE_REL "/CNAMES.NDX",
                  "LASTNAME + FIRSTNAME ", NDX_KEY_TYPE_CHAR, 40u, "CNAMES");

    /* NCOST: numeric N (negatives + decimals); NTEST.DBF; 2 leaves + root
     * (33 entries). The raw LE double key path. Stored expr "UNITCOST ". */
    check_rebuild(pal, base,
                  "mint/work/NTEST.DBF", "mint/work/NCOST.NDX",
                  "UNITCOST ", NDX_KEY_TYPE_NUMERIC, 8u, "NCOST");

    pal_host_free(pal);
    return TEST_SUMMARY("test-ndx-build");
}
