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
	FAT12_ERR_NOT_FOUND   = -9, /* fat12_find: no dir entry matches the name  */
	FAT12_ERR_WRITE       = -10,/* underlying blockdev write failed / NULL    */
	FAT12_ERR_NO_SPACE    = -11,/* no free cluster (full volume)              */
	FAT12_ERR_DIR_FULL    = -12,/* no free root-directory slot                */
	FAT12_ERR_EXISTS      = -13,/* (reserved) name already present            */
	FAT12_ERR_UNSUPPORTED = -14 /* valid but unsupported FS (e.g. FAT16); the  */
	                            /* 12-bit decode/encode is FAT12-only (bcg.4)  */
};

/* Deterministic dir-entry timestamp (CLAUDE.md Rule 11): the artifact has NO
 * clock and reproducible builds forbid a host timestamp. The FAT date/time
 * fields are implementation-specific on write (docs/research/fat12-ground-truth.md
 * Sec 4: "normalize to zero before diffing"), so the WRITE path stamps a FIXED
 * constant -- zero -- which is also the normalization target. The round-trip
 * oracle normalizes mtime/mdate/serial away before diffing (they are NOT
 * meaningful bytes; the meaningful bytes are name/attr/size/start_cluster +
 * content). */
#define FAT12_FIXED_MTIME 0x0000u
#define FAT12_FIXED_MDATE 0x0000u

/* Canonical FAT12 end-of-chain value mformat writes (brief Sec 3): we emit the
 * same so a written chain is byte-identical to mtools' for the EOC link. */
#define FAT12_EOC_VALUE   0x0FFFu

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

/* A directory cursor (beads initech-ti8): the FIXED root directory OR a
 * subdirectory rooted at a starting data cluster. The root directory is a
 * fixed region (NOT a cluster chain) so it carries no cluster; a subdirectory
 * is a cluster chain like a regular file, ending at the 0x00 sentinel or EOC
 * (NOT bounded by root_entry_count). start_cluster==0 ALWAYS means "the root"
 * (a subdir whose ".." points at the root stores start_cluster 0), so
 * fat12_resolve_path normalizes start_cluster==0 => is_root=1 BEFORE any
 * cluster-LBA math (BPB_CLUSTER_LBA underflows on cluster 0).
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 4 (root dir region) +
 *   Sec 3 (chain); spec/dos_structs.h (dir_entry_t.start_cluster). */
typedef struct fat12_dir {
	int      is_root;        /* 1 => the fixed root directory; 0 => a subdir */
	uint16_t start_cluster;  /* subdir's first data cluster (unused if root) */
} fat12_dir_t;

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
 * fat12_find_slot: like fat12_find, but ALSO returns the matching entry's
 * 0-based root-directory SLOT index in *out_slot (beads initech-0qh; epic
 * initech-6qy). The multi-tenant positioned file backend (fileio_fat.c) needs
 * the slot so a later fat12_write_partial / fat12_read_dir_entry can patch or
 * refresh the entry in place. Slot semantics match fat12_create's *out_slot.
 * Returns FAT12_OK with *out_entry + *out_slot set; FAT12_ERR_NOT_FOUND if no
 * match; FAT12_ERR_NULL / a parse or read error otherwise.
 */
int fat12_find_slot(const fat12_volume_t *vol, void *sector_buf,
                    const char *name83, dir_entry_t *out_entry,
                    uint32_t *out_slot);

/*
 * fat12_find_slot_in: like fat12_find_slot, but locate `name83` in the directory
 * whose first data cluster is `parent_dir_start` (0 == the fixed root, byte-
 * identical to fat12_find_slot) and return the entry's slot WITHIN that directory
 * (beads initech-zs24) -- the root-dir-region index for the root, the linear
 * cluster-chain index for a subdir (so a later positioned write_at /
 * fat12_read_dir_entry_in patches the entry in the right place). `fat`/`fat_len`
 * decode the subdir links. Returns FAT12_OK with *out_entry + *out_slot set;
 * FAT12_ERR_NOT_FOUND if no match; a parse / read / chain error propagated.
 */
