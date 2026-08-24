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

/* Ensure the root TRASH directory exists. fat12_mkdir has no attribute
 * parameter and fat12_set_attr deliberately rejects directory targets, so this
 * skeleton creates an ordinary directory; hidden-directory attributes remain
 * deferred rather than adding speculative FAT machinery in tdnl.8. */
int desktop_trash_ensure(const fat12_volume_t *vol, void *fat,
	                     uint32_t fat_len, void *sector_buf,
	                     void *cluster_buf,
	                     desktop_trash_state_t *out_state);

#endif /* INITECH_MILTON_DESKTOP_DB_H */
