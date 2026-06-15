/*
 * os/milton/fat12.c -- FAT12 volume mount + BPB parse (MILTON).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): shipped InitechDOS kernel code. Freestanding
 * C -- only <stdint.h> / <stddef.h>, NO libc, NO malloc. Buffers are caller-
 * provided. Builds freestanding (target) and hosted (factory oracle).
 *
 * Ref (Law 1):
 *   - PRD Sec 6.1 (InitechDOS FAT/disk path).
 *   - ADR-0003 DEC-07 (Sec 5.7): "boot sector; first FAT; second (redundant)
 *     FAT; fixed-size root directory; data area."
 *   - docs/research/fat12-ground-truth.md:
 *       Sec 1 (BPB layout), Sec 2 (derived geometry + FAT type rule).
 *   - spec/dos_structs.h (locked bpb_t, BPB_* geometry macros).
 *
 * This slice implements mount/parse/classify (prior step) plus FAT12 12-bit
 * entry decode + cluster-chain walk (this step). Dir enumerate and file read
 * are the next tasks (declared TODO in fat12.h).
 *
 * ASCII-clean (Rule 12). Reproducible: no timestamps / host paths (Rule 11).
 */

#include "fat12.h"

/* Read a little-endian uint16 from an unaligned byte pointer. The on-disk BPB
 * is little-endian (brief Sec 1 / RISK-4); reading the boot signature this way
 * avoids any host-endianness assumption (Rule 11 reproducibility). */