int fat12_find_slot_in(const fat12_volume_t *vol, const void *fat,
                       uint32_t fat_len, uint16_t parent_dir_start,
                       void *sector_buf, const char *name83,
                       dir_entry_t *out_entry, uint32_t *out_slot);

/*
 * fat12_read_dir_entry: re-read the 32-byte directory entry at root-dir slot
 * `slot` into *out_entry (beads initech-0qh). Used after a positioned write to
 * refresh the cached dir entry (size + start_cluster). `sector_buf` (>=512) is
 * scratch. Returns FAT12_OK, or a read error / FAT12_ERR_DIR_FULL (slot out of
 * range) / FAT12_ERR_NULL.
 */
int fat12_read_dir_entry(const fat12_volume_t *vol, void *sector_buf,
                         uint32_t slot, dir_entry_t *out_entry);

/*
 * fat12_read_dir_entry_in: re-read the 32-byte directory entry at slot `slot` of
 * the directory whose first data cluster is `parent_dir_start` (0 == the fixed
 * root) into *out_entry (beads initech-zs24). The subdir-aware counterpart of
 * fat12_read_dir_entry, used by the file backend's positioned write_at() to
 * refresh a SUBDIR file's cached dir entry from the right slot. `fat`/`fat_len`
 * decode the subdir cluster chain (unused for the root). `sector_buf` (>=512) is
 * scratch. Returns FAT12_OK, or a read/chain error / FAT12_ERR_DIR_FULL (slot
 * out of range) / FAT12_ERR_NULL.
 */
int fat12_read_dir_entry_in(const fat12_volume_t *vol, const void *fat,
                            uint32_t fat_len, uint16_t parent_dir_start,
                            void *sector_buf, uint32_t slot,
                            dir_entry_t *out_entry);

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

/*
 * fat12_read_partial: POSITIONED read of up to `len` bytes starting at byte
 * `offset` within the file described by dir entry `e`, walking ONLY the cluster
 * chain it needs -- never loading the whole file (beads initech-lq2). This is
 * the foundational random-access primitive for the per-handle positioned
 * cluster-chain I/O epic (beads initech-6qy): >64 KiB files, simultaneous open
 * handles, and seek all rest on reading a slice without materializing the rest.
 *
 * Semantics:
 *   - Copies min(len, e->file_size - offset) bytes from byte position `offset`
 *     into `out_buf` and sets *out_read to that count.
 *   - offset >= file_size  -> *out_read = 0, return FAT12_OK (clean EOF, NOT an
 *     error: a positioned read at/after end-of-file yields zero bytes).
 *   - len == 0             -> *out_read = 0, return FAT12_OK.
 *   - Otherwise: skip (offset / bytes_per_cluster) clusters down the chain,
 *     then copy from the byte offset WITHIN the first needed cluster, spanning
 *     as many further clusters as `len` requires. file_size is authoritative:
 *     the partial last cluster never contributes padding (RISK-5).
 *
 * Unlike fat12_read_file, the chain is walked INCREMENTALLY (one
 * fat12_next_cluster step at a time) -- no on-stack chain[] array -- so an
 * arbitrarily large file is served from the caller's single-cluster `cluster_buf`
 * (>= sectors_per_cluster * 512) without a whole-file or whole-chain buffer.
 *
 * `out_buf` must hold at least the bytes actually returned (<= len); the caller
 * sizes it to `len`. `fat` / `fat_len` is the whole-FAT buffer from
 * fat12_read_fat.
 *
 * Fail loud (Rule 2):
 *   - any required NULL arg                          -> FAT12_ERR_NULL
 *     (out_buf/cluster_buf/fat are required only when bytes are actually read;
 *      a clean-EOF or zero-len call needs only vol/e/out_read.)
 *   - a free/bad cluster, or the chain ending BEFORE the requested range is
 *     covered, is corruption                         -> FAT12_ERR_CHAIN
 *   - a failing device read                          -> FAT12_ERR_READ
 *   - an out-of-range cluster decode (propagated)    -> FAT12_ERR_CLUSTER
 * Anti-hang (Rule 2): the incremental walk is bounded by the volume's cluster
 * count; a cyclic/corrupt chain errors rather than looping forever.
 * Returns FAT12_OK with *out_read set on success.
 */
