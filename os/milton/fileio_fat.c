/* fileio_fat.c -- FAT12-backed INT 21h file backend (beads initech-0qh; epic
 * initech-6qy -- multi-tenant positioned file I/O).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): shipped InitechDOS kernel code, freestanding
 * (only <stdint.h>). This is the concrete int21_file_backend_t the kernel binds
 * after a successful FAT12 mount: it bridges the host-testable INT 21h dispatch
 * logic (os/milton/int21.c) to the mounted volume (os/milton/fat12.c over
 * os/milton/ata.c). int21.c owns the SFT/JFT slots, the PER-HANDLE file offset,
 * and the DTA find-data write; THIS file owns the volume, the POSITIONED
 * cluster-chain read/write, and root-directory enumeration. It is NOT
 * host-testable (it would pull fat12.c + the volume into the unit oracle); the
 * host oracle (test_fileio.c) binds an in-memory MOCK backend instead. The
 * end-to-end FAT path is validated on the emulator (test-type / test-dir /
 * test-fatwrite / test-multiopen).
 *
 * Ref (Law 1): docs/research/fs-mount-sft-ground-truth.md Sec 4.1 (OPEN =
 *   locate), Sec 4.2 (positioned READ), Sec 4.5 (FINDFIRST/NEXT dir
 *   enumeration); os/milton/fat12.h (fat12_find / fat12_read_partial /
 *   fat12_write_partial / fat12_create / fat12_unlink / fat12_read_root_dir /
 *   fat12_read_fat); spec/dos_structs.h (dir_entry_t). CLAUDE.md Law 2 (oracle),
 *   Law 3 (artifact = C), Rule 2 (fail loud), Rule 11 (deterministic), Rule 12
 *   (ASCII).
 *
 * MULTI-TENANT, POSITIONED, STATELESS (beads initech-0qh): the single whole-file
 * buffer is GONE. open() merely LOCATES a file (fat12_find) and returns its dir
 * entry + root-dir slot; the SFT slot in int21.c carries the position and the
 * dir_entry copy, so any number of files may be open at once -- each its own SFT
 * slot -- and a >64 KiB file is served slice-by-slice with NO whole-file buffer.
 * read_at()/write_at() are positioned over the cluster chain via
 * fat12_read_partial / fat12_write_partial (both differential-proven vs mtools +
 * a python reference, test-fat-partial / test-fat-write-partial).
 *
 * SHARED SCRATCH (cooperative, single-threaded): the dispatcher runs sequential
 * INT 21h calls (no preemption -- PRD non-goal), so ONE shared cluster/sector
 * scratch (g_sector/g_cluster) is safe BETWEEN calls. Per-call reentrancy
 * hardening (an IRQ-issued INT 21h landing mid-call) is beads initech-xk2 -- out
 * of scope here, and the IRQ handlers touch no dispatcher state today.
 *
 * SAME-FILE COHERENCE CAVEAT (documented): each handle holds its OWN copy of the
 * dir_entry (size/start_cluster). Two handles open on the SAME file do not see
 * each other's writes until re-opened; concurrent write-sharing of one file is
 * not a goal (real DOS shares the SFT entry per path -- a future bead). Distinct
 * files are fully independent, which is what InitechBase (.dbf + .mdx) needs.
 */

#include "fileio_fat.h"
#include "fat12.h"
#include "int21.h"
#include "dos_structs.h"  /* dir_entry_t */

/* DOS error codes returned to int21.c (the same set int21.h uses). */
#define FILEIO_ERR_FILE_NOT_FOUND  0x0002u
#define FILEIO_ERR_PATH_NOT_FOUND  0x0003u
#define FILEIO_ERR_TOO_MANY_OPEN   0x0004u
#define FILEIO_ERR_ACCESS_DENIED   0x0005u

/* The maximum path the resolve seam copies into a stack buffer (beads
 * initech-mzxa). The int21.c side already bounds the ASCIIZ path to
 * INT21_PATH_SCAN_MAX (128) before calling resolve, so a small margin over that
 * is enough; an overlength path never reaches here. */
#define FILEIO_PATH_MAX  130u

/* The mounted volume (caller-owned; bound by fileio_fat_bind). */
static const fat12_volume_t *g_vol = 0;

/* The whole-FAT buffer (1.44 MB FAT12: 9 sectors * 512 = 4608 bytes; round up
 * to a comfortable cap so a slightly larger FAT still fits). BSS, kernel-owned;
 * filled once at bind by fat12_read_fat. The write functions MUTATE it in place
 * and flush BOTH on-disk FAT copies from it (fat12.h DEC-07). */
static uint8_t  g_fat[12u * 512u];
static uint32_t g_fat_len = 0;

/* Shared scratch (cooperative, single-threaded -- safe between sequential INT
 * 21h calls; see file header). `g_sector` (>=512) is the fat12_find /
 * root-dir-enumeration / dir-entry-RMW scratch; `g_cluster`
 * (sectors_per_cluster*512; one sector on 1.44 MB) is the positioned read/write
 * cluster scratch. Neither buffers a whole file. */
static uint8_t  g_sector[BLOCKDEV_SECTOR_SIZE];
static uint8_t  g_cluster[BLOCKDEV_SECTOR_SIZE];

/* ------------------------------------------------------------------------ *
 * Directory leaf lookup (beads initech-mzxa READ side; initech-zs24 WRITE side).
 * fat12_find_slot_in locates the bare 8.3 `name83` in the directory whose first
 * data cluster is `dir_start` (0 == the fixed root) and returns BOTH the matched
 * entry AND its dir-entry slot WITHIN that directory -- the root-dir-region index
 * for the root, the linear cluster-chain index for a subdir. The slot is what a
 * later positioned write_at() / refresh uses to write-back the entry in place, so
 * a subdir file is now fully WRITE-addressable (initech-zs24 lifted the READ-only
 * out_slot==0 restriction). dir_start==0 keeps the root path byte-identical.
 * ------------------------------------------------------------------------ */

