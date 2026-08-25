/*
 * finder_icons.h -- InitechOS FLAIR disk-window icon strikes v1 (LOCKED
 * spec-data).
 *
 * beads: initech-tdnl.10 (R3.3 "disk windows": the three icon kinds a disk
 *        window can show -- FOLDER, DOCUMENT and APPLICATION, 32x32).
 *
 * Ref: docs/design/GUI-remediation-R3-finder-design.md F1.1 (the icon-kind
 *      heuristic these three strikes serve: "DIR_ATTR_DIRECTORY => folder";
 *      "ICON_APP is a display classification of ICON_FILE whose 8.3 name ends
 *      in .EXE"; everything else is a document), F1.2 ("Sprite size: 32x32
 *      (the classic ICN# size; uncontroversial across the era). Hand-authored
 *      strikes per the glyph rule"), F3.1/F3.3 (the disk-window enumeration
 *      that produces the records).
 *      spec/assets/desk_icons.h -- the SIBLING file this one EXTENDS. It owns
 *      the FLAIRDeskIcon type, the three-bitplane FORMAT, the tone decode, the
 *      invariants, and the VOLUME + TRASH strikes; this file adds three more
 *      strikes in that exact format and adds NOTHING to the contract. It is a
 *      separate file rather than an edit to desk_icons.h so the R3.2 locked
 *      bytes stay untouched (Rule 8: changing locked spec-data is a deliberate,
 *      issue-tracked act -- ADDING a sibling is the cheaper honest move).
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
 * tab-top manila folder, a dog-eared sheet of paper, and the generic-program
 * diamond -- and to be byte-stable (Rule 11). Authored 2026-08-25, bead
 * initech-tdnl.10.
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
 * FORMAT + TONE DECODE + INVARIANTS: see spec/assets/desk_icons.h. Verbatim:
 * three 32-bit bitplanes per strike (mask / ink / shade), MSB = column 0, a
 * mask bit of 0 means the desktop shows through, and for every row
 *      ink & ~mask == 0,   shade & ~mask == 0,   ink & shade == 0.
 *
 * ASCII map legend (used in every block below):
 *      '.' = clear (transparent)      '#' = ink   (black outline)
 *      'w' = white face               'g' = gray  (shade)
 *
 * Freestanding-safe: <stdint.h> + desk_icons.h only, no libc (Law 3).
 */

#ifndef INITECH_FINDER_ICONS_H
#define INITECH_FINDER_ICONS_H

#include <stdint.h>

#include "desk_icons.h"   /* FLAIRDeskIcon, DESK_ICON_DIM, the tone decode */

/* =========================================================================
 * FLAIR_FINDER_ICON_FOLDER -- the tab-top manila FOLDER
 * =========================================================================
 *
 * PROVENANCE: HAND-AUTHORED clean-room ASCII pixel map (below); packed rows
 * mechanically transcribed from the map by a one-shot factory transcriber; no
 * image extraction. Authored 2026-08-25, bead initech-tdnl.10.
 *
 * The subject is the period desk object every windowed file system used for a
 * DIRECTORY (design F1.1, "DIR_ATTR_DIRECTORY => folder"): a manila folder seen
 * square-on -- the index tab on the top-left, the front flap edge ruled across
 * the body, and the flap itself shaded where it catches less light.
 *
 * Worked bit arithmetic -- ONE row shown; the other 31 rows follow the
 * identical rule and were transcribed mechanically by the same pass:
 *   row 7 is the front-flap edge line: ink runs cols 3..29 inclusive.
 *   bit(c) = 1u << (31 - c), so the run spans bit 28 (col 3) down to
 *   bit 2 (col 29):
 *       word = 2^29 - 2^2 = 536870912 - 4 = 536870908 = 0x1FFFFFFC
 *   The row is fully opaque ink, so mask[7] = ink[7] = 0x1FFFFFFC and
 *   shade[7] = 0x00000000.
 *
 * Shape (32x32; legend '.' clear, '#' ink, 'w' white, 'g' gray):
 *
     * row  0: ................................
     * row  1: ................................
     * row  2: ................................
     * row  3: ...##########...................
     * row  4: ...#wwwwwwww#...................
     * row  5: ...#wwwwwwww##################..
     * row  6: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row  7: ...###########################..
     * row  8: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row  9: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 10: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 11: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 12: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 13: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 14: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 15: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 16: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 17: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 18: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 19: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 20: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 21: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 22: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 23: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 24: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
     * row 25: ...#ggggggggggggggggggggggggg#..
     * row 26: ...#ggggggggggggggggggggggggg#..
     * row 27: ...###########################..
     * row 28: ................................
     * row 29: ................................
     * row 30: ................................
     * row 31: ................................
 */
