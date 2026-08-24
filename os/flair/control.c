/*
 * os/flair/control.c -- FLAIR Control Manager (THE ARTIFACT).
 *
 * beads: initech-8h9 ("FLAIR Control Manager: buttons, scrollbars, progress
 *        bar"). See control.h for the full contract, Law-3 separation, and
 *        verbatim Inside Macintosh source citations.
 *
 * ERA AXIS: Mac OS 8 Platinum (DEC-10) is the FLAIR BASE. The scrollbar,
 * unpressed push button, and checked checkbox use sampled Platinum anatomy.
 * The unchecked checkbox, pressed button, and radio remain explicitly tagged
 * golden gaps; their retained heritage paths are not promoted as Platinum.
 *
 * Freestanding artifact code: draws push buttons, check boxes, radio buttons,
 * vertical scrollbars, and the FILE COPY progress bar into a GrafPort, writing
 * ONLY through the surface module + blitter (ADR-0004 D-2/C-2: no second pixel
 * path). Dimensions derive from the LOCKED constants in spec/chrome_metrics.h.
 * Hit-testing and tracking are pure geometry + value math (deterministic;
 * Rule 11). No libc. No malloc. No 2026-isms.
 *
 * COLOR MODEL (C-8; ADR-0004-AMENDMENT-DEC-09 Sec 3.1/3.3). control.c is a
 * DECORATION policy: it keeps control GEOMETRY but names NO color.  It names a
 * wctb-keyed PART (FLAIR_PART_*) and resolves PART -> destination pixel ONLY
 * through the ONE policy seam flair_look_pixel(port, PART)
 * (os/flair/flair_look.h).  This TU ships ZERO 0xRRGGBB literal, ZERO
 * INITECH_*_RGB, ZERO index->RGB switch -- the C-8 cut-line.
 *
 * MUTATION HOOKS (Rule 6):
 *   CONTROL_MUTATE_THUMB_OFF  -- thumb position math uses wrong scale factor
 *                                (multiply by 2 instead of divide by range);
 *                                => scrollbar thumb y and value<->thumb tests RED.
 *   CONTROL_MUTATE_NO_CLAMP   -- SetControlValue does NOT clamp to [min,max];
 *                                => clamping tests RED.
 *   SB_MUT_THUMB_16           -- heritage 16px thumb replaces sampled 15px.
 *   SB_MUT_DISABLED_ENABLED_LOOK -- disabled bar paints the enabled anatomy.
 *   CTRL_MUT_CHECK_FILL -- checked checkbox relapses to the accent fill.
 *   CTRL_MUT_BTN_FLAT -- unpressed push button loses its sampled bevel.
 *   CTRL_MUT_NO_DEFAULT_RING -- Dialog Manager default ring is suppressed.
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
#include "flair_look.h"         /* flair_look_pixel + FLAIR_PART_* (the seam)   */

/* ===========================================================================
 * PART NAMESPACE  (C-8; the wctb-keyed roles control draw names -- NOT colors)
 * The control draw code names these semantic PARTs and resolves PART ->
 * destination pixel through the ONE policy seam flair_look_pixel.  The names
 * below are local aliases onto FLAIR_PART_* so the draw code reads naturally
 * (CTRL_BLACK == frame ink, CTRL_WHITE == content, ...).  No index, no color.
 * ===========================================================================*/