/* Locate the bare 8.3 `name83` inside the directory whose first data cluster is
 * `dir_start` (0 == the fixed root) and return its entry + its slot WITHIN that
 * directory. Returns 0 on success (out_entry/out_slot set), or
 * FILEIO_ERR_FILE_NOT_FOUND. */
static uint16_t locate_in_dir(const char *name83, uint16_t dir_start,
                              dir_entry_t *out_entry, uint32_t *out_slot)
{
    dir_entry_t de;
    uint32_t    slot = 0;
    int rc = fat12_find_slot_in(g_vol, g_fat, g_fat_len, dir_start, g_sector,
                                name83, &de, &slot);
    if (rc != FAT12_OK) {
        return FILEIO_ERR_FILE_NOT_FOUND;
    }
    *out_entry = de;
    *out_slot  = slot;
    return 0u;
}

/* ------------------------------------------------------------------------ *
 * Backend vtable implementations (positioned, stateless).
 * ------------------------------------------------------------------------ */

/* OPEN: LOCATE the named file in the directory `dir_start_cluster` (0 == root;
 * beads initech-mzxa) -- NO whole-file read. Return its dir entry + (root) slot;
 * the SFT slot (int21.c) carries the position. Any number of files may be open
 * at once (each its own SFT slot). */
static uint16_t fat_open(const char *name83, uint16_t dir_start_cluster,
                         dir_entry_t *out_entry, uint32_t *out_slot)
{
    if (g_vol == 0) {
        return FILEIO_ERR_FILE_NOT_FOUND;   /* no volume -> as if absent */
    }
    return locate_in_dir(name83, dir_start_cluster, out_entry, out_slot);
}

/* READ_AT: positioned read over the cluster chain (fat12_read_partial). No
 * whole-file buffer; `e` is the per-handle dir entry copy, `offset` the
 * per-handle position. A read at/after EOF returns 0 bytes cleanly. */
static uint16_t fat_read_at(const dir_entry_t *e, uint32_t offset,
                            uint8_t *buf, uint32_t len, uint32_t *out_read)
{
    if (g_vol == 0) {
        *out_read = 0u;
        return FILEIO_ERR_FILE_NOT_FOUND;
    }
    uint32_t got = 0u;
    int rc = fat12_read_partial(g_vol, g_fat, g_fat_len, e, offset, len,
                                buf, g_cluster, &got);
    if (rc != FAT12_OK) {
        *out_read = 0u;
        return FILEIO_ERR_FILE_NOT_FOUND;   /* corrupt chain / read error */
    }
    *out_read = got;
    return 0u;
}

/* Cookie for the indexed dir-entry visitor: find the `target`-th surviving
 * entry and copy it out. fat12_read_root_dir invokes the callback for each
 * surviving (non-free, non-deleted, non-LFN) entry in order; we count to
 * `target` and stop. */
typedef struct {
    uint32_t    target;   /* the 0-based index we want                  */
    uint32_t    seen;     /* surviving entries seen so far              */
    dir_entry_t out;      /* the matched entry (valid when found==1)    */
    int         found;    /* 1 once the target index is matched         */
} dir_index_cookie_t;

static int dir_index_cb(const dir_entry_t *e, void *user)
{
    dir_index_cookie_t *c = (dir_index_cookie_t *)user;
    if (c->seen == c->target) {
        c->out   = *e;       /* struct copy (the entry lives in g_sector) */
        c->found = 1;
        return 1;            /* stop enumeration early */
    }
    c->seen++;
    return 0;                /* continue */
}

static uint16_t fat_dir_entry(uint32_t index, uint16_t dir_start_cluster,
                              dir_entry_t *out_entry, int *out_found)
{
    *out_found = 0;
    if (g_vol == 0) {
        return 0u;           /* no volume -> empty directory (not an error) */
    }

    dir_index_cookie_t c;
    c.target = index;
    c.seen   = 0;
    c.found  = 0;

    /* Enumerate the requested directory: the fixed root (dir_start==0) verbatim
     * via fat12_read_root_dir (byte-identical to the prior behavior), or a
     * SUBDIRECTORY cluster chain (dir_start!=0) via fat12_read_dir (Layer 1,
     * beads initech-mzxa). fat12_read_dir delegates the root case to
     * fat12_read_root_dir, so a single fat12_read_dir(dir) call would also work,
     * but keeping the explicit root path makes the root==0 contract obvious. */
    int rc;
    if (dir_start_cluster == 0u) {
        rc = fat12_read_root_dir(g_vol, g_sector, dir_index_cb, &c);
    } else {
        fat12_dir_t dir;
        dir.is_root       = 0;
        dir.start_cluster = dir_start_cluster;
        rc = fat12_read_dir(g_vol, &dir, g_sector, g_fat, g_fat_len,
                            dir_index_cb, &c);
    }
    /* fat12_read_root_dir / fat12_read_dir return FAT12_OK after a full scan, or
     * the callback's non-zero early-stop value (our cb returns 1 on a hit). A
     * negative rc is a real backend read error. */
    if (rc < 0) {
        return FILEIO_ERR_FILE_NOT_FOUND;   /* surfaced as a backend error */
    }
    if (c.found) {
        *out_entry = c.out;
        *out_found = 1;
    }
    return 0u;
}

/* ---- WRITE backend (beads initech-0qh, positioned) ------------------------ *
 * CREATE: create/truncate the file on disk (size 0), return its dir entry +
 * root-dir slot. WRITE_AT: positioned write over the chain (fat12_write_partial)
 * -- overwrite/extend/zero-fill-hole, both-FAT sync, allocate-then-commit
 * disk-full rollback; commits to disk per call (no deferred flush) and returns
 * the UPDATED dir entry so the caller refreshes its SFT copy. UNLINK: delete the
 * file + free its chain. All go through the oracle-green fat12.c write
 * functions over the mounted volume. */

