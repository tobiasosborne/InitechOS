/* THE ARTIFACT, initech-uqah. Ref: R2.2 Reference /Copy pp. 3-1..5
 * (PDF 69..73), /Move pp. 7-1..4 (PDF 189..192), /Range Name and Trans.
 * NNAME 0x47, 25 bytes re-derived from MINT02.WK1 @0x86 (header),
 * body @0x8a: TOTAL, A5..A5, trailer 0. Existing names remain in carry,
 * retaining original record order and the unknown trailer byte. No locked
 * WK1 header changes. Snapshot overlap semantics are an authored choice:
 * the manual warns overlapping FROM/TO ranges may yield unexpected results.
 */
#include "i123.h"
#include "i123/wk1_format.h"
#include "i123/wk1_formula.h"
extern int i123_put_cell(i123_sheet_t *,uint16_t,uint16_t,uint8_t,uint8_t,
                         const uint8_t *,const uint8_t *,uint32_t,int,uint8_t,
                         const uint8_t *,uint32_t);
static uint16_t rd(const uint8_t *p) { return (uint16_t)(p[0]|(p[1]<<8)); }
static void wr(uint8_t *p,uint16_t v) { p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8); }
static int up(int c) { return c>='a'&&c<='z'?c-32:c; }
static uint32_t length(const char *s) { uint32_t n=0;while(s[n])n++;return n; }
static int inside(i123_rect_t r,int c,int y) { return c>=r.c0&&c<=r.c1&&y>=r.r0&&y<=r.r1; }
static int valid(i123_rect_t r) { return r.c0<=r.c1&&r.r0<=r.r1&&r.c1<256&&r.r1<8192; }
static uint32_t next(const i123_sheet_t *s,uint32_t p) { uint16_t n=rd(s->carry+p+2);return p+4+(n==65535?0:n); }
static int eq(const char *nm,uint32_t n,const uint8_t *p)
{ uint32_t k=0;while(k<n&&k<16&&p[k]&&up(nm[k])==up(p[k]))k++;return k==n&&k<16&&!p[k]; }
int i123_name_find(const i123_sheet_t *sh,const char *nm,uint32_t n,i123_rect_t *rg)
{
    for(uint32_t p=0;p<sh->carry_len;p=next(sh,p)) if(rd(sh->carry+p)==0x47&&rd(sh->carry+p+2)==25&&eq(nm,n,sh->carry+p+4)) {
        const uint8_t *b=sh->carry+p+20;
        rg->c0=rd(b);rg->r0=rd(b+2);rg->c1=rd(b+4);rg->r1=rd(b+6);
        return valid(*rg)?I123_OK:I123_E_VALUE;
    }
    return I123_E_VALUE;
}
static int name_valid(const char *nm)
{
    uint32_t n=length(nm);if(!n||n>15)return 0;
    if(!(up(nm[0])>='A'&&up(nm[0])<='Z')&&nm[0]!='_'&&nm[0]!='\\')return 0;
    for(uint32_t i=1;i<n;i++)if(!(up(nm[i])>='A'&&up(nm[i])<='Z')&&!(nm[i]>='0'&&nm[i]<='9')&&nm[i]!='_'&&nm[i]!='.')return 0;
    /* Reject names that are cell addresses (manual /Range Name). */
    uint32_t i=0,c=0,r=0;
    while(up(nm[i])>='A'&&up(nm[i])<='Z')c=c*26+(uint32_t)(up(nm[i++])-'A'+1);
    uint32_t start=i;while(nm[i]>='0'&&nm[i]<='9')r=r*10+(uint32_t)(nm[i++]-'0');
    return !(i==n&&i>start&&c<=256&&r>=1&&r<=8192);
}
int i123_name_set(i123_sheet_t *s,const char *nm,i123_rect_t r)
{
    if(!valid(r)||!name_valid(nm))return I123_E_ARG;
    uint32_t p;for(p=0;p<s->carry_len;p=next(s,p))if(rd(s->carry+p)==0x47&&rd(s->carry+p+2)==25&&eq(nm,length(nm),s->carry+p+4))break;
    if(p<s->carry_len && (rd(s->carry+p+20)!=r.c0 || rd(s->carry+p+22)!=r.r0 ||
        rd(s->carry+p+24)!=r.c1 || rd(s->carry+p+26)!=r.r1)) {
        /* Rebinding compiled formulas is deferred, never silently stale. */
        for(uint32_t i=0;i<s->ncells;i++)if(s->cells[i].kind==I123_K_FORMULA)return I123_E_ARG;
    }
    if(p==s->carry_len) {
        if(s->carry_len+29>I123_CARRY_CAP)return I123_E_FULL;
        /* Insert before the cell run, preserving a loaded file's order. */
        p=s->carry_cellpos;
        for(uint32_t i=s->carry_len;i>p;i--)s->carry[i+28]=s->carry[i-1];
        s->carry_len+=29;s->carry_cellpos+=29;
        for(uint32_t i=0;i<29;i++)s->carry[p+i]=0;
        wr(s->carry+p,0x47);wr(s->carry+p+2,25);
        for(uint32_t i=0;nm[i];i++)s->carry[p+4+i]=(uint8_t)up(nm[i]);
    }
    wr(s->carry+p+20,r.c0);wr(s->carry+p+22,r.r0);wr(s->carry+p+24,r.c1);wr(s->carry+p+26,r.r1);
    return I123_OK;
}
int i123_name_delete(i123_sheet_t *s,const char *nm)
{
    int found=0;
    for(uint32_t p=0;p<s->carry_len;) {
        uint32_t end=next(s,p);
        if(rd(s->carry+p)==0x47&&(!nm||eq(nm,length(nm),s->carry+p+4))) {
            uint32_t n=end-p;for(uint32_t i=end;i<s->carry_len;i++)s->carry[i-n]=s->carry[i];
            s->carry_len-=n;if(p<s->carry_cellpos)s->carry_cellpos-=n;found=1;
        } else p=end;
    }
    return found||!nm?I123_OK:I123_E_VALUE;
}
static int address(const char **text,uint16_t *c,uint16_t *r)
{
    const char *p=*text;unsigned col=0,row=0;int nd=0;
    if(*p=='$')p++;
    while(up(*p)>='A'&&up(*p)<='Z') {col=col*26+(unsigned)(up(*p++)-'A'+1);if(col>256)return 0;}
    if(*p=='$')p++;
    while(*p>='0'&&*p<='9') {row=row*10+(unsigned)(*p++-'0');nd=1;if(row>8192)return 0;}
    if(!col||!row||!nd)return 0;
    *c=(uint16_t)(col-1);*r=(uint16_t)(row-1);*text=p;return 1;
}
int i123_range_parse(const i123_sheet_t *sh,const char *text,i123_rect_t *r)
{
    if(i123_name_find(sh,text,length(text),r)==0)return 0;
    if(!address(&text,&r->c0,&r->r0))return I123_E_ARG;
    r->c1=r->c0;r->r1=r->r0;
    if(*text=='.') {text++;if(*text=='.')text++;if(!address(&text,&r->c1,&r->r1))return I123_E_ARG;}
    if(*text)return I123_E_ARG;
    if(r->c0>r->c1){uint16_t t=r->c0;r->c0=r->c1;r->c1=t;}
    if(r->r0>r->r1){uint16_t t=r->r0;r->r0=r->r1;r->r1=t;}
    return I123_OK;
}
int i123_sheet_clone(const i123_sheet_t *s,i123_sheet_t *d)
{
    if(s==d||s->cells==d->cells)return I123_E_ARG;
    if(s->ncells>d->cap||s->pool_used>d->pool_cap)return I123_E_FULL;
    i123_cell_t *cells=d->cells;uint8_t *pool=d->pool,*carry=d->carry;uint32_t cap=d->cap,pc=d->pool_cap;
    *d=*s;d->cells=cells;d->pool=pool;d->carry=carry;d->cap=cap;d->pool_cap=pc;
    for(uint32_t i=0;i<s->ncells;i++)cells[i]=s->cells[i];
    for(uint32_t i=0;i<s->pool_used;i++)pool[i]=s->pool[i];
    for(uint32_t i=0;i<s->carry_len;i++)carry[i]=s->carry[i];
    return I123_OK;
}
/* Retarget reference operands (bead initech-nn46). Ref: real 1-2-3 R2.2
 * before/after goldens, ../lotus123-decomp goldens/minted/{COPY01,MOVE01,
 * WRAP01}?.WK1 and re/mint-results-002.md; wk1_formula.h WKF_REF_*.
 *  /Copy never rewrites a reference word: a relative word is an offset, an
 *   absolute one a coordinate, so the copied code is byte-identical
 *   (COPY01B..D; WRAP01B keeps ffbf at column A, shown +IV1+1; WRAP01C
 *   keeps ffbf at row 1, shown +A8192+1 -- both axes wrap, no ERR).
 *  /Move re-encodes a corner whose target lies in FROM (it follows the
 *   cell) or, for a formula that moved, every corner (its home moved:
 *   MOVE01C F5); a corner whose target lies in the overwritten TO becomes
 *   the ERR reference, both words 0xFFFF, the $ flags dropped (MOVE01B E1,
 *   E2 +$D$1, H1's D1 corner); every other corner keeps its bytes (MOVE01B
 *   B2 first operand, H1 D2 corner). Re-encoded: absolute words hold the
 *   coordinate (MOVE01B F1 $A1 -> 0300), a relative column the 8-bit form
 *   0x8000|((target-home)&0xFF) (MOVE01C F5 fb80, MOVE01D F1 fb80 even
 *   though the column did not change), a relative row the 14-bit form
 *   (MOVE01C F5 fdbf). A corner already ERR keeps 0xFFFF (MOVE01C/D E1,
 *   E2, H1). A moved formula whose target moved with it is re-encoded too
 *   [inferred: no golden moves a formula together with its precedent]. */
