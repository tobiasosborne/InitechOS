/*
 * spec/window_record.h -- FLAIR Window Manager: the LOCKED contract.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8; PRD Sec 6.3 -- "the Toolbox layer").
 * beads: initech-k8o5.4 (FLAIR event + window model as spec-data).
 *
 * This header locks the WindowRecord, the FindWindow part-codes, the window-
 * kind constants, and the windowDefProc variant codes -- all verbatim from
 * Inside Macintosh (ADR-0004 D-3).
 *
 * SOURCE CITATIONS (Law 1 -- all local or free Apple developer archive):
 *
 *   Inside Macintosh: Macintosh Toolbox Essentials (1992) [MTE; free Apple
 *     dev archive; docs/research/gui-ground-truth.md Sec 3.2]:
 *     Chapter 4 "Window Manager" -- the PRIMARY reference for this header:
 *       WindowRecord definition (MTE p. 4-78..4-81).
 *       FindWindow return codes / part-code constants (MTE p. 4-63,
 *         "FindWindow" routine, Table 4-2 "Part-Code Constants").
 *       Window-definition procedure (windowDefProc) variant codes (MTE p. 4-8
 *         Table 4-1 "Window Types", giving documentProc=0, dBoxProc=1,
 *         plainDBox=2, altDBoxProc=3, noGrowDocProc=4, zoomDocProc=8,
 *         zoomNoGrow=12).
 *       Window-kind constants: documentKind (MTE p. 4-81 WindowRecord
 *         "windowKind: A value that identifies the type of window"), dialogKind,
 *         userKind (IM-I Ch 4 p. I-270 -- see IM-I note below).
 *       visRgn / clipRgn clipping contract (MTE p. 4-15: "The Window Manager
 *         sets a window's visible region to that portion of the window not
 *         obscured by windows in front of it").
 *
 *   Inside Macintosh Vol I Ch 4 (IM-I) [archive.org bitsavers_applemacIn84;
 *     docs/research/gui-ground-truth.md Sec 3.2]:
 *     "The Window Manager" (IM-I p. I-267 ff):
 *       WindowRecord fields (IM-I p. I-268): port, strucRgn, contRgn,
 *         updateRgn, windowKind, visible, hilited, goAwayFlag, spareFlag,
 *         refCon, nextWindow, windowPic/dataHandle, title fields.
 *       windowKind values: dialogKind=2 (IM-I p. I-270); userKind=8 (IM-I
 *         p. I-270: "applications may use values from 8 through 32767 for
 *         their own windows"); documentKind=8 is the canonical app-window
 *         value (per MTE Ch 4 "documentKind" constant listing).
 *       FindWindow part-codes (IM-I p. I-280): same values as MTE, verbatim.
 *
 *   ADR-0004 (FLAIR Toolbox Architecture, RATIFIED 2026-06-19):
 *     D-3 (Manager decomposition with verbatim Inside Macintosh records and
 *       part-codes): "Window Manager -- WindowRecord (port, strucRgn/contRgn/
 *       updateRgn, windowKind, visibility, title); FindWindow part-codes
 *       inDesk, inMenuBar, inContent, inDrag, inGrow, inGoAway, inZoomIn/
 *       inZoomOut. Drag/z-order/update-region maintenance via layer 2
 *       (DiffRgn, Sec 3.5)."
 *     D-5 (region-difference damage model): when a window moves, closes, or
 *       changes z-order, newly-exposed area is computed by DiffRgn and
 *       accumulated into each window's `updateRgn`. The pump issues updateEvts.
 *
 *   spec/grafport.h (GrafPort -- EMBEDDED in WindowRecord as `port`; the
 *     QuickDraw drawing context; flair_point_t for `where` coordinates).
 *     Do NOT redefine GrafPort.
 *   spec/region_algebra.h (region_t -- for strucRgn, contRgn, updateRgn;
 *     rgn_rect_t for Rect; region pointers are direct, NOT handles).
 *     Do NOT redefine region_t.
 *
 * DESIGN: GrafPort EMBED vs POINTER DECISION
 * -------------------------------------------
 * The QuickDraw WindowRecord embeds a GrafPort as its FIRST field (IM-I
 * p. I-268: "port: GrafPort. The window's grafPort."). The port-embedded-
 * first convention is LOAD-BEARING in inside Macintosh: a WindowPtr (pointer
 * to WindowRecord) can be cast directly to a GrafPort * because the GrafPort
 * is at offset 0. SetPort(theWindow) works because theWindow IS a GrafPtr.
 *
 * FLAIR EMBEDS (not pointers to) the GrafPort for the same reason:
 *   (a) Verbatim IM-I field name and layout: port at offset 0, WindowPtr
 *       castable to GrafPort * (IM-I p. I-268 contract).
 *   (b) Freestanding / no-malloc discipline (Rule 11, Law 3): embedding the
 *       port avoids a separate allocation from the FLAIR heap (ADR-0004 DEC-03).
 *       The WindowRecord is itself allocated from the FLAIR heap arena
 *       (bump + typed free-list; ADR-0004 DEC-03); the embedded GrafPort is
 *       part of that single allocation. This is simpler, more cache-friendly,
 *       and avoids two-level indirection.
 *   (c) Period-authentic: Mac OS never heap-allocated GrafPorts separately
 *       from WindowRecords; the struct layout IS the "handle" model for
 *       windows.
 *
 * DESIGN: region_t POINTERS (not handles)
 * ----------------------------------------
 * QuickDraw carries strucRgn/contRgn/updateRgn as RgnHandles (double-
 * indirected heap handles; IM-I p. I-268). FLAIR uses DIRECT POINTERS
 * (region_t *) to arena-allocated region_t instances for the same reason as
 * in grafport.h (spec/grafport.h design note): FLAIR has no heap-handle table;
 * region storage is caller-supplied from the FLAIR arena (ADR-0004 DEC-03;
 * spec/region_algebra.h arena-backed model; Law 3 freestanding). The Window
 * Manager owns these region_t instances and initializes them from the arena
 * at window creation; they are valid for the window's lifetime.
 * QuickDraw NAMES (strucRgn, contRgn, updateRgn) are preserved verbatim.
 *
 * DESIGN: title storage
 * ----------------------
 * QuickDraw's WindowRecord carries `titleHandle` (a StringHandle, i.e. a
 * double-indirected heap handle to a Pascal-format length-prefixed string;
 * IM-I p. I-268). FLAIR carries a fixed-length in-place title buffer
 * (null-terminated, max FLAIR_WINDOW_TITLE_MAX chars) for the current release,
 * avoiding heap handles. The verbatim name `titleHandle` is used as the field
 * name to preserve search-ability against IM references; the type is a char
 * array, not a double-pointer. This is an acknowledged FLAIR deviation from
 * the verbatim QuickDraw handle model, justified by freestanding constraints.
 * A follow-up may add a StringHandle-equivalent once the FLAIR heap handle
 * table is authored (ADR-0004 FO-5; beads initech-k8o5.5).
 *
 * DUAL-COMPILE: freestanding (gcc -m32 -ffreestanding -nostdlib -std=c11)
 * AND hosted (cc -std=c11). Only <stdint.h> + <stddef.h> + local headers.
 * No host malloc; no libc beyond stdint/stddef. Rule 11 (reproducible).
 * ASCII-clean (Rule 12). Changing this file is a deliberate, Rule 8 act.
 */
