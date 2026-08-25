/*
 * harness/diff/fat_diff/test_fat12_move.c -- FAT12 cross-directory MOVE oracle
 * (beads initech-tdnl.26; the R3 Finder's load-bearing MILTON dependency).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (CHECK / TEST_HARNESS / TEST_SUMMARY) -- non-zero exit on any failed
 * check (Law 2: the oracle is the truth, never false-green).
 *
 * WHAT THIS PINS. fat12_move_dirent is a DIRECTORY-ENTRY TRANSPLANT: the 32-byte
 * dirent is copied into a free slot of the destination directory, the source slot
 * is marked deleted, and NO FAT entry and NO data cluster is touched. The
 * load-bearing assertions are therefore NEGATIVE ones -- what the move did NOT
 * change -- captured by hand BEFORE each move and memcmp'd after:
 *   (a) start_cluster + file_size + attribute + mtime/mdate bit-for-bit equal;
 *   (b) the whole FAT region (BOTH on-disk copies, read raw off the device, plus
 *       the in-memory buffer) byte-equal;
 *   (c) the file's DATA CLUSTERS byte-equal (raw cluster reads, not just the
 *       decoded content) and its content still readable through the new entry.
 * Anything weaker would pass a move that quietly re-allocated the file.
 *
 * The scripted move set (argv[1], the DIFFERENTIAL image -- the Makefile verifies
 * it afterwards with mtools mdir/mtype from the HOST, against hand-authored
 * expectations, HER-02: nothing in the expectation is computed from the writer):
 *   [1] root   -> subdir : ::MOVEME.TXT      -> ::DST/MOVEME.TXT
 *   [2] subdir -> root   : ::SRC/SUBFILE.TXT -> ::SUBFILE.TXT
 *   [3] subdir -> subdir : ::SRC/CROSS.TXT   -> ::DST/CROSS.TXT
 *   [4] Trash stage      : ::SRC/REPORT.DBF  -> ::TRASH/REPOR001.DBF, the name
 *       chosen by fat12_trash_suffix_name because ::TRASH/REPORT.DBF exists
 *       (F1.4; transplant + name rewrite in ONE call)
 *   [5] DIRECTORY move   : ::SRC/MOVEDIR     -> ::DST/MOVEDIR, with the '..'
 *       fixup (MOVEDIR's own '..' must now name DST's cluster) and its child
 *       INSIDE.TXT still enumerable
 *   [6] EXISTS refusal   : ::COLL.TXT -> ::DST (which already holds COLL.TXT)
 *       must refuse and change NOTHING.
 *
 * The STRUCTURAL image (argv[2]) carries the legs that mutate the volume in ways
 * an mtools listing cannot express -- degenerate rejections, the destination
 * GROW, the fault-injected grow ROLLBACK, and finally a deliberately FULL root:
 *   [7] degenerate: src dir == dst dir -> FAT12_ERR_SAME_DIR; the volume label ->
 *       FAT12_ERR_ACCESS; "." / ".." -> FAT12_ERR_NOT_FOUND (parse_name83 refuses
 *       a dot name -- the entry-level dot guard in fat12.c is the second line of
 *       defense); a directory into its own subtree, and into ITSELF ->
 *       FAT12_ERR_CYCLE;
 *   [8] the suffix-helper ladder (free passthrough, 001, 002, extension kept,
 *       8.3 legality);
 *   [9] destination GROW: a FULL subdir destination grows by one cluster and the
 *       entry lands; then the SAME move with an injected write fault at the
 *       transplant write, and again at the source-delete write -- each must roll
 *       back completely (destination chain back to one cluster, free-cluster
 *       count exactly restored, the SOURCE entry still intact, the destination
 *       NOT holding the entry). The fault ordinals are SELF-CALIBRATED from the
 *       successful move's write count, so no fat12.c internal is hard-coded here;
 *  [10] a FULL fixed root -> FAT12_ERR_DIR_FULL (the root cannot grow).
 *
 * Mutants (Rule 6; the Makefile builds these with one perturbed seam each):
 *   FAT12_MUTATE_MOVE_RELINKS_CHAIN     -> start_cluster zeroed on the transplant
 *                                          -> the (a)/(c) equality legs RED;
 *   FAT12_MUTATE_MOVE_LOSES_ENTRY       -> destination write skipped, source still
 *                                          deleted -> the entry vanishes -> RED;
 *   FAT12_MUTATE_MOVE_NO_DOTDOT_FIX     -> a moved directory keeps its OLD '..'
 *                                          -> leg [5]'s '..' assertion RED;
 *   FAT12_MUTATE_MOVE_NO_GROW_ROLLBACK  -> the appended destination cluster leaks
 *                                          on a failed transplant -> leg [9] RED;
 *   FAT12_MUTATE_SUFFIX_NO_COLLIDE_CHECK-> the suffix helper hands back a taken
 *                                          name -> legs [4]/[8] RED.
 *
 * Ref (Law 1): docs/design/GUI-remediation-R3-finder-design.md F3.1 (the move
 *   primitive) + F1.4 (Trash staging, the collision suffix, and the
 *   "start_cluster is unchanged" load-bearing assertion) + Sec 0.4; the
 *   constraints carried verbatim from fat12_rename; docs/research/
 *   fat12-ground-truth.md Sec 3/4; spec/dos_structs.h. Image paths are argv (the
 *   Makefile mints them) -> no host path baked in (Rule 11). ASCII (Rule 12).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "test_assert.h"      /* seed/, on -Iseed           */
