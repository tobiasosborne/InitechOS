/* Factory vocabulary, independent expected RFB keysyms and pointer steps. */
#include "bochs_trace.h"
#include <stdlib.h>
static void check(int ok) { if (!ok) { fputs("RED bochs trace parser\n", stderr); exit(1); } }
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
    puts("GREEN bochs trace parser");
    return 0;
}
