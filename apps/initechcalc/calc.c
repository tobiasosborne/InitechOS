/* CALC.EXE -- InitechCalc: the frame's ledger and its pie chart, PRD 6.5 /
 * Appendix A. A disk tenant of the Initech 123 cell engine.
 *
 * The ledger is a document (README.md "The ledger document"): APPS\TABLES.TXT
 * when the user has saved one, else the shipped APPS\LEDGER.TXT, whose cells
 * are the transcription of the surviving still (goldens/frame). It carries
 * both windows' positions, the chart's series and the source note, so the
 * frame's arrangement is ordinary saved state. Cells the still does not show
 * are empty, never invented (operator ruling, 2026-11-14).
 *
 * The chart is drawn from the document's series by value/sum, so the pie
 * closes; each label is its value with a percent sign, so the labels of the
 * frame's series really sum to 116 (CLAUDE.md, The constitution, point 2).
 * Negative numbers print with a trailing minus, in every format: 570-.
 * Freestanding C, ASCII, deterministic. */
#include "tbx.h"
#include "i123.h"
#include "i123/wk1_format.h"
#include "calc_ledger.h"
const char tbx_app_name[]="CALC";
static const tbx_menuitem_t file_items[]={
    {"Save",0,'S',0,1,0,{0,0,0}}, /* the FILE COPY box (PRD Appendix A) */
    {"Quit",0,'Q',0,1,0,{0,0,0}}
};
static const tbx_menuinfo_t menus[]={{129,0,"File",file_items,2,0}};
static const tbx_menubar_t mbar={menus,1,1,0};
static const char copy_caption[]=CALC_COPY_CAPTION;
static const char shipped[]="APPS\\LEDGER.TXT";
static const char copy_dst[]="APPS\\TABLES.TXT";
/* Stage names, tried in DOS order; an occupied name is never touched. */
static const char *const stage_names[]={"APPS\\TABLES.TMP","APPS\\TABLE~1.TMP","APPS\\TABLE~2.TMP","APPS\\TABLE~3.TMP"};
static const char *copy_stage;
static const char quit_text[]="Save changes to the worksheet\r\"TABLES.TXT\"\rbefore quitting?";
static const char replace_text[]="\"TABLES.TXT\" already exists.\rDo you want to replace it with the\rworksheet you are saving?";
static const char fail_text[]="The worksheet could not be saved.";
static const char unread_text[]="\"TABLES.TXT\" could not be read.\rThe original ledger was opened.";
static const char lost_text[]="The ledger could not be read.";
enum { ASK_NONE, ASK_QUIT, ASK_REPLACE, ASK_NOTICE };

/* ---- the ledger's layout (content pixels): label column, eight amount
 * columns and a narrow one, as in the still (goldens/frame/geometry.tsv,
 * LEDGRID), at the Geneva 9 cell height. ---------------------------------- */
#define CALC_COLS 10
#define CALC_ROW_MAX 26
#define ROW_H 12
static const int colx[CALC_COLS+1]={0,100,146,192,238,284,330,376,422,468,478};
#define FMT_GENERAL ((uint8_t)((WK1_FT_SPECIAL<<4)|WK1_FS_GENERAL))
#define SERIES_MAX 8
#define NOTE_MAX 79
/* Window bounds used when a document gives none it can use. */
static const int32_t home[2][4]={{116,104,622,474},{349,72,598,288}};

static uint8_t store[40960];
static i123_sheet_t g_sheet;
static int32_t win[2];
#define ledger win[0]
#define chart win[1]
static int32_t bounds[2][4];
static double series[SERIES_MAX];
static unsigned nseries, g_rows;
static char note[NOTE_MAX+1] TBX_IN_IMAGE={0};
static char line[96] TBX_IN_IMAGE={0};
static char entry[64] TBX_IN_IMAGE={0};
static unsigned entered;
static uint8_t g_dirty, g_ask, g_quit_after_save, g_copying, g_staged;
static char doc[20480];
static char stage[20480];

static unsigned length(const char *s) { unsigned n=0; while(s[n]) n++; return n; }
static void copy(char *d,const char *s) { do { *d++=*s; } while(*s++); }
static int same(const char *a,const char *b,unsigned n) { for(unsigned i=0;i<n;i++) if(a[i]!=b[i]) return 0; return 1; }

/* ---- numbers ------------------------------------------------------------ */
/* The only Calc format adaptation: Initech 123's number text with the sign
 * moved to the tail, in every format (PRD Appendix A: 570-). */
