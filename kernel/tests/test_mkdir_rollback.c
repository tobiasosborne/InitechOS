/* Microsoft FAT specification (allocation conservation); CLAUDE.md, The
 * constitution. mtools owns the full-parent and one-free-cluster fixtures.
 * Compare pre-call metadata bytes, never expectations from a kernel decoder.
 * The shell independently checks named bytes, ownership and both FAT copies. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat12.h"
#include "blockdev_file.h"
#include "test_assert.h"
TEST_HARNESS();
static unsigned char fat[6144], before[6144], diskfat[6144];
static unsigned char sector[512], cluster[512], parent_before[512], parent_after[512];
int main(int argc, char **argv)
{
    blockdev_file_t disk;
    fat12_volume_t vol;
    dir_entry_t sub;
    if (argc != 3 || blockdev_file_open_rw(&disk, argv[1]) ||
        fat12_mount(&vol, &disk.dev, sector) ||
        fat12_read_fat(&vol, fat, sizeof fat) ||
        fat12_find(&vol, sector, "SUB", &sub)) return 2;
    unsigned bytes = vol.bpb.sectors_per_fat * 512u;
    unsigned lba = BPB_CLUSTER_LBA(&vol.bpb, sub.start_cluster);
    if (bytes > sizeof before || vol.bpb.sectors_per_cluster != 1 ||
        disk.dev.read_sectors(disk.dev.ctx, lba, 1, parent_before)) return 2;
    memcpy(before, fat, bytes);
    int flush = !strcmp(argv[2], "flush");
    /* Grow zero-fill + two FAT-copy writes, then own-cluster FAT flush. */
    if (flush) blockdev_file_arm_write_fault(&disk, 4);
    int rc = fat12_mkdir(&vol, fat, bytes, "NEWDIR", sub.start_cluster, sector, cluster);
    CHECK(rc == (flush ? FAT12_ERR_WRITE : FAT12_ERR_NO_SPACE),
          "failed MKDIR reports the reached flush/full condition");
    if (flush) CHECK(blockdev_file_write_faulted(&disk), "post-grow FAT flush fault fired");
    CHECK(!memcmp(before, fat, bytes), "failed MKDIR restores live allocation map");
    if (fat12_read_fat(&vol, diskfat, sizeof diskfat) ||
        disk.dev.read_sectors(disk.dev.ctx, lba, 1, parent_after)) return 2;
    CHECK(!memcmp(before, diskfat, bytes), "failed MKDIR restores persisted allocation map");
    CHECK(!memcmp(parent_before, parent_after, sizeof parent_before),
          "failed MKDIR preserves the full parent directory");
    blockdev_file_close(&disk);
    return TEST_SUMMARY("mkdir-rollback");
}
