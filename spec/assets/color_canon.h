/*
 * color_canon.h -- GENERATED from spec/assets/color_canon.json
 *                   by tools/color_canon_extract.c
 *
 * DO NOT EDIT BY HAND.
 * Regenerate: tools/color_canon_extract spec/assets/color_canon.json > spec/assets/color_canon.h
 *
 * Canon version: WL-0053 (Initech Color Canon, operator-ratified 2026-06-21)
 *
 * THE single color-policy authority for FLAIR.  Exposes flair_canon_rgb(idx):
 * idx 0..8  -> the 9-entry color_canon table;
 * idx >= 9  -> deterministic gray ramp (v<<16)|(v<<8)|v  (v = idx),
 *              identical to palette.h flair_palette_rgb default branch
 *              (ADR-0004-AMENDMENT-DEC-09 ARB-1).
 * Freestanding-safe: <stdint.h> only, no libc.
 *
 * HONESTY (ADR-0004-AMENDMENT-DEC-09 FO-9 / P4):
 * idx2 (CIDX_DESKTOP teal) and both bevel derived rows are
 * graded_by:authored (Initech-identity injections; no upstream decomp
 * golden exists).  Gated against drift by VALUE mutants (seafoam/lavender
 * relapse MUST go RED).  Never claimed decomp-sourced.
 *
 * Ref: ADR-0004-AMENDMENT-DEC-09 Sec 3.2 (ARB-1) + Sec 3.9 (index table);
 *      spec/assets/color_canon.json (LOCKED, CLAUDE.md Rule 8);
 *      CLAUDE.md Rule 11 (deterministic), Rule 12 (ASCII-clean).
 */
#ifndef INITECH_COLOR_CANON_H
#define INITECH_COLOR_CANON_H

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * color_canon[idx][0..2] = {R, G, B}  --  9 entries, idx 0..8, index order.
 * Ref: ADR-0004-AMENDMENT-DEC-09 Sec 3.9; color_canon.json "entries".
 */
static const unsigned char color_canon[9][3] = {
    /* [0] CIDX_BLACK       */ {   0,   0,   0 },
    /* [1] CIDX_WHITE       */ { 255, 255, 255 },
    /* [2] CIDX_DESKTOP     */ { 141, 220, 220 },
    /* [3] CIDX_MENUBAR     */ { 255, 255, 255 },
    /* [4] CIDX_TITLE_INK   */ {   0,   0,   0 },
    /* [5] CIDX_ACCENT      */ {   0,   0, 128 },
    /* [6] CIDX_CONTROL     */ { 192, 192, 192 },
    /* [7] CIDX_PIN_LIGHT   */ { 243, 243, 243 },
    /* [8] CIDX_PIN_DARK    */ { 150, 150, 150 }
};
_Static_assert(sizeof(color_canon) == 27, "9 x 3 canon bytes");

/* ---------------------------------------------------------------------------
 * CIDX_* index constants -- name each slot so consumers can write
 * flair_canon_rgb(CIDX_BLACK) rather than flair_canon_rgb(0).
 */
#define CIDX_BLACK           0
#define CIDX_WHITE           1
#define CIDX_DESKTOP         2
#define CIDX_MENUBAR         3
#define CIDX_TITLE_INK       4
#define CIDX_ACCENT          5
#define CIDX_CONTROL         6
#define CIDX_PIN_LIGHT       7
#define CIDX_PIN_DARK        8

/* ---------------------------------------------------------------------------
 * INITECH_CANON_<SUFFIX>_RGB -- compile-time 0x00RRGGBB packed constant.
 * Convention: entry->name with leading "CIDX_" stripped becomes the SUFFIX
 * (e.g. CIDX_BLACK -> INITECH_CANON_BLACK_RGB).
 * Prefer flair_canon_rgb(CIDX_*) in runtime paths; use these macros only
 * where a compile-time constant is required.
 */
#define INITECH_CANON_BLACK_RGB              0x000000u
#define INITECH_CANON_WHITE_RGB              0xFFFFFFu
#define INITECH_CANON_DESKTOP_RGB            0x8DDCDCu
#define INITECH_CANON_MENUBAR_RGB            0xFFFFFFu
#define INITECH_CANON_TITLE_INK_RGB          0x000000u
#define INITECH_CANON_ACCENT_RGB             0x000080u
#define INITECH_CANON_CONTROL_RGB            0xC0C0C0u
#define INITECH_CANON_PIN_LIGHT_RGB          0xF3F3F3u
#define INITECH_CANON_PIN_DARK_RGB           0x969696u

/* ---------------------------------------------------------------------------
 * Derived bevel rows  (WL-0053 lavender->teal swap; graded_by:authored).
 * bevel_light  == idx2 teal #8DDCDC;  bevel_shadow == darkened teal #4E9BA3.
 * Neither is wired to a wctb decomp golden; both gated by VALUE mutants.
 * Ref: ADR-0004-AMENDMENT-DEC-09 Sec 3.4 (ARB-5); color_canon.json derived_rows.
 */
