/* The accepted AX set is read independently from the PRODUCT dispatcher,
 * not from the fake's switch or from the SDK header. */
#include "tbx_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
static unsigned char accepted[65536];
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END))return 2;
    long n=ftell(f);if(n<0)return 2;
    rewind(f);char *text=malloc((size_t)n+1);if(!text||fread(text,1,(size_t)n,f)!=(size_t)n)return 2;
    text[n]=0;fclose(f);
    char *start=strstr(text,"switch (ax) {");
    char *end=start?strstr(start,"default:"):0;if(!end)return 2;
    unsigned count=0;
    f=fopen(argv[2],"r");if(!f)return 2;
    char row[256],name[80],binding[80];unsigned code;int arity;
    while(fgets(row,sizeof row,f)) {
        if(sscanf(row,"%79s %x %d %79s",name,&code,&arity,binding)!=4||code>65535)continue;
        char needle[100];snprintf(needle,sizeof needle,"case %s:",name);
        char *found=strstr(start,needle);
        if(found&&found<end){accepted[code]=1;count++;}
    }
    fclose(f);free(text);if(count!=37)return 2;
    tbx_host_reset();tbx_host.registrations=1;
    unsigned bad=0;uint32_t args[6]={0};
    for(unsigned c=0;c<65536;c++) {
        tbx_host.exit_calls=0;
        int32_t r=tbx_trap(c,6,args);
        if((r!=TBX_ERR_BADCODE)!=accepted[c]) {
            fprintf(stderr,"dispatch AX=%04x live table=%u fake=%d\n",c,accepted[c],r);bad++;
        }
    }
    printf("dispatch conformance: 65536 AX values, %u product verbs, %u mismatches\n",count,bad);
    return bad!=0;
}
