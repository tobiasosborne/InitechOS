/*
 * harness/diff/fat_diff/fat12_corrupt_fuzz.c -- deterministic, seeded CORRUPTION
 * fuzzer for the FAT12 read/walk layer (os/milton/fat12.c).
 *
 * FACTORY host tool (CLAUDE.md Law 3): libc OK; the SUBJECT under test is the
 * UNMODIFIED artifact fat12.c, driven through the host file-backed blockdev
 * (blockdev_file.c) and against in-memory synthetic FAT buffers / dir entries.
 * NOTHING about the artifact is touched here.
 *
 * Why (beads initech-dnn; malformed-BPB part of initech-9xl): the generative
 * fuzzer (fat12_fuzz.c) explores the HAPPY-path state space against a model.
 * This one is its adversarial twin: it DELIBERATELY builds MALFORMED images and
 * asserts the kernel's fat12 layer (Rule 2, "fail fast, fail loud"):
 *   never HANGS  (the max_steps / max_clusters anti-hang bounds always trip);
 *   never reads/writes OUT OF BOUNDS (the blockdev backend bounds-checks every
 *     read -- a cluster LBA past end-of-image is a short fread -> FAT12_ERR_READ,
 *     never garbage; fat12_next_cluster bounds-checks off+1 vs fat_len);
 *   FAILS LOUD with the DOCUMENTED error return (FAT12_ERR_CHAIN / _CLUSTER /
 *     _READ for a corrupt chain; FAT12_ERR_GEOMETRY for a malformed BPB).
 *
 * Corruption families exercised (each iteration draws + asserts):
 *   1. CLUSTER LOOP-CHAINS    -- a FAT entry pointing back into the chain
 *      (self-cycle N->N, or a 2..k cycle). The walk MUST TERMINATE (the
 *      max_steps / max_clusters bound), returning FAT12_ERR_CHAIN -- not hang,
 *      not loop forever.
 *   2. RESERVED / BAD markers -- 0xFF0..0xFF6 (reserved) or 0xFF7 (bad)
 *      encountered MID-CHAIN. 0xFF7 (bad) is corruption -> FAT12_ERR_CHAIN.
 *      0xFF0..0xFF6 decode as ordinary "next cluster" values (the artifact's
 *      fat12_is_eoc threshold is >= 0xFF8); they are >= total_clusters+2, so the
 *      next step is an out-of-FAT decode -> FAT12_ERR_CLUSTER (still fail-loud,
 *      still bounded -- the property we assert is "errors, does not hang/OOB").
 *   3. TRUNCATED chain        -- a dir entry's start_cluster (or a mid-chain
 *      link) >= total_clusters+2, or a chain shorter than file_size demands.
 *      Out-of-range -> FAT12_ERR_CLUSTER; a chain too short -> FAT12_ERR_CHAIN.
 *   4. RESERVED start_cluster -- start_cluster = 0 or 1 in a dir entry with a
 *      NON-zero file_size. fat12_read_file / fat12_read_partial / fat12_walk_chain
 *      must reject the reserved cluster -> FAT12_ERR_CLUSTER (never treat 0/1 as
 *      data clusters).
 *   5. MALFORMED BPB          -- sectors_per_cluster=0, total_sectors=0,
 *      first_data_sector>=total_sectors (and num_fats=0 / sectors_per_fat=0 /
 *      root_entry_count=0). fat12_mount MUST return FAT12_ERR_GEOMETRY (the real
 *      guards at fat12.c:80 / :92 / :105). This is the initech-9xl malformed-BPB
 *      coverage -- the guards' actual documented return is asserted.
 *
 * Determinism (Rule 11): a splitmix64-seeded xorshift PRNG; NO time()/rand()/
 * clock. Same --seed => identical corruption draw => identical result, so a
 * failure REPLAYS exactly by seed. The gate runs a fixed seed sweep.
 *
 * Bounded (no hang): every fat12 read/walk entry point the fuzzer drives has an
 * internal max_steps / max_clusters bound (fat12.c walk_chain:257,
 * read_partial:707/730/792, free_chain:977). The fuzzer NEVER loops in its OWN
 * driver code (each iteration is a fixed-step sequence). The Makefile mutant
 * gate additionally runs the build under a wall-clock `timeout` as the hard
 * backstop, so a MUTANT that removes a max_steps guard (-> the artifact loops
 * forever on a self-cycle) is caught as a timeout -- the bound tripped.
 *
 * Mutants (Rule 6; built from fat12.c's own #ifdefs, never in a real build):
 *   -DFAT12_MUTATE_NO_STEP_GUARD : removes the max_steps guard in
 *     fat12_read_partial -> a self-cycle loop-chain HANGS. The fuzzer's
 *     loop-chain leg never returns; the Makefile `timeout` trips -> gate RED.
 *   -DFAT12_MUTATE_ACCEPT_BAD_BPB : removes the sectors_per_cluster==0 geometry
 *     guard at mount -> a malformed BPB is ACCEPTED (mount returns OK instead of
 *     FAT12_ERR_GEOMETRY). The fuzzer's BPB leg asserts FAT12_ERR_GEOMETRY -> RED.
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 2 (geometry), Sec 3
 *   (12-bit decode, free=0x000, reserved 0xFF0..0xFF6, bad 0xFF7, EOC >= 0xFF8);
 *   spec/dos_structs.h (bpb_t field offsets, dir_entry_t); fat12.c guard sites
 *   cited inline. CLAUDE.md Rule 2 (fail fast, fail loud) / Rule 3 (all bugs are
 *   deep). The blank-image BPB (mformat -f 1440): bytes_per_sector 512,
 *   sectors_per_cluster 1, reserved 1, num_fats 2, root_entry_count 224,
 *   total_sectors 2880, sectors_per_fat 9 -> first_data_sector 33,
 *   total_clusters 2847 (data clusters 2..2848).
 *
 * Exit code: 0 if every seed's every corruption was handled fail-loud + bounded;
 * non-zero on the FIRST violation (printing the seed + family + diverging check)
 * so the Makefile gate goes RED (Law 2).
 *
 * ASCII-clean (Rule 12).
 *
 * Usage:
 *   fat12_corrupt_fuzz --seed N [--ops K] <blank-fat12-image-template>
 *   fat12_corrupt_fuzz --sweep A B [--ops K] <blank-fat12-image-template>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "fat12.h"
#include "blockdev_file.h"

/* ---- Geometry of the 1.44 MB blank template (mformat -f 1440). ------------- */
#define FAT_BYTES      (9u * 512u)   /* sectors_per_fat(9) * bytes_per_sector    */
#define CHAIN_CAP      2880u         /* >= total_clusters; walk_chain array cap  */

