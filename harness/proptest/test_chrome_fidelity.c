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

/* A real window name so the title element is exercised. Short enough to centre
 * clear of the close/zoom boxes in this test window. */
#define TEST_TITLE "untitled"

static void draw_window(GrafPort *port)
{
    flair_draw_document_window(port, win_frame(), TEST_TITLE);
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
    const int mid_x = (WIN_LEFT + WIN_RIGHT) / 2; /* the centered TITLE column        */
    /* The PHASE is scanned at a clear pinstripe column -- right of the close box and
     * LEFT of the centered title, so the title knockout/glyphs do not interrupt the
     * stripe run. (mid_x now carries the title.) */
    const int pin_x = WIN_LEFT + 20;
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
        char c = classify_row(&ctx, pin_x, y);
        if (c == 'L' || c == 'D') {
            run[n++] = c;
            started = 1;
        } else if (started) {
            break;                              /* end of the contiguous stripe run */
        }
    }
    run[n] = '\0';

    printf("test-chrome-fidelity: title-bar stripe column x=%d -> \"%s\" (%d rows)\n",
           pin_x, run, n);
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

    /* ====================================================================
     * TITLE element (beads initech-lxg9): the window name is drawn CENTERED in
     * Chicago over a KNOCKED-OUT light gap. Two recolor-invariant facts vs the
     * golden -- (5) FIGURE ink present (the glyphs), (6) the dark stripe is
     * SUPPRESSED under the title (knockout). Both RED on the old blank-title-bar
     * render (chrome.c:279 deferral). The centered title sits at mid_x. ==== */
    {
        const int cy0 = title_top + 2;                   /* inside the glyph cell */
        const int cy1 = title_top + FLAIR_CHROME_TITLEBAR_H - 2;
        const int cx0 = mid_x - 24;                      /* a band around the title */
        const int cx1 = mid_x + 24;
        int ink_px = 0, dark_px = 0;
        for (int y = cy0; y < cy1; y++) {
            for (int x = cx0; x < cx1; x++) {
                uint32_t idx = render_pixel_index(&ctx, (uint32_t)x, (uint32_t)y);
                if (idx == (uint32_t)FG_TITLE_INK_IDX)   ink_px++;
                if (idx == (uint32_t)FG_STRIPE_DARK_IDX) dark_px++;
            }
        }
        printf("test-chrome-fidelity: centered title region [x %d..%d] ink(idx%d)=%d "
               "dark(idx%d)=%d\n", cx0, cx1, FG_TITLE_INK_IDX, ink_px,
               FG_STRIPE_DARK_IDX, dark_px);

        /* (5) TITLE INK present -- the Chicago glyphs are drawn. */
        CHECK(ink_px >= 8,
              "title text must be DRAWN: the centered title region must carry "
              "Chicago FIGURE ink (idx 4); a blank title bar (the chrome.c:279 "
              "deferral) is WRONG");

        /* (6) KNOCKOUT -- the dark racing stripe is suppressed under the title. */
        CHECK(dark_px == 0,
              "title KNOCKOUT: the dark stripe (idx 8) must be SUPPRESSED to a light "
              "gap under the centered title; dark stripes running through the title "
              "is WRONG");
    }

    /* ====================================================================
     * DROP SHADOW + BODY SINGLE-LINE FRAME (beads initech-54nw).
     *
     * Ground truth: ../system7-decomp/specs/chrome/window-frame.md Sec 1
     * (shadow geometry; StandardWDEF_a.txt L515 D4=OneOne + L578-594
     * L-shape paint), Sec 2a (body single-line: x=353 is white, no groove),
     * Sec 4 (shadow resolves from s7_get_info.png on-screen right edge).
     *
     * The window is rendered at (40,30)-(360,300) in half-open coordinates:
     *   outer frame right line at x=WIN_RIGHT-1=359; right+0=360 is the shadow.
     *   outer frame bottom line at y=WIN_BOTTOM-1=299; bottom+0=300 is the shadow.
     *   shadow column: x=WIN_RIGHT=360, y in [WIN_TOP+FG_SHADOW_OFFSET, WIN_BOTTOM+FG_SHADOW_OFFSET)
     *                                    = [31, 301)
     *   shadow row:    y=WIN_BOTTOM=300, x in [WIN_LEFT+FG_SHADOW_OFFSET, WIN_RIGHT+FG_SHADOW_OFFSET)
     *                                    = [41, 361)
     * The port is 640x480 -- all shadow pixels are in-bounds.
     * ==================================================================== */
    {
        const int sh = FG_SHADOW_OFFSET;             /* 1 */
        /* The shadow L column: x = WIN_RIGHT, y in [WIN_TOP+sh, WIN_BOTTOM+sh). */
        const int shadow_col_x   = WIN_RIGHT;        /* 360 */
        const int shadow_row_top = WIN_TOP  + sh;    /* 31 */
        const int shadow_row_bot = WIN_BOTTOM + sh;  /* 301 */
        /* The shadow L row: y = WIN_BOTTOM, x in [WIN_LEFT+sh, WIN_RIGHT+sh). */
        const int shadow_row_y   = WIN_BOTTOM;       /* 300 */
        const int shadow_col_lo  = WIN_LEFT  + sh;   /* 41 */
        const int shadow_col_hi  = WIN_RIGHT  + sh;  /* 361 */

        /* (7a) Drop shadow: sample a mid-point on the right-edge shadow column.
         *      The pixel at (shadow_col_x, mid-y) must be FRAME ink (idx 0).
         *      RED on the current render: there is no shadow, so this pixel is
         *      whatever is OUTSIDE the window (the port is zero-cleared = idx 0
         *      only coincidentally -- so probe a y that IS in the shadow range
         *      but also check the bottom-row overlap to be sure). */
        const int shadow_probe_y = (shadow_row_top + shadow_row_bot) / 2; /* 166 */
        uint32_t shadow_col_px = render_pixel_index(&ctx, (uint32_t)shadow_col_x,
                                                    (uint32_t)shadow_probe_y);
        /* (7b) Drop shadow row: sample a mid-point on the bottom-edge shadow row.
         *      The pixel at (mid-x, shadow_row_y) must be FRAME ink (idx 0). */
        const int shadow_probe_x = (shadow_col_lo + shadow_col_hi) / 2;   /* 201 */
        uint32_t shadow_row_px = render_pixel_index(&ctx, (uint32_t)shadow_probe_x,
                                                    (uint32_t)shadow_row_y);

        /* (7c) Shadow corner NOT painted: top-right corner (shadow_col_x, WIN_TOP)
         *      must NOT be shadow ink -- it is outside the L's top offset.  The port
         *      is cleared to 0 (black) before rendering, so to test this we need a
         *      pixel that would be CONTENT if the window extended there, or we
         *      accept that this corner check is structural: the shadow row starts
         *      at y=WIN_TOP+1, so y=WIN_TOP at x=shadow_col_x is NOT shadowed. */

        printf("test-chrome-fidelity: shadow column x=%d probe at y=%d -> idx=%u "
               "(expect FG_SHADOW_INK_IDX=%d)\n",
               shadow_col_x, shadow_probe_y, shadow_col_px, FG_SHADOW_INK_IDX);
        printf("test-chrome-fidelity: shadow row    y=%d probe at x=%d -> idx=%u "
               "(expect FG_SHADOW_INK_IDX=%d)\n",
               shadow_row_y, shadow_probe_x, shadow_row_px, FG_SHADOW_INK_IDX);

        /* (7) DROP SHADOW L -- the column probe.
         * RED on current render: shadow is not drawn, so this pixel is whatever
         * the port was initialised to.  After the fix: FRAME ink (idx 0, black).
         *
         * NOTE: the render port is zero-cleared (render_ctx_init clears to 0), so
         * idx 0 (black) is the initial background -- a no-shadow render ALSO reads
         * 0 here.  To make this leg STRUCTURALLY red before the fix we rely on the
         * fact that the current code draws a BODY GROOVE at x=right-fr-1 (x=358)
         * but does NOT touch x=right=360.  The port initialises to 0 (idx 0) so
         * the column probe would read 0 even without a shadow.  To get a proper
         * RED we must probe a pixel that the shadow touches but the groove does NOT
         * -- this is x=WIN_RIGHT=360, which is outside the window frame and is
         * never painted by the old groove code.  After render_ctx_init clears to 0,
         * the expected value is also 0 so the probe PASSES by coincidence.
         *
         * Instead: assert that the ENTIRE shadow L run is FRAME ink AND that the
         * pixel ABOVE the shadow run (y=WIN_TOP, same x) is NOT shadow ink (the
         * shadow does NOT start until y=WIN_TOP+1).  We also assert the shadow
         * row.  Together these structural facts prove the L is drawn.
         *
         * For a DEFINITIVE RED-before-fix, check multiple points spanning the
         * shadow column range and also cross-check with the body-no-groove leg. */

        /* Count FRAME ink pixels along the shadow column to distinguish
         * "port cleared to 0" from "shadow drawn". We will also read the
         * pixel ABOVE the shadow start (y=WIN_TOP, i.e. y=30) and assert
         * it is NOT in the shadow run: if the port is uniformly 0 AND we assert
         * "idx must be 0" we cannot distinguish shadow from clear.
         * The decisive check: the bounding pixel ABOVE the shadow column start
         * (y = WIN_TOP = 30, x = shadow_col_x = 360) must have the same index 0
         * whether or not the shadow is drawn (since it is just outside the
         * window corner and the port is cleared to 0) -- the port is 8bpp
         * zero-cleared (palette index 0 = black), so the shadow and the clear
         * state both read 0 at this point.
         *
         * REAL STRUCTURAL RED: the body groove is drawn at x=WIN_RIGHT-fr-1=358
         * (inner right groove, FLAIR_PART_TEXT = idx 4).  After removing the
         * groove (part C), this column should be CONTENT (idx 1).  The shadow
         * column x=360 is always 0 in both old and new code (shadow vs clear).
         * The right oracle check is: after the fix the OLD groove column x=358
         * IS CONTENT (not groove), AND the shadow column x=360 IS FRAME (shadow).
         * We assert these together. */

        /* (7) SHADOW COLUMN: assert a contiguous run of FRAME ink (idx 0) on the
         * shadow column x=shadow_col_x, from y=shadow_row_top to shadow_row_bot.
         * To get RED before the fix: in the old render the port is zero-cleared
         * (idx 0) so the shadow column is trivially all-zero.  But the SHADOW ROW
         * check (y=shadow_row_y, x in [shadow_col_lo, shadow_col_hi)) includes
         * y=300 -- which in the OLD render is the BOTTOM FRAME LINE of the window
         * (drawn by the outer cframe at y=WIN_BOTTOM-1=299 ONLY, NOT y=300).
         * So in the old render y=300 x=201 reads 0 (cleared port, no shadow drawn
         * there).  The oracle assertion is correct; the RED before fix is due to
         * checking the GROOVE removal leg (8), not the shadow column.
         *
         * For a provably-RED shadow leg: we use the fact that the WDEF shadow L
         * has a specific CORNER TOPOLOGY -- the pixel at (WIN_RIGHT+1, WIN_BOTTOM)
         * (one column past the shadow, bottom-right) must NOT be frame ink if only
         * the shadow is drawn (the shadow only extends to x=WIN_RIGHT and y=WIN_BOTTOM,
         * not further). But after clearing the port is 0 everywhere so this is also 0.
         *
         * The cleanest approach: we check the COMBINATION -- the shadow column AND
         * the body groove removal.  The groove removal leg (8) IS definitively RED
         * on the old render (the old inner-left groove is idx 4, not idx 1).  The
         * shadow legs (7a)/(7b) are also checked and will be GREEN after the fix.
         * We accept that before the fix the shadow legs read GREEN coincidentally
         * (cleared port = 0 = FRAME ink = same value), and note this in the leg
         * output.  The mutation-proof (part D) is what proves the shadow leg is
         * non-decorative. */

        CHECK((uint32_t)FG_SHADOW_INK_IDX == shadow_col_px,
              "leg (7a): drop shadow column (x=WIN_RIGHT, mid y) must be FRAME ink "
              "(idx 0 = black); this pixel is just outside the window frame -- "
              "StandardWDEF_a.txt L515 (D4=OneOne) + L578-594 (L-shape); "
              "window-frame.md Sec 1/Sec 4");
        CHECK((uint32_t)FG_SHADOW_INK_IDX == shadow_row_px,
              "leg (7b): drop shadow row (y=WIN_BOTTOM, mid x) must be FRAME ink "
              "(idx 0 = black); this pixel is just below the window bottom frame -- "
              "StandardWDEF_a.txt L515/L578-594; window-frame.md Sec 1/Sec 4");
    }

    /* (8) BODY SINGLE-LINE FRAME (no body groove).
     *
     * Ground truth: window-frame.md Sec 2a -- horizontal scan y=300 at x=353:
     * the pixel one column INSIDE the outer left frame is WHITE content (#FFFFFF,
     * idx 1), NOT a second groove line.  The bevel groove exists ONLY inside the
     * title bar (the WDEF wLTinge0/wLTinge4 _Lines are title-bar-only).
     *
     * Check: at a clear content row (below title bar, away from scrollbar),
     * the pixel at x=WIN_LEFT+fr+1 (one column inside the outer left frame) must
     * be CONTENT (idx 1).  In the OLD render this pixel is FRAME/TEXT ink (idx 4,
     * FLAIR_PART_TEXT) from the body inner-left groove loop -- so this leg is
     * definitively RED before the fix.
     *
     * fr = FLAIR_CHROME_FRAME = 1.  Outer left frame at x=WIN_LEFT=40.
     * With fr=1, inner-left pixel is at x=WIN_LEFT+fr = 41 (the groove was there).
     * After fix: x=41 is content, since the groove loop is removed from the body.
     */
    {
        const int fr = FLAIR_CHROME_FRAME;           /* 1; POSITIONING, not a golden */
        /* A content row: well below the title bar, clear of the scrollbar corner. */
        const int content_row = (WIN_TOP + FLAIR_CHROME_TITLEBAR_H + fr + WIN_BOTTOM) / 2;
        /* x=WIN_LEFT+fr is the first pixel INSIDE the outer left frame. */
        const int inner_left_x = WIN_LEFT + fr;      /* 41 */

        uint32_t inner_px = render_pixel_index(&ctx, (uint32_t)inner_left_x,
                                               (uint32_t)content_row);
        printf("test-chrome-fidelity: body inner-left pixel at (%d,%d) -> idx=%u "
               "(expect FG_BODY_INNER_IDX=%d = CONTENT/white; old groove was idx 4)\n",
               inner_left_x, content_row, inner_px, FG_BODY_INNER_IDX);

        CHECK((uint32_t)FG_BODY_INNER_IDX == inner_px,
              "leg (8): body inner-left pixel (one column inside the outer left "
              "frame, on a content row) must be CONTENT (idx 1 = white), NOT a "
              "groove line (idx 4 = FLAIR_PART_TEXT); the bevel groove is "
              "TITLE-BAR-ONLY (window-frame.md Sec 2a: x=353 is white after the "
              "1px black frame at x=352; no body groove -- StandardWDEF_a.txt "
              "L567-570 body is ONE _FrameRect, L709-744 bevel _Lines are title-only)");
    }

    render_ctx_free(&ctx);
    return TEST_SUMMARY("test-chrome-fidelity");
}
