/*
 * os/flair/control.h -- FLAIR Control Manager: the LOCKED contract (THE ARTIFACT).
 *
 * beads: initech-8h9 ("FLAIR Control Manager: buttons, scrollbars, progress bar").
 *
 * Ref: ADR-0004 D-3 ("Control Manager -- ControlRecord {value/min/max,
 *        contrlHilite, contrlRect}; part-codes inButton, inCheckBox,
 *        inUpButton/inDownButton/inPageUp/inPageDown/inThumb. Buttons,
 *        scrollbars, the FILE COPY progress bar.");
 *      Inside Macintosh: Macintosh Toolbox Essentials (MTE) Ch 5 "The Control
 *        Manager" -- the PRIMARY reference for field names and part-codes:
 *        ControlRecord (MTE p. 5-51..5-54), part-code constants (MTE p. 5-63
 *        Table 5-3 "Part Code Constants"); NewControl, DrawControl,
 *        TestControl, TrackControl, GetControlValue, SetControlValue,
 *        GetControlMinimum, GetControlMaximum (MTE Ch 5 Routines).
 *      Inside Macintosh Vol I Ch 5 (IM-I) "The Control Manager" -- the original
 *        verbatim record (IM-I p. I-318: contrlRect, contrlValue, contrlMin,
 *        contrlMax, contrlHilite, contrlVis, contrlTitle, contrlRfCon);
 *        part-code table (IM-I p. I-327: inButton=10, inCheckBox=11,
 *        inUpButton=20, inDownButton=21, inPageUp=22, inPageDown=23,
 *        inThumb=129).
 *      spec/chrome_metrics.h (FLAIR_CHROME_SCROLLBAR_W = 16); scrollbar
 *        layout and arrow button size derive from this locked constant.
 *      os/flair/blitter.{c,h} (the region-clipped fill/blit primitives);
 *        os/flair/text.{c,h} (Chicago for button labels; text_measure /
 *        text_draw / text_center_in); os/flair/surface.h (bitmap_t);
 *        spec/grafport.h (GrafPort).
 *      CLAUDE.md Law 1 (ground truth -- verbatim IM names; metrics from the
 *        LOCKED spec), Law 2 (oracle in test_control.c), Law 3 (freestanding
 *        artifact; dual-compile), Rule 2 (fail loud), Rule 11 (deterministic),
 *        Rule 12 (ASCII-clean source).
 *
 * ARTIFACT CODE: freestanding C (ADR-0002). No libc. No malloc. No 2026-isms.
 * Dual-compile: kernel (gcc -m32 -ffreestanding -nostdlib -DFLAIR_HOSTED=0)
 *               hosted  (gcc -DFLAIR_HOSTED=1) for the oracle harness.
 *
 * VERBATIM INSIDE MACINTOSH NAMES (ADR-0004 D-3):
 *   ControlRecord fields: contrlRect, contrlValue, contrlMin, contrlMax,
 *     contrlHilite, contrlVis, contrlType, contrlTitle, contrlRfCon.
 *   Part-code constants: inButton, inCheckBox, inUpButton, inDownButton,
 *     inPageUp, inPageDown, inThumb.
 *   Routines: DrawControl, TestControl, TrackControl,
 *     SetControlValue, GetControlValue.
 *
 * CALLER-SUPPLIED STORAGE (no malloc):
 *   ControlRecord storage is provided by the caller. No heap allocation here.
 *   The draw routines write only into the caller-supplied GrafPort.
 *
 * CONTROL TYPES (contrlType):
 *   pushButton    (0) -- push button; centered Chicago label; hilites on press.
 *   checkBox      (1) -- check box; box + check mark; contrlValue 0/1.
 *   radioButton   (2) -- radio button; circle + dot; contrlValue 0/1.
 *   scrollBar     (3) -- vertical, FLAIR_CHROME_SCROLLBAR_W (16) px wide;
 *                        up/down arrow buttons + track + proportional thumb.
 *   progressBar   (4) -- determinate fill bar (the FILE COPY bar, PRD Sec 6.5).
 *                        contrlValue/contrlMax -> filled fraction.
 *
 * SCROLLBAR VALUE<->THUMB MATH (Law 1; verbatim Inside Macintosh convention):
 *   Ref: MTE Ch 5 "Scrolling with Scroll Bars" p. 5-14, Inside Macintosh IV
 *     "Calculating Scroll Bar Thumb Position."
 *   The scrollbar TRACK is the region between the two arrow buttons:
 *     track_top    = contrlRect.top + SB_ARROW
 *     track_bot    = contrlRect.bottom - SB_ARROW
 *     track_h      = track_bot - track_top
 *   where SB_ARROW = FLAIR_CHROME_SCROLLBAR_W (16 px -- square buttons).
 *   The thumb size (proportional) and position:
 *     thumb_h      = max(SB_THUMB_MIN, track_h * visible_range / value_range)
 *   For the simple (non-proportional) scrollbar (visible_range == 1):
 *     thumb_h      = SB_THUMB_MIN  (the minimum thumb, 16 px)
 *   The thumb top position within the track:
 *     range        = contrlMax - contrlMin
 *     if (range == 0): thumb_y = track_top
 *     else:            thumb_y = track_top +
 *                        (contrlValue - contrlMin) * (track_h - thumb_h) / range
 *   This is INVERTIBLE: given thumb_y, the value is:
 *     if (track_h == thumb_h): value = contrlMin
 *     else: value = contrlMin +
 *             (thumb_y - track_top) * range / (track_h - thumb_h)
 *   (integer division; the forward and inverse are verified by the oracle.)
 *
 * PROGRESS BAR FILL MODEL:
 *   filled_px = inner_w * contrlValue / contrlMax   (integer division)
 *   inner_w = contrlRect.right - contrlRect.left - 2  (inside the 1 px border)
 *   At contrlValue==0: filled_px == 0 (no fill).
 *   At contrlValue==contrlMax: filled_px == inner_w (full fill).
 *
 * ASCII-clean (Rule 12). No nondeterminism / no timestamps (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_CONTROL_H
#define INITECH_OS_FLAIR_CONTROL_H

#include <stdint.h>
#include <stddef.h>

#include "grafport.h"           /* GrafPort, flair_point_t (-Ispec)             */
#include "region_algebra.h"     /* rgn_rect_t (-Ispec)                          */
#include "chrome_metrics.h"     /* FLAIR_CHROME_SCROLLBAR_W (-Ispec)            */