static const FLAIRDeskIcon FLAIR_FINDER_ICON_FOLDER = {
    /* mask[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x00000000u,  /* ................................ */
        /* row  3 */ 0x1FF80000u,  /* ...##########................... */
        /* row  4 */ 0x1FF80000u,  /* ...#wwwwwwww#................... */
        /* row  5 */ 0x1FFFFFFCu,  /* ...#wwwwwwww##################.. */
        /* row  6 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row  7 */ 0x1FFFFFFCu,  /* ...###########################.. */
        /* row  8 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row  9 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 10 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 11 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 12 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 13 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 14 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 15 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 16 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 17 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 18 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 19 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 20 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 21 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 22 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 23 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 24 */ 0x1FFFFFFCu,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 25 */ 0x1FFFFFFCu,  /* ...#ggggggggggggggggggggggggg#.. */
        /* row 26 */ 0x1FFFFFFCu,  /* ...#ggggggggggggggggggggggggg#.. */
        /* row 27 */ 0x1FFFFFFCu,  /* ...###########################.. */
        /* row 28 */ 0x00000000u,  /* ................................ */
        /* row 29 */ 0x00000000u,  /* ................................ */
        /* row 30 */ 0x00000000u,  /* ................................ */
        /* row 31 */ 0x00000000u  /* ................................ */
    },
    /* ink[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x00000000u,  /* ................................ */
        /* row  3 */ 0x1FF80000u,  /* ...##########................... */
        /* row  4 */ 0x10080000u,  /* ...#wwwwwwww#................... */
        /* row  5 */ 0x100FFFFCu,  /* ...#wwwwwwww##################.. */
        /* row  6 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row  7 */ 0x1FFFFFFCu,  /* ...###########################.. */
        /* row  8 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row  9 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 10 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 11 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 12 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 13 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 14 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 15 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 16 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 17 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 18 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 19 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 20 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 21 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 22 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 23 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 24 */ 0x10000004u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 25 */ 0x10000004u,  /* ...#ggggggggggggggggggggggggg#.. */
        /* row 26 */ 0x10000004u,  /* ...#ggggggggggggggggggggggggg#.. */
        /* row 27 */ 0x1FFFFFFCu,  /* ...###########################.. */
        /* row 28 */ 0x00000000u,  /* ................................ */
        /* row 29 */ 0x00000000u,  /* ................................ */
        /* row 30 */ 0x00000000u,  /* ................................ */
        /* row 31 */ 0x00000000u  /* ................................ */
    },
    /* shade[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x00000000u,  /* ................................ */
        /* row  3 */ 0x00000000u,  /* ...##########................... */
        /* row  4 */ 0x00000000u,  /* ...#wwwwwwww#................... */
        /* row  5 */ 0x00000000u,  /* ...#wwwwwwww##################.. */
        /* row  6 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row  7 */ 0x00000000u,  /* ...###########################.. */
        /* row  8 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row  9 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 10 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 11 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 12 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 13 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 14 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 15 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 16 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 17 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 18 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 19 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 20 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 21 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 22 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 23 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 24 */ 0x00000000u,  /* ...#wwwwwwwwwwwwwwwwwwwwwwwww#.. */
        /* row 25 */ 0x0FFFFFF8u,  /* ...#ggggggggggggggggggggggggg#.. */
        /* row 26 */ 0x0FFFFFF8u,  /* ...#ggggggggggggggggggggggggg#.. */
        /* row 27 */ 0x00000000u,  /* ...###########################.. */
        /* row 28 */ 0x00000000u,  /* ................................ */
        /* row 29 */ 0x00000000u,  /* ................................ */
        /* row 30 */ 0x00000000u,  /* ................................ */
        /* row 31 */ 0x00000000u  /* ................................ */
    }
};

