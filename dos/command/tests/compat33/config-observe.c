/* Observation only: print the actual pure parser result, no reference claims. */
#include <stdio.h>
#include <string.h>
#include "config_sys.h"
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    char line[2048];unsigned n=0;
    while(fgets(line,sizeof line,f)) {
        dos_config_t c;int r=config_sys_parse(line,(unsigned)strlen(line),&c);
        printf("line=%u recognized=%d files=%u:%u buffers=%u:%u lastdrive=%u:%c device=%u install=%u shell=%u:%s break=%u:%u\n",
            ++n,r,c.files_present,c.files,c.buffers_present,c.buffers,c.lastdrive_present,
            c.lastdrive?c.lastdrive:'-',c.device_count,c.install_count,c.shell_present,c.shell,c.break_present,c.break_on);
    }
    fclose(f);return 0;
}
