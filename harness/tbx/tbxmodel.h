/*
 * harness/tbx/tbxmodel.h -- the host MODEL of the Initech Toolbox Gate that
 * the binding oracles run tenants against (factory C; beads initech-tdnl.22
 * R5.2, initech-tdnl.23 R5.3).
 *
 * WHAT IT IS. A deterministic stand-in for the kernel side of INT 81h
 * (os/flair/tbxgate.c): it receives each Toolbox request as DECODED values,
 * prints ONE canonical trace line for it, and answers the way the gate
 * answers (window tokens 1..TBX_TENANT_WINDOWS, a content origin, a local
 * point, the Scrap, the error codes of spec/toolbox_gate.h Sec 4). It draws
 * nothing: the oracles grade the ORDERED REQUEST SEQUENCE a program makes --
 * the cross-compiler contract of ADR-0007 DEC-10 (reconciled,
 * docs/design/GUI-remediation-ADR-reconciliation.md Part B) -- never pixels
 * (the emu gates grade pixels against the real Mac captures).
 *
 * WHO DRIVES IT. Two front ends decode requests into these calls:
 *   tbx32_rt.c / tbxhost32.c -- an i386 Linux process that loads a REAL
 *     InitechMZ tenant (.EXE) at a fixed base, applies its relocation table,
 *     runs it and catches every real `int 0x81` (SIGSEGV on the CD 81
 *     bytes), reading AX and the argument block exactly where the gate does
 *     (spec Sec 2) -- the C Calculator, the seed-built and the Turbo
 *     Initech-built Pascal programs all run the same way;
 *   tbxhost64.c -- the same requests made by the SAME Pascal source compiled
 *     by Free Pascal on the host, whose FLAIR.PAS externals it implements
 *     (the declared print harness of DEC-10, in C: Law 3).
 * Identical traces from the front ends = the binding held on every rail.
 *
 * Freestanding (no libc): the front end supplies tm_write. ASCII (Rule 12).
 * Deterministic (Rule 11): no time, no addresses in the trace.
 */
#ifndef INITECH_HARNESS_TBXMODEL_H
#define INITECH_HARNESS_TBXMODEL_H

#include <stdint.h>
#include "toolbox_gate.h"

/* The front end's output channel (fd 1). */
void tm_write(const char *s, int n);

/* Printing helpers the front ends share (decimal, hex, quoted strings). */
void tm_puts(const char *s);
void tm_putd(int32_t v);
void tm_putx(uint32_t v, int digits);
void tm_putq(const char *s);          /* "..." with \xNN for non-printables */

/* A decoded MenuBar resource (spec Sec 9). */
#define TM_MENUS  TBX_MBAR_MAX_MENUS
#define TM_ITEMS  32      /* storage per menu: the binding stub's item cap
                           * (seed/rt/flairstub.asm FLAIR_MAX_ITEMS), so a
                           * menu past TBX_MBAR_MAX_ITEMS reaches the check */
typedef struct tm_item {
    char    text[TBX_MBAR_STR_MAX];
    uint8_t mark, cmd, style, enabled, divider;
} tm_item_t;
typedef struct tm_menu {
    int32_t   id;
    char      title[TBX_MBAR_STR_MAX];
    int       n_items;
    tm_item_t item[TM_ITEMS];
} tm_menu_t;
typedef struct tm_mbar {
    int       has_apple;
    int       n_menus;
    tm_menu_t menu[TM_MENUS];
} tm_mbar_t;

void    tm_reset(void);
int     tm_exited(void);          /* EXIT was requested (no more events) */
int     tm_registered(void);

int32_t tm_register(const char *name);
int32_t tm_register_bad(const char *why);   /* a refused record (32-bit) */
int32_t tm_exit(int32_t rc);
int32_t tm_newwindow(int32_t l, int32_t t, int32_t r, int32_t b, uint32_t goaway);
int32_t tm_setwtitle(int32_t win, const char *s);
int32_t tm_contentorigin(int32_t win);
int32_t tm_globaltolocal(int32_t win, int32_t h, int32_t v);
int32_t tm_textdraw(int32_t win, int32_t x, int32_t y, const char *s,
                    uint32_t fg, uint32_t bg);
int32_t tm_drawcells(int32_t win, int32_t x, int32_t y, const char *s,
                     uint32_t fg, uint32_t bg);
int32_t tm_fillrect(int32_t win, int32_t l, int32_t t, int32_t r, int32_t b,
                    uint32_t c);
int32_t tm_setmbar(const tm_mbar_t *bar);
int32_t tm_drawmenubar(void);
int32_t tm_alert(const char *text, const char *def, const char *cancel,
                 const char *other);   /* cancel/other may be 0 */
int32_t tm_scrap_put(const uint8_t *buf, uint32_t len);
int32_t tm_scrap_get(uint8_t *buf, uint32_t cap);   /* buf 0: length only */
int32_t tm_badcode(uint32_t ax);     /* a code the model does not serve */

/* The event the front end is about to deliver (one EVT line). */
void tm_event(const int32_t ev[6]);
/* A string or pointer the front end could not accept (one BAD line). */
void tm_bad(const char *what);

/* The event script: "what message modifiers when h v" per line, decimal or
 * 0x-hex, '#' comments. Returns the number of events parsed (<= cap), or -1
 * on a malformed line. */
int tm_parse_events(const char *text, int len, int32_t (*ev)[6], int cap);

#endif /* INITECH_HARNESS_TBXMODEL_H */
