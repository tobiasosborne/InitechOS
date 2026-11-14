/* Run actual SYSINIT/FAT parsing; the host substitutes the console transport.
 * Expected wording comes from genuine DOS 3.30 boot captures, not this code. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysinit.h"
#include "blockdev_file.h"
#include "fileio_fat.h"
#include "sft.h"

static void console_putc(char c) { putchar((unsigned char)c); }
static void serial_discard(const char *s) { (void)s; }
int main(int argc,char **argv)
{
    blockdev_file_t disk;
    fat12_volume_t vol;
    uint8_t sector[512],fat[6144];
    psp_t process;
    psp_params_t params={0};
    if (argc<2 || argc>3 || blockdev_file_open_rw(&disk,argv[1]) ||
        fat12_mount(&vol,&disk.dev,sector) || fileio_fat_bind(&vol)) return 2;
    params.alloc_end_linear=0x70000;
    if (psp_build(&process,&params)) return 2;
    sft_init(); int21_set_psp(&process); int21_set_sink(console_putc);
    (void)sysinit_apply_config(&vol,sector,fat,sizeof fat,serial_discard);
    if (argc==3 && !strcmp(argv[2],"refused")) {
        if (sysinit_shell_ready()) return 1;
    } else if (argc==3 && !strcmp(argv[2],"ready")) {
        if (!sysinit_shell_ready()) return 1;
    } else if (argc==3) {
        unsigned count=0;
        uint16_t handles[20];
        for (;count<20;count++) {
            int_frame_t frame={0};
            frame.eax=0x3d00;
            frame.edx=(uint32_t)(uintptr_t)"COMMAND.COM";
            int21_dispatch(&frame);
            if (frame.eflags&1) {
                if ((frame.eax&65535)!=4) return 1;
                break;
            }
            handles[count]=(uint16_t)frame.eax;
        }
        for (unsigned i=0;i<count;i++) {
            int_frame_t frame={0}; frame.eax=0x3e00; frame.ebx=handles[i];
            int21_dispatch(&frame);
        }
        if (count!=strtoul(argv[2],0,10)) {
            fprintf(stderr,"open capacity %u differs from genuine DOS %s\n",count,argv[2]);
            return 1;
        }
    }
    blockdev_file_close(&disk);
    return ferror(stdout) ? 2 : 0;
}
