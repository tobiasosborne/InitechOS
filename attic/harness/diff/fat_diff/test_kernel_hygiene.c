/* MILTON FAT regressions, on images independently exercised by mtools.
 * Ref: ADR-0003 Appendix A 39h/43h/56h, B.1; IBM DOS 3.30 Technical
 * Reference pp. 6-145, 6-206; mmd/mren/mattrib on identical seed images.
 * The GROW directory is the LAST data cluster; every free cluster is below it.
 */
#include <stdio.h>
#include <string.h>
#include "test_assert.h"
#include "blockdev_file.h"
#include "desktop_db.h"
TEST_HARNESS();
static unsigned char fat[6144], sector[512], cluster[512], before[512];
int main(int argc, char **argv)
{
    blockdev_file_t bf;
    fat12_volume_t v;
    dir_entry_t de;
    uint32_t slot, len;
    desktop_trash_state_t state;
    if (argc != 3) return 2;
    CHECK(blockdev_file_open_rw(&bf, argv[2]) == 0, "open crafted image");
    CHECK(fat12_mount(&v, &bf.dev, sector) == 0, "mount crafted image");
    CHECK(fat12_read_fat(&v, fat, sizeof(fat)) == 0, "read FAT");
    len = v.bpb.sectors_per_fat * 512u;
    if (!strcmp(argv[1], "rename")) {
        CHECK(fat12_find(&v, sector, "OLD", &de) == 0, "OLD exists");
        CHECK(fat12_read_sectors(&v, BPB_CLUSTER_LBA(&v.bpb, de.start_cluster),
                                 1, before) == 0, "snapshot dot/dotdot");
        uint16_t start = de.start_cluster;
        CHECK(fat12_rename(&v, fat, len, "OLD", "NEW", 0, sector) == 0,
              "f0hh same-directory DIRECTORY rename succeeds");
        CHECK(fat12_read_sectors(&v, BPB_CLUSTER_LBA(&v.bpb, start),
                                 1, cluster) == 0 && !memcmp(before, cluster, 512),
              "directory contents including dotdot unchanged");
        CHECK(fat12_rename(&v, fat, len, "KHYGIENE", "LABEL2", 0, sector)
              == FAT12_ERR_ACCESS, "volume label rename still rejected");
    } else if (!strcmp(argv[1], "grow")) {
        CHECK(fat12_find(&v, sector, "GROW", &de) == 0, "GROW exists");
        CHECK(de.start_cluster == v.total_clusters + 1u,
              "crafted directory tail is last cluster; all free space precedes it");
        uint16_t parent = de.start_cluster;
        for (unsigned i = 0; i < 15; ++i) {
            char name[13];
            snprintf(name, sizeof(name), "F%02u.TXT", i);
            CHECK(fat12_create(&v, fat, len, name, DIR_ATTR_ARCHIVE,
                               parent, sector, cluster, &de, &slot) == 0,
                  "fov8 directory grows using earlier free clusters");
        }
    } else if (!strcmp(argv[1], "hidden")) {
        CHECK(desktop_trash_ensure(&v, fat, len, sector, cluster, &state) == 0,
              "create runtime TRASH");
        CHECK(fat12_find(&v, sector, "TRASH", &de) == 0 && de.attribute == 0x12,
              "tdnl.30 runtime TRASH attribute is DIRECTORY|HIDDEN");
        CHECK(fat12_set_attr(&v, fat, len, "TRASH", 0, 2, sector)
              == FAT12_ERR_ACCESS, "CHMOD still rejects a directory target");
        CHECK(desktop_trash_ensure(&v, fat, len, sector, cluster, &state) == 0
              && state == DESKTOP_TRASH_OK, "existing TRASH ensure is idempotent");
    } else return 2;
    blockdev_file_close(&bf);
    return TEST_SUMMARY("test_kernel_hygiene");
}
