/* What InitechCalc's windows should show, pixel for pixel, worked out
 * without Calc's drawing code, against what the real tenant drew
 * (initech-dbdp3). test_calc_paint caps the gate calls from above and
 * compares a keystroke's repaint with the same code's full repaint, so a
 * renderer that draws less in both passes it; this check grades the full
 * repaint itself.
 *
 * The tenant runs over sdk/host and its gate calls are replayed into each
 * window's content (tests/replay.h). The expected pictures are built here,
 * from:
 *   - goldens/frame/transcription.tsv: what each ledger cell shows (certain
 *     and probable readings, document row = fragment row + 9, README
 *     question 7), the double rule;
 *   - goldens/frame/geometry.tsv: the pie's box on screen (PIE, PIEDEPTH),
 *     which the model below must fill exactly;
 *   - the canon: slices of value/sum of a turn, labels the values with a
 *     percent sign (40 35 18 14 9, 116%);
 *   - apps/initechcalc/README.md, "The ledger" and "The chart": the layout
 *     and the pie's specification, restated where they are used;
 *   - FLAIR's Geneva 9 strike (flair/include/assets/geneva9.h), the only
 *     face the gate draws.
 * The model is a different algorithm from Calc's: a cell-by-cell painter
 * for the ledger, and for the pie a per-pixel test in floating point with
 * the C library's sin and cos, where Calc scans integer row spans with a
 * fixed-point sine of 16-bit angles and sends cover rectangles. Where the
 * two arithmetics can honestly disagree (a pixel centre within the 16-bit
 * rounding of a slice's edge ray; a cut to whole pixels that rounding could
 * tip) the pixel may be either colour, or the cut is tried both ways; both
 * are counted and capped. Every other pixel must be exactly the model's. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "tbx_host.h"
#include "host_windows.h"
static int32_t frame_origin(int32_t w)
{
    if(w<1||w>tbx_host.nwindows) return TBX_ERR_BADARG;
    const int l=tbx_host.windows[w-1].l+6,t=tbx_host.windows[w-1].t+22;
    return (int32_t)(((uint32_t)(uint16_t)t<<16)|(uint16_t)l);
}
static int32_t frame_size(int32_t w)
{
    if(w<1||w>tbx_host.nwindows) return TBX_ERR_BADARG;
    const int cw=tbx_host.windows[w-1].r-tbx_host.windows[w-1].l-host_inset_w(w);
    const int ch=tbx_host.windows[w-1].b-tbx_host.windows[w-1].t-host_inset_h(w);
    return (int32_t)(((uint32_t)(uint16_t)ch<<16)|(uint16_t)cw);
}
#define tbx_content_origin frame_origin
#define tbx_content_size frame_size
#include "../calc.c"
#undef tbx_content_origin
#undef tbx_content_size
#include "replay.h"

enum { WHITE=TBX_COLOR_WHITE+1, BLACK=TBX_COLOR_BLACK+1, GREY=TBX_COLOR_GRAY+1, NAVY=TBX_COLOR_NAVY+1, EITHER=255 };
static int checks, failures;
static void check(int ok,const char *what)
{
    checks++;
    if(!ok) { failures++; printf("FAIL %s\n",what); }
}

/* ---- a picture, content-local, colour token + 1 --------------------------- */
typedef struct { int w, h; unsigned char px[480][640]; } pic_t;
static void fill(pic_t *p,int l,int t,int r,int b,unsigned char v)
{
    for(int y=t<0?0:t;y<b&&y<p->h;y++) for(int x=l<0?0:l;x<r&&x<p->w;x++) p->px[y][x]=v;
}
/* Geneva 9 text as the gate draws it (sdk/include/toolbox_gate.h Sec 7):
 * a box the sum of the advances wide and a cell tall, ink over it. */
static void words(pic_t *p,int x,int y,const char *s,unsigned char fg,unsigned char bg)
{
    for(;*s;s++) {
        const unsigned char *g=geneva9_glyph(*s); int adv=(int)geneva9_advance_w(*s);
        for(int r=0;r<GENEVA9_CELL_H;r++) for(int k=0;k<adv;k++) {
            int xx=x+k, yy=y+r;
            if(xx>=0&&yy>=0&&xx<p->w&&yy<p->h) p->px[yy][xx]=(k<8&&(g[r]&(0x80u>>k)))?fg:bg;
        }
        x+=adv;
    }
}
/* Compare what window w shows with the expected picture; print the first
 * few differences. Returns the number of pixels that may be either. */
