/* Independent virtual-clock serial producer exercises the real journey wait. */
static long long clock_ms;
#define clock_gettime wait_clock_gettime
#define nanosleep wait_nanosleep
#define main desktop_driver_main
#include "desktop-bochs.c"
#undef main
#undef nanosleep
#undef clock_gettime
int wait_clock_gettime(clockid_t id,struct timespec *ts)
{
    (void)id;ts->tv_sec=clock_ms/1000;ts->tv_nsec=clock_ms%1000*1000000;return 0;
}
int wait_nanosleep(const struct timespec *ts,struct timespec *rest)
{
    (void)rest;clock_ms+=ts->tv_sec*1000+ts->tv_nsec/1000000;return 0;
}
static FILE *serial;
static long long next_progress, marker_at;
static const char *marker, *activity;
static int published;
static void produce(void *unused)
{
    (void)unused;
    while(activity && clock_ms>=next_progress) {
        fputs(activity,serial);next_progress+=1000;
    }
    if(!published && clock_ms>=marker_at) {fprintf(serial,"%s\n",marker);published=1;}
    fflush(serial);
}
static int run(const char *name,const char *progress,long long at,long long hard,int want,long long elapsed)
{
    clock_ms=0;next_progress=1000;marker_at=at;marker=name;activity=progress;published=0;
    if(ftruncate(fileno(serial),0)||fseek(serial,0,SEEK_SET))return 1;
    long long deadline=hard;
    BOCHS_TRACE_BIND_IDLE(NULL); /* Same binding as bochs_run, virtual transport. */
    journey_idle=produce;journey_context=NULL;
    char token[160];snprintf(token,sizeof token,"W%s",name);
    BochsTraceStep step;
    int ok=journey_parse(token,strlen(token),&step);
    if(ok!=want || clock_ms!=elapsed) {
        fprintf(stderr,"FAIL desktop wait: %s result=%d elapsed=%lld expected=%d/%lld\n",name,ok,clock_ms,want,elapsed);
        return 1;
    }
    printf("ok desktop wait: %s result=%d elapsed=%lld\n",name,ok,clock_ms);return 0;
}
int main(void)
{
    unsetenv("DESKTOP_WAIT_MS");
    BochsTraceStep step;
    if(!journey_parse("kret",4,&step))return 2; /* complete preflight */
    char path[]="/tmp/initech-desktop-wait.XXXXXX";
    int fd=mkstemp(path);if(fd<0)return 2;
    serial=fdopen(fd,"w+");if(!serial)return 2;
    setenv("DESKTOP_SERIAL",path,1);
    const char *input="FLAIR-EVT what=3 where=112,102 msg=00001e61\n";
    int bad=run("slow-input",input,15000,60000,1,15000);
    bad|=run("missing",NULL,999999,60000,0,10000);
    bad|=run("clock-only","FLAIR-CLOCK-POLL\nFLAIR-CLOCK 9:00 am\nFLAIR-EVT what=6\n",999999,60000,0,10000);
    bad|=run("boundary",input,12000,12000,0,12000);
    bad|=run("late",input,14000,12000,0,12000);
    bad|=run("painting","TENANT-GATE ax=0x0031 win=1 l=0 t=0 r=100 b=100\n",18000,60000,1,18000);
    bad|=run("repeated",input,0,60000,1,0);
    /* Do not clear the transcript: occurrence2 must wait for fresh evidence. */
    marker_at=12000;published=0;activity=input;next_progress=1000;
    int ok=journey_parse("Wrepeated",9,&step);
    if(!ok || clock_ms!=12000) {puts("FAIL desktop wait: repeated occurrence reused/stopped early");bad=1;}
    fclose(serial);unlink(path);
    return bad;
}
