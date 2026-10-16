/*
 * desk_icons.h -- InitechOS FLAIR desktop icon strikes v2 (LOCKED spec-data).
 *
 * beads: initech-tdnl.53 (Platinum dimensional icon art; supersedes the v1
 *        outline strikes of initech-tdnl.9), initech-tdnl.74 (the FULL Trash),
 *        initech-6k12 (the FULL-trash state's consumer).
 *
 * Ref: ../system7-decomp/re/mint-results-008.md (mint-008, 2026-10-17: the
 *      Mac OS 8.1 Finder icon kinds at 1:1 under Basilisk II -- the cells,
 *      the two-background opacity recovery, the 30 colours and the selection
 *      colour function this file transcribes);
 *      ../system7-decomp/goldens/captures/s8_icons_*.png (the captures; the
 *      host oracle test-finder-icon-art and the emulator gates decode them
 *      at run time and grade every strike cell-for-cell, zero tolerance);
 *      docs/design/GUI-remediation-R3-finder-design.md F1.2 (32x32 sprites);
 *      os/flair/flair_look.h FLAIR_PART_ICON_CLUT (the colour ordinals below
 *      are SEMANTIC icon-colour roles resolved at the ONE policy seam -- this
 *      file names NO palette index; ADR-0004-AMENDMENT-DEC-09 Sec 3.1 C-8);
 *      CLAUDE.md Law 1 (ground truth + honesty about where a strike came
 *      from), Law 4 (period fidelity), Rule 8 (locked spec-data), Rule 11
 *      (byte-stable), Rule 12 (ASCII-clean); docs/worklog/WL-0140.
 *
 * =========================================================================
 * PROVENANCE -- READ BEFORE ANY EDIT
 * =========================================================================
 *
 * TRANSCRIBED, cell for cell, from the mint-008 Mac OS 8.1 captures named
 * in each block (the WL-0126 arrow-cursor pattern). The ASCII map in each
 * block was PRINTED from that capture cell by a one-shot factory transcriber
 * (a scratch script, not committed and never run by a gate) and is
 * reproduced verbatim; the packed words were generated from the map by the
 * same pass. Nothing here is drawn freehand, nothing is read from a ROM, an
 * 'icl8' / 'ICN#' resource or an icon-set binary, and nothing comes from the
 * frame still. The strikes reproduce what the Mac OS 8.1 Finder put on the
 * screen for its generic icon kinds, because fidelity to that screen is the
 * product (Law 4) -- they are graded against the captures, not against this
 * file (Law 2): harness/proptest/test_finder_icon_art.c decodes the PNGs and
 * never includes this header's maps.
 *
 * Changing these bytes is a DELIBERATE, issue-tracked Rule 8 act.
 *
 * =========================================================================
 * FORMAT -- FLAIRDeskIcon v2: 32x32, 4 bits per pixel, a 16-entry tone table
 * =========================================================================
 *
 *   tone[16]   -- nibble value -> a DESK_IC_* icon colour (below).
 *                 tone[0] is DESK_IC_CLEAR: nibble 0 is TRANSPARENT (the
 *                 desktop shows through); unused trailing entries are CLEAR.
 *   px[32][4]  -- one row = four 32-bit words of eight 4-bit pixels; column
 *                 c lives in word c/8, bits (31 - 4*(c%8)) .. (28 - 4*(c%8))
 *                 (the leftmost pixel in the top nibble -- the cursors.h
 *                 MSB-first convention, widened to a nibble).
 *
 * Pixel (r,c):  n = (px[r][c >> 3] >> (28 - 4*(c & 7))) & 0xF;
 *               n == 0 -> clear (skip); else colour = tone[n].
 * Opacity / hit-test = (n != 0); there is no separate mask plane (v1 had
 * one): a nibble is never both clear and coloured.
 *
 * INVARIANTS (asserted by the oracle, not assumed):
 *      tone[0] == DESK_IC_CLEAR;  tone[n] != DESK_IC_CLEAR for every n a
 *      pixel uses;  every tone < DESK_IC__COUNT;  every used colour has an
 *      OBSERVED selected form in DESK_IC_SELECTED (mint-008 Sec 3).
 *
 * WHY 4bpp + A TONE TABLE: a Platinum icon is up to 14 colours (the generic
 * application: 14 + clear), so v1's three bit-planes (four tones) cannot
 * carry it; 4 bits per pixel with a per-strike table is the smallest exact
 * form (512 + 16 bytes a strike), and each 32-pixel row is still one line of
 * the ASCII map, so the transcription stays auditable by eye.
 *
 * =========================================================================
 * SELECTION (mint-008 Sec 3): ONE colour function for every icon
 * =========================================================================
 * The Finder draws a SELECTED icon (and a drop target, IM VI p. 2-19..2-20:
 * "The selection mechanism for color icons lowers the brightness of colors")
 * with every colour replaced by DESK_IC_SELECTED[colour]; the silhouette is
 * unchanged. The map below is the measured function -- each unselected colour
 * maps to exactly one selected colour across all seven captured icon kinds.
 * Rows marked (unobserved) are colours that occur in no strike as an INPUT
 * (they exist only as selected results); they map to themselves and the
 * oracle asserts no strike uses them.
 *
 * ASCII map legend (every block below):
 *   '.' clear            '#' BLACK #000000
 *   'W' WHITE #FFFFFF    'w' GRAY_F3   'v' GRAY_E7   'u' GRAY_DA   't' GRAY_CD
 *   's' GRAY_C0          'r' GRAY_A5   'q' GRAY_96   'p' GRAY_87   'o' GRAY_77
 *   'n' GRAY_66          'm' GRAY_54   'l' GRAY_3F   'k' GRAY_27
 *   'L' LAV_LIGHT #DADAFF 'V' LAV #B3B3FF 'D' LAV_DARK #8787DA
 *   'S' LAV_SEL #545487  'N' NAVY_SEL #000087
 *   'Y' YELLOW #FFFF00   'T' TAN #DAB387  'F' FLESH #FFDAB3  'R' RED #E70000
 *   'P' PINK #FF87DA     'y' YELLOW_SEL #878700  'j' TAN_SEL #875400
 *   'f' FLESH_SEL #875454 'e' RED_SEL #870000   'i' PINK_SEL #870054
 * (The hex values are the mint-008 measurements, quoted for the reader; the
 * value a role renders as is the canon row flair_look resolves it to.)
 *
 * All identifiers are prefixed FLAIR_ / DESK_ to stay in the InitechOS
 * namespace. Freestanding-safe: <stdint.h> only, no libc (Law 3).
 */

