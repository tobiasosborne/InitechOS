/* Physical-line bytes graded against the independent DOS 3.30 capture. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define COMMAND_BATCH_HOST
#include "command.c"
#include "batch.c"
#include "env.c"
static unsigned echoes;
static int run_pipeline_or_redirect(const char *s)
{
    if(dispatch_control(s))return 0;
    if(!strncmp(s,"ECHO >E",7)) {
        int number=atoi(s+7);
        const char *payload=strchr(s+7,' ');
        char path[256],expected[256];
        snprintf(path,sizeof path,"command/tests/dos33/reference/batch/files/E%d.OUT",number);
        FILE *f=fopen(path,"rb"); if(!f)exit(2);
        size_t n=fread(expected,1,sizeof expected,f);fclose(f);
        if(!payload || n<2 || strlen(payload)!=n-2 || memcmp(payload,expected,n-2)) {
            fprintf(stderr,"RED batch-physical: E%d differs from real DOS (command %zu, captured %zu)\n",number,strlen(payload),n-2);exit(1);
        }
        echoes++;
    }
    return 0;
}
int main(void)
{
    char script[4097];
    FILE *f=fopen("command/tests/dos33/reference/batch/AUTOEXEC.BAT","rb");if(!f)return 2;
    size_t n=fread(script,1,sizeof script-1,f);fclose(f);script[n]=0;
    env_init(&g_master_env);files[0]=script;
    const char *av[]={"MAIN.BAT"};run_batch("MAIN.BAT",av,1);
    f=fopen("command/tests/dos33/reference/batch/files/REM.OUT","rb");
    if(!f)return 2;
    char after[32];n=fread(after,1,sizeof after,f);fclose(f);
    if(n!=11 || memcmp(after,"REM-AFTER\r\n",11))return 1;
    if(echoes!=9){fprintf(stderr,"RED batch-physical: missing lines\n");return 1;}
    /* Control bodies must retain SHIFT's %0 and execute under IF/FOR.
     * MS-DOS 3.3 Reference pp.144-151; tests grade resulting commands. */
    puts("GREEN batch-physical: 124..132 bytes match OEM DOS 3.30, including 126/127/128 boundaries");
    return 0;
}
