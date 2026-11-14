/* Independent OS bind/listen state; private ephemeral port and lock file.
 * A busy listener must never be mistaken for the new emulator's server. */
#define _POSIX_C_SOURCE 200809L
#include "../emu/port_lock.h"
#include <stdlib.h>
#include <sys/wait.h>
static int listener(unsigned *port)
{
    int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return -1;
    struct sockaddr_in a={.sin_family=AF_INET,.sin_addr.s_addr=htonl(INADDR_ANY),.sin_port=htons(*port)};
    socklen_t n=sizeof a;
    if(bind(fd,(struct sockaddr *)&a,n)||listen(fd,1)||getsockname(fd,(struct sockaddr *)&a,&n))return -1;
    *port=ntohs(a.sin_port);return fd;
}
int main(void)
{
    char path[]="/tmp/initech-port-lock.XXXXXX";int initial=mkstemp(path);if(initial<0)return 2;close(initial);
    unsigned port=0;int server=listener(&port);if(server<0)return 2;
    int fd=factory_port_lock(path,port,80);
    if(fd>=0) {close(fd);close(server);unlink(path);fputs("FAIL port isolation: existing listener accepted\n",stderr);return 1;}
    close(server);
    fd=factory_port_lock(path,port,1000);if(fd<0)return 1;
    pid_t child=fork();if(child<0)return 2;
    if(!child) {
        close(fd);int second=factory_port_lock(path,port,80);
        if(second>=0) {close(second);_exit(1);} _exit(0);
    }
    int status;if(waitpid(child,&status,0)!=child || status)return 1;
    close(fd);
    fd=factory_port_lock(path,port,1000);if(fd<0)return 1;
    close(fd);unlink(path);
    puts("ok port isolation: occupied listener rejected, free endpoint accepted, cooperating holders exclude each other");
    return 0;
}
