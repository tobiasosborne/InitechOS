/*
 * os/milton/fat12.h -- FAT12 volume mount + read interface (MILTON).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): shipped InitechDOS kernel code. Freestanding
 * C -- only <stdint.h> / <stddef.h>, NO libc, NO malloc. Every buffer is
 * caller-provided. Compiles both freestanding (target) and hosted (so the
 * factory oracle can exercise it on the host).
 *
 * Ref (Law 1): PRD Sec 6.1; ADR-0003 DEC-07 (Sec 5.7, on-volume layout);
 *   docs/research/fat12-ground-truth.md (Sec 1 BPB, Sec 2 geometry,
 *   Sec 3 FAT12 decode); spec/dos_structs.h (locked bpb_t / dir_entry_t).
 *
 * This first slice (beads initech-adf) implements ONLY mount + BPB parse +
 * derived-geometry computation + FAT12/16 classification. FAT-entry decode,
 * directory enumeration, and cluster-chain file read are the NEXT tasks; they
 * are declared here as TODO stubs (not implemented) so the surface is visible.
 */
#ifndef INITECH_MILTON_FAT12_H
#define INITECH_MILTON_FAT12_H

#include <stdint.h>
#include <stddef.h>

#include "blockdev.h"
#include "dos_structs.h"   /* bpb_t (spec/, on -Ispec) */

/* Fail-loud error codes (CLAUDE.md Rule 2). All negative; 0 == success. */
enum {
	FAT12_OK              =  0,
	FAT12_ERR_READ        = -1, /* underlying blockdev read failed            */
	FAT12_ERR_SIGNATURE   = -2, /* boot sector 0x1FE..0x1FF != 0xAA55         */
	FAT12_ERR_SECTOR_SIZE = -3, /* bytes_per_sector != 512                    */
	FAT12_ERR_GEOMETRY    = -4, /* nonsense BPB geometry (zero/overflow)      */
	FAT12_ERR_NULL        = -5, /* NULL argument                              */
	FAT12_ERR_BUFFER      = -6, /* caller buffer too small for the whole FAT  */
	FAT12_ERR_CLUSTER     = -7, /* cluster index < 2 (reserved) or out of FAT */
	FAT12_ERR_CHAIN       = -8, /* corrupt/cyclic chain or out-of-room walk   */
	FAT12_ERR_NOT_FOUND   = -9  /* fat12_find: no dir entry matches the name  */
};

/* FAT type classification by cluster count (Microsoft FAT spec; brief Sec 2).
 * < 4085 => FAT12; 4085..65524 => FAT16. FAT32 is out of scope here. */
typedef enum {
	FAT_TYPE_UNKNOWN = 0,
	FAT_TYPE_FAT12   = 12,
	FAT_TYPE_FAT16   = 16
} fat_type_t;

/* The FAT12/16 cluster-count threshold (Microsoft FAT spec). */
#define FAT12_MAX_CLUSTERS 4085u   /* total_clusters < 4085 => FAT12 */
#define FAT16_MAX_CLUSTERS 65525u  /* total_clusters < 65525 => FAT16 */

/* Offsets of the 0xAA55 boot signature within the boot sector. */
#define FAT12_BOOTSIG_OFFSET 510u
#define FAT12_BOOTSIG_VALUE  0xAA55u

/* A mounted volume: a parsed BPB plus precomputed derived geometry. No heap;
 * the caller embeds/owns this. All sector counts are in the volume's sectors
 * (bytes_per_sector each). */
typedef struct fat12_volume {
	const blockdev_t *dev;        /* backing block device (caller-owned)        */
	bpb_t       bpb;              /* parsed copy of the on-disk BPB             */

	/* Derived geometry (brief Sec 2). */
	uint32_t    first_fat_sector; /* LBA of FAT #1 (= reserved_sectors)         */
	uint32_t    root_dir_sector;  /* LBA of the fixed root directory            */
	uint32_t    root_dir_sectors; /* sectors spanned by the root directory      */
	uint32_t    first_data_sector;/* LBA of cluster 2 (start of the data area)  */
	uint32_t    total_clusters;   /* data-area cluster count (drives fat_type)  */
	fat_type_t  fat_type;         /* FAT12 / FAT16 classification               */
} fat12_volume_t;

