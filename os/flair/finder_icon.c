/*
 * os/flair/finder_icon.c -- the Finder desktop-icon blitter (THE ARTIFACT).
 *
 * beads: initech-tdnl.9 (R3 "STAPLER-DESK": desktop icon assets + their blit).
 * Ref:   os/flair/finder_icon.h (the contract and the three rules);
 *        spec/assets/desk_icons.h (the LOCKED strike format + tone decode);
 *        os/flair/flair_look.h (the ONE policy seam -- C-8);
 *        os/flair/cursor.c (the per-pixel CURS composite precedent);
 *        os/flair/desktop.c :: desktop_px (the PART->pixel idiom mirrored here).
 *
 * Artifact C per ADR-0002: freestanding, no allocation, no libc, deterministic,
 * ASCII-only. The two mutation knobs are host-oracle builds only (Rule 6) and
 * are never compiled into a real build.
 */
#include "finder_icon.h"

#include "flair_look.h"     /* flair_look_pixel_depth -- the ONE color seam */

/* Column bit for col c: MSB (bit 31) is col 0 (the cursors.h convention). */
static uint32_t icon_bit(int c)
{
    return (uint32_t)1u << (31 - c);
}

/* Decode the tone of strike pixel (r,c) from the three planes.
 *
 * Rule-6 knob DESK_ICON_MUT_MASK_IGNORED: treat every pixel as opaque, so the
 * transparent cells get painted and the oracle's sentinel-survival check must
 * go RED. Rule-6 knob DESK_ICON_MUT_ROW_OFF1: read the NEXT strike row, so the
 * image shifts up by one and the probe pixels must go RED. */
static desk_tone_t icon_tone(const FLAIRDeskIcon *icon, int r, int c)
{
    uint32_t b = icon_bit(c);
    uint32_t mask;
    uint32_t ink;
    uint32_t shade;
#if defined(DESK_ICON_MUT_ROW_OFF1)
    int sr = (r + 1) % DESK_ICON_DIM;
#else
    int sr = r;
#endif

    mask  = icon->mask[sr];
    ink   = icon->ink[sr];
    shade = icon->shade[sr];

#if defined(DESK_ICON_MUT_MASK_IGNORED)
    mask = 0xFFFFFFFFu;
#endif

    if ((mask & b) == 0u) return DESK_TONE_CLEAR;
    if ((ink & b) != 0u)  return DESK_TONE_INK;
    if ((shade & b) != 0u) return DESK_TONE_SHADE;
    return DESK_TONE_FACE;
}

/* Tone -> the SEMANTIC role the policy seam resolves. No color is named here
 * and no palette index is named here (C-8). */
static int icon_tone_part(desk_tone_t tone)
{
    if (tone == DESK_TONE_INK)   return (int)FLAIR_PART_ICON_INK;
    if (tone == DESK_TONE_SHADE) return (int)FLAIR_PART_ICON_SHADE;
    return (int)FLAIR_PART_ICON_FACE;
}

void finder_icon_draw(const bitmap_t *dst, int16_t x, int16_t y,
                      const FLAIRDeskIcon *icon, const region_t *clip)
{
    uint32_t tone_px[4];
    int r;
    int c;

    /* Fail-soft on a NULL/zero destination or strike (Rule 2: never write past
     * a buffer we were not given). */
    if (dst == (const bitmap_t *)0 || dst->base == (volatile uint8_t *)0 ||
        dst->width == 0u || dst->height == 0u ||
        icon == (const FLAIRDeskIcon *)0) {
        return;
    }

    /* Resolve the three tones ONCE per blit at this drawing-context boundary
     * (the desktop.c desktop_px idiom), not once per pixel. The tone enum
     * values double as the index (CLEAR=0 is never drawn). */
    tone_px[DESK_TONE_CLEAR] = 0u;
    tone_px[DESK_TONE_INK]   = flair_look_pixel_depth(dst->bpp,
                                                      icon_tone_part(DESK_TONE_INK));
    tone_px[DESK_TONE_FACE]  = flair_look_pixel_depth(dst->bpp,
                                                      icon_tone_part(DESK_TONE_FACE));
    tone_px[DESK_TONE_SHADE] = flair_look_pixel_depth(dst->bpp,
                                                      icon_tone_part(DESK_TONE_SHADE));

    for (r = 0; r < DESK_ICON_DIM; r++) {
        int py = (int)y + r;
        if (py < 0 || py >= (int)dst->height) continue;
        for (c = 0; c < DESK_ICON_DIM; c++) {
            int px = (int)x + c;
            desk_tone_t tone;
            uint32_t off;

            if (px < 0 || px >= (int)dst->width) continue;

            tone = icon_tone(icon, r, c);
            if (tone == DESK_TONE_CLEAR) continue;   /* transparency (rule 1) */

            if (clip != (const region_t *)0 &&
                !region_contains_point(clip, (int16_t)px, (int16_t)py)) {
                continue;                            /* the clip (rule 3)     */
            }

            off = (uint32_t)py * dst->pitch + (uint32_t)px * dst->bytes_per_pixel;
            surface_put_pixel(dst, off, tone_px[tone]);
        }
    }
}

int finder_icon_hit(int16_t x, int16_t y,
                    const FLAIRDeskIcon *icon, int16_t icon_x, int16_t icon_y)
{
    int r = (int)y - (int)icon_y;
    int c = (int)x - (int)icon_x;

    if (icon == (const FLAIRDeskIcon *)0) return 0;
    if (r < 0 || r >= DESK_ICON_DIM || c < 0 || c >= DESK_ICON_DIM) return 0;
    return (icon->mask[r] & icon_bit(c)) != 0u ? 1 : 0;
}
