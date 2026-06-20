/*
 * os/flair/dialog.h -- FLAIR Dialog Manager (THE ARTIFACT).
 *
 * beads: (Dialog Manager: ModalDialog, FILE COPY box, item lists)
 *
 * Ref: ADR-0004 D-3 ("Dialog Manager -- DialogRecord + item lists;
 *        ModalDialog; the modal FILE COPY box ('Saving tables to disk...',
 *        the comedic centerpiece, PRD Sec 6.5 / Appendix B).");
 *      Inside Macintosh: Macintosh Toolbox Essentials (MTE) Ch 6 "The
 *        Dialog Manager" -- the PRIMARY reference: DialogRecord (MTE p.
 *        6-71..6-75), dialog item constants, NewDialog, DisposeDialog,
 *        DrawDialog, ModalDialog, GetDialogItem, SetDialogItem,
 *        GetDialogItemText, SetDialogItemText (MTE Ch 6 Routines).
 *      Inside Macintosh Vol I Ch 6 (IM-I) "The Dialog Manager" -- the
 *        original verbatim record, item list, and ModalDialog filter proc.
 *      PRD Sec 6.5 (FILE COPY dialog, the canon "Saving tables to disk...");
 *      PRD Appendix A / Appendix B (the comedic Office Space frame canon).
 *      Law 4 (canon constants MUST NOT be paraphrased or corrected).
 *      spec/chrome_metrics.h (FLAIR_CHROME_DIALOG_BORDER = 7 px dBoxProc);
 *      spec/window_record.h (dBoxProc = 1, dialogKind = 2);
 *      os/flair/window.h (WindowRecord + WindowMgr);
 *      os/flair/control.h (ControlRecord, DrawControl, TrackControl);
 *      os/flair/text.h (text_draw, text_measure, FONT_CHICAGO);
 *      os/flair/blitter.h (blitter_fill_rect_clipped);
 *      os/flair/event.h (WaitNextEvent, EventRecord, flair_raw_ring_t);
 *      ADR-0004 D-6 (cooperative WaitNextEvent, non-preemptive).
 *      CLAUDE.md Law 1 (ground truth), Law 2 (oracle is truth), Law 3
 *        (freestanding dual-compile), Law 4 (canon must not be paraphrased),
 *        Rule 2 (fail loud), Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * ARTIFACT CODE: freestanding C (ADR-0002). No libc. No malloc. No 2026-isms.
 * Dual-compile: kernel (-m32 -ffreestanding -nostdlib -DFLAIR_HOSTED=0)
 *               hosted  (cc -DFLAIR_HOSTED=1) for the oracle harness.
 *
 * VERBATIM INSIDE MACINTOSH NAMES (ADR-0004 D-3):
 *   DialogRecord fields: window (embedded WindowRecord), items, numItems,
 *     defaultItem, cancelItem.
 *   Routines: NewDialog, GetNewDialog, DisposeDialog, DrawDialog,
 *     ShowDialog, ModalDialog, FindDialogItem, GetDialogItem, SetDialogItem,
 *     GetDialogItemText, SetDialogItemText.
 *
 * CALLER-SUPPLIED STORAGE (no malloc):
 *   DialogRecord, item array, and any ControlRecords are provided by the
 *   caller. The Dialog Manager performs no heap allocation.
 *
 * COOPERATIVE ModalDialog (ADR-0004 D-6):
 *   ModalDialog drains events via flair_raw_ring / WaitNextEvent in task
 *   context. It is a cooperative blocking loop: it holds the CPU until a
 *   dismissal event (enabled item click, Return -> default, Escape -> cancel).
 *   An app that enters ModalDialog without a dismissal event eventually
 *   queued will hold the CPU indefinitely -- that is period-authentic
 *   (ADR-0004 D-6 non-goal: "An app that doesn't yield hangs the desktop;
 *   that's authentic, not a bug").
 *   In the test harness the ring is pre-loaded with a deterministic event
 *   sequence and a sleepTicks of 0 is used (returns immediately on empty
 *   ring after draining all events).
 *
 * FILE COPY CANON (Law 4 -- PRD Sec 6.5 / Appendix A/B):
 *   The canonical message string is LOCKED below. It MUST NOT be paraphrased,
 *   corrected, or shortened. The test oracle asserts it byte-exactly.
 *
 * ASCII-clean (Rule 12). No nondeterminism / no timestamps (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_DIALOG_H
#define INITECH_OS_FLAIR_DIALOG_H

#include <stdint.h>
#include <stddef.h>

#include "grafport.h"         /* GrafPort, flair_point_t (-Ispec)            */
#include "region_algebra.h"   /* rgn_rect_t (-Ispec)                         */
#include "window_record.h"    /* WindowRecord, dBoxProc, dialogKind (-Ispec) */
#include "chrome_metrics.h"   /* FLAIR_CHROME_DIALOG_BORDER = 7 (-Ispec)     */
#include "window.h"           /* WindowMgr, NewWindow, DisposeWindow          */
#include "control.h"          /* ControlRecord, DrawControl, TrackControl     */
#include "event.h"            /* flair_raw_ring_t, EventRecord, WaitNextEvent */

