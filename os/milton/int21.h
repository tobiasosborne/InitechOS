/* int21.h -- InitechDOS INT 21h dispatcher (the `int 0x21` syscall spine).
 *
 * beads: initech-509.5 ("INT 21h dispatcher: full controlled register").
 *        Gate ratification: initech-1f9. CONSOLE subset only (no filesystem);
 *        file-handle/SFT functions (3Dh/3Eh/3Fh, find-first/next) are deferred
 *        to initech-509.3.
 * Ref:   docs/research/internals-int21h-ground-truth.md Sec 5 (flat 32-bit
 *        calling convention: AH-dispatch, EDX flat ptr, ECX count, EBX handle,
 *        EAX return, CF in saved EFLAGS), Sec 5.4 (the register frame + the
 *        carry-flag return mechanism), Sec 6 (the console-output first
 *        functions); spec/int21h_calling_convention.json (the LOCKED ABI + the
 *        per-function table); spec/int21h_register.json (the controlled scope --
 *        NO unlisted functions; ADR-0003 DEC-04 / DEC-13); spec/dos_messages.json
 *        (the controlled diagnostic catalogue). CLAUDE.md Law 1 (cite source),
 *        Law 2 (oracle is truth), Law 3 (artifact = C), Rule 2 (fail loud +
 *        controlled scope), Rule 8 (specs-as-data), Rule 11 (deterministic),
 *        Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only. The
 * SAME translation unit (int21.c) ALSO compiles HOSTED for the factory dispatch
 * oracle (os/milton/test_int21.c, reuses seed/test_assert.h). So the CON output
 * and the terminate action are routed through a SINK abstraction (a function
 * pointer the host test overrides) -- never a direct console/serial call from
 * the dispatch logic. The kernel binds the real sink (console + serial) once at
 * boot via int21_set_sink().
 */
#ifndef INITECH_INT21_H
#define INITECH_INT21_H

#include <stdint.h>
#include "idt.h"   /* int_frame_t -- the trap stub builds the SAME layout */

/* DOS error codes used by this subset (DOS 3.3 INT 21h error returns). */
#define INT21_ERR_INVALID_FUNCTION  0x0001u  /* unlisted/not-yet-impl AH      */
#define INT21_ERR_FILE_NOT_FOUND    0x0002u  /* OPEN: no such file            */
#define INT21_ERR_PATH_NOT_FOUND    0x0003u  /* OPEN: subdir/path (deferred)  */
#define INT21_ERR_TOO_MANY_OPEN     0x0004u  /* no free SFT/JFT slot / buffer busy */
#define INT21_ERR_ACCESS_DENIED     0x0005u  /* write to AUX/PRN/file (no backing yet) */
#define INT21_ERR_INVALID_HANDLE    0x0006u  /* out-of-range / closed handle  */
#define INT21_ERR_INSUFFICIENT_MEM  0x0008u  /* EXEC: no memory / load active (nested) */
#define INT21_ERR_INVALID_MEMORY    0x0009u  /* bad user buffer ptr: NULL / 32-bit wrap (ADR-0003 DEC-14) */
#define INT21_ERR_BAD_FORMAT        0x000Bu  /* EXEC: bad/oversized program image */
#define INT21_ERR_CURRENT_DIR       0x0010u  /* RMDIR: target is the current dir (DOS "attempt to remove current directory") */
#define INT21_ERR_NOT_SAME_DEVICE   0x0011u  /* RENAME: cross-directory pair (old_dir != new_dir); cross-dir MOVE deferred (beads initech-ycb3) */
#define INT21_ERR_NO_MORE_FILES     0x0012u  /* FINDFIRST/NEXT: no (more) match */
#define INT21_ERR_FILE_EXISTS       0x0050u  /* CREATNEW (5Bh): target already exists (DOS ERROR_FILE_EXISTS) */

/* Predefined device handles (no SFT yet -- the only handles this subset honors;
 * real JFT/SFT file handles arrive with beads initech-509.3). */
#define INT21_HANDLE_STDOUT  1u
#define INT21_HANDLE_STDERR  2u

/* ---- AH=44h AL=00h IOCTL Get-Device-Info: the device-information word (DX) ---
 * Ref: DOS 3.3 Programmer's Reference Manual, INT 21h Function 44h Subfunction
 * 00h (Get Device Information). DX is a bit-field whose meaning forks on bit 15
 * (ISDEV): a CHARACTER device (bit15=1) reports the device-class bits below; a
 * disk FILE (bit15=0) reports its drive number in bits 0-5 and a "not written"
 * flag in bit 6. spec/int21h_calling_convention.json AH=44h locks the two words
 * this milestone emits. (CLAUDE.md Law 1: every value cited; Rule 8: locked data.)
 *
 * The character-device bit names (PRM Fig. for Fn 44h/00h): */
#define INT21_DEVINFO_ISDEV    0x8000u  /* bit15: 1 = handle is a device       */
#define INT21_DEVINFO_ISCTL    0x4000u  /* bit14: device handles control strings */
#define INT21_DEVINFO_RAW      0x0020u  /* bit5 : 1 = binary (raw) mode         */
#define INT21_DEVINFO_ISCLK    0x0010u  /* bit4 : the CLOCK$ device (special)   */
#define INT21_DEVINFO_ISNUL    0x0008u  /* bit3 : the NUL device                */
#define INT21_DEVINFO_ISCOT    0x0004u  /* bit2 : 1 = console output (screen)   */
#define INT21_DEVINFO_ISCIN    0x0002u  /* bit1 : 1 = console input (keyboard)  */
#define INT21_DEVINFO_STDIN    0x0001u  /* bit0 : 1 = standard-input device     */

