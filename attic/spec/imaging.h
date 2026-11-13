/*
 * spec/imaging.h -- FLAIR imaging primitives: the LOCKED contract.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8; PRD Sec 6.3 -- "the Toolbox layer").
 * beads: initech-k8o5.3 (FLAIR imaging contract as spec-data).
 *
 * This header fixes the QuickDraw imaging DATA: the BitMap descriptor, the
 * 8x8 1-bpp Pattern, the transfer-mode enum, and the RGBColor record. It is
 * the imaging contract that spec/grafport.h (the drawing-context record) and
 * the FLAIR Toolbox (Layer 1, ADR-0004 D-1) are written against.
 *
 * Source / Law 1 citations (all local or free Apple developer archive):
 *
 *   Inside Macintosh: Imaging With QuickDraw (1994) [EXTERNAL, free Apple
 *     developer archive; cached per gui-ground-truth.md Sec 3.2 P0 item 3;
 *     cited as "IWQ" below] -- the primary reference for ALL record names,
 *     field names, and transfer-mode constants in this file. Verbatim field
 *     names carried for period authenticity and self-documentation (ADR-0004
 *     D-3). IWQ is the "Imaging With QuickDraw" volume cited in
 *     docs/research/gui-ground-truth.md Sec 3.2.
 *
 *   Inside Macintosh: Macintosh Toolbox Essentials (1992) [EXTERNAL, free;
 *     cited as "MTE" below] -- supplementary on GrafPort and pattern use.
 *
 *   Inside Macintosh Vols I-VI (1984-1991) [archive.org bitsavers_applemacIn84;
 *     cited as "IM-I" .. "IM-VI" below] -- the original verbatim record
 *     definitions (BitMap in IM-I Ch 7; Pattern in IM-I Ch 7; transfer modes
 *     in IM-I Ch 7; RGBColor in IM-V Color QuickDraw).
 *
 *   ADR-0004 (FLAIR Toolbox Architecture, RATIFIED 2026-06-19):
 *     D-1 (5-layer stack; Layer 1 = surface/GrafPort),
 *     D-2 (one surface module; GrafPort imaging; no second pixel path;
 *          offscreen bitmaps are indexed-8 per OD-2),
 *     D-3 (verbatim Inside Macintosh records and field names),
 *     OD-2 (indexed-8 offscreen depth; 1 byte/pixel = palette index;
 *           8-bpp VGA/SVGA era; VGA Mode 13h, VBE modes 0x101/0x103).
 *
 *   os/flair/surface.h (the ONE pixel-buffer type, bitmap_t; ADR-0004 D-2;
 *     beads initech-k8o5) -- BitMap (below) REFERENCES this type; there is
 *     NO second pixel-buffer definition in InitechOS.
 *
 *   spec/region_algebra.h (rgn_rect_t; the QuickDraw Rect order top,left,
 *     bottom,right) -- BitMap.bounds uses rgn_rect_t for period authenticity
 *     and to keep the single Rect typedef.
 *
 * DUAL-COMPILE: this header compiles BOTH freestanding (gcc -m32 -ffreestanding
 * -nostdlib -std=c11) and hosted (cc -std=c11). Dependencies: <stdint.h> and
 * <stddef.h> (both freestanding-provided), plus the two local headers above.
 * No host malloc; no libc beyond stdint/stddef. Rule 11 (reproducible).
 *
 * ASCII-clean (Rule 12). No nondeterminism / no timestamps (Rule 11).
 * Changing this file is a deliberate, beads-tracked Rule 8 act.
 */
#ifndef INITECH_SPEC_IMAGING_H
#define INITECH_SPEC_IMAGING_H

#include <stdint.h>
#include <stddef.h>

/* Pull in the ONE pixel-buffer type and the single Rect typedef. */
#include "../os/flair/surface.h"     /* bitmap_t                   */
#include "region_algebra.h"          /* rgn_rect_t (QuickDraw Rect) */