/* ---- BPB field byte offsets in the boot sector (spec/dos_structs.h Sec). ---- */
#define BPB_OFF_SEC_PER_CLUSTER 0x0Du   /* uint8                                 */
#define BPB_OFF_NUM_FATS        0x10u   /* uint8                                 */
#define BPB_OFF_ROOT_ENTRIES    0x11u   /* uint16 LE                             */
#define BPB_OFF_TOTAL_SEC_16    0x13u   /* uint16 LE                             */
#define BPB_OFF_SEC_PER_FAT     0x16u   /* uint16 LE                             */
#define BPB_OFF_TOTAL_SEC_32    0x20u   /* uint32 LE                             */

/* ---- Deterministic PRNG: splitmix64 seed -> xorshift128+ stream (Rule 11). -- *
 * Byte-identical to fat12_fuzz.c so the two fuzzers share replay semantics. */
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

/* ---- Failure reporting (first divergence wins). ---------------------------- */
typedef struct {
	int  failed;
	int  op_index;        /* iteration index within the seed            */
	char family[24];      /* corruption family tag                      */
	char where[112];      /* the failing assertion                      */
} fail_t;

static void fail_set(fail_t *f, int op_index, const char *family,
                     const char *where)
{
	if (f->failed) {
		return;   /* keep the FIRST failure */
	}
	f->failed   = 1;
	f->op_index = op_index;
	snprintf(f->family, sizeof(f->family), "%s", family);
	snprintf(f->where, sizeof(f->where), "%s", where);
}