/* LOCKED words (spec/int21h_calling_convention.json AH=44h):
 *
 * CON, the console character device. The canonical real-DOS CON device-info word
 * is 0x80D3 = bits {15,7,6,4,1,0}: ISDEV(15) + reserved(7, set on the standard
 * devices) + bit6 (0 == EOF-on-input; 1 here == NOT at EOF, the live keyboard) +
 * special/CLOCK-class(4) + ISCIN(1, console input) + STDIN(0). Our CON SFT models
 * the single bidirectional console (keyboard in / screen out) the way real DOS
 * reports the CON handle, so the faithful value is the PRM CON word verbatim. */
#define INT21_DEVINFO_CON       0x80D3u

/* Disk FILE handle (bit15 clear). Bits 0-5 = drive number (0 == A:, our single
 * mounted volume); bit6 = 1 ("file has NOT been written to", the PRM default);
 * bit7 = 0. Our sft_entry_t carries no per-handle dirty bit, so bit6 is FIXED to
 * the PRM not-written default (1); per-handle written-state tracking is deferred
 * with the AL=01 set-info minor (beads initech-4nbn). Drive 0 -> 0x0000 low bits,
 * | bit6 -> 0x0040. */
#define INT21_DEVINFO_FILE_BIT_NOTWRITTEN  0x0040u  /* bit6: 1 = not written (PRM default) */
#define INT21_DEVINFO_FILE_DRIVE_A         0x0000u  /* drive 0 (A:) in bits 0-5 */
#define INT21_DEVINFO_FILE       (INT21_DEVINFO_FILE_DRIVE_A | INT21_DEVINFO_FILE_BIT_NOTWRITTEN)

/* InitechDOS version (3.30; ADR-0003 DEC-12 / spec/dos_banner.txt). GETVER
 * returns AL=major, AH=minor. 3.30 -> minor 30 = 0x1E. */
#define INT21_VER_MAJOR  3u
#define INT21_VER_MINOR  30u   /* 0x1E */

/* The CON SINK: every byte the dispatcher would "display" goes here. The kernel
 * binds a sink that fans out to the LFB console + COM1 serial; the host oracle
 * binds a sink that captures into a buffer. NULL (the default) discards bytes
 * (so the dispatch logic never faults if the sink is unbound). */
typedef void (*int21_sink_fn)(char c);

/* The TERMINATE hook: AH=4Ch / AH=00h call this with the return code. The kernel
 * binds a hook that emits the exit line then cli;hlt (terminate == stop, no
 * process model yet); the host oracle binds a hook that records the code +
 * returns (so the test can observe it without halting). NULL -> the dispatcher
 * still emits the diagnostic but simply returns (host-safe default). */
typedef void (*int21_exit_fn)(uint8_t code);

/* Bind the CON sink (NULL clears it). Called once by the kernel at boot. */
void int21_set_sink(int21_sink_fn sink);

/* Bind the terminate hook (NULL clears it). Called once by the kernel at boot. */
void int21_set_exit(int21_exit_fn fn);

/* Bind the CURRENT process's PSP (beads initech-509.3). The handle-based
 * functions (40h WRITE, 45h DUP, 46h DUP2, and the file functions to come)
 * resolve a handle through this PSP's Job File Table into the system SFT
 * (sft.h). The kernel binds a kernel PSP at SYSINIT (so kernel-context INT 21h
 * has valid standard handles); the loader rebinds to the loaded program's PSP
 * and the kernel restores its own on return. NULL clears it -> handle functions
 * then return invalid-handle. `struct psp` is forward-declared to keep int21.h
 * free of the spec header; int21.c includes sft.h (-> psp.h) for the full type. */
struct psp;
void int21_set_psp(struct psp *psp);

/* ---- CWD seam (beads initech-mzxa; ti8 Layer 2, READ side) ----------------
 * The current working directory the file/find functions resolve a RELATIVE path
 * from (AH=47h GET CURRENT DIR reports it). It is a file-static in int21.c (NOT
 * a psp_t field -- psp_t is LOCKED spec with sizeof==256). Until the CHDIR writer
 * (AH=3Bh, beads initech-u6wa) lands the CWD never leaves the root, but the
 * read-side plumbing is established now:
 *   int21_cwd_reset   : set the CWD to the root. The loader calls it on each
 *                       program launch (beside int21_mcb_reset); do_terminate
 *                       calls it on exit. So a freshly loaded program starts at
 *                       the root and a child's CWD never leaks on terminate.
 *   int21_cwd_save / int21_cwd_restore : snapshot + restore the CWD around a
 *                       kernel-context PSP rebind (kmain restores its own PSP
 *                       after a child run; the CWD is saved/restored in lockstep
 *                       so the child's directory never leaks into kernel INT 21h).
 * The snapshot is opaque (start cluster + the root-relative path text). */
#define INT21_CWD_MAX 64u
typedef struct int21_cwd_snapshot {
    uint16_t start_cluster;        /* 0 == the fixed root                       */
    char     path[INT21_CWD_MAX];  /* root-relative, no leading '\' / drive     */
} int21_cwd_snapshot_t;
void                 int21_cwd_reset(void);
int21_cwd_snapshot_t int21_cwd_save(void);
void                 int21_cwd_restore(const int21_cwd_snapshot_t *s);

