/*
 * harness/diff/fat_diff/test_fat16_window.c -- WINDOWED/streaming FAT-sector
 * read oracle (beads initech-d27i).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (CLAUDE.md Law 2: the oracle is the truth, never false-green).
 *
 * THE GAP THIS GATE CLOSES (beads initech-d27i): z01 landed the FAT16 DECODE
 * layer (fat16_next_cluster + fat_type dispatch) but a real FAT16 volume still
 * cannot MOUNT in-kernel -- the whole-FAT buffer g_fat[12*512] (~6 KB, FAT12-
 * floppy-sized) is far smaller than a FAT16 FAT (8 MB volume => 64 sectors/FAT
 * => 32 KB). This gate exercises the WINDOWED FAT-sector read that fetches only
 * the FAT sector(s) holding the current cluster's entry (mirroring dao's
 * streaming data-region walk), so a FAT16 volume reads with a TINY in-memory
 * FAT window (>= 2 sectors), NOT the whole FAT.
 *
 * ASSERTIONS:
 *
 *   (a) FAT16 WINDOWED READ: mount the real `mkfs.fat -F 16` fixture, install a
 *       2-SECTOR FAT window (vol.fat_window) -- NO whole-FAT buffer -- and read
 *       each fixture byte-for-byte vs the committed golden. The chain walkers
 *       (fat12_read_file, dispatching to the 16-bit decode) must produce
 *       IDENTICAL bytes to the whole-FAT path. BIGCHAIN.TXT (1368 clusters) and
 *       HIGHCLUS.BIN (5120 clusters, chain crosses cluster 0xFF8) force the
 *       window to slide across MANY FAT sectors mid-walk.
 *
 *   (b) FAT12 STRADDLE BYTE-IDENTICAL (the load-bearing hazard): a FAT12 12-bit
 *       entry can STRADDLE a 512-byte FAT-sector boundary (off (c*3)/2 == k*512+
 *       511). The windowed read MUST fetch BOTH sectors when an entry straddles,
 *       else a naive single-sector window mis-decodes the high nibble. BIGCHAIN
 *       on the 1.44 MB FAT12 floppy occupies clusters 2..1369, crossing the
 *       straddle clusters 341 (off 511) and 682 (off 1023). Read BIGCHAIN
 *       through the FAT12 WINDOW and assert it is byte-identical to the fixture.
 *
 *   (c) DIRECT fat12_read_fat_sector PROBE: for a known STRADDLING FAT12 cluster
 *       (341 / 682) the windowed next-cluster link reproduces the SAME value the
 *       whole-FAT buffer yields at the absolute offset -- proving the straddle
 *       fetch is correct, not merely that the file happened to read clean.
 *
 * Ref (Law 1): docs/research/fat16-ground-truth.md Sec 0/2/3 (the decode is the
 *   only per-type difference; identical geometry); docs/research/
 *   fat12-ground-truth.md RISK-1 (12-bit entries straddle sector boundaries) +
 *   Sec 3 (decode); Microsoft FAT spec Sec 3.2. Image + fixture-dir paths are
 *   argv (Makefile) -> no host path baked in (Rule 11). ASCII-clean (Rule 12).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "test_assert.h"      /* seed/, on -Iseed           */
#include "fat12.h"            /* os/milton/, on -Ios/milton */
#include "blockdev_file.h"    /* fat_diff host backend       */

TEST_HARNESS();

/* The window buffer: >= 2 sectors so a straddling FAT12 entry (off k*512+511,
 * spanning sectors k and k+1) fits in a single windowed read (beads d27i). This
 * mirrors the SHRUNK kernel g_fat: a 2-sector window, NOT the whole FAT. */
#define WIN_SECTORS 2u

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
		return -1;
	}
	fclose(fp);
	return n;
}

/* Read a fixture by name through the WINDOWED FAT (vol->fat_window set) and
 * compare byte-for-byte with the committed golden file. The fat/fat_len args to
 * fat12_read_file are IGNORED in windowed mode (the window carries its own
 * buffer); pass NULL/0 to PROVE the read never touches a whole-FAT buffer. */