static uint16_t fat_create(const char *name83, uint16_t dir_start_cluster,
                           dir_entry_t *out_entry, uint32_t *out_slot)
{
    if (g_vol == 0 || g_vol->dev == 0 || g_vol->dev->write_sectors == 0) {
        return FILEIO_ERR_ACCESS_DENIED;   /* read-only / no volume */
    }
    /* CREATE inside a SUBDIRECTORY (dir_start_cluster != 0) is now supported
     * (beads initech-zs24): fat12_create scans + write-backs down the parent's
     * cluster chain, growing the subdir by a cluster if it is full. dir_start==0
     * keeps the root path byte-identical. The g_cluster scratch doubles as the
     * directory-grow zero-fill buffer (only touched on a subdir-full grow, never
     * on a root create or a non-full subdir). */
#ifndef FILEIO_MUTATE_SUBDIR_CREATE_ROOTONLY
    uint16_t parent = dir_start_cluster;
#else
    /* MUTANT (Rule 6; make test-zs24-mutant only): keep the OLD root-only guard
     * so a subdir CREATE fails 0x0003 -- the subdir CREATE+WRITE round-trip then
     * never lands the file and the oracle goes RED. NEVER in a real build. */
    if (dir_start_cluster != 0u) {
        return FILEIO_ERR_PATH_NOT_FOUND;
    }
    uint16_t parent = 0u;
#endif

    dir_entry_t de;
    uint32_t    slot = 0;
    int rc = fat12_create(g_vol, g_fat, g_fat_len, name83, DIR_ATTR_ARCHIVE,
                          parent, g_sector, g_cluster, &de, &slot);
    if (rc == FAT12_ERR_DIR_FULL) {
        return FILEIO_ERR_TOO_MANY_OPEN;   /* dir full (root cannot grow) */
    }
    if (rc != FAT12_OK) {
        return FILEIO_ERR_ACCESS_DENIED;   /* bad name / write error */
    }

    *out_entry = de;
    *out_slot  = slot;
    return 0u;
}

static uint16_t fat_write_at(uint16_t dir_start, uint32_t slot, uint32_t offset,
                             const uint8_t *data, uint32_t len,
                             uint32_t *out_written, dir_entry_t *out_entry)
{
    if (g_vol == 0 || g_vol->dev == 0 || g_vol->dev->write_sectors == 0) {
        *out_written = 0u;
        return FILEIO_ERR_ACCESS_DENIED;
    }

    uint32_t wrote = 0u;
    int rc = fat12_write_partial(g_vol, g_fat, g_fat_len, dir_start, slot,
                                 offset, data, len, g_sector, g_cluster, &wrote);
    if (rc != FAT12_OK) {
        *out_written = 0u;
        return FILEIO_ERR_ACCESS_DENIED;   /* disk full (rolled back) / write err */
    }
    *out_written = wrote;

    /* Re-read the (now-patched) dir entry at the same slot OF THE SAME DIRECTORY
     * so the caller refreshes the SFT copy's size + start_cluster
     * (fat12_write_partial patched them on disk). For a subdir the slot is
     * cluster-chain-addressed (dir_start != 0); dir_start==0 is the root path. */
    {
        dir_entry_t de;
        int rc2 = fat12_read_dir_entry_in(g_vol, g_fat, g_fat_len, dir_start,
                                          g_sector, slot, &de);
        if (rc2 == FAT12_OK) {
            *out_entry = de;
        }
    }
    return 0u;
}

/* CLOSE: no-op with per-call flushing (write_at commits immediately). The hook
 * is kept for symmetry / a future deferred-buffering write model. */
static void fat_close(uint32_t slot)
{
    (void)slot;
}

/* SET_TIME (beads initech-qekc; AH=57h AL=01h SET FILE DATE/TIME): patch the
 * packed mtime/mdate of the dir entry at slot `slot` of the directory at
 * `dir_start` (0 == root) and FLUSH IMMEDIATELY (parity with the per-call
 * write_at commit). Delegates to the single fat12 primitive, which re-reads the
 * entry, overwrites ONLY the two packed words, and writes it back. A read-only
 * volume (no write_sectors) -> access denied. */
static uint16_t fat_set_time(uint16_t dir_start, uint32_t slot,
                             uint16_t mtime, uint16_t mdate)
{
    if (g_vol == 0 || g_vol->dev == 0 || g_vol->dev->write_sectors == 0) {
        return FILEIO_ERR_ACCESS_DENIED;
    }
#ifdef FILEIO_MUTATE_SETTIME_ROOTDIR
    /* MUTANT 6 (Rule 6; make test-qekc-mutant only): force dir_start=0 (the root)
     * so a SUBDIR file's stamp is written to a ROOT slot of the same index instead
     * of the subdir cluster-chain slot. The subdir --stat-path-time differential
     * goes RED (the subdir entry never gets the stamp). NEVER in a real build. */
    dir_start = 0u;
#endif
    int rc = fat12_set_dirent_time(g_vol, g_fat, g_fat_len, dir_start, slot,
                                   mtime, mdate, g_sector);
    if (rc != FAT12_OK) {
        return FILEIO_ERR_ACCESS_DENIED;   /* read/chain/write error */
    }
    return 0u;
}

/* CHMOD (beads initech-b53d; AH=43h GET/SET FILE ATTRIBUTES): GET (set==0) reads
 * the attribute byte of `name83` in directory `dir_start_cluster` (0 == root)
 * into *io_attr; SET (set==1) writes *io_attr as the new attribute byte. Both
 * delegate to the fat12 primitives, which scan the directory (subdir-aware) and,
 * for SET, write back ONLY the attribute byte (mtime/mdate preserved -- Rule 11).
 * GET works on a read-only volume and reports a directory/volume-label entry's
 * real attribute byte (CX=0x10 for a subdir) -- it does NOT reject (RBIL AX=4300h
 * has no directory exclusion; the reject is SET-only). A SET requires a writable
 * volume else access-denied. The fat12 access-denied (a SET dir/vol-label target,
 * or a SET attr that re-types the entry) maps to 0x0005; not-found maps to
 * 0x0002. */
