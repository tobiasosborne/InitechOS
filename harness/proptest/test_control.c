/* test_control.c -- the FLAIR Control Manager property suite (THE ORACLE).
 *
 * beads: initech-8h9 ("FLAIR Control Manager: buttons, scrollbars, progress bar").
 * Ref:   ADR-0004 D-3 ("Control Manager -- ControlRecord {value/min/max,
 *          contrlHilite, contrlRect}; part-codes inButton, inCheckBox,
 *          inUpButton/inDownButton/inPageUp/inPageDown/inThumb. Buttons,
 *          scrollbars, the FILE COPY progress bar."); MTE Ch 5 "The Control
 *          Manager"; spec/chrome_metrics.h (FLAIR_CHROME_SCROLLBAR_W=16).
 *        os/flair/control.{c,h} (the unit under test; the artifact freestanding C).
 *        harness/render/render.{c,h} (host render skeleton, the dual-compile path
 *          that runs the SAME freestanding draw code on a host offscreen).
 *        harness/proptest/test_chrome.c + test_window.c (the harness idiom this
 *          suite mirrors: TEST_HARNESS/CHECK/TEST_SUMMARY + seeded LCG + render).
 *        CLAUDE.md Law 2 (the oracle is the truth), Law 4 (look like the frame),
 *        Rule 6 (mutation-proven), Rule 11 (seeded LCG + deterministic),
 *        Rule 12 (ASCII-clean source).
 *
 * THE PROPERTIES (in order of decisiveness):
 *
 *  1. SCROLLBAR MATH: thumb position (ctrl_thumb_y) derived from value/min/max
 *     is correct AND INVERTIBLE: for a sampled set of values the forward pass
 *     (value -> thumb_y) followed by the inverse (thumb_y -> value) roundtrips
 *     to the original value (modulo integer rounding to the nearest step).
 *     The 16 px scrollbar width and arrow-button geometry are verified.
 *     TestControl returns the right part code per region:
 *       up-arrow / page-up / thumb / page-down / down-arrow.
 *
 *  2. VALUE CLAMPING: SetControlValue clamps to [min,max]; out-of-range values
 *     are stored as min or max, never exceeding the range.
 *
 *  3. PROGRESS BAR: value/max -> filled pixel width via ctrl_progress_fill_px
 *     is correct; 0% -> 0 px, 100% -> inner_w px, 50% -> inner_w/2 px.
 *     Edge cases: value==0, value==max.
 *
 *  4. BUTTON: TestControl returns inButton inside the contrlRect, 0 outside.
 *     TrackControl with a pts[] sequence entirely inside returns inButton;
 *     TrackControl releasing outside returns 0. Centered Chicago title x
 *     via text_measure / text_center_in.
 *
 *  5. DRAW: render a push button + a vertical scrollbar + a progress bar into
 *     a host 8bpp offscreen via the render skeleton (the dual-compile path).
 *     Assert key pixels:
 *       - Button: the button frame (outer 1 px black border) is painted.
 *       - Scrollbar: the thumb band (SB_THUMB_MIN rows of CTRL_CONTROL or
 *         CTRL_ACCENT) is present at the y coordinate ctrl_thumb_y predicts;
 *         the track above and below the thumb is CTRL_CONTROL.
 *         The left divider column (CTRL_BLACK == 0) is present.
 *       - Progress bar: the filled band (CTRL_ACCENT) occupies exactly
 *         ctrl_progress_fill_px pixels of the inner width; the remaining
 *         inner area is CTRL_WHITE.
 *
 * MUTANTS (Rule 6), each MUST drive this oracle RED:
 *   CONTROL_MUTATE_THUMB_OFF  -- thumb y computed with wrong factor.
 *                                => property 1 (scrollbar math) goes RED.
 *   CONTROL_MUTATE_NO_CLAMP   -- SetControlValue does not clamp.
 *                                => property 2 (clamping) goes RED.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render.h"             /* host render skeleton (-Iharness/render)     */
#include "control.h"            /* the Control Manager under test (-Ios/flair) */
#include "chrome_metrics.h"     /* FLAIR_CHROME_SCROLLBAR_W (-Ispec)           */
#include "text.h"               /* text_measure / text_center_in (-Ios/flair)  */
#include "test_assert.h"        /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)    */

TEST_HARNESS();

/* ===========================================================================
 * Seeded LCG (Rule 11 -- deterministic across runs; mirrors test_window/region)
 * ===========================================================================*/
static uint32_t g_seed = 0x4354524Cu; /* "CTRL" */
static uint32_t lcg(void)
{
    g_seed = g_seed * 1103515245u + 12345u;
    return g_seed;
}

/* ===========================================================================
 * RENDER HELPERS  (mirrors test_chrome.c render_one idiom)
 * ===========================================================================*/
