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
