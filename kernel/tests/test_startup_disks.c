/* GM8 startup layout, exercised over independently minted mtools volumes.
 * Only the ATA hardware transport and serial output are supplied by the host. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ata.h"
#include "block_device.h"
#include "fileio_fat.h"
#include "int21.h"
#include "psp.h"
#include "sft.h"
#include "startup_disks.h"
#include "blockdev_file.h"
#include "test_assert.h"

TEST_HARNESS();

static blockdev_file_t drives[2];
static blockdev_t missing;
static int have_a;
static char path[128];
static uint8_t bytes[1024];
static psp_t process;
static unsigned critical_calls, critical_drive;
static uint32_t fault_lba;
static int faulted;
static int (*master_read)(void *, uint32_t, uint32_t, void *);

static int read_with_fault(void *ctx, uint32_t lba, uint32_t count, void *buf)
{
    if (!faulted && fault_lba >= lba && fault_lba - lba < count) {
        faulted = 1;
        return ATA_ERR_DEVICE;
    }
    return master_read(ctx, lba, count, buf);
}

void serial_puts(const char *s) { (void)s; }
void serial_putc(char c) { (void)c; }
void serial_puti(int32_t n) { (void)n; }

void __wrap_ata_ctx_init_primary_master(ata_ctx_t *ctx)
{
    memset(ctx, 0, sizeof *ctx);
    ctx->drive_select = ATA_DRIVE_MASTER_LBA;
}

void __wrap_ata_ctx_init_primary_slave(ata_ctx_t *ctx)
{
    memset(ctx, 0, sizeof *ctx);
    ctx->drive_select = ATA_DRIVE_SLAVE_LBA;
}

void __wrap_ata_blockdev_init(blockdev_t *dev, ata_ctx_t *ctx)
{
    if (ctx->drive_select == ATA_DRIVE_MASTER_LBA) *dev = drives[0].dev;
    else *dev = have_a ? drives[1].dev : missing;
}

static int absent_read(void *ctx, uint32_t lba, uint32_t count, void *buf)
{
    (void)ctx; (void)lba; (void)count; (void)buf;
    return ATA_ERR_NO_DRIVE;
}

static uint32_t sectors(blockdev_file_t *drive)
{
    if (fseek(drive->fp, 0, SEEK_END)) return 0;
    long size = ftell(drive->fp);
    return size > 0 ? (uint32_t)(size / 512) : 0;
}

static int_frame_t call(unsigned ax, unsigned bx, unsigned cx, uintptr_t dx)
{
    int_frame_t f = {0};
    f.eax = ax;
    f.ebx = bx;
    f.ecx = cx;
    f.edx = (uint32_t)dx;
    int21_dispatch(&f);
    return f;
}

static int open_file(const char *name)
{
    strcpy(path, name);
    int_frame_t f = call(0x3d00, 0, 0, (uintptr_t)path);
    CHECK(!(f.eflags & 1), "volume opens its root payload");
    return f.eflags & 1 ? -1 : (int)(f.eax & 65535);
}

static void read_file(const char *name, const char *expected)
{
    int h = open_file(name);
    if (h < 0) return;
    int_frame_t f = call(0x3f00, (unsigned)h, sizeof bytes, (uintptr_t)bytes);
    CHECK(!(f.eflags & 1) && f.eax == strlen(expected) &&
          !memcmp(bytes, expected, strlen(expected)),
          "file read returns the independently minted root payload");
    f = call(0x3e00, (unsigned)h, 0, 0);
    CHECK(!(f.eflags & 1), "root file closes");
}

static int fail_critical(int write, int error)
{
    CHECK(!write && error < 0, "critical handler sees the injected read error");
    critical_calls++;
    critical_drive = int21_io_drive();
    return CRIT_BLOCKDEV_FAIL;
}

/* MS-DOS 4.0 SYSINIT2.ASM INT24 answers Fail without a prompt until the
 * command processor runs; the boot console cannot be read before then. */
static unsigned key_reads;
static char screen[256];
static unsigned screen_len;

static void screen_sink(char c)
{
    if (screen_len < sizeof screen - 1) screen[screen_len++] = c;
}