static unsigned compare(const replay_screen_t *s,const pic_t *want,const char *what)
{
    unsigned bad=0, either=0; char msg[160];
    for(int y=0;y<want->h;y++) for(int x=0;x<want->w;x++) {
        unsigned char e=want->px[y][x], g=s->px[y][x];
        if(e==EITHER) { either++; if(g!=NAVY&&g!=BLACK) bad++; continue; }
        if(g!=e) {
            if(bad<8) printf("  %s: pixel %d,%d is %u, expected %u\n",what,x,y,(unsigned)g,(unsigned)e);
            bad++;
        }
    }
    snprintf(msg,sizeof msg,"%s: %u pixels differ from the expected picture",what,bad);
    check(bad==0,msg);
    return either;
}

/* ---- the reference package ------------------------------------------------ */
static char tsv_text[1<<16];
static const char *tsv_field(const char *line,int k,char *out,size_t n)
{
    const char *p=line;
    for(int i=0;i<k;i++) { p=strchr(p,'\t'); if(!p) { out[0]=0; return out; } p++; }
    size_t m=0; while(p[m]&&p[m]!='\t'&&p[m]!='\n'&&p[m]!='\r') m++;
    if(m>=n) m=n-1;
    memcpy(out,p,m); out[m]=0; return out;
}
static int read_file(const char *path,char *buf,size_t cap)
{
    FILE *f=fopen(path,"rb"); if(!f) { perror(path); return -1; }
    size_t n=fread(buf,1,cap-1,f); fclose(f); buf[n]=0; return (int)n;
}
/* geometry.tsv: the 640 x 480 box (columns 11-14) of an item. */
static int geometry(const char *path,const char *id,int box[4])
{
    static char g[1<<14];
    if(read_file(path,g,sizeof g)<0) return 0;
    for(char *line=g;line&&*line;) {
        char *nl=strchr(line,'\n'); if(nl) *nl=0;
        char f[64];
        if(!strcmp(tsv_field(line,0,f,sizeof f),id)) {
            for(int i=0;i<4;i++) box[i]=atoi(tsv_field(line,10+i,f,sizeof f));
            return 1;
        }
        line=nl?nl+1:0;
    }
    return 0;
}

/* ---- the ledger ------------------------------------------------------------
 * README "The ledger": ten columns, label 100 pixels, eight amounts of 46 and
 * a narrow one of 10; rows 12 pixels, a grey rule round every cell; text in
 * black Geneva 9 on white, 2 right of the cell's rule and 1 below it, left
 * aligned; a label runs over empty cells to its right and stops at a full
 * one; a repeating label (\=) fills the cell's interior with whole
 * characters. 26 rows: nine ruled rows stand for the ledger hidden under
 * Drive A Files and the chart, then the still's 17 fragment rows. A white
 * strip a row deep, with no column rules, lies above the total's row
 * (LED.R16.C7) with the note in it, 4 right of the edge (treatment A). The
 * cell pointer is a black box on the first cell. */
