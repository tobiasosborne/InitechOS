/*
 * desk_icons.h -- InitechOS FLAIR desktop icon strikes v1 (LOCKED spec-data).
 *
 * beads: initech-tdnl.9 (R3 "STAPLER-DESK": the Finder desktop icon assets --
 *        the VOLUME 3.5-inch floppy and the TRASH waste basket, 32x32).
 *
 * Ref: docs/design/GUI-remediation-R3-finder-design.md F1.2 ("Sprite size:
 *      32x32 (the classic ICN# size; uncontroversial across the era).
 *      Hand-authored strikes per the glyph rule"), F1.1 (the VOLUME and TRASH
 *      icon records that consume these strikes).
 *      spec/assets/cursors.h (the 16x16 CURS data/mask idiom this format
 *      EXTENDS: MSB = col 0; a mask bit of 0 means the screen shows through).
 *      os/flair/flair_look.h (FLAIR_PART_ICON_INK / _FACE / _SHADE: the three
 *      tones are SEMANTIC roles resolved at the ONE policy seam. This file
 *      names NO color and NO palette index -- ADR-0004-AMENDMENT-DEC-09
 *      Sec 3.1 constraint C-8).
 *      CLAUDE.md Law 1 (ground truth, and honesty about where a strike came
 *      from), Law 4 (period fidelity), Rule 8 (locked spec-data), Rule 11
 *      (byte-stable / deterministic), Rule 12 (ASCII-clean).
 *
 * =========================================================================
 * PROVENANCE -- READ BEFORE ANY EDIT
 * =========================================================================
 *
 * HAND-AUTHORED clean-room ASCII pixel map (reproduced below in full); the
 * packed rows were mechanically transcribed from that map by a one-shot
 * factory transcriber; NO IMAGE EXTRACTION.
 *
 * These strikes are NOT recovered from the frame still (a dim, compressed CRT
 * photo -- pixel recovery is impossible and would be an IP violation; CLAUDE.md
 * hallucination-risk callout "Glyphs are hand-authored, not pixel-extracted"),
 * NOT lifted from any ROM, any 'ICN#' resource, or any icon-set binary. They
 * are fresh shapes authored to read as the period objects they depict -- a
 * 3.5-inch diskette and a ridged waste basket -- and to be byte-stable
 * (Rule 11). Authored 2026-08-24, bead initech-tdnl.9.
 *
 * The ASCII map in each block below IS the authored artifact and the source of
 * truth for the words; the words are its transcription. If the two ever
 * disagree, the map wins and the words are wrong. The drift tooth is
 * harness/proptest/test_desk_icons.c, whose per-row (ink, shade, opaque) table
 * and probe pixels are read off these maps by hand -- a silent edit to a word
 * that the map does not justify goes RED there.
 *
 * Changing these bytes is a DELIBERATE, issue-tracked Rule 8 act.
 *
 * =========================================================================
 * FORMAT -- FLAIRDeskIcon: 32x32, three 32-bit bitplanes
 * =========================================================================
 *
 *   mask[32]  -- uint32_t per row; bit 1 = the pixel is OPAQUE (drawn),
 *                bit 0 = TRANSPARENT (the desktop shows through).
 *   ink[32]   -- bit 1 = tone INK   (the black outline).
 *   shade[32] -- bit 1 = tone SHADE (the mid gray).
 *   MSB (bit 31) = leftmost column (col 0) -- the cursors.h convention.
 *
 * Tone decode for pixel (row r, col c), with b = 1u << (31 - c):
 *      (mask[r] & b) == 0                          -> DESK_TONE_CLEAR (skip)
 *      (mask[r] & b) && (ink[r] & b)               -> DESK_TONE_INK
 *      (mask[r] & b) && !(ink[r] & b) && (shade&b) -> DESK_TONE_SHADE
 *      (mask[r] & b) && !(ink[r] & b) && !(shade&b)-> DESK_TONE_FACE
 *
 * INVARIANTS (asserted by the oracle, not assumed):
 *      ink[r]   & ~mask[r] == 0     (no ink outside the silhouette)
 *      shade[r] & ~mask[r] == 0     (no gray outside the silhouette)
 *      ink[r]   &  shade[r] == 0    (a pixel is never two tones at once)
 *
 * WHY THREE PLANES RATHER THAN ONE PACKED 2bpp WORD PER ROW:
 *   (a) every row stays a single 32-bit word -- the ADR-0001 32-bit flat
 *       target's natural type, no 64-bit literals in freestanding kernel code;
 *   (b) the mask plane is bit-for-bit the same concept the CursorMgr already
 *       consumes from cursors.h, so the Toolbox learns one transparency rule,
 *       not two;
 *   (c) each row word lines up 1:1 with exactly one line of the ASCII map,
 *       which is what keeps the transcription auditable by eye.
 * A 2bpp packed row would have interleaved two tones into every nibble and
 * made the map-to-word correspondence unreadable, which is the property that
 * makes this locked data reviewable.
 *
 * ASCII map legend (used in every block below):
 *      '.' = clear (transparent)      '#' = ink   (black outline)
 *      'w' = white face               'g' = gray  (shade)
 *
 * All identifiers are prefixed FLAIR_ / DESK_ to stay in the InitechOS
 * namespace. Freestanding-safe: <stdint.h> only, no libc (Law 3).
 */

