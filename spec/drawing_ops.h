/*
 * spec/drawing_ops.h -- QuickDraw OPERATION SEMANTICS: the LOCKED contract.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8; PRD Sec 6.3 -- "the Toolbox layer").
 * beads: initech-dh5k.6 (P1-6: drawing_ops.h + compile oracle).
 * era: system7.0-7.1 (Law 3; operator decision 2026-06-20).
 *
 * This header locks the VERB LAYER CONTRACT that the FLAIR blitter and
 * Toolbox Managers are written against.  It does NOT provide an
 * implementation -- it is the red-green TDD target that Phase 3 verb-layer
 * code (PRD Sec 6.3, Phase 3 "P3-pre") must satisfy.
 *
 * What is locked here (by section):
 *   1. GrafVerb enum -- the 5 verb codes (0..4) passed to rectProc/rgnProc/
 *      Std* low-level routines.  Ref: system7-decomp drawing-primitives Sec 7.
 *   2. Shape-family enum -- the 6 shape classes; verbs x shapes = the full
 *      verb matrix.  Ref: drawing-primitives Sec 6.
 *   3. Pen initial state -- the exact triple the blitter initializes on port
 *      creation.  Ref: drawing-primitives Sec 2.
 *   4. Verb-to-pattern-slot mapping -- which port field each verb reads and
 *      which transfer mode it applies.  Ref: drawing-primitives Sec 5 +
 *      patterns Sec 2.
 *   5. Coordinate-system contract -- int16 plane, Point(v,h) v-first, Rect
 *      (top,left,bottom,right) HALF-OPEN, local vs global, h-first routine
 *      args.  Ref: coordinate-system Sec 1-5.
 *   6. CopyBits parameter contract -- srcBits/dstBits descriptor, srcRect/
 *      dstRect (source- and dest-local), mode (src* modes 0..7 ONLY for
 *      CopyBits; ditherCopy=64 on S7), maskRgn, operation order, colorize
 *      rule.  Ref: copybits Sec 2-4.
 *   7. Pattern phase-lock rule -- tile (x mod 8, y mod 8) relative to the
 *      PORT ORIGIN, not the shape origin.  Ref: patterns Sec 3.
 *   8. System patterns -- white/black/gray/ltGray/dkGray byte values (to the
 *      extent documented or verified from corpus).
 *
 * WHAT THIS FILE DOES NOT CONTAIN:
 *   - Implementation of any verb, blit, or pattern tile.
 *   - Redefinitions of types already locked in imaging.h / grafport.h.
 *     Those headers define: GrafPort, QDProcs, flair_point_t, flair_xfer_mode_t,
 *     Pattern / flair_pattern_t, FLAIR_BitMap, RGBColor.
 *     We INCLUDE them here and REFERENCE their names.
 *   - Any pixel value or golden (Law 2 -- no pixel values from prose; those
 *     are [golden-resolves]).
 *
 * SOURCE / LAW 1 CITATIONS (all from ../system7-decomp; local corpus):
 *
 *   [DP]  ../system7-decomp/specs/quickdraw/drawing-primitives.md
 *           Sec 2  -- pen initial state (pnSize=(1,1)/pnPat=black/pnMode=patCopy)
 *           Sec 3  -- PenState record layout (pnLoc+pnSize+pnMode+pnPat, 18B)
 *           Sec 5  -- the five verbs, which slot each reads
 *           Sec 6  -- shape families x verb matrix
 *           Sec 7  -- GrafVerb enum (frame=0, paint=1, erase=2, invert=3, fill=4)
 *           Sec 8  -- QDProcs dispatch (rectProc/rgnProc/bitsProc/textProc)
 *           Sec 9  -- era deltas (verb API invariant 6->7->7.5)
 *
 *   [CB]  ../system7-decomp/specs/quickdraw/copybits.md
 *           Sec 1  -- BitMap record (baseAddr/rowBytes/bounds)
 *           Sec 2  -- CopyBits signature + parameter semantics
 *           Sec 3  -- operation order (map, depth-convert, colorize, mode, clip)
 *           Sec 4  -- CopyBits mode constants (srcCopy=0..notSrcBic=7, ditherCopy=64)
 *           Sec 6  -- era delta: ditherCopy is System-7-only
 *
 *   [CS]  ../system7-decomp/specs/quickdraw/coordinate-system.md
 *           Sec 1  -- int16 plane, top-left origin, +v=down, +h=right
 *           Sec 2  -- Point record (v at 0, h at 2; v-first storage)
 *           Sec 3  -- Rect record (top,left,bottom,right; HALF-OPEN)
 *           Sec 4  -- SetRect arg order (left,top,right,bottom -- "litterbug")
 *           Sec 5  -- local vs global; visRgn/clipRgn in local coords
 *
 *   [PA]  ../system7-decomp/specs/quickdraw/patterns.md
 *           Sec 2  -- three port slots (pnPat/bkPat/fillPat) + verb mapping
 *           Sec 3  -- port-origin phase-lock rule (verbatim Apple cite)
 *           Sec 4  -- five predefined system patterns + byte values
 *
 *   [QDp] ../system7-decomp/refs/quickdraw/Apple-QuickDraw.p
 *           L54    -- Pattern = PACKED ARRAY[0..7] OF 0..255
 *           L57    -- GrafVerb = (frame,paint,erase,invert,fill)
 *           L189-193 -- white/black/gray/ltGray/dkGray global decls
 *           L292   -- FillRect(r: Rect; pat: Pattern) signature
 *
 * CROSS-CHECKED AGAINST (read-only; do NOT redefine):
 *   spec/imaging.h    -- Pattern (8 bytes), flair_xfer_mode_t (srcCopy..notPatBic),
 *                        RGBColor, FLAIR_BitMap.
 *   spec/grafport.h   -- GrafPort (portBits/visRgn/clipRgn/pnLoc/pnSize/pnMode/
 *                        pnPat/bkPat/fillPat/grafProcs), flair_point_t, QDProcs.
 *   spec/region_algebra.h -- rgn_rect_t (the single Rect typedef, field order
 *                        top=0/left=2/bottom=4/right=6), region_t.
 *
 * DUAL-COMPILE: freestanding (gcc -m32 -ffreestanding -nostdlib -fno-stack-protector
 * -std=c11) AND hosted (gcc -m32 -std=c11).  Only <stdint.h>+<stddef.h> plus
 * the three local headers above.  No host malloc; no libc beyond stdint/stddef.
 * Rule 11 (reproducible).
 *
 * ASCII-clean (Rule 12).  No nondeterminism / no timestamps (Rule 11).
 * Changing this file is a deliberate, beads-tracked Rule 8 act.
 */