/* =========================================================================
 * FLAIR_FINDER_ICON_DOC -- the dog-eared page (DOCUMENT)
 * =========================================================================
 *
 * PROVENANCE: HAND-AUTHORED clean-room ASCII pixel map (below); packed rows
 * mechanically transcribed from the map by a one-shot factory transcriber; no
 * image extraction. Authored 2026-08-25, bead initech-tdnl.10.
 *
 * The subject is the period icon for a plain FILE (design F1.1, "else
 * document"): a sheet of paper seen square-on with its top-right corner turned
 * down -- the dog-ear whose folded-back triangle is the mid gray, the paper
 * BEYOND the fold transparent (that corner of the sheet is simply not there),
 * and five ruled text lines across the body.
 *
 * Worked bit arithmetic -- ONE row shown; the other 31 rows follow the
 * identical rule and were transcribed mechanically by the same pass:
 *   row 29 is the page bottom edge: ink runs cols 6..25 inclusive.
 *   bit(c) = 1u << (31 - c), so the run spans bit 25 (col 6) down to
 *   bit 6 (col 25):
 *       word = 2^26 - 2^6 = 67108864 - 64 = 67108800 = 0x03FFFFC0
 *   The row is fully opaque ink, so mask[29] = ink[29] = 0x03FFFFC0 and
 *   shade[29] = 0x00000000.
 *
 * Shape (32x32; legend '.' clear, '#' ink, 'w' white, 'g' gray):
 *
     * row  0: ................................
     * row  1: ................................
     * row  2: ......###############...........
     * row  3: ......#wwwwwwwwwwwww##..........
     * row  4: ......#wwwwwwwwwwwww#g#.........
     * row  5: ......#wwwwwwwwwwwww#gg#........
     * row  6: ......#wwwwwwwwwwwww#ggg#.......
     * row  7: ......#wwwwwwwwwwwww######......
     * row  8: ......#wwwwwwwwwwwwwwwwww#......
     * row  9: ......#wwwwwwwwwwwwwwwwww#......
     * row 10: ......#wwwwwwwwwwwwwwwwww#......
     * row 11: ......#wwwwwwwwwwwwwwwwww#......
     * row 12: ......#wwggggggggggggggww#......
     * row 13: ......#wwwwwwwwwwwwwwwwww#......
     * row 14: ......#wwwwwwwwwwwwwwwwww#......
     * row 15: ......#wwggggggggggggggww#......
     * row 16: ......#wwwwwwwwwwwwwwwwww#......
     * row 17: ......#wwwwwwwwwwwwwwwwww#......
     * row 18: ......#wwggggggggggggggww#......
     * row 19: ......#wwwwwwwwwwwwwwwwww#......
     * row 20: ......#wwwwwwwwwwwwwwwwww#......
     * row 21: ......#wwggggggggggggggww#......
     * row 22: ......#wwwwwwwwwwwwwwwwww#......
     * row 23: ......#wwwwwwwwwwwwwwwwww#......
     * row 24: ......#wwggggggggggggggww#......
     * row 25: ......#wwwwwwwwwwwwwwwwww#......
     * row 26: ......#wwwwwwwwwwwwwwwwww#......
     * row 27: ......#wwwwwwwwwwwwwwwwww#......
     * row 28: ......#wwwwwwwwwwwwwwwwww#......
     * row 29: ......####################......
     * row 30: ................................
     * row 31: ................................
 */
