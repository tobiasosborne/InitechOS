/*
 * os/apps/tbx/tbx_rt.c -- the run-time half of the C Toolbox binding: the
 * entry, the link-synthesized FlairTenantRec, the event buffer, the INT 81h
 * trap and the four memory routines a freestanding C program needs
 * (THE ARTIFACT; bead initech-w96l).
 *
 * Ref: spec/toolbox_gate.h Sec 2 (cdecl args at the interrupted ESP), Sec 3
 *      (AX codes, arity), Sec 5 (the record: tag, namePtr, imageBase,
 *      imageLen, evBufPtr, eventProc -- every pointer a relocation site the
 *      InitechMZ table covers, imageLen a plain length), Sec 6 (the flat
 *      event), Sec 9 (the MenuBar layouts); os/apps/tenantfx.asm (the same
 *      record and the same entry/eventProc split, hand-assembled);
 *      os/apps/tbx/tenant.ld (where __image_start / __image_len come from).
 *
 * THE ENTRY is tbx_entry, placed at module byte 0 by tenant.ld's .text.entry
 * (ADR-0003 DEC-08a.2: entry == load base). It runs the app's main to its
 * RETURN; the kernel's trampoline (os/milton/tbx_gate.asm tbx_tenant_call)
 * saved every callee-saved register, so a plain C function is a valid entry.
 *
 * Freestanding (Law 3): no libc. ASCII-clean (Rule 12). Deterministic.
 */
#include "tbx.h"

/* ---- the locked layouts (spec/toolbox_gate.h Sec 6, Sec 9) ------------- */
_Static_assert(sizeof(tbx_event_t) == FLAIR_EVBUF_BYTES, "FlairEvent size");
_Static_assert(__builtin_offsetof(tbx_event_t, message) == FLAIR_EVBUF_MSG_OFF, "Message");
_Static_assert(__builtin_offsetof(tbx_event_t, whereh) == FLAIR_EVBUF_WHEREH_OFF, "WhereH");
_Static_assert(__builtin_offsetof(tbx_event_t, wherev) == FLAIR_EVBUF_WHEREV_OFF, "WhereV");
_Static_assert(sizeof(tbx_menubar_t) == TBX_MBAR_BYTES, "MenuBar size");
_Static_assert(__builtin_offsetof(tbx_menubar_t, n_menus) == TBX_MBAR_COUNT_OFF, "n_menus");
_Static_assert(__builtin_offsetof(tbx_menubar_t, has_apple) == TBX_MBAR_APPLE_OFF, "has_apple");
_Static_assert(sizeof(tbx_menuinfo_t) == TBX_MINFO_BYTES, "MenuInfo size");
_Static_assert(__builtin_offsetof(tbx_menuinfo_t, title) == TBX_MINFO_TITLE_OFF, "title");
_Static_assert(__builtin_offsetof(tbx_menuinfo_t, items) == TBX_MINFO_ITEMS_OFF, "items");
_Static_assert(__builtin_offsetof(tbx_menuinfo_t, n_items) == TBX_MINFO_COUNT_OFF, "n_items");
_Static_assert(sizeof(tbx_menuitem_t) == TBX_MITEM_BYTES, "MenuItem size");

/* ---- the record (spec Sec 5), synthesized at link time ------------------ */
typedef struct tbx_rec {
    uint32_t    tag;
    const char *name;
    const void *image_base;
    const void *image_len;    /* an ABSOLUTE linker symbol: its "address" IS
                               * the length, identical at every link base, so
                               * the relocation derivation never marks it */
    tbx_event_t *evbuf;
    void       (*proc)(void);
} tbx_rec_t;
_Static_assert(sizeof(tbx_rec_t) == FLAIR_TENANT_REC_BYTES, "FlairTenantRec size");
_Static_assert(__builtin_offsetof(tbx_rec_t, evbuf) == FLAIR_TENANT_REC_EVB_OFF, "evBufPtr");
_Static_assert(__builtin_offsetof(tbx_rec_t, proc) == FLAIR_TENANT_REC_PROC_OFF, "eventProc");

extern const char __image_start[];   /* tenant.ld */
extern const char __image_len[];     /* tenant.ld (absolute) */

static tbx_event_t g_evbuf TBX_IN_IMAGE = { 0, 0, 0, 0, 0, 0 };

static void tbx_event_proc(void)
{
    tbx_app_event(&g_evbuf);
}

static const tbx_rec_t g_rec __attribute__((aligned(4))) = {
    FLAIR_TENANT_REC_TAG, tbx_app_name, __image_start, __image_len,
    &g_evbuf, tbx_event_proc
};

__attribute__((section(".text.entry"), used))
void tbx_entry(void)
{
    (void)tbx_app_main();
}

/* ---- the trap ----------------------------------------------------------- *
 * int32_t tbx_trap(uint32_t code, uint32_t argc, const uint32_t *args):
 * push args[argc-1] .. args[0] (so arg 0 sits at the interrupted ESP, spec
 * Sec 2), AX = code, INT 81h, pop them, return EAX. The gate's own stub
 * (os/milton/tbx_gate.asm) restores every other register (pushad/popad). */
