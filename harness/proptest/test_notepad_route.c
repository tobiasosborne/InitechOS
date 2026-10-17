/* R4.4 host routing oracle: the real tenant + shared TextEdit and real
 * Scrap Manager, recorded Toolbox file/drawing verbs (InitechText pattern).
 * Expectations are literal user contract / period-gui-suite Sec 4 / IM Scrap
 * behaviours. Pixel expectations belong to the independent PNG gate.
 * Compile twice: Note Pad, then SCRAPBOOK. No expected values use app state. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void mock_diagnostic(const char *s);
#define DA_AUX(s) mock_diagnostic(s)
#include "../../os/apps/notepad/notepad.c"
#include "scrap.h"
#define CONT_H 237
static long checks, fails;
#define CHECK(c, why) do { checks++; if (!(c)) { fails++; printf("RED-REASON %s: line %d\n", why, __LINE__); } } while (0)
#define MAXOPS 512
typedef struct { int kind, x, y, r, b; uint32_t fg, bg; char s[96]; } op_t; /* 0 text 1 fill */
static op_t   m_ops[MAXOPS];
static int    m_nops;
static int    m_exits, m_alerts, m_track, m_sf;          /* sf: 1 get, 2 put */
static char   m_alert_text[128], m_alert_btn[3][24];
static char   m_sf_prompt[64], m_sf_name[24];
static char  *m_sf_reply;
static char   m_title[32];
static int    m_scroll_len, m_scroll_val;
/* the shell's desk scrap: survives every launch */
static char   m_scrap[8192];
static FlairScrap desk;
static int diagnostics;
static int    m_scrap_len = -1;
static int    m_scrap_puts;
/* the volume */
typedef struct { char name[64]; char data[40000]; int len; int used; } mfile_t;
static mfile_t m_files[8];
static struct { int f; int pos; int wr; } m_h[8];