#ifndef INITECH_DESK_ICONS_H
#define INITECH_DESK_ICONS_H

#include <stdint.h>

/* Sprite edge, in pixels. 32x32 is the classic ICN# / icl8 size (F1.2). */
#define DESK_ICON_DIM    32
#define DESK_ICON_TONES  16   /* nibble values; 0 = clear                    */
#define DESK_ICON_WORDS   4   /* 32-bit words per 32-pixel row               */

/* The icon colour ordinals (mint-008 Sec 3). The VALUES are an internal
 * encoding; they are NOT colours and NOT palette indices (C-8). The blitter
 * turns ordinal k into the semantic role FLAIR_PART_ICON_CLUT + (k - 1) and
 * resolves it at the ONE policy seam (os/flair/finder_icon.c). */
typedef enum {
    DESK_IC_CLEAR = 0,  /* transparent: leave the destination alone          */
    DESK_IC_WHITE,      /* 'W' */
    DESK_IC_GRAY_F3,    /* 'w' */
    DESK_IC_GRAY_E7,    /* 'v' */
    DESK_IC_GRAY_DA,    /* 'u' */
    DESK_IC_GRAY_CD,    /* 't' */
    DESK_IC_GRAY_C0,    /* 's' */
    DESK_IC_GRAY_A5,    /* 'r' */
    DESK_IC_GRAY_96,    /* 'q' */
    DESK_IC_GRAY_87,    /* 'p' */
    DESK_IC_GRAY_77,    /* 'o' */
    DESK_IC_GRAY_66,    /* 'n' */
    DESK_IC_GRAY_54,    /* 'm' */
    DESK_IC_GRAY_3F,    /* 'l' */
    DESK_IC_GRAY_27,    /* 'k' */
    DESK_IC_BLACK,      /* '#' */
    DESK_IC_LAV_LIGHT,  /* 'L' folder face                                   */
    DESK_IC_LAV,        /* 'V' folder bevel                                  */
    DESK_IC_LAV_DARK,   /* 'D' folder edge                                   */
    DESK_IC_LAV_SEL,    /* 'S' selected folder face (unobserved as input)     */
    DESK_IC_NAVY_SEL,   /* 'N' selected folder edge (unobserved as input)     */
    DESK_IC_YELLOW,     /* 'Y' pencil / caution body                         */
    DESK_IC_TAN,        /* 'T' hand shade / caution rim                      */
    DESK_IC_FLESH,      /* 'F' hand                                          */
    DESK_IC_RED,        /* 'R' pencil ferrule                                */
    DESK_IC_PINK,       /* 'P' eraser                                        */
    DESK_IC_YELLOW_SEL, /* 'y' (unobserved as input)                         */
    DESK_IC_TAN_SEL,    /* 'j' (unobserved as input)                         */
    DESK_IC_FLESH_SEL,  /* 'f' (unobserved as input)                         */
    DESK_IC_RED_SEL,    /* 'e' (unobserved as input)                         */
    DESK_IC_PINK_SEL,   /* 'i' (unobserved as input)                         */
    DESK_IC__COUNT
} desk_ic_t;

/* A 32x32 4bpp desktop icon strike (see FORMAT above). */
typedef struct {
    uint8_t  tone[DESK_ICON_TONES];
    uint32_t px[DESK_ICON_DIM][DESK_ICON_WORDS];
} FLAIRDeskIcon;

