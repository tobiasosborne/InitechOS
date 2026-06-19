/*
 * os/flair/text.h -- FLAIR text rendering API (proportional bitmap fonts).
 *
 * Ref: ADR-0004 D-7 ("proportional NFNT text measurement -- Chicago (system/
 *      dialog) and Geneva 9 (cell), hand-authored strikes; text width = sum
 *      of per-glyph advances; no fixed-pitch assumption");
 *      PRD Sec 6.4 (font resources); PRD Sec 6.3 (Toolbox layer).
 *      docs/research/gui-ground-truth.md Sec 3.5 (Chicago 12 and Geneva 9).
 *      spec/assets/chicago8x16.h (Chicago strike, fixed-cell v0).
 *      spec/assets/geneva9.h (Geneva 9 strike, proportional v0 -- this module).
 *      os/flair/surface.h (bitmap_t; surface_blit -- the ONE glyph-blit primitive).
 *      spec/grafport.h (GrafPort: txFont, txFace, txSize, txMode).
 *      CLAUDE.md Law 1 (ground truth before code), Law 3 (freestanding artifact;
 *      no libc), Rule 11 (reproducible), Rule 12 (ASCII-clean source).
 *
 * ARTIFACT CODE: freestanding C (ADR-0002). No libc. No malloc. No 2026-isms.
 * Dual-compile: kernel (gcc -m32 -ffreestanding -nostdlib -DFLAIR_HOSTED=0)
 *               hosted  (gcc -DFLAIR_HOSTED=1) for the oracle harness.
 *
 * BUILD NOTE: This header includes surface.h via the name "surface.h".
 *   The calling build must place os/flair/ on the include path so that
 *   surface.h (which defines bitmap_t) is found. Both the kernel build
 *   (-Ios/flair) and the oracle build (-Ios/flair from the repo root)
 *   satisfy this requirement.
 *
 * API CONTRACT (ADR-0004 D-7):
 *
 *   text_measure(font, str) -> int
 *     Returns the pixel width of 'str' rendered in 'font':
 *     width = SUM of per-glyph advance widths (proportional NFNT; no
 *     fixed-pitch assumption). Returns 0 for NULL or empty string.
 *
 *   text_draw(bm, x, y, str, font, fg, bg)
 *     Renders each glyph of 'str' into bitmap *bm via surface_blit at
 *     running x += advance. 'y' is the top of the cell (row 0). Clipping
 *     is delegated to surface_blit (bounds-safe). 'fg'/'bg' are packed
 *     0x00RRGGBB colors (surface.h canonical format).
 *
 *   text_center_in(rect_w, str, font) -> int
 *     Returns the x offset within a rectangle of width rect_w at which
 *     to start rendering 'str' so that it is centered (for title-bar
 *     chrome and dialog label centering; ADR-0004 D-7 chrome).
 *     x = (rect_w - text_measure(font, str)) / 2  (integer division).
 *     If the string is wider than rect_w, returns 0 (left-justified fallback).
 *
 * FONT SELECTION:
 *   text_font_t selects Chicago 8x16 (FONT_CHICAGO) or Geneva 9
 *   (FONT_GENEVA9). Both are backed by hand-authored clean-room strikes
 *   (spec/assets/chicago8x16.h and spec/assets/geneva9.h respectively).
 *
 *   txFont in GrafPort (spec/grafport.h) maps to:
 *     0 (systemFont) -> Chicago  (ADR-0004 D-7 "Chicago (system/dialog)")
 *     1 (applFont)   -> Geneva 9 (ADR-0004 D-7 "Geneva 9 (cell)")
 *   The text_font_from_txfont() helper performs this mapping (text.c).
 *
 * ASCII-clean (Rule 12). No nondeterminism / no timestamps (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_TEXT_H
#define INITECH_OS_FLAIR_TEXT_H

#include <stdint.h>
#include <stddef.h>

/* Pull in the ONE pixel-buffer type and surface_blit declaration.
 * Ref: os/flair/surface.h (ADR-0004 D-2; bitmap_t; surface_blit). */
#include "surface.h"      /* bitmap_t, surface_blit (found via -Ios/flair) */

/* Pull in Chicago and Geneva strikes (header-only static data).
 * Ref: spec/assets/chicago8x16.h, spec/assets/geneva9.h (found via -Ispec/assets). */
#include "chicago8x16.h"  /* chicago8x16_glyph(), CHICAGO_CELL_W, CHICAGO_CELL_H */
#include "geneva9.h"      /* geneva9_glyph(), geneva9_advance_w(), GENEVA9_CELL_H */

/* --------------------------------------------------------------------------
 * text_font_t -- font selector (ADR-0004 D-7).
 *
 * Maps to the GrafPort txFont values:
 *   0 (systemFont) -> FONT_CHICAGO
 *   1 (applFont)   -> FONT_GENEVA9
 * -------------------------------------------------------------------------- */
typedef enum text_font {
    FONT_CHICAGO  = 0,  /* Chicago 8x16 (fixed cell v0); system/dialog font  */
    FONT_GENEVA9  = 1   /* Geneva 9 (proportional cell v0); small UI font     */
} text_font_t;

