/*
 * test_chrome_fidelity.c -- the FLAIR window-chrome FIDELITY oracle.
 *
 * beads: initech-hmll. Ref: CLAUDE.md Law 2 (the oracle is the truth; NOT
 *        by-construction -- the expected pattern comes from the INDEPENDENT
 *        ../system7-decomp golden, spec/chrome_fidelity_golden.h, never from
 *        chrome_metrics.h which chrome.c renders FROM), Law 4 (look like the
 *        frame), Rule 1 (Red->Green: this oracle is RED on today's chrome.c, by
 *        design -- it pins a real fidelity bug), Rule 6 (mutation-proven once the
 *        render is fixed GREEN), Rule 11 (deterministic), Rule 12 (ASCII).
 *
 * WHAT IT GRADES (first increment): the title-bar pinstripe PHASE. It drives the
 * REAL artifact drawer os/flair/chrome.c through the host render skeleton (the same
 * dual-compile path test_chrome.c uses), scans the title-bar stripe column on the
 * 8bpp pass, and asserts the System-7 phase-lock SIGNATURE -- the doubled-LIGHT row
 * pairs the patAlign mod-8 lock lands at the interior boundaries -- against the
 * INDEPENDENT golden in spec/chrome_fidelity_golden.h.
 *
 * MECHANISM, NOT COLOR. The assertions reason in shade-index RELATIONS (light idx 7
 * vs dark idx 8), which are invariant under the Initech teal recolor (graded
 * separately by test-color-canon). This oracle never touches the teal/lavender axis.
 *
 * WHY IT IS RED TODAY (the gap this closes). chrome.c free-runs the period-2 fill
 * from the title-bar top (phase = (y - title_top) % 2 -> L,D,L,D,...). That is also
 * "period-2 alternation", so it PASSES test-chrome's period_ok and ppm_flair_check
 * leg (c). But it has NO doubled-LIGHT pairs -- so the real System-7 phase is not
 * reproduced, and NOTHING currently catches it. This oracle does.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "render.h"                  /* the host render skeleton (-Iharness/render) */
#include "chrome.h"                  /* flair_draw_document_window (-Ios/flair)     */
#include "chrome_metrics.h"          /* FLAIR_CHROME_FRAME -- POSITIONING ONLY, never
                                      * as an expected golden value (-Ispec)        */
#include "chrome_fidelity_golden.h"  /* THE independent golden (-Ispec)             */
#include "test_assert.h"             /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)    */

TEST_HARNESS();

/* The window we render -- a generous documentProc window away from the port edges
 * (identical to test_chrome.c so clipping never trims the chrome). */
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

static void draw_window(GrafPort *port)
{
    flair_draw_document_window(port, win_frame());
}

/* Classify a rendered title-bar row by its 8bpp shade index: 'L' light (idx 7),
 * 'D' dark (idx 8), '.' neither (frame/bevel/body -- not a stripe). The expected
 * PATTERN is the independent golden; this only reads how the render encoded the row. */
static char classify_row(const render_ctx_t *ctx, int x, int y)
{
    uint32_t idx = render_pixel_index(ctx, (uint32_t)x, (uint32_t)y);
    if (idx == (uint32_t)FG_STRIPE_LIGHT_IDX) return 'L';
    if (idx == (uint32_t)FG_STRIPE_DARK_IDX)  return 'D';
    return '.';
}

int main(void)
{
    const int fr = FLAIR_CHROME_FRAME;          /* title-bar POSITION, not a golden */
    const int title_top = WIN_TOP + fr;
    /* Scan a generous band from the title top down past where the stripe can run,
     * clamped to the content; the classifier ignores non-stripe rows. The column is
     * the window's horizontal center -- comfortably between the close/zoom boxes. */
    const int mid_x = (WIN_LEFT + WIN_RIGHT) / 2;
    const int scan_lo = title_top;
    const int scan_hi = WIN_BOTTOM;             /* generous upper bound             */

    render_ctx_t ctx;
    render_boot_info_t boot;
    memset(&boot, 0, sizeof boot);
    boot.lfb_bpp = 8u;                          /* OD-2 indexed-8: shade INDICES    */
    boot.lfb_width = 640u;
    boot.lfb_height = 480u;
    int rc = render_ctx_init(&ctx, &boot);
    CHECK(rc == 0, "render_ctx_init(8bpp) must succeed (AM-1 geometry param)");
    if (rc != 0) {
        return TEST_SUMMARY("test-chrome-fidelity");
    }
    render_run(&ctx, draw_window);

    /* Collect the contiguous run of STRIPE rows (L/D) in the title-bar column,
     * skipping any leading/trailing non-stripe (frame/bevel/body) rows. */
    char run[64];
    int  n = 0;
    int  started = 0;
    for (int y = scan_lo; y < scan_hi && n < (int)sizeof run - 1; y++) {
        char c = classify_row(&ctx, mid_x, y);
        if (c == 'L' || c == 'D') {
            run[n++] = c;
            started = 1;
        } else if (started) {
            break;                              /* end of the contiguous stripe run */
        }
    }
    run[n] = '\0';

    printf("test-chrome-fidelity: title-bar stripe column x=%d -> \"%s\" (%d rows)\n",
           mid_x, run, n);
    printf("test-chrome-fidelity: System-7 golden interior pattern    -> \"%s\" "
           "(%dL/%dD, doubled-light pairs at both ends)\n",
           FG_TITLE_INTERIOR_PATTERN, FG_TITLE_INTERIOR_LIGHT_N,
           FG_TITLE_INTERIOR_DARK_N);

    /* (1) There must BE a stripe run to grade. */
    CHECK(n >= 4,
          "title-bar column must contain a pinstripe run (>=4 stripe rows)");

    if (n >= 4) {
        /* (2) PHASE-LOCK, TOP: the stripe run BEGINS with a doubled-LIGHT pair
         * (golden interior rows 0,1 = L,L). A free-running L,D,... fails here. */
        CHECK(run[0] == 'L' && run[1] == 'L',
              "title-bar pinstripe must begin with a doubled-LIGHT pair "
              "(System-7 patAlign mod-8 phase lock); a free-running period-2 fill "
              "from the title top gives L,D and is WRONG");

        /* (3) PHASE-LOCK, BOTTOM: the stripe run ENDS with a doubled-LIGHT pair
         * (golden interior rows 13,14 = L,L). */
        CHECK(run[n - 1] == 'L' && run[n - 2] == 'L',
              "title-bar pinstripe must end with a doubled-LIGHT pair "
              "(System-7 phase lock); a free-running fill ends D,L and is WRONG");

        /* (4) The decisive recolor-invariant tooth: a phase-locked racing stripe
         * has >=1 pair of adjacent EQUAL rows; a free-running period-2 alternation
         * has ZERO. This is what test-chrome's period_ok and ppm_flair_check leg (c)
         * cannot see. */
        int doubled = 0;
        for (int k = 1; k < n; k++) {
            if (run[k] == run[k - 1]) { doubled = 1; break; }
        }
        CHECK(doubled,
              "title-bar pinstripe must be PHASE-LOCKED (>=1 adjacent equal-row "
              "pair); FLAIR's free-running period-2 alternation has none -- this is "
              "the phase bug ppm_flair_check's 'period-2 alternation' cannot catch");
    }

    render_ctx_free(&ctx);
    return TEST_SUMMARY("test-chrome-fidelity");
}
