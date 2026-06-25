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

/* TITLE-BAR INTERIOR DECOMPOSITION (beads initech-92li; Rule 8 deliberate spec
 * change, 92li IS the issue).  The 19-px title band is NOT 19 all-stripe rows: it
 * decomposes (top-to-bottom, INCLUSIVE of both frame lines) as
 *   top-frame(1) + bevel-hi(1) + 15 pinstripe + bevel-lo(1) + shared-frame(1) = 19.
 * The bevel rows are the WDEF wLTinge0 (top highlight) / wLTinge4 (bottom shadow)
 * 3-D inset frame, 1px inside the black title FrameRect; the shared frame line is
 * the bottom of the title FrameRect AND the top of the content-body FrameRect.
 * Ref: ../system7-decomp/specs/chrome/window-frame.md Sec 2a (x=400 vertical scan:
 *   y=164 frame / y=165 bevel-hi #DADAFF / y=166..180 15 stripe / y=181 bevel-lo
 *   #B3B3DA / y=182 shared frame) + Sec 2b + pinstripe.md ("title interior height
 *   15 px y=166..180; bevel rows 1 px top + 1 px bottom").  These are NOT in
 *   chrome_metrics.json's `native` section (the JSON locks the 19-px BAND; this
 *   decomposition is the on-screen row breakdown the WDEF draws within it).
 *   ANY change to these is a deliberate Rule-8 act with an issue. */
#define FLAIR_CHROME_TITLE_BEVEL_ROWS   1   /* wLTinge0/wLTinge4 edge, 1px each   */
#define FLAIR_CHROME_TITLE_STRIPE_ROWS  15  /* pinstripe interior (golden y=166..180)*/

/* native.scrollbar_width.value = 16  (WDEF scrollBarSize EQU 16; Toolbox-313) */
#define FLAIR_CHROME_SCROLLBAR_W        16

/* native.window_frame.value = 1  (WDEF frame inset = 1; Toolbox-313 "1-pixel
 * window frame"). The classic Mac double line is this 1 px frame + 1 px groove. */
#define FLAIR_CHROME_FRAME              1

/* native.dialog_dboxproc_border.value = 7  (WDEF dBoxBorderSize EQU 7) */
#define FLAIR_CHROME_DIALOG_BORDER      7

/* native.close_zoom_box_frame_delta.value = 13  (WDEF WBoxDelta: the box-height
 * DERIVATION -- (titleHeight-13)/2 yields wBoxDelta, the vertical gap above the
 * box. This is the WDEF derivation, NOT the rendered gadget size.) */
#define FLAIR_CHROME_WBOX_DELTA         13

/* native.close_zoom_box_frame_delta.rendered_px = 11  (the close/zoom gadget
 * RENDERS 11x11 in the golden, NOT 13x13: the WDEF derives a 13 px box but the
 * InsetRect/EraseRect pair pulls the CopyBits dest in and the lavender bevel sits
 * INSIDE the dark frame, so the visible gadget is 11x11).  LAW 2: the golden wins.
 * Ref: ../system7-decomp/specs/chrome/close-zoom-box.md Geometry table (golden
 * s7_doc_window.png close box x=361..371, y=168..178 = 11x11 whole gadget incl
 * bevel; x=360/372 and y=167/179 are pinstripe #F3F3F3, NOT box). */
#define FLAIR_CHROME_WBOX_RENDER        11

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
 * ERA-TAGGED CHROME RGB CONSTANTS  (P1-4 uplift; beads initech-dh5k.4, 2026-06-21)
 * ---------------------------------------------------------------------------
 * Moved OUT of golden_resolves into locked native.* values in schema_version 3.
 * All values cited from ../system7-decomp/ and ../win31-decomp/ corpora.
 * era=system7.0-7.1 unless tagged win31-accent.
 * Ref: spec/chrome_metrics.json native.* entries (the authority; this header
 * re-states them for freestanding C consumption and is consistency-checked).
 * ===========================================================================*/

/* --- Pinstripe RGBs (era=system7.0-7.1) ----------------------------------- */

