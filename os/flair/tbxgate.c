/*
 * os/flair/tbxgate.c -- the Initech Toolbox Gate (INT 81h) + the one-slot
 * disk-tenant host (THE ARTIFACT; bead initech-tdnl.14, GUI-remediation R3.7).
 *
 * See tbxgate.h for the contract and the serial vocabulary. Implemented from
 * docs/design/GUI-remediation-ADR-reconciliation.md Part B (DEC-AC3-1..4) and
 * Part C (deltas 1-7) -- NOT from the struck D1.3/D1.4 prose:
 *   - event delivery is PUSH-CALLBACK through a KERNEL-OWNED vtable
 *     (disk_tenant_procs: open=NULL, event=disk_tenant_event, idle=NULL,
 *     close=disk_tenant_close); returning from the tenant's eventProc IS the
 *     cooperative yield (ADR-0013 Sec 3.4). No park/resume, no stack swap, no
 *     KEEP_RESIDENT trap, no shipping watchdog (BC-4);
 *   - the FlairTenantRec is LINK-SYNTHESIZED data the kernel only validates;
 *   - registration is windowless (FlairProcess_admit); the first NEWWINDOW asks
 *     for affirmation, which the pump performs through FlairProcess_activate
 *     once the tenant has RETURNED (tenant code is never re-entered from inside
 *     its own trap);
 *   - teardown happens between pump iterations (tbx_reap), never mid-handler;
 *     the code block is carved FIRST and freed LAST (code->handle->records->
 *     data, strict reverse on the way out -- DEC-AC3-1 / Part C item 6);
 *   - a fault inside the tenant image is triaged to FlairProcess_kill; every
 *     other fault still halts fail-loud (Part C item 7).
 *
 * SETMBAR (bead initech-cnpm; spec/toolbox_gate.h Sec 9; D2.1 row 12): the
 * tenant's MenuBar graph lives in its relocated image; v_setmbar validates the
 * WHOLE graph against [imageBase, imageBase+imageLen) and installs the pointer
 * as FlairApp.menubar. Band 2 itself is never drawn here: the pump's ONE swap
 * path (kmain flair_live_finish_tenant_switch -> flair_live_tenant_bar) shows
 * it at affirmation, and teardown's head change restores the survivor's bar.
 *
 * Mutation knobs (Rule 6; each defined ONLY by its mutant image, NEVER in a
 * real build):
 *   TBX_MUT_NO_SLOT_GUARD  the one-slot check always says "free" (DOUBLE_LAUNCH)
 *   TBX_MUT_EXIT_LEAK      the code block is never freed on exit (EXIT_LEAK)
 *   TBX_MUT_MBAR_NOT_SWAPPED  SETMBAR validates + returns 0 but never installs
 *                          the bar (MBAR_NOT_SWAPPED: band 2 keeps the fallback)
 *   TBX_MUT_MBAR_NO_BOUNDS SETMBAR's image-range checks compiled out
 *                          (MBAR_NO_BOUNDS: an out-of-image bar is accepted)
 *
 * Freestanding (Law 3): no libc. ASCII-clean (Rule 12). Deterministic (Rule 11):
 * every serial number is a pure function of the boot's allocation order.
 */
#include <stdint.h>

#include "tbxgate.h"
#include "process.h"
#include "window.h"
#include "heap.h"
#include "blitter.h"
#include "flair_look.h"
#include "region_algebra.h"
#include "window_record.h"
#include "event_model.h"
#include "chrome_metrics.h"
#include "event.h"           /* FLAIR_SCREEN_W / FLAIR_SCREEN_H               */
#include "toolbox_gate.h"
#include "text.h"            /* chicago_advance / chicago_cell_bits (Chicago 12;
                              * the tables are text.o's ONE copy)           */
#include "loader.h"          /* loader_load_tenant (-Ios/milton)               */
#include "menu.h"            /* MenuBar/MenuInfo/MenuItem + the Sec 5 layout   */

/* spec/toolbox_gate.h Sec 9 IS menu.h Sec 3's i386 layout: pin every offset
 * the validator reads, so the locked ABI and the Menu Manager never drift. */
_Static_assert(sizeof(MenuBar) == TBX_MBAR_BYTES, "MenuBar size (spec Sec 9)");
_Static_assert(offsetof(MenuBar, menus) == TBX_MBAR_MENUS_OFF, "MenuBar.menus");
_Static_assert(offsetof(MenuBar, n_menus) == TBX_MBAR_COUNT_OFF, "MenuBar.n_menus");
_Static_assert(offsetof(MenuBar, has_apple) == TBX_MBAR_APPLE_OFF, "MenuBar.has_apple");
_Static_assert(sizeof(MenuInfo) == TBX_MINFO_BYTES, "MenuInfo size (spec Sec 9)");
_Static_assert(offsetof(MenuInfo, menuID) == TBX_MINFO_ID_OFF, "MenuInfo.menuID");
_Static_assert(offsetof(MenuInfo, title) == TBX_MINFO_TITLE_OFF, "MenuInfo.title");
_Static_assert(offsetof(MenuInfo, items) == TBX_MINFO_ITEMS_OFF, "MenuInfo.items");
_Static_assert(offsetof(MenuInfo, n_items) == TBX_MINFO_COUNT_OFF, "MenuInfo.n_items");
_Static_assert(sizeof(MenuItem) == TBX_MITEM_BYTES, "MenuItem size (spec Sec 9)");
_Static_assert(offsetof(MenuItem, text) == TBX_MITEM_TEXT_OFF, "MenuItem.text");

