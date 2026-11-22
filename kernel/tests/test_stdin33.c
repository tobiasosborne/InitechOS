/* Replays fixtures/stdin33/stdin33.asm through the production INT 21h and
 * FAT12 backend: standard input and output are B:IN.TXT and B:ECHO.TXT by
 * DUP2. Writes RES.BIN's bytes to argv[2]. With argv[3] = 01, 07 or 08 it
 * then makes that call at end of file, which genuine DOS 3.30 never returns
 * from; the gate grades that with a timeout. */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "int21.h"
#include "fileio_fat.h"
#include "sft.h"
#include "psp.h"
#include "blockdev_file.h"

static int_frame_t call(unsigned ax, unsigned bx, unsigned cx, uintptr_t dx)
{
    int_frame_t f = {0};
    f.eax = ax; f.ebx = bx; f.ecx = cx; f.edx = (uint32_t)dx; f.eflags = 0x202;
    int21_dispatch(&f);
    return f;
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 4) return 2;
    blockdev_file_t disk; fat12_volume_t vol; uint8_t sector[512];
    psp_t psp; psp_params_t params = {0}; params.alloc_end_linear = 0x70000;
    if (blockdev_file_open_rw(&disk, argv[1]) || fat12_mount(&vol, &disk.dev, sector) ||
        fileio_fat_bind(&vol) || psp_build(&psp, &params)) return 2;
    sft_init(); int21_set_psp(&psp);
    char *low = mmap(0, 4096, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    if (low == MAP_FAILED) return 2;
    uint8_t res[16];
    unsigned n = 0;
    int_frame_t f = call(0x3000, 0, 0, 0);
    res[n++] = (uint8_t)f.eax; res[n++] = (uint8_t)(f.eax >> 8);
    strcpy(low, "IN.TXT");
    f = call(0x3d00, 0, 0, (uintptr_t)low);
    if (f.eflags & 1) return 2;
    unsigned h = f.eax & 65535;
    if (call(0x4600, h, 0, 0).eflags & 1) return 2;
    (void)call(0x3e00, h, 0, 0);
    strcpy(low, "ECHO.TXT");
    f = call(0x3c00, 0, 0, (uintptr_t)low);
    if (f.eflags & 1) return 2;
    h = f.eax & 65535;
    if (call(0x4600, h, 1, 0).eflags & 1) return 2;
    (void)call(0x3e00, h, 0, 0);
    static const uint8_t script[] = {0x01, 0x07, 0x08, 0x01, 0x07, 0x08, 0x01, 0x07};
    for (unsigned i = 0; i < sizeof script; i++)
        res[n++] = (uint8_t)call((unsigned)script[i] << 8, 0, 0, 0).eax;
    FILE *o = fopen(argv[2], "wb");
    if (!o || fwrite(res, 1, n, o) != n || fclose(o)) return 2;
    if (argc == 4) {
        unsigned ah = (unsigned)strtoul(argv[3], 0, 16);
        (void)call(ah << 8, 0, 0, 0);
        printf("AH=%02Xh returned at end of file\n", ah);
        return 1;
    }
    (void)call(0x3e00, 1, 0, 0);
    blockdev_file_close(&disk);
    return 0;
}