/*
 * fat12_mount: read sector 0 from `dev`, validate it, copy the BPB into `vol`,
 * compute derived geometry, and classify FAT12 vs FAT16.
 *
 * Validation (fail loud, CLAUDE.md Rule 2):
 *   - boot-sector signature must be 0xAA55           -> FAT12_ERR_SIGNATURE
 *   - bytes_per_sector must equal 512                -> FAT12_ERR_SECTOR_SIZE
 *   - geometry must be sane (nonzero divisors etc.)  -> FAT12_ERR_GEOMETRY
 *   - any NULL argument                              -> FAT12_ERR_NULL
 *   - a failing device read                          -> FAT12_ERR_READ
 *
 * `sector_buf` is a caller-provided scratch buffer of at least 512 bytes used
 * to read sector 0 (no malloc in the artifact). Returns FAT12_OK on success.
 */
int fat12_mount(fat12_volume_t *vol, const blockdev_t *dev, void *sector_buf);

/* ---- FAT12 entry decode + cluster-chain walk (beads initech-adf) ---- */

/* FAT12 special-value thresholds (brief Sec 3 / docs/research table).
 *
 *   0x000          free
 *   0x001          reserved
 *   0x002..0xFEF   next cluster in chain
 *   0xFF0..0xFF6   reserved
 *   0xFF7          bad cluster
 *   0xFF8..0xFFF   end of chain (EOC); mformat writes 0xFFF
 *
 * NOTE: these thresholds are FAT12-specific. FAT16 uses 0xFFF8 (EOC) /
 * 0xFFF7 (bad) over a 16-bit entry; FAT16 decode/classification is OUT OF
 * SCOPE for this slice and would need its own predicates. */
#define FAT12_FREE      0x000u
#define FAT12_BAD       0xFF7u
#define FAT12_EOC_MIN   0xFF8u   /* >= this is end-of-chain                  */
#define FAT12_ENTRY_MASK 0x0FFFu /* a FAT12 entry is 12 bits                 */

/* The lowest valid data cluster. Clusters 0 and 1 are reserved (FAT[0] holds
 * the media descriptor, FAT[1] is an EOC marker); the data area starts at
 * cluster 2 (brief Sec 2/3). */
#define FAT12_FIRST_DATA_CLUSTER 2u

/* Classification predicates on a raw 12-bit FAT12 entry value (brief Sec 3).
 * Static inlines: zero cost, no extra TU, usable freestanding. */
static inline int fat12_is_free(uint16_t v) { return v == FAT12_FREE; }
static inline int fat12_is_bad(uint16_t v)  { return v == FAT12_BAD; }
static inline int fat12_is_eoc(uint16_t v)  { return v >= FAT12_EOC_MIN; }

/*
 * fat12_read_fat: read the ENTIRE first FAT (sectors_per_fat sectors, =
 * sectors_per_fat * bytes_per_sector bytes) into the caller-provided flat
 * buffer `fat_buf`. Reading the whole FAT contiguously is mandatory: it
 * eliminates the odd-cluster-straddles-a-sector-boundary case (brief RISK-1,
 * clusters 341/682/...). Decode (fat12_next_cluster) reads from this buffer.
 *
 * Fail loud (Rule 2):
 *   - any NULL argument / NULL read fn                 -> FAT12_ERR_NULL
 *   - fat_buf_len < sectors_per_fat * bytes_per_sector -> FAT12_ERR_BUFFER
 *   - a failing device read                            -> FAT12_ERR_READ
 * Returns FAT12_OK on success.
 */
int fat12_read_fat(const fat12_volume_t *vol, void *fat_buf,
                   uint32_t fat_buf_len);

/*
 * fat12_next_cluster: decode the 12-bit FAT12 entry for `cluster` from the
 * flat whole-FAT buffer `fat` (fat_len bytes, as filled by fat12_read_fat)
 * and write the RAW 12-bit value to *out_next. The caller interprets the
 * result via fat12_is_eoc / fat12_is_bad / fat12_is_free.
 *
 * Decode rule (brief Sec 3): byte_offset = (cluster*3)/2;
 *   v = b[off] | (b[off+1] << 8);
 *   entry = (cluster even) ? (v & 0x0FFF) : (v >> 4).
 *
 * Fail loud (Rule 2):
 *   - any NULL argument                          -> FAT12_ERR_NULL
 *   - cluster < 2 (reserved 0/1)                 -> FAT12_ERR_CLUSTER
 *   - byte_offset+1 out of range vs fat_len      -> FAT12_ERR_CLUSTER
 * Returns FAT12_OK on success.
 */
