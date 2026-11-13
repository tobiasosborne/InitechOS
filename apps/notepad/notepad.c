/* NOTEPAD.EXE / SCRAPBK.EXE -- R4.4 C disk desk accessories, tdnl.18.
 * Ref: docs/research/period-gui-suite.md Sec 4; spec/notepad/store.{json,h};
 * shared InitechText pattern (TextEdit, desk Scrap, DOS-backed file verbs).
 * documentProc is the default NEWWINDOW WDEF, TBX_GOAWAY_ASK saves before
 * close. Apple launch is entirely the existing table-driven launch verb.
 *
 * Native Note Pad/Scrapbook captures are ABSENT. This is the explicitly
 * authorized composition: graded Platinum document chrome/controls + the
 * SimpleText (5,4) inset/selection/caret, Geneva9 (PRD Sec 6.4). The corner
 * fold and n-of-m placement are functional adaptations, NOT native pixel
 * fidelity claims. No animation is claimed. Scrapbook stores TEXT only;
 * PICT and sound are not supported. Limits and on-disk bytes are locked.
 * No kernel additions, private clipboard, editor, or file implementation.
 * ASCII-only, deterministic, freestanding C (Laws 1-3, Rules 8/11/12).
 */
#include "tbx.h"
#include "tbx_save.h"   /* initech-mcep: the atomic document save */
#include "textedit.h"
#include "text.h"
#include "notepad/store.h"

#ifdef SCRAPBOOK
const char tbx_app_name[] = "SCRAPBK";
#define DA_TITLE "Scrapbook"
#else
const char tbx_app_name[] = "NOTEPAD";
#define DA_TITLE "Note Pad"
#endif
/* DOS AUX output, AH=04h (os/milton/int21.c do_aux_output), the existing
 * serial device seam. Host routing records it; no host env or paths. */
#ifndef DA_AUX
static void da_aux(const char *s)
{
    for (; *s; s++) {
        uint32_t ax = 0x0400, dx = (unsigned char)*s;
        __asm__ volatile("int $0x21" : "+a"(ax) : "d"(dx) : "memory", "cc");
    }
}
#define DA_AUX(s) da_aux(s)
#endif
static tbx_menuitem_t file_items[] TBX_IN_IMAGE = {
    {"Close",0,'W',0,1,0,{0,0,0}},
    {"Quit",0,'Q',0,1,0,{0,0,0}}
};
static tbx_menuitem_t edit_items[] TBX_IN_IMAGE = {
    {"Undo",0,'Z',0,0,0,{0,0,0}}, {"-",0,0,0,0,1,{0,0,0}},
    {"Cut",0,'X',0,0,0,{0,0,0}}, {"Copy",0,'C',0,0,0,{0,0,0}},
    {"Paste",0,'V',0,0,0,{0,0,0}}, {"Clear",0,0,0,0,0,{0,0,0}},
    {"Select All",0,'A',0,1,0,{0,0,0}}
};
static const tbx_menuinfo_t menus[] = {
    {129,0,"File",file_items,2,0}, {130,0,"Edit",edit_items,7,0}
};
static const tbx_menubar_t mbar = {menus,2,1,0};
static FlairTE g_te;
static char g_pages[DA_SLOTS][DA_TEXT_MAX];
static uint16_t g_len[DA_SLOTS];
static unsigned char g_disk[DA_FILE_BYTES + 1];
static char g_clip[8192];
static char g_run[128] TBX_IN_IMAGE, g_label[24] TBX_IN_IMAGE;
static int32_t g_win;
static int g_page, g_count, g_scroll, g_anchor, g_active;