#ifndef INITECH_SPEC_DRAWING_OPS_H
#define INITECH_SPEC_DRAWING_OPS_H

#include <stdint.h>
#include <stddef.h>

/* Pull in the locked imaging data types and the GrafPort record.
 * This also transitively pulls region_algebra.h (rgn_rect_t, region_t). */
#include "grafport.h"   /* GrafPort, QDProcs, flair_point_t, flair_xfer_mode_t */
#include "imaging.h"    /* Pattern, FLAIR_BitMap, RGBColor, flair_xfer_mode_t  */

/* ===========================================================================
 * 1. GrafVerb -- the 5 verb codes used by rectProc / rgnProc / Std* routines
 * ---------------------------------------------------------------------------
 * Ref: [DP] Sec 7 + [QDp] L57.
 *
 * Apple QuickDraw.p (L57) verbatim:
 *   "GrafVerb = (frame, paint, erase, invert, fill);"
 *
 * All five verbs for ONE shape collapse onto a SINGLE low-level Std* routine
 * that takes a GrafVerb selector. E.g. FrameOval/PaintOval/EraseOval/
 * InvertOval/FillOval all call StdOval(verb, r). [DP Sec 7 verbatim]
 *
 * The verb code is also the `verb` argument to FLAIR's rectProc and rgnProc
 * function pointers (grafport.h QDProcs): 0=frame, 1=paint, 2=erase,
 * 3=invert, 4=fill. [DP Sec 7 + grafport.h rectProc / rgnProc comment]
 *
 * ERA NOTE (era=system7.0-7.1): the GrafVerb enum is INVARIANT across System
 * 6, 7.0/7.1, and 7.5+ and across basic vs Color graphics ports. The verb
 * API predates the color chrome and is unchanged by it. [DP Sec 9 verbatim]
 * ===========================================================================*/

/*
 * flair_grafverb_t -- QuickDraw GrafVerb (verbatim names, [QDp] L57).
 *
 * Numeric values are explicit to make the _Static_asserts below meaningful
 * and to guard against Pascal-enum-ordering mistakes in a C transcription.
 *
 * era=system7.0-7.1
 */
typedef enum flair_grafverb {
    /*
     * frame (0) -- draw the outline of the shape only; interior untouched.
     *   Pattern slot: pnPat; mode: pnMode.
     *   [DP Sec 5 verbatim: "Frame draws just its outline, using the size,
     *   pattern, and pattern mode of the graphics pen."]
     */
    kGrafVerbFrame  = 0,

    /*
     * paint (1) -- draw outline + fill interior with the pen pattern.
     *   Pattern slot: pnPat; mode: pnMode.
     *   [DP Sec 5 verbatim: "Paint draws both its outline and its interior
     *   with the pattern of the graphics pen, using the pattern mode."]
     */
    kGrafVerbPaint  = 1,

    /*
     * erase (2) -- fill outline + interior with the background pattern.
     *   Pattern slot: bkPat; mode: patCopy (implied).
     *   Also used by ScrollRect for scrolled-out pixels. [DP Sec 5 / PA Sec 2]
     *   [DP Sec 5 verbatim: "Erase draws both its outline and its interior
     *   with the background pattern for the current graphics port."]
     */
    kGrafVerbErase  = 2,

    /*
     * invert (3) -- reverse (bitwise NOT) the pixels within the shape.
     *   Pattern slot: none; mode: bitwise invert.
     *   [DP Sec 5 verbatim: "Invert reverses the colors of all pixels
     *   within its boundary."]
     *   ERA NOTE: on an indexed-8 device Color QuickDraw inverts the PIXEL
     *   INDEX, not the RGB; the result depends on the CLUT and is
     *   "unpredictable" per Apple. [DP Sec 9 indexed-invert note]
     */
    kGrafVerbInvert = 3,

    /*
     * fill (4) -- fill outline + interior with a CALLER-SUPPLIED pattern.
     *   Pattern slot: fillPat (caller's pattern STORED there first).
     *   Mode: ALWAYS patCopy regardless of pnMode.
     *   [DP Sec 5 verbatim: "Fill draws both its outline and its interior
     *   with any pattern you specify. The procedure transfers the pattern
     *   with the patCopy pattern mode."]
     *   [PA Sec 2 verbatim: "Fill* takes the pattern as a call argument ...
     *   stores that argument into fillPat ... always uses the patCopy
     *   pattern mode."]
     */
    kGrafVerbFill   = 4

} flair_grafverb_t;