#ifndef INITECH_DESK_ICONS_H
#define INITECH_DESK_ICONS_H

#include <stdint.h>

/* Sprite edge, in pixels. 32x32 is the classic ICN# size (design F1.2). */
#define DESK_ICON_DIM 32

/* The four tones a strike can name. The VALUES are an internal encoding of the
 * two bitplanes; they are NOT colors and NOT palette indices (C-8). The blitter
 * maps each tone to a FLAIR_PART_* semantic role and resolves it at the ONE
 * policy seam (os/flair/finder_icon.c). */
typedef enum {
    DESK_TONE_CLEAR = 0,   /* transparent: leave the destination alone      */
    DESK_TONE_INK   = 1,   /* black outline    -> FLAIR_PART_ICON_INK       */
    DESK_TONE_FACE  = 2,   /* white body       -> FLAIR_PART_ICON_FACE      */
    DESK_TONE_SHADE = 3    /* mid-gray detail  -> FLAIR_PART_ICON_SHADE     */
} desk_tone_t;

/* A 32x32 three-plane desktop icon strike (see FORMAT above). */
typedef struct {
    uint32_t mask[DESK_ICON_DIM];   /* 1 = opaque                            */
    uint32_t ink[DESK_ICON_DIM];    /* 1 = INK tone                          */
    uint32_t shade[DESK_ICON_DIM];  /* 1 = SHADE tone                        */
} FLAIRDeskIcon;


/* =========================================================================
 * FLAIR_DESK_ICON_VOLUME -- the mounted volume: a 3.5-inch diskette
 * =========================================================================
 *
 * PROVENANCE: HAND-AUTHORED clean-room ASCII pixel map (below); packed rows
 * mechanically transcribed from the map by a one-shot factory transcriber; no
 * image extraction. Authored 2026-08-24, bead initech-tdnl.9.
 *
 * The subject is the period object the Finder's mounted-volume icon depicts
 * (design F1.1, "Volume icon ... always on-desktop"): a 3.5-inch diskette seen
 * square-on -- square body with the anti-mis-insertion bevel cut into the
 * top-right corner, the metal shutter with its read slot across the top
 * centre, and the write-on label with its ruled lines across the bottom.
 *
 * Shape (32x32; legend '.' clear, '#' ink, 'w' white, 'g' gray):
 *
     * row  0: ................................
     * row  1: ................................
     * row  2: ................................
     * row  3: ...#######################......
     * row  4: ...#wwwwwwwwwwwwwwwwwwwwww#.....
     * row  5: ...#wwwwwww###########wwwww#....
     * row  6: ...#wwwwwww#ggggggggg#wwwwww#...
     * row  7: ...#wwwwwww#gwwwggggg#wwwwww#...
     * row  8: ...#wwwwwww#gwwwggggg#wwwwww#...
     * row  9: ...#wwwwwww#gwwwggggg#wwwwww#...
     * row 10: ...#wwwwwww#gwwwggggg#wwwwww#...
     * row 11: ...#wwwwwww#gwwwggggg#wwwwww#...
     * row 12: ...#wwwwwww#gwwwggggg#wwwwww#...
     * row 13: ...#wwwwwww#ggggggggg#wwwwww#...
     * row 14: ...#wwwwwww###########wwwwww#...
     * row 15: ...#wwwwwwwwwwwwwwwwwwwwwwww#...
     * row 16: ...#www##################www#...
     * row 17: ...#www#wwwwwwwwwwwwwwww#www#...
     * row 18: ...#www#wwwwwwwwwwwwwwww#www#...
     * row 19: ...#www#wwggggggggggggww#www#...
     * row 20: ...#www#wwwwwwwwwwwwwwww#www#...
     * row 21: ...#www#wwggggggggggggww#www#...
     * row 22: ...#www#wwwwwwwwwwwwwwww#www#...
     * row 23: ...#www#wwggggggggggggww#www#...
     * row 24: ...#www#wwwwwwwwwwwwwwww#www#...
     * row 25: ...#www#wwwwwwwwwwwwwwww#www#...
     * row 26: ...#www##################www#...
     * row 27: ...#wwwwwwwwwwwwwwwwwwwwwwww#...
     * row 28: ...##########################...
     * row 29: ................................
     * row 30: ................................
     * row 31: ................................
 *
 * Worked bit arithmetic -- ONE row shown; the other 31 rows follow the
 * identical rule and were transcribed mechanically by the same pass:
 *   row 3 is the body's top edge: ink runs cols 3..25 inclusive.
 *   bit(c) = 1u << (31 - c), so the run spans bit 28 (col 3) down to
 *   bit 6 (col 25):
 *       word = 2^29 - 2^6 = 536870912 - 64 = 536870848 = 0x1FFFFFC0
 *   The row is fully opaque ink, so mask[3] = ink[3] = 0x1FFFFFC0 and
 *   shade[3] = 0x00000000.
 */
