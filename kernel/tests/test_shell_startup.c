/* Native DOS CFG3 never enters AUTOEXEC after an interpreter refusal.
 * Exercise the real production entry with boundary spies; HLT traps only in
 * this hosted child. No test path is compiled into the product. */
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include "test_assert.h"
TEST_HARNESS();
void kernel_main(void);
static int event_fd,ready;
static void event(char c) { if(write(event_fd,&c,1)!=1) _exit(3); }
void startup(void) { event('S'); }
int sysinit_shell_ready(void) { return ready; }
void serial_puts(const char *s) { (void)s; event('M'); }
void command_repl(int permanent) { event('R'); _exit(permanent==1 ? 0 : 4); }
int main(void)
{
    for(int accepted=0;accepted<2;accepted++) {
        int fds[2],status;
        if(pipe(fds)) return 2;
        pid_t child=fork();
        if(child<0) return 2;
        if(!child) {
            struct rlimit no_core={0,0}; setrlimit(RLIMIT_CORE,&no_core);
            close(fds[0]); event_fd=fds[1]; ready=accepted;
            kernel_main(); _exit(5);
        }
        close(fds[1]);
        char events[16]={0}; ssize_t got=read(fds[0],events,sizeof events-1);
        /* Events can arrive in separate writes. Read through child exit. */
        size_t total=got>0 ? (size_t)got : 0;
        while(total<sizeof events-1 &&
              (got=read(fds[0],events+total,sizeof events-1-total))>0) total+=(size_t)got;
        close(fds[0]); waitpid(child,&status,0);
        if(accepted) {
            CHECK(WIFEXITED(status) && WEXITSTATUS(status)==0,"accepted shell enters permanent interpreter");
            CHECK(!strcmp(events,"SMR"),"accepted entry orders startup, marker, interpreter");
        } else {
            CHECK(WIFSIGNALED(status) && WTERMSIG(status)==SIGSEGV,"refused shell halts hosted production entry");
            CHECK(!strcmp(events,"S"),"refused shell never enters interpreter/AUTOEXEC");
        }
    }
    return TEST_SUMMARY("shell-startup");
}
