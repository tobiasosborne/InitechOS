/* Audit L001/L003/L007: independent byte and lifetime oracles.
 * Ref: dbf.md ss2/4/6/8; Using dBase III Plus.pdf U7-7 (#3),
 * U5-284 (ZAP prompt), U5-257 (SAFETY default ON).
 * Failure atomicity and abort/EOF rules: authored, no local reference.
 * Fixtures are factory-authored DBFs; dbf_ref.py independently reopens them.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test_assert.h"
#include "ibase/interp.h"
#include "ibase/workarea.h"
#include "ibase/dbf.h"
#include "ibase/dbt.h"
#include "ibase/ndx.h"
#include "ibase/set.h"
TEST_HARNESS();
struct pal_host_cfg { uint8_t yy,mm,dd; uint32_t heap_size; };
extern ibase_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(ibase_pal_t *);
extern int ibase_repl(ibase_pal_t *,xb_interp *);
extern int proc_register(xb_interp *);
extern int mutate_register(xb_interp *);
extern int report_register(xb_interp *);
extern int query_register(xb_interp *);
static struct {
    ibase_pal_t p; ibase_pal_t *inner; xb_interp *ip;
    const char *script[32]; int line;
    char out[16384]; size_t used;
    int armed, boundary, hit, calls, seek_fault, short_kind, error, persistent, failure_op;
    void *live_mark; int reset_bad, closes, opens, objects;
    uintptr_t heap_base;
    uint32_t heap_high;
    int removes;
    char backup[260],stage[260];
} c;
/* Sibling ground-truth root (Makefile DBASE3_DECOMP). Relative paths are
 * anchored to project_root because main() chdirs into a private sandbox
 * before the tests run. Never an absolute host path (Law 3 / initech-3kiy). */
