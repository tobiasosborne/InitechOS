/*
 * harness/diff/fat_diff/fat12_fuzz.c -- deterministic, seeded, shrinking
 * generative fuzzer for the FAT12 + positioned-file-I/O layer (os/milton/fat12.c).
 *
 * FACTORY host tool (CLAUDE.md Law 3): libc OK; the SUBJECT under test is the
 * UNMODIFIED artifact fat12.c, driven through the host file-backed blockdev
 * (blockdev_file.c). NOTHING about the artifact is touched here.
 *
 * Why (beads initech-0dq): the FAT12 + multi-tenant file-I/O layer's existing
 * oracles (test-fat[-write][-partial], test-multiopen) are fixed ENUMERATED
 * matrices + mutants -- strong, but they do not explore the deep state space
 * (Rule 3: "all bugs are deep"). This is the standing GENERATIVE fuzzer: a
 * deterministic PRNG drives random positioned CREATE/WRITE/READ/UNLINK op
 * sequences across a small pool of files (the multi-tenant path), maintains an
 * INDEPENDENT byte MODEL of the filesystem, and after every mutating op asserts
 * THREE-WAY agreement (Rule 5):
 *   (a) IN-PROCESS  : fat12_read_partial / fat12_read_file from the live image
 *                     == the model bytes;
 *   (b) REMOUNT     : re-mount the image fresh and read again (catches FAT/dir
 *                     corruption that only shows on a clean remount);
 *   (c) EXTERNAL    : at run end (and during shrink), mtools `mcopy ::NAME` AND
 *                     python3 fat12_ref.py --cat read every live file == model.
 *
 * Determinism (Rule 11): a splitmix64-seeded xorshift PRNG; NO time()/rand()/
 * clock. Same --seed => identical op sequence => identical result, so a failure
 * REPLAYS exactly by seed. The gate runs a fixed seed sweep (deterministic).
 *
 * SHRINKING (the human-facing artifact): on a failing run the recorded op list
 * is minimized -- greedily drop ops that still reproduce the failure, then
 * minimize offsets/lens -- and the SEED + minimal recipe + first diverging
 * file/offset are printed (replayable). Determinism is what makes replay work.
 *
 * Ref (Law 1): Microsoft FAT spec (12-bit encode/decode, free=0x000, EOC 0xFFF);
 *   docs/research/fat12-ground-truth.md Sec 3/4; ADR-0003 DEC-07 (both FATs in
 *   sync). Positioned-write semantics (overwrite / extend / zero-hole) per
 *   fat12.h fat12_write_partial. The model is the independent oracle: a plain
 *   memcpy / zero-fill-hole over a byte vector, no knowledge of the chain walk.
 *
 * Exit code: 0 if every seed's every op agreed; non-zero on the FIRST divergence
 * (after printing the shrunk reproducer) so the Makefile gate goes RED (Law 2).
 *
 * ASCII-clean (Rule 12).
 *
 * Usage:
 *   fat12_fuzz --seed N [--ops K] [--no-external] <blank-fat12-image-template>
 *   fat12_fuzz --sweep A B [--ops K] [--no-external] <blank-fat12-image-template>
 *
 * The image argument is a path to a BLANK 1.44 MB FAT12 image; each run COPIES
 * it to a fresh scratch image (so every run starts from the same blank volume).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "fat12.h"
#include "blockdev_file.h"

/* ---- Geometry / model limits (a 1.44 MB floppy: 512-byte clusters). -------- */
#define BPC            512u        /* bytes per cluster on 1.44 MB             */
#define MAX_FILES      6u          /* size of the fixed 8.3 name pool          */
#define FILE_CAP       65536u      /* per-file model cap (spans MANY clusters) */
#define MAX_OPS        256u        /* hard cap on a recorded op sequence       */

/* ---- The fixed 8.3 name pool. Small so collisions / reuse happen often. ---- */
static const char *const NAME_POOL[MAX_FILES] = {
	"FZA.BIN", "FZB.BIN", "FZC.DAT", "FZD.DAT", "FZE.TXT", "FZF.TXT"
};

/* ---- Op kinds. -------------------------------------------------------------- */
enum {
	OP_CREATE = 0,   /* create (truncate if exists)                            */
	OP_WRITE,        /* positioned write at (offset,len) with a pattern seed   */
	OP_READ_PARTIAL, /* positioned read of (offset,len), check vs model       */
	OP_READ_FILE,    /* whole-file read, check vs model                       */
	OP_UNLINK,       /* delete                                                 */
	OP_KIND_COUNT
};

/* One recorded op. `fi` is the file-pool index; offset/len/pat used by WRITE
 * and READ_PARTIAL. Recording every op makes the run replayable + shrinkable. */
typedef struct {
	int      kind;
	uint32_t fi;
	uint32_t offset;
	uint32_t len;
	uint32_t pat;     /* pattern seed for WRITE so a mis-spliced byte is caught */
} op_t;

/* ---- Deterministic PRNG: splitmix64 seed -> xorshift128+ stream (Rule 11). -- */
typedef struct { uint64_t s0, s1; } prng_t;

