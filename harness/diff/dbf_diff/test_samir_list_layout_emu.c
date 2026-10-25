/* initech-31d1: real dBASE III PLUS 1.1 mint 005 LIST/DISPLAY tables.
 * Programs and expected bytes come from read-only M005U/M005N.PRG and
 * M005ULOG/M005NLOG.TXT (re/mint-results-005.md; md5-pinned by make).
 * Replay each original command, wrapping table commands in disk PRGs to
 * preserve program context. Markers delimit output without supplying a CR LF.
 * mtools stages identical corpus inputs and extracts stdout; fsck.fat checks
 * the volume. The screen projects the first REAL M005U table, never SAMIR.
 */
#include "scratch_lock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_assert.h"
TEST_HARNESS();
#define D "build/samir-list-layout"
static char serial[262144], output[65536];
static char tables[2][8][4096];
static int counts[2];
static int command(const char *s)
{ int rc=system(s); CHECK(rc==0,s); return rc; }
static size_t slurp(const char *path,char *s,size_t cap)
{
    FILE *f=fopen(path,"rb");size_t n=0;
    CHECK(f!=NULL,"layout evidence file opened");
    if(f) { n=fread(s,1,cap-1,f);fclose(f); } s[n]=0;return n;
}
static FILE *create(const char *path)
{ FILE *f=fopen(path,"wb");CHECK(f!=NULL,"layout replay file opened");return f; }
static int plant(const char *path,const char *name)
{
    char cmd[4096];
    snprintf(cmd,sizeof cmd,"mcopy -o -i " D "/data.img '%s' ::%s",path,name);
    return command(cmd);
}
static void prepare(const char *corpus,int which)
{
    char path[2048],prg[8192],gold[8192],name[32];
    char *p,*g;int count=0;FILE *input;
    char letter=which?'N':'U';
    snprintf(path,sizeof path,"%s/mint/work/M005%cLOG.TXT",corpus,letter);
    slurp(path,gold,sizeof gold);g=gold;
    snprintf(path,sizeof path,"%s/mint/work/M005%c.PRG",corpus,letter);
    slurp(path,prg,sizeof prg);
    snprintf(path,sizeof path,D "/%c.IN",letter);input=create(path);
    if(!input) exit(2);
    for(p=prg;*p;) {
        char *e=p;while(*e && *e!='\r' && *e!='\n') e++;
        if(*e) *e++=0;
        while(*e=='\r' || *e=='\n') e++;
        if(!strncmp(p,"LIST ",5) || !strncmp(p,"DISPLAY ",8)) {
            FILE *f;char *start=strstr(g,"\r\nRecord#"),*end;
            CHECK(count<8 && start!=NULL,"each original table command has a real golden table");
            if(count>=8 || !start) exit(2);
            end=strstr(start+2,"\r\n\r\n");
            CHECK(end && end+2-start<(int)sizeof tables[0][0],"real table bounds include opening and final CR LF");
            if(!end || end+2-start>=(int)sizeof tables[0][0]) exit(2);
            memcpy(tables[which][count],start,(size_t)(end+2-start));g=end+2;
            snprintf(name,sizeof name,"%c%d.PRG",letter,count);
            snprintf(path,sizeof path,D "/%s",name);f=create(path);
            if(!f) exit(2);
            fprintf(f,"?? '[M005%c%d]'\r\n%s\r\n?? '[/M005%c%d]'\r\n",letter,count,p,letter,count);
            CHECK(fclose(f)==0,"verbatim table PRG written");plant(path,name);
            fprintf(input,"DO %c%d\r\n",letter,count++);
        } else if(*p && strncmp(p,"SET ALTERNATE",13) && strncmp(p,"ON ERROR",8) && strncmp(p,"QUIT",4))
            fprintf(input,"%s\r\n",p);
        p=e;
    }
    fprintf(input,"QUIT\r\n");CHECK(fclose(input)==0,"original command replay written");
    counts[which]=count;
    CHECK(count==(which?8:1),"M005U has one table; M005N has eight including DISPLAY");
    snprintf(path,sizeof path,D "/%c.IN",letter);
    snprintf(name,sizeof name,"%c.IN",letter);plant(path,name);
}
static void grade(const char *got,int which,const char *transport)
{
    int i;char begin[32],end[32],why[128];char letter=which?'N':'U';
    for(i=0;i<counts[which];i++) {
        const char *p,*q;size_t n=strlen(tables[which][i]);int equal;
        snprintf(begin,sizeof begin,"[M005%c%d]",letter,i);
        snprintf(end,sizeof end,"[/M005%c%d]",letter,i);
        p=strstr(got,begin);if(p) p+=strlen(begin);
        q=p?strstr(p,end):NULL;
        equal=p && q && (size_t)(q-p)==n && !memcmp(p,tables[which][i],n);
        snprintf(why,sizeof why,"M005%c table %d %s byte-identical to real dBASE",letter,i,transport);
        if(!equal) {
            size_t j=0;
            if(p && q) while(j<n && p+j<q && p[j]==tables[which][i][j]) j++;
            fprintf(stderr,"LIST M005%c table %d %s differs at byte %lu\n",letter,i,transport,(unsigned long)j);
        }
        CHECK(equal,why);
    }
}
int main(int argc,char **argv)
{
    scratch_lock("build/.lock-samir-list-layout");
    const char *mode=argc>1?argv[1]:"files",*com=argc>2?argv[2]:"build/SAMIR.COM";
    const char *corpus=argc>3?argv[3]:"../dbase3-decomp";
    static const char *const inputs[]={"TRAVEL.DBF","TRAVEL.DBT","TRIPS.NDX","AVAL_FLT.DBF","AVAL_FLT.NDX","FLT_NO.NDX"};
    int screen=!strcmp(mode,"screen"),i;FILE *f;
    char cmd[12288],path[2048],report[8192];
    const char *trace=screen?
        "s,a,m,i,r,ret,u,s,e,spc,t,r,a,v,e,l,spc,i,n,d,e,x,spc,t,r,i,p,s,ret,d,o,spc,s,c,r,e,e,n,ret":
        "l,a,y,o,u,t,ret,e,x,i,t,ret";
    command("mkdir -p " D);
    f=create(D "/data.img");if(!f) return 2;
    CHECK(fseek(f,1474560-1,SEEK_SET)==0 && fputc(0,f)!=EOF,"FAT image sized");fclose(f);
    command("mformat -i " D "/data.img -f 1440 ::");plant(com,"SAMIR.COM");
    for(i=0;i<6;i++) {
        snprintf(path,sizeof path,"%s/mint/work/%s",corpus,inputs[i]);plant(path,inputs[i]);
        snprintf(cmd,sizeof cmd,"mcopy -o -i " D "/data.img ::%s " D "/%s && cmp '%s' " D "/%s",inputs[i],inputs[i],path,inputs[i]);
        command(cmd);
    }
    prepare(corpus,0);prepare(corpus,1);
    f=create(D "/LAYOUT.BAT");if(!f) return 2;
    fputs("@echo off\r\nsamir < u.in > u.out\r\nsamir < n.in > n.out\r\ntype u.out\r\ntype n.out\r\n",f);fclose(f);
    plant(D "/LAYOUT.BAT","LAYOUT.BAT");
    command("fsck.fat -n " D "/data.img > " D "/fsck-before.log");
    if(screen) {
        char source[2048],*p;int n;
        /* Force a deterministic final 80x25 viewport with factory blank lines.
         * Original U table then occupies rows 21..23; row 24 is the prompt. */
        snprintf(path,sizeof path,D "/U0.PRG");slurp(path,source,sizeof source);
        f=create(D "/SCREEN.PRG");if(!f) return 2;
        for(n=0;n<30;n++) fputs("? ''\r\n",f);
        fputs(source,f);fclose(f);plant(D "/SCREEN.PRG","SCREEN.PRG");
        f=create(D "/screen.cells");if(!f) return 2;
        p=tables[0][0]+2;
        for(n=21;n<=23;n++) {
            char cell[81],*e=strstr(p,"\r\n");size_t len=e?(size_t)(e-p):0;
            CHECK(e && len<=80,"real U heading and two rows fit console cells");
            if(!e || len>80) return 2;
            memset(cell,' ',80);memcpy(cell,p,len);cell[80]=0;
            fprintf(f,"%d 0 %s\n",n,cell);p=e+2;
        }
        fclose(f);
        snprintf(cmd,sizeof cmd,"build/qemu_harness --disk build/tracer_secondary.img --disk2 " D "/data.img --name samir_list_layout_screen --out build --timeout-ms 45000 --keys '%s' --keys-after SHELL-READY --screendump --screendump-after '[/M005U0]. ' > " D "/screen.out 2> " D "/screen.report",trace);
    } else
        snprintf(cmd,sizeof cmd,"build/qemu_harness --disk build/tracer_secondary.img --disk2 " D "/data.img --name samir_list_layout_files --out build --timeout-ms 60000 --keys '%s' --keys-after SHELL-READY --quit-after SHELL-DONE > " D "/files.out 2> " D "/files.report; rc=$?; cat " D "/files.report 1>&2; exit $rc",trace);
    command(cmd);
    snprintf(path,sizeof path,D "/%s.report",mode);slurp(path,report,sizeof report);
    CHECK(strstr(report,"triple_fault=0")!=NULL,"layout guest did not triple fault");
    CHECK(strstr(report,screen?"screendump_taken=1":"quit_after_found=1")!=NULL,"layout trace completed");
    {
        unsigned expected=1;const char *t=trace,*sent=strstr(report,"keys_sent=");
        while(*t) if(*t++==',') expected++;
        CHECK(sent && strtoul(sent+10,NULL,10)==expected,"entire locked layout trace injected");
    }
    snprintf(path,sizeof path,"build/samir_list_layout_%s.serial",mode);slurp(path,serial,sizeof serial);
    CHECK(strstr(serial,"SHELL-READY") && !strstr(serial,"PC LOAD LETTER"),"layout OS booted without panic");
    if(screen) {
        grade(serial,0,"screen serial");
        command("build/ppm_text_check --cells build/samir_list_layout_screen.ppm " D "/screen.cells");
    } else {
        grade(serial,0,"serial");grade(serial,1,"serial");
        for(i=0;i<2;i++) {
            snprintf(cmd,sizeof cmd,"mcopy -o -i " D "/data.img ::%c.OUT " D "/%c.OUT",i?'N':'U',i?'N':'U');command(cmd);
            snprintf(path,sizeof path,D "/%c.OUT",i?'N':'U');slurp(path,output,sizeof output);grade(output,i,"stdout file");
        }
    }
    command("fsck.fat -n " D "/data.img > " D "/fsck-after.log");
    return TEST_SUMMARY(screen?"test-samir-list-layout-emu (screen)":"test-samir-list-layout-emu (files)");
}