int fat12_read_partial(const fat12_volume_t *vol, const void *fat,
                       uint32_t fat_len, const dir_entry_t *e,
                       uint32_t offset, uint32_t len, void *out_buf,
                       void *cluster_buf, uint32_t *out_read);

/* ======================================================================== *
 * FAT12 WRITE path (beads initech-509.11)
 *
 * The WRITE half: create/truncate a root-directory 8.3 entry, allocate a
 * cluster chain from the FAT free list, write the data clusters, and keep BOTH
 * FAT copies in sync. Proven by the differential round-trip oracle against
 * mtools (the gold reference) + an independent python3 reader (test-fat-write)
 * and the in-emulator round-trip (test-fatwrite).
 *
 * Ref (Law 1): Microsoft FAT spec (12-bit packed entries, free=0x000,
 *   EOC 0xFF8..0xFFF, allocate the lowest free cluster); docs/research/
 *   fat12-ground-truth.md Sec 3 (FAT encode -- the EXACT inverse of the read
 *   decode) + Sec 4 (dir entry) + Sec 4 normalization note (date/time stamped
 *   to a fixed constant, Rule 11); ADR-0003 DEC-07 (both FATs kept in sync).
 *
 * All buffers caller-provided (no malloc). Every write goes through
 * vol->dev->write_sectors, which MUST be non-NULL for these to succeed (a
 * read-only device fails loud with FAT12_ERR_WRITE). The whole-FAT buffer
 * `fat`/`fat_len` is the SAME buffer fat12_read_fat fills; the write functions
 * MUTATE it in place and flush BOTH on-disk FAT copies from it.
 * ======================================================================== */

/*
 * fat12_set_entry: write the 12-bit FAT12 entry for `cluster` into the flat
 * whole-FAT buffer `fat` (the EXACT inverse of fat12_next_cluster's decode,
 * brief Sec 3). Only the 12 bits for this cluster are modified; the straddled
 * neighbour nibble is preserved (read-modify-write of the two bytes).
 *
 *   byte_offset = (cluster*3)/2;
 *   even cluster: b[off] = value low 8 bits;
 *                 b[off+1] = (b[off+1] & 0xF0) | (value>>8 & 0x0F);
 *   odd cluster:  b[off] = (b[off] & 0x0F) | ((value<<4) & 0xF0);
 *                 b[off+1] = (value>>4) & 0xFF;
 *
 * Fail loud (Rule 2): NULL/cluster<2/out-of-range -> FAT12_ERR_NULL/_CLUSTER.
 * `value` is masked to 12 bits. Returns FAT12_OK on success. This mutates the
 * in-memory FAT only; fat12_flush_fats commits it to disk. */
int fat12_set_entry(void *fat, uint32_t fat_len, uint16_t cluster,
                    uint16_t value);

/*
 * fat12_flush_fats: write the whole in-memory FAT buffer `fat` (fat_len bytes,
 * = sectors_per_fat*bytes_per_sector) to ALL on-disk FAT copies (num_fats of
 * them, ADR-0003 DEC-07 "second (redundant) FAT"). Keeping the copies in sync
 * is mandatory: a real DOS / chkdsk reads either copy. Fail loud (Rule 2):
 * NULL/short buffer/no write_sectors -> error; a failing write -> FAT12_ERR_WRITE.
 */
int fat12_flush_fats(const fat12_volume_t *vol, const void *fat,
                     uint32_t fat_len);