static int render_one(render_ctx_t *ctx, uint32_t bpp)
{
    render_boot_info_t boot;
    memset(&boot, 0, sizeof boot);
    boot.lfb_bpp    = bpp;
    boot.lfb_width  = 640u;
    boot.lfb_height = 480u;
    boot.lfb_pitch  = 0u;      /* tight */
    return render_ctx_init(ctx, &boot);
}

/* Palette index at (x,y) in the 8bpp offscreen. */
static uint32_t pidx(const render_ctx_t *ctx, uint32_t x, uint32_t y)
{
    return render_pixel_index(ctx, x, y);
}

/* ===========================================================================
 * PROPERTY 1 -- SCROLLBAR MATH: thumb y correctness + INVERTIBILITY
 *
 * For a set of sampled values (0%, 25%, 50%, 75%, 100% of range, plus a
 * seeded random set), verify:
 *   (a) ctrl_thumb_y(ctrl) places the thumb within the track region.
 *   (b) ctrl_value_from_thumb_y(ctrl, ctrl_thumb_y(ctrl)) roundtrips to the
 *       original value (within +-1 due to integer division truncation).
 *   (c) The part codes: a point in the up-arrow button -> inUpButton; just
 *       above the thumb -> inPageUp; inside the thumb -> inThumb; below the
 *       thumb -> inPageDown; in the down-arrow -> inDownButton.
 * ===========================================================================*/
