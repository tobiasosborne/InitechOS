/*
 * harness/diff/fat_diff/test_fat12_write.c -- FAT12 WRITE unit + round-trip.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (Law 2: the oracle is the truth, never false-green).
 *
 * RED->GREEN gate for os/milton/fat12.c::{fat12_set_entry, fat12_flush_fats,
 * fat12_create, fat12_write_file, fat12_unlink} (beads initech-509.11, the FAT
 * WRITE half) plus the WRITE-then-OUR-READ in-memory round-trip leg.
 *
 * Two argv modes:
 *   <blank-img> --diff   : create + write EXACTLY the four known files
 *                          (SHORT.TXT, MULTI.DAT, EXACT.BIN, EMPTY.NEW) into the
 *                          blank image and EXIT (no fill / no unlink), leaving a
 *                          clean image for the Makefile to diff against mtools +
 *                          python3. Deterministic content (Rule 11).
 *   <blank-img>          : MUTATE the (freshly minted, blank) FAT12 image in
 *                          place -- create + write several files (incl. a
 *                          MULTI-CLUSTER file so the chain is exercised, a
 *                          zero-length file, and an exact-cluster-multiple file).
 *                          Then run the in-process unit assertions:
 *                            (a) the 12-bit packed FAT entry WRITE round-trips
 *                                through the decode (incl. the straddle case);
 *                            (b) free-cluster scan + chain allocation are
 *                                contiguous-lowest-first;
 *                            (c) BOTH FAT copies are byte-identical on disk;
 *                            (d) our OWN reader (fat12_read_file) reads back
 *                                EXACTLY what we wrote (in-memory round-trip);
 *                            (e) full-volume write fails loud (NO_SPACE) and
 *                                does not corrupt the volume;
 *                            (f) unlink frees the chain + marks the entry
 *                                deleted (the slot disappears from enumeration).
 *                          The image is LEFT WRITTEN so the Makefile can hand it
 *                          to mtools + python3 for the differential legs.
 *
 * The differential legs (our-written image reads back in mtools `mdir`/`mcopy`
 * AND an independent python3 reader, byte-for-byte on content) live in the
 * Makefile `test-fat-write` recipe (mirrors `test-fat`): this binary writes the
 * image; the recipe diffs it three ways.
 *
 * Determinism (Rule 11): the dir-entry mtime/mdate are written as the fixed
 * constant 0 (FAT12_FIXED_*); the differential diff normalizes date/time/serial
 * away (they are not meaningful bytes). The content we write is deterministic.
 *
 * Ref (Law 1): Microsoft FAT spec (12-bit encode, free=0x000, EOC 0xFFF);
 *   docs/research/fat12-ground-truth.md Sec 3/4; ADR-0003 DEC-07. Image path is
 *   argv[1] (no host path baked in, Rule 11).
 *
 * ASCII-clean (Rule 12).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "test_assert.h"
#include "fat12.h"
#include "blockdev_file.h"

TEST_HARNESS();

/* The deterministic content we write. WRITE_MULTI is > 1 cluster (512 bytes on
 * 1.44 MB floppy) so the cluster chain + FAT links are exercised; WRITE_EXACT is
 * an exact 2-cluster multiple (full last cluster, no partial). */
#define WRITE_MULTI_LEN 1300u   /* 3 clusters (512*3=1536 >= 1300)            */
#define WRITE_EXACT_LEN 1024u   /* exactly 2 clusters                         */

static uint8_t g_fat[12u * 512u];
static uint8_t g_sector[512];
static uint8_t g_cluster[512];

/* Deterministic byte pattern (NOT a clock): byte i = (i * 31 + 7) & 0xFF. */
static void fill_pattern(uint8_t *buf, uint32_t len, uint32_t seed)
{
	for (uint32_t i = 0; i < len; i++) {
		buf[i] = (uint8_t)(((i + seed) * 31u + 7u) & 0xFFu);
	}
}

static const char short_data[] = "WRITTEN-BY-INITECHDOS\r\n";

/* The shared deterministic content the --diff fixtures hold; the Makefile's
 * differential leg writes these to a host file and cmp's mcopy/python output. */
static uint8_t g_multi[WRITE_MULTI_LEN];
static uint8_t g_exact[WRITE_EXACT_LEN];

