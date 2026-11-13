/*
 * spec/grafport.h -- GrafPort drawing context: the LOCKED contract.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8; PRD Sec 6.3 -- "the Toolbox layer").
 * beads: initech-k8o5.3 (FLAIR imaging contract as spec-data).
 *
 * This header defines the GrafPort record (ADR-0004 D-2): the QuickDraw
 * drawing context that all FLAIR drawing flows through. Every drawing
 * primitive (FillRect, FrameRect, BlitBits, text draw) consults the current
 * GrafPort's effective clip before touching a pixel. The Managers (Layer 3,
 * ADR-0004 D-1) never touch the framebuffer directly; the GrafPort is the
 * SOLE conduit (ADR-0004 C-1).
 *
 * SOURCE CITATIONS (Law 1 -- all local or free Apple developer archive):
 *
 *   Inside Macintosh Vol I Ch 7 (IM-I) [archive.org bitsavers_applemacIn84;
 *     docs/research/gui-ground-truth.md Sec 3.2]:
 *     "Data Types" (GrafPort record, p. I-148); "Routines" (SetPort, GetPort,
 *     OpenPort, ClosePort); the GrafPort's portBits, portRect, visRgn,
 *     clipRgn, pnLoc, pnSize, pnMode, pnPat, pnVis, fgColor, bkColor,
 *     txFont, txFace, txMode, txSize, spExtra (p. I-148 ff).
 *
 *   Inside Macintosh: Imaging With QuickDraw (1994) [IWQ; free Apple dev
 *     archive; gui-ground-truth.md Sec 3.2]:
 *     Chapter 2 "Basic QuickDraw Data Structures" (GrafPort field semantics);
 *     Chapter 4 (CopyBits / transfer modes); Chapter 6 (QDProcs extensibility
 *     table; "You can customize QuickDraw's drawing routines by installing your
 *     own routines in a QDProcs record and pointing the grafProcs field of a
 *     GrafPort to the QDProcs record" -- IWQ Ch 6).
 *
 *   Inside Macintosh: Macintosh Toolbox Essentials (1992) [MTE; free Apple
 *     dev archive; gui-ground-truth.md Sec 3.2]:
 *     Chapter 4 "Window Manager" (visRgn / clipRgn clipping contract;
 *     SetPort / GetPort model; MTE p. 4-15 "The Window Manager sets a
 *     window's visible region to that portion of the window not obscured by
 *     windows in front of it").
 *
 *   ADR-0004 (FLAIR Toolbox Architecture, RATIFIED 2026-06-19):
 *     D-1 (5-layer stack; Layer 1 = surface/GrafPort; the ONE drawing path),
 *     D-2 (GrafPort imaging; one surface module; no second pixel path;
 *          all drawing clipped by visRgn INTERSECT clipRgn; QDProcs seam),
 *     D-3 (verbatim Inside Macintosh records and field names),
 *     OD-2 (indexed-8 offscreen canonical depth; 1 byte/pixel = palette
 *           index; the authentic 8-bpp VGA/SVGA era depth per AM-7).
 *
 *   spec/imaging.h (FLAIR_BitMap, Pattern, flair_xfer_mode_t, RGBColor).
 *   spec/region_algebra.h (region_t for visRgn/clipRgn; rgn_rect_t for
 *     portRect and pnSize/pnLoc coordinates).
 *   os/flair/surface.h (bitmap_t; FLAIR_BitMap.bm -- the ONE pixel buffer).
 *
 * DUAL-COMPILE: freestanding (gcc -m32 -ffreestanding -nostdlib -std=c11)
 * AND hosted (cc -std=c11). Only <stdint.h> + <stddef.h> + local headers.
 * No host malloc; no libc beyond stdint/stddef. Rule 11 (reproducible).
 *
 * ASCII-clean (Rule 12). No nondeterminism / no timestamps (Rule 11).
 * Changing this file is a deliberate, beads-tracked Rule 8 act.
 */
#ifndef INITECH_SPEC_GRAFPORT_H
#define INITECH_SPEC_GRAFPORT_H

