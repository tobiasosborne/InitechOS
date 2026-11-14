/* What InitechCalc shows of the frame, observed from outside: run the real
 * tenant over sdk/host with a ledger document, record its Toolbox calls and
 * print what was drawn where. tests/frame-content.sh compares this with
 * goldens/frame/transcription.tsv; nothing here knows the expected content.
 *
 * Output lines (tab-separated):
 *   WINDOW  n l t r b       the bounds each window was opened with
 *   CELL    row col text    a ledger draw at a cell origin (row 1-based)
 *   NOTE    text            the ledger's recovery annotation
 *   OTHER   x y text        any other ledger draw (a failure for the script)
 *   LABEL   text            a chart draw
 *   RING    360 chars       the chart around the pie, 1 per degree clockwise
 *                           from 12 o'clock: N solid navy (a slice's face),
 *                           S navy and black rows (a side), B black, o other
 * The cell origins are the layout (README.md "The ledger"): column rules at
 * 0 100 146 ... 468, rows 12 pixels, text 2 right of the rule and 1 below. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "tbx_host.h"
/* The SDK fake's default origin/size is one fixed test viewport. Use each
 * recorded window's CalcDocContentRect (the public Toolbox binding's insets),
 * as the existing file-copy suite does, when observing saved bounds. */
static int32_t frame_origin(int32_t w)
{
    if(w<1||w>tbx_host.nwindows) return TBX_ERR_BADARG;
    const int l=tbx_host.windows[w-1].l+6,t=tbx_host.windows[w-1].t+22;
    return (int32_t)(((uint32_t)(uint16_t)t<<16)|(uint16_t)l);
}
static int32_t frame_size(int32_t w)
{
    if(w<1||w>tbx_host.nwindows) return TBX_ERR_BADARG;
    const int cw=tbx_host.windows[w-1].r-tbx_host.windows[w-1].l-27;
    const int ch=tbx_host.windows[w-1].b-tbx_host.windows[w-1].t-43;
    return (int32_t)(((uint32_t)(uint16_t)ch<<16)|(uint16_t)cw);
}
#define tbx_content_origin frame_origin
#define tbx_content_size frame_size
#include "../calc.c"
#undef tbx_content_origin
#undef tbx_content_size

static const int lx[]={0,100,146,192,238,284,330,376,422,468};
static uint8_t px[480][640];   /* chart content, colour token + 1 */
int main(int argc,char **argv)
{
    const char *path=argc>1?argv[1]:"data/LEDGER.TXT";
    static char buf[65536]; FILE *f=fopen(path,"rb"); size_t n;
    if(!f) { perror(path); return 2; }
    n=fread(buf,1,sizeof buf,f); fclose(f);
    tbx_host_reset();
    if(!tbx_host_put_file("APPS\\LEDGER.TXT",buf,n)) return 2;
    if(tbx_app_main()!=0) { puts("LAUNCH failed"); return 1; }
    for(unsigned i=0,k=0;i<tbx_host.nops;i++) {
        const tbx_host_op_t *o=&tbx_host.ops[i];
        if(!strcmp(o->name,"tbx_new_window")) printf("WINDOW\t%u\t%d\t%d\t%d\t%d\n",++k,o->a[0],o->a[1],o->a[2],o->a[3]);
        if(!strcmp(o->name,"tbx_alert")) printf("ALERT\t%s\n",o->text);
    }
    tbx_event_t e={updateEvt,0,0,0,0,0};
    tbx_host.nops=0;
    if(!tbx_host_push_event(&e)) return 2;
    (void)tbx_host_dispatch(tbx_app_event);
    if(tbx_host.dropped_ops) { puts("DROPPED ops"); return 1; }
    int bottom=0,note_y=-1;
    for(unsigned i=0;i<tbx_host.nops;i++) {
        const tbx_host_op_t *o=&tbx_host.ops[i];
        if(!strcmp(o->name,"tbx_fill_rect")&&o->a[0]==1&&o->a[5]==TBX_COLOR_GRAY&&o->a[4]-o->a[2]==1&&o->a[1]==0) bottom=o->a[2];
        if(!strcmp(o->name,"tbx_draw_text")&&o->a[0]==1&&o->a[1]==4) note_y=o->a[2];
    }
    for(unsigned i=0;i<tbx_host.nops;i++) {
        const tbx_host_op_t *o=&tbx_host.ops[i];
        int w=o->a[0], x=o->a[1], y=o->a[2];
        if(!strcmp(o->name,"tbx_draw_text")&&w==1) {
            int c=-1; for(int k=0;k<10;k++) if(x==lx[k]+2) c=k;
            if(x==4) printf("NOTE\t%s\n",o->text);
            else if(c>=0&&y%12==1&&y<bottom) printf("CELL\t%d\t%d\t%s\n",y/12+1-(note_y>=0&&note_y<bottom&&y>note_y),c,o->text);
            else printf("OTHER\t%d\t%d\t%s\n",x,y,o->text);
        }
        if(!strcmp(o->name,"tbx_draw_text")&&w==2) printf("LABEL\t%s\n",o->text);
        if(!strcmp(o->name,"tbx_fill_rect")&&w==2)
            for(int yy=o->a[2];yy<o->a[4];yy++) for(int xx=o->a[1];xx<o->a[3];xx++)
                if(yy>=0&&yy<480&&xx>=0&&xx<640) px[yy][xx]=(uint8_t)(o->a[5]+1);
    }
    /* The ring: 80% of the way out on the unexploded top face. */
    char ring[361];
    for(int d=0;d<360;d++) {
        double t=d*3.14159265358979323846/180.0;
        int xx=(int)lround(PIE_CX+0.8*PIE_A*sin(t)), yy=(int)lround(PIE_CY-0.8*PIE_B*cos(t));
        int v=px[yy][xx], up=px[yy-1][xx], dn=px[yy+1][xx], navy=TBX_COLOR_NAVY+1, black=TBX_COLOR_BLACK+1;
        if(v==navy&&up==navy&&dn==navy) ring[d]='N';
        else if(v==black&&up==black&&dn==black) ring[d]='B';
        else if((v==navy||v==black)&&(up==navy||up==black)&&(dn==navy||dn==black)) ring[d]='S';
        else ring[d]='o';
    }
    ring[360]=0;
    printf("RING\t%s\n",ring);
    if(argc>2) {
        /* Drive the ordinary Save request, then the host FILE COPY provider
         * copies the stage and delivers its completion, as in file-copy tests.
         * Export the resulting destination for the separate awk reader. */
        e=(tbx_event_t){TBX_EVT_MENU,TBX_MENU_WORD(129,1),0,0,0,0};
        if(!tbx_host_push_event(&e)) return 2;
        (void)tbx_host_dispatch(tbx_app_event);
        const tbx_host_file_t *s=tbx_host_file(tbx_host.copy_src);
        unsigned bytes=s?s->size:0;
        if(s&&!tbx_host_put_file(tbx_host.copy_dst,s->data,s->size)) return 2;
        if(s) {
            e=(tbx_event_t){TBX_EVT_FILECOPY,(int32_t)bytes,0,0,0,0};
            if(!tbx_host_push_event(&e)) return 2;
            (void)tbx_host_dispatch(tbx_app_event);
        }
        const tbx_host_file_t *saved=tbx_host_file("APPS\\TABLES.TXT");
        FILE *out=fopen(argv[2],"wb"); if(!out) return 2;
        if(saved&&fwrite(saved->data,1,saved->size,out)!=saved->size) return 2;
        if(fclose(out)) return 2;
    }
    return 0;
}
