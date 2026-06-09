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
#define INT21_ERR_BAD_FORMAT        0x000Bu  /* EXEC: bad/oversized program image */
#define INT21_ERR_NO_MORE_FILES     0x0012u  /* FINDFIRST/NEXT: no (more) match */

/* Predefined device handles (no SFT yet -- the only handles this subset honors;
 * real JFT/SFT file handles arrive with beads initech-509.3). */
#define INT21_HANDLE_STDOUT  1u
#define INT21_HANDLE_STDERR  2u

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
 * size) + the root-dir slot for write-back (sft_entry.root_slot). open() merely
 * LOCATES the file (no whole-file read); read_at()/write_at() are positioned
 * over the cluster chain via fat12_read_partial / fat12_write_partial. So N
 * files open at once -- each its own SFT slot -- works for free; a >64 KiB file
 * is served slice-by-slice with no whole-file buffer. The dispatch is
 * cooperative + single-threaded (sequential INT 21h calls), so ONE shared
 * cluster scratch in the backend is safe between calls; per-call reentrancy
 * hardening is beads initech-xk2 (out of scope here).
 *
 * `struct dir_entry` is forward-declared (the full type is spec/dos_structs.h,
 * which int21.c pulls in via sft.h -> psp.h). */
struct dir_entry;

typedef struct int21_file_backend {
    /* OPEN: LOCATE the 8.3 file `name83` in the (root) directory (fat12_find) --
     * NO whole-file read. Returns a copy of its 32-byte directory entry (with
     * start_cluster + size) in *out_entry and its 0-based root-dir slot index in
     * *out_slot (for a later write-back). Returns 0 on success or a DOS error
     * (0x0002 not found). out_entry/out_slot are written only on success. The
     * dispatcher stores these in the SFT slot; the position lives in the slot,
     * so any number of files may be open concurrently. */
    uint16_t (*open)(const char *name83, struct dir_entry *out_entry,
                     uint32_t *out_slot);

    /* READ_AT: POSITIONED read of up to `len` bytes starting at byte `offset`
     * within the file described by dir entry `e` (a per-handle copy), via
     * fat12_read_partial -- walking ONLY the clusters it needs (no whole-file
     * buffer). Sets *out_read to the bytes copied (0 at/after EOF is a clean
     * success, not an error). Returns 0 on success or a DOS error. */
    uint16_t (*read_at)(const struct dir_entry *e, uint32_t offset,
                        uint8_t *buf, uint32_t len, uint32_t *out_read);

    /* Directory enumeration for FINDFIRST/FINDNEXT: copy the directory entry at
     * 0-based `index` into `*out_entry` and set `*out_found` = 1; at/after the
     * end of the directory set `*out_found` = 0. Returns 0 on success, non-zero
     * (a DOS error) on a backend read failure. Deleted/LFN slots are skipped by
     * the backend so consecutive indices map to surviving 8.3 entries. */
    uint16_t (*dir_entry)(uint32_t index, struct dir_entry *out_entry,
                          int *out_found);

    /* ---- WRITE path (beads initech-0qh, positioned) ----------------------- *
     * CREATE (AH=3Ch): create or TRUNCATE the 8.3 file `name83` in the (root)
     * directory and return a copy of its (zeroed-size) dir entry in *out_entry
     * and its root-dir slot index in *out_slot (so write_at()/close() can patch
     * size/start_cluster in place). Returns 0 on success or a DOS error (0x0004
     * dir full, 0x0003 path, 0x0005 read-only/no volume). May be NULL on a
     * read-only backend (the host read oracle binds NULL -> CREAT returns
     * access-denied). */
    uint16_t (*create)(const char *name83, struct dir_entry *out_entry,
                        uint32_t *out_slot);

    /* WRITE_AT (AH=40h to a FILE): POSITIONED write of `len` bytes of `data`
     * starting at byte `offset` within the file at root-dir slot `slot`, via
     * fat12_write_partial -- overwrite-in-place / extend / zero-fill-hole, with
     * both-FAT sync and allocate-then-commit disk-full rollback. Returns the
     * UPDATED dir entry (new size / start_cluster) in *out_entry so the caller
     * refreshes its SFT copy. Sets *out_written (== len on success; 0 on a
     * disk-full rollback). Returns 0 on success or a DOS error (0x0005 disk full
     * / write error). The bytes are committed to disk by THIS call (no deferred
     * flush). May be NULL on a read-only backend. */
    uint16_t (*write_at)(uint32_t slot, uint32_t offset, const uint8_t *data,
                         uint32_t len, uint32_t *out_written,
                         struct dir_entry *out_entry);

    /* CLOSE: finalize a handle at root-dir slot `slot`. With per-call flushing
     * in write_at() this is a no-op; the hook is kept for symmetry (and a future
     * deferred-buffering write model). Called by 3Eh CLOSE when the last
     * reference to a FILE slot drops. May be NULL. */
    void (*close)(uint32_t slot);

    /* UNLINK (AH=41h DELETE): delete the 8.3 file `name83` (mark deleted + free
     * its chain). Returns 0 on success or 0x0002 (not found) / 0x0005 (error). */
    uint16_t (*unlink)(const char *name83);
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
 * The callback loads the flat .COM `name83` (root-dir 8.3 only this milestone)
 * from the mounted volume and RUNS it to completion (the child terminates via
 * 4Ch / INT 20h and the loader regains control). On a clean run it returns 0 and
 * writes the child's exit code to *out_rc; on a failure it returns a DOS error
 * code (0x0002 file not found, 0x0008 insufficient memory / load already active
 * -- nested EXEC, 0x000B bad format / too large) and leaves *out_rc unchanged.
 * `name83` is already validated (no '\'/':'); the callback need not re-check. */
typedef uint16_t (*int21_exec_fn)(const char *name83, uint8_t *out_rc);

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
