/* loader.h -- InitechDOS flat program loader (lay out PSP + image, run, return).
 *
 * beads: initech-509.5 (the MILTON loader keystone; advances f8v.4). This is the
 *        milestone where InitechDOS RUNS A PROGRAM for the first time: a flat
 *        (.COM-equivalent) loader that builds a PSP + program image at the locked
 *        addresses, transfers control to the program, and handles the program's
 *        return via INT 21h AH=4Ch / INT 20h.
 *
 * Ref:   docs/research/psp-loader-ground-truth.md Sec 3 (flat program model +
 *        memory layout), Sec 4 (THE control-transfer + return-to-loader
 *        mechanism: saved kernel context + rebindable exit hook + non-returning
 *        stack-restore jump), Sec 5 (the baked test program), Sec 6 (scope --
 *        FAT-sourcing, full SFT/JFT, FCB, INT 22/23/24 all deferred), Sec 7
 *        (Risk 1 the return path, Risk 2 the 0x100 offset, Risk 3 the 4Ch
 *        contract); spec/memory_map.h (the LOCKED addresses); os/milton/psp.h
 *        (psp_build + psp_params_t); os/milton/int21.h (int21_set_exit /
 *        int21_exit_fn -- the hook we repoint). CLAUDE.md Law 1 (cite), Law 2
 *        (oracle is truth), Law 3 (artifact = C), Rule 2 (fail loud), Rule 8
 *        (specs-as-data), Rule 11 (deterministic), Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only. The
 * HOST-TESTABLE prep (loader_prepare: input validation + layout + the psp_params
 * the loader feeds psp_build) is split out so the factory oracle
 * (os/milton/test_loader.c) can verify it deterministically WITHOUT performing
 * the asm stack-switch + jump (which only makes sense in the kernel). The asm
 * control transfer + the non-returning return-to-loader live in load_program()
 * (kernel only), guarded by #ifndef __STDC_HOSTED__-style compilation: the host
 * build of loader.c omits the kernel-only jump.
 */
#ifndef INITECH_LOADER_H
#define INITECH_LOADER_H

#include <stdint.h>
#include "psp.h"   /* psp_params_t -- the inputs loader_prepare computes */

/* loader_status_t -- the result of input validation (Rule 2: fail loud, never
 * silently load a bogus image). loader_prepare() returns one of these so the
 * host oracle can assert the fail-loud behavior without trapping. */
typedef enum loader_status {
    LOADER_OK = 0,           /* inputs valid; layout + params computed          */
    LOADER_ERR_NULL_IMAGE,   /* image == NULL                                   */
    LOADER_ERR_ZERO_LEN,     /* image_len == 0 (nothing to run)                 */
    LOADER_ERR_TOO_BIG,      /* image_len > PROGRAM_IMAGE_MAX (would overrun)   */
    LOADER_ERR_NULL_OUT,     /* out plan pointer == NULL                        */
    /* FAT-sourced load (load_program_from_fat; beads initech-saw): */
    LOADER_ERR_NO_VOLUME,    /* no FAT volume bound                             */
    LOADER_ERR_NOT_FOUND,    /* named .COM not in the (root) directory          */
    LOADER_ERR_READ,         /* FAT read error pulling the .COM bytes           */
    LOADER_ERR_BUSY,         /* a load is already active (nested EXEC; deferred) */
    /* EXEC env inheritance (beads initech-1i0x Tranche E inc 3): a non-zero
     * env_block that is not the locked ENV_BLOCK region -- the caller asked the
     * child to inherit an env block at an address the loader never wrote. Fail
     * loud (Rule 2) rather than build a PSP env_seg pointing at garbage. */
    LOADER_ERR_BAD_ENV,
    /* InitechMZ (.EXE) load (beads initech-wczy / dtw.2; ADR-0003 DEC-08a): */
    LOADER_ERR_FOREIGN_MZ,   /* a genuine UNTAGGED 16-bit DOS MZ reached the
                              * loader (DEC-08a.5). The pure prep returns THIS;
                              * the KERNEL call site PANICS fail-loud rather than
                              * relocate-and-misexecute 16-bit code in flat mode.
                              * NEVER run -- a flat CPU cannot decode 16-bit. */
    LOADER_ERR_BAD_FORMAT    /* a tagged MZ that is otherwise malformed: a bad
                              * header (truncated, header > image), an OOB reloc,
                              * or a load module too big for PROGRAM_IMAGE_MAX.
                              * Fail loud (Rule 2) -- do NOT run garbage. */
} loader_status_t;

