/* Audit L001/L003/L007: independent byte and lifetime oracles.
 * Ref: dbf.md ss2/4/6/8; Using dBase III Plus.pdf U7-7 (#3),
 * U5-284 (ZAP prompt), U5-249 (SAFETY default ON).
 * Failure atomicity and abort/EOF rules: authored, no local reference.
 * Fixtures are factory-authored DBFs; dbf_ref.py independently reopens them.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "test_assert.h"
#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/dbf.h"
#include "samir/dbt.h"
#include "samir/ndx.h"
TEST_HARNESS();
struct pal_host_cfg { uint8_t yy,mm,dd; uint32_t heap_size; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(samir_pal_t *);
extern int samir_repl(samir_pal_t *,xb_interp *);
static struct {
    samir_pal_t p; samir_pal_t *inner; xb_interp *ip;
    const char *script[32]; int line;
    char out[16384]; size_t used;
    int armed, boundary, hit, calls, seek_fault, short_kind, error;
    void *live_mark; int reset_bad, closes, opens;
} c;
static const char *path="build/dbsafe/FAULT.DBF";
static pal_fd op(samir_pal_t *p,const char *s,int m){(void)p;c.opens++;return c.inner->open(c.inner,s,m);}
static int cl(samir_pal_t *p,pal_fd f){(void)p;c.closes++;return c.inner->close(c.inner,f);}
static int32_t rd(samir_pal_t *p,pal_fd f,void *b,uint32_t n){(void)p;return c.inner->read(c.inner,f,b,n);}
static int32_t wr(samir_pal_t *p,pal_fd f,const void *b,uint32_t n){
    (void)p;
    if(c.armed && !c.seek_fault && ++c.calls==c.boundary){
        uint32_t k=0;c.hit=1;
        if(!c.short_kind) return -c.error;
        if(c.short_kind==2) k=1;
        if(c.short_kind==3) k=n/2;
        if(c.short_kind==4) k=n-1;
        if(k>=n) k=n-1;
        return k?c.inner->write(c.inner,f,b,k):0;
    }
    return c.inner->write(c.inner,f,b,n);
}
static int32_t sk(samir_pal_t *p,pal_fd f,int32_t o,int w){
    (void)p;
    if(c.armed && c.seek_fault && ++c.calls==c.boundary){c.hit=1;return -PAL_EIO;}
    return c.inner->seek(c.inner,f,o,w);
}
static int rmf(samir_pal_t *p,const char *s){(void)p;return c.inner->remove(c.inner,s);}
static int rn(samir_pal_t *p,const char *a,const char *b){(void)p;return c.inner->rename(c.inner,a,b);}
static void co(samir_pal_t *p,const char *s,uint32_t n){(void)p;if(c.used+n<sizeof c.out){memcpy(c.out+c.used,s,n);c.used+=n;c.out[c.used]=0;}}
static int32_t ci(samir_pal_t *p,char *b,uint32_t n){
    const char *s=c.script[c.line++];(void)p;
    if(c.armed){
        if(c.hit){CHECK(dbf_nrec(wa_table(xb_interp_env(c.ip),1))==255,"L001 in-memory committed count");CHECK(wa_recno(xb_interp_env(c.ip),1)==1,"L001 refused append preserves cursor");}
        c.armed=0;
    }
    if(!s)return -1;
    if(!strcmp(s,"append blank"))c.armed=1;
    snprintf(b,n,"%s",s);return (int32_t)strlen(b);
}
static void td(samir_pal_t *p,uint8_t *a,uint8_t *b,uint8_t *d){(void)p;c.inner->today(c.inner,a,b,d);}
static void *al(samir_pal_t *p,uint32_t n){(void)p;return c.inner->alloc(c.inner,n);}
static void rs(samir_pal_t *p,void *m){
    uint8_t *end=c.inner->alloc(c.inner,0),*at=m;(void)p;
    if(at>end || (c.live_mark && at<(uint8_t *)c.live_mark)){
        c.reset_bad++;CHECK(0,"L003 reset cannot cross a live owner");return;
    }
    if(at) memset(at,0xa5,(size_t)(end-at));
    c.inner->reset(c.inner,m);
}
static samir_pal_t *setup(void){
    struct pal_host_cfg cfg={85,10,30,4*1024*1024};
    memset(&c,0,sizeof c);c.inner=pal_host_make(cfg);c.error=PAL_ENOSPC;
    c.p.open=op;c.p.close=cl;c.p.read=rd;c.p.write=wr;c.p.seek=sk;
    c.p.remove=rmf;c.p.rename=rn;c.p.conout=co;c.p.conin_line=ci;
    c.p.today=td;c.p.alloc=al;c.p.reset=rs;
    return &c.p;
}
static void put16(unsigned char *b,unsigned n){b[0]=n;b[1]=n>>8;}
static void put32(unsigned char *b,unsigned n){put16(b,n);put16(b+2,n>>16);}
static unsigned char original[66000]; static size_t olen; static int hlen;
static void fixture(const char *name,int extra,unsigned count){
    FILE *f;unsigned i,j;hlen=64+extra;olen=(size_t)hlen+count*255+1;
    memset(original,0,olen);original[0]=3;original[1]=85;original[2]=10;original[3]=30;
    put32(original+4,count);put16(original+8,hlen);put16(original+10,255);
    memcpy(original+32,"SENTINEL",8);original[43]='C';original[48]=254;original[64]=13;
    for(i=0;i<count;i++){original[hlen+i*255]=' ';for(j=1;j<255;j++)original[hlen+i*255+j]=(unsigned char)('A'+i%26);}
    original[olen-1]=26;f=fopen(name,"wb");CHECK(f!=NULL,"fixture open");if(f){CHECK(fwrite(original,1,olen,f)==olen,"fixture written");fclose(f);}
}
static unsigned char extracted[67000];
static size_t readbytes(const char *name){FILE *f=fopen(name,"rb");size_t n=0;CHECK(f!=NULL,"result readable");if(f){n=fread(extracted,1,sizeof extracted,f);fclose(f);}return n;}
static void independent(unsigned expected){
    char cmd[512],buf[1024],needle[80];FILE *f;size_t n;
    snprintf(cmd,sizeof cmd,"python3 harness/diff/dbf_diff/dbf_ref.py --schema %s > build/dbsafe/ref.json 2> build/dbsafe/ref.err",path);
    CHECK(system(cmd)==0,"L001 independent reader reopens DBF");
    f=fopen("build/dbsafe/ref.json","rb");n=f?fread(buf,1,sizeof buf-1,f):0;if(f)fclose(f);buf[n]=0;
    snprintf(needle,sizeof needle,"record_count: %u",expected);CHECK(strstr(buf,needle)!=NULL,"L001 independent committed count");
}
static int append_case(int extra,int seek,int boundary,int kind,int error){
    samir_pal_t *p=setup();int hit;size_t n;
    fixture(path,extra,255);c.seek_fault=seek;c.boundary=boundary;c.short_kind=kind;c.error=error;
    c.script[0]="use build/dbsafe/FAULT.DBF";c.script[1]="append blank";c.script[2]="quit";
    c.ip=xb_interp_make(p);CHECK(c.ip!=NULL,"interpreter allocated");samir_repl(p,c.ip);hit=c.hit;
    xb_interp_free(c.ip);pal_host_free(c.inner);
    n=readbytes(path);
    if(hit){
        CHECK(n>=olen-1,"L001 old records remain present");
        CHECK(!memcmp(extracted,original,(size_t)hlen),"L001 original header geometry and committed count");
        CHECK(!memcmp(extracted+hlen,original+hlen,255*255),"L001 prior record bytes untouched");
        if(!seek) CHECK(strstr(c.out,error==PAL_EACCES?"File is not accessible.":error==PAL_EIO?"Database write failed":"Disk full when writing file:")!=NULL,"B02 typed write diagnostic");
        independent(255);
    }else{
        CHECK(extracted[4]==0 && extracted[5]==1,"L001 successful append commits 256");independent(256);
    }
    return hit;
}
static void append_tests(void){
    int extra,seek,b,k,err;
    for(extra=1;extra<=2;extra++)for(seek=0;seek<2;seek++){
        for(b=1;b<=16;b++){
            if(!append_case(extra,seek,b,0,seek?PAL_EIO:PAL_ENOSPC))break;
            if(!seek)for(k=1;k<=4;k++)append_case(extra,0,b,k,PAL_ENOSPC);
        }
        CHECK(b<=16,"L001 enumerated every append boundary");
    }
    for(err=PAL_EACCES;err<=PAL_EIO;err+=PAL_EIO-PAL_EACCES)append_case(2,0,1,0,err);
}
static void lifecycle_tests(void){
    samir_pal_t *p=setup();wa_env *env;int first,second,i,rc;
    fixture("build/dbsafe/A.DBF",1,3);fixture("build/dbsafe/B.DBF",2,3);
    c.ip=xb_interp_make(p);env=xb_interp_env(c.ip);
    for(first=1;first<=2;first++){
        second=3-first;
        CHECK(wa_set_open_rw(env,first,"build/dbsafe/A.DBF",NULL,NULL)==0,"L003 first open");
        CHECK(wa_set_open_rw(env,second,"build/dbsafe/B.DBF",NULL,NULL)==0,"L003 second distinct open");
        c.live_mark=c.inner->alloc(c.inner,0);
        wa_close(env,first);
        CHECK(dbf_nrec(wa_table(env,second))==3,"L003 newer area survives older CLOSE");
        c.live_mark=NULL;wa_close(env,second);
        for(i=0;i<60;i++){
            CHECK(wa_set_open_rw(env,first,"build/dbsafe/A.DBF",NULL,NULL)==0,"L003 reclaimed open");
            rc=wa_set_open_rw(env,second,"build/dbsafe/a.dbf",NULL,NULL);
            CHECK(rc!=0,"L003 duplicate refused before allocation");
            CHECK(wa_table(env,second)==NULL,"L003 duplicate leaves area closed");wa_close_all(env);
        }
    }
    CHECK(c.reset_bad==0,"L003 no invalid arena rewind");
    xb_interp_free(c.ip);pal_host_free(c.inner);
}
static void safety_case(const char *answer,int off,unsigned expected){
    samir_pal_t *p=setup();int i=0;size_t n;
    fixture(path,2,3);c.script[i++]="use build/dbsafe/FAULT.DBF";
    if(off)c.script[i++]="set safety off";
    c.script[i++]="zap";if(!off && answer)c.script[i++]=answer;
    if(answer || off){c.script[i++]="? reccount()";c.script[i++]="quit";}
    c.ip=xb_interp_make(p);samir_repl(p,c.ip);xb_interp_free(c.ip);pal_host_free(c.inner);
    CHECK((strstr(c.out,"ZAP build/dbsafe/FAULT.DBF? (Y/N)")!=NULL)==!off,"L007 manual ZAP prompt including filename");
    n=readbytes(path);CHECK(n>=65 && extracted[4]==expected,"L007 only affirmative destroys records");
    if(expected)CHECK(n==olen && !memcmp(extracted,original,olen),"L007 declined ZAP byte-identical");
    if(answer || off)CHECK(strstr(c.out,expected?"3\n":"0\n")!=NULL,"L007 following command remains framed");
}
static void safety_tests(void){safety_case("N",0,3);safety_case("\033",0,3);safety_case("",0,3);safety_case(NULL,0,3);safety_case("Y",0,0);safety_case("y",0,0);safety_case(NULL,1,0);}
int main(int argc,char **argv){
    const char *mode=argc>1?argv[1]:"append";
    if(!strcmp(mode,"append"))append_tests();else if(!strcmp(mode,"lifetime"))lifecycle_tests();else safety_tests();
    return TEST_SUMMARY(mode);
}