static void calc_format(double x,uint8_t fmt,char *out)
{
    char number[48]; double a=x<0?-x:x;
    unsigned t=WK1_FMT_TYPE(fmt);
    if(t==WK1_FT_FIXED||t==WK1_FT_CURRENCY) {
        i123_format_number(a,(uint8_t)(fmt&0x7F),24,number);
        unsigned s=0, n=length(number); while(number[s]==' ') s++;
        while(n>s&&number[n-1]==' ') number[--n]=0;
        copy(out,number+s);
    } else { i123_number_text(a,number,sizeof number); copy(out,number); }
    if(x<0) { unsigned n=length(out); out[n]='-'; out[n+1]=0; }
}
/* A typed or saved number: digits with an optional point and exponent, a
 * leading or a trailing minus, and for currency a dollar sign and thousands
 * commas. The exponent (1E-7, 1.5E+21) is Initech 123's typed-value syntax
 * (i123.h, i123_parse_value) and the form i123_number_text writes, so every
 * number Calc saves reads back. */
static int calc_parse(const char *s,unsigned n,double *x)
{
    char t[48]; unsigned k=0; int neg=0, digits=0, expo=0;
    if(n&&s[0]=='-') { neg=1; s++; n--; }
    else if(n&&s[n-1]=='-') { neg=1; n--; }
    if(n&&s[0]=='$') { s++; n--; }
    for(unsigned i=0;i<n;i++) {
        char c=s[i];
        if(c==','&&!expo) { if(!digits) return 0; continue; }
        if(k>=sizeof t-1) return 0;
        if(c=='E'||c=='e') { if(!digits||expo) return 0; expo=1; }
        else if(c=='+'||c=='-') { if(!k||(t[k-1]!='E'&&t[k-1]!='e')) return 0; }
        else if(c>='0'&&c<='9') { if(!expo) digits++; }
        else if(c!='.'||expo) return 0;
        t[k++]=c;
    }
    if(!digits||i123_parse_value(t,k,x)!=I123_OK) return 0;
    if(neg) *x=-*x;
    return 1;
}
static uint8_t cell_fmt(const i123_cell_t *p)
{
    unsigned t=WK1_FMT_TYPE(p->fmt);
    return (t==WK1_FT_FIXED||t==WK1_FT_CURRENCY)?(uint8_t)(p->fmt&0x7F):FMT_GENERAL;
}

/* ---- cells -------------------------------------------------------------- */
/* What a cell shows: its label, its formatted number or nothing. A repeating
 * label (Lotus '\') fills `width` pixels with its character. */
static int cell_text(unsigned c,unsigned r,char *out,unsigned width)
{
    const i123_cell_t *p=i123_find(&g_sheet,(uint16_t)c,(uint16_t)r);
    out[0]=0;
    if(!p) return 0;
    if(p->kind==I123_K_LABEL) {
        const char *b=(const char *)i123_cell_bytes(&g_sheet,p);
        unsigned n=p->len>1?p->len-1u:0u;
        if(n>79) n=79;
        if(b[0]=='\\') {
            unsigned k=0, m=width/6u; if(m>79) m=79;
            while(n&&k<m) { out[k]=b[1+k%n]; k++; }
            out[k]=0;
        } else { for(unsigned i=0;i<n;i++) out[i]=b[1+i]; out[n]=0; }
        return out[0]!=0;
    }
    if(p->kind==I123_K_NUMBER||p->kind==I123_K_INTEGER) {
        double x; char raw[48];
        i123_cell_content(&g_sheet,p,raw,sizeof raw);
        if(i123_parse_value(raw,length(raw),&x)!=I123_OK) return 0;
        calc_format(x,cell_fmt(p),out); return 1;
    }
    return 0;
}
static int put_number(unsigned c,unsigned r,double x,uint8_t fmt)
{
    if(i123_set_number(&g_sheet,(uint16_t)c,(uint16_t)r,x)!=I123_OK) return 0;
    return i123_set_cell_format(&g_sheet,(uint16_t)c,(uint16_t)r,fmt)==I123_OK;
}

