/*
 * os/flair/desktop.c -- the FLAIR desktop compositor / repaint glue (THE ARTIFACT).
 *
 * beads: initech-87a (the M3 drag-gate capstone; ADR-0004 AM-8). See desktop.h for
 *        the full contract, the effective-clip seam, and the storage discipline.
 *
 * Ref:   ADR-0004 D-1 (5-layer stack), D-2 (one surface module; no second pixel
 *        path -- desktop background goes through the region-clipped blitter, chrome
 *        through the chrome drawer; both write ONLY via the surface module), D-5
 *        (DiffRgn damage; MINIMAL repaint, NO over-repaint -- the compositor touches
 *        ONLY damaged pixels), AM-8 (the drag gate). PRD Sec 6.2 / 6.3.
 *        os/flair/window.h, os/flair/blitter.h, os/flair/chrome.h,
 *        os/flair/surface.h, os/flair/flair_look.h (the C-8 policy seam),
 *        spec/grafport.h, spec/region_algebra.h.
 *        CLAUDE.md Law 2/3/4, Rule 2/3/11/12.
 *
 * C-8 (ADR-0004-AMENDMENT-DEC-09 Sec 3.1/5.1): desktop.c is MECHANISM
 * (compositor / repaint geometry).  It names NO color: the desktop background
 * pixel is resolved through the ONE policy seam flair_look_pixel_depth(bpp,
 * FLAIR_PART_DESKTOP) (os/flair/flair_look.h), the bitmap-only variant of the
 * seam (the compositor fills a bitmap_t with no GrafPort).  ZERO 0xRRGGBB
 * literal, ZERO INITECH_*_RGB, ZERO index->RGB switch below the cut-line.
 *
 * STORAGE (Law 3): mallocs nothing. The per-window visible region is held in a
 * CALLER-SUPPLIED, attached scratch region (distinct from the manager's three
 * internal scratch regions, which ComputeVisible uses, and from every window's own
 * regions). The aliasing rule (region_op out distinct from inputs) is respected:
 * ComputeVisible writes `scratch` using the manager scratch internally; the
 * compositor then only READS `scratch` (as the port visRgn) -- no further op.
 *
 * MUTATION SWITCHES (Rule 6): test_drag.c compiles this file with two named
 * mutants to prove the drag gate bites. The default build defines neither.
 *
 *   DRAG_MUTATE_SKIP_EXPOSED -- in desktop_paint_damage, SKIP filling the desktop's
 *     vacated (exposed) area. The bare-desktop pixels the moved window left behind
 *     keep their STALE old chrome -> frame1 != the from-scratch reference at the
 *     vacated area: assertions (a) [bit-exact match] and (c) [newly-exposed shows
 *     what's behind] go RED. This is the "forgot to repaint the hole" bug.
 *
 *   DRAG_MUTATE_NO_CLIP -- in desktop_paint_damage, IGNORE the per-element clip:
 *     fill the desktop background over the WHOLE desktop frame (not clipped to the
 *     desktop update region) and paint each damaged window clipped to the whole
 *     frame (not visible(W) INTERSECT updateRgn). The unclipped whole-frame seafoam
 *     fill STOMPS the pixels of every stationary window -> pixels OUTSIDE the
 *     computed damage union change owner: assertion (b) [differing pixels ==
 *     damage union -- NO over-repaint] goes RED (and (a) RED). This is the
 *     flicker/over-repaint bug D-5 exists to forbid.
 *
 * A THIRD mutant lives on desktop_paint_all (beads initech-jmc5/-qi8v; the emu O-5
 * gate's own mutant target, not test_drag.c's):
 *
 *   DESKTOP_MUTATE_NO_PAINTALL_CLEAR -- desktop_paint_all does NOT reset
 *     wm->desktop_update at its tail. A desktop_update seeded BEFORE a
 *     desktop_paint_all (e.g. by a HideWindow the caller issued earlier, whose
 *     footprint desktop_paint_all's from-scratch composite already rendered over
 *     correctly) survives into the NEXT desktop_paint_damage call as stale
 *     geometry. That call's step-1 seafoam fill then clips to that stale footprint
 *     and teal-stomps whatever window chrome now occupies it, without repainting
 *     it (the window loop only repaints windows with a non-empty updateRgn, and
 *     the stomped window's chrome was never marked dirty in that same call) -- the
 *     initech-jmc5 app-switch erasure and initech-qi8v drag erasure (same root
 *     cause, two call sites). See desktop.h desktop_paint_all for the fix
 *     rationale.
 */