/* The SELECTION colour function (mint-008 Sec 3 table, verbatim). */
static const uint8_t DESK_IC_SELECTED[DESK_IC__COUNT] = {
    /* CLEAR      */ DESK_IC_CLEAR,
    /* WHITE      */ DESK_IC_GRAY_96,
    /* GRAY_F3    */ DESK_IC_GRAY_96,
    /* GRAY_E7    */ DESK_IC_GRAY_87,
    /* GRAY_DA    */ DESK_IC_GRAY_87,
    /* GRAY_CD    */ DESK_IC_GRAY_77,
    /* GRAY_C0    */ DESK_IC_GRAY_77,
    /* GRAY_A5    */ DESK_IC_GRAY_66,
    /* GRAY_96    */ DESK_IC_GRAY_54,
    /* GRAY_87    */ DESK_IC_GRAY_87,
    /* GRAY_77    */ DESK_IC_GRAY_3F,
    /* GRAY_66    */ DESK_IC_GRAY_3F,
    /* GRAY_54    */ DESK_IC_GRAY_54,
    /* GRAY_3F    */ DESK_IC_GRAY_27,
    /* GRAY_27    */ DESK_IC_GRAY_27,      /* (unobserved) */
    /* BLACK      */ DESK_IC_BLACK,
    /* LAV_LIGHT  */ DESK_IC_LAV_SEL,
    /* LAV        */ DESK_IC_LAV_SEL,
    /* LAV_DARK   */ DESK_IC_NAVY_SEL,
    /* LAV_SEL    */ DESK_IC_LAV_SEL,      /* (unobserved) */
    /* NAVY_SEL   */ DESK_IC_NAVY_SEL,     /* (unobserved) */
    /* YELLOW     */ DESK_IC_YELLOW_SEL,
    /* TAN        */ DESK_IC_TAN_SEL,
    /* FLESH      */ DESK_IC_FLESH_SEL,
    /* RED        */ DESK_IC_RED_SEL,
    /* PINK       */ DESK_IC_PINK_SEL,
    /* YELLOW_SEL */ DESK_IC_YELLOW_SEL,   /* (unobserved) */
    /* TAN_SEL    */ DESK_IC_TAN_SEL,      /* (unobserved) */
    /* FLESH_SEL  */ DESK_IC_FLESH_SEL,    /* (unobserved) */
    /* RED_SEL    */ DESK_IC_RED_SEL,      /* (unobserved) */
    /* PINK_SEL   */ DESK_IC_PINK_SEL      /* (unobserved) */
};

/* 1 when colour `k` is ever an INPUT of the measured selection function
 * (a colour some captured unselected icon used). The oracle asserts every
 * strike uses only these. */
static inline int desk_ic_observed_input(int k)
{
    return k == (int)DESK_IC_WHITE || (k >= (int)DESK_IC_GRAY_F3 && k <= (int)DESK_IC_GRAY_3F) ||
           k == (int)DESK_IC_BLACK || k == (int)DESK_IC_LAV_LIGHT ||
           k == (int)DESK_IC_LAV || k == (int)DESK_IC_LAV_DARK ||
           (k >= (int)DESK_IC_YELLOW && k <= (int)DESK_IC_PINK);
}

/* Decode the colour ordinal of strike pixel (r,c); DESK_IC_CLEAR when clear
 * or out of the cell. Pure; shared by the blitter and the hit test. */
static inline int desk_icon_colour(const FLAIRDeskIcon *icon, int r, int c)
{
    uint32_t n;
    if (icon == (const FLAIRDeskIcon *)0 || r < 0 || r >= DESK_ICON_DIM ||
        c < 0 || c >= DESK_ICON_DIM) {
        return (int)DESK_IC_CLEAR;
    }
    n = (icon->px[r][c >> 3] >> (28 - 4 * (c & 7))) & 0xFu;
    return n == 0u ? (int)DESK_IC_CLEAR : (int)icon->tone[n];
}


