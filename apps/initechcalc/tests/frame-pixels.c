/* The frame's chart on the booted screen, graded against the still's
 * reference package. Reads a 640 x 480 binary PPM (the QEMU screendump) and
 * points and boxes given on the command line; tests/frame-pixels.sh derives
 * every one of them from goldens/frame (geometry.tsv, colours.tsv,
 * transcription.tsv). Nothing here knows InitechCalc's drawing code.
 *
 *   frame-pixels shot.ppm REGION l t r b  PIE l t r b  ELLIPSE cx cy a b
 *                [SERIES v1 ... vn] [AT x y role] ...
 *   frame-pixels shot.ppm FRAME l t r b ... [TEXT l t r b] [BAR l t r b]
 *                [NONAVY l t r b] [COLOUR l t r b R G B name] ...
 *                [HOURGLASS l t r b]
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
 * COLOUR    a colours.tsv sample's box: the per-channel median of its pixels
 *           (the sheet's own statistic) within 5/255 of R G B, which is the
 *           sample's colour relative to the photographed white scaled to
 *           full white: 3 for the DAC, 2 for the sheet's two decimal places
 * HOURGLASS the still's busy cursor: its black pixels (searched 16 px to
 *           either side, 4 above and below) bounded within 4 px of the box,
 *           and an hourglass (both ends at least 3/4 of its width, its
 *           waist at most 1/4), never a watch
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
/* Bounds of the black pixels in box b grown by mx across and 4 down;
 * 0 when there are none. */
static int dark_bounds_by(const int *b,int mx,int *o)
{
    int l=W,t=H,r=-1,d=-1;
    for(int y=b[1]-4;y<b[3]+4;y++) for(int x=b[0]-mx;x<b[2]+mx;x++) if(black(x,y)) {
        if(x<l) l=x;
        if(x>r) r=x;
        if(y<t) t=y;
        if(y>d) d=y;
    }
    o[0]=l; o[1]=t; o[2]=r+1; o[3]=d+1;
    return r>=0;
}
static int dark_bounds(const int *b,int *o) { return dark_bounds_by(b,4,o); }
static int cmp_byte(const void *a,const void *b) { return *(const unsigned char *)a-*(const unsigned char *)b; }
/* Per-channel median of box b, as colours.tsv takes it from the still. */
static void median(const int *b,int *out)
{
    static unsigned char ch[3][W*H];
    int n=0;
    for(int y=b[1];y<b[3];y++) for(int x=b[0];x<b[2];x++) if(x>=0&&x<W&&y>=0&&y<H) {
        for(int c=0;c<3;c++) ch[c][n]=px[y][x][c];
        n++;
    }
    for(int c=0;c<3;c++) {
        if(!n) { out[c]=-1; continue; }
        qsort(ch[c],(size_t)n,1,cmp_byte);
        out[c]=ch[c][n/2];
    }
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
    int text[4], bar[4], nonavy[4], glass[4], havetext=0, havebar=0, havenonavy=0, haveglass=0;
    for(int i=2;i<argc;) {
        if(!strcmp(argv[i],"REGION")) { for(int k=0;k<4;k++) reg[k]=num(argv,i+1+k,argc); i+=5; }
        else if(!strcmp(argv[i],"PIE")) { for(int k=0;k<4;k++) pie[k]=num(argv,i+1+k,argc); havepie=1; i+=5; }
        else if(!strcmp(argv[i],"ELLIPSE")) { for(int k=0;k<4;k++) { if(i+1+k>=argc) return 2; ell[k]=atof(argv[i+1+k]); } haveell=1; i+=5; }
        else if(!strcmp(argv[i],"SERIES")) { i++; while(i<argc&&ns<8&&(argv[i][0]=='.'||(argv[i][0]>='0'&&argv[i][0]<='9'))) ser[ns++]=atof(argv[i++]); }
        else if(!strcmp(argv[i],"FRAME")) { if(nframes==FRAMES) return 2; for(int k=0;k<4;k++) frames[nframes][k]=num(argv,i+1+k,argc); nframes++; i+=5; }
        else if(!strcmp(argv[i],"TEXT")) { for(int k=0;k<4;k++) text[k]=num(argv,i+1+k,argc); havetext=1; i+=5; }
        else if(!strcmp(argv[i],"BAR")) { for(int k=0;k<4;k++) bar[k]=num(argv,i+1+k,argc); havebar=1; i+=5; }
        else if(!strcmp(argv[i],"HOURGLASS")) { for(int k=0;k<4;k++) glass[k]=num(argv,i+1+k,argc); haveglass=1; i+=5; }
        else if(!strcmp(argv[i],"COLOUR")) {
            int b[4], want[3], got[3], off=0;
            for(int k=0;k<4;k++) b[k]=num(argv,i+1+k,argc);
            for(int k=0;k<3;k++) want[k]=num(argv,i+5+k,argc);
            const char *name=i+8<argc?argv[i+8]:"";
            median(b,got); checks++;
            for(int k=0;k<3;k++) if(got[k]<want[k]-5||got[k]>want[k]+5) off=1;
            if(off) { failures++; printf("FAIL frame pixels: %s (%d,%d)-(%d,%d) has median #%02x%02x%02x; the still's is #%02x%02x%02x relative to its white\n",name,b[0],b[1],b[2],b[3],got[0]&255,got[1]&255,got[2]&255,want[0],want[1],want[2]); }
            i+=9;
        }
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
    if(haveglass) {
        /* The canon busy cursor (CLAUDE.md, The constitution, point 2;
         * transcription CANON.HOURGLASS): an hourglass, never a wristwatch.
         * Its black pixels must sit at the still's place, and its rows must
         * be wide at both ends and narrow at the waist: a watch's face is
         * widest in the middle. */
        int o[4], off=0, seen; checks++;
        seen=dark_bounds_by(glass,16,o);   /* wide enough to see a sprite past the 4 px */
        if(seen) for(int k=0;k<4;k++) if(o[k]-glass[k]<-4||o[k]-glass[k]>4) off=1;
        if(!seen||off) { failures++; printf("FAIL frame pixels: the busy cursor spans (%d,%d)-(%d,%d); the still has the hourglass at (%d,%d)-(%d,%d)\n",o[0],o[1],o[2],o[3],glass[0],glass[1],glass[2],glass[3]); }
        else {
            int w=o[2]-o[0], h=o[3]-o[1], top=0, bottom=0, waist=w;
            for(int y=o[1];y<o[3];y++) {
                int n=0; for(int x=o[0];x<o[2];x++) n+=black(x,y);
                if(y<o[1]+h/4&&n>top) top=n;
                if(y>=o[3]-h/4&&n>bottom) bottom=n;
                if(y>=o[1]+h/3&&y<o[3]-h/3&&n<waist) waist=n;
            }
            checks++;
            if(4*top<3*w||4*bottom<3*w||4*waist>w) { failures++; printf("FAIL frame pixels: the busy cursor is not an hourglass: ends %d and %d wide, waist %d, of %d\n",top,bottom,waist,w); }
        }
    }
    printf("frame-pixels: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