static void bytes(void *dst, const void *src, int n)
{ unsigned char *d=dst; const unsigned char *s=src; for(int i=0;i<n;i++) d[i]=s[i]; }
static int word(int off) { return g_disk[off] | (g_disk[off+1]<<8); }
static void putword(int off, int v) { g_disk[off]=(unsigned char)v; g_disk[off+1]=(unsigned char)(v>>8); }
static uint32_t hash(void)
{
    uint32_t h=2166136261u;
    for(int i=0;i<DA_FILE_BYTES;i++) h=(h ^ (uint32_t)((i>=12 && i<16)?0:g_disk[i]))*16777619u;
    return h;
}
static void empty(void)
{
    for(int p=0;p<DA_SLOTS;p++) { g_len[p]=0; for(int i=0;i<DA_TEXT_MAX;i++) g_pages[p][i]=0; }
    g_page=0;
#ifdef SCRAPBOOK
    g_count=0;
#else
    g_count=8;
#endif
}
static int valid(void)
{
    const char *magic=DA_MAGIC;
    uint32_t h=0;
    for(int i=0;i<4;i++) if(g_disk[i]!=(unsigned char)magic[i]) return 0;
    if(word(4)!=1 || word(10)!=DA_SLOTS || word(6)>DA_SLOTS ||
       (word(6)==0 ? word(8)!=0 : word(8)>=word(6))) return 0;
#ifndef SCRAPBOOK
    if(word(6)!=8) return 0;
#endif
    for(int i=0;i<4;i++) h|=(uint32_t)g_disk[12+i]<<(i*8);
#ifndef DA_MUT_ACCEPT_CORRUPT
    if(h!=hash()) return 0;
#else
    (void)h;
#endif
    for(int p=0;p<DA_SLOTS;p++) {
        int off=16+p*DA_RECORD_BYTES, n=word(off);
        if(n>DA_TEXT_MAX || (p>=word(6) && n)) return 0;
        for(int i=0;i<DA_TEXT_MAX;i++) {
            int c=g_disk[off+2+i];
            if(i>=n ? c!=0 : !(c==13 || c==9 || c>=32)) return 0;
        }
        if(!FlairTECanReplace(&g_te,0,g_te.teLength,(const char *)g_disk+off+2,(int16_t)n)) return 0;
    }
    return 1;
}
/* initech-mcep (audit 2026-10-23-das L008): a store that exists but cannot
 * be read back -- a bad checksum, a wrong length, a read error -- is KEPT:
 * this session runs on empty pages and never writes over it (save() below),
 * and closing says so. Only a missing store (first launch) is created. */
