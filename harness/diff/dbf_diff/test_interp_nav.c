/*
 * harness/diff/dbf_diff/test_interp_nav.c -- host oracle for S5.2: navigation
 * commands (GO TOP / GO BOTTOM / GOTO / SKIP; physical and index order; EOF/BOF).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_interp_use.c:
 * the seed test_assert.h harness, a host PAL via pal_host_make (pal_host.c).
 * A non-zero exit on any failed check keeps `make test-interp-nav` from
 * false-greening (Law 2: the oracle is the truth).
 *
 * WHAT S5.2 IS: the navigation layer that S5.4 (LIST/LOCATE scope) and S5.5
 * (REPLACE scope) will consume. Two modes:
 *   Physical order (master==0): records 1..nrec in disk order.
 *   Index order (master>0):     records in ascending key order per the master .ndx.
 *
 * Two tiers (plan Sec 2.A):
 *   Tier 0 (committed, operator-free):
 *     A SYNTHETIC table built in /tmp with 5 records. Proves GO TOP, GO BOTTOM,
 *     GOTO, SKIP +1 / -1, and EOF/BOF maintenance in PHYSICAL order only. No
 *     corpus needed; the -DNAV_MUTATE_SKIP mutant bites this tier.
 *   Tier 1 (golden-diff):
 *     CLIENTS.DBF + NAMES.NDX (char key, 49 records/entries): full index-order
 *     walk via SKIP +1 from GO TOP to EOF; compare each RECNO against the
 *     ndx_inorder-derived reference sequence; GO BOTTOM; SKIP -1 back.
 *     (NAMES.NDX is used instead of CNAMES.NDX: both index LASTNAME+FIRSTNAME on
 *     CLIENTS.DBF with the same key order; CNAMES.NDX has a corrupted trailing
 *     child pointer in the root page that causes ndx_inorder to return
 *     NDX_ERR_BAD_PAGE after 40 entries -- a pre-existing corpus file defect,
 *     confirmed by test_ndx_keys failures.  NAMES.NDX is structurally correct.)
 *     TOURS.DBF + TOURDATE.NDX (numeric/date key, 30 records/entries): same.
 *     Absent goldens -> LOUD skip; Tier 0 still runs.
 *
 * Mutation proof (Rule 6 / ARB rider (a)):
 *   Build with -DNAV_MUTATE_SKIP:
 *     Physical tier: SKIP +1 advances by 2; the step-by-step RECNO checks go RED
 *     at the very first SKIP (expect recno 2, get recno 3). The mutant is
 *     impossible to miss.
 *     Index tier: SKIP uses physical recno order instead of the index sequence,
 *     so the first few RECNOs diverge from the reference sequence and the checks
 *     go RED.
 *
 * GATED edges (plan sec7 / GAPS secP -- loud-skipped, NOT asserted):
 *   - Exact error-vs-silent of SKIP at EOF/BOF.
 *   - GO to a record hidden by SET DELETED/FILTER (S5.6).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.2 oracle ("walk in both orders").
 *   - os/samir/include/samir/nav.h    (wa_nav_go_top / go_bottom / goto / skip).
 *   - os/samir/include/samir/workarea.h (wa_recno / wa_eof / wa_bof / wa_is_open).
 *   - os/samir/include/samir/ndx.h    (ndx_inorder -- the index reference walk).
 *   - Corpus ground truth (byte-verified 2026-06-17):
 *       CLIENTS.DBF nrec=49; NAMES.NDX key_type=0 (char), 49 entries.
 *         (NAMES.NDX == CNAMES.NDX in key content; CNAMES.NDX has corrupt trailing
 *          child in root -- see Tier 1 comment and test_ndx_keys pre-existing fails.)
 *         First  (Adams+Nathan)    -> recno 15.
 *         Last   (Zambini+Rick)    -> recno 8.
 *         Full recno order: [15,29,9,42,3,39,1,27,45,7,19,46,47,23,40,35,4,
 *                             18,43,11,24,28,25,6,26,5,38,37,16,41,2,34,48,
 *                             36,30,32,10,21,33,12,14,20,31,17,44,22,49,13,8]
 *       TOURS.DBF nrec=30; TOURDATE.NDX key_type=1 (numeric/date), 30 entries.
 *         First (JDN 2446283) -> recno 1.
 *         Last  (JDN 2446395) -> recno 26.
 *         Full recno order: [1,29,30,2,3,4,5,6,14,15,7,8,9,10,17,19,12,18,
 *                             16,13,20,22,21,23,11,24,28,25,27,26]
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"          /* seed/, on -Iseed */
#include "samir/workarea.h"       /* os/samir/include/ on -Ios/samir/include */
#include "samir/nav.h"
#include "samir/interp.h"
#include "samir/dbf.h"
#include "samir/ndx.h"
#include "samir/value.h"
#include "samir/rt.h"