/* native.pinstripe_light_rgb = #F3F3F3 (243,243,243)
 * Rendered LIGHT stripe row in the active title bar (wHiliteShade8 fore over
 * wTitleBarShade1 back, $FF00 HilitePattern).
 * Ref: ../system7-decomp/specs/chrome/pinstripe.md; golden s7_doc_window.png
 * @ x=450,y=166; era-stable 7.0.1 [verified: re/mint-results-005.md]. */
#define FLAIR_CHROME_PINSTRIPE_LIGHT_R  243
#define FLAIR_CHROME_PINSTRIPE_LIGHT_G  243
#define FLAIR_CHROME_PINSTRIPE_LIGHT_B  243

/* native.pinstripe_dark_rgb = #969696 (150,150,150)
 * Rendered DARK stripe row in the active title bar (wTitleBarShade1 back).
 * Ref: ../system7-decomp/specs/chrome/pinstripe.md; golden s7_doc_window.png
 * @ x=450,y=168; era-stable 7.0.1 [verified: re/mint-results-005.md]. */
#define FLAIR_CHROME_PINSTRIPE_DARK_R   150
#define FLAIR_CHROME_PINSTRIPE_DARK_G   150
#define FLAIR_CHROME_PINSTRIPE_DARK_B   150

/* native.pinstripe_bevel_top_rgb = #DADAFF (218,218,255)
 * Top bevel HIGHLIGHT row (wLTinge0), 1px inside the black frame.
 * Ref: ../system7-decomp/specs/chrome/pinstripe.md; golden s7_doc_window.png
 * @ x=450,y=165; era-stable 7.0.1 [verified: re/mint-results-005.md]. */
#define FLAIR_CHROME_BEVEL_TOP_R        218
#define FLAIR_CHROME_BEVEL_TOP_G        218
#define FLAIR_CHROME_BEVEL_TOP_B        255

/* native.pinstripe_bevel_bottom_rgb = #B3B3DA (179,179,218)
 * Bottom bevel SHADOW row (wLTinge4), 1px inside the black frame at the base.
 * Ref: ../system7-decomp/specs/chrome/pinstripe.md; golden s7_doc_window.png
 * @ x=450,y=181; era-stable 7.0.1 [verified: re/mint-results-005.md]. */
#define FLAIR_CHROME_BEVEL_BOTTOM_R     179
#define FLAIR_CHROME_BEVEL_BOTTOM_G     179
#define FLAIR_CHROME_BEVEL_BOTTOM_B     218

/* --- Close/zoom box RGBs (era=system7.0-7.1) ------------------------------ */

/* native.close_zoom_box_dark_outline_rgb = #545487 (84,84,135)
 * Dark blue-violet outline of the close/zoom box gadget in IDLE state.
 * NOT pure black (wFrameColor / wDTingeF blend).
 * Ref: ../system7-decomp/specs/chrome/close-zoom-box.md; golden
 * s7_doc_window.png @ x=361,y=170; era-stable 7.0.1 [verified: re/mint-results-005.md]. */
#define FLAIR_CHROME_BOX_DARK_R         84
#define FLAIR_CHROME_BOX_DARK_G         84
#define FLAIR_CHROME_BOX_DARK_B         135

/* native.close_zoom_box_bevel_highlight_rgb = #DADAFF (218,218,255)
 * Inner lavender bevel HIGHLIGHT of the close/zoom box gadget (IDLE).
 * Same wLTinge0 family as the title-bar bevel top.
 * Ref: ../system7-decomp/specs/chrome/close-zoom-box.md; golden
 * s7_doc_window.png @ x=362,y=169; era-stable 7.0.1 [verified: re/mint-results-005.md].
 * (Same value as FLAIR_CHROME_BEVEL_TOP_*.) */
#define FLAIR_CHROME_BOX_BEVEL_R        218
#define FLAIR_CHROME_BOX_BEVEL_G        218
#define FLAIR_CHROME_BOX_BEVEL_B        255

