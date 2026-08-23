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
 * -DWINDOW_MUTATE_OVERPAINT/-DWINDOW_MUTATE_ZORDER and the activation plus
 * CHROME_FID_MUT_* CalcDoc restorations to prove the oracle bites. The switches
 * are documented at their use sites; the default build defines none of them.
 */
#include <stdint.h>
#include <stddef.h>

#include "region_algebra.h"   /* the LOCKED region contract (-Ispec)             */
#include "window.h"           /* the Window Manager API (-Ios/flair)             */
#include "chrome_metrics.h"    /* SetWTitle title-band invalidation height         */

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

/* The Platinum document structure is the frame UNION its exact notched shadow
 * L. The shadow is part of strucRgn so z-order, exposure, and move damage own
 * the same pixels the WDEF draws. Ref: sys8/window-chrome.md Sec 1 and the
 * CalcDoc finding in docs/FLAIR-solidity-drive-2026-07-21.md Sec 4. */
static int document_variant(int16_t variant)
{
    return variant == documentProc || variant == noGrowDocProc ||
           variant == zoomDocProc || variant == zoomNoGrow;
}

/* One capability datum feeds both the WDEF drawer and FindWindow. Platinum
 * keeps collapse in its fixed R-16 slot even when zoom is absent (the sampled
 * control-panel case). NewDocumentWindow selects zoomDocProc for the full
 * document set; direct NewWindow callers retain the verbatim variant split.
 * Ref: sys8/window-chrome.md Sec 3.1; D3.a row 16; beads tbef/tdnl.2. */
static uint8_t widget_flags_for_variant(int16_t variant, uint8_t go_away)
{
    uint8_t flags = 0u;
    if (go_away && document_variant(variant))
        flags |= FLAIR_WINDOW_WIDGET_CLOSE;
    if (variant == zoomDocProc || variant == zoomNoGrow)
        flags |= FLAIR_WINDOW_WIDGET_ZOOM;
    if (document_variant(variant))
        flags |= FLAIR_WINDOW_WIDGET_COLLAPSE;
    if (variant == documentProc || variant == zoomDocProc)
        flags |= FLAIR_WINDOW_WIDGET_GROW;
    return flags;
}

static region_t *structure_scratch(WindowMgr *wm, rgn_rect_t frame,
                                   int16_t variant)
{
    region_t *a = wm->scratch_a;
    region_set_rect(a, frame);
#if !defined(CHROME_FID_MUT_SHADOW_CLIPPED)
    if (document_variant(variant)) {
        region_t *b = wm->scratch_b;
        region_t *c = wm->scratch_c;
        rgn_rect_t shadow;

        if (frame.right == INT16_MAX || frame.bottom == INT16_MAX) {
            WIN_PANIC("document shadow coordinate overflow");
        }
        shadow.top = (int16_t)(frame.top + FLAIR_CHROME_SHADOW_NOTCH);
        shadow.left = frame.right;
        shadow.bottom = (int16_t)(frame.bottom + FLAIR_CHROME_SHADOW_OFFSET);
        shadow.right = (int16_t)(frame.right + FLAIR_CHROME_SHADOW_OFFSET);
        region_set_rect(b, shadow);
        region_op(c, a, b, RGN_OP_UNION);

        shadow.top = frame.bottom;
        shadow.left = (int16_t)(frame.left + FLAIR_CHROME_SHADOW_NOTCH);
        shadow.bottom = (int16_t)(frame.bottom + FLAIR_CHROME_SHADOW_OFFSET);
        shadow.right = (int16_t)(frame.right + FLAIR_CHROME_SHADOW_OFFSET);
        region_set_rect(b, shadow);
        region_op(a, c, b, RGN_OP_UNION);
    }
#else
    /* Rule-6 mutant (initech-9d0e): restore frame-only strucRgn clipping. */
    (void)document_variant(variant);
#endif
    return a;
}