int32_t tbx_register(void) { return TBX_OK; }
int32_t tbx_content_origin(int32_t w) { (void)w; return (102 << 16) | 166; }
int32_t tbx_new_window(int32_t l, int32_t t, int32_t r, int32_t b, int32_t g)
{ (void)l; (void)t; (void)r; (void)b; (void)g; return 1; }
int32_t tbx_set_wtitle(int32_t w, const char *s) { (void)w; snprintf(m_title, sizeof m_title, "%s", s); return TBX_OK; }
int32_t tbx_draw_text(int32_t w, int32_t x, int32_t y, const char *s, uint32_t f, uint32_t b)
{
    (void)w;
    if (m_nops < MAXOPS) { op_t *o = &m_ops[m_nops++]; o->kind = 0; o->x = x; o->y = y;
        o->fg = f; o->bg = b; snprintf(o->s, sizeof o->s, "%s", s); }
    return TBX_OK;
}
int32_t tbx_fill_rect(int32_t w, int32_t l, int32_t t, int32_t r, int32_t b, uint32_t c)
{
    (void)w;
    if (m_nops < MAXOPS) { op_t *o = &m_ops[m_nops++]; o->kind = 1; o->x = l; o->y = t;
        o->r = r; o->b = b; o->fg = c; o->s[0] = 0; }
    return TBX_OK;
}
int32_t tbx_draw_cells(int32_t w, int32_t x, int32_t y, const char *s, uint32_t f, uint32_t b)
{ (void)w; (void)x; (void)y; (void)s; (void)f; (void)b; return TBX_OK; }
int32_t tbx_set_mbar(const tbx_menubar_t *b) { (void)b; return TBX_OK; }
int32_t tbx_draw_menubar(void) { return TBX_OK; }
int32_t tbx_exit(int32_t rc) { (void)rc; m_exits++; return TBX_OK; }
int32_t tbx_trap(uint32_t c, uint32_t n, const uint32_t *a) { (void)c; (void)n; (void)a; return TBX_ERR_BADCODE; }
int32_t tbx_alert(const char *t, const char *d, const char *c, const char *o)
{
    m_alerts++;
    snprintf(m_alert_text, sizeof m_alert_text, "%s", t);
    snprintf(m_alert_btn[0], 24, "%s", d ? d : "");
    snprintf(m_alert_btn[1], 24, "%s", c ? c : "");
    snprintf(m_alert_btn[2], 24, "%s", o ? o : "");
    return TBX_OK;
}
int32_t tbx_scrap_put(const void *buf, uint32_t len)
{
    if (len > sizeof m_scrap) return TBX_ERR_BADARG;
    FlairZeroScrap(&desk);
    if (FlairPutScrap(&desk, FLAIR_SCRAP_TYPE_TEXT, buf, len) != 0) return TBX_ERR_BADARG;
    memcpy(m_scrap, buf, len); m_scrap_len = (int)len; m_scrap_puts++;
    return TBX_OK;
}
int32_t tbx_scrap_get(void *buf, uint32_t bufsz)
{
    if (FlairGetScrap(&desk, FLAIR_SCRAP_TYPE_TEXT, NULL, 8192, NULL) < 0) return TBX_ERR_NOTYPE;
    if (buf == (void *)0) return m_scrap_len;
    if (bufsz < (uint32_t)m_scrap_len) return TBX_ERR_BADARG;
    FlairGetScrap(&desk, FLAIR_SCRAP_TYPE_TEXT, buf, bufsz, NULL);
    return m_scrap_len;
}
int32_t tbx_sf_getfile(char *reply) { m_sf = 1; m_sf_reply = reply; return TBX_OK; }
int32_t tbx_sf_putfile(const char *p, const char *n, char *reply)
{ m_sf = 2; m_sf_reply = reply; snprintf(m_sf_prompt, 64, "%s", p); snprintf(m_sf_name, 24, "%s", n); return TBX_OK; }
int32_t tbx_track_mouse(void) { m_track++; return TBX_OK; }
int32_t tbx_set_scroll(int32_t w, int32_t len, int32_t v)
{
    int max = len - CONT_H;
    (void)w;
    if (max < 0) max = 0;
    if (v > max) v = max;
    if (v < 0) v = 0;
    m_scroll_len = len; m_scroll_val = v;
    return v;
}
int32_t tbx_get_scroll(int32_t w) { (void)w; return m_scroll_val; }
int32_t tbx_global_to_local(int32_t w, int32_t h, int32_t v)
{ (void)w; return (int32_t)(((uint32_t)(uint16_t)(v - 102) << 16) | (uint16_t)(h - 166)); }

static mfile_t *mfind(const char *p)
{
    for (int i = 0; i < 8; i++) if (m_files[i].used && strcmp(m_files[i].name, p) == 0) return &m_files[i];
    return NULL;
}
int32_t tbx_file_create(const char *path)
{
    mfile_t *f = mfind(path);
    if (!f) for (int i = 0; i < 8 && !f; i++) if (!m_files[i].used) { f = &m_files[i]; f->used = 1; snprintf(f->name, 64, "%s", path); }
    f->len = 0;
    for (int h = 0; h < 8; h++) if (!m_h[h].f) { m_h[h].f = (int)(f - m_files) + 1; m_h[h].pos = 0; m_h[h].wr = 1; return 5 + h; }
    return TBX_ERR_BUSY;
}
int32_t tbx_file_open(const char *path, uint32_t mode)
{
    mfile_t *f = mfind(path);
    if (!f) return TBX_ERR_DOS(2);
    for (int h = 0; h < 8; h++) if (!m_h[h].f) { m_h[h].f = (int)(f - m_files) + 1; m_h[h].pos = 0; m_h[h].wr = mode != 0; return 5 + h; }
    return TBX_ERR_BUSY;
}
int32_t tbx_file_read(int32_t h, void *buf, uint32_t n)
{
    mfile_t *f = &m_files[m_h[h - 5].f - 1];
    int k = f->len - m_h[h - 5].pos;
    if (k > (int)n) k = (int)n;
    memcpy(buf, f->data + m_h[h - 5].pos, (size_t)k); m_h[h - 5].pos += k;
    return k;
}
int32_t tbx_file_write(int32_t h, const void *buf, uint32_t n)
{
    mfile_t *f = &m_files[m_h[h - 5].f - 1];
    memcpy(f->data + m_h[h - 5].pos, buf, n); m_h[h - 5].pos += (int)n;
    if (m_h[h - 5].pos > f->len) f->len = m_h[h - 5].pos;
    return (int32_t)n;
}
int32_t tbx_file_seek(int32_t h, int32_t d, uint32_t o) { (void)h; (void)d; (void)o; return 0; }
int32_t tbx_file_close(int32_t h) { m_h[h - 5].f = 0; return TBX_OK; }
int32_t tbx_file_delete(const char *p) { mfile_t *f = mfind(p); if (f) f->used = 0; return TBX_OK; }

