/* Accept only the adapter's complete publication in this child's fresh log.
 * The Bochs 2.7 plugin still logs its requested port (gui/rfb.cc). */
#ifndef INITECH_RFB_PORT_H
#define INITECH_RFB_PORT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BOCHS_RFB_PORT_PREFIX "[initech-rfb] bound loopback port "
static inline int bochs_rfb_log_port(FILE *f)
{
    char line[1024];
    const char prefix[] = BOCHS_RFB_PORT_PREFIX;
    while(fgets(line,sizeof line,f)) {
        if(strncmp(line,prefix,sizeof prefix-1))continue;
        char *digits=line+sizeof prefix-1,*end;
        long port=strtol(digits,&end,10);
        if(end==digits || port<1 || port>65535)continue;
        if(!strcmp(end,"\n") || !strcmp(end,"\r\n"))return (int)port;
    }
    return 0;
}
#endif
