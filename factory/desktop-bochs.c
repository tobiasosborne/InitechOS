/* Journey frontend: consume meaningful serial progress within an idle budget,
 * while the driver's absolute overall deadline always bounds every wait. */
#include <time.h>
#include "emu/bochs_trace.h"
static void (*journey_idle)(void *);
static void *journey_context;
static long long journey_deadline;
static long long mono_ms(void);
static void journey_rfb_pump(void *context);
static int journey_progress(const char *line)
{
    int what;
    if (sscanf(line,"FLAIR-EVT what=%d",&what)==1 ||
        sscanf(line,"TENANT-EVT what=%d",&what)==1)
        return what>=1 && what<=5;
    /* Initial painting and file operations can outlast input delivery.
     * Clock polling and clock-only redraw events are deliberately excluded. */
    const char *prefixes[]={"TENANT-GATE ","TENANT-SF ","TENANT-EXIT ",
        "FLAIR-DISPATCH ","FINDER-"};
    for(unsigned i=0;i<sizeof prefixes/sizeof prefixes[0];i++)
        if(!strncmp(line,prefixes[i],strlen(prefixes[i])))return 1;
    return 0;
}
static int journey_parse(const char *s, size_t n, BochsTraceStep *v)
{
    static struct { char text[160]; unsigned count; } waits[64];
    static unsigned used;
    static int validated;
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
    long long idle_ms=10000;
    const char *limit=getenv("DESKTOP_WAIT_MS");
    if(limit) {
        char *end;unsigned long ms=strtoul(limit,&end,10);
        if(!*limit || *end || ms<20 || ms>3600000)return 0;
        idle_ms=(long long)ms;
    }
    struct timespec pause={0,20000000};
    long long start=mono_ms(), last_progress=start;
    long consumed=0;
    unsigned extensions=0;
    for(;;) {
        long long now=mono_ms();
        if(journey_deadline && now>=journey_deadline) {
            fprintf(stderr,"desktop: hard overall deadline expired waiting for W marker '%s' occurrence %u (waited %lldms)\n",marker,want,now-start);
            return 0;
        }
        if(now-last_progress>=idle_ms) {
            fprintf(stderr,"desktop: W marker '%s' occurrence %u missing: no meaningful serial/input progress for %lldms (waited %lldms; input stopped)\n",marker,want,idle_ms,now-start);
            return 0;
        }
        if(journey_idle)journey_idle(journey_context);
        now=mono_ms();
        /* Expiry is checked before accepting even a newly arrived marker. */
        if((journey_deadline && now>=journey_deadline) || now-last_progress>=idle_ms)
            continue;
        FILE *f=fopen(path,"r");unsigned seen=0;char line[4096];
        int progress=0;
        if(f) {
            long complete=0;
            while(fgets(line,sizeof line,f)) {
                if(!strchr(line,'\n'))continue;
                complete=ftell(f);
                if(strstr(line,marker))seen++;
                if(complete>consumed && journey_progress(line))progress=1;
            }
            fclose(f);
            if(complete<consumed) {
                fputs("desktop: serial transcript shrank during a wait\n",stderr);return 0;
            }
            consumed=complete;
        }
        if(seen>=want) {
            if(extensions)fprintf(stderr,"desktop: W marker '%s' occurrence %u reached after %lldms; progress renewals=%u\n",marker,want,now-start,extensions);
            memset(v,0,sizeof *v);v->kind=BX_WAIT;return 1;
        }
        if(progress) {last_progress=now;extensions++;}
        nanosleep(&pause,0);
    }
}
#define BOCHS_TRACE_TAP_KEYS 1
#define bochs_trace_parse journey_parse
#define BOCHS_TRACE_BIND_IDLE(transport) do { \
    journey_idle=journey_rfb_pump; journey_context=(transport); \
    journey_deadline=deadline; \
} while(0)
#include "emu/bochs.c"
#include "emu/bochs_main.c"
static void journey_rfb_pump(void *context) { rfb_pump(context); }