static uint64_t splitmix64(uint64_t *x)
{
	uint64_t z = (*x += 0x9E3779B97F4A7C15ull);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
	return z ^ (z >> 31);
}

static void prng_seed(prng_t *p, uint64_t seed)
{
	uint64_t x = seed + 0x1234567890ABCDEFull;  /* fixed offset; no clock */
	p->s0 = splitmix64(&x);
	p->s1 = splitmix64(&x);
}

static uint64_t prng_next(prng_t *p)
{
	uint64_t s1 = p->s0;
	uint64_t s0 = p->s1;
	uint64_t r  = s0 + s1;
	p->s0 = s0;
	s1 ^= s1 << 23;
	p->s1 = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
	return r;
}

/* Uniform-ish in [0, n) (n > 0); fine for fuzz selection (not crypto). */
static uint32_t prng_below(prng_t *p, uint32_t n)
{
	return (uint32_t)(prng_next(p) % (uint64_t)n);
}

/* ---- The independent MODEL: a byte vector + exists flag per pool slot. ------ */
typedef struct {
	int      exists;
	uint8_t  bytes[FILE_CAP];
	uint32_t size;
} model_file_t;

/* Deterministic content pattern (NOT a clock): a function of (byte index, seed)
 * so each positioned write splices a distinguishable stream -- a mis-placed or
 * wrong-cluster byte is detectable. Must match what the real write splices. */
static uint8_t pat_byte(uint32_t i, uint32_t seed)
{
	return (uint8_t)(((i + seed) * 37u + 11u + (seed << 3)) & 0xFFu);
}

/* ---- Live FAT12 state for one run. ------------------------------------------ */
typedef struct {
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint8_t         fat[12u * 512u];
	uint32_t        fat_len;
	uint8_t         sector[512];
	uint8_t         cluster[512];
	uint32_t        slot[MAX_FILES];   /* root-dir slot of each live file     */
	int             live[MAX_FILES];   /* 1 if currently created (has a slot) */
	model_file_t    model[MAX_FILES];
	const char     *img_path;          /* scratch image being mutated         */
} run_t;

/* Scratch read buffer for whole-file / partial read-back (heap, libc OK). */
static uint8_t g_rbuf[FILE_CAP];

/* ---- Failure reporting context (filled on the first divergence). ----------- */
typedef struct {
	int      failed;
	int      op_index;       /* index in the op array that diverged          */
	uint32_t fi;             /* file pool index                              */
	uint32_t diverge_off;    /* first diverging byte offset (or 0)           */
	char     where[96];      /* short human tag of the failing check         */
} fail_t;

static void fail_set(fail_t *f, int op_index, uint32_t fi, uint32_t off,
                     const char *where)
{
	if (f->failed) {
		return;   /* keep the FIRST failure */
	}
	f->failed      = 1;
	f->op_index    = op_index;
	f->fi          = fi;
	f->diverge_off = off;
	snprintf(f->where, sizeof(f->where), "%s", where);
}

/* Find the first index where two byte ranges differ, or len if equal. */
static uint32_t first_diff(const uint8_t *a, const uint8_t *b, uint32_t len)
{
	uint32_t i;
	for (i = 0u; i < len; i++) {
		if (a[i] != b[i]) {
			return i;
		}
	}
	return len;
}

/* ---- In-process read-back check (a) + remount check (b) for one file. ------ *
 * Reads the file via fat12_read_file AND fat12_read_partial (a few sub-ranges)
 * from the CURRENT mount, then RE-MOUNTS the image fresh and repeats the whole-
 * file read. Compares all to the model. Sets *f on the first mismatch. */