#ifndef INITECH_SPEC_WINDOW_RECORD_H
#define INITECH_SPEC_WINDOW_RECORD_H

#include <stdint.h>
#include <stddef.h>

#include "grafport.h"         /* GrafPort (embedded); flair_point_t          */
#include "region_algebra.h"   /* region_t *, rgn_rect_t                      */

/* ===========================================================================
 * 1. FINDWINDOW PART-CODE CONSTANTS  (verbatim Inside Macintosh values)
 * ---------------------------------------------------------------------------
 * Ref: MTE Chapter 4, Table 4-2 (p. 4-63): "Part-Code Constants."
 *      IM-I Ch 4 p. I-280: FindWindow routine, result codes (same values).
 *
 * FindWindow(point, whichWindow) determines which part of which window a
 * global-coordinate point falls in. It returns one of these constants (stored
 * in an int16_t) and, via whichWindow, a pointer to the window (or NULL for
 * inDesk/inMenuBar). FLAIR's Window Manager implements FindWindow using
 * ATKINSON region membership tests (region_contains_point on strucRgn/contRgn)
 * to locate the window and then tests chrome sub-regions to select the part.
 *
 * Values are verbatim from MTE Table 4-2 / IM-I p. I-280 (Pascal Integer).
 * ===========================================================================*/

