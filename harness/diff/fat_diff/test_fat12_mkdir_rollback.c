/*
 * harness/diff/fat_diff/test_fat12_mkdir_rollback.c -- FAT12 MKDIR post-grow
 * ROLLBACK atomicity oracle (beads initech-m0bp rollback fix; adversarial finding).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (Law 2: the oracle is the truth, never false-green).
 *
 * THE DEFECT this gate pins (Rule 3, all-bugs-are-deep): fat12_mkdir grows the
 * SUBDIR parent by one cluster (fat12_grow_dir, parent_grew=1) BEFORE it claims
 * the NEW directory's own data cluster. If that subsequent claim fails -- the
 * commonest being NO_SPACE when the grow consumed the volume's LAST free cluster
 * -- the early `return FAT12_ERR_NO_SPACE` historically left the freshly-appended
 * parent cluster LINKED into the parent chain: orphaned, zero-filled, and
 * UNRECLAIMABLE (this FS only shrinks a dir via fat12_shrink_dir_tail, which the
 * early return never called). MKDIR reported failure while the disk was
 * permanently mutated -- broken write atomicity. The fix rolls the grow back
 * (fat12_shrink_dir_tail) on every post-grow failure path so MKDIR leaks NOTHING.
 *
 * GEOMETRY-DRIVEN, REAL trigger (NOT a compile-time mutant): a fresh mformatted
 * 1.44 MB floppy with ONLY '\SUB' (mmd ::SUB). We then EXHAUST the data area in
 * the live FAT -- mark every data cluster ALLOCATED (0xFFF) except SUB's own
 * cluster (kept EOC, it is a live dir) and EXACTLY ONE free cluster -- flush it,
 * and FILL SUB's single cluster with 14 filler entries so it is full. Now
 * fat12_mkdir('NEWDIR', parent=SUB):
 *   - SUB is full -> grow: fat12_grow_dir consumes the ONE free cluster
 *     (SUB -> 2 clusters);
 *   - find_free for NEWDIR's OWN cluster returns 0 -> NO_SPACE.
 * ASSERTIONS (the disk must be byte-restored modulo nothing leaked):
 *   - the call returns FAT12_ERR_NO_SPACE;
 *   - SUB's chain length is back to 1 (NOT 2);
 *   - the previously-free cluster's FAT entry is FREE again (0x000);
 *   - the live volume free-cluster count is restored to exactly 1.
 * WITHOUT the fix the same image makes ALL of these RED: SUB stays 2 clusters,
 * the free cluster stays consumed, the free count drops to 0 -- a leaked,
 * orphaned cluster while MKDIR failed.
 *
 * Mutant (Rule 6; the Makefile builds it with one perturbed seam):
 *   m-nospace-noroll (FAT12_MUTATE_MKDIR_NO_NOSPACE_ROLLBACK): SKIP the parent-
 *     grow rollback on the NO_SPACE post-grow path -> the appended cluster leaks
 *     -> SUB stays 2 / the free cluster stays consumed / free count == 0 -> RED.
 *
 * Ref (Law 1): the Microsoft FAT spec (free=0x000, EOC 0xFF8..0xFFF) -- the same
 *   source fat12_create / fat12_write_file cite for claimed-but-unused free +
 *   partial-allocation rollback; docs/research/fat12-ground-truth.md Sec 3.
 *   fat12_shrink_dir_tail is the EXACT inverse of one fat12_grow_dir step
 *   (os/milton/fat12.c). Image path is argv (the Makefile mints it) -> no host
 *   path baked in (Rule 11). ASCII (Rule 12).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "test_assert.h"      /* seed/, on -Iseed           */
#include "fat12.h"            /* os/milton/, on -Ios/milton */
#include "blockdev_file.h"    /* fat_diff host backend       */

TEST_HARNESS();

static uint8_t g_fat[12u * 512u];
static uint8_t g_sector[512];
/* DISTINCT from g_sector: fat12_mkdir uses sector_buf for the new dir's cluster
 * zero-fill + the parent-entry RMW, and cluster_buf for the parent-grow zero-
 * fill -- they MUST be different live buffers. */
static uint8_t g_cluster[512];

#define SUB_NAME    "SUB"
#define NEWDIR_NAME "NEWDIR"