static const FLAIRDeskIcon FLAIR_FINDER_ICON_DOC = {
    /* mask[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x03FFF800u,  /* ......###############........... */
        /* row  3 */ 0x03FFFC00u,  /* ......#wwwwwwwwwwwww##.......... */
        /* row  4 */ 0x03FFFE00u,  /* ......#wwwwwwwwwwwww#g#......... */
        /* row  5 */ 0x03FFFF00u,  /* ......#wwwwwwwwwwwww#gg#........ */
        /* row  6 */ 0x03FFFF80u,  /* ......#wwwwwwwwwwwww#ggg#....... */
        /* row  7 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwww######...... */
        /* row  8 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row  9 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 10 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 11 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 12 */ 0x03FFFFC0u,  /* ......#wwggggggggggggggww#...... */
        /* row 13 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 14 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 15 */ 0x03FFFFC0u,  /* ......#wwggggggggggggggww#...... */
        /* row 16 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 17 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 18 */ 0x03FFFFC0u,  /* ......#wwggggggggggggggww#...... */
        /* row 19 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 20 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 21 */ 0x03FFFFC0u,  /* ......#wwggggggggggggggww#...... */
        /* row 22 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 23 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 24 */ 0x03FFFFC0u,  /* ......#wwggggggggggggggww#...... */
        /* row 25 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 26 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 27 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 28 */ 0x03FFFFC0u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 29 */ 0x03FFFFC0u,  /* ......####################...... */
        /* row 30 */ 0x00000000u,  /* ................................ */
        /* row 31 */ 0x00000000u  /* ................................ */
    },
    /* ink[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x03FFF800u,  /* ......###############........... */
        /* row  3 */ 0x02000C00u,  /* ......#wwwwwwwwwwwww##.......... */
        /* row  4 */ 0x02000A00u,  /* ......#wwwwwwwwwwwww#g#......... */
        /* row  5 */ 0x02000900u,  /* ......#wwwwwwwwwwwww#gg#........ */
        /* row  6 */ 0x02000880u,  /* ......#wwwwwwwwwwwww#ggg#....... */
        /* row  7 */ 0x02000FC0u,  /* ......#wwwwwwwwwwwww######...... */
        /* row  8 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row  9 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 10 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 11 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 12 */ 0x02000040u,  /* ......#wwggggggggggggggww#...... */
        /* row 13 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 14 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 15 */ 0x02000040u,  /* ......#wwggggggggggggggww#...... */
        /* row 16 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 17 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 18 */ 0x02000040u,  /* ......#wwggggggggggggggww#...... */
        /* row 19 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 20 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 21 */ 0x02000040u,  /* ......#wwggggggggggggggww#...... */
        /* row 22 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 23 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 24 */ 0x02000040u,  /* ......#wwggggggggggggggww#...... */
        /* row 25 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 26 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 27 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 28 */ 0x02000040u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 29 */ 0x03FFFFC0u,  /* ......####################...... */
        /* row 30 */ 0x00000000u,  /* ................................ */
        /* row 31 */ 0x00000000u  /* ................................ */
    },
    /* shade[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x00000000u,  /* ......###############........... */
        /* row  3 */ 0x00000000u,  /* ......#wwwwwwwwwwwww##.......... */
        /* row  4 */ 0x00000400u,  /* ......#wwwwwwwwwwwww#g#......... */
        /* row  5 */ 0x00000600u,  /* ......#wwwwwwwwwwwww#gg#........ */
        /* row  6 */ 0x00000700u,  /* ......#wwwwwwwwwwwww#ggg#....... */
        /* row  7 */ 0x00000000u,  /* ......#wwwwwwwwwwwww######...... */
        /* row  8 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row  9 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 10 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 11 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 12 */ 0x007FFE00u,  /* ......#wwggggggggggggggww#...... */
        /* row 13 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 14 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 15 */ 0x007FFE00u,  /* ......#wwggggggggggggggww#...... */
        /* row 16 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 17 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 18 */ 0x007FFE00u,  /* ......#wwggggggggggggggww#...... */
        /* row 19 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 20 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 21 */ 0x007FFE00u,  /* ......#wwggggggggggggggww#...... */
        /* row 22 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 23 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 24 */ 0x007FFE00u,  /* ......#wwggggggggggggggww#...... */
        /* row 25 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 26 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 27 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 28 */ 0x00000000u,  /* ......#wwwwwwwwwwwwwwwwww#...... */
        /* row 29 */ 0x00000000u,  /* ......####################...... */
        /* row 30 */ 0x00000000u,  /* ................................ */
        /* row 31 */ 0x00000000u  /* ................................ */
    }
};

