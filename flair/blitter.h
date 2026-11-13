/*
 * os/flair/blitter.h -- the FLAIR region-clipped blitter (THE ARTIFACT).
 *
 * beads: initech-i50 ("Blitter with region clipping (visRgn/clipRgn)"; copy/fill
 *        clipped to a region -- the substrate for window drawing and update
 *        regions). Depends on initech-b5g (region engine), initech-k8o5.6 (the
 *        ONE surface module), initech-slz (LFB). Blocks initech-87a (window drag).
 *
 * WHAT THIS IS: the load-bearing CLIPPING primitive of FLAIR. Every Manager and
 * the window-drag path draw THROUGH this layer; a wrong clip corrupts the whole
 * Toolbox (CLAUDE.md Rule 3 "all bugs are deep"). It is a thin LAYER OVER the ONE
 * surface module (os/flair/surface.h): it decides WHICH pixels are inside the
 * effective clip region (the visRgn INTERSECT clipRgn a Manager passes) and then
 * writes ONLY those, exclusively via surface_fill_span / surface_put_pixel. There
 * is NO second pixel path (ADR-0004 D-2 / C-2): the blitter never touches base[]
 * directly.
 *
 * THE CLIP RULE (ADR-0004 D-1/D-2, MTE Ch 4; grafport.h CLIPPING INVARIANT):
 *   ALL DRAWING IS CLIPPED BY  visRgn INTERSECT clipRgn.
 * The Manager intersects visRgn and clipRgn into one effective clip region (via
 * the ATKINSON region_op INTERSECT, ADR-0005) and hands it to the blitter. The
 * blitter writes a destination pixel IF AND ONLY IF that pixel is:
 *   (a) within the blit/fill rectangle, AND
 *   (b) inside the clip region.
 * A NULL clip means "no additional clip" -- draw the whole rect (clip == the
 * universe), matching grafport.h ("A NULL clipRgn means no additional clip").
 *
 * Ref: PRD Sec 6.2 (region algebra -- clipping reduces to region membership),
 *      Sec 6.3 (the Toolbox layer; the GrafPort is the sole conduit).
 *      ADR-0004 D-1 (5-layer stack; drawing clipped by an ATKINSON region),
 *      D-2 (one surface module; no second pixel path; QDProcs bitsProc seam).
 *      spec/region_algebra.h (region_t; region_contains_point; per-scanline
 *      inversion-list rep -- a row valid for [y_top, next.y_top), half-open span
 *      pairs [x_2k, x_2k+1)). os/flair/surface.h (bitmap_t; the ONE writer).
 *      os/flair/chrome.c (the ad-hoc per-pixel clip_in()/cfill() this primitive
 *      GENERALIZES into a reusable, scanline-span-walking clipped blit/fill).
 *      CLAUDE.md Law 2 (oracle is truth), Law 3 (freestanding artifact +
 *      dual-compile), Rule 2 (fail loud / bounds-checked), Rule 3 (all bugs
 *      deep), Rule 6 (mutation-proven oracle), Rule 11 (reproducible), Rule 12
 *      (ASCII-clean).
 *
 * DUAL-COMPILE (the surface.c / region.c pattern): blitter.c compiles BOTH
 * freestanding for the kernel (gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall
 * -Wextra -Werror) AND hosted for the property suite (harness/proptest/
 * test_blitter.c). It does NO host malloc and uses only <stdint.h> + the surface
 * and region headers; all storage is caller-supplied (the dst bitmap, the src
 * pixels, the region pools). Bounds-checked: nothing is written past the dst
 * buffer (the surface module additionally clips to the bitmap bounds -- Rule 2).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_BLITTER_H
#define INITECH_OS_FLAIR_BLITTER_H

#include <stdint.h>

#include "surface.h"                 /* bitmap_t + the ONE pixel writer  */
#include "region_algebra.h"          /* region_t, rgn_rect_t (-Ispec)    */

/* --------------------------------------------------------------------------
 * blitter_fill_rect_clipped -- fill the rectangle `rect` (half-open
 * [left,right) x [top,bottom), QuickDraw Rect order) with packed color `px`
 * (the surface canonical 0x00RRGGBB / palette-index low byte, exactly what
 * surface_put_pixel expects for dst->bpp), writing a pixel IFF it is inside
 * `clip`.
 *
 *   clip == NULL  -> no clip; fill the whole rect (clip == the universe).
 *   clip != NULL  -> write pixel (x,y) IFF region_contains_point(clip,x,y).
 *
 * Per scanline the routine span-walks `clip`'s inversion list (the row covering
 * y) intersected with [rect.left, rect.right), emitting each maximal in-clip run
 * as a single surface_fill_span (efficiency), so it does not pay a region query
 * per pixel. Correctness is the bar; the span walk is the same set of pixels a
 * per-pixel region_contains_point loop would write (proven by the oracle).
 *
 * Bounds-checked / fail-soft on a NULL/zero dst (Rule 2); the surface module
 * clips every span to the bitmap bounds so no write lands past the buffer.
 * -------------------------------------------------------------------------- */
void blitter_fill_rect_clipped(const bitmap_t *dst, rgn_rect_t rect,
                               uint32_t px, const region_t *clip);

/* --------------------------------------------------------------------------
 * blitter_blit_clipped -- copy a w x h block of source pixels to dst at
 * destination top-left (x,y), writing a destination pixel IFF it is inside
 * `clip`.
 *
 *   src       -- source pixels, ROW-MAJOR, src_pitch bytes per row, each pixel
 *                one entry of the SAME canonical surface value as dst expects
 *                (i.e. src pixels are already in the dst's pixel representation;
 *                this is a copy blit, srcCopy semantics, not a depth converter).
 *                Indexed as src[(sy)*src_pitch_pixels + sx] where src_pitch is
 *                given in PIXELS (src_pitch_px), so the caller controls stride.
 *   x, y      -- destination top-left (may be negative for partial off-screen).
 *   w, h      -- block size in pixels.
 *   clip      -- effective visRgn INTERSECT clipRgn (NULL => no clip).
 *
 * A destination pixel (dx,dy) for dx in [x,x+w), dy in [y,y+h) receives
 * src[(dy-y)*src_pitch_px + (dx-x)] IFF (dx,dy) is inside `clip`. Writes go ONLY
 * through surface_put_pixel. Bounds-checked: pixels off the dst bitmap, or with
 * negative coordinates, are skipped (Rule 2 -- never write past the buffer).
 *
 * `src_pitch_px` is the source stride in PIXELS (>= w). Passing exactly w means
 * a tightly-packed block.
 * -------------------------------------------------------------------------- */
void blitter_blit_clipped(const bitmap_t *dst,
                          const uint32_t *src, uint32_t src_pitch_px,
                          int32_t x, int32_t y, uint32_t w, uint32_t h,
                          const region_t *clip);

#endif /* INITECH_OS_FLAIR_BLITTER_H */