static const FLAIRDeskIcon FLAIR_DESK_ICON_VOLUME = {
    /* mask[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x00000000u,  /* ................................ */
        /* row  3 */ 0x1FFFFFC0u,  /* ...#######################...... */
        /* row  4 */ 0x1FFFFFE0u,  /* ...#wwwwwwwwwwwwwwwwwwwwww#..... */
        /* row  5 */ 0x1FFFFFF0u,  /* ...#wwwwwww###########wwwww#.... */
        /* row  6 */ 0x1FFFFFF8u,  /* ...#wwwwwww#ggggggggg#wwwwww#... */
        /* row  7 */ 0x1FFFFFF8u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row  8 */ 0x1FFFFFF8u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row  9 */ 0x1FFFFFF8u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row 10 */ 0x1FFFFFF8u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row 11 */ 0x1FFFFFF8u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row 12 */ 0x1FFFFFF8u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row 13 */ 0x1FFFFFF8u,  /* ...#wwwwwww#ggggggggg#wwwwww#... */
        /* row 14 */ 0x1FFFFFF8u,  /* ...#wwwwwww###########wwwwww#... */
        /* row 15 */ 0x1FFFFFF8u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwww#... */
        /* row 16 */ 0x1FFFFFF8u,  /* ...#www##################www#... */
        /* row 17 */ 0x1FFFFFF8u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 18 */ 0x1FFFFFF8u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 19 */ 0x1FFFFFF8u,  /* ...#www#wwggggggggggggww#www#... */
        /* row 20 */ 0x1FFFFFF8u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 21 */ 0x1FFFFFF8u,  /* ...#www#wwggggggggggggww#www#... */
        /* row 22 */ 0x1FFFFFF8u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 23 */ 0x1FFFFFF8u,  /* ...#www#wwggggggggggggww#www#... */
        /* row 24 */ 0x1FFFFFF8u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 25 */ 0x1FFFFFF8u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 26 */ 0x1FFFFFF8u,  /* ...#www##################www#... */
        /* row 27 */ 0x1FFFFFF8u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwww#... */
        /* row 28 */ 0x1FFFFFF8u,  /* ...##########################... */
        /* row 29 */ 0x00000000u,  /* ................................ */
        /* row 30 */ 0x00000000u,  /* ................................ */
        /* row 31 */ 0x00000000u  /* ................................ */
    },
    /* ink[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x00000000u,  /* ................................ */
        /* row  3 */ 0x1FFFFFC0u,  /* ...#######################...... */
        /* row  4 */ 0x10000020u,  /* ...#wwwwwwwwwwwwwwwwwwwwww#..... */
        /* row  5 */ 0x101FFC10u,  /* ...#wwwwwww###########wwwww#.... */
        /* row  6 */ 0x10100408u,  /* ...#wwwwwww#ggggggggg#wwwwww#... */
        /* row  7 */ 0x10100408u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row  8 */ 0x10100408u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row  9 */ 0x10100408u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row 10 */ 0x10100408u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row 11 */ 0x10100408u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row 12 */ 0x10100408u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row 13 */ 0x10100408u,  /* ...#wwwwwww#ggggggggg#wwwwww#... */
        /* row 14 */ 0x101FFC08u,  /* ...#wwwwwww###########wwwwww#... */
        /* row 15 */ 0x10000008u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwww#... */
        /* row 16 */ 0x11FFFF88u,  /* ...#www##################www#... */
        /* row 17 */ 0x11000088u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 18 */ 0x11000088u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 19 */ 0x11000088u,  /* ...#www#wwggggggggggggww#www#... */
        /* row 20 */ 0x11000088u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 21 */ 0x11000088u,  /* ...#www#wwggggggggggggww#www#... */
        /* row 22 */ 0x11000088u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 23 */ 0x11000088u,  /* ...#www#wwggggggggggggww#www#... */
        /* row 24 */ 0x11000088u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 25 */ 0x11000088u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 26 */ 0x11FFFF88u,  /* ...#www##################www#... */
        /* row 27 */ 0x10000008u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwww#... */
        /* row 28 */ 0x1FFFFFF8u,  /* ...##########################... */
        /* row 29 */ 0x00000000u,  /* ................................ */
        /* row 30 */ 0x00000000u,  /* ................................ */
        /* row 31 */ 0x00000000u  /* ................................ */
    },
    /* shade[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x00000000u,  /* ................................ */
        /* row  3 */ 0x00000000u,  /* ...#######################...... */
        /* row  4 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwww#..... */
        /* row  5 */ 0x00000000u,  /* ...#wwwwwww###########wwwww#.... */
        /* row  6 */ 0x000FF800u,  /* ...#wwwwwww#ggggggggg#wwwwww#... */
        /* row  7 */ 0x0008F800u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row  8 */ 0x0008F800u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row  9 */ 0x0008F800u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row 10 */ 0x0008F800u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row 11 */ 0x0008F800u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row 12 */ 0x0008F800u,  /* ...#wwwwwww#gwwwggggg#wwwwww#... */
        /* row 13 */ 0x000FF800u,  /* ...#wwwwwww#ggggggggg#wwwwww#... */
        /* row 14 */ 0x00000000u,  /* ...#wwwwwww###########wwwwww#... */
        /* row 15 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwww#... */
        /* row 16 */ 0x00000000u,  /* ...#www##################www#... */
        /* row 17 */ 0x00000000u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 18 */ 0x00000000u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 19 */ 0x003FFC00u,  /* ...#www#wwggggggggggggww#www#... */
        /* row 20 */ 0x00000000u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 21 */ 0x003FFC00u,  /* ...#www#wwggggggggggggww#www#... */
        /* row 22 */ 0x00000000u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 23 */ 0x003FFC00u,  /* ...#www#wwggggggggggggww#www#... */
        /* row 24 */ 0x00000000u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 25 */ 0x00000000u,  /* ...#www#wwwwwwwwwwwwwwww#www#... */
        /* row 26 */ 0x00000000u,  /* ...#www##################www#... */
        /* row 27 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwww#... */
        /* row 28 */ 0x00000000u,  /* ...##########################... */
        /* row 29 */ 0x00000000u,  /* ................................ */
        /* row 30 */ 0x00000000u,  /* ................................ */
        /* row 31 */ 0x00000000u  /* ................................ */
    }
};