/* The two asm trampolines (os/milton/tbx_gate.asm). */
extern int  tbx_tenant_call(uint32_t fn, uint32_t psp);
extern void tbx_tenant_abort(void) __attribute__((noreturn));

/* ===========================================================================
 * 1. STATE -- ONE slot (D1-3 / DEC-AC3-1 V1 scope).
 * ===========================================================================*/

/* A region plus its caller-supplied backing, carved from the tenant's RECORDS
 * arena (AC-2: everything the shell reads at teardown lives there). The visible
 * region of a window among several can be ragged, so the scratch pair is sized
 * above the three per-window regions. */
enum { TBX_ROWS = 32, TBX_POOL = 128, TBX_SROWS = 64, TBX_SPOOL = 256 };
typedef struct tbx_rgn {
    region_t  r;
    rgn_row_t rows[TBX_ROWS];
    int16_t   pool[TBX_POOL];
} tbx_rgn_t;
typedef struct tbx_srgn {
    region_t  r;
    rgn_row_t rows[TBX_SROWS];
    int16_t   pool[TBX_SPOOL];
} tbx_srgn_t;

typedef struct tbx_slot {
    uint8_t      *block;        /* the carved code block (NULL == slot free)  */
    tenant_plan_t plan;         /* psp / load base / module / entry           */
    FlairApp     *app;          /* non-NULL once REGISTERed                   */
    uint32_t      evbuf;        /* validated flat FlairEvent address          */
    uint32_t      eventproc;    /* validated flat eventProc address           */
    WindowRecord *win;          /* the V1 single window (token 1)             */
    tbx_srgn_t   *sa, *sb;      /* draw-clip scratch                          */
    uint8_t       in_call;      /* tenant code is executing                   */
    uint8_t       in_update;    /* an updateEvt is being delivered            */
    uint8_t       affirm;       /* first NEWWINDOW asked for affirmation      */
    uint8_t       exit_req;     /* TBX_EXIT seen                              */
    uint8_t       crashed;      /* fault triaged inside the image             */
    uint8_t       torn_down;    /* terminate/kill already ran (close hook)    */
    int32_t       exit_rc;
    char          file[16];     /* the 8.3 file name (markers)                */
} tbx_slot_t;

static tbx_host_t g_host;
static tbx_slot_t g_slot;

/* The registered app name. OUTSIDE the slot on purpose: FlairApp.name points
 * here, and FlairProcess_close_window hands that pointer back to the pump to
 * print AFTER teardown ("the stable app-name pointer", process.h) -- so it
 * must survive the slot clear. Overwritten only by the next REGISTER. */
static char       g_app_name[FLAIR_TENANT_NAME_MAX];

/* ===========================================================================
 * 2. SMALL FREESTANDING HELPERS (serial formatting; no libc)
 * ===========================================================================*/
static void tputs(const char *s)
{
    if (g_host.puts != 0 && s != 0) g_host.puts(s);
}

static void tputu(uint32_t v)
{
    char b[11];
    int i = 10;
    b[i] = '\0';
    do { b[--i] = (char)('0' + (v % 10u)); v /= 10u; } while (v != 0u && i > 0);
    tputs(&b[i]);
}

static void tputi(int32_t v)
{
    if (v < 0) { tputs("-"); tputu((uint32_t)(-(v + 1)) + 1u); }
    else tputu((uint32_t)v);
}

static void tputx(uint32_t v, int digits)
{
    static const char H[] = "0123456789abcdef";
    char b[9];
    int n = (digits > 8) ? 8 : digits;
    for (int i = 0; i < n; i++)
        b[i] = H[(v >> ((n - 1 - i) * 4)) & 0xFu];
    b[n] = '\0';
    tputs(b);
}

static void tbx_zero(void *p, uint32_t n)
{
    uint8_t *b = (uint8_t *)p;
    for (uint32_t i = 0; i < n; i++) b[i] = 0u;
}

/* Fail loud (Rule 2): marker + HALTED, then a deterministic halt. */
__attribute__((noreturn)) static void tbx_panic(const char *why)
{
    tputs("PANIC tbx: ");
    tputs(why);
    tputs("\nHALTED\n");
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}

/* ===========================================================================
 * 3. IMAGE-RANGE VALIDATION (the FlairTenantRec rules, spec Sec 5)
 * ===========================================================================*/
static int in_image(uint32_t addr, uint32_t len)
{
    uint32_t lo = g_slot.plan.load_base;
    uint32_t hi = lo + g_slot.plan.module_len;
    if (g_slot.block == 0) return 0;
    if (addr < lo || addr > hi) return 0;
    if (len > hi - addr) return 0;
    return 1;
}

/* A NUL-terminated string wholly inside the image, at most `max` bytes incl.
 * the NUL. Returns its length, or -1. */
