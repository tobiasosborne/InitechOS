/* Audit L001/L003/L007 target gate. Real FAT disk judged by mtools/fsck.fat
 * and dbf_ref.py, never by SAMIR alone. Ref: Using III+ U7-7/U5-284.
 * Full-disk refusal atomicity and scripted EOF abort: authored, no local reference.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "test_assert.h"
TEST_HARNESS();
static char serial[262144],raw[262144];
static int run(const char *cmd){int r=system(cmd);CHECK(r==0,cmd);return r;}
static void file(const char *path,const char *text){FILE *f=fopen(path,"wb");CHECK(f!=NULL,"fixture open");if(f){CHECK(fwrite(text,1,strlen(text),f)==strlen(text),"fixture written");fclose(f);}}
static size_t readfile(const char *path,char *b,size_t cap){FILE *f=fopen(path,"rb");size_t n=0;CHECK(f!=NULL,"extracted file readable");if(f){n=fread(b,1,cap-1,f);fclose(f);}b[n]=0;return n;}
static unsigned marker_count(const char *mark){
    const char *p=strstr(serial,mark);
    CHECK(p!=NULL,mark);if(!p)return 99999;
    p+=strlen(mark);while(*p=='.' || *p==' ' || *p=='\n')p++;
    return (unsigned)strtoul(p,NULL,10);
}
static unsigned freebytes(void){
    char *p,*q;unsigned n=0;
    run("mdir -i build/dbsafe/emu.img :: > build/dbsafe/free.log");readfile("build/dbsafe/free.log",raw,sizeof raw);
    p=strstr(raw,"bytes free");CHECK(p!=NULL,"mtools free-space judgment");if(!p)return 0;
    q=p;while(q>raw && q[-1]!='\n')q--;
    for(;q<p;q++)if(*q>='0' && *q<='9')n=n*10+(unsigned)(*q-'0');
    return n;
}
static void fill(void){
    unsigned n=freebytes(),i;FILE *f=fopen("build/dbsafe/FILLER.BIN","wb");char block[512]={0};
    CHECK(f!=NULL && n>0,"filler uses mtools remaining free bytes");if(!f)return;
    for(i=0;i<n;i+=sizeof block)CHECK(fwrite(block,1,sizeof block,f)==sizeof block,"filler block written");
    fclose(f);
    run("mcopy -o -i build/dbsafe/emu.img build/dbsafe/FILLER.BIN ::FILLER.BIN");
    CHECK(freebytes()==0,"L001 FAT disk has zero free clusters (mtools)");
}
int main(int argc,char **argv){
    const char *mode=argc>1?argv[1]:"full",*com=argc>2?argv[2]:"build/SAMIR.COM";
    char cmd[16384],script[8192],out[4096];size_t n,i,j;FILE *f;
    int full=!strcmp(mode,"full"),life=!strcmp(mode,"lifetime"),z;
    run("mkdir -p build/dbsafe");f=fopen("build/dbsafe/emu.img","wb");CHECK(f!=NULL,"FAT image open");if(!f)return 2;
    CHECK(fseek(f,1474560-1,SEEK_SET)==0 && fputc(0,f)!=EOF,"FAT image sized");fclose(f);
    run("mformat -i build/dbsafe/emu.img -f 1440 ::");
    snprintf(cmd,sizeof cmd,"mcopy -o -i build/dbsafe/emu.img %s ::SAMIR.COM",com);if(run(cmd))return 2;
    run("mcopy -o -i build/dbsafe/emu.img build/CLIENTS.DBF ::CLIENTS.DBF");
    if(full){
        strcpy(script,"use clients\n");for(z=0;z<32;z++)strcat(script,"append blank\n");
        strcat(script,"? 'COMMITTED'\n? reccount()\nuse\nuse clients\n? 'REOPENED'\n? reccount()\nlist\nquit\n");
    }else if(life){
        run("mcopy -o -i build/dbsafe/emu.img build/CLIENTS.DBF ::OTHER.DBF");
        strcpy(script,"use clients\nselect 2\nuse clients\nselect 1\n? reccount()\nselect 2\nuse other\nselect 1\nuse\nselect 2\nlist\nselect 1\nuse clients\nclose databases\nselect 2\nuse other\nselect 1\nuse clients\nquit\n");
    }else strcpy(script,"use clients\nzap\nN\n? 'DECLINED'\n? reccount()\nlist\nzap\nY\n? 'ACCEPTED'\n? reccount()\nuse\nuse clients\n? reccount()\nquit\n");
    file("build/dbsafe/CASE.TXT",script);file("build/dbsafe/AFTER.TXT","use clients\n? 'AFTERQUIT'\n? reccount()\nquit\n");
    file("build/dbsafe/DRIVE.BAT","@echo off\r\nsamir < case.txt\r\nsamir < after.txt\r\n");
    run("mcopy -o -i build/dbsafe/emu.img build/dbsafe/CASE.TXT ::CASE.TXT");
    run("mcopy -o -i build/dbsafe/emu.img build/dbsafe/AFTER.TXT ::AFTER.TXT");
    run("mcopy -o -i build/dbsafe/emu.img build/dbsafe/DRIVE.BAT ::DRIVE.BAT");
    if(full)fill();
    run("fsck.fat -n build/dbsafe/emu.img > build/dbsafe/fsck-before.log");
    snprintf(cmd,sizeof cmd,"build/qemu_harness --disk build/tracer_boot.img --disk2 build/dbsafe/emu.img --name samir_dbsafe_%s --out build --timeout-ms 60000 --keys 'd,r,i,v,e,ret,e,x,i,t,ret' --keys-after SHELL-READY --quit-after SHELL-DONE > build/dbsafe/emu.stdout 2> build/dbsafe/emu.report",mode);
    run(cmd);snprintf(cmd,sizeof cmd,"build/samir_dbsafe_%s.serial",mode);n=readfile(cmd,raw,sizeof raw);
    for(i=0,j=0;i<n;i++)if(raw[i]!='\r')serial[j++]=raw[i];
    serial[j]=0;
    CHECK(strstr(serial,"SHELL-DONE")!=NULL,"L003 QUIT returns cleanly to desktop");
    CHECK(!strstr(serial,"PC LOAD LETTER"),"L003 no guest panic");
    CHECK(strstr(serial,"AFTERQUIT")!=NULL,"L003 another SAMIR launch succeeds after QUIT");
    CHECK(!strstr(serial,"Not a dBASE database."),"L001 table reopens in guest");
    if(life){CHECK(strstr(serial,"3  File is already open.")!=NULL,"L003 real catalog duplicate-open refusal");CHECK(strstr(serial,"PESTON") && strstr(serial,"WADDAMS") && strstr(serial,"LUMBERGH"),"L003 surviving area records readable");}
    if(full){CHECK(strstr(serial,"56  Disk full when writing file:")!=NULL,"B02 real disk-full catalog diagnostic");CHECK(!strstr(serial,"read-only"),"B02 disk full distinguished from read-only");}
    if(!full && !life){CHECK(strstr(serial,"ZAP CLIENTS.DBF? (Y/N)")!=NULL,"L007 manual ZAP confirmation in guest");CHECK(marker_count("DECLINED")==3,"L007 N preserves all records");CHECK(marker_count("ACCEPTED")==0,"L007 Y commits empty table");}
    run("fsck.fat -n build/dbsafe/emu.img > build/dbsafe/fsck-after.log");
    run("mcopy -o -i build/dbsafe/emu.img ::CLIENTS.DBF build/dbsafe/EXTRACT.DBF");
    run("python3 harness/diff/dbf_diff/dbf_ref.py --schema build/dbsafe/EXTRACT.DBF > build/dbsafe/emu-schema.txt");
    run("python3 harness/diff/dbf_diff/dbf_ref.py --records build/dbsafe/EXTRACT.DBF > build/dbsafe/emu-records.txt");
    n=readfile("build/dbsafe/EXTRACT.DBF",raw,sizeof raw);
    { unsigned char *b=(unsigned char *)raw;unsigned count=b[4]+256u*b[5]+65536u*b[6]+16777216u*b[7];
      snprintf(out,sizeof out,"REOPENED\n%u\n",count);if(full)CHECK(marker_count("REOPENED")==count,"L001 guest reopen count agrees with independent disk bytes");
      snprintf(out,sizeof out,"COMMITTED\n%u\n",count);if(full)CHECK(marker_count("COMMITTED")==count,"L001 refused appends preserve in-memory count");
      if(!full && !life)CHECK(count==0,"L007 accepted ZAP persisted");
      if(life)CHECK(count==3,"L003 close leaves table intact");
      if(full){
        size_t seedn=readfile("build/CLIENTS.DBF",out,sizeof out);unsigned h=(unsigned char)out[8]+256u*(unsigned char)out[9];unsigned r=(unsigned char)out[10]+256u*(unsigned char)out[11];
        unsigned expected=(512u*(((unsigned)seedn+511u)/512u)-h-1u)/r;
        CHECK(count==expected,"L001 count equals records fitting original allocated cluster (mtools zero free)");
        CHECK(n>=(size_t)h+count*r,"L001 committed header fits actual file length");
        CHECK(!memcmp(raw+8,out+8,h+3u*r-8u),"L001 original header geometry and three record bytes preserved");
      }
    }
    return TEST_SUMMARY(mode);
}
