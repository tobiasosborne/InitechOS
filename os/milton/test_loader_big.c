/* test_loader_big.c -- host unit oracle for the FAT IN-PLACE loader prep
 * (beads initech-za4m; ADR-0009 companion to DEC-04). Factory test: libc OK,
 * reuses seed/test_assert.h. Compiles HOSTED against the REAL artifact loader.c
 * (the same loader_prepare_in_place the kernel runs; loader.c compiles both
 * freestanding and hosted -- the asm control-transfer path is elided hosted).
 *
 * THE BUG THIS LOCKS DOWN (beads initech-za4m): load_program_from_fat USED to
 * read the FAT .COM into a 64 KiB LOAD_STAGING buffer [0x70000,0x80000) (it abuts
 * the kernel stack at 0x80000 and cannot grow), then copy DOWN to PROGRAM_IMAGE.
 * The size guard rejected file_size > LOAD_STAGING_MAX (64 KiB), so a 77 KiB
 * SAMIR.COM was wrongly rejected LOADER_ERR_TOO_BIG even though the program arena
 * allows ~188 KiB (PROGRAM_IMAGE_MAX = ENV_BLOCK - PROGRAM_IMAGE). The fix reads
 * the .COM DIRECTLY into PROGRAM_IMAGE -- no staging -- so PROGRAM_IMAGE_MAX is
 * the SOLE bound. This oracle PROVES the new bound at the loader_prepare_in_place
 * seam (the pure, host-testable layout/validation the in-place run path uses).
 *
 * Ref: spec/memory_map.h (PROGRAM_IMAGE / PROGRAM_IMAGE_MAX / LOAD_STAGING_MAX);
 *      docs/adr/ADR-0009-...md (DEC-04 + the za4m companion); os/milton/loader.h.
 *      CLAUDE.md Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 2 (fail loud),
 *      Rule 6 (mutation-prove), Rule 8 (locked spec), Rule 12 (ASCII).
 *
 * Strategy: loader_prepare_in_place() is the pure, host-testable seam (no image
 * pointer -- the bytes are already at PROGRAM_IMAGE; it validates image_len only).
 * We assert the binding size bound is PROGRAM_IMAGE_MAX, NOT LOAD_STAGING_MAX:
 *   - 64 KiB + 1 (was rejected by the old LOAD_STAGING_MAX cap) -> now OK.
 *   - PROGRAM_IMAGE_MAX (the boundary) -> OK.
 *   - PROGRAM_IMAGE_MAX + 1 (over the arena) -> LOADER_ERR_TOO_BIG.
 *   - the in-place plan carries image_src == NULL (nothing to copy from) with
 *     image_dst == entry == PROGRAM_IMAGE (load straight into the program region).
 *   - a compile-time check that PROGRAM_IMAGE_MAX > LOAD_STAGING_MAX (so 64K+1
 *     is a MEANINGFUL case -- the old cap really was the smaller, binding one).
 *
 * MUTATION (Rule 6), driven by make:
 *   -DLOADER_MUTATE_REIMPOSE_STAGING_CAP : loader.c (loader_prepare_core) re-
 *       imposes the PRE-za4m 64 KiB LOAD_STAGING_MAX cap on the in-place path --
 *       the EXACT bug that wrongly rejected a 77 KiB SAMIR.COM. This guard lives
 *       in the host-compilable loader_prepare_core, so THIS oracle observes it
 *       directly: the "64 KiB + 1 loads" case goes RED (loader_prepare_in_place
 *       returns LOADER_ERR_TOO_BIG instead of LOADER_OK). A mutant that PASSES
 *       means the cap-lift assertion is decoration.
 *   (There is also -DLOADER_MUTATE_INPLACE_DOUBLE_HANDLE in the FREESTANDING run
 *   path loader_run_plan -- it forces the image copy ON for the in-place plan,
 *   re-imposing the pre-za4m double-handling. That path is kernel-only/asm and
 *   not reachable from this host oracle; it is documented + compile-guarded so a
 *   future emu mutant can exercise it. The host gate uses the cap mutant above.)
 */

#include <stdint.h>

#include "loader.h"
#include "memory_map.h"
#include "test_assert.h"

TEST_HARNESS();

/* Compile-time sanity: the OLD staging cap really IS the smaller, binding one --
 * otherwise "64 KiB + 1" would not be a meaningful regression case. If this ever
 * fails the spec changed and the oracle's premise is stale (fail loud at compile
 * time, Rule 2). */
