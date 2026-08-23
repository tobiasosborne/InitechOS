/*
 * cursors.h -- InitechOS FLAIR cursor assets v1 (LOCKED spec-data).
 *
 * beads: initech-zaqj (canon assets: hourglass CURS bytes + Photoshop menu
 *        string, ADR-0004 AM-4 / FO-E).
 *
 * Ref: CLAUDE.md Law 4 ("the hourglass cursor (not a wristwatch) ... are
 *      enforced, not fixed"), Law 1 (ground truth before code), Rule 11
 *      (byte-stable), Rule 12 (ASCII-clean).
 *      ADR-0004 D-7 ("the hourglass is canon ... shipped as FIXED BYTES
 *      in spec/assets/, byte-stable per Rule 11; FROZEN locked-data per
 *      AM-4, gated by the canon oracle").
 *      ADR-0004 AM-4 ("D-7's hourglass cursor must be FROZEN locked-data
 *      (named spec/assets/ files or named locked constants), not prose an
 *      agent may silently 'correct'").
 *      docs/research/gui-ground-truth.md Sec 3.6 ("the real System 7 busy
 *      cursor is an animated WRISTWATCH ... InitechOS canon is the HOURGLASS
 *      -- the wristwatch is 'the bug', the deliberate frame anachronism ...
 *      do not implement it").
 *      PRD Appendix A (the canon frame); PRD Sec 6.3.
 *
 * =========================================================================
 * CANON WARNING -- READ BEFORE ANY EDIT
 * =========================================================================
 *
 * THE HOURGLASS IS CANON. THE WRISTWATCH IS THE BUG.
 *
 * The real Mac System 7 busy cursor is an animated wristwatch (16x16,
 * 8 frames). InitechOS does NOT ship the wristwatch. The HOURGLASS below
 * IS the correct busy cursor for InitechOS. The wristwatch is documented in
 * gui-ground-truth.md Sec 3.6 only to confirm that the anachronism is real.
 * Any agent reading this file MUST NOT replace the hourglass with a
 * wristwatch -- doing so would "fix" a deliberate canon element and violate
 * Law 4 + ADR-0004 D-7 / AM-4. This is a Stop Condition.
 *
 * These bytes are HAND-AUTHORED canon, NOT extracted from the frame still
 * (which is a dim, compressed CRT photo -- pixel recovery is impossible and
 * would be an IP violation; CLAUDE.md hallucination-risk callout). They are
 * also NOT lifted from any ROM dump or real Mac CURS resource. They are a
 * fresh, clean-room authored 16x16 hourglass / sandglass shape authored to
 * be recognizable and byte-stable (Rule 11).
 *
 * The canon oracle (ADR-0004 D-8 "canon" gate, beads initech-k8o5.10) asserts
 * these exact bytes; a test that includes this header checks
 * FLAIR_CURSOR_BUSY.data[] and .mask[] against expected values to ensure
 * they have not drifted.
 *
 * Changing these bytes is a DELIBERATE, issue-tracked Rule 8 act. Never
 * silently edit to "improve" the appearance.
 *
 * =========================================================================
 *
 * FORMAT: CURS-style 16x16 monochrome cursor (Mac Inside Macintosh Toolbox
 * Essentials reference -- the 'CURS' resource format).
 *   data[16]  -- uint16_t per row; 1 = black (ink), 0 = white (paper).
 *                MSB (bit 15) = leftmost column (col 0).
 *   mask[16]  -- uint16_t per row; 1 = cursor pixel is opaque (drawn);
 *                0 = transparent (screen shows through). The mask is the
 *                union / convex hull of the image: every ink pixel is
 *                opaque, and in our case the mask == data (no dilation
 *                needed because the cursor is a solid silhouette).
 *   hotSpot   -- (row, col) of the active pixel, 0-indexed. For the busy
 *                (hourglass) cursor, hotspot = center = (8, 8) so that the
 *                waist of the glass tracks the click point.
 *                For the arrow cursor, hotspot = tip = (0, 0).
 *
 * Encoding: uint16_t in native byte order of the host rendering code.
 * The CURS resource body is big-endian on the Mac; here we use uint16_t
 * and let the blitter read it correctly (the harness is host-code, Law 3).
 *
 * All identifiers are prefixed FLAIR_ to stay in the InitechOS namespace.
 */

