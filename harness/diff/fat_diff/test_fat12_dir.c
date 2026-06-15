/*
 * harness/diff/fat_diff/test_fat12_dir.c -- FAT12 root-dir enumerate + read.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (CLAUDE.md Law 2: the oracle is the truth, never false-green).
 *
 * RED->GREEN gate for os/milton/fat12.c::{fat12_format_83, fat12_read_root_dir,
 * fat12_find, fat12_read_file} (beads initech-adf, the FAT12 read-path core).
 *
 * It mounts the same minted 1.44 MB FAT12 image as test_fat12_bpb/chain.c
 * through the REAL artifact fat12.c via the host file-backed blockdev, and
 * asserts:
 *
 *   (a) ENUMERATION: the set of regular-file 8.3 names found in the root dir
 *       equals the fixture set {HELLO.TXT, SECOND.TXT, CHAIN.TXT}. This proves
 *       LFN/volume-label entries are skipped and the 0x00 sentinel stops the
 *       scan. Matched against actual `mdir -i <img> ::` output (verified this
 *       session): mtools stores these 8.3-legal names directly (NO LFN slots).
 *
 *   (b) FIND: fat12_find for each fixture returns FAT12_OK with the correct
 *       file_size + start_cluster (HELLO=2/31, SECOND=3/104, CHAIN=4/1600 --
 *       re-verified with python3 on the raw root dir this session); a
 *       non-existent name returns FAT12_ERR_NOT_FOUND (no false match).
 *
 *   (c) READ-FILE (the load-bearing correctness assertion): fat12_read_file
 *       reproduces each fixture's bytes EXACTLY (compared against the committed
 *       fixture file read directly as the golden) and sets out_bytes to the
 *       real size. chain.txt (1600 bytes => 4 clusters with a partial last
 *       cluster) is the RISK-5 case: the chain walk + file_size truncation must
 *       reproduce it byte-for-byte with NO trailing padding.
 *
 *   (d) EDGE: fat12_read_file into an undersized buffer returns
 *       FAT12_ERR_BUFFER (no overflow).
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 4 (dir read semantics +
 *   RISK-5 partial last cluster). Image + fixture-dir paths are argv (Makefile)
 *   -> no host path baked in (Rule 11).
 *
 * ASCII-clean (Rule 12).
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
	/* Must hit EOF exactly at n (file fit in cap). */
	if (!feof(fp) && n == cap) {
		fclose(fp);
		return -1; /* file larger than cap -- test fixture sizing bug */
	}
	fclose(fp);
	return n;
}

/* ---- In-memory blockdev for the synthetic root-dir test ---- *
 * Backs blockdev_t.read_sectors with a plain byte buffer so we can hand-build a
 * root directory containing a DELETED entry, an LFN slot, a volume-label entry,
 * and trailing GARBAGE after the 0x00 end sentinel -- cases the minted mtools
 * image does not exercise. This makes the skip/stop logic load-bearing (Rule 6).
 * The synthetic volume reuses the same BPB geometry as the minted image so
 * root_dir_sector / root_dir_sectors / root_entry_count are realistic. */
typedef struct memdev {
	const uint8_t *base;  /* whole disk image bytes */
	uint32_t       len;   /* image length in bytes  */
	blockdev_t     dev;
} memdev_t;

static int memdev_read(void *ctx, uint32_t lba, uint32_t count, void *buf)
{
	memdev_t *md = (memdev_t *)ctx;
	uint32_t  off = lba * BLOCKDEV_SECTOR_SIZE;
	uint32_t  n   = count * BLOCKDEV_SECTOR_SIZE;
	if (md == NULL || buf == NULL || count == 0u) {
		return -1;
	}
	if (off + n > md->len) {
		return -1; /* short read -> fail loud */
	}
	memcpy(buf, md->base + off, n);
	return 0;
}

/* Enumeration collector: gather formatted 8.3 names of ARCHIVE/regular files. */
#define MAX_NAMES 32
typedef struct {
	char names[MAX_NAMES][FAT12_NAME83_MAX];
	int  count;
} namelist_t;

static int collect_cb(const dir_entry_t *e, void *user)
{
	namelist_t *nl = (namelist_t *)user;
	/* Skip volume-label entries for the file listing (brief Sec 4); LFN
	 * entries are already filtered by fat12_read_root_dir. */
	if (e->attribute & DIR_ATTR_VOLLABEL) {
		return 0;
	}
	if (nl->count < MAX_NAMES) {
		fat12_format_83(e, nl->names[nl->count]);
		nl->count++;
	}
	return 0;
}

