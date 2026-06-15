/*
 * harness/diff/fat_diff/test_fat12_rename.c -- FAT12 RENAME differential vs mren.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (Law 2: the oracle is the truth, never false-green).
 *
 * Port-and-verify (the project's differential TDD shape) gate for
 * os/milton/fat12.c::fat12_rename (beads initech-gnrc, the FAT12 SAME-directory
 * dir-entry rename -- the WRITE side of INT 21h AH=56h RENAME).
 *
 * The DIFFERENTIAL: both images start as a fresh mformatted 1.44 MB floppy with a
 * seed file mcopy'd in as ::OLD.TXT (a multi-cluster body so start_cluster + size
 * are non-trivial). The ARTIFACT (fat12_rename) renames OLD.TXT -> NEW.BAK on its
 * image; mtools `mren ::OLD.TXT ::NEW.BAK` independently does the SAME rename on
 * the GOLDEN image. OLD and NEW differ in BOTH name AND extension (.TXT -> .BAK)
 * so the name-only8 mutant (which leaves the extension stale) bites. We assert
 * the artifact's on-disk bytes are byte-identical to mtools' on the MEANINGFUL
 * bytes:
 *   (1) the NEW.BAK dir entry now reads NEW.BAK in its 11-byte name field, and is
 *       byte-identical to mren's after time-normalize (name/attr/start/size);
 *   (2) start_cluster + file_size are UNCHANGED from the pre-rename OLD.TXT entry
 *       (rename allocates/frees nothing -- name-field-ONLY; Rule 11);
 *   (3) the OLD.TXT name no longer resolves in the root;
 *   (4) the whole FAT (both copies) is BYTE-UNCHANGED by the rename.
 * mtime/mdate are NORMALIZED away before the entry diff (mtools writes a real
 * clock; the artifact preserves the FIXED bytes mcopy seeded -- not meaningful;
 * Rule 11).
 *
 * Plus in-process legs (no second image needed):
 *   - rename of a MISSING source -> FAT12_ERR_NOT_FOUND;
 *   - rename ONTO an existing dest name -> FAT12_ERR_EXISTS (the load-bearing
 *     dest-exists reject -- m1 NO_DESTCHECK bites here + on the differential).
 *
 * Mutants (Rule 6; the Makefile builds these with one perturbed constant each):
 *   m1 (FAT12_MUTATE_RENAME_NO_DESTCHECK): skip the dest-absent scan -> rename
 *      onto an existing dest wrongly succeeds -> the dest-exists leg + the mren
 *      differential go RED.
 *   m2 (FAT12_MUTATE_RENAME_TOUCH_CHAIN): zero start_cluster on the rewrite ->
 *      the 'start_cluster + size unchanged' assertion vs mren goes RED.
 *   m3 (FAT12_MUTATE_RENAME_NAME_ONLY8): copy only filename[0..7], leave the
 *      extension stale -> the name-field differential goes RED.
 *
 * Ref (Law 1): the EMPIRICAL mtools 4.0.43 name-field layout (mren-minted here as
 *   the golden -- NOT inferred); docs/research/fat12-ground-truth.md Sec 4;
 *   ADR-0003 DEC-07; spec/dos_structs.h. Image paths are argv (the Makefile mints
 *   them) -> no host path baked in (Rule 11). ASCII (Rule 12).
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
static uint8_t g_fat_before[12u * 512u];   /* FAT snapshot to prove it is untouched */
static uint8_t g_sector[512];

/* OLD and NEW deliberately differ in BOTH the name AND the extension (OLD.TXT ->
 * NEW.BAK) so the name-only8 mutant (copies only filename[0..7], leaves the
 * extension stale) leaves a wrong '.TXT' on disk and the name-field differential
 * bites. A NEW name sharing OLD's extension would hide that mutant. */
#define OLD_NAME "OLD.TXT"
#define NEW_NAME "NEW.BAK"

/* Read the 32-byte dir entry at 0-based root slot `slot` into `out` (32 bytes). */
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

/* Locate the root slot whose 8.3 name matches `name83` (case-insensitive).
 * start_cluster + size are returned via the OUT params. Returns the slot index,
 * or -1 if not found. */
