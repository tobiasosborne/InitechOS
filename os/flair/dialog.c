/*
 * os/flair/dialog.c -- FLAIR Dialog Manager (THE ARTIFACT).
 *
 * beads: (Dialog Manager: ModalDialog, FILE COPY box, item lists)
 *
 * ERA AXIS: Mac OS 8 Platinum (DEC-10) is the FLAIR BASE. The movable modal
 * delegates its title band to the Platinum chrome mechanism; retained dialog
 * manager semantics and dBoxProc geometry remain explicitly heritage-tagged.
 *
 * Freestanding artifact code. Implements the Dialog Manager (FLAIR Layer 3,
 * ADR-0004 D-3): the modal FILE COPY box (Law 4 canon, PRD Sec 6.5), item
 * list management, dBoxProc 7-px border frame drawing, ModalDialog cooperative
 * loop, and FindDialogItem hit-testing.
 *
 * FREESTANDING CONSTRAINTS (Law 3):
 *   No libc (no stdio, stdlib, string.h). No malloc. No host OS calls.
 *   The dual-compile trick: freestanding on kernel; hosted for the oracle.
 *   Only <stdint.h> + <stddef.h> + local locked headers.
 *
 * MUTATION SWITCHES (Rule 6 -- named mutants, oracle-proven):
 *   DIALOG_MUTATE_BORDER       -- DrawDialog uses wrong border width (8 px
 *                                 instead of 7); oracle must go RED.
 *   DIALOG_MUTATE_HIT_STATIC   -- FindDialogItem + ModalDialog return statText
 *                                 items as enabled; oracle must go RED.
 *   DIALOG_MUTATE_FILECOPY_MSG -- FileCopyDialog uses a different (wrong)
 *                                 message string; oracle must go RED (Law 4).
 *   DIALOG_MUTATE_TITLELESS_MODAL -- FileCopyDialog reverts to the OLD
 *                                 titleless dBoxProc chrome (thick 7px
 *                                 border, no title bar) instead of the
 *                                 moveable titled modal; oracle must go RED
 *                                 (Law 4, beads initech-zvo6).
 *   DIALOG_MUTATE_PROGRESS_ZERO -- FileCopyDialog reverts the progress bar's
 *                                 initial value to 0 (the old bug) instead of
 *                                 FLAIR_CANON_FILECOPY_PROGRESS; oracle must
 *                                 go RED (Law 4, beads initech-a90f).
 *   MODAL_PASSTHROUGH          -- dispatch classification lets outside clicks
 *                                 reach FindWindow behind the modal; oracle RED
 *                                 (bead initech-zn61).
 *
 * DRAW MODEL:
 *   dBoxProc dialogs (the NewDialog default): a solid FLAIR_CHROME_DIALOG_
 *   BORDER (7)-px filled band around the content rect (the outer ring).
 *   movableDBoxProc dialogs (the FILE COPY modal; beads initech-zvo6): a
 *   Platinum title band + a PLAIN 1-px frame (os/flair/chrome.h
 *   flair_draw_movable_dbox_chrome -- the SAME title-bar composer
 *   flair_draw_document_window uses; NOT a hand-rolled second chrome path).
 *   Inside either border, the sampled E7 content field carries a white TL /
 *   C0 BR inset bevel. Items are drawn in order: statText via text_draw
 *   (Chicago), ctrlItem via DrawControl. All drawing flows through the
 *   dialog's GrafPort and is clipped to visRgn INTERSECT clipRgn (ADR-0004
 *   D-1/D-2 invariant).
 *
 * MODALDIALOG LOOP MODEL (ADR-0004 D-6 cooperative):
 *   Drains flair_raw_ring via WaitNextEvent in task context. Returns itemHit
 *   on: mouseDown in enabled non-statText item (after TrackControl), Return
 *   -> defaultItem, Escape -> cancelItem, filterProc returning non-zero.
 *   On sleepTicks==0 with empty ring returns itemHit=0 immediately.
 *
 * Ref: dialog.h (the full contract, Law 1 citations, FLAIR_CANON_FILECOPY_MSG).
 *      spec/chrome_metrics.h (FLAIR_CHROME_DIALOG_BORDER = 7).
 *      os/flair/text.h (text_draw, FONT_CHICAGO).
 *      os/flair/control.h (DrawControl, TrackControl).
 *      os/flair/blitter.h (blitter_fill_rect_clipped).
 *      os/flair/surface.h (surface_fill_span, bitmap_t).
 *      os/flair/event.h (WaitNextEvent, EventRecord, flair_raw_ring_t).
 *      spec/event_model.h (mouseDown, keyDown, updateEvt, etc.).
 *      CLAUDE.md Law 1/2/3/4, Rule 2/6/11/12.
 */
#include <stdint.h>
#include <stddef.h>

#include "dialog.h"
#include "surface.h"          /* surface_fill_span (-Ios/flair)               */
#include "blitter.h"          /* blitter_fill_rect_clipped (-Ios/flair)       */
#include "text.h"             /* text_draw / text_measure / FONT_CHICAGO       */
#include "chrome.h"           /* flair_draw_movable_dbox_chrome (-Ios/flair)   */
#include "chrome_metrics.h"   /* FLAIR_CHROME_DIALOG_BORDER (-Ispec)           */
#include "region_algebra.h"   /* region_contains_point (-Ispec)               */
#include "event_model.h"      /* flair_event_what_t, EventRecord (-Ispec)      */
#include "flair_look.h"       /* flair_look_pixel + FLAIR_PART_* (the seam)    */