/* ---- 12-bit FAT entry setter (the EXACT inverse of fat12_next_cluster's
 * decode -- a private copy so the fuzzer can LAY corrupt chains independent of
 * the artifact's fat12_set_entry, which itself rejects cluster < 2). Mirrors
 * the even/odd packing in fat12.c and the worked example in test_fat12_chain.c. */
static void fat_poke(uint8_t *fat, uint16_t cluster, uint16_t value)
{
	uint32_t off = ((uint32_t)cluster * 3u) / 2u;
	uint16_t v   = (uint16_t)(value & 0x0FFFu);
	if (off + 1u >= FAT_BYTES) {
		return;   /* out of our scratch FAT; the artifact will catch the index */
	}
	if ((cluster & 1u) == 0u) {
		fat[off]      = (uint8_t)(v & 0xFFu);
		fat[off + 1u] = (uint8_t)((fat[off + 1u] & 0xF0u) | ((v >> 8) & 0x0Fu));
	} else {
		fat[off]      = (uint8_t)((fat[off] & 0x0Fu) | ((v << 4) & 0xF0u));
		fat[off + 1u] = (uint8_t)((v >> 4) & 0xFFu);
	}
}

/* Build a minimal valid dir entry for a synthetic file of `size` bytes whose
 * chain starts at `start_cluster`. mtime/mdate/attr deterministic (Rule 11). */
static void make_dirent(dir_entry_t *e, uint16_t start_cluster, uint32_t size)
{
	uint32_t k;
	for (k = 0u; k < sizeof(*e); k++) {
		((uint8_t *)e)[k] = 0u;
	}
	for (k = 0u; k < 8u; k++) {
		e->filename[k] = (uint8_t)"FUZZFILE"[k];
	}
	e->extension[0] = (uint8_t)'B';
	e->extension[1] = (uint8_t)'I';
	e->extension[2] = (uint8_t)'N';
	e->attribute     = DIR_ATTR_ARCHIVE;
	e->start_cluster = start_cluster;
	e->file_size     = size;
}

/* Scratch buffers shared per iteration (static: large, off the stack). The
 * read_partial output buffer is sized ABOVE total_clusters * bytes_per_cluster
 * (2847 * 512 = ~1.46 MB) so the loop-chain leg can request a range that PROVABLY
 * exceeds any finite traversal of a small cycle -- forcing the read_partial
 * max_steps guard to trip (error) rather than reading duplicate cluster data. */
#define OUT_CAP        (2u * 1024u * 1024u)   /* 2 MB > total_clusters*512        */
static uint8_t  g_fat[FAT_BYTES];
static uint16_t g_chain[CHAIN_CAP];
static uint8_t  g_out[OUT_CAP];
static uint8_t  g_cluster[512];

/* ---- Family 1: cluster loop-chains MUST terminate (bounded), never hang. ---- *
 * Lay a chain head -> head+1 -> ... and then point the last cluster BACK into
 * the chain (self-cycle, or back to the head). fat12_walk_chain AND
 * fat12_read_partial must return an error (not loop). The blockdev only matters
 * for read_partial's cluster reads; a cycle trips the step bound either way. */