#include <stdint.h>

#include "desktop.h"
#include "window.h"             /* WindowMgr, ComputeVisible, WindowMgr_validate */
#include "blitter.h"            /* blitter_fill_rect_clipped (clipped bg fill)   */
#include "chrome.h"             /* flair_draw_document_window (clipped chrome)   */
#include "surface.h"            /* bitmap_t                                      */
#include "grafport.h"           /* GrafPort, FLAIR_BitMap                         */
#include "region_algebra.h"     /* region_t, rgn_rect_t, region_is_empty         */
#include "flair_look.h"         /* flair_look_pixel_depth + FLAIR_PART_* (seam)  */

/* ---------------------------------------------------------------------------
 * Fail-loud (dual: abort hosted / deterministic hang in-kernel), mirroring the
 * window.c / region engine convention so desktop.c dual-compiles with only the
 * FLAIR/spec headers (no panic.h dependency).
 * ------------------------------------------------------------------------- */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1 && !defined(__KERNEL_FREESTANDING__)
#  include <stdlib.h>   /* abort -- hosted only */
#  define DESK_PANIC(msg)  abort()
#else
#  define DESK_PANIC(msg)  do { for (;;) { } } while (0)
#endif

/* The desktop background pixel value for this destination depth.  C-8: this
 * MECHANISM names NO color -- it names FLAIR_PART_DESKTOP and resolves PART ->
 * destination pixel through the ONE policy seam flair_look_pixel_depth (the
 * bitmap-only variant; the compositor fills a bitmap_t with no GrafPort).
 *   8bpp     -> the desktop palette index byte (CIDX_DESKTOP == idx2 teal).
 *   32/24bpp -> the packed canon teal RGB (flair_canon_rgb(CIDX_DESKTOP)).
 * Byte-identical to the prior named-constant site (same value, now resolved
 * through the seam rather than named here). */
static uint32_t desktop_px(const bitmap_t *dst)
{
    return flair_look_pixel_depth(dst->bpp, FLAIR_PART_DESKTOP);
}

/* Build a GrafPort over `dst` with the given visRgn / clipRgn so the chrome drawer
 * clips to visRgn INTERSECT clipRgn (ADR-0004 D-1/D-2). portRect is the whole
 * bitmap (the surface module enforces the clip at the pixel level). */
static void make_port(GrafPort *port, const bitmap_t *dst,
                      region_t *visRgn, region_t *clipRgn)
{
    rgn_rect_t whole;
    whole.top    = 0;
    whole.left   = 0;
    whole.bottom = (int16_t)dst->height;
    whole.right  = (int16_t)dst->width;

    port->portBits.bm     = *dst;
    port->portBits.bounds = whole;
    port->portRect        = whole;
    port->visRgn          = visRgn;
    port->clipRgn         = clipRgn;
    port->pnLoc.v  = 0;
    port->pnLoc.h  = 0;
    port->pnSize.v = 1;
    port->pnSize.h = 1;
    port->pnVis    = 0;
    port->grafProcs = (QDProcs *)0;
}

/* Draw one window's Platinum chrome clipped to (visRgn INTERSECT clipRgn).
 * WindowFrameRect decouples the drawer frame from the larger strucRgn that owns
 * the shadow band (initech-9d0e). Global == port-local on this offscreen. */
static void paint_window_chrome(const bitmap_t *dst, WindowPtr w,
                                region_t *visRgn, region_t *clipRgn)
{
    GrafPort port;
    const flair_skin_t *skin = flair_look_default_skin();
    rgn_rect_t frame = WindowFrameRect(w);
    make_port(&port, dst, visRgn, clipRgn);
    /* w->hilited (spec/window_record.h; set by window.c reaffirm_active on every
     * BringToFront/SendBehind) drives the StandardWDEF active/inactive title-bar
     * split (beads initech-a9iq) -- already in scope here, just never consumed
     * before this fix. */
    flair_draw_document_window(&port, skin, frame,
                               w->titleHandle, w->hilited, w->widgetFlags);
}

/* Recursive back-to-front walk: paint the window list from the BACK (tail) toward
 * the FRONT (head) so a front window's chrome lands on top. Each window is clipped
 * to its OWN visible region, so order is not strictly required for correctness, but
 * back-to-front is the authentic painter's-algorithm order. Recursion depth is the
 * (small, bounded) window count; no malloc (Law 3).
 *
 * The chrome is clipped to visible(W): we pass `scratch` (which holds visible(W))
 * as BOTH the port's visRgn AND clipRgn; their intersection is visible(W) itself,
 * so no separate whole-frame region (and no extra scratch) is needed. */
