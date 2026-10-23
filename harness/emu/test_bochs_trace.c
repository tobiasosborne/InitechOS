/* Factory vocabulary, independent expected RFB keysyms and pointer steps. */
#include "bochs_trace.h"
#include <stdlib.h>
static void check(int ok) { if (!ok) { fputs("RED bochs trace parser\n", stderr); exit(1); } }
/* Independent Bochs REL_2_7_FINAL bochs/gui/rfb.cc:636-667 model:
 * oldx/oldy start at -1; only y > headerbar (32) updates them, and
 * relative PS/2 motion is (x-oldx, oldy-y). The guest inverts PS/2 y
 * and clips at the desktop edges (os/flair/event.c). No absolute warp.
 * Source: https://github.com/bochs-emu/Bochs/blob/REL_2_7_FINAL/bochs/gui/rfb.cc
 * Capture: the harness continues past the debugger prompt (rc "c"), and
 * bx_debug/dbg_main.cc bx_dbg_user_input_loop sets BXPN_MOUSE_ENABLED=0
 * there, so iodev/devices.cc mouse_motion drops every packet ("PS/2 mouse
 * disabled" at t=0 in the KBD debug log; no FLAIR-CURSOR on the serial,
 * initech-kxrk) until a header-bar press (y <= 32, button 1) lands on the
 * mouse button: gui.cc init places floppyA, floppyB, cdrom1, mouse at left
 * gravity, 32 px each from x=0, and its handler is toggle_mouse_enable.
 * PS/2: iodev/keyboard.cc mouse_motion "scale down the motion" halves any
 * |delta| > 1 (C division), clamps it to [-256,255] and accumulates it;
 * create_mouse_packet emits at most +-254 and carries the rest. Measured
 * before the replay knew this: 100-px RFB steps moved FLAIR 50 px.
 * The literal destinations below are DAS L002 / copybreak.mk's Apple-menu
 * opening from the locked boot cursor (320,240), not encoder outputs. */
static int ps2_scale(int d)
{
    if (d < -1 || d > 1) d /= 2;
    if (d > 255) d = 255;
    if (d < -256) d = -256;
    return d;
}
static int ps2_emit(int *delayed)
{
    int e = *delayed > 254 ? 254 : *delayed < -254 ? -254 : *delayed;
    *delayed -= e;
    return e;
}
static void check_opening(void)
{
    static const struct { int x, y, buttons; } want[] = {
        {220,140,0}, {120,40,0}, {20,10,0}, {20,10,1},
        {20,10,1}, {60,98,1}, {60,98,0}, {60,98,0}
    };
    BochsTracePointer pointer = { .x = 320, .y = 240 };
    int oldx = -1, oldy = -1, guest_x = 320, guest_y = 240, captured = 0;
    int delayed_x = 0, delayed_y = 0, seeded = 0;
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
                if (y <= 32) {            /* header bar: only the mouse button */
                    check(!consumed && x >= 96 && x < 128);
                    if (buttons == 1) captured = !captured;
                    continue;
                }
                {
                    int dx = ps2_scale((int)x - oldx), dy = ps2_scale(oldy - (int)y);
                    /* After the one seed packet, every calibration move is
                     * ordinary motion (<= 100 guest px a packet). */
                    if (!consumed && seeded) check(abs(dx) <= 100 && abs(dy) <= 100);
                    seeded = 1;
                    wrong_delta = consumed && step.kind == BX_MOVE &&
                                  (dx != step.x || dy != -step.y);
                    if (!captured) dx = dy = 0;   /* devices.cc drops it */
                    delayed_x += dx; delayed_y += dy;
                    guest_x += ps2_emit(&delayed_x); guest_y -= ps2_emit(&delayed_y);
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
/* Guest time against the host-paced replay (initech-kxrk Bochs leg).
 * Every step is paced in host wall milliseconds (hold_ms per pointer
 * packet, BOCHS_TRACE_KEY_MS per key edge, w<ms> waits). With
 * Bochs 2.7 "clock: sync=none" an idle guest's HLT skips straight to the
 * next timer event, so guest PIT time has no relation to host time:
 * measured on copybreak, the image's 1500-tick FLAIR budget elapsed
 * 0.13 s wall after FLAIR-LIVE-READY and FLAIR-LIVE-OK halted the guest
 * before the first RFB packet (serial: no FLAIR-CURSOR, no TENANT-*).
 * "realtime" drives the emulated timers from the host clock (Bochs 2.7
 * bochsrc "clock:" option), the same relation QEMU's PIT has to the
 * locked QEMU trace. Model: guest ms reached by host ms t under a mode. */
static long guest_ms_at(const char *sync, long host_ms)
{
    if (!strcmp(sync, "realtime")) return host_ms;
    return -1;                     /* unbounded: the budget is already spent */
}
static long replay_ms(const char *s)
{
    BochsTracePointer pointer = { .x = 320, .y = 240 };
    long ms = 0;
    while (s) {
        const char *end = strchr(s, ',');
        BochsTraceStep step;
        check(bochs_trace_parse(s, end ? (size_t)(end-s) : strlen(s), &step));
        if (step.kind == BX_KEY) ms += 2 * BOCHS_TRACE_KEY_MS;
        else if (step.kind == BX_WAIT) ms += step.delay;
        else {
            unsigned x, y, b;
            int consumed;
            do {
                consumed = bochs_trace_pointer_next(&pointer, &step, &x, &y, &b);
                ms += pointer.hold_ms;
            } while (!consumed);
        }
        s = end ? end + 1 : NULL;
    }
    return ms;
}
static void check_clock(void)
{
    /* Slack: the keys-after serial poll (100 ms loop) plus one RFB
     * round-trip per step is far below this; it guards a trace that has
     * grown to the edge of the budget, not scheduling noise. */
    const long slack_ms = 3000, budget_ms = COPYBREAK_TICK_BUDGET * 10L;
    const char *sync = bochs_trace_clock_sync(1);
    long host = replay_ms(COPYBREAK_SPEC), guest = guest_ms_at(sync, host + slack_ms);
    if (guest < 0 || guest > budget_ms) {
        fprintf(stderr, "RED bochs replay clock: sync=%s, %ld ms host-paced trace outlives the %ld ms guest budget\n",
                sync, host, budget_ms);
        exit(1);
    }
    check(!strcmp(bochs_trace_clock_sync(0), "none"));
    printf("GREEN bochs replay clock: sync=%s, copybreak trace %ld ms inside the %ld ms guest budget\n",
           sync, host, budget_ms);
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
    check_clock();
    puts("GREEN bochs trace parser");
    return 0;
}
