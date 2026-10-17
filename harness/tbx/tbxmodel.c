/*
 * harness/tbx/tbxmodel.c -- the host model of the Initech Toolbox Gate (see
 * tbxmodel.h). Factory C, freestanding (compiled -m32 for tbxhost32 /
 * test_flairstub and natively for tbxhost64). ASCII-clean (Rule 12).
 *
 * The answers follow os/flair/tbxgate.c where a tenant can observe them:
 *   REGISTER   0 once; a second registration TBX_ERR_BUSY.
 *   NEWWINDOW  the v_newwindow argument rules (l >= 0, r <= 640,
 *              t >= 2 * menubar, b <= 480, r - l >= 96, b - t >= 64,
 *              goAway bits 0..7 <= ASK, bits 8.. 0 or RDOC) -> the token
 *              1..TBX_TENANT_WINDOWS in creation order; TBX_ERR_BUSY after.
 *   CONTENTORIGIN / GLOBALTOLOCAL  the content's top-left is the frame's
 *              (l + 1, t + 19) -- the rDocProc offsets of
 *              spec/calcda/calcda_layout.h; a host model needs one fixed
 *              rule, not the kernel's region arithmetic.
 *   draws      0 for a live token, TBX_ERR_BADARG otherwise; TEXTDRAW's fg
 *              rule of spec Sec 7 (token < 4, face bits <= 7, font <= 1).
 *   SCRAP      the one desk scrap: PUT stores, GET returns the length or
 *              TBX_ERR_NOTYPE when empty, BADARG when it does not fit.
 *   EXIT       0, and no event is delivered after it (DEC-AC3-3).
 */
#include "tbxmodel.h"

#define SCREEN_W 640
#define SCREEN_H 480
#define MENUBAR_H 20
#define CONTENT_DX 1
#define CONTENT_DY 19

static struct {
    int     registered, exited;
    int     nwins;
    int32_t wl[TBX_TENANT_WINDOWS], wt[TBX_TENANT_WINDOWS];
    int     scrap_has;
    uint32_t scrap_len;
    uint8_t scrap[8192];
} g;

/* ---------------------------------------------------------------- output */
void tm_puts(const char *s)
{
    int n = 0;
    while (s[n]) n++;
    tm_write(s, n);
}

void tm_putd(int32_t v)
{
    char b[16];
    int n = 0, i;
    uint32_t m = (v < 0) ? (uint32_t)0 - (uint32_t)v : (uint32_t)v;
    do { b[n++] = (char)('0' + (int)(m % 10u)); m /= 10u; } while (m);
    if (v < 0) b[n++] = '-';
    for (i = 0; i < n / 2; i++) { char t = b[i]; b[i] = b[n - 1 - i]; b[n - 1 - i] = t; }
    tm_write(b, n);
}

void tm_putx(uint32_t v, int digits)
{
    char b[8];
    int i;
    for (i = 0; i < digits && i < 8; i++) {
        int d = (int)((v >> (4 * (digits - 1 - i))) & 15u);
        b[i] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
    }
    tm_write(b, i);
}

void tm_putq(const char *s)
{
    tm_write("\"", 1);
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c >= 0x20 && c < 0x7F && c != '"' && c != '\\') {
            tm_write((const char *)&c, 1);
        } else {
            tm_write("\\x", 2);
            tm_putx(c, 2);
        }
    }
    tm_write("\"", 1);
}

static void kv(const char *k, int32_t v)
{
    tm_puts(k);
    tm_putd(v);
}

static int32_t done(int32_t rc)
{
    tm_puts(" -> ");
    tm_putd(rc);
    tm_puts("\n");
    return rc;
}

/* ----------------------------------------------------------------- state */
void tm_reset(void)
{
    g.registered = 0;
    g.exited = 0;
    g.nwins = 0;
    g.scrap_has = 0;
    g.scrap_len = 0;
}

int tm_exited(void)     { return g.exited; }
int tm_registered(void) { return g.registered; }

static int live(int32_t win)
{
    return g.registered && !g.exited && win >= 1 && win <= g.nwins;
}

/* ----------------------------------------------------------------- verbs */
int32_t tm_register(const char *name)
{
    tm_puts("TBX REGISTER name=");
    tm_putq(name);
    if (g.registered) return done(TBX_ERR_BUSY);
    g.registered = 1;
    return done(TBX_OK);
}

int32_t tm_register_bad(const char *why)
{
    tm_puts("TBX REGISTER-BAD why=");
    tm_puts(why);
    return done(TBX_ERR_BADREC);
}

int32_t tm_exit(int32_t rc)
{
    kv("TBX EXIT rc=", rc);
    if (!g.registered) return done(TBX_ERR_NOTREG);
    g.exited = 1;
    return done(TBX_OK);
}

