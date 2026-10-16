/* Factory routing oracle, initech-tdnl.20. Expectations transcribed from
 * PRD 6.5 / Appendix A/B, NEVER from calc_ledger.json or its generated header.
 * Two windows use the existing Window Manager / tenant gate. Layout is
 * explicitly product policy in spec/flair_calc_traces.mk, not a frame crop. */
#include <stdio.h>
#include <string.h>
#include "../../os/apps/calc/calc.c"

static int failures, checks, nw, exits, draws[3], total, negative;
static const tbx_menubar_t *installed;
#define CHECK(c,s) do { checks++; if (!(c)) { failures++; fprintf(stderr,"FAIL %s\n",s); } } while(0)
int32_t tbx_register(void) { return 0; }
int32_t tbx_new_window(int32_t l,int32_t t,int32_t r,int32_t b,int32_t g)
{ (void)l;(void)t;(void)r;(void)b; CHECK(g==TBX_GOAWAY_ASK,"close asks tenant"); return ++nw; }
int32_t tbx_set_wtitle(int32_t w,const char *s)
{ CHECK((w==1&&!strcmp(s,"InitechCalc"))||(w==2&&!strcmp(s,"Pie Chart")),"window titles"); return 0; }
int32_t tbx_set_mbar(const tbx_menubar_t *b) { installed=b; return 0; }
int32_t tbx_draw_menubar(void) { return 0; }
int32_t tbx_fill_rect(int32_t w,int32_t l,int32_t t,int32_t r,int32_t b,uint32_t c)
{ (void)l;(void)t;(void)r;(void)b;(void)c; CHECK(w==1||w==2,"fill token"); return 0; }
int32_t tbx_draw_text(int32_t w,int32_t x,int32_t y,const char *s,uint32_t f,uint32_t b)
{
    (void)x;(void)y;(void)b;
    CHECK(w==1||w==2,"text token");
    if(w==1||w==2) draws[w]++;
    CHECK(f==65537u,"Geneva 9 token");
    if(!strcmp(s,"$112,276.87")) total++;
    if(!strcmp(s,"570-")) negative++;
    return 0;
}
int32_t tbx_exit(int32_t r) { CHECK(r==0,"exit status"); exits++; return 0; }
int32_t tbx_file_copy(const char *c,const char *s,const char *d) { (void)c; (void)s; (void)d; return 0; } /* bf2: routed in test_calc_file_copy.c */
static void event(int what,int msg)
{ tbx_event_t e={what,msg,0,0,0,0}; tbx_app_event(&e); }
static void content(int c,int r,char *s)
{ i123_cell_content(&g_sheet,i123_find(&g_sheet,(uint16_t)c,(uint16_t)r),s,80); }
int main(void)
{
    char s[80]; static const char *labels[]={"'Film Costs","'Matte Cutters","'Exacto Blades"};
    CHECK(tbx_app_main()==0,"launch");
    CHECK(!strcmp(tbx_app_name,"CALC"),"registered identity");
    CHECK(!strcmp(CALC_PAINT_MENU,"File Edit Image Layer Select View Window Help"),"PRD paint bar locked data");
    CHECK(!strcmp(CALC_COPY_CAPTION,"Saving tables to disk..."),"PRD copy caption locked data");
    CHECK(!strcmp(CALC_COPY_RATE_FILE,"michael_bolton.conf"),"PRD configuration filename locked data");
    CHECK(!strcmp(CALC_COPY_RATE_KEY,"PROGRESS_BAR_RATE"),"PRD progress key locked data");
    CHECK(nw==2,"ledger and chart are separate windows");
    for(int i=0;i<3;i++) { content(0,i,s); CHECK(!strcmp(s,labels[i]),"PRD ledger label");
        content(1,i,s); CHECK(!strcmp(s,"'TODO_FRAME"),"unknown amount stays TODO_FRAME"); }
    CHECK(installed&&installed->n_menus==1,"File menu installed");
    CHECK(!strcmp(installed->menus[0].items[0].text,"Save")&&installed->menus[0].items[0].enabled,"Save enabled: the FILE COPY box (initech-bf2)");
    CHECK(!strcmp(installed->menus[0].items[1].text,"Quit")&&installed->menus[0].items[1].enabled,"Quit enabled");
    calc_format(-570,s); CHECK(!strcmp(s,"570-"),"TRAILING_MINUS -570 -> 570-");
    calc_format(570,s); CHECK(!strcmp(s,"570"),"positive has no sign");
    calc_format(-12.5,s); CHECK(!strcmp(s,"12.5-"),"fraction trailing minus");
    calc_format(0,s); CHECK(!strcmp(s,"0"),"zero");
    CHECK(calc_pie_sum()==116,"PIE_SUM 40+35+18+14+9 == 116");
    for(int i=0;i<5;i++) CHECK((calc_slice(i)==(int[]){40,35,18,14,9}[i]),"PRD slice");
    event(updateEvt,0);
    CHECK(draws[1]>0&&draws[2]>0,"both window tokens painted");
    CHECK(total&&negative,"footer total and trailing-minus specimen actually drawn");
    event(keyDown,'7'); event(keyDown,'3'); event(keyDown,'\r');
    content(0,0,s); CHECK(!strcmp(s,"73"),"editing uses i123 model");
    event(TBX_EVT_MENU,TBX_MENU_WORD(129,2)); CHECK(exits==1,"File Quit routed");
    event(TBX_EVT_GOAWAY,2); CHECK(exits==2,"chart close routed");
    printf("test_calc_tenant: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