/* loader_plan_t -- the fully-computed, deterministic load layout. loader_prepare
 * fills this from the (image, image_len, cmd_tail) inputs and the LOCKED
 * spec/memory_map.h addresses; load_program() then copies the image, builds the
 * PSP from `params`, and transfers control to `entry` with ESP=`stack_top` and
 * EBX=`psp_addr`. The host oracle asserts every field. */
typedef struct loader_plan {
    uint32_t     psp_addr;     /* PROGRAM_BASE  -- where the 256-byte PSP lands  */
    uint32_t     image_dst;    /* PROGRAM_IMAGE -- PSP + 0x100 (the .COM offset) */
    uint32_t     entry;        /* program entry EIP == image_dst                 */
    uint32_t     stack_top;    /* PROGRAM_STACK_TOP -- initial program ESP       */
    uint32_t     image_len;    /* bytes to copy from `image` to image_dst        */
    const uint8_t *image_src;  /* the source image bytes (validated non-NULL)    */
    psp_params_t params;       /* the exact inputs handed to psp_build()         */

    /* AH=48h heap-arena window, COMPUTED disjoint from the loaded program (beads
     * initech-1q4u; ADR-0009 DEC-04). load_program binds the arena to
     * [arena_base, arena_ceil) via int21_mcb_bind_program -- NOT the old whole-
     * window [PROGRAM_BASE, PROGRAM_ALLOC_END) that overlaid the running program.
     * arena_base = roundup_paragraph(PROGRAM_IMAGE + image_len + PROGRAM_BSS_
     * RESERVE); arena_ceil = PROGRAM_ARENA_CEIL (== ENV_BLOCK). arena_present is 0
     * (and arena_base==arena_ceil==0) when the image is too large to leave a
     * positive arena below the ceiling -- the program then runs with NO heap and
     * 48h reports insufficient memory (fail loud, never a corrupting overlap).
     * The host oracle asserts these directly (Law 2). */
    uint32_t     arena_base;   /* flat linear base of the disjoint heap arena     */
    uint32_t     arena_ceil;   /* flat linear ceiling (exclusive); == ENV_BLOCK   */
    uint8_t      arena_present; /* 1 if a positive-size arena was computed, else 0 */
} loader_plan_t;

/* loader_env_decision_t -- the EXEC env-inheritance decision (beads initech-1i0x
 * Tranche E inc 3), computed by the pure loader_decide_env(). `env_linear` is the
 * flat addr the child PSP env_seg must point at; `write_empty` is 1 iff the loader
 * must synthesize the 2-byte empty env block at ENV_BLOCK (the inherit-empty /
 * legacy path), 0 iff the shell already populated [ENV_BLOCK, ...) and the loader
 * must leave it intact. The host oracle asserts both fields directly (Law 2). */
typedef struct loader_env_decision {
    uint32_t env_linear;   /* flat addr stored in the child PSP env_seg        */
    int      write_empty;  /* 1 => synthesize the 2-byte empty block at ENV_BLOCK */
} loader_env_decision_t;

/* loader_decide_env -- the PURE, host-testable EXEC env-inheritance decision
 * (beads initech-1i0x Tranche E inc 3). Maps the caller's env_block (0 =
 * inherit-empty; ENV_BLOCK = inherit the shell's populated env) to a
 * loader_env_decision_t. A non-zero env_block that is NOT ENV_BLOCK fails loud
 * (LOADER_ERR_BAD_ENV); a NULL out fails LOADER_ERR_NULL_OUT (Rule 2). The kernel
 * loader (loader_run_plan, kernel-only asm path) calls this to choose, so the
 * oracle and the artifact agree by construction. Ref: spec/memory_map.h ENV_BLOCK;
 * spec/dos_structs.h exec_param_block_t.env_block. */
loader_status_t loader_decide_env(uint32_t env_block, loader_env_decision_t *out);

/* loader_prepare -- validate inputs and compute the deterministic load plan
 * (layout + psp_params) WITHOUT touching memory or transferring control. Pure
 * and host-testable: the factory oracle drives this directly. Returns LOADER_OK
 * and fills *out on success; returns a LOADER_ERR_* code and leaves *out
 * unspecified on a fail-loud rejection (NULL/zero-length/oversized image).
 *
 * `cmd_tail` may be NULL (no-argument launch). The plan's params.cmd_tail points
 * at the caller's `cmd_tail` (the loader does not copy it; psp_build does).
 * Ref: psp-loader-ground-truth.md Sec 3.2 / Sec 4.1; spec/memory_map.h. */