/* ---------------------------------------------------------------------------
 * FAIL-LOUD (Rule 2): dual panic (kernel infinite loop / hosted abort).
 * Mirrors window.c WIN_PANIC and control.c pattern exactly.
 * ------------------------------------------------------------------------- */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1 && !defined(__KERNEL_FREESTANDING__)
#  include <stdlib.h>   /* abort -- hosted only */
#  define DIALOG_PANIC(msg)  abort()
#else
#  define DIALOG_PANIC(msg)  do { for (;;) { } } while (0)
#endif

/* ---------------------------------------------------------------------------
 * PART NAMESPACE (C-8; the wctb-keyed roles the dialog draw names -- NOT colors)
 * Local aliases onto FLAIR_PART_* so the dialog frame/items read naturally.
 * dialog.c names a PART and resolves PART -> destination pixel through the ONE
 * policy seam flair_look_pixel.  No index, no color (the C-8 cut-line).
 * ------------------------------------------------------------------------- */
enum {
    DLG_BLACK     = FLAIR_PART_FRAME,    /* frame lines / borders               */
    DLG_WHITE     = FLAIR_PART_CONTENT,  /* sampled white highlight             */
    DLG_FACE      = FLAIR_PART_PLAT_FACE,/* sampled Platinum E7 content face     */
    DLG_SHADOW    = FLAIR_PART_PLAT_WELL,/* sampled Platinum C0 inset shadow     */
    DLG_DESKTOP   = FLAIR_PART_DESKTOP,  /* desktop background                  */
    DLG_TEXT_INK  = FLAIR_PART_TEXT      /* text / static text foreground       */
};

/* ---------------------------------------------------------------------------
 * PIXEL VALUE HELPER (C-8 policy seam; the ONE PART->pixel resolution)
 * ------------------------------------------------------------------------- */
/* dlg_px resolves a wctb-keyed PART to the destination pixel for this port's
 * depth via the ONE policy seam flair_look_pixel (os/flair/flair_look.h).
 * dialog.c names NO color and carries NO index->RGB switch (C-8). */
static uint32_t dlg_px(const GrafPort *port, int part)
{
    return flair_look_pixel(port, part);
}

/* ---------------------------------------------------------------------------
 * CLIPPING HELPER (verbatim pattern from control.c ctrl_in_clip)
 * ADR-0004 D-1/D-2: ALL drawing clipped by visRgn INTERSECT clipRgn.
 * ------------------------------------------------------------------------- */
static int dlg_in_clip(const GrafPort *port, int x, int y)
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

/* cfill_dlg -- fill [x, x+w) on row y with PART `part`, clipped.
 * Mirrors cfill_ctrl in control.c: batches maximal in-clip runs into
 * surface_fill_span.  PART -> pixel resolution is the ONE policy seam
 * (dlg_px -> flair_look_pixel; C-8): dialog.c names a PART, never a color. */
