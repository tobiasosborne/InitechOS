/* PRD 6.6; initech-0spyf/fzsvo. The real III+ M005ULOG.TXT table
 * starts 0D 0A 52 65 63 6F 72 64 23. Enter's CR is separate from LIST's
 * opening CR LF. Check raw serial bytes before any line normalization. */
#include <stdio.h>
#include <string.h>
static char real[65536], serial[262144];
static int read_file(const char *path,char *buf,size_t cap)
{
    FILE *f=fopen(path,"rb");size_t n;
    if(!f) { perror(path);return 2; }
    n=fread(buf,1,cap-1,f);
    if(ferror(f) || !feof(f)) { fclose(f);return 2; }
    buf[n]=0;return fclose(f)?2:0;
}
int main(int argc,char **argv)
{
    const char *g,*a;size_t n=9;
    if(argc!=3) return 2;
    if(read_file(argv[1],real,sizeof real) || read_file(argv[2],serial,sizeof serial)) return 2;
    g=strstr(real,"\r\nRecord#");a=strstr(serial,". list\r");
    if(!g || !a) { fputs("boot LIST: missing real table or command echo\n",stderr);return 2; }
    a+=strlen(". list\r");
    if(strlen(a)<n || memcmp(g,a,n)) {
        puts("RED boot LIST opening after Enter: differs at byte 0");
        printf("  real:");for(size_t i=0;i<n;i++) printf(" %02X",(unsigned char)g[i]);
        printf("\n  got: ");for(size_t i=0;i<n && a[i];i++) printf(" %02X",(unsigned char)a[i]);
        putchar('\n');return 1;
    }
    puts("GREEN boot LIST opening after Enter: real 0D 0A 52 65 63 6F 72 64 23");
    return 0;
}