static void paint_back_to_front(WindowMgr *wm, WindowPtr w,
                                const bitmap_t *dst, region_t *scratch)
{
    if (w == NULL) {
        return;
    }
    paint_back_to_front(wm, w->nextWindow, dst, scratch);   /* deeper first */
    if (!w->visible) {
        return;
    }
    /* visible(W) = strucRgn DIFF union-of-fronts, into the caller scratch. */
    ComputeVisible(wm, w, scratch);
    if (region_is_empty(scratch)) {
        return;
    }
    paint_window_chrome(dst, w, scratch, scratch);
}

void desktop_paint_all(WindowMgr *wm, const bitmap_t *dst, region_t *scratch)
{
    if (wm == NULL || dst == NULL || scratch == NULL) {
        DESK_PANIC("desktop_paint_all: NULL");
    }
    if (scratch->rows == NULL || scratch->x_pool == NULL) {
        DESK_PANIC("desktop_paint_all: scratch unattached");
    }
    if (dst->base == NULL || dst->width == 0u || dst->height == 0u) {
        DESK_PANIC("desktop_paint_all: bad dst");
    }

    /* 1. Fill the whole desktop background (seafoam) over the desktop frame.
     * NULL clip == no additional clip (blitter draws the whole rect). */
    blitter_fill_rect_clipped(dst, wm->desktop_frame, desktop_px(dst), (const region_t *)0);

    /* 1b. THE DESKTOP UNDERLAY (beads initech-tdnl.9; design F2.3 seam 1 /
     * F2-4): immediately after the base fill, with THIS site's clip (NULL ==
     * the whole desktop frame). desktop.c stays MECHANISM -- it names no color
     * and knows no icon; the Finder's painter does. NULL hook == every
     * non-Finder build is byte-identical (window.h). */
    if (wm->desktop_underlay != NULL) {
        wm->desktop_underlay(wm->desktop_underlay_user, dst, (const region_t *)0);
    }

    /* 2. Paint every visible window BACK-to-FRONT, each clipped to its visible
     * region (the chrome drawer clips to visRgn INTERSECT clipRgn; we pass
     * visible(W) as both, so the effective clip is exactly visible(W)). */
    paint_back_to_front(wm, wm->front, dst, scratch);

    /* 3. RESET wm->desktop_update (beads initech-jmc5/-qi8v; desktop.h has the
     * full rationale). A full from-scratch composite satisfies ALL pending
     * desktop-background damage by construction -- every desktop pixel this
     * manager owns was just repainted -- so no desktop_update seeded before this
     * call (e.g. by a HideWindow the caller issued earlier) may survive it. If it
     * did, the NEXT desktop_paint_damage would clip its seafoam fill to that now-
     * meaningless stale footprint and teal-stomp whatever window chrome currently
     * sits there without repainting it (that call's window loop only repaints
     * windows with a non-empty updateRgn). */
#ifndef DESKTOP_MUTATE_NO_PAINTALL_CLEAR
    region_set_empty(wm->desktop_update);
#else
    /* MUTANT (Rule 6; the O-5 gate's own mutant target): do NOT clear -- restores
     * the initech-jmc5/-qi8v erasure so the new BACKGROUND-FOOTPRINT oracle tier
     * (tools/ppm_flair_appswitch_check.c) is proven to bite. NEVER in a real
     * build. */
#endif
}