#include "fat12.h"            /* os/milton/, on -Ios/milton */
#include "blockdev_file.h"    /* fat_diff host backend       */

TEST_HARNESS();

#define FATBUF_BYTES (12u * 512u)

static uint8_t g_fat[FATBUF_BYTES];
static uint8_t g_fat_before[FATBUF_BYTES];
static uint8_t g_disk_fat_before[FATBUF_BYTES * 2u];   /* BOTH on-disk copies */
static uint8_t g_disk_fat_after[FATBUF_BYTES * 2u];
static uint8_t g_sector[512];
static uint8_t g_cluster[512];
static uint8_t g_body_before[4096];
static uint8_t g_body_after[4096];
static uint8_t g_clusters_before[4096];
static uint8_t g_clusters_after[4096];

/* ------------------------------------------------------------------ *
 * Small read-side helpers (the existing fat_diff oracles' idiom).
 * ------------------------------------------------------------------ */

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

/* Read BOTH on-disk FAT copies raw off the device into `out` (num_fats * spf
 * sectors). This is stronger than diffing the in-memory buffer: it proves the
 * move issued no FAT write at all. */
static int snapshot_disk_fats(const fat12_volume_t *vol, uint8_t *out,
                              uint32_t out_len, uint32_t *out_bytes)
{
	uint32_t spf   = vol->bpb.sectors_per_fat;
	uint32_t total = spf * (uint32_t)vol->bpb.num_fats;
	uint32_t s;
	if (total * 512u > out_len) return -1;
	for (s = 0u; s < total; s++) {
		if (vol->dev->read_sectors(vol->dev->ctx, vol->first_fat_sector + s,
		                           1u, out + s * 512u) != 0) {
			return -1;
		}
	}
	*out_bytes = total * 512u;
	return 0;
}

/* Read the RAW bytes of every cluster in `start`'s chain into `out` (so the
 * "no data cluster was touched" assertion compares disk bytes, not a decode). */
static int snapshot_chain_bytes(const fat12_volume_t *vol, uint32_t fat_len,
                                uint16_t start, uint8_t *out, uint32_t out_len,
                                uint32_t *out_bytes)
{
	uint32_t bpc = (uint32_t)vol->bpb.sectors_per_cluster * 512u;
	uint16_t cur = start;
	uint32_t n   = 0u;
	uint32_t max = vol->total_clusters + 2u;
	uint32_t steps = 0u;

	if (start < FAT12_FIRST_DATA_CLUSTER) { *out_bytes = 0u; return 0; }
	for (;;) {
		uint16_t next;
		uint32_t lba = BPB_CLUSTER_LBA(&vol->bpb, cur);
		uint32_t s;
		if (n + bpc > out_len) return -1;
		for (s = 0u; s < vol->bpb.sectors_per_cluster; s++) {
			if (vol->dev->read_sectors(vol->dev->ctx, lba + s, 1u,
			                           out + n + s * 512u) != 0) {
				return -1;
			}
		}
		n += bpc;
		if (fat12_next_cluster(vol, g_fat, fat_len, cur, &next) != FAT12_OK) return -1;
		if (fat12_is_eoc(next)) break;
		cur = next;
		if (++steps > max) return -1;
	}
	*out_bytes = n;
	return 0;
}

/* Locate a directory's first data cluster by name in the root (0 == root). */
static uint16_t dir_cluster(const fat12_volume_t *vol, const char *name83)
{
	dir_entry_t de;
	if (fat12_find(vol, g_sector, name83, &de) != FAT12_OK) return 0u;
	return de.start_cluster;
}

/* Look an 8.3 name up in the directory at `dir_start`; 1 == present. */
static int name_present(const fat12_volume_t *vol, uint32_t fat_len,
                        uint16_t dir_start, const char *name83,
                        dir_entry_t *out)
{
	dir_entry_t de;
	uint32_t    slot;
	int rc = fat12_find_slot_in(vol, g_fat, fat_len, dir_start, g_sector,
	                            name83, &de, &slot);
	if (rc != FAT12_OK) return 0;
	if (out != NULL) *out = de;
	return 1;
}

/* Compare the MEANINGFUL, must-be-preserved bytes of two dir entries: the
 * chain head, size, attribute and BOTH packed timestamps. The name field is
 * excluded on purpose (a move may rename). */
static int entry_payload_equal(const dir_entry_t *a, const dir_entry_t *b)
{
	return (a->start_cluster == b->start_cluster) &&
	       (a->file_size     == b->file_size)     &&
	       (a->attribute     == b->attribute)     &&
	       (a->mtime         == b->mtime)         &&
	       (a->mdate         == b->mdate);
}