/* ===========================================================================
 * 1. CANONICAL FILE COPY STRING (Law 4 -- PRD Sec 6.5 / Appendix A/B)
 * ---------------------------------------------------------------------------
 * This string is the comedic centerpiece of the Office Space reference frame.
 * It MUST be byte-exact. Do NOT paraphrase, correct, shorten, or modify.
 * The oracle (test_dialog.c DIALOG_MUTATE_FILECOPY_MSG) asserts it is present
 * in the rendered FILE COPY dialog item list.
 *
 * Ref: PRD Sec 6.5 / Appendix A -- "Saving tables to disk..." (canon).
 *      Law 4: "The canonical bugs are canon ... enforced, not fixed."
 * ===========================================================================*/
#define FLAIR_CANON_FILECOPY_MSG "Saving tables to disk..."
/* Ref: PRD Sec 6.5 / Appendix A -- Office Space frame canon; do NOT paraphrase */

/* ===========================================================================
 * 2. DIALOG ITEM TYPES  (verbatim IM-I / MTE names)
 * ---------------------------------------------------------------------------
 * Ref: Inside Macintosh Vol I Ch 6 (IM-I) p. I-408: "Item types."
 *      MTE Ch 6 p. 6-15 "Dialog Box Items."
 *
 * Values are verbatim from Inside Macintosh.
 * ===========================================================================*/
typedef enum flair_dialog_item_type {
    userItem   = 0,  /* user-drawn item; caller provides draw callback        */
    ctrlItem   = 4,  /* control item (button, checkbox, scrollbar, progress)  */
    iconItem   = 32, /* icon item (ICON / SICN -- reserved for M5+)           */
    statText   = 8,  /* static text item (non-interactive label)              */
    editText   = 16  /* editable text item (single-line; M5+ full support)    */
} flair_dialog_item_type_t;

/* ===========================================================================
 * 3. DialogItem -- one item in a dialog's item list
 * ---------------------------------------------------------------------------
 * Ref: IM-I Ch 6 "DITL -- Dialog Item List" (p. I-410);
 *      MTE Ch 6 p. 6-15 "Dialog Box Items."
 *
 * FLAIR carries items as an explicit struct array (caller-supplied), mirroring
 * the Mac OS DITL resource layout but in C-struct form (no resource fork yet;
 * PRD Sec 6.4 resource-fork analogue is deferred).
 *
 * For statText / editText: `text` holds the string (pointer to const char;
 *   the string is NOT copied; caller must keep it alive).
 * For ctrlItem: `ctrl` points to a caller-supplied ControlRecord.
 * For userItem / iconItem: both text and ctrl are ignored.
 * enabled=1: ModalDialog may return this item when clicked.
 * enabled=0: the item is drawn but never returned (e.g. static text labels).
 * ===========================================================================*/
#define DIALOG_ITEM_TEXT_MAX  128u  /* max inline text length for editText    */