void desktop_paint_damage(WindowMgr *wm, const bitmap_t *dst, region_t *scratch)
{
    if (wm == NULL || dst == NULL || scratch == NULL) {
        DESK_PANIC("desktop_paint_damage: NULL");
    }
    if (scratch->rows == NULL || scratch->x_pool == NULL) {
        DESK_PANIC("desktop_paint_damage: scratch unattached");
    }
    if (dst->base == NULL || dst->width == 0u || dst->height == 0u) {
        DESK_PANIC("desktop_paint_damage: bad dst");
    }

    uint32_t bg = desktop_px(dst);

    /* 1. Repaint the bare desktop where the moved window vacated it -- seafoam
     * clipped to the desktop update region (ONLY the damaged background pixels). */
#if defined(DRAG_MUTATE_SKIP_EXPOSED)
    /* MUTANT: skip the vacated-area fill. The hole keeps its stale old chrome ->
     * frame1 != the from-scratch reference at the vacated pixels: (a)/(c) RED. */
    (void)bg;
#elif defined(DRAG_MUTATE_NO_CLIP)
    /* MUTANT: fill the WHOLE desktop frame with NO clip -- stomps every stationary
     * window's pixels with seafoam, changing pixels OUTSIDE the computed damage
     * union: (b) [no over-repaint] RED (and (a) RED). */
    blitter_fill_rect_clipped(dst, wm->desktop_frame, bg, (const region_t *)0);
#else
    if (!region_is_empty(wm->desktop_update)) {
        rgn_rect_t dbb = region_get_bbox(wm->desktop_update);
        blitter_fill_rect_clipped(dst, dbb, bg, wm->desktop_update);
    }
#endif

    /* 1b. THE DESKTOP UNDERLAY (beads initech-tdnl.9; design F2.3 seam 1 /
     * F2-4): immediately after the base fill, clipped to EXACTLY the desktop
     * damage this call just serviced -- so an icon partially covered by a
     * window edge is clipped to pixels no window owns and can never paint over
     * chrome. An empty desktop_update means no bare-desktop pixel changed, so
     * there is nothing for the underlay to redraw either. */
    if (wm->desktop_underlay != NULL && !region_is_empty(wm->desktop_update)) {
        wm->desktop_underlay(wm->desktop_underlay_user, dst, wm->desktop_update);
    }

    /* 2. Redraw each VISIBLE window with a NON-EMPTY updateRgn, clipped to
     * visible(W) INTERSECT updateRgn (ONLY the damaged part of that window). */
    for (WindowPtr w = wm->front; w != NULL; w = w->nextWindow) {
        if (!w->visible) {
            continue;
        }
        if (region_is_empty(w->updateRgn)) {
            continue;
        }
        /* visible(W) into the caller scratch (manager scratch used internally). */
        ComputeVisible(wm, w, scratch);
        if (region_is_empty(scratch)) {
            continue;
        }
#if defined(DRAG_MUTATE_NO_CLIP)
        /* MUTANT: paint the window over its FULL structure with no clip at all
         * (visRgn == clipRgn == the whole frame is faked by passing the window's
         * own struc region as both, ignoring updateRgn) -- over-repaints the whole
         * window every step, compounding the over-repaint the whole-frame fill
         * already triggers: (b) RED. */
        paint_window_chrome(dst, w, w->strucRgn, w->strucRgn);
#else
        /* Effective clip = visible(W) INTERSECT updateRgn: visRgn = visible(W),
         * clipRgn = updateRgn. The chrome drawer intersects them (D-1/D-2). */
        paint_window_chrome(dst, w, scratch, w->updateRgn);
#endif
    }

    /* 3. Clear ONLY the desktop-background damage -- this painter fully serviced
     * it in step 1 (no tenant ever paints the bare desktop, so ownership stays
     * here). The WINDOW updateRgns are deliberately LEFT PENDING (beads
     * initech-gofc; epic initech-av7s DQ1, ratified 2026-07-31): this painter
     * draws WDEF CHROME only -- the CONTENT inside each damaged window is still
     * owed to its owning app, and validating here destroyed that damage before
     * any updateEvt could be delivered (the WL-0075 white-hole family). The
     * pump's content phase clears them: flair_route_updates (deliver, then
     * validate) in a tenants scene, or desktop_validate_all when no app will
     * ever repaint (the chrome IS the full paint). */
    region_set_empty(wm->desktop_update);
}

void desktop_validate_all(WindowMgr *wm)
{
    if (wm == NULL) {
        DESK_PANIC("desktop_validate_all: NULL");
    }
    for (WindowPtr w = wm->front; w != NULL; w = w->nextWindow) {
        WindowMgr_validate(w);
    }
#ifndef DESKTOP_MUTATE_NO_PAINTALL_CLEAR
    region_set_empty(wm->desktop_update);
#else
    /* MUTANT (Rule 6; shared knob with desktop_paint_all above): the stale-
     * desktop_update mutation must defeat BOTH books-closing paths -- the
     * paint_all tail reset AND this explicit validate -- or the second one
     * masks it and the TIER-C oracle grades a mutant that cannot occur (the
     * winh x ojxn double-backstop lesson, WL-0068). Window validation stays
     * LIVE either way: the mutation models ONLY the jmc5/qi8v stale-desktop
     * footprint. NEVER in a real build. */
#endif
}
