/*
 * os/milton/test_fpu.c -- host oracle for the x87 bring-up DECISION logic
 * (beads initech-zj6w; make test-fpu-probe / test-fpu-probe-mutant).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Includes the SAME fpu.h the
 * kernel's sysinit_fpu_init uses, so the probe verdict and the CR0 arithmetic
 * graded here are byte-for-byte the boot path's.
 *
 * Why a host oracle: neither emulator can take the x87 unit away (QEMU TCG
 * executes x87 even with -cpu ...,-fpu; Bochs is built with its FPU), so the
 * "no coprocessor" readings (sentinel untouched / bus zeros / bus ones) can
 * only be fed to the verdict here. The emulator side of the absent path
 * (EM=1 -> #NM) is test-fpu-absent.
 *
 * Expected values (independent of fpu.h -- worked by hand from Intel):
 *   - FNINIT on a live x87 leaves SW = 0000H and CW = 037FH (SDM Vol 1 8.1.5);
 *     037FH AND 103FH = 003FH -> PRESENT. Bits 8-15 of SW are not examined
 *     (C3..C0/TOP/B), only the low byte, per the classic Intel sequence.
 *   - absent coprocessor, nothing stored: SW = CW = 5A5AH sentinel ->
 *     5AH != 0 -> ABSENT.
 *   - bus reads zeros: SW = 0000H, CW = 0000H -> 0000H AND 103FH = 0 != 3FH
 *     -> ABSENT (the case a status-word-only probe gets WRONG).
 *   - bus reads ones: SW = CW = FFFFH -> low byte FFH -> ABSENT.
 *   - CR0 (SDM Vol 3A 2.5 bit positions MP=1 EM=2 TS=3 NE=5, PE=0, ET=4):
 *       stage2 handoff 00000011H (PE|ET), present -> 00000033H (PE|MP|ET|NE),
 *       absent -> 00000035H (PE|EM|ET|NE); a CR0 carrying EM|TS|MP
 *       (0000001FH) -> present 00000033H, absent 00000035H; high bits
 *       (PG 80000000H, CD/NW 60000000H) pass through untouched.
 *
 * MUTANTS (Rule 6), each must make this exit NON-ZERO:
 *   -DFPU_MUTATE_PROBE_SW_ONLY          verdict ignores the control word
 *   -DFPU_MUTATE_ABSENT_KEEPS_EM_CLEAR  absent CR0 forgets EM=1
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "test_assert.h"
#include "fpu.h"
#include "fpu_contract.h"

TEST_HARNESS();

static void test_probe_verdict(void)
{
    CHECK(fpu_probe_present(0x0000u, 0x037Fu) == 1,
          "probe: live x87 after FNINIT (SW 0000, CW 037F) -> present");
    CHECK(fpu_probe_present(0x0000u, (uint16_t)X87_BOOT_CONTROL_WORD) == 1,
          "probe: the spec boot CW also satisfies the FNINIT pattern");
    CHECK(fpu_probe_present(0x3800u, 0x037Fu) == 1,
          "probe: SW high byte (TOP/C bits) is not examined -> present");
    CHECK(fpu_probe_present(0x5A5Au, 0x5A5Au) == 0,
          "probe: sentinel untouched (absent FPU stores nothing) -> absent");
    CHECK(fpu_probe_present(0x0000u, 0x0000u) == 0,
          "probe: bus reads zeros (SW 0000, CW 0000) -> absent");
    CHECK(fpu_probe_present(0xFFFFu, 0xFFFFu) == 0,
          "probe: bus reads ones -> absent");
    CHECK(fpu_probe_present(0x0001u, 0x037Fu) == 0,
          "probe: SW low byte non-zero (IE still set) -> absent");
    CHECK(fpu_probe_present(0x0000u, 0x137Fu) == 0,
          "probe: CW bit 12 set (not the FNINIT pattern) -> absent");
    CHECK(fpu_probe_present(0x0000u, 0x0040u) == 0,
          "probe: post-RESET CW 0040 (no FNINIT happened) -> absent");
}

static void test_cr0(void)
{
    CHECK(fpu_cr0_present(0x00000011u) == 0x00000033u,
          "cr0 present: handoff PE|ET -> PE|MP|ET|NE");
    CHECK(fpu_cr0_absent(0x00000011u) == 0x00000035u,
          "cr0 absent: handoff PE|ET -> PE|EM|ET|NE");
    CHECK(fpu_cr0_present(0x0000001Fu) == 0x00000033u,
          "cr0 present: clears EM and TS, keeps MP");
    CHECK(fpu_cr0_absent(0x0000001Fu) == 0x00000035u,
          "cr0 absent: clears MP and TS, keeps EM");
    CHECK(fpu_cr0_present(0xE0000011u) == 0xE0000033u,
          "cr0 present: PG/CD/NW high bits pass through");
    CHECK(fpu_cr0_absent(0xE0000011u) == 0xE0000035u,
          "cr0 absent: PG/CD/NW high bits pass through");
    /* Absent -> EM must be set, else x87 ops would not trap #NM. */
    CHECK((fpu_cr0_absent(0x00000011u) & 0x00000004u) != 0u,
          "cr0 absent: EM (bit 2) set");
    CHECK((fpu_cr0_present(0x00000015u) & 0x00000004u) == 0u,
          "cr0 present: EM (bit 2) clear");
}

static void test_spec_cw(void)
{
    /* The locked boot CW is Intel's FINIT value (SDM Vol 1 8.1.5). */
    CHECK(X87_BOOT_CONTROL_WORD == 0x037Fu,
          "spec/fpu_contract.h: X87_BOOT_CONTROL_WORD == 0x037F (FINIT value)");
}

int main(void)
{
    test_probe_verdict();
    test_cr0();
    test_spec_cw();
    return TEST_SUMMARY("test_fpu");
}