static void fam_loop_chain(prng_t *p, fat12_volume_t *vol, int idx, fail_t *f)
{
	uint16_t head = (uint16_t)(2u + prng_below(p, 8u));          /* 2..9        */
	uint16_t span = (uint16_t)(1u + prng_below(p, 6u));          /* 1..6 links  */
	uint16_t j;
	uint32_t count = 0xFFFFFFFFu;
	dir_entry_t e;
	uint32_t got = 0xFFFFFFFFu;
	int rc;

	memset(g_fat, 0, sizeof(g_fat));

	/* Forward run head, head+1, ..., head+span-1. */
	for (j = 0u; j + 1u < span; j++) {
		fat_poke(g_fat, (uint16_t)(head + j), (uint16_t)(head + j + 1u));
	}
	/* Close the loop: last cluster points back into the chain (head) or to
	 * itself -- a cycle either way. prng picks which to vary the shape. */
	{
		uint16_t last = (uint16_t)(head + span - 1u);
		uint16_t back = (prng_below(p, 2u) == 0u) ? head : last; /* head|self  */
		fat_poke(g_fat, last, back);
	}

	/* (1a) walk_chain MUST terminate with FAT12_ERR_CHAIN (the max_clusters
	 * bound is the anti-hang guard). g_chain is sized to the volume's cluster
	 * count, so a cycle overruns it -> bounded error, NO OOB write. */
	rc = fat12_walk_chain(vol, g_fat, sizeof(g_fat), head, g_chain,
	                      (uint32_t)(sizeof(g_chain) / sizeof(g_chain[0])),
	                      &count);
	if (rc != FAT12_ERR_CHAIN) {
		fail_set(f, idx, "loop-chain",
		         "walk_chain on a cyclic chain must return FAT12_ERR_CHAIN (anti-hang)");
		return;
	}

	/* (1b) read_file walks the WHOLE chain to EOC (via walk_chain), so a cyclic
	 * chain MUST fail loud with FAT12_ERR_CHAIN -- never hang, never OOB. The
	 * file_size claims one cluster of data; the cycle is hit walking the chain. */
	make_dirent(&e, head, 512u);
	rc = fat12_read_file(vol, g_fat, sizeof(g_fat), &e, g_out, sizeof(g_out),
	                     g_cluster, &got);
	if (rc != FAT12_ERR_CHAIN) {
		fail_set(f, idx, "loop-chain",
		         "read_file on a cyclic chain must return FAT12_ERR_CHAIN (anti-hang)");
		return;
	}

	/* (1c) read_partial of a range that PROVABLY exceeds the cycle: file_size =
	 * OUT_CAP (2 MB = 4096 clusters) > total_clusters (2847), but the chain
	 * cycles among <= 6 clusters. The incremental copy walk must advance through
	 * the cycle more than max_steps (total_clusters+2) times before satisfying
	 * the request, so the read_partial max_steps guard MUST trip -> a fail-loud
	 * error, never reading endless duplicate-cluster data, never OOB. (A SMALL
	 * positioned read of a self-cycle is legitimately satisfiable by re-reading
	 * the same cluster and is NOT corruption -- hence the deliberately oversized
	 * range here.) Removing the step guard (the mutant) makes this return OK. */
	make_dirent(&e, head, OUT_CAP);
	rc = fat12_read_partial(vol, g_fat, sizeof(g_fat), &e,
	                        0u, OUT_CAP, g_out, g_cluster, &got);
	if (rc == FAT12_OK) {
		fail_set(f, idx, "loop-chain",
		         "read_partial of a range exceeding a cyclic chain must fail loud (not OK)");
		return;
	}
	/* Any negative fail-loud code is acceptable (the property is bounded +
	 * loud): _CHAIN (cycle / step bound), _CLUSTER (out-of-FAT decode), or _READ
	 * (a cluster LBA past EOF caught by the bounded blockdev). NEVER a silent OK. */
}

/* ---- Family 2: reserved (0xFF0..0xFF6) / bad (0xFF7) markers mid-chain. ----- *
 * 0xFF7 (bad) is corruption the walk rejects directly (FAT12_ERR_CHAIN).
 * 0xFF0..0xFF6 are < EOC (0xFF8): the decoder returns them as "next cluster",
 * but they are >= total_clusters+2, so the FOLLOWING decode is out-of-FAT ->
 * FAT12_ERR_CLUSTER. Either way: fail-loud + bounded, never a hang/OOB. */
