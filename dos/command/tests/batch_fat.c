/* Drive actual batch control with genuine DOS input; stdout is compared by
 * the shell gate to captured transcript bytes, not to this program. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blockdev_file.h"
#include "devices.h"
#include "psp.h"
#include "sft.h"
#define COMMAND_BATCH_HOST
#define COMMAND_BATCH_FAT_HOST
#include "command.c"
#include "batch.c"
#include "env.c"
static void sink(char c) { putchar(c); }
static int run_pipeline_or_redirect(const char *s)
{
    if(dispatch_control(s))return 0;
    cmd_line_t c;cmd_parse(s,&c);
    if(c.kind!=CMD_ECHO) { fprintf(stderr,"unsupported batch fixture body: %s\n",s);exit(2); }
    dos_puts_raw(c.arg);dos_print("\r\n$");return 0;
}
int main(int argc,char **argv)
{
    (void)g_out;
    if(argc!=3 && argc!=4)return 2;
    blockdev_file_t disk;fat12_volume_t vol;uint8_t sector[512];
    static psp_t process;psp_params_t params={0};params.alloc_end_linear=0x80000;
    if(blockdev_file_open_rw(&disk,argv[1]) || fat12_mount(&vol,&disk.dev,sector) || fileio_fat_bind(&vol))return 2;
    psp_build(&process,&params);sft_init();int21_set_psp(&process);int21_set_sink(sink);
    devices_init();int21_set_devices(devices_head(),0);env_init(&g_master_env);
    if(argc==4) {
        run_external(argv[2],argv[3]);
        blockdev_file_close(&disk);return 0;
    }
    FILE *f=fopen(argv[2],"rb");if(!f)return 2;
    char line[128],expanded[128];
    while(fgets(line,sizeof line,f)) {
        line[strcspn(line,"\r\n")]=0;
        if(batch_expand(line,0,0,batch_env_cb,&g_master_env,expanded,sizeof expanded)<0)return 2;
        run_pipeline_or_redirect(expanded);
    }
    fclose(f);blockdev_file_close(&disk);return 0;
}