static int g_keep, g_closing;
static void load(void)
{
    int32_t fd=tbx_file_open(DA_PATH,TBX_FILE_READ_ONLY), n;
    g_keep=0; g_closing=0;                     /* each launch judges afresh */
    if(fd==TBX_ERR_DOS(2)) return;             /* first launch */
    if(fd<0) { g_keep=1; DA_AUX("DA-READ-ERROR " DA_PATH "\r\n"); return; }
    n=tbx_file_read(fd,g_disk,sizeof g_disk);
    if(tbx_file_close(fd)<0 || n!=DA_FILE_BYTES || !valid()) {
#ifndef DA_MUT_OVERWRITE_DAMAGED
        g_keep=1;
#endif
        DA_AUX("DA-CORRUPT " DA_PATH " empty pages\r\n");
        return;                              /* defaults remain intact */
    }
    g_count=word(6); g_page=word(8);
    for(int p=0;p<DA_SLOTS;p++) {
        int off=16+p*DA_RECORD_BYTES;
        g_len[p]=(uint16_t)word(off);
        bytes(g_pages[p],g_disk+off+2,g_len[p]);
    }
    DA_AUX("DA-LOADED " DA_PATH "\r\n");
}
static void notice(const char *s) { (void)tbx_alert(s,"OK",0,0); }
static void stash(void)
{
    if(!g_count) return;
    g_len[g_page]=(uint16_t)g_te.teLength;
    bytes(g_pages[g_page],g_te.text,g_te.teLength);
}
static int save(void)
{
    uint32_t h; const char *magic=DA_MAGIC;
    if(g_keep) { DA_AUX("DA-KEPT " DA_PATH " damaged, not replaced\r\n"); return 1; }
    for(int i=0;i<DA_FILE_BYTES;i++) g_disk[i]=0;
    for(int i=0;i<4;i++) g_disk[i]=(unsigned char)magic[i];
    putword(4,1); putword(6,g_count); putword(8,g_page); putword(10,DA_SLOTS);
    for(int p=0;p<g_count;p++) {
        int off=16+p*DA_RECORD_BYTES;
        putword(off,g_len[p]); bytes(g_disk+off+2,g_pages[p],g_len[p]);
    }
    h=hash(); for(int i=0;i<4;i++) g_disk[12+i]=(unsigned char)(h>>(8*i));
    /* initech-mcep: the atomic save -- a full disk keeps the old pages. */
    if(tbx_file_save(DA_PATH,g_disk,DA_FILE_BYTES)!=DA_FILE_BYTES) { notice("The pages could not be saved."); return 0; }
    DA_AUX("DA-SAVED " DA_PATH "\r\n");
    return 1;
}
static void read_page(void)
{
    FlairTESetText(&g_te,g_pages[g_page],g_count?(int16_t)g_len[g_page]:0);
    FlairTESetSelect(&g_te,0,0); g_scroll=0;
}
static void menus_sync(void)
{
#ifdef SCRAPBOOK
    int range=g_count>0;
    edit_items[6].enabled=0;
#else
    int range=g_te.selStart<g_te.selEnd;
#endif
    edit_items[2].enabled=(uint8_t)range; edit_items[3].enabled=(uint8_t)range;
    edit_items[5].enabled=(uint8_t)range;
    edit_items[4].enabled=(uint8_t)(tbx_scrap_get(0,0)>0);
}
static int decimal(int at,int n)
{
    if(n>=10) g_label[at++]=(char)('0'+n/10);
    g_label[at++]=(char)('0'+n%10); return at;
}
static int draw_run(int a,int b,int x,int y,int hil)
{
    while(a<b) {
        int n=0,w=0;
        while(a+n<b && n<120) { g_run[n]=g_te.text[a+n]; w+=text_face_advance(FONT_GENEVA9,0,(unsigned char)g_run[n]); n++; }
        g_run[n]=0;
        (void)tbx_draw_text(g_win,x,y,g_run,(hil?TBX_COLOR_WHITE:TBX_COLOR_BLACK)|TBX_TEXT_GENEVA9,
                            hil?TBX_COLOR_BLACK:TBX_COLOR_WHITE);
        x+=w; a+=n;
    }
    return x;
}
static void bar(void)
{
#ifdef SCRAPBOOK
    (void)tbx_set_scroll(g_win,(g_count?g_count:1)*DA_VIEW_H,g_page*DA_VIEW_H);
#else
    int doc=8+g_te.nLines*FlairTELineHeight(&g_te)+28, s=g_scroll;
    int16_t h,v;
    FlairTEGetPoint(&g_te,g_te.selEnd,&h,&v);
    if(v<g_scroll) g_scroll=v;
    if(v+15>g_scroll+DA_TEXT_H) g_scroll=v+15-DA_TEXT_H;
    if(g_scroll<0) g_scroll=0;
    s=tbx_set_scroll(g_win,doc,g_scroll); if(s>=0) g_scroll=s;
#endif
}
static void paint(void)
{
    int h=FlairTELineHeight(&g_te), s=g_te.selStart,e=g_te.selEnd;
    (void)tbx_fill_rect(g_win,0,0,DA_VIEW_W,DA_VIEW_H,TBX_COLOR_WHITE);
    for(int l=0;l<g_te.nLines;l++) {
        int y=4+l*h-g_scroll,ls=g_te.lineStarts[l];
        int ns=l+1<g_te.nLines?g_te.lineStarts[l+1]:g_te.teLength;
        int le=(ns>ls && g_te.text[ns-1]=='\r')?ns-1:ns;
        int a=(s>ls)?(s<le?s:le):ls, b=(e>ls)?(e<le?e:le):ls,x=5;
        if(y<0 || y+h>DA_TEXT_H) continue;
#ifdef SCRAPBOOK
        a=b=le;
#endif
        x=draw_run(ls,a,x,y,0); x=draw_run(a,b,x,y,1); x=draw_run(b,le,x,y,0);
#ifndef SCRAPBOOK
        if(s<e && s<=le && e>le) (void)tbx_fill_rect(g_win,x,y,DA_VIEW_W-5,y+h,TBX_COLOR_BLACK);
#endif
    }
#ifndef SCRAPBOOK
    if(s==e && g_active) {
        int16_t x,y; FlairTEGetPoint(&g_te,(int16_t)s,&x,&y);
        if(y>=g_scroll && y+h-g_scroll<=DA_TEXT_H)
            (void)tbx_fill_rect(g_win,4+x,4+y-g_scroll,5+x,4+y-g_scroll+h,TBX_COLOR_BLACK);
    }
    /* Functional folded corners: documented adaptation, not a native golden.
     * Left corner previous; right corner next. Half-open 20x20 hit regions. */
    for(int i=0;i<DA_FOLD;i++) {
        if(g_page>0) (void)tbx_fill_rect(g_win,i,DA_VIEW_H-DA_FOLD+i,i+1,DA_VIEW_H-DA_FOLD+i+1,TBX_COLOR_BLACK);
        if(g_page<7) (void)tbx_fill_rect(g_win,DA_VIEW_W-DA_FOLD+i,DA_VIEW_H-1-i,DA_VIEW_W-DA_FOLD+i+1,DA_VIEW_H-i,TBX_COLOR_BLACK);
    }
#endif
    {
        int at=decimal(0,g_count?g_page+1:0);
        g_label[at++]=' ';g_label[at++]='o';g_label[at++]='f';g_label[at++]=' ';
        at=decimal(at,g_count);g_label[at]=0;
        (void)tbx_draw_text(g_win,30,DA_VIEW_H-19,g_label,TBX_COLOR_BLACK|TBX_TEXT_GENEVA9,TBX_COLOR_WHITE);
#ifdef SCRAPBOOK
        (void)tbx_draw_text(g_win,160,DA_VIEW_H-19,"TEXT only",TBX_COLOR_BLACK|TBX_TEXT_GENEVA9,TBX_COLOR_WHITE);
#endif
    }
}
static void refresh(void) { bar();menus_sync();paint(); }
static void turn(int page)
{
    if(page<0) page=0;
    if(page>=g_count) page=g_count?g_count-1:0;
    if(page==g_page) return;
    {
        int old=g_page;
        stash();g_page=page;
        if(!save()) {g_page=old;return;}
    }
    read_page();
}
#ifndef SCRAPBOOK
static int replace(const char *s,int n)
{
    if(n<0 || n>DA_TEXT_MAX || g_te.teLength-(g_te.selEnd-g_te.selStart)+n>DA_TEXT_MAX ||
       !FlairTECanReplace(&g_te,g_te.selStart,g_te.selEnd,s,(int16_t)n)) {
        notice("This page is full.");return 0;
    }
    FlairTEInsert(&g_te,s,(int16_t)n);return 1;
}
#endif
static int copy(void)
{
#ifdef SCRAPBOOK
    return g_count && tbx_scrap_put(g_te.text,(uint32_t)g_te.teLength)==TBX_OK;
#else
    return g_te.selStart<g_te.selEnd && tbx_scrap_put(g_te.text+g_te.selStart,(uint32_t)(g_te.selEnd-g_te.selStart))==TBX_OK;
#endif
}
static void clear(void)
{
#ifdef SCRAPBOOK
    if(!g_count) return;
    for(int p=g_page;p<g_count-1;p++) { g_len[p]=g_len[p+1]; bytes(g_pages[p],g_pages[p+1],g_len[p]); }
    g_count--; g_len[g_count]=0;
    if(g_page>=g_count) g_page=g_count?g_count-1:0;
    read_page();
#else
    (void)replace("",0);
#endif
}
static void paste(void)
{
    int n=tbx_scrap_get(g_clip,sizeof g_clip),k=0,was_cr=0;
    if(n<=0) return;                          /* no TEXT, including PICT */
    for(int i=0;i<n;i++) {
        int c=(unsigned char)g_clip[i];
        if(c==10 && was_cr) {was_cr=0;continue;}
        was_cr=c==13;
        if(c==10) c=13;
        if(!(c==13 || c==9 || c>=32)) { notice("Only plain TEXT clips are supported."); return; }
        g_clip[k++]=(char)c;
    }
#ifdef SCRAPBOOK
    if(g_count==DA_SLOTS || k>DA_TEXT_MAX || !FlairTECanReplace(&g_te,0,g_te.teLength,g_clip,(int16_t)k)) {
        notice("The Scrapbook is full, or this clip is too large.");return;
    }
    stash();g_page=g_count++;
#ifdef DA_MUT_PASTE_WRONG_PAGE
    int target=(g_page+1)%DA_SLOTS;
#else
    int target=g_page;
#endif
    g_len[target]=(uint16_t)k;bytes(g_pages[target],g_clip,k);read_page();
#else
#ifdef DA_MUT_PASTE_WRONG_PAGE
    int target=(g_page+1)%DA_SLOTS;
    if(k<=DA_TEXT_MAX) {g_len[target]=(uint16_t)k;bytes(g_pages[target],g_clip,k);}
#else
    (void)replace(g_clip,k);
#endif
#endif
}
static void close_app(void)
{
#ifndef DA_MUT_SKIP_CLOSE_SAVE
    stash();
    if(!save()) return;
#endif
    /* A kept (damaged) store: say so once, and close on the answer. */
    if(g_keep && !g_closing &&
       tbx_alert("The " DA_TITLE " file is damaged.\rIt was kept as it is; these\rpages were not saved.","OK",0,0)==TBX_OK) {
        g_closing=1; return;
    }
    (void)tbx_exit(0);
}
int tbx_app_main(void)
{
    if(tbx_register()!=TBX_OK) return 1;
    FlairTE_init(&g_te,DA_VIEW_W-10,FLAIR_TE_JUST_LEFT);g_te.txFont=1;
    empty();load();read_page();g_active=1;g_anchor=0;
    (void)tbx_set_mbar(&mbar);
    g_win=tbx_new_window(DA_WIN_L,DA_WIN_T,DA_WIN_R,DA_WIN_B,TBX_GOAWAY_ASK);
    if(g_win<=0) {(void)tbx_exit(1);return 1;}
    (void)tbx_set_wtitle(g_win,DA_TITLE);bar();menus_sync();return 0;
}
void tbx_app_event(const tbx_event_t *ev)
{
    switch(ev->what) {
    case updateEvt: {
        int sv=tbx_get_scroll(g_win);
#ifdef SCRAPBOOK
        if(sv>=0 && sv!=g_page*DA_VIEW_H) turn(sv>g_page*DA_VIEW_H?(sv+DA_VIEW_H-1)/DA_VIEW_H:sv/DA_VIEW_H);
        bar();
#else
        if(sv>=0) g_scroll=sv;
#endif
        paint();break;
    }
    case activateEvt: g_active=(ev->modifiers&FLAIR_EVT_MOD_ACTIVE_FLAG)!=0;menus_sync();paint();break;
    case TBX_EVT_GOAWAY: close_app();break;
    case TBX_EVT_ALERT: if(g_closing) (void)tbx_exit(0);break;
    case TBX_EVT_MENU:
        if(((uint32_t)ev->message>>16)==129) {close_app();break;}
        if(((uint32_t)ev->message>>16)!=130) break;
        switch(ev->message&65535) {
        case 3: if(copy()) clear();break;
        case 4: (void)copy();break;
        case 5: paste();break;
        case 6: clear();break;
        case 7: FlairTESetSelect(&g_te,0,g_te.teLength);break;
        default:break;
        }
        refresh();break;
    case mouseDown:
    case TBX_EVT_TRACK: {
        int32_t org=tbx_content_origin(g_win);
        int x=ev->whereh-(int)(org&65535),y=ev->wherev-(int)((uint32_t)org>>16);
        if(org<0) break;
#ifndef SCRAPBOOK
        if(ev->what==mouseDown && y>=DA_VIEW_H-DA_FOLD && y<DA_VIEW_H && x>=0 && x<DA_VIEW_W) {
            if(x<DA_FOLD) turn(g_page-1);
            else if(x>=DA_VIEW_W-DA_FOLD) {
#ifdef DA_MUT_TURN_OFFBYONE
                turn(g_page+2);
#else
                turn(g_page+1);
#endif
            }
            refresh();break;
        }
        if(ev->what==mouseDown) {
            if(y>=DA_TEXT_H || y<0) break;
            g_anchor=FlairTEClick(&g_te,(int16_t)(x-5),(int16_t)(y-4+g_scroll),(ev->modifiers&FLAIR_EVT_MOD_SHIFT_KEY)!=0);
            (void)tbx_track_mouse();
        } else FlairTESetSelect(&g_te,(int16_t)g_anchor,FlairTEGetOffset(&g_te,(int16_t)(x-5),(int16_t)(y-4+g_scroll)));
        refresh();
#else
        (void)x;(void)y;
#endif
        break;
    }
    case keyDown:
    case autoKey: {
        int c=ev->message&255, vk=(ev->message>>8)&255;
        if(ev->modifiers&FLAIR_EVT_MOD_CONTROL_KEY) break;
#ifdef SCRAPBOOK
        (void)c;
        if(vk==0x4b || vk==0x49) turn(g_page-1);
        else if(vk==0x4d || vk==0x51) turn(g_page+1);
        else if(vk==0x48 && g_scroll>0) g_scroll-=11;
        else if(vk==0x50 && g_scroll+DA_TEXT_H<g_te.nLines*11+4) g_scroll+=11;
#else
        if(c==8) FlairTEKey(&g_te,8);
        else if(c==13 || c==10 || (c>=32 && c<127)) {char ch=c==10?13:(char)c;(void)replace(&ch,1);}
        else if(vk==0x4b) FlairTEKey(&g_te,FLAIR_TE_CH_LEFT);
        else if(vk==0x4d) FlairTEKey(&g_te,FLAIR_TE_CH_RIGHT);
        else if(vk==0x48) FlairTEKey(&g_te,FLAIR_TE_CH_UP);
        else if(vk==0x50) FlairTEKey(&g_te,FLAIR_TE_CH_DOWN);
#endif
        refresh();break;
    }
    default:break;
    }
}