static void fam_reserved_marker(prng_t *p, fat12_volume_t *vol, int idx,
                                fail_t *f)
{
	uint16_t head = (uint16_t)(2u + prng_below(p, 8u));
	/* Pick a marker in [0xFF0, 0xFF7]: 0xFF0..0xFF6 reserved, 0xFF7 bad. */
	uint16_t marker = (uint16_t)(0xFF0u + prng_below(p, 8u));     /* FF0..FF7 */
	uint32_t count = 0xFFFFFFFFu;
	int rc;

	memset(g_fat, 0, sizeof(g_fat));
	/* head -> head+1 -> marker (the reserved/bad value sits at head+1). */
	fat_poke(g_fat, head, (uint16_t)(head + 1u));
	fat_poke(g_fat, (uint16_t)(head + 1u), marker);

	rc = fat12_walk_chain(vol, g_fat, sizeof(g_fat), head, g_chain,
	                      (uint32_t)(sizeof(g_chain) / sizeof(g_chain[0])),
	                      &count);
	/* MUST fail loud. 0xFF7 -> FAT12_ERR_CHAIN (is_bad caught on the next visit).
	 * 0xFF0..0xFF6 -> the walk visits the marker as a cluster index, whose own
	 * decode is out-of-FAT -> FAT12_ERR_CLUSTER. Both are documented fail-loud
	 * codes; the property is "NOT OK, NOT EOC-accepted, bounded". */
	if (rc == FAT12_OK) {
		fail_set(f, idx, "reserved-marker",
		         "walk_chain hitting a reserved/bad marker mid-chain must fail loud");
		return;
	}
	if (rc != FAT12_ERR_CHAIN && rc != FAT12_ERR_CLUSTER) {
		fail_set(f, idx, "reserved-marker",
		         "reserved/bad marker mid-chain: expected FAT12_ERR_CHAIN or _CLUSTER");
		return;
	}
}

/* ---- Family 3: truncated chain -- start_cluster / mid-link out of range. ---- *
 * total_clusters = 2847 -> data clusters 2..2848. A cluster >= 2849 (or a chain
 * shorter than file_size needs) is corruption. Drive both walk_chain and
 * read_file with a dir entry whose start or chain runs out. */
static void fam_truncated(prng_t *p, fat12_volume_t *vol, int idx, fail_t *f)
{
	uint16_t last_valid = (uint16_t)(FAT12_FIRST_DATA_CLUSTER +
	                                 vol->total_clusters - 1u);   /* 2848 */
	uint32_t out_b = 0xFFFFFFFFu;
	dir_entry_t e;
	int rc;
	uint32_t choice = prng_below(p, 3u);

	memset(g_fat, 0, sizeof(g_fat));

	if (choice == 0u) {
		/* start_cluster ABOVE the last valid data cluster (out of range). */
		uint16_t bad = (uint16_t)(last_valid + 1u +
		                          (uint16_t)prng_below(p, 64u));   /* >= 2849 */
		make_dirent(&e, bad, 512u);
		rc = fat12_read_file(vol, g_fat, sizeof(g_fat), &e, g_out,
		                     sizeof(g_out), g_cluster, &out_b);
		/* An out-of-FAT start cluster -> decode rejects it: _CLUSTER (off+1 >=
		 * fat_len) or _CHAIN. Must fail loud, never OK. */
		if (rc == FAT12_OK) {
			fail_set(f, idx, "truncated",
			         "read_file with out-of-range start_cluster must fail loud");
			return;
		}
		if (rc != FAT12_ERR_CLUSTER && rc != FAT12_ERR_CHAIN) {
			fail_set(f, idx, "truncated",
			         "out-of-range start_cluster: expected _CLUSTER or _CHAIN");
			return;
		}
	} else if (choice == 1u) {
		/* A chain that ENDS (EOC) before file_size demands: head -> EOC, but
		 * file_size claims 4 clusters. read_file must detect the short chain. */
		uint16_t head = (uint16_t)(2u + prng_below(p, 8u));
		fat_poke(g_fat, head, FAT12_EOC_VALUE);            /* 1-cluster chain  */
		make_dirent(&e, head, 4u * 512u);                  /* claims 4 clusters */
		rc = fat12_read_file(vol, g_fat, sizeof(g_fat), &e, g_out,
		                     sizeof(g_out), g_cluster, &out_b);
		if (rc != FAT12_ERR_CHAIN) {
			fail_set(f, idx, "truncated",
			         "read_file with a chain shorter than file_size must return _CHAIN");
			return;
		}
	} else {
		/* A mid-chain link jumps out of range: head -> bad(>last_valid). The
		 * incremental decode of `bad` is out-of-FAT -> _CLUSTER (or _CHAIN). */
		uint16_t head = (uint16_t)(2u + prng_below(p, 8u));
		uint16_t bad  = (uint16_t)(last_valid + 1u +
		                           (uint16_t)prng_below(p, 32u));
		uint32_t count = 0xFFFFFFFFu;
		fat_poke(g_fat, head, bad);
		rc = fat12_walk_chain(vol, g_fat, sizeof(g_fat), head, g_chain,
		                      (uint32_t)(sizeof(g_chain) / sizeof(g_chain[0])),
		                      &count);
		if (rc == FAT12_OK) {
			fail_set(f, idx, "truncated",
			         "walk_chain with an out-of-range mid-link must fail loud");
			return;
		}
		if (rc != FAT12_ERR_CLUSTER && rc != FAT12_ERR_CHAIN) {
			fail_set(f, idx, "truncated",
			         "out-of-range mid-link: expected _CLUSTER or _CHAIN");
			return;
		}
	}
}

