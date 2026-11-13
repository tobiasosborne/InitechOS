/*
 * DESKTOP.DB/TRASH persistence skeleton for the flagship FLAIR volume.
 * Ref: docs/design/GUI-remediation-R3-finder-design.md F1.3 (F1-2),
 * F1.4 (F1-3), and F5.2 tdnl.8. Artifact-safe: fixed buffers, no libc.
 */
#ifndef INITECH_MILTON_DESKTOP_DB_H
#define INITECH_MILTON_DESKTOP_DB_H

#include <stdint.h>

#include "fat12.h"

#define DESKTOP_DB_NAME        "DESKTOP.DB"
#define DESKTOP_TRASH_NAME     "TRASH"
#define DESKTOP_DB_HEADER_SIZE 8u
#define DESKTOP_DB_RECORD_SIZE 24u
#define DESKTOP_DB_MAX_BYTES   4096u

typedef enum desktop_db_validation {
	DESKTOP_DB_VALID   = 0,
	DESKTOP_DB_CORRUPT = 1
} desktop_db_validation_t;

typedef enum desktop_db_reason {
	DESKTOP_DB_REASON_NONE    = 0,
	DESKTOP_DB_REASON_LENGTH  = 1,
	DESKTOP_DB_REASON_MAGIC   = 2,
	DESKTOP_DB_REASON_VERSION = 3,
	DESKTOP_DB_REASON_SIZE    = 4,
	DESKTOP_DB_REASON_TYPE    = 5
} desktop_db_reason_t;

typedef enum desktop_db_boot_state {
	DESKTOP_DB_BOOT_OK     = 0,
	DESKTOP_DB_BOOT_CREATE = 1,
	DESKTOP_DB_BOOT_REGEN  = 2
} desktop_db_boot_state_t;

typedef enum desktop_trash_state {
	DESKTOP_TRASH_OK     = 0,
	DESKTOP_TRASH_CREATE = 1
} desktop_trash_state_t;

/* Validate the exact F1.3 layout: 8-byte header plus n 24-byte records. */
desktop_db_validation_t desktop_db_validate(const void *data, uint32_t len,
	                                         desktop_db_reason_t *out_reason);

/* Load the root DB, creating an absent file or regenerating a corrupt file.
 * `db_buf` is caller-owned header storage (at least DESKTOP_DB_HEADER_SIZE).
 * The returned FAT12 code is loud I/O status; out_state remains CREATE/REGEN
 * when the corresponding write attempt fails so serial can report both facts. */
int desktop_db_bootstrap(const fat12_volume_t *vol, void *fat,
	                     uint32_t fat_len, void *sector_buf,
	                     void *cluster_buf, void *db_buf,
	                     uint32_t db_buf_len,
	                     desktop_db_boot_state_t *out_state,
	                     desktop_db_reason_t *out_reason);

/* ---------------------------------------------------------------------------
 * PER-ICON RECORD I/O (beads initech-tdnl.9; design F1.3 "whole-file rewrite").
 *
 * LAYERING: the 24-byte record CODEC lives in os/flair/finder_desktop.c with
 * the icon records it serialises (os/flair may not include os/milton headers,
 * and the codec has to be host-gradable without a FAT). THIS file owns the FAT
 * side only: read the blob, or truncate-and-rewrite it. The two meet in
 * os/milton/kmain.c. Structural validation of the blob is desktop_db_validate
 * above (identical layout law, so the two validators cannot drift on the
 * header) plus the codec's own record-count check.
 * ------------------------------------------------------------------------- */

/* Read the whole root DESKTOP.DB into `out` (`cap` bytes). Returns FAT12_OK
 * with *out_len set, FAT12_ERR_NOT_FOUND when the file is absent, or
 * FAT12_ERR_BUFFER when the file is larger than `cap` (never overflow, Rule 2).
 * The caller validates the bytes; a corrupt blob is a REGEN, not a panic. */
int desktop_db_read(const fat12_volume_t *vol, const void *fat,
                    uint32_t fat_len, void *sector_buf, void *cluster_buf,
                    void *out, uint32_t cap, uint32_t *out_len);

/* Whole-file rewrite of \DESKTOP.DB (hidden): fat12_create truncates the old
 * chain, fat12_write_file lays the new one down and patches size/start cluster.
 * `data` MUST already be a structurally valid image (desktop_db_validate);
 * a malformed blob is refused with FAT12_ERR_BUFFER rather than written, so a
 * writer bug can never produce a file that the next boot must REGEN. */
int desktop_db_write(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                     void *sector_buf, void *cluster_buf,
                     const void *data, uint32_t len);

/* Copy the mounted volume's LABEL (the root DIR_ATTR_VOLLABEL entry's raw
 * 11 name bytes, trailing spaces trimmed) into `out` as a NUL-terminated
 * string. Returns FAT12_OK, FAT12_ERR_NOT_FOUND when the volume has no label,
 * or FAT12_ERR_BUFFER when out_len < 12. This is the Finder's volume-icon
 * label (design F1.1) read from the disk itself rather than hard-coded. */
int desktop_db_volume_label(const fat12_volume_t *vol, void *sector_buf,
                            char *out, uint32_t out_len);

/* Ensure the root TRASH directory exists. fat12_mkdir has no attribute
 * parameter and fat12_set_attr deliberately rejects directory targets, so this
 * skeleton creates an ordinary directory; hidden-directory attributes remain
 * deferred rather than adding speculative FAT machinery in tdnl.8. */
int desktop_trash_ensure(const fat12_volume_t *vol, void *fat,
	                     uint32_t fat_len, void *sector_buf,
	                     void *cluster_buf,
	                     desktop_trash_state_t *out_state);

#endif /* INITECH_MILTON_DESKTOP_DB_H */
