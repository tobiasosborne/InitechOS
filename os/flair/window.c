/*
 * os/flair/window.c -- the FLAIR Window Manager (THE ARTIFACT).
 *
 * beads: initech-9qf. The z-ordered window list, FindWindow part-code hit-test,
 *        the visible-region overlap computation, and the region-difference DAMAGE
 *        model (ADR-0004 D-5) that drives minimal repaint. See window.h for the
 *        full contract and references.
 *
 * Ref:   PRD Sec 6.2 (region algebra), Sec 6.3 (Toolbox). ADR-0004 D-1 (5-layer
 *        stack), D-3 (Window Manager records + part-codes), D-5 (DiffRgn damage
 *        model: newly-exposed = (old-covered) DIFF (now-covered), accumulated
 *        into updateRgn, NO over-repaint). spec/window_record.h (WindowRecord +
 *        part-codes), spec/region_algebra.h (region_op UNION/DIFF/SECT,
 *        region_set_empty, region_contains_point, region_is_empty,
 *        region_get_bbox), spec/grafport.h (embedded GrafPort + visRgn).
 *        CLAUDE.md Law 2 (oracle is truth), Law 3 (freestanding dual-compile),
 *        Rule 2 (fail loud), Rule 3 (all bugs deep), Rule 11 (deterministic),
 *        Rule 12 (ASCII).
 *
 * STORAGE DISCIPLINE (Law 3): this module mallocs nothing. Every WindowRecord
 * and every region (strucRgn/contRgn/updateRgn, the desktop update, the THREE
 * manager scratch regions) is caller-supplied from the FLAIR arena with its
 * rows[]/x_pool already attached. The ATKINSON engine writes into that storage
 * and FAILS LOUD on cap overflow (Rule 2).
 *
 * THE ALIASING RULE: region_op(out, A, B, op) requires `out` DISTINCT from both
 * A and B (spec/region_algebra.h Section 6b: "default contract: out is
 * distinct"). Every chained computation here uses the three rotating scratch
 * regions so no op ever writes its output over one of its inputs.
 *
 * FAIL-LOUD (Rule 2): WIN_PANIC() halts (kernel) / aborts (hosted) on an
 * invariant violation -- a NULL manager, an unattached region, a window not in
 * the list. A panic with context beats a silently-wrong damage region.
 *
 * DUAL-COMPILE: freestanding (kernel) AND hosted (test_window.c). Only
 * <stdint.h>/<stddef.h> + the locked headers.
 *
 * THE MUTATION SWITCHES (Rule 6): test_window.c compiles this file with
 * -DWINDOW_MUTATE_OVERPAINT and -DWINDOW_MUTATE_ZORDER to prove the oracle
 * bites. The switches are documented at their use sites; the default build
 * defines neither.
 */
#include <stdint.h>
#include <stddef.h>

#include "region_algebra.h"   /* the LOCKED region contract (-Ispec)             */
#include "window.h"           /* the Window Manager API (-Ios/flair)             */

/* ---------------------------------------------------------------------------
 * Fail-loud (dual fail-loud, mirroring the region engine: panic in-kernel /
 * abort hosted). Self-contained (no panic.h dependency) so window.c dual-
 * compiles with only the locked headers.
 * ------------------------------------------------------------------------- */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1 && !defined(__KERNEL_FREESTANDING__)
#  include <stdlib.h>   /* abort -- hosted only */
#  define WIN_PANIC(msg)  abort()
#else
/* Freestanding: a deliberate, loud, deterministic hang (Rule 2 / Rule 11). */
#  define WIN_PANIC(msg)  do { for (;;) { } } while (0)
#endif

/* ===========================================================================
 * Region helpers built on the ATKINSON engine, respecting the aliasing rule.
 * ===========================================================================*/

/* dst := src (the engine's identity-copy idiom: self-union, exactly as region.c
 * documents and test_region.c uses). dst must be distinct from src. */
static void rgn_copy(region_t *dst, const region_t *src)
{
    region_op(dst, src, src, RGN_OP_UNION);   /* self-union == identity copy */
}

/* dst := dst OP other, using a scratch DISTINCT from dst and other:
 *   scratch := dst OP other;  dst := scratch. */
