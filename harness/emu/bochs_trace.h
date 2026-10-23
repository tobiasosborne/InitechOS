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
