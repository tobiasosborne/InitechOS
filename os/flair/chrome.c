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
 * COLOR MODEL (OD-2): the chrome is authored in indexed-8 palette indices -- the
 * canonical FLAIR offscreen depth. For an 8bpp destination the surface module
 * writes the index byte directly (surface_put_pixel low byte). For a 32bpp
 * destination it needs a packed 0x00RRGGBB, so this file carries a small fixed
 * index->RGB table (CHROME_PAL) -- artifact chrome data, byte-stable (Rule 11) --
 * and chrome_px() returns the right pixel value for the port's bpp. The host
 * oracle's render_palette_rgb() mirrors CHROME_PAL so 8bpp and 32bpp render
 * identically up to the depth.
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
#include "surface.h"            /* surface_fill_span / surface_pack_rgb (-Ios/flair) */
#include "chrome_metrics.h"     /* FLAIR_CHROME_* (-Ispec)                    */
#include "region_algebra.h"     /* region_contains_point (-Ispec)            */

/* ---------------------------------------------------------------------------
 * The indexed-8 chrome palette. INDEX is the authored color; RGB is for 32bpp.
 * MUST stay in lockstep with harness/render render_palette_rgb (the oracle maps
 * 8bpp indices to RGB through that mirror). Byte-stable (Rule 11).
 * ------------------------------------------------------------------------- */
enum {
    CIDX_BLACK      = 0,  /* frame lines / box borders / ink                  */
    CIDX_WHITE      = 1,  /* content body fill                                 */
    CIDX_DESKTOP    = 2,  /* desktop gray (unused by a single window)          */
    CIDX_MENUBAR    = 3,  /* menubar gray (unused by a single window)          */
    CIDX_TITLE_INK  = 4,  /* title ink / inner groove dark                     */
    CIDX_ACCENT     = 5,  /* accent blue (unused by bare chrome)               */
    CIDX_CONTROL    = 6,  /* scrollbar track / arrow-button face (light gray)  */
    CIDX_PIN_LIGHT  = FLAIR_CHROME_TITLE_SHADE_LIGHT, /* 7 -- pinstripe light  */
    CIDX_PIN_DARK   = FLAIR_CHROME_TITLE_SHADE_DARK   /* 8 -- pinstripe dark   */
};

/* index -> 0x00RRGGBB, for the 32bpp path. Mirrors render_palette_rgb. */
static uint32_t chrome_pal_rgb(uint8_t index)
{
    switch (index) {
    case CIDX_BLACK:     return 0x000000u;
    case CIDX_WHITE:     return 0x7F7F86u; /* INITECH_WINDOW_WHITE_RGB        */
    case CIDX_DESKTOP:   return 0x73696Cu; /* INITECH_DESKTOP_BG_RGB          */
    case CIDX_MENUBAR:   return 0x67696Cu; /* INITECH_MENUBAR_BG_RGB          */
    case CIDX_TITLE_INK: return 0x525A63u; /* INITECH_TITLEBAR_INK_RGB        */
    case CIDX_ACCENT:    return 0x1E2F87u; /* INITECH_ACCENT_BLUE_RGB         */
    case CIDX_CONTROL:   return 0xBFBFBFu;
    case CIDX_PIN_LIGHT: return 0x6B6B74u; /* INITECH_TITLEBAR_PINSTRIPE_RGB  */
    case CIDX_PIN_DARK:  return 0x8A8A93u;
    default: {
        uint32_t v = (uint32_t)index;
        return (v << 16) | (v << 8) | v;
    }
    }
}

/* Convert an authored palette index to the surface pixel value for this port's
 * destination depth. 8bpp: the index byte. 32bpp: the packed RGB. */
static uint32_t chrome_px(const GrafPort *port, uint8_t index)
{
    if (port->portBits.bm.bpp == 8u) {
        return (uint32_t)index;            /* surface writes the low byte      */
    }
    return surface_pack_rgb(port->portBits.bm.bpp, 0, 0, 0) |
           chrome_pal_rgb(index);          /* canonical 0x00RRGGBB             */
}

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
 * cfill -- fill the half-open span [x, x+w) on row y with palette index `idx`,
 * clipped to visRgn INTERSECT clipRgn, writing only through the surface module.
 *
 * Walks the span, batching maximal in-clip runs into single surface_fill_span
 * calls. The surface module additionally clips to the bitmap bounds (Rule 2).
 * ------------------------------------------------------------------------- */
