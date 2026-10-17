/*
 * os/flair/finder_icon.c -- the Finder icon blitter (THE ARTIFACT).
 *
 * beads: initech-tdnl.9 (R3 desktop icon blit), initech-tdnl.53 (the v2
 *        Platinum 4bpp strikes + the measured selection function),
 *        initech-tdnl.87 (the Dialog Manager's iconItem draws through here).
 * Ref:   os/flair/finder_icon.h (the contract and the three rules);
 *        spec/assets/desk_icons.h (the LOCKED v2 strike format, the DESK_IC_*
 *        colour ordinals and DESK_IC_SELECTED -- mint-008 Sec 3);
 *        os/flair/flair_look.h FLAIR_PART_ICON_CLUT (the ONE policy seam --
 *        C-8: this TU names a colour ORDINAL, never a colour or an index);
 *        os/flair/cursor.c (the per-pixel composite precedent).
 *
 * Artifact C per ADR-0002: freestanding, no allocation, no libc, deterministic,
 * ASCII-only. The mutation knobs are host-oracle builds only (Rule 6) and are
 * never compiled into a real build.
 */
#include "finder_icon.h"
#include "finder_small_icons.h" /* native16 capture cells, mint-008 Sec 6 */

#include "flair_look.h"     /* flair_look_pixel_depth -- the ONE color seam */

/* One policy role per non-clear colour ordinal, in ordinal order. */
_Static_assert((int)FLAIR_PART_ICON_CLUT_LAST - (int)FLAIR_PART_ICON_CLUT + 1 ==
               (int)DESK_IC__COUNT - 1,
               "one FLAIR_PART_ICON_CLUT role per DESK_IC_* colour (desk_icons.h)");
_Static_assert(FINDER_SMALL_COLORS >= DESK_ICON_TONES,
               "shared tone table must fit both icon cell sizes");

/* The 4-bit nibble of strike pixel (r,c); 0 is clear.
 *
 * Rule-6 knob DESK_ICON_MUT_MASK_IGNORED: a clear nibble is painted as nibble
 * 1, so the transparent cells get painted and the oracles' sentinel /
 * background checks must go RED. Rule-6 knob DESK_ICON_MUT_ROW_OFF1: read the
 * NEXT strike row, so the image shifts up by one and every cell oracle must
 * go RED. */
static uint32_t icon_nibble(const FLAIRDeskIcon *icon, int r, int c, int small)
{
    int dim = small >= 0 ? 16 : DESK_ICON_DIM;
#if defined(DESK_ICON_MUT_ROW_OFF1)
    int sr = (r + 1) % dim;
#else
    int sr = r;
    (void)dim;
#endif
    uint32_t n = small >= 0 ? finder_small_art[small][sr * 16 + c] :
        (icon->px[sr][c >> 3] >> (28 - 4 * (c & 7))) & 0xFu;
#if defined(DESK_ICON_MUT_MASK_IGNORED)
    if (n == 0u) n = 1u;
#endif
    return n;
}

static void icon_draw(const bitmap_t *dst, int16_t x, int16_t y,
                      const FLAIRDeskIcon *icon, const region_t *clip,
                      int hilite, int small)
{
    uint32_t tone_px[FINDER_SMALL_COLORS];
    uint8_t  tone_on[FINDER_SMALL_COLORS];
    int dim = small >= 0 ? 16 : DESK_ICON_DIM;
    int tones = small >= 0 ? FINDER_SMALL_COLORS : DESK_ICON_TONES;
    int r;
    int c;
    int n;

    /* Fail-soft on a NULL/zero destination or strike (Rule 2: never write past
     * a buffer we were not given). */
    if (dst == (const bitmap_t *)0 || dst->base == (volatile uint8_t *)0 ||
        dst->width == 0u || dst->height == 0u ||
        (icon == (const FLAIRDeskIcon *)0 && small < 0)) {
        return;
    }

    /* Resolve the strike's tones ONCE per blit at this drawing-context
     * boundary (the desktop.c desktop_px idiom), not once per pixel. A
     * SELECTED / drop-target draw first maps each colour through the
     * measured selection function (desk_icons.h DESK_IC_SELECTED). */
    for (n = 0; n < tones; n++) {
        tone_on[n] = 0u;
        tone_px[n] = 0u;
        /* Native16 art has its own captured tones, including #FFFF54, but
         * shares this tone table, opacity, clipping and destination walk.
         * Its selected state is unmeasured (mint-008 Sec 6). */
        if (small >= 0) {
            tone_on[n] = n != 0;
            tone_px[n] = flair_look_small_icon_pixel(dst->bpp, (unsigned)n);
            continue;
        }
        int k = (int)icon->tone[n];
        if (n == 0 || k <= (int)DESK_IC_CLEAR || k >= (int)DESK_IC__COUNT) continue;
#if !defined(DESK_ICON_MUT_NO_SELECT)
        if (hilite) k = (int)DESK_IC_SELECTED[k];
#endif
        tone_on[n] = 1u;
        tone_px[n] = flair_look_pixel_depth(dst->bpp,
                                            (int)FLAIR_PART_ICON_CLUT + k - 1);
    }
#if defined(DESK_ICON_MUT_NO_SELECT)
    (void)hilite;   /* MUTANT (Rule 6): selection draws the unselected art */
#endif

    for (r = 0; r < dim; r++) {
        int py = (int)y + r;
        if (py < 0 || py >= (int)dst->height) continue;
        for (c = 0; c < dim; c++) {
            int px = (int)x + c;
            uint32_t nib;
            uint32_t off;

            if (px < 0 || px >= (int)dst->width) continue;

            nib = icon_nibble(icon, r, c, small);
            if (!tone_on[nib]) continue;             /* transparency (rule 1) */

            if (clip != (const region_t *)0 &&
                !region_contains_point(clip, (int16_t)px, (int16_t)py)) {
                continue;                            /* the clip (rule 3)     */
            }

            off = (uint32_t)py * dst->pitch + (uint32_t)px * dst->bytes_per_pixel;
            surface_put_pixel(dst, off, tone_px[nib]);
        }
    }
}

void finder_icon_draw(const bitmap_t *dst, int16_t x, int16_t y,
                      const FLAIRDeskIcon *icon, const region_t *clip)
{
    icon_draw(dst, x, y, icon, clip, 0, -1);
}

void finder_icon_draw_hilite(const bitmap_t *dst, int16_t x, int16_t y,
                             const FLAIRDeskIcon *icon, const region_t *clip)
{
    icon_draw(dst, x, y, icon, clip, 1, -1);
}

void finder_icon_draw_small(const bitmap_t *dst, int16_t x, int16_t y,
                             unsigned kind, const region_t *clip)
{
    if (kind < 3)
        icon_draw(dst, x, y, (const FLAIRDeskIcon *)0, clip, 0, (int)kind);
}

int finder_icon_hit(int16_t x, int16_t y,
                    const FLAIRDeskIcon *icon, int16_t icon_x, int16_t icon_y)
{
    return desk_icon_colour(icon, (int)y - (int)icon_y, (int)x - (int)icon_x) !=
           (int)DESK_IC_CLEAR ? 1 : 0;
}
