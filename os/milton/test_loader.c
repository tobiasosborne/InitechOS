/* test_loader.c -- host unit oracle for the flat program loader's PREP
 * (beads initech-509.5). Factory test: libc OK, reuses seed/test_assert.h.
 * Compiles HOSTED against the REAL artifact loader.c (the same loader_prepare
 * the kernel runs; loader.c compiles both freestanding and hosted -- the asm
 * control-transfer path is elided in the hosted build).
 *
 * Ref: docs/research/psp-loader-ground-truth.md Sec 3.2 (the layout addresses),
 *      Sec 4.1 (the psp_params the loader feeds psp_build), Sec 7 Risk 1/2/3;
 *      spec/memory_map.h (the LOCKED addresses); os/milton/psp.h. CLAUDE.md
 *      Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 2 (fail loud), Rule 6
 *      (mutation-prove), Rule 12 (ASCII).
 *
 * Strategy: loader_prepare() is the pure, host-testable seam. We assert:
 *   - oversized image is REJECTED (LOADER_ERR_TOO_BIG) -- fail loud, no plan.
 *   - NULL image / zero length / NULL out are each rejected with their code.
 *   - a valid image yields the LOCKED layout (psp at PROGRAM_BASE; image at
 *     PROGRAM_BASE+0x100; entry == image_dst; stack at PROGRAM_STACK_TOP) and
 *     the correct psp_params (alloc/env/parent linear + cmd_tail passthrough).
 *
 * MUTATION (Rule 6), driven by make:
 *   -DLOADER_MUTATE_NO_OFFSET : image loaded at PROGRAM_BASE (no +0x100) ->
 *                               the image_dst/entry == PROGRAM_BASE+0x100
 *                               assertions go RED. A mutant that PASSES means
 *                               the layout oracle is decoration.
 *   -DLOADER_MUTATE_FORCE_EMPTY_ENV : loader_decide_env IGNORES the provided
 *                               env_block and always chooses the empty-block path
 *                               (write_empty=1, env_linear=ENV_BLOCK) -- the exact
 *                               pre-inc-3 bug (every child inherits an empty env).
 *                               The EXEC-env inheritance assertions (a populated
 *                               env_block yields write_empty=0 + env_linear==block)
 *                               go RED (beads initech-1i0x Tranche E inc 3).
 */

#include <stdint.h>
#include <string.h>   /* memset -- poison the decision struct before each check */

#include "loader.h"
#include "memory_map.h"
#include "test_assert.h"

TEST_HARNESS();

/* A tiny, valid program image (bytes are irrelevant to prep -- only the length
 * and non-NULL-ness matter for loader_prepare). */
static const uint8_t k_image[] = { 0xB4, 0x09, 0xCD, 0x21, 0xB4, 0x4C, 0x30,
                                   0xC0, 0xCD, 0x21 };

