/* Ref: locked spec/easter/jump_to_conclusions.json (authored mat geometry).
 * Half-open pads, row-major; gaps and outside points never select a pad. */
#include "egg_jtc_model.h"
unsigned egg_jtc_value(unsigned f)
{
    return egg_jtc_data[f * 2] | ((unsigned)egg_jtc_data[f * 2 + 1] << 8);
}
static const char *cell(unsigned i)
{
    /* Packed strings (egg_jtc_model.h): skip i terminated strings. */
    const char *p = (const char *)egg_jtc_data + JTC_WORDS * 2;
#ifdef EGG_MUT_CELL_WALK
    i++;   /* MUTANT (Rule 6; test-egg-jtc-cell-mutant): one string too far */
#endif
    while (i--) while (*p++) { }
    return p;
}
const char *egg_jtc_title(void) { return cell(0); }
const char *egg_jtc_heading(void) { return cell(1); }
const char *egg_jtc_label(int i) { return cell((unsigned)i + 2); }
int egg_jtc_count(void)
{
    int n = (int)(egg_jtc_value(JTC_ROWS) * egg_jtc_value(JTC_COLS));
#ifdef EGG_MUT_PAD_MISSING
    n--;
#endif
    return n;
}
rgn_rect_t egg_jtc_pad(rgn_rect_t c, int i)
{
    int cols = (int)egg_jtc_value(JTC_COLS);
    int w = (int)egg_jtc_value(JTC_PAD_W), h = (int)egg_jtc_value(JTC_PAD_H);
    int gap = (int)egg_jtc_value(JTC_GAP), m = (int)egg_jtc_value(JTC_MARGIN);
    rgn_rect_t p;
    p.left = (int16_t)(c.left + m + (i % cols) * (w + gap));
    p.top = (int16_t)(c.top + m + egg_jtc_value(JTC_HEADING_H) + (i / cols) * (h + gap));
    p.right = (int16_t)(p.left + w); p.bottom = (int16_t)(p.top + h);
    return p;
}
int egg_jtc_hit(rgn_rect_t c, int x, int y)
{
    for (int i = 0; i < egg_jtc_count(); i++) {
        rgn_rect_t p = egg_jtc_pad(c, i);
        if (x >= p.left && x < p.right && y >= p.top && y < p.bottom) return i;
    }
    return -1;
}
int egg_jtc_trigger(unsigned *n, const EventRecord *e)
{
    if (!(e->modifiers & FLAIR_EVT_MOD_OPTION_KEY)) { *n = 0; return 0; }
    if (e->what != mouseDown) return 0;
    /* Band-1 Apple slot [0,33), per os/flair/menu.h geometry notes. This hidden
     * authored gesture leaves About's enabled byte and all menu rows alone. */
    if (e->where.h < 0 || e->where.h >= 33 || e->where.v < 0 || e->where.v >= 20) {
        *n = 0; return 0;
    }
    unsigned limit = egg_jtc_value(JTC_CLICKS);
#ifdef EGG_MUT_TRIGGER_OFF_BY_ONE
    limit--;
#endif
    return ++*n == limit;
}
/* ADR-0014 DEC-14-10 lane 1: moved here from egg_jtc.c (now overlaid) so the
 * per-event poll stays resident beside the trigger. */
int egg_jtc_pending;
int egg_jtc_take_pad(void)
{
    int i = egg_jtc_pending - 1;
    egg_jtc_pending = 0;
    return i;
}
