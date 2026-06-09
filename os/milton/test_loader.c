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
 */

#include <stdint.h>

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
              "psp_addr must be PROGRAM_BASE (0x30000)");
        CHECK(plan.image_dst == PROGRAM_BASE + 0x100u,
              "image_dst must be PROGRAM_BASE + 0x100 (the .COM offset)");
        CHECK(plan.image_dst == PROGRAM_IMAGE,
              "image_dst must equal the locked PROGRAM_IMAGE");
        CHECK(plan.entry == plan.image_dst,
              "entry EIP must equal image_dst (flat .COM entry)");
        CHECK(plan.stack_top == PROGRAM_STACK_TOP,
              "stack_top must be PROGRAM_STACK_TOP (0x6FFFC)");
        CHECK(plan.image_len == (uint32_t)sizeof(k_image),
              "image_len must pass through unchanged");
        CHECK(plan.image_src == k_image,
              "image_src must point at the caller's image");

        /* psp_params (Sec 2.2/2.5/2.7): linear addresses for the vestigial
         * segment fields; psp_build converts each to a fake paragraph. */
        CHECK(plan.params.alloc_end_linear == PROGRAM_ALLOC_END,
              "params.alloc_end_linear must be PROGRAM_ALLOC_END (0x70000)");
        CHECK(plan.params.env_linear == ENV_BLOCK,
              "params.env_linear must be ENV_BLOCK (0x5F000, beads 2og)");
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

    return TEST_SUMMARY("test_loader");
}
