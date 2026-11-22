/* Bind the Finder to the installed system volume: the kernel's initial
 * binding (fileio_fat_system_drive), independent of the DOS default drive
 * the user may have changed before typing FLAIR and of the last backend
 * another service selected. fileio_fat_drive_volume names that volume and
 * its own FAT cache without selecting it, so no CWD, default drive or I/O
 * selection is touched. Ref: kernel/include/fileio_fat.h; C activation
 * plan step 8; accepted GM8 delivery design (C system, optional A data). */
#include "volume_binding.h"
#include "fileio_fat.h"
#include "finder_storage.h"
int finder_bind_startup_volume(finder_volume_binding_t *out)
{
    int sys=fileio_fat_system_drive();
    void *fat=0;
    uint32_t fat_len=0;
    out->volume=0;out->fat=0;out->fat_len=0;out->drive=0;
    if(sys<0) return FAT12_ERR_PARTITION;
    out->drive=(unsigned)sys;
    out->volume=fileio_fat_drive_volume((unsigned)sys,&fat,&fat_len);
    if(!out->volume) return FAT12_ERR_PARTITION;
    out->fat=finder_storage_read_cache(out->volume,fat);
    out->fat_len=fat_len;
    return FAT12_OK;
}
