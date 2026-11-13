/*
 * harness/diff/fat_diff/test_fat12_partial.c -- FAT12 positioned-read oracle.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (Law 2: the oracle is the truth, never false-green).
 *
 * Differential gate for os/milton/fat12.c::fat12_read_partial (beads
 * initech-lq2): the POSITIONED random-access read primitive that walks ONLY the
 * cluster chain it needs (never the whole file), the foundation of the
 * per-handle file-I/O epic (beads initech-6qy).
 *
 * It mounts the same minted 1.44 MB FAT12 image as test_fat12_dir.c through the
 * REAL artifact fat12.c via the host file-backed blockdev, then, for a MATRIX of
 * (offset, len) cases over each fixture file, asserts that fat12_read_partial's
 * bytes EXACTLY equal the corresponding slice of the committed fixture file
 * (the byte-for-byte golden, read directly -- an INDEPENDENT reference computed
 * with no knowledge of the chain walk). The Makefile gate (test-fat-partial)
 * adds a THIRD reference: the independent python3 reader's --cat-range mode.
 *
 * Matrix (per the task brief): offset 0; a mid-cluster offset; an offset
 * spanning a cluster boundary; offset at EOF; offset past EOF; len longer than
 * the file; and -- on CHAIN.TXT (1600 bytes => 4 clusters, partial last) and
 * BLOCK.BIN (1024 bytes => 2 full clusters) -- a read crossing MANY clusters.
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 3 (chain link decode) +
 *   Sec 4 / RISK-5 (file_size authoritative, no padding). Image + fixture-dir
 *   paths are argv (Makefile) -> no host path baked in (Rule 11).
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
 * -1 on error. The fixture files are the golden the slices are compared to. */
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

/* The whole-file golden + size for the file under test, shared by every case. */
typedef struct {
	const fat12_volume_t *vol;
	void                 *sector_buf;
	const void           *fat;
	uint32_t              fat_len;
	const char           *name83;
	dir_entry_t           ent;
	const uint8_t        *golden;   /* committed fixture bytes (whole file) */
	uint32_t              size;     /* golden length == file_size           */
} subject_t;

/* Run one positioned-read case and assert it matches the golden slice exactly.
 *
 * Expected result (the INDEPENDENT computation): a positioned read of
 * [offset, offset+len) returns min(len, size-offset) bytes when offset < size,
 * or zero bytes when offset >= size (clean EOF). The returned bytes equal
 * golden[offset .. offset+want]. A guard byte just past the expected count
 * catches an over-long copy. */
static void run_case(subject_t *s, uint32_t offset, uint32_t len)
{
	uint8_t   out[4096 + 1];
	uint8_t   cluster_buf[512];   /* sectors_per_cluster(1) * 512 (floppy) */
	uint32_t  out_read = 0xFFFFFFFFu;
	uint32_t  want;
	int       rc;
	char      msg[160];
	const uint8_t GUARD = 0x7Eu;

	/* Independent expected count: clamp len to what remains from offset. */
	if (offset >= s->size) {
		want = 0u;
	} else {
		uint32_t avail = s->size - offset;
		want = (len < avail) ? len : avail;
	}

	if (want + 1u > sizeof(out)) {
		snprintf(msg, sizeof(msg),
		         "%s off=%u len=%u: case exceeds out[] capacity (test bug)",
		         s->name83, offset, len);
		CHECK(0, msg);
		return;
	}

	/* Poison the slot just past the expected count: a correct read writes only
	 * `want` bytes and leaves the guard intact; an over-copy trips it. */
	memset(out, 0, sizeof(out));
	out[want] = GUARD;

	rc = fat12_read_partial(s->vol, s->fat, s->fat_len, &s->ent,
	                        offset, len, out, cluster_buf, &out_read);

	snprintf(msg, sizeof(msg),
	         "%s off=%u len=%u: fat12_read_partial returns FAT12_OK",
	         s->name83, offset, len);
	CHECK(rc == FAT12_OK, msg);
	if (rc != FAT12_OK) {
		return;
	}

	snprintf(msg, sizeof(msg),
	         "%s off=%u len=%u: out_read == min(len, size-offset) == %u",
	         s->name83, offset, len, want);
	CHECK(out_read == want, msg);

	snprintf(msg, sizeof(msg),
	         "%s off=%u len=%u: guard past out_read untouched (no over-copy)",
	         s->name83, offset, len);
	CHECK(out[want] == GUARD, msg);

	/* The load-bearing assertion: the returned bytes EXACTLY equal the golden
	 * slice golden[offset .. offset+want]. */
	if (out_read == want) {
		int eq = (want == 0u) ||
		         (memcmp(out, s->golden + offset, want) == 0);
		snprintf(msg, sizeof(msg),
		         "%s off=%u len=%u: bytes EXACTLY match golden slice",
		         s->name83, offset, len);
		CHECK(eq, msg);
	}
}

