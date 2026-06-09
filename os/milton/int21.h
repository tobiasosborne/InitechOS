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

/* ---- File backend (beads initech-509.5 read-side) -------------------------
 * The file-handle functions (3Dh OPEN, 3Fh READ on a FILE, 4Eh/4Fh
 * FINDFIRST/FINDNEXT) need the mounted FAT12 volume -- which lives in the
 * kernel (ata.c + fat12.c), is NOT host-testable, and must NOT be linked into
 * the int21 unit oracle. So the FAT-specific work is reached through a backend
 * vtable the kernel binds (a FAT12-backed impl in os/milton/fileio_fat.c) and
 * the host oracle binds to an in-memory mock. int21.c owns the SFT/JFT slot
 * management, the file offset, and the DTA/find-data write; the backend owns
 * the volume, the whole-file read into the static buffer, and dir enumeration.
 * This mirrors the sink/exit-hook seam that keeps int21.c host-testable.
 *
 * `struct dir_entry` is forward-declared (the full type is spec/dos_structs.h,
 * which int21.c pulls in via sft.h -> psp.h). */
struct dir_entry;

typedef struct int21_file_backend {
    /* OPEN: locate the 8.3 file `name83` in the (root) directory, read its
     * WHOLE contents into the backend's static buffer, and return a pointer +
     * byte count + a copy of its 32-byte directory entry. Returns 0 on success
     * or a DOS error code (0x0002 not found, 0x0004 buffer busy / file too
     * large for the single buffer). `*out_data`/`*out_size`/`*out_entry` are
     * written only on success. */
    uint16_t (*open)(const char *name83, struct dir_entry *out_entry,
                     const uint8_t **out_data, uint32_t *out_size);

    /* CLOSE: release the single open-file buffer so a later OPEN may reuse it.
     * Called by 3Eh CLOSE when the last reference to a FILE slot drops. */
    void (*close)(void);

    /* Directory enumeration for FINDFIRST/FINDNEXT: copy the directory entry at
     * 0-based `index` into `*out_entry` and set `*out_found` = 1; at/after the
     * end of the directory set `*out_found` = 0. Returns 0 on success, non-zero
     * (a DOS error) on a backend read failure. Deleted/LFN slots are skipped by
     * the backend so consecutive indices map to surviving 8.3 entries. */
    uint16_t (*dir_entry)(uint32_t index, struct dir_entry *out_entry,
                          int *out_found);
} int21_file_backend_t;

/* Bind the file backend (NULL clears it -> the file functions return
 * file-not-found / no-more-files as if the volume were empty). The kernel binds
 * the FAT12 backend after a successful mount; the host oracle binds a mock. */
void int21_set_file_backend(const int21_file_backend_t *backend);

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
