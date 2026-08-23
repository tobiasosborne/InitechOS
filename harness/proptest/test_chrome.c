/* test_chrome.c -- the FLAIR Platinum chrome structural oracle (D-8 hard gate).
 *
 * beads: initech-k8o5.8 (first rendered System-7 window chrome + test-chrome).
 * Ref:   ADR-0004 D-8 ("test-chrome | hard pass/fail | Chrome renders match
 *        chrome_metrics v1 ... STRUCTURAL compare, not SSIM"); FO-2/AM-3
 *        (chrome_metrics v1 LOCKED AND test-chrome MUTATION-PROVEN with three
 *        named mutants, before ANY Window/Control Manager drawing ships).
 *        CLAUDE.md Law 2 (the oracle is the truth, not the agent), Law 4 (look
 *        like the frame), Rule 6 (golden/oracle mutation-proven), Rule 11
 *        (deterministic), Rule 12 (ASCII).
 *
 * This drives the REAL artifact chrome drawer (os/flair/chrome.c) HOSTED via the
 * host render skeleton (harness/render), the dual-compile pattern: the SAME
 * chrome.c the kernel links freestanding. It renders one System-7 documentProc
 * window into BOTH an 8bpp (OD-2) and a 32bpp offscreen and STRUCTURALLY ASSERTS
 * the rendered pixels against chrome_metrics v1:
 *
 *   - the title-bar band has the exact 22-row Platinum profile,
 *   - the window frame is exactly FRAME (1) px,
 *   - the vertical scrollbar occupies a SCROLLBAR_W (16) px-wide column on the
 *     right,
 *   - close, zoom, and collapse widgets are present at the measured offsets.
 *
 * Plus the STEP-1 .h<->.json CONSISTENCY tooth lives in the Makefile gate
 * (python3 parses chrome_metrics.json and diffs each #define) so spec/
 * chrome_metrics.h can NEVER silently drift from the locked JSON.
 *
 * MUTATION (Rule 6; FO-2/AM-3): three named mutants are compiled into chrome.c
 * via -D and MUST drive this oracle RED:
 *   CHROME_MUTATE_TITLEBAR_H   -- title bar 1 px too tall.
 *   CHROME_MUTATE_NO_FRAME     -- the 1 px window frame skipped.
 *   CHROME_MUTATE_SCROLLBAR_W  -- scrollbar 15 px wide.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render.h"             /* the host render skeleton (-Iharness/render) */
#include "chrome.h"             /* flair_draw_document_window (-Ios/flair)     */
#include "flair_look.h"         /* flair_look_default_skin (-Ios/flair)         */
#include "chrome_metrics.h"     /* FLAIR_CHROME_* (-Ispec)                     */
#include "color_canon.h"        /* named sampled Platinum canon indices         */
#include "test_assert.h"        /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)    */

TEST_HARNESS();

/* The window we render: a generous documentProc window inside a 640x480 port,
 * away from the port edges so clipping never trims the chrome (clipping itself
 * is exercised by the region engine's own suite). */
enum {
    WIN_LEFT   = 40,
    WIN_TOP    = 30,
    WIN_RIGHT  = 360,
    WIN_BOTTOM = 300
};

static rgn_rect_t win_frame(void)
{
    rgn_rect_t r;
    r.top = WIN_TOP; r.left = WIN_LEFT; r.bottom = WIN_BOTTOM; r.right = WIN_RIGHT;
    return r;
}

/* The draw program the skeleton runs. */
static void draw_window(GrafPort *port)
{
    /* No title here: this oracle grades the pinstripe/box/frame/scrollbar geometry,
     * not the title (the title element is graded by test-chrome-fidelity, lxg9). An
     * empty title draws no text, so the mid_x pinstripe probes stay valid. */
    /* hilited=1: this oracle grades the ACTIVE geometry (pinstripe/box/frame/
     * scrollbar); the inactive appearance is graded separately by
     * test-chrome-fidelity (beads initech-a9iq). */
    flair_draw_document_window(port, flair_look_default_skin(),
                               win_frame(), "", 1);
}