/* =========================================================================
 * FLAIR_DESK_ICON_VOLUME -- the mounted volume: the generic 3.5-inch floppy
 * =========================================================================
 *
 * PROVENANCE: TRANSCRIBED from mint-008 (re/mint-results-008.md Sec 2) cell
 * [576,608) x [156,188) of s8_icons_boot.png (== s8_icons_desktop_empty.png);
 * opacity from those two plus s8_icons_floppy_sel.png against the 128x128
 * desktop picture. The InitechOS data volume is a 1.44 MB FAT12 floppy image,
 * and the 8.1 Finder drew a 1.44 MB volume with exactly this art. 934 opaque
 * cells; the clear cell at row 1 col 5 is a real mask hole (clear in every
 * capture). The two-column #3F3F3F drop shadow at cols 29..31 is part of the
 * icon. Selected form: s8_icons_floppy_sel.png (DESK_IC_SELECTED).
 *
 * Tones (nibble: map char = colour):
 *   0 .=clear 1 W=DESK_IC_WHITE 2 w=DESK_IC_GRAY_F3 3 v=DESK_IC_GRAY_E7
 *   4 u=DESK_IC_GRAY_DA 5 t=DESK_IC_GRAY_CD 6 s=DESK_IC_GRAY_C0
 *   7 r=DESK_IC_GRAY_A5 8 q=DESK_IC_GRAY_96 9 o=DESK_IC_GRAY_77
 *   10 l=DESK_IC_GRAY_3F 11 #=DESK_IC_BLACK
 *
 * Map (32x32):
 *   row  0: ................................
 *   row  1: .####.l#####################....
 *   row  2: #WWWWqvvvlWWWWWWWWWWWvlWWwwu#...
 *   row  3: #WvvvqsssrWvWvWvvvvvvvqWvutsr#..
 *   row  4: #WvuuqsssrWWvWWooooWvvqWuttsr#..
 *   row  5: #WvruqsssrWvWvWorrrWvvqWuttsr#..
 *   row  6: #WsrrqsssrWWvvWorrrWvvqWtttsr#..
 *   row  7: #WurtqsssrWvvvWorrrWvvqWtttsr#..
 *   row  8: #WurtqsssrWvvvWorrrWvvqWtttsr#..
 *   row  9: #WuttqsssrWvvvWorrrWvvqWtttsr#..
 *   row 10: #WuttqsssrWvvvWWWWWWvvqWtttsr#..
 *   row 11: #WuttqsssrWvvvvvvvvvvvqWtttsr#..
 *   row 12: #WuttqssssqqqqqqqqqqqqoWtttsr#..
 *   row 13: #WutttWWWWWWWWWWWWWWWWWWtttsr#..
 *   row 14: #Wuttttttttttttttttttttttttsr#..
 *   row 15: #Wuttttttttttttttttttttttttsr#..
 *   row 16: #Wutttqqqqqqqqqqqqqqqqqqqttsr#..
 *   row 17: #WuttqtttttttttttttttttttWtsr#..
 *   row 18: #WuttqtttttttttttttttttttWtsr#..
 *   row 19: #WuttqtttttttttttttttttttWtsr#..
 *   row 20: #WuttqtttttttttttttttttttWtsr#..
 *   row 21: #WuttqtttttttttttttttttttWtsr#..
 *   row 22: #WuttqtttttttttttttttttttWtsr#..
 *   row 23: #WuttqtttttttttttttttttttWtsr#..
 *   row 24: #WuttqtttttttttttttttttttWtsr#..
 *   row 25: #WuttqtttttttttttttttttttWtsr#..
 *   row 26: #WuttqtttttttttttttttttttWtsr#l.
 *   row 27: #WrrtqtttttttttttttttttttWtsr#ll
 *   row 28: #W##WqtttttttttttttttttttWtsr#ll
 *   row 29: #WwWWqtttttttttttttttttttWtsr#ll
 *   row 30: #rrrrqrrrrrrrrrrrrrrrrrrrurrr#ll
 *   row 31: .############################ll.
 */
static const FLAIRDeskIcon FLAIR_DESK_ICON_VOLUME = {
    /* tone[16] */
    { DESK_IC_CLEAR, DESK_IC_WHITE, DESK_IC_GRAY_F3, DESK_IC_GRAY_E7,
      DESK_IC_GRAY_DA, DESK_IC_GRAY_CD, DESK_IC_GRAY_C0, DESK_IC_GRAY_A5,
      DESK_IC_GRAY_96, DESK_IC_GRAY_77, DESK_IC_GRAY_3F, DESK_IC_BLACK,
      DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR },
    /* px[32][4] -- 4bpp, col 8w+k in bits (31-4k)..(28-4k) of word w */
    {
        /* row  0 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row  1 */ { 0x0BBBB0ABu, 0xBBBBBBBBu, 0xBBBBBBBBu, 0xBBBB0000u },
        /* row  2 */ { 0xB1111833u, 0x3A111111u, 0x111113A1u, 0x1224B000u },
        /* row  3 */ { 0xB1333866u, 0x67131313u, 0x33333381u, 0x34567B00u },
        /* row  4 */ { 0xB1344866u, 0x67113119u, 0x99913381u, 0x45567B00u },
        /* row  5 */ { 0xB1374866u, 0x67131319u, 0x77713381u, 0x45567B00u },
        /* row  6 */ { 0xB1677866u, 0x67113319u, 0x77713381u, 0x55567B00u },
        /* row  7 */ { 0xB1475866u, 0x67133319u, 0x77713381u, 0x55567B00u },
        /* row  8 */ { 0xB1475866u, 0x67133319u, 0x77713381u, 0x55567B00u },
        /* row  9 */ { 0xB1455866u, 0x67133319u, 0x77713381u, 0x55567B00u },
        /* row 10 */ { 0xB1455866u, 0x67133311u, 0x11113381u, 0x55567B00u },
        /* row 11 */ { 0xB1455866u, 0x67133333u, 0x33333381u, 0x55567B00u },
        /* row 12 */ { 0xB1455866u, 0x66888888u, 0x88888891u, 0x55567B00u },
        /* row 13 */ { 0xB1455511u, 0x11111111u, 0x11111111u, 0x55567B00u },
        /* row 14 */ { 0xB1455555u, 0x55555555u, 0x55555555u, 0x55567B00u },
        /* row 15 */ { 0xB1455555u, 0x55555555u, 0x55555555u, 0x55567B00u },
        /* row 16 */ { 0xB1455588u, 0x88888888u, 0x88888888u, 0x85567B00u },
        /* row 17 */ { 0xB1455855u, 0x55555555u, 0x55555555u, 0x51567B00u },
        /* row 18 */ { 0xB1455855u, 0x55555555u, 0x55555555u, 0x51567B00u },
        /* row 19 */ { 0xB1455855u, 0x55555555u, 0x55555555u, 0x51567B00u },
        /* row 20 */ { 0xB1455855u, 0x55555555u, 0x55555555u, 0x51567B00u },
        /* row 21 */ { 0xB1455855u, 0x55555555u, 0x55555555u, 0x51567B00u },
        /* row 22 */ { 0xB1455855u, 0x55555555u, 0x55555555u, 0x51567B00u },
        /* row 23 */ { 0xB1455855u, 0x55555555u, 0x55555555u, 0x51567B00u },
        /* row 24 */ { 0xB1455855u, 0x55555555u, 0x55555555u, 0x51567B00u },
        /* row 25 */ { 0xB1455855u, 0x55555555u, 0x55555555u, 0x51567B00u },
        /* row 26 */ { 0xB1455855u, 0x55555555u, 0x55555555u, 0x51567BA0u },
        /* row 27 */ { 0xB1775855u, 0x55555555u, 0x55555555u, 0x51567BAAu },
        /* row 28 */ { 0xB1BB1855u, 0x55555555u, 0x55555555u, 0x51567BAAu },
        /* row 29 */ { 0xB1211855u, 0x55555555u, 0x55555555u, 0x51567BAAu },
        /* row 30 */ { 0xB7777877u, 0x77777777u, 0x77777777u, 0x74777BAAu },
        /* row 31 */ { 0x0BBBBBBBu, 0xBBBBBBBBu, 0xBBBBBBBBu, 0xBBBBBAA0u }
    }
};

