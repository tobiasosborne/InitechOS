/*
 * harness/diff/fat_diff/test_fat12_fault_rollback.c -- FAT12 WRITE-FAULT
 * rollback atomicity oracle (beads initech-lpf3; m0bp adversarial follow-up).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (Law 2: the oracle is the truth, never false-green).
 *
 * WHAT THIS PINS (the m0bp adversarial finding -- "the riskiest new function
 * had no fault-injection oracle"): the rollback legs in fat12_write_file,
 * fat12_create (via fat12_grow_dir), and fat12_mkdir (the flush-fail post-grow
 * leg) are STRUCTURALLY PRESENT but were unreachable by any existing oracle.
 * They only fire when an UNDERLYING device write fails MID-operation -- a real
 * disk-geometry exhaustion only reaches the NO_SPACE leg (fat12_find_free
 * returns 0, NO device write happens), which test-m0bp-rollback already covers.
 * Here we ARM the host backend's write-fault seam (blockdev_file_arm_write_fault,
 * beads initech-lpf3) to fail the Nth write_sectors -- exactly like a real
 * ata_write I/O error -- and assert that fat12.c rolls the on-disk state BACK so
 * NOTHING leaks: no orphan cluster, the FAT chain stays consistent, the dir
 * entry is not half-written, and the free-cluster count is exactly restored.
 *
 * This is a FAULT injection (a real device error reported loud, Rule 2), NOT a
 * compile-time mutant of fat12.c -- the artifact under test is byte-for-byte the
 * shipped fat12.c. The mutation-proof (Rule 6) is in the Makefile: each
 * scenario has a sibling build that DISABLES exactly the rollback it targets
 * (e.g. FAT12_MUTATE_WRITEFILE_NO_ROLLBACK) -> the matching CHECK goes RED.
 *
 * Three scenarios, each on its own freshly-prepared region of ONE blank image:
 *
 *   [A] fat12_write_file partial-allocation rollback (fat12.c ~2146): write a
 *       MULTI-cluster file; arm the fault to fail the 2nd cluster's data write
 *       AFTER the 1st cluster is committed. The call must return FAT12_ERR_WRITE
 *       and free EVERY cluster it claimed -- the free count is restored, no
 *       cluster is left allocated, the dir entry keeps start_cluster 0 / size 0.
 *
 *   [B] fat12_create full-subdir GROW rollback (fat12.c ~2012 -> fat12_grow_dir
 *       ~1175): fill a subdir's single cluster so CREATE must GROW it; arm the
 *       fault to fail the grow's zero-fill write. fat12_grow_dir frees the just-
 *       claimed cluster + re-flushes; CREATE returns the error. The subdir chain
 *       stays length 1, the claimed cluster is FREE again, free count restored.
 *
 *   [C] fat12_mkdir flush-fail POST-GROW rollback (fat12.c ~2918): fill a subdir
 *       so MKDIR must GROW the parent, then arm the fault to fail the FAT flush
 *       that commits the NEW dir's own-cluster EOC (the grow's own writes already
 *       succeeded + committed). fat12_mkdir frees newc AND shrinks the parent
 *       grow (fat12_shrink_dir_tail). The parent chain returns to length 1, both
 *       the appended parent cluster and newc are FREE again, free count restored.
 *
 * Each scenario ALSO asserts blockdev_file_write_faulted() -- a rollback test
 * whose fault never fired has proven nothing (Law 2). The image is RE-minted by
 * the Makefile each run (the test mutates it in place).
 *
 * Ref (Law 1): os/milton/blockdev.h write contract (0 ok / negative on error);
 *   the rollback discipline is fat12.c's own (Rule 2/Rule 3) -- fat12_write_file
 *   ~2092 "on a no-space failure mid-way, roll back every cluster claimed",
 *   fat12_grow_dir ~1169 "a failure rolls back the alloc", fat12_mkdir ~2918
 *   flush-fail leg (beads initech-m0bp rollback fix, the lpf3 follow-up). The
 *   Microsoft FAT spec (free=0x000, EOC 0xFF8..0xFFF). Image path is argv (the
 *   Makefile mints it) -> no host path baked in (Rule 11). ASCII (Rule 12).
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
static uint8_t g_cluster[512];

/* A multi-cluster body for scenario [A] (512 bytes/cluster on the floppy);
 * 1300 bytes spans 3 clusters, so the 2nd-cluster write fault rolls back >1. */
#define WRITE_MULTI_LEN 1300u

/* --- small FAT helpers (mirror the existing rollback oracle's idiom) --- */

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

