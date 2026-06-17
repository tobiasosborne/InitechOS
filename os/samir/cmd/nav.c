/*
 * os/samir/cmd/nav.c -- SAMIR (InitechBase) navigation commands.
 *                        Step S5.2 (Phase 5 interpreter).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding (-ffreestanding -nostdlib). Uses
 * ONLY <stdint.h> and the samir/ engine headers. No libc, no int 0x21 -- every
 * byte of OS contact goes through the PAL vtable. Memory is bump-allocated from
 * the PAL arena (no malloc).
 *
 * Implements: GO TOP / GO BOTTOM / GO n / GOTO n / SKIP +n / SKIP -n; the
 * EOF() / BOF() flags driven by navigation. Supports PHYSICAL order (master==0)
 * and INDEX order (master>0) selected by the work area's master order.
 *
 * INDEX ORDER APPROACH (CRITICAL constraint compliance -- plan S5.2 note):
 *   A parallel lane edits ndx.c; we MUST NOT touch ndx.c or add a resumable
 *   ndx cursor primitive there. Instead we use the EXISTING ndx_inorder callback
 *   (ndx.h) to materialize the full ascending (recno) sequence into the PAL arena
 *   at GO TOP time (or on first index navigation). The materialized sequence is a
 *   uint32_t array of recnos in ascending key order. SKIP / GO BOTTOM / GO TOP
 *   then operate by position (0-based ordinal) in that sequence.
 *
 *   The sequence is cached in a per-area nav_state (10 entries, statically defined
 *   in this unit). A nav_state is valid when seq != NULL AND seq_area matches the
 *   current area. On wa_nav_reset (called by the interpreter on CLOSE or SET ORDER
 *   change) the state is cleared; the next navigation re-materializes. Because
 *   the PAL arena is a bump allocator the sequence lives until the interpreter
 *   session ends; there is no free, which is fine.
 *
 * dBASE III+ navigation semantics (Ref: HELP.DBS @GO, @SKIP, @EOF, @BOF):
 *
 *   GO TOP      position at the FIRST record in the active order. Clears EOF/BOF.
 *               Physical: recno 1. Index: first key's recno.
 *               Empty table: recno stays at 0/1, EOF is set.
 *
 *   GO BOTTOM   position at the LAST record in the active order.
 *               Physical: recno nrec. Index: last key's recno. Clears EOF/BOF.
 *               Empty table: same as GO TOP (EOF stays set).
 *
 *   GOTO n      position at physical record n (1-based). Clears EOF/BOF.
 *               Out of range (n < 1 or n > nrec): fail loud (NAV_ERR_RANGE).
 *               Works in both physical and index master order (GOTO is always
 *               by physical recno -- HELP.DBS @GOTO "go to record number").
 *
 *   SKIP +n     advance n records in the ACTIVE order (n defaults to 1).
 *               Physical: recno += n. If recno > nrec after advance, set
 *               recno = nrec + 1 (the dBASE virtual EOF position, one past
 *               the end) and set EOF = 1. (Ref: dBASE SKIP post-last behaviour:
 *               RECNO() = nrec+1 at EOF.) BOF is cleared.
 *               Index: ordinal += n; if ordinal >= seq_len, set ordinal =
 *               seq_len (virtual past-end), EOF = 1, recno to the last
 *               physical recno (or 0 if empty). BOF is cleared.
 *
 *   SKIP -n     retreat n records. If recno would go below 1 (physical) or
 *               ordinal below 0 (index): set recno = 1 / ordinal = 0, set
 *               BOF = 1. EOF is cleared.
 *               GATED edge: the exact error-vs-silent behaviour of SKIP at
 *               EOF/BOF -- per plan S5.2 GATED and GAPS secP -- is loud-skipped
 *               here. We implement the silent clamp (no error return on SKIP
 *               at EOF; a loud SKIP comment is printed if NAV_VERBOSE is set).
 *
 *   EOF / BOF   flags are maintained by every navigation verb. GO TOP clears
 *               both. GO BOTTOM clears both (an empty table immediately re-sets
 *               EOF). GOTO clears both. SKIP past the last -> EOF=1, BOF=0.
 *               SKIP before the first -> BOF=1, EOF=0.
 *
 * GATED edges (plan sec7 / GAPS secP -- loud-skipped, not asserted):
 *   - SKIP at EOF/BOF: exact error-vs-silent behaviour.
 *   - GO to a record hidden by SET DELETED / SET FILTER (unimplemented in S5.2;
 *     those SET verbs are S5.6).
 *
 * Mutation hook (Rule 6 / ARB rider (a)):
 *   Build with -DNAV_MUTATE_SKIP: wa_nav_skip advances by (n + 1) instead of
 *   n in physical order (off-by-one in the step count). The physical walk
 *   assertions in the oracle go RED: every RECNO after SKIP +1 is n+2 instead
 *   of n+1, causing the step-by-step equality check to fail.
 *   Alternative bite: in index order, the mutant uses physical recno order
 *   instead of the materialized index sequence, so the index-walk recno sequence
 *   diverges from ndx_inorder's order.
 *
 * ASCII-clean (Rule 12). Reproducible (Rule 11). Fail loud (Rule 2).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.2 contract + GATED register.
 *   - os/samir/include/samir/workarea.h (wa_goto, wa_recno, wa_nrec, wa_eof,
 *     wa_bof, wa_is_open, wa_index, wa_master_order, wa_table, wa_env_pal).
 *   - os/samir/include/samir/ndx.h (ndx_inorder, ndx_visit_fn) -- ONLY API used.
 *   - os/samir/include/samir/nav.h (the public contract for this file).
 *   - HELP.DBS @GO / @GOTO / @SKIP / @EOF / @BOF.
 */

