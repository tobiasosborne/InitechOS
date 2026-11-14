/* Compatibility inputs only. No expected result is calculated here. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
static FILE *file(const char *d,const char *n) {
    char p[1024]; snprintf(p,sizeof p,"%s/%s",d,n);
    FILE *f=fopen(p,"wb"); if(!f){perror(p);exit(2);} return f;
}
static void line(FILE *f,const char *s){fprintf(f,"%s\r\n",s);}
static void padded(FILE *f,const char *prefix,int n,const char *suffix) {
    fputs(prefix,f);for(int i=(int)strlen(prefix)+(int)strlen(suffix);i<n;i++)fputc('x',f);
    fprintf(f,"%s\r\n",suffix);
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    const char *d=argv[1]; FILE *f;
    f=file(d,"BATCH.BAT");
    line(f,"@echo off");line(f,"echo PARAMS=[%0][%1][%2][%9]");line(f,"shift");
    line(f,"echo SHIFT=[%0][%1][%2][%9]");line(f,"set ABSENT=");
    line(f,"echo MISSING=[%ABSENT%]");line(f,"echo PERCENT=[%%][trailing%]");
    line(f,"if x==X echo BAD-CASE");line(f,"if not x==X echo CASE-OK");
    line(f,"if exist *.TXT echo WILDCARD-EXISTS");
    line(f,"if exist SUB echo DIRECTORY-EXISTS");
    line(f,"for %%f in (one,two;three=four) do echo TOKEN=[%%f]");
    line(f,"for %%f in (NO*.ZZZ) do echo UNMATCHED=[%%f]");
    line(f,"for %%f in (A.TXT B.TXT) do echo FILE=[%%f]");
    line(f,"for %%f in (*.TXT literal) do echo WILD=[%%f]");
    line(f,"if nonsense echo BAD-IF");line(f,"for broken");
    line(f,"call CHILD.BAT");line(f,"echo AFTER-CALL");
    line(f,"command /c CHILD.BAT");line(f,"echo AFTER-COMMAND");
    line(f,"goto :COLON");line(f,"echo BAD-COLON");line(f,":COLON");
    line(f,"echo COLON-OK");line(f,"goto missing");line(f,"echo BAD-GOTO");fclose(f);
    f=file(d,"CHILD.BAT");line(f,"@echo CHILD-RAN");fclose(f);
    f=file(d,"LIMIT.BAT");line(f,"@echo off");
    for(int n=125;n<=132;n++){fprintf(f,"echo LENGTH-%d\r\n",n);padded(f,"echo ",n,"");}
    padded(f,"rem ",512,"");line(f,"echo AFTER-LONG-REM");
    padded(f,"set V=",86,"");line(f,"echo LIMIT-DONE");fclose(f);
    f=file(d,"EXPAND.BAT");line(f,"@echo off");padded(f,"set V=",86,"");line(f,"echo EXPAND-BEGIN");line(f,"echo %V%%V%");
    line(f,"echo EXPAND-END");line(f,"echo EXPAND-DONE");fclose(f);
    f=file(d,"PARAMS.BAT");line(f,"@echo off");line(f,"echo PARAM-BEGIN");
    line(f,"command /c PARAM.BAT abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz");
    line(f,"echo LIMIT-DONE");fclose(f);
    f=file(d,"PARAM.BAT");line(f,"@echo off");line(f,"echo %0-%1-%2-%3-%1-%2-%3");line(f,"shift");line(f,"echo %0-%1-%2-%3");fclose(f);
    f=file(d,"ENV.BAT");line(f,"@echo off");
    for(int n=0;n<12;n++){char p[20];snprintf(p,sizeof p,"set V%02d=",n);padded(f,p,86,"");}
    line(f,"echo ENV-BEGIN");line(f,"set");line(f,"echo ENV-END");line(f,"set V00=");line(f,"set NEW=ok");line(f,"echo NEW=[%NEW%]");fclose(f);
    f=file(d,"SWITCH.BAT");line(f,"@echo off");
    const char *sw[]={"echo DIR-W","dir/w A.TXT","echo DIR-Z","dir /z","echo COPY-V","copy A.TXT CP.TXT/V","echo COPY-Z","copy A.TXT BAD.TXT/Z","echo DEL-Z","copy A.TXT DEL.TXT","del DEL.TXT /z","echo REN-Z","ren B.TXT R.TXT /z","echo CD-Z","cd /z","echo SWITCH-DONE",NULL};
    for(int i=0;sw[i];i++)line(f,sw[i]);
    fclose(f);
    f=file(d,"NAMES.BAT");line(f,"@echo off");
    const char *names[]={"echo LONG","copy A.TXT LONGFILENAME.LONG","echo DEVICE","echo discarded>NUL.TXT","echo discarded>SUB\\NUL.TXT","echo discarded>SUB\\..\\NUL","echo ROOT","cd SUB","cd .","cd","cd ..","cd","cd..","cd","echo WILDCARD","dir A?.TXT","echo DRIVE","z:","echo NAMES-DONE",NULL};
    for(int i=0;names[i];i++)line(f,names[i]);
    fclose(f);
    f=file(d,"REDIR.BAT");line(f,"@echo off");
    const char *re[]={"echo abc>R.OUT","echo def>>R.OUT","type<R.OUT","badname>BAD.OUT","echo BEFORE-PIPE","echo hello|echo consumer","echo AFTER-PIPE","echo do-not-clobber>PIPE1.$$$","echo hello|dir >PIPES.OUT","type PIPE1.$$$","set PP=parent","set PP=child|rem","echo PIPE-ENV=[%PP%]","echo REDIR-DONE",NULL};
    for(int i=0;re[i];i++)line(f,re[i]);
    fclose(f);
    f=file(d,"FILTER.BAT");line(f,"@echo off");
    const char *filters[]={"sort < NOEND.TXT >SNO.OUT","find \"needle\" < NOEND.TXT >FNO.OUT","more < NOEND.TXT >MNO.OUT","find \"needle\" NOEND.TXT < A.TXT >FNAM.OUT","find /z \"needle\" < NOEND.TXT >FZ.OUT","sort /z < NOEND.TXT >SZ.OUT","more /z < NOEND.TXT >MZ.OUT","sort < BIG.TXT >SBIG.OUT","find \"needle\" < BIG.TXT >FBIG.OUT","find \"needle\" < LONG.TXT >FLONG.OUT","sort < LONG.TXT >SLONG.OUT","echo FILTER-DONE",NULL};
    for(int i=0;filters[i];i++)line(f,filters[i]);
    fclose(f);
    f=file(d,"MORECASE.BAT");line(f,"@echo off");line(f,"more < BIG.TXT >MBIG.OUT");line(f,"more < LONG.TXT >MLONG.OUT");line(f,"echo MORE-DONE");fclose(f);
    f=file(d,"NOEND.TXT");fputs("z needle\r\na needle",f);fclose(f);
    f=file(d,"A.TXT");line(f,"alpha");fclose(f);f=file(d,"B.TXT");line(f,"beta");fclose(f);
    f=file(d,"LONG.TXT");for(int i=0;i<600;i++)fputc('x',f);fputs("needle\r\nlast needle",f);fclose(f);
    f=file(d,"BIG.TXT");for(int i=3000;i>0;i--)fprintf(f,"%05d needle abcdefghijklmnopqrstuvwxyz\r\n",i);fclose(f);
    f=file(d,"FULL.BAT");line(f,"@echo off");line(f,"type BIG.TXT|echo CONSUMER-RAN");line(f,"echo FULL-DONE");fclose(f);
    f=file(d,"TCASE.BAT");line(f,"@echo off");for(int n=126;n<=128;n++){fprintf(f,"echo TAIL-%d\r\n",n);padded(f,"TAIL.COM ",n+8,"");}line(f,"echo TAIL-DONE");fclose(f);
    f=file(d,"STREAM.BAT");line(f,"@echo off");line(f,"PASS <A.TXT >IN.OUT");line(f,"TYPE A.TXT|PASS >PIPE.OUT");line(f,"echo STREAM-DONE");fclose(f);
    f=file(d,"DEVICES.BAT");line(f,"@echo off");line(f,"DEVPROBE.COM");line(f,"echo DEVICES-DONE");fclose(f);
    f=file(d,"CFG1.CFG");
    const char *cfg[]={"FILES=banana","FILES=-1","FILES=0","FILES=256","FILES=12junk","BUFFERS=banana","BUFFERS=-1","BUFFERS=0","BUFFERS=100","BUFFERS=12junk","LASTDRIVE=9","LASTDRIVE=Zjunk","BREAK=MAYBE","DEVICE=MISSING.SYS /Z","INSTALL=MISSING.EXE /Z","BOGUS=1","SHELL=COMMAND.COM /P /E:512",NULL};
    for(int i=0;cfg[i];i++)line(f,cfg[i]);
    fclose(f);
    f=file(d,"CFG2.CFG");line(f,"SHELL=COMMAND.COM /P /E:1024");fclose(f);
    f=file(d,"CFG3.CFG");line(f,"SHELL=MISSING.COM /P");fclose(f);
    f=file(d,"CFG4.CFG");line(f,"DEVICE=ANSI.SYS /Z");line(f,"BREAK=OFF");line(f,"FILES=20");line(f,"BUFFERS=20");line(f,"LASTDRIVE=Z");fclose(f);
    const char *badcfg[]={"FILES=banana","BUFFERS=-1","LASTDRIVE=9","BREAK=MAYBE","DEVICE=MISSING.SYS /Z","INSTALL=MISSING.EXE /Z"};
    for(unsigned i=0;i<sizeof badcfg/sizeof *badcfg;i++) {
        char n[16];snprintf(n,sizeof n,"CFG%c.CFG",'A'+(int)i);
        f=file(d,n);line(f,badcfg[i]);fclose(f);
    }
    const char *utils[]={"MORE","FIND","SORT","CHKDSK","EDLIN","FDISK","FORMAT","SYS","DISKCOPY","DISKCOMP","XCOPY","TREE","ATTRIB","LABEL","TPSREP"};
    for(unsigned i=0;i<sizeof utils/sizeof *utils;i++) {
        char n[16];snprintf(n,sizeof n,"U%02u.BAT",i+1);
        f=file(d,n);line(f,"@echo off");
        fprintf(f,"%s /Z <NUL >UTIL.OUT\r\n",utils[i]);
        line(f,"echo UTIL-DONE");fclose(f);
    }
    return 0;
}