/* ---- the document -------------------------------------------------------- */
/* Read one tab-separated field of [p,end); returns its end. */
static const char *field(const char *p,const char *end,const char **f,unsigned *n)
{
    *f=p; while(p<end&&*p!='\t') p++;
    *n=(unsigned)(p-*f); return p<end?p+1:p;
}
static int number_field(const char *f,unsigned n,int32_t *v)
{
    int32_t x=0; if(!n||n>5) return 0;
    for(unsigned i=0;i<n;i++) { if(f[i]<'0'||f[i]>'9') return 0; x=x*10+(f[i]-'0'); }
    *v=x; return 1;
}
/* One saved cell: 'label, \repeat, or (G) / (Fd) / (Cd) and a number. */
static int load_cell(unsigned c,unsigned r,const char *f,unsigned n)
{
    if(!n) return 1;
    if(f[0]=='\''||f[0]=='\\') {
        if(n<2||n>64) return 0;
        for(unsigned i=0;i<n;i++) if((unsigned char)f[i]<32||(unsigned char)f[i]>126) return 0;
        return i123_set_label(&g_sheet,(uint16_t)c,(uint16_t)r,(uint8_t)f[0],f+1,n-1)==I123_OK;
    }
    uint8_t fmt; unsigned k;
    if(n>4&&same(f,"(G) ",4)) { fmt=FMT_GENERAL; k=4; }
    else if(n>5&&f[0]=='('&&(f[1]=='F'||f[1]=='C')&&f[2]>='0'&&f[2]<='9'&&same(f+3,") ",2)) {
        fmt=(uint8_t)(((f[1]=='F'?WK1_FT_FIXED:WK1_FT_CURRENCY)<<4)|(unsigned)(f[2]-'0')); k=5;
    } else return 0;
    double x; if(!calc_parse(f+k,n-k,&x)) return 0;
    return put_number(c,r,x,fmt);
}
static int load_bounds(int32_t *b,const char *p,const char *end)
{
    const char *f; unsigned n;
    for(int i=0;i<4;i++) { p=field(p,end,&f,&n); if(!number_field(f,n,&b[i])) return 0; }
    return p==end;
}
/* Fit saved bounds to the screen without changing their size. */
static void fit(int32_t *b,const int32_t *fallback)
{
    int32_t w=b[2]-b[0], h=b[3]-b[1];
    if(w<96||w>640||h<64||h>460) { for(int i=0;i<4;i++) b[i]=fallback[i]; return; }
    if(b[0]<0) b[0]=0;
    if(b[0]+w>640) b[0]=640-w;
    if(b[1]<20) b[1]=20;
    if(b[1]+h>480) b[1]=480-h;
    b[2]=b[0]+w; b[3]=b[1]+h;
}
/* The empty model: a blank ledger of full size, no chart, the windows in
 * their home places. It is an ordinary document: what is typed into it saves
 * and reads back. */
static void empty_model(void)
{
    i123_sheet_clear(&g_sheet); nseries=0; g_rows=CALC_ROW_MAX; note[0]=0;
    for(int w=0;w<2;w++) for(int i=0;i<4;i++) bounds[w][i]=home[w][i];
}
/* Parse doc[0,len) into the model. On any error the model is left empty and
 * the caller decides what to open instead; a document is never half-loaded. */
static int parse(unsigned len)
{
    const char *p=doc, *end=doc+len; unsigned seen=0, last_row=0;
    empty_model();
    unsigned first=1;
    while(p<end) {
        const char *e=p; while(e<end&&*e!='\r') e++;
        if(e+1>=end||e[1]!='\n') goto bad;
        const char *f; unsigned n; const char *q=field(p,e,&f,&n);
        if(first) { if(!(n==14&&same(f,"INITECH LEDGER",14)&&q==e)) goto bad; first=0; }
        else if(n==6&&same(f,"WINDOW",6)) { if((seen&1)||!load_bounds(bounds[0],q,e)) goto bad; seen|=1; }
        else if(n==5&&same(f,"CHART",5)) { if((seen&2)||!load_bounds(bounds[1],q,e)) goto bad; seen|=2; }
        else if(n==6&&same(f,"SERIES",6)) {
            if(seen&4) goto bad;
            while(q<e) {
                q=field(q,e,&f,&n); double x;
                if(nseries==SERIES_MAX||!calc_parse(f,n,&x)||x<=0) goto bad;
                series[nseries++]=x;
            }
            seen|=4;   /* no values: the chart is empty */
        }
        else if(n==4&&same(f,"NOTE",4)) {
            unsigned k=(unsigned)(e-q); if((seen&8)||k>NOTE_MAX) goto bad;
            for(unsigned i=0;i<k;i++) { if((unsigned char)q[i]<32||(unsigned char)q[i]>126) goto bad; note[i]=q[i]; }
            note[k]=0; seen|=8;
        }
        else if(n==4&&same(f,"ROWS",4)) {
            int32_t v; if((seen&16)||(seen&32)) goto bad;
            f=q; n=(unsigned)(e-q);
            if(!number_field(f,n,&v)||v<1||v>CALC_ROW_MAX) goto bad;
            g_rows=(unsigned)v; seen|=16;
        } else {
            int32_t r; if(!(seen&16)||!number_field(f,n,&r)||r<1||(unsigned)r>g_rows||(unsigned)r<=last_row) goto bad;
            last_row=(unsigned)r; seen|=32;
            unsigned c=0, any=0;
            while(q<e) {
                if(c==CALC_COLS) goto bad;
                q=field(q,e,&f,&n);
                if(!load_cell(c,(unsigned)r-1,f,n)) goto bad;
                any|=n; c++;
            }
            if(!any) goto bad;
        }
        p=e+2;
    }
    if(first||!(seen&16)||!(seen&4)) goto bad;
    fit(bounds[0],home[0]); fit(bounds[1],home[1]);
    return 1;
bad:
    empty_model();
    return 0;
}
/* 1 loaded, 0 absent, -1 present but unreadable. */
static int load(const char *path)
{
    int32_t h=tbx_file_open(path,TBX_FILE_READ_ONLY);
    if(h<0) return h==TBX_ERR_DOS(2)?0:-1;
    int32_t n=tbx_file_read(h,doc,sizeof doc);
    int32_t more=n>=0&&(uint32_t)n==sizeof doc?tbx_file_read(h,stage,1):0;
    int32_t closed=tbx_file_close(h);
    if(n<0||more!=0||closed<0) return -1;
    return parse((unsigned)n)?1:-1;
}