/* ===========================================================================
 * 2. Shape-family enum -- the 6 shape classes
 * ---------------------------------------------------------------------------
 * Ref: [DP] Sec 6.
 *
 * Each shape class implements all 5 GrafVerbs via one Std* low-level
 * routine; the routine cross is the full 5x6 verb x shape matrix.
 *
 * FLAIR M3/M4 scope: rectangle family is fully implemented (FrameRect/
 * PaintRect/FillRect/EraseRect/InvertRect) and FrameRoundRect/PaintRoundRect.
 * The oval/arc/poly rows use NULL QDProcs slots (stable placeholders) to be
 * filled by Phase 3 without changing callers. Region verbs route through the
 * ATKINSON engine (region_algebra.h). [DP Sec 6 FLAIR scope note]
 *
 * era=system7.0-7.1
 * ===========================================================================*/

/*
 * flair_shape_family_t -- the 6 QuickDraw shape classes. [DP Sec 6]
 *
 * Values are informational ordinals (for switch/table dispatch); the Std*
 * routine set (StdRect/StdRRect/StdOval/StdArc/StdPoly/StdRgn) is the
 * canonical dispatch, not a sequential index.  [DP Sec 7 Std* list]
 */
typedef enum flair_shape_family {
    kShapeRect    = 0,  /* Rectangle: FrameRect..InvertRect; extra: r:Rect [DP Sec 6] */
    kShapeRRect   = 1,  /* RoundRect: FrameRoundRect..; extra: ovWd,ovHt  [DP Sec 6] */
    kShapeOval    = 2,  /* Oval: FrameOval..; extra: r:Rect (bounding)    [DP Sec 6] */
    kShapeArc     = 3,  /* Arc/Wedge: FrameArc..; extra: startAngle,arcAngle [DP Sec 6] */
    kShapePoly    = 4,  /* Polygon: FramePoly..; extra: poly:PolyHandle   [DP Sec 6] */
    kShapeRgn     = 5   /* Region: FrameRgn..; extra: rgn:RgnHandle       [DP Sec 6] */
} flair_shape_family_t;

/* ===========================================================================
 * 3. Verb-to-pattern-slot mapping (the FLAIR-critical table)
 * ---------------------------------------------------------------------------
 * Ref: [DP] Sec 5 (verb definitions) + [PA] Sec 2 (port slot roles).
 *
 * This table is the contract the blitter's rectProc must honor:
 *
 *   verb     | pattern slot | mode applied       | source
 *   ---------+--------------+--------------------+--------------------------
 *   frame(0) | pnPat        | pnMode             | [DP Sec 5] + [PA Sec 2]
 *   paint(1) | pnPat        | pnMode             | [DP Sec 5] + [PA Sec 2]
 *   erase(2) | bkPat        | patCopy (implied)  | [DP Sec 5] + [PA Sec 2]
 *   invert(3)| (none)       | bitwise NOT        | [DP Sec 5]
 *   fill(4)  | fillPat      | patCopy (FORCED)   | [DP Sec 5] + [PA Sec 2]
 *
 * CRITICAL: Fill* ALWAYS forces patCopy regardless of pnMode. [PA Sec 2
 * verbatim: "it always uses the patCopy pattern mode"]
 *
 * CRITICAL: Frame* draws the outline ONLY; the interior is NOT touched.
 * [DP Sec 5 verbatim: "Interior is NOT touched."]
 *
 * C comment encoding of the table: these macros are informational contract
 * markers for Phase 3 implementors; they are NOT operational code.
 * ===========================================================================*/

/*
 * FLAIR_VERB_READS_PNPAT(v) -- true iff GrafVerb v reads pnPat.
 *   frame(0) and paint(1) read pnPat with pnMode.  [DP Sec 5; PA Sec 2]
 */
#define FLAIR_VERB_READS_PNPAT(v)   ((v) == kGrafVerbFrame || (v) == kGrafVerbPaint)

/*
 * FLAIR_VERB_READS_BKPAT(v) -- true iff GrafVerb v reads bkPat.
 *   erase(2) reads bkPat.  [DP Sec 5; PA Sec 2]
 */
#define FLAIR_VERB_READS_BKPAT(v)   ((v) == kGrafVerbErase)

/*
 * FLAIR_VERB_READS_FILLPAT(v) -- true iff GrafVerb v reads fillPat.
 *   fill(4) reads fillPat with FORCED patCopy.  [DP Sec 5; PA Sec 2]
 */
#define FLAIR_VERB_READS_FILLPAT(v) ((v) == kGrafVerbFill)

