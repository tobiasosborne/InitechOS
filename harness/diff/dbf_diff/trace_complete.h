/* PRD 8 / Rules 2/6: do not confuse launch/trace failure with a mutant bite. */
#ifndef SAMIR_TRACE_COMPLETE_H
#define SAMIR_TRACE_COMPLETE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static inline unsigned emu_trace_keys(const char *keys)
{
    unsigned n=*keys?1:0;
    for(;*keys;keys++)if(*keys==',')n++;
    return n;
}
static inline int emu_trace_complete(const char *report,const char *serial,unsigned expected)
{
    char buf[16384];size_t n=0;int present=0;const char *keys;
    FILE *f=fopen(report,"rb");
    if(f){n=fread(buf,1,sizeof buf-1,f);fclose(f);}buf[n]=0;
    f=fopen(serial,"rb");if(f){present=fgetc(f)!=EOF;fclose(f);}
    keys=strstr(buf,"[harness] keys_sent=");
    if(!present || !expected || !keys || strtoul(keys+20,NULL,10)!=expected ||
       !strstr(buf,"[harness] OK=1\n") ||
       !strstr(buf,"[harness] launched=1 timed_out=0 exit=0 signal=0\n")) {
        fprintf(stderr,"RED emulator trace infrastructure: %s / %s\n",report,serial);
        return 0;
    }
    return 1;
}
#endif
