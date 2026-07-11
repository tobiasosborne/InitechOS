/*
 * apple_glyph.h -- InitechOS FLAIR Apple-menu glyph strike v0 (LOCKED asset).
 *
 * beads: initech-yx4v ("Apple menu slot rendered as a solid black filled
 *        square, not an apple glyph"). Ref: os/flair/menu.h (FLAIR_MENU_APPLE_W
 *        / FLAIR_MENU_TITLE_VPAD -- the Apple slot is exactly APPLE_GLYPH_W x
 *        APPLE_GLYPH_H = 16x15 px, top-left at (FLAIR_MENU_TITLE_VPAD,
 *        FLAIR_MENU_TITLE_VPAD) in the bar); os/flair/menu.c DrawMenuBar (the
 *        consumer -- a masked/transparent blit, ink-on-bar, NOT a filled rect).
 *        CLAUDE.md hallucination-risk callout "Glyphs are hand-authored, not
 *        pixel-extracted" (this file follows the SAME provenance discipline as
 *        spec/assets/chicago8x16.h / geneva9.h / cursors.h); Law 1 (ground
 *        truth before code); Rule 8 (locked spec-data); Rule 11 (byte-stable,
 *        no timestamps); Rule 12 (ASCII-clean; this file is bit-pattern data
 *        with ASCII-art comments only).
 *
 * PROVENANCE (Law 1 honesty) -- READ BEFORE ANY EDIT:
 *
 * The bit pattern below was HAND-AUTHORED, row by row, to match the classic
 * "apple with a bite taken out of the right side, plus a leaf" silhouette used
 * by the System-7-era Apple-menu glyph -- a widely-recognized, simple geometric
 * shape (leaf top-right, a two-lobe dip at the top of the body, a bite notch
 * on the upper-right of the body, a rounded taper to a two-foot base with a
 * center dimple). It was NOT pixel-extracted from any frame still or capture.
 *
 * BACK-CHECK (not extraction): after hand-authoring, the strike was visually
 * compared against two INDEPENDENT reference captures that are NOT part of
 * this codebase's render path (so they cannot agree with the artifact "by
 * construction", Law 2):
 *   ../system7-decomp/goldens/captures/s7_701_8bit_docwindow.png (top-left of
 *     the small "m" disk-image window's menu bar, ~ x[16,32) y[0,20))
 *   ../system7-decomp/goldens/captures/cal_filemenu.png (top-left of the
 *     desktop menu bar, same crop window)
 * Both captures show the SAME canon proportions (confirmed by eye, cropped and
 * pixel-gridded for counting, NOT copied bit-for-bit into this file): a small
 * leaf bending down-left from a point in the upper right, a shallow two-lobe
 * dip at the top center of the body where the leaf attaches, a receding notch
 * (the "bite") on the upper-right flank of the body about 1/3 of the way down,
 * a bulge that is WIDEST a few rows below the bite, then a taper down to a
 * base with a small center dimple/cleft. This file's strike reproduces those
 * PROPORTIONS and RECOGNIZABLE FEATURES from a fresh, independently-drawn bit
 * pattern -- it is not a pixel-for-pixel transcription of either PNG (compare
 * row-by-row against the captures above and you will find differences in the
 * exact column offsets; the FEATURES -- leaf, top dip, bite, taper, base
 * dimple -- are what was matched, per the same "match by hand, back-check"
 * discipline as chicago8x16.h / geneva9.h).
 *
 * Ink coverage of the authored strike is ~39% of the 16x15 = 240-pixel cell
 * (94 ink pixels) -- inside the property oracle's asserted 25-60% band
 * (harness/proptest/test_menu.c "APPLE GLYPH" property): dense enough to read
 * as a solid-ish silhouette (like the real glyph), sparse enough that it is
 * NOT the old solid-square bug (initech-yx4v) it replaces.
 *
 * FORMAT (v0; mirrors spec/assets/cursors.h's CURS-style row format, generalized
 * to a THE bar's 16px Apple slot rather than a 16x16 cursor):
 *   - APPLE_GLYPH_W = 16, APPLE_GLYPH_H = 15 (the Apple slot's exact pixel size;
 *     os/flair/menu.h: FLAIR_MENU_APPLE_W - 2*FLAIR_MENU_TITLE_VPAD == 16,
 *     FLAIR_MENUBAR_H - 2*FLAIR_MENU_TITLE_VPAD - 1 == 15).
 *   - APPLE_GLYPH_ROWS[15] -- one uint16_t per row, top to bottom. MSB (bit
 *     0x8000) is the LEFTMOST column (col 0); a set bit is INK, a clear bit is
 *     BACKGROUND (transparent -- the bar fill shows through; menu.c's masked
 *     blit draws ONLY the set bits, in the bar's ink color).
 *   - Byte-stable (Rule 11): pure static const data, no timestamps.
 *
 * ASCII-art key (ROW COMMENTS): '#' = ink (set bit), '.' = background. Each
 * row shown as 16 columns (0..15) left to right. The leaf is rows 0-2; the top
 * dip (two lobes meeting the leaf) is row 3; the body is rows 4-13 (with the
 * bite notch at rows 6-7, receding on the right flank vs rows 4-5/8-10); the
 * base + center dimple is row 14.
 */
