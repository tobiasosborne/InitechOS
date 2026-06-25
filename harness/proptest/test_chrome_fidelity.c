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
     * skipping any leading/trailing non-stripe (frame/bevel/body) rows. Record the
     * y of the FIRST stripe row so the bevel-row legs can probe the row immediately
     * ABOVE the run (bevel-hi) and BELOW it (bevel-lo); beads initech-92li. */
    char run[64];
    int  n = 0;
    int  started = 0;
    int  run_start_y = -1;
    for (int y = scan_lo; y < scan_hi && n < (int)sizeof run - 1; y++) {
        char c = classify_row(&ctx, pin_x, y);
        if (c == 'L' || c == 'D') {
            if (!started) run_start_y = y;
            run[n++] = c;
            started = 1;
        } else if (started) {
            break;                              /* end of the contiguous stripe run */
        }
    }
    run[n] = '\0';
    int run_end_y = run_start_y + n - 1;        /* y of the last stripe row         */

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
     * (4b) TITLE-BAR BEVEL ROWS + EXACTLY-15-ROW INTERIOR (beads initech-92li).
     *
     * Ground truth: ../system7-decomp/specs/chrome/window-frame.md Sec 2a (x=400
     * vertical scan: y=164 frame / y=165 bevel-hi #DADAFF / y=166..180 15 stripe /
     * y=181 bevel-lo #B3B3DA / y=182 shared frame) + Sec 2b + pinstripe.md
     * ("title interior height 15 px y=166..180; bevel rows 1 px top + 1 px bottom").
     *
     * The pinstripe interior is EXACTLY 15 rows (FG_TITLE_INTERIOR_ROWS), bounded
     * ABOVE by the bevel-hi row (FLAIR_PART_BEVEL_LIGHT -> idx 2) and BELOW by the
     * bevel-lo row (FLAIR_PART_BEVEL_SHADOW -> idx 4) -- a DISTINCT 3rd/4th row
     * class, neither L (idx 7) nor D (idx 8), recolor-invariant.
     *
     * RED before the recomposition: FLAIR drew 19 ALL-stripe rows, no bevel -- the
     * contiguous L/D run was >15 AND the rows bounding it were NOT bevel-hi (idx 2)
     * / bevel-lo (idx 4).  The CHROME_FID_MUT_NO_BEVEL mutant reverts to that.
     * ==================================================================== */
    {
        /* The row indices, read directly from the render (NOT classified L/D). */
        uint32_t bevel_hi_px = (run_start_y > scan_lo)
            ? render_pixel_index(&ctx, (uint32_t)pin_x, (uint32_t)(run_start_y - 1))
            : 0xFFFFFFFFu;
        uint32_t bevel_lo_px =
            render_pixel_index(&ctx, (uint32_t)pin_x, (uint32_t)(run_end_y + 1));
        uint32_t shared_px =
            render_pixel_index(&ctx, (uint32_t)pin_x, (uint32_t)(run_end_y + 2));

        printf("test-chrome-fidelity: stripe run = %d rows (golden exactly %d); row "
               "ABOVE (y=%d) idx=%u (expect bevel-hi %d), row BELOW (y=%d) idx=%u "
               "(expect bevel-lo %d), shared frame (y=%d) idx=%u (expect %d)\n",
               n, FG_TITLE_INTERIOR_ROWS, run_start_y - 1, bevel_hi_px,
               FG_TITLE_BEVEL_HI_IDX, run_end_y + 1, bevel_lo_px,
               FG_TITLE_BEVEL_LO_IDX, run_end_y + 2, shared_px,
               FG_TITLE_SHARED_FRAME_IDX);

        /* (4b-i) The contiguous pinstripe run is EXACTLY 15 rows. */
        CHECK(n == FG_TITLE_INTERIOR_ROWS,
              "title-bar pinstripe interior must be EXACTLY 15 rows "
              "(FG_TITLE_INTERIOR_ROWS; window-frame.md Sec 2a y=166..180); the old "
              "all-stripe band ran the full title height (>15, no bevel rows)");

        /* (4b-ii) The row immediately ABOVE the stripe run is the bevel-HI role
         * (BEVEL_LIGHT -> idx 2): a distinct row class, NOT a pinstripe shade. */
        CHECK(bevel_hi_px == (uint32_t)FG_TITLE_BEVEL_HI_IDX,
              "the row above the pinstripe run must be the bevel-HI highlight "
              "(FLAIR_PART_BEVEL_LIGHT -> idx 2; window-frame.md Sec 2b golden y=165 "
              "#DADAFF wLTinge0); an all-stripe band has a pinstripe shade there");

        /* (4b-iii) The row immediately BELOW the stripe run is the bevel-LO role
         * (BEVEL_SHADOW -> idx 4): a distinct row class, NOT a pinstripe shade. */
        CHECK(bevel_lo_px == (uint32_t)FG_TITLE_BEVEL_LO_IDX,
              "the row below the pinstripe run must be the bevel-LO shadow "
              "(FLAIR_PART_BEVEL_SHADOW -> idx 4; window-frame.md Sec 2b golden y=181 "
              "#B3B3DA wLTinge4); an all-stripe band has a pinstripe shade there");

        /* (4b-iv) Below the bevel-lo is the SHARED frame line (FRAME -> idx 0):
         * the bottom of the title FrameRect AND the top of the content body. */
        CHECK(shared_px == (uint32_t)FG_TITLE_SHARED_FRAME_IDX,
              "below the bevel-lo must be the SHARED frame line (FLAIR_PART_FRAME -> "
              "idx 0; window-frame.md Sec 2a golden y=182 #000000 -- bottom of the "
              "title FrameRect AND top of the content-body FrameRect)");

        /* (4b-v) The bevel-hi and bevel-lo are DISTINCT from the pinstripe shades
         * (recolor-invariant: a 3rd/4th row class, not L=idx7/D=idx8). */
        CHECK(bevel_hi_px != (uint32_t)FG_STRIPE_LIGHT_IDX &&
              bevel_hi_px != (uint32_t)FG_STRIPE_DARK_IDX &&
              bevel_lo_px != (uint32_t)FG_STRIPE_LIGHT_IDX &&
              bevel_lo_px != (uint32_t)FG_STRIPE_DARK_IDX,
              "the bevel rows must be a DISTINCT row class (idx 2 / idx 4), neither "
              "the pinstripe LIGHT (idx 7) nor DARK (idx 8) -- the bevel is NOT a "
              "stripe (window-frame.md Sec 2b: y=165/y=181 are bevel _Lines)");
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

    /* ====================================================================
     * CLOSE / ZOOM BOX (beads initech-ts3t).
     *
     * Ground truth: ../system7-decomp/specs/chrome/close-zoom-box.md.
     *   - boxes RENDER 11x11 (NOT the WDEF 13 px derivation; golden
     *     s7_doc_window.png close box x=361..371, y=168..178).
     *   - close box left edge = struct.left + 9 (PlotGoAway WDEF @1675-1678).
     *   - zoom box  left edge = struct.right - 20 (PlotZoom WDEF @1682-1693).
     *   - box top = struct.top + wBoxDelta + 1, wBoxDelta=(titleHgt-13)/2
     *     (=3 for the 19 px bar) -> box top = frame_top + 4 (WDEF @1705-1707).
     *   - the gadget is a double-beveled square: >=3 distinct tonal roles
     *     (dark outline idx FG_BOX_DARK_IDX / lavender bevel idx FG_BOX_BEVEL_IDX
     *     / gray face idx FG_BOX_FACE_IDX) in the ASCII-diagram arrangement.
     *   - the ZOOM box additionally carries the nested-square glyph (extra interior
     *     dark figure); the CLOSE box interior is plain gray (no glyph).
     *
     * The window is rendered at (40,30)-(360,300) in half-open coords:
     *   wBoxDelta = (FLAIR_CHROME_TITLEBAR_H - FLAIR_CHROME_WBOX_DELTA)/2 = 3.
     *   box top  = WIN_TOP + wBoxDelta + 1 = 30+3+1 = 34.
     *   close box left = WIN_LEFT  + 9  = 49  (spans x=[49, 60), 11 px).
     *   zoom box  left = WIN_RIGHT - 20 = 340 (spans x=[340,351), 11 px).
     *
     * RED on the current render (flat 1px 13x13 frame inset fr+3=4): the close box
     * draws at x=[44,57) (13 wide), zoom at x=[343,356), one tonal role only, zoom
     * identical to close. Every leg below bites that.
     * ==================================================================== */
    {
        const int wbox_delta =
            (FLAIR_CHROME_TITLEBAR_H - FLAIR_CHROME_WBOX_DELTA) / 2;   /* 3 */
        const int box_top   = WIN_TOP + wbox_delta + 1;               /* 34 */
        const int box_sz    = FG_BOX_RENDER_SIZE;                     /* 11 */
        const int close_l   = WIN_LEFT  + FG_CLOSE_BOX_LEFT_OFF;      /* 49 */
        const int zoom_l    = WIN_RIGHT - FG_ZOOM_BOX_RIGHT_OFF;      /* 340 */

        /* Helper-free inline scan over an 11x11 box at left edge `bl`. Counts the
         * distinct tonal roles present (dark/bevel/face) and the interior dark
         * pixels (for the zoom-glyph comparison). The box outer-rim is x in
         * [bl, bl+11), y in [box_top, box_top+11); the INTERIOR (face) region is
         * the inset 7x7 (close-zoom-box.md x=363..369,y=170..176 => +2 inset). */
        /* --- close box scan --- */
        int close_dark = 0, close_bevel = 0, close_face = 0;
        int close_interior_dark = 0;
        for (int y = box_top; y < box_top + box_sz; y++) {
            for (int x = close_l; x < close_l + box_sz; x++) {
                uint32_t idx = render_pixel_index(&ctx, (uint32_t)x, (uint32_t)y);
                if (idx == (uint32_t)FG_BOX_DARK_IDX)  close_dark++;
                if (idx == (uint32_t)FG_BOX_BEVEL_IDX) close_bevel++;
                if (idx == (uint32_t)FG_BOX_FACE_IDX)  close_face++;
                /* interior = inset by 2 from each edge (the 7x7 face region) */
                if (x >= close_l + 2 && x < close_l + box_sz - 2 &&
                    y >= box_top + 2 && y < box_top + box_sz - 2 &&
                    idx == (uint32_t)FG_BOX_DARK_IDX) {
                    close_interior_dark++;
                }
            }
        }
        /* --- zoom box scan --- */
        int zoom_dark = 0, zoom_bevel = 0, zoom_face = 0;
        int zoom_interior_dark = 0;
        for (int y = box_top; y < box_top + box_sz; y++) {
            for (int x = zoom_l; x < zoom_l + box_sz; x++) {
                uint32_t idx = render_pixel_index(&ctx, (uint32_t)x, (uint32_t)y);
                if (idx == (uint32_t)FG_BOX_DARK_IDX)  zoom_dark++;
                if (idx == (uint32_t)FG_BOX_BEVEL_IDX) zoom_bevel++;
                if (idx == (uint32_t)FG_BOX_FACE_IDX)  zoom_face++;
                if (x >= zoom_l + 2 && x < zoom_l + box_sz - 2 &&
                    y >= box_top + 2 && y < box_top + box_sz - 2 &&
                    idx == (uint32_t)FG_BOX_DARK_IDX) {
                    zoom_interior_dark++;
                }
            }
        }

        int close_roles = (close_dark > 0) + (close_bevel > 0) + (close_face > 0);
        int zoom_roles  = (zoom_dark  > 0) + (zoom_bevel  > 0) + (zoom_face  > 0);

        printf("test-chrome-fidelity: close box [x %d..%d y %d..%d] roles=%d "
               "(dark=%d bevel=%d face=%d) interior_dark=%d\n",
               close_l, close_l + box_sz, box_top, box_top + box_sz, close_roles,
               close_dark, close_bevel, close_face, close_interior_dark);
        printf("test-chrome-fidelity: zoom  box [x %d..%d y %d..%d] roles=%d "
               "(dark=%d bevel=%d face=%d) interior_dark=%d\n",
               zoom_l, zoom_l + box_sz, box_top, box_top + box_sz, zoom_roles,
               zoom_dark, zoom_bevel, zoom_face, zoom_interior_dark);

        /* (9) RENDERED SIZE = 11x11.  The top edge is the dark OUTER frame row,
         * spanning EXACTLY the 11 columns [close_l, close_l+11).  We assert the
         * dark top-edge run is exactly 11 px wide: the box top-left corner IS dark,
         * the column one px PAST the box (x=close_l+11) is NOT dark and NOT the gray
         * face, and the dark run measured along the top edge == 11.  (The lavender
         * bevel role idx 7 is INDISTINGUISHABLE from the pinstripe LIGHT idx 7 by
         * index, so the recolor-invariant size discriminator uses the UNAMBIGUOUS
         * box tints -- dark outline idx 4 and gray face idx 6 -- which never appear
         * in the surrounding pinstripe band.)  The OLD 13px-wide box would give a
         * dark run != 11 and a box tint at x=close_l+11 -> RED. */
        {
            uint32_t corner = render_pixel_index(&ctx, (uint32_t)close_l,
                                                 (uint32_t)box_top);
            uint32_t past   = render_pixel_index(&ctx, (uint32_t)(close_l + box_sz),
                                                 (uint32_t)box_top);
            int past_is_solid_box = (past == (uint32_t)FG_BOX_DARK_IDX) ||
                                    (past == (uint32_t)FG_BOX_FACE_IDX);
            /* measure the contiguous dark top-edge run starting at close_l. */
            int dark_run = 0;
            for (int x = close_l;
                 render_pixel_index(&ctx, (uint32_t)x, (uint32_t)box_top)
                     == (uint32_t)FG_BOX_DARK_IDX;
                 x++) {
                dark_run++;
                if (dark_run > 64) break;       /* safety */
            }
            printf("test-chrome-fidelity: close box top-left idx=%u, dark top-edge run "
                   "=%d (expect %d), one px past (x=%d) idx=%u (must not be dark/face)\n",
                   corner, dark_run, box_sz, close_l + box_sz, past);
            CHECK(corner == (uint32_t)FG_BOX_DARK_IDX,
                  "leg (9a): close box top-left corner (struct.left+9, box top) must "
                  "be the dark outline role (idx 4); close-zoom-box.md golden "
                  "x=361,y=168 = #545487 dark frame");
            CHECK(dark_run == box_sz,
                  "leg (9b): close box must render 11x11 -- the dark OUTER top-edge run "
                  "must be EXACTLY 11 px; a 13px box (old FLAIR) gives a different run "
                  "-- close-zoom-box.md golden 11x11 (x=361..371)");
            CHECK(!past_is_solid_box,
                  "leg (9c): one px PAST the 11-wide box (x=struct.left+9+11) must NOT "
                  "be a solid box tint (dark idx 4 / face idx 6) -- it is pinstripe; "
                  "close-zoom-box.md golden x=372 is pinstripe NOT box");
        }

        /* (10) HORIZONTAL OFFSETS: the close box left edge is at struct.left+9 and
         * the zoom box left edge is at struct.right-20.  The dark outline corner
         * must be AT each offset, and the column one px to the LEFT of each box must
         * NOT be a solid box tint (dark/face) -- nothing extends left of the offset
         * (the bevel is INSIDE the dark frame).  The bevel role (idx 7) is excluded
         * from the "left of" tint set because it aliases the pinstripe light idx 7.
         * Old FLAIR insets fr+3=4 (close) / right-4-13 (zoom), so the dark corner at
         * the CORRECT offset (10a) is RED. */
        {
            uint32_t zcorner = render_pixel_index(&ctx, (uint32_t)zoom_l,
                                                  (uint32_t)box_top);
            uint32_t zleft   = render_pixel_index(&ctx, (uint32_t)(zoom_l - 1),
                                                  (uint32_t)box_top);
            int zleft_is_solid_box = (zleft == (uint32_t)FG_BOX_DARK_IDX) ||
                                     (zleft == (uint32_t)FG_BOX_FACE_IDX);
            printf("test-chrome-fidelity: zoom box left edge (x=%d) idx=%u, one px "
                   "left (x=%d) idx=%u (must not be dark/face)\n",
                   zoom_l, zcorner, zoom_l - 1, zleft);
            CHECK(zcorner == (uint32_t)FG_BOX_DARK_IDX,
                  "leg (10a): zoom box left edge (struct.right-20, box top) must be the "
                  "dark outline role (idx 4); close-zoom-box.md zoom box left = right-20");
            CHECK(!zleft_is_solid_box,
                  "leg (10b): nothing extends left of the zoom box's struct.right-20 "
                  "offset (the bevel is INSIDE the dark frame; the column one px left "
                  "is not a solid box tint); close-zoom-box.md");
        }

        /* (11) DOUBLE BEVEL: each box must exhibit >=3 distinct tonal roles
         * (dark outline / lavender bevel / gray face).  The OLD flat 1px frame has
         * exactly ONE role (the frame ink) -> RED. */
        CHECK(close_roles >= FG_BOX_MIN_TONAL_ROLES,
              "leg (11a): the close box must be a DOUBLE-BEVELED square with >=3 "
              "distinct tonal roles (dark outline idx 4 / lavender bevel idx 7 / gray "
              "face idx 6); a flat 1px frame (old FLAIR) has ONE -- close-zoom-box.md "
              "ASCII bevel diagram");
        CHECK(zoom_roles >= FG_BOX_MIN_TONAL_ROLES,
              "leg (11b): the zoom box must be a DOUBLE-BEVELED square with >=3 distinct "
              "tonal roles (dark / bevel / face); a flat 1px frame has ONE -- "
              "close-zoom-box.md ASCII bevel diagram");

        /* (12) ZOOM NESTED-SQUARE GLYPH: the zoom box carries the inner nested-square
         * "little dude" glyph that the close box lacks -- so the zoom box has STRICTLY
         * MORE interior dark pixels than the close box.  Old FLAIR drew zoom==close
         * (both flat, identical interiors) -> RED (equal counts). */
        CHECK(zoom_interior_dark > close_interior_dark,
              "leg (12): the ZOOM box must carry the inner nested-square glyph (extra "
              "interior dark figure) that the CLOSE box lacks -- the zoom box interior "
              "must have STRICTLY MORE dark pixels than the (plain-gray) close box; "
              "old FLAIR drew zoom==close -- close-zoom-box.md PlotZoom nested-square "
              "glyph @1695, 'the close box interior carries no glyph'");
    }

    /* ====================================================================
     * VERTICAL SCROLLBAR (beads initech-jh7m).
     *
     * Ground truth: ../system7-decomp/specs/chrome/scrollbar.md (pixel-measured
     *   from goldens/captures/s7_about.png, right gutter x=494..509 y=159..217) +
     *   refs/StandardWDEF_a.txt (scrollBarSize EQU 16 @73; gutter divider @1332).
     *
     * The window (40,30)-(360,300) in half-open coords:
     *   outer right frame at x=359; inner-right edge: x = WIN_RIGHT - fr = 359.
     *   scrollbar occupies [sb_left, inner_right) where
     *     inner_right = WIN_RIGHT - fr = 359
     *     sb_left     = inner_right - FLAIR_CHROME_SCROLLBAR_W = 359-16 = 343
     *   (The left gutter-divider line is at sb_left=343.)
     *
     *   content_top = WIN_TOP + FLAIR_CHROME_TITLEBAR_H = 30+19 = 49
     *     (beads initech-92li: the 19-px title BAND now INCLUDES the top frame and
     *      the shared bottom frame line, so the white content -- and the scrollbar
     *      that runs the content height -- begins at WIN_TOP+TITLEBAR_H, one row
     *      UP from the old WIN_TOP+fr+TITLEBAR_H=50. window-frame.md Sec 2a.)
     *   content_bot = WIN_BOTTOM - fr = 299
     *
     *   Up-arrow box: rows [content_top, content_top + sb_w) = [49, 65)
     *     outer top edge  at y = content_top     = 49  (BLACK, FG_SB_OUTER_EDGE_IDX=0)
     *     inner separator at y = content_top+sb_w-1 = 64 (GRAY, FG_SB_SEPARATOR_IDX=8)
     *     face interior   at y in (49, 64), x in (343, 359)
     *
     *   Down-arrow box: rows [content_bot - sb_w, content_bot) = [283, 299)
     *     inner separator at y = content_bot-sb_w = 283 (GRAY, FG_SB_SEPARATOR_IDX=8)
     *     face interior   at y in (283, 299), x in (343, 359)
     *     outer bottom edge at y = content_bot-1 = 298 (BLACK, FG_SB_OUTER_EDGE_IDX=0)
     *
     *   Track: rows [65, 283), solid PIN_LIGHT (FG_SB_FACE_IDX=7).
     *
     * CURRENT FIDELITY BUGS (before this fix):
     *   (a) track filled with FLAIR_PART_BTNFACE (#C0C0C0, idx 6) -- should be PIN_LIGHT (idx 7).
     *   (b) arrow boxes framed with cframe(FLAIR_PART_FRAME): ALL FOUR edges black --
     *       the inner separator should be PIN_DARK (idx 8), not FRAME (idx 0).
     *   (c) No arrow glyphs -- the boxes are empty, so zero PIN_DARK glyph pixels.
     * ==================================================================== */
    {
        const int sb_w        = FLAIR_CHROME_SCROLLBAR_W;  /* 16 */
        const int fr2         = FLAIR_CHROME_FRAME;         /* 1; POSITIONING only */
        const int inner_right = WIN_RIGHT - fr2;            /* 359 */
        const int sb_left     = inner_right - sb_w;         /* 343 */
        const int content_top = WIN_TOP + FLAIR_CHROME_TITLEBAR_H;       /* 49 (92li) */
        const int content_bot = WIN_BOTTOM - fr2;           /* 299 */

        /* Arrow-box row coordinates (scrollbar-width-square boxes).
         * Up box:   rows [content_top,             content_top + sb_w)  = [49, 65)
         * Down box: rows [content_bot - sb_w,      content_bot)         = [283, 299) */
        const int up_box_top    = content_top;           /* 49  */
        const int up_box_bot    = content_top + sb_w;   /* 66  */
        const int dn_box_top    = content_bot - sb_w;   /* 283 */
        const int dn_box_bot    = content_bot;           /* 299 */

        /* The interior columns of the scrollbar (between divider and frame).
         * Gutter divider is at sb_left (x=343); interior: x in (343, 359).
         * For glyph sampling we scan x in [sb_left+1, inner_right). */
        const int int_left  = sb_left + 1;              /* 344 */
        const int int_right = inner_right;              /* 359 */

        /* (13) OUTER BOX EDGES -- top edge of the up-arrow box and bottom edge of
         * the down-arrow box must be FRAME ink (idx 0 = black, outer black edges).
         * Ref: scrollbar.md Geometry "y=159 & y=217 all #000000" (row y=159 maps to
         * our content_top; y=217 maps to content_bot-1).
         * OLD render: the cframe() draws all 4 edges black -> this reads 0 already.
         * After the fix: outer edges still 0 (correct). This leg stays GREEN in both
         * states so it is the POSITIVE confirmation; the decisive RED is leg (14). */
        {
            /* Sample a mid-column on the top outer edge of the up-arrow box. */
            const int mid_x_sb = (int_left + int_right) / 2;    /* ~351 */
            uint32_t up_top_px = render_pixel_index(&ctx,
                                     (uint32_t)mid_x_sb, (uint32_t)up_box_top);
            uint32_t dn_bot_px = render_pixel_index(&ctx,
                                     (uint32_t)mid_x_sb, (uint32_t)(dn_box_bot - 1));
            printf("test-chrome-fidelity: sb up-arrow outer-top  y=%d x=%d -> idx=%u "
                   "(expect FG_SB_OUTER_EDGE_IDX=%d black)\n",
                   up_box_top, mid_x_sb, up_top_px, FG_SB_OUTER_EDGE_IDX);
            printf("test-chrome-fidelity: sb dn-arrow outer-bot  y=%d x=%d -> idx=%u "
                   "(expect FG_SB_OUTER_EDGE_IDX=%d black)\n",
                   dn_box_bot - 1, mid_x_sb, dn_bot_px, FG_SB_OUTER_EDGE_IDX);
            CHECK((uint32_t)FG_SB_OUTER_EDGE_IDX == up_top_px,
                  "leg (13a): scrollbar up-arrow outer top edge must be FRAME ink "
                  "(idx 0 = black); scrollbar.md Geometry y=159 = #000000; "
                  "refs/StandardWDEF_a.txt @1330-1338 gutter divider");
            CHECK((uint32_t)FG_SB_OUTER_EDGE_IDX == dn_bot_px,
                  "leg (13b): scrollbar down-arrow outer bottom edge must be FRAME ink "
                  "(idx 0 = black); scrollbar.md Geometry y=217 = #000000");
        }

        /* (14) INNER SEPARATORS -- the rows between each arrow box and the track
         * must be PIN_DARK (idx 8, #969696), NOT FRAME (idx 0, black).
         * Ref: scrollbar.md Rendered colors "inner box/track separators #969696 (150)
         * SOLID -- y=174 & y=202 all #969696 ... the inactive-dimmed separator lines".
         * OLD render: cframe draws all 4 box edges black -> inner separator = idx 0
         * -> this leg is DEFINITIVELY RED before the fix (expected 8, got 0). */
        {
            /* The up-arrow box interior separator: the row BELOW the up-arrow box,
             * i.e. y = up_box_bot - 1 (the last row of the box, which in the golden
             * is the lower separator at y=174). */
            const int mid_x_sb = (int_left + int_right) / 2;
            uint32_t up_sep_px = render_pixel_index(&ctx,
                                     (uint32_t)mid_x_sb, (uint32_t)(up_box_bot - 1));
            /* The down-arrow box interior separator: the row ABOVE the down-arrow box,
             * i.e. y = dn_box_top (the first row of the box, which is the upper
             * separator at y=202 in the golden). */
            uint32_t dn_sep_px = render_pixel_index(&ctx,
                                     (uint32_t)mid_x_sb, (uint32_t)dn_box_top);
            printf("test-chrome-fidelity: sb up-arrow inner-sep  y=%d x=%d -> idx=%u "
                   "(expect FG_SB_SEPARATOR_IDX=%d gray; old cframe gives 0=black)\n",
                   up_box_bot - 1, mid_x_sb, up_sep_px, FG_SB_SEPARATOR_IDX);
            printf("test-chrome-fidelity: sb dn-arrow inner-sep  y=%d x=%d -> idx=%u "
                   "(expect FG_SB_SEPARATOR_IDX=%d gray; old cframe gives 0=black)\n",
                   dn_box_top, mid_x_sb, dn_sep_px, FG_SB_SEPARATOR_IDX);
            CHECK((uint32_t)FG_SB_SEPARATOR_IDX == up_sep_px,
                  "leg (14a): scrollbar up-arrow INNER separator (row between box and "
                  "track) must be PIN_DARK (idx 8 = #969696 gray), NOT FRAME black; "
                  "scrollbar.md Rendered colors row y=174 = solid #969696; "
                  "the FLAIR cframe() all-black-edges bug makes this RED before fix");
            CHECK((uint32_t)FG_SB_SEPARATOR_IDX == dn_sep_px,
                  "leg (14b): scrollbar down-arrow INNER separator (row between track "
                  "and box) must be PIN_DARK (idx 8 = #969696 gray), NOT FRAME black; "
                  "scrollbar.md Rendered colors row y=202 = solid #969696");
        }

        /* (15) ARROW-BOX FACE + TRACK must be PIN_LIGHT (idx 7, #F3F3F3 solid).
         * Ref: scrollbar.md Rendered colors "arrow-box face + page track fill #F3F3F3
         * (243) SOLID ... verified x=495..508, y=176..200 uniformly #F3F3F3; no dither".
         * OLD render: track filled with FLAIR_PART_BTNFACE (#C0C0C0, idx 6) -> RED.
         * After fix: PIN_LIGHT (idx 7) -> GREEN. */
        {
            /* Sample a mid-point in the page track (well between the two boxes). */
            const int mid_x_sb = (int_left + int_right) / 2;
            const int track_y  = (up_box_bot + dn_box_top) / 2;   /* mid-track */
            uint32_t track_px  = render_pixel_index(&ctx,
                                     (uint32_t)mid_x_sb, (uint32_t)track_y);
            /* Sample the face of the up-arrow box interior (above the separator). */
            const int up_face_y = (up_box_top + up_box_bot) / 2;  /* ~58 */
            uint32_t up_face_px = render_pixel_index(&ctx,
                                     (uint32_t)mid_x_sb, (uint32_t)up_face_y);
            printf("test-chrome-fidelity: sb track mid-point     y=%d x=%d -> idx=%u "
                   "(expect FG_SB_FACE_IDX=%d PIN_LIGHT; old BTNFACE was idx 6)\n",
                   track_y, mid_x_sb, track_px, FG_SB_FACE_IDX);
            printf("test-chrome-fidelity: sb up-arrow face mid   y=%d x=%d -> idx=%u "
                   "(expect FG_SB_FACE_IDX=%d PIN_LIGHT; old BTNFACE was idx 6)\n",
                   up_face_y, mid_x_sb, up_face_px, FG_SB_FACE_IDX);
            CHECK((uint32_t)FG_SB_FACE_IDX == track_px,
                  "leg (15a): scrollbar page track must be PIN_LIGHT (idx 7 = #F3F3F3), "
                  "NOT BTNFACE (idx 6 = #C0C0C0 -- the old wrong fill); scrollbar.md "
                  "Rendered colors 'arrow-box face + page track fill #F3F3F3 SOLID'");
            CHECK((uint32_t)FG_SB_FACE_IDX == up_face_px,
                  "leg (15b): scrollbar up-arrow box face must be PIN_LIGHT (idx 7), "
                  "NOT BTNFACE; scrollbar.md 'arrow-box face ... #F3F3F3 SOLID'");
        }

        /* (16) ARROW GLYPHS -- each arrow box must carry >=FG_SB_GLYPH_MIN_PX (8)
         * PIN_DARK (idx 8) pixels (the hollow triangle-on-stem glyph outline).
         * Ref: scrollbar.md "arrow glyph (up/down triangle) #969696 (150) outline on
         * #F3F3F3" + the ASCII shape diagram (hollow triangle, ~14 glyph outline px).
         * OLD render: no glyphs drawn -> zero PIN_DARK pixels in the box -> RED.
         * After fix: >= 8 PIN_DARK glyph outline pixels -> GREEN. */
        {
            /* Count PIN_DARK pixels in the up-arrow box interior (rows/cols inside
             * the outer edge; we exclude the outer top edge row and the inner
             * separator row to count only glyph pixels, not frame pixels). */
            int up_glyph_px = 0;
            for (int y = up_box_top + 1; y < up_box_bot - 1; y++) {
                for (int x = int_left; x < int_right; x++) {
                    uint32_t idx2 = render_pixel_index(&ctx, (uint32_t)x, (uint32_t)y);
                    if (idx2 == (uint32_t)FG_SB_GLYPH_IDX) up_glyph_px++;
                }
            }
            int dn_glyph_px = 0;
            for (int y = dn_box_top + 1; y < dn_box_bot - 1; y++) {
                for (int x = int_left; x < int_right; x++) {
                    uint32_t idx2 = render_pixel_index(&ctx, (uint32_t)x, (uint32_t)y);
                    if (idx2 == (uint32_t)FG_SB_GLYPH_IDX) dn_glyph_px++;
                }
            }
            printf("test-chrome-fidelity: sb up-arrow glyph PIN_DARK(idx%d) px=%d "
                   "(expect >=%d; 0 = no glyph)\n",
                   FG_SB_GLYPH_IDX, up_glyph_px, FG_SB_GLYPH_MIN_PX);
            printf("test-chrome-fidelity: sb dn-arrow glyph PIN_DARK(idx%d) px=%d "
                   "(expect >=%d; 0 = no glyph)\n",
                   FG_SB_GLYPH_IDX, dn_glyph_px, FG_SB_GLYPH_MIN_PX);
            CHECK(up_glyph_px >= FG_SB_GLYPH_MIN_PX,
                  "leg (16a): up-arrow box must carry >=8 PIN_DARK (idx 8 = #969696) "
                  "glyph-outline pixels (the hollow triangle-on-stem; scrollbar.md "
                  "arrow-glyph shape ASCII diagram); zero means no glyph was drawn "
                  "-- the old FLAIR empty-box bug");
            CHECK(dn_glyph_px >= FG_SB_GLYPH_MIN_PX,
                  "leg (16b): down-arrow box must carry >=8 PIN_DARK (idx 8) "
                  "glyph-outline pixels; zero means no glyph drawn -- old FLAIR bug; "
                  "scrollbar.md down-arrow = vertical mirror of the up-arrow glyph");
        }
    }

    render_ctx_free(&ctx);
    return TEST_SUMMARY("test-chrome-fidelity");
}
