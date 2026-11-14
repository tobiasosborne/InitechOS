/*
 * tools/ppm_flair_paint_check.c -- InitechPaint screendump grader (factory C,
 * host; bead initech-tdnl.20, the bar-carrier half). The BAR itself is graded
 * by ppm_flair_app_launch_check bar-photoshop (band 2 pixel-exact against the
 * hand-carried canon titles in the real NFNT 5478 metrics); this tool grades:
 *
 *   canvas dump.ppm  the blank document: the canvas content (global x 340..600,
 *                    y 180..370: inside PAINT's (330,150)-(630,400) window,
 *                    clear of its frame, title bar and scroll bars) is
 *                    entirely white -- InitechPaint opens an EMPTY canvas.
 *   empty idle.ppm held.ppm title: the authored Inside Macintosh IV-55 empty
 *                    heading behavior; held title highlighted, all pixels
 *                    below the bar unchanged. Pointer must occupy the same
 *                    position in both captures. Toolbox owner supplies these
 *                    captures after fixing zero-row panel geometry.
 *
 * Exit 0 PASS, 1 named FAIL, 2 unusable input. ASCII-clean (CLAUDE.md, The constitution).
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "chicago12_nfnt_golden.h"

static unsigned char pixels[640 * 480 * 3];
static uint32_t px(int x, int y)
{
    if (x < 0 || x >= 640 || y < 0 || y >= 480) return 0xFFFFFFFFu;
    const unsigned char *p = pixels + 3 * (y * 640 + x);
    return (uint32_t)p[0] * 65536u + (uint32_t)p[1] * 256u + p[2];
}
static int sw(const char *s)
{
    int w = 0;
    for (; *s; s++) w += NFNT5478_AW[(unsigned char)*s - NFNT5478_FIRST];
    return w;
}

static int canvas(void)
{
    int bad = 0;
    for (int y = 180; y < 370; y++)
        for (int x = 340; x < 600; x++) bad += px(x, y) != 0xFFFFFFu;
    if (bad) {
        fprintf(stderr, "FAIL paint CANVAS: %d non-white pixels in the blank document\n", bad);
        return 1;
    }
    printf("paint CANVAS PASS: the document is a blank white canvas\n");
    return 0;
}

/* Empty titles follow the authored IM IV-55 contract. Idle and held captures
 * must use the same pointer position so pointer pixels are not a mask. */
static unsigned char idle[sizeof pixels];
static int empty(const char *title)
{
    const char *titles[]={"File","Edit","Image","Layer","Select","View","Window","Help"};
    /* The live desktop merges its 33-pixel system slot before the app's
     * titles (one bar). NFNT widths here remain independent golden data. */
    int x=33,found=0;
    for(unsigned i=0;i<8;i++) {
        if(!strcmp(title,titles[i]) && i) {found=1;break;}
        x+=sw(titles[i])+18;
    }
    if(!found) return 2;
    if(px(x+2,3)!=0 || !memcmp(idle+3*(3*640+x+2),pixels+3*(3*640+x+2),3)) {
        fprintf(stderr,"FAIL paint EMPTY: title is not highlighted\n");return 1;
    }
    if(memcmp(idle+640*20*3,pixels+640*20*3,sizeof pixels-640*20*3)) {
        fprintf(stderr,"FAIL paint EMPTY: pixels below the menu bar changed\n");return 1;
    }
    puts("paint EMPTY PASS: held title highlighted; no panel or shadow");return 0;
}
static int read_ppm(const char *path)
{
    int w,h,max;FILE *f=fopen(path,"rb");
    if(!f || fscanf(f,"P6 %d %d %d",&w,&h,&max)!=3 || w!=640 || h!=480 ||
       max!=255 || fgetc(f)==EOF || fread(pixels,1,sizeof pixels,f)!=sizeof pixels) {
        if(f) fclose(f);
        fprintf(stderr,"FAIL paint: missing or malformed 640x480 PPM %s\n",path);return 0;
    }
    fclose(f);return 1;
}
int main(int argc,char **argv)
{
    if(argc==3 && !strcmp(argv[1],"canvas")) {
        if(!read_ppm(argv[2])) return 2;
        return canvas();
    }
    if(argc==5 && !strcmp(argv[1],"empty")) {
        if(!read_ppm(argv[2])) return 2;
        memcpy(idle,pixels,sizeof pixels);
        if(!read_ppm(argv[3])) return 2;
        return empty(argv[4]);
    }
    fputs("usage: pixels canvas dump.ppm | empty idle.ppm held.ppm title\n",stderr);return 2;
}
