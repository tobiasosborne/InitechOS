/*
 * os/milton/test_flair_heap_ram.c -- FLAIR-heap RAM-sufficiency oracle (FO-G).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. TEST_HARNESS / CHECK /
 * TEST_SUMMARY from seed/test_assert.h. Non-zero exit on any failure (Law 2:
 * the oracle is the truth, never false-green).
 *
 * WHAT IT GATES (ADR-0004 DEC-03 / FO-G; beads initech-k8o5.5): the kernel boot
 * path (os/milton/kmain.c) refuses to run -- PANICS LOUD, Rule 2 -- on a machine
 * whose probed extended memory (boot_info_t.ext_mem_kb) cannot back the FIXED
 * FLAIR heap window [FLAIR_HEAP_BASE, FLAIR_HEAP_BASE+FLAIR_HEAP_SIZE). The
 * decision is the PURE function flair_heap_ram_ok() in spec/memory_map.h, shared
 * verbatim by the kernel gate and this oracle, so artifact and test agree by
 * construction (Law 2). This test pins that pure function:
 *
 *   - returns FAIL (0) for every ext_mem_kb BELOW the derived minimum,
 *   - returns PASS (1) at the boundary (exactly the minimum) and above,
 *   - the threshold is DERIVED from the locked constants (not a magic 4096),
 *   - real-machine sizes (128 MiB -> ~130048 KiB) PASS,
 *   - a probe failure (ext_mem_kb == 0) FAILS (the fail-safe gate fires).
 *
 * MUTATION GATE (compile with -DFLAIR_RAM_MUTANT): the test's EXPECTATION at
 * the boundary is flipped to a wrong off-by-one stance that the CORRECT pure
 * function cannot satisfy, so the gate goes RED -- proving the oracle bites
 * (Rule 6). Restoring (no -D) returns it GREEN.
 *
 * Compile (host, -std=c11):
 *   gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *       -Iseed -Ispec os/milton/test_flair_heap_ram.c -o build/test_flair_heap_ram
 *   ./build/test_flair_heap_ram
 *
 * Mutant (should go RED):
 *   gcc ... -DFLAIR_RAM_MUTANT ... -o build/test_flair_heap_ram_mutant
 *   ./build/test_flair_heap_ram_mutant     # expect FAIL lines + exit 1
 *
 * Ref (Law 1): ADR-0004 DEC-03 full ruling (Sec 8.1) + FO-G (Sec 8.4);
 *   spec/memory_map.h (FLAIR_HEAP_BASE/SIZE/MIN, FLAIR_HEAP_REQUIRED_EXT_KB,
 *   flair_heap_ram_ok); os/milton/kmain.c (the boot gate that calls it);
 *   CLAUDE.md Law 2 + Rule 2 + Rule 6 + Rule 11 + Rule 12.
 * ASCII-clean (Rule 12). No timestamps / host paths / nondeterminism (Rule 11).
 */

#include <stdio.h>

#include "test_assert.h"
#include "memory_map.h"   /* flair_heap_ram_ok + FLAIR_HEAP_* (the contract) */

TEST_HARNESS();

/* The minimum, computed INDEPENDENTLY of the header's derivation, so the test
 * is a genuine second opinion (not a tautology against FLAIR_HEAP_REQUIRED_EXT_KB).
 * Required extended KiB = (top-of-window - 1 MiB) / 1024. The window top is
 * FLAIR_HEAP_BASE + FLAIR_HEAP_SIZE; ext_mem_kb counts KiB above the 1 MiB line. */
#define MIN_KB_INDEP \
    (((unsigned long)FLAIR_HEAP_BASE + (unsigned long)FLAIR_HEAP_SIZE \
      - 0x00100000UL) / 1024UL)

/* -----------------------------------------------------------------------
 * Test A: the derived constants are internally consistent + match the locked
 * window. This catches a Rule-8 drift between the header's derivation and the
 * window it claims to back.
 * ----------------------------------------------------------------------- */
