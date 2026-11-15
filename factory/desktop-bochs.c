/* Journey frontend: consume meaningful serial progress within an idle budget,
 * while the driver's absolute overall deadline always bounds every wait. */
#include <time.h>
#include "emu/bochs_trace.h"
#include "emu/marker_match.h"
static char *read_file(const char *path, size_t *len);
/* End of the last accepted W marker in the transcript (emu/marker_match.h). */
static size_t journey_marker_consumed;
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
    static int validated;
    if (!validated) {
        int ok;
        if(n>=2 && s[0]=='W') { memset(v,0,sizeof *v);v->kind=BX_WAIT;ok=1; }
        else ok=bochs_trace_parse(s,n,v);
        if(s[n]==0)validated=1;
        return ok;
    }
    if (n < 2 || s[0] != 'W') return bochs_trace_parse(s,n,v);
    if (n >= 160) return 0;
    char marker[160];memcpy(marker,s+1,n-1);marker[n-1]=0;
    size_t after=journey_marker_consumed;
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
    size_t consumed=0;
    unsigned extensions=0;
    for(;;) {
        long long now=mono_ms();
        if(journey_deadline && now>=journey_deadline) {
            fprintf(stderr,"desktop: hard overall deadline expired waiting for W marker '%s' after byte %zu (waited %lldms)\n",marker,after,now-start);
            return 0;
        }
        if(now-last_progress>=idle_ms) {
            fprintf(stderr,"desktop: W marker '%s' after byte %zu missing: no meaningful serial/input progress for %lldms (waited %lldms; input stopped)\n",marker,after,idle_ms,now-start);
            return 0;
        }
        if(journey_idle)journey_idle(journey_context);
        now=mono_ms();
        /* Expiry is checked before accepting even a newly arrived marker. */
        if((journey_deadline && now>=journey_deadline) || now-last_progress>=idle_ms)
            continue;
        size_t len=0,end=0;char *txt=read_file(path,&len);
        int progress=0,seen=0;
        if(txt) {
            size_t complete=len;
            while(complete>0 && txt[complete-1]!='\n')complete--;
            if(complete<consumed || len<journey_marker_consumed) {
                free(txt);fputs("desktop: serial transcript shrank during a wait\n",stderr);return 0;
            }
            for(size_t at=consumed;at<complete;) {
                char line[4096];size_t k=0;
                while(at<complete && txt[at]!='\n') {if(k<sizeof line-1)line[k++]=txt[at];at++;}
                line[k]=0;at++;
                if(journey_progress(line))progress=1;
            }
            consumed=complete;
            seen=marker_token_after(txt,len,journey_marker_consumed,marker,&end);
            free(txt);
        }
        if(seen) {
            journey_marker_consumed=end;
            if(extensions)fprintf(stderr,"desktop: W marker '%s' reached after %lldms; progress renewals=%u\n",marker,now-start,extensions);
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
