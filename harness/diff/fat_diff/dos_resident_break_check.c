/* initech-c80y. Independent audit HUGE.TXT bytes + DOS 3.3 Guide p.159.
 * Raw lengths matter: strlen would hide the audited 14863 leaked NULs. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char serial[262144], file[262144];
static size_t load(const char *path,unsigned char *b,size_t cap)
{
    FILE *f=fopen(path,"rb"); if(!f)exit(2);
    size_t n=fread(b,1,cap-1,f); if(ferror(f) || !feof(f))exit(2);
    fclose(f);b[n]=0;return n;
}
static void red(const char *s) { fprintf(stderr,"RED dos-resident-break: %s\n",s);exit(1); }
int main(int argc,char **argv)
{
    if(argc!=4)return 2;
    size_t n=load(argv[1],serial,sizeof serial), f=load(argv[2],file,sizeof file);
    if(memchr(serial,0,n))red("serial contains non-file NUL bytes");
    char *s=(char *)serial, *start=strstr(s,"C80Y-BEGIN\r\n");
    char *stop=strstr(s,"^C\r\nINT23-BREAK\n"), *prompt=stop?strstr(stop,"A:\\T>"):NULL;
    if(!start || !stop || !prompt)red("missing cancellation or returned prompt");
    if(strstr(s,"HUGE LINE 01999") || strstr(s,"F299"))red("listing reached the final line after cancellation");
    if(strstr(s,"C80Y-MUST-NOT-RUN"))red("cancelled batch continued");
    if(!strstr(prompt,"A:\\T>echo after-cancel\r\nafter-cancel\r\n"))red("restored prompt did not execute ECHO");
    start+=strlen("C80Y-BEGIN\r\n");
    if(!strcmp(argv[3],"controlshort")) {
        size_t prefix=(size_t)(stop-start);
        if(!prefix || prefix>=f || memcmp(start,file,prefix))red("TYPE emitted non-file bytes before cancellation");
        stop+=strlen("^C\r\nINT23-BREAK\n");
        if(stop!=prompt)red("TYPE emitted bytes after cancellation");
    }
    if(!strcmp(argv[3],"gaps2")) {
        stop+=strlen("^C\r\nINT23-BREAK\n");
        if(stop!=prompt)red("DIR emitted rows after the cancelled pause");
    }
    puts("GREEN dos-resident-break byte oracle");return 0;
}
