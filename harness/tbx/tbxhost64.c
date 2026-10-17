/*
 * harness/tbx/tbxhost64.c -- the Free Pascal leg of the binding oracles: the
 * DECLARED HOST PRINT HARNESS of ADR-0007 DEC-10 (as reconciled,
 * docs/design/GUI-remediation-ADR-reconciliation.md Part B), written in C so
 * the factory stays C-only (CLAUDE.md Law 3) (beads initech-tdnl.22 R5.2,
 * initech-tdnl.23 R5.3).
 *
 * The SAME Pascal source the seed and Turbo Initech compile (os/tps/FLAIR.PAS
 * + an application) is compiled by fpc on the x86-64 host and linked with
 * this object, which implements every FLAIR.PAS `external` under its declared
 * name (fpc resolves `external;` to the identifier as written; x86-64 SysV:
 * a var parameter is a pointer, a string is a ShortString pointer, a char or
 * boolean is the low byte of its register). Each binding reproduces what
 * seed/rt/flairstub.asm does BEFORE its trap -- the local refusals (a name
 * past FLAIR_TENANT_NAME_MAX, a menu or item past the stub's storage, a
 * label past TBX_ALERT_LABEL_MAX), the ASCIZ copy, the menu graph built in
 * place, an empty alert label passed as 0 -- and what tbxhost32.c does after
 * it (the kernel's string bounds), then hands the decoded request to the
 * SAME model (tbxmodel.c). A trace that differs from the i386 runs of the
 * seed- and TPS-built binaries is a binding, stub or compiler divergence.
 *
 * EVENTS: fpc's program runs its main block and then its exit procedures,
 * so the first registration adds one (System.AddExitProc), and it delivers
 * tbxevents.txt (in the working directory) to the program's
 * FlairEventHandler -- the fpc-mangled name of FLAIR.PAS's `program FlairApp`
 * routine -- exactly as tbxhost32 delivers to eventProc.
 *
 * Freestanding (fpc links no libc): raw x86-64 syscalls. ASCII (Rule 12).
 */
#include "tbxmodel.h"

#define STUB_MENUS 8      /* seed/rt/flairstub.asm FLAIR_MAX_MENUS */
#define STUB_ITEMS 32     /* seed/rt/flairstub.asm FLAIR_MAX_ITEMS */
#define EVENTS_CAP (4 * 1024 * 1024)
#define EV_MAX     200000

typedef struct { int32_t w[6]; } flair_event_t;

extern void app_handler(flair_event_t *ev)
    __asm__("P$FLAIRAPP_$$_FLAIREVENTHANDLER$FLAIREVENT");
extern void fpc_add_exit_proc(void (*p)(void))
    __asm__("SYSTEM_$$_ADDEXITPROC$TPROCEDURE");

/* ---------------------------------------------------------------- syscalls */
static long sys3(long n, long a, long b, long c)
{
    long r;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b), "d"(c)
                     : "rcx", "r11", "memory");
    return r;
}

static char g_out[65536];
static int  g_outn;

static void flush(void)
{
    int p = 0;
    while (p < g_outn) {
        long w = sys3(1, 1, (long)(g_out + p), g_outn - p);
        if (w <= 0) break;
        p += (int)w;
    }
    g_outn = 0;
}

void tm_write(const char *s, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        if (g_outn == (int)sizeof g_out) flush();
        g_out[g_outn++] = s[i];
    }
}

/* gcc may call these for the struct copies below */
void *memcpy(void *d, const void *s, unsigned long n);
void *memset(void *d, int c, unsigned long n);
void *memcpy(void *d, const void *s, unsigned long n)
{
    char *dp = (char *)d;
    const char *sp = (const char *)s;
    while (n--) *dp++ = *sp++;
    return d;
}
void *memset(void *d, int c, unsigned long n)
{
    char *dp = (char *)d;
    while (n--) *dp++ = (char)c;
    return d;
}

/* ------------------------------------------------------- the session */
static int g_started, g_admitted;
static char g_evtext[EVENTS_CAP];
static int32_t g_ev[EV_MAX][6];

