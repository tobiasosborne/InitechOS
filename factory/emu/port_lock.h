/* Host resource arbitration. A listener is available only after both the
 * cooperating harness lock and the OS endpoint are free. Queueing happens
 * before the guest deadline. Never connect to another run's RFB/GDB server. */
#ifndef INITECH_FACTORY_PORT_LOCK_H
#define INITECH_FACTORY_PORT_LOCK_H
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
static long long factory_port_now(void)
{
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);
    return (long long)t.tv_sec*1000+t.tv_nsec/1000000;
}
static int factory_port_lock(const char *path,unsigned port,int wait_ms)
{
    int fd=open(path,O_CREAT|O_RDWR|O_CLOEXEC,0666);
    if(fd<0) {perror(path);return -1;}
    long long until=factory_port_now()+wait_ms;
    int locked=0,told=0;
    for(;;) {
        if(!locked) {
            if(flock(fd,LOCK_EX|LOCK_NB)==0)locked=1;
            else if(errno!=EWOULDBLOCK && errno!=EINTR) {perror(path);close(fd);return -1;}
        }
        if(locked) {
            int probe=socket(AF_INET,SOCK_STREAM,0);
            if(probe<0) {perror("port probe socket");close(fd);return -1;}
            int reuse=1;
            if(setsockopt(probe,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof reuse)) {
                perror("port probe reuse");close(probe);close(fd);return -1;
            }
            struct sockaddr_in a={.sin_family=AF_INET,
                .sin_addr.s_addr=htonl(INADDR_ANY),.sin_port=htons(port)};
            int rc=bind(probe,(struct sockaddr *)&a,sizeof a),saved=errno;
            close(probe);
            if(rc==0) return fd;
            if(saved!=EADDRINUSE && saved!=EINTR) {
                errno=saved;perror("port probe bind");close(fd);return -1;
            }
        }
        if(!told) {fprintf(stderr,"[harness] waiting for port %u (%s)\n",port,path);told=1;}
        if(factory_port_now()>=until) {
            fprintf(stderr,"[harness] port %u unavailable after %dms (%s)\n",port,wait_ms,path);
            close(fd);return -1;
        }
        struct timespec pause={0,20000000};nanosleep(&pause,NULL);
    }
}
#endif
