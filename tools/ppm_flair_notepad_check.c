/* R4.4 independent composed-chrome pixel gate (factory C).
 * No DA layout/renderer headers. Original Mac OS 8.1 PNGs are arguments.
 * documentProc title cross-section and close-widget classes from
 * s8_doc_window_active at struct(65,70); SimpleText white content from
 * s8_simpletext_empty. No claim to grade missing native DA fold/label pixels.
 * Scrapbook arrow crops are compared directly with s8_doc_window_inactive's
 * ACTIVE Applications window (scrollbars.md Sec 2.4), nominal-map via the
 * independently PNG-verified scroll-look golden (NOT renderer data). */
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scrollbar_look_golden.h"
typedef struct { int w,h; unsigned char *p; } img;
static int png(const char *path,img *a)
{
    png_image p; memset(&p,0,sizeof p); p.version=PNG_IMAGE_VERSION;
    if(!png_image_begin_read_from_file(&p,path)) return 0;
    p.format=PNG_FORMAT_RGB; a->w=(int)p.width; a->h=(int)p.height;
    a->p=malloc(PNG_IMAGE_SIZE(p));
    return a->p && png_image_finish_read(&p,NULL,a->p,0,NULL);
}
static unsigned rgb(const img *a,int x,int y)
{ const unsigned char *p=a->p+3*(y*a->w+x); return p[0]*65536u+p[1]*256u+p[2]; }
static int cls(unsigned p) { return p==0 ? 1 : p==0xffffff ? 0 : 2; }
int main(int argc,char **argv)
{
    img a,b,c,d; int max,w,h,bad=0; FILE *f;
    if(argc!=6) { fprintf(stderr,"usage: check pad|scrapbook dump.ppm active.png simpletext.png inactive.png\n"); return 2; }
    f=fopen(argv[2],"rb");
    if(!f || fscanf(f,"P6 %d %d %d",&w,&h,&max)!=3 || w!=640 || h!=480 || max!=255) return 2;
    a.w=w; a.h=h; a.p=malloc(640*480*3); fgetc(f);
    if(!a.p || fread(a.p,3,640*480,f)!=640*480) return 2;
    fclose(f);
    if(!png(argv[3],&b)||!png(argv[4],&c)||!png(argv[5],&d) ||
       b.w!=640||b.h!=480||c.w!=640||c.h!=480||d.w!=640||d.h!=480) return 2;
    /* Full 22-row title anatomy, away from titles and widgets. */
    for(int y=0;y<22;y++) bad+=cls(rgb(&a,210,80+y))!=cls(rgb(&b,120,70+y));
    for(int y=0;y<13;y++)for(int x=0;x<13;x++)
        bad+=cls(rgb(&a,164+x,84+y))!=cls(rgb(&b,69+x,74+y));
    for(int y=0;y<50;y++)for(int x=0;x<80;x++)
        bad+=rgb(&a,180+x,210+y)!=rgb(&c,30+x,210+y);
    if(!strcmp(argv[1],"scrapbook")) {
        /* Active arrow tiles compared against original PNG classes, and
         * exact nominal matrices independently proven by test-scrollbar-look. */
        for(int y=0;y<14;y++)for(int x=0;x<14;x++) {
            bad+=cls(rgb(&a,460+x,102+y))!=cls(rgb(&d,608+x,296+y));
            bad+=rgb(&a,460+x,102+y)!=sl_mapped_rgb(SL_up[y*14+x],8);
        }
    }
    if(bad) printf("RED-REASON chrome: %d independent PNG probe mismatches\n",bad);
    else printf(">>> DA PIXEL: documentProc title/close + TextEdit paper%s agree with independent Mac OS 8.1 captures\n",
                !strcmp(argv[1],"scrapbook")?" + Platinum page-bar arrow":"");
    free(a.p);free(b.p);free(c.p);free(d.p);
    return bad!=0;
}