static uint16_t fat_chmod(const char *name83, uint16_t dir_start_cluster,
                          int set, uint8_t *io_attr)
{
	int rc;

	if (io_attr == 0) {
		return FILEIO_ERR_ACCESS_DENIED;
	}
	if (set == 0) {
		/* GET: read-only -- only needs the volume mounted + readable. */
		if (g_vol == 0) {
			return FILEIO_ERR_FILE_NOT_FOUND;   /* no volume -> as if absent */
		}
		rc = fat12_get_attr(g_vol, g_fat, g_fat_len, name83, dir_start_cluster,
		                    g_sector, io_attr);
	} else {
		/* SET: requires a writable backend (Rule 2: never a silent no-op). */
		if (g_vol == 0 || g_vol->dev == 0 || g_vol->dev->write_sectors == 0) {
			return FILEIO_ERR_ACCESS_DENIED;
		}
		rc = fat12_set_attr(g_vol, g_fat, g_fat_len, name83, dir_start_cluster,
		                    *io_attr, g_sector);
	}
	if (rc == FAT12_ERR_NOT_FOUND) {
#ifndef FILEIO_MUTATE_CHMOD_NOTFOUND_ACCESS
		return FILEIO_ERR_FILE_NOT_FOUND;       /* 0x0002 */
#else
		/* MUTANT 5 (Rule 6; make test-b53d-mutant only): map a fat12 NOT_FOUND to
		 * ACCESS_DENIED (0x0005) instead of FILE_NOT_FOUND (0x0002), so a CHMOD of
		 * a missing file reports the wrong DOS error. The missing-file 0x0002
		 * contract goes RED. NEVER in a real build. */
		return FILEIO_ERR_ACCESS_DENIED;
#endif
	}
	if (rc != FAT12_OK) {
		return FILEIO_ERR_ACCESS_DENIED;        /* 0x0005 (dir/vol-label / write) */
	}
	return 0u;
}

static uint16_t fat_unlink(const char *name83, uint16_t dir_start_cluster)
{
    if (g_vol == 0 || g_vol->dev == 0 || g_vol->dev->write_sectors == 0) {
        return FILEIO_ERR_ACCESS_DENIED;
    }
    /* UNLINK inside a SUBDIRECTORY (dir_start_cluster != 0) is now supported
     * (beads initech-zs24): fat12_unlink scans + marks the entry deleted down the
     * parent's cluster chain. dir_start==0 keeps the root path byte-identical. */
#ifndef FILEIO_MUTATE_SUBDIR_UNLINK_NOOP
    {
        int rc = fat12_unlink(g_vol, g_fat, g_fat_len, name83,
                              dir_start_cluster, g_sector);
        if (rc == FAT12_ERR_NOT_FOUND) {
            return FILEIO_ERR_FILE_NOT_FOUND;
        }
        if (rc != FAT12_OK) {
            return FILEIO_ERR_ACCESS_DENIED;
        }
    }
#else
    /* MUTANT (Rule 6; make test-zs24-mutant only): make a SUBDIR unlink a NO-OP
     * (report success without deleting), so `mdir ::SUB` still lists the file and
     * the UNLINK oracle goes RED. The root path is unchanged. NEVER in a real
     * build. */
    if (dir_start_cluster != 0u) {
        return 0u;   /* lie: pretend it was deleted */
    }
    {
        int rc = fat12_unlink(g_vol, g_fat, g_fat_len, name83,
                              dir_start_cluster, g_sector);
        if (rc == FAT12_ERR_NOT_FOUND) {
            return FILEIO_ERR_FILE_NOT_FOUND;
        }
        if (rc != FAT12_OK) {
            return FILEIO_ERR_ACCESS_DENIED;
        }
    }
#endif
    return 0u;
}

/* RENAME (AH=56h, SAME-directory dir-entry rename; beads initech-gnrc): rename
 * the 8.3 file `old83` to `new83` within the directory `old_dir` (0 == root). The
 * dispatcher already rejected the cross-directory case (old_dir != new_dir ->
 * 0x0011 NOT_SAME_DEVICE) BEFORE this seam, so old_dir == new_dir holds; both are
 * passed for symmetry (and a future cross-dir MOVE, beads initech-ycb3). The
 * backend forwards old_dir to fat12_rename, which scans + name-field-rewrites down
 * the parent's cluster chain (dir_start==0 keeps the root path byte-identical).
 * fat12_rename maps:
 *   FAT12_ERR_NOT_FOUND -> 0x0002 (source missing -- DOS RENAME not-found);
 *   FAT12_ERR_EXISTS    -> 0x0005 (dest name already present -- access denied);
 *   FAT12_ERR_ACCESS    -> 0x0005 (a directory/volume-label source);
 *   FAT12_OK            -> 0;
 *   any other error     -> 0x0005 (write error). */
static uint16_t fat_rename(const char *old83, uint16_t old_dir,
                           const char *new83, uint16_t new_dir)
{
    if (g_vol == 0 || g_vol->dev == 0 || g_vol->dev->write_sectors == 0) {
        return FILEIO_ERR_ACCESS_DENIED;   /* read-only / no volume */
    }
    /* old_dir == new_dir is guaranteed by the dispatcher (cross-dir -> 0x0011);
     * the cross-dir MOVE that would use new_dir is deferred (beads initech-ycb3). */
    (void)new_dir;
    int rc = fat12_rename(g_vol, g_fat, g_fat_len, old83, new83, old_dir,
                          g_sector);
    if (rc == FAT12_OK) {
        return 0u;
    }
    if (rc == FAT12_ERR_NOT_FOUND) {
        return FILEIO_ERR_FILE_NOT_FOUND;   /* 0x0002 source missing */
    }
    /* EXISTS (dest present), ACCESS (dir/vol-label source), or a write error all
     * map to DOS access-denied for RENAME (0x0005). */
    return FILEIO_ERR_ACCESS_DENIED;
}

