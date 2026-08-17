/*
 * test_chrome_fidelity.c -- Platinum chrome fidelity oracle (B1 RED).
 *
 * This grades the real os/flair/chrome.c render against the independent
 * sys8 golden encoded in spec/chrome_fidelity_golden.h. Expected values never
 * come from chrome_metrics.h (Law 2 / HER-02). The current renderer is still
 * System 7, so B1 intentionally runs RED for the Platinum differences; Phase
 * B2 makes these same legs green.
 *
 * Ground truth: ../system7-decomp/specs/sys8/window-chrome.md Sec 1-6,
 * ../system7-decomp/specs/sys8/scrollbars.md Sec 1-5, and
 * ../system7-decomp/specs/sys8/platinum-palette.md Sec 1-2.
 * Re-key authority: ADR-0004-AMENDMENT-DEC-10 Sec 4 / OQ-7.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "render.h"
#include "chrome.h"
#include "flair_look.h"
#include "chrome_fidelity_golden.h"
#include "test_assert.h"

TEST_HARNESS();

enum {
    WIN_LEFT = 40,
    WIN_TOP = 30,
    WIN_RIGHT = 360,
    WIN_BOTTOM = 300
};

#define TEST_TITLE "untitled"

static rgn_rect_t win_frame(void)
{
    rgn_rect_t r;
    r.top = WIN_TOP;
    r.left = WIN_LEFT;
    r.bottom = WIN_BOTTOM;
    r.right = WIN_RIGHT;
    return r;
}

static void draw_active(GrafPort *port)
{
    flair_draw_document_window(port, flair_look_default_skin(),
                               win_frame(), TEST_TITLE, 1);
}

static void draw_inactive(GrafPort *port)
{
    flair_draw_document_window(port, flair_look_default_skin(),
                               win_frame(), TEST_TITLE, 0);
}

static uint32_t px(const render_ctx_t *ctx, int x, int y)
{
    return render_pixel_index(ctx, (uint32_t)x, (uint32_t)y);
}

static int count_idx(const render_ctx_t *ctx, int x0, int y0, int x1, int y1,
                     uint32_t want)
{
    int n = 0;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            if (px(ctx, x, y) == want) {
                n++;
            }
        }
    }
    return n;
}

static uint32_t title_class_idx(char c)
{
    switch (c) {
    case 'K': return FG_FRAME_IDX;
    case 'H': return FG_WHITE_IDX;
    case 'F': return FG_TITLE_FRAME_FACE_IDX;
    case 'L': return FG_TITLE_STRIPE_LIGHT_IDX;
    case 'D': return FG_TITLE_STRIPE_DARK_IDX;
    case 'S': return FG_TITLE_FRAME_SHADOW_IDX;
    default:  return 0xFFFFFFFFu;
    }
}

static int widget_shell_ok(const render_ctx_t *ctx, int bx, int by)
{
    int ok = 1;
    for (int k = 0; k < FG_BOX_RENDER_SIZE; k++) {
        ok = ok && px(ctx, bx + k, by) == (uint32_t)FG_BOX_EDGE_IDX;
        ok = ok && px(ctx, bx, by + k) == (uint32_t)FG_BOX_EDGE_IDX;
    }
    for (int k = 1; k < FG_BOX_RENDER_SIZE; k++) {
        ok = ok && px(ctx, bx + k, by + 1) == (uint32_t)FG_BOX_RING_IDX;
        ok = ok && px(ctx, bx + 1, by + k) == (uint32_t)FG_BOX_RING_IDX;
        ok = ok && px(ctx, bx + k, by + FG_BOX_RENDER_SIZE - 1) ==
                       (uint32_t)FG_BOX_RING_IDX;
        ok = ok && px(ctx, bx + FG_BOX_RENDER_SIZE - 1, by + k) ==
                       (uint32_t)FG_BOX_RING_IDX;
        ok = ok && px(ctx, bx + FG_BOX_RENDER_SIZE, by + k) ==
                       (uint32_t)FG_BOX_OUTER_HIGHLIGHT_IDX;
        ok = ok && px(ctx, bx + k, by + FG_BOX_RENDER_SIZE) ==
                       (uint32_t)FG_BOX_OUTER_HIGHLIGHT_IDX;
    }
    return ok;
}

static int ramp_ok_at(const render_ctx_t *ctx, int bx, int by)
{
    int ok = FG_BOX_RAMP_SAMPLED[0] == 179 &&
             FG_BOX_RAMP_SAMPLED[1] == 192 &&
             FG_BOX_RAMP_SAMPLED[2] == 205 &&
             FG_BOX_RAMP_SAMPLED[3] == 218 &&
             FG_BOX_RAMP_SAMPLED[4] == 231 &&
             FG_BOX_RAMP_SAMPLED[5] == 243 &&
             FG_BOX_RAMP_SAMPLED[6] == 255;
    for (int dy = 1; dy <= FG_BOX_RAMP_FACE_SIZE; dy++) {
        for (int dx = 1; dx <= FG_BOX_RAMP_FACE_SIZE; dx++) {
            int rung = (dx + dy - 2) / FG_BOX_RAMP_PX_PER_STEP;
            ok = ok && rung >= 0 && rung < FG_BOX_RAMP_RUNGS;
            if (rung >= 0 && rung < FG_BOX_RAMP_RUNGS) {
                ok = ok && px(ctx, bx + 2 + dx, by + 2 + dy) ==
                               (uint32_t)FG_BOX_RAMP_IDX[rung];
            }
        }
    }
    return ok;
}

int main(void)
{
    render_boot_info_t boot;
    render_ctx_t active;
    render_ctx_t inactive;
    memset(&boot, 0, sizeof boot);
    boot.lfb_bpp = 8u;
    boot.lfb_width = 640u;
    boot.lfb_height = 480u;

    int rc = render_ctx_init(&active, &boot);
    CHECK(rc == 0, "active 8bpp render context must initialize");
    if (rc != 0) {
        return TEST_SUMMARY("test-chrome-fidelity");
    }
    render_run(&active, draw_active);

    const int ri = WIN_RIGHT - 1;   /* inclusive structure right */
    const int bi = WIN_BOTTOM - 1;  /* inclusive structure bottom */
    const int pin_x = WIN_LEFT + 20;
    const int mid_x = (WIN_LEFT + WIN_RIGHT) / 2;

    /* TITLE BAND: exact 22-row class profile, including 2/4 face asymmetry.
     * Ref: window-chrome.md Sec 2.1 and Sec 2.2.
     * Existing _PHASE and _BVL mutants bite this exact profile. */
    int band_ok = strlen(FG_TITLE_BAND_PROFILE) == FG_TITLE_BAND_ROWS;
    for (int row = 0; row < FG_TITLE_BAND_ROWS; row++) {
        band_ok = band_ok &&
            px(&active, pin_x, WIN_TOP + row) ==
                title_class_idx(FG_TITLE_BAND_PROFILE[row]);
    }
    CHECK(band_ok,
          "leg BAND: title must match 22-row KHFF+(LDx6)+FFFF+SK Platinum profile; "
          "12 stripes start light/end dark and face rows are 2 above/4 below "
          "(window-chrome.md Sec 2.1/2.2)");

    /* TITLE TEXT: active black centered ink over frame-face gap.
     * Ref: window-chrome.md Sec 2.3. _NO_TITLE bites ink and knockout. */
    int title_ink = count_idx(&active, mid_x - 40, WIN_TOP + 4,
                              mid_x + 40, WIN_TOP + 21,
                              FG_TITLE_INK_IDX);
    int title_gap = count_idx(&active, mid_x - 40, WIN_TOP + 4,
                              mid_x + 40, WIN_TOP + 21,
                              FG_TITLE_KNOCKOUT_IDX);
    int title_ok = title_ink >= 8 && title_gap >= 24;
    CHECK(title_ok,
          "leg TITLE: centered active ink must be black and its knockout gap must "
          "be Platinum frame-face idx218, not the System-7 light-stripe role "
          "(window-chrome.md Sec 2.3)");

    /* SHADOW: (+1,+1) black L with two-pixel near-corner notch.
     * Ref: window-chrome.md Sec 1. _NO_SHADOW and new _NOTCH bite here. */
    int shadow_ok =
        px(&active, WIN_RIGHT, WIN_TOP + FG_SHADOW_NOTCH - 1) ==
            (uint32_t)RENDER_DESKTOP_INDEX &&
        px(&active, WIN_RIGHT, WIN_TOP + FG_SHADOW_NOTCH) ==
            (uint32_t)FG_SHADOW_INK_IDX &&
        px(&active, WIN_RIGHT, (WIN_TOP + WIN_BOTTOM) / 2) ==
            (uint32_t)FG_SHADOW_INK_IDX &&
        px(&active, WIN_LEFT + FG_SHADOW_NOTCH - 1, WIN_BOTTOM) ==
            (uint32_t)RENDER_DESKTOP_INDEX &&
        px(&active, WIN_LEFT + FG_SHADOW_NOTCH, WIN_BOTTOM) ==
            (uint32_t)FG_SHADOW_INK_IDX;
    CHECK(shadow_ok,
          "leg SHADOW/NOTCH: black (+1,+1) shadow must start at top+2/left+2; "
          "System-7 starts one pixel too near and a no-shadow render misses the L "
          "(window-chrome.md Sec 1)");

    /* BODY: outer black, four-pixel raised bar, inner black, content inset.
     * Ref: window-chrome.md Sec 4. New _BODYBAR bites this inversion of the
     * retained FG_SYS7_BODY_NO_GROOVE tell. */
    static const uint8_t left_body[] = { 0, 1, 218, 218, 179, 0, 1 };
    int body_ok = 1;
    const int body_y = 150;
    for (int k = 0; k < (int)(sizeof left_body / sizeof left_body[0]); k++) {
        body_ok = body_ok && px(&active, WIN_LEFT + k, body_y) == left_body[k];
    }
    body_ok = body_ok && px(&active, ri - 6, body_y) == FG_CONTENT_INSET_SHADOW_IDX;
    body_ok = body_ok && px(&active, ri - 5, body_y) == FG_BODY_INNER_LINE_IDX;
    for (int k = 0; k < FG_BODY_BAR_ROWS; k++) {
        body_ok = body_ok && px(&active, ri - 4 + k, body_y) == FG_BODY_BAR_IDX[k];
    }
    body_ok = body_ok && px(&active, ri, body_y) == FG_FRAME_IDX;
    body_ok = body_ok && px(&active, mid_x, bi - 5) == FG_BODY_INNER_LINE_IDX;
    for (int k = 0; k < FG_BODY_BAR_ROWS; k++) {
        body_ok = body_ok && px(&active, mid_x, bi - 4 + k) == FG_BODY_BAR_IDX[k];
    }
    body_ok = body_ok && px(&active, mid_x, bi) == FG_FRAME_IDX;
    body_ok = body_ok && px(&active, mid_x, bi - 6) == FG_CONTENT_INSET_SHADOW_IDX;
    CHECK(body_ok,
          "leg BODYBAR: body must have the four-pixel raised white/218/218/179 bar "
          "between outer and inner black lines plus the one-pixel content inset; "
          "a System-7 single-line body is wrong (window-chrome.md Sec 4)");

    /* WIDGETS: measured inclusive-R offsets. Ref: window-chrome.md Sec 3.1. */
    const int box_y = WIN_TOP + FG_BOX_TOP_OFF;
    const int close_x = WIN_LEFT + FG_CLOSE_BOX_LEFT_OFF;
    const int zoom_x = ri - FG_ZOOM_BOX_RIGHT_OFF;
    const int collapse_x = ri - FG_COLLAPSE_BOX_RIGHT_OFF;
    int widget_ok = widget_shell_ok(&active, close_x, box_y) &&
                    widget_shell_ok(&active, zoom_x, box_y) &&
                    widget_shell_ok(&active, collapse_x, box_y) &&
                    collapse_x > zoom_x;
    CHECK(widget_ok,
          "leg WIDGETS: active chrome must carry three 12x12+highlight widgets at "
          "L+4, R-32, R-16 with collapse rightmost (window-chrome.md Sec 3.1)");

    int widget_ramp_ok = ramp_ok_at(&active, close_x, box_y);
    CHECK(widget_ramp_ok,
          "leg RAMP: close interior must be the seven-rung two-pixels-per-step "
          "diagonal ramp, dark upper-left to white lower-right "
          "(window-chrome.md Sec 3.2)");

    int collapse_ok = 1;
    for (int dx = 0; dx < FG_BOX_INTERIOR_SIZE; dx++) {
        collapse_ok = collapse_ok &&
            px(&active, collapse_x + 2 + dx,
               box_y + 2 + FG_COLLAPSE_GLYPH_ROW_0) == FG_BOX_RING_IDX;
        collapse_ok = collapse_ok &&
            px(&active, collapse_x + 2 + dx,
               box_y + 2 + FG_COLLAPSE_GLYPH_ROW_1) == FG_BOX_RING_IDX;
    }
    CHECK(collapse_ok,
          "leg COLLAPSE: the rightmost widget must exist and carry dark full-interior "
          "rows dy=3 and dy=5 (window-chrome.md Sec 3.3)");

    int zoom_glyph_ok = 1;
    for (int d = 0; d < FG_ZOOM_GLYPH_EDGE; d++) {
        zoom_glyph_ok = zoom_glyph_ok &&
            px(&active, zoom_x + 2 + 5, box_y + 2 + d) == FG_BOX_RING_IDX;
        zoom_glyph_ok = zoom_glyph_ok &&
            px(&active, zoom_x + 2 + d, box_y + 2 + 5) == FG_BOX_RING_IDX;
    }
    int close_glyph_px = count_idx(&active, close_x + 3, box_y + 3,
                                    close_x + 10, box_y + 10,
                                    FG_BOX_RING_IDX);
    int glyph_ok = zoom_glyph_ok && close_glyph_px == 0;
    CHECK(glyph_ok,
          "leg GLYPHS: close has no glyph; zoom has only the six-pixel right and "
          "bottom edges in dark-ring ink (window-chrome.md Sec 3.3)");

    /* ACTIVE skeleton state choice is DISABLED: flat trough, dim arrows and
     * separators, no thumb. Ref: scrollbars.md Sec 3 and source capture named
     * in chrome_fidelity_golden.h. _SCROLL_FLAT bites the state classes. */
    const int sb_left = ri - 20;
    const int sb_right = ri - 5;
    const int sb_top = WIN_TOP + FG_TITLE_BAND_ROWS;
    const int sb_mid_x = (sb_left + sb_right) / 2;
    const int sb_sep_y = sb_top + FG_SB_ARROW_TILE - 1;
    const int sb_track_y = 150;
    int sb_arrow_px = count_idx(&active, sb_left + 1, sb_top + 1,
                                 sb_right, sb_sep_y,
                                 FG_SB_DISABLED_ARROW_IDX);
    int sb_thumb_px = count_idx(&active, sb_left, sb_top,
                                 sb_right + 1, bi - 19,
                                 FG_SB_ENABLED_THUMB_LIGHT_IDX) +
                      count_idx(&active, sb_left, sb_top,
                                 sb_right + 1, bi - 19,
                                 FG_SB_ENABLED_THUMB_SHADOW_IDX);
    int scrollbar_ok =
        px(&active, sb_left, sb_track_y) == FG_SB_ENABLED_FRAME_IDX &&
        px(&active, sb_mid_x, sb_track_y) == FG_SB_DISABLED_TROUGH_IDX &&
        px(&active, sb_mid_x, sb_sep_y) == FG_SB_DISABLED_SEPARATOR_IDX &&
        sb_arrow_px >= 8 && sb_thumb_px == 0;
    CHECK(scrollbar_ok,
          "leg SCROLL-DISABLED: active no-range scene must use trough idx243, "
          "arrow idx165, separator idx119, and no thumb (scrollbars.md Sec 3)");

    /* GROW BOX: 18x18 cell, face/highlight, three pitched grip lines.
     * Ref: window-chrome.md Sec 5. */
    const int grow_x = ri - 19;
    const int grow_y = bi - 19;
    int grow_ok = strlen(FG_GROW_PROFILE) ==
                  FG_GROW_PROFILE_ROWS * FG_GROW_PROFILE_COLS &&
                  FG_GROW_GRIP_LINES == 3 && FG_GROW_GRIP_PITCH == 4;
    for (int y = 0; y < FG_GROW_PROFILE_ROWS; y++) {
        for (int x = 0; x < FG_GROW_PROFILE_COLS; x++) {
            char klass = FG_GROW_PROFILE[y * FG_GROW_PROFILE_COLS + x];
            uint32_t want = klass == 'W' ? FG_GROW_GRIP_LEAD_IDX :
                            klass == 'g' ? FG_GROW_GRIP_TRAIL_IDX :
                            klass == 'c' ? FG_GROW_GRIP_TERMINATOR_IDX :
                                           FG_GROW_FILL_IDX;
            grow_ok = grow_ok && px(&active, grow_x + x, grow_y + y) == want;
        }
    }
    CHECK(grow_ok,
          "leg GROW: active 18x18 cell must have idx218 fill, white top/left, "
          "and three white+150 grip lines at pitch4 with idx192 terminators "
          "(window-chrome.md Sec 5)");

    rc = render_ctx_init(&inactive, &boot);
    CHECK(rc == 0, "inactive 8bpp render context must initialize");
    if (rc != 0) {
        render_ctx_free(&active);
        return TEST_SUMMARY("test-chrome-fidelity");
    }
    render_run(&inactive, draw_inactive);

    /* INACTIVE title: flat 231, no stripes/bevel. _NO_INACTIVE bites here.
     * Ref: window-chrome.md Sec 6. */
    int inactive_title_ok = 1;
    for (int y = WIN_TOP + 1; y < WIN_TOP + FG_TITLE_BAND_ROWS - 1; y++) {
        inactive_title_ok = inactive_title_ok &&
            px(&inactive, pin_x, y) == FG_INACTIVE_TITLE_FILL_IDX;
    }
    CHECK(inactive_title_ok,
          "leg INACTIVE-TITLE: background title is flat idx231 for 20 interior "
          "rows, with no stripes or bevel; System-7 white idx1 is wrong "
          "(window-chrome.md Sec 6)");

    /* Inactive every frame line and shadow are 119. _INACTIVE_BLACK_FRAME bites. */
    int inactive_frame_ok =
        px(&inactive, pin_x, WIN_TOP) == FG_INACTIVE_FRAME_IDX &&
        px(&inactive, WIN_LEFT, body_y) == FG_INACTIVE_FRAME_IDX &&
        px(&inactive, ri, body_y) == FG_INACTIVE_FRAME_IDX &&
        px(&inactive, mid_x, bi) == FG_INACTIVE_FRAME_IDX &&
        px(&inactive, WIN_RIGHT, body_y) == FG_INACTIVE_SHADOW_IDX;
    CHECK(inactive_frame_ok,
          "leg INACTIVE-FRAME: all structure lines and the drop shadow must be "
          "inactive-frame idx119, not active black (window-chrome.md Sec 1/Sec 6)");

    /* Inactive text is sampled idx135, not retained System-7 idx165.
     * _INACTIVE_BRIGHT_TITLE bites active black residue. */
    int inactive_dim = count_idx(&inactive, mid_x - 40, WIN_TOP + 4,
                                 mid_x + 40, WIN_TOP + 21,
                                 FG_INACTIVE_TEXT_IDX);
    int inactive_black = count_idx(&inactive, mid_x - 40, WIN_TOP + 4,
                                   mid_x + 40, WIN_TOP + 21,
                                   FG_TITLE_INK_IDX);
    int inactive_text_ok = inactive_dim >= 8 && inactive_black == 0;
    CHECK(inactive_text_ok,
          "leg INACTIVE-TEXT: background title ink must be idx135 and never active "
          "black; retained System-7 dim ink idx165 is wrong "
          "(window-chrome.md Sec 6)");

    /* No inactive widgets. _KEEP_GADGETS bites this retained relation.
     * Ref: window-chrome.md Sec 6. */
    int inactive_widget_px = 0;
    const int widget_x[3] = { close_x, zoom_x, collapse_x };
    for (int k = 0; k < 3; k++) {
        inactive_widget_px += count_idx(&inactive, widget_x[k], box_y,
                                        widget_x[k] + FG_BOX_FOOTPRINT,
                                        box_y + FG_BOX_FOOTPRINT,
                                        FG_BOX_EDGE_IDX);
        inactive_widget_px += count_idx(&inactive, widget_x[k], box_y,
                                        widget_x[k] + FG_BOX_FOOTPRINT,
                                        box_y + FG_BOX_FOOTPRINT,
                                        FG_BOX_RING_IDX);
    }
    int inactive_gadgets_ok = inactive_widget_px == 0;
    CHECK(inactive_gadgets_ok,
          "leg INACTIVE-GADGETS: background title carries no close, zoom, or "
          "collapse widget (window-chrome.md Sec 6)");

    /* Inactive scrollbar is HOLLOW and grow is flat. _SCROLL_FLAT/_NO_INACTIVE
     * cover the old active residue. Ref: scrollbars.md Sec 4 and
     * window-chrome.md Sec 5/Sec 6. */
    int hollow_interior_ok = 1;
    for (int y = sb_top + 1; y < bi - 20; y++) {
        for (int x = sb_left + 1; x < sb_right; x++) {
            hollow_interior_ok = hollow_interior_ok &&
                px(&inactive, x, y) == FG_SB_HOLLOW_TROUGH_IDX;
        }
    }
    int inactive_grow_ok = 1;
    for (int y = 0; y < FG_GROW_CELL; y++) {
        for (int x = 0; x < FG_GROW_CELL; x++) {
            inactive_grow_ok = inactive_grow_ok &&
                px(&inactive, grow_x + x, grow_y + y) ==
                    FG_INACTIVE_GROW_FILL_IDX;
        }
    }
    int hollow_ok = px(&inactive, sb_left, sb_track_y) == FG_SB_HOLLOW_FRAME_IDX &&
                    px(&inactive, sb_mid_x, sb_track_y) == FG_SB_HOLLOW_TROUGH_IDX &&
                    hollow_interior_ok && inactive_grow_ok;
    CHECK(hollow_ok,
          "leg HOLLOW/GROW-INACTIVE: background bars are idx243 trough inside "
          "idx119 frame with nothing else, and grow cell is flat idx231 "
          "(scrollbars.md Sec 4; window-chrome.md Sec 5/Sec 6)");

    /* Keep the acceptance tail self-contained: one line per Platinum leg. */
    printf("B1 Platinum leg inventory (current System-7 renderer is expected RED):\n");