static int32_t image_str(uint32_t addr, uint32_t max)
{
    for (uint32_t i = 0; i < max; i++) {
        if (!in_image(addr + i, 1u)) return -1;
        if (*(const volatile uint8_t *)(uintptr_t)(addr + i) == 0u)
            return (int32_t)i;
    }
    return -1;
}

static uint32_t rd32(uint32_t addr)
{
    return *(const volatile uint32_t *)(uintptr_t)addr;
}

static uint32_t rd16(uint32_t addr)
{
    return *(const volatile uint16_t *)(uintptr_t)addr;
}

/* ===========================================================================
 * 4. THE SLOT GUARD + THE CARVE CALLBACK
 * ===========================================================================*/
static int slot_occupied(void)
{
#ifdef TBX_MUT_NO_SLOT_GUARD
    /* MUTANT TBX_MUT_NO_SLOT_GUARD (Rule 6; test-flair-app-launch-mutant):
     * the one-slot model is never enforced, so a second double-click loads and
     * REGISTERs a second copy over the first -- two TENANT-REGISTER ok lines and
     * no TENANT-SLOT-BUSY. NEVER in a real build. */
    return 0;
#else
    return g_slot.block != 0;
#endif
}

static void *carve_code(uint32_t len, void *user)
{
    (void)user;
    return flair_alloc(g_host.master, FLAIR_CLASS_GENERAL, len);
}

/* ===========================================================================
 * 5. THE KERNEL-OWNED VTABLE (DEC-AC3-3)
 * ===========================================================================*/
static void disk_tenant_event(FlairApp *self, const EventRecord *ev);
static void disk_tenant_close(FlairApp *self);

static const FlairAppProcs disk_tenant_procs = {
    (int (*)(FlairApp *, const FlairLaunchParams *))0,   /* open: entry ran   */
    disk_tenant_event,                                   /* event (REQUIRED)  */
    (void (*)(FlairApp *))0,                             /* idle              */
    disk_tenant_close                                    /* close             */
};

/* Run tenant code at `fn`. A crash unwinds here (tbx_tenant_call returns 1)
 * with the fault already recorded by tbx_fault_triage. */
static void run_tenant(uint32_t fn)
{
    int crashed;
    if (g_slot.in_call) tbx_panic("tenant code re-entered from inside a trap");
    g_slot.in_call = 1u;
    crashed = tbx_tenant_call(fn, g_slot.plan.psp_addr);
    g_slot.in_call = 0u;
    g_slot.in_update = 0u;
    if (crashed) g_slot.crashed = 1u;
}

/* Push-callback delivery: copy the cooked EventRecord into the tenant's flat
 * FlairEvent (widening each field to int32) and call its eventProc. Returning
 * IS the yield. A dying/crashed tenant receives nothing more. */
static void disk_tenant_event(FlairApp *self, const EventRecord *ev)
{
    volatile int32_t *eb;

    if (self == (FlairApp *)0 || self != g_slot.app)
        tbx_panic("event for an app that is not the resident disk tenant");
    if (ev == (const EventRecord *)0) return;
    if (g_slot.exit_req || g_slot.crashed || g_slot.torn_down) return;

    tputs("TENANT-EVT what=");
    tputu((uint32_t)ev->what);
    tputs("\n");

    eb = (volatile int32_t *)(uintptr_t)g_slot.evbuf;
    eb[FLAIR_EVBUF_WHAT_OFF / 4u]   = (int32_t)ev->what;
    eb[FLAIR_EVBUF_MSG_OFF / 4u]    = (int32_t)ev->message;
    eb[FLAIR_EVBUF_MODS_OFF / 4u]   = (int32_t)ev->modifiers;
    eb[FLAIR_EVBUF_WHEN_OFF / 4u]   = (int32_t)ev->when;
    eb[FLAIR_EVBUF_WHEREH_OFF / 4u] = (int32_t)ev->where.h;
    eb[FLAIR_EVBUF_WHEREV_OFF / 4u] = (int32_t)ev->where.v;

    g_slot.in_update = (ev->what == (uint16_t)updateEvt) ? 1u : 0u;
    run_tenant(g_slot.eventproc);
}

/* The close hook FlairProcess_terminate calls (step 1). V1 tenants register no
 * close callback, so NO tenant code runs here; it only records that the AC-2
 * trio teardown is under way so tbx_reap frees the code block after it. */
static void disk_tenant_close(FlairApp *self)
{
    if (self == g_slot.app) g_slot.torn_down = 1u;
}

/* ===========================================================================
 * 6. DRAWING (clipped: visible INTERSECT content [INTERSECT update])
 * ===========================================================================*/
static void srgn_attach(tbx_srgn_t *s)
{
    s->r.rows = s->rows; s->r.cap_rows = TBX_SROWS;
    s->r.x_pool = s->pool; s->r.x_pool_cap = TBX_SPOOL;
    region_set_empty(&s->r);
}

static void rgn_attach(tbx_rgn_t *s)
{
    s->r.rows = s->rows; s->r.cap_rows = TBX_ROWS;
    s->r.x_pool = s->pool; s->r.x_pool_cap = TBX_POOL;
    region_set_empty(&s->r);
}