/* ===========================================================================
 * 1. CONTROL TYPES  (contrlType field values)
 * ---------------------------------------------------------------------------
 * Ref: Inside Macintosh Vol I Ch 5 (IM-I) p. I-319; MTE Ch 5 "Control Types."
 * FLAIR implements pushButton, checkBox, radioButton from IM-I, plus the two
 * non-IM types scrollBar and progressBar that the current release requires.
 * ===========================================================================*/
typedef enum flair_ctrl_type {
    pushButton  = 0,   /* push button (IM-I p. I-319)                           */
    checkBox    = 1,   /* check box (IM-I p. I-319)                             */
    radioButton = 2,   /* radio button (IM-I p. I-319)                          */
    scrollBar   = 3,   /* vertical scrollbar (16 px wide; chrome_metrics.h)     */
    progressBar = 4    /* determinate progress bar (FILE COPY; PRD Sec 6.5)     */
} flair_ctrl_type_t;

/* ===========================================================================
 * 2. PART-CODE CONSTANTS  (verbatim IM-I p. I-327 / MTE Table 5-3)
 * ---------------------------------------------------------------------------
 * Part codes identify which part of a control a hit or tracking event targets.
 * Values are verbatim from Inside Macintosh (IM-I p. I-327; MTE p. 5-63).
 * ===========================================================================*/
