/* FAT multi-sector-cluster differential driver (initech-fol7).
 * Ref: IBM DOS 3.30 TR pp2-31,4-17,5-8..9; spec/dos_structs.h.
 * Expected bytes come from mcopy, never the artifact's cluster arithmetic.
 * Guarded 512-byte scratch mirrors the kernel, including directory growth.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat12.h"
#include "blockdev_file.h"

struct scratch { unsigned char pre[16], data[512], post[16]; };
static struct scratch sector, cluster;
struct bad_bpb { blockdev_file_t *disk; unsigned char spc; };
static int bad_read(void *ctx, uint32_t lba, uint32_t count, void *buf)
{
    struct bad_bpb *bad = ctx;
    int rc = bad->disk->dev.read_sectors(bad->disk->dev.ctx, lba, count, buf);
    if (!rc && lba == 0 && count) ((unsigned char *)buf)[13] = bad->spc;
    return rc;
}
#define REQUIRE(c, msg) do { if (!(c)) { \
    fprintf(stderr, "RED fat-spc %s\n", msg); return 1; } } while (0)
#define OK(c) REQUIRE((c) == FAT12_OK, #c)
static int guarded(void)
{
    unsigned i;
    for (i = 0; i < 16; i++)
        if (sector.pre[i] != 0xa5 || sector.post[i] != 0xa5 ||
            cluster.pre[i] != 0xa5 || cluster.post[i] != 0xa5) return 0;
    return 1;
}
static int list_file(const dir_entry_t *entry, void *ctx)
{
    char name[13];
    if (!(entry->attribute & (DIR_ATTR_DIRECTORY | DIR_ATTR_VOLLABEL))) {
        if (fat12_format_83(entry, name) != FAT12_OK) return -1;
        if (fprintf(ctx, "%s %u\n", name, entry->file_size) < 0) return -1;
    }
    return 0;
}
int main(int argc, char **argv)
{
    blockdev_file_t disk;
    fat12_volume_t vol;
    fat12_fat_window_t win;
    unsigned char window[1024];
    unsigned char *fat, *input, *out;
    uint32_t fat_len, size, got, slot, bpc, i;
    dir_entry_t de;
    FILE *fp;
    int rc;
    REQUIRE(argc == 5, "usage: image input spc type");
    memset(&sector, 0xa5, sizeof(sector));
    memset(&cluster, 0xa5, sizeof(cluster));
    REQUIRE(!blockdev_file_open_rw(&disk, argv[1]), "open image");
    {
        static const unsigned char invalid[] = {3, 63, 65, 128, 255};
        struct bad_bpb bad = {&disk, 0};
        blockdev_t dev = disk.dev;
        fat12_volume_t rejected;
        dev.ctx = &bad; dev.read_sectors = bad_read;
        for (i = 0; i < sizeof(invalid); i++) {
            bad.spc = invalid[i];
            REQUIRE(fat12_mount(&rejected, &dev, sector.data) == FAT12_ERR_CLUSTER_SIZE,
                    "absurd spc must report FAT12_ERR_CLUSTER_SIZE");
        }
        bad.spc = 0;
        REQUIRE(fat12_mount(&rejected, &dev, sector.data) == FAT12_ERR_GEOMETRY,
                "zero spc must report FAT12_ERR_GEOMETRY");
    }
    rc = fat12_mount(&vol, &disk.dev, sector.data);
    if (rc) fprintf(stderr, "mount returned %d (spc=%s)\n", rc, argv[3]);
    REQUIRE(rc == FAT12_OK, "mount");
    REQUIRE(vol.bpb.sectors_per_cluster == atoi(argv[3]), "fixture spc");
    REQUIRE((int)vol.fat_type == atoi(argv[4]), "fixture FAT type");
    bpc = 512u * vol.bpb.sectors_per_cluster;
    fat_len = 512u * vol.bpb.sectors_per_fat;
    fat = malloc(fat_len);
    REQUIRE(fat != NULL, "allocate FAT");
    OK(fat12_read_fat(&vol, fat, fat_len));
    fp = fopen(argv[2], "rb");
    REQUIRE(fp != NULL, "open independent input");
    REQUIRE(!fseek(fp, 0, SEEK_END), "input seek");
    size = (uint32_t)ftell(fp);
    rewind(fp);
    input = malloc(size); out = malloc(size + 1u);
    REQUIRE(input && out && fread(input, 1, size, fp) == size, "input bytes");
    fclose(fp);
    OK(fat12_find(&vol, sector.data, "CHAIN.BIN", &de));
    for (i = 0; i < 2; i++) {
        /* Whole-FAT and the kernel's two-sector FAT window must agree. */
        if (i) {
            fat12_fat_window_init(&win, window, sizeof(window));
            vol.fat_window = &win;
        }
        memset(out, 0xa5, size + 1u);
        OK(fat12_read_file(&vol, fat, fat_len, &de, out, size,
                          cluster.data, &got));
        REQUIRE(got == size && !memcmp(out, input, size) && out[size] == 0xa5,
                "whole-file bytes vs mcopy");
        for (uint32_t off = 0; off < size + 1u; off += 509u) {
            uint32_t want = off < size ? size - off : 0u;
            if (want > bpc + 19u) want = bpc + 19u;
            memset(out, 0xa5, size + 1u);
            OK(fat12_read_partial(&vol, fat, fat_len, &de, off, bpc + 19u,
                                 out, cluster.data, &got));
            REQUIRE(got == want && !memcmp(out, input + (off < size ? off : size), want)
                    && out[want] == 0xa5, "partial bytes vs mcopy");
            REQUIRE(guarded(), "512-byte scratch canaries");
        }
    }
    vol.fat_window = NULL;
    blockdev_file_arm_read_fault(&disk, 2u);
    REQUIRE(fat12_read_file(&vol, fat, fat_len, &de, out, size,
                           cluster.data, &got) == FAT12_ERR_READ &&
            blockdev_file_read_faulted(&disk), "second sector read error propagated");
    blockdev_file_arm_read_fault(&disk, 0u);
    if (vol.fat_type == FAT_TYPE_FAT12) {
        unsigned char *before = malloc(fat_len);
        REQUIRE(before != NULL, "FAT rollback snapshot");
        OK(fat12_create(&vol, fat, fat_len, "FAULT.BIN", 0, 0,
                        sector.data, cluster.data, &de, &slot));
        memcpy(before, fat, fat_len);
        blockdev_file_arm_write_fault(&disk, 2u);
        REQUIRE(fat12_write_file(&vol, fat, fat_len, slot, input, size,
                                 sector.data, cluster.data) == FAT12_ERR_WRITE &&
                blockdev_file_write_faulted(&disk), "second sector write error propagated");
        blockdev_file_arm_write_fault(&disk, 0u);
        REQUIRE(!memcmp(before, fat, fat_len), "second sector write allocation rollback");
        free(before);
        OK(fat12_create(&vol, fat, fat_len, "WRITE.BIN", 0, 0,
                        sector.data, cluster.data, &de, &slot));
        OK(fat12_write_file(&vol, fat, fat_len, slot, input, size,
                            sector.data, cluster.data));
        OK(fat12_create(&vol, fat, fat_len, "PART.BIN", 0, 0,
                        sector.data, cluster.data, &de, &slot));
        OK(fat12_write_partial(&vol, fat, fat_len, 0, slot, 0, input, size,
                               sector.data, cluster.data, &got));
        OK(fat12_write_partial(&vol, fat, fat_len, 0, slot, bpc - 7u, input + 11u, 1031u,
                               sector.data, cluster.data, &got));
        OK(fat12_set_size(&vol, fat, fat_len, 0, slot, bpc + 13u,
                          sector.data, cluster.data));
        OK(fat12_write_partial(&vol, fat, fat_len, 0, slot, 2u*bpc + 29u, input, 777u,
                               sector.data, cluster.data, &got));
        OK(fat12_mkdir(&vol, fat, fat_len, "GROW", 0, sector.data, cluster.data));
        OK(fat12_find(&vol, sector.data, "GROW", &de));
        {
            uint16_t parent = de.start_cluster;
            /* Fill every sector, then extend the directory by one cluster. */
            for (i = 0; i < bpc/32u + 1u; i++) {
                char name[13];
                snprintf(name, sizeof(name), "F%07u", i);
                OK(fat12_create(&vol, fat, fat_len, name, 0, parent,
                                sector.data, cluster.data, &de, &slot));
                REQUIRE(guarded(), "directory grow scratch canaries");
            }
            OK(fat12_mkdir(&vol, fat, fat_len, "CHILD", parent,
                           sector.data, cluster.data));
            {
                char *path = malloc(strlen(argv[1]) + 6u);
                fat12_dir_t dir = {0, parent};
                REQUIRE(path != NULL, "listing path");
                sprintf(path, "%s.list", argv[1]);
                fp = fopen(path, "w");
                REQUIRE(fp != NULL, "listing output");
                OK(fat12_read_dir(&vol, &dir, sector.data, fat, fat_len,
                                  list_file, fp));
                REQUIRE(!fclose(fp), "close listing");
                free(path);
            }
        }
    }
    REQUIRE(guarded(), "final 512-byte scratch canaries");
    blockdev_file_close(&disk);
    free(out); free(input); free(fat);
    printf(">>> fat-spc driver: green (FAT%s spc=%s, whole/window/partial, scratch)\n",
           argv[4], argv[3]);
    return 0;
}
