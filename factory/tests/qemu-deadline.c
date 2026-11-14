/* Deterministic QMP peer + virtual wall clock. Exercises the actual session
 * with completion at 125 s against caller deadlines of 180 s and 90 s.
 * No real emulator, product renderer, or multi-minute sleep supplies expected
 * results: a caller's wall deadline is the independent contract. */
#define clock_gettime deadline_clock_gettime
#define nanosleep deadline_nanosleep
#define poll deadline_poll
#include "../emu/qemu.c"
#undef clock_gettime
#undef nanosleep
#undef poll
static long long virtual_ms, marker_at;
static const char *transcript;
static void advance(long long ms)
{
    virtual_ms+=ms;
    FILE *f=fopen(transcript,"w");
    if(!f)abort();
    fputs(virtual_ms>=marker_at?"done\n":"progress\n",f);
    fclose(f);
}
int deadline_clock_gettime(clockid_t id,struct timespec *ts)
{
    (void)id;ts->tv_sec=virtual_ms/1000;ts->tv_nsec=virtual_ms%1000*1000000;return 0;
}
int deadline_nanosleep(const struct timespec *ts,struct timespec *remaining)
{
    (void)remaining;advance(ts->tv_sec*1000+ts->tv_nsec/1000000);return 0;
}
int deadline_poll(struct pollfd *fds,nfds_t n,int ms)
{
    (void)fds;(void)n;if(ms>0)advance(ms);return 0;
}
static int run(long long deadline,long long complete,int want)
{
    int fd[2],seen=0;virtual_ms=0;marker_at=complete;advance(0);
    if(socketpair(AF_UNIX,SOCK_STREAM,0,fd))return 1;
    int rc=qmp_session(fd[0],NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,
        transcript,NULL,NULL,NULL,NULL,0,NULL,0,"done",1000000,1,&seen,
        NULL,NULL,NULL,NULL,deadline,-1);
    close(fd[1]);
    if((want && (rc || !seen)) || (!want && (!rc || seen))) {
        fprintf(stderr,"FAIL QEMU deadline: requested=%lld marker=%lld elapsed=%lld rc=%d seen=%d expected=%d\n",
            deadline,complete,virtual_ms,rc,seen,want);return 1;
    }
    return 0;
}
int main(void)
{
    char path[]="/tmp/initech-qemu-deadline.XXXXXX";
    int fd=mkstemp(path);if(fd<0)return 2;close(fd);transcript=path;
    int bad=run(180000,125000,1) || run(90000,125000,0) || run(90000,90000,0);
    unlink(path);
    if(!bad)puts("ok QEMU: caller 180s honored past 120s; 90s expiry/late marker fail explicitly");
    return bad;
}
