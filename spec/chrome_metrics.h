/*
 * spec/chrome_metrics.h -- C-consumable Platinum chrome metrics.
 *
 * Route-2 base re-key: ADR-0004-AMENDMENT-DEC-10 Sec 3.6.1 / Sec 4,
 * OQ-7.  Unqualified FLAIR_CHROME_* names in the re-keyed window-chrome
 * surface describe Mac OS 8.1 Platinum.  System-7 values remain compiled
 * under FLAIR_CHROME_SYS7_* (BC-10.10); explicitly out-of-scope legacy
 * metrics retain compatibility aliases below.
 *
 * Value domain: SAMPLED framebuffer values only.  Ref:
 * ../system7-decomp/specs/sys8/platinum-palette.md Sec 1 (gamma table) and
 * Sec 2 (sampled gray ramp).  Nominal values in the source corpus are
 * provenance only and never enter these constants.
 *
 * The locked JSON mirror is spec/chrome_metrics.json.  test-chrome STEP-1
 * compares the legacy consumer-facing geometry names with its native entries.
 * The independent fidelity expectations live in chrome_fidelity_golden.h and
 * are never derived from this file (CLAUDE.md Law 2 / HER-02).
 */
#ifndef INITECH_SPEC_CHROME_METRICS_H
#define INITECH_SPEC_CHROME_METRICS_H

/* Menu height is unchanged by this chrome re-key.
 * Ref: spec/chrome_metrics.json native.menubar_height (retained metric). */
#define FLAIR_CHROME_MENUBAR_H 20

/* Platinum title band, frame-to-frame inclusive.
 * Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 2.1. */
#define FLAIR_CHROME_TITLEBAR_H             22
#define FLAIR_CHROME_TITLE_FRAME_TOP_ROWS    1
#define FLAIR_CHROME_TITLE_HIGHLIGHT_ROWS    1
#define FLAIR_CHROME_TITLE_FACE_ABOVE        2
#define FLAIR_CHROME_TITLE_STRIPE_TOP_OFF    4
#define FLAIR_CHROME_TITLE_BAND_STRIPE_ROWS 12
#define FLAIR_CHROME_TITLE_FACE_BELOW        4
#define FLAIR_CHROME_TITLE_SHADOW_ROWS       1
#define FLAIR_CHROME_TITLE_FRAME_BOTTOM_ROWS 1

/* Platinum widgets: 12x12 dark box plus white right/bottom outer highlight.
 * R is the inclusive structure-right coordinate in the measured rule.
 * Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 3.1. */
#define FLAIR_CHROME_WIDGET_COUNT              3
#define FLAIR_CHROME_WIDGET_BOX               12
#define FLAIR_CHROME_WIDGET_OUTER_HIGHLIGHT    1
#define FLAIR_CHROME_WIDGET_FOOTPRINT         13
#define FLAIR_CHROME_WIDGET_TOP_OFF            4
#define FLAIR_CHROME_CLOSE_LEFT_OFF            4
#define FLAIR_CHROME_ZOOM_RIGHT_OFF           32
#define FLAIR_CHROME_COLLAPSE_RIGHT_OFF       16

/* Widget interior and glyph geometry.
 * Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 3.2 and Sec 3.3. */
#define FLAIR_CHROME_WIDGET_INTERIOR           9
#define FLAIR_CHROME_WIDGET_RAMP_FACE          7
#define FLAIR_CHROME_WIDGET_RAMP_RUNGS         7
#define FLAIR_CHROME_WIDGET_RAMP_PX_PER_STEP   2
#define FLAIR_CHROME_ZOOM_GLYPH_EDGE           6
#define FLAIR_CHROME_COLLAPSE_GLYPH_ROW_0      3
#define FLAIR_CHROME_COLLAPSE_GLYPH_ROW_1      5

/* One-pixel outer/inner black lines enclose a four-pixel raised body bar.
 * The content inset is one pixel on each side.
 * Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 4. */
#define FLAIR_CHROME_FRAME                  1
#define FLAIR_CHROME_BODY_BAR               4
#define FLAIR_CHROME_BODY_BAR_FACE_ROWS     2
#define FLAIR_CHROME_CONTENT_INSET           1

/* Platinum shadow: one-pixel black ink at (+1,+1), beginning two pixels from
 * each near corner.  Ref: window-chrome.md Sec 1. */
#define FLAIR_CHROME_SHADOW_OFFSET          1
#define FLAIR_CHROME_SHADOW_NOTCH           2