/* ---- MEMORY ARENA SEAM (beads initech-509.6; AH=48h/49h/4Ah) --------------
 * The DOS memory functions (48h ALLOC / 49h FREE / 4Ah SETBLOCK) operate on an
 * MCB chain (os/milton/mcb.c) laid over a real program-memory region. The arena
 * itself (the buffer + its base LINEAR address, needed to convert a data
 * paragraph index to a DOS segment) lives in the KERNEL's flat memory map
 * (spec/memory_map.h: PROGRAM_BASE..PROGRAM_ALLOC_END), so the kernel binds it
 * through THIS seam -- the SAME pattern as the sink/file/exec seams -- and the
 * host oracle binds an in-memory arena. int21.c owns only the AH=48/49/4A
 * dispatch + the data-paragraph<->DOS-segment conversion at the syscall edge.
 *
 * AUTHENTIC MODEL (Law 1; spec/memory_map.h; DOS 3.3 PRM AH=48h/49h/4Ah): a
 * freshly-loaded .COM owns ONE big block -- its PSP+image+stack window, from
 * PROGRAM_BASE up to the allocation ceiling PROGRAM_ALLOC_END (the PSP's
 * alloc_end_seg). The loader rebinds the arena to one terminal block of the
 * program's whole window, owned by the program's PSP, on each load
 * (int21_mcb_reset). To get a heap a program SHRINKS its own block (4Ah
 * SETBLOCK) to what it needs, then 48h ALLOC carves from the freed tail. This
 * needs NO locked-spec change: the region is exactly [PROGRAM_BASE,
 * PROGRAM_ALLOC_END), already in spec/memory_map.h.
 *
 *   base        : pointer to the arena buffer (paragraph-aligned)
 *   total_paras : number of 16-byte paragraphs the arena spans
 *   base_linear : the arena base as a flat LINEAR address; the DOS segment a
 *                 program sees is (base_linear >> 4) + data_para.
 *
 * A NULL/zero-total arena (the unbound default) makes 48h/49h/4Ah return CF=1,
 * AX=0008h (insufficient memory) -- never a fault (Rule 2). */
void int21_set_mcb_arena(void *base, uint32_t total_paras, uint32_t base_linear);

/* Lay a single terminal FREE-or-owned block over the bound arena and (re)assign
 * its owner to the CURRENT PSP's DOS segment. The loader calls this on each
 * program load so every program starts owning its whole window (the authentic
 * single-big-block). With no arena bound this is a no-op. Returns 1 if an arena
 * was (re)initialized, 0 if none is bound. */
int int21_mcb_reset(void);

/* ---- File backend (beads initech-0qh; epic initech-6qy) --------------------
 * The file-handle functions (3Dh OPEN, 3Fh READ/40h WRITE on a FILE, 4Eh/4Fh
 * FINDFIRST/FINDNEXT) need the mounted FAT12 volume -- which lives in the
 * kernel (ata.c + fat12.c), is NOT host-testable, and must NOT be linked into
 * the int21 unit oracle. So the FAT-specific work is reached through a backend
 * vtable the kernel binds (a FAT12-backed impl in os/milton/fileio_fat.c) and
 * the host oracle binds to an in-memory mock. int21.c owns the SFT/JFT slot
 * management, the per-handle file offset, and the DTA/find-data write; the
 * backend owns the volume, the POSITIONED cluster-chain read/write, and dir
 * enumeration. This mirrors the sink/exit-hook seam that keeps int21.c
 * host-testable.
 *
 * POSITIONED + STATELESS (the multi-tenant redesign, beads initech-0qh):
 * the backend no longer owns a single whole-file buffer. Each SFT slot carries
 * its OWN position (sft_entry.file_offset) and dir_entry copy (start_cluster +
 * size) + the dir-entry slot AND its containing directory's start cluster for
 * write-back (sft_entry.root_slot + sft_entry.dir_start; the latter is 0 for a
 * root file and the subdir's first cluster for a subdir file -- beads
 * initech-zs24). open() merely LOCATES the file (no whole-file read);
 * read_at()/write_at() are positioned over the cluster chain via
 * fat12_read_partial / fat12_write_partial. So N
 * files open at once -- each its own SFT slot -- works for free; a >64 KiB file
 * is served slice-by-slice with no whole-file buffer. The dispatch is
 * cooperative + single-threaded (sequential INT 21h calls), so ONE shared
 * cluster scratch in the backend is safe between calls; per-call reentrancy
 * hardening is beads initech-xk2 (out of scope here).
 *
 * `struct dir_entry` is forward-declared (the full type is spec/dos_structs.h,
 * which int21.c pulls in via sft.h -> psp.h).
 *
 * SUBDIRECTORY RESOLUTION (beads initech-mzxa; ti8 Layer 2). open()/create()/
 * dir_entry()/unlink() take a `dir_start_cluster` -- the start cluster of the
 * directory the bare 8.3 `name83` (or the enumeration `index`) lives in, with
 * 0 == the fixed root directory. The dispatcher resolves a '\SUB\FILE'-style
 * path through the resolve() seam BELOW to that containing-directory cluster
 * BEFORE calling these, and passes the bare leaf name. dir_start_cluster==0
 * (the only value the root-only milestone ever produced) reproduces the prior
 * root-relative behavior BYTE-IDENTICALLY, so every existing caller/oracle stays
 * green. Ref: os/milton/fat12.h (fat12_resolve_path / fat12_read_dir, Layer 1);
 * brief Sec 4.1/4.5; DOS 3.3 PRM AH=3Dh/3Ch/41h/4Eh. */
