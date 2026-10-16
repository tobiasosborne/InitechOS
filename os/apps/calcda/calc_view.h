/*
 * os/apps/calcda/calc_view.h -- the Calculator's FACE: the layout's paint and
 * hit-test (THE ARTIFACT; bead initech-tdnl.17). Pure and freestanding, like
 * calc_core: CALCULAT.EXE sends its rects through TBX_FILLRECT, the host oracle
 * rasterizes the SAME rects into a bitmap and grades them against the real
 * Mac OS 8.1 capture (never against spec/calcda/calcda_layout.h -- Law 2).
 *
 * Colours are the gate's tokens (spec/toolbox_gate.h Sec 7): 0 white,
 * 1 black, 2 gray. Rects are content-local, right/bottom EXCLUSIVE.
 *
 * MUTANT KNOBS (Rule 6; -D on this TU only):
 *   CALCDA_MUT_KEY_CLICK_DIVERGE -- a click on the '-' key commands '+', while
 *                                 the keyboard's '-' still commands '-': the
 *                                 click and key paths disagree (the bead's
 *                                 named mutant). The parity leg goes RED.
 *   CALCDA_MUT_READOUT_SHIFT     -- the readout is right-aligned one pixel
 *                                 further left: every readout crop goes RED.
 *
 * ASCII-clean (Rule 12).
 */
#ifndef INITECH_APPS_CALC_VIEW_H
#define INITECH_APPS_CALC_VIEW_H

#include "calc_core.h"

#define CALC_TOK_WHITE 0
#define CALC_TOK_BLACK 1
#define CALC_TOK_GRAY  2

typedef void (*calc_rect_fn)(void *user, int l, int t, int r, int b, int tok);

/* The whole content: gray field, readout, every key (none pressed). */
void calc_view_paint(const char *readout, calc_rect_fn fn, void *user);
/* The readout interior only (white + right-aligned text). */
void calc_view_paint_readout(const char *readout, calc_rect_fn fn, void *user);
/* One key, idle or pressed (pressed: the frame inverted, the label white). */
void calc_view_paint_key(int idx, int pressed, calc_rect_fn fn, void *user);

/* Content-local point -> key index (0..CALC_KEY_COUNT-1) or -1. A click is
 * on a key when it lands inside its black frame. */
int calc_view_hit(int x, int y);
/* The command a CLICK on key idx sends. */
calc_cmd_t calc_view_click_cmd(int idx);
/* The key the keyboard command k lights (for the pressed flash), or -1. */
int calc_view_key_of_cmd(calc_cmd_t k);
/* The key's frame rect (content-local, exclusive right/bottom). */
void calc_view_key_rect(int idx, int *l, int *t, int *r, int *b);

/* The readout's pen x (content-local) for a string: where the right-aligned
 * text starts. Exposed for the oracle's alignment check. */
int calc_view_text_left(const char *s);

#endif /* INITECH_APPS_CALC_VIEW_H */
