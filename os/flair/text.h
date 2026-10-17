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
 *   test-chicago-metrics / test-chrome-fidelity / the re-keyed layout gates,
 *   hooked in chicago12.h's chicago12_advance so window.c is covered too)
 *   widens ONE glyph by 1 px in measure AND draw; NEVER in a real build.
 * chicago_bearing(c) -- the left bearing (owTable lb): image column 0 sits at
 *   pen + lb.
 * chicago_cell_bits(c, r) -- row r of the glyph in CELL coordinates: bit
 *   (0x8000 >> col) set = ink at pen + col, for col in [0, aw).
 * -------------------------------------------------------------------------- */
static inline int chicago_advance(int c)
{
    return chicago12_advance(c);   /* the mutant hook lives there */
}

static inline int chicago_bearing(int c)
{
    return (int)chicago12_lb[chicago12_index(c)];
}

/* The kernel stores row indices into a dictionary generated from the locked
 * strike. Host rendering continues to use the original rows. PRD Sec 6.4;
 * test-chicago-pack compares EVERY row before any pixel acceptance. */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
static inline uint16_t text_chicago_row(unsigned g,unsigned r)
{
    return chicago12_rows[g][r];
}
#else
uint16_t text_chicago_row(unsigned g,unsigned r);
#endif

