/*
 * Boot-time DESKTOP.DB/TRASH persistence skeleton (beads initech-tdnl.8).
 * Ref: GUI-remediation-R3-finder-design.md F1.3/F1-2 and F1.4/F1-3.
 * contract: header assembly is byte-explicit and contains no timestamp.
 */

#include "desktop_db.h"
static char db_recovery_name[12];
static uint8_t db_verify[DESKTOP_DB_MAX_BYTES];
const char *desktop_db_recovery(void) { return db_recovery_name[0]?db_recovery_name:0; }
int desktop_db_recovery_alert(const char *text[3])
{
    if(!db_recovery_name[0]) return 0;
    text[0]="The disk could not be updated.";
    text[1]="The previous Desktop file is saved as:";
    text[2]=db_recovery_name;
    return 3;
}

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
	/* contract mutant: creation emits a corrupt magic byte. The next boot must
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

/* GM4: the previous database remains allocated throughout staging. Check
 * genuinely free clusters, never count the previous database's chain.
 * Ref: fat12.h create/write_file contracts, PRD 6.5 fail-loud storage. */
static int db_space(const fat12_volume_t *vol,const void *fat,uint32_t fat_len,
                     void *sector_buf,uint32_t len)
{
    uint32_t bytes=(uint32_t)vol->bpb.bytes_per_sector*vol->bpb.sectors_per_cluster;
    uint32_t need=(len+bytes-1u)/bytes, free=0u;
    uint16_t next;
    int rc;
    (void)sector_buf;
    for(uint32_t i=2;free<need && i<vol->total_clusters+2u;i++) {
        rc=fat12_next_cluster(vol,fat,fat_len,(uint16_t)i,&next);
        if(rc!=FAT12_OK) return rc;
        if(!next) free++;
    }
    return free>=need?FAT12_OK:FAT12_ERR_NO_SPACE;
}

static int desktop_db_write_default(const fat12_volume_t *vol, void *fat,
	                                uint32_t fat_len, void *sector_buf,
	                                void *cluster_buf)
{
#ifdef DB_WRITE_SKIP
	/* contract mutant: report success while skipping creation. The mtools
	 * differential must find DESKTOP.DB absent/wrong and go RED. */
	(void)vol;
	(void)fat;
	(void)fat_len;
	(void)sector_buf;
	(void)cluster_buf;
	return FAT12_OK;
#else
	uint8_t header[DESKTOP_DB_HEADER_SIZE];
	desktop_db_empty_header(header);
	return desktop_db_write(vol,fat,fat_len,sector_buf,cluster_buf,header,sizeof header);
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
	int rc, old_moved=0;
	char stage[]="DESKTOP.$00", backup[]="DESKTOP.~00";

	if (vol == 0 || sector_buf == 0 || cluster_buf == 0 || data == 0) {
		return FAT12_ERR_NULL;
	}
	if(db_recovery_name[0]) return FAT12_ERR_WRITE;
	/* Refuse to commit a blob that the next boot would have to REGEN. */
	if (len > DESKTOP_DB_MAX_BYTES ||
	    desktop_db_validate(data, len, 0) != DESKTOP_DB_VALID) {
		return FAT12_ERR_BUFFER;
	}
	rc=fat12_find(vol,sector_buf,DESKTOP_DB_NAME,&entry);
	if(rc==FAT12_OK && (entry.attribute & DIR_ATTR_READONLY)) return FAT12_ERR_ACCESS;
	if(rc!=FAT12_OK && rc!=FAT12_ERR_NOT_FOUND) return rc;
	rc=db_space(vol,fat,fat_len,sector_buf,len);
	if(rc!=FAT12_OK) return rc;
	/* Reserve only absent names; the cooperative pump cannot interleave a
	 * competing writer. Never truncate a retained recovery stream. */
	for(unsigned k=0;k<100;k++) {
		stage[9]=(char)('0'+k/10); stage[10]=(char)('0'+k%10);
		rc=fat12_find(vol,sector_buf,stage,&entry);
		if(rc==FAT12_ERR_NOT_FOUND) break;
		if(rc!=FAT12_OK) return rc;
	}
	if(rc!=FAT12_ERR_NOT_FOUND) return FAT12_ERR_EXISTS;
	rc = fat12_create(vol, fat, fat_len, stage, DIR_ATTR_HIDDEN,
	                  0u, sector_buf, cluster_buf, &entry, &slot);
	if (rc != FAT12_OK) {
		return rc;
	}
	rc=fat12_write_file(vol,fat,fat_len,slot,data,len,sector_buf,cluster_buf);
	if(rc!=FAT12_OK) goto discard;
	rc=fat12_find(vol,sector_buf,stage,&entry);
	if(rc!=FAT12_OK) goto discard;
	uint32_t got=0;
	rc=fat12_read_file(vol,fat,fat_len,&entry,db_verify,sizeof db_verify,cluster_buf,&got);
	if(rc!=FAT12_OK) goto discard;
	if(got!=len) { rc=FAT12_ERR_READ; goto discard; }
	for(uint32_t i=0;i<len;i++) if(db_verify[i]!=((const uint8_t *)data)[i]) { rc=FAT12_ERR_READ; goto discard; }
	for(unsigned k=0;k<100;k++) {
		backup[9]=(char)('0'+k/10); backup[10]=(char)('0'+k%10);
		rc=fat12_find(vol,sector_buf,backup,&entry);
		if(rc==FAT12_ERR_NOT_FOUND) break;
		if(rc!=FAT12_OK) goto discard;
	}
	if(rc!=FAT12_ERR_NOT_FOUND) { rc=FAT12_ERR_EXISTS; goto discard; }
	rc=fat12_rename(vol,fat,fat_len,DESKTOP_DB_NAME,backup,0,sector_buf);
	if(rc==FAT12_OK) old_moved=1;
	else if(rc!=FAT12_ERR_NOT_FOUND) goto discard;
	rc=fat12_rename(vol,fat,fat_len,stage,DESKTOP_DB_NAME,0,sector_buf);
	if(rc!=FAT12_OK) {
		if(old_moved && fat12_rename(vol,fat,fat_len,backup,DESKTOP_DB_NAME,0,sector_buf)!=FAT12_OK) {
			for(unsigned i=0;i<sizeof backup;i++) db_recovery_name[i]=backup[i];
			return rc; /* Keep both complete streams if rollback also fails. */
		}
		goto discard;
	}
	if(old_moved) (void)fat12_unlink(vol,fat,fat_len,backup,0,sector_buf);
	return FAT12_OK; /* Cleanup cannot turn a committed save into a failure. */
discard:
	(void)fat12_unlink(vol,fat,fat_len,stage,0,sector_buf);
	return rc;
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
		rc = fat12_mkdir(vol, fat, fat_len, DESKTOP_TRASH_NAME, 0u,
		                   sector_buf, cluster_buf);
		if (rc != FAT12_OK) return rc;
		return fat12_hide_dir(vol, fat, fat_len, DESKTOP_TRASH_NAME, 0u,
		                      sector_buf);
	}
	if (rc != FAT12_OK) {
		return rc;
	}
	if ((entry.attribute & DIR_ATTR_DIRECTORY) == 0u) {
		return FAT12_ERR_ACCESS;
	}
	/* Upgrade an existing service directory through the same safe attribute API.
	 * F1-3 / tdnl.30: hidden TRASH, just as DESKTOP.DB is hidden. */
	return fat12_hide_dir(vol, fat, fat_len, DESKTOP_TRASH_NAME, 0u, sector_buf);
}
