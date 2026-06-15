/*
 * harness/diff/fat_diff/test_fat12_mkdir.c -- FAT12 MKDIR differential vs mmd.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (Law 2: the oracle is the truth, never false-green).
 *
 * Port-and-verify (the project's differential TDD shape) gate for
 * os/milton/fat12.c::{fat12_mkdir, fat12_rmdir} (beads initech-u6wa, the FAT12
 * subdirectory CREATE/REMOVE write side, ROOT-PARENT only).
 *
 * The DIFFERENTIAL: the artifact (fat12_mkdir) creates '\NEWDIR' on a fresh
 * mformatted image; mtools `mmd` independently mints the SAME directory on a
 * second fresh mformatted image (the GOLDEN). We assert the artifact's on-disk
 * bytes are byte-identical to mtools' on the MEANINGFUL bytes:
 *   (1) the parent (root) NEWDIR entry: name/attr (0x10)/start_cluster/size 0;
 *   (2) the '.'  entry: name {0x2E,0x20*10}, attr 0x10, start = the dir's own
 *       cluster, size 0;
 *   (3) the '..' entry: name {0x2E,0x2E,0x20*9}, attr 0x10, start = the PARENT
 *       (ROOT encoded as 0 -- the empirical mtools rule), size 0;
 *   (4) slot[2] of the dir cluster is the 0x00 end-of-directory sentinel;
 *   (5) the new directory's FAT entry == 0xFFF (EOC).
 * mtime/mdate are NORMALIZED away before the diff (mtools writes a real clock;
 * the artifact writes the FIXED 0 -- they are NOT meaningful bytes; Rule 11).
 *
 * Plus an in-process RMDIR round-trip leg: after MKDIR, fat12_rmdir removes the
 * empty '\NEWDIR' (the parent slot goes deleted, the FAT entry goes free), and a
 * second fat12_rmdir of the now-missing name fails with FAT12_ERR_NOT_FOUND; and
 * fat12_rmdir of a NON-EMPTY directory (one with a child file) is refused with
 * FAT12_ERR_NOT_EMPTY (the empty-check bites).
 *
 * Mutants (Rule 6; the Makefile builds these with one perturbed constant each):
 *   m2 (FAT12_MUTATE_MKDIR_DOTDOT_SELF): '..' start = the dir's OWN cluster
 *      instead of the parent (root=0) -> the '..' diff vs mmd goes RED (THE
 *      canonical '..'-rule mutant).
 *   m3 (FAT12_MUTATE_MKDIR_NO_EOC): skip the set_entry EOC on the new cluster
 *      -> the FAT-entry / free-count diff bites.
 *   m4 (FAT12_MUTATE_RMDIR_NO_EMPTYCHECK): fat12_rmdir omits the empty-check ->
 *      RD of a non-empty dir wrongly SUCCEEDS (the RMDIR-non-empty leg goes RED).
 *
 * Ref (Law 1): the EMPIRICAL mtools 4.0.43 '.'/'..' layout (triple-confirmed,
 *   re-minted here as the golden -- NOT inferred); docs/research/fat12-ground-
 *   truth.md Sec 4; ADR-0003 DEC-07; spec/dos_structs.h. Image paths are argv
 *   (the Makefile mints them) -> no host path baked in (Rule 11). ASCII (Rule 12).
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
/* DISTINCT from g_sector: fat12_mkdir uses sector_buf for the NEW dir's cluster
 * zero-fill + the parent-entry RMW, and cluster_buf for the parent-grow zero-
 * fill -- they must be different live buffers (beads initech-m0bp). The root-
 * parent legs in this test never grow, so g_cluster is untouched here, but the
 * call must still pass a distinct buffer. */
static uint8_t g_cluster[512];

/* The bare 8.3 directory name both the artifact and mmd create. */
#define NEWDIR_NAME "NEWDIR"

/* Read the 32-byte dir entry at 0-based root slot `slot` from a mounted volume
 * into `out` (32 bytes). */
static int read_root_slot(const fat12_volume_t *vol, uint32_t slot, uint8_t *out)
{
	uint32_t per_sector = 512u / 32u;
	uint32_t sec_index  = slot / per_sector;
	uint32_t in_sec     = slot % per_sector;
	uint32_t lba        = vol->root_dir_sector + sec_index;
	uint8_t  sb[512];
	if (sec_index >= vol->root_dir_sectors) return -1;
	if (vol->dev->read_sectors(vol->dev->ctx, lba, 1u, sb) != 0) return -1;
	memcpy(out, sb + in_sec * 32u, 32u);
	return 0;
}

