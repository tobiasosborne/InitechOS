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
    LOADER_ERR_NULL_OUT      /* out plan pointer == NULL                        */
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
} loader_plan_t;

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

#endif /* INITECH_LOADER_H */