/* Fill a subdir's single cluster: '.', '..', then 14 filler entries so the
 * cluster is FULL (16 slots) and the next placement must GROW it. */
static int fill_subdir_cluster(fat12_volume_t *vol, uint16_t sub_cluster)
{
	uint32_t lba = BPB_CLUSTER_LBA(&vol->bpb, sub_cluster);
	uint8_t  sb[512];
	int      i;
	if (vol->dev->read_sectors(vol->dev->ctx, lba, 1u, sb) != 0) return -1;
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
		f.attribute     = 0x20u;
		f.start_cluster = 0u;
		f.file_size     = 0u;
		memcpy(sb + (2 + i) * 32, &f, 32);
	}
	if (vol->dev->write_sectors(vol->dev->ctx, lba, 1u, sb) != 0) return -1;
	return 0;
}

/* Fill EVERY root-directory slot with a distinct clusterless filler entry, so
 * the fixed root is genuinely FULL (it cannot grow -- FAT12_ERR_DIR_FULL). This
 * destroys the root's contents, so it runs as the LAST leg of its image. */
static int fill_root_dir(fat12_volume_t *vol)
{
	uint32_t per_sector = 512u / 32u;
	uint32_t s;
	uint32_t idx = 0u;
	for (s = 0u; s < vol->root_dir_sectors; s++) {
		uint8_t  sb[512];
		uint32_t i;
		for (i = 0u; i < per_sector; i++, idx++) {
			dir_entry_t f;
			char        nm[9];
			size_t      ln;
			memset(&f, 0, sizeof(f));
			snprintf(nm, sizeof(nm), "RFIL%03u", (unsigned)(idx % 1000u));
			ln = strlen(nm);
			memset(f.filename, ' ', 8);
			memcpy(f.filename, nm, ln < 8 ? ln : 8);
			memset(f.extension, ' ', 3);
			f.attribute     = 0x20u;
			f.start_cluster = 0u;
			f.file_size     = 0u;
			memcpy(sb + i * 32u, &f, 32);
		}
		if (vol->dev->write_sectors(vol->dev->ctx, vol->root_dir_sector + s,
		                            1u, sb) != 0) {
			return -1;
		}
	}
	return 0;
}

/* ------------------------------------------------------------------ *
 * THE load-bearing move check: capture the entry payload, BOTH on-disk FAT
 * copies, the in-memory FAT and the file's raw data clusters BEFORE the move;
 * move; then prove all four are unchanged and the content still reads back.
 * `tag` names the leg in the CHECK messages.
 * ------------------------------------------------------------------ */