#include <stdint.h>
#include <stddef.h>

#include "imaging.h"          /* FLAIR_BitMap, Pattern, flair_xfer_mode_t, RGBColor */
#include "region_algebra.h"   /* region_t, rgn_rect_t                               */

/* ===========================================================================
 * 1. GRAFPORT COORDINATE TYPES
 * ---------------------------------------------------------------------------
 * QuickDraw uses a 2-D coordinate system with int16 values (IM-I p. I-124:
 * "QuickDraw defines points using two integers, a vertical (v) and a
 * horizontal (h) component, both in the range -32768 to 32767"). We reuse
 * int16_t (from region_algebra.h's rgn_rect_t basis) throughout.
 * ===========================================================================*/

/*
 * flair_point_t -- a 2-D point (QuickDraw Point, IM-I Ch 7 p. I-127).
 *   "A point is a location in a coordinate plane."
 *   v = vertical coordinate (row), h = horizontal coordinate (column).
 * Field order v,h matches the verbatim QuickDraw Point record (IM-I p. I-127).
 */
typedef struct flair_point {
    int16_t v;   /* vertical coordinate (row)    -- QuickDraw Point.v */
    int16_t h;   /* horizontal coordinate (column) -- QuickDraw Point.h */
} flair_point_t;

/* ===========================================================================
 * 2. QDProcs -- the QuickDraw customizable drawing-procedure table
 * ---------------------------------------------------------------------------
 * Ref: IWQ Chapter 6 "Customizing QuickDraw Operations":
 *   "You can customize QuickDraw's drawing routines by installing your own
 *    routines in a QDProcs record and pointing the grafProcs field of a
 *    GrafPort to the QDProcs record."
 *   The standard fields in QDProcs (IWQ Ch 6, Table 6-1) are:
 *     textProc, lineProc, rectProc, rRectProc, ovalProc, arcProc,
 *     polyProc, rgnProc, bitsProc, commentProc, txMeasProc, getPicProc,
 *     putPicProc.
 *
 * FLAIR carries the subset that the Toolbox actually dispatches to. For the
 * current release (PRD M3/M4), the LOAD-BEARING seam (ADR-0004 D-2) is the
 * blit/rect/text dispatch that lets the Window Manager and blitter substitute
 * custom clip-masked versions without changing the GrafPort callers. The full
 * QDProcs table is forward-declared as a type; unused slots are NULL and the
 * dispatcher skips them.
 *
 * Forward-declared here; function-pointer types follow the GrafPort typedef
 * to avoid circular references (GrafPort * appears in some signatures).
 * ===========================================================================*/

/* Forward-declare GrafPort so QDProcs function pointers may reference it. */
typedef struct GrafPort GrafPort;

