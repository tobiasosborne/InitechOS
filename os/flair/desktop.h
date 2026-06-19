/*
 * os/flair/desktop.h -- the FLAIR desktop compositor / repaint glue (THE ARTIFACT).
 *
 * beads: initech-87a ("Draw a System-7 window and drag it with correct clipping")
 *        -- the M3 drag-gate capstone. ADR-0004 AM-8: the earliest human-verifiable
 *        Law-4 fidelity moment. This module is the COMPOSITOR that ties the FLAIR
 *        Managers (Window Manager z-order + DiffRgn damage) to the ONE surface
 *        (via the region-clipped blitter + the System-7 chrome drawer) so a window
 *        drags across the desktop with correct update regions, NO over-repaint, and
 *        the chrome unchanged outside the damaged area (ADR-0004 D-5).
 *
 * WHAT THIS IS (FLAIR Layer 5 glue, ADR-0004 D-1): the minimal-repaint compositor.
 * It owns NO pixel path of its own -- every pixel is written through the
 * region-clipped blitter (os/flair/blitter.h: blitter_fill_rect_clipped, which
 * calls ONLY the ONE surface module) for the desktop background and through the
 * System-7 chrome drawer (os/flair/chrome.h: flair_draw_document_window, which
 * also writes ONLY through the surface module, clipped to visRgn INTERSECT clipRgn)
 * for each window's chrome. There is NO second pixel path (ADR-0004 D-2 / C-2).
 *
 * THE TWO ENTRY POINTS:
 *
 *   desktop_paint_all     -- the FULL initial paint: fill the desktop background
 *                            (seafoam, INITECH_DESKTOP_BG_RGB; OD-4) over the whole
 *                            desktop frame, then draw every VISIBLE window's chrome
 *                            BACK-to-FRONT, each clipped to its own visible region
 *                            (strucRgn DIFF union-of-fronts). Used for frame 0 and
 *                            as the independent "paint everything from scratch"
 *                            reference the drag gate diffs against.
 *
 *   desktop_paint_damage  -- the D-5 MINIMAL-REPAINT compositor: for the desktop's
 *                            own update area, fill seafoam clipped to the desktop
 *                            update region; for each window with a NON-EMPTY
 *                            updateRgn, redraw its chrome clipped to
 *                            visible(W) INTERSECT updateRgn -- i.e. it touches ONLY
 *                            damaged pixels. It then VALIDATES (clears) every
 *                            updateRgn it consumed (the pump's EndUpdate analogue).
 *                            This is the headline drag-gate behaviour: no
 *                            over-repaint, chrome unchanged outside the damage.
 *
 * STORAGE DISCIPLINE (Law 3, freestanding; ADR-0004 DEC-03 FLAIR heap): this module
 * mallocs nothing. Like the Window Manager and the region engine, it takes a
 * CALLER-SUPPLIED scratch region (arena-backed; rows[]/x_pool attached) to hold the
 * per-window visible region while painting. The Window Manager's own three scratch
 * regions are used INTERNALLY by ComputeVisible, so the compositor must NOT reuse
 * them as its visible-region carrier -- it needs one more, distinct, caller region.
 *
 * THE EFFECTIVE-CLIP SEAM (ADR-0004 D-1/D-2): the chrome drawer clips to
 * port->visRgn INTERSECT port->clipRgn. The compositor uses that directly:
 *   - full paint:   port.visRgn = visible(W),  port.clipRgn = the whole frame.
 *   - damage paint: port.visRgn = visible(W),  port.clipRgn = W->updateRgn.
 * The intersection visible(W) INTERSECT updateRgn is exactly the D-5 repaint set
 * "the part of the window's visible region that owes a repaint" -- computed by the
 * region engine the SAME way the rest of the Toolbox computes a clip (no special
 * case). The desktop background fill goes through the blitter, clipped to the
 * desktop frame (full paint) or the desktop update region (damage paint).
 *
 * DUAL-COMPILE (the window.c / blitter.c / chrome.c pattern; Law 3): desktop.c
 * compiles BOTH freestanding for the kernel (gcc -m32 -ffreestanding -nostdlib
 * -std=c11 -Wall -Wextra -Werror) AND hosted for the drag gate
 * (harness/proptest/test_drag.c). It uses only <stdint.h> + the FLAIR/spec headers;
 * no host malloc; no libc. Fail-loud (Rule 2) on a NULL manager / NULL dst /
 * unattached scratch.
 *
 * Ref: ADR-0004 D-1 (5-layer stack; all drawing through a region clip), D-2 (one
 *      surface module; no second pixel path), D-5 (DiffRgn damage; minimal repaint,
 *      no over-repaint), AM-8 (the drag gate). PRD Sec 6.2 (region algebra), Sec 6.3
 *      (Toolbox). os/flair/window.h (WindowMgr, ComputeVisible, WindowMgr_validate),
 *      os/flair/blitter.h (blitter_fill_rect_clipped), os/flair/chrome.h
 *      (flair_draw_document_window), os/flair/surface.h (bitmap_t), spec/grafport.h
 *      (GrafPort), spec/region_algebra.h (region_t), spec/assets/palette.h
 *      (INITECH_DESKTOP_BG_RGB seafoam). CLAUDE.md Law 2 (oracle is truth), Law 3
 *      (freestanding dual-compile), Law 4 (look like the frame), Rule 2 (fail loud),
 *      Rule 3 (all bugs deep), Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_DESKTOP_H
#define INITECH_OS_FLAIR_DESKTOP_H

#include <stdint.h>

#include "window.h"            /* WindowMgr, ComputeVisible, WindowMgr_validate */
#include "surface.h"           /* bitmap_t (the ONE pixel-buffer descriptor)    */
#include "region_algebra.h"    /* region_t, rgn_rect_t (-Ispec)                 */

