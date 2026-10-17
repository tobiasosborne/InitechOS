/* Core hardening oracle: initech-{1pd7,f6q7,frei,ifpd,jy6y,ptqc,xiqz,
 * 52ph,u5mw,8w5e,u468,nrkx,68ox}.
 * Law 1: Using dBASE III PLUS U1-1/2 (sizes/current memvar storage),
 * U6-33 (FILE), U7-5 (concat errors 77/76, NOT SPACE's 59).
 * Byte truth: ../dbase3-decomp/specs/file-formats/{dbf,dbt,ndx}.md,
 * ss8, ss2-5, ss1/5/6 respectively. Malformed-input rejection and arena
 * bounds are authored safety properties (CLAUDE Rules 2/3), not claims
 * about historical recovery from corrupt input. No shared offset macros.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "test_assert.h"
#include "samir/dbf.h"
#include "samir/dbt.h"
#include "samir/ndx.h"
#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/eval.h"
#include "samir/rt.h"
TEST_HARNESS();
struct pal_host_cfg { uint8_t yy,mm,dd; uint32_t heap_size; };
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void pal_host_free(samir_pal_t *p);
static const char *dbfpath="build/core.dbf", *dbtpath="build/core.dbt";
static const char *ndxpath="build/core.ndx";
static void put16(uint8_t *p,uint32_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static void put32(uint8_t *p,uint32_t n){put16(p,n);put16(p+2,n>>16);}
static void write_bytes(const char *path,const void *p,size_t n)
{
    FILE *f=fopen(path,"wb");CHECK(f!=NULL,"fixture open");
    if(f){CHECK(fwrite(p,1,n,f)==n,"fixture write");fclose(f);}
}
static void snapshot(const char *from,const char *to)
{
    uint8_t b[4096];FILE *f=fopen(from,"rb");size_t n=0;
    CHECK(f!=NULL,"memo snapshot opened");if(!f)return;
    n=fread(b,1,sizeof(b),f);fclose(f);write_bytes(to,b,n);
}
static void raw_dbf(uint32_t nrec,uint8_t width,char type)
{
    uint8_t b[400];memset(b,0,sizeof(b));
    b[0]=3;put32(b+4,nrec);put16(b+8,97);put16(b+10,width+3u);
    memcpy(b+32,"VALUE",5);b[43]=(uint8_t)type;b[48]=width;
    memcpy(b+64,"TAIL",4);b[75]='C';b[80]=2;b[96]=13;
    memset(b+97,' ',width+3u);memcpy(b+98+width,"OK",2);
    write_bytes(dbfpath,b,100u+width);
}
static void test_dbf_size(samir_pal_t *p)
{
    dbf_table *t=NULL;int rc;
    raw_dbf(0x80000001u,1,'C');
    rc=dbf_open(p,dbfpath,&t);
    CHECK(rc==-DBF_ERR_BAD_FILESZ,"DBF_SIZE: wrapped record span rejected on read");
    if(t) dbf_close(t);
    t=NULL;rc=dbf_open_rw(p,dbfpath,&t);
    CHECK(rc==-DBF_ERR_BAD_FILESZ,"DBF_SIZE: wrapped record span rejected before RW allocation");
    if(t) dbf_close(t);
}
static void test_date(samir_pal_t *p)
{
    uint8_t width;
    for(width=1;width<=10;width++) {
        dbf_table *t=NULL;xb_val v=xb_d(jdn_from_ymd(1986,3,14));
        uint8_t b[120];FILE *f;int rc;
        raw_dbf(1,width,'D');rc=dbf_open_rw(p,dbfpath,&t);
        CHECK(rc==0,"DATE_WIDTH: malformed-width fixture admitted safely");
        if(!t) continue;
        CHECK(dbf_replace(t,1,0,&v)==0,"DATE_WIDTH: bounded date replacement");
        CHECK(dbf_flush(t)==0,"DATE_WIDTH: flush");dbf_close(t);
        f=fopen(dbfpath,"rb");CHECK(f!=NULL,"DATE_WIDTH: independent bytes");
        if(!f) continue;
        memset(b,0,sizeof(b));(void)fread(b,1,sizeof(b),f);fclose(f);
        CHECK(!memcmp(b+98+width,"OK",2),"DATE_WIDTH: next field preserved");
        CHECK(!memcmp(b+98,"19860314",width<8?width:8),"DATE_WIDTH: date prefix");
        if(width>8) CHECK(b[106]==' ',"DATE_WIDTH: long slot padded");
    }
}
static void test_memo(samir_pal_t *p)
{
    dbf_field_spec schema[]={{"NOTE",'M',10,0},{"TAIL",'C',2,0}};
    dbf_table *t=NULL;dbt_file *m=NULL,*reader=NULL;xb_val v=xb_c("hello",5);
    uint8_t b[128],*text=NULL;uint32_t len=0;FILE *f;
    CHECK(dbt_create(p,dbtpath,&m)==0,"MEMO_POINTER: create companion");
    if(m) dbt_close(m);
    CHECK(dbt_open(p,dbtpath,0,&reader)==0,"MEMO_POINTER: concurrent read handle");
    CHECK(dbf_create(p,dbfpath,schema,2,&t)==0,"MEMO_POINTER: create table");
    if(!t) return;
    CHECK(dbf_append_blank(t)==0,"MEMO_POINTER: append record");
    CHECK(dbf_replace(t,1,0,&v)==0,"MEMO_POINTER: text replacement");
    if(reader){CHECK(dbt_read(reader,1,&text,&len)==0 && len==5 && !memcmp(text,"hello",5),
                     "MEMO_POINTER: existing reader sees appended live span");dbt_close(reader);}
    CHECK(dbf_flush(t)==0,"MEMO_POINTER: flush");dbf_close(t);
    f=fopen(dbfpath,"rb");CHECK(f!=NULL,"MEMO_POINTER: independent DBF bytes");
    if(f){memset(b,0,sizeof(b));(void)fread(b,1,sizeof(b),f);fclose(f);
        CHECK(!memcmp(b+98,"         1",10),"MEMO_POINTER: right-justified decimal pointer, not text");}
    m=NULL;CHECK(dbt_open(p,dbtpath,0,&m)==0,"MEMO_POINTER: reopen companion");
    if(m){CHECK(dbt_read(m,1,&text,&len)==0 && len==5 && !memcmp(text,"hello",5),
                "MEMO_POINTER: appended text survives independent reopen");dbt_close(m);}
    snapshot(dbfpath,"build/core-memo.dbf");snapshot(dbtpath,"build/core-memo.dbt");
}
static void test_dbt_span(samir_pal_t *p)
{
    uint8_t b[1536],*out=NULL;uint32_t len=0;dbt_file *m=NULL;
    memset(b,0,sizeof(b));put32(b,2);memset(b+512,'X',513);b[1025]=26;b[1026]=26;
    write_bytes(dbtpath,b,sizeof(b));
    CHECK(dbt_open(p,dbtpath,0,&m)==0,"DBT_SPAN: corrupt trailing data fixture opens");
    if(m){CHECK(dbt_read(m,1,&out,&len)==-DBT_ERR_NO_TERM && !out && !len,
               "DBT_SPAN: terminator outside live span rejected");dbt_close(m);}
    put32(b,0x80000002u);b[512]=26;b[513]=26;write_bytes(dbtpath,b,sizeof(b));
    m=NULL;CHECK(dbt_open(p,dbtpath,0,&m)==0,"DBT_SPAN: overflowing block fixture opens");
    if(m){CHECK(dbt_read(m,0x80000001u,&out,&len)==-DBT_ERR_BAD_BLOCK,
               "DBT_SPAN: wrapped block seek rejected");dbt_close(m);}
    put32(b,0x80000001u);write_bytes(dbtpath,b,sizeof(b));
    m=NULL;CHECK(dbt_open_rw(p,dbtpath,&m)==0,"DBT_SPAN: corrupt writable header fixture");
    if(m){uint32_t block=0;
        CHECK(dbt_append(m,(const uint8_t *)"hello",5,&block)==-DBT_ERR_BAD_BLOCK && block==0,
              "DBT_SPAN: writable overflowing block rejected before append");dbt_close(m);}
}
static unsigned allocations,providers;
static void *deny_alloc(samir_pal_t *p,uint32_t n)
{(void)p;(void)n;allocations++;return NULL;}
static int same_key(void *u,uint32_t rec,uint8_t *out,uint16_t len)
{(void)u;(void)rec;providers++;memset(out,'K',len);return 0;}
static void test_ndx_bounds(samir_pal_t *p)
{
    uint8_t b[513]={0};size_t n;ndx_index *idx=NULL;
    samir_pal_t fake=*p;
    for(n=0;n<=513;n+= n==0?1:n==1?510:2){
        write_bytes(ndxpath,b,n);
        CHECK(ndx_open(p,ndxpath,&idx)==-NDX_ERR_BAD_SIZE,"NDX_SIZE: short or partial page is structural error");
        if(idx){ndx_close(idx);idx=NULL;}
    }
    fake.alloc=deny_alloc;allocations=providers=0;
    CHECK(ndx_build(&fake,ndxpath,0,100,"K",50000001u,same_key,NULL)==-NDX_ERR_NOROOM,
          "NDX_BUILD_SIZE: key span: reject overflowing key buffer before allocation");
    CHECK(allocations==0 && providers==0,"NDX_BUILD_SIZE: key span: no allocator or provider side effects");
    allocations=providers=0;
    CHECK(ndx_build(&fake,ndxpath,0,1,"K",0x40000000u,same_key,NULL)==-NDX_ERR_NOROOM,
          "NDX_BUILD_SIZE: recno span: reject overflowing record-number buffer");
    allocations=providers=0;
    CHECK(ndx_build(&fake,ndxpath,0,1,"K",0xfffffff0u,same_key,NULL)==-NDX_ERR_NOROOM,
          "NDX_BUILD_SIZE: leaf ceiling: reject overflowing leaf ceiling");
}
struct order {uint32_t prev,count;int bad;uint8_t key[8];};
static int ordered(void *u,const uint8_t *key,uint32_t rec)
{
    struct order *o=u;int cmp=memcmp(o->key,key,8);
    if(o->count && (cmp>0 || (!cmp && rec<=o->prev)))o->bad=1;
    memcpy(o->key,key,8);o->prev=rec;o->count++;return 0;
}
static void check_order(ndx_index *idx,uint32_t count)
{
    struct order o={0,0,0,{0}};int rc=ndx_inorder(idx,ordered,&o);
    CHECK(rc==0 && !o.bad && o.count==count,"NDX_DUP_DESCENT: same-key traversal strictly ordered by recno");
}
static int mixed_key(void *u,uint32_t rec,uint8_t *out,uint16_t len)
{(void)u;memset(out,rec>=17?'J':'K',len);return 0;}
static void test_stale_separator(samir_pal_t *p)
{
    uint8_t j[8],k[8];ndx_index *idx=NULL;
    memset(j,'J',8);memset(k,'K',8);
    CHECK(ndx_build(p,ndxpath,0,8,"KEY",31,mixed_key,NULL)==0,"NDX_DUP_DESCENT: mixed-key full leaf");
    CHECK(ndx_open_rw(p,ndxpath,&idx)==0,"NDX_DUP_DESCENT: mixed-key reopen");
    if(!idx)return;
    CHECK(ndx_insert_key(idx,k,32)==0,"NDX_DUP_DESCENT: mixed-key split");
    CHECK(ndx_delete_key(idx,k,1)==0,"NDX_DUP_DESCENT: remove left leaf's only K");
    CHECK(ndx_update_key(idx,j,k,20)==0,"NDX_DUP_DESCENT: replacement with stale equal separator");
    check_order(idx,31);ndx_close(idx);
}
static void test_duplicates(samir_pal_t *p)
{
    uint8_t key[8];ndx_index *idx=NULL;uint32_t n;
    memset(key,'K',8);
    CHECK(ndx_build(p,ndxpath,0,8,"K",31,same_key,NULL)==0,"NDX_DUP_DESCENT: full same-key leaf");
    CHECK(ndx_open_rw(p,ndxpath,&idx)==0,"NDX_DUP_DESCENT: writable reopen");
    if(!idx)return;
    for(n=32;n<=95;n++) {
        CHECK(ndx_insert_key(idx,key,n)==0,"NDX_DUP_DESCENT: insert across repeated same-key splits");
        check_order(idx,n);
    }
    CHECK(ndx_delete_key(idx,key,17)==0,"NDX_DUP_DESCENT: remove interior pair");
    CHECK(ndx_insert_key(idx,key,17)==0,"NDX_DUP_DESCENT: reinsert smaller recno");
    check_order(idx,95);ndx_close(idx);
}
static void test_eval(samir_pal_t *p)
{
    xb_interp *ip=xb_interp_make(p);xb_val out;int ec,rc;xb_ctx ctx;
    xb_val arg=xb_c("build/core.dbf",14);
    CHECK(ip!=NULL,"interpreter allocated");if(!ip)return;
    rc=xb_interp_eval_str(ip,"SPACE(127)+SPACE(127)",21,&out,&ec);
    CHECK(rc==0 && out.t==XB_C && out.u.c.len==254,"CONCAT_PLUS: 254 accepted");
    rc=xb_interp_eval_str(ip,"SPACE(130)+SPACE(130)",21,&out,&ec);
    CHECK(rc!=0 && ec==77,"CONCAT_PLUS: 260 rejected with manual error 77");
    rc=xb_interp_eval_str(ip,"SPACE(130)-SPACE(130)",21,&out,&ec);
    CHECK(rc!=0 && ec==76,"CONCAT_MINUS: 260 rejected with manual error 76");
    memset(&ctx,0,sizeof(ctx));ctx.file_pal=p;ctx.scratch=(char[260]){0};ctx.scratch_cap=260;
    rc=xb_call_builtin("FILE",4,&arg,1,&ctx,&out,&ec);
    CHECK(rc==0 && out.t==XB_L && out.u.l,"FILE_NEEDS_USE: existing file works without cursor");
    arg=xb_c("build/absent.dbf",16);ctx.scratch_used=0;
    rc=xb_call_builtin("FILE",4,&arg,1,&ctx,&out,&ec);
    CHECK(rc==0 && out.t==XB_L && !out.u.l,"FILE_NEEDS_USE: absent file works without cursor");
    rc=samir_do(ip,"STORE 'keep' TO K\nSTORE 0 TO I\nDO WHILE I < 400\nSTORE SPACE(200) TO B\nSTORE B TO C\nRELEASE C\nSTORE 1 TO B\nI=I+1\nENDDO\n");
    CHECK(rc==0,"MEMVAR_RECLAIM: repeated update, alias copy, release and type change reclaim bytes");
    CHECK(xb_interp_get_memvar(ip,"K",&out)==0 && out.t==XB_C && out.u.c.len==4 && !memcmp(out.u.c.p,"keep",4),
          "MEMVAR_RECLAIM: other live value preserved");
    CHECK(samir_do(ip,"RELEASE ALL\nSTORE 'outer' TO S\n")==0,"MEMVAR_RECLAIM: outer value");
    CHECK(xb_interp_scope_enter(ip)==1 && xb_interp_declare_private(ip,"S")==0,
          "MEMVAR_RECLAIM: first private scope");
    CHECK(samir_do(ip,"STORE 'first' TO S\n")==0,"MEMVAR_RECLAIM: first private value");
    CHECK(xb_interp_scope_enter(ip)==2 && xb_interp_declare_private(ip,"S")==0,
          "MEMVAR_RECLAIM: second private scope");
    CHECK(samir_do(ip,"STORE 'second' TO S\nSTORE 0 TO I\nDO WHILE I<100\nSTORE SPACE(200) TO B\nI=I+1\nENDDO\n")==0,
          "MEMVAR_RECLAIM: compact with two hidden ancestors");
    xb_interp_scope_leave(ip);
    CHECK(xb_interp_get_memvar(ip,"S",&out)==0 && out.t==XB_C && out.u.c.len==5 && !memcmp(out.u.c.p,"first",5),
          "MEMVAR_RECLAIM: first hidden value restored");
    xb_interp_scope_leave(ip);
    CHECK(xb_interp_get_memvar(ip,"S",&out)==0 && out.t==XB_C && out.u.c.len==5 && !memcmp(out.u.c.p,"outer",5),
          "MEMVAR_RECLAIM: outer hidden value restored");
    xb_interp_free(ip);
}
int main(void)
{
    samir_pal_t *p=pal_host_make((struct pal_host_cfg){86,3,14,8u*1024u*1024u});
    CHECK(p!=NULL,"PAL allocated");if(!p)return 1;
    test_dbf_size(p);test_date(p);test_memo(p);test_dbt_span(p);
    test_ndx_bounds(p);test_stale_separator(p);test_duplicates(p);test_eval(p);
    pal_host_free(p);return TEST_SUMMARY("samir-core");
}
