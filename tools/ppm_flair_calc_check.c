/* Factory structural video oracle, PRD 6.5 / Appendix A/B; tdnl.20.
 * NO artifact/spec-data includes. Chrome pixels come DIRECTLY from the
 * independent Mac OS 8.1 capture, window-chrome.md 2.1/3.1 coordinates.
 * Content rectangles are authored policy in flair_calc_traces.mk. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <png.h>
static unsigned char p[640*480*3], g[640*480*3];
static uint32_t px(const unsigned char *im,int x,int y)
{ const unsigned char *b=im+3*(640*y+x); return b[0]*65536u+b[1]*256u+b[2]; }
static int ink(int l,int t,int r,int b)
{ int n=0; for(int y=t;y<b;y++) for(int x=l;x<r;x++) n+=px(p,x,y)==0; return n; }
static int ink_height(int l,int t,int r,int b)
{ int lo=b,hi=t-1; for(int y=t;y<b;y++) if(ink(l,y,r,y+1)) { if(y<lo)lo=y; hi=y; } return hi-lo+1; }
static int check(int ok,const char *s)
{ if(!ok) fprintf(stderr,"FAIL calc pixels: %s\n",s); return !ok; }
/* Structural five-slice check, independent of the artifact's atan2/rasterizer:
 * count colour transitions on a Pythagorean radius40 contour. Each of the
 * five positive PRD weights must have a visible sector, including the seam. */
static int pie_cx=476, pie_cy=197;
static int pie_transitions(void)
{
    uint32_t prev=0; int n=0,seen=0;
    for(int q=0;q<4;q++) for(int y=0;y<=40;y++) {
        int x=0; while((x+1)*(x+1)+y*y<=1600) x++;
        int dx=q==0?x:q==1?-y:q==2?-x:y;
        int dy=q==0?y:q==1?x:q==2?-y:-x;
        uint32_t c=px(p,pie_cx+dx,pie_cy+dy);
        if(seen&&c!=prev) n++;
        seen=1; prev=c;
    }
    return n;
}
int main(int argc,char **argv)
{
    int w,h,m,bad=0; FILE *f; png_image im;
    if(argc!=3&&argc!=4) { fprintf(stderr,"usage: ppm_flair_calc_check dump.ppm s8_doc_window_active.png\n"); return 2; }
    f=fopen(argv[1],"rb");
    if(!f||fscanf(f,"P6 %d %d %d",&w,&h,&m)!=3||w!=640||h!=480||m!=255||
        fgetc(f)==EOF||fread(p,1,sizeof p,f)!=sizeof p) return 2;
    fclose(f); memset(&im,0,sizeof im); im.version=PNG_IMAGE_VERSION;
    if(!png_image_begin_read_from_file(&im,argv[2])) return 2;
    im.format=PNG_FORMAT_RGB;
    if(im.width!=640||im.height!=480||!png_image_finish_read(&im,0,g,0,0)) return 2;
    png_image_free(&im);
    if(argc==4) {
        int dragged=!strcmp(argv[3],"drag");
        int l=dragged?360:300, t=dragged?160:120;
        /* Deliberate FILM re-key from the old blank WDEF bodies: tenant ink
         * is required in the original M024/M025 patches. Calc's existing
         * local layout is retained; no photo dimensions/palette are graded. */
        for(int y=0;y<22;y++) bad+=check(px(p,l+42,t+y)==px(g,120,70+y),"film independent Platinum title profile");
        bad+=check(ink(74,126,230,141)>40&&ink(74,150,230,165)>40&&ink(74,174,230,189)>40,"FILM_LEDGER three known labels");
        bad+=check(px(p,80,120)!=0xFFFFFF&&px(p,80,144)==px(p,80,120)&&px(p,80,168)==px(p,80,120),"FILM_GRID existing Calc rules");
        if(dragged) {
            pie_cx=436; pie_cy=257;
            bad+=check(ink(388,209,485,306)>300,"PIE_DRAW real tenant pie");
            bad+=check(pie_transitions()==5,"PIE_FIVE five visible sectors");
            for(int i=0;i<5;i++) bad+=check(ink(486,202+22*i,520,214+22*i)>8,"PIE_LABELS five percentage labels");
            bad+=check(ink(270,230,299,244)>8,"FILM_MINUS trailing-minus specimen cell");
            bad+=check(px(p,300,150)==0xFFFFFF,"FILM_DRAG restored old chart footprint");
        } else {
            bad+=check(ink(330,170,420,198)>10,"PIE_DRAW original M024 patch no longer blank");
            /* Independent literal silhouette from ADR-0004 AM-4 canon;
             * no renderer asset includes. The film photo only establishes
             * presence. Cursor hotspot(8,8) -> origin(462,220). */
            static const unsigned short hourglass[]={0xffff,0xffff,0x7ffe,0x3ffc,0x1ff8,0xff0,0x7e0,0x180,0x180,0x7e0,0xff0,0x1ff8,0x3ffc,0x7ffe,0xffff,0xffff};
            for(int y=0;y<16;y++) for(int x=0;x<16;x++) if(hourglass[y]&(0x8000u>>x))
                bad+=check(px(p,462+x,220+y)==0,"BUSY_HOURGLASS Saving selects canon cursor");
        }
        if(bad) return 1;
        puts("film pixels PASS: independent Platinum chrome + Calc ledger/grid/pie + Saving cursor or chart drag");
        return 0;
    }
    for(int y=0;y<22;y++) bad+=check(px(p,442,100+y)==px(g,120,70+y),"independent Platinum title profile");
    for(int y=0;y<12;y++) for(int x=0;x<12;x++)
        bad+=check(px(p,404+x,104+y)==px(g,69+x,74+y),"independent close widget");
    /* Inactive ledger outline is #777777: sys8/window-chrome.md6,
     * s8_doc_window_inactive.png x13/325 y100; active chart is black. */
    bad+=check(px(p,10,200)==0x777777&&px(p,389,200)==0x777777&&px(p,400,240)==0&&px(p,634,240)==0,"two separate framed windows");
    bad+=check(ink(24,110,190,125)>40&&ink(24,134,190,149)>40&&ink(24,158,190,173)>40,"three ledger labels");
    /* Independent NFNT_5415.bin: union of Film Costs ink is 7 rows.
     * Only height is graded: the existing clean-room Geneva v0 art/advances
     * are retained, not falsely advertised as a pixel-exact NFNT port. */
    bad+=check(ink_height(24,111,190,127)==7,"GENEVA9 seven-row ledger ink");
    bad+=check(ink(220,110,340,125)>40,"TODO_FRAME amount");
    bad+=check(ink(220,214,300,230)>10,"trailing-minus specimen cell");
    bad+=check(ink(220,280,350,296)>30,"footer total");
    bad+=check(ink(416,140,515,240)>300,"pie has painted sectors");
    bad+=check(pie_transitions()==5,"PIE_FIVE five visible sectors");
    bad+=check(ink(526,140,600,260)>40,"WINDOW_TOKENS chart labels and sum");
    if(bad) return 1;
    puts("calc pixels PASS: independent Platinum profile/widget + two windows + ledger/total/pie structure");
    return 0;
}