static void move_and_prove(fat12_volume_t *vol, uint32_t fat_len,
                           const char *tag,
                           const char *src_name, uint16_t src_dir,
                           const char *dst_name, uint16_t dst_dir)
{
	dir_entry_t before, after;
	uint32_t    disk_before_len = 0u, disk_after_len = 0u;
	uint32_t    clus_before_len = 0u, clus_after_len = 0u;
	uint32_t    body_before_len = 0u, body_after_len = 0u;
	const char *final_name = (dst_name != NULL && dst_name[0] != '\0')
	                         ? dst_name : src_name;
	int         rc;
	int         is_dir;
	char        msg[160];

	snprintf(msg, sizeof(msg), "[%s] source entry present before the move", tag);
	if (!name_present(vol, fat_len, src_dir, src_name, &before)) {
		CHECK(0, msg);
		return;
	}
	CHECK(1, msg);
	is_dir = ((before.attribute & DIR_ATTR_DIRECTORY) != 0u);

	CHECK(snapshot_disk_fats(vol, g_disk_fat_before, sizeof(g_disk_fat_before),
	                         &disk_before_len) == 0,
	      "snapshot both on-disk FAT copies before the move");
	memcpy(g_fat_before, g_fat, sizeof(g_fat));
	CHECK(snapshot_chain_bytes(vol, fat_len, before.start_cluster,
	                           g_clusters_before, sizeof(g_clusters_before),
	                           &clus_before_len) == 0,
	      "snapshot the source's raw data clusters before the move");
	if (!is_dir && before.file_size > 0u &&
	    before.file_size <= sizeof(g_body_before)) {
		CHECK(fat12_read_file(vol, g_fat, fat_len, &before, g_body_before,
		                      sizeof(g_body_before), g_cluster,
		                      &body_before_len) == FAT12_OK,
		      "read the file's content before the move");
	}

	rc = fat12_move_dirent(vol, g_fat, fat_len, src_name, src_dir,
	                       dst_name, dst_dir, g_sector, g_cluster);
	snprintf(msg, sizeof(msg), "[%s] fat12_move_dirent succeeds", tag);
	CHECK(rc == FAT12_OK, msg);
	if (rc != FAT12_OK) return;

	snprintf(msg, sizeof(msg),
	         "[%s] the entry is GONE from the source directory", tag);
	CHECK(!name_present(vol, fat_len, src_dir, src_name, NULL), msg);

	snprintf(msg, sizeof(msg),
	         "[%s] the entry is PRESENT in the destination directory "
	         "(MOVE_LOSES_ENTRY bites here)", tag);
	if (!name_present(vol, fat_len, dst_dir, final_name, &after)) {
		CHECK(0, msg);
		return;
	}
	CHECK(1, msg);

	/* (a) the payload bytes are preserved bit-for-bit. */
	snprintf(msg, sizeof(msg),
	         "[%s] start_cluster/size/attr/mtime/mdate preserved bit-for-bit "
	         "(MOVE_RELINKS_CHAIN bites here)", tag);
	CHECK(entry_payload_equal(&before, &after), msg);

	/* (b) neither on-disk FAT copy, nor the in-memory FAT, changed. */
	CHECK(snapshot_disk_fats(vol, g_disk_fat_after, sizeof(g_disk_fat_after),
	                         &disk_after_len) == 0,
	      "snapshot both on-disk FAT copies after the move");
	snprintf(msg, sizeof(msg),
	         "[%s] BOTH on-disk FAT copies byte-unchanged (a move touches no "
	         "FAT entry)", tag);
	CHECK(disk_before_len == disk_after_len &&
	      memcmp(g_disk_fat_before, g_disk_fat_after, disk_before_len) == 0, msg);
	snprintf(msg, sizeof(msg), "[%s] the in-memory FAT is byte-unchanged", tag);
	CHECK(memcmp(g_fat, g_fat_before, sizeof(g_fat)) == 0, msg);

	/* (c) the data clusters are byte-identical, and the content still reads. */
	CHECK(snapshot_chain_bytes(vol, fat_len, after.start_cluster,
	                           g_clusters_after, sizeof(g_clusters_after),
	                           &clus_after_len) == 0,
	      "snapshot the moved entry's raw data clusters after the move");
	if (is_dir) {
		/* A moved DIRECTORY is the ONE case where the entry's own cluster
		 * legitimately changes: its '..' must be repointed at the new parent.
		 * State that EXACTLY rather than exempting the directory from the check
		 * -- patch the expected two bytes (start_cluster at 0x1A of slot 1, i.e.
		 * offset 32+26) into the BEFORE snapshot and demand byte-equality
		 * everywhere else. Two bytes, and only those two. */
		if (clus_before_len >= 64u) {
			g_clusters_before[32u + 26u] = (uint8_t)(dst_dir & 0xFFu);
			g_clusters_before[32u + 27u] = (uint8_t)((dst_dir >> 8) & 0xFFu);
		}
		snprintf(msg, sizeof(msg),
		         "[%s] the directory's clusters are byte-identical EXCEPT the two "
		         "'..' start_cluster bytes (nothing else was rewritten)", tag);
	} else {
		snprintf(msg, sizeof(msg),
		         "[%s] the data clusters are byte-identical (no cluster rewritten)",
		         tag);
	}
	CHECK(clus_before_len == clus_after_len &&
	      memcmp(g_clusters_before, g_clusters_after, clus_before_len) == 0, msg);

	if (!is_dir && before.file_size > 0u &&
	    before.file_size <= sizeof(g_body_after)) {
		CHECK(fat12_read_file(vol, g_fat, fat_len, &after, g_body_after,
		                      sizeof(g_body_after), g_cluster,
		                      &body_after_len) == FAT12_OK,
		      "read the file's content back through the moved entry");
		snprintf(msg, sizeof(msg),
		         "[%s] the content read through the NEW entry is identical", tag);
		CHECK(body_before_len == body_after_len &&
		      memcmp(g_body_before, g_body_after, body_before_len) == 0, msg);
	}
}

/* ================================================================== *
 * IMAGE 1 -- the scripted move set the Makefile then verifies with mtools.
 * ================================================================== */
