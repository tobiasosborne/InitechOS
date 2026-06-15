/*
 * harness/diff/fat_diff/test_fat16.c -- FAT16 read-path unit oracle (initech-z01).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (CLAUDE.md Law 2: the oracle is the truth, never false-green).
 *
 * RED->GREEN gate for the FAT16 READ path (beads initech-z01): mount a real
 * `mkfs.fat -F 16` non-partitioned image (hidden_sectors==0) through the REAL
 * artifact os/milton/fat12.c via the host file-backed blockdev, and assert:
 *
 *   (a) MOUNT + CLASSIFY: fat12_mount returns FAT12_OK and vol.fat_type ==
 *       FAT_TYPE_FAT16 (the rejection lifted; classification by cluster count).
 *       This is the M5 guard: a FAT16 volume must be CLASSIFIED FAT16, not
 *       decoded by the 12-bit path.
 *
 *   (b) ENUMERATION: the set of regular-file 8.3 names found in the root dir
 *       equals the fixture set {HELLO.TXT, CHAIN.TXT, BLOCK.BIN, BIGCHAIN.TXT}.
 *
 *   (c) READ-FILE (the load-bearing assertion): fat16_read / fat12_read_file
 *       (dispatching to the 16-bit decode) reproduces each fixture's bytes
 *       EXACTLY (vs the committed fixture golden). CHAIN.TXT (1600 bytes => a
 *       multi-cluster chain with a partial last cluster) is the RISK-5 case;
 *       BIGCHAIN.TXT (700060 bytes => 1368 clusters) is the long-chain case
 *       whose cluster pointers exceed 0xFF8 / 0x0FFF -- so the M2 (12-bit mask),
 *       M3 (0xFF8 EOC), and M5 (FAT12 decode) mutants all corrupt it.
 *
 *   (d) FAT16 ENTRY DECODE: fat16_next_cluster on a known chain returns the
 *       expected 16-bit links; the EOC link classifies as fat16_is_eoc and a
 *       16-bit pointer >= 0xFF8 (which the FAT12 predicate would mis-flag) does
 *       NOT classify as EOC.
 *
 *   (e) EDGE: fat12_read_file into an undersized buffer returns FAT12_ERR_BUFFER.
 *
 * Ref (Law 1): docs/research/fat16-ground-truth.md Sec 0-4; Microsoft FAT spec
 *   Sec 3.2. Image + fixture-dir paths are argv (Makefile) -> no host path baked
 *   in (Rule 11). ASCII-clean (Rule 12).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "test_assert.h"      /* seed/, on -Iseed           */
#include "fat12.h"            /* os/milton/, on -Ios/milton */
#include "blockdev_file.h"    /* fat_diff host backend       */

TEST_HARNESS();

/* Read an entire host file into `buf` (capacity cap). Returns byte count, or
 * -1 on error. The fixture files are the file-content golden (Law 2). */
static long read_whole_file(const char *path, uint8_t *buf, long cap)
{
	FILE *fp = fopen(path, "rb");
	long  n;
	if (fp == NULL) {
		return -1;
	}
	n = (long)fread(buf, 1u, (size_t)cap, fp);
	if (!feof(fp) && n == cap) {
		fclose(fp);
		return -1; /* file larger than cap -- test fixture sizing bug */
	}
	fclose(fp);
	return n;
}

/* The expected regular-file set on the FAT16 fixture image. HIGHCLUS.BIN is the
 * load-bearing high-cluster file: its chain crosses cluster 4088 (0xFF8) so the
 * M3 (0xFF8 EOC) / M4 (cluster-2 bias) / M5 (FAT12 decode) mutants corrupt it. */
static const char *EXPECT_NAMES[] = {
	"HELLO.TXT", "CHAIN.TXT", "BLOCK.BIN", "BIGCHAIN.TXT", "HIGHCLUS.BIN"
};

/* The deterministic byte pattern the mint writes into HIGHCLUS.BIN: byte i is
 * (i & 0xFF). Regenerated here in memory so the read check needs no golden file
 * (the build/highclus.bin is a build intermediate). */
