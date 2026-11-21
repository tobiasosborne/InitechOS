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
static uint8_t bytes[512];
static psp_t process;
static unsigned critical_calls, critical_drive;

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

static void check_fixed_system(void)
{
    du_map map;
    void *c_fat = NULL, *a_fat = NULL;
    uint32_t c_size, a_size, selected_size;
    CHECK(fileio_fat_drive_volume(2, &c_fat, &c_size) && c_fat && c_size,
          "named C mount supplies the system FAT cache");
    CHECK(fileio_fat_fat_buffer(&selected_size) == c_fat && selected_size == c_size,
          "startup selects the C cache for CONFIG and EXEC");
    (void)call(0x0e00, 0, 0, 0);
    if (have_a) {
        CHECK(!block_device_map(0, &map) && map.disk == 0x81 && map.flags == 2,
              "ATA A is data, independently of running C");
        CHECK(int21_current_drive() == 0, "optional ATA data drive can be selected");
        CHECK(fileio_fat_drive_volume(0, &a_fat, &a_size) && a_fat && a_size &&
              a_fat != c_fat && a_size != c_size,
              "A and C have separately sized FAT caches");
        CHECK(fileio_fat_fat_buffer(&selected_size) == a_fat && selected_size == a_size,
              "drive selection uses the named A cache");
        read_file("ROOT.BIN", "legacy A bytes\n");
    } else {
        CHECK(int21_current_drive() == 2 && !fileio_fat_drive_volume(0, NULL, NULL),
              "absent A cannot replace default C");
    }
    CHECK(fileio_fat_system_drive() == 2, "AH=0Eh does not change system identity");
    CHECK(!block_device_map(2, &map) && map.disk == 0x80 && map.base == 63 && map.flags == 1,
          "the installed physical disk stays marked running after AH=0Eh");
    read_file("C:ROOT.BIN", "system C bytes\n");
    check_read_faults();
}

int main(int argc, char **argv)
{
    if (argc != 4) return 2;
    have_a = strcmp(argv[2], "-") != 0;
    unsigned want = (unsigned)atoi(argv[3]);
    if (blockdev_file_open_rw(&drives[0], argv[1])) return 2;
    if (have_a && blockdev_file_open_rw(&drives[1], argv[2])) return 2;
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
    CHECK(system, "production startup binds a working volume");
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
        if (want == 2) check_fixed_system();
    }
    CHECK(!drives[0].write_calls && (!have_a || !drives[1].write_calls),
          "startup and reads never write either disk");
    blockdev_file_close(&drives[0]);
    if (have_a) blockdev_file_close(&drives[1]);
    return TEST_SUMMARY("startup-disks");
}
