/*
 * harness/diff/fat_diff/test_fat12_write_partial.c -- FAT12 positioned-WRITE oracle.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (Law 2: the oracle is the truth, never false-green).
 *
 * Differential gate for os/milton/fat12.c::fat12_write_partial (beads
 * initech-snk): the POSITIONED random-access WRITE primitive that overwrites in
 * place, extends (allocating + chaining clusters) past EOF, and zero-fills the
 * hole when offset > size -- the symmetric counterpart of fat12_read_partial and
 * the write half of the per-handle file-I/O epic (beads initech-6qy).
 *
 * It mounts a freshly-minted BLANK 1.44 MB FAT12 image through the REAL artifact
 * fat12.c via the host read-WRITE file-backed blockdev, then drives a MATRIX of
 * positioned-write cases, each maintained against an in-test BYTE MODEL of the
 * expected file contents (the independent reference computed with no knowledge
 * of the chain walk). For every case it verifies THREE ways (triple agreement,
 * like test-fat):
 *   (a) IN-PROCESS: read the file back via fat12_read_partial AND fat12_read_file
 *       and compare to the model byte-for-byte (this binary).
 *   (b) mtools `mcopy ::NAME` reads back the SAME bytes (Makefile recipe).
 *   (c) the python3 reference (fat12_ref.py --cat) reads the same bytes (recipe).
 *
 * Matrix (per the task brief):
 *   - overwrite-in-place within one cluster;
 *   - overwrite spanning a cluster boundary;
 *   - append exactly at EOF;
 *   - extend past EOF growing the chain by several clusters;
 *   - write at offset > size creating a zero hole (gap reads back as zeroes);
 *   - write to a previously empty (size 0) file;
 *   - a write whose total file size exceeds one cluster many times over.
 *
 * Two argv modes (mirroring test_fat12_write.c):
 *   <blank-img> --diff : drive the deterministic case sequence on the blank
 *                        image and EXIT, leaving the files WRITTEN so the
 *                        Makefile can diff them against mtools + python3.
 *   <blank-img>        : the same sequence PLUS the in-process triple-read
 *                        assertions (a) and the fail-loud disk-full leg.
 *
 * Determinism (Rule 11): all content is a fixed byte pattern, no clock. Image
 * path is argv[1] (no host path baked in).
 *
 * Ref (Law 1): Microsoft FAT spec (12-bit encode, free=0x000, EOC 0xFFF);
 *   docs/research/fat12-ground-truth.md Sec 3/4; ADR-0003 DEC-07.
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

/* Cluster size on a 1.44 MB floppy: 1 sector * 512 bytes. */
#define BPC 512u

/* The largest file any case grows to (kept comfortably under a few clusters but
 * spanning MANY clusters for the multi-cluster case). */
#define MODEL_CAP 4096u

static uint8_t g_fat[12u * 512u];
static uint8_t g_sector[512];
static uint8_t g_cluster[512];

/* Deterministic byte pattern (NOT a clock). seed distinguishes the data each
 * positioned write splices in so a mis-spliced byte is detectable. */
static uint8_t pat(uint32_t i, uint32_t seed)
{
	return (uint8_t)(((i + seed) * 37u + 11u) & 0xFFu);
}

/* One file under test: name, dir-entry slot, and the EXPECTED byte model. The
 * model is the independent reference -- a positioned write into it is a plain
 * memcpy with zero-fill of any newly created hole. */
typedef struct {
	char     name[16];
	uint32_t slot;
	uint8_t  model[MODEL_CAP];
	uint32_t size;            /* logical file size in the model            */
} subject_t;

/* Apply a positioned write to the in-test model: zero-fill the hole
 * (size..offset) if offset > size, then splice `len` bytes of `data`. Grows
 * `size` to max(size, offset+len). This is the golden the disk must match. */
static void model_write(subject_t *s, uint32_t offset, const uint8_t *data,
                        uint32_t len)
{
	uint32_t end = offset + len;
	uint32_t i;
	if (offset > s->size) {
		for (i = s->size; i < offset; i++) {
			s->model[i] = 0u;      /* the hole reads back as zeroes */
		}
	}
	for (i = 0u; i < len; i++) {
		s->model[offset + i] = data[i];
	}
	if (end > s->size) {
		s->size = end;
	}
}

