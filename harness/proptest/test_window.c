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
 *  3. COMPOSITOR-LAYER INVALIDATE. WindowMgr_invalidate_desktop partitions an
 *     arbitrary clipped rectangle exactly once among the frontmost visible
 *     window owners and the remaining bare desktop. This is the damage entry
 *     used to remove temporary menu panels (beads initech-b3hl/-j0vt).
 *
 *  4. z-order invariants: front/back ordering preserved across Select/Move;
 *     SelectWindow brings to front + activates exactly one window.
 *
 *  5. FindWindow's full Platinum title-band sweep is exactly the same 12x12
 *     close/zoom/collapse geometry the drawer consumes, plus the sampled 18x18
 *     grow cell. Zoom/grow/collapse transactions preserve exact old exposure,
 *     exact new regions, and exact stored-state restoration (R1.1/R1.2).
 *
 * MUTANTS (Rule 6), each driven RED by the Makefile gate:
 *   WINDOW_MUTATE_ZORDER     -- visible region ignores windows in front.
 *                               => property 1 (and 2) go RED.
 *   WINDOW_MUTATE_OVERPAINT  -- damage = the whole exposed set handed to every
 *                               window (not clipped to its structure, remainder
 *                               never shrunk). => property 2 goes RED.
 *   WINDOW_MUTATE_ZONE_OFF          -- shift widget hit geometry by +3px.
 *   WINDOW_MUTATE_ZOOM_NO_RESTORE   -- second zoom keeps standardState.
 *   WINDOW_MUTATE_COLLAPSE_LEAK     -- collapsed contRgn remains non-empty.
 *   WINDOW_MUTATE_GROW_NO_MIN       -- SizeWindow accepts a 1x1 frame.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "region_algebra.h"   /* the LOCKED spec (-Ispec)                        */
#include "region.h"           /* engine constructors (-Ios/flair/atkinson)       */
#include "window_record.h"    /* WindowRecord, part-codes (-Ispec)               */
#include "chrome_metrics.h"   /* independent sampled widget/R1 policy geometry   */
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

static int rect_same(rgn_rect_t a, rgn_rect_t b)
{
    return a.top == b.top && a.left == b.left &&
           a.bottom == b.bottom && a.right == b.right;
}

/* Directed R1 geometry transactions use a 200x160 desktop, independently of
 * the small randomized owner-grid above. These helpers snapshot pixel sets
 * before the operation and compare the exact old-loss/new-self contracts by
 * point membership; they do not call a second Window Manager operation. */
enum { OP_W = 200, OP_H = 160 };

static void snapshot_op_region(const region_t *r, uint8_t *out)
{
    for (int y = 0; y < OP_H; y++) {
        for (int x = 0; x < OP_W; x++) {
            out[y * OP_W + x] =
                (uint8_t)(region_contains_point(r, (int16_t)x, (int16_t)y) != 0);
        }
    }
}