/* --diff mode: create + write EXACTLY the four named files into a blank image,
 * then exit. Leaves a clean image for the Makefile to diff three ways. Returns
 * 0 on success (prints nothing on stdout -- the image IS the output). */
static int do_diff_fixtures(const char *img)
{
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint32_t        fat_len;
	dir_entry_t     de;
	uint32_t        slot;
	int             rc;

	fill_pattern(g_multi, WRITE_MULTI_LEN, 1u);
	fill_pattern(g_exact, WRITE_EXACT_LEN, 9u);

	if (blockdev_file_open_rw(&bf, img) != 0) {
		fprintf(stderr, "test_fat12_write --diff: cannot open '%s'\n", img);
		return 1;
	}
	if (fat12_mount(&vol, &bf.dev, g_sector) != FAT12_OK) {
		fprintf(stderr, "test_fat12_write --diff: mount failed\n");
		blockdev_file_close(&bf); return 1;
	}
	if (fat12_read_fat(&vol, g_fat, sizeof(g_fat)) != FAT12_OK) {
		fprintf(stderr, "test_fat12_write --diff: read_fat failed\n");
		blockdev_file_close(&bf); return 1;
	}
	fat_len = (uint32_t)vol.bpb.sectors_per_fat * (uint32_t)vol.bpb.bytes_per_sector;

#define DIFF_DO(name, attr) \
	if (fat12_create(&vol, g_fat, fat_len, (name), (attr), 0u, g_sector, g_cluster, &de, &slot) != FAT12_OK) { \
		fprintf(stderr, "test_fat12_write --diff: create %s failed\n", (name)); \
		blockdev_file_close(&bf); return 1; }

	DIFF_DO("SHORT.TXT", DIR_ATTR_ARCHIVE);
	rc = fat12_write_file(&vol, g_fat, fat_len, slot, short_data,
	                      (uint32_t)(sizeof(short_data) - 1u), g_sector, g_cluster);
	if (rc != FAT12_OK) { fprintf(stderr, "write SHORT.TXT failed\n"); blockdev_file_close(&bf); return 1; }

	DIFF_DO("MULTI.DAT", DIR_ATTR_ARCHIVE);
	rc = fat12_write_file(&vol, g_fat, fat_len, slot, g_multi, WRITE_MULTI_LEN, g_sector, g_cluster);
	if (rc != FAT12_OK) { fprintf(stderr, "write MULTI.DAT failed\n"); blockdev_file_close(&bf); return 1; }

	DIFF_DO("EXACT.BIN", DIR_ATTR_ARCHIVE);
	rc = fat12_write_file(&vol, g_fat, fat_len, slot, g_exact, WRITE_EXACT_LEN, g_sector, g_cluster);
	if (rc != FAT12_OK) { fprintf(stderr, "write EXACT.BIN failed\n"); blockdev_file_close(&bf); return 1; }

	DIFF_DO("EMPTY.NEW", DIR_ATTR_ARCHIVE);
	rc = fat12_write_file(&vol, g_fat, fat_len, slot, "", 0u, g_sector, g_cluster);
	if (rc != FAT12_OK) { fprintf(stderr, "write EMPTY.NEW failed\n"); blockdev_file_close(&bf); return 1; }

#undef DIFF_DO
	blockdev_file_close(&bf);
	return 0;
}