/* Serialise the model in the same layout. A full stage refuses the save
 * rather than writing a shortened document. */
static unsigned put_at;
static int put_full;
static void put(const char *s) { while(*s) { if(put_at>=sizeof stage) { put_full=1; return; } stage[put_at++]=*s++; } }
static void put_int(int32_t v)
{
    char t[12]; int k=0; if(v<0) { put("-"); v=-v; }
    do { t[k++]=(char)('0'+v%10); v/=10; } while(v);
    char s[2]={0,0}; while(k) { s[0]=t[--k]; put(s); }
}
/* A number as Calc reads it, sign at the tail. One that would not read back
 * as itself refuses the save: a successful Save never publishes a document
 * Calc cannot reopen. */
static void put_value(double x)
{
    char t[48]; double back; unsigned n;
    i123_number_text(x<0?-x:x,t,sizeof t-1); n=length(t);
    if(x<0) { t[n++]='-'; t[n]=0; }
    if(!calc_parse(t,n,&back)||back!=x) { put_full=1; return; }
    put(t);
}
/* The bounds a window has now: its content rect less the document chrome
 * (CalcDocContentRect: 6 left, 22 top, 21 right, 21 bottom). */
static void current_bounds(int w)
{
    int32_t o=tbx_content_origin(win[w]), s=tbx_content_size(win[w]);
    if(o<0||s<=0) return;   /* collapsed or unknown: keep what we had */
    int32_t l=(o&0xFFFF)-6, t=((o>>16)&0xFFFF)-22;
    int32_t r=l+(s&0xFFFF)+27, b=t+((s>>16)&0xFFFF)+43;
    int32_t nb[4]={l,t,r,b}; fit(nb,bounds[w]);
    for(int i=0;i<4;i++) bounds[w][i]=nb[i];
}
static unsigned serialise(void)
{
    put_at=0; put_full=0;
    put("INITECH LEDGER\r\n");
    for(int w=0;w<2;w++) {
        current_bounds(w);
        put(w?"CHART":"WINDOW");
        for(int i=0;i<4;i++) { put("\t"); put_int(bounds[w][i]); }
        put("\r\n");
    }
    put("SERIES");
    for(unsigned i=0;i<nseries;i++) { put("\t"); put_value(series[i]); }
    put("\r\n");
    if(note[0]) { put("NOTE\t"); put(note); put("\r\n"); }
    put("ROWS\t"); put_int((int32_t)g_rows); put("\r\n");
    for(unsigned r=0;r<g_rows;r++) {
        unsigned lastc=CALC_COLS;
        for(unsigned c=0;c<CALC_COLS;c++) {
            const i123_cell_t *p=i123_find(&g_sheet,(uint16_t)c,(uint16_t)r);
            if(p&&(p->kind==I123_K_LABEL||p->kind==I123_K_NUMBER||p->kind==I123_K_INTEGER)) lastc=c;
        }
        if(lastc==CALC_COLS) continue;
        put_int((int32_t)r+1);
        for(unsigned c=0;c<=lastc;c++) {
            const i123_cell_t *p=i123_find(&g_sheet,(uint16_t)c,(uint16_t)r);
            put("\t");
            if(!p) continue;
            if(p->kind==I123_K_LABEL) {
                const char *b=(const char *)i123_cell_bytes(&g_sheet,p);
                char t[2]={0,0};
                for(unsigned i=0;i<p->len;i++) { t[0]=b[i]; put(t); }
            } else if(p->kind==I123_K_NUMBER||p->kind==I123_K_INTEGER) {
                char raw[48]; double x; uint8_t fmt=cell_fmt(p);
                i123_cell_content(&g_sheet,p,raw,sizeof raw);
                if(i123_parse_value(raw,length(raw),&x)!=I123_OK) { put_full=1; continue; }
                if(fmt==FMT_GENERAL) put("(G) ");
                else { char t[6]={'(',WK1_FMT_TYPE(fmt)==WK1_FT_FIXED?'F':'C',(char)('0'+(fmt&15)),')',' ',0}; put(t); }
                put_value(x);
            }
        }
        put("\r\n");
    }
    return put_full?0:put_at;
}