/* Is pixel (x,y) CHROME (i.e. NOT the bare desktop background)? The skeleton
 * fills the offscreen with the desktop index (RENDER_DESKTOP_INDEX), NOT 0,
 * because index 0 is black ink (frame/box borders) -- so "painted" means "drawn
 * by the chrome", distinct from both the desktop AND black ink. For 8bpp compare
 * the index; for 32bpp compare the RGB to the desktop RGB. */
static int is_painted(const render_ctx_t *ctx, int x, int y)
{
    if (ctx->fb.bm.bpp == 8u) {
        return render_pixel_index(ctx, (uint32_t)x, (uint32_t)y)
               != (uint32_t)RENDER_DESKTOP_INDEX;
    }
    uint32_t bg = render_palette_rgb((uint8_t)RENDER_DESKTOP_INDEX) & 0x00FFFFFFu;
    return (render_pixel_rgb(ctx, (uint32_t)x, (uint32_t)y) & 0x00FFFFFFu) != bg;
}

/* The shade INDEX at (x,y) for the 8bpp context (the pinstripe alternation is
 * authored as wctb shade indices 7/8 -- exact as indices, golden-resolves as
 * RGB, so the oracle reasons in indices for the 8bpp pass). */
static uint32_t shade_index(const render_ctx_t *ctx, int x, int y)
{
    return render_pixel_index(ctx, (uint32_t)x, (uint32_t)y);
}

/* By-construction row classifier for this structural oracle. The independent
 * value oracle remains test-chrome-fidelity. Re-key authority:
 * ADR-0004-AMENDMENT-DEC-10 Sec 4; sampled geometry/classes:
 * ../system7-decomp/specs/sys8/window-chrome.md Sec 2.1-2.2. */
static uint32_t platinum_title_row_index(int row)
{
    if (row == 0 || row == FLAIR_CHROME_TITLEBAR_H - 1) {
        return CIDX_BLACK;
    }
    if (row == 1) {
        return CIDX_WHITE;
    }
    if (row < FLAIR_CHROME_TITLE_STRIPE_TOP_OFF) {
        return CIDX_PLAT_FRAME_FACE;
    }
    if (row < FLAIR_CHROME_TITLE_STRIPE_TOP_OFF +
              FLAIR_CHROME_TITLE_BAND_STRIPE_ROWS) {
        return ((row - FLAIR_CHROME_TITLE_STRIPE_TOP_OFF) & 1)
               ? CIDX_PLAT_STRIPE_DARK : CIDX_WHITE;
    }
    if (row < FLAIR_CHROME_TITLEBAR_H - 2) {
        return CIDX_PLAT_FRAME_FACE;
    }
    return CIDX_PLAT_FRAME_SHADOW;
}

/* ===========================================================================
 * The structural assertions, run against one rendered context.
 * `bpp_tag` is a label for failure messages. `idx_mode` is 1 for the 8bpp pass
 * (assert on shade indices) and 0 for the 32bpp pass (assert on painted/RGB).
 * ===========================================================================*/