/* =========================================================================
 * FLAIR_DESK_ICON_TRASH -- the Trash: a ridged waste basket (EMPTY variant)
 * =========================================================================
 *
 * PROVENANCE: HAND-AUTHORED clean-room ASCII pixel map (below); packed rows
 * mechanically transcribed from the map by a one-shot factory transcriber; no
 * image extraction. Authored 2026-08-24, bead initech-tdnl.9.
 *
 * The subject is the period desk object (design F1.1, "Trash icon ... fixed
 * bottom-right of the desktop"): a lidded, ridged waste basket seen square-on
 * -- arched lid handle, a slab lid, and a tapered body whose ridges run
 * vertically down the walls. This slice ships the EMPTY variant only; the
 * bulging FULL variant is a later bead's asset (design F1.4 Trash semantics).
 *
 * Shape (32x32; legend '.' clear, '#' ink, 'w' white, 'g' gray):
 *
     * row  0: ................................
     * row  1: ................................
     * row  2: .............######.............
     * row  3: .............#....#.............
     * row  4: .......##################.......
     * row  5: .......#gggggggggggggggg#.......
     * row  6: .......##################.......
     * row  7: .........##############.........
     * row  8: .........#wwwwwwwwwwww#.........
     * row  9: .........#wwwgwwgwwgww#.........
     * row 10: .........#wwwgwwgwwgww#.........
     * row 11: .........#wwwgwwgwwgww#.........
     * row 12: .........#wwwgwwgwwgww#.........
     * row 13: .........#wwwgwwgwwgww#.........
     * row 14: .........#wwwgwwgwwgww#.........
     * row 15: ..........#wwgwwgwwgw#..........
     * row 16: ..........#wwgwwgwwgw#..........
     * row 17: ..........#wwgwwgwwgw#..........
     * row 18: ..........#wwgwwgwwgw#..........
     * row 19: ..........#wwgwwgwwgw#..........
     * row 20: ..........#wwgwwgwwgw#..........
     * row 21: ..........#wwgwwgwwgw#..........
     * row 22: ...........#wgwwgwwg#...........
     * row 23: ...........#wgwwgwwg#...........
     * row 24: ...........#wgwwgwwg#...........
     * row 25: ...........#wgwwgwwg#...........
     * row 26: ...........#wgwwgwwg#...........
     * row 27: ...........#wgwwgwwg#...........
     * row 28: ...........#wwwwwwww#...........
     * row 29: ...........##########...........
     * row 30: ................................
     * row 31: ................................
 *
 * Worked bit arithmetic -- ONE row shown; the other 31 rows follow the
 * identical rule and were transcribed mechanically by the same pass:
 *   row 29 is the basket's base: ink runs cols 11..20 inclusive.
 *   bit(c) = 1u << (31 - c), so the run spans bit 20 (col 11) down to
 *   bit 11 (col 20):
 *       word = 2^21 - 2^11 = 2097152 - 2048 = 2095104 = 0x001FF800
 *   The row is fully opaque ink, so mask[29] = ink[29] = 0x001FF800 and
 *   shade[29] = 0x00000000.
 */
