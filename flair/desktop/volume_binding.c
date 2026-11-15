/* Capture the startup DOS default drive explicitly, independent of the last
 * backend selected by another service. Ref: int21.h drive/CWD contracts. */
#include "volume_binding.h"
#include "fileio_fat.h"
#include "int21.h"
#include "finder_storage.h"
int finder_bind_startup_volume(finder_volume_binding_t *out)
{
    int21_cwd_snapshot_t saved=int21_cwd_save();
    char root[]="A:\\";
    const char *path=root;
    out->volume=0;out->fat=0;out->fat_len=0;
    out->drive=int21_current_drive();
    root[0]=(char)('A'+out->drive);
    if(int21_select_path(&path)==0) {
        out->volume=fileio_fat_current_volume();
        out->fat=fileio_fat_fat_buffer(&out->fat_len);
        out->fat=finder_storage_read_cache(out->volume,out->fat);
    }
    int21_cwd_restore(&saved);
    return out->volume ? FAT12_OK : FAT12_ERR_PARTITION;
}