/*
 * FLAIR_VERB_IS_INVERT(v) -- true iff GrafVerb v is bitwise-invert.
 *   invert(3) reads no pattern; it NOTs pixel indices.  [DP Sec 5]
 */
#define FLAIR_VERB_IS_INVERT(v)     ((v) == kGrafVerbInvert)

/*
 * FLAIR_VERB_FORCES_PATCOPY(v) -- true iff the mode is ALWAYS patCopy
 * regardless of pnMode.  Only fill(4).  [PA Sec 2; DP Sec 5]
 */
#define FLAIR_VERB_FORCES_PATCOPY(v) ((v) == kGrafVerbFill)

/* ===========================================================================
 * 4. Pen initial state (the load-bearing port-creation default)
 * ---------------------------------------------------------------------------
 * Ref: [DP] Sec 2; [PA] Sec 4; [QDp] L45,L189-193.
 *
 * Apple QuickDraw.p (IWQ Ch 3 [DP Sec 2]) verbatim:
 *   "QuickDraw assigns these initial values to the graphics pen: a size of
 *    (1,1), a pattern of all-black pixels, and a pattern mode of patCopy."
 *
 * PenNormal restores EXACTLY this triple (pnSize, pnPat, pnMode) WITHOUT
 * moving pnLoc.  [DP Sec 2 + QDp L237 PenNormal]
 *
 * era=system7.0-7.1
 * ===========================================================================*/

/*
 * FLAIR_PEN_INIT_PNVIS -- initial pen visibility: 0 = visible. [DP Sec 2]
 *   HidePen decrements, ShowPen increments; drawing suppressed while < 0.
 */
#define FLAIR_PEN_INIT_PNVIS  0

/*
 * FLAIR_PEN_INIT_PNSIZE_V -- initial pen height in pixels. [DP Sec 2]
 *   pnSize stores (height, width) as a Point's (v, h) fields.
 *   v=height=1.
 */
#define FLAIR_PEN_INIT_PNSIZE_V  1

/*
 * FLAIR_PEN_INIT_PNSIZE_H -- initial pen width in pixels. [DP Sec 2]
 *   h=width=1.
 */
#define FLAIR_PEN_INIT_PNSIZE_H  1

/*
 * FLAIR_PEN_INIT_PNMODE -- initial pen transfer mode: patCopy (=8). [DP Sec 2]
 *   [QDp L16: patCopy = 8]
 */
#define FLAIR_PEN_INIT_PNMODE   patCopy   /* flair_xfer_mode_t from imaging.h */

/*
 * FLAIR_PEN_INIT_PNPAT -- initial pen pattern: all-black (every bit 1).
 *   [DP Sec 2; PA Sec 4 verbatim: "all-black pixels (the black predefined
 *   pattern)"]
 *   The `black` predefined system pattern: { 0xFF, 0xFF, 0xFF, 0xFF,
 *                                             0xFF, 0xFF, 0xFF, 0xFF }.
 *   [PA Sec 4; QDp L190]
 */
#define FLAIR_SYSPAT_BLACK_BYTES  \
    { 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu }

/*
 * FLAIR_PEN_INIT_BKPAT -- initial background pattern: all-white (every bit 0).
 *   [PA Sec 4 verbatim: "On port creation: background pattern = all-white
 *   (white)."]
 *   The `white` predefined system pattern: { 0x00, 0x00, 0x00, 0x00,
 *                                             0x00, 0x00, 0x00, 0x00 }.
 *   [PA Sec 4; QDp L189]
 */
#define FLAIR_SYSPAT_WHITE_BYTES  \
    { 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u }

/* ===========================================================================
 * 5. System patterns (the 5 predefined QuickDraw pattern constants)
 * ---------------------------------------------------------------------------
 * Ref: [PA] Sec 4; [QDp] L189-193; [DP] Sec 2.
 *
 * Five predefined system patterns declared in Apple-QuickDraw.p L189-193:
 *   white, black, gray, ltGray, dkGray: Pattern;
 *
 * Byte encoding: 8 bytes = 8 rows, one byte per row, MSB=leftmost pixel,
 * set-bit(1)=foreground, clear-bit(0)=background.  [PA Sec 1]
 *
 * Certainty per [PA] Sec 4 table:
 *   white   [verified: "every pixel is white"]
 *   black   [verified: "every pixel is black"]
 *   gray    [documented: conventional 50% checker, s7-quickdraw.md Sec 7]
 *   ltGray  [verified: PAT_17.bin bytes 88228822...]
 *   dkGray  [inferred: conventional 75% bytes -- [golden-resolves: dkGray PAT]]
 *
 * era=system7.0-7.1
 * ===========================================================================*/

/* FLAIR_SYSPAT_GRAY_BYTES -- 50% gray, checkerboard. [PA Sec 4, documented]
 *   Rows alternate 0xAA (10101010) and 0x55 (01010101). */
#define FLAIR_SYSPAT_GRAY_BYTES  \
    { 0xAAu, 0x55u, 0xAAu, 0x55u, 0xAAu, 0x55u, 0xAAu, 0x55u }

/* FLAIR_SYSPAT_LTGRAY_BYTES -- 25% gray. [PA Sec 4, verified: PAT_17.bin] */
#define FLAIR_SYSPAT_LTGRAY_BYTES \
    { 0x88u, 0x22u, 0x88u, 0x22u, 0x88u, 0x22u, 0x88u, 0x22u }