/* ===========================================================================
 * 1. BitMap -- the QuickDraw bitmap descriptor
 * ---------------------------------------------------------------------------
 * Ref: Inside Macintosh Vol I Ch 7 (IM-I), "Data Types", BitMap record;
 *      IWQ Chapter 2 "Basic QuickDraw Data Structures."
 *
 * A BitMap in QuickDraw is a triplet (baseAddr, rowBytes, bounds).
 *
 *   baseAddr -- pointer to the first row of pixel data.
 *   rowBytes -- the number of bytes in each row of the bitmap; must be even
 *               (IM-I p. I-143: "The value of rowBytes must be even").
 *   bounds   -- the boundary rectangle that defines the boundaries of the
 *               bitmap's coordinate system (IM-I p. I-143).
 *
 * Design decisions (Law 1 / ADR-0004 D-2):
 *
 *   ONE pixel-buffer type: InitechOS has exactly one pixel-buffer descriptor,
 *   bitmap_t, defined in os/flair/surface.h (ADR-0004 D-2, C-2). Rather than
 *   define a second baseAddr/rowBytes/bpp layout here, FLAIR_BitMap EMBEDS
 *   bitmap_t as its `bm` field (which carries base + pitch/rowBytes + bpp +
 *   bytes_per_pixel + width + height). The QuickDraw `rowBytes` framing is
 *   provided as a read-only computed value via the inline FLAIR_bitmap_rowbytes
 *   accessor so callers that must name `rowBytes` (e.g., when matching the
 *   verbatim QuickDraw API) have a clear path. The QuickDraw `bounds` field
 *   is carried verbatim as `bounds` (type rgn_rect_t, the single Rect type).
 *
 *   Offscreen bitmaps at indexed-8 (OD-2, ADR-0004): bm.bpp == 8, one byte
 *   per pixel. The LFB target bitmap may be 8, 24, or 32 bpp per surface.h.
 *
 * Transfer modes, patterns, and palette are consulted by the GrafPort
 * (grafport.h); the BitMap itself is just the destination/source descriptor.
 * ===========================================================================*/

/*
 * FLAIR_BitMap -- QuickDraw BitMap (verbatim field names, IM-I Ch 7).
 *
 * Ref: Inside Macintosh Vol I p. I-143 (BitMap record):
 *   baseAddr: "Pointer. A pointer to the upper-left corner of the bitmap."
 *   rowBytes: "Integer. The number of bytes in each row of the bitmap."
 *   bounds:   "Rect. The boundary rectangle."
 *
 * `bm` is the low-level pixel-buffer descriptor (os/flair/surface.h); it
 * carries the equivalent of baseAddr (bm.base), rowBytes (bm.pitch), bpp,
 * width, and height. `bounds` is the QuickDraw boundary rectangle.
 */
typedef struct FLAIR_BitMap {
    bitmap_t    bm;        /* the ONE pixel-buffer descriptor (surface.h)  */
    rgn_rect_t  bounds;    /* boundary rectangle (QuickDraw Rect; IM-I Ch 7)*/
} FLAIR_BitMap;

/* FLAIR_bitmap_rowbytes -- return the QuickDraw rowBytes value for a bitmap.
 * rowBytes = bm.pitch (bytes per row), which must be even (IM-I p. I-143).
 * This accessor makes the IM-I invariant machine-checkable at runtime and
 * gives callers the canonical QuickDraw name. */
static inline uint32_t FLAIR_bitmap_rowbytes(const FLAIR_BitMap *bmp)
{
    return bmp->bm.pitch;
}