static void cfill(GrafPort *port, int x, int y, int w, uint8_t idx)
{
    if (w <= 0) {
        return;
    }
    uint32_t px = chrome_px(port, idx);
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

/* Fill a solid rectangle [x0,x1) x [y0,y1) with palette index `idx`. */
static void crect(GrafPort *port, int x0, int y0, int x1, int y1, uint8_t idx)
{
    for (int y = y0; y < y1; y++) {
        cfill(port, x0, y, x1 - x0, idx);
    }
}

/* Frame (1 px hollow outline) of [x0,x1) x [y0,y1) with palette index `idx`. */
static void cframe(GrafPort *port, int x0, int y0, int x1, int y1, uint8_t idx)
{
    if (x1 <= x0 || y1 <= y0) {
        return;
    }
    cfill(port, x0, y0,     x1 - x0, idx);          /* top edge    */
    cfill(port, x0, y1 - 1, x1 - x0, idx);          /* bottom edge */
    for (int y = y0; y < y1; y++) {
        cfill(port, x0,     y, 1, idx);             /* left edge   */
        cfill(port, x1 - 1, y, 1, idx);             /* right edge  */
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
        uint8_t shade = (phase == 0) ? CIDX_PIN_LIGHT : CIDX_PIN_DARK;
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
        cframe(port, cx0, by0, cx1, by1, CIDX_BLACK);

        /* zoom box, top-right */
        int zx1 = right - margin;
        int zx0 = zx1 - box;
        cframe(port, zx0, by0, zx1, by1, CIDX_BLACK);
    }

    /* 3. The content area: white body below the title bar, inside the frame and
     * to the left of the scrollbar. Drawn before the scrollbar so the scrollbar
     * sits on top of (overlaps) the right content edge by 1 px frame (Toolbox
     * 15-vs-16; gui-ground-truth.md Sec 3.4). */
    int content_top = title_bot;                 /* below the title bar         */
    int content_bot = bottom - fr;               /* above the bottom frame      */
    int content_left = left + fr;
    int content_right = right - fr;
    crect(port, content_left, content_top, content_right, content_bot, CIDX_WHITE);

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
        crect(port, sb_left, sb_top, sb_right, sb_bot, CIDX_CONTROL);
        /* left edge of the scrollbar gutter (1 px black divider) */
        cfill(port, sb_left, sb_top, 1, CIDX_BLACK);
        for (int y = sb_top; y < sb_bot; y++) {
            cfill(port, sb_left, y, 1, CIDX_BLACK);
        }
        /* up-arrow button (top square) + down-arrow button (bottom square):
         * framed boxes the width of the scrollbar with a 1 px black border. */
        int btn = sb_w;
        if (sb_top + btn <= sb_bot) {
            cframe(port, sb_left, sb_top, sb_right, sb_top + btn, CIDX_BLACK);
        }
        if (sb_bot - btn >= sb_top) {
            cframe(port, sb_left, sb_bot - btn, sb_right, sb_bot, CIDX_BLACK);
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
    cframe(port, left, top, right, bottom, CIDX_BLACK);           /* outer 1 px */
    /* The inner groove (the second line of the classic Mac double-line frame):
     * a 1 px line just inside the outer frame on the LEFT, RIGHT and BOTTOM
     * edges only. It deliberately does NOT cross the TOP / title bar -- the
     * title bar's top edge is the outer frame line, and adding a groove there
     * would consume a pinstripe scanline and shorten the band below the locked
     * TITLEBAR_H. The groove runs from the title bar down to the bottom frame. */
    for (int y = top + fr; y < bottom - fr; y++) {
        cfill(port, left + fr,      y, 1, CIDX_TITLE_INK);   /* inner left  */
        cfill(port, right - fr - 1, y, 1, CIDX_TITLE_INK);   /* inner right */
    }
    cfill(port, left + fr, bottom - fr - 1, w - 2 * fr, CIDX_TITLE_INK); /* bottom */
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