static void test_threshold_derivation(void)
{
    /* The header's published threshold must equal the independently-computed one. */
    CHECK((unsigned long)FLAIR_HEAP_REQUIRED_EXT_KB == MIN_KB_INDEP,
          "FLAIR_HEAP_REQUIRED_EXT_KB == (top-window - 1MiB)/1024 (derivation agrees)");

    /* For the locked window it must be exactly 4096 KiB (4 MiB) -- and that is
     * FLAIR_HEAP_MIN / 1024. NOT hardcoded into the function; checked here as
     * the known-correct value for the ratified DEC-03 window. */
    CHECK((unsigned long)FLAIR_HEAP_REQUIRED_EXT_KB == 4096UL,
          "FLAIR_HEAP_REQUIRED_EXT_KB == 4096 KiB for the locked 4 MiB window");

    CHECK((unsigned long)FLAIR_HEAP_REQUIRED_EXT_KB
              == (unsigned long)FLAIR_HEAP_MIN / 1024UL,
          "required ext KiB == FLAIR_HEAP_MIN / 1024 (window/min agree)");
}

/* -----------------------------------------------------------------------
 * Test B: below the minimum FAILS. Several values strictly under the threshold,
 * including 0 (the probe-failed fail-safe case) and min-1 (off-by-one).
 * ----------------------------------------------------------------------- */
static void test_below_min_fails(void)
{
    unsigned long min = MIN_KB_INDEP;

    CHECK(flair_heap_ram_ok(0UL) == 0,
          "ext_mem_kb == 0 (probe failed) -> FAIL (fail-safe gate fires)");

    CHECK(flair_heap_ram_ok(1UL) == 0,
          "ext_mem_kb == 1 KiB -> FAIL (well below min)");

    CHECK(flair_heap_ram_ok(min - 1UL) == 0,
          "ext_mem_kb == min-1 -> FAIL (boundary, off-by-one low)");

    CHECK(flair_heap_ram_ok(min / 2UL) == 0,
          "ext_mem_kb == min/2 -> FAIL");

    /* 2 MiB extended (2048 KiB) is the rejected minority window's size -- it
     * must FAIL the ratified 4 MiB contract. */
    CHECK(flair_heap_ram_ok(2048UL) == 0,
          "ext_mem_kb == 2048 KiB (2 MiB) -> FAIL (below the ratified 4 MiB)");
}

/* -----------------------------------------------------------------------
 * Test C: at and above the minimum PASSES. The boundary (exactly min) MUST
 * pass -- a machine with exactly the required RAM backs the whole window.
 *
 * The MUTANT flips the boundary expectation: it asserts the boundary FAILS,
 * which the CORRECT >= function does not do -> RED (Rule 6).
 * ----------------------------------------------------------------------- */
static void test_at_and_above_min_passes(void)
{
    unsigned long min = MIN_KB_INDEP;

#ifndef FLAIR_RAM_MUTANT
    CHECK(flair_heap_ram_ok(min) == 1,
          "ext_mem_kb == min -> PASS (boundary; at-min backs the window)");
#else
    /* MUTANT: assert the boundary FAILS. The correct flair_heap_ram_ok uses
     * '>=' so it returns 1 at the boundary -> this CHECK fails -> gate RED. */
    CHECK(flair_heap_ram_ok(min) == 0,
          "MUTANT: ext_mem_kb == min -> FAIL (should NOT hold for a '>=' gate)");
#endif

    CHECK(flair_heap_ram_ok(min + 1UL) == 1,
          "ext_mem_kb == min+1 -> PASS (just above the boundary)");

    CHECK(flair_heap_ram_ok(min * 2UL) == 1,
          "ext_mem_kb == 2*min -> PASS");

    /* Real emulator/hardware sizes. 128 MiB guest reports ~130048 KiB above
     * 1 MiB (131072 total - 1024); 16 MiB -> 15360; 64 MiB -> 65536. */
    CHECK(flair_heap_ram_ok(15360UL) == 1,
          "ext_mem_kb == 15360 (16 MiB machine) -> PASS");

    CHECK(flair_heap_ram_ok(65536UL) == 1,
          "ext_mem_kb == 65536 (64 MiB machine) -> PASS");

    CHECK(flair_heap_ram_ok(130048UL) == 1,
          "ext_mem_kb == 130048 (128 MiB QEMU/Bochs default) -> PASS");
}

int main(void)
{
#ifndef FLAIR_RAM_MUTANT
    printf("test_flair_heap_ram: FLAIR_HEAP_REQUIRED_EXT_KB = %lu KiB\n",
           (unsigned long)FLAIR_HEAP_REQUIRED_EXT_KB);
#else
    printf("test_flair_heap_ram (MUTANT): expects failures; gate should exit non-zero\n");
#endif

    test_threshold_derivation();
    test_below_min_fails();
    test_at_and_above_min_passes();

    return TEST_SUMMARY("test_flair_heap_ram");
}