loader_status_t loader_prepare(const uint8_t *image, uint32_t image_len,
                               const char *cmd_tail, uint32_t cmd_tail_len,
                               loader_plan_t *out);

/* loader_prepare_in_place -- the IN-PLACE variant (beads initech-za4m; ADR-0009
 * companion to DEC-04). Validates + lays out for an image ALREADY resident at
 * PROGRAM_IMAGE (0x30100) -- e.g. a FAT .COM that load_program_from_fat read
 * DIRECTLY into the program region. Identical layout/params/arena to
 * loader_prepare EXCEPT it takes NO image pointer (there is nothing to copy
 * from): it validates image_len only, sets out->image_src = NULL and leaves
 * out->image_dst == out->entry == PROGRAM_IMAGE (the in-place address). The
 * single size bound is PROGRAM_IMAGE_MAX -- the OLD 64 KiB LOAD_STAGING_MAX cap
 * is GONE (the image never transits the 64 KiB staging buffer; it is read
 * straight into the ~188 KiB program arena). Host-testable: the big-.COM oracle
 * (test_loader_big.c) drives this with image_len = 64KiB+1 / PROGRAM_IMAGE_MAX /
 * +1 to prove the new bound. Ref: spec/memory_map.h; ADR-0009 DEC-04. */
loader_status_t loader_prepare_in_place(uint32_t image_len,
                                        const char *cmd_tail,
                                        uint32_t cmd_tail_len,
                                        loader_plan_t *out);

/* loader_prepare_mz -- the PURE, host-testable InitechMZ (.EXE) PROLOGUE (beads
 * initech-wczy / dtw.2; ADR-0003 DEC-08a). The dispatch sibling of
 * loader_prepare_in_place: the WHOLE MZ FILE is already resident at `file_at`
 * (PROGRAM_IMAGE on the kernel; a host buffer in the oracle), `file_len` bytes.
 * This function performs the DEC-08a content path IN PLACE:
 *
 *   1. mz_parse_header(file_at, file_len) -- validate the MZ container.
 *        MZ_ERR_FOREIGN  -> LOADER_ERR_FOREIGN_MZ (the KERNEL caller PANICS;
 *                           DEC-08a.5 -- never relocate-and-misexecute 16-bit).
 *        any other error -> LOADER_ERR_BAD_FORMAT (fail loud; do NOT run).
 *   2. Bound-check load_module_len <= PROGRAM_IMAGE_MAX (LOADER_ERR_BAD_FORMAT).
 *   3. memmove the LOAD MODULE down over the header: from
 *        file_at + load_module_off  ->  file_at (PROGRAM_IMAGE), load_module_len
 *      bytes. After this the load base PROGRAM_IMAGE holds the flat code with the
 *      MZ header SKIPPED (DEC-08a.2 -- the header is never part of the image).
 *   4. mz_apply_relocs(file_at, load_module_len, PROGRAM_IMAGE,
 *        reloc table at file_at + reloc_table_off (pre-move offset; the reloc
 *        table lives in the header/trailer region OUTSIDE the moved load module),
 *        reloc_count) -- add PROGRAM_IMAGE to each flat dword (DEC-08a.1).
 *        An OOB reloc -> LOADER_ERR_BAD_FORMAT.
 *   5. Lay out the plan exactly as loader_prepare_in_place (PSP @ PROGRAM_BASE,
 *      image @ PROGRAM_IMAGE, arena disjoint from the load module), then:
 *        out->entry     = PROGRAM_IMAGE + entry_off       (DEC-08a.2)
 *        out->image_len = load_module_len  (the arena base sits ABOVE the module)
 *        out->image_src = NULL             (in place; loader_run_plan must NOT copy)
 *      ESP stays PROGRAM_STACK_TOP for the current release (DEC-08a.2 -- e_ss:e_sp
 *      advisory; the canonical InitechMZ emission is ss=sp=0).
 *   6. e_minalloc TEETH (DEC-08a.3): if the disjoint arena (arena_ceil -
 *      arena_base) cannot satisfy min_alloc_paras*16 bytes, fail loud
 *      LOADER_ERR_BAD_FORMAT (maps to DOS 08h insufficient memory at the EXEC
 *      seam). e_maxalloc is naturally clamped to the ceiling -- the arena already
 *      ends at PROGRAM_ARENA_CEIL, so a max-hungry 0xFFFF never reaches past it.
 *
 * The relocation WRITES into the in-place buffer, so this is NOT a no-op layout
 * computation like loader_prepare_in_place -- it mutates [file_at, file_at +
 * file_len). The host oracle (test_mzload.c) drives it against a hand-authored
 * InitechMZ image and asserts the relocated dword, the entry, and the arena.
 *
 * MUTATION hook (Rule 6; -DLOADER_MUTATE_MZ_NO_RELOC): step 4 is SKIPPED so the
 * relocated-dword assertion goes RED.  Ref: ADR-0003 DEC-08a; mz.h. */
