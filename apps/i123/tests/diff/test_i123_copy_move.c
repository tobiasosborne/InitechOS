/* Factory C independent coordinate model, R2.2 Reference /Copy pp. 3-2..6
 * and /Move pp. 7-2..4, CORRECTED to the real 1-2-3 R2.2 bytes (bead
 * initech-nn46; ../lotus123-decomp re/mint-results-002.md and goldens/
 * minted/{COPY01,MOVE01,WRAP01}?.WK1): an overwritten reference shows ERR
 * (not @ERR) and is stored as col/row words 0xFFFF (MOVE01B E1/E2/H1);
 * /Copy keeps every reference byte and both axes wrap -- column modulo
 * 256 (WRAP01B +IV1+1), row modulo 8192 (WRAP01C +A8192+1, not ERR);
 * /Move re-encodes a followed corner, and every corner of a moved formula,
 * with a relative column in the 8-bit form 0x8000|(delta&0xFF) (MOVE01C F5
 * fb80, MOVE01D F1 fb80) and a relative row in the 14-bit form (fdbf).
 * Text and byte expectations never use artifact ref helpers.
 * --corpus: the differential against real 1-2-3 R2.2 /Copy and /Move
 * goldens (initech-cjot, lotus123-decomp re/mint-results-002.md). */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include "i123.h"
static uint8_t mem[2][131072], cm[I123_CALC_BYTES(256)];
static i123_calc_t cx;
static unsigned rng=0x123a55,checks,failures;
static unsigned random_(unsigned n) { rng=rng*1664525u+1013904223u; return (rng>>8)%n; }
#define CHECK(x,why) do {checks++; if (!(x)) {failures++; if(failures<8) fprintf(stderr,"RED-REASON %s line %d\n",why,__LINE__);}}while(0)
static void ref(char *out,int c,int r,int ac,int ar)
{ sprintf(out,"%s%c%s%d",ac?"$":"",'A'+c,ar?"$":"",r+1); }
static void text(i123_sheet_t *s,int c,int r,char *out)
{ const i123_cell_t *q=i123_find(s,c,r); out[0]=0; if(q) CHECK(i123_formula_text(s->pool+q->off,q->len,c,r,out,512)==0,"decompile"); }
/* The code bytes of the formula at (c,r), or 0. */
static const uint8_t *code_at(const i123_sheet_t *s,int c,int r,uint32_t *len)
{
    const i123_cell_t *q=i123_find(s,c,r);
    if(!q||q->kind!=I123_K_FORMULA)return 0;
    *len=q->len;return s->pool+q->off;
}
static unsigned le16(const uint8_t *p) { return (unsigned)p[0]|((unsigned)p[1]<<8); }
/* The two words one corner must hold after a /Move re-encoded it, from the
 * real goldens: absolute = the coordinate, relative column = 8-bit form,
 * relative row = 14-bit two's complement, ERR = ffff ffff. */
static void moved_words(int err,int c,int r,int hc,int hr,int ac,int ar,unsigned *cw,unsigned *rw)
{
    if(err){*cw=*rw=0xFFFFu;return;}
    *cw=ac?(unsigned)c:0x8000u|((unsigned)(c-hc)&0xFFu);
    *rw=ar?(unsigned)r:0x8000u|((unsigned)(r-hr)&0x3FFFu);
}
/* ---- corpus differential (initech-cjot) ---------------------------------
 * Every <corpus>/mint/NAME.steps file that types /Copy or /Move is replayed:
 * the golden saved BEFORE a command (real 1-2-3 bytes) is loaded, the
 * command typed in the steps file is applied with i123_transfer, the sheet
 * is recalculated (the goldens are AUTOMATIC), written, and graded against
 * the golden real 1-2-3 saved AFTER the command. Both files are decoded by
 * the corpus' independent reader only (wk1_ref.py for every cell record,
 * wk1_formula_ref.py --bytes for every FORMULA body); this file parses no
 * reference bytes and derives no expectation of its own. */
