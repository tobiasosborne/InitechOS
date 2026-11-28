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
static unsigned checks;
static void require(int yes, const char *what)
{
    checks++;
    if (!yes) {
        fprintf(stderr,"FAIL config: %s\nINITECH_CHECKS %u failures=1\n",what,checks);
        exit(1);
    }
}
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
        require(!sysinit_shell_ready(), "unsupported SHELL refuses startup");
    } else if (argc==3 && !strcmp(argv[2],"ready")) {
        require(sysinit_shell_ready(), "supported SHELL permits startup");
    } else if (argc==3) {
        unsigned expected=(unsigned)strtoul(argv[2],0,10);
        unsigned expect_break, expect_drives, expect_ansi;
        if (sscanf(argv[2],"state:%2x%2x%2x:%u",
                   &expect_break,&expect_drives,&expected,&expect_ansi)==4) {
            int_frame_t frame={0};
            frame.eax=0x3300;
            int21_dispatch(&frame);
            require((frame.edx&255)==expect_break, "BREAK agrees with genuine DOS");
            frame.eax=0x0e00; frame.edx=0;
            int21_dispatch(&frame);
            require((frame.eax&255)==expect_drives, "LASTDRIVE agrees with genuine DOS");
            require(sysinit_ansi_enabled()==(int)expect_ansi, "ANSI selector changes console mode");
            require(sysinit_shell_ready(), "supported final SHELL permits startup");
        }
        unsigned count=0;
        uint16_t handles[20];
        for (;count<20;count++) {
            int_frame_t frame={0};
            frame.eax=0x3d00;
            frame.edx=(uint32_t)(uintptr_t)"COMMAND.COM";
            int21_dispatch(&frame);
            if (frame.eflags&1) {
                require((frame.eax&65535)==4, "exhaustion reports too many open files");
                break;
            }
            handles[count]=(uint16_t)frame.eax;
        }
        for (unsigned i=0;i<count;i++) {
            int_frame_t frame={0}; frame.eax=0x3e00; frame.ebx=handles[i];
            int21_dispatch(&frame);
        }
        require(count==expected, "open capacity agrees with genuine DOS");
    }
    blockdev_file_close(&disk);
    return ferror(stdout) ? 2 : 0;
}
