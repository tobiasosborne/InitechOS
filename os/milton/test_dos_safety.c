/* Host integration of the real INT21/FAT stack. The thin shell grades disk
 * bytes with mtools/cmp/fsck.fat. No artifact read-back can certify the disk.
 * Ref: MS-DOS 3.3 User's Reference p. 50; User's Reference p. 12;
 * IBM DOS 3.30 Technical Reference pp. 6-122/6-123 (CREAT).
 * Beads initech-vj28 / initech-wdzq; audit K01/K02. */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "int21.h"
#include "fileio_fat.h"
#include "fat12.h"
#include "sft.h"
#include "psp.h"
#include "blockdev_file.h"
#include "test_assert.h"
TEST_HARNESS();
static psp_t psp;
static uint16_t call_path(uint16_t ax, const char *path, int *error)
{
    char *low = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    if (low == MAP_FAILED || (uintptr_t)low > UINT32_MAX) exit(2);
    strcpy(low, path);
    int_frame_t f = {0};
    f.eax = ax; f.edx = (uint32_t)(uintptr_t)low; f.eflags = 0x202;
    int21_dispatch(&f);
    *error = (f.eflags & 1u) != 0;
    munmap(low, 4096);
    return (uint16_t)f.eax;
}
static void close_handle(uint16_t h)
{
    int_frame_t f = {0}; f.eax = 0x3e00; f.ebx = h;
    int21_dispatch(&f);
}
static void identity(const char *first, const char *second, int expected)
{
    int e1, e2;
    uint16_t a = call_path(0x3d00, first, &e1);
    uint16_t b = call_path(0x3d00, second, &e2);
    CHECK(!e1 && !e2, "identity operands open successfully");
    if (!e1 && !e2) {
        CHECK(int21_same_file(a, b) == expected, "resolved entry identity");
        close_handle(a); close_handle(b);
    }
}
static void reject_path(uint16_t ax_in, const char *path, const char *reason)
{
    int error;
    uint16_t ax = call_path(ax_in, path, &error);
    CHECK(error && ax == INT21_ERR_ACCESS_DENIED, reason);
    if (!error && (ax_in & 0xff00u) != 0x4100) close_handle(ax);
}
int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    static blockdev_file_t disk;
    static fat12_volume_t vol;
    uint8_t sector[512];
    CHECK(blockdev_file_open_rw(&disk, argv[1]) == 0, "open host image");
    CHECK(fat12_mount(&vol, &disk.dev, sector) == FAT12_OK, "mount host image");
    CHECK(fileio_fat_bind(&vol) == 0, "bind real FAT backend");
    sft_init();
    psp_params_t params = {0};
    params.alloc_end_linear = 0x70000;
    CHECK(psp_build(&psp, &params) == 0, "build kernel PSP");
    int21_set_psp(&psp);
    if (strcmp(argv[2], "identity") == 0) {
        identity("SELF.BIN", "A:\\SELF.BIN", 1);
        identity("SELF.BIN", "SUB\\..\\SELF.BIN", 1);
        identity("SUB\\SELF.BIN", "SELF.BIN", 0); /* identical bytes, different dir */
        identity("ZERO1", "ZERO2", 0); /* both cluster zero, different slots */
        identity("ZERO1", "A:\\ZERO1.", 1);
        identity("SUB\\SELF.BIN", "A:\\SUB\\DEEP\\..\\SELF.BIN", 1);
        CHECK(!int21_same_file(255, 255), "invalid handles are not identities");
    } else if (strcmp(argv[2], "create") == 0) {
        const char *paths[] = {"FILLED", "EMPTY", "SUB\\DEEP", "SAFETY"};
        for (unsigned i = 0; i < sizeof paths / sizeof paths[0]; i++)
            reject_path(0x3c00, paths[i], "CREAT rejects nonregular entry before freeing chain");
    } else if (strcmp(argv[2], "create-ro") == 0 ||
               strcmp(argv[2], "unlink-ro") == 0) {
        int creating = strcmp(argv[2], "create-ro") == 0;
        const char *paths[] = {"LOCK.BIN", "LOCK0.TXT", "SUB\\LOCK.BIN", "SUB\\LOCK0.TXT"};
        for (unsigned i = 0; i < sizeof paths / sizeof paths[0]; i++)
            reject_path(creating ? 0x3c00 : 0x4100, paths[i], creating ?
                "CREAT read-only refusal" : "UNLINK read-only refusal");
        if (!creating) {
            const char *other[] = {"FILLED", "EMPTY", "SUB\\DEEP", "SAFETY"};
            for (unsigned i = 0; i < sizeof other / sizeof other[0]; i++)
                reject_path(0x4100, other[i], "UNLINK nonregular refusal");
        }
    } else if (strcmp(argv[2], "write-ro") == 0) {
        reject_path(0x3d01, "LOCK.BIN", "OPEN write read-only refusal");
        reject_path(0x3d02, "SUB\\LOCK.BIN", "OPEN read/write read-only refusal");
        int error;
        uint16_t h = call_path(0x3d00, "LOCK.BIN", &error);
        CHECK(!error, "read-only file remains readable");
        if (!error) close_handle(h);
        uint8_t fat[4608], cluster[512], data = 0x7f;
        dir_entry_t de;
        uint32_t slot = 0, wrote = 99;
        CHECK(fat12_read_fat(&vol, fat, sizeof fat) == FAT12_OK, "read fixture FAT");
        CHECK(fat12_find_slot(&vol, sector, "LOCK.BIN", &de, &slot) == FAT12_OK, "locate protected entry");
        CHECK(fat12_write_partial(&vol, fat, sizeof fat, 0, slot, 0, &data, 1,
              sector, cluster, &wrote) == FAT12_ERR_ACCESS && wrote == 0,
              "positioned write read-only refusal");
        CHECK(fat12_write_file(&vol, fat, sizeof fat, slot, &data, 1,
              sector, cluster) == FAT12_ERR_ACCESS, "whole-file write read-only refusal");
        CHECK(fat12_write_file(&vol, fat, sizeof fat, slot, &data, 0,
              sector, cluster) == FAT12_ERR_ACCESS, "zero-length write read-only refusal");
        h = call_path(0x3d02, "KEEP.TXT", &error);
        CHECK(!error, "open unlocked write control");
        if (!error) {
            char *low = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
            if (low == MAP_FAILED || (uintptr_t)low > UINT32_MAX) exit(2);
            low[0] = 'X';
            CHECK(fat12_set_attr(&vol, fat, sizeof fat, "KEEP.TXT", 0,
                  DIR_ATTR_ARCHIVE | DIR_ATTR_READONLY, sector) == FAT12_OK, "lock already-open file");
            int_frame_t f = {0};
            f.eax = 0x4000; f.ebx = h; f.ecx = 1; f.edx = (uint32_t)(uintptr_t)low;
            int21_dispatch(&f);
            CHECK((f.eflags & 1u) && (uint16_t)f.eax == INT21_ERR_ACCESS_DENIED,
                  "AH40 already-open handle observes late lock");
            CHECK(fat12_set_attr(&vol, fat, sizeof fat, "KEEP.TXT", 0,
                  DIR_ATTR_ARCHIVE, sector) == FAT12_OK, "restore fixture attribute");
            close_handle(h);
            munmap(low, 4096);
        }
    } else return 2;
    blockdev_file_close(&disk);
    return TEST_SUMMARY("test_dos_safety");
}
