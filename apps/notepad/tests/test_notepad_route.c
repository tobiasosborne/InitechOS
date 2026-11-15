/* R4.4 host routing oracle: the real tenant + shared TextEdit and real
 * Scrap Manager, recorded Toolbox file/drawing verbs (InitechText pattern).
 * Expectations are literal user contract / period-gui-suite Sec 4 / IM Scrap
 * behaviours. Pixel expectations belong to the independent PNG gate.
 * Compile twice: Note Pad, then SCRAPBOOK. No expected values use app state. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tbx.h"
/* initech-gmyxm: a read that fails after a chosen number of successful calls
 * (DOS error 5, access denied). A test shim around the gate verb, never in
 * product source; -1 passes every read through. */
static int g_read_fail_after = -1;
static int32_t shim_file_read(int32_t h, void *buf, uint32_t n)
{
    if (g_read_fail_after == 0) return TBX_ERR_DOS(5);
    if (g_read_fail_after > 0) g_read_fail_after--;
    return tbx_file_read(h, buf, n);
}
#define tbx_file_read shim_file_read
#include "../notepad.c"
#undef tbx_file_read
#include "scrap.h"
#define CONT_H 237
static long checks, fails;
#define CHECK(c, why) do { checks++; if (!(c)) { fails++; printf("FAIL %s: line %d\n", why, __LINE__); } } while (0)
#include "tbx_host.h"
#include "chrome_metrics.h"
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
static void launch(void) { restart_host_tenant(); CHECK(tbx_app_main() == 0, "launch"); CHECK(tbx_host.windows[0].t==FLAIR_CHROME_MENUBAR_H,"initial frame meets menu bar"); event(updateEvt, 0, 0, 0, 0); }
static int u16(const unsigned char *p) { return (unsigned char)p[0] | ((unsigned char)p[1] << 8); }
static int disk_page(const mfile_t *f, int p, const char *s)
{
    int off = 16 + p * 1026, n = (int)strlen(s);
    return f && f->size > (unsigned)(off + 1025) && u16(f->data + off) == n && !memcmp(f->data + off + 2, s, (size_t)n);
}
static int text_is(const char *s)
{ return g_te.teLength==(int)strlen(s) && !memcmp(g_te.text,s,strlen(s)); }
static void command_sweep(void)
{
    /* Exact reviewed native surface. Shortcuts are dispatched by the shell;
     * these are independent literal spellings/keys, including disabled rows. */
    const char *names[]={"Undo","-","Cut","Copy","Paste","Clear"
#ifndef SCRAPBOOK
        ,"Select All"
#endif
    };
    const char keys[]={'Z',0,'X','C','V',0,'A'};
    CHECK(tbx_host.bar->n_menus==2 && tbx_host.bar->menus[0].n_items==2,
          "sweep File Close/Quit shape");
    for(unsigned i=0;i<2;i++) {
        const tbx_menuinfo_t *mi=&tbx_host.bar->menus[i];
        CHECK(mi->menu_id==129+(int)i && !strcmp(mi->title,i?"Edit":"File"),
              "sweep menu dispatch identity");
        const tbx_menuitem_t *it=&tbx_host.bar->menus[0].items[i];
        CHECK(it->enabled && !it->is_divider && it->cmd_char==(i?'Q':'W') &&
              !strcmp(it->text,i?"Quit":"Close"),"sweep exact File row/key");
    }
    const tbx_menuinfo_t *m=&tbx_host.bar->menus[1];
    CHECK(m->n_items==sizeof names/sizeof names[0],"sweep exact Edit count");
    for(unsigned i=0;i<m->n_items;i++)
        CHECK(i<sizeof names/sizeof names[0] && !strcmp(m->items[i].text,names[i]) &&
              m->items[i].cmd_char==keys[i],"sweep exact Edit row/key");
    CHECK(!m->items[0].enabled,"Undo initially gray");
    CHECK(!edit_items[2].enabled && !edit_items[3].enabled &&
          !edit_items[4].enabled && !edit_items[5].enabled,"empty context grays only unavailable edits");
#ifdef SCRAPBOOK
    tbx_scrap_put("alpha",5);menu(5);
    CHECK(edit_items[0].enabled && edit_items[2].enabled && edit_items[3].enabled &&
          edit_items[4].enabled && edit_items[5].enabled,"all Scrapbook commands have live context");
    CHECK(m->items[0].enabled,"append enables Undo");menu(1);
    CHECK(shown("0 of 0") && text_is(""),"Undo append to empty");menu(1);
    CHECK(text_is("alpha"),"one-level Undo toggle");
    tbx_scrap_put("bravo",5);menu(5);tbx_scrap_put("charlie",7);menu(5);
    event(keyDown,0x4b00,0,0,0);CHECK(text_is("bravo"),"previous clip key");
    menu(3);CHECK(text_is("charlie") && g_count==2,"Cut removes middle clip");
    menu(1);CHECK(text_is("bravo") && g_count==3 && g_page==1,"Undo restores clip/order/index");
    CHECK(m_scrap_len==5 && !memcmp(m_scrap,"bravo",5),"Undo Cut preserves scrap");
    menu(6);menu(1);CHECK(text_is("bravo") && g_count==3,"Undo Clear");
    event(keyDown,0x4d00,0,0,0);CHECK(text_is("charlie"),"next clip key");
    event(keyDown,0x4900,0,0,0);CHECK(text_is("bravo"),"previous page key");
    event(keyDown,0x5100,0,0,0);CHECK(text_is("charlie"),"next page key");
    menu(4);CHECK(m_scrap_len==7 && !memcmp(m_scrap,"charlie",7),"Copy whole clip");
    event(TBX_EVT_MENU,TBX_MENU_WORD(129,2),0,0,0);
    CHECK(disk_page(mfind(DA_PATH),0,"alpha") && disk_page(mfind(DA_PATH),1,"bravo") &&
          disk_page(mfind(DA_PATH),2,"charlie"),"Undo store independently read after Quit");
    CHECK(m_exits==1,"File Quit performs save/exit");
    launch();CHECK(!edit_items[0].enabled && text_is("charlie"),"reload clears history");
    /* Capacity rejection preserves the last valid undo, even when full. */
    for(int i=3;i<32;i++) {tbx_scrap_put("tail",4);menu(5);}
    menu(5);CHECK(g_count==32,"full append rejected");menu(1);
    CHECK(g_count==31,"failed append keeps previous history");
    m_alerts=0;
#else
    type("alpha");event(updateEvt,0,0,0,0);menu(1);
    CHECK(text_is("") && !g_dirty && g_te.selStart==0 && g_te.selEnd==0,"Undo whole typing run across repaint");
    menu(1);CHECK(text_is("alpha") && g_dirty,"Undo toggle restores typing/dirty state");
    CHECK(edit_items[0].enabled,"typing enables visible Undo");
    event(keyDown,0x4b00,0,0,0);type("X");menu(1);
    CHECK(text_is("alpha") && g_te.selStart==4,"arrow breaks typing run; Undo selection");
    menu(7);menu(3);CHECK(text_is(""),"Cut whole selection");menu(1);
    CHECK(text_is("alpha") && g_te.selStart==0 && g_te.selEnd==5,"Undo Cut restores selection");
    CHECK(edit_items[0].enabled && edit_items[2].enabled && edit_items[3].enabled &&
          edit_items[4].enabled && edit_items[5].enabled && edit_items[6].enabled,
          "all Note Pad commands have live context");
    CHECK(m_scrap_len==5 && !memcmp(m_scrap,"alpha",5),"Undo Cut keeps shared scrap");
    menu(6);menu(1);CHECK(text_is("alpha"),"Undo Clear");
    tbx_scrap_put("bravo",5);menu(5);menu(1);
    CHECK(text_is("alpha") && m_scrap_len==5 && !memcmp(m_scrap,"bravo",5),"Undo Paste preserves scrap");
    event(keyDown,8,0,0,0);menu(1);CHECK(text_is("alpha"),"Undo deletion");
    click_turn(0);CHECK(!edit_items[0].enabled,"page turn clears page-local history");
    click_turn(1);CHECK(text_is("alpha"),"page identity and bytes restored");
    event(TBX_EVT_MENU,TBX_MENU_WORD(129,2),0,0,0);
    CHECK(disk_page(mfind(DA_PATH),0,"alpha"),"Undo saved independently on Quit");
    CHECK(m_exits==1,"File Quit performs save/exit");
#endif
    /* Close, its shortcut and the close box share saving, but each event
     * route is exercised. Reported write failures never discard the store. */
    launch();
    const mfile_t *old=mfind(DA_PATH);static unsigned char keep[DA_FILE_BYTES];
    CHECK(old && old->size==DA_FILE_BYTES,"saved store fixture");
    if(old) memcpy(keep,old->data,DA_FILE_BYTES);
    tbx_scrap_put("later",5);menu(5);
    tbx_host.fail_write=1;event(TBX_EVT_MENU,TBX_MENU_WORD(129,1),0,0,0);
    CHECK(m_exits==0 && g_dirty && m_alerts>0,"failed Close leaves editable dirty store");
    old=mfind(DA_PATH);
    CHECK(old && !memcmp(old->data,keep,DA_FILE_BYTES),"failed Close keeps independently saved bytes");
    tbx_host.fail_write=0;menu(1);
    event(TBX_EVT_MENU,TBX_MENU_WORD(129,1),0,0,0);
    CHECK(m_exits==1,"File Close works after failed save and Undo");
#ifndef SCRAPBOOK
    launch();menu(7);tbx_scrap_put("ab\rcd",5);menu(5);
    event(keyDown,0x4800,0,0,0);CHECK(g_te.selStart==2,"Up moves caret to prior line");
    event(keyDown,0x5000,0,0,0);CHECK(g_te.selStart==5,"Down returns caret");
    event(keyDown,0x4b00,0,0,0);CHECK(g_te.selStart==4,"Left moves caret");
    event(keyDown,0x4d00,0,0,0);CHECK(g_te.selStart==5,"Right moves caret");
    event(autoKey,'X',0,0,0);CHECK(text_is("ab\rcdX"),"autoKey types through same editor");
    menu(1);CHECK(text_is("ab\rcd"),"autoKey participates in Undo");
    static char full[1024];memset(full,'k',sizeof full);
    menu(7);tbx_scrap_put(full,sizeof full);menu(5);
    CHECK(g_te.teLength==1024,"capacity fixture is accepted");
    type("X");CHECK(g_te.teLength==1024,"capacity refuses extra typing");
    menu(1);CHECK(text_is("ab\rcd"),"refused edit keeps prior Undo");
#endif
}
int main(void)
{
    /* sdk/host has TEXT only: an absent TEXT scrap models PICT-only get. */
    tbx_host_reset(); tbx_host.origin_h=166; tbx_host.origin_v=102; tbx_host.ch=237;
    launch(); menu(5);
    command_sweep();
    tbx_host_reset();tbx_host.origin_h=166;tbx_host.origin_v=102;tbx_host.ch=237;
    launch();menu(5);
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
    CHECK(m_alerts == 0, "unexpected-alert");
    if (file) { file->data[20] ^= 1; launch(); CHECK(g_te.teLength == 0, "corrupt"); }
    /* initech-mcep (audit 2026-10-23-das L008): closing over the damaged store
     * never writes it; the bytes are the damaged bytes. initech-gmyxm: the
     * warning comes at launch (the leg at the end of this file), so Close
     * ends at once. The single-byte leg below then starts from a first
     * launch (no store), as it must: it no longer rides on the overwrite. */
    if (file) {
        static char keep[DA_FILE_BYTES + 1]; int kl = file->size, ex = (int)m_exits;
        memcpy(keep, file->data, (size_t)kl);
        CHECK(m_alerts == 1 && (int)m_exits == ex, "keep-damaged");
        event(TBX_EVT_ALERT, TBX_ALERT_DEFAULT, 0, 0, 0);
        event(TBX_EVT_GOAWAY, 0, 0, 0, 0);
        CHECK((int)m_exits == ex + 1 && m_alerts == 1, "keep-damaged");
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
        int alerts=(int)m_alerts; event(TBX_EVT_GOAWAY,0,0,0,0);
        CHECK((int)m_alerts==alerts,"single-byte-text saved without a damaged-store notice");
        launch();
        CHECK(g_te.teLength==6 && !memcmp(g_te.text,want,6),"single-byte-text");
    }
    /* initech-l3ahn: TBX_FILE_READ returns "bytes | err" (toolbox_gate.h),
     * the INT 21h AH=3Fh contract: AX is the count actually read, fewer than
     * asked is legal and only 0 is end of file. The pinned conformance
     * fixtures (cases.tsv N01/S01: every page full, the last one current) are
     * decoded here by their own byte offsets, never by the loader under test;
     * N02/S02 (one byte short) stay damaged however the bytes arrive. */
    {
#ifdef SCRAPBOOK
        static const char *good = DA_FIXTURES "S01.DAT", *cut = DA_FIXTURES "S02.DAT", *label = "32 of 32";
        enum { want_size = 32848 };
#else
        static const char *good = DA_FIXTURES "N01.DAT", *cut = DA_FIXTURES "N02.DAT", *label = "8 of 8";
        enum { want_size = 8224 };
#endif
        static unsigned char fx[want_size + 64], fc[want_size + 64];
        static const uint32_t limits[] = {4, 1000, 4096, 8223};
        FILE *f = fopen(good, "rb"); size_t n = f ? fread(fx, 1, sizeof fx, f) : 0; if (f) fclose(f);
        f = fopen(cut, "rb"); size_t nc = f ? fread(fc, 1, sizeof fc, f) : 0; if (f) fclose(f);
        CHECK(n == want_size && nc == want_size - 1, "short-read fixtures");
        const unsigned char *rec = fx + 16 + u16(fx + 8) * 1026;
        int len = u16(rec);
        CHECK(len == 1024, "short-read fixture page is full");
        for (unsigned i = 0; i < sizeof limits / sizeof limits[0]; i++) {
            tbx_host_reset(); tbx_host.origin_h=166; tbx_host.origin_v=102; tbx_host.ch=237;
            CHECK(tbx_host_put_file(DA_PATH, fx, (uint32_t)n), "short-read store");
            tbx_host.read_limit = limits[i];
            launch();
            CHECK(g_te.teLength == len && !memcmp(g_te.text, rec + 2, (size_t)len), "short read shows the stored page");
            CHECK(shown(label), "short read keeps the stored page count");
            int ex = (int)m_exits;
            event(TBX_EVT_GOAWAY, 0, 0, 0, 0);
            mfile_t *after = mfind(DA_PATH);
            CHECK(m_alerts == 0 && (int)m_exits == ex + 1, "short read is not a damaged store");
            CHECK(after && after->size == n && !memcmp(after->data, fx, n), "short read: unedited close keeps every byte");
            tbx_host_reset(); tbx_host.origin_h=166; tbx_host.origin_v=102; tbx_host.ch=237;
            CHECK(tbx_host_put_file(DA_PATH, fc, (uint32_t)nc), "short-read cut store");
            tbx_host.read_limit = limits[i];
            launch();
            CHECK(g_te.teLength == 0, "cut store under short reads stays damaged");
            event(TBX_EVT_GOAWAY, 0, 0, 0, 0);
            after = mfind(DA_PATH);
            CHECK(m_alerts == 1 && after && after->size == nc && !memcmp(after->data, fc, nc), "cut store kept as it is");
        }
        tbx_host.read_limit = 0;
    }
    /* initech-yhqb: every refusal is SHOWN. The gate refuses an alert line
     * over 47 bytes or 285 Chicago 12 pixels (toolbox_gate.h Sec 11) and
     * sdk/host applies the same validator, so a notice that does not fit
     * never reaches the screen. Refusals leave the store as it was. */
    {
        static char big[DA_TEXT_MAX + 1];
        memset(big, 'k', sizeof big);
        tbx_host_reset(); tbx_host.origin_h=166; tbx_host.origin_v=102; tbx_host.ch=237;
        launch();
#ifdef SCRAPBOOK
        for (int i = 0; i < DA_SLOTS; i++) { tbx_scrap_put("clip", 4); menu(5); }
        int before = (int)m_alerts;
        menu(5);
        CHECK((int)m_alerts == before + 1 && strstr(m_alert_text, "full"), "full Scrapbook notice is shown");
        event(TBX_EVT_ALERT, TBX_ALERT_DEFAULT, 0, 0, 0);
        menu(6);
        tbx_scrap_put(big, sizeof big); before = (int)m_alerts; menu(5);
        CHECK((int)m_alerts == before + 1 && strstr(m_alert_text, "too large"), "oversized clip notice is shown");
        event(TBX_EVT_ALERT, TBX_ALERT_DEFAULT, 0, 0, 0);
        tbx_scrap_put("a\001b", 3); before = (int)m_alerts; menu(5);
        CHECK((int)m_alerts == before + 1, "non-TEXT clip notice is shown");
        event(TBX_EVT_ALERT, TBX_ALERT_DEFAULT, 0, 0, 0);
        event(TBX_EVT_GOAWAY, 0, 0, 0, 0);
        CHECK(mfind(DA_PATH) && u16(mfind(DA_PATH)->data + 6) == DA_SLOTS - 1, "refusals leave the 31 clips");
#else
        tbx_scrap_put(big, DA_TEXT_MAX); menu(5);
        int before = (int)m_alerts;
        type("X");
        CHECK((int)m_alerts == before + 1 && strstr(m_alert_text, "full"), "full page notice is shown");
        event(TBX_EVT_ALERT, TBX_ALERT_DEFAULT, 0, 0, 0);
        menu(7); menu(6);
        tbx_scrap_put("a\001b", 3); before = (int)m_alerts; menu(5);
        CHECK((int)m_alerts == before + 1, "non-TEXT clip notice is shown");
        event(TBX_EVT_ALERT, TBX_ALERT_DEFAULT, 0, 0, 0);
        event(TBX_EVT_GOAWAY, 0, 0, 0, 0);
        CHECK(disk_page(mfind(DA_PATH), 0, ""), "refusals leave the page as it was");
#endif
    }
    /* initech-gmyxm (GM4: no path that loses a user's data). A store that
     * exists but cannot be read is kept as it is and never written, so this
     * session cannot save. Macintosh System Software User's Guide v6.0 p. 87
     * (initech-manuals-ref/macos): "Locking a document also prevents saving
     * any changes to it. You can, however, open and print locked documents."
     * Expected: the failure is told before any edit can be made, as soon as
     * the window first appears; the pages take no new work while the store
     * cannot be saved (each refused edit is heard, SysBeep, the
     * Macintosh's answer to an action it will not take); Cut, Paste and Clear
     * are gray; closing loses nothing because nothing was taken, and the old
     * bytes are unchanged. Three ways to fail: the first read, a read after
     * positive partial reads, and a store one byte short (N02/S02). */
    {
#ifdef SCRAPBOOK
        static const char *good = DA_FIXTURES "S01.DAT", *cut = DA_FIXTURES "S02.DAT";
        enum { want_size = 32848 };
#else
        static const char *good = DA_FIXTURES "N01.DAT", *cut = DA_FIXTURES "N02.DAT";
        enum { want_size = 8224 };
#endif
        static unsigned char fx[want_size + 64], fc[want_size + 64];
        FILE *f = fopen(good, "rb"); size_t n = f ? fread(fx, 1, sizeof fx, f) : 0; if (f) fclose(f);
        f = fopen(cut, "rb"); size_t nc = f ? fread(fc, 1, sizeof fc, f) : 0; if (f) fclose(f);
        CHECK(n == want_size && nc == want_size - 1, "failed-read fixtures");
        const unsigned char *rec = fx + 16 + u16(fx + 8) * 1026;
        static const struct { const char *why; int fail_after; uint32_t limit; int truncated; } modes[] = {
            { "failed-read: error on the first read", 0, 0, 0 },
            { "failed-read: error after positive partial reads", 2, 8, 0 },
            { "failed-read: store one byte short", -1, 0, 1 },
        };
        for (unsigned m = 0; m < sizeof modes / sizeof modes[0]; m++) {
            const unsigned char *src = modes[m].truncated ? fc : fx;
            size_t sn = modes[m].truncated ? nc : n;
            tbx_host_reset(); tbx_host.origin_h=166; tbx_host.origin_v=102; tbx_host.ch=237;
            CHECK(tbx_host_put_file(DA_PATH, src, (uint32_t)sn), modes[m].why);
            tbx_host.read_limit = modes[m].limit;
            g_read_fail_after = modes[m].fail_after;
            launch();
            g_read_fail_after = -1; tbx_host.read_limit = 0;   /* the failure was transient */
            printf("  %s:\n", modes[m].why);
            CHECK(m_alerts == 1 && strstr(m_alert_text, "could not be read") &&
                  !strcmp(m_alert_btn[0], "OK"), "  told at launch, before any edit");
            event(TBX_EVT_ALERT, TBX_ALERT_DEFAULT, 0, 0, 0);
            unsigned beeps = tbx_host.beeps;
            tbx_scrap_put("new note", 8);
#ifdef SCRAPBOOK
            menu(5);
            CHECK(g_count == 0 && g_te.teLength == 0 && !g_dirty, "  Paste takes no clip");
            CHECK(tbx_host.beeps > beeps, "  the refused Paste is heard");
#else
            type("new note");
            CHECK(g_te.teLength == 0 && !g_dirty, "  typing takes no text");
            CHECK(tbx_host.beeps >= beeps + 8, "  each refused key is heard");
            menu(5);
            CHECK(g_te.teLength == 0 && !g_dirty, "  Paste takes no text");
#endif
            CHECK(!edit_items[2].enabled && !edit_items[4].enabled && !edit_items[5].enabled &&
                  !edit_items[0].enabled, "  Cut, Paste, Clear and Undo are gray");
            int ex = (int)m_exits, al = (int)m_alerts;
            event(TBX_EVT_GOAWAY, 0, 0, 0, 0);
            mfile_t *after = mfind(DA_PATH);
            CHECK((int)m_exits == ex + 1 && (int)m_alerts == al, "  Close ends at once: nothing was taken");
            CHECK(after && after->size == sn && !memcmp(after->data, src, sn), "  the store is kept as it was");
            if (!modes[m].truncated) {
                launch();
                CHECK(m_alerts == 0 && g_te.teLength == u16(rec) && !memcmp(g_te.text, rec + 2, (size_t)u16(rec)),
                      "  read again, the stored page is back");
                event(TBX_EVT_GOAWAY, 0, 0, 0, 0);
            }
        }
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