/* ---- painting ------------------------------------------------------------ */
static void text(int32_t w,int x,int y,const char *s,uint32_t fg,uint32_t bg)
{ (void)tbx_draw_text(w,x,y,s,fg|TBX_TEXT_GENEVA9,bg); }
static void rect(int32_t w,int l,int t,int r,int b,uint32_t c) { if(l<r&&t<b) (void)tbx_fill_rect(w,l,t,r,b,c); }
/* Treatment A (operator ruling 2026-11-14, design-frame section 3): reserve
 * one white annotation strip before the final total and its rule. It is
 * layout, never a worksheet row or a fabricated cell value. */
static unsigned note_row(void) { return g_rows>1?g_rows-2:0; }
static int row_y(unsigned r)
{ return (int)r*ROW_H+(note[0]&&r>=note_row()?ROW_H:0); }
static void paint_cell(unsigned c,unsigned r)
{
    int x=colx[c], y=row_y(r);
    int editing=entered&&c==i123_cursor_col(&g_sheet)&&r==i123_cursor_row(&g_sheet);
    if(editing) copy(line,entry);
    else if(!cell_text(c,r,line,(unsigned)(colx[c+1]-x-2))) return;   /* empty: a left neighbour may run over it */
    rect(ledger,x+1,y+1,colx[c+1],y+ROW_H,TBX_COLOR_WHITE);            /* stop the left neighbour here */
    rect(ledger,x,y,x+1,y+ROW_H+1,TBX_COLOR_GRAY);
    text(ledger,x+2,y+1,line,TBX_COLOR_BLACK,TBX_COLOR_WHITE);
}
static void paint_ledger(void)
{
    int32_t s=tbx_content_size(ledger);
    int cw=s&0xFFFF, ch=(s>>16)&0xFFFF, gw=colx[CALC_COLS], gh=row_y(g_rows);
    int ny=(int)note_row()*ROW_H;
    if(s<=0) return;
    rect(ledger,0,0,cw,ch,TBX_COLOR_WHITE);
    if(note[0]) rect(ledger,0,ny,gw+1,ny+1,TBX_COLOR_GRAY);
    for(unsigned r=0;r<=g_rows;r++) rect(ledger,0,row_y(r),gw+1,row_y(r)+1,TBX_COLOR_GRAY);
    for(unsigned c=0;c<=CALC_COLS;c++) {
        if(note[0]) {
            rect(ledger,colx[c],0,colx[c]+1,ny+1,TBX_COLOR_GRAY);
            rect(ledger,colx[c],ny+ROW_H,colx[c]+1,gh+1,TBX_COLOR_GRAY);
        } else rect(ledger,colx[c],0,colx[c]+1,gh+1,TBX_COLOR_GRAY);
    }
    for(unsigned r=0;r<g_rows;r++) for(unsigned c=0;c<CALC_COLS;c++) paint_cell(c,r);
    if(g_rows) {   /* the cell pointer */
        int x0=colx[i123_cursor_col(&g_sheet)], x1=colx[i123_cursor_col(&g_sheet)+1];
        int y0=row_y(i123_cursor_row(&g_sheet)), y1=y0+ROW_H;
        rect(ledger,x0,y0,x1+1,y0+1,TBX_COLOR_BLACK); rect(ledger,x0,y1,x1+1,y1+1,TBX_COLOR_BLACK);
        rect(ledger,x0,y0,x0+1,y1+1,TBX_COLOR_BLACK); rect(ledger,x1,y0,x1+1,y1+1,TBX_COLOR_BLACK);
    }
    if(note[0]) { copy(line,note); text(ledger,4,ny+1,line,TBX_COLOR_BLACK,TBX_COLOR_WHITE); }
}

/* The pie: an elliptical disk of slices, each value/sum of a turn, exploded
 * and extruded as in the still (geometry.tsv PIE, PIEDEPTH). Integer only.
 * Angles: 65536 to the turn, clockwise from 12 o'clock. */
enum { PIE_CX=112, PIE_CY=62, PIE_A=92, PIE_B=52, PIE_D=28, PIE_X=6, PIE_LABEL=62 };
static const int16_t quarter[17]={0,100,200,297,392,483,569,650,724,792,851,903,946,980,1004,1019,1024};
static int sine(uint32_t a)   /* sin of a turn fraction, scaled 1024 */
{
    a&=65535u; unsigned q=a>>14, i=a&16383u;
    if(q&1) i=16384u-i;
    int k=(int)(i>>10), f=(int)(i&1023u);
    int v=k<16?quarter[k]+((quarter[k+1]-quarter[k])*f>>10):1024;
    return q>=2?-v:v;
}
static int cosine(uint32_t a) { return sine(a+16384u); }
static int isqrt(int32_t v) { int32_t r=0, b=1<<30; while(b>v) b>>=2; while(b) { if(v>=r+b) { v-=r+b; r=(r>>1)+b; } else r>>=1; b>>=2; } return (int)r; }
static int16_t span_l[2*PIE_B+1], span_r[2*PIE_B+1];
/* Row spans of the top face of one slice piece (at most half a turn):
 * inside the ellipse, clockwise of the ray at `s`, before the ray at `e`. */