int32_t tm_newwindow(int32_t l, int32_t t, int32_t r, int32_t b, uint32_t goaway)
{
    uint32_t ga = goaway & 0xFFu, wdef = goaway & ~0xFFu;
    kv("TBX NEWWINDOW l=", l); kv(" t=", t); kv(" r=", r); kv(" b=", b);
    kv(" goaway=", (int32_t)goaway);
    if (!g.registered || g.exited) return done(TBX_ERR_NOTREG);
    if (g.nwins == (int)TBX_TENANT_WINDOWS) return done(TBX_ERR_BUSY);
    if (l < 0 || r > SCREEN_W || t < 2 * MENUBAR_H || b > SCREEN_H ||
        r - l < 96 || b - t < 64 || ga > TBX_GOAWAY_ASK ||
        (wdef != 0u && wdef != TBX_WDEF_RDOC))
        return done(TBX_ERR_BADARG);
    g.wl[g.nwins] = l;
    g.wt[g.nwins] = t;
    g.nwins++;
    return done(g.nwins);
}

int32_t tm_setwtitle(int32_t win, const char *s)
{
    kv("TBX SETWTITLE win=", win);
    tm_puts(" s=");
    tm_putq(s);
    return done(live(win) ? TBX_OK : TBX_ERR_BADARG);
}

int32_t tm_contentorigin(int32_t win)
{
    kv("TBX CONTENTORIGIN win=", win);
    if (!live(win)) return done(TBX_ERR_BADARG);
    return done((int32_t)(((uint32_t)(g.wt[win - 1] + CONTENT_DY) << 16) |
                          (uint32_t)(g.wl[win - 1] + CONTENT_DX)));
}

int32_t tm_globaltolocal(int32_t win, int32_t h, int32_t v)
{
    int32_t lh, lv;
    kv("TBX GLOBALTOLOCAL win=", win); kv(" h=", h); kv(" v=", v);
    if (!live(win)) return done(TBX_ERR_BADARG);
    lh = h - (g.wl[win - 1] + CONTENT_DX);
    lv = v - (g.wt[win - 1] + CONTENT_DY);
    if (lh < -32768 || lh > 32767 || lv < -32768 || lv > 32767)
        return done(TBX_ERR_BADARG);
    return done((int32_t)(((uint32_t)lv << 16) | ((uint32_t)lh & 0xFFFFu)));
}

static int text_fg_ok(uint32_t fg)
{
    uint32_t face = (fg >> TBX_TEXT_FACE_SHIFT) & 0xFFu;
    uint32_t font = (fg >> TBX_TEXT_FONT_SHIFT) & 0xFFu;
    return (fg & 0xFFu) < TBX_COLOR_COUNT && (fg >> 24) == 0u &&
           font <= TBX_TEXT_FONT_MAX && (face & ~TBX_TEXT_FACE_OK) == 0u;
}

static int32_t run(const char *verb, int32_t win, int32_t x, int32_t y,
                   const char *s, uint32_t fg, uint32_t bg, int cells)
{
    tm_puts(verb);
    kv(" win=", win); kv(" x=", x); kv(" y=", y);
    tm_puts(" s=");
    tm_putq(s);
    kv(" fg=", (int32_t)fg); kv(" bg=", (int32_t)bg);
    if (!live(win)) return done(TBX_ERR_BADARG);
    if (cells) {
        if ((fg & 0xFFu) >= TBX_COLOR_COUNT || (fg >> 16) != 0u ||
            (((fg >> 8) & 0xFFu) & ~TBX_TEXT_FACE_OK) != 0u)
            return done(TBX_ERR_BADARG);
    } else if (!text_fg_ok(fg)) {
        return done(TBX_ERR_BADARG);
    }
    if (bg >= TBX_COLOR_COUNT) return done(TBX_ERR_BADARG);
    return done(TBX_OK);
}

int32_t tm_textdraw(int32_t win, int32_t x, int32_t y, const char *s,
                    uint32_t fg, uint32_t bg)
{
    return run("TBX TEXTDRAW", win, x, y, s, fg, bg, 0);
}

int32_t tm_drawcells(int32_t win, int32_t x, int32_t y, const char *s,
                     uint32_t fg, uint32_t bg)
{
    return run("TBX DRAWCELLS", win, x, y, s, fg, bg, 1);
}

int32_t tm_fillrect(int32_t win, int32_t l, int32_t t, int32_t r, int32_t b,
                    uint32_t c)
{
    kv("TBX FILLRECT win=", win); kv(" l=", l); kv(" t=", t);
    kv(" r=", r); kv(" b=", b); kv(" c=", (int32_t)c);
    if (!live(win) || c >= TBX_COLOR_COUNT) return done(TBX_ERR_BADARG);
    return done(TBX_OK);
}

int32_t tm_setmbar(const tm_mbar_t *bar)
{
    int i, j;
    tm_puts("TBX SETMBAR");
    kv(" apple=", bar->has_apple);
    kv(" menus=", bar->n_menus);
    for (i = 0; i < bar->n_menus; i++) {
        const tm_menu_t *m = &bar->menu[i];
        kv(" [", m->id);
        tm_puts(" ");
        tm_putq(m->title);
        for (j = 0; j < m->n_items; j++) {
            const tm_item_t *it = &m->item[j];
            tm_puts(" {");
            tm_putq(it->text);
            kv(" cmd=", it->cmd);
            kv(" en=", it->enabled);
            kv(" div=", it->divider);
            kv(" mark=", it->mark);
            kv(" style=", it->style);
            tm_puts("}");
        }
        tm_puts("]");
    }
    if (!g.registered || g.exited) return done(TBX_ERR_NOTREG);
    return done(TBX_OK);
}