/* native.close_zoom_box_face_rgb = #C0C0C0 (192,192,192)
 * Recessed 7x7 interior face of the close/zoom box (IDLE; wTitleBarShade4 blend).
 * Ref: ../system7-decomp/specs/chrome/close-zoom-box.md; golden
 * s7_doc_window.png @ x=366,y=172; era-stable 7.0.1 [verified: re/mint-results-005.md]. */
#define FLAIR_CHROME_BOX_FACE_R         192
#define FLAIR_CHROME_BOX_FACE_G         192
#define FLAIR_CHROME_BOX_FACE_B         192

/* native.close_zoom_box_pressed_interior_rgb = #B3B3DA (179,179,218)
 * Close/zoom box interior fill in PRESSED (mousedown) state (wGoAwayHilitedGadget).
 * PRESSED also changes outline from #545487 to #000000.
 * Ref: ../system7-decomp/specs/chrome/close-zoom-box.md;
 * golden s7_close_pressed.png + s7_zoom_pressed.png (mint-004);
 * era-stable 7.0.1 [verified: re/mint-results-005.md]. */
#define FLAIR_CHROME_BOX_PRESSED_R      179
#define FLAIR_CHROME_BOX_PRESSED_G      179
#define FLAIR_CHROME_BOX_PRESSED_B      218

/* --- Grow box RGBs (era=system7.0-7.1) ------------------------------------ */

/* native.grow_box_ground_rgb = #F3F3F3 (243,243,243)
 * DrawGrowIcon grow-box background field (wTitleBarShade1). Dominant color
 * (104 px). Same shade as pinstripe light row.
 * Ref: ../system7-decomp/specs/chrome/grow-box.md; golden s7_growbox.png
 * (mint-004); era-stable 7.0.1 [verified: re/mint-results-005.md]. */
#define FLAIR_CHROME_GROW_GROUND_R      243
#define FLAIR_CHROME_GROW_GROUND_G      243
#define FLAIR_CHROME_GROW_GROUND_B      243

/* native.grow_box_figure_rgb = #545487 (84,84,135)
 * Nested-box figure outlines in the active DrawGrowIcon gadget (wFrameColor /
 * wDTingeF). Same dark blue-violet as close/zoom box outline.
 * Ref: ../system7-decomp/specs/chrome/grow-box.md; golden s7_growbox.png
 * (mint-004); era-stable 7.0.1 [verified: re/mint-results-005.md]. */
#define FLAIR_CHROME_GROW_FIGURE_R      84
#define FLAIR_CHROME_GROW_FIGURE_G      84
#define FLAIR_CHROME_GROW_FIGURE_B      135

/* native.grow_box_highlight_rgb = #DADAFF (218,218,255)
 * wLTinge0 light highlight edges of the DrawGrowIcon outer nested box.
 * Same wLTinge0 family as the title-bar bevel top and box bevel highlight.
 * Ref: ../system7-decomp/specs/chrome/grow-box.md; golden s7_growbox.png
 * (mint-004); era-stable 7.0.1 [verified: re/mint-results-005.md]. */
#define FLAIR_CHROME_GROW_HIGHLIGHT_R   218
#define FLAIR_CHROME_GROW_HIGHLIGHT_G   218
#define FLAIR_CHROME_GROW_HIGHLIGHT_B   255

/* native.grow_box_inner_fill_rgb = #C0C0C0 (192,192,192)
 * Inner nested-box interior fill (wTitleBarShade4). Same as close/zoom box face.
 * Ref: ../system7-decomp/specs/chrome/grow-box.md; golden s7_growbox.png
 * (mint-004); era-stable 7.0.1 [verified: re/mint-results-005.md]. */
#define FLAIR_CHROME_GROW_FILL_R        192
#define FLAIR_CHROME_GROW_FILL_G        192
#define FLAIR_CHROME_GROW_FILL_B        192

/* --- Scrollbar RGBs (era=system7.0-7.1) ----------------------------------- */

/* native.scrollbar_active_thumb_light_rgb = #DADAFF (218,218,255)
 * LIGHT row of the active scrollbar thumb face dither (50/50 lavender dither).
 * Era-stable 7.0.1 [verified: re/mint-results-005.md].
 * Ref: ../system7-decomp/specs/chrome/scrollbar.md; golden
 * s7_scrollbar_active.png (mint-004). */
