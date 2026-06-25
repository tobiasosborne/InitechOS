/* test_interact.c -- the FLAIR live-loop INTERACTION oracle (HOST; FO-9 HOST leg).
 *
 * beads: initech-5l5z step FO-9 (the `test-interact` behavioural oracle, HOST).
 *        ADR-0006 E-D5(A) / Sec 4.1: the HOST internal-geometry oracle for the
 *        FLAIR live cooperative WaitNextEvent pump (FO-7/8). It grades the SAME
 *        window.c verbs the live pump will drive (FindWindow part-code routing,
 *        DragWindow geometry, z-order visible regions) -- the BINDING leg for the
 *        ungoldenable INTERNAL geometry (the EMU look-leg `test-flair-drag` comes
 *        LATER and is NOT graded here; ADR-0006 E-D5/E-D8, BC-5/BC-6).
 *
 * WHY HOST + INDEPENDENT-BY-RECOMPUTATION (Law 2; ADR-0006 E-D5(A), Sec 2.3):
 *   The post-interaction internal state (which window/part a click routes to, the
 *   exact drag delta, the visible/occluded region) is NOT externally observable in
 *   any period capture -- the same reason ADR-0005 ratified the region homomorphism
 *   suite as the ENTIRE region signal. It can only be graded against a SECOND,
 *   DIFFERENT in-host recomputation. So this oracle NEVER reads the answer back out
 *   of the verb under test: every expected value is recomputed HERE by independent
 *   arithmetic / an independent front-to-back owner-grid, and diffed against the
 *   artifact (window.c). An oracle that asked FindWindow what FindWindow returned
 *   would agree BY CONSTRUCTION and is forbidden (Law 2; HER-02/HER-14).
 *
 * THE THREE LEGS (ADR-0006 Sec 4.1; window.h Sec 3/4/5):
 *
 *   LEG 1 -- FindWindow part-code routing.  A 2-window z-ordered scene; every
 *     probe point (a FULL deterministic grid sweep, plus directed hand-known
 *     anchors) is classified by an INDEPENDENT from-spec part-code classifier
 *     (expected_part_code, which does NOT call FindWindow) and diffed against the
 *     artifact's FindWindow: inContent / inDrag / inGoAway / inDesk, and the FRONT
 *     window winning a shared overlap point. (window.h Sec 4; window_record.h Sec1.)
 *
 *   LEG 2 -- DragWindow geometry.  For a window at known bounds, DragWindow(dh,dv)
 *     must translate strucRgn AND contRgn by EXACTLY (dh,dv).  Several deltas incl.
 *     negative + mixed; the expected post-drag bbox is recomputed HERE as
 *     old-bbox + (dh,dv), NOT read from DragWindow's own region_get_bbox path.
 *     (window.h Sec 3; the live pump's inDrag dispatch calls this per step.)
 *
 *   LEG 3 -- Z-order + visible regions (incl. an occlusion-CHANGING drag).  Two
 *     overlapping windows; visible(W) = strucRgn(W) DIFF union-of-fronts is graded
 *     bit-exact against an INDEPENDENT first-writer-wins owner-grid (rasterized off
 *     the rows/x-lists, NOT via ComputeVisible), AND against a hand-computed
 *     occluded rectangle for the simple 2-rect case.  Then the BACK window is
 *     dragged out from under the FRONT one and visible(W) is re-graded -- the
 *     occlusion-changing drag the live desktop must get right (window.h Sec 3/5).
 *
 * RED -> GREEN (Rule 1).  This oracle is GREEN against the current CORRECT window.c
 *   (it grades existing, ratified behaviour the pump will merely DRIVE; the wiring
 *   is FO-7/8, not new algorithm -- ADR-0006 E-D2/E-D4).
 *
 * MUTATION-PROVEN (Rule 6; the self-mutation pattern).  This file's only editable
 *   surface is the oracle ITSELF (window.c is owned by a parallel lane and is NOT
 *   touched here), so the mutants perturb the INDEPENDENT EXPECTED-value
 *   recomputation -- a self-mutation that proves each check is a LIVE comparison
 *   between two independently-computed values, not a tautology.  The orchestrator
 *   compiles each `-DINTERACT_MUT_*` and confirms the binary goes RED (exit != 0):
 *
 *     INTERACT_MUT_DRAG_NOOP        -- the expected drag delta is forced to (0,0)
 *       (the window is EXPECTED to stay put).  The CORRECT DragWindow still moves
 *       by (dh,dv), so LEG 2's bbox equality goes RED.  Proves the geometry check
 *       actually demands movement -- i.e. it would catch the HER-14 static-frame
 *       drag-noop (a DragWindow that ignored its delta).  (ADR-0006 M1 analogue.)
 *
 *     INTERACT_MUT_FINDWINDOW_OFFBYONE -- the independent classifier's close/zoom
 *       box width (tb) is biased by +1px.  The CORRECT FindWindow uses the true
 *       band, so at the perturbed boundary the two disagree and LEG 1's sweep goes
 *       RED.  Proves the part-code routing check is a live differential that bites
 *       a 1px hit-test error (an inGoAway-vs-inDrag boundary slip).
 *
 *     INTERACT_MUT_VISIBLE_IGNORE_FRONT -- the independent owner-grid skips the
 *       FRONT window, so the EXPECTED visible set ignores occlusion.  The CORRECT
 *       ComputeVisible subtracts the front, so LEG 3's occluded-scene check goes
 *       RED.  Proves the visible-region check actually subtracts union-of-fronts
 *       -- it would catch the z-order-ignoring bug (window.c WINDOW_MUTATE_ZORDER).
 *
 *   Each mutant introduces >=1 failing CHECK; any failure makes TEST_SUMMARY return
 *   non-zero (test_assert.h), so the mutant build is RED while the default is GREEN.
 *
 * Ref:  ADR-0006 E-D1 (oracle-first), E-D2 (single spine -- the live binary runs
 *       these exact symbols, inheriting this proof), E-D5(A) (HOST internal
 *       geometry, independent-by-recomputation), Sec 4.1; window.h Sec 3/4/5;
 *       spec/window_record.h Sec 1 (part-codes); spec/region_algebra.h
 *       (region_get_bbox / region_contains_point / half-open span convention).
 *       CLAUDE.md Law 2 (oracle is truth, independent golden), Rule 1 (RED->GREEN),
 *       Rule 6 (mutation-proven), Rule 11 (deterministic -- fixed scenes/deltas +
 *       a full deterministic grid sweep, no randomness), Rule 12 (ASCII-clean).
 *       Mirrors harness/proptest/test_window.c + test_drag.c scaffolding so this
 *       dual-compiles the same way (the orchestrator models the target on test-window).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "region_algebra.h"   /* the LOCKED region contract (-Ispec)             */
