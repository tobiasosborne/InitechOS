/* Factory-only extra III+ probes: label comma spacing, block copies, pubs ?.
 * Input generation only; expected bytes come solely from DBASE.EXE. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int main(int argc,char **argv)
{
    const char *expr[]={"'A','B'","'A',' ','B'","FIRSTNAME,'X',LASTNAME","CITY,\", \",STATE,\" \",ZIPCODE","FIRSTNAME,LASTNAME"};
    const int sizes[]={0,1,511,512,513,1000};
    unsigned char lbl[1034];char name[32];FILE *f;
    if(argc!=2 || chdir(argv[1]))return 2;
    f=fopen("EXTRA.PRG","wb");if(!f)return 2;
    fputs("SET TALK OFF\r\nSET SAFETY OFF\r\nON ERROR ? 'MINT-ERROR',ERROR(),MESSAGE()\r\nPUBLIC PUB\r\nPUB=2\r\nPRI=3\r\nSAVE TO SCOPE0\r\nDO INNER\r\nUSE CLIENTS\r\n",f);
    for(int i=0;i<5;i++)fprintf(f,"LABEL FORM LP%d RECORD 1 TO FILE LO%d.TXT\r\n",i,i);
    for(int i=0;i<6;i++)fprintf(f,"COPY FILE IN%d.BIN TO OUT%d.BIN\r\n",i,i);
    fputs("USE\r\nSET ALTERNATE TO SALARY.OUT\r\nSET ALTERNATE ON\r\nDO SALARY\r\nSET ALTERNATE TO\r\nQUIT\r\nPROCEDURE INNER\r\nPRIVATE PRI\r\nPRI=4\r\nLOCAL=5\r\nSAVE TO SCOPE1\r\nRETURN\r\n",f);fclose(f);
    for(int i=0;i<5;i++) {
        memset(lbl,' ',sizeof lbl);lbl[0]=lbl[1033]=2;
        memset(lbl+61,0,12);lbl[61]=1;lbl[63]=80;lbl[71]=1;
        memcpy(lbl+73,expr[i],strlen(expr[i]));
        snprintf(name,sizeof name,"LP%d.LBL",i);f=fopen(name,"wb");if(!f)return 2;
        fwrite(lbl,1,sizeof lbl,f);fclose(f);
    }
    for(int i=0;i<6;i++) {
        snprintf(name,sizeof name,"IN%d.BIN",i);f=fopen(name,"wb");if(!f)return 2;
        for(int j=0;j<sizes[i];j++)fputc('A'+j%26,f);
        fclose(f);
    }
    return 0;
}