/*
 * fat12_create: create (or TRUNCATE an existing) regular 8.3 file in the root
 * directory and copy its (zeroed-size, zero-start-cluster) directory entry to
 * *out_entry, along with the root-dir SLOT INDEX (0-based 32-byte entry index)
 * to *out_slot so a later fat12_write_file can patch size/start_cluster in place.
 *
 *   - `name83` is "NAME.EXT" / "NAME" (case-insensitive; upper-cased + space-
 *     padded into the 11-byte 8.3 fields). Names with '\\' or ':' are rejected.
 *   - If a regular file of that name already exists, it is TRUNCATED: its old
 *     cluster chain is freed (FAT updated, both copies flushed) and the entry
 *     reset to size 0 / start_cluster 0 (DOS CREAT semantics).
 *   - Otherwise the lowest free / deleted root slot is claimed; if the root dir
 *     is full -> FAT12_ERR_DIR_FULL (fail loud, Rule 2).
 *   - attr is stored (caller passes DIR_ATTR_ARCHIVE for a normal file).
 *   - mtime/mdate stamped to the fixed deterministic constant (Rule 11).
 *
 * SUBDIRECTORY PARENT (beads initech-zs24): `parent_dir_start` is the first data
 * cluster of the directory the file is created in (0 == the fixed root, byte-
 * identical to the historical root-only behavior). For a subdir the scan / slot
 * placement / dir-entry write-back walk the parent's CLUSTER CHAIN; when the
 * subdir is FULL it is GROWN by one cluster (fat12_grow_dir) so the new entry
 * lands in the fresh cluster -- exactly as real FAT12 / mtools grow a directory
 * (the fixed root cannot grow -> FAT12_ERR_DIR_FULL). `cluster_buf`
 * (>= sectors_per_cluster*512) is the directory-grow zero-fill scratch (it is
 * used ONLY on a subdir-full grow; a root create / a non-full subdir never
 * touches it, so it may be the same scratch the caller passes write_at).
 *
 * `fat`/`fat_len` is the whole-FAT buffer (mutated when truncating frees a chain
 * or a subdir grows). `sector_buf` (>=512) is scratch. Returns FAT12_OK; the dir
 * entry is already written to disk (size 0). */
int fat12_create(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                 const char *name83, uint8_t attr, uint16_t parent_dir_start,
                 void *sector_buf, void *cluster_buf, dir_entry_t *out_entry,
                 uint32_t *out_slot);

/*
 * fat12_write_file: write `len` bytes of `data` as the contents of the file at
 * root-dir slot `slot` (as returned by fat12_create). Allocates a fresh cluster
 * chain from the FAT free list (lowest-free-first), writes the data clusters
 * (the last cluster zero-padded to a full cluster on disk; file_size is
 * authoritative on read), links the FAT entries (EOC on the last), flushes BOTH
 * FAT copies, and patches the dir entry's size + start_cluster on disk.
 *
 * A zero-length write leaves start_cluster 0 (no chain) and size 0.
 * If the file already had a chain (re-write without truncate), the caller must
 * have truncated first (fat12_create truncates); this function assumes the slot
 * currently has start_cluster 0 / size 0.
 *
 * Fail loud (Rule 2): no free cluster -> FAT12_ERR_NO_SPACE (and the partial
 * allocation is rolled back so the volume is not corrupted); write error ->
 * FAT12_ERR_WRITE. `cluster_buf` (>= sectors_per_cluster*512) is scratch.
 * Returns FAT12_OK on success. */
int fat12_write_file(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                     uint32_t slot, const void *data, uint32_t len,
                     void *sector_buf, void *cluster_buf);