static void check_file_inproc(run_t *r, uint32_t fi, int op_index, fail_t *f)
{
	const char    *name = NAME_POOL[fi];
	model_file_t  *m    = &r->model[fi];
	dir_entry_t    e;
	uint32_t       out_bytes;
	int            rc;

	if (f->failed) {
		return;
	}

	/* Re-read the FAT (a mutating op flushed both copies) before any read. */
	if (fat12_read_fat(&r->vol, r->fat, sizeof(r->fat)) != FAT12_OK) {
		fail_set(f, op_index, fi, 0u, "read_fat after op");
		return;
	}

	rc = fat12_find(&r->vol, r->sector, name, &e);
	if (rc != FAT12_OK) {
		fail_set(f, op_index, fi, 0u, "find (live file missing)");
		return;
	}
	if (e.file_size != m->size) {
		fail_set(f, op_index, fi, 0u, "dir file_size != model size");
		return;
	}

	/* (a-1) whole-file read == model. */
	out_bytes = 0xFFFFFFFFu;
	rc = fat12_read_file(&r->vol, r->fat, r->fat_len, &e, g_rbuf, sizeof(g_rbuf),
	                     r->cluster, &out_bytes);
	if (rc != FAT12_OK || out_bytes != m->size) {
		fail_set(f, op_index, fi, 0u, "read_file rc/size");
		return;
	}
	{
		uint32_t d = first_diff(g_rbuf, m->bytes, m->size);
		if (d != m->size) {
			fail_set(f, op_index, fi, d, "read_file bytes != model");
			return;
		}
	}

	/* (a-2) positioned read over a few deterministic sub-ranges == model. */
	{
		uint32_t ranges[5][2];
		uint32_t k;
		uint32_t sz = m->size;
		ranges[0][0] = 0u;                          ranges[0][1] = sz;
		ranges[1][0] = sz / 2u;                     ranges[1][1] = sz - sz / 2u;
		ranges[2][0] = (sz > BPC) ? (BPC - 3u) : 0u;
		ranges[2][1] = (sz > BPC) ? 9u : sz;
		ranges[3][0] = (sz > 0u) ? (sz - 1u) : 0u;
		ranges[3][1] = (sz > 0u) ? 1u : 0u;
		ranges[4][0] = sz;                          ranges[4][1] = 16u; /* past EOF */
		for (k = 0u; k < 5u; k++) {
			uint32_t off = ranges[k][0];
			uint32_t ln  = ranges[k][1];
			uint32_t got = 0xFFFFFFFFu;
			uint32_t want = (off >= sz) ? 0u : ((ln < sz - off) ? ln : sz - off);
			rc = fat12_read_partial(&r->vol, r->fat, r->fat_len, &e, off, ln,
			                        g_rbuf, r->cluster, &got);
			if (rc != FAT12_OK || got != want) {
				fail_set(f, op_index, fi, off, "read_partial rc/count");
				return;
			}
			if (got > 0u) {
				uint32_t d = first_diff(g_rbuf, m->bytes + off, got);
				if (d != got) {
					fail_set(f, op_index, fi, off + d, "read_partial bytes != model");
					return;
				}
			}
		}
	}

	/* (b) REMOUNT fresh and re-read the whole file -- catches FAT/dir
	 * corruption that the still-mounted in-memory FAT might mask. */
	{
		fat12_volume_t v2;
		uint8_t        fat2[12u * 512u];
		uint8_t        sec2[512];
		uint8_t        clu2[512];
		dir_entry_t    e2;
		uint32_t       ob2 = 0xFFFFFFFFu;

		if (fat12_mount(&v2, &r->bf.dev, sec2) != FAT12_OK) {
			fail_set(f, op_index, fi, 0u, "remount failed");
			return;
		}
		if (fat12_read_fat(&v2, fat2, sizeof(fat2)) != FAT12_OK) {
			fail_set(f, op_index, fi, 0u, "remount read_fat");
			return;
		}
		if (fat12_find(&v2, sec2, name, &e2) != FAT12_OK) {
			fail_set(f, op_index, fi, 0u, "remount find");
			return;
		}
		if (e2.file_size != m->size) {
			fail_set(f, op_index, fi, 0u, "remount file_size != model");
			return;
		}
		rc = fat12_read_file(&v2, fat2, r->fat_len, &e2, g_rbuf, sizeof(g_rbuf),
		                     clu2, &ob2);
		if (rc != FAT12_OK || ob2 != m->size) {
			fail_set(f, op_index, fi, 0u, "remount read_file rc/size");
			return;
		}
		{
			uint32_t d = first_diff(g_rbuf, m->bytes, m->size);
			if (d != m->size) {
				fail_set(f, op_index, fi, d, "remount bytes != model");
				return;
			}
		}
	}
}

/* ---- EXTERNAL check (c): mtools mcopy + python3 fat12_ref.py vs model. ------ *
 * For each LIVE file, extract its bytes two independent ways from the on-disk
 * image and compare to the model. Returns 0 if all agree, sets *f + returns -1
 * on the first mismatch. Skipped when do_external == 0 (the per-op checks (a)/(b)
 * run every op; external runs at run-end + during shrink). */
static int run_command(const char *cmd)
{
	int rc = system(cmd);
	return (rc == 0) ? 0 : -1;
}

/* Read a whole file produced by an external tool into buf (cap). Returns the
 * byte count, or (uint32_t)-1 on error. */
static uint32_t slurp(const char *path, uint8_t *buf, uint32_t cap)
{
	FILE    *fp = fopen(path, "rb");
	size_t   n;
	if (fp == NULL) {
		return (uint32_t)-1;
	}
	n = fread(buf, 1u, cap, fp);
	fclose(fp);
	return (uint32_t)n;
}

/* ---- Structural FAT-redundancy check: every on-disk FAT copy must be byte-
 * identical (ADR-0003 DEC-07: both FATs kept in sync; a real DOS/chkdsk reads
 * either copy). Reads the raw image directly -- INDEPENDENT of the artifact's
 * own read path, so a writer that updates only FAT #1 (leaving the redundant
 * copy stale) is caught here even though a content read-back from FAT #1 agrees.
 * Cheap (no shell-out), so it runs at every run-end including the fast sweep. */