#include "region.h"           /* engine constructors (-Ios/flair/atkinson)       */
#include "window_record.h"    /* WindowRecord, part-codes (-Ispec)               */
#include "window.h"           /* the Window Manager under test (-Ios/flair)      */
#include "test_assert.h"      /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)        */

TEST_HARNESS();

/* ===========================================================================
 * Self-mutation knobs (Rule 6).  Default 0 / disabled; the orchestrator flips
 * exactly one `-DINTERACT_MUT_*` to prove the matching check BITES.  See file
 * header for what each mutant proves.
 * ===========================================================================*/
#ifdef INTERACT_MUT_FINDWINDOW_OFFBYONE
#  define INTERACT_TB_BIAS  1   /* close/zoom box width biased +1px (LEG 1)       */
#else
#  define INTERACT_TB_BIAS  0
#endif

/* ===========================================================================
 * The bounded grid + a per-pixel OWNER ground truth (the independent oracle for
 * LEG 3 visible regions).  OWNER_NONE = bare desktop; else the owner is a window
 * INDEX.  Windows are rasterized FRONT-to-BACK, first writer wins (front-most) --
 * exactly z-order occlusion -- computed WITHOUT any Window Manager region op, so a
 * WM bug cannot mask an oracle bug (the test_window idiom).
 * ===========================================================================*/
enum { GW = 64, GH = 48 };               /* grid width/height (pixels)           */
enum { OWNER_NONE = 255 };               /* bare-desktop sentinel                 */