struct dir_entry;

typedef struct int21_file_backend {
    /* OPEN: LOCATE the 8.3 file `name83` in the directory whose first data
     * cluster is `dir_start_cluster` (0 == the fixed root; fat12_find_slot over
     * fat12_read_dir) -- NO whole-file read. Returns a copy of its 32-byte
     * directory entry (with start_cluster + size) in *out_entry and its 0-based
     * dir slot index in *out_slot (for a later write-back). Returns 0 on success
     * or a DOS error (0x0002 not found). out_entry/out_slot are written only on
     * success. The dispatcher stores these in the SFT slot; the position lives in
     * the slot, so any number of files may be open concurrently. */
    uint16_t (*open)(const char *name83, uint16_t dir_start_cluster,
                     struct dir_entry *out_entry, uint32_t *out_slot);

    /* READ_AT: POSITIONED read of up to `len` bytes starting at byte `offset`
     * within the file described by dir entry `e` (a per-handle copy), via
     * fat12_read_partial -- walking ONLY the clusters it needs (no whole-file
     * buffer). Sets *out_read to the bytes copied (0 at/after EOF is a clean
     * success, not an error). Returns 0 on success or a DOS error. */
    uint16_t (*read_at)(const struct dir_entry *e, uint32_t offset,
                        uint8_t *buf, uint32_t len, uint32_t *out_read);

    /* Directory enumeration for FINDFIRST/FINDNEXT: copy the directory entry at
     * 0-based `index` WITHIN the directory whose first data cluster is
     * `dir_start_cluster` (0 == the fixed root) into `*out_entry` and set
     * `*out_found` = 1; at/after the end of the directory set `*out_found` = 0.
     * Returns 0 on success, non-zero (a DOS error) on a backend read failure.
     * Deleted/LFN slots are skipped by the backend so consecutive indices map to
     * surviving 8.3 entries. */
    uint16_t (*dir_entry)(uint32_t index, uint16_t dir_start_cluster,
                          struct dir_entry *out_entry, int *out_found);

    /* ---- WRITE path (beads initech-0qh, positioned) ----------------------- *
     * CREATE (AH=3Ch): create or TRUNCATE the 8.3 file `name83` in the directory
     * whose first data cluster is `dir_start_cluster` (0 == the fixed root) and
     * return a copy of its (zeroed-size) dir entry in *out_entry and its dir
     * slot index in *out_slot (so write_at()/close() can patch size/start_cluster
     * in place). Returns 0 on success or a DOS error (0x0004 dir full, 0x0003
     * path, 0x0005 read-only/no volume). May be NULL on a read-only backend (the
     * host read oracle binds NULL -> CREAT returns access-denied). */
    uint16_t (*create)(const char *name83, uint16_t dir_start_cluster,
                        struct dir_entry *out_entry, uint32_t *out_slot);

    /* WRITE_AT (AH=40h to a FILE): POSITIONED write of `len` bytes of `data`
     * starting at byte `offset` within the file whose dir entry is at slot `slot`
     * of the directory whose first data cluster is `dir_start` (0 == the fixed
     * root; beads initech-zs24 -- `slot` is the root-dir-region index for the
     * root, the linear cluster-chain index for a subdir), via fat12_write_partial
     * -- overwrite-in-place / extend / zero-fill-hole, with both-FAT sync and
     * allocate-then-commit disk-full rollback. Returns the UPDATED dir entry (new
     * size / start_cluster) in *out_entry so the caller refreshes its SFT copy.
     * Sets *out_written (== len on success; 0 on a disk-full rollback). Returns 0
     * on success or a DOS error (0x0005 disk full / write error). The bytes are
     * committed to disk by THIS call (no deferred flush). May be NULL on a
     * read-only backend. dir_start==0 keeps the root write-back byte-identical. */
    uint16_t (*write_at)(uint16_t dir_start, uint32_t slot, uint32_t offset,
                         const uint8_t *data, uint32_t len,
                         uint32_t *out_written, struct dir_entry *out_entry);

    /* CLOSE: finalize a handle at root-dir slot `slot`. With per-call flushing
     * in write_at() this is a no-op; the hook is kept for symmetry (and a future
     * deferred-buffering write model). Called by 3Eh CLOSE when the last
     * reference to a FILE slot drops. May be NULL. */
    void (*close)(uint32_t slot);

    /* UNLINK (AH=41h DELETE): delete the 8.3 file `name83` in the directory whose
     * first data cluster is `dir_start_cluster` (0 == the fixed root; mark
     * deleted + free its chain). Returns 0 on success or 0x0002 (not found) /
     * 0x0005 (error). */
    uint16_t (*unlink)(const char *name83, uint16_t dir_start_cluster);

    /* FREESPACE (AH=36h GET DISK FREE SPACE): report the mounted volume's
     * geometry + free-cluster count. On success returns 0 and fills:
     *   *out_spc        = sectors per cluster
     *   *out_bps        = bytes per sector
     *   *out_total_clus = total data-area clusters
     *   *out_free_clus  = free clusters (counted from the cached FAT)
     * Returns non-zero (or is NULL) when no volume is mounted -> AH=36h reports
     * AX=0xFFFF (invalid drive). May be NULL on a backend with no volume. */
    uint16_t (*freespace)(uint16_t *out_spc, uint16_t *out_bps,
                          uint16_t *out_total_clus, uint16_t *out_free_clus);

    /* RESOLVE (beads initech-mzxa; ti8 Layer 2 -- the thin path->directory seam):
     * resolve the CONTAINING directory of the '\SUB\FILE'-style `path` to its
     * first data cluster (0 == the fixed root), and point `*out_leaf` at the bare
     * final 8.3 component WITHIN `path` (the name OPEN/CREAT/UNLINK locate, or the
     * search template FINDFIRST builds). `cwd_start` is the start cluster of the
     * current working directory (0 == root) a RELATIVE path (no leading '\', no
     * 'X:' prefix) is resolved from; an ABSOLUTE or drive-prefixed path resolves
     * from the root regardless. The backend owns ALL backslash/drive parsing (it
     * already has fat12_resolve_path's parser) so int21.c never includes fat12.h.
     *
     * Returns 0 on success with *out_dir_start + *out_leaf set, or
     * INT21_ERR_PATH_NOT_FOUND (0x0003) when a non-final component is missing or
     * is not a directory (the DOS path-not-found contract -- preserved at every
     * rejection site, and mapped by AH=59h GET EXTENDED ERROR). A NULL resolve
     * member means "root-only" (the dispatcher treats any subdir/drive path as
     * 0x0003 and a bare name as dir_start_cluster 0 -- the pre-mzxa behavior). */
    uint16_t (*resolve)(const char *path, uint16_t cwd_start,
                        const char **out_leaf, uint16_t *out_dir_start);

    /* RESOLVE_DIR (beads initech-u6wa; AH=3Bh CHDIR -- the path->DIRECTORY seam):
     * resolve a FULL `path` to a DIRECTORY (not a containing parent like resolve()
     * above, which splits off a file leaf). On success returns 0 with:
     *   *out_dir_start = the TARGET directory's own first data cluster (0 == the
     *                    fixed root, after the start_cluster==0 => root normalize);
     *   out_canon      = the canonical ROOT-RELATIVE path text of that directory
     *                    (uppercase 8.3 components, single '\' joins, NO leading
     *                    '\', NO drive, root == the empty string), NUL-terminated
     *                    and bounded by canon_max (the backend derives it from the
     *                    filesystem structure via a reverse '..' walk, so a
     *                    RELATIVE / '.' / '..' path canonicalizes identically to
     *                    the equivalent absolute one).
     * `cwd_start` seeds a RELATIVE path's descent (no leading '\', no 'X:'); an
     * ABSOLUTE or drive-prefixed path descends from the root regardless. Returns
     * INT21_ERR_PATH_NOT_FOUND (0x0003) when a component is missing OR the final
     * component is not a directory (CHDIR into a file -- the DOS path-not-found
     * contract). A NULL resolve_dir member means "root-only": the dispatcher
     * treats any non-empty/non-root path as 0x0003 (the pre-u6wa behavior). */
    uint16_t (*resolve_dir)(const char *path, uint16_t cwd_start,
                            uint16_t *out_dir_start, char *out_canon,
                            uint32_t canon_max);

    /* MKDIR (beads initech-u6wa; AH=39h CREATE DIRECTORY): create the new
     * subdirectory `name83` in the directory whose first data cluster is
     * `dir_start_cluster` (0 == the fixed root; a NON-ROOT value is OUT OF SCOPE
     * this landing -> 0x0003 PATH_NOT_FOUND, mirroring create()/unlink()). On
     * success returns 0; an existing name -> 0x0005 (DOS MKDIR-exists); no write
     * backend / dir full / full volume -> 0x0005. May be NULL on a read-only
     * backend (the host read oracle binds NULL -> MKDIR returns access-denied). */
    uint16_t (*mkdir)(const char *name83, uint16_t dir_start_cluster);

    /* RMDIR (beads initech-u6wa; AH=3Ah REMOVE DIRECTORY): remove the EMPTY
     * subdirectory `name83` from the directory whose first data cluster is
     * `dir_start_cluster` (0 == the fixed root; a NON-ROOT value is OUT OF SCOPE
     * -> 0x0003). On success returns 0; a missing dir / not-a-directory ->
     * 0x0003; a NON-EMPTY directory -> 0x0005 (DOS RMDIR-non-empty). The
     * current-directory and root-reject checks are the dispatcher's (it owns the
     * CWD state); this seam only resolves + removes. May be NULL on a read-only
     * backend. */
    uint16_t (*rmdir)(const char *name83, uint16_t dir_start_cluster);

    /* SET_TIME (beads initech-qekc; AH=57h AL=01h SET FILE DATE/TIME by handle):
     * patch the packed DOS modification time/date of the directory entry at slot
     * `slot` of the directory whose first data cluster is `dir_start` (0 == the
     * fixed root) to `mtime`/`mdate` and FLUSH IMMEDIATELY -- parity with the
     * per-call write-commit model (write_at commits each call; AH=57h SET commits
     * here, NOT deferred to CLOSE). `mtime`/`mdate` are the SAME packed words
     * dir_entry_t.mtime(0x16)/.mdate(0x18) store on disk, so the dispatcher copies
     * CX->mtime, DX->mdate VERBATIM (no encode/decode). On success returns 0; a
     * read/write error -> 0x0005 (access denied). A NULL set_time member means a
     * READ-ONLY backend: AH=57h SET then returns CF=1, AX=0x0005 (Rule 2 fail
     * loud -- never a silent no-op). GET (AL=00) needs NO seam (it reads the SFT's
     * in-memory dir_entry copy directly). Ref: DOS 3.3 PRM AH=57h; spec/
     * dos_structs.h dir_entry_t (mtime 0x16 / mdate 0x18). */
    uint16_t (*set_time)(uint16_t dir_start, uint32_t slot,
                         uint16_t mtime, uint16_t mdate);

    /* CHMOD (beads initech-b53d; AH=43h GET/SET FILE ATTRIBUTES, path-based):
     * GET (set==0) reads the attribute byte of the 8.3 file `name83` in the
     * directory whose first data cluster is `dir_start_cluster` (0 == the fixed
     * root) into *io_attr; SET (set==1) writes *io_attr as the new attribute
     * byte and FLUSHES (immediate commit -- the per-call write model). On disk
     * the SET patches ONLY the attribute byte (0x0B); mtime(0x16)/mdate(0x18)/
     * name/cluster/size are preserved VERBATIM (Rule 11: a CHMOD never disturbs
     * the timestamp bytes). Returns 0 on success, 0x0002 (not found), or 0x0005
     * (access denied: a directory/volume-label TARGET, a SET attr that itself
     * sets the Directory(0x10)/VolLabel(0x08) bit, or a SET with no write
     * backend). The dispatcher rejects an out-of-set AL and a directory/vol-label
     * CX BEFORE this seam; the backend ALSO rejects them (defense in depth -- the
     * fat12 primitive is the canonical guard). A NULL chmod member means a
     * READ-ONLY backend with NO attribute store: the dispatcher then returns
     * 0x0005 for a SET and 0x0005 for a GET (Rule 2 fail loud -- never a silent
     * no-op). Ref: DOS 3.3 PRM AH=43h; spec/dos_structs.h (attribute 0x0B). */
    uint16_t (*chmod)(const char *name83, uint16_t dir_start_cluster,
                      int set, uint8_t *io_attr);

    /* RENAME (beads initech-gnrc; AH=56h RENAME, SAME-directory dir-entry rename):
     * rename the 8.3 file `old83` in the directory whose first data cluster is
     * `old_dir` (0 == the fixed root) to `new83` in the directory `new_dir`. The
     * dispatcher resolves BOTH paths (EDX old, EDI new) to their containing
     * directories + bare leaves through the resolve seam and forwards the pair;
     * the cross-directory case (old_dir != new_dir) is rejected by the DISPATCHER
     * (0x0011 NOT_SAME_DEVICE) BEFORE this seam is consulted, so a backend MAY
     * assume old_dir == new_dir, but it is passed both for symmetry + a future
     * cross-dir MOVE (beads initech-ycb3). The backend scans the source in
     * `old_dir`; rejects a missing source (0x0002), a directory/volume-label
     * source (0x0005 -- our backend has no '..' fixup path yet), or a dest name
     * already present in `new_dir` (0x0005, the load-bearing reject); then
     * rewrites ONLY the matched entry's 11-byte name field (filename[0..7] +
     * extension[0..2]) in place -- start_cluster/file_size/attribute/mtime/mdate
     * and the FAT are left UNTOUCHED (rename allocates/frees nothing; Rule 11).
     * Returns 0 on success or a DOS error (0x0002 not found / 0x0005 dest-exists,
     * dir/vol-label source, or write error). May be NULL on a read-only backend
     * (the host read oracle binds NULL -> RENAME returns access-denied). Ref: DOS
     * 3.3 PRM AH=56h; spec/dos_structs.h (filename 0x00 / extension 0x08). */
    uint16_t (*rename)(const char *old83, uint16_t old_dir,
                       const char *new83, uint16_t new_dir);
} int21_file_backend_t;

