/* Ref: spec/easter/jump_to_conclusions.json; operator brief initech-2ter (a).
 * The factory packs twelve little-endian words, then the title, the heading
 * and the rows*cols labels as consecutive NUL-terminated strings, each at most
 * JTC_CELL-1 characters (size round 7, initech-5qsm: was fixed 40-byte cells,
 * 584 B; JTC_BYTES stays the packer's upper bound). No JSON parser ships in
 * the OS. */
#ifndef FLAIR_EGG_JTC_MODEL_H
#define FLAIR_EGG_JTC_MODEL_H
#include <stdint.h>
#include "region_algebra.h"
#include "event_model.h"
enum {
    JTC_ROWS, JTC_COLS, JTC_PAD_W, JTC_PAD_H, JTC_GAP, JTC_MARGIN,
    JTC_HEADING_H, JTC_X, JTC_Y, JTC_W, JTC_H, JTC_CLICKS,
    JTC_WORDS, JTC_CELL = 40, JTC_MAX = 12,
    JTC_BYTES = JTC_WORDS * 2 + (2 + JTC_MAX) * JTC_CELL
};
extern const uint8_t egg_jtc_data[];
unsigned egg_jtc_value(unsigned field);
const char *egg_jtc_title(void);
const char *egg_jtc_heading(void);
const char *egg_jtc_label(int pad);
int egg_jtc_count(void);
rgn_rect_t egg_jtc_pad(rgn_rect_t content, int pad);
int egg_jtc_hit(rgn_rect_t content, int x, int y);
/* Returns 1 only on the last click; consumes no ordinary Apple gesture.
 * Caller invokes on every event, including Option release, to reset. */
int egg_jtc_trigger(unsigned *clicks, const EventRecord *ev);
#endif