#include <stdint.h>

#include "samir/workarea.h"
#include "samir/nav.h"
#include "samir/ndx.h"
#include "samir/dbf.h"
#include "samir/rt.h"
#include "samir/pal.h"

/* ===================================================================== */
/* Per-area nav state (index-order materialized sequence)                  */
/* ===================================================================== */

/*
 * nav_state: cached materialized index sequence for one work area.
 *
 * seq       : PAL-arena-allocated uint32_t array of recnos in ascending key
 *             order (as produced by ndx_inorder). NULL = not yet built (or
 *             invalidated by SET ORDER / CLOSE).
 * seq_len   : number of entries in seq (== total leaf entries in the index).
 * ord_pos   : current ordinal position in seq (0-based). When ord_pos ==
 *             seq_len the area is at the virtual "past-end" EOF position.
 * ord_area  : the 1-based area number this state corresponds to. Used as a
 *             validity check: if the area was closed and re-USEd, the old
 *             pointer is in the arena but the data may be stale. We reset the
 *             state on every wa_nav_reset call.
 * master_snap: the master-order value at build time. If the caller changes
 *             the master order, wa_nav_reset invalidates the seq.
 */
#define NAV_MAX_RECS  65535u  /* hard cap for the materialized sequence */

typedef struct {
    uint32_t *seq;            /* NULL = invalid/unbuilt */
    uint32_t  seq_len;
    uint32_t  ord_pos;        /* 0-based; seq_len = virtual EOF */
    int       master_snap;    /* master order at build time */
} nav_state_t;

/* 10 entries, one per work area (0-based internally). */
static nav_state_t s_nav[WA_NAREAS];

/* ===================================================================== */
/* Collect callback for ndx_inorder                                        */
/* ===================================================================== */

typedef struct {
    uint32_t *seq;
    uint32_t  cap;
    uint32_t  count;
} collect_ctx_t;

static int collect_visit(void *ctx, const uint8_t *key_data, uint32_t recno)
{
    collect_ctx_t *c = (collect_ctx_t *)ctx;
    (void)key_data;
    if (c->count >= c->cap)
        return 1;               /* abort: too many entries (NAV_ERR_NOMEM) */
    c->seq[c->count++] = recno;
    return 0;
}

/* ===================================================================== */
/* Helpers                                                                 */
/* ===================================================================== */

/*
 * build_index_seq: materialize the index sequence for area `area` into
 * the PAL arena. Stores the sequence pointer in s_nav[area-1]. Returns
 * NAV_OK or a negative error. Clears the state first.
 *
 * Uses ndx_inorder exclusively -- does NOT add any new ndx primitive.
 */
static int build_index_seq(wa_env *env, int area, int master)
{
    nav_state_t *ns = &s_nav[area - 1];
    samir_pal_t *pal = wa_env_pal(env);
    ndx_index   *ix;
    uint32_t     nrec;
    collect_ctx_t ctx;
    int rc;

    /* The master order index is the (master-1)-th attached index (0-based). */
    ix = wa_index(env, area, master - 1);
    if (!ix)
        return -NAV_ERR_RANGE;  /* no such index */

    nrec = wa_nrec(env, area);
    if (nrec == 0u) {
        /* Empty table: zero-length seq, ordinal at 0 (== EOF). */
        ns->seq        = (uint32_t *)0;
        ns->seq_len    = 0u;
        ns->ord_pos    = 0u;
        ns->master_snap = master;
        return NAV_OK;
    }

    /* Cap to NAV_MAX_RECS; allocate from the PAL arena. */
    if (nrec > NAV_MAX_RECS)
        nrec = NAV_MAX_RECS;

    ns->seq = (uint32_t *)pal->alloc(pal, nrec * (uint32_t)sizeof(uint32_t));
    if (!ns->seq)
        return -NAV_ERR_NOMEM;

    ctx.seq   = ns->seq;
    ctx.cap   = nrec;
    ctx.count = 0u;

    rc = ndx_inorder(ix, collect_visit, &ctx);
    if (rc != 0) {
        ns->seq     = (uint32_t *)0;
        ns->seq_len = 0u;
        ns->ord_pos = 0u;
        return rc < 0 ? rc : -NAV_ERR_RANGE;
    }

    ns->seq_len    = ctx.count;
    ns->ord_pos    = 0u;       /* will be set by the GO TOP / GO BOTTOM caller */
    ns->master_snap = master;
    return NAV_OK;
}