#define L_ROWS 26
#define L_ABOVE 9
#define L_TOTAL_ROW (16+L_ABOVE-1)   /* 0-based document row of the total */
static const int lx[11]={0,100,146,192,238,284,330,376,422,468,478};
static const char note_text[]="Blank entries were not recovered from the surviving material.";
static char ledger_cell[L_ROWS][10][64];
static int ledger_from_transcription(const char *path)
{
    if(read_file(path,tsv_text,sizeof tsv_text)<0) return 0;
    int n=0;
    for(char *line=tsv_text;line&&*line;) {
        char *nl=strchr(line,'\n'); if(nl) *nl=0;
        char id[32], reading[64], conf[16];
        tsv_field(line,0,id,sizeof id); tsv_field(line,3,reading,sizeof reading); tsv_field(line,4,conf,sizeof conf);
        if(strlen(id)==10&&!strncmp(id,"LED.R",5)&&id[7]=='.'&&id[8]=='C'&&reading[0]&&(!strcmp(conf,"certain")||!strcmp(conf,"probable"))) {
            int row=(id[5]-'0')*10+(id[6]-'0')+L_ABOVE-1, col=id[9]=='L'?0:id[9]-'0';
            if(row<0||row>=L_ROWS||col<0||col>9) return 0;
            if(!strcmp(reading,"(double rule)")) {   /* \= across the 44-pixel interior */
                int k=0, room=lx[col+1]-lx[col]-2;
                while((k+1)*(int)geneva9_advance_w('=')<=room) ledger_cell[row][col][k++]='=';
                ledger_cell[row][col][k]=0;
            } else snprintf(ledger_cell[row][col],sizeof ledger_cell[row][col],"%s",reading);
            n++;
        }
        line=nl?nl+1:0;
    }
    return n;
}
static int ledger_row_y(int r) { return r*12+(r>=L_TOTAL_ROW?12:0); }
static void model_ledger(pic_t *p)
{
    const int strip=L_TOTAL_ROW*12, bottom=ledger_row_y(L_ROWS-1)+12;
    fill(p,0,0,p->w,p->h,WHITE);
    for(int k=0;k<=10;k++) { fill(p,lx[k],0,lx[k]+1,strip+1,GREY); fill(p,lx[k],strip+12,lx[k]+1,bottom+1,GREY); }
    for(int r=0;r<L_ROWS;r++) {   /* cell by cell, left to right */
        int y=ledger_row_y(r), earlier=0;
        for(int c=0;c<10;c++) {
            if(!ledger_cell[r][c][0]) continue;
            if(earlier) { fill(p,lx[c]+1,y+1,lx[c+1],y+12,WHITE); fill(p,lx[c],y,lx[c]+1,y+13,GREY); }
            words(p,lx[c]+2,y+1,ledger_cell[r][c],BLACK,WHITE);
            earlier=1;
        }
    }
    for(int r=0;r<L_ROWS;r++) fill(p,0,ledger_row_y(r),lx[10]+1,ledger_row_y(r)+1,GREY);
    fill(p,0,bottom,lx[10]+1,bottom+1,GREY);
    fill(p,0,strip,lx[10]+1,strip+1,GREY);
    words(p,4,strip+1,note_text,BLACK,WHITE);
    fill(p,0,0,lx[1]+1,1,BLACK); fill(p,0,12,lx[1]+1,13,BLACK);
    fill(p,0,0,1,13,BLACK); fill(p,lx[1],0,lx[1]+1,13,BLACK);
}

/* ---- the chart -------------------------------------------------------------
 * README "The chart": on a black field, an elliptical disk of slices, each
 * value/sum of a turn clockwise from 12 o'clock. In the chart's content
 * pixels the disk's centre is (124,70) and its top face the pixel centres
 * with (x/96)^2 + (y/55)^2 <= 1. Each slice is pushed out from the centre
 * along its middle by 6 pixels across and 6*55/96 down the screen, each cut
 * to whole pixels toward zero. Its side is its face swept 42 rows down, navy
 * on even content rows and black on odd ones; the row below each face pixel
 * that is not face is a black edge; the face is navy. A slice of more than
 * half a turn is two halves, each swept on its own. Slices are drawn from the
 * back (middle nearest 12 o'clock) to the front. Each label, the value and a
 * percent sign in white on navy, has its top-left corner 5 rows above and 3
 * pixels per character left of the point 59 x 34 pixels (62% of the axes, cut
 * to whole pixels) out along the slice's middle from its pushed-out centre.
 * The model's own reach must be geometry.tsv's PIE and PIEDEPTH box: the
 * place on screen comes from the still. */
#define P_CX 124
#define P_CY 70
#define P_A 96
#define P_B 55
#define P_D 42
#define P_X 6
#define P_LX 59
#define P_LY 34
static const double TURN=6.283185307179586;
/* Calc keeps an angle to 1/65536 of a turn and halves two of them for a
 * slice's middle (README "The chart"), so its rays and middles may be up to
 * 1e-4 radians from the exact ones; its sine is far closer than that. At a
 * whole quarter turn both are exact. Sines here are of turn fractions. */