/* ===========================================================================
 * Region storage bundles (the test_window/test_region rgn_store arena: the engine
 * never mallocs; the CALLER supplies rows[]/x_pool).  One bundle per region.
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
    w->rec.strucRgn   = &w->struc.r;
    w->rec.contRgn    = &w->cont.r;
    w->rec.updateRgn  = &w->upd.r;
    w->rec.nextWindow = NULL;
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
 * Rasterize a region into a flat 0/1 membership grid, straight off rows/x-lists
 * (the test_window idiom -- INDEPENDENT of region_contains_point).
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
 * owner = the window's index in `idx`.  Pixels no visible window covers are
 * OWNER_NONE.  Rasterized off the struc bbox (INDEPENDENT of any region op).
 *
 * MUTANT INTERACT_MUT_VISIBLE_IGNORE_FRONT (Rule 6): SKIP the front-most window so
 * the EXPECTED visible set ignores occlusion.  The CORRECT ComputeVisible still
 * subtracts the front, so LEG 3's occluded-scene check goes RED -- proving the
 * visible-region check truly subtracts union-of-fronts.
 * ===========================================================================*/
static void build_owner_grid(const WindowMgr *wm, win_store_t *const *idx, int n,
                             uint8_t *own /* GW*GH */)
{
    memset(own, OWNER_NONE, (size_t)GW * GH);
    for (const WindowRecord *p = wm->front; p != NULL; p = p->nextWindow) {
        if (!p->visible) continue;
#ifdef INTERACT_MUT_VISIBLE_IGNORE_FRONT
        if (p == wm->front) continue;   /* self-mutation: drop the front occluder */
#endif
        int wi = -1;
        for (int i = 0; i < n; i++) if (&idx[i]->rec == p) { wi = i; break; }
        if (wi < 0) continue;
        rgn_rect_t s = region_get_bbox(p->strucRgn);
        for (int y = s.top; y < s.bottom; y++) {
            if (y < 0 || y >= GH) continue;
            for (int x = s.left; x < s.right; x++) {
                if (x < 0 || x >= GW) continue;
                if (own[y * GW + x] == OWNER_NONE) own[y * GW + x] = (uint8_t)wi;
            }
        }
    }
}

/* ===========================================================================
 * INDEPENDENT geometry helpers (rect arithmetic; do NOT call into window.c).
 * ===========================================================================*/
static int rect_is_empty(rgn_rect_t r)
{
    return (r.right <= r.left) || (r.bottom <= r.top);
}

/* Half-open membership [top,bottom) x [left,right) -- the LOCKED region span
 * convention (region_algebra.h Sec 1).  Mirrors region_contains_point for a
 * rectangular region, computed by a SEPARATE arithmetic path. */
static int rect_contains(rgn_rect_t r, int16_t h, int16_t v)
{
    if (rect_is_empty(r)) return 0;
    return (v >= r.top && v < r.bottom && h >= r.left && h < r.right);
}

static int rect_eq(rgn_rect_t a, rgn_rect_t b)
{
    return a.top == b.top && a.left == b.left &&
           a.bottom == b.bottom && a.right == b.right;
}

static rgn_rect_t rect_offset(rgn_rect_t r, int16_t dh, int16_t dv)
{
    rgn_rect_t o;
    o.top    = (int16_t)(r.top + dv);
    o.left   = (int16_t)(r.left + dh);
    o.bottom = (int16_t)(r.bottom + dv);
    o.right  = (int16_t)(r.right + dh);
    return o;
}

/* ===========================================================================
 * LEG 1: the INDEPENDENT part-code classifier (a from-spec reimplementation of
 * the FindWindow routing rules; window.h Sec 4 / window_record.h Sec 1).  It
 * walks FRONT-to-BACK, classifies the hit by chrome sub-band from the STORED
 * rects, and NEVER calls FindWindow -- so a hit-test bug in window.c (or in the
 * classifier) makes the two DISAGREE (the differential / port-and-verify shape).
 * Returns the part-code and, via *which, the front-most hit window index (or -1
 * for inDesk).
 * ===========================================================================*/
typedef struct iwin {
    rgn_rect_t struc;
    rgn_rect_t cont;
    int16_t    variant;
    uint8_t    goAway;
    uint8_t    visible;
    WindowPtr  rec;          /* artifact record -- to cross-map FindWindow's hit  */
} iwin_t;