/*
 * QDProcs -- the QuickDraw extensible drawing-proc table (IWQ Ch 6).
 *
 * Each member is a function pointer for the corresponding drawing verb; NULL
 * means "use the default FLAIR implementation." The grafProcs field of a
 * GrafPort points to an instance of this record (or is NULL for defaults).
 *
 * Covered verbs and their roles in FLAIR:
 *
 *   textProc    -- draw text into the current port (the text-draw primitive;
 *                  consulted by the Font Manager path in FLAIR Layer 3).
 *                  Signature: void (*)(int16_t byteCount, const uint8_t *textBuf,
 *                                      flair_point_t numer, flair_point_t denom,
 *                                      GrafPort *port)
 *                  Ref: IWQ Ch 6 (textProc).
 *
 *   rectProc    -- fill or frame a rectangle (FillRect / FrameRect dispatch;
 *                  the primary shape primitive used by chrome and Managers).
 *                  Signature: void (*)(int16_t verb, const rgn_rect_t *r,
 *                                      GrafPort *port)
 *                  verb codes: 0=frame, 1=paint, 2=erase, 3=invert, 4=fill
 *                  Ref: IWQ Ch 6 (rectProc); IM-I p. I-175 verb constants.
 *
 *   rgnProc     -- fill or frame a region (PaintRgn / FillRgn dispatch;
 *                  used by the damage-model repaint and visRgn clipping).
 *                  Signature: void (*)(int16_t verb, const region_t *rgn,
 *                                      GrafPort *port)
 *                  Ref: IWQ Ch 6 (rgnProc).
 *
 *   bitsProc    -- CopyBits / BlitBits dispatch (the bitmap blit primitive;
 *                  the load-bearing path for offscreen-to-LFB compositing and
 *                  the save-under restore on window move, ADR-0004 D-5).
 *                  Signature: void (*)(const FLAIR_BitMap *src,
 *                                      const FLAIR_BitMap *dst,
 *                                      const rgn_rect_t *srcRect,
 *                                      const rgn_rect_t *dstRect,
 *                                      flair_xfer_mode_t mode,
 *                                      const region_t *maskRgn,
 *                                      GrafPort *port)
 *                  Ref: IWQ Ch 4 (CopyBits signature); IWQ Ch 6 (bitsProc).
 *
 *   txMeasProc  -- measure text (returns pixel width of a text run; used by
 *                  the Menu Manager and Dialog Manager for proportional NFNT
 *                  text layout per ADR-0004 D-7).
 *                  Signature: int16_t (*)(int16_t byteCount,
 *                                         const uint8_t *textBuf,
 *                                         flair_point_t *numer,
 *                                         flair_point_t *denom,
 *                                         GrafPort *port)
 *                  Ref: IWQ Ch 6 (txMeasProc).
 *
 * Slots not yet needed for the current release (lineProc, rRectProc,
 * ovalProc, arcProc, polyProc, commentProc, getPicProc, putPicProc) are
 * carried as void* placeholders so sizeof(QDProcs) is stable; they are NULL
 * and never called. This keeps the struct layout deterministic (Rule 11).
 */
typedef struct QDProcs {
    /* textProc: draw byteCount bytes of text at the current pen location.   */
    void     (*textProc)(int16_t byteCount, const uint8_t *textBuf,
                         flair_point_t numer, flair_point_t denom,
                         GrafPort *port);

    /* lineProc: draw a line (reserved; NULL in current release).            */
    void     *lineProc;

    /* rectProc: fill/frame a rectangle.                                     */
    void     (*rectProc)(int16_t verb, const rgn_rect_t *r, GrafPort *port);

    /* rRectProc: draw a rounded rectangle (reserved; NULL in current release)*/
    void     *rRectProc;

    /* ovalProc: draw an oval (reserved; NULL in current release).           */
    void     *ovalProc;

    /* arcProc: draw an arc (reserved; NULL in current release).             */
    void     *arcProc;

    /* polyProc: draw a polygon (reserved; NULL in current release).         */
    void     *polyProc;

    /* rgnProc: fill/frame a region.                                         */
    void     (*rgnProc)(int16_t verb, const region_t *rgn, GrafPort *port);

    /* bitsProc: CopyBits / BlitBits dispatch.                               */
    void     (*bitsProc)(const FLAIR_BitMap *src, const FLAIR_BitMap *dst,
                         const rgn_rect_t *srcRect, const rgn_rect_t *dstRect,
                         flair_xfer_mode_t mode, const region_t *maskRgn,
                         GrafPort *port);

    /* commentProc: picture comment (reserved; NULL in current release).     */
    void     *commentProc;

    /* txMeasProc: measure text width.                                       */
    int16_t  (*txMeasProc)(int16_t byteCount, const uint8_t *textBuf,
                            flair_point_t *numer, flair_point_t *denom,
                            GrafPort *port);

    /* getPicProc / putPicProc: picture I/O (reserved; NULL in current).     */
    void     *getPicProc;
    void     *putPicProc;
} QDProcs;