/* Bind the file backend (NULL clears it -> the file functions return
 * file-not-found / no-more-files as if the volume were empty). The kernel binds
 * the FAT12 backend after a successful mount; the host oracle binds a mock. */
void int21_set_file_backend(const int21_file_backend_t *backend);

/* ---- EXEC backend (beads initech-saw + initech-509.5 AH=4Bh) --------------
 * AH=4Bh EXEC (load-and-execute a child program by name) needs the FAT-sourced
 * loader (os/milton/loader.c load_program_from_fat), which lives in the kernel
 * (it pulls fat12.c + the volume + the asm control transfer) and is NOT
 * host-testable. So the dispatcher reaches the actual load+run through THIS seam
 * -- a function pointer the kernel binds to the saw path and the host oracle
 * binds to a mock -- exactly mirroring the file-backend / sink / conin seams so
 * int21.c stays free of loader.c and compiles HOSTED.
 *
 * The callback loads the flat .COM `name83` (the BARE 8.3 leaf) from the
 * directory whose first data cluster is `dir_start` (0 == the fixed root) and
 * RUNS it to completion (the child terminates via 4Ch / INT 20h and the loader
 * regains control). On a clean run it returns 0 and writes the child's exit code
 * to *out_rc; on a failure it returns a DOS error code (0x0002 file not found,
 * 0x0008 insufficient memory / load already active -- nested EXEC, 0x000B bad
 * format / too large) and leaves *out_rc unchanged.
 *
 * SUBDIR EXEC (beads initech-zs24, Landing 2): do_exec resolves the EDX path to
 * a (containing-directory `dir_start`, bare leaf `name83`) pair through the SAME
 * resolve seam OPEN/CREAT/UNLINK use (resolve_dir_path -> the file backend's
 * resolve()), so EXEC honors absolute, CWD-relative, and '\SUB\FILE' paths
 * identically to OPEN -- no second path grammar. The backend then locates the
 * leaf in `dir_start` (the loader's fat12_find_slot_in; dir_start==0 keeps the
 * historical root find byte-identical) and feeds the located entry's own cluster
 * chain to load_program. `name83` is already the validated bare leaf (no '\'/':').
 *
 * `cmd_tail`/`cmd_tail_len` carry the command-tail TEXT (no count byte, no CR)
 * the loader writes to the child PSP:80h (beads initech-456); cmd_tail may be
 * NULL with cmd_tail_len 0 for a no-argument launch. The signature matches
 * loader.c load_program_from_fat so the kernel can bind it through one adapter. */