#define SLACK 2e-4
static double quarter_turns(double f) { double q=f*4; return fabs(q-nearbyint(q))<1e-12?nearbyint(q):-1; }
static double tsin(double f)
{
    double q=quarter_turns(f);
    if(q>=0) { static const double v[4]={0,1,0,-1}; return v[(long)q%4]; }
    return sin(f*TURN);
}
static double tcos(double f) { return tsin(f+0.25); }
static double slack(double f) { return quarter_turns(f)>=0?0:SLACK; }
/* A length times a sine or cosine of the angle f, cut toward zero. Within
 * the slack of a whole pixel other than 0 the cut could honestly go either
 * way: the n-th such cut of a model takes bit n of cut_choice (0: the exact
 * value's cut, 1: the whole pixel on the other side). */
static unsigned cut_choice, cut_count;
static int cut(double len,double trig,double f)
{
    double v=len*trig, r=nearbyint(v); int t=(int)v;
    if(r==0||!(fabs(v-r)<fabs(len)*slack(f))) return t;
    int alt=t==(int)r?(int)r-(r>0?1:-1):(int)r;
    return (cut_choice>>cut_count++)&1u?alt:t;
}
typedef struct { double s, e; int ox, oy; } piece_t;
/* 1 inside, 0 outside, 2 too near an edge ray to call. Inside the ellipse,
 * clockwise of the ray at s and short of the ray at e. */
static int in_face(const piece_t *q,int x,int y)
{
    if((long)x*x*P_B*P_B+(long)y*y*P_A*P_A>(long)P_A*P_A*P_B*P_B) return 0;
    double reach=fabs((double)y)*P_A+fabs((double)x)*P_B, ts=slack(q->s)*reach, te=slack(q->e)*reach;
    double vs=tsin(q->s)*y*P_A+tcos(q->s)*x*P_B, ve=tsin(q->e)*y*P_A+tcos(q->e)*x*P_B;
    if(vs<-ts||ve>=te) return 0;
    if(vs<ts||ve>-te) return 2;
    return 1;
}
/* Paint one piece; `take` decides the pixels too near a ray (0 or 1). */
static void model_piece(pic_t *p,const piece_t *q,int take)
{
    static signed char face[2*P_B+1][2*P_A+1];
    int lo[2*P_B+1], hi[2*P_B+1];
    for(int y=-P_B;y<=P_B;y++) {
        lo[y+P_B]=1000; hi[y+P_B]=-1000;
        for(int x=-P_A;x<=P_A;x++) {
            int f=in_face(q,x,y); if(f==2) f=take;
            face[y+P_B][x+P_A]=(signed char)f;
            if(f) { if(x<lo[y+P_B]) lo[y+P_B]=x; if(x>hi[y+P_B]) hi[y+P_B]=x; }
        }
    }
    int cx=P_CX+q->ox, cy=P_CY+q->oy;
    for(int y=-P_B;y<=P_B+P_D;y++) {   /* the side */
        int l=1000, r=-1000;
        for(int t=y-P_D;t<=y;t++) if(t>=-P_B&&t<=P_B&&lo[t+P_B]<=hi[t+P_B]) {
            if(lo[t+P_B]<l) l=lo[t+P_B];
            if(hi[t+P_B]>r) r=hi[t+P_B];
        }
        if(l<=r) fill(p,cx+l,cy+y,cx+r+1,cy+y+1,((cy+y)&1)?BLACK:NAVY);
    }
    for(int y=-P_B;y<=P_B;y++) for(int x=-P_A;x<=P_A;x++)   /* the edge */
        if(face[y+P_B][x+P_A]&&!(y<P_B&&face[y+1+P_B][x+P_A])) fill(p,cx+x,cy+y+1,cx+x+1,cy+y+2,BLACK);
    for(int y=-P_B;y<=P_B;y++) for(int x=-P_A;x<=P_A;x++)   /* the face */
        if(face[y+P_B][x+P_A]) fill(p,cx+x,cy+y,cx+x+1,cy+y+1,NAVY);
}
typedef struct { double v[8]; char label[8][24]; int n; } series_t;
static void model_chart(pic_t *p,const series_t *sr,int take)
{
    double sum=0, at[9]; int ox[8], oy[8], lx[8], ly[8];
    fill(p,0,0,p->w,p->h,BLACK);
    cut_count=0;
    for(int i=0;i<sr->n;i++) sum+=sr->v[i];
    at[0]=0; for(int i=0;i<sr->n;i++) at[i+1]=at[i]+sr->v[i]/sum;
    for(int i=0;i<sr->n;i++) {
        double m=(at[i]+at[i+1])/2;
        ox[i]=cut(P_X,tsin(m),m); oy[i]=cut(-(double)P_X*P_B/P_A,tcos(m),m);
        lx[i]=cut(P_LX,tsin(m),m); ly[i]=cut(P_LY,tcos(m),m);
    }
    int order[8]; for(int i=0;i<sr->n;i++) order[i]=i;
    for(int i=1;i<sr->n;i++) for(int k=i;k>0;k--) {   /* back to front, ties kept in series order */
        double ca=tcos((at[order[k-1]]+at[order[k-1]+1])/2), cb=tcos((at[order[k]]+at[order[k]+1])/2);
        if(cb>ca+1e-9) { int t=order[k]; order[k]=order[k-1]; order[k-1]=t; } else break;
    }
    for(int k=0;k<sr->n;k++) {
        int i=order[k]; double s=at[i], e=at[i+1], m=(s+e)/2;
        piece_t q={s,e,ox[i],oy[i]};
        if(e-s>0.5) { piece_t h=q; h.e=m; model_piece(p,&h,take); h.s=m; h.e=e; model_piece(p,&h,take); }
        else model_piece(p,&q,take);
    }
    for(int i=0;i<sr->n;i++)
        words(p,P_CX+ox[i]+lx[i]-3*(int)strlen(sr->label[i]),P_CY+oy[i]-ly[i]-5,sr->label[i],WHITE,NAVY);
}

