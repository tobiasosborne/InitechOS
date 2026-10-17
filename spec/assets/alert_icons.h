/*
 * alert_icons.h -- InitechOS FLAIR alert icon strike (LOCKED spec-data).
 *
 * beads: initech-tdnl.87 (the Empty Trash confirmation alert drew no caution
 *        icon: iconItem was a reserved no-op and no strike existed).
 *
 * Ref: spec/assets/desk_icons.h (the FLAIRDeskIcon v2 type, the 4bpp FORMAT,
 *      the DESK_IC_* colour ordinals and the decode -- reused verbatim, so the
 *      Dialog Manager's iconItem draws through the same finder_icon blitter);
 *      ../system7-decomp/re/mint-results-008.md Sec 2 + Sec 4 (the caution
 *      alert captures and the cell);
 *      Inside Macintosh Vol I p. I-419 (the three standard alert icons: Stop
 *      0, Note 1, Caution 2 -- the ICON resource ID an iconItem names);
 *      os/flair/dialog.c (iconItem); CLAUDE.md Law 1, Rule 8, Rule 12.
 *
 * PROVENANCE: exactly as spec/assets/desk_icons.h -- TRANSCRIBED cell for
 * cell from the mint-008 capture named below; the map printed from the cell
 * by a one-shot factory transcriber and reproduced verbatim. Graded against
 * the capture by harness/proptest/test_finder_icon_art.c and, on the booted
 * image, by the Empty Trash alert probe.
 *
 * Only CAUTION is captured; Stop and Note have no local golden, so no strike
 * exists for them and an iconItem naming them draws nothing (dialog.c).
 *
 * Changing these bytes is a DELIBERATE, issue-tracked Rule 8 act.
 * Freestanding-safe: <stdint.h> + desk_icons.h only (Law 3).
 */

#ifndef INITECH_ALERT_ICONS_H
#define INITECH_ALERT_ICONS_H

#include <stdint.h>

#include "desk_icons.h"   /* FLAIRDeskIcon v2 + the decode */

/* The standard alert icon IDs are os/flair/dialog.h DIALOG_ICON_ID_*. */


/* =========================================================================
 * FLAIR_ALERT_ICON_CAUTION -- the CAUTION alert icon
 * =========================================================================
 *
 * PROVENANCE: TRANSCRIBED from mint-008 cell [156,188) x [100,132) of
 * s8_icons_alert_caution.png (== s8_icons_alert_caution_1item.png, and
 * byte-identical to the same area of the earlier sys8 s8_alert_modal.png);
 * opacity against the alert face #E7E7E7 (an opaque #E7E7E7 cell could not be told
 * from the face; the icon only ever sits on that face, so it renders alike). The
 * yellow triangle with the black "!", 572 opaque cells. An alert icon has no
 * selected state.
 *
 * Tones (nibble: map char = colour):
 *   0 .=clear 1 W=DESK_IC_WHITE 2 r=DESK_IC_GRAY_A5 3 o=DESK_IC_GRAY_77
 *   4 n=DESK_IC_GRAY_66 5 #=DESK_IC_BLACK 6 Y=DESK_IC_YELLOW
 *   7 T=DESK_IC_TAN
 *
 * Map (32x32):
 *   row  0: ...............##...............
 *   row  1: ..............####..............
 *   row  2: ..............#rr#..............
 *   row  3: .............##YW##.............
 *   row  4: .............#rYWr#.............
 *   row  5: ............##YWYT##............
 *   row  6: ............#rYWYYr#............
 *   row  7: ...........##YWYYYT##...........
 *   row  8: ...........#rYWYYYYr#...........
 *   row  9: ..........##YWYYYYYT##..........
 *   row 10: ..........#rYWrnnrYYr#..........
 *   row 11: .........##YWYn##nYYT##.........
 *   row 12: .........#rYWY####YYYr#.........
 *   row 13: ........##YWYY####YYYT##........
 *   row 14: ........#rYWYY####YYYYr#........
 *   row 15: .......##YWYYY####YYYYT##.......
 *   row 16: .......#rYWYYY####YYYYYr#.......
 *   row 17: ......##YWYYYY####YYYYYT##......
 *   row 18: ......#rYWYYYY####YYYYYYr#......
 *   row 19: .....##YWYYYYYn##nYYYYYYT##.....
 *   row 20: .....#rYWYYYYYr##rYYYYYYYr#.....
 *   row 21: ....##YWYYYYYYYnnYYYYYYYYT##....
 *   row 22: ....#rYWYYYYYYYYYYYYYYYYYYr#....
 *   row 23: ...##YWYYYYYYYYYYYYYYYYYYYT##...
 *   row 24: ...#rYWYYYYYYYr##rYYYYYYYYYr#...
 *   row 25: ..##YWYYYYYYYY####YYYYYYYYYT##..
 *   row 26: ..#rYWYYYYYYYY####YYYYYYYYYYr#..
 *   row 27: .##YWYYYYYYYYYr##rYYYYYYYYYYT##.
 *   row 28: .#rYWYYYYYYYYYYYYYYYYYYYYYYYYr#.
 *   row 29: ##TTTTTTTTTTTTTTTTTTTTTTTTTTTT##
 *   row 30: ##oooooooooooooooooooooooooooo##
 *   row 31: .##############################.
 */