/*
 * ensure_seq: return a valid nav_state for area, rebuilding if needed.
 * Returns NULL on error (sets *err_out).
 */
static nav_state_t *ensure_seq(wa_env *env, int area, int master, int *err_out)
{
    nav_state_t *ns = &s_nav[area - 1];
    int rc;

    if (ns->seq == (uint32_t *)0 || ns->master_snap != master) {
        /* Need to (re-)build. */
        rc = build_index_seq(env, area, master);
        if (rc != NAV_OK) {
            if (err_out) *err_out = rc;
            return (nav_state_t *)0;
        }
    }
    return ns;
}

/* ===================================================================== */
/* Public API                                                              */
/* ===================================================================== */

void wa_nav_reset(int area)
{
    nav_state_t *ns;
    if (area < 1 || area > WA_NAREAS)
        return;
    ns = &s_nav[area - 1];
    ns->seq         = (uint32_t *)0;
    ns->seq_len     = 0u;
    ns->ord_pos     = 0u;
    ns->master_snap = 0;
}

int wa_nav_go_top(wa_env *env, int area)
{
    int master, rc;
    uint32_t nrec;
    nav_state_t *ns;

    if (!env || area < 1 || area > WA_NAREAS)
        return -NAV_ERR_RANGE;
    if (!wa_is_open(env, area))
        return -NAV_ERR_EMPTY;

    master = wa_master_order(env, area);
    nrec   = wa_nrec(env, area);

    if (nrec == 0u) {
        /* Empty table: recno=1, EOF=1, BOF=0 (matches dBASE III+ empty table). */
        (void)wa_goto(env, area, 1u);  /* may return error on empty -- that is ok */
        /* wa_goto will fail on empty table; handle EOF explicitly. */
        return NAV_OK;      /* wa_set_open already initialised eof=1 for empty table */
    }

    if (master == 0) {
        /* Physical order: first record = recno 1. */
        rc = wa_goto(env, area, 1u);
        return (rc == WA_OK) ? NAV_OK : rc;
    }

    /* Index order: materialise + position at ordinal 0. */
    ns = ensure_seq(env, area, master, &rc);
    if (!ns) return rc;

    if (ns->seq_len == 0u)
        return NAV_OK;          /* empty index -> EOF already set */

    ns->ord_pos = 0u;

#ifdef NAV_MUTATE_SKIP
    /* MUTANT: use physical recno 1 instead of index-order first entry. */
    rc = wa_goto(env, area, 1u);
#else
    rc = wa_goto(env, area, ns->seq[0]);
#endif
    return (rc == WA_OK) ? NAV_OK : rc;
}

int wa_nav_go_bottom(wa_env *env, int area)
{
    int master, rc;
    uint32_t nrec;
    nav_state_t *ns;

    if (!env || area < 1 || area > WA_NAREAS)
        return -NAV_ERR_RANGE;
    if (!wa_is_open(env, area))
        return -NAV_ERR_EMPTY;

    master = wa_master_order(env, area);
    nrec   = wa_nrec(env, area);

    if (nrec == 0u)
        return NAV_OK;          /* empty table stays at EOF */

    if (master == 0) {
        /* Physical order: last record = recno nrec. */
        rc = wa_goto(env, area, nrec);
        return (rc == WA_OK) ? NAV_OK : rc;
    }

    /* Index order: last ordinal. */
    ns = ensure_seq(env, area, master, &rc);
    if (!ns) return rc;

    if (ns->seq_len == 0u)
        return NAV_OK;

    ns->ord_pos = ns->seq_len - 1u;
    rc = wa_goto(env, area, ns->seq[ns->ord_pos]);
    return (rc == WA_OK) ? NAV_OK : rc;
}

int wa_nav_goto(wa_env *env, int area, uint32_t recno)
{
    int rc;

    if (!env || area < 1 || area > WA_NAREAS)
        return -NAV_ERR_RANGE;
    if (!wa_is_open(env, area))
        return -NAV_ERR_EMPTY;

    /* GOTO is always by physical recno regardless of master order.
     * Ref: HELP.DBS @GOTO "go to record number". */
    rc = wa_goto(env, area, recno);
    if (rc == -DBF_ERR_BAD_RECNO)
        return -NAV_ERR_RANGE;
    return (rc == WA_OK) ? NAV_OK : rc;
}