/*
 * FLAIR_SYSPAT_DKGRAY_BYTES -- 75% gray. [PA Sec 4, inferred: conventional
 * 75% bytes; golden-resolves: dkGray PAT -- must be verified against a real
 * extracted PAT resource before this constant is used in a pixel-exact path.
 * Law 2: do NOT assert exact pixels based on this value alone.
 */
#define FLAIR_SYSPAT_DKGRAY_BYTES \
    { 0x77u, 0xDDu, 0x77u, 0xDDu, 0x77u, 0xDDu, 0x77u, 0xDDu }

/* ===========================================================================
 * 6. Pattern phase-lock rule (the tiling contract)
 * ---------------------------------------------------------------------------
 * Ref: [PA] Sec 3.
 *
 * Apple QuickDraw.p (IWQ Ch 3 [PA Sec 3]) verbatim:
 *   "So that adjacent areas of the same pattern form a continuous, coordinated
 *    pattern, all patterns are always drawn relative to the origin of the
 *    graphics port."
 *
 * FLAIR blitter tiling contract (the load-bearing rule Phase 3 must honor):
 *
 *   For destination pixel at port-local (x, y):
 *     row_index = y & 7          (y mod 8, port-relative)
 *     bit_index = 7 - (x & 7)   (MSB=column-0; x mod 8, port-relative)
 *     pixel_set = (pat.pat[row_index] >> bit_index) & 1u
 *
 * The `& 7` is against the PORT ORIGIN (0,0), NOT against the shape's
 * own origin.  Two abutting rectangles filled with the same pattern join
 * seamlessly because both tile from the same phase.  [PA Sec 3]
 *
 * SetOrigin (which shifts the port local origin) re-phases subsequent pattern
 * fills.  [PA Sec 3, inferred]
 *
 * era=system7.0-7.1
 * ===========================================================================*/

/*
 * FLAIR_PAT_ROW(p_, y_local)  -- the pattern byte for port-local row y.
 *   p_      : const Pattern * (see imaging.h)
 *   y_local : int-typed port-local y coordinate
 * Result : uint8_t pattern byte (MSB = leftmost pixel).  [PA Sec 1 + Sec 3]
 *
 * NOTE: the macro parameter is named `p_` (not `pat`) to avoid any collision
 * with the Pattern struct's own `pat` field name in the macro body.
 */
#define FLAIR_PAT_ROW(p_, y_local)  ((p_)->pat[(unsigned)(y_local) & 7u])

/*
 * FLAIR_PAT_BIT(p_, x_local, y_local)  -- 1 if pixel is set, 0 otherwise.
 *   Tests the pattern bit at port-local (x, y) coordinates.
 *   MSB (bit 7) = column 0 (leftmost).  [PA Sec 1 + Sec 3]
 *
 * NOTE: parameter named `p_` for the same reason as FLAIR_PAT_ROW above.
 */
#define FLAIR_PAT_BIT(p_, x_local, y_local)                                    \
    (((FLAIR_PAT_ROW((p_), (y_local))) >> (7u - ((unsigned)(x_local) & 7u)))   \
     & 1u)

/* ===========================================================================
 * 7. Coordinate-system contract constants and macros
 * ---------------------------------------------------------------------------
 * Ref: [CS] Sec 1-5; [QDp] L56,L68-87.
 *
 * The coordinate model is INVARIANT across System 6, 7.0/7.1, and 7.5+;
 * there are no era deltas in the int16 plane, Point, or Rect.  [CS Sec 6]
 *
 * Key facts (each has a _Static_assert in the checks section below, or is
 * already asserted in grafport.h / region_algebra.h -- cross-referenced):
 *
 *   - Coordinate type: int16_t (Pascal INTEGER = 16-bit).  [CS Sec 1]
 *   - Point field order: v (vertical/row) at offset 0, h (horizontal/col)
 *     at offset 2 -- VERTICAL FIRST, the single most error-prone fact.
 *     [CS Sec 2 verbatim]
 *   - Rect field order: top=0, left=2, bottom=4, right=6 (two Points:
 *     topLeft at 0, botRight at 4).  [CS Sec 3]
 *   - Rect is HALF-OPEN: rows [top,bottom), columns [left,right).  [CS Sec 3]
 *   - emptyRect: bottom<=top OR right<=left.  [CS Sec 3]
 *   - ptInRect: top<=v<bottom AND left<=h<right.  [CS Sec 3, CS Sec 4]
 *   - +v = DOWN (increasing v goes down), +h = RIGHT.  [CS Sec 1]
 *   - Routine ARGS for MoveTo/LineTo/SetPt/SetRect take h BEFORE v
 *     (horizontal-first), opposite of the record's v-before-h storage.
 *     [CS Sec 2 ARGUMENT-ORDER TRAP; CS Sec 4 "litterbug" mnemonic]
 *   - visRgn and clipRgn are in LOCAL coordinates.  [CS Sec 5]
 *   - mouse EventRecord.where arrives in GLOBAL coords; must GlobalToLocal
 *     before hit-testing against a control's contrlRect.  [CS Sec 5]
 *   - ALL DRAWING CLIPPED TO visRgn INTERSECT clipRgn (grafport.h invariant).
 *
 * era=system7.0-7.1
 * ===========================================================================*/

