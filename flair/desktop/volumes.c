/* Extracted production volumes; reference: legacy-final kmain.c. */
#include "desktop_internal.h"
#include "flair_platform.h"
#include "finder_storage.h"

int finder_abs(int v) { return v < 0 ? -v : v; }

void __attribute__((unused)) finder_trace_serial(void *user, const char *line)
{
    (void)user;
    if (line == (const char *)0) return;
    serial_puts(line);
    serial_putc('\n');
}

void __attribute__((unused)) finder_desk_load_positions(void)
{
    uint32_t len = 0u;
    int rc;
    if (g_finder_desk == (finder_desk_t *)0 ||
        g_finder_vol == (const fat12_volume_t *)0) return;
    rc = desktop_db_read(g_finder_vol, g_finder_fat, g_finder_fat_len,
                         g_finder_secbuf, g_finder_clusbuf,
                         g_finder_db_buf, (uint32_t)sizeof g_finder_db_buf,
                         &len);
    if (rc != FAT12_OK) {
        serial_puts("DESKTOP-DB-POS-SKIP rc=");
        serial_puti((int32_t)rc);
        serial_putc('\n');
        return;
    }
    if (finder_desk_db_apply(g_finder_desk, g_finder_db_buf, len) !=
        FINDER_DB_OK) {
        serial_puts("DESKTOP-DB-POS-REGEN\n");
        return;
    }
    if (g_finder_shell != (finder_shell_t *)0) {
        uint16_t views = finder_shell_load_views(g_finder_shell,
                                                 g_finder_db_buf, len);
        uint16_t origins;
        if (views != 0u) {
            serial_puts("DESKTOP-DB-VIEWS n=");
            serial_putu((uint32_t)views);
            serial_putc('\n');
        }
        origins = finder_shell_load_origins(g_finder_shell, g_finder_db_buf,
                                            len);
        if (origins != 0u) {
            serial_puts("DESKTOP-DB-ORIGINS n=");
            serial_putu((uint32_t)origins);
            serial_putc('\n');
        }
    }
}

void flair_tenant_mount_volume(void)
{
    const fat12_volume_t *v=fileio_fat_current_volume();
    flair_platform_bind_files(int21_io_drive());
    uint8_t *ften_secbuf=g_volume_sector;
    uint8_t ften_cluster[BLOCKDEV_SECTOR_SIZE];
    uint8_t ften_db_header[DESKTOP_DB_HEADER_SIZE];
    int mount_rc=v ? FAT12_OK : FAT12_ERR_PARTITION;
    serial_puts("FLAIR-FAT-MOUNT-BEGIN\n");
    if (mount_rc == FAT12_OK) {
        serial_puts("FLAIR-FAT-MOUNT-OK\n");
        {
            uint32_t fat_len = 0u;
            uint8_t *fat = (uint8_t *)fileio_fat_fat_buffer(&fat_len);
            fat = finder_storage_read_cache(v, fat);
            desktop_db_boot_state_t db_state = DESKTOP_DB_BOOT_OK;
            desktop_db_reason_t db_reason = DESKTOP_DB_REASON_NONE;
            desktop_trash_state_t trash_state = DESKTOP_TRASH_OK;
            int db_rc;
            int trash_rc;
            db_rc = desktop_db_bootstrap(v, fat, fat_len,
                                          ften_secbuf, ften_cluster,
                                          ften_db_header,
                                          sizeof(ften_db_header),
                                          &db_state, &db_reason);
            if (db_state == DESKTOP_DB_BOOT_CREATE) {
                serial_puts("DESKTOP-DB-CREATE\n");
            } else if (db_state == DESKTOP_DB_BOOT_REGEN) {
                serial_puts("DESKTOP-DB-REGEN reason=");
                serial_putu((uint32_t)db_reason);
                serial_putc('\n');
            } else if (db_rc == FAT12_OK) {
                serial_puts("DESKTOP-DB-OK\n");
            }
            if (db_rc != FAT12_OK) {
                g_finder_io_error=db_rc==FAT12_ERR_ACCESS ? FINDER_WIN_ERR_WRITEPROTECT :
                                    db_rc==FAT12_ERR_NO_SPACE || db_rc==FAT12_ERR_DIR_FULL?
                                    FINDER_WIN_ERR_DISKFULL:FINDER_WIN_ERR_STORAGE;
                if (db_state == DESKTOP_DB_BOOT_CREATE ||
                    db_state == DESKTOP_DB_BOOT_REGEN) {
                    serial_puts("DESKTOP-DB-WRITE-FAIL rc=");
                } else {
                    serial_puts("DESKTOP-DB-READ-FAIL rc=");
                }
                serial_puti((int32_t)db_rc);
                serial_putc('\n');
            }
            trash_rc = desktop_trash_ensure(v, fat, fat_len,
                                            ften_secbuf, ften_cluster,
                                            &trash_state);
            if (trash_rc == FAT12_OK) {
                serial_puts(trash_state == DESKTOP_TRASH_CREATE
                                ? "TRASH-CREATE\n" : "TRASH-OK\n");
            } else {
                serial_puts("TRASH-INIT-FAIL rc=");
                g_finder_io_error=trash_rc==FAT12_ERR_ACCESS ? FINDER_WIN_ERR_WRITEPROTECT :
                                    trash_rc==FAT12_ERR_NO_SPACE || trash_rc==FAT12_ERR_DIR_FULL?
                                    FINDER_WIN_ERR_DISKFULL:FINDER_WIN_ERR_STORAGE;
                serial_puti((int32_t)trash_rc);
                serial_putc('\n');
            }
            loader_bind_fat_volume(v, fat, fat_len);
            g_finder_vol = v;
            g_finder_fat = fat;
            g_finder_fat_len = fat_len;
            g_finder_secbuf = ften_secbuf;
            flair_live_apple_refresh();
            if (desktop_db_volume_label(v, ften_secbuf,
                                        g_finder_vol_label,
                                        (uint32_t)sizeof g_finder_vol_label) !=
                FAT12_OK) {
                g_finder_vol_label[0] = '\0';
            }
        }
    } else {
        serial_puts("FLAIR-FAT-MOUNT-FAIL rc=");
        serial_puti(mount_rc);
    }
}