/* Decode the raw 12-bit FAT entry for `cluster` from the live in-memory FAT. */
static uint16_t fat_get(const fat12_volume_t *vol, uint32_t fat_len,
                        uint16_t cluster)
{
	uint16_t v = 0xFFFFu;
	(void)fat12_next_cluster(vol, g_fat, fat_len, cluster, &v);
	return v;
}

/* Count the clusters in `start`'s chain (1 = single-cluster, 2 = grew once). */
static uint32_t chain_len(const fat12_volume_t *vol, uint32_t fat_len,
                          uint16_t start)
{
	uint16_t cur = start;
	uint32_t n   = 0u;
	uint32_t max = vol->total_clusters + 2u;
	for (;;) {
		uint16_t next;
		n++;
		if (fat12_next_cluster(vol, g_fat, fat_len, cur, &next) != FAT12_OK) break;
		if (fat12_is_eoc(next)) break;
		cur = next;
		if (n > max) break;
	}
	return n;
}

/* Count FREE data clusters (entry == 0x000) across the whole data area. */
static uint32_t free_count(const fat12_volume_t *vol, uint32_t fat_len)
{
	uint32_t n = 0u;
	uint16_t c;
	for (c = FAT12_FIRST_DATA_CLUSTER;
	     c < (uint16_t)(vol->total_clusters + FAT12_FIRST_DATA_CLUSTER); c++) {
		if (fat12_is_free(fat_get(vol, fat_len, c))) {
			n++;
		}
	}
	return n;
}

/* Locate SUB in the root and return its start cluster (the parent cluster). */
static int sub_start_cluster(const fat12_volume_t *vol, uint16_t *out_start)
{
	dir_entry_t de;
	int rc = fat12_find(vol, g_sector, SUB_NAME, &de);
	if (rc != FAT12_OK) return -1;
	*out_start = de.start_cluster;
	return 0;
}