typedef uint16_t (*int21_exec_fn)(const char *name83, uint16_t dir_start,
                                  const char *cmd_tail,
                                  uint32_t cmd_tail_len, uint8_t *out_rc);

/* Bind the EXEC backend (NULL clears it -> AH=4Bh returns file-not-found, as if
 * no program could be loaded). The kernel binds the saw-path loader; the host
 * oracle binds a mock. */
void int21_set_exec_backend(int21_exec_fn fn);

/* ---- CON INPUT SOURCE (beads initech-n62) ---------------------------------
 * The CON-input functions (AH=01h/06h/07h/08h/0Ah/0Bh/0Ch) read keystrokes
 * through an INPUT SOURCE abstraction that mirrors the sink/exit/file-backend
 * seams: the kernel binds it to the live PS/2 keyboard (kbd.c), and the host
 * oracle binds it to a queued mock string -- so int21.c stays free of any I/O,
 * hlt, or hardware dependency and compiles HOSTED in the unit oracle.
 *
 * Two callbacks (both report a char as 0..255):
 *   get  -- BLOCKING: return the next character; it MUST NOT return -1 (the
 *           kernel impl loops on `hlt` until a key arrives; the host mock
 *           dequeues from the test string and aborts the test on underflow so a
 *           test can never hang). Used by 01h/07h/08h and 0Ah's read loop.
 *   poll -- NON-blocking: return the next character (0..255) WITHOUT removing it
 *           from the producer's notion of "blocking" -- i.e. it consumes one
 *           char if present, else returns -1 (no char). Used by 06h (DL=FF),
 *           0Bh status, and 0Ch's flush drain.
 *
 * A NULL source (the unbound default) makes every input function return
 * gracefully: blocking get yields 0 (treated as no input / EOF, never a hang),
 * poll yields -1 (no char). Ref: DOS 3.3 Programmer's Reference Manual
 * AH=01h/06h/07h/08h/0Ah/0Bh/0Ch. */
