/* Independent real III+ 1.1 mint/differential. No SAMIR engine/spec includes.
 * Law 1: corpus ndx.md ss1/3/5; Using III+ U7-5/U6-33; Programming P8-17.
 * Refuse absent reference files: no fallback to self-generated expectations.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "test_assert.h"
TEST_HARNESS();
static uint32_t le(const uint8_t *p,unsigned n)
{uint32_t v=0;while(n)v=(v<<8)|p[--n];return v;}
static void write_file(const char *name,const void *p,size_t n)
{FILE *f=fopen(name,"wb");CHECK(f!=NULL,name);if(f){CHECK(fwrite(p,1,n,f)==n,"mint fixture written");fclose(f);}}
static void prepare(void)
{
    uint8_t b[65+31*9];char cwd[4096],cfg[8192];unsigned i;
    const char *prg=
      "SET TALK OFF\r\nSET SAFETY OFF\r\nSET ALTERNATE TO MINT.TXT\r\nSET ALTERNATE ON\r\n"
      "ON ERROR ? 'ERR',ERROR()\r\n? 'PLUS'\r\n? LEN(SPACE(130)+SPACE(130))\r\n"
      "? 'MINUS'\r\n? LEN(SPACE(130)-SPACE(130))\r\n? 'FILES'\r\n"
      "? FILE('DBASE.EXE')\r\n? FILE('ABSENT.DBF')\r\n"
      "USE BASE\r\nINDEX ON KEY TO REF\r\nSET INDEX TO REF\r\n"
      "DO WHILE RECCOUNT()<95\r\nAPPEND BLANK\r\nREPLACE KEY WITH 'KKKKKKKK'\r\nENDDO\r\n"
      "USE MEMO\r\n? NOTE\r\nUSE\r\nSET ALTERNATE TO\r\nQUIT\r\n";
    memset(b,0,sizeof(b));b[0]=3;b[1]=86;b[2]=3;b[3]=14;b[4]=31;b[8]=65;b[10]=9;
    memcpy(b+32,"KEY",3);b[43]='C';b[48]=8;b[64]=13;
    for(i=65;i<sizeof(b);i+=9){b[i]=' ';memset(b+i+1,'K',8);}
    write_file("build/samir-core-mint/BASE.DBF",b,sizeof(b));
    write_file("build/samir-core-mint/MINT.PRG",prg,strlen(prg));
    CHECK(getcwd(cwd,sizeof(cwd))!=NULL,"factory working directory");
    snprintf(cfg,sizeof(cfg),"[sdl]\noutput=surface\n[dosbox]\nmachine=svga_s3\nmemsize=8\n[cpu]\ncore=auto\ncputype=386\ncycles=max\n[autoexec]\nmount c %s/build/samir-core-mint\nc:\nDBASE MINT\nexit\n",cwd);
    write_file("build/samir-core-mint/ref.conf",cfg,strlen(cfg));
    write_file("build/samir-core-mint/CONFIG.DB","STATUS=OFF\r\nBELL=OFF\r\n",22);
}
typedef struct {uint32_t rec;uint8_t key[8];} entry;
static int walk(FILE *f,uint32_t page,entry *out,unsigned *count,unsigned depth)
{
    uint8_t b[512];uint32_t n,i,child;
    if(depth>10 || !page || fseek(f,(long)page*512,SEEK_SET) || fread(b,1,512,f)!=512)return 0;
    n=le(b,2);if(n>31)return 0;
    for(i=0;i<n;i++) {
        child=le(b+4+i*16,4);
        if(child){if(!walk(f,child,out,count,depth+1))return 0;}
        else {if(*count>=95)return 0;out[*count].rec=le(b+8+i*16,4);memcpy(out[*count].key,b+12+i*16,8);(*count)++;}
    }
    if(n && le(b+4,4))return walk(f,le(b+4+n*16,4),out,count,depth+1);
    return 1;
}
static int load(const char *name,entry *out)
{
    uint8_t hdr[512];FILE *f=fopen(name,"rb");unsigned count=0;int ok=0;
    CHECK(f!=NULL,name);if(!f)return 0;
    if(fread(hdr,1,512,f)==512 && le(hdr+12,2)==8 && le(hdr+18,2)==16)
        ok=walk(f,le(hdr,4),out,&count,0);
    fclose(f);CHECK(ok && count==95,"independent NDX reader collected 95 entries");return ok && count==95;
}
static void check(void)
{
    entry a[95],b[95];unsigned i;char log[8192];FILE *f;size_t n;
    int ar=load("build/core.ndx",a),br=load("build/samir-core-mint/REF.NDX",b);
    if(ar&&br)for(i=0;i<95;i++) CHECK(a[i].rec==b[i].rec && !memcmp(a[i].key,b[i].key,8),
                      "same-key append/split differential vs real III+ (key,recno)");
    f=fopen("build/samir-core-mint/MINT.TXT","rb");
    CHECK(f!=NULL,"missing MINT.TXT: run test-samir-core-diff to mint real III+");
    if(f){n=fread(log,1,sizeof(log)-1,f);fclose(f);log[n]=0;
        CHECK(strstr(log,"ERR         77")!=NULL,"real III+ plus overflow code 77");
        CHECK(strstr(log,"ERR         76")!=NULL,"real III+ minus overflow code 76");
        CHECK(strstr(log,".T.") && strstr(log,".F."),"real FILE before USE");
        CHECK(strstr(log,"hello")!=NULL,"real III+ reads SAMIR's memo append and pointer");}
}
int main(int argc,char **argv)
{
    if(argc<2 || chdir(argv[1])) { perror("reference private root");return 2; }
    if(argc==3 && !strcmp(argv[2],"prepare"))prepare();else check();
    return TEST_SUMMARY("samir-core-real-diff");
}
