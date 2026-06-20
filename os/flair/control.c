/*
 * os/flair/control.c -- FLAIR Control Manager (THE ARTIFACT).
 *
 * beads: initech-8h9 ("FLAIR Control Manager: buttons, scrollbars, progress
 *        bar"). See control.h for the full contract, Law-3 separation, and
 *        verbatim Inside Macintosh source citations.
 *
 * Freestanding artifact code: draws push buttons, check boxes, radio buttons,
 * vertical scrollbars, and the FILE COPY progress bar into a GrafPort, writing
 * ONLY through the surface module + blitter (ADR-0004 D-2/C-2: no second pixel
 * path). Dimensions derive from the LOCKED constants in spec/chrome_metrics.h.
 * Hit-testing and tracking are pure geometry + value math (deterministic;
 * Rule 11). No libc. No malloc. No 2026-isms.
 *
 * COLOR MODEL (OD-2): indexed-8 palette indices authored here; the surface
 * module + chrome.c's chrome_px pattern handles the 8bpp/32bpp dispatch. We
 * replicate the CTRL_PAL inline (mirroring chrome.c CHROME_PAL / CIDX_*) so
 * this TU compiles standalone for freestanding and hosted oracle builds.
 *
 * MUTATION HOOKS (Rule 6):
 *   CONTROL_MUTATE_THUMB_OFF  -- thumb position math uses wrong scale factor
 *                                (multiply by 2 instead of divide by range);
 *                                => scrollbar thumb y and value<->thumb tests RED.
 *   CONTROL_MUTATE_NO_CLAMP   -- SetControlValue does NOT clamp to [min,max];
 *                                => clamping tests RED.
 *
 * Ref: control.h (the full API contract + Law 1 citations);
 *      spec/chrome_metrics.h (LOCKED native metrics);
 *      os/flair/blitter.h (region-clipped fill);
 *      os/flair/text.h (Chicago button label + centering);
 *      os/flair/surface.h (the ONE pixel writer);
 *      spec/grafport.h (GrafPort, flair_point_t);
 *      CLAUDE.md Law 1/2/3, Rule 2/6/11/12.
 */

#include <stdint.h>
#include <stddef.h>

#include "control.h"
#include "surface.h"            /* surface_fill_span (-Ios/flair)               */
#include "blitter.h"            /* blitter_fill_rect_clipped (-Ios/flair)       */
#include "text.h"               /* text_measure / text_draw / text_center_in    */
#include "chrome_metrics.h"     /* FLAIR_CHROME_SCROLLBAR_W (-Ispec)            */
#include "region_algebra.h"     /* region_contains_point (-Ispec)              */

/* ===========================================================================
 * PALETTE INDICES  (mirror chrome.c CIDX_* / render_palette_rgb; byte-stable)
 * ===========================================================================*/
enum {
    CTRL_BLACK      = 0,  /* frame lines / borders                              */
    CTRL_WHITE      = 1,  /* content body / button face                         */
    CTRL_DESKTOP    = 2,  /* desktop gray (unused by controls directly)         */
    CTRL_MENUBAR    = 3,  /* menubar gray (unused)                              */
    CTRL_TITLE_INK  = 4,  /* title ink / dark frame                             */
    CTRL_ACCENT     = 5,  /* accent blue (hilite fill for pressed button)       */
    CTRL_CONTROL    = 6,  /* scrollbar track / control face (light gray)        */
    CTRL_PIN_LIGHT  = 7,  /* pinstripe light (reused for scrollbar thumb light) */
    CTRL_PIN_DARK   = 8   /* pinstripe dark  (reused for scrollbar thumb dark)  */
};

/* ===========================================================================
 * PIXEL VALUE HELPER  (same pattern as chrome.c chrome_px; 8bpp vs 32bpp)
 * ===========================================================================*/
