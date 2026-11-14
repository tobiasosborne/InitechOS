/* Test inputs only, independent of product headers/code.
 * Geometry: dbase3-decomp specs/file-formats/{dbf,dbt,ndx}.md, fixture-
 * verified III+ formats. Boundary values are probes, not expected behavior.
 * Regenerate into an EMPTY directory; compare with committed cases/. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
static void die(const char *s) { perror(s); exit(2); }
static void put(const char *p,const void *b,size_t n) {
    FILE *f=fopen(p,"wb"); if(!f)die(p);
    if(fwrite(b,1,n,f)!=n || fclose(f))die(p);
}
static void u16(unsigned char *p,unsigned n){p[0]=n;p[1]=n>>8;}
static void u32(unsigned char *p,unsigned n){u16(p,n);u16(p+2,n>>16);}
static unsigned char table[40000]; static size_t tablesz;
static void dbf(unsigned nf,const unsigned *width,const char *type,unsigned nr,int extra) {
    unsigned h=32+32*nf+1+extra,r=1,i,j,k,off;
    for(i=0;i<nf;i++)r+=width[i];
    tablesz=h+r*nr+1;if(tablesz>sizeof table)exit(2);
    memset(table,0,sizeof table);table[0]=3;table[1]=92;table[2]=3;table[3]=2;
    u32(table+4,nr);u16(table+8,h);u16(table+10,r);
    for(i=0;i<nf;i++) {
        unsigned char *d=table+32+i*32;char name[12];
        snprintf(name,sizeof name,"F%03u",i+1);
        memcpy(d,nf==2?(i?"VALUE":"KEY"):name,nf==2?(i?5:3):4);
        d[11]=type[i];d[16]=width[i];if(type[i]=='M')table[0]=0x83;
    }
    table[32+32*nf]=13;
    for(j=0;j<nr;j++) {
        off=h+j*r;table[off++]=' ';
        for(i=0;i<nf;i++) {
            memset(table+off,' ',width[i]);
            if(type[i]=='C')for(k=0;k<width[i];k++)table[off+k]=(unsigned char)('A'+j);
            if(type[i]=='N')table[off+width[i]-1]=(unsigned char)('1'+j);
            if(type[i]=='D')memcpy(table+off,"19920302",8);
            if(type[i]=='M')table[off+width[i]-1]='1';
            off+=width[i];
        }
    }
    table[tablesz-1]=26;
}
static void base(void){unsigned w[]={8,6};dbf(2,w,"CN",3,0);}
static void memo(size_t len,int terminators,unsigned ptr,unsigned next,int missing) {
    unsigned w[]={8,10};unsigned char b[8192];size_t n=512+len+(size_t)terminators;
    dbf(2,w,"CM",1,0);table[97+1+8+9]=(unsigned char)('0'+ptr);
    put("BASE.DBF",table,tablesz);
    if(missing)return;
    memset(b,0,sizeof b);u32(b,next);memset(b+512,'m',len);
    if(len>=4)memcpy(b+512,"memo",4);
    memset(b+512+len,26,(size_t)terminators);
    put("BASE.DBT",b,n);
}
static void ndx(unsigned n,size_t size) {
    unsigned char b[1024];unsigned i;
    memset(b,0,sizeof b);u32(b,1);u32(b+4,2);u16(b+12,8);
    u16(b+14,31);u16(b+18,16);memcpy(b+24,"KEY",3);u32(b+512,n);
    for(i=0;i<n;i++){u32(b+512+8+i*16,i+1);memset(b+512+12+i*16,'A'+i,8);}
    put("OLD.NDX",b,size);
}
static const char *names[]={
"intact","no-eof","partial-record","missing-record","short-header","zero-count-tail",
"low-count-tail","deleted-bottom","header-plus2","missing-dbt","missing-dbt-zero",
"empty-dbt","header-only-dbt","memo-no-term","memo-one-term","memo-cross-block",
"memo-2300","memo-bad-pointer","stale-key","stale-append","stale-recno","short-ndx",
"fields128","fields129","record4000","record4001","char254","date10",
"century-date","line254","line255","if24","do-depth","lines513","bytes8192","continued255"};
int main(int argc,char **argv){
    unsigned c,i,w[129];char types[130],dir[32],cmd[2048];FILE *p;
    if(argc!=2 || chdir(argv[1]))die("output directory");
    for(c=1;c<=36;c++) {
        snprintf(dir,sizeof dir,"C%02u",c);if(mkdir(dir,0755)||chdir(dir))die(dir);
        base();strcpy(cmd,"USE BASE\r\nLIST OFF\r\nGO BOTTOM\r\n? 'BOTTOM',RECNO(),KEY,VALUE\r\nREPLACE VALUE WITH 9\r\n");
        if(c==2)tablesz--;
        if(c==3)tablesz-=6;
        if(c==4)tablesz-=16;
        if(c==5)tablesz=50;
        if(c==6)u32(table+4,0);
        if(c==7)u32(table+4,2);
        if(c==8){table[97+2*15]='*';strcpy(cmd,"USE BASE\r\nSET DELETED ON\r\nLIST OFF\r\nGO BOTTOM\r\n? 'BOTTOM',RECNO(),KEY,VALUE\r\nREPLACE VALUE WITH 9\r\n");}
        if(c==9){unsigned a[]={8,6};dbf(2,a,"CN",3,1);}
        put("BASE.DBF",table,tablesz);
        if(c>=10&&c<=18){
            size_t len=c==16?511:c==17?2300:12;
            memo(len,c==14?0:c==15?1:2,c==11?0:c==18?9:1,c==16?3:c==17?6:2,c==10||c==11);
            if(c==12)put("BASE.DBT","",0);
            if(c==13){unsigned char b[512]={2};put("BASE.DBT",b,sizeof b);}
            strcpy(cmd,"USE BASE\r\nLIST OFF KEY\r\nGO TOP\r\n? 'LENGTH',LEN(VALUE)\r\n? 'MEMO'\r\n? VALUE\r\nGO BOTTOM\r\n? 'BOTTOM',RECNO()\r\nREPLACE KEY WITH 'SAVED'\r\n");
        }
        if(c>=19&&c<=22){
            if(c==19)memset(table+98,'Z',8);
            if(c==21){u32(table+4,2);tablesz=97+2*15+1;table[tablesz-1]=26;}
            put("BASE.DBF",table,tablesz);ndx(c==20?2:3,c==22?600:1024);
            strcpy(cmd,"USE BASE INDEX OLD\r\nLIST OFF\r\nGO BOTTOM\r\n? 'BEFORE',RECNO(),KEY\r\nSEEK 'AAAAAAAA'\r\n? 'SEEK',FOUND(),RECNO(),KEY\r\nREINDEX\r\nGO BOTTOM\r\n? 'AFTER',RECNO(),KEY\r\nREPLACE VALUE WITH 9\r\n");
        }
        if(c>=23&&c<=29){
            unsigned nf=1;
            for(i=0;i<129;i++){w[i]=1;types[i]='C';}
            if(c==23||c==24)nf=c==23?128:129;
            if(c==25||c==26){nf=16;for(i=0;i<15;i++)w[i]=254;w[15]=c==25?189:190;}
            if(c==27)w[0]=254;
            if(c==28||c==29){w[0]=c==28?10:8;types[0]='D';}
            dbf(nf,w,types,1,0);
            if(c==29)memcpy(table+66,"20000229",8);
            if(c==28)memcpy(table+66,"1992030200",10);
            put("BASE.DBF",table,tablesz);
            if(c<=24)strcpy(cmd,"USE BASE\r\n? 'FIELDS',FIELD(128),RECSIZE()\r\nLIST OFF F001,F128\r\nGO BOTTOM\r\nREPLACE F128 WITH 'Z'\r\n");
            else if(c<=26)strcpy(cmd,"USE BASE\r\n? 'SIZE',RECSIZE()\r\nLIST OFF F001,F016\r\nGO BOTTOM\r\nREPLACE F016 WITH 'SAVED'\r\n");
            else if(c==27)strcpy(cmd,"USE BASE\r\n? 'WIDTH',LEN(F001)\r\nLIST OFF F001\r\nGO BOTTOM\r\nREPLACE F001 WITH 'SAVED'\r\n");
            else strcpy(cmd,"USE BASE\r\nSET CENTURY ON\r\nLIST OFF F001\r\nGO BOTTOM\r\n? 'DATE',DTOC(F001),YEAR(F001)\r\nREPLACE F001 WITH CTOD('12/31/1992')\r\n");
        }
        if(c>=30){
            p=fopen("PROBE.PRG","wb");if(!p)die("PROBE.PRG");
            if(c==30||c==31){unsigned len=c==30?254:255;fputs("? LEN('",p);for(i=0;i<len-9;i++)fputc('x',p);fputs("')\r\n? 'PROBE-END'\r\n",p);}
            if(c==32){for(i=0;i<24;i++)fputs("IF .T.\r\n",p);fputs("? 'INNER'\r\n",p);for(i=0;i<24;i++)fputs("ENDIF\r\n",p);}
            if(c==33){fputs("D=D+1\r\nIF D<23\r\nDO PROBE\r\nENDIF\r\n? 'DEPTH',D\r\nRETURN\r\n",p);}
            if(c==34){for(i=0;i<512;i++)fputs("D=D+1\r\n",p);fputs("? 'LINE513'\r\n",p);}
            if(c==35){fputs("? 'START'\r\n",p);while(ftell(p)+16<=8175)fputs("*1234567890123\r\n",p);while(ftell(p)<8175)fputc(' ',p);fputs("\r\n? 'PROBE-END'\r\n",p);}
            if(c==36){fputs("? LEN( ;\r\n'",p);for(i=0;i<245;i++)fputc('x',p);fputs("')\r\n? 'PROBE-END'\r\n",p);}
            if(fclose(p))die("PROBE.PRG");
            strcpy(cmd,"USE BASE\r\nPUBLIC D\r\nD=0\r\nDO PROBE\r\n? 'RESUMED'\r\nGO BOTTOM\r\nREPLACE VALUE WITH 9\r\n");
        }
        p=fopen("MINT.PRG","wb");if(!p)die("MINT.PRG");
        fprintf(p,"SET TALK OFF\r\nSET SAFETY OFF\r\nSET STATUS OFF\r\nSET ALTERNATE TO RESULT.OUT\r\nSET ALTERNATE ON\r\nON ERROR ? 'ERR',ERROR(),MESSAGE()\r\n? 'BEGIN C%02u %s'\r\n",c,names[c-1]);
        /* Avoid III+'s interactive 'No database is in USE' file prompt after
         * a failed open. The failing USE itself is always recorded. */
        char *rest=strstr(cmd,"\r\n");if(!rest)exit(2);rest+=2;
        if(fwrite(cmd,1,(size_t)(rest-cmd),p)!=(size_t)(rest-cmd))die("MINT.PRG");
        fprintf(p,"IF LEN(DBF())>0\r\n%sENDIF\r\n? 'SAVE'\r\nUSE\r\nCOPY FILE BASE.DBF TO SAVED.DBF\r\n? 'END C%02u'\r\nSET ALTERNATE TO\r\nQUIT\r\n",rest,c);
        if(fclose(p))die("MINT.PRG");
        put("CONFIG.DB","STATUS=OFF\r\nBELL=OFF\r\n",22);
        if(chdir(".."))die("..");
    }
    return 0;
}