#define HIGHCLUS_SIZE 2621440u
#define EXPECT_N ((int)(sizeof(EXPECT_NAMES) / sizeof(EXPECT_NAMES[0])))

typedef struct enum_ctx {
	int seen[EXPECT_N];
	int extra;            /* count of regular files NOT in the expected set */
} enum_ctx_t;

static int enum_cb(const dir_entry_t *e, void *user)
{
	enum_ctx_t *c = (enum_ctx_t *)user;
	char        name[FAT12_NAME83_MAX];
	int         i;

	if ((e->attribute & DIR_ATTR_VOLLABEL) != 0u) {
		return 0;
	}
	if ((e->attribute & DIR_ATTR_DIRECTORY) != 0u) {
		return 0;
	}
	fat12_format_83(e, name);
	for (i = 0; i < EXPECT_N; i++) {
		if (strcmp(name, EXPECT_NAMES[i]) == 0) {
			c->seen[i] = 1;
			return 0;
		}
	}
	c->extra++;
	return 0;
}

/* Read a fixture by name through the artifact (16-bit decode dispatched on
 * vol->fat_type) and compare byte-for-byte with the committed golden file.
 * Buffers are `static` so even BIGCHAIN's 700060 bytes need no large stack. */
static void check_file(const fat12_volume_t *vol, void *sector_buf,
                       const void *fat, uint32_t fat_len,
                       const char *name83, const char *golden_path)
{
	static uint8_t got[1024 * 1024];
	static uint8_t golden[1024 * 1024];
	uint8_t        cluster_buf[512];   /* sectors_per_cluster(1) * 512 */
	dir_entry_t    e;
	uint32_t       out_bytes = 0xFFFFFFFFu;
	long           glen;
	int            rc;
	char           msg[160];
	const uint8_t  GUARD = 0x7Eu;

	rc = fat12_find(vol, sector_buf, name83, &e);
	snprintf(msg, sizeof(msg), "fat12_find(%s) should return FAT12_OK", name83);
	CHECK(rc == FAT12_OK, msg);
	if (rc != FAT12_OK) {
		return;
	}

	glen = read_whole_file(golden_path, golden, (long)sizeof(golden));
	snprintf(msg, sizeof(msg), "read golden fixture %s", golden_path);
	CHECK(glen >= 0, msg);
	snprintf(msg, sizeof(msg), "%s golden size == dir file_size %u",
	         name83, e.file_size);
	CHECK(glen == (long)e.file_size, msg);

	/* Poison the guard just past file_size; out_buf_len = file_size + 1 so a
	 * correct read writes only file_size bytes (RISK-5). */
	memset(got, 0, sizeof(got));
	if (e.file_size < sizeof(got)) {
		got[e.file_size] = GUARD;
	}
	rc = fat12_read_file(vol, fat, fat_len, &e, got, e.file_size + 1u,
	                     cluster_buf, &out_bytes);
	snprintf(msg, sizeof(msg), "fat12_read_file(%s) should return FAT12_OK", name83);
	CHECK(rc == FAT12_OK, msg);
	snprintf(msg, sizeof(msg), "%s out_bytes == file_size %u", name83, e.file_size);
	CHECK(out_bytes == e.file_size, msg);
	snprintf(msg, sizeof(msg),
	         "%s RISK-5: guard past file_size untouched (no padding)", name83);
	CHECK(e.file_size >= sizeof(got) || got[e.file_size] == GUARD, msg);

	if (rc == FAT12_OK && glen == (long)e.file_size) {
		snprintf(msg, sizeof(msg),
		         "%s bytes read EXACTLY match the fixture (byte-for-byte)", name83);
		CHECK(memcmp(got, golden, (size_t)e.file_size) == 0, msg);
	}

	/* EDGE: undersized buffer -> FAT12_ERR_BUFFER (no overflow) -- only for a
	 * non-empty file. */
	if (e.file_size > 0u) {
		rc = fat12_read_file(vol, fat, fat_len, &e, got, e.file_size - 1u,
		                     cluster_buf, &out_bytes);
		snprintf(msg, sizeof(msg),
		         "%s undersized buffer -> FAT12_ERR_BUFFER", name83);
		CHECK(rc == FAT12_ERR_BUFFER, msg);
	}
}