/* ---- Family 4: reserved start_cluster (0 or 1) with a non-zero size. -------- *
 * Clusters 0/1 are reserved (FAT[0] media, FAT[1] EOC) -- never data. A dir
 * entry claiming size>0 from start_cluster 0/1 is corruption: read_file /
 * read_partial / walk_chain must reject it (FAT12_ERR_CLUSTER), never treat
 * 0/1 as a data cluster. (size==0 from start 0 is the LEGITIMATE empty-file
 * case and is NOT corruption -- we always pass size>0 here.) */
static void fam_reserved_start(prng_t *p, fat12_volume_t *vol, int idx, fail_t *f)
{
	uint16_t start = (uint16_t)prng_below(p, 2u);     /* 0 or 1 (reserved) */
	uint32_t size  = 512u + prng_below(p, 4096u);     /* non-zero */
	dir_entry_t e;
	uint32_t got = 0xFFFFFFFFu;
	uint32_t count = 0xFFFFFFFFu;
	int rc;

	memset(g_fat, 0, sizeof(g_fat));
	make_dirent(&e, start, size);

	/* read_file: reserved start with size>0 must be rejected. */
	rc = fat12_read_file(vol, g_fat, sizeof(g_fat), &e, g_out, sizeof(g_out),
	                     g_cluster, &got);
	if (rc != FAT12_ERR_CLUSTER && rc != FAT12_ERR_CHAIN) {
		fail_set(f, idx, "reserved-start",
		         "read_file with reserved start_cluster 0/1 (size>0) must return _CLUSTER/_CHAIN");
		return;
	}

	/* read_partial of a real range likewise must reject the reserved start. */
	rc = fat12_read_partial(vol, g_fat, sizeof(g_fat), &e, 0u, 32u, g_out,
	                        g_cluster, &got);
	if (rc != FAT12_ERR_CLUSTER && rc != FAT12_ERR_CHAIN) {
		fail_set(f, idx, "reserved-start",
		         "read_partial with reserved start_cluster 0/1 must return _CLUSTER/_CHAIN");
		return;
	}

	/* walk_chain directly: a reserved start cluster is rejected up front. */
	rc = fat12_walk_chain(vol, g_fat, sizeof(g_fat), start, g_chain,
	                      (uint32_t)(sizeof(g_chain) / sizeof(g_chain[0])),
	                      &count);
	if (rc != FAT12_ERR_CLUSTER) {
		fail_set(f, idx, "reserved-start",
		         "walk_chain with reserved start_cluster 0/1 must return _CLUSTER");
		return;
	}
}

/* ---- Family 5: malformed BPB -> fat12_mount MUST return FAT12_ERR_GEOMETRY. -- *
 * (initech-9xl malformed-BPB coverage.) Mint a fresh scratch image from the
 * blank template, corrupt one (or a combination of) BPB field(s) on disk, mount
 * the REAL fat12.c over it, and assert the documented geometry-guard return.
 * The boot signature (0xAA55) is left intact so we reach the geometry checks
 * (a bad signature returns FAT12_ERR_SIGNATURE earlier -- a different guard). */