static flair_part_code_t expected_part_code(const iwin_t *w, int n,
                                            flair_point_t pt, int *which)
{
    int16_t h = pt.h, v = pt.v;
    for (int i = 0; i < n; i++) {                 /* front-to-back               */
        if (!w[i].visible) continue;
        if (!rect_contains(w[i].struc, h, v)) continue;
        if (which) *which = i;

        /* content first (the most common hit). */
        if (rect_contains(w[i].cont, h, v)) return inContent;

        rgn_rect_t s = w[i].struc, c = w[i].cont;
        int16_t tb = (int16_t)(c.top - s.top);    /* title-bar band height       */
        if (tb < 1) tb = 1;
        /* the close/zoom box width; INTERACT_TB_BIAS is 0 by default and +1px
         * under INTERACT_MUT_FINDWINDOW_OFFBYONE (the self-mutation). */
        int16_t tb_eff = (int16_t)(tb + INTERACT_TB_BIAS);

        /* go-away (close) box: a tb-square at the top-left of the title bar. */
        if (w[i].goAway) {
            if (h >= s.left && h < (int16_t)(s.left + tb_eff) &&
                v >= s.top  && v < c.top)
                return inGoAway;
        }
        /* zoom box: a tb-square at the top-right of the title bar (zoom variants).*/
        if (w[i].variant == zoomDocProc || w[i].variant == zoomNoGrow) {
            if (h >= (int16_t)(s.right - tb_eff) && h < s.right &&
                v >= s.top && v < c.top)
                return inZoomIn;
        }
        /* grow box: a square at the bottom-right corner (grow variants). */
        if (w[i].variant == documentProc || w[i].variant == zoomDocProc) {
            int16_t gb = (int16_t)(s.bottom - c.bottom);
            if (gb < 1) gb = 1;
            if (h >= (int16_t)(s.right - gb) && h < s.right &&
                v >= (int16_t)(s.bottom - gb) && v < s.bottom)
                return inGrow;
        }
        return inDrag;                            /* any other chrome pixel      */
    }
    if (which) *which = -1;
    return inDesk;
}

/* map a FindWindow hit pointer to its iwin index (-1 if NULL / not found). */
static int idx_of(const iwin_t *w, int n, WindowPtr hit)
{
    if (hit == NULL) return -1;
    for (int i = 0; i < n; i++) if (w[i].rec == hit) return i;
    return -2;   /* a hit pointer not in the scene: a bug -- never equals -1     */
}

/* ===========================================================================
 * LEG 1 -- FindWindow part-code routing (directed anchors + full grid sweep).
 * ===========================================================================*/