/* Platinum scrollbar metrics.  The tile is 16 px line-to-line and has a
 * 14x14 interior; separators are one pixel and the measured minimum thumb is
 * 15 px.  Ref: ../system7-decomp/specs/sys8/scrollbars.md Sec 1. */
#define FLAIR_CHROME_SCROLLBAR_W            16
#define FLAIR_CHROME_SCROLL_INTERIOR        14
#define FLAIR_CHROME_SCROLL_ARROW_TILE      16
#define FLAIR_CHROME_SCROLL_SEPARATOR        1
#define FLAIR_CHROME_SCROLL_THUMB_MIN       15

/* Enabled well cross-section: two dark rows, ten fill rows, two light rows.
 * Ref: ../system7-decomp/specs/sys8/scrollbars.md Sec 2.3. */
#define FLAIR_CHROME_SCROLL_WELL_SHADOW_ROWS 2
#define FLAIR_CHROME_SCROLL_WELL_FILL_ROWS  10
#define FLAIR_CHROME_SCROLL_WELL_LIGHT_ROWS  2

/* Grow cell and grip.
 * Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 5. */
#define FLAIR_CHROME_GROW                   18
#define FLAIR_CHROME_GROW_GRIP_LINES         3
#define FLAIR_CHROME_GROW_GRIP_PITCH         4

/* Retained non-Platinum variant metrics.
 * Ref: the retained System-7 sources recorded in chrome_metrics.json. */
#define FLAIR_CHROME_DIALOG_BORDER           7
#define FLAIR_CHROME_SMALL_ICON             20
#define FLAIR_WIN31_SM_CYCAPTION            18
#define FLAIR_WIN31_SM_CXVSCROLL            15

/* -------------------------------------------------------------------------
 * SYSTEM-7 RETAINED GEOMETRY (BC-10.10).
 * Ref: ../system7-decomp/specs/chrome/window-frame.md Sec 2a,
 * pinstripe.md Geometry, close-zoom-box.md Geometry, grow-box.md Geometry,
 * and scrollbar.md Geometry.
 */
#define FLAIR_CHROME_SYS7_TITLEBAR_H          19
#define FLAIR_CHROME_SYS7_TITLE_BEVEL_ROWS     1
#define FLAIR_CHROME_SYS7_TITLE_STRIPE_ROWS   15
#define FLAIR_CHROME_SYS7_MENUBAR_H            20
#define FLAIR_CHROME_SYS7_SCROLLBAR_W          16
#define FLAIR_CHROME_SYS7_FRAME                 1
#define FLAIR_CHROME_SYS7_DIALOG_BORDER         7
#define FLAIR_CHROME_SYS7_WBOX_DELTA          13
#define FLAIR_CHROME_SYS7_WBOX_RENDER         11
#define FLAIR_CHROME_SYS7_PINSTRIPE_PERIOD     2
#define FLAIR_CHROME_SYS7_TITLE_SHADE_LIGHT    7
#define FLAIR_CHROME_SYS7_TITLE_SHADE_DARK     8
#define FLAIR_CHROME_SYS7_GROW                 16
#define FLAIR_CHROME_SYS7_SMALL_ICON           20
#define FLAIR_CHROME_SYS7_SHADOW_OFFSET         1
#define FLAIR_CHROME_SYS7_SHADOW_NOTCH          0

/* Retained compatibility seam: these legacy spellings preserve compiled
 * System-7 heritage consumers and their old row/box algorithm. chrome.c uses
 * the Platinum profile constants above; these aliases carry no BASE meaning.
 * Ref: ADR-0004-AMENDMENT-DEC-10 Sec 3.6.1 Route 2 / OQ-7. */
#define FLAIR_CHROME_TITLE_BEVEL_ROWS   1
#define FLAIR_CHROME_TITLE_STRIPE_ROWS 15
#define FLAIR_CHROME_WBOX_DELTA        13
#define FLAIR_CHROME_WBOX_RENDER       11
#define FLAIR_CHROME_PINSTRIPE_PERIOD   2
#define FLAIR_CHROME_TITLE_SHADE_LIGHT  1
#define FLAIR_CHROME_TITLE_SHADE_DARK 150

/* -------------------------------------------------------------------------
 * PLATINUM SAMPLED RGB BLOCKS.
 * Every component is from platinum-palette.md Sec 1 (SAMPLED domain) and Sec 2
 * (role table), cross-referenced by window-chrome.md Sec 2.1/2.2/3.2/4/6.
 */
#define FLAIR_CHROME_PINSTRIPE_LIGHT_R 255
#define FLAIR_CHROME_PINSTRIPE_LIGHT_G 255
#define FLAIR_CHROME_PINSTRIPE_LIGHT_B 255

