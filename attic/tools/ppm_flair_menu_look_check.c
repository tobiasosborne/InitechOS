/* Independent menu-look crops; no artifact includes or expected-value calls.
 * Finder File width: Close Window NFNT advance91 + LPAD19 + gap8 + ^W20 +
 * cmdpad13 =151; frame x33+151-1=183. View origin33+41+43=117.
 * Caption geometry preserves Chicago (Charcoal decision tdnl.69 unchanged).
 * Marks stay in the existing leading margin: cell panel+4, ink panel+5;
 * menus.md2.2 reserves20px for item ink but records NO captured mark item.
 * That placement is retained FLAIR policy, not a claimed measured Mac pixel.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "menu_look_golden.h"
static unsigned char pixels[640*480*3];
static uint32_t px(int x,int y)
{
    if(x<0||x>=640||y<0||y>=480)return 0xFFFFFFFF;
    unsigned char *p=pixels+3*(y*640+x);return p[0]*65536u+p[1]*256u+p[2];
}
int main(int argc,char **argv)
{
    if(argc!=3){fprintf(stderr,"usage: ppm_flair_menu_look_check file|view dump.ppm\n");return 2;}
    FILE *f=fopen(argv[2],"rb");int w,h,max,bad=0;
    if(!f||fscanf(f,"P6 %d %d %d",&w,&h,&max)!=3||w!=640||h!=480||max!=255||
       fgetc(f)==EOF||fread(pixels,1,sizeof pixels,f)!=sizeof pixels) {
        fprintf(stderr,"FAIL menu-look missing/malformed640x480 PPM\n");if(f)fclose(f);return 2;
    }
    fclose(f);
    if(!strcmp(argv[1],"file")) {
        int band=0;for(int y=0;y<20;y++)band+=px(34,20+y)!=ml_band_rgb(y,8);
        if(band)fprintf(stderr,"FAIL menu-look ML1 title band: %d/20 differ\n",band);
        bad+=band;int left=640,top=480,right=-1;
        for(int y=41;y<56;y++)for(int x=35;x<182;x++)if(px(x,y)==0) {
            if(x<left)left=x;
            if(x>right)right=x;
            if(y<top)top=y;
        }
        int geom=left!=53||top!=43||right!=169||px(183,45)!=0||px(33,45)!=0;
        if(geom)fprintf(stderr,"FAIL menu-look ML4 geometry: first(%d,%d), last%d, want53,43/169 + frame183 = clearance14\n",left,top,right);
        bad+=geom;
        int divider=0;
        /* Finder has four items before its first divider: captured row102
         * translated to y106 for this inventory and second menu band. Grade
         * EVERY column x33..183; neither inner bevel is exempt (r78f). */
        for(int y=0;y<2;y++)for(int x=33;x<=183;x++)
            divider+=px(x,106+y)!=ml_divider_rgb(x-33,151,y);
        if(divider)fprintf(stderr,"FAIL menu-look ML10 whole divider rows: %d/302 differ\n",divider);
        bad+=divider;
        int ink=0;for(int y=0;y<9;y++)for(int x=0;x<7;x++) {
            uint32_t want=(ML_file_F[y]&(0x8000u>>x))?0xFFFFFF:ml_band_rgb(y+5,8);
            ink+=px(42+x,25+y)!=want;
        }
        if(ink){fprintf(stderr,"FAIL menu-look ML9 white NFNT F: %d/63 differ\n",ink);bad++;}
    } else if(!strcmp(argv[1],"view")) {
        int diff=0,count=0;for(int y=0;y<16;y++)for(int x=0;x<11;x++) {
            int ink=px(121+x,40+y)==0;count+=ink;diff+=ink!=!!(ML_check[y]&(0x8000u>>x));
        }
        if(diff||count!=18){fprintf(stderr,"FAIL menu-look ML5 NFNT check: %d/176 differ, ink%d !=18\n",diff,count);bad++;}
    } else return 2;
    if(bad)return 1;
    printf(">>> ppm_flair_menu_look_check: %s green (independent crop)\n",argv[1]);return 0;
}
