/*
 * os/flair/winscroll.h -- the STANDARD WINDOW SCROLL BARS (THE ARTIFACT).
 *
 * beads: initech-tdnl.35 (P0, audit F02: "Scrollbar input drags the window;
 *        overflowing files are unreachable normally"); first slice of
 *        initech-tdnl.4 (R1.5 live scrollbars).
 *
 * WHAT THIS IS. Every Platinum document window carries a vertical and a
 * horizontal scroll bar in the 16-px gutters the WDEF reserves (chrome.c draws
 * them; StandardWDEF scrollBarSize = 16). In the Toolbox those bars are two
 * Control Manager scroll-bar controls owned by the application: FindWindow
 * answers inContent for them, the application calls FindControl, then
 * TrackControl -- arrows and page regions REPEAT while the button is held,
 * the thumb is dragged and the value is set on release
 * (../system7-decomp/specs/toolbox/control-manager.md "Hit-testing and
 * tracking"; manager-part-codes.md inUpButton=20 .. inThumb=129).
 *
 * FLAIR has no per-application control list in the locked WindowRecord
 * (spec/window_record.h has no controlList field, and that record is LOCKED
 * spec-data, Rule 8), so the two standard bars of a window are modelled HERE,
 * once, for every document window:
 *
 *   - THE STATE is a caller-supplied WindowScroll record (one ControlRecord's
 *     value/min/max/hilite per axis; contrlMin is always 0). A client that has
 *     scrollable content (the Finder's icon view) embeds one and attaches it
 *     to its window through the Window Manager registry (window.h
 *     WindowScrollAttach). A window with NO attached record -- a disk app, a
 *     built-in tenant, a film window -- has nothing to scroll: both its bars
 *     are DISABLED and every click on them does nothing (scrollbars.md Sec 3:
 *     "the folder contents fit the view" draws the disabled bar, no thumb).
 *   - THE GEOMETRY is the gutter chrome.c already draws (the two bar rects
 *     below are exactly its frame lines), so hit-testing and drawing cannot
 *     disagree.
 *   - THE ARITHMETIC (thumb position <-> value, part codes, line/page steps)
 *     is the Inside Macintosh convention control.c already implements for its
 *     vertical control (control.h "SCROLLBAR VALUE<->THUMB MATH"), restated
 *     axis-generically for both bars. Pure static inline functions: no link
 *     dependency, host-testable (harness/proptest/test_winscroll.c).
 *
 * THE UNIT of value/max/line/page is the PIXEL of document scrolled.
 *
 * WHAT THE REFERENCES DO NOT SAY, AND WHAT WAS CHOSEN (stated, Law 1):
 *   - Thumb size: scrollbars.md Sec 6 cannot tell whether the 8.1 thumb is
 *     proportional; both measured thumbs are 15 px and System 7's is fixed.
 *     FIXED 15 px here (FLAIR_CHROME_SCROLL_THUMB_MIN), as control.c.
 *   - Line step: no local reference gives the Finder's arrow increment. 16 px
 *     (one arrow-tile length) -- a stated choice, not a citation.
 *   - Page step: the view length minus one line, so one line of the previous
 *     page stays visible -- the common Toolbox-application convention; no
 *     local reference, stated.
 *   - Thumb drag: control-manager.md -- TrackControl moves the indicator with
 *     the mouse and sets the value ON RELEASE (no live scrolling; the 8.x
 *     live-scroll feedback is an explicit Sec 6 gap). The content follows on
 *     release.
 *   - Auto-repeat timing: TrackControl calls the action procedure "as long as
 *     the button is held" (control-manager.md); no local reference gives a
 *     delay. One step on the press, repeats after WSCROLL_REPEAT_DELAY ticks,
 *     then every WSCROLL_REPEAT_RATE ticks (100 Hz PIT) -- stated choices.
 *   - Pressed arrow art: scrollbars.md Sec 6 "the pressed arrow ... is
 *     unknown" -- authored in chrome.c after the System 7 pressed delta.
 *
 * Ref: ../system7-decomp/specs/sys8/scrollbars.md Sec 1-6 (geometry, the
 *      ENABLED / DISABLED / HOLLOW states, the gaps);
 *      ../system7-decomp/specs/toolbox/control-manager.md (part codes,
 *      TrackControl, the value<->thumb mapping);
 *      ../system7-decomp/specs/toolbox/window-manager.md Sec 2 (FindWindow
 *      inContent); os/flair/control.h (the vertical control's same math);
 *      spec/chrome_metrics.h (FLAIR_CHROME_SCROLLBAR_W, _SCROLL_ARROW_TILE,
 *      _SCROLL_THUMB_MIN, _GROW_*); os/flair/chrome.c flair_draw_document_window
 *      (the gutter lines these rects reproduce).
 *      CLAUDE.md Law 1, Law 3 (freestanding), Rule 2, Rule 11, Rule 12.
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_WINSCROLL_H
#define INITECH_OS_FLAIR_WINSCROLL_H

#include <stdint.h>
#include <stddef.h>

#include "region_algebra.h"   /* rgn_rect_t (-Ispec)                           */
#include "window_record.h"    /* WindowRecord, flair_point_t (-Ispec)          */
#include "chrome_metrics.h"   /* the scroll-bar + grow metrics (-Ispec)        */

