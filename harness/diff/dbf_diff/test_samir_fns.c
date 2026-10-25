/* PRD 6.6 / ADR-0008: same PRG and independent real III+ capture on host
 * and injected QEMU. fns_mint.sh creates evidence; this gate never mints it.
 * Transcript slices exclude only driver prompts and DOS EOF, not spacing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_assert.h"
#include "samir/interp.h"
#include "samir/rt.h"
TEST_HARNESS();
struct pal_host_cfg { uint8_t yy,mm,dd; uint32_t heap; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(samir_pal_t *);
static char captured[65536];
static unsigned used,input_line;
static const char *const driver[]={"SET TALK OFF","SET SAFETY OFF","ON ERROR ? \"MINT-ERROR\", ERROR(), MESSAGE()","SET ALTERNATE TO CAPTURE.OUT","SET ALTERNATE ON","DO FNSTEST","SET ALTERNATE TO","QUIT"};
static void output(samir_pal_t *p,const char *s,uint32_t n)
{ (void)p; if(n<sizeof captured-used) { memcpy(captured+used,s,n); used+=n; captured[used]=0; } }
static int32_t input(samir_pal_t *p,char *s,uint32_t cap)
{ const char *v=input_line<sizeof driver/sizeof driver[0]?driver[input_line++]:"QUIT"; (void)p;snprintf(s,cap,"%s",v);return (int32_t)strlen(s); }
static int command(const char *s)
{ int rc=system(s); if(rc)fprintf(stderr,"command failed: %s\n",s); CHECK(rc==0,"FNS factory command succeeded");return rc; }
static size_t readfile(const char *path,char *buf,size_t cap)
{ FILE *f=fopen(path,"rb");size_t n=0;if(f){n=fread(buf,1,cap-1,f);fclose(f);}buf[n]=0;return n; }
static void diff_line(const char *label,const char *p,size_t n,size_t at)
{
    size_t start=at<n?at:n,end;
    if(!p){fprintf(stderr,"%s: <missing FNS-BEGIN>\n",label);return;}
    while(start&&p[start-1]!='\n')start--;
    end=start;while(end<n&&p[end]!='\r'&&p[end]!='\n')end++;
    fprintf(stderr,"%s: %.*s\n",label,(int)(end-start),p+start);
}
static void test_log_contract(xb_interp *ip)
{
    xb_ctx *ctx = xb_interp_ctx(ip);
    xb_val v;
    int ec, rc, before = g_fails;
    char picture[256];

    /* PM LOG review: an overflow value is a successful evaluation; only
     * the statement executor may raise its deferred error. */
    ctx->deferred_error = 0;
    rc = xb_interp_eval(ip, "LOG(0)", &v, &ec);
    CHECK(rc == INTERP_OK && ec == 0 && ctx->deferred_error == 58,
          "LOG(0) returns normally and defers error 58");
    CHECK(v.t == XB_N && v.u.n < -1e300 && v.num_width == 4,
          "LOG(0) returns negative infinity in its natural overflow picture");
    ctx->deferred_error = 87;
    rc = xb_interp_eval(ip, "LOG(-1)", &v, &ec);
    CHECK(rc == INTERP_OK && ec == 0 && ctx->deferred_error == 87,
          "LOG domain failure preserves the first deferred error");
    CHECK(v.t == XB_N && v.u.n != v.u.n && v.num_width == 5,
          "LOG(-1) returns NaN in its natural overflow picture");
    ctx->deferred_error = 0;

    rc = xb_interp_eval(ip, "LOG(22027)", &v, &ec);
    CHECK(rc == INTERP_OK && v.num_width == 5 && v.num_dec == 2,
          "LOG(22027) derives its width from two result integer digits");
    dec_format(v.u.n, v.num_width, v.num_dec, picture);
    picture[v.num_width] = 0;
    CHECK_STR_EQ(picture, "10.00", "LOG(22027) retains both decimal places");
    rc = xb_interp_eval(ip, "LOG(0.000001)", &v, &ec);
    CHECK(rc == INTERP_OK && v.num_width == 6 && v.num_dec == 2,
          "negative LOG result includes its integer digits and sign");
    dec_format(v.u.n, v.num_width, v.num_dec, picture);
    picture[v.num_width] = 0;
    CHECK_STR_EQ(picture, "-13.82", "negative LOG result retains its decimals");
    ctx->set_decimals = 0;
    rc = xb_interp_eval(ip, "LOG(22027)", &v, &ec);
    CHECK(rc == INTERP_OK && v.num_width == 2 && v.num_dec == 0,
          "LOG respects zero decimal places and derives integer-only width");
    ctx->set_decimals = 2;

    used = 0; captured[0] = 0;
    rc = samir_do(ip, "? \"left\", LOG(0), \"right\", LOG(-1), \"done\"");
    CHECK(rc == INTERP_OK && ctx->deferred_error == 0,
          "statement consumes deferred LOG error and ON ERROR handles it");
    CHECK_STR_EQ(captured,
        "\r\nleft **** right ***** done\r\nMINT-ERROR         58 LOG() : Zero or negative.",
        "complete output statement precedes deferred LOG error report");
    used = 0; captured[0] = 0;
    rc = samir_do(ip, "? LOG(0), LOG(\"X\")");
    CHECK(rc == INTERP_OK && ctx->deferred_error == 0 && ctx->error_number == 11,
          "an immediate argument error wins over a pending LOG domain error");
    CHECK_STR_EQ(captured, "\r\nMINT-ERROR         11 Invalid function argument.",
        "immediate error uses the ordinary ON ERROR report");
    ctx->deferred_error = 87;
    CHECK(samir_do(ip, "? \"nested\"") == INTERP_OK && ctx->deferred_error == 87,
          "nested statement execution restores its caller's deferred slot");
    ctx->deferred_error = 0;
    if (g_fails == before)
        puts(">>> test-samir-log-contract: green (deferred error ordering, first error, result widths)");
}
static int equal_text(const char *a,const char *b)
{
    const char *p=strstr(a,"[FNS-BEGIN]"),*q=strstr(b,"[FNS-BEGIN]");
    const char *e=p?strstr(p,"[FNS-END]"):NULL,*f=q?strstr(q,"[FNS-END]"):NULL;
    size_t n=e?(size_t)(e-p)+9:p?strlen(p):0,m=f?(size_t)(f-q)+9:q?strlen(q):0;
    if(!e||!f||n!=m||memcmp(p,q,n)) {
        size_t j=0;if(p&&q)while(j<n&&j<m&&p[j]==q[j])j++;
        fprintf(stderr,"FNS transcript differs at byte %lu (got 0x%02x, real 0x%02x; lengths %lu/%lu)\n",(unsigned long)j,
            p&&j<n?(unsigned char)p[j]:0,q&&j<m?(unsigned char)q[j]:0,(unsigned long)n,(unsigned long)m);
        diff_line("  got",p,n,j);diff_line("  real",q,m,j);return 0;
    }return 1;
}
int main(int argc,char **argv)
{
    const char *name=argc>1?argv[1]:"unknown",*mode=argc>2?argv[2]:"host";
    const char *com=argc>3?argv[3]:"build/SAMIR.COM";
    char gold[1024],dir[1024],cmd[16384],path[2048];
    static char expected[65536],minted[8192],current[8192];size_t mn,cn;
    int emu=!strcmp(mode,"emu");FILE *f;
    if(strspn(name,"abcdefghijklmnopqrstuvwxyz")!=strlen(name)||(!emu&&strcmp(mode,"host")))return 2;
    snprintf(gold,sizeof gold,"build/samirfns/golden/%s",name);
    snprintf(path,sizeof path,"%s/GOLD.OUT",gold);
    if(!readfile(path,expected,sizeof expected)){fprintf(stderr,"RED: MINT required: %s/GOLD.OUT\n",gold);return 2;}
    snprintf(path,sizeof path,"%s/FNSTEST.PRG",gold);mn=readfile(path,minted,sizeof minted);
    snprintf(path,sizeof path,"harness/diff/dbf_diff/xbase_prog_diff/corpus/fns/%s.prg",name);cn=readfile(path,current,sizeof current);
    if(!mn||mn!=cn||memcmp(minted,current,mn)){fprintf(stderr,"RED: MINT required: stale %s corpus\n",name);return 2;}
    snprintf(dir,sizeof dir,"build/samirfns/%s-%s",mode,name);
    snprintf(cmd,sizeof cmd,"rm -rf %s && mkdir -p %s/DATA && cp -f build/CLIENTS.DBF %s/BASE.DBF && cp -f %s/FNSTEST.PRG %s/FNSTEST.PRG && cp -f %s/LIBRARY.PRG %s/LIBRARY.PRG && cp -f %s/DATA/* %s/DATA/",dir,dir,dir,gold,dir,gold,dir,gold,dir);command(cmd);
    if(!strcmp(name,"default")||!strcmp(name,"path")){snprintf(cmd,sizeof cmd,"rm -f %s/BASE.DBF",dir);command(cmd);}
    if(!strcmp(name,"alias")||!strcmp(name,"relation")){snprintf(cmd,sizeof cmd,"cp -f build/CLIENTS.DBF %s/OTHER.DBF",dir);command(cmd);}
    if(emu){
        snprintf(path,sizeof path,"%s/data.img",dir);f=fopen(path,"wb");if(!f)return 2;fseek(f,1474560-1,SEEK_SET);fputc(0,f);fclose(f);
        snprintf(path,sizeof path,"%s/FNS.IN",dir);f=fopen(path,"wb");if(!f)return 2;
        for(unsigned i=0;i<sizeof driver/sizeof driver[0];i++)fprintf(f,"%s\r\n",driver[i]);
        fclose(f);
        snprintf(path,sizeof path,"%s/FNS.BAT",dir);f=fopen(path,"wb");if(!f)return 2;fputs("@echo off\r\nsamir < fns.in > result.txt\r\n",f);fclose(f);
        snprintf(cmd,sizeof cmd,"mformat -i %s/data.img -f 1440 :: && mcopy -o -i %s/data.img %s ::SAMIR.COM && mcopy -o -i %s/data.img %s/*.PRG %s/*.IN %s/*.BAT :: && mmd -i %s/data.img ::DATA && mcopy -o -i %s/data.img %s/DATA/* ::DATA/",dir,dir,com,dir,dir,dir,dir,dir,dir,dir);command(cmd);
        if(strcmp(name,"default")&&strcmp(name,"path")){snprintf(cmd,sizeof cmd,"mcopy -o -i %s/data.img %s/BASE.DBF ::",dir,dir);command(cmd);}
        if(!strcmp(name,"alias")||!strcmp(name,"relation")){snprintf(cmd,sizeof cmd,"mcopy -o -i %s/data.img %s/OTHER.DBF ::",dir,dir);command(cmd);}
        snprintf(cmd,sizeof cmd,"fsck.fat -n %s/data.img > %s/fsck-before.log",dir,dir);command(cmd);
        snprintf(cmd,sizeof cmd,"INITECH_QMP_STDIO=1 build/qemu_harness --disk build/tracer_secondary.img --disk2 %s/data.img --name fns_%s --out build --timeout-ms 60000 --keys 'f,n,s,ret,e,x,i,t,ret' --keys-after SHELL-READY --quit-after SHELL-DONE > %s/qemu.out 2> %s/qemu.report",dir,name,dir,dir);command(cmd);
        snprintf(path,sizeof path,"%s/qemu.report",dir);readfile(path,captured,sizeof captured);
        CHECK(strstr(captured,"triple_fault=0")&&strstr(captured,"quit_after_found=1")&&strstr(captured,"keys_sent=9"),"FNS entire locked QEMU trace returned without fault");
        snprintf(cmd,sizeof cmd,"mcopy -o -i %s/data.img ::CAPTURE.OUT %s/CAPTURE.OUT",dir,dir);command(cmd);
        snprintf(cmd,sizeof cmd,"fsck.fat -n %s/data.img > %s/fsck-after.log",dir,dir);command(cmd);
    }else{
        char cwd[2048];samir_pal_t *pal;xb_interp *ip;
        if(!getcwd(cwd,sizeof cwd)||chdir(dir))return 2;
        pal=pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});if(!pal)return 2;
        pal->conout=output;pal->conin_line=input;ip=xb_interp_make(pal);
        CHECK(ip&&samir_repl(pal,ip)==0,"FNS full disk program returned");
        if(ip && !strcmp(name,"log"))test_log_contract(ip);
        if(ip)xb_interp_free(ip);
        pal_host_free(pal);if(chdir(cwd))return 2;
    }
    snprintf(path,sizeof path,"%s/CAPTURE.OUT",dir);readfile(path,captured,sizeof captured);
    snprintf(path,sizeof path,"%s/captured.out",dir);f=fopen(path,"wb");if(f){fwrite(captured,1,strlen(captured),f);fclose(f);}
    CHECK(equal_text(captured,expected),"FNS transcript byte-identical to real III+");
    return TEST_SUMMARY("test-samir-fns");
}