/* ===========================================================================
 * 2. Pattern -- the QuickDraw 8x8 1-bpp pattern
 * ---------------------------------------------------------------------------
 * Ref: Inside Macintosh Vol I Ch 7 (IM-I), "Data Types", Pattern:
 *   "A pattern is a 64-bit image that defines a repeating design used to fill
 *    or frame a shape, or to draw a line. An 8-byte value (Pat) represents 8
 *    rows of an 8x8-pixel pattern, where each row is 1 byte."
 * IWQ Chapter 3 confirms this representation and the 8-byte size.
 *
 * Each byte is one row; within a byte, MSB (bit 7) = leftmost pixel,
 * LSB (bit 0) = rightmost pixel (IM-I p. I-130, scan direction).
 * A set bit (1) means the foreground color (fgColor in the GrafPort);
 * a clear bit (0) means the background color (bgColor).
 *
 * Standard QuickDraw patterns (IM-I p. I-133 system patterns, Appendix D):
 *   white    = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
 *   black    = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
 *   gray     = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 }
 *   ltGray   = { 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22 }
 *   dkGray   = { 0xDD, 0x77, 0xDD, 0x77, 0xDD, 0x77, 0xDD, 0x77 }
 * These are informational; the locked values live in spec/assets/ (ADR-0004
 * Sec 2.2, Rule 8). The desktop background is the SEAFOAM solid (OD-4).
 * ===========================================================================*/

/*
 * Pattern (verbatim QuickDraw name, IM-I Ch 7):
 *   "An 8-byte value (Pat) represents 8 rows of an 8x8-pixel pattern."
 * Named `Pattern` to match the verbatim QuickDraw identifier. In C contexts
 * that must avoid namespace collision, callers may use the flair_pattern_t
 * typedef alias below.
 */
typedef struct Pattern {
    uint8_t pat[8];   /* 8 rows x 1 byte/row; MSB = leftmost pixel (IM-I) */
} Pattern;

/* flair_pattern_t -- alias for use where the bare name Pattern collides. */
typedef Pattern flair_pattern_t;

/* ===========================================================================
 * 3. Transfer modes -- verbatim QuickDraw source/pattern transfer-mode enum
 * ---------------------------------------------------------------------------
 * Ref: Inside Macintosh Vol I Ch 7 (IM-I), "Transfer Modes" (p. I-156 ff);
 *      IWQ Chapter 4 "Copying Bits Between Pixel Maps and Bitmaps" (Table 4-1
 *      lists the transfer-mode constants and their Pascal integer values).
 *      Also confirmed in IM-V Color QuickDraw and the Inside Macintosh
 *      Toolbox Essentials (MTE) Appendix D (numeric values repeated).
 *
 * The src* modes govern how a source bitmap's bits are combined with the
 * destination; the pat* modes govern how a pattern is applied. Both sets are
 * defined for 1-bpp (monochrome) and color pixMaps (CopyBits uses these).
 *
 * Numeric values (IWQ Table 4-1 / IM-I p. I-157):
 *   srcCopy=0, srcOr=1, srcXor=2, srcBic=3,
 *   notSrcCopy=4, notSrcOr=5, notSrcXor=6, notSrcBic=7,
 *   patCopy=8, patOr=9, patXor=10, patBic=11,
 *   notPatCopy=12, notPatOr=13, notPatXor=14, notPatBic=15.
 *
 * The `not*` modes invert the source/pattern bits BEFORE applying the boolean
 * operation -- e.g. notSrcCopy copies the bitwise complement of the source
 * (IM-I p. I-157: "NotSrcCopy copies the inverse of the source").
 *
 * FLAIR renders at indexed-8 (OD-2); for color pixMaps the transfer modes
 * apply to the palette index (the 8-bit byte), not to RGB components. The
 * arithmetic-mode extensions (blend, addPin, etc.) introduced in Color
 * QuickDraw (IM-V) are noted for completeness but are NOT part of the locked
 * FLAIR contract for the current release (PRD M3/M4; the 8 src + 8 pat modes
 * above cover all required Toolbox use).
 * ===========================================================================*/

/*
 * flair_xfer_mode_t -- QuickDraw transfer modes (verbatim names, IM-I Ch 7
 * / IWQ Table 4-1). Numeric values match the published QuickDraw constants.
 */