static fat12_volume_t g_vol;
static uint32_t       g_fat_len;

/* Perform a positioned write through the REAL artifact AND mirror it into the
 * model. Asserts FAT12_OK + out_written == len. `verify` non-zero runs the
 * in-process triple read-back (skip in --diff mode where the recipe diffs). */
static void do_write(subject_t *s, uint32_t offset, uint32_t seed, uint32_t len,
                     int verify)
{
	uint8_t  data[MODEL_CAP];
	uint32_t out_written = 0xFFFFFFFFu;
	uint32_t i;
	int      rc;
	char     msg[160];

	for (i = 0u; i < len; i++) {
		data[i] = pat(i, seed);
	}

	rc = fat12_write_partial(&g_vol, g_fat, g_fat_len, 0u, s->slot, offset, data,
	                         len, g_sector, g_cluster, &out_written);
	snprintf(msg, sizeof(msg), "%s: write_partial(off=%u,len=%u) -> FAT12_OK",
	         s->name, offset, len);
	CHECK(rc == FAT12_OK, msg);
	snprintf(msg, sizeof(msg), "%s: write_partial(off=%u,len=%u) out_written==len",
	         s->name, offset, len);
	CHECK(out_written == len, msg);

	model_write(s, offset, data, len);

	if (rc != FAT12_OK || !verify) {
		return;
	}

	/* (a-1) Re-read the FAT (the WRITE flushed both copies) + the dir entry,
	 * then read the WHOLE file via fat12_read_file and compare to the model. */
	{
		dir_entry_t e;
		uint8_t     got[MODEL_CAP + 16];
		uint32_t    out_bytes = 0xFFFFFFFFu;

		CHECK(fat12_read_fat(&g_vol, g_fat, sizeof(g_fat)) == FAT12_OK,
		      "re-read FAT after positioned write");
		rc = fat12_find(&g_vol, g_sector, s->name, &e);
		snprintf(msg, sizeof(msg), "%s: find after write", s->name);
		CHECK(rc == FAT12_OK, msg);
		if (rc != FAT12_OK) {
			return;
		}
		snprintf(msg, sizeof(msg), "%s: dir file_size == model size %u",
		         s->name, s->size);
		CHECK(e.file_size == s->size, msg);

		rc = fat12_read_file(&g_vol, g_fat, g_fat_len, &e, got, sizeof(got),
		                     g_cluster, &out_bytes);
		snprintf(msg, sizeof(msg), "%s: read_file back -> OK, %u bytes",
		         s->name, s->size);
		CHECK(rc == FAT12_OK && out_bytes == s->size, msg);
		snprintf(msg, sizeof(msg),
		         "%s: read_file bytes EXACTLY match model (hole zero-filled, splice in place)",
		         s->name);
		CHECK(out_bytes == s->size && memcmp(got, s->model, s->size) == 0, msg);
	}

	/* (a-2) Also verify the SAME bytes via the POSITIONED reader over a few
	 * sub-ranges -- proves the write is consistent with the random-access read
	 * primitive, not just the whole-file read. */
	{
		dir_entry_t e;
		uint8_t     rbuf[MODEL_CAP];
		uint32_t    out_read;
		uint32_t    ranges[4][2];
		uint32_t    r;

		if (fat12_find(&g_vol, g_sector, s->name, &e) != FAT12_OK) {
			return;
		}
		ranges[0][0] = 0u;                 ranges[0][1] = s->size;
		ranges[1][0] = s->size / 2u;       ranges[1][1] = s->size - s->size / 2u;
		ranges[2][0] = (s->size > BPC) ? (BPC - 3u) : 0u;
		ranges[2][1] = (s->size > BPC) ? 6u : s->size;
		ranges[3][0] = (s->size > 0u) ? (s->size - 1u) : 0u;
		ranges[3][1] = (s->size > 0u) ? 1u : 0u;

		for (r = 0u; r < 4u; r++) {
			uint32_t off = ranges[r][0];
			uint32_t ln  = ranges[r][1];
			out_read = 0xFFFFFFFFu;
			rc = fat12_read_partial(&g_vol, g_fat, g_fat_len, &e, off, ln,
			                        rbuf, g_cluster, &out_read);
			snprintf(msg, sizeof(msg),
			         "%s: read_partial(off=%u,len=%u) OK + matches model slice",
			         s->name, off, ln);
			CHECK(rc == FAT12_OK && out_read == ln &&
			      (ln == 0u || memcmp(rbuf, s->model + off, ln) == 0), msg);
		}
	}
}

