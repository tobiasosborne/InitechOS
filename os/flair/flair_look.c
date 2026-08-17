/*
 * os/flair/flair_look.c -- the FLAIR policy seam resolver (THE ARTIFACT).
 *
 * beads: initech-6bq2 (C-8 MECHANISM/POLICY enforcement); epic initech-qipc.
 *
 * THE ONE POLICY SEAM (ADR-0004-AMENDMENT-DEC-09 Sec 3.1 C-8 / Sec 3.3 ARB-3).
 * This TU is the single place (besides the device CLUT, spec/assets/clut.h)
 * permitted to turn a palette index or skin slot into a color. The base path
 * maps a wctb-keyed PART to a canon INDEX through pure data. The D-9 chrome
 * path overlays offsets for same-valued flair_skin_t slots, so era arrives as
 * a const data pointer and never as switch(era) in the mechanism. Both paths
 * device-quantize here through the ONE locked canon authority.
 *
 * ARB-3 invariant: this resolver owns ZERO 0xRRGGBB hand-typed literal and
 * ZERO index->RGB switch.  The color VALUES all live in color_canon.h (the
 * canon authority); the only thing this TU owns is the PART->index/slot map
 * (pure data). The two derived bevel rows have no canon INDEX (flair_canon_rgb only
 * spans idx 0..8 + a gray ramp), so the map carries their canon-authored RGB
 * BY REFERENCE to the canon module's own named derived-row macros
 * (INITECH_CANON_BEVEL_*_RGB) -- the canon authority's constants, not a
 * hand-typed color and not a switch.
 *
 * FLAIR_PART_HILITE_FRAME / FLAIR_PART_HILITE_TEXT (beads initech-hv7u): the
 * inactive-window StandardWDEF wHiliteShadeA/wHiliteShade7 grays. These are
 * ORDINARY indexed rows (derived=0), NOT the derived-bevel special case --
 * their canon index (CIDX_HILITE_FRAME=119 / CIDX_HILITE_TEXT=165) is a named
 * idx>=9 gray-ramp slot (color_canon.h; color_canon.json "ramp_named_indices"),
 * so flair_canon_rgb(idx) already returns the exact cited #777777/#A5A5A5 with
 * zero special-casing here.
 *

 * Freestanding-safe (Law 3).  ASCII-clean (Rule 12).  Deterministic (Rule 11).
 *
 * Ref: ADR-0004-AMENDMENT-DEC-09 Sec 3.1 (C-8), Sec 3.3 (ARB-3), Sec 3.4
 *      (ARB-5 bevel teal), Sec 3.9 (the canon index table);
 *      spec/assets/color_canon.h (flair_canon_rgb, CIDX_*, bevel macros);
 *      os/flair/chrome.c chrome_px (the depth logic mirrored here);
 *      os/flair/surface.h (surface_pack_rgb -- the depth pack helper).
 */

#include <stddef.h>
#include <stdint.h>

#include "flair_look.h"
#include "flair_skins.h"       /* complete flair_skin_t + registry resolver     */
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
    uint8_t  skin_offset;  /* flair_skin_slot_t offset; 0xFF keeps base row    */
    uint32_t derived_rgb;  /* canon-authored 0x00RRGGBB (canon macro; ARB-5)   */
} flair_part_row_t;

#define FLAIR_NO_SKIN_SLOT       ((uint8_t)0xFFu)
#define FLAIR_SKIN_SLOT(member)  ((uint8_t)offsetof(flair_skin_t, member))