#ifndef INITECH_CURSORS_H
#define INITECH_CURSORS_H

#include <stdint.h>

/* A CURS-style 16x16 monochrome cursor descriptor.
 *
 * data[r] : ink row r, MSB = col 0.
 * mask[r] : opaque region row r, MSB = col 0.
 * hot_row / hot_col : hotspot (0-indexed). */
typedef struct {
    uint16_t data[16];
    uint16_t mask[16];
    uint16_t hot_row;
    uint16_t hot_col;
} FLAIRCursor;


/* =========================================================================
 * FLAIR_CURSOR_BUSY -- the HOURGLASS busy cursor (CANON, FROZEN)
 * =========================================================================
 *
 * PROVENANCE: HAND-AUTHORED clean-room. NOT the System-7 wristwatch (which
 * is the deliberate frame bug; see file header above). NOT extracted from
 * any frame still or ROM. Authored 2026-06-19, beads initech-zaqj.
 *
 * Shape (16x16 pixel map; '#' = ink, '.' = paper):
 *
 *   col:  0123456789012345
 *   row 0: ################   top bar (solid)
 *   row 1: ################   top bar (solid)
 *   row 2: .##############.   taper in by 1 each side
 *   row 3: ..############..   taper in by 2 each side
 *   row 4: ...##########...   taper in by 3 each side
 *   row 5: ....########....   taper in by 4 each side
 *   row 6: .....######.....   taper in by 5 each side
 *   row 7: .......##.......   2-pixel waist (neck of the glass)
 *   row 8: .......##.......   2-pixel waist (neck of the glass)
 *   row 9: .....######.....   expand by 5 each side (sand builds)
 *   row10: ....########....   expand by 4 each side
 *   row11: ...##########...   expand by 3 each side
 *   row12: ..############..   expand by 2 each side
 *   row13: .##############.   expand by 1 each side
 *   row14: ################   bottom bar (solid)
 *   row15: ################   bottom bar (solid)
 *
 * The mask equals the image data (solid silhouette, no dilation needed).
 * HotSpot = (8, 8) -- center / waist of the glass.
 *
 * Bit arithmetic (MSB=col0, 16 bits):
 *   Full 16 cols      : 0xFFFF
 *   14 wide (col1-14) : 0x7FFE
 *   12 wide (col2-13) : 0x3FFC
 *   10 wide (col3-12) : 0x1FF8
 *    8 wide (col4-11) : 0x0FF0
 *    6 wide (col5-10) : 0x07E0
 *    2 wide (col7-8)  : 0x0180
 */
