/*
 * spec/chrome_metrics.h -- the C-consumable native chrome constants.
 *
 * GENERATED-EQUIVALENT, HAND-MAINTAINED form of the `native` section of
 * spec/chrome_metrics.json (the LOCKED spec-data, CLAUDE.md Rule 8). Freestanding
 * C cannot parse JSON, so the artifact chrome drawer (os/flair/chrome.c) consumes
 * these #defines instead. They are NOT a second source of truth: test-chrome's
 * STEP-1 consistency tooth parses spec/chrome_metrics.json with python3 and
 * asserts every #define below equals the locked JSON `native.<key>.value`, so this
 * header can NEVER silently drift from the lock (ADR-0004 FO-2/AM-3; Law 2).
 *
 * beads: initech-k8o5.8 (test-chrome oracle) / initech-k8o5.7 (host render
 *        skeleton). chrome_metrics.json v1 was locked by initech-k8o5.9.
 *
 * SOURCE / Law 1 citations (every value cites spec/chrome_metrics.json AND the
 * first-hand WDEF/Toolbox source the JSON records):
 *
 *   spec/chrome_metrics.json -- the LOCKED v1 native constants (this header's
 *     ONLY authority; the .json `native.<key>.value` is the truth, this header
 *     re-states it for freestanding C and is consistency-checked against it).
 *
 *   StandardWDEF.a -- Apple System 7.0 WDEF assembly, FIRST-HAND FETCHED 2026-06-19:
 *     https://developer.apple.com/library/archive/samplecode/System_7.0_WDEF/Listings/StandardWDEF_a.html
 *     Constants: minTitleH EQU 19, scrollBarSize EQU 16, dBoxBorderSize EQU 7,
 *     IconSize EQU 20, WBoxDelta (titleHeight-13)/2 => 13 px box,
 *     wTitleBarLight EQU 7, wTitleBarDark EQU 8.
 *
 *   GetMBarHeight (Inside Macintosh: Toolbox), FIRST-HAND FETCHED 2026-06-19:
 *     https://developer.apple.com/library/archive/documentation/mac/Toolbox/Toolbox-128.html
 *     Returns 20 px (Roman script) -- the menu-bar height.
 *
 *   Inside Macintosh: Macintosh Toolbox Essentials (Toolbox-303/313) -- the
 *     1-px window frame and 16-px scrollbar; BRIEF-SOURCED (PDF binary; see
 *     docs/research/gui-ground-truth.md Sec 3.3 + chrome_metrics.json provenance).
 *
 * DUAL-COMPILE: freestanding (gcc -m32 -ffreestanding -nostdlib -std=c11) AND
 * hosted (cc -std=c11). Pure #defines + _Static_asserts; no includes needed
 * beyond <stdint.h> for the assert expressions. Rule 11 (deterministic),
 * Rule 12 (ASCII-clean). Changing this header is a deliberate Rule 8 act that
 * MUST track a matching change to spec/chrome_metrics.json (the consistency
 * tooth fails otherwise).
 */
#ifndef INITECH_SPEC_CHROME_METRICS_H
#define INITECH_SPEC_CHROME_METRICS_H

/* ===========================================================================
 * THE NATIVE CHROME CONSTANTS  (== spec/chrome_metrics.json `native.*.value`)
 * ---------------------------------------------------------------------------
 * Each macro names the JSON key it mirrors; the consistency tooth in
 * test-chrome maps them 1:1 and diffs the values (build/decode the JSON, not
 * trust the comment).
 * ===========================================================================*/

/* native.menubar_height.value = 20  (GetMBarHeight, Roman script; Toolbox-128) */
#define FLAIR_CHROME_MENUBAR_H          20

/* native.titlebar_height_std.value = 19  (WDEF minTitleH EQU 19) */
#define FLAIR_CHROME_TITLEBAR_H         19

/* native.scrollbar_width.value = 16  (WDEF scrollBarSize EQU 16; Toolbox-313) */
#define FLAIR_CHROME_SCROLLBAR_W        16

/* native.window_frame.value = 1  (WDEF frame inset = 1; Toolbox-313 "1-pixel
 * window frame"). The classic Mac double line is this 1 px frame + 1 px groove. */
#define FLAIR_CHROME_FRAME              1

/* native.dialog_dboxproc_border.value = 7  (WDEF dBoxBorderSize EQU 7) */
#define FLAIR_CHROME_DIALOG_BORDER      7

/* native.close_zoom_box_frame_delta.value = 13  (WDEF WBoxDelta: box is 13 px
 * tall, vertically centered in the title bar via (titleHeight-13)/2) */
#define FLAIR_CHROME_WBOX_DELTA         13

/* native.pinstripe_period.value = 2  (1 dark + 1 light alternating scanline) */
#define FLAIR_CHROME_PINSTRIPE_PERIOD   2

/* native.titlebar_shade_indices.wTitleBarLight = 7  (wctb index, NOT an RGB --
 * the exact 8-bpp RGB is golden-resolves; we use the index as the indexed-8
 * palette index for the light pinstripe scanline) */
#define FLAIR_CHROME_TITLE_SHADE_LIGHT  7

/* native.titlebar_shade_indices.wTitleBarDark = 8  (wctb index; dark scanline) */
#define FLAIR_CHROME_TITLE_SHADE_DARK   8

/* native.grow_box_size.value = 16  (16x16 grow box; WDEF, computed from
 * scrollBarSize) */
#define FLAIR_CHROME_GROW               16

/* native.small_icon_in_title.value = 20  (WDEF IconSize EQU 20; the small SICN
 * icon width in the title bar) */
#define FLAIR_CHROME_SMALL_ICON         20

/* ===========================================================================
 * COMPILE-TIME SANITY  (the cheap internal-consistency tooth; the .h<->.json
 * lock is enforced at build time by the python3 consistency tooth in
 * test-chrome, not here -- C cannot read the JSON freestanding).
 * ===========================================================================*/
#include <stdint.h>

/* The pinstripe period MUST be 2 (a one-dark/one-light pair); period 1 would be
 * a solid band, period >2 would not alternate per the WDEF shade-index pair. */
_Static_assert(FLAIR_CHROME_PINSTRIPE_PERIOD == 2,
               "pinstripe period is 2 px (1 light + 1 dark; WDEF shade pair)");

/* The two shade indices MUST differ (an alternation needs two distinct shades).*/
_Static_assert(FLAIR_CHROME_TITLE_SHADE_LIGHT != FLAIR_CHROME_TITLE_SHADE_DARK,
               "pinstripe light/dark shade indices must differ (WDEF 7 vs 8)");

/* The frame is the classic 1 px Mac line (a 0 px frame would be no frame; a
 * thicker frame would be the dBoxProc 7 px border, a different chrome). */
_Static_assert(FLAIR_CHROME_FRAME == 1,
               "documentProc window frame is exactly 1 px (WDEF/Toolbox-313)");

/* The close/zoom box (13 px) must fit inside the title bar (19 px) with room to
 * center it (WBoxDelta = (titleH-13)/2 must be >= 0). */
_Static_assert(FLAIR_CHROME_WBOX_DELTA <= FLAIR_CHROME_TITLEBAR_H,
               "the 13 px close/zoom box must fit in the 19 px title bar");

/* The scrollbar and grow box are both the 16 px scrollBarSize family. */
_Static_assert(FLAIR_CHROME_SCROLLBAR_W == FLAIR_CHROME_GROW,
               "scrollbar width and grow box both derive from scrollBarSize=16");

#endif /* INITECH_SPEC_CHROME_METRICS_H */
