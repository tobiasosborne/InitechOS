/*
 * harness/diff/fat_diff/test_fat12_subdir.c -- FAT12 subdir + path traversal.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (Law 2: the oracle is the truth, never false-green).
 *
 * RED->GREEN gate for os/milton/fat12.c::{fat12_read_dir, fat12_resolve_path}
 * (beads initech-ti8, the FAT12 subdirectory / path-traversal READ path).
 *
 * It mounts the minted NESTED FAT12 image through the REAL artifact fat12.c via
 * the host file-backed blockdev and asserts:
 *
 *   (1) resolve ''/'A:\\'/'\\'  => the ROOT directory (is_root==1).
 *   (2) '\\SUB'                 => a SUBDIR (is_root==0, start_cluster == SUB's,
 *                                 cross-checked by an independent root scan).
 *   (3) '\\SUB\\DEEP'           => a deeper SUBDIR (different, larger cluster;
 *                                 cross-checked by enumerating SUB).
 *   (4) '\\SUB\\NESTED.TXT'     => final OK (a regular file); but
 *       '\\SUB\\NESTED.TXT\\X'  => FAILS (a non-final component that lacks
 *                                 DIR_ATTR_DIRECTORY is not traversable).
 *   (5) a leading 'A:' is stripped: 'A:\\SUB' resolves identically to '\\SUB'.
 *   (6) '..' from '\\SUB\\DEEP' => '\\SUB'; '.' is identity.
 *   (7) enumerate '\\BIGDIR' returns >16 files AND a file demonstrably in
 *       cluster 2+ (FILE20.TXT) reads byte-for-byte vs fixtures/big_fill.txt --
 *       THE load-bearing multi-cluster assertion (a single-sector reader fails).
 *   (8) find+read '\\SUB\\NESTED.TXT' and '\\SUB\\DEEP\\DEEP.TXT' byte-for-byte
 *       vs the committed fixtures.
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 3 (cluster chain) +
 *   Sec 4 (dir entry / sentinels); ADR-0003 DEC-07. Image + fixture-dir paths
 *   are argv (Makefile) -> no host path baked in (Rule 11). ASCII-clean (Rule 12).
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
		return -1; /* file larger than cap -- fixture sizing bug */
	}
	fclose(fp);
	return n;
}

/* ---- A name->entry collector for enumeration assertions ---- */
#define MAX_NAMES 64
typedef struct {
	char     names[MAX_NAMES][FAT12_NAME83_MAX];
	uint16_t starts[MAX_NAMES];
	uint32_t sizes[MAX_NAMES];
	uint8_t  attrs[MAX_NAMES];
	int      count;
} namelist_t;

static int collect_cb(const dir_entry_t *e, void *user)
{
	namelist_t *nl = (namelist_t *)user;
	if (nl->count < MAX_NAMES) {
		fat12_format_83(e, nl->names[nl->count]);
		nl->starts[nl->count] = e->start_cluster;
		nl->sizes[nl->count]  = e->file_size;
		nl->attrs[nl->count]  = e->attribute;
		nl->count++;
	}
	return 0;
}

/* Look up a formatted 8.3 name in a collected list; returns index or -1. */
static int namelist_find(const namelist_t *nl, const char *want)
{
	int i;
	for (i = 0; i < nl->count; i++) {
		if (strcmp(nl->names[i], want) == 0) {
			return i;
		}
	}
	return -1;
}

/* Find + read a path's file via the artifact and assert byte-for-byte equality
 * with the committed fixture (the golden). Uses a guard byte past file_size to
 * prove no trailing padding (RISK-5). */
