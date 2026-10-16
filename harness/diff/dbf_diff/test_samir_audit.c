/* Program-level audit oracle for initech-hw4j / initech-3t55 / initech-nds1.
 * MANUAL-GROUNDED, NOT a real-dBASE differential: the local corpus has no
 * output golden for these exact filter/deleted/USE sessions. K09 additionally
 * grades RECCOUNT/TOP values against real mint/work/SMKLOG.TXT (SMOKE.PRG);
 * that is a corpus-golden projection, not a full transcript differential.
 * Do not mint expected
 * output from SAMIR. References: Ashton-Tate Using dBase III Plus.pdf,
 * U1-4 (default .DBF), U2-26 (asterisk), U5-215 (DELETED and scope exceptions),
 * U5-229/230 (per-area filters, activation, clearing); Programming with dBase
 * III Plus.pdf P10-17/18 (filter vs global DELETED). Error text: local corpus
 * archive/golden-mined/DBASE.MSG / spec/samir/dbase_msg_codes.tsv, #1/#15/#29.
 * The required marker is manual-grounded; its exact column/spacing is
 * authored, no local reference golden. The fixture is the audit's three-row
 * CLIENTS table; never a generated expected-output golden.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>

#include "test_assert.h"
#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/dbf.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"
#include "samir/ndx.h"

TEST_HARNESS();

/* pal_host.c surface (declared here -- not in a header). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* samir_main.c surface (the S5.8 entry point under test). */
extern int samir_repl(samir_pal_t *pal, xb_interp *ip);

/* =====================================================================
 * Capturing + scripting PAL (same idiom as test_samir_repl.c).
 * ===================================================================== */

#define CAP_BUF          65536
#define SCRIPT_MAX_LINES 128

typedef struct {
    samir_pal_t  pal;        /* MUST be first: &cap.pal is handed to the engine */
    samir_pal_t *inner;
    char         buf[CAP_BUF];
    uint32_t     len;
    const char  *lines[SCRIPT_MAX_LINES];
    int          nlines;
    int          lineidx;
    int          strip_prompt;   /* drop the dot prompt just before each read */
} cap_pal;

static cap_pal g_cap;

static pal_fd  cap_open (samir_pal_t *p, const char *n, int m) { cap_pal *c=(cap_pal*)p; if(strstr(n,"DENIED")) return -PAL_EACCES; return c->inner->open(c->inner,n,m); }
static int     cap_close(samir_pal_t *p, pal_fd fd)           { cap_pal *c=(cap_pal*)p; return c->inner->close(c->inner,fd); }
static int32_t cap_read (samir_pal_t *p, pal_fd fd, void *b, uint32_t n){ cap_pal *c=(cap_pal*)p; return c->inner->read(c->inner,fd,b,n); }
static int32_t cap_write(samir_pal_t *p, pal_fd fd, const void *b, uint32_t n){ cap_pal *c=(cap_pal*)p; return c->inner->write(c->inner,fd,b,n); }
static int32_t cap_seek (samir_pal_t *p, pal_fd fd, int32_t o, int w){ cap_pal *c=(cap_pal*)p; return c->inner->seek(c->inner,fd,o,w); }
static int     cap_remove(samir_pal_t *p, const char *n)      { cap_pal *c=(cap_pal*)p; return c->inner->remove(c->inner,n); }
static int     cap_rename(samir_pal_t *p, const char *f, const char *t){ cap_pal *c=(cap_pal*)p; return c->inner->rename(c->inner,f,t); }
static void    cap_conout(samir_pal_t *p, const char *s, uint32_t n)
{
    cap_pal *c=(cap_pal*)p;
    uint32_t i;
    for (i = 0; i < n && c->len < (uint32_t)(CAP_BUF - 1); i++)
        c->buf[c->len++] = s[i];
    c->buf[c->len] = '\0';
}
static int32_t cap_conin_line(samir_pal_t *p, char *buf, uint32_t cap)
{
    cap_pal *c=(cap_pal*)p;
    const char *s;
    uint32_t k;
    if (c->strip_prompt && c->len >= 2u && c->buf[c->len-2u]=='.' && c->buf[c->len-1u]==' ') {
        c->buf[c->len-2u] = '\1';      /* command boundary marker (m005_normalize) */
        c->buf[c->len -= 1u] = '\0';
    }
    if (c->lineidx >= c->nlines) { if (cap) buf[0] = '\0'; return -1; }
    s = c->lines[c->lineidx++];
    k = 0;
    while (s[k] != '\0' && k < cap - 1u) { buf[k] = s[k]; k++; }
    buf[k] = '\0';
    return (int32_t)k;
}
static int32_t cap_conin_char(samir_pal_t *p){ cap_pal *c=(cap_pal*)p; return c->inner->conin_char(c->inner); }
static void    cap_gotoxy(samir_pal_t *p, uint8_t r, uint8_t col){ cap_pal *c=(cap_pal*)p; c->inner->gotoxy(c->inner,r,col); }
static void    cap_set_attr(samir_pal_t *p, uint8_t a){ cap_pal *c=(cap_pal*)p; c->inner->set_attr(c->inner,a); }
static void    cap_today(samir_pal_t *p, uint8_t *yy, uint8_t *mm, uint8_t *dd){ cap_pal *c=(cap_pal*)p; c->inner->today(c->inner,yy,mm,dd); }
static void   *cap_alloc(samir_pal_t *p, uint32_t n){ cap_pal *c=(cap_pal*)p; return c->inner->alloc(c->inner,n); }
static void    cap_reset(samir_pal_t *p, void *m){ cap_pal *c=(cap_pal*)p; c->inner->reset(c->inner,m); }