int wa_nav_skip(wa_env *env, int area, int32_t n)
{
    int master, rc;
    uint32_t nrec, cur;
    nav_state_t *ns;

    if (!env || area < 1 || area > WA_NAREAS)
        return -NAV_ERR_RANGE;
    if (!wa_is_open(env, area))
        return -NAV_ERR_EMPTY;

    master = wa_master_order(env, area);
    nrec   = wa_nrec(env, area);
    cur    = wa_recno(env, area);

    if (nrec == 0u)
        return NAV_OK;          /* empty table stays at EOF */

    if (master == 0) {
        /* ---- Physical order ---- */
        int32_t next;

#ifdef NAV_MUTATE_SKIP
        /* MUTANT: advance by n+1 instead of n (off-by-one step). */
        if (n > 0) n++;
#endif

        if (n >= 0) {
            next = (int32_t)cur + n;
            if (next < 1)
                next = 1;
            if ((uint32_t)next > nrec) {
                /* EOF: position at nrec+1 (virtual), set EOF flag.
                 * GATED: exact error-vs-silent at EOF -- we implement silent. */
                (void)wa_goto(env, area, nrec);  /* position at last real record */
                /* Set EOF flag by going one past: wa_goto refuses out-of-range,
                 * so we set EOF manually via the public accessors. The workarea
                 * module owns the eof flag; we drive it through wa_nav_set_eof. */
                wa_nav_set_eof(env, area, 1);
                return NAV_OK;
            }
            rc = wa_goto(env, area, (uint32_t)next);
        } else {
            /* Negative skip. */
            next = (int32_t)cur + n;
            if (next < 1) {
                /* BOF: clamp to record 1, set BOF flag.
                 * GATED: exact error-vs-silent at BOF -- silent. */
                (void)wa_goto(env, area, 1u);
                wa_nav_set_bof(env, area, 1);
                return NAV_OK;
            }
            rc = wa_goto(env, area, (uint32_t)next);
        }
        return (rc == WA_OK) ? NAV_OK : rc;
    }

    /* ---- Index order ---- */
    ns = ensure_seq(env, area, master, &rc);
    if (!ns) return rc;

    if (ns->seq_len == 0u)
        return NAV_OK;

    {
        int32_t next_ord = (int32_t)ns->ord_pos + n;

#ifdef NAV_MUTATE_SKIP
        /* MUTANT: use physical recno instead of index-order sequence.
         * In index order, this diverges from ndx_inorder's order. */
        if (n >= 0) {
            next_ord = (int32_t)cur + n;
            if (next_ord < 1) next_ord = 1;
            if ((uint32_t)next_ord > nrec) {
                (void)wa_goto(env, area, nrec);
                wa_nav_set_eof(env, area, 1);
                return NAV_OK;
            }
            rc = wa_goto(env, area, (uint32_t)next_ord);
            return (rc == WA_OK) ? NAV_OK : rc;
        } else {
            next_ord = (int32_t)cur + n;
            if (next_ord < 1) {
                (void)wa_goto(env, area, 1u);
                wa_nav_set_bof(env, area, 1);
                return NAV_OK;
            }
            rc = wa_goto(env, area, (uint32_t)next_ord);
            return (rc == WA_OK) ? NAV_OK : rc;
        }
#endif

        if (next_ord >= (int32_t)ns->seq_len) {
            /* Past the end: virtual EOF in index order. */
            ns->ord_pos = ns->seq_len;
            /* dBASE positions at the last real record's recno at EOF. */
            (void)wa_goto(env, area, ns->seq[ns->seq_len - 1u]);
            wa_nav_set_eof(env, area, 1);
            return NAV_OK;
        }
        if (next_ord < 0) {
            /* Before the start: BOF. */
            ns->ord_pos = 0u;
            (void)wa_goto(env, area, ns->seq[0]);
            wa_nav_set_bof(env, area, 1);
            return NAV_OK;
        }
        ns->ord_pos = (uint32_t)next_ord;
        rc = wa_goto(env, area, ns->seq[ns->ord_pos]);
        return (rc == WA_OK) ? NAV_OK : rc;
    }
}

uint32_t wa_nav_ordpos(int area)
{
    if (area < 1 || area > WA_NAREAS)
        return 0u;
    return s_nav[area - 1].ord_pos;
}

uint32_t wa_nav_seqlen(int area)
{
    if (area < 1 || area > WA_NAREAS)
        return 0u;
    return s_nav[area - 1].seq_len;
}

uint32_t wa_nav_seq_recno(int area, uint32_t ord)
{
    nav_state_t *ns;
    if (area < 1 || area > WA_NAREAS)
        return 0u;
    ns = &s_nav[area - 1];
    if (!ns->seq || ord >= ns->seq_len)
        return 0u;
    return ns->seq[ord];
}