/*
 * fat12_write_partial: POSITIONED write of `len` bytes of `data` starting at
 * byte `offset` within the file at root-dir slot `slot` -- the symmetric
 * counterpart of fat12_read_partial for the per-handle file-I/O epic (beads
 * initech-snk; epic initech-6qy). OVERWRITES bytes in place where the file
 * already extends, EXTENDS the file (allocating + chaining new clusters from the
 * free list, growing the dir-entry size) where offset+len runs past the current
 * size, and ZERO-FILLS the gap when offset > current_size (the hole
 * current_size..offset reads back as zeroes).
 *
 * Semantics:
 *   - len == 0 -> no-op: *out_written = 0, size unchanged, FAT12_OK.
 *   - Otherwise writes EXACTLY len bytes; the file's size becomes
 *     max(old_size, offset+len). *out_written is the bytes committed (== len on
 *     success; 0 on a disk-full rollback).
 *   - Partial-cluster writes are read-modify-write at cluster granularity, so a
 *     shared cluster's neighbouring bytes are preserved. Newly allocated
 *     clusters (hole + extend) are zero-filled on disk.
 *
 * Allocate-then-commit (Rule 2): a disk-full extend frees every cluster THIS
 * call claimed and re-terminates the original chain (no half-corrupt chain that
 * diffs wrong) -> FAT12_ERR_NO_SPACE, *out_written 0.
 *
 * `fat`/`fat_len` is the whole-FAT buffer (mutated; both on-disk copies flushed).
 * `sector_buf` (>=512) is scratch for the dir-entry RMW; `cluster_buf`
 * (>= sectors_per_cluster*512) is scratch for the cluster RMW + zero-fill.
 *
 * SUBDIRECTORY PARENT (beads initech-zs24): `parent_dir_start` is the first data
 * cluster of the directory that CONTAINS the file (0 == the fixed root, byte-
 * identical to the historical root-only behavior). `slot` is the file's dir-
 * entry index WITHIN that directory -- for the root the root-dir-region slot, for
 * a subdir the linear index down the parent's cluster chain. Only WHERE the dir
 * entry's size/start_cluster are read+written changes; the file's OWN cluster
 * chain (the data) is independent of the parent directory.
 *
 * Fail loud (Rule 2): required NULL -> FAT12_ERR_NULL; no write backend ->
 * FAT12_ERR_WRITE; offset+len overflow -> FAT12_ERR_BUFFER; full volume ->
 * FAT12_ERR_NO_SPACE (rolled back); corrupt/cyclic chain -> FAT12_ERR_CHAIN; a
 * device error -> FAT12_ERR_READ/_WRITE. Returns FAT12_OK on success. */
int fat12_write_partial(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                        uint16_t parent_dir_start, uint32_t slot,
                        uint32_t offset, const uint8_t *data,
                        uint32_t len, void *sector_buf, void *cluster_buf,
                        uint32_t *out_written);

/*
 * fat12_unlink: delete the regular 8.3 file `name83` from the directory whose
 * first data cluster is `parent_dir_start` (0 == the fixed root, byte-identical
 * to the historical root-only behavior; a non-root value scans + write-backs the
 * deleted entry down the parent's CLUSTER CHAIN -- beads initech-zs24): mark its
 * dir entry deleted (filename[0] = 0xE5, DIR_NAME_DELETED) and free its whole
 * cluster chain (set each FAT entry to 0x000, flush both copies). A non-existent
 * name -> FAT12_ERR_NOT_FOUND. `fat`/`fat_len` is the whole-FAT buffer (mutated);
 * `sector_buf` (>=512) is scratch. Returns FAT12_OK. */
int fat12_unlink(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                 const char *name83, uint16_t parent_dir_start,
                 void *sector_buf);