#define FLAIR_CHROME_THUMB_LIGHT_R      218
#define FLAIR_CHROME_THUMB_LIGHT_G      218
#define FLAIR_CHROME_THUMB_LIGHT_B      255

/* native.scrollbar_active_thumb_dark_rgb = #8787B3 (135,135,179)
 * DARK row of the active scrollbar thumb face dither (50/50 lavender dither).
 * LOW CONFIDENCE for era-stability to 7.0/7.1 -- the 7.5.3 value is verified;
 * 7.0.1 confirmation is [golden-resolves]. Use with caution.
 * Ref: ../system7-decomp/specs/chrome/scrollbar.md; golden
 * s7_scrollbar_active.png (mint-004). */
#define FLAIR_CHROME_THUMB_DARK_R       135
#define FLAIR_CHROME_THUMB_DARK_G       135
#define FLAIR_CHROME_THUMB_DARK_B       179

/* native.scrollbar_active_track_light_rgb = #E7E7E7 (231,231,231)
 * LIGHT element (2/3 ratio) of the active scrollbar track gutter dither.
 * Also the enabled arrow-box face fill.
 * Ref: ../system7-decomp/specs/chrome/scrollbar.md; golden
 * s7_scrollbar_active.png (mint-004). */
#define FLAIR_CHROME_TRACK_LIGHT_R      231
#define FLAIR_CHROME_TRACK_LIGHT_G      231
#define FLAIR_CHROME_TRACK_LIGHT_B      231

/* native.scrollbar_active_track_dark_rgb = #969696 (150,150,150)
 * DARK element (1/3 ratio) of the active scrollbar track gutter dither.
 * Same shade as the pinstripe dark row.
 * Ref: ../system7-decomp/specs/chrome/scrollbar.md; golden
 * s7_scrollbar_active.png (mint-004). */
#define FLAIR_CHROME_TRACK_DARK_R       150
#define FLAIR_CHROME_TRACK_DARK_G       150
#define FLAIR_CHROME_TRACK_DARK_B       150

/* --- Win 3.1 system metrics (era=win31-accent) ----------------------------- */

/* native.win31_sm_cycaption.value = 18
 * Windows 3.1 SM_CYCAPTION (caption bar interior height, px).
 * VERIFIED from two independent DOSBox-X golden measurements (ProgMan + MDI child).
 * Ref: ../win31-decomp/specs/chrome/system-metrics.md Sec 1 M-A row 1+5. */
#define FLAIR_WIN31_SM_CYCAPTION        18

/* native.win31_sm_cxvscroll.value = 15
 * Windows 3.1 SM_CXVSCROLL (vertical scrollbar width, px).
 * LOW CONFIDENCE: ~15 px from 16-color golden; 17px web claim REFUTED.
 * Definitive value needs 256-color re-mint.
 * Ref: ../win31-decomp/specs/chrome/system-metrics.md Sec 1 + Sec 3. */
#define FLAIR_WIN31_SM_CXVSCROLL        15

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

/* The 19-px title BAND decomposes EXACTLY as top-frame(1) + bevel-hi(1) + 15
 * stripe + bevel-lo(1) + shared-frame(1) (window-frame.md Sec 2a; beads
 * initech-92li).  If the decomposition ever stops summing to TITLEBAR_H a band
 * row has been mis-counted -- the recomposition would over/under-fill the band. */
_Static_assert(2 * FLAIR_CHROME_FRAME + 2 * FLAIR_CHROME_TITLE_BEVEL_ROWS +
                   FLAIR_CHROME_TITLE_STRIPE_ROWS == FLAIR_CHROME_TITLEBAR_H,
               "title band = top-frame + bevel-hi + 15 stripe + bevel-lo + "
               "shared-frame must sum to the locked 19-px TITLEBAR_H "
               "(window-frame.md Sec 2a; pinstripe.md interior=15)");

/* The golden pinstripe interior is exactly 15 rows (FG_TITLE_INTERIOR_ROWS;
 * pinstripe.md y=166..180).  A drift here would un-sync the render from the
 * locked fidelity golden FG_TITLE_INTERIOR_PATTERN "LLDLDLDLDLDLDLL". */