static uint32_t ctrl_pal_rgb(uint8_t index)
{
    switch (index) {
    case CTRL_BLACK:     return 0x000000u;
    case CTRL_WHITE:     return 0x7F7F86u;
    case CTRL_DESKTOP:   return 0x73696Cu;
    case CTRL_MENUBAR:   return 0x67696Cu;
    case CTRL_TITLE_INK: return 0x525A63u;
    case CTRL_ACCENT:    return 0x1E2F87u;
    case CTRL_CONTROL:   return 0xBFBFBFu;
    case CTRL_PIN_LIGHT: return 0x6B6B74u;
    case CTRL_PIN_DARK:  return 0x8A8A93u;
    default: {
        uint32_t v = (uint32_t)index;
        return (v << 16) | (v << 8) | v;
    }
    }
}

static uint32_t ctrl_px(const GrafPort *port, uint8_t index)
{
    if (port->portBits.bm.bpp == 8u) {
        return (uint32_t)index;
    }
    return ctrl_pal_rgb(index);
}

/* ===========================================================================
 * CLIPPING HELPER  (same contract as chrome.c clip_in; D-1/D-2)
 * ===========================================================================*/
static int ctrl_in_clip(const GrafPort *port, int x, int y)
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

/* cfill_ctrl -- fill [x, x+w) on row y with index `idx`, clipped.
 * Mirrors chrome.c cfill; batches maximal in-clip runs into surface_fill_span. */