static const FLAIRDeskIcon FLAIR_DESK_ICON_TRASH = {
    /* mask[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x0007E000u,  /* .............######............. */
        /* row  3 */ 0x00042000u,  /* .............#....#............. */
        /* row  4 */ 0x01FFFF80u,  /* .......##################....... */
        /* row  5 */ 0x01FFFF80u,  /* .......#gggggggggggggggg#....... */
        /* row  6 */ 0x01FFFF80u,  /* .......##################....... */
        /* row  7 */ 0x007FFE00u,  /* .........##############......... */
        /* row  8 */ 0x007FFE00u,  /* .........#wwwwwwwwwwww#......... */
        /* row  9 */ 0x007FFE00u,  /* .........#wwwgwwgwwgww#......... */
        /* row 10 */ 0x007FFE00u,  /* .........#wwwgwwgwwgww#......... */
        /* row 11 */ 0x007FFE00u,  /* .........#wwwgwwgwwgww#......... */
        /* row 12 */ 0x007FFE00u,  /* .........#wwwgwwgwwgww#......... */
        /* row 13 */ 0x007FFE00u,  /* .........#wwwgwwgwwgww#......... */
        /* row 14 */ 0x007FFE00u,  /* .........#wwwgwwgwwgww#......... */
        /* row 15 */ 0x003FFC00u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 16 */ 0x003FFC00u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 17 */ 0x003FFC00u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 18 */ 0x003FFC00u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 19 */ 0x003FFC00u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 20 */ 0x003FFC00u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 21 */ 0x003FFC00u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 22 */ 0x001FF800u,  /* ...........#wgwwgwwg#........... */
        /* row 23 */ 0x001FF800u,  /* ...........#wgwwgwwg#........... */
        /* row 24 */ 0x001FF800u,  /* ...........#wgwwgwwg#........... */
        /* row 25 */ 0x001FF800u,  /* ...........#wgwwgwwg#........... */
        /* row 26 */ 0x001FF800u,  /* ...........#wgwwgwwg#........... */
        /* row 27 */ 0x001FF800u,  /* ...........#wgwwgwwg#........... */
        /* row 28 */ 0x001FF800u,  /* ...........#wwwwwwww#........... */
        /* row 29 */ 0x001FF800u,  /* ...........##########........... */
        /* row 30 */ 0x00000000u,  /* ................................ */
        /* row 31 */ 0x00000000u  /* ................................ */
    },
    /* ink[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x0007E000u,  /* .............######............. */
        /* row  3 */ 0x00042000u,  /* .............#....#............. */
        /* row  4 */ 0x01FFFF80u,  /* .......##################....... */
        /* row  5 */ 0x01000080u,  /* .......#gggggggggggggggg#....... */
        /* row  6 */ 0x01FFFF80u,  /* .......##################....... */
        /* row  7 */ 0x007FFE00u,  /* .........##############......... */
        /* row  8 */ 0x00400200u,  /* .........#wwwwwwwwwwww#......... */
        /* row  9 */ 0x00400200u,  /* .........#wwwgwwgwwgww#......... */
        /* row 10 */ 0x00400200u,  /* .........#wwwgwwgwwgww#......... */
        /* row 11 */ 0x00400200u,  /* .........#wwwgwwgwwgww#......... */
        /* row 12 */ 0x00400200u,  /* .........#wwwgwwgwwgww#......... */
        /* row 13 */ 0x00400200u,  /* .........#wwwgwwgwwgww#......... */
        /* row 14 */ 0x00400200u,  /* .........#wwwgwwgwwgww#......... */
        /* row 15 */ 0x00200400u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 16 */ 0x00200400u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 17 */ 0x00200400u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 18 */ 0x00200400u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 19 */ 0x00200400u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 20 */ 0x00200400u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 21 */ 0x00200400u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 22 */ 0x00100800u,  /* ...........#wgwwgwwg#........... */
        /* row 23 */ 0x00100800u,  /* ...........#wgwwgwwg#........... */
        /* row 24 */ 0x00100800u,  /* ...........#wgwwgwwg#........... */
        /* row 25 */ 0x00100800u,  /* ...........#wgwwgwwg#........... */
        /* row 26 */ 0x00100800u,  /* ...........#wgwwgwwg#........... */
        /* row 27 */ 0x00100800u,  /* ...........#wgwwgwwg#........... */
        /* row 28 */ 0x00100800u,  /* ...........#wwwwwwww#........... */
        /* row 29 */ 0x001FF800u,  /* ...........##########........... */
        /* row 30 */ 0x00000000u,  /* ................................ */
        /* row 31 */ 0x00000000u  /* ................................ */
    },
    /* shade[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x00000000u,  /* .............######............. */
        /* row  3 */ 0x00000000u,  /* .............#....#............. */
        /* row  4 */ 0x00000000u,  /* .......##################....... */
        /* row  5 */ 0x00FFFF00u,  /* .......#gggggggggggggggg#....... */
        /* row  6 */ 0x00000000u,  /* .......##################....... */
        /* row  7 */ 0x00000000u,  /* .........##############......... */
        /* row  8 */ 0x00000000u,  /* .........#wwwwwwwwwwww#......... */
        /* row  9 */ 0x00049000u,  /* .........#wwwgwwgwwgww#......... */
        /* row 10 */ 0x00049000u,  /* .........#wwwgwwgwwgww#......... */
        /* row 11 */ 0x00049000u,  /* .........#wwwgwwgwwgww#......... */
        /* row 12 */ 0x00049000u,  /* .........#wwwgwwgwwgww#......... */
        /* row 13 */ 0x00049000u,  /* .........#wwwgwwgwwgww#......... */
        /* row 14 */ 0x00049000u,  /* .........#wwwgwwgwwgww#......... */
        /* row 15 */ 0x00049000u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 16 */ 0x00049000u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 17 */ 0x00049000u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 18 */ 0x00049000u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 19 */ 0x00049000u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 20 */ 0x00049000u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 21 */ 0x00049000u,  /* ..........#wwgwwgwwgw#.......... */
        /* row 22 */ 0x00049000u,  /* ...........#wgwwgwwg#........... */
        /* row 23 */ 0x00049000u,  /* ...........#wgwwgwwg#........... */
        /* row 24 */ 0x00049000u,  /* ...........#wgwwgwwg#........... */
        /* row 25 */ 0x00049000u,  /* ...........#wgwwgwwg#........... */
        /* row 26 */ 0x00049000u,  /* ...........#wgwwgwwg#........... */
        /* row 27 */ 0x00049000u,  /* ...........#wgwwgwwg#........... */
        /* row 28 */ 0x00000000u,  /* ...........#wwwwwwww#........... */
        /* row 29 */ 0x00000000u,  /* ...........##########........... */
        /* row 30 */ 0x00000000u,  /* ................................ */
        /* row 31 */ 0x00000000u  /* ................................ */
    }
};

#endif /* INITECH_DESK_ICONS_H */