static void check_fat_sync(run_t *r, int op_index, fail_t *f)
{
	FILE    *fp;
	uint32_t res_bytes;
	uint32_t spf_bytes;
	uint32_t nfats;
	uint8_t  fat0[12u * 512u];
	uint8_t  fatk[12u * 512u];
	uint32_t k;

	if (f->failed) {
		return;
	}
	res_bytes = (uint32_t)r->vol.first_fat_sector * 512u;
	spf_bytes = (uint32_t)r->vol.bpb.sectors_per_fat * 512u;
	nfats     = (uint32_t)r->vol.bpb.num_fats;
	if (spf_bytes > sizeof(fat0)) {
		return;   /* geometry larger than our scratch -- not this slice */
	}

	fp = fopen(r->img_path, "rb");
	if (fp == NULL) {
		fail_set(f, op_index, 0u, 0u, "fat-sync: reopen image");
		return;
	}
	if (fseek(fp, (long)res_bytes, SEEK_SET) != 0 ||
	    fread(fat0, 1u, spf_bytes, fp) != spf_bytes) {
		fclose(fp);
		fail_set(f, op_index, 0u, 0u, "fat-sync: read FAT #0");
		return;
	}
	for (k = 1u; k < nfats; k++) {
		uint32_t off = res_bytes + k * spf_bytes;
		if (fseek(fp, (long)off, SEEK_SET) != 0 ||
		    fread(fatk, 1u, spf_bytes, fp) != spf_bytes) {
			fclose(fp);
			fail_set(f, op_index, 0u, 0u, "fat-sync: read FAT #k");
			return;
		}
		{
			uint32_t d = first_diff(fat0, fatk, spf_bytes);
			if (d != spf_bytes) {
				fclose(fp);
				fail_set(f, op_index, 0u, d,
				         "fat-sync: redundant FAT copies diverge (DEC-07)");
				return;
			}
		}
	}
	fclose(fp);
}

static void check_external(run_t *r, int op_index, fail_t *f)
{
	uint32_t fi;
	char     cmd[1024];
	char     tmp_mt[256];
	char     tmp_py[256];

	snprintf(tmp_mt, sizeof(tmp_mt), "%s.mt.bin", r->img_path);
	snprintf(tmp_py, sizeof(tmp_py), "%s.py.bin", r->img_path);

	for (fi = 0u; fi < MAX_FILES; fi++) {
		model_file_t *m = &r->model[fi];
		const char   *name = NAME_POOL[fi];
		uint32_t      n;

		if (!r->live[fi] || f->failed) {
			continue;
		}

		/* mtools: mcopy ::NAME out (overwrite). -n suppresses the prompt. */
		snprintf(cmd, sizeof(cmd),
		         "mcopy -n -i '%s' '::%s' '%s' >/dev/null 2>&1",
		         r->img_path, name, tmp_mt);
		if (run_command(cmd) != 0) {
			fail_set(f, op_index, fi, 0u, "mtools mcopy errored");
			return;
		}
		n = slurp(tmp_mt, g_rbuf, sizeof(g_rbuf));
		if (n != m->size) {
			fail_set(f, op_index, fi, 0u, "mtools size != model");
			return;
		}
		{
			uint32_t d = first_diff(g_rbuf, m->bytes, m->size);
			if (d != m->size) {
				fail_set(f, op_index, fi, d, "mtools bytes != model");
				return;
			}
		}

		/* python3 independent reference. */
		snprintf(cmd, sizeof(cmd),
		         "python3 harness/diff/fat_diff/fat12_ref.py '%s' --cat '%s' "
		         "> '%s' 2>/dev/null", r->img_path, name, tmp_py);
		if (run_command(cmd) != 0) {
			fail_set(f, op_index, fi, 0u, "python ref errored");
			return;
		}
		n = slurp(tmp_py, g_rbuf, sizeof(g_rbuf));
		if (n != m->size) {
			fail_set(f, op_index, fi, 0u, "python size != model");
			return;
		}
		{
			uint32_t d = first_diff(g_rbuf, m->bytes, m->size);
			if (d != m->size) {
				fail_set(f, op_index, fi, d, "python bytes != model");
				return;
			}
		}
	}

	remove(tmp_mt);
	remove(tmp_py);
}

/* ---- Apply one op to BOTH the real image and the model. -------------------- *
 * Returns 0 on a normal apply; on an honestly-expected error (disk full on a
 * WRITE) the model is NOT updated (per the artifact's allocate-then-commit
 * rollback contract) and we assert out_written == 0. A genuine unexpected error
 * sets *f. The mutating ops (CREATE/WRITE/UNLINK) are followed by the in-process
 * + remount check for the affected file. */