/* ===========================================================================
 * 3. GrafPort -- the QuickDraw drawing context (verbatim field names)
 * ---------------------------------------------------------------------------
 * Ref: Inside Macintosh Vol I Ch 7 (IM-I) p. I-148 -- "The GrafPort Record":
 *   "All QuickDraw operations use a graphics port as the current drawing
 *    environment. A GrafPort is a complete drawing environment."
 * IWQ Chapter 2 "Basic QuickDraw Data Structures" gives an updated field
 * listing for Color QuickDraw (cgrafPort extension); FLAIR uses the classic
 * GrafPort shape with the Color QuickDraw RGBColor fore/back extension.
 *
 * CLIPPING INVARIANT (ADR-0004 D-1, D-2; MTE Ch 4; Law 4 load-bearing rule):
 *
 *   ALL DRAWING IS CLIPPED BY  visRgn INTERSECT clipRgn.
 *
 *   visRgn is set by the WINDOW MANAGER to the portion of the window not
 *   obscured by windows in front (MTE p. 4-15; ADR-0004 D-5 damage model).
 *   clipRgn is set by the APPLICATION to further restrict drawing within the
 *   port (IM-I p. I-149: "Your drawing will be clipped to clipRgn").
 *   Neither the application nor any Manager may draw outside this intersection.
 *   The surface module (os/flair/surface.h, ADR-0004 D-2) enforces this at
 *   the pixel level -- it is the SOLE conversion site (no second pixel path).
 *
 * SetPort / GetPort contract (IM-I Ch 7):
 *   SetPort(aPort)  -- sets the current GrafPort to aPort; all subsequent
 *                      QuickDraw calls until the next SetPort draw into aPort.
 *   GetPort(&aPort) -- returns a pointer to the current GrafPort.
 *   There is a single GLOBAL current-port pointer in FLAIR's state (not a
 *   per-thread concept -- FLAIR is cooperative/single-threaded, ADR-0004 D-6).
 *   Managers MUST save + restore the current port around their drawing (the
 *   standard IM-I discipline: GetPort(&savedPort); SetPort(myPort); ...draw...;
 *   SetPort(savedPort)). This is a convention, not hardware-enforced.
 *
 * FIELD SURVEY (verbatim IM-I / IWQ names; sources cited per field):
 *
 *   portBits    (FLAIR_BitMap): the destination bitmap.
 *   portRect    (rgn_rect_t):   the port's "bounding rectangle" in local coords.
 *   visRgn      (region_t *):   the visible region (set by Window Manager).
 *   clipRgn     (region_t *):   the application clip region.
 *   pnLoc       (flair_point_t):pen location (current drawing position).
 *   pnSize      (flair_point_t):pen size in pixels (h x v, both >= 1).
 *   pnMode      (flair_xfer_mode_t): pen transfer mode.
 *   pnPat       (Pattern):      pen pattern.
 *   pnVis       (int16_t):      pen visibility level (0 = visible; < 0 = hidden;
 *                               ShowPen increments, HidePen decrements -- IM-I).
 *   rgbFgColor  (RGBColor):     foreground color (Color QuickDraw; IWQ Ch 7).
 *   rgbBkColor  (RGBColor):     background color (Color QuickDraw; IWQ Ch 7).
 *   bkPat       (Pattern):      background pattern (IM-I p. I-149).
 *   fillPat     (Pattern):      fill pattern (IM-I p. I-149; used by FillRect).
 *   txFont      (int16_t):      font ID for text drawing (IM-I p. I-149).
 *   txFace      (uint8_t):      text style (bold/italic/underline bits; IM-I).
 *   txMode      (flair_xfer_mode_t): text transfer mode (IM-I p. I-150).
 *   txSize      (int16_t):      text size in points (IM-I p. I-150; 0 = default).
 *   spExtra     (int16_t):      extra pixels per space in text (IM-I p. I-150;
 *                               Fixed in QuickDraw, int16 in FLAIR's unit --
 *                               see design note below).
 *   grafProcs   (QDProcs *):    pointer to customizable proc table (IWQ Ch 6);
 *                               NULL means use default FLAIR implementations.
 *
 * Design note -- region handles vs pointers:
 *   QuickDraw uses opaque region HANDLES (double-indirected, heap-managed) for
 *   visRgn and clipRgn (IM-I p. I-149). FLAIR does NOT use a heap handle
 *   table: region storage is caller-supplied (ADR-0005 / spec/region_algebra.h,
 *   the arena-backed freestanding model). visRgn and clipRgn are therefore
 *   DIRECT POINTERS (region_t *) to arena-allocated region_t structures whose
 *   storage is owned by the Window Manager (visRgn) or the application
 *   (clipRgn). This is the clean-room departure justified by Law 3
 *   (freestanding; no heap handle manager in the kernel) and Rule 11
 *   (deterministic layout). The QuickDraw NAMES are preserved verbatim; only
 *   the pointer level changes.
 *
 * Design note -- spExtra:
 *   QuickDraw defined spExtra as a Fixed (16.16 signed fixed-point, 4 bytes).
 *   FLAIR uses int16_t (whole pixels of extra space per space character) for
 *   the current release (M3/M4); proportional text layout (ADR-0004 D-7 NFNT)
 *   does not require sub-pixel spacing. This is an acknowledged departure from
 *   verbatim QuickDraw; the field name is preserved.
 *
 * Design note -- txFace:
 *   QuickDraw Style is a SET of style attributes (bold=1, italic=2,
 *   underline=4, outline=8, shadow=16, condense=32, extend=64; IM-I p. I-150).
 *   uint8_t holds all seven bits. The verbatim name is `txFace`.
 * ===========================================================================*/