static void check_path_file(const fat12_volume_t *vol, void *sector_buf,
                            const void *fat, uint32_t fat_len,
                            const char *path, const char *golden_path)
{
	fat12_dir_t dir;
	dir_entry_t e;
	uint8_t     got[8192];
	uint8_t     golden[8192];
	uint8_t     cluster_buf[512]; /* 1 sector/cluster on this floppy geometry */
	uint32_t    out_bytes = 0xFFFFFFFFu;
	long        glen;
	int         rc;
	char        msg[160];
	const uint8_t GUARD = 0x7Eu;

	rc = fat12_resolve_path(vol, sector_buf, fat, fat_len, path, &dir, &e);
	snprintf(msg, sizeof(msg), "resolve_path(%s) should return FAT12_OK", path);
	CHECK(rc == FAT12_OK, msg);
	if (rc != FAT12_OK) {
		return;
	}

	glen = read_whole_file(golden_path, golden, (long)sizeof(golden));
	snprintf(msg, sizeof(msg), "read golden fixture %s", golden_path);
	CHECK(glen >= 0, msg);
	snprintf(msg, sizeof(msg), "%s file_size == golden size %ld", path, glen);
	CHECK(e.file_size == (uint32_t)glen, msg);

	memset(got, 0, sizeof(got));
	if (e.file_size < sizeof(got)) {
		got[e.file_size] = GUARD;
	}
	rc = fat12_read_file(vol, fat, fat_len, &e, got, e.file_size + 1u,
	                     cluster_buf, &out_bytes);
	snprintf(msg, sizeof(msg), "read(%s) should return FAT12_OK", path);
	CHECK(rc == FAT12_OK, msg);
	snprintf(msg, sizeof(msg), "%s out_bytes == file_size", path);
	CHECK(out_bytes == e.file_size, msg);
	snprintf(msg, sizeof(msg),
	         "%s RISK-5: guard byte past file_size untouched", path);
	CHECK(e.file_size >= sizeof(got) || got[e.file_size] == GUARD, msg);

	if (rc == FAT12_OK && glen >= 0 && e.file_size == (uint32_t)glen) {
		snprintf(msg, sizeof(msg),
		         "%s bytes read EXACTLY match the fixture (byte-for-byte)", path);
		CHECK(memcmp(got, golden, (size_t)glen) == 0, msg);
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
	int             rc;
	char            nested_path[512];
	char            deep_path[512];
	char            bigfill_path[512];
	fat12_dir_t     root_dir;
	uint16_t        sub_start  = 0xFFFFu;
	uint16_t        deep_start = 0xFFFFu;

	if (argc < 3) {
		fprintf(stderr, "usage: %s <fat12-nested-image> <fixture-dir>\n", argv[0]);
		return 2;
	}
	img    = argv[1];
	fixdir = argv[2];

	snprintf(nested_path,  sizeof(nested_path),  "%s/nested.txt",   fixdir);
	snprintf(deep_path,    sizeof(deep_path),    "%s/deep.txt",     fixdir);
	snprintf(bigfill_path, sizeof(bigfill_path), "%s/big_fill.txt", fixdir);

	rc = blockdev_file_open(&bf, img);
	CHECK(rc == 0, "blockdev_file_open should succeed on the nested image");
	if (rc != 0) {
		return TEST_SUMMARY("test_fat12_subdir");
	}

	rc = fat12_mount(&vol, &bf.dev, sector_buf);
	CHECK(rc == FAT12_OK, "fat12_mount should return FAT12_OK on the nested image");
	rc = fat12_read_fat(&vol, fat_buf, sizeof(fat_buf));
	CHECK(rc == FAT12_OK, "fat12_read_fat should fill the whole-FAT buffer");

	root_dir.is_root = 1;
	root_dir.start_cluster = 0u;

	/* Independently discover SUB's and DEEP's start clusters by enumerating the
	 * directories directly (so assertions (2)/(3) do not hardcode mtools layout
	 * -- they cross-check resolve_path against fat12_read_dir). */
	{
		namelist_t nl;
		fat12_dir_t subdir;
		int idx;

		memset(&nl, 0, sizeof(nl));
		rc = fat12_read_dir(&vol, &root_dir, sector_buf, fat_buf, sizeof(fat_buf),
		                    collect_cb, &nl);
		CHECK(rc == FAT12_OK, "fat12_read_dir(root) should complete the scan");
		idx = namelist_find(&nl, "SUB");
		CHECK(idx >= 0, "root enumeration finds the SUB subdirectory");
		if (idx >= 0) {
			CHECK((nl.attrs[idx] & DIR_ATTR_DIRECTORY) != 0,
			      "SUB carries the DIR_ATTR_DIRECTORY attribute");
			sub_start = nl.starts[idx];
		}
		CHECK(namelist_find(&nl, "HELLO.TXT") >= 0,
		      "root enumeration also finds the root file HELLO.TXT");

		/* Enumerate SUB to discover DEEP. */
		if (sub_start != 0xFFFFu) {
			memset(&nl, 0, sizeof(nl));
			subdir.is_root = 0;
			subdir.start_cluster = sub_start;
			rc = fat12_read_dir(&vol, &subdir, sector_buf, fat_buf,
			                    sizeof(fat_buf), collect_cb, &nl);
			CHECK(rc == FAT12_OK, "fat12_read_dir(SUB) should complete the scan");
			CHECK(namelist_find(&nl, "NESTED.TXT") >= 0,
			      "SUB enumeration finds NESTED.TXT");
			idx = namelist_find(&nl, "DEEP");
			CHECK(idx >= 0, "SUB enumeration finds the DEEP subdirectory");
			if (idx >= 0) {
				deep_start = nl.starts[idx];
			}
			/* '.' and '..' are real entries inside a subdirectory. */
			CHECK(namelist_find(&nl, ".") >= 0, "SUB enumeration surfaces '.'");
			CHECK(namelist_find(&nl, "..") >= 0, "SUB enumeration surfaces '..'");
		}
	}

	/* ---- (1) empty / drive-only / backslash => the ROOT ---- */
	{
		fat12_dir_t d;
		dir_entry_t e;
		rc = fat12_resolve_path(&vol, sector_buf, fat_buf, sizeof(fat_buf),
		                        "", &d, &e);
		CHECK(rc == FAT12_OK && d.is_root == 1,
		      "resolve('') => root (is_root==1)");
		rc = fat12_resolve_path(&vol, sector_buf, fat_buf, sizeof(fat_buf),
		                        "\\", &d, &e);
		CHECK(rc == FAT12_OK && d.is_root == 1,
		      "resolve('\\') => root (is_root==1)");
		rc = fat12_resolve_path(&vol, sector_buf, fat_buf, sizeof(fat_buf),
		                        "A:\\", &d, &e);
		CHECK(rc == FAT12_OK && d.is_root == 1,
		      "resolve('A:\\') => root (drive prefix stripped, is_root==1)");
	}

	/* ---- (2) '\\SUB' => subdir, is_root==0, start_cluster == SUB's ---- */
	{
		fat12_dir_t d;
		dir_entry_t e;
		rc = fat12_resolve_path(&vol, sector_buf, fat_buf, sizeof(fat_buf),
		                        "\\SUB", &d, &e);
		CHECK(rc == FAT12_OK, "resolve('\\SUB') should return FAT12_OK");
		CHECK(rc == FAT12_OK && (e.attribute & DIR_ATTR_DIRECTORY) != 0,
		      "'\\SUB' final entry is a directory");
		CHECK(rc == FAT12_OK && e.start_cluster == sub_start,
		      "'\\SUB' start_cluster == SUB's (cross-checked vs root scan)");
		CHECK(rc == FAT12_OK && d.is_root == 1,
		      "'\\SUB' containing directory is the root");
	}

	/* ---- (3) '\\SUB\\DEEP' => deeper subdir cluster ---- */
	{
		fat12_dir_t d;
		dir_entry_t e;
		rc = fat12_resolve_path(&vol, sector_buf, fat_buf, sizeof(fat_buf),
		                        "\\SUB\\DEEP", &d, &e);
		CHECK(rc == FAT12_OK, "resolve('\\SUB\\DEEP') should return FAT12_OK");
		CHECK(rc == FAT12_OK && (e.attribute & DIR_ATTR_DIRECTORY) != 0,
		      "'\\SUB\\DEEP' final entry is a directory");
		CHECK(rc == FAT12_OK && e.start_cluster == deep_start,
		      "'\\SUB\\DEEP' start_cluster == DEEP's (cross-checked vs SUB scan)");
		CHECK(rc == FAT12_OK && d.is_root == 0 && d.start_cluster == sub_start,
		      "'\\SUB\\DEEP' containing directory is SUB");
	}

	/* ---- (4) final file OK; non-final non-dir component FAILS ---- */
	{
		fat12_dir_t d;
		dir_entry_t e;
		rc = fat12_resolve_path(&vol, sector_buf, fat_buf, sizeof(fat_buf),
		                        "\\SUB\\NESTED.TXT", &d, &e);
		CHECK(rc == FAT12_OK, "resolve('\\SUB\\NESTED.TXT') => OK (final file)");
		CHECK(rc == FAT12_OK && (e.attribute & DIR_ATTR_DIRECTORY) == 0,
		      "'\\SUB\\NESTED.TXT' final entry is a regular file (not a directory)");
		CHECK(rc == FAT12_OK && d.is_root == 0 && d.start_cluster == sub_start,
		      "'\\SUB\\NESTED.TXT' containing directory is SUB");

		/* A NON-FINAL component that is not a directory must be rejected. */
		rc = fat12_resolve_path(&vol, sector_buf, fat_buf, sizeof(fat_buf),
		                        "\\SUB\\NESTED.TXT\\X", &d, &e);
		CHECK(rc == FAT12_ERR_NOT_FOUND,
		      "resolve('\\SUB\\NESTED.TXT\\X') => NOT_FOUND (file is not traversable)");

		/* The LOAD-BEARING attr-gate assertion (Rule 6, mutation-observable):
		 * NESTED.TXT's first 32 bytes are a SYNTHETIC dir entry 'PEEKDIR' (attr
		 * 0x10) -- see the fixture. With the DIR_ATTR_DIRECTORY gate REMOVED, a
		 * resolver would descend into NESTED.TXT-as-a-directory, find PEEKDIR,
		 * and WRONGLY resolve '\SUB\NESTED.TXT\PEEKDIR'. The correct resolver
		 * rejects it at the file (NOT_FOUND) without ever reading the cluster. */
		rc = fat12_resolve_path(&vol, sector_buf, fat_buf, sizeof(fat_buf),
		                        "\\SUB\\NESTED.TXT\\PEEKDIR", &d, &e);
		CHECK(rc == FAT12_ERR_NOT_FOUND,
		      "resolve('\\SUB\\NESTED.TXT\\PEEKDIR') => NOT_FOUND (the file's data is "
		      "NOT a directory -- the attr gate refuses to descend into it)");
	}

	/* ---- (5) leading 'A:' stripped: 'A:\\SUB' == '\\SUB' ---- */
	{
		fat12_dir_t d1, d2;
		dir_entry_t e1, e2;
		rc = fat12_resolve_path(&vol, sector_buf, fat_buf, sizeof(fat_buf),
		                        "\\SUB", &d1, &e1);
		CHECK(rc == FAT12_OK, "resolve('\\SUB') for the drive-prefix compare");
		rc = fat12_resolve_path(&vol, sector_buf, fat_buf, sizeof(fat_buf),
		                        "A:\\SUB", &d2, &e2);
		CHECK(rc == FAT12_OK, "resolve('A:\\SUB') should return FAT12_OK");
		CHECK(rc == FAT12_OK && e1.start_cluster == e2.start_cluster,
		      "'A:\\SUB' resolves identically to '\\SUB' (drive prefix stripped)");
	}

	/* ---- (6) '..' pops to parent; '.' is identity ---- */
	{
		fat12_dir_t d;
		dir_entry_t e;
		/* '\\SUB\\DEEP\\..' should land back on SUB (the directory itself). */
		rc = fat12_resolve_path(&vol, sector_buf, fat_buf, sizeof(fat_buf),
		                        "\\SUB\\DEEP\\..", &d, &e);
		CHECK(rc == FAT12_OK, "resolve('\\SUB\\DEEP\\..') should return FAT12_OK");
		CHECK(rc == FAT12_OK && d.is_root == 0 && d.start_cluster == sub_start,
		      "'\\SUB\\DEEP\\..' => SUB (parent popped)");

		/* '\\SUB\\.' should be SUB itself (identity). */
		rc = fat12_resolve_path(&vol, sector_buf, fat_buf, sizeof(fat_buf),
		                        "\\SUB\\.", &d, &e);
		CHECK(rc == FAT12_OK, "resolve('\\SUB\\.') should return FAT12_OK");
		CHECK(rc == FAT12_OK && d.is_root == 0 && d.start_cluster == sub_start,
		      "'\\SUB\\.' => SUB (dot is identity)");

		/* '\\SUB\\DEEP\\..\\..' should land back on the root. */
		rc = fat12_resolve_path(&vol, sector_buf, fat_buf, sizeof(fat_buf),
		                        "\\SUB\\DEEP\\..\\..", &d, &e);
		CHECK(rc == FAT12_OK, "resolve('\\SUB\\DEEP\\..\\..') should return FAT12_OK");
		CHECK(rc == FAT12_OK && d.is_root == 1,
		      "'\\SUB\\DEEP\\..\\..' => root (start_cluster 0 normalizes to root)");
	}

	/* ---- (7) BIGDIR multi-cluster: >16 files AND FILE20 in cluster 2+ ---- *
	 * THE load-bearing assertion. BIGDIR holds . + .. + 40 files = 42 entries
	 * = 3 clusters; a single-sector reader stops after entry 16 and never sees
	 * FILE17..FILE40. We require >16 regular files AND a byte-for-byte read of
	 * FILE20.TXT (which lives in the 2nd/3rd cluster) vs big_fill.txt. */
	{
		fat12_dir_t bigdir;
		dir_entry_t bde;
		namelist_t  nl;
		int         regs;
		int         i;

		rc = fat12_resolve_path(&vol, sector_buf, fat_buf, sizeof(fat_buf),
		                        "\\BIGDIR", &bigdir, &bde);
		CHECK(rc == FAT12_OK && (bde.attribute & DIR_ATTR_DIRECTORY) != 0,
		      "resolve('\\BIGDIR') => a directory");

		/* Descend into BIGDIR and enumerate it. */
		bigdir.is_root = 0;
		bigdir.start_cluster = bde.start_cluster;
		memset(&nl, 0, sizeof(nl));
		rc = fat12_read_dir(&vol, &bigdir, sector_buf, fat_buf, sizeof(fat_buf),
		                    collect_cb, &nl);
		CHECK(rc == FAT12_OK, "fat12_read_dir(BIGDIR) should complete the scan");

		/* Count regular (.TXT) files -- exclude '.' and '..'. */
		regs = 0;
		for (i = 0; i < nl.count; i++) {
			if ((nl.attrs[i] & DIR_ATTR_DIRECTORY) == 0 &&
			    (nl.attrs[i] & DIR_ATTR_VOLLABEL) == 0) {
				regs++;
			}
		}
		CHECK(regs > 16,
		      "BIGDIR enumeration returns >16 files (multi-cluster chain walked, "
		      "not a single sector)");
		CHECK(regs == 40, "BIGDIR holds all 40 FILEnn.TXT entries");
		CHECK(namelist_find(&nl, "FILE20.TXT") >= 0,
		      "BIGDIR enumeration includes FILE20.TXT (in cluster 2+)");
		CHECK(namelist_find(&nl, "FILE40.TXT") >= 0,
		      "BIGDIR enumeration includes FILE40.TXT (the last, in cluster 3)");
	}

	/* FILE20.TXT read byte-for-byte vs big_fill.txt -- the multi-cluster file. */
	check_path_file(&vol, sector_buf, fat_buf, sizeof(fat_buf),
	                "\\BIGDIR\\FILE20.TXT", bigfill_path);

	/* ---- (8) deep files read byte-for-byte vs the committed fixtures ---- */
	check_path_file(&vol, sector_buf, fat_buf, sizeof(fat_buf),
	                "\\SUB\\NESTED.TXT", nested_path);
	check_path_file(&vol, sector_buf, fat_buf, sizeof(fat_buf),
	                "\\SUB\\DEEP\\DEEP.TXT", deep_path);

	blockdev_file_close(&bf);
	return TEST_SUMMARY("test_fat12_subdir");
}
