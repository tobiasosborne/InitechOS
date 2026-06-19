/* test_window.c -- the FLAIR Window Manager's property suite (the ORACLE).
 *
 * beads: initech-9qf ("Window Manager: z-order, drag, update regions"). The
 *        load-bearing M3/M4 logic behind the live, draggable desktop. This is
 *        its mechanical oracle (Law 2): the visible-region overlap computation
 *        and the region-difference DAMAGE model (ADR-0004 D-5) verified BIT-EXACT
 *        against INDEPENDENT pixel ground truth.
 *
 * Ref:   PRD Sec 6.2 (region algebra), Sec 6.3 (Toolbox). ADR-0004 D-3 (Window
 *        Manager records + FindWindow part-codes), D-5 (DiffRgn damage: newly-
 *        exposed = (old-covered) DIFF (now-covered), accumulated into updateRgn,
 *        NO over-repaint). spec/window_record.h, spec/region_algebra.h,
 *        os/flair/window.h. harness/proptest/test_region.c + test_blitter.c (the
 *        property idiom + seeded LCG + rgn_store arena + rasterize ground truth
 *        this suite MIRRORS). CLAUDE.md Law 2 (oracle is truth), Rule 1
 *        (RED->GREEN), Rule 6 (mutation-proven), Rule 11 (seeded LCG ->
 *        deterministic), Rule 12 ASCII.
 *
 * THE PROPERTIES (in order of decisiveness):
 *
 *  1. VISIBLE-REGION correctness.  For random window stacks, each window's
 *     ComputeVisible(W) == strucRgn(W) DIFF (union of all strucRgns in front),
 *     verified by an INDEPENDENT owner-grid: rasterize all windows front-to-back
 *     into a per-pixel OWNER id (first writer wins == front-most owner). The set
 *     of pixels owned by W is the true visible region. We rasterize the Window
 *     Manager's computed region and memcmp it to that owner-set -- NOT by calling
 *     the same region op the implementation calls.
 *
 *  2. DAMAGE / UPDATE correctness (D-5, the decisive homomorphism-style check).
 *     After a MoveWindow, build the OWNER grid before and after. The TRUE
 *     exposure is exactly the set of pixels that the moved window owned BEFORE
 *     and no longer owns AFTER (clipped to the desktop frame) -- i.e. the pixels
 *     whose owner changed away from the moved window. The union of every window's
 *     accumulated updateRgn PLUS the desktop_update region must EQUAL that set,
 *     BIT-EXACT: every newly-exposed pixel is marked dirty (completeness) and no
 *     already-correct pixel is marked dirty (NO over-repaint, soundness). We also
 *     assert each window's updateRgn lies WITHIN that window's own new visible
 *     region (a window never repaints another's pixels) and that no two damage
 *     sets overlap (no double-count).
 *
 *  3. z-order invariants: front/back ordering preserved across Select/Move;
 *     SelectWindow brings to front + activates exactly one window.
 *
 *  4. FindWindow returns the front-most window containing the point with the
 *     right part-code (inContent / inDrag / inGoAway / inDesk).
 *
 * MUTANTS (Rule 6), each driven RED by the Makefile gate:
 *   WINDOW_MUTATE_ZORDER     -- visible region ignores windows in front.
 *                               => property 1 (and 2) go RED.
 *   WINDOW_MUTATE_OVERPAINT  -- damage = the whole exposed set handed to every
 *                               window (not clipped to its structure, remainder
 *                               never shrunk). => property 2 goes RED.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "region_algebra.h"   /* the LOCKED spec (-Ispec)                        */
#include "region.h"           /* engine constructors (-Ios/flair/atkinson)       */
#include "window_record.h"    /* WindowRecord, part-codes (-Ispec)               */
#include "window.h"           /* the Window Manager under test (-Ios/flair)      */
#include "test_assert.h"      /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)        */

TEST_HARNESS();

