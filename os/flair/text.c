/*
 * os/flair/text.c -- FLAIR text rendering (proportional bitmap fonts).
 *
 * Ref: ADR-0004 D-7 ("proportional NFNT text measurement -- Chicago (system/
 *      dialog) and Geneva 9 (cell), hand-authored strikes; text width = sum
 *      of per-glyph advances; no fixed-pitch assumption");
 *      PRD Sec 6.4 (font resources); PRD Sec 6.3 (Toolbox layer).
 *      spec/assets/chicago8x16.h (Chicago strike, fixed-cell v0).
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
 * object file for the kernel link that carries the font data (chicago8x16.h
 * and geneva9.h tables are static, so they live in this TU's data section).
 *
 * ASCII-clean (Rule 12). No nondeterminism (Rule 11).
 */

/* Pull in the full text API (inline implementations + strike tables). */
#include "text.h"

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