/* =========================================================================
 * FLAIR_DESK_ICON_HARDDISK -- a hard-disk volume
 * =========================================================================
 *
 * PROVENANCE: TRANSCRIBED from mint-008 cell [576,608) x [28,60) of
 * s8_icons_desktop_empty.png (selected: s8_icons_hd_sel.png). HONESTY: this
 * is the icon the Basilisk II disk driver hands the Finder (Control csCode
 * 21/22) for the emulated SCSI disk -- what the 8.1 Finder displayed for this
 * machine's hard disk -- NOT Apple's generic hard-disk art (mint-008 Sec 0,
 * Sec 5). No InitechOS volume is a hard disk yet (the one data volume is a
 * 1.44 MB floppy image), so no painter selects this strike today; the host
 * oracle grades it so it is ready, and an unreferenced static const costs the
 * kernel nothing.
 *
 * Tones (nibble: map char = colour):
 *   0 .=clear 1 W=DESK_IC_WHITE 2 #=DESK_IC_BLACK
 *
 * Map (32x32):
 *   row  0: ................................
 *   row  1: ................................
 *   row  2: ................................
 *   row  3: ................................
 *   row  4: ................................
 *   row  5: ................................
 *   row  6: ................................
 *   row  7: ................................
 *   row  8: ................................
 *   row  9: ................................
 *   row 10: ................................
 *   row 11: ................................
 *   row 12: ................................
 *   row 13: ................................
 *   row 14: ................................
 *   row 15: ................................
 *   row 16: ................................
 *   row 17: ................................
 *   row 18: ................................
 *   row 19: .##############################.
 *   row 20: #WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW#
 *   row 21: #WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW#
 *   row 22: #WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW#
 *   row 23: #WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW#
 *   row 24: #WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW#
 *   row 25: #WWW##WWWWWWWWWWWWWWWWWWWWWWWWW#
 *   row 26: #WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW#
 *   row 27: #WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW#
 *   row 28: .##############################.
 *   row 29: ................................
 *   row 30: ................................
 *   row 31: ................................
 */
static const FLAIRDeskIcon FLAIR_DESK_ICON_HARDDISK = {
    /* tone[16] */
    { DESK_IC_CLEAR, DESK_IC_WHITE, DESK_IC_BLACK, DESK_IC_CLEAR,
      DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR,
      DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR,
      DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR },
    /* px[32][4] -- 4bpp, col 8w+k in bits (31-4k)..(28-4k) of word w */
    {
        /* row  0 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row  1 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row  2 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row  3 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row  4 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row  5 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row  6 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row  7 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row  8 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row  9 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row 10 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row 11 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row 12 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row 13 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row 14 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row 15 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row 16 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row 17 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row 18 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row 19 */ { 0x02222222u, 0x22222222u, 0x22222222u, 0x22222220u },
        /* row 20 */ { 0x21111111u, 0x11111111u, 0x11111111u, 0x11111112u },
        /* row 21 */ { 0x21111111u, 0x11111111u, 0x11111111u, 0x11111112u },
        /* row 22 */ { 0x21111111u, 0x11111111u, 0x11111111u, 0x11111112u },
        /* row 23 */ { 0x21111111u, 0x11111111u, 0x11111111u, 0x11111112u },
        /* row 24 */ { 0x21111111u, 0x11111111u, 0x11111111u, 0x11111112u },
        /* row 25 */ { 0x21112211u, 0x11111111u, 0x11111111u, 0x11111112u },
        /* row 26 */ { 0x21111111u, 0x11111111u, 0x11111111u, 0x11111112u },
        /* row 27 */ { 0x21111111u, 0x11111111u, 0x11111111u, 0x11111112u },
        /* row 28 */ { 0x02222222u, 0x22222222u, 0x22222222u, 0x22222220u },
        /* row 29 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row 30 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
        /* row 31 */ { 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u }
    }
};