/* txFace style bit constants (IM-I p. I-150 "Style" type). */
#define FLAIR_STYLE_BOLD       0x01u  /* bold                                */
#define FLAIR_STYLE_ITALIC     0x02u  /* italic                              */
#define FLAIR_STYLE_UNDERLINE  0x04u  /* underline                           */
#define FLAIR_STYLE_OUTLINE    0x08u  /* outline                             */
#define FLAIR_STYLE_SHADOW     0x10u  /* shadow                              */
#define FLAIR_STYLE_CONDENSE   0x20u  /* condense                            */
#define FLAIR_STYLE_EXTEND     0x40u  /* extend                              */

/*
 * GrafPort (verbatim QuickDraw name, IM-I Ch 7 p. I-148):
 *   "A GrafPort is a complete drawing environment, including a destination
 *    bitmap, a visible region, a clipping region, and pen attributes."
 *
 * ALL drawing in FLAIR flows through a GrafPort. The Managers (Layer 3)
 * address ports through SetPort/GetPort and NEVER touch the framebuffer
 * directly (ADR-0004 C-1).
 */
struct GrafPort {
    /* --- Destination bitmap (IM-I p. I-148: "portBits")
     * Ref: IM-I "portBits: BitMap. The port's bit image."
     * FLAIR: the ONE pixel-buffer descriptor (ADR-0004 D-2; surface.h). The
     * destination is the LFB or an indexed-8 offscreen (OD-2). */
    FLAIR_BitMap       portBits;

    /* --- Port bounding rectangle (IM-I p. I-148: "portRect")
     * Ref: IM-I "portRect: Rect. The port rectangle, in local coordinates."
     * The subset of the bitmap this port uses as its drawing area; drawing is
     * clipped to portRect BEFORE visRgn/clipRgn. */
    rgn_rect_t         portRect;

    /* --- Visible region (IM-I p. I-149: "visRgn")
     * Ref: IM-I "visRgn: RgnHandle. The visible region, used to clip output
     *      to the part of the grafPort not hidden by another window."
     * MTE p. 4-15: "The Window Manager sets a window's visible region to
     *      that portion of the window not obscured by windows in front of it."
     * FLAIR: direct pointer (not a handle; see design note above). The Window
     * Manager owns the storage; this pointer is never NULL for an open port. */
    region_t          *visRgn;

    /* --- Application clip region (IM-I p. I-149: "clipRgn")
     * Ref: IM-I "clipRgn: RgnHandle. The clipping region, used to restrict
     *      drawing to a portion of the visible region."
     * FLAIR: direct pointer. The application sets this to further restrict
     * drawing. A NULL clipRgn means "no additional clip" (full visRgn). */
    region_t          *clipRgn;

