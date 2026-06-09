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
	if (vol->bpb.sectors_per_cluster == 0u ||
	    vol->bpb.num_fats == 0u ||
	    vol->bpb.sectors_per_fat == 0u ||
	    vol->bpb.root_entry_count == 0u) {
		return FAT12_ERR_GEOMETRY;
	}

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

	/* total_clusters = data_sectors / sectors_per_cluster (brief Sec 2). */
	data_sectors        = total_sectors - vol->first_data_sector;
	vol->total_clusters = data_sectors / vol->bpb.sectors_per_cluster;

	/* Classify FAT12 vs FAT16 SOLELY by cluster count (brief Sec 2 / RISK-8;
	 * the fs_type string is informational and unreliable). */
	if (vol->total_clusters < FAT12_MAX_CLUSTERS) {
		vol->fat_type = FAT_TYPE_FAT12;
	} else if (vol->total_clusters < FAT16_MAX_CLUSTERS) {
		vol->fat_type = FAT_TYPE_FAT16;
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

/*
 * fat12_next_cluster -- decode the 12-bit entry for `cluster` (brief Sec 3).
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 3 --
 *   byte_offset = (N*3)/2;  v = b[off] | (b[off+1]<<8);
 *   entry = (N even) ? (v & 0x0FFF) : (v >> 4).
 * Worked example (clusters 4..7) verified there.
 */
int fat12_next_cluster(const fat12_volume_t *vol, const void *fat,
                       uint32_t fat_len, uint16_t cluster, uint16_t *out_next)
{
	const uint8_t *b;
	uint32_t       off;
	uint16_t       v;

	(void)vol; /* decode is purely a function of the flat FAT buffer */

	if (fat == NULL || out_next == NULL) {
		return FAT12_ERR_NULL;
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
		if (fat12_is_free(cur)) {
			return FAT12_ERR_CHAIN;
		}
		if (fat12_is_bad(cur)) {
			return FAT12_ERR_CHAIN;
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
		if (fat12_is_eoc(next)) {
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
 * fat12_read_file -- read exactly file_size bytes via the cluster chain.
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 4 (file read) + RISK-5
 *   ("file_size is authoritative for the last cluster ... do not return
 *   padding zeros"). Reuses fat12_walk_chain (this file) for the chain +
 *   anti-hang guard, and BPB_CLUSTER_LBA (spec/dos_structs.h) for the
 *   cluster->LBA mapping.
 */
int fat12_read_file(const fat12_volume_t *vol, const void *fat, uint32_t fat_len,
                    const dir_entry_t *e, void *out_buf, uint32_t out_buf_len,
                    void *cluster_buf, uint32_t *out_bytes)
{
	uint32_t  file_size;
	uint32_t  bytes_per_cluster;
	uint32_t  copied;
	uint32_t  ci;
	uint8_t  *out;
	int       rc;

	/*
	 * Chain scratch: a valid chain visits at most total_clusters clusters.
	 * We cap the on-stack array at the standard 1.44 MB cluster count (2847,
	 * round up to 2880) -- ample for this slice's geometry and small enough
	 * for a freestanding stack. fat12_walk_chain's max_clusters bound doubles
	 * as the anti-hang guard, so a corrupt/cyclic chain errors here too.
	 */
	uint16_t  chain[2880];
	uint32_t  chain_len;

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

	/* Walk start_cluster..EOC. The anti-hang / corruption guards live in
	 * fat12_walk_chain; a too-long or cyclic chain returns FAT12_ERR_CHAIN. */
	rc = fat12_walk_chain(vol, fat, fat_len, e->start_cluster, chain,
	                      (uint32_t)(sizeof(chain) / sizeof(chain[0])),
	                      &chain_len);
	if (rc != FAT12_OK) {
		return rc;
	}

	/* The chain must hold enough clusters to cover file_size. A chain that is
	 * too short for the declared size is corruption (fail loud, Rule 2). */
	{
		uint32_t need = (file_size + bytes_per_cluster - 1u) / bytes_per_cluster;
		if (chain_len < need) {
			return FAT12_ERR_CHAIN;
		}
	}

	out    = (uint8_t *)out_buf;
	copied = 0u;

	/* Copy cluster-by-cluster; the LAST cluster is truncated to whatever is
	 * left of file_size (RISK-5: no trailing padding). */
	for (ci = 0u; ci < chain_len && copied < file_size; ci++) {
		uint32_t lba       = BPB_CLUSTER_LBA(&vol->bpb, chain[ci]);
		uint32_t remaining = file_size - copied;
		uint32_t take      = remaining < bytes_per_cluster
		                       ? remaining : bytes_per_cluster;
		const uint8_t *cbuf = (const uint8_t *)cluster_buf;
		uint32_t k;

		/* Read the whole cluster (sectors_per_cluster sectors) into scratch,
		 * then copy only `take` bytes out -- the partial last cluster never
		 * contributes its padding to out_buf. */
		if (vol->dev->read_sectors(vol->dev->ctx, lba,
		                           vol->bpb.sectors_per_cluster,
		                           cluster_buf) != 0) {
			return FAT12_ERR_READ;
		}
		for (k = 0u; k < take; k++) {
			out[copied + k] = cbuf[k];
		}
		copied += take;
	}

	*out_bytes = file_size;
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
		if (fat12_is_eoc(next) || fat12_is_free(next) || fat12_is_bad(next)) {
			break;
		}
		cur = next;
		if (++steps > max) {
			return FAT12_ERR_CHAIN;   /* cyclic/corrupt: do not loop forever */
		}
	}
	return FAT12_OK;
}

/* Read a single dir entry (32 bytes) from its root-dir slot into *e. */
static int fat12_read_dirent_local(const fat12_volume_t *vol, uint32_t slot,
                                   dir_entry_t *e, void *sector_buf)
{
	uint32_t per_sector = BLOCKDEV_SECTOR_SIZE / 32u;
	uint32_t sec_index  = slot / per_sector;
	uint32_t in_sec     = slot % per_sector;
	uint32_t lba        = vol->root_dir_sector + sec_index;
	const uint8_t *sb   = (const uint8_t *)sector_buf;
	uint8_t *dst        = (uint8_t *)e;
	uint32_t k;

	if (sec_index >= vol->root_dir_sectors) {
		return FAT12_ERR_DIR_FULL;
	}
	if (vol->dev->read_sectors(vol->dev->ctx, lba, 1u, sector_buf) != 0) {
		return FAT12_ERR_READ;
	}
	for (k = 0u; k < 32u; k++) {
		dst[k] = sb[in_sec * 32u + k];
	}
	return FAT12_OK;
}

/* Write a single dir entry (32 bytes) to its root-dir slot on disk: read the
 * containing sector, overwrite the 32-byte entry, write the sector back. */
static int fat12_write_dirent(const fat12_volume_t *vol, uint32_t slot,
                              const dir_entry_t *e, void *sector_buf)
{
	uint32_t per_sector = BLOCKDEV_SECTOR_SIZE / 32u;
	uint32_t sec_index  = slot / per_sector;
	uint32_t in_sec     = slot % per_sector;
	uint32_t lba        = vol->root_dir_sector + sec_index;
	uint8_t *sb         = (uint8_t *)sector_buf;
	const uint8_t *src  = (const uint8_t *)e;
	uint32_t k;

	if (vol->dev->write_sectors == NULL) {
		return FAT12_ERR_WRITE;
	}
	if (sec_index >= vol->root_dir_sectors) {
		return FAT12_ERR_DIR_FULL;
	}
	if (vol->dev->read_sectors(vol->dev->ctx, lba, 1u, sb) != 0) {
		return FAT12_ERR_READ;
	}
	for (k = 0u; k < 32u; k++) {
		sb[in_sec * 32u + k] = src[k];
	}
	if (vol->dev->write_sectors(vol->dev->ctx, lba, 1u, sb) != 0) {
		return FAT12_ERR_WRITE;
	}
	return FAT12_OK;
}

/* Locate a root-dir slot: if a non-deleted entry's 8.3 fields == want11, set
 * *match_slot and copy it to *match (found=1); else record the FIRST reusable
 * slot (deleted 0xE5 or free 0x00) in *free_slot. Stops at the 0x00 sentinel
 * for the match search but a free slot found earlier is still usable. Returns
 * FAT12_OK (found flagged via *found) or a read error. */
static int fat12_scan_root(const fat12_volume_t *vol, void *sector_buf,
                           const uint8_t want11[11], uint32_t *free_slot,
                           int *have_free, uint32_t *match_slot,
                           dir_entry_t *match, int *found)
{
	uint32_t per_sector = BLOCKDEV_SECTOR_SIZE / 32u;
	uint32_t slot = 0u;
	uint32_t s;

	*have_free = 0;
	*found     = 0;

	for (s = 0u; s < vol->root_dir_sectors; s++) {
		const uint8_t *sb;
		uint32_t i;
		if (vol->dev->read_sectors(vol->dev->ctx, vol->root_dir_sector + s,
		                           1u, sector_buf) != 0) {
			return FAT12_ERR_READ;
		}
		sb = (const uint8_t *)sector_buf;
		for (i = 0u; i < per_sector; i++, slot++) {
			const uint8_t *e = sb + i * 32u;
			uint8_t first = e[0];

			if (first == DIR_NAME_FREE) {
				/* End-of-directory: this slot (and all after) are free. The
				 * first free slot for placement is here if none seen earlier. */
				if (!*have_free) {
					*free_slot = slot;
					*have_free = 1;
				}
				return FAT12_OK;   /* no match exists beyond the sentinel */
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
					return FAT12_OK;
				}
			}
		}
	}
	/* Scanned the whole fixed root dir with no 0x00 sentinel and no match: the
	 * directory is full unless a deleted slot was recorded. */
	return FAT12_OK;
}

int fat12_create(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                 const char *name83, uint8_t attr, void *sector_buf,
                 dir_entry_t *out_entry, uint32_t *out_slot)
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

	rc = fat12_scan_root(vol, sector_buf, want11, &free_slot, &have_free,
	                     &match_slot, &match, &found);
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
			return FAT12_ERR_DIR_FULL;
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

	rc = fat12_write_dirent(vol, slot, &de, sector_buf);
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
			if (first_cluster != 0u) {
				(void)fat12_free_chain(vol, fat, fat_len, first_cluster);
				(void)fat12_flush_fats(vol, fat, fat_len);
			}
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

int fat12_unlink(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                 const char *name83, void *sector_buf)
{
	uint8_t     want11[11];
	uint32_t    free_slot = 0u;
	int         have_free = 0;
	uint32_t    match_slot = 0u;
	dir_entry_t match;
	int         found = 0;
	int         rc;

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
	rc = fat12_scan_root(vol, sector_buf, want11, &free_slot, &have_free,
	                     &match_slot, &match, &found);
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
	return fat12_write_dirent(vol, match_slot, &match, sector_buf);
}