int main(int argc, char **argv)
{
	const char     *art_img;     /* artifact: blank-but-for-SUB, WRITABLE */
	blockdev_file_t art_bf;
	fat12_volume_t  art_vol;
	uint32_t        fat_len;
	int             rc;
	uint16_t        art_sub = 0u;
	uint16_t        the_free = 0u;   /* the ONE cluster left free            */

	if (argc < 2) {
		fprintf(stderr, "usage: %s <sub-only-artifact-image>\n", argv[0]);
		return 2;
	}
	art_img = argv[1];

	rc = blockdev_file_open_rw(&art_bf, art_img);
	CHECK(rc == 0, "open the SUB-only artifact image read-write");
	if (rc != 0) return TEST_SUMMARY("test_fat12_mkdir_rollback");

	rc = fat12_mount(&art_vol, &art_bf.dev, g_sector);
	CHECK(rc == FAT12_OK, "mount the artifact image");
	rc = fat12_read_fat(&art_vol, g_fat, sizeof(g_fat));
	CHECK(rc == FAT12_OK, "read the artifact FAT");
	fat_len = (uint32_t)art_vol.bpb.sectors_per_fat *
	          (uint32_t)art_vol.bpb.bytes_per_sector;

	rc = sub_start_cluster(&art_vol, &art_sub);
	CHECK(rc == 0 && art_sub >= FAT12_FIRST_DATA_CLUSTER,
	      "SUB present in the artifact root (the non-root parent)");

	/* ---- EXHAUST the data area: mark every data cluster ALLOCATED (0xFFF)
	 * except SUB's OWN cluster (keep its EOC -- it is a live dir) and EXACTLY ONE
	 * free cluster. Pick the free cluster as the HIGHEST data cluster that is not
	 * SUB (so fat12_find_free, which scans low->high from FAT12_FIRST_DATA_CLUSTER,
	 * still hands it to the GROW; after the grow consumes it there is nothing left
	 * for NEWDIR's own cluster -> NO_SPACE). */
	{
		uint16_t c;
		uint16_t last = (uint16_t)(art_vol.total_clusters +
		                           FAT12_FIRST_DATA_CLUSTER - 1u);
		the_free = (last == art_sub) ? (uint16_t)(last - 1u) : last;
		for (c = FAT12_FIRST_DATA_CLUSTER;
		     c < (uint16_t)(art_vol.total_clusters + FAT12_FIRST_DATA_CLUSTER);
		     c++) {
			if (c == art_sub) continue;             /* SUB stays its EOC */
			if (c == the_free) {
				rc = fat12_set_entry(g_fat, fat_len, c, FAT12_FREE);
			} else {
				rc = fat12_set_entry(g_fat, fat_len, c, FAT12_EOC_VALUE);
			}
			CHECK(rc == FAT12_OK, "set a FAT entry while exhausting the data area");
		}
		rc = fat12_flush_fats(&art_vol, g_fat, fat_len);
		CHECK(rc == FAT12_OK, "flush the exhausted FAT to disk");
	}

	/* Sanity: SUB is a single cluster, EXACTLY one free cluster remains, and it
	 * is `the_free`. */
	CHECK(chain_len(&art_vol, fat_len, art_sub) == 1u,
	      "SUB is a single cluster before the grow");
	CHECK(free_count(&art_vol, fat_len) == 1u,
	      "EXACTLY one free data cluster remains after exhaustion");
	CHECK(fat12_is_free(fat_get(&art_vol, fat_len, the_free)),
	      "the chosen free cluster is FREE before MKDIR");

	/* ---- FILL SUB's single cluster: slot0 '.', slot1 '..', slots 2..15 = 14
	 * fillers so the cluster is FULL and MKDIR is forced to GROW SUB. */
	{
		uint32_t lba = BPB_CLUSTER_LBA(&art_vol.bpb, art_sub);
		uint8_t  sb[512];
		int i;
		rc = art_vol.dev->read_sectors(art_vol.dev->ctx, lba, 1u, sb);
		CHECK(rc == 0, "read SUB's cluster to plant the fill entries");
		for (i = 0; i < 14; i++) {
			dir_entry_t f;
			char nm[9];
			size_t ln;
			memset(&f, 0, sizeof(f));
			snprintf(nm, sizeof(nm), "FILL%02d", i);
			ln = strlen(nm);
			memset(f.filename, ' ', 8);
			memcpy(f.filename, nm, ln < 8 ? ln : 8);
			memset(f.extension, ' ', 3);
			f.attribute     = 0x20u;   /* archive (a regular child) */
			f.start_cluster = 0u;
			f.file_size     = 0u;
			memcpy(sb + (2 + i) * 32, &f, 32);   /* slots 2..15 */
		}
		rc = art_vol.dev->write_sectors(art_vol.dev->ctx, lba, 1u, sb);
		CHECK(rc == 0, "plant 14 fill entries to fill SUB's single cluster");
	}

	/* ---- THE TRIGGER: MKDIR must GROW SUB (consuming the_free) and then fail to
	 * claim NEWDIR's own cluster (none left) -> NO_SPACE. */
	rc = fat12_mkdir(&art_vol, g_fat, fat_len, NEWDIR_NAME, art_sub,
	                 g_sector, g_cluster);
	CHECK(rc == FAT12_ERR_NO_SPACE,
	      "fat12_mkdir on a full SUB with one free cluster -> FAT12_ERR_NO_SPACE "
	      "(grow ate the last free cluster, no cluster for the new dir)");

	/* ---- ATOMICITY: the disk is byte-restored modulo nothing leaked. WITHOUT
	 * the rollback fix every one of these is RED (SUB stayed 2 / the_free stayed
	 * consumed / free count == 0). */
	CHECK(chain_len(&art_vol, fat_len, art_sub) == 1u,
	      "after the failed MKDIR, SUB is BACK to 1 cluster (the grow rolled back)");
	CHECK(fat12_is_free(fat_get(&art_vol, fat_len, the_free)),
	      "after the failed MKDIR, the consumed cluster is FREE again (0x000)");
	CHECK(free_count(&art_vol, fat_len) == 1u,
	      "after the failed MKDIR, the free-cluster count is restored to 1 "
	      "(NOTHING leaked -- write atomicity held)");

	/* ---- also assert it on the RE-READ on-disk FAT (the rollback flushed both
	 * copies, not just the in-memory buffer): re-read the FAT and re-check. */
	{
		rc = fat12_read_fat(&art_vol, g_fat, sizeof(g_fat));
		CHECK(rc == FAT12_OK, "re-read the on-disk FAT after the rollback");
		CHECK(chain_len(&art_vol, fat_len, art_sub) == 1u,
		      "on-disk SUB chain is 1 cluster after rollback (flushed to disk)");
		CHECK(fat12_is_free(fat_get(&art_vol, fat_len, the_free)),
		      "on-disk consumed cluster is FREE again after rollback (flushed)");
	}

	blockdev_file_close(&art_bf);
	return TEST_SUMMARY("test_fat12_mkdir_rollback");
}
