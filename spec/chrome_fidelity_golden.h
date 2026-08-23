/*
 * spec/chrome_fidelity_golden.h -- INDEPENDENT Mac OS 8.1 Platinum golden.
 *
 * Route-2 re-key: ADR-0004-AMENDMENT-DEC-10 Sec 4 / OQ-7, beads
 * initech-3knt.  Values are pixel-measured in ../system7-decomp and are never
 * derived from chrome_metrics.h, which the renderer consumes (Law 2 / HER-02).
 * Every Platinum color class is in the SAMPLED framebuffer domain established
 * by sys8/platinum-palette.md Sec 1 and Sec 2.
 *
 * The former System-7 golden remains compiled below under FG_SYS7_* names
 * (BC-10.10); no era datum is deleted or hidden behind a disabled block.
 */
#ifndef INITECH_SPEC_CHROME_FIDELITY_GOLDEN_H
#define INITECH_SPEC_CHROME_FIDELITY_GOLDEN_H

#include <stdint.h>

/* -------------------------------------------------------------------------
 * PLATINUM TITLE BAND.
 * Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 2.1 and Sec 2.2.
 * Legend: K=black frame, H=white highlight, F=frame face, L=white stripe,
 * D=dark stripe, S=frame shadow.  The 12 stripe rows start L and end D;
 * face rows are asymmetric (2 above, 4 below).
 */
#define FG_TITLE_BAND_ROWS       22
#define FG_TITLE_BAND_PROFILE    "KHFFLDLDLDLDLDLDFFFFSK"
#define FG_TITLE_STRIPE_TOP_OFF   4
#define FG_TITLE_STRIPE_ROWS     12
/* Equal-length sampled field: light [L+21,R-38], dark shifted +1 at both ends.
 * Half-open consumer arithmetic uses RIGHT_OFF against frame.right. */
#define FG_TITLE_STRIPE_LEFT_OFF 21
#define FG_TITLE_STRIPE_RIGHT_OFF 38
#define FG_TITLE_STRIPE_DARK_SHIFT 1
#define FG_TITLE_FACE_ABOVE       2
#define FG_TITLE_FACE_BELOW       4
#define FG_FRAME_IDX              0
#define FG_WHITE_IDX              1
#define FG_TITLE_INK_IDX          4
#define FG_TITLE_FRAME_FACE_IDX 218
#define FG_TITLE_STRIPE_LIGHT_IDX FG_WHITE_IDX
#define FG_TITLE_STRIPE_DARK_IDX 150
#define FG_TITLE_FRAME_SHADOW_IDX 179

/* Active title text is centered black; the stripe knockout is frame-face.
 * Ref: window-chrome.md Sec 2.3. */
#define FG_TITLE_CENTERED          1
#define FG_TITLE_KNOCKOUT_IDX    218
#define FG_TITLE_GAP_PAD_LEFT       6
#define FG_TITLE_GAP_PAD_RIGHT      5
#define FG_TITLE_DARK_GAP_SHIFT     1
#define FG_TITLE_TEXT_TOP_OFF       4
#define FG_TITLE_RUN_LEFT_OFF      21
#define FG_TITLE_RUN_RIGHT_OFF     38
#define FG_TITLE_FIXED_TEXT       "TITLE"
#define FG_TITLE_TRUNC_SOURCE     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define FG_TITLE_TRUNC_EXPECTED   "ABCDEFG."

/* -------------------------------------------------------------------------
 * PLATINUM WIDGETS.
 * Placement and 12+1 footprint: window-chrome.md Sec 3.1.
 * Anatomy and diagonal ramp: Sec 3.2. Glyphs: Sec 3.3.
 */
