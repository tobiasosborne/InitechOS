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
 * flair_draw_document_window -- the chrome composition (top to bottom).
 * ------------------------------------------------------------------------- */
void flair_draw_document_window(GrafPort *port, rgn_rect_t frame)
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

    /* The title-bar band sits just inside the top frame line. */
#if defined(CHROME_MUTATE_TITLEBAR_H)
    /* MUTANT: title bar 1 px too tall (FO-2/AM-3). test-chrome must catch this:
     * the pinstripe band would run one scanline past the locked 19 px. */
    const int title_h = FLAIR_CHROME_TITLEBAR_H + 1;
#else
    const int title_h = FLAIR_CHROME_TITLEBAR_H; /* 19 px (WDEF minTitleH)      */
#endif

    int title_top = top + fr;
    int title_bot = title_top + title_h;         /* half-open                   */

    /* 1. Pinstripe title bar: alternating light/dark horizontal scanlines at
     * period FLAIR_CHROME_PINSTRIPE_PERIOD (2). The shade indices are the WDEF
     * wTitleBarLight (7) / wTitleBarDark (8). Spans the inner width (inside the
     * left/right frame lines). */
    for (int y = title_top; y < title_bot; y++) {
        int phase = (y - title_top) % FLAIR_CHROME_PINSTRIPE_PERIOD;
        int shade = (phase == 0) ? FLAIR_PART_PIN_LIGHT : FLAIR_PART_PIN_DARK;
        cfill(port, left + fr, y, w - 2 * fr, shade);
    }

    /* 2. Close box (top-left) and zoom box (top-right): hollow 1 px squares,
     * FLAIR_CHROME_WBOX_DELTA (13) px tall, vertically centered in the title
     * bar, inset a few px from the corners (WDEF goAway near the left edge, zoom
     * near the right edge). Interior reduced to the pinstripe so the box reads
     * hollow over the band. */
    {
        int box = FLAIR_CHROME_WBOX_DELTA;       /* 13 px square                */
        int vpad = (FLAIR_CHROME_TITLEBAR_H - box) / 2; /* WBoxDelta centering   */
        if (vpad < 0) {
            vpad = 0;
        }
        int by0 = title_top + vpad;
        int by1 = by0 + box;
        int margin = fr + 3;                     /* small inset from the corner */

        /* close box, top-left */
        int cx0 = left + margin;
        int cx1 = cx0 + box;
        cframe(port, cx0, by0, cx1, by1, FLAIR_PART_FRAME);

        /* zoom box, top-right */
        int zx1 = right - margin;
        int zx0 = zx1 - box;
        cframe(port, zx0, by0, zx1, by1, FLAIR_PART_FRAME);
    }

    /* 3. The content area: white body below the title bar, inside the frame and
     * to the left of the scrollbar. Drawn before the scrollbar so the scrollbar
     * sits on top of (overlaps) the right content edge by 1 px frame (Toolbox
     * 15-vs-16; gui-ground-truth.md Sec 3.4). */
    int content_top = title_bot;                 /* below the title bar         */
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
     * running the height of the content area. Up/down arrow buttons (square,
     * scrollbar-wide) at top and bottom, a light-gray track between. The right
     * edge overlaps the 1 px window frame (presents 15 px of added width). */
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
        /* track fill (light control gray) */
        crect(port, sb_left, sb_top, sb_right, sb_bot, FLAIR_PART_BTNFACE);
        /* left edge of the scrollbar gutter (1 px black divider) */
        cfill(port, sb_left, sb_top, 1, FLAIR_PART_FRAME);
        for (int y = sb_top; y < sb_bot; y++) {
            cfill(port, sb_left, y, 1, FLAIR_PART_FRAME);
        }
        /* up-arrow button (top square) + down-arrow button (bottom square):
         * framed boxes the width of the scrollbar with a 1 px black border. */
        int btn = sb_w;
        if (sb_top + btn <= sb_bot) {
            cframe(port, sb_left, sb_top, sb_right, sb_top + btn, FLAIR_PART_FRAME);
        }
        if (sb_bot - btn >= sb_top) {
            cframe(port, sb_left, sb_bot - btn, sb_right, sb_bot, FLAIR_PART_FRAME);
        }
    }

    /* 5. The window frame: classic Mac double line -- a 1 px black outer frame
     * around the whole window, plus a 1 px groove line separating the title bar
     * from the content (and the content from the bottom frame). Drawn LAST so it
     * is never painted over by the body/scrollbar. The groove does NOT cross the
     * title bar (that would eat a pinstripe scanline and shorten the band below
     * the locked TITLEBAR_H). */
#if defined(CHROME_MUTATE_NO_FRAME)
    /* MUTANT: skip the window frame entirely (FO-2/AM-3). test-chrome must catch
     * that the outer 1 px frame line is missing. */
    (void)fr;
#else
    cframe(port, left, top, right, bottom, FLAIR_PART_FRAME);     /* outer 1 px */
    /* The inner groove (the second line of the classic Mac double-line frame):
     * a 1 px line just inside the outer frame on the LEFT, RIGHT and BOTTOM
     * edges only. It deliberately does NOT cross the TOP / title bar -- the
     * title bar's top edge is the outer frame line, and adding a groove there
     * would consume a pinstripe scanline and shorten the band below the locked
     * TITLEBAR_H. The groove runs from the title bar down to the bottom frame. */
    for (int y = top + fr; y < bottom - fr; y++) {
        cfill(port, left + fr,      y, 1, FLAIR_PART_TEXT);   /* inner left  */
        cfill(port, right - fr - 1, y, 1, FLAIR_PART_TEXT);   /* inner right */
    }
    cfill(port, left + fr, bottom - fr - 1, w - 2 * fr, FLAIR_PART_TEXT); /* bottom */
#endif

    /* Title text (Chicago strike, centered) is DEFERRED this pass (chrome.h):
     * the pinstripe band is drawn without text. The chicago8x16.h strike exists
     * and surface_blit can render it, but proportional Chicago centering + the
     * close/zoom boxes overlapping the text run is a Font-Manager concern
     * (ADR-0004 D-7) -- adding it here would couple chrome geometry to font
     * metrics before the Font Manager lands. Documented deferral, not a silent
     * omission (Law 1 honesty). The title-bar GEOMETRY (the load-bearing
     * test-chrome datum) is complete. */
}