    /* --- Background pattern (IM-I p. I-149: "bkPat")
     * Ref: IM-I "bkPat: Pattern. The background pattern, used to erase
     *      shapes and to define the background of a bit map."
     * Used by EraseRect/EraseRgn and background fills. */
    Pattern            bkPat;

    /* --- Fill pattern (IM-I p. I-149: "fillPat")
     * Ref: IM-I "fillPat: Pattern. The fill pattern, used with the fill
     *      drawing verbs (FillRect, FillRgn, etc.)."                        */
    Pattern            fillPat;

    /* --- Pen location (IM-I p. I-149: "pnLoc")
     * Ref: IM-I "pnLoc: Point. The pen's current location in local
     *      coordinates."
     * MoveTo, LineTo, DrawChar, etc. start from pnLoc.                      */
    flair_point_t      pnLoc;

    /* --- Pen size (IM-I p. I-149: "pnSize")
     * Ref: IM-I "pnSize: Point. The pen's width and height in pixels."
     * pnSize.h = width, pnSize.v = height; both must be >= 1 (IM-I p. I-154
     * "The pen size must be a positive value"). */
    flair_point_t      pnSize;

    /* --- Pen transfer mode (IM-I p. I-149: "pnMode")
     * Ref: IM-I "pnMode: Integer. The pen's current transfer mode."
     * governs how line/shape drawing combines with the destination. */
    flair_xfer_mode_t  pnMode;

    /* --- Pen pattern (IM-I p. I-149: "pnPat")
     * Ref: IM-I "pnPat: Pattern. The pen's pattern."
     * Applied via pnMode when stroking lines and frame shapes. */
    Pattern            pnPat;

    /* --- Pen visibility (IM-I p. I-149: "pnVis")
     * Ref: IM-I "pnVis: Integer. The pen's visibility level."
     * 0 = visible; < 0 = hidden. HidePen decrements, ShowPen increments.
     * Drawing is suppressed when pnVis < 0 (IM-I p. I-154). */
    int16_t            pnVis;

    /* --- Text font (IM-I p. I-149: "txFont")
     * Ref: IM-I "txFont: Integer. The font number for text."
     * 0 = systemFont (Chicago 12; FLAIR default). Ref: IM-I "Font Numbers"
     * (systemFont=0, applFont=1; ADR-0004 D-7). */
    int16_t            txFont;

    /* --- Text face / style (IM-I p. I-150: "txFace")
     * Ref: IM-I "txFace: Style. The style of text."
     * Bit flags: see FLAIR_STYLE_* constants above (IM-I p. I-150 "Style"). */
    uint8_t            txFace;

    /* Padding to keep pnMode alignment consistent (txFace is 1 byte; the
     * next field txMode is an enum / int; align to natural boundary). */
    uint8_t            _pad_txFace;

    /* --- Text transfer mode (IM-I p. I-150: "txMode")
     * Ref: IM-I "txMode: Integer. The transfer mode for text."
     * Governs how text pixels combine with the destination; srcOr is the
     * normal text-draw mode (IM-I p. I-177). */
    flair_xfer_mode_t  txMode;

    /* --- Text size (IM-I p. I-150: "txSize")
     * Ref: IM-I "txSize: Integer. The size of text in points."
     * 0 means use the font's preferred size. */
    int16_t            txSize;

    /* --- Extra pixels per space (IM-I p. I-150: "spExtra")
     * Ref: IM-I "spExtra: Fixed. The amount of extra space for each space
     *      character during text drawing."
     * FLAIR uses int16_t (whole pixels) for M3/M4 (design note above). */
    int16_t            spExtra;

    /* --- Foreground color (IWQ Ch 7: "rgbFgColor")
     * Ref: IWQ "rgbFgColor: RGBColor. The foreground color used for
     *      drawing."
     * Classic QuickDraw called this fgColor (Integer; one of the classic
     * QD color constants). Color QuickDraw (IM-V / IWQ) renamed to
     * rgbFgColor and uses the full RGBColor. FLAIR targets indexed-8 color
     * (OD-2) and uses RGBColor mapped through the palette. */
    RGBColor           rgbFgColor;