#define FG_WIDGET_COUNT_ACTIVE        3
#define FG_BOX_RENDER_SIZE           12
#define FG_BOX_OUTER_HIGHLIGHT        1
#define FG_BOX_FOOTPRINT             13
#define FG_CLOSE_BOX_LEFT_OFF         4
#define FG_ZOOM_BOX_RIGHT_OFF        32
#define FG_COLLAPSE_BOX_RIGHT_OFF    16
#define FG_BOX_TOP_OFF                4
#define FG_BOX_EDGE_IDX             165
#define FG_BOX_RING_IDX              63
#define FG_BOX_OUTER_HIGHLIGHT_IDX    1
#define FG_BOX_INTERIOR_SIZE          9
#define FG_BOX_RAMP_FACE_SIZE         7
#define FG_BOX_RAMP_RUNGS             7
#define FG_BOX_RAMP_PX_PER_STEP       2

/* Exact sampled rung values from window-chrome.md Sec 3.2.  The INDEX-class
 * array uses FLAIR's existing white role (idx 1) for sampled rung 255, as
 * required by the policy seam; it resolves to the same sampled white. */
static const uint8_t FG_BOX_RAMP_SAMPLED[FG_BOX_RAMP_RUNGS] = {
    179, 192, 205, 218, 231, 243, 255
};
static const uint8_t FG_BOX_RAMP_IDX[FG_BOX_RAMP_RUNGS] = {
    179, 192, 205, 218, 231, 243, FG_WHITE_IDX
};

/* Close has no glyph. Zoom is a six-pixel right+bottom edge. Collapse has two
 * full-interior rows and occupies the rightmost slot.
 * Ref: window-chrome.md Sec 3.3. */
#define FG_CLOSE_HAS_GLYPH            0
#define FG_ZOOM_GLYPH_EDGE            6
#define FG_COLLAPSE_GLYPH_ROW_0       3
#define FG_COLLAPSE_GLYPH_ROW_1       5
#define FG_COLLAPSE_RIGHTMOST         1

/* -------------------------------------------------------------------------
 * PLATINUM BODY FRAME + CONTENT INSET.
 * Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 4.
 */
#define FG_BODY_BAR_ROWS              4
static const uint8_t FG_BODY_BAR_IDX[FG_BODY_BAR_ROWS] = {
    FG_WHITE_IDX, 218, 218, 179
};
#define FG_BODY_INNER_LINE_IDX        0
#define FG_BODY_HAS_RAISED_BAR        1
#define FG_CONTENT_INSET              1
#define FG_CONTENT_INSET_HI_IDX       1
#define FG_CONTENT_INSET_SHADOW_IDX 192

/* -------------------------------------------------------------------------
 * PLATINUM DROP SHADOW.
 * Active black, inactive frame-gray, (+1,+1), two-pixel near-corner notch.
 * Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 1.
 */
#define FG_SHADOW_INK_IDX             0
#define FG_SHADOW_OFFSET              1
#define FG_SHADOW_NOTCH               2
#define FG_INACTIVE_SHADOW_IDX      119

/* -------------------------------------------------------------------------
 * PLATINUM SCROLLBARS.
 * Geometry: scrollbars.md Sec 1. ENABLED: Sec 2. DISABLED: Sec 3.
 * HOLLOW: Sec 4. State summary: Sec 5.
 *
 * Scene choice: flair_draw_document_window has no content-range input and the
 * active skeleton window requests no thumb, matching the DISABLED Finder
 * capture s8_doc_window_active (Sec 3). The hilited=0 skeleton window matches
 * HOLLOW s8_doc_window_inactive (Sec 4). ENABLED constants remain locked for
 * the later stateful control leg but are not guessed into this scene.
 */
#define FG_SB_BAND                    16
#define FG_SB_INTERIOR                14
#define FG_SB_ARROW_TILE              16
#define FG_SB_SEPARATOR_ROWS           1
#define FG_SB_THUMB_MIN               15
/* The gutter outer-top line coincides with title row T+21; T+22 is trough. */
#define FG_SB_TOP_OFF                 (FG_TITLE_BAND_ROWS - 1)
#define FG_SB_AFTER_TOP_IDX           243