/* Build the draw clip for the window into g_slot.sa->r (returned). */
static const region_t *draw_clip(void)
{
    WindowRecord *w = g_slot.win;
    ComputeVisible(g_host.wm, w, &g_slot.sa->r);
    region_op(&g_slot.sb->r, &g_slot.sa->r, w->contRgn, RGN_OP_INTERSECT);
    if (g_slot.in_update) {
        region_op(&g_slot.sa->r, &g_slot.sb->r, w->updateRgn, RGN_OP_INTERSECT);
        return &g_slot.sa->r;
    }
    return &g_slot.sb->r;
}

static uint32_t color_px(uint32_t tok)
{
    int part;
    switch (tok) {
    case TBX_COLOR_BLACK: part = (int)FLAIR_PART_FRAME;        break;
    case TBX_COLOR_GRAY:  part = (int)FLAIR_PART_BTNFACE;      break;
    case TBX_COLOR_NAVY:  part = (int)FLAIR_PART_CAPTION_NAVY; break;
    default:              part = (int)FLAIR_PART_CONTENT;      break;
    }
    return flair_look_pixel_depth(g_host.surface->bpp, part);
}

static void draw_text(int32_t gx, int32_t gy, const char *s, uint32_t fg,
                      uint32_t bg, const region_t *clip)
{
    const bitmap_t *dst = g_host.surface;
    int32_t len = 0;
    rgn_rect_t box;

    /* spec/toolbox_gate.h Sec 7: the run is the proportional Chicago 12
     * string (the sum of the NFNT 5478 advances, bead initech-tdnl.33); the
     * box behind it is that wide and one CHICAGO_CELL_H line tall. */
    int32_t run_w = 0;
    while (s[len] != '\0') run_w += chicago_advance((int)(unsigned char)s[len++]);
    box.left = (int16_t)gx; box.top = (int16_t)gy;
    box.right = (int16_t)(gx + run_w);
    box.bottom = (int16_t)(gy + CHICAGO_CELL_H);
    blitter_fill_rect_clipped(dst, box, bg, clip);
    for (int32_t k = 0, x0 = gx; k < len; k++) {
        int ch = (int)(unsigned char)s[k];
        int aw = chicago_advance(ch);
        for (int r = 0; r < CHICAGO_CELL_H; r++) {
            int32_t py = gy + r;
            unsigned int bits = chicago_cell_bits(ch, r);
            if (py < 0 || py >= (int32_t)dst->height) continue;
            for (int c = 0; c < aw; c++) {
                int32_t px = x0 + c;
                if ((bits & (0x8000u >> c)) == 0u) continue;
                if (px < 0 || px >= (int32_t)dst->width) continue;
                if (!region_contains_point(clip, (int16_t)px, (int16_t)py))
                    continue;
                surface_put_pixel(dst, (uint32_t)py * dst->pitch +
                                       (uint32_t)px * dst->bytes_per_pixel, fg);
            }
        }
        x0 += aw;
    }
}

/* ===========================================================================
 * 7. THE VERBS
 * ===========================================================================*/
static int32_t v_register(uint32_t rec)
{
    uint32_t namep, evb, proc;
    int32_t nlen;

    if (g_slot.exit_req || g_slot.crashed) return TBX_ERR_NOTREG;
    if (g_slot.app != (FlairApp *)0) {
#ifndef TBX_MUT_NO_SLOT_GUARD
        tputs("TENANT-SLOT-BUSY name=");
        tputs(g_slot.file);
        tputs("\n");
        return TBX_ERR_BUSY;
#endif
    }
    if (!in_image(rec, FLAIR_TENANT_REC_BYTES)) {
        tputs("TENANT-REGISTER-BAD why=rec-outside-image\n");
        return TBX_ERR_BADREC;
    }
    namep = rd32(rec + FLAIR_TENANT_REC_NAME_OFF);
    evb   = rd32(rec + FLAIR_TENANT_REC_EVB_OFF);
    proc  = rd32(rec + FLAIR_TENANT_REC_PROC_OFF);
    if (rd32(rec + FLAIR_TENANT_REC_TAG_OFF) != FLAIR_TENANT_REC_TAG) {
        tputs("TENANT-REGISTER-BAD why=tag\n");
        return TBX_ERR_BADREC;
    }
    if (rd32(rec + FLAIR_TENANT_REC_BASE_OFF) != g_slot.plan.load_base ||
        rd32(rec + FLAIR_TENANT_REC_LEN_OFF) != g_slot.plan.module_len) {
        tputs("TENANT-REGISTER-BAD why=image-mismatch\n");
        return TBX_ERR_BADREC;
    }
    nlen = image_str(namep, FLAIR_TENANT_NAME_MAX);
    if (nlen <= 0 || !in_image(evb, FLAIR_EVBUF_BYTES) || !in_image(proc, 1u)) {
        tputs("TENANT-REGISTER-BAD why=pointer-outside-image\n");
        return TBX_ERR_BADREC;
    }
    for (int32_t i = 0; i <= nlen; i++)
        g_app_name[i] = *(const volatile char *)(uintptr_t)(namep + (uint32_t)i);

    g_slot.app = FlairProcess_admit(g_host.list, g_host.master,
                                    &disk_tenant_procs, g_app_name,
                                    TBX_TENANT_RECORDS_BUDGET,
                                    TBX_TENANT_DATA_BUDGET);
    if (g_slot.app == (FlairApp *)0) {
        tputs("TENANT-REGISTER-FAIL nomem\n");
        return TBX_ERR_NOMEM;
    }
    g_slot.evbuf     = evb;
    g_slot.eventproc = proc;
    tputs("TENANT-REGISTER ok app=");
    tputs(g_app_name);
    tputs("\n");
    return TBX_OK;
}