enum {
    CTRL_BLACK      = FLAIR_PART_FRAME,        /* frame lines / borders         */
    CTRL_WHITE      = FLAIR_PART_CONTENT,      /* content body / button face    */
    CTRL_DESKTOP    = FLAIR_PART_DESKTOP,      /* desktop background            */
    CTRL_TITLE_INK  = FLAIR_PART_TEXT,         /* title ink / dark frame        */
    CTRL_ACCENT     = FLAIR_PART_CAPTION_NAVY, /* accent (hilite fill)          */
    CTRL_CONTROL    = FLAIR_PART_BTNFACE,      /* retained control face          */
    CTRL_PLAT_FACE  = FLAIR_PART_PLAT_FACE,    /* sampled E7 control face         */
    CTRL_PLAT_SHADOW = FLAIR_PART_PLAT_WELL,   /* sampled C0 bevel/moat shadow    */
    CTRL_PLAT_DARK_SHADOW = FLAIR_PART_PLAT_STRIPE_DARK, /* sampled 96 shadow */
    CTRL_PLAT_WIDGET_EDGE = FLAIR_PART_PLAT_WIDGET_EDGE, /* sampled A5 edge   */
    CTRL_PLAT_DARK_RING = FLAIR_PART_PLAT_DARK_RING, /* sampled 3F smoothing */
    CTRL_PLAT_CORNER = FLAIR_PART_PLAT_TILE_SHADOW, /* sampled CD smoothing  */
    CTRL_SB_THUMB_HL = FLAIR_PART_SB_THUMB_HL,
    CTRL_SB_THUMB_FACE = FLAIR_PART_SB_THUMB_FACE,
    CTRL_SB_THUMB_SHADOW = FLAIR_PART_SB_THUMB_SHADOW,
    CTRL_SB_THUMB_GRIP = FLAIR_PART_SB_THUMB_GRIP
};

/* ===========================================================================
 * PIXEL VALUE HELPER  (C-8 policy seam; the ONE PART->pixel resolution)
 * ===========================================================================*/
/* ctrl_px resolves a wctb-keyed PART to the destination pixel for this port's
 * depth via the ONE policy seam flair_look_pixel (os/flair/flair_look.h).
 * control.c names NO color and carries NO index->RGB switch (C-8). */