/* Raw visitor: count EVERY entry fat12_read_root_dir surfaces (no caller-side
 * filtering). This directly observes the reader's own skip/stop logic, so an
 * LFN/deleted entry leaking through (or the sentinel not stopping) changes the
 * count -- making those branches load-bearing (Rule 6). */
static int rawcount_cb(const dir_entry_t *e, void *user)
{
	(void)e;
	(*(int *)user)++;
	return 0;
}

static int namelist_has(const namelist_t *nl, const char *want)
{
	int i;
	for (i = 0; i < nl->count; i++) {
		if (strcmp(nl->names[i], want) == 0) {
			return 1;
		}
	}
	return 0;
}

/* Find a fixture, read it via the artifact, and assert byte-for-byte equality
 * with the committed fixture file (the golden). */
static void check_file(const fat12_volume_t *vol, void *sector_buf,
                       const void *fat, uint32_t fat_len,
                       const char *name83, const char *golden_path,
                       uint16_t want_start, uint32_t want_size)
{
	dir_entry_t e;
	uint8_t     got[4096];
	uint8_t     golden[4096];
	uint8_t     cluster_buf[512]; /* sectors_per_cluster(1) * 512 for floppy */
	uint32_t    out_bytes = 0xFFFFFFFFu;
	long        glen;
	int         rc;
	char        msg[128];

	/* Guard byte sentinel: poison the byte at out_buf[file_size] so that if
	 * fat12_read_file writes the full last cluster (i.e. drops the RISK-5
	 * truncation), it clobbers the guard and we catch it. */
	const uint8_t GUARD = 0x7Eu;

	rc = fat12_find(vol, sector_buf, name83, &e);
	snprintf(msg, sizeof(msg), "fat12_find(%s) should return FAT12_OK", name83);
	CHECK(rc == FAT12_OK, msg);
	if (rc != FAT12_OK) {
		return;
	}

	snprintf(msg, sizeof(msg), "%s start_cluster == %u", name83, want_start);
	CHECK(e.start_cluster == want_start, msg);
	snprintf(msg, sizeof(msg), "%s file_size == %u", name83, want_size);
	CHECK(e.file_size == want_size, msg);

	/* Golden: the committed fixture file read directly. */
	glen = read_whole_file(golden_path, golden, (long)sizeof(golden));
	snprintf(msg, sizeof(msg), "read golden fixture %s", golden_path);
	CHECK(glen >= 0, msg);
	snprintf(msg, sizeof(msg), "%s golden size == file_size %u", name83, want_size);
	CHECK(glen == (long)want_size, msg);

	/* Poison the guard byte just past file_size, then pass an out_buf_len of
	 * EXACTLY file_size + 1 so the guard is in bounds. A correct read writes
	 * only file_size bytes and leaves the guard intact (RISK-5); a read that
	 * returns the full final cluster overruns file_size and trips the guard. */
	memset(got, 0, sizeof(got));
	if (want_size < sizeof(got)) {
		got[want_size] = GUARD;
	}
	rc = fat12_read_file(vol, fat, fat_len, &e, got, want_size + 1u,
	                     cluster_buf, &out_bytes);
	snprintf(msg, sizeof(msg), "fat12_read_file(%s) should return FAT12_OK", name83);
	CHECK(rc == FAT12_OK, msg);
	snprintf(msg, sizeof(msg), "%s out_bytes == real file size %u", name83, want_size);
	CHECK(out_bytes == want_size, msg);
	snprintf(msg, sizeof(msg),
	         "%s RISK-5: guard byte past file_size untouched (no padding written)", name83);
	CHECK(want_size >= sizeof(got) || got[want_size] == GUARD, msg);

	/* The load-bearing assertion: bytes read EXACTLY equal the golden, with
	 * no trailing padding (RISK-5). memcmp over file_size bytes. */
	if (rc == FAT12_OK && glen == (long)want_size) {
		snprintf(msg, sizeof(msg),
		         "%s bytes read EXACTLY match the fixture (byte-for-byte)", name83);
		CHECK(memcmp(got, golden, want_size) == 0, msg);
	}
}