int32_t tm_drawmenubar(void)
{
    tm_puts("TBX DRAWMENUBAR");
    if (!g.registered || g.exited) return done(TBX_ERR_NOTREG);
    return done(TBX_OK);
}

int32_t tm_alert(const char *text, const char *def, const char *cancel,
                 const char *other)
{
    tm_puts("TBX ALERT text=");
    tm_putq(text);
    tm_puts(" def=");
    tm_putq(def);
    tm_puts(" cancel=");
    if (cancel) tm_putq(cancel); else tm_puts("0");
    tm_puts(" other=");
    if (other) tm_putq(other); else tm_puts("0");
    if (!g.registered || g.exited) return done(TBX_ERR_NOTREG);
    if (!def[0] || (!cancel && other)) return done(TBX_ERR_BADARG);
    return done(TBX_OK);
}

int32_t tm_scrap_put(const uint8_t *buf, uint32_t len)
{
    uint32_t i;
    tm_puts("TBX SCRAP-PUT n=");
    tm_putd((int32_t)len);
    tm_puts(" data=\"");
    for (i = 0; i < len && i < 300u; i++) {
        unsigned char c = buf[i];
        if (c >= 0x20 && c < 0x7F && c != '"' && c != '\\') {
            tm_write((const char *)&c, 1);
        } else {
            tm_write("\\x", 2);
            tm_putx(c, 2);
        }
    }
    tm_puts("\"");
    if (!g.registered || g.exited) return done(TBX_ERR_NOTREG);
    if (len > sizeof g.scrap) return done(TBX_ERR_BADARG);
    for (i = 0; i < len; i++) g.scrap[i] = buf[i];
    g.scrap_len = len;
    g.scrap_has = 1;
    return done(TBX_OK);
}

int32_t tm_scrap_get(uint8_t *buf, uint32_t cap)
{
    uint32_t i;
    tm_puts("TBX SCRAP-GET cap=");
    tm_putd((int32_t)cap);
    if (!g.registered || g.exited) return done(TBX_ERR_NOTREG);
    if (!g.scrap_has) return done(TBX_ERR_NOTYPE);
    if (buf == 0) return done((int32_t)g.scrap_len);
    if (g.scrap_len > cap) return done(TBX_ERR_BADARG);
    for (i = 0; i < g.scrap_len; i++) buf[i] = g.scrap[i];
    return done((int32_t)g.scrap_len);
}

int32_t tm_badcode(uint32_t ax)
{
    tm_puts("TBX BADCODE ax=0x");
    tm_putx(ax, 4);
    return done(TBX_ERR_BADCODE);
}

void tm_event(const int32_t ev[6])
{
    kv("EVT what=", ev[0]); kv(" msg=", ev[1]); kv(" mods=", ev[2]);
    kv(" when=", ev[3]); kv(" h=", ev[4]); kv(" v=", ev[5]);
    tm_puts("\n");
}

void tm_bad(const char *what)
{
    tm_puts("TBX BAD ");
    tm_puts(what);
    tm_puts("\n");
}

/* --------------------------------------------------------- event script */
static int parse_num(const char *t, int len, int *pos, int32_t *out)
{
    int p = *pos, neg = 0, any = 0;
    uint32_t v = 0;
    while (p < len && (t[p] == ' ' || t[p] == '\t')) p++;
    if (p < len && t[p] == '-') { neg = 1; p++; }
    if (p + 1 < len && t[p] == '0' && (t[p + 1] == 'x' || t[p + 1] == 'X')) {
        p += 2;
        while (p < len) {
            char c = t[p];
            int d = (c >= '0' && c <= '9') ? c - '0' :
                    (c >= 'a' && c <= 'f') ? c - 'a' + 10 :
                    (c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
            if (d < 0) break;
            v = v * 16u + (uint32_t)d;
            p++; any = 1;
        }
    } else {
        while (p < len && t[p] >= '0' && t[p] <= '9') {
            v = v * 10u + (uint32_t)(t[p] - '0');
            p++; any = 1;
        }
    }
    if (!any) return 0;
    *out = neg ? (int32_t)((uint32_t)0 - v) : (int32_t)v;
    *pos = p;
    return 1;
}

int tm_parse_events(const char *text, int len, int32_t (*ev)[6], int cap)
{
    int p = 0, n = 0;
    while (p < len) {
        int e = p, k;
        while (e < len && text[e] != '\n') e++;
        {
            int q = p;
            while (q < e && (text[q] == ' ' || text[q] == '\t' || text[q] == '\r')) q++;
            if (q < e && text[q] != '#') {
                if (n >= cap) return -1;
                for (k = 0; k < 6; k++)
                    if (!parse_num(text, e, &q, &ev[n][k])) return -1;
                while (q < e && (text[q] == ' ' || text[q] == '\t' || text[q] == '\r')) q++;
                if (q != e) return -1;
                n++;
            }
        }
        p = e + 1;
    }
    return n;
}