loader_status_t loader_prepare_mz(uint8_t *file_at, uint32_t file_len,
                                  const char *cmd_tail, uint32_t cmd_tail_len,
                                  loader_plan_t *out);

/* load_program -- the full kernel loader: validate (via loader_prepare), copy
 * the image to PROGRAM_IMAGE, build the PSP at PROGRAM_BASE + the env block,
 * save the loader's kernel context, repoint the INT 21h exit hook to the
 * loader's non-returning return-to-loader hook, switch to the program stack,
 * JMP to the program entry, and -- when the program issues INT 21h AH=4Ch or
 * INT 20h -- regain control, restore the previous exit hook, and return the
 * program's exit code.
 *
 * Returns LOADER_OK on a clean run-and-return; *out_exit_code receives the
 * program's exit code. Returns a LOADER_ERR_* on a validation failure (the
 * program is NOT run; *out_exit_code is left unchanged).
 *
 * KERNEL ONLY: the body performs an inline-asm stack switch + jump and is
 * compiled only in the freestanding (kernel) build. In a hosted build it is a
 * no-op stub that returns the prepared status (the host oracle exercises
 * loader_prepare directly, never the asm).
 *
 * Ref: psp-loader-ground-truth.md Sec 4 (the mechanism) / Sec 7 Risk 1. */
loader_status_t load_program(const uint8_t *image, uint32_t image_len,
                             const char *cmd_tail, uint32_t cmd_tail_len,
                             uint8_t *out_exit_code);

/* load_program_in_place -- run a program whose image is ALREADY resident at
 * PROGRAM_IMAGE (beads initech-za4m; ADR-0009 companion to DEC-04). Everything
 * load_program does EXCEPT the image copy: validate via loader_prepare_in_place,
 * build the PSP at PROGRAM_BASE + the env block, save/repoint the INT 21h exit
 * hook, bind the disjoint MCB arena, save the loader's kernel context, switch to
 * the program stack, JMP to PROGRAM_IMAGE, and -- on the child's INT 21h AH=4Ch /
 * INT 20h -- regain control and return its exit code. The control-transfer and
 * return-to-loader path is BYTE-FOR-BYTE the same as load_program (a shared
 * static helper drives both); only the "copy staging -> PROGRAM_IMAGE" step is
 * omitted, because the caller (load_program_from_fat) already read the .COM
 * straight into PROGRAM_IMAGE.
 *
 * `image_len` is the on-disk byte count already present at PROGRAM_IMAGE (used
 * for the arena base = roundup(PROGRAM_IMAGE + image_len + BSS_RESERVE) and the
 * PROGRAM_IMAGE_MAX bound). `env_block` (beads initech-1i0x Tranche E inc 3) is
 * the FLAT linear address of the env block the child inherits, or 0 for
 * inherit-empty (the loader then synthesizes the 2-byte empty block at ENV_BLOCK,
 * the legacy behavior); a non-zero value MUST be the locked ENV_BLOCK region or
 * the call fails LOADER_ERR_BAD_ENV (Rule 2). Returns LOADER_OK on a clean
 * run-and-return with *out_exit_code set; a LOADER_ERR_* on a validation failure
 * (the program is NOT run). KERNEL ONLY; in a hosted build it is a validate-only
 * stub. Ref: spec/memory_map.h; ADR-0009 DEC-04; psp-loader-ground-truth.md Sec 4. */
loader_status_t load_program_in_place(uint32_t image_len, const char *cmd_tail,
                                      uint32_t cmd_tail_len, uint32_t env_block,
                                      uint8_t *out_exit_code);