static const FLAIRDeskIcon FLAIR_ALERT_ICON_CAUTION = {
    /* tone[16] */
    { DESK_IC_CLEAR, DESK_IC_WHITE, DESK_IC_GRAY_A5, DESK_IC_GRAY_77,
      DESK_IC_GRAY_66, DESK_IC_BLACK, DESK_IC_YELLOW, DESK_IC_TAN,
      DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR,
      DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR, DESK_IC_CLEAR },
    /* px[32][4] -- 4bpp, col 8w+k in bits (31-4k)..(28-4k) of word w */
    {
        /* row  0 */ { 0x00000000u, 0x00000005u, 0x50000000u, 0x00000000u },
        /* row  1 */ { 0x00000000u, 0x00000055u, 0x55000000u, 0x00000000u },
        /* row  2 */ { 0x00000000u, 0x00000052u, 0x25000000u, 0x00000000u },
        /* row  3 */ { 0x00000000u, 0x00000556u, 0x15500000u, 0x00000000u },
        /* row  4 */ { 0x00000000u, 0x00000526u, 0x12500000u, 0x00000000u },
        /* row  5 */ { 0x00000000u, 0x00005561u, 0x67550000u, 0x00000000u },
        /* row  6 */ { 0x00000000u, 0x00005261u, 0x66250000u, 0x00000000u },
        /* row  7 */ { 0x00000000u, 0x00055616u, 0x66755000u, 0x00000000u },
        /* row  8 */ { 0x00000000u, 0x00052616u, 0x66625000u, 0x00000000u },
        /* row  9 */ { 0x00000000u, 0x00556166u, 0x66675500u, 0x00000000u },
        /* row 10 */ { 0x00000000u, 0x00526124u, 0x42662500u, 0x00000000u },
        /* row 11 */ { 0x00000000u, 0x05561645u, 0x54667550u, 0x00000000u },
        /* row 12 */ { 0x00000000u, 0x05261655u, 0x55666250u, 0x00000000u },
        /* row 13 */ { 0x00000000u, 0x55616655u, 0x55666755u, 0x00000000u },
        /* row 14 */ { 0x00000000u, 0x52616655u, 0x55666625u, 0x00000000u },
        /* row 15 */ { 0x00000005u, 0x56166655u, 0x55666675u, 0x50000000u },
        /* row 16 */ { 0x00000005u, 0x26166655u, 0x55666662u, 0x50000000u },
        /* row 17 */ { 0x00000055u, 0x61666655u, 0x55666667u, 0x55000000u },
        /* row 18 */ { 0x00000052u, 0x61666655u, 0x55666666u, 0x25000000u },
        /* row 19 */ { 0x00000556u, 0x16666645u, 0x54666666u, 0x75500000u },
        /* row 20 */ { 0x00000526u, 0x16666625u, 0x52666666u, 0x62500000u },
        /* row 21 */ { 0x00005561u, 0x66666664u, 0x46666666u, 0x67550000u },
        /* row 22 */ { 0x00005261u, 0x66666666u, 0x66666666u, 0x66250000u },
        /* row 23 */ { 0x00055616u, 0x66666666u, 0x66666666u, 0x66755000u },
        /* row 24 */ { 0x00052616u, 0x66666625u, 0x52666666u, 0x66625000u },
        /* row 25 */ { 0x00556166u, 0x66666655u, 0x55666666u, 0x66675500u },
        /* row 26 */ { 0x00526166u, 0x66666655u, 0x55666666u, 0x66662500u },
        /* row 27 */ { 0x05561666u, 0x66666625u, 0x52666666u, 0x66667550u },
        /* row 28 */ { 0x05261666u, 0x66666666u, 0x66666666u, 0x66666250u },
        /* row 29 */ { 0x55777777u, 0x77777777u, 0x77777777u, 0x77777755u },
        /* row 30 */ { 0x55333333u, 0x33333333u, 0x33333333u, 0x33333355u },
        /* row 31 */ { 0x05555555u, 0x55555555u, 0x55555555u, 0x55555550u }
    }
};

#endif /* INITECH_ALERT_ICONS_H */