/* =========================================================================
 * FLAIR_DESK_ICON_TRASH -- the Trash, EMPTY
 * =========================================================================
 *
 * PROVENANCE: TRANSCRIBED from mint-008 cell [576,608) x [412,444) of
 * s8_icons_desktop_empty.png (selected: s8_icons_trash_empty_sel.png). The
 * lidded can, the lid handle at rows 0..2, the ridged body, its shadow to the
 * right. 644 opaque cells.
 *
 * Tones (nibble: map char = colour):
 *   0 .=clear 1 W=DESK_IC_WHITE 2 v=DESK_IC_GRAY_E7 3 t=DESK_IC_GRAY_CD
 *   4 s=DESK_IC_GRAY_C0 5 r=DESK_IC_GRAY_A5 6 q=DESK_IC_GRAY_96
 *   7 p=DESK_IC_GRAY_87 8 o=DESK_IC_GRAY_77 9 n=DESK_IC_GRAY_66
 *   10 m=DESK_IC_GRAY_54 11 l=DESK_IC_GRAY_3F 12 #=DESK_IC_BLACK
 *
 * Map (32x32):
 *   row  0: .........##.....................
 *   row  1: .......##rvn#...................
 *   row  2: .....nnnnlnrW##n................
 *   row  3: ...nnrvnrlllqv#r#nr.............
 *   row  4: .nntvvWnlntnrq#srrn#............
 *   row  5: nstvWWWWvttnq#lqsrron...........
 *   row  6: #tvWWWWWWWWn#nqqrsrq#...........
 *   row  7: #qvWWWWWWvvrrqqqrsro#...........
 *   row  8: #qrvWWWWWvvvttssrrnn#...........
 *   row  9: #rvtttvWWvvtttroooon#...........
 *   row 10: nrvvvvtttsrrqqooqqon#...........
 *   row 11: .lnvvWWvvtssrrqqqol#q...........
 *   row 12: .#qoptvvvtssrrqommn#............
 *   row 13: .#sttrrpnnnnmmmooon#............
 *   row 14: .#sssWvvvttsrrqqqqn#............
 *   row 15: .#sqsvWWqvvtsosssmn#............
 *   row 16: .#sqsvWqssvtoqqsqlo#............
 *   row 17: .#sqsvWqssvtoqqsqlo#ooo.........
 *   row 18: .#sqsvWqssvtoqqsqlo#nnooo.......
 *   row 19: .#sqsvWqssvtoqqsqlo#llnnoo......
 *   row 20: .#sqsvWqssvtoqqsqlo#llllnoo.....
 *   row 21: .#sqsvWqssvtoqqsqlo#lllllnoo....
 *   row 22: .#sqsvWqssvtoqqsqlo#lllllnno....
 *   row 23: .#sqsvWqssvtoqqsqlo#llllllnn....
 *   row 24: .#sqsvWqssvtoqqsqlo#llllllnn....
 *   row 25: .#sqsvWqssvtoqqsqlo#llllllnn....
 *   row 26: .#sssvWqssvtoqqsqoo#lllllnn.....
 *   row 27: .#ssvvWqssvtoqqsqqo#lllln.......
 *   row 28: .lqvvWWWqvvttossqon#lll.........
 *   row 29: ..l#sWWvvvvsssrrq##ll...........
 *   row 30: ....l#qssttssq#l#ll.............
 *   row 31: ......ll#####llll...............
 */