static inline unsigned int chicago_cell_bits(int c, int r)
{
    int g = chicago12_index(c);
    return (unsigned int)text_chicago_row((unsigned)g,(unsigned)r) >> chicago12_lb[g];
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
            unsigned int lb = chicago12_lb[g];
            int r0 = (y < 0) ? -y : 0;
            for (int r = 0; r < CHICAGO_CELL_H; r++) {
                unsigned int bits = (unsigned int)text_chicago_row((unsigned)g,(unsigned)r) >> lb;
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

/* ==========================================================================
 * FONT MANAGER SLICE 2 (bead initech-tdnl.68): txFace synthesis, styled
 * advances, and THE ONE region-clipped text run.
 *
 * GROUND TRUTH (Law 1), per style -- the rule, its source, and what grades it:
 *
 *   BOLD   = OR each glyph row with a copy shifted TEXT_BOLD_SMEAR (1) px
 *            right; the pen advance grows by the same 1 px per glyph.
 *            [documented: ../system7-decomp/specs/fonts/font-manager.md Sec 6.1
 *            + chicago.md "Synthesis algorithm" (FMOutRec.bold "how much to
 *            smear horiz", refs/quickdraw/Apple-QuickDraw2.p); GOLDEN-VERIFIED:
 *            test-font-styles leg G1 feeds THIS synthesis the real Geneva 9
 *            strike (NFNT 5415) and reproduces, pixel for pixel, the bold
 *            labels System 7.5.3 drew in goldens/captures/s7_get_info.png
 *            ("Kind:", "Size:", "Where:", "Created:", "Modified:", "Version:",
 *            "Comments:", "Locked") and s7_about.png ("Total Memory:").]
 *            Chicago has no bold golden in the corpus; the Font Manager
 *            synthesizes bold the same way for every family (font-manager.md
 *            Sec 6.3: "bold is synthesized by smear exactly as for Chicago").
 *
 *   ITALIC = shear: row r moves text_italic_shift(d) px, d = rows above the
 *            baseline (d = baseline_row - r; d <= 0 below it): floor(d / 5),
 *            so the 4 rows sitting on the baseline stay put, rows 5..9 above
 *            it move +1, and the descender rows move -1. The pen advance does
 *            NOT change. [documented: font-manager.md Sec 6.2 (shear, "rows
 *            above the baseline shift right, rows below shift left", advance
 *            unchanged; the exact ratio left "golden-resolves"). MEASURED:
 *            the ratio is the per-row MODAL shift between Apple's own Geneva 9
 *            italic strike NFNT 6475 and plain NFNT 5415 over the 94 printable
 *            glyphs (geneva.md "Geneva 9 italic"; geneva.md "Clean-room note":
 *            a synthesized shear of the plain strike is the accepted clean-room
 *            substitute). test-font-styles leg I re-measures that mode from the
 *            two resources at run time and grades text_italic_shift against it
 *            row by row. FOR CHICAGO the same ratio is applied:
 *            [documented, not golden-verified] -- the corpus holds no rendered
 *            italic Chicago.]
 *
 *   UNDERLINE = a TEXT_UL_THICK (1) px line TEXT_UL_OFFSET (1) row below the
 *            baseline row, under the whole run [x, x + width), broken with a
 *            TEXT_UL_SHADOW (1) px halo wherever glyph ink touches that row
 *            (the descender gap). The pen advance does NOT change.
 *            [documented, not golden-verified: font-manager.md Sec 6.2.1 /
 *            QuickDraw2.p FMOutRec ulOffset ("pixels below baseline"),
 *            ulThick, ulShadow ("halo size"). The corpus holds no underlined
 *            capture; offset 1 / thickness 1 / halo 1 are InitechOS's reading
 *            of those fields, NOT measurements.]
 *
 *   Outline, shadow, condense, extend: out of scope (no measurement); those
 *   txFace bits are IGNORED by this slice (TEXT_FACE_SYNTH masks them off).
 *
 * The baseline row (first row BELOW the baseline, the QuickDraw pen v):
 *   Chicago 12  row CHICAGO_ASCENT (12) -- NFNT 5478 ascent 12.
 *   Geneva 9 v0 row GENEVA9_BASELINE_ROW (8) -- the hand-authored v0 strike's
 *               caps / x-height / period end on row 7 (spec/assets/geneva9.h).
 * ========================================================================== */
#define TEXT_FACE_BOLD       0x01u   /* == FLAIR_STYLE_BOLD      (grafport.h) */
#define TEXT_FACE_ITALIC     0x02u   /* == FLAIR_STYLE_ITALIC                 */
#define TEXT_FACE_UNDERLINE  0x04u   /* == FLAIR_STYLE_UNDERLINE              */
#define TEXT_FACE_SYNTH      0x07u   /* the faces this slice synthesizes      */
#define TEXT_BOLD_SMEAR      1       /* FMOutRec.bold, the standard smear     */
#define TEXT_ITALIC_RUN      5       /* rows per 1 px of shear (measured)     */
#define TEXT_UL_OFFSET       1       /* FMOutRec.ulOffset (documented reading) */
#define TEXT_UL_THICK        1       /* FMOutRec.ulThick                      */
#define TEXT_UL_SHADOW       1       /* FMOutRec.ulShadow halo                */
#define GENEVA9_BASELINE_ROW 8       /* first row below the v0 Geneva baseline */
/* Row masks: bit (31 - (col + TEXT_OVERHANG)) = ink at pen + col, so a glyph
 * row may overhang its pen by up to TEXT_OVERHANG columns on the left (the
 * italic descender shift) and reach col 31 - TEXT_OVERHANG on the right. */
#define TEXT_OVERHANG        4
/* The underline halo looks at most this far right of the run's pen origin;
 * a run is at most one 640-px screen line, so 1024 columns cover any run. */
#define TEXT_RUN_MAX_W       1024

/* The first row below the baseline for `font` (the QuickDraw pen v row). */
static inline int text_baseline_row(text_font_t font)
{
    return (font == FONT_CHICAGO) ? (int)CHICAGO_ASCENT : GENEVA9_BASELINE_ROW;
}

/* The italic shear of a row d rows above the baseline (d <= 0: on or below):
 * floor(d / TEXT_ITALIC_RUN). TEXT_MUT_ITALIC_RUN4 (Rule 6, test-font-styles)
 * uses 1:4 -- the measured mode disagrees with it. */
static inline int text_italic_shift(int d)
{
#if defined(TEXT_MUT_ITALIC_RUN4)
    const int run = 4;
#else
    const int run = TEXT_ITALIC_RUN;
#endif
    return (d >= 0) ? d / run : -((-d + run - 1) / run);
}

/* text_synth_row -- THE synthesis of one glyph row: `plain` is the plain
 * strike's row as a TEXT_OVERHANG mask (see above), d its height above the
 * baseline. Italic shears first, then bold smears (Geneva 9 bold+italic is
 * the italic strike plus the smear, font-manager.md Sec 6.3). Underline is a
 * RUN decoration (text_draw_run), not a glyph-row transform.
 * TEXT_MUT_SMEAR_OFF (Rule 6): bold keeps its +1 advance but loses the smear. */
static inline uint32_t text_synth_row(uint32_t plain, unsigned face, int d)
{
    uint32_t m = plain;
    if (face & TEXT_FACE_ITALIC) {
        int s = text_italic_shift(d);
        m = (s >= 0) ? (m >> s) : (m << -s);
    }
#if !defined(TEXT_MUT_SMEAR_OFF)
    if (face & TEXT_FACE_BOLD)
        m |= m >> TEXT_BOLD_SMEAR;
#endif
    return m;
}

/* The plain strike's row r of glyph c as a TEXT_OVERHANG mask. */
static inline uint32_t text_plain_row(text_font_t font, int c, int r)
{
    if (font == FONT_CHICAGO)
        return (uint32_t)chicago_cell_bits(c, r) << (16 - TEXT_OVERHANG);
    if (r < 0 || r >= GENEVA9_CELL_H)
        return 0u;
    return (uint32_t)geneva9_glyph(c)[r] << (24 - TEXT_OVERHANG);
}

/* Row r (cell coordinates, 0 = cell top) of glyph c in `face`. */
static inline uint32_t text_face_row(text_font_t font, unsigned face, int c,
                                     int r)
{
    return text_synth_row(text_plain_row(font, c, r), face,
                          text_baseline_row(font) - r);
}

/* The styled pen advance of byte c: plain advance + TEXT_BOLD_SMEAR when bold
 * (font-manager.md Sec 6: synth_aw = plain_aw + FMOutRec.bold + extra; italic
 * and underline never change it). TEXT_MUT_BOLD_ADVANCE_PLAIN (Rule 6) drops
 * the +1. */
static inline int text_face_advance(text_font_t font, unsigned face, int c)
{
    int aw = (font == FONT_CHICAGO) ? chicago_advance(c)
                                    : (int)geneva9_advance_w(c);
#if !defined(TEXT_MUT_BOLD_ADVANCE_PLAIN)
    if (face & TEXT_FACE_BOLD)
        aw += TEXT_BOLD_SMEAR;
#else
    (void)face;
#endif
    return aw;
}

/* text_measure_face -- the styled StringWidth: the sum of the styled advances
 * (font-manager.md Sec 6: StringWidth_bold(s) = StringWidth_plain(s) + len(s);
 * italic and underline widths equal plain). Plain face == text_measure. */
static inline int text_measure_face(text_font_t font, unsigned face,
                                    const char *str)
{
    int w = 0;
    if (!str)
        return 0;
    for (; *str; str++)
        w += text_face_advance(font, face, (int)(unsigned char)*str);
    return w;
}

/* --------------------------------------------------------------------------
 * text_clip_t -- where a run may paint. A pixel is written IFF it is on the
 * bitmap AND inside rgn_a (NULL = everywhere) AND inside rgn_b (NULL =
 * everywhere) AND inside the half-open box [left,right) x [top,bottom) (an
 * empty box, right <= left, means no box). rgn_a/rgn_b are a port's visRgn and
 * clipRgn (grafport.h CLIPPING INVARIANT: all drawing is clipped by visRgn
 * INTERSECT clipRgn) or any one effective clip; the box is a sub-rect such as
 * a menu item row. The regions are ATKINSON region_t (spec/region_algebra.h),
 * named by tag here so this header keeps its include set.
 * -------------------------------------------------------------------------- */
struct region;
int region_contains_point(const struct region *r, int16_t h, int16_t v);

typedef struct text_clip {
    const struct region *rgn_a;
    const struct region *rgn_b;
    int left, top, right, bottom;
} text_clip_t;

/* Linkage of text_draw_run, the font_linkage.h pattern (one kernel copy,
 * defined in text.c; a static per-TU copy in hosted builds so the host
 * oracles' link lines stay unchanged; an unused static INLINE is never emitted). */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
#  define TEXT_RUN_EMIT 1
#  define TEXT_RUN_FN   static inline __attribute__((unused))
#elif defined(FONT_TABLES_DEFINE)
#  define TEXT_RUN_EMIT 1
#  define TEXT_RUN_FN
#else
#  define TEXT_RUN_EMIT 0
#endif

/* --------------------------------------------------------------------------
 * text_draw_run -- THE styled, clipped text run (bead initech-tdnl.68).
 *
 * Draws `str` with its cell top at (x, y) in `font` / `face` (TEXT_FACE_*;
 * other bits ignored): first the OPAQUE box [x, x + W) x [y, y + cell_h) in
 * bg, W = text_measure_face, then every glyph's styled ink in fg (srcOr over
 * the erased box, so an italic overhang is never erased by its neighbour's
 * box), then the underline. Every write passes the text_clip_t test (clip
 * NULL = bitmap bounds only). Returns W. For the plain face the pixels are
 * exactly text_draw's (opaque advance-wide cells, ink on the strike's bits).
 * TEXT_MUT_CLIP_IGNORE (Rule 6, test-text-clip) ignores the regions + box.
 * -------------------------------------------------------------------------- */
#if TEXT_RUN_EMIT
static inline int text_clip_in(const bitmap_t *bm, const text_clip_t *k,
                               int px, int py)
{
    if (px < 0 || py < 0 || px >= (int)bm->width || py >= (int)bm->height)
        return 0;
#if !defined(TEXT_MUT_CLIP_IGNORE)
    if (k != (const text_clip_t *)0) {
        if (k->right > k->left &&
            (px < k->left || px >= k->right || py < k->top || py >= k->bottom))
            return 0;
        if (k->rgn_a && !region_contains_point(k->rgn_a, (int16_t)px, (int16_t)py))
            return 0;
        if (k->rgn_b && !region_contains_point(k->rgn_b, (int16_t)px, (int16_t)py))
            return 0;
    }
#else
    (void)k;
#endif
    return 1;
}

static inline void text_clip_put(const bitmap_t *bm, const text_clip_t *k,
                                 int px, int py, uint32_t v)
{
    if (text_clip_in(bm, k, px, py))
        surface_put_pixel(bm, (uint32_t)py * bm->pitch +
                              (uint32_t)px * bm->bytes_per_pixel, v);
}

TEXT_RUN_FN int text_draw_run(const bitmap_t *bm, int x, int y,
                              const char *str, text_font_t font,
                              unsigned face, uint32_t fg, uint32_t bg,
                              const text_clip_t *clip)
{
    int w, h, pen, base;
    const char *s;

    if (!bm || !bm->base || !str)
        return 0;
    face &= TEXT_FACE_SYNTH;
    w = text_measure_face(font, face, str);
    h = text_cell_height(font);
    base = text_baseline_row(font);

    /* 1. the erased box, the run's opaque background */
    for (int r = 0; r < h; r++)
        for (int c = 0; c < w; c++)
            text_clip_put(bm, clip, x + c, y + r, bg);

    /* 2. the styled ink, srcOr */
    pen = x;
    for (s = str; *s; s++) {
        int ch = (int)(unsigned char)*s;
        for (int r = 0; r < h; r++) {
            uint32_t m = text_face_row(font, face, ch, r);
            for (int b = 31; m != 0u && b >= 0; b--) {
                if (m & (1u << b)) {
                    m &= ~(1u << b);
                    text_clip_put(bm, clip,
                                  pen + (31 - b) - TEXT_OVERHANG, y + r, fg);
                }
            }
        }
        pen += text_face_advance(font, face, ch);
    }

    /* 3. the underline: TEXT_UL_THICK rows from base + TEXT_UL_OFFSET, broken
     * by a TEXT_UL_SHADOW halo around the ink on each of those rows. */
    if (face & TEXT_FACE_UNDERLINE) {
        for (int t = 0; t < TEXT_UL_THICK; t++) {
#if defined(TEXT_MUT_UL_OFFSET)
            int ur = base + TEXT_UL_OFFSET + 1 + t;   /* named mutant: 1 low */
#else
            int ur = base + TEXT_UL_OFFSET + t;
#endif
            uint32_t ink[TEXT_RUN_MAX_W / 32 + 2];
            int iw = (int)(sizeof ink / sizeof ink[0]);
            if (ur >= h)
                continue;
            for (int i = 0; i < iw; i++)
                ink[i] = 0u;
            /* the run's ink on this row, as columns col + 1 of the bitset
             * (col = pixel - x, from -1 so the halo of col 0 is covered) */
            pen = 0;
            for (s = str; *s; s++) {
                int ch = (int)(unsigned char)*s;
                uint32_t m = text_face_row(font, face, ch, ur);
                for (int b = 31; b >= 0; b--) {
                    int col = pen + (31 - b) - TEXT_OVERHANG;
                    if ((m & (1u << b)) && col + 1 >= 0 && col + 1 < iw * 32)
                        ink[(col + 1) >> 5] |= 1u << ((col + 1) & 31);
                }
                pen += text_face_advance(font, face, ch);
            }
            for (int col = 0; col < w; col++) {
                int gap = 0;
                for (int k = -TEXT_UL_SHADOW; k <= TEXT_UL_SHADOW; k++) {
                    int q = col + k + 1;
                    if (q >= 0 && q < iw * 32 && (ink[q >> 5] & (1u << (q & 31))))
                        gap = 1;
                }
                /* the ink itself stays (it was drawn in step 2); only the
                 * line's own pixels are withheld around it */
                if (!gap)
                    text_clip_put(bm, clip, x + col, y + ur, fg);
            }
        }
    }
    return w;
}
#else
int text_draw_run(const bitmap_t *bm, int x, int y, const char *str,
                  text_font_t font, unsigned face, uint32_t fg, uint32_t bg,
                  const text_clip_t *clip);
#endif


#endif /* INITECH_OS_FLAIR_TEXT_H */
