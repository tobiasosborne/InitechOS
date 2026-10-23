/* Factory RFB input vocabulary. RFC 6143 Sec 7.5.4/5. The m/l/w/k forms
 * match the locked QEMU trace syntax, permitting one trace on both emulators. */
#ifndef INITECH_BOCHS_TRACE_H
#define INITECH_BOCHS_TRACE_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
typedef struct {
    enum { BX_KEY, BX_MOVE, BX_BUTTON, BX_WAIT } kind;
    unsigned key;
    int x, y, ctrl, delay;
} BochsTraceStep;
typedef struct {
    int x, y;                     /* the guest cursor this replay intends */
    unsigned buttons;
    int phase;
    int hold_ms;                  /* host wait after the packet just made */
    long sx, sy;                  /* the RFB position last sent (server) */
} BochsTracePointer;
/* Host pacing of the replay, in wall milliseconds (bochs.c run loop). */
#define BOCHS_TRACE_STEP_MS 80   /* each pointer packet / button step */
#define BOCHS_TRACE_KEY_MS  30   /* each key press and each key release */
/* After a header-bar press: rfb.cc discards every event still queued when
 * it dispatches a header-bar click, and handle_events runs on the VGA
 * update timer (interval=200000 us in the Bochs log), so let two periods
 * pass before the next packet. */
#define BOCHS_TRACE_CAPTURE_MS 500
/* Header-bar mouse button (gui.cc init, left gravity, 32 px each from 0:
 * floppyA, floppyB, cdrom1, mouse -> [96,128)); y=16 is inside the bar. */
#define BOCHS_TRACE_CAPTURE_X 112u
#define BOCHS_TRACE_HEADER_Y   16u
/* rfb.cc's relative branch never bounds x/y (only y > the 32-px header
 * counts), so the server position is a free 16-bit axis: start mid-range
 * and keep every scripted excursion far from both ends. */
#define BOCHS_TRACE_SERVER_BASE 16384L
/* One guest-pixel step on the server axis: keyboard.cc mouse_motion halves
 * any |delta| > 1 ("scale down the motion"), so 2d arrives as d (d=+-1 too). */
static inline void bochs_trace_pointer_move(BochsTracePointer *p, int dx, int dy)
{
    p->sx += 2L * dx; p->sy += 2L * dy;
    p->x += dx; p->y += dy;
    if (p->x < 0) p->x = 0;
    if (p->x > 639) p->x = 639;
    if (p->y < 0) p->y = 0;
    if (p->y > 479) p->y = 479;
}
/* The bochsrc "clock: sync=" mode for a run (bochs.c write_bochsrc).
 * Without a replay: "none" (fastest; an idle guest's HLT skips straight to
 * the next timer event). A replay is paced in host milliseconds above, and
 * a GUI trace runs inside a guest tick budget, so the guest PIT must follow
 * the host clock as QEMU's does: "realtime" (Bochs 2.7 bochsrc "clock:").
 * Under "none" the copybreak image spent its 1500-tick budget 0.13 s wall
 * after FLAIR-LIVE-READY, before the first RFB packet (initech-kxrk). */
static inline const char *bochs_trace_clock_sync(int replay)
{
#ifdef BOCHS_TRACE_MUT_SYNC_NONE
    replay = 0;                   /* Restore the free-running replay clock. */
#endif
    return replay ? "realtime" : "none";
}
/* Capture first. The harness continues Bochs past its debugger prompt (rc
 * "c"), and bx_debug/dbg_main.cc bx_dbg_user_input_loop sets
 * BXPN_MOUSE_ENABLED=0 at that prompt, so iodev/devices.cc mouse_motion
 * drops every packet until the header-bar mouse button (handler
 * toggle_mouse_enable) is pressed once: phases 0-1 press and release it.
 * Measured before this: KBD debug log "PS/2 mouse disabled" at t=0, keys
 * delivered, no FLAIR-CURSOR at all (initech-kxrk).
 * Origin next. Bochs 2.7 rfb.cc:636-667 starts oldx/oldy at (-1,-1),
 * converts absolute packets to relative motion, and ignores y <= its
 * 32-pixel header; keyboard.cc then halves each delta (see
 * bochs_trace_pointer_move). The seed packet's jump is unknown to the
 * guest model, so assume the cursor at (0,0) and walk +100 guest px a
 * packet to the bottom-right: the guest's edge clipping brings the real
 * cursor and the model into agreement. Return to the locked trace's boot
 * cursor (320,240) before consuming any step. After the seed every move is
 * <= 100 guest px, buttons stay up, and the caller paces each packet by
 * hold_ms.
 * Ref: REL_2_7_FINAL bochs/gui/rfb.cc, gui.cc, bx_debug/dbg_main.cc,
 * iodev/devices.cc; PM round 2 / initech-kxrk.
 * Return one RFB packet; 1 means the scripted step was consumed. */