/* flair_part_code_t -- FindWindow result (verbatim IM constant values). */
typedef enum flair_part_code {
    inDesk     = 0,  /* point is on the desktop (no window)     (MTE Table 4-2) */
    inMenuBar  = 1,  /* point is in the menu bar                (MTE Table 4-2) */
    inSysWindow= 2,  /* point is in a system window (desk acc.) (MTE Table 4-2) */
    inContent  = 3,  /* point is in the window's content region (MTE Table 4-2) */
    inDrag     = 4,  /* point is in the window's drag region    (MTE Table 4-2) */
    inGrow     = 5,  /* point is in the window's grow box       (MTE Table 4-2) */
    inGoAway   = 6,  /* point is in the window's close (goAway) box             */
    inZoomIn   = 7,  /* point is in the zoom-in box             (MTE Table 4-2) */
    inZoomOut  = 8   /* point is in the zoom-out box            (MTE Table 4-2) */
} flair_part_code_t;

/* ===========================================================================
 * 2. WINDOW-KIND CONSTANTS  (verbatim Inside Macintosh values)
 * ---------------------------------------------------------------------------
 * Ref: IM-I Ch 4 p. I-270 (windowKind field of WindowRecord):
 *   "The windowKind field contains a value that distinguishes different types
 *    of windows." (Paraphrase; IM-I p. I-270 and MTE Ch 4 p. 4-81.)
 *   dialogKind  = 2: "the window is a dialog window" (IM-I p. I-270).
 *   userKind    = 8: applications use values 8..32767 for their own windows
 *                    (IM-I p. I-270: "Applications may use values from 8
 *                     through 32767 for their own windows").
 *   documentKind= 8: the conventional value for a standard document window
 *                    (MTE Ch 4 "documentKind" constant; equal to userKind
 *                    because all document windows are application-owned;
 *                    two names for the same numeric value, by convention).
 *
 * NOTE: documentKind and userKind are both 8. This is correct and verbatim
 * from Inside Macintosh -- there is no conflict. The Window Manager uses
 * windowKind < 0 for system/DA windows in Mac OS; userKind (8) is the
 * canonical starting value for application windows.
 * ===========================================================================*/

#define FLAIR_WINDOW_KIND_DIALOG    2   /* dialogKind  = 2  (IM-I p. I-270) */
#define FLAIR_WINDOW_KIND_USER      8   /* userKind    = 8  (IM-I p. I-270) */
#define FLAIR_WINDOW_KIND_DOCUMENT  8   /* documentKind= 8  (MTE Ch 4)      */

/* Aliases for verbatim IM names (easier cross-reference). */
#define dialogKind    FLAIR_WINDOW_KIND_DIALOG
#define userKind      FLAIR_WINDOW_KIND_USER
#define documentKind  FLAIR_WINDOW_KIND_DOCUMENT