static void leg1_findwindow(void)
{
    rgn_rect_t FRAME = { 0, 0, GH, GW };
    static win_store_t W[2];
    static mgr_store_t M;
    mgr_attach(&M, FRAME);

    /* Two document windows; A created first (back), B created second (front). */
    rgn_rect_t As = { 8,  6, 30, 34 }, Ac = { 11,  7, 29, 33 };
    rgn_rect_t Bs = { 18, 24, 42, 56 }, Bc = { 21, 25, 41, 55 };
    win_attach(&W[0]);
    NewWindow(&M.wm, &W[0].rec, As, Ac, documentKind, documentProc, 1);   /* A */
    win_attach(&W[1]);
    NewWindow(&M.wm, &W[1].rec, Bs, Bc, documentKind, documentProc, 1);   /* B front */

    /* INDEPENDENT scene mirror, FRONT-to-BACK (NewWindow pushes front, so the
     * front-most is the LAST created): [B, A]. */
    iwin_t scene[2] = {
        { Bs, Bc, documentProc, 1, 1, &W[1].rec },   /* index 0 = B (front)       */
        { As, Ac, documentProc, 1, 1, &W[0].rec },   /* index 1 = A (back)        */
    };
    enum { NSC = 2 };

    /* --- directed, hand-known anchors (independent: derived by hand from the
     *     geometry above, NOT from FindWindow). --- */
    WindowPtr hit;
    flair_point_t p_content = { 30, 40 };   /* in B content [21,41)x[25,55)       */
    CHECK(FindWindow(&M.wm, p_content, &hit) == inContent && hit == &W[1].rec,
          "LEG1: point in B content -> inContent on B");

    flair_point_t p_title = { 19, 40 };     /* B title band [18,21), clear of boxes*/
    CHECK(FindWindow(&M.wm, p_title, &hit) == inDrag && hit == &W[1].rec,
          "LEG1: point in B title bar -> inDrag on B");

    flair_point_t p_close = { 19, 25 };     /* B close box [24,27)x[18,21)         */
    CHECK(FindWindow(&M.wm, p_close, &hit) == inGoAway && hit == &W[1].rec,
          "LEG1: point in B close box -> inGoAway on B");

    flair_point_t p_desk = { 2, 2 };        /* far from any window                 */
    CHECK(FindWindow(&M.wm, p_desk, &hit) == inDesk && hit == NULL,
          "LEG1: bare-desktop point -> inDesk, whichWindow=NULL");

    /* FRONT wins a shared overlap point: (v=25,h=28) is inside A's struc AND B's
     * struc; B is front, so FindWindow must report B (and it is B content). */
    flair_point_t p_overlap = { 25, 28 };
    CHECK(rect_contains(As, 28, 25) && rect_contains(Bs, 28, 25),
          "LEG1: overlap probe is genuinely inside BOTH windows (meaningful)");
    CHECK(FindWindow(&M.wm, p_overlap, &hit) == inContent && hit == &W[1].rec,
          "LEG1: shared overlap point -> FRONT window (B) wins, not the back (A)");

    /* raising the BACK window flips the hit -- z-order drives hit-testing. */
    SelectWindow(&M.wm, &W[0].rec);                 /* raise A to front            */
    CHECK(FindWindow(&M.wm, p_overlap, &hit) == inContent && hit == &W[0].rec,
          "LEG1: after SelectWindow(A), the raised window (A) wins the overlap hit");
    SelectWindow(&M.wm, &W[1].rec);                 /* restore B front for the sweep*/

    /* --- the FULL deterministic grid sweep: every pixel diffed FindWindow vs the
     *     independent classifier (part-code AND which-window). Rule 11: exhaustive,
     *     not random. --- */
    int part_bad = 0, which_bad = 0, first_v = -1, first_h = -1;
    for (int v = 0; v < GH; v++) {
        for (int h = 0; h < GW; h++) {
            flair_point_t pt = { (int16_t)v, (int16_t)h };
            int ewhich = -1;
            flair_part_code_t ep = expected_part_code(scene, NSC, pt, &ewhich);
            WindowPtr ahit = NULL;
            flair_part_code_t ap = FindWindow(&M.wm, pt, &ahit);
            int awhich = idx_of(scene, NSC, ahit);
            if (ap != ep) {
                if (!part_bad) { first_v = v; first_h = h; }
                part_bad = 1;
            }
            if (awhich != ewhich) which_bad = 1;
        }
    }
    CHECK(!part_bad,
          "LEG1: FindWindow part-code == independent classifier over the full grid");
    CHECK(!which_bad,
          "LEG1: FindWindow whichWindow == independent front-most owner over the full grid");
    if (part_bad)
        fprintf(stderr, "    LEG1: first part-code mismatch at (v=%d,h=%d)\n",
                first_v, first_h);
}

/* ===========================================================================
 * LEG 2 -- DragWindow geometry (strucRgn + contRgn translate by EXACTLY (dh,dv)).
 * Several deltas incl. negative + mixed; the expected bbox is recomputed HERE as
 * old + delta, NOT read from DragWindow's own path.
 *
 * MUTANT INTERACT_MUT_DRAG_NOOP (Rule 6): the EXPECTED delta is forced to (0,0)
 * (the window is expected to stay put).  The CORRECT DragWindow still moves by
 * (dh,dv), so the bbox equality goes RED -- proving the check demands real
 * movement (it would catch a HER-14 drag-noop / static frame).
 * ===========================================================================*/