#define FLAIR_CHROME_PINSTRIPE_DARK_R 150
#define FLAIR_CHROME_PINSTRIPE_DARK_G 150
#define FLAIR_CHROME_PINSTRIPE_DARK_B 150

#define FLAIR_CHROME_FRAME_FACE_R 218
#define FLAIR_CHROME_FRAME_FACE_G 218
#define FLAIR_CHROME_FRAME_FACE_B 218

#define FLAIR_CHROME_FRAME_SHADOW_R 179
#define FLAIR_CHROME_FRAME_SHADOW_G 179
#define FLAIR_CHROME_FRAME_SHADOW_B 179

#define FLAIR_CHROME_WIDGET_EDGE_R 165
#define FLAIR_CHROME_WIDGET_EDGE_G 165
#define FLAIR_CHROME_WIDGET_EDGE_B 165

#define FLAIR_CHROME_WIDGET_RING_R 63
#define FLAIR_CHROME_WIDGET_RING_G 63
#define FLAIR_CHROME_WIDGET_RING_B 63

#define FLAIR_CHROME_INACTIVE_FRAME_R 119
#define FLAIR_CHROME_INACTIVE_FRAME_G 119
#define FLAIR_CHROME_INACTIVE_FRAME_B 119

#define FLAIR_CHROME_INACTIVE_FILL_R 231
#define FLAIR_CHROME_INACTIVE_FILL_G 231
#define FLAIR_CHROME_INACTIVE_FILL_B 231

#define FLAIR_CHROME_INACTIVE_INK_R 135
#define FLAIR_CHROME_INACTIVE_INK_G 135
#define FLAIR_CHROME_INACTIVE_INK_B 135

/* -------------------------------------------------------------------------
 * SYSTEM-7 RETAINED RGB BLOCKS (BC-10.10).
 * These remain compiled and era-visible with their original source groups.
 */
/* Original citation: ../system7-decomp/specs/chrome/pinstripe.md Rendered
 * colors, s7_doc_window.png x=450 y=165..181, era-stable mint-results-005. */
#define FLAIR_CHROME_SYS7_PINSTRIPE_LIGHT_R 243
#define FLAIR_CHROME_SYS7_PINSTRIPE_LIGHT_G 243
#define FLAIR_CHROME_SYS7_PINSTRIPE_LIGHT_B 243
#define FLAIR_CHROME_SYS7_PINSTRIPE_DARK_R 150
#define FLAIR_CHROME_SYS7_PINSTRIPE_DARK_G 150
#define FLAIR_CHROME_SYS7_PINSTRIPE_DARK_B 150
#define FLAIR_CHROME_SYS7_BEVEL_TOP_R 218
#define FLAIR_CHROME_SYS7_BEVEL_TOP_G 218
#define FLAIR_CHROME_SYS7_BEVEL_TOP_B 255
#define FLAIR_CHROME_SYS7_BEVEL_BOTTOM_R 179
#define FLAIR_CHROME_SYS7_BEVEL_BOTTOM_G 179
#define FLAIR_CHROME_SYS7_BEVEL_BOTTOM_B 218

/* Original citation: ../system7-decomp/specs/chrome/close-zoom-box.md
 * Rendered colors, s7_doc_window.png and the mint-004 pressed captures. */
#define FLAIR_CHROME_SYS7_BOX_DARK_R 84
#define FLAIR_CHROME_SYS7_BOX_DARK_G 84
#define FLAIR_CHROME_SYS7_BOX_DARK_B 135
#define FLAIR_CHROME_SYS7_BOX_BEVEL_R 218
#define FLAIR_CHROME_SYS7_BOX_BEVEL_G 218
#define FLAIR_CHROME_SYS7_BOX_BEVEL_B 255
#define FLAIR_CHROME_SYS7_BOX_FACE_R 192
#define FLAIR_CHROME_SYS7_BOX_FACE_G 192
#define FLAIR_CHROME_SYS7_BOX_FACE_B 192
#define FLAIR_CHROME_SYS7_BOX_PRESSED_R 179
#define FLAIR_CHROME_SYS7_BOX_PRESSED_G 179
#define FLAIR_CHROME_SYS7_BOX_PRESSED_B 218

/* Original citation: ../system7-decomp/specs/chrome/grow-box.md Rendered
 * colors, s7_growbox.png (mint-004), era-stable mint-results-005. */
