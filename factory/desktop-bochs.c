/* Journey-only frontend over the unchanged Bochs driver. W waits use the
 * serial transcript, as QEMU does; each repeated marker needs a new occurrence.
 * No emulator-driver source or product code is changed. */
#include <time.h>
#include "emu/bochs_trace.h"
static int journey_parse(const char *s, size_t n, BochsTraceStep *v)
{
    static struct { char text[160]; unsigned count; } waits[64];
    static unsigned used;
    static int validated;
    /* bochs_run validates the entire trace before starting the emulator.
     * The final token terminates that pass; subsequent calls replay it. */
    if (!validated) {
        int ok;
        if(n>=2 && s[0]=='W') { memset(v,0,sizeof *v);v->kind=BX_WAIT;ok=1; }
        else ok=bochs_trace_parse(s,n,v);
        if(s[n]==0)validated=1;
        return ok;
    }
    if (n < 2 || s[0] != 'W') return bochs_trace_parse(s,n,v);
    if (n >= sizeof waits[0].text) return 0;
    char marker[160];memcpy(marker,s+1,n-1);marker[n-1]=0;
    unsigned slot=0;
    while(slot<used && strcmp(waits[slot].text,marker))slot++;
    if(slot==used) {
        if(used==64)return 0;
        strcpy(waits[used++].text,marker);
    }
    unsigned want=++waits[slot].count;
    const char *path=getenv("DESKTOP_SERIAL");
    if(!path)return 0;
    struct timespec pause={0,20000000};
    for(unsigned poll=0;poll<500;poll++) {
        FILE *f=fopen(path,"r");unsigned seen=0;char line[4096];
        if(f) {
            while(fgets(line,sizeof line,f))if(strstr(line,marker))seen++;
            fclose(f);
        }
        if(seen>=want) { memset(v,0,sizeof *v);v->kind=BX_WAIT;return 1; }
        nanosleep(&pause,0);
    }
    fprintf(stderr,"desktop: W marker '%s' occurrence %u missing (input stopped)\n",marker,want);
    return 0;
}
#define bochs_trace_parse journey_parse
#include "emu/bochs.c"
#include "emu/bochs_main.c"
