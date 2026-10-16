/* Factory C pixel gate. Expected pixels are the independent historical crops
 * (same oracle as host) plus literal disabled-state geometry from scrollbars.md
 * 3. Geometry endpoints are locked in flair_scroll_look_traces.mk. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "scrollbar_look_golden.h"
static unsigned char pixels[640*480*3];
static uint32_t px(int x,int y)
{
    if(x<0 || x>=640 || y<0 || y>=480) return 0xFFFFFFFFu;
    unsigned char *p=pixels+3*(y*640+x);
    return p[0]*65536u+p[1]*256u+p[2];
}
static int crop(int i,int x,int y)
{
    const scroll_look_crop *c=&SL_crops[i]; int bad=0;
    for(int v=0;v<c->h;v++) for(int h=0;h<c->w;h++)
        bad+=px(x+h,y+v)!=sl_mapped_rgb(c->pixels[v*c->w+h],8);
    if(bad) fprintf(stderr,"FAIL scroll-look crop %s: %d/%d pixels at(%d,%d)\n",
                    c->name,bad,c->w*c->h,x,y);
    return bad;
}
static int disabled(int axis,int lo,int hi,int cross)
{
    int bad=0;
    /* Whole 14px interior, including arrow tiles and their separators. */
    for(int a=lo+1;a<hi-1;a++) for(int c=0;c<14;c++) {
        uint32_t want=0xF3F3F3;
        if(a==lo+15 || a==hi-16) want=0x777777;
        else if(a<lo+15 || a>hi-16) {
            int far=a>hi-16, off=far ? a-(hi-15) : a-(lo+1);
            int tile=axis==0 ? (far?3:2) : (far?5:4);
            int i=axis==0 ? off*14+c : c*14+off;
            if(SL_crops[tile].pixels[i]=='K') want=0xA5A5A5;
        }
        bad+=px(axis==0?cross+c:a,axis==0?a:cross+c)!=want;
    }
    if(bad) fprintf(stderr,"FAIL scroll-look disabled band axis%d: %d pixels\n",axis,bad);
    return bad;
}
int main(int argc,char **argv)
{
    if(argc!=3) {fprintf(stderr,"usage: ppm_flair_scroll_look_check shrink|click|restore|min|itext dump.ppm\n");return 2;}
    FILE *f=fopen(argv[2],"rb"); int w,h,max,bad=0;
    if(!f || fscanf(f,"P6 %d %d %d",&w,&h,&max)!=3 || w!=640 || h!=480 || max!=255 ||
       fgetc(f)==EOF || fread(pixels,1,sizeof pixels,f)!=sizeof pixels) {
        fprintf(stderr,"FAIL scroll-look missing or malformed640x480 PPM\n");if(f)fclose(f);return 2;
    }
    fclose(f);
    if(!strcmp(argv[1],"shrink") || !strcmp(argv[1],"click")) {
        /* value16 of max35 (tdnl.51: view 101, was max30 -> x70) */
        bad+=crop(1,!strcmp(argv[1],"click")?66:41,160);
        bad+=crop(4,26,160)+crop(5,113,160);
        bad+=disabled(0,81,160,128);
        bad+=px(147,100)!=0 || px(80,60)!=0;
    } else if(!strcmp(argv[1],"restore")) {
        bad+=disabled(0,81,260,360)+disabled(1,25,360,260);
        bad+=px(379,200)!=0 || px(80,60)!=0;
    } else if(!strcmp(argv[1],"min")) {
        bad+=crop(0,96,97)+crop(1,41,128);
        bad+=crop(2,96,82)+crop(3,96,113)+crop(4,26,128)+crop(5,81,128);
        bad+=px(115,120)!=0 || px(80,147)!=0 || px(80,60)!=0;
    } else if(!strcmp(argv[1],"itext")) {
        /* ehq4: locked TEXT frame(4,44)..(588,476), 40 Returns -> 41
         * Geneva9 lines, doc459/view389, value=max70. V-bar x567..582,
         * y65..455; thumb at the far end425..439. Never query a renderer.
         * The six independent SL crops are PNG-verified by test-scrollbar-
         * look. The well's five-value cross-section + near-end shadow are
         * measured in s8_doc_window_inactive.png x608..621,y327..384
         * (scrollbars.md 2.3). Check the ENTIRE bar, including its frame. */
        bad+=crop(0,568,425)+crop(2,568,66)+crop(3,568,441);
        int band=0;
        for(int y=65;y<456;y++) {
            band+=px(567,y)!=0;band+=px(582,y)!=0;
            if(y==65 || y==80 || y==424 || y==440 || y==455)
                for(int x=568;x<582;x++)band+=px(x,y)!=0;
        }
        for(int y=81;y<424;y++)for(int c=0;c<14;c++) {
            uint32_t want;
            if(y==81) want=0x969696;
            else if(y==82) want=0xA5A5A5;
            else if(c==0) want=0x969696;
            else if(c==1) want=0xA5A5A5;
            else if(c==12) want=0xCDCDCD;
            else if(c==13) want=0xDADADA;
            else want=0xC0C0C0;
            band+=px(568+c,y)!=want;
        }
        if(band)fprintf(stderr,"FAIL itext-scroll whole enabled bar: %d pixels\n",band);
        bad+=band;
        /* s8_simpletext_doc_caret.png: black, 1px wide, one line tall, one
         * pixel left of text position. Global(14,440)..(15,451) after the
         * locked input; all other content must be white (empty CR lines).
         * Thus a clipped/offscreen caret cannot satisfy the gate. */
        int caret=0;
        for(int y=66;y<455;y++)for(int x=10;x<567;x++) {
            uint32_t want=x==14 && y>=440 && y<451 ? 0 : 0xFFFFFF;
            caret+=px(x,y)!=want;
        }
        if(caret)fprintf(stderr,"FAIL itext-scroll caret/content: %d pixels\n",caret);
        bad+=caret;
        bad+=disabled(1,9,568,456);
    } else {fprintf(stderr,"FAIL unknown scroll-look phase\n");return 2;}
    if(bad) {fprintf(stderr,"FAIL scroll-look %s: %d pixel differences\n",argv[1],bad);return 1;}
    printf("scroll-look %s PASS: independent crops + full disabled bands + frame endpoint\n",argv[1]);
    return 0;
}
