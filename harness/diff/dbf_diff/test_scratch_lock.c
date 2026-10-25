/* Fault-injected lock preconditions (CLAUDE Rules 2/6). */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static const char *mode;
static unsigned attempts;
static int lock_open(const char *path,int flags,int permissions)
{
    if(!strcmp(mode,"open")) { errno=EACCES;return -1; }
    return open(path,flags,permissions);
}
static int lock_fcntl(int fd,int operation,...)
{
    if(operation==F_SETFD)return fcntl(fd,F_SETFD,FD_CLOEXEC);
    attempts++;
    if(!strcmp(mode,"fcntl")) { errno=ENOLCK;return -1; }
    if(!strcmp(mode,"eintr") && attempts==1) { errno=EINTR;return -1; }
    struct flock l={.l_type=F_WRLCK,.l_whence=SEEK_SET,.l_start=0,.l_len=0};
    return fcntl(fd,operation,&l);
}
#define open lock_open
#define fcntl lock_fcntl
#include "scratch_lock.h"
int main(int argc,char **argv)
{
    if(argc!=3)return 3;
    mode=argv[1];scratch_lock(argv[2]);
    if(!strcmp(mode,"eintr") && attempts!=2)return 4;
    puts("critical section entered");return 0;
}
