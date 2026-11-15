/* The frame's chart on the booted screen, graded against the still's
 * reference package. Reads a 640 x 480 binary PPM (the QEMU screendump) and
 * points and boxes given on the command line; tests/frame-pixels.sh derives
 * every one of them from goldens/frame (geometry.tsv, colours.tsv,
 * transcription.tsv). Nothing here knows InitechCalc's drawing code.
 *
 *   frame-pixels shot.ppm REGION l t r b  PIE l t r b  ELLIPSE cx cy a b
 *                [SERIES v1 ... vn] [AT x y role] ...
 *   frame-pixels shot.ppm FRAME l t r b ... [TEXT l t r b] [BAR l t r b]
 *                [NONAVY l t r b]
 *
 * REGION    where on screen the chart window may be (navy outside the pie is
 *           searched for only here)
 * PIE       the still's pie: the navy pixels' bounds must be within 4 px
 * ELLIPSE   the still's top face: at 80% of the way out, at each slice's
 *           middle (value/sum of a turn clockwise from 12 o'clock), the
 *           pixel must be a slice face
 * AT        a sampled point and its role, "field" (black) or "face" (navy)
 * FRAME     a window's outer bounds, half-open, given back to front: each
 *           edge pixel no later FRAME covers is the black outline, and the
 *           pixel just inside it is not black (the frame's rail or bevel,
 *           never a black field running to the edge), except at most 4 per
 *           edge (its two corners and where a title bar's rule meets it).
 *           Exact: no slop.
 * TEXT      a caption's box: its black pixels (searched 4 px around the box)
 *           start within 4 px of its left, top and bottom
 * BAR       a progress bar's box: its black outline's bounds within 4 px
 * NONAVY    a box in front of the chart: no slice navy shows through it
 * Colours: the Toolbox's black (0,0,0) and navy (0,0,128), the stand-in for
 * the still's slice blue until the SDK has one (goldens/frame/README.md
 * question 12). Tolerance: 3/255 per channel, the DAC's quantisation. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { W=640, H=480 };
static unsigned char px[H][W][3];
static int failures, checks;
static void fail(const char *s,int x,int y)
{
    failures++;
    printf("FAIL frame pixels: %s at (%d,%d): #%02x%02x%02x\n",s,x,y,px[y][x][0],px[y][x][1],px[y][x][2]);
}
static int near(int v,int want) { return v>=want-3&&v<=want+3; }
static int is(int x,int y,int r,int g,int b)
{ return x>=0&&x<W&&y>=0&&y<H&&near(px[y][x][0],r)&&near(px[y][x][1],g)&&near(px[y][x][2],b); }
static int navy(int x,int y) { return is(x,y,0,0,128); }
static int black(int x,int y) { return is(x,y,0,0,0); }
enum { FRAMES=8 };
static int frames[FRAMES][4], nframes;
static int covered(int k,int x,int y)   /* by a FRAME in front of frame k */
{
    for(int j=k+1;j<nframes;j++) if(x>=frames[j][0]&&x<frames[j][2]&&y>=frames[j][1]&&y<frames[j][3]) return 1;
    return 0;
}
static int inner_black;   /* edge pixels of the current edge with a black inside */
static void edge(int k,int x,int y,int ix,int iy,const char *side)
{
    char s[96];
    if(x<0||x>=W||y<0||y>=H||covered(k,x,y)) return;
    checks++;
    if(black(x,y)&&black(ix,iy)&&++inner_black<=4) return;
    if(!black(x,y)||black(ix,iy)) {
        snprintf(s,sizeof s,"window %d (%d,%d)-(%d,%d): no outline on its %s edge",k+1,frames[k][0],frames[k][1],frames[k][2],frames[k][3],side);
        fail(s,x,y);
    }
}
/* Bounds of the black pixels in box b grown by 4; 0 when there are none. */
static int dark_bounds(const int *b,int *o)
{
    int l=W,t=H,r=-1,d=-1;
    for(int y=b[1]-4;y<b[3]+4;y++) for(int x=b[0]-4;x<b[2]+4;x++) if(black(x,y)) {
        if(x<l) l=x;
        if(x>r) r=x;
        if(y<t) t=y;
        if(y>d) d=y;
    }
    o[0]=l; o[1]=t; o[2]=r+1; o[3]=d+1;
    return r>=0;
}
static int num(char **argv,int i,int argc) { if(i>=argc) { fputs("frame-pixels: missing argument\n",stderr); exit(2); } return atoi(argv[i]); }
int main(int argc,char **argv)
{
    if(argc<2) return 2;
    FILE *f=fopen(argv[1],"rb"); char magic[3]; unsigned w,h,max;
    if(!f||fscanf(f,"%2s %u %u %u",magic,&w,&h,&max)!=4||strcmp(magic,"P6")||w!=W||h!=H||max!=255||fgetc(f)!='\n'
       ||fread(px,1,sizeof px,f)!=sizeof px) { fprintf(stderr,"frame-pixels: not a 640x480 P6: %s\n",argv[1]); return 2; }
    fclose(f);
    int reg[4]={0,0,W,H}, pie[4]={0}, havepie=0, haveell=0; double ell[4]={0}, ser[8]; int ns=0;
    int text[4], bar[4], nonavy[4], havetext=0, havebar=0, havenonavy=0;
    for(int i=2;i<argc;) {
        if(!strcmp(argv[i],"REGION")) { for(int k=0;k<4;k++) reg[k]=num(argv,i+1+k,argc); i+=5; }
        else if(!strcmp(argv[i],"PIE")) { for(int k=0;k<4;k++) pie[k]=num(argv,i+1+k,argc); havepie=1; i+=5; }
        else if(!strcmp(argv[i],"ELLIPSE")) { for(int k=0;k<4;k++) { if(i+1+k>=argc) return 2; ell[k]=atof(argv[i+1+k]); } haveell=1; i+=5; }
        else if(!strcmp(argv[i],"SERIES")) { i++; while(i<argc&&ns<8&&(argv[i][0]=='.'||(argv[i][0]>='0'&&argv[i][0]<='9'))) ser[ns++]=atof(argv[i++]); }
        else if(!strcmp(argv[i],"FRAME")) { if(nframes==FRAMES) return 2; for(int k=0;k<4;k++) frames[nframes][k]=num(argv,i+1+k,argc); nframes++; i+=5; }
        else if(!strcmp(argv[i],"TEXT")) { for(int k=0;k<4;k++) text[k]=num(argv,i+1+k,argc); havetext=1; i+=5; }
        else if(!strcmp(argv[i],"BAR")) { for(int k=0;k<4;k++) bar[k]=num(argv,i+1+k,argc); havebar=1; i+=5; }
        else if(!strcmp(argv[i],"NONAVY")) { for(int k=0;k<4;k++) nonavy[k]=num(argv,i+1+k,argc); havenonavy=1; i+=5; }
        else if(!strcmp(argv[i],"AT")) {
            int x=num(argv,i+1,argc), y=num(argv,i+2,argc); const char *role=i+3<argc?argv[i+3]:"";
            checks++;
            if(!strcmp(role,"field")) { if(!black(x,y)) fail("the chart field is not black",x,y); }
            else if(!strcmp(role,"face")) { if(!navy(x,y)) fail("no slice face",x,y); }
            else { fprintf(stderr,"frame-pixels: unknown role %s\n",role); return 2; }
            i+=4;
        } else { fprintf(stderr,"frame-pixels: unknown argument %s\n",argv[i]); return 2; }
    }
    if(havepie) {
        int l=W,t=H,r=-1,b=-1;
        for(int y=reg[1];y<reg[3];y++) for(int x=reg[0];x<reg[2];x++) if(navy(x,y)) {
            if(x<l) l=x;
            if(x>r) r=x;
            if(y<t) t=y;
            if(y>b) b=y;
        }
        checks++;
        if(r<0) { failures++; puts("FAIL frame pixels: no pie on screen"); }
        else {
            int got[4]={l,t,r+1,b+1}, off=0;
            for(int k=0;k<4;k++) if(got[k]-pie[k]<-4||got[k]-pie[k]>4) off=1;
            if(off) { failures++; printf("FAIL frame pixels: the pie spans %d,%d to %d,%d; the still puts it at %d,%d to %d,%d\n",l,t,r+1,b+1,pie[0],pie[1],pie[2],pie[3]); }
        }
    }
    if(haveell&&ns) {
        double sum=0,a=0; for(int i=0;i<ns;i++) sum+=ser[i];
        for(int i=0;i<ns;i++) {
            double m=(a+ser[i]/2)/sum*2*3.14159265358979323846;
            int x=(int)lround(ell[0]+0.8*ell[2]*sin(m)), y=(int)lround(ell[1]-0.8*ell[3]*cos(m));
            checks++;
            if(!navy(x,y)) fail("no slice face at a slice's middle",x,y);
            a+=ser[i];
        }
    }
    for(int k=0;k<nframes;k++) {
        const int *f=frames[k];
        inner_black=0; for(int y=f[1];y<f[3];y++) edge(k,f[0],y,f[0]+1,y,"left");
        inner_black=0; for(int y=f[1];y<f[3];y++) edge(k,f[2]-1,y,f[2]-2,y,"right");
        inner_black=0; for(int x=f[0];x<f[2];x++) edge(k,x,f[1],x,f[1]+1,"top");
        inner_black=0; for(int x=f[0];x<f[2];x++) edge(k,x,f[3]-1,x,f[3]-2,"bottom");
    }
    if(havetext) {
        int o[4]; checks++;
        if(!dark_bounds(text,o)) { failures++; printf("FAIL frame pixels: no caption in (%d,%d)-(%d,%d)\n",text[0],text[1],text[2],text[3]); }
        else if(o[0]-text[0]<-4||o[0]-text[0]>4||o[1]-text[1]<-4||o[1]-text[1]>4||o[3]-text[3]<-4||o[3]-text[3]>4) {
            failures++; printf("FAIL frame pixels: the caption spans (%d,%d)-(%d,%d); the still sets it in (%d,%d)-(%d,%d)\n",o[0],o[1],o[2],o[3],text[0],text[1],text[2],text[3]); }
    }
    if(havebar) {
        int o[4], off=0, seen; checks++;
        seen=dark_bounds(bar,o);
        if(seen) for(int k=0;k<4;k++) if(o[k]-bar[k]<-4||o[k]-bar[k]>4) off=1;
        if(off||!seen) { failures++; printf("FAIL frame pixels: the progress bar spans (%d,%d)-(%d,%d); the still has it at (%d,%d)-(%d,%d)\n",o[0],o[1],o[2],o[3],bar[0],bar[1],bar[2],bar[3]); }
    }
    if(havenonavy) {
        int n=0; checks++;
        for(int y=nonavy[1];y<nonavy[3];y++) for(int x=nonavy[0];x<nonavy[2];x++) n+=navy(x,y);
        if(n) { failures++; printf("FAIL frame pixels: %d navy pixels show through (%d,%d)-(%d,%d), which is in front of the chart\n",n,nonavy[0],nonavy[1],nonavy[2],nonavy[3]); }
    }
    printf("frame-pixels: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
