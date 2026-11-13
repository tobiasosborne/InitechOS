/*
 * Host oracle for the R3.1 DESKTOP.DB/TRASH persistence skeleton.
 * Ref: GUI-remediation-R3-finder-design.md F1.3/F1-2 and F1.4/F1-3.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "test_assert.h"
#include "blockdev_file.h"
#include "desktop_db.h"

TEST_HARNESS();

static uint8_t g_fat[12u * 512u];
static uint8_t g_sector[512];
static uint8_t g_cluster[512];
static uint8_t g_db[DESKTOP_DB_MAX_BYTES];

static const uint8_t expected_empty_db[DESKTOP_DB_HEADER_SIZE] = {
	'I', 'D', 'B', '1', 0x01u, 0x00u, 0x00u, 0x00u
};

static int read_db(const fat12_volume_t *vol, uint32_t fat_len,
                   dir_entry_t *out_entry, uint32_t *out_bytes)
{
	int rc = fat12_find(vol, g_sector, DESKTOP_DB_NAME, out_entry);
	if (rc != FAT12_OK) {
		return rc;
	}
	return fat12_read_file(vol, g_fat, fat_len, out_entry, g_db,
	                       sizeof(g_db), g_cluster, out_bytes);
}

int main(int argc, char **argv)
{
	blockdev_file_t bf;
	fat12_volume_t vol;
	desktop_db_boot_state_t state;
	desktop_db_reason_t reason;
	desktop_trash_state_t trash_state;
	dir_entry_t entry;
	uint32_t fat_len;
	uint32_t bytes = 0u;
	int rc;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <blank-fat12-image>\n", argv[0]);
		return 2;
	}

	CHECK(DESKTOP_DB_RECORD_SIZE == 24u,
	      "DESKTOP.DB fixed record size is 24 bytes (F1.3)");
	CHECK(desktop_db_validate(expected_empty_db, sizeof(expected_empty_db),
	                          &reason) == DESKTOP_DB_VALID,
	      "independent empty-header golden validates");

	rc = blockdev_file_open_rw(&bf, argv[1]);
	CHECK(rc == 0, "open blank FAT12 image read-write");
	if (rc != 0) {
		return TEST_SUMMARY("test_desktop_db");
	}
	rc = fat12_mount(&vol, &bf.dev, g_sector);
	CHECK(rc == FAT12_OK, "mount blank FAT12 image");
	rc = fat12_read_fat(&vol, g_fat, sizeof(g_fat));
	CHECK(rc == FAT12_OK, "read FAT12 allocation table");
	fat_len = (uint32_t)vol.bpb.sectors_per_fat *
	          (uint32_t)vol.bpb.bytes_per_sector;

	rc = desktop_db_bootstrap(&vol, g_fat, fat_len, g_sector, g_cluster,
	                          g_db, sizeof(g_db), &state, &reason);
	CHECK(rc == FAT12_OK && state == DESKTOP_DB_BOOT_CREATE,
	      "absent DESKTOP.DB takes the create path");
	rc = read_db(&vol, fat_len, &entry, &bytes);
	CHECK(rc == FAT12_OK, "created DESKTOP.DB reads through FAT12 rails");
	CHECK(bytes == sizeof(expected_empty_db) &&
	      memcmp(g_db, expected_empty_db, sizeof(expected_empty_db)) == 0,
	      "created DESKTOP.DB equals independent 8-byte zero-record golden");
	CHECK(entry.attribute == DIR_ATTR_HIDDEN,
	      "created DESKTOP.DB carries the FAT hidden attribute");

	rc = desktop_trash_ensure(&vol, g_fat, fat_len, g_sector, g_cluster,
	                          &trash_state);
	CHECK(rc == FAT12_OK && trash_state == DESKTOP_TRASH_CREATE,
	      "absent TRASH directory is created");
	rc = fat12_find(&vol, g_sector, DESKTOP_TRASH_NAME, &entry);
	CHECK(rc == FAT12_OK &&
	      (entry.attribute & DIR_ATTR_DIRECTORY) != 0u,
	      "TRASH exists as a directory");

	rc = desktop_db_bootstrap(&vol, g_fat, fat_len, g_sector, g_cluster,
	                          g_db, sizeof(g_db), &state, &reason);
	CHECK(rc == FAT12_OK && state == DESKTOP_DB_BOOT_OK,
	      "second bootstrap reads the persisted DB without rewriting it");
	rc = desktop_trash_ensure(&vol, g_fat, fat_len, g_sector, g_cluster,
	                          &trash_state);
	CHECK(rc == FAT12_OK && trash_state == DESKTOP_TRASH_OK,
	      "second bootstrap preserves the existing TRASH directory");

	/* Replace the file with a bad-magic header through the real create/write
	 * rails, then prove bootstrap detects corruption and regenerates defaults. */
	{
		static const uint8_t corrupt[DESKTOP_DB_HEADER_SIZE] = {
			'X', 'D', 'B', '1', 0x01u, 0x00u, 0x00u, 0x00u
		};
		uint32_t slot = 0u;
		rc = fat12_create(&vol, g_fat, fat_len, DESKTOP_DB_NAME,
		                  DIR_ATTR_HIDDEN, 0u, g_sector, g_cluster,
		                  &entry, &slot);
		CHECK(rc == FAT12_OK, "truncate DESKTOP.DB for corrupt-header leg");
		rc = fat12_write_file(&vol, g_fat, fat_len, slot, corrupt,
		                      sizeof(corrupt), g_sector, g_cluster);
		CHECK(rc == FAT12_OK, "write corrupt DESKTOP.DB header through FAT12");
		CHECK(desktop_db_validate(corrupt, sizeof(corrupt), &reason) ==
		      DESKTOP_DB_CORRUPT && reason == DESKTOP_DB_REASON_MAGIC,
		      "bad magic is detected before regeneration");
	}

	rc = desktop_db_bootstrap(&vol, g_fat, fat_len, g_sector, g_cluster,
	                          g_db, sizeof(g_db), &state, &reason);
	CHECK(rc == FAT12_OK && state == DESKTOP_DB_BOOT_REGEN &&
	      reason == DESKTOP_DB_REASON_MAGIC,
	      "corrupt DESKTOP.DB takes the loud regeneration path");
	rc = read_db(&vol, fat_len, &entry, &bytes);
	CHECK(rc == FAT12_OK && bytes == sizeof(expected_empty_db) &&
	      memcmp(g_db, expected_empty_db, sizeof(expected_empty_db)) == 0,
	      "regenerated DESKTOP.DB round-trips to the independent golden");

	blockdev_file_close(&bf);
	return TEST_SUMMARY("test_desktop_db");
}
