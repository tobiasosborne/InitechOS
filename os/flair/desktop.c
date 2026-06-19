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
 *        os/flair/surface.h, spec/grafport.h, spec/region_algebra.h,
 *        spec/assets/palette.h (INITECH_DESKTOP_BG_RGB seafoam).
 *        CLAUDE.md Law 2/3/4, Rule 2/3/11/12.
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
 */
#include <stdint.h>

#include "desktop.h"
#include "window.h"             /* WindowMgr, ComputeVisible, WindowMgr_validate */
#include "blitter.h"            /* blitter_fill_rect_clipped (clipped bg fill)   */
#include "chrome.h"             /* flair_draw_document_window (clipped chrome)   */
#include "surface.h"            /* bitmap_t, surface_pack_rgb                     */
#include "grafport.h"           /* GrafPort, FLAIR_BitMap                         */
#include "region_algebra.h"     /* region_t, rgn_rect_t, region_is_empty         */
#include "palette.h"            /* INITECH_DESKTOP_BG_RGB (seafoam; -Ispec/assets)*/

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

/* The desktop background pixel value for this destination depth. 8bpp: the
 * desktop palette index (FLAIR_DESKTOP_BG_INDEX, == RENDER_DESKTOP_INDEX, maps to
 * seafoam through the palette). 32bpp/24bpp: the packed seafoam RGB directly (the
 * OD-4 canon INITECH_DESKTOP_BG_RGB). One conversion site, branching on bpp. */
static uint32_t desktop_px(const bitmap_t *dst)
{
    if (dst->bpp == 8u) {
        return (uint32_t)FLAIR_DESKTOP_BG_INDEX;
    }
    return surface_pack_rgb(dst->bpp, 0, 0, 0) | (uint32_t)INITECH_DESKTOP_BG_RGB;
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

/* Draw one window's System-7 chrome clipped to (visRgn INTERSECT clipRgn). The
 * chrome geometry comes from the window's structure bounding box (global coords;
 * the offscreen IS the screen, so port-local == global). */
static void paint_window_chrome(const bitmap_t *dst, WindowPtr w,
                                region_t *visRgn, region_t *clipRgn)
{
    GrafPort port;
    rgn_rect_t frame = region_get_bbox(w->strucRgn);
    make_port(&port, dst, visRgn, clipRgn);
    flair_draw_document_window(&port, frame);
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

    /* 2. Paint every visible window BACK-to-FRONT, each clipped to its visible
     * region (the chrome drawer clips to visRgn INTERSECT clipRgn; we pass
     * visible(W) as both, so the effective clip is exactly visible(W)). */
    paint_back_to_front(wm, wm->front, dst, scratch);
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

    /* 3. VALIDATE (clear) every updateRgn the compositor consumed -- the
     * BeginUpdate/EndUpdate analogue. The desktop update is cleared too. */
    for (WindowPtr w = wm->front; w != NULL; w = w->nextWindow) {
        WindowMgr_validate(w);
    }
    region_set_empty(wm->desktop_update);
}