/* Build the subject (find + golden) then drive the full (offset,len) matrix. */
static void exercise(const fat12_volume_t *vol, void *sector_buf,
                     const void *fat, uint32_t fat_len,
                     const char *name83, const char *golden_path,
                     uint8_t *golden_buf, long golden_cap)
{
	subject_t s;
	long      glen;
	int       rc;
	char      msg[160];
	uint32_t  size;

	rc = fat12_find(vol, sector_buf, name83, &s.ent);
	snprintf(msg, sizeof(msg), "fat12_find(%s) for the partial-read subject", name83);
	CHECK(rc == FAT12_OK, msg);
	if (rc != FAT12_OK) {
		return;
	}

	glen = read_whole_file(golden_path, golden_buf, golden_cap);
	snprintf(msg, sizeof(msg), "read golden fixture %s", golden_path);
	CHECK(glen >= 0, msg);
	if (glen < 0) {
		return;
	}
	snprintf(msg, sizeof(msg), "%s golden size == dir file_size", name83);
	CHECK((uint32_t)glen == s.ent.file_size, msg);

	s.vol        = vol;
	s.sector_buf = sector_buf;
	s.fat        = fat;
	s.fat_len    = fat_len;
	s.name83     = name83;
	s.golden     = golden_buf;
	s.size       = (uint32_t)glen;
	size         = s.size;

	/* --- The (offset, len) matrix --- */

	/* offset 0, a small len entirely within the first cluster. */
	run_case(&s, 0u, (size < 16u) ? size : 16u);

	/* offset 0, the WHOLE file in one positioned read. */
	run_case(&s, 0u, size);

	/* offset 0, len LONGER than the file -> clamps to file_size. */
	run_case(&s, 0u, size + 4096u);

	if (size >= 4u) {
		/* A small mid-file offset (still within the first cluster for small
		 * files; a mid-cluster start in general). */
		run_case(&s, size / 2u, (size / 4u) ? (size / 4u) : 1u);
		/* A 1-byte read at a mid-file offset (the within-cluster offset path). */
		run_case(&s, size / 2u, 1u);
	}

	/* A read whose offset lands inside the FIRST data cluster (mid-cluster) but
	 * is non-zero -- exercises cluster_pos on the first cluster. */
	if (size > 100u) {
		run_case(&s, 100u, 50u);
	}

	/* offset AT EOF -> zero bytes, clean (return OK, out_read 0). */
	run_case(&s, size, 32u);

	/* offset PAST EOF -> zero bytes, clean. */
	run_case(&s, size + 7u, 32u);

	/* len 0 at a valid offset -> zero bytes, clean. */
	if (size > 0u) {
		run_case(&s, 0u, 0u);
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
	uint8_t         golden[4096];
	int             rc;
	char            hello_path[512];
	char            second_path[512];
	char            chain_path[512];
	char            empty_path[512];
	char            block_path[512];

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

	rc = blockdev_file_open(&bf, img);
	CHECK(rc == 0, "blockdev_file_open should succeed on the minted image");
	if (rc != 0) {
		return TEST_SUMMARY("test_fat12_partial");
	}

	rc = fat12_mount(&vol, &bf.dev, sector_buf);
	CHECK(rc == FAT12_OK, "fat12_mount should return FAT12_OK on the 1.44MB image");

	rc = fat12_read_fat(&vol, fat_buf, sizeof(fat_buf));
	CHECK(rc == FAT12_OK, "fat12_read_fat should fill the whole-FAT buffer");

	/* HELLO.TXT: 31 bytes, single cluster -- the within-one-cluster cases. */
	exercise(&vol, sector_buf, fat_buf, sizeof(fat_buf),
	         "HELLO.TXT", hello_path, golden, (long)sizeof(golden));

	/* SECOND.TXT: 104 bytes, single cluster. */
	exercise(&vol, sector_buf, fat_buf, sizeof(fat_buf),
	         "SECOND.TXT", second_path, golden, (long)sizeof(golden));

	/* CHAIN.TXT: 1600 bytes => 4 clusters (partial last). The MULTI-CLUSTER /
	 * cluster-boundary-spanning case. */
	exercise(&vol, sector_buf, fat_buf, sizeof(fat_buf),
	         "CHAIN.TXT", chain_path, golden, (long)sizeof(golden));

	/* BLOCK.BIN: 1024 bytes => exactly 2 full clusters (boundary at 512). */
	exercise(&vol, sector_buf, fat_buf, sizeof(fat_buf),
	         "BLOCK.BIN", block_path, golden, (long)sizeof(golden));

	/* EMPTY.TXT: 0 bytes -- every offset is at/after EOF (clean-EOF path; no
	 * chain walk, no fat/out_buf/cluster_buf touched). */
	exercise(&vol, sector_buf, fat_buf, sizeof(fat_buf),
	         "EMPTY.TXT", empty_path, golden, (long)sizeof(golden));

	/* ---- Targeted cluster-boundary + cross-many-cluster cases on CHAIN.TXT ----
	 * 512-byte clusters: probe reads that START just before a boundary and SPAN
	 * into the next cluster, a read that crosses ALL the chain's boundaries, and
	 * a mid-cluster offset in a LATER cluster (cluster_pos on a skipped chain). */
	{
		subject_t s;
		long      glen;

		rc = fat12_find(&vol, sector_buf, "CHAIN.TXT", &s.ent);
		CHECK(rc == FAT12_OK, "fat12_find(CHAIN.TXT) for the boundary cases");
		glen = read_whole_file(chain_path, golden, (long)sizeof(golden));
		CHECK(glen == 1600, "CHAIN.TXT golden is 1600 bytes (4x512 partial last)");

		s.vol = &vol; s.sector_buf = sector_buf; s.fat = fat_buf;
		s.fat_len = sizeof(fat_buf); s.name83 = "CHAIN.TXT";
		s.golden = golden; s.size = (uint32_t)glen;

		/* Spans the first cluster boundary (offset 500, len 100 -> 500..600). */
		run_case(&s, 500u, 100u);
		/* Starts mid in the 2nd cluster, spans into the 3rd (600..1100). */
		run_case(&s, 600u, 500u);
		/* Starts mid in the LAST (4th, partial) cluster (1550..1600, clamped). */
		run_case(&s, 1550u, 200u);
		/* Crosses ALL boundaries: offset 1, len to the end (1..1600). */
		run_case(&s, 1u, 1599u);
		/* A mid-cluster offset deep in the chain, exactly one cluster wide
		 * (1024..1536 -> cluster_pos 0 in cluster #3, full 512). */
		run_case(&s, 1024u, 512u);
		/* A mid-cluster offset deep in the chain, sub-cluster (1030..1100). */
		run_case(&s, 1030u, 70u);
	}

	blockdev_file_close(&bf);
	return TEST_SUMMARY("test_fat12_partial");
}