static int32_t v_newwindow(const uint32_t *a)
{
    int32_t l = (int32_t)a[0], t = (int32_t)a[1], r = (int32_t)a[2],
            b = (int32_t)a[3];
    FlairApp *app = g_slot.app;
    WindowRecord *rec;
    tbx_rgn_t *rs, *rc, *ru;
    rgn_rect_t fr;

    if (app == (FlairApp *)0 || g_slot.exit_req) return TBX_ERR_NOTREG;
    if (g_slot.win != (WindowRecord *)0) return TBX_ERR_BUSY;   /* V1 cap 1 */
    if (l < 0 || r > FLAIR_SCREEN_W || t < 2 * FLAIR_CHROME_MENUBAR_H ||
        b > FLAIR_SCREEN_H || r - l < 96 || b - t < 64)
        return TBX_ERR_BADARG;

    rec = (WindowRecord *)flair_alloc(&app->records_arena, FLAIR_CLASS_HANDLE,
                                      (uint32_t)sizeof *rec);
    rs = (tbx_rgn_t *)flair_alloc(&app->records_arena, FLAIR_CLASS_REGION,
                                  (uint32_t)sizeof *rs);
    rc = (tbx_rgn_t *)flair_alloc(&app->records_arena, FLAIR_CLASS_REGION,
                                  (uint32_t)sizeof *rc);
    ru = (tbx_rgn_t *)flair_alloc(&app->records_arena, FLAIR_CLASS_REGION,
                                  (uint32_t)sizeof *ru);
    g_slot.sa = (tbx_srgn_t *)flair_alloc(&app->records_arena,
                                          FLAIR_CLASS_REGION,
                                          (uint32_t)sizeof(tbx_srgn_t));
    g_slot.sb = (tbx_srgn_t *)flair_alloc(&app->records_arena,
                                          FLAIR_CLASS_REGION,
                                          (uint32_t)sizeof(tbx_srgn_t));
    if (rec == 0 || rs == 0 || rc == 0 || ru == 0 || g_slot.sa == 0 ||
        g_slot.sb == 0)
        return TBX_ERR_NOMEM;   /* the arena dies with the app at teardown */

    tbx_zero(rec, (uint32_t)sizeof *rec);
    rgn_attach(rs); rgn_attach(rc); rgn_attach(ru);
    srgn_attach(g_slot.sa); srgn_attach(g_slot.sb);
    rec->strucRgn   = &rs->r;
    rec->contRgn    = &rc->r;
    rec->updateRgn  = &ru->r;
    rec->nextWindow = (WindowRecord *)0;

    fr.left = (int16_t)l; fr.top = (int16_t)t;
    fr.right = (int16_t)r; fr.bottom = (int16_t)b;
    NewDocumentWindow(g_host.wm, rec, fr, (int16_t)documentKind,
                      (uint8_t)(a[4] != 0u ? 1u : 0u));
    rec->refCon  = (int32_t)(uintptr_t)app;   /* the demux key, Sec 3.1 */
    app->windows = rec;
    g_slot.win   = rec;
    g_slot.affirm = 1u;                       /* DEC-AC3-4 */
    return 1;                                 /* window token 1 (opaque) */
}

static int check_win(uint32_t tok)
{
    return g_slot.app != (FlairApp *)0 && !g_slot.exit_req &&
           g_slot.win != (WindowRecord *)0 && tok == 1u;
}

static int32_t v_setwtitle(const uint32_t *a)
{
    if (!check_win(a[0])) return TBX_ERR_BADARG;
    if (image_str(a[1], 64u) < 0) return TBX_ERR_BADARG;
    SetWTitle(g_host.wm, g_slot.win, (const char *)(uintptr_t)a[1]);
    return TBX_OK;
}

static int32_t v_textdraw(const uint32_t *a)
{
    rgn_rect_t c;
    if (!check_win(a[0])) return TBX_ERR_BADARG;
    if (image_str(a[3], 81u) < 0) return TBX_ERR_BADARG;
    if (a[4] >= TBX_COLOR_COUNT || a[5] >= TBX_COLOR_COUNT) return TBX_ERR_BADARG;
    if (g_host.surface == (const bitmap_t *)0) return TBX_OK;
    c = region_get_bbox(g_slot.win->contRgn);
    draw_text((int32_t)c.left + (int32_t)a[1], (int32_t)c.top + (int32_t)a[2],
              (const char *)(uintptr_t)a[3], color_px(a[4]), color_px(a[5]),
              draw_clip());
    return TBX_OK;
}