_Static_assert(FLAIR_CHROME_TITLE_STRIPE_ROWS == 15,
               "the title pinstripe interior is exactly 15 rows "
               "(pinstripe.md golden y=166..180; FG_TITLE_INTERIOR_ROWS)");

/* The close/zoom box (13 px derivation) must fit inside the title bar (19 px) with
 * room to center it (WBoxDelta = (titleH-13)/2 must be >= 0). */
_Static_assert(FLAIR_CHROME_WBOX_DELTA <= FLAIR_CHROME_TITLEBAR_H,
               "the 13 px close/zoom box derivation must fit in the 19 px title bar");

/* The RENDERED gadget (11 px) is smaller than the WDEF derivation (13 px) -- the
 * bevel/InsetRect pulls the visible box in by 2 px (close-zoom-box.md Geometry).
 * It is positive, square, and must fit in the title bar with the wBoxDelta gap. */
_Static_assert(FLAIR_CHROME_WBOX_RENDER > 0 &&
               FLAIR_CHROME_WBOX_RENDER < FLAIR_CHROME_WBOX_DELTA,
               "the rendered close/zoom gadget is 11 px -- smaller than the 13 px "
               "WDEF derivation (the bevel sits inside the dark frame; "
               "close-zoom-box.md golden s7_doc_window.png 11x11)");

/* The rendered box plus the wBoxDelta gap above and the +1 dest-top offset must
 * leave the gadget inside the title bar: wBoxDelta+1 + 11 <= titleH (3+1+11=15<=19).
 * (box top = struct.top + (titleH-13)/2 + 1; close-zoom-box.md WDEF @1705-1707.) */
_Static_assert(((FLAIR_CHROME_TITLEBAR_H - FLAIR_CHROME_WBOX_DELTA) / 2) + 1 +
               FLAIR_CHROME_WBOX_RENDER <= FLAIR_CHROME_TITLEBAR_H,
               "rendered box (11) + wBoxDelta gap + 1 must fit in the 19 px title bar");

/* The scrollbar and grow box are both the 16 px scrollBarSize family. */
_Static_assert(FLAIR_CHROME_SCROLLBAR_W == FLAIR_CHROME_GROW,
               "scrollbar width and grow box both derive from scrollBarSize=16");

/* --- RGB color consistency asserts (P1-4 era-tagged constants) ------------ */

/* The pinstripe bevel top and the box bevel highlight are the SAME wLTinge0
 * family value (#DADAFF = 218,218,255). If they ever diverge a Law-3 error
 * has been introduced -- they map to the same WDEF shade. */
_Static_assert(FLAIR_CHROME_BEVEL_TOP_R == FLAIR_CHROME_BOX_BEVEL_R &&
               FLAIR_CHROME_BEVEL_TOP_G == FLAIR_CHROME_BOX_BEVEL_G &&
               FLAIR_CHROME_BEVEL_TOP_B == FLAIR_CHROME_BOX_BEVEL_B,
               "bevel top (#DADAFF) and box bevel highlight must match (both wLTinge0)");

/* The bevel top (#DADAFF) and grow box highlight (#DADAFF) must also match. */
_Static_assert(FLAIR_CHROME_BEVEL_TOP_R == FLAIR_CHROME_GROW_HIGHLIGHT_R &&
               FLAIR_CHROME_BEVEL_TOP_G == FLAIR_CHROME_GROW_HIGHLIGHT_G &&
               FLAIR_CHROME_BEVEL_TOP_B == FLAIR_CHROME_GROW_HIGHLIGHT_B,
               "bevel top and grow box highlight both derive from wLTinge0 (#DADAFF)");

/* The close/zoom box dark outline (#545487) and grow box figure (#545487) must
 * match -- both are wFrameColor / wDTingeF (the same WDEF shade). */
