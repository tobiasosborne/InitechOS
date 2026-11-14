#ifndef INITECH_FINDER_STORAGE_H
#define INITECH_FINDER_STORAGE_H
#include "fat12.h"

/* fileio_fat_fat_buffer returns NULL for windowed volumes. The public FAT
 * walkers still require a non-NULL cache argument, although link decoding
 * uses vol->fat_window. Pass its real buffer without claiming a whole FAT. */
static inline void *finder_storage_read_cache(const fat12_volume_t *vol, void *fat)
{
    return !fat && vol && vol->fat_window ? vol->fat_window->buf : fat;
}

/* Operator ruling, 2026-11-14: FAT12 is read/write; FAT16 is read-only.
 * This is the Finder filesystem's single write-capability decision. Both
 * the user operation binding and Desktop DB/Trash persistence use it before
 * any FAT writer (including directory-only edits) can run. Reads use only
 * their existing geometry checks. See README.md for policy/message sources.
 */
static inline int finder_storage_write_check(const fat12_volume_t *vol)
{
    if (!vol) return FAT12_ERR_NULL;
    return vol->fat_type == FAT_TYPE_FAT12 ? FAT12_OK : FAT12_ERR_ACCESS;
}
#endif
