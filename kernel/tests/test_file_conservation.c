/* Fault every physical metadata write, before and after it reaches the disk.
 * Expected state: the pre-operation volume, not another kernel result.
 * See CLAUDE.md, The constitution (oracle); initech-rg6d5. FAT layout:
 * Microsoft FAT specification, BPB/FAT12 and directory-entry definitions.
 * The shell gate independently reads the resulting images with mtools and a
 * separate FAT reader. No kernel decoder supplies its expected allocation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat12.h"
#include "blockdev_file.h"

static blockdev_file_t backing;
static blockdev_t device;
static unsigned calls, fail_at, fired;
static int fail_after;
static unsigned char fat[6144], original_fat[6144], sector[512], cluster[32768];

static int write_fault(void *ctx, uint32_t lba, uint32_t n, const void *p)
{
    (void)ctx;
    for (uint32_t i = 0; i < n; i++) {
        int hit = ++calls == fail_at;
        if ((!hit || fail_after) &&
            backing.dev.write_sectors(backing.dev.ctx, lba+i, 1,
                                      (const unsigned char *)p+i*512) != 0)
            return -1;
        if (hit) { fired++; return -1; }
    }
    return 0;
}

static void require(int ok, const char *what)
{
    if (!ok) { fprintf(stderr, "FAIL conservation: %s\n", what); exit(1); }
}

static void copy_image(const char *from, const char *to)
{
    unsigned char buf[8192];
    size_t n;
    FILE *in = fopen(from, "rb"), *out = fopen(to, "wb");
    require(in && out, "open image copy");
    while ((n = fread(buf, 1, sizeof buf, in)) != 0)
        require(fwrite(buf, 1, n, out) == n, "write image copy");
    require(!ferror(in), "read image copy");
    require(fclose(in) == 0 && fclose(out) == 0, "close image copy");
}

int main(int argc, char **argv)
{
    int failures = 0;
    require(argc == 4, "usage: fixture output-directory create|unlink");
    for (unsigned sub = 0; sub < 2; sub++) {
        unsigned total = 0;
        for (unsigned nth = 0; nth <= total; nth++) {
            for (int after = 0; after < (nth ? 2 : 1); after++) {
                char path[1024];
                fat12_volume_t vol;
                dir_entry_t entry;
                uint32_t slot, bytes;
                uint16_t parent = 0;
                int rc;
                snprintf(path, sizeof path, "%s/%s-%s-%u-%d.img", argv[2],
                         argv[3], sub ? "sub" : "root", nth, after);
                copy_image(argv[1], path);
                require(blockdev_file_open_rw(&backing, path) == 0, "open backend");
                device = backing.dev;
                device.write_sectors = write_fault;
                require(fat12_mount(&vol, &device, sector) == FAT12_OK, "mount");
                require(fat12_read_fat(&vol, fat, sizeof fat) == FAT12_OK, "read FAT");
                bytes = vol.bpb.sectors_per_fat * vol.bpb.bytes_per_sector;
                memcpy(original_fat, fat, bytes);
                if (sub) {
                    require(fat12_find(&vol, sector, "SUB", &entry) == FAT12_OK,
                            "find parent");
                    parent = entry.start_cluster;
                }
                calls = fired = 0; fail_at = nth; fail_after = after;
                if (!strcmp(argv[3], "create"))
                    rc = fat12_create(&vol, fat, bytes, "OLD.BIN", 0x20, parent,
                                      sector, cluster, &entry, &slot);
                else
                    rc = fat12_unlink(&vol, fat, bytes, "OLD.BIN", parent, sector);
                if (!nth) {
                    require(rc == FAT12_OK, "successful control");
                    total = calls;
                    require(total > 2, "control visits FAT copies and directory");
                } else {
                    require(fired == 1, "fault actually fired once");
                    require(rc == FAT12_ERR_WRITE, "injected write reports failure");
                    /* Do not let rereading the FAT hide a poisoned live allocator. */
                    if (memcmp(original_fat, fat, bytes) != 0) {
                        fprintf(stderr, "FAIL %s: live allocation map changed\n", path);
                        failures++;
                    }
                }
                blockdev_file_close(&backing);
            }
        }
        printf("PASS %s %s: %u physical write positions, before/after faults\n",
               argv[3], sub ? "subdirectory" : "root", total);
    }
    return failures != 0;
}