#define FG_SB_DISABLED_TROUGH_IDX    243
#define FG_SB_DISABLED_ARROW_IDX     165
#define FG_SB_DISABLED_SEPARATOR_IDX 119
#define FG_SB_DISABLED_NO_THUMB        1

#define FG_SB_ENABLED_FRAME_IDX        0
#define FG_SB_ENABLED_TILE_FACE_IDX  231
#define FG_SB_ENABLED_TILE_HI_IDX      1
#define FG_SB_ENABLED_TILE_SHADOW_IDX 205
#define FG_SB_ENABLED_GLYPH_IDX        0
#define FG_SB_ENABLED_WELL_IDX       192
#define FG_SB_ENABLED_WELL_SHADOW0_IDX 150
#define FG_SB_ENABLED_WELL_SHADOW1_IDX 165
#define FG_SB_ENABLED_THUMB_LIGHT_IDX  2
#define FG_SB_ENABLED_THUMB_SHADOW_IDX 4

#define FG_SB_HOLLOW_TROUGH_IDX      243
#define FG_SB_HOLLOW_FRAME_IDX       119
#define FG_SB_HOLLOW_NOTHING_ELSE      1

/* -------------------------------------------------------------------------
 * PLATINUM INACTIVE DELTA.
 * Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 6 and
 * scrollbars.md Sec 4.  Explicit era change: System-7 inactive title fill was
 * white; Platinum is sampled face index 231.
 */
#define FG_INACTIVE_TITLE_FILL_IDX   231
#define FG_INACTIVE_FRAME_IDX        119
#define FG_INACTIVE_TEXT_IDX         135
#define FG_INACTIVE_NO_STRIPES         1
#define FG_INACTIVE_NO_BEVEL           1
#define FG_INACTIVE_NO_GADGETS         1
#define FG_INACTIVE_GROW_FILL_IDX     231

/* -------------------------------------------------------------------------
 * PLATINUM GROW BOX.
 * Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 5.
 */
#define FG_GROW_CELL                  18
#define FG_GROW_FILL_IDX             218
#define FG_GROW_HIGHLIGHT_IDX          1
#define FG_GROW_GRIP_LINES             3
#define FG_GROW_GRIP_PITCH             4
#define FG_GROW_GRIP_LEAD_IDX           1
#define FG_GROW_GRIP_TRAIL_IDX        150
#define FG_GROW_GRIP_TERMINATOR_IDX   192
#define FG_GROW_ACTIVE_ONLY             1

/* Exact sampled x=461..478, y=296..310 cell profile. W=white lead/highlight,
 * d=frame-face fill, g=dark trail, c=well-color terminator.
 * Ref: window-chrome.md Sec 5, machine transcription below the metric table. */
#define FG_GROW_PROFILE_ROWS           15
#define FG_GROW_PROFILE_COLS           18
#define FG_GROW_PROFILE \
    "WWWWWWWWWWWWWWWWdd" \
    "Wddddddddddddddddd" \
    "Wddddddddddddddddd" \
    "WdddddddWWdddddddd" \
    "WddddddWdgdddddddd" \
    "WdddddWdgdWWdddddd" \
    "WddddWdgdWdgdddddd" \
    "WdddWdgdWdgdWWdddd" \
    "WddWdgdWdgdWdgdddd" \
    "WddcgdWdgdWdgddddd" \
    "WddddWdgdWdgdddddd" \
    "WddddcgdWdgddddddd" \
    "WddddddWdgdddddddd" \
    "Wddddddcgddddddddd" \
    "Wddddddddddddddddd"

/* =========================================================================
 * RETAINED SYSTEM-7 GOLDEN (BC-10.10).
 * ========================================================================= */
/* Stripe classes, exact 15-row phase signature, and counts.
 * Original citation: ../system7-decomp/specs/chrome/pinstripe.md Geometry,
 * s7_doc_window.png x=450 y=166..180, plus StandardWDEF_a.txt @77-78 and
 * patAlign @891-894. */
