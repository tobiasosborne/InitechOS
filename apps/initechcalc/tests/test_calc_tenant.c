/* InitechCalc's behaviour as a tenant: launch, the document reader's
 * all-or-nothing refusals, number formats, editing and the quit guard.
 * The frame's content is graded separately (test_calc_frame against
 * goldens/frame). Format expectations are Lotus 1-2-3 Release 2 display
 * (Initech 123's own Lotus differential is the oracle for that core) with
 * the sign moved to the tail (PRD Appendix A: 570-), typed here. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tbx_host.h"
#include "host_windows.h"
#include "../calc.c"
#include "chrome_metrics.h"

static int failures, checks;
#define CHECK(c,s) do { checks++; if (!(c)) { failures++; fprintf(stderr,"FAIL %s\n",s); } } while(0)
static char shipped_bytes[4096]; static size_t shipped_len;
static void event(int what,int msg)
{
    tbx_event_t e={what,msg,0,0,0,0};
    tbx_host.nops=0;
    if(!tbx_host_push_event(&e)) exit(2);
    (void)tbx_host_dispatch(tbx_app_event);
    for(unsigned i=0;i<tbx_host.nops;i++) {
        const tbx_host_op_t *o=&tbx_host.ops[i];
        if(!strcmp(o->name,"tbx_fill_rect")) CHECK(o->a[0]==1||o->a[0]==2,"fills go to Calc's own windows");
        if(!strcmp(o->name,"tbx_draw_text")) {
            CHECK(o->a[0]==1||o->a[0]==2,"text goes to Calc's own windows");
            CHECK((o->a[3]&~3)==TBX_TEXT_GENEVA9,"Geneva 9");
            CHECK(strlen(o->text)<=80,"text within the Toolbox limit");
        }
    }
}
static int drawn(int w,const char *s)
{ for(unsigned i=0;i<tbx_host.nops;i++) if(!strcmp(tbx_host.ops[i].name,"tbx_draw_text")&&tbx_host.ops[i].a[0]==w&&!strcmp(tbx_host.ops[i].text,s)) return 1; return 0; }
static int pixel(int x,int y)
{
    int colour=-1;
    for(unsigned i=0;i<tbx_host.nops;i++) {
        const tbx_host_op_t *o=&tbx_host.ops[i];
        if(!strcmp(o->name,"tbx_fill_rect")&&o->a[0]==1&&
           x>=o->a[1]&&x<o->a[3]&&y>=o->a[2]&&y<o->a[4]) colour=o->a[5];
    }
    return colour;
}
static void treatment_a(void)
{
    int ny=-1,ty=-1,count=0,style=0;
    for(unsigned i=0;i<tbx_host.nops;i++) {
        const tbx_host_op_t *o=&tbx_host.ops[i];
        if(strcmp(o->name,"tbx_draw_text")) continue;
        CHECK(!strstr(o->text,"TODO_" "FRAME"),"no placeholder is drawn in either window");
        if(o->a[0]!=1) continue;
        if(!strcmp(o->text,"Blank entries were not recovered from the surviving material.")) {
            ny=o->a[2]; count++;
            style=o->a[3]==(TBX_COLOR_BLACK|TBX_TEXT_GENEVA9)&&o->a[4]==TBX_COLOR_WHITE;
        }
        if(!strcmp(o->text,"$112,276.87")) ty=o->a[2];
    }
    CHECK(count==1&&style,"one black Geneva 9 source note on white");
    CHECK(ny>=0&&ty>=0&&ny+12<=ty,"source note occupies its own strip above the total");
    /* Public chrome metric plus observed window bounds, and COPY's bottom
     * from goldens/frame/geometry.tsv: the upper overlays cannot cover it. */
    int global_y=tbx_host.windows[0].t+FLAIR_CHROME_TITLEBAR_H+ny;
    CHECK(ny>=0&&global_y>=tbx_host.windows[1].b&&global_y>=274,
          "note remains exposed below the chart and reference FILE COPY box");
    int white=ny>=0;
    for(int x=0;x<tbx_host.cw&&white;x++) if(pixel(x,ny+3)!=TBX_COLOR_WHITE) white=0;
    CHECK(white,"source-note strip spans the white content width without cell rules");
    CHECK(!i123_find(&g_sheet,1,0)&&!i123_find(&g_sheet,4,12)&&!i123_find(&g_sheet,9,12)&&!i123_find(&g_sheet,0,24),
          "unrecovered cells are absent model entries, not numeric zero or labels");
    CHECK(pixel(110,18)==TBX_COLOR_WHITE&&pixel(100,18)==TBX_COLOR_GRAY&&pixel(110,24)==TBX_COLOR_GRAY,
          "an unrecovered cell has a white interior and ordinary gray rules");
}
static int start_calc(const char *ledger_doc,const char *tables)
{
    tbx_host_reset();
    if(ledger_doc) tbx_host_put_file("APPS\\LEDGER.TXT",ledger_doc,(uint32_t)strlen(ledger_doc));
    if(tables) tbx_host_put_file("APPS\\TABLES.TXT",tables,(uint32_t)strlen(tables));
    return tbx_app_main();
}
static int opens(const char *doc) { return start_calc(doc,0)==0&&tbx_host.alerts==0; }
static const char *shows(unsigned c,unsigned r) { static char s[96]; if(!cell_text(c,r,s,42)) s[0]=0; return s; }
#define HDR "INITECH LEDGER\r\nSERIES\t1\r\nROWS\t3\r\n"
int main(void)
{
    char s[96]; double x;
    FILE *f=fopen("data/LEDGER.TXT","rb"); if(!f) return 2;
    shipped_len=fread(shipped_bytes,1,sizeof shipped_bytes-1,f); fclose(f); shipped_bytes[shipped_len]=0;

    CHECK(start_calc(shipped_bytes,0)==0,"launch");
    CHECK(!strcmp(tbx_app_name,"CALC"),"registered identity");
    CHECK(tbx_host.nwindows==2&&tbx_host.alerts==0,"ledger and chart are separate windows; the shipped ledger reads cleanly");
    CHECK(!strcmp(tbx_host.windows[0].title,"InitechCalc")&&!strcmp(tbx_host.windows[1].title,"Pie Chart"),"window titles");
    CHECK(tbx_host.windows[0].goaway==2&&(tbx_host.windows[1].goaway&255)==2,"close asks the tenant");
    CHECK(!host_plain(1)&&host_plain(2),"the ledger is a document window; the chart's field runs to its frame (no-grow, no scroll bars: the still's CHARTWIN)");
    CHECK(tbx_host.windows[0].l==116&&tbx_host.windows[0].t==104&&tbx_host.windows[1].l==349&&tbx_host.windows[1].t==72,"windows open where the document says");
    CHECK(tbx_host.bar&&tbx_host.bar->n_menus==1&&!strcmp(tbx_host.bar->menus[0].title,"File"),"File menu installed");
    CHECK(!strcmp(tbx_host.bar->menus[0].items[1].text,"Quit")&&tbx_host.bar->menus[0].items[1].enabled,"Quit enabled");
    CHECK(!strcmp(CALC_PAINT_MENU,"File Edit Image Layer Select View Window Help"),"PRD paint bar locked data");
    CHECK(!strcmp(CALC_COPY_CAPTION,"Saving tables to disk..."),"PRD copy caption locked data");
    CHECK(!strcmp(CALC_COPY_RATE_FILE,"michael_bolton.conf")&&!strcmp(CALC_COPY_RATE_KEY,"PROGRESS_BAR_RATE"),"PRD rate file and key locked data");

    /* Formats: Lotus display, the sign at the tail in every format. */
    calc_format(-570,FMT_GENERAL,s); CHECK(!strcmp(s,"570-"),"general -570 -> 570-");
    calc_format(570,FMT_GENERAL,s); CHECK(!strcmp(s,"570"),"general 570 has no sign");
    calc_format(-12.5,FMT_GENERAL,s); CHECK(!strcmp(s,"12.5-"),"general fraction, trailing minus");
    calc_format(0,FMT_GENERAL,s); CHECK(!strcmp(s,"0"),"general zero");
    calc_format(-570,(WK1_FT_FIXED<<4)|2,s); CHECK(!strcmp(s,"570.00-"),"fixed 2 -570 -> 570.00-");
    calc_format(98,(WK1_FT_FIXED<<4)|2,s); CHECK(!strcmp(s,"98.00"),"fixed 2 98 -> 98.00");
    calc_format(89302,(WK1_FT_FIXED<<4)|1,s); CHECK(!strcmp(s,"89302.0"),"fixed 1 89302 -> 89302.0");
    calc_format(112276.87,(WK1_FT_CURRENCY<<4)|2,s); CHECK(!strcmp(s,"$112,276.87"),"currency 2 -> $112,276.87");
    calc_format(-1234.5,(WK1_FT_CURRENCY<<4)|2,s); CHECK(!strcmp(s,"$1,234.50-"),"currency negative, trailing minus");
    CHECK(calc_parse("570-",4,&x)&&x==-570,"reads a trailing minus");
    CHECK(calc_parse("-570",4,&x)&&x==-570,"reads a leading minus");
    CHECK(calc_parse("$112,276.87",11,&x)&&x==112276.87,"reads currency");
    CHECK(!calc_parse("abc",3,&x)&&!calc_parse(".",1,&x)&&!calc_parse("",0,&x)&&!calc_parse("1-2",3,&x),"refuses what is not a number");
    /* Exponent form, as Initech 123 reads a typed value and writes a number
     * (i123.h: "d[.ddd][E[+-]x]"), with Calc's signs around it. */
    CHECK(calc_parse("1E-7",4,&x)&&x==1e-7,"reads an exponent");
    CHECK(calc_parse("1.5E+21-",8,&x)&&x==-1.5e21,"reads an exponent with a trailing minus");
    CHECK(calc_parse("-2e3",4,&x)&&x==-2000,"reads a lower-case exponent with a leading minus");
    CHECK(!calc_parse("1E",2,&x)&&!calc_parse("E5",2,&x)&&!calc_parse("1E-",3,&x)&&!calc_parse("1E5E3",5,&x)&&!calc_parse("1E,5",4,&x)&&!calc_parse("1+E5",4,&x),"refuses a broken exponent");

    /* The chart: value/sum of a turn, labels as stated. */
    CHECK(nseries==5&&series_sum()==116,"the shipped series sums to 116");
    CHECK(slice_start(0)==0&&slice_start(5)==65536,"the slices close the turn");
    CHECK(slice_start(1)==(uint32_t)(40*65536.0/116+0.5),"each slice is value/sum of a turn");
    event(updateEvt,0);
    CHECK(drawn(2,"40%")&&drawn(2,"9%")&&!drawn(2,"116%"),"labels are the values; no invented total");
    CHECK(drawn(1,"Blank entries were not recovered from the surviving material."),"the source note is drawn");
    treatment_a();

    /* The clean worksheet quits at once, never asking. */
    event(TBX_EVT_GOAWAY,2); CHECK(tbx_host.exit_calls==1&&tbx_host.alerts==0,"QUIT_CLEAN a clean close box quits without a question");

    /* Editing. */
    CHECK(start_calc(shipped_bytes,0)==0,"fresh Calc");
    event(keyDown,'7'); event(keyDown,'3'); event(keyDown,'\r');
    CHECK(!strcmp(shows(0,0),"73"),"typing a number into an empty cell makes a general number");
    i123_set_cursor(&g_sheet,1,12); event(keyDown,'5'); event(keyDown,'\r');
    CHECK(!strcmp(shows(1,12),"5.00"),"a new value keeps the cell's fixed format");
    i123_set_cursor(&g_sheet,2,0); event(keyDown,'5'); event(keyDown,'7'); event(keyDown,'0'); event(keyDown,'-'); event(keyDown,'\r');
    CHECK(!strcmp(shows(2,0),"570-"),"570- typed is minus 570");
    event(keyDown,'a'); event(keyDown,0x1b); CHECK(!strcmp(shows(2,0),"570-"),"Escape abandons the entry");
    event(keyDown,'\\'); event(keyDown,'='); event(keyDown,'\r');
    CHECK(!strcmp(shows(2,0),"======="),"a repeating label fills its cell");
    event(keyDown,0x5300); CHECK(!i123_find(&g_sheet,2,0),"Del clears the cell");
    i123_set_cursor(&g_sheet,9,25); event(keyDown,0x4d00); event(keyDown,0x5000);
    CHECK(i123_cursor_col(&g_sheet)==9&&i123_cursor_row(&g_sheet)==25,"the pointer stops at the last cell");
    i123_set_cursor(&g_sheet,0,0); event(keyDown,0x4b00); event(keyDown,0x4800);
    CHECK(i123_cursor_col(&g_sheet)==0&&i123_cursor_row(&g_sheet)==0,"and at the first");
    event(keyDown,'\t'); CHECK(i123_cursor_col(&g_sheet)==1,"Tab moves right");
    event(TBX_EVT_MENU,TBX_MENU_WORD(129,2));
    CHECK(tbx_host.exit_calls==0&&tbx_host.alerts==1&&strstr(tbx_host.alert_text,"Save changes"),"QUIT_ASKS an edited File > Quit asks");
    event(TBX_EVT_ALERT,TBX_ALERT_CANCEL); CHECK(tbx_host.exit_calls==0,"QUIT_CANCEL Cancel keeps the worksheet open");
    event(TBX_EVT_GOAWAY,2); CHECK(tbx_host.alerts==2,"QUIT_CLOSE the close box asks again");
    event(TBX_EVT_ALERT,TBX_ALERT_OTHER); CHECK(tbx_host.exit_calls==1,"QUIT_DONTSAVE Don't Save ends the session");

    /* Which document opens. */
    CHECK(start_calc(shipped_bytes,HDR "2\t'saved\r\n")==0&&tbx_host.alerts==0&&!strcmp(shows(0,1),"saved"),"saved tables open in preference to the shipped ledger");
    CHECK(start_calc(shipped_bytes,"Previous saved ledger\r\n")==0&&tbx_host.alerts==1&&strstr(tbx_host.alert_text,"could not be read")&&!strcmp(shows(0,12),"Film Costs"),"unreadable tables: say so, open the shipped ledger");
        CHECK(start_calc(0,0)==0&&tbx_host.alerts==1&&tbx_host.nwindows==2&&g_rows==CALC_ROW_MAX&&g_sheet.ncells==0&&nseries==0,"no ledger at all: say so, open an empty ledger of full size");
    CHECK(tbx_host.windows[0].l==116&&tbx_host.windows[1].l==349,"and the windows open in their home places");

    /* The reader is all or nothing. */
    CHECK(opens(HDR "1\t'a\t(F2) 1\t(G) 2-\t(C2) 3\t\\=\r\n"),"a well-formed document opens");
    CHECK(!strcmp(shows(1,0),"1.00")&&!strcmp(shows(2,0),"2-")&&!strcmp(shows(3,0),"$3.00"),"each saved format shows as itself");
    const char *bad[]={
        "INITECH LEDGEr\r\nSERIES\t1\r\nROWS\t3\r\n",          /* header */
        HDR "1\t'a",                                              /* no CR LF at the end */
        HDR "1\t'a\n",                                            /* bare LF */
        "INITECH LEDGER\r\nROWS\t3\r\n",                          /* no series */
        "INITECH LEDGER\r\nSERIES\t1\r\n1\t'a\r\n",                /* a row before ROWS */
        "INITECH LEDGER\r\nSERIES\t1\r\nROWS\t27\r\n",            /* more rows than the window */
        HDR "4\t'a\r\n",                                          /* row beyond ROWS */
        HDR "2\t'a\r\n1\t'b\r\n",                                 /* rows out of order */
        HDR "1\t'a\r\n1\t'b\r\n",                                 /* a row twice */
        HDR "1\t\t\r\n",                                          /* an empty row */
        HDR "1\t(X2) 5\r\n",                                      /* unknown format */
        HDR "1\t(F2) five\r\n",                                   /* not a number */
        HDR "1\t^centred\r\n",                                    /* unknown label prefix */
        HDR "1\t\t\t\t\t\t\t\t\t\t\t'eleventh\r\n",               /* eleven columns */
        "INITECH LEDGER\r\nSERIES\t0\r\nROWS\t3\r\n",             /* a zero slice */
        "INITECH LEDGER\r\nSERIES\t1\t2\t3\t4\t5\t6\t7\t8\t9\r\nROWS\t3\r\n", /* nine slices */
        "INITECH LEDGER\r\nWINDOW\t1\t2\t3\r\nSERIES\t1\r\nROWS\t3\r\n",       /* three bounds */
        "INITECH LEDGER\r\nSERIES\t1\r\nROWS\t3\r\nBOGUS\r\n",   /* unknown record */
    };
    for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++) {
        char msg[64]; snprintf(msg,sizeof msg,"malformed document %u is refused whole",i);
        CHECK(start_calc(bad[i],0)==0&&tbx_host.alerts==1&&g_rows==CALC_ROW_MAX&&g_sheet.ncells==0&&nseries==0,msg);
    }
    {   static char big[sizeof doc+16]; memset(big,' ',sizeof big); memcpy(big,HDR,strlen(HDR));
        tbx_host_reset(); tbx_host_put_file("APPS\\LEDGER.TXT",big,sizeof big);
        CHECK(tbx_app_main()==0&&tbx_host.alerts==1&&g_sheet.ncells==0&&nseries==0,"a document larger than the reader's buffer is refused, not cut");
    }
    CHECK(opens("INITECH LEDGER\r\nWINDOW\t300\t30\t790\t400\r\nCHART\t600\t460\t849\t676\r\nSERIES\t1\r\nROWS\t1\r\n"),"off-screen bounds open");
    CHECK(tbx_host.windows[0].l==150&&tbx_host.windows[0].r==640&&tbx_host.windows[0].t==30,"a window off the right edge comes back whole, same size");
    CHECK(tbx_host.windows[1].r==640&&tbx_host.windows[1].b==480&&tbx_host.windows[1].r-tbx_host.windows[1].l==249,"a window off the bottom right comes back whole, same size");
    CHECK(opens("INITECH LEDGER\r\nSERIES\r\nROWS\t1\r\n")&&nseries==0,"a document with no chart values opens, its chart empty");
    CHECK(opens("INITECH LEDGER\r\nSERIES\t100\r\nROWS\t1\r\n"),"a one-slice chart opens");
    event(updateEvt,0); CHECK(drawn(2,"100%"),"and draws its whole turn");

    printf("test_calc_tenant: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