#define INITECH_CANON_BEVEL_LIGHT_RGB        0x8DDCDCu
#define INITECH_CANON_BEVEL_SHADOW_RGB       0x4E9BA3u

/* ---------------------------------------------------------------------------
 * Named ramp indices -- the idx>=9 gray-ramp EXTENSION (Rule 8; NOT a new
 * 9-entry table slot). Each constant below names an index >= 9 whose
 * (idx<<16)|(idx<<8)|idx value ALREADY equals the cited decomp golden RGB
 * (a neutral gray, R==G==B) via flair_canon_rgb's existing idx>=9 branch
 * below. Same idiom as spec/flair_skins.h FLAIR_GRAY_IDX_808080 /
 * FLAIR_CANON_GRAY_RGB (win31 btnshadow) -- promoted here so each carries a
 * versioned per-row decomp citation. See color_canon.json
 * "ramp_named_indices" for the source_golden/graded_by text per row.
 */
#define CIDX_HILITE_FRAME    119   /* #777777 via the idx>=9 ramp */
#define CIDX_HILITE_TEXT     165   /* #A5A5A5 via the idx>=9 ramp */
#define CIDX_PLAT_STRIPE_DARK 150   /* #969696 via the idx>=9 ramp */
#define CIDX_PLAT_FRAME_FACE 218   /* #DADADA via the idx>=9 ramp */
#define CIDX_PLAT_FACE       231   /* #E7E7E7 via the idx>=9 ramp */
#define CIDX_PLAT_FRAME_SHADOW 179   /* #B3B3B3 via the idx>=9 ramp */
#define CIDX_PLAT_WIDGET_EDGE 165   /* #A5A5A5 via the idx>=9 ramp */
#define CIDX_PLAT_DARK_RING  63   /* #3F3F3F via the idx>=9 ramp */
#define CIDX_PLAT_INACTIVE_FRAME 119   /* #777777 via the idx>=9 ramp */
#define CIDX_PLAT_INACTIVE_TEXT 135   /* #878787 via the idx>=9 ramp */
#define CIDX_PLAT_TROUGH     243   /* #F3F3F3 via the idx>=9 ramp */
#define CIDX_PLAT_WELL       192   /* #C0C0C0 via the idx>=9 ramp */
#define CIDX_PLAT_TILE_SHADOW 205   /* #CDCDCD via the idx>=9 ramp */

_Static_assert(((uint32_t)CIDX_HILITE_FRAME << 16 | (uint32_t)CIDX_HILITE_FRAME << 8 | (uint32_t)CIDX_HILITE_FRAME) == 0x777777u,
               "CIDX_HILITE_FRAME ramp-index formula == #777777 (color_canon.json ramp_named_indices)");
_Static_assert(((uint32_t)CIDX_HILITE_TEXT << 16 | (uint32_t)CIDX_HILITE_TEXT << 8 | (uint32_t)CIDX_HILITE_TEXT) == 0xA5A5A5u,
               "CIDX_HILITE_TEXT ramp-index formula == #A5A5A5 (color_canon.json ramp_named_indices)");
_Static_assert(((uint32_t)CIDX_PLAT_STRIPE_DARK << 16 | (uint32_t)CIDX_PLAT_STRIPE_DARK << 8 | (uint32_t)CIDX_PLAT_STRIPE_DARK) == 0x969696u,
               "CIDX_PLAT_STRIPE_DARK ramp-index formula == #969696 (color_canon.json ramp_named_indices)");
_Static_assert(((uint32_t)CIDX_PLAT_FRAME_FACE << 16 | (uint32_t)CIDX_PLAT_FRAME_FACE << 8 | (uint32_t)CIDX_PLAT_FRAME_FACE) == 0xDADADAu,
               "CIDX_PLAT_FRAME_FACE ramp-index formula == #DADADA (color_canon.json ramp_named_indices)");
_Static_assert(((uint32_t)CIDX_PLAT_FACE << 16 | (uint32_t)CIDX_PLAT_FACE << 8 | (uint32_t)CIDX_PLAT_FACE) == 0xE7E7E7u,
               "CIDX_PLAT_FACE ramp-index formula == #E7E7E7 (color_canon.json ramp_named_indices)");
_Static_assert(((uint32_t)CIDX_PLAT_FRAME_SHADOW << 16 | (uint32_t)CIDX_PLAT_FRAME_SHADOW << 8 | (uint32_t)CIDX_PLAT_FRAME_SHADOW) == 0xB3B3B3u,
               "CIDX_PLAT_FRAME_SHADOW ramp-index formula == #B3B3B3 (color_canon.json ramp_named_indices)");
_Static_assert(((uint32_t)CIDX_PLAT_WIDGET_EDGE << 16 | (uint32_t)CIDX_PLAT_WIDGET_EDGE << 8 | (uint32_t)CIDX_PLAT_WIDGET_EDGE) == 0xA5A5A5u,
               "CIDX_PLAT_WIDGET_EDGE ramp-index formula == #A5A5A5 (color_canon.json ramp_named_indices)");
