/*
 * os/flair/rdoc.h -- the rDocProc (16) WDEF geometry, shared by the Window
 * Manager (structure + content regions, the close-box hit) and the drawer
 * (chrome.c flair_draw_rdoc_window). THE ARTIFACT; bead initech-tdnl.17.
 *
 * MEASURED (Law 1) off the real Mac OS 8.1 Calculator window, the one
 * rDocProc on the volume (../system7-decomp GAPS wind-1: Calculator/Puzzle
 * WIND procID 16): goldens/captures/s8_calc_idle.png (active) and
 * s8_calc_inactive.png (inactive), frame (267,94)..(372,261) inclusive,
 * mint-results-007. All values are FRAME-relative.
 *
 *   outline     a rounded rect; rows 0..4 (and the last five, mirrored) are
 *               inset {5,3,2,1,1} px at both ends.
 *   title band  rows 0..18 (19 rows, the last one the band's bottom line).
 *               ACTIVE: solid black, a 10x10 white close-box ring at (9,4),
 *               the title in white. INACTIVE: a 1-px black outline (the
 *               rounded top, the two sides, row 18), white inside, the title
 *               in black, no close box.
 *   body        content = frame + (1, 19) .. frame - (3, 3): a 1-px left
 *               side, a 3-px right side and bottom (the WDEF's shadow is part
 *               of its outline); the content's four corners give up the
 *               pixels below (the rounded inner corner the WDEF draws black).
 *   title pen   the title is centred in [left + 18, right): "Calculator"
 *               (67 px of Chicago) pens at +28, ink at +29 -- [inferred]
 *               rule from that ONE rDocProc capture, exact for it.
 *
 * ASCII-clean (Rule 12).
 */
#ifndef INITECH_OS_FLAIR_RDOC_H
#define INITECH_OS_FLAIR_RDOC_H

#include <stdint.h>

#define FLAIR_RDOC_TITLE_H      19   /* band rows 0..18                      */
#define FLAIR_RDOC_INSET_L       1   /* content left  = frame left + 1       */
#define FLAIR_RDOC_INSET_R       3   /* content right = frame right - 3      */
#define FLAIR_RDOC_INSET_B       3   /* content bottom = frame bottom - 3    */
#define FLAIR_RDOC_CORNER_ROWS   5
#define FLAIR_RDOC_CLOSE_L       9   /* the close-box ring, 10 x 10          */
#define FLAIR_RDOC_CLOSE_T       4
#define FLAIR_RDOC_CLOSE_W      10
#define FLAIR_RDOC_TITLE_LEFT   18   /* the title centres in [L+18, R)       */
#define FLAIR_RDOC_TITLE_CAP     5   /* the title's cap-top row              */

/* outer inset of rows 0..4 from the top AND (mirrored) from the bottom */
static const uint8_t flair_rdoc_corner[FLAIR_RDOC_CORNER_ROWS] = { 5, 3, 2, 1, 1 };
/* content pixels given up at the left / right end of content rows 0..1 and of
 * the last three content rows (the inner rounding the WDEF paints black) */
static const uint8_t flair_rdoc_notch_top_l[2] = { 2, 1 };
static const uint8_t flair_rdoc_notch_top_r[2] = { 2, 1 };
static const uint8_t flair_rdoc_notch_bot_l[3] = { 1, 1, 2 };
static const uint8_t flair_rdoc_notch_bot_r[3] = { 0, 1, 2 };

#endif /* INITECH_OS_FLAIR_RDOC_H */
