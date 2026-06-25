/*
 * os/flair/flair_look.c -- the FLAIR policy seam resolver (THE ARTIFACT).
 *
 * beads: initech-6bq2 (C-8 MECHANISM/POLICY enforcement); epic initech-qipc.
 *
 * THE ONE POLICY SEAM (ADR-0004-AMENDMENT-DEC-09 Sec 3.1 C-8 / Sec 3.3 ARB-3).
 * This TU is the single place (besides the device CLUT, spec/assets/clut.h)
 * permitted to turn a palette index into a color.  flair_look_pixel maps a
 * wctb-keyed PART to a canon INDEX via the static const PART->idx map below
 * (PURE DATA), then resolves the color through flair_canon_rgb(idx) -- the ONE
 * locked color authority (spec/assets/color_canon.h) -- and device-quantizes
 * for the port depth, mirroring chrome.c chrome_px's existing 8bpp/32bpp logic.
 *
 * ARB-3 invariant: this resolver owns ZERO 0xRRGGBB hand-typed literal and
 * ZERO index->RGB switch.  The color VALUES all live in color_canon.h (the
 * canon authority); the only thing this TU owns is the PART->index map (pure
 * data).  The two derived bevel rows have no canon INDEX (flair_canon_rgb only
 * spans idx 0..8 + a gray ramp), so the map carries their canon-authored RGB
 * BY REFERENCE to the canon module's own named derived-row macros
 * (INITECH_CANON_BEVEL_*_RGB) -- the canon authority's constants, not a
 * hand-typed color and not a switch.
 *
 * Freestanding-safe (Law 3).  ASCII-clean (Rule 12).  Deterministic (Rule 11).
 *
 * Ref: ADR-0004-AMENDMENT-DEC-09 Sec 3.1 (C-8), Sec 3.3 (ARB-3), Sec 3.4
 *      (ARB-5 bevel teal), Sec 3.9 (the canon index table);
 *      spec/assets/color_canon.h (flair_canon_rgb, CIDX_*, bevel macros);
 *      os/flair/chrome.c chrome_px (the depth logic mirrored here);
 *      os/flair/surface.h (surface_pack_rgb -- the depth pack helper).
 */

#include <stdint.h>

#include "flair_look.h"
#include "surface.h"           /* surface_pack_rgb (-Ios/flair)                 */
#include "color_canon.h"       /* flair_canon_rgb + CIDX_* + bevel (-Ispec/assets) */

/* ---------------------------------------------------------------------------
 * The PART->index crosswalk (PURE DATA; ARB-3).  One row per FLAIR_PART, in
 * enum order.  `idx` is the canon index for the 9 indexed parts; `derived` is
 * 1 only for the two bevel rows (which have no canon index) and then
 * `derived_rgb` carries the canon module's own named derived-row constant.
 *
 * NO 0xRRGGBB literal appears here: indexed rows resolve through
 * flair_canon_rgb(idx); the two derived rows reference the canon authority's
 * INITECH_CANON_BEVEL_*_RGB macros (color_canon.h derived_rows; ARB-5).
 * ------------------------------------------------------------------------- */
typedef struct {
    uint8_t  idx;          /* canon index (valid when derived == 0)            */
    uint8_t  derived;      /* 1 -> use derived_rgb; 0 -> flair_canon_rgb(idx)  */
    uint32_t derived_rgb;  /* canon-authored 0x00RRGGBB (canon macro; ARB-5)   */
} flair_part_row_t;

