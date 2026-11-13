/* Pin the frontend's validation/replay distinction and repeated W semantics. */
#define main desktop_driver_main
#include "desktop-bochs.c"
#undef main
int main(void)
{
    BochsTraceStep step;
    /* The validation pass must not wait for a transcript that does not exist. */
    if(!journey_parse("Wprogress,kret",9,&step)||step.kind!=BX_WAIT)return 1;
    if(!journey_parse("kret",4,&step)||step.kind!=BX_KEY)return 1;
    FILE *f=tmpfile();if(!f)return 2;
    char path[64];snprintf(path,sizeof path,"/proc/self/fd/%d",fileno(f));
    if(setenv("DESKTOP_SERIAL",path,1))return 2;
    fputs("progress\n",f);fflush(f);
    if(!journey_parse("Wprogress",9,&step)||step.kind!=BX_WAIT)return 1;
    long long start=mono_ms();
    pid_t child=fork();if(child<0)return 2;
    if(!child) {
        struct timespec pause={0,60000000};nanosleep(&pause,0);
        fputs("progress\n",f);fflush(f);_exit(0);
    }
    if(!journey_parse("Wprogress",9,&step)||step.kind!=BX_WAIT)return 1;
    int status;if(waitpid(child,&status,0)!=child||status||mono_ms()-start<40)return 1;
    if(journey_parse("not-a-key",9,&step))return 1;
    fclose(f);
    puts("ok desktop primitives: validation, live marker waits, repeated progress, bad token");
    return 0;
}