/* Read HIGHCLUS.BIN (2.5 MB, chain crosses cluster 4088 = 0xFF8) and verify the
 * deterministic pattern byte-for-byte. This is the M3/M4/M5 trap: a chain whose
 * cluster numbers exceed 0xFF8 is mis-terminated by the FAT12 0xFF8 EOC threshold
 * (M3), mis-mapped by the omitted cluster-2 bias (M4), and mis-decoded by the
 * 12-bit path (M5). The read must reproduce (i & 0xFF) for every byte. */
static void check_highclus(const fat12_volume_t *vol, void *sector_buf,
                           const void *fat, uint32_t fat_len)
{
	static uint8_t got[HIGHCLUS_SIZE];
	uint8_t        cluster_buf[512];
	dir_entry_t    e;
	uint32_t       out_bytes = 0xFFFFFFFFu;
	uint32_t       i;
	int            rc;
	int            ok;

	rc = fat12_find(vol, sector_buf, "HIGHCLUS.BIN", &e);
	CHECK(rc == FAT12_OK, "fat12_find(HIGHCLUS.BIN) should return FAT12_OK");
	if (rc != FAT12_OK) {
		return;
	}
	CHECK(e.file_size == HIGHCLUS_SIZE,
	      "HIGHCLUS.BIN file_size == 2621440 (chain crosses cluster 0xFF8)");
	if (e.file_size != HIGHCLUS_SIZE) {
		return;
	}
	rc = fat12_read_file(vol, fat, fat_len, &e, got, HIGHCLUS_SIZE,
	                     cluster_buf, &out_bytes);
	CHECK(rc == FAT12_OK,
	      "fat12_read_file(HIGHCLUS.BIN) OK (16-bit chain past 0xFF8 walked)");
	CHECK(out_bytes == HIGHCLUS_SIZE, "HIGHCLUS.BIN out_bytes == 2621440");
	if (rc != FAT12_OK || out_bytes != HIGHCLUS_SIZE) {
		return;
	}
	ok = 1;
	for (i = 0u; i < HIGHCLUS_SIZE; i++) {
		if (got[i] != (uint8_t)(i & 0xFFu)) {
			ok = 0;
			break;
		}
	}
	CHECK(ok == 1,
	      "HIGHCLUS.BIN bytes == deterministic (i & 0xFF) pattern (M3/M4/M5 trap)");
}