/* --------------------------------------------------------------------------
 * text_cell_height -- return the cell height for a given font (pixels).
 *
 * For use by callers that need to position the next line below a text row.
 * -------------------------------------------------------------------------- */
static inline int text_cell_height(text_font_t font)
{
    if (font == FONT_CHICAGO)
        return (int)CHICAGO_CELL_H;
    return (int)GENEVA9_CELL_H;
}

/* --------------------------------------------------------------------------
 * text_measure -- pixel width of string 'str' in the given font.
 *
 * ADR-0004 D-7: "text width = sum of per-glyph advances; no fixed-pitch
 * assumption." Returns 0 for NULL or empty str.
 *
 * For Chicago (fixed-cell v0): advance is CHICAGO_CELL_W for every glyph.
 * For Geneva 9 (proportional): advance is per-glyph from geneva9_advance[].
 *
 * TEXT_MUTATE_FIXED_PITCH (mutation oracle -- Rule 6):
 *   When compiled with -DTEXT_MUTATE_FIXED_PITCH=1, text_measure uses a
 *   fixed advance of 6 px for EVERY glyph of EVERY font, ignoring the
 *   per-glyph advance table. This mutant MUST make the proportional
 *   property tests RED, proving the oracle catches fixed-pitch drift.
 * -------------------------------------------------------------------------- */
static inline int text_measure(text_font_t font, const char *str)
{
    int width = 0;
    if (!str)
        return 0;
    while (*str) {
        int c = (unsigned char)*str;
#if defined(TEXT_MUTATE_FIXED_PITCH) && TEXT_MUTATE_FIXED_PITCH
        /* NAMED MUTANT: ignores per-glyph advance; uses fixed 6px. */
        (void)font;
        (void)c;
        width += 6;
#else
        if (font == FONT_CHICAGO) {
            width += (int)CHICAGO_CELL_W;
        } else {
            width += (int)geneva9_advance_w(c);
        }
#endif
        ++str;
    }
    return width;
}

/* --------------------------------------------------------------------------
 * text_draw -- render 'str' into bitmap *bm at pixel (x, y).
 *
 * 'y' is the top of the cell (row 0 of the glyph bitmap).
 * Renders left-to-right; x advances by each glyph's advance width.
 * Delegates clipping to surface_blit (bounds-safe; Rule 2).
 * 'fg'/'bg' are packed 0x00RRGGBB (surface.h canonical format).
 *
 * When TEXT_MUTATE_FIXED_PITCH is defined, advance is fixed 6 px (matching
 * text_measure's mutant so the pair stays consistent under mutation).
 * -------------------------------------------------------------------------- */
static inline void text_draw(const bitmap_t *bm,
                             int x, int y,
                             const char *str,
                             text_font_t font,
                             uint32_t fg, uint32_t bg)
{
    if (!bm || !str)
        return;
    while (*str) {
        int c = (unsigned char)*str;
        const unsigned char *glyph;
        unsigned int cell_w, cell_h, adv;

        if (font == FONT_CHICAGO) {
            glyph  = chicago8x16_glyph(c);
            cell_w = (unsigned int)CHICAGO_CELL_W;
            cell_h = (unsigned int)CHICAGO_CELL_H;
            adv    = (unsigned int)CHICAGO_CELL_W;
        } else {
            glyph  = geneva9_glyph(c);
            /* Geneva bitmaps are packed 8 columns wide; render as 8px cell. */
            cell_w = 8u;
            cell_h = (unsigned int)GENEVA9_CELL_H;
            adv    = geneva9_advance_w(c);
        }

#if defined(TEXT_MUTATE_FIXED_PITCH) && TEXT_MUTATE_FIXED_PITCH
        adv = 6u; /* named mutant: fixed advance */
#endif

        surface_blit(bm,
                     (uint32_t)x, (uint32_t)y,
                     glyph,
                     cell_w, cell_h,
                     fg, bg);

        x += (int)adv;
        ++str;
    }
}

/* --------------------------------------------------------------------------
 * text_center_in -- x offset to center 'str' in a rect of width rect_w.
 *
 * Returns (rect_w - text_measure(font, str)) / 2. If the string is wider
 * than rect_w, returns 0 (left-justified; never returns negative).
 *
 * Used by title-bar chrome rendering (ADR-0004 D-7) and dialog labels.
 * -------------------------------------------------------------------------- */
static inline int text_center_in(int rect_w, const char *str, text_font_t font)
{
    int w = text_measure(font, str);
    int off = (rect_w - w) / 2;
    return (off < 0) ? 0 : off;
}

/* --------------------------------------------------------------------------
 * text_font_from_txfont -- map GrafPort txFont integer to text_font_t.
 *
 * Ref: spec/grafport.h (txFont; "0=systemFont (Chicago); 1=applFont");
 *      ADR-0004 D-7 ("Chicago (system/dialog) and Geneva 9 (cell)").
 *
 * txFont 0 (systemFont) -> FONT_CHICAGO  (safe default + unknown values)
 * txFont 1 (applFont)   -> FONT_GENEVA9
 *
 * Implemented in text.c (non-inline, for stable linkage).
 * -------------------------------------------------------------------------- */
text_font_t text_font_from_txfont(int txfont);

#endif /* INITECH_OS_FLAIR_TEXT_H */