#ifndef INITECH_APPLE_GLYPH_H
#define INITECH_APPLE_GLYPH_H

#include <stdint.h>

#define APPLE_GLYPH_W  16   /* slot width  (FLAIR_MENU_APPLE_W - 2*VPAD)  */
#define APPLE_GLYPH_H  15   /* slot height (FLAIR_MENUBAR_H - 2*VPAD - 1) */

/*
 * APPLE_GLYPH_ROWS -- the hand-authored apple-with-bite silhouette, one
 * uint16_t per row (MSB = col 0). See PROVENANCE above: hand-authored,
 * back-checked against s7_701_8bit_docwindow.png / cal_filemenu.png, NOT
 * pixel-extracted.
 */
static const uint16_t APPLE_GLYPH_ROWS[APPLE_GLYPH_H] = {
    /* row  0 */ 0x00C0, /* ........##...... -- leaf tip                    */
    /* row  1 */ 0x0180, /* .......##....... -- leaf mid                   */
    /* row  2 */ 0x0200, /* ......#......... -- leaf stem into the body    */
    /* row  3 */ 0x1C70, /* ...###...###.... -- top dip: two lobes         */
    /* row  4 */ 0x1FF0, /* ...#########.... -- body (pre-bite)            */
    /* row  5 */ 0x1FF0, /* ...#########.... -- body (pre-bite)            */
    /* row  6 */ 0x1FC0, /* ...#######...... -- BITE: right flank recedes  */
    /* row  7 */ 0x1FC0, /* ...#######...... -- BITE: right flank recedes  */
    /* row  8 */ 0x1FF0, /* ...#########.... -- body (post-bite, puffs out)*/
    /* row  9 */ 0x3FF0, /* ..##########.... -- widest row (left bulges)   */
    /* row 10 */ 0x1FF0, /* ...#########.... -- body                       */
    /* row 11 */ 0x0FE0, /* ....#######..... -- taper                      */
    /* row 12 */ 0x0FE0, /* ....#######..... -- taper                      */
    /* row 13 */ 0x07C0, /* .....#####...... -- taper                      */
    /* row 14 */ 0x0C60, /* ....##...##..... -- base: center dimple        */
};

/*
 * APPLE_GLYPH_BITE_ROW / _COL -- a named probe cell that is INK a few rows
 * above (row 4, the pre-bite body: cols 3-11 are ink) but BACKGROUND at the
 * bite notch itself: row 6, col 10 (0-based; row 6 is only cols 3-9 ink, so
 * col 10 recedes). Under the old solid-square bug this pixel was ink (idx
 * CIDX_BLACK) at every row; under the authored glyph it is background at row
 * 6 while still inside the glyph's overall bounding box. Named so the oracle
 * (test_menu.c) and any reader can cite the exact cell without re-deriving it
 * from the row table by eye.
 */
#define APPLE_GLYPH_BITE_ROW       6
#define APPLE_GLYPH_BITE_COL       10
#define APPLE_GLYPH_PREBITE_ROW    4   /* same column, ink (pre-bite body) */

/* APPLE_GLYPH_LEAF_ROW/_COL -- the leaf-tip ink cell (row 0, col 9): proves
 * ink exists ABOVE the body (the leaf), not just a filled lower block. */
#define APPLE_GLYPH_LEAF_ROW  0
#define APPLE_GLYPH_LEAF_COL  9

/* APPLE_GLYPH_WIDEST_ROW -- the row with the most ink pixels (row 9, 10 wide):
 * used by the oracle to confirm the body is denser than the leaf tip. */
#define APPLE_GLYPH_WIDEST_ROW  9

#endif /* INITECH_APPLE_GLYPH_H */