static void leg2_drag_geometry(void)
{
    rgn_rect_t FRAME = { 0, 0, GH, GW };
    static win_store_t W;
    static mgr_store_t M;
    mgr_attach(&M, FRAME);

    rgn_rect_t s0 = { 15, 15, 35, 45 }, c0 = { 18, 16, 34, 44 };  /* known bounds */
    win_attach(&W);
    NewWindow(&M.wm, &W.rec, s0, c0, documentKind, documentProc, 1);

    /* fixed deltas: +, -, mixed (Rule 11). Cumulative, since DragWindow is a
     * delta from the window's CURRENT position. */
    static const int16_t DH[] = {  8, -3,  6, -9 };
    static const int16_t DV[] = {  5, -2, -4,  7 };
    enum { NSTEP = (int)(sizeof DH / sizeof DH[0]) };

    /* independently-tracked expected position (start = the constructed bounds). */
    rgn_rect_t exp_s = s0, exp_c = c0;

    int struc_bad = 0, cont_bad = 0;
    for (int i = 0; i < NSTEP; i++) {
        /* the artifact ALWAYS moves by the REAL delta. */
        DragWindow(&M.wm, &W.rec, DH[i], DV[i]);

        /* the EXPECTED delta (self-mutation knob): (0,0) under DRAG_NOOP. */
        int16_t edh = DH[i], edv = DV[i];
#ifdef INTERACT_MUT_DRAG_NOOP
        edh = 0; edv = 0;
#endif
        exp_s = rect_offset(exp_s, edh, edv);
        exp_c = rect_offset(exp_c, edh, edv);

        rgn_rect_t got_s = region_get_bbox(W.rec.strucRgn);
        rgn_rect_t got_c = region_get_bbox(W.rec.contRgn);
        if (!rect_eq(got_s, exp_s)) struc_bad = 1;
        if (!rect_eq(got_c, exp_c)) cont_bad = 1;
    }
    CHECK(!struc_bad,
          "LEG2: DragWindow translates strucRgn by EXACTLY (dh,dv) over all deltas");
    CHECK(!cont_bad,
          "LEG2: DragWindow translates contRgn by EXACTLY (dh,dv) over all deltas");

    /* a single explicit negative-delta anchor for clarity. */
    rgn_rect_t before = region_get_bbox(W.rec.strucRgn);
    DragWindow(&M.wm, &W.rec, -6, -4);
    rgn_rect_t after = region_get_bbox(W.rec.strucRgn);
#ifdef INTERACT_MUT_DRAG_NOOP
    rgn_rect_t want = before;                 /* mutant: expect no move           */
#else
    rgn_rect_t want = rect_offset(before, -6, -4);
#endif
    CHECK(rect_eq(after, want),
          "LEG2: negative-delta drag (-6,-4) moves the struct bbox by exactly (-6,-4)");
}

/* ===========================================================================
 * LEG 3 -- Z-order + visible regions + the occlusion-CHANGING drag.
 * visible(W) = strucRgn(W) DIFF union-of-fronts, graded bit-exact against the
 * INDEPENDENT owner-grid AND a hand-computed occluded rect for the simple 2-rect
 * case; then the BACK window is dragged out from under the FRONT and re-graded.
 * ===========================================================================*/
static int visible_equals_owner(const WindowMgr *wm, win_store_t *const *idx,
                                int n, int wi, region_t *vis_scratch)
{
    /* INDEPENDENT ground truth: the owner-set of window wi. */
    static uint8_t own[GW * GH];
    build_owner_grid(wm, idx, n, own);

    /* the artifact's computed visible region, rasterized off its rows/x-lists. */
    ComputeVisible(wm, &idx[wi]->rec, vis_scratch);
    static uint8_t vg[GW * GH];
    rasterize_set(vis_scratch, vg);

    for (int j = 0; j < GW * GH; j++) {
        int want = (own[j] == (uint8_t)wi) ? 1 : 0;
        if (vg[j] != want) return 0;
    }
    return 1;
}