/* ===========================================================================
 * 3. WINDOWDEFPROC VARIANT CODES  (verbatim Inside Macintosh values)
 * ---------------------------------------------------------------------------
 * Ref: MTE Chapter 4, Table 4-1 (p. 4-8): "Window Types."
 *      IM-I Ch 4 p. I-273 (window definition ID = resource ID * 16 + variant;
 *      the standard WDEF is resource 0; variant codes below are the low 4 bits
 *      of the window definition ID passed to NewWindow/GetNewWindow).
 *
 * The windowDefProc selects the WDEF variant that determines the window's
 * chrome appearance and hit-testing geometry. FLAIR's Window Manager carries
 * an internal WDEF dispatch keyed by these variant codes; the window-drawing
 * code in os/flair consults the variant stored in each WindowRecord.
 *
 * Verbatim values (MTE Table 4-1 / IM-I p. I-273, "Window Definition ID"):
 *   documentProc  = 0:  Standard document window (title bar, close, zoom, grow)
 *   dBoxProc      = 1:  Dialog box with shadow (the standard dialog box)
 *   plainDBox     = 2:  Plain dialog box (no shadow, no title)
 *   altDBoxProc   = 3:  Alert box (slightly thinner border than dBoxProc)
 *   noGrowDocProc = 4:  Document window without grow box
 *   zoomDocProc   = 8:  Document window with zoom box (same chrome as
 *                        documentProc on most WDEF implementations; zoom
 *                        box + grow box present)
 *   zoomNoGrow    = 12: Document window with zoom box but no grow box
 *
 * NOTE: the windowDefProc stored in WindowRecord is the full 16-bit definition
 * ID (WDEF resource ID * 16 + variant). For the standard Mac WDEF (resource
 * ID 0), the definition ID equals the variant code directly. FLAIR's resident
 * WDEF is always resource 0, so the definition IDs below ARE the variant codes.
 * ===========================================================================*/

#define FLAIR_WDEF_DOCUMENT_PROC   0   /* documentProc  = 0  (MTE Table 4-1) */
#define FLAIR_WDEF_DBOX_PROC       1   /* dBoxProc      = 1  (MTE Table 4-1) */
#define FLAIR_WDEF_PLAIN_DBOX      2   /* plainDBox     = 2  (MTE Table 4-1) */
#define FLAIR_WDEF_ALT_DBOX_PROC   3   /* altDBoxProc   = 3  (MTE Table 4-1) */
#define FLAIR_WDEF_NO_GROW_DOC     4   /* noGrowDocProc = 4  (MTE Table 4-1) */
#define FLAIR_WDEF_ZOOM_DOC_PROC   8   /* zoomDocProc   = 8  (MTE Table 4-1) */
#define FLAIR_WDEF_ZOOM_NO_GROW   12   /* zoomNoGrow    = 12 (MTE Table 4-1) */

/* Aliases for verbatim IM names. */
#define documentProc  FLAIR_WDEF_DOCUMENT_PROC
#define dBoxProc      FLAIR_WDEF_DBOX_PROC
#define plainDBox     FLAIR_WDEF_PLAIN_DBOX
#define altDBoxProc   FLAIR_WDEF_ALT_DBOX_PROC
#define noGrowDocProc FLAIR_WDEF_NO_GROW_DOC
#define zoomDocProc   FLAIR_WDEF_ZOOM_DOC_PROC
#define zoomNoGrow    FLAIR_WDEF_ZOOM_NO_GROW