static void test_scrollbar_math(void)
{
    /* Build a standard scrollbar: 16px wide, 200 px tall, value range 0..100 */
    ControlRecord sb;
    rgn_rect_t r;
    r.top = 50; r.left = 200; r.bottom = 250; r.right = 216;
    control_init(&sb, scrollBar, r, 0, 0, 100, 1, "");

    int track_top = (int)r.top    + SB_ARROW;  /* = 50 + 16 = 66              */
    int track_bot = (int)r.bottom - SB_ARROW;  /* = 250 - 16 = 234            */
    int track_h   = track_bot - track_top;      /* = 168                       */
    int thumb_h   = SB_THUMB_MIN;               /* = 16                        */
    char msg[200];

    /* Width assertion: FLAIR_CHROME_SCROLLBAR_W */
    snprintf(msg, sizeof msg,
             "scrollbar rect width must be FLAIR_CHROME_SCROLLBAR_W=%d",
             FLAIR_CHROME_SCROLLBAR_W);
    CHECK((int)r.right - (int)r.left == FLAIR_CHROME_SCROLLBAR_W, msg);

    /* Arrow button size assertion: SB_ARROW == FLAIR_CHROME_SCROLLBAR_W */
    snprintf(msg, sizeof msg,
             "SB_ARROW (%d) must equal FLAIR_CHROME_SCROLLBAR_W (%d)",
             SB_ARROW, FLAIR_CHROME_SCROLLBAR_W);
    CHECK(SB_ARROW == FLAIR_CHROME_SCROLLBAR_W, msg);

    /* Track height sanity. */
    CHECK(track_h > thumb_h, "track_h must be larger than thumb_h for a 200px bar");

    /* Sampled values: 0, 25, 50, 75, 100 */
    int test_vals[5] = { 0, 25, 50, 75, 100 };
    for (int i = 0; i < 5; i++) {
        SetControlValue(&sb, (int16_t)test_vals[i]);
        int16_t ty = ctrl_thumb_y(&sb);

        snprintf(msg, sizeof msg,
                 "thumb_y (value=%d) must be >= track_top (%d)",
                 test_vals[i], track_top);
        CHECK((int)ty >= track_top, msg);

        snprintf(msg, sizeof msg,
                 "thumb_y (value=%d) must be <= track_bot - thumb_h (%d)",
                 test_vals[i], track_bot - thumb_h);
        CHECK((int)ty <= track_bot - thumb_h, msg);

        /* INVERTIBILITY: round-trip value -> thumb_y -> value */
        int16_t recovered = ctrl_value_from_thumb_y(&sb, ty);
        snprintf(msg, sizeof msg,
                 "thumb roundtrip: value %d -> ty=%d -> recovered=%d (must be within 1)",
                 test_vals[i], (int)ty, (int)recovered);
        /* Integer division may introduce up to 1 unit of error. */
        int diff = (int)recovered - test_vals[i];
        if (diff < 0) {
            diff = -diff;
        }
        CHECK(diff <= 1, msg);
    }

    /* Value=0: thumb at track_top. */
    SetControlValue(&sb, 0);
    CHECK(ctrl_thumb_y(&sb) == (int16_t)track_top,
          "at value=0 thumb_y must be track_top");

    /* Value=100 (max): thumb at track_bot - thumb_h. */
    SetControlValue(&sb, 100);
    int16_t ty_max = ctrl_thumb_y(&sb);
    snprintf(msg, sizeof msg,
             "at value=max thumb_y=%d must be track_bot - thumb_h = %d",
             (int)ty_max, track_bot - thumb_h);
    CHECK((int)ty_max == track_bot - thumb_h, msg);

    /* Value=50: thumb at midpoint of moveable range. */
    SetControlValue(&sb, 50);
    {
        int moveable = track_h - thumb_h;
        int expected = track_top + 50 * moveable / 100;
        int16_t ty50 = ctrl_thumb_y(&sb);
        snprintf(msg, sizeof msg,
                 "at value=50 thumb_y=%d must equal expected mid=%d",
                 (int)ty50, expected);
        int d = (int)ty50 - expected;
        if (d < 0) {
            d = -d;
        }
        CHECK(d <= 1, msg);
    }

    /* LCG random sample: 20 values, all roundtrip within 2 */
    for (int t = 0; t < 20; t++) {
        int v = (int)(lcg() % 101);   /* [0, 100] */
        SetControlValue(&sb, (int16_t)v);
        int16_t ty = ctrl_thumb_y(&sb);
        int16_t rv = ctrl_value_from_thumb_y(&sb, ty);
        int d = (int)rv - v;
        if (d < 0) {
            d = -d;
        }
        snprintf(msg, sizeof msg,
                 "LCG scrollbar roundtrip: v=%d -> ty=%d -> rv=%d (diff %d, must be <=1)",
                 v, (int)ty, (int)rv, d);
        CHECK(d <= 1, msg);
    }

    /* ===========================================================================
     * PART-CODE TESTS: TestControl returns the right code per region.
     * ===========================================================================*/
    SetControlValue(&sb, 50);

    /* Up-arrow button region: pt at top of scrollbar. */
    {
        flair_point_t pt;
        pt.v = r.top + 4;             /* well inside up-arrow button            */
        pt.h = r.left + (FLAIR_CHROME_SCROLLBAR_W / 2);
        int16_t code = TestControl(&sb, pt);
        CHECK(code == inUpButton, "point in up-arrow button -> inUpButton");
    }
    /* Down-arrow button region. */
    {
        flair_point_t pt;
        pt.v = r.bottom - 4;          /* well inside down-arrow button          */
        pt.h = r.left + (FLAIR_CHROME_SCROLLBAR_W / 2);
        int16_t code = TestControl(&sb, pt);
        CHECK(code == inDownButton, "point in down-arrow button -> inDownButton");
    }
    /* Thumb region: use ctrl_thumb_y to locate the thumb and probe its center. */
    {
        int16_t ty  = ctrl_thumb_y(&sb);
        flair_point_t pt;
        pt.v = (int16_t)((int)ty + SB_THUMB_MIN / 2);
        pt.h = r.left + (FLAIR_CHROME_SCROLLBAR_W / 2);
        int16_t code = TestControl(&sb, pt);
        CHECK(code == inThumb, "point in thumb -> inThumb");
    }
    /* Page-up: between up-arrow bottom and thumb top. */
    {
        int16_t ty  = ctrl_thumb_y(&sb);
        int pup_y = (track_top + (int)ty) / 2;  /* midway in page-up region    */
        if (pup_y >= track_top && pup_y < (int)ty) {
            flair_point_t pt;
            pt.v = (int16_t)pup_y;
            pt.h = r.left + (FLAIR_CHROME_SCROLLBAR_W / 2);
            int16_t code = TestControl(&sb, pt);
            CHECK(code == inPageUp, "point above thumb in track -> inPageUp");
        }
    }
    /* Page-down: between thumb bottom and down-arrow top. */
    {
        int16_t ty  = ctrl_thumb_y(&sb);
        int pdn_y = ((int)ty + SB_THUMB_MIN + track_bot) / 2;
        if (pdn_y > (int)ty + SB_THUMB_MIN && pdn_y < track_bot) {
            flair_point_t pt;
            pt.v = (int16_t)pdn_y;
            pt.h = r.left + (FLAIR_CHROME_SCROLLBAR_W / 2);
            int16_t code = TestControl(&sb, pt);
            CHECK(code == inPageDown, "point below thumb in track -> inPageDown");
        }
    }
    /* Point outside the scrollbar rect -> 0. */
    {
        flair_point_t pt;
        pt.v = r.top - 5;
        pt.h = r.left + 4;
        int16_t code = TestControl(&sb, pt);
        CHECK(code == 0, "point above scrollbar rect -> 0");
    }
}

/* ===========================================================================
 * PROPERTY 2 -- VALUE CLAMPING
 * ===========================================================================*/