static int write_bpb_byte(const char *img, uint32_t off, uint8_t val)
{
	FILE *fp = fopen(img, "r+b");
	if (fp == NULL) {
		return -1;
	}
	if (fseek(fp, (long)off, SEEK_SET) != 0 || fputc((int)val, fp) == EOF) {
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 0;
}

static int write_bpb_le16(const char *img, uint32_t off, uint16_t val)
{
	if (write_bpb_byte(img, off, (uint8_t)(val & 0xFFu)) != 0) {
		return -1;
	}
	return write_bpb_byte(img, off + 1u, (uint8_t)((val >> 8) & 0xFFu));
}

static void fam_bad_bpb(prng_t *p, const char *template_img,
                        const char *scratch_img, int idx, fail_t *f)
{
	uint32_t choice = prng_below(p, 6u);
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint8_t         sector_buf[512];
	char            cmd[1024];
	int             rc;

	/* Fresh copy of the blank template (so each corruption starts pristine). */
	snprintf(cmd, sizeof(cmd), "cp -f '%s' '%s'", template_img, scratch_img);
	if (system(cmd) != 0) {
		fail_set(f, idx, "bad-bpb", "could not mint a fresh scratch image");
		return;
	}

	switch (choice) {
	case 0:  /* sectors_per_cluster = 0 (fat12.c:80 guard). */
		(void)write_bpb_byte(scratch_img, BPB_OFF_SEC_PER_CLUSTER, 0u);
		break;
	case 1:  /* num_fats = 0 (fat12.c:81 guard). */
		(void)write_bpb_byte(scratch_img, BPB_OFF_NUM_FATS, 0u);
		break;
	case 2:  /* sectors_per_fat = 0 (fat12.c:82 guard). */
		(void)write_bpb_le16(scratch_img, BPB_OFF_SEC_PER_FAT, 0u);
		break;
	case 3:  /* root_entry_count = 0 (fat12.c:83 guard). */
		(void)write_bpb_le16(scratch_img, BPB_OFF_ROOT_ENTRIES, 0u);
		break;
	case 4:  /* total_sectors_16 = 0 AND total_sectors_32 = 0 (fat12.c:92). */
		(void)write_bpb_le16(scratch_img, BPB_OFF_TOTAL_SEC_16, 0u);
		(void)write_bpb_byte(scratch_img, BPB_OFF_TOTAL_SEC_32 + 0u, 0u);
		(void)write_bpb_byte(scratch_img, BPB_OFF_TOTAL_SEC_32 + 1u, 0u);
		(void)write_bpb_byte(scratch_img, BPB_OFF_TOTAL_SEC_32 + 2u, 0u);
		(void)write_bpb_byte(scratch_img, BPB_OFF_TOTAL_SEC_32 + 3u, 0u);
		break;
	default: /* total_sectors_16 = 1 -> first_data_sector(33) >= total (fat12.c:105). */
		(void)write_bpb_le16(scratch_img, BPB_OFF_TOTAL_SEC_16, 1u);
		break;
	}

	if (blockdev_file_open(&bf, scratch_img) != 0) {
		fail_set(f, idx, "bad-bpb", "could not open the corrupted scratch image");
		return;
	}
	rc = fat12_mount(&vol, &bf.dev, sector_buf);
	blockdev_file_close(&bf);

	/* THE assertion (initech-9xl): a malformed BPB MUST fail loud as
	 * FAT12_ERR_GEOMETRY -- the documented return of the mount guards at
	 * fat12.c:80 / :92 / :105. NOT FAT12_OK, NOT a different code. */
	if (rc != FAT12_ERR_GEOMETRY) {
		char msg[112];
		snprintf(msg, sizeof(msg),
		         "malformed BPB (case %u) must return FAT12_ERR_GEOMETRY, got rc=%d",
		         choice, rc);
		fail_set(f, idx, "bad-bpb", msg);
		return;
	}
}

/* ---- One seed: a fixed K-op corruption sequence, deterministic by seed. ----- *
 * Returns 0 if every op was handled fail-loud + bounded; -1 on the first
 * violation (out_fail->failed == 1). The volume is mounted ONCE over the pristine
 * blank template (valid geometry) and reused for the in-memory FAT/dir families;
 * the bad-BPB family mints + corrupts its own scratch copies. */
static int run_seed(const char *template_img, const char *scratch_img,
                    uint64_t seed, uint32_t k, fail_t *out_fail)
{
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint8_t         sector_buf[512];
	prng_t          p;
	uint32_t        i;

	out_fail->failed = 0;
	prng_seed(&p, seed);

	if (blockdev_file_open(&bf, template_img) != 0) {
		snprintf(out_fail->where, sizeof(out_fail->where), "open blank template");
		out_fail->failed = 1;
		return -1;
	}
	if (fat12_mount(&vol, &bf.dev, sector_buf) != FAT12_OK) {
		blockdev_file_close(&bf);
		snprintf(out_fail->where, sizeof(out_fail->where),
		         "mount of pristine blank template");
		out_fail->failed = 1;
		return -1;
	}

	for (i = 0u; i < k && !out_fail->failed; i++) {
		uint32_t fam = prng_below(&p, 5u);
		switch (fam) {
		case 0: fam_loop_chain(&p, &vol, (int)i, out_fail);       break;
		case 1: fam_reserved_marker(&p, &vol, (int)i, out_fail);  break;
		case 2: fam_truncated(&p, &vol, (int)i, out_fail);        break;
		case 3: fam_reserved_start(&p, &vol, (int)i, out_fail);   break;
		default: fam_bad_bpb(&p, template_img, scratch_img, (int)i, out_fail);
			break;
		}
	}

	blockdev_file_close(&bf);
	return out_fail->failed ? -1 : 0;
}

static void usage(const char *argv0)
{
	fprintf(stderr,
	    "usage: %s --seed N [--ops K] <blank-fat12-image>\n"
	    "       %s --sweep A B [--ops K] <blank-fat12-image>\n",
	    argv0, argv0);
}

int main(int argc, char **argv)
{
	const char *template_img = NULL;
	uint64_t    seed_lo = 1, seed_hi = 1;
	uint32_t    k = 40u;
	int         i;
	char        scratch_img[640];
	uint64_t    s;
	uint32_t    seeds_run = 0u;
	uint64_t    total_ops = 0u;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
			seed_lo = seed_hi = strtoull(argv[++i], NULL, 0);
		} else if (strcmp(argv[i], "--sweep") == 0 && i + 2 < argc) {
			seed_lo = strtoull(argv[++i], NULL, 0);
			seed_hi = strtoull(argv[++i], NULL, 0);
		} else if (strcmp(argv[i], "--ops") == 0 && i + 1 < argc) {
			k = (uint32_t)strtoul(argv[++i], NULL, 0);
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

	for (s = seed_lo; s <= seed_hi; s++) {
		fail_t f;
		/* Per-seed scratch image for the bad-BPB family. Deterministic name. */
		snprintf(scratch_img, sizeof(scratch_img), "%s.cseed%llu.scratch",
		         template_img, (unsigned long long)s);
		memset(&f, 0, sizeof(f));
		if (run_seed(template_img, scratch_img, s, k, &f) != 0) {
			fprintf(stderr,
			    "\n!!! fat12_corrupt_fuzz: VIOLATION on seed %llu (op #%d, %s): %s\n"
			    "    (replay: --seed %llu --ops %u)\n",
			    (unsigned long long)s, f.op_index, f.family, f.where,
			    (unsigned long long)s, k);
			remove(scratch_img);
			return 1;
		}
		remove(scratch_img);
		total_ops += k;
		seeds_run++;
	}

	printf("fat12_corrupt_fuzz: ALL GREEN -- %u seed(s) [%llu..%llu], %llu corruptions "
	       "exercised; every malformed input handled fail-loud + bounded (no hang, no OOB).\n",
	       seeds_run, (unsigned long long)seed_lo, (unsigned long long)seed_hi,
	       (unsigned long long)total_ops);
	return 0;
}