/* ===========================================================================
 * 4. WindowRecord -- the window descriptor (verbatim field names)
 * ---------------------------------------------------------------------------
 * Ref: MTE Chapter 4 p. 4-78..4-81: "The WindowRecord Data Type."
 *      IM-I Ch 4 p. I-268: WindowRecord field listing.
 *
 * The verbatim Pascal WindowRecord in Inside Macintosh (IM-I p. I-268):
 *
 *   port          GrafPort      ; the window's grafPort (FIRST; WindowPtr
 *                               ; casts to GrafPtr -- IM-I p. I-268 contract)
 *   windowKind    Integer       ; window type identifier
 *   visible       Boolean       ; whether the window is visible
 *   hilited       Boolean       ; whether the window is highlighted (active)
 *   goAwayFlag    Boolean       ; whether the window has a close box
 *   spareFlag     Boolean       ; reserved (MTE p. 4-81: "reserved")
 *   strucRgn      RgnHandle     ; region covering the window's complete structure
 *   contRgn       RgnHandle     ; region of the window's content area
 *   updateRgn     RgnHandle     ; region of the content area needing update
 *   windowDefProc Handle        ; handle to the window definition procedure
 *   dataHandle    Handle        ; reserved / private (MTE p. 4-81)
 *   titleHandle   StringHandle  ; handle to the window's title string
 *   titleWidth    Integer       ; pixel width of the window's title
 *   nextWindow    WindowPtr     ; next window in the window list (linked list)
 *   refCon        LongInt       ; application-specific data (32-bit)
 *
 * FLAIR DEVIATIONS (all justified, Law 3 / freestanding):
 *   - port: EMBEDDED (not a pointer); see design note in file header.
 *   - strucRgn/contRgn/updateRgn: region_t * (direct pointer, not RgnHandle);
 *     see design note in file header.
 *   - windowDefProc: int16_t holding the WDEF variant code (see Section 3);
 *     no heap handle to a WDEF procedure (FLAIR has a built-in WDEF dispatch).
 *   - titleHandle: char[] in-place buffer (null-terminated; max
 *     FLAIR_WINDOW_TITLE_MAX bytes including null); no StringHandle heap.
 *   - dataHandle: omitted (no heap handles in current release).
 *   - titleWidth: carried as int16_t (verbatim IM-I name); set by the Window
 *     Manager when the title is installed (proportional NFNT measurement,
 *     ADR-0004 D-7).
 *   - nextWindow: struct WindowRecord * (typed pointer for the linked list;
 *     verbatim IM-I name; NULL for the last window).
 * ===========================================================================*/

/* Maximum window title length (bytes, including null terminator).
 * Enough for Chicago 12 proportional text in a 640-px title bar (ADR-0004 D-7;
 * a 63-char title at min ~5 px/char = 315 px, well under the 640 px bar width
 * minus close/zoom boxes). Locked (Rule 8).                                   */
#define FLAIR_WINDOW_TITLE_MAX  64u   /* 63 chars + null terminator (bytes)   */

/* Forward-declare WindowRecord so nextWindow can self-reference. */
typedef struct WindowRecord WindowRecord;

/*
 * WindowRecord (verbatim QuickDraw name, IM-I Ch 4 p. I-268; MTE Ch 4 p. 4-78):
 *   "The Window Manager maintains a linked list of window records ..."
 *   All FLAIR windows are allocated from the FLAIR heap arena (ADR-0004 DEC-03)
 *   and are linked through nextWindow. The front window is at the head of the
 *   list (lowest z-order = top of the screen).
 *
 * INVARIANTS:
 *   - port is at offset 0; a WindowRecord * casts to GrafPort * (IM-I contract).
 *   - strucRgn, contRgn, updateRgn are non-NULL for any open window; owned by
 *     the Window Manager and backed by the FLAIR heap arena.
 *   - visible == 0 means the window is hidden; the Window Manager does not
 *     include hidden windows in the front-to-back z-order paint pass.
 *   - hilited == 1 for the frontmost (active) window; all others have hilited=0.
 *   - updateRgn starts empty; the damage model (ADR-0004 D-5, DiffRgn) adds
 *     to it; the pump issues updateEvt when updateRgn is non-empty.
 */
struct WindowRecord {
    /* --- GrafPort (FIRST field; MUST be at offset 0; IM-I p. I-268) ---------
     * Ref: IM-I "port: GrafPort. The window's grafPort."
     * MTE Ch 4: "The first field of a window record is a grafPort."
     * WindowRecord * is freely castable to GrafPort * (= WindowPtr to GrafPtr).
     * The port's portRect corresponds to the content area (contRgn bounding box);
     * its visRgn is set by the Window Manager to the visible portion (MTE 4-15).
     * Its clipRgn is initially the full content area; apps narrow it as needed. */
    GrafPort     port;