/* MKDIR (AH=39h CREATE DIRECTORY): create the new subdirectory `name83` in the
 * directory `dir_start_cluster` (0 == root). A NON-ROOT parent is now supported
 * (beads initech-m0bp): fat12_mkdir scans + write-backs down the parent's
 * cluster chain, GROWING the parent subdir by a cluster if it is full. The INT21
 * dispatch already resolved the parent path to dir_start_cluster, so the backend
 * just forwards it. The g_cluster scratch is the parent-grow zero-fill buffer
 * (DISTINCT from the g_sector scratch fat12_mkdir uses to zero-fill the NEW
 * dir's cluster + RMW the parent entry -- they MUST be different live buffers).
 * fat12_mkdir maps:
 *   FAT12_ERR_EXISTS    -> 0x0005 (DOS MKDIR-exists);
 *   FAT12_ERR_DIR_FULL  -> 0x0005 (parent dir full -- access denied for MKDIR);
 *   FAT12_OK            -> 0;
 *   any other error     -> 0x0005 (bad name / no space / write error). */
static uint16_t fat_mkdir(const char *name83, uint16_t dir_start_cluster)
{
    if (g_vol == 0 || g_vol->dev == 0 || g_vol->dev->write_sectors == 0) {
        return FILEIO_ERR_ACCESS_DENIED;   /* read-only / no volume */
    }
    int rc = fat12_mkdir(g_vol, g_fat, g_fat_len, name83, dir_start_cluster,
                         g_sector, g_cluster);
    if (rc == FAT12_OK) {
        return 0u;
    }
    /* Every failure mode maps to ACCESS_DENIED for DOS MKDIR (name exists, dir
     * full, full volume, bad name, write error). */
    return FILEIO_ERR_ACCESS_DENIED;
}

/* RMDIR (AH=3Ah REMOVE DIRECTORY): remove the EMPTY subdirectory `name83` from
 * the directory `dir_start_cluster` (0 == root). A NON-ROOT parent is now
 * supported (beads initech-m0bp): fat12_rmdir scans + marks the entry deleted
 * down the parent's cluster chain (the empty-check already enumerates the
 * TARGET's chain). The INT21 dispatch resolved the parent to dir_start_cluster;
 * the backend forwards it. RMDIR never grows the parent, so no cluster_buf is
 * needed. fat12_rmdir maps:
 *   FAT12_ERR_NOT_FOUND -> 0x0003 (missing dir / not a directory -- DOS RMDIR
 *                          path-not-found);
 *   FAT12_ERR_NOT_EMPTY -> 0x0005 (DOS RMDIR of a non-empty directory);
 *   FAT12_OK            -> 0;
 *   any other error     -> 0x0005 (write error). */
static uint16_t fat_rmdir(const char *name83, uint16_t dir_start_cluster)
{
    if (g_vol == 0 || g_vol->dev == 0 || g_vol->dev->write_sectors == 0) {
        return FILEIO_ERR_ACCESS_DENIED;
    }
    int rc = fat12_rmdir(g_vol, g_fat, g_fat_len, name83, dir_start_cluster,
                         g_sector);
    if (rc == FAT12_OK) {
        return 0u;
    }
    if (rc == FAT12_ERR_NOT_FOUND) {
        return FILEIO_ERR_PATH_NOT_FOUND;  /* missing dir / not a directory */
    }
    if (rc == FAT12_ERR_NOT_EMPTY) {
        return FILEIO_ERR_ACCESS_DENIED;   /* non-empty -- DOS RMDIR-non-empty */
    }
    return FILEIO_ERR_ACCESS_DENIED;       /* write error */
}

/* FREESPACE (AH=36h GET DISK FREE SPACE): report the volume geometry + free
 * clusters. Walk the cached whole-FAT (g_fat) once, decoding each data cluster's
 * 12-bit entry and counting the free (0x000) ones. The geometry comes from the
 * mounted BPB / derived volume fields. All four out-values are clamped to 16 bits
 * (the AH=36h registers are 16-bit); a 1.44 MB FAT12 has 2847 data clusters and
 * 512-byte sectors so nothing overflows. Returns 0 on success, non-zero with no
 * volume mounted. Ref (Law 1): Microsoft FAT spec (free=0x000); DOS 3.3 PRM
 * AH=36h; os/milton/fat12.h (fat12_next_cluster / fat12_is_free). */
static uint16_t fat_freespace(uint16_t *out_spc, uint16_t *out_bps,
                              uint16_t *out_total_clus, uint16_t *out_free_clus)
{
    if (g_vol == 0 || g_fat_len == 0u) {
        return 1u;   /* no volume mounted -> invalid drive (AX=0xFFFF) */
    }

    uint32_t total = g_vol->total_clusters;
    uint32_t freec = 0u;

    /* Data clusters are numbered 2 .. (total_clusters + 1). Decode each FAT
     * entry; count the free ones. fat12_next_cluster returns the raw 12-bit
     * value; fat12_is_free tests == 0x000. */
    for (uint32_t c = FAT12_FIRST_DATA_CLUSTER;
         c < FAT12_FIRST_DATA_CLUSTER + total; c++) {
        uint16_t v = 0u;
        int rc = fat12_next_cluster(g_vol, g_fat, g_fat_len, (uint16_t)c, &v);
        if (rc != FAT12_OK) {
            return 1u;   /* a decode error -> fail loud (Rule 2) */
        }
        if (fat12_is_free(v)) {
            freec++;
        }
    }

    *out_spc        = (uint16_t)g_vol->bpb.sectors_per_cluster;
    *out_bps        = (uint16_t)g_vol->bpb.bytes_per_sector;
    *out_total_clus = (uint16_t)(total & 0xFFFFu);
    *out_free_clus  = (uint16_t)(freec & 0xFFFFu);
    return 0u;
}