static const FLAIRCursor FLAIR_CURSOR_BUSY = {
    /* data[16] -- ink */
    {
        /* row  0 */ 0xFFFF, /* ################ */
        /* row  1 */ 0xFFFF, /* ################ */
        /* row  2 */ 0x7FFE, /* .##############. */
        /* row  3 */ 0x3FFC, /* ..############.. */
        /* row  4 */ 0x1FF8, /* ...##########... */
        /* row  5 */ 0x0FF0, /* ....########.... */
        /* row  6 */ 0x07E0, /* .....######..... */
        /* row  7 */ 0x0180, /* .......##....... */
        /* row  8 */ 0x0180, /* .......##....... */
        /* row  9 */ 0x07E0, /* .....######..... */
        /* row 10 */ 0x0FF0, /* ....########.... */
        /* row 11 */ 0x1FF8, /* ...##########... */
        /* row 12 */ 0x3FFC, /* ..############.. */
        /* row 13 */ 0x7FFE, /* .##############. */
        /* row 14 */ 0xFFFF, /* ################ */
        /* row 15 */ 0xFFFF  /* ################ */
    },
    /* mask[16] -- opaque region (identical to data; solid silhouette) */
    {
        /* row  0 */ 0xFFFF,
        /* row  1 */ 0xFFFF,
        /* row  2 */ 0x7FFE,
        /* row  3 */ 0x3FFC,
        /* row  4 */ 0x1FF8,
        /* row  5 */ 0x0FF0,
        /* row  6 */ 0x07E0,
        /* row  7 */ 0x0180,
        /* row  8 */ 0x0180,
        /* row  9 */ 0x07E0,
        /* row 10 */ 0x0FF0,
        /* row 11 */ 0x1FF8,
        /* row 12 */ 0x3FFC,
        /* row 13 */ 0x7FFE,
        /* row 14 */ 0xFFFF,
        /* row 15 */ 0xFFFF
    },
    /* hot_row */ 8,
    /* hot_col */ 8
};


/* =========================================================================
 * FLAIR_CURSOR_ARROW -- standard northwest-pointing arrow (period-standard)
 * =========================================================================
 *
 * PROVENANCE: HAND-AUTHORED clean-room. Period-standard Mac arrow cursor
 * shape; NOT extracted from any ROM or frame.
 * v1 authored 2026-06-19 (beads initech-zaqj): solid silhouette, mask==data,
 *    hotspot (0,0).
 * v2 authored 2026-08-23 (beads initech-tdnl.25, DELIBERATE Rule-8 amendment,
 *    GUI remediation R0.1): the period Mac arrow composites a 1px WHITE
 *    OUTLINE around the black body (CURS mask = one-pixel dilation of data;
 *    Ref: Inside Macintosh, Imaging With QuickDraw, 'CURS' compositing --
 *    mask&~data pixels paint white). The body sits at +1,+1 so the dilation
 *    fits the 16x16 grid; hotspot = tip ink pixel = (row 1, col 1). The
 *    undilated v1 read as a flat silhouette on the desktop (operator
 *    pixel-perfect ruling 2026-08-23). Supersedes v1 wholesale.
 *
 * Shape v2 (16x16; '#' = ink, 'o' = white outline (mask only), '.' = clear):
 *
 *   col:  0123456789012345
 *   row 0: ooo.............
 *   row 1: o#oo............   tip (hotspot 1,1)
 *   row 2: o##oo...........
 *   row 3: o###oo..........
 *   row 4: o####oo.........
 *   row 5: o#####oo........
 *   row 6: o######oo.......
 *   row 7: o#######oo......
 *   row 8: o########oo.....
 *   row 9: o#########o.....   widest
 *   row10: o#####ooooo.....   back-edge diagonal cuts in
 *   row11: o##o##o.........
 *   row12: o#oo##o.........
 *   row13: oooo##o.........   stem only
 *   row14: ...o##o.........
 *   row15: ...oooo.........
 *
 * NOTE: This is a secondary (non-canon-gated) asset; the hourglass above is
 * the CANON deliverable gated by the canon oracle. The arrow's drift tooth
 * is harness/proptest/test_cursor.c, whose independent hard-coded golden
 * tables must equal these bytes (a silent edit here goes RED there).
 */