static void cfill_dlg(GrafPort *port, int x, int y, int w, int part)
{
    if (w <= 0) {
        return;
    }
    uint32_t px = dlg_px(port, part);
    int run_start = -1;
    for (int i = 0; i <= w; i++) {
        int cx = x + i;
        int in = (i < w) ? dlg_in_clip(port, cx, y) : 0;
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

/* crect_dlg -- fill solid rectangle [x0,x1) x [y0,y1) with PART `part`. */
static void crect_dlg(GrafPort *port, int x0, int y0, int x1, int y1, int part)
{
    for (int y = y0; y < y1; y++) {
        cfill_dlg(port, x0, y, x1 - x0, part);
    }
}

/* draw_dialog_content -- sampled Platinum content field.
 *
 * The body is E7 with a one-pixel inset bevel: white top/left, C0
 * bottom/right. The surrounding dBox/movable frame supplies the black
 * bounding line. Ref: ../system7-decomp/specs/sys8/controls.md Sec 1. */
static void draw_dialog_content(GrafPort *port,
                                int x0, int y0, int x1, int y1)
{
    if (x1 <= x0 || y1 <= y0) {
        return;
    }
    crect_dlg(port, x0, y0, x1, y1, DLG_FACE);
    cfill_dlg(port, x0, y0, x1 - x0, DLG_WHITE);
    for (int y = y0; y < y1; y++) {
        cfill_dlg(port, x0, y, 1, DLG_WHITE);
    }
    if (y1 - y0 > 1) {
        cfill_dlg(port, x0 + 1, y1 - 1, x1 - x0 - 1, DLG_SHADOW);
    }
    if (x1 - x0 > 1) {
        for (int y = y0 + 1; y < y1; y++) {
            cfill_dlg(port, x1 - 1, y, 1, DLG_SHADOW);
        }
    }
}

/* ===========================================================================
 * FREESTANDING STRING HELPERS (no libc; mirror control.c pattern)
 * ===========================================================================*/
static uint32_t flair_strlen(const char *s)
{
    if (!s) {
        return 0;
    }
    uint32_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

static void flair_strlcpy_dlg(char *dst, const char *src, uint32_t dsz)
{
    if (dsz == 0 || !dst) {
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

/* flair_strcmp: reserved for future use (editText comparison, M5+). */
static int flair_strcmp(const char *a, const char *b)
    __attribute__((unused));
static int flair_strcmp(const char *a, const char *b)
{
    if (!a || !b) {
        return (a == b) ? 0 : (a ? 1 : -1);
    }
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

/* ===========================================================================
 * HELPER: zero-fill a DialogRecord (no memset; freestanding)
 * ===========================================================================*/
static void dialog_zero(DialogRecord *dp)
    __attribute__((unused));
static void dialog_zero(DialogRecord *dp)
{
    uint8_t *p = (uint8_t *)dp;
    uint32_t n = (uint32_t)sizeof(DialogRecord);
    for (uint32_t i = 0; i < n; i++) {
        p[i] = 0;
    }
}

/* ===========================================================================
 * NewDialog / GetNewDialog
 * ===========================================================================*/
DialogPtr NewDialog(DialogRecord   *storage,
                    rgn_rect_t      bounds,
                    const char     *title,
                    DialogItem     *items,
                    uint16_t        itemCount,
                    uint16_t        defaultItem,
                    uint16_t        cancelItem,
                    WindowMgr      *wm,
                    region_t       *strucRgn,
                    region_t       *contRgn,
                    region_t       *updateRgn)
{
    if (!storage || !items || itemCount == 0 || !strucRgn || !contRgn || !updateRgn) {
        DIALOG_PANIC("NewDialog: NULL required argument (Rule 2)");
        return 0;
    }

    /* Zero the record. */
    dialog_zero(storage);

    /* Set up item list. */
    storage->items       = items;
    storage->itemCount   = itemCount;
    storage->defaultItem = defaultItem;
    storage->cancelItem  = cancelItem;
    storage->editField   = 0;
    storage->editBuf[0]  = '\0';

    /* Set up the embedded WindowRecord.
     * Ref: IM-I Ch 6 -- the dialog window is dBoxProc variant with dialogKind.
     *      spec/window_record.h: dBoxProc=1, dialogKind=2. */
    WindowRecord *wr = &storage->window;
    region_set_empty(strucRgn);
    region_set_empty(contRgn);
    region_set_empty(updateRgn);

    if (wm) {
        /* Register with the Window Manager (adds to z-order list, sets up
         * strucRgn/contRgn from bounds, hilited=1, visible=1). */
        NewWindow(wm, wr, bounds, bounds, (int16_t)dialogKind,
                  (int16_t)dBoxProc, 0 /* no goAway for dialog */);
    } else {
        /* Standalone dialog (not in the full z-order): manually initialize
         * the WindowRecord fields that NewWindow would set. */
        wr->strucRgn           = strucRgn;
        wr->contRgn            = contRgn;
        wr->updateRgn          = updateRgn;
        wr->windowKind         = (int16_t)dialogKind;
        wr->windowDefProcVariant = (int16_t)dBoxProc;
        wr->visible            = 1;
        wr->hilited            = 1;
        wr->goAwayFlag         = 0;
        wr->spareFlag          = 0;
        wr->nextWindow         = 0;
        wr->refCon             = 0;
        wr->titleWidth         = 0;

        /* Initialize regions to cover bounds. */
        region_set_rect(strucRgn, bounds);
        region_set_rect(contRgn,  bounds);
        region_set_empty(updateRgn);

        /* Set portRect to bounds. */
        wr->port.portRect = bounds;
    }

    /* The ONE kernel-held title seam (D2-3 / future FLAIR_SETWTITLE).  wm may
     * be NULL for the documented standalone Dialog Manager construction. */
    SetWTitle(wm, wr, title);

    return storage;
}

DialogPtr GetNewDialog(DialogRecord *storage,
                       rgn_rect_t    bounds,
                       const char   *title,
                       DialogItem   *items,
                       uint16_t      itemCount,
                       uint16_t      defaultItem,
                       uint16_t      cancelItem,
                       WindowMgr    *wm,
                       region_t     *strucRgn,
                       region_t     *contRgn,
                       region_t     *updateRgn)
{
    /* Ref: IM-I Ch 6 "GetNewDialog" -- uses the same initialization as
     * NewDialog; the resource-fork distinction is deferred (PRD Sec 6.4). */
    return NewDialog(storage, bounds, title, items, itemCount,
                     defaultItem, cancelItem, wm,
                     strucRgn, contRgn, updateRgn);
}

/* ===========================================================================
 * DisposeDialog
 * ===========================================================================*/
void DisposeDialog(DialogPtr dp, WindowMgr *wm)
{
    if (!dp) {
        DIALOG_PANIC("DisposeDialog: NULL DialogPtr (Rule 2)");
        return;
    }
    if (wm) {
        DisposeWindow(wm, &dp->window);
    }
    /* Zero the record to invalidate it (safe to reuse storage). */
    dialog_zero(dp);
}

/* ===========================================================================
 * DrawDialog
 *
 * Draws the dBoxProc 7-px solid border frame (FLAIR_CHROME_DIALOG_BORDER)
 * and then each dialog item.
 *
 * BORDER IMPLEMENTATION:
 *   The dBoxProc border is a solid filled band around the outer struct region.
 *   Per Inside Macintosh WDEF: "dBoxProc draws a thick border (7 pixels wide)
 *   around the dialog box" (IM-I Ch 6 / StandardWDEF.a dBoxBorderSize EQU 7).
 *   The border is drawn as FLAIR_CHROME_DIALOG_BORDER solid black filled
 *   bands on all four sides of the dialog's bounding rect.
 *
 * MUTATION:
 *   DIALOG_MUTATE_BORDER: uses border width 8 instead of 7.
 * ===========================================================================*/

#if defined(DIALOG_MUTATE_BORDER) && DIALOG_MUTATE_BORDER
/* NAMED MUTANT: wrong border width -> oracle must go RED (Rule 6). */
#  define DLG_BORDER_W  8
#else
#  define DLG_BORDER_W  FLAIR_CHROME_DIALOG_BORDER  /* == 7 */
#endif

void DrawDialog(DialogPtr dp)
{
    if (!dp) {
        DIALOG_PANIC("DrawDialog: NULL DialogPtr (Rule 2)");
        return;
    }

    GrafPort *port = &dp->window.port;
    rgn_rect_t bounds = port->portRect;

    int left   = (int)bounds.left;
    int top    = (int)bounds.top;
    int right  = (int)bounds.right;
    int bottom = (int)bounds.bottom;

    if (dp->window.windowDefProcVariant == (int16_t)movableDBoxProc) {
        /* --- movableDBoxProc: the moveable titled modal (beads initech-zvo6).
         * A pinstripe title bar + a PLAIN 1-px frame, via chrome.c's SHARED
         * title-bar composer (the same one flair_draw_document_window uses --
         * "do not hand-roll new chrome"). Content fill FIRST (below the title
         * band, inside the 1px frame), THEN the chrome draw (band + outer
         * frame) on top -- mirrors the dBoxProc division of labor below
         * (fill content, then draw the border). */
        int fr = FLAIR_CHROME_FRAME;                      /* 1 px            */
        int content_top = top + FLAIR_CHROME_TITLEBAR_H;  /* locked 22px band */

        draw_dialog_content(port, left + fr, content_top,
                            right - fr, bottom - fr);
        flair_draw_movable_dbox_chrome(port, flair_look_default_skin(),
                                       bounds, dp->window.titleHandle);
    } else {
        /* --- dBoxProc (the NewDialog default; every other/generic dialog):
         * the classic solid FLAIR_CHROME_DIALOG_BORDER (7)-px border frame.
         * Ref: WDEF dBoxBorderSize EQU 7; StandardWDEF.a; chrome_metrics.h. */
        int bw = DLG_BORDER_W;

        /* Fill sampled Platinum content + its one-pixel inset bevel. */
        draw_dialog_content(port, left + bw, top + bw,
                            right - bw, bottom - bw);

        /* Draw the dBoxProc 7-px border frame (solid black bands).
         * Top band: [left, right) x [top, top+bw)
         * Bottom band: [left, right) x [bottom-bw, bottom)
         * Left band:  [left, left+bw) x [top, bottom)
         * Right band: [right-bw, right) x [top, bottom) */
        crect_dlg(port, left,       top,        right,       top + bw,    DLG_BLACK);
        crect_dlg(port, left,       bottom - bw, right,       bottom,      DLG_BLACK);
        crect_dlg(port, left,       top,        left + bw,   bottom,      DLG_BLACK);
        crect_dlg(port, right - bw, top,        right,       bottom,      DLG_BLACK);
    }

    /* --- Draw each item. */
    uint16_t n = dp->itemCount;
    for (uint16_t i = 0; i < n; i++) {
        DialogItem *item = &dp->items[i];

        switch (item->type) {
        case statText: {
            /* Static text: draw with Chicago font (systemFont; ADR-0004 D-7).
             * Ref: IM-I Ch 6; ADR-0004 D-7 "Chicago (system/dialog)". */
            if (item->text) {
                text_draw(&port->portBits.bm,
                          (int)item->rect.left,
                          (int)item->rect.top,
                          item->text,
                          FONT_CHICAGO,
                          dlg_px(port, DLG_TEXT_INK),
                          dlg_px(port, DLG_FACE));
            }
            break;
        }
        case editText: {
            /* Editable text: draw current editBuf content.
             * Ref: IM-I Ch 6 "editText" items. */
            const char *txt = (dp->editField == (uint16_t)(i + 1))
                              ? dp->editBuf : item->text;
            if (txt) {
                text_draw(&port->portBits.bm,
                          (int)item->rect.left,
                          (int)item->rect.top,
                          txt,
                          FONT_CHICAGO,
                          dlg_px(port, DLG_TEXT_INK),
                          dlg_px(port, DLG_FACE));
            }
            break;
        }
        case ctrlItem: {
            /* Control item: delegate to DrawControl.
             * Ref: IM-I Ch 6 "ctrlItem"; MTE Ch 6. */
            if (item->ctrl) {
                if ((uint16_t)(i + 1) == dp->defaultItem &&
                    item->ctrl->contrlType == pushButton) {
                    DrawControlDefaultRing(port, item->ctrl);
                }
                DrawControl(port, item->ctrl);
            }
            break;
        }
        case userItem:
        case iconItem:
        default:
            /* User/icon items: caller handles; no-op here. */
            break;
        }
    }
}

/* ===========================================================================
 * ShowDialog
 * ===========================================================================*/
void ShowDialog(DialogPtr dp)
{
    if (!dp) {
        DIALOG_PANIC("ShowDialog: NULL DialogPtr (Rule 2)");
        return;
    }
    dp->window.visible = 1;
}

/* ===========================================================================
 * TRUE-MODAL DISPATCH SEAM (bead initech-zn61).
 *
 * This classification lives ABOVE FindWindow. FindWindow must keep returning
 * the true geometric hit for non-dispatch callers, while the event pump must
 * never let a frontmost modal click/key reach a window behind it. Period dBox
 * click-outside feedback was SysBeep; the kernel has no beep, so the caller's
 * blockProc emits the required FLAIR-MODAL-BLOCK marker instead.
 * ===========================================================================*/
flair_modal_disposition_t DialogModalDispatch(
    const DialogPtr dp, const EventRecord *ev,
    dialog_modal_block_fn blockProc, void *blockUser)
{
    if (dp == (DialogPtr)0 || ev == (const EventRecord *)0 ||
        !dp->window.visible)
        return FLAIR_MODAL_PASSTHROUGH;

#if defined(MODAL_PASSTHROUGH)
    /* Named Rule-6 mutant: restore the pre-fix pump behavior. An outside click
     * reaches FindWindow and raises/drags the document behind the modal. */
    (void)blockProc;
    (void)blockUser;
    return FLAIR_MODAL_PASSTHROUGH;
#else
    if (ev->what == (uint16_t)mouseDown ||
        ev->what == (uint16_t)mouseUp) {
        if (!region_contains_point(dp->window.strucRgn,
                                   ev->where.h, ev->where.v)) {
            if (ev->what == (uint16_t)mouseDown && blockProc != 0)
                blockProc(ev->where, blockUser);
            return FLAIR_MODAL_BLOCK;
        }

        if (ev->what == (uint16_t)mouseDown &&
            dp->window.windowDefProcVariant == (int16_t)movableDBoxProc) {
            rgn_rect_t bounds = region_get_bbox(dp->window.strucRgn);
            if (ev->where.v >= bounds.top &&
                ev->where.v < (int16_t)(bounds.top +
                                         FLAIR_CHROME_TITLEBAR_H))
                return FLAIR_MODAL_DRAG;
        }
        return FLAIR_MODAL_CAPTURE;
    }

    /* keyDown/autoKey and every other cooked event stay on the modal side of
     * the seam. In particular, no key can reach the active document behind it. */
    return FLAIR_MODAL_CAPTURE;
#endif
}

static rgn_rect_t dialog_offset_rect(rgn_rect_t r, int16_t dh, int16_t dv)
{
    int32_t left = (int32_t)r.left + dh;
    int32_t right = (int32_t)r.right + dh;
    int32_t top = (int32_t)r.top + dv;
    int32_t bottom = (int32_t)r.bottom + dv;
    if (left < INT16_MIN || left > INT16_MAX ||
        right < INT16_MIN || right > INT16_MAX ||
        top < INT16_MIN || top > INT16_MAX ||
        bottom < INT16_MIN || bottom > INT16_MAX)
        DIALOG_PANIC("MoveDialog: coordinate overflow");
    r.left = (int16_t)left;
    r.right = (int16_t)right;
    r.top = (int16_t)top;
    r.bottom = (int16_t)bottom;
    return r;
}

void MoveDialog(DialogPtr dp, int16_t dh, int16_t dv)
{
    rgn_rect_t struc;
    rgn_rect_t cont;
    rgn_rect_t update;

    if (dp == (DialogPtr)0) {
        DIALOG_PANIC("MoveDialog: NULL DialogPtr");
        return;
    }
    if (dh == 0 && dv == 0) return;

    struc = dialog_offset_rect(region_get_bbox(dp->window.strucRgn), dh, dv);
    cont = dialog_offset_rect(region_get_bbox(dp->window.contRgn), dh, dv);
    region_set_rect(dp->window.strucRgn, struc);
    region_set_rect(dp->window.contRgn, cont);
    if (!region_is_empty(dp->window.updateRgn)) {
        update = dialog_offset_rect(region_get_bbox(dp->window.updateRgn), dh, dv);
        region_set_rect(dp->window.updateRgn, update);
    }
    dp->window.port.portRect = dialog_offset_rect(dp->window.port.portRect,
                                                   dh, dv);

    for (uint16_t i = 0; i < dp->itemCount; i++) {
        dp->items[i].rect = dialog_offset_rect(dp->items[i].rect, dh, dv);
        if (dp->items[i].ctrl != (ControlRecord *)0)
            dp->items[i].ctrl->contrlRect = dialog_offset_rect(
                dp->items[i].ctrl->contrlRect, dh, dv);
    }
}

/* ===========================================================================
 * FindDialogItem
 *
 * Ref: IM-I Ch 6 "FindDItem" (p. I-419); MTE Ch 6 "FindDialogItem."
 * QuickDraw half-open rect: left <= pt.h < right AND top <= pt.v < bottom.
 *
 * MUTATION:
 *   DIALOG_MUTATE_HIT_STATIC: returns statText items as if enabled, so
 *   ModalDialog would return them on click. Oracle must go RED.
 * ===========================================================================*/
uint16_t FindDialogItem(const DialogPtr dp, flair_point_t pt)
{
    if (!dp) {
        return 0;
    }
    for (uint16_t i = 0; i < dp->itemCount; i++) {
        const DialogItem *item = &dp->items[i];
        if (pt.h >= item->rect.left && pt.h < item->rect.right &&
            pt.v >= item->rect.top  && pt.v < item->rect.bottom) {
            return (uint16_t)(i + 1);  /* 1-based */
        }
    }
    return 0;
}

/* ===========================================================================
 * GetDialogItem / SetDialogItem
 * ===========================================================================*/
void GetDialogItem(const DialogPtr            dp,
                   uint16_t                   itemIndex,
                   flair_dialog_item_type_t  *type,
                   rgn_rect_t                *rect)
{
    if (!dp || itemIndex == 0 || itemIndex > dp->itemCount) {
        DIALOG_PANIC("GetDialogItem: NULL dp or out-of-range itemIndex (Rule 2)");
        return;
    }
    const DialogItem *item = &dp->items[itemIndex - 1];
    if (type) {
        *type = item->type;
    }
    if (rect) {
        *rect = item->rect;
    }
}

void SetDialogItem(DialogPtr                 dp,
                   uint16_t                  itemIndex,
                   flair_dialog_item_type_t  type,
                   rgn_rect_t                rect,
                   const char               *text,
                   ControlRecord            *ctrl,
                   uint8_t                   enabled)
{
    if (!dp || itemIndex == 0 || itemIndex > dp->itemCount) {
        DIALOG_PANIC("SetDialogItem: NULL dp or out-of-range itemIndex (Rule 2)");
        return;
    }
    DialogItem *item = &dp->items[itemIndex - 1];
    item->type    = type;
    item->rect    = rect;
    item->text    = text;
    item->ctrl    = ctrl;
    item->enabled = enabled;
}

/* ===========================================================================
 * GetDialogItemText / SetDialogItemText
 * ===========================================================================*/
void GetDialogItemText(const DialogPtr dp, uint16_t itemIndex,
                       char *buf, uint32_t bufsz)
{
    if (!dp || itemIndex == 0 || itemIndex > dp->itemCount || !buf || bufsz == 0) {
        DIALOG_PANIC("GetDialogItemText: invalid argument (Rule 2)");
        return;
    }
    const DialogItem *item = &dp->items[itemIndex - 1];
    const char *src = item->text ? item->text : "";
    if (item->type == editText && dp->editField == itemIndex) {
        src = dp->editBuf;
    }
    flair_strlcpy_dlg(buf, src, bufsz);
}

void SetDialogItemText(DialogPtr dp, uint16_t itemIndex, const char *text)
{
    if (!dp || itemIndex == 0 || itemIndex > dp->itemCount) {
        DIALOG_PANIC("SetDialogItemText: invalid argument (Rule 2)");
        return;
    }
    DialogItem *item = &dp->items[itemIndex - 1];
    item->text = text;
    if (item->type == editText) {
        flair_strlcpy_dlg(dp->editBuf,
                          text ? text : "",
                          (uint32_t)DIALOG_ITEM_TEXT_MAX);
        dp->editField = itemIndex;
    }
}

/* Handle one event. Kept separate so the shell-owned live pump and the
 * cooperative ModalDialog loop share exactly one modal behavior path. */
int DialogHandleEvent(DialogPtr dp, EventRecord *ev,
                      dialog_filter_fn filterProc, uint16_t *itemHit)
{
    if (!dp || !ev || !itemHit) {
        DIALOG_PANIC("DialogHandleEvent: NULL required argument");
        return 0;
    }

    if (filterProc) {
        uint16_t fitem = 0;
        if (filterProc(dp, ev, &fitem)) {
            *itemHit = fitem;
            return 1;
        }
    }

    switch (ev->what) {
    case mouseDown: {
        flair_point_t pt = ev->where;
        pt.h -= dp->window.port.portRect.left;
        pt.v -= dp->window.port.portRect.top;
        uint16_t idx = FindDialogItem(dp, pt);
        if (idx == 0) break;
        DialogItem *item = &dp->items[idx - 1];
#if defined(DIALOG_MUTATE_HIT_STATIC) && DIALOG_MUTATE_HIT_STATIC
        if (item->enabled || item->type == statText) {
#else
        if (item->enabled && item->type != statText) {
#endif
            if (item->type == ctrlItem && item->ctrl) {
                int16_t part = TrackControl(item->ctrl, &pt, 1);
                if (part != 0) {
                    *itemHit = idx;
                    return 1;
                }
            } else {
                *itemHit = idx;
                return 1;
            }
        }
        break;
    }

    case keyDown:
    case autoKey: {
        uint8_t ascii = (uint8_t)(ev->message & 0xFFu);
        uint8_t vkey = (uint8_t)((ev->message >> 8u) & 0xFFu);
        int is_return = (ascii == 0x0Du || ascii == 0x0Au ||
                         ascii == 0x03u || vkey == 0x1Cu);
        int is_escape = (ascii == 0x1Bu || vkey == 0x01u);
        if (is_return && dp->defaultItem != 0) {
            *itemHit = dp->defaultItem;
            return 1;
        }
        if (is_escape && dp->cancelItem != 0) {
            *itemHit = dp->cancelItem;
            return 1;
        }
        break;
    }

    case updateEvt:
        DrawDialog(dp);
        break;

    case nullEvent:
    default:
        break;
    }
    return 0;
}

/* ===========================================================================
 * ModalDialog -- cooperative modal event loop.
 * Ref: IM-I Ch 6; ADR-0004 D-4/D-6. DialogHandleEvent is also the live pump's
 * captured-event path, so nested and inverted shell-owned modal delivery agree.
 * ===========================================================================*/
void ModalDialog(DialogPtr dp, flair_raw_ring_t *ring,
                 dialog_filter_fn filterProc, uint32_t sleepTicks,
                 uint16_t *itemHit)
{
    if (!dp || !ring || !itemHit) {
        DIALOG_PANIC("ModalDialog: NULL required argument (Rule 2)");
        return;
    }
    *itemHit = 0;

    for (;;) {
        EventRecord ev;
        int got = WaitNextEvent(ring, 0xFFFFu, &ev, sleepTicks);
        if (!got) {
            if (sleepTicks == 0) return;
            continue;
        }
        if (DialogHandleEvent(dp, &ev, filterProc, itemHit)) return;
    }
}

/* ===========================================================================
 * FileCopyDialog -- canonical FILE COPY modal dialog (Law 4, PRD Sec 6.5)
 *
 * Layout (canonical position for 640x480 desktop):
 *   bounds:      left=140, top=200, right=500, bottom=280  (360 x 80 px)
 *   Item 1 (statText): "Saving tables to disk..."
 *     rect: left=14, top=25, right=346, bottom=41  (below the title band)
 *   Item 2 (ctrlItem/progressBar): 0..100
 *     rect: left=14, top=49, right=346, bottom=69  (below the title band)
 *
 * Ref: PRD Sec 6.5 / Appendix A -- the canon FILE COPY box dimensions and
 *      message string from the Office Space reference frame (Law 4).
 *
 * MUTATION:
 *   DIALOG_MUTATE_FILECOPY_MSG: uses a WRONG message string (not the canon
 *   FLAIR_CANON_FILECOPY_MSG). Oracle must go RED (Law 4 enforced).
 * ===========================================================================*/

/* Canonical FILE COPY bounds (640x480 desktop; PRD Appendix A). */
#define FILECOPY_LEFT    140
#define FILECOPY_TOP     200
#define FILECOPY_RIGHT   500
#define FILECOPY_BOTTOM  280

/* Item rects in dialog-local coordinates (measured from dialog top-left).
 * FLAIR_CHROME_TITLEBAR_H (19 px; spec/chrome_metrics.h) precedes the content
 * area now that the modal is the moveable titled chrome (movableDBoxProc,
 * beads initech-zvo6) instead of the old dBoxProc 7-px border -- these
 * offsets were shifted down to clear the title band (were 12/28/36/56 under
 * the old 7px-border layout; the same 16px text-row height and 8px/20px
 * gaps/bar-height are preserved, just re-based off the taller title band). */
#define FILECOPY_TEXT_TOP     25
#define FILECOPY_TEXT_BOTTOM  41
#define FILECOPY_BAR_TOP      49
#define FILECOPY_BAR_BOTTOM   69
#define FILECOPY_ITEM_LEFT    14
#define FILECOPY_ITEM_RIGHT   346

DialogPtr FileCopyDialog(DialogRecord  *storage,
                         DialogItem    *itemStorage,
                         ControlRecord *ctrlStorage,
                         region_t      *strucRgn,
                         region_t      *contRgn,
                         region_t      *updateRgn)
{
    if (!storage || !itemStorage || !ctrlStorage ||
        !strucRgn || !contRgn || !updateRgn) {
        DIALOG_PANIC("FileCopyDialog: NULL required argument (Rule 2)");
        return 0;
    }

    /* --- Canonical FILE COPY message string.
     * Law 4 (PRD Sec 6.5 / Appendix A): MUST be byte-exact.
     * DIALOG_MUTATE_FILECOPY_MSG: substitutes a wrong string (oracle RED). */
#if defined(DIALOG_MUTATE_FILECOPY_MSG) && DIALOG_MUTATE_FILECOPY_MSG
    /* NAMED MUTANT: wrong message string -> oracle MUST go RED (Law 4). */
    static const char *canon_msg = "Saving tables to disk";   /* truncated! */
#else
    static const char *canon_msg = FLAIR_CANON_FILECOPY_MSG;
    /* Ref: PRD Sec 6.5 / Appendix A -- Office Space frame canon;
     *      do NOT paraphrase */
#endif

    /* --- Dialog bounds (global coords). */
    rgn_rect_t bounds;
    bounds.left   = (int16_t)FILECOPY_LEFT;
    bounds.top    = (int16_t)FILECOPY_TOP;
    bounds.right  = (int16_t)FILECOPY_RIGHT;
    bounds.bottom = (int16_t)FILECOPY_BOTTOM;

    /* --- Item 1: static text ("Saving tables to disk...").
     * enabled=0: statText items are display-only (never returned by ModalDialog).
     * Ref: IM-I Ch 6 "statText items cannot be enabled." */
    rgn_rect_t text_rect;
    text_rect.left   = (int16_t)(FILECOPY_LEFT + FILECOPY_ITEM_LEFT);
    text_rect.top    = (int16_t)(FILECOPY_TOP  + FILECOPY_TEXT_TOP);
    text_rect.right  = (int16_t)(FILECOPY_LEFT + FILECOPY_ITEM_RIGHT);
    text_rect.bottom = (int16_t)(FILECOPY_TOP  + FILECOPY_TEXT_BOTTOM);

    itemStorage[0].type    = statText;
    itemStorage[0].rect    = text_rect;
    itemStorage[0].text    = canon_msg;
    itemStorage[0].ctrl    = 0;
    itemStorage[0].enabled = 0;
    itemStorage[0]._pad[0] = 0;
    itemStorage[0]._pad[1] = 0;
    itemStorage[0]._pad[2] = 0;

    /* --- Item 2: progress bar control (the FILE COPY progress indicator).
     * Ref: ADR-0004 D-3 ("the FILE COPY progress bar"; control.h progressBar);
     *      PRD Sec 6.5. enabled=0: the progress bar is display-only. */
    rgn_rect_t bar_rect;
    bar_rect.left   = (int16_t)(FILECOPY_LEFT + FILECOPY_ITEM_LEFT);
    bar_rect.top    = (int16_t)(FILECOPY_TOP  + FILECOPY_BAR_TOP);
    bar_rect.right  = (int16_t)(FILECOPY_LEFT + FILECOPY_ITEM_RIGHT);
    bar_rect.bottom = (int16_t)(FILECOPY_TOP  + FILECOPY_BAR_BOTTOM);

    /* --- Canon progress bar initial value (beads initech-a90f, Law 4).
     * DIALOG_MUTATE_PROGRESS_ZERO: reverts to the old bug (value=0, which
     * ctrl_progress_fill_px renders as NO fill at all) -> oracle must go RED. */
#if defined(DIALOG_MUTATE_PROGRESS_ZERO) && DIALOG_MUTATE_PROGRESS_ZERO
    const int16_t canon_progress = 0;   /* NAMED MUTANT: the old bug */
#else
    /* Ref: dialog.h FLAIR_CANON_FILECOPY_PROGRESS (docs/FLAIR-GUI-bug-hunt-
     * 2026-06-28.md #25: "preview.png reference shows the bar ~65-70% filled
     * solid blue"; canon = the documented midpoint, 68). */
    const int16_t canon_progress = (int16_t)FLAIR_CANON_FILECOPY_PROGRESS;
#endif

    control_init(ctrlStorage,
                 progressBar,
                 bar_rect,
                 canon_progress /* initial value: FLAIR_CANON_FILECOPY_PROGRESS */,
                 0    /* min */,
                 100  /* max */,
                 1    /* vis */,
                 "");

    itemStorage[1].type    = ctrlItem;
    itemStorage[1].rect    = bar_rect;
    itemStorage[1].text    = 0;
    itemStorage[1].ctrl    = ctrlStorage;
    itemStorage[1].enabled = 0;
    itemStorage[1]._pad[0] = 0;
    itemStorage[1]._pad[1] = 0;
    itemStorage[1]._pad[2] = 0;

    /* --- The moveable titled modal window (beads initech-zvo6, Law 4).
     * DIALOG_MUTATE_TITLELESS_MODAL: reverts to the OLD titleless dBoxProc
     * chrome (empty title, NewDialog's default dBoxProc variant left as-is)
     * -> oracle must go RED. */
#if defined(DIALOG_MUTATE_TITLELESS_MODAL) && DIALOG_MUTATE_TITLELESS_MODAL
    static const char *canon_title = "";   /* NAMED MUTANT: the old bug */
#else
    /* Ref: dialog.h FLAIR_CANON_FILECOPY_TITLE ("FILE COPY"; PRD Appendix A). */
    static const char *canon_title = FLAIR_CANON_FILECOPY_TITLE;
#endif

    /* --- Initialize the DialogRecord (no WindowMgr for standalone). NewDialog
     * always sets windowDefProcVariant = dBoxProc (the correct default for
     * every OTHER/generic dialog); FileCopyDialog upgrades it below because
     * the FILE COPY modal specifically is the moveable-titled reference. */
    DialogPtr dp = NewDialog(storage, bounds,
                             canon_title,
                             itemStorage, 2u,
                             0u /* no default item */,
                             0u /* no cancel item */,
                             0  /* no WindowMgr */,
                             strucRgn, contRgn, updateRgn);

#if !(defined(DIALOG_MUTATE_TITLELESS_MODAL) && DIALOG_MUTATE_TITLELESS_MODAL)
    /* Upgrade to movableDBoxProc (5; MTE Table 4-1; ../system7-decomp/specs/
     * toolbox/window-manager.md line 173 "movable modal dialog box (title
     * bar, no grow/zoom)"). DrawDialog dispatches on this field to draw the
     * pinstripe title bar + plain 1px frame instead of the 7px dBoxProc
     * border (os/flair/chrome.h flair_draw_movable_dbox_chrome). */
    if (dp) {
        dp->window.windowDefProcVariant = (int16_t)movableDBoxProc;
    }
#endif

    return dp;
}

/* Suppress unused-function warnings for internal helpers (freestanding builds
 * may not exercise every path in a single TU; Rule 12 ASCII-clean). */
static void _dlg_suppress_unused(void)
    __attribute__((unused));
static void _dlg_suppress_unused(void)
{
    (void)flair_strlen;
    (void)flair_strcmp;
}