/* Control Manager part codes for a scroll bar -- verbatim IM-I p. I-327, the
 * SAME values os/flair/control.h asserts (inUpButton=20 ... inThumb=129). They
 * are restated under WSCROLL_ names so this header needs no control.h (whose
 * enum would otherwise be pulled into every Window Manager client). */
#define WSCROLL_PART_NONE      0
#define WSCROLL_PART_UP       20   /* inUpButton   -- up / left arrow           */
#define WSCROLL_PART_DOWN     21   /* inDownButton -- down / right arrow        */
#define WSCROLL_PART_PAGEUP   22   /* inPageUp     -- track above / left thumb  */
#define WSCROLL_PART_PAGEDOWN 23   /* inPageDown   -- track below / right thumb */
#define WSCROLL_PART_THUMB   129   /* inThumb      -- the indicator             */

#define WSCROLL_V     0            /* the vertical bar (right gutter)           */
#define WSCROLL_H     1            /* the horizontal bar (bottom gutter)        */
#define WSCROLL_AXES  2

#define WSCROLL_LINE_DEFAULT  16   /* px per arrow click (stated choice, above) */
#define WSCROLL_REPEAT_DELAY  30   /* ticks before the first repeat (choice)    */
#define WSCROLL_REPEAT_RATE    5   /* ticks between repeats (choice)            */

/* One bar's ControlRecord state (control-manager.md ControlRecord: value,
 * min, max, hilite). contrlMin is always 0. */
typedef struct WindowScrollAxis {
    int16_t value;     /* contrlValue: document px scrolled, in [0, max]       */
    int16_t max;       /* contrlMax: content length - view length, >= 0        */
    int16_t line;      /* arrow step, px                                       */
    int16_t page;      /* page step, px                                        */
    int16_t hilite;    /* contrlHilite: the part held down, or 0               */
    int16_t drag_pos;  /* thumb leading edge while inThumb is tracked, else -1 */
} WindowScrollAxis;

/* The two standard bars of one window. Caller-supplied storage (Law 3: no
 * malloc); linked into WindowMgr.scrolls by WindowScrollAttach. */
typedef struct WindowScroll {
    struct WindowScroll *next;     /* WindowMgr registry link                  */
    WindowRecord        *owner;    /* the window these bars belong to          */
    WindowScrollAxis     axis[WSCROLL_AXES];
} WindowScroll;

/* ---------------------------------------------------------------------------
 * GEOMETRY -- the control rects of the two bars of a document window whose
 * drawer frame is `frame` (WindowFrameRect). These are EXACTLY the outer lines
 * chrome.c draws (flair_draw_document_window):
 *   vertical   x [R-21, R-5)  y [T+21, B-20)  -- top line on the title's
 *              shared bottom row, bottom line just above the 18-px grow cell
 *   horizontal x [L+5, R-20)  y [B-21, B-5)   -- left line on the body rail's
 *              inner line, right line shared with the vertical bar's left line
 * with R-21 = right - FRAME - BODY_BAR - SCROLLBAR_W, B-20 = bottom - 1 -
 * GROW_BOTTOM_OFF, and so on (spec/chrome_metrics.h). Half-open rects.
 * ------------------------------------------------------------------------- */
