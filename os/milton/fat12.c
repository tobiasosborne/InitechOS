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