/* Large multi-cluster file check (beads initech-dao): read BIGCHAIN.TXT (700060
 * bytes => 1368 clusters on the 1-sector/cluster floppy, > half the old on-stack
 * uint16_t chain[2880] array) through the REAL artifact fat12_read_file and
 * assert byte-for-byte equality with the committed fixture golden, plus the
 * RISK-5 guard byte just past file_size (no trailing padding from the partial
 * last cluster: 700060 % 512 == 156). Buffers are `static` so the streaming read
 * is exercised WITHOUT a large test stack frame -- the very hazard initech-dao
 * removes from the kernel. */
static void check_large_file(const fat12_volume_t *vol, void *sector_buf,
                             const void *fat, uint32_t fat_len,
                             const char *name83, const char *golden_path,
                             uint16_t want_start, uint32_t want_size)
{
	static uint8_t got[1024 * 1024];
	static uint8_t golden[1024 * 1024];
	uint8_t        cluster_buf[512]; /* sectors_per_cluster(1) * 512 for floppy */
	dir_entry_t    e;
	uint32_t       out_bytes = 0xFFFFFFFFu;
	long           glen;
	int            rc;
	char           msg[128];
	const uint8_t  GUARD = 0x7Eu;

	rc = fat12_find(vol, sector_buf, name83, &e);
	snprintf(msg, sizeof(msg), "fat12_find(%s) should return FAT12_OK", name83);
	CHECK(rc == FAT12_OK, msg);
	if (rc != FAT12_OK) {
		return;
	}
	snprintf(msg, sizeof(msg), "%s start_cluster == %u", name83, want_start);
	CHECK(e.start_cluster == want_start, msg);
	snprintf(msg, sizeof(msg), "%s file_size == %u", name83, want_size);
	CHECK(e.file_size == want_size, msg);

	glen = read_whole_file(golden_path, golden, (long)sizeof(golden));
	snprintf(msg, sizeof(msg), "read golden fixture %s", golden_path);
	CHECK(glen >= 0, msg);
	snprintf(msg, sizeof(msg), "%s golden size == file_size %u", name83, want_size);
	CHECK(glen == (long)want_size, msg);

	/* Poison the guard just past file_size; pass out_buf_len = file_size + 1 so
	 * a correct streaming read writes only file_size bytes (RISK-5). */
	memset(got, 0, sizeof(got));
	if (want_size < sizeof(got)) {
		got[want_size] = GUARD;
	}
	rc = fat12_read_file(vol, fat, fat_len, &e, got, want_size + 1u,
	                     cluster_buf, &out_bytes);
	snprintf(msg, sizeof(msg), "fat12_read_file(%s) should return FAT12_OK", name83);
	CHECK(rc == FAT12_OK, msg);
	snprintf(msg, sizeof(msg), "%s out_bytes == real file size %u", name83, want_size);
	CHECK(out_bytes == want_size, msg);
	snprintf(msg, sizeof(msg),
	         "%s RISK-5: guard byte past file_size untouched (no padding)", name83);
	CHECK(want_size >= sizeof(got) || got[want_size] == GUARD, msg);

	if (rc == FAT12_OK && glen == (long)want_size) {
		snprintf(msg, sizeof(msg),
		         "%s streaming read matches the fixture (byte-for-byte, 1368 clusters)",
		         name83);
		CHECK(memcmp(got, golden, want_size) == 0, msg);
	}
}