typedef struct DialogItem {
    flair_dialog_item_type_t type;      /* item type (statText, ctrlItem ...) */
    rgn_rect_t               rect;      /* item bounding rect in dialog coords */
    const char              *text;      /* statText / editText string (NOT copied) */
    ControlRecord           *ctrl;      /* ctrlItem: pointer to ControlRecord */
    uint8_t                  enabled;   /* 1 = ModalDialog may return; 0 = display only */
    uint8_t                  _pad[3];   /* alignment pad; must be 0           */
} DialogItem;

/* ===========================================================================
 * 4. DialogRecord -- the dialog descriptor (verbatim field names)
 * ---------------------------------------------------------------------------
 * Ref: MTE Ch 6 p. 6-71 "The DialogRecord Data Structure";
 *      IM-I Ch 6 p. I-408 "Dialog Records."
 *
 * The verbatim IM-I Pascal DialogRecord:
 *   window        DialogWindow  ; the dialog's window (WindowRecord at offset 0)
 *   items         Handle        ; handle to item list (DITL resource)
 *   textH         TEHandle      ; TextEdit handle for editable text
 *   editField     Integer       ; index of the active editText item (or -1)
 *   editOpen      Integer       ; non-zero while editText is active
 *   aDefItem      Integer       ; default item number
 *
 * FLAIR DEPARTURES (Law 3 justification):
 *   - window: EMBEDDED WindowRecord (verbatim IM-I "the dialog's window
 *     record"), at offset 0 so DialogPtr casts to WindowPtr (= GrafPtr).
 *   - items: caller-supplied DialogItem * array + itemCount (no heap handle).
 *   - textH / editField / editOpen: reduced to a single editBuf for the
 *     active editText content (M3/M4 scope: single-line, no TextEdit).
 *   - aDefItem -> defaultItem (verbatim IM-I intent, FLAIR field name).
 *   - cancelItem: the item index that Escape triggers (FLAIR extension; not
 *     in IM-I but standard practice).
 *
 * INVARIANTS:
 *   - window is at offset 0; DialogPtr casts to WindowPtr casts to GrafPtr.
 *   - items is non-NULL for an initialized dialog; itemCount >= 1.
 *   - defaultItem is 1-based (IM-I convention: item numbers start at 1).
 *     0 means no default item.
 *   - cancelItem is 1-based; 0 means no cancel item.
 * ===========================================================================*/
typedef struct DialogRecord DialogRecord;

struct DialogRecord {
    /* --- Embedded WindowRecord (FIRST field; MUST be at offset 0) -----------
     * Ref: IM-I Ch 6: "The dialog's window record is the first field."
     * A DialogPtr (= DialogRecord *) casts to WindowPtr to GrafPtr.
     * The window's windowKind is set to dialogKind (2; spec/window_record.h).
     * The window's windowDefProcVariant is dBoxProc (1) for the 7px border. */
    WindowRecord  window;

    /* --- Item list (caller-supplied; no heap handle) -----------------------
     * Ref: IM-I Ch 6 "items: Handle. A handle to the dialog's item list."
     * FLAIR: a direct pointer to a caller-supplied DialogItem array.
     * itemCount: the number of elements in the items array. */
    DialogItem   *items;
    uint16_t      itemCount;

    /* --- Default item (IM-I "aDefItem") ------------------------------------
     * Ref: IM-I Ch 6 "aDefItem: Integer. The default item number."
     * 1-based index into items[]. Return key confirms the default item.
     * 0 means no default item. */
    uint16_t      defaultItem;

    /* --- Cancel item (FLAIR extension) -------------------------------------
     * The 1-based item index that the Escape key triggers. 0 = none. */
    uint16_t      cancelItem;

    /* --- Edit text buffer (M3/M4 editText; single-line inline) ------------
     * For the active editText item (if any). Stores the current content.
     * Null-terminated. Only one editText item may be active at a time. */
    char          editBuf[DIALOG_ITEM_TEXT_MAX];

    /* --- Active edit item index (1-based; 0 = no active editText) --------- */
    uint16_t      editField;

    uint8_t       _pad[2]; /* alignment */
};

/* DialogPtr -- pointer to a DialogRecord (verbatim QuickDraw name).
 * A DialogPtr is also a WindowPtr (window at offset 0) and a GrafPtr. */
typedef DialogRecord *DialogPtr;