/* Answers F so that a premature prompt ends; the read count grades it. */
static int scripted_key(void)
{
    key_reads++;
    return 'F';
}

static int no_typeahead(void) { return -1; }

static void check_startup_prompts(void)
{
    int h = open_file("C:ROOT.BIN");
    if (h < 0) return;
    int21_set_sink(screen_sink);
    int21_set_conin(scripted_key, no_typeahead);
    blockdev_file_arm_read_fault(&drives[0], 1);
    int_frame_t f = call(0x3f00, (unsigned)h, sizeof bytes, (uintptr_t)bytes);
    CHECK((f.eflags & 1) && blockdev_file_read_faulted(&drives[0]) && !key_reads,
          "a C read error during startup fails without an operator prompt");
    blockdev_file_arm_read_fault(&drives[0], 0);

    /* Once input is live the production handler prompts and honours F. */
    key_reads = 0;
    screen_len = 0;
    startup_disks_enable_prompts();
    blockdev_file_arm_read_fault(&drives[0], 1);
    f = call(0x4200, (unsigned)h, 0, 0);
    f = call(0x3f00, (unsigned)h, sizeof bytes, (uintptr_t)bytes);
    screen[screen_len] = 0;
    CHECK((f.eflags & 1) && blockdev_file_read_faulted(&drives[0]) && key_reads == 1 &&
          strstr(screen, "Abort, Retry, Fail?"),
          "after input is live a C read error prompts once and F fails the read");
    blockdev_file_arm_read_fault(&drives[0], 0);
    crit_blockdev_set_hook(0);
    int21_set_conin(0, 0);
    int21_set_sink(0);
    (void)call(0x3e00, (unsigned)h, 0, 0);
}

static void check_read_faults(void)
{
    int h = open_file("C:ROOT.BIN");
    if (h < 0) return;
    crit_blockdev_set_hook(fail_critical);
    blockdev_file_arm_read_fault(&drives[0], 1);
    int_frame_t f = call(0x3f00, (unsigned)h, sizeof bytes, (uintptr_t)bytes);
    CHECK((f.eflags & 1) && blockdev_file_read_faulted(&drives[0]) &&
          critical_calls == 1 && critical_drive == 2,
          "C file read errors invoke the critical handler with the I/O drive");
    blockdev_file_arm_read_fault(&drives[0], 0);
    (void)call(0x3e00, (unsigned)h, 0, 0);

    critical_calls = 0;
    blockdev_file_arm_read_fault(&drives[0], 1);
    f = (int_frame_t){0};
    f.eax = 2;
    f.ebx = (uintptr_t)bytes;
    f.ecx = 1;
    int25_dispatch(&f);
    CHECK((f.eflags & 1) && blockdev_file_read_faulted(&drives[0]) && !critical_calls,
          "raw INT25 on C reports its error without invoking the critical handler");
    blockdev_file_arm_read_fault(&drives[0], 0);
    crit_blockdev_set_hook(0);
}

/* BPB byte 21 read straight from the mtools-minted image file. */
static int image_media(const char *image, long vbr)
{
    FILE *fp = fopen(image, "rb");
    int media = fp && !fseek(fp, vbr + 21, SEEK_SET) ? fgetc(fp) : -1;
    if (fp) fclose(fp);
    return media;
}

static const char *c_image, *a_image;

static int allocation_media(unsigned dl)
{
    int_frame_t f = call(0x1c00, 0, 0, dl);
    if ((f.eax & 255) == 255 || !f.ebx) return -1;
    return *(const uint8_t *)(uintptr_t)f.ebx;
}