static void rgn_accumulate(region_t *dst, const region_t *other,
                           rgn_op_t op, region_t *scratch)
{
    region_op(scratch, dst, other, op);
    rgn_copy(dst, scratch);
}

/* ===========================================================================
 * fronts_union -- UNION of strucRgn over every VISIBLE window strictly in front
 * of `w`. Writes into `out` (distinct from scratch_b and scratch_c, which it
 * uses internally). The occlusion the visible-region + damage model subtract.
 *
 * MUTANT WINDOW_MUTATE_ZORDER (Rule 6): pretend NOTHING is in front -- the
 * union is always empty, so visible(W) == strucRgn(W) for every window: z-order
 * is ignored and a covered window claims its full structure. The visible-region
 * oracle (independent rasterize-and-compare) goes RED.
 * ===========================================================================*/
static void fronts_union(const WindowMgr *wm, const WindowPtr w, region_t *out)
{
    region_set_empty(out);
#ifndef WINDOW_MUTATE_ZORDER
    /* `out` is the accumulator; rgn_accumulate needs a scratch distinct from
     * `out` and from the operand (f->strucRgn). scratch_c is reserved for this;
     * if a caller passed scratch_c AS `out`, fall back to scratch_b. */
    region_t *sc = (out == wm->scratch_c) ? wm->scratch_b : wm->scratch_c;
    for (WindowPtr f = wm->front; f != NULL && f != w; f = f->nextWindow) {
        if (!f->visible) continue;
        rgn_accumulate(out, f->strucRgn, RGN_OP_UNION, sc);
    }
#else
    (void)wm; (void)w;   /* z-order ignored: union-of-fronts stays empty */
#endif
}

/* ===========================================================================
 * visible_into -- visible(W) = strucRgn(W) DIFF fronts_union(W), optionally
 * clipped to the desktop frame, written into `out`.
 *
 * `out` MUST be DISTINCT from all three manager scratch regions (it is always a
 * window's own region or an independent caller region at every call site -- the
 * exposed-carrier callers pass w->updateRgn, ComputeVisible passes an external
 * region). visible_into then has all three scratch regions free for its chained
 * computation, so no engine op ever aliases its output with an input:
 *   scratch_a := fronts-union;  scratch_b := struc DIFF fronts;
 *   [clip] scratch_c := frame rect;  out := scratch_b SECT scratch_c.
 *
 * When `clip_to_frame` is 0, no frame clip is applied (ComputeVisible for the
 * shell wants the raw visible region): out := struc DIFF fronts.
 * ===========================================================================*/
static void visible_into(const WindowMgr *wm, const WindowPtr w, region_t *out,
                         int clip_to_frame)
{
    region_t *fu = wm->scratch_a;
    fronts_union(wm, w, fu);                                  /* scratch_a (uses scratch_c) */
    if (!clip_to_frame) {
        region_op(out, w->strucRgn, fu, RGN_OP_DIFF);        /* out := struc DIFF fronts */
        return;
    }
    region_t *vis = wm->scratch_b;
    region_op(vis, w->strucRgn, fu, RGN_OP_DIFF);            /* scratch_b := struc DIFF fronts */
    region_t *frame = wm->scratch_c;
    region_set_rect(frame, wm->desktop_frame);               /* scratch_c := frame rect */
    region_op(out, vis, frame, RGN_OP_INTERSECT);            /* out := vis SECT frame */
}

void ComputeVisible(const WindowMgr *wm, const WindowPtr w, region_t *out)
{
    if (wm == NULL || w == NULL || out == NULL) WIN_PANIC("ComputeVisible: NULL");
    if (out->rows == NULL || out->x_pool == NULL) WIN_PANIC("ComputeVisible: out unattached");
    if (!w->visible) { region_set_empty(out); return; }
    visible_into(wm, w, out, 0);
}

/* ===========================================================================
 * List operations.
 * ===========================================================================*/

static int list_contains(const WindowMgr *wm, const WindowPtr w)
{
    for (WindowPtr p = wm->front; p != NULL; p = p->nextWindow)
        if (p == w) return 1;
    return 0;
}