static inline int bochs_trace_pointer_next(BochsTracePointer *p,
                                          const BochsTraceStep *step,
                                          unsigned *x, unsigned *y,
                                          unsigned *buttons)
{
    int consumed = 0;
#ifdef BOCHS_TRACE_MUT_NO_CAPTURE
    if (p->phase < 2) p->phase = 2;   /* Restore the never-captured mouse. */
#endif
#ifdef BOCHS_TRACE_MUT_ORIGIN
    if (p->phase == 2) {              /* Restore the unsynchronised origin. */
        p->sx = p->sy = BOCHS_TRACE_SERVER_BASE; p->phase = 5;
    }
#endif
    p->hold_ms = BOCHS_TRACE_STEP_MS;
    if (p->phase < 2) {
        *x = BOCHS_TRACE_CAPTURE_X; *y = BOCHS_TRACE_HEADER_Y;
        *buttons = p->phase == 0;
        p->hold_ms = BOCHS_TRACE_CAPTURE_MS;
        p->phase++;
        return 0;
    }
    if (p->phase == 2) {
        p->x = 0; p->y = 0; p->phase = 3;
        p->sx = p->sy = BOCHS_TRACE_SERVER_BASE;
    } else if (p->phase == 3) {
        bochs_trace_pointer_move(p, 100, 100);
        if (p->x == 639 && p->y == 479) p->phase = 4;
    } else if (p->phase == 4) {
        bochs_trace_pointer_move(p, 320 - p->x < -100 ? -100 : 320 - p->x,
                                    240 - p->y < -100 ? -100 : 240 - p->y);
        if (p->x == 320 && p->y == 240) p->phase = 5;
    } else if (step->kind == BX_MOVE) {
        consumed = 1;
        bochs_trace_pointer_move(p, step->x, step->y);
    } else {
        consumed = 1;
        p->buttons = (unsigned)step->x;
    }
    if (p->sx < 0 || p->sx > 65535 || p->sy <= 32 || p->sy > 65535) {
        fprintf(stderr, "bochs_trace: RFB pointer left the 16-bit axis\n");
        abort();                  /* Rule 2: never send a wrapped position */
    }
    *x = (unsigned)p->sx; *y = (unsigned)p->sy;
    *buttons = p->buttons;
    return consumed;
}
static inline int bochs_trace_number(const char *s, const char **end,
                                     int low, int high, int *out)
{
    const char *digits = s + (*s == '-' || *s == '+');
    char *tail;
    if (*digits < '0' || *digits > '9') return 0;
    errno = 0;
    long value = strtol(s, &tail, 10);
    if (errno || value < low || value > high) return 0;
    *end = tail; *out = (int)value;
    return 1;
}
static inline int bochs_trace_parse(const char *s, size_t n, BochsTraceStep *v)
{
    char b[48];
    const char *end;
    if (!n || n >= sizeof b) return 0;
    memcpy(b, s, n); b[n] = 0;
    memset(v, 0, sizeof *v);
    if (b[0] == 'm' && n > 1) {
        if (!bochs_trace_number(b+1, &end, -100, 100, &v->x) || *end != ':') return 0;
        if (!bochs_trace_number(end+1, &end, -100, 100, &v->y) || *end) return 0;
        v->kind = BX_MOVE; return 1;
    }
    if (n == 2 && b[0] == 'l' && (b[1] == '0' || b[1] == '1')) {
        v->kind = BX_BUTTON; v->x = b[1] - '0'; return 1;
    }
    if (b[0] == 'w' && n > 1) {
        if (!bochs_trace_number(b+1, &end, 0, 30000, &v->delay) || *end) return 0;
        v->kind = BX_WAIT; return 1;
    }
    const char *key = b;
    if (n > 1 && *key == 'k') ++key;
    if (!strncmp(key, "ctrl-", 5)) { v->ctrl = 1; key += 5; }
    if (strlen(key) == 1 && ((*key >= 'a' && *key <= 'z') ||
                            (*key >= '0' && *key <= '9'))) v->key = (unsigned char)*key;
    else if (!strcmp(key, "ret")) {
#ifdef BOCHS_KEYS_MUT_RETURN
        v->key = 0xff08;
#else
        v->key = 0xff0d;
#endif
    }
    else if (!strcmp(key, "spc")) v->key = ' ';
    else if (!strcmp(key, "dot")) v->key = '.';
    return v->key != 0;
}
#endif