static void *cap_acquire(samir_pal_t *p,uint32_t n) { cap_pal *c=(cap_pal *)p;return c->inner->acquire(c->inner,n); }
static void cap_release(samir_pal_t *p,void *m) { cap_pal *c=(cap_pal *)p;c->inner->release(c->inner,m); }
static samir_pal_t *cap_pal_make(samir_pal_t *inner)
{
    g_cap.inner = inner;
    g_cap.len = 0; g_cap.buf[0] = '\0';
    g_cap.nlines = 0; g_cap.lineidx = 0;
    g_cap.pal.open       = cap_open;
    g_cap.pal.close      = cap_close;
    g_cap.pal.read       = cap_read;
    g_cap.pal.write      = cap_write;
    g_cap.pal.seek       = cap_seek;
    g_cap.pal.remove     = cap_remove;
    g_cap.pal.rename     = cap_rename;
    g_cap.pal.conout     = cap_conout;
    g_cap.pal.conin_line = cap_conin_line;
    g_cap.pal.conin_char = cap_conin_char;
    g_cap.pal.gotoxy     = cap_gotoxy;
    g_cap.pal.set_attr   = cap_set_attr;
    g_cap.pal.today      = cap_today;
    g_cap.pal.alloc      = cap_alloc;
    g_cap.pal.reset      = cap_reset;
    g_cap.pal.acquire = cap_acquire; g_cap.pal.release = cap_release;
    return &g_cap.pal;
}

static void cap_clear(void) { g_cap.len = 0; g_cap.buf[0] = '\0'; }
static void script_reset(void) { g_cap.nlines = 0; g_cap.lineidx = 0; }
static void script_push(const char *s) { if (g_cap.nlines < SCRIPT_MAX_LINES) g_cap.lines[g_cap.nlines++] = s; }
static int cap_has(const char *needle) { return strstr(g_cap.buf, needle) != NULL; }