#define FLAIR_CHROME_SYS7_GROW_GROUND_R 243
#define FLAIR_CHROME_SYS7_GROW_GROUND_G 243
#define FLAIR_CHROME_SYS7_GROW_GROUND_B 243
#define FLAIR_CHROME_SYS7_GROW_FIGURE_R 84
#define FLAIR_CHROME_SYS7_GROW_FIGURE_G 84
#define FLAIR_CHROME_SYS7_GROW_FIGURE_B 135
#define FLAIR_CHROME_SYS7_GROW_HIGHLIGHT_R 218
#define FLAIR_CHROME_SYS7_GROW_HIGHLIGHT_G 218
#define FLAIR_CHROME_SYS7_GROW_HIGHLIGHT_B 255
#define FLAIR_CHROME_SYS7_GROW_FILL_R 192
#define FLAIR_CHROME_SYS7_GROW_FILL_G 192
#define FLAIR_CHROME_SYS7_GROW_FILL_B 192

/* Original citation: ../system7-decomp/specs/chrome/scrollbar.md Rendered
 * colors, s7_scrollbar_active.png (mint-004). */
#define FLAIR_CHROME_SYS7_THUMB_LIGHT_R 218
#define FLAIR_CHROME_SYS7_THUMB_LIGHT_G 218
#define FLAIR_CHROME_SYS7_THUMB_LIGHT_B 255
#define FLAIR_CHROME_SYS7_THUMB_DARK_R 135
#define FLAIR_CHROME_SYS7_THUMB_DARK_G 135
#define FLAIR_CHROME_SYS7_THUMB_DARK_B 179
#define FLAIR_CHROME_SYS7_TRACK_LIGHT_R 231
#define FLAIR_CHROME_SYS7_TRACK_LIGHT_G 231
#define FLAIR_CHROME_SYS7_TRACK_LIGHT_B 231
#define FLAIR_CHROME_SYS7_TRACK_DARK_R 150
#define FLAIR_CHROME_SYS7_TRACK_DARK_G 150
#define FLAIR_CHROME_SYS7_TRACK_DARK_B 150

#include <stdint.h>

_Static_assert(FLAIR_CHROME_TITLE_FRAME_TOP_ROWS +
               FLAIR_CHROME_TITLE_HIGHLIGHT_ROWS +
               FLAIR_CHROME_TITLE_FACE_ABOVE +
               FLAIR_CHROME_TITLE_BAND_STRIPE_ROWS +
               FLAIR_CHROME_TITLE_FACE_BELOW +
               FLAIR_CHROME_TITLE_SHADOW_ROWS +
               FLAIR_CHROME_TITLE_FRAME_BOTTOM_ROWS == FLAIR_CHROME_TITLEBAR_H,
               "Platinum title profile must sum to 22 rows (window-chrome.md Sec 2.1)");
_Static_assert(FLAIR_CHROME_TITLE_STRIPE_TOP_OFF ==
               FLAIR_CHROME_TITLE_FRAME_TOP_ROWS +
               FLAIR_CHROME_TITLE_HIGHLIGHT_ROWS +
               FLAIR_CHROME_TITLE_FACE_ABOVE,
               "Platinum stripes start at T+4 (window-chrome.md Sec 2.1)");
_Static_assert(FLAIR_CHROME_WIDGET_BOX + FLAIR_CHROME_WIDGET_OUTER_HIGHLIGHT ==
               FLAIR_CHROME_WIDGET_FOOTPRINT,
               "Platinum widget footprint is 12+1=13 (window-chrome.md Sec 3.1)");
_Static_assert(FLAIR_CHROME_SCROLL_INTERIOR + 2 * FLAIR_CHROME_FRAME ==
               FLAIR_CHROME_SCROLLBAR_W,
               "Platinum scrollbar is 14px interior plus two lines (scrollbars.md Sec 1)");
_Static_assert(FLAIR_CHROME_SCROLL_WELL_SHADOW_ROWS +
               FLAIR_CHROME_SCROLL_WELL_FILL_ROWS +
               FLAIR_CHROME_SCROLL_WELL_LIGHT_ROWS == FLAIR_CHROME_SCROLL_INTERIOR,
               "Platinum well cross-section is 2+10+2=14 (scrollbars.md Sec 2.3)");
_Static_assert(FLAIR_CHROME_SYS7_TITLEBAR_H == 19 &&
               FLAIR_CHROME_SYS7_TITLE_STRIPE_ROWS == 15,
               "retained System-7 title metrics remain 19/15 (BC-10.10)");
_Static_assert(FLAIR_WIN31_SM_CYCAPTION > 0 && FLAIR_WIN31_SM_CXVSCROLL > 0,
               "retained Win31 accent metrics remain positive");

#endif /* INITECH_SPEC_CHROME_METRICS_H */