typedef int (*int21_conin_fn)(void);      /* BLOCKING: next char 0..255         */
typedef int (*int21_coninpoll_fn)(void);  /* NON-blocking: char 0..255, or -1   */

/* Bind the CON input source (either may be NULL to clear). The kernel binds
 * keyboard-backed callbacks near the sti; the host oracle binds a queued mock. */
void int21_set_conin(int21_conin_fn get, int21_coninpoll_fn poll);

/* ---- CLOCK SOURCE (beads initech-yv9) -------------------------------------
 * GET/SET DATE+TIME (AH=2Ah/2Bh/2Ch/2Dh) read and write the wall clock through
 * a CLOCK seam -- the same pattern as the sink/conin/file seams -- so int21.c
 * never touches CMOS ports (0x70/0x71) directly and compiles HOSTED. The kernel
 * binds the live MC146818 RTC (os/milton/rtc.c rtc_now/rtc_set); the host oracle
 * binds a fixed mock. The seam exchanges a flat 7-field date/time so int21.c
 * does not need rtc.h's struct layout.
 *
 *   get : write the current y/mo/d/h/mi/s + dow (0=Sun) into the OUT params;
 *         return 1 on success, 0 if no clock is bound / read failed.
 *   set : set the clock from the IN params (full 4-digit year). `which` is a
 *         bitmask: bit0 = set date, bit1 = set time (so SET DATE does not clobber
 *         the time and vice versa). Return 1 on success, 0 on invalid/no clock.
 * A NULL clock (unbound default) makes AH=2Ah/2Ch return a fixed epoch sentinel
 * (1980-01-01 00:00:00, the DOS file-time epoch) and AH=2Bh/2Dh report failure. */
typedef int (*int21_clock_get_fn)(uint16_t *year, uint8_t *month, uint8_t *day,
                                  uint8_t *hour, uint8_t *minute, uint8_t *second,
                                  uint8_t *dow);
typedef int (*int21_clock_set_fn)(uint16_t year, uint8_t month, uint8_t day,
                                  uint8_t hour, uint8_t minute, uint8_t second,
                                  uint8_t which);
#define INT21_CLOCK_SET_DATE 0x01u
#define INT21_CLOCK_SET_TIME 0x02u

/* Bind the clock source (either may be NULL to clear). The kernel binds RTC-
 * backed callbacks at boot; the host oracle binds a fixed mock. */
void int21_set_clock(int21_clock_get_fn get, int21_clock_set_fn set);

