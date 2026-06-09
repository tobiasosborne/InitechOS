/* fileio_fat.c -- FAT12-backed INT 21h file backend (beads initech-509.5).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): shipped InitechDOS kernel code, freestanding
 * (only <stdint.h>). This is the concrete int21_file_backend_t the kernel binds
 * after a successful FAT12 mount: it bridges the host-testable INT 21h dispatch
 * logic (os/milton/int21.c) to the mounted volume (os/milton/fat12.c over
 * os/milton/ata.c). int21.c owns the SFT/JFT slots, the file offset, and the
 * DTA find-data write; THIS file owns the volume, the whole-file read into the
 * static file buffer, and root-directory enumeration. It is NOT host-testable
 * (it would pull fat12.c + the volume into the unit oracle); the host oracle
 * (test_fileio.c) binds an in-memory MOCK backend instead. The end-to-end FAT
 * read path is validated on the emulator (make test-type / make test-dir).
 *
 * Ref (Law 1): docs/research/fs-mount-sft-ground-truth.md Sec 4.1 (OPEN
 *   whole-file read), Sec 4.2 (READ), Sec 4.5 (FINDFIRST/NEXT dir enumeration),
 *   Sec 6 Step 4/5, Risk 2 (file buffer off the stack -> spec/memory_map.h);
 *   os/milton/fat12.h (fat12_find / fat12_read_file / fat12_read_root_dir /
 *   fat12_read_fat); spec/memory_map.h (FILE_BUFFER_BASE / FILE_BUFFER_MAX);
 *   spec/dos_structs.h (dir_entry_t). CLAUDE.md Law 2 (oracle), Law 3 (artifact
 *   = C), Rule 2 (fail loud + single-buffer limit), Rule 11 (deterministic),
 *   Rule 12 (ASCII).
 *
 * SINGLE-OPEN-FILE LIMIT (milestone scope, documented): exactly one file may be
 * open at a time -- the file buffer (FILE_BUFFER_BASE, 64 KiB cap) holds ONE
 * file's contents. fat_open() fails loud (TOO_MANY_OPEN) if the buffer is in
 * use, and (TOO_MANY_OPEN) if the file is larger than FILE_BUFFER_MAX.
 * fat_close() frees it. Multi-file-open + positioned partial-read are deferred
 * (a follow-up bead).
 */

#include "fileio_fat.h"
#include "fat12.h"
#include "int21.h"
#include "memory_map.h"   /* FILE_BUFFER_BASE / FILE_BUFFER_MAX */
#include "dos_structs.h"  /* dir_entry_t */

/* DOS error codes returned to int21.c (the same set int21.h uses). */
#define FILEIO_ERR_FILE_NOT_FOUND  0x0002u
#define FILEIO_ERR_TOO_MANY_OPEN   0x0004u

/* The mounted volume (caller-owned; bound by fileio_fat_bind). */
static const fat12_volume_t *g_vol = 0;

/* The whole-FAT buffer (1.44 MB FAT12: 9 sectors * 512 = 4608 bytes; round up
 * to a comfortable cap so a slightly larger FAT still fits). BSS, kernel-owned;
 * filled once at bind by fat12_read_fat. */
static uint8_t  g_fat[12u * 512u];
static uint32_t g_fat_len = 0;

/* Sector scratch for fat12_find / root-dir enumeration (one sector). */
static uint8_t  g_sector[BLOCKDEV_SECTOR_SIZE];

/* Cluster scratch for fat12_read_file (sectors_per_cluster * 512; 1 sector on
 * 1.44 MB). Sized to one cluster max for this geometry. */
static uint8_t  g_cluster[BLOCKDEV_SECTOR_SIZE];

/* The single open-file data buffer lives at FILE_BUFFER_BASE (spec/memory_map.h
 * Risk 2: off the kernel stack). g_buf_in_use enforces the single-file limit. */
static uint8_t  g_buf_in_use = 0;

/* ---- WRITE state (beads initech-509.11) ----------------------------------- *
 * CREAT claims the SAME single buffer (FILE_BUFFER_BASE) as the WRITE staging
 * area: AH=40h WRITE appends bytes here; CLOSE (flush) allocates the cluster
 * chain and writes them out. Mirrors the read single-buffer limit (Rule 2): one
 * file open for write at a time; > FILE_BUFFER_MAX bytes -> fail loud. */
static uint8_t  g_write_in_use   = 0;     /* a CREAT'd write file is open      */
static uint32_t g_write_len      = 0;     /* bytes appended so far             */
static uint32_t g_write_slot     = 0;     /* root-dir slot of the created file */

/* DOS error codes returned to int21.c for the write path. */
#define FILEIO_ERR_ACCESS_DENIED   0x0005u

/* ------------------------------------------------------------------------ *
 * Backend vtable implementations.
 * ------------------------------------------------------------------------ */