/* Fill a subdir's single cluster: '.', '..', then 14 filler entries so the
 * cluster is FULL (16 slots) and the next CREATE/MKDIR must GROW it. Reads the
 * cluster's first slot to keep '.'/'..' intact, then plants the fillers. */
static int fill_subdir_cluster(fat12_volume_t *vol, uint16_t sub_cluster)
{
	uint32_t lba = BPB_CLUSTER_LBA(&vol->bpb, sub_cluster);
	uint8_t  sb[512];
	int      i;
	if (vol->dev->read_sectors(vol->dev->ctx, lba, 1u, sb) != 0) {
		return -1;
	}
	for (i = 0; i < 14; i++) {
		dir_entry_t f;
		char        nm[9];
		size_t      ln;
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
	if (vol->dev->write_sectors(vol->dev->ctx, lba, 1u, sb) != 0) {
		return -1;
	}
	return 0;
}

/* ===================================================================== */

int main(int argc, char **argv)
{
	const char     *img;
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint32_t        fat_len;
	int             rc;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <blank-fat12-image>\n", argv[0]);
		return 2;
	}
	img = argv[1];

	rc = blockdev_file_open_rw(&bf, img);
	CHECK(rc == 0, "open the blank FAT12 image read-write");
	if (rc != 0) return TEST_SUMMARY("test_fat12_fault_rollback");

	rc = fat12_mount(&vol, &bf.dev, g_sector);
	CHECK(rc == FAT12_OK, "mount the blank image");
	if (rc != FAT12_OK) { blockdev_file_close(&bf); return TEST_SUMMARY("test_fat12_fault_rollback"); }

	rc = fat12_read_fat(&vol, g_fat, sizeof(g_fat));
	CHECK(rc == FAT12_OK, "read the FAT");
	fat_len = (uint32_t)vol.bpb.sectors_per_fat * (uint32_t)vol.bpb.bytes_per_sector;

	/* =================================================================
	 * SCENARIO [A] -- fat12_write_file partial-allocation rollback.
	 * Create a root file, then write a 3-cluster body but arm the fault to
	 * fail the 2nd cluster's data write. Writes in fat12_write_file before
	 * the data writes: NONE (set_entry is in-memory only). So write_sectors
	 * #1 = cluster-0 data, #2 = cluster-1 data. Arm fault-at-2 -> the 2nd
	 * cluster write fails AFTER cluster 0 is committed -> rollback frees the
	 * whole partial chain (fat12_write_file ~2146-2153). =============== */
	{
		uint8_t      body[WRITE_MULTI_LEN];
		dir_entry_t  de;
		uint32_t     slot;
		uint32_t     free_before;
		uint32_t     i;
		dir_entry_t  re;

		for (i = 0u; i < WRITE_MULTI_LEN; i++) {
			body[i] = (uint8_t)((i * 31u + 7u) & 0xFFu);
		}

		rc = fat12_create(&vol, g_fat, fat_len, "FAULT.DAT", DIR_ATTR_ARCHIVE,
		                  0u, g_sector, g_cluster, &de, &slot);
		CHECK(rc == FAT12_OK, "[A] create FAULT.DAT in the root");

		free_before = free_count(&vol, fat_len);
		CHECK(free_before >= 3u, "[A] at least 3 free clusters before the write");

		/* Arm: fail the 2nd write_sectors (the 2nd cluster's data). */
		blockdev_file_arm_write_fault(&bf, 2u);
		rc = fat12_write_file(&vol, g_fat, fat_len, slot, body, WRITE_MULTI_LEN,
		                      g_sector, g_cluster);
		blockdev_file_arm_write_fault(&bf, 0u);   /* disarm */

		CHECK(rc == FAT12_ERR_WRITE,
		      "[A] fat12_write_file returns FAT12_ERR_WRITE on the injected fault");
		CHECK(blockdev_file_write_faulted(&bf),
		      "[A] the injected write fault ACTUALLY fired (else the test proves nothing)");

		/* ATOMICITY -- check the LIVE in-memory FAT WITHOUT re-reading from disk.
		 * fat12_write_file commits the FAT to disk only AFTER the whole chain is
		 * laid down (fat12.c ~2160), so a mid-write fault never reaches that flush
		 * -- the leak (if the rollback is skipped) lives in the in-memory FAT, the
		 * live allocation map the NEXT allocation consults. The rollback's job is
		 * to free that partial chain in memory (+ flush the already-clean disk).
		 * So the meaningful assertion is on g_fat as the call left it. */
		CHECK(free_count(&vol, fat_len) == free_before,
		      "[A] in-memory free-cluster count restored -- NO cluster leaked by the partial write");

		/* And the on-disk FAT must be untouched (the partial chain was never
		 * committed): re-read and re-confirm the free count matches. */
		rc = fat12_read_fat(&vol, g_fat, sizeof(g_fat));
		CHECK(rc == FAT12_OK, "[A] re-read the on-disk FAT after the failed write");
		CHECK(free_count(&vol, fat_len) == free_before,
		      "[A] on-disk free-cluster count unchanged (the partial chain was never flushed)");

		/* The dir entry must still be size 0 / cluster 0 (write_file patches the
		 * entry only on the success path; the rollback returns before that). */
		rc = fat12_find(&vol, g_sector, "FAULT.DAT", &re);
		CHECK(rc == FAT12_OK, "[A] FAULT.DAT entry still present after the failed write");
		CHECK(re.file_size == 0u && re.start_cluster == 0u,
		      "[A] FAULT.DAT entry NOT half-written (size 0, start_cluster 0)");
	}

	/* =================================================================
	 * SCENARIO [B] -- fat12_create full-subdir GROW rollback.
	 * Make '\BSUB', fill its single cluster (14 fillers), then CREATE a file
	 * in it: the subdir is full so CREATE must GROW it. fat12_grow_dir claims
	 * a cluster, marks it EOC (in-memory), then ZERO-FILLS it on disk -- that
	 * zero-fill is the FIRST write_sectors of the grow. Arm fault-at-1
	 * (counting from the arm) -> the zero-fill fails -> fat12_grow_dir frees
	 * the claimed cluster + flushes (fat12.c ~1177); CREATE returns the error.
	 * The subdir chain stays length 1, nothing leaks. ================== */
	{
		dir_entry_t  sub_de;
		uint16_t     bsub;
		uint32_t     free_before;
		dir_entry_t  de;
		uint32_t     slot;

		rc = fat12_mkdir(&vol, g_fat, fat_len, "BSUB", 0u, g_sector, g_cluster);
		CHECK(rc == FAT12_OK, "[B] mkdir \\BSUB");
		rc = fat12_read_fat(&vol, g_fat, sizeof(g_fat));
		CHECK(rc == FAT12_OK, "[B] re-read FAT after mkdir BSUB");
		rc = fat12_find(&vol, g_sector, "BSUB", &sub_de);
		CHECK(rc == FAT12_OK && sub_de.start_cluster >= FAT12_FIRST_DATA_CLUSTER,
		      "[B] BSUB present, has a data cluster");
		bsub = sub_de.start_cluster;

		rc = fill_subdir_cluster(&vol, bsub);
		CHECK(rc == 0, "[B] fill BSUB's single cluster (14 fillers -> full)");

		CHECK(chain_len(&vol, fat_len, bsub) == 1u,
		      "[B] BSUB is a single cluster before the grow");
		free_before = free_count(&vol, fat_len);

		/* Arm: fail the grow's zero-fill write (the 1st write after the arm). */
		blockdev_file_arm_write_fault(&bf, 1u);
		rc = fat12_create(&vol, g_fat, fat_len, "CHILD.TXT", DIR_ATTR_ARCHIVE,
		                  bsub, g_sector, g_cluster, &de, &slot);
		blockdev_file_arm_write_fault(&bf, 0u);   /* disarm */

		CHECK(rc == FAT12_ERR_WRITE,
		      "[B] fat12_create returns FAT12_ERR_WRITE when the GROW zero-fill faults");
		CHECK(blockdev_file_write_faulted(&bf),
		      "[B] the injected GROW-zero-fill fault ACTUALLY fired");

		/* ATOMICITY -- the LIVE in-memory FAT (no re-read). fat12_grow_dir marks
		 * the claimed cluster EOC in memory BEFORE the zero-fill; the rollback
		 * returns it to FREE (+ flushes the already-clean disk). Without the
		 * rollback that cluster LEAKS in the in-memory allocation map (EOC, but
		 * never linked into any chain) -- the next allocation loses it forever
		 * until remount. Assert on g_fat as the call left it. */
		CHECK(chain_len(&vol, fat_len, bsub) == 1u,
		      "[B] BSUB chain STILL length 1 -- the failed grow did not link a cluster");
		CHECK(free_count(&vol, fat_len) == free_before,
		      "[B] in-memory free-cluster count restored -- the claimed grow cluster did NOT leak");

		/* The on-disk FAT was never given the orphan (the grow never reached its
		 * link-then-flush), so a re-read also shows the count restored. */
		rc = fat12_read_fat(&vol, g_fat, sizeof(g_fat));
		CHECK(rc == FAT12_OK, "[B] re-read the on-disk FAT after the GROW rollback");
		CHECK(free_count(&vol, fat_len) == free_before,
		      "[B] on-disk free-cluster count unchanged after the failed grow");

		/* CHILD.TXT must NOT exist (CREATE failed before writing the entry). */
		{
			dir_entry_t miss;
			uint32_t    miss_slot;
			rc = fat12_find_slot_in(&vol, g_fat, fat_len, bsub, g_sector,
			                        "CHILD.TXT", &miss, &miss_slot);
			CHECK(rc != FAT12_OK,
			      "[B] CHILD.TXT NOT created (the entry was never written)");
		}
	}

	/* =================================================================
	 * SCENARIO [C] -- fat12_mkdir flush-fail POST-GROW rollback.
	 * Make '\CSUB', fill its single cluster, then MKDIR 'CNEW' with parent=CSUB:
	 * CSUB is full so MKDIR GROWS it. The grow's writes (zero-fill + the FAT
	 * flush inside fat12_grow_dir) SUCCEED + commit. Then fat12_mkdir claims
	 * newc, marks it EOC in memory, and calls fat12_flush_fats to commit it --
	 * THAT flush is the target. Write sequence after the arm: grow zero-fill
	 * (#1), grow's FAT flush = 2 FAT copies (#2, #3), then mkdir's own EOC
	 * flush = 2 FAT copies (#4, #5). Arm fault-at-4 -> the post-grow EOC flush
	 * fails -> fat12_mkdir frees newc AND shrinks the parent grow (fat12.c
	 * ~2918-2935). CSUB returns to length 1, nothing leaks. ============= */
	{
		dir_entry_t  sub_de;
		uint16_t     csub;
		uint32_t     free_before;

		rc = fat12_mkdir(&vol, g_fat, fat_len, "CSUB", 0u, g_sector, g_cluster);
		CHECK(rc == FAT12_OK, "[C] mkdir \\CSUB");
		rc = fat12_read_fat(&vol, g_fat, sizeof(g_fat));
		CHECK(rc == FAT12_OK, "[C] re-read FAT after mkdir CSUB");
		rc = fat12_find(&vol, g_sector, "CSUB", &sub_de);
		CHECK(rc == FAT12_OK && sub_de.start_cluster >= FAT12_FIRST_DATA_CLUSTER,
		      "[C] CSUB present, has a data cluster");
		csub = sub_de.start_cluster;

		rc = fill_subdir_cluster(&vol, csub);
		CHECK(rc == 0, "[C] fill CSUB's single cluster (14 fillers -> full)");

		CHECK(chain_len(&vol, fat_len, csub) == 1u,
		      "[C] CSUB is a single cluster before the grow");
		free_before = free_count(&vol, fat_len);

		/* Arm: fail the 4th write after the arm = the FIRST FAT copy of mkdir's
		 * own-EOC flush, AFTER the grow (zero-fill + its 2-copy FAT flush =
		 * writes 1..3) is fully committed. */
		blockdev_file_arm_write_fault(&bf, 4u);
		rc = fat12_mkdir(&vol, g_fat, fat_len, "CNEW", csub, g_sector, g_cluster);
		blockdev_file_arm_write_fault(&bf, 0u);   /* disarm */

		CHECK(rc != FAT12_OK,
		      "[C] fat12_mkdir fails when the post-grow own-EOC flush faults");
		CHECK(blockdev_file_write_faulted(&bf),
		      "[C] the injected post-grow flush fault ACTUALLY fired");

		rc = fat12_read_fat(&vol, g_fat, sizeof(g_fat));
		CHECK(rc == FAT12_OK, "[C] re-read the on-disk FAT after the post-grow rollback");
		CHECK(chain_len(&vol, fat_len, csub) == 1u,
		      "[C] CSUB chain BACK to length 1 -- the parent grow rolled back");
		CHECK(free_count(&vol, fat_len) == free_before,
		      "[C] free-cluster count restored -- neither the grow cluster nor newc leaked");

		/* CNEW must NOT exist in CSUB. */
		{
			dir_entry_t miss;
			uint32_t    miss_slot;
			rc = fat12_find_slot_in(&vol, g_fat, fat_len, csub, g_sector,
			                        "CNEW", &miss, &miss_slot);
			CHECK(rc != FAT12_OK, "[C] CNEW NOT created (the own-entry never landed)");
		}
	}

	blockdev_file_close(&bf);
	return TEST_SUMMARY("test_fat12_fault_rollback");
}