static void list_unlink(WindowMgr *wm, WindowPtr w)
{
    if (wm->front == w) { wm->front = w->nextWindow; w->nextWindow = NULL; return; }
    for (WindowPtr p = wm->front; p != NULL; p = p->nextWindow) {
        if (p->nextWindow == w) { p->nextWindow = w->nextWindow; w->nextWindow = NULL; return; }
    }
    WIN_PANIC("list_unlink: window not in list");
}

static void list_push_front(WindowMgr *wm, WindowPtr w)
{
    w->nextWindow = wm->front;
    wm->front = w;
}

/* The frontmost VISIBLE window is active; every other window inactive. */
static void reaffirm_active(WindowMgr *wm)
{
    int seen_front = 0;
    for (WindowPtr p = wm->front; p != NULL; p = p->nextWindow) {
        if (!seen_front && p->visible) { p->hilited = 1; seen_front = 1; }
        else p->hilited = 0;
    }
}

/* ===========================================================================
 * distribute_exposure -- distribute a freshly-exposed pixel set across the
 * windows behind a departing/moving window and the desktop background.
 *
 * `exposed` (in scratch_b) is the set of pixels that just lost their owner. We
 * walk the remaining windows FRONT-to-BACK; each claims the part of the still-
 * unclaimed `exposed` that falls within its structure (the front-most remaining
 * owner repaints a contested pixel), then that part is removed from `exposed`.
 * Whatever remains damages the bare desktop.
 *
 * `exposed` lives in scratch_b. We use scratch_c for the per-window claim and
 * scratch_a for the in-place shrink, so no op aliases. `departing` is skipped
 * (it is being removed/moved and is handled by the caller).
 *
 * RESULT INVARIANT (the oracle): union(all updateRgns touched) UNION
 * (desktop_update added) == `exposed` on entry, EXACTLY, with no pixel counted
 * twice -- no over-repaint (D-5 / Rationale 4.3).
 *
 * MUTANT WINDOW_MUTATE_OVERPAINT (Rule 6): the bandaid the damage model exists
 * to forbid -- repaint the WHOLE window, not just the diff. Each window behind
 * the departing one gets its ENTIRE structure marked dirty (clipped to the
 * frame), regardless of whether those pixels were actually exposed. This is the
 * "slow + wrong-looking flicker" behaviour D-5 / Rationale 4.3 rejects. The
 * damage oracle goes RED on the headline checks: pixels that did NOT change
 * owner get marked dirty (soundness / no-over-repaint fails), windows damage
 * each other's overlapping pixels (double-count fails), and a window's damage
 * extends across its whole structure rather than the exposed sub-area.
 * ===========================================================================*/
static void distribute_exposure(WindowMgr *wm, const WindowPtr departing,
                                region_t *exposed)
{
    for (WindowPtr p = wm->front; p != NULL; p = p->nextWindow) {
        if (p == departing) continue;
        if (!p->visible) continue;

#ifndef WINDOW_MUTATE_OVERPAINT
        if (region_is_empty(exposed)) break;
        /* claim := exposed SECT p->strucRgn  (scratch_c). */
        region_t *claim = wm->scratch_c;
        region_op(claim, exposed, p->strucRgn, RGN_OP_INTERSECT);
        if (region_is_empty(claim)) continue;
        /* p->updateRgn += claim  (scratch_a as the accumulate temp; distinct
         * from updateRgn and claim). */
        rgn_accumulate(p->updateRgn, claim, RGN_OP_UNION, wm->scratch_a);
        /* exposed -= p->strucRgn  (a window behind cannot re-claim these). The
         * out (scratch_a) is distinct from exposed and the operand. */
        region_op(wm->scratch_a, exposed, p->strucRgn, RGN_OP_DIFF);
        rgn_copy(exposed, wm->scratch_a);
#else
        /* OVERPAINT mutant: mark p's WHOLE structure dirty, not just the diff. */
        region_t *frame = wm->scratch_c;
        region_set_rect(frame, wm->desktop_frame);
        region_t *whole = wm->scratch_b;
        region_op(whole, p->strucRgn, frame, RGN_OP_INTERSECT);   /* struc ^ frame */
        rgn_accumulate(p->updateRgn, whole, RGN_OP_UNION, wm->scratch_a);
#endif
    }

    /* Whatever exposure remains damages the bare desktop. (Under the mutant,
     * `exposed` was never shrunk, so the desktop also gets the whole set --
     * compounding the over-count.) */
    if (!region_is_empty(exposed))
        rgn_accumulate(wm->desktop_update, exposed, RGN_OP_UNION, wm->scratch_a);
}

