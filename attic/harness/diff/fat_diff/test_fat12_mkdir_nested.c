/*
 * harness/diff/fat_diff/test_fat12_mkdir_nested.c -- FAT12 nested MKDIR/RMDIR
 * differential vs mmd (NON-ROOT parent; beads initech-m0bp).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (Law 2: the oracle is the truth, never false-green).
 *
 * Port-and-verify (the project's differential TDD shape) gate for
 * os/milton/fat12.c::{fat12_mkdir, fat12_rmdir} when the PARENT is itself a
 * subdirectory cluster (MD/RD \SUB\NEWDIR where \SUB exists), the nested follow-
 * up to the root-only initech-u6wa landing.
 *
 * The DIFFERENTIAL mirrors test_fat12_mkdir.c but with a SUBDIR parent:
 *   ARTIFACT image  -- a fresh mformatted floppy with ONLY '\SUB' (mmd ::SUB);
 *                      the artifact's fat12_mkdir('NEWDIR', parent=SUB's cluster)
 *                      creates '\SUB\NEWDIR' in place.
 *   GOLDEN image    -- a fresh mformatted floppy with '\SUB' AND '\SUB\NEWDIR'
 *                      both minted by mtools `mmd` (the independent reference for
 *                      the nested '.'/'..' layout: '..' start == the REAL SUB
 *                      cluster, NOT 0).
 * We assert byte-identical on the MEANINGFUL bytes (times normalized away):
 *   (1) the parent NEWDIR entry IN SUB's chain: name/attr (0x10)/start/size 0;
 *   (2) the '.'  entry: start == NEWDIR's own cluster;
 *   (3) the '..' entry: start == SUB's cluster (NOT 0 -- a subdir parent), and
 *       the whole 96-byte cluster head byte-identical to mmd; slot[2] == 0x00;
 *   (4) the new directory's FAT entry == 0xFFF (EOC).
 * NEWDIR is located in SUB's CHAIN in both images via fat12_find_slot_in (the
 * parent_dir_start-aware finder), so the slot addressing is exercised too.
 *
 * Then three in-process legs:
 *   RMDIR    -- fat12_rmdir('NEWDIR', parent=SUB) succeeds; the parent slot in
 *               SUB goes 0xE5 (re-scan SUB) and the cluster goes FREE; a 2nd RD
 *               -> FAT12_ERR_NOT_FOUND; RD of a NON-EMPTY subdir -> NOT_EMPTY.
 *   GROW     -- fill SUB to exactly one cluster of entries, then MKDIR one more
 *               so the parent SUB must GROW; assert the boundary dir lands (found
 *               by fat12_find_slot_in down SUB's chain) AND SUB now spans 2
 *               clusters (fat12_grow_dir was actually exercised). The Makefile
 *               recipe additionally cross-checks `mdir ::SUB` + `fat12_ref.py
 *               --list-path SUB` on the artifact image after this test writes it.
 *
 * Mutants (Rule 6; the Makefile builds these with one perturbed seam each):
 *   m-noroot-mkdir  (FAT12_MUTATE_MKDIR_PARENT_ROOTONLY): re-impose the root-only
 *      guard -> nested MKDIR rejected -> the parent-entry/'.'/'..'/EOC diffs RED.
 *   m-rootscan-mkdir(FAT12_MUTATE_MKDIR_PARENT_SCANROOT): force the parent scan
 *      is_root=1 -> the entry is written to the wrong dir / a false dup -> RED.
 *   m-rootslot-write(FAT12_MUTATE_MKDIR_OWNENTRY_ROOTSLOT): force the own-entry
 *      write-back to a ROOT slot -> NEWDIR lands in root not SUB -> RED.
 *   m-nogrow-parent (FAT12_MUTATE_MKDIR_PARENT_NOGROW): subdir-parent-full
 *      returns DIR_FULL instead of growing -> the GROW leg RED.
 *   m-noroot-rmdir  (FAT12_MUTATE_RMDIR_PARENT_ROOTONLY): re-impose the rmdir
 *      root-only guard -> nested RD wrongly NOT_FOUND -> the RMDIR leg RED.
 *
 * Ref (Law 1): the EMPIRICAL mtools 4.0.43 '.'/'..' layout for a SUBDIR parent
 *   (mmd-minted here as the golden -- NOT inferred; '..' start == the real SUB
 *   cluster); docs/research/fat12-ground-truth.md Sec 4; ADR-0003 DEC-07;
 *   spec/dos_structs.h. Image paths are argv (the Makefile mints them) -> no host
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
/* DISTINCT from g_sector (beads initech-m0bp): fat12_mkdir uses sector_buf for
 * the NEW dir's cluster zero-fill + the parent-entry RMW, and cluster_buf for
 * the parent-grow zero-fill -- they MUST be different live buffers. */