/* ---- running the tenant --------------------------------------------------- */
static replay_screen_t shown[2];
static int run(const char *docbytes,size_t n,pic_t *lw,pic_t *cw)
{
    tbx_host_reset();
    if(!tbx_host_put_file("APPS\\LEDGER.TXT",docbytes,(uint32_t)n)||tbx_app_main()!=0) return 0;
    for(unsigned i=0;i<tbx_host.nops;i++) if(!strcmp(tbx_host.ops[i].name,"tbx_alert")) return 0;
    tbx_host.nops=0;
    tbx_event_t e={updateEvt,0,0,0,0,0};
    if(!tbx_host_push_event(&e)) return 0;
    (void)tbx_host_dispatch(tbx_app_event);
    if(tbx_host.dropped_ops) return 0;
    memset(shown,0,sizeof shown);
    for(int w=1;w<=2;w++) (void)replay(&shown[w-1],w,0);
    pic_t *pics[2]={lw,cw};
    for(int w=1;w<=2;w++) {
        int l,t,ww,hh; replay_content(w,&l,&t,&ww,&hh);
        pics[w-1]->w=ww<640?ww:640; pics[w-1]->h=hh<480?hh:480;
    }
    return 1;
}
/* The expected chart: both decisions for the pixels too near a ray, and
 * where they differ either colour is right; for the cuts that could go
 * either way, the combination that matches what is shown, if one does.
 * Returns the number of pixels on a ray; *cuts is the number of open cuts. */
static unsigned differences(const replay_screen_t *s,const pic_t *want)
{
    unsigned bad=0;
    for(int y=0;y<want->h;y++) for(int x=0;x<want->w;x++) {
        unsigned char e=want->px[y][x], g=s->px[y][x];
        bad+=e==EITHER?(g!=NAVY&&g!=BLACK):g!=e;
    }
    return bad;
}
static unsigned expected_chart_as(pic_t *want,const series_t *sr,unsigned choice)
{
    static pic_t other;
    other.w=want->w; other.h=want->h; cut_choice=choice;
    model_chart(want,sr,1); model_chart(&other,sr,0);
    unsigned n=0;
    for(int y=0;y<want->h;y++) for(int x=0;x<want->w;x++)
        if(want->px[y][x]!=other.px[y][x]) { want->px[y][x]=EITHER; n++; }
    return n;
}
static unsigned expected_chart(pic_t *want,const series_t *sr,const replay_screen_t *s,unsigned *cuts)
{
    unsigned best=0, fewest=~0u;
    (void)expected_chart_as(want,sr,0);
    *cuts=cut_count;
    if(*cuts>4) return ~0u;
    for(unsigned c=0;c<1u<<*cuts;c++) {
        (void)expected_chart_as(want,sr,c);
        unsigned d=differences(s,want);
        if(d<fewest) { fewest=d; best=c; }
    }
    return expected_chart_as(want,sr,best);
}
static pic_t want_l, want_c;
static void chart_case(const char *name,const char *series_line,const series_t *sr)
{
    char doc[512], what[96];
    int n=snprintf(doc,sizeof doc,"INITECH LEDGER\r\nWINDOW\t116\t104\t622\t474\r\nCHART\t349\t72\t598\t288\r\n%s\r\nROWS\t1\r\n",series_line);
    snprintf(what,sizeof what,"%s: the tenant opens it",name);
    if(!run(doc,(size_t)n,&want_l,&want_c)) { check(0,what); return; }
    check(1,what);
    unsigned cuts, either=expected_chart(&want_c,sr,&shown[1],&cuts);
    snprintf(what,sizeof what,"%s: at most 4 cuts to whole pixels could go either way (%u)",name,cuts);
    check(cuts<=4,what);
    snprintf(what,sizeof what,"%s: at most 16 pixels sit on a ray (%u)",name,either);
    check(either<=16,what);
    snprintf(what,sizeof what,"%s chart",name);
    compare(&shown[1],&want_c,what);
}