/* ===========================================================================
 * 5. DIALOG MANAGER API (verbatim Inside Macintosh routine names; MTE Ch 6)
 * ===========================================================================*/

/* --------------------------------------------------------------------------
 * NewDialog -- initialize a DialogRecord in caller-supplied storage and open
 * the dialog window.
 *
 * Parameters (mirroring IM-I Ch 6 "NewDialog"):
 *   storage     -- caller-supplied DialogRecord storage (not NULL).
 *   bounds      -- the dialog's bounding rectangle (global screen coords).
 *   title       -- the window title (ASCII; may be "" for a dBoxProc dialog).
 *   items       -- caller-supplied DialogItem array.
 *   itemCount   -- number of items in the array (>= 1).
 *   defaultItem -- 1-based default item index (Return key); 0 = none.
 *   cancelItem  -- 1-based cancel item index (Escape key); 0 = none.
 *   wm          -- the WindowMgr to register the dialog window with (for
 *                  z-order management). May be NULL for a standalone dialog
 *                  that is not part of the full desktop z-order.
 *   bounds_rgn  -- FOUR caller-supplied region_t's for the embedded window
 *                  (strucRgn, contRgn, updateRgn, and one scratch); all must
 *                  have rows[]/x_pool pools attached.
 *
 * Returns storage on success; FAIL-LOUD (dialog_panic) and returns NULL on
 * any NULL required argument or bounds_rgn shortage.
 *
 * After NewDialog the dialog is visible (window.visible = 1); the window's
 * windowDefProcVariant is dBoxProc (1) and windowKind is dialogKind (2).
 *
 * Ref: IM-I Ch 6 "NewDialog"; MTE Ch 6 p. 6-86 "NewDialog."
 * -------------------------------------------------------------------------- */
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
                    region_t       *updateRgn);

/* --------------------------------------------------------------------------
 * GetNewDialog -- alias for NewDialog (IM-I distinguished between in-memory
 * and resource-loaded; FLAIR has no resource fork in the current release, so
 * both use the same signature).
 *
 * Ref: IM-I Ch 6 "GetNewDialog"; MTE Ch 6 p. 6-87 "GetNewDialog."
 * -------------------------------------------------------------------------- */
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
                       region_t     *updateRgn);

/* --------------------------------------------------------------------------
 * DisposeDialog -- close and clean up the dialog.
 *
 * Removes the dialog's window from the WindowMgr list (if wm != NULL) and
 * zeroes the DialogRecord (safe to reuse the storage). Does NOT free any
 * caller-supplied region or item storage.
 *
 * Ref: IM-I Ch 6 "DisposDialog"; MTE Ch 6 "DisposeDialog."
 * -------------------------------------------------------------------------- */
void DisposeDialog(DialogPtr dp, WindowMgr *wm);

/* --------------------------------------------------------------------------
 * DrawDialog -- draw the dialog frame and all items into the dialog's port.
 *
 * Draws the dBoxProc 7-px solid border frame (FLAIR_CHROME_DIALOG_BORDER)
 * around the content rect, then draws each item in the item list:
 *   statText  -> text_draw (Chicago font, left-aligned within item rect).
 *   editText  -> text_draw (the current editBuf content) + cursor.
 *   ctrlItem  -> DrawControl(dp->window.port, item->ctrl).
 *   userItem  -> no-op (caller handles).
 *   iconItem  -> no-op (reserved).
 *
 * All drawing goes through the dialog's GrafPort (ADR-0004 D-1/D-2; the
 * surface module enforces clip).
 *
 * Ref: IM-I Ch 6 "DrawDialog"; MTE Ch 6 "DrawDialog."
 * -------------------------------------------------------------------------- */
void DrawDialog(DialogPtr dp);

/* --------------------------------------------------------------------------
 * ShowDialog -- make the dialog's window visible.
 *
 * Sets window.visible = 1. If the window was already visible, this is a
 * no-op. Does NOT redraw; the caller should call DrawDialog after ShowDialog.
 *
 * Ref: IM-I Ch 6 "ShowWindow" (used on dialog windows); MTE Ch 4 "ShowWindow."
 * -------------------------------------------------------------------------- */
