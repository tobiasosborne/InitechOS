/*
 * os/flair/blitter.c -- the FLAIR region-clipped blitter (THE ARTIFACT).
 *
 * beads: initech-i50. See blitter.h for the full contract + Law-3 separation.
 *
 * Freestanding artifact code: a thin LAYER OVER the ONE surface module
 * (os/flair/surface.h). It computes WHICH pixels are inside the effective clip
 * region (visRgn INTERSECT clipRgn the Manager passes) and writes ONLY those,
 * exclusively via surface_fill_span / surface_put_pixel -- never base[] directly
 * (ADR-0004 D-2 / C-2: no second pixel path).
 *
 * THE CLIP RULE (ADR-0004 D-1/D-2; grafport.h): write a pixel IFF it is BOTH in
 * the blit/fill rect AND inside the clip region; clip == NULL means no clip.
 *
 * THE SPAN WALK (efficiency, PRD Sec 6.2): a region row is a sorted inversion
 * list -- the row covering scanline y holds half-open "inside" spans
 * [x_2k, x_2k+1). To fill a rect row clipped to a region we INTERSECT the rect's
 * column interval [rect.left, rect.right) with each clip span and emit the
 * overlap as one surface_fill_span. This is the SAME pixel set a per-pixel
 * region_contains_point loop would write (the oracle proves it bit-exact), but
 * it pays O(spans) per scanline, not O(width). This generalizes chrome.c's
 * ad-hoc per-pixel clip_in()/cfill() into one reusable primitive.
 *
 * MUTATION HOOKS (Rule 6; mirroring chrome.c CHROME_MUTATE_* / region.c
 * RGN_MUTATE_*): two named mutants compiled via -D, each a single deliberate
 * clip defect test-blitter must catch:
 *   BLITTER_MUTATE_IGNORE_CLIP -- treat every clip as "no clip" (draw the whole
 *                                 rect regardless of the clip); the load-bearing
 *                                 clip rule is violated outright.
 *   BLITTER_MUTATE_OFF_BY_ONE  -- the clip span's right edge is treated as
 *                                 inclusive (x < x1+1 instead of x < x1), so one
 *                                 extra column past each clip span is written.
 * In a normal build neither is defined and the clip is exact. The IGNORE_CLIP
 * mutant is applied by forcing `clip = 0` at the top of each entry point, so the
 * real clipped code path stays compiled (no -Werror unused-function break).
 *
 * Ref: spec/region_algebra.h (region_t rep + region_contains_point); surface.h
 *      (the ONE writer); chrome.c (the generalized-from source). CLAUDE.md
 *      Law 1/2/3, Rule 2/3/6/11/12.
 */

#include <stdint.h>

#include "blitter.h"
#include "surface.h"
#include "region_algebra.h"

/* ---------------------------------------------------------------------------
 * clip_row_for_y -- return the index of the region row covering scanline y, or
 * -1 if y is outside the region's vertical extent (above the first row or at/
 * below the closing row). A row r is valid for [rows[r].y_top, rows[r+1].y_top);
 * the closing row (last) is the lower bound and covers no scanline. Region row
 * counts are small (RGN_ROWS_CAP), so a linear scan is fine and branch-simple;
 * it mirrors region_contains_point's row-find so the span walk paints exactly
 * the membership set.
 * ------------------------------------------------------------------------- */
static int clip_row_for_y(const region_t *clip, int y)
{
    if (clip->is_empty || clip->n_rows == 0) {
        return -1;
    }
    if (y < clip->rows[0].y_top) {
        return -1;                              /* above the first live band   */
    }
    for (uint16_t i = 0; i < clip->n_rows; i++) {
        int y0 = clip->rows[i].y_top;
        int y1 = (i + 1 < clip->n_rows) ? clip->rows[i + 1].y_top : y0;
        /* The closing row (i == n_rows-1) has y1 == y0 => covers nothing. */
        if (y >= y0 && y < y1) {
            return (int)i;
        }
    }
    return -1;
}

/* ---------------------------------------------------------------------------
 * apply_ignore_clip_mutant -- under BLITTER_MUTATE_IGNORE_CLIP the clip is
 * dropped (treated as "no clip"); otherwise the clip is returned unchanged.
 * Keeping this a function (not a per-callsite #if) leaves the real clipped path
 * compiled in every build, so clip_row_for_y is always referenced (Rule 12 /
 * -Werror clean) and the mutant is a single deliberate defect (Rule 6).
 * ------------------------------------------------------------------------- */
static const region_t *effective_clip(const region_t *clip)
{
#if defined(BLITTER_MUTATE_IGNORE_CLIP)
    (void)clip;
    return 0;                                   /* MUTANT: clip ignored        */
#else
    return clip;
#endif
}

/* ---------------------------------------------------------------------------
 * fill_clipped_run -- fill the half-open destination column interval
 * [col_lo, col_hi) on scanline y with packed color `px`, clipped to `clip`.
 *
 *   clip == NULL -> the whole interval (no clip / clip == the universe).
 *   clip != NULL -> the interval INTERSECT the clip row's inside-spans.
 *
 * Writes each maximal in-clip run as ONE surface_fill_span (the surface module
 * clips that span to the bitmap bounds, Rule 2). Negative columns are clamped to
 * 0 (surface_fill_span takes uint32_t and itself rejects x >= width).
 * ------------------------------------------------------------------------- */
