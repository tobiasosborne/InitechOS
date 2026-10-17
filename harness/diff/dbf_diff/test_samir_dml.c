/* SAMIR DML: real III+ 1.1 output, same corpus in host and injected QEMU.
 * Ground truth: dml_mint.sh; Using III+ U5-28/51/53/59/121/131/143/172/
 * 175/180/185/187/269/276/278 and U6-7/8. No expected bytes from SAMIR.
 * DBF comparison independently reads field descriptors and physical records;
 * NDX comparison uses the existing independent ndx_ref.py (ADR-0008 DEC-05).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test_assert.h"
#include "samir/interp.h"
TEST_HARNESS();
struct pal_host_cfg { uint8_t date_yy,date_mm,date_dd; uint32_t heap_size; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(samir_pal_t *);
static char captured[65536];
static unsigned used, input_line;
static xb_interp *state_ip;
static const char *state_name;
static void output(samir_pal_t *p,const char *s,uint32_t n)
{ (void)p; if(n<sizeof captured-used) { memcpy(captured+used,s,n);used+=n;captured[used]=0; } }
static int32_t input(samir_pal_t *p,char *s,uint32_t n)
{ const char *v=input_line++==0?"DO DMLTEST":"QUIT";(void)p;snprintf(s,n,"%s",v);return (int32_t)strlen(s); }
/* Separate manual-grounded state checks. These do not replace or normalize
 * the exact real-output gates, which must still pass before acceptance. */