#define FG_SYS7_STRIPE_LIGHT_IDX             7
#define FG_SYS7_STRIPE_DARK_IDX              8
#define FG_SYS7_TITLE_INTERIOR_PATTERN       "LLDLDLDLDLDLDLL"
#define FG_SYS7_TITLE_INTERIOR_ROWS          15
#define FG_SYS7_TITLE_INTERIOR_LIGHT_N        9
#define FG_SYS7_TITLE_INTERIOR_DARK_N         6
#define FG_SYS7_PHASE_DOUBLED_LIGHT_AT_EDGES  1

/* Title bevel and shared line.
 * Original citation: ../system7-decomp/specs/chrome/window-frame.md Sec 2a
 * and Sec 2b, and pinstripe.md Geometry. */
#define FG_SYS7_TITLE_BEVEL_HI_IDX            2
#define FG_SYS7_TITLE_BEVEL_LO_IDX            4
#define FG_SYS7_TITLE_SHARED_FRAME_IDX        0

/* Shadow and single-line body.
 * Original citation: ../system7-decomp/specs/chrome/window-frame.md Sec 1,
 * Sec 2a, and Sec 4; StandardWDEF_a.txt L515 and L578-594. */
#define FG_SYS7_SHADOW_INK_IDX                0
#define FG_SYS7_SHADOW_OFFSET                 1
#define FG_SYS7_BODY_NO_GROOVE                1
#define FG_SYS7_BODY_INNER_IDX                1

/* Active title text and knockout.
 * Original citation: ../system7-decomp/specs/chrome/title-bar.md Sec 3 and
 * pinstripe.md, s7_doc_window.png title-gap scan. */
#define FG_SYS7_TITLE_INK_IDX                 4
#define FG_SYS7_TITLE_KNOCKOUT_IDX            7

/* Close/zoom geometry and anatomy.
 * Original citation: ../system7-decomp/specs/chrome/close-zoom-box.md Geometry
 * and Rendered colors; StandardWDEF_a.txt @1675-1707. */
#define FG_SYS7_BOX_RENDER_SIZE              11
#define FG_SYS7_CLOSE_BOX_LEFT_OFF            9
#define FG_SYS7_ZOOM_BOX_RIGHT_OFF           20
#define FG_SYS7_BOX_MIN_TONAL_ROLES           3
#define FG_SYS7_BOX_DARK_IDX                  4
#define FG_SYS7_BOX_BEVEL_IDX                 2
#define FG_SYS7_BOX_FACE_IDX                  6
#define FG_SYS7_ZOOM_HAS_NESTED_GLYPH         1

/* Inactive/no-thumb scrollbar classes and arrow minimum.
 * Original citation: ../system7-decomp/specs/chrome/scrollbar.md Geometry,
 * Rendered colors, and arrow-glyph shape; StandardWDEF_a.txt @73/@1310-1338. */
#define FG_SYS7_SB_OUTER_EDGE_IDX             0
#define FG_SYS7_SB_SEPARATOR_IDX              8
#define FG_SYS7_SB_FACE_IDX                   7
#define FG_SYS7_SB_GLYPH_IDX                  8
#define FG_SYS7_SB_GLYPH_MIN_PX               8
#define FG_SYS7_SB_INACTIVE_NO_THUMB           1

/* Inactive title delta and gadget absence.
 * Original citation: ../system7-decomp/specs/chrome/title-bar.md Sec 2 and
 * Sec 2.1, plus close-zoom-box.md Mechanism. */
#define FG_SYS7_INACTIVE_TITLE_FILL_IDX        1
#define FG_SYS7_HILITE_FRAME_IDX             119
#define FG_SYS7_HILITE_TEXT_IDX              165
#define FG_SYS7_INACTIVE_NO_GADGETS            1

#endif /* INITECH_SPEC_CHROME_FIDELITY_GOLDEN_H */