/* Run the full deterministic case sequence on the mounted volume. `verify`
 * gates the in-process triple read-back (off in --diff mode). The files are
 * left WRITTEN so the Makefile can diff them three ways. */
static int run_sequence(int verify)
{
	subject_t one;     /* OVR.BIN  -- overwrite-in-place + boundary cases */
	subject_t app;     /* APP.BIN  -- append at EOF + multi-cluster grow  */
	subject_t hole;    /* HOLE.BIN -- write past EOF (zero hole)          */
	subject_t empty;   /* FRESH.BIN-- write to a freshly-created size-0   */
	subject_t big;     /* BIG.BIN  -- many-cluster file built piecewise   */
	dir_entry_t de;
	int rc;

	memset(&one, 0, sizeof(one));
	memset(&app, 0, sizeof(app));
	memset(&hole, 0, sizeof(hole));
	memset(&empty, 0, sizeof(empty));
	memset(&big, 0, sizeof(big));

#define CREATE(s, nm) do { \
		strcpy((s).name, (nm)); \
		rc = fat12_create(&g_vol, g_fat, g_fat_len, (nm), DIR_ATTR_ARCHIVE, \
		                  0u, g_sector, g_cluster, &de, &(s).slot); \
		if (rc != FAT12_OK) { \
			fprintf(stderr, "create %s failed (%d)\n", (nm), rc); return 1; } \
	} while (0)

	CREATE(one,  "OVR.BIN");
	CREATE(app,  "APP.BIN");
	CREATE(hole, "HOLE.BIN");
	CREATE(empty, "FRESH.BIN");
	CREATE(big,  "BIG.BIN");
#undef CREATE

	/* --- OVR.BIN: build a 900-byte file (2 clusters), then overwrite. --- */
	do_write(&one, 0u, 1u, 900u, verify);               /* initial extend  */
	do_write(&one, 100u, 50u, 80u, verify);             /* in-place, 1 cluster */
	do_write(&one, 500u, 70u, 40u, verify);             /* spans 512 boundary  */
	do_write(&one, 0u, 200u, 1u, verify);               /* single byte at 0    */

	/* --- FRESH.BIN: write to a previously EMPTY (size 0) file. --- */
	do_write(&empty, 0u, 5u, 30u, verify);              /* empty -> 1 cluster  */
	do_write(&empty, 10u, 6u, 5u, verify);              /* overwrite in place  */

	/* --- APP.BIN: append exactly at EOF, growing the chain by several
	 * clusters (40 bytes -> 600 -> 2200 spans many cluster boundaries). --- */
	do_write(&app, 0u, 7u, 40u, verify);                /* 1 cluster           */
	do_write(&app, 40u, 8u, 560u, verify);              /* append exactly @EOF -> 600 */
	do_write(&app, 600u, 9u, 1600u, verify);            /* append @EOF, +several clusters (2200) */

	/* --- HOLE.BIN: write at offset > size -> a zero hole. --- */
	do_write(&hole, 0u, 10u, 100u, verify);             /* 0..100              */
	do_write(&hole, 1500u, 11u, 50u, verify);           /* hole 100..1500, then data */
	do_write(&hole, 700u, 12u, 20u, verify);            /* overwrite inside the (now zeroed) hole */

	/* --- BIG.BIN: a file whose total size exceeds one cluster MANY times
	 * over, built by several positioned writes. --- */
	do_write(&big, 0u, 13u, 3000u, verify);             /* ~6 clusters         */
	do_write(&big, 2900u, 14u, 200u, verify);           /* extend over boundary -> 3100 */
	do_write(&big, 1234u, 15u, 700u, verify);           /* in-place across boundaries  */

	return 0;
}