int main(void)
{
    const char *tsv=getenv("FRAME_TSV"), *geom=getenv("FRAME_GEOM");
    if(!tsv||!geom) { puts("set FRAME_TSV and FRAME_GEOM to goldens/frame"); return 2; }
    static char buf[65536];
    int n=read_file("data/LEDGER.TXT",buf,sizeof buf);
    if(n<0) return 2;
    check(ledger_from_transcription(tsv)>0,"transcription.tsv has ledger readings");

    /* The shipped ledger: both windows. */
    if(!run(buf,(size_t)n,&want_l,&want_c)) { puts("FAIL the shipped ledger does not open"); return 1; }
    model_ledger(&want_l);
    compare(&shown[0],&want_l,"shipped ledger");
    series_t frame={{40,35,18,14,9},{"40%","35%","18%","14%","9%"},5};
    unsigned cuts, either=expected_chart(&want_c,&frame,&shown[1],&cuts);
    printf("frame chart: %u pixels on a ray may be either colour, %u cuts either way\n",either,cuts);
    check(cuts<=4,"frame chart: at most 4 cuts to whole pixels could go either way");
    check(either<=16,"frame chart: at most 16 pixels sit on a ray");
    /* The model's reach is the still's pie: geometry.tsv PIE across and
     * from the top, PIEDEPTH's foot, on screen through the chart's content
     * origin as the window was opened. */
    {   int pie[4], depth[4], l=640, t=480, r=-1, b=-1, ox=tbx_host.windows[1].l+6, oy=tbx_host.windows[1].t+22;
        for(int y=0;y<want_c.h;y++) for(int x=0;x<want_c.w;x++) if(want_c.px[y][x]==NAVY) {
            if(x<l) l=x;
            if(x>r) r=x;
            if(y<t) t=y;
            if(y>b) b=y;
        }
        char what[160];
        int ok=geometry(geom,"PIE",pie)&&geometry(geom,"PIEDEPTH",depth);
        snprintf(what,sizeof what,"the model's pie spans %d,%d to %d,%d on screen; geometry.tsv PIE/PIEDEPTH %d,%d to %d,%d",
                 l+ox,t+oy,r+1+ox,b+1+oy,pie[0],pie[1],pie[2],depth[3]);
        check(ok&&l+ox==pie[0]&&t+oy==pie[1]&&r+1+ox==pie[2]&&b+1+oy==depth[3],what);
    }
    compare(&shown[1],&want_c,"frame chart");

    /* Other series: one slice (a whole turn, two halves), eight equal
     * slices (ties in the drawing order), a sliver, no chart at all. */
    series_t one={{5},{"5%"},1};
    chart_case("one slice","SERIES\t5",&one);
    series_t eight={{1,1,1,1,1,1,1,1},{"1%","1%","1%","1%","1%","1%","1%","1%"},8};
    chart_case("eight slices","SERIES\t1\t1\t1\t1\t1\t1\t1\t1",&eight);
    series_t sliver={{1,999},{"1%","999%"},2};
    chart_case("a sliver","SERIES\t1\t999",&sliver);
    series_t none={{0},{""},0};
    chart_case("no series","SERIES",&none);

    printf("test_calc_picture: %d checks, %d failures\n",checks,failures);
    return failures!=0;
}