/* RESOLVE (beads initech-mzxa; ti8 Layer 2 -- the path->containing-directory
 * seam int21.c calls so it never includes fat12.h). Split off the bare final 8.3
 * leaf (*out_leaf, a pointer INTO `path`) and resolve the PARENT directory chain
 * to its first data cluster (*out_dir_start, 0 == root) via fat12_resolve_path
 * (Layer 1). `cwd_start` is the start cluster a RELATIVE path resolves from; this
 * milestone the CWD is always the root (no CHDIR writer yet) so cwd_start is 0
 * and a relative path resolves from the root exactly like an absolute one --
 * fat12_resolve_path always starts at the root (initech-u6wa adds a non-root CWD
 * base). int21.c already stripped a leading 'X:' drive; we re-skip defensively.
 * Returns 0 with *out_leaf + *out_dir_start set, or FILEIO_ERR_PATH_NOT_FOUND
 * (0x0003) for a missing / non-directory parent component (int21.c maps any
 * non-zero return to AX=0x0003). */
/* ------------------------------------------------------------------------ *
 * ONE base-seeding rule, shared by resolve() and resolve_dir() (beads
 * initech-u6wa). DOS resolves a RELATIVE path (no leading '\', no 'X:' drive
 * prefix) from the CURRENT directory; an ABSOLUTE or drive-prefixed path always
 * from the root. `path` is the original ASCIIZ (drive prefix still present);
 * returns the cluster fat12_resolve_path_from should descend from: cwd_start for
 * a relative path, 0 (root) for absolute/drive-prefixed. (The drive letter is
 * ignored for the descent -- one volume this milestone -- but its PRESENCE makes
 * the path absolute.) This is the single seam mutant m5 reverts to always-root
 * to prove the relative-CWD oracle bites (Rule 6). Ref: PRD Sec 6.5; DOS 3.3
 * CHDIR/OPEN path resolution. */
static uint16_t fat_descend_seed(const char *path, uint16_t cwd_start)
{
#ifndef FILEIO_MUTATE_CWD_NOROOT
    int had_drive = (path[0] != '\0' && path[1] == ':');
    const char *after_drive = had_drive ? path + 2 : path;
    int absolute = (after_drive[0] == '\\');
    if (had_drive || absolute) {
        return 0u;                 /* root: absolute or drive-prefixed */
    }
    return cwd_start;              /* relative: descend from the CWD */
#else
    /* MUTANT m5 (Rule 6; make test-u6wa-mutant only): IGNORE cwd_start and
     * always seed from the root, reverting the relative-CWD fix. A relative
     * multi-component CHDIR (e.g. 'DEEP' from CWD '\SUB') then wrongly resolves
     * from the root and goes RED. NEVER define in a real build. */
    (void)cwd_start; (void)path;
    return 0u;
#endif
}

static uint16_t fat_resolve(const char *path, uint16_t cwd_start,
                            const char **out_leaf, uint16_t *out_dir_start)
{
    if (g_vol == 0) {
        return FILEIO_ERR_PATH_NOT_FOUND;   /* no volume -> nothing resolves */
    }

    /* Seed the parent descent from the CWD for a relative path, the root for an
     * absolute/drive-prefixed one -- BEFORE the drive strip, so the prefix is
     * still visible to fat_descend_seed (beads initech-u6wa; this REPLACES the
     * old '(void)cwd_start' that always resolved from the root). */
    uint16_t seed = fat_descend_seed(path, cwd_start);

    /* Skip a leading drive prefix defensively (int21.c already did). */
    const char *p = path;
    if (p[0] != '\0' && p[1] == ':') {
        p += 2;
    }

    /* Find the last backslash: the leaf is everything after it; the parent path
     * is everything up to and INCLUDING it (a trailing '\' makes
     * fat12_resolve_path return the directory itself per Layer 1). */
    const char *last_sep = 0;
    uint32_t    plen     = 0u;
    for (const char *q = p; *q; q++) {
        if (*q == '\\') {
            last_sep = q;
        }
        plen++;
    }

    if (last_sep == 0) {
        /* A bare 8.3 name -> it lives directly in the current directory (the
         * CWD seed; 0 == root). The leaf is the whole (drive-stripped) name. */
        *out_leaf      = p;
        *out_dir_start = seed;
        return 0u;
    }

    *out_leaf = last_sep + 1;

    /* Copy the parent path [p .. last_sep] INCLUSIVE (keep the trailing '\') into
     * a bounded stack buffer and resolve it to the containing directory. The
     * int21.c side bounds the ASCIIZ path to INT21_PATH_SCAN_MAX (128) before
     * calling, so it fits FILEIO_PATH_MAX; guard anyway (Rule 2). */
    uint32_t parent_len = (uint32_t)((last_sep - p) + 1);   /* include the '\' */
    if (parent_len >= FILEIO_PATH_MAX) {
        return FILEIO_ERR_PATH_NOT_FOUND;       /* overlength -> fail loud */
    }
    char parent[FILEIO_PATH_MAX];
    for (uint32_t i = 0u; i < parent_len; i++) {
        parent[i] = p[i];
    }
    parent[parent_len] = '\0';

    fat12_dir_t dir;
    dir_entry_t e;
    int rc = fat12_resolve_path_from(g_vol, g_sector, g_fat, g_fat_len, parent,
                                     seed, &dir, &e);
    if (rc != FAT12_OK) {
        return FILEIO_ERR_PATH_NOT_FOUND;       /* missing/non-dir component */
    }
    /* fat12_resolve_path with a trailing '\' after a FILE component (e.g.
     * "\SUB\NESTED.TXT\") resolves to the FILE itself (attr 0x20), NOT a
     * directory -- so a path like '\SUB\NESTED.TXT\X' must be rejected here: the
     * parent of leaf X is a file, not a traversable directory (the non-dir
     * mid-path case). A real directory parent carries DIR_ATTR_DIRECTORY (the
     * root marker too); accept only those. */
    if ((e.attribute & DIR_ATTR_DIRECTORY) == 0u) {
        return FILEIO_ERR_PATH_NOT_FOUND;       /* a file is not traversable */
    }
    /* The directory parent: *out_entry's start_cluster is that directory's first
     * data cluster (0 normalizes to the root). */
    *out_dir_start = e.start_cluster;
    return 0u;
}