    /* --- Window type identifier (IM-I p. I-268: "windowKind") ---------------
     * Ref: IM-I "windowKind: Integer. Contains a value that identifies the type
     *      of window." Values: dialogKind=2; documentKind=userKind=8 (Sec 2).
     * The Window Manager does not interpret windowKind for rendering; it is
     * available to applications to distinguish window flavors. */
    int16_t      windowKind;

    /* --- Visibility (IM-I p. I-268: "visible") ------------------------------
     * Ref: IM-I "visible: Boolean. TRUE if the window is visible, FALSE if it's
     *      invisible."
     * uint8_t holds a Boolean (0 = FALSE, non-zero = TRUE; IM-I convention). */
    uint8_t      visible;

    /* --- Highlighted / active state (IM-I p. I-268: "hilited") --------------
     * Ref: IM-I "hilited: Boolean. TRUE if the window is highlighted."
     * The frontmost visible window has hilited=TRUE; all others FALSE.
     * The Window Manager updates hilited on BringToFront/SendBehind. */
    uint8_t      hilited;

    /* --- Close box present (IM-I p. I-268: "goAwayFlag") --------------------
     * Ref: IM-I "goAwayFlag: Boolean. TRUE if the window has a close box."
     * Standard document windows (documentProc/zoomDocProc) have goAwayFlag=TRUE;
     * dialog boxes (dBoxProc/plainDBox) have goAwayFlag=FALSE. */
    uint8_t      goAwayFlag;

    /* --- Reserved / spare (IM-I p. I-268: "spareFlag") ----------------------
     * Ref: MTE p. 4-81: "reserved for use by Apple." Must be 0 on construction.
     * Not set by the FLAIR Window Manager; preserved for layout compatibility. */
    uint8_t      spareFlag;

    /* --- Structure region (IM-I p. I-268: "strucRgn") -----------------------
     * Ref: IM-I "strucRgn: RgnHandle. The structure region, which is the
     *      complete region of the window."
     * Covers the full chrome + content: title bar, frame, drop shadow, grow
     * box, etc. Used by FindWindow to hit-test the outer chrome. Owned by
     * the Window Manager (FLAIR heap arena). Never NULL for an open window. */
    region_t    *strucRgn;

    /* --- Content region (IM-I p. I-268: "contRgn") --------------------------
     * Ref: IM-I "contRgn: RgnHandle. The content region, which is the area
     *      inside the window frame (excluding the title bar and scroll bars)."
     * Used for inContent hit-testing. The port's portRect bounding box matches
     * contRgn's bounding box for a rectangular window. Never NULL. */
    region_t    *contRgn;

    /* --- Update region (IM-I p. I-268: "updateRgn") -------------------------
     * Ref: IM-I "updateRgn: RgnHandle. The update region, which is the part
     *      of the content region that must be redrawn."
     * Starts empty on window creation; the damage model (ADR-0004 D-5, DiffRgn)
     * accumulates newly-exposed areas into it; the pump issues updateEvt when
     * updateRgn is non-empty; the app draws clipped to updateRgn and calls
     * BeginUpdate/EndUpdate to clear it. Never NULL (may be the empty region). */
    region_t    *updateRgn;

    /* --- Window definition proc variant (FLAIR-internal; verbatim IM name) --
     * Ref: IM-I p. I-268 "windowDefProc: Handle. A handle to the window's
     *      definition procedure."
     * FLAIR substitution: int16_t holding the WDEF variant code from Section 3
     * (documentProc=0, dBoxProc=1, ..., zoomNoGrow=12). FLAIR has a built-in
     * WDEF dispatch keyed by this value; no heap handle is needed. The Window
     * Manager reads windowDefProcVariant to select chrome drawing and hit-
     * testing sub-regions (title bar / drag / grow / goAway / zoom boxes). */
    int16_t      windowDefProcVariant;  /* FLAIR: WDEF variant (Sec 3 constants) */