static void apply_op(run_t *r, const op_t *op, int op_index, fail_t *f)
{
	uint32_t fi = op->fi;

	if (f->failed) {
		return;
	}

	switch (op->kind) {
	case OP_CREATE: {
		dir_entry_t de;
		uint32_t    slot = 0u;
		int rc = fat12_create(&r->vol, r->fat, r->fat_len, NAME_POOL[fi],
		                      DIR_ATTR_ARCHIVE, r->sector, &de, &slot);
		if (rc != FAT12_OK) {
			/* The only honestly-expected non-OK is DIR_FULL; our pool is tiny
			 * (<= MAX_FILES live), so this should not happen -- treat as a bug. */
			fail_set(f, op_index, fi, 0u, "create unexpected error");
			return;
		}
		r->slot[fi] = slot;
		r->live[fi] = 1;
		/* CREATE truncates: model becomes empty. */
		r->model[fi].exists = 1;
		r->model[fi].size   = 0u;
		check_file_inproc(r, fi, op_index, f);
		break;
	}
	case OP_WRITE: {
		uint8_t  *data;
		uint32_t  i;
		uint32_t  out_written = 0xFFFFFFFFu;
		int       rc;

		if (!r->live[fi]) {
			return;   /* skip: nothing to write to (recorded but inert) */
		}
		/* Build the deterministic pattern for [0,len). Heap buffer (libc OK). */
		data = (uint8_t *)malloc(op->len ? op->len : 1u);
		if (data == NULL) {
			fail_set(f, op_index, fi, 0u, "malloc");
			return;
		}
		for (i = 0u; i < op->len; i++) {
			data[i] = pat_byte(i, op->pat);
		}
		rc = fat12_write_partial(&r->vol, r->fat, r->fat_len, r->slot[fi],
		                         op->offset, data, op->len, r->sector,
		                         r->cluster, &out_written);
		if (rc == FAT12_ERR_NO_SPACE) {
			/* Disk full: allocate-then-commit rollback -- nothing committed.
			 * The model MUST NOT apply the write (Rule 2). */
			if (out_written != 0u) {
				fail_set(f, op_index, fi, 0u, "NO_SPACE but out_written != 0");
			}
			free(data);
			/* The file is unchanged: still verify it reads back as the model. */
			check_file_inproc(r, fi, op_index, f);
			return;
		}
		if (rc != FAT12_OK) {
			fail_set(f, op_index, fi, 0u, "write_partial unexpected error");
			free(data);
			return;
		}
		/* len == 0 is a TRUE no-op in the artifact (fat12_write_partial returns
		 * OK with *out_written == 0 and size UNCHANGED -- it neither extends nor
		 * zero-fills, fat12.h). The model MUST match: no hole-fill, no growth. A
		 * len==0 write therefore commits out_written == 0, not len. */
		if (op->len == 0u) {
			if (out_written != 0u) {
				fail_set(f, op_index, fi, 0u, "len==0 write but out_written != 0");
			}
			free(data);
			check_file_inproc(r, fi, op_index, f);
			break;
		}
		if (out_written != op->len) {
			fail_set(f, op_index, fi, 0u, "out_written != len");
			free(data);
			return;
		}
		/* Mirror into the model: zero-fill the hole, splice the bytes. */
		{
			model_file_t *m = &r->model[fi];
			uint32_t end = op->offset + op->len;
			if (op->offset > m->size) {
				for (i = m->size; i < op->offset; i++) {
					m->bytes[i] = 0u;
				}
			}
			for (i = 0u; i < op->len; i++) {
				m->bytes[op->offset + i] = data[i];
			}
			if (end > m->size) {
				m->size = end;
			}
		}
		free(data);
		check_file_inproc(r, fi, op_index, f);
		break;
	}
	case OP_READ_PARTIAL: {
		dir_entry_t e;
		uint32_t    got = 0xFFFFFFFFu;
		uint32_t    want;
		uint32_t    sz;
		int         rc;
		if (!r->live[fi]) {
			return;
		}
		if (fat12_read_fat(&r->vol, r->fat, sizeof(r->fat)) != FAT12_OK ||
		    fat12_find(&r->vol, r->sector, NAME_POOL[fi], &e) != FAT12_OK) {
			fail_set(f, op_index, fi, 0u, "read_partial: find live file");
			return;
		}
		sz   = r->model[fi].size;
		want = (op->offset >= sz) ? 0u
		     : ((op->len < sz - op->offset) ? op->len : sz - op->offset);
		rc = fat12_read_partial(&r->vol, r->fat, r->fat_len, &e, op->offset,
		                        op->len, g_rbuf, r->cluster, &got);
		if (rc != FAT12_OK || got != want) {
			fail_set(f, op_index, fi, op->offset, "read_partial rc/count (op)");
			return;
		}
		if (got > 0u) {
			uint32_t d = first_diff(g_rbuf, r->model[fi].bytes + op->offset, got);
			if (d != got) {
				fail_set(f, op_index, fi, op->offset + d,
				         "read_partial bytes != model (op)");
				return;
			}
		}
		break;
	}
	case OP_READ_FILE: {
		if (!r->live[fi]) {
			return;
		}
		check_file_inproc(r, fi, op_index, f);   /* whole-file == model */
		break;
	}
	case OP_UNLINK: {
		int rc;
		if (!r->live[fi]) {
			return;
		}
		rc = fat12_unlink(&r->vol, r->fat, r->fat_len, NAME_POOL[fi], r->sector);
		if (rc != FAT12_OK) {
			fail_set(f, op_index, fi, 0u, "unlink unexpected error");
			return;
		}
		r->live[fi]         = 0;
		r->model[fi].exists = 0;
		r->model[fi].size   = 0u;
		/* Re-read FAT so a later create/find sees the freed clusters. */
		(void)fat12_read_fat(&r->vol, r->fat, sizeof(r->fat));
		break;
	}
	default:
		break;
	}
}

/* ---- Mint a fresh blank image: copy the template to img_path. -------------- */
static int mint_fresh(const char *template_img, const char *img_path)
{
	char cmd[1024];
	snprintf(cmd, sizeof(cmd), "cp -f '%s' '%s'", template_img, img_path);
	return run_command(cmd);
}