#define LEG_STATUS(name, ok, why) \
    printf("  %-24s %s -- %s\n", name, (ok) ? "PASS" : "RED", why)
    LEG_STATUS("BAND", band_ok, "22-row profile / 12 light-first stripes / 2+4 face");
    LEG_STATUS("TITLE", title_ok, "black centered ink over idx218 gap");
    LEG_STATUS("SHADOW-NOTCH", shadow_ok, "(+1,+1) black L with 2px notch");
    LEG_STATUS("BODYBAR", body_ok, "four-pixel raised body bar plus inset");
    LEG_STATUS("WIDGETS", widget_ok, "three 12+1 widgets at Platinum offsets");
    LEG_STATUS("RAMP", widget_ramp_ok, "seven-rung diagonal widget ramp");
    LEG_STATUS("COLLAPSE", collapse_ok, "rightmost collapse widget and two rows");
    LEG_STATUS("GLYPHS", glyph_ok, "close none; zoom right+bottom edges");
    LEG_STATUS("SCROLL-DISABLED", scrollbar_ok, "243/165/119 classes, no thumb");
    LEG_STATUS("GROW", grow_ok, "18x18, three pitched grip lines");
    LEG_STATUS("INACTIVE-TITLE", inactive_title_ok, "flat idx231, not Sys7 white");
    LEG_STATUS("INACTIVE-FRAME", inactive_frame_ok, "all lines/shadow idx119");
    LEG_STATUS("INACTIVE-TEXT", inactive_text_ok, "idx135, not Sys7 idx165");
    LEG_STATUS("INACTIVE-GADGETS", inactive_gadgets_ok, "all three widgets absent");
    LEG_STATUS("HOLLOW/GROW-INACTIVE", hollow_ok, "hollow bar and flat idx231 grow");
#undef LEG_STATUS

    render_ctx_free(&inactive);
    render_ctx_free(&active);
    return TEST_SUMMARY("test-chrome-fidelity");
}
