/* Initech's three-block menu mark, operator-approved candidate A.
 * Authored from the boot logo; phase-2 marks.md records the design decision.
 * This is a 16 by 15 transparent bitmap at bar (15, 2), independent of the
 * 33-pixel menu hit slot. Each uint16_t row is MSB-left; only set bits draw.
 */
#ifndef FLAIR_INITECH_MARK_H
#define FLAIR_INITECH_MARK_H
#include <stdint.h>
#define INITECH_MARK_W 16
#define INITECH_MARK_H 15
static const uint16_t INITECH_MARK_ROWS[INITECH_MARK_H] = {
    /* row  0 */ 0x0000, /* ................ */
    /* row  1 */ 0x0180, /* .......##....... */
    /* row  2 */ 0x03C0, /* ......####...... */
    /* row  3 */ 0x0660, /* .....##..##..... */
    /* row  4 */ 0x0660, /* .....##..##..... */
    /* row  5 */ 0x07E0, /* .....######..... */
    /* row  6 */ 0x1BD8, /* ...##.####.##... */
    /* row  7 */ 0x3DBC, /* ..####.##.####.. */
    /* row  8 */ 0x6666, /* .##..##..##..##. */
    /* row  9 */ 0x6666, /* .##..##..##..##. */
    /* row 10 */ 0x7E7E, /* .######..######. */
    /* row 11 */ 0x3C3C, /* ..####....####.. */
    /* row 12 */ 0x1818, /* ...##......##... */
    /* row 13 */ 0x0000, /* ................ */
    /* row 14 */ 0x0000, /* ................ */
};

#endif