static int find_root_named(const fat12_volume_t *vol, const char *name83,
                           uint16_t *out_start, uint32_t *out_size)
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
		int eq = 1; const char *a = nm, *b = name83;
		while (*a && *b) {
			char ca = *a, cb = *b;
			if (ca >= 'a' && ca <= 'z') ca = (char)(ca - 32);
			if (cb >= 'a' && cb <= 'z') cb = (char)(cb - 32);
			if (ca != cb) { eq = 0; break; }
			a++; b++;
		}
		if (eq && *a == '\0' && *b == '\0') {
			if (out_start) *out_start = de.start_cluster;
			if (out_size)  *out_size  = de.file_size;
			return (int)s;
		}
	}
	return -1;
}

/* Normalize a 32-byte dir entry's CLOCK-derived bytes to zero so the differential
 * compares only the MEANINGFUL bytes (name 0x00..0x0A, attr 0x0B, start 0x1A..
 * 0x1B, size 0x1C..0x1F). The timestamp region (+ FAT12 high-cluster word, always
 * 0) spans 0x0D..0x19; see test_fat12_mkdir.c::normalize_times for the field map. */
static void normalize_times(uint8_t *e32)
{
	for (int i = 0x0D; i <= 0x19; i++) {
		e32[i] = 0u;
	}
}