static void test_value_clamping(void)
{
    ControlRecord ctrl;
    rgn_rect_t r;
    r.top = 10; r.left = 10; r.bottom = 30; r.right = 100;
    control_init(&ctrl, pushButton, r, 50, 0, 100, 1, "OK");

    char msg[160];

    /* SetControlValue within range. */
    SetControlValue(&ctrl, 60);
    CHECK(GetControlValue(&ctrl) == 60, "SetControlValue(60) -> GetControlValue==60");

    /* SetControlValue below min -> clamped to min. */
    SetControlValue(&ctrl, -10);
    snprintf(msg, sizeof msg,
             "SetControlValue(-10) below min(0) -> clamped to 0, got %d",
             (int)GetControlValue(&ctrl));
    CHECK(GetControlValue(&ctrl) == 0, msg);

    /* SetControlValue above max -> clamped to max. */
    SetControlValue(&ctrl, 200);
    snprintf(msg, sizeof msg,
             "SetControlValue(200) above max(100) -> clamped to 100, got %d",
             (int)GetControlValue(&ctrl));
    CHECK(GetControlValue(&ctrl) == 100, msg);

    /* SetControlValue at exactly min and max: no clamping needed. */
    SetControlValue(&ctrl, 0);
    CHECK(GetControlValue(&ctrl) == 0, "SetControlValue(0)==min is exact");
    SetControlValue(&ctrl, 100);
    CHECK(GetControlValue(&ctrl) == 100, "SetControlValue(100)==max is exact");

    /* Range with non-zero min. */
    ControlRecord ctrl2;
    r.top = 10; r.left = 10; r.bottom = 30; r.right = 100;
    control_init(&ctrl2, scrollBar, r, 50, 20, 80, 1, "");
    SetControlValue(&ctrl2, 10);
    CHECK(GetControlValue(&ctrl2) == 20, "value below min(20) clamped to 20");
    SetControlValue(&ctrl2, 90);
    CHECK(GetControlValue(&ctrl2) == 80, "value above max(80) clamped to 80");
    SetControlValue(&ctrl2, 50);
    CHECK(GetControlValue(&ctrl2) == 50, "value 50 in [20,80] stored exact");

    /* LCG clamping: random out-of-range values; all must be clamped. */
    for (int t = 0; t < 20; t++) {
        int v = (int)(lcg() % 300) - 50;  /* [-50, 249] */
        SetControlValue(&ctrl, (int16_t)v);
        int got = (int)GetControlValue(&ctrl);
        int expected = v < 0 ? 0 : (v > 100 ? 100 : v);
        snprintf(msg, sizeof msg,
                 "LCG clamp: v=%d -> expected=%d, got=%d", v, expected, got);
        CHECK(got == expected, msg);
    }
}

/* ===========================================================================
 * PROPERTY 3 -- PROGRESS BAR FILL MATH
 * ===========================================================================*/
static void test_progress_bar_math(void)
{
    ControlRecord pb;
    rgn_rect_t r;
    /* A 202 px wide bar: inner_w = 202 - 2 = 200. */
    r.top = 100; r.left = 50; r.bottom = 120; r.right = 252;
    control_init(&pb, progressBar, r, 0, 0, 100, 1, "");

    int inner_w = (int)r.right - (int)r.left - 2; /* = 200 */
    char msg[160];

    CHECK(inner_w == 200, "test fixture: inner_w must be 200");

    /* 0%: filled_px == 0. */
    SetControlValue(&pb, 0);
    snprintf(msg, sizeof msg, "progress 0%% -> filled_px=0, got %d",
             (int)ctrl_progress_fill_px(&pb));
    CHECK(ctrl_progress_fill_px(&pb) == 0, msg);

    /* 100%: filled_px == inner_w. */
    SetControlValue(&pb, 100);
    snprintf(msg, sizeof msg, "progress 100%% -> filled_px=%d, got %d",
             inner_w, (int)ctrl_progress_fill_px(&pb));
    CHECK((int)ctrl_progress_fill_px(&pb) == inner_w, msg);

    /* 50%: filled_px == inner_w/2 = 100. */
    SetControlValue(&pb, 50);
    snprintf(msg, sizeof msg, "progress 50%% -> filled_px=100, got %d",
             (int)ctrl_progress_fill_px(&pb));
    CHECK((int)ctrl_progress_fill_px(&pb) == inner_w / 2, msg);

    /* 25%: 200*25/100 = 50. */
    SetControlValue(&pb, 25);
    snprintf(msg, sizeof msg, "progress 25%% -> filled_px=50, got %d",
             (int)ctrl_progress_fill_px(&pb));
    CHECK((int)ctrl_progress_fill_px(&pb) == 50, msg);

    /* 75%: 200*75/100 = 150. */
    SetControlValue(&pb, 75);
    snprintf(msg, sizeof msg, "progress 75%% -> filled_px=150, got %d",
             (int)ctrl_progress_fill_px(&pb));
    CHECK((int)ctrl_progress_fill_px(&pb) == 150, msg);

    /* Degenerate: max==0 -> 0 (no divide-by-zero). */
    ControlRecord pb2;
    r.top = 100; r.left = 50; r.bottom = 120; r.right = 252;
    control_init(&pb2, progressBar, r, 0, 0, 0, 1, "");
    CHECK(ctrl_progress_fill_px(&pb2) == 0,
          "progress bar max==0 -> fill==0 (no divide by zero)");

    /* LCG fractional values (0..100) -> filled_px == inner_w * v / max */
    for (int t = 0; t < 20; t++) {
        int v = (int)(lcg() % 101);
        SetControlValue(&pb, (int16_t)v);
        int got      = (int)ctrl_progress_fill_px(&pb);
        int expected = inner_w * v / 100;
        snprintf(msg, sizeof msg,
                 "LCG progress v=%d -> expected=%d, got=%d", v, expected, got);
        CHECK(got == expected, msg);
    }
}