/* ---- INTERRUPT-VECTOR TABLE SEAM (beads initech-509.8) --------------------
 * AH=25h SETVECT / AH=35h GETVECT read and write the live interrupt vector for
 * a given vector number. On this 32-bit protected-flat kernel that means the
 * IDT gate offset (idt_set_gate / idt_get_gate, idt.c). int21.c reaches the IDT
 * through a seam -- the SAME pattern as the sink/conin/clock seams -- so it does
 * NOT link idt.c and stays HOSTED-clean (Law 3): the host oracle binds a mock
 * vector table; the kernel binds idt-backed callbacks at SYSINIT.
 *   set : install `handler` (a flat linear address) as the vector for `vec`,
 *         keeping the kernel code selector + 0x8F TRAP type (real DOS rewrites
 *         the whole IVT entry; here we rewrite the gate offset).
 *   get : return the current flat handler offset for `vec` (0 if none/unbound).
 * A NULL table (unbound default) makes SETVECT a graceful no-op and GETVECT
 * return 0 -- never a fault (Rule 2). Ref: spec/int21h_register.json (25h/35h
 * SETVECT/GETVECT, Core class); spec/int21h_calling_convention.json (flat ABI:
 * pointer args are flat 32-bit linear addresses). */
typedef void     (*int21_setvect_fn)(uint8_t vec, uint32_t handler);
typedef uint32_t (*int21_getvect_fn)(uint8_t vec);
void int21_set_vectortable(int21_setvect_fn set, int21_getvect_fn get);

/* ---- DOS termination / control-break / critical-error handlers (DEC-10) ----
 * beads initech-509.8. The asm trap stubs (int22_entry / int23_entry /
 * int24_entry, isr.asm) call these with a pointer to the on-stack int_frame_t,
 * exactly like int21_dispatch / int20_dispatch.
 *
 *   int22_dispatch : INT 22h = the DOS program-TERMINATE return address (DOS
 *     PRM). In the single-level process model the default handler terminates the
 *     current program through the bound exit hook with code 0 (the SAME path as
 *     INT 20h / 4Ch AL=0). Normally non-returning.
 *   int23_dispatch : INT 23h = the control-BREAK handler. The DOS default action
 *     ABORTS the program, so this routes to the SAME terminate path as 22h (the
 *     break-abort). (No keyboard ^C wiring yet -- beads initech-4tw.)
 *   int24_dispatch : INT 24h = the CRITICAL-ERROR handler. Presents MSG-DOS-0001
 *     ("Abort, Retry, Fail?") to CON, reads ONE operator key, decides the action
 *     via crit_error_action (below), writes the action into AL, clears CF, and
 *     RETURNS to the failed caller (DOS contract -- 24h does NOT terminate).
 * Ref: docs/adr/ADR-0003 Sec 5.10 (DEC-10); App C (MSG-DOS-0001). */
void int22_dispatch(int_frame_t *frame);
void int23_dispatch(int_frame_t *frame);
void int24_dispatch(int_frame_t *frame);

/* PURE, host-testable INT 24h Abort/Retry/Fail decision. Maps an operator key to
 * the DOS AL action code: 'R'->0 (Retry), 'A'->1 (Abort), 'F'->2 (Fail), case-
 * insensitive. Any other key -> -1 ("re-prompt": int24_dispatch loops, re-reads,
 * and re-presents MSG-DOS-0001 until a valid A/R/F key arrives). Ignore=3 is the
 * fourth DOS action and is DEFERRED (no Ignore prompt this milestone). No asm/IO
 * -- a SEAM the host oracle drives directly. Ref: DOS 3.3 PRM INT 24h (AL return
 * 0=Ignore/1=Retry/2=Abort historically varies by DOS revision; we use the
 * documented modern A/R/F mapping pinned here). */
int crit_error_action(int ch);

/* Record the most recent DOS error code so AH=59h GET EXTENDED ERROR can report
 * it (beads initech-yv9). The error-returning functions (OPEN/UNLINK/etc.) call
 * this with the DOS error they return; AH=59h derives the class/action/locus.
 * Exposed (non-static) only so the host oracle can seed/inspect it. */
void int21_note_error(uint16_t code);

/* The InDOS depth predicate (beads initech-xk2). Returns 1 while one or more
 * INT 21h calls are in flight (g_indos != 0), else 0. This is the period-
 * authentic DOS InDOS-flag contract: a future ISR/TSR/driver MUST poll
 * dos_in_dos() before issuing its OWN INT 21h call and DEFER while it is set,
 * because DOS INT 21h is not reentrant. (Distinct from the stricter in-IRQ guard
 * in irq.h, which FAILS LOUD if an ISR enters the dispatcher at all.) Nothing
 * checks this yet -- it is the documented hook + the counter. Ref: irq.h; DOS
 * 3.3 internals (InDOS flag). */
int dos_in_dos(void);

/* The C dispatch routine the asm trap stub (int21_entry, isr.asm) invokes with a
 * pointer to the on-stack int_frame_t. Reads AH = (frame->eax >> 8) & 0xFF and
 * switches per spec/int21h_register.json. Writes the return value into
 * frame->eax and sets/clears CF (bit 0 of frame->eflags) before returning; the
 * stub's iretd then restores the modified EFLAGS so the caller sees CF. An AH
 * not in the locked register emits a diagnostic and returns CF=1/AX=0x0001 --
 * NEVER a silent no-op (Rule 2 / DEC-13). */
void int21_dispatch(int_frame_t *frame);

/* The INT 20h legacy-terminate handler the asm trap stub (int20_entry, isr.asm)
 * invokes. Routes to the SAME terminate path as INT 21h AH=4Ch with exit code 0
 * (DOS legacy termination; ADR-0003 DEC-10, beads initech-509.5). Ref:
 * docs/research/psp-loader-ground-truth.md Sec 2.1 / Sec 4.4. */
void int20_dispatch(int_frame_t *frame);

#endif /* INITECH_INT21_H */