/* ===========================================================================
 * Lifecycle.
 * ===========================================================================*/

void WindowMgr_init(WindowMgr *wm, rgn_rect_t desktop_frame,
                    region_t *desktop_update,
                    region_t *scratch_a, region_t *scratch_b,
                    region_t *scratch_c)
{
    if (wm == NULL || desktop_update == NULL ||
        scratch_a == NULL || scratch_b == NULL || scratch_c == NULL)
        WIN_PANIC("WindowMgr_init: NULL");
    if (desktop_update->rows == NULL || scratch_a->rows == NULL ||
        scratch_b->rows == NULL || scratch_c->rows == NULL)
        WIN_PANIC("WindowMgr_init: region unattached");

    wm->front          = NULL;
    wm->desktop_frame  = desktop_frame;
    wm->desktop_update = desktop_update;
    wm->scratch_a      = scratch_a;
    wm->scratch_b      = scratch_b;
    wm->scratch_c      = scratch_c;
    region_set_empty(desktop_update);
    region_set_empty(scratch_a);
    region_set_empty(scratch_b);
    region_set_empty(scratch_c);
}

void NewWindow(WindowMgr *wm, WindowPtr w, rgn_rect_t bounds, rgn_rect_t content,
               int16_t wKind, int16_t wVariant, uint8_t goAway)
{
    if (wm == NULL || w == NULL) WIN_PANIC("NewWindow: NULL");
    if (w->strucRgn == NULL || w->contRgn == NULL || w->updateRgn == NULL)
        WIN_PANIC("NewWindow: window regions unattached");
    if (w->strucRgn->rows == NULL || w->contRgn->rows == NULL ||
        w->updateRgn->rows == NULL)
        WIN_PANIC("NewWindow: window region pools unattached");

    region_set_rect(w->strucRgn, bounds);
    region_set_rect(w->contRgn, content);
    region_set_empty(w->updateRgn);

    w->windowKind           = wKind;
    w->windowDefProcVariant = wVariant;
    w->goAwayFlag           = goAway;
    w->visible              = 1;
    w->spareFlag            = 0;
    w->titleHandle[0]       = '\0';   /* empty title until SetWTitle/caller sets it
                                       * (NewWindow leaves "" per IM-I; required now
                                       * that the chrome drawer renders the title) */

    list_push_front(wm, w);
    reaffirm_active(wm);
}

void HideWindow(WindowMgr *wm, WindowPtr w)
{
    if (wm == NULL || w == NULL) WIN_PANIC("HideWindow: NULL");
    if (!list_contains(wm, w)) WIN_PANIC("HideWindow: not in list");
    if (!w->visible) return;

    /* exposed := visible(w) clipped to frame, BEFORE flipping visibility. We use
     * w's OWN updateRgn as the exposed carrier (it owes no repaint once hidden,
     * and is reset to empty below), which frees all three scratch regions for
     * visible_into's chained algebra (no aliasing). */
    region_t *exposed = w->updateRgn;
    visible_into(wm, w, exposed, 1);

    w->visible = 0;

    distribute_exposure(wm, w, exposed);
    region_set_empty(w->updateRgn);    /* a hidden window owes no repaint */
    reaffirm_active(wm);
}

void DisposeWindow(WindowMgr *wm, WindowPtr w)
{
    if (wm == NULL || w == NULL) WIN_PANIC("DisposeWindow: NULL");
    if (!list_contains(wm, w)) WIN_PANIC("DisposeWindow: not in list");

    if (w->visible) {
        /* exposed := visible(w) clipped to frame, BEFORE unlinking (fronts need
         * w in the list). Carry it in w's OWN updateRgn so all scratch is free. */
        region_t *exposed = w->updateRgn;
        visible_into(wm, w, exposed, 1);
        /* Unlink so distribute_exposure does not count w as a claimant of its
         * own exposure; w->updateRgn (== exposed) stays valid storage. */
        list_unlink(wm, w);
        distribute_exposure(wm, w, exposed);
    } else {
        list_unlink(wm, w);
    }
    region_set_empty(w->updateRgn);
    reaffirm_active(wm);
}

