/*
 * finder_icons.h -- InitechOS FLAIR disk-window icon strikes v2 (LOCKED
 * spec-data).
 *
 * beads: initech-tdnl.53 (Platinum dimensional icon art; supersedes the v1
 *        outline/diamond strikes of initech-tdnl.10 -- FOLDER, DOCUMENT and
 *        APPLICATION, 32x32).
 *
 * Ref: spec/assets/desk_icons.h -- the SIBLING file this one EXTENDS: it owns
 *      the FLAIRDeskIcon v2 type, the 4bpp FORMAT, the DESK_IC_* colour
 *      ordinals, the measured selection function, the invariants and the
 *      ASCII legend; this file adds three more strikes in that exact format
 *      and NOTHING to the contract.
 *      ../system7-decomp/re/mint-results-008.md (the cells + the evidence);
 *      docs/design/GUI-remediation-R3-finder-design.md F1.1 (the icon-kind
 *      heuristic: "DIR_ATTR_DIRECTORY => folder"; ".EXE" => application;
 *      everything else a document).
 *      CLAUDE.md Law 1, Law 4, Rule 8, Rule 11, Rule 12; docs/worklog/WL-0140.
 *
 * PROVENANCE -- READ BEFORE ANY EDIT: exactly as spec/assets/desk_icons.h.
 * TRANSCRIBED cell for cell from the mint-008 Mac OS 8.1 captures named in
 * each block; the ASCII map was printed from the capture cell by a one-shot
 * factory transcriber and is reproduced verbatim, the words generated from
 * it by the same pass. Not drawn freehand; not read from any resource, ROM or
 * icon-set binary; not from the frame still. Graded against the captures by
 * harness/proptest/test_finder_icon_art.c, which never reads these maps.
 * Every strike here was captured over TWO backgrounds (a window's white and
 * the desktop picture) and the two recoveries agree on every opaque cell.
 *
 * Changing these bytes is a DELIBERATE, issue-tracked Rule 8 act.
 *
 * Freestanding-safe: <stdint.h> + desk_icons.h only, no libc (Law 3).
 */

#ifndef INITECH_FINDER_ICONS_H
#define INITECH_FINDER_ICONS_H

#include <stdint.h>

#include "desk_icons.h"   /* FLAIRDeskIcon v2, DESK_IC_*, the decode */


/* =========================================================================
 * FLAIR_FINDER_ICON_FOLDER -- the generic FOLDER
 * =========================================================================
 *
 * PROVENANCE: TRANSCRIBED from mint-008 cell [285,317) x [72,104) of
 * s8_icons_floppy_window.png (window, over white) -- identical on all 609
 * opaque cells to the copy at [460,492) x [174,206) of s8_icons_on_desktop.png
 * (over the desktop picture). Selected: s8_icons_folder_sel.png /
 * s8_icons_desk_folder_sel.png. The lavender tab-top folder seen in
 * perspective, its #3F3F3F shadow lower right.
 *
 * Tones (nibble: map char = colour):
 *   0 .=clear 1 W=DESK_IC_WHITE 2 o=DESK_IC_GRAY_77 3 n=DESK_IC_GRAY_66
 *   4 l=DESK_IC_GRAY_3F 5 #=DESK_IC_BLACK 6 L=DESK_IC_LAV_LIGHT
 *   7 V=DESK_IC_LAV 8 D=DESK_IC_LAV_DARK
 *
 * Map (32x32):
 *   row  0: ..##n...........................
 *   row  1: ..#LV#o.........................
 *   row  2: ##nVVLV#o.......................
 *   row  3: #WVnnVVLV#o.....................
 *   row  4: #LLWVnnVVLV#o..##o..............
 *   row  5: #LLLLWVnnVVLV#oVLV#o............
 *   row  6: #LLLLLLWVnnVVLVVVVLV#o..........
 *   row  7: #LLLLLLLLWVnnVVVVVVVL#..........
 *   row  8: #LLLLLLLLLLWVnnVVVVVVV#.........
 *   row  9: #LLLLLLLLLLLLWVnnVVVVVo.........
 *   row 10: #LLLLLLLLLLLLLLWVnnVVVV#o.......
 *   row 11: #LLLLLLLLLLLLLLLLWVnnVVL#.......
 *   row 12: #LLLLLLLLLLLLLLLLLLWVnVV#.......
 *   row 13: #LLLLLLLLLLLLLLLLLLLLW#V#.......
 *   row 14: #LLLLLLLLLLLLLLLLLLLLD#D#.......
 *   row 15: #LLLLLLLLLLLLLLLLLLLLD#D#.......
 *   row 16: #LLLLLLLLLLLLLLLLLLLLD#D#.......
 *   row 17: #LLLLLLLLLLLLLLLLLLLLD#D#.......
 *   row 18: #LLLLLLLLLLLLLLLLLLLLD#D#.......
 *   row 19: #LLLLLLLLLLLLLLLLLLLLD#D#.......
 *   row 20: #DVLLLLLLLLLLLLLLLLLLD#D#.......
 *   row 21: .#nDVLLLLLLLLLLLLLLLLD#D#.......
 *   row 22: ...#nDVLLLLLLLLLLLLLLD#D#.......
 *   row 23: .....#nDVLLLLLLLLLLLLD#D#.......
 *   row 24: .......#nDVLLLLLLLLLLD#D#o......
 *   row 25: .........#nDVLLLLLLLLD#D#ooo....
 *   row 26: ...........#nDVLLLLLLD#D#lnoo...
 *   row 27: .............#nDVLLLLD#D#llnoo..
 *   row 28: ...............#nDVLLD#D#llnno..
 *   row 29: .................#nDDD#D#llnn...
 *   row 30: ...................#nD#D#lno....
 *   row 31: .....................###ll......
 */
