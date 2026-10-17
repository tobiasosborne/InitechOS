/* QEMU injected trace for SAMIR core. Using III+ U1-2/U6-33/U7-5;
 * CORE.PRG is authored from those rules. mtools/fsck judge identical images.
 * Same serial + real EXEC pattern as test_samir_audit_emu.c and m0dc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_assert.h"
TEST_HARNESS();
static int command(const char *s){int rc=system(s);CHECK(rc==0,s);return rc;}
static void keys(const char *text,char *out)
{
    *out=0;
    while(*text){const char *t;char one[2]={*text,0};
        switch(*text){case '\n':t="ret";break;case ' ':t="spc";break;
        case '?':t="shift-slash";break;case '(':t="shift-9";break;
        case ')':t="shift-0";break;case '+':t="shift-equal";break;
        case '-':t="minus";break;default:t=one;break;}
        if(*out)strcat(out,",");
        strcat(out,t);text++;
    }
}
int main(int argc,char **argv)
{
    const char *com=argc>1?argv[1]:"build/SAMIR.COM";
    char cmd[4096],trace[2048],serial[262144];FILE *f;size_t n;
    const char *prg=
      "SET TALK OFF\r\nSET STATUS OFF\r\n"
      "IF FILE('CORE.PRG')\r\n? 'COREFILE'\r\nENDIF\r\n"
      "STORE 0 TO I\r\nSTORE 'keep' TO K\r\n"
      "DO WHILE I<400\r\nSTORE SPACE(200) TO B\r\nSTORE B TO C\r\n"
      "RELEASE C\r\nSTORE 1 TO B\r\nI=I+1\r\nENDDO\r\n"
      "IF I=400 .AND. K='keep'\r\n? 'COREMEM'\r\nENDIF\r\n";
    command("mkdir -p build/samir-core-emu");
    f=fopen("build/samir-core-emu/emu.img","wb");CHECK(f!=NULL,"FAT allocated");
    if(!f)return 1;
    CHECK(fseek(f,1474560-1,SEEK_SET)==0 && fputc(0,f)!=EOF,"FAT sized");fclose(f);
    command("mformat -i build/samir-core-emu/emu.img -f 1440 ::");
    snprintf(cmd,sizeof(cmd),"mcopy -o -i build/samir-core-emu/emu.img %s ::SAMIR.COM",com);
    if(command(cmd))return 1;
    f=fopen("build/samir-core-emu/CORE.PRG","wb");CHECK(f!=NULL,"PRG allocated");
    if(!f)return 1;
    CHECK(fwrite(prg,1,strlen(prg),f)==strlen(prg),"PRG written");fclose(f);
    command("mcopy -o -i build/samir-core-emu/emu.img build/samir-core-emu/CORE.PRG ::CORE.PRG");
    command("fsck.fat -n build/samir-core-emu/emu.img > build/samir-core-emu/fsck-before.log");
    keys("samir\n? space(130)+space(130)\n? space(130)-space(130)\ndo core\nquit\nexit\n",trace);
    snprintf(cmd,sizeof(cmd),"build/qemu_harness --disk build/tracer_secondary.img --disk2 build/samir-core-emu/emu.img --name samir_core --out build --timeout-ms 60000 --keys '%s' --keys-after SHELL-READY --quit-after SHELL-DONE > build/samir-core-emu/stdout.log 2> build/samir-core-emu/report.log",trace);
    command(cmd);
    f=fopen("build/samir_core.serial","rb");CHECK(f!=NULL,"serial captured");if(!f)return 1;
    n=fread(serial,1,sizeof(serial)-1,f);fclose(f);serial[n]=0;
    CHECK(strstr(serial,"SHELL-DONE")!=NULL,"guest shell returned");
    CHECK(strstr(serial,"PC LOAD LETTER")==NULL,"guest did not panic");
    CHECK(strstr(serial,"77  + : Concatenated string too large.\n")!=NULL,"CONCAT_PLUS: guest exact manual error 77");
    CHECK(strstr(serial,"76  - : Concatenated string too large.\n")!=NULL,"CONCAT_MINUS: guest exact manual error 76");
    CHECK(strstr(serial,"COREFILE")!=NULL,"FILE_NEEDS_USE: guest FILE before USE");
    CHECK(strstr(serial,"COREMEM")!=NULL,"MEMVAR_RECLAIM: guest completed 400 updates");
    command("fsck.fat -n build/samir-core-emu/emu.img > build/samir-core-emu/fsck-after.log");
    return TEST_SUMMARY("samir-core-emu");
}