/* ===========================================================================
 * PROPERTY 4 -- BUTTON HIT-TEST + TRACK + CENTERED TITLE
 * ===========================================================================*/
static void test_button_hittest_and_track(void)
{
    ControlRecord btn;
    rgn_rect_t r;
    r.top = 200; r.left = 300; r.bottom = 220; r.right = 400;
    control_init(&btn, pushButton, r, 0, 0, 1, 1, "OK");

    char msg[160];

    /* Point inside -> inButton. */
    flair_point_t in_pt;
    in_pt.v = 210; in_pt.h = 350;
    CHECK(TestControl(&btn, in_pt) == inButton,
          "point inside push button -> inButton");

    /* Point outside -> 0. */
    flair_point_t out_pt;
    out_pt.v = 190; out_pt.h = 350;
    CHECK(TestControl(&btn, out_pt) == 0,
          "point outside push button -> 0");

    /* Point at exact top edge (inside, half-open rect). */
    flair_point_t top_pt;
    top_pt.v = 200; top_pt.h = 350;
    CHECK(TestControl(&btn, top_pt) == inButton,
          "point at top edge (in half-open rect) -> inButton");

    /* Point at bottom edge (outside half-open rect). */
    flair_point_t bot_pt;
    bot_pt.v = 220; bot_pt.h = 350;
    CHECK(TestControl(&btn, bot_pt) == 0,
          "point at bottom edge (outside half-open rect) -> 0");

    /* TrackControl: sequence entirely inside -> inButton. */
    {
        flair_point_t pts[3];
        pts[0].v = 210; pts[0].h = 350;
        pts[1].v = 211; pts[1].h = 351;
        pts[2].v = 210; pts[2].h = 352;
        int16_t final_code = TrackControl(&btn, pts, 3);
        snprintf(msg, sizeof msg,
                 "TrackControl all-inside -> inButton (%d), got %d",
                 inButton, (int)final_code);
        CHECK(final_code == inButton, msg);
        /* contrlHilite restored to 0 after tracking. */
        CHECK(btn.contrlHilite == 0,
              "contrlHilite must be 0 after TrackControl completes");
    }

    /* TrackControl: sequence ends outside -> 0. */
    {
        flair_point_t pts[3];
        pts[0].v = 210; pts[0].h = 350;
        pts[1].v = 215; pts[1].h = 360;
        pts[2].v = 190; pts[2].h = 350;  /* outside */
        int16_t final_code = TrackControl(&btn, pts, 3);
        CHECK(final_code == 0,
              "TrackControl ending outside -> 0");
        CHECK(btn.contrlHilite == 0,
              "contrlHilite must be 0 after TrackControl completes (outside end)");
    }

    /* Centered title x: (rect_w - text_measure(FONT_CHICAGO, "OK")) / 2. */
    {
        int rect_w = (int)r.right - (int)r.left;
        int lw     = text_measure(FONT_CHICAGO, "OK");
        int expected_x = (rect_w - lw) / 2;
        int got_x      = text_center_in(rect_w, "OK", FONT_CHICAGO);
        snprintf(msg, sizeof msg,
                 "text_center_in(%d, 'OK', CHICAGO): expected x=%d, got %d",
                 rect_w, expected_x, got_x);
        CHECK(got_x == expected_x, msg);
        CHECK(got_x >= 0, "centered x must be >= 0");
        CHECK(got_x + lw <= rect_w,
              "centered title must fit within button width");
    }
}

/* ===========================================================================
 * PROPERTY 5 -- DRAW: render controls into a host offscreen and assert pixels
 * ===========================================================================*/
