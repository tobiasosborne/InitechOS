/* Private Finder identity. MTE, Finder Interface: a file specification names
 * its volume as well as its parent directory. FAT cluster numbers are local
 * to a volume. This is resident Finder state, not a new kernel service.
 * DESKTOP.DB retains its locked 16-bit on-disk cluster fields per volume. */
#ifndef INITECH_FINDER_IDENTITY_H
#define INITECH_FINDER_IDENTITY_H
#include <stdint.h>
typedef uint32_t finder_dir_id_t;
#define FINDER_DIR_TRASH UINT32_C(0x7fffffff)
#define FINDER_DIR_INVALID UINT32_MAX
#define FINDER_VOLUME_SLOTS 2u
static inline finder_dir_id_t finder_dir_make(unsigned drive,uint16_t cluster)
{
    return drive<26u ? ((uint32_t)(drive+1u)<<16)|cluster : FINDER_DIR_INVALID;
}
static inline unsigned finder_dir_drive(finder_dir_id_t dir,unsigned legacy_drive)
{
    unsigned tag=(unsigned)(dir>>16);
    return tag ? tag-1u : legacy_drive;
}
static inline uint16_t finder_dir_cluster(finder_dir_id_t dir)
{
    return (uint16_t)dir;
}
static inline int finder_dir_is_root(finder_dir_id_t dir)
{
    return finder_dir_cluster(dir)==0u;
}
#endif