static int duplicate_names;
static void copy_fixture(void)
{
    FILE *in=fopen("build/CLIENTS.DBF", "rb");
    FILE *out=fopen("build/samir.audit/CLIENTS.DBF", "wb");
    int ch;
    CHECK(in && out, "fixture files opened");
    if (in && out) while ((ch=fgetc(in))!=EOF) fputc(ch,out);
    if(in) fclose(in);
    if(out) fclose(out);
    if(duplicate_names) {
        unsigned char hdr[12];unsigned int start,len,r;
        out=fopen("build/samir.audit/CLIENTS.DBF","r+b");
        CHECK(out!=NULL,"duplicate fixture opened");
        if(out) {
            CHECK(fread(hdr,1,12,out)==12,"DBF header read for byte fixture");
            start=hdr[8]+256u*hdr[9];len=hdr[10]+256u*hdr[11];
            for(r=0;r<2;r++) {
                CHECK(fseek(out,(long)(start+r*len+1),SEEK_SET)==0,"DBF record offset (corpus dbf.md)");
                CHECK(fwrite("MATCH     ",1,10,out)==10,"duplicate NAME fixture bytes");
            }
            fclose(out);
        }
    }
}
#ifndef TEST_AUDIT_USE
static int name_key(void *u,uint32_t recno,uint8_t *out,uint16_t len)
{
    static const char *names[]={"PESTON","WADDAMS","LUMBERGH"};
    const char *name=duplicate_names && recno<=2 ? "MATCH" : names[recno-1];
    (void)u;
    if(recno<1 || recno>3 || len!=10) return -1;
    memset(out,' ',len);memcpy(out,name,strlen(name));
    return 0;
}
#endif

#ifdef TEST_AUDIT_USE
/* The original program USEs CLIENTS without an extension. Copy each file
 * byte-identically before the REPL opens it RW; the corpus is read-only.
 * Ref: dbase3-decomp/re/mint-results-001.md, SMOKE.PRG and SMKLOG.TXT.
 */
static int reference_copy(const char *source,const char *dest)
{
    FILE *a=fopen(source,"rb"), *b=fopen(dest,"wb");
    int x,y,ok=a && b;
    CHECK(ok,"real corpus fixture available for byte-identical local copy");
    if(ok) while((x=fgetc(a))!=EOF) if(fputc(x,b)==EOF) { ok=0;break; }
    if(a) fclose(a);
    if(b && fclose(b)) ok=0;
    a=fopen(source,"rb");b=fopen(dest,"rb");
    if(!a || !b) ok=0;
    if(ok) do { x=fgetc(a);y=fgetc(b);if(x!=y) ok=0; } while(ok && x!=EOF);
    if(a) fclose(a);
    if(b) fclose(b);
    CHECK(ok,"real fixture and local RW copy are byte-identical");
    return ok;
}

/* Mint session 005 replay (initech-vfys / initech-k7lc / initech-5vmn).
 * REAL-dBASE DIFFERENTIAL: the program text is read from the corpus PRG
 * (dbase3-decomp/mint/work/M005U.PRG, M005N.PRG) and the expected transcript is
 * the SET ALTERNATE log real DBASE.EXE 1.1 wrote under dosbox-x
 * (M005ULOG.TXT, M005NLOG.TXT; md5s in dbase3-decomp/re/mint-results-005.md and
 * pinned by the Makefile before this runs). Nothing here is generated by SAMIR.
 * Replay rules (the only normalization, both documented in the ledger):
 *  - SET ALTERNATE / QUIT lines are the capture harness, not the probe; the
 *    ON ERROR line installs a trap whose MESSAGE() SAMIR lacks, so it is
 *    dropped and the trap's "  ->ERR#<n> <msg>" line is compared as the REPL's
 *    default catalog rendering "<n>  <msg>" + newline (same code, same
 *    DBASE.MSG text);
 *  - the dot prompt the REPL prints before each read is removed;
 *  - LIST/DISPLAY commands run verbatim in single-command PRGs so their
 *    program-opening CR LF is graded (M005U/M005N; initech-31d1);
 *  - spaces, including trailing cell padding, are preserved. Only the error
 *    renderer's missing opening newline is supplied at a command boundary;
 *  - the log's CR LF becomes LF and the closing ^Z is dropped. */
static char m005_text[8192];
/* M005U/M005N are byte oracles for layout, not just record order. Never
 * collapse spaces or repair a missing newline before a Record# heading. */