void ShowWindow(WindowMgr *wm, WindowPtr w)
{
    if (wm == NULL || w == NULL) WIN_PANIC("ShowWindow: NULL");
    if (!list_contains(wm, w)) WIN_PANIC("ShowWindow: not in list");
    if (w->visible) return;
    w->visible = 1;
    /* The newly-shown window owes a full repaint of its visible region; the
     * caller seeds updateRgn via WindowMgr_invalidate. Covering windows behind
     * it are unaffected (showing covers, it does not expose). */
    reaffirm_active(wm);
}

void SelectWindow(WindowMgr *wm, WindowPtr w)
{
    if (wm == NULL || w == NULL) WIN_PANIC("SelectWindow: NULL");
    if (!list_contains(wm, w)) WIN_PANIC("SelectWindow: not in list");
    if (wm->front != w) {
        list_unlink(wm, w);
        list_push_front(wm, w);
        /* Raising a window NEVER exposes another window (it only covers more),
         * so no exposure damage is generated. The raised window may itself need
         * to repaint the part of it that was previously covered; the caller
         * seeds that via WindowMgr_invalidate(w, ...) if desired. D-5 exposure
         * is for move/hide/dispose, not raise. */
    }
    reaffirm_active(wm);
}

/* ===========================================================================
 * MoveWindow / DragWindow -- the damage payoff (ADR-0004 D-5).
 * ===========================================================================*/

void MoveWindow(WindowMgr *wm, WindowPtr w, int16_t newLeft, int16_t newTop)
{
    if (wm == NULL || w == NULL) WIN_PANIC("MoveWindow: NULL");
    if (!list_contains(wm, w)) WIN_PANIC("MoveWindow: not in list");

    rgn_rect_t s = region_get_bbox(w->strucRgn);
    rgn_rect_t c = region_get_bbox(w->contRgn);
    int16_t dh = (int16_t)(newLeft - s.left);
    int16_t dv = (int16_t)(newTop  - s.top);
    if (dh == 0 && dv == 0) return;

    /* exposed := visible(w_old) clipped to frame. A window only exposes pixels
     * it actually OWNED (its visible region), never pixels a front window owned.
     * Carry it in w's OWN updateRgn so all three scratch are free for the
     * chained algebra (visible_into + the new-struc subtraction). */
    region_t *exposed = w->updateRgn;
    visible_into(wm, w, exposed, 1);

    /* The pixels w_old owned that w_new STILL covers did not change owner, so
     * they are not damage:  exposed := exposed DIFF region(w_new_struc). */
    rgn_rect_t ns = { (int16_t)(s.top + dv), (int16_t)(s.left + dh),
                      (int16_t)(s.bottom + dv), (int16_t)(s.right + dh) };
    rgn_rect_t nc = { (int16_t)(c.top + dv), (int16_t)(c.left + dh),
                      (int16_t)(c.bottom + dv), (int16_t)(c.right + dh) };
    {
        region_t *nsr = wm->scratch_a;
        region_set_rect(nsr, ns);
        region_op(wm->scratch_b, exposed, nsr, RGN_OP_DIFF);  /* out distinct */
        rgn_copy(exposed, wm->scratch_b);                     /* exposed shrinks */
    }

    /* Commit the move. NOTE: strucRgn/contRgn are recomputed AFTER `exposed`
     * (== w->updateRgn) is fully built; the move does not touch updateRgn until
     * distribute_exposure consumes `exposed`, after which updateRgn is cleared.
     * The moved window's OWN repaint is the caller's concern; D-5 / the oracle
     * is about the OTHER windows + the desktop. */
    region_set_rect(w->strucRgn, ns);
    region_set_rect(w->contRgn, nc);

    distribute_exposure(wm, w, exposed);
    region_set_empty(w->updateRgn);
}