static void assert_chrome(render_ctx_t *ctx, const char *bpp_tag, int idx_mode)
{
    const int fr = FLAIR_CHROME_FRAME;
    /* Platinum Route-2 re-key: 22 rows are K,H,2 face,12 stripe,4 face,S,K.
     * Ref: ADR-0004-AMENDMENT-DEC-10 Sec 4 and
     * ../system7-decomp/specs/sys8/window-chrome.md Sec 2.1-2.2. */
    const int title_top = WIN_TOP + fr;
    const int stripe_top = WIN_TOP + FLAIR_CHROME_TITLE_STRIPE_TOP_OFF;
    const int stripe_bot = stripe_top +
                           FLAIR_CHROME_TITLE_BAND_STRIPE_ROWS;
    /* content_top: white body start (one row below the shared frame line). */
    const int title_content_top = WIN_TOP + FLAIR_CHROME_TITLEBAR_H;
    /* A column well inside the title bar, clear of the close/zoom boxes (those
     * are inset ~fr+3 .. +3+13 from each corner). The window is 320 px wide; the
     * horizontal center is comfortably between the two boxes. */
    const int mid_x = (WIN_LEFT + WIN_RIGHT) / 2;

    char msg[160];

    /* --- 1. The window frame is exactly FRAME (1) px ----------------------- */
    /* The outer frame line is painted; the pixel just OUTSIDE the window is the
     * blank background (frame does not bleed outward). Probe the left edge at a
     * title-bar row. */
    int edge_y = title_top + 4;
    snprintf(msg, sizeof msg, "[%s] left frame pixel must be painted", bpp_tag);
    CHECK(is_painted(ctx, WIN_LEFT, edge_y), msg);

    snprintf(msg, sizeof msg,
             "[%s] pixel just LEFT of the window must be blank (frame is %d px)",
             bpp_tag, fr);
    CHECK(!is_painted(ctx, WIN_LEFT - 1, edge_y), msg);

    /* The frame is exactly 1 px: the outer line is black; the pixel one in is
     * the inner groove (also painted), but the line at WIN_LEFT is a single
     * column. We verify the band [WIN_LEFT, WIN_LEFT+fr) is painted and that the
     * frame did not become >1 px outward (already checked WIN_LEFT-1 blank). */
    for (int i = 0; i < fr; i++) {
        snprintf(msg, sizeof msg,
                 "[%s] frame column %d of %d must be painted", bpp_tag, i, fr);
        CHECK(is_painted(ctx, WIN_LEFT + i, edge_y), msg);
    }

    /* A clear pinstripe column: right of the close box and LEFT of the centered
     * title, so the title knockout/glyphs do not interrupt the stripe run when we
     * scan the 15-row pinstripe band (beads initech-92li recomposition). */
    const int pin_x = WIN_LEFT + FLAIR_CHROME_TITLE_RUN_LEFT_OFF + 2;

    /* --- 2. The title-bar band occupies exactly 22 rows --------------------
     * Ref: ADR-0004-AMENDMENT-DEC-10 Sec 4 and window-chrome.md Sec 2.1. */
    snprintf(msg, sizeof msg,
             "[%s] title-bar first pinstripe row (y=%d) must be painted",
             bpp_tag, stripe_top);
    CHECK(is_painted(ctx, mid_x, stripe_top), msg);

    /* Last interior row of the title band (the shared frame line at
     * WIN_TOP+TITLEBAR_H-1) is painted (it is the black shared FrameRect line). */
    snprintf(msg, sizeof msg,
             "[%s] title-bar shared frame line (y=%d) must be painted",
             bpp_tag, title_content_top - 1);
    CHECK(is_painted(ctx, mid_x, title_content_top - 1), msg);

    /* The exact row classifier is stronger than the retained System-7
     * two-shade-only check and catches a centered/symmetric stripe field too.
     * Ref: ADR-0004-AMENDMENT-DEC-10 Sec 4 and window-chrome.md Sec 2.1-2.2. */
    if (idx_mode) {
        int profile_ok = 1;
        for (int row = 0; row < FLAIR_CHROME_TITLEBAR_H; row++) {
            profile_ok = profile_ok &&
                shade_index(ctx, pin_x, WIN_TOP + row) ==
                    platinum_title_row_index(row);
        }
        snprintf(msg, sizeof msg,
                 "[%s] title band must match the exact 22-row Platinum profile",
                 bpp_tag);
        CHECK(profile_ok, msg);

        /* The row below the measured band is content. This is the decisive
         * height-mutant tooth. Ref: DEC-10 Sec 4; window-chrome.md Sec 2.1. */
        uint32_t below = shade_index(ctx, mid_x, title_content_top);
        snprintf(msg, sizeof msg,
                 "[%s] row below title band (y=%d) must be content body (idx %d), "
                 "after the exact %d-row band",
                 bpp_tag, title_content_top, CIDX_WHITE,
                 FLAIR_CHROME_TITLEBAR_H);
        CHECK(below == CIDX_WHITE, msg);
    }

    /* --- 3. Pinstripe is a two-shade STRIPE over the 12-row interior -------
     * The pinstripe interior [stripe_top,stripe_bot) uses the sampled Platinum
     * white/150 classes. This
     * oracle asserts only that it IS a two-shade stripe -- NOT a specific phase.
     * The exact Platinum phase and exactly-12-row interior are graded against
     * the INDEPENDENT ../system7-decomp golden by test-chrome-fidelity (beads
     * initech-hmll/92li, Law 2).  Scanned at a clear column (pin_x) so the
     * centered title knockout does not interrupt the run. */
    if (idx_mode) {
        int saw_light = 0, saw_dark = 0, striped = 0;
        uint32_t prev = 0xFFFFFFFFu;
        for (int y = stripe_top; y < stripe_bot; y++) {
            uint32_t s = shade_index(ctx, pin_x, y);
            if (s == (uint32_t)FLAIR_CHROME_TITLE_SHADE_LIGHT) saw_light = 1;
            if (s == (uint32_t)FLAIR_CHROME_TITLE_SHADE_DARK)  saw_dark = 1;
            if (y > stripe_top && s != prev) striped = 1;
            prev = s;
        }
        snprintf(msg, sizeof msg,
                 "[%s] pinstripe interior must show BOTH Platinum shades (light %d + dark %d)",
                 bpp_tag, FLAIR_CHROME_TITLE_SHADE_LIGHT,
                 FLAIR_CHROME_TITLE_SHADE_DARK);
        CHECK(saw_light && saw_dark, msg);
        snprintf(msg, sizeof msg,
                 "[%s] pinstripe interior must be STRIPED (>=1 adjacent shade change)",
                 bpp_tag);
        CHECK(striped, msg);
    } else {
        /* 32bpp: the interior must contain >=2 distinct row RGBs (it is striped). */
        int striped = 0;
        uint32_t first = render_pixel_rgb(ctx, (uint32_t)pin_x,
                                          (uint32_t)stripe_top);
        for (int y = stripe_top + 1; y < stripe_bot; y++) {
            uint32_t a = render_pixel_rgb(ctx, (uint32_t)pin_x, (uint32_t)y);
            if (a != first) { striped = 1; break; }
        }
        snprintf(msg, sizeof msg,
                 "[%s] pinstripe interior must be STRIPED in RGB (>=2 distinct row colors)",
                 bpp_tag);
        CHECK(striped, msg);
    }

    /* --- 4. The vertical scrollbar is a 16-pixel Platinum band ------------
     * It terminates at the inner body line, inside the four-pixel raised rail.
     * Ref: ADR-0004-AMENDMENT-DEC-10 Sec 4,
     * ../system7-decomp/specs/sys8/scrollbars.md Sec 1 and
     * ../system7-decomp/specs/sys8/window-chrome.md Sec 4. */
    {
        int content_top = title_content_top;
        int content_bot = WIN_BOTTOM - fr;
        int row = (content_top + content_bot) / 2;
        int sb_right_exclusive = WIN_RIGHT - fr - FLAIR_CHROME_BODY_BAR;

        /* The control is the exact half-open band
         * [sb_right_exclusive-16,sb_right_exclusive). */
        int sb_left = sb_right_exclusive - FLAIR_CHROME_SCROLLBAR_W;

        /* The scrollbar's left gutter divider (black) sits at sb_left. */
        snprintf(msg, sizeof msg,
                 "[%s] scrollbar left divider must be painted at the %d px column",
                 bpp_tag, FLAIR_CHROME_SCROLLBAR_W);
        CHECK(is_painted(ctx, sb_left, row), msg);

        if (idx_mode) {
            /* Just left is content; the band starts with a black divider and
             * ends with the black inner-body line. If the band were 15 px wide
             * (CHROME_MUTATE_SCROLLBAR_W), sb_left would be body, not divider:
             * this is the decisive width tooth. Ref: DEC-10 Sec 4;
             * scrollbars.md Sec 1; window-chrome.md Sec 4. */
            uint32_t at_sb_left = shade_index(ctx, sb_left, row);
            snprintf(msg, sizeof msg,
                     "[%s] scrollbar must be EXACTLY %d px: column at -%d is the "
                     "divider (idx %d), not the white body (idx 1)",
                     bpp_tag, FLAIR_CHROME_SCROLLBAR_W,
                     FLAIR_CHROME_SCROLLBAR_W, CIDX_BLACK);
            CHECK(at_sb_left == CIDX_BLACK, msg);

            uint32_t left_of_sb = shade_index(ctx, sb_left - 1, row);
            snprintf(msg, sizeof msg,
                     "[%s] content just left of the %d px scrollbar must be the "
                     "white body (idx %d)", bpp_tag,
                     FLAIR_CHROME_SCROLLBAR_W, CIDX_WHITE);
            CHECK(left_of_sb == CIDX_WHITE, msg);

            snprintf(msg, sizeof msg,
                     "[%s] disabled Platinum track interior must be trough idx %d",
                     bpp_tag, CIDX_PLAT_TROUGH);
            CHECK(shade_index(ctx, sb_left + 1, row) == CIDX_PLAT_TROUGH,
                  msg);
        }

        /* The final column is the inner body line, not the outer frame rail. */
        snprintf(msg, sizeof msg,
                 "[%s] scrollbar must reach its inner-body-line boundary",
                 bpp_tag);
        CHECK(is_painted(ctx, sb_right_exclusive - 1, row), msg);
    }

    /* --- 5. Three Platinum title widgets at measured inclusive-R offsets ---
     * This replaces the retained two-widget System-7 check with the stronger
     * three-widget/edge-class relation. Ref: ADR-0004-AMENDMENT-DEC-10 Sec 4
     * and ../system7-decomp/specs/sys8/window-chrome.md Sec 3.1-3.2. */
    {
        int ri = WIN_RIGHT - 1;
        int by0 = WIN_TOP + FLAIR_CHROME_WIDGET_TOP_OFF;

        int cx0 = WIN_LEFT + FLAIR_CHROME_CLOSE_LEFT_OFF;
        snprintf(msg, sizeof msg,
                 "[%s] close widget at struct.left+%d must be present",
                 bpp_tag, FLAIR_CHROME_CLOSE_LEFT_OFF);
        CHECK(is_painted(ctx, cx0, by0), msg);
        if (idx_mode) {
            snprintf(msg, sizeof msg,
                     "[%s] close top/left edge must be Platinum widget-edge idx %d",
                     bpp_tag, CIDX_PLAT_WIDGET_EDGE);
            CHECK(shade_index(ctx, cx0, by0) == CIDX_PLAT_WIDGET_EDGE,
                  msg);
        }

        int zx0 = ri - FLAIR_CHROME_ZOOM_RIGHT_OFF;
        snprintf(msg, sizeof msg,
                 "[%s] zoom widget at inclusive-R-%d must be present",
                 bpp_tag, FLAIR_CHROME_ZOOM_RIGHT_OFF);
        CHECK(is_painted(ctx, zx0, by0), msg);
        if (idx_mode) {
            snprintf(msg, sizeof msg,
                     "[%s] zoom top/left edge must be Platinum widget-edge idx %d",
                     bpp_tag, CIDX_PLAT_WIDGET_EDGE);
            CHECK(shade_index(ctx, zx0, by0) == CIDX_PLAT_WIDGET_EDGE,
                  msg);
        }

        int kx0 = ri - FLAIR_CHROME_COLLAPSE_RIGHT_OFF;
        snprintf(msg, sizeof msg,
                 "[%s] collapse widget at inclusive-R-%d must be present and rightmost",
                 bpp_tag, FLAIR_CHROME_COLLAPSE_RIGHT_OFF);
        CHECK(is_painted(ctx, kx0, by0) && kx0 > zx0, msg);
        if (idx_mode) {
            snprintf(msg, sizeof msg,
                     "[%s] collapse top/left edge must be Platinum widget-edge idx %d",
                     bpp_tag, CIDX_PLAT_WIDGET_EDGE);
            CHECK(shade_index(ctx, kx0, by0) == CIDX_PLAT_WIDGET_EDGE,
                  msg);
        }
    }
}