static void check_fixed_system(int mounted_a)
{
    du_map map;
    void *c_fat = NULL, *a_fat = NULL;
    uint32_t c_size, a_size, selected_size;
    CHECK(fileio_fat_drive_volume(2, &c_fat, &c_size) && c_fat && c_size,
          "named C mount supplies the system FAT cache");
    CHECK(fileio_fat_fat_buffer(&selected_size) == c_fat && selected_size == c_size,
          "startup selects the C cache for CONFIG and EXEC");
    (void)call(0x0e00, 0, 0, 0);
    if (mounted_a) {
        CHECK(!block_device_map(0, &map) && map.disk == 0x81 && map.flags == 2,
              "ATA A is data, independently of running C");
        CHECK(int21_current_drive() == 0, "optional ATA data drive can be selected");
        CHECK(fileio_fat_drive_volume(0, &a_fat, &a_size) && a_fat && a_size &&
              a_fat != c_fat && a_size != c_size,
              "A and C have separately sized FAT caches");
        CHECK(fileio_fat_fat_buffer(&selected_size) == a_fat && selected_size == a_size,
              "drive selection uses the named A cache");
        read_file("ROOT.BIN", "legacy A bytes\n");
        int a_media = allocation_media(1), c_media = allocation_media(3);
        CHECK(a_media == image_media(a_image, 0) && c_media == image_media(c_image, 63 * 512) &&
              a_media != c_media,
              "AH=1Ch reports each drive's own BPB media byte");
    } else {
        CHECK(int21_current_drive() == 2 && !fileio_fat_drive_volume(0, NULL, NULL),
              "an unmounted A cannot replace default C");
    }
    CHECK(fileio_fat_system_drive() == 2, "AH=0Eh does not change system identity");
    CHECK(!block_device_map(2, &map) && map.disk == 0x80 && map.base == 63 && map.flags == 1,
          "the installed physical disk stays marked running after AH=0Eh");
    read_file("C:ROOT.BIN", "system C bytes\n");
    check_startup_prompts();
    check_read_faults();
}

static void check_failed_data_probe(unsigned letter, uint32_t base)
{
    unsigned disk = letter == 0 ? 0x81 : 0x80;
    du_map map;
    CHECK(letter == 0 ? blockdev_file_read_faulted(&drives[1]) : faulted,
          "the boot-sector read fault fired");
    CHECK(!fileio_fat_drive_volume(letter, NULL, NULL), "failed probe leaves data unmounted");
    CHECK(!block_device_map(letter, &map) && map.disk == disk && map.base == base,
          "the data extent can still be mapped");
    uint32_t lba = base ? base - 1 : 0;
    unsigned cyl = lba / (16 * 63), head = lba / 63 % 16, sec = lba % 63 + 1;
    int_frame_t f = {0};
    f.eax = base ? 0x0302 : 0x0301;
    f.ebx = (uintptr_t)bytes;
    f.ecx = ((cyl & 255) << 8) | ((cyl >> 2) & 192) | sec;
    f.edx = (head << 8) | disk;
    memset(bytes, 0xa5, sizeof bytes);
    int13_dispatch(&f);
    CHECK((f.eflags & 1) && ((f.eax >> 8) & 255) == 3,
          "BIOS returns write protection for a request reaching unreadable FAT16 data");
}

/* mtools will not create these tables; GM2 forbids FAT12 writes reaching
 * FAT16 sectors. Neither overlapping fixed partition may be published. */
static void check_overlap_refused(const fat12_volume_t *system, uint32_t d_base)
{
    CHECK(!system, "startup refuses a disk whose C and D partitions overlap");
    CHECK(!fileio_fat_drive_volume(2, NULL, NULL) && !fileio_fat_drive_volume(3, NULL, NULL),
          "neither overlapping fixed partition is mounted");
    memset(bytes, 0xa5, sizeof bytes);
    int_frame_t f = {0};
    f.eax = 2;
    f.ebx = (uintptr_t)bytes;
    f.ecx = 1;
    f.edx = d_base - 63; /* C-relative sector holding D's boot record */
    int26_dispatch(&f);
    CHECK(f.eflags & 1, "INT26 on C cannot write the FAT16 boot record it overlaps");
    unsigned cyl = d_base / (16 * 63), head = d_base / 63 % 16, sec = d_base % 63 + 1;
    f = (int_frame_t){0};
    f.eax = 0x0301;
    f.ebx = (uintptr_t)bytes;
    f.ecx = ((cyl & 255) << 8) | ((cyl >> 2) & 192) | sec;
    f.edx = (head << 8) | 0x80;
    int13_dispatch(&f);
    CHECK((f.eflags & 1) && ((f.eax >> 8) & 255) == 3,
          "BIOS write protection covers the shared extent");
    f.ecx = 0x0002; /* LBA 64, head 1 sector 2: C's first FAT, outside D */
    f.edx = 0x0180;
    int13_dispatch(&f);
    CHECK((f.eflags & 1) && ((f.eax >> 8) & 255) == 3,
          "BIOS write protection covers the rest of the refused C extent");
}