static const FLAIRDeskIcon FLAIR_DESK_ICON_TRASH = {
    /* tone[16] */
    { DESK_IC_CLEAR, DESK_IC_WHITE, DESK_IC_GRAY_E7, DESK_IC_GRAY_CD,
      DESK_IC_GRAY_C0, DESK_IC_GRAY_A5, DESK_IC_GRAY_96, DESK_IC_GRAY_87,
      DESK_IC_GRAY_77, DESK_IC_GRAY_66, DESK_IC_GRAY_54, DESK_IC_GRAY_3F,
      DESK_IC_BLACK, DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR },
    /* px[32][4] -- 4bpp, col 8w+k in bits (31-4k)..(28-4k) of word w */
    {
        /* row  0 */ { 0x00000000u, 0x0CC00000u, 0x00000000u, 0x00000000u },
        /* row  1 */ { 0x0000000Cu, 0xC529C000u, 0x00000000u, 0x00000000u },
        /* row  2 */ { 0x00000999u, 0x9B951CC9u, 0x00000000u, 0x00000000u },
        /* row  3 */ { 0x00099529u, 0x5BBB62C5u, 0xC9500000u, 0x00000000u },
        /* row  4 */ { 0x09932219u, 0xB93956C4u, 0x559C0000u, 0x00000000u },
        /* row  5 */ { 0x94321111u, 0x23396CB6u, 0x45589000u, 0x00000000u },
        /* row  6 */ { 0xC3211111u, 0x1119C966u, 0x5456C000u, 0x00000000u },
        /* row  7 */ { 0xC6211111u, 0x12255666u, 0x5458C000u, 0x00000000u },
        /* row  8 */ { 0xC6521111u, 0x12223344u, 0x5599C000u, 0x00000000u },
        /* row  9 */ { 0xC5233321u, 0x12233358u, 0x8889C000u, 0x00000000u },
        /* row 10 */ { 0x95222233u, 0x34556688u, 0x6689C000u, 0x00000000u },
        /* row 11 */ { 0x0B922112u, 0x23445566u, 0x68BC6000u, 0x00000000u },
        /* row 12 */ { 0x0C687322u, 0x23445568u, 0xAA9C0000u, 0x00000000u },
        /* row 13 */ { 0x0C433557u, 0x9999AAA8u, 0x889C0000u, 0x00000000u },
        /* row 14 */ { 0x0C444122u, 0x23345566u, 0x669C0000u, 0x00000000u },
        /* row 15 */ { 0x0C464211u, 0x62234844u, 0x4A9C0000u, 0x00000000u },
        /* row 16 */ { 0x0C464216u, 0x44238664u, 0x6B8C0000u, 0x00000000u },
        /* row 17 */ { 0x0C464216u, 0x44238664u, 0x6B8C8880u, 0x00000000u },
        /* row 18 */ { 0x0C464216u, 0x44238664u, 0x6B8C9988u, 0x80000000u },
        /* row 19 */ { 0x0C464216u, 0x44238664u, 0x6B8CBB99u, 0x88000000u },
        /* row 20 */ { 0x0C464216u, 0x44238664u, 0x6B8CBBBBu, 0x98800000u },
        /* row 21 */ { 0x0C464216u, 0x44238664u, 0x6B8CBBBBu, 0xB9880000u },
        /* row 22 */ { 0x0C464216u, 0x44238664u, 0x6B8CBBBBu, 0xB9980000u },
        /* row 23 */ { 0x0C464216u, 0x44238664u, 0x6B8CBBBBu, 0xBB990000u },
        /* row 24 */ { 0x0C464216u, 0x44238664u, 0x6B8CBBBBu, 0xBB990000u },
        /* row 25 */ { 0x0C464216u, 0x44238664u, 0x6B8CBBBBu, 0xBB990000u },
        /* row 26 */ { 0x0C444216u, 0x44238664u, 0x688CBBBBu, 0xB9900000u },
        /* row 27 */ { 0x0C442216u, 0x44238664u, 0x668CBBBBu, 0x90000000u },
        /* row 28 */ { 0x0B622111u, 0x62233844u, 0x689CBBB0u, 0x00000000u },
        /* row 29 */ { 0x00BC4112u, 0x22244455u, 0x6CCBB000u, 0x00000000u },
        /* row 30 */ { 0x0000BC64u, 0x433446CBu, 0xCBB00000u, 0x00000000u },
        /* row 31 */ { 0x000000BBu, 0xCCCCCBBBu, 0xB0000000u, 0x00000000u }
    }
};

/* =========================================================================
 * FLAIR_DESK_ICON_TRASH_FULL -- the Trash, FULL
 * =========================================================================
 *
 * PROVENANCE: TRANSCRIBED from mint-008 cell [576,608) x [412,444) of
 * s8_icons_trash_full.png (== s8_icons_boot.png, whose Trash held 3 items;
 * selected: s8_icons_trash_full_sel.png). Paper bulging under a lid pushed
 * ajar. Drawn while \TRASH holds anything (finder_desk_t.trash_full, bead
 * initech-6k12 / tdnl.74). 725 opaque cells.
 *
 * Tones (nibble: map char = colour):
 *   0 .=clear 1 W=DESK_IC_WHITE 2 v=DESK_IC_GRAY_E7 3 t=DESK_IC_GRAY_CD
 *   4 s=DESK_IC_GRAY_C0 5 r=DESK_IC_GRAY_A5 6 q=DESK_IC_GRAY_96
 *   7 p=DESK_IC_GRAY_87 8 o=DESK_IC_GRAY_77 9 n=DESK_IC_GRAY_66
 *   10 m=DESK_IC_GRAY_54 11 l=DESK_IC_GRAY_3F 12 #=DESK_IC_BLACK
 *
 * Map (32x32):
 *   row  0: ......l........l................
 *   row  1: .....lvl......lWl...............
 *   row  2: ....lvvvllllllWWWl..............
 *   row  3: ...#lllllllllWWWWWl.............
 *   row  4: .##vlWWvvvvlWWWlWtt#............
 *   row  5: ##tvlWWvvvlWWWlvlttln...........
 *   row  6: ##ttlWvvvlWWWlvvvlno#...........
 *   row  7: #q#tlvvvlWWvlvvttnor#...........
 *   row  8: #Wr###vlWvvlttqnnorr#...........
 *   row  9: #rWWro#######llrrrqo#...........
 *   row 10: #rvvWWttttttttrrqonn#...........
 *   row 11: .lnvvvvvvttsrrqol#####..........
 *   row 12: .#qoptvvvtssrrqlovvqts#.........
 *   row 13: .#sttrqoonnnmmmovvrvvt#r........
 *   row 14: .#sssWvvvttsrr#vvtvvWvt#........
 *   row 15: .#sqsvWWqvvts#tvvrvWWWv#........
 *   row 16: .#sqsvWqssvto#tWvrvWWWvr#.......
 *   row 17: .#sqsvWqssvto#vWtvWWWWWv#.......
 *   row 18: .#sqsvWqssvto#vvrvWWWWnnn#......
 *   row 19: .#sqsvWqssvto#vvrvWWWvltn#......
 *   row 20: .#sqsvWqssvto#ttrvvvWtn#lr#.....
 *   row 21: .#sqsvWqssvto#ssqvvvWtt#nv#oo...
 *   row 22: .#sqsvWqssvto#ssqtvrnnnlW#oooo..
 *   row 23: .#sqsvWqssvto#rrotvr#orqv#nnooo.
 *   row 24: .#sqsvWqssvto#rrotvqn#rv#lnnnnoo
 *   row 25: .#sqsvWqssvto#nqorvqnl###nnnnnoo
 *   row 26: .#sssvWqssvtoolooovqqnnn#nnnnnoo
 *   row 27: .#ssvvWqssvtoq#oootvrnnn#nnnnoo.
 *   row 28: .lqvvWWWqvvtto#noorvtrn#nnnnoo..
 *   row 29: ..l#sWWvvvvsssrloonrtrr#nnno....
 *   row 30: ....###ssttssq##nnnoqo#nnno.....
 *   row 31: .......l#####llll#####ooo.......
 */
