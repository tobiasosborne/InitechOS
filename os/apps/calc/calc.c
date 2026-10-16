/* CALC.EXE -- a SMALL C disk tenant of the i123 core, PRD 6.5 / Appendix A/B.
 * Locked content: spec/assets/calc_ledger.json -> calc_ledger.h. Unknown
 * frame cells stay TODO_FRAME. Positions, two-column view and grayscale pie
 * are authored product policy (flair_calc_traces.mk), NOT recovered film art.
 * The unlocated -570 specimen is separate from the unknown ledger amounts.
 * Uses the existing Toolbox text, Window/Menu/Event Managers and i123 sparse
 * cell/edit/number engine. No kernel app, second spreadsheet or event loop.
 * C artifact, freestanding, ASCII, deterministic (CLAUDE Laws 1-4). */
#include "tbx.h"
#include "i123.h"
#include "i123/wk1_format.h"
#include "calc_ledger.h"
const char tbx_app_name[]="CALC";
static const tbx_menuitem_t file_items[]={
    {"Save",0,'S',0,1,0,{0,0,0}}, /* bf2: the FILE COPY box (Sec 12) */
    {"Quit",0,'Q',0,1,0,{0,0,0}}
};
/* initech-bf2, PRD 6.5 / Appendix A: File > Save is the FILE COPY box. The
 * caption is the locked PRD literal (calc_ledger.json copy_caption); the copy
 * is REAL: the shipped ledger file is copied byte-for-byte, through the DOS
 * handle API, to a new file on the data volume (spec/toolbox_gate.h Sec 12).
 * Paths are authored product policy (APPS holds the volume's data files; the
 * root listing is locked). */
#ifdef CALC_MUT_CAPTION
static const char copy_caption[]="Saving tables to disk"; /* NAMED MUTANT */
#else
static const char copy_caption[]=CALC_COPY_CAPTION;
#endif
static const char copy_src[]="APPS\\LEDGER.TXT";
static const char copy_dst[]="APPS\\TABLES.TXT";
static const tbx_menuinfo_t menus[]={{129,0,"File",file_items,2,0}};
static const tbx_menubar_t mbar={menus,1,1,0};
static uint8_t store[8192];
static i123_sheet_t g_sheet;
static int32_t ledger TBX_IN_IMAGE=0, chart TBX_IN_IMAGE=0;
static char line[80] TBX_IN_IMAGE={0};
static char entry[64] TBX_IN_IMAGE={0};
static unsigned entered;
static unsigned length(const char *s) { unsigned n=0; while(s[n]) n++; return n; }
static void copy(char *d,const char *s) { do { *d++=*s; } while(*s++); }
static int calc_slice(int i)
{
#ifdef CALC_MUT_PIE100
    if(i==0) return calc_pie_slices[0]-16;
#endif
    return calc_pie_slices[i];
}
static int calc_pie_sum(void) { int n=0; for(int i=0;i<5;i++) n+=calc_slice(i); return n; }
/* The only Calc format adaptation: reuse i123's number conversion, move its
 * sign to the tail. No second numeric parser/formatter (PRD trailing minus). */