    /* --- Background color (IWQ Ch 7: "rgbBkColor")
     * Ref: IWQ "rgbBkColor: RGBColor. The background color used for
     *      erasing and background fills." */
    RGBColor           rgbBkColor;

    /* --- QDProcs customization table (IWQ Ch 6: "grafProcs")
     * Ref: IWQ "grafProcs: QDProcsPtr. A pointer to a QDProcs record
     *      containing customized low-level drawing routines; NIL means
     *      use the default drawing routines."
     * This is the standard QuickDraw extensibility seam (ADR-0004 D-2).
     * The Window Manager and the blitter install custom procs here to
     * inject region-clipped implementations without changing port callers.
     * NULL = use FLAIR defaults. */
    QDProcs           *grafProcs;
};

/* ===========================================================================
 * 4. SetPort / GetPort -- current-port model
 * ---------------------------------------------------------------------------
 * Ref: Inside Macintosh Vol I Ch 7 (IM-I) "Routines" (p. I-158 ff):
 *   SetPort(aGrafPtr): "Set the current grafPort to aGrafPtr."
 *   GetPort(aGrafPtr): "Return (via aGrafPtr) a pointer to the current port."
 *
 * CONTRACT (FLAIR implementation note, not a callable here -- prototypes live
 * in the Layer 1 drawing module, not in locked spec-data):
 *
 *   - There is ONE global current-port pointer in FLAIR state (single-
 *     threaded cooperative OS, ADR-0004 D-6; no per-thread port context).
 *   - Every Manager that draws MUST save + restore the port:
 *       GrafPort *savedPort;
 *       GetPort(&savedPort);
 *       SetPort(myPort);
 *       ...draw...
 *       SetPort(savedPort);
 *     (IM-I discipline; enforced by convention in cooperative single-thread.)
 *   - Drawing is FORBIDDEN before SetPort has been called at least once
 *     (the global current port is NULL at boot until the desktop port is
 *     opened by the Desktop Shell, Layer 5, ADR-0004 D-1).
 *   - A port is "open" when its visRgn / clipRgn pointers are non-NULL and
 *     its portBits.bm is initialized (base, pitch, bpp, width, height valid).
 *
 * CLIPPING INVARIANT (ADR-0004 D-1 / D-2; MTE Ch 4) -- load-bearing rule:
 *
 *   EVERY PIXEL WRITE IS CLIPPED TO  visRgn INTERSECT clipRgn.
 *
 *   If grafProcs->bitsProc / rectProc / rgnProc / textProc is non-NULL, the
 *   custom proc is responsible for enforcing the clip (it receives the port).
 *   If NULL (defaults), the FLAIR surface module enforces the clip before
 *   calling surface_put_pixel / surface_fill_span / surface_blit.
 *   Violation of this invariant is a FAIL-LOUD panic (Rule 2) detectable by
 *   the fb-agree oracle (ADR-0004 D-8, FO-1 / AM-2).
 * ===========================================================================*/

/* ===========================================================================
 * 5. COMPILE-TIME CONTRACT CHECKS  (the oracle bites at build time)
 * ---------------------------------------------------------------------------
 * Style follows spec/region_algebra.h (the style exemplar per the brief).
 * ===========================================================================*/

/* flair_point_t is 2 x int16_t = 4 bytes; field order v,h (IM-I). */
_Static_assert(sizeof(flair_point_t) == 4,
               "flair_point_t must be 2 x int16_t = 4 bytes (QuickDraw Point)");
_Static_assert(offsetof(flair_point_t, v) == 0,
               "flair_point_t.v (vertical) must be at offset 0 (IM-I)");
_Static_assert(offsetof(flair_point_t, h) == 2,
               "flair_point_t.h (horizontal) must be at offset 2 (IM-I)");

/* GrafPort contains the required QuickDraw fields (presence checks). */
_Static_assert(offsetof(GrafPort, portBits) == 0,
               "GrafPort.portBits must be the first field (IM-I p. I-148)");
_Static_assert(offsetof(GrafPort, visRgn) > offsetof(GrafPort, portRect),
               "GrafPort.visRgn must follow portRect (IM-I field order)");
