/* initech-e9sx host oracle / fragmented fixture, authored before the loader.
 * Law 1: MS-DOS 3.3 User's Guide pp28 (visible COMMAND.COM), 46 (disk
 * reload), 119 (COPY separately from SYS); ADR-0003 Appendix A 3Dh/3Fh,
 * Appendix B.1; DEC-07a spec/mbr_partition_contract.json (volume at 63).
 * Expected bytes come from the separately linked interpreter, then mtype on
 * the identical image. The real artifact FAT reader must agree with both.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat12.h"
#include "blockdev_file.h"

static unsigned char disk[2944u * 512u + 1u], bytes[262144], want[262144];
static void need(int yes, const char *rule)
{
    if (!yes) { fprintf(stderr, "RED command-file %s\n", rule); exit(1); }
}
static unsigned get16(const unsigned char *p) { return p[0] | p[1] << 8; }
static unsigned next(const unsigned char *fat, unsigned c)
{
    unsigned n = get16(fat + c + c / 2u);
    return c & 1u ? n >> 4 : n & 4095u;
}
static void set(unsigned char *fat, unsigned c, unsigned value)
{
    unsigned off = c + c / 2u, n = get16(fat + off);
    n = c & 1u ? (n & 15u) | value << 4 : (n & 0xf000u) | value;
    fat[off] = (unsigned char)n; fat[off + 1u] = (unsigned char)(n >> 8);
}
int main(int argc, char **argv)
{
    blockdev_file_t file;
    fat12_volume_t vol;
    fat12_fat_window_t window;
    unsigned char sector[512], cache[1024], cluster[4096];
    dir_entry_t entry;
    uint32_t got, slot;
    FILE *f;
    size_t size, expected;
    if (argc != 4) return 2;
    need(blockdev_file_open(&file, argv[1]) == 0, "OPEN image");
    need(fat12_mount(&vol, &file.dev, sector) == 0 && vol.base_lba == 63u,
         "BASE DEC-07a boot partition");
    need(fat12_find_slot(&vol, sector, "COMMAND.COM", &entry, &slot) == 0,
         "NAME COMMAND.COM missing from boot root");
    need(entry.attribute == 0x20u, "ATTR visible writable archive file");
    need(entry.mtime == 0u && entry.mdate == 0x21u,
         "DATE SOURCE_DATE_EPOCH 1980-01-01");
    f = fopen(argv[2], "rb"); need(f != NULL, "OPEN reference");
    expected = fread(want, 1, sizeof want, f); fclose(f);
    need(expected > 44u && expected < sizeof want && entry.file_size == expected,
         "SIZE exact linked interpreter");
    fat12_fat_window_init(&window, cache, sizeof cache);
    vol.fat_window = &window;
    need(fat12_read_file(&vol, NULL, 0, &entry, bytes, sizeof bytes, cluster, &got) == 0,
         "CHAIN FAT file read");
    need(got == expected && !memcmp(bytes, want, expected), "BYTES != linked interpreter");
    blockdev_file_close(&file);
    f = fopen(argv[1], "rb"); need(f != NULL, "OPEN raw image");
    size = fread(disk, 1, sizeof disk, f); fclose(f);
    need(size == 2944u * 512u, "GEOMETRY 46 cylinders; no raw system area");
    if (!strcmp(argv[3], "fragment")) {
        /* Relocate every cluster to descending, nonadjacent FREE clusters.
         * Change both FATs and the root entry; leave the old bytes poisoned.
         * A raw-slot or contiguous-file reader cannot survive this fixture.
         */
        unsigned char *base = disk + vol.base_lba * 512u;
        unsigned char *fat = base + vol.first_fat_sector * 512u;
        unsigned c = entry.start_cluster, prev = 0, first = 0, n = 0;
        unsigned dest = vol.total_clusters;
        unsigned width = vol.bpb.sectors_per_cluster * 512u;
        while (n < got) {
            unsigned oldnext = next(fat, c);
            while (dest >= 2u && next(fat, dest)) dest -= 2u;
            need(dest >= 2u, "FRAGMENT free space");
            unsigned char *p = base + (vol.first_data_sector +
                                (dest - 2u) * vol.bpb.sectors_per_cluster) * 512u;
            unsigned len = got - n < width ? got - n : width;
            memset(p, 0, width); memcpy(p, bytes + n, len);
            memset(base + (vol.first_data_sector +
                   (c - 2u) * vol.bpb.sectors_per_cluster) * 512u, 0xa5, width);
            for (unsigned i = 0; i < 2u; ++i) {
                unsigned char *copy = fat + i * vol.bpb.sectors_per_fat * 512u;
                set(copy, c, 0); set(copy, dest, 4095u);
                if (prev) set(copy, prev, dest);
            }
            if (!first) first = dest;
            prev = dest; dest -= 2u; c = oldnext; n += len;
        }
        unsigned char *root = base + vol.root_dir_sector * 512u + slot * 32u;
        root[26] = (unsigned char)first; root[27] = (unsigned char)(first >> 8);
        f = fopen(argv[1], "wb"); need(f != NULL, "OPEN fragmented output");
        need(fwrite(disk, 1, size, f) == size && !fclose(f), "WRITE fragmented image");
        puts("GREEN command-file FRAGMENT: descending nonadjacent chain, old bytes poisoned");
    } else {
        need(!strcmp(argv[3], "check"), "MODE");
        printf("GREEN command-file: root COMMAND.COM attr=20, %u bytes == linked interpreter; FAT@63, 46 cylinders\n", got);
    }
    return 0;
}
