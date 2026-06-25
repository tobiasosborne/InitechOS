/*
 * os/flair/chrome.c -- the FLAIR System-7 window chrome drawer (THE ARTIFACT).
 *
 * beads: initech-k8o5.8. See chrome.h for the contract + Law-3 separation.
 *
 * Freestanding artifact code: draws one System-7 documentProc window's chrome
 * through a GrafPort, clipping every pixel to visRgn INTERSECT clipRgn (D-1/D-2),
 * writing ONLY via the ONE surface module (os/flair/surface.h). Dimensions are
 * the LOCKED native constants from spec/chrome_metrics.h (== chrome_metrics.json).
 *
 * COLOR MODEL (C-8; ADR-0004-AMENDMENT-DEC-09 Sec 3.1/3.3). chrome.c is a
 * DECORATION policy: it keeps the System-7 window GEOMETRY but names NO color.
 * It names a wctb-keyed PART (FLAIR_PART_*) and resolves PART -> destination
 * pixel ONLY through the ONE policy seam flair_look_pixel(port, PART)
 * (os/flair/flair_look.h).  The seam is the single site that turns an index
 * into a color (via flair_canon_rgb over the locked color_canon.h) and
 * device-quantizes for the port depth.  This TU ships ZERO 0xRRGGBB literal,
 * ZERO INITECH_*_RGB, ZERO index->RGB switch -- the C-8 cut-line (constraint
 * C-8).  The PART enum keys on the wctb namespace so the seam KEY matches the
 * golden KEY (the value oracle diffs key-for-key; ADR-0010).
 *
 * MUTATION HOOKS (Rule 6; FO-2/AM-3): three named mutants compiled via -D, each
 * a single deliberate defect that test-chrome must catch:
 *   CHROME_MUTATE_TITLEBAR_H   -- title bar drawn 1 px too tall.
 *   CHROME_MUTATE_NO_FRAME     -- the 1 px window frame is skipped.
 *   CHROME_MUTATE_SCROLLBAR_W  -- the scrollbar drawn 15 px wide (not 16).
 * In a normal build none is defined and the chrome is correct.
 *
 * Ref: spec/chrome_metrics.h / chrome_metrics.json (LOCKED); StandardWDEF.a
 *      (WDEF constants); gui-ground-truth.md Sec 3.3/4.2 (chimera element map).
 *      CLAUDE.md Law 1/2/4, Rule 2/6/11/12.
 */

#include <stdint.h>

#include "chrome.h"
#include "surface.h"            /* surface_fill_span (-Ios/flair)             */
#include "chrome_metrics.h"     /* FLAIR_CHROME_* (-Ispec)                    */
#include "region_algebra.h"     /* region_contains_point (-Ispec)            */
#include "flair_look.h"         /* flair_look_pixel + FLAIR_PART_* (the seam) */
#include "text.h"               /* text_draw/measure/cell_height (inline)     */

/* ---------------------------------------------------------------------------
 * clip_in -- is port-local pixel (x,y) inside visRgn INTERSECT clipRgn?
 *
 * The load-bearing clip rule (D-1/D-2): NO pixel is written outside the
 * intersection of the visible region (Window Manager) and the application clip.
 * A NULL clipRgn means "no additional clip" (full visRgn), per grafport.h.
 * ------------------------------------------------------------------------- */
static int clip_in(const GrafPort *port, int x, int y)
{
    if (x < 0 || y < 0) {
        return 0;
    }
    if (port->visRgn != 0 &&
        !region_contains_point(port->visRgn, (int16_t)x, (int16_t)y)) {
        return 0;
    }
    if (port->clipRgn != 0 &&
        !region_contains_point(port->clipRgn, (int16_t)x, (int16_t)y)) {
        return 0;
    }
    return 1;
}

/* ---------------------------------------------------------------------------
 * cfill -- fill the half-open span [x, x+w) on row y with the color of PART
 * `part`, clipped to visRgn INTERSECT clipRgn, writing only through the surface
 * module.  The PART -> destination pixel resolution is the ONE policy seam
 * (flair_look_pixel; C-8): chrome.c names a PART, never a color.
 *
 * Walks the span, batching maximal in-clip runs into single surface_fill_span
 * calls. The surface module additionally clips to the bitmap bounds (Rule 2).
 * ------------------------------------------------------------------------- */
static void cfill(GrafPort *port, int x, int y, int w, int part)
{
    if (w <= 0) {
        return;
    }
    uint32_t px = flair_look_pixel(port, part);     /* the ONE color seam (C-8) */
    int run_start = -1;
    for (int i = 0; i <= w; i++) {
        int cx = x + i;
        int in = (i < w) ? clip_in(port, cx, y) : 0;
        if (in && run_start < 0) {
            run_start = cx;
        } else if (!in && run_start >= 0) {
            surface_fill_span(&port->portBits.bm,
                              (uint32_t)run_start, (uint32_t)y,
                              (uint32_t)(cx - run_start), px);
            run_start = -1;
        }
    }
}

/* Fill a solid rectangle [x0,x1) x [y0,y1) with PART `part`. */
static void crect(GrafPort *port, int x0, int y0, int x1, int y1, int part)
{
    for (int y = y0; y < y1; y++) {
        cfill(port, x0, y, x1 - x0, part);
    }
}