/* ---- FAT-sourced load (beads initech-saw; DIRECT-LOAD beads initech-za4m) --
 * load_program_from_fat -- the "saw" core: load a flat .COM BY NAME from the
 * mounted FAT12 volume and run it. Reads the named file (the bare 8.3 leaf, e.g.
 * "GREET.COM") from the directory whose first data cluster is `dir_start` (0 ==
 * the fixed root) off the volume bound via loader_bind_fat_volume() DIRECTLY into
 * the program region (PROGRAM_IMAGE, 0x30100) -- NO intermediate staging buffer --
 * then runs it through load_program_in_place() (PSP + JMP + return; no copy). All
 * scratch is kernel BSS (spec/memory_map.h Risk 2 -- never a multi-KB buffer on
 * the kernel stack).
 *
 * SIZE BOUND (beads initech-za4m): because the .COM is read straight into the
 * program arena, the ONLY size limit is PROGRAM_IMAGE_MAX (~188 KiB). The OLD
 * path staged through a 64 KiB buffer (LOAD_STAGING_MAX) that abutted the kernel
 * stack and could not grow -- it wrongly rejected a 77 KiB SAMIR.COM. That cap is
 * gone; LOAD_STAGING is now UNUSED by this path.
 *
 * SUBDIR EXEC (beads initech-zs24, Landing 2): `dir_start` is the containing
 * directory do_exec resolved the EXEC path to through the file-backend resolve
 * seam (the SAME resolution OPEN uses). dir_start==0 takes the byte-identical
 * historical ROOT find (fat12_find); a non-zero cluster locates the leaf in that
 * subdir via fat12_find_slot_in over the cached FAT, then loads its own cluster
 * chain exactly as the root case.
 *
 * Fail loud (Rule 2) with a DISTINCT status on each failure:
 *   - no volume bound                       -> LOADER_ERR_NO_VOLUME
 *   - file not in the directory             -> LOADER_ERR_NOT_FOUND
 *   - leaf resolves to a DIRECTORY (attr)   -> LOADER_ERR_NOT_FOUND (not runnable)
 *   - a FAT/ATA read error                  -> LOADER_ERR_READ
 *   - image too large for staging/program   -> LOADER_ERR_TOO_BIG
 *   - a load is already active (nested)      -> LOADER_ERR_BUSY (deferred; guarded)
 *   - env_block != 0 and != ENV_BLOCK        -> LOADER_ERR_BAD_ENV (inc 3)
 * On LOADER_OK *out_rc receives the child's exit code.
 *
 * `env_block` (beads initech-1i0x Tranche E inc 3 -- EXEC env inheritance) is the
 * FLAT linear address of the env block the child inherits, or 0 for inherit-empty
 * (the loader then synthesizes the 2-byte empty block at ENV_BLOCK, the legacy /
 * baked-demo behavior). The SHELL serializes its master env into [ENV_BLOCK, ...)
 * and passes env_block == ENV_BLOCK; the loader then does NOT overwrite that block
 * and points the child PSP env_seg at it. Threaded verbatim to load_program_in_place.
 *
 * REENTRANCY: load_program's return-to-loader context (g_loader_ctx) is
 * single-level (Sec 6.2 -- no process table). A nested call (EXEC from inside a
 * running program) is REJECTED with LOADER_ERR_BUSY rather than corrupting the
 * context; nested EXEC is a follow-up bead. KERNEL/shell-context EXEC is fine.
 *
 * KERNEL ONLY (pulls fat12.c + the volume + the asm transfer). In a HOSTED build
 * it is a no-op stub returning LOADER_ERR_NO_VOLUME (the host oracle drives the
 * AH=4Bh register/validation logic through int21's mock EXEC backend instead).
 * Ref: beads initech-saw; fs-mount-sft-ground-truth.md; spec/memory_map.h. */
struct fat12_volume;   /* full type in os/milton/fat12.h (kernel-only) */

/* Bind the mounted FAT12 volume the loader reads .COMs from (caller-owned,
 * outlives the binding). NULL clears it -> load_program_from_fat returns
 * LOADER_ERR_NO_VOLUME. Mirrors int21_set_file_backend's bind discipline. */
void loader_bind_fat_volume(const struct fat12_volume *vol);

loader_status_t load_program_from_fat(const char *name83, uint16_t dir_start,
                                      const char *cmd_tail, uint32_t cmd_tail_len,
                                      uint32_t env_block, uint8_t *out_rc);

#endif /* INITECH_LOADER_H */