static void draw_all_controls(GrafPort *port)
{
    /* Push button: [300,50) to [400,70) in port coords. */
    {
        ControlRecord btn;
        rgn_rect_t r;
        r.top = 50; r.left = 300; r.bottom = 70; r.right = 400;
        control_init(&btn, pushButton, r, 0, 0, 1, 1, "OK");
        DrawControl(port, &btn);
    }

    /* Vertical scrollbar: [50,100) to [66,300) -- 16px wide, 200px tall.
     * Value=50 (midpoint). */
    {
        ControlRecord sb;
        rgn_rect_t r;
        r.top = 100; r.left = 50; r.bottom = 300; r.right = 66;
        control_init(&sb, scrollBar, r, 50, 0, 100, 1, "");
        DrawControl(port, &sb);
    }

    /* Progress bar: [200,150) to [440,170) -- 240px wide, value=50/100. */
    {
        ControlRecord pb;
        rgn_rect_t r;
        r.top = 150; r.left = 200; r.bottom = 170; r.right = 440;
        control_init(&pb, progressBar, r, 50, 0, 100, 1, "");
        DrawControl(port, &pb);
    }
}

static void test_draw_controls(void)
{
    render_ctx_t ctx;
    int rc = render_one(&ctx, 8u);
    CHECK(rc == 0, "render_ctx_init(8bpp) for draw test must succeed");
    if (rc != 0) {
        return;
    }

    render_run(&ctx, draw_all_controls);

    char msg[200];

    /* --- BUTTON ASSERTIONS ---
     * Button rect: [300,50) to [400,70).
     * Frame (1 px black border); corners cleared for rounded appearance.
     * The corner pixels (300,50), (399,50), (300,69), (399,69) are cleared
     * (set to CTRL_DESKTOP) for the round-ish corner effect. Probe non-corner
     * frame pixels: top row at x=350 (center), left col at y=60 (midheight). */
    {
        /* Top frame pixel (non-corner, center of top edge). */
        snprintf(msg, sizeof msg,
                 "button top frame center (x=350, y=50) must be CTRL_BLACK (0)");
        CHECK(pidx(&ctx, 350u, 50u) == 0u, msg);

        /* Left frame pixel (non-corner, mid-height). */
        snprintf(msg, sizeof msg,
                 "button left frame mid (x=300, y=60) must be CTRL_BLACK (0)");
        CHECK(pidx(&ctx, 300u, 60u) == 0u, msg);

        /* Right frame pixel (non-corner). */
        snprintf(msg, sizeof msg,
                 "button right frame mid (x=399, y=60) must be CTRL_BLACK (0)");
        CHECK(pidx(&ctx, 399u, 60u) == 0u, msg);

        /* Bottom frame pixel (non-corner). */
        snprintf(msg, sizeof msg,
                 "button bottom frame center (x=350, y=69) must be CTRL_BLACK (0)");
        CHECK(pidx(&ctx, 350u, 69u) == 0u, msg);

        /* Corner pixel (300,50) is cleared to desktop (rounded corner). */
        snprintf(msg, sizeof msg,
                 "button TL corner (x=300, y=50) must be CTRL_DESKTOP (%u) -- rounded",
                 (unsigned)RENDER_DESKTOP_INDEX);
        CHECK(pidx(&ctx, 300u, 50u) == (uint32_t)RENDER_DESKTOP_INDEX, msg);

        /* Interior face: should be CTRL_WHITE (1) for unhilited button. */
        snprintf(msg, sizeof msg,
                 "button interior (x=350, y=60) must be CTRL_WHITE (1) when not hilited");
        CHECK(pidx(&ctx, 350u, 60u) == 1u, msg);

        /* Pixel just outside button (above) should be desktop. */
        snprintf(msg, sizeof msg,
                 "pixel above button (x=350, y=49) must be desktop index (%u)",
                 (unsigned)RENDER_DESKTOP_INDEX);
        CHECK(pidx(&ctx, 350u, 49u) == (uint32_t)RENDER_DESKTOP_INDEX, msg);
    }

    /* --- SCROLLBAR ASSERTIONS ---
     * Scrollbar rect: top=100, left=50, bottom=300, right=66 (16px wide).
     * Arrow button height: SB_ARROW=16.
     * Track: y in [116, 284).
     * Value=50, range=[0,100], track_h=168, thumb_h=16.
     * thumb_y = track_top + 50*(168-16)/100 = 116 + 76 = 192.
     *
     * Left gutter divider (CTRL_BLACK=0) at x=50, y in [100,300). */
    {
        /* Left divider column. */
        snprintf(msg, sizeof msg,
                 "scrollbar left divider (x=50, y=150) must be CTRL_BLACK (0)");
        CHECK(pidx(&ctx, 50u, 150u) == 0u, msg);

        /* Track above thumb (CTRL_CONTROL=6): at track_top + some offset above thumb. */
        /* track_top=116, thumb_y=192. Pick y=154 (in track, above thumb). */
        snprintf(msg, sizeof msg,
                 "scrollbar track above thumb (x=60, y=154) must be CTRL_CONTROL (6)");
        CHECK(pidx(&ctx, 60u, 154u) == 6u, msg);

        /* Thumb band (CTRL_CONTROL=6 face, unless hilited; unhilited here).
         * thumb_y=192, thumb extends [192,208). Check middle of thumb. */
        int thumb_mid_y = 192 + SB_THUMB_MIN / 2;
        snprintf(msg, sizeof msg,
                 "scrollbar thumb face (x=60, y=%d) must be CTRL_CONTROL (6)",
                 thumb_mid_y);
        CHECK(pidx(&ctx, 60u, (uint32_t)thumb_mid_y) == 6u, msg);

        /* Track below thumb: at y=220 (thumb_bot=208, track_bot=284). */
        snprintf(msg, sizeof msg,
                 "scrollbar track below thumb (x=60, y=220) must be CTRL_CONTROL (6)");
        CHECK(pidx(&ctx, 60u, 220u) == 6u, msg);

        /* Up-arrow button face (CTRL_CONTROL=6): y in [100,116). */
        snprintf(msg, sizeof msg,
                 "scrollbar up-arrow face (x=60, y=108) must be CTRL_CONTROL (6)");
        CHECK(pidx(&ctx, 60u, 108u) == 6u, msg);

        /* Down-arrow button face: y in [284,300). */
        snprintf(msg, sizeof msg,
                 "scrollbar down-arrow face (x=60, y=292) must be CTRL_CONTROL (6)");
        CHECK(pidx(&ctx, 60u, 292u) == 6u, msg);

        /* Thumb top frame (CTRL_BLACK=0 at thumb top edge). */
        snprintf(msg, sizeof msg,
                 "scrollbar thumb top frame (x=51, y=192) must be CTRL_BLACK (0)");
        CHECK(pidx(&ctx, 51u, 192u) == 0u, msg);

        /* Pixel to the left of the scrollbar should be desktop background. */
        snprintf(msg, sizeof msg,
                 "pixel left of scrollbar (x=49, y=200) must be desktop (%u)",
                 (unsigned)RENDER_DESKTOP_INDEX);
        CHECK(pidx(&ctx, 49u, 200u) == (uint32_t)RENDER_DESKTOP_INDEX, msg);
    }

    /* --- PROGRESS BAR ASSERTIONS ---
     * Progress bar: top=150, left=200, bottom=170, right=440 (240px wide).
     * inner_w = 240 - 2 = 238. Value=50/100: filled_px = 238*50/100 = 119.
     * Filled region: x in [201, 201+119) = [201, 320). Index CTRL_ACCENT=5.
     * Unfilled region: x in [320, 439). Index CTRL_WHITE=1.
     * Border at x=200 (left), x=439 (right), y=150 (top), y=169 (bottom). */
    {
        /* Left border. */
        snprintf(msg, sizeof msg,
                 "progress bar left border (x=200, y=160) must be CTRL_BLACK (0)");
        CHECK(pidx(&ctx, 200u, 160u) == 0u, msg);

        /* Top border. */
        snprintf(msg, sizeof msg,
                 "progress bar top border (x=300, y=150) must be CTRL_BLACK (0)");
        CHECK(pidx(&ctx, 300u, 150u) == 0u, msg);

        /* Filled region: x=250, y=160 (inside filled half). */
        snprintf(msg, sizeof msg,
                 "progress bar filled region (x=250, y=160) must be CTRL_ACCENT (5)");
        CHECK(pidx(&ctx, 250u, 160u) == 5u, msg);

        /* Unfilled region: x=380, y=160 (inside unfilled half). */
        snprintf(msg, sizeof msg,
                 "progress bar unfilled region (x=380, y=160) must be CTRL_WHITE (1)");
        CHECK(pidx(&ctx, 380u, 160u) == 1u, msg);

        /* Exact boundary: filled_px=119 -> last filled x = 201+119-1 = 319.
         * x=319 should be CTRL_ACCENT; x=320 should be CTRL_WHITE. */
        snprintf(msg, sizeof msg,
                 "progress bar at boundary (x=319, y=160) must be CTRL_ACCENT (5)");
        CHECK(pidx(&ctx, 319u, 160u) == 5u, msg);

        snprintf(msg, sizeof msg,
                 "progress bar at boundary+1 (x=320, y=160) must be CTRL_WHITE (1)");
        CHECK(pidx(&ctx, 320u, 160u) == 1u, msg);
    }

    render_ctx_free(&ctx);
}