/* Frame (1 px hollow outline) of [x0,x1) x [y0,y1) with PART `part`. */
static void cframe(GrafPort *port, int x0, int y0, int x1, int y1, int part)
{
    if (x1 <= x0 || y1 <= y0) {
        return;
    }
    cfill(port, x0, y0,     x1 - x0, part);         /* top edge    */
    cfill(port, x0, y1 - 1, x1 - x0, part);         /* bottom edge */
    for (int y = y0; y < y1; y++) {
        cfill(port, x0,     y, 1, part);            /* left edge   */
        cfill(port, x1 - 1, y, 1, part);            /* right edge  */
    }
}

/* ---------------------------------------------------------------------------
 * cbox -- draw one close/zoom title-bar gadget: a double-beveled 11x11 square at
 * top-left (bx0, by0) (half-open extent [bx0, bx0+11) x [by0, by0+11)).
 *
 * Ref: ../system7-decomp/specs/chrome/close-zoom-box.md (ASCII bevel diagram +
 *   Rendered colors).  The gadget reads (golden s7_doc_window.png x=361..371,
 *   y=168..178) as a 3-D double bevel (lavender bevel INSIDE the dark frame):
 *     row 0  : D D D D D D D D D D D    -- top dark OUTER frame
 *     row 1  : D ^ ^ ^ ^ ^ ^ ^ ^ ^ ^   -- inner lavender bevel highlight (top)
 *     rows2-8: D ^ g g g g g g g D ^    -- col0 D, col1 ^, 7x7 gray FACE, col9 D
 *                                          (inner-right dark), col10 ^ (lavender)
 *     row 9  : D ^ D D D D D D D D ^    -- inner-bottom dark ring
 *     row 10 : D ^ ^ ^ ^ ^ ^ ^ ^ ^ ^   -- bottom lavender bevel
 *   D = dark outline (#545487, the wDTingeF/wFrameColor dark tinge),
 *   ^ = lavender bevel highlight (#DADAFF, the wLTinge0 light tinge),
 *   g = 7x7 recessed gray face (#C0C0C0).
 *
 * MECHANISM/POLICY (C-8): every pixel resolves through the flair_look_pixel seam
 * by PART, never a color literal.  The recolor-invariant tonal roles map:
 *   dark outline -> FLAIR_PART_BEVEL_SHADOW (canon teal-dark; 8bpp idx 4),
 *   lavender bevel -> FLAIR_PART_BEVEL_LIGHT (canon TEAL, the WL-0053 lavender->teal
 *                     recolor of the wLTinge0 #DADAFF bevel; 8bpp idx 2),
 *   gray face -> FLAIR_PART_BTNFACE (#C0C0C0 control face; 8bpp idx 6).
 * If `zoom` is non-zero the box additionally carries the inner nested-square
 * "little dude" glyph (PlotZoom, close-zoom-box.md WDEF @1695); the close box
 * (zoom == 0) leaves the face plain gray.
 *
 * Defined only when the box is drawn the real way: the CHROME_FID_MUT_BOX_GEOM
 * mutant (Rule 6) reverts to the old flat cframe path and never calls cbox, so it
 * is compiled out there to keep -Werror=unused-function clean.
 * ------------------------------------------------------------------------- */
#if !defined(CHROME_FID_MUT_BOX_GEOM)
static void cbox(GrafPort *port, int bx0, int by0, int zoom)
{
    const int sz = FLAIR_CHROME_WBOX_RENDER;     /* 11 px (close-zoom-box.md)   */
    const int dark  = FLAIR_PART_BEVEL_SHADOW;   /* dark outline #545487 -> canon teal-dark */
    const int bevel = FLAIR_PART_BEVEL_LIGHT;    /* lavender bevel #DADAFF -> canon TEAL
                                                  * (WL-0053 lavender->teal; NOT pinstripe
                                                  * near-white -- same lavender role as the
                                                  * title bevel, beads initech-92li). */
    const int face  = FLAIR_PART_BTNFACE;        /* 7x7 gray face (#C0C0C0)     */
    int bx1 = bx0 + sz;                          /* half-open right (exclusive) */
    int by1 = by0 + sz;                          /* half-open bottom (exclusive)*/
    int last = sz - 1;                           /* col/row index 10            */

    /* Fill the whole gadget with the lavender bevel first; the dark frame, gray
     * face and inner-dark ring overwrite from there (matches the WDEF erase +
     * tinge order). */
    crect(port, bx0, by0, bx1, by1, bevel);

    /* row 0: top dark OUTER frame (all 11 columns). */
    cfill(port, bx0, by0, sz, dark);
    /* col 0: left dark OUTER frame (all 11 rows). */
    for (int y = by0; y < by1; y++) {
        cfill(port, bx0, y, 1, dark);
    }
    /* the 7x7 recessed gray FACE: cols [2..8], rows [2..8] (inset 2 from edges). */
    crect(port, bx0 + 2, by0 + 2, bx0 + sz - 2, by0 + sz - 2, face);
    /* inner-RIGHT dark ring: col (sz-2)=9, rows [2 .. sz-2)=2..8. */
    for (int y = by0 + 2; y < by1 - 2; y++) {
        cfill(port, bx0 + last - 1, y, 1, dark);
    }
    /* inner-BOTTOM dark ring: row (sz-2)=9, cols [2 .. sz-2)=2..8. */
    cfill(port, bx0 + 2, by0 + last - 1, sz - 4, dark);

    if (zoom) {
        /* The nested-square glyph: a small dark square centred in the 7x7 face
         * (PlotZoom "little dude", close-zoom-box.md WDEF @1695).  A hollow 3x3
         * dark frame at the face centre -- extra interior dark figure the close
         * box lacks.  Centre of the 11x11 box is (sz/2, sz/2) = (5,5); the glyph
         * frame spans cols [4..6], rows [4..6]. */
        cframe(port, bx0 + 4, by0 + 4, bx0 + 7, by0 + 7, dark);
    }
}
#endif /* !CHROME_FID_MUT_BOX_GEOM */