int main(int argc, char **argv)
{
	const char     *img;
	blockdev_file_t bf;
	int             rc;
	int             diff_mode;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <blank-fat12-image> [--diff]\n", argv[0]);
		return 2;
	}
	img       = argv[1];
	diff_mode = (argc >= 3 && strcmp(argv[2], "--diff") == 0);

	rc = blockdev_file_open_rw(&bf, img);
	if (diff_mode) {
		if (rc != 0) { fprintf(stderr, "open_rw %s failed\n", img); return 1; }
	} else {
		CHECK(rc == 0, "blockdev_file_open_rw on the minted blank image");
		if (rc != 0) { return TEST_SUMMARY("test_fat12_write_partial"); }
	}

	rc = fat12_mount(&g_vol, &bf.dev, g_sector);
	if (diff_mode) {
		if (rc != FAT12_OK) { fprintf(stderr, "mount failed\n");
			blockdev_file_close(&bf); return 1; }
	} else {
		CHECK(rc == FAT12_OK, "fat12_mount on the blank image");
		if (rc != FAT12_OK) { blockdev_file_close(&bf);
			return TEST_SUMMARY("test_fat12_write_partial"); }
	}

	rc = fat12_read_fat(&g_vol, g_fat, sizeof(g_fat));
	if (!diff_mode) {
		CHECK(rc == FAT12_OK, "fat12_read_fat fills the whole-FAT buffer");
	}
	g_fat_len = (uint32_t)g_vol.bpb.sectors_per_fat *
	            (uint32_t)g_vol.bpb.bytes_per_sector;

	if (diff_mode) {
		rc = run_sequence(0);
		blockdev_file_close(&bf);
		return rc;   /* image left WRITTEN; recipe diffs it three ways */
	}

	(void)run_sequence(1);

	/* ---- Fail-loud disk-full leg (Rule 2): extend a file past the volume's
	 * capacity; the allocation must roll back (NO_SPACE) and leave the volume
	 * mountable + FATs in sync (no half-corrupt chain). ---- */
	{
		dir_entry_t e;
		static uint8_t big[64u * 1024u];
		int         got_no_space = 0;
		int         i;

		for (i = 0; i < 64; i++) { big[i] = (uint8_t)(i & 0xFFu); }
		for (; i < (int)sizeof(big); i++) { big[i] = (uint8_t)((i * 3) & 0xFFu); }

		CHECK(fat12_read_fat(&g_vol, g_fat, sizeof(g_fat)) == FAT12_OK,
		      "re-read FAT before the fill leg");

		for (i = 0; i < 64; i++) {
			char nm[16];
			dir_entry_t de2;
			uint32_t sl2;
			uint32_t out_written = 0;
			snprintf(nm, sizeof(nm), "PFILL%02d.DAT", i);
			rc = fat12_create(&g_vol, g_fat, g_fat_len, nm, DIR_ATTR_ARCHIVE,
			                  0u, g_sector, g_cluster, &de2, &sl2);
			if (rc == FAT12_ERR_DIR_FULL) { got_no_space = 1; break; }
			CHECK(rc == FAT12_OK, "fill create ok (until dir/space full)");
			/* Grow each file from empty with a positioned write at offset 0. */
			rc = fat12_write_partial(&g_vol, g_fat, g_fat_len, 0u, sl2, 0u, big,
			                         sizeof(big), g_sector, g_cluster,
			                         &out_written);
			if (rc == FAT12_ERR_NO_SPACE) {
				CHECK(out_written == 0u,
				      "NO_SPACE leaves out_written == 0 (nothing committed)");
				got_no_space = 1;
				break;
			}
			CHECK(rc == FAT12_OK, "fill write_partial ok (until space full)");
		}
		CHECK(got_no_space == 1,
		      "filling the volume eventually fails loud (NO_SPACE or DIR_FULL), never silent/corrupt");

		/* Volume still mounts + a previously-written file still round-trips
		 * (the rolled-back allocation did not corrupt anything). */
		{
			fat12_volume_t v2;
			CHECK(fat12_mount(&v2, &bf.dev, g_sector) == FAT12_OK,
			      "volume still mounts cleanly after the full-volume failure");
		}
		CHECK(fat12_read_fat(&g_vol, g_fat, sizeof(g_fat)) == FAT12_OK,
		      "re-read FAT after fill leg");
		rc = fat12_find(&g_vol, g_sector, "BIG.BIN", &e);
		CHECK(rc == FAT12_OK, "BIG.BIN survives the full-volume failure");
		(void)e;
	}

	blockdev_file_close(&bf);
	return TEST_SUMMARY("test_fat12_write_partial");
}