TEST_HARNESS();

/* pal_host.c surface */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* ---- path helpers ---- */
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

#define SP_PATH "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities"

/* =====================================================================
 * Tier 0: SYNTHETIC physical-order walk (no goldens needed).
 *
 * Build a minimal 5-record, 1-field-C(4) .dbf in /tmp, USE into area 1
 * (physical order only), then drive GO TOP / SKIP / GO BOTTOM / GOTO / BOF/EOF.
 * The -DNAV_MUTATE_SKIP mutant makes SKIP +1 advance by 2 -> RED here.
 * ===================================================================== */

static int write_min5_dbf(const char *path)
{
    /* 1-field C(4) table, 5 records: "REC1", "REC2", ..., "REC5". */
    uint8_t hdr[32];
    uint8_t desc[32];
    uint8_t term[1];
    int i;
    FILE *f;

    memset(hdr, 0, sizeof(hdr));
    hdr[0x00] = 0x03u;
    hdr[0x04] = 5; hdr[0x05] = 0; hdr[0x06] = 0; hdr[0x07] = 0; /* nrec=5 LE */
    hdr[0x08] = 65; hdr[0x09] = 0;  /* header_length = 32+32+1 = 65 */
    hdr[0x0A] = 5;  hdr[0x0B] = 0;  /* record_length = 1 + 4 = 5 */

    memset(desc, 0, sizeof(desc));
    desc[0x00] = 'V'; desc[0x01] = 'A'; desc[0x02] = 'L'; desc[0x03] = '\0';
    desc[0x0B] = 'C';
    desc[0x10] = 4;
    desc[0x11] = 0;

    term[0] = 0x0Du;

    f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(hdr, 1, 32, f);
    fwrite(desc, 1, 32, f);
    fwrite(term, 1, 1, f);
    for (i = 0; i < 5; i++) {
        uint8_t rec[5];
        rec[0] = 0x20u; /* delete flag: live */
        rec[1] = 'R'; rec[2] = 'E'; rec[3] = 'C'; rec[4] = (uint8_t)('1' + i);
        fwrite(rec, 1, 5, f);
    }
    fclose(f);
    return 0;
}