int fat12_next_cluster(const fat12_volume_t *vol, const void *fat,
                       uint32_t fat_len, uint16_t cluster, uint16_t *out_next);

/*
 * fat12_walk_chain: follow the cluster chain from `start_cluster`, appending
 * each visited cluster to `out_clusters` (caller-provided, capacity
 * max_clusters) and stopping at EOC. Sets *out_count to the number visited.
 *
 * This is the load-bearing ANTI-HANG guard (Rule 2): a cyclic or overlong
 * chain MUST terminate with an error, never loop forever. The max_clusters
 * cap doubles as the loop bound -- a chain longer than the caller's array
 * (which is sized to the volume's cluster count) is by definition corrupt.
 *
 * Fail loud (Rule 2):
 *   - any NULL arg / start_cluster < 2           -> FAT12_ERR_NULL / _CLUSTER
 *   - a free cluster mid-chain (corruption)      -> FAT12_ERR_CHAIN
 *   - a bad (0xFF7) cluster mid-chain            -> FAT12_ERR_CHAIN
 *   - more than max_clusters steps (loop/overflow guard) -> FAT12_ERR_CHAIN
 *   - a decode error from fat12_next_cluster     -> propagated
 * Returns FAT12_OK with *out_count set on success.
 */
int fat12_walk_chain(const fat12_volume_t *vol, const void *fat,
                     uint32_t fat_len, uint16_t start_cluster,
                     uint16_t *out_clusters, uint32_t max_clusters,
                     uint32_t *out_count);

/* ---- Root-directory enumeration + file read (beads initech-adf) ---- */

/* Length of a NUL-terminated formatted 8.3 name: 8 + '.' + 3 + NUL = 13. */
#define FAT12_NAME83_MAX 13u

/* The VFAT Long-File-Name attribute: RO|Hidden|System|VolLabel = 0x0F. An
 * entry whose WHOLE attribute byte equals this is an LFN slot (not a real
 * file); it MUST be skipped (brief Sec 4 / RISK-2). Distinct from a plain
 * volume-label entry (DIR_ATTR_VOLLABEL alone). */
#define FAT12_ATTR_LFN \
	(DIR_ATTR_READONLY | DIR_ATTR_HIDDEN | DIR_ATTR_SYSTEM | DIR_ATTR_VOLLABEL)

/* The 0x05 first-byte special case: the real first character is 0xE5
 * (Japanese DOS / Kanji lead byte), NOT a deleted-entry sentinel
 * (brief Sec 4 / RISK-3). */
#define FAT12_NAME_E5_ALIAS 0x05u

/*
 * fat12_format_83: render dir entry `e`'s 8.3 name as a NUL-terminated
 * "NAME.EXT" (or "NAME" when the extension is all spaces) into `out`, trimming
 * the space (0x20) padding. Applies the 0x05 -> 0xE5 first-byte fix
 * (brief Sec 4 / RISK-3). Pure: no I/O, no globals.
 *
 * `out` must hold at least FAT12_NAME83_MAX (13) bytes.
 *
 * Fail loud (Rule 2): NULL arg -> FAT12_ERR_NULL. Returns FAT12_OK on success.
 */
int fat12_format_83(const dir_entry_t *e, char *out);

/*
 * Directory-entry visitor callback for fat12_read_root_dir. `e` points at one
 * surviving (non-free, non-deleted, non-LFN) directory entry; `user` is the
 * caller's opaque cookie. Return 0 to continue enumeration, non-zero to stop
 * early (the non-zero value is propagated as fat12_read_root_dir's return).
 * The pointed-at entry is valid only for the duration of the call (it lives in
 * the caller's sector_buf, which the next sector read overwrites).
 */
typedef int (*fat12_dirent_cb)(const dir_entry_t *e, void *user);