static void cfill_ctrl(GrafPort *port, int x, int y, int w, uint8_t idx)
{
    if (w <= 0) {
        return;
    }
    uint32_t px = ctrl_px(port, idx);
    int run_start = -1;
    for (int i = 0; i <= w; i++) {
        int cx = x + i;
        int in = (i < w) ? ctrl_in_clip(port, cx, y) : 0;
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

/* crect_ctrl -- fill solid rectangle [x0,x1) x [y0,y1) with idx. */
static void crect_ctrl(GrafPort *port, int x0, int y0, int x1, int y1, uint8_t idx)
{
    for (int y = y0; y < y1; y++) {
        cfill_ctrl(port, x0, y, x1 - x0, idx);
    }
}

/* cframe_ctrl -- 1 px hollow outline [x0,x1) x [y0,y1) with idx. */
static void cframe_ctrl(GrafPort *port, int x0, int y0, int x1, int y1, uint8_t idx)
{
    if (x1 <= x0 || y1 <= y0) {
        return;
    }
    cfill_ctrl(port, x0, y0,     x1 - x0, idx);       /* top           */
    cfill_ctrl(port, x0, y1 - 1, x1 - x0, idx);       /* bottom        */
    for (int y = y0; y < y1; y++) {
        cfill_ctrl(port, x0,     y, 1, idx);           /* left          */
        cfill_ctrl(port, x1 - 1, y, 1, idx);           /* right         */
    }
}

/* ===========================================================================
 * STRING HELPERS  (freestanding; no libc strlen / strncpy)
 * ===========================================================================*/
static void flair_strlcpy(char *dst, const char *src, uint32_t dsz)
{
    if (dsz == 0 || dst == 0) {
        return;
    }
    uint32_t i = 0;
    if (src) {
        while (src[i] && i + 1 < dsz) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

/* ===========================================================================
 * control_init
 * ===========================================================================*/
void control_init(ControlRecord *ctrl,
                  flair_ctrl_type_t type,
                  rgn_rect_t rect,
                  int16_t value, int16_t min_val, int16_t max_val,
                  int16_t vis,
                  const char *title)
{
    if (ctrl == 0) {
        return;                                 /* fail-loud guard (Rule 2)     */
    }
    ctrl->contrlType    = type;
    ctrl->contrlRect    = rect;
    ctrl->contrlMin     = min_val;
    ctrl->contrlMax     = max_val;
    /* Clamp initial value to [min,max] */
    if (value < min_val) {
        value = min_val;
    }
    if (value > max_val) {
        value = max_val;
    }
    ctrl->contrlValue   = value;
    ctrl->contrlHilite  = 0;
    ctrl->contrlVis     = vis;
    ctrl->contrlRfCon   = 0;
    flair_strlcpy(ctrl->contrlTitle, title ? title : "",
                  (uint32_t)CTRL_TITLE_MAX);
}

/* ===========================================================================
 * SetControlValue  (clamped to [min,max]; Rule 2)
 * ===========================================================================*/
void SetControlValue(ControlRecord *ctrl, int16_t value)
{
    if (ctrl == 0) {
        return;                                 /* fail-loud guard (Rule 2)     */
    }
#if defined(CONTROL_MUTATE_NO_CLAMP) && CONTROL_MUTATE_NO_CLAMP
    /* NAMED MUTANT: no clamping. SetControlValue stores the value verbatim
     * even if out of [min,max]. test_control MUST catch this (Rule 6). */
    ctrl->contrlValue = value;
#else
    if (value < ctrl->contrlMin) {
        value = ctrl->contrlMin;
    }
    if (value > ctrl->contrlMax) {
        value = ctrl->contrlMax;
    }
    ctrl->contrlValue = value;
#endif
}

/* ===========================================================================
 * GetControlValue
 * ===========================================================================*/
int16_t GetControlValue(const ControlRecord *ctrl)
{
    if (ctrl == 0) {
        return 0;
    }
    return ctrl->contrlValue;
}

/* ===========================================================================
 * THUMB MATH  (value <-> pixel; Law 1: Inside Macintosh IV thumb convention)
 *
 * Track region (port-local):
 *   track_top = contrlRect.top + SB_ARROW
 *   track_bot = contrlRect.bottom - SB_ARROW
 *   track_h   = track_bot - track_top
 *
 * Thumb height: SB_THUMB_MIN (16 px -- non-proportional for scrollBar type).
 *
 * Thumb top y (forward, value -> pixel):
 *   range = contrlMax - contrlMin
 *   if (range == 0 || track_h <= thumb_h) -> thumb_y = track_top
 *   else -> thumb_y = track_top +
 *             (contrlValue - contrlMin) * (track_h - thumb_h) / range
 *
 * Inverse (pixel -> value):
 *   if (track_h <= thumb_h) -> value = contrlMin
 *   else -> value = contrlMin +
 *             (thumb_y - track_top) * range / (track_h - thumb_h)
 *   clamped to [contrlMin, contrlMax].
 * ===========================================================================*/
int16_t ctrl_thumb_y(const ControlRecord *ctrl)
{
    int top      = (int)ctrl->contrlRect.top;
    int bot      = (int)ctrl->contrlRect.bottom;
    int track_top = top + SB_ARROW;
    int track_bot = bot - SB_ARROW;
    int track_h   = track_bot - track_top;
    int thumb_h   = SB_THUMB_MIN;
    int range     = (int)ctrl->contrlMax - (int)ctrl->contrlMin;

    if (track_h <= thumb_h || range <= 0) {
        return (int16_t)track_top;
    }

#if defined(CONTROL_MUTATE_THUMB_OFF) && CONTROL_MUTATE_THUMB_OFF
    /* NAMED MUTANT: wrong scale factor -- multiply by 2 instead of dividing
     * correctly. test_control MUST catch this divergence (Rule 6). */
    int offset = ((int)ctrl->contrlValue - (int)ctrl->contrlMin) * 2;
#else
    int offset = ((int)ctrl->contrlValue - (int)ctrl->contrlMin)
                 * (track_h - thumb_h) / range;
#endif

    int ty = track_top + offset;
    if (ty < track_top) {
        ty = track_top;
    }
    if (ty > track_bot - thumb_h) {
        ty = track_bot - thumb_h;
    }
    return (int16_t)ty;
}

int16_t ctrl_value_from_thumb_y(const ControlRecord *ctrl, int16_t thumb_y)
{
    int top      = (int)ctrl->contrlRect.top;
    int bot      = (int)ctrl->contrlRect.bottom;
    int track_top = top + SB_ARROW;
    int track_bot = bot - SB_ARROW;
    int track_h   = track_bot - track_top;
    int thumb_h   = SB_THUMB_MIN;
    int range     = (int)ctrl->contrlMax - (int)ctrl->contrlMin;

    if (track_h <= thumb_h || range <= 0) {
        return ctrl->contrlMin;
    }

    int moveable = track_h - thumb_h;
    int offset   = (int)thumb_y - track_top;
    if (offset < 0) {
        offset = 0;
    }
    if (offset > moveable) {
        offset = moveable;
    }

#if defined(CONTROL_MUTATE_THUMB_OFF) && CONTROL_MUTATE_THUMB_OFF
    /* NAMED MUTANT: inverse uses same wrong formula to stay "consistent" with
     * the mutated forward pass. The forward+inverse pair is tested together so
     * the mutant ensures the ratio is wrong in both directions (Rule 6). */
    int v = (int)ctrl->contrlMin + offset / 2;
#else
    int v = (int)ctrl->contrlMin + offset * range / moveable;
#endif

    if (v < (int)ctrl->contrlMin) {
        v = (int)ctrl->contrlMin;
    }
    if (v > (int)ctrl->contrlMax) {
        v = (int)ctrl->contrlMax;
    }
    return (int16_t)v;
}

/* ===========================================================================
 * PROGRESS BAR FILL MATH
 * filled_px = inner_w * contrlValue / contrlMax   (integer division)
 * inner_w = contrlRect.right - contrlRect.left - 2  (1 px border each side)
 * ===========================================================================*/
int16_t ctrl_progress_fill_px(const ControlRecord *ctrl)
{
    int inner_w = (int)ctrl->contrlRect.right - (int)ctrl->contrlRect.left - 2;
    if (inner_w <= 0 || ctrl->contrlMax <= 0) {
        return 0;
    }
    int v = (int)ctrl->contrlValue;
    if (v <= 0) {
        return 0;
    }
    if (v >= (int)ctrl->contrlMax) {
        return (int16_t)inner_w;
    }
    return (int16_t)(inner_w * v / (int)ctrl->contrlMax);
}

/* ===========================================================================
 * DRAW DISPATCH
 * ===========================================================================*/

/* draw_push_button -- System-7-style push button.
 *
 * A rounded-ish rect (approximated as a 1 px framed rounded rect via a pair
 * of small corner fillets; period-authentic); Chicago label centered.
 * Hilite: when contrlHilite == inButton, invert the button interior (fill
 * with CTRL_ACCENT / dark; the label reads on the accent fill).
 *
 * Ref: Inside Macintosh Vol I Ch 5; System 7 push-button appearance. */
static void draw_push_button(GrafPort *port, const ControlRecord *ctrl)
{
    int x0 = (int)ctrl->contrlRect.left;
    int y0 = (int)ctrl->contrlRect.top;
    int x1 = (int)ctrl->contrlRect.right;
    int y1 = (int)ctrl->contrlRect.bottom;
    int w  = x1 - x0;
    int h  = y1 - y0;
    if (w < 4 || h < 4) {
        return;
    }

    int hilited = (ctrl->contrlHilite == inButton);

    /* Button face fill. */
    uint8_t face_idx = hilited ? CTRL_ACCENT : CTRL_WHITE;
    crect_ctrl(port, x0, y0, x1, y1, face_idx);

    /* 1 px black outer frame. */
    cframe_ctrl(port, x0, y0, x1, y1, CTRL_BLACK);

    /* Simple rounded corners: clear the 4 corner pixels of the frame to give
     * a slight round-ish appearance (System-7 push button style). The
     * surrounding desktop color shows through the cleared corners. */
    cfill_ctrl(port, x0,     y0,     1, CTRL_DESKTOP);  /* TL */
    cfill_ctrl(port, x1 - 1, y0,     1, CTRL_DESKTOP);  /* TR */
    cfill_ctrl(port, x0,     y1 - 1, 1, CTRL_DESKTOP);  /* BL */
    cfill_ctrl(port, x1 - 1, y1 - 1, 1, CTRL_DESKTOP);  /* BR */

    /* Chicago label, centered. */
    uint32_t fg = ctrl_px(port, hilited ? CTRL_WHITE : CTRL_BLACK);
    uint32_t bg = ctrl_px(port, face_idx);
    int label_w = text_measure(FONT_CHICAGO, ctrl->contrlTitle);
    int label_h = text_cell_height(FONT_CHICAGO);
    int lx = x0 + text_center_in(w, ctrl->contrlTitle, FONT_CHICAGO);
    int ly = y0 + (h - label_h) / 2;
    if (ly < y0 + 1) {
        ly = y0 + 1;
    }
    if (label_w > 0) {
        text_draw(&port->portBits.bm, lx, ly,
                  ctrl->contrlTitle, FONT_CHICAGO, fg, bg);
    }
}

/* draw_check_box -- System-7 check box.
 *
 * 12 px square box at left, centered vertically in the rect. A black "X" or
 * solid fill indicates checked (contrlValue != 0). Title to the right of box.
 * Ref: IM-I Ch 5. */
static void draw_check_box(GrafPort *port, const ControlRecord *ctrl)
{
    int x0 = (int)ctrl->contrlRect.left;
    int y0 = (int)ctrl->contrlRect.top;
    int x1 = (int)ctrl->contrlRect.right;
    int y1 = (int)ctrl->contrlRect.bottom;
    int h  = y1 - y0;

    enum { BOX_SZ = 12 };   /* System-7 check box: 12 px square               */
    int box_y = y0 + (h - BOX_SZ) / 2;
    if (box_y < y0) {
        box_y = y0;
    }

    /* White fill + black frame. */
    crect_ctrl(port,  x0, box_y, x0 + BOX_SZ, box_y + BOX_SZ, CTRL_WHITE);
    cframe_ctrl(port, x0, box_y, x0 + BOX_SZ, box_y + BOX_SZ, CTRL_BLACK);

    /* Check mark: fill interior with accent if checked. */
    if (ctrl->contrlValue != 0) {
        /* A solid interior fill (simplified checkmark; period-authentic for
         * 8-bpp with 12 px box). */
        crect_ctrl(port, x0 + 2, box_y + 2, x0 + BOX_SZ - 2,
                   box_y + BOX_SZ - 2, CTRL_ACCENT);
    }

    /* Title to the right of the box. */
    if (ctrl->contrlTitle[0]) {
        int label_h = text_cell_height(FONT_CHICAGO);
        int ly = y0 + (h - label_h) / 2;
        if (ly < y0) {
            ly = y0;
        }
        uint32_t fg = ctrl_px(port, CTRL_BLACK);
        /* bg: the caller's background -- we use CTRL_DESKTOP as a pass-through;
         * the actual background is whatever was drawn behind the control. For
         * the oracle's host-bitmap, CTRL_DESKTOP renders the background fill
         * color, which is appropriate for a dialog background. */
        uint32_t bg = ctrl_px(port, CTRL_DESKTOP);
        text_draw(&port->portBits.bm, x0 + BOX_SZ + 4, ly,
                  ctrl->contrlTitle, FONT_CHICAGO, fg, bg);
    }
    (void)x1;  /* suppress unused-variable warning (x1 not needed past box) */
}

/* draw_radio_button -- System-7 radio button.
 *
 * 12 px circle (approximated as a framed square + corner pixels cleared) at
 * left; filled dot if selected. Title to the right.
 * Ref: IM-I Ch 5. */
static void draw_radio_button(GrafPort *port, const ControlRecord *ctrl)
{
    int x0 = (int)ctrl->contrlRect.left;
    int y0 = (int)ctrl->contrlRect.top;
    int y1 = (int)ctrl->contrlRect.bottom;
    int h  = y1 - y0;

    enum { RADIO_SZ = 12 };
    int box_y = y0 + (h - RADIO_SZ) / 2;
    if (box_y < y0) {
        box_y = y0;
    }

    /* White fill + black frame; clear corners for "circle" effect. */
    crect_ctrl(port,  x0, box_y, x0 + RADIO_SZ, box_y + RADIO_SZ, CTRL_WHITE);
    cframe_ctrl(port, x0, box_y, x0 + RADIO_SZ, box_y + RADIO_SZ, CTRL_BLACK);
    /* clear 4 corners */
    cfill_ctrl(port, x0,              box_y,              1, CTRL_DESKTOP);
    cfill_ctrl(port, x0 + RADIO_SZ-1, box_y,              1, CTRL_DESKTOP);
    cfill_ctrl(port, x0,              box_y + RADIO_SZ-1, 1, CTRL_DESKTOP);
    cfill_ctrl(port, x0 + RADIO_SZ-1, box_y + RADIO_SZ-1, 1, CTRL_DESKTOP);

    /* Dot if selected. */
    if (ctrl->contrlValue != 0) {
        crect_ctrl(port, x0 + 3, box_y + 3,
                   x0 + RADIO_SZ - 3, box_y + RADIO_SZ - 3, CTRL_BLACK);
    }

    /* Title to the right. */
    if (ctrl->contrlTitle[0]) {
        int label_h = text_cell_height(FONT_CHICAGO);
        int ly = y0 + (h - label_h) / 2;
        if (ly < y0) {
            ly = y0;
        }
        uint32_t fg = ctrl_px(port, CTRL_BLACK);
        uint32_t bg = ctrl_px(port, CTRL_DESKTOP);
        text_draw(&port->portBits.bm, x0 + RADIO_SZ + 4, ly,
                  ctrl->contrlTitle, FONT_CHICAGO, fg, bg);
    }
}

/* draw_scrollbar -- vertical scrollbar.
 *
 * Layout (port-local, from contrlRect):
 *   left   = contrlRect.left
 *   right  = contrlRect.right    (width = FLAIR_CHROME_SCROLLBAR_W = 16)
 *   top    = contrlRect.top
 *   bottom = contrlRect.bottom
 *
 * Elements (top to bottom):
 *   [top,     top+SB_ARROW) -- up-arrow button (framed square, SB_ARROW wide)
 *   [top+SB_ARROW, bottom-SB_ARROW) -- track region (light gray)
 *     within track: proportional thumb (1 px framed, SB_THUMB_MIN min height)
 *   [bottom-SB_ARROW, bottom) -- down-arrow button
 *
 * Left edge: 1 px black divider (the left gutter line, matching chrome.c).
 *
 * Ref: FLAIR_CHROME_SCROLLBAR_W (spec/chrome_metrics.h); control.h math. */
static void draw_scrollbar(GrafPort *port, const ControlRecord *ctrl)
{
    int x0  = (int)ctrl->contrlRect.left;
    int y0  = (int)ctrl->contrlRect.top;
    int x1  = (int)ctrl->contrlRect.right;
    int y1  = (int)ctrl->contrlRect.bottom;
    int h   = y1 - y0;
    int btn = SB_ARROW;

    if (h < 2 * btn + SB_THUMB_MIN + 2) {
        return;   /* too small to draw                                           */
    }

    /* Left edge gutter divider (1 px black, matching chrome.c's sb divider). */
    for (int y = y0; y < y1; y++) {
        cfill_ctrl(port, x0, y, 1, CTRL_BLACK);
    }

    /* Track fill (light gray). */
    int track_top = y0 + btn;
    int track_bot = y1 - btn;
    crect_ctrl(port, x0 + 1, track_top, x1, track_bot, CTRL_CONTROL);

    /* Thumb (proportional; centered in track). */
    int thumb_h  = SB_THUMB_MIN;
    int16_t ty   = ctrl_thumb_y(ctrl);   /* port-local absolute y               */

    /* Thumb is SB_THUMB_MIN px tall, full scrollbar width (minus left divider).
     * Frame it black; interior light gray (or accent if tracking). */
    int thumb_hilite = (ctrl->contrlHilite == inThumb);
    uint8_t thumb_face = thumb_hilite ? CTRL_ACCENT : CTRL_CONTROL;
    crect_ctrl(port,  x0 + 1, (int)ty, x1, (int)ty + thumb_h, thumb_face);
    cframe_ctrl(port, x0 + 1, (int)ty, x1, (int)ty + thumb_h, CTRL_BLACK);

    /* Up-arrow button (top): framed box with a small up-triangle indicator.
     * Hilite: fill face with accent when contrlHilite == inUpButton. */
    int up_hilite = (ctrl->contrlHilite == inUpButton);
    uint8_t up_face = up_hilite ? CTRL_ACCENT : CTRL_CONTROL;
    crect_ctrl(port,  x0 + 1, y0, x1, y0 + btn, up_face);
    cframe_ctrl(port, x0 + 1, y0, x1, y0 + btn, CTRL_BLACK);
    /* Arrow indicator: a small upward-pointing triangle drawn in the center.
     * Approximated by 3 rows decreasing in width from bottom to top. */
    {
        int cx = (x0 + 1 + x1) / 2;
        int ay = y0 + 4;
        cfill_ctrl(port, cx,     ay,     1, CTRL_BLACK); /* tip */
        cfill_ctrl(port, cx - 1, ay + 1, 3, CTRL_BLACK);
        cfill_ctrl(port, cx - 2, ay + 2, 5, CTRL_BLACK);
    }

    /* Down-arrow button (bottom): framed box with down-triangle indicator. */
    int dn_hilite = (ctrl->contrlHilite == inDownButton);
    uint8_t dn_face = dn_hilite ? CTRL_ACCENT : CTRL_CONTROL;
    crect_ctrl(port,  x0 + 1, y1 - btn, x1, y1, dn_face);
    cframe_ctrl(port, x0 + 1, y1 - btn, x1, y1, CTRL_BLACK);
    {
        int cx = (x0 + 1 + x1) / 2;
        int ay = y1 - btn + 4;
        cfill_ctrl(port, cx - 2, ay,     5, CTRL_BLACK); /* top of down arrow  */
        cfill_ctrl(port, cx - 1, ay + 1, 3, CTRL_BLACK);
        cfill_ctrl(port, cx,     ay + 2, 1, CTRL_BLACK); /* tip                */
    }
}

/* draw_progress_bar -- FILE COPY determinate progress bar.
 *
 * Layout:
 *   outer frame (1 px black border) around contrlRect.
 *   inner area: [left+1, right-1) x [top+1, bottom-1).
 *   filled portion: [left+1, left+1+filled_px) in CTRL_ACCENT (blue).
 *   unfilled remainder in CTRL_WHITE.
 *
 * Ref: PRD Sec 6.5 (the comedic FILE COPY progress bar; ADR-0004 D-3). */
static void draw_progress_bar(GrafPort *port, const ControlRecord *ctrl)
{
    int x0 = (int)ctrl->contrlRect.left;
    int y0 = (int)ctrl->contrlRect.top;
    int x1 = (int)ctrl->contrlRect.right;
    int y1 = (int)ctrl->contrlRect.bottom;
    if (x1 - x0 < 4 || y1 - y0 < 3) {
        return;
    }

    /* Outer frame. */
    cframe_ctrl(port, x0, y0, x1, y1, CTRL_BLACK);

    /* Inner area: white background first. */
    crect_ctrl(port, x0 + 1, y0 + 1, x1 - 1, y1 - 1, CTRL_WHITE);

    /* Filled portion. */
    int filled = (int)ctrl_progress_fill_px(ctrl);
    if (filled > 0) {
        crect_ctrl(port, x0 + 1, y0 + 1, x0 + 1 + filled, y1 - 1, CTRL_ACCENT);
    }
}

/* ===========================================================================
 * DrawControl (dispatch by contrlType)
 * ===========================================================================*/
void DrawControl(GrafPort *port, ControlRecord *ctrl)
{
    if (port == 0 || ctrl == 0) {
        return;                                 /* fail-loud guard (Rule 2)     */
    }
    if (!ctrl->contrlVis) {
        return;
    }

    switch (ctrl->contrlType) {
    case pushButton:
        draw_push_button(port, ctrl);
        break;
    case checkBox:
        draw_check_box(port, ctrl);
        break;
    case radioButton:
        draw_radio_button(port, ctrl);
        break;
    case scrollBar:
        draw_scrollbar(port, ctrl);
        break;
    case progressBar:
        draw_progress_bar(port, ctrl);
        break;
    default:
        break;   /* unknown type: no-op (fail-soft for forward compat)         */
    }
}

/* ===========================================================================
 * POINT-IN-RECT HELPER  (pure geometry; no clip check needed for hit-testing)
 * ===========================================================================*/
static int pt_in_rect(flair_point_t pt, rgn_rect_t r)
{
    return (pt.h >= r.left && pt.h < r.right &&
            pt.v >= r.top  && pt.v < r.bottom);
}

/* ===========================================================================
 * TestControl -- hit-test a point; return part code or 0.
 * ===========================================================================*/
int16_t TestControl(const ControlRecord *ctrl, flair_point_t pt)
{
    if (ctrl == 0 || !ctrl->contrlVis) {
        return 0;
    }

    /* Quick outer bounds check. */
    if (!pt_in_rect(pt, ctrl->contrlRect)) {
        return 0;
    }

    switch (ctrl->contrlType) {
    case pushButton:
        return inButton;

    case checkBox:
    case radioButton:
        return inCheckBox;

    case scrollBar: {
        int x0  = (int)ctrl->contrlRect.left;
        int y0  = (int)ctrl->contrlRect.top;
        int x1  = (int)ctrl->contrlRect.right;
        int y1  = (int)ctrl->contrlRect.bottom;
        int btn = SB_ARROW;
        int h   = y1 - y0;

        /* Must be at least wide enough to have a scrollbar. */
        if (h < 2 * btn) {
            return inButton; /* degenerate: treat as generic hit */
        }

        int y = (int)pt.v;
        (void)x0; (void)x1;

        /* Up-arrow button: [y0, y0+btn) */
        if (y < y0 + btn) {
            return inUpButton;
        }
        /* Down-arrow button: [y1-btn, y1) */
        if (y >= y1 - btn) {
            return inDownButton;
        }

        /* Track region: [y0+btn, y1-btn) */
        int16_t ty     = ctrl_thumb_y(ctrl);
        int     thumb_h = SB_THUMB_MIN;
        int     thumb_bot = (int)ty + thumb_h;

        if (y >= (int)ty && y < thumb_bot) {
            return inThumb;
        }
        if (y < (int)ty) {
            return inPageUp;
        }
        return inPageDown;
    }

    case progressBar:
        /* Progress bars are display-only; no sub-parts to hit. */
        return inButton;

    default:
        return inButton;
    }
}

/* ===========================================================================
 * TrackControl -- follow a point sequence; return final part code.
 *
 * Deterministic (Rule 11): pure function of pts[] and ctrl state. No hardware,
 * no ISR, no PIT. For thumb drag, updates contrlValue proportionally.
 * ===========================================================================*/
int16_t TrackControl(ControlRecord *ctrl,
                     const flair_point_t *pts, uint32_t n_pts)
{
    if (ctrl == 0 || pts == 0 || n_pts == 0) {
        return 0;
    }

    /* Determine the initial part from the first point. */
    int16_t initial_part = TestControl(ctrl, pts[0]);
    if (initial_part == 0) {
        ctrl->contrlHilite = 0;
        return 0;
    }

    /* Hilite the pressed part. */
    ctrl->contrlHilite = initial_part;

    /* For thumb drag: remember the initial thumb_y. */
    int16_t drag_start_y  = 0;
    if (initial_part == inThumb) {
        drag_start_y = ctrl_thumb_y(ctrl);
    }

    int16_t last_part = initial_part;
    for (uint32_t i = 1; i < n_pts; i++) {
        flair_point_t cur = pts[i];
        int16_t cur_part  = TestControl(ctrl, cur);

        if (initial_part == inThumb) {
            /* Thumb drag: update value proportionally from vertical delta. */
            /* Compute new thumb_y by offsetting from drag_start_y. */
            int delta   = (int)cur.v - (int)pts[0].v;
            int new_ty  = (int)drag_start_y + delta;
            int16_t new_val = ctrl_value_from_thumb_y(ctrl, (int16_t)new_ty);
            SetControlValue(ctrl, new_val);
            /* Update last_part based on whether still in thumb region. */
            last_part = TestControl(ctrl, cur);
        } else {
            /* Non-thumb parts: hilite only while inside the original part. */
            if (cur_part == initial_part) {
                ctrl->contrlHilite = initial_part;
                last_part = initial_part;
            } else {
                ctrl->contrlHilite = 0;
                last_part = 0;
            }
        }
    }

    /* Restore hilite after tracking. */
    ctrl->contrlHilite = 0;

    /* Return the part code of the final point, or 0 if outside. */
    return last_part;
}
