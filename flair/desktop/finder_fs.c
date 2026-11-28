/* Finder filesystem binding, shared by all Finder operation routes.
 * Read algorithms/geometry: kernel/include/fat12.h and README.md.
 * Write capability: finder_storage.h, operator ruling 2026-11-14. */
#include "finder_actions.h"
#include "desktop_db.h"
#include "finder_storage.h"
#include "fileio_fat.h"
extern void serial_puts(const char *);

/* Every operation resolves its own volume (C activation plan step 10; MTE
 * Finder Interface: a specification names a volume and a directory). The
 * identity's drive selects the mounted volume and its own FAT cache BEFORE
 * the 16-bit physical cluster is extracted. An untagged identity is the
 * system volume bound at startup (volumes.c); another drive comes from
 * fileio_fat_drive_volume, never from the global system binding. */
typedef struct finder_fs_target {
    const fat12_volume_t *vol;
    void *fat;
    uint32_t fat_len;
    uint16_t cluster;
    finder_dir_id_t tag;   /* drive tag of the identity, 0 when untagged */
} finder_fs_target_t;

static int finder_fs_geometry(const fat12_volume_t *v)
{
    return v != (const fat12_volume_t *)0 &&
           v->bpb.sectors_per_cluster >= 1u &&
           (uint32_t)v->bpb.sectors_per_cluster <= FINDER_FS_MAX_SPC;
}

static int finder_fs_target(finder_dir_id_t dir, finder_fs_target_t *t)
{
    unsigned drive;
    t->vol = 0; t->fat = 0; t->fat_len = 0u;
    if (dir == FINDER_DIR_INVALID) return 0;
    drive = finder_dir_drive(dir, g_finder_drive);
    t->cluster = finder_dir_cluster(dir);
    t->tag = dir & ~(finder_dir_id_t)0xffffu;
    if (drive == g_finder_drive) {
        t->vol = g_finder_vol; t->fat = g_finder_fat; t->fat_len = g_finder_fat_len;
    } else {
        void *fat = 0;
        uint32_t len = 0u;
        t->vol = fileio_fat_drive_volume(drive, &fat, &len);
        t->fat = finder_storage_read_cache(t->vol, fat);
        t->fat_len = len;
    }
    return finder_fs_geometry(t->vol);
}
/* Write-capable target: the identity's own volume, FAT12 only (operator
 * ruling 2026-11-14, finder_storage.h), geometry within the scratch. */
static int finder_fs_write_target(finder_dir_id_t dir, finder_fs_target_t *t,
                                  int geometry_error)
{
    int ok = finder_fs_target(dir, t);
    if (t->vol && finder_storage_write_check(t->vol) == FAT12_ERR_ACCESS)
        return FINDER_WIN_ERR_WRITEPROTECT;
    return ok ? (int)FINDER_WIN_OK : geometry_error;
}