static uint32_t ctrl_px(const GrafPort *port, int part)
{
    return flair_look_pixel(port, part);
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

/* cfill_ctrl -- fill [x, x+w) on row y with PART `part`, clipped.
 * Mirrors chrome.c cfill; batches maximal in-clip runs into surface_fill_span.
 * PART -> pixel resolution is the ONE policy seam (ctrl_px -> flair_look_pixel;
 * C-8): control.c names a PART, never a color. */
static void cfill_ctrl(GrafPort *port, int x, int y, int w, int part)
{
    if (w <= 0) {
        return;
    }
    uint32_t px = ctrl_px(port, part);
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

/* crect_ctrl -- fill solid rectangle [x0,x1) x [y0,y1) with PART `part`. */
static void crect_ctrl(GrafPort *port, int x0, int y0, int x1, int y1, int part)
{
    for (int y = y0; y < y1; y++) {
        cfill_ctrl(port, x0, y, x1 - x0, part);
    }
}

/* cframe_ctrl -- 1 px hollow outline [x0,x1) x [y0,y1) with PART `part`. */
static void cframe_ctrl(GrafPort *port, int x0, int y0, int x1, int y1, int part)
{
    if (x1 <= x0 || y1 <= y0) {
        return;
    }
    cfill_ctrl(port, x0, y0,     x1 - x0, part);      /* top           */
    cfill_ctrl(port, x0, y1 - 1, x1 - x0, part);      /* bottom        */
    for (int y = y0; y < y1; y++) {
        cfill_ctrl(port, x0,     y, 1, part);          /* left          */
        cfill_ctrl(port, x1 - 1, y, 1, part);          /* right         */
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
 * Thumb height: SB_THUMB_MIN (15 px -- fixed until R1.5 supplies a content
 * range for proportional sizing; initech-tdnl.4).
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

/* draw_push_button -- sampled Mac OS 8.1 Platinum unpressed push button.
 *
 * The 20px class uses the exact asymmetric edge families measured on both
 * s8_alert_modal buttons: E7 inset, white highlight, C0/96 shadow, and the
 * C0/96/3F/CD rounded-corner ramp. Pressed art is a golden GAP, so the
 * pre-existing accent-fill tracking branch is retained and not claimed as
 * Platinum. Ref: ../system7-decomp/specs/sys8/controls.md Sec 5.1/Sec 8. */
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
    int face_part = hilited ? CTRL_ACCENT : CTRL_PLAT_FACE;

    if (hilited) {
        /* controls.md Sec 8: pressed/tracking is unresolved. Preserve the
         * retained heritage state byte-for-byte; do not invent Platinum art. */
        crect_ctrl(port, x0, y0, x1, y1, CTRL_ACCENT);
        cframe_ctrl(port, x0, y0, x1, y1, CTRL_BLACK);
        cfill_ctrl(port, x0,     y0,     1, CTRL_DESKTOP);
        cfill_ctrl(port, x1 - 1, y0,     1, CTRL_DESKTOP);
        cfill_ctrl(port, x0,     y1 - 1, 1, CTRL_DESKTOP);
        cfill_ctrl(port, x1 - 1, y1 - 1, 1, CTRL_DESKTOP);
    } else {
        crect_ctrl(port, x0, y0, x1, y1, CTRL_PLAT_FACE);
#if defined(CTRL_MUT_BTN_FLAT) && CTRL_MUT_BTN_FLAT
        /* NAMED MUTANT: remove the sampled bevel/corner anatomy. */
        cframe_ctrl(port, x0, y0, x1, y1, CTRL_BLACK);
#else
        /* Top and bottom rounded ramps: C0, 96, 3F, black run, 3F, 96, C0. */
        cfill_ctrl(port, x0,     y0, 1, CTRL_PLAT_SHADOW);
        cfill_ctrl(port, x0 + 1, y0, 1, CTRL_PLAT_DARK_SHADOW);
        cfill_ctrl(port, x0 + 2, y0, 1, CTRL_PLAT_DARK_RING);
        cfill_ctrl(port, x0 + 3, y0, w - 6, CTRL_BLACK);
        cfill_ctrl(port, x1 - 3, y0, 1, CTRL_PLAT_DARK_RING);
        cfill_ctrl(port, x1 - 2, y0, 1, CTRL_PLAT_DARK_SHADOW);
        cfill_ctrl(port, x1 - 1, y0, 1, CTRL_PLAT_SHADOW);
        cfill_ctrl(port, x0,     y1 - 1, 1, CTRL_PLAT_SHADOW);
        cfill_ctrl(port, x0 + 1, y1 - 1, 1, CTRL_PLAT_DARK_SHADOW);
        cfill_ctrl(port, x0 + 2, y1 - 1, 1, CTRL_PLAT_DARK_RING);
        cfill_ctrl(port, x0 + 3, y1 - 1, w - 6, CTRL_BLACK);
        cfill_ctrl(port, x1 - 3, y1 - 1, 1, CTRL_PLAT_DARK_RING);
        cfill_ctrl(port, x1 - 2, y1 - 1, 1, CTRL_PLAT_DARK_SHADOW);
        cfill_ctrl(port, x1 - 1, y1 - 1, 1, CTRL_PLAT_SHADOW);

        /* Corner-transition rows, transcribed from the two sampled buttons. */
        cfill_ctrl(port, x0,     y0 + 1, 1, CTRL_PLAT_DARK_SHADOW);
        cfill_ctrl(port, x0 + 1, y0 + 1, 1, CTRL_BLACK);
        cfill_ctrl(port, x0 + 2, y0 + 1, 1, CTRL_PLAT_CORNER);
        cfill_ctrl(port, x1 - 3, y0 + 1, 1, CTRL_PLAT_CORNER);
        cfill_ctrl(port, x1 - 2, y0 + 1, 1, CTRL_BLACK);
        cfill_ctrl(port, x1 - 1, y0 + 1, 1, CTRL_PLAT_DARK_SHADOW);

        cfill_ctrl(port, x0,     y0 + 2, 1, CTRL_PLAT_DARK_RING);
        cfill_ctrl(port, x0 + 1, y0 + 2, 1, CTRL_PLAT_CORNER);
        cfill_ctrl(port, x0 + 2, y0 + 2, w - 5, CTRL_WHITE);
        cfill_ctrl(port, x1 - 3, y0 + 2, 1, CTRL_PLAT_FACE);
        cfill_ctrl(port, x1 - 2, y0 + 2, 1, CTRL_PLAT_CORNER);
        cfill_ctrl(port, x1 - 1, y0 + 2, 1, CTRL_PLAT_DARK_RING);

        /* Straight body: black frame, E7 inset, white light, C0/96 dark. */
        for (int y = y0 + 3; y < y1 - 3; y++) {
            cfill_ctrl(port, x0,     y, 1, CTRL_BLACK);
            cfill_ctrl(port, x0 + 1, y, 1, CTRL_PLAT_FACE);
            cfill_ctrl(port, x0 + 2, y, 1, CTRL_WHITE);
            cfill_ctrl(port, x1 - 3, y, 1, CTRL_PLAT_SHADOW);
            cfill_ctrl(port, x1 - 2, y, 1, CTRL_PLAT_DARK_SHADOW);
            cfill_ctrl(port, x1 - 1, y, 1, CTRL_BLACK);
        }
        cfill_ctrl(port, x0 + 3, y0 + 3, 1, CTRL_WHITE);
        cfill_ctrl(port, x1 - 4, y1 - 4, 1, CTRL_PLAT_SHADOW);

        cfill_ctrl(port, x0,     y1 - 3, 1, CTRL_PLAT_DARK_RING);
        cfill_ctrl(port, x0 + 1, y1 - 3, 1, CTRL_PLAT_CORNER);
        cfill_ctrl(port, x0 + 2, y1 - 3, 1, CTRL_PLAT_FACE);
        cfill_ctrl(port, x0 + 3, y1 - 3, w - 6, CTRL_PLAT_SHADOW);
        cfill_ctrl(port, x1 - 3, y1 - 3, 2, CTRL_PLAT_DARK_SHADOW);
        cfill_ctrl(port, x1 - 1, y1 - 3, 1, CTRL_PLAT_DARK_RING);

        cfill_ctrl(port, x0,     y1 - 2, 1, CTRL_PLAT_DARK_SHADOW);
        cfill_ctrl(port, x0 + 1, y1 - 2, 1, CTRL_BLACK);
        cfill_ctrl(port, x0 + 2, y1 - 2, 1, CTRL_PLAT_CORNER);
        cfill_ctrl(port, x0 + 3, y1 - 2, w - 6, CTRL_PLAT_DARK_SHADOW);
        cfill_ctrl(port, x1 - 2, y1 - 2, 1, CTRL_BLACK);
        cfill_ctrl(port, x1 - 1, y1 - 2, 1, CTRL_PLAT_DARK_SHADOW);
#endif
    }

    /* Chicago label, centered. */
    uint32_t fg = ctrl_px(port, hilited ? CTRL_WHITE : CTRL_BLACK);
    uint32_t bg = ctrl_px(port, face_part);
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

static int checked_box_part(char cell) __attribute__((unused));
static int checked_box_part(char cell)
{
    switch (cell) {
    case 'K': return CTRL_BLACK;
    case 'W': return CTRL_WHITE;
    case 'a': return CTRL_PLAT_WIDGET_EDGE;
    case 'e': return CTRL_PLAT_FACE;
    case 'g': return CTRL_PLAT_DARK_SHADOW;
    case 'c': return CTRL_PLAT_SHADOW;
    default:  return CTRL_BLACK;
    }
}

/* draw_check_box -- exact sampled checked art, retained unchecked gap.
 *
 * The checked branch is the 12x12 controls.md Sec 2 transcription: black
 * outline, raised 10x10 E7 tile, and black X with 96/C0 shade. Every available
 * capture is checked. The unchecked branch therefore remains the pre-existing
 * heritage empty box and is not claimed as Platinum (Sec 2/Sec 8 GAP). */
static void draw_check_box(GrafPort *port, const ControlRecord *ctrl)
{
    int x0 = (int)ctrl->contrlRect.left;
    int y0 = (int)ctrl->contrlRect.top;
    int x1 = (int)ctrl->contrlRect.right;
    int y1 = (int)ctrl->contrlRect.bottom;
    int h  = y1 - y0;

    enum { BOX_SZ = 12 };   /* controls.md Sec 2: sampled 12 px footprint     */
    int box_y = y0 + (h - BOX_SZ) / 2;
    if (box_y < y0) {
        box_y = y0;
    }

    if (ctrl->contrlValue != 0) {
#if defined(CTRL_MUT_CHECK_FILL) && CTRL_MUT_CHECK_FILL
        /* NAMED MUTANT: relapse to the old solid accent-fill interior. */
        crect_ctrl(port,  x0, box_y, x0 + BOX_SZ, box_y + BOX_SZ, CTRL_WHITE);
        cframe_ctrl(port, x0, box_y, x0 + BOX_SZ, box_y + BOX_SZ, CTRL_BLACK);
        crect_ctrl(port, x0 + 2, box_y + 2, x0 + BOX_SZ - 2,
                   box_y + BOX_SZ - 2, CTRL_ACCENT);
#else
        static const char profile[BOX_SZ * BOX_SZ + 1] =
            "KKKKKKKKKKKK"
            "KWWWWWWWWWeK"
            "KWeKeeeeKeaK"
            "KWeKKeeKKgaK"
            "KWeeKKKKgcaK"
            "KWeeeKKgceaK"
            "KWeeKKKKeeaK"
            "KWeKKgcKKeaK"
            "KWeKgceeKgaK"
            "KWeeceeeecaK"
            "KeaaaaaaaaaK"
            "KKKKKKKKKKKK";
        for (int y = 0; y < BOX_SZ; y++) {
            for (int x = 0; x < BOX_SZ; x++) {
                cfill_ctrl(port, x0 + x, box_y + y, 1,
                           checked_box_part(profile[y * BOX_SZ + x]));
            }
        }
#endif
    } else {
        /* Unchecked Platinum art is a golden GAP: preserve heritage output. */
        crect_ctrl(port,  x0, box_y, x0 + BOX_SZ, box_y + BOX_SZ, CTRL_WHITE);
        cframe_ctrl(port, x0, box_y, x0 + BOX_SZ, box_y + BOX_SZ, CTRL_BLACK);
    }

    /* Title to the right of the box. */
    if (ctrl->contrlTitle[0]) {
        int label_h = text_cell_height(FONT_CHICAGO);
        int ly = y0 + (h - label_h) / 2;
        if (ly < y0) {
            ly = y0;
        }
        uint32_t fg = ctrl_px(port, CTRL_BLACK);
        uint32_t bg = ctrl_px(port, CTRL_PLAT_FACE);
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

/* Exact 8x4 Platinum arrow glyphs: widths 2/4/6/8. */
static void draw_ctrl_up_triangle(GrafPort *port, int cx, int top, int part)
{
    for (int row = 0; row < 4; row++) {
        int width = 2 + 2 * row;
        cfill_ctrl(port, cx - width / 2, top + row, width, part);
    }
}

static void draw_ctrl_down_triangle(GrafPort *port, int cx, int top, int part)
{
    for (int row = 0; row < 4; row++) {
        int width = 8 - 2 * row;
        cfill_ctrl(port, cx - width / 2, top + row, width, part);
    }
}

static void draw_ctrl_scroll_tile(GrafPort *port,
                                  int x0, int y0, int x1, int y1)
{
    crect_ctrl(port, x0, y0, x1, y1, FLAIR_PART_PLAT_FACE);
#if !defined(SB_MUT_TILE_FLAT)
    cfill_ctrl(port, x0, y0, x1 - x0 - 1, FLAIR_PART_CONTENT);
    for (int y = y0; y < y1; y++) {
        cfill_ctrl(port, x0, y, 1, FLAIR_PART_CONTENT);
    }
    cfill_ctrl(port, x0 + 1, y1 - 1, x1 - x0 - 1,
               FLAIR_PART_PLAT_TILE_SHADOW);
    for (int y = y0; y < y1; y++) {
        cfill_ctrl(port, x1 - 1, y, 1, FLAIR_PART_PLAT_TILE_SHADOW);
    }
#endif
}

static void draw_ctrl_scroll_well(GrafPort *port,
                                  int x0, int y0, int x1, int y1)
{
    if (x1 <= x0 || y1 <= y0) {
        return;
    }
#if defined(SB_MUT_FLAT_WELL)
    crect_ctrl(port, x0, y0, x1, y1, FLAIR_PART_PLAT_WELL);
#else
    for (int y = y0; y < y1; y++) {
        cfill_ctrl(port, x0, y, 1, FLAIR_PART_PLAT_STRIPE_DARK);
        cfill_ctrl(port, x0 + 1, y, 1, FLAIR_PART_PLAT_WIDGET_EDGE);
        cfill_ctrl(port, x0 + 2, y, FLAIR_CHROME_SCROLL_WELL_FILL_ROWS,
                   FLAIR_PART_PLAT_WELL);
        cfill_ctrl(port, x1 - 2, y, 1, FLAIR_PART_PLAT_TILE_SHADOW);
        cfill_ctrl(port, x1 - 1, y, 1, FLAIR_PART_PLAT_FRAME_FACE);
    }
    cfill_ctrl(port, x0, y0, x1 - x0, FLAIR_PART_PLAT_STRIPE_DARK);
    if (y0 + 1 < y1) {
        cfill_ctrl(port, x0, y0 + 1, x1 - x0,
                   FLAIR_PART_PLAT_WIDGET_EDGE);
    }
#endif
}

/* Transpose of the sampled horizontal 15x14 thumb matrix.  The clut-208
 * lavender entries retain their semantic anatomy while DEC-10 OQ-2/OQ-3 maps
 * them onto the existing two-row Initech-teal canon. */
static void draw_ctrl_scroll_thumb(GrafPort *port,
                                   int x0, int x1, int ty, int thumb_h)
{
    crect_ctrl(port, x0, ty, x1, ty + thumb_h, CTRL_SB_THUMB_FACE);

    /* Leading/top and left highlight, trailing/bottom and right shadow. */
    cfill_ctrl(port, x0, ty, 1, FLAIR_PART_PLAT_TROUGH);
    cfill_ctrl(port, x0 + 1, ty, x1 - x0 - 2, CTRL_SB_THUMB_HL);
    for (int y = ty + 1; y < ty + thumb_h - 1; y++) {
        cfill_ctrl(port, x0, y, 1, CTRL_SB_THUMB_HL);
        cfill_ctrl(port, x1 - 1, y, 1, CTRL_SB_THUMB_SHADOW);
    }
    cfill_ctrl(port, x0 + 1, ty + thumb_h - 1, x1 - x0 - 1,
               CTRL_SB_THUMB_SHADOW);

    /* Four dark seven-pixel grip lines, with six-pixel highlight companions
     * one axis pixel earlier and an F3 cap at cross offset 3. */
    for (int line = 0; line < 4; line++) {
        int companion_y = ty + 3 + 2 * line;
        int grip_y = companion_y + 1;
        cfill_ctrl(port, x0 + 3, companion_y, 1,
                   FLAIR_PART_PLAT_TROUGH);
        cfill_ctrl(port, x0 + 4, companion_y, 6, CTRL_SB_THUMB_HL);
        cfill_ctrl(port, x0 + 4, grip_y, 7, CTRL_SB_THUMB_GRIP);
    }
}

/* draw_scrollbar -- vertical Mac OS 8.1 Platinum scrollbar.
 *
 * Enabled: black frame/separators, raised arrow tiles, recessed five-value
 * well, and a fixed 15px accent thumb positioned by value/min/max.
 * Disabled (contrlHilite==255): flat F3 interior, gray separators/arrows, no
 * thumb. Proportional sizing is honestly deferred to R1.5 / initech-tdnl.4.
 * Ref: ../system7-decomp/specs/sys8/scrollbars.md Sec 1-5 (SAMPLED). */
static void draw_scrollbar(GrafPort *port, const ControlRecord *ctrl)
{
    int x0 = (int)ctrl->contrlRect.left;
    int y0 = (int)ctrl->contrlRect.top;
    int x1 = (int)ctrl->contrlRect.right;
    int y1 = (int)ctrl->contrlRect.bottom;
    int h = y1 - y0;
    int btn = SB_ARROW;
    int disabled = ctrl->contrlHilite == 255;
#if defined(SB_MUT_DISABLED_ENABLED_LOOK)
    disabled = 0;
#endif

    if (x1 - x0 != FLAIR_CHROME_SCROLLBAR_W ||
        h < 2 * btn + SB_THUMB_MIN + 2) {
        return;
    }

    int top_sep = y0 + btn - 1;
    int bottom_sep = y1 - btn;
    int track_top = top_sep + 1;
    int track_bot = bottom_sep;
    int cx = x0 + FLAIR_CHROME_SCROLLBAR_W / 2;

    crect_ctrl(port, x0 + 1, y0 + 1, x1 - 1, y1 - 1,
               FLAIR_PART_PLAT_TROUGH);
    cframe_ctrl(port, x0, y0, x1, y1, CTRL_BLACK);

    if (disabled) {
        cfill_ctrl(port, x0 + 1, top_sep, x1 - x0 - 2,
                   FLAIR_PART_PLAT_INACTIVE_FRAME);
        cfill_ctrl(port, x0 + 1, bottom_sep, x1 - x0 - 2,
                   FLAIR_PART_PLAT_INACTIVE_FRAME);
        draw_ctrl_up_triangle(port, cx, y0 + 6,
                              FLAIR_PART_PLAT_WIDGET_EDGE);
        draw_ctrl_down_triangle(port, cx, y1 - 10,
                                FLAIR_PART_PLAT_WIDGET_EDGE);
        return;
    }

    int thumb_h = SB_THUMB_MIN;
    int ty = (int)ctrl_thumb_y(ctrl);
    int leading_sep = ty - 1;
    int trailing_sep = ty + thumb_h;
    int sep_part = FLAIR_PART_FRAME;
#if defined(SB_MUT_SEP_GRAY)
    sep_part = FLAIR_PART_PLAT_INACTIVE_FRAME;
#endif

    draw_ctrl_scroll_well(port, x0 + 1, track_top,
                          x1 - 1, leading_sep);
    draw_ctrl_scroll_well(port, x0 + 1, trailing_sep + 1,
                          x1 - 1, track_bot);
    draw_ctrl_scroll_thumb(port, x0 + 1, x1 - 1, ty, thumb_h);
    draw_ctrl_scroll_tile(port, x0 + 1, y0 + 1, x1 - 1, top_sep);
    draw_ctrl_scroll_tile(port, x0 + 1, bottom_sep + 1,
                          x1 - 1, y1 - 1);

    cfill_ctrl(port, x0 + 1, top_sep, x1 - x0 - 2, sep_part);
    cfill_ctrl(port, x0 + 1, bottom_sep, x1 - x0 - 2, sep_part);
    cfill_ctrl(port, x0 + 1, leading_sep, x1 - x0 - 2, sep_part);
    cfill_ctrl(port, x0 + 1, trailing_sep, x1 - x0 - 2, sep_part);
    draw_ctrl_up_triangle(port, cx, y0 + 6, FLAIR_PART_FRAME);
    draw_ctrl_down_triangle(port, cx, y1 - 10, FLAIR_PART_FRAME);
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

/* DrawControlDefaultRing -- Dialog Manager defaultItem decoration.
 *
 * The ring bounds are InsetRect(button,-3,-3). The two-pixel moat is E7/C0
 * on top/left and C0/96 on bottom/right. The button is drawn afterward and
 * covers the moat's inner boundary. Ref: controls.md Sec 5.2. */
void DrawControlDefaultRing(GrafPort *port, const ControlRecord *ctrl)
{
    if (port == 0 || ctrl == 0 || ctrl->contrlType != pushButton ||
        !ctrl->contrlVis) {
        return;
    }
#if defined(CTRL_MUT_NO_DEFAULT_RING) && CTRL_MUT_NO_DEFAULT_RING
    /* NAMED MUTANT: defaultItem no longer reaches visible ring pixels. */
    return;
#else
    int bx0 = (int)ctrl->contrlRect.left;
    int by0 = (int)ctrl->contrlRect.top;
    int bx1 = (int)ctrl->contrlRect.right;
    int by1 = (int)ctrl->contrlRect.bottom;
    int x0 = bx0 - 3;
    int y0 = by0 - 3;
    int x1 = bx1 + 3;
    int y1 = by1 + 3;

    if (bx1 <= bx0 || by1 <= by0) {
        return;
    }

    /* Establish the sampled E7 field within the expanded bounds. */
    crect_ctrl(port, x0, y0, x1, y1, CTRL_PLAT_FACE);

    /* One-pixel rounded black ring with sampled 3F smoothing pixels. */
    cfill_ctrl(port, x0 + 3, y0,     x1 - x0 - 6, CTRL_BLACK);
    cfill_ctrl(port, x0 + 3, y1 - 1, x1 - x0 - 6, CTRL_BLACK);
    for (int y = y0 + 3; y < y1 - 3; y++) {
        cfill_ctrl(port, x0,     y, 1, CTRL_BLACK);
        cfill_ctrl(port, x1 - 1, y, 1, CTRL_BLACK);
    }
    cfill_ctrl(port, x0 + 2, y0,     1, CTRL_PLAT_DARK_RING);
    cfill_ctrl(port, x1 - 3, y0,     1, CTRL_PLAT_DARK_RING);
    cfill_ctrl(port, x0 + 2, y1 - 1, 1, CTRL_PLAT_DARK_RING);
    cfill_ctrl(port, x1 - 3, y1 - 1, 1, CTRL_PLAT_DARK_RING);
    cfill_ctrl(port, x0 + 1, y0 + 1, 1, CTRL_BLACK);
    cfill_ctrl(port, x1 - 2, y0 + 1, 1, CTRL_BLACK);
    cfill_ctrl(port, x0 + 1, y1 - 2, 1, CTRL_BLACK);
    cfill_ctrl(port, x1 - 2, y1 - 2, 1, CTRL_BLACK);
    cfill_ctrl(port, x0,     y0 + 2, 1, CTRL_PLAT_DARK_RING);
    cfill_ctrl(port, x1 - 1, y0 + 2, 1, CTRL_PLAT_DARK_RING);
    cfill_ctrl(port, x0,     y1 - 3, 1, CTRL_PLAT_DARK_RING);
    cfill_ctrl(port, x1 - 1, y1 - 3, 1, CTRL_PLAT_DARK_RING);

    /* Straight moat legs; DrawControl overlays the central button next. */
    cfill_ctrl(port, bx0, y0 + 1, bx1 - bx0, CTRL_PLAT_FACE);
    cfill_ctrl(port, bx0, y0 + 2, bx1 - bx0, CTRL_PLAT_SHADOW);
    for (int y = by0; y < by1; y++) {
        cfill_ctrl(port, x0 + 1, y, 1, CTRL_PLAT_FACE);
        cfill_ctrl(port, x0 + 2, y, 1, CTRL_PLAT_SHADOW);
        cfill_ctrl(port, x1 - 3, y, 1, CTRL_PLAT_SHADOW);
        cfill_ctrl(port, x1 - 2, y, 1, CTRL_PLAT_DARK_SHADOW);
    }
    cfill_ctrl(port, bx0, by1,     bx1 - bx0, CTRL_PLAT_SHADOW);
    cfill_ctrl(port, bx0, by1 + 1, bx1 - bx0, CTRL_PLAT_DARK_SHADOW);
#endif
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