static void run_differential_image(const char *img)
{
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint32_t        fat_len;
	uint16_t        src_c, dst_c, trash_c, movedir_c;
	int             rc;

	rc = blockdev_file_open_rw(&bf, img);
	CHECK(rc == 0, "open the differential image read-write");
	if (rc != 0) return;
	rc = fat12_mount(&vol, &bf.dev, g_sector);
	CHECK(rc == FAT12_OK, "mount the differential image");
	if (rc != FAT12_OK) { blockdev_file_close(&bf); return; }
	rc = fat12_read_fat(&vol, g_fat, sizeof(g_fat));
	CHECK(rc == FAT12_OK, "read the differential image FAT");
	fat_len = (uint32_t)vol.bpb.sectors_per_fat *
	          (uint32_t)vol.bpb.bytes_per_sector;

	src_c   = dir_cluster(&vol, "SRC");
	dst_c   = dir_cluster(&vol, "DST");
	trash_c = dir_cluster(&vol, "TRASH");
	CHECK(src_c >= FAT12_FIRST_DATA_CLUSTER &&
	      dst_c >= FAT12_FIRST_DATA_CLUSTER &&
	      trash_c >= FAT12_FIRST_DATA_CLUSTER,
	      "SRC / DST / TRASH resolve to real data clusters");
	if (src_c < FAT12_FIRST_DATA_CLUSTER || dst_c < FAT12_FIRST_DATA_CLUSTER ||
	    trash_c < FAT12_FIRST_DATA_CLUSTER) {
		blockdev_file_close(&bf);
		return;
	}

	/* [1] root -> subdir; [2] subdir -> root; [3] subdir -> subdir. */
	move_and_prove(&vol, fat_len, "root->sub",  "MOVEME.TXT",  0u,    NULL, dst_c);
	move_and_prove(&vol, fat_len, "sub->root",  "SUBFILE.TXT", src_c, NULL, 0u);
	move_and_prove(&vol, fat_len, "sub->sub",   "CROSS.TXT",   src_c, NULL, dst_c);

	/* [4] the Trash stage: the suffix helper picks the name, the move carries the
	 * rename in the SAME call (F1.4 "collide-check -> suffix -> transplant"). */
	{
		char staged[FAT12_NAME83_MAX];
		memset(staged, 0, sizeof(staged));
		rc = fat12_trash_suffix_name(&vol, g_fat, fat_len, trash_c, g_sector,
		                             "REPORT.DBF", staged);
		CHECK(rc == FAT12_OK, "[trash] fat12_trash_suffix_name succeeds");
		CHECK(strcmp(staged, "REPOR001.DBF") == 0,
		      "[trash] REPORT.DBF taken in TRASH -> REPOR001.DBF "
		      "(trunc5 + %03u, lowest free first; SUFFIX_NO_COLLIDE bites here)");
		move_and_prove(&vol, fat_len, "trash", "REPORT.DBF", src_c,
		               staged, trash_c);
		CHECK(name_present(&vol, fat_len, trash_c, "REPORT.DBF", NULL),
		      "[trash] the ORIGINAL TRASH/REPORT.DBF is untouched by the staging");
	}

	/* [5] the DIRECTORY move + the '..' fixup. */
	movedir_c = 0u;
	{
		dir_entry_t de;
		if (name_present(&vol, fat_len, src_c, "MOVEDIR", &de)) {
			movedir_c = de.start_cluster;
		}
		CHECK(movedir_c >= FAT12_FIRST_DATA_CLUSTER,
		      "[dirmove] SRC/MOVEDIR resolves to a real data cluster");
	}
	if (movedir_c >= FAT12_FIRST_DATA_CLUSTER) {
		dir_entry_t dd;
		CHECK(fat12_read_dir_entry_in(&vol, g_fat, fat_len, movedir_c, g_sector,
		                              1u, &dd) == FAT12_OK,
		      "[dirmove] read MOVEDIR's '..' entry before the move");
		CHECK(dd.filename[0] == 0x2Eu && dd.filename[1] == 0x2Eu,
		      "[dirmove] slot 1 of MOVEDIR really is '..'");
		CHECK(dd.start_cluster == src_c,
		      "[dirmove] MOVEDIR's '..' names SRC before the move");

		move_and_prove(&vol, fat_len, "dirmove", "MOVEDIR", src_c, NULL, dst_c);

		CHECK(fat12_read_dir_entry_in(&vol, g_fat, fat_len, movedir_c, g_sector,
		                              1u, &dd) == FAT12_OK,
		      "[dirmove] read MOVEDIR's '..' entry after the move");
		CHECK(dd.start_cluster == dst_c,
		      "[dirmove] MOVEDIR's '..' now names DST -- the '..' fixup "
		      "(MOVE_NO_DOTDOT_FIX bites here)");
		CHECK(dd.filename[0] == 0x2Eu && dd.filename[1] == 0x2Eu &&
		      dd.attribute == DIR_ATTR_DIRECTORY,
		      "[dirmove] the '..' entry's name + attr are untouched by the fixup");
		CHECK(name_present(&vol, fat_len, movedir_c, "INSIDE.TXT", NULL),
		      "[dirmove] the moved folder's child INSIDE.TXT is still there");
	}

	/* [6] the EXISTS refusal: the destination already holds COLL.TXT. Nothing
	 * anywhere may change (a move never clobbers -- fat12_rename's contract). */
	{
		dir_entry_t src_before, dst_before, src_after, dst_after;
		uint32_t    disk_len_a = 0u, disk_len_b = 0u;

		CHECK(name_present(&vol, fat_len, 0u, "COLL.TXT", &src_before),
		      "[exists] root COLL.TXT present before the refused move");
		CHECK(name_present(&vol, fat_len, dst_c, "COLL.TXT", &dst_before),
		      "[exists] DST/COLL.TXT present before the refused move");
		CHECK(snapshot_disk_fats(&vol, g_disk_fat_before,
		                         sizeof(g_disk_fat_before), &disk_len_a) == 0,
		      "[exists] snapshot the on-disk FATs before the refused move");

		rc = fat12_move_dirent(&vol, g_fat, fat_len, "COLL.TXT", 0u,
		                       NULL, dst_c, g_sector, g_cluster);
		CHECK(rc == FAT12_ERR_EXISTS,
		      "[exists] a move onto an existing destination name -> "
		      "FAT12_ERR_EXISTS (a move never clobbers)");

		CHECK(name_present(&vol, fat_len, 0u, "COLL.TXT", &src_after),
		      "[exists] the source entry survives the refusal");
		CHECK(name_present(&vol, fat_len, dst_c, "COLL.TXT", &dst_after),
		      "[exists] the destination entry survives the refusal");
		CHECK(entry_payload_equal(&src_before, &src_after) &&
		      entry_payload_equal(&dst_before, &dst_after),
		      "[exists] both entries' payload bytes are unchanged");
		CHECK(snapshot_disk_fats(&vol, g_disk_fat_after,
		                         sizeof(g_disk_fat_after), &disk_len_b) == 0,
		      "[exists] snapshot the on-disk FATs after the refused move");
		CHECK(disk_len_a == disk_len_b &&
		      memcmp(g_disk_fat_before, g_disk_fat_after, disk_len_a) == 0,
		      "[exists] the FAT is byte-unchanged by the refusal");
	}

	blockdev_file_close(&bf);
}