void ShowDialog(DialogPtr dp);

/* --------------------------------------------------------------------------
 * ModalDialog -- cooperative modal event loop.
 *
 * Drains events from `ring` via WaitNextEvent (ADR-0004 D-4/D-6; task
 * context; cooperative). For each event:
 *   - mouseDown in an ENABLED ctrlItem -> TrackControl; if TrackControl
 *     returns non-zero, sets *itemHit to the item's 1-based index and
 *     returns.
 *   - mouseDown in a statText or disabled item -> ignored (not returned).
 *   - keyDown: if the key is Return/Enter and defaultItem != 0, set
 *     *itemHit to defaultItem and return.
 *   - keyDown: if the key is Escape and cancelItem != 0, set *itemHit to
 *     cancelItem and return.
 *   - filterProc (if non-NULL): called with (dp, event); if it returns
 *     non-zero, its *itemHit value is used and ModalDialog returns.
 *   - Other events: ignored (updateEvt -> DrawDialog; activateEvt -> no-op).
 *   - nullEvent (ring empty, sleepTicks expired): if sleepTicks == 0, also
 *     returns with *itemHit = 0 (allows non-blocking use in the oracle).
 *
 * `sleepTicks` controls how long to wait per pump call (0 = return
 * immediately on null event, useful for the deterministic test harness).
 *
 * Ref: IM-I Ch 6 "ModalDialog"; MTE Ch 6 p. 6-84 "ModalDialog."
 *   "ModalDialog(filterProc, itemHit): calls GetNextEvent in a loop,
 *    takes care of some events automatically, and calls your filter
 *    procedure for others."
 * -------------------------------------------------------------------------- */
typedef int (*dialog_filter_fn)(DialogPtr dp, EventRecord *ev, uint16_t *itemHit);

void ModalDialog(DialogPtr         dp,
                 flair_raw_ring_t *ring,
                 dialog_filter_fn  filterProc,
                 uint32_t          sleepTicks,
                 uint16_t         *itemHit);

/* --------------------------------------------------------------------------
 * FindDialogItem -- hit-test a point against dialog items.
 *
 * Walks items[] front-to-back; returns the 1-based index of the FIRST item
 * whose rect contains pt (using QuickDraw half-open rect convention:
 * left <= pt.h < right AND top <= pt.v < bottom), or 0 if none.
 *
 * Ref: IM-I Ch 6 "FindDItem"; MTE Ch 6 "FindDialogItem."
 * -------------------------------------------------------------------------- */
uint16_t FindDialogItem(const DialogPtr dp, flair_point_t pt);

/* --------------------------------------------------------------------------
 * GetDialogItem -- retrieve item attributes.
 *
 * `itemIndex` is 1-based. On return:
 *   *type   -- the item's type.
 *   *rect   -- the item's bounding rect.
 * Panics if dp is NULL or itemIndex is out of range.
 *
 * Ref: IM-I Ch 6 "GetDItem"; MTE Ch 6 "GetDialogItem."
 * -------------------------------------------------------------------------- */
void GetDialogItem(const DialogPtr            dp,
                   uint16_t                   itemIndex,
                   flair_dialog_item_type_t  *type,
                   rgn_rect_t                *rect);

/* --------------------------------------------------------------------------
 * SetDialogItem -- replace item attributes.
 *
 * `itemIndex` is 1-based. Updates items[itemIndex-1].type, .rect, .text,
 * .ctrl as directed. Panics if dp is NULL or itemIndex is out of range.
 *
 * Ref: IM-I Ch 6 "SetDItem"; MTE Ch 6 "SetDialogItem."
 * -------------------------------------------------------------------------- */
void SetDialogItem(DialogPtr                 dp,
                   uint16_t                  itemIndex,
                   flair_dialog_item_type_t  type,
                   rgn_rect_t                rect,
                   const char               *text,
                   ControlRecord            *ctrl,
                   uint8_t                   enabled);