/* ===========================================================================
 * The bounded grid + a per-pixel OWNER ground truth (the independent oracle).
 * ---------------------------------------------------------------------------
 * OWNER_NONE = bare desktop. Otherwise the owner is a window INDEX (0..N-1).
 * We rasterize windows FRONT-to-BACK; the FIRST writer of a pixel is its owner
 * (front-most), which is exactly z-order occlusion -- computed WITHOUT any
 * Window Manager region op (so a bug in the WM cannot mask a bug in the oracle).
 * ===========================================================================*/
enum { GW = 48, GH = 36 };               /* grid width/height (pixels)          */
enum { OWNER_NONE = 255 };               /* bare desktop sentinel               */

typedef struct owngrid { uint8_t own[GW * GH]; } owngrid_t;

/* ===========================================================================
 * Region storage (the test_region/test_blitter rgn_store arena: the engine
 * never mallocs; the CALLER supplies rows[]/x_pool). One bundle per region.
 * ===========================================================================*/
typedef struct rgn_store {
    region_t   r;
    rgn_row_t  rows[RGN_ROWS_CAP];
    int16_t    pool[RGN_X_POOL_CAP];
} rgn_store_t;

static void store_attach(rgn_store_t *s)
{
    memset(s, 0, sizeof *s);
    s->r.rows       = s->rows;
    s->r.cap_rows   = RGN_ROWS_CAP;
    s->r.x_pool     = s->pool;
    s->r.x_pool_cap = RGN_X_POOL_CAP;
    region_set_empty(&s->r);
}

/* A window bundle: the record + its three regions' arenas, all attached. */
typedef struct win_store {
    WindowRecord rec;
    rgn_store_t  struc, cont, upd;
} win_store_t;

static void win_attach(win_store_t *w)
{
    memset(&w->rec, 0, sizeof w->rec);
    store_attach(&w->struc);
    store_attach(&w->cont);
    store_attach(&w->upd);
    w->rec.strucRgn  = &w->struc.r;
    w->rec.contRgn   = &w->cont.r;
    w->rec.updateRgn = &w->upd.r;
    w->rec.nextWindow = NULL;
}

/* ===========================================================================
 * rasterize a region into a flat 0/1 membership grid, straight off rows/x-lists
 * (the test_region idiom -- independent of region_contains_point).
 * ===========================================================================*/
static void rasterize_set(const region_t *r, uint8_t *grid /* GW*GH */)
{
    memset(grid, 0, (size_t)GW * GH);
    if (r->is_empty || r->n_rows == 0) return;
    for (uint16_t i = 0; i < r->n_rows; i++) {
        int y0 = r->rows[i].y_top;
        int y1 = (i + 1 < r->n_rows) ? r->rows[i + 1].y_top : y0;
        if (r->rows[i].x_count == 0) continue;
        for (int y = y0; y < y1; y++) {
            if (y < 0 || y >= GH) continue;
            for (uint16_t k = 0; k + 1 < r->rows[i].x_count; k += 2) {
                int xa = r->rows[i].x[k], xb = r->rows[i].x[k + 1];
                for (int x = xa; x < xb; x++)
                    if (x >= 0 && x < GW) grid[y * GW + x] = 1;
            }
        }
    }
}

/* ===========================================================================
 * Build the OWNER grid for a window list (front-to-back; first writer wins).
 * Only VISIBLE windows participate. owner = the window's index in the `idx`
 * array (the test assigns a stable index per window). Pixels no visible window
 * covers are OWNER_NONE (bare desktop).
 * ===========================================================================*/
static void build_owner_grid(const WindowMgr *wm,
                             win_store_t *const *idx, int n,
                             owngrid_t *g)
{
    memset(g->own, OWNER_NONE, sizeof g->own);
    for (const WindowRecord *p = wm->front; p != NULL; p = p->nextWindow) {
        if (!p->visible) continue;
        /* find this record's stable index */
        int wi = -1;
        for (int i = 0; i < n; i++) if (&idx[i]->rec == p) { wi = i; break; }
        if (wi < 0) continue;
        uint8_t sg[GW * GH];
        rasterize_set(p->strucRgn, sg);
        for (int j = 0; j < GW * GH; j++)
            if (sg[j] && g->own[j] == OWNER_NONE) g->own[j] = (uint8_t)wi;
    }
}