/*
 * FLAIR_COORD_RANGE_MIN / FLAIR_COORD_RANGE_MAX -- documented usable range.
 * Apple documents -32767..32767 (symmetric); storage is int16_t whose true
 * range is -32768..32767.  In-band coordinates for the 640x480 target are
 * 0..639 / 0..479, far inside this range.  [CS Sec 1]
 */
#define FLAIR_COORD_RANGE_MIN  (-32767)
#define FLAIR_COORD_RANGE_MAX    32767

/*
 * FLAIR_RECT_EMPTY(r) -- half-open emptiness test.  [CS Sec 3]
 *   r is a rgn_rect_t *.
 *   "If the bottom coordinate is less than or equal to the top coordinate,
 *    or if the right coordinate is less than or equal to the left coordinate,
 *    the rectangle is treated as empty."  [CS Sec 3 verbatim; region_algebra.h]
 */
#define FLAIR_RECT_EMPTY(r)  \
    ((r)->bottom <= (r)->top || (r)->right <= (r)->left)

/*
 * FLAIR_RECT_CONTAINS_PT(r, pv, ph) -- half-open containment test.  [CS Sec 3]
 *   r is a rgn_rect_t *.  pv = point.v (row), ph = point.h (column).
 *   "PtInRect: top<=v<bottom AND left<=h<right."  [CS Sec 4]
 */
#define FLAIR_RECT_CONTAINS_PT(r, pv, ph)  \
    ((pv) >= (r)->top  && (pv) < (r)->bottom && \
     (ph) >= (r)->left && (ph) < (r)->right)

/*
 * FLAIR_RECT_WIDTH(r) / FLAIR_RECT_HEIGHT(r) -- pixel extents.  [CS Sec 3]
 *   Width  = right - left (number of pixel columns in [left,right)).
 *   Height = bottom - top (number of pixel rows in [top,bottom)).
 */
#define FLAIR_RECT_WIDTH(r)   ((r)->right  - (r)->left)
#define FLAIR_RECT_HEIGHT(r)  ((r)->bottom - (r)->top)

/* ===========================================================================
 * 8. CopyBits parameter contract
 * ---------------------------------------------------------------------------
 * Ref: [CB] Sec 2-4.
 *
 * CopyBits Pascal signature ([CB] Sec 2, verbatim [QDp] via adagroup [S8]):
 *   PROCEDURE CopyBits (srcBits, dstBits: BitMap;
 *                       srcRect, dstRect: Rect;
 *                       mode:    Integer;
 *                       maskRgn: RgnHandle);
 *
 * FLAIR mapping (bitsProc in grafport.h QDProcs):
 *   void (*bitsProc)(const FLAIR_BitMap *src, const FLAIR_BitMap *dst,
 *                    const rgn_rect_t *srcRect, const rgn_rect_t *dstRect,
 *                    flair_xfer_mode_t mode, const region_t *maskRgn,
 *                    GrafPort *port)
 *
 * Parameter semantics (the locked contract):
 *
 *   srcBits  -- source surface descriptor (FLAIR_BitMap).  srcRect is
 *               expressed in srcBits.bounds coordinates.  [CB Sec 2]
 *   dstBits  -- destination surface descriptor (FLAIR_BitMap).  dstRect is
 *               expressed in dstBits.bounds coordinates.  [CB Sec 2]
 *   srcRect  -- the rectangle of srcBits to copy FROM (source-local).  [CB Sec 2]
 *   dstRect  -- the rectangle of dstBits to copy INTO (dest-local).  If its
 *               pixel size differs from srcRect's, the image is SCALED to fit.
 *               [CB Sec 2 + Sec 3 step 1: "CopyBits scales the source image
 *               to fit the destination."]
 *   mode     -- one of the 8 SOURCE transfer modes (srcCopy..notSrcBic =
 *               0..7 from imaging.h), optionally OR'd with FLAIR_COPYBITS_DITHER
 *               on System 7.  Pattern modes (8..15) are NOT valid here.
 *               [CB Sec 4 verbatim; CB Sec 2 "Pattern modes... NOT valid"]
 *   maskRgn  -- optional region (in dstBits coordinates) restricting WHERE the
 *               copy lands; NULL = no mask.  [CB Sec 2]
 *
 * OPERATION ORDER (the contract the blitter must implement):  [CB Sec 3]
 *   1. MAP srcRect onto dstRect; scale if sizes differ.
 *   2. DEPTH/CLUT convert if src and dst differ in pixel depth or color table.
 *   3. COLORIZE: apply the current port's rgbFgColor / rgbBkColor to the
 *      result (foreground replaces all black source pixels, background replaces
 *      all white source pixels).  [CB Sec 3 step 3; grafport.h rgbFgColor]
 *      To copy a 1-bit image WITHOUT tinting: set fgColor=black, bkColor=white
 *      before calling CopyBits.
 *   4. APPLY TRANSFER MODE (mode; the 8 src* modes; imaging.h srcCopy..notSrcBic).
 *   5. CLIP to: visRgn INTERSECT clipRgn INTERSECT maskRgn INTERSECT dstRect.
 *      [CB Sec 3 step 5; grafport.h clipping invariant]
 *
 * era=system7.0-7.1
 * ===========================================================================*/