/* Pick a random offset that deliberately hits interesting positions for the
 * given current size: 0, mid-cluster, cluster boundary, past-EOF (hole), and
 * spanning many clusters. */
static uint32_t pick_offset(prng_t *p, uint32_t cur_size)
{
	uint32_t choice = prng_below(p, 8u);
	switch (choice) {
	case 0: return 0u;
	case 1: return BPC;                                /* cluster boundary    */
	case 2: return (cur_size > 3u) ? (cur_size - 3u) : 0u;  /* near current EOF */
	case 3: return cur_size + (uint32_t)(prng_below(p, 1200u)); /* hole/append */
	case 4: return BPC - 5u;                           /* straddle boundary   */
	case 5: return prng_below(p, 4000u);               /* anywhere in range   */
	case 6: return (cur_size / BPC) * BPC;             /* aligned to a cluster */
	default:return prng_below(p, 64u) * BPC;           /* many clusters in     */
	}
}

/* Pick a random length: mostly small (sub-cluster), sometimes multi-cluster. */
static uint32_t pick_len(prng_t *p)
{
	uint32_t choice = prng_below(p, 8u);
	switch (choice) {
	case 0: return 0u;                       /* zero-length (no-op write)     */
	case 1: return 1u;                       /* single byte                   */
	case 2: return prng_below(p, BPC);       /* sub-cluster                   */
	case 3: return BPC;                      /* exactly one cluster           */
	case 4: return BPC + prng_below(p, 8u);  /* just over a cluster           */
	case 5: return prng_below(p, 2048u);     /* a few clusters                */
	case 6: return prng_below(p, 6000u);     /* many clusters                 */
	default:return prng_below(p, 300u);      /* small-ish                     */
	}
}

/* Clamp a (offset,len) so the resulting end stays within FILE_CAP (the model
 * vector bound). A too-large request is simply trimmed (still exercises the
 * extend path up to the cap). */
static void clamp_range(uint32_t *offset, uint32_t *len)
{
	if (*offset > FILE_CAP) {
		*offset = FILE_CAP;
	}
	if (*offset + *len > FILE_CAP) {
		*len = FILE_CAP - *offset;
	}
}

/* ---- Generate the random op sequence for a seed (deterministic). ----------- *
 * Drawn so files are created, written across interesting offsets/lens (the
 * multi-tenant interleave happens naturally: several files stay live and ops
 * round-robin across them), read back, unlinked and recreated. */
static uint32_t gen_ops(uint64_t seed, uint32_t k, op_t *ops,
                        uint32_t cur_size_hint[MAX_FILES])
{
	prng_t   p;
	uint32_t i;
	uint32_t n = 0u;

	prng_seed(&p, seed);
	for (i = 0u; i < MAX_FILES; i++) {
		cur_size_hint[i] = 0u;
	}

	/* Always start by creating a couple of files so early writes have targets. */
	for (i = 0u; i < 2u && n < k; i++) {
		ops[n].kind = OP_CREATE;
		ops[n].fi   = i;
		ops[n].offset = ops[n].len = ops[n].pat = 0u;
		cur_size_hint[i] = 0u;
		n++;
	}

	while (n < k) {
		uint32_t fi   = prng_below(&p, MAX_FILES);
		uint32_t kind = prng_below(&p, 10u);  /* weighted toward WRITE */
		op_t    *o    = &ops[n];

		o->fi     = fi;
		o->offset = 0u;
		o->len    = 0u;
		o->pat    = (uint32_t)(prng_next(&p) & 0xFFFFu);

		if (kind <= 4u) {                 /* ~50% writes (the load-bearing path) */
			uint32_t off = pick_offset(&p, cur_size_hint[fi]);
			uint32_t ln  = pick_len(&p);
			clamp_range(&off, &ln);
			o->kind   = OP_WRITE;
			o->offset = off;
			o->len    = ln;
			if (off + ln > cur_size_hint[fi]) {
				cur_size_hint[fi] = off + ln;   /* track model size for offsets */
			}
		} else if (kind <= 6u) {          /* ~20% positioned reads */
			uint32_t off = pick_offset(&p, cur_size_hint[fi]);
			uint32_t ln  = pick_len(&p);
			clamp_range(&off, &ln);
			o->kind   = OP_READ_PARTIAL;
			o->offset = off;
			o->len    = ln;
		} else if (kind == 7u) {          /* ~10% whole-file reads */
			o->kind = OP_READ_FILE;
		} else if (kind == 8u) {          /* ~10% create (truncate/reuse) */
			o->kind = OP_CREATE;
			cur_size_hint[fi] = 0u;
		} else {                          /* ~10% unlink */
			o->kind = OP_UNLINK;
			cur_size_hint[fi] = 0u;
		}
		n++;
	}
	return n;
}

/* ---- Replay an op sequence on a fresh image; return the first failure. ------ *
 * Mints a fresh blank image, mounts the REAL fat12.c over it, applies ops[0..n),
 * running the (a)/(b) checks each mutating op and (c) external checks at the END
 * (if do_external) for every live file. *out_fail receives the first divergence.
 * Returns 0 if clean, -1 on a divergence (out_fail->failed == 1). */
