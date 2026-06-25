/* test_chrome.c -- the FLAIR chrome structural oracle (THE ORACLE; D-8 hard gate).
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
 *   - the title-bar band occupies rows [frame, frame+TITLEBAR_H) and shows
 *     pinstripe ALTERNATION at PINSTRIPE_PERIOD (2),
 *   - the window frame is exactly FRAME (1) px,
 *   - the vertical scrollbar occupies a SCROLLBAR_W (16) px-wide column on the
 *     right,
 *   - the close box and zoom box are present at the expected corners.
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
#include "chrome_metrics.h"     /* FLAIR_CHROME_* (-Ispec)                     */
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
    flair_draw_document_window(port, win_frame(), "");
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

/* ===========================================================================
 * The structural assertions, run against one rendered context.
 * `bpp_tag` is a label for failure messages. `idx_mode` is 1 for the 8bpp pass
 * (assert on shade indices) and 0 for the 32bpp pass (assert on painted/RGB).
 * ===========================================================================*/
static void assert_chrome(render_ctx_t *ctx, const char *bpp_tag, int idx_mode)
{
    const int fr = FLAIR_CHROME_FRAME;
    /* The 19-px title BAND is now decomposed (beads initech-92li; window-frame.md
     * Sec 2a): top-frame(1, y=WIN_TOP) + bevel-hi(1) + 15 pinstripe + bevel-lo(1) +
     * shared-frame(1).  title_top is the first INTERIOR row (the bevel-hi), the
     * pinstripe runs [stripe_top, stripe_bot), and the white content body begins
     * one row below the shared frame line at content_top = WIN_TOP + TITLEBAR_H. */
    const int title_top  = WIN_TOP + fr;                              /* bevel-hi */
    const int stripe_top = title_top + FLAIR_CHROME_TITLE_BEVEL_ROWS; /* 15-stripe top */
    const int stripe_bot = stripe_top + FLAIR_CHROME_TITLE_STRIPE_ROWS; /* half-open */
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
    const int pin_x = WIN_LEFT + 20;

    /* --- 2. The title-bar band occupies rows [WIN_TOP, WIN_TOP+TITLEBAR_H) ---
     * decomposed top-frame + bevel-hi + 15 stripe + bevel-lo + shared-frame.
     *
     * NOTE (beads initech-92li): is_painted treats RENDER_DESKTOP_INDEX (idx 2 =
     * CIDX_DESKTOP) as "blank", and the bevel-hi row resolves to FLAIR_PART_BEVEL_
     * LIGHT -> canon teal idx 2 (the WL-0053 lavender->teal recolor ALIASES the
     * desktop index by design; chrome_fidelity_golden.h FG_BOX_BEVEL_IDX note).
     * So we probe the first PINSTRIPE row (idx 7/8, unambiguously painted) for the
     * "band is painted" tooth; the bevel-row CLASS is graded by the recolor-
     * invariant index legs in test-chrome-fidelity, not by is_painted here. */
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

    /* The row just BELOW the title BAND is the WHITE content body, not the
     * pinstripe band. For 8bpp the content is index CIDX_WHITE (1); the pinstripe
     * is index 7/8. This asserts the band height is EXACTLY TITLEBAR_H: if the
     * title bar were 1 px too tall (CHROME_MUTATE_TITLEBAR_H), this row would be
     * a pinstripe shade (the stripe ran 16 rows, pushing content down 1), not the
     * body. window-frame.md Sec 2a: white content begins one row below the shared
     * frame line at content_top = WIN_TOP + TITLEBAR_H. */
    if (idx_mode) {
        uint32_t below = shade_index(ctx, mid_x, title_content_top);
        snprintf(msg, sizeof msg,
                 "[%s] row below title band (y=%d) must be content body (idx %d), "
                 "not a pinstripe shade -- title band must be exactly %d",
                 bpp_tag, title_content_top, 1, FLAIR_CHROME_TITLEBAR_H);
        CHECK(below == 1u, msg);

        /* And it must NOT be either pinstripe shade index (the decisive tooth
         * for the title-bar-height mutant). */
        snprintf(msg, sizeof msg,
                 "[%s] row below title band must NOT be pinstripe light/dark "
                 "(idx %d/%d) -- catches a too-tall title bar",
                 bpp_tag, FLAIR_CHROME_TITLE_SHADE_LIGHT,
                 FLAIR_CHROME_TITLE_SHADE_DARK);
        CHECK(below != (uint32_t)FLAIR_CHROME_TITLE_SHADE_LIGHT &&
              below != (uint32_t)FLAIR_CHROME_TITLE_SHADE_DARK, msg);
    }

    /* --- 3. Pinstripe is a two-shade STRIPE over the 15-row interior (phase
     * owned elsewhere) --------------------------------------------------------
     * The pinstripe interior [stripe_top, stripe_bot) is filled with the two WDEF
     * shades (wTitleBarLight 7 / wTitleBarDark 8) as a horizontal stripe. This
     * oracle asserts only that it IS a two-shade stripe -- NOT a specific phase.
     * The System-7 racing-stripe PHASE (the patAlign mod-8 doubled-LIGHT pairs)
     * and the exactly-15-row interior bounded by the bevel rows are graded against
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
                 "[%s] pinstripe interior must show BOTH WDEF shades (light %d + dark %d)",
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

    /* --- 4. The vertical scrollbar is a SCROLLBAR_W (16) px column on right -- */
    /* The scrollbar runs the content height, inside the right frame. Its left
     * gutter edge is a black divider; the column to its left (still in content)
     * up to the right frame is the scrollbar. We assert the column WIDTH: count
     * painted columns from the inner-right edge leftward on a content row that
     * is NOT inside an arrow button (mid-height of the content). */
    {
        int content_top = title_content_top;        /* WIN_TOP + TITLEBAR_H (92li) */
        int content_bot = WIN_BOTTOM - fr;
        int row = (content_top + content_bot) / 2;   /* between the arrow buttons*/
        int inner_right = WIN_RIGHT - fr;            /* first col inside R frame */

        /* The scrollbar control occupies [inner_right - SCROLLBAR_W, inner_right).
         * Assert the divider/track is present at the expected left edge and the
         * content just LEFT of the scrollbar is white body (idx 1 on 8bpp). */
        int sb_left = inner_right - FLAIR_CHROME_SCROLLBAR_W;

        /* The scrollbar's left gutter divider (black) sits at sb_left. */
        snprintf(msg, sizeof msg,
                 "[%s] scrollbar left divider must be painted at the %d px column",
                 bpp_tag, FLAIR_CHROME_SCROLLBAR_W);
        CHECK(is_painted(ctx, sb_left, row), msg);

        if (idx_mode) {
            /* Just LEFT of the scrollbar is the white content body (idx 1) --
             * proves the scrollbar starts at exactly SCROLLBAR_W from the inner
             * right edge. If the scrollbar were 15 px wide
             * (CHROME_MUTATE_SCROLLBAR_W), sb_left would be body, not divider:
             * this is the decisive scrollbar-width tooth. */
            uint32_t at_sb_left = shade_index(ctx, sb_left, row);
            snprintf(msg, sizeof msg,
                     "[%s] scrollbar must be EXACTLY %d px: column at -%d is the "
                     "divider (idx %d), not the white body (idx 1)",
                     bpp_tag, FLAIR_CHROME_SCROLLBAR_W,
                     FLAIR_CHROME_SCROLLBAR_W, 0);
            CHECK(at_sb_left == 0u, msg);   /* divider is CIDX_BLACK (0) */

            uint32_t left_of_sb = shade_index(ctx, sb_left - 1, row);
            snprintf(msg, sizeof msg,
                     "[%s] content just left of the %d px scrollbar must be the "
                     "white body (idx 1)", bpp_tag, FLAIR_CHROME_SCROLLBAR_W);
            CHECK(left_of_sb == 1u, msg);
        }

        /* The scrollbar track fills toward the inner right edge: the column at
         * inner_right-1 (just inside the right frame) is painted (track/arrow). */
        snprintf(msg, sizeof msg,
                 "[%s] scrollbar must reach the inner right edge", bpp_tag);
        CHECK(is_painted(ctx, inner_right - 1, row), msg);
    }

    /* --- 5. Close box (top-left) and zoom box (top-right) present ----------- */
    /* Each is a double-beveled FLAIR_CHROME_WBOX_RENDER (11x11) gadget, NOT a flat
     * 13px frame.  Geometry amended to the rendered close-zoom-box.md ground truth
     * (beads initech-ts3t; strengthening toward the golden, Law 2):
     *   box top = WIN_TOP + wBoxDelta + 1, wBoxDelta=(titleHgt-13)/2 (WDEF @1705-1707);
     *   close box LEFT edge = WIN_LEFT + 9 (PlotGoAway @1675-1678);
     *   zoom box  LEFT edge = WIN_RIGHT - 20 (PlotZoom @1682-1693).
     * The box top-left corner is the dark OUTLINE (close-zoom-box.md golden
     * x=361,y=168 = #545487 dark frame -> the dark figure role; 8bpp idx 4).
     * (The exact bevel/role structure is graded by test-chrome-fidelity; here we
     * pin the geometry: the box corners are painted at the NEW offsets.) */
    {
        int wbox_delta = (FLAIR_CHROME_TITLEBAR_H - FLAIR_CHROME_WBOX_DELTA) / 2;
        if (wbox_delta < 0) {
            wbox_delta = 0;
        }
        int by0 = WIN_TOP + wbox_delta + 1;  /* box top edge (struct.top+wBoxDelta+1) */

        /* close box top-left corner is painted (its dark outline). */
        int cx0 = WIN_LEFT + 9;              /* struct.left + 9 (close-zoom-box.md) */
        snprintf(msg, sizeof msg,
                 "[%s] close box (top-left, struct.left+9) must be present", bpp_tag);
        CHECK(is_painted(ctx, cx0, by0), msg);
        if (idx_mode) {
            /* the dark outline role at the corner (recolor-invariant, 8bpp idx 4). */
            snprintf(msg, sizeof msg,
                     "[%s] close box top-left corner must be the dark outline "
                     "(idx 4; close-zoom-box.md x=361,y=168 #545487)", bpp_tag);
            CHECK(shade_index(ctx, cx0, by0) == 4u, msg);
        }

        /* zoom box top-left corner is painted (its dark outline). */
        int zx0 = WIN_RIGHT - 20;            /* struct.right - 20 (close-zoom-box.md) */
        snprintf(msg, sizeof msg,
                 "[%s] zoom box (top-left, struct.right-20) must be present", bpp_tag);
        CHECK(is_painted(ctx, zx0, by0), msg);
        if (idx_mode) {
            snprintf(msg, sizeof msg,
                     "[%s] zoom box top-left corner must be the dark outline "
                     "(idx 4; close-zoom-box.md zoom left = right-20)", bpp_tag);
            CHECK(shade_index(ctx, zx0, by0) == 4u, msg);
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