/* ===========================================================================
 * Seeded LCG (Rule 11), mirroring test_region/test_blitter.
 * ===========================================================================*/
static uint32_t g_seed = 0x57A91Eu;   /* a distinct seed (STAPLER-ish)          */
static uint32_t lcg(void)
{
    g_seed = g_seed * 1103515245u + 12345u;
    return (g_seed >> 16) & 0x7FFFu;
}
static int rnd(int lo, int hi)        /* inclusive [lo,hi] */
{
    if (hi <= lo) return lo;
    return lo + (int)(lcg() % (uint32_t)(hi - lo + 1));
}

/* A random window: structure rect + an inset content rect (chrome band on every
 * side, title bar on top), all within the grid. */
static void gen_window_rects(rgn_rect_t *struc, rgn_rect_t *cont)
{
    int t = rnd(0, GH - 8), l = rnd(0, GW - 10);
    int b = rnd(t + 7, GH), r = rnd(l + 9, GW);
    struc->top = (int16_t)t; struc->left = (int16_t)l;
    struc->bottom = (int16_t)b; struc->right = (int16_t)r;
    /* content: title bar (3 px) on top, 1-px frame elsewhere. */
    cont->top    = (int16_t)(t + 3);
    cont->left   = (int16_t)(l + 1);
    cont->bottom = (int16_t)(b - 1);
    cont->right  = (int16_t)(r - 1);
}

/* The manager + its three scratch regions + desktop update, all arena-backed. */
typedef struct mgr_store {
    WindowMgr   wm;
    rgn_store_t desk, sa, sb, sc;
} mgr_store_t;

static void mgr_attach(mgr_store_t *m, rgn_rect_t frame)
{
    store_attach(&m->desk); store_attach(&m->sa);
    store_attach(&m->sb);   store_attach(&m->sc);
    WindowMgr_init(&m->wm, frame, &m->desk.r, &m->sa.r, &m->sb.r, &m->sc.r);
}

/* ===========================================================================
 * MAIN -- the suite.
 * ===========================================================================*/