static inline rgn_rect_t wscroll_bar_rect(rgn_rect_t frame, int axis)
{
    rgn_rect_t r;
    int grow_x = (int)frame.right - 1 - FLAIR_CHROME_GROW_RIGHT_OFF;   /* R-20 */
    int grow_y = (int)frame.bottom - 1 - FLAIR_CHROME_GROW_BOTTOM_OFF; /* B-20 */
    if (axis == WSCROLL_V) {
        r.left   = (int16_t)(grow_x - FLAIR_CHROME_FRAME);             /* R-21 */
        r.right  = (int16_t)(r.left + FLAIR_CHROME_SCROLLBAR_W);       /* R-5  */
        r.top    = (int16_t)((int)frame.top + FLAIR_CHROME_TITLEBAR_H - 1);
        r.bottom = (int16_t)grow_y;                                    /* B-20 */
    } else {
        r.top    = (int16_t)(grow_y - FLAIR_CHROME_FRAME);             /* B-21 */
        r.bottom = (int16_t)(r.top + FLAIR_CHROME_SCROLLBAR_W);        /* B-5  */
        r.left   = (int16_t)((int)frame.left + FLAIR_CHROME_FRAME +
                             FLAIR_CHROME_BODY_BAR);                   /* L+5  */
        r.right  = (int16_t)grow_x;                                    /* R-20 */
    }
    return r;
}

/* The bar's extent ALONG its axis: [*lo, *hi). */
static inline void wscroll_along(rgn_rect_t bar, int axis, int *lo, int *hi)
{
    if (axis == WSCROLL_V) { *lo = bar.top;  *hi = bar.bottom; }
    else                   { *lo = bar.left; *hi = bar.right;  }
}

static inline int wscroll_pt_in(rgn_rect_t r, flair_point_t pt)
{
    return pt.h >= r.left && pt.h < r.right && pt.v >= r.top && pt.v < r.bottom;
}

/* 1 when the bar can scroll (ENABLED, scrollbars.md Sec 2); 0 when everything
 * fits (DISABLED, Sec 3) or there is no model at all. */
static inline int wscroll_enabled(const WindowScrollAxis *a)
{
    return a != (const WindowScrollAxis *)0 && a->max > 0;
}

/* The track the thumb's leading edge travels: [track_lo, track_lo + span].
 * The arrow tiles are FLAIR_CHROME_SCROLL_ARROW_TILE (16) long including the
 * bar's outer line and the separator; the thumb is 15 long between two
 * separators (scrollbars.md Sec 2.1: arrow 315..328, K 329, thumb 330..344,
 * K 345 for a bar whose outer line is 314). */
static inline int wscroll_track_lo(rgn_rect_t bar, int axis)
{
    int lo, hi;
    wscroll_along(bar, axis, &lo, &hi);
    (void)hi;
    return lo + FLAIR_CHROME_SCROLL_ARROW_TILE;
}

static inline int wscroll_track_span(rgn_rect_t bar, int axis)
{
    int lo, hi, span;
    wscroll_along(bar, axis, &lo, &hi);
    span = (hi - FLAIR_CHROME_SCROLL_ARROW_TILE - FLAIR_CHROME_SCROLL_THUMB_MIN)
           - (lo + FLAIR_CHROME_SCROLL_ARROW_TILE);
    return span < 0 ? 0 : span;
}

/* value -> thumb leading edge (IM "Calculating Scroll Bar Thumb Position";
 * control.c ctrl_thumb_y, both axes):
 *   pos = track_lo + value * span / max          (integer division)       */
static inline int wscroll_thumb_pos(rgn_rect_t bar, int axis,
                                    const WindowScrollAxis *a)
{
    int lo = wscroll_track_lo(bar, axis);
    int span = wscroll_track_span(bar, axis);
    int v;
    if (a == (const WindowScrollAxis *)0 || a->max <= 0 || span <= 0) return lo;
    v = a->value;
    if (v < 0) v = 0;
    if (v > a->max) v = a->max;
#if defined(WSCROLL_MUT_THUMB_UNSCALED)
    /* MUTANT (Rule 6; test-winscroll): the thumb ignores the range -- one
     * value unit per pixel, the pre-proportional-mapping slip. NEVER real. */
    return lo + (v > span ? span : v);
#else
    return lo + (int)((int32_t)v * (int32_t)span / (int32_t)a->max);
#endif
}

/* thumb leading edge -> value, the inverse, clamped to [0, max]
 * (control-manager.md: "the round-trip-invertible mapping, with the result
 * CLAMPED to [min, max]"). The forward map floors, so the inverse takes the
 * SMALLEST value whose thumb lands at `pos` (a ceiling): value -> pos -> value
 * is then the identity whenever span >= max (graded by test-winscroll W2). */