void DragWindow(WindowMgr *wm, WindowPtr w, int16_t dh, int16_t dv)
{
    if (wm == NULL || w == NULL) WIN_PANIC("DragWindow: NULL");
    if (!list_contains(wm, w)) WIN_PANIC("DragWindow: not in list");
    rgn_rect_t s = region_get_bbox(w->strucRgn);
    MoveWindow(wm, w, (int16_t)(s.left + dh), (int16_t)(s.top + dv));
}

/* ===========================================================================
 * INVALIDATE / VALIDATE.
 * ===========================================================================*/

void WindowMgr_invalidate(WindowMgr *wm, WindowPtr w, rgn_rect_t rect)
{
    if (wm == NULL || w == NULL) WIN_PANIC("invalidate: NULL");
    if (!list_contains(wm, w)) WIN_PANIC("invalidate: not in list");
    if (!w->visible) return;

    /* vis := visible(w) (raw, not frame-clipped) into scratch_b. */
    region_t *vis = wm->scratch_b;
    visible_into(wm, w, vis, 0);

    /* product := region(rect) SECT vis  (scratch_c). region(rect) is in
     * scratch_a; vis is scratch_b; product is scratch_c -- all distinct. */
    region_t *rr = wm->scratch_a;
    region_set_rect(rr, rect);
    region_t *product = wm->scratch_c;
    region_op(product, rr, vis, RGN_OP_INTERSECT);

    /* updateRgn += product, with scratch_a as the accumulate temp (distinct
     * from updateRgn and product). */
    rgn_accumulate(w->updateRgn, product, RGN_OP_UNION, wm->scratch_a);
}

void WindowMgr_validate(WindowPtr w)
{
    if (w == NULL) WIN_PANIC("validate: NULL");
    region_set_empty(w->updateRgn);
}

/* ===========================================================================
 * FindWindow -- z-order hit-test + chrome part-code (verbatim IM part-codes).
 * Walks the z-order FRONT-to-BACK; the first VISIBLE window whose strucRgn
 * contains the point wins (it is the front-most window there); the part-code is
 * resolved by which chrome sub-band the point falls in (window.h Section 4).
 * ===========================================================================*/
flair_part_code_t FindWindow(const WindowMgr *wm, flair_point_t pt,
                             WindowPtr *whichWindow)
{
    if (wm == NULL) WIN_PANIC("FindWindow: NULL");
    int16_t h = pt.h, v = pt.v;

    for (WindowPtr p = wm->front; p != NULL; p = p->nextWindow) {
        if (!p->visible) continue;
        if (!region_contains_point(p->strucRgn, h, v)) continue;

        if (whichWindow) *whichWindow = p;

        /* content first (most common). */
        if (region_contains_point(p->contRgn, h, v))
            return inContent;

        /* chrome: derive the box bands from the struc/content bounding rects. */
        rgn_rect_t s = region_get_bbox(p->strucRgn);
        rgn_rect_t c = region_get_bbox(p->contRgn);
        int16_t tb = (int16_t)(c.top - s.top);     /* title-bar band height */
        if (tb < 1) tb = 1;

        /* go-away (close) box: a tb-square at the top-left of the title bar. */
        if (p->goAwayFlag) {
            if (h >= s.left && h < (int16_t)(s.left + tb) &&
                v >= s.top  && v < c.top)
                return inGoAway;
        }
        /* zoom box: a tb-square at the top-right of the title bar (zoom variants).*/
        if (p->windowDefProcVariant == zoomDocProc ||
            p->windowDefProcVariant == zoomNoGrow) {
            if (h >= (int16_t)(s.right - tb) && h < s.right &&
                v >= s.top && v < c.top)
                return inZoomIn;
        }
        /* grow box: a square at the bottom-right corner (grow variants). */
        if (p->windowDefProcVariant == documentProc ||
            p->windowDefProcVariant == zoomDocProc) {
            int16_t gb = (int16_t)(s.bottom - c.bottom);   /* bottom chrome band */
            if (gb < 1) gb = 1;
            if (h >= (int16_t)(s.right - gb) && h < s.right &&
                v >= (int16_t)(s.bottom - gb) && v < s.bottom)
                return inGrow;
        }
        /* any other chrome pixel (title bar, frame) is the drag region. */
        return inDrag;
    }

    if (whichWindow) *whichWindow = NULL;
    return inDesk;
}