enum {
    /* Part codes for push button, check box, radio button (IM-I p. I-327):   */
    inButton  = 10,   /* inside a push button; also generic "in control"        */
    inCheckBox = 11,  /* inside a check box or radio button                     */

    /* Part codes for scrollbar regions (IM-I p. I-327):                       */
    inUpButton   = 20, /* inside the up arrow button (top of vertical scroll)   */
    inDownButton = 21, /* inside the down arrow button (bottom of vertical)     */
    inPageUp     = 22, /* inside the page-up region (track above thumb)         */
    inPageDown   = 23, /* inside the page-down region (track below thumb)       */
    inThumb      = 129 /* inside the scrollbar thumb (MTE p. 5-63; IM-I p. I-327) */
};

/* ===========================================================================
 * 3. ControlRecord  (verbatim IM-I field names)
 * ---------------------------------------------------------------------------
 * Ref: Inside Macintosh Vol I Ch 5 (IM-I) p. I-318 "The ControlRecord Data
 *   Structure"; MTE Ch 5 p. 5-51 "Data Structures."
 *
 * FLAIR DEPARTURES from IM-I (Law 3 justification; noted verbatim):
 *   - contrlOwner: omitted (FLAIR is cooperative/single-threaded; no
 *     per-window port-ownership concept applies, ADR-0004 D-6).
 *   - contrlAction: omitted (TrackControl takes an action function as a
 *     parameter; the field is unnecessary in the caller-supplied-storage model).
 *   - contrlData / contrlHand: omitted (no heap handle table, ADR-0004 DEC-03).
 *   - contrlTitle: stored as a const char * (freestanding NUL-terminated string,
 *     not a Pascal string -- FLAIR uses C strings throughout, Law 3).
 *   - Region handles (contrlStrucRgn etc.): not in the IM-I ControlRecord;
 *     clipping is through the GrafPort visRgn/clipRgn (ADR-0004 D-1/D-2).
 * ===========================================================================*/

/* Maximum title length for a control (push-button label, etc.). Compile-time
 * bound so ControlRecord carries no heap pointer and is self-contained. */
#define CTRL_TITLE_MAX 64

typedef struct ControlRecord {
    /* contrlRect: the control's bounding rectangle in port-local coordinates.
     * Ref: IM-I p. I-318 "contrlRect: Rect. The control rectangle." */
    rgn_rect_t         contrlRect;

    /* contrlValue: the current value. For scrollBar, value in [min,max].
     * For checkBox/radioButton: 0=unchecked, 1=checked. For progressBar:
     * the current fill level. Ref: IM-I p. I-318 "contrlValue: Integer." */
    int16_t            contrlValue;

    /* contrlMin: the minimum legal value (inclusive).
     * Ref: IM-I p. I-318 "contrlMin: Integer." */
    int16_t            contrlMin;

    /* contrlMax: the maximum legal value (inclusive).
     * Ref: IM-I p. I-318 "contrlMax: Integer." */
    int16_t            contrlMax;

    /* contrlHilite: the highlighted (pressed) part code, or 0 if none.
     * 255 = the control is inactive/disabled (grayed). Ref: IM-I p. I-318
     * "contrlHilite: Integer. The highlighted part of the control, or 0
     * if no part is highlighted." */
    int16_t            contrlHilite;

    /* contrlVis: non-zero if the control is visible.
     * Ref: IM-I p. I-318 "contrlVis: Integer." */
    int16_t            contrlVis;

    /* contrlType: the control variant (pushButton, checkBox, etc.).
     * Not a verbatim IM-I field name but required for dispatch. */
    flair_ctrl_type_t  contrlType;

    /* contrlTitle: the control label (NUL-terminated). For push buttons the
     * label is centered in the rect via text_center_in. For check/radio it
     * appears to the right of the box/dot. Ref: IM-I p. I-318 "title." */
    char               contrlTitle[CTRL_TITLE_MAX];

    /* contrlRfCon: the application-specific reference constant (IM-I p. I-318
     * "contrlRfCon: Long. A reference constant for the application."). */
    int32_t            contrlRfCon;
} ControlRecord;