_Static_assert(offsetof(GrafPort, clipRgn) > offsetof(GrafPort, visRgn),
               "GrafPort.clipRgn must follow visRgn (IM-I field order)");
_Static_assert(offsetof(GrafPort, pnLoc) > offsetof(GrafPort, clipRgn),
               "GrafPort pen state must follow clip regions (IM-I field order)");
_Static_assert(offsetof(GrafPort, grafProcs) > offsetof(GrafPort, rgbBkColor),
               "GrafPort.grafProcs must follow color fields (IWQ Ch 6 ordering)");

/* pnSize and pnLoc are flair_point_t (4 bytes each). */
_Static_assert(sizeof(((GrafPort *)0)->pnLoc) == 4,
               "GrafPort.pnLoc must be flair_point_t (4 bytes)");
_Static_assert(sizeof(((GrafPort *)0)->pnSize) == 4,
               "GrafPort.pnSize must be flair_point_t (4 bytes)");

/* pen pattern and background pattern are Pattern (8 bytes each). */
_Static_assert(sizeof(((GrafPort *)0)->pnPat) == 8,
               "GrafPort.pnPat must be Pattern (8 bytes; IM-I Ch 7)");
_Static_assert(sizeof(((GrafPort *)0)->bkPat) == 8,
               "GrafPort.bkPat must be Pattern (8 bytes; IM-I Ch 7)");
_Static_assert(sizeof(((GrafPort *)0)->fillPat) == 8,
               "GrafPort.fillPat must be Pattern (8 bytes; IM-I Ch 7)");

/* RGBColor fore/back are 6 bytes each (3 x uint16_t; IM-V). */
_Static_assert(sizeof(((GrafPort *)0)->rgbFgColor) == 6,
               "GrafPort.rgbFgColor must be RGBColor (6 bytes; IM-V)");
_Static_assert(sizeof(((GrafPort *)0)->rgbBkColor) == 6,
               "GrafPort.rgbBkColor must be RGBColor (6 bytes; IM-V)");

/* visRgn and clipRgn are pointers (not handles); pointer size on the target
 * is 4 bytes (32-bit flat, ADR-0001). On the host these checks adapt to
 * sizeof(void *) so the header compiles clean on 64-bit hosts too. */
_Static_assert(sizeof(((GrafPort *)0)->visRgn) == sizeof(void *),
               "GrafPort.visRgn must be a pointer (region_t *)");
_Static_assert(sizeof(((GrafPort *)0)->clipRgn) == sizeof(void *),
               "GrafPort.clipRgn must be a pointer (region_t *)");

/* grafProcs is a pointer to QDProcs (IWQ Ch 6; may be NULL). */
_Static_assert(sizeof(((GrafPort *)0)->grafProcs) == sizeof(void *),
               "GrafPort.grafProcs must be a QDProcs pointer");

/* txFace style bits are within uint8_t (all 7 known bits fit). */
_Static_assert((FLAIR_STYLE_BOLD | FLAIR_STYLE_ITALIC | FLAIR_STYLE_UNDERLINE |
                FLAIR_STYLE_OUTLINE | FLAIR_STYLE_SHADOW |
                FLAIR_STYLE_CONDENSE | FLAIR_STYLE_EXTEND) <= 0xFFu,
               "All txFace style bits must fit in uint8_t (IM-I 7 bits)");

/* QDProcs contains the five load-bearing proc slots (size > 0). */
_Static_assert(sizeof(QDProcs) > 0,
               "QDProcs must have non-zero size (contains proc table)");
_Static_assert(offsetof(QDProcs, bitsProc) > offsetof(QDProcs, textProc),
               "QDProcs.bitsProc must follow textProc (IWQ Ch 6 table order)");
_Static_assert(offsetof(QDProcs, txMeasProc) > offsetof(QDProcs, bitsProc),
               "QDProcs.txMeasProc must follow bitsProc (IWQ Ch 6)");

#endif /* INITECH_SPEC_GRAFPORT_H */