static uint8_t cin_[65536],cout_[65536],cbank[2][131072],ccalc[I123_CALC_BYTES(256)];
static int reader(const char *corpus_dir,const char *script,int bytes,const char *file,const char *result)
{
    pid_t pid=fork();if(pid<0)return 1;
    if(!pid) {
        int fd=open(result,O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0)_exit(2);
        if(dup2(fd,1)<0)_exit(2);
        close(fd);if(setenv("LOTUS123_DECOMP",corpus_dir,1))_exit(2);
        if(bytes)execlp("python3","python3",script,"--bytes",file,(char *)0);
        else execlp("python3","python3",script,file,(char *)0);
        _exit(2);
    }
    int status=0;if(waitpid(pid,&status,0)<0)return 1;
    return !WIFEXITED(status)||WEXITSTATUS(status);
}
/* The cell records of a wk1_ref.py dump (header records are not graded). */
static int cell_line(const char *l)
{
    static const char *const k[]={"  INTEGER ","  NUMBER ","  LABEL ","  FORMULA ","  STRING ","  BLANK "};
    for(unsigned i=0;i<sizeof k/sizeof k[0];i++)if(!strncmp(l,k[i],strlen(k[i])))return 1;
    return 0;
}
static unsigned load_lines(const char *path,int cells_only,char lines[][256],unsigned cap)
{
    FILE *f=fopen(path,"r");unsigned n=0;char l[256];if(!f)return 0;
    while(n<cap&&fgets(l,sizeof l,f)) {
        l[strcspn(l,"\r\n")]=0;
        if(cells_only&&!cell_line(l))continue;
        strcpy(lines[n++],l);
    }
    fclose(f);return n;
}
static char gl[512][256],al[512][256];
/* 1 when the two decodes agree; prints every differing line otherwise. */
static int same_decode(const char *name,const char *what,const char *g,const char *a,int cells_only)
{
    unsigned ng=load_lines(g,cells_only,gl,512),na=load_lines(a,cells_only,al,512),bad=0;
    if(!ng){fprintf(stderr,"!!! corpus %s: empty %s decode of the golden\n",name,what);return 0;}
    for(unsigned i=0;i<ng||i<na;i++) {
        const char *x=i<ng?gl[i]:"(none)",*y=i<na?al[i]:"(none)";
        if(strcmp(x,y)){if(!bad++)fprintf(stderr,"RED-REASON corpus %s %s\n",name,what);
            fprintf(stderr,"    real 1-2-3: %s\n    artifact  : %s\n",x,y);}
    }
    return !bad;
}
/* Factory address parser (plain A1 / A1..B2, as typed at the FROM/TO prompts). */
static int addr(const char **t,uint16_t *c,uint16_t *r)
{
    const char *p=*t;unsigned col=0,row=0;
    while(*p>='A'&&*p<='Z'&&col<=256)col=col*26+(unsigned)(*p++-'A'+1);
    while(*p>='0'&&*p<='9'&&row<=8192)row=row*10+(unsigned)(*p++-'0');
    if(!col||col>256||!row||row>8192)return 0;
    *c=(uint16_t)(col-1);*r=(uint16_t)(row-1);*t=p;return 1;
}
static int range_text(const char *t,i123_rect_t *rg)
{
    if(!addr(&t,&rg->c0,&rg->r0))return 0;
    rg->c1=rg->c0;rg->r1=rg->r0;
    if(t[0]=='.'&&t[1]=='.'){t+=2;if(!addr(&t,&rg->c1,&rg->r1))return 0;}
    return !*t&&rg->c0<=rg->c1&&rg->r0<=rg->r1;
}
static char sl[4096][256];
/* Independently minted TO rectangles, fixtures/i123-copy step files. Neither
 * the expected extent nor the expected cell values come from range.c. */