/*
 * fat12_read_root_dir: enumerate the FIXED root directory (brief Sec 4). The
 * root directory is NOT a cluster chain: it is root_dir_sectors sectors
 * starting at root_dir_sector, holding root_entry_count 32-byte entries. We
 * read it sector-by-sector into the caller's `sector_buf` (>= 512 bytes, no
 * malloc) and invoke `cb` for each surviving entry.
 *
 * Sentinels / filtering (brief Sec 4):
 *   - filename[0] == 0x00 (DIR_NAME_FREE)    -> end of directory: STOP.
 *   - filename[0] == 0xE5 (DIR_NAME_DELETED) -> deleted: SKIP, continue.
 *   - attribute   == 0x0F (FAT12_ATTR_LFN)   -> VFAT long-name slot: SKIP.
 * Volume-label and directory entries are NOT filtered here (the callback may
 * inspect e->attribute and decide); only the structural skips above are
 * applied so the callback sees every real 8.3 entry.
 *
 * Fail loud (Rule 2):
 *   - any NULL arg / NULL read fn  -> FAT12_ERR_NULL
 *   - a failing device read        -> FAT12_ERR_READ
 * Returns FAT12_OK after the full scan, or the callback's non-zero value if it
 * requested an early stop.
 */
int fat12_read_root_dir(const fat12_volume_t *vol, void *sector_buf,
                        fat12_dirent_cb cb, void *user);

/*
 * fat12_find: locate a regular file/directory in the root directory by its
 * formatted 8.3 name and copy its dir_entry_t to *out_entry. `name83` is
 * compared case-insensitively against fat12_format_83 of each entry (so the
 * caller may pass "hello.txt" or "HELLO.TXT"). LFN/deleted entries are skipped
 * by fat12_read_root_dir; the 0x00 sentinel stops the scan.
 *
 * `sector_buf` is a caller scratch buffer of >= 512 bytes.
 *
 * Fail loud (Rule 2):
 *   - any NULL arg          -> FAT12_ERR_NULL
 *   - a failing device read -> FAT12_ERR_READ (propagated)
 *   - no matching entry     -> FAT12_ERR_NOT_FOUND
 * Returns FAT12_OK with *out_entry filled on a match.
 */
int fat12_find(const fat12_volume_t *vol, void *sector_buf,
               const char *name83, dir_entry_t *out_entry);

/*
 * fat12_read_file: read the file described by dir entry `e` into `out_buf`,
 * exposing EXACTLY e->file_size bytes (brief Sec 4 / RISK-5: the last cluster
 * is partially filled; file_size is authoritative -- no trailing padding).
 *
 * `fat` / `fat_len` is the whole-FAT buffer from fat12_read_fat. We walk
 * e->start_cluster's chain (fat12_walk_chain) and copy each cluster's data,
 * sector-by-sector, via `cluster_buf` -- a caller scratch buffer of at least
 * sectors_per_cluster * 512 bytes. *out_bytes is set to e->file_size.
 *
 * A zero-length file (file_size == 0; start_cluster may legitimately be 0) is
 * handled WITHOUT a chain walk: *out_bytes = 0, nothing copied.
 *
 * Fail loud (Rule 2):
 *   - any NULL arg                      -> FAT12_ERR_NULL
 *   - out_buf_len < e->file_size        -> FAT12_ERR_BUFFER (never overflow)
 *   - a failing device read             -> FAT12_ERR_READ
 *   - a corrupt/over-short chain        -> FAT12_ERR_CHAIN
 *     (chain shorter than file_size needs, or walk errors propagated)
 * Returns FAT12_OK with *out_bytes == e->file_size on success.
 */
int fat12_read_file(const fat12_volume_t *vol, const void *fat, uint32_t fat_len,
                    const dir_entry_t *e, void *out_buf, uint32_t out_buf_len,
                    void *cluster_buf, uint32_t *out_bytes);

/* ---- NEXT tasks (beads initech-adf continuation); NOT implemented here ---- *
 *   With root-dir enumeration + find + file-read landed, the FAT12 READ path
 *   is essentially complete. Remaining, as SEPARATE issues:
 *     - subdirectory traversal (walk a DIR_ATTR_DIRECTORY entry's chain as a
 *       directory, recursively) -- this slice is root-dir only.
 *     - the FAT WRITE path (allocate clusters, update both FATs, write dir
 *       entries) -- beads initech-509.11; blockdev write_sectors is still NULL.
 *     - the differential-vs-mtools `test-fat` gate wiring (beads initech-5cu).
 */

#endif /* INITECH_MILTON_FAT12_H */