static void test_physical_nav(samir_pal_t *pal)
{
    const char *pa = "/tmp/test_interp_nav5.dbf";
    xb_interp *ip;
    wa_env *env;
    int rc;
    char msg[256];

    CHECK(write_min5_dbf(pa) == 0, "tier0: write 5-record table");

    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "tier0: xb_interp_make");
    if (!ip) { remove(pa); return; }
    env = xb_interp_env(ip);

    rc = wa_set_open(env, 1, pa, "T0", NULL);
    snprintf(msg, sizeof(msg), "tier0: USE 5-rec table rc=%d", rc);
    CHECK(rc == WA_OK, msg);
    if (rc != WA_OK) { xb_interp_free(ip); remove(pa); return; }

    /* After USE: RECNO=1, nrec=5, eof=0, bof=0. */
    CHECK(wa_recno(env, 1) == 1u, "tier0: USE -> RECNO=1");
    CHECK(wa_nrec(env, 1)  == 5u, "tier0: USE -> nrec=5");
    CHECK(wa_eof(env, 1)   == 0,  "tier0: USE -> eof=0");
    CHECK(wa_bof(env, 1)   == 0,  "tier0: USE -> bof=0");

    /* GO TOP -> RECNO=1, eof=0, bof=0. */
    rc = wa_nav_go_top(env, 1);
    CHECK(rc == NAV_OK, "tier0: GO TOP rc");
    snprintf(msg, sizeof(msg), "tier0: GO TOP -> RECNO=1 (got %u)", wa_recno(env, 1));
    CHECK(wa_recno(env, 1) == 1u, msg);
    CHECK(wa_eof(env, 1) == 0,  "tier0: GO TOP clears EOF");
    CHECK(wa_bof(env, 1) == 0,  "tier0: GO TOP clears BOF");

    /* SKIP +1 -> RECNO=2. (MUTANT: would give 3.) */
    rc = wa_nav_skip(env, 1, 1);
    snprintf(msg, sizeof(msg), "tier0: SKIP +1 after TOP -> RECNO=2 (got %u)", wa_recno(env, 1));
    CHECK(rc == NAV_OK, "tier0: SKIP +1 rc");
    CHECK(wa_recno(env, 1) == 2u, msg);
    CHECK(wa_eof(env, 1) == 0, "tier0: SKIP +1 -> eof=0");

    /* SKIP +1 -> RECNO=3. */
    rc = wa_nav_skip(env, 1, 1);
    snprintf(msg, sizeof(msg), "tier0: SKIP +1 again -> RECNO=3 (got %u)", wa_recno(env, 1));
    CHECK(rc == NAV_OK, "tier0: SKIP +1 #2 rc");
    CHECK(wa_recno(env, 1) == 3u, msg);

    /* SKIP -1 -> RECNO=2. */
    rc = wa_nav_skip(env, 1, -1);
    snprintf(msg, sizeof(msg), "tier0: SKIP -1 -> RECNO=2 (got %u)", wa_recno(env, 1));
    CHECK(rc == NAV_OK, "tier0: SKIP -1 rc");
    CHECK(wa_recno(env, 1) == 2u, msg);

    /* GO BOTTOM -> RECNO=5 (nrec). */
    rc = wa_nav_go_bottom(env, 1);
    snprintf(msg, sizeof(msg), "tier0: GO BOTTOM -> RECNO=5 (got %u)", wa_recno(env, 1));
    CHECK(rc == NAV_OK, "tier0: GO BOTTOM rc");
    CHECK(wa_recno(env, 1) == 5u, msg);
    CHECK(wa_eof(env, 1) == 0, "tier0: GO BOTTOM clears EOF");
    CHECK(wa_bof(env, 1) == 0, "tier0: GO BOTTOM clears BOF");

    /* SKIP +1 past last -> EOF=1. */
    rc = wa_nav_skip(env, 1, 1);
    CHECK(rc == NAV_OK, "tier0: SKIP past last rc");
    snprintf(msg, sizeof(msg), "tier0: SKIP past last -> EOF=1 (got %d)", wa_eof(env, 1));
    CHECK(wa_eof(env, 1) == 1, msg);
    CHECK(wa_bof(env, 1) == 0, "tier0: SKIP past last -> BOF=0");

    /* GATED (loud-skip): exact SKIP-at-EOF behavior -- not asserted. */
    fprintf(stderr,
            "  SKIP (LOUD): SKIP at EOF error-vs-silent (GAPS secP) -- not asserted.\n");

    /* GO TOP clears EOF. */
    rc = wa_nav_go_top(env, 1);
    CHECK(rc == NAV_OK, "tier0: GO TOP after EOF rc");
    CHECK(wa_recno(env, 1) == 1u, "tier0: GO TOP after EOF -> RECNO=1");
    CHECK(wa_eof(env, 1) == 0,  "tier0: GO TOP clears EOF");

    /* SKIP -1 before first (from RECNO=1) -> BOF=1. */
    rc = wa_nav_skip(env, 1, -1);
    CHECK(rc == NAV_OK, "tier0: SKIP before first rc");
    snprintf(msg, sizeof(msg), "tier0: SKIP before first -> BOF=1 (got %d)", wa_bof(env, 1));
    CHECK(wa_bof(env, 1) == 1, msg);
    CHECK(wa_eof(env, 1) == 0, "tier0: SKIP before first -> EOF=0");

    /* GATED: exact BOF SKIP behavior not asserted. */
    fprintf(stderr,
            "  SKIP (LOUD): SKIP at BOF error-vs-silent (GAPS secP) -- not asserted.\n");

    /* GOTO 3 -> RECNO=3; clears BOF. */
    rc = wa_nav_goto(env, 1, 3u);
    snprintf(msg, sizeof(msg), "tier0: GOTO 3 -> RECNO=3 (got %u)", wa_recno(env, 1));
    CHECK(rc == NAV_OK, "tier0: GOTO 3 rc");
    CHECK(wa_recno(env, 1) == 3u, msg);
    CHECK(wa_bof(env, 1) == 0, "tier0: GOTO clears BOF");
    CHECK(wa_eof(env, 1) == 0, "tier0: GOTO clears EOF");

    /* GOTO 0 (out of range) -> -NAV_ERR_RANGE. */
    rc = wa_nav_goto(env, 1, 0u);
    snprintf(msg, sizeof(msg), "tier0: GOTO 0 -> -NAV_ERR_RANGE (got %d)", rc);
    CHECK(rc == -NAV_ERR_RANGE, msg);

    /* GOTO 6 (> nrec) -> -NAV_ERR_RANGE. */
    rc = wa_nav_goto(env, 1, 6u);
    snprintf(msg, sizeof(msg), "tier0: GOTO 6 (>5) -> -NAV_ERR_RANGE (got %d)", rc);
    CHECK(rc == -NAV_ERR_RANGE, msg);

    /* SKIP +2 from RECNO=3 -> RECNO=5. */
    rc = wa_nav_goto(env, 1, 3u);
    CHECK(rc == NAV_OK, "tier0: GOTO 3 for SKIP+2 rc");
    rc = wa_nav_skip(env, 1, 2);
    snprintf(msg, sizeof(msg), "tier0: SKIP +2 from 3 -> RECNO=5 (got %u)", wa_recno(env, 1));
    CHECK(rc == NAV_OK, "tier0: SKIP +2 rc");
    CHECK(wa_recno(env, 1) == 5u, msg);

    /* Complete walk from GO TOP via SKIP +1 and count steps. */
    {
        int steps = 0;
        uint32_t expected = 1u;
        rc = wa_nav_go_top(env, 1);
        CHECK(rc == NAV_OK, "tier0: full walk GO TOP");
        while (!wa_eof(env, 1)) {
            snprintf(msg, sizeof(msg),
                     "tier0: walk step %d -> RECNO=%u (got %u)", steps + 1,
                     expected, wa_recno(env, 1));
            CHECK(wa_recno(env, 1) == expected, msg);
            steps++;
            expected++;
            if (expected > 6u) break;  /* safety */
            (void)wa_nav_skip(env, 1, 1);
        }
        snprintf(msg, sizeof(msg), "tier0: walked %d records (want 5)", steps);
        CHECK(steps == 5, msg);
        CHECK(wa_eof(env, 1) == 1, "tier0: walk -> EOF after step 5");
    }

    xb_interp_free(ip);
    remove(pa);
}