#ifdef I123_MUT_MOVE_COL14
#define MOVE_COL_MASK WKF_REF_MASK   /* MUTANT (Rule 6): the 14-bit column form */
#else
#define MOVE_COL_MASK WKF_REF_COL8
#endif
#ifdef I123_MUT_ERR_WORD
#define MOVE_ERR_WORD 0xBFFFu        /* MUTANT (Rule 6): ERR spelled relative -1 */
#else
#define MOVE_ERR_WORD WKF_REF_ERR
#endif
static uint16_t moved_word(uint16_t w,int target,int home,unsigned mask)
{ return (uint16_t)((w&WKF_REF_REL)?WKF_REF_REL|((unsigned)(target-home)&mask):(unsigned)target); }
static int adjust(const i123_cell_t *cell,const uint8_t *in,uint8_t *out,uint32_t *olen,
                  uint16_t nc,uint16_t nr,i123_rect_t from,i123_rect_t to,int move)
{
    int dx=(int)to.c0-from.c0,dy=(int)to.r0-from.r0;
    if(i123_formula_check(in,cell->len)!=0)return I123_E_FORMAT;
    /* A /Move whose TO is its own FROM rewrites no reference byte
     * [inferred: no golden moves a range onto itself; the re-encoding is
     * evidenced only for a displaced move; lotus123-decomp GAPS G-13] */
    if(!(dx|dy))move=0;
    for(uint32_t j=0;j<cell->len;j++)out[j]=in[j];
    *olen=cell->len;
    for(uint32_t p=0;p<cell->len;) {
        uint32_t n=i123_token_size(in,cell->len,p);
        if(in[p]==WKF_VAR||in[p]==WKF_RANGE) for(uint32_t j=1;j<n;j+=4) {
            uint16_t cw=rd(in+p+j),rw=rd(in+p+j+2),ncw=MOVE_ERR_WORD,nrw=MOVE_ERR_WORD;
            int c=i123_ref_resolve(cw,cell->col,1),r=i123_ref_resolve(rw,cell->row,0);
#ifdef I123_MUT_COPY_ABSOLUTE
            if(!move&&c>=0&&r>=0) {     /* MUTANT: /Copy shifts absolute words */
                if(!(cw&WKF_REF_REL))wr(out+p+j,(uint16_t)((c+(int)nc-cell->col)&255));
                if(!(rw&WKF_REF_REL))wr(out+p+j+2,(uint16_t)((r+(int)nr-cell->row)&8191));
            }
#endif
            if(!move||c<0||r<0)continue;        /* /Copy keeps bytes; ERR stays ERR */
            int follow=inside(from,c,r);
            if(follow){c+=dx;r+=dy;}
            else if(!inside(to,c,r)) {
                if(nc==cell->col&&nr==cell->row)continue;   /* untouched corner */
                follow=1;                                  /* its home moved */
            }
            if(follow){ncw=moved_word(cw,c,nc,MOVE_COL_MASK);nrw=moved_word(rw,r,nr,WKF_REF_MASK);}
            wr(out+p+j,ncw);wr(out+p+j+2,nrw);
        }
        p+=n;
    }
    return I123_OK;
}
int i123_transfer(const i123_sheet_t *s,i123_sheet_t *d,i123_rect_t from,uint16_t col,uint16_t row,int mode)
{
    return i123_transfer_rect(s,d,from,(i123_rect_t){col,row,col,row},mode);
}
int i123_transfer_rect(const i123_sheet_t *s,i123_sheet_t *d,i123_rect_t from,i123_rect_t to,int mode)
{
    int rc;uint16_t col=to.c0,row=to.r0;
    if(!valid(from)||!valid(to)||mode<0||mode>3)return I123_E_ARG;
    if(mode&&((to.c0!=to.c1)||(to.r0!=to.r1)))return I123_E_ARG;
#ifdef I123_MUT_COPY_RECT_REFUSED
    if(to.c0!=to.c1||to.r0!=to.r1)return I123_E_ARG;
#endif
    uint32_t w=from.c1-from.c0,h=from.r1-from.r0;
    if(mode==3){uint32_t t=w;w=h;h=t;}
    if(col+w>=256||row+h>=8192)return I123_E_ARG;
    /* Real R2.2 fixtures/i123-copy/{ROW,BLOCK,OVER}.WK1: replicate ONLY
     * singleton source axes. A1..B1 -> F1..H3 writes F/G, leaves H alone;
     * a 2x2 source is copied once even into a larger TO. Manual pp.3-4/5.
     * The original bank remains the snapshot, including overlap. */
    if(w||mode)to.c1=(uint16_t)(col+w);
    if(h||mode)to.r1=(uint16_t)(row+h);
    /* Manual: global protection applies to cells marked Prot. */
    if(s->protec) {
        /* B02 review (initech-3s12): an ABSENT cell is the default 0xFF,
         * protected; only a stored unprotected cell (bit 7 clear, a BLANK
         * from /Range Unprot) may be written. So every cell of TO (and of
         * FROM for /Move) must exist unprotected: count them. */
        uint32_t un_to=0,un_from=0;
        for(uint32_t i=0;i<s->ncells;i++) {
            const i123_cell_t *c=&s->cells[i];int t=inside(to,c->col,c->row),f=mode==1&&inside(from,c->col,c->row);
            if((t||f)&&(c->fmt&128))return I123_E_ARG;
            un_to+=(uint32_t)t;un_from+=(uint32_t)f;
        }
#ifndef I123_MUT_ABSENT_UNPROTECTED
        if(un_to<(uint32_t)(to.c1-col+1)*(to.r1-row+1)||(mode==1&&un_from<(w+1u)*(h+1u)))return I123_E_ARG;
#else
        /* MUTANT (Rule 6; test-i123-cells-mutant C9): absent cells count as
         * unprotected. NEVER real. */
#endif
    }
    rc=i123_sheet_clone(s,d);if(rc)return rc;
    for(uint32_t i=d->ncells;i>0;i--) {
        i123_cell_t *c=&d->cells[i-1];
        if(inside(to,c->col,c->row)||(mode==1&&inside(from,c->col,c->row)))i123_erase(d,c->col,c->row);
    }
    for(uint32_t i=0;i<s->ncells;i++) {
        const i123_cell_t *c=&s->cells[i];int selected=inside(from,c->col,c->row);
        if(!selected)continue;
        for(uint32_t nr=row+(mode==3?c->col-from.c0:c->row-from.r0);nr<=to.r1;nr+=h+1)
        for(uint32_t nc=col+(mode==3?c->row-from.r0:c->col-from.c0);nc<=to.c1;nc+=w+1) {
        uint8_t code[I123_CODE_MAX];uint32_t n=c->len;
        if(c->kind==I123_K_FORMULA&&mode<2) {
            if(n>sizeof code)return I123_E_FULL;
            rc=adjust(c,s->pool+c->off,code,&n,nc,nr,from,to,mode==1);if(rc)return rc;
            rc=i123_set_formula_raw(d,nc,nr,c->fmt,c->v,code,n,c->has_str?(const char *)(s->pool+c->off+c->len):0,c->slen);
        } else if(c->kind==I123_K_FORMULA) {
            i123_val_t v;i123_cell_value(s,c->col,c->row,&v);
            if(v.t==I123_V_STR)rc=i123_set_label(d,nc,nr,'\'',(const char *)v.s,v.slen);
            else rc=i123_put_cell(d,nc,nr,I123_K_NUMBER,c->fmt,c->v,0,0,0,0,0,0);
            if(!rc)rc=i123_set_cell_format(d,nc,nr,c->fmt);
        } else {
            rc=i123_set_cell_format(d,nc,nr,c->fmt);
            if(!rc)rc=i123_put_cell(d,nc,nr,c->kind,c->fmt,c->v,s->pool+c->off,c->len,c->has_str,c->str_fmt,s->pool+c->off+c->len,c->slen);
        }
        if(rc)return rc;
        }
    }
    if(mode==1) {
#ifndef I123_MUT_MOVE_STALE
        for(uint32_t i=0;i<s->ncells;i++) {
            const i123_cell_t *c=&s->cells[i];
            if(c->kind!=I123_K_FORMULA||inside(from,c->col,c->row)||inside(to,c->col,c->row))continue;
            uint8_t code[I123_CODE_MAX];uint32_t n;
            if(c->len>sizeof code)return I123_E_FULL;
            rc=adjust(c,s->pool+c->off,code,&n,c->col,c->row,from,to,1);if(rc)return rc;
            rc=i123_set_formula_raw(d,c->col,c->row,c->fmt,c->v,code,n,c->has_str?(const char *)(s->pool+c->off+c->len):0,c->slen);if(rc)return rc;
        }
#endif
        for(uint32_t p=0;p<d->carry_len;p=next(d,p))if(rd(d->carry+p)==0x47&&rd(d->carry+p+2)==25) {
            for(unsigned j=20;j<=24;j+=4) {
                int c=rd(d->carry+p+j),r=rd(d->carry+p+j+2);
                if(inside(from,c,r)) {c+=(int)col-from.c0;r+=(int)row-from.r0;}
                else if(inside(to,c,r)){c=65535;r=65535;}
                wr(d->carry+p+j,(uint16_t)c);wr(d->carry+p+j+2,(uint16_t)r);
            }
        }
    }
    return I123_OK;
}
int i123_name_labels(i123_sheet_t *s,i123_rect_t rg,int dc,int dr)
{
    if(!valid(rg))return I123_E_ARG;
    for(uint32_t i=0;i<s->ncells;i++) {
        const i123_cell_t *c=&s->cells[i];if(c->kind!=I123_K_LABEL||!inside(rg,c->col,c->row))continue;
        char name[16];uint32_t n=c->len-1;if(n>15)n=15;
        for(uint32_t j=0;j<n;j++)name[j]=(char)s->pool[c->off+1+j];
        name[n]=0;
        int col=c->col+dc,row=c->row+dr;if(col<0||col>=256||row<0||row>=8192)return I123_E_ARG;
        i123_rect_t r={(uint16_t)col,(uint16_t)row,(uint16_t)col,(uint16_t)row};
        int rc=i123_name_set(s,name,r);if(rc)return rc;
    }
    return I123_OK;
}
static int name_cmp(const uint8_t *a,const uint8_t *b)
{ while(*a&&*a==*b){a++;b++;}return (int)*a-(int)*b; }
int i123_name_table(i123_sheet_t *s,uint16_t col,uint16_t row)
{
    uint8_t prev[17]={0};
    for(;;) {
        uint32_t p=0xffffffffu;
        for(uint32_t q=0;q<s->carry_len;q=next(s,q)) {
            if(rd(s->carry+q)!=0x47||rd(s->carry+q+2)!=25||name_cmp(s->carry+q+4,prev)<=0)continue;
            if(p==0xffffffffu||name_cmp(s->carry+q+4,s->carry+p+4)<0)p=q;
        }
        if(p==0xffffffffu)break;
        for(unsigned j=0;j<16;j++)prev[j]=s->carry[p+4+j];
        char name[17],a[16],b[16],range[40];unsigned n=0;
        while(n<16&&s->carry[p+4+n]){name[n]=(char)s->carry[p+4+n];n++;}name[n]=0;
        if(col>=255||row>=8192)return I123_E_ARG;
        uint16_t c0=rd(s->carry+p+20),r0=rd(s->carry+p+22),c1=rd(s->carry+p+24),r1=rd(s->carry+p+26);
        if(c0>=256||c1>=256||r0>=8192||r1>=8192)return I123_E_VALUE;
        uint32_t k=i123_addr_text(c0,r0,a),z=i123_addr_text(c1,r1,b);
        for(uint32_t j=0;j<k;j++)range[j]=a[j];
        range[k++]='.';range[k++]='.';
        for(uint32_t j=0;j<z;j++)range[k++]=b[j];
        range[k]=0;
        int rc=i123_set_label(s,col,row,'\'',name,n);if(rc)return rc;
        rc=i123_set_label(s,col+1,row++,'\'',range,k);if(rc)return rc;
    }
    if(row<8192){i123_erase(s,col,row);i123_erase(s,col+1,row);}
    return I123_OK;
}
