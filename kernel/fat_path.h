#ifndef INITECH_FAT_PATH_H
#define INITECH_FAT_PATH_H

#include "fat12.h"

/* Resolve a directory's on-disk identity to root-relative 8.3 components. */
uint16_t fat_directory_path(const fat12_volume_t *vol, void *sector,
                            const void *fat, uint32_t fat_len,
                            uint16_t dir_cluster, char *canon, uint32_t canon_max);

#endif