static const FLAIRCursor FLAIR_CURSOR_ARROW = {
    /* data[16] -- ink */
    {
        /* row  0 */ 0x0000, /* ................ */
        /* row  1 */ 0x4000, /* .#.............. */
        /* row  2 */ 0x6000, /* .##............. */
        /* row  3 */ 0x7000, /* .###............ */
        /* row  4 */ 0x7800, /* .####........... */
        /* row  5 */ 0x7C00, /* .#####.......... */
        /* row  6 */ 0x7E00, /* .######......... */
        /* row  7 */ 0x7F00, /* .#######........ */
        /* row  8 */ 0x7F80, /* .########....... */
        /* row  9 */ 0x7FC0, /* .#########...... */
        /* row 10 */ 0x7C00, /* .#####.......... */
        /* row 11 */ 0x6C00, /* .##.##.......... */
        /* row 12 */ 0x4C00, /* .#..##.......... */
        /* row 13 */ 0x0C00, /* ....##.......... */
        /* row 14 */ 0x0C00, /* ....##.......... */
        /* row 15 */ 0x0000  /* ................ */
    },
    /* mask[16] -- opaque region (1px dilation of data; mask&~data = white) */
    {
        /* row  0 */ 0xE000,
        /* row  1 */ 0xF000,
        /* row  2 */ 0xF800,
        /* row  3 */ 0xFC00,
        /* row  4 */ 0xFE00,
        /* row  5 */ 0xFF00,
        /* row  6 */ 0xFF80,
        /* row  7 */ 0xFFC0,
        /* row  8 */ 0xFFE0,
        /* row  9 */ 0xFFE0,
        /* row 10 */ 0xFFE0,
        /* row 11 */ 0xFE00,
        /* row 12 */ 0xFE00,
        /* row 13 */ 0xFE00,
        /* row 14 */ 0x1E00,
        /* row 15 */ 0x1E00
    },
    /* hot_row */ 1,
    /* hot_col */ 1
};


/* =========================================================================
 * Canon-assertion helpers (for use by the canon oracle, beads k8o5.10)
 * =========================================================================
 *
 * The oracle includes this header and calls:
 *   FLAIR_CURSOR_BUSY_ASSERT_CANON()  -- verifies exact hourglass bytes.
 *
 * The macro expands to a series of compile-time and run-time assertions.
 * It is NOT meant to be called in shipping OS code (it uses comparison
 * loops); it lives in the harness.
 *
 * The hourglass data bytes (frozen, ADR-0004 AM-4):
 *   {0xFFFF,0xFFFF,0x7FFE,0x3FFC,0x1FF8,0x0FF0,0x07E0,0x0180,
 *    0x0180,0x07E0,0x0FF0,0x1FF8,0x3FFC,0x7FFE,0xFFFF,0xFFFF}
 * The mask equals the data array above (solid silhouette).
 * HotSpot: row=8, col=8.
 */
#define FLAIR_CURSOR_BUSY_DATA_R0  0xFFFF
#define FLAIR_CURSOR_BUSY_DATA_R1  0xFFFF
#define FLAIR_CURSOR_BUSY_DATA_R2  0x7FFE
#define FLAIR_CURSOR_BUSY_DATA_R3  0x3FFC
#define FLAIR_CURSOR_BUSY_DATA_R4  0x1FF8
#define FLAIR_CURSOR_BUSY_DATA_R5  0x0FF0
#define FLAIR_CURSOR_BUSY_DATA_R6  0x07E0
#define FLAIR_CURSOR_BUSY_DATA_R7  0x0180
#define FLAIR_CURSOR_BUSY_DATA_R8  0x0180
#define FLAIR_CURSOR_BUSY_DATA_R9  0x07E0
#define FLAIR_CURSOR_BUSY_DATA_R10 0x0FF0
#define FLAIR_CURSOR_BUSY_DATA_R11 0x1FF8
#define FLAIR_CURSOR_BUSY_DATA_R12 0x3FFC
#define FLAIR_CURSOR_BUSY_DATA_R13 0x7FFE
#define FLAIR_CURSOR_BUSY_DATA_R14 0xFFFF
#define FLAIR_CURSOR_BUSY_DATA_R15 0xFFFF

#define FLAIR_CURSOR_BUSY_HOT_ROW  8
#define FLAIR_CURSOR_BUSY_HOT_COL  8

#endif /* INITECH_CURSORS_H */
