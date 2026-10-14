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
    if(argc!=3) {fprintf(stderr,"usage: ppm_flair_scroll_look_check shrink|click|restore|min dump.ppm\n");return 2;}
    FILE *f=fopen(argv[2],"rb"); int w,h,max,bad=0;
    if(!f || fscanf(f,"P6 %d %d %d",&w,&h,&max)!=3 || w!=640 || h!=480 || max!=255 ||
       fgetc(f)==EOF || fread(pixels,1,sizeof pixels,f)!=sizeof pixels) {
        fprintf(stderr,"FAIL scroll-look missing or malformed640x480 PPM\n");if(f)fclose(f);return 2;
    }
    fclose(f);
    if(!strcmp(argv[1],"shrink") || !strcmp(argv[1],"click")) {
        bad+=crop(1,!strcmp(argv[1],"click")?70:41,160);
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
    } else {fprintf(stderr,"FAIL unknown scroll-look phase\n");return 2;}
    if(bad) {fprintf(stderr,"FAIL scroll-look %s: %d pixel differences\n",argv[1],bad);return 1;}
    printf("scroll-look %s PASS: independent crops + full disabled bands + frame endpoint\n",argv[1]);
    return 0;
}
