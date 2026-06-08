/*
 * harness/diff/fat_diff/test_fat12_bpb.c -- BPB parse / geometry oracle.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- a non-zero exit on any failed
 * check (CLAUDE.md Law 2: the oracle is the truth, never false-green).
 *
 * This is the RED->GREEN gate for os/milton/fat12.c::fat12_mount. It opens a
 * minted 1.44 MB FAT12 image via the host file-backed blockdev
 * (harness/diff/fat_diff/blockdev_file.c), mounts it through the REAL artifact
 * fat12.c, and asserts EVERY BPB field + EVERY derived-geometry value equals
 * the constants independently verified by the orchestrator and recorded in
 * docs/research/fat12-ground-truth.md (Sec 1 + Sec 2), and that the volume
 * classifies as FAT12.
 *
 * Ref (Law 1): the verified mformat-4.0.43 1.44 MB BPB constants (brief Sec 1);
 *   the derived-geometry formulas (brief Sec 2). The image path is passed as
 *   argv[1] by the Makefile mint rule (no host path baked in -> Rule 11).
 *
 * ASCII-clean (Rule 12).
 */

#include <stdio.h>
#include <string.h>

#include "test_assert.h"      /* seed/, on -Iseed       */
#include "fat12.h"            /* os/milton/, on -Ios/milton */
#include "blockdev_file.h"    /* fat_diff host backend   */

TEST_HARNESS();

/* Compare an N-byte BPB byte field against an exact expected literal. */
static int bytes_eq(const uint8_t *got, const char *want, size_t n)
{
	return memcmp(got, want, n) == 0;
}

int main(int argc, char **argv)
{
	const char     *img;
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint8_t         sector_buf[512];
	int             rc;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <fat12-image>\n", argv[0]);
		return 2;
	}
	img = argv[1];

	rc = blockdev_file_open(&bf, img);
	CHECK(rc == 0, "blockdev_file_open should succeed on the minted image");
	if (rc != 0) {
		return TEST_SUMMARY("test_fat12_bpb");
	}

	rc = fat12_mount(&vol, &bf.dev, sector_buf);
	CHECK(rc == FAT12_OK, "fat12_mount should return FAT12_OK on a valid 1.44MB FAT12 image");

	/* ---- BPB proper: verified constants (brief Sec 1) ---- */
	CHECK(vol.bpb.bytes_per_sector    == 512,  "bytes_per_sector == 512");
	CHECK(vol.bpb.sectors_per_cluster == 1,    "sectors_per_cluster == 1");
	CHECK(vol.bpb.reserved_sectors    == 1,    "reserved_sectors == 1");
	CHECK(vol.bpb.num_fats            == 2,    "num_fats == 2");
	CHECK(vol.bpb.root_entry_count    == 224,  "root_entry_count == 224");
	CHECK(vol.bpb.total_sectors_16    == 2880, "total_sectors_16 == 2880");
	CHECK(vol.bpb.media_descriptor    == 0xF0, "media_descriptor == 0xF0");
	CHECK(vol.bpb.sectors_per_fat     == 9,    "sectors_per_fat == 9");
	CHECK(vol.bpb.sectors_per_track   == 18,   "sectors_per_track == 18");
	CHECK(vol.bpb.num_heads           == 2,    "num_heads == 2");
	CHECK(vol.bpb.hidden_sectors      == 0,    "hidden_sectors == 0");

	/* OEM name "MTOO4043" (mtools 4.0.43; brief Sec 1). */
	CHECK(bytes_eq(vol.bpb.oem_name, "MTOO4043", 8), "oem_name == \"MTOO4043\"");

	/* Extended BPB present (boot_sig 0x29) with FAT12 fs_type string. */
	CHECK(vol.bpb.boot_sig == 0x29, "extended boot_sig == 0x29");
	CHECK(bytes_eq(vol.bpb.fs_type, "FAT12   ", 8), "fs_type == \"FAT12   \"");

	/* ---- Derived geometry: verified values (brief Sec 2) ---- */
	CHECK(vol.first_fat_sector  == 1,    "first_fat_sector == 1");
	CHECK(vol.root_dir_sector   == 19,   "root_dir_sector == 19 (1 + 2*9)");
	CHECK(vol.root_dir_sectors  == 14,   "root_dir_sectors == 14 (224*32/512)");
	CHECK(vol.first_data_sector == 33,   "first_data_sector == 33 (19 + 14)");
	CHECK(vol.total_clusters    == 2847, "total_clusters == 2847 (2880-33)");

	/* ---- FAT type classification (brief Sec 2; 2847 < 4085 => FAT12) ---- */
	CHECK(vol.fat_type == FAT_TYPE_FAT12, "fat_type == FAT12 (2847 < 4085)");

	/* Spot-check the cluster->LBA mapping macro: cluster 2 -> LBA 33. */
	CHECK(BPB_CLUSTER_LBA(&vol.bpb, 2u) == 33u, "BPB_CLUSTER_LBA(2) == 33");
	CHECK(BPB_CLUSTER_LBA(&vol.bpb, 3u) == 34u, "BPB_CLUSTER_LBA(3) == 34");

	blockdev_file_close(&bf);
	return TEST_SUMMARY("test_fat12_bpb");
}