int main(void)
{
    /* ---- Case 1: a valid image yields the LOCKED layout + params. -------- */
    {
        loader_plan_t plan;
        loader_status_t st = loader_prepare(k_image, (uint32_t)sizeof(k_image),
                                            (const char *)0, 0u, &plan);
        CHECK(st == LOADER_OK, "valid image must prepare OK");

        /* Layout (Sec 3.2): psp at PROGRAM_BASE, image at PSP+0x100. */
        CHECK(plan.psp_addr == PROGRAM_BASE,
              "psp_addr must be PROGRAM_BASE (0x38000)");
        CHECK(plan.image_dst == PROGRAM_BASE + 0x100u,
              "image_dst must be PROGRAM_BASE + 0x100 (the .COM offset)");
        CHECK(plan.image_dst == PROGRAM_IMAGE,
              "image_dst must equal the locked PROGRAM_IMAGE");
        CHECK(plan.entry == plan.image_dst,
              "entry EIP must equal image_dst (flat .COM entry)");
        CHECK(plan.stack_top == PROGRAM_STACK_TOP,
              "stack_top must be PROGRAM_STACK_TOP (0x77FFC)");
        CHECK(plan.image_len == (uint32_t)sizeof(k_image),
              "image_len must pass through unchanged");
        CHECK(plan.image_src == k_image,
              "image_src must point at the caller's image");

        /* psp_params (Sec 2.2/2.5/2.7): linear addresses for the vestigial
         * segment fields; psp_build converts each to a fake paragraph. */
        CHECK(plan.params.alloc_end_linear == PROGRAM_ALLOC_END,
              "params.alloc_end_linear must be PROGRAM_ALLOC_END (0x78000)");
        CHECK(plan.params.env_linear == ENV_BLOCK,
              "params.env_linear must be ENV_BLOCK (0x67000, beads 2og)");
        CHECK(plan.params.parent_psp_linear == 0u,
              "params.parent_psp_linear must be 0 (no parent PSP yet, Sec 2.5)");
        CHECK(plan.params.cmd_tail == (const char *)0,
              "params.cmd_tail must pass through (NULL for no-arg launch)");
        CHECK(plan.params.cmd_tail_len == 0u,
              "params.cmd_tail_len must pass through (0 for no-arg launch)");
    }

    /* ---- Case 2: a command tail passes through to psp_params. ------------ */
    {
        loader_plan_t plan;
        const char *tail = "FOO";
        loader_status_t st = loader_prepare(k_image, (uint32_t)sizeof(k_image),
                                            tail, 3u, &plan);
        CHECK(st == LOADER_OK, "valid image + tail must prepare OK");
        CHECK(plan.params.cmd_tail == tail,
              "params.cmd_tail must point at the caller's tail");
        CHECK(plan.params.cmd_tail_len == 3u,
              "params.cmd_tail_len must pass through (3)");
    }

    /* ---- Case 3: oversized image is REJECTED (fail loud, Sec 3.2 gap). --- */
    {
        loader_plan_t plan;
        /* PROGRAM_IMAGE_MAX is the largest image that fits; +1 must be rejected.
         * We do NOT allocate that many bytes -- loader_prepare validates the
         * LENGTH before touching the image, so any non-NULL pointer is fine. */
        loader_status_t st = loader_prepare(k_image, PROGRAM_IMAGE_MAX + 1u,
                                            (const char *)0, 0u, &plan);
        CHECK(st == LOADER_ERR_TOO_BIG,
              "image_len > PROGRAM_IMAGE_MAX must be rejected (LOADER_ERR_TOO_BIG)");

        /* Exactly PROGRAM_IMAGE_MAX must be ACCEPTED (boundary). */
        st = loader_prepare(k_image, PROGRAM_IMAGE_MAX, (const char *)0, 0u, &plan);
        CHECK(st == LOADER_OK,
              "image_len == PROGRAM_IMAGE_MAX must be accepted (boundary)");
    }

    /* ---- Case 4: NULL image / zero length / NULL out are rejected. ------- */
    {
        loader_plan_t plan;
        CHECK(loader_prepare((const uint8_t *)0, 4u, (const char *)0, 0u, &plan)
                  == LOADER_ERR_NULL_IMAGE,
              "NULL image must be rejected (LOADER_ERR_NULL_IMAGE)");
        CHECK(loader_prepare(k_image, 0u, (const char *)0, 0u, &plan)
                  == LOADER_ERR_ZERO_LEN,
              "zero-length image must be rejected (LOADER_ERR_ZERO_LEN)");
        CHECK(loader_prepare(k_image, 4u, (const char *)0, 0u, (loader_plan_t *)0)
                  == LOADER_ERR_NULL_OUT,
              "NULL out plan must be rejected (LOADER_ERR_NULL_OUT)");
    }

    /* ---- Case 5: EXEC env inheritance decision (beads initech-1i0x Tranche E
     *      inc 3). loader_decide_env maps the AH=4Bh env_block (0 = inherit-empty;
     *      ENV_BLOCK = inherit the shell's populated env) to (env_linear,
     *      write_empty). The kernel loader (loader_run_plan, asm-path, kernel-only)
     *      calls THIS, so proving it here proves the artifact's inheritance choice
     *      hosted -- and the LOADER_MUTATE_FORCE_EMPTY_ENV mutant flips it RED. -- */
    {
        loader_env_decision_t dec;

        /* (a) env_block == 0 -> inherit-EMPTY: write the 2-byte block, PSP env_seg
         *     -> ENV_BLOCK. THIS is the byte-identical legacy / baked-demo path. */
        memset(&dec, 0xAA, sizeof(dec));   /* poison so we prove both fields written */
        CHECK(loader_decide_env(0u, &dec) == LOADER_OK,
              "loader_decide_env(0) must succeed (inherit-empty)");
        CHECK(dec.env_linear == ENV_BLOCK,
              "env_block=0 -> env_linear == ENV_BLOCK (PSP env_seg -> empty block)");
        CHECK(dec.write_empty == 1,
              "env_block=0 -> write_empty=1 (synthesize the 2-byte empty block; legacy)");

        /* (b) env_block == ENV_BLOCK -> inherit the POPULATED block: do NOT write
         *     the empty stub (the shell already serialized its env there); PSP
         *     env_seg -> the populated block. THE load-bearing inheritance case --
         *     the FORCE_EMPTY_ENV mutant makes write_empty=1 here, going RED. */
        memset(&dec, 0xAA, sizeof(dec));
        CHECK(loader_decide_env((uint32_t)ENV_BLOCK, &dec) == LOADER_OK,
              "loader_decide_env(ENV_BLOCK) must succeed (inherit populated env)");
        CHECK(dec.env_linear == (uint32_t)ENV_BLOCK,
              "env_block=ENV_BLOCK -> env_linear == ENV_BLOCK (PSP env_seg -> populated)");
        CHECK(dec.write_empty == 0,
              "env_block=ENV_BLOCK -> write_empty=0 (DO NOT overwrite the shell's env)");

        /* (c) a non-zero env_block that is NOT ENV_BLOCK -> fail loud (the loader
         *     never wrote that region; a child PSP env_seg there would be garbage). */
        CHECK(loader_decide_env((uint32_t)ENV_BLOCK + 0x10u, &dec)
                  == LOADER_ERR_BAD_ENV,
              "a non-ENV_BLOCK env_block must fail loud (LOADER_ERR_BAD_ENV)");

        /* (d) NULL out -> fail loud, never a write through NULL (Rule 2). */
        CHECK(loader_decide_env(0u, (loader_env_decision_t *)0)
                  == LOADER_ERR_NULL_OUT,
              "loader_decide_env NULL out must be rejected (LOADER_ERR_NULL_OUT)");
    }

    return TEST_SUMMARY("test_loader");
}