static void piece_spans(uint32_t s,uint32_t e)
{
    int ss=sine(s), cs=cosine(s), se=sine(e), ce=cosine(e);
    for(int y=-PIE_B;y<=PIE_B;y++) {
        int xe=isqrt((int32_t)PIE_A*PIE_A*(PIE_B*PIE_B-y*y))/PIE_B, l=-xe, r=xe;
        /* (x*B, y*A) after the ray at s: ss*y*A + cs*x*B >= 0; before e: < 0 */
        for(int x=-xe;x<=xe;x++) {
            int32_t ys=(int32_t)ss*y*PIE_A+(int32_t)cs*x*PIE_B, ye=(int32_t)se*y*PIE_A+(int32_t)ce*x*PIE_B;
            if(ys>=0&&ye<0) { l=x; break; }
            l=xe+1;
        }
        for(int x=xe;x>=l;x--) {
            int32_t ys=(int32_t)ss*y*PIE_A+(int32_t)cs*x*PIE_B, ye=(int32_t)se*y*PIE_A+(int32_t)ce*x*PIE_B;
            if(ys>=0&&ye<0) { r=x; break; }
            r=l-1;
        }
        span_l[y+PIE_B]=(int16_t)l; span_r[y+PIE_B]=(int16_t)r;
    }
}
static void draw_piece(uint32_t s,uint32_t e,int ox,int oy)
{
    piece_spans(s,e);
    int cx=PIE_CX+ox, cy=PIE_CY+oy;
    /* the side: the face swept down PIE_D rows, shaded darker than the face
     * by a scanline dither (the Toolbox has one blue; colours.tsv) */
    for(int y=-PIE_B;y<=PIE_B+PIE_D;y++) {
        int l=32767, r=-32768;
        for(int t=y-PIE_D;t<=y;t++) if(t>=-PIE_B&&t<=PIE_B&&span_l[t+PIE_B]<=span_r[t+PIE_B]) {
            if(span_l[t+PIE_B]<l) l=span_l[t+PIE_B];
            if(span_r[t+PIE_B]>r) r=span_r[t+PIE_B];
        }
        rect(chart,cx+l,cy+y,cx+r+1,cy+y+1,((cy+y)&1)?TBX_COLOR_BLACK:TBX_COLOR_NAVY);
    }
    /* the edge between face and side, then the face */
    for(int y=-PIE_B;y<=PIE_B;y++) if(span_l[y+PIE_B]<=span_r[y+PIE_B])
        rect(chart,cx+span_l[y+PIE_B],cy+y+1,cx+span_r[y+PIE_B]+1,cy+y+2,TBX_COLOR_BLACK);
    for(int y=-PIE_B;y<=PIE_B;y++) if(span_l[y+PIE_B]<=span_r[y+PIE_B])
        rect(chart,cx+span_l[y+PIE_B],cy+y,cx+span_r[y+PIE_B]+1,cy+y+1,TBX_COLOR_NAVY);
}
static double series_sum(void) { double t=0; for(unsigned i=0;i<nseries;i++) t+=series[i]; return t; }
static uint32_t slice_start(unsigned i)   /* value/sum of a turn */
{
    double t=series_sum(), a=0; for(unsigned k=0;k<i;k++) a+=series[k];
    return i>=nseries?65536u:(uint32_t)(a*65536.0/t+0.5);
}
static void paint_chart(void)
{
    int32_t sz=tbx_content_size(chart);
    if(sz<=0) return;
    rect(chart,0,0,sz&0xFFFF,(sz>>16)&0xFFFF,TBX_COLOR_BLACK);
    if(!nseries) return;
    /* back to front: slices whose middle points away (12 o'clock) first */
    unsigned order[SERIES_MAX];
    for(unsigned i=0;i<nseries;i++) order[i]=i;
    for(unsigned i=1;i<nseries;i++) for(unsigned k=i;k>0;k--) {
        uint32_t ma=(slice_start(order[k-1])+slice_start(order[k-1]+1))/2, mb=(slice_start(order[k])+slice_start(order[k]+1))/2;
        if(cosine(mb)>cosine(ma)) { unsigned t=order[k]; order[k]=order[k-1]; order[k-1]=t; } else break;
    }
    for(unsigned k=0;k<nseries;k++) {
        unsigned i=order[k]; uint32_t s=slice_start(i), e=slice_start(i+1), m=(s+e)/2;
        if(e<=s) continue;
        int ox=PIE_X*sine(m)/1024, oy=-PIE_X*cosine(m)*PIE_B/PIE_A/1024;
        if(e-s>32768u) { draw_piece(s,m,ox,oy); draw_piece(m,e,ox,oy); }
        else draw_piece(s,e,ox,oy);
    }
    for(unsigned i=0;i<nseries;i++) {
        uint32_t s=slice_start(i), e=slice_start(i+1), m=(s+e)/2;
        int ox=PIE_X*sine(m)/1024, oy=-PIE_X*cosine(m)*PIE_B/PIE_A/1024;
        int x=PIE_CX+ox+PIE_A*PIE_LABEL/100*sine(m)/1024, y=PIE_CY+oy-PIE_B*PIE_LABEL/100*cosine(m)/1024;
        i123_number_text(series[i],line,sizeof line);
        unsigned n=length(line); line[n]='%'; line[n+1]=0;
        text(chart,x-3*(int)(n+1),y-5,line,TBX_COLOR_WHITE,TBX_COLOR_NAVY);
    }
}
static void paint(void) { paint_ledger(); paint_chart(); }

