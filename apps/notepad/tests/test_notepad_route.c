/* R4.4 host routing oracle: the real tenant + shared TextEdit and real
 * Scrap Manager, recorded Toolbox file/drawing verbs (InitechText pattern).
 * Expectations are literal user contract / period-gui-suite Sec 4 / IM Scrap
 * behaviours. Pixel expectations belong to the independent PNG gate.
 * Compile twice: Note Pad, then SCRAPBOOK. No expected values use app state. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void mock_diagnostic(const char *s);
#define DA_AUX(s) mock_diagnostic(s)
#include "../notepad.c"
#include "scrap.h"
#define CONT_H 237
static long checks, fails;
#define CHECK(c, why) do { checks++; if (!(c)) { fails++; printf("RED-REASON %s: line %d\n", why, __LINE__); } } while (0)
#include "tbx_host.h"
#include "tenant_lifecycle.h"
typedef tbx_host_file_t mfile_t;
typedef tbx_host_op_t op_t;
#define m_ops tbx_host.ops
#define m_nops tbx_host.nops
#define m_exits tbx_host.exit_calls
#define m_alerts tbx_host.alerts
#define m_track tbx_host.track
#define m_sf tbx_host.sf
#define m_alert_text tbx_host.alert_text
#define m_alert_btn tbx_host.alert_btn
#define m_sf_prompt tbx_host.sf_prompt
#define m_sf_name tbx_host.sf_name
#define m_sf_reply tbx_host.sf_reply
#define m_title tbx_host.windows[0].title
#define m_scroll_len tbx_host.windows[0].scroll_len
#define m_scrap tbx_host.scrap
#define m_scrap_len tbx_host.scrap_len
#define m_scrap_puts tbx_host.scrap_puts
static mfile_t *mfind(const char *p) { return (mfile_t *)tbx_host_file(p); }
static int diagnostics;

static void mock_diagnostic(const char *s) { if (strstr(s, "DA-CORRUPT")) diagnostics++; }
static void event(int what, int msg, int mods, int x, int y)
{ tbx_event_t ev = {what, msg, mods, 0, x, y}; m_nops = 0;
  if(!tbx_host_push_event(&ev))exit(2);
  (void)tbx_host_dispatch(tbx_app_event); }
static void menu(int item) { event(TBX_EVT_MENU, TBX_MENU_WORD(130, item), 0, 0, 0); }
#ifndef SCRAPBOOK
static void type(const char *s) { for (; *s; s++) event(keyDown, (unsigned char)*s, 0, 0, 0); }
static void click_turn(int prev) { event(mouseDown, 0, 0, prev ? 170 : 455, 329); }
#endif
static int shown(const char *s)
{
    for (unsigned i = 0; i < m_nops; i++) if (!strcmp(m_ops[i].name,"tbx_draw_text") && !strcmp(m_ops[i].text, s)) return 1;
    return 0;
}
static void launch(void) { restart_host_tenant(); CHECK(tbx_app_main() == 0, "launch"); event(updateEvt, 0, 0, 0, 0); }
static int u16(const unsigned char *p) { return (unsigned char)p[0] | ((unsigned char)p[1] << 8); }
static int disk_page(const mfile_t *f, int p, const char *s)
{
    int off = 16 + p * 1026, n = (int)strlen(s);
    return f && f->size > (unsigned)(off + 1025) && u16(f->data + off) == n && !memcmp(f->data + off + 2, s, (size_t)n);
}
int main(void)
{
    /* sdk/host has TEXT only: an absent TEXT scrap models PICT-only get. */
    tbx_host_reset(); tbx_host.origin_h=166; tbx_host.origin_v=102; tbx_host.ch=237;
    launch(); menu(5);
#ifdef SCRAPBOOK
    CHECK(shown("0 of 0"), "flavour");
    tbx_scrap_put("alpha", 5); event(activateEvt, 0, FLAIR_EVT_MOD_ACTIVE_FLAG, 0, 0); menu(5);
    CHECK(shown("alpha") && shown("1 of 1"), "append");
    tbx_scrap_put("bravo", 5); menu(5);
    CHECK(shown("bravo") && shown("2 of 2"), "paste-page");
    /* Bar line-down/up events map to whole clip pages, preserving labels. */
    /* The shell's scroll value goes through the same public setter as a bar
     * update; this works with both per-window and shared-scroll recording. */
    (void)tbx_set_scroll(1,2*CONT_H,221); event(updateEvt, 0, 0, 0, 0);
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
    /* initech-mcep (audit 2026-10-23-das L008): closing over the damaged store
     * never writes it -- it warns once and ends on the answer; the bytes are
     * the damaged bytes. The single-byte leg below then starts from a first
     * launch (no store), as it must: it no longer rides on the overwrite. */
    if (file) {
        static char keep[DA_FILE_BYTES + 1]; int kl = file->size, ex = (int)m_exits;
        memcpy(keep, file->data, (size_t)kl);
        event(TBX_EVT_GOAWAY, 0, 0, 0, 0);
        CHECK(m_alerts == 1 && (int)m_exits == ex, "keep-damaged");
        event(TBX_EVT_ALERT, TBX_ALERT_DEFAULT, 0, 0, 0);
        CHECK((int)m_exits == ex + 1, "keep-damaged");
        CHECK(file->used && file->size == (unsigned)kl && !memcmp(file->data, keep, (size_t)kl), "keep-damaged");
        file->used = 0;
        launch();
    }
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
