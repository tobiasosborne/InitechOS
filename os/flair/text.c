/*
 * os/flair/text.c -- FLAIR text rendering (proportional bitmap fonts).
 *
 * Ref: ADR-0004 D-7 ("proportional NFNT text measurement -- Chicago (system/
 *      dialog) and Geneva 9 (cell), hand-authored strikes; text width = sum
 *      of per-glyph advances; no fixed-pitch assumption");
 *      PRD Sec 6.4 (font resources); PRD Sec 6.3 (Toolbox layer).
 *      spec/assets/chicago12.h (Chicago 12 strike, NFNT 5478 metrics).
 *      spec/assets/geneva9.h (Geneva 9 strike, proportional v0).
 *      os/flair/surface.h (surface_blit -- the ONE glyph-blit primitive).
 *      os/flair/text.h (API declarations -- all implementations are inline
 *        in the header for dual-compile freestanding/hosted simplicity).
 *      CLAUDE.md Law 1 (ground truth before code), Law 3 (freestanding
 *      artifact; no libc, no malloc, no 2026-isms), Rule 11 (reproducible),
 *      Rule 12 (ASCII-clean source).
 *
 * ARTIFACT CODE: freestanding C (ADR-0002). No libc. No malloc. No host paths
 * or timestamps baked in. Compiles BOTH under kernel flags:
 *   gcc -m32 -ffreestanding -nostdlib -fno-builtin -DFLAIR_HOSTED=0 \
 *       -Ios/flair -Ispec/assets -Ispec -c text.c
 * AND hosted for the oracle harness:
 *   gcc -DFLAIR_HOSTED=1 -Ios/flair -Ispec/assets -Ispec -c text.c
 *
 * MUTATION ORACLE NOTE (Rule 6):
 *   Build with -DTEXT_MUTATE_FIXED_PITCH=1 to activate the named mutant
 *   in text_measure() and text_draw(): every glyph gets a fixed 6px advance
 *   instead of the per-glyph value. The proportional property tests in
 *   harness/proptest/test_text.c MUST go RED under this mutant, proving the
 *   oracle catches fixed-pitch drift.
 *
 * All the text_measure / text_draw / text_center_in implementations live as
 * static inline functions in text.h. This translation unit (#include "text.h")
 * causes the compiler to instantiate those functions and catches any compile
 * error under both freestanding and hosted modes. It also provides a stable
 * object file for the kernel link that carries the font data: chicago12.h and
 * geneva9.h are DEFINED here (FONT_TABLES_DEFINE) and only declared elsewhere.
 * The same switch makes this the ONE kernel definition of text_draw_run, the
 * styled, region-clipped Font Manager run (text.h; bead initech-tdnl.68).
 *
 * ASCII-clean (Rule 12). No nondeterminism (Rule 11).
 */

/* THE one definition site of the font tables in a kernel image (bead
 * initech-tdnl.33 ruling 3; spec/assets/font_linkage.h): every other kernel TU
 * sees an extern declaration and links against this TU's single copy. */
#define FONT_TABLES_DEFINE 1

/* Pull in the full text API (inline implementations + strike tables). */
#include "text.h"

#if !defined(__STDC_HOSTED__) || !__STDC_HOSTED__
#include "chicago_packed.h"
uint16_t text_chicago_row(unsigned g,unsigned r)
{
    return chicago_packed_row(g,r);
}
/* Geneva 12 (bead initech-uelk): the same generated-dictionary storage. */
#include "geneva12_packed.h"
uint16_t text_geneva12_row(unsigned g,unsigned r)
{
    return geneva12_packed_row(g,r);
}
#endif

/*
 * text_font_from_txfont -- map a GrafPort txFont integer to text_font_t.
 *
 * Ref: spec/grafport.h (txFont field; "0 = systemFont (Chicago 12); 1 =
 * applFont"); ADR-0004 D-7 ("Chicago (system/dialog) and Geneva 9 (cell)").
 *
 * txFont 0 (systemFont) -> FONT_CHICAGO  (system/menu/dialog/title text)
 * txFont 1 (applFont)   -> FONT_GENEVA9  (small cell/UI labels)
 * All other values       -> FONT_CHICAGO  (safe default; fail-closed)
 *
 * This is the ONLY place the txFont integer is converted to text_font_t;
 * callers outside this module should use this function rather than a bare
 * cast to keep the mapping centralised and auditable.
 */
text_font_t text_font_from_txfont(int txfont)
{
    if (txfont == (int)FONT_GENEVA9)
        return FONT_GENEVA9;
    return FONT_CHICAGO; /* default: systemFont and all unknown values */
}

/*
 * text_font_from_txfont_size -- (txFont, txSize) to a strike (bead
 * initech-uelk; text.h). Ref: IM-I QuickDraw (txFont picks the family,
 * txSize the point size; 0 = the family's default); ../system7-decomp
 * specs/sys8/simpletext-stdfile.md Sec 1.2 (SimpleText: Geneva = applFont,
 * 12 Point). applFont at 12 is the Geneva 12 strike; txSize 0 keeps the
 * one-argument mapping (Chicago 12 / Geneva 9); any other pair falls back to
 * it too -- the gate rejects such pairs before they get here.
 */
text_font_t text_font_from_txfont_size(int txfont, int txsize)
{
    if (txfont == (int)FONT_GENEVA9 && txsize == 12)
        return FONT_GENEVA12;
    return text_font_from_txfont(txfont);
}
