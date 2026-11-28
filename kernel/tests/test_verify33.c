/* Replays fixtures/verify33/verify33.asm through the production INT 21h
 * and writes RES.BIN's bytes to argv[2]. Then checks VERIFY's effect on
 * writes through the FAT12 backend with a disk that silently drops writes:
 * with VERIFY off AH=40h reports success, as DOS does, and with VERIFY on
 * the readback fails the write. A good disk still writes with VERIFY on. */
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

static int_frame_t call(unsigned ax, unsigned bx, unsigned cx, uintptr_t dx)
{
    int_frame_t f = {0};
    f.eax = ax; f.ebx = bx; f.ecx = cx; f.edx = (uint32_t)dx; f.eflags = 0x202;
    int21_dispatch(&f);
    return f;
}

static blockdev_file_t disk;
static int drop_writes;
static int rd(void *c, uint32_t l, uint32_t n, void *b) { (void)c; return disk.dev.read_sectors(disk.dev.ctx, l, n, b); }
static int wr(void *c, uint32_t l, uint32_t n, const void *b)
{
    (void)c;
    return drop_writes ? 0 : disk.dev.write_sectors(disk.dev.ctx, l, n, b);
}

static int failures;
static void need(int ok, const char *what)
{
    if (!ok) { printf("FAIL: %s\n", what); failures++; }
}

/* Create NAME and write four bytes, dropping the disk writes of AH=40h
 * alone when `drop` is set; returns the AH=40h frame. */
static int_frame_t write_new(char *low, const char *name, int drop)
{
    strcpy(low, name);
    int_frame_t f = call(0x3c00, 0, 0, (uintptr_t)low);
    need(!(f.eflags & 1), "create");
    unsigned h = f.eax & 0xffffu;
    memcpy(low + 32, "data", 4);
    drop_writes = drop;
    int_frame_t w = call(0x4000, h, 4, (uintptr_t)(low + 32));
    drop_writes = 0;
    (void)call(0x3e00, h, 0, 0);
    return w;
}

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    fat12_volume_t vol; uint8_t sector[512];
    psp_t psp; psp_params_t params = {0}; params.alloc_end_linear = 0x70000;
    if (blockdev_file_open_rw(&disk, argv[1])) return 2;
    blockdev_t dev = {0, rd, wr};
    if (fat12_mount(&vol, &dev, sector) || fileio_fat_bind(&vol) || psp_build(&psp, &params)) return 2;
    sft_init(); int21_set_psp(&psp);
    char *low = mmap(0, 4096, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    if (low == MAP_FAILED) return 2;

    uint8_t res[32];
    unsigned n = 0;
    int_frame_t f = call(0x3000, 0, 0, 0);
    res[n++] = (uint8_t)f.eax; res[n++] = (uint8_t)(f.eax >> 8);
    res[n++] = (uint8_t)call(0x5400, 0, 0, 0).eax;
    static const uint8_t script[] = {0x01, 0x00, 0x02, 0x03, 0xff, 0x80, 0x01};
    for (unsigned i = 0; i < sizeof script; i++) {
        res[n++] = (uint8_t)call(0x2e00u | script[i], 0, 0, 0).eax;
        res[n++] = (uint8_t)call(0x5400, 0, 0, 0).eax;
    }
    FILE *o = fopen(argv[2], "wb");
    if (!o || fwrite(res, 1, n, o) != n || fclose(o)) return 2;

    (void)call(0x2e00, 0, 0, 0);
    f = write_new(low, "LOST.TXT", 1);
    need(!(f.eflags & 1) && (f.eax & 0xffffu) == 4u, "VERIFY OFF reports a dropped write as written");
    (void)call(0x2e01, 0, 0, 0);
    f = write_new(low, "CHECKED.TXT", 1);
    need(f.eflags & 1, "VERIFY ON fails a write that does not read back");
    f = write_new(low, "GOOD.TXT", 0);
    need(!(f.eflags & 1) && (f.eax & 0xffffu) == 4u, "VERIFY ON writes a good disk");
    blockdev_file_close(&disk);
    return failures ? 1 : 0;
}