int main(int argc, char **argv)
{
	const char     *img;
	const char     *fixdir;
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint8_t         sector_buf[512];
	uint8_t         fat_buf[9 * 512];
	namelist_t      nl;
	int             rc;
	char            hello_path[512];
	char            second_path[512];
	char            chain_path[512];
	char            empty_path[512];
	char            block_path[512];
	char            bigchain_path[512];

	if (argc < 3) {
		fprintf(stderr, "usage: %s <fat12-image> <fixture-dir>\n", argv[0]);
		return 2;
	}
	img    = argv[1];
	fixdir = argv[2];

	snprintf(hello_path,  sizeof(hello_path),  "%s/hello.txt",  fixdir);
	snprintf(second_path, sizeof(second_path), "%s/second.txt", fixdir);
	snprintf(chain_path,  sizeof(chain_path),  "%s/chain.txt",  fixdir);
	snprintf(empty_path,  sizeof(empty_path),  "%s/empty.txt",  fixdir);
	snprintf(block_path,  sizeof(block_path),  "%s/block.bin",  fixdir);
	snprintf(bigchain_path, sizeof(bigchain_path), "%s/bigchain.txt", fixdir);

	rc = blockdev_file_open(&bf, img);
	CHECK(rc == 0, "blockdev_file_open should succeed on the minted image");
	if (rc != 0) {
		return TEST_SUMMARY("test_fat12_dir");
	}

	rc = fat12_mount(&vol, &bf.dev, sector_buf);
	CHECK(rc == FAT12_OK, "fat12_mount should return FAT12_OK on the 1.44MB image");

	rc = fat12_read_fat(&vol, fat_buf, sizeof(fat_buf));
	CHECK(rc == FAT12_OK, "fat12_read_fat should fill the whole-FAT buffer");

	/* ---- fat12_format_83 unit checks (pure, no I/O) ---- *
	 * Build a dir entry by hand and confirm space-trim + dot insertion +
	 * the 0x05 -> 0xE5 first-byte fix (brief Sec 4 / RISK-3). */
	{
		dir_entry_t e;
		char        out[FAT12_NAME83_MAX];

		memset(&e, 0, sizeof(e));
		memcpy(e.filename,  "HELLO   ", 8); /* space-padded */
		memcpy(e.extension, "TXT",      3);
		fat12_format_83(&e, out);
		CHECK_STR_EQ(out, "HELLO.TXT", "format_83 trims name padding, inserts dot");

		/* All-space extension => no dot, name only. */
		memset(&e, 0, sizeof(e));
		memcpy(e.filename,  "README  ", 8);
		memcpy(e.extension, "   ",      3);
		fat12_format_83(&e, out);
		CHECK_STR_EQ(out, "README", "format_83 omits dot when extension is all spaces");

		/* 0x05 first byte => real first char 0xE5 (RISK-3). */
		memset(&e, 0, sizeof(e));
		e.filename[0] = 0x05u;
		memcpy(e.filename + 1, "AME    ", 7);
		memcpy(e.extension, "TXT",      3);
		fat12_format_83(&e, out);
		CHECK((uint8_t)out[0] == 0xE5u && strcmp(out + 1, "AME.TXT") == 0,
		      "format_83 maps 0x05 first byte to 0xE5 (RISK-3), not a delete");
	}

	/* ---- (a) ENUMERATION: the regular-file set == fixture set ---- *
	 * Matched against `mdir -i <img> ::` (this session): HELLO.TXT, SECOND.TXT,
	 * CHAIN.TXT, EMPTY.TXT, BLOCK.BIN, BIGCHAIN.TXT -- no LFN, no volume-label
	 * entries. EMPTY.TXT (0 bytes, start_cluster 0) and BLOCK.BIN (1024 bytes =
	 * an exact 2-cluster multiple, full last cluster) extend coverage to the
	 * empty-file and partial-vs-full last-cluster boundary cases (initech-5cu).
	 * BIGCHAIN.TXT (700060 bytes => 1368 clusters, partial last) is the large
	 * multi-cluster stress file that exercises the STREAMING read (initech-dao):
	 * it would have used > half the old on-stack chain[2880] array. */
	memset(&nl, 0, sizeof(nl));
	rc = fat12_read_root_dir(&vol, sector_buf, collect_cb, &nl);
	CHECK(rc == FAT12_OK, "fat12_read_root_dir should complete the scan (FAT12_OK)");
	CHECK(nl.count == 6, "root dir holds exactly 6 regular files (0x00 sentinel stopped scan)");
	CHECK(namelist_has(&nl, "HELLO.TXT"),  "enumeration found HELLO.TXT");
	CHECK(namelist_has(&nl, "SECOND.TXT"), "enumeration found SECOND.TXT");
	CHECK(namelist_has(&nl, "CHAIN.TXT"),  "enumeration found CHAIN.TXT");
	CHECK(namelist_has(&nl, "EMPTY.TXT"),  "enumeration found EMPTY.TXT");
	CHECK(namelist_has(&nl, "BLOCK.BIN"),  "enumeration found BLOCK.BIN");
	CHECK(namelist_has(&nl, "BIGCHAIN.TXT"), "enumeration found BIGCHAIN.TXT");

	/* ---- (a') SYNTHETIC root dir: prove skip/stop are load-bearing ---- *
	 * Hand-build a disk: copy the minted boot sector + FAT region for valid
	 * geometry, then overwrite the root dir with our own entries:
	 *   [0] LFN slot (attr 0x0F)        -> MUST be skipped
	 *   [1] deleted (0xE5)              -> MUST be skipped
	 *   [2] GOOD.TXT (archive)          -> visited
	 *   [3] volume-label (attr 0x08)    -> visited by enum, dropped by collect_cb
	 *   [4] 0x00 end sentinel           -> STOP here
	 *   [5] AFTER.GBG (archive garbage) -> MUST NOT be reached (proves stop). */
	{
		/* A full 1.44 MB image is 2880*512; we only need through the root dir
		 * (first_data_sector sectors). Build that prefix from the minted image
		 * so the BPB/geometry parse cleanly, then patch the root dir. */
		static uint8_t  synth[64 * 512]; /* >= first_data_sector(33)*512 */
		memdev_t        md;
		fat12_volume_t  svol;
		uint8_t         sbuf[512];
		namelist_t      snl;
		dir_entry_t     ent;
		uint32_t        rdoff;
		FILE           *fp;
		size_t          rd;

		fp = fopen(img, "rb");
		CHECK(fp != NULL, "synthetic: reopen minted image for prefix");
		rd = (fp != NULL) ? fread(synth, 1u, sizeof(synth), fp) : 0u;
		if (fp != NULL) {
			fclose(fp);
		}
		CHECK(rd == sizeof(synth), "synthetic: read image prefix into buffer");

		md.base = synth;
		md.len  = sizeof(synth);
		md.dev.ctx           = &md;
		md.dev.read_sectors  = memdev_read;
		md.dev.write_sectors = NULL;

		rc = fat12_mount(&svol, &md.dev, sbuf);
		CHECK(rc == FAT12_OK, "synthetic: fat12_mount on the prefix image");

		/* Patch the root directory (svol.root_dir_sector * 512). */
		rdoff = svol.root_dir_sector * 512u;
		memset(synth + rdoff, 0, 6u * 32u); /* clear 6 entries */

		/* [0] LFN slot: attr 0x0F, arbitrary bytes. */
		synth[rdoff + 0*32 + 11] = FAT12_ATTR_LFN;
		synth[rdoff + 0*32 + 0]  = 0x41u; /* LFN sequence byte, not 0x00/0xE5 */
		/* [1] deleted: first byte 0xE5. */
		synth[rdoff + 1*32 + 0]  = DIR_NAME_DELETED;
		/* [2] GOOD.TXT archive. */
		memcpy(synth + rdoff + 2*32 + 0, "GOOD    ", 8);
		memcpy(synth + rdoff + 2*32 + 8, "TXT",      3);
		synth[rdoff + 2*32 + 11] = DIR_ATTR_ARCHIVE;
		/* [3] volume label. */
		memcpy(synth + rdoff + 3*32 + 0, "MYVOL      ", 11);
		synth[rdoff + 3*32 + 11] = DIR_ATTR_VOLLABEL;
		/* [4] 0x00 end sentinel (already zeroed). */
		/* [5] garbage archive AFTER the sentinel -- must never be reached. */
		memcpy(synth + rdoff + 5*32 + 0, "AFTER   ", 8);
		memcpy(synth + rdoff + 5*32 + 8, "GBG",      3);
		synth[rdoff + 5*32 + 11] = DIR_ATTR_ARCHIVE;

		/* Raw count: the reader must surface ONLY GOOD.TXT + the volume label
		 * (indices 2,3) -- LFN (0) and deleted (1) skipped by the reader, and
		 * the 0x00 sentinel (4) stops before AFTER.GBG (5). Raw count == 2
		 * isolates the LFN/deleted skips from the caller-side vollabel filter. */
		{
			int raw = 0;
			rc = fat12_read_root_dir(&svol, sbuf, rawcount_cb, &raw);
			CHECK(rc == FAT12_OK, "synthetic: raw enumeration completes");
			CHECK(raw == 2,
			      "synthetic: reader surfaces exactly 2 entries (LFN+deleted skipped, sentinel stops)");
		}

		/* Enumerate the synthetic dir with the regular-file collector. */
		memset(&snl, 0, sizeof(snl));
		rc = fat12_read_root_dir(&svol, sbuf, collect_cb, &snl);
		CHECK(rc == FAT12_OK, "synthetic: fat12_read_root_dir completes");
		CHECK(snl.count == 1, "synthetic: exactly 1 regular file (LFN+deleted+vollabel skipped, stop at 0x00)");
		CHECK(namelist_has(&snl, "GOOD.TXT"), "synthetic: GOOD.TXT is the only regular file");
		CHECK(!namelist_has(&snl, "AFTER.GBG"),
		      "synthetic: AFTER.GBG past the 0x00 sentinel is NOT reached (stop, not skip)");

		/* fat12_find must also stop at the sentinel: AFTER.GBG not findable. */
		rc = fat12_find(&svol, sbuf, "AFTER.GBG", &ent);
		CHECK(rc == FAT12_ERR_NOT_FOUND,
		      "synthetic: fat12_find(AFTER.GBG) is NOT_FOUND (sentinel stopped scan)");
		rc = fat12_find(&svol, sbuf, "GOOD.TXT", &ent);
		CHECK(rc == FAT12_OK, "synthetic: fat12_find(GOOD.TXT) succeeds");
	}

	/* ---- (b)+(c) FIND + READ each fixture (byte-for-byte golden) ---- *
	 * start_cluster / file_size re-verified with python3 this session. */
	check_file(&vol, sector_buf, fat_buf, sizeof(fat_buf),
	           "HELLO.TXT", hello_path, 2u, 31u);
	check_file(&vol, sector_buf, fat_buf, sizeof(fat_buf),
	           "SECOND.TXT", second_path, 3u, 104u);
	/* The load-bearing RISK-5 case: 1600 bytes over 4 clusters, partial last. */
	check_file(&vol, sector_buf, fat_buf, sizeof(fat_buf),
	           "CHAIN.TXT", chain_path, 4u, 1600u);
	/* EMPTY.TXT: 0 bytes; start_cluster 0; read WITHOUT a chain walk. */
	check_file(&vol, sector_buf, fat_buf, sizeof(fat_buf),
	           "EMPTY.TXT", empty_path, 0u, 0u);
	/* BLOCK.BIN: 1024 bytes == EXACT 2-cluster multiple (full last cluster, no
	 * partial-cluster truncation) -- the partial-vs-full boundary's other side
	 * vs CHAIN.TXT (beads initech-5cu). start_cluster re-verified with python3. */
	check_file(&vol, sector_buf, fat_buf, sizeof(fat_buf),
	           "BLOCK.BIN", block_path, 8u, 1024u);
	/* BIGCHAIN.TXT: the load-bearing STREAMING case (beads initech-dao). 700060
	 * bytes => 1368 clusters (> half the old on-stack chain[2880]); partial last
	 * cluster (700060 % 512 == 156) is RISK-5. The incremental walk must
	 * reproduce it byte-for-byte through the artifact's single-cluster scratch. */
	check_large_file(&vol, sector_buf, fat_buf, sizeof(fat_buf),
	                 "BIGCHAIN.TXT", bigchain_path, 10u, 700060u);

	/* fat12_find is case-insensitive (lower-case input must match). */
	{
		dir_entry_t e;
		rc = fat12_find(&vol, sector_buf, "hello.txt", &e);
		CHECK(rc == FAT12_OK && e.start_cluster == 2u,
		      "fat12_find is case-insensitive (hello.txt matches HELLO.TXT)");
	}

	/* ---- (d) EDGE: not-found + undersized buffer ---- */
	{
		dir_entry_t e;
		rc = fat12_find(&vol, sector_buf, "NOPE.XYZ", &e);
		CHECK(rc == FAT12_ERR_NOT_FOUND,
		      "fat12_find for a non-existent name returns FAT12_ERR_NOT_FOUND");
	}
	{
		dir_entry_t e;
		uint8_t     tiny[16];
		uint8_t     cluster_buf[512];
		uint32_t    out_bytes;

		rc = fat12_find(&vol, sector_buf, "CHAIN.TXT", &e);
		CHECK(rc == FAT12_OK, "fat12_find(CHAIN.TXT) for the undersized-buffer test");
		rc = fat12_read_file(&vol, fat_buf, sizeof(fat_buf), &e, tiny,
		                     (uint32_t)sizeof(tiny), cluster_buf, &out_bytes);
		CHECK(rc == FAT12_ERR_BUFFER,
		      "fat12_read_file into an undersized buffer returns FAT12_ERR_BUFFER (no overflow)");
	}

	blockdev_file_close(&bf);
	return TEST_SUMMARY("test_fat12_dir");
}