static void check_file_windowed(const fat12_volume_t *vol, void *sector_buf,
                                const char *name83, const char *golden_path)
{
	static uint8_t got[1024 * 1024];
	static uint8_t golden[1024 * 1024];
	uint8_t        cluster_buf[512];
	dir_entry_t    e;
	uint32_t       out_bytes = 0xFFFFFFFFu;
	long           glen;
	int            rc;
	char           msg[160];

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

	memset(got, 0, sizeof(got));
	/* fat==NULL, fat_len==0: the windowed read MUST NOT consult a whole-FAT
	 * buffer (Law 2 -- if it does, this read faults / mis-reads). */
	rc = fat12_read_file(vol, NULL, 0u, &e, got, e.file_size + 1u,
	                     cluster_buf, &out_bytes);
	snprintf(msg, sizeof(msg), "fat12_read_file(%s) windowed -> FAT12_OK", name83);
	CHECK(rc == FAT12_OK, msg);
	snprintf(msg, sizeof(msg), "%s windowed out_bytes == file_size %u",
	         name83, e.file_size);
	CHECK(out_bytes == e.file_size, msg);

	if (rc == FAT12_OK && glen == (long)e.file_size) {
		snprintf(msg, sizeof(msg),
		         "%s windowed bytes match the fixture byte-for-byte", name83);
		CHECK(memcmp(got, golden, (size_t)e.file_size) == 0, msg);
	}
}

/* Read HIGHCLUS.BIN (2.5 MB; its chain crosses cluster 0xFF8 AND slides the FAT
 * window across MANY FAT sectors mid-walk) through the WINDOWED FAT and verify
 * the deterministic pattern byte i == (i & 0xFF). This stresses the cache /
 * window-slide on a long FAT16 chain -- the byte-for-byte match proves the
 * windowed decode (and its sector-cache) holds across the whole chain. */
#define HIGHCLUS_SIZE 2621440u
static void check_highclus_windowed(const fat12_volume_t *vol, void *sector_buf)
{
	static uint8_t got[HIGHCLUS_SIZE];
	uint8_t        cluster_buf[512];
	dir_entry_t    e;
	uint32_t       out_bytes = 0xFFFFFFFFu;
	uint32_t       i;
	int            rc;
	int            ok;

	rc = fat12_find(vol, sector_buf, "HIGHCLUS.BIN", &e);
	CHECK(rc == FAT12_OK, "fat12_find(HIGHCLUS.BIN) windowed -> FAT12_OK");
	if (rc != FAT12_OK) {
		return;
	}
	CHECK(e.file_size == HIGHCLUS_SIZE,
	      "HIGHCLUS.BIN file_size == 2621440 (chain crosses cluster 0xFF8)");
	if (e.file_size != HIGHCLUS_SIZE) {
		return;
	}
	rc = fat12_read_file(vol, NULL, 0u, &e, got, HIGHCLUS_SIZE,
	                     cluster_buf, &out_bytes);
	CHECK(rc == FAT12_OK,
	      "fat12_read_file(HIGHCLUS.BIN) windowed OK (window slides across the FAT)");
	CHECK(out_bytes == HIGHCLUS_SIZE, "HIGHCLUS.BIN windowed out_bytes == 2621440");
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
	      "HIGHCLUS.BIN windowed bytes == (i & 0xFF) pattern (window-slide + 0xFF8 cross)");
}

/* Mount `img` and bind a 2-sector FAT window onto `vol`. Returns 0 on success. */
static int mount_windowed(const char *img, blockdev_file_t *bf,
                          fat12_volume_t *vol, uint8_t *sector_buf,
                          fat12_fat_window_t *win, uint8_t *win_buf)
{
	int rc = blockdev_file_open(bf, img);
	CHECK(rc == 0, "blockdev_file_open(image)");
	if (rc != 0) {
		return -1;
	}
	rc = fat12_mount(vol, &bf->dev, sector_buf);
	CHECK(rc == FAT12_OK, "fat12_mount(image) should return FAT12_OK");
	if (rc != FAT12_OK) {
		blockdev_file_close(bf);
		return -1;
	}
	/* Install the windowed FAT reader: a 2-sector buffer (WIN_SECTORS) +
	 * fat12_fat_window_init binds it to the volume. NO whole-FAT slurp. */
	fat12_fat_window_init(win, win_buf, WIN_SECTORS * BLOCKDEV_SECTOR_SIZE);
	vol->fat_window = win;
	return 0;
}