/* ------------------------------------------------------------------------ *
 * RESOLVE_DIR (beads initech-u6wa; AH=3Bh CHDIR -- a FULL path -> a DIRECTORY).
 *
 * Unlike resolve() (which splits a file leaf off the PARENT), this resolves the
 * WHOLE path to a directory and validates it IS one. The canonical text is then
 * derived from the FILESYSTEM STRUCTURE -- a reverse '..' walk from the resolved
 * cluster up to the root -- NOT from the input text. That makes a RELATIVE, a
 * '.', or a '..' path canonicalize identically to the equivalent absolute path
 * (the deep, root-cause solution; Rule 3), and it is the only way to build the
 * absolute canon from cwd_start alone (the vtable signature carries no cwd text).
 * ------------------------------------------------------------------------ */

/* Reverse-walk cookie: find the directory entry whose start_cluster matches
 * `want_cluster` and is a real subdir (DIR_ATTR_DIRECTORY, name not '.'/'..').
 * Captures that child's 8.3 name so the parent level of the canon is named. */
typedef struct {
    uint16_t want_cluster;          /* the child's first data cluster        */
    char     name[FAT12_NAME83_MAX];/* the matched child's 8.3 name          */
    int      found;                 /* 1 once the name is captured           */
} child_name_cookie_t;

/* A '.'/'..' dot-entry has filename[0]=='.', so skip those by name. */
static int child_name_cb(const dir_entry_t *e, void *user)
{
    child_name_cookie_t *c = (child_name_cookie_t *)user;
    if ((e->attribute & DIR_ATTR_DIRECTORY) == 0u) {
        return 0;                   /* not a directory */
    }
    if (e->filename[0] == (uint8_t)'.') {
        return 0;                   /* the '.' / '..' dot-entries */
    }
    if (e->start_cluster != c->want_cluster) {
        return 0;
    }
    fat12_format_83(e, c->name);
    c->found = 1;
    return 1;                       /* stop enumeration */
}

/* Reverse-walk cookie: read a subdir's own '..' entry to learn its parent's
 * start_cluster (0 == root after the start_cluster==0 => root normalize). */
typedef struct {
    uint16_t parent_cluster;        /* the '..' entry's start_cluster        */
    int      found;
} dotdot_cookie_t;

static int dotdot_cb(const dir_entry_t *e, void *user)
{
    dotdot_cookie_t *c = (dotdot_cookie_t *)user;
    if (e->filename[0] == (uint8_t)'.' && e->filename[1] == (uint8_t)'.' &&
        e->filename[2] == (uint8_t)' ') {
        c->parent_cluster = e->start_cluster;   /* 0 => root */
        c->found = 1;
        return 1;
    }
    return 0;
}

/* Build the canonical ROOT-RELATIVE text of the directory at `dir_cluster`
 * (0 == root) into `canon` (bounded by `canon_max`, NUL-terminated). Root => the
 * empty string. Walks UP via each level's '..' entry, naming each level by
 * matching its start_cluster in its parent, then JOINS the names top-down with
 * single '\' separators. Returns 0 on success, or 0x0003 on a malformed chain /
 * an overlength canon (fail loud -- NEVER a truncated path; Rule 2). */
static uint16_t fat_canon_from_cluster(uint16_t dir_cluster, char *canon,
                                       uint32_t canon_max)
{
    /* Collect names bottom-up (deepest first); FAT12 directory nesting is
     * bounded well under this on a 1.44 MB volume. */
    char     names[16][FAT12_NAME83_MAX];
    uint32_t depth = 0u;
    uint16_t cur   = dir_cluster;

    if (canon == 0 || canon_max == 0u) {
        return FILEIO_ERR_PATH_NOT_FOUND;
    }
    canon[0] = '\0';
    if (cur == 0u) {
        return 0u;                  /* the root: the empty string */
    }

    while (cur != 0u) {
        fat12_dir_t cdir;
        dotdot_cookie_t dd;
        child_name_cookie_t cn;
        int rc;

        if (depth >= (uint32_t)(sizeof(names) / sizeof(names[0]))) {
            return FILEIO_ERR_PATH_NOT_FOUND;   /* pathologically deep */
        }

        /* Parent cluster = cur's own '..' entry. */
        cdir.is_root       = 0;
        cdir.start_cluster = cur;
        dd.parent_cluster  = 0u;
        dd.found           = 0;
        rc = fat12_read_dir(g_vol, &cdir, g_sector, g_fat, g_fat_len,
                            dotdot_cb, &dd);
        if (rc < 0 || !dd.found) {
            return FILEIO_ERR_PATH_NOT_FOUND;   /* no '..' -> corrupt subdir */
        }

        /* cur's NAME = the entry in the parent whose start_cluster == cur. */
        {
            fat12_dir_t pdir;
            pdir.is_root       = (dd.parent_cluster == 0u) ? 1 : 0;
            pdir.start_cluster = dd.parent_cluster;
            cn.want_cluster    = cur;
            cn.found           = 0;
            rc = fat12_read_dir(g_vol, &pdir, g_sector, g_fat, g_fat_len,
                                child_name_cb, &cn);
            if (rc < 0 || !cn.found) {
                return FILEIO_ERR_PATH_NOT_FOUND;   /* unreachable from parent */
            }
        }

        {
            uint32_t k = 0u;
            for (; cn.name[k] != '\0' && k < FAT12_NAME83_MAX - 1u; k++) {
                names[depth][k] = cn.name[k];
            }
            names[depth][k] = '\0';
        }
        depth++;
        cur = dd.parent_cluster;
    }

    /* Join names top-down (depth-1 .. 0) with single '\' separators. */
    {
        uint32_t pos = 0u;
        uint32_t li  = depth;
        while (li > 0u) {
            uint32_t i;
            li--;
            if (pos != 0u) {
                if (pos + 1u >= canon_max) {
                    return FILEIO_ERR_PATH_NOT_FOUND;   /* overlength */
                }
                canon[pos++] = '\\';
            }
            for (i = 0u; names[li][i] != '\0'; i++) {
                if (pos + 1u >= canon_max) {
                    return FILEIO_ERR_PATH_NOT_FOUND;   /* overlength */
                }
                canon[pos++] = names[li][i];
            }
        }
        canon[pos] = '\0';
    }
    return 0u;
}

