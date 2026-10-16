/* Host half of initech-bsy.1/bcg.13: same matcher as test-80k, independent
 * mdir inventories on identical FAT images. MS-DOS 3.3 Reference pp.57-58;
 * ADR-0003 App A 4Eh/4Fh. No shell formatter supplies expected membership. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
extern void int21_wildcard_build(const char *,uint8_t[11]);
extern int int21_wildcard_match(const uint8_t[11],const uint8_t[11]);
int main(int argc,char **argv)
{
    char line[256]; uint8_t pattern[11],name[11];
    if(argc!=2)return 2;
    int21_wildcard_build(argv[1],pattern);
    while(fgets(line,sizeof line,stdin)) {
        char *p=strrchr(line,'/'); p=p?p+1:line;
        p[strcspn(p,"\r\n")]=0;
        if(!*p)continue;
        memset(name,' ',sizeof name);
        int i=0,ext=0;
        for(char *s=p;*s;s++) {
            if(*s=='.'){i=8;ext=1;continue;}
            if(i>=(ext?11:8))return 2;
            name[i++]=(uint8_t)*s;
        }
        if(int21_wildcard_match(pattern,name))puts(p);
    }
    return ferror(stdin)?2:0;
}