    /* --- Window title (FLAIR-internal; IM name: titleHandle) ----------------
     * Ref: IM-I p. I-268 "titleHandle: StringHandle. A handle to the window's
     *      title."
     * FLAIR substitution: in-place null-terminated UTF-8/ASCII char buffer of
     * FLAIR_WINDOW_TITLE_MAX bytes (63 chars + null). See design note in header.
     * The titleHandle name is preserved for IM cross-reference. Empty string ("")
     * means no title. Set by NewWindow / SetWTitle. */
    char         titleHandle[FLAIR_WINDOW_TITLE_MAX];  /* window title (FLAIR) */

    /* --- Title pixel width (IM-I p. I-268: "titleWidth") --------------------
     * Ref: IM-I "titleWidth: Integer. The width in pixels of the window title."
     * Set by the Window Manager using proportional NFNT text measurement
     * (ADR-0004 D-7: "proportional NFNT text measurement ... text width is
     * the sum of per-glyph advances"). 0 if no title. */
    int16_t      titleWidth;

    /* Pad for alignment after titleWidth (int16_t) to keep nextWindow pointer
     * aligned to 4 bytes on 32-bit flat (ADR-0001). */
    uint8_t      _pad_title[2];

    /* --- Next window in the z-order list (IM-I p. I-268: "nextWindow") ------
     * Ref: IM-I "nextWindow: WindowPtr. A pointer to the next window in the
     *      window list (behind this window in the z-order)."
     * NULL for the backmost window. The desktop shell (Layer 5, ADR-0004 D-1)
     * maintains the list head (frontmost window pointer). The Window Manager
     * traverses this linked list for z-order operations (BringToFront,
     * SendBehind, SelectWindow). */
    WindowRecord *nextWindow;

    /* --- Application-specific data (IM-I p. I-268: "refCon") ----------------
     * Ref: IM-I "refCon: LongInt. An application-defined reference value."
     * A 32-bit opaque slot for application use; the Window Manager never reads
     * or modifies it. Set by NewWindow / SetWRefCon; read by GetWRefCon. */
    int32_t      refCon;
};

/* WindowPtr -- a pointer to a WindowRecord (verbatim QuickDraw name).
 * Since GrafPort is at offset 0, a WindowPtr is also a valid GrafPtr.
 * Ref: IM-I Ch 4 "A WindowPtr is a pointer to a window record." */
typedef WindowRecord *WindowPtr;

/* ===========================================================================
 * 5. COMPILE-TIME CONTRACT CHECKS  (the oracle bites at build time)
 * ---------------------------------------------------------------------------
 * Style follows spec/region_algebra.h (the style exemplar).
 * ===========================================================================*/

/* FindWindow part-codes -- verbatim IM values (MTE Table 4-2 / IM-I p. I-280). */
_Static_assert((int)inDesk      == 0, "inDesk=0 (MTE Table 4-2 / IM-I p. I-280)");
_Static_assert((int)inMenuBar   == 1, "inMenuBar=1 (MTE Table 4-2)");
_Static_assert((int)inSysWindow == 2, "inSysWindow=2 (MTE Table 4-2)");
_Static_assert((int)inContent   == 3, "inContent=3 (MTE Table 4-2)");
_Static_assert((int)inDrag      == 4, "inDrag=4 (MTE Table 4-2)");
_Static_assert((int)inGrow      == 5, "inGrow=5 (MTE Table 4-2)");
_Static_assert((int)inGoAway    == 6, "inGoAway=6 (MTE Table 4-2)");
_Static_assert((int)inZoomIn    == 7, "inZoomIn=7 (MTE Table 4-2)");
_Static_assert((int)inZoomOut   == 8, "inZoomOut=8 (MTE Table 4-2)");