/* ---------------------------------------------------------------------------
 * flair_draw_document_window -- the chrome composition (top to bottom).
 * ------------------------------------------------------------------------- */
void flair_draw_document_window(GrafPort *port, rgn_rect_t frame,
                                const char *title)
{
    if (port == 0) {
        return;
    }
    int left   = frame.left;
    int top    = frame.top;
    int right  = frame.right;
    int bottom = frame.bottom;
    int w = right - left;
    int h = bottom - top;

    /* The window must be big enough to hold its own chrome (title bar + frame +
     * a row of content + the scrollbar width). Otherwise no-op (fail-soft; the
     * Window Manager sizes the window, Rule 2 -- never draw garbage). */
    int min_h = FLAIR_CHROME_TITLEBAR_H + 2 * FLAIR_CHROME_FRAME + 2;
    int min_w = FLAIR_CHROME_SCROLLBAR_W + 2 * FLAIR_CHROME_FRAME + 2;
    if (w < min_w || h < min_h) {
        return;
    }

    const int fr = FLAIR_CHROME_FRAME;           /* 1 px frame                  */

    /* The title-bar INTERIOR sits just inside the top frame line (drawn by the
     * outer cframe in section 5 at y=top).  title_top is the first INTERIOR row
     * (the top bevel highlight), NOT the top frame line.
     *
     * BAND DECOMPOSITION (../system7-decomp/specs/chrome/window-frame.md Sec 2a,
     * the x=400 vertical scan, top frame at y=164):
     *   y=164 top frame (black)                       -> y=top      (outer cframe)
     *   y=165 bevel-hi  #DADAFF (wLTinge0)             -> y=title_top      (BEVEL_LIGHT)
     *   y=166..180 = 15 pinstripe rows                 -> [title_top+1, +16)
     *   y=181 bevel-lo  #B3B3DA (wLTinge4)             -> y=title_top+16   (BEVEL_SHADOW)
     *   y=182 SHARED frame line (black; bottom of the title FrameRect AND top of
     *         the content-body FrameRect)              -> y=title_top+17   (FRAME)
     * So the 19-px title band (FLAIR_CHROME_TITLEBAR_H) = top-frame(1) +
     * bevel-hi(1) + 15 stripe + bevel-lo(1) + shared-frame(1) = 19, and the white
     * content begins one row below the shared line at content_top = top+TITLEBAR_H
     * (beads initech-92li; window-frame.md Sec 2a/2b + pinstripe.md y=165..181). */
    const int bevel_rows  = FLAIR_CHROME_TITLE_BEVEL_ROWS;   /* 1 (each edge)    */
#if defined(CHROME_MUTATE_TITLEBAR_H)
    /* MUTANT: title bar 1 px too tall (FO-2/AM-3). test-chrome must catch this:
     * the stripe band runs one scanline past the locked 15 px, pushing the shared
     * frame line + the content body one row past the locked 19 px band. */
    const int stripe_rows = FLAIR_CHROME_TITLE_STRIPE_ROWS + 1;
#else
    const int stripe_rows = FLAIR_CHROME_TITLE_STRIPE_ROWS;  /* 15 (golden)      */
#endif

    int title_top   = top + fr;                  /* first interior row (bevel-hi)*/
    int stripe_top  = title_top + bevel_rows;    /* first of the 15 stripe rows  */
    int stripe_bot  = stripe_top + stripe_rows;  /* half-open; bevel-lo row      */
    int shared_line = stripe_bot + bevel_rows;   /* the shared bottom frame line */

    /* 1a. Top bevel HIGHLIGHT row (wLTinge0 #DADAFF -> the WL-0053 lavender->teal
     * recolor canon TEAL, FLAIR_PART_BEVEL_LIGHT, 8bpp idx 2 -- the SAME bevel-hi
     * role the close/zoom box uses, beads initech-ts3t; window-frame.md Sec 2b
     * golden y=165 #DADAFF).  Spans the inner width (inside the left/right frame
     * lines).  C-8 seam: a PART, never a color. */
#if defined(CHROME_FID_MUT_NO_BEVEL)
    /* MUTANT (Rule 6; beads initech-92li): SKIP the two bevel rows -- revert to the
     * old all-stripe interior (the full [title_top, shared_line) band -- bevel-hi +
     * 15 stripe + bevel-lo, = 17 rows -- ALL filled as pinstripe, no BEVEL_LIGHT /
     * BEVEL_SHADOW edge rows).  The contiguous L/D stripe run is then 17, NOT the
     * golden 15, AND there is no bevel-hi (idx 2) above / bevel-lo (idx 4) below it,
     * so test-chrome-fidelity's NEW bevel + 15-row leg MUST go RED. */
    {
        int interior_rows = stripe_rows + 2 * bevel_rows;   /* 17 */
        for (int y = title_top; y < shared_line; y++) {
            int k = y - title_top;               /* 0 .. interior_rows-1         */
            int light = (k <= 1) || (k >= interior_rows - 2) ||
                        ((k % FLAIR_CHROME_PINSTRIPE_PERIOD) == 1);
            int shade = light ? FLAIR_PART_PIN_LIGHT : FLAIR_PART_PIN_DARK;
            cfill(port, left + fr, y, w - 2 * fr, shade);
        }
    }
#else
    for (int by = title_top; by < stripe_top; by++) {
        cfill(port, left + fr, by, w - 2 * fr, FLAIR_PART_BEVEL_LIGHT);
    }

    /* 1b. The 15-row phase-locked pinstripe (the System-7 "racing stripe") --
     * the period-2 horizontal stripe (HilitePattern $FF00) PHASE-LOCKED to the
     * window structure so a LIGHT row lands at BOTH stripe-band edges, yielding
     * the doubled-LIGHT row pairs the golden shows
     * (../system7-decomp/specs/chrome/pinstripe.md, golden s7_doc_window.png
     * column x=450: interior y=166/167 both light .. y=179/180 both light;
     * the exactly-15-row interior = FG_TITLE_INTERIOR_PATTERN "LLDLDLDLDLDLDLL").
     * A free-running period-2 fill anchored to the band top (L,D,L,D...) is WRONG
     * -- it has NO doubled-light pairs, yet still satisfies a naive "period-2
     * alternation" check; graded against the INDEPENDENT decomp golden by
     * test-chrome-fidelity (beads initech-hmll/92li, Law 2). The stripe run is now
     * bounded ABOVE by the bevel-hi row and BELOW by the bevel-lo row, so the
     * contiguous L/D run is EXACTLY 15.  Shade indices are the WDEF wTitleBarLight
     * (7) / wTitleBarDark (8). */
    for (int y = stripe_top; y < stripe_bot; y++) {
        int j = y - stripe_top;                  /* 0 .. stripe_rows-1           */
#if defined(CHROME_FID_MUT_PHASE)
        /* MUTANT (Rule 6; beads initech-hmll): revert to the free-running period-2
         * fill anchored to the stripe top -- no phase lock, no doubled-light pairs.
         * test-chrome-fidelity MUST go RED. */
        int light = ((j % FLAIR_CHROME_PINSTRIPE_PERIOD) == 0);
#else
        /* Phase lock: LIGHT at both stripe-band edges (the top pair j<=1 and the
         * bottom pair j>=stripe_rows-2) and on the odd interior rows; DARK on the
         * even interior rows -- reproducing the golden's doubled-light-pairs
         * signature "LLDLDLDLDLDLDLL" over the exactly-15-row run. */
        int light = (j <= 1) || (j >= stripe_rows - 2) ||
                    ((j % FLAIR_CHROME_PINSTRIPE_PERIOD) == 1);
#endif
        int shade = light ? FLAIR_PART_PIN_LIGHT : FLAIR_PART_PIN_DARK;
        cfill(port, left + fr, y, w - 2 * fr, shade);
    }

    /* 1c. Bottom bevel SHADOW row (wLTinge4 #B3B3DA -> the canon teal-dark
     * recolor, FLAIR_PART_BEVEL_SHADOW, 8bpp idx 4 -- the SAME bevel-lo role the
     * close/zoom box dark outline uses; window-frame.md Sec 2b golden y=181
     * #B3B3DA). */
    for (int by = stripe_bot; by < shared_line; by++) {
        cfill(port, left + fr, by, w - 2 * fr, FLAIR_PART_BEVEL_SHADOW);
    }
#endif /* CHROME_FID_MUT_NO_BEVEL */

    /* 1d. The SHARED bottom frame line (black): the bottom edge of the title-bar
     * FrameRect AND the top edge of the content-body FrameRect (window-frame.md
     * Sec 2a golden y=182 #000000 "the shared line").  Spans the inner width;
     * the outer left/right frame columns are drawn by the outer cframe (section
     * 5).  C-8 seam (FLAIR_PART_FRAME = wFrameColor = black). */
    cfill(port, left + fr, shared_line, w - 2 * fr, FLAIR_PART_FRAME);

    /* 2. Close box (top-left) and zoom box (top-right): each a double-beveled
     * 11x11 gadget (NOT a flat 1px frame, NOT 13x13).  Geometry from
     * ../system7-decomp/specs/chrome/close-zoom-box.md:
     *   - the gadget RENDERS FLAIR_CHROME_WBOX_RENDER (11) px square (the WDEF
     *     derives 13 but the bevel sits INSIDE the dark frame; LAW 2 golden wins);
     *   - close box LEFT edge = struct.left + 9 (PlotGoAway 'moveq #9,D1' @1675-1678);
     *   - zoom box  LEFT edge = struct.right - 20 (PlotZoom 'left:=right-20' @1682-1693);
     *   - box TOP = struct.top + wBoxDelta + 1, wBoxDelta = (titleHgt-13)/2 = 3
     *     (so box top = frame_top + 4; WDEF @1705-1707).  `top` is the outer frame
     *     top (struct.top); title_top = top + fr.
     * The zoom box additionally carries the nested-square glyph; the close box
     * does not (cbox `zoom` flag). */
    {
        int wbox_delta = (FLAIR_CHROME_TITLEBAR_H - FLAIR_CHROME_WBOX_DELTA) / 2;
        if (wbox_delta < 0) {
            wbox_delta = 0;
        }
#if defined(CHROME_FID_MUT_BOX_GEOM)
        /* MUTANT (Rule 6; beads initech-ts3t): revert to the OLD flat 1px 13x13
         * box at inset fr+3 with NO bevel and NO zoom glyph (zoom == close).
         * test-chrome-fidelity's box legs (9)/(10)/(11)/(12) MUST go RED:
         * wrong size (13 not 11), wrong offset (fr+3 not +9/-20), one tonal role
         * (flat frame), and zoom identical to close (no nested-square glyph). */
        {
            int box = FLAIR_CHROME_WBOX_DELTA;   /* 13 px */
            int by0 = (top + fr) + wbox_delta;   /* old centering (title_top+vpad) */
            int margin = fr + 3;                 /* old inset */
            int cx0 = left + margin;
            cframe(port, cx0, by0, cx0 + box, by0 + box, FLAIR_PART_FRAME);
            int zx1 = right - margin;
            cframe(port, zx1 - box, by0, zx1, by0 + box, FLAIR_PART_FRAME);
        }
#else
        int box_top = top + wbox_delta + 1;      /* struct.top + wBoxDelta + 1   */
        int close_x = left + 9;                  /* struct.left + 9 (PlotGoAway) */
        int zoom_x  = right - 20;                /* struct.right - 20 (PlotZoom) */
        cbox(port, close_x, box_top, 0);         /* close box (no glyph)         */
        cbox(port, zoom_x,  box_top, 1);         /* zoom box  (nested-square)    */
#endif
    }

    /* 2.5 Title text: the window's name, drawn CENTERED in the title bar in
     * Chicago over a KNOCKED-OUT light gap. System 7 suppresses the racing stripe
     * under the centered title and draws black Chicago glyphs there (golden
     * s7_doc_window.png: a centered #F3F3F3 gap with #000000 glyphs;
     * ../system7-decomp/specs/chrome/title-bar.md Sec 3). surface_blit writes the
     * glyph-cell background OPAQUELY, so drawing the title with bg = the pinstripe
     * LIGHT shade paints the knockout panel AND the text in one pass. Both the ink
     * and the knockout resolve through the C-8 policy seam (flair_look_pixel) --
     * NEVER a color literal -- so test-flair-mechanism-colorblind stays green
     * (beads initech-lxg9). The centering indent is clamped right of the close box
     * (WDEF reserves the go-away box; title-bar.md "indent x=left+32"). */
#if defined(CHROME_FID_MUT_NO_TITLE)
    /* MUTANT (Rule 6; beads initech-lxg9): skip the title render -> a blank title
     * bar. test-chrome-fidelity's title-ink + knockout legs MUST go RED. */
    (void)title;
#else
    if (title != 0 && title[0] != '\0') {
        int cell_h = text_cell_height(FONT_CHICAGO);
        int tw     = text_measure(FONT_CHICAGO, title);
        /* Clamp the centered title right of the close box: the close box left edge
         * is struct.left + 9 and it renders FLAIR_CHROME_WBOX_RENDER (11) px wide,
         * so its right edge is left+9+11 = left+20; +3 px gap = left+23.  Recomputed
         * from the NEW close-box geometry (close-zoom-box.md; was fr+3+WBOX_DELTA+2
         * for the old 13px box at inset fr+3). */
        int box_clear = 9 + FLAIR_CHROME_WBOX_RENDER + 3;     /* right of close box */
        int tx = left + fr + (w - 2 * fr - tw) / 2;           /* centered          */
        if (tx < left + box_clear) {
            tx = left + box_clear;
        }
        /* Center the title in the 15-row STRIPE band (not the whole title band):
         * the knockout panel is drawn with bg = PIN_LIGHT, so the glyph cells must
         * land on the pinstripe rows -- NOT over the bevel-hi/bevel-lo or the shared
         * frame line.  Ref: title-bar.md Sec 3; beads initech-92li recomposition. */
        int ty = stripe_top + (stripe_rows - cell_h) / 2;     /* vertical center   */
        if (ty < stripe_top) {
            ty = stripe_top;
        }
        uint32_t ink   = flair_look_pixel(port, FLAIR_PART_TEXT);      /* seam, black */
        uint32_t knock = flair_look_pixel(port, FLAIR_PART_PIN_LIGHT); /* seam, light */
        text_draw(&port->portBits.bm, tx, ty, title, FONT_CHICAGO, ink, knock);
    }
#endif /* CHROME_FID_MUT_NO_TITLE */

    /* 3. The content area: white body below the title bar, inside the frame and
     * to the left of the scrollbar. Drawn before the scrollbar so the scrollbar
     * sits on top of (overlaps) the right content edge by 1 px frame (Toolbox
     * 15-vs-16; gui-ground-truth.md Sec 3.4).
     *
     * The white body begins ONE row below the shared frame line (window-frame.md
     * Sec 2a: the shared black line at golden y=182 is the content-body FrameRect
     * top; the white content begins at y=183 = content_top).  In the canonical
     * (non-mutant) build content_top = top + FLAIR_CHROME_TITLEBAR_H (the 19-px
     * title band -- top-frame + interior + shared-frame -- precedes the body).
     * The recomposition shifted content_top UP by 1 vs the old all-stripe band
     * (old content_top = title_top + TITLEBAR_H = top + 1 + 19; new = top + 19),
     * because the golden's 19-row band is measured INCLUSIVE of both frame lines.
     * beads initech-92li. */
    int content_top = shared_line + fr;          /* one row below the shared line*/
    int content_bot = bottom - fr;               /* above the bottom frame      */
    int content_left = left + fr;
    int content_right = right - fr;
    crect(port, content_left, content_top, content_right, content_bot,
          FLAIR_PART_CONTENT);

#if defined(FLAIR_COLORBLIND_MUTANT)
    /* NAMED MUTANT (Rule 6; ADR-0004-AMENDMENT-DEC-09 Sec 3.10 #2): a
     * resurrected color in a decoration draw -- one span written with a color
     * COMPUTED straight to the surface, BYPASSING the C-8 policy seam
     * (flair_look_pixel).  The value is computed (no bare color literal token)
     * precisely so the SOURCE scanner (test-mech-policy) cannot see it -- this
     * is exactly the obfuscated/computed-literal case that the BEHAVIORAL
     * colorblind oracle exists to catch: it renders non-sentinel pixels and
     * test-flair-mechanism-colorblind MUST go RED.  Default builds never define
     * this; it is a host-oracle-only perturbation, never shipped. */
    {
        uint32_t orange = ((uint32_t)0xFFu << 16) | ((uint32_t)0x88u << 8); /* != sentinel */
        surface_fill_span(&port->portBits.bm,
                          (uint32_t)content_left, (uint32_t)content_top,
                          (uint32_t)(content_right - content_left), orange);
    }
#endif

    /* 4. Vertical scrollbar on the right: FLAIR_CHROME_SCROLLBAR_W (16) px wide,
     * running the height of the content area.  Inactive (no thumb) per the golden
     * (s7_about.png, right gutter x=494..509 y=159..217; scrollbar.md).
     *
     * STRUCTURE (scrollbar.md Geometry + Rendered colors; Law 2 golden wins):
     *   - 1 px black gutter-divider at sb_left (WDEF @1332; FLAIR_PART_FRAME idx 0)
     *   - 14 px interior: track + arrow boxes (x in (sb_left, sb_right))
     *   - 1 px black right window-frame (drawn in section 5; shared line)
     *
     *   Arrow boxes: top square + bottom square, each sb_w (16) px tall.
     *   - Entire band (track + boxes) filled PIN_LIGHT (idx 7, #F3F3F3) as base.
     *   - Gutter-divider column (x=sb_left): FRAME/black for full height.
     *   - Up-arrow box: outer TOP edge (y=sb_top) = FRAME black (idx 0).
     *     Inner lower separator (y=sb_top+sb_w-1) = PIN_DARK gray (idx 8, #969696).
     *     The remaining edges (left col=divider, right col=frame) are already
     *     handled by the divider line and the window frame (section 5).
     *   - Down-arrow box: inner upper separator (y=sb_bot-sb_w) = PIN_DARK (idx 8).
     *     Outer BOTTOM edge (y=sb_bot-1) = FRAME black (idx 0).
     *
     *   Arrow GLYPHS (inactive/dimmed = gray outline; scrollbar.md ASCII diagram):
     *   Up arrow: a hollow triangle-on-stem in PIN_DARK (idx 8).  Interior is
     *     sb_w-2 cols wide (inside outer left/right lines).  The glyph spans ~10
     *     rows with apex ~3 px from the box top; two vertical shaft strokes close
     *     with a base bar.  Using the 14-px interior (cols relative to int_left):
     *
     *       row+2  (apex):        ......mm......   col 6,7
     *       row+3:                .....m..m.....   col 5,8
     *       row+4:                ....m....m....   col 4,9
     *       row+5:                ...m......m...   col 3,10
     *       row+6:                ..m........m..   col 2,11
     *       row+7 (barbs):        .mmmm....mmmm.   col 1-4,9-12
     *       row+8:                ....m....m....   col 4,9
     *       row+9:                ....m....m....   col 4,9
     *       row+10:               ....m....m....   col 4,9
     *       row+11 (base):        ....mmmmmm....   col 4-9
     *
     *   Down arrow: vertical mirror (apex near bottom of box).
     *   All rendered through FLAIR_PART_PIN_DARK (the C-8 seam; no color literal).
     *   Inactive bar carries NO thumb (golden-resolves: active thumb = CDEF).
     *
     * RECOLOR-INVARIANCE: FLAIR_PART_FRAME (idx 0), FLAIR_PART_PIN_LIGHT (idx 7),
     * FLAIR_PART_PIN_DARK (idx 8) only -- zero color literals (C-8 seam).
     *
     * Ref: ../system7-decomp/specs/chrome/scrollbar.md (golden s7_about.png;
     *   Geometry + Rendered colors + arrow-glyph shape); CLAUDE.md Law 1/2/4;
     *   beads initech-jh7m. */
#if defined(CHROME_MUTATE_SCROLLBAR_W)
    /* MUTANT: scrollbar 15 px wide, not 16 (FO-2/AM-3). test-chrome must catch
     * that the scrollbar column is one pixel too narrow. */
    int sb_w = FLAIR_CHROME_SCROLLBAR_W - 1;
#else
    int sb_w = FLAIR_CHROME_SCROLLBAR_W;         /* 16 px (WDEF scrollBarSize)  */
#endif
    int sb_right = right - fr;                    /* inside the right frame line */
    int sb_left  = sb_right - sb_w;
    int sb_top   = content_top;
    int sb_bot   = content_bot;
    if (sb_left > content_left) {
#if defined(CHROME_FID_MUT_SCROLL_FLAT)
        /* MUTANT (Rule 6; beads initech-jh7m): revert to the OLD render --
         * BTNFACE track (idx 6, wrong), all-black cframe edges (inner separators
         * black not gray), no arrow glyphs (empty boxes).
         * test-chrome-fidelity legs (14)/(15)/(16) MUST go RED. */
        crect(port, sb_left, sb_top, sb_right, sb_bot, FLAIR_PART_BTNFACE);
        for (int y = sb_top; y < sb_bot; y++) {
            cfill(port, sb_left, y, 1, FLAIR_PART_FRAME);
        }
        {
            int btn = sb_w;
            if (sb_top + btn <= sb_bot) {
                cframe(port, sb_left, sb_top, sb_right, sb_top + btn, FLAIR_PART_FRAME);
            }
            if (sb_bot - btn >= sb_top) {
                cframe(port, sb_left, sb_bot - btn, sb_right, sb_bot, FLAIR_PART_FRAME);
            }
        }
#else
        /* --- STEP 1: fill the whole scrollbar band PIN_LIGHT (track + box faces). */
        crect(port, sb_left, sb_top, sb_right, sb_bot, FLAIR_PART_PIN_LIGHT);

        /* --- STEP 2: gutter-divider column (x=sb_left) = FRAME/black for full
         * height. (StandardWDEF_a.txt @1330-1338; scrollbar.md Geometry.) */
        for (int y = sb_top; y < sb_bot; y++) {
            cfill(port, sb_left, y, 1, FLAIR_PART_FRAME);
        }

        /* --- STEP 3: up-arrow box separators + outer edge.
         * Outer top edge (row y=sb_top): FRAME black.
         * Inner lower separator (row y=sb_top+sb_w-1): PIN_DARK gray.
         * Ref: scrollbar.md Geometry "y=159 black / y=174 gray". */
        if (sb_top + sb_w <= sb_bot) {
            /* outer top edge of up-arrow box */
            cfill(port, sb_left, sb_top, sb_w, FLAIR_PART_FRAME);
            /* inner lower separator (between up-arrow box and track) */
            cfill(port, sb_left, sb_top + sb_w - 1, sb_w, FLAIR_PART_PIN_DARK);
        }

        /* --- STEP 4: down-arrow box separators + outer edge.
         * Inner upper separator (row y=sb_bot-sb_w): PIN_DARK gray.
         * Outer bottom edge (row y=sb_bot-1): FRAME black.
         * Ref: scrollbar.md Geometry "y=202 gray / y=217 black". */
        if (sb_bot - sb_w >= sb_top) {
            /* inner upper separator (between track and down-arrow box) */
            cfill(port, sb_left, sb_bot - sb_w, sb_w, FLAIR_PART_PIN_DARK);
            /* outer bottom edge of down-arrow box */
            cfill(port, sb_left, sb_bot - 1, sb_w, FLAIR_PART_FRAME);
        }

        /* --- STEP 5: arrow glyphs (inactive/dimmed, PIN_DARK gray outline).
         * Table-driven to minimise compiled code size.  Each glyph row is encoded
         * as two span descriptors {x_off, width} relative to int_left (sb_left+1).
         * A row with two separate spans uses both entries; a solid span has the
         * second entry start past the interior (terminator: x2=99).
         * Ref: scrollbar.md arrow-glyph shape ASCII (INACTIVE hollow outline;
         * 14-col interior x=0..13 relative to int_left). */
        {
            /* Glyph row table for the UP-arrow (top-down, rows 1..10 inside box).
             * Each entry: { x1_off, w1, x2_off, w2 } where x2_off>=14 => no 2nd span.
             *   row 1: apex   col 6..7    (1 span, w=2)
             *   row 2:        col 5, 8    (2 spans, w=1 each)
             *   row 3:        col 4, 9
             *   row 4:        col 3, 10
             *   row 5:        col 2, 11
             *   row 6: barbs  col 1..4, col 9..12  (2 spans)
             *   row 7: shaft  col 4, 9
             *   row 8: shaft  col 4, 9
             *   row 9: shaft  col 4, 9
             *   row10: base   col 4..9   (1 span, w=6) */
            static const signed char up_glyph[10][4] = {
                { 6, 2, 99, 0 },   /* row 1: apex */
                { 5, 1,  8, 1 },   /* row 2 */
                { 4, 1,  9, 1 },   /* row 3 */
                { 3, 1, 10, 1 },   /* row 4 */
                { 2, 1, 11, 1 },   /* row 5 */
                { 1, 4,  9, 4 },   /* row 6: barbs */
                { 4, 1,  9, 1 },   /* row 7: shaft */
                { 4, 1,  9, 1 },   /* row 8: shaft */
                { 4, 1,  9, 1 },   /* row 9: shaft */
                { 4, 6, 99, 0 }    /* row10: base */
            };
            /* Down-arrow is the vertical mirror: read up_glyph in reverse. */
            if (sb_top + sb_w <= sb_bot) {
                int il = sb_left + 1;
                int gy = sb_top + 1;          /* first interior row of up-arrow box */
                for (int k = 0; k < 10; k++) {
                    int y2 = gy + 1 + k;      /* row+1 .. row+10 */
                    cfill(port, il + up_glyph[k][0], y2, up_glyph[k][1],
                          FLAIR_PART_PIN_DARK);
                    if (up_glyph[k][2] < 14) {
                        cfill(port, il + up_glyph[k][2], y2, up_glyph[k][3],
                              FLAIR_PART_PIN_DARK);
                    }
                }
            }
            if (sb_bot - sb_w >= sb_top) {
                int il = sb_left + 1;
                int dy = sb_bot - sb_w + 1;   /* first interior row of down-arrow box */
                for (int k = 0; k < 10; k++) {
                    /* mirror: row k of down = row (9-k) of up */
                    int mk = 9 - k;
                    int y2 = dy + k;
                    cfill(port, il + up_glyph[mk][0], y2, up_glyph[mk][1],
                          FLAIR_PART_PIN_DARK);
                    if (up_glyph[mk][2] < 14) {
                        cfill(port, il + up_glyph[mk][2], y2, up_glyph[mk][3],
                              FLAIR_PART_PIN_DARK);
                    }
                }
            }
        }
#endif /* CHROME_FID_MUT_SCROLL_FLAT */
    }

    /* 5. The window frame: 1 px black outer frame around the whole window.
     * Drawn LAST so it is never painted over by the body/scrollbar.
     *
     * FIDELITY NOTE (beads initech-54nw; Law 4): per window-frame.md Sec 2a
     * (../system7-decomp/specs/chrome/window-frame.md) the body edge is a
     * SINGLE 1px _FrameRect -- horizontal scan y=300: x=352 = black, x=353 =
     * white content, NO second groove line.  The lavender bevel _Lines
     * (wLTinge0/wLTinge4) are TITLE-BAR-ONLY (StandardWDEF_a.txt L709-744).
     * The inner-groove loop that previously ran down the body left/right/bottom
     * was a fidelity bug and has been removed.  The title-bar bevel is a
     * separate element (beads initech-92li; out of scope here). */
#if defined(CHROME_MUTATE_NO_FRAME)
    /* MUTANT: skip the window frame entirely (FO-2/AM-3). test-chrome must catch
     * that the outer 1 px frame line is missing. */
    (void)fr;
#else
    cframe(port, left, top, right, bottom, FLAIR_PART_FRAME);     /* outer 1 px */
    /* Body groove INTENTIONALLY ABSENT (fidelity fix, initech-54nw):
     * the inner groove is title-bar-only (window-frame.md Sec 2a / Sec 1;
     * StandardWDEF_a.txt L567-570 body = one _FrameRect, L709-744 bevel
     * _Lines = title-bar interior only).  No body inner-left/inner-right
     * columns or inner-bottom groove line.  Title-bar bevel is beads initech-92li. */
#endif

    /* 6. Drop shadow: documentProc varCode 0 shadow factor = (1,1) px
     * (StandardWDEF_a.txt L515: `move.l OneOne,D4`).  The WDEF paints an L:
     *   MoveTo(right, top+shadow); LineTo(right, bottom);   -- down the right
     *   LineTo(left+shadow, bottom);                        -- across the bottom
     * (StandardWDEF_a.txt L578-594), in wFrameColor = black = FLAIR_PART_FRAME.
     * In our half-open [left,right) x [top,bottom) coordinate space:
     *   shadow column: x=right,   y in [top+1, bottom+1)   (one px right of frame)
     *   shadow row:    y=bottom,   x in [left+1, right+1)   (one px below frame)
     * The top-right corner (x=right, y=top) and bottom-left corner (x=left,
     * y=bottom) are NOT part of the L (offset +1 misses them -- WDEF geometry).
     *
     * Drawn via C-8 seam (FLAIR_PART_FRAME -- wFrameColor = black).  NEVER a
     * raw color literal (constraint C-8; ADR-0004-AMENDMENT-DEC-09 Sec 3.1).
     * Ref: window-frame.md Sec 1 / Sec 4; StandardWDEF_a.txt L515/L578-594.
     * Golden-resolves: s7_get_info.png on-screen right edge (Sec 4).
     * Mutation probe: CHROME_FID_MUT_NO_SHADOW (beads initech-54nw, Rule 6). */
#if defined(CHROME_FID_MUT_NO_SHADOW)
    /* MUTANT (Rule 6; beads initech-54nw): skip the drop-shadow draw entirely.
     * test-chrome-fidelity leg (7) MUST go RED. */
    (void)h;
#else
    /* Shadow column: x=right, y in [top+1, bottom+1). */
    crect(port, right, top + 1, right + 1, bottom + 1, FLAIR_PART_FRAME);
    /* Shadow row: y=bottom, x in [left+1, right+1).
     * The column above already writes (right, bottom), so the row [left+1, right+1)
     * includes (right, bottom) -- written twice is fine (same pixel, same color). */
    crect(port, left + 1, bottom, right + 1, bottom + 1, FLAIR_PART_FRAME);
#endif
}