__asm__(
    ".text\n"
    ".globl tbx_trap\n"
    ".type tbx_trap, @function\n"
    "tbx_trap:\n"
    "    pushl %ebp\n"
    "    movl  %esp, %ebp\n"
    "    pushl %esi\n"
    "    movl  12(%ebp), %ecx\n"
    "    movl  16(%ebp), %esi\n"
    "1:  testl %ecx, %ecx\n"
    "    jz    2f\n"
    "    decl  %ecx\n"
    "    pushl (%esi,%ecx,4)\n"
    "    jmp   1b\n"
    "2:  movl  8(%ebp), %eax\n"
    "    int   $0x81\n"
    "    leal  -4(%ebp), %esp\n"
    "    popl  %esi\n"
    "    popl  %ebp\n"
    "    ret\n"
    ".size tbx_trap, .-tbx_trap\n");

int32_t tbx_register(void)
{
    uint32_t a[1] = { (uint32_t)(uintptr_t)&g_rec };
    return tbx_trap(TBX_REGISTER, TBX_ARGC_REGISTER, a);
}

int32_t tbx_new_window(int32_t l, int32_t t, int32_t r, int32_t b, int32_t go_away)
{
    uint32_t a[5] = { (uint32_t)l, (uint32_t)t, (uint32_t)r, (uint32_t)b, (uint32_t)go_away };
    return tbx_trap(TBX_NEWWINDOW, TBX_ARGC_NEWWINDOW, a);
}

int32_t tbx_set_wtitle(int32_t win, const char *title)
{
    uint32_t a[2] = { (uint32_t)win, (uint32_t)(uintptr_t)title };
    return tbx_trap(TBX_SETWTITLE, TBX_ARGC_SETWTITLE, a);
}

int32_t tbx_draw_text(int32_t win, int32_t x, int32_t y, const char *s,
                      uint32_t fg, uint32_t bg)
{
    uint32_t a[6] = { (uint32_t)win, (uint32_t)x, (uint32_t)y,
                      (uint32_t)(uintptr_t)s, fg, bg };
    return tbx_trap(TBX_TEXTDRAW, TBX_ARGC_TEXTDRAW, a);
}

int32_t tbx_fill_rect(int32_t win, int32_t l, int32_t t, int32_t r, int32_t b,
                      uint32_t color)
{
    uint32_t a[6] = { (uint32_t)win, (uint32_t)l, (uint32_t)t, (uint32_t)r,
                      (uint32_t)b, color };
    return tbx_trap(TBX_FILLRECT, TBX_ARGC_FILLRECT, a);
}

int32_t tbx_draw_cells(int32_t win, int32_t x, int32_t y, const char *s,
                       uint32_t fg, uint32_t bg)
{
    uint32_t a[6] = { (uint32_t)win, (uint32_t)x, (uint32_t)y,
                      (uint32_t)(uintptr_t)s, fg, bg };
    return tbx_trap(TBX_DRAWCELLS, TBX_ARGC_DRAWCELLS, a);
}

int32_t tbx_set_mbar(const tbx_menubar_t *bar)
{
    uint32_t a[1] = { (uint32_t)(uintptr_t)bar };
    return tbx_trap(TBX_SETMBAR, TBX_ARGC_SETMBAR, a);
}

int32_t tbx_draw_menubar(void)
{
    return tbx_trap(TBX_DRAWMENUBAR, TBX_ARGC_DRAWMENUBAR, (const uint32_t *)0);
}

int32_t tbx_exit(int32_t rc)
{
    uint32_t a[1] = { (uint32_t)rc };
    return tbx_trap(TBX_EXIT, TBX_ARGC_EXIT, a);
}

/* ---- the four routines gcc may call on its own (struct copies, zeroing).
 * Built with -fno-tree-loop-distribute-patterns so these loops are never
 * turned back into calls to themselves. ---------------------------------- */
void *memcpy(void *d, const void *s, uint32_t n)
{
    uint8_t *dp = (uint8_t *)d;
    const uint8_t *sp = (const uint8_t *)s;
    while (n--) *dp++ = *sp++;
    return d;
}

void *memmove(void *d, const void *s, uint32_t n)
{
    uint8_t *dp = (uint8_t *)d;
    const uint8_t *sp = (const uint8_t *)s;
    if (dp < sp) { while (n--) *dp++ = *sp++; }
    else { dp += n; sp += n; while (n--) *--dp = *--sp; }
    return d;
}

void *memset(void *d, int c, uint32_t n)
{
    uint8_t *dp = (uint8_t *)d;
    while (n--) *dp++ = (uint8_t)c;
    return d;
}

int memcmp(const void *a, const void *b, uint32_t n)
{
    const uint8_t *x = (const uint8_t *)a, *y = (const uint8_t *)b;
    for (uint32_t i = 0; i < n; i++)
        if (x[i] != y[i]) return (int)x[i] - (int)y[i];
    return 0;
}
