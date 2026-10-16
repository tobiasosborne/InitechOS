/*
 * spec/calc/calc_layout.h -- the Calculator desk accessory's LAYOUT (LOCKED
 * spec-data; bead initech-tdnl.17; CLAUDE.md Rule 8).
 *
 * MEASURED (Law 1) off the real Mac OS 8.1 Calculator,
 * ../system7-decomp/goldens/captures/s8_calc_idle.png (mint-results-007): the
 * rDocProc window frame (267,94)..(372,261) inclusive, its content origin
 * (268,113). Every number below is CONTENT-LOCAL (origin = content top-left)
 * unless it says FRAME. The oracle never reads this header to grade the
 * pixels: harness/proptest/test_calc.c and tools/ppm_flair_calc_check.c read
 * the CAPTURE (Law 2 / ADR-0010) -- this file only tells the artifact where
 * to draw and what a click hits.
 *
 *   keys      18x16 black-framed rects, white 16x14 face, a 2-px black shadow
 *             offset (+2,+2); pitch 22 both ways; columns at x 8/30/52/74,
 *             rows at y 32/54/76/98/120. '0' spans two columns (40 wide),
 *             the lower '=' two rows (38 tall).
 *   readout   black frame (7,7)..(92,26) inclusive, white interior; the text
 *             is RIGHT-ALIGNED with its advance ending at x 89 (s8_calc_idle
 *             '0' cell 351..356, every probe ends at screen x 357) and its
 *             cap top at y 14 (screen 127).
 *   labels    the per-key label origin (cell left, cap top) offset from the
 *             key frame's top-left -- measured per key, because the real
 *             Calculator does not center every glyph the same way (the '*'
 *             sits at +7 and the slash one row high, where the digits sit at +6).
 *   window    FRAME 106 x 168 (rDocProc: rounded, 19-row title band, 1-px
 *             left frame, 3-px right and bottom border); CONTENT = frame
 *             (+1, +19) .. (-3, -3).
 *
 * ASCII-clean (Rule 12).
 */
#ifndef INITECH_SPEC_CALC_LAYOUT_H
#define INITECH_SPEC_CALC_LAYOUT_H

/* The window (FRAME, global; the capture's own position). */
#define CALC_WIN_L        267
#define CALC_WIN_T         94
#define CALC_WIN_W        106
#define CALC_WIN_H        168
#define CALC_CONTENT_DX     1      /* content left  = frame left + 1        */
#define CALC_CONTENT_DY    19      /* content top   = frame top + 19        */
#define CALC_CONTENT_W    102
#define CALC_CONTENT_H    146

/* The readout (content-local). */
#define CALC_DISP_L         7      /* black frame, inclusive                */
#define CALC_DISP_T         7
#define CALC_DISP_R        92
#define CALC_DISP_B        26
#define CALC_DISP_TEXT_RIGHT 89    /* advance end of the right-aligned text */
#define CALC_DISP_TEXT_TOP   14    /* cap top (strike row 0)                */

/* A key: frame rect l,t (inclusive) and w,h; the label; its origin offset. */
#define CALC_KEY_W         18
#define CALC_KEY_H         16
#define CALC_KEY_SHADOW     2
#define CALC_KEY_COUNT     18

typedef struct calc_key_rect {
    signed char cmd;               /* calc_cmd_t (os/apps/calc/calc_core.h) */
    unsigned char l, t, w, h;      /* frame, content-local                  */
    char label;                    /* the glyph drawn on the key            */
    unsigned char ldx, ldy;        /* label cell origin - frame top-left    */
} calc_key_rect_t;

/* cmd numbers are calc_cmd_t: 0..9 digits, 10 '.', 11 '+', 12 '-', 13 '*',
 * 14 '/', 15 '=', 16 clear. Order: reading order, the four rows then '0' '.'. */
#define CALC_KEY_TABLE {                                   \
    { 16,  8, 32, 18, 16, 'C', 6, 5 },                     \
    { 15, 30, 32, 18, 16, '=', 6, 5 },                     \
    { 14, 52, 32, 18, 16, '/', 7, 4 },                     \
    { 13, 74, 32, 18, 16, '*', 7, 5 },                     \
    {  7,  8, 54, 18, 16, '7', 6, 5 },                     \
    {  8, 30, 54, 18, 16, '8', 6, 5 },                     \
    {  9, 52, 54, 18, 16, '9', 6, 5 },                     \
    { 12, 74, 54, 18, 16, '-', 6, 5 },                     \
    {  4,  8, 76, 18, 16, '4', 6, 5 },                     \
    {  5, 30, 76, 18, 16, '5', 6, 5 },                     \
    {  6, 52, 76, 18, 16, '6', 6, 5 },                     \
    { 11, 74, 76, 18, 16, '+', 6, 6 },                     \
    {  1,  8, 98, 18, 16, '1', 6, 5 },                     \
    {  2, 30, 98, 18, 16, '2', 6, 5 },                     \
    {  3, 52, 98, 18, 16, '3', 6, 5 },                     \
    { 15, 74, 98, 18, 38, '=', 6, 27 },                    \
    {  0,  8, 120, 40, 16, '0', 6, 5 },                    \
    { 10, 52, 120, 18, 16, '.', 6, 5 },                    \
}

#endif /* INITECH_SPEC_CALC_LAYOUT_H */