int main(int argc, char **argv)
{
    if (argc != 4 && argc != 5) return 2;
    have_a = strcmp(argv[2], "-") != 0;
    c_image = argv[1];
    a_image = argv[2];
    unsigned want = (unsigned)atoi(argv[3]);
    if (blockdev_file_open_rw(&drives[0], argv[1])) return 2;
    if (have_a && blockdev_file_open_rw(&drives[1], argv[2])) return 2;
    int fail_a = argc == 5 && !strcmp(argv[4], "fail-a");
    int overlap = argc == 5 && !strcmp(argv[4], "overlap");
    int c_fault = argc == 5 && !strncmp(argv[4], "c-fault=", 8);
    if (c_fault) {
        fault_lba = (uint32_t)strtoul(argv[4] + 8, NULL, 10);
        if (!have_a) return 2;
        master_read = drives[0].dev.read_sectors;
        drives[0].dev.read_sectors = read_with_fault;
        faulted = !fault_lba; /* c-fault=0: a clean C without system files */
    } else if (overlap) {
        if (have_a) return 2;
    } else if (fail_a) {
        if (!have_a) return 2;
        /* The superfloppy map reads sector zero once before FAT mounts it. */
        blockdev_file_arm_read_fault(&drives[1], 2);
    } else if (argc == 5) {
        fault_lba = (uint32_t)strtoul(argv[4], NULL, 10);
        if (!fault_lba) return 2;
        master_read = drives[0].dev.read_sectors;
        drives[0].dev.read_sectors = read_with_fault;
    }
    missing = (blockdev_t){0, absent_read, 0};
    block_device_bind(0, &drives[0].dev, sectors(&drives[0]));
    block_device_bind(1, have_a ? &drives[1].dev : &missing,
                      have_a ? sectors(&drives[1]) : 0);

    psp_params_t params = {0};
    params.alloc_end_linear = 0x80000;
    if (psp_build(&process, &params)) return 2;
    sft_init();
    int21_set_psp(&process);

    const fat12_volume_t *system = startup_disks_init();
    if (overlap) {
        check_overlap_refused(system, 8255);
        system = 0;
    } else if (c_fault) {
        CHECK(faulted, "the injected C read fault fired");
        CHECK(!system && !fileio_fat_drive_volume(0, NULL, NULL),
              "only a cleanly read old C layout may select data A");
        fault_lba = 0;
        system = 0;
    } else {
        CHECK(system, "production startup binds a working volume");
    }
    if (system) {
        CHECK(int21_current_drive() == want && int21_io_drive() == want,
              "default and I/O drive select the installed system");
        CHECK(fileio_fat_system_drive() == (int)want, "resident system drive identifies the installed volume");
        CHECK(system->base_lba == (want == 2 ? 63u : 0u),
              "system mount has the independently minted origin");
        CHECK(fileio_fat_drive_volume(want, NULL, NULL) == system,
              "named mount matches the startup result");
        read_file("ROOT.BIN", want == 2 ? "system C bytes\n" : "legacy A bytes\n");
        du_map map;
        CHECK(!block_device_map(want, &map) && (map.flags & 1),
              "the actual system pair is marked running");
        if (want == 2) check_fixed_system(have_a && !fail_a);
    }
    if (fault_lba) check_failed_data_probe(3, fault_lba);
    if (fail_a) check_failed_data_probe(0, 0);
    CHECK(!drives[0].write_calls && (!have_a || !drives[1].write_calls),
          "startup, reads and refused requests never write either disk");
    blockdev_file_close(&drives[0]);
    if (have_a) blockdev_file_close(&drives[1]);
    return TEST_SUMMARY("startup-disks");
}