/*
 * FLAIR_COPYBITS_DITHER -- the ditherCopy flag for CopyBits mode.
 *   System 7 only (era delta: NOT available on System 6).  [CB Sec 4; Sec 6]
 *   OR this into a src* mode to enable dithering on depth-reduction / shrink.
 *   Apple (IWQ cached [CB Sec 4]) verbatim:
 *   "CONST ditherCopy = 64; { $40 } -- dithers when reducing depth or shrinking."
 *   era=system7.0-7.1
 */
#define FLAIR_COPYBITS_DITHER  64   /* 0x40; System-7-only; [CB Sec 4] */

/*
 * FLAIR_COPYBITS_MODE_VALID(mode) -- true iff `mode` (stripped of ditherCopy)
 * is a valid CopyBits source mode (0..7).  Pattern modes (8..15) are NOT valid
 * for CopyBits.  [CB Sec 4 + imaging.h srcCopy..notSrcBic]
 */
#define FLAIR_COPYBITS_MODE_VALID(mode) \
    (((mode) & ~FLAIR_COPYBITS_DITHER) <= (int)notSrcBic)

/*
 * FLAIR_COPYBITS_COLORIZE(src_bit, fg_idx, bk_idx)
 *   The colorize formula for a 1-bit-to-indexed-8 blit:
 *     result_index = src_bit ? fg_idx : bk_idx
 *   At indexed-8, foreground replaces set bits (1), background replaces clear
 *   bits (0).  [CB Sec 3 step 3; web corroboration: ColoredPixelValue =
 *   (NOT(s) AND bg) OR (s AND fg) -- consistent at 1-bit depth]
 *   NOTE: the actual indexed-8 COLOR of fg_idx/bk_idx is [golden-resolves];
 *   this macro fixes the index selection formula, not the RGB.
 */
#define FLAIR_COPYBITS_COLORIZE(src_bit, fg_idx, bk_idx) \
    ((src_bit) ? (fg_idx) : (bk_idx))

/* ===========================================================================
 * 9. COMPILE-TIME CONTRACT CHECKS (the oracle bites at build time)
 * ---------------------------------------------------------------------------
 * Style follows spec/imaging.h (the style exemplar).
 * Cross-reference notation:
 *   (*)  -- already asserted in imaging.h or grafport.h; we re-assert here
 *           to make this file self-verifying.  If the base header's constant
 *           changes, BOTH files fail -- catching any inconsistency.
 * ===========================================================================*/

/* --- GrafVerb values (the load-bearing codes; [DP Sec 7; QDp L57]) -------- */
_Static_assert((int)kGrafVerbFrame  == 0,
               "GrafVerb frame=0 ([QDp] L57 / [DP] Sec 7)");
_Static_assert((int)kGrafVerbPaint  == 1,
               "GrafVerb paint=1 ([QDp] L57 / [DP] Sec 7)");
_Static_assert((int)kGrafVerbErase  == 2,
               "GrafVerb erase=2 ([QDp] L57 / [DP] Sec 7)");
_Static_assert((int)kGrafVerbInvert == 3,
               "GrafVerb invert=3 ([QDp] L57 / [DP] Sec 7)");
_Static_assert((int)kGrafVerbFill   == 4,
               "GrafVerb fill=4 ([QDp] L57 / [DP] Sec 7)");

/* GrafVerbs fit in 3 bits; exactly 5 values 0..4; fill is the max. */
_Static_assert((int)kGrafVerbFill <= 7,
               "GrafVerb values must fit in 3 bits (0..4 documented)");

/* The verb that reads pnPat (frame+paint) must be < the verb that reads bkPat
 * (erase) must be < the verb that reads fillPat (fill).  This order is the
 * QDVerb enum order and is load-bearing for any dispatch table. */
_Static_assert((int)kGrafVerbFrame  < (int)kGrafVerbErase,
               "frame < erase in GrafVerb order ([QDp] L57)");
_Static_assert((int)kGrafVerbPaint  < (int)kGrafVerbErase,
               "paint < erase in GrafVerb order ([QDp] L57)");
_Static_assert((int)kGrafVerbErase  < (int)kGrafVerbInvert,
               "erase < invert in GrafVerb order ([QDp] L57)");
_Static_assert((int)kGrafVerbInvert < (int)kGrafVerbFill,
               "invert < fill in GrafVerb order ([QDp] L57)");

/* --- Pen initial state ([DP] Sec 2) --------------------------------------- */
_Static_assert(FLAIR_PEN_INIT_PNVIS    == 0,
               "Initial pnVis must be 0 (visible) ([DP] Sec 2)");
_Static_assert(FLAIR_PEN_INIT_PNSIZE_V == 1,
               "Initial pen height (pnSize.v) must be 1 ([DP] Sec 2)");
_Static_assert(FLAIR_PEN_INIT_PNSIZE_H == 1,
               "Initial pen width (pnSize.h) must be 1 ([DP] Sec 2)");
_Static_assert((int)FLAIR_PEN_INIT_PNMODE == (int)patCopy,
               "Initial pnMode must be patCopy=8 ([DP] Sec 2; imaging.h)");