int finder_fat_mkdir(void *fs_user, const char *name83,
                            finder_dir_id_t parent_dir_start)
{
    finder_fs_target_t t;
    int rc;

    (void)fs_user;
    rc = finder_fs_write_target(parent_dir_start, &t, (int)FINDER_WIN_ERR_MKDIR);
    if (rc != (int)FINDER_WIN_OK) return rc;
    rc = fat12_mkdir(t.vol, t.fat, t.fat_len, name83, t.cluster,
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
                           finder_dir_id_t src_dir, const char *dst_name83,
                           finder_dir_id_t dst_dir)
{
    finder_fs_target_t s, d;
    int rc;
    (void)fs_user;
    rc = finder_fs_write_target(src_dir, &s, (int)FINDER_WIN_ERR_MOVE);
    if (rc != (int)FINDER_WIN_OK) return rc;
    rc = finder_fs_write_target(dst_dir, &d, (int)FINDER_WIN_ERR_MOVE);
    if (rc != (int)FINDER_WIN_OK) return rc;
    /* A directory entry moves only within its own volume. Copying between
     * volumes is a separate operation (plan step 12); never relink across. */
    if (s.vol != d.vol) return (int)FINDER_WIN_ERR_MOVE;
    return finder_fat_map(fat12_move_dirent(s.vol, s.fat, s.fat_len,
                                            src_name83, s.cluster,
                                            dst_name83, d.cluster,
                                            g_finder_dirbuf,
                                            g_finder_clusbuf));
}

int finder_fat_trash_name(void *fs_user, const char *name83,
                                 finder_dir_id_t dir, char *out)
{
    finder_fs_target_t t;
    (void)fs_user;
    if (!finder_fs_target(dir, &t)) return (int)FINDER_WIN_ERR_MOVE;
    return finder_fat_map(fat12_trash_suffix_name(t.vol, t.fat, t.fat_len,
                                                  t.cluster, g_finder_dirbuf,
                                                  name83, out));
}

int finder_trash_reserve(finder_dir_id_t dir)
{
    finder_fs_target_t t;
    desktop_trash_state_t state = DESKTOP_TRASH_OK;
    int rc;

    if (!finder_fs_target(dir, &t)) return FAT12_ERR_NULL;
    rc = finder_storage_write_check(t.vol);
    if (rc != FAT12_OK) return rc;
    rc = desktop_trash_ensure(t.vol, t.fat, t.fat_len,
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

int finder_fat_unlink(void *fs_user, const char *name83, finder_dir_id_t dir)
{
    finder_fs_target_t t;
    int rc;
    (void)fs_user;
    rc = finder_fs_write_target(dir, &t, (int)FINDER_WIN_ERR_MOVE);
    if (rc != (int)FINDER_WIN_OK) return rc;
    return finder_fat_map(fat12_unlink(t.vol, t.fat, t.fat_len, name83,
                                       t.cluster, g_finder_dirbuf));
}

int finder_fat_rmdir(void *fs_user, const char *name83, finder_dir_id_t dir)
{
    finder_fs_target_t t;
    int rc;
    (void)fs_user;
    rc = finder_fs_write_target(dir, &t, (int)FINDER_WIN_ERR_MOVE);
    if (rc != (int)FINDER_WIN_OK) return rc;
    return finder_fat_map(fat12_rmdir(t.vol, t.fat, t.fat_len, name83,
                                      t.cluster, g_finder_dirbuf));
}

int finder_fat_create(void *fs_user, const char *name83, finder_dir_id_t dir,
                             uint32_t *out_slot)
{
    finder_fs_target_t t;
    dir_entry_t e;
    int rc;
    (void)fs_user;
    rc = finder_fs_write_target(dir, &t, (int)FINDER_WIN_ERR_MOVE);
    if (rc != (int)FINDER_WIN_OK) return rc;
    return finder_fat_map(fat12_create(t.vol, t.fat, t.fat_len, name83,
                                       (uint8_t)DIR_ATTR_ARCHIVE, t.cluster,
                                       g_finder_dirbuf, g_finder_clusbuf, &e,
                                       out_slot));
}

int finder_fat_dir_ident(void *fs_user, finder_dir_id_t dir, finder_dir_id_t parent,
                                const char *name83)
{
    finder_fs_target_t t, p;
    (void)fs_user;
    if (!finder_fs_target(dir, &t) || !finder_fs_target(parent, &p) ||
        t.vol != p.vol)
        return (int)FINDER_WIN_ERR_STALE;
    return fat12_dir_identity(t.vol, t.fat, t.fat_len, t.cluster, p.cluster,
                              name83, g_finder_dirbuf) == FAT12_OK
         ? (int)FINDER_WIN_OK : (int)FINDER_WIN_ERR_STALE;
}

int finder_fat_read_partial(void *fs_user, const char *name83,
                                   finder_dir_id_t dir, uint32_t offset,
                                   uint32_t len, uint8_t *out,
                                   uint32_t *out_read)
{
    finder_fs_target_t t;
    dir_entry_t e;
    uint32_t slot = 0u;
    int rc;

    (void)fs_user;
    if (!finder_fs_target(dir, &t)) return (int)FINDER_WIN_ERR_MOVE;
    rc = fat12_find_slot_in(t.vol, t.fat, t.fat_len, t.cluster,
                            g_finder_dirbuf, name83, &e, &slot);
    if (rc != FAT12_OK) return finder_fat_map(rc);
    return finder_fat_map(fat12_read_partial(t.vol, t.fat, t.fat_len, &e,
                                             offset, len, out,
                                             g_finder_clusbuf, out_read));
}

int finder_fat_write_partial(void *fs_user, finder_dir_id_t dir, uint32_t slot,
                                    uint32_t offset, const uint8_t *data,
                                    uint32_t len, uint32_t *out_written)
{
    finder_fs_target_t t;
    int rc;
    (void)fs_user;
    if (out_written) *out_written = 0u;
    rc = finder_fs_write_target(dir, &t, (int)FINDER_WIN_ERR_MOVE);
    if (rc != (int)FINDER_WIN_OK) return rc;
    return finder_fat_map(fat12_write_partial(t.vol, t.fat, t.fat_len,
                                              t.cluster, slot, offset, data,
                                              len, g_finder_dirbuf,
                                              g_finder_clusbuf, out_written));
}

typedef struct finder_fs_enum {
    finder_enum_cb cb;
    void *user;
    finder_dir_id_t tag;
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
    /* A child's identity is on its parent's volume. */
    d.start_cluster = fe->tag | (finder_dir_id_t)e->start_cluster;
    d.date = e->mdate; d.time = e->mtime;
    return fe->cb(&d, fe->user);
}

int finder_fat_enumerate(void *fs_user, finder_dir_id_t dir_start,
                                finder_enum_cb cb, void *cb_user)
{
    finder_fs_target_t t;
    fat12_dir_t dir;
    finder_fat_enum_t fe;
    int rc;
    (void)fs_user;
    if (!finder_fs_target(dir_start, &t) || cb == (finder_enum_cb)0) return -1;
    dir.is_root = (t.cluster == 0u) ? 1 : 0;
    dir.start_cluster = t.cluster;
    fe.cb = cb;
    fe.user = cb_user;
    fe.tag = t.tag;
    rc = fat12_read_dir(t.vol, &dir, g_finder_dirbuf, t.fat, t.fat_len,
                        finder_fat_dirent_cb, &fe);
    return (rc < 0) ? -1 : rc;
}

/* Inside Macintosh: Macintosh Toolbox Essentials, Finder Interface ch. 7:
 * each mounted volume has its own Trash directory at its root. */
int finder_fat_trash_dir(void *fs_user, unsigned drive)
{
    finder_fs_target_t t;
    finder_dir_id_t root = drive == g_finder_drive ? 0u : finder_dir_make(drive, 0u);
    dir_entry_t e;
    uint32_t slot = 0u;
    (void)fs_user;
    if (!finder_fs_target(root, &t)) return -1;
    if (fat12_find_slot_in(t.vol, t.fat, t.fat_len, 0u,
                           g_finder_dirbuf, DESKTOP_TRASH_NAME, &e, &slot) !=
        FAT12_OK)
        return -1;
    if ((e.attribute & DIR_ATTR_DIRECTORY) == 0u || e.start_cluster < 2u)
        return -1;
    return (int)(root | e.start_cluster);
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