static const flair_part_row_t flair_part_map[FLAIR_PART__COUNT] = {
    /* FLAIR_PART_CONTENT      */ { CIDX_WHITE,      0u, 0u },
    /* FLAIR_PART_FRAME        */ { CIDX_BLACK,      0u, 0u },
    /* FLAIR_PART_TEXT         */ { CIDX_TITLE_INK,  0u, 0u },
    /* FLAIR_PART_DESKTOP      */ { CIDX_DESKTOP,    0u, 0u },
    /* FLAIR_PART_MENUBAR      */ { CIDX_MENUBAR,    0u, 0u },
    /* FLAIR_PART_CAPTION_NAVY */ { CIDX_ACCENT,     0u, 0u },
    /* FLAIR_PART_BTNFACE      */ { CIDX_CONTROL,    0u, 0u },
    /* FLAIR_PART_PIN_LIGHT    */ { CIDX_PIN_LIGHT,  0u, 0u },
    /* FLAIR_PART_PIN_DARK     */ { CIDX_PIN_DARK,   0u, 0u },
    /* FLAIR_PART_BEVEL_LIGHT  */ { 0u, 1u, INITECH_CANON_BEVEL_LIGHT_RGB },
    /* FLAIR_PART_BEVEL_SHADOW */ { 0u, 1u, INITECH_CANON_BEVEL_SHADOW_RGB }
};

/* ---------------------------------------------------------------------------
 * resolve_rgb -- PART -> canonical 0x00RRGGBB through the ONE color authority.
 * The single index->color resolution: indexed parts via flair_canon_rgb;
 * the two derived bevel rows via the canon module's own constant.  No literal,
 * no switch (data-driven lookup).
 * ------------------------------------------------------------------------- */
static uint32_t resolve_rgb(int part)
{
    const flair_part_row_t *row;
    if (part < 0 || part >= (int)FLAIR_PART__COUNT) {
        /* Out-of-range PART: fail-soft to frame ink (CIDX_BLACK) rather than
         * read past the map (Rule 2 -- never index out of bounds). */
        return flair_canon_rgb((uint8_t)CIDX_BLACK);
    }
    row = &flair_part_map[part];
    if (row->derived) {
        return row->derived_rgb & 0x00FFFFFFu;
    }
    return flair_canon_rgb(row->idx);
}

/* ---------------------------------------------------------------------------
 * flair_look_pixel_depth -- the resolution CORE (bitmap-only seam).
 * 8bpp -> the palette index byte; else -> packed 0x00RRGGBB.  For the two
 * derived bevel rows there is no index, so the 8bpp path nearest-maps the
 * derived RGB to a device-CLUT index (the canonical 8bpp quantize); this path
 * is currently unexercised by decoration (no bevel draw ships), but is total.
 * ------------------------------------------------------------------------- */
uint32_t flair_look_pixel_depth(uint32_t bpp, int part)
{
    const flair_part_row_t *row =
        (part >= 0 && part < (int)FLAIR_PART__COUNT) ? &flair_part_map[part] : 0;

    if (bpp == 8u) {
        /* 8bpp destination writes the palette index low byte (OD-2). The 9
         * indexed parts ARE canon indices; the derived bevel rows have none, so
         * fall back to their light/shadow index neighbour where exact (bevel
         * light == idx2 teal) and CIDX_TITLE_INK otherwise.  The 8bpp bevel
         * path IS now exercised: the close/zoom box gadget (chrome.c cbox,
         * beads initech-ts3t) draws BEVEL_LIGHT (-> idx2 teal) + BEVEL_SHADOW
         * (-> idx4) for its 3-D double bevel; the title bevel (initech-92li)
         * will too. */
        if (row == 0) {
            return (uint32_t)CIDX_BLACK;
        }
        if (!row->derived) {
            return (uint32_t)row->idx;
        }
        /* derived: bevel_light renders identically to idx2 teal (same value). */
        return (part == FLAIR_PART_BEVEL_LIGHT)
                   ? (uint32_t)CIDX_DESKTOP
                   : (uint32_t)CIDX_TITLE_INK;
    }

    return surface_pack_rgb(bpp, 0, 0, 0) | resolve_rgb(part);
}

/* ---------------------------------------------------------------------------
 * flair_look_pixel -- the GrafPort-keyed policy seam (ARB-3).  Reads the
 * destination depth from the port and delegates to the resolution core.
 * ------------------------------------------------------------------------- */
uint32_t flair_look_pixel(const GrafPort *port, int part)
{
    uint32_t bpp = (port != 0) ? port->portBits.bm.bpp : 32u;
    return flair_look_pixel_depth(bpp, part);
}