static uint16_t fat_open(const char *name83, dir_entry_t *out_entry,
                         const uint8_t **out_data, uint32_t *out_size)
{
    if (g_vol == 0) {
        return FILEIO_ERR_FILE_NOT_FOUND;   /* no volume -> as if absent */
    }
    if (g_buf_in_use) {
        /* Single-buffer limit (Rule 2): a second concurrent OPEN cannot be
         * served by the one file buffer. Fail loud rather than corrupt the
         * already-open file's data. */
        return FILEIO_ERR_TOO_MANY_OPEN;
    }

    dir_entry_t de;
    int rc = fat12_find(g_vol, g_sector, name83, &de);
    if (rc == FAT12_ERR_NOT_FOUND) {
        return FILEIO_ERR_FILE_NOT_FOUND;
    }
    if (rc != FAT12_OK) {
        return FILEIO_ERR_FILE_NOT_FOUND;   /* a read error reads as absent here */
    }

    if (de.file_size > FILE_BUFFER_MAX) {
        /* The file does not fit the single static buffer (milestone limit). */
        return FILEIO_ERR_TOO_MANY_OPEN;
    }

    uint8_t *buf = (uint8_t *)(uintptr_t)FILE_BUFFER_BASE;
    uint32_t got = 0;
    rc = fat12_read_file(g_vol, g_fat, g_fat_len, &de,
                         buf, FILE_BUFFER_MAX, g_cluster, &got);
    if (rc != FAT12_OK) {
        return FILEIO_ERR_FILE_NOT_FOUND;
    }

    g_buf_in_use = 1;
    *out_entry = de;          /* struct copy of the 32-byte dir entry */
    *out_data  = buf;
    *out_size  = got;         /* == de.file_size */
    return 0u;
}

static void fat_close(void)
{
    /* Release the single file buffer so a later OPEN may reuse it. */
    g_buf_in_use = 0;
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

/* ---- WRITE backend (beads initech-509.11) --------------------------------- *
 * CREAT: create/truncate the file on disk (size 0) and claim the write buffer.
 * WRITE: append bytes to the buffer (capped at FILE_BUFFER_MAX). FLUSH (CLOSE):
 * allocate the chain + write the data + FAT + patch the dir entry. UNLINK:
 * delete the file + free its chain. All go through the oracle-green fat12.c
 * write functions over the mounted volume. */

static uint16_t fat_create(const char *name83, dir_entry_t *out_entry)
{
    if (g_vol == 0 || g_vol->dev == 0 || g_vol->dev->write_sectors == 0) {
        return FILEIO_ERR_ACCESS_DENIED;   /* read-only / no volume */
    }
    if (g_buf_in_use || g_write_in_use) {
        /* Single-buffer limit (Rule 2): the one buffer is busy. */
        return FILEIO_ERR_TOO_MANY_OPEN;
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

    g_write_in_use = 1;
    g_write_len    = 0;
    g_write_slot   = slot;
    *out_entry     = de;
    return 0u;
}

static uint16_t fat_write(const uint8_t *data, uint32_t len, uint32_t *out_written)
{
    if (!g_write_in_use) {
        return FILEIO_ERR_ACCESS_DENIED;
    }
    /* Append into the single write buffer (FILE_BUFFER_BASE), capped (Rule 2:
     * never overrun the 64 KiB buffer). */
    if (g_write_len + len > FILE_BUFFER_MAX) {
        return FILEIO_ERR_ACCESS_DENIED;   /* file too large for the buffer */
    }
    {
        uint8_t *buf = (uint8_t *)(uintptr_t)FILE_BUFFER_BASE;
        uint32_t i;
        for (i = 0; i < len; i++) {
            buf[g_write_len + i] = data[i];
        }
    }
    g_write_len += len;
    *out_written = len;
    return 0u;
}

static uint16_t fat_flush(void)
{
    if (!g_write_in_use) {
        return 0u;   /* nothing open -> no-op success */
    }
    {
        const uint8_t *buf = (const uint8_t *)(uintptr_t)FILE_BUFFER_BASE;
        int rc = fat12_write_file(g_vol, g_fat, g_fat_len, g_write_slot,
                                  buf, g_write_len, g_sector, g_cluster);
        /* Release the buffer regardless of outcome so a failed write does not
         * wedge the single buffer (the caller surfaces the error to the app). */
        g_write_in_use = 0;
        g_write_len    = 0;
        if (rc != FAT12_OK) {
            return FILEIO_ERR_ACCESS_DENIED;   /* disk full / write error */
        }
    }
    return 0u;
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
    fat_close,
    fat_dir_entry,
    fat_create,
    fat_write,
    fat_flush,
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

    /* Cache the whole FAT once (read-only for the milestone). fat12_read_file
     * needs it for the cluster-chain walk. */
    int rc = fat12_read_fat(vol, g_fat, (uint32_t)sizeof(g_fat));
    if (rc != FAT12_OK) {
        return rc;   /* fail loud: a too-large FAT or read error surfaces */
    }
    g_fat_len    = (uint32_t)vol->bpb.sectors_per_fat *
                   (uint32_t)vol->bpb.bytes_per_sector;
    g_vol          = vol;
    g_buf_in_use   = 0;
    g_write_in_use = 0;
    g_write_len    = 0;

    int21_set_file_backend(&g_fat_backend);
    return 0;
}