/* ===========================================================================
 * 4. SCROLLBAR LAYOUT CONSTANTS  (derived from chrome_metrics.h)
 * ---------------------------------------------------------------------------
 * Ref: FLAIR_CHROME_SCROLLBAR_W = 16 (spec/chrome_metrics.h; WDEF scrollBarSize
 *   EQU 16; StandardWDEF.a). Arrow buttons are square: SB_ARROW x SB_ARROW.
 *   Minimum thumb height (period-authentic System-7 thumb floor): 16 px.
 * ===========================================================================*/
#define SB_ARROW      FLAIR_CHROME_SCROLLBAR_W  /* 16 px; square arrow buttons  */
#define SB_THUMB_MIN  16                         /* minimum thumb height (px)    */

/* ===========================================================================
 * 5. CONTROL MANAGER API (verbatim Inside Macintosh routine names; MTE Ch 5)
 * ===========================================================================*/

/* --------------------------------------------------------------------------
 * control_init -- initialize a ControlRecord with type, rect, value/min/max,
 * visibility, and title. Sets contrlHilite to 0 (no hilite) and contrlRfCon
 * to 0. Does NOT perform any allocation.
 *
 * Ref: IM-I Ch 5 "NewControl" semantics, adapted for caller-supplied storage.
 * -------------------------------------------------------------------------- */
void control_init(ControlRecord *ctrl,
                  flair_ctrl_type_t type,
                  rgn_rect_t rect,
                  int16_t value, int16_t min_val, int16_t max_val,
                  int16_t vis,
                  const char *title);

/* --------------------------------------------------------------------------
 * SetControlValue -- set contrlValue, clamped to [contrlMin, contrlMax].
 *
 * Ref: IM-I p. I-323 "SetCtlValue"; MTE Ch 5 "SetControlValue."
 * Fail-loud (Rule 2): panics (via flair_panic or a DEBUG_ASSERT) if ctrl
 * is NULL. Out-of-range values are SILENTLY CLAMPED (Inside Macintosh
 * SetControlValue semantics -- clamp is the defined behavior).
 * -------------------------------------------------------------------------- */
void SetControlValue(ControlRecord *ctrl, int16_t value);

/* --------------------------------------------------------------------------
 * GetControlValue -- return contrlValue.
 *
 * Ref: IM-I p. I-323 "GetCtlValue"; MTE Ch 5 "GetControlValue."
 * -------------------------------------------------------------------------- */
int16_t GetControlValue(const ControlRecord *ctrl);

/* --------------------------------------------------------------------------
 * DrawControl -- draw the control into the GrafPort `port`.
 *
 * Dispatches by contrlType to the appropriate draw function. Writing goes
 * only through the surface module + blitter, clipped to visRgn INTERSECT
 * clipRgn (ADR-0004 D-1/D-2). If contrlVis == 0, this is a no-op.
 *
 * Ref: IM-I p. I-321 "DrawControls"; MTE Ch 5 "DrawControl."
 * -------------------------------------------------------------------------- */
void DrawControl(GrafPort *port, ControlRecord *ctrl);

/* --------------------------------------------------------------------------
 * TestControl -- hit-test a point against a control; return the part code.
 *
 * Returns the part code (inButton, inUpButton, inDownButton, inPageUp,
 * inPageDown, inThumb, inCheckBox) if `pt` is inside the corresponding
 * part of the control, or 0 if the point misses the control entirely or the
 * control is invisible.
 *
 * Ref: IM-I p. I-329 "TestControl"; MTE Ch 5 "TestControl."
 * -------------------------------------------------------------------------- */