static void finish(void)
{
    if (g_admitted) {
        long fd = sys3(2, (long)"tbxevents.txt", 0, 0);
        long n = 0, got;
        int nev, i, k;
        if (fd < 0) { tm_puts("TBX64-FATAL no tbxevents.txt\n"); flush(); sys3(60, 2, 0, 0); }
        while (n < EVENTS_CAP) {
            got = sys3(0, fd, (long)(g_evtext + n), EVENTS_CAP - n);
            if (got <= 0) break;
            n += got;
        }
        sys3(3, fd, 0, 0);
        nev = tm_parse_events(g_evtext, (int)n, g_ev, EV_MAX);
        if (nev < 0) { tm_puts("TBX64-FATAL malformed event script\n"); flush(); sys3(60, 2, 0, 0); }
        for (i = 0; i < nev && !tm_exited(); i++) {
            flair_event_t ev;
            for (k = 0; k < 6; k++) ev.w[k] = g_ev[i][k];
            tm_event(g_ev[i]);
            app_handler(&ev);
        }
    }
    tm_puts("TBX END\n");
    flush();
}

static void start(void)
{
    if (g_started) return;
    g_started = 1;
    tm_reset();
    tm_puts("TBX ENTRY\n");
    fpc_add_exit_proc(finish);
}

/* ShortString -> NUL-terminated (the stub's flair_asciz): -1 when length + 1
 * exceeds cap (nothing copied). The copy stops being a C string at an
 * embedded NUL, exactly as the gate reads the stub's copy. */
static int asciz(const uint8_t *ss, char *out, int cap)
{
    int n = ss[0], i;
    if (n + 1 > cap) return -1;
    for (i = 0; i < n; i++) out[i] = (char)ss[1 + i];
    out[n] = 0;
    return n;
}

static int clen(const char *s)
{
    int n = 0;
    while (s[n]) n++;
    return n;
}

/* the kernel's image_str bound: the string with its NUL within max bytes */
static int fits(const char *s, int max)
{
    return clen(s) + 1 <= max;
}

static char t0[300], t1[300], t2[300], t3[300];

/* ------------------------------------------------------ the externals */
int32_t FlairRegister(const uint8_t *name)
{
    start();
    if (asciz(name, t0, FLAIR_TENANT_NAME_MAX) < 0) return TBX_ERR_BADARG;
    if (t0[0] == 0) return tm_register_bad("pointer-outside-image");
    if (tm_register(t0) != TBX_OK) return TBX_ERR_BUSY;
    g_admitted = 1;
    return TBX_OK;
}

int32_t FlairExit(int32_t code)
{
    start();
    return tm_exit(code);
}

int32_t FlairNewWindow(int32_t l, int32_t t, int32_t r, int32_t b, int32_t ga)
{
    start();
    return tm_newwindow(l, t, r, b, (uint32_t)ga);
}

int32_t FlairSetWTitle(int32_t win, const uint8_t *title)
{
    start();
    (void)asciz(title, t0, 256);
    if (!fits(t0, 64)) { tm_bad("SETWTITLE string"); return TBX_ERR_BADARG; }
    return tm_setwtitle(win, t0);
}

int32_t FlairContentOrigin(int32_t win)
{
    start();
    return tm_contentorigin(win);
}

int32_t FlairGlobalToLocal(int32_t win, int32_t h, int32_t v)
{
    start();
    return tm_globaltolocal(win, h, v);
}

int32_t FlairTextDraw(int32_t win, int32_t x, int32_t y, const uint8_t *s,
                      int32_t fg, int32_t bg)
{
    start();
    (void)asciz(s, t0, 256);
    if (!fits(t0, 81)) { tm_bad("TEXTDRAW string"); return TBX_ERR_BADARG; }
    return tm_textdraw(win, x, y, t0, (uint32_t)fg, (uint32_t)bg);
}