int main(void)
{
    rgn_rect_t FRAME = { 0, 0, GH, GW };

    /* ======================================================================
     * PROPERTY 1: VISIBLE-REGION correctness, random window stacks.
     * ComputeVisible(W) rasterized == the owner-set of W (independent grid).
     * ====================================================================== */
    {
        enum { CASES = 1500, MAXW = 5 };
        int bad = 0;
        for (int t = 0; t < CASES && !bad; t++) {
            static win_store_t W[MAXW];
            static mgr_store_t M;
            static rgn_store_t VIS;
            mgr_attach(&M, FRAME);
            int n = rnd(1, MAXW);
            win_store_t *idx[MAXW];
            /* NewWindow pushes to FRONT, so the LAST created is front-most. We
             * create in order 0..n-1; index i keeps its identity regardless. */
            for (int i = 0; i < n; i++) {
                win_attach(&W[i]);
                idx[i] = &W[i];
                rgn_rect_t s, c; gen_window_rects(&s, &c);
                NewWindow(&M.wm, &W[i].rec, s, c, documentKind, documentProc, 1);
            }

            owngrid_t g; build_owner_grid(&M.wm, idx, n, &g);

            for (int i = 0; i < n && !bad; i++) {
                store_attach(&VIS);
                ComputeVisible(&M.wm, &W[i].rec, &VIS.r);
                uint8_t vg[GW * GH]; rasterize_set(&VIS.r, vg);
                for (int j = 0; j < GW * GH; j++) {
                    int want = (g.own[j] == (uint8_t)i) ? 1 : 0;
                    if (vg[j] != want) { bad = 1; break; }
                }
            }
        }
        CHECK(!bad, "ComputeVisible == owner-set (strucRgn DIFF union-of-fronts), 1500 stacks");
    }

    /* ======================================================================
     * PROPERTY 2: DAMAGE / UPDATE correctness after MoveWindow (ADR-0004 D-5).
     * The union of all updateRgns + desktop_update == the EXACT set of pixels
     * the moved window owned before and lost after (clipped to frame).
     * Verified by owner-grids before/after, NO over-repaint, no double-count.
     * ====================================================================== */
    {
        enum { CASES = 1500, MAXW = 5 };
        int complete_bad = 0;   /* a truly-exposed pixel was NOT marked dirty   */
        int overpaint_bad = 0;  /* a pixel marked dirty was NOT actually exposed */
        int overlap_bad = 0;    /* two windows' damage overlapped (double-count) */
        int ownership_bad = 0;  /* a window's damage left its own structure      */

        for (int t = 0; t < CASES &&
                        !complete_bad && !overpaint_bad &&
                        !overlap_bad && !ownership_bad; t++) {
            static win_store_t W[MAXW];
            static mgr_store_t M;
            mgr_attach(&M, FRAME);
            int n = rnd(2, MAXW);
            win_store_t *idx[MAXW];
            for (int i = 0; i < n; i++) {
                win_attach(&W[i]);
                idx[i] = &W[i];
                rgn_rect_t s, c; gen_window_rects(&s, &c);
                NewWindow(&M.wm, &W[i].rec, s, c, documentKind, documentProc, 1);
            }

            /* pick a window to move (any in the stack) + a random delta. */
            int mw = rnd(0, n - 1);
            rgn_rect_t os = region_get_bbox(W[mw].rec.strucRgn);
            int dh = rnd(-6, 6), dv = rnd(-5, 5);
            if (dh == 0 && dv == 0) dh = 1;

            /* OWNER grid BEFORE the move. */
            owngrid_t before; build_owner_grid(&M.wm, idx, n, &before);

            MoveWindow(&M.wm, &W[mw].rec,
                       (int16_t)(os.left + dh), (int16_t)(os.top + dv));

            /* OWNER grid AFTER the move. */
            owngrid_t after; build_owner_grid(&M.wm, idx, n, &after);

            /* TRUE exposure: pixels the moved window owned BEFORE and does NOT
             * own AFTER (it gave them up). These are exactly the damaged pixels
             * the windows behind / the desktop must repaint. */
            uint8_t truth[GW * GH];
            for (int j = 0; j < GW * GH; j++)
                truth[j] = (before.own[j] == (uint8_t)mw &&
                            after.own[j]  != (uint8_t)mw) ? 1 : 0;

            /* The Window Manager's accumulated damage: union of every window's
             * updateRgn (except the moved window, whose own repaint is the
             * caller's concern) + the desktop_update. Rasterize each and OR. */
            uint8_t dmg[GW * GH]; memset(dmg, 0, sizeof dmg);
            /* per-window damage grids, kept to check overlap + ownership. */
            uint8_t wdmg[MAXW][GW * GH];
            for (int i = 0; i < n; i++) {
                if (i == mw) { memset(wdmg[i], 0, sizeof wdmg[i]); continue; }
                rasterize_set(W[i].rec.updateRgn, wdmg[i]);
                for (int j = 0; j < GW * GH; j++) if (wdmg[i][j]) dmg[j] = 1;
            }
            uint8_t deskg[GW * GH];
            rasterize_set(M.wm.desktop_update, deskg);
            for (int j = 0; j < GW * GH; j++) if (deskg[j]) dmg[j] = 1;

            /* (a) COMPLETENESS + SOUNDNESS: damage == truth, bit-exact. */
            for (int j = 0; j < GW * GH; j++) {
                if (truth[j] && !dmg[j]) { complete_bad = 1; }
                if (dmg[j] && !truth[j]) { overpaint_bad = 1; }
            }

            /* (b) NO over-repaint within a window: each window's damage lies in
             * its OWN new structure (it never repaints another's pixels). */
            for (int i = 0; i < n && !ownership_bad; i++) {
                if (i == mw) continue;
                uint8_t sg[GW * GH]; rasterize_set(W[i].rec.strucRgn, sg);
                for (int j = 0; j < GW * GH; j++)
                    if (wdmg[i][j] && !sg[j]) { ownership_bad = 1; break; }
            }

            /* (c) NO double-count: window damage sets are pairwise disjoint, and
             * disjoint from the desktop damage. */
            for (int a = 0; a < n && !overlap_bad; a++) {
                if (a == mw) continue;
                for (int j = 0; j < GW * GH; j++) {
                    if (!wdmg[a][j]) continue;
                    if (deskg[j]) { overlap_bad = 1; break; }
                    for (int b = a + 1; b < n; b++) {
                        if (b == mw) continue;
                        if (wdmg[b][j]) { overlap_bad = 1; break; }
                    }
                    if (overlap_bad) break;
                }
            }
        }
        CHECK(!complete_bad,  "MoveWindow damage covers EVERY newly-exposed pixel (completeness)");
        CHECK(!overpaint_bad, "MoveWindow damage marks NO unchanged pixel dirty (no over-repaint)");
        CHECK(!ownership_bad, "each window's damage lies within its own structure");
        CHECK(!overlap_bad,   "window/desktop damage sets are pairwise disjoint (no double-count)");
    }

    /* ======================================================================
     * z-order invariants: SelectWindow brings to front + activates exactly one;
     * front/back ordering is a well-formed list after Select/Move.
     * ====================================================================== */
    {
        enum { CASES = 600, MAXW = 5 };
        int front_bad = 0, active_bad = 0, listlen_bad = 0;
        for (int t = 0; t < CASES &&
                        !front_bad && !active_bad && !listlen_bad; t++) {
            static win_store_t W[MAXW];
            static mgr_store_t M;
            mgr_attach(&M, FRAME);
            int n = rnd(2, MAXW);
            for (int i = 0; i < n; i++) {
                win_attach(&W[i]);
                rgn_rect_t s, c; gen_window_rects(&s, &c);
                NewWindow(&M.wm, &W[i].rec, s, c, documentKind, documentProc, 1);
            }
            int pick = rnd(0, n - 1);
            SelectWindow(&M.wm, &W[pick].rec);

            /* front of the list is the picked window. */
            if (M.wm.front != &W[pick].rec) front_bad = 1;

            /* exactly one hilited window, and it is the front-most visible. */
            int hil = 0;
            for (WindowRecord *p = M.wm.front; p; p = p->nextWindow)
                if (p->hilited) hil++;
            if (hil != 1 || !M.wm.front->hilited) active_bad = 1;

            /* list length is preserved (no window lost/duplicated). */
            int len = 0;
            for (WindowRecord *p = M.wm.front; p; p = p->nextWindow) len++;
            if (len != n) listlen_bad = 1;
        }
        CHECK(!front_bad,   "SelectWindow brings the chosen window to the front");
        CHECK(!active_bad,  "exactly one window hilited, and it is the front-most");
        CHECK(!listlen_bad, "z-order list length preserved across Select");
    }

    /* ======================================================================
     * FindWindow: front-most window containing the point, correct part-code.
     * Directed cases (deterministic geometry) + a randomized front-most check.
     * ====================================================================== */
    {
        /* Directed: a single document window with a known title bar + close box. */
        static win_store_t A; static mgr_store_t M;
        mgr_attach(&M, FRAME);
        win_attach(&A);
        rgn_rect_t s = { 5, 6, 25, 30 };          /* struc */
        rgn_rect_t c = { 8, 7, 24, 29 };          /* content: 3-px title, 1-px frame */
        NewWindow(&M.wm, &A.rec, s, c, documentKind, documentProc, 1);

        WindowPtr hit;
        flair_point_t p_content = { 15, 18 };     /* v=15,h=18 inside content */
        CHECK(FindWindow(&M.wm, p_content, &hit) == inContent && hit == &A.rec,
              "FindWindow: point in content -> inContent");

        flair_point_t p_title = { 6, 18 };        /* v=6 in title band, h=18 mid */
        CHECK(FindWindow(&M.wm, p_title, &hit) == inDrag && hit == &A.rec,
              "FindWindow: point in title bar -> inDrag");

        flair_point_t p_close = { 6, 7 };         /* v=6,h=7 top-left close box */
        CHECK(FindWindow(&M.wm, p_close, &hit) == inGoAway && hit == &A.rec,
              "FindWindow: point in close box -> inGoAway");

        flair_point_t p_desk = { 1, 1 };          /* far from the window */
        CHECK(FindWindow(&M.wm, p_desk, &hit) == inDesk && hit == NULL,
              "FindWindow: point on desktop -> inDesk, whichWindow=NULL");

        /* Front-most resolution: two overlapping windows; the front one wins. */
        static win_store_t B;
        win_attach(&B);
        rgn_rect_t s2 = { 5, 6, 25, 30 };         /* same footprint as A */
        rgn_rect_t c2 = { 8, 7, 24, 29 };
        NewWindow(&M.wm, &B.rec, s2, c2, documentKind, documentProc, 1); /* B now front */
        CHECK(FindWindow(&M.wm, p_content, &hit) == inContent && hit == &B.rec,
              "FindWindow: overlap -> front-most window wins");
        SelectWindow(&M.wm, &A.rec);              /* raise A */
        CHECK(FindWindow(&M.wm, p_content, &hit) == inContent && hit == &A.rec,
              "FindWindow: after SelectWindow, raised window wins the hit");
    }

    /* ======================================================================
     * HideWindow exposure: hiding the front window exposes exactly what it
     * owned (independent owner-grid before/after); damage is complete + sound.
     * ====================================================================== */
    {
        enum { CASES = 600, MAXW = 4 };
        int bad = 0;
        for (int t = 0; t < CASES && !bad; t++) {
            static win_store_t W[MAXW];
            static mgr_store_t M;
            mgr_attach(&M, FRAME);
            int n = rnd(2, MAXW);
            win_store_t *idx[MAXW];
            for (int i = 0; i < n; i++) {
                win_attach(&W[i]); idx[i] = &W[i];
                rgn_rect_t s, c; gen_window_rects(&s, &c);
                NewWindow(&M.wm, &W[i].rec, s, c, documentKind, documentProc, 1);
            }
            int hw = rnd(0, n - 1);
            owngrid_t before; build_owner_grid(&M.wm, idx, n, &before);
            HideWindow(&M.wm, &W[hw].rec);
            owngrid_t after; build_owner_grid(&M.wm, idx, n, &after);

            uint8_t truth[GW * GH];
            for (int j = 0; j < GW * GH; j++)
                truth[j] = (before.own[j] == (uint8_t)hw) ? 1 : 0;

            uint8_t dmg[GW * GH]; memset(dmg, 0, sizeof dmg);
            for (int i = 0; i < n; i++) {
                if (i == hw) continue;
                uint8_t wg[GW * GH]; rasterize_set(W[i].rec.updateRgn, wg);
                for (int j = 0; j < GW * GH; j++) if (wg[j]) dmg[j] = 1;
            }
            uint8_t dk[GW * GH]; rasterize_set(M.wm.desktop_update, dk);
            for (int j = 0; j < GW * GH; j++) if (dk[j]) dmg[j] = 1;

            for (int j = 0; j < GW * GH; j++)
                if (truth[j] != dmg[j]) { bad = 1; break; }
            (void)after;
        }
        CHECK(!bad, "HideWindow damage == exactly the hidden window's owned pixels");
    }

    return TEST_SUMMARY("test_window");
}