/* ---- launch -------------------------------------------------------------- */
static void notice(const char *s) { g_ask=ASK_NOTICE; if(tbx_alert(s,"OK",0,0)!=TBX_OK) g_ask=ASK_NONE; }
int tbx_app_main(void)
{
    const char *problem=0;
    if(tbx_register()<0||i123_sheet_init(&g_sheet,store,sizeof store,320)!=I123_OK) return 1;
    entered=0; entry[0]=0; empty_model();
    int got=load(copy_dst);
    if(got<0) problem=unread_text;
    if(got<=0&&load(shipped)<=0) problem=lost_text;
    i123_set_cursor(&g_sheet,0,0);
    if(tbx_set_mbar(&mbar)<0) return 1;
    ledger=tbx_new_window(bounds[0][0],bounds[0][1],bounds[0][2],bounds[0][3],TBX_GOAWAY_ASK);
    if(ledger<0||tbx_set_wtitle(ledger,"InitechCalc")<0) { tbx_exit(1); return 1; }
    chart=tbx_new_window(bounds[1][0],bounds[1][1],bounds[1][2],bounds[1][3],TBX_GOAWAY_ASK);
    if(chart<0||tbx_set_wtitle(chart,"Pie Chart")<0) { tbx_exit(1); return 1; }
    if(problem) notice(problem);
    return 0;
}
static void commit(void)
{
    double x; uint16_t c=i123_cursor_col(&g_sheet), r=i123_cursor_row(&g_sheet);
    if(!entered) return;
    const i123_cell_t *p=i123_find(&g_sheet,c,r);
    uint8_t fmt=p&&(p->kind==I123_K_NUMBER||p->kind==I123_K_INTEGER)?cell_fmt(p):FMT_GENERAL;
    if(calc_parse(entry,entered,&x)) (void)put_number(c,r,x,fmt);
    else if(entry[0]=='\\'&&entered>1) (void)i123_set_label(&g_sheet,c,r,'\\',entry+1,entered-1);
    else (void)i123_set_label(&g_sheet,c,r,'\'',entry,entered);
    entered=0; entry[0]=0;
    g_dirty=1;  /* a committed edit is unsaved work */
}
static void erase(void)
{
    uint16_t c=i123_cursor_col(&g_sheet), r=i123_cursor_row(&g_sheet);
    entered=0; entry[0]=0;
    if(i123_find(&g_sheet,c,r)) { (void)i123_erase(&g_sheet,c,r); g_dirty=1; }
}

/* ---- saving ------------------------------------------------------------- */
/* Only DOS file-not-found proves absence. A refused OPEN or CLOSE must not
 * bypass the replace question (README.md, failed-save ruling). */
static int exists(const char *path)
{
    int32_t h=tbx_file_open(path,TBX_FILE_READ_ONLY);
    if(h<0) return h==TBX_ERR_DOS(2)?0:h;
    int32_t rc=tbx_file_close(h);
    return rc<0?rc:1;
}
/* Every save writes the model, with the windows where they are now, to a
 * stage this process owns, and FILE COPY copies the stage to TABLES.TXT. */