/* =========================================================================
 * FLAIR_FINDER_ICON_APP -- the diamond mark (APPLICATION)
 * =========================================================================
 *
 * PROVENANCE: HAND-AUTHORED clean-room ASCII pixel map (below); packed rows
 * mechanically transcribed from the map by a one-shot factory transcriber; no
 * image extraction. Authored 2026-08-25, bead initech-tdnl.10.
 *
 * The subject is the period generic-APPLICATION mark (design F1.1, "name ends
 * .EXE => app"): the diamond every era of this desktop used to say "this is a
 * program, not a document", drawn as an outlined diamond with a smaller solid
 * diamond at its centre so the two read apart at a glance on a 640x480 screen.
 *
 * Worked bit arithmetic -- ONE row shown; the other 31 rows follow the
 * identical rule and were transcribed mechanically by the same pass:
 *   row 16 is the diamond waist: the silhouette runs cols 1..30 inclusive.
 *   bit(c) = 1u << (31 - c), so the span is bit 30 (col 1) down to bit 1
 *   (col 30):
 *       mask = 2^31 - 2^1 = 2147483648 - 2 = 2147483646 = 0x7FFFFFFE
 *   Its ink is the two outline pixels (cols 1 and 30 -> bits 30 and 1,
 *   0x40000002) OR the centre diamond (cols 10..21 -> bits 21..10,
 *   2^22 - 2^10 = 4194304 - 1024 = 4193280 = 0x003FFC00), i.e.
 *   ink[16] = 0x403FFC02 and shade[16] = 0x00000000.
 *
 * Shape (32x32; legend '.' clear, '#' ink, 'w' white, 'g' gray):
 *
     * row  0: ................................
     * row  1: ................................
     * row  2: ...............##...............
     * row  3: ..............#ww#..............
     * row  4: .............#wwww#.............
     * row  5: ............#wwwwww#............
     * row  6: ...........#wwwwwwww#...........
     * row  7: ..........#wwwwwwwwww#..........
     * row  8: .........#wwwwwwwwwwww#.........
     * row  9: ........#wwwwwwwwwwwwww#........
     * row 10: .......#wwwwwwwwwwwwwwww#.......
     * row 11: ......#wwwwwwww##wwwwwwww#......
     * row 12: .....#wwwwwwww####wwwwwwww#.....
     * row 13: ....#wwwwwwww######wwwwwwww#....
     * row 14: ...#wwwwwwww########wwwwwwww#...
     * row 15: ..#wwwwwwww##########wwwwwwww#..
     * row 16: .#wwwwwwww############wwwwwwww#.
     * row 17: ..#wwwwwwww##########wwwwwwww#..
     * row 18: ...#wwwwwwww########wwwwwwww#...
     * row 19: ....#wwwwwwww######wwwwwwww#....
     * row 20: .....#wwwwwwww####wwwwwwww#.....
     * row 21: ......#wwwwwwww##wwwwwwww#......
     * row 22: .......#wwwwwwwwwwwwwwww#.......
     * row 23: ........#wwwwwwwwwwwwww#........
     * row 24: .........#wwwwwwwwwwww#.........
     * row 25: ..........#wwwwwwwwww#..........
     * row 26: ...........#wwwwwwww#...........
     * row 27: ............#wwwwww#............
     * row 28: .............#wwww#.............
     * row 29: ..............#ww#..............
     * row 30: ...............##...............
     * row 31: ................................
 */
