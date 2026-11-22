/* Replays fixtures/create-attrs33/creat33.asm through the production INT 21h
 * and FAT12 file backend and writes the same record layout. The gate compares
 * it with the genuine MS-DOS 3.30 RESULT.BIN and reads the disk with mtools. */
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

static uint8_t out[512];
static unsigned at;
static char *name;

static int_frame_t call(unsigned ax, unsigned bx, unsigned cx, uintptr_t dx)
{
    int_frame_t f = {0};
    f.eax = ax; f.ebx = bx; f.ecx = cx; f.edx = (uint32_t)dx; f.eflags = 0x202;
    int21_dispatch(&f);
    return f;
}

static void put(uint8_t b) { if (at < sizeof out) out[at++] = b; }
static void record(int_frame_t f, uint32_t value)
{
    put((uint8_t)(f.eflags & 1)); put((uint8_t)value); put((uint8_t)(value >> 8));
}
static void flag(int_frame_t f) { put((uint8_t)(f.eflags & 1)); }
static void named(const char *two) { name[1] = two[0]; name[2] = two[1]; }

static void create_any(unsigned ah, unsigned cx)
{
    int_frame_t f = call(ah << 8, 0, cx, (uintptr_t)name);
    record(f, f.eax);
    if (!(f.eflags & 1)) (void)call(0x3e00, f.eax & 65535, 0, 0);
}

static void get_attr(void)
{
    int_frame_t f = call(0x4300, 0, 0, (uintptr_t)name);
    record(f, f.ecx);
}

int main(int argc, char **argv)
{
    static const struct { uint8_t cx; char n[3]; } matrix[] = {
        {0x00, "00"}, {0x01, "01"}, {0x02, "02"}, {0x04, "04"}, {0x07, "07"},
        {0x20, "20"}, {0x27, "27"}, {0x08, "08"}, {0x10, "10"}, {0x40, "40"},
        {0x80, "80"}, {0x18, "18"},
    };
    if (argc != 3) return 2;
    blockdev_file_t disk; fat12_volume_t vol; uint8_t sector[512];
    psp_t psp; psp_params_t params = {0}; params.alloc_end_linear = 0x70000;
    if (blockdev_file_open_rw(&disk, argv[1]) || fat12_mount(&vol, &disk.dev, sector) ||
        fileio_fat_bind(&vol) || psp_build(&psp, &params)) return 2;
    sft_init(); int21_set_psp(&psp);
    uint8_t *low = mmap(0, 4096, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    if (low == MAP_FAILED) return 2;
    name = (char *)low;
    strcpy(name, "F00");
    uint8_t *payload = low + 64, *buf = low + 128;
    memcpy(payload, "abc", 3);

    int_frame_t f = call(0x3000, 0, 0, 0);
    put((uint8_t)f.eax); put((uint8_t)(f.eax >> 8));
    for (unsigned i = 0; i < sizeof matrix / sizeof matrix[0]; i++) {
        named(matrix[i].n);
        f = call(0x3c00, 0, matrix[i].cx, (uintptr_t)name);
        record(f, f.eax);
        if (f.eflags & 1) {
            for (unsigned k = 0; k < 13; k++) put(0xee);
            continue;
        }
        unsigned h = f.eax & 65535;
        f = call(0x4000, h, 3, (uintptr_t)payload); record(f, f.eax);
        f = call(0x4200, h, 0, 0); flag(f);
        memset(buf, 0, 3);
        f = call(0x3f00, h, 3, (uintptr_t)buf); record(f, f.eax);
        put(buf[0]); put(buf[1]); put(buf[2]);
        (void)call(0x3e00, h, 0, 0);
        get_attr();
    }
    named("01");
    create_any(0x3c, 0);
    f = call(0x3d01, 0, 0, (uintptr_t)name); record(f, f.eax);
    if (!(f.eflags & 1)) (void)call(0x3e00, f.eax & 65535, 0, 0);
    f = call(0x3d00, 0, 0, (uintptr_t)name); record(f, f.eax);
    if (!(f.eflags & 1)) (void)call(0x3e00, f.eax & 65535, 0, 0);
    named("02"); create_any(0x3c, 0); get_attr();
    named("00"); create_any(0x3c, 1); get_attr();
    named("N3");
    f = call(0x5b00, 0, 3, (uintptr_t)name); record(f, f.eax);
    if (!(f.eflags & 1)) {
        unsigned h = f.eax & 65535;
        f = call(0x4000, h, 3, (uintptr_t)payload); record(f, f.eax);
        (void)call(0x4200, h, 0, 0);
        memset(buf, 0, 3);
        f = call(0x3f00, h, 3, (uintptr_t)buf); record(f, f.eax);
        put(buf[0]); put(buf[1]); put(buf[2]);
        (void)call(0x3e00, h, 0, 0);
    }
    get_attr();
    create_any(0x5b, 0);
    named("02"); create_any(0x3c, 2); get_attr();
    named("04"); create_any(0x3c, 0); create_any(0x3c, 4); get_attr();
    named("N8"); create_any(0x5b, 8);
    named("NX"); create_any(0x5b, 0x10);
    named("H1"); create_any(0x3c, 0x100); get_attr();

    blockdev_file_close(&disk);
    FILE *o = fopen(argv[2], "wb");
    if (!o || fwrite(out, 1, at, o) != at || fclose(o)) return 2;
    munmap(low, 4096);
    return 0;
}
