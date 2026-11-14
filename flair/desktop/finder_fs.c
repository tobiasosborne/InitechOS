/* Finder filesystem binding, shared by all Finder operation routes.
 * Read algorithms/geometry: kernel/include/fat12.h and README.md.
 * Write capability: finder_storage.h, operator ruling 2026-11-14. */
#include "finder_actions.h"
#include "desktop_db.h"
#include "finder_storage.h"
extern void serial_puts(const char *);
int finder_fat_mkdir(void *fs_user, const char *name83,
                            uint16_t parent_dir_start)
{
    int rc;

    (void)fs_user;
    if (finder_storage_write_check(g_finder_vol) == FAT12_ERR_ACCESS)
        return FINDER_WIN_ERR_WRITEPROTECT;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MKDIR;

    rc = fat12_mkdir(g_finder_vol, g_finder_fat, g_finder_fat_len,
                     name83, parent_dir_start,
                     g_finder_dirbuf, g_finder_clusbuf);
    if (rc == FAT12_OK)             return (int)FINDER_WIN_OK;
    if (rc == FAT12_ERR_DIR_FULL)   return (int)FINDER_WIN_ERR_DIRFULL;
    if (rc == FAT12_ERR_NO_SPACE)   return (int)FINDER_WIN_ERR_DISKFULL;
    return (int)FINDER_WIN_ERR_MKDIR;
}

/* Internal to COLD: this utility is never a resident entry or callback. */
static int finder_fat_map(int rc)
{
    if (rc == FAT12_OK)            return (int)FINDER_WIN_OK;
    if (rc == FAT12_ERR_SAME_DIR)  return (int)FINDER_WIN_ERR_SAMEDIR;
    if (rc == FAT12_ERR_CYCLE)     return (int)FINDER_WIN_ERR_CYCLE;
    if (rc == FAT12_ERR_EXISTS)    return (int)FINDER_WIN_ERR_EXISTS;
    if (rc == FAT12_ERR_NOT_EMPTY) return (int)FINDER_WIN_ERR_NOTEMPTY;
    if (rc == FAT12_ERR_NO_SPACE)  return (int)FINDER_WIN_ERR_DISKFULL;
    return (int)FINDER_WIN_ERR_MOVE;
}

int finder_fat_move(void *fs_user, const char *src_name83,
                           uint16_t src_dir, const char *dst_name83,
                           uint16_t dst_dir)
{
    (void)fs_user;
    if (finder_storage_write_check(g_finder_vol) == FAT12_ERR_ACCESS)
        return FINDER_WIN_ERR_WRITEPROTECT;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MOVE;
    return finder_fat_map(fat12_move_dirent(g_finder_vol, g_finder_fat,
                                            g_finder_fat_len, src_name83,
                                            src_dir, dst_name83, dst_dir,
                                            g_finder_dirbuf,
                                            g_finder_clusbuf));
}

int finder_fat_trash_name(void *fs_user, const char *name83,
                                 uint16_t dir, char *out)
{
    (void)fs_user;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MOVE;
    return finder_fat_map(fat12_trash_suffix_name(g_finder_vol, g_finder_fat,
                                                  g_finder_fat_len, dir,
                                                  g_finder_dirbuf, name83,
                                                  out));
}

int finder_trash_reserve(void)
{
    desktop_trash_state_t state = DESKTOP_TRASH_OK;
    int rc;

    if (!finder_fat_geometry_ok()) return FAT12_ERR_NULL;
    rc = finder_storage_write_check(g_finder_vol);
    if (rc != FAT12_OK) return rc;
    rc = desktop_trash_ensure(g_finder_vol, g_finder_fat, g_finder_fat_len,
                              g_finder_dirbuf, g_finder_clusbuf, &state);
    if (rc == FAT12_OK) {
        if (state == DESKTOP_TRASH_CREATE) serial_puts("TRASH-CREATE\n");
    } else {
        serial_puts("TRASH-RESERVE-FAIL rc=");
        serial_puti((int32_t)rc);
        serial_putc('\n');
    }
    return rc;
}

int finder_fat_unlink(void *fs_user, const char *name83, uint16_t dir)
{
    (void)fs_user;
    if (finder_storage_write_check(g_finder_vol) == FAT12_ERR_ACCESS)
        return FINDER_WIN_ERR_WRITEPROTECT;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MOVE;
    return finder_fat_map(fat12_unlink(g_finder_vol, g_finder_fat,
                                       g_finder_fat_len, name83, dir,
                                       g_finder_dirbuf));
}

int finder_fat_rmdir(void *fs_user, const char *name83, uint16_t dir)
{
    (void)fs_user;
    if (finder_storage_write_check(g_finder_vol) == FAT12_ERR_ACCESS)
        return FINDER_WIN_ERR_WRITEPROTECT;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MOVE;
    return finder_fat_map(fat12_rmdir(g_finder_vol, g_finder_fat,
                                      g_finder_fat_len, name83, dir,
                                      g_finder_dirbuf));
}