static int32_t state_input(samir_pal_t *p,char *s,uint32_t n)
{
    static const char *const macro[]={"N=1000","CMD='N=N+5'","&CMD",NULL};
    static const char *const close[]={"USE BASE INDEX NAMES","GO 2","CLOSE FORMAT","CLOSE PROCEDURE","CLOSE INDEX",NULL};
    static const char *const clear[]={"DO DMLTEST",NULL};
    const char *const *commands=!strcmp(state_name,"macro")?macro:!strcmp(state_name,"close")?close:clear;
    const char *v=commands[input_line++];xb_val value;wa_env *env;
    (void)p;
    if(v){snprintf(s,n,"%s",v);return (int32_t)strlen(s);}
    env=xb_interp_env(state_ip);
    if(!strcmp(state_name,"clear")) {
        CHECK(xb_interp_get_memvar(state_ip,"N",&value)!=0,"DML state clear removed hidden PRIVATE N");
        CHECK(xb_interp_get_memvar(state_ip,"PN",&value)!=0,"DML state clear removed PUBLIC PN");
        CHECK(wa_nrec(env,1)==3,"DML state clear preserved open table");
    } else if(!strcmp(state_name,"macro")) {
        CHECK(xb_interp_get_memvar(state_ip,"N",&value)==0&&value.t==XB_N&&value.u.n==1005,"DML state macro executed substituted assignment");
    } else {
        CHECK(wa_is_open(env,1)&&wa_nrec(env,1)==3&&wa_recno(env,1)==2,"DML state close preserved DBF and cursor");
        CHECK(wa_index_count(env,1)==0,"DML state close released indexes only");
    }
    return -1;
}
static int command(const char *s)
{ int rc=system(s);if(rc) fprintf(stderr,"command failed: %s\n",s);CHECK(rc==0,"factory command succeeded");return rc; }
static size_t readfile(const char *path,unsigned char *s,size_t cap)
{
    FILE *f=fopen(path,"rb");size_t n=0;
    if(f) { n=fread(s,1,cap-1,f);fclose(f); } s[n]=0;return n;
}
static int equal_text(const char *a,const char *b)
{
    const char *p=strstr(a,"[DML-BEGIN]"),*q=strstr(b,"[DML-BEGIN]");
    const char *e=p?strstr(p,"[DML-END]"):NULL,*f=q?strstr(q,"[DML-END]"):NULL;
    size_t n=e?(size_t)(e-p)+9:p?strlen(p):0,m=f?(size_t)(f-q)+9:q?strlen(q):0;
    if(!e||!f||n!=m||memcmp(p,q,n)) {
        size_t j=0;if(p&&q) while(j<n&&j<m&&p[j]==q[j]) j++;
        fprintf(stderr,"DML transcript differs at byte %lu (got 0x%02x, real 0x%02x; lengths %lu/%lu)\n",(unsigned long)j,
                p&&j<n?(unsigned char)p[j]:0,q&&j<m?(unsigned char)q[j]:0,(unsigned long)n,(unsigned long)m);
        return 0;
    } return 1;
}
static unsigned u16(const unsigned char *p) { return p[0]+256u*p[1]; }
static unsigned u32(const unsigned char *p) { return u16(p)+65536u*u16(p+2); }
static int equal_dbf(const unsigned char *a,size_t an,const unsigned char *b,size_t bn)
{
    unsigned ah,bh,ar,br,n,i,af,bf;
    if(an<33||bn<33)return 0;
    ah=u16(a+8);bh=u16(b+8);ar=u16(a+10);br=u16(b+10);n=u32(a+4);
    af=(ah-33)/32;bf=(bh-33)/32;
    if(a[0]!=b[0]||n!=u32(b+4)||ar!=br||af!=bf||ah+(size_t)n*ar>an||bh+(size_t)n*br>bn)return 0;
    for(i=0;i<af;i++) {
        const unsigned char *x=a+32+i*32,*y=b+32+i*32;
        if(memcmp(x,y,12)||memcmp(x+16,y+16,2))return 0;
    }
    return !memcmp(a+ah,b+bh,(size_t)n*ar);
}
static int equal_mem(unsigned char *a,size_t an,unsigned char *b,size_t bn)
{
    size_t i=0;
    if(an!=bn)return 0;
    while(i<an&&a[i]!=26&&b[i]!=26) {
        unsigned n,t;if(i+32>an)return 0;
        /* mem.md Sec 4: serialized transient pool addresses are not data. */
        memset(a+i+12,0,4);memset(b+i+12,0,4);
        t=a[i+11]&127;n=t=='C'?u16(a+i+16):t=='L'?1:8;
        i+=32+n;
    }
    return i+1==an&&!memcmp(a,b,an);
}
static void compare_file(const char *dir,const char *gold,const char *file)
{
    static unsigned char a[65536],b[65536];char p[2048],q[2048],why[128],cmd[16384];
    size_t an,bn;int eq;
    snprintf(p,sizeof p,"%s/%s",dir,file);snprintf(q,sizeof q,"%s/%s",gold,file);
    an=readfile(p,a,sizeof a);bn=readfile(q,b,sizeof b);
    if(strstr(file,".NDX")) {
        snprintf(cmd,sizeof cmd,"python3 harness/diff/dbf_diff/ndx_ref.py --index-dump '%s' > build/samirdml/got.ndx && python3 harness/diff/dbf_diff/ndx_ref.py --index-dump '%s' > build/samirdml/want.ndx && cmp build/samirdml/got.ndx build/samirdml/want.ndx",p,q);
        eq=an&&bn&&command(cmd)==0;
    } else if(strstr(file,".DBF"))eq=equal_dbf(a,an,b,bn);
    else if(strstr(file,".MEM"))eq=equal_mem(a,an,b,bn);
    else eq=an&&an==bn&&!memcmp(a,b,an);
    snprintf(why,sizeof why,"DML %s bytes match real III+",file);CHECK(eq,why);
}
int main(int argc,char **argv)
{
    const char *name=argc>1?argv[1]:"clear",*mode=argc>2?argv[2]:"host";
    const char *com=argc>3?argv[3]:"build/SAMIR.COM";
    static const char *const names[]={"clear","macro","close","index","reindex","sort","total","append","save","restore","copyfile","type","rename","erase","label","run"};
    char gold[1024],dir[1024],cmd[16384],path[2048];
    static unsigned char expected[65536];FILE *f;
    int emu=!strcmp(mode,"emu"),state=!strcmp(mode,"state"),valid=0;unsigned ni;
    for(ni=0;ni<sizeof names/sizeof names[0];ni++)if(!strcmp(name,names[ni]))valid=1;
    if(!valid||(!emu&&!state&&strcmp(mode,"host")))return 2;
    if(state&&strcmp(name,"clear")&&strcmp(name,"macro")&&strcmp(name,"close"))return 2;
    snprintf(gold,sizeof gold,"build/samirdml/golden/%s",name);
    snprintf(path,sizeof path,"%s/GOLD.OUT",gold);
    if(!readfile(path,expected,sizeof expected)) { fprintf(stderr,"RED: MINT required: %s/GOLD.OUT (make mint-samir-dml)\n",gold);return 2; }
    {
        static unsigned char minted[8192],current[8192];size_t mn,cn;
        snprintf(path,sizeof path,"%s/DMLTEST.PRG",gold);mn=readfile(path,minted,sizeof minted);
        snprintf(path,sizeof path,"harness/diff/dbf_diff/xbase_prog_diff/corpus/dml/%s.prg",name);cn=readfile(path,current,sizeof current);
        if(!mn||mn!=cn||memcmp(minted,current,mn)) {
            fprintf(stderr,"RED: MINT required: %s corpus changed since real III+ run\n",name);return 2;
        }
    }
    snprintf(dir,sizeof dir,"build/samirdml/%s-%s",mode,name);
    snprintf(cmd,sizeof cmd,"rm -rf %s && mkdir -p %s && cp -f build/CLIENTS.DBF %s/BASE.DBF && cp -f harness/diff/dbf_diff/xbase_prog_diff/corpus/dml/%s.prg %s/DMLTEST.PRG",dir,dir,dir,name,dir);command(cmd);
    snprintf(path,sizeof path,"%s/INPUT.TXT",dir);f=fopen(path,"wb");if(!f)return 2;fputs("ASCII first\r\nsecond line\r\n\032IGNORED\r\n",f);fclose(f);
    if(!strcmp(name,"reindex")||!strcmp(name,"restore")||!strcmp(name,"append")||!strcmp(name,"label")) {
        const char *files=!strcmp(name,"reindex")?"NAMES.NDX":!strcmp(name,"restore")?"VALUES.MEM":!strcmp(name,"append")?"IMPORT.DBF":"CLIENTS.DBF ADDRESS.LBL";
        snprintf(cmd,sizeof cmd,"for f in %s; do cp -f %s/$f %s/$f; done",files,gold,dir);command(cmd);
    }
    if(state&&!strcmp(name,"close")) {
        snprintf(cmd,sizeof cmd,"cp -f build/samirdml/golden/index/NAMES.NDX %s/NAMES.NDX",dir);command(cmd);
    }
    if(emu) {
        const char *trace="d,m,l,ret,e,x,i,t,ret";
        snprintf(path,sizeof path,"%s/data.img",dir);f=fopen(path,"wb");if(!f)return 2;fseek(f,1474560-1,SEEK_SET);fputc(0,f);fclose(f);
        snprintf(path,sizeof path,"%s/DML.IN",dir);f=fopen(path,"wb");if(!f)return 2;fputs("DO DMLTEST\r\nQUIT\r\n",f);fclose(f);
        snprintf(path,sizeof path,"%s/DML.BAT",dir);f=fopen(path,"wb");if(!f)return 2;fputs("@echo off\r\nsamir < dml.in > result.txt\r\ntype result.txt\r\n",f);fclose(f);
        snprintf(cmd,sizeof cmd,"mformat -i %s/data.img -f 1440 :: && mcopy -o -i %s/data.img %s ::SAMIR.COM && mcopy -o -i %s/data.img %s/*.DBF %s/*.PRG %s/*.TXT %s/*.IN %s/*.BAT ::",dir,dir,com,dir,dir,dir,dir,dir,dir);command(cmd);
        snprintf(cmd,sizeof cmd,"for f in %s/*.NDX %s/*.MEM %s/*.LBL; do test ! -f \"$f\" || mcopy -o -i %s/data.img \"$f\" ::; done",dir,dir,dir,dir);command(cmd);
        snprintf(cmd,sizeof cmd,"fsck.fat -n %s/data.img > %s/fsck-before.log",dir,dir);command(cmd);
        snprintf(cmd,sizeof cmd,"INITECH_QMP_STDIO=1 build/qemu_harness --disk build/tracer_secondary.img --disk2 %s/data.img --name dml_%s --out build --timeout-ms 60000 --keys '%s' --keys-after SHELL-READY --quit-after SHELL-DONE > %s/qemu.out 2> %s/qemu.report",dir,name,trace,dir,dir);command(cmd);
        snprintf(path,sizeof path,"%s/qemu.report",dir);readfile(path,(unsigned char *)captured,sizeof captured);
        CHECK(strstr(captured,"triple_fault=0")&&strstr(captured,"quit_after_found=1")&&strstr(captured,"keys_sent=9"),"DML entire locked QEMU trace returned without fault");
        snprintf(cmd,sizeof cmd,"mcopy -o -i %s/data.img ::RESULT.TXT %s/RESULT.TXT",dir,dir);command(cmd);
        snprintf(path,sizeof path,"%s/RESULT.TXT",dir);readfile(path,(unsigned char *)captured,sizeof captured);
        snprintf(cmd,sizeof cmd,"fsck.fat -n %s/data.img > %s/fsck-after.log",dir,dir);command(cmd);
    } else {
        char cwd[2048];samir_pal_t *pal;xb_interp *ip;
        getcwd(cwd,sizeof cwd);if(chdir(dir))return 2;
        pal=pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});if(!pal)return 2;
        pal->conout=output;pal->conin_line=state?state_input:input;ip=xb_interp_make(pal);
        state_ip=ip;state_name=name;
        CHECK(ip&&samir_repl(pal,ip)==0,"DML full disk program returned");
        if(ip)xb_interp_free(ip);
        pal_host_free(pal);if(chdir(cwd))return 2;
    }
    snprintf(path,sizeof path,"%s/captured.out",dir);f=fopen(path,"wb");if(f){fwrite(captured,1,strlen(captured),f);fclose(f);}
    if(state) {
        compare_file(dir,gold,"BASE.DBF");
        return TEST_SUMMARY("test-samir-dml-state (manual-grounded; exact differential remains mandatory)");
    }
    CHECK(equal_text(captured,(char *)expected),"DML transcript byte-identical to real III+");
    if(!strcmp(name,"run") && !strstr(captured,"[DML-END]"))
        fprintf(stderr,"RED RUN: PAL has no EXEC seam; loader.c:1193 rejects nested EXEC with LOADER_ERR_BUSY (zs24 tests sequential subdirectory EXEC).\n");
    {
        const char *artifact=!strcmp(name,"index")||!strcmp(name,"reindex")?"NAMES.NDX":!strcmp(name,"sort")?"SORTED.DBF":!strcmp(name,"total")?"TOTALS.DBF":!strcmp(name,"save")?"VALUES.MEM":!strcmp(name,"copyfile")?"COPIED.TXT":!strcmp(name,"rename")?"RENAMED.TXT":!strcmp(name,"label")?"LABELS.TXT":!strcmp(name,"run")?"VERSION.TXT":NULL;
        if(emu) {
            snprintf(cmd,sizeof cmd,"mcopy -o -i %s/data.img ::BASE.DBF %s/BASE.DBF",dir,dir);command(cmd);
            if(artifact) { snprintf(cmd,sizeof cmd,"mcopy -o -i %s/data.img ::%s %s/%s",dir,artifact,dir,artifact);command(cmd); }
        }
        compare_file(dir,gold,"BASE.DBF");
        if(artifact)compare_file(dir,gold,artifact);
        if(!strcmp(name,"erase")||!strcmp(name,"rename")) {
            if(emu) { snprintf(cmd,sizeof cmd,"mcopy -o -i %s/data.img ::INPUT.TXT %s/probe.txt > %s/probe.log 2>&1",dir,dir,dir);CHECK(system(cmd)!=0,"DML original file absent on FAT"); }
            else { snprintf(path,sizeof path,"%s/INPUT.TXT",dir);CHECK(access(path,F_OK)!=0,"DML original file absent"); }
        }
    }
    return TEST_SUMMARY("test-samir-dml");
}