static int32_t v_fillrect(const uint32_t *a)
{
    rgn_rect_t c, r;
    if (!check_win(a[0])) return TBX_ERR_BADARG;
    if (a[5] >= TBX_COLOR_COUNT) return TBX_ERR_BADARG;
    if ((int32_t)a[1] >= (int32_t)a[3] || (int32_t)a[2] >= (int32_t)a[4])
        return TBX_ERR_BADARG;
    if (g_host.surface == (const bitmap_t *)0) return TBX_OK;
    c = region_get_bbox(g_slot.win->contRgn);
    /* content-local -> global, clamped to int16 by the content box itself */
    r.left   = (int16_t)((int32_t)c.left + (int32_t)a[1] < c.right
                         ? (int32_t)c.left + (int32_t)a[1] : c.right);
    r.top    = (int16_t)((int32_t)c.top + (int32_t)a[2] < c.bottom
                         ? (int32_t)c.top + (int32_t)a[2] : c.bottom);
    r.right  = (int16_t)((int32_t)c.left + (int32_t)a[3] < c.right
                         ? (int32_t)c.left + (int32_t)a[3] : c.right);
    r.bottom = (int16_t)((int32_t)c.top + (int32_t)a[4] < c.bottom
                         ? (int32_t)c.top + (int32_t)a[4] : c.bottom);
    blitter_fill_rect_clipped(g_host.surface, r, color_px(a[5]), draw_clip());
    return TBX_OK;
}

/* SETMBAR (spec Sec 9; bead initech-cnpm). The image-range half of the
 * validation goes through MB_IN so the MBAR_NO_BOUNDS mutant can compile
 * exactly that half out and nothing else. */
#ifndef TBX_MUT_MBAR_NO_BOUNDS
#define MB_IN(a, n) in_image((a), (n))
#else
/* MUTANT TBX_MUT_MBAR_NO_BOUNDS (Rule 6; test-flair-app-launch-mutant): every
 * image-range check of SETMBAR is true, so a bar outside the image is
 * installed. NEVER in a real build. */
#define MB_IN(a, n) ((void)(a), (void)(n), 1)
#endif

/* An ASCIZ string of at most TBX_MBAR_STR_MAX-1 chars wholly in the image:
 * its length, or -1. */
static int32_t mb_str(uint32_t a)
{
    for (uint32_t i = 0; i < TBX_MBAR_STR_MAX; i++) {
        if (!MB_IN(a + i, 1u)) return -1;
        if (*(const volatile uint8_t *)(uintptr_t)(a + i) == 0u)
            return (int32_t)i;
    }
    return -1;
}

static int32_t mbar_bad(const char *why)
{
    tputs("TENANT-SETMBAR-BAD why=");
    tputs(why);
    tputs("\n");
    return TBX_ERR_BADARG;
}

static int32_t v_setmbar(uint32_t bar)
{
    uint32_t menus, n, x;

    if (g_slot.app == (FlairApp *)0 || g_slot.exit_req) return TBX_ERR_NOTREG;
    if (g_slot.win != (WindowRecord *)0) return TBX_ERR_BUSY;  /* Sec 9 V2 */
    if (!MB_IN(bar, TBX_MBAR_BYTES)) return mbar_bad("bar-outside-image");
    menus = rd32(bar + TBX_MBAR_MENUS_OFF);
    n     = rd16(bar + TBX_MBAR_COUNT_OFF);
    if (n > TBX_MBAR_MAX_MENUS) return mbar_bad("too-many-menus");
    if (n != 0u && !MB_IN(menus, n * TBX_MINFO_BYTES))
        return mbar_bad("menus-outside-image");
    /* menu.h Sec 5: the Apple slot, then text_measure(title) + 2*PAD per
     * title -- the proportional Chicago 12 width (spec/toolbox_gate.h Sec 9;
     * bead initech-tdnl.33), summed from the validated in-image bytes. */
    x = (*(const volatile uint8_t *)(uintptr_t)(bar + TBX_MBAR_APPLE_OFF) != 0u)
            ? (uint32_t)FLAIR_MENU_APPLE_W : 0u;
    for (uint32_t k = 0; k < n; k++) {
        uint32_t m     = menus + k * TBX_MINFO_BYTES;
        uint32_t items = rd32(m + TBX_MINFO_ITEMS_OFF);
        uint32_t ni    = rd16(m + TBX_MINFO_COUNT_OFF);
        int32_t  tl    = mb_str(rd32(m + TBX_MINFO_TITLE_OFF));
        if ((int16_t)rd16(m + TBX_MINFO_ID_OFF) < 1) return mbar_bad("menu-id");
        if (tl < 1) return mbar_bad("title");
        {
            uint32_t ta = rd32(m + TBX_MINFO_TITLE_OFF);
            for (int32_t i = 0; i < tl; i++)
                x += (uint32_t)chicago_advance(
                         (int)*(const volatile uint8_t *)(uintptr_t)(ta + (uint32_t)i));
        }
        x += 2u * (uint32_t)FLAIR_MENU_TITLE_PAD;
        if (ni > TBX_MBAR_MAX_ITEMS) return mbar_bad("too-many-items");
        if (ni != 0u && !MB_IN(items, ni * TBX_MITEM_BYTES))
            return mbar_bad("items-outside-image");
        for (uint32_t i = 0; i < ni; i++)
            if (mb_str(rd32(items + i * TBX_MITEM_BYTES + TBX_MITEM_TEXT_OFF)) < 0)
                return mbar_bad("item-text");
    }
    if (x > (uint32_t)FLAIR_SCREEN_W) return mbar_bad("too-wide");
#ifndef TBX_MUT_MBAR_NOT_SWAPPED
    g_slot.app->menubar = (MenuBar *)(uintptr_t)bar;
#else
    /* MUTANT TBX_MUT_MBAR_NOT_SWAPPED (Rule 6; test-flair-app-launch-mutant):
     * the bar validates and the call reports success, but it is never
     * installed -- band 2 keeps the shell fallback bar while the tenant is
     * foreground. NEVER in a real build. */
#endif
    return TBX_OK;
}