static int tiles(const char *dir,const char *build,const char *formula_ref)
{
    const char *corpus_dir=getenv("LOTUS123_DECOMP");
    if(!corpus_dir)return 2;
    char ref[1024];snprintf(ref,sizeof ref,"%s/tools/wk1_ref.py",corpus_dir);
    static const struct {const char *name;i123_rect_t from,to;} cases[]={
        {"SINGLE",{0,0,0,0},{3,0,3,2}}, {"ROW",{0,0,1,0},{5,0,7,2}},
        {"OVER",{0,0,1,0},{0,0,3,1}}, {"BLOCK",{0,0,1,1},{5,0,8,3}}
    };
    i123_calc_t ccx;if(i123_calc_init(&ccx,ccalc,sizeof ccalc,256,0))return 2;
    unsigned bad=0;
    for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++) {
        i123_sheet_t s,d;uint32_t n;
        if(i123_sheet_init(&s,cbank[0],sizeof cbank[0],256)||i123_sheet_init(&d,cbank[1],sizeof cbank[1],256))return 2;
        if(i==2) {
            char path[1024];snprintf(path,sizeof path,"%s/ROW.WK1",dir);
            FILE *f=fopen(path,"rb");if(!f)return 2;n=(uint32_t)fread(cin_,1,sizeof cin_,f);fclose(f);
            if(i123_wk1_read(&s,cin_,n))return 2;
        } else {
            if(i123_set_number(&s,0,0,i==0?7:i==3?1:11))return 2;
            if(i&&i123_set_number(&s,1,0,i==3?2:22))return 2;
            if(i==3&&(i123_set_number(&s,0,1,3)||i123_set_number(&s,1,1,4)))return 2;
        }
        const char *name=cases[i].name;
        if(i123_transfer_rect(&s,&d,cases[i].from,cases[i].to,0)||i123_after_entry(&ccx,&d)||i123_wk1_write(&d,cout_,sizeof cout_,&n)) {
            fprintf(stderr,"RED-REASON tile %s refused\n",name);bad++;continue;
        }
        char golden[1024],out[1024],ga[1024],aa[1024];
        snprintf(golden,sizeof golden,"%s/%s.WK1",dir,name);
        snprintf(out,sizeof out,"%s/i123-tile-%s.WK1",build,name);
        FILE *f=fopen(out,"wb");if(!f||fwrite(cout_,1,n,f)!=n)return 2;fclose(f);
        snprintf(ga,sizeof ga,"%s/i123-tile-%s.golden.cells",build,name);
        snprintf(aa,sizeof aa,"%s/i123-tile-%s.artifact.cells",build,name);
        if(reader(corpus_dir,ref,0,golden,ga)||reader(corpus_dir,ref,0,out,aa))return 2;
        int ok=same_decode(name,"tile cell records",ga,aa,1);bad+=!ok;
        printf("tile %s: %s\n",name,ok?"agrees with real R2.2":"RED");
    }
    (void)formula_ref;
    return bad?1:0;
}
static int skip(const char *l) { return !strncmp(l,"sleep ",6)||!strncmp(l,"shot ",5)||!l[0]||l[0]=='#'; }
static unsigned next_line(unsigned i,unsigned n) { while(i<n&&skip(sl[i]))i++; return i; }
static int corpus(const char *corpus_dir,const char *build,const char *formula_ref)
{
    struct dirent **list;char path[1024],ref[1024];
    snprintf(path,sizeof path,"%s/mint",corpus_dir);
    int nf=scandir(path,&list,0,alphasort);if(nf<0){perror("mint corpus");return 2;}
    snprintf(ref,sizeof ref,"%s/tools/wk1_ref.py",corpus_dir);
    unsigned copies=0,moves=0,pairs=0,red=0,files=0;
    i123_calc_t ccx;
    if(i123_calc_init(&ccx,ccalc,sizeof ccalc,256,0))return 2;
    for(int fi=0;fi<nf;fi++) {
        const char *fname=list[fi]->d_name;size_t fl=strlen(fname);
        if(fl<7||strcmp(fname+fl-6,".steps"))continue;
        snprintf(path,sizeof path,"%s/mint/%s",corpus_dir,fname);
        FILE *f=fopen(path,"r");if(!f)return 2;
        unsigned n=0;while(n<4096&&fgets(sl[n],sizeof sl[n],f)){sl[n][strcspn(sl[n],"\r\n")]=0;n++;}
        fclose(f);
        int has=0;for(unsigned i=0;i<n;i++)if(!strncmp(sl[i],"type /c",7)||!strncmp(sl[i],"type /m",7))has=1;
        if(!has)continue;
        files++;
        i123_sheet_t bank[2];int cur=0,loaded=0;unsigned pending=0;char opdesc[256]="";
        for(unsigned i=0;i<n;i++) {
            if(!strcmp(sl[i],"type /fs")) {
                unsigned j=next_line(i+1,n);
                if(j>=n||strncmp(sl[j],"type ",5)){fprintf(stderr,"!!! corpus %s: /fs without a name at line %u\n",fname,i+1);return 2;}
                const char *name=sl[j]+5;i=j;
                char golden[1024],ga[1024],aa[1024],gb[1024],ab[1024],out[1024];
                snprintf(golden,sizeof golden,"%s/goldens/minted/%s.WK1",corpus_dir,name);
                FILE *g=fopen(golden,"rb");if(!g){fprintf(stderr,"!!! corpus golden %s missing\n",golden);return 2;}
                uint32_t len=(uint32_t)fread(cin_,1,sizeof cin_,g);fclose(g);
                if(loaded&&pending) {
                    i123_sheet_t *s=&bank[cur];uint32_t olen;
                    if(i123_after_entry(&ccx,s)||i123_wk1_write(s,cout_,sizeof cout_,&olen)){fprintf(stderr,"RED-REASON corpus %s: artifact refused recalc/write\n",name);return 1;}
                    snprintf(out,sizeof out,"%s/i123-copy-move-%s.WK1",build,name);
                    FILE *o=fopen(out,"wb");if(!o||fwrite(cout_,1,olen,o)!=olen)return 2;
                    fclose(o);
                    snprintf(ga,sizeof ga,"%s/i123-copy-move-%s.golden.cells",build,name);
                    snprintf(aa,sizeof aa,"%s/i123-copy-move-%s.artifact.cells",build,name);
                    snprintf(gb,sizeof gb,"%s/i123-copy-move-%s.golden.bytes",build,name);
                    snprintf(ab,sizeof ab,"%s/i123-copy-move-%s.artifact.bytes",build,name);
                    if(reader(corpus_dir,ref,0,golden,ga)||reader(corpus_dir,ref,0,out,aa)||
                       reader(corpus_dir,formula_ref,1,golden,gb)||reader(corpus_dir,formula_ref,1,out,ab)){fprintf(stderr,"!!! corpus %s: independent reader failed\n",name);return 2;}
                    int ok=same_decode(name,"cell records (wk1_ref.py)",ga,aa,1);
                    ok&=same_decode(name,"FORMULA bodies (wk1_formula_ref.py --bytes)",gb,ab,0);
                    pairs++;if(!ok)red++;
                    printf("corpus %-8s after%s: %s\n",name,opdesc,ok?"agrees with real 1-2-3":"RED");
                }
                i123_sheet_t *s=&bank[cur];
                if(i123_sheet_init(s,cbank[cur],sizeof cbank[cur],256)||i123_wk1_read(s,cin_,len)){fprintf(stderr,"!!! corpus %s: artifact cannot load the golden\n",name);return 2;}
                loaded=1;pending=0;opdesc[0]=0;continue;
            }
            int mv=!strncmp(sl[i],"type /m",7);
            if(!mv&&strncmp(sl[i],"type /c",7))continue;
            unsigned a=next_line(i+1,n),b=next_line(a+1,n),c=next_line(b+1,n);
            i123_rect_t from,to;
            if(c>=n||strcmp(sl[a],"key Return")||strncmp(sl[b],"type ",5)||strcmp(sl[c],"key Return")||
               !range_text(sl[i]+7,&from)||!range_text(sl[b]+5,&to)||to.c0!=to.c1||to.r0!=to.r1) {
                fprintf(stderr,"!!! corpus %s line %u: not the 'type /cFROM, key Return, type TO, key Return' form\n",fname,i+1);return 2;
            }
            if(!loaded){fprintf(stderr,"!!! corpus %s line %u: command before any saved golden\n",fname,i+1);return 2;}
            int other=1-cur;
            if(i123_sheet_init(&bank[other],cbank[other],sizeof cbank[other],256))return 2;
            int rc=i123_transfer(&bank[cur],&bank[other],from,to.c0,to.r0,mv?I123_TRANSFER_MOVE:I123_TRANSFER_COPY);
            if(rc){fprintf(stderr,"RED-REASON corpus %s line %u: artifact refused the transfer (%d)\n",fname,i+1,rc);return 1;}
            cur=other;pending++;if(mv)moves++;else copies++;
            size_t dl=strlen(opdesc);snprintf(opdesc+dl,sizeof opdesc-dl," /%c%s->%s",mv?'m':'c',sl[i]+7,sl[b]+5);
            i=c;
        }
        if(pending){fprintf(stderr,"!!! corpus %s: a command is not followed by a saved golden\n",fname);return 2;}
    }
    for(int fi=0;fi<nf;fi++)free(list[fi]);
    free(list);
    if(!copies||!moves||!pairs) {
        fprintf(stderr,"!!! test-i123-copy-move: corpus evidence unavailable: %u /Copy and %u /Move mint commands; real command goldens required\n",copies,moves);
        return 2;
    }
    printf("i123-copy-move corpus: %u before/after pairs (%u /Copy, %u /Move) from %u step files, %u RED\n",pairs,copies,moves,files,red);
    return red?1:0;
}
int main(int argc,char **argv)
{
    if(argc==5&&!strcmp(argv[1],"--saved")) {
        const char *corpus_dir=getenv("LOTUS123_DECOMP");char ref[1024],a[1024],b[1024];
        if(!corpus_dir)return 2;
        snprintf(ref,sizeof ref,"%s/tools/wk1_ref.py",corpus_dir);
        snprintf(a,sizeof a,"%s/i123-saved.golden.cells",argv[4]);
        snprintf(b,sizeof b,"%s/i123-saved.artifact.cells",argv[4]);
        if(reader(corpus_dir,ref,0,argv[2],a)||reader(corpus_dir,ref,0,argv[3],b))return 2;
        return same_decode(argv[2],"saved cell records",a,b,1)?0:1;
    }
    if(argc==5&&!strcmp(argv[1],"--corpus"))return corpus(argv[2],argv[3],argv[4]);
    if(argc==5&&!strcmp(argv[1],"--tile"))return tiles(argv[2],argv[3],argv[4]);
    if(argc!=1)return 2;
    i123_sheet_t sh,dst;
    CHECK(i123_calc_init(&cx,cm,sizeof cm,256,0)==0,"init");
    for(unsigned trial=0;trial<4000;trial++) {
        int moving=trial&1, sc=4+random_(4),sr=4+random_(6), dc=1+random_(10),dr=1+random_(12);
        int tc[2]={ (int)random_(16),(int)random_(22) },tr[2]={ (int)random_(20),(int)random_(22) };
        int ac[2]={ (int)random_(2),(int)random_(2) },ar[2]={ (int)random_(2),(int)random_(2) };
        char a[32],b[32],formula[100],expected[512],got[512];
        i123_rect_t rg={(uint16_t)sc,(uint16_t)sr,(uint16_t)(sc+1),(uint16_t)(sr+1)};
        if(trial%3==0) {tc[0]=sc;tr[0]=sr+1;} /* moved precedent */
        if(trial%5==0) {tc[1]=dc;tr[1]=dr+1;} /* moved-over precedent */
        CHECK(i123_sheet_init(&sh,mem[0],sizeof mem[0],256)==0,"init");
        CHECK(i123_sheet_init(&dst,mem[1],sizeof mem[1],256)==0,"init");
        sh.calcmode=0;
        for(int r=0;r<24;r++) for(int c=0;c<10;c++) CHECK(i123_set_number(&sh,c,r,c+r)==0,"number");
        ref(a,tc[0],tr[0],ac[0],ar[0]);ref(b,tc[1],tr[1],ac[1],ar[1]);
        if(trial%4==2)sprintf(formula,"@SUM(%s..%s)",a,b);else sprintf(formula,"+%s+%s",a,b);
        CHECK(i123_enter_formula(&cx,&sh,sc,sr,formula,strlen(formula))==0,"formula");
        CHECK(i123_enter_formula(&cx,&sh,23,23,formula,strlen(formula))==0,"formula");
        CHECK(i123_transfer(&sh,&dst,rg,dc,dr,moving)==0,"transfer");
        int still=moving&&dc==sc&&dr==sr; /* a /Move onto itself [inferred] */
        for(int outside=0;outside<2;outside++) {
            int nc=outside?23:dc,nr=outside?23:dr,oc=outside?23:sc,orow=outside?23:sr;
            char parts[2][32];
            uint32_t ol=0,nl=0;
            const uint8_t *oldc=code_at(&sh,oc,orow,&ol),*newc=code_at(&dst,nc,nr,&nl);
            CHECK(oldc&&newc&&ol==nl,outside?"move-referrers":"absolute-relative");
            for(int k=0;k<2;k++) {
                int c=tc[k],r=tr[k],err=0,followed=0;
                if(moving&&!still) {
                    if(c>=sc&&c<=sc+1&&r>=sr&&r<=sr+1) {c+=dc-sc;r+=dr-sr;followed=1;}
                    else if(c>=dc&&c<=dc+1&&r>=dr&&r<=dr+1) err=1;
                } else if(!moving&&!outside) {if(!ac[k])c+=dc-sc;if(!ar[k])r+=dr-sr;}
                /* Both axes wrap: column modulo 256 (WRAP01B), row modulo
                 * 8192 (WRAP01C) -- an off-sheet copy is never ERR. */
                if(c<0)c+=256;
                if(r<0)r+=8192;
                if(err)strcpy(parts[k],"ERR");
                else if(c>=26) { /* independent A..IV spelling */
                    sprintf(parts[k],"%s%c%c%s%d",ac[k]?"$":"",'A'+c/26-1,'A'+c%26,ar[k]?"$":"",r+1);
                } else ref(parts[k],c,r,ac[k],ar[k]);
                /* the bytes: /Copy and an untouched corner keep the source
                 * words; a re-encoded corner holds moved_words() */
                if(oldc&&newc&&ol==nl) {
                    uint32_t at=trial%4==2?1u+4u*(uint32_t)k:1u+5u*(uint32_t)k;
                    unsigned cw=le16(oldc+at),rw=le16(oldc+at+2);
                    if(moving&&!still&&(err||followed||!outside))
                        moved_words(err,c,r,nc,nr,ac[k],ar[k],&cw,&rw);
                    CHECK(le16(newc+at)==cw&&le16(newc+at+2)==rw,moving?"move-encoding":"copy-bytes");
                }
            }
            if(trial%4==2) {
                if(!strcmp(parts[0],"ERR")||!strcmp(parts[1],"ERR"))strcpy(expected,"@SUM(ERR)");
                else sprintf(expected,"@SUM(%s..%s)",parts[0],parts[1]);
            } else sprintf(expected,"%s%s+%s",parts[0][0]=='$'?"":"+",parts[0],parts[1]);
            text(&dst,nc,nr,got);
            CHECK(!strcmp(got,expected),outside?"move-referrers":"absolute-relative");
        }
    }
    CHECK(i123_sheet_init(&sh,mem[0],sizeof mem[0],256)==0,"init");
    CHECK(i123_sheet_init(&dst,mem[1],sizeof mem[1],256)==0,"init");
    i123_rect_t rg={0,0,1,0},found;
    CHECK(i123_name_set(&sh,"TOTAL",rg)==0,"name-create");
    CHECK(i123_name_find(&sh,"total",5,&found)==0&&found.c1==1,"name-find");
    CHECK(i123_set_number(&sh,0,0,3)==0&&i123_set_number(&sh,1,0,4)==0,"numbers");
    CHECK(i123_enter_formula(&cx,&sh,4,0,"@SUM(TOTAL)",11)==0,"name-formula");
    i123_val_t v; i123_cell_value(&sh,4,0,&v); CHECK(v.t==I123_V_NUM&&v.n==7,"name-value");
    CHECK(i123_transfer(&sh,&dst,rg,0,2,1)==0,"named-move");
    CHECK(i123_name_find(&dst,"TOTAL",5,&found)==0&&found.r0==2&&found.r1==2,"name-move");
    CHECK(i123_recalc(&cx,&dst)==0,"recalc"); i123_cell_value(&dst,4,0,&v);
    CHECK(v.t==I123_V_NUM&&v.n==7,"name-referrer");
    CHECK(i123_name_delete(&dst,"TOTAL")==0&&i123_name_find(&dst,"TOTAL",5,&found)!=0,"name-delete");
    CHECK(i123_name_set(&dst,"ZZZ",rg)==0&&i123_name_set(&dst,"AAA",rg)==0,"name-create");
    CHECK(i123_name_table(&dst,10,10)==0,"name-table");
    char table[512];i123_cell_content(&dst,i123_find(&dst,10,10),table,sizeof table);
    CHECK(!strcmp(table,"'AAA"),"name-table-alphabetical");
    i123_cell_content(&dst,i123_find(&dst,10,11),table,sizeof table);
    CHECK(!strcmp(table,"'ZZZ"),"name-table-alphabetical");
    CHECK(!i123_find(&dst,10,12)&&!i123_find(&dst,11,12),"name-table-blank-row");
    CHECK(i123_name_set(&sh,"TOTAL",(i123_rect_t){3,3,4,3})==I123_E_ARG,"name-rebind-refused");
    CHECK(i123_sheet_init(&dst,mem[1],sizeof mem[1],1)==0,"init");
    CHECK(i123_transfer(&sh,&dst,rg,0,2,0)==I123_E_FULL,"capacity-refusal");
    i123_cell_value(&sh,0,0,&v);CHECK(v.t==I123_V_NUM&&v.n==3,"source-intact");
    CHECK(i123_transfer(&sh,&sh,rg,0,2,0)==I123_E_ARG,"alias-refusal");
    CHECK(i123_sheet_init(&sh,mem[0],sizeof mem[0],256)==0&&
          i123_sheet_init(&dst,mem[1],sizeof mem[1],256)==0,"tile-init");
    CHECK(i123_set_number(&sh,0,0,7)==0,"tile-source");
    i123_rect_t one={0,0,0,0},edge={255,8191,255,8191};
    CHECK(i123_transfer_rect(&sh,&dst,one,edge,0)==0,"tile-last-cell");
    CHECK(i123_transfer_rect(&sh,&dst,(i123_rect_t){0,0,1,0},edge,0)==I123_E_ARG,"tile-boundary-refusal");
    sh.protec=255;
    CHECK(i123_transfer_rect(&sh,&dst,one,(i123_rect_t){3,0,3,2},0)==I123_E_ARG,"tile-protected-absent");
    for(unsigned r=0;r<3;r++)CHECK(i123_set_cell_format(&sh,3,r,0x7f)==0,"tile-unprotect");
    CHECK(i123_transfer_rect(&sh,&dst,one,(i123_rect_t){3,0,3,2},0)==0,"tile-unprotected");
    CHECK(i123_set_cell_format(&sh,3,2,0xff)==0&&
          i123_transfer_rect(&sh,&dst,one,(i123_rect_t){3,0,3,2},0)==I123_E_ARG,"tile-protected-last");
    sh.protec=0;
    const char *mixed="+A1+$A1+A$1+$A$1";
    CHECK(i123_enter_formula(&cx,&sh,1,1,mixed,(uint32_t)strlen(mixed))==0,"tile-mixed-formula");
    CHECK(i123_transfer_rect(&sh,&dst,(i123_rect_t){1,1,1,1},(i123_rect_t){4,3,5,5},0)==0,"tile-mixed-copy");
    const i123_cell_t *source=i123_find(&sh,1,1);
    for(unsigned r=3;r<6;r++)for(unsigned c=4;c<6;c++) {
        const i123_cell_t *q=i123_find(&dst,c,r);
        CHECK(q&&q->len==source->len&&!memcmp(sh.pool+source->off,dst.pool+q->off,q->len),"tile-copy-bytes");
    }
    printf("i123-copy-move: %u checks, %u failures (4000 random sheets)\n",checks,failures);
    return failures?1:0;
}