static uint16_t rd_le16(const uint8_t *p)
{
	return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

int fat12_mount(fat12_volume_t *vol, const blockdev_t *dev, void *sector_buf)
{
	const uint8_t *sec;
	uint16_t sig;
	uint32_t total_sectors;
	uint32_t data_sectors;
	uint32_t root_dir_sectors;

	/* Fail loud on NULLs (Rule 2): a missing dev or buffer is a programming
	 * error, not something to paper over. */
	if (vol == NULL || dev == NULL || sector_buf == NULL ||
	    dev->read_sectors == NULL) {
		return FAT12_ERR_NULL;
	}

	/* Read the boot sector (LBA 0, one sector) into the caller's scratch. */
	if (dev->read_sectors(dev->ctx, 0u, 1u, sector_buf) != 0) {
		return FAT12_ERR_READ;
	}
	sec = (const uint8_t *)sector_buf;

	/* Validate the 0xAA55 boot signature at offset 510 (brief Sec 1). */
	sig = rd_le16(sec + FAT12_BOOTSIG_OFFSET);
	if (sig != FAT12_BOOTSIG_VALUE) {
		return FAT12_ERR_SIGNATURE;
	}

	/* Copy the BPB (offsets 0x00..0x3D, the first 62 bytes; brief Sec 6 /
	 * spec/dos_structs.h). bpb_t is packed, so a byte-wise copy from the
	 * boot sector reproduces the on-disk little-endian layout exactly. */
	{
		uint8_t *dst = (uint8_t *)&vol->bpb;
		size_t i;
		for (i = 0; i < sizeof(vol->bpb); i++) {
			dst[i] = sec[i];
		}
	}

	/* Sector size must be 512 (brief Sec 1; BLOCKDEV_SECTOR_SIZE). Anything
	 * else means a non-floppy or corrupt BPB -- fail loud (Rule 2). */
	if (vol->bpb.bytes_per_sector != BLOCKDEV_SECTOR_SIZE) {
		return FAT12_ERR_SECTOR_SIZE;
	}

	/* Geometry sanity: divisors must be nonzero before we use them. A zero
	 * sectors_per_cluster / num_fats / sectors_per_fat / total sectors is
	 * nonsense geometry, not a usable volume (Rule 2 -- panic-grade). */
#ifdef FAT12_MUTATE_ACCEPT_BAD_BPB
	/* MUTANT (Rule 6; test-fat-corrupt-fuzz-mutant only): REMOVE the
	 * sectors_per_cluster==0 nonsense-geometry guard, so a malformed BPB
	 * (sectors_per_cluster 0) is ACCEPTED at mount instead of failing loud
	 * with FAT12_ERR_GEOMETRY. The corruption fuzzer's bad-BPB leg asserts
	 * FAT12_ERR_GEOMETRY and must go RED. NEVER define in a real build. */
	if (vol->bpb.num_fats == 0u ||
	    vol->bpb.sectors_per_fat == 0u ||
	    vol->bpb.root_entry_count == 0u) {
		return FAT12_ERR_GEOMETRY;
	}
#else
	if (vol->bpb.sectors_per_cluster == 0u ||
	    vol->bpb.num_fats == 0u ||
	    vol->bpb.sectors_per_fat == 0u ||
	    vol->bpb.root_entry_count == 0u) {
		return FAT12_ERR_GEOMETRY;
	}
#endif

	/* total_sectors: 16-bit field, or the 32-bit field when it is zero
	 * (brief Sec 1 note on total_sectors_32). */
	total_sectors = vol->bpb.total_sectors_16 != 0u
	                  ? (uint32_t)vol->bpb.total_sectors_16
	                  : vol->bpb.total_sectors_32;
	if (total_sectors == 0u) {
		return FAT12_ERR_GEOMETRY;
	}

	/* Derived geometry (brief Sec 2; macros in spec/dos_structs.h). */
	vol->dev              = dev;
	vol->first_fat_sector = BPB_FAT1_SECTOR(&vol->bpb);
	vol->root_dir_sector  = BPB_ROOT_DIR_SECTOR(&vol->bpb);
	root_dir_sectors      = BPB_ROOT_DIR_SECTORS(&vol->bpb);
	vol->root_dir_sectors = root_dir_sectors;
	vol->first_data_sector= BPB_FIRST_DATA_SECTOR(&vol->bpb);

	/* The data area must lie within the volume and not before the root dir. */
	if (vol->first_data_sector >= total_sectors ||
	    vol->first_data_sector <= vol->root_dir_sector) {
		return FAT12_ERR_GEOMETRY;
	}

	/* sectors_per_cluster MUST be 1 (Rule 2 -- fail loud over silent corruption).
	 * The WRITE path (fat12_grow_dir, fat12_write_partial, fat12_write_file,
	 * fat12_mkdir) zero-fills / read-modify-writes bytes_per_cluster =
	 * sectors_per_cluster*512 bytes into the caller's cluster_buf and writes
	 * sectors_per_cluster sectors per cluster; but the kernel caller
	 * (os/milton/fileio_fat.c) sizes g_cluster at ONE BLOCKDEV_SECTOR_SIZE (512).
	 * On a spc>1 volume those writes overrun the buffer -> heap/stack corruption.
	 * Every period image this artifact targets is a 1.44 MB floppy (mformat
	 * -f 1440 => spc==1), so this is a never-hit-yet invariant; a loud refusal at
	 * mount beats a silent overrun if a future image violates it (the spc>1
	 * cluster-buf-sizing contract is the bead-zs24 caveat). Gated BEFORE the
	 * total_clusters division below so a spc==0 BPB (caught above by the geometry
	 * guard in a real build) never even reaches a divide-by-zero. */
	if (vol->bpb.sectors_per_cluster != 1u) {
		return FAT12_ERR_UNSUPPORTED;
	}

	/* total_clusters = data_sectors / sectors_per_cluster (brief Sec 2). */
	data_sectors        = total_sectors - vol->first_data_sector;
	vol->total_clusters = data_sectors / vol->bpb.sectors_per_cluster;

	/* Classify FAT12 vs FAT16 SOLELY by cluster count (brief Sec 2 / RISK-8;
	 * the fs_type string is informational and unreliable). */
	if (vol->total_clusters < FAT12_MAX_CLUSTERS) {
		vol->fat_type = FAT_TYPE_FAT12;
#ifdef FAT16_MUTATE_NO_CLASSIFY
	} else if (vol->total_clusters < FAT16_MAX_CLUSTERS) {
		/* MUTANT (Rule 6; test-fat16 mutant M5 only): a FAT16 volume is
		 * classified FAT12, so fat12_next_cluster runs the 12-bit decode on a
		 * 16-bit FAT. Every chain decodes to garbage -> the FAT16 read oracle
		 * goes RED. This proves the cluster-count classification + the fat_type
		 * dispatch are load-bearing. NEVER define in a real build. */
		vol->fat_type = FAT_TYPE_FAT12;
#else
	} else if (vol->total_clusters < FAT16_MAX_CLUSTERS) {
		/* A real FAT16 volume (beads initech-z01, READ-ONLY, non-partitioned).
		 * FAT16 is ACCEPTED now: the link decode + EOC/bad thresholds differ for
		 * a 16-bit FAT, but the chain WALKERS are parameterized by the decode
		 * primitive (fat12_next_cluster dispatches to fat16_next_cluster on
		 * fat_type) + vol-aware classify helpers (chain_is_eoc/_free/_bad), so a
		 * FAT16 volume reads correctly with no 12-bit mis-decode (the bcg.4
		 * concern that previously forced a reject) and the FAT12 path stays
		 * byte-identical. Ref (Law 1): docs/research/fat16-ground-truth.md
		 * Sec 0/2/3; Microsoft FAT spec Sec 3.2. WRITE is deferred (509.11);
		 * the FAT12 WRITE primitives are never invoked on a FAT16 volume. */
		vol->fat_type = FAT_TYPE_FAT16;
#endif /* FAT16_MUTATE_NO_CLASSIFY */
	} else {
		/* FAT32 is out of scope for this slice; do not silently mis-handle. */
		return FAT12_ERR_GEOMETRY;
	}

	return FAT12_OK;
}

/*
 * fat12_read_fat -- read the entire first FAT contiguously (brief RISK-1).
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md RISK-1 -- "read the entire
 *   FAT into memory ... eliminates the boundary case entirely." The first FAT
 *   begins at vol->first_fat_sector (= reserved_sectors) and spans
 *   sectors_per_fat sectors (brief Sec 2).
 */
int fat12_read_fat(const fat12_volume_t *vol, void *fat_buf, uint32_t fat_buf_len)
{
	uint32_t fat_bytes;

	if (vol == NULL || fat_buf == NULL || vol->dev == NULL ||
	    vol->dev->read_sectors == NULL) {
		return FAT12_ERR_NULL;
	}

	/* Whole-FAT size in bytes (brief RISK-1: 9 * 512 = 4608 for 1.44 MB).
	 * bytes_per_sector and sectors_per_fat were range-checked at mount. */
	fat_bytes = (uint32_t)vol->bpb.sectors_per_fat *
	            (uint32_t)vol->bpb.bytes_per_sector;

	/* Fail loud if the caller's buffer cannot hold the whole FAT (Rule 2):
	 * a partial FAT would silently misdecode straddling entries. */
	if (fat_buf_len < fat_bytes) {
		return FAT12_ERR_BUFFER;
	}

	/* One contiguous read of all sectors_per_fat sectors of FAT #1. */
	if (vol->dev->read_sectors(vol->dev->ctx, vol->first_fat_sector,
	                           vol->bpb.sectors_per_fat, fat_buf) != 0) {
		return FAT12_ERR_READ;
	}

	return FAT12_OK;
}

/* fat12_last_cluster (defined below) -- forward decl so the in-range predicate
 * can be used by the chain walkers above its definition. */
static uint16_t fat12_last_cluster(const fat12_volume_t *vol);

/* fat12_cluster_in_range -- true iff `c` is a valid DATA cluster, i.e. in
 * [FAT12_FIRST_DATA_CLUSTER, last_data_cluster]. A link value above the last
 * data cluster but below the bad/EOC range (0xFF7) decodes as a normal pointer
 * and, on the 1.44 MB geometry, its FAT byte offset still sits inside the
 * whole-FAT buffer -- so the buffer-bounds check in fat12_next_cluster does NOT
 * catch it. Used as an LBA it would silently map to a wrong (or out-of-volume)
 * sector. This explicit upper bound is the only thing that catches it (bcg.3).
 * Ref: docs/research/fat12-ground-truth.md Sec 2-3; CLAUDE.md Rule 2. */
static int fat12_cluster_in_range(const fat12_volume_t *vol, uint16_t c)
{
	return c >= FAT12_FIRST_DATA_CLUSTER && c <= fat12_last_cluster(vol);
}

/*
 * fat16_next_cluster -- decode the 16-bit FAT16 entry for `cluster` (beads
 * initech-z01, READ-ONLY). A SEPARATE decoder, NOT the 12-bit fat12 decode with
 * a mask: FAT16 is a flat little-endian uint16_t array, byte_offset = cluster*2,
 * no nibble packing, no even/odd case.
 *
 * Ref (Law 1): docs/research/fat16-ground-truth.md Sec 3 --
 *   byte_offset = N*2;  entry = fat[off] | (fat[off+1]<<8).
 * Worked examples (CHAIN16.TXT 3->4->5->6->EOC; cluster 7 entry 0x0008)
 * verified there against a real mkfs.fat -F 16 image.
 */
int fat16_next_cluster(const fat12_volume_t *vol, const void *fat,
                       uint32_t fat_len, uint16_t cluster, uint16_t *out_next)
{
	const uint8_t *b;
	uint32_t       off;

	(void)vol; /* decode is purely a function of the flat FAT buffer */

	if (fat == NULL || out_next == NULL) {
		return FAT12_ERR_NULL;
	}

	/* Clusters 0 and 1 are reserved, not data clusters (brief Sec 3). */
	if (cluster < FAT12_FIRST_DATA_CLUSTER) {
		return FAT12_ERR_CLUSTER;
	}

#ifndef FAT16_MUTATE_ENTRY_OFFSET
	/* byte_offset = cluster*2 (FAT16 entry stride; brief Sec 3). */
	off = (uint32_t)cluster * 2u;
#else
	/* MUTANT (Rule 6; test-fat16 mutant M1 only): off-by-one entry offset --
	 * read a MISALIGNED 16-bit word (cluster*2 + 1). Every chain link decodes
	 * to garbage, so the FAT16 differential (chain walk / file content) goes
	 * RED. NEVER define in a real build. */
	off = (uint32_t)cluster * 2u + 1u;
#endif
	/* Both b[off] and b[off+1] must be in range -- bounds-check before reading
	 * (Rule 2). Catches an out-of-volume cluster index. */
	if (off + 1u >= fat_len) {
		return FAT12_ERR_CLUSTER;
	}

	b = (const uint8_t *)fat;
#ifndef FAT16_MUTATE_ENTRY_MASK12
	/* Little-endian 16-bit entry, used VERBATIM (no mask): a valid FAT16 cluster
	 * pointer can exceed 0x0FFF, and the EOC value 0xFFFF must survive intact for
	 * the >= 0xFFF8 classification (brief Sec 3). */
	*out_next = (uint16_t)((uint16_t)b[off] | ((uint16_t)b[off + 1u] << 8));
#else
	/* MUTANT (Rule 6; test-fat16 mutant M2 only): mask the FAT16 entry to 12
	 * bits as if it were a FAT12 entry. Any cluster pointer >= 0x1000 corrupts,
	 * and EOC 0xFFFF -> 0x0FFF which is NOT >= 0xFFF8, so chains never terminate
	 * correctly -> the differential goes RED. NEVER define in a real build. */
	*out_next = (uint16_t)(((uint16_t)b[off] | ((uint16_t)b[off + 1u] << 8))
	                       & FAT12_ENTRY_MASK);
#endif

	return FAT12_OK;
}

/*
 * fat12_next_cluster -- decode the next-cluster link for `cluster`. DISPATCHES
 * on vol->fat_type (beads initech-z01): a FAT16 volume decodes via the separate
 * 16-bit fat16_next_cluster; a FAT12 volume runs the 12-bit decode below,
 * BYTE-IDENTICAL to its historical behavior (the FAT12 path is unchanged). Every
 * higher-level chain walker funnels through here, so this single dispatch makes
 * read_file / read_partial / read_dir / resolve_path / the dir scans correct for
 * both FAT types with no duplicated walk logic.
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 3 (12-bit decode) +
 *   fat16-ground-truth.md Sec 0/3 (the decode is the only per-type difference).
 *   FAT12: byte_offset = (N*3)/2; v = b[off]|(b[off+1]<<8);
 *          entry = (N even) ? (v & 0x0FFF) : (v >> 4).
 */
int fat12_next_cluster(const fat12_volume_t *vol, const void *fat,
                       uint32_t fat_len, uint16_t cluster, uint16_t *out_next)
{
	const uint8_t *b;
	uint32_t       off;
	uint16_t       v;

	if (fat == NULL || out_next == NULL) {
		return FAT12_ERR_NULL;
	}

	/* FAT16 dispatch (beads initech-z01): a FAT16 volume must NOT be decoded by
	 * the 12-bit path (the M5 mutation -- garbage chains). vol may be NULL for a
	 * few decode-only unit callers; treat a NULL vol as FAT12 (the historical
	 * decode-is-a-pure-function-of-the-buffer contract, brief Sec 3). */
	if (vol != NULL && vol->fat_type == FAT_TYPE_FAT16) {
		return fat16_next_cluster(vol, fat, fat_len, cluster, out_next);
	}

	/* Clusters 0 and 1 are reserved, not data clusters (brief Sec 3). */
	if (cluster < FAT12_FIRST_DATA_CLUSTER) {
		return FAT12_ERR_CLUSTER;
	}

	/* byte_offset = (cluster*3)/2 (integer div). Both b[off] and b[off+1]
	 * must be in range -- bounds-check before reading (Rule 2). The whole-FAT
	 * buffer made boundary straddles harmless; this catches an out-of-volume
	 * cluster index. */
	off = ((uint32_t)cluster * 3u) / 2u;
	if (off + 1u >= fat_len) {
		return FAT12_ERR_CLUSTER;
	}

	b = (const uint8_t *)fat;
	v = (uint16_t)((uint16_t)b[off] | ((uint16_t)b[off + 1u] << 8));

	/* Even cluster -> low 12 bits; odd cluster -> high 12 bits (brief Sec 3). */
	if ((cluster & 1u) == 0u) {
		*out_next = (uint16_t)(v & FAT12_ENTRY_MASK);
	} else {
		*out_next = (uint16_t)((v >> 4) & FAT12_ENTRY_MASK);
	}

	return FAT12_OK;
}

/* ------------------------------------------------------------------------ *
 * Vol-aware chain CLASSIFICATION (beads initech-z01).
 *
 * The raw value fat12_next_cluster returns is a 12-bit value on a FAT12 volume
 * and a 16-bit value on a FAT16 volume; the EOC/bad/free thresholds differ
 * (FAT12 0xFF8/0xFF7; FAT16 0xFFF8/0xFFF7). A bare fat12_is_eoc(v) would wrongly
 * flag a valid FAT16 cluster pointer >= 0xFF8 as EOC (the M3 failure). These
 * helpers dispatch on vol->fat_type so the chain walkers classify correctly for
 * both types; for a FAT12 volume they reduce EXACTLY to the FAT12 predicates, so
 * the FAT12 path stays byte-identical. Ref: docs/research/fat16-ground-truth.md
 * Sec 3 (special values per type); Microsoft FAT spec Sec 3.2.
 * ------------------------------------------------------------------------ */
static int chain_is_eoc(const fat12_volume_t *vol, uint16_t v)
{
#ifndef FAT16_MUTATE_EOC_THRESHOLD
	if (vol != NULL && vol->fat_type == FAT_TYPE_FAT16) {
		return fat16_is_eoc(v);   /* >= 0xFFF8 */
	}
#else
	/* MUTANT (Rule 6; test-fat16 mutant M3 only): use the FAT12 0xFF8 EOC
	 * threshold on a FAT16 volume. A normal FAT16 cluster pointer in
	 * 0x0FF8..0x0FFF (and any cluster index >= 4088) is wrongly seen as EOC, so
	 * large files truncate mid-chain -> the differential goes RED. NEVER in a
	 * real build. */
	(void)vol;
#endif
	return fat12_is_eoc(v);       /* >= 0xFF8 (FAT12) */
}
static int chain_is_free(const fat12_volume_t *vol, uint16_t v)
{
	if (vol != NULL && vol->fat_type == FAT_TYPE_FAT16) {
		return fat16_is_free(v);  /* == 0x0000 (same numeric as FAT12 0x000) */
	}
	return fat12_is_free(v);
}
static int chain_is_bad(const fat12_volume_t *vol, uint16_t v)
{
	if (vol != NULL && vol->fat_type == FAT_TYPE_FAT16) {
		return fat16_is_bad(v);   /* == 0xFFF7 */
	}
	return fat12_is_bad(v);
}

/*
 * fat12_walk_chain -- follow start_cluster..EOC, with the anti-hang guard.
 *
 * Ref (Law 1): brief Sec 3 (chain semantics; EOC >= 0xFF8) and CLAUDE.md
 *   Rule 2 -- "a cyclic chain must NOT hang." max_clusters bounds the loop:
 *   a valid chain can visit at most total_clusters distinct clusters, so the
 *   caller sizes out_clusters accordingly; exceeding it means a cycle or
 *   corruption, which we surface as FAT12_ERR_CHAIN rather than spin.
 */
int fat12_walk_chain(const fat12_volume_t *vol, const void *fat,
                     uint32_t fat_len, uint16_t start_cluster,
                     uint16_t *out_clusters, uint32_t max_clusters,
                     uint32_t *out_count)
{
	uint16_t cur;
	uint32_t n;

	if (fat == NULL || out_clusters == NULL || out_count == NULL) {
		return FAT12_ERR_NULL;
	}
	if (start_cluster < FAT12_FIRST_DATA_CLUSTER) {
		return FAT12_ERR_CLUSTER;
	}

	cur = start_cluster;
	n   = 0u;

	for (;;) {
		uint16_t next;
		int      rc;

		/* A free or bad cluster appearing IN a chain is corruption, not a
		 * normal terminator -- fail loud (Rule 2). Checked before storing so
		 * a corrupt start_cluster is also rejected. */
		if (chain_is_free(vol, cur)) {
			return FAT12_ERR_CHAIN;
		}
		if (chain_is_bad(vol, cur)) {
			return FAT12_ERR_CHAIN;
		}
		/* An in-chain link above the last valid data cluster is corruption that
		 * the buffer-bounds check cannot catch; reject it before it is stored or
		 * mapped to an LBA (bcg.3). */
		if (!fat12_cluster_in_range(vol, cur)) {
			return FAT12_ERR_CLUSTER;
		}

		/* Anti-hang / overflow guard (Rule 2): never write past the caller's
		 * array, and never visit more clusters than it can hold. A cyclic
		 * chain hits this bound and errors instead of looping forever. */
		if (n >= max_clusters) {
			return FAT12_ERR_CHAIN;
		}
		out_clusters[n] = cur;
		n++;

		/* Decode the next link from the flat FAT buffer. */
		rc = fat12_next_cluster(vol, fat, fat_len, cur, &next);
		if (rc != FAT12_OK) {
			return rc;
		}

		/* EOC terminates the chain normally (brief Sec 3). */
		if (chain_is_eoc(vol, next)) {
			break;
		}

		cur = next;
	}

	*out_count = n;
	return FAT12_OK;
}

/* Number of 32-byte directory entries that fit in one 512-byte sector. */
#define FAT12_DIRENTS_PER_SECTOR (BLOCKDEV_SECTOR_SIZE / 32u)

/* ASCII lower->upper for the case-insensitive 8.3 compare. Freestanding: no
 * <ctype.h>; ASCII-only (Rule 12). Non-letters pass through unchanged. */
static char up_ascii(char c)
{
	if (c >= 'a' && c <= 'z') {
		return (char)(c - ('a' - 'A'));
	}
	return c;
}

/*
 * fat12_format_83 -- render the locked 8.3 name (brief Sec 4).
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 4 -- "Stored as 11
 *   bytes (8+3), NOT separated by a dot. Space-padded (0x20) ... 0x05 first
 *   byte means the real first char is 0xE5" (RISK-3). filename[8] then
 *   extension[3] in spec/dos_structs.h dir_entry_t.
 */
int fat12_format_83(const dir_entry_t *e, char *out)
{
	uint32_t i;
	uint32_t o;

	if (e == NULL || out == NULL) {
		return FAT12_ERR_NULL;
	}

	o = 0u;

	/* Name part: copy up to 8 chars, dropping trailing space padding. The
	 * 0x05 -> 0xE5 fix applies ONLY to the first byte (brief Sec 4 / RISK-3);
	 * 0xE5 is never a real padding/sentinel value once we are past byte 0. */
	for (i = 0u; i < 8u; i++) {
		uint8_t ch = e->filename[i];
		if (ch == 0x20u) {
			break; /* space padding -- name ends here (no embedded spaces) */
		}
		if (i == 0u && ch == FAT12_NAME_E5_ALIAS) {
			ch = DIR_NAME_DELETED; /* 0x05 -> 0xE5 real first character */
		}
		out[o++] = (char)ch;
	}

	/* Extension part: only if at least one non-space char. Prefix with '.'. */
	{
		uint32_t ext_len = 0u;
		for (i = 0u; i < 3u; i++) {
			if (e->extension[i] != 0x20u) {
				ext_len = i + 1u; /* last non-space index + 1 */
			}
		}
		if (ext_len > 0u) {
			out[o++] = '.';
			for (i = 0u; i < ext_len; i++) {
				out[o++] = (char)e->extension[i];
			}
		}
	}

	out[o] = '\0';
	return FAT12_OK;
}

/*
 * fat12_read_root_dir -- enumerate the fixed root directory (brief Sec 4).
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 4 -- root dir is a
 *   FIXED region (root_dir_sector .. + root_dir_sectors), root_entry_count
 *   entries of 32 bytes; 0x00 => end (STOP), 0xE5 => deleted (SKIP),
 *   attr==0x0F => LFN (SKIP). NOT a cluster chain.
 */
int fat12_read_root_dir(const fat12_volume_t *vol, void *sector_buf,
                        fat12_dirent_cb cb, void *user)
{
	uint32_t entries_seen;
	uint32_t s;

	if (vol == NULL || sector_buf == NULL || cb == NULL ||
	    vol->dev == NULL || vol->dev->read_sectors == NULL) {
		return FAT12_ERR_NULL;
	}

	entries_seen = 0u;

	/* Read the root directory one sector at a time into the caller's scratch.
	 * Bounded by root_dir_sectors AND root_entry_count (brief Sec 4): we never
	 * read past the fixed region, and we never inspect more than the declared
	 * entry count even if the last sector is not fully populated. */
	for (s = 0u; s < vol->root_dir_sectors; s++) {
		const uint8_t *sec;
		uint32_t       i;

		if (vol->dev->read_sectors(vol->dev->ctx,
		                           vol->root_dir_sector + s, 1u,
		                           sector_buf) != 0) {
			return FAT12_ERR_READ;
		}
		sec = (const uint8_t *)sector_buf;

		for (i = 0u; i < FAT12_DIRENTS_PER_SECTOR; i++) {
			const dir_entry_t *e;
			uint8_t            first;

			if (entries_seen >= vol->bpb.root_entry_count) {
				return FAT12_OK; /* consumed the whole declared root dir */
			}
			entries_seen++;

			e     = (const dir_entry_t *)(sec + i * 32u);
			first = e->filename[0];

			/* 0x00 => end of directory: no valid entries follow (brief Sec 4). */
			if (first == DIR_NAME_FREE) {
				return FAT12_OK;
			}
			/* 0xE5 => deleted: skip and continue. */
			if (first == DIR_NAME_DELETED) {
				continue;
			}
			/* attr == 0x0F => VFAT LFN slot: skip entirely (brief RISK-2). */
			if (e->attribute == FAT12_ATTR_LFN) {
				continue;
			}

			/* Surviving entry -> visit. Non-zero from cb stops early and is
			 * propagated verbatim (e.g. fat12_find's found-sentinel). */
			{
				int rc = cb(e, user);
				if (rc != 0) {
					return rc;
				}
			}
		}
	}

	return FAT12_OK;
}

/* fat12_find context: the target name (case-insensitive) and the slot the
 * matching entry is copied into. `found` flips to 1 on the first match so the
 * enumeration stops early. */
typedef struct fat12_find_ctx {
	const char  *want;       /* caller's target 8.3 name */
	dir_entry_t *out_entry;  /* destination for the match */
	int          found;      /* 0 until a match copied   */
} fat12_find_ctx_t;

/* Case-insensitive ASCII compare of two NUL-terminated 8.3 names. */
static int name83_ieq(const char *a, const char *b)
{
	uint32_t i = 0u;
	for (;;) {
		char ca = up_ascii(a[i]);
		char cb = up_ascii(b[i]);
		if (ca != cb) {
			return 0;
		}
		if (ca == '\0') {
			return 1; /* both terminated at the same point */
		}
		i++;
	}
}

/* fat12_read_root_dir visitor: format each entry's 8.3 name and, on a
 * case-insensitive match, copy the entry out and request an early stop. */
static int fat12_find_cb(const dir_entry_t *e, void *user)
{
	fat12_find_ctx_t *ctx = (fat12_find_ctx_t *)user;
	char              name[FAT12_NAME83_MAX];

	if (fat12_format_83(e, name) != FAT12_OK) {
		return 0; /* unreadable name -- treat as non-match, keep scanning */
	}
	if (name83_ieq(name, ctx->want)) {
		uint32_t       k;
		const uint8_t *src = (const uint8_t *)e;
		uint8_t       *dst = (uint8_t *)ctx->out_entry;
		for (k = 0u; k < sizeof(dir_entry_t); k++) {
			dst[k] = src[k];
		}
		ctx->found = 1;
		return 1; /* stop enumeration */
	}
	return 0;
}

/*
 * fat12_find -- locate an entry by formatted 8.3 name (brief Sec 4).
 */
int fat12_find(const fat12_volume_t *vol, void *sector_buf,
               const char *name83, dir_entry_t *out_entry)
{
	fat12_find_ctx_t ctx;
	int              rc;

	if (vol == NULL || sector_buf == NULL || name83 == NULL ||
	    out_entry == NULL) {
		return FAT12_ERR_NULL;
	}

	ctx.want      = name83;
	ctx.out_entry = out_entry;
	ctx.found     = 0;

	/* The callback returns 1 on a match (early stop). A genuine read error
	 * from fat12_read_root_dir is negative and must be propagated; the
	 * early-stop 1 is not an error -- distinguish on ctx.found. */
	rc = fat12_read_root_dir(vol, sector_buf, fat12_find_cb, &ctx);
	if (rc < 0) {
		return rc; /* propagate FAT12_ERR_READ / _NULL */
	}
	if (!ctx.found) {
		return FAT12_ERR_NOT_FOUND;
	}
	return FAT12_OK;
}

/*
 * fat12_read_file -- read exactly file_size bytes via the cluster chain,
 * walked INCREMENTALLY so a large file never materializes a whole-chain
 * buffer on the kernel stack (beads initech-dao). The historical version held
 * a uint16_t chain[2880] (~5.6 KB) on the stack, sized to the 1.44 MB floppy
 * cluster count -- a kernel-stack hazard that the FAT16 HDD geometry (far more
 * clusters, beads initech-z01) would overflow. The streaming walk below serves
 * an arbitrarily large file from the caller's single-cluster `cluster_buf`,
 * stepping one fat12_next_cluster link at a time -- the SAME proven pattern as
 * fat12_read_partial (this file).
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 3 (chain link decode:
 *   next = fat12_next_cluster; EOC >= 0xFF8; cyclic detection) + Sec 4 / RISK-5
 *   ("file_size is authoritative for the last cluster ... do not return padding
 *   zeros"). The cluster->LBA mapping is BPB_CLUSTER_LBA (spec/dos_structs.h);
 *   the anti-hang guard mirrors fat12_walk_chain / fat12_read_partial (Rule 2):
 *   never step more than the volume's cluster count, so a cyclic/corrupt chain
 *   errors (FAT12_ERR_CHAIN) rather than looping forever or over-reading.
 *   Microsoft FAT spec: file_size is the authoritative byte count.
 */
int fat12_read_file(const fat12_volume_t *vol, const void *fat, uint32_t fat_len,
                    const dir_entry_t *e, void *out_buf, uint32_t out_buf_len,
                    void *cluster_buf, uint32_t *out_bytes)
{
	uint32_t  file_size;
	uint32_t  bytes_per_cluster;
	uint32_t  copied;
	uint16_t  cur;
	uint32_t  steps;
	uint32_t  max_steps;
	uint8_t  *out;
	int       rc;

	if (vol == NULL || e == NULL || out_buf == NULL || out_bytes == NULL) {
		return FAT12_ERR_NULL;
	}

	file_size = e->file_size;

	/* Never overflow the caller's buffer (Rule 2): refuse before copying. */
	if (out_buf_len < file_size) {
		return FAT12_ERR_BUFFER;
	}

	/* Zero-length file: no chain walk (start_cluster may legitimately be 0).
	 * Done before touching fat/cluster_buf so a 0-byte read needs neither. */
	if (file_size == 0u) {
		*out_bytes = 0u;
		return FAT12_OK;
	}

	/* Non-empty file from here on: fat + cluster_buf are required. */
	if (fat == NULL || cluster_buf == NULL) {
		return FAT12_ERR_NULL;
	}

	bytes_per_cluster = (uint32_t)vol->bpb.sectors_per_cluster *
	                    (uint32_t)vol->bpb.bytes_per_sector;

	out    = (uint8_t *)out_buf;
	copied = 0u;
	cur    = e->start_cluster;
	steps  = 0u;
	/* Anti-hang bound (Rule 2): a valid chain visits at most total_clusters
	 * distinct data clusters; +2 covers the reserved 0/1 slack. We step at
	 * most this many times; a cyclic/corrupt chain trips it and errors instead
	 * of spinning forever -- the SAME bound fat12_read_partial uses. */
#ifndef FAT12_MUTATE_READFILE_STEP_BOUND
	max_steps = vol->total_clusters + 2u;
#else
	/* MUTANT (Rule 6; make test-fat-readfile-mutant only): shrink the anti-hang
	 * step bound so it no longer covers the longest LEGITIMATE chain. A bound of
	 * total_clusters/4 (~711 on the 1.44 MB floppy) is below the 1368-cluster
	 * BIGCHAIN.TXT chain, so that large valid file trips the bound mid-walk and
	 * errors with FAT12_ERR_CHAIN instead of reading clean -> the read oracle
	 * (its byte-for-byte BIGCHAIN leg) goes RED. This proves the bound must be at
	 * least the volume's cluster count -- a too-small bound is the real failure
	 * mode the +2 form guards against (and is exactly what the FAT16 HDD geometry
	 * would have hit with the old chain[2880]). NEVER define in a real build. */
	max_steps = vol->total_clusters / 4u;
#endif

	/* Copy cluster-by-cluster, stepping the chain incrementally. The LAST
	 * cluster is truncated to whatever remains of file_size (RISK-5: no
	 * trailing padding). The chain MUST cover file_size; an EOC or a
	 * free/bad/out-of-range link before file_size is satisfied is corruption
	 * (fail loud, Rule 2). */
	while (copied < file_size) {
		uint32_t       lba;
		uint32_t       remaining;
		uint32_t       take;
		uint32_t       copy_n;
		const uint8_t *cbuf;
		uint32_t       k;

		/* An in-chain link below cluster 2, above the last data cluster, or a
		 * free/bad cluster is corruption -- never map it to an LBA (bcg.3). */
		if (!fat12_cluster_in_range(vol, cur) || chain_is_free(vol, cur) ||
		    chain_is_bad(vol, cur)) {
			return FAT12_ERR_CHAIN;
		}

		remaining = file_size - copied;
		take      = remaining < bytes_per_cluster
		              ? remaining : bytes_per_cluster;
#ifndef FAT12_MUTATE_READFILE_TRUNC
		/* RISK-5: copy only `take` bytes -- the partial last cluster never
		 * contributes its padding to out_buf. */
		copy_n = take;
#else
		/* MUTANT (Rule 6; make test-fat-readfile-mutant only): drop the RISK-5
		 * last-cluster truncation -- copy a FULL cluster every iteration instead
		 * of `take` bytes. On the partial last cluster this writes padding into
		 * out_buf and copied overshoots file_size, so the bytes read no longer
		 * match the golden -> the read oracle goes RED. NEVER in a real build. */
		(void)take;
		copy_n = bytes_per_cluster;
#endif

		/* Read the whole cluster (sectors_per_cluster sectors) into scratch,
		 * then copy copy_n bytes out. */
#ifndef FAT16_MUTATE_NO_CLUSTER2_BIAS
		lba = BPB_CLUSTER_LBA(&vol->bpb, cur);
#else
		/* MUTANT (Rule 6; test-fat16 mutant M4 only): OMIT the cluster-2 LBA bias
		 * -- map cluster N to first_data_sector + N*spc instead of (N-2)*spc.
		 * Every cluster reads two sectors too high, so the file content no longer
		 * matches the golden -> the FAT16 read oracle goes RED. This proves the
		 * cluster->LBA mapping's -2 bias is load-bearing. NEVER in a real build. */
		lba = BPB_FIRST_DATA_SECTOR(&vol->bpb) +
		      (uint32_t)cur * (uint32_t)vol->bpb.sectors_per_cluster;
#endif
		if (vol->dev->read_sectors(vol->dev->ctx, lba,
		                           vol->bpb.sectors_per_cluster,
		                           cluster_buf) != 0) {
			return FAT12_ERR_READ;
		}
		cbuf = (const uint8_t *)cluster_buf;
		for (k = 0u; k < copy_n; k++) {
			out[copied + k] = cbuf[k];
		}
		copied += copy_n;

		/* Advance to the next link only if more bytes are still wanted. */
		if (copied < file_size) {
			uint16_t next;

			rc = fat12_next_cluster(vol, fat, fat_len, cur, &next);
			if (rc != FAT12_OK) {
				return rc;
			}
#ifndef FAT12_MUTATE_READFILE_EOC
			if (chain_is_eoc(vol, next)) {
				/* Chain ended before file_size bytes were satisfied -- the
				 * declared size demands more clusters than the chain has, which
				 * is corruption (fail loud, Rule 2). */
				return FAT12_ERR_CHAIN;
			}
#else
			/* MUTANT (Rule 6; make test-fat-readfile-mutant only): INVERT the EOC
			 * test polarity -- treat a NORMAL mid-chain link as end-of-chain and
			 * a true EOC as a normal link. Every multi-cluster file (CHAIN.TXT,
			 * BLOCK.BIN, the 1368-cluster BIGCHAIN.TXT) then errors with
			 * FAT12_ERR_CHAIN on its very first non-EOC link -- it cannot read
			 * past cluster 1 -- so the read oracle goes RED. This proves the EOC
			 * predicate (which link STOPS the walk) is load-bearing, not the
			 * dead-for-valid-files corruption return alone. NEVER in a real build. */
			if (!chain_is_eoc(vol, next)) {
				return FAT12_ERR_CHAIN;
			}
#endif
			cur = next;
			if (++steps > max_steps) {
				return FAT12_ERR_CHAIN; /* cyclic / corrupt */
			}
		}
	}

	/* All file_size bytes are copied; `cur` is the LAST data cluster. The
	 * historical read_file (which materialized the whole chain via
	 * fat12_walk_chain) validated that the chain TERMINATES at EOC -- so a
	 * cyclic chain whose declared file_size happens to fit in its first cluster
	 * was still rejected (the corruption fuzzer's loop-chain leg asserts exactly
	 * this: read_file on a cyclic chain MUST return FAT12_ERR_CHAIN even when
	 * file_size claims one cluster). The streaming refactor preserves that
	 * contract WITHOUT an on-stack chain array: keep stepping from `cur` until
	 * EOC, bounded by the SAME anti-hang step counter, so a cycle/corruption
	 * past the last data cluster is caught (Rule 2; beads initech-dao).
	 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 3 (chain EOC +
	 *   cyclic detection). */
	for (;;) {
		uint16_t next;

		if (!fat12_cluster_in_range(vol, cur) || chain_is_free(vol, cur) ||
		    chain_is_bad(vol, cur)) {
			return FAT12_ERR_CHAIN;
		}
		rc = fat12_next_cluster(vol, fat, fat_len, cur, &next);
		if (rc != FAT12_OK) {
			return rc;
		}
		if (chain_is_eoc(vol, next)) {
			break; /* chain terminates cleanly -- the file is intact */
		}
		cur = next;
		if (++steps > max_steps) {
			return FAT12_ERR_CHAIN; /* cyclic / corrupt past the data */
		}
	}

	*out_bytes = file_size;
	return FAT12_OK;
}

/*
 * fat12_read_partial -- POSITIONED read of [offset, offset+len) via the cluster
 * chain, walked INCREMENTALLY so a large file never needs a whole-file or
 * whole-chain buffer (beads initech-lq2; foundation of the per-handle I/O epic
 * initech-6qy).
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 3 (chain link decode:
 *   next = fat12_next_cluster; EOC >= 0xFF8) + Sec 4 / RISK-5 (file_size is
 *   authoritative; no trailing padding). The offset->cluster math mirrors the
 *   cluster sizing in fat12_read_file (this file): bytes_per_cluster =
 *   sectors_per_cluster * bytes_per_sector; the first needed cluster is
 *   offset / bytes_per_cluster steps down the chain, the byte position inside
 *   it is offset % bytes_per_cluster. The cluster->LBA map is BPB_CLUSTER_LBA
 *   (spec/dos_structs.h). The anti-hang guard mirrors fat12_walk_chain (Rule 2):
 *   never step more than the volume's cluster count.
 */
int fat12_read_partial(const fat12_volume_t *vol, const void *fat,
                       uint32_t fat_len, const dir_entry_t *e,
                       uint32_t offset, uint32_t len, void *out_buf,
                       void *cluster_buf, uint32_t *out_read)
{
	uint32_t  file_size;
	uint32_t  bytes_per_cluster;
	uint32_t  avail;
	uint32_t  to_read;
	uint32_t  skip;
	uint32_t  cluster_pos;   /* byte offset within the first needed cluster   */
	uint16_t  cur;
	uint32_t  steps;
	uint32_t  max_steps;
	uint32_t  copied;
	uint8_t  *out;
	int       rc;

	/* vol/e/out_read are always required (even for clean-EOF / zero-len). */
	if (vol == NULL || e == NULL || out_read == NULL) {
		return FAT12_ERR_NULL;
	}

	file_size = e->file_size;

	/* Clean EOF: a positioned read at or past end-of-file returns zero bytes
	 * and is NOT an error (this is how a seek-then-read past the end behaves).
	 * Also covers a zero-length file (offset 0 >= size 0). Done before touching
	 * fat / out_buf / cluster_buf so the clean-EOF case needs none of them. */
	if (offset >= file_size) {
		*out_read = 0u;
		return FAT12_OK;
	}

	/* Clamp the request to what actually remains from `offset` (RISK-5: never
	 * read past file_size, no padding). */
	avail   = file_size - offset;
	to_read = (len < avail) ? len : avail;

	/* A zero-length request (caller asked for 0 bytes, or none remain after the
	 * clamp -- the latter cannot happen here since avail > 0) copies nothing. */
	if (to_read == 0u) {
		*out_read = 0u;
		return FAT12_OK;
	}

	/* Real bytes to move from here on: fat + out_buf + cluster_buf required. */
	if (fat == NULL || out_buf == NULL || cluster_buf == NULL) {
		return FAT12_ERR_NULL;
	}

	bytes_per_cluster = (uint32_t)vol->bpb.sectors_per_cluster *
	                    (uint32_t)vol->bpb.bytes_per_sector;

	/* offset > 0 with a non-empty read implies a real chain. start_cluster must
	 * be a data cluster (>= 2); a free/short start is corruption (Rule 2). The
	 * incremental walk below also rejects free/bad clusters it lands on. */
#ifdef FAT12_MUTATE_PARTIAL_SKIP
	/* MUTANT (Rule 6; make test-fat-partial-mutant only): off-by-one in the
	 * skip-cluster count -- skip ONE TOO FEW clusters, so a mid/late-cluster
	 * positioned read returns data from the wrong cluster. The differential
	 * oracle vs the python ref must go RED. NEVER define in a real build. */
	skip        = (offset / bytes_per_cluster) > 0u
	                ? (offset / bytes_per_cluster) - 1u
	                : 0u;
	cluster_pos = offset % bytes_per_cluster;
#elif defined(FAT12_MUTATE_PARTIAL_POS)
	/* MUTANT (Rule 6; make test-fat-partial-mutant only): drop the within-cluster
	 * byte offset -- always start the first cluster at byte 0. A mid-cluster
	 * positioned read returns cluster-aligned bytes instead of the requested
	 * slice; the oracle must go RED. NEVER define in a real build. */
	skip        = offset / bytes_per_cluster;
	cluster_pos = 0u;
#else
	skip        = offset / bytes_per_cluster; /* whole clusters to skip past   */
	cluster_pos = offset % bytes_per_cluster; /* byte offset in first cluster  */
#endif

	cur       = e->start_cluster;
	steps     = 0u;
	/* Anti-hang bound (Rule 2): a valid chain visits at most total_clusters
	 * distinct data clusters; +2 covers the reserved 0/1 slack. We step at most
	 * this many times (skip + span); a cyclic/corrupt chain trips it and errors
	 * instead of looping forever. */
	max_steps = vol->total_clusters + 2u;

	/* Walk down to the first NEEDED cluster, rejecting an early end-of-chain or
	 * a free/bad link (the chain is too short for `offset` => corruption). */
	{
		uint32_t k;
		for (k = 0u; k < skip; k++) {
			uint16_t next;

			if (!fat12_cluster_in_range(vol, cur) || chain_is_free(vol, cur) ||
			    chain_is_bad(vol, cur)) {
				return FAT12_ERR_CHAIN;
			}
			rc = fat12_next_cluster(vol, fat, fat_len, cur, &next);
			if (rc != FAT12_OK) {
				return rc;
			}
			/* Hitting EOC while still skipping means the chain is shorter than
			 * `offset` demands -- the requested range is past the real data. */
			if (chain_is_eoc(vol, next)) {
				return FAT12_ERR_CHAIN;
			}
			cur = next;
#ifndef FAT12_MUTATE_NO_STEP_GUARD
			if (++steps > max_steps) {
				return FAT12_ERR_CHAIN; /* cyclic / corrupt */
			}
#else
			/* MUTANT (Rule 6; test-fat-corrupt-fuzz-mutant only): REMOVE the
			 * anti-hang max_steps guard on the skip walk, so a cyclic/self-loop
			 * chain spins FOREVER instead of failing loud. The corruption
			 * fuzzer's loop-chain leg never returns; the Makefile `timeout`
			 * trips -> gate RED. NEVER define in a real build. */
			(void)max_steps;
			steps++;
#endif
		}
	}

	out    = (uint8_t *)out_buf;
	copied = 0u;

	/* Copy `to_read` bytes, cluster by cluster. The FIRST cluster starts at
	 * cluster_pos (the within-cluster byte offset); every later cluster starts
	 * at byte 0. The last cluster contributes only the remaining tail. */
	while (copied < to_read) {
		uint32_t       lba;
		uint32_t       in_off;
		uint32_t       remaining;
		uint32_t       take;
		const uint8_t *cbuf;
		uint32_t       i;

		if (!fat12_cluster_in_range(vol, cur) || chain_is_free(vol, cur) ||
		    chain_is_bad(vol, cur)) {
			return FAT12_ERR_CHAIN; /* chain ended/corrupt before the range */
		}

		/* Within-cluster start: cluster_pos on the first cluster, else 0. */
		in_off    = (copied == 0u) ? cluster_pos : 0u;
		remaining = to_read - copied;
		take      = bytes_per_cluster - in_off;     /* room left in this cluster */
		if (take > remaining) {
			take = remaining;
		}

		/* Read the whole cluster into scratch, then copy the [in_off, in_off+take)
		 * slice out. Reading the whole cluster keeps the LBA math identical to
		 * fat12_read_file; only `take` bytes ever reach out_buf. */
		lba = BPB_CLUSTER_LBA(&vol->bpb, cur);
		if (vol->dev->read_sectors(vol->dev->ctx, lba,
		                           vol->bpb.sectors_per_cluster,
		                           cluster_buf) != 0) {
			return FAT12_ERR_READ;
		}
		cbuf = (const uint8_t *)cluster_buf;
		for (i = 0u; i < take; i++) {
			out[copied + i] = cbuf[in_off + i];
		}
		copied += take;

		/* Advance to the next cluster only if more bytes are still wanted. */
		if (copied < to_read) {
			uint16_t next;
			rc = fat12_next_cluster(vol, fat, fat_len, cur, &next);
			if (rc != FAT12_OK) {
				return rc;
			}
			if (chain_is_eoc(vol, next)) {
				/* Chain ended before `to_read` bytes were satisfied -- and the
				 * clamp guarantees to_read <= file_size-offset, so a chain that
				 * cannot cover it is corruption (fail loud, Rule 2). */
				return FAT12_ERR_CHAIN;
			}
			cur = next;
#ifndef FAT12_MUTATE_NO_STEP_GUARD
			if (++steps > max_steps) {
				return FAT12_ERR_CHAIN; /* cyclic / corrupt */
			}
#else
			/* MUTANT (Rule 6; test-fat-corrupt-fuzz-mutant only): REMOVE the
			 * anti-hang max_steps guard on the copy walk too -- a cyclic chain
			 * entered during the copy phase spins forever. NEVER in a real build. */
			(void)max_steps;
			steps++;
#endif
		}
	}

	*out_read = copied;
	return FAT12_OK;
}

/* ======================================================================== *
 * FAT12 WRITE path (beads initech-509.11)
 *
 * Ref (Law 1): Microsoft FAT spec (12-bit packed entry encode = inverse of the
 *   read decode); docs/research/fat12-ground-truth.md Sec 3 (encode) + Sec 4
 *   (dir entry, normalization note); ADR-0003 DEC-07 (both FATs in sync).
 * ======================================================================== */

/*
 * fat12_set_entry -- the EXACT inverse of fat12_next_cluster's decode (brief
 * Sec 3). Read-modify-write the two straddled bytes so the neighbour's 12-bit
 * entry (which shares one byte) is preserved.
 */
int fat12_set_entry(void *fat, uint32_t fat_len, uint16_t cluster,
                    uint16_t value)
{
	uint8_t *b;
	uint32_t off;
	uint16_t v;

	if (fat == NULL) {
		return FAT12_ERR_NULL;
	}
	if (cluster < FAT12_FIRST_DATA_CLUSTER) {
		return FAT12_ERR_CLUSTER;
	}
	off = ((uint32_t)cluster * 3u) / 2u;
	if (off + 1u >= fat_len) {
		return FAT12_ERR_CLUSTER;
	}

	v = (uint16_t)(value & FAT12_ENTRY_MASK);
	b = (uint8_t *)fat;

	if ((cluster & 1u) == 0u) {
		/* even: low 8 bits in b[off]; high nibble in low nibble of b[off+1]. */
		b[off]      = (uint8_t)(v & 0xFFu);
		b[off + 1u] = (uint8_t)((b[off + 1u] & 0xF0u) | ((v >> 8) & 0x0Fu));
	} else {
		/* odd: low nibble in high nibble of b[off]; high 8 bits in b[off+1]. */
		b[off]      = (uint8_t)((b[off] & 0x0Fu) | ((v << 4) & 0xF0u));
		b[off + 1u] = (uint8_t)((v >> 4) & 0xFFu);
	}
	return FAT12_OK;
}

int fat12_flush_fats(const fat12_volume_t *vol, const void *fat,
                     uint32_t fat_len)
{
	uint32_t fat_bytes;
	uint8_t  i;

	if (vol == NULL || fat == NULL || vol->dev == NULL ||
	    vol->dev->write_sectors == NULL) {
		return FAT12_ERR_WRITE;   /* no write backend -> fail loud (Rule 2) */
	}

	fat_bytes = (uint32_t)vol->bpb.sectors_per_fat *
	            (uint32_t)vol->bpb.bytes_per_sector;
	if (fat_len < fat_bytes) {
		return FAT12_ERR_BUFFER;
	}

	/* Write the SAME bytes to each on-disk FAT copy (ADR-0003 DEC-07). The
	 * copies are contiguous: FAT #k starts at first_fat_sector + k*spf. */
#ifdef FAT12_MUTATE_ONE_FAT_COPY
	/* MUTANT (Rule 6; make test-fat-write-mutant only): write ONLY FAT #1, so
	 * the redundant copy diverges -- the both-FAT-sync assertion + an mtools
	 * read that prefers FAT #2 go RED. NEVER define in a real build. */
	{
		if (vol->dev->write_sectors(vol->dev->ctx, vol->first_fat_sector,
		                            vol->bpb.sectors_per_fat, fat) != 0) {
			return FAT12_ERR_WRITE;
		}
		return FAT12_OK;
	}
#endif
	for (i = 0u; i < vol->bpb.num_fats; i++) {
		uint32_t lba = vol->first_fat_sector +
		               (uint32_t)i * (uint32_t)vol->bpb.sectors_per_fat;
		if (vol->dev->write_sectors(vol->dev->ctx, lba,
		                            vol->bpb.sectors_per_fat, fat) != 0) {
			return FAT12_ERR_WRITE;
		}
	}
	return FAT12_OK;
}

/* Parse "NAME.EXT" / "NAME" into the 11-byte space-padded upper-case 8.3 fields
 * (filename[8] + extension[3]). Rejects '\\' / ':' (no subdir/drive). Returns
 * FAT12_OK on success, FAT12_ERR_NOT_FOUND on a malformed name (treated as "no
 * such file" by callers). Mirrors the read-side format_83 inverse. */
static int parse_name83(const char *name83, uint8_t out11[11])
{
	uint32_t i;
	uint32_t pos;
	const char *p;
	int field;

	for (i = 0u; i < 11u; i++) {
		out11[i] = 0x20u;   /* space pad */
	}

	p     = name83;
	field = 0;              /* 0 = name (0..7), 1 = ext (8..10) */
	pos   = 0u;

	for (; *p; p++) {
		char c = *p;
		if (c == '\\' || c == ':') {
			return FAT12_ERR_NOT_FOUND;   /* subdir / drive: unsupported */
		}
		if (c == '.') {
			if (field == 1) {
				return FAT12_ERR_NOT_FOUND;   /* a second dot is malformed */
			}
			field = 1;
			pos   = 0u;
			continue;
		}
		{
			uint32_t base = (field == 0) ? 0u : 8u;
			uint32_t cap  = (field == 0) ? 8u : 3u;
			if (pos >= cap) {
				return FAT12_ERR_NOT_FOUND;   /* field overflow */
			}
			if (c >= 'a' && c <= 'z') {
				c = (char)(c - ('a' - 'A'));  /* upper-case ASCII */
			}
			out11[base + pos] = (uint8_t)c;
			pos++;
		}
	}
	if (out11[0] == 0x20u) {
		return FAT12_ERR_NOT_FOUND;   /* empty name */
	}
	return FAT12_OK;
}

/* The highest valid data cluster index (clusters 2..last inclusive). */
static uint16_t fat12_last_cluster(const fat12_volume_t *vol)
{
	return (uint16_t)(FAT12_FIRST_DATA_CLUSTER + vol->total_clusters - 1u);
}

/* Find the lowest FREE data cluster (entry == 0x000) at or after `from`, in
 * [2..last]. Returns 0 if none free (the volume is full). */
static uint16_t fat12_find_free(const fat12_volume_t *vol, const void *fat,
                                uint32_t fat_len, uint16_t from)
{
	uint16_t c;
	uint16_t last = fat12_last_cluster(vol);

	if (from < FAT12_FIRST_DATA_CLUSTER) {
		from = FAT12_FIRST_DATA_CLUSTER;
	}
	for (c = from; c <= last; c++) {
		uint16_t v;
		if (fat12_next_cluster(vol, fat, fat_len, c, &v) != FAT12_OK) {
			return 0u;   /* out of range -> treat as no space (fail loud upstream) */
		}
		if (fat12_is_free(v)) {
			return c;
		}
	}
	return 0u;   /* no free cluster */
}

/* Free a whole cluster chain in the in-memory FAT (set each entry to 0x000).
 * Bounded by the cluster count (anti-hang). Does NOT flush (caller flushes). */
static int fat12_free_chain(const fat12_volume_t *vol, void *fat,
                            uint32_t fat_len, uint16_t start)
{
	uint16_t cur = start;
	uint32_t steps = 0u;
	uint32_t max   = vol->total_clusters + 2u;

	if (start < FAT12_FIRST_DATA_CLUSTER) {
		return FAT12_OK;   /* nothing to free (size-0 / no chain) */
	}
	for (;;) {
		uint16_t next;
		int rc = fat12_next_cluster(vol, fat, fat_len, cur, &next);
		if (rc != FAT12_OK) {
			return rc;
		}
		rc = fat12_set_entry(fat, fat_len, cur, FAT12_FREE);
		if (rc != FAT12_OK) {
			return rc;
		}
		if (chain_is_eoc(vol, next) || chain_is_free(vol, next) || chain_is_bad(vol, next)) {
			break;
		}
		cur = next;
		if (++steps > max) {
			return FAT12_ERR_CHAIN;   /* cyclic/corrupt: do not loop forever */
		}
	}
	return FAT12_OK;
}

/* Grow a SUBDIRECTORY by ONE cluster (beads initech-zs24; subdir WRITE side).
 * Walk start_cluster's chain to its EOC tail, claim a free cluster, zero-fill
 * its data sectors on disk (so the freshly exposed dir slots read as 0x00 =
 * end-of-directory, exactly as mtools/DOS leaves a grown directory), link the
 * old tail -> new cluster, mark the new cluster EOC, and flush BOTH FAT copies.
 * This is the same allocate-then-commit discipline the file-extend path uses
 * (Rule 2): the new cluster is reserved EOC, zero-filled BEFORE the link is
 * committed, and on a zero-fill write failure the allocation is rolled back so
 * no orphan cluster is left. `cluster_buf` (>= sectors_per_cluster*512) is the
 * zero-fill scratch. The new tail cluster is returned in *out_new_cluster (so
 * the caller knows where the new slots live). Anti-hang bounded (Rule 2).
 * Ref (Law 1): Microsoft FAT spec (a directory is a cluster chain that grows
 *   like a file; a new dir cluster is zero-filled so the 0x00 sentinel still
 *   terminates it); DOS 3.3 directory-full behavior. */
static int fat12_grow_dir(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                          uint16_t start_cluster, void *cluster_buf,
                          uint16_t *out_new_cluster)
{
	uint32_t bytes_per_cluster = (uint32_t)vol->bpb.sectors_per_cluster *
	                             (uint32_t)vol->bpb.bytes_per_sector;
	uint16_t cur   = start_cluster;
	uint32_t steps = 0u;
	uint32_t max   = vol->total_clusters + 2u;
	uint16_t newc;
	uint32_t lba;
	uint8_t *cb = (uint8_t *)cluster_buf;
	uint32_t b;
	int      rc;

	if (fat == NULL || cluster_buf == NULL || out_new_cluster == NULL) {
		return FAT12_ERR_NULL;
	}
	if (vol->dev->write_sectors == NULL) {
		return FAT12_ERR_WRITE;
	}

#ifdef FAT12_MUTATE_GROW_NOOP
	/* MUTANT (Rule 6; make test-zs24-mutant only): REFUSE to grow the directory
	 * -- return DIR_FULL as if the subdir could not be extended. The boundary-
	 * crossing CREATE then fails and the file never lands, so the GROW
	 * differential (mtools/python list the boundary file) goes RED. This proves
	 * the grow path is actually REACHED + relied upon. NEVER in a real build. */
	(void)fat_len; (void)start_cluster;
	(void)bytes_per_cluster; (void)cur; (void)steps; (void)max; (void)newc;
	(void)lba; (void)cb; (void)b; (void)rc;
	return FAT12_ERR_DIR_FULL;
#else

	/* Walk to the EOC tail of the existing chain. */
	for (;;) {
		uint16_t next;
		if (!fat12_cluster_in_range(vol, cur) || chain_is_free(vol, cur) ||
		    chain_is_bad(vol, cur)) {
			return FAT12_ERR_CHAIN;
		}
		rc = fat12_next_cluster(vol, fat, fat_len, cur, &next);
		if (rc != FAT12_OK) {
			return rc;
		}
		if (chain_is_eoc(vol, next)) {
			break;
		}
		cur = next;
		if (++steps > max) {
			return FAT12_ERR_CHAIN;
		}
	}

	/* Claim + reserve a free cluster (mark EOC immediately so find_free skips). */
	newc = fat12_find_free(vol, fat, fat_len, (uint16_t)(cur + 1u));
	if (newc == 0u) {
		return FAT12_ERR_NO_SPACE;
	}
#ifndef FAT12_MUTATE_GROW_NO_EOC
	rc = fat12_set_entry(fat, fat_len, newc, FAT12_EOC_VALUE);
	if (rc != FAT12_OK) {
		return rc;
	}
#else
	/* MUTANT (Rule 6; make test-zs24-mutant only): SKIP marking the freshly
	 * claimed cluster end-of-chain. The new cluster's FAT entry stays 0x000
	 * (free); the old-tail->newc link below then points at a "free" cluster, so
	 * the directory chain is BROKEN at the join -- a reader (mtools/python)
	 * walking the chain stops at (or rejects) the unterminated/free 2nd cluster
	 * and the boundary-crossing dir entry is unreachable -> the GROW differential
	 * goes RED. This proves the chain RELINK (old-tail -> new EOC cluster) is
	 * what makes the grown slots reachable. NEVER in a real build. */
#endif

	/* Zero-fill the new cluster on disk BEFORE committing the link (so the new
	 * slots read as 0x00 end-of-directory; a failure rolls back the alloc). */
	for (b = 0u; b < bytes_per_cluster; b++) {
		cb[b] = 0u;
	}
	lba = BPB_CLUSTER_LBA(&vol->bpb, newc);
	if (vol->dev->write_sectors(vol->dev->ctx, lba,
	                            vol->bpb.sectors_per_cluster, cb) != 0) {
#ifndef FAT12_MUTATE_GROWDIR_NO_ZEROFILL_ROLLBACK
		(void)fat12_set_entry(fat, fat_len, newc, FAT12_FREE);
		(void)fat12_flush_fats(vol, fat, fat_len);
#else
		/* MUTANT (Rule 6; make test-fat-fault-rollback-mutant only): SKIP the
		 * zero-fill-fail rollback. The cluster the grow CLAIMED (marked EOC in
		 * memory) is NOT returned to FREE, so it LEAKS while the grow -- and the
		 * fat12_create that drove it -- still fails. The free-count / chain-len
		 * assertion in scenario [B] must go RED. NEVER define in a real build. */
#endif
		return FAT12_ERR_WRITE;
	}

	/* Link the old tail -> new cluster, flush both FAT copies. */
	rc = fat12_set_entry(fat, fat_len, cur, newc);
	if (rc != FAT12_OK) {
		(void)fat12_set_entry(fat, fat_len, newc, FAT12_FREE);
		(void)fat12_flush_fats(vol, fat, fat_len);
		return rc;
	}
	rc = fat12_flush_fats(vol, fat, fat_len);
	if (rc != FAT12_OK) {
		return rc;
	}
	*out_new_cluster = newc;
	return FAT12_OK;
#endif /* FAT12_MUTATE_GROW_NOOP */
}

/* fat12_shrink_dir_tail -- the EXACT inverse of a single fat12_grow_dir step:
 * detach `appended` (the last cluster fat12_grow_dir linked onto the chain at
 * `start_cluster`) by walking to the cluster that points at it, restoring THAT
 * cluster to EOC, freeing `appended`, and flushing both FAT copies. Used as the
 * rollback for a MKDIR whose own-entry write into the just-grown parent slot
 * fails AFTER the parent grew (beads initech-m0bp): without this the freshly-
 * appended parent cluster would leak (Rule 2/Rule 3 -- the grow path must roll
 * back on a later write failure, exactly the discipline fat12_write_file uses
 * for its partial-allocation rollback). Best-effort: a chain-decode error
 * during rollback is swallowed (we are already on a failure path); the caller's
 * original error is the one returned. */
static void fat12_shrink_dir_tail(const fat12_volume_t *vol, void *fat,
                                  uint32_t fat_len, uint16_t start_cluster,
                                  uint16_t appended)
{
	uint16_t cur   = start_cluster;
	uint32_t steps = 0u;
	uint32_t max   = vol->total_clusters + 2u;

	if (fat == NULL || appended == 0u) {
		return;
	}
	/* Walk to the cluster whose link is `appended`; restore it to EOC. */
	for (;;) {
		uint16_t next;
		if (!fat12_cluster_in_range(vol, cur) || chain_is_free(vol, cur) ||
		    chain_is_bad(vol, cur)) {
			return;   /* corrupt chain -- give up the rollback (best-effort) */
		}
		if (fat12_next_cluster(vol, fat, fat_len, cur, &next) != FAT12_OK) {
			return;
		}
		if (next == appended) {
			(void)fat12_set_entry(fat, fat_len, cur, FAT12_EOC_VALUE);
			break;
		}
		if (chain_is_eoc(vol, next)) {
			return;   /* appended not found in the chain -- nothing to undo */
		}
		cur = next;
		if (++steps > max) {
			return;
		}
	}
	(void)fat12_set_entry(fat, fat_len, appended, FAT12_FREE);
	(void)fat12_flush_fats(vol, fat, fat_len);
}

/* ------------------------------------------------------------------------ *
 * Subdir-aware dir-slot addressing (beads initech-zs24; subdir WRITE side).
 *
 * A directory SLOT is a 0-based 32-byte-entry index WITHIN a directory. For the
 * fixed ROOT directory it is the linear index into the root-dir region (slot
 * i lives at root_dir_sector + i/per_sector, byte (i%per_sector)*32) -- exactly
 * the historical root-slot semantics, so the root path stays byte-identical.
 * For a SUBDIRECTORY (is_root==0, start_cluster != 0) it is the linear index
 * down the subdir's CLUSTER CHAIN: slot i lives in the (i / ents_per_cluster)-th
 * cluster of the chain at byte (i % ents_per_cluster)*32. The cluster walk
 * mirrors fat12_read_dir's incremental fat12_next_cluster step with the same
 * anti-hang guards (Rule 2). The DATA SECTOR holding the slot is read-modified-
 * written back in place (the root path read-modify-writes a root-dir sector).
 * ------------------------------------------------------------------------ */

/* Resolve a SUBDIR slot index to the data-sector LBA that holds it + the byte
 * offset of the 32-byte entry within that sector. Walks `start_cluster`'s chain
 * (slot / ents_per_cluster) clusters down, then within that cluster picks the
 * sector + in-sector offset. Anti-hang bounded by total_clusters+2 (Rule 2). On
 * a chain too short for the slot, returns FAT12_ERR_DIR_FULL (the slot is past
 * the allocated directory -- the caller grows the chain). `fat`/`fat_len` decode
 * the links. Returns FAT12_OK with *out_lba + *out_in_sec set. */
static int fat12_subdir_slot_lba(const fat12_volume_t *vol, const void *fat,
                                  uint32_t fat_len, uint16_t start_cluster,
                                  uint32_t slot, uint32_t *out_lba,
                                  uint32_t *out_in_sec)
{
	uint32_t per_sector = BLOCKDEV_SECTOR_SIZE / 32u;
	uint32_t ents_per_cluster = per_sector * (uint32_t)vol->bpb.sectors_per_cluster;
	uint32_t want_cluster = slot / ents_per_cluster;
	uint32_t in_cluster   = slot % ents_per_cluster;
	uint32_t sec_in_clus  = in_cluster / per_sector;
	uint16_t cur          = start_cluster;
	uint32_t steps        = 0u;
	uint32_t max_steps    = vol->total_clusters + 2u;
	uint32_t k;

	for (k = 0u; k < want_cluster; k++) {
		uint16_t next;
		int      rc;
		if (!fat12_cluster_in_range(vol, cur) || chain_is_free(vol, cur) ||
		    chain_is_bad(vol, cur)) {
			return FAT12_ERR_CHAIN;
		}
		rc = fat12_next_cluster(vol, fat, fat_len, cur, &next);
		if (rc != FAT12_OK) {
			return rc;
		}
		if (chain_is_eoc(vol, next)) {
			return FAT12_ERR_DIR_FULL;   /* slot past the allocated chain */
		}
		cur = next;
		if (++steps > max_steps) {
			return FAT12_ERR_CHAIN;      /* cyclic / corrupt */
		}
	}
	if (!fat12_cluster_in_range(vol, cur)) {
		return FAT12_ERR_CHAIN;
	}
	*out_lba    = BPB_CLUSTER_LBA(&vol->bpb, cur) + sec_in_clus;
	*out_in_sec = (in_cluster % per_sector) * 32u;
	return FAT12_OK;
}

/* Read a single dir entry (32 bytes) at slot `slot` of the directory
 * (is_root / start_cluster) into *e. Root: the root-dir region; subdir: the
 * cluster chain (beads initech-zs24). The root path is byte-identical to the
 * historical fat12_read_dirent_local. */
static int fat12_read_dirent_in_dir(const fat12_volume_t *vol, const void *fat,
                                    uint32_t fat_len, int is_root,
                                    uint16_t start_cluster, uint32_t slot,
                                    dir_entry_t *e, void *sector_buf)
{
	uint32_t per_sector = BLOCKDEV_SECTOR_SIZE / 32u;
	uint32_t lba;
	uint32_t in_sec;
	const uint8_t *sb = (const uint8_t *)sector_buf;
	uint8_t *dst      = (uint8_t *)e;
	uint32_t k;

	if (is_root) {
		uint32_t sec_index = slot / per_sector;
		if (sec_index >= vol->root_dir_sectors) {
			return FAT12_ERR_DIR_FULL;
		}
		lba    = vol->root_dir_sector + sec_index;
		in_sec = (slot % per_sector) * 32u;
	} else {
		int rc = fat12_subdir_slot_lba(vol, fat, fat_len, start_cluster, slot,
		                               &lba, &in_sec);
		if (rc != FAT12_OK) {
			return rc;
		}
	}
	if (vol->dev->read_sectors(vol->dev->ctx, lba, 1u, sector_buf) != 0) {
		return FAT12_ERR_READ;
	}
	for (k = 0u; k < 32u; k++) {
		dst[k] = sb[in_sec + k];
	}
	return FAT12_OK;
}

/* Read a single dir entry (32 bytes) from its root-dir slot into *e (the root-
 * only thin wrapper; byte-identical historical behavior). */
static int fat12_read_dirent_local(const fat12_volume_t *vol, uint32_t slot,
                                   dir_entry_t *e, void *sector_buf)
{
	return fat12_read_dirent_in_dir(vol, NULL, 0u, 1, 0u, slot, e, sector_buf);
}

/* Write a single dir entry (32 bytes) to slot `slot` of the directory
 * (is_root / start_cluster) on disk: read the containing data sector, overwrite
 * the 32-byte entry in place, write the sector back (beads initech-zs24). Root:
 * the root-dir region (byte-identical to the historical fat12_write_dirent);
 * subdir: the cluster chain via fat12_subdir_slot_lba. `fat`/`fat_len` decode
 * the subdir links (unused for the root). */
static int fat12_write_dirent_in_dir(const fat12_volume_t *vol, const void *fat,
                                     uint32_t fat_len, int is_root,
                                     uint16_t start_cluster, uint32_t slot,
                                     const dir_entry_t *e, void *sector_buf)
{
	uint32_t per_sector = BLOCKDEV_SECTOR_SIZE / 32u;
	uint32_t lba;
	uint32_t in_sec;
	uint8_t *sb        = (uint8_t *)sector_buf;
	const uint8_t *src = (const uint8_t *)e;
	uint32_t k;

	if (vol->dev->write_sectors == NULL) {
		return FAT12_ERR_WRITE;
	}
#ifdef FAT12_MUTATE_SUBDIR_WRITE_ROOTSLOT
	/* MUTANT (Rule 6; make test-zs24-mutant only): write a SUBDIR entry back to a
	 * ROOT-dir slot of the same index instead of the subdir cluster-chain slot.
	 * The subdir dir entry never lands (its size/start_cluster are never patched
	 * on disk), so the read-back via mcopy/python sees the wrong/zero entry and
	 * the differential goes RED. The root path (is_root already 1) is unchanged.
	 * NEVER define in a real build. */
	is_root = 1;
#endif
	if (is_root) {
		uint32_t sec_index = slot / per_sector;
		if (sec_index >= vol->root_dir_sectors) {
			return FAT12_ERR_DIR_FULL;
		}
		lba    = vol->root_dir_sector + sec_index;
		in_sec = (slot % per_sector) * 32u;
	} else {
		int rc = fat12_subdir_slot_lba(vol, fat, fat_len, start_cluster, slot,
		                               &lba, &in_sec);
		if (rc != FAT12_OK) {
			return rc;
		}
	}
	if (vol->dev->read_sectors(vol->dev->ctx, lba, 1u, sb) != 0) {
		return FAT12_ERR_READ;
	}
	for (k = 0u; k < 32u; k++) {
		sb[in_sec + k] = src[k];
	}
	if (vol->dev->write_sectors(vol->dev->ctx, lba, 1u, sb) != 0) {
		return FAT12_ERR_WRITE;
	}
	return FAT12_OK;
}

/* Write a single dir entry (32 bytes) to its root-dir slot on disk (the root-
 * only thin wrapper; byte-identical historical behavior). */
static int fat12_write_dirent(const fat12_volume_t *vol, uint32_t slot,
                              const dir_entry_t *e, void *sector_buf)
{
	return fat12_write_dirent_in_dir(vol, NULL, 0u, 1, 0u, slot, e, sector_buf);
}

/* Scan ONE 512-byte directory sector already in `sb` (its first slot index is
 * `base_slot`): on an exact 11-byte 8.3 match set match_slot/match/found and
 * return 1 (matched -- stop); on the 0x00 end-of-directory sentinel record the
 * first free slot (if none seen) and return 2 (end -- stop); record the first
 * deleted (reusable) slot otherwise. Returns 0 to continue to the next sector.
 * Shared by the root + subdir scan so the per-entry logic is identical. */
static int fat12_scan_sector(const uint8_t *sb, uint32_t base_slot,
                             const uint8_t want11[11], uint32_t *free_slot,
                             int *have_free, uint32_t *match_slot,
                             dir_entry_t *match, int *found)
{
	uint32_t per_sector = BLOCKDEV_SECTOR_SIZE / 32u;
	uint32_t i;
	uint32_t slot = base_slot;

	for (i = 0u; i < per_sector; i++, slot++) {
		const uint8_t *e = sb + i * 32u;
		uint8_t first = e[0];

		if (first == DIR_NAME_FREE) {
			/* End-of-directory: this slot (and all after) are free. The first
			 * free slot for placement is here if none seen earlier. */
			if (!*have_free) {
				*free_slot = slot;
				*have_free = 1;
			}
			return 2;   /* no match exists beyond the sentinel */
		}
		if (first == DIR_NAME_DELETED) {
			if (!*have_free) {
				*free_slot = slot;
				*have_free = 1;
			}
			continue;
		}
		/* LFN slots are not a name match; skip for matching, never reuse. */
		if (e[11] == FAT12_ATTR_LFN) {
			continue;
		}
		/* Compare the 11 raw 8.3 bytes for an exact match. */
		{
			int eq = 1;
			uint32_t k;
			for (k = 0u; k < 11u; k++) {
				if (e[k] != want11[k]) {
					eq = 0;
					break;
				}
			}
			if (eq) {
				uint32_t m;
				uint8_t *dst = (uint8_t *)match;
				for (m = 0u; m < 32u; m++) {
					dst[m] = e[m];
				}
				*match_slot = slot;
				*found      = 1;
				return 1;
			}
		}
	}
	return 0;   /* continue to the next sector */
}

/* Scan the directory (is_root / start_cluster) for an 8.3 entry (beads
 * initech-zs24, generalizing fat12_scan_root). If a non-deleted entry's 8.3
 * fields == want11, set *match_slot and copy it to *match (found=1); else record
 * the FIRST reusable slot (deleted 0xE5 or free 0x00) in free_slot/have_free.
 *
 * ROOT (is_root): the fixed root region -- byte-identical to the historical
 * fat12_scan_root (the per-entry logic is fat12_scan_sector, shared).
 * SUBDIR: walk start_cluster's CLUSTER CHAIN (mirroring fat12_read_dir's
 * incremental fat12_next_cluster step + anti-hang guards), tracking the LINEAR
 * 32-byte slot index across clusters. If the chain ends at EOC with NO 0x00
 * sentinel and NO reusable slot recorded, the directory is FULL: *have_free
 * stays 0 and *free_slot is the slot index JUST PAST the last cluster (so the
 * caller can grow the chain by one cluster to place the new entry there).
 * Returns FAT12_OK (found flagged via *found) or a read/chain error. */
static int fat12_scan_dir(const fat12_volume_t *vol, const void *fat,
                          uint32_t fat_len, int is_root, uint16_t start_cluster,
                          void *sector_buf, const uint8_t want11[11],
                          uint32_t *free_slot, int *have_free,
                          uint32_t *match_slot, dir_entry_t *match, int *found)
{
	uint32_t per_sector = BLOCKDEV_SECTOR_SIZE / 32u;

	*have_free = 0;
	*found     = 0;

	if (is_root) {
		uint32_t slot = 0u;
		uint32_t s;
		for (s = 0u; s < vol->root_dir_sectors; s++) {
			const uint8_t *sb;
			int r;
			if (vol->dev->read_sectors(vol->dev->ctx, vol->root_dir_sector + s,
			                           1u, sector_buf) != 0) {
				return FAT12_ERR_READ;
			}
			sb = (const uint8_t *)sector_buf;
			r = fat12_scan_sector(sb, slot, want11, free_slot, have_free,
			                      match_slot, match, found);
			if (r != 0) {
				return FAT12_OK;   /* matched or hit the 0x00 sentinel */
			}
			slot += per_sector;
		}
		/* Scanned the whole fixed root dir with no 0x00 sentinel and no match:
		 * the directory is full unless a deleted slot was recorded. */
		return FAT12_OK;
	}

	/* ---- Subdirectory: walk the cluster chain. ---- */
	{
		uint16_t cur          = start_cluster;
		uint32_t slot         = 0u;
		uint32_t steps        = 0u;
		uint32_t max_steps    = vol->total_clusters + 2u;
		uint32_t spc          = (uint32_t)vol->bpb.sectors_per_cluster;

		if (fat == NULL) {
			return FAT12_ERR_NULL;
		}
		for (;;) {
			uint32_t lba;
			uint32_t s;
			uint16_t next;
			int      rc;

			if (!fat12_cluster_in_range(vol, cur) || chain_is_free(vol, cur) ||
			    chain_is_bad(vol, cur)) {
				return FAT12_ERR_CHAIN;
			}
			lba = BPB_CLUSTER_LBA(&vol->bpb, cur);
			for (s = 0u; s < spc; s++) {
				const uint8_t *sb;
				int r;
				if (vol->dev->read_sectors(vol->dev->ctx, lba + s, 1u,
				                           sector_buf) != 0) {
					return FAT12_ERR_READ;
				}
				sb = (const uint8_t *)sector_buf;
				r = fat12_scan_sector(sb, slot, want11, free_slot, have_free,
				                      match_slot, match, found);
				if (r != 0) {
					return FAT12_OK;   /* matched or hit the 0x00 sentinel */
				}
				slot += per_sector;
			}
			rc = fat12_next_cluster(vol, fat, fat_len, cur, &next);
			if (rc != FAT12_OK) {
				return rc;
			}
			if (chain_is_eoc(vol, next)) {
				/* Chain ended with no 0x00 sentinel and (if no deleted slot was
				 * found) no free slot: the directory is full at exactly `slot`,
				 * the index just past the last cluster. Leaving *have_free 0
				 * signals the caller to grow the chain to place a new entry. */
				if (!*have_free) {
					*free_slot = slot;
				}
				return FAT12_OK;
			}
			cur = next;
			if (++steps > max_steps) {
				return FAT12_ERR_CHAIN;   /* cyclic / corrupt */
			}
		}
	}
}

/* fat12_scan_root -- the root-only thin wrapper (byte-identical historical
 * behavior; routes through fat12_scan_dir with is_root=1). */
static int fat12_scan_root(const fat12_volume_t *vol, void *sector_buf,
                           const uint8_t want11[11], uint32_t *free_slot,
                           int *have_free, uint32_t *match_slot,
                           dir_entry_t *match, int *found)
{
	return fat12_scan_dir(vol, NULL, 0u, 1, 0u, sector_buf, want11,
	                      free_slot, have_free, match_slot, match, found);
}

/*
 * fat12_find_slot -- locate an entry by 8.3 name AND return its root-dir SLOT
 * index (beads initech-0qh; epic initech-6qy). fat12_find returns only the dir
 * entry; the multi-tenant file backend (os/milton/fileio_fat.c) needs the slot
 * too so a later positioned write_at() / fat12_read_dir_entry can patch/refresh
 * the entry in place. Implemented over fat12_scan_root (the same exact-11-byte
 * matcher fat12_create uses), so the slot semantics match fat12_create's
 * *out_slot.
 *
 * Returns FAT12_OK with *out_entry + *out_slot set on a match; FAT12_ERR_NOT_FOUND
 * if no entry matches; a parse error (bad name) or read error propagated. Note:
 * fat12_scan_root stops at the 0x00 end-of-directory sentinel, so a name only
 * matches if its entry precedes the sentinel (the normal case).
 */
int fat12_find_slot(const fat12_volume_t *vol, void *sector_buf,
                    const char *name83, dir_entry_t *out_entry,
                    uint32_t *out_slot)
{
	uint8_t     want11[11];
	uint32_t    free_slot = 0u;
	int         have_free = 0;
	uint32_t    match_slot = 0u;
	dir_entry_t match;
	int         found = 0;
	int         rc;

	if (vol == NULL || sector_buf == NULL || name83 == NULL ||
	    out_entry == NULL || out_slot == NULL) {
		return FAT12_ERR_NULL;
	}

	rc = parse_name83(name83, want11);
	if (rc != FAT12_OK) {
		return rc;
	}
	rc = fat12_scan_root(vol, sector_buf, want11, &free_slot, &have_free,
	                     &match_slot, &match, &found);
	if (rc != FAT12_OK) {
		return rc;
	}
	if (!found) {
		return FAT12_ERR_NOT_FOUND;
	}
	*out_entry = match;
	*out_slot  = match_slot;
	return FAT12_OK;
}

/*
 * fat12_find_slot_in -- like fat12_find_slot, but locate the 8.3 `name83` in the
 * directory whose first data cluster is `parent_dir_start` (0 == the fixed root,
 * byte-identical to fat12_find_slot) and return the entry's slot WITHIN that
 * directory (beads initech-zs24). For a subdir the slot is the linear cluster-
 * chain index a later fat12_write_partial / fat12_read_dir_entry_in uses to
 * write-back the entry. `fat`/`fat_len` decode the subdir links. Returns
 * FAT12_OK with *out_entry + *out_slot set; FAT12_ERR_NOT_FOUND if no match; a
 * parse / read / chain error propagated.
 */
int fat12_find_slot_in(const fat12_volume_t *vol, const void *fat,
                       uint32_t fat_len, uint16_t parent_dir_start,
                       void *sector_buf, const char *name83,
                       dir_entry_t *out_entry, uint32_t *out_slot)
{
	uint8_t     want11[11];
	uint32_t    free_slot = 0u;
	int         have_free = 0;
	uint32_t    match_slot = 0u;
	dir_entry_t match;
	int         found = 0;
	int         rc;

	if (vol == NULL || sector_buf == NULL || name83 == NULL ||
	    out_entry == NULL || out_slot == NULL) {
		return FAT12_ERR_NULL;
	}

	rc = parse_name83(name83, want11);
	if (rc != FAT12_OK) {
		return rc;
	}
	rc = fat12_scan_dir(vol, fat, fat_len, parent_dir_start == 0u,
	                    parent_dir_start, sector_buf, want11, &free_slot,
	                    &have_free, &match_slot, &match, &found);
	if (rc != FAT12_OK) {
		return rc;
	}
	if (!found) {
		return FAT12_ERR_NOT_FOUND;
	}
	*out_entry = match;
	*out_slot  = match_slot;
	return FAT12_OK;
}

/*
 * fat12_read_dir_entry -- re-read the 32-byte directory entry at root-dir slot
 * `slot` into *out_entry (beads initech-0qh). Public wrapper over the internal
 * single-slot read; the file backend uses it after a positioned write_at() to
 * refresh the SFT's dir_entry copy (size + start_cluster patched on disk by
 * fat12_write_partial). `sector_buf` (>=512) is scratch. Returns FAT12_OK, or a
 * read error / FAT12_ERR_DIR_FULL if the slot is out of range / FAT12_ERR_NULL.
 */
int fat12_read_dir_entry(const fat12_volume_t *vol, void *sector_buf,
                         uint32_t slot, dir_entry_t *out_entry)
{
	if (vol == NULL || sector_buf == NULL || out_entry == NULL) {
		return FAT12_ERR_NULL;
	}
	return fat12_read_dirent_local(vol, slot, out_entry, sector_buf);
}

/*
 * fat12_read_dir_entry_in -- re-read the 32-byte directory entry at slot `slot`
 * of the directory whose first data cluster is `parent_dir_start` (0 == the
 * fixed root) into *out_entry (beads initech-zs24). The subdir-aware counterpart
 * of fat12_read_dir_entry: the file backend's positioned write_at() refresh uses
 * it so a subdir file's SFT copy is refreshed from the SUBDIR slot, not a root
 * slot. `fat`/`fat_len` decode the subdir links (unused for the root, which is
 * byte-identical to fat12_read_dir_entry). Returns FAT12_OK, or a read/chain
 * error / FAT12_ERR_DIR_FULL (slot out of range) / FAT12_ERR_NULL.
 */
int fat12_read_dir_entry_in(const fat12_volume_t *vol, const void *fat,
                            uint32_t fat_len, uint16_t parent_dir_start,
                            void *sector_buf, uint32_t slot,
                            dir_entry_t *out_entry)
{
	if (vol == NULL || sector_buf == NULL || out_entry == NULL) {
		return FAT12_ERR_NULL;
	}
	return fat12_read_dirent_in_dir(vol, fat, fat_len, parent_dir_start == 0u,
	                                parent_dir_start, slot, out_entry,
	                                sector_buf);
}

/*
 * fat12_set_dirent_time -- patch ONLY mtime/mdate of the dir entry at `slot` of
 * the directory at `parent_dir_start` (0 == root) and flush it back. The thin
 * single-primitive behind AH=57h AL=01h SET (beads initech-qekc). Read-modify-
 * write of the one 32-byte entry: re-read it, overwrite the two packed words
 * VERBATIM (no encode/decode -- the caller hands on-disk packed CX/DX), write it
 * back via the SAME primitive WRITE uses (fat12_write_dirent_in_dir). All other
 * fields are preserved -- the INVERSE of the FAT12_FIXED_MTIME stamping CREATE
 * does. Ref: DOS 3.3 PRM AH=57h; spec/dos_structs.h dir_entry_t (0x16/0x18).
 */
int fat12_set_dirent_time(const fat12_volume_t *vol, const void *fat,
                          uint32_t fat_len, uint16_t parent_dir_start,
                          uint32_t slot, uint16_t mtime, uint16_t mdate,
                          void *sector_buf)
{
	int         is_root = (parent_dir_start == 0u);
	dir_entry_t de;
	int         rc;

	if (vol == NULL || sector_buf == NULL) {
		return FAT12_ERR_NULL;
	}
	if (vol->dev == NULL || vol->dev->write_sectors == NULL ||
	    vol->dev->read_sectors == NULL) {
		return FAT12_ERR_WRITE;
	}

	/* Re-read the live on-disk entry (subdir-aware: dir_start != 0 walks the
	 * parent's cluster chain). */
	rc = fat12_read_dirent_in_dir(vol, fat, fat_len, is_root, parent_dir_start,
	                              slot, &de, sector_buf);
	if (rc != FAT12_OK) {
		return rc;
	}

	/* Overwrite ONLY the two packed timestamp words, VERBATIM. Everything else
	 * (name / attr / start_cluster / size) is preserved. */
	de.mtime = mtime;
#ifndef FAT12_MUTATE_SETTIME_DROP_MDATE
	de.mdate = mdate;
#else
	/* MUTANT 1 (Rule 6; make test-qekc-mutant only): write ONLY mtime, DROP the
	 * mdate=DX assignment so the on-disk date keeps its old (0/0 baseline) value.
	 * The --stat-path-time / mdir DATE differential goes RED (MDATE mismatches).
	 * The (void) keeps -Werror=unused-parameter quiet. NEVER in a real build. */
	(void)mdate;
#endif

#ifndef FAT12_MUTATE_SETTIME_NO_FLUSH
	return fat12_write_dirent_in_dir(vol, fat, fat_len, is_root, parent_dir_start,
	                                 slot, &de, sector_buf);
#else
	/* MUTANT 3 (Rule 6; make test-qekc-mutant only): SKIP the write-back flush --
	 * the patched `de` is computed but never written to disk, so the on-disk
	 * 0x16/0x18 words keep their baseline value. The SET reports success (return
	 * FAT12_OK) and a SAME-handle GET (which reads the in-memory SFT copy the
	 * dispatcher refreshed) stays GREEN, but the on-disk DIFFERENTIAL (after
	 * re-mount) goes RED -- proving the flush is load-bearing for persistence.
	 * NEVER define in a real build. */
	return FAT12_OK;
#endif
}

/*
 * fat12_get_attr -- the AL=00 GET leg of INT 21h AH=43h CHMOD (beads
 * initech-b53d). Scan the directory for `name83` (subdir-aware via
 * fat12_scan_dir, exactly as fat12_unlink); on a match, copy the matched
 * entry's attribute byte (offset 0x0B) into *out_attr -- INCLUDING a directory
 * (0x10) or volume-label (0x08) entry. GET is a pure READ: reporting the real
 * attribute byte of a directory (CX=0x10) IS the faithful DOS behaviour and the
 * canonical "does this directory exist / stat a path" idiom (ATTRIB, dir-exists
 * probes all use AH=4300h on a directory and expect CF=0/CX=0x10). RBIL/PRM
 * AH=4300h: "CF clear if successful / CX = file attributes" with NO directory
 * exclusion -- the dir/vol-label reject is SET-only (fat12_set_attr), NOT GET.
 * A missing name -> FAT12_ERR_NOT_FOUND. Read-only: the volume need not be
 * writable. Ref: RBIL INT 21h/AX=4300h; DOS 3.3 PRM AH=43h AL=00;
 * spec/dos_structs.h (attribute 0x0B). */
int fat12_get_attr(const fat12_volume_t *vol, const void *fat, uint32_t fat_len,
                   const char *name83, uint16_t parent_dir_start,
                   void *sector_buf, uint8_t *out_attr)
{
	uint8_t     want11[11];
	uint32_t    free_slot = 0u;
	int         have_free = 0;
	uint32_t    match_slot = 0u;
	dir_entry_t match;
	int         found = 0;
	int         rc;
	int         is_root = (parent_dir_start == 0u);

	if (vol == NULL || name83 == NULL || sector_buf == NULL || out_attr == NULL) {
		return FAT12_ERR_NULL;
	}
	if (vol->dev == NULL || vol->dev->read_sectors == NULL) {
		return FAT12_ERR_READ;
	}

	rc = parse_name83(name83, want11);
	if (rc != FAT12_OK) {
		return FAT12_ERR_NOT_FOUND;
	}
	rc = fat12_scan_dir(vol, fat, fat_len, is_root, parent_dir_start, sector_buf,
	                    want11, &free_slot, &have_free, &match_slot, &match,
	                    &found);
	if (rc != FAT12_OK) {
		return rc;
	}
	if (!found) {
		return FAT12_ERR_NOT_FOUND;
	}
#ifdef FAT12_MUTATE_GETATTR_DIR_REJECT
	/* MUTANT (Rule 6; make test-b53d-mutant only): RE-INTRODUCE the
	 * (unfaithful) GET-on-directory reject that the b53d fidelity fix removed --
	 * deny a directory / volume-label entry instead of reporting its real 0x10
	 * attribute byte. The corrected "GET-on-dir -> CF=0/CX=0x10" contract (host
	 * oracle + the test-b53d 3-way differential) then goes RED. NEVER in a real
	 * build. Ref: RBIL AX=4300h (no directory exclusion on GET). */
	if ((match.attribute & (DIR_ATTR_DIRECTORY | DIR_ATTR_VOLLABEL)) != 0u) {
		return FAT12_ERR_ACCESS;
	}
#endif
	/* A directory (0x10) or volume-label (0x08) entry is reported VERBATIM: GET
	 * is a pure read and CX=0x10 is the faithful DOS answer (RBIL AX=4300h has no
	 * directory exclusion -- the reject is SET-only, fat12_set_attr). */
	*out_attr = match.attribute;
	return FAT12_OK;
}

/*
 * fat12_set_attr -- the AL=01 SET leg of INT 21h AH=43h CHMOD (beads
 * initech-b53d). Scan the directory for `name83` (subdir-aware), REJECT a
 * directory / volume-label TARGET and REJECT a new `attr` that itself sets the
 * Directory(0x10) or VolLabel(0x08) bit (DOS forbids re-typing a dirent via
 * CHMOD), both with FAT12_ERR_ACCESS BEFORE any write -- Rule 2: never silently
 * corrupt a dirent's type bits. Then read-modify-write ONLY the attribute byte
 * (offset 0x0B) and flush via the SAME primitive WRITE uses
 * (fat12_write_dirent_in_dir). Name / start_cluster / size AND mtime(0x16) /
 * mdate(0x18) are PRESERVED VERBATIM (Rule 11: the timestamp bytes are never
 * touched, so a CHMOD stays deterministic). A missing name -> FAT12_ERR_NOT_FOUND;
 * a read-only volume -> FAT12_ERR_WRITE. Ref: DOS 3.3 PRM AH=43h AL=01. */
int fat12_set_attr(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                   const char *name83, uint16_t parent_dir_start,
                   uint8_t attr, void *sector_buf)
{
	uint8_t     want11[11];
	uint32_t    free_slot = 0u;
	int         have_free = 0;
	uint32_t    match_slot = 0u;
	dir_entry_t match;
	int         found = 0;
	int         rc;
	int         is_root = (parent_dir_start == 0u);

	if (vol == NULL || name83 == NULL || sector_buf == NULL) {
		return FAT12_ERR_NULL;
	}
	if (vol->dev == NULL || vol->dev->write_sectors == NULL ||
	    vol->dev->read_sectors == NULL) {
		return FAT12_ERR_WRITE;
	}

	/* A new attribute that re-types the entry (sets Directory or VolLabel) is
	 * forbidden -- reject BEFORE the scan touches disk (Rule 2). */
#ifndef FAT12_MUTATE_SETATTR_NO_REJECT
	if ((attr & (DIR_ATTR_DIRECTORY | DIR_ATTR_VOLLABEL)) != 0u) {
		return FAT12_ERR_ACCESS;
	}
#endif

	rc = parse_name83(name83, want11);
	if (rc != FAT12_OK) {
		return FAT12_ERR_NOT_FOUND;
	}
	rc = fat12_scan_dir(vol, fat, fat_len, is_root, parent_dir_start, sector_buf,
	                    want11, &free_slot, &have_free, &match_slot, &match,
	                    &found);
	if (rc != FAT12_OK) {
		return rc;
	}
	if (!found) {
		return FAT12_ERR_NOT_FOUND;
	}
	/* The TARGET must be a regular file -- never CHMOD a directory or the
	 * volume-label entry (Rule 2). */
#ifndef FAT12_MUTATE_SETATTR_NO_REJECT
	if ((match.attribute & (DIR_ATTR_DIRECTORY | DIR_ATTR_VOLLABEL)) != 0u) {
		return FAT12_ERR_ACCESS;
	}
#endif

	/* Patch ONLY the attribute byte; name / cluster / size / mtime / mdate are
	 * left exactly as scanned (Rule 11: the timestamp bytes stay deterministic). */
	match.attribute = attr;
#ifndef FAT12_MUTATE_SETATTR_NO_FLUSH
	return fat12_write_dirent_in_dir(vol, fat, fat_len, is_root, parent_dir_start,
	                                 match_slot, &match, sector_buf);
#else
	/* MUTANT 3 (Rule 6; make test-b53d-mutant only): patch `match.attribute` in
	 * memory but SKIP the write-back flush, so the on-disk 0x0B byte keeps its
	 * old (0x20 ARCHIVE) baseline. The SET reports success (FAT12_OK) but the
	 * on-disk DIFFERENTIAL (python --attr / mtools mattrib / a fresh
	 * fat12_get_attr scan) goes RED -- proving the flush is load-bearing for
	 * persistence. The in-memory dispatch-GET path the harness drives reads the
	 * SAME re-scan though, so even the in-harness fat12_get_attr bites. NEVER in a
	 * real build. */
	return FAT12_OK;
#endif
}

int fat12_create(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                 const char *name83, uint8_t attr, uint16_t parent_dir_start,
                 void *sector_buf, void *cluster_buf, dir_entry_t *out_entry,
                 uint32_t *out_slot)
{
	uint8_t     want11[11];
	uint32_t    free_slot = 0u;
	int         have_free = 0;
	uint32_t    match_slot = 0u;
	dir_entry_t match;
	int         found = 0;
	int         rc;
	uint32_t    slot;
	dir_entry_t de;
	uint32_t    k;
	int         is_root = (parent_dir_start == 0u);

	if (vol == NULL || name83 == NULL || sector_buf == NULL ||
	    out_entry == NULL || out_slot == NULL) {
		return FAT12_ERR_NULL;
	}
	if (vol->dev == NULL || vol->dev->write_sectors == NULL ||
	    vol->dev->read_sectors == NULL) {
		return FAT12_ERR_WRITE;
	}

	rc = parse_name83(name83, want11);
	if (rc != FAT12_OK) {
		return rc;
	}

	rc = fat12_scan_dir(vol, fat, fat_len, is_root, parent_dir_start, sector_buf,
	                    want11, &free_slot, &have_free, &match_slot, &match,
	                    &found);
	if (rc != FAT12_OK) {
		return rc;
	}

	if (found) {
		/* TRUNCATE: free the existing chain and reset the entry to size 0. */
		slot = match_slot;
		if (fat != NULL && match.start_cluster >= FAT12_FIRST_DATA_CLUSTER) {
			rc = fat12_free_chain(vol, fat, fat_len, match.start_cluster);
			if (rc != FAT12_OK) {
				return rc;
			}
			rc = fat12_flush_fats(vol, fat, fat_len);
			if (rc != FAT12_OK) {
				return rc;
			}
		}
		de = match;          /* keep the existing name bytes verbatim */
	} else {
		/* CREATE: claim the first reusable slot. */
		if (!have_free) {
			/* The directory is FULL at *free_slot. The fixed ROOT cannot grow ->
			 * dir-full. A SUBDIR grows by one cluster so the free_slot index (the
			 * first slot of the new cluster) becomes valid (beads initech-zs24). */
			if (is_root) {
				return FAT12_ERR_DIR_FULL;
			}
			{
				uint16_t newc = 0u;
				if (cluster_buf == NULL || fat == NULL) {
					return FAT12_ERR_NULL;
				}
				rc = fat12_grow_dir(vol, fat, fat_len, parent_dir_start,
				                    cluster_buf, &newc);
				if (rc != FAT12_OK) {
					return rc;   /* NO_SPACE / write error -- already rolled back */
				}
			}
		}
		slot = free_slot;
		for (k = 0u; k < sizeof(de); k++) {
			((uint8_t *)&de)[k] = 0u;
		}
		for (k = 0u; k < 8u; k++) {
			de.filename[k] = want11[k];
		}
		for (k = 0u; k < 3u; k++) {
			de.extension[k] = want11[8u + k];
		}
	}

	de.attribute     = attr;
	de.mtime         = FAT12_FIXED_MTIME;   /* deterministic (Rule 11) */
	de.mdate         = FAT12_FIXED_MDATE;
	de.start_cluster = 0u;
	de.file_size     = 0u;

	rc = fat12_write_dirent_in_dir(vol, fat, fat_len, is_root, parent_dir_start,
	                               slot, &de, sector_buf);
	if (rc != FAT12_OK) {
		return rc;
	}

	*out_entry = de;
	*out_slot  = slot;
	return FAT12_OK;
}

int fat12_write_file(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                     uint32_t slot, const void *data, uint32_t len,
                     void *sector_buf, void *cluster_buf)
{
	uint32_t bytes_per_cluster;
	uint32_t need_clusters;
	uint16_t first_cluster = 0u;
	uint16_t prev = 0u;
	uint32_t written = 0u;
	uint32_t ci;
	dir_entry_t de;
	const uint8_t *src = (const uint8_t *)data;
	int rc;

	if (vol == NULL || sector_buf == NULL) {
		return FAT12_ERR_NULL;
	}
	if (vol->dev == NULL || vol->dev->write_sectors == NULL ||
	    vol->dev->read_sectors == NULL) {
		return FAT12_ERR_WRITE;
	}

	bytes_per_cluster = (uint32_t)vol->bpb.sectors_per_cluster *
	                    (uint32_t)vol->bpb.bytes_per_sector;

	/* Zero-length: no chain; just patch the dir entry to size 0 / cluster 0. */
	if (len == 0u) {
		rc = fat12_read_dirent_local(vol, slot, &de, sector_buf);
		if (rc != FAT12_OK) {
			return rc;
		}
		de.start_cluster = 0u;
		de.file_size     = 0u;
		return fat12_write_dirent(vol, slot, &de, sector_buf);
	}

	if (fat == NULL || cluster_buf == NULL) {
		return FAT12_ERR_NULL;
	}

	need_clusters = (len + bytes_per_cluster - 1u) / bytes_per_cluster;

	/* Allocate + write each cluster. Allocate lazily (lowest-free-first) so the
	 * chain links as we go; on a no-space failure mid-way, roll back every
	 * cluster claimed so far (Rule 2: a partial write must not corrupt the FAT). */
	for (ci = 0u; ci < need_clusters; ci++) {
		uint16_t cl = fat12_find_free(vol, fat, fat_len,
		                              (uint16_t)(prev ? (prev + 1u)
		                                              : FAT12_FIRST_DATA_CLUSTER));
		uint32_t lba;
		uint32_t take;
		uint32_t off;
		uint8_t *cb = (uint8_t *)cluster_buf;
		uint32_t b;

		if (cl == 0u) {
			/* Full volume: roll back the partial allocation and fail loud. */
			if (first_cluster != 0u) {
				(void)fat12_free_chain(vol, fat, fat_len, first_cluster);
				(void)fat12_flush_fats(vol, fat, fat_len);
			}
			return FAT12_ERR_NO_SPACE;
		}

		/* Reserve this cluster IN the FAT immediately (mark EOC) so the next
		 * fat12_find_free does not hand back the same cluster. */
#ifdef FAT12_MUTATE_WRONG_EOC
		/* MUTANT (Rule 6; make test-fat-write-mutant only): mark the cluster
		 * with a NON-EOC value (0x002) instead of EOC, so the last cluster's
		 * chain link is wrong -- the read-back walk runs off the end / mismatches
		 * and the round-trip goes RED. NEVER define in a real build. */
		rc = fat12_set_entry(fat, fat_len, cl, 0x002u);
#else
		rc = fat12_set_entry(fat, fat_len, cl, FAT12_EOC_VALUE);
#endif
		if (rc != FAT12_OK) {
			return rc;
		}
		if (prev != 0u) {
			rc = fat12_set_entry(fat, fat_len, prev, cl);  /* link prev -> cl */
			if (rc != FAT12_OK) {
				return rc;
			}
		} else {
			first_cluster = cl;
		}

		/* Fill the cluster buffer: the bytes for this cluster, zero-padded. */
		off  = ci * bytes_per_cluster;
		take = (len - off < bytes_per_cluster) ? (len - off) : bytes_per_cluster;
		for (b = 0u; b < take; b++) {
			cb[b] = src[off + b];
		}
		for (b = take; b < bytes_per_cluster; b++) {
			cb[b] = 0u;   /* zero-pad the slack of the last cluster on disk */
		}

		lba = BPB_CLUSTER_LBA(&vol->bpb, cl);
		if (vol->dev->write_sectors(vol->dev->ctx, lba,
		                            vol->bpb.sectors_per_cluster, cb) != 0) {
#ifndef FAT12_MUTATE_WRITEFILE_NO_ROLLBACK
			if (first_cluster != 0u) {
				(void)fat12_free_chain(vol, fat, fat_len, first_cluster);
				(void)fat12_flush_fats(vol, fat, fat_len);
			}
#else
			/* MUTANT (Rule 6; make test-fat-fault-rollback-mutant only): SKIP the
			 * partial-allocation rollback on a mid-write device fault. Every
			 * cluster claimed so far LEAKS (stays allocated) while the call still
			 * fails -- the exact atomicity defect the fault-injection oracle pins.
			 * The free-count assertion in scenario [A] must go RED. NEVER define in
			 * a real build. */
#endif
			return FAT12_ERR_WRITE;
		}

		written += take;
		prev = cl;
	}

	/* Commit the FAT (both copies) now that the whole chain is laid down. */
	rc = fat12_flush_fats(vol, fat, fat_len);
	if (rc != FAT12_OK) {
		return rc;
	}

	/* Patch the dir entry: start_cluster + size. */
	rc = fat12_read_dirent_local(vol, slot, &de, sector_buf);
	if (rc != FAT12_OK) {
		return rc;
	}
	de.start_cluster = first_cluster;
	de.file_size     = written;   /* == len */
	return fat12_write_dirent(vol, slot, &de, sector_buf);
}

/*
 * fat12_write_partial -- POSITIONED write of `len` bytes of `data` starting at
 * byte `offset` within the file at root-dir slot `slot`: OVERWRITE bytes in
 * place where the file already extends, EXTEND (allocate + chain new clusters)
 * where offset+len runs past the current size, and ZERO-FILL the gap when
 * offset > current_size (standard positioned-write semantics: the hole
 * current_size..offset reads back as zeroes). The symmetric counterpart of
 * fat12_read_partial (this file) for the per-handle file-I/O epic (beads
 * initech-snk; epic initech-6qy).
 *
 * Ref (Law 1): Microsoft FAT spec (12-bit packed entries, free=0x000, EOC
 *   0xFF8..0xFFF, allocate the lowest free cluster) -- the SAME write/allocation
 *   source fat12_write_file cites; docs/research/fat12-ground-truth.md Sec 3
 *   (FAT encode = inverse of the read decode) + Sec 4 (dir entry); ADR-0003
 *   DEC-07 (both FATs in sync). The cluster sizing + offset->cluster math mirror
 *   fat12_read_partial: bytes_per_cluster = sectors_per_cluster*bytes_per_sector;
 *   the first touched cluster is offset/bpc steps down the chain, the byte inside
 *   it is offset%bpc. The cluster->LBA map is BPB_CLUSTER_LBA (spec/dos_structs.h).
 *
 * Strategy (Rule 2 fail-loud, allocate-then-commit so a disk-full extend never
 * leaves a half-corrupt chain that diffs wrong):
 *   1. Determine the chain length the file needs to cover new_size =
 *      max(old_size, offset+len): need = ceil(new_size / bpc) clusters.
 *   2. EXTEND the in-memory chain to `need` clusters: walk to the tail, then
 *      allocate/link new clusters (lowest-free-first), marking the last EOC; if
 *      the file was empty (start_cluster 0) the first allocated cluster becomes
 *      start_cluster. Every NEWLY allocated cluster is zero-filled on disk
 *      immediately (so the hole + the last cluster's slack read back as zeroes,
 *      and a partial overwrite into a fresh cluster never exposes garbage). On a
 *      no-free-cluster failure mid-extend, roll back EVERY cluster claimed by
 *      THIS call (free + flush) and return FAT12_ERR_NO_SPACE -- *out_written 0.
 *   3. Flush BOTH FAT copies (the chain is fully laid down).
 *   4. Read-modify-write the `data` bytes over [offset, offset+len): for each
 *      cluster the range touches, if the write covers the WHOLE cluster, write
 *      it straight; otherwise READ the existing cluster into cluster_buf, splice
 *      the new bytes in, write it back -- so neighbouring bytes in a shared
 *      cluster are preserved (the load-bearing read-modify-write).
 *   5. Patch the dir entry: start_cluster (if it changed) + file_size = new_size.
 *
 * *out_written is the bytes actually committed (== len on success; 0 on a
 * disk-full rollback). All buffers caller-provided (no malloc): `sector_buf`
 * (>=512) for the dir-entry RMW, `cluster_buf` (>= sectors_per_cluster*512) for
 * the cluster RMW. `fat`/`fat_len` is the whole-FAT buffer (mutated in place).
 *
 * Fail loud (Rule 2): any required NULL -> FAT12_ERR_NULL; no write backend ->
 * FAT12_ERR_WRITE; full volume mid-extend -> FAT12_ERR_NO_SPACE (rolled back);
 * a corrupt/cyclic existing chain -> FAT12_ERR_CHAIN; a device error ->
 * FAT12_ERR_READ/_WRITE. Anti-hang (Rule 2): the chain walk is bounded by the
 * volume's cluster count.
 */
int fat12_write_partial(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                        uint16_t parent_dir_start, uint32_t slot,
                        uint32_t offset, const uint8_t *data,
                        uint32_t len, void *sector_buf, void *cluster_buf,
                        uint32_t *out_written)
{
	int         is_root = (parent_dir_start == 0u);
	uint32_t    bytes_per_cluster;
	uint32_t    old_size;
	uint32_t    new_size;
	uint32_t    have_clusters;   /* clusters the existing chain covers     */
	uint32_t    need_clusters;   /* clusters new_size needs                */
	uint16_t    orig_start;      /* start_cluster before any extend        */
	uint16_t    new_start;       /* start_cluster after extend (may differ)*/
	uint16_t    tail;            /* current last cluster of the chain      */
	uint16_t    ext_first;       /* first cluster THIS call allocated (0=none) */
	uint16_t    end_cluster;     /* first touched cluster index (offset/bpc)*/
	uint32_t    end;             /* offset + len (exclusive)               */
	uint32_t    steps;
	uint32_t    max_steps;
	uint16_t    cur;
	dir_entry_t de;
	int         rc;

	if (out_written != NULL) {
		*out_written = 0u;
	}
	if (vol == NULL || sector_buf == NULL || out_written == NULL) {
		return FAT12_ERR_NULL;
	}
	if (vol->dev == NULL || vol->dev->write_sectors == NULL ||
	    vol->dev->read_sectors == NULL) {
		return FAT12_ERR_WRITE;
	}

	/* len == 0 is a no-op: nothing written, size unchanged (a zero-length
	 * positioned write neither extends nor zero-fills). */
	if (len == 0u) {
		return FAT12_OK;
	}

	/* Real bytes to move from here on: fat + data + cluster_buf required. */
	if (fat == NULL || data == NULL || cluster_buf == NULL) {
		return FAT12_ERR_NULL;
	}

	/* Guard against offset+len overflowing uint32 (Rule 2: a wrapped end would
	 * compute a bogus, too-small size). */
	end = offset + len;
	if (end < offset) {
		return FAT12_ERR_BUFFER;
	}

	bytes_per_cluster = (uint32_t)vol->bpb.sectors_per_cluster *
	                    (uint32_t)vol->bpb.bytes_per_sector;

	rc = fat12_read_dirent_in_dir(vol, fat, fat_len, is_root, parent_dir_start,
	                              slot, &de, sector_buf);
	if (rc != FAT12_OK) {
		return rc;
	}
	old_size   = de.file_size;
	orig_start = de.start_cluster;
	new_start  = orig_start;
	new_size   = (end > old_size) ? end : old_size;

	have_clusters = (old_size + bytes_per_cluster - 1u) / bytes_per_cluster;
	need_clusters = (new_size + bytes_per_cluster - 1u) / bytes_per_cluster;

	max_steps = vol->total_clusters + 2u;
	ext_first = 0u;

	/* ---- Walk to the current tail of the existing chain (if any). ----
	 * have_clusters counts the clusters the OLD size occupies; walk that many
	 * minus one to land on the last existing cluster. A chain shorter than
	 * old_size demands is corruption (fail loud). */
	tail = 0u;
	if (orig_start >= FAT12_FIRST_DATA_CLUSTER && have_clusters > 0u) {
		uint32_t k;
		cur   = orig_start;
		steps = 0u;
		for (k = 1u; k < have_clusters; k++) {
			uint16_t next;
			if (!fat12_cluster_in_range(vol, cur) || chain_is_free(vol, cur) ||
			    chain_is_bad(vol, cur)) {
				return FAT12_ERR_CHAIN;
			}
			rc = fat12_next_cluster(vol, fat, fat_len, cur, &next);
			if (rc != FAT12_OK) {
				return rc;
			}
			if (chain_is_eoc(vol, next)) {
				return FAT12_ERR_CHAIN;   /* chain shorter than old_size */
			}
			cur = next;
			if (++steps > max_steps) {
				return FAT12_ERR_CHAIN;
			}
		}
		tail = cur;
	}

	/* ---- EXTEND: allocate (need_clusters - have_clusters) new clusters,
	 * chain them onto `tail` (or set start_cluster if the file was empty), mark
	 * the new tail EOC, and ZERO-FILL each freshly allocated cluster on disk.
	 * Allocate-then-commit: on disk-full, roll back THIS call's clusters. */
	if (need_clusters > have_clusters) {
		uint32_t to_add = need_clusters - have_clusters;
		uint32_t i;
		uint16_t prev = tail;
		uint8_t *cb   = (uint8_t *)cluster_buf;
		uint32_t b;

		for (b = 0u; b < bytes_per_cluster; b++) {
			cb[b] = 0u;   /* zero pattern reused for every new cluster */
		}

		for (i = 0u; i < to_add; i++) {
			uint16_t cl;
			uint32_t lba;
#ifdef FAT12_MUTATE_PARTIAL_EXTEND_SHORT
			/* MUTANT (Rule 6; test-fat-write-partial-mutant only): allocate ONE
			 * TOO FEW extend clusters, so a write that grows the file by several
			 * clusters leaves the chain short -- the read-back / diff goes RED.
			 * NEVER define in a real build. */
			if (i + 1u >= to_add) {
				break;
			}
#endif
			cl = fat12_find_free(vol, fat, fat_len,
			                     (uint16_t)(prev ? (prev + 1u)
			                                     : FAT12_FIRST_DATA_CLUSTER));
			if (cl == 0u) {
				/* Full volume: roll back ONLY the clusters THIS call claimed
				 * (free + flush) so the prior chain is untouched. */
				if (ext_first != 0u) {
					(void)fat12_free_chain(vol, fat, fat_len, ext_first);
				}
				/* Re-terminate the original tail as EOC (free_chain set it free
				 * only if it was part of ext_first; the original tail never was,
				 * but its link still points at the now-freed extension -- restore
				 * its EOC marker so the original chain is valid). */
				if (tail != 0u) {
					(void)fat12_set_entry(fat, fat_len, tail, FAT12_EOC_VALUE);
				}
				(void)fat12_flush_fats(vol, fat, fat_len);
				return FAT12_ERR_NO_SPACE;
			}

			/* Reserve immediately (mark EOC) so the next find_free skips it. */
			rc = fat12_set_entry(fat, fat_len, cl, FAT12_EOC_VALUE);
			if (rc != FAT12_OK) {
				return rc;
			}
			if (prev != 0u) {
				rc = fat12_set_entry(fat, fat_len, prev, cl);  /* link prev->cl */
				if (rc != FAT12_OK) {
					return rc;
				}
			} else {
				new_start = cl;   /* file was empty: this is start_cluster */
			}
			if (ext_first == 0u) {
				ext_first = cl;   /* first cluster THIS call allocated */
			}

			/* Zero-fill the fresh cluster on disk (hole + slack read as zero). */
			lba = BPB_CLUSTER_LBA(&vol->bpb, cl);
			if (vol->dev->write_sectors(vol->dev->ctx, lba,
			                            vol->bpb.sectors_per_cluster, cb) != 0) {
				if (ext_first != 0u) {
					(void)fat12_free_chain(vol, fat, fat_len, ext_first);
				}
				if (tail != 0u) {
					(void)fat12_set_entry(fat, fat_len, tail, FAT12_EOC_VALUE);
				}
				(void)fat12_flush_fats(vol, fat, fat_len);
				return FAT12_ERR_WRITE;
			}
			prev = cl;
		}

		/* Commit the FAT (both copies) now the whole extension is laid down. */
		rc = fat12_flush_fats(vol, fat, fat_len);
		if (rc != FAT12_OK) {
			return rc;
		}
	}

	/* ---- Splice `data` into [offset, offset+len) via read-modify-write. ----
	 * Walk to the first touched cluster (offset/bpc steps down the chain), then
	 * for each cluster overwrite [in_off, in_off+take): a whole-cluster write
	 * goes straight; a partial one reads the cluster, splices, writes back so a
	 * shared cluster's neighbouring bytes are preserved. The chain is guaranteed
	 * long enough now (we extended to cover new_size >= offset+len). */
	end_cluster = (uint16_t)(offset / bytes_per_cluster);  /* clusters to skip */
	cur         = new_start;
	steps       = 0u;
	{
		uint16_t k;
		for (k = 0u; k < end_cluster; k++) {
			uint16_t next;
			if (!fat12_cluster_in_range(vol, cur) || chain_is_free(vol, cur) ||
			    chain_is_bad(vol, cur)) {
				return FAT12_ERR_CHAIN;
			}
			rc = fat12_next_cluster(vol, fat, fat_len, cur, &next);
			if (rc != FAT12_OK) {
				return rc;
			}
			if (chain_is_eoc(vol, next)) {
				return FAT12_ERR_CHAIN;   /* extend should have prevented this */
			}
			cur = next;
			if (++steps > max_steps) {
				return FAT12_ERR_CHAIN;
			}
		}
	}

	{
		uint32_t  written = 0u;
		uint8_t  *cb      = (uint8_t *)cluster_buf;

		while (written < len) {
			uint32_t in_off;
			uint32_t remaining;
			uint32_t take;
			uint32_t lba;
			uint32_t i;

			if (!fat12_cluster_in_range(vol, cur) || chain_is_free(vol, cur) ||
			    chain_is_bad(vol, cur)) {
				return FAT12_ERR_CHAIN;
			}

			in_off    = (written == 0u) ? (offset % bytes_per_cluster) : 0u;
			remaining = len - written;
			take      = bytes_per_cluster - in_off;
			if (take > remaining) {
				take = remaining;
			}

			lba = BPB_CLUSTER_LBA(&vol->bpb, cur);

#ifdef FAT12_MUTATE_PARTIAL_NO_RMW
			/* MUTANT (Rule 6; test-fat-write-partial-mutant only): SKIP the
			 * read-modify-write read for a partial cluster -- write a cluster
			 * that is zero outside [in_off,in_off+take), clobbering the
			 * neighbouring bytes that should have been preserved. The differential
			 * oracle goes RED. NEVER define in a real build. */
			{
				uint32_t z;
				for (z = 0u; z < bytes_per_cluster; z++) {
					cb[z] = 0u;
				}
			}
#else
			/* Read-modify-write: when the write does NOT cover the whole cluster,
			 * read the existing contents first so the splice preserves the rest. */
			if (in_off != 0u || take != bytes_per_cluster) {
				if (vol->dev->read_sectors(vol->dev->ctx, lba,
				                           vol->bpb.sectors_per_cluster,
				                           cb) != 0) {
					return FAT12_ERR_READ;
				}
			}
#endif
			for (i = 0u; i < take; i++) {
				cb[in_off + i] = data[written + i];
			}

			if (vol->dev->write_sectors(vol->dev->ctx, lba,
			                            vol->bpb.sectors_per_cluster, cb) != 0) {
				return FAT12_ERR_WRITE;
			}
			written += take;

			/* Advance only if more bytes remain. */
			if (written < len) {
				uint16_t next;
				rc = fat12_next_cluster(vol, fat, fat_len, cur, &next);
				if (rc != FAT12_OK) {
					return rc;
				}
				if (chain_is_eoc(vol, next)) {
					return FAT12_ERR_CHAIN;   /* extend should have prevented */
				}
				cur = next;
				if (++steps > max_steps) {
					return FAT12_ERR_CHAIN;
				}
			}
		}

		*out_written = written;
	}

	/* ---- Patch the dir entry: start_cluster (if changed) + size. ---- */
	rc = fat12_read_dirent_in_dir(vol, fat, fat_len, is_root, parent_dir_start,
	                              slot, &de, sector_buf);
	if (rc != FAT12_OK) {
		return rc;
	}
	de.start_cluster = new_start;
	de.file_size     = new_size;
	return fat12_write_dirent_in_dir(vol, fat, fat_len, is_root, parent_dir_start,
	                                 slot, &de, sector_buf);
}

int fat12_unlink(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                 const char *name83, uint16_t parent_dir_start, void *sector_buf)
{
	uint8_t     want11[11];
	uint32_t    free_slot = 0u;
	int         have_free = 0;
	uint32_t    match_slot = 0u;
	dir_entry_t match;
	int         found = 0;
	int         rc;
	int         is_root = (parent_dir_start == 0u);

	if (vol == NULL || name83 == NULL || sector_buf == NULL) {
		return FAT12_ERR_NULL;
	}
	if (vol->dev == NULL || vol->dev->write_sectors == NULL ||
	    vol->dev->read_sectors == NULL) {
		return FAT12_ERR_WRITE;
	}

	rc = parse_name83(name83, want11);
	if (rc != FAT12_OK) {
		return FAT12_ERR_NOT_FOUND;
	}
	rc = fat12_scan_dir(vol, fat, fat_len, is_root, parent_dir_start, sector_buf,
	                    want11, &free_slot, &have_free, &match_slot, &match,
	                    &found);
	if (rc != FAT12_OK) {
		return rc;
	}
	if (!found) {
		return FAT12_ERR_NOT_FOUND;
	}

	/* Free the cluster chain (if any), flush both FATs. */
	if (fat != NULL && match.start_cluster >= FAT12_FIRST_DATA_CLUSTER) {
		rc = fat12_free_chain(vol, fat, fat_len, match.start_cluster);
		if (rc != FAT12_OK) {
			return rc;
		}
		rc = fat12_flush_fats(vol, fat, fat_len);
		if (rc != FAT12_OK) {
			return rc;
		}
	}

	/* Mark the dir entry deleted (filename[0] = 0xE5). */
	match.filename[0] = DIR_NAME_DELETED;
	return fat12_write_dirent_in_dir(vol, fat, fat_len, is_root, parent_dir_start,
	                                 match_slot, &match, sector_buf);
}

/*
 * fat12_rename -- SAME-directory dir-entry rename (beads initech-gnrc; the WRITE
 * side of INT 21h AH=56h). Scan the directory for OLD (subdir-aware via
 * fat12_scan_dir, exactly as fat12_unlink/fat12_set_attr); REJECT a directory /
 * volume-label source (FAT12_ERR_ACCESS -- no '..' fixup path yet, Rule 2);
 * REJECT a NEW name that already exists in the directory (FAT12_ERR_EXISTS -- the
 * load-bearing dest-exists reject; a rename never clobbers an entry). Then
 * read-modify-write ONLY the matched entry's 11-byte name field (filename[0..7] +
 * extension[0..2]) from the parsed NEW name and flush via the SAME primitive
 * WRITE uses (fat12_write_dirent_in_dir). start_cluster / file_size / attribute /
 * mtime / mdate AND the FAT are PRESERVED VERBATIM (Rule 11: rename allocates /
 * frees nothing, so the chain + size + timestamp bytes stay deterministic). A
 * missing name (or a malformed old/new parse) -> FAT12_ERR_NOT_FOUND; a read-only
 * volume -> FAT12_ERR_WRITE. Ref (Law 1): DOS 3.3 PRM AH=56h; spec/dos_structs.h
 * (filename 0x00 / extension 0x08); the EMPIRICAL mtools name-field layout. */
int fat12_rename(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                 const char *old83, const char *new83, uint16_t dir_start,
                 void *sector_buf)
{
	uint8_t     old11[11];
	uint8_t     new11[11];
	uint32_t    free_slot  = 0u;
	int         have_free  = 0;
	uint32_t    match_slot = 0u;
	dir_entry_t match;
	int         found      = 0;
	int         rc;
	int         is_root = (dir_start == 0u);
	uint32_t    k;

	if (vol == NULL || old83 == NULL || new83 == NULL || sector_buf == NULL) {
		return FAT12_ERR_NULL;
	}
	if (vol->dev == NULL || vol->dev->write_sectors == NULL ||
	    vol->dev->read_sectors == NULL) {
		return FAT12_ERR_WRITE;
	}

#ifdef FAT12_MUTATE_RENAME_IGNORE_DIRSTART
	/* MUTANT 4 (Rule 6; make test-gnrc-mutant only): IGNORE the caller's dir_start
	 * and resolve root-anchored (force the SUBDIR parent to the fixed root). Every
	 * subsequent fat12_scan_dir + the final fat12_write_dirent_in_dir then operate
	 * on the ROOT region instead of the requested subdirectory: a non-root
	 * \SUB\OLD -> \SUB\NEW rename FAILS to find OLD in the (empty-of-OLD) root and
	 * returns FAT12_ERR_NOT_FOUND, so the NON-ROOT same-dir success leg of
	 * test_fat12_rename goes RED. This proves the leg actually exercises the
	 * subdir-aware dir_start path (beads initech-isil) and is not silently passing
	 * via the root scan. NEVER in a real build. */
	dir_start = 0u;
	is_root   = 1;
#endif

	/* Parse BOTH names up front (rename allocates nothing -- a malformed name on
	 * either side is the source-not-found contract). */
	rc = parse_name83(old83, old11);
	if (rc != FAT12_OK) {
		return FAT12_ERR_NOT_FOUND;
	}
	rc = parse_name83(new83, new11);
	if (rc != FAT12_OK) {
		return FAT12_ERR_NOT_FOUND;
	}

	/* Locate the SOURCE in the directory. */
	rc = fat12_scan_dir(vol, fat, fat_len, is_root, dir_start, sector_buf,
	                    old11, &free_slot, &have_free, &match_slot, &match,
	                    &found);
	if (rc != FAT12_OK) {
		return rc;
	}
	if (!found) {
		return FAT12_ERR_NOT_FOUND;
	}
	/* The SOURCE must be a regular file -- never rename a directory or the
	 * volume-label entry (no '..' fixup path yet; Rule 2 fail loud). */
	if ((match.attribute & (DIR_ATTR_DIRECTORY | DIR_ATTR_VOLLABEL)) != 0u) {
		return FAT12_ERR_ACCESS;
	}

	/* The DEST name must be ABSENT in the directory -- a rename never clobbers an
	 * existing entry (the load-bearing dest-exists reject). */
#ifndef FAT12_MUTATE_RENAME_NO_DESTCHECK
	{
		uint32_t    dfree_slot  = 0u;
		int         dhave_free  = 0;
		uint32_t    dmatch_slot = 0u;
		dir_entry_t dmatch;
		int         dfound      = 0;
		rc = fat12_scan_dir(vol, fat, fat_len, is_root, dir_start, sector_buf,
		                    new11, &dfree_slot, &dhave_free, &dmatch_slot,
		                    &dmatch, &dfound);
		if (rc != FAT12_OK) {
			return rc;
		}
		if (dfound) {
			return FAT12_ERR_EXISTS;   /* dest name already present */
		}
	}
#else
	/* MUTANT 1 (Rule 6; make test-gnrc-mutant only): SKIP the dest-absent scan, so
	 * a rename ONTO an existing dest wrongly SUCCEEDS (it overwrites the source's
	 * own name with a name that already lives elsewhere in the dir, producing a
	 * duplicate). The dest-exists (0x0005) leg + the mren differential go RED.
	 * NEVER in a real build. */
#endif

	/* Rewrite ONLY the 11-byte name field; everything else (start_cluster /
	 * file_size / attribute / mtime / mdate) is preserved VERBATIM in `match` and
	 * the FAT is never touched (Rule 11: rename allocates/frees nothing). */
	for (k = 0u; k < 8u; k++) {
		match.filename[k] = new11[k];
	}
#ifndef FAT12_MUTATE_RENAME_NAME_ONLY8
	for (k = 0u; k < 3u; k++) {
		match.extension[k] = new11[8u + k];
	}
#else
	/* MUTANT 3 (Rule 6; make test-gnrc-mutant only): copy ONLY filename[0..7] and
	 * LEAVE the extension[0..2] stale, so X.TXT -> Y.BAK keeps the old .TXT. The
	 * name-field differential vs mren goes RED. NEVER in a real build. */
#endif
#ifdef FAT12_MUTATE_RENAME_TOUCH_CHAIN
	/* MUTANT 2 (Rule 6; make test-gnrc-mutant only): ZERO the start_cluster on the
	 * rewrite, so the renamed entry loses its data chain head. The
	 * 'start_cluster + size unchanged vs mren' assertion goes RED, proving the
	 * rename is name-FIELD-only and never disturbs the chain. NEVER in a real
	 * build. */
	match.start_cluster = 0u;
#endif
	return fat12_write_dirent_in_dir(vol, fat, fat_len, is_root, dir_start,
	                                 match_slot, &match, sector_buf);
}

/* ======================================================================== *
 * FAT12 subdirectory CREATE / REMOVE -- WRITE side (beads initech-u6wa)
 *
 * Ref (Law 1): the EMPIRICAL mtools 4.0.43 '.'/'..' layout (mmd-minted, triple-
 *   confirmed in the differential oracle harness/diff/fat_diff/test_fat12_mkdir.c
 *   -- NOT inferred): '.' name {0x2E,0x20*10} attr 0x10 start=OWN cluster size 0;
 *   '..' name {0x2E,0x2E,0x20*9} attr 0x10 start=PARENT (ROOT encoded as 0, NOT
 *   self, NOT 1) size 0; slot[2]=0x00 end-of-dir; the new dir's FAT entry = 0xFFF
 *   (EOC). This matches the reader's start_cluster==0 => root normalization
 *   (fat12_resolve_path / fat12_dir), so reader + writer are consistent.
 *   docs/research/fat12-ground-truth.md Sec 4 (dir entry / '.'/'..'); ADR-0003
 *   DEC-07; spec/dos_structs.h (DIR_ATTR_DIRECTORY); DOS 3.3 PRM AH=39h/3Ah.
 * ======================================================================== */

/* Build one of the canonical dotdir entries in `de`: `dots` == 1 for '.' (one
 * leading 0x2E) or 2 for '..' (two leading 0x2E); the remainder of the 11-byte
 * 8.3 field is space-padded; attr DIR; start_cluster = `start`; size 0; the
 * date/time fields are the FIXED deterministic constant (Rule 11). */
static void fat12_make_dotdir(dir_entry_t *de, int dots, uint16_t start)
{
	uint32_t k;
	for (k = 0u; k < sizeof(*de); k++) {
		((uint8_t *)de)[k] = 0u;
	}
	de->filename[0] = 0x2Eu;                 /* '.' */
	if (dots == 2) {
		de->filename[1] = 0x2Eu;         /* '..' second dot */
	}
	for (k = (dots == 2) ? 2u : 1u; k < 8u; k++) {
		de->filename[k] = 0x20u;         /* space-pad the name field */
	}
	for (k = 0u; k < 3u; k++) {
		de->extension[k] = 0x20u;        /* space-pad the extension field */
	}
	de->attribute     = DIR_ATTR_DIRECTORY;
	de->mtime         = FAT12_FIXED_MTIME;   /* deterministic (Rule 11) */
	de->mdate         = FAT12_FIXED_MDATE;
	de->start_cluster = start;
	de->file_size     = 0u;
}

int fat12_mkdir(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                const char *name83, uint16_t parent_dir_start, void *sector_buf,
                void *cluster_buf)
{
	uint8_t     want11[11];
	uint32_t    free_slot = 0u;
	int         have_free = 0;
	uint32_t    match_slot = 0u;
	dir_entry_t match;
	int         found = 0;
	int         rc;
	uint16_t    newc;
	uint32_t    bytes_per_cluster;
	uint32_t    lba;
	uint8_t    *cb;
	uint32_t    k;
	dir_entry_t de;
	int         is_root          = (parent_dir_start == 0u);
	int         parent_grew      = 0;
	uint16_t    parent_newc      = 0u;

	if (vol == NULL || name83 == NULL || sector_buf == NULL) {
		return FAT12_ERR_NULL;
	}
	if (vol->dev == NULL || vol->dev->write_sectors == NULL ||
	    vol->dev->read_sectors == NULL) {
		return FAT12_ERR_WRITE;
	}
	/* Parent may be the fixed root (parent_dir_start == 0) OR a subdirectory
	 * cluster (a non-zero first data cluster), beads initech-m0bp -- the nested
	 * MD '\SUB\NEWDIR' path. The parent-aware Layer-1 infra (fat12_scan_dir /
	 * fat12_write_dirent_in_dir / fat12_grow_dir, landed in initech-zs24) lets
	 * the duplicate-name scan, the own-entry write-back, and the parent-full
	 * GROW all walk the parent's cluster chain instead of the fixed root region;
	 * the root path (is_root) stays byte-identical. */

	rc = parse_name83(name83, want11);
	if (rc != FAT12_OK) {
		return rc;
	}

	/* Reject a duplicate name in the parent. DOS MKDIR of an existing name is an
	 * error (the backend maps FAT12_ERR_EXISTS to 0x0005). For a SUBDIR parent
	 * fat12_scan_dir walks the parent's cluster chain; for the root it is the
	 * fixed-region scan (byte-identical to the historical fat12_scan_root). */
#ifdef FAT12_MUTATE_MKDIR_PARENT_ROOTONLY
	/* MUTANT m-noroot-mkdir (Rule 6; make test-m0bp-mutant only): re-impose the
	 * old root-only guard so a NON-ROOT parent is rejected NOT_FOUND -- nested
	 * MKDIR never creates the entry/cluster and the nested differential (parent
	 * entry / '.'/'..' / listing) goes RED. NEVER define in a real build. */
	if (parent_dir_start != 0u) {
		return FAT12_ERR_NOT_FOUND;
	}
#endif
	{
		int scan_is_root = is_root;
#ifdef FAT12_MUTATE_MKDIR_PARENT_SCANROOT
		/* MUTANT m-rootscan-mkdir (Rule 6; make test-m0bp-mutant only): force the
		 * parent scan to treat the parent as the ROOT even for a SUBDIR parent.
		 * The duplicate-name check + the free_slot then come from the ROOT region,
		 * not SUB's chain, so the new dir's entry is written to the wrong dir (or
		 * a false dup is reported) -- the nested differential goes RED. NEVER
		 * define in a real build. */
		scan_is_root = 1;
#endif
		rc = fat12_scan_dir(vol, fat, fat_len, scan_is_root, parent_dir_start,
		                    sector_buf, want11, &free_slot, &have_free,
		                    &match_slot, &match, &found);
	}
	if (rc != FAT12_OK) {
		return rc;
	}
	if (found) {
		return FAT12_ERR_EXISTS;
	}
	if (!have_free) {
		/* The parent is FULL at *free_slot. The fixed ROOT cannot grow ->
		 * dir-full. A SUBDIR parent GROWS by one cluster so the free_slot index
		 * (the first slot of the appended cluster, set by fat12_scan_dir as the
		 * index just past the last cluster) becomes valid (beads initech-m0bp,
		 * mirroring fat12_create's full-subdir path). REUSE free_slot verbatim --
		 * do NOT recompute it (a classic off-by-one). */
#ifndef FAT12_MUTATE_MKDIR_PARENT_NOGROW
		if (is_root) {
			return FAT12_ERR_DIR_FULL;   /* root dir full */
		}
		if (cluster_buf == NULL || fat == NULL) {
			return FAT12_ERR_NULL;
		}
		rc = fat12_grow_dir(vol, fat, fat_len, parent_dir_start,
		                    cluster_buf, &parent_newc);
		if (rc != FAT12_OK) {
			return rc;   /* NO_SPACE / write error -- grow already rolled back */
		}
		parent_grew = 1;
#else
		/* MUTANT m-nogrow-parent (Rule 6; make test-m0bp-mutant only): a FULL
		 * subdir parent returns DIR_FULL instead of growing -- the boundary
		 * MKDIR fails and the new dir never lands, so the parent-grow leg
		 * (listed by mdir/python, parent spans 2 clusters) goes RED. NEVER define
		 * in a real build. (cluster_buf is unused on this dead path; keep the
		 * -Werror=unused-parameter clean.) */
		(void)cluster_buf;
		return FAT12_ERR_DIR_FULL;
#endif
	}

	/* Claim a free data cluster for the new directory and mark it EOC. If the
	 * parent GREW above (parent_grew) and we now fail to claim/commit the new
	 * dir's own cluster, the freshly-appended parent cluster must be rolled back
	 * -- otherwise it orphans, zero-filled and unreclaimable (this FS only
	 * shrinks a dir via fat12_shrink_dir_tail), while MKDIR reports failure: a
	 * broken write atomicity (Rule 2/Rule 3; beads initech-m0bp rollback fix,
	 * adversarial finding). The discipline mirrors fat12_create's no-space
	 * rollback: a claimed-but-unused cluster goes back to FREE (+ flush), and a
	 * grow is undone with its exact inverse fat12_shrink_dir_tail. */
	newc = fat12_find_free(vol, fat, fat_len, FAT12_FIRST_DATA_CLUSTER);
	if (newc == 0u) {
		/* No free cluster for the new dir's OWN cluster. newc==0 means NOTHING
		 * was claimed for it, so roll back ONLY the parent grow (the one
		 * cluster the grow consumed -- here, the volume's last free cluster). */
#ifndef FAT12_MUTATE_MKDIR_NO_NOSPACE_ROLLBACK
		if (parent_grew) {
			fat12_shrink_dir_tail(vol, fat, fat_len, parent_dir_start,
			                      parent_newc);
		}
#else
		/* MUTANT m-nospace-noroll (Rule 6; make test-m0bp-rollback-mutant only):
		 * SKIP the parent-grow rollback on the NO_SPACE post-grow path. The
		 * appended parent cluster then leaks -- SUB stays 2 clusters and the
		 * volume's last free cluster stays consumed while MKDIR fails: exactly
		 * the atomicity defect this fix closes. The rollback oracle must go RED.
		 * NEVER define in a real build. */
		(void)parent_grew;
#endif
		return FAT12_ERR_NO_SPACE;   /* full volume */
	}
#ifndef FAT12_MUTATE_MKDIR_NO_EOC
	rc = fat12_set_entry(fat, fat_len, newc, FAT12_EOC_VALUE);
	if (rc != FAT12_OK) {
		/* newc WAS claimed (in-memory only) -- return it to FREE, then undo the
		 * parent grow, mirroring fat12_create's claimed-but-unused free. (Not
		 * reachable without a write-fail seam; tracked by beads initech-lpf3.) */
		(void)fat12_set_entry(fat, fat_len, newc, FAT12_FREE);
		if (parent_grew) {
			fat12_shrink_dir_tail(vol, fat, fat_len, parent_dir_start,
			                      parent_newc);
		}
		return rc;
	}
#else
	/* MUTANT m3 (Rule 6; make test-fat12-mkdir-mutant only): skip the EOC
	 * set_entry so the new directory's FAT entry stays free (0x000). The
	 * differential FAT-entry / free-count assertion must go RED. NEVER define in
	 * a real build. */
	rc = FAT12_OK;
#endif
	rc = fat12_flush_fats(vol, fat, fat_len);
	if (rc != FAT12_OK) {
		/* The FAT commit failed with newc marked EOC in memory (the parent-grow
		 * link was already flushed inside fat12_grow_dir). Mirror fat12_create's
		 * flush-fail discipline -- but here the parent grow is committed on disk,
		 * so we additionally roll it back: return newc to FREE in memory and run
		 * fat12_shrink_dir_tail, which restores the parent tail to EOC, frees the
		 * appended cluster, and re-flushes both FAT copies (best-effort: if the
		 * device write is hard-down the rollback flush also fails, but the
		 * in-memory FAT is left consistent and no cluster is leaked logically).
		 * Reached + pinned by the fault-injection oracle scenario [C], beads
		 * initech-lpf3 (arm the post-grow EOC flush to fault). */
#ifndef FAT12_MUTATE_MKDIR_NO_FLUSHFAIL_ROLLBACK
		(void)fat12_set_entry(fat, fat_len, newc, FAT12_FREE);
		if (parent_grew) {
			fat12_shrink_dir_tail(vol, fat, fat_len, parent_dir_start,
			                      parent_newc);
		}
#else
		/* MUTANT (Rule 6; make test-fat-fault-rollback-mutant only): SKIP the
		 * post-grow flush-fail rollback. newc stays claimed AND the appended
		 * parent cluster stays linked, both leaking while MKDIR fails -- the
		 * atomicity defect scenario [C] pins. CSUB stays 2 clusters / the free
		 * count drops -> scenario [C] goes RED. NEVER define in a real build. */
#endif
		return rc;
	}

	/* Zero-fill the new cluster's data sectors, then lay '.' at offset 0 and
	 * '..' at offset 32 (slot[2] stays 0x00 = end-of-directory). The '..' start
	 * cluster is the PARENT verbatim: the fixed root is encoded as 0 (the
	 * EMPIRICAL mtools rule; matches the reader's 0 => root normalization), and a
	 * SUBDIR parent is its real non-zero first data cluster (beads initech-m0bp;
	 * fat12_make_dotdir writes parent_dir_start unchanged either way). */
	bytes_per_cluster = (uint32_t)vol->bpb.sectors_per_cluster *
	                    (uint32_t)vol->bpb.bytes_per_sector;
	cb = (uint8_t *)sector_buf;
	for (k = 0u; k < bytes_per_cluster; k++) {
		cb[k] = 0u;
	}
	fat12_make_dotdir(&de, 1, newc);             /* '.'  -> own cluster */
	for (k = 0u; k < 32u; k++) {
		cb[k] = ((const uint8_t *)&de)[k];
	}
#ifndef FAT12_MUTATE_MKDIR_DOTDOT_SELF
	fat12_make_dotdir(&de, 2, parent_dir_start); /* '..' -> parent (root=0) */
#else
	/* MUTANT m2 (Rule 6; make test-fat12-mkdir-mutant only) -- THE canonical
	 * '..'-rule mutant: write '..' start = the dir's OWN cluster instead of the
	 * parent (root encoded as 0). The mmd '..' diff must go RED. NEVER define in
	 * a real build. */
	fat12_make_dotdir(&de, 2, newc);
#endif
	for (k = 0u; k < 32u; k++) {
		cb[32u + k] = ((const uint8_t *)&de)[k];
	}

	lba = BPB_CLUSTER_LBA(&vol->bpb, newc);
	if (vol->dev->write_sectors(vol->dev->ctx, lba,
	                            vol->bpb.sectors_per_cluster, cb) != 0) {
		/* Roll back the allocation so a write failure leaves no orphan cluster
		 * (Rule 2). If the parent GREW for this MKDIR, also detach + free the
		 * freshly-appended parent cluster so it does not leak (Rule 3). */
		(void)fat12_set_entry(fat, fat_len, newc, FAT12_FREE);
		(void)fat12_flush_fats(vol, fat, fat_len);
		if (parent_grew) {
			fat12_shrink_dir_tail(vol, fat, fat_len, parent_dir_start,
			                      parent_newc);
		}
		return FAT12_ERR_WRITE;
	}

	/* Write the new directory's own entry into the PARENT free slot. For a
	 * SUBDIR parent the slot is cluster-chain-addressed down parent_dir_start
	 * (fat12_write_dirent_in_dir); for the root it is the fixed-region slot
	 * (is_root, byte-identical to the historical fat12_write_dirent). */
	for (k = 0u; k < sizeof(de); k++) {
		((uint8_t *)&de)[k] = 0u;
	}
	for (k = 0u; k < 8u; k++) {
		de.filename[k] = want11[k];
	}
	for (k = 0u; k < 3u; k++) {
		de.extension[k] = want11[8u + k];
	}
	de.attribute     = DIR_ATTR_DIRECTORY;
	de.mtime         = FAT12_FIXED_MTIME;        /* deterministic (Rule 11) */
	de.mdate         = FAT12_FIXED_MDATE;
	de.start_cluster = newc;
	de.file_size     = 0u;

#ifndef FAT12_MUTATE_MKDIR_OWNENTRY_ROOTSLOT
	rc = fat12_write_dirent_in_dir(vol, fat, fat_len, is_root, parent_dir_start,
	                               free_slot, &de, sector_buf);
#else
	/* MUTANT m-rootslot-write (Rule 6; make test-m0bp-mutant only): force the new
	 * dir's OWN-entry write-back to a ROOT slot (is_root=1) instead of the parent
	 * subdir cluster-chain slot. NEWDIR then lands in the ROOT, not in SUB:
	 * `mdir ::SUB` lacks it (and `mdir ::` wrongly gains it), so the nested
	 * differential goes RED. NEVER define in a real build. */
	rc = fat12_write_dirent_in_dir(vol, fat, fat_len, 1, 0u,
	                               free_slot, &de, sector_buf);
#endif
	if (rc != FAT12_OK) {
		/* The parent entry failed to land (the LAST failable step). Roll back
		 * the new dir's data cluster AND, if the parent grew for this MKDIR, the
		 * freshly-appended parent cluster -- otherwise both leak (Rule 2/Rule 3;
		 * matches fat12_write_file's partial-allocation rollback discipline). */
		(void)fat12_set_entry(fat, fat_len, newc, FAT12_FREE);
		(void)fat12_flush_fats(vol, fat, fat_len);
		if (parent_grew) {
			fat12_shrink_dir_tail(vol, fat, fat_len, parent_dir_start,
			                      parent_newc);
		}
		return rc;
	}
	return FAT12_OK;
}

/* Empty-check callback: count any surviving entry that is NOT '.' or '..'. A
 * non-zero return stops the enumeration early (the dir is provably non-empty).
 * fat12_read_dir already skips free/deleted/LFN slots, so every entry handed
 * here is a real 8.3 name. (Compiled out under the m4 mutant, which omits the
 * empty-check entirely -- so the callback would be unused; Rule 6.) */
#ifndef FAT12_MUTATE_RMDIR_NO_EMPTYCHECK
static int fat12_rmdir_empty_cb(const dir_entry_t *e, void *user)
{
	int *nonempty = (int *)user;
	if (e->filename[0] == 0x2Eu &&
	    (e->filename[1] == 0x20u ||
	     (e->filename[1] == 0x2Eu && e->filename[2] == 0x20u))) {
		return 0;   /* '.' or '..' -- expected; keep scanning */
	}
	*nonempty = 1;
	return 1;           /* a real child entry -- stop, the dir is non-empty */
}
#endif /* !FAT12_MUTATE_RMDIR_NO_EMPTYCHECK */

int fat12_rmdir(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                const char *name83, uint16_t parent_dir_start, void *sector_buf)
{
	uint8_t     want11[11];
	uint32_t    free_slot = 0u;
	int         have_free = 0;
	uint32_t    match_slot = 0u;
	dir_entry_t match;
	int         found = 0;
	int         rc;
	int         nonempty = 0;
	fat12_dir_t target;
	int         is_root  = (parent_dir_start == 0u);

	if (vol == NULL || name83 == NULL || sector_buf == NULL) {
		return FAT12_ERR_NULL;
	}
	if (vol->dev == NULL || vol->dev->write_sectors == NULL ||
	    vol->dev->read_sectors == NULL) {
		return FAT12_ERR_WRITE;
	}
	/* Parent may be the fixed root (parent_dir_start == 0) OR a subdirectory
	 * cluster (beads initech-m0bp -- nested RD '\SUB\NEWDIR'). The parent scan +
	 * the deleted-mark write-back walk the parent's cluster chain via the
	 * parent-aware Layer-1 infra (fat12_scan_dir / fat12_write_dirent_in_dir);
	 * the empty-check below already enumerates the TARGET's chain (subdir-
	 * capable). No GROW here -- removing an entry never extends the parent (keep
	 * mkdir/rmdir asymmetric). */
#ifdef FAT12_MUTATE_RMDIR_PARENT_ROOTONLY
	/* MUTANT m-noroot-rmdir (Rule 6; make test-m0bp-mutant only): re-impose the
	 * old root-only guard so RD of a NON-ROOT parent is wrongly rejected
	 * NOT_FOUND -- the nested rmdir-success / parent-slot-deleted assertions go
	 * RED. NEVER define in a real build. */
	if (parent_dir_start != 0u) {
		return FAT12_ERR_NOT_FOUND;
	}
#endif

	rc = parse_name83(name83, want11);
	if (rc != FAT12_OK) {
		return FAT12_ERR_NOT_FOUND;
	}
	rc = fat12_scan_dir(vol, fat, fat_len, is_root, parent_dir_start, sector_buf,
	                    want11, &free_slot, &have_free, &match_slot, &match,
	                    &found);
	if (rc != FAT12_OK) {
		return rc;
	}
	if (!found) {
		return FAT12_ERR_NOT_FOUND;
	}
	/* RMDIR of a non-directory is path-not-found (DOS contract). */
	if ((match.attribute & DIR_ATTR_DIRECTORY) == 0u) {
		return FAT12_ERR_NOT_FOUND;
	}

	/* VERIFY-EMPTY: enumerate the target's own cluster; only '.'/'..' may
	 * survive. Any other entry -> non-empty (the backend maps to 0x0005). */
	target.is_root = 0;
	target.start_cluster = match.start_cluster;
#ifndef FAT12_MUTATE_RMDIR_NO_EMPTYCHECK
	rc = fat12_read_dir(vol, &target, sector_buf, fat, fat_len,
	                    fat12_rmdir_empty_cb, &nonempty);
	if (rc != FAT12_OK && rc != 1) {
		/* fat12_read_dir returns the callback's non-zero (1) on an early stop;
		 * any other non-OK is a real read/chain error (fail loud). */
		return rc;
	}
	if (nonempty) {
		return FAT12_ERR_NOT_EMPTY;
	}
#else
	/* MUTANT m4 (Rule 6; make test-fat12-mkdir-mutant only): omit the empty-
	 * check so RMDIR of a NON-EMPTY directory wrongly succeeds. The
	 * RMDIR-non-empty assertion must go RED. NEVER define in a real build. */
	(void)target; (void)nonempty;
#endif

	/* Free the target's cluster chain (the '.'/'..' cluster), flush both FATs. */
	if (fat != NULL && match.start_cluster >= FAT12_FIRST_DATA_CLUSTER) {
		rc = fat12_free_chain(vol, fat, fat_len, match.start_cluster);
		if (rc != FAT12_OK) {
			return rc;
		}
		rc = fat12_flush_fats(vol, fat, fat_len);
		if (rc != FAT12_OK) {
			return rc;
		}
	}

	/* Mark the parent's dir entry deleted (filename[0] = 0xE5). For a SUBDIR
	 * parent the slot is cluster-chain-addressed down parent_dir_start
	 * (fat12_write_dirent_in_dir); for the root it is the fixed-region slot
	 * (is_root, byte-identical to the historical fat12_write_dirent). */
	match.filename[0] = DIR_NAME_DELETED;
	return fat12_write_dirent_in_dir(vol, fat, fat_len, is_root,
	                                 parent_dir_start, match_slot, &match,
	                                 sector_buf);
}

/* ======================================================================== *
 * FAT12 subdirectory / path traversal -- READ side (beads initech-ti8)
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 3 (cluster chain decode,
 *   EOC >= 0xFF8) + Sec 4 (dir entry / sentinels: 0x00 STOP, 0xE5 SKIP, attr
 *   0x0F LFN SKIP); ADR-0003 DEC-07 (root dir is a FIXED region, subdirs are
 *   cluster chains). The subdir walk MIRRORS fat12_read_partial's incremental
 *   chain step (one fat12_next_cluster at a time, gated through
 *   fat12_cluster_in_range, anti-hang bounded by total_clusters+2). A subdir is
 *   NOT bounded by root_entry_count -- it ends only at the 0x00 sentinel or EOC.
 * ======================================================================== */

int fat12_read_dir(const fat12_volume_t *vol, const fat12_dir_t *dir,
                   void *sector_buf, const void *fat, uint32_t fat_len,
                   fat12_dirent_cb cb, void *user)
{
	uint16_t cur;
	uint32_t steps;
	uint32_t max_steps;

	if (vol == NULL || dir == NULL || sector_buf == NULL || cb == NULL ||
	    vol->dev == NULL || vol->dev->read_sectors == NULL) {
		return FAT12_ERR_NULL;
	}

	/* The fixed root directory: delegate VERBATIM so the root path stays
	 * byte-identical to the existing reader (callers/oracles stay green). */
	if (dir->is_root) {
		return fat12_read_root_dir(vol, sector_buf, cb, user);
	}

	/* A subdirectory: the cluster-chain walk needs the FAT buffer to decode
	 * links (fail loud, Rule 2). */
	if (fat == NULL) {
		return FAT12_ERR_NULL;
	}

	cur       = dir->start_cluster;
	steps     = 0u;
	/* Anti-hang bound (Rule 2): a valid chain visits at most total_clusters
	 * distinct data clusters; +2 covers the reserved 0/1 slack. A cyclic/corrupt
	 * chain trips this and errors instead of looping forever. */
	max_steps = vol->total_clusters + 2u;

	for (;;) {
		uint32_t       lba;
		uint32_t       s;
		const uint8_t *secbase;
		uint16_t       next;
		int            rc;

		/* Gate EVERY cluster through the range check BEFORE the LBA math: a
		 * free/bad/out-of-range link is corruption (Rule 2 / bcg.3). This also
		 * guards BPB_CLUSTER_LBA against an underflowing cluster < 2. */
		if (!fat12_cluster_in_range(vol, cur) || chain_is_free(vol, cur) ||
		    chain_is_bad(vol, cur)) {
			return FAT12_ERR_CHAIN;
		}

		/* Read this cluster's sectors at BPB_CLUSTER_LBA. sectors_per_cluster
		 * is 1 on the floppy geometry, but read each sector into sector_buf and
		 * process it so the caller's scratch need only be one sector wide for
		 * spc==1 (the existing callers pass a 512-byte buffer). */
		lba = BPB_CLUSTER_LBA(&vol->bpb, cur);
		for (s = 0u; s < vol->bpb.sectors_per_cluster; s++) {
			uint32_t i;

			if (vol->dev->read_sectors(vol->dev->ctx, lba + s, 1u,
			                           sector_buf) != 0) {
				return FAT12_ERR_READ;
			}
			secbase = (const uint8_t *)sector_buf;

			/* Same per-32-byte-entry inner loop + sentinels as the root reader:
			 * 0x00 => STOP (end of directory), 0xE5 => skip (deleted), attr 0x0F
			 * => skip (LFN); else visit. A non-zero cb return stops early and is
			 * propagated verbatim. A subdir is NOT bounded by root_entry_count. */
			for (i = 0u; i < FAT12_DIRENTS_PER_SECTOR; i++) {
				const dir_entry_t *e = (const dir_entry_t *)(secbase + i * 32u);
				uint8_t            first = e->filename[0];

				if (first == DIR_NAME_FREE) {
					return FAT12_OK; /* end of directory: no entries follow */
				}
				if (first == DIR_NAME_DELETED) {
					continue;
				}
				if (e->attribute == FAT12_ATTR_LFN) {
					continue;
				}
				{
					int r = cb(e, user);
					if (r != 0) {
						return r;
					}
				}
			}
		}

		/* Advance one step down the chain. */
		rc = fat12_next_cluster(vol, fat, fat_len, cur, &next);
		if (rc != FAT12_OK) {
			return rc;
		}
		if (chain_is_eoc(vol, next)) {
			return FAT12_OK; /* chain ended at EOC with no 0x00 sentinel */
		}
		cur = next;
#ifndef FAT12_MUTATE_SUBDIR_SINGLESECTOR
		if (++steps > max_steps) {
			return FAT12_ERR_CHAIN; /* cyclic / corrupt */
		}
#else
		/* MUTANT (Rule 6; test-fat-subdir-mutant only): read ONLY the FIRST
		 * cluster of a subdirectory -- stop the chain walk after one cluster.
		 * BIGDIR enumeration then loses FILE17..FILE40 (entries in cluster 2+),
		 * so the >16-files multi-cluster assertion goes RED. NEVER in a real
		 * build. */
		(void)steps; (void)max_steps;
		return FAT12_OK;
#endif
	}
}

/* Exact 11-byte 8.3 compare of a dir entry against a parsed want11 (the same
 * raw-byte match scan_root uses -- byte-exact with the stored on-disk name). */
static int dirent_name_eq(const dir_entry_t *e, const uint8_t want11[11])
{
	const uint8_t *raw = (const uint8_t *)e;
	uint32_t       k;
	for (k = 0u; k < 11u; k++) {
		if (raw[k] != want11[k]) {
			return 0;
		}
	}
	return 1;
}

/* fat12_resolve_path component-finder context: the parsed 11-byte target name,
 * the slot the match is copied into, and a found flag. */
typedef struct fat12_comp_ctx {
	const uint8_t *want11;     /* parsed 8.3 name to match (raw 11 bytes) */
	dir_entry_t   *out_entry;  /* destination for the matched entry       */
	int            found;      /* 0 until a match is copied               */
} fat12_comp_ctx_t;

/* fat12_read_dir visitor: on an exact 11-byte 8.3 match, copy the entry and
 * request an early stop (return 1). */
static int fat12_comp_cb(const dir_entry_t *e, void *user)
{
	fat12_comp_ctx_t *ctx = (fat12_comp_ctx_t *)user;
	if (dirent_name_eq(e, ctx->want11)) {
		uint32_t       k;
		const uint8_t *src = (const uint8_t *)e;
		uint8_t       *dst = (uint8_t *)ctx->out_entry;
		for (k = 0u; k < sizeof(dir_entry_t); k++) {
			dst[k] = src[k];
		}
		ctx->found = 1;
		return 1; /* stop enumeration */
	}
	return 0;
}

/* Find a single component (parsed want11) in directory `dir`; copy the matched
 * entry to *out_entry. Returns FAT12_OK on a match, FAT12_ERR_NOT_FOUND if no
 * entry matches, or a read/chain error propagated. */
static int fat12_find_in_dir(const fat12_volume_t *vol, const fat12_dir_t *dir,
                             void *sector_buf, const void *fat, uint32_t fat_len,
                             const uint8_t want11[11], dir_entry_t *out_entry)
{
	fat12_comp_ctx_t ctx;
	int              rc;

	ctx.want11    = want11;
	ctx.out_entry = out_entry;
	ctx.found     = 0;

	rc = fat12_read_dir(vol, dir, sector_buf, fat, fat_len, fat12_comp_cb, &ctx);
	if (rc < 0) {
		return rc; /* propagate FAT12_ERR_READ / _CHAIN / _NULL */
	}
	if (!ctx.found) {
		return FAT12_ERR_NOT_FOUND;
	}
	return FAT12_OK;
}

/* Copy one '\\'-delimited component starting at *p into `comp` (NUL-terminated,
 * capacity cap). Advances *p past the component and any single trailing '\\'.
 * Returns the component length (0 for an empty component, e.g. a leading or
 * doubled backslash). */
static uint32_t path_next_component(const char **p, char *comp, uint32_t cap)
{
	const char *s = *p;
	uint32_t    n = 0u;

	while (*s != '\0' && *s != '\\') {
		if (n + 1u < cap) {
			comp[n] = *s;
		}
		n++;
		s++;
	}
	if (*s == '\\') {
		s++; /* consume the separator */
	}
	*p = s;
	comp[(n < cap) ? n : (cap - 1u)] = '\0';
	return n;
}

/* Synthesize a directory marker dir_entry_t for a directory cursor: used when a
 * path resolves to a directory itself (empty path / trailing '\\'). Carries the
 * DIR_ATTR_DIRECTORY attribute + the cursor's start_cluster (0 for the root). */
static void synth_dir_entry(const fat12_dir_t *dir, dir_entry_t *out)
{
	uint32_t k;
	uint8_t *raw = (uint8_t *)out;
	for (k = 0u; k < sizeof(dir_entry_t); k++) {
		raw[k] = 0u;
	}
	out->attribute     = DIR_ATTR_DIRECTORY;
	out->start_cluster = dir->is_root ? 0u : dir->start_cluster;
}

int fat12_resolve_path(const fat12_volume_t *vol, void *sector_buf,
                       const void *fat, uint32_t fat_len, const char *path,
                       fat12_dir_t *out_dir, dir_entry_t *out_entry)
{
	/* The thin start_dir_cluster==0 wrapper: descent from the root, byte-
	 * identical to the historical behavior (beads initech-u6wa). */
	return fat12_resolve_path_from(vol, sector_buf, fat, fat_len, path, 0u,
	                               out_dir, out_entry);
}

int fat12_resolve_path_from(const fat12_volume_t *vol, void *sector_buf,
                            const void *fat, uint32_t fat_len, const char *path,
                            uint16_t start_dir_cluster,
                            fat12_dir_t *out_dir, dir_entry_t *out_entry)
{
	fat12_dir_t cur;
	const char *p;
	char        comp[64];
	uint32_t    comp_len;

	if (vol == NULL || sector_buf == NULL || path == NULL ||
	    out_dir == NULL || out_entry == NULL) {
		return FAT12_ERR_NULL;
	}

	p = path;

	/* Strip a leading drive prefix: any single letter followed by ':'. */
	if (p[0] != '\0' && p[1] == ':' &&
	    ((p[0] >= 'A' && p[0] <= 'Z') || (p[0] >= 'a' && p[0] <= 'z'))) {
		p += 2;
	}

	/* Seed the cursor from `start_dir_cluster`. start_cluster==0 means "the
	 * root" everywhere, normalized BEFORE any cluster-LBA math so cluster 0
	 * never reaches BPB_CLUSTER_LBA (Rule 2). For start_dir_cluster==0 this is
	 * exactly the old root seed (the wrapper's invariant -- root behavior is
	 * byte-identical). */
	cur.is_root       = (start_dir_cluster == 0u) ? 1 : 0;
	cur.start_cluster = start_dir_cluster;

	/* Walk components. The cursor `cur` is the directory we are currently IN;
	 * the LAST non-empty component resolves into *out_entry with *out_dir = the
	 * directory that contains it. */
	for (;;) {
		dir_entry_t ent;
		int         rc;
		uint8_t     want11[11];
		const char *probe;

		comp_len = path_next_component(&p, comp, (uint32_t)sizeof(comp));

		/* An empty component (leading/doubled/trailing '\\', or end-of-path):
		 * if nothing remains, the path resolved to the directory `cur` itself
		 * (empty path or trailing '\\'); otherwise skip the empty component. */
		if (comp_len == 0u) {
			if (*p == '\0') {
				/* Resolve to the directory itself: synthesize a dir marker. */
				*out_dir = cur;
				synth_dir_entry(&cur, out_entry);
				return FAT12_OK;
			}
			continue; /* doubled backslash -- skip the empty component */
		}

		/* '.' is a no-op (stay in `cur`); '..' pops to the parent. We cannot
		 * read a true parent pointer without descending into `cur`, so resolve
		 * '..' by reading the '..' entry of `cur` (which stores the parent's
		 * start_cluster; 0 => the root). The root's '..' stays the root. */
		if (comp[0] == '.' && comp[1] == '\0') {
			/* identity: leave `cur` unchanged */
		} else if (comp[0] == '.' && comp[1] == '.' && comp[2] == '\0') {
			if (cur.is_root) {
				/* '..' from the root is the root (DOS clamps at the root). */
			} else {
				static const uint8_t dotdot11[11] =
					{ '.', '.', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' };
				rc = fat12_find_in_dir(vol, &cur, sector_buf, fat, fat_len,
				                       dotdot11, &ent);
				if (rc != FAT12_OK) {
					return rc;
				}
				cur.start_cluster = ent.start_cluster;
				/* NORMALIZE start_cluster==0 => the root BEFORE any cluster
				 * math (BPB_CLUSTER_LBA underflows on cluster 0). */
				cur.is_root = (ent.start_cluster == 0u) ? 1 : 0;
			}
		} else {
			/* A real component. Parse + match it (raw 11-byte compare). */
			rc = parse_name83(comp, want11);
			if (rc != FAT12_OK) {
				return FAT12_ERR_NOT_FOUND; /* malformed (embedded ':' etc.) */
			}
			rc = fat12_find_in_dir(vol, &cur, sector_buf, fat, fat_len,
			                       want11, &ent);
			if (rc != FAT12_OK) {
				return rc;
			}

			/* Is this the FINAL component? Peek: if nothing meaningful remains,
			 * this is the final one and resolves into *out_entry. */
			probe = p;
			while (*probe == '\\') {
				probe++; /* tolerate a trailing '\\' after the final component */
			}
			if (*probe == '\0') {
				*out_dir   = cur; /* the directory that CONTAINS the entry */
				*out_entry = ent;
				return FAT12_OK;
			}

			/* NON-final: it MUST be a directory to descend into it. */
			if ((ent.attribute & DIR_ATTR_DIRECTORY) == 0) {
#ifndef FAT12_MUTATE_SUBDIR_NOATTR
				return FAT12_ERR_NOT_FOUND;
#else
				/* MUTANT (Rule 6; test-fat-subdir-mutant only): SKIP the
				 * DIR_ATTR_DIRECTORY gate on a non-final component, so a path
				 * like '\SUB\NESTED.TXT\X' wrongly tries to descend into a
				 * regular file. The negative-test assertion goes RED. NEVER in
				 * a real build. */
#endif
			}
			cur.start_cluster = ent.start_cluster;
			/* NORMALIZE start_cluster==0 => the root BEFORE any cluster math. */
			cur.is_root = (ent.start_cluster == 0u) ? 1 : 0;
		}

		/* If the path ends here (only trailing '\\' or nothing left), the cursor
		 * IS the resolved directory. */
		{
			const char *probe2 = p;
			while (*probe2 == '\\') {
				probe2++;
			}
			if (*probe2 == '\0') {
				*out_dir = cur;
				synth_dir_entry(&cur, out_entry);
				return FAT12_OK;
			}
		}
	}
}