/* Read the first 96 bytes (slots 0..2) of the directory cluster `cluster` into
 * `out` (96 bytes). 1 sector/cluster on the 1.44 MB floppy geometry. */
static int read_dir_cluster_head(const fat12_volume_t *vol, uint16_t cluster,
                                 uint8_t *out)
{
	uint32_t lba = BPB_CLUSTER_LBA(&vol->bpb, cluster);
	uint8_t  sb[512];
	if (vol->dev->read_sectors(vol->dev->ctx, lba, 1u, sb) != 0) return -1;
	memcpy(out, sb, 96u);
	return 0;
}

/* Locate the root slot whose 8.3 name matches `name83` (case-insensitive). The
 * NEWDIR start cluster is returned via *out_start. Returns the slot index, or -1
 * if not found. */
static int find_root_dir_named(const fat12_volume_t *vol, const char *name83,
                               uint16_t *out_start)
{
	uint32_t per_sector = 512u / 32u;
	uint32_t total = vol->root_dir_sectors * per_sector;
	for (uint32_t s = 0; s < total; s++) {
		uint8_t e[32];
		if (read_root_slot(vol, s, e) != 0) return -1;
		if (e[0] == 0x00) break;            /* end of directory */
		if (e[0] == 0xE5) continue;
		if (e[11] == 0x0Fu) continue;       /* LFN */
		char nm[13];
		dir_entry_t de;
		memcpy(&de, e, 32);
		fat12_format_83(&de, nm);
		/* Case-insensitive compare against name83. */
		int eq = 1; const char *a = nm, *b = name83;
		while (*a && *b) {
			char ca = *a, cb = *b;
			if (ca >= 'a' && ca <= 'z') ca = (char)(ca - 32);
			if (cb >= 'a' && cb <= 'z') cb = (char)(cb - 32);
			if (ca != cb) { eq = 0; break; }
			a++; b++;
		}
		if (eq && *a == '\0' && *b == '\0') {
			*out_start = de.start_cluster;
			return (int)s;
		}
	}
	return -1;
}

/* Decode the raw 12-bit FAT entry for `cluster` from the live in-memory FAT. */
static uint16_t fat_get(const fat12_volume_t *vol, uint16_t cluster)
{
	uint16_t v = 0xFFFFu;
	(void)fat12_next_cluster(vol, g_fat, (uint32_t)vol->bpb.sectors_per_fat *
	                         (uint32_t)vol->bpb.bytes_per_sector, cluster, &v);
	return v;
}

/* Normalize a 32-byte dir entry's CLOCK-derived bytes to zero so the
 * differential compares only the MEANINGFUL bytes (Rule 11; mtools writes a real
 * clock into the creation/access/write time+date fields, the artifact writes the
 * FIXED 0). The DOS dir-entry timestamp region (and the FAT high-cluster word,
 * always 0 on FAT12) spans offsets 0x0D..0x19:
 *   0x0D       creation time, tenths of a second
 *   0x0E..0x0F creation time
 *   0x10..0x11 creation date
 *   0x12..0x13 last-access date
 *   0x14..0x15 high word of start_cluster (0 on FAT12)
 *   0x16..0x17 last-write time (mtime)
 *   0x18..0x19 last-write date (mdate)
 * The MEANINGFUL bytes the diff keeps: name (0x00..0x0A), attr (0x0B),
 * start_cluster low word (0x1A..0x1B), size (0x1C..0x1F). */
static void normalize_times(uint8_t *e32)
{
	for (int i = 0x0D; i <= 0x19; i++) {
		e32[i] = 0u;
	}
}

