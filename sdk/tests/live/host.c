#include "tbx_host.h"
#include <stdio.h>
#include <string.h>
#include "cases.h"
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    tbx_host_reset();tbx_host_put_file("SDK.DAT","abcdefghij",10);
    tbx_host_put_file("SUB",0,0);tbx_host.files[1].attr=16;
    cases();
    unsigned bad=0;
    int post_ok=postexit();
    if(exit_first!=0||exit_second!=0||tbx_host.exit_rc!=0){
        fprintf(stderr,"gate repeated EXIT: live first=0 second=0 final=0; fake first=%d second=%d final=%d\n",
                exit_first,exit_second,tbx_host.exit_rc);bad++;
    }else if(!post_ok){fprintf(stderr,"gate post-exit conformance FAIL\n");bad++;}
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    for(unsigned i=0;i<nr;i++) {
        unsigned char b[4];if(fread(b,1,4,f)!=4)return 2;
        int32_t want=(int32_t)((uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24);
        if(want!=results[i]) {fprintf(stderr,"gate case %u: live=%d fake=%d\n",i,want,results[i]);bad++;}
    }
    if(fgetc(f)!=EOF)return 2;
    fclose(f);printf("gate conformance: %u calls, %u mismatches\n",nr,bad);return bad!=0;
}
