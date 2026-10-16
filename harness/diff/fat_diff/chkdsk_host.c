/* Host adapter, not an oracle. Real MCB allocations make memory checks bite.
 * Ref: ADR-0003 DEC-03/B.3; os/milton/mcb.c. */
#include <stdio.h>
#include <stdlib.h>
#include "chkdsk.h"
#include "blockdev_file.h"
static void put(void *ctx, const char *s) { (void)ctx; fputs(s, stdout); }
static int get(void *ctx) { (void)ctx; return getchar(); }
int main(int argc, char **argv)
{
    blockdev_file_t bf;
    static uint8_t memory[4096];
    chkdsk_io_t io = {0};
    uint32_t p, largest;
    if (argc != 3 || blockdev_file_open_rw(&bf, argv[1])) return 2;
    io.disk = bf.dev; io.put = put; io.get = get;
    io.drive = 0; io.cwd = "\\";
    mcb_init(&io.arena, memory, sizeof(memory)/16);
    if (mcb_alloc(&io.arena, 0x4000, 16, &p, &largest)) return 2;
    /* 256 paras = 1 header + 16 owned + 1 header + 238 free.
     * Expected report: 4064 total usable bytes, 3808 free bytes. */
    int rc = chkdsk_run(&io, argv[2]);
    blockdev_file_close(&bf);
    return rc < 0 ? 2 : 0;
}