/* =====================================================================
 * Tier 1 helper: walk an open area in the active order, collecting RECNOs.
 * ===================================================================== */

/* Collect RECNOs by walking GO TOP + repeated SKIP +1 until EOF. */
static uint32_t walk_collect(wa_env *env, int area, uint32_t *out, uint32_t cap)
{
    uint32_t count = 0u;
    if (wa_nav_go_top(env, area) != NAV_OK)
        return 0u;
    while (!wa_eof(env, area)) {
        if (count >= cap)
            break;
        out[count++] = wa_recno(env, area);
        (void)wa_nav_skip(env, area, 1);
    }
    return count;
}

/* ndx_inorder reference walk: collect all recnos in key order. */
typedef struct { uint32_t *seq; uint32_t cap; uint32_t count; } ref_collect_t;

static int ref_visit(void *ctx, const uint8_t *key_data, uint32_t recno)
{
    ref_collect_t *r = (ref_collect_t *)ctx;
    (void)key_data;
    if (r->count >= r->cap) return 1;
    r->seq[r->count++] = recno;
    return 0;
}

/* =====================================================================
 * Tier 1: CLIENTS + NAMES (char index) + TOURS + TOURDATE (numeric index).
 * ===================================================================== */

/*
 * Corpus-verified recno sequences (byte-verified 2026-06-17 via Python inorder
 * walk of NAMES.NDX and TOURDATE.NDX; NAMES.NDX used in place of CNAMES.NDX --
 * see Tier 1 header comment for rationale).
 * Ref: docs comment at top of this file.
 */