_Static_assert(((uint32_t)CIDX_PLAT_DARK_RING << 16 | (uint32_t)CIDX_PLAT_DARK_RING << 8 | (uint32_t)CIDX_PLAT_DARK_RING) == 0x3F3F3Fu,
               "CIDX_PLAT_DARK_RING ramp-index formula == #3F3F3F (color_canon.json ramp_named_indices)");
_Static_assert(((uint32_t)CIDX_PLAT_INACTIVE_FRAME << 16 | (uint32_t)CIDX_PLAT_INACTIVE_FRAME << 8 | (uint32_t)CIDX_PLAT_INACTIVE_FRAME) == 0x777777u,
               "CIDX_PLAT_INACTIVE_FRAME ramp-index formula == #777777 (color_canon.json ramp_named_indices)");
_Static_assert(((uint32_t)CIDX_PLAT_INACTIVE_TEXT << 16 | (uint32_t)CIDX_PLAT_INACTIVE_TEXT << 8 | (uint32_t)CIDX_PLAT_INACTIVE_TEXT) == 0x878787u,
               "CIDX_PLAT_INACTIVE_TEXT ramp-index formula == #878787 (color_canon.json ramp_named_indices)");
_Static_assert(((uint32_t)CIDX_PLAT_TROUGH << 16 | (uint32_t)CIDX_PLAT_TROUGH << 8 | (uint32_t)CIDX_PLAT_TROUGH) == 0xF3F3F3u,
               "CIDX_PLAT_TROUGH ramp-index formula == #F3F3F3 (color_canon.json ramp_named_indices)");
_Static_assert(((uint32_t)CIDX_PLAT_WELL << 16 | (uint32_t)CIDX_PLAT_WELL << 8 | (uint32_t)CIDX_PLAT_WELL) == 0xC0C0C0u,
               "CIDX_PLAT_WELL ramp-index formula == #C0C0C0 (color_canon.json ramp_named_indices)");
_Static_assert(((uint32_t)CIDX_PLAT_TILE_SHADOW << 16 | (uint32_t)CIDX_PLAT_TILE_SHADOW << 8 | (uint32_t)CIDX_PLAT_TILE_SHADOW) == 0xCDCDCDu,
               "CIDX_PLAT_TILE_SHADOW ramp-index formula == #CDCDCD (color_canon.json ramp_named_indices)");

/* ---------------------------------------------------------------------------
 * wctb part<->index crosswalk (comments only; for consumers and the oracle).
 *
 * idx0  CIDX_BLACK      part1  wFrameColor         system7-quickdraw
 * idx1  CIDX_WHITE      part0  wContentColor       system7-quickdraw
 * idx2  CIDX_DESKTOP    none   no wctb part; Initech teal, graded:authored
 * idx3  CIDX_MENUBAR    part0  wContentColor (bar shares content white)
 * idx4  CIDX_TITLE_INK  part2  wTextColor          system7-quickdraw
 * idx5  CIDX_ACCENT     none   Win-3.1/GDI accent navy (graded LEG B)
 * idx6  CIDX_CONTROL    none   Win-3.1/GDI BTNFACE (graded LEG B)
 * idx7  CIDX_PIN_LIGHT  none   rendered dither shade -- NOT wctb part7
 *                              (#FFFFFF WDEF endpoint); graded vs pinstripe.md
 * idx8  CIDX_PIN_DARK   none   rendered dither shade -- NOT wctb part8
 *                              (#000000 WDEF endpoint); graded vs pinstripe.md
 *
 * Bevel derived rows substitute teal for System-7 dialog-lavender:
 * bevel_light  part9/part11  (#CCCCFF/#DADAFF teal-SUBSTITUTED) authored
 * bevel_shadow part9/part11  (#B3B3DA teal-SUBSTITUTED shadow)  authored
 */

/* ---------------------------------------------------------------------------
 * flair_canon_rgb(idx) -- single runtime index->0x00RRGGBB accessor.
 *
 * idx 0..8  -> color_canon table (the 9 indexed canon entries above).
 * idx >= 9  -> deterministic gray ramp: (v<<16)|(v<<8)|v  where v = idx.
 *              This is the IDENTICAL default branch from palette.h
 *              flair_palette_rgb, so idx>=9 behavior is byte-identical.
 *
 * Freestanding-safe (no libc).  Callable from the kernel (kmain.c live-DAC
 * path) and the host harness.  Ref: ADR-0004-AMENDMENT-DEC-09 ARB-1;
 * palette.h flair_palette_rgb default branch (gray ramp shared verbatim).
 */
static inline uint32_t flair_canon_rgb(uint8_t idx)
{
    if (idx < 9u) {
        return ((uint32_t)color_canon[idx][0] << 16)
             | ((uint32_t)color_canon[idx][1] <<  8)
             |  (uint32_t)color_canon[idx][2];
    }
    /* idx >= 9: deterministic gray ramp (identical to palette.h default). */
    {
        uint32_t v = (uint32_t)idx;
        return (v << 16) | (v << 8) | v;
    }
}

#endif /* INITECH_COLOR_CANON_H */
