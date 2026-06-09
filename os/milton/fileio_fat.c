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
#define FILEIO_ERR_TOO_MANY_OPEN   0x0004u
#define FILEIO_ERR_ACCESS_DENIED   0x0005u

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
 * Backend vtable implementations (positioned, stateless).
 * ------------------------------------------------------------------------ */

/* OPEN: LOCATE the named file -- NO whole-file read. Return its dir entry +
 * root-dir slot; the SFT slot (int21.c) carries the position. Any number of
 * files may be open at once (each its own SFT slot). */
static uint16_t fat_open(const char *name83, dir_entry_t *out_entry,
                         uint32_t *out_slot)
{
    if (g_vol == 0) {
        return FILEIO_ERR_FILE_NOT_FOUND;   /* no volume -> as if absent */
    }

    dir_entry_t de;
    uint32_t    slot = 0;
    int rc = fat12_find_slot(g_vol, g_sector, name83, &de, &slot);
    if (rc == FAT12_ERR_NOT_FOUND) {
        return FILEIO_ERR_FILE_NOT_FOUND;
    }
    if (rc != FAT12_OK) {
        return FILEIO_ERR_FILE_NOT_FOUND;   /* a read error reads as absent here */
    }

    *out_entry = de;          /* struct copy of the 32-byte dir entry */
    *out_slot  = slot;        /* root-dir slot for a later write-back  */
    return 0u;
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

static uint16_t fat_dir_entry(uint32_t index, dir_entry_t *out_entry,
                              int *out_found)
{
    *out_found = 0;
    if (g_vol == 0) {
        return 0u;           /* no volume -> empty directory (not an error) */
    }

    dir_index_cookie_t c;
    c.target = index;
    c.seen   = 0;
    c.found  = 0;

    int rc = fat12_read_root_dir(g_vol, g_sector, dir_index_cb, &c);
    /* fat12_read_root_dir returns FAT12_OK after a full scan, or the callback's
     * non-zero early-stop value (our cb returns 1 on a hit). A negative rc is a
     * real backend read error. */
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

static uint16_t fat_create(const char *name83, dir_entry_t *out_entry,
                           uint32_t *out_slot)
{
    if (g_vol == 0 || g_vol->dev == 0 || g_vol->dev->write_sectors == 0) {
        return FILEIO_ERR_ACCESS_DENIED;   /* read-only / no volume */
    }

    dir_entry_t de;
    uint32_t    slot = 0;
    int rc = fat12_create(g_vol, g_fat, g_fat_len, name83, DIR_ATTR_ARCHIVE,
                          g_sector, &de, &slot);
    if (rc == FAT12_ERR_DIR_FULL) {
        return FILEIO_ERR_TOO_MANY_OPEN;   /* root dir full */
    }
    if (rc != FAT12_OK) {
        return FILEIO_ERR_ACCESS_DENIED;   /* bad name / write error */
    }

    *out_entry = de;
    *out_slot  = slot;
    return 0u;
}

static uint16_t fat_write_at(uint32_t slot, uint32_t offset, const uint8_t *data,
                             uint32_t len, uint32_t *out_written,
                             dir_entry_t *out_entry)
{
    if (g_vol == 0 || g_vol->dev == 0 || g_vol->dev->write_sectors == 0) {
        *out_written = 0u;
        return FILEIO_ERR_ACCESS_DENIED;
    }

    uint32_t wrote = 0u;
    int rc = fat12_write_partial(g_vol, g_fat, g_fat_len, slot, offset, data,
                                 len, g_sector, g_cluster, &wrote);
    if (rc != FAT12_OK) {
        *out_written = 0u;
        return FILEIO_ERR_ACCESS_DENIED;   /* disk full (rolled back) / write err */
    }
    *out_written = wrote;

    /* Re-read the (now-patched) dir entry at the same root-dir slot so the caller
     * refreshes the SFT copy's size + start_cluster (fat12_write_partial patched
     * them on disk). */
    {
        dir_entry_t de;
        int rc2 = fat12_read_dir_entry(g_vol, g_sector, slot, &de);
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

static uint16_t fat_unlink(const char *name83)
{
    if (g_vol == 0 || g_vol->dev == 0 || g_vol->dev->write_sectors == 0) {
        return FILEIO_ERR_ACCESS_DENIED;
    }
    {
        int rc = fat12_unlink(g_vol, g_fat, g_fat_len, name83, g_sector);
        if (rc == FAT12_ERR_NOT_FOUND) {
            return FILEIO_ERR_FILE_NOT_FOUND;
        }
        if (rc != FAT12_OK) {
            return FILEIO_ERR_ACCESS_DENIED;
        }
    }
    return 0u;
}

static const int21_file_backend_t g_fat_backend = {
    fat_open,
    fat_read_at,
    fat_dir_entry,
    fat_create,
    fat_write_at,
    fat_close,
    fat_unlink
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
