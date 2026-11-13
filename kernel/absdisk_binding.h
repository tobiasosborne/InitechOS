/* Production A: binding, separated from hardware startup for the host oracle.
 * FAT16 is read-only by product policy (include/fat12.h), as on C:/D:.
 */
#ifndef ABS_DISK_BINDING_H
#define ABS_DISK_BINDING_H
#include "fat12.h"
#include "int21.h"
#include "fileio_fat.h"
#include "block_device.h"
extern fat12_volume_t g_absdisk_vol;
int int21_absdisk_read(uint32_t, uint32_t, void *);
int int21_absdisk_write(uint32_t, uint32_t, const void *);
static void bind_absdisk_volume(const fat12_volume_t *vol, const blockdev_t *raw)
{
    int21_absdisk_backend_t backend;
    g_absdisk_vol = *vol;
    g_absdisk_vol.dev = raw;
    block_device_mounted(0x81,vol->base_lba,vol->total_sectors,
                         fileio_fat_absdisk_written);
    backend.read = int21_absdisk_read;
    backend.write = vol->fat_type == FAT_TYPE_FAT12 && raw->write_sectors
        ? int21_absdisk_write : 0;
    backend.total_sectors = vol->total_sectors;
    int21_set_blockdev(&backend);
}
#endif