/* ===========================================================================
 * PROPERTY 6 -- SCROLLBAR DRAW: verify the scrollbar left-divider column and
 * thumb placement directly from a 8bpp render, independent of the draw code.
 * (Structural pixel proof that draw and math agree.)
 * ===========================================================================*/
static void draw_sb_only(GrafPort *port)
{
    ControlRecord sb;
    rgn_rect_t r;
    r.top = 50; r.left = 100; r.bottom = 250; r.right = 116;
    control_init(&sb, scrollBar, r, 0, 0, 100, 1, "");
    DrawControl(port, &sb);
}

static void test_scrollbar_draw_vs_math(void)
{
    render_ctx_t ctx;
    int rc = render_one(&ctx, 8u);
    CHECK(rc == 0, "render_ctx_init(8bpp) for scrollbar draw/math agree test");
    if (rc != 0) {
        return;
    }

    render_run(&ctx, draw_sb_only);

    /* Scrollbar: top=50, left=100, bottom=250, right=116 (16px wide).
     * Value=0: thumb should be at track_top = 50 + 16 = 66.
     * Verify: pixel at x=108, y=66 (thumb face, CTRL_CONTROL=6).
     *         pixel at x=100, y=150 (left divider, CTRL_BLACK=0). */
    char msg[200];

    snprintf(msg, sizeof msg,
             "scrollbar left divider (x=100, y=150) must be CTRL_BLACK (0)");
    CHECK(pidx(&ctx, 100u, 150u) == 0u, msg);

    /* At value=0, thumb_y=66. Thumb interior at x=108, y=66+8=74. */
    snprintf(msg, sizeof msg,
             "scrollbar thumb at value=0 (x=108, y=74) must be CTRL_CONTROL (6)");
    CHECK(pidx(&ctx, 108u, 74u) == 6u, msg);

    /* Track above thumb at value=0: there is NO track above (thumb is at top).
     * Track below thumb: y=66+16=82 should be CTRL_CONTROL=6. */
    snprintf(msg, sizeof msg,
             "scrollbar track below thumb at value=0 (x=108, y=82) must be CTRL_CONTROL (6)");
    CHECK(pidx(&ctx, 108u, 82u) == 6u, msg);

    render_ctx_free(&ctx);
}