static void leg3_visible_zorder(void)
{
    rgn_rect_t FRAME = { 0, 0, GH, GW };
    static win_store_t W[2];           /* W[0] = B (back), W[1] = F (front)        */
    win_store_t *idx[2] = { &W[0], &W[1] };
    static mgr_store_t M;
    static rgn_store_t VIS;
    mgr_attach(&M, FRAME);
    store_attach(&VIS);

    rgn_rect_t Bs = { 10, 10, 30, 30 }, Bc = { 13, 11, 29, 29 };   /* back         */
    rgn_rect_t Fs = { 20, 20, 40, 40 }, Fc = { 23, 21, 39, 39 };   /* front        */
    win_attach(&W[0]);
    NewWindow(&M.wm, &W[0].rec, Bs, Bc, documentKind, documentProc, 1);  /* B back  */
    win_attach(&W[1]);
    NewWindow(&M.wm, &W[1].rec, Fs, Fc, documentKind, documentProc, 1);  /* F front */

    /* --- BEFORE: B is partly occluded by F.  visible(B) == owner-set(B). --- */
    CHECK(visible_equals_owner(&M.wm, idx, 2, 0, &VIS.r),
          "LEG3: occluded visible(B) == independent owner-set(B) (strucRgn DIFF fronts)");
    CHECK(visible_equals_owner(&M.wm, idx, 2, 1, &VIS.r),
          "LEG3: front visible(F) == independent owner-set(F) (full F, nothing in front)");

    /* hand-computed SIMPLE 2-rect case: the occluded rectangle is EXACTLY the
     * intersection [20,30)x[20,30) (B's bottom-right corner under F's top-left);
     * those 100 pixels must NOT be in visible(B), and a non-overlap sample MUST. */
    ComputeVisible(&M.wm, &W[0].rec, &VIS.r);
    int occluded_leak = 0, occluded_count = 0;
    for (int v = 20; v < 30; v++)
        for (int h = 20; h < 30; h++) {
            occluded_count++;
            if (region_contains_point(&VIS.r, (int16_t)h, (int16_t)v)) occluded_leak = 1;
        }
    CHECK(occluded_count == 100, "LEG3: the hand-computed occluded rect is 10x10=100 px (meaningful)");
    CHECK(!occluded_leak,
          "LEG3: visible(B) excludes EVERY pixel of the occluded rect [20,30)x[20,30)");
    CHECK(region_contains_point(&VIS.r, 12, 12),
          "LEG3: visible(B) includes a non-overlapped sample (v=12,h=12)");

    /* --- the OCCLUSION-CHANGING drag (the leg-2 occlusion requirement, shown on
     *     a 2-window scene): drag the BACK window UP by 10 so it clears F. --- */
    DragWindow(&M.wm, &W[0].rec, 0, -10);           /* B -> {0,10,20,30}           */
    rgn_rect_t Bnew = region_get_bbox(W[0].rec.strucRgn);
    CHECK(rect_eq(Bnew, rect_offset(Bs, 0, -10)),
          "LEG3: the back window moved up by exactly (0,-10)");

    /* AFTER: B and F are disjoint, so visible(B) == FULL B struct == owner-set(B).
     * Re-grade against the INDEPENDENT owner-grid recomputed at the new positions.*/
    CHECK(visible_equals_owner(&M.wm, idx, 2, 0, &VIS.r),
          "LEG3: after the drag clears the occluder, visible(B) == owner-set(B)");

    ComputeVisible(&M.wm, &W[0].rec, &VIS.r);
    int full_b = 1;
    for (int v = Bnew.top; v < Bnew.bottom; v++)
        for (int h = Bnew.left; h < Bnew.right; h++)
            if (!region_contains_point(&VIS.r, (int16_t)h, (int16_t)v)) full_b = 0;
    CHECK(full_b,
          "LEG3: visible(B) is now the FULL un-occluded B rectangle (occlusion gone)");
    /* and the formerly-occluded corner pixel (now at v=10..20 after the -10 move)
     * is visible again: pre-move it was hidden; the live desktop must re-expose it. */
    CHECK(region_contains_point(&VIS.r, 25, 15),
          "LEG3: a formerly-occluded B pixel is visible after the occlusion-changing drag");
}

/* ===========================================================================
 * MAIN -- the FO-9 HOST interaction oracle.
 * ===========================================================================*/
int main(void)
{
    leg1_findwindow();
    leg2_drag_geometry();
    leg3_visible_zorder();
    return TEST_SUMMARY("test-interact");
}
