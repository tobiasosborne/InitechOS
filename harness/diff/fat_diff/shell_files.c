/* Same command.c builtins, real INT21/FAT, independent mtools reference.
 * MS-DOS 3.3 Reference pp.50-52,104; Guide p.17. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blockdev_file.h"
#include "devices.h"
#include "psp.h"
#include "sft.h"
#define COMMAND_FILE_HOST
#include "command.c"
static void sink(char c) { putchar(c); }
static int con_get(void) { int c=getchar(); if(c==EOF)exit(2); return c=='\n'?'\r':c; }
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    blockdev_file_t bf; fat12_volume_t vol; unsigned char sector[512];
    static psp_t psp; psp_params_t params={0}; params.alloc_end_linear=0x80000;
    if(blockdev_file_open_rw(&bf,argv[1]) || fat12_mount(&vol,&bf.dev,sector) || fileio_fat_bind(&vol))return 2;
    psp_build(&psp,&params); sft_init(); int21_set_psp(&psp); int21_set_sink(sink);
    int21_set_conin(con_get,NULL);
    devices_init(); int21_set_devices(devices_head(),NULL);
    (void)g_out; (void)g_shell_dta;
    if(!strcmp(argv[2],"--namespace")) {
        /* Raw INT21 paths avoid shell tokenization hiding illegal bytes.
         * Every request must refuse before any allocation or directory write. */
        const char *bad[]={"BAD?NAME","BAD*NAME","BAD+NAME","BAD,NAME",
            "BAD;NAME","BAD=NAME","BAD[NAME","BAD]NAME","BAD|NAME",
            "BAD NAME","BAD\x01NAME","TOOLONGNM.TXT","OK.LONG"};
        for(unsigned i=0;i<sizeof bad/sizeof *bad;i++) {
            for(unsigned j=0;j<3;j++) {
                unsigned ah=j==0?0x3900:j==1?0x3c00:0x5b00;
                if(call_host(ah,0,0,(uintptr_t)bad[i],0)>=0) {
                    fprintf(stderr,"RED shell-files: invalid raw name allocated\n");return 1;
                }
            }
        }
        const char *dev[]={"CON","PRN","AUX","NUL","CLOCK$",
            "A:\\SUB\\con.txt","AUX.TXT","SUB\\LPT1","LPT2.TXT","LPT3",
            "COM1","COM2.TXT","SUB\\COM3","COM4"};
        for(unsigned i=0;i<sizeof dev/sizeof *dev;i++) {
            if(call_host(0x3900,0,0,(uintptr_t)dev[i],0)>=0 ||
               call_host(0x5b00,0,0,(uintptr_t)dev[i],0)>=0) {
                fprintf(stderr,"RED shell-files: reserved raw name allocated\n");return 1;
            }
            int h=dos_open(dev[i]);
            if(h<0){fprintf(stderr,"RED shell-files: device alias did not open\n");return 1;}
            dos_close(h);
        }
        blockdev_file_close(&bf);
        puts("GREEN created-name matrix: raw 39h/3Ch/5Bh refusals, qualified devices and aliases");
        return 0;
    }
    FILE *in=fopen(argv[2],"r"); if(!in)return 2;
    char line[128];
    while(fgets(line,sizeof line,in)) {
        line[strcspn(line,"\r\n")]=0;
        cmd_line_t c;cmd_parse(line,&c);
        switch(c.kind) {
        case CMD_COPY: builtin_copy(c.arg);break;
        case CMD_REN: builtin_ren(c.arg);break;
        case CMD_DEL: builtin_del(c.arg);break;
        case CMD_CD: call_host(0x3b00,0,0,(uintptr_t)c.arg,0);break;
        case CMD_MD: call_host(0x3900,0,0,(uintptr_t)c.arg,0);break;
        default: fprintf(stderr,"RED shell-files: unsupported host command\n");return 2;
        }
    }
    fclose(in);blockdev_file_close(&bf);
    fprintf(stderr,"HOST-DONE verify_reads=%u\n",verify_reads);
    return 0;
}