int32_t FlairDrawCells(int32_t win, int32_t x, int32_t y, const uint8_t *s,
                       int32_t fg, int32_t bg)
{
    start();
    (void)asciz(s, t0, 256);
    if (!fits(t0, (int)TBX_CELLS_MAX_RUN + 1)) { tm_bad("DRAWCELLS string"); return TBX_ERR_BADARG; }
    return tm_drawcells(win, x, y, t0, (uint32_t)fg, (uint32_t)bg);
}

int32_t FlairFillRect(int32_t win, int32_t l, int32_t t, int32_t r, int32_t b,
                      int32_t c)
{
    start();
    return tm_fillrect(win, l, t, r, b, (uint32_t)c);
}

/* the menu graph, built where the stub builds it */
static tm_mbar_t g_bar;
static int g_items;

int32_t FlairNewMenu(int32_t id, const uint8_t *title)
{
    tm_menu_t *m;
    start();
    if (g_bar.n_menus >= STUB_MENUS) return TBX_ERR_BADARG;
    m = &g_bar.menu[g_bar.n_menus];
    if (asciz(title, m->title, TBX_MBAR_STR_MAX) < 0) return TBX_ERR_BADARG;
    m->id = (int16_t)(id & 0xFFFF);
    m->n_items = 0;
    g_bar.n_menus++;
    return TBX_OK;
}

int32_t FlairAppendItem(const uint8_t *text, uint32_t cmd, uint32_t enabled)
{
    tm_menu_t *m;
    tm_item_t *it;
    int n;
    start();
    if (g_bar.n_menus == 0 || g_items >= STUB_ITEMS) return TBX_ERR_BADARG;
    m = &g_bar.menu[g_bar.n_menus - 1];
    it = &m->item[m->n_items];
    n = asciz(text, it->text, TBX_MBAR_STR_MAX);
    if (n < 0) return TBX_ERR_BADARG;
    it->mark = 0;
    it->cmd = (uint8_t)(cmd & 0xFFu);
    it->style = 0;
    it->enabled = (uint8_t)(enabled & 0xFFu);
    it->divider = (uint8_t)(n == 1 && it->text[0] == '-');
    m->n_items++;
    g_items++;
    return TBX_OK;
}

int32_t FlairSetMenuBar(uint32_t has_apple)
{
    int i;
    start();
    g_bar.has_apple = (int)(has_apple & 0xFFu);
    for (i = 0; i < g_bar.n_menus; i++)          /* tbxhost32's decode bound */
        if (g_bar.menu[i].n_items > (int)TBX_MBAR_MAX_ITEMS) {
            tm_bad("SETMBAR n_items");
            return TBX_ERR_BADARG;
        }
    return tm_setmbar(&g_bar);
}

int32_t FlairDrawMenuBar(void)
{
    start();
    return tm_drawmenubar();
}

int32_t FlairAlert(const uint8_t *text, const uint8_t *def,
                   const uint8_t *cancel, const uint8_t *other)
{
    int nc, no;
    start();
    (void)asciz(text, t0, 256);
    if (asciz(def, t1, TBX_ALERT_LABEL_MAX) < 0) return TBX_ERR_BADARG;
    nc = asciz(cancel, t2, TBX_ALERT_LABEL_MAX);
    if (nc < 0) return TBX_ERR_BADARG;
    no = asciz(other, t3, TBX_ALERT_LABEL_MAX);
    if (no < 0) return TBX_ERR_BADARG;
    if (!fits(t0, 3 * (int)TBX_ALERT_LINE_MAX)) { tm_bad("ALERT string"); return TBX_ERR_BADARG; }
    return tm_alert(t0, t1, nc ? t2 : 0, no ? t3 : 0);
}

int32_t FlairScrapPut(const uint8_t *s)
{
    start();
    return tm_scrap_put(s + 1, s[0]);
}

int32_t FlairScrapGet(uint8_t *s)
{
    uint8_t buf[256];
    int32_t rc, i;
    start();
    rc = tm_scrap_get(buf, 255u);
    if (rc < 0) return rc;
    if (rc > 255) return TBX_ERR_BADARG;
    s[0] = (uint8_t)rc;
    for (i = 0; i < rc; i++) s[1 + i] = buf[i];
    return rc;
}