_Static_assert(PROGRAM_IMAGE_MAX > LOAD_STAGING_MAX,
               "za4m premise: program arena must exceed the old 64 KiB staging cap");

/* The smallest image the OLD LOAD_STAGING_MAX cap wrongly rejected. */
#define OVER_OLD_CAP   (LOAD_STAGING_MAX + 1u)   /* 64 KiB + 1 -- e.g. 77 KiB SAMIR */

int main(void)
{
    /* ---- Case 1: 64 KiB + 1 -- REJECTED by the old cap, now ACCEPTED. ---- */
    {
        loader_plan_t plan;
        loader_status_t st = loader_prepare_in_place(OVER_OLD_CAP,
                                                     (const char *)0, 0u, &plan);
        CHECK(st == LOADER_OK,
              "64 KiB + 1 image must now load IN PLACE (old LOAD_STAGING_MAX cap "
              "lifted to PROGRAM_IMAGE_MAX -- beads za4m)");

        /* The in-place plan: no source (already at PROGRAM_IMAGE), dst == entry
         * == PROGRAM_IMAGE (load straight into the program region). */
        CHECK(plan.image_src == (const uint8_t *)0,
              "in-place plan must carry NULL image_src (nothing to copy from)");
        CHECK(plan.image_dst == PROGRAM_IMAGE,
              "in-place image_dst must be PROGRAM_IMAGE (read directly there)");
        CHECK(plan.entry == PROGRAM_IMAGE,
              "in-place entry must be PROGRAM_IMAGE (flat .COM entry, no copy)");
        CHECK(plan.psp_addr == PROGRAM_BASE,
              "in-place psp_addr must be PROGRAM_BASE (PSP built in place)");
        CHECK(plan.image_len == OVER_OLD_CAP,
              "in-place image_len must pass through unchanged");
    }

    /* ---- Case 2: PROGRAM_IMAGE_MAX -- the boundary -- ACCEPTED. ---------- */
    {
        loader_plan_t plan;
        loader_status_t st = loader_prepare_in_place(PROGRAM_IMAGE_MAX,
                                                     (const char *)0, 0u, &plan);
        CHECK(st == LOADER_OK,
              "image_len == PROGRAM_IMAGE_MAX must be accepted (the boundary)");
        CHECK(plan.image_dst == PROGRAM_IMAGE,
              "boundary in-place image_dst must be PROGRAM_IMAGE");
    }

    /* ---- Case 3: PROGRAM_IMAGE_MAX + 1 -- over the arena -- REJECTED. ---- */
    {
        loader_plan_t plan;
        loader_status_t st = loader_prepare_in_place(PROGRAM_IMAGE_MAX + 1u,
                                                     (const char *)0, 0u, &plan);
        CHECK(st == LOADER_ERR_TOO_BIG,
              "image_len > PROGRAM_IMAGE_MAX must be rejected (LOADER_ERR_TOO_BIG) "
              "-- PROGRAM_IMAGE_MAX is the SOLE bound");
    }

    /* ---- Case 4: zero length / NULL out are still rejected (fail loud). -- */
    {
        loader_plan_t plan;
        CHECK(loader_prepare_in_place(0u, (const char *)0, 0u, &plan)
                  == LOADER_ERR_ZERO_LEN,
              "zero-length in-place image must be rejected (LOADER_ERR_ZERO_LEN)");
        CHECK(loader_prepare_in_place(4u, (const char *)0, 0u, (loader_plan_t *)0)
                  == LOADER_ERR_NULL_OUT,
              "NULL out plan must be rejected (LOADER_ERR_NULL_OUT)");
    }

    /* ---- Case 5: a command tail passes through to psp_params. ------------ */
    {
        loader_plan_t plan;
        const char *tail = "BAR";
        loader_status_t st = loader_prepare_in_place(OVER_OLD_CAP, tail, 3u, &plan);
        CHECK(st == LOADER_OK, "in-place + tail must prepare OK");
        CHECK(plan.params.cmd_tail == tail,
              "in-place params.cmd_tail must point at the caller's tail");
        CHECK(plan.params.cmd_tail_len == 3u,
              "in-place params.cmd_tail_len must pass through (3)");
    }

    return TEST_SUMMARY("test_loader_big");
}