static const FLAIRDeskIcon FLAIR_FINDER_ICON_FOLDER = {
    /* tone[16] */
    { DESK_IC_CLEAR, DESK_IC_WHITE, DESK_IC_GRAY_77, DESK_IC_GRAY_66,
      DESK_IC_GRAY_3F, DESK_IC_BLACK, DESK_IC_LAV_LIGHT, DESK_IC_LAV,
      DESK_IC_LAV_DARK, DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR,
      DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR },
    /* px[32][4] -- 4bpp, col 8w+k in bits (31-4k)..(28-4k) of word w */
    {
        /* row  0 */ { 0x00553000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row  1 */ { 0x00567520u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row  2 */ { 0x55377675u, 0x20000000u, 0x00000000u, 0x00000000u },
        /* row  3 */ { 0x51733776u, 0x75200000u, 0x00000000u, 0x00000000u },
        /* row  4 */ { 0x56617337u, 0x76752005u, 0x52000000u, 0x00000000u },
        /* row  5 */ { 0x56666173u, 0x37767527u, 0x67520000u, 0x00000000u },
        /* row  6 */ { 0x56666661u, 0x73377677u, 0x77675200u, 0x00000000u },
        /* row  7 */ { 0x56666666u, 0x61733777u, 0x77776500u, 0x00000000u },
        /* row  8 */ { 0x56666666u, 0x66617337u, 0x77777750u, 0x00000000u },
        /* row  9 */ { 0x56666666u, 0x66666173u, 0x37777720u, 0x00000000u },
        /* row 10 */ { 0x56666666u, 0x66666661u, 0x73377775u, 0x20000000u },
        /* row 11 */ { 0x56666666u, 0x66666666u, 0x61733776u, 0x50000000u },
        /* row 12 */ { 0x56666666u, 0x66666666u, 0x66617377u, 0x50000000u },
        /* row 13 */ { 0x56666666u, 0x66666666u, 0x66666157u, 0x50000000u },
        /* row 14 */ { 0x56666666u, 0x66666666u, 0x66666858u, 0x50000000u },
        /* row 15 */ { 0x56666666u, 0x66666666u, 0x66666858u, 0x50000000u },
        /* row 16 */ { 0x56666666u, 0x66666666u, 0x66666858u, 0x50000000u },
        /* row 17 */ { 0x56666666u, 0x66666666u, 0x66666858u, 0x50000000u },
        /* row 18 */ { 0x56666666u, 0x66666666u, 0x66666858u, 0x50000000u },
        /* row 19 */ { 0x56666666u, 0x66666666u, 0x66666858u, 0x50000000u },
        /* row 20 */ { 0x58766666u, 0x66666666u, 0x66666858u, 0x50000000u },
        /* row 21 */ { 0x05387666u, 0x66666666u, 0x66666858u, 0x50000000u },
        /* row 22 */ { 0x00053876u, 0x66666666u, 0x66666858u, 0x50000000u },
        /* row 23 */ { 0x00000538u, 0x76666666u, 0x66666858u, 0x50000000u },
        /* row 24 */ { 0x00000005u, 0x38766666u, 0x66666858u, 0x52000000u },
        /* row 25 */ { 0x00000000u, 0x05387666u, 0x66666858u, 0x52220000u },
        /* row 26 */ { 0x00000000u, 0x00053876u, 0x66666858u, 0x54322000u },
        /* row 27 */ { 0x00000000u, 0x00000538u, 0x76666858u, 0x54432200u },
        /* row 28 */ { 0x00000000u, 0x00000005u, 0x38766858u, 0x54433200u },
        /* row 29 */ { 0x00000000u, 0x00000000u, 0x05388858u, 0x54433000u },
        /* row 30 */ { 0x00000000u, 0x00000000u, 0x00053858u, 0x54320000u },
        /* row 31 */ { 0x00000000u, 0x00000000u, 0x00000555u, 0x44000000u }
    }
};

/* =========================================================================
 * FLAIR_FINDER_ICON_DOC -- the generic DOCUMENT
 * =========================================================================
 *
 * PROVENANCE: TRANSCRIBED from mint-008 cell [157,189) x [72,104) of
 * s8_icons_floppy_window.png -- identical on all 797 opaque cells to
 * [460,492) x [382,414) of s8_icons_on_desktop.png. Selected:
 * s8_icons_doc_sel.png / s8_icons_desk_doc_sel.png. A dog-eared sheet with a
 * shadow; column 0 and column 31 are clear.
 *
 * Tones (nibble: map char = colour):
 *   0 .=clear 1 W=DESK_IC_WHITE 2 w=DESK_IC_GRAY_F3 3 v=DESK_IC_GRAY_E7
 *   4 t=DESK_IC_GRAY_CD 5 s=DESK_IC_GRAY_C0 6 r=DESK_IC_GRAY_A5
 *   7 n=DESK_IC_GRAY_66 8 #=DESK_IC_BLACK
 *
 * Map (32x32):
 *   row  0: .###################............
 *   row  1: .#wwwwwwwwwwwwwwwww##...........
 *   row  2: .#wwwwwwwwwwwwwwwww#s#..........
 *   row  3: .#wwwwwwwwwwwwwwwww#Ws#.........
 *   row  4: .#wwwwwwwwwwwwwwwww#WWs#........
 *   row  5: .#wwwwwwwwwwwwwwwww#wvvs#.......
 *   row  6: .#wwwwwwwwwwwwwwwww#######......
 *   row  7: .#wwwwwwwwwwwwwwwwwwrrrrr#......
 *   row  8: .#wwwwwwwwwwwwwwwwwwttttt#......
 *   row  9: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 10: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 11: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 12: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 13: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 14: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 15: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 16: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 17: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 18: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 19: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 20: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 21: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 22: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 23: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 24: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 25: .#wwwwwwwwwwwwwwwwwwwwwwt#......
 *   row 26: .#wwwwwwwwwwwwwwwwwwwwwwt#n.....
 *   row 27: .#wwwwwwwwwwwwwwwwwwwwwwt#nnn...
 *   row 28: .#wwwwwwwwwwwwwwwwwwwwwwt#nnnnn.
 *   row 29: .#wwwwwwwwwwwwwwwwwwwwwwt#nnnnn.
 *   row 30: .#wwwwwwwwwwwwwwwwwwwwwwt#nnn...
 *   row 31: .#########################n.....
 */
static const FLAIRDeskIcon FLAIR_FINDER_ICON_DOC = {
    /* tone[16] */
    { DESK_IC_CLEAR, DESK_IC_WHITE, DESK_IC_GRAY_F3, DESK_IC_GRAY_E7,
      DESK_IC_GRAY_CD, DESK_IC_GRAY_C0, DESK_IC_GRAY_A5, DESK_IC_GRAY_66,
      DESK_IC_BLACK, DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR,
      DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR },
    /* px[32][4] -- 4bpp, col 8w+k in bits (31-4k)..(28-4k) of word w */
    {
        /* row  0 */ { 0x08888888u, 0x88888888u, 0x88880000u, 0x00000000u },
        /* row  1 */ { 0x08222222u, 0x22222222u, 0x22288000u, 0x00000000u },
        /* row  2 */ { 0x08222222u, 0x22222222u, 0x22285800u, 0x00000000u },
        /* row  3 */ { 0x08222222u, 0x22222222u, 0x22281580u, 0x00000000u },
        /* row  4 */ { 0x08222222u, 0x22222222u, 0x22281158u, 0x00000000u },
        /* row  5 */ { 0x08222222u, 0x22222222u, 0x22282335u, 0x80000000u },
        /* row  6 */ { 0x08222222u, 0x22222222u, 0x22288888u, 0x88000000u },
        /* row  7 */ { 0x08222222u, 0x22222222u, 0x22226666u, 0x68000000u },
        /* row  8 */ { 0x08222222u, 0x22222222u, 0x22224444u, 0x48000000u },
        /* row  9 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 10 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 11 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 12 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 13 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 14 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 15 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 16 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 17 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 18 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 19 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 20 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 21 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 22 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 23 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 24 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 25 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48000000u },
        /* row 26 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48700000u },
        /* row 27 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48777000u },
        /* row 28 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48777770u },
        /* row 29 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48777770u },
        /* row 30 */ { 0x08222222u, 0x22222222u, 0x22222222u, 0x48777000u },
        /* row 31 */ { 0x08888888u, 0x88888888u, 0x88888888u, 0x88700000u }
    }
};

/* =========================================================================
 * FLAIR_FINDER_ICON_APP -- the generic APPLICATION
 * =========================================================================
 *
 * PROVENANCE: TRANSCRIBED from mint-008 cell [29,61) x [72,104) of
 * s8_icons_floppy_window.png -- identical on all 619 opaque cells to
 * [460,492) x [285,317) of s8_icons_on_desktop.png. Selected:
 * s8_icons_app_sel.png / s8_icons_desk_app_sel.png. The Mac OS 8 generic
 * application: a diamond sheet, a hand holding a pencil. The clear cell at
 * row 22 col 28 is a real mask hole (clear over both backgrounds).
 *
 * Tones (nibble: map char = colour):
 *   0 .=clear 1 W=DESK_IC_WHITE 2 w=DESK_IC_GRAY_F3 3 s=DESK_IC_GRAY_C0
 *   4 r=DESK_IC_GRAY_A5 5 q=DESK_IC_GRAY_96 6 o=DESK_IC_GRAY_77
 *   7 n=DESK_IC_GRAY_66 8 l=DESK_IC_GRAY_3F 9 #=DESK_IC_BLACK
 *   10 Y=DESK_IC_YELLOW 11 T=DESK_IC_TAN 12 F=DESK_IC_FLESH 13 R=DESK_IC_RED
 *   14 P=DESK_IC_PINK
 *
 * Map (32x32):
 *   row  0: ................................
 *   row  1: ...........................###..
 *   row  2: ...........#########......#PP#..
 *   row  3: ..........#W#WWWWW#r#....#Y#R#..
 *   row  4: .........#Www#WWW#rrw#..#YT##...
 *   row  5: ........#Wwwww#w#rrwww##YT##....
 *   row  6: .......#Wwwwwww#wrw####YT##.....
 *   row  7: ......#Wwwwwwwwwww#FT#YT##......
 *   row  8: .....#Wwwwwwwwwww#FT#YT###......
 *   row  9: ....#Wwwwwwwwwww#FT#YT##TT#.....
 *   row 10: ...#Wwwwwwwwwww#FT#YT##TTTF#....
 *   row 11: ..#Wwwwwwwwwww#FT#YT###TTFFF#...
 *   row 12: .#Wwwwwwwwwwww#T#YT##s#TFFFFF###
 *   row 13: #Wwwwwwwwwwwwww#Y#####FFFFFFT###
 *   row 14: .#wwwwwwwwwwww#Y#FFFFFFFFFFFT###
 *   row 15: ..#wwwwwwwwwww#T###TTTTFFFFFT###
 *   row 16: ...#wwwwwwwww###qqq###TTTTFFT###
 *   row 17: ....#wwwwwwww#qssssqqq####TTT###
 *   row 18: .....#wwwwwwwwwwwwsssssqqq######
 *   row 19: ......#wwwwwwwwwwwwwssssssqq####
 *   row 20: .......#wwwwwwwwwwwwwwwssssss#..
 *   row 21: ........#wwwwwwwwwwwwwwwwwwW#...
 *   row 22: .........#wwwwwwwwwwwwwwwww#....
 *   row 23: ..........#wwwwwwwwwwwwwww#ooooo
 *   row 24: ...........#wwwwwwwwwwwww#llnnno
 *   row 25: ............#wwwwwwwwwww#llnnoo.
 *   row 26: .............#wwwwwwwww#llnoo...
 *   row 27: ..............#wwwwwww#lnoo.....
 *   row 28: ...............#wwwww#noo.......
 *   row 29: ................#www#ooo........
 *   row 30: .................#w#oo..........
 *   row 31: ..................#o............
 */
static const FLAIRDeskIcon FLAIR_FINDER_ICON_APP = {
    /* tone[16] */
    { DESK_IC_CLEAR, DESK_IC_WHITE, DESK_IC_GRAY_F3, DESK_IC_GRAY_C0,
      DESK_IC_GRAY_A5, DESK_IC_GRAY_96, DESK_IC_GRAY_77, DESK_IC_GRAY_66,
      DESK_IC_GRAY_3F, DESK_IC_BLACK, DESK_IC_YELLOW, DESK_IC_TAN,
      DESK_IC_FLESH, DESK_IC_RED, DESK_IC_PINK, DESK_IC_CLEAR },
    /* px[32][4] -- 4bpp, col 8w+k in bits (31-4k)..(28-4k) of word w */
    {
        /* row  0 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row  1 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00099900u },
        /* row  2 */ { 0x00000000u, 0x00099999u, 0x99990000u, 0x009EE900u },
        /* row  3 */ { 0x00000000u, 0x00919111u, 0x11949000u, 0x09A9D900u },
        /* row  4 */ { 0x00000000u, 0x09122911u, 0x19442900u, 0x9AB99000u },
        /* row  5 */ { 0x00000000u, 0x91222292u, 0x94422299u, 0xAB990000u },
        /* row  6 */ { 0x00000009u, 0x12222229u, 0x2429999Au, 0xB9900000u },
        /* row  7 */ { 0x00000091u, 0x22222222u, 0x229CB9ABu, 0x99000000u },
        /* row  8 */ { 0x00000912u, 0x22222222u, 0x29CB9AB9u, 0x99000000u },
        /* row  9 */ { 0x00009122u, 0x22222222u, 0x9CB9AB99u, 0xBB900000u },
        /* row 10 */ { 0x00091222u, 0x22222229u, 0xCB9AB99Bu, 0xBBC90000u },
        /* row 11 */ { 0x00912222u, 0x2222229Cu, 0xB9AB999Bu, 0xBCCC9000u },
        /* row 12 */ { 0x09122222u, 0x2222229Bu, 0x9AB9939Bu, 0xCCCCC999u },
        /* row 13 */ { 0x91222222u, 0x22222229u, 0xA99999CCu, 0xCCCCB999u },
        /* row 14 */ { 0x09222222u, 0x2222229Au, 0x9CCCCCCCu, 0xCCCCB999u },
        /* row 15 */ { 0x00922222u, 0x2222229Bu, 0x999BBBBCu, 0xCCCCB999u },
        /* row 16 */ { 0x00092222u, 0x22222999u, 0x555999BBu, 0xBBCCB999u },
        /* row 17 */ { 0x00009222u, 0x22222953u, 0x33355599u, 0x99BBB999u },
        /* row 18 */ { 0x00000922u, 0x22222222u, 0x22333335u, 0x55999999u },
        /* row 19 */ { 0x00000092u, 0x22222222u, 0x22223333u, 0x33559999u },
        /* row 20 */ { 0x00000009u, 0x22222222u, 0x22222223u, 0x33333900u },
        /* row 21 */ { 0x00000000u, 0x92222222u, 0x22222222u, 0x22219000u },
        /* row 22 */ { 0x00000000u, 0x09222222u, 0x22222222u, 0x22290000u },
        /* row 23 */ { 0x00000000u, 0x00922222u, 0x22222222u, 0x22966666u },
        /* row 24 */ { 0x00000000u, 0x00092222u, 0x22222222u, 0x29887776u },
        /* row 25 */ { 0x00000000u, 0x00009222u, 0x22222222u, 0x98877660u },
        /* row 26 */ { 0x00000000u, 0x00000922u, 0x22222229u, 0x88766000u },
        /* row 27 */ { 0x00000000u, 0x00000092u, 0x22222298u, 0x76600000u },
        /* row 28 */ { 0x00000000u, 0x00000009u, 0x22222976u, 0x60000000u },
        /* row 29 */ { 0x00000000u, 0x00000000u, 0x92229666u, 0x00000000u },
        /* row 30 */ { 0x00000000u, 0x00000000u, 0x09296600u, 0x00000000u },
        /* row 31 */ { 0x00000000u, 0x00000000u, 0x00960000u, 0x00000000u }
    }
};

#endif /* INITECH_FINDER_ICONS_H */