static uint16_t fat_resolve_dir(const char *path, uint16_t cwd_start,
                                uint16_t *out_dir_start, char *out_canon,
                                uint32_t canon_max)
{
    if (g_vol == 0) {
        return FILEIO_ERR_PATH_NOT_FOUND;   /* no volume -> nothing resolves */
    }

    /* Same base-seeding rule resolve() uses: a relative path descends from the
     * CWD, an absolute / drive-prefixed one from the root (beads initech-u6wa). */
    uint16_t seed = fat_descend_seed(path, cwd_start);

    fat12_dir_t dir;
    dir_entry_t e;
    int rc = fat12_resolve_path_from(g_vol, g_sector, g_fat, g_fat_len, path,
                                     seed, &dir, &e);
    if (rc != FAT12_OK) {
        return FILEIO_ERR_PATH_NOT_FOUND;   /* missing component (0x0003) */
    }

    /* The FINAL component MUST be a directory: CHDIR into a file is path-not-
     * found (the DOS contract). fat12_resolve_path synthesizes a directory
     * marker for the dir-itself / trailing-'\' case, so a real dir carries
     * DIR_ATTR_DIRECTORY. (The reverse-'..'-walk canonicalizer below is a second
     * structural guard -- a file cluster has no '..' entry -- but the attr gate
     * is the authoritative, cheap rejection; the m1 mutant proof lives in the
     * MOCK oracle test_fileio.c where the file-rejection has no such redundancy.) */
    if ((e.attribute & DIR_ATTR_DIRECTORY) == 0u) {
        return FILEIO_ERR_PATH_NOT_FOUND;   /* a file is not a directory */
    }

    uint16_t target = e.start_cluster;      /* 0 normalizes to the root */
    if (out_canon != 0 && canon_max > 0u) {
        uint16_t cerr = fat_canon_from_cluster(target, out_canon, canon_max);
        if (cerr != 0u) {
            return cerr;                    /* malformed chain / overlength */
        }
    }
    *out_dir_start = target;
    return 0u;
}

static const int21_file_backend_t g_fat_backend = {
    fat_open,
    fat_read_at,
    fat_dir_entry,
    fat_create,
    fat_write_at,
    fat_close,
    fat_unlink,
    fat_freespace,
    fat_resolve,
    fat_resolve_dir,
    fat_mkdir,          /* AH=39h MKDIR (beads initech-u6wa)  */
    fat_rmdir,          /* AH=3Ah RMDIR (beads initech-u6wa)  */
    fat_set_time,       /* AH=57h SET FILE DATE/TIME (beads initech-qekc) */
    fat_chmod,          /* AH=43h CHMOD GET/SET ATTRIBUTES (beads initech-b53d) */
    fat_rename          /* AH=56h RENAME same-directory dir-entry (beads initech-gnrc) */
};

/* ------------------------------------------------------------------------ *
 * Bind: cache the FAT, then hand the backend vtable to int21.
 * ------------------------------------------------------------------------ */

int fileio_fat_bind(const fat12_volume_t *vol)
{
    if (vol == 0) {
        return -1;
    }

    /* Cache the whole FAT once. The positioned read/write functions need it for
     * the cluster-chain walk; the write functions mutate it + flush both copies. */
    int rc = fat12_read_fat(vol, g_fat, (uint32_t)sizeof(g_fat));
    if (rc != FAT12_OK) {
        return rc;   /* fail loud: a too-large FAT or read error surfaces */
    }
    g_fat_len = (uint32_t)vol->bpb.sectors_per_fat *
                (uint32_t)vol->bpb.bytes_per_sector;
    g_vol     = vol;

    int21_set_file_backend(&g_fat_backend);
    return 0;
}

/* Expose the backend's already-cached FAT so SYSINIT (CONFIG.SYS read) can walk
 * the cluster chain WITHOUT allocating a SECOND ~4.6 KiB FAT buffer -- the kernel
 * .bss was butting against PROGRAM_BASE (beads initech-509.2; _kernel_end margin).
 * Valid only after a successful fileio_fat_bind; returns 0 (and *out_len = 0) if
 * unbound. The buffer is the live backend cache; a caller that re-reads the same
 * on-disk FAT into it is idempotent in the single-threaded SYSINIT context. */
void *fileio_fat_fat_buffer(uint32_t *out_len)
{
    if (g_fat_len == 0u) {
        if (out_len != 0) {
            *out_len = 0u;
        }
        return 0;
    }
    if (out_len != 0) {
        *out_len = g_fat_len;
    }
    return g_fat;
}