static const FLAIRDeskIcon FLAIR_FINDER_ICON_APP = {
    /* mask[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x00018000u,  /* ...............##............... */
        /* row  3 */ 0x0003C000u,  /* ..............#ww#.............. */
        /* row  4 */ 0x0007E000u,  /* .............#wwww#............. */
        /* row  5 */ 0x000FF000u,  /* ............#wwwwww#............ */
        /* row  6 */ 0x001FF800u,  /* ...........#wwwwwwww#........... */
        /* row  7 */ 0x003FFC00u,  /* ..........#wwwwwwwwww#.......... */
        /* row  8 */ 0x007FFE00u,  /* .........#wwwwwwwwwwww#......... */
        /* row  9 */ 0x00FFFF00u,  /* ........#wwwwwwwwwwwwww#........ */
        /* row 10 */ 0x01FFFF80u,  /* .......#wwwwwwwwwwwwwwww#....... */
        /* row 11 */ 0x03FFFFC0u,  /* ......#wwwwwwww##wwwwwwww#...... */
        /* row 12 */ 0x07FFFFE0u,  /* .....#wwwwwwww####wwwwwwww#..... */
        /* row 13 */ 0x0FFFFFF0u,  /* ....#wwwwwwww######wwwwwwww#.... */
        /* row 14 */ 0x1FFFFFF8u,  /* ...#wwwwwwww########wwwwwwww#... */
        /* row 15 */ 0x3FFFFFFCu,  /* ..#wwwwwwww##########wwwwwwww#.. */
        /* row 16 */ 0x7FFFFFFEu,  /* .#wwwwwwww############wwwwwwww#. */
        /* row 17 */ 0x3FFFFFFCu,  /* ..#wwwwwwww##########wwwwwwww#.. */
        /* row 18 */ 0x1FFFFFF8u,  /* ...#wwwwwwww########wwwwwwww#... */
        /* row 19 */ 0x0FFFFFF0u,  /* ....#wwwwwwww######wwwwwwww#.... */
        /* row 20 */ 0x07FFFFE0u,  /* .....#wwwwwwww####wwwwwwww#..... */
        /* row 21 */ 0x03FFFFC0u,  /* ......#wwwwwwww##wwwwwwww#...... */
        /* row 22 */ 0x01FFFF80u,  /* .......#wwwwwwwwwwwwwwww#....... */
        /* row 23 */ 0x00FFFF00u,  /* ........#wwwwwwwwwwwwww#........ */
        /* row 24 */ 0x007FFE00u,  /* .........#wwwwwwwwwwww#......... */
        /* row 25 */ 0x003FFC00u,  /* ..........#wwwwwwwwww#.......... */
        /* row 26 */ 0x001FF800u,  /* ...........#wwwwwwww#........... */
        /* row 27 */ 0x000FF000u,  /* ............#wwwwww#............ */
        /* row 28 */ 0x0007E000u,  /* .............#wwww#............. */
        /* row 29 */ 0x0003C000u,  /* ..............#ww#.............. */
        /* row 30 */ 0x00018000u,  /* ...............##............... */
        /* row 31 */ 0x00000000u  /* ................................ */
    },
    /* ink[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x00018000u,  /* ...............##............... */
        /* row  3 */ 0x00024000u,  /* ..............#ww#.............. */
        /* row  4 */ 0x00042000u,  /* .............#wwww#............. */
        /* row  5 */ 0x00081000u,  /* ............#wwwwww#............ */
        /* row  6 */ 0x00100800u,  /* ...........#wwwwwwww#........... */
        /* row  7 */ 0x00200400u,  /* ..........#wwwwwwwwww#.......... */
        /* row  8 */ 0x00400200u,  /* .........#wwwwwwwwwwww#......... */
        /* row  9 */ 0x00800100u,  /* ........#wwwwwwwwwwwwww#........ */
        /* row 10 */ 0x01000080u,  /* .......#wwwwwwwwwwwwwwww#....... */
        /* row 11 */ 0x02018040u,  /* ......#wwwwwwww##wwwwwwww#...... */
        /* row 12 */ 0x0403C020u,  /* .....#wwwwwwww####wwwwwwww#..... */
        /* row 13 */ 0x0807E010u,  /* ....#wwwwwwww######wwwwwwww#.... */
        /* row 14 */ 0x100FF008u,  /* ...#wwwwwwww########wwwwwwww#... */
        /* row 15 */ 0x201FF804u,  /* ..#wwwwwwww##########wwwwwwww#.. */
        /* row 16 */ 0x403FFC02u,  /* .#wwwwwwww############wwwwwwww#. */
        /* row 17 */ 0x201FF804u,  /* ..#wwwwwwww##########wwwwwwww#.. */
        /* row 18 */ 0x100FF008u,  /* ...#wwwwwwww########wwwwwwww#... */
        /* row 19 */ 0x0807E010u,  /* ....#wwwwwwww######wwwwwwww#.... */
        /* row 20 */ 0x0403C020u,  /* .....#wwwwwwww####wwwwwwww#..... */
        /* row 21 */ 0x02018040u,  /* ......#wwwwwwww##wwwwwwww#...... */
        /* row 22 */ 0x01000080u,  /* .......#wwwwwwwwwwwwwwww#....... */
        /* row 23 */ 0x00800100u,  /* ........#wwwwwwwwwwwwww#........ */
        /* row 24 */ 0x00400200u,  /* .........#wwwwwwwwwwww#......... */
        /* row 25 */ 0x00200400u,  /* ..........#wwwwwwwwww#.......... */
        /* row 26 */ 0x00100800u,  /* ...........#wwwwwwww#........... */
        /* row 27 */ 0x00081000u,  /* ............#wwwwww#............ */
        /* row 28 */ 0x00042000u,  /* .............#wwww#............. */
        /* row 29 */ 0x00024000u,  /* ..............#ww#.............. */
        /* row 30 */ 0x00018000u,  /* ...............##............... */
        /* row 31 */ 0x00000000u  /* ................................ */
    },
    /* shade[32] */
    {
        /* row  0 */ 0x00000000u,  /* ................................ */
        /* row  1 */ 0x00000000u,  /* ................................ */
        /* row  2 */ 0x00000000u,  /* ...............##............... */
        /* row  3 */ 0x00000000u,  /* ..............#ww#.............. */
        /* row  4 */ 0x00000000u,  /* .............#wwww#............. */
        /* row  5 */ 0x00000000u,  /* ............#wwwwww#............ */
        /* row  6 */ 0x00000000u,  /* ...........#wwwwwwww#........... */
        /* row  7 */ 0x00000000u,  /* ..........#wwwwwwwwww#.......... */
        /* row  8 */ 0x00000000u,  /* .........#wwwwwwwwwwww#......... */
        /* row  9 */ 0x00000000u,  /* ........#wwwwwwwwwwwwww#........ */
        /* row 10 */ 0x00000000u,  /* .......#wwwwwwwwwwwwwwww#....... */
        /* row 11 */ 0x00000000u,  /* ......#wwwwwwww##wwwwwwww#...... */
        /* row 12 */ 0x00000000u,  /* .....#wwwwwwww####wwwwwwww#..... */
        /* row 13 */ 0x00000000u,  /* ....#wwwwwwww######wwwwwwww#.... */
        /* row 14 */ 0x00000000u,  /* ...#wwwwwwww########wwwwwwww#... */
        /* row 15 */ 0x00000000u,  /* ..#wwwwwwww##########wwwwwwww#.. */
        /* row 16 */ 0x00000000u,  /* .#wwwwwwww############wwwwwwww#. */
        /* row 17 */ 0x00000000u,  /* ..#wwwwwwww##########wwwwwwww#.. */
        /* row 18 */ 0x00000000u,  /* ...#wwwwwwww########wwwwwwww#... */
        /* row 19 */ 0x00000000u,  /* ....#wwwwwwww######wwwwwwww#.... */
        /* row 20 */ 0x00000000u,  /* .....#wwwwwwww####wwwwwwww#..... */
        /* row 21 */ 0x00000000u,  /* ......#wwwwwwww##wwwwwwww#...... */
        /* row 22 */ 0x00000000u,  /* .......#wwwwwwwwwwwwwwww#....... */
        /* row 23 */ 0x00000000u,  /* ........#wwwwwwwwwwwwww#........ */
        /* row 24 */ 0x00000000u,  /* .........#wwwwwwwwwwww#......... */
        /* row 25 */ 0x00000000u,  /* ..........#wwwwwwwwww#.......... */
        /* row 26 */ 0x00000000u,  /* ...........#wwwwwwww#........... */
        /* row 27 */ 0x00000000u,  /* ............#wwwwww#............ */
        /* row 28 */ 0x00000000u,  /* .............#wwww#............. */
        /* row 29 */ 0x00000000u,  /* ..............#ww#.............. */
        /* row 30 */ 0x00000000u,  /* ...............##............... */
        /* row 31 */ 0x00000000u  /* ................................ */
    }
};

#endif /* INITECH_FINDER_ICONS_H */