rgn_rect_t WindowFrameRect(const WindowPtr w)
{
    rgn_rect_t frame;
    if (w == NULL || w->strucRgn == NULL) WIN_PANIC("WindowFrameRect: NULL");
    frame = region_get_bbox(w->strucRgn);
#if !defined(CHROME_FID_MUT_SHADOW_CLIPPED)
    if (document_variant(w->windowDefProcVariant) && !region_is_empty(w->strucRgn)) {
        frame.right = (int16_t)(frame.right - FLAIR_CHROME_SHADOW_OFFSET);
        frame.bottom = (int16_t)(frame.bottom - FLAIR_CHROME_SHADOW_OFFSET);
    }
#endif
    return frame;
}

rgn_rect_t CalcDocContentRect(rgn_rect_t frame)
{
    rgn_rect_t content;
#if defined(CHROME_FID_MUT_CONTENT_TOP_STALE)
    content.top = (int16_t)(frame.top + FLAIR_CHROME_FRAME +
                            FLAIR_CHROME_TITLEBAR_H);
#else
    content.top = (int16_t)(frame.top + FLAIR_CHROME_TITLEBAR_H);
#endif
    content.left = (int16_t)(frame.left + FLAIR_CHROME_FRAME);
    content.bottom = (int16_t)(frame.bottom - FLAIR_CHROME_FRAME);
#if defined(CHROME_FID_MUT_CONTENT_OVER_SCROLL)
    content.right = (int16_t)(frame.right - FLAIR_CHROME_FRAME);
#else
    content.right = (int16_t)(frame.right - FLAIR_CHROME_FRAME -
                              FLAIR_CHROME_BODY_BAR -
                              FLAIR_CHROME_SCROLLBAR_W);
#endif
    return content;
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
 *
 * THE ALWAYS-ON-TOP OVERLAY (beads initech-pipa; ADR-0005 region spine): after the
 * z-order union, fold in wm->overlay_rgn (the shell's menu bars + modal, a layer
 * ABOVE every window but OUTSIDE the WindowMgr list). Because fronts_union feeds
 * the WHOLE damage chain (visible_into -> exposed -> distribute_exposure ->
 * desktop_update), treating the overlay as occluding closes BOTH compositor
 * defects at once: no window's visible region (and thus no repaint) includes the
 * overlay -- it is never overpainted (defect b) -- AND the vacated area of a drag
 * is never classified as exposed over the overlay, so desktop_update never
 * seafoam-fills it (defect a). The overlay pixels are simply never written by
 * desktop_paint_damage and survive from the initial shell_render.
 *
 * MUTANT WINDOW_MUTATE_IGNORE_OVERLAY (Rule 6): compile the fold out so a SET
 * overlay_rgn is IGNORED -- the pre-fix compositor (windows overpaint / desktop
 * seafoam-erases the bars + modal). The EMU survival oracle (initech-dc4v) RED.
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

#ifndef WINDOW_MUTATE_IGNORE_OVERLAY
    /* ALIASING-CRITICAL (Rule 3): rgn_accumulate(out, overlay, UNION, sc2) does
     * `sc2 := out UNION overlay; out := sc2`, so sc2 MUST be distinct from BOTH
     * `out` and overlay_rgn -- the naive region_op(out, out, overlay) self-alias
     * corrupts on OVERLAP (the exact drag-across-overlay case under test). `out`
     * is scratch_a at every call site (visible_into passes fu==scratch_a), so sc2
     * resolves to scratch_c (distinct); the fallback covers a scratch_c `out`.
     * overlay_rgn is a separate arena region, never a manager scratch. */
    if (wm->overlay_rgn != NULL && !region_is_empty(wm->overlay_rgn)) {
        region_t *sc2 = (out == wm->scratch_c) ? wm->scratch_b : wm->scratch_c;
        rgn_accumulate(out, wm->overlay_rgn, RGN_OP_UNION, sc2);
    }
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

int WindowMgr_window_index(const WindowMgr *wm, const WindowPtr w)
{
    int index = 0;
    if (wm == NULL || w == NULL) return -1;
    for (WindowPtr p = wm->front; p != NULL; p = p->nextWindow, index++) {
        if (p == w) return index;
    }
    return -1;
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

/* The frontmost VISIBLE window is active; every other window inactive.
 *
 * Ref: bead initech-v6t2; ADR-0004 D-5. BUG (fixed here): this loop used to
 * only flip the in-memory `hilited` flag -- a window whose hilited went
 * 1->0 (it WAS the active window and just lost that status) never had any
 * repaint seeded, so its active-style chrome stayed on screen until an
 * unrelated expose happened to cover/re-expose it. FIX: on a 1->0
 * transition, seed that window's repaint by adding its current visible
 * region to its updateRgn, reusing the existing WindowMgr_invalidate
 * primitive (the same accumulate-into-updateRgn idiom HideWindow/
 * DisposeWindow use via visible_into, window.c ~314/332). `rect` is passed
 * as region_get_bbox(p->strucRgn): a bounding rect is always a superset of
 * strucRgn, which is always a superset of visible(p) (visible_into DIFFs
 * strucRgn by the fronts-union), so WindowMgr_invalidate's internal SECT
 * with visible(p) yields exactly visible(p) -- no over-invalidate. If p is
 * hidden (visible == 0, e.g. reaffirm_active called from HideWindow after
 * w->visible was cleared), WindowMgr_invalidate is a documented no-op
 * ("if (!w->visible) return;"), which is correct: a hidden window owes no
 * repaint (HideWindow/DisposeWindow already clear its updateRgn).
 * WindowMgr_invalidate itself calls list_contains(wm, p); `p` is always
 * still linked here because we are iterating wm->front's own list, so that
 * check always succeeds (a DisposeWindow'd window is unlinked BEFORE this
 * loop runs, per window.c:336, so it is never visited here at all).
 *
 * BOTH transitions are seeded (bead initech-rqz5; epic initech-av7s DQ3,
 * ratified 2026-07-31). The 1->0 (deactivation) half is the original
 * initech-v6t2 fix. The 0->1 (activation) half was originally left to the
 * caller ("the caller seeds updateRgn via WindowMgr_invalidate") -- and the
 * 2026-07-21 drive battery proved every live caller forgot: newly-active
 * windows kept flat inactive title bars (s02/s08), exposed strips painted
 * HALF-active bars (s05), and raise generated no self-exposure repaint at
 * all (SelectWindow's old "the caller seeds that if desired"). Seeding
 * 0->1 here makes activation chrome a WINDOW-MANAGER invariant, not a
 * per-call-site convention: the seed is the strucRgn bbox, SECTed down to
 * visible(p) by WindowMgr_invalidate exactly as the 1->0 half, and it
 * subsumes the raise-exposure seed on the click-to-front path (raise
 * implies activation there). Callers that ALSO invalidate (a moved window's
 * self-repaint) stay correct -- updateRgn accumulation is a union.
 *
 * MUTANT WINDOW_MUTATE_NO_ACTIVATE_INVAL (Rule 6; the initech-rqz5 bug
 * restored): suppresses ONLY the 0->1 seed -- newly-active windows keep
 * whatever chrome/content pixels they had. Bites the host activation
 * checks in test_window.c and the emu test-flair-solid leg C. NEVER in a
 * real build. */
static void reaffirm_active(WindowMgr *wm)
{
    int seen_front = 0;
    for (WindowPtr p = wm->front; p != NULL; p = p->nextWindow) {
        uint8_t was_hilited = p->hilited;
        if (!seen_front && p->visible) { p->hilited = 1; seen_front = 1; }
        else p->hilited = 0;
        if (was_hilited && !p->hilited) {
#ifndef WINDOW_MUTATE_NO_DEACT_INVAL
            WindowMgr_invalidate(wm, p, region_get_bbox(p->strucRgn));
#endif
        } else if (!was_hilited && p->hilited) {
#ifndef WINDOW_MUTATE_NO_ACTIVATE_INVAL
            WindowMgr_invalidate(wm, p, region_get_bbox(p->strucRgn));
#endif
        }
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
    wm->overlay_rgn    = NULL;   /* no always-on-top overlay until the caller sets
                                  * it (initech-pipa). MUST be zeroed: fronts_union
                                  * dereferences it only when non-NULL -- an
                                  * uninitialized pointer is UB (Rule 2). */
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

    rgn_copy(w->strucRgn, structure_scratch(wm, bounds, wVariant));
    region_set_rect(w->contRgn, content);
    region_set_empty(w->updateRgn);

    w->windowKind           = wKind;
    w->windowDefProcVariant = wVariant;
    w->goAwayFlag           = goAway;
    w->visible              = 1;
    w->spareFlag            = 0;
    w->widgetFlags          = widget_flags_for_variant(wVariant, goAway);
    w->zoomed               = 0;
    w->collapsed            = 0;
    w->userStateValid       = 0;
    w->userState            = bounds;
    w->collapseState        = bounds;
    w->titleHandle[0]       = '\0';   /* empty title until SetWTitle/caller sets it
                                       * (NewWindow leaves "" per IM-I; required now
                                       * that the chrome drawer renders the title) */
    w->titleWidth           = 0;

    list_push_front(wm, w);
    reaffirm_active(wm);
}

void NewDocumentWindow(WindowMgr *wm, WindowPtr w, rgn_rect_t frame,
                       int16_t wKind, uint8_t goAway)
{
    NewWindow(wm, w, frame, CalcDocContentRect(frame),
              wKind, (int16_t)zoomDocProc, goAway);
}

void SetWTitle(WindowMgr *wm, WindowPtr w, const char *title)
{
    int n = 0;
    rgn_rect_t title_band;

    if (w == NULL) WIN_PANIC("SetWTitle: NULL window");
    if (wm != NULL && !list_contains(wm, w)) {
        WIN_PANIC("SetWTitle: window not in manager list");
    }
    if (title == NULL) title = "";
    while (title[n] != '\0' && n < (int)FLAIR_WINDOW_TITLE_MAX - 1) {
        w->titleHandle[n] = title[n];
        n++;
    }
    w->titleHandle[n] = '\0';
    w->titleWidth = (int16_t)(n * FLAIR_CHROME_TITLE_CELL_W);

    if (wm == NULL || !w->visible) return;
    title_band = WindowFrameRect(w);
    title_band.bottom = (int16_t)(title_band.top + FLAIR_CHROME_TITLEBAR_H);
    if (title_band.bottom > WindowFrameRect(w).bottom) {
        title_band.bottom = WindowFrameRect(w).bottom;
    }
    WindowMgr_invalidate(wm, w, title_band);
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
    /* The newly-shown window owes a full repaint of its visible region. If the
     * show ACTIVATES it (it is the front-most visible window), reaffirm_active's
     * 0->1 branch seeds that repaint (initech-rqz5 / DQ3). If it re-appears as a
     * BACKGROUND window (an active window still sits in front of it), no hilited
     * transition fires and the caller still seeds updateRgn via
     * WindowMgr_invalidate. Covering windows behind it are unaffected (showing
     * covers, it does not expose). */
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
         * so no D-5 exposure damage is generated for the windows behind it.
         * The raised window's OWN repaint (the previously-covered part + the
         * active-chrome flip) is seeded by reaffirm_active's 0->1 branch below
         * (bead initech-rqz5 / DQ3): raising a non-front window always flips
         * its hilited 0->1, and the seed is its full post-raise visible
         * region. D-5 exposure remains move/hide/dispose-only. */
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

    rgn_rect_t s = WindowFrameRect(w);
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
        region_t *nsr = structure_scratch(wm, ns, w->windowDefProcVariant);
        region_op(wm->scratch_b, exposed, nsr, RGN_OP_DIFF);  /* out distinct */
        rgn_copy(exposed, wm->scratch_b);                     /* exposed shrinks */
    }

    /* Commit the move. NOTE: strucRgn/contRgn are recomputed AFTER `exposed`
     * (== w->updateRgn) is fully built; the move does not touch updateRgn until
     * distribute_exposure consumes `exposed`, after which updateRgn is cleared.
     * The moved window's OWN repaint is the caller's concern; D-5 / the oracle
     * is about the OTHER windows + the desktop. */
    rgn_copy(w->strucRgn,
             structure_scratch(wm, ns, w->windowDefProcVariant));
    region_set_rect(w->contRgn, nc);

    distribute_exposure(wm, w, exposed);
    region_set_empty(w->updateRgn);
}

void DragWindow(WindowMgr *wm, WindowPtr w, int16_t dh, int16_t dv)
{
    if (wm == NULL || w == NULL) WIN_PANIC("DragWindow: NULL");
    if (!list_contains(wm, w)) WIN_PANIC("DragWindow: not in list");
    rgn_rect_t s = WindowFrameRect(w);
    MoveWindow(wm, w, (int16_t)(s.left + dh), (int16_t)(s.top + dv));
}

#if defined(WINDOW_ENABLE_R1_OPS)
/* Transactional document-frame change. The old visible footprint is captured
 * before any region changes, reduced by the new structure, then partitioned
 * through the existing D-5 exposure spine. Only after that partition is
 * consumed do we seed the actor's exact new visible footprint. This is the
 * SizeWindow-class counterpart to MoveWindow, with no intermediate geometry
 * (and therefore no over-damage when position and size both change for zoom).
 * Ref: window.h Sec 3; WL-0076; GUI-remediation-plan.md R1.2. */
static void set_document_frame(WindowMgr *wm, WindowPtr w,
                               rgn_rect_t next, int collapsed)
{
    region_t *exposed;
    region_t *next_structure;

    if (wm == NULL || w == NULL) WIN_PANIC("set_document_frame: NULL");
    if (!list_contains(wm, w)) WIN_PANIC("set_document_frame: not in list");
    if (!document_variant(w->windowDefProcVariant))
        WIN_PANIC("set_document_frame: non-document variant");
    if (next.right <= next.left || next.bottom <= next.top)
        WIN_PANIC("set_document_frame: empty frame");

    exposed = w->updateRgn;
    visible_into(wm, w, exposed, 1);
    next_structure = structure_scratch(wm, next, w->windowDefProcVariant);
    region_op(wm->scratch_b, exposed, next_structure, RGN_OP_DIFF);
    rgn_copy(exposed, wm->scratch_b);

    rgn_copy(w->strucRgn,
             structure_scratch(wm, next, w->windowDefProcVariant));
    if (collapsed) {
#ifndef WINDOW_MUTATE_COLLAPSE_LEAK
        region_set_empty(w->contRgn);
#else
        /* Rule-6 mutant: body ownership leaks through the shade state. */
#endif
    } else {
        region_set_rect(w->contRgn, CalcDocContentRect(next));
    }

    distribute_exposure(wm, w, exposed);
    region_set_empty(w->updateRgn);
    WindowMgr_invalidate(wm, w, region_get_bbox(w->strucRgn));
}

void ConstrainWindowSize(const WindowMgr *wm, const WindowPtr w,
                         int16_t *width, int16_t *height)
{
    rgn_rect_t frame;
    int32_t desktop_w;
    int32_t desktop_h;
    int32_t max_w;
    int32_t max_h;
    int32_t min_w;
    int32_t min_h;
    int32_t want_w;
    int32_t want_h;

    if (wm == NULL || w == NULL || width == NULL || height == NULL)
        WIN_PANIC("ConstrainWindowSize: NULL");
    frame = WindowFrameRect(w);
    desktop_w = (int32_t)wm->desktop_frame.right - wm->desktop_frame.left;
    desktop_h = (int32_t)wm->desktop_frame.bottom - wm->desktop_frame.top;
    max_w = (int32_t)wm->desktop_frame.right - frame.left;
    max_h = (int32_t)wm->desktop_frame.bottom - frame.top;
    if (max_w > desktop_w) max_w = desktop_w;
    if (max_h > desktop_h) max_h = desktop_h;
    if (max_w > FLAIR_CHROME_WINDOW_MAX_W) max_w = FLAIR_CHROME_WINDOW_MAX_W;
    if (max_h > FLAIR_CHROME_WINDOW_MAX_H) max_h = FLAIR_CHROME_WINDOW_MAX_H;
    if (max_w < 1 || max_h < 1) WIN_PANIC("ConstrainWindowSize: origin off desktop");

    min_w = FLAIR_CHROME_WINDOW_MIN_W;
    min_h = FLAIR_CHROME_WINDOW_MIN_H;
    if (min_w > max_w) min_w = max_w;
    if (min_h > max_h) min_h = max_h;
    want_w = *width;
    want_h = *height;
#ifndef WINDOW_MUTATE_GROW_NO_MIN
    if (want_w < min_w) want_w = min_w;
    if (want_h < min_h) want_h = min_h;
#else
    /* Rule-6 mutant: restore the unusable sub-chrome grow result. */
#endif
    if (want_w > max_w) want_w = max_w;
    if (want_h > max_h) want_h = max_h;
    *width = (int16_t)want_w;
    *height = (int16_t)want_h;
}

void SizeWindow(WindowMgr *wm, WindowPtr w, int16_t width, int16_t height)
{
    rgn_rect_t frame;
    if (wm == NULL || w == NULL) WIN_PANIC("SizeWindow: NULL");
    if (w->collapsed) WIN_PANIC("SizeWindow: collapsed window");
    ConstrainWindowSize(wm, w, &width, &height);
    frame = WindowFrameRect(w);
    frame.right = (int16_t)(frame.left + width);
    frame.bottom = (int16_t)(frame.top + height);
    set_document_frame(wm, w, frame, 0);
    if (w->zoomed) {
        w->zoomed = 0;
        w->userStateValid = 0;
    }
}

int ZoomWindow(WindowMgr *wm, WindowPtr w)
{
    rgn_rect_t target;
    if (wm == NULL || w == NULL) WIN_PANIC("ZoomWindow: NULL");
    if (w->collapsed) {
        target = w->collapseState;
        w->collapsed = 0;
        set_document_frame(wm, w, target, 0);
    }

    if (!w->zoomed) {
        w->userState = WindowFrameRect(w);
        w->userStateValid = 1;
        target.top = (int16_t)(wm->desktop_frame.top +
                               FLAIR_CHROME_ZOOM_MARGIN_TOP);
        target.left = (int16_t)(wm->desktop_frame.left +
                                FLAIR_CHROME_ZOOM_MARGIN_LEFT);
        target.bottom = (int16_t)(wm->desktop_frame.bottom -
                                  FLAIR_CHROME_ZOOM_MARGIN_BOTTOM);
        target.right = (int16_t)(wm->desktop_frame.right -
                                 FLAIR_CHROME_ZOOM_MARGIN_RIGHT);
        w->zoomed = 1;
    } else {
        if (!w->userStateValid) WIN_PANIC("ZoomWindow: missing userState");
#ifndef WINDOW_MUTATE_ZOOM_NO_RESTORE
        target = w->userState;
#else
        target.top = (int16_t)(wm->desktop_frame.top +
                               FLAIR_CHROME_ZOOM_MARGIN_TOP);
        target.left = (int16_t)(wm->desktop_frame.left +
                                FLAIR_CHROME_ZOOM_MARGIN_LEFT);
        target.bottom = (int16_t)(wm->desktop_frame.bottom -
                                  FLAIR_CHROME_ZOOM_MARGIN_BOTTOM);
        target.right = (int16_t)(wm->desktop_frame.right -
                                 FLAIR_CHROME_ZOOM_MARGIN_RIGHT);
#endif
        w->zoomed = 0;
    }
    set_document_frame(wm, w, target, 0);
    return w->zoomed != 0;
}

int CollapseWindow(WindowMgr *wm, WindowPtr w)
{
    rgn_rect_t target;
    if (wm == NULL || w == NULL) WIN_PANIC("CollapseWindow: NULL");
    if (!w->collapsed) {
        w->collapseState = WindowFrameRect(w);
        target = w->collapseState;
        target.bottom = (int16_t)(target.top + FLAIR_CHROME_TITLEBAR_H);
        w->collapsed = 1;
        set_document_frame(wm, w, target, 1);
    } else {
        target = w->collapseState;
        w->collapsed = 0;
        set_document_frame(wm, w, target, 0);
    }
    return w->collapsed != 0;
}
#endif /* WINDOW_ENABLE_R1_OPS */

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

void WindowMgr_invalidate_desktop(WindowMgr *wm, rgn_rect_t rect)
{
    if (wm == NULL) WIN_PANIC("invalidate_desktop: NULL");

    /* damage := region(rect) INTERSECT desktop_frame. scratch_b is the
     * exposure carrier expected by distribute_exposure; scratch_a/scratch_c
     * are free for its front-to-back owner partition after this intersection. */
    region_t *rr = wm->scratch_a;
    region_t *frame = wm->scratch_c;
    region_t *damage = wm->scratch_b;
    region_set_rect(rr, rect);
    region_set_rect(frame, wm->desktop_frame);
    region_op(damage, rr, frame, RGN_OP_INTERSECT);

    /* A compositor-layer drawing obscures whichever ordinary owner was below
     * it. Treat removing that drawing as fresh exposure with no departing
     * WindowRecord: each visible window claims its frontmost portion; whatever
     * remains is bare-desktop damage. DQ2 then repaints chrome and routes the
     * still-pending window updateRgns to their content owners. */
    distribute_exposure(wm, (WindowPtr)0, damage);
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

        /* Chrome zones consume the same sampled constants and record flags as
         * chrome.c. Inactive windows draw no widgets, so their whole title is
         * inDrag; the click activates before a gadget can act. */
        rgn_rect_t s = WindowFrameRect(p);
        int16_t ri = (int16_t)(s.right - 1);
        int16_t bi = (int16_t)(s.bottom - 1);
        int16_t by = (int16_t)(s.top + FLAIR_CHROME_WIDGET_TOP_OFF);
#if defined(WINDOW_MUTATE_ZONE_OFF)
        int16_t zone_shift = 3;
#else
        int16_t zone_shift = 0;
#endif

        if (p->hilited &&
            (p->widgetFlags & FLAIR_WINDOW_WIDGET_CLOSE) != 0u &&
            p->goAwayFlag) {
            int16_t bx = (int16_t)(s.left + FLAIR_CHROME_CLOSE_LEFT_OFF +
                                   zone_shift);
            if (h >= bx && h < (int16_t)(bx + FLAIR_CHROME_WIDGET_BOX) &&
                v >= by && v < (int16_t)(by + FLAIR_CHROME_WIDGET_BOX))
                return inGoAway;
        }
        if (p->hilited &&
            (p->widgetFlags & FLAIR_WINDOW_WIDGET_ZOOM) != 0u) {
            int16_t bx = (int16_t)(ri - FLAIR_CHROME_ZOOM_RIGHT_OFF +
                                   zone_shift);
            if (h >= bx && h < (int16_t)(bx + FLAIR_CHROME_WIDGET_BOX) &&
                v >= by && v < (int16_t)(by + FLAIR_CHROME_WIDGET_BOX))
                return p->zoomed ? inZoomOut : inZoomIn;
        }
        if (p->hilited &&
            (p->widgetFlags & FLAIR_WINDOW_WIDGET_COLLAPSE) != 0u) {
            int16_t bx = (int16_t)(ri - FLAIR_CHROME_COLLAPSE_RIGHT_OFF +
                                   zone_shift);
            if (h >= bx && h < (int16_t)(bx + FLAIR_CHROME_WIDGET_BOX) &&
                v >= by && v < (int16_t)(by + FLAIR_CHROME_WIDGET_BOX))
                return inCollapse;
        }
        if (p->hilited && !p->collapsed &&
            (p->widgetFlags & FLAIR_WINDOW_WIDGET_GROW) != 0u) {
            int16_t gx = (int16_t)(ri - FLAIR_CHROME_GROW_RIGHT_OFF +
                                   zone_shift);
            int16_t gy = (int16_t)(bi - FLAIR_CHROME_GROW_BOTTOM_OFF +
                                   zone_shift);
            if (h >= gx && h < (int16_t)(gx + FLAIR_CHROME_GROW) &&
                v >= gy && v < (int16_t)(gy + FLAIR_CHROME_GROW))
                return inGrow;
        }
        /* any other chrome pixel (title bar, frame) is the drag region. */
        return inDrag;
    }

    if (whichWindow) *whichWindow = NULL;
    return inDesk;
}