static void calc_format(double x,char *out)
{
    char number[80]; i123_number_text(x<0?-x:x,number,sizeof number);
#ifdef CALC_MUT_LEADING_MINUS
    if(x<0) { out[0]='-'; copy(out+1,number); }
    else copy(out,number);
#else
    if(x<0) { copy(out,number); unsigned n=length(out); out[n]='-'; out[n+1]=0; }
    else copy(out,number);
#endif
}
static void text(int32_t w,int x,int y,const char *s)
{ (void)tbx_draw_text(w,x,y,s,TBX_COLOR_BLACK|TBX_TEXT_GENEVA9,TBX_COLOR_WHITE); }
static void cell_text(unsigned c,unsigned r)
{
    const i123_cell_t *p=i123_find(&g_sheet,(uint16_t)c,(uint16_t)r);
    i123_cell_content(&g_sheet,p,line,sizeof line);
    if(p&&p->kind==I123_K_LABEL) {
        for(unsigned i=0;line[i];i++) line[i]=line[i+1]; /* strip i123 label prefix */
    } else { double x; if(i123_parse_value(line,length(line),&x)==I123_OK) calc_format(x,line); }
}
static void paint_ledger(void)
{
    (void)tbx_fill_rect(ledger,0,0,353,252,TBX_COLOR_WHITE);
    /* Grid labels A/B and a selected cell are worksheet UI, not film data. */
    text(ledger,8,2,"A"); text(ledger,204,2,"B");
    for(int i=0;i<=3;i++) (void)tbx_fill_rect(ledger,4,18+24*i,344,19+24*i,TBX_COLOR_GRAY);
    for(int i=0;i<3;i++) {
        cell_text(0,(unsigned)i); text(ledger,8,24+24*i,line);
        cell_text(1,(unsigned)i); text(ledger,204,24+24*i,line);
    }
    (void)tbx_fill_rect(ledger,196,18,197,91,TBX_COLOR_GRAY);
    int x=i123_cursor_col(&g_sheet)?200:4, y=20+24*i123_cursor_row(&g_sheet);
    (void)tbx_fill_rect(ledger,x,y,x+140,y+1,TBX_COLOR_BLACK);
    text(ledger,8,104,"570- format");
    cell_text(1,4); text(ledger,204,128,line);
    text(ledger,8,164,entered?entry:"TODO_FRAME");
    /* Footer is the PRD-stated total; never sum unknown TODO_FRAME amounts. */
    i123_format_number((double)CALC_TOTAL_CENTS/100.0,(uint8_t)((WK1_FT_CURRENCY<<4)|2),24,line);
    unsigned start=0; while(line[start]==' ') start++;
    unsigned n=length(line); while(n>start&&line[n-1]==' ') line[--n]=0;
    text(ledger,204,194,line+start);
}
/* Integer atan2 approximation (one turn=65536), used only to partition a
 * mathematical disk by the five PRD weights. No trig runtime or bitmap asset.
 * All percentages retain their stated values, even though total is 116. */