static int32_t v_exit(uint32_t rc)
{
    if (g_slot.app == (FlairApp *)0) return TBX_ERR_NOTREG;
    g_slot.exit_req = 1u;
    g_slot.exit_rc  = (int32_t)rc;
    return TBX_OK;
}

/* ===========================================================================
 * 8. THE GATE -- AX demux + the per-call TENANT-GATE trace (DEC-AC3-2)
 * ===========================================================================*/
static void trace_str(const char *key, uint32_t p)
{
    tputs(key);
    if (image_str(p, 81u) >= 0) {
        tputs("\"");
        tputs((const char *)(uintptr_t)p);
        tputs("\"");
    } else {
        tputs("?");
    }
}

static void trace_int(const char *key, uint32_t v)
{
    tputs(key);
    tputi((int32_t)v);
}

void tbx_gate_dispatch(uint8_t *frame)
{
    volatile uint32_t *eax_slot = (volatile uint32_t *)(void *)(frame + 28);
    uint32_t ax = *eax_slot & 0xFFFFu;
    uint32_t a[TBX_ARGC_MAX];
    uint32_t argc = 0u;
    int32_t rc;

    switch (ax) {
    case TBX_REGISTER:  argc = TBX_ARGC_REGISTER;  break;
    case TBX_NEWWINDOW: argc = TBX_ARGC_NEWWINDOW; break;
    case TBX_SETWTITLE: argc = TBX_ARGC_SETWTITLE; break;
    case TBX_TEXTDRAW:  argc = TBX_ARGC_TEXTDRAW;  break;
    case TBX_FILLRECT:  argc = TBX_ARGC_FILLRECT;  break;
    case TBX_EXIT:      argc = TBX_ARGC_EXIT;      break;
    case TBX_SETMBAR:   argc = TBX_ARGC_SETMBAR;   break;
    default:            argc = 0u;                 break;
    }
    for (uint32_t i = 0; i < argc; i++)
        a[i] = *(const volatile uint32_t *)(void *)(frame + TBX_GATE_ARG_OFFSET +
                                                    4u * i);

    if (!g_slot.in_call || g_slot.block == 0) {
        rc = TBX_ERR_NOTTENANT;           /* a stray int 81h from the kernel */
    } else {
        switch (ax) {
        case TBX_REGISTER:  rc = v_register(a[0]); break;
        case TBX_NEWWINDOW: rc = v_newwindow(a);   break;
        case TBX_SETWTITLE: rc = v_setwtitle(a);   break;
        case TBX_TEXTDRAW:  rc = v_textdraw(a);    break;
        case TBX_FILLRECT:  rc = v_fillrect(a);    break;
        case TBX_EXIT:      rc = v_exit(a[0]);     break;
        case TBX_SETMBAR:   rc = v_setmbar(a[0]);  break;
        default:            rc = TBX_ERR_BADCODE;  break;
        }
    }

    tputs("TENANT-GATE ax=0x");
    tputx(ax, 4);
    switch (ax) {
    case TBX_REGISTER:
        tputs(" rec=+0x");
        tputx(a[0] - g_slot.plan.load_base, 4);
        break;
    case TBX_NEWWINDOW:
        trace_int(" l=", a[0]); trace_int(" t=", a[1]);
        trace_int(" r=", a[2]); trace_int(" b=", a[3]);
        trace_int(" goaway=", a[4]);
        break;
    case TBX_SETWTITLE:
        trace_int(" win=", a[0]); trace_str(" s=", a[1]);
        break;
    case TBX_TEXTDRAW:
        trace_int(" win=", a[0]); trace_int(" x=", a[1]); trace_int(" y=", a[2]);
        trace_str(" s=", a[3]); trace_int(" fg=", a[4]); trace_int(" bg=", a[5]);
        break;
    case TBX_FILLRECT:
        trace_int(" win=", a[0]); trace_int(" l=", a[1]); trace_int(" t=", a[2]);
        trace_int(" r=", a[3]); trace_int(" b=", a[4]); trace_int(" c=", a[5]);
        break;
    case TBX_EXIT:
        trace_int(" rc=", a[0]);
        break;
    case TBX_SETMBAR:
        tputs(" bar=+0x");
        tputx(a[0] - g_slot.plan.load_base, 4);
        break;
    default:
        break;
    }
    tputs(" -> ");
    tputi(rc);
    tputs("\n");
    *eax_slot = (uint32_t)rc;
}

/* ===========================================================================
 * 9. LIFECYCLE
 * ===========================================================================*/
