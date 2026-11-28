/* Read both FAT12 copies before the factory FRAGMENT injector writes to
 * its fixed destinations. IBM DOS 3.30 Technical Reference, pp. 5-7..11. */
#include <stdio.h>
#include <stdlib.h>

enum { SECTOR = 512, PARTITION_LBA = 63, MAX_FAT_SECTORS = 16 };

static unsigned le16(const unsigned char *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static unsigned le32(const unsigned char *p)
{
    return le16(p) | le16(p + 2) << 16;
}

static void read_at(FILE *disk, long offset, void *buf, size_t size)
{
    if (fseek(disk, offset, SEEK_SET) || fread(buf, 1, size, disk) != size) {
        fprintf(stderr, "ERROR boot FRAGMENT: unreadable image at %ld\n", offset);
        exit(2);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    FILE *disk = fopen(argv[1], "rb");
    if (!disk) { perror(argv[1]); return 2; }

    const long volume = PARTITION_LBA * SECTOR;
    unsigned char bpb[SECTOR], root[64], fat[2][MAX_FAT_SECTORS * SECTOR];
    read_at(disk, volume, bpb, sizeof bpb);
    unsigned cluster_sectors = bpb[13], reserved = le16(bpb + 14);
    unsigned fat_sectors = le16(bpb + 22), sectors = le16(bpb + 19);
    unsigned root_sector = reserved + 2 * fat_sectors;
    unsigned data_sector = root_sector + (le16(bpb + 17) * 32 + SECTOR - 1) / SECTOR;
    if (le16(bpb + 11) != SECTOR || !cluster_sectors || !reserved || bpb[16] != 2 ||
        !fat_sectors || fat_sectors > MAX_FAT_SECTORS || data_sector >= sectors) {
        fprintf(stderr, "ERROR boot FRAGMENT: invalid FAT12 BPB\n");
        return 2;
    }
    read_at(disk, volume + root_sector * SECTOR, root, sizeof root);
    for (unsigned copy = 0; copy < 2; copy++)
        read_at(disk, volume + (reserved + copy * fat_sectors) * SECTOR,
                fat[copy], fat_sectors * SECTOR);
    fclose(disk);

    unsigned clusters = (sectors - data_sector) / cluster_sectors;
    unsigned cluster_bytes = cluster_sectors * SECTOR;
    const unsigned starts[] = {700, 1000};
    for (unsigned file = 0; file < 2; file++) {
        unsigned length = le32(root + file * 32 + 28);
        if (!length) return 2;
        unsigned count = 1 + (length - 1) / cluster_bytes;
        for (unsigned i = 0; i < count; i++) {
            unsigned dest = starts[file] + 2 * i, offset = dest + dest / 2;
            if (dest >= clusters + 2 || offset + 1 >= fat_sectors * SECTOR ||
                (file == 0 && dest >= starts[1])) {
                fprintf(stderr, "ERROR boot FRAGMENT: destination %u out of range\n", dest);
                return 2;
            }
            for (unsigned copy = 0; copy < 2; copy++) {
                unsigned entry = le16(fat[copy] + offset);
                if ((dest & 1 ? entry >> 4 : entry & 0xfff) != 0) {
                    fprintf(stderr, "RED boot FRAGMENT: destination %u allocated in FAT %u\n",
                            dest, copy + 1);
                    return 1;
                }
            }
        }
    }
    puts("GREEN boot FRAGMENT: all destinations free in both FATs");
    return 0;
}