static int replay(const char *template_img, const char *img_path,
                  const op_t *ops, uint32_t n, int do_external, fail_t *out_fail)
{
	static run_t r;   /* large (per-file 64 KiB models) -> static, not on stack */
	uint32_t     i;

	memset(&r, 0, sizeof(r));
	out_fail->failed = 0;
	r.img_path = img_path;

	if (mint_fresh(template_img, img_path) != 0) {
		snprintf(out_fail->where, sizeof(out_fail->where), "mint fresh image");
		out_fail->failed = 1;
		return -1;
	}
	if (blockdev_file_open_rw(&r.bf, img_path) != 0) {
		snprintf(out_fail->where, sizeof(out_fail->where), "open_rw");
		out_fail->failed = 1;
		return -1;
	}
	if (fat12_mount(&r.vol, &r.bf.dev, r.sector) != FAT12_OK) {
		blockdev_file_close(&r.bf);
		snprintf(out_fail->where, sizeof(out_fail->where), "mount");
		out_fail->failed = 1;
		return -1;
	}
	if (fat12_read_fat(&r.vol, r.fat, sizeof(r.fat)) != FAT12_OK) {
		blockdev_file_close(&r.bf);
		snprintf(out_fail->where, sizeof(out_fail->where), "initial read_fat");
		out_fail->failed = 1;
		return -1;
	}
	r.fat_len = (uint32_t)r.vol.bpb.sectors_per_fat *
	            (uint32_t)r.vol.bpb.bytes_per_sector;

	for (i = 0u; i < n && !out_fail->failed; i++) {
		apply_op(&r, &ops[i], (int)i, out_fail);
	}

	/* Structural FAT-redundancy check at run-end (cheap, every seed): both on-disk
	 * FAT copies must agree (DEC-07). Catches a writer that syncs only FAT #1 even
	 * though a content read-back from FAT #1 alone would pass. */
	if (!out_fail->failed) {
		check_fat_sync(&r, (int)(n > 0u ? (int)n - 1 : 0), out_fail);
	}

	/* External (c) check on the FINAL live set (and only if no earlier failure
	 * -- a clean run end). The blockdev's fwrite path fflushes each write, so the
	 * on-disk image is current for mtools/python. */
	if (!out_fail->failed && do_external) {
		check_external(&r, (int)(n > 0u ? (int)n - 1 : 0), out_fail);
	}

	blockdev_file_close(&r.bf);
	return out_fail->failed ? -1 : 0;
}

/* ---- Shrinker: minimize a failing op list to a small reproducer. ----------- *
 * Greedy delta-debugging-lite: (1) drop each op if the shortened list still
 * fails; (2) shrink each WRITE's len then offset toward 0 while it still fails.
 * Determinism (Rule 11) guarantees a deterministic reproducer. `do_external`
 * matches the run that failed so the same divergence is reproduced. */
static const char *op_name(int k)
{
	switch (k) {
	case OP_CREATE:       return "CREATE";
	case OP_WRITE:        return "WRITE";
	case OP_READ_PARTIAL: return "READ_PARTIAL";
	case OP_READ_FILE:    return "READ_FILE";
	case OP_UNLINK:       return "UNLINK";
	default:              return "?";
	}
}

static void print_recipe(const op_t *ops, uint32_t n, uint64_t seed,
                         const fail_t *f)
{
	uint32_t i;
	printf("---- SHRUNK REPRODUCER (seed=%llu) ----\n",
	       (unsigned long long)seed);
	printf("    diverged at op #%d  file=%s  off=%u  check=\"%s\"\n",
	       f->op_index, NAME_POOL[f->fi % MAX_FILES], f->diverge_off, f->where);
	printf("    %u op(s):\n", n);
	for (i = 0u; i < n; i++) {
		const op_t *o = &ops[i];
		if (o->kind == OP_WRITE || o->kind == OP_READ_PARTIAL) {
			printf("      [%u] %-12s %-8s off=%u len=%u pat=%u\n",
			       i, op_name(o->kind), NAME_POOL[o->fi], o->offset, o->len,
			       o->pat);
		} else {
			printf("      [%u] %-12s %-8s\n",
			       i, op_name(o->kind), NAME_POOL[o->fi]);
		}
	}
	printf("--------------------------------------\n");
}

static int still_fails(const char *template_img, const char *img_path,
                       const op_t *ops, uint32_t n, int do_external,
                       fail_t *out_fail)
{
	return replay(template_img, img_path, ops, n, do_external, out_fail) != 0;
}