static const flair_part_row_t flair_part_map[FLAIR_PART__COUNT] = {
    /* FLAIR_PART_CONTENT      */ { CIDX_WHITE, 0u,
                                     FLAIR_SKIN_SLOT(content), 0u },
    /* FLAIR_PART_FRAME        */ { CIDX_BLACK, 0u,
                                     FLAIR_SKIN_SLOT(frame_ink), 0u },
    /* FLAIR_PART_TEXT         */ { CIDX_TITLE_INK, 0u,
                                     FLAIR_SKIN_SLOT(caption_ink), 0u },
    /* FLAIR_PART_DESKTOP      */ { CIDX_DESKTOP, 0u,
                                     FLAIR_SKIN_SLOT(desktop), 0u },
    /* FLAIR_PART_MENUBAR      */ { CIDX_MENUBAR, 0u,
                                     FLAIR_NO_SKIN_SLOT, 0u },
    /* FLAIR_PART_CAPTION_NAVY */ { CIDX_ACCENT, 0u,
                                     FLAIR_SKIN_SLOT(caption_navy), 0u },
    /* FLAIR_PART_BTNFACE      */ { CIDX_CONTROL, 0u,
                                     FLAIR_NO_SKIN_SLOT, 0u },
    /* FLAIR_PART_PIN_LIGHT    */ { CIDX_PIN_LIGHT, 0u,
                                     FLAIR_NO_SKIN_SLOT, 0u },
    /* FLAIR_PART_PIN_DARK     */ { CIDX_PIN_DARK, 0u,
                                     FLAIR_NO_SKIN_SLOT, 0u },
    /* FLAIR_PART_BEVEL_LIGHT  */ { 0u, 1u, FLAIR_NO_SKIN_SLOT,
                                     INITECH_CANON_BEVEL_LIGHT_RGB },
    /* FLAIR_PART_BEVEL_SHADOW */ { 0u, 1u, FLAIR_NO_SKIN_SLOT,
                                     INITECH_CANON_BEVEL_SHADOW_RGB },
    /* FLAIR_PART_HILITE_FRAME */ { CIDX_HILITE_FRAME, 0u,
                                     FLAIR_NO_SKIN_SLOT, 0u },
    /* FLAIR_PART_HILITE_TEXT  */ { CIDX_HILITE_TEXT, 0u,
                                     FLAIR_NO_SKIN_SLOT, 0u },
    /* Platinum sampled mappings; every row is a named canon index from
     * platinum-palette.md Sec 2. */
    /* STRIPE_DARK: window-chrome.md Sec 2.2. */
    { CIDX_PLAT_STRIPE_DARK, 0u, FLAIR_NO_SKIN_SLOT, 0u },
    /* FRAME_FACE: window-chrome.md Sec 2.1, Sec 4, Sec 5. */
    { CIDX_PLAT_FRAME_FACE, 0u, FLAIR_NO_SKIN_SLOT, 0u },
    /* FACE: window-chrome.md Sec 6; scrollbars.md Sec 2.2. */
    { CIDX_PLAT_FACE, 0u, FLAIR_SKIN_SLOT(btnface), 0u },
    /* FRAME_SHADOW: window-chrome.md Sec 2.1 and Sec 4. */
    { CIDX_PLAT_FRAME_SHADOW, 0u, FLAIR_NO_SKIN_SLOT, 0u },
    /* WIDGET_EDGE: window-chrome.md Sec 3.2; scrollbars.md Sec 3. */
    { CIDX_PLAT_WIDGET_EDGE, 0u, FLAIR_SKIN_SLOT(btnshadow), 0u },
    /* DARK_RING: window-chrome.md Sec 3.2 and Sec 3.3. */
    { CIDX_PLAT_DARK_RING, 0u, FLAIR_NO_SKIN_SLOT, 0u },
    /* INACTIVE_FRAME: window-chrome.md Sec 1 and Sec 6; scrollbars.md Sec 4. */
    { CIDX_PLAT_INACTIVE_FRAME, 0u, FLAIR_NO_SKIN_SLOT, 0u },
    /* INACTIVE_TEXT: window-chrome.md Sec 6. */
    { CIDX_PLAT_INACTIVE_TEXT, 0u, FLAIR_NO_SKIN_SLOT, 0u },
    /* TROUGH: scrollbars.md Sec 3 and Sec 4. */
    { CIDX_PLAT_TROUGH, 0u, FLAIR_NO_SKIN_SLOT, 0u },
    /* WELL: scrollbars.md Sec 2.3; window-chrome.md Sec 4 and Sec 5. */
    { CIDX_PLAT_WELL, 0u, FLAIR_NO_SKIN_SLOT, 0u },
    /* TILE_SHADOW: scrollbars.md Sec 2.2. */
    { CIDX_PLAT_TILE_SHADOW, 0u, FLAIR_NO_SKIN_SLOT, 0u }
};

_Static_assert(sizeof(flair_skin_t) < 255u,
               "flair skin slot offsets fit the policy map");

/* Resolve the live default at a drawing-context boundary. The Rule-6 mutant
 * changes only this policy choice; chrome geometry and palette mechanisms are
 * untouched. There is deliberately no runtime era branch. */
const flair_skin_t *flair_look_default_skin(void)
{
#if defined(FLAIR_MUT_SKIN_WRONG_ERA)
    return flair_skin_resolve(ERA_SYS7_0_1, HERITAGE_QUICKDRAW);
#else
    return flair_skin_default();
#endif
}

/* Return the row-carried slot for a PART whose default Platinum value already
 * equals the old base-map value. Other PARTs remain palette indices: the
 * registry has no matching slot for the Platinum ramp, while MENUBAR,
 * BTNFACE, PIN_LIGHT, and PIN_DARK intentionally describe different roles or
 * values and therefore cannot be silently re-keyed in this look-neutral lane. */
static const flair_skin_slot_t *skin_slot_for_part(const flair_skin_t *skin,
                                                   int part)
{
    const flair_part_row_t *row;
    const uint8_t *base;

    if (skin == (const flair_skin_t *)0) {
        FLAIR_SKIN_FAIL_LOUD();
    }
    if (part < 0 || part >= (int)FLAIR_PART__COUNT) {
        return (const flair_skin_slot_t *)0;
    }
    row = &flair_part_map[part];
    if (row->skin_offset == FLAIR_NO_SKIN_SLOT) {
        return (const flair_skin_slot_t *)0;
    }
    base = (const uint8_t *)(const void *)skin;
    return (const flair_skin_slot_t *)(const void *)(base + row->skin_offset);
}

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

/* Explicit D-9 seam: the caller supplies the selected data row once, and the
 * policy layer reads a named slot when the row carries this exact role. The
 * mechanism still supplies only PART names and never branches on era. */
uint32_t flair_look_pixel_for_skin(const GrafPort *port,
                                   const flair_skin_t *skin, int part)
{
    const flair_skin_slot_t *slot = skin_slot_for_part(skin, part);
    uint32_t bpp = (port != 0) ? port->portBits.bm.bpp : 32u;

    if (slot == (const flair_skin_slot_t *)0) {
        return flair_look_pixel_depth(bpp, part);
    }
    if (bpp == 8u) {
        return (uint32_t)slot->idx;
    }
    return surface_pack_rgb(bpp, 0, 0, 0) | (slot->rgb & 0x00FFFFFFu);
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