static inline int16_t wscroll_value_from_thumb(rgn_rect_t bar, int axis,
                                               const WindowScrollAxis *a,
                                               int pos)
{
    int lo = wscroll_track_lo(bar, axis);
    int span = wscroll_track_span(bar, axis);
    int off;
    if (a == (const WindowScrollAxis *)0 || a->max <= 0 || span <= 0) return 0;
    off = pos - lo;
    if (off < 0) off = 0;
    if (off > span) off = span;
    return (int16_t)(((int32_t)off * (int32_t)a->max + (int32_t)(span - 1)) /
                     (int32_t)span);
}

/* TestControl for one bar: the part code under `pt`, or 0 when the point is
 * outside the bar OR the bar is not enabled (an inactive control is never hit:
 * control-manager.md "A point in an INACTIVE control returns 0"). The thumb is
 * located at the TRACKED position while a drag is in progress. */
static inline int wscroll_test_part(rgn_rect_t bar, int axis,
                                    const WindowScrollAxis *a,
                                    flair_point_t pt)
{
    int lo, hi, p, tp;
    if (!wscroll_pt_in(bar, pt) || !wscroll_enabled(a)) return WSCROLL_PART_NONE;
    wscroll_along(bar, axis, &lo, &hi);
    p = (axis == WSCROLL_V) ? pt.v : pt.h;
    if (p < lo + FLAIR_CHROME_SCROLL_ARROW_TILE) return WSCROLL_PART_UP;
    if (p >= hi - FLAIR_CHROME_SCROLL_ARROW_TILE) return WSCROLL_PART_DOWN;
    tp = (a->drag_pos >= 0) ? a->drag_pos : wscroll_thumb_pos(bar, axis, a);
    if (p >= tp && p < tp + FLAIR_CHROME_SCROLL_THUMB_MIN) return WSCROLL_PART_THUMB;
    return (p < tp) ? WSCROLL_PART_PAGEUP : WSCROLL_PART_PAGEDOWN;
}

/* Clamp + store a value. Returns 1 when it changed. */
static inline int wscroll_set_value(WindowScrollAxis *a, int v)
{
    if (a == (WindowScrollAxis *)0) return 0;
    if (v > a->max) v = a->max;
    if (v < 0) v = 0;
    if (v == a->value) return 0;
    a->value = (int16_t)v;
    return 1;
}

/* The action procedure for an arrow / page part: one step. Returns 1 when
 * the value changed (0 at either end -- the repeat then has nothing to do). */
static inline int wscroll_step(WindowScrollAxis *a, int part)
{
    int d;
    if (!wscroll_enabled(a)) return 0;
    switch (part) {
    case WSCROLL_PART_UP:       d = -a->line; break;
    case WSCROLL_PART_DOWN:     d =  a->line; break;
#if defined(WSCROLL_MUT_PAGE_WRONG_DIR)
    /* MUTANT (Rule 6; test-winscroll, the R1.5 PAGE_WRONG_DIR class): the
     * page regions scroll the wrong way. NEVER in a real build. */
    case WSCROLL_PART_PAGEUP:   d =  a->page; break;
    case WSCROLL_PART_PAGEDOWN: d = -a->page; break;
#else
    case WSCROLL_PART_PAGEUP:   d = -a->page; break;
    case WSCROLL_PART_PAGEDOWN: d =  a->page; break;
#endif
    default: return 0;
    }
    return wscroll_set_value(a, (int)a->value + d);
}

/* Set an axis' range from a content length and the view length along that
 * axis (both px). max = content - view (0 when it fits); page = view - line
 * (never below one line); the value is re-clamped. Returns 1 when the value
 * had to change (the content shrank under it). */
static inline int wscroll_set_extent(WindowScrollAxis *a, int content_len,
                                     int view_len)
{
    int m, pg;
    if (a == (WindowScrollAxis *)0) return 0;
    if (a->line <= 0) a->line = WSCROLL_LINE_DEFAULT;
    m = content_len - view_len;
    if (m < 0) m = 0;
    if (m > 32767) m = 32767;
    a->max = (int16_t)m;
    pg = view_len - a->line;
    if (pg < a->line) pg = a->line;
    a->page = (int16_t)pg;
    return wscroll_set_value(a, a->value);
}

#endif /* INITECH_OS_FLAIR_WINSCROLL_H */