static int geometry_damage_exact(const uint8_t *old_owned,
                                 const WindowRecord *w,
                                 const WindowMgr *wm)
{
    for (int y = 0; y < OP_H; y++) {
        for (int x = 0; x < OP_W; x++) {
            int j = y * OP_W + x;
            int new_owned = region_contains_point(w->strucRgn,
                                                   (int16_t)x, (int16_t)y);
            int desk_dirty = region_contains_point(wm->desktop_update,
                                                    (int16_t)x, (int16_t)y);
            int self_dirty = region_contains_point(w->updateRgn,
                                                    (int16_t)x, (int16_t)y);
            if (desk_dirty != (old_owned[j] && !new_owned)) return 0;
            if (self_dirty != new_owned) return 0;
        }
    }
    return 1;
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
     * R0.3 CalcDoc seam: a document's strucRgn is frame UNION the exact
     * notched shadow L, while contRgn is derived once from sampled chrome
     * metrics and stops at the vertical scrollbar. Moving the window must
     * damage the OLD shadow band too. Ref: sys8/window-chrome.md Sec 1/2.1/4,
     * scrollbars.md Sec 1; beads initech-9d0e/javs/l0mh.
     * ====================================================================== */
    {
        static win_store_t W;
        static mgr_store_t M;
        rgn_rect_t frame = { 1, 1, 34, 47 };
        rgn_rect_t want_content = { 23, 2, 33, 26 };
        mgr_attach(&M, FRAME);
        win_attach(&W);
        NewDocumentWindow(&M.wm, &W.rec, frame, documentKind, 1);

        rgn_rect_t got_frame = WindowFrameRect(&W.rec);
        rgn_rect_t got_content = region_get_bbox(W.rec.contRgn);
        CHECK(got_frame.top == frame.top && got_frame.left == frame.left &&
              got_frame.bottom == frame.bottom && got_frame.right == frame.right,
              "CalcDoc: drawer frame remains the caller's frame when strucRgn grows");
        CHECK(got_content.top == want_content.top &&
              got_content.left == want_content.left &&
              got_content.bottom == want_content.bottom &&
              got_content.right == want_content.right,
              "CalcDoc: contRgn top=top+22 and right stops before body rail+16px scrollbar");

        CHECK(!region_contains_point(W.rec.strucRgn, 47, 2) &&
              region_contains_point(W.rec.strucRgn, 47, 3) &&
              !region_contains_point(W.rec.strucRgn, 2, 34) &&
              region_contains_point(W.rec.strucRgn, 3, 34),
              "CalcDoc: strucRgn contains the +1 shadow L with a 2px near-corner notch");

        WindowMgr_validate(&W.rec);
        MoveWindow(&M.wm, &W.rec, 0, 0);
        CHECK(region_contains_point(M.wm.desktop_update, 47, 10) &&
              region_contains_point(M.wm.desktop_update, 10, 34),
              "MoveWindow: exact exposure includes the OLD right and bottom shadow bands");
    }

    /* ======================================================================
     * R0.2/R0.4 directed seam oracle: the ONE kernel-held title setter and
     * the real-list serial identity used by live FLAIR markers.
     *
     * NewWindow pushes W0,W1,W2 to produce [W2,W1,W0]. Hide preserves that
     * list; Dispose compacts it; Select reflects the new z-order. This is an
     * independent literal golden, not a second traversal of nextWindow.
     * Ref: D2-3; beads initech-vfd8 / initech-883x.
     * ====================================================================== */
    {
        static win_store_t W[3];
        static mgr_store_t M;
        rgn_rect_t s[3] = {
            { 2,  2, 25, 24 },
            { 4, 12, 29, 36 },
            { 1, 20, 26, 46 }
        };
        rgn_rect_t c[3] = {
            { 5,  3, 24, 23 },
            { 7, 13, 28, 35 },
            { 4, 21, 25, 45 }
        };
        mgr_attach(&M, FRAME);
        for (int i = 0; i < 3; i++) {
            win_attach(&W[i]);
            NewWindow(&M.wm, &W[i].rec, s[i], c[i],
                      documentKind, documentProc, 1);
        }

        CHECK(WindowMgr_window_index(&M.wm, &W[2].rec) == 0 &&
              WindowMgr_window_index(&M.wm, &W[1].rec) == 1 &&
              WindowMgr_window_index(&M.wm, &W[0].rec) == 2,
              "window id: three-window scene is indexed front-to-back [W2,W1,W0]");

        WindowMgr_validate(&W[2].rec);
        SetWTitle(&M.wm, &W[2].rec, "HELLO");
        CHECK(strcmp(W[2].rec.titleHandle, "HELLO") == 0 &&
              W[2].rec.titleWidth == 5 * 8 &&
              !region_is_empty(W[2].rec.updateRgn),
              "SetWTitle: one kernel-held setter copies, measures Chicago, and invalidates");

        HideWindow(&M.wm, &W[1].rec);
        CHECK(WindowMgr_window_index(&M.wm, &W[2].rec) == 0 &&
              WindowMgr_window_index(&M.wm, &W[1].rec) == 1 &&
              WindowMgr_window_index(&M.wm, &W[0].rec) == 2,
              "window id: hide churn preserves list indices (hidden records stay linked)");

        DisposeWindow(&M.wm, &W[1].rec);
        CHECK(WindowMgr_window_index(&M.wm, &W[1].rec) == -1 &&
              WindowMgr_window_index(&M.wm, &W[2].rec) == 0 &&
              WindowMgr_window_index(&M.wm, &W[0].rec) == 1,
              "window id: dispose returns -1 for removed record and compacts suffix");

        SelectWindow(&M.wm, &W[0].rec);
        CHECK(WindowMgr_window_index(&M.wm, &W[0].rec) == 0 &&
              WindowMgr_window_index(&M.wm, &W[2].rec) == 1 &&
              WindowMgr_window_index(NULL, &W[0].rec) == -1,
              "window id: select reflects deterministic z-order; NULL manager is -1");
    }

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

            /* Ref: bead initech-v6t2. NewWindow's reaffirm_active now seeds a
             * repaint on every 1->0 hilited transition (the deactivation-
             * repaint fix), so building an n-window stack above may itself
             * have left construction-time damage in the updateRgn of every
             * window that lost front-window status along the way (each
             * window but the last created). That construction damage is real
             * and correct, but it is NOT what this property measures -- this
             * property isolates the damage caused SPECIFICALLY by the
             * MoveWindow call below. Validate (clear) every updateRgn first,
             * exactly as a real event pump would between an app's earlier
             * BeginUpdate/EndUpdate and the next operation, so `truth` (which
             * only accounts for pixels the MOVE exposes) matches what is
             * measured. */
            for (int i = 0; i < n; i++) WindowMgr_validate(&W[i].rec);

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
     * PROPERTY 3: COMPOSITOR-LAYER INVALIDATE (beads initech-b3hl/-j0vt).
     * An arbitrary screen rect painted above the ordinary WindowMgr scene is
     * removed by invalidating every pixel under it. The expected partition is
     * independent owner-grid truth: each in-frame rect pixel belongs exactly
     * once to its frontmost window, or to desktop damage when OWNER_NONE.
     * ====================================================================== */
    {
        enum { CASES = 1000, MAXW = 5 };
        int complete_bad = 0;
        int overpaint_bad = 0;
        int owner_bad = 0;
        int double_bad = 0;

        for (int t = 0; t < CASES && !complete_bad && !overpaint_bad &&
                        !owner_bad && !double_bad; t++) {
            static win_store_t W[MAXW];
            static mgr_store_t M;
            mgr_attach(&M, FRAME);
            int n = rnd(1, MAXW);
            win_store_t *idx[MAXW];
            for (int i = 0; i < n; i++) {
                win_attach(&W[i]);
                idx[i] = &W[i];
                rgn_rect_t s, c;
                gen_window_rects(&s, &c);
                NewWindow(&M.wm, &W[i].rec, s, c,
                          documentKind, documentProc, 1);
            }
            for (int i = 0; i < n; i++) WindowMgr_validate(&W[i].rec);

            owngrid_t owners;
            build_owner_grid(&M.wm, idx, n, &owners);

            rgn_rect_t inval;
            inval.top = (int16_t)rnd(-6, GH - 1);
            inval.left = (int16_t)rnd(-6, GW - 1);
            inval.bottom = (int16_t)rnd(inval.top + 1, GH + 6);
            inval.right = (int16_t)rnd(inval.left + 1, GW + 6);
            WindowMgr_invalidate_desktop(&M.wm, inval);

            uint8_t wdmg[MAXW][GW * GH];
            for (int i = 0; i < n; i++)
                rasterize_set(W[i].rec.updateRgn, wdmg[i]);
            uint8_t ddmg[GW * GH];
            rasterize_set(M.wm.desktop_update, ddmg);

            for (int y = 0; y < GH; y++) {
                for (int x = 0; x < GW; x++) {
                    int j = y * GW + x;
                    int want = y >= inval.top && y < inval.bottom &&
                               x >= inval.left && x < inval.right;
                    int count = ddmg[j] ? 1 : 0;
                    for (int i = 0; i < n; i++) {
                        if (wdmg[i][j]) {
                            count++;
                            if (owners.own[j] != (uint8_t)i) owner_bad = 1;
                        }
                    }
                    if (ddmg[j] && owners.own[j] != OWNER_NONE) owner_bad = 1;
                    if (want && count == 0) complete_bad = 1;
                    if (!want && count != 0) overpaint_bad = 1;
                    if (count > 1) double_bad = 1;
                }
            }
        }
        CHECK(!complete_bad,
              "WindowMgr_invalidate_desktop covers every clipped rect pixel");
        CHECK(!overpaint_bad,
              "WindowMgr_invalidate_desktop marks no pixel outside the rect");
        CHECK(!owner_bad,
              "WindowMgr_invalidate_desktop assigns each pixel to its frontmost owner or desktop");
        CHECK(!double_bad,
              "WindowMgr_invalidate_desktop partitions damage without double-counting");
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
     * ACTIVATION-TRANSITION REPAINT (beads initech-v6t2 + initech-rqz5;
     * ADR-0004 D-5; epic initech-av7s DQ3, ratified 2026-07-31):
     * reaffirm_active must SEED a repaint on BOTH hilited transitions --
     * 1->0 (the window LOSING active status repaints its chrome inactive;
     * the v6t2 half) AND 0->1 (the window GAINING active status repaints
     * its chrome active + the part a raise just uncovered; the rqz5 half --
     * without it newly-active windows keep flat inactive title bars and
     * raise exposes stale pixels). Directed case first (hand-verifiable
     * geometry), then randomized properties for each half.
     *
     * MUTANT WINDOW_MUTATE_NO_DEACT_INVAL (Rule 6): suppresses ONLY the 1->0
     * seed -- the deactivation checks below go RED.
     * MUTANT WINDOW_MUTATE_NO_ACTIVATE_INVAL (Rule 6): suppresses ONLY the
     * 0->1 seed -- the activation checks below go RED.
     * ====================================================================== */
    {
        /* Directed: w2 created first (front), then w1 created overlapping it
         * (w1 becomes front, hilited=1) -- exactly the "w1 front, hilited=1"
         * setup the bug report describes. SelectWindow(w2) then deactivates
         * w1: w1->updateRgn must become non-empty and must equal w1's OWN
         * visible region post-select -- neither under- nor over-invalidated.
         * The expected pixels are hand-derived from the rects (Law 2: an
         * independent ground truth, not a call into visible_into/
         * ComputeVisible, the primitive the fix under test itself calls). */
        static win_store_t w1, w2; static mgr_store_t M;
        mgr_attach(&M, FRAME);

        win_attach(&w2);
        rgn_rect_t s2 = { 0, 0, 20, 24 };         /* w2 struc: y[0,20) x[0,24) */
        rgn_rect_t c2 = { 3, 1, 19, 23 };
        NewWindow(&M.wm, &w2.rec, s2, c2, documentKind, documentProc, 1);

        win_attach(&w1);
        rgn_rect_t s1 = { 10, 10, 30, 34 };       /* w1 struc: y[10,30) x[10,34), overlaps w2 */
        rgn_rect_t c1 = { 13, 11, 29, 33 };
        NewWindow(&M.wm, &w1.rec, s1, c1, documentKind, documentProc, 1);

        CHECK(w1.rec.hilited == 1 && M.wm.front == &w1.rec,
              "deactivation-repaint setup: w1 is front and hilited after creation");

        /* Clear construction-time transition damage (each NewWindow seeded the
         * new front's activation AND the old front's deactivation, initech-rqz5
         * / v6t2) so the deltas measured below come from the ONE SelectWindow
         * call under test -- the same isolation the randomized properties use. */
        WindowMgr_validate(&w1.rec);
        WindowMgr_validate(&w2.rec);

        SelectWindow(&M.wm, &w2.rec);             /* deactivates w1 (1->0), activates w2 (0->1) */

        CHECK(w1.rec.hilited == 0, "deactivation-repaint setup: w1 lost hilited after SelectWindow(w2)");
        CHECK(!region_is_empty(w1.rec.updateRgn),
              "SelectWindow: deactivated w1's updateRgn is seeded (non-empty)");

        /* A pixel in w1 NOT covered by w2 (y=25,x=15 -- below w2's y<20 band)
         * must be in w1's updateRgn: it is genuinely visible-and-owed. */
        CHECK(region_contains_point(w1.rec.updateRgn, 15, 25) != 0,
              "SelectWindow: w1's updateRgn covers a pixel actually visible after deactivation");
        /* A pixel w2 now covers (y=15,x=15 -- inside both structs, w2 is
         * front) must NOT be in w1's updateRgn: no over-invalidate past
         * w1's own currently-visible region. */
        CHECK(region_contains_point(w1.rec.updateRgn, 15, 15) == 0,
              "SelectWindow: w1's updateRgn does NOT cover a pixel w2 now occludes (no over-invalidate)");

        /* --- the ACTIVATION half (initech-rqz5): the raised w2 (0->1) is
         * seeded with its own post-raise visible region -- the previously-
         * occluded overlap it must now repaint plus the active-chrome flip. */
        CHECK(w2.rec.hilited == 1 && M.wm.front == &w2.rec,
              "activation-repaint setup: w2 is front and hilited after SelectWindow(w2)");
        CHECK(!region_is_empty(w2.rec.updateRgn),
              "SelectWindow: newly-activated w2's updateRgn is seeded (0->1, initech-rqz5)");
        /* The overlap pixel (y=15,x=15) was covered by w1 before the raise and
         * is w2's own visible pixel after it -- it MUST be in w2's seed (the
         * raise-exposure the old code never generated). */
        CHECK(region_contains_point(w2.rec.updateRgn, 15, 15) != 0,
              "SelectWindow: w2's updateRgn covers the previously-occluded overlap pixel (raise exposure)");
        /* (y=25,x=15) is OUTSIDE w2's structure (below its y<20 band) -- the
         * seed must not leak past w2's own visible region. */
        CHECK(region_contains_point(w2.rec.updateRgn, 15, 25) == 0,
              "SelectWindow: w2's updateRgn does NOT leak outside its structure (no over-invalidate)");
    }

    {
        /* Randomized: random overlapping stacks; SelectWindow a random
         * non-front window; the OLD front window's updateRgn must equal
         * EXACTLY its own currently-visible region, verified against the
         * INDEPENDENT owner-grid ground truth (rasterized front-to-back
         * ownership -- the same idiom PROPERTY 1/2 above use, Law 2: not a
         * re-call of visible_into/ComputeVisible, the primitive under test). */
        enum { CASES = 800, MAXW = 5 };
        int empty_bad = 0;      /* deactivated window's updateRgn wrongly stayed empty */
        int mismatch_bad = 0;   /* updateRgn != its own currently-visible region        */

        for (int t = 0; t < CASES && !empty_bad && !mismatch_bad; t++) {
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

            /* NewWindow always pushes to front, so the LAST created (index
             * n-1) is front/hilited right after the loop above. */
            int old_front = n - 1;

            /* Clear construction-time deactivation damage (same reasoning as
             * the MoveWindow/HideWindow properties above) so the ONLY damage
             * measured below is the one SelectWindow call under test. */
            for (int i = 0; i < n; i++) WindowMgr_validate(&W[i].rec);

            /* Pick a DIFFERENT window to select; always < old_front (== n-1),
             * so this always causes an actual 1->0 deactivation of old_front. */
            int sel = rnd(0, n - 2);

            SelectWindow(&M.wm, &W[sel].rec);

            owngrid_t after; build_owner_grid(&M.wm, idx, n, &after);
            uint8_t truth[GW * GH];
            for (int j = 0; j < GW * GH; j++)
                truth[j] = (after.own[j] == (uint8_t)old_front) ? 1 : 0;

            uint8_t updg[GW * GH];
            rasterize_set(W[old_front].rec.updateRgn, updg);

            int any_truth = 0;
            for (int j = 0; j < GW * GH; j++) if (truth[j]) { any_truth = 1; break; }
            if (any_truth) {
                int any_upd = 0;
                for (int j = 0; j < GW * GH; j++) if (updg[j]) { any_upd = 1; break; }
                if (!any_upd) empty_bad = 1;
            }

            for (int j = 0; j < GW * GH; j++)
                if (updg[j] != truth[j]) { mismatch_bad = 1; break; }
        }
        CHECK(!empty_bad,
              "SelectWindow: deactivated window's updateRgn is seeded whenever it is still visible");
        CHECK(!mismatch_bad,
              "SelectWindow: deactivated window's updateRgn == its visible region (owner-grid truth), 800 stacks");
    }

    {
        /* Randomized ACTIVATION property (initech-rqz5 / DQ3): the RAISED
         * window's updateRgn must equal EXACTLY its own post-raise visible
         * region -- verified against the INDEPENDENT owner-grid ground truth
         * (Law 2: the same rasterized front-to-back ownership idiom as the
         * deactivation property above, not a re-call of the primitive under
         * test). The raised window is front, so its visible region is its
         * whole in-frame structure; the seed must cover all of it (the
         * previously-occluded part included) and nothing more. */
        enum { CASES = 800, MAXW = 5 };
        int empty_bad = 0;      /* raised window's updateRgn wrongly stayed empty */
        int mismatch_bad = 0;   /* updateRgn != its own post-raise visible region */

        for (int t = 0; t < CASES && !empty_bad && !mismatch_bad; t++) {
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

            /* Clear construction-time transition damage so the ONLY damage
             * measured is the one SelectWindow call under test. */
            for (int i = 0; i < n; i++) WindowMgr_validate(&W[i].rec);

            /* Raise a window that is NOT currently front (index n-1 is front
             * after the creation loop), so the select is a real 0->1. */
            int sel = rnd(0, n - 2);

            SelectWindow(&M.wm, &W[sel].rec);

            owngrid_t after; build_owner_grid(&M.wm, idx, n, &after);
            uint8_t truth[GW * GH];
            for (int j = 0; j < GW * GH; j++)
                truth[j] = (after.own[j] == (uint8_t)sel) ? 1 : 0;

            uint8_t updg[GW * GH];
            rasterize_set(W[sel].rec.updateRgn, updg);

            int any_truth = 0;
            for (int j = 0; j < GW * GH; j++) if (truth[j]) { any_truth = 1; break; }
            if (any_truth) {
                int any_upd = 0;
                for (int j = 0; j < GW * GH; j++) if (updg[j]) { any_upd = 1; break; }
                if (!any_upd) empty_bad = 1;
            }

            for (int j = 0; j < GW * GH; j++)
                if (updg[j] != truth[j]) { mismatch_bad = 1; break; }
        }
        CHECK(!empty_bad,
              "SelectWindow: raised window's updateRgn is seeded (0->1 activation, initech-rqz5)");
        CHECK(!mismatch_bad,
              "SelectWindow: raised window's updateRgn == its post-raise visible region (owner-grid truth), 800 stacks");
    }

    /* ======================================================================
     * R1.1 FindWindow: every pixel in the full Platinum title band is graded
     * against independent 12x12 close/zoom/collapse rectangles. The one-pixel
     * outer highlights and every gap/frame pixel remain inDrag. The 18x18 grow
     * cell is likewise exact. Ref: sys8 window-chrome.md Sec 3.1/Sec 5;
     * beads initech-tbef/ci4o/cjfr.
     * ====================================================================== */
    {
        static win_store_t A; static mgr_store_t M;
        rgn_rect_t desk = { 0, 0, 120, 180 };
        rgn_rect_t s = { 20, 15, 100, 165 };
        rgn_rect_t c = CalcDocContentRect(s);
        mgr_attach(&M, desk);
        win_attach(&A);
        NewWindow(&M.wm, &A.rec, s, c, documentKind, zoomDocProc, 1);

        WindowPtr hit;
        flair_point_t p_content = { 60, 50 };
        CHECK(FindWindow(&M.wm, p_content, &hit) == inContent && hit == &A.rec,
              "FindWindow: point in content -> inContent");

        int title_bad = 0;
        int ri = s.right - 1;
        int box_y = s.top + FLAIR_CHROME_WIDGET_TOP_OFF;
        int close_x = s.left + FLAIR_CHROME_CLOSE_LEFT_OFF;
        int zoom_x = ri - FLAIR_CHROME_ZOOM_RIGHT_OFF;
        int collapse_x = ri - FLAIR_CHROME_COLLAPSE_RIGHT_OFF;
        for (int y = s.top; y < s.top + FLAIR_CHROME_TITLEBAR_H; y++) {
            for (int x = s.left; x < s.right; x++) {
                flair_part_code_t want = inDrag;
                if (x >= close_x && x < close_x + FLAIR_CHROME_WIDGET_BOX &&
                    y >= box_y && y < box_y + FLAIR_CHROME_WIDGET_BOX)
                    want = inGoAway;
                else if (x >= zoom_x && x < zoom_x + FLAIR_CHROME_WIDGET_BOX &&
                         y >= box_y && y < box_y + FLAIR_CHROME_WIDGET_BOX)
                    want = inZoomIn;
                else if (x >= collapse_x &&
                         x < collapse_x + FLAIR_CHROME_WIDGET_BOX &&
                         y >= box_y && y < box_y + FLAIR_CHROME_WIDGET_BOX)
                    want = inCollapse;
                flair_point_t p = { (int16_t)y, (int16_t)x };
                if (FindWindow(&M.wm, p, &hit) != want || hit != &A.rec) {
                    title_bad = 1;
                    break;
                }
            }
            if (title_bad) break;
        }
        CHECK(!title_bad,
              "FindWindow: full title band == exact drawn 12x12 close/zoom/collapse boxes only");

        A.rec.zoomed = 1;
        flair_point_t p_zoom = { (int16_t)box_y, (int16_t)zoom_x };
        CHECK(FindWindow(&M.wm, p_zoom, &hit) == inZoomOut && hit == &A.rec,
              "FindWindow: zoom widget reports inZoomOut while standard-state is active");
        A.rec.zoomed = 0;

        int grow_bad = 0;
        int grow_x = ri - FLAIR_CHROME_GROW_RIGHT_OFF;
        int grow_y = (s.bottom - 1) - FLAIR_CHROME_GROW_BOTTOM_OFF;
        for (int y = grow_y; y < grow_y + FLAIR_CHROME_GROW; y++) {
            for (int x = grow_x; x < grow_x + FLAIR_CHROME_GROW; x++) {
                flair_point_t p = { (int16_t)y, (int16_t)x };
                if (FindWindow(&M.wm, p, &hit) != inGrow || hit != &A.rec)
                    grow_bad = 1;
            }
        }
        CHECK(!grow_bad,
              "FindWindow: every pixel of the sampled drawn 18x18 grow cell is inGrow");

        flair_point_t p_desk = { 1, 1 };
        CHECK(FindWindow(&M.wm, p_desk, &hit) == inDesk && hit == NULL,
              "FindWindow: point on desktop -> inDesk, whichWindow=NULL");

        /* Front-most resolution: two overlapping windows; the front one wins. */
        static win_store_t B;
        win_attach(&B);
        NewWindow(&M.wm, &B.rec, s, c, documentKind, zoomDocProc, 1);
        CHECK(FindWindow(&M.wm, p_content, &hit) == inContent && hit == &B.rec,
              "FindWindow: overlap -> front-most window wins");
        SelectWindow(&M.wm, &A.rec);              /* raise A */
        CHECK(FindWindow(&M.wm, p_content, &hit) == inContent && hit == &A.rec,
              "FindWindow: after SelectWindow, raised window wins the hit");
    }

    /* ======================================================================
     * R1.2 zoom/grow/collapse transaction contracts. A one-window 200x160
     * desktop makes underlying exposure equal desktop_update, while the actor's
     * new visible footprint must equal updateRgn. Every comparison is exact.
     * ====================================================================== */
    {
        static win_store_t W;
        static mgr_store_t M;
        static uint8_t old_owned[OP_W * OP_H];
        rgn_rect_t desk = { 0, 0, OP_H, OP_W };
        rgn_rect_t user = { 35, 80, 159, 199 };
        rgn_rect_t standard = {
            FLAIR_CHROME_ZOOM_MARGIN_TOP,
            FLAIR_CHROME_ZOOM_MARGIN_LEFT,
            OP_H - FLAIR_CHROME_ZOOM_MARGIN_BOTTOM,
            OP_W - FLAIR_CHROME_ZOOM_MARGIN_RIGHT
        };
        mgr_attach(&M, desk);
        win_attach(&W);
        NewDocumentWindow(&M.wm, &W.rec, user, documentKind, 1);
        WindowMgr_validate(&W.rec);
        region_set_empty(M.wm.desktop_update);

        snapshot_op_region(W.rec.strucRgn, old_owned);
        CHECK(ZoomWindow(&M.wm, &W.rec) == 1 && W.rec.zoomed,
              "ZoomWindow: first click enters deterministic standard-state");
        CHECK(rect_same(WindowFrameRect(&W.rec), standard) &&
              rect_same(region_get_bbox(W.rec.contRgn),
                        CalcDocContentRect(standard)),
              "ZoomWindow: standard frame and derived content are exact");
        CHECK(geometry_damage_exact(old_owned, &W.rec, &M.wm),
              "ZoomWindow in: old loss and new self footprint are exact damage");

        WindowMgr_validate(&W.rec);
        region_set_empty(M.wm.desktop_update);
        snapshot_op_region(W.rec.strucRgn, old_owned);
        CHECK(ZoomWindow(&M.wm, &W.rec) == 0 && !W.rec.zoomed,
              "ZoomWindow: second click leaves standard-state");
        CHECK(rect_same(WindowFrameRect(&W.rec), user) &&
              rect_same(region_get_bbox(W.rec.contRgn), CalcDocContentRect(user)),
              "ZoomWindow: second click restores userState exactly");
        CHECK(geometry_damage_exact(old_owned, &W.rec, &M.wm),
              "ZoomWindow out: old loss and restored self footprint are exact damage");

        WindowMgr_validate(&W.rec);
        region_set_empty(M.wm.desktop_update);
        snapshot_op_region(W.rec.strucRgn, old_owned);
        SizeWindow(&M.wm, &W.rec, 1, 1);
        rgn_rect_t grown = { user.top, user.left,
                             user.top + FLAIR_CHROME_WINDOW_MIN_H,
                             user.left + FLAIR_CHROME_WINDOW_MIN_W };
        CHECK(rect_same(WindowFrameRect(&W.rec), grown) &&
              rect_same(region_get_bbox(W.rec.contRgn), CalcDocContentRect(grown)),
              "SizeWindow: 1x1 request clamps to 96x64 and re-derives content");
        CHECK(geometry_damage_exact(old_owned, &W.rec, &M.wm),
              "SizeWindow: old loss and resized self footprint are exact damage");

        WindowMgr_validate(&W.rec);
        region_set_empty(M.wm.desktop_update);
        snapshot_op_region(W.rec.strucRgn, old_owned);
        SizeWindow(&M.wm, &W.rec, 300, 300);
        rgn_rect_t maximum = { user.top, user.left, OP_H, OP_W };
        CHECK(rect_same(WindowFrameRect(&W.rec), maximum) &&
              rect_same(region_get_bbox(W.rec.contRgn),
                        CalcDocContentRect(maximum)),
              "SizeWindow: oversize request clamps to the available desktop maximum");
        CHECK(geometry_damage_exact(old_owned, &W.rec, &M.wm),
              "SizeWindow max: old loss and enlarged self footprint are exact damage");

        rgn_rect_t expanded = WindowFrameRect(&W.rec);
        WindowMgr_validate(&W.rec);
        region_set_empty(M.wm.desktop_update);
        snapshot_op_region(W.rec.strucRgn, old_owned);
        CHECK(CollapseWindow(&M.wm, &W.rec) == 1 && W.rec.collapsed,
              "CollapseWindow: first click enters windowshade state");
        rgn_rect_t shade = expanded;
        shade.bottom = (int16_t)(shade.top + FLAIR_CHROME_TITLEBAR_H);
        CHECK(rect_same(WindowFrameRect(&W.rec), shade) &&
              region_is_empty(W.rec.contRgn),
              "CollapseWindow: structure is title-band-only and contRgn is empty");
        CHECK(geometry_damage_exact(old_owned, &W.rec, &M.wm),
              "CollapseWindow: removed body is exact underlying exposure");

        WindowMgr_validate(&W.rec);
        region_set_empty(M.wm.desktop_update);
        snapshot_op_region(W.rec.strucRgn, old_owned);
        CHECK(CollapseWindow(&M.wm, &W.rec) == 0 && !W.rec.collapsed,
              "CollapseWindow: second click restores expanded state");
        CHECK(rect_same(WindowFrameRect(&W.rec), expanded) &&
              rect_same(region_get_bbox(W.rec.contRgn),
                        CalcDocContentRect(expanded)),
              "CollapseWindow: restore frame and content are exact");
        CHECK(geometry_damage_exact(old_owned, &W.rec, &M.wm),
              "CollapseWindow restore: expanded self footprint is exact damage");
    }

    /* ======================================================================
     * HideWindow exposure: hiding a window damages exactly what it owned
     * (independent owner-grid before/after) -- PLUS, when the hidden window
     * was the ACTIVE one, the newly-activated successor's whole post-hide
     * visible region (the 0->1 activation seed, initech-rqz5 / DQ3: the
     * successor owes an active-chrome repaint across its full width, not
     * just the exposed strip -- the WL-0075 s05 half-active-bar bug).
     * Damage is complete + sound against that union.
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
            /* Ref: beads initech-v6t2/-rqz5 -- clear construction-time
             * transition damage (both hilited halves) before measuring
             * HideWindow's own delta. */
            for (int i = 0; i < n; i++) WindowMgr_validate(&W[i].rec);
            int hw = rnd(0, n - 1);
            int hw_was_active = (W[hw].rec.hilited != 0);
            owngrid_t before; build_owner_grid(&M.wm, idx, n, &before);
            HideWindow(&M.wm, &W[hw].rec);
            owngrid_t after; build_owner_grid(&M.wm, idx, n, &after);

            /* The independent expected golden: the hidden window's owned
             * pixels, UNION the successor's post-hide visible region iff the
             * hide flipped the active window (DQ3). The successor is read off
             * the hilited flags -- window.c's own state, but the EXPECTED
             * pixel set for it comes from the after owner-grid, never from
             * the updateRgn under test. */
            uint8_t truth[GW * GH];
            for (int j = 0; j < GW * GH; j++)
                truth[j] = (before.own[j] == (uint8_t)hw) ? 1 : 0;
            if (hw_was_active) {
                int nf = -1;
                for (int i = 0; i < n; i++)
                    if (i != hw && W[i].rec.hilited) { nf = i; break; }
                if (nf >= 0)
                    for (int j = 0; j < GW * GH; j++)
                        if (after.own[j] == (uint8_t)nf) truth[j] = 1;
            }

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
        }
        CHECK(!bad, "HideWindow damage == the hidden window's owned pixels + the successor's activation seed (DQ3)");
    }

    return TEST_SUMMARY("test_window");
}