static uint8_t g_cluster[512];

#define SUB_NAME    "SUB"
#define NEWDIR_NAME "NEWDIR"

/* Read the first 96 bytes (slots 0..2) of the directory cluster `cluster`. 1
 * sector/cluster on the 1.44 MB floppy geometry. */
static int read_dir_cluster_head(const fat12_volume_t *vol, uint16_t cluster,
                                 uint8_t *out)
{
	uint32_t lba = BPB_CLUSTER_LBA(&vol->bpb, cluster);
	uint8_t  sb[512];
	if (vol->dev->read_sectors(vol->dev->ctx, lba, 1u, sb) != 0) return -1;
	memcpy(out, sb, 96u);
	return 0;
}

/* Decode the raw 12-bit FAT entry for `cluster` from the live in-memory FAT. */
static uint16_t fat_get(const fat12_volume_t *vol, uint16_t cluster)
{
	uint16_t v = 0xFFFFu;
	(void)fat12_next_cluster(vol, g_fat, (uint32_t)vol->bpb.sectors_per_fat *
	                         (uint32_t)vol->bpb.bytes_per_sector, cluster, &v);
	return v;
}

/* Count the clusters in `start`'s chain (1 = single-cluster, 2 = grew once). */
static uint32_t chain_len(const fat12_volume_t *vol, uint16_t start)
{
	uint32_t fat_len = (uint32_t)vol->bpb.sectors_per_fat *
	                   (uint32_t)vol->bpb.bytes_per_sector;
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

/* Normalize a 32-byte dir entry's CLOCK-derived bytes (offsets 0x0D..0x19) to
 * zero so the differential compares only the MEANINGFUL bytes (Rule 11; mtools
 * writes a real clock, the artifact writes the FIXED 0). */
static void normalize_times(uint8_t *e32)
{
	for (int i = 0x0D; i <= 0x19; i++) {
		e32[i] = 0u;
	}
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
	const char     *gold_img;    /* golden: mmd ::SUB + ::SUB/NEWDIR       */
	blockdev_file_t art_bf, gold_bf;
	fat12_volume_t  art_vol, gold_vol;
	uint32_t        fat_len;
	int             rc;
	uint16_t        art_sub = 0u, gold_sub = 0u;
	dir_entry_t     art_de, gold_de;
	uint32_t        art_slot = 0u, gold_slot = 0u;
	uint16_t        art_start = 0xFFFFu, gold_start = 0xFFFFu;

	if (argc < 3) {
		fprintf(stderr, "usage: %s <sub-only-artifact-image> <mmd-nested-golden-image>\n",
		        argv[0]);
		return 2;
	}
	art_img  = argv[1];
	gold_img = argv[2];

	/* ---- the ARTIFACT writes '\SUB\NEWDIR' via fat12_mkdir --------------- */
	rc = blockdev_file_open_rw(&art_bf, art_img);
	CHECK(rc == 0, "open the SUB-only artifact image read-write");
	if (rc != 0) return TEST_SUMMARY("test_fat12_mkdir_nested");

	rc = fat12_mount(&art_vol, &art_bf.dev, g_sector);
	CHECK(rc == FAT12_OK, "mount the artifact image");
	rc = fat12_read_fat(&art_vol, g_fat, sizeof(g_fat));
	CHECK(rc == FAT12_OK, "read the artifact FAT");
	fat_len = (uint32_t)art_vol.bpb.sectors_per_fat *
	          (uint32_t)art_vol.bpb.bytes_per_sector;

	rc = sub_start_cluster(&art_vol, &art_sub);
	CHECK(rc == 0 && art_sub >= FAT12_FIRST_DATA_CLUSTER,
	      "SUB present in the artifact root (the non-root parent)");

	rc = fat12_mkdir(&art_vol, g_fat, fat_len, NEWDIR_NAME, art_sub,
	                 g_sector, g_cluster);
	CHECK(rc == FAT12_OK, "fat12_mkdir('NEWDIR', parent=SUB) succeeds (nested MD)");

	/* ---- the GOLDEN (mmd ::SUB + ::SUB/NEWDIR) -- read-only ------------- */
	rc = blockdev_file_open(&gold_bf, gold_img);
	CHECK(rc == 0, "open the mmd nested golden image read-only");
	if (rc != 0) {
		blockdev_file_close(&art_bf);
		return TEST_SUMMARY("test_fat12_mkdir_nested");
	}
	rc = fat12_mount(&gold_vol, &gold_bf.dev, g_sector);
	CHECK(rc == FAT12_OK, "mount the golden image");
	rc = sub_start_cluster(&gold_vol, &gold_sub);
	CHECK(rc == 0 && gold_sub >= FAT12_FIRST_DATA_CLUSTER,
	      "SUB present in the golden root");

	/* Locate NEWDIR IN SUB's CHAIN in both (fat12_find_slot_in, parent-aware). */
	rc = fat12_find_slot_in(&art_vol, g_fat, fat_len, art_sub, g_sector,
	                        NEWDIR_NAME, &art_de, &art_slot);
	CHECK(rc == FAT12_OK, "NEWDIR present in the artifact SUB (chain-addressed)");
	art_start = art_de.start_cluster;
	{
		/* The golden FAT for fat12_find_slot_in's chain decode. */
		uint8_t gfat[12u * 512u];
		rc = fat12_read_fat(&gold_vol, gfat, sizeof(gfat));
		CHECK(rc == FAT12_OK, "read the golden FAT");
		rc = fat12_find_slot_in(&gold_vol, gfat, fat_len, gold_sub, g_sector,
		                        NEWDIR_NAME, &gold_de, &gold_slot);
		CHECK(rc == FAT12_OK, "NEWDIR present in the golden SUB (chain-addressed)");
		gold_start = gold_de.start_cluster;
	}

	/* (1) the parent NEWDIR entry IN SUB: byte-identical after time-normalize.
	 * fat12_find_slot_in already returned the parsed entry; diff the meaningful
	 * fields (name/attr/start/size) directly. */
	if (art_start != 0xFFFFu && gold_start != 0xFFFFu) {
		uint8_t ae[32], ge[32];
		memcpy(ae, &art_de, 32);
		memcpy(ge, &gold_de, 32);
		/* The start_cluster differs between images (different free-cluster
		 * layout), so normalize it too along with the times -- the NAME + ATTR +
		 * SIZE are the meaningful, image-independent bytes here; the start_cluster
		 * is checked structurally below ('.' == own, parent FAT == EOC). */
		normalize_times(ae); normalize_times(ge);
		ae[26] = ae[27] = 0u; ge[26] = ge[27] = 0u;   /* start_cluster low word */
		CHECK(ae[11] == 0x10u, "artifact SUB\\NEWDIR attr == 0x10 (directory)");
		CHECK(memcmp(ae, ge, 32) == 0,
		      "SUB\\NEWDIR parent entry name/attr/size byte-identical to mmd");
	}

	/* (2)+(3)+(4) the NEWDIR cluster head (slots 0..2 = '.'/'..'/end). The '..'
	 * start must be the REAL SUB cluster (NOT 0) -- the nested-parent rule. The
	 * whole 96-byte head is byte-identical to mmd after time-normalize. */
	if (art_start != 0xFFFFu && gold_start != 0xFFFFu) {
		uint8_t ah[96], gh[96];
		read_dir_cluster_head(&art_vol,  art_start,  ah);
		read_dir_cluster_head(&gold_vol, gold_start, gh);
		for (int k = 0; k < 3; k++) { normalize_times(ah + k*32); normalize_times(gh + k*32); }

		CHECK(ah[0] == 0x2Eu && ah[1] == 0x20u && ah[11] == 0x10u,
		      "artifact '.' = {0x2E, 0x20*10}, attr 0x10");
		CHECK((uint16_t)(ah[26] | (ah[27] << 8)) == art_start,
		      "artifact '.' start_cluster == NEWDIR's OWN cluster");
		CHECK(ah[32] == 0x2Eu && ah[33] == 0x2Eu && ah[34] == 0x20u && ah[43] == 0x10u,
		      "artifact '..' = {0x2E,0x2E, 0x20*9}, attr 0x10");
		CHECK((uint16_t)(ah[58] | (ah[59] << 8)) == art_sub,
		      "artifact '..' start_cluster == SUB's REAL cluster (NOT 0 -- subdir parent)");
		CHECK((uint16_t)(ah[58] | (ah[59] << 8)) != 0u,
		      "artifact '..' start_cluster is NON-ZERO (the m-dotdot-self/root mutants bite)");
		CHECK(ah[64] == 0x00u,
		      "artifact dir slot[2] == 0x00 (end-of-directory sentinel)");

		/* The cluster heads differ ONLY in the '.'/'..' start_cluster values
		 * (each image has a different free-cluster layout). Normalize the two
		 * start words in '.' (0x1A) and '..' (0x3A) to a structural form before
		 * the byte diff: set '.' start to 1 (own marker) and '..' start to 2
		 * (parent marker) in both, then compare. This still bites the mutants:
		 * m-dotdot-self writes '..' == own (so '..' != SUB, and != the parent
		 * marker after a SUB-relative normalize) -- caught by the explicit
		 * '..' == SUB check above. */
		uint8_t an[96], gn[96];
		memcpy(an, ah, 96); memcpy(gn, gh, 96);
		an[26]=1; an[27]=0;  gn[26]=1; gn[27]=0;   /* '.' start -> 1 */
		an[58]=2; an[59]=0;  gn[58]=2; gn[59]=0;   /* '..' start -> 2 */
		CHECK(memcmp(an, gn, 96) == 0,
		      "NEWDIR cluster head ('.'/'..'/end) structurally identical to mmd");
	}

	/* (4) the new directory's FAT entry == 0xFFF (EOC). */
	{
		uint16_t v = fat_get(&art_vol, art_start);
		CHECK(v == FAT12_EOC_VALUE,
		      "artifact SUB\\NEWDIR FAT entry == 0xFFF (EOC)");
	}

	/* (5) the own-entry landed at the CORRECT SLOT in SUB, and SUB's '.'/'..'
	 * are UNTOUCHED. SUB started with slot0 '.', slot1 '..', so the new entry
	 * must occupy slot 2 (the first free slot past '.'/'..'). A scan that wrongly
	 * treats the parent as the ROOT (m-rootscan-mkdir) computes a free_slot from
	 * the root region (index 1, just after SUB's root entry) and write_dirent_in
	 * then clobbers SUB's '..' at slot 1 -- so this slot==2 + '..'-intact check is
	 * what makes the rootscan mutant bite the BASIC nested case, not merely the
	 * grow leg (Rule 3: a mutant that only passes because the slots don't overlap
	 * proves nothing). */
	{
		CHECK(art_slot == 2u,
		      "SUB\\NEWDIR landed at SUB slot 2 (first free past '.'/'..'); "
		      "m-rootscan-mkdir writes it to the wrong slot");
		/* Re-read SUB's '..' (slot 1) from disk: still {0x2E,0x2E,...} with start
		 * == 0 (SUB's parent is the root). */
		uint8_t sh[96];
		read_dir_cluster_head(&art_vol, art_sub, sh);
		CHECK(sh[32] == 0x2Eu && sh[33] == 0x2Eu && sh[34] == 0x20u &&
		      sh[43] == 0x10u && (uint16_t)(sh[58] | (sh[59] << 8)) == 0u,
		      "SUB's own '..' (slot 1) is UNTOUCHED by the nested MKDIR "
		      "(start == 0, the root)");
	}

	/* ---- RMDIR leg: remove '\SUB\NEWDIR' from the SUBDIR parent ---------- */
	{
		rc = fat12_rmdir(&art_vol, g_fat, fat_len, NEWDIR_NAME, art_sub, g_sector);
		CHECK(rc == FAT12_OK, "fat12_rmdir('NEWDIR', parent=SUB) (empty) succeeds");

		/* Re-scan SUB: the parent slot is gone (deleted); the cluster is freed. */
		dir_entry_t gde; uint32_t gslot;
		rc = fat12_find_slot_in(&art_vol, g_fat, fat_len, art_sub, g_sector,
		                        NEWDIR_NAME, &gde, &gslot);
		CHECK(rc == FAT12_ERR_NOT_FOUND,
		      "after nested RMDIR, NEWDIR is gone from SUB (parent slot deleted)");
		{
			uint16_t v = fat_get(&art_vol, art_start);
			CHECK(v == FAT12_FREE,
			      "after nested RMDIR, the freed cluster's FAT entry == 0x000 (free)");
		}

		/* A 2nd RD of the now-missing name -> FAT12_ERR_NOT_FOUND. */
		rc = fat12_rmdir(&art_vol, g_fat, fat_len, NEWDIR_NAME, art_sub, g_sector);
		CHECK(rc == FAT12_ERR_NOT_FOUND,
		      "fat12_rmdir of a missing nested name -> FAT12_ERR_NOT_FOUND");
	}

	/* ---- RMDIR non-empty leg: a nested dir with a child is NOT removable -- *
	 * Re-MKDIR 'FULLDIR' in SUB, plant a real child file entry at slot[2] of its
	 * cluster (the old 0x00 sentinel) via a raw RMW, then fat12_rmdir must refuse
	 * NOT_EMPTY (the empty-check enumerates the TARGET chain regardless of where
	 * the target's PARENT is). */
	{
		uint16_t full_start = 0xFFFFu;
		rc = fat12_mkdir(&art_vol, g_fat, fat_len, "FULLDIR", art_sub,
		                 g_sector, g_cluster);
		CHECK(rc == FAT12_OK, "re-MKDIR 'FULLDIR' in SUB for the non-empty proof");
		dir_entry_t fde; uint32_t fslot;
		rc = fat12_find_slot_in(&art_vol, g_fat, fat_len, art_sub, g_sector,
		                        "FULLDIR", &fde, &fslot);
		CHECK(rc == FAT12_OK, "FULLDIR present in SUB after re-MKDIR");
		full_start = fde.start_cluster;

		if (rc == FAT12_OK) {
			uint32_t lba = BPB_CLUSTER_LBA(&art_vol.bpb, full_start);
			uint8_t  sb[512];
			rc = art_vol.dev->read_sectors(art_vol.dev->ctx, lba, 1u, sb);
			CHECK(rc == 0, "read FULLDIR cluster for the non-empty plant");
			dir_entry_t child;
			memset(&child, 0, sizeof(child));
			memcpy(child.filename, "CHILD   ", 8);
			memcpy(child.extension, "TXT", 3);
			child.attribute     = 0x20u;
			child.start_cluster = 0u;
			child.file_size     = 0u;
			memcpy(sb + 64, &child, 32);   /* slot[2] */
			rc = art_vol.dev->write_sectors(art_vol.dev->ctx, lba, 1u, sb);
			CHECK(rc == 0, "write the planted child entry into FULLDIR");

			rc = fat12_rmdir(&art_vol, g_fat, fat_len, "FULLDIR", art_sub, g_sector);
			CHECK(rc == FAT12_ERR_NOT_EMPTY,
			      "fat12_rmdir of a NON-EMPTY nested dir -> FAT12_ERR_NOT_EMPTY");

			/* Clean FULLDIR back out so it does not interfere with the GROW leg's
			 * slot accounting -- remove the planted child then RMDIR it. */
			memset(sb + 64, 0, 32);
			(void)art_vol.dev->write_sectors(art_vol.dev->ctx, lba, 1u, sb);
			rc = fat12_rmdir(&art_vol, g_fat, fat_len, "FULLDIR", art_sub, g_sector);
			CHECK(rc == FAT12_OK, "FULLDIR (now empty) removed before the GROW leg");
		}
	}

	/* ---- PARENT-GROW leg: fill SUB to exactly one cluster, then MKDIR once
	 * more so SUB must GROW. SUB's cluster holds 16 entries: slot0 '.', slot1
	 * '..', slots 2..15 = 14 free. Plant 14 filler FILE entries (start 0, size 0)
	 * into slots 2..15 via raw sector RMW so the cluster is FULL, then MKDIR
	 * 'GROWDIR' -- the only free slot is index 16, one past the last cluster, so
	 * fat12_mkdir must grow SUB (fat12_grow_dir) to place it. We assert GROWDIR
	 * lands (found down SUB's NOW 2-cluster chain) and SUB spans 2 clusters. The
	 * m-nogrow-parent mutant returns DIR_FULL here -> the MKDIR fails -> RED. */
	{
		uint32_t lba = BPB_CLUSTER_LBA(&art_vol.bpb, art_sub);
		uint8_t  sb[512];
		rc = art_vol.dev->read_sectors(art_vol.dev->ctx, lba, 1u, sb);
		CHECK(rc == 0, "read SUB's cluster to plant the fill entries");

		for (int i = 0; i < 14; i++) {
			dir_entry_t f;
			char nm[9];
			memset(&f, 0, sizeof(f));
			snprintf(nm, sizeof(nm), "FILL%02d", i);
			memset(f.filename, ' ', 8);
			memcpy(f.filename, nm, strlen(nm) < 8 ? strlen(nm) : 8);
			memset(f.extension, ' ', 3);
			f.attribute     = 0x20u;   /* archive (a regular child) */
			f.start_cluster = 0u;
			f.file_size     = 0u;
			memcpy(sb + (2 + i) * 32, &f, 32);   /* slots 2..15 */
		}
		rc = art_vol.dev->write_sectors(art_vol.dev->ctx, lba, 1u, sb);
		CHECK(rc == 0, "plant 14 fill entries to fill SUB's single cluster");

		CHECK(chain_len(&art_vol, art_sub) == 1u,
		      "SUB is a single cluster BEFORE the grow");

		rc = fat12_mkdir(&art_vol, g_fat, fat_len, "GROWDIR", art_sub,
		                 g_sector, g_cluster);
		CHECK(rc == FAT12_OK,
		      "fat12_mkdir('GROWDIR', parent=SUB) on a FULL SUB grows + succeeds");

		dir_entry_t gde; uint32_t gslot;
		rc = fat12_find_slot_in(&art_vol, g_fat, fat_len, art_sub, g_sector,
		                        "GROWDIR", &gde, &gslot);
		CHECK(rc == FAT12_OK,
		      "GROWDIR found in SUB's grown chain (slot past the 1st cluster)");
		CHECK(gslot >= 16u,
		      "GROWDIR landed at slot >= 16 (the appended 2nd cluster, not the 1st)");

		CHECK(chain_len(&art_vol, art_sub) == 2u,
		      "SUB now spans 2 clusters -- fat12_grow_dir was actually exercised");
	}

	blockdev_file_close(&art_bf);
	blockdev_file_close(&gold_bf);
	return TEST_SUMMARY("test_fat12_mkdir_nested");
}