static void fill_clipped_run(const bitmap_t *dst, int y,
                             int col_lo, int col_hi,
                             uint32_t px, const region_t *clip)
{
    if (col_hi <= col_lo || y < 0) {
        return;
    }

    if (clip == 0) {
        int lo = col_lo < 0 ? 0 : col_lo;
        if (col_hi > lo) {
            surface_fill_span(dst, (uint32_t)lo, (uint32_t)y,
                              (uint32_t)(col_hi - lo), px);
        }
        return;
    }

    int row = clip_row_for_y(clip, y);
    if (row < 0) {
        return;                                 /* scanline outside the region */
    }
    const rgn_row_t *r = &clip->rows[row];
    /* Walk the inside-spans [x[2k], x[2k+1)); intersect each with [col_lo,col_hi). */
    for (uint16_t k = 0; k + 1 < r->x_count; k += 2) {
        int sx0 = r->x[k];
        int sx1 = r->x[k + 1];
#if defined(BLITTER_MUTATE_OFF_BY_ONE)
        sx1 = sx1 + 1;                           /* MUTANT: right edge inclusive */
#endif
        int lo = sx0 > col_lo ? sx0 : col_lo;   /* max(span.lo, interval.lo)   */
        int hi = sx1 < col_hi ? sx1 : col_hi;   /* min(span.hi, interval.hi)   */
        if (lo < 0) {
            lo = 0;
        }
        if (hi > lo) {
            surface_fill_span(dst, (uint32_t)lo, (uint32_t)y,
                              (uint32_t)(hi - lo), px);
        }
    }
}

/* ---- blitter_fill_rect_clipped ------------------------------------------- */
void blitter_fill_rect_clipped(const bitmap_t *dst, rgn_rect_t rect,
                               uint32_t px, const region_t *clip)
{
    if (dst == 0 || dst->base == 0) {
        return;
    }
    int top    = rect.top;
    int bottom = rect.bottom;
    int left   = rect.left;
    int right  = rect.right;
    if (right <= left || bottom <= top) {
        return;                                 /* empty rect (QuickDraw empty) */
    }
    const region_t *clp = effective_clip(clip);
    for (int y = top; y < bottom; y++) {
        fill_clipped_run(dst, y, left, right, px, clp);
    }
}

/* ---------------------------------------------------------------------------
 * copy_clipped_run -- copy source pixels into the half-open destination column
 * interval [col_lo, col_hi) on destination scanline dy, clipped to `clip`. Each
 * in-clip destination pixel (dx,dy) takes src[srow*src_pitch_px + (dx - x0)].
 * Per-pixel write (the source varies) but the IN-CLIP COLUMNS are still resolved
 * by the same span walk (no region query per pixel). Bounds-checked against the
 * dst width (Rule 2 -- never write past the buffer).
 * ------------------------------------------------------------------------- */
static void copy_clipped_run(const bitmap_t *dst, int dy,
                             int col_lo, int col_hi,
                             const uint32_t *src, uint32_t src_pitch_px,
                             uint32_t srow, int x0, const region_t *clip)
{
    if (col_hi <= col_lo || dy < 0 || (uint32_t)dy >= dst->height) {
        return;
    }

    if (clip == 0) {
        int lo = col_lo < 0 ? 0 : col_lo;
        for (int dx = lo; dx < col_hi; dx++) {
            if ((uint32_t)dx >= dst->width) {
                break;
            }
            uint32_t sx = (uint32_t)(dx - x0);
            uint32_t sp = src[srow * src_pitch_px + sx];
            uint32_t off = (uint32_t)dy * dst->pitch +
                           (uint32_t)dx * dst->bytes_per_pixel;
            surface_put_pixel(dst, off, sp);
        }
        return;
    }

    int row = clip_row_for_y(clip, dy);
    if (row < 0) {
        return;
    }
    const rgn_row_t *r = &clip->rows[row];
    for (uint16_t k = 0; k + 1 < r->x_count; k += 2) {
        int sx0 = r->x[k];
        int sx1 = r->x[k + 1];
#if defined(BLITTER_MUTATE_OFF_BY_ONE)
        sx1 = sx1 + 1;                           /* MUTANT: right edge inclusive */
#endif
        int lo = sx0 > col_lo ? sx0 : col_lo;
        int hi = sx1 < col_hi ? sx1 : col_hi;
        if (lo < 0) {
            lo = 0;
        }
        for (int dx = lo; dx < hi; dx++) {
            if ((uint32_t)dx >= dst->width) {
                break;
            }
            uint32_t sx = (uint32_t)(dx - x0);
            uint32_t sp = src[srow * src_pitch_px + sx];
            uint32_t off = (uint32_t)dy * dst->pitch +
                           (uint32_t)dx * dst->bytes_per_pixel;
            surface_put_pixel(dst, off, sp);
        }
    }
}

/* ---- blitter_blit_clipped ------------------------------------------------ */
void blitter_blit_clipped(const bitmap_t *dst,
                          const uint32_t *src, uint32_t src_pitch_px,
                          int32_t x, int32_t y, uint32_t w, uint32_t h,
                          const region_t *clip)
{
    if (dst == 0 || dst->base == 0 || src == 0 || w == 0 || h == 0) {
        return;
    }
    const region_t *clp = effective_clip(clip);
    int col_lo = (int)x;                        /* destination [x, x+w)        */
    int col_hi = (int)x + (int)w;
    for (uint32_t row = 0; row < h; row++) {
        int dy = (int)(y + (int32_t)row);
        copy_clipped_run(dst, dy, col_lo, col_hi,
                         src, src_pitch_px, row, (int)x, clp);
    }
}