int main(int argc, char **argv)
{
	const char     *art_img;     /* artifact image: blank, WRITABLE */
	const char     *gold_img;    /* golden image: mmd-minted ::NEWDIR */
	blockdev_file_t art_bf, gold_bf;
	fat12_volume_t  art_vol, gold_vol;
	uint32_t        fat_len;
	int             rc;
	int             art_slot, gold_slot;
	uint16_t        art_start = 0xFFFFu, gold_start = 0xFFFFu;

	if (argc < 3) {
		fprintf(stderr, "usage: %s <blank-artifact-image> <mmd-golden-image>\n",
		        argv[0]);
		return 2;
	}
	art_img  = argv[1];
	gold_img = argv[2];

	/* ---- the ARTIFACT writes '\NEWDIR' via fat12_mkdir ------------------- */
	rc = blockdev_file_open_rw(&art_bf, art_img);
	CHECK(rc == 0, "open the blank artifact image read-write");
	if (rc != 0) return TEST_SUMMARY("test_fat12_mkdir");

	rc = fat12_mount(&art_vol, &art_bf.dev, g_sector);
	CHECK(rc == FAT12_OK, "mount the artifact image");
	rc = fat12_read_fat(&art_vol, g_fat, sizeof(g_fat));
	CHECK(rc == FAT12_OK, "read the artifact FAT");
	fat_len = (uint32_t)art_vol.bpb.sectors_per_fat *
	          (uint32_t)art_vol.bpb.bytes_per_sector;

	rc = fat12_mkdir(&art_vol, g_fat, fat_len, NEWDIR_NAME, 0u, g_sector, g_cluster);
	CHECK(rc == FAT12_OK, "fat12_mkdir('\\NEWDIR') in the root succeeds");

	/* ---- the GOLDEN (mmd) image -- read-only ----------------------------- */
	rc = blockdev_file_open(&gold_bf, gold_img);
	CHECK(rc == 0, "open the mmd golden image read-only");
	if (rc != 0) {
		blockdev_file_close(&art_bf);
		return TEST_SUMMARY("test_fat12_mkdir");
	}
	rc = fat12_mount(&gold_vol, &gold_bf.dev, g_sector);
	CHECK(rc == FAT12_OK, "mount the golden image");

	/* Locate NEWDIR in both root directories. */
	art_slot  = find_root_dir_named(&art_vol,  NEWDIR_NAME, &art_start);
	gold_slot = find_root_dir_named(&gold_vol, NEWDIR_NAME, &gold_start);
	CHECK(art_slot >= 0,  "NEWDIR present in the artifact root");
	CHECK(gold_slot >= 0, "NEWDIR present in the golden root");

	/* (1) the parent (root) NEWDIR entry: byte-identical after time-normalize. */
	if (art_slot >= 0 && gold_slot >= 0) {
		uint8_t ae[32], ge[32];
		read_root_slot(&art_vol,  (uint32_t)art_slot,  ae);
		read_root_slot(&gold_vol, (uint32_t)gold_slot, ge);
		normalize_times(ae); normalize_times(ge);
		CHECK(ae[11] == 0x10u, "artifact NEWDIR attr == 0x10 (directory)");
		CHECK(memcmp(ae, ge, 32) == 0,
		      "root NEWDIR entry byte-identical to mmd (name/attr/start/size)");
	}

	/* (2)+(3)+(4) the dir cluster head (slots 0..2 = '.' / '..' / end). The diff
	 * is byte-for-byte after time-normalize -- so '.' start == own cluster, '..'
	 * start == parent (ROOT encoded as 0), slot[2] == 0x00, all proven at once. */
	if (art_slot >= 0 && gold_slot >= 0) {
		uint8_t ah[96], gh[96];
		read_dir_cluster_head(&art_vol,  art_start,  ah);
		read_dir_cluster_head(&gold_vol, gold_start, gh);
		/* Normalize times in each of the three 32-byte entries. */
		for (int k = 0; k < 3; k++) { normalize_times(ah + k*32); normalize_times(gh + k*32); }

		/* Spell out the empirical layout so a failure points at the exact rule. */
		CHECK(ah[0] == 0x2Eu && ah[1] == 0x20u && ah[11] == 0x10u,
		      "artifact '.' = {0x2E, 0x20*10}, attr 0x10");
		CHECK((uint16_t)(ah[26] | (ah[27] << 8)) == art_start,
		      "artifact '.' start_cluster == the dir's OWN cluster");
		CHECK(ah[32] == 0x2Eu && ah[33] == 0x2Eu && ah[34] == 0x20u && ah[43] == 0x10u,
		      "artifact '..' = {0x2E,0x2E, 0x20*9}, attr 0x10");
		CHECK((uint16_t)(ah[58] | (ah[59] << 8)) == 0u,
		      "artifact '..' start_cluster == 0 (PARENT is the root, encoded as 0)");
		CHECK(ah[64] == 0x00u,
		      "artifact dir slot[2] == 0x00 (end-of-directory sentinel)");

		/* THE differential: the whole 96-byte head is byte-identical to mmd. The
		 * m2 ('..'=self) mutant flips ah[58..59] from 0 to art_start -> this RED. */
		CHECK(memcmp(ah, gh, 96) == 0,
		      "dir cluster head ('.'/'..'/end) byte-identical to mmd (the '..'-rule diff)");
	}

	/* (5) the new directory's FAT entry == 0xFFF (EOC). The m3 (no-EOC) mutant
	 * leaves it free (0x000) -> this RED. */
	{
		uint16_t v = fat_get(&art_vol, art_start);
		CHECK(v == FAT12_EOC_VALUE,
		      "artifact NEWDIR FAT entry == 0xFFF (EOC); m3 (no-EOC) bites here");
	}

	/* ---- RMDIR round-trip + empty-check leg (in-process) ----------------- *
	 * The MKDIR'd '\NEWDIR' is EMPTY ('.'/'..' only), so RMDIR succeeds; a second
	 * RMDIR of the now-missing name fails NOT_FOUND; and a re-created dir we stuff
	 * with a child entry is refused NOT_EMPTY (the m4 empty-check proof, below). */
	{
		/* RMDIR the now-empty '\NEWDIR' from the root -> success. */
		rc = fat12_rmdir(&art_vol, g_fat, fat_len, NEWDIR_NAME, 0u, g_sector);
		CHECK(rc == FAT12_OK, "fat12_rmdir('\\NEWDIR') (empty) succeeds");

		/* The parent slot is gone (deleted) + the cluster is freed (FAT free). */
		uint16_t gone_start = 0xFFFFu;
		CHECK(find_root_dir_named(&art_vol, NEWDIR_NAME, &gone_start) < 0,
		      "after RMDIR, NEWDIR is gone from the root (entry deleted)");
		{
			uint16_t v = fat_get(&art_vol, art_start);
			CHECK(v == FAT12_FREE,
			      "after RMDIR, the freed cluster's FAT entry == 0x000 (free)");
		}

		/* (c) RMDIR of the now-missing name -> FAT12_ERR_NOT_FOUND. */
		rc = fat12_rmdir(&art_vol, g_fat, fat_len, NEWDIR_NAME, 0u, g_sector);
		CHECK(rc == FAT12_ERR_NOT_FOUND,
		      "fat12_rmdir of a missing name -> FAT12_ERR_NOT_FOUND");
	}

	/* ---- the empty-check (m4) proof: a dir with a child is NOT removable --- *
	 * A nested MKDIR is out of scope (root-only this landing), so we make a dir
	 * non-empty by MKDIR'ing a fresh 'FULLDIR' then planting a real file entry at
	 * slot[2] of its cluster (the old 0x00 end sentinel) via a raw read-modify-
	 * write sector. fat12_rmdir must then refuse FULLDIR (NOT_EMPTY); the m4
	 * mutant (no empty-check) wrongly succeeds, turning the assertion RED. */
	{
		rc = fat12_mkdir(&art_vol, g_fat, fat_len, "FULLDIR", 0u, g_sector, g_cluster);
		CHECK(rc == FAT12_OK, "re-MKDIR 'FULLDIR' for the non-empty proof");
		uint16_t full_start = 0xFFFFu;
		int fs = find_root_dir_named(&art_vol, "FULLDIR", &full_start);
		CHECK(fs >= 0, "FULLDIR present after re-MKDIR");

		if (fs >= 0) {
			/* Plant a real child file entry at slot[2] of FULLDIR's cluster (the
			 * old 0x00 end sentinel) via a raw read-modify-write sector. */
			uint32_t lba = BPB_CLUSTER_LBA(&art_vol.bpb, full_start);
			uint8_t  sb[512];
			rc = art_vol.dev->read_sectors(art_vol.dev->ctx, lba, 1u, sb);
			CHECK(rc == 0, "read FULLDIR cluster for the non-empty plant");
			dir_entry_t child;
			memset(&child, 0, sizeof(child));
			memcpy(child.filename, "CHILD   ", 8);
			memcpy(child.extension, "TXT", 3);
			child.attribute     = 0x20u;  /* archive */
			child.start_cluster = 0u;
			child.file_size     = 0u;
			memcpy(sb + 64, &child, 32);  /* slot[2] */
			rc = art_vol.dev->write_sectors(art_vol.dev->ctx, lba, 1u, sb);
			CHECK(rc == 0, "write the planted child entry into FULLDIR");

			/* fat12_rmdir must now REFUSE FULLDIR: it is non-empty. The m4 mutant
			 * (no empty-check) wrongly returns FAT12_OK -> this assertion RED. */
			rc = fat12_rmdir(&art_vol, g_fat, fat_len, "FULLDIR", 0u, g_sector);
			CHECK(rc == FAT12_ERR_NOT_EMPTY,
			      "fat12_rmdir of a NON-EMPTY dir -> FAT12_ERR_NOT_EMPTY (m4 bites)");
		}
	}

	blockdev_file_close(&art_bf);
	blockdev_file_close(&gold_bf);
	return TEST_SUMMARY("test_fat12_mkdir");
}