_Static_assert(FLAIR_CHROME_BOX_DARK_R == FLAIR_CHROME_GROW_FIGURE_R &&
               FLAIR_CHROME_BOX_DARK_G == FLAIR_CHROME_GROW_FIGURE_G &&
               FLAIR_CHROME_BOX_DARK_B == FLAIR_CHROME_GROW_FIGURE_B,
               "box dark outline and grow figure both derive from wFrameColor (#545487)");

/* The box face (#C0C0C0) and grow box inner fill (#C0C0C0) must match
 * (both are wTitleBarShade4 blend). */
_Static_assert(FLAIR_CHROME_BOX_FACE_R == FLAIR_CHROME_GROW_FILL_R &&
               FLAIR_CHROME_BOX_FACE_G == FLAIR_CHROME_GROW_FILL_G &&
               FLAIR_CHROME_BOX_FACE_B == FLAIR_CHROME_GROW_FILL_B,
               "box face and grow box fill both derive from wTitleBarShade4 (#C0C0C0)");

/* The grow box ground (#F3F3F3) and the pinstripe light row (#F3F3F3) must match
 * -- both are wTitleBarShade1 (shade index 22). */
_Static_assert(FLAIR_CHROME_GROW_GROUND_R == FLAIR_CHROME_PINSTRIPE_LIGHT_R &&
               FLAIR_CHROME_GROW_GROUND_G == FLAIR_CHROME_PINSTRIPE_LIGHT_G &&
               FLAIR_CHROME_GROW_GROUND_B == FLAIR_CHROME_PINSTRIPE_LIGHT_B,
               "grow ground and pinstripe light both derive from wTitleBarShade1 (#F3F3F3)");

/* Scrollbar track dark (#969696) and pinstripe dark (#969696) must match --
 * both derive from the wTitleBarDark family. */
_Static_assert(FLAIR_CHROME_TRACK_DARK_R == FLAIR_CHROME_PINSTRIPE_DARK_R &&
               FLAIR_CHROME_TRACK_DARK_G == FLAIR_CHROME_PINSTRIPE_DARK_G &&
               FLAIR_CHROME_TRACK_DARK_B == FLAIR_CHROME_PINSTRIPE_DARK_B,
               "scrollbar track dark and pinstripe dark both derive from wTitleBarDark (#969696)");

/* The pressed-interior lavender (#B3B3DA) must differ from the bevel top
 * lavender (#DADAFF): they are distinct WDEF shade blends and must not merge. */
_Static_assert(!(FLAIR_CHROME_BOX_PRESSED_R == FLAIR_CHROME_BEVEL_TOP_R &&
                 FLAIR_CHROME_BOX_PRESSED_G == FLAIR_CHROME_BEVEL_TOP_G &&
                 FLAIR_CHROME_BOX_PRESSED_B == FLAIR_CHROME_BEVEL_TOP_B),
               "pressed interior (#B3B3DA) and bevel top (#DADAFF) must be distinct shades");

/* The thumb light (#DADAFF) and the bevel top (#DADAFF) are the same wLTinge0
 * family -- confirm they remain equal. */
_Static_assert(FLAIR_CHROME_THUMB_LIGHT_R == FLAIR_CHROME_BEVEL_TOP_R &&
               FLAIR_CHROME_THUMB_LIGHT_G == FLAIR_CHROME_BEVEL_TOP_G &&
               FLAIR_CHROME_THUMB_LIGHT_B == FLAIR_CHROME_BEVEL_TOP_B,
               "scrollbar thumb light (#DADAFF) and bevel top (#DADAFF) must match (wLTinge0)");

/* Win31 SM_CYCAPTION must be a positive value > 0 and less than the system
 * menubar height (it is a caption, not a full screen). */
_Static_assert(FLAIR_WIN31_SM_CYCAPTION > 0 && FLAIR_WIN31_SM_CYCAPTION < 640,
               "win31 SM_CYCAPTION must be a sane positive pixel count");

/* Win31 SM_CXVSCROLL must be a positive non-zero width. */
_Static_assert(FLAIR_WIN31_SM_CXVSCROLL > 0,
               "win31 SM_CXVSCROLL must be a positive pixel width");

#endif /* INITECH_SPEC_CHROME_METRICS_H */