static void m005_normalize(const char *in,char *out,size_t cap)
{
    size_t k=0;
    for(;*in && k+2<cap;in++) {
        if(*in=='\1') {
            const char *q=in;
            while(*q=='\1') q++;
            if(*q && *q!='\n' && *q!='\r' && strncmp(q,"Record#",7) && k>0 && out[k-1]!='\n')
                out[k++]='\n';
            in=q-1; continue;
        }
        if(*in=='\r') continue;
        out[k++]=*in;
    }
    out[k]=0;
}
static char m005_norm[CAP_BUF];
static void m005_replay(const char *corpus,const char *prg,const char *log,
                        const char *const *inputs,const char *why)
{
    char path[1200],dst[1200],cwd[1024],gold[8192],want[8192];
    const char *lines[SCRIPT_MAX_LINES];
    char do_lines[SCRIPT_MAX_LINES][32];
    int nl=0,i;
    size_t n=0,k=0;
    char *p;
    FILE *f;
    xb_interp *ip;
    mkdir("build/samir.audit/m005",0777);
    for(;*inputs;inputs++) {
        snprintf(path,sizeof path,"%s/mint/work/%s",corpus,*inputs);
        snprintf(dst,sizeof dst,"build/samir.audit/m005/%s",*inputs);
        CHECK(reference_copy(path,dst),"mint 005 input staged byte-identically");
    }
    snprintf(path,sizeof path,"%s/mint/work/%s",corpus,prg);
    f=fopen(path,"rb");CHECK(f!=NULL,"mint 005 program present in the corpus");
    if(!f) return;
    n=fread(m005_text,1,sizeof m005_text-1,f);fclose(f);m005_text[n]=0;
    for(p=m005_text;*p && nl<SCRIPT_MAX_LINES-1;) {
        char *e=p;
        while(*e && *e!='\r' && *e!='\n') e++;
        if(*e) *e++=0;
        while(*e=='\r' || *e=='\n') e++;
        if(*p && strncmp(p,"SET ALTERNATE",13) && strncmp(p,"ON ERROR",8) && strncmp(p,"QUIT",4)) {
            if(!strncmp(p,"LIST ",5) || !strncmp(p,"DISPLAY ",8)) {
                snprintf(dst,sizeof dst,"build/samir.audit/m005/L%02d.PRG",nl);
                f=fopen(dst,"wb");
                CHECK(f!=NULL,"mint 005 verbatim table command PRG opened");
                if(f) { fprintf(f,"%s\r\n",p);fclose(f); }
                snprintf(do_lines[nl],sizeof do_lines[nl],"DO L%02d",nl);
                lines[nl]=do_lines[nl];
            } else lines[nl]=p;
            nl++;
        }
        p=e;
    }
    snprintf(path,sizeof path,"%s/mint/work/%s",corpus,log);
    f=fopen(path,"rb");CHECK(f!=NULL,"mint 005 real dBASE log present");
    if(!f) return;
    n=fread(gold,1,sizeof gold-1,f);fclose(f);gold[n]=0;
    for(i=0,k=0;(size_t)i<n && k<sizeof want-1;i++) {
        if(gold[i]=='\r' || gold[i]==0x1a) continue;
        if(!strncmp(gold+i,"  ->ERR#",8) && (i==0 || gold[i-1]=='\n')) {
            const char *q=gold+i+8;
            while(*q>='0' && *q<='9' && k<sizeof want-3) want[k++]=*q++;
            want[k++]=' ';want[k++]=' ';
            i=(int)(q-gold);              /* skip the single blank after the code */
            while(gold[i+1] && gold[i+1]!='\r' && k<sizeof want-2) want[k++]=gold[++i];
            want[k++]='\n';               /* the catalog line ends its own line */
            continue;
        }
        want[k++]=gold[i];
    }
    want[k]=0;
    CHECK(getcwd(cwd,sizeof cwd)!=NULL && chdir("build/samir.audit/m005")==0,"mint 005 session directory entered");
    ip=xb_interp_make(&g_cap.pal);
    CHECK(ip!=NULL,"interpreter constructed");
    if(ip) {
        cap_clear(); script_reset();
        for(i=0;i<nl;i++) script_push(lines[i]);
        script_push("QUIT");
        g_cap.strip_prompt=1;
        CHECK(samir_repl(&g_cap.pal,ip)==0,"REPL returns cleanly");
        g_cap.strip_prompt=0;
        xb_interp_free(ip);
    }
    CHECK(chdir(cwd)==0,"mint 005 session directory left");
    m005_normalize(g_cap.buf,m005_norm,sizeof m005_norm);
    m005_normalize(want,gold,sizeof gold);
    rt_memcpy(want,gold,rt_strlen(gold)+1u);
    if(getenv("M005_TRACE") || strcmp(m005_norm,want)) {
        size_t j=0;
        while(m005_norm[j] && m005_norm[j]==want[j]) j++;
        if(strcmp(m005_norm,want)) {
            size_t b=j;
            while(b>0 && want[b-1]!='\n') b--;
            fprintf(stderr,"%s first differing line\n  real : %.*s\n  samir: %.*s\n",prg,
                    (int)strcspn(want+b,"\n"),want+b,(int)strcspn(m005_norm+b,"\n"),m005_norm+b);
        }
        fprintf(stderr,"%s: SAMIR transcript differs from real dBASE %s at byte %lu\n--- real ---\n%s\n--- samir ---\n%s\n",
                prg,log,(unsigned long)j,want,m005_norm);
    }
    CHECK(strcmp(m005_norm,want)==0,why);
}
#endif
static void run(samir_pal_t *pal, const char **lines, int n, const char *expected,
                const char *why)
{
    xb_interp *ip;
    int i;
    copy_fixture();
#ifndef TEST_AUDIT_USE
    CHECK(ndx_build(pal,"build/samir.audit/NAMES.NDX",0,10,"NAME",3,name_key,NULL)==0,"indexed fixture built");
#endif
    ip=xb_interp_make(pal);
    CHECK(ip!=NULL,"interpreter constructed");
    if(!ip) return;
    cap_clear(); script_reset();
    for(i=0;i<n;i++) script_push(lines[i]);
    script_push("quit");
    CHECK(samir_repl(pal,ip)==0,"REPL returns cleanly");
    CHECK(cap_has(expected),why);
    if(!cap_has(expected)) fprintf(stderr,"TRANSCRIPT:\n%s\n",g_cap.buf);
    xb_interp_free(ip);
}
#define RUN(lines,want,why) run(pal,lines,(int)(sizeof(lines)/sizeof(lines[0])),want,why)
int main(int argc,char **argv)
{
    (void)argc; (void)argv;
    struct pal_host_cfg cfg={99,12,31,16u*1024u*1024u};
    samir_pal_t *host=pal_host_make(cfg), *pal;
    CHECK(host!=NULL,"host PAL constructed");
    if(!host) return 2;
    pal=cap_pal_make(host);
#ifdef TEST_AUDIT_USE
    {
        const char *s[]={"use build/samir.audit/CLIENTS","list"};
        RUN(s,"PESTON","K09 short USE defaults to .DBF");
    }
    {
        const char *s[]={"use build/samir.audit/NOFILE"};
        RUN(s,"1  File does not exist.","K09 missing file is #1");
    }
    {
        const char *s[]={"use build/samir.audit/BAD.DBF"};
        FILE *f=fopen("build/samir.audit/BAD.DBF","wb");
        if(f){fputs("This is not a database",f);fclose(f);}
        RUN(s,"15  Not a dBASE database.","K09 malformed table is #15");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","list"};
        RUN(s,"WADDAMS","K09 explicit extension preserved in dotted path");
    }
    {
        const char *s[]={"use build/samir.audit/DENIED"};
        RUN(s,"29  File is not accessible.","K09 inaccessible file is #29 (injected PAL access failure)");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.TAB","list"};
        FILE *a=fopen("build/CLIENTS.DBF","rb"), *b=fopen("build/samir.audit/CLIENTS.TAB","wb");
        int ch;
        CHECK(a && b,"byte-identical explicit extension fixture opened");
        if(a && b) while((ch=fgetc(a))!=EOF) fputc(ch,b);
        if(a) fclose(a);
        if(b) fclose(b);
        RUN(s,"PESTON","K09 explicit nondefault extension preserved in dotted directory");
    }
    {
        const char *s[]={"use build/samir.audit/SHORTBODY.DBF"};
        FILE *a=fopen("build/CLIENTS.DBF","rb"), *b=fopen("build/samir.audit/SHORTBODY.DBF","wb");
        int ch;
        CHECK(a && b,"corrupt record-count fixture opened");
        if(a && b) while((ch=fgetc(a))!=EOF) fputc(ch,b);
        if(a) fclose(a);
        if(b) fclose(b);
        b=fopen("build/samir.audit/SHORTBODY.DBF","r+b");
        CHECK(b!=NULL,"corrupt record-count fixture writable");
        if(b) { fseek(b,4,SEEK_SET); fputc(4,b); fclose(b); }
        RUN(s,"15  Not a dBASE database.","K09 header declares more records than file, still #15 (corpus dbf.md invariant 2)");
    }

    {
        const char *corpus=argc>1 ? argv[1] : "../dbase3-decomp";
        char path[1024],gold[1024],tag[128],top[64];
        char *at,*end;
        FILE *f;
        size_t n=0;
        long count=-1;
        const char *script[]={"use build/samir.audit/REFCLI index build/samir.audit/REFIDX.NDX","? 'RECCOUNT=',reccount()","go top","? 'TOP=',trim(lastname)"};
        snprintf(path,sizeof(path),"%s/mint/work/SMKLOG.TXT",corpus);
        f=fopen(path,"rb");CHECK(f!=NULL,"real dBASE SMKLOG.TXT golden available");
        if(f){n=fread(gold,1,sizeof(gold)-1,f);fclose(f);}gold[n]=0;
        at=strstr(gold,"RECCOUNT=");
        CHECK(at!=NULL,"SMKLOG golden has RECCOUNT projection");
        if(at) count=strtol(at+9,NULL,10);
        CHECK(count>0,"SMKLOG golden count parsed");
        at=strstr(gold,"TOP=");
        CHECK(at!=NULL,"SMKLOG golden has indexed TOP projection");
        top[0]=0;
        if(at){
            at+=4;while(*at==' ')at++;
            end=at;while(*end && *end!='\r' && *end!='\n')end++;
            while(end>at && end[-1]==' ')end--;
            if((size_t)(end-at)<sizeof(top)){memcpy(top,at,(size_t)(end-at));top[end-at]=0;}
        }
        snprintf(path,sizeof(path),"%s/mint/work/CLIENTS.DBF",corpus);
        CHECK(reference_copy(path,"build/samir.audit/REFCLI.DBF"),"real CLIENTS RW copy prepared");
        snprintf(path,sizeof(path),"%s/mint/work/SMKIDX.NDX",corpus);
        CHECK(reference_copy(path,"build/samir.audit/REFIDX.NDX"),"real SMKIDX RW copy prepared");
        /* Grade values, not the pre-existing numeric whitespace/dot formatter.
         * Expected values are read from the real golden, never from SAMIR. */
        snprintf(tag,sizeof(tag),"RECCOUNT= %ld.",count);
        RUN(script,tag,"K09 real-dBASE golden: implicit USE record count");
        snprintf(tag,sizeof(tag),"TOP= %s",top);
        CHECK(top[0] && cap_has(tag),"K09 real-dBASE golden: same indexed TOP name");
    }
    {
        const char *corpus=argc>1 ? argv[1] : "../dbase3-decomp";
        static const char *const in[]={"TRAVEL.DBF","TRAVEL.DBT","TRIPS.NDX","AVAL_FLT.DBF","AVAL_FLT.NDX","FLT_NO.NDX",0};
        m005_replay(corpus,"M005U.PRG","M005ULOG.TXT",in,"M005U real-dBASE golden: USE INDEX defaults .NDX and starts at the index top");
        m005_replay(corpus,"M005N.PRG","M005NLOG.TXT",in,"M005N real-dBASE golden: NEXT n leaves the pointer ON the n-th record");
    }
    {
        /* initech-x5cc: SET state belongs to one interpreter. Session 1 sets
         * CENTURY ON and leaves it; session 2 is a NEW interpreter on the same
         * reused arena (same address) and must start at the III+ default
         * CENTURY OFF [corpus mint-results-003.md; set.h defaults]. */
        const char *first[]={"set century on","? ctod('07/24/1986')"};
        const char *second[]={"? ctod('07/24/86')"};
        RUN(first,"07/24/1986","x5cc first session renders SET CENTURY ON");
        RUN(second,"07/24/86","x5cc second session renders the default CENTURY OFF");
        CHECK(!cap_has("07/24/1986"),"x5cc SET CENTURY does not leak into a new interpreter at a reused address");
    }
    {
        /* The stored SET state too (set.c registry, read by set_get_safety):
         * SET SAFETY OFF in one session must not let the next one ZAP without
         * the confirmation (SAFETY defaults ON, Using III+ U5-257/U5-284). */
        const char *first[]={"set safety off"};
        const char *second[]={"use build/samir.audit/CLIENTS.DBF","zap"};
        RUN(first,". ","x5cc first session sets SAFETY OFF");
        RUN(second,"? (Y/N)","x5cc SET SAFETY does not leak into a new interpreter at a reused address");
    }
    {
        /* initech-x5cc, same root cause in proc.c: an ON ERROR trap belongs to
         * the interpreter that installed it (Using III+ ON ERROR). */
        const char *first[]={"on error ? 'TRAPPED'"};
        const char *second[]={"? 1 + 'a'"};
        RUN(first,". ","x5cc first session installs an ON ERROR trap");
        RUN(second,"9  Data type mismatch.","x5cc second session reports the plain catalog error (#9)");
        CHECK(!cap_has("TRAPPED"),"x5cc ON ERROR trap does not leak into a new interpreter at a reused address");
    }
#else
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","set filter to bal > 0","list bal off"};
        /* Re-keyed 2026-10-15 (initech-1y0h/r3pa): numeric cells render
         * right-aligned in their N(w,d) width (Using III+ U5-144), so BAL
         * N(8,2) prints " 1234.50". */
        RUN(s," 1234.50\r\n 7777.77\r\n","K05 LIST excludes negative balance");
        CHECK(!cap_has("-42"),"K05 hidden row absent");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","set filter to bal < 0","go top","? recno()","skip","? eof()","skip -1","? recno()","set filter to","go top","? recno()"};
        RUN(s,"\n2.","K05 GO TOP reaches passing physical record");
        CHECK(cap_has(".T."),"K05 filtered EOF");
        CHECK(cap_has("\n1."),"K05 clearing restores records");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","set filter to bal > 0","locate for .t.","? recno()","continue","? recno()"};
        RUN(s,"\n3.","K05 CONTINUE skips excluded row");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","set filter to .f.","go top","? eof()","list"};
        RUN(s,".T.","K05 empty filtered view has EOF");
        CHECK(!cap_has("PESTON"),"K05 empty view lists nothing");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","set filter to 123"};
        RUN(s,"37  Not a Logical expression.","K05 rejects nonlogical condition");
    }
    {
        /* III+ refuses duplicate open (Using U7-7); retain the per-area filter
         * oracle using a byte-identical second file, not duplicate USE. */
        FILE *in=fopen("build/CLIENTS.DBF","rb"),*out=fopen("build/samir.audit/OTHER.DBF","wb");
        int ch;
        CHECK(in && out,"second independent filter fixture copied");
        if(in && out)while((ch=fgetc(in))!=EOF)fputc(ch,out);
        if(in)fclose(in);
        if(out)fclose(out);
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","set filter to bal > 0","select 2","use build/samir.audit/OTHER.DBF alias other","set filter to bal < 0","go top","? recno()","select 1","go bottom","? recno()"};
        RUN(s,"\n3.","K05 filter survives area switch");
        CHECK(cap_has("\n2."),"K05 second area has separate filter");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","go 2","delete","display","list","set deleted on","list off name","go 2","display","set deleted off","recall","display"};
        RUN(s,"*      2  WADDAMS","K06 marker precedes deleted record");
        CHECK(!cap_has("File already exists"),"K06 SET DELETED recognized");
        /* initech-31d1: M005U/M005N real III+ transcripts use 7 record-number
         * columns + TWO spaces. Deleted-marker placement remains authored:
         * M005 contains no deleted rows, so that placement is unchanged. */
        CHECK(cap_has("      2  WADDAMS"),"K06 RECALL removes marker");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","go 2","delete","set deleted on","set filter to bal < 0","go top","? eof()","set deleted off","go top","? recno()"};
        RUN(s,".T.","K06 deleted and filter compose");
        CHECK(cap_has("\n2."),"K06 OFF restores deleted record");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","set filter to bal > 0","replace all city with 'VISIBLE'","set filter to","go 2","display off city"};
        RUN(s,"AUSTIN","K05 scoped REPLACE leaves hidden record untouched");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","go 2","delete","set deleted on","go top","skip","? recno()","go 2","list next 1 off name"};
        RUN(s,"\n3.","K06 SKIP excludes deleted record");
        CHECK(cap_has("WADDAMS"),"K06 NEXT scope includes deleted (U5-215)");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF index build/samir.audit/NAMES.NDX","set filter to bal > 0","go top","? recno()","skip","? recno()","skip","? eof()","skip -1","? recno()","go 3","skip","? recno()"};
        RUN(s,"\n3.","K05 indexed TOP follows NAME order");
        CHECK(cap_has("\n1."),"K05 indexed SKIP bypasses excluded record");
        CHECK(cap_has(".T."),"K05 indexed filtered EOF");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF index build/samir.audit/NAMES.NDX","set filter to bal > 0","seek 'WADDAMS'","? found()","find PESTON","? found()"};
        RUN(s,".F.","K05 SEEK cannot find excluded key");
        CHECK(cap_has(".T."),"K05 FIND still finds visible key");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","store 0 to cut","set filter to bal > cut","store 8000 to cut","go top","? eof()"};
        RUN(s,".T.","K05 filter reevaluates current memory variable");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","set filter to IIF(recno()=2,3,.T.)","list"};
        RUN(s,"37  Not a Logical expression.","K05 later predicate type error fails loud instead of being hidden");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","set filter to bal > 0","delete all","set filter to","go 2","? deleted()"};
        RUN(s,".F.","K05 scoped DELETE preserves hidden row");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","go top","delete","set deleted on","recall all","set deleted off","go top","? deleted()"};
        RUN(s,".T.","K06 RECALL ALL cannot see hidden deleted row (U5-215)");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF index build/samir.audit/NAMES.NDX","set filter to bal < 0","seek 'MATCH'","? found(),recno()"};
        duplicate_names=1;
        RUN(s,".T. 2.","K05 SEEK skips excluded first duplicate and finds visible second");
        duplicate_names=0;
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","set filter to bal > 0","use build/samir.audit/CLIENTS.DBF","go top","skip","? recno()"};
        RUN(s,"\n2.","K05 reopening table clears previous filter");
    }
    {
        const char *on[]={"use build/samir.audit/CLIENTS.DBF","set deleted on","go top"};
        const char *fresh[]={"use build/samir.audit/CLIENTS.DBF","go top","delete","list"};
        RUN(on,". ","K06 first session enables DELETED");
        RUN(fresh,"*      1  PESTON","K06 new interpreter defaults DELETED OFF on reused arena");
        CHECK(cap_has("1234.50"),"K06 fresh view renders numeric fields after arena reuse");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","set filter to IIF(recno()=2,3,.T.)","list","go 999"};
        RUN(s,"5  Record is out of range.","K05 filter fault does not mislabel a later GO range error");
    }
    /* BOF/EOF flags: Using III+ U5-268, U6-13/27 and corpus function specs.
     * Exact filtered BOF cursor clamp is authored, no local reference golden. */
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF","set filter to bal < 0","go top","skip -1","? bof(),eof(),recno()"};
        RUN(s,".T. .F. 2.","K05 backward boundary stays at first visible record, BOF without EOF");
    }
    {
        const char *s[]={"use build/samir.audit/CLIENTS.DBF index build/samir.audit/NAMES.NDX","set filter to bal < 0","go top","skip -1","? bof(),eof(),recno()"};
        RUN(s,".T. .F. 2.","K05 indexed backward boundary is the filtered first record");
    }
#endif
    pal_host_free(host);
    return TEST_SUMMARY("test-samir-audit");
}
