/* Locked flat ABI check, IBM DOS 3.30 Technical Reference pp.6-119..121,
 * 6-188; ADR-0003 DEC-04a turns DS:DX/DS:SI into EDX/ESI. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main(int argc,char **argv)
{
    char buf[40000];if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(buf,1,sizeof(buf)-1,f);buf[n]=0;fclose(f);
    const char *ah[]={"39h","3Ah","3Bh","47h","40h"};
    for(int i=0;i<5;i++) {
        char key[30];snprintf(key,sizeof key,"\"ah\": \"%s\"",ah[i]);
        char *p=strstr(buf,key),*end=p?strstr(p,"\n    }"):NULL;
        if(!p || !end) {fprintf(stderr,"RED dir-abi: %s row missing\n",ah[i]);return 1;}
        char saved=*end;*end=0;
        if(!strstr(p,i==3?"\"ESI\"":"\"EDX\"") || !strstr(p,"\"cf\"") ||
           (i==3 && !strstr(p,"\"DL\""))) {
            fprintf(stderr,"RED dir-abi: %s register contract\n",ah[i]);return 1;
        }
        *end=saved;
    }
    char *write=strstr(buf,"\"ah\": \"40h\"");
    char *end=strstr(write,"\n    }");*end=0;
    if(!strstr(write,"ECX=0") || !strstr(write,"\"EBX\"") ||
       !strstr(write,"\"ECX\"") || !strstr(write,"6-140")) {
        fprintf(stderr,"RED dir-abi: 40h zero-count size contract\n");return 1;
    }
    puts("GREEN test-dir-abi: 39h/3Ah/3Bh EDX, 47h DL/ESI, 40h ECX=0 size; IBM pp.6-119..121,6-140,6-188");
    return 0;
}