/* Window-kind constants (IM-I p. I-270). */
_Static_assert(dialogKind   == 2, "dialogKind=2 (IM-I p. I-270)");
_Static_assert(userKind     == 8, "userKind=8 (IM-I p. I-270)");
_Static_assert(documentKind == 8, "documentKind=8 (MTE Ch 4; same as userKind)");

/* Window-definition proc variant codes (MTE Table 4-1). */
_Static_assert(documentProc  == 0,  "documentProc=0 (MTE Table 4-1)");
_Static_assert(dBoxProc      == 1,  "dBoxProc=1 (MTE Table 4-1)");
_Static_assert(plainDBox     == 2,  "plainDBox=2 (MTE Table 4-1)");
_Static_assert(altDBoxProc   == 3,  "altDBoxProc=3 (MTE Table 4-1)");
_Static_assert(noGrowDocProc == 4,  "noGrowDocProc=4 (MTE Table 4-1)");
_Static_assert(zoomDocProc   == 8,  "zoomDocProc=8 (MTE Table 4-1)");
_Static_assert(zoomNoGrow    == 12, "zoomNoGrow=12 (MTE Table 4-1)");

/* WindowRecord.port must be the FIRST field at offset 0 (IM-I contract). */
_Static_assert(offsetof(WindowRecord, port) == 0,
               "WindowRecord.port must be at offset 0 (WindowPtr casts to GrafPtr; IM-I)");

/* The region pointers must follow the GrafPort (ordering check). */
_Static_assert(offsetof(WindowRecord, strucRgn) > offsetof(WindowRecord, port),
               "WindowRecord.strucRgn must follow port (IM-I field order)");
_Static_assert(offsetof(WindowRecord, contRgn) > offsetof(WindowRecord, strucRgn),
               "WindowRecord.contRgn must follow strucRgn");
_Static_assert(offsetof(WindowRecord, updateRgn) > offsetof(WindowRecord, contRgn),
               "WindowRecord.updateRgn must follow contRgn");

/* Region fields are pointers (not inline regions; see design note). */
_Static_assert(sizeof(((WindowRecord *)0)->strucRgn)  == sizeof(void *),
               "WindowRecord.strucRgn must be a pointer (region_t *)");
_Static_assert(sizeof(((WindowRecord *)0)->contRgn)   == sizeof(void *),
               "WindowRecord.contRgn must be a pointer (region_t *)");
_Static_assert(sizeof(((WindowRecord *)0)->updateRgn) == sizeof(void *),
               "WindowRecord.updateRgn must be a pointer (region_t *)");

/* Title buffer is FLAIR_WINDOW_TITLE_MAX bytes. */
_Static_assert(sizeof(((WindowRecord *)0)->titleHandle) == FLAIR_WINDOW_TITLE_MAX,
               "WindowRecord.titleHandle must be FLAIR_WINDOW_TITLE_MAX bytes");

/* nextWindow is a pointer to WindowRecord (linked list node). */
_Static_assert(sizeof(((WindowRecord *)0)->nextWindow) == sizeof(void *),
               "WindowRecord.nextWindow must be a WindowRecord pointer");

/* refCon is 4 bytes (int32_t / LongInt). */
_Static_assert(sizeof(((WindowRecord *)0)->refCon) == 4,
               "WindowRecord.refCon must be 4 bytes (LongInt; IM-I p. I-268)");

/* WindowRecord is non-trivially sized (contains a full GrafPort + regions). */
_Static_assert(sizeof(WindowRecord) > sizeof(GrafPort),
               "WindowRecord must be larger than GrafPort (it embeds one + extras)");

/* FLAIR_WINDOW_TITLE_MAX must allow at least 1 char plus the null. */
_Static_assert(FLAIR_WINDOW_TITLE_MAX >= 2u,
               "FLAIR_WINDOW_TITLE_MAX must be at least 2 (1 char + null)");

#endif /* INITECH_SPEC_WINDOW_RECORD_H */
