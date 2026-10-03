/*
 * os/flair/text.h -- FLAIR text rendering API (proportional bitmap fonts).
 *
 * Ref: ADR-0004 D-7 ("proportional NFNT text measurement -- Chicago (system/
 *      dialog) and Geneva 9 (cell), hand-authored strikes; text width = sum
 *      of per-glyph advances; no fixed-pitch assumption");
 *      PRD Sec 6.4 (font resources); PRD Sec 6.3 (Toolbox layer).
 *      docs/research/gui-ground-truth.md Sec 3.5 (Chicago 12 and Geneva 9).
 *      spec/assets/chicago12.h (Chicago 12 strike: REAL NFNT 5478 advances +
 *        left bearings, hand-authored art; bead initech-tdnl.33).
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
 *   text_font_t selects Chicago 12 (FONT_CHICAGO) or Geneva 9
 *   (FONT_GENEVA9). Both are backed by hand-authored clean-room strikes
 *   (spec/assets/chicago12.h and spec/assets/geneva9.h respectively).
 *
 * CHICAGO CELL MODEL (bead initech-tdnl.33; NFNT owTable semantics, font-
 *   manager.md Sec 1.1): glyph c owns the OPAQUE cell [pen, pen + aw(c)) x
 *   CHICAGO_CELL_H rows; its ink image starts at pen + lb(c). aw and lb are the
 *   real System 7.0.1 Chicago 12 values. The baseline is row CHICAGO_ASCENT
 *   (12): caps/ascenders on rows 3..11, descenders on rows 12..14, row 15 is
 *   the leading row. chicago_cell_bits(c, r) is the row in CELL coordinates
 *   (MSB = cell column 0) for the clipped glyph walkers in chrome.c/tbxgate.c.
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
 * Ref: spec/assets/chicago12.h, spec/assets/geneva9.h (found via -Ispec/assets). */
#include "chicago12.h"    /* chicago12_aw/_lb/_rows, CHICAGO_CELL_H/_ASCENT    */
#include "geneva9.h"      /* geneva9_glyph(), geneva9_advance_w(), GENEVA9_CELL_H */

/* --------------------------------------------------------------------------
 * text_font_t -- font selector (ADR-0004 D-7).
 *
 * Maps to the GrafPort txFont values:
 *   0 (systemFont) -> FONT_CHICAGO
 *   1 (applFont)   -> FONT_GENEVA9
 * -------------------------------------------------------------------------- */
typedef enum text_font {
    FONT_CHICAGO  = 0,  /* Chicago 12 (proportional, NFNT 5478 metrics)     */
    FONT_GENEVA9  = 1   /* Geneva 9 (proportional cell v0); small UI font     */
} text_font_t;

/* --------------------------------------------------------------------------
 * Chicago 12 per-glyph accessors (bead initech-tdnl.33).
 *
 * chicago_advance(c) -- the pen advance (NFNT owTable aw); the opaque cell
 *   width. CHICAGO_MUT_ONE_ADVANCE=<code> (Rule 6; the font mutants of
 *   test-chicago-metrics / test-chrome-fidelity / the re-keyed layout gates)
 *   widens ONE glyph by 1 px in measure AND draw; NEVER in a real build.
 * chicago_bearing(c) -- the left bearing (owTable lb): image column 0 sits at
 *   pen + lb.
 * chicago_cell_bits(c, r) -- row r of the glyph in CELL coordinates: bit
 *   (0x8000 >> col) set = ink at pen + col, for col in [0, aw).
 * -------------------------------------------------------------------------- */
static inline int chicago_advance(int c)
{
    int aw = chicago12_advance(c);
#if defined(CHICAGO_MUT_ONE_ADVANCE)
    if (c == (CHICAGO_MUT_ONE_ADVANCE))
        aw += 1;
#endif
    return aw;
}

static inline int chicago_bearing(int c)
{
    return (int)chicago12_lb[chicago12_index(c)];
}

static inline unsigned int chicago_cell_bits(int c, int r)
{
    int g = chicago12_index(c);
    return (unsigned int)chicago12_rows[g][r] >> chicago12_lb[g];
}

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
 * For Chicago 12: advance is per-glyph chicago_advance (NFNT 5478 aw).
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
            width += chicago_advance(c);
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
        unsigned int adv;

        if (font == FONT_CHICAGO) {
            /* The opaque aw-wide cell, as at most two 8-column halves through
             * surface_blit (the ONE glyph-blit primitive, ADR-0004 D-2). */
            /* A cell starting above row 0 is TOP-CLIPPED (its first -y rows
             * dropped), so a caller can place a sampled cap row inside a
             * sub-bitmap view (menu.c item rows). */
            unsigned char lo[CHICAGO_CELL_H], hi[CHICAGO_CELL_H];
            unsigned int aw = (unsigned int)chicago_advance(c);
            int g = chicago12_index(c);
            const uint16_t *rows = chicago12_rows[g];
            unsigned int lb = chicago12_lb[g];
            int r0 = (y < 0) ? -y : 0;
            for (int r = 0; r < CHICAGO_CELL_H; r++) {
                unsigned int bits = (unsigned int)rows[r] >> lb;
                lo[r] = (unsigned char)(bits >> 8);
                hi[r] = (unsigned char)(bits & 0xFFu);
            }
            if (r0 < CHICAGO_CELL_H) {
                uint32_t rows = (uint32_t)(CHICAGO_CELL_H - r0);
                surface_blit(bm, (uint32_t)x, (uint32_t)(y + r0), lo + r0,
                             aw < 8u ? aw : 8u, rows, fg, bg);
                if (aw > 8u)
                    surface_blit(bm, (uint32_t)(x + 8), (uint32_t)(y + r0),
                                 hi + r0, aw - 8u, rows, fg, bg);
            }
            adv = aw;
        } else {
            /* Geneva bitmaps are packed 8 columns wide; render as 8px cell. */
            surface_blit(bm, (uint32_t)x, (uint32_t)y, geneva9_glyph(c),
                         8u, (unsigned int)GENEVA9_CELL_H, fg, bg);
            adv = geneva9_advance_w(c);
        }

#if defined(TEXT_MUTATE_FIXED_PITCH) && TEXT_MUTATE_FIXED_PITCH
        adv = 6u; /* named mutant: fixed advance */
#endif

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