/* ===========================================================================
 * PROPERTY 7 -- THUMB DRAG: TrackControl updates value proportionally.
 * ===========================================================================*/
static void test_thumb_drag(void)
{
    ControlRecord sb;
    rgn_rect_t r;
    r.top = 50; r.left = 100; r.bottom = 250; r.right = 116;
    control_init(&sb, scrollBar, r, 0, 0, 100, 1, "");

    /* value=0: thumb at track_top=66. Drag the thumb 50% of moveable range
     * downward. moveable = (250-16) - (50+16) - 16 = 234 - 66 - 16 = 152.
     * 50% of moveable = 76 px down. New thumb_y = 66+76=142.
     * Expected new value = ctrl_value_from_thumb_y(ctrl, 142). */
    int track_top = (int)r.top + SB_ARROW;      /* 66 */
    int track_bot = (int)r.bottom - SB_ARROW;   /* 234 */
    int track_h   = track_bot - track_top;       /* 168 */
    int thumb_h   = SB_THUMB_MIN;               /* 16 */
    int moveable  = track_h - thumb_h;           /* 152 */

    /* Initial thumb center. */
    int16_t start_y = ctrl_thumb_y(&sb); /* = track_top = 66 (at value=0) */
    CHECK((int)start_y == track_top, "at value=0 thumb starts at track_top=66");

    /* Simulate dragging from thumb center to 50% down. */
    int drag_delta = moveable / 2;   /* 76 */
    flair_point_t pts[2];
    pts[0].v = start_y + thumb_h / 2; pts[0].h = 108; /* click in thumb center */
    pts[1].v = pts[0].v + (int16_t)drag_delta; pts[1].h = 108; /* drag down */

    int16_t final_code = TrackControl(&sb, pts, 2);

    char msg[200];
    snprintf(msg, sizeof msg,
             "TrackControl thumb drag final_code: expected inThumb (%d) or 0, got %d",
             inThumb, (int)final_code);
    /* After drag to midpoint, the final point should still be in the thumb
     * or in a track region (both are valid depending on new thumb position). */
    CHECK(final_code == inThumb || final_code == inPageDown || final_code == 0, msg);

    /* After the drag, value should be approximately 50. */
    int new_val = (int)GetControlValue(&sb);
    snprintf(msg, sizeof msg,
             "after thumb drag 50%% down, value should be ~50 (got %d)", new_val);
    /* Allow +-5 tolerance for integer rounding in the track. */
    int d = new_val - 50;
    if (d < 0) {
        d = -d;
    }
    CHECK(d <= 5, msg);

    /* contrlHilite must be 0 after TrackControl. */
    CHECK(sb.contrlHilite == 0, "contrlHilite must be 0 after TrackControl");

    (void)moveable;
}

/* ===========================================================================
 * main -- run all properties; report.
 * ===========================================================================*/
int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    printf("test-control: FLAIR Control Manager oracle\n");

    test_scrollbar_math();
    test_value_clamping();
    test_progress_bar_math();
    test_button_hittest_and_track();
    test_draw_controls();
    test_scrollbar_draw_vs_math();
    test_thumb_drag();

    return TEST_SUMMARY("test-control");
}
