/* W4.0 review item 4: a stalled optional helper cannot outlive the harness.
 * Exercise the production helper branch with real fork/exec, kill and reap. */
#define _POSIX_C_SOURCE 200809L
#include "qemu.c"

int main(void)
{
    char work[]="build/screen-helper-test.XXXXXX", tool[256], output[256];
    if (!mkdtemp(work)) return 2;
    snprintf(tool,sizeof tool,"%s/stall",work);
    snprintf(output,sizeof output,"%s/page.ppm.screen",work);
    FILE *f=fopen(tool,"w");
    if (!f) return 2;
    fputs("#!/bin/sh\nexec sleep 20\n",f);
    if (fclose(f) || chmod(tool,0700)) return 2;
    if (setenv("INITECH_TEXT_SCREEN_ELF","unused.elf",1) ||
        setenv("INITECH_TEXT_SCREEN_TOOL",tool,1)) return 2;
    long long start=mono_ms();
    char ppm[256]; snprintf(ppm,sizeof ppm,"%s/page.ppm",work);
    int pair[2];
    if (socketpair(AF_UNIX,SOCK_STREAM,0,pair)) return 2;
    int rc=qmp_do_screendump(pair[0],ppm,start+120);
    close(pair[0]); close(pair[1]);
    long long elapsed=mono_ms()-start;
    int status; errno=0;
    pid_t child=waitpid(-1,&status,WNOHANG);
    int bad=rc!=-1 || elapsed<120 || elapsed>1500 ||
            child!=-1 || errno!=ECHILD || access(output,F_OK)==0;
    unlink(tool); rmdir(work);
    if (bad) { fprintf(stderr,"RED: stalled helper escaped deadline/reap (%lld ms)\n",elapsed); return 1; }
    puts("GREEN: stalled text-screen helper killed and reaped within harness deadline");
    return 0;
}