/* ---------------------------------------------------------------------------
 * The desktop background palette index (indexed-8 / OD-2). It matches the host
 * render skeleton's RENDER_DESKTOP_INDEX and chrome.c's CIDX_DESKTOP so the 8bpp
 * offscreen reads back a consistent "this is the bare desktop" value, and it maps
 * (via the render palette / the surface conversion site) to INITECH_DESKTOP_BG_RGB
 * (seafoam, OD-4) -- the canon the desktop paints. For a 32bpp destination the
 * compositor writes the packed seafoam RGB directly (the canon value).
 * ------------------------------------------------------------------------- */
#define FLAIR_DESKTOP_BG_INDEX  2u

/* ---------------------------------------------------------------------------
 * desktop_paint_all -- the FULL initial paint.
 *
 * Fills the desktop background (seafoam) over the manager's desktop_frame, then
 * draws every VISIBLE window's System-7 chrome BACK-to-FRONT, each clipped to its
 * own visible region (strucRgn DIFF union-of-fronts; the part not obscured by
 * windows in front). `dst` is the destination bitmap (LFB or offscreen). `scratch`
 * is a caller-supplied, attached (rows[]/x_pool) region the compositor uses to
 * hold each window's visible region; it MUST be DISTINCT from the manager's three
 * internal scratch regions and from every window's strucRgn/contRgn/updateRgn.
 *
 * Fail-loud (Rule 2) on a NULL manager / NULL dst / NULL or unattached scratch.
 * ------------------------------------------------------------------------- */
void desktop_paint_all(WindowMgr *wm, const bitmap_t *dst, region_t *scratch);

/* ---------------------------------------------------------------------------
 * desktop_paint_damage -- the D-5 MINIMAL-REPAINT compositor.
 *
 * Repaints ONLY damaged pixels:
 *   - the desktop background is re-filled with seafoam clipped to the manager's
 *     desktop update region (the bare-desktop pixels the moved window vacated),
 *   - each VISIBLE window with a NON-EMPTY updateRgn has its chrome redrawn,
 *     clipped to visible(W) INTERSECT updateRgn.
 * After painting, every updateRgn the compositor consumed (each window's + the
 * desktop's) is VALIDATED (cleared) -- the BeginUpdate/EndUpdate analogue.
 *
 * This is the headline drag-gate property (ADR-0004 D-5 / AM-8): it touches ONLY
 * damaged pixels, so the chrome is unchanged outside the damaged area and there is
 * no over-repaint / flicker. `dst` and `scratch` are as in desktop_paint_all.
 *
 * Fail-loud (Rule 2) on a NULL manager / NULL dst / NULL or unattached scratch.
 * ------------------------------------------------------------------------- */
void desktop_paint_damage(WindowMgr *wm, const bitmap_t *dst, region_t *scratch);

#endif /* INITECH_OS_FLAIR_DESKTOP_H */