typedef enum flair_xfer_mode {
    /* --- Source transfer modes (src bits -> destination) --------------------
     * Ref: IM-I p. I-156 "Source Transfer Modes"; IWQ Table 4-1.           */
    srcCopy      = 0,  /* copy source bits into destination (IM-I p. I-157)  */
    srcOr        = 1,  /* OR source bits into destination                    */
    srcXor       = 2,  /* XOR source bits with destination                   */
    srcBic       = 3,  /* BIC: AND destination with NOT source (bit-clear)   */
    notSrcCopy   = 4,  /* copy complement of source into destination         */
    notSrcOr     = 5,  /* OR complement of source into destination           */
    notSrcXor    = 6,  /* XOR complement of source with destination          */
    notSrcBic    = 7,  /* BIC: AND destination with source (i.e. BIC(~src))  */

    /* --- Pattern transfer modes (pattern bits -> destination) ---------------
     * Ref: IM-I p. I-158 "Pattern Transfer Modes"; IWQ Table 4-1.          */
    patCopy      = 8,  /* copy pattern bits into destination                 */
    patOr        = 9,  /* OR pattern bits into destination                   */
    patXor       = 10, /* XOR pattern bits with destination                  */
    patBic       = 11, /* BIC: AND destination with NOT pattern              */
    notPatCopy   = 12, /* copy complement of pattern into destination        */
    notPatOr     = 13, /* OR complement of pattern into destination          */
    notPatXor    = 14, /* XOR complement of pattern with destination         */
    notPatBic    = 15  /* BIC: AND destination with pattern (i.e. BIC(~pat)) */
} flair_xfer_mode_t;

/* ===========================================================================
 * 4. RGBColor -- 16-bit-per-channel QuickDraw color record
 * ---------------------------------------------------------------------------
 * Ref: Inside Macintosh Vol V, Color QuickDraw, "Data Types", RGBColor
 *   (IM-V p. V-15): "A color is described by three components -- red, green,
 *    and blue -- each represented as an unsigned integer in the range 0-65535.
 *    A value of 0 indicates the absence of the color component, and a value of
 *    65535 indicates full intensity."
 * Also confirmed in IWQ Chapter 7 "Color QuickDraw".
 *
 * FLAIR rendering depth note (ADR-0004 OD-2, AM-7, Law 1):
 *   FLAIR renders internally at indexed-8 (one byte per pixel = palette index;
 *   the authentic early-1990s 8-bpp VGA/SVGA offscreen depth -- VGA Mode 13h,
 *   VBE modes 0x101/0x103). An RGBColor therefore maps through the FLAIR
 *   palette (spec/assets/palette.json) to a palette index before being written
 *   to an offscreen bitmap. RGBColor is used in GrafPort fgColor/bgColor for
 *   Color QuickDraw compatibility and for the `RGBForeColor`/`RGBBackColor`
 *   equivalents; the palette lookup is the responsibility of the blitter
 *   (ADR-0004 D-2, the one conversion site in the surface module).
 *
 * Note on the fgColor/bgColor in GrafPort (classic QuickDraw, IM-I Ch 7):
 *   Classic (1-bpp) GrafPort carried integer `fgColor`/`bgColor` (pen color
 *   selector; IM-I p. I-148: values are one of the classic QD color constants
 *   blackColor=33, whiteColor=30, redColor=205, etc.). Color QuickDraw (IM-V)
 *   extended this with an RGBColor pair (rgbFgColor / rgbBkColor). FLAIR
 *   adopts RGBColor for fore/back in the GrafPort (grafport.h) because it
 *   targets color indexed-8 output (OD-2), not 1-bpp; the classic integer
 *   color selector is not used.
 * ===========================================================================*/

/*
 * RGBColor (verbatim QuickDraw name, IM-V Color QuickDraw):
 *   red, green, blue -- each a 16-bit unsigned integer in [0, 65535].
 *   65535 = full intensity; 0 = absent.
 */
