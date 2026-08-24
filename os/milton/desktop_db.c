/*
 * Boot-time DESKTOP.DB/TRASH persistence skeleton (beads initech-tdnl.8).
 * Ref: GUI-remediation-R3-finder-design.md F1.3/F1-2 and F1.4/F1-3.
 * Rule 11: header assembly is byte-explicit and contains no timestamp.
 */

#include "desktop_db.h"

static uint16_t db_le16(const uint8_t *p)
{
	return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

#ifndef DB_WRITE_SKIP
static void desktop_db_empty_header(uint8_t out[DESKTOP_DB_HEADER_SIZE])
{
	out[0] = (uint8_t)'I';
	out[1] = (uint8_t)'D';
	out[2] = (uint8_t)'B';
	out[3] = (uint8_t)'1';
#ifdef DB_MAGIC_FLIP
	/* Rule-6 mutant: creation emits a corrupt magic byte. The next boot must
	 * take DESKTOP-DB-REGEN (or the persistence oracle goes RED). */
	out[0] = (uint8_t)'X';
#endif
	out[4] = 0x01u; /* little-endian version 1 */
	out[5] = 0x00u;
	out[6] = 0x00u; /* little-endian n_records 0 */
	out[7] = 0x00u;
}
#endif

desktop_db_validation_t desktop_db_validate(const void *data, uint32_t len,
	                                         desktop_db_reason_t *out_reason)
{
	const uint8_t *p = (const uint8_t *)data;
	uint16_t records;
	uint32_t expected;

	if (out_reason != 0) {
		*out_reason = DESKTOP_DB_REASON_NONE;
	}
	if (p == 0 || len < DESKTOP_DB_HEADER_SIZE) {
		if (out_reason != 0) *out_reason = DESKTOP_DB_REASON_LENGTH;
		return DESKTOP_DB_CORRUPT;
	}
	if (p[0] != (uint8_t)'I' || p[1] != (uint8_t)'D' ||
	    p[2] != (uint8_t)'B' || p[3] != (uint8_t)'1') {
		if (out_reason != 0) *out_reason = DESKTOP_DB_REASON_MAGIC;
		return DESKTOP_DB_CORRUPT;
	}
	if (db_le16(p + 4u) != 1u) {
		if (out_reason != 0) *out_reason = DESKTOP_DB_REASON_VERSION;
		return DESKTOP_DB_CORRUPT;
	}
	records = db_le16(p + 6u);
	expected = DESKTOP_DB_HEADER_SIZE +
	           (uint32_t)records * DESKTOP_DB_RECORD_SIZE;
	if (expected != len) {
		if (out_reason != 0) *out_reason = DESKTOP_DB_REASON_SIZE;
		return DESKTOP_DB_CORRUPT;
	}
	return DESKTOP_DB_VALID;
}

static int desktop_db_write_default(const fat12_volume_t *vol, void *fat,
	                                uint32_t fat_len, void *sector_buf,
	                                void *cluster_buf)
{
#ifdef DB_WRITE_SKIP
	/* Rule-6 mutant: report success while skipping creation. The mtools
	 * differential must find DESKTOP.DB absent/wrong and go RED. */
	(void)vol;
	(void)fat;
	(void)fat_len;
	(void)sector_buf;
	(void)cluster_buf;
	return FAT12_OK;
#else
	uint8_t header[DESKTOP_DB_HEADER_SIZE];
	dir_entry_t entry;
	uint32_t slot = 0u;
	int rc;

	desktop_db_empty_header(header);
	rc = fat12_create(vol, fat, fat_len, DESKTOP_DB_NAME, DIR_ATTR_HIDDEN,
	                  0u, sector_buf, cluster_buf, &entry, &slot);
	if (rc != FAT12_OK) {
		return rc;
	}
	return fat12_write_file(vol, fat, fat_len, slot, header,
	                        DESKTOP_DB_HEADER_SIZE, sector_buf, cluster_buf);
#endif
}

int desktop_db_bootstrap(const fat12_volume_t *vol, void *fat,
	                     uint32_t fat_len, void *sector_buf,
	                     void *cluster_buf, void *db_buf,
	                     uint32_t db_buf_len,
	                     desktop_db_boot_state_t *out_state,
	                     desktop_db_reason_t *out_reason)
{
	dir_entry_t entry;
	uint32_t out_bytes = 0u;
	int rc;

	if (vol == 0 || sector_buf == 0 || cluster_buf == 0 || db_buf == 0 ||
	    out_state == 0 || out_reason == 0) {
		return FAT12_ERR_NULL;
	}
	*out_state = DESKTOP_DB_BOOT_OK;
	*out_reason = DESKTOP_DB_REASON_NONE;

	rc = fat12_find(vol, sector_buf, DESKTOP_DB_NAME, &entry);
	if (rc == FAT12_ERR_NOT_FOUND) {
		*out_state = DESKTOP_DB_BOOT_CREATE;
		return desktop_db_write_default(vol, fat, fat_len, sector_buf,
		                                cluster_buf);
	}
	if (rc != FAT12_OK) {
		return rc;
	}
	if ((entry.attribute & (DIR_ATTR_DIRECTORY | DIR_ATTR_VOLLABEL)) != 0u) {
		*out_state = DESKTOP_DB_BOOT_REGEN;
		*out_reason = DESKTOP_DB_REASON_TYPE;
		return FAT12_ERR_ACCESS;
	}
	if (entry.file_size > DESKTOP_DB_MAX_BYTES) {
		*out_state = DESKTOP_DB_BOOT_REGEN;
		*out_reason = DESKTOP_DB_REASON_LENGTH;
		return desktop_db_write_default(vol, fat, fat_len, sector_buf,
		                                cluster_buf);
	}
	if (db_buf_len < DESKTOP_DB_HEADER_SIZE) {
		return FAT12_ERR_BUFFER;
	}
	rc = fat12_read_partial(vol, fat, fat_len, &entry, 0u,
	                        DESKTOP_DB_HEADER_SIZE, db_buf, cluster_buf,
	                        &out_bytes);
	if (rc != FAT12_OK) {
		return rc;
	}
	/* Validation reads only the 8-byte header; the directory entry's file_size
	 * supplies the exact 8 + 24*n structural-length oracle. A short file yields
	 * out_bytes < 8 and is rejected before the validator reads past the buffer. */
	if (out_bytes < DESKTOP_DB_HEADER_SIZE) {
		*out_reason = DESKTOP_DB_REASON_LENGTH;
		*out_state = DESKTOP_DB_BOOT_REGEN;
		return desktop_db_write_default(vol, fat, fat_len, sector_buf,
		                                cluster_buf);
	}
	if (desktop_db_validate(db_buf, entry.file_size, out_reason) ==
	    DESKTOP_DB_CORRUPT) {
		*out_state = DESKTOP_DB_BOOT_REGEN;
		return desktop_db_write_default(vol, fat, fat_len, sector_buf,
		                                cluster_buf);
	}
	return FAT12_OK;
}

/* ---------------------------------------------------------------------------
 * PER-ICON RECORD I/O (beads initech-tdnl.9; design F1.3). See desktop_db.h for
 * the layering rule: the 24-byte record codec is os/flair/finder_desktop.c's,
 * the FAT read/write is here.
 * ------------------------------------------------------------------------- */

int desktop_db_read(const fat12_volume_t *vol, const void *fat,
                    uint32_t fat_len, void *sector_buf, void *cluster_buf,
                    void *out, uint32_t cap, uint32_t *out_len)
{
	dir_entry_t entry;
	uint32_t got = 0u;
	int rc;

	if (vol == 0 || sector_buf == 0 || cluster_buf == 0 || out == 0 ||
	    out_len == 0) {
		return FAT12_ERR_NULL;
	}
	*out_len = 0u;

	rc = fat12_find(vol, sector_buf, DESKTOP_DB_NAME, &entry);
	if (rc != FAT12_OK) {
		return rc;
	}
	if ((entry.attribute & (DIR_ATTR_DIRECTORY | DIR_ATTR_VOLLABEL)) != 0u) {
		return FAT12_ERR_ACCESS;
	}
	if (entry.file_size > cap) {
		return FAT12_ERR_BUFFER;
	}
	rc = fat12_read_file(vol, fat, fat_len, &entry, out, cap, cluster_buf,
	                     &got);
	if (rc != FAT12_OK) {
		return rc;
	}
	*out_len = got;
	return FAT12_OK;
}

int desktop_db_write(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                     void *sector_buf, void *cluster_buf,
                     const void *data, uint32_t len)
{
	dir_entry_t entry;
	uint32_t slot = 0u;
	int rc;

	if (vol == 0 || sector_buf == 0 || cluster_buf == 0 || data == 0) {
		return FAT12_ERR_NULL;
	}
	/* Refuse to commit a blob that the next boot would have to REGEN. */
	if (len > DESKTOP_DB_MAX_BYTES ||
	    desktop_db_validate(data, len, 0) != DESKTOP_DB_VALID) {
		return FAT12_ERR_BUFFER;
	}
	rc = fat12_create(vol, fat, fat_len, DESKTOP_DB_NAME, DIR_ATTR_HIDDEN,
	                  0u, sector_buf, cluster_buf, &entry, &slot);
	if (rc != FAT12_OK) {
		return rc;
	}
	return fat12_write_file(vol, fat, fat_len, slot, data, len, sector_buf,
	                        cluster_buf);
}

/* fat12_read_root_dir visitor: capture the first volume-label entry's raw 11
 * name bytes. Returns non-zero to STOP the enumeration once found (the fat12.h
 * callback contract); the entry is valid only for the call's duration, so the
 * bytes are copied out here. */
typedef struct desktop_db_label_ctx {
	char    name[12];
	uint8_t found;
} desktop_db_label_ctx_t;

static int desktop_db_label_cb(const dir_entry_t *e, void *user)
{
	desktop_db_label_ctx_t *ctx = (desktop_db_label_ctx_t *)user;
	uint32_t i;

	if (e == 0 || ctx == 0) {
		return 0;
	}
	if ((e->attribute & DIR_ATTR_VOLLABEL) == 0u ||
	    (e->attribute & DIR_ATTR_DIRECTORY) != 0u) {
		return 0;
	}
	for (i = 0u; i < 8u; i++) {
		ctx->name[i] = (char)e->filename[i];
	}
	for (i = 0u; i < 3u; i++) {
		ctx->name[8u + i] = (char)e->extension[i];
	}
	ctx->name[11] = '\0';
	/* Trailing-space trim: the 11 bytes are space padded on disk. */
	for (i = 11u; i > 0u; i--) {
		if (ctx->name[i - 1u] != ' ') {
			break;
		}
		ctx->name[i - 1u] = '\0';
	}
	ctx->found = 1u;
	return 1;   /* stop */
}

int desktop_db_volume_label(const fat12_volume_t *vol, void *sector_buf,
                            char *out, uint32_t out_len)
{
	desktop_db_label_ctx_t ctx;
	uint32_t i;
	int rc;

	if (vol == 0 || sector_buf == 0 || out == 0) {
		return FAT12_ERR_NULL;
	}
	if (out_len < 12u) {
		return FAT12_ERR_BUFFER;
	}
	for (i = 0u; i < 12u; i++) {
		ctx.name[i] = '\0';
	}
	ctx.found = 0u;

	rc = fat12_read_root_dir(vol, sector_buf, desktop_db_label_cb, &ctx);
	/* The callback's non-zero early stop is propagated verbatim; only a real
	 * error code (negative) is a failure. */
	if (rc != FAT12_OK && rc != 1) {
		return rc;
	}
	if (!ctx.found) {
		return FAT12_ERR_NOT_FOUND;
	}
	for (i = 0u; i < 12u; i++) {
		out[i] = ctx.name[i];
	}
	return FAT12_OK;
}

int desktop_trash_ensure(const fat12_volume_t *vol, void *fat,
	                     uint32_t fat_len, void *sector_buf,
	                     void *cluster_buf,
	                     desktop_trash_state_t *out_state)
{
	dir_entry_t entry;
	int rc;

	if (vol == 0 || sector_buf == 0 || cluster_buf == 0 || out_state == 0) {
		return FAT12_ERR_NULL;
	}
	*out_state = DESKTOP_TRASH_OK;
	rc = fat12_find(vol, sector_buf, DESKTOP_TRASH_NAME, &entry);
	if (rc == FAT12_ERR_NOT_FOUND) {
		*out_state = DESKTOP_TRASH_CREATE;
		return fat12_mkdir(vol, fat, fat_len, DESKTOP_TRASH_NAME, 0u,
		                   sector_buf, cluster_buf);
	}
	if (rc != FAT12_OK) {
		return rc;
	}
	if ((entry.attribute & DIR_ATTR_DIRECTORY) == 0u) {
		return FAT12_ERR_ACCESS;
	}
	return FAT12_OK;
}