static int __attribute__((unused)) sector(int x,int y)
{
    int ay=y<0?-y:y, a;
    if(!x&&!y) return 0;
    if(x>=0) a=8192-8192*(x-ay)/(x+ay);
    else a=24576-8192*(x+ay)/(ay-x);
    if(y<0) a=65536-a;
    int point=a*calc_pie_sum()/65536, end=0;
    for(int i=0;i<4;i++) { end+=calc_slice(i); if(point<end) return i; }
    return 4;
}
static void paint_chart(void)
{
    /* Authored palette policy over Sec7 role tokens: neighbours, including
     * last/first at the east seam, have different shades (five visible slices). */
#ifdef CALC_MUT_MERGE_SLICES
    static const uint32_t shades[]={TBX_COLOR_BLACK,TBX_COLOR_GRAY,TBX_COLOR_NAVY,TBX_COLOR_GRAY,TBX_COLOR_BLACK};
#else
    static const uint32_t shades[]={TBX_COLOR_BLACK,TBX_COLOR_GRAY,TBX_COLOR_NAVY,TBX_COLOR_BLACK,TBX_COLOR_GRAY};
#endif
    (void)shades;
    (void)tbx_fill_rect(chart,0,0,208,217,TBX_COLOR_WHITE);
#ifndef CALC_MUT_NO_PIE
    for(int y=-48;y<=48;y++) {
        int x=-48;
        while(x<=48) {
            if(x*x+y*y>48*48) { x++; continue; }
            int begin=x, s=sector(x,y); x++;
            while(x<=48&&x*x+y*y<=48*48&&sector(x,y)==s) x++;
            (void)tbx_fill_rect(chart,70+begin,75+y,70+x,76+y,shades[s]);
        }
    }
#endif
#ifndef CALC_MUT_NO_LABELS
    for(int i=0;i<5;i++) {
        i123_number_text((double)calc_slice(i),line,sizeof line);
        unsigned n=length(line); line[n]='%'; line[n+1]=0;
        text(chart,120,20+22*i,line);
    }
#endif
    i123_number_text((double)calc_pie_sum(),line,sizeof line);
    unsigned n=length(line); line[n]='%'; line[n+1]=0;
    text(chart,120,134,line);
}
static void paint(void) { paint_ledger(); paint_chart(); }
int tbx_app_main(void)
{
    if(tbx_register()<0||i123_sheet_init(&g_sheet,store,sizeof store,32)!=I123_OK) return 1;
    entered=0; entry[0]=0;
    for(unsigned i=0;i<3;i++) {
        if(i123_set_label(&g_sheet,0,(uint16_t)i,'\'',calc_ledger_labels[i],length(calc_ledger_labels[i]))!=I123_OK||
           i123_set_label(&g_sheet,1,(uint16_t)i,'\'',calc_ledger_amounts[i],length(calc_ledger_amounts[i]))!=I123_OK) return 1;
    }
    if(i123_set_number(&g_sheet,1,4,(double)CALC_NEGATIVE_SPECIMEN)!=I123_OK) return 1;
    if(tbx_set_mbar(&mbar)<0) return 1;
    ledger=tbx_new_window(10,65,390,360,TBX_GOAWAY_ASK);
    if(ledger<0||tbx_set_wtitle(ledger,"InitechCalc")<0) { tbx_exit(1); return 1; }
    chart=tbx_new_window(400,100,635,360,TBX_GOAWAY_ASK);
    if(chart<0||tbx_set_wtitle(chart,"Pie Chart")<0) { tbx_exit(1); return 1; }
    return 0;
}
static void commit(void)
{
    double x; uint16_t c=i123_cursor_col(&g_sheet), r=i123_cursor_row(&g_sheet);
    if(!entered) return;
    if(i123_parse_value(entry,entered,&x)==I123_OK) (void)i123_set_number(&g_sheet,c,r,x);
    else (void)i123_set_label(&g_sheet,c,r,'\'',entry,entered);
    entered=0; entry[0]=0;
}
void tbx_app_event(const tbx_event_t *e)
{
    if(!e) return;
    if(e->what==updateEvt) { paint(); return; }
    if(e->what==TBX_EVT_GOAWAY||(e->what==TBX_EVT_MENU&&e->message==TBX_MENU_WORD(129,2))) { (void)tbx_exit(0); return; }
    if(e->what==TBX_EVT_MENU&&e->message==TBX_MENU_WORD(129,1)) {
        (void)tbx_file_copy(copy_caption,copy_src,copy_dst);
        return;
    }
    if(e->what==TBX_EVT_FILECOPY) return; /* the shell restored what the box covered */
    if(e->what!=keyDown) return;
    unsigned ch=(unsigned)e->message&255, vk=((unsigned)e->message>>8)&255;
    int c=i123_cursor_col(&g_sheet), r=i123_cursor_row(&g_sheet);
    if(vk==0x4b||vk==0x4d||vk==0x48||vk==0x50||ch=='\t') {
        commit();
        if(vk==0x4b) c=0;
        if(vk==0x4d||ch=='\t') c=1;
        if(vk==0x48&&r>0) r--;
        if(vk==0x50&&r<2) r++;
        i123_set_cursor(&g_sheet,(uint16_t)c,(uint16_t)r);
    } else if(ch=='\r') commit();
    else if(ch==27) { entered=0; entry[0]=0; }
    else if(ch==8) { if(entered) entry[--entered]=0; }
    else if(ch>=32&&ch<=126&&entered<sizeof entry-1) { entry[entered++]=(char)ch; entry[entered]=0; }
    paint_ledger();
}