static const uint32_t K_CNAMES_SEQ[49] = { /* same sequence as NAMES.NDX */
    15,29,9,42,3,39,1,27,45,7,19,46,47,23,40,35,4,
    18,43,11,24,28,25,6,26,5,38,37,16,41,2,34,48,
    36,30,32,10,21,33,12,14,20,31,17,44,22,49,13,8
};

static const uint32_t K_TOURDATE_SEQ[30] = {
    1,29,30,2,3,4,5,6,14,15,7,8,9,10,17,19,12,18,
    16,13,20,22,21,23,11,24,28,25,27,26
};

static void test_index_nav(samir_pal_t *pal, const char *base)
{
    char clients[1024], names[1024], tours[1024], tourdate[1024];
    xb_interp *ip;
    wa_env *env;
    wa_index_list il;
    int rc;
    char msg[256];
    uint32_t walked[64];
    uint32_t ref_seq[64];
    uint32_t i, nwalked, nref;
    ndx_index *ix;
    ref_collect_t rctx;

    /* Use NAMES.NDX (not CNAMES.NDX): same LASTNAME+FIRSTNAME key on CLIENTS.DBF,
     * but NAMES.NDX has a structurally correct root page (trailing_child=5 points
     * to the rightmost leaf, giving all 49 entries).  CNAMES.NDX has a corrupted
     * trailing_child (19933) in its root that prevents ndx_inorder from reaching
     * page 5 -- a pre-existing defect confirmed by test_ndx_keys failures.
     * Ref: CLAUDE.md Rule 3 (ground truth before code); verified 2026-06-18. */
    join(clients,  sizeof(clients),  base, SP_PATH "/CLIENTS.DBF");
    join(names,    sizeof(names),    base, SP_PATH "/NAMES.NDX");
    join(tours,    sizeof(tours),    base, SP_PATH "/TOURS.DBF");
    join(tourdate, sizeof(tourdate), base, SP_PATH "/TOURDATE.NDX");

    if (!file_exists(clients) || !file_exists(names) ||
        !file_exists(tours) || !file_exists(tourdate)) {
        fprintf(stderr,
                "  SKIP (LOUD): corpus golden(s) absent under base '%s'\n"
                "               need: %s %s %s %s\n"
                "               (Tier-0 synthetic ran; -DNAV_MUTATE_SKIP bites Tier 0)\n",
                base, clients, names, tours, tourdate);
        return;
    }

    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "tier1: xb_interp_make");
    if (!ip) return;
    env = xb_interp_env(ip);

    /* ---- CLIENTS + NAMES (char index, 49 records) ---- */

    il.names[0] = names;
    il.count = 1;
    rc = wa_set_open(env, 1, clients, NULL, &il);
    snprintf(msg, sizeof(msg), "tier1: USE CLIENTS INDEX NAMES rc=%d", rc);
    CHECK(rc == WA_OK, msg);
    if (rc != WA_OK) { xb_interp_free(ip); return; }

    CHECK(wa_nrec(env, 1) == 49u, "tier1: CLIENTS nrec=49");
    CHECK(wa_master_order(env, 1) == 1, "tier1: CLIENTS master=1");

    /* GO TOP: first entry in NAMES order = recno 15 (Adams Nathan). */
    rc = wa_nav_go_top(env, 1);
    CHECK(rc == NAV_OK, "tier1: NAMES GO TOP rc");
    snprintf(msg, sizeof(msg), "tier1: NAMES GO TOP -> RECNO=15 (got %u)", wa_recno(env, 1));
    CHECK(wa_recno(env, 1) == 15u, msg);
    CHECK(wa_eof(env, 1) == 0, "tier1: NAMES GO TOP -> eof=0");
    CHECK(wa_bof(env, 1) == 0, "tier1: NAMES GO TOP -> bof=0");

    /* Full forward walk: collect the RECNO sequence. */
    nwalked = walk_collect(env, 1, walked, 64u);
    snprintf(msg, sizeof(msg), "tier1: NAMES walk count=49 (got %u)", nwalked);
    CHECK(nwalked == 49u, msg);

    /* The walk must equal the known index order. */
    for (i = 0u; i < (nwalked < 49u ? nwalked : 49u); i++) {
        snprintf(msg, sizeof(msg),
                 "tier1: NAMES walk[%u]=recno %u (want %u)",
                 i, walked[i], K_CNAMES_SEQ[i]);
        CHECK(walked[i] == K_CNAMES_SEQ[i], msg);
    }

    /* After the walk the area is at EOF. */
    CHECK(wa_eof(env, 1) == 1, "tier1: NAMES walk -> EOF");

    /* Independent reference check: ndx_inorder gives the same sequence. */
    ix = wa_index(env, 1, 0);
    rctx.seq   = ref_seq;
    rctx.cap   = 64u;
    rctx.count = 0u;
    rc = ndx_inorder(ix, ref_visit, &rctx);
    nref = rctx.count;
    snprintf(msg, sizeof(msg), "tier1: NAMES ndx_inorder count=49 (got %u rc=%d)", nref, rc);
    CHECK(rc == 0 && nref == 49u, msg);
    for (i = 0u; i < (nref < 49u ? nref : 49u); i++) {
        snprintf(msg, sizeof(msg),
                 "tier1: NAMES ndx_inorder[%u]=%u == walk[%u]=%u",
                 i, ref_seq[i], i, walked[i < nwalked ? i : 0u]);
        CHECK(i >= nwalked || ref_seq[i] == walked[i], msg);
    }

    /* GO BOTTOM: last entry = recno 8 (Zambini Rick). */
    rc = wa_nav_go_bottom(env, 1);
    snprintf(msg, sizeof(msg), "tier1: NAMES GO BOTTOM -> RECNO=8 (got %u)", wa_recno(env, 1));
    CHECK(rc == NAV_OK, "tier1: NAMES GO BOTTOM rc");
    CHECK(wa_recno(env, 1) == 8u, msg);
    CHECK(wa_eof(env, 1) == 0, "tier1: NAMES GO BOTTOM clears EOF");

    /* SKIP -1 from last: should retreat one step to recno 13 (index order second-to-last). */
    rc = wa_nav_skip(env, 1, -1);
    snprintf(msg, sizeof(msg), "tier1: NAMES SKIP -1 from last -> RECNO=13 (got %u)", wa_recno(env, 1));
    CHECK(rc == NAV_OK, "tier1: NAMES SKIP -1 from last rc");
    CHECK(wa_recno(env, 1) == 13u, msg);

    /* SKIP +1 back to last. */
    rc = wa_nav_skip(env, 1, 1);
    CHECK(rc == NAV_OK, "tier1: NAMES SKIP +1 to last rc");
    snprintf(msg, sizeof(msg), "tier1: NAMES SKIP +1 back to last -> RECNO=8 (got %u)", wa_recno(env, 1));
    CHECK(wa_recno(env, 1) == 8u, msg);

    /* SKIP +1 from last -> EOF. */
    rc = wa_nav_skip(env, 1, 1);
    CHECK(rc == NAV_OK, "tier1: NAMES SKIP past last rc");
    CHECK(wa_eof(env, 1) == 1, "tier1: NAMES SKIP past last -> EOF");
    fprintf(stderr,
            "  SKIP (LOUD): NAMES SKIP at EOF exact error-vs-silent (GAPS secP) -- not asserted.\n");

    /* GO TOP clears EOF. */
    rc = wa_nav_go_top(env, 1);
    CHECK(rc == NAV_OK, "tier1: NAMES GO TOP after EOF rc");
    CHECK(wa_recno(env, 1) == 15u, "tier1: NAMES GO TOP after EOF -> RECNO=15");
    CHECK(wa_eof(env, 1) == 0, "tier1: NAMES GO TOP after EOF -> eof=0");

    /* SKIP -1 from first -> BOF. */
    rc = wa_nav_skip(env, 1, -1);
    CHECK(rc == NAV_OK, "tier1: NAMES SKIP before first rc");
    CHECK(wa_bof(env, 1) == 1, "tier1: NAMES SKIP before first -> BOF");
    fprintf(stderr,
            "  SKIP (LOUD): NAMES SKIP at BOF exact error-vs-silent (GAPS secP) -- not asserted.\n");

    /* GATED: GO to a record hidden by SET DELETED / SET FILTER -- not tested (S5.6). */
    fprintf(stderr,
            "  SKIP (LOUD): GO to record hidden by SET DELETED/FILTER (GAPS secP) -- not asserted (S5.6).\n");

    wa_close(env, 1);
    wa_nav_reset(1);

    /* ---- TOURS + TOURDATE (numeric/date index, 30 records) ---- */

    il.names[0] = tourdate;
    il.count = 1;
    rc = wa_set_open(env, 1, tours, NULL, &il);
    snprintf(msg, sizeof(msg), "tier1: USE TOURS INDEX TOURDATE rc=%d", rc);
    CHECK(rc == WA_OK, msg);
    if (rc != WA_OK) { xb_interp_free(ip); return; }

    CHECK(wa_nrec(env, 1) == 30u, "tier1: TOURS nrec=30");
    CHECK(wa_master_order(env, 1) == 1, "tier1: TOURS master=1");

    /* GO TOP: first entry in TOURDATE order = recno 1 (earliest departure). */
    rc = wa_nav_go_top(env, 1);
    CHECK(rc == NAV_OK, "tier1: TOURDATE GO TOP rc");
    snprintf(msg, sizeof(msg), "tier1: TOURDATE GO TOP -> RECNO=1 (got %u)", wa_recno(env, 1));
    CHECK(wa_recno(env, 1) == 1u, msg);

    /* Full forward walk. */
    nwalked = walk_collect(env, 1, walked, 64u);
    snprintf(msg, sizeof(msg), "tier1: TOURDATE walk count=30 (got %u)", nwalked);
    CHECK(nwalked == 30u, msg);

    for (i = 0u; i < (nwalked < 30u ? nwalked : 30u); i++) {
        snprintf(msg, sizeof(msg),
                 "tier1: TOURDATE walk[%u]=recno %u (want %u)",
                 i, walked[i], K_TOURDATE_SEQ[i]);
        CHECK(walked[i] == K_TOURDATE_SEQ[i], msg);
    }

    CHECK(wa_eof(env, 1) == 1, "tier1: TOURDATE walk -> EOF");

    /* Independent reference check. */
    ix = wa_index(env, 1, 0);
    rctx.seq   = ref_seq;
    rctx.cap   = 64u;
    rctx.count = 0u;
    rc = ndx_inorder(ix, ref_visit, &rctx);
    nref = rctx.count;
    snprintf(msg, sizeof(msg), "tier1: TOURDATE ndx_inorder count=30 (got %u rc=%d)", nref, rc);
    CHECK(rc == 0 && nref == 30u, msg);
    for (i = 0u; i < (nref < 30u ? nref : 30u); i++) {
        snprintf(msg, sizeof(msg),
                 "tier1: TOURDATE ndx_inorder[%u]=%u == walk[%u]=%u",
                 i, ref_seq[i], i, walked[i < nwalked ? i : 0u]);
        CHECK(i >= nwalked || ref_seq[i] == walked[i], msg);
    }

    /* GO BOTTOM: last entry = recno 26 (latest departure). */
    rc = wa_nav_go_bottom(env, 1);
    snprintf(msg, sizeof(msg), "tier1: TOURDATE GO BOTTOM -> RECNO=26 (got %u)", wa_recno(env, 1));
    CHECK(rc == NAV_OK, "tier1: TOURDATE GO BOTTOM rc");
    CHECK(wa_recno(env, 1) == 26u, msg);

    /* SKIP -1 from last -> recno 27 (second-to-last in index order). */
    rc = wa_nav_skip(env, 1, -1);
    snprintf(msg, sizeof(msg),
             "tier1: TOURDATE SKIP -1 from last -> RECNO=27 (got %u)", wa_recno(env, 1));
    CHECK(rc == NAV_OK, "tier1: TOURDATE SKIP -1 from last rc");
    CHECK(wa_recno(env, 1) == 27u, msg);

    /* Physical GOTO: GOTO 5 positions at physical record 5 (independent of index). */
    rc = wa_nav_goto(env, 1, 5u);
    snprintf(msg, sizeof(msg), "tier1: TOURDATE GOTO 5 (physical) -> RECNO=5 (got %u)", wa_recno(env, 1));
    CHECK(rc == NAV_OK, "tier1: TOURDATE GOTO 5 rc");
    CHECK(wa_recno(env, 1) == 5u, msg);

    wa_close(env, 1);
    wa_nav_reset(1);

    xb_interp_free(ip);
}

/* =====================================================================
 * main
 * ===================================================================== */

int main(int argc, char **argv)
{
    const char *base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    struct pal_host_cfg cfg;
    samir_pal_t *pal;
    char path[1024];

    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy   = 99;
    cfg.date_mm   = 12;
    cfg.date_dd   = 31;
    cfg.heap_size = 2u * 1024u * 1024u;   /* 2 MiB: index seq + area caches */

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    /* Tier 0: synthetic physical-order walk (always runs). */
    test_physical_nav(pal);

    /* Tier 1: corpus index-order walk (loud-skip if absent). */
    join(path, sizeof(path), base, SP_PATH "/CLIENTS.DBF");
    if (!file_exists(path)) {
        fprintf(stderr,
                "  SKIP (LOUD): no corpus goldens under base '%s'\n"
                "               expected e.g. %s/%s/CLIENTS.DBF\n"
                "               (pass the corpus base as argv[1]; Tier-0 physical ran)\n",
                base, base, SP_PATH);
    }
    test_index_nav(pal, base);

    pal_host_free(pal);
    return TEST_SUMMARY("test-interp-nav");
}