static void mock_diagnostic(const char *s) { if (strstr(s, "DA-CORRUPT")) diagnostics++; }
static void event(int what, int msg, int mods, int x, int y)
{ tbx_event_t ev = {what, msg, mods, 0, x, y}; m_nops = 0; tbx_app_event(&ev); }
static void menu(int item) { event(TBX_EVT_MENU, TBX_MENU_WORD(130, item), 0, 0, 0); }
#ifndef SCRAPBOOK
static void type(const char *s) { for (; *s; s++) event(keyDown, (unsigned char)*s, 0, 0, 0); }
static void click_turn(int prev) { event(mouseDown, 0, 0, prev ? 170 : 455, 329); }
#endif
static int shown(const char *s)
{
    for (int i = 0; i < m_nops; i++) if (!m_ops[i].kind && !strcmp(m_ops[i].s, s)) return 1;
    return 0;
}
static void launch(void) { CHECK(tbx_app_main() == 0, "launch"); event(updateEvt, 0, 0, 0, 0); }
static int u16(const char *p) { return (unsigned char)p[0] | ((unsigned char)p[1] << 8); }
static int disk_page(const mfile_t *f, int p, const char *s)
{
    int off = 16 + p * 1026, n = (int)strlen(s);
    return f && f->len > off + 1025 && u16(f->data + off) == n && !memcmp(f->data + off + 2, s, (size_t)n);
}
int main(void)
{
    FlairScrap_init(&desk);
    /* A PICT-only scrap must never masquerade as TEXT. */
    FlairPutScrap(&desk, FLAIR_SCRAP_TYPE_PICT, "pict", 4);
    launch(); menu(5);
#ifdef SCRAPBOOK
    CHECK(shown("0 of 0"), "flavour");
    tbx_scrap_put("alpha", 5); event(activateEvt, 0, FLAIR_EVT_MOD_ACTIVE_FLAG, 0, 0); menu(5);
    CHECK(shown("alpha") && shown("1 of 1"), "append");
    tbx_scrap_put("bravo", 5); menu(5);
    CHECK(shown("bravo") && shown("2 of 2"), "paste-page");
    /* Bar line-down/up events map to whole clip pages, preserving labels. */
    m_scroll_val = 221; event(updateEvt, 0, 0, 0, 0);
    CHECK(shown("alpha") && shown("1 of 2"), "page-turn");
    menu(4); CHECK(m_scrap_len == 5 && !memcmp(m_scrap, "alpha", 5), "scrap");
    menu(3); CHECK(shown("bravo") && shown("1 of 1"), "cut");
    menu(6); CHECK(shown("0 of 0"), "clear");
    menu(5); CHECK(shown("alpha") && shown("1 of 1"), "append");
    event(TBX_EVT_GOAWAY, 0, 0, 0, 0);
    CHECK(disk_page(mfind("SCRAPBK.DAT"), 0, "alpha") && u16(mfind("SCRAPBK.DAT")->data+6)==1, "save-close");
    launch(); CHECK(shown("alpha") && shown("1 of 1"), "reload");
    /* Append must leave page zero intact. */
    tbx_scrap_put("bravo", 5); menu(5); event(TBX_EVT_GOAWAY, 0, 0, 0, 0);
    CHECK(disk_page(mfind("SCRAPBK.DAT"), 0, "alpha") && disk_page(mfind("SCRAPBK.DAT"), 1, "bravo"), "paste-page");
    mfile_t *file = mfind("SCRAPBK.DAT");
#else
    CHECK(shown("1 of 8"), "flavour");
    type("alpha"); menu(7); menu(4);
    CHECK(m_scrap_len == 5 && !memcmp(m_scrap, "alpha", 5), "scrap");
    click_turn(0); CHECK(shown("2 of 8"), "page-turn");
    CHECK(disk_page(mfind("NOTEPAD.DAT"), 0, "alpha"), "save-turn");
    type("bravo"); menu(7); menu(5);
    CHECK(shown("alpha"), "paste-page");
    click_turn(1); CHECK(shown("alpha") && shown("1 of 8"), "page-turn");
    click_turn(1); CHECK(shown("1 of 8"), "page-turn");
    /* Sweep all eight boundaries, independently expected page labels. */
    static const char *labels[] = {"1 of 8","2 of 8","3 of 8","4 of 8","5 of 8","6 of 8","7 of 8","8 of 8"};
    for (int i = 1; i < 8; i++) { click_turn(0); CHECK(shown(labels[i]), "page-turn"); }
    click_turn(0); CHECK(shown("8 of 8"), "page-turn");
    for (int i = 6; i >= 0; i--) { click_turn(1); CHECK(shown(labels[i]), "page-turn"); }
    menu(7); type("close"); event(TBX_EVT_GOAWAY, 0, 0, 0, 0);
    CHECK(disk_page(mfind("NOTEPAD.DAT"), 0, "close"), "save-close");
    launch(); CHECK(shown("close") && shown("1 of 8"), "reload");
    /* Corner hits are half-open; body/one-past corner never turns. */
    event(mouseDown, 0, 0, 458, 318); event(updateEvt,0,0,0,0); CHECK(shown("1 of 8"), "dog-ear");
    event(mouseDown, 0, 0, 459, 339); event(updateEvt,0,0,0,0); CHECK(shown("1 of 8"), "dog-ear");
    event(mouseDown, 0, 0, 458, 338); CHECK(shown("2 of 8"), "dog-ear");
    menu(7); menu(3); CHECK(m_scrap_len == 5 && !memcmp(m_scrap, "alpha", 5), "scrap");
    menu(6); event(TBX_EVT_GOAWAY, 0, 0, 0, 0);
    CHECK(disk_page(mfind("NOTEPAD.DAT"), 1, ""), "clear");
    mfile_t *file = mfind("NOTEPAD.DAT");
#endif
    CHECK(m_exits > 0, "close");
    if (file) { file->data[20] ^= 1; launch(); CHECK(diagnostics == 1, "corrupt");
        CHECK(g_te.teLength == 0, "corrupt"); }
    CHECK(m_alerts == 0, "unexpected-alert");
    /* TEXT is single-byte, not ASCII-only data: preserve tabs and an extended
     * MacRoman byte through Scrap, save and reload. Mixed LF/CRLF -> CR. */
    {
        static const char raw[] = {'A','\t',(char)0x8e,13,10,10,'B'};
        static const char want[] = {'A','\t',(char)0x8e,13,13,'B'};
        tbx_scrap_put(raw,sizeof raw); menu(5);
        CHECK(g_te.teLength==6 && !memcmp(g_te.text,want,6),"single-byte-text");
        menu(7); menu(4);
        CHECK(m_scrap_len==6 && !memcmp(m_scrap,want,6),"single-byte-text");
        event(TBX_EVT_GOAWAY,0,0,0,0); launch();
        CHECK(g_te.teLength==6 && !memcmp(g_te.text,want,6) && diagnostics==1,"single-byte-text");
    }
    printf("test_notepad_route%s: %ld checks, %ld failures\n",
#ifdef SCRAPBOOK
           "_scrapbook",
#else
           "",
#endif
           checks, fails);
    return fails != 0;
}
