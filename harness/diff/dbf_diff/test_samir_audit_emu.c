/* Audit K05/K06/K09 and K13, actual OS EXEC of SAMIR.COM.
 * MANUAL-GROUNDED (not a historical output differential): Using III+
 * U1-4, U2-26, U5-215, U5-229; Programming P10-17/18. K13 is authored,
 * no local reference to historical redirected dBASE; PAL conin_line contract.
 * Replays audit samir-filter/state and dos-extra command sequences. mtools
 * and fsck.fat judge the disk; no SAMIR decoder supplies the expected bytes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "test_assert.h"
TEST_HARNESS();
static char serial[262144], fragment[65536];
static int command(const char *s)
{
    int rc=system(s);
    CHECK(rc==0,s);
    return rc;
}
static void write_text(const char *path,const char *s)
{
    FILE *f=fopen(path,"wb");
    CHECK(f!=NULL,"factory fixture opened");
    if(f){CHECK(fwrite(s,1,strlen(s),f)==strlen(s),"factory fixture written");fclose(f);}
}
static size_t read_file(const char *path,char *out,size_t cap)
{
    FILE *f=fopen(path,"rb");size_t n=0;
    CHECK(f!=NULL,"independent file extraction opened");
    if(f){n=fread(out,1,cap-1,f);fclose(f);}out[n]=0;return n;
}
static const char *between(const char *first,const char *last)
{
    const char *p=strstr(serial,first), *q;
    CHECK(p!=NULL,first);
    if(!p) return "";
    p+=strlen(first);q=strstr(p,last);
    CHECK(q!=NULL,last);
    if(!q || q-p>=(int)sizeof(fragment)) return "";
    memcpy(fragment,p,(size_t)(q-p));fragment[q-p]=0;return fragment;
}
static void keys(const char *text,char *out,size_t cap)
{
    size_t n=0;
    while(*text){
        const char *t=NULL;char one[2]={*text,0};
        if(*text=='\n') t="ret";
        else if(*text==' ') t="spc";
        else if(*text=='.') t="dot";
        else if(*text=='>') t="shift-dot";
        else if(*text=='-') t="minus";
        else if((*text>='a' && *text<='z') || (*text>='0' && *text<='9')) t=one;
        CHECK(t!=NULL,"key script character supported");
        if(!t) exit(2);
        if(n) out[n++]=',';
        CHECK(n+strlen(t)+1<cap,"key script fits");
        strcpy(out+n,t);n+=strlen(t);text++;
    }
    out[n]=0;
}
int main(int argc,char **argv)
{
    const char *mode=argc>1?argv[1]:"view";
    const char *com=argc>2?argv[2]:"build/SAMIR.COM";
    char cmd[16384],keybuf[8192],raw[262144];
    FILE *f;size_t n,i,j;
    int input=strcmp(mode,"input")==0;
    command("mkdir -p build/samir-audit");
    f=fopen("build/samir-audit/emu.img","wb");
    CHECK(f!=NULL,"FAT image allocated");
    if(!f) return 2;
    CHECK(fseek(f,1474560-1,SEEK_SET)==0 && fputc(0,f)!=EOF,"FAT image sized");fclose(f);
    command("mformat -i build/samir-audit/emu.img -f 1440 ::");
    snprintf(cmd,sizeof(cmd),"mcopy -o -i build/samir-audit/emu.img %s ::SAMIR.COM",com);
    if(command(cmd)) return 2;
    command("mcopy -o -i build/samir-audit/emu.img build/CLIENTS.DBF ::CLIENTS.DBF");
    if(input){
        write_text("build/samir-audit/INPUT.BAT",
          "@echo off\r\necho use clients.dbf > input.txt\r\necho list >> input.txt\r\necho quit >> input.txt\r\n"
          "samir < input.txt > dbout.txt\r\ntype dbout.txt\r\n"
          "samir < input.txt > dbout2.txt\r\ntype dbout2.txt\r\n");
        command("mcopy -o -i build/samir-audit/emu.img build/samir-audit/INPUT.BAT ::INPUT.BAT");
        keys("input\nsamir\nuse clients.dbf\nlist\nquit\nexit\n",keybuf,sizeof(keybuf));
    }else{
        /* The audit's own LIST FOR control, SET FILTER+GO TOP+LIST, and
         * GO TOP+DELETE+DISPLAY before PACK. Also grade ON/OFF and missing USE. */
        keys("samir\nuse clients\nuse clients.dbf\nlist for bal > 1000\nset filter to bal > 1000\ngo top\nlist\nset filter to bal > 7000\ngo top\nskip -1\ndisplay\ngo 2\nset filter to\ngo top\ndelete\ndisplay\nlist\nset deleted on\ngo top\nlist\nset deleted off\ngo top\nrecall\nlist\nuse absent\nquit\nexit\n",keybuf,sizeof(keybuf));
    }
    command("fsck.fat -n build/samir-audit/emu.img > build/samir-audit/fsck-before.log");
    snprintf(cmd,sizeof(cmd),"build/qemu_harness --disk build/tracer_boot.img --disk2 build/samir-audit/emu.img --name samir_audit_%s --out build --timeout-ms 60000 --keys '%s' --keys-after SHELL-READY --quit-after SHELL-DONE > build/samir-audit/emu.stdout 2> build/samir-audit/emu.report",mode,keybuf);
    command(cmd);
    snprintf(cmd,sizeof(cmd),"build/samir_audit_%s.serial",mode);
    n=read_file(cmd,raw,sizeof(raw));
    for(i=0,j=0;i<n;i++) if(raw[i]!='\r') serial[j++]=raw[i];
    serial[j]=0;
    CHECK(strstr(serial,"SHELL-READY")!=NULL,"OS shell booted");
    CHECK(strstr(serial,"SHELL-DONE")!=NULL,"SAMIR quit and shell returned cleanly");
    CHECK(strstr(serial,"PC LOAD LETTER")==NULL,"no guest panic");
    if(input){
        char a[8192],b[8192],in[8192];size_t an,bn;
        command("mcopy -o -i build/samir-audit/emu.img ::INPUT.TXT build/samir-audit/input-extracted.txt");
        read_file("build/samir-audit/input-extracted.txt",in,sizeof(in));
        CHECK_STR_EQ(in,"use clients.dbf\r\nlist\r\nquit\r\n","K13 audit ECHO created three CRLF commands");
        command("mcopy -o -i build/samir-audit/emu.img ::DBOUT.TXT build/samir-audit/dbout.txt");
        command("mcopy -o -i build/samir-audit/emu.img ::DBOUT2.TXT build/samir-audit/dbout2.txt");
        an=read_file("build/samir-audit/dbout.txt",a,sizeof(a));bn=read_file("build/samir-audit/dbout2.txt",b,sizeof(b));
        CHECK(an==bn && !memcmp(a,b,an),"K13 repeated redirection byte-identical");
        CHECK(strstr(a,"PESTON") && strstr(a,"WADDAMS") && strstr(a,"LUMBERGH"),"K13 USE/LIST/QUIT rendered all three rows");
        CHECK(!strstr(a,"Unrecognized") && !strstr(a,"Not a dBASE"),"K13 no combined-command error");
        CHECK(strstr(serial,"PESTON") && strstr(serial,"WADDAMS") && strstr(serial,"LUMBERGH"),"K13 interactive control renders same rows");
    }else{
        const char *p=between(". list for bal > 1000\n",". set filter");
        CHECK(strstr(p,"PESTON") && strstr(p,"LUMBERGH") && !strstr(p,"WADDAMS"),"K05 LIST FOR audit control");
        p=between(". set filter to bal > 1000\n. go top\n. list\n",". go 2");
        CHECK(strstr(p,"PESTON") && strstr(p,"LUMBERGH") && !strstr(p,"WADDAMS"),"K05 active FILTER excludes negative row");
        /* Authored visible-TOP clamp; manual BOF is not EOF (Using U6-13/27). */
        p=between(". set filter to bal > 7000\n. go top\n. skip -1\n. display\n",". go 2");
        CHECK(strstr(p,"LUMBERGH") && !strstr(p,"PESTON"),"K05 filtered backward boundary keeps visible data, not virtual EOF");
        p=between(". delete\n. display\n",". list");
        CHECK(strstr(p,"PESTON") && strchr(p,'*'),"K06 audit DELETE+DISPLAY has asterisk");
        p=between(". set deleted on\n. go top\n. list\n",". set deleted off");
        CHECK(!strstr(p,"PESTON") && strstr(p,"WADDAMS") && strstr(p,"LUMBERGH"),"K06 ON hides deleted row");
        p=between(". use clients\n",". use clients.dbf");
        CHECK(!strstr(p,"File does not exist.") && !strstr(p,"Not a dBASE database."),"K09 implicit extension succeeded before explicit control");
        CHECK(!strstr(serial,"Not a dBASE database."),"K09 implicit USE opens supplied table");
        CHECK(strstr(serial,"1  File does not exist."),"K09 missing table is #1");
        CHECK(!strstr(serial,"File already exists."),"K06 SET DELETED accepted");
    }
    command("fsck.fat -n build/samir-audit/emu.img > build/samir-audit/fsck-after.log");
    return TEST_SUMMARY(input?"test-samir-input-emu":"test-samir-audit-emu");
}