typedef struct RGBColor {
    uint16_t red;    /* red   component, 0-65535 (IM-V p. V-15)             */
    uint16_t green;  /* green component, 0-65535                             */
    uint16_t blue;   /* blue  component, 0-65535                             */
} RGBColor;

/* flair_rgb_t -- alias for contexts where the bare name RGBColor collides. */
typedef RGBColor flair_rgb_t;

/* ===========================================================================
 * 5. COMPILE-TIME CONTRACT CHECKS  (the oracle bites at build time)
 * ---------------------------------------------------------------------------
 * Style follows spec/region_algebra.h (the style exemplar per the brief).
 * ===========================================================================*/

/* Pattern is exactly 8 bytes (8 rows x 1 byte; IM-I Ch 7). */
_Static_assert(sizeof(Pattern) == 8,
               "Pattern must be 8 bytes (8 rows x 1 byte; IM-I Ch 7)");

/* Transfer-mode canonical values (IWQ Table 4-1 / IM-I p. I-157). */
_Static_assert((int)srcCopy    == 0,  "srcCopy = 0 (IWQ Table 4-1)");
_Static_assert((int)srcOr      == 1,  "srcOr = 1");
_Static_assert((int)srcXor     == 2,  "srcXor = 2");
_Static_assert((int)srcBic     == 3,  "srcBic = 3");
_Static_assert((int)notSrcCopy == 4,  "notSrcCopy = 4");
_Static_assert((int)notSrcOr   == 5,  "notSrcOr = 5");
_Static_assert((int)notSrcXor  == 6,  "notSrcXor = 6");
_Static_assert((int)notSrcBic  == 7,  "notSrcBic = 7");
_Static_assert((int)patCopy    == 8,  "patCopy = 8");
_Static_assert((int)patOr      == 9,  "patOr = 9");
_Static_assert((int)patXor     == 10, "patXor = 10");
_Static_assert((int)patBic     == 11, "patBic = 11");
_Static_assert((int)notPatCopy == 12, "notPatCopy = 12");
_Static_assert((int)notPatOr   == 13, "notPatOr = 13");
_Static_assert((int)notPatXor  == 14, "notPatXor = 14");
_Static_assert((int)notPatBic  == 15, "notPatBic = 15");

/* src* modes are in [0,7] and pat* modes are in [8,15] -- the two halves
 * are cleanly separated; no overlap (a typo creating a collision is a build
 * error). */
_Static_assert((int)srcCopy < (int)patCopy,
               "src* modes [0,7] must precede pat* modes [8,15]");
_Static_assert((int)notSrcBic < (int)patCopy,
               "notSrcBic (7) must be below patCopy (8)");

/* RGBColor is 3 x uint16_t = 6 bytes; field offsets as IM-V defines them. */
_Static_assert(sizeof(RGBColor) == 6,
               "RGBColor must be 3 x uint16_t = 6 bytes (IM-V)");
_Static_assert(offsetof(RGBColor, red)   == 0, "RGBColor.red @ 0");
_Static_assert(offsetof(RGBColor, green) == 2, "RGBColor.green @ 2");
_Static_assert(offsetof(RGBColor, blue)  == 4, "RGBColor.blue @ 4");

/* FLAIR_BitMap.bounds is the QuickDraw Rect (rgn_rect_t) at a stable offset.
 * The bounds field follows the bitmap_t; we assert it is accessible (its
 * offset is non-zero and sizeof is the standard 8 bytes). */
_Static_assert(offsetof(FLAIR_BitMap, bounds) > 0,
               "FLAIR_BitMap.bounds must follow bm (after the bitmap_t)");
_Static_assert(sizeof(((FLAIR_BitMap *)0)->bounds) == 8,
               "FLAIR_BitMap.bounds is rgn_rect_t = 8 bytes (QuickDraw Rect)");

#endif /* INITECH_SPEC_IMAGING_H */
