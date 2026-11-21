/* GM8 delivery design: fixed C: system, optional ATA A: data.
 * Disk transports, absolute I/O and policy retain the legacy startup wiring.
 * The mount sequence is shared by production and the mtools host fixture. */
#include "startup_disks.h"
#include "ata.h"
#include "block_device.h"
#include "fileio_fat.h"
#include "int21.h"
#include "absdisk_binding.h"

extern void serial_puts(const char *);
extern void serial_putc(char);
extern void serial_puti(int32_t);

static ata_ctx_t fixed_master;
static blockdev_t fixed_dev;
static crit_blockdev_t fixed_crit;
static fat12_volume_t fixed_volumes[2];
static int fixed_mounted[2];
static uint8_t sector[BLOCKDEV_SECTOR_SIZE];
static void bind_fixed_absdisk(unsigned letter, int mounted);

fat12_volume_t g_absdisk_vol;

int int21_absdisk_read(uint32_t lba, uint32_t count, void *buf)
{
    return fat12_read_sectors(&g_absdisk_vol, lba, count, buf);
}

int int21_absdisk_write(uint32_t lba, uint32_t count, const void *buf)
{
    int rc = fat12_write_sectors(&g_absdisk_vol, lba, count, buf);
    if (!rc) fileio_fat_absdisk_written(lba, count, buf);
    return rc;
}

static int mount_dos_volume(unsigned letter, fat12_volume_t *vol,
                            const blockdev_t *dev, uint8_t *scratch)
{
    du_map map;
    if (block_device_map(letter, &map)) return FAT12_ERR_PARTITION;
    int rc = map.partition ? fat12_mount_primary(vol, dev, scratch, map.partition)
                           : fat12_mount(vol, dev, scratch);
    if (!rc && (vol->base_lba != map.base || vol->total_sectors > map.sectors))
        return FAT12_ERR_PARTITION;
    return rc;
}

static void mount_fixed_volume(unsigned letter)
{
    fat12_volume_t *vol = &fixed_volumes[letter - 2];
    /* A raw metadata notification must not start a file-I/O INT24 exchange.
     * Mount/cache through the raw transport, then attach the file driver. */
    int rc = mount_dos_volume(letter, vol, &fixed_dev, sector);
    if (!rc) rc = fileio_fat_add_drive(letter, vol);
    if (!rc) vol->dev = &fixed_crit.dev;
    else fileio_fat_drop_drive(letter);
    fixed_mounted[letter - 2] = !rc;
    bind_fixed_absdisk(letter, !rc);
    block_device_volume_policy(letter, 0x80, vol->base_lba,
        rc ? 0 : vol->extent_sectors, !rc && vol->fat_type == FAT_TYPE_FAT16);
    serial_puts(rc ? "DOS-VOLUME-FAIL " : "DOS-VOLUME-OK ");
    serial_putc((char)('A' + letter));
    serial_puts(" rc=");
    serial_puti(rc);
    serial_putc('\n');
}

static void fixed_volume_written(unsigned letter, uint32_t lba, uint32_t count,
                                 const void *buf)
{
    if (!lba) mount_fixed_volume(letter);
    else fileio_fat_raw_written(letter, lba, count, buf);
}

static int fixed_read(unsigned letter, uint32_t lba, uint32_t count, void *buf)
{
    fat12_volume_t raw = fixed_volumes[letter - 2];
    raw.dev = &fixed_dev;
    return fat12_read_sectors(&raw, lba, count, buf);
}

static int fixed_write(unsigned letter, uint32_t lba, uint32_t count, const void *buf)
{
    fat12_volume_t raw = fixed_volumes[letter - 2];
    raw.dev = &fixed_dev;
    int rc = fat12_write_sectors(&raw, lba, count, buf);
    if (!rc) fixed_volume_written(letter, lba, count, buf);
    return rc;
}

static int abs_read_c(uint32_t lba, uint32_t count, void *buf)
{
    return fixed_read(2, lba, count, buf);
}