/* ======================================================================== *
 * FAT12 subdirectory CREATE / REMOVE -- WRITE side (beads initech-u6wa)
 *
 * AH=39h MKDIR / AH=3Ah RMDIR over a ROOT-PARENT directory only: the new (or
 * removed) directory's PARENT is the fixed root. A non-root `parent_dir_start`
 * is OUT OF SCOPE for this landing (nested MD '\\SUB\\NEW' -> deferred to beads
 * initech-zs24) and returns FAT12_ERR_NOT_FOUND (which the backend maps to the
 * DOS 0x0003 PATH_NOT_FOUND), mirroring the existing
 * fat12_create/fat12_unlink root-only WRITE-side guard. The directory ENTRY
 * model below is the EMPIRICAL mtools 4.0.43 layout (mmd-minted, triple-
 * confirmed -- NOT inferred), so the artifact's writer is byte-identical to
 * mtools' on the meaningful bytes (name/attr/start_cluster/size + the EOC FAT
 * link); the implementation-specific mtime/mdate are stamped FIXED (Rule 11)
 * and normalized away before the differential diff.
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 4 (dir entry / '.'/'..');
 *   ADR-0003 DEC-07 (root dir is a FIXED region, subdirs are cluster chains);
 *   spec/dos_structs.h (DIR_ATTR_DIRECTORY); DOS 3.3 PRM AH=39h/3Ah.
 * ======================================================================== */

/* Reserved signal returns (beyond the FAT12_ERR_* set above) the WRITE-side
 * mkdir/rmdir use so the backend can map them to the right DOS code:
 *   FAT12_ERR_EXISTS    (reused) -> MKDIR: a name of that 8.3 already exists in
 *                                  the parent (the backend maps it to 0x0005
 *                                  ACCESS_DENIED -- DOS MKDIR-exists).
 *   FAT12_ERR_NOT_EMPTY (new)    -> RMDIR: the target directory still holds an
 *                                  entry other than '.'/'..' (the backend maps
 *                                  it to 0x0005 ACCESS_DENIED -- DOS RMDIR of a
 *                                  non-empty directory). */
#define FAT12_ERR_NOT_EMPTY (-15)

/*
 * fat12_mkdir: create a new subdirectory `name83` whose PARENT is the directory
 * at first data cluster `parent_dir_start` (0 == the fixed root; a NON-ROOT
 * value is OUT OF SCOPE -> FAT12_ERR_PATH_NOT_FOUND this landing). Steps:
 *   - reject a duplicate name in the parent (scan_root match) -> FAT12_ERR_EXISTS;
 *   - claim a free data cluster (find_free), set its FAT entry to EOC (0xFFF)
 *     and flush both FAT copies;
 *   - ZERO-FILL the new cluster's data sectors, then write the two canonical
 *     directory entries at offsets 0 and 32:
 *       '.'  name {0x2E, 0x20*10}, attr 0x10, start_cluster = the new cluster,
 *            size 0;
 *       '..' name {0x2E,0x2E, 0x20*9}, attr 0x10, start_cluster = parent_dir_start
 *            (0 for the root -- the EMPIRICAL mtools rule: ROOT is encoded as 0,
 *            NOT self, NOT 1), size 0;
 *     slot[2] stays 0x00 (end-of-directory); both '.'/'..' carry the FIXED
 *     mtime/mdate (Rule 11);
 *   - write the new directory's own 8.3 entry (DIR_ATTR_DIRECTORY,
 *     start_cluster = the new cluster, size 0) into the PARENT (scan_root free
 *     slot + write_dirent).
 *
 * `fat`/`fat_len` is the whole-FAT buffer (mutated; both copies flushed);
 * `sector_buf` (>= sectors_per_cluster*512) is scratch for the zero-fill +
 * dotdir write. Fail loud (Rule 2): no free cluster -> FAT12_ERR_NO_SPACE;
 * parent dir full -> FAT12_ERR_DIR_FULL; bad name -> FAT12_ERR_NOT_FOUND; no
 * write backend -> FAT12_ERR_WRITE. Returns FAT12_OK on success. */
int fat12_mkdir(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                const char *name83, uint16_t parent_dir_start, void *sector_buf);

