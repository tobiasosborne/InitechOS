/* Replays fixtures/ioctl33/ioctl33.asm through the production INT 21h and
 * FAT12 backend and writes RES.BIN's bytes to argv[2]. The disk is drive A
 * here and B: in the capture, so the file word's drive field (bits 0-5) is
 * rewritten to 1 before the gate compares it. */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "int21.h"
#include "fileio_fat.h"
#include "sft.h"
#include "psp.h"
#include "blockdev_file.h"
#include "devices.h"

static int_frame_t call(unsigned ax, unsigned bx, uintptr_t dx)
{
    int_frame_t f = {0};
    f.eax = ax; f.ebx = bx; f.edx = (uint32_t)dx; f.eflags = 0x202;
    int21_dispatch(&f);
    return f;
}

static unsigned n;
static uint8_t res[64];

static void query(unsigned h, int file)
{
    int_frame_t f = call(0x4400, h, 0);
    unsigned dx = f.edx & 0xffffu;
    if (file && !(f.eflags & 1)) dx = (dx & ~0x3fu) | 1u;
    res[n++] = (uint8_t)(f.eflags & 1);
    res[n++] = (uint8_t)dx;
    res[n++] = (uint8_t)(dx >> 8);
}

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    blockdev_file_t disk; fat12_volume_t vol; uint8_t sector[512];
    psp_t psp; psp_params_t params = {0}; params.alloc_end_linear = 0x70000;
    if (blockdev_file_open_rw(&disk, argv[1]) || fat12_mount(&vol, &disk.dev, sector) ||
        fileio_fat_bind(&vol) || psp_build(&psp, &params)) return 2;
    sft_init(); int21_set_psp(&psp);
    devices_init(); int21_set_devices(devices_head(), 0);
    char *low = mmap(0, 4096, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    if (low == MAP_FAILED) return 2;
    int_frame_t f = call(0x3000, 0, 0);
    res[n++] = (uint8_t)f.eax; res[n++] = (uint8_t)(f.eax >> 8);
    for (unsigned h = 0; h < 5; h++) query(h, 0);
    static const char *const names[] = {"CON", "NUL", "AUX", "PRN", "CLOCK$", "FILE.TXT"};
    for (unsigned i = 0; i < 6; i++) {
        strcpy(low, names[i]);
        f = call(0x3d02, 0, (uintptr_t)low);
        if (f.eflags & 1) f = call(0x3d00, 0, (uintptr_t)low);
        if (f.eflags & 1) {
            res[n++] = 0xee; res[n++] = 0; res[n++] = 0;
            continue;
        }
        query(f.eax & 0xffffu, i == 5);
        (void)call(0x3e00, f.eax & 0xffffu, 0);
    }
    blockdev_file_close(&disk);
    FILE *o = fopen(argv[2], "wb");
    return !o || fwrite(res, 1, n, o) != n || fclose(o) ? 2 : 0;
}