/* ================================================================== *
 * IMAGE 2 -- degenerate rejections, the suffix ladder, the GROW + its
 * fault-injected ROLLBACK, and the FULL fixed root.
 * ================================================================== */
static void run_structural_image(const char *img)
{
	blockdev_file_t bf;
	fat12_volume_t  vol;
	uint32_t        fat_len;
	uint16_t        suba, subb, subc, holder, parent, child;
	uint32_t        calibrated_writes = 0u;
	int             rc;

	rc = blockdev_file_open_rw(&bf, img);
	CHECK(rc == 0, "open the structural image read-write");
	if (rc != 0) return;
	rc = fat12_mount(&vol, &bf.dev, g_sector);
	CHECK(rc == FAT12_OK, "mount the structural image");
	if (rc != FAT12_OK) { blockdev_file_close(&bf); return; }
	rc = fat12_read_fat(&vol, g_fat, sizeof(g_fat));
	CHECK(rc == FAT12_OK, "read the structural image FAT");
	fat_len = (uint32_t)vol.bpb.sectors_per_fat *
	          (uint32_t)vol.bpb.bytes_per_sector;

	suba   = dir_cluster(&vol, "SUBA");
	subb   = dir_cluster(&vol, "SUBB");
	subc   = dir_cluster(&vol, "SUBC");
	holder = dir_cluster(&vol, "HOLDER");
	parent = dir_cluster(&vol, "PARENT");
	CHECK(suba >= 2u && subb >= 2u && subc >= 2u && holder >= 2u && parent >= 2u,
	      "SUBA / SUBB / SUBC / HOLDER / PARENT resolve to real data clusters");
	child = 0u;
	{
		dir_entry_t de;
		if (name_present(&vol, fat_len, parent, "CHILD", &de)) {
			child = de.start_cluster;
		}
		CHECK(child >= FAT12_FIRST_DATA_CLUSTER,
		      "PARENT/CHILD resolves to a real data cluster");
	}

	/* ---- [7] degenerate rejections ---------------------------------- */
	rc = fat12_move_dirent(&vol, g_fat, fat_len, "FILEA.TXT", 0u, NULL, 0u,
	                       g_sector, g_cluster);
	CHECK(rc == FAT12_ERR_SAME_DIR,
	      "[degen] src dir == dst dir -> FAT12_ERR_SAME_DIR (a same-dir move is "
	      "a rename's job, not a silent no-op)");

	rc = fat12_move_dirent(&vol, g_fat, fat_len, "INITECH", 0u, NULL, holder,
	                       g_sector, g_cluster);
	CHECK(rc == FAT12_ERR_ACCESS,
	      "[degen] moving the VOLUME LABEL -> FAT12_ERR_ACCESS");

	rc = fat12_move_dirent(&vol, g_fat, fat_len, ".", suba, NULL, 0u,
	                       g_sector, g_cluster);
	CHECK(rc == FAT12_ERR_NOT_FOUND,
	      "[degen] moving '.' -> refused (parse_name83 rejects a dot name)");
	rc = fat12_move_dirent(&vol, g_fat, fat_len, "..", suba, NULL, 0u,
	                       g_sector, g_cluster);
	CHECK(rc == FAT12_ERR_NOT_FOUND,
	      "[degen] moving '..' -> refused (parse_name83 rejects a dot name)");

	rc = fat12_move_dirent(&vol, g_fat, fat_len, "PARENT", 0u, NULL, child,
	                       g_sector, g_cluster);
	CHECK(rc == FAT12_ERR_CYCLE,
	      "[degen] moving a directory INTO ITS OWN SUBTREE -> FAT12_ERR_CYCLE");
	rc = fat12_move_dirent(&vol, g_fat, fat_len, "PARENT", 0u, NULL, parent,
	                       g_sector, g_cluster);
	CHECK(rc == FAT12_ERR_CYCLE,
	      "[degen] moving a directory INTO ITSELF -> FAT12_ERR_CYCLE");
	CHECK(name_present(&vol, fat_len, 0u, "PARENT", NULL) &&
	      name_present(&vol, fat_len, parent, "CHILD", NULL),
	      "[degen] the refused cycle moves changed nothing");

	/* ---- [8] the suffix-helper ladder ------------------------------- */
	{
		char out[FAT12_NAME83_MAX];

		memset(out, 0, sizeof(out));
		CHECK(fat12_trash_suffix_name(&vol, g_fat, fat_len, holder, g_sector,
		                              "NOTHERE.TXT", out) == FAT12_OK,
		      "[suffix] a FREE name is accepted");
		CHECK(strcmp(out, "NOTHERE.TXT") == 0,
		      "[suffix] a free name passes through canonicalized, unsuffixed");

		memset(out, 0, sizeof(out));
		CHECK(fat12_trash_suffix_name(&vol, g_fat, fat_len, holder, g_sector,
		                              "keep.txt", out) == FAT12_OK,
		      "[suffix] a colliding name is suffixed");
		CHECK(strcmp(out, "KEEP001.TXT") == 0,
		      "[suffix] KEEP.TXT taken -> KEEP001.TXT (base < 5 chars kept whole; "
		      "lower-case input canonicalized; extension preserved)");

		/* Stage KEEP.TXT into HOLDER twice: the second must climb to 002. */
		rc = fat12_move_dirent(&vol, g_fat, fat_len, "KEEPA.TXT", 0u,
		                       "KEEP001.TXT", holder, g_sector, g_cluster);
		CHECK(rc == FAT12_OK, "[suffix] stage a second KEEP as KEEP001.TXT");
		memset(out, 0, sizeof(out));
		CHECK(fat12_trash_suffix_name(&vol, g_fat, fat_len, holder, g_sector,
		                              "KEEP.TXT", out) == FAT12_OK,
		      "[suffix] the ladder climbs when 001 is taken too");
		CHECK(strcmp(out, "KEEP002.TXT") == 0,
		      "[suffix] KEEP.TXT and KEEP001.TXT taken -> KEEP002.TXT "
		      "(lowest FREE counter, not next-after-last)");

		/* 8.3 legality of a long base: REPORTS.DAT -> REPOR001.DAT. */
		memset(out, 0, sizeof(out));
		CHECK(fat12_trash_suffix_name(&vol, g_fat, fat_len, holder, g_sector,
		                              "LONGBASE.DAT", out) == FAT12_OK,
		      "[suffix] a free 8-char base passes through");
		CHECK(strcmp(out, "LONGBASE.DAT") == 0,
		      "[suffix] the free 8-char base is unchanged");
		rc = fat12_move_dirent(&vol, g_fat, fat_len, "LONGBASE.DAT", 0u, NULL,
		                       holder, g_sector, g_cluster);
		CHECK(rc == FAT12_OK, "[suffix] stage LONGBASE.DAT into HOLDER");
		memset(out, 0, sizeof(out));
		CHECK(fat12_trash_suffix_name(&vol, g_fat, fat_len, holder, g_sector,
		                              "LONGBASE.DAT", out) == FAT12_OK,
		      "[suffix] the long base collides and is truncated");
		CHECK(strcmp(out, "LONGB001.DAT") == 0,
		      "[suffix] LONGBASE.DAT taken -> LONGB001.DAT (trunc5 + %03u)");
		{
			const char *dot = strchr(out, '.');
			CHECK(dot != NULL && (dot - out) <= 8 && strlen(dot + 1) <= 3 &&
			      strlen(out) <= 12,
			      "[suffix] the suffixed name is 8.3-legal (base <= 8, ext <= 3, "
			      "one dot)");
		}
	}

	/* ---- [9] destination GROW + the fault-injected ROLLBACK ---------- *
	 * SUBA / SUBB / SUBC are prepared IDENTICALLY (single cluster, all 16 slots
	 * occupied) so the successful move's write count calibrates the fault
	 * ordinals for the other two -- no fat12.c internal is hard-coded here. */
	CHECK(fill_subdir_cluster(&vol, suba) == 0, "[grow] fill SUBA's only cluster");
	CHECK(fill_subdir_cluster(&vol, subb) == 0, "[grow] fill SUBB's only cluster");
	CHECK(fill_subdir_cluster(&vol, subc) == 0, "[grow] fill SUBC's only cluster");

	{
		dir_entry_t before, after;
		uint32_t    free_before = free_count(&vol, fat_len);

		CHECK(name_present(&vol, fat_len, 0u, "GROWA.TXT", &before),
		      "[grow] GROWA.TXT present in the root before the growing move");
		CHECK(chain_len(&vol, fat_len, suba) == 1u,
		      "[grow] SUBA is one cluster before the growing move");

		bf.write_calls = 0u;
		rc = fat12_move_dirent(&vol, g_fat, fat_len, "GROWA.TXT", 0u, NULL,
		                       suba, g_sector, g_cluster);
		calibrated_writes = bf.write_calls;
		CHECK(rc == FAT12_OK,
		      "[grow] a move into a FULL subdir GROWS it and succeeds");
		CHECK(chain_len(&vol, fat_len, suba) == 2u,
		      "[grow] SUBA now spans two clusters");
		CHECK(name_present(&vol, fat_len, suba, "GROWA.TXT", &after),
		      "[grow] the entry landed in the grown cluster");
		CHECK(entry_payload_equal(&before, &after),
		      "[grow] the grown-destination move preserved the payload bytes");
		CHECK(free_count(&vol, fat_len) == free_before - 1u,
		      "[grow] exactly ONE cluster was consumed (the directory extension)");
		CHECK(calibrated_writes >= 3u,
		      "[grow] the successful growing move issued a calibratable write "
		      "sequence");
	}

	/* [9a] fault the TRANSPLANT write (the last-but-one write of the sequence):
	 * the grow is committed, then the dirent write fails -> everything unwinds. */
	if (calibrated_writes >= 2u) {
		uint32_t free_before = free_count(&vol, fat_len);
		dir_entry_t keep;

		CHECK(name_present(&vol, fat_len, 0u, "GROWB.TXT", &keep),
		      "[rollback-a] GROWB.TXT present in the root before the faulted move");
		blockdev_file_arm_write_fault(&bf, calibrated_writes - 1u);
		rc = fat12_move_dirent(&vol, g_fat, fat_len, "GROWB.TXT", 0u, NULL,
		                       subb, g_sector, g_cluster);
		blockdev_file_arm_write_fault(&bf, 0u);
		CHECK(rc != FAT12_OK,
		      "[rollback-a] a write fault at the transplant fails the move");
		CHECK(blockdev_file_write_faulted(&bf),
		      "[rollback-a] the injected fault actually FIRED (a rollback test "
		      "whose fault never fired proves nothing)");
		CHECK(chain_len(&vol, fat_len, subb) == 1u,
		      "[rollback-a] SUBB is back to ONE cluster -- the grow was rolled "
		      "back (MOVE_NO_GROW_ROLLBACK bites here)");
		CHECK(free_count(&vol, fat_len) == free_before,
		      "[rollback-a] the free-cluster count is exactly restored");
		CHECK(name_present(&vol, fat_len, 0u, "GROWB.TXT", NULL),
		      "[rollback-a] the SOURCE entry is intact -- a failed move loses "
		      "nothing");
		CHECK(!name_present(&vol, fat_len, subb, "GROWB.TXT", NULL),
		      "[rollback-a] the destination did NOT gain the entry");
	}

	/* [9b] fault the SOURCE-DELETE write (the last write): the destination entry
	 * IS on disk when the fault fires, so the rollback must un-write it too --
	 * otherwise the entry would exist twice, two directories on one chain. */
	if (calibrated_writes >= 1u) {
		uint32_t free_before = free_count(&vol, fat_len);
		dir_entry_t keep;

		CHECK(name_present(&vol, fat_len, 0u, "GROWC.TXT", &keep),
		      "[rollback-b] GROWC.TXT present in the root before the faulted move");
		blockdev_file_arm_write_fault(&bf, calibrated_writes);
		rc = fat12_move_dirent(&vol, g_fat, fat_len, "GROWC.TXT", 0u, NULL,
		                       subc, g_sector, g_cluster);
		blockdev_file_arm_write_fault(&bf, 0u);
		CHECK(rc != FAT12_OK,
		      "[rollback-b] a write fault at the source-delete fails the move");
		CHECK(blockdev_file_write_faulted(&bf),
		      "[rollback-b] the injected fault actually FIRED");
		CHECK(name_present(&vol, fat_len, 0u, "GROWC.TXT", NULL),
		      "[rollback-b] the SOURCE entry is intact");
		CHECK(!name_present(&vol, fat_len, subc, "GROWC.TXT", NULL),
		      "[rollback-b] the destination entry was UN-written -- the entry "
		      "never exists in two directories at once");
		CHECK(chain_len(&vol, fat_len, subc) == 1u,
		      "[rollback-b] SUBC is back to ONE cluster");
		CHECK(free_count(&vol, fat_len) == free_before,
		      "[rollback-b] the free-cluster count is exactly restored");
	}

	/* ---- [10] a FULL fixed root cannot grow ------------------------- *
	 * LAST leg: it overwrites every root slot. */
	CHECK(fill_root_dir(&vol) == 0, "[dirfull] fill every root-directory slot");
	rc = fat12_move_dirent(&vol, g_fat, fat_len, "KEEP.TXT", holder, NULL, 0u,
	                       g_sector, g_cluster);
	CHECK(rc == FAT12_ERR_DIR_FULL,
	      "[dirfull] a move into a FULL fixed root -> FAT12_ERR_DIR_FULL "
	      "(the root cannot grow)");
	CHECK(name_present(&vol, fat_len, holder, "KEEP.TXT", NULL),
	      "[dirfull] the source entry survives the DIR_FULL refusal");

	blockdev_file_close(&bf);
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "usage: %s <differential-image> <structural-image>\n",
		        argv[0]);
		return 2;
	}
	run_differential_image(argv[1]);
	run_structural_image(argv[2]);
	return TEST_SUMMARY("test_fat12_move");
}