void tbx_bind(const tbx_host_t *host)
{
    if (host == (const tbx_host_t *)0 || host->list == 0 || host->wm == 0 ||
        host->master == 0)
        tbx_panic("tbx_bind: incomplete host");
    g_host = *host;
    tbx_zero(&g_slot, (uint32_t)sizeof g_slot);
}

static void slot_clear(void)
{
    tbx_zero(&g_slot, (uint32_t)sizeof g_slot);
}

/* Free the code block LAST (DEC-AC3-1 strict reverse) and report. */
static void finish(const char *via, int32_t rc)
{
#ifndef TBX_MUT_EXIT_LEAK
    flair_free(g_host.master, FLAIR_CLASS_GENERAL, g_slot.block);
#else
    /* MUTANT TBX_MUT_EXIT_LEAK (Rule 6; test-flair-app-launch-mutant): the
     * code block is never returned, so every launch/exit cycle bumps a fresh
     * one and TENANT-HEAPAVAIL drifts DOWN (the O-3 avail-stable invariant goes
     * RED). NEVER in a real build. */
#endif
    tputs("TENANT-EXIT rc=");
    tputi(rc);
    tputs(" via=");
    tputs(via);
    tputs("\nTENANT-HEAPAVAIL n=");
    tputu(flair_heap_avail(g_host.master));
    tputs("\n");
    slot_clear();
}

int tbx_launch(const char *name83, uint16_t dir_start)
{
    void *blk = (void *)0;
    loader_status_t st;
    int i;

    if (g_host.master == (flair_heap_t *)0) tbx_panic("tbx_launch before tbx_bind");
    if (slot_occupied()) {
        tputs("TENANT-SLOT-BUSY name=");
        tputs(name83);
        tputs("\n");
        return TBX_ERR_BUSY;
    }
    slot_clear();
    st = loader_load_tenant(name83, dir_start, carve_code, (void *)0, &blk,
                            &g_slot.plan);
    if (st != LOADER_OK) {
        if (blk != (void *)0)
            flair_free(g_host.master, FLAIR_CLASS_GENERAL, blk);
        slot_clear();
        tputs("TENANT-LOAD-FAIL name=");
        tputs(name83);
        tputs(" rc=");
        tputu((uint32_t)st);
        tputs("\n");
        return (st == LOADER_ERR_NOMEM) ? TBX_ERR_NOMEM : TBX_ERR_BADARG;
    }
    g_slot.block = (uint8_t *)blk;
    for (i = 0; i < 15 && name83[i] != '\0'; i++) g_slot.file[i] = name83[i];
    g_slot.file[i] = '\0';

    tputs("TENANT-LOAD name=");
    tputs(g_slot.file);
    tputs(" len=");
    tputu(g_slot.plan.module_len);
    tputs(" base=0x");
    tputx(g_slot.plan.load_base, 8);
    tputs("\n");

    run_tenant(g_slot.plan.entry);
    return TBX_OK;
}

FlairApp *tbx_take_affirm(void)
{
    if (!g_slot.affirm || g_slot.app == (FlairApp *)0 || g_slot.exit_req ||
        g_slot.crashed || g_slot.torn_down)
        return (FlairApp *)0;
    g_slot.affirm = 0u;
    return g_slot.app;
}

int tbx_owns_window(const WindowRecord *w)
{
    return w != (const WindowRecord *)0 && g_slot.block != 0 && w == g_slot.win;
}

int tbx_reap(void)
{
    if (g_slot.block == 0 || g_slot.in_call) return 0;

    if (g_slot.app == (FlairApp *)0) {
        /* The entry returned (or crashed) without registering: nothing was
         * installed; the code block is the only thing to return. Not a
         * watchdog -- the tenant already gave the CPU back (BC-4). */
        if (g_slot.crashed) {
            finish("crash", -1);
        } else {
            tputs("TENANT-UNREGISTERED name=");
            tputs(g_slot.file);
            tputs("\n");
            flair_free(g_host.master, FLAIR_CLASS_GENERAL, g_slot.block);
            slot_clear();
        }
        return 0;
    }
    if (g_slot.torn_down) {                 /* close box ran terminate already */
        finish("close", 0);
        return 1;
    }
    if (g_slot.crashed) {                   /* death: never close(), never re-enter */
        FlairProcess_kill(g_host.list, g_host.wm, g_host.master, g_slot.app);
        finish("crash", -1);
        return 1;
    }
    if (g_slot.exit_req) {
        int32_t rc = g_slot.exit_rc;
        FlairProcess_terminate(g_host.list, g_host.wm, g_host.master,
                               g_slot.app);
        finish("exit", rc);
        return 1;
    }
    return 0;
}

void tbx_fault_triage(uint32_t vector, uint32_t eip)
{
    uint32_t lo, hi;
    if (!g_slot.in_call || g_slot.block == 0) return;   /* not tenant code */
    lo = g_slot.plan.load_base;
    hi = lo + g_slot.plan.module_len;
    if (eip < lo || eip >= hi) return;   /* a kernel fault during a trap: HALT */
    tputs("TENANT-CRASH vec=");
    tputu(vector);
    tputs(" eip=+0x");
    tputx(eip - lo, 4);
    tputs("\n");
    tbx_tenant_abort();                  /* unwinds to run_tenant; noreturn */
}