int finder_fat_create(void *fs_user, const char *name83, uint16_t dir,
                             uint32_t *out_slot)
{
    dir_entry_t e;
    (void)fs_user;
    if (finder_storage_write_check(g_finder_vol) == FAT12_ERR_ACCESS)
        return FINDER_WIN_ERR_WRITEPROTECT;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MOVE;
    return finder_fat_map(fat12_create(g_finder_vol, g_finder_fat,
                                       g_finder_fat_len, name83,
                                       (uint8_t)DIR_ATTR_ARCHIVE, dir,
                                       g_finder_dirbuf, g_finder_clusbuf, &e,
                                       out_slot));
}

int finder_fat_dir_ident(void *fs_user, uint16_t dir, uint16_t parent,
                                const char *name83)
{
    (void)fs_user;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_STALE;
    return fat12_dir_identity(g_finder_vol, g_finder_fat, g_finder_fat_len,
                              dir, parent, name83, g_finder_dirbuf) == FAT12_OK
         ? (int)FINDER_WIN_OK : (int)FINDER_WIN_ERR_STALE;
}

int finder_fat_read_partial(void *fs_user, const char *name83,
                                   uint16_t dir, uint32_t offset,
                                   uint32_t len, uint8_t *out,
                                   uint32_t *out_read)
{
    dir_entry_t e;
    uint32_t slot = 0u;
    int rc;

    (void)fs_user;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MOVE;
    rc = fat12_find_slot_in(g_finder_vol, g_finder_fat, g_finder_fat_len, dir,
                            g_finder_dirbuf, name83, &e, &slot);
    if (rc != FAT12_OK) return finder_fat_map(rc);
    return finder_fat_map(fat12_read_partial(g_finder_vol, g_finder_fat,
                                             g_finder_fat_len, &e, offset,
                                             len, out, g_finder_clusbuf,
                                             out_read));
}

int finder_fat_write_partial(void *fs_user, uint16_t dir, uint32_t slot,
                                    uint32_t offset, const uint8_t *data,
                                    uint32_t len, uint32_t *out_written)
{
    (void)fs_user;
    if (out_written) *out_written = 0u;
    if (finder_storage_write_check(g_finder_vol) == FAT12_ERR_ACCESS)
        return FINDER_WIN_ERR_WRITEPROTECT;
    if (!finder_fat_geometry_ok()) return (int)FINDER_WIN_ERR_MOVE;
    return finder_fat_map(fat12_write_partial(g_finder_vol, g_finder_fat,
                                              g_finder_fat_len, dir, slot,
                                              offset, data, len,
                                              g_finder_dirbuf,
                                              g_finder_clusbuf, out_written));
}

typedef struct finder_fs_enum {
    finder_enum_cb cb;
    void *user;
} finder_fat_enum_t;

int finder_fat_dirent_cb(const dir_entry_t *e, void *user)
{
    finder_fat_enum_t *fe = (finder_fat_enum_t *)user;
    finder_dirent_t d;
    char name[FAT12_NAME83_MAX];
    if (e == (const dir_entry_t *)0 || fe == (finder_fat_enum_t *)0) return 0;
    if (fat12_format_83(e, name) != FAT12_OK) return 0;
    d.name83 = name;
    d.attribute = e->attribute;
    d.size = e->file_size;
    d.start_cluster = e->start_cluster;
    d.date = e->mdate; d.time = e->mtime;
    return fe->cb(&d, fe->user);
}

int finder_fat_geometry_ok(void)
{
    return g_finder_vol != (const fat12_volume_t *)0 &&
           g_finder_vol->bpb.sectors_per_cluster >= 1u &&
           (uint32_t)g_finder_vol->bpb.sectors_per_cluster <= FINDER_FS_MAX_SPC;
}

int finder_fat_enumerate(void *fs_user, uint16_t dir_start,
                                finder_enum_cb cb, void *cb_user)
{
    fat12_dir_t dir;
    finder_fat_enum_t fe;
    int rc;
    (void)fs_user;
    if (!finder_fat_geometry_ok() || cb == (finder_enum_cb)0) return -1;
    dir.is_root = (dir_start == 0u) ? 1 : 0;
    dir.start_cluster = dir_start;
    fe.cb = cb;
    fe.user = cb_user;
    rc = fat12_read_dir(g_finder_vol, &dir, g_finder_dirbuf,
                        g_finder_fat, g_finder_fat_len,
                        finder_fat_dirent_cb, &fe);
    return (rc < 0) ? -1 : rc;
}

int finder_fat_trash_dir(void *fs_user)
{
    dir_entry_t e;
    uint32_t slot = 0u;
    (void)fs_user;
    if (!finder_fat_geometry_ok()) return -1;
    if (fat12_find_slot_in(g_finder_vol, g_finder_fat, g_finder_fat_len, 0u,
                           g_finder_dirbuf, DESKTOP_TRASH_NAME, &e, &slot) !=
        FAT12_OK)
        return -1;
    if ((e.attribute & DIR_ATTR_DIRECTORY) == 0u || e.start_cluster < 2u)
        return -1;
    return (int)e.start_cluster;
}

const finder_fs_t g_finder_fs_binding __attribute__((unused)) = {
    finder_fat_enumerate,
    finder_fat_mkdir,
    finder_fat_move,
    finder_fat_trash_name,
    finder_fat_trash_dir,
    finder_fat_unlink,
    finder_fat_rmdir,
    finder_fat_create,
    finder_fat_read_partial,
    finder_fat_write_partial,
    finder_fat_dir_ident,
    (void *)0
};