static void do_save(void)
{
    unsigned n=serialise(); int32_t h=TBX_ERR_DOS(80);
    if(!n) { g_quit_after_save=0; notice(fail_text); return; }
    copy_stage=0;
    /* CREATE_NEW is the ownership proof (toolbox_gate.h Sec 10b). OPEN
     * errors never license truncating someone else's file. Only file-exists
     * advances to another candidate. */
    for(unsigned i=0;i<sizeof stage_names/sizeof stage_names[0];i++) {
        h=tbx_file_create_new(stage_names[i]);
        if(h>=0) { copy_stage=stage_names[i]; break; }
        if(h!=TBX_ERR_DOS(80)) break;
    }
    if(h<0) { g_quit_after_save=0; notice(fail_text); return; }
    g_staged=1;
    int32_t written=tbx_file_write(h,stage,n);
    int32_t closed=tbx_file_close(h); /* also after a short or failed WRITE */
    if(written!=(int32_t)n||closed<0) { g_staged=0; (void)tbx_file_delete(copy_stage); g_quit_after_save=0; notice(fail_text); return; }
    g_copying=1;
    if(tbx_file_copy(copy_caption,copy_stage,copy_dst)!=TBX_OK) {
        g_copying=0;
        /* A rejected request has no completion event to clean up for us. */
        g_staged=0; (void)tbx_file_delete(copy_stage);
        g_quit_after_save=0; notice(fail_text);
    }
}
static void begin_save(void)
{
    if(g_copying||g_ask) return;
    commit();   /* an entry being typed is part of the work */
    int present=exists(copy_dst);
    if(present<0) { g_quit_after_save=0; notice(fail_text); return; }
    if(present) {
        g_ask=ASK_REPLACE;
        if(tbx_alert(replace_text,"Yes","No",0)!=TBX_OK) { g_ask=ASK_NONE; g_quit_after_save=0; } /* never replace unasked */
        return;
    }
    do_save();
}
/* THE ONE EXIT GUARD: File > Quit, Ctrl-Q, the close box and the Finder's
 * Restart / Shut Down request all come here. Unsaved work asks; a clean
 * worksheet quits. If the question cannot be shown the session goes on. */
static void request_quit(void)
{
    if(g_ask||g_copying) return;
    if(!g_dirty&&!entered) { (void)tbx_exit(0); return; }
    g_ask=ASK_QUIT;
    if(tbx_alert(quit_text,"Save","Cancel","Don't Save")!=TBX_OK) g_ask=ASK_NONE;
}
static void answer(int32_t item)
{
    int k=g_ask; g_ask=ASK_NONE;
    if(k==ASK_QUIT) {
        if(item==(int32_t)TBX_ALERT_OTHER) { (void)tbx_exit(0); return; }
        if(item==(int32_t)TBX_ALERT_DEFAULT) { g_quit_after_save=1; begin_save(); }
    } else if(k==ASK_REPLACE) {
        if(item==(int32_t)TBX_ALERT_DEFAULT) do_save(); else g_quit_after_save=0;
    }
}
void tbx_app_event(const tbx_event_t *e)
{
    if(!e) return;
    if(e->what==updateEvt) { paint(); return; }
    if(e->what==TBX_EVT_GOAWAY||(e->what==TBX_EVT_MENU&&e->message==TBX_MENU_WORD(129,2))) { request_quit(); return; }
    if(e->what==TBX_EVT_MENU&&e->message==TBX_MENU_WORD(129,1)) { begin_save(); return; }
    if(e->what==TBX_EVT_ALERT) { answer((int32_t)e->message); return; }
    if(e->what==TBX_EVT_FILECOPY) { /* the shell restored what the box covered */
        int ok=(int32_t)e->message>=0; g_copying=0;
        if(g_staged) { g_staged=0; (void)tbx_file_delete(copy_stage); } /* the stage never outlives the copy */
        if(ok) { g_dirty=0; if(g_quit_after_save) { g_quit_after_save=0; (void)tbx_exit(0); } }
        else { g_quit_after_save=0; notice(fail_text); }
        return;
    }
    if(e->what!=keyDown) return;
    unsigned ch=(unsigned)e->message&255, vk=((unsigned)e->message>>8)&255;
    int c=i123_cursor_col(&g_sheet), r=i123_cursor_row(&g_sheet);
    if(vk==0x4b||vk==0x4d||vk==0x48||vk==0x50||ch=='\t') {
        commit();
        if(vk==0x4b&&c>0) c--;
        if((vk==0x4d||ch=='\t')&&c<CALC_COLS-1) c++;
        if(vk==0x48&&r>0) r--;
        if(vk==0x50&&r+1<(int)g_rows) r++;
        i123_set_cursor(&g_sheet,(uint16_t)c,(uint16_t)r);
    } else if(vk==0x53) erase();            /* Del clears the cell */
    else if(ch=='\r'||ch=='\n') commit();    /* the emulated Return arrives as LF (vk 0x1C) */
    else if(ch==27) { entered=0; entry[0]=0; }
    else if(ch==8) { if(entered) entry[--entered]=0; }
    else if(ch>=32&&ch<=126&&entered<sizeof entry-1) { entry[entered++]=(char)ch; entry[entered]=0; }
    paint_ledger();
}