static int abs_read_d(uint32_t lba, uint32_t count, void *buf)
{
    return fixed_read(3, lba, count, buf);
}

static int abs_write_c(uint32_t lba, uint32_t count, const void *buf)
{
    return fixed_write(2, lba, count, buf);
}

static int abs_write_d(uint32_t lba, uint32_t count, const void *buf)
{
    return fixed_write(3, lba, count, buf);
}

static void bind_fixed_absdisk(unsigned letter, int mounted)
{
    if (!mounted) {
        int21_set_absdisk_drive(letter, 0);
        return;
    }
    const fat12_volume_t *vol = &fixed_volumes[letter - 2];
    int21_absdisk_backend_t backend;
    backend.read = letter == 2 ? abs_read_c : abs_read_d;
    backend.write = vol->fat_type == FAT_TYPE_FAT12 && fixed_dev.write_sectors
        ? (letter == 2 ? abs_write_c : abs_write_d) : 0;
    backend.total_sectors = vol->total_sectors;
    int21_set_absdisk_drive(letter, &backend);
}

const fat12_volume_t *startup_disks_init(void)
{
    static ata_ctx_t slave;
    static blockdev_t data_dev;
    static crit_blockdev_t data_crit;
    static fat12_volume_t data_vol;

    crit_blockdev_set_hook(0); /* Discovering optional media does not prompt. */
    ata_ctx_init_primary_slave(&slave);
    ata_blockdev_init(&data_dev, &slave);
    crit_blockdev_init(&data_crit, &data_dev);
    int have_data = mount_dos_volume(0, &data_vol, &data_crit.dev, sector) == FAT12_OK;

    ata_ctx_init_primary_master(&fixed_master);
    ata_blockdev_init(&fixed_dev, &fixed_master);
    crit_blockdev_init(&fixed_crit, &fixed_dev);
    for (unsigned letter = 2; letter < 4; letter++) {
        du_map map;
        if (block_device_map(letter, &map)) continue;
        mount_fixed_volume(letter);
        block_device_watch(letter, &map, fixed_volume_written);
    }

    fat12_volume_t *system = 0;
    unsigned drive = 2;
    dir_entry_t command;
    if (fixed_mounted[0] && fixed_volumes[0].fat_type == FAT_TYPE_FAT12 &&
        fat12_find(&fixed_volumes[0], sector, "COMMAND.COM", &command) == FAT12_OK &&
        !(command.attribute & (DIR_ATTR_DIRECTORY | DIR_ATTR_VOLLABEL)))
        system = &fixed_volumes[0];
    /* Temporary fallback for old images with only IO/INITDOS on C.
     * Remove at C activation plan step 21 (initech-tlhqy). */
    if (!system && have_data) {
        system = &data_vol;
        drive = 0;
    }
    if (!system) return 0;
    int rc = fileio_fat_bind_drive(drive, system);
    if (rc) {
        serial_puts("FILEIO-BIND-FAIL rc=");
        serial_puti(rc);
        serial_putc('\n');
        return 0;
    }
    if (have_data) {
        int added = drive == 0 ? 0 : fileio_fat_add_drive(0, &data_vol);
        if (!added) bind_absdisk_volume(&data_vol, &data_dev);
    }
    for (unsigned letter = 2; letter < 4; letter++) {
        if (fixed_mounted[letter - 2] && system != &fixed_volumes[letter - 2]) {
            rc = fileio_fat_add_drive(letter, &fixed_volumes[letter - 2]);
            if (rc) {
                fileio_fat_drop_drive(letter);
                bind_fixed_absdisk(letter, 0);
            }
        }
    }
    int_frame_t select = {0};
    select.eax = 0x0e00;
    select.edx = drive;
    int21_dispatch(&select);
    /* A's cache watch also sets the running pair. Record the actual system. */
    block_device_running(drive == 2 ? 0x80 : 0x81, system->base_lba);
    crit_blockdev_set_hook(int21_run_critical_error);
    serial_puts("FILEIO-BIND-OK\nABSDISK-BIND-OK\n");
    return system;
}
