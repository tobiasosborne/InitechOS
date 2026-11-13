/* Factory routing oracle, initech-tdnl.20. Expectations transcribed from
 * PRD 6.5 / Appendix A/B, NEVER from calc_ledger.json or its generated header.
 * Two windows use the existing Window Manager / tenant gate. Layout is
 * explicitly product policy in spec/flair_calc_traces.mk, not a frame crop. */
#include <stdio.h>
#include <string.h>
#include "../calc.c"
#include "tbx_host.h"

static int failures, checks, draws[3], total, negative;
#define nw tbx_host.nwindows
#define exits tbx_host.exit_calls
#define installed tbx_host.bar
#define alerts tbx_host.alerts
#define alert_text tbx_host.alert_text
#define alert_buttons (1+!!tbx_host.alert_btn[1][0]+!!tbx_host.alert_btn[2][0])
static uint32_t chart_pixels[217][208];
static int percent_labels[5];
static int contour_changes(void)
{
    uint32_t prev=chart_pixels[75][110]; int n=0;
    for(int q=0;q<4;q++) for(int y=0;y<=40;y++) {
        int x=0; while((x+1)*(x+1)+y*y<=1600) x++;
        int dx=q==0?x:q==1?-y:q==2?-x:y;
        int dy=q==0?y:q==1?x:q==2?-y:-x;
        uint32_t c=chart_pixels[75+dy][70+dx];
        if(c!=prev) n++;
        prev=c;
    }
    return n;
}
#define CHECK(c,s) do { checks++; if (!(c)) { failures++; fprintf(stderr,"FAIL %s\n",s); } } while(0)
/* Rasterize recorded calls only; no private Toolbox transport. */
static void event(int what,int msg) {
    tbx_event_t e={what,msg,0,0,0,0}; tbx_host.nops=0; tbx_app_event(&e);
    for(unsigned i=0;i<tbx_host.nops;i++) {
        const tbx_host_op_t *o=&tbx_host.ops[i];
        int w=o->a[0];
        if(!strcmp(o->name,"tbx_fill_rect")) {
            CHECK(w==1||w==2,"fill token");
            if(w==2) for(int y=o->a[2];y<o->a[4]&&y<217;y++)
                for(int x=o->a[1];x<o->a[3]&&x<208;x++)
                    if(y>=0&&x>=0) chart_pixels[y][x]=(uint32_t)o->a[5];
        }
        if(!strcmp(o->name,"tbx_draw_text")) {
            CHECK(w==1||w==2,"text token");if(w==1||w==2)draws[w]++;
            CHECK(o->a[3]==65537,"Geneva 9 token");
            if(!strcmp(o->text,"$112,276.87"))total++;
            if(!strcmp(o->text,"570-"))negative++;
            const char *pct[]={"40%","35%","18%","14%","9%"};
            if(w==2)for(int k=0;k<5;k++)if(!strcmp(o->text,pct[k]))percent_labels[k]++;
        }
    }
}
static void content(int c,int r,char *s)
{ i123_cell_content(&g_sheet,i123_find(&g_sheet,(uint16_t)c,(uint16_t)r),s,80); }
int main(void)
{
    tbx_host_reset();
    char s[80]; static const char *labels[]={"'Film Costs","'Matte Cutters","'Exacto Blades"};
    CHECK(tbx_app_main()==0,"launch");
    CHECK(!strcmp(tbx_app_name,"CALC"),"registered identity");
    CHECK(!strcmp(CALC_PAINT_MENU,"File Edit Image Layer Select View Window Help"),"PRD paint bar locked data");
    CHECK(!strcmp(CALC_COPY_CAPTION,"Saving tables to disk..."),"PRD copy caption locked data");
    CHECK(!strcmp(CALC_COPY_RATE_FILE,"michael_bolton.conf"),"PRD configuration filename locked data");
    CHECK(!strcmp(CALC_COPY_RATE_KEY,"PROGRESS_BAR_RATE"),"PRD progress key locked data");
    CHECK(tbx_host.windows[0].goaway==2 && tbx_host.windows[1].goaway==2,"close asks tenant");
    CHECK(!strcmp(tbx_host.windows[0].title,"InitechCalc") && !strcmp(tbx_host.windows[1].title,"Pie Chart"),"window titles");
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
    CHECK(chart_pixels[75][70]!=TBX_COLOR_WHITE,"PIE_DRAW chart is painted");
    CHECK(contour_changes()==5,"PIE_FIVE five distinct visible sectors");
    for(int i=0;i<5;i++) CHECK(percent_labels[i]>0,"PIE_LABELS each percentage rendered");
    CHECK(draws[1]>0&&draws[2]>0,"both window tokens painted");
    CHECK(total&&negative,"footer total and trailing-minus specimen actually drawn");
    /* L007: a CLEAN worksheet quits at once, never asking. */
    event(TBX_EVT_GOAWAY,2); CHECK(exits==1&&alerts==0,"QUIT_CLEAN a clean close box quits without a question");
    event(keyDown,'7'); event(keyDown,'3'); event(keyDown,'\r');
    content(0,0,s); CHECK(!strcmp(s,"73"),"editing uses i123 model");
    /* L007: after an edit File > Quit / the close box ASK (Save / Cancel / Don't Save). */
    event(TBX_EVT_MENU,TBX_MENU_WORD(129,2));
    CHECK(exits==1&&alerts==1&&alert_buttons==3,"QUIT_ASKS an edited File > Quit asks, three buttons, no exit");
    CHECK(strstr(alert_text,"Save changes")!=NULL,"QUIT_ASKS the Macintosh Save-changes wording");
    event(TBX_EVT_ALERT,TBX_ALERT_CANCEL); CHECK(exits==1,"QUIT_CANCEL Cancel keeps the worksheet open");
    event(TBX_EVT_GOAWAY,2); CHECK(exits==1&&alerts==2,"QUIT_CLOSE the close box asks again (still dirty)");
    event(TBX_EVT_ALERT,TBX_ALERT_OTHER); CHECK(exits==2,"QUIT_DONTSAVE Don't Save ends the session");
    printf("test_calc_tenant: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
