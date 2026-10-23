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
    int x, y;
    unsigned buttons;
    int phase;
} BochsTracePointer;
/* Bochs 2.7 rfb.cc:636-667 starts oldx/oldy at (-1,-1), converts absolute
 * packets to relative motion, and ignores y <= its 32-pixel header. Seed
 * inside the guest at (0,1), then walk to the bottom-right: clipping brings
 * guest and server coordinates into agreement. Return to the locked trace's
 * boot cursor (320,240) before consuming any step. Every delta is <=100,
 * buttons stay up, and the caller paces each packet like ordinary motion.
 * Ref: REL_2_7_FINAL bochs/gui/rfb.cc; PM round 2 / initech-kxrk.
 * Return one RFB packet; 1 means the scripted step was consumed. */
static inline int bochs_trace_pointer_next(BochsTracePointer *p,
                                          const BochsTraceStep *step,
                                          unsigned *x, unsigned *y,
                                          unsigned *buttons)
{
    int consumed = 0;
#ifdef BOCHS_TRACE_MUT_ORIGIN
    p->phase = 3;                 /* Restore the unsynchronised dual origin. */
#endif
    if (p->phase == 0) {
        p->x = 0; p->y = 1; p->phase = 1;
    } else if (p->phase == 1) {
        p->x += 100; p->y += 100;
        if (p->x > 639) p->x = 639;
        if (p->y > 479) p->y = 479;
        if (p->x == 639 && p->y == 479) p->phase = 2;
    } else if (p->phase == 2) {
        p->x -= 100; p->y -= 100;
        if (p->x < 320) p->x = 320;
        if (p->y < 240) p->y = 240;
        if (p->x == 320 && p->y == 240) p->phase = 3;
    } else if (step->kind == BX_MOVE) {
        consumed = 1;
        p->x += step->x; p->y += step->y;
        if (p->x < 0) p->x = 0;
        if (p->x > 639) p->x = 639;
        if (p->y < 0) p->y = 0;
        if (p->y > 479) p->y = 479;
    } else {
        consumed = 1;
        p->buttons = (unsigned)step->x;
    }
    *x = (unsigned)p->x; *y = (unsigned)p->y + 32u;
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