int main(int argc, char **argv)
{
	const char     *img;
	const char     *fixdir;
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint8_t         sector_buf[512];
	static uint8_t  fat_buf[256 * 1024];  /* whole FAT16 FAT (sized for -F 16) */
	enum_ctx_t      ec;
	int             i;
	int             rc;
	char            path[512];

	if (argc != 3) {
		fprintf(stderr, "usage: %s <fat16_image> <fixture_dir>\n", argv[0]);
		return 2;
	}
	img    = argv[1];
	fixdir = argv[2];

	rc = blockdev_file_open(&bf, img);
	CHECK(rc == 0, "blockdev_file_open(fat16 image)");
	if (rc != 0) {
		return TEST_SUMMARY("test_fat16");
	}

	/* (a) MOUNT + CLASSIFY. */
	rc = fat12_mount(&vol, &bf.dev, sector_buf);
	CHECK(rc == FAT12_OK, "fat12_mount(FAT16 image) should return FAT12_OK (rejection lifted)");
	CHECK(vol.fat_type == FAT_TYPE_FAT16,
	      "vol.fat_type == FAT_TYPE_FAT16 (classified by cluster count, M5 guard)");
	CHECK(vol.total_clusters >= 4085u && vol.total_clusters < 65525u,
	      "total_clusters in the FAT16 range [4085, 65525)");
	CHECK(vol.bpb.hidden_sectors == 0u,
	      "hidden_sectors == 0 (NON-PARTITIONED single volume, z01 scope)");
	if (rc != FAT12_OK) {
		blockdev_file_close(&bf);
		return TEST_SUMMARY("test_fat16");
	}

	rc = fat12_read_fat(&vol, fat_buf, (uint32_t)sizeof(fat_buf));
	CHECK(rc == FAT12_OK, "fat12_read_fat(FAT16) should return FAT12_OK");

	/* (b) ENUMERATION. */
	memset(&ec, 0, sizeof(ec));
	rc = fat12_read_root_dir(&vol, sector_buf, enum_cb, &ec);
	CHECK(rc == FAT12_OK, "fat12_read_root_dir(FAT16) should return FAT12_OK");
	for (i = 0; i < EXPECT_N; i++) {
		char msg[96];
		snprintf(msg, sizeof(msg), "root dir contains %s", EXPECT_NAMES[i]);
		CHECK(ec.seen[i] == 1, msg);
	}
	CHECK(ec.extra == 0, "no UNEXPECTED regular files in the FAT16 root dir");

	/* (d) FAT16 ENTRY DECODE + classification predicates. A FAT16 cluster
	 * pointer >= 0xFF8 must NOT be EOC (the M3 failure); the EOC link must be. */
	CHECK(fat16_is_eoc(0xFFFFu) == 1, "fat16_is_eoc(0xFFFF) == 1 (canonical EOC)");
	CHECK(fat16_is_eoc(0xFFF8u) == 1, "fat16_is_eoc(0xFFF8) == 1 (EOC threshold)");
	CHECK(fat16_is_eoc(0x0FF8u) == 0,
	      "fat16_is_eoc(0x0FF8) == 0 (a valid cluster, NOT EOC -- the M3 trap)");
	CHECK(fat16_is_eoc(0x1388u) == 0,
	      "fat16_is_eoc(5000) == 0 (a 16-bit pointer the FAT12 0xFF8 thresh mis-flags)");
	CHECK(fat16_is_free(0x0000u) == 1, "fat16_is_free(0x0000) == 1");
	CHECK(fat16_is_bad(0xFFF7u) == 1, "fat16_is_bad(0xFFF7) == 1");
	{
		/* Walk HELLO.TXT's start cluster: a single-cluster file's first link is
		 * EOC. Then verify fat16_next_cluster on a multi-cluster file advances. */
		dir_entry_t e;
		uint16_t    nxt = 0u;
		rc = fat12_find(&vol, sector_buf, "CHAIN.TXT", &e);
		CHECK(rc == FAT12_OK, "fat12_find(CHAIN.TXT) for decode probe");
		if (rc == FAT12_OK) {
			rc = fat16_next_cluster(&vol, fat_buf, (uint32_t)sizeof(fat_buf),
			                        e.start_cluster, &nxt);
			CHECK(rc == FAT12_OK, "fat16_next_cluster(CHAIN start) OK");
			/* CHAIN.TXT is multi-cluster, so the first link is a normal pointer
			 * (>= 2), not EOC/free/bad. */
			CHECK(nxt >= FAT12_FIRST_DATA_CLUSTER && fat16_is_eoc(nxt) == 0,
			      "CHAIN.TXT first link is a normal forward pointer (not EOC)");
		}
	}

	/* (c) READ-FILE byte-for-byte for each fixture. */
	snprintf(path, sizeof(path), "%s/hello.txt", fixdir);
	check_file(&vol, sector_buf, fat_buf, (uint32_t)sizeof(fat_buf),
	           "HELLO.TXT", path);
	snprintf(path, sizeof(path), "%s/chain.txt", fixdir);
	check_file(&vol, sector_buf, fat_buf, (uint32_t)sizeof(fat_buf),
	           "CHAIN.TXT", path);
	snprintf(path, sizeof(path), "%s/block.bin", fixdir);
	check_file(&vol, sector_buf, fat_buf, (uint32_t)sizeof(fat_buf),
	           "BLOCK.BIN", path);
	snprintf(path, sizeof(path), "%s/bigchain.txt", fixdir);
	check_file(&vol, sector_buf, fat_buf, (uint32_t)sizeof(fat_buf),
	           "BIGCHAIN.TXT", path);

	/* High-cluster file: chain crosses cluster 0xFF8 -- the M3/M4/M5 trap. */
	check_highclus(&vol, sector_buf, fat_buf, (uint32_t)sizeof(fat_buf));

	blockdev_file_close(&bf);
	return TEST_SUMMARY("test_fat16");
}