int main(int argc, char **argv)
{
	const char     *art_img;     /* artifact image: OLD.TXT seeded, WRITABLE */
	const char     *gold_img;    /* golden image: mren OLD.TXT -> NEW.TXT     */
	blockdev_file_t art_bf, gold_bf;
	fat12_volume_t  art_vol, gold_vol;
	uint32_t        fat_len;
	int             rc;
	int             slot;
	uint16_t        old_start = 0xFFFFu;
	uint32_t        old_size  = 0xFFFFFFFFu;

	if (argc < 3) {
		fprintf(stderr, "usage: %s <artifact-image OLD.TXT> <mren-golden-image>\n",
		        argv[0]);
		return 2;
	}
	art_img  = argv[1];
	gold_img = argv[2];

	/* ---- the ARTIFACT renames OLD.TXT -> NEW.TXT via fat12_rename --------- */
	rc = blockdev_file_open_rw(&art_bf, art_img);
	CHECK(rc == 0, "open the artifact image read-write");
	if (rc != 0) return TEST_SUMMARY("test_fat12_rename");

	rc = fat12_mount(&art_vol, &art_bf.dev, g_sector);
	CHECK(rc == FAT12_OK, "mount the artifact image");
	rc = fat12_read_fat(&art_vol, g_fat, sizeof(g_fat));
	CHECK(rc == FAT12_OK, "read the artifact FAT");
	fat_len = (uint32_t)art_vol.bpb.sectors_per_fat *
	          (uint32_t)art_vol.bpb.bytes_per_sector;

	/* Snapshot the OLD.TXT start_cluster + size + the whole FAT BEFORE the rename
	 * (so the name-field-only + FAT-untouched assertions are load-bearing). */
	slot = find_root_named(&art_vol, OLD_NAME, &old_start, &old_size);
	CHECK(slot >= 0, "OLD.TXT present in the artifact root before the rename");
	CHECK(old_start >= FAT12_FIRST_DATA_CLUSTER,
	      "OLD.TXT has a real data chain (start_cluster >= 2) -- the chain assertion bites");
	CHECK(old_size > 0u, "OLD.TXT is non-empty -- the size assertion bites");
	memcpy(g_fat_before, g_fat, sizeof(g_fat));

	rc = fat12_rename(&art_vol, g_fat, fat_len, OLD_NAME, NEW_NAME, 0u, g_sector);
	CHECK(rc == FAT12_OK, "fat12_rename(OLD.TXT -> NEW.TXT) in the root succeeds");

	/* (3) the OLD name no longer resolves; (2) NEW keeps start_cluster + size. */
	{
		uint16_t gone_start = 0xFFFFu; uint32_t gone_size = 0u;
		CHECK(find_root_named(&art_vol, OLD_NAME, &gone_start, &gone_size) < 0,
		      "after RENAME, OLD.TXT no longer resolves in the root");
	}
	uint16_t new_start = 0xFFFFu; uint32_t new_size = 0u;
	int new_slot = find_root_named(&art_vol, NEW_NAME, &new_start, &new_size);
	CHECK(new_slot >= 0, "after RENAME, NEW.TXT resolves in the root");
	CHECK(new_start == old_start,
	      "NEW.TXT start_cluster == OLD.TXT's (chain UNCHANGED; m2 bites here)");
	CHECK(new_size == old_size,
	      "NEW.TXT file_size == OLD.TXT's (size UNCHANGED; m2 bites here)");

	/* (4) the FAT (both copies, in-memory) is byte-UNCHANGED by the rename. */
	CHECK(memcmp(g_fat, g_fat_before, sizeof(g_fat)) == 0,
	      "the FAT is byte-unchanged by the rename (rename allocates/frees nothing)");

	/* ---- the GOLDEN (mren) image -- read-only ---------------------------- */
	rc = blockdev_file_open(&gold_bf, gold_img);
	CHECK(rc == 0, "open the mren golden image read-only");
	if (rc != 0) {
		blockdev_file_close(&art_bf);
		return TEST_SUMMARY("test_fat12_rename");
	}
	rc = fat12_mount(&gold_vol, &gold_bf.dev, g_sector);
	CHECK(rc == FAT12_OK, "mount the golden image");

	uint16_t gnew_start = 0xFFFFu; uint32_t gnew_size = 0u;
	int gold_slot = find_root_named(&gold_vol, NEW_NAME, &gnew_start, &gnew_size);
	CHECK(gold_slot >= 0, "NEW.TXT present in the mren golden root");
	CHECK(find_root_named(&gold_vol, OLD_NAME, NULL, NULL) < 0,
	      "OLD.TXT is gone from the mren golden root too");

	/* (1) THE differential: the NEW.TXT dir entry is byte-identical to mren's on
	 * the meaningful bytes (name/attr/start/size) after time-normalize. The m3
	 * (NAME_ONLY8) mutant leaves the artifact's extension stale -> the name field
	 * differs from mren's NEW.TXT -> this RED. */
	if (new_slot >= 0 && gold_slot >= 0) {
		uint8_t ae[32], ge[32];
		read_root_slot(&art_vol,  (uint32_t)new_slot,  ae);
		read_root_slot(&gold_vol, (uint32_t)gold_slot, ge);
		/* Spell out the name field so a failure points at the exact rule. */
		CHECK(memcmp(ae + 0x00, "NEW     ", 8) == 0,
		      "artifact NEW.BAK filename[0..7] == 'NEW     '");
		CHECK(memcmp(ae + 0x08, "BAK", 3) == 0,
		      "artifact NEW.BAK extension[0..2] == 'BAK' (m3 name-only8 bites here)");
		normalize_times(ae); normalize_times(ge);
		CHECK(memcmp(ae, ge, 32) == 0,
		      "NEW.BAK dir entry byte-identical to mren (name/attr/start/size)");
	}

	/* ---- in-process legs: missing source + dest-exists reject ------------ *
	 * A rename of a now-missing OLD.TXT -> NOT_FOUND; a rename of NEW.TXT onto
	 * itself-as-an-existing-dest (NEW.TXT -> NEW.TXT, dest present) -> EXISTS (the
	 * load-bearing dest-exists reject; m1 NO_DESTCHECK wrongly succeeds -> RED). */
	{
		rc = fat12_rename(&art_vol, g_fat, fat_len, OLD_NAME, "GONE.XYZ", 0u, g_sector);
		CHECK(rc == FAT12_ERR_NOT_FOUND,
		      "fat12_rename of a missing source -> FAT12_ERR_NOT_FOUND");

		rc = fat12_rename(&art_vol, g_fat, fat_len, NEW_NAME, NEW_NAME, 0u, g_sector);
		CHECK(rc == FAT12_ERR_EXISTS,
		      "fat12_rename onto an existing dest name -> FAT12_ERR_EXISTS (m1 bites)");
	}

	blockdev_file_close(&art_bf);
	blockdev_file_close(&gold_bf);
	return TEST_SUMMARY("test_fat12_rename");
}