static uint32_t shrink(const char *template_img, const char *img_path,
                       op_t *ops, uint32_t n, int do_external, fail_t *out_fail)
{
	op_t   *tmp = (op_t *)malloc(sizeof(op_t) * (n ? n : 1u));
	int     changed = 1;

	if (tmp == NULL) {
		return n;
	}

	/* (1) Greedily drop ops while the failure persists. */
	while (changed) {
		uint32_t i;
		changed = 0;
		for (i = 0u; i < n; ) {
			uint32_t j;
			uint32_t m = 0u;
			fail_t   ff;
			for (j = 0u; j < n; j++) {
				if (j != i) {
					tmp[m++] = ops[j];
				}
			}
			if (m < n && still_fails(template_img, img_path, tmp, m, do_external,
			                         &ff)) {
				memcpy(ops, tmp, sizeof(op_t) * m);
				n = m;
				*out_fail = ff;
				changed = 1;
				/* do not advance i: the list shifted under us */
			} else {
				i++;
			}
		}
	}

	/* (2) Minimize each WRITE's len, then offset, toward 0. */
	{
		uint32_t i;
		for (i = 0u; i < n; i++) {
			if (ops[i].kind != OP_WRITE) {
				continue;
			}
			/* Shrink len by halving while it still fails. */
			while (ops[i].len > 1u) {
				op_t   save = ops[i];
				fail_t ff;
				ops[i].len = ops[i].len / 2u;
				if (!still_fails(template_img, img_path, ops, n, do_external,
				                 &ff)) {
					ops[i] = save;
					break;
				}
				*out_fail = ff;
			}
			/* Shrink offset by halving while it still fails. */
			while (ops[i].offset > 0u) {
				op_t   save = ops[i];
				fail_t ff;
				ops[i].offset = ops[i].offset / 2u;
				if (!still_fails(template_img, img_path, ops, n, do_external,
				                 &ff)) {
					ops[i] = save;
					break;
				}
				*out_fail = ff;
			}
		}
	}

	free(tmp);
	return n;
}

/* ---- Run one seed end-to-end. Returns 0 if clean, -1 on a (shrunk) failure. */
static int run_seed(const char *template_img, const char *img_path,
                    uint64_t seed, uint32_t k, int do_external,
                    uint32_t *out_ops_done)
{
	static op_t ops[MAX_OPS];
	uint32_t    hint[MAX_FILES];
	uint32_t    n;
	fail_t      f;

	if (k > MAX_OPS) {
		k = MAX_OPS;
	}
	n = gen_ops(seed, k, ops, hint);
	*out_ops_done = n;

	if (replay(template_img, img_path, ops, n, do_external, &f) == 0) {
		return 0;   /* clean: every op agreed across model + remount (+external) */
	}

	/* Failure: shrink to a minimal reproducer and print the recipe (Law 2). */
	printf("\n!!! fat12_fuzz: DIVERGENCE on seed %llu (op #%d, %s) -- shrinking...\n",
	       (unsigned long long)seed, f.op_index, f.where);
	{
		uint32_t shrunk_n = shrink(template_img, img_path, ops, n, do_external, &f);
		print_recipe(ops, shrunk_n, seed, &f);
	}
	return -1;
}

static void usage(const char *argv0)
{
	fprintf(stderr,
	    "usage: %s --seed N [--ops K] [--no-external] <blank-fat12-image>\n"
	    "       %s --sweep A B [--ops K] [--no-external] <blank-fat12-image>\n",
	    argv0, argv0);
}

int main(int argc, char **argv)
{
	const char *template_img = NULL;
	uint64_t    seed_lo = 1, seed_hi = 1;
	uint32_t    k = 40u;
	int         do_external = 1;
	int         sweep = 0;
	int         i;
	char        img_path[512];
	uint64_t    s;
	uint64_t    total_ops = 0u;
	uint32_t    seeds_run = 0u;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
			seed_lo = seed_hi = strtoull(argv[++i], NULL, 0);
		} else if (strcmp(argv[i], "--sweep") == 0 && i + 2 < argc) {
			sweep   = 1;
			seed_lo = strtoull(argv[++i], NULL, 0);
			seed_hi = strtoull(argv[++i], NULL, 0);
		} else if (strcmp(argv[i], "--ops") == 0 && i + 1 < argc) {
			k = (uint32_t)strtoul(argv[++i], NULL, 0);
		} else if (strcmp(argv[i], "--no-external") == 0) {
			do_external = 0;
		} else if (argv[i][0] != '-') {
			template_img = argv[i];
		} else {
			usage(argv[0]);
			return 2;
		}
	}

	if (template_img == NULL) {
		usage(argv[0]);
		return 2;
	}

	(void)sweep;
	for (s = seed_lo; s <= seed_hi; s++) {
		uint32_t ops_done = 0u;
		/* Per-seed scratch image so a failing seed's image survives for forensics
		 * yet successive seeds do not collide. Deterministic name (no clock). */
		snprintf(img_path, sizeof(img_path), "%s.seed%llu.scratch",
		         template_img, (unsigned long long)s);
		if (run_seed(template_img, img_path, s, k, do_external, &ops_done) != 0) {
			fprintf(stderr,
			    "fat12_fuzz: FAILED on seed %llu (see shrunk reproducer above).\n",
			    (unsigned long long)s);
			/* Leave the scratch image in place for inspection; remove siblings. */
			return 1;
		}
		remove(img_path);
		total_ops += ops_done;
		seeds_run++;
	}

	printf("fat12_fuzz: ALL GREEN -- %u seed(s) [%llu..%llu], %llu ops exercised,"
	       " model + remount%s agreement on every op.\n",
	       seeds_run, (unsigned long long)seed_lo, (unsigned long long)seed_hi,
	       (unsigned long long)total_ops, do_external ? " + mtools/python" : "");
	return 0;
}