static const FLAIRDeskIcon FLAIR_DESK_ICON_TRASH_FULL = {
    /* tone[16] */
    { DESK_IC_CLEAR, DESK_IC_WHITE, DESK_IC_GRAY_E7, DESK_IC_GRAY_CD,
      DESK_IC_GRAY_C0, DESK_IC_GRAY_A5, DESK_IC_GRAY_96, DESK_IC_GRAY_87,
      DESK_IC_GRAY_77, DESK_IC_GRAY_66, DESK_IC_GRAY_54, DESK_IC_GRAY_3F,
      DESK_IC_BLACK, DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR },
    /* px[32][4] -- 4bpp, col 8w+k in bits (31-4k)..(28-4k) of word w */
    {
        /* row  0 */ { 0x000000B0u, 0x0000000Bu, 0x00000000u, 0x00000000u },
        /* row  1 */ { 0x00000B2Bu, 0x000000B1u, 0xB0000000u, 0x00000000u },
        /* row  2 */ { 0x0000B222u, 0xBBBBBB11u, 0x1B000000u, 0x00000000u },
        /* row  3 */ { 0x000CBBBBu, 0xBBBBB111u, 0x11B00000u, 0x00000000u },
        /* row  4 */ { 0x0CC2B112u, 0x222B111Bu, 0x133C0000u, 0x00000000u },
        /* row  5 */ { 0xCC32B112u, 0x22B111B2u, 0xB33B9000u, 0x00000000u },
        /* row  6 */ { 0xCC33B122u, 0x2B111B22u, 0x2B98C000u, 0x00000000u },
        /* row  7 */ { 0xC6C3B222u, 0xB112B223u, 0x3985C000u, 0x00000000u },
        /* row  8 */ { 0xC15CCC2Bu, 0x122B3369u, 0x9855C000u, 0x00000000u },
        /* row  9 */ { 0xC51158CCu, 0xCCCCCBB5u, 0x5568C000u, 0x00000000u },
        /* row 10 */ { 0xC5221133u, 0x33333355u, 0x6899C000u, 0x00000000u },
        /* row 11 */ { 0x0B922222u, 0x23345568u, 0xBCCCCC00u, 0x00000000u },
        /* row 12 */ { 0x0C687322u, 0x2344556Bu, 0x822634C0u, 0x00000000u },
        /* row 13 */ { 0x0C433568u, 0x8999AAA8u, 0x225223C5u, 0x00000000u },
        /* row 14 */ { 0x0C444122u, 0x233455C2u, 0x2322123Cu, 0x00000000u },
        /* row 15 */ { 0x0C464211u, 0x62234C32u, 0x2521112Cu, 0x00000000u },
        /* row 16 */ { 0x0C464216u, 0x44238C31u, 0x25211125u, 0xC0000000u },
        /* row 17 */ { 0x0C464216u, 0x44238C21u, 0x32111112u, 0xC0000000u },
        /* row 18 */ { 0x0C464216u, 0x44238C22u, 0x52111199u, 0x9C000000u },
        /* row 19 */ { 0x0C464216u, 0x44238C22u, 0x521112B3u, 0x9C000000u },
        /* row 20 */ { 0x0C464216u, 0x44238C33u, 0x5222139Cu, 0xB5C00000u },
        /* row 21 */ { 0x0C464216u, 0x44238C44u, 0x6222133Cu, 0x92C88000u },
        /* row 22 */ { 0x0C464216u, 0x44238C44u, 0x6325999Bu, 0x1C888800u },
        /* row 23 */ { 0x0C464216u, 0x44238C55u, 0x8325C856u, 0x2C998880u },
        /* row 24 */ { 0x0C464216u, 0x44238C55u, 0x83269C52u, 0xCB999988u },
        /* row 25 */ { 0x0C464216u, 0x44238C96u, 0x85269BCCu, 0xC9999988u },
        /* row 26 */ { 0x0C444216u, 0x442388B8u, 0x88266999u, 0xC9999988u },
        /* row 27 */ { 0x0C442216u, 0x442386C8u, 0x88325999u, 0xC9999880u },
        /* row 28 */ { 0x0B622111u, 0x622338C9u, 0x8852359Cu, 0x99998800u },
        /* row 29 */ { 0x00BC4112u, 0x2224445Bu, 0x8895355Cu, 0x99980000u },
        /* row 30 */ { 0x0000CCC4u, 0x433446CCu, 0x999868C9u, 0x99800000u },
        /* row 31 */ { 0x0000000Bu, 0xCCCCCBBBu, 0xBCCCCC88u, 0x80000000u }
    }
};

#endif /* INITECH_DESK_ICONS_H */
