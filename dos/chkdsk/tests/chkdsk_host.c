/* Host adapter, not an oracle. It stands in for the kernel's OEM AH=F9h
 * summary with figures from a real local MCB arena (kernel/mcb.c), the same
 * accounting the kernel service performs: total data bytes exclude headers.
 * Ref: ADR-0003 DEC-03/B.3; kernel/README.md (OEM functions). */
#include <stdio.h>
#include <stdlib.h>
#include "chkdsk.h"
#include "blockdev_file.h"
#include "mcb.h"
static void put(void *ctx, const char *s) { (void)ctx; fputs(s, stdout); }
static int get(void *ctx) { (void)ctx; return getchar(); }
static int cwd(unsigned drive, char out[64])
{ if (drive) return -1; out[0] = 0; return 0; }   /* A: only, at the root */
static mcb_arena_t arena;
static int memory(uint32_t *total, uint32_t *free_bytes)
{
    /* Adapter fault injection, outside the shipped product: an absent
     * arena summary (product AH=F9h CF=1/AX=7) must fail the report. */
    if (getenv("CHKDSK_FAIL_MEMORY")) return -1;
    uint32_t free_paras, largest, headers=0, at=0;
    if (!mcb_stats(&arena, &free_paras, &largest)) return -1;
    while (at < arena.total_paras) {
        const mcb_t *m = (const mcb_t *)(arena.base + at*16u);
        headers++; at += 1u + m->size_paras;
    }
    *total = (arena.total_paras - headers)*16u;
    *free_bytes = free_paras*16u;
    return 0;
}
int main(int argc, char **argv)
{
    blockdev_file_t bf;
    static uint8_t arena_mem[4096];
    chkdsk_io_t io = {0};
    uint32_t p, largest;
    if (argc != 3 || blockdev_file_open_rw(&bf, argv[1])) return 2;
    io.disk = bf.dev; io.put = put; io.get = get;
    io.drive = 0; io.cwd = cwd;
    mcb_init(&arena, arena_mem, sizeof(arena_mem)/16);
    if (mcb_alloc(&arena, 0x4000, 16, &p, &largest)) return 2;
    io.memory = memory;
    /* 256 paras = 1 header + 16 owned + 1 header + 238 free.
     * Expected report: 4064 total usable bytes, 3808 free bytes. */
    int rc = chkdsk_run(&io, argv[2]);
    blockdev_file_close(&bf);
    if(getenv("CHKDSK_REPORT_ONLY")) {
        fprintf(stderr,"chkdsk result=%d\n",rc);return 0;
    }
    return rc < 0 ? 2 : 0;
}