int16_t TestControl(const ControlRecord *ctrl, flair_point_t pt);

/* --------------------------------------------------------------------------
 * TrackControl -- follow a supplied sequence of points, hiliting the pressed
 * part, and return the final part code (0 if the sequence ends outside).
 *
 * `pts`   -- array of flair_point_t positions (the tracking sequence; the
 *            first point is the initial click, subsequent are move deltas).
 * `n_pts` -- length of the `pts` array.
 *
 * The function:
 *   1. Hits-tests pts[0] to determine the initial part (inButton / inThumb /
 *      inUpButton / inDownButton / inPageUp / inPageDown / inCheckBox).
 *   2. Sets contrlHilite to the pressed part code.
 *   3. For each subsequent point, if inside the SAME original part, keeps
 *      hilite; if outside, clears hilite (but continues tracking the sequence
 *      -- this is the standard Mac "autoscroll tracking" discipline: the
 *      action fires only while the mouse is within the part).
 *   4. For a thumb drag (inThumb): updates contrlValue proportionally as
 *      the point moves vertically within the track region. The value is
 *      clamped to [contrlMin, contrlMax] at each step.
 *   5. Returns the part code if the LAST point in the sequence is inside
 *      the initial part, or 0 if it is outside (released outside = no action).
 *      After return, contrlHilite is restored to 0.
 *
 * Deterministic (Rule 11): the tracking state is a pure function of the
 * supplied point sequence (no mouse hardware, no ISR, no PIT tick). This
 * makes the TrackControl behavior host-testable and oracle-provable.
 *
 * Ref: IM-I p. I-329 "TrackControl"; MTE Ch 5 "TrackControl."
 * -------------------------------------------------------------------------- */
int16_t TrackControl(ControlRecord *ctrl, const flair_point_t *pts, uint32_t n_pts);

/* --------------------------------------------------------------------------
 * ctrl_thumb_y -- compute the thumb top pixel (port-local, absolute) from
 * the current contrlValue.
 *
 * This is the forward math shared by DrawControl (scrollbar draw) and
 * TrackControl (thumb drag update). Extracted as a non-static function so
 * the oracle can assert it directly and verify INVERTIBILITY.
 *
 * Returns the absolute y coordinate of the thumb top edge (in port-local
 * coordinates, i.e. relative to the top of the port, matching contrlRect).
 *
 * Ref: Inside Macintosh IV "Calculating Scroll Bar Thumb Position" (the
 *   proportional thumb derivation); CLAUDE.md Law 1 (cited in header).
 * -------------------------------------------------------------------------- */
int16_t ctrl_thumb_y(const ControlRecord *ctrl);

/* --------------------------------------------------------------------------
 * ctrl_value_from_thumb_y -- inverse of ctrl_thumb_y.
 *
 * Given a thumb top pixel `thumb_y` (absolute, port-local), returns the
 * contrlValue that would produce that thumb position. Used by TrackControl
 * during thumb drag to set contrlValue proportionally.
 *
 * Result is clamped to [contrlMin, contrlMax].
 * -------------------------------------------------------------------------- */
int16_t ctrl_value_from_thumb_y(const ControlRecord *ctrl, int16_t thumb_y);

/* --------------------------------------------------------------------------
 * ctrl_progress_fill_px -- compute the filled pixel width for a progress bar.
 *
 * filled_px = inner_w * contrlValue / contrlMax  (integer division).
 * inner_w = contrlRect.right - contrlRect.left - 2  (1 px border each side).
 *
 * Returns 0 if contrlMax == 0 (degenerate; no divide-by-zero).
 * At contrlValue == contrlMax: returns inner_w (full fill).
 * -------------------------------------------------------------------------- */
int16_t ctrl_progress_fill_px(const ControlRecord *ctrl);

#endif /* INITECH_OS_FLAIR_CONTROL_H */