#ifndef DBASE3_DECOMP
#define DBASE3_DECOMP "../dbase3-decomp"
#endif
static char project_root[1024];
static const char *sibling(const char *rel){
    static char buf[2048];
    if(DBASE3_DECOMP[0]=='/') snprintf(buf,sizeof buf,"%s/%s",DBASE3_DECOMP,rel);
    else snprintf(buf,sizeof buf,"%s/%s/%s",project_root,DBASE3_DECOMP,rel);
    return buf;
}
static uint32_t test_heap_size=4u*1024u*1024u;
static const char *path="build/dbsafe/FAULT.DBF";
static int fault(int kind){
    if(!c.armed || c.failure_op!=kind)return 0;
    if(++c.calls==c.boundary || (c.persistent && c.hit)){c.hit=1;return 1;}return 0;
}
static pal_fd op(ibase_pal_t *p,const char *s,int m){pal_fd fd;(void)p;if(fault(4))return -PAL_EIO;fd=c.inner->open(c.inner,s,m);if(fd>=0)c.opens++;return fd;}
static int cl(ibase_pal_t *p,pal_fd f){int rc;(void)p;c.closes++;rc=c.inner->close(c.inner,f);return fault(2)?-PAL_EIO:rc;}
static int32_t rd(ibase_pal_t *p,pal_fd f,void *b,uint32_t n){(void)p;if(fault(5))return -PAL_EIO;return c.inner->read(c.inner,f,b,n);}
static int32_t wr(ibase_pal_t *p,pal_fd f,const void *b,uint32_t n){
    (void)p;
    if(n && c.armed && !c.failure_op && !c.seek_fault && (++c.calls==c.boundary || (c.persistent && c.hit))){
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
static int32_t sk(ibase_pal_t *p,pal_fd f,int32_t o,int w){
    (void)p;
    if(fault(1))return -PAL_EIO;
    if(c.armed && c.seek_fault && ++c.calls==c.boundary){c.hit=1;return -PAL_EIO;}
    return c.inner->seek(c.inner,f,o,w);
}
static int rmf(ibase_pal_t *p,const char *s){(void)p;if(c.armed)c.removes++;return c.inner->remove(c.inner,s);}
static int rn(ibase_pal_t *p,const char *a,const char *b){
    int rc;(void)p;
    if(fault(3)){if(!c.stage[0])snprintf(c.stage,sizeof c.stage,"%s",a);return -PAL_EIO;}
    rc=c.inner->rename(c.inner,a,b);
    if(!rc && strstr(b,".BAK"))snprintf(c.backup,sizeof c.backup,"%s",b);
    return rc;
}
static void co(ibase_pal_t *p,const char *s,uint32_t n){(void)p;if(c.used+n<sizeof c.out){memcpy(c.out+c.used,s,n);c.used+=n;c.out[c.used]=0;}}
static int32_t ci(ibase_pal_t *p,char *b,uint32_t n){
    const char *s=c.script[c.line++];(void)p;
    if(c.armed){
        if(c.hit){CHECK(dbf_nrec(wa_table(xb_interp_env(c.ip),1))==255,"L001 in-memory committed count");CHECK(wa_recno(xb_interp_env(c.ip),1)==1,"L001 refused append preserves cursor");}
        c.armed=0;
    }
    if(!s)return -1;
    if(!strcmp(s,"#INPUT_ERROR"))return -PAL_EIO;
    if(!strcmp(s,"append blank"))c.armed=1;
    snprintf(b,n,"%s",s);return (int32_t)strlen(b);
}
static void td(ibase_pal_t *p,uint8_t *a,uint8_t *b,uint8_t *d){(void)p;c.inner->today(c.inner,a,b,d);}
static void *al(ibase_pal_t *p,uint32_t n){(void)p;if(n && fault(6))return NULL;return c.inner->alloc(c.inner,n);}
static void rs(ibase_pal_t *p,void *m){
    uint8_t *end=c.inner->alloc(c.inner,0),*at=m;(void)p;
    if(at>end || (c.live_mark && at<(uint8_t *)c.live_mark)){
        c.reset_bad++;CHECK(0,"L003 reset cannot cross a live owner");return;
    }
    if(at) memset(at,0xa5,(size_t)(end-at));
    c.inner->reset(c.inner,m);
}
static void *ac(ibase_pal_t *p,uint32_t n){
    void *b;uint32_t end;(void)p;if(n && fault(6))return NULL;b=c.inner->acquire(c.inner,n);
    if(b){
        c.objects++;
        if(!c.heap_base)c.heap_base=(uintptr_t)b;
        end=(uint32_t)((uintptr_t)b-c.heap_base)+((n+7u)&~7u);
        if(end>c.heap_high)c.heap_high=end;
    }
    return b;
}
static void fr(ibase_pal_t *p,void *b){(void)p;if(b)c.objects--;c.inner->release(c.inner,b);}
static ibase_pal_t *setup(void){
    struct pal_host_cfg cfg={85,10,30,4*1024*1024};
    cfg.heap_size=test_heap_size;
    memset(&c,0,sizeof c);c.inner=pal_host_make(cfg);c.error=PAL_ENOSPC;
    c.p.open=op;c.p.close=cl;c.p.read=rd;c.p.write=wr;c.p.seek=sk;
    c.p.remove=rmf;c.p.rename=rn;c.p.conout=co;c.p.conin_line=ci;
    c.p.today=td;c.p.alloc=al;c.p.reset=rs;c.p.acquire=ac;c.p.release=fr;
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
    char cmd[2048],buf[1024],needle[80];FILE *f;size_t n;
    snprintf(cmd,sizeof cmd,"python3 '%s/tests/dbf_ref.py' --schema %s > build/dbsafe/ref.json 2> build/dbsafe/ref.err",project_root,path);
    CHECK(system(cmd)==0,"L001 independent reader reopens DBF");
    snprintf(cmd,sizeof cmd,"python3 '%s/tests/dbf_ref.py' --records %s > build/dbsafe/ref-records.txt 2> build/dbsafe/ref-records.err",project_root,path);
    CHECK(system(cmd)==0,"L001 independent reader reads every committed record");
    f=fopen("build/dbsafe/ref.json","rb");n=f?fread(buf,1,sizeof buf-1,f):0;if(f)fclose(f);buf[n]=0;
    snprintf(needle,sizeof needle,"record_count: %u",expected);CHECK(strstr(buf,needle)!=NULL,"L001 independent committed count");
}
static int append_case(int extra,int seek,int boundary,int kind,int error){
    ibase_pal_t *p=setup();int hit;size_t n;
    fixture(path,extra,255);c.seek_fault=seek;c.boundary=boundary;c.short_kind=kind;c.error=error;
    c.script[0]="use build/dbsafe/FAULT.DBF";c.script[1]="append blank";c.script[2]="quit";
    c.ip=xb_interp_make(p);CHECK(c.ip!=NULL,"interpreter allocated");ibase_repl(p,c.ip);hit=c.hit;
    xb_interp_free(c.ip);pal_host_free(c.inner);
    n=readbytes(path);
    if(hit){
        CHECK(n>=olen-1,"L001 old records remain present");
        CHECK(!memcmp(extracted,original,(size_t)hlen),"L001 original header geometry and committed count");
        CHECK(!memcmp(extracted+hlen,original+hlen,255*255),"L001 prior record bytes untouched");
        if(!seek) CHECK(strstr(c.out,error==PAL_EACCES?"File is not accessible.":error==PAL_EIO?"Database write failed":"Disk full when writing file:")!=NULL,"B02 typed write diagnostic");
        /* Every boundary compares all bytes above; decode one refused file
         * per geometry/fault class rather than spawning duplicate readers. */
        if(boundary==1 && kind==0)independent(255);
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
static void copy_local(const char *source,const char *dest){
    FILE *a=fopen(source,"rb"),*b=fopen(dest,"wb");char buf[8192];size_t n;
    CHECK(a && b,"byte-identical local corpus copy opened");
    if(a && b)while((n=fread(buf,1,sizeof buf,a))!=0)CHECK(fwrite(buf,1,n,b)==n,"local corpus copy written");
    if(a)fclose(a);
    if(b)fclose(b);
}
/* Preservation is the operator's failed-save ruling, not an inferred dBASE
 * transaction contract. Real III+ input is independently decoded on failure. */
static void flush_tests(void){
    int b,k,persistent,source;size_t n;dbf_table *t;xb_val value=xb_c("CHANGED",7);
    for(source=0;source<2;source++)for(persistent=0;persistent<2;persistent++)for(k=0;k<5;k++)for(b=1;b<64;b++){
        ibase_pal_t *p=setup();
        copy_local(sibling(source?"goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/TAX.DBF":"goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/CLIENTS.DBF"),path);
        olen=readbytes(path);memcpy(original,extracted,olen);
        t=NULL;CHECK(dbf_open_rw(p,path,&t)==0,"flush opens real III+ table");
        CHECK(t && dbf_replace(t,1,source?1:0,&value)==0,"flush edits character field");
        c.armed=1;c.boundary=b;c.short_kind=k;c.persistent=persistent;
        {int rc=dbf_flush(t);CHECK(!c.hit || rc!=0,"flush reports injected write failure");}
        c.armed=0;dbf_close(t);
        n=readbytes(path);
        if(c.hit){
            CHECK(n==olen && !memcmp(extracted,original,n),source?"failed +2 real TAX flush preserves every original byte":"failed flush preserves every original byte");
            if(source && b==5 && k==0 && !persistent){
                char export[1200];snprintf(export,sizeof export,"%s/build/FLUSHERR.DBF",project_root);copy_local(path,export);
            }
        }
        pal_host_free(c.inner);
        if(!c.hit)break;
    }
}
static void output_tests(void){
    static const char *commands[]={
        "COPY FILE SOURCE.TXT TO OUTPUT.TXT",
        "SAVE TO OUTPUT.TXT",
        "COPY FILE OUTPUT.TXT TO OUTPUT.TXT",
        "REPORT FORM TRAVELER NEXT 1 TO FILE OUTPUT.TXT",
        "LABEL FORM ADDRESS TO FILE OUTPUT.TXT",
        "SET ALTERNATE TO OUTPUT.TXT\nSET ALTERNATE ON\n? 'NEW LOG'\nSET ALTERNATE TO"};
    unsigned cmd;int b,k,persistent,kind;
    copy_local(sibling("goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/TRAVELER.FRM"),"TRAVELER.FRM");
    copy_local(sibling("goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/ADDRESS.LBL"),"ADDRESS.LBL");
    copy_local(sibling("goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/TRAVEL.DBF"),"TRAVEL.DBF");
    copy_local(sibling("goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/TRAVEL.DBT"),"TRAVEL.DBT");
    copy_local(sibling("goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/CLIENTS.DBF"),"CLIENTS.DBF");
    /* One real record exercises every label output path. The DBF-format
     * record count is independently authored at offset 4; stale tail bytes
     * are permitted by the III+ format and are never used as expectations. */
    {FILE *f=fopen("CLIENTS.DBF","r+b");const unsigned char one[4]={1,0,0,0};
     CHECK(f!=NULL,"single-record label fixture");if(f){fseek(f,4,SEEK_SET);fwrite(one,1,4,f);fclose(f);}}
    for(cmd=0;cmd<6;cmd++)for(kind=0;kind<=5;kind++)for(persistent=0;persistent<(kind?1:2);persistent++)for(k=0;k<(kind?1:5);k++)for(b=1;b<200;b++){
        /* Source-table seek/read errors are a separate work-area contract.
         * This matrix exhausts output writes/closes/opens/renames. */
        if(cmd>=3 && (kind==1 || kind==5))break;
        ibase_pal_t *p=setup();FILE *f;char before[64]="PREVIOUS-FILE\r\n";size_t n;
        f=fopen("SOURCE.TXT","wb");fputs("NEW-FILE\r\n",f);fclose(f);
        f=fopen("OUTPUT.TXT","wb");fputs(before,f);fclose(f);
        c.ip=xb_interp_make(p);set_register(c.ip);proc_register(c.ip);mutate_register(c.ip);query_register(c.ip);report_register(c.ip);
        CHECK(ibase_do(c.ip,"SET SAFETY OFF\nSTORE 'VALUE' TO SENTINEL")==0,"output setup");
        if(cmd==3 || cmd==4)CHECK(wa_set_open_rw(xb_interp_env(c.ip),1,cmd==3?"TRAVEL.DBF":"CLIENTS.DBF",NULL,NULL)==0,"report input opens");
        c.armed=1;c.boundary=b;c.short_kind=k;c.persistent=persistent;c.failure_op=kind;
        {int rc=ibase_do(c.ip,commands[cmd]);CHECK(!c.hit || rc!=0,"output reports write failure");
         if(c.hit && rc==0)fprintf(stderr,"ignored fault cmd=%u kind=%d b=%d\n",cmd,kind,b);
         CHECK(c.hit || rc==0,"output positive control executes");
         if(!c.hit && rc)fprintf(stderr,"command %s rc=%d error=%d\n",commands[cmd],rc,ibase_last_error(c.ip));}
        c.armed=0;
        n=readbytes("OUTPUT.TXT");
        if(c.hit)CHECK(n==strlen(before) && !memcmp(extracted,before,n),"failed output preserves old destination");
        if(cmd==2 && !c.hit)CHECK(n>=strlen(before) && !memcmp(extracted,before,strlen(before)),"self copy preserves source data");
        xb_interp_free(c.ip);pal_host_free(c.inner);
        if(!c.hit)break;
    }
}
static int index_key(void *user,uint32_t rec,uint8_t *key,uint16_t len){
    (void)user;memset(key,' ',len);key[0]=(uint8_t)('D'-rec);return 0;
}
static void index_tests(void){
    int kind,b,k;
    for(kind=0;kind<=4;kind++)for(k=0;k<(kind?1:5);k++)for(b=1;b<40;b++){
        ibase_pal_t *p=setup();size_t n;int rc;
        copy_local(sibling("goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/CNAMES.NDX"),"OUTPUT.NDX");
        olen=readbytes("OUTPUT.NDX");memcpy(original,extracted,olen);
        c.armed=1;c.failure_op=kind;c.boundary=b;c.short_kind=k;
        rc=ndx_build(p,"OUTPUT.NDX",0,8,"NAME",3,index_key,NULL);
        c.armed=0;CHECK(!c.hit || rc!=0,"index reports failed rebuild");
        n=readbytes("OUTPUT.NDX");
        if(c.hit)CHECK(n==olen && !memcmp(extracted,original,n),"failed rebuild preserves real III+ index");
        pal_host_free(c.inner);if(!c.hit)break;
    }
}
static void metadata_tests(void){
    int kind,b;size_t n;
    for(kind=1;kind<=4;kind++)for(b=1;b<40;b++){
        ibase_pal_t *p=setup();dbf_table *t=NULL;xb_val v=xb_c("CHANGED",7);
        fixture(path,2,3);
        CHECK(dbf_open_rw(p,path,&t)==0,"metadata open +2 fixture");
        CHECK(dbf_replace(t,1,0,&v)==0,"metadata edit");
        c.armed=1;c.failure_op=kind;c.boundary=b;
        {int rc=dbf_flush(t);CHECK(!c.hit || rc!=0,"metadata failure reported");}
        c.armed=0;dbf_close(t);n=readbytes(path);
        if(c.hit)CHECK(n==olen && !memcmp(extracted,original,n),"metadata failure restores old +2 file under its name");
        pal_host_free(c.inner);if(!c.hit)break;
    }
}
/* R1: the public rebuild API and INDEX command must retain the old bytes
 * through final reopen, header read, attach and navigation validation. */
static void final_index_tests(void){
    int command,kind,b;
    for(command=0;command<3;command++)for(kind=1;kind<=6;kind++)for(b=1;b<80;b++){
        ibase_pal_t *p=setup();ndx_index *ix=NULL;size_t n;int rc;
        copy_local(sibling("goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/CNAMES.NDX"),"FINAL.NDX");
        olen=readbytes("FINAL.NDX");memcpy(original,extracted,olen);
        if(command){
            c.ip=xb_interp_make(p);set_register(c.ip);mutate_register(c.ip);
            CHECK(wa_set_open_rw(xb_interp_env(c.ip),1,"CLIENTS.DBF",NULL,NULL)==0,"INDEX input opens");
            CHECK(ibase_do(c.ip,"SET SAFETY OFF")==0,"INDEX replacement permitted");
            if(command==2){
                CHECK(ndx_open_rw(p,"FINAL.NDX",&ix)==0,"REINDEX command predecessor opens");
                CHECK(wa_attach_index(xb_interp_env(c.ip),1,ix)==0,"REINDEX predecessor attached");
            }
        }else CHECK(ndx_open_rw(p,"FINAL.NDX",&ix)==0,"real predecessor opens for REINDEX");
        c.armed=1;c.failure_op=kind;c.boundary=b;
        rc=command?ibase_do(c.ip,command==1?"INDEX ON FIRSTNAME TO FINAL.NDX":"REINDEX"):ndx_rebuild(ix,3,index_key,NULL);
        c.armed=0;n=readbytes("FINAL.NDX");
        if(rc)CHECK(n==olen && !memcmp(original,extracted,n),command==1?"reported INDEX failure preserves predecessor":"reported REINDEX failure preserves predecessor");
        else {
            ndx_node_t *node=NULL;
            ndx_index *live=command?wa_index(xb_interp_env(c.ip),1,0):ix;
            CHECK(live && ndx_read_node(live,ndx_root_page(live),&node)==0,"successful index operation retains usable live handle");
            if(node)ndx_node_free(live,node);
        }
        if(command)xb_interp_free(c.ip);else ndx_close(ix);
        pal_host_free(c.inner);if(!c.hit)break;
    }
    { /* Attachment refusal must precede discarding an existing destination. */
        ibase_pal_t *p=setup();size_t n;ndx_index *ix;
        c.ip=xb_interp_make(p);set_register(c.ip);mutate_register(c.ip);
        CHECK(wa_set_open_rw(xb_interp_env(c.ip),1,"CLIENTS.DBF",NULL,NULL)==0,"attachment test table");
        for(int i=0;i<NDX_PER_AREA;i++){
            CHECK(ndx_open_rw(p,"FINAL.NDX",&ix)==0,"attachment existing index");
            CHECK(wa_attach_index(xb_interp_env(c.ip),1,ix)==0,"fill attachment slots");
        }
        copy_local(sibling("goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/CNAMES.NDX"),"ATTACH.NDX");
        olen=readbytes("ATTACH.NDX");memcpy(original,extracted,olen);
        CHECK(ibase_do(c.ip,"SET SAFETY OFF\nINDEX ON FIRSTNAME TO ATTACH.NDX")!=0,"full attachment reports refusal");
        n=readbytes("ATTACH.NDX");CHECK(n==olen && !memcmp(original,extracted,n),"attachment failure preserves predecessor");
        xb_interp_free(c.ip);pal_host_free(c.inner);
    }
}
static void persistent_rename_tests(void){
    const char *old="PREVIOUS-CONTENT\r\n";FILE *f;size_t n;ibase_pal_t *p=setup();
    f=fopen("SOURCE.TXT","wb");fputs("NEW-CONTENT\r\n",f);fclose(f);
    f=fopen("RECOVERY.TXT","wb");fputs(old,f);fclose(f);
    c.ip=xb_interp_make(p);set_register(c.ip);mutate_register(c.ip);
    CHECK(ibase_do(c.ip,"SET SAFETY OFF")==0,"recovery setup");
    c.armed=1;c.failure_op=3;c.boundary=2;c.persistent=1;
    CHECK(ibase_do(c.ip,"COPY FILE SOURCE.TXT TO RECOVERY.TXT")!=0,"persistent installation/restore failure reported");
    c.armed=0;
    CHECK(c.hit && c.calls>=3,"installation and restoration both fail");
    CHECK(c.backup[0]!=0,"predecessor recovery name recorded independently");
    n=readbytes(c.backup);CHECK(n==strlen(old) && !memcmp(extracted,old,n),"previous file survives at exact recovery name");
    {char diagnostic[320];snprintf(diagnostic,sizeof diagnostic,"Previous file retained as: %s\r\n",c.backup);
     CHECK(strstr(c.out,diagnostic)!=NULL,"reported error names exact predecessor location");}
    CHECK(c.removes==0,"double failure deletes nothing");
    CHECK(access(c.stage,F_OK)==0,"complete new stage retained too");
    xb_interp_free(c.ip);pal_host_free(c.inner);
}
/* initech-hls50: independent originals are the captured C21 old index and
 * real III+ B004 (a full leaf). Preservation is the operator's ruling;
 * successful insert/split geometry is graded by test_ndx_incremental. */
static void incremental_failure_tests(void){
    int operation,kind,k,b,persistent;
    for(operation=0;operation<4;operation++)for(kind=0;kind<=6;kind++)
    for(persistent=0;persistent<(kind?1:2);persistent++)for(k=0;k<(kind?1:5);k++)for(b=1;b<80;b++){
        ibase_pal_t *p=setup();ndx_index *ix=NULL;size_t n;int rc;uint8_t oldkey[100],newkey[100];char src[1400];
        if(operation==3)snprintf(src,sizeof src,"%s",sibling("mint/work/B004.NDX"));
        else snprintf(src,sizeof src,"%s/tests/conformance/cases/C21/OLD.NDX",project_root);
        copy_local(src,"INCR.NDX");olen=readbytes("INCR.NDX");memcpy(original,extracted,olen);
        CHECK(ndx_open_rw(p,"INCR.NDX",&ix)==0,"independent incremental predecessor opens");
        memset(oldkey,'C',8);memset(newkey,operation==2?'D':'Z',8);
        if(operation==3){memset(newkey,'0',100);newkey[98]='5';newkey[99]='0';}
        c.armed=1;c.failure_op=kind;c.boundary=b;c.short_kind=k;c.persistent=persistent;c.error=PAL_EIO;
        rc=operation==0?ndx_update_key(ix,oldkey,newkey,3):operation==1?ndx_delete_key(ix,oldkey,3):ndx_insert_key(ix,newkey,operation==3?5:4);
        c.armed=0;ndx_close(ix);n=readbytes("INCR.NDX");
        if(rc)CHECK(n==olen && !memcmp(original,extracted,n),"failed incremental operation preserves every independent NDX byte");
        CHECK(c.hit || rc==0,"incremental positive control succeeds");
        pal_host_free(c.inner);if(!c.hit)break;
        CHECK(b<79,"all incremental failure boundaries enumerated");
    }
}
/* One failed REPLACE must not commit a DBF with either preceding index.
 * Earlier successful records of a multi-record command are outside this
 * invariant. All three originals are captured III+ files. */
static void indexed_replace_failure_tests(void){
    static unsigned char before_dbf[66000],before_ndx[66000];
    int kind,k,b,persistent;size_t dbflen,ndxlen;
    for(kind=0;kind<=6;kind++)for(persistent=0;persistent<(kind?1:2);persistent++)
    for(k=0;k<(kind?1:5);k++)for(b=1;b<160;b++){
        ibase_pal_t *p=setup();ndx_index *ix;size_t n;int rc;
        copy_local(sibling("goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/CLIENTS.DBF"),"ROW.DBF");
        dbflen=readbytes("ROW.DBF");memcpy(before_dbf,extracted,dbflen);
        copy_local(sibling("goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/CNAMES.NDX"),"ROW1.NDX");
        ndxlen=readbytes("ROW1.NDX");memcpy(before_ndx,extracted,ndxlen);
        copy_local("ROW1.NDX","ROW2.NDX");
        c.ip=xb_interp_make(p);set_register(c.ip);mutate_register(c.ip);
        CHECK(wa_set_open_rw(xb_interp_env(c.ip),1,"ROW.DBF",NULL,NULL)==0,"indexed REPLACE table opens");
        for(int i=0;i<2;i++){
            CHECK(ndx_open_rw(p,i?"ROW2.NDX":"ROW1.NDX",&ix)==0,"indexed REPLACE predecessor opens");
            CHECK(wa_attach_index(xb_interp_env(c.ip),1,ix)==0,"indexed REPLACE predecessor attached");
        }
        CHECK(wa_goto(xb_interp_env(c.ip),1,1)==0,"indexed REPLACE physical record selected");
        c.armed=1;c.failure_op=kind;c.boundary=b;c.short_kind=k;c.persistent=persistent;c.error=PAL_EIO;
        rc=ibase_do(c.ip,"REPLACE FIRSTNAME WITH 'ZZZZZZZZZZZZZZZZZZZZ'");
        c.armed=0;
        if(rc){
            n=readbytes("ROW.DBF");CHECK(n==dbflen && !memcmp(before_dbf,extracted,n),"failed indexed REPLACE preserves preceding DBF");
            for(int i=0;i<2;i++){
                n=readbytes(i?"ROW2.NDX":"ROW1.NDX");
                CHECK(n==ndxlen && !memcmp(before_ndx,extracted,n),"failed indexed REPLACE preserves each preceding NDX");
            }
            /* A retry after a transient failure must not flush stale memory. */
            CHECK(ibase_do(c.ip,"REPLACE LASTNAME WITH LASTNAME")==0,"same work area accepts retry after device recovery");
            n=readbytes("ROW.DBF");
            {unsigned oldoff=before_dbf[8]+256u*before_dbf[9],newoff=extracted[8]+256u*extracted[9];
             unsigned rows=before_dbf[4]+256u*before_dbf[5],width=before_dbf[10]+256u*before_dbf[11];
             CHECK(n>=newoff+rows*width && !memcmp(before_dbf+oldoff,extracted+newoff,rows*width),"retry retains original record bytes");}
        }
        CHECK(c.hit || rc==0,"indexed REPLACE positive control succeeds");
        xb_interp_free(c.ip);pal_host_free(c.inner);if(!c.hit)break;
        CHECK(b<159,"all indexed REPLACE boundaries enumerated");
    }
}
static void output_alias_tests(void){
    static const char *commands[]={"COPY FILE SOURCE.TXT TO ./CLIENTS.DBF", "SAVE TO .\\CLIENTS.DBF",
        "LABEL FORM ADDRESS TO FILE ./CLIENTS.DBF", "SET ALTERNATE TO ./CLIENTS.DBF"};
    ibase_pal_t *p=setup();size_t n;
    c.ip=xb_interp_make(p);set_register(c.ip);mutate_register(c.ip);report_register(c.ip);
    CHECK(wa_set_open_rw(xb_interp_env(c.ip),1,"CLIENTS.DBF",NULL,NULL)==0,"active-output table opens");
    olen=readbytes("CLIENTS.DBF");memcpy(original,extracted,olen);
    for(unsigned i=0;i<sizeof commands/sizeof commands[0];i++){
        CHECK(ibase_do(c.ip,commands[i])!=0 && ibase_last_error(c.ip)==3,"active-output alias refused with error 3");
        n=readbytes("CLIENTS.DBF");CHECK(n==olen && !memcmp(original,extracted,n),"active table survives refused output alias");
    }
    xb_interp_free(c.ip);pal_host_free(c.inner);
}
static void codec_lifetimes(ibase_pal_t *p){
    dbt_file *ma=NULL,*mb=NULL;ndx_index *ia=NULL,*ib=NULL;ndx_node_t *node=NULL;
    unsigned char block[512]={1};FILE *f=fopen("build/dbsafe/A.DBT","wb");
    CHECK(f!=NULL,"empty memo fixture");if(f){fwrite(block,1,sizeof block,f);fclose(f);}
    copy_local(sibling("goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/CNAMES.NDX"),"build/dbsafe/CNAMES.NDX");
    CHECK(dbt_open(p,"build/dbsafe/A.DBT",0,&ma)==0,"L003 first memo open");
    CHECK(ndx_open_rw(p,"build/dbsafe/CNAMES.NDX",&ia)==0,"L003 first index open (local corpus copy)");
    CHECK(dbt_open(p,"build/dbsafe/A.DBT",0,&mb)==0,"L003 second memo open");
    CHECK(ndx_open(p,"build/dbsafe/CNAMES.NDX",&ib)==0,"L003 second index open");
    dbt_close(ma);ndx_close(ia);
    CHECK(dbt_next_free(mb)==1,"L003 memo survives unrelated close");
    CHECK(ndx_read_node(ib,ndx_root_page(ib),&node)==0,"L003 surviving index node readable");
    ndx_node_free(ib,node);dbt_close(mb);ndx_close(ib);
    CHECK(c.objects==0,"L003 codec buffers reclaimed independently");
}
/* initech-104j3 review fix: persistent storage must outlive scratch resets
 * only while owned. One PAL, 32 interpreter lifetimes, no CLOSE beforehand.
 * Measure the highest acquired address + rounded size from the first block,
 * including intervening heap headers; no production allocator test hooks. */
static void procedure_lifetimes(void){
    ibase_pal_t *p=setup();FILE *f;uint32_t first=0;int i;
    f=fopen("build/dbsafe/LIBRARY.PRG","wb");
    CHECK(f!=NULL,"procedure lifetime fixture open");
    if(!f){pal_host_free(c.inner);return;}
    fputs("PROCEDURE SAME\nSTORE 1 TO FLAG\nRETURN\n",f);fclose(f);
    for(i=0;i<32;i++){
        c.ip=xb_interp_make(p);
        CHECK(c.ip!=NULL,"procedure lifetime interpreter create");
        if(!c.ip)break;
        CHECK(proc_register(c.ip)==0,"procedure lifetime register");
        CHECK(ibase_do(c.ip,"SET PROCEDURE TO build/dbsafe/LIBRARY") == 0,
              "procedure lifetime library open");
        xb_interp_free(c.ip);
        CHECK(c.objects==0,"procedure teardown releases persistent library");
        if(!i)first=c.heap_high;
        CHECK(c.heap_high==first,"procedure persistent heap high-water stable");
    }
    printf("PROCEDURE HEAP cycles=%d first=%u final=%u retained=%d bytes-from-first-block\n",
           i,first,c.heap_high,c.objects);
    pal_host_free(c.inner);
}
static void lifecycle_tests(void){
    ibase_pal_t *p;wa_env *env;int first,second,i,rc;
    test_heap_size=32768u;p=setup();
    fixture("build/dbsafe/A.DBF",1,3);fixture("build/dbsafe/B.DBF",2,3);
    codec_lifetimes(p);
    c.ip=xb_interp_make(p);env=xb_interp_env(c.ip);
    for(first=1;first<=2;first++){
        second=3-first;
        rc=wa_set_open_rw(env,first,"build/dbsafe/A.DBF",NULL,NULL);
        CHECK(rc==0,"L003 first open");if(rc)break;
        /* A later scratch owner also must survive persistent codec CLOSE. */
        p->alloc(p,32u);
        CHECK(wa_set_open_rw(env,second,"build/dbsafe/B.DBF",NULL,NULL)==0,"L003 second distinct open");
        c.live_mark=c.inner->alloc(c.inner,0);
        wa_close(env,first);
        CHECK(dbf_nrec(wa_table(env,second))==3,"L003 newer area survives older CLOSE");
        c.live_mark=NULL;wa_close(env,second);
        for(i=0;i<60;i++){
            rc=wa_set_open_rw(env,first,"build/dbsafe/A.DBF",NULL,NULL);
            CHECK(rc==0,"L003 reclaimed open");if(rc)break;
            rc=wa_set_open_rw(env,second,"build/dbsafe/A.DBF",NULL,NULL);
            CHECK(rc==-WA_ERR_ALREADY_OPEN,"L003 exact duplicate refused");
            rc=wa_set_open_rw(env,second,"./build/dbsafe/a.dbf",NULL,NULL);
            CHECK(rc==-WA_ERR_ALREADY_OPEN,"L003 duplicate refused before allocation");
            CHECK(wa_table(env,second)==NULL,"L003 duplicate leaves area closed");
            rc=wa_set_open_rw(env,second,"build/dbsafe/MISSING.DBF",NULL,NULL);
            CHECK(rc==-DBF_ERR_NOENT,"L003 failed open leaves older area usable");
            CHECK(dbf_nrec(wa_table(env,first))==3,"L003 failed cleanup preserves older table");
            wa_close_all(env);
        }
    }
    CHECK(c.reset_bad==0,"L003 no invalid arena rewind");
    CHECK(c.objects==0,"L003 repeated work-area closes reclaim buffers");
    CHECK(c.opens==c.closes,"L003 all handles closed exactly once");
    xb_interp_free(c.ip);pal_host_free(c.inner);
    procedure_lifetimes();
}
static void safety_case(const char *answer,int off,unsigned expected){
    ibase_pal_t *p=setup();int i=0;size_t n;
    fixture(path,2,3);c.script[i++]="use build/dbsafe/FAULT.DBF";
    if(off==1)c.script[i++]="set safety off";
    if(off==2)c.script[i++]="set safety on";
    c.script[i++]="zap";if(off!=1 && answer)c.script[i++]=answer;
    if(answer || off==1){c.script[i++]="? reccount()";c.script[i++]="quit";}
    c.ip=xb_interp_make(p);ibase_repl(p,c.ip);xb_interp_free(c.ip);pal_host_free(c.inner);
    CHECK((strstr(c.out,"ZAP build/dbsafe/FAULT.DBF? (Y/N)")!=NULL)==(off!=1),"L007 manual ZAP prompt including filename");
    n=readbytes(path);CHECK(n>=65 && extracted[4]==expected,"L007 only affirmative destroys records");
    if(expected)CHECK(n==olen && !memcmp(extracted,original,olen),"L007 declined ZAP byte-identical");
    /* initech-ioke: ? RECCOUNT() is a 10-wide numeric in real III+ 1.1
     * (xbase_prog_diff/mint/golden/append/GOLD.OUT "         6"). */
    if(answer || off==1)CHECK(strstr(c.out,expected?"\n         3. ":"\n         0. ")!=NULL,"L007 following command remains framed");
}
static void safety_tests(void){safety_case("N",0,3);safety_case("N",2,3);safety_case("Y",2,0);safety_case("\033",0,3);safety_case("",0,3);safety_case(NULL,0,3);safety_case("#INPUT_ERROR",0,3);safety_case("Y",0,0);safety_case("y",0,0);safety_case(NULL,1,0);}
int main(int argc,char **argv){
    const char *mode=argc>1?argv[1]:"append";
    char directory[]="build/dbsafe/host-XXXXXX";
    /* Factory-only isolation: parallel oracles must never share mutable DBFs.
     * Temporary path does not enter DBF bytes or the shipped executable. */
    CHECK(getcwd(project_root,sizeof project_root)!=NULL,"factory project root");
    CHECK(mkdtemp(directory)!=NULL,"private oracle directory");
    if(chdir(directory) || mkdir("build",0700) || mkdir("build/dbsafe",0700)) return 2;
    if(!strcmp(mode,"append"))append_tests();else if(!strcmp(mode,"lifetime"))lifecycle_tests();else if(!strcmp(mode,"flush"))flush_tests();else if(!strcmp(mode,"ndxfail"))incremental_failure_tests();else if(!strcmp(mode,"rowfail"))indexed_replace_failure_tests();else if(!strcmp(mode,"writes")){flush_tests();output_tests();metadata_tests();index_tests();output_alias_tests();final_index_tests();persistent_rename_tests();}else safety_tests();
    return TEST_SUMMARY(mode);
}
