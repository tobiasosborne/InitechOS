/* LOOK pass 6 OS oracle. Numeric/status values are manual-derived:
 * Using III+ U5-144/210/235/259, PDF 256/322/347/371. LIST/DISPLAY spacing
 * re-keyed initech-31d1: real M005U/M005N, 7 columns + 2 spaces, final
 * heading unpadded (dbase3-decomp/re/mint-results-005.md). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_assert.h"
#include "trace_complete.h"
#include "samir_install.h"
TEST_HARNESS();
static char serial[262144];
static int command(const char *s) { int rc=system(s);CHECK(rc==0,s);return rc; }
static void slurp(const char *name,char *out,size_t cap)
{
    FILE *f=fopen(name,"rb");size_t n=0;
    CHECK(f!=NULL,"emulator artifact exists");
    if(f) { n=fread(out,1,cap-1,f);fclose(f); }out[n]=0;
}
static void keys(const char *s,char *out)
{
    while(*s) {
        const char *key;char one[2]={*s,0};
        if(*s=='\n') key="ret";else if(*s==' ') key="spc";
        else if(*s=='.') key="dot";else key=one;
        strcpy(out,key);out+=strlen(key);s++;
        if(*s) *out++=',';
    }
    *out=0;
}
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"status";
    const char *com=argc>2?argv[2]:"build/SAMIR.EXE";
    char cmd[16384],keybuf[4096],report[4096],path[1024];
    int status=!strcmp(mode,"status"),off=!strcmp(mode,"off"),console=!strcmp(mode,"console");
    /* Scratch key (initech-2kkqr): a mutant run (non-default SAMIR.EXE) must not share
     * image/report/serial/PPM paths with the accepted leg of the same mode, or with
     * another mutant, when gates run concurrently under make -j. Default COM => key==mode. */
    char key[512]; { const char *b=strrchr(com,'/'); b=b?b+1:com;
      if(!strcmp(com,"build/SAMIR.EXE")) snprintf(key,sizeof key,"%s",mode);
      else snprintf(key,sizeof key,"%s_%s",mode,b); }
    /* The QMP transport (socket or INITECH_QMP_STDIO=1 stdio) is the harness's
     * choice, not this oracle's: the Codex lane's sandbox needed stdio, the
     * certificate runs the default (PM, 2026-10-15). */
    if(command("mkdir -p build/samir-screen")) return 2;
    snprintf(cmd,sizeof(cmd),"cp -f build/samir_list.img build/samir-screen/%s.img",key);
    if(command(cmd)) return 2;
    {char img[1024];snprintf(img,sizeof img,"build/samir-screen/%s.img",key);CHECK(samir_install_cmd(cmd,sizeof cmd,img,com)==0,"SAMIR.EXE + SAMIR.OVL named");}
    if(command(cmd)) return 2;
    if(console) {
        FILE *f; snprintf(path,sizeof(path),"build/samir-screen/SCREEN_%s.PRG",key); f=fopen(path,"wb");
        const char *program="SET CONSOLE OFF\r\n? 'HIDDEN ROW'\r\nACCEPT 'HIDDEN PROMPT' TO answer\r\nSET CONSOLE ON\r\n? answer\r\nSET CONSOLE OFF\r\n? 1+'x'\r\n";
        CHECK(f!=NULL,"console: program fixture opened");if(!f) return 2;
        CHECK(fwrite(program,1,strlen(program),f)==strlen(program),"console: program fixture written");fclose(f);
        snprintf(cmd,sizeof(cmd),"mcopy -o -i build/samir-screen/%s.img %s ::SCREEN.PRG",key,path);
        if(command(cmd)) return 2;
        keys("samir\nuse clients\nset console off\ndisplay\ndo screen\nsecret\n",keybuf);
    } else if(status || off) keys(off?
        "samir\nuse clients\nset status on\ngo 2\ndisplay\nset status off\n":
        "samir\nuse clients\nset status on\ngo 2\ndisplay\n",keybuf);
    else keys("samir\nuse clients\nset heading off\nlist\ngo 2\ndisplay\n",keybuf);
    snprintf(cmd,sizeof(cmd),"build/qemu_harness --disk build/tracer_secondary.img --disk2 build/samir-screen/%s.img --name samir_screen_%s --out build --timeout-ms 45000 --keys '%s' --keys-after SHELL-READY --screendump --screendump-after SHELL-READY > build/samir-screen/%s.out 2> build/samir-screen/%s.report",key,key,keybuf,key,key);
    /* A live screenshot leg intentionally stops on the bounded timeout. */
    if(system(cmd)!=0)return 2;
    snprintf(path,sizeof(path),"build/samir-screen/%s.report",key);slurp(path,report,sizeof(report));
    CHECK(strstr(report,"triple_fault=0")!=NULL,"guest did not triple fault");
    { const char *sent=strstr(report,"keys_sent=");unsigned expected=1;const char *k=keybuf;
      while(*k) if(*k++==',') expected++;
      CHECK(sent && strtoul(sent+10,NULL,10)==expected,"entire input trace injected"); }
    snprintf(path,sizeof(path),"build/samir_screen_%s.serial",key);
    { char rp[1024];snprintf(rp,sizeof rp,"build/samir-screen/%s.report",key);
      if(!emu_trace_complete(rp,path,emu_trace_keys(keybuf)))return 2; }
    slurp(path,serial,sizeof(serial));
    CHECK(strstr(serial,"SHELL-READY")!=NULL,"shell booted");
    CHECK(strstr(serial,"File already exists")==NULL,"screen settings accepted");
    CHECK(strstr(serial,"PC LOAD LETTER")==NULL,"guest did not panic");
    if(console) {
        const char *p=serial;int occurrences=0;
        while((p=strstr(p,"secret"))!=NULL) { occurrences++;p++; }
        CHECK(strstr(serial,"PESTON")!=NULL,"console: dot DISPLAY remains visible");
        CHECK(!strstr(serial,"HIDDEN ROW") && !strstr(serial,"HIDDEN PROMPT"),"console: program output and prompt suppressed");
        CHECK(occurrences==1,"console: ACCEPT typing hidden, accepted value printed once");
        CHECK(strstr(serial,"9  Data type mismatch.")!=NULL,"console: errors remain visible under OFF");
    } else if(status || off) {
        CHECK(strstr(serial,"Command Line")!=NULL,"status: serial Command Line");
        CHECK(strstr(serial,"CLIENTS")!=NULL,"status: serial database name");
        /* STATUS draws positioned spans: CR LF moves its cursor without
         * echoing newline bytes to serial. The next span is record 2. */
        CHECK(strstr(serial,"Record#  NAME       CITY              BAL      2  WADDAMS")!=NULL,"heading: column-aligned DISPLAY titles");
        CHECK(strstr(serial,"Rec: 2/3")!=NULL,"status: serial live record count");
        CHECK(strstr(serial,"Enter a dBASE III PLUS command.")!=NULL,"status: serial guidance");
        if(status) {
            snprintf(cmd,sizeof(cmd),"build/ppm_text_check --samir-status build/samir_screen_%s.ppm 352 368 7000 384 600",key);
            CHECK(system(cmd)==0,"status: reverse band on reserved row 22");
            snprintf(cmd,sizeof(cmd),"build/ppm_text_check build/samir_screen_%s.ppm 368 384 300 384 600",key);
            CHECK(system(cmd)==0,"status: guidance ink on row 23");
        } else {
            snprintf(cmd,sizeof(cmd),"build/ppm_text_check build/samir_screen_%s.ppm 0 16 1 352 0",key);
            CHECK(system(cmd)==0,"status: OFF erases both reserved rows");
        }
    } else {
        CHECK(strstr(serial,"      1  PESTON     HONOLULU      1234.50")!=NULL,"numeric: LIST row 1 fixed N(8,2)");
        CHECK(strstr(serial,"      2  WADDAMS    AUSTIN         -42.00")!=NULL,"numeric: integral fraction and right alignment");
        CHECK(strstr(serial,"      3  LUMBERGH   DALLAS        7777.77")!=NULL,"numeric: LIST row 3");
        { const char *p=serial;int occurrences=0;
          while((p=strstr(p,"      2  WADDAMS    AUSTIN         -42.00"))!=NULL) { occurrences++;p++; }
          CHECK(occurrences==2,"numeric: LIST and DISPLAY both use the field formatter"); }
    }
    return TEST_SUMMARY(console?"test-samir-console-emu":status?"test-samir-status-emu":off?"test-samir-status-off-emu":"test-samir-numeric-format-emu");
}
