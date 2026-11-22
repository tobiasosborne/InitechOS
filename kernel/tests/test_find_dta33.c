/* Replays fixtures/find-dta33/findta33.asm through the production INT 21h
 * and FAT12 backend and compares every step with the genuine MS-DOS 3.30
 * record, except DOS-internal DTA bytes 17-20 and the name's padding. The
 * disk is bound as A: here and was B: under DOS, so DTA byte 0 differs by 1. */
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
#include "test_assert.h"
TEST_HARNESS();

enum { FIRST = 1, NEXT = 2, COPY_NEXT = 3 };
static const struct { uint8_t op, dta; const char *spec; } script[] = {
    {FIRST, 0, "A*.*"}, {FIRST, 1, "B*.*"}, {NEXT, 0, 0}, {NEXT, 1, 0},
    {COPY_NEXT, 2, 0}, {NEXT, 0, 0}, {FIRST, 1, "\\SUB\\*.*"}, {NEXT, 0, 0},
    {NEXT, 1, 0}, {NEXT, 0, 0}, {NEXT, 1, 0}, {NEXT, 1, 0}, {NEXT, 1, 0},
    {NEXT, 2, 0}, {NEXT, 2, 0},
};

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    static uint8_t want[1024];
    FILE *r = fopen(argv[2], "rb");
    size_t have = r ? fread(want, 1, sizeof want, r) : 0;
    if (r) fclose(r);
    const unsigned steps = sizeof script / sizeof script[0];
    if (have != 2 + steps * 47u) return 2;
    blockdev_file_t disk; fat12_volume_t vol; uint8_t sector[512];
    psp_t psp; psp_params_t params = {0}; params.alloc_end_linear = 0x70000;
    if (blockdev_file_open_rw(&disk, argv[1]) || fat12_mount(&vol, &disk.dev, sector) ||
        fileio_fat_bind(&vol) || psp_build(&psp, &params)) return 2;
    sft_init(); int21_set_psp(&psp);
    uint8_t *low = mmap(0, 4096, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    if (low == MAP_FAILED) return 2;
    uint8_t *dta[3] = {low, low + 64, low + 128};
    char *spec = (char *)low + 256;
    memset(low, 0xcc, 192);
    CHECK(want[0] == 3 && want[1] == 30, "the reference is a DOS 3.30 capture");
    for (unsigned i = 0; i < steps; i++) {
        const uint8_t *w = want + 2 + i * 47u, *wd = w + 4;
        int_frame_t f = {0};
        f.eax = 0x1a00; f.edx = (uint32_t)(uintptr_t)dta[script[i].dta];
        int21_dispatch(&f);
        if (script[i].op == COPY_NEXT) memcpy(dta[2], dta[0], 43);
        f = (int_frame_t){0};
        f.eflags = 0x202;
        if (script[i].op == FIRST) {
            strcpy(spec, script[i].spec);
            f.eax = 0x4e00; f.ecx = 0x10; f.edx = (uint32_t)(uintptr_t)spec;
        } else f.eax = 0x4f00;
        int21_dispatch(&f);
        const uint8_t *d = dta[script[i].dta];
        unsigned ax = f.eax & 65535;
        size_t name = strnlen((const char *)wd + 30, 13);
        int same = w[0] == script[i].op && w[1] == (f.eflags & 1) &&
                   ax == (unsigned)(w[2] | w[3] << 8) &&
                   d[0] + 1 == wd[0] && !memcmp(d + 1, wd + 1, 16) &&
                   !memcmp(d + 21, wd + 21, 9) &&
                   name < 13 && !memcmp(d + 30, wd + 30, name + 1);
        if (!same) {
            printf("  step %u: CF=%u AX=%04x DTA", i, f.eflags & 1, ax);
            for (unsigned k = 0; k < 43; k++) printf(" %02x", d[k]);
            printf("\n  genuine: CF=%u AX=%04x DTA", w[1], w[2] | w[3] << 8);
            for (unsigned k = 0; k < 43; k++) printf(" %02x", wd[k]);
            printf("\n");
        }
        CHECK(same, "search step matches genuine DOS 3.30");
    }
    blockdev_file_close(&disk);
    munmap(low, 4096);
    return TEST_SUMMARY("find-dta33");
}
