/* Factory vocabulary, independent expected RFB keysyms and pointer steps. */
#include "bochs_trace.h"
#include <stdlib.h>
static void check(int ok) { if (!ok) { fputs("RED bochs trace parser\n", stderr); exit(1); } }
/* Independent Bochs REL_2_7_FINAL bochs/gui/rfb.cc:636-667 model:
 * oldx/oldy start at -1; only y > headerbar (32) updates them, and
 * relative PS/2 motion is (x-oldx, oldy-y). The guest inverts PS/2 y
 * and clips at the desktop edges (os/flair/event.c). No absolute warp.
 * Source: https://github.com/bochs-emu/Bochs/blob/REL_2_7_FINAL/bochs/gui/rfb.cc
 * The literal destinations below are DAS L002 / copybreak.mk's Apple-menu
 * opening from the locked boot cursor (320,240), not encoder outputs. */
static void check_opening(void)
{
    static const struct { int x, y, buttons; } want[] = {
        {220,140,0}, {120,40,0}, {20,10,0}, {20,10,1},
        {20,10,1}, {60,98,1}, {60,98,0}, {60,98,0}
    };
    BochsTracePointer pointer = { .x = 320, .y = 240 };
    int oldx = -1, oldy = -1, guest_x = 320, guest_y = 240;
    const char *s = COPYBREAK_OPEN;
    unsigned i = 0;
    while (s) {
        const char *end = strchr(s, ',');
        BochsTraceStep step;
        check(i < sizeof want / sizeof want[0]);
        check(bochs_trace_parse(s, end ? (size_t)(end-s) : strlen(s), &step));
        if (step.kind == BX_MOVE || step.kind == BX_BUTTON) {
            int consumed;
            unsigned packets = 0;
            do {
                unsigned x, y, buttons;
                int wrong_delta = 0;
                consumed = bochs_trace_pointer_next(&pointer, &step, &x, &y, &buttons);
                check(++packets <= 32);
                if (!consumed) {
                    check(y > 32 && y <= 511 && x <= 639);
                    check(abs((int)x - oldx) <= 100 && abs(oldy - (int)y) <= 100);
                }
                if (y > 32) {
                    int dx = (int)x - oldx, dy = oldy - (int)y;
                    wrong_delta = consumed && step.kind == BX_MOVE &&
                                  (dx != step.x || dy != -step.y);
                    guest_x += dx; guest_y -= dy;
                    if (guest_x < 0) guest_x = 0;
                    if (guest_x > 639) guest_x = 639;
                    if (guest_y < 0) guest_y = 0;
                    if (guest_y > 479) guest_y = 479;
                    oldx = (int)x; oldy = (int)y;
                }
                if (wrong_delta || buttons != (unsigned)(consumed ? want[i].buttons : 0) ||
                    (consumed && (guest_x != want[i].x || guest_y != want[i].y))) {
                    fprintf(stderr, "RED bochs pointer origin: opening step %u at %d,%d want %d,%d\n",
                            i, guest_x, guest_y, want[i].x, want[i].y);
                    exit(1);
                }
            } while (!consumed);
        }
        ++i;
        s = end ? end + 1 : NULL;
    }
    check(i == sizeof want / sizeof want[0]);
    puts("GREEN bochs pointer origin: Apple press 20,10; Scrapbook release 60,98");
}
int main(void)
{
    BochsTraceStep v;
    const char *bad[] = {"", "m101:0", "m1:2x", "l2", "w-1", "w30001", "kctrl-", "kunknown", "w2x", "w4294967296", "m 1:2"};
    for (unsigned i = 0; i < sizeof bad / sizeof bad[0]; ++i)
        check(!bochs_trace_parse(bad[i], strlen(bad[i]), &v));
    check(bochs_trace_parse("m-100:88", 8, &v) && v.kind == BX_MOVE && v.x == -100 && v.y == 88);
    check(bochs_trace_parse("l1", 2, &v) && v.kind == BX_BUTTON && v.x == 1);
    check(bochs_trace_parse("w2200", 5, &v) && v.kind == BX_WAIT && v.delay == 2200);
    check(bochs_trace_parse("kctrl-c", 7, &v) && v.kind == BX_KEY && v.ctrl && v.key == 'c');
    check(bochs_trace_parse("ret", 3, &v) && v.key == 0xff0d);
    check(bochs_trace_parse("k", 1, &v) && v.key == 'k');
    check_opening();
    puts("GREEN bochs trace parser");
    return 0;
}