/* Render one window at the given bpp via the skeleton (AM-1: geometry is a
 * runtime parameter -- a fake boot_info, never a hardcoded aperture). */
static int render_one(render_ctx_t *ctx, uint32_t bpp)
{
    render_boot_info_t boot;
    memset(&boot, 0, sizeof boot);
    boot.lfb_addr   = 0xE0000000u; /* a plausible VBE PhysBasePtr (ignored host)*/
    boot.lfb_pitch  = 0u;          /* tight -> width*bpp/8 (honored by skeleton)*/
    boot.lfb_bpp    = bpp;
    boot.lfb_width  = 640u;        /* the native 640x480 (ADR-0004 OD-3)        */
    boot.lfb_height = 480u;
    int rc = render_ctx_init(ctx, &boot);
    if (rc == 0) {
        render_run(ctx, draw_window);
    }
    return rc;
}

int main(int argc, char **argv)
{
    /* --- 8bpp (OD-2 indexed-8) pass --------------------------------------- */
    render_ctx_t c8;
    int rc8 = render_one(&c8, 8u);
    CHECK(rc8 == 0, "render_ctx_init(8bpp) must succeed (AM-1 geometry param)");
    if (rc8 == 0) {
        assert_chrome(&c8, "8bpp", 1);
    }

    /* --- 32bpp (direct XRGB8888) pass ------------------------------------- */
    render_ctx_t c32;
    int rc32 = render_one(&c32, 32u);
    CHECK(rc32 == 0, "render_ctx_init(32bpp) must succeed (AM-1 geometry param)");
    if (rc32 == 0) {
        assert_chrome(&c32, "32bpp", 0);
    }

    /* --- AM-1 proof: the skeleton honors a DIFFERENT runtime geometry ------ */
    /* Pass a padded pitch + a non-640 width to prove nothing is hardcoded. */
    {
        render_boot_info_t boot;
        memset(&boot, 0, sizeof boot);
        boot.lfb_bpp = 8u;
        boot.lfb_width = 400u;            /* not 640 -- a runtime parameter    */
        boot.lfb_height = 320u;
        boot.lfb_pitch = 512u;            /* padded (> 400) -- honored exactly */
        render_ctx_t cp;
        int rcp = render_ctx_init(&cp, &boot);
        CHECK(rcp == 0, "skeleton accepts an arbitrary runtime geometry (AM-1)");
        if (rcp == 0) {
            CHECK(cp.fb.bm.width == 400u && cp.fb.bm.height == 320u &&
                  cp.fb.bm.pitch == 512u,
                  "skeleton geometry == caller boot_info, not a hardcoded aperture");
            render_ctx_free(&cp);
        }
    }

    /* --- optional: dump a PPM of the 8bpp window for human audit (Law 4) --- */
    if (argc > 1 && rc8 == 0) {
        if (render_write_ppm(&c8, argv[1]) == 0) {
            printf("    wrote rendered window PPM to %s\n", argv[1]);
        } else {
            fprintf(stderr, "    WARN: could not write PPM to %s\n", argv[1]);
        }
    }

    if (rc8 == 0) {
        render_ctx_free(&c8);
    }
    if (rc32 == 0) {
        render_ctx_free(&c32);
    }
    return TEST_SUMMARY("test-chrome");
}
