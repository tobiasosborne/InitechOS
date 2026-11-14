/* Bochs 2.7 rfb.cc publishes the OS-selected listener after trying 5900+.
 * Only a complete publication in this child's fresh log is authoritative;
 * a "Trying port" line is not a successful bind. */
#ifndef INITECH_RFB_PORT_H
#define INITECH_RFB_PORT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static inline int bochs_rfb_log_port(FILE *f)
{
    char line[1024];
    const char prefix[]=" listening for connections on port ";
    while(fgets(line,sizeof line,f)) {
        char *tag=strstr(line,"[RFB"), *p=tag?strchr(tag,']'):NULL;
        if(!p || strncmp(p+1,prefix,sizeof prefix-1))continue;
        char *digits=p+1+sizeof prefix-1,*end;
        long port=strtol(digits,&end,10);
        if(end==digits || port<1 || port>65535)continue;
        if(!strcmp(end,"\n") || !strcmp(end,"\r\n"))return (int)port;
    }
    return 0;
}
#endif