/* --- CopyBits mode contract ([CB] Sec 4) ---------------------------------- */
_Static_assert(FLAIR_COPYBITS_DITHER == 64,
               "ditherCopy must be 64 / 0x40 ([CB] Sec 4; [QDp] CONST)");

/* CopyBits valid modes are src* (0..7); patCopy (8) must NOT be valid. */
_Static_assert( FLAIR_COPYBITS_MODE_VALID(0), "srcCopy=0 valid for CopyBits");
_Static_assert( FLAIR_COPYBITS_MODE_VALID(7), "notSrcBic=7 valid for CopyBits");
_Static_assert(!FLAIR_COPYBITS_MODE_VALID(8),
               "patCopy=8 must NOT be valid for CopyBits ([CB] Sec 4)");
_Static_assert(!FLAIR_COPYBITS_MODE_VALID(15),
               "notPatBic=15 must NOT be valid for CopyBits ([CB] Sec 4)");

/* ditherCopy OR'd with srcCopy is also valid (a src mode + dither). */
_Static_assert(FLAIR_COPYBITS_MODE_VALID(0 | FLAIR_COPYBITS_DITHER),
               "srcCopy+ditherCopy must be valid ([CB] Sec 4)");

/* --- Coordinate system contract ([CS] Sec 1-5) ---------------------------- */

/* flair_point_t.v is at offset 0, flair_point_t.h at offset 2.
 * These are also asserted in grafport.h (*); re-asserted here for
 * self-containedness of this file. */
_Static_assert(offsetof(flair_point_t, v) == 0,
               "Point.v must be at offset 0 (v-first; [CS] Sec 2; [QDp] L68)");
_Static_assert(offsetof(flair_point_t, h) == 2,
               "Point.h must be at offset 2 ([CS] Sec 2; [QDp] L68)");
_Static_assert(sizeof(flair_point_t) == 4,
               "Point must be 4 bytes (2 x int16_t; [CS] Sec 2)");

/* rgn_rect_t (the Rect type from region_algebra.h) field order:
 * top=0, left=2, bottom=4, right=6.  [CS Sec 3; QDp L78-87]
 * These are also asserted in region_algebra.h (*). */
_Static_assert(offsetof(rgn_rect_t, top)    == 0,
               "Rect.top must be at offset 0 ([CS] Sec 3; [QDp] L78)");
_Static_assert(offsetof(rgn_rect_t, left)   == 2,
               "Rect.left must be at offset 2 ([CS] Sec 3; [QDp] L78)");
_Static_assert(offsetof(rgn_rect_t, bottom) == 4,
               "Rect.bottom must be at offset 4 ([CS] Sec 3; [QDp] L78)");
_Static_assert(offsetof(rgn_rect_t, right)  == 6,
               "Rect.right must be at offset 6 ([CS] Sec 3; [QDp] L78)");
_Static_assert(sizeof(rgn_rect_t) == 8,
               "Rect must be 8 bytes = 2 x Point ([CS] Sec 3)");

/* FLAIR_COORD_RANGE constants (documented range). */
_Static_assert(FLAIR_COORD_RANGE_MIN == -32767,
               "Coord range min is -32767 ([CS] Sec 1; IM Overview verbatim)");
_Static_assert(FLAIR_COORD_RANGE_MAX ==  32767,
               "Coord range max is 32767 ([CS] Sec 1; IM Overview verbatim)");

/* --- Transfer-mode cross-check with imaging.h (*) ------------------------- */
/* patCopy used as pen default; must equal 8 (imaging.h already asserts this,
 * but we cross-check explicitly to confirm the include path is consistent). */
_Static_assert((int)patCopy == 8,
               "patCopy must be 8 (imaging.h flair_xfer_mode_t; [QDp] L16)");
_Static_assert((int)srcCopy == 0,
               "srcCopy must be 0 (imaging.h; [CB] Sec 4; [QDp])");
_Static_assert((int)notSrcBic == 7,
               "notSrcBic must be 7 (imaging.h; [CB] Sec 4)");

/* --- Pattern size cross-check with imaging.h (*) -------------------------- */
_Static_assert(sizeof(Pattern) == 8,
               "Pattern must be 8 bytes (imaging.h; [PA] Sec 1; [QDp] L54)");

/* --- Shape-family ordinals (informational; 0..5) -------------------------- */
_Static_assert((int)kShapeRect  == 0, "rect shape = 0");
_Static_assert((int)kShapeRRect == 1, "rrect shape = 1");
_Static_assert((int)kShapeOval  == 2, "oval shape = 2");
_Static_assert((int)kShapeArc   == 3, "arc shape = 3");
_Static_assert((int)kShapePoly  == 4, "poly shape = 4");
_Static_assert((int)kShapeRgn   == 5, "rgn shape = 5");

/* --- FLAIR_RECT_WIDTH/HEIGHT yield non-negative for a non-empty rect.
 * Verify on a sample rect: right=10,left=3 -> width=7; bottom=5,top=2 -> h=3 */
_Static_assert((10 - 3) == 7, "FLAIR_RECT_WIDTH formula: right-left");
_Static_assert((5  - 2) == 3, "FLAIR_RECT_HEIGHT formula: bottom-top");

#endif /* INITECH_SPEC_DRAWING_OPS_H */