/*
 * fat12_rmdir: remove the EMPTY subdirectory `name83` whose PARENT is the
 * directory at first data cluster `parent_dir_start` (0 == the fixed root; a
 * NON-ROOT value is OUT OF SCOPE -> FAT12_ERR_PATH_NOT_FOUND). Steps:
 *   - locate `name83` in the parent (scan_root match); a missing name OR a
 *     match that is NOT a directory -> FAT12_ERR_NOT_FOUND;
 *   - VERIFY-EMPTY by enumerating the target's own cluster (fat12_read_dir):
 *     only '.' and '..' may survive; any other entry -> FAT12_ERR_NOT_EMPTY;
 *   - free the target's cluster chain (free_chain) + flush both FAT copies;
 *   - mark the parent's dir slot deleted (filename[0] = 0xE5).
 *
 * `fat`/`fat_len` is the whole-FAT buffer (mutated; both copies flushed);
 * `sector_buf` (>= sectors_per_cluster*512) is scratch for the parent scan +
 * the empty-check enumeration. Fail loud (Rule 2): NULL -> FAT12_ERR_NULL; no
 * write backend -> FAT12_ERR_WRITE. Returns FAT12_OK on success. */
int fat12_rmdir(const fat12_volume_t *vol, void *fat, uint32_t fat_len,
                const char *name83, uint16_t parent_dir_start, void *sector_buf);

/* ======================================================================== *
 * FAT12 subdirectory / path traversal -- READ side (beads initech-ti8)
 *
 * Layer 1 of subdirectory support: enumerate ANY directory (root OR a subdir
 * cluster chain) and resolve a backslash-separated 8.3 path to its containing
 * directory + dir entry. The DOS-API (int21) wiring is the SEPARATE Layer 2
 * bead (initech-mzxa); this layer is additive at the fat12 layer only.
 *
 * Ref (Law 1): docs/research/fat12-ground-truth.md Sec 3 (cluster chain) +
 *   Sec 4 (dir entry / sentinels); ADR-0003 DEC-07 (root dir is a FIXED region,
 *   subdirs are cluster chains); spec/dos_structs.h (DIR_ATTR_DIRECTORY,
 *   BPB_CLUSTER_LBA). A subdirectory is NOT bounded by root_entry_count: it
 *   ends only at the 0x00 sentinel or EOC (reusing root_entry_count would
 *   silently truncate a large directory).
 * ======================================================================== */

/*
 * fat12_read_dir: enumerate a directory `dir` -- the fixed root OR a subdir
 * cluster chain -- invoking `cb` for each surviving (non-free, non-deleted,
 * non-LFN) 32-byte entry, with the SAME sentinels/filtering as
 * fat12_read_root_dir (0x00 STOP, 0xE5 SKIP, attr 0x0F LFN SKIP).
 *
 *   - dir->is_root: delegates to fat12_read_root_dir VERBATIM (the root path
 *     stays byte-identical so existing callers/oracles remain green).
 *   - else: walks dir->start_cluster's cluster chain. Each cluster is gated
 *     through fat12_cluster_in_range BEFORE the LBA math, its sectors are read
 *     at BPB_CLUSTER_LBA, the chain advances one fat12_next_cluster step at a
 *     time (mirroring fat12_read_partial's incremental walk), the step count is
 *     anti-hang bounded by vol->total_clusters+2 (Rule 2), and the walk stops
 *     at EOC. Within each cluster the per-32-byte-entry inner loop applies the
 *     same sentinels as the root reader; a non-zero cb return is propagated
 *     verbatim (early stop). A subdir is NOT bounded by root_entry_count.
 *
 * `sector_buf` (>= sectors_per_cluster*512 for the subdir path; >=512 for root)
 * is caller scratch; `fat`/`fat_len` is the whole-FAT buffer (only the subdir
 * path needs it). Fail loud (Rule 2): NULL arg -> FAT12_ERR_NULL; a read error
 * -> FAT12_ERR_READ; a corrupt/cyclic chain -> FAT12_ERR_CHAIN. Returns FAT12_OK
 * after the full scan, or the callback's non-zero value on an early stop.
 */
