/* Deliberate invariant-fault traces: complete at the kernel panic, never a
 * timeout, missing output, failed launch, or partial injection (F2 / Rule 6).
 * Expected diagnostics are literals independent of the manager implementation.
 * Ref: docs/design/samir-overlays.md Sec 6, PRD Appendix B / Rule 2. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_assert.h"
#include "samir_install.h"
#include "trace_complete.h"
#include "scratch_lock.h"
TEST_HARNESS();
static int command(const char *s) { int r=system(s);CHECK(r==0,s);return r; }
int main(int argc,char **argv)
{
    char d[1024],img[1200],cmd[8192],ins[4096],path[1400],serial[262144];
    FILE *f;size_t n;
    if(argc!=4)return 2;
    scratch_lock("build/.lock-samir-ovl-guard");
    snprintf(d,sizeof d,"build/samir-ovl-guard/%s",argv[1]);
    snprintf(cmd,sizeof cmd,"mkdir -p %s",d);if(command(cmd))return 2;
    snprintf(img,sizeof img,"%s/data.img",d);f=fopen(img,"wb");if(!f)return 2;
    CHECK(fseek(f,1474560-1,SEEK_SET)==0 && fputc(0,f)!=EOF,"disk sized");fclose(f);
    snprintf(cmd,sizeof cmd,"mformat -i %s -f 1440 ::",img);if(command(cmd))return 2;
    if(samir_install_cmd(ins,sizeof ins,img,argv[2]) || command(ins))return 2;
    snprintf(path,sizeof path,"%s/PROBE.PRG",d);f=fopen(path,"wb");if(!f)return 2;
    fputs("ON ERROR LIST STATUS\r\nSET TALK OFF\r\n? 'UNSAFE-RETURN'\r\n",f);fclose(f);
    snprintf(cmd,sizeof cmd,"mcopy -o -i %s %s ::PROBE.PRG",img,path);if(command(cmd))return 2;
    snprintf(path,sizeof path,"%s/PROBE.IN",d);f=fopen(path,"wb");if(!f)return 2;
    fputs("DO PROBE\r\nQUIT\r\n",f);fclose(f);
    snprintf(cmd,sizeof cmd,"mcopy -o -i %s %s ::PROBE.IN",img,path);if(command(cmd))return 2;
    snprintf(path,sizeof path,"%s/PROBE.BAT",d);f=fopen(path,"wb");if(!f)return 2;
    fputs("@echo off\r\nsamir < probe.in\r\n",f);fclose(f);
    snprintf(cmd,sizeof cmd,"mcopy -o -i %s %s ::PROBE.BAT",img,path);if(command(cmd))return 2;
    snprintf(cmd,sizeof cmd,"INITECH_QMP_STDIO=1 build/qemu_harness --disk build/tracer_secondary.img --disk2 %s --name probe --out %s --timeout-ms 60000 --keys 'p,r,o,b,e,ret' --keys-after SHELL-READY --quit-after HALTED > %s/out 2> %s/report",img,d,d,d);
    if(command(cmd))return TEST_SUMMARY("samir-ovl-guard");
    snprintf(path,sizeof path,"%s/report",d);snprintf(img,sizeof img,"%s/probe.serial",d);
    CHECK(emu_trace_complete(path,img,6),"complete deliberate-panic trace (all six keys)");
    f=fopen(img,"rb");if(!f)return 2;n=fread(serial,1,sizeof serial-1,f);fclose(f);serial[n]=0;
    CHECK(strstr(serial,"SAMIR OVERLAY PANIC: ")!=NULL,"overlay diagnostic");
    CHECK(strstr(serial,argv[3])!=NULL,"specific violated invariant diagnosed");
    { const char *context=!strcmp(argv[1],"LIVE")
        ? "loaded=00000003 requested=00000002 depth=00000001"
        : !strcmp(argv[1],"NO_RELOC")
        ? "loaded=FFFFFFFF requested=00000003 depth=00000000"
        : "loaded=00000003 requested=00000003 depth=00000001";
      CHECK(strstr(serial,context)!=NULL,"loaded/requested overlay and exact activation depth reported"); }
    CHECK(strstr(serial,"PANIC vec=06")!=NULL && strstr(serial,"HALTED")!=NULL,"real kernel panic rendered");
    CHECK(strstr(serial,"UNSAFE-RETURN")==NULL,"escaped/live overlay did not return");
    return TEST_SUMMARY("samir-ovl-guard");
}