/* --------------------------------------------------------------------------
 * GetDialogItemText -- read the text of a statText or editText item.
 *
 * `itemIndex` is 1-based. Copies the item's text (or the current editBuf for
 * an active editText) into `buf`, truncated to `bufsz - 1` bytes + NUL.
 * Panics if dp is NULL or itemIndex is out of range.
 *
 * Ref: IM-I Ch 6 "GetIText"; MTE Ch 6 "GetDialogItemText."
 * -------------------------------------------------------------------------- */
void GetDialogItemText(const DialogPtr dp, uint16_t itemIndex,
                       char *buf, uint32_t bufsz);

/* --------------------------------------------------------------------------
 * SetDialogItemText -- set the text of a statText or editText item.
 *
 * `itemIndex` is 1-based. Stores a pointer to `text` in the item's text
 * field. For an editText item also copies into editBuf. Panics if dp is
 * NULL or itemIndex out of range.
 *
 * Ref: IM-I Ch 6 "SetIText"; MTE Ch 6 "SetDialogItemText."
 * -------------------------------------------------------------------------- */
void SetDialogItemText(DialogPtr dp, uint16_t itemIndex, const char *text);

/* --------------------------------------------------------------------------
 * FileCopyDialog -- build the canonical FILE COPY modal dialog.
 *
 * Fills `storage` + `itemStorage` (caller-supplied; must be at least 2
 * DialogItems) and `ctrlStorage` (at least 1 ControlRecord for the progress
 * bar) with the pre-configured FILE COPY layout:
 *   Item 1: statText, FLAIR_CANON_FILECOPY_MSG, enabled=0 (display only).
 *   Item 2: ctrlItem, progressBar (contrlValue=0, contrlMax=100), enabled=0.
 *
 * Dialog bounds are set to the canonical FILE COPY box size and position
 * (centered, 360x80 px, top-left at (140, 200) on a 640x480 screen).
 * The dBoxProc 7-px border frame (FLAIR_CHROME_DIALOG_BORDER) is applied
 * by DrawDialog as for all dBoxProc dialogs.
 *
 * Returns the initialized DialogPtr (= storage).
 *
 * Ref: PRD Sec 6.5 / Appendix A/B (the canon FILE COPY box, Law 4).
 *      FLAIR_CANON_FILECOPY_MSG (Section 1 above).
 * -------------------------------------------------------------------------- */
DialogPtr FileCopyDialog(DialogRecord  *storage,
                         DialogItem    *itemStorage,
                         ControlRecord *ctrlStorage,
                         region_t      *strucRgn,
                         region_t      *contRgn,
                         region_t      *updateRgn);

/* ===========================================================================
 * 6. COMPILE-TIME CONTRACT CHECKS
 * ===========================================================================*/

/* DialogRecord.window must be the first field at offset 0
 * (DialogPtr castable to WindowPtr / GrafPtr). */
_Static_assert(offsetof(DialogRecord, window) == 0,
               "DialogRecord.window must be at offset 0 "
               "(DialogPtr casts to WindowPtr; IM-I Ch 6)");

/* The embedded WindowRecord must be followed by the item list fields. */
_Static_assert(offsetof(DialogRecord, items) > offsetof(DialogRecord, window),
               "DialogRecord.items must follow the embedded window (IM-I)");

/* FLAIR_CHROME_DIALOG_BORDER must be 7 (dBoxBorderSize EQU 7; WDEF). */
_Static_assert(FLAIR_CHROME_DIALOG_BORDER == 7,
               "dBoxProc border must be 7 px (WDEF dBoxBorderSize EQU 7; "
               "spec/chrome_metrics.h)");

/* dBoxProc variant code must be 1 (MTE Table 4-1). */
_Static_assert(dBoxProc == 1,
               "dBoxProc must be 1 (MTE Table 4-1; spec/window_record.h)");

/* dialogKind must be 2 (IM-I p. I-270). */
_Static_assert(dialogKind == 2,
               "dialogKind must be 2 (IM-I p. I-270; spec/window_record.h)");

/* FLAIR_CANON_FILECOPY_MSG must be a non-empty string literal. */
_Static_assert(sizeof(FLAIR_CANON_FILECOPY_MSG) > 1u,
               "FLAIR_CANON_FILECOPY_MSG must be non-empty (Law 4 canon)");

#endif /* INITECH_OS_FLAIR_DIALOG_H */