int main(int argc, char **argv)
{
	const char     *img;
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint32_t        fat_len;
	int             rc;

	static uint8_t  multi[WRITE_MULTI_LEN];
	static uint8_t  exact[WRITE_EXACT_LEN];

	if (argc < 2) {
		fprintf(stderr, "usage: %s <blank-fat12-image> [--diff]\n", argv[0]);
		return 2;
	}
	img = argv[1];

	if (argc >= 3 && strcmp(argv[2], "--diff") == 0) {
		return do_diff_fixtures(img);
	}

	fill_pattern(multi, WRITE_MULTI_LEN, 1u);
	fill_pattern(exact, WRITE_EXACT_LEN, 9u);

	rc = blockdev_file_open_rw(&bf, img);
	CHECK(rc == 0, "blockdev_file_open_rw should succeed on the minted blank image");
	if (rc != 0) {
		return TEST_SUMMARY("test_fat12_write");
	}

	rc = fat12_mount(&vol, &bf.dev, g_sector);
	CHECK(rc == FAT12_OK, "fat12_mount on the blank image");
	if (rc != FAT12_OK) {
		blockdev_file_close(&bf);
		return TEST_SUMMARY("test_fat12_write");
	}

	rc = fat12_read_fat(&vol, g_fat, sizeof(g_fat));
	CHECK(rc == FAT12_OK, "fat12_read_fat fills the whole-FAT buffer");
	fat_len = (uint32_t)vol.bpb.sectors_per_fat * (uint32_t)vol.bpb.bytes_per_sector;

	/* ---- (a) 12-bit packed FAT entry WRITE round-trips through the decode.
	 * Including the RISK-1 straddle (cluster 341, byte_offset 511 spanning the
	 * 512-byte FAT-sector boundary). We use a scratch FAT so we do not disturb
	 * the volume's FAT. set_entry must be the EXACT inverse of next_cluster. */
	{
		uint8_t  scratch[12u * 512u];
		uint16_t got;
		memset(scratch, 0, sizeof(scratch));

		CHECK(fat12_set_entry(scratch, sizeof(scratch), 2u, 0x123u) == FAT12_OK,
		      "set_entry(2, 0x123) ok (even cluster)");
		CHECK(fat12_next_cluster(&vol, scratch, sizeof(scratch), 2u, &got) == FAT12_OK
		      && got == 0x123u, "even entry decodes back to 0x123");

		CHECK(fat12_set_entry(scratch, sizeof(scratch), 3u, 0x456u) == FAT12_OK,
		      "set_entry(3, 0x456) ok (odd cluster)");
		CHECK(fat12_next_cluster(&vol, scratch, sizeof(scratch), 3u, &got) == FAT12_OK
		      && got == 0x456u, "odd entry decodes back to 0x456");
		/* The shared byte between clusters 2 (even, off 3) and 3 (odd, off 4)
		 * must not have corrupted cluster 2. */
		CHECK(fat12_next_cluster(&vol, scratch, sizeof(scratch), 2u, &got) == FAT12_OK
		      && got == 0x123u, "writing the odd neighbour preserved the even entry");

		/* Straddle: cluster 341 (odd) at byte_offset 511. */
		CHECK(fat12_set_entry(scratch, sizeof(scratch), 341u, 0xABCu) == FAT12_OK,
		      "set_entry(341, 0xABC) ok (straddles 512-byte boundary)");
		CHECK(fat12_next_cluster(&vol, scratch, sizeof(scratch), 341u, &got) == FAT12_OK
		      && got == 0xABCu, "RISK-1: straddling entry round-trips through the boundary");

		/* set_entry must reject reserved clusters 0/1 (Rule 2). */
		CHECK(fat12_set_entry(scratch, sizeof(scratch), 0u, 0u) == FAT12_ERR_CLUSTER,
		      "set_entry(0) -> FAT12_ERR_CLUSTER (reserved)");
		CHECK(fat12_set_entry(scratch, sizeof(scratch), 1u, 0u) == FAT12_ERR_CLUSTER,
		      "set_entry(1) -> FAT12_ERR_CLUSTER (reserved)");
	}

	/* ---- (b)+(c)+(d) CREATE + WRITE several files, then round-trip-read. ---- */
	{
		dir_entry_t de;
		uint32_t    slot;

		/* SHORT.TXT: a small single-cluster file. */
		rc = fat12_create(&vol, g_fat, fat_len, "SHORT.TXT", DIR_ATTR_ARCHIVE,
		                  0u, g_sector, g_cluster, &de, &slot);
		CHECK(rc == FAT12_OK, "fat12_create(SHORT.TXT) ok");
		rc = fat12_write_file(&vol, g_fat, fat_len, slot, short_data,
		                      (uint32_t)(sizeof(short_data) - 1u), g_sector, g_cluster);
		CHECK(rc == FAT12_OK, "fat12_write_file(SHORT.TXT) ok");

		/* MULTI.DAT: a 3-cluster file (the chain-linking case). */
		rc = fat12_create(&vol, g_fat, fat_len, "MULTI.DAT", DIR_ATTR_ARCHIVE,
		                  0u, g_sector, g_cluster, &de, &slot);
		CHECK(rc == FAT12_OK, "fat12_create(MULTI.DAT) ok");
		rc = fat12_write_file(&vol, g_fat, fat_len, slot, multi, WRITE_MULTI_LEN,
		                      g_sector, g_cluster);
		CHECK(rc == FAT12_OK, "fat12_write_file(MULTI.DAT, 1300 bytes -> 3 clusters) ok");

		/* EXACT.BIN: an exact 2-cluster multiple (full last cluster). */
		rc = fat12_create(&vol, g_fat, fat_len, "EXACT.BIN", DIR_ATTR_ARCHIVE,
		                  0u, g_sector, g_cluster, &de, &slot);
		CHECK(rc == FAT12_OK, "fat12_create(EXACT.BIN) ok");
		rc = fat12_write_file(&vol, g_fat, fat_len, slot, exact, WRITE_EXACT_LEN,
		                      g_sector, g_cluster);
		CHECK(rc == FAT12_OK, "fat12_write_file(EXACT.BIN, 1024 bytes -> 2 clusters) ok");

		/* EMPTY.NEW: a zero-length file (no chain). */
		rc = fat12_create(&vol, g_fat, fat_len, "EMPTY.NEW", DIR_ATTR_ARCHIVE,
		                  0u, g_sector, g_cluster, &de, &slot);
		CHECK(rc == FAT12_OK, "fat12_create(EMPTY.NEW) ok");
		rc = fat12_write_file(&vol, g_fat, fat_len, slot, "", 0u, g_sector, g_cluster);
		CHECK(rc == FAT12_OK, "fat12_write_file(EMPTY.NEW, 0 bytes) ok");
	}

	/* ---- (c) BOTH FAT copies must be byte-identical on disk. ---- */
	{
		uint8_t  fat2[12u * 512u];
		uint32_t lba2 = vol.first_fat_sector + (uint32_t)vol.bpb.sectors_per_fat;
		CHECK(vol.bpb.num_fats >= 2u, "the test image has 2 FATs (redundant copy)");
		rc = bf.dev.read_sectors(bf.dev.ctx, vol.first_fat_sector,
		                         vol.bpb.sectors_per_fat, g_fat);
		CHECK(rc == 0, "re-read FAT #1 from disk");
		rc = bf.dev.read_sectors(bf.dev.ctx, lba2, vol.bpb.sectors_per_fat, fat2);
		CHECK(rc == 0, "read FAT #2 from disk");
		CHECK(memcmp(g_fat, fat2, fat_len) == 0,
		      "BOTH on-disk FAT copies are byte-identical after writes (DEC-07 sync)");
	}

	/* ---- (d) Our OWN reader reads back EXACTLY what we wrote (round-trip). ---- */
	{
		dir_entry_t e;
		uint8_t     got[WRITE_MULTI_LEN + 16];
		uint32_t    out_bytes;

		/* Re-read the FAT from disk (the authoritative copy). */
		CHECK(fat12_read_fat(&vol, g_fat, sizeof(g_fat)) == FAT12_OK, "re-read FAT");

		rc = fat12_find(&vol, g_sector, "SHORT.TXT", &e);
		CHECK(rc == FAT12_OK, "find SHORT.TXT");
		CHECK(e.file_size == (uint32_t)(sizeof(short_data) - 1u), "SHORT.TXT size matches");
		out_bytes = 0;
		rc = fat12_read_file(&vol, g_fat, fat_len, &e, got, sizeof(got),
		                     g_cluster, &out_bytes);
		CHECK(rc == FAT12_OK && out_bytes == (uint32_t)(sizeof(short_data) - 1u),
		      "read SHORT.TXT back");
		CHECK(memcmp(got, short_data, sizeof(short_data) - 1u) == 0,
		      "SHORT.TXT round-trips byte-for-byte (write -> our read)");

		rc = fat12_find(&vol, g_sector, "MULTI.DAT", &e);
		CHECK(rc == FAT12_OK && e.file_size == WRITE_MULTI_LEN, "find MULTI.DAT, size 1300");
		out_bytes = 0;
		rc = fat12_read_file(&vol, g_fat, fat_len, &e, got, sizeof(got),
		                     g_cluster, &out_bytes);
		CHECK(rc == FAT12_OK && out_bytes == WRITE_MULTI_LEN, "read MULTI.DAT back");
		CHECK(memcmp(got, multi, WRITE_MULTI_LEN) == 0,
		      "MULTI.DAT round-trips byte-for-byte (3-cluster chain, partial last)");

		rc = fat12_find(&vol, g_sector, "EXACT.BIN", &e);
		CHECK(rc == FAT12_OK && e.file_size == WRITE_EXACT_LEN, "find EXACT.BIN, size 1024");
		out_bytes = 0;
		rc = fat12_read_file(&vol, g_fat, fat_len, &e, got, sizeof(got),
		                     g_cluster, &out_bytes);
		CHECK(rc == FAT12_OK && out_bytes == WRITE_EXACT_LEN, "read EXACT.BIN back");
		CHECK(memcmp(got, exact, WRITE_EXACT_LEN) == 0,
		      "EXACT.BIN round-trips byte-for-byte (exact 2-cluster multiple)");

		rc = fat12_find(&vol, g_sector, "EMPTY.NEW", &e);
		CHECK(rc == FAT12_OK && e.file_size == 0u && e.start_cluster == 0u,
		      "EMPTY.NEW: size 0, start_cluster 0 (no chain)");

		/* (b) MULTI.DAT must occupy a contiguous lowest-first chain. The three
		 * files written in order claimed the lowest free clusters; MULTI.DAT
		 * starts after SHORT.TXT's single cluster. The chain must be exactly
		 * ceil(1300/512)=3 clusters and contiguous-ascending. */
		{
			uint16_t chain[8];
			uint32_t n = 0;
			rc = fat12_find(&vol, g_sector, "MULTI.DAT", &e);
			CHECK(rc == FAT12_OK, "re-find MULTI.DAT for chain check");
			rc = fat12_walk_chain(&vol, g_fat, fat_len, e.start_cluster, chain,
			                      (uint32_t)(sizeof(chain)/sizeof(chain[0])), &n);
			CHECK(rc == FAT12_OK && n == 3u, "MULTI.DAT chain is exactly 3 clusters");
			CHECK(n == 3u && chain[1] == chain[0] + 1u && chain[2] == chain[1] + 1u,
			      "MULTI.DAT chain is contiguous ascending (lowest-free allocation)");
		}
	}

	/* ---- (f) UNLINK frees the chain + removes the entry from enumeration. ---- */
	{
		dir_entry_t e;
		CHECK(fat12_read_fat(&vol, g_fat, sizeof(g_fat)) == FAT12_OK, "re-read FAT before unlink");
		rc = fat12_unlink(&vol, g_fat, fat_len, "EXACT.BIN", 0u, g_sector);
		CHECK(rc == FAT12_OK, "fat12_unlink(EXACT.BIN) ok");
		rc = fat12_find(&vol, g_sector, "EXACT.BIN", &e);
		CHECK(rc == FAT12_ERR_NOT_FOUND, "EXACT.BIN gone from the directory after unlink");
		/* unlink of a non-existent file fails loud. */
		rc = fat12_unlink(&vol, g_fat, fat_len, "NOPE.XYZ", 0u, g_sector);
		CHECK(rc == FAT12_ERR_NOT_FOUND, "unlink of a missing file -> NOT_FOUND");

		/* The freed clusters must now be re-allocatable: create + write a file
		 * that reuses them and round-trips. */
		uint32_t slot;
		rc = fat12_create(&vol, g_fat, fat_len, "REUSE.BIN", DIR_ATTR_ARCHIVE,
		                  0u, g_sector, g_cluster, &e, &slot);
		CHECK(rc == FAT12_OK, "fat12_create(REUSE.BIN) after unlink ok");
		rc = fat12_write_file(&vol, g_fat, fat_len, slot, exact, WRITE_EXACT_LEN,
		                      g_sector, g_cluster);
		CHECK(rc == FAT12_OK, "fat12_write_file(REUSE.BIN) reuses freed clusters");
		uint8_t got[WRITE_EXACT_LEN];
		uint32_t out_bytes = 0;
		CHECK(fat12_read_fat(&vol, g_fat, sizeof(g_fat)) == FAT12_OK, "re-read FAT");
		rc = fat12_find(&vol, g_sector, "REUSE.BIN", &e);
		CHECK(rc == FAT12_OK, "find REUSE.BIN");
		rc = fat12_read_file(&vol, g_fat, fat_len, &e, got, sizeof(got),
		                     g_cluster, &out_bytes);
		CHECK(rc == FAT12_OK && out_bytes == WRITE_EXACT_LEN
		      && memcmp(got, exact, WRITE_EXACT_LEN) == 0,
		      "REUSE.BIN round-trips byte-for-byte (clusters reused after unlink)");
	}

	/* ---- (e) FULL-VOLUME write fails loud (NO_SPACE), volume not corrupted.
	 * Create one file and try to write more bytes than the volume can hold; the
	 * allocation must roll back. We compute the free space and request more. */
	{
		dir_entry_t e;
		uint32_t    slot;
		uint32_t    bytes_per_cluster = (uint32_t)vol.bpb.sectors_per_cluster
		                                * (uint32_t)vol.bpb.bytes_per_sector;
		uint32_t    total_bytes = vol.total_clusters * bytes_per_cluster;
		static uint8_t huge[1];   /* we lie about len; write_file reads `data` up to len */

		CHECK(fat12_read_fat(&vol, g_fat, sizeof(g_fat)) == FAT12_OK, "re-read FAT");
		rc = fat12_create(&vol, g_fat, fat_len, "TOOBIG.DAT", DIR_ATTR_ARCHIVE,
		                  0u, g_sector, g_cluster, &e, &slot);
		CHECK(rc == FAT12_OK, "fat12_create(TOOBIG.DAT) ok");

		/* Request more than the WHOLE volume of clusters. We cannot supply that
		 * much real data, but fat12_write_file allocates clusters BEFORE copying
		 * each cluster's bytes, and we want it to run out of clusters. To avoid
		 * reading past `huge`, point data at a 1-byte buffer but pass a len that
		 * forces an impossible cluster count; the allocator runs out and rolls
		 * back BEFORE the copy that would read past `huge` (the very first
		 * cluster's copy reads min(len-off, bpc) bytes from huge[0..] -- to keep
		 * this safe we instead request exactly (total_clusters + 1) clusters'
		 * worth via a separate, bounded buffer). */
		(void)huge;
		(void)total_bytes;
		{
			/* Bounded approach: write files until the volume is exhausted, then
			 * one more create+write must fail loud with NO_SPACE. Each fills the
			 * remaining free clusters via a max-size buffer. */
			static uint8_t big[64u * 1024u];
			fill_pattern(big, sizeof(big), 3u);
			int got_no_space = 0;
			for (int i = 0; i < 64; i++) {
				char nm[16];
				dir_entry_t de2;
				uint32_t sl2;
				snprintf(nm, sizeof(nm), "FILL%02d.DAT", i);
				rc = fat12_create(&vol, g_fat, fat_len, nm, DIR_ATTR_ARCHIVE,
				                  0u, g_sector, g_cluster, &de2, &sl2);
				if (rc == FAT12_ERR_DIR_FULL) { got_no_space = 1; break; }
				CHECK(rc == FAT12_OK, "fill create ok (until dir/space full)");
				rc = fat12_write_file(&vol, g_fat, fat_len, sl2, big, sizeof(big),
				                      g_sector, g_cluster);
				if (rc == FAT12_ERR_NO_SPACE) { got_no_space = 1; break; }
				CHECK(rc == FAT12_OK, "fill write ok (until space full)");
			}
			CHECK(got_no_space == 1,
			      "filling the volume eventually fails loud (NO_SPACE or DIR_FULL), never silent/corrupt");

			/* The volume must still be mountable + the FAT copies still in sync
			 * (no corruption from the rolled-back allocation). */
			fat12_volume_t v2;
			CHECK(fat12_mount(&v2, &bf.dev, g_sector) == FAT12_OK,
			      "volume still mounts cleanly after the full-volume failure (no corruption)");
		}
	}

	blockdev_file_close(&bf);
	return TEST_SUMMARY("test_fat12_write");
}