int fat12_read_dir(const fat12_volume_t *vol, const fat12_dir_t *dir,
                   void *sector_buf, const void *fat, uint32_t fat_len,
                   fat12_dirent_cb cb, void *user);

/*
 * fat12_resolve_path: resolve a backslash-separated 8.3 path (e.g.
 * "A:\\SUB\\DEEP\\DEEP.TXT") to its FINAL dir entry (*out_entry) and the
 * directory that contains it (*out_dir), starting from the root.
 *
 *   - A leading drive prefix (any letter + ':') is stripped.
 *   - The path is split on '\\'. Each NON-FINAL component is found in the
 *     current directory (fat12_read_dir + an exact 11-byte 8.3 compare) and
 *     MUST be a directory (DIR_ATTR_DIRECTORY) else FAT12_ERR_NOT_FOUND; the
 *     cursor descends into it. '.' is a no-op; '..' pops to the parent's
 *     start_cluster (start_cluster==0 normalizes to the root, is_root=1, BEFORE
 *     any cluster-LBA math so BPB_CLUSTER_LBA never underflows).
 *   - The FINAL component is resolved into *out_entry with *out_dir set to its
 *     containing directory. An empty path or a trailing '\\' resolves to the
 *     directory itself: *out_dir is that directory and *out_entry is a
 *     synthesized DIR_ATTR_DIRECTORY marker (start_cluster = the dir's).
 *
 * Component-internal '\\' / ':' stay rejected by parse_name83 (after the
 * top-level split). `sector_buf` (>= sectors_per_cluster*512) + `fat`/`fat_len`
 * (whole-FAT buffer) are scratch/decode inputs. Fail loud (Rule 2): NULL ->
 * FAT12_ERR_NULL; a missing/typed-wrong component -> FAT12_ERR_NOT_FOUND; read
 * errors propagated. Returns FAT12_OK with *out_dir + *out_entry set.
 */
int fat12_resolve_path(const fat12_volume_t *vol, void *sector_buf,
                       const void *fat, uint32_t fat_len, const char *path,
                       fat12_dir_t *out_dir, dir_entry_t *out_entry);

/*
 * fat12_resolve_path_from: identical to fat12_resolve_path EXCEPT the descent
 * begins in the directory whose first data cluster is `start_dir_cluster`
 * (0 == the fixed root, normalized BEFORE any cluster math) instead of always
 * the root. This is the additive base-seeding primitive a non-root CWD needs
 * (beads initech-u6wa, AH=3Bh CHDIR): a RELATIVE path (no leading '\', no drive
 * prefix) descends from the caller's CWD cluster, while the CALLER passes 0 for
 * an ABSOLUTE or drive-prefixed path so it descends from the root regardless.
 * fat12_resolve_path is now the thin start_dir_cluster==0 wrapper, so root
 * behavior is byte-identical. Ref: PRD Sec 6.5 (DOS path resolution); the DOS
 * 3.3 CHDIR contract (a relative path is taken from the current directory).
 */
int fat12_resolve_path_from(const fat12_volume_t *vol, void *sector_buf,
                            const void *fat, uint32_t fat_len, const char *path,
                            uint16_t start_dir_cluster,
                            fat12_dir_t *out_dir, dir_entry_t *out_entry);

/* ---- NEXT tasks (beads initech-adf continuation); NOT implemented here ---- *
 *   With root-dir enumeration + find + file-read landed, the FAT12 READ path
 *   is essentially complete. Remaining, as SEPARATE issues:
 *     - the DOS-API (int21) wiring of subdir traversal -- beads initech-mzxa
 *       (Layer 2; consumes fat12_read_dir / fat12_resolve_path from Layer 1).
 *     - the FAT WRITE path (allocate clusters, update both FATs, write dir
 *       entries) -- beads initech-509.11; blockdev write_sectors is still NULL.
 *     - the differential-vs-mtools `test-fat` gate wiring (beads initech-5cu).
 */

#endif /* INITECH_MILTON_FAT12_H */
