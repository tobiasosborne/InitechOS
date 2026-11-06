/* Deterministic negative controls, factory C. initech-9nzpr, DEC-03a.10. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv)
{
    if (argc!=5) { fputs("usage: w4_mutate --xor|--truncate OFFSET INPUT OUTPUT\n",stderr); return 2; }
    char *end; errno=0; unsigned long at=strtoul(argv[2],&end,0);
    if (errno || *end || argv[2][0]=='-') return 2;
    FILE *f=fopen(argv[3],"rb");
    if (!f) { perror(argv[3]); return 2; }
    if (fseek(f,0,SEEK_END)) return 2;
    long size=ftell(f); rewind(f);
    if (size<0 || size>16*1024*1024 || at>=(unsigned long)size) return 2;
    unsigned char *p=malloc((size_t)size);
    if (!p || fread(p,1,(size_t)size,f)!=(size_t)size || fclose(f)) return 2;
    if (!strcmp(argv[1],"--xor")) p[at]^=1;
    else if (!strcmp(argv[1],"--truncate")) size=(long)at;
    else return 2;
    f=fopen(argv[4],"wb");
    if (!f || fwrite(p,1,(size_t)size,f)!=(size_t)size || fclose(f)) return 2;
    free(p); return 0;
}
