/*
 * os/apps/tbx/tbx.h -- the C binding to the Initech Toolbox Gate (INT 81h):
 * what a disk-launched application written in C includes (THE ARTIFACT).
 *
 * bead: initech-w96l (I123 P1b -- the general part: a C application can be a
 *       disk tenant; reused by every later C application, InitechWord next).
 * Ref:  spec/toolbox_gate.h -- THE locked ABI; every number below is its
 *         (the vector, the AX codes, the argument order and arity, the
 *         FlairTenantRec, the flat FlairEvent, the colour tokens, the MenuBar
 *         record layouts -- _Static_assert-pinned in tbx_rt.c);
 *       os/apps/tenantfx.asm (the hand-assembled fixture this generalizes:
 *         the same record, the same push-callback model);
 *       spec/event_model.h (the Event Manager codes; MTE Table 2-1);
 *       ADR-0003-AMENDMENT-DEC-08a (the InitechMZ flat-32 container; the
 *         Makefile's tenant-exe-rules links, relocates and wraps it);
 *       ADR-0013 Sec 3.4 (yield-is-return: the app NEVER loops waiting for
 *         events; the kernel calls tbx_app_event and the return is the yield).
 *
 * THE SHAPE OF AN APPLICATION (the C counterpart of tenantfx.asm):
 *
 *     const char tbx_app_name[] = "MYAPP";      at most 11 chars (spec Sec 5)
 *     int  tbx_app_main(void)    the entry: tbx_register(), SETMBAR, a window,
 *                                then RETURN (the app stays resident)
 *     void tbx_app_event(const tbx_event_t *ev)
 *                                every event, pushed by the kernel; RETURN to
 *                                yield. updateEvt: repaint. keyDown/mouseDown:
 *                                act and draw (the pump presents what a tenant
 *                                drew once it returns -- bead initech-8zii).
 *                                TBX_EVT_MENU: Message = (menuID<<16)|item.
 *
 * THE ONE RULE OF MEMORY. The kernel validates every pointer an app hands the
 * Toolbox against the LOADED MODULE (spec Sec 5/Sec 7: the record, the event
 * buffer, a string to draw, a MenuBar graph). Code, constants and initialised
 * data are in the module; zero-filled data (.bss, the InitechMZ e_minalloc
 * area) is NOT. So a buffer the app fills at run time and then passes to
 * tbx_draw_text / tbx_draw_cells must be declared TBX_IN_IMAGE; big private
 * state (a worksheet store) belongs in .bss, which costs no file bytes.
 *
 * Freestanding C (no libc; tbx_rt.c supplies memcpy/memset/memmove/memcmp,
 * which the compiler may call for struct copies). ASCII-clean (Rule 12).
 */
#ifndef INITECH_APPS_TBX_H
#define INITECH_APPS_TBX_H

#include <stdint.h>
#include "toolbox_gate.h"
#include "event_model.h"

/* A writable object the Toolbox may read: forced into the loaded module. */
#define TBX_IN_IMAGE __attribute__((section(".data")))

/* The flat FlairEvent (spec Sec 6): six int32 scalars at the locked offsets. */
typedef struct tbx_event {
    int32_t what;        /* FLAIR_EVBUF_WHAT_OFF   0 */
    int32_t message;     /* FLAIR_EVBUF_MSG_OFF    4 */
    int32_t modifiers;   /* FLAIR_EVBUF_MODS_OFF   8 */
    int32_t when;        /* FLAIR_EVBUF_WHEN_OFF  12 */
    int32_t whereh;      /* FLAIR_EVBUF_WHEREH_OFF 16 (GLOBAL coordinates) */
    int32_t wherev;      /* FLAIR_EVBUF_WHEREV_OFF 20 */
} tbx_event_t;

/* The MenuBar resource (spec Sec 9 = os/flair/menu.h Sec 3, i386 layout). */
typedef struct tbx_menuitem {
    const char *text;
    uint8_t     mark, cmd_char, style, enabled, is_divider;
    uint8_t     pad[3];
} tbx_menuitem_t;

typedef struct tbx_menuinfo {
    int16_t               menu_id;
    int16_t               pad;
    const char           *title;
    const tbx_menuitem_t *items;
    uint16_t              n_items;
    int16_t               menu_width;   /* a kernel cache: author 0 */
} tbx_menuinfo_t;

typedef struct tbx_menubar {
    const tbx_menuinfo_t *menus;
    uint16_t              n_menus;
    uint8_t               has_apple, pad;
} tbx_menubar_t;

/* (menuID << 16) | item -- the TBX_EVT_MENU Message (spec Sec 6a). */
#define TBX_MENU_WORD(id, item) ((int32_t)(((uint32_t)(id) << 16) | (uint32_t)(item)))

/* ---- the application provides --------------------------------------- */
extern const char tbx_app_name[];
int  tbx_app_main(void);
void tbx_app_event(const tbx_event_t *ev);

/* ---- the Toolbox (tbx_rt.c) -------------------------------------------
 * Each returns the gate's EAX: >= 0 success (NEWWINDOW: the window token),
 * negative TBX_ERR_* (spec Sec 4) -- a refused call never kills the app. */
int32_t tbx_register(void);
int32_t tbx_new_window(int32_t l, int32_t t, int32_t r, int32_t b, int32_t go_away);
int32_t tbx_set_wtitle(int32_t win, const char *title);
int32_t tbx_draw_text(int32_t win, int32_t x, int32_t y, const char *s,
                      uint32_t fg, uint32_t bg);
int32_t tbx_fill_rect(int32_t win, int32_t l, int32_t t, int32_t r, int32_t b,
                      uint32_t color);
int32_t tbx_draw_cells(int32_t win, int32_t x, int32_t y, const char *s,
                       uint32_t fg, uint32_t bg);
int32_t tbx_set_mbar(const tbx_menubar_t *bar);
int32_t tbx_draw_menubar(void);
int32_t tbx_exit(int32_t rc);

/* Files: spec Sec 10 (initech-tdnl.91). Buffers must be static module/BSS
 * storage; automatic buffers are on the shared kernel stack and are refused.
 * Paths resolve from the data root; errors are TBX_ERR_* / TBX_ERR_DOS(code). */
int32_t tbx_file_create(const char *path);
int32_t tbx_file_open(const char *path, uint32_t mode);
int32_t tbx_file_read(int32_t handle, void *buffer, uint32_t count);
int32_t tbx_file_write(int32_t handle, const void *buffer, uint32_t count);
int32_t tbx_file_seek(int32_t handle, int32_t delta, uint32_t origin);
int32_t tbx_file_close(int32_t handle);
int32_t tbx_file_delete(const char *path);

/* The raw trap: AX = code, `argc` dwords pushed cdecl (args[0] lowest). */
int32_t tbx_trap(uint32_t code, uint32_t argc, const uint32_t *args);

#endif /* INITECH_APPS_TBX_H */