int main(int argc, char **argv)
{
	const char        *fat16_img;
	const char        *fat12_img;
	const char        *fixdir;
	blockdev_file_t    bf16;
	blockdev_file_t    bf12;
	fat12_volume_t     vol16;
	fat12_volume_t     vol12;
	uint8_t            sector_buf[512];
	fat12_fat_window_t win16;
	fat12_fat_window_t win12;
	static uint8_t     win_buf16[WIN_SECTORS * 512u];
	static uint8_t     win_buf12[WIN_SECTORS * 512u];
	char               path[512];
	int                rc;

	if (argc != 4) {
		fprintf(stderr, "usage: %s <fat16_image> <fat12_image> <fixture_dir>\n",
		        argv[0]);
		return 2;
	}
	fat16_img = argv[1];
	fat12_img = argv[2];
	fixdir    = argv[3];

	/* ---- (a) FAT16 WINDOWED READ ---- */
	if (mount_windowed(fat16_img, &bf16, &vol16, sector_buf, &win16,
	                   win_buf16) == 0) {
		CHECK(vol16.fat_type == FAT_TYPE_FAT16,
		      "FAT16 fixture classified FAT16 (windowed mount)");
		/* The FAT16 FAT is FAR larger than the 2-sector window -- proving the
		 * read truly streams sectors, never slurps the whole FAT. */
		CHECK((uint32_t)vol16.bpb.sectors_per_fat > WIN_SECTORS,
		      "FAT16 sectors_per_fat > the 2-sector window (whole-FAT would not fit)");

		snprintf(path, sizeof(path), "%s/hello.txt", fixdir);
		check_file_windowed(&vol16, sector_buf, "HELLO.TXT", path);
		snprintf(path, sizeof(path), "%s/chain.txt", fixdir);
		check_file_windowed(&vol16, sector_buf, "CHAIN.TXT", path);
		snprintf(path, sizeof(path), "%s/block.bin", fixdir);
		check_file_windowed(&vol16, sector_buf, "BLOCK.BIN", path);
		snprintf(path, sizeof(path), "%s/bigchain.txt", fixdir);
		check_file_windowed(&vol16, sector_buf, "BIGCHAIN.TXT", path);
		/* HIGHCLUS.BIN: chain crosses cluster 0xFF8 AND slides the window across
		 * many FAT sectors -- the cache/window-slide stress leg. */
		check_highclus_windowed(&vol16, sector_buf);
		blockdev_file_close(&bf16);
	}

	/* ---- (b) FAT12 STRADDLE BYTE-IDENTICAL via the window ---- */
	if (mount_windowed(fat12_img, &bf12, &vol12, sector_buf, &win12,
	                   win_buf12) == 0) {
		CHECK(vol12.fat_type == FAT_TYPE_FAT12,
		      "FAT12 fixture classified FAT12 (windowed mount)");

		/* BIGCHAIN occupies clusters 2..1369 (contiguous mtools alloc), crossing
		 * the FAT12 straddle clusters 341 (off 511) and 682 (off 1023). A naive
		 * single-sector window mis-decodes those; the byte-for-byte match proves
		 * the straddle fetch is correct. */
		snprintf(path, sizeof(path), "%s/bigchain.txt", fixdir);
		check_file_windowed(&vol12, sector_buf, "BIGCHAIN.TXT", path);
		snprintf(path, sizeof(path), "%s/chain.txt", fixdir);
		check_file_windowed(&vol12, sector_buf, "CHAIN.TXT", path);

		/* ---- (c) DIRECT straddle probe ---- *
		 * Decode the straddling clusters 341 and 682 through the window and via a
		 * whole-FAT buffer; the next-cluster links MUST be identical. */
		{
			static uint8_t whole_fat[64 * 1024];
			fat12_volume_t vol_whole = vol12;
			uint16_t       straddlers[2] = { 341u, 682u };
			int            k;

			vol_whole.fat_window = NULL;   /* whole-FAT path */
			rc = fat12_read_fat(&vol_whole, whole_fat, (uint32_t)sizeof(whole_fat));
			CHECK(rc == FAT12_OK, "fat12_read_fat(whole FAT12 FAT) for the cross-check");

			for (k = 0; k < 2; k++) {
				uint16_t c = straddlers[k];
				uint16_t win_next = 0xEEEEu;
				uint16_t whole_next = 0xDDDDu;
				char     msg[120];

				/* Confirm the cluster's entry truly straddles a sector boundary. */
				uint32_t off = ((uint32_t)c * 3u) / 2u;
				snprintf(msg, sizeof(msg),
				         "cluster %u FAT entry straddles a sector boundary (off %% 512 == 511)",
				         c);
				CHECK((off % BLOCKDEV_SECTOR_SIZE) == (BLOCKDEV_SECTOR_SIZE - 1u),
				      msg);

				rc = fat12_next_cluster(&vol12, NULL, 0u, c, &win_next);
				snprintf(msg, sizeof(msg),
				         "windowed fat12_next_cluster(%u) OK", c);
				CHECK(rc == FAT12_OK, msg);
				rc = fat12_next_cluster(&vol_whole, whole_fat,
				                        (uint32_t)sizeof(whole_fat), c, &whole_next);
				snprintf(msg, sizeof(msg),
				         "whole-FAT fat12_next_cluster(%u) OK", c);
				CHECK(rc == FAT12_OK, msg);
				snprintf(msg, sizeof(msg),
				         "straddle cluster %u: windowed link == whole-FAT link (0x%X)",
				         c, whole_next);
				CHECK(win_next == whole_next, msg);
			}
		}
		blockdev_file_close(&bf12);
	}

	return TEST_SUMMARY("test_fat16_window");
}
