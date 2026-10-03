/*
 * harness/diff/dbf_diff/test_hardware_spec.c -- hardware contract spec oracle.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Mirrors test_samir_spec.c
 * idiom: TEST_HARNESS / CHECK / TEST_SUMMARY from seed/test_assert.h.
 * Non-zero exit on any failure (Law 2: the oracle is the truth, never false-green).
 *
 * Asserts (ADR-0009 DEC-07; CLAUDE.md Rule 8 -- locked spec-data gate):
 *   1. spec/hardware.json: file exists, is ASCII-clean (Rule 12), contains
 *      the required top-level keys (cpu, fpu, memory, video, toolchain_reproducibility,
 *      provenance).
 *   2. cpu.value == "486+ (386 desirable, non-blocking)" (ADR-0001 DEC-02; PRD Sec 5).
 *   3. fpu.value == "optional" (ADR-0009 DEC-07: NOT "required"; most period PCs
 *      lacked an 8087; software FP is the authentic stance).
 *   4. fpu.init_by_kernel == true (Rule-8 change 2026-10-03, beads initech-zj6w /
 *      initech-yjlo: the kernel brings up the x87; was false per ADR-0009
 *      DEC-01/DEC-07). Tested as the literal JSON true. 4b: fpu.boot_control_word
 *      == spec/fpu_contract.h X87_BOOT_CONTROL_WORD (== 0x037F), os_boots_without_fpu,
 *      required_by ["Initech 123"], SAMIR soft-float recorded; isolated mutant
 *      -DHARDWARE_SPEC_MUTANT_FPU.
 *   5. memory window references PROGRAM_BASE (0x00040000) and PROGRAM_ALLOC_END
 *      (0x00080000) by value, matching spec/memory_map.h exactly (raised +0x8000
 *      from 0x30000/0x70000 by beads initech-o0td, then ANOTHER +0x8000 to
 *      0x38000/0x78000 -> 0x40000/0x80000 by beads initech-re30.2; both whole-map
 *      kernel-headroom shifts).
 *   6. schema_version key present.
 *
 * MUTATION GATE (compile with -DHARDWARE_SPEC_MUTANT to invert expectations):
 *   gcc -DHARDWARE_SPEC_MUTANT ... -> should EXIT NON-ZERO (gate goes RED).
 *   See the mutant comment blocks below.
 *
 * Compile (host, -std=c11):
 *   mkdir -p build
 *   gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *       -Iseed \
 *       harness/diff/dbf_diff/test_hardware_spec.c \
 *       -o build/test_hardware_spec
 *   ./build/test_hardware_spec spec/hardware.json
 *
 * Mutant (should go RED):
 *   gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *       -Iseed -DHARDWARE_SPEC_MUTANT \
 *       harness/diff/dbf_diff/test_hardware_spec.c \
 *       -o build/test_hardware_spec_mutant
 *   ./build/test_hardware_spec_mutant spec/hardware.json
 *   # Expected: FAIL lines + exit code 1
 *
 * Ref (Law 1): ADR-0009 DEC-07 (hardware.json mandate + fpu=optional +
 *   init_by_kernel=false); ADR-0009 DEC-01 (soft-float; kernel uninvolved);
 *   PRD Sec 5 (hardware contract table); spec/memory_map.h (PROGRAM_BASE /
 *   PROGRAM_ALLOC_END); CLAUDE.md Rule 8 + Rule 12 + Rule 11.
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_assert.h"
#include "memory_map.h"   /* PROGRAM_BASE, PROGRAM_ALLOC_END -- compare, not duplicate */
#include "fpu_contract.h" /* X87_BOOT_CONTROL_WORD -- compare, not duplicate (initech-zj6w) */

TEST_HARNESS();

/* -----------------------------------------------------------------------
 * file_contains_substr: scan at most max_bytes of the file at path for the
 * literal substring needle. Returns 1 if found, 0 if not found, -1 on error.
 * max_bytes <= 0 means read the whole file.
 * Mirrors the same helper in test_samir_spec.c.
 * ----------------------------------------------------------------------- */
static int file_contains_substr(const char *path, const char *needle,
                                 long max_bytes)
{
    FILE   *f;
    char   *buf;
    size_t  nread;
    long    fsize;
    int     found = 0;
    size_t  nlen;

    f = fopen(path, "rb");
    if (!f) { return -1; }

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    rewind(f);

    if (max_bytes > 0 && fsize > max_bytes) fsize = max_bytes;
    if (fsize <= 0) { fclose(f); return 0; }

    buf = (char *)malloc((size_t)fsize + 1);
    if (!buf) { fclose(f); return -1; }

    nread = fread(buf, 1, (size_t)fsize, f);
    fclose(f);
    buf[nread] = '\0';

    nlen = strlen(needle);
    for (size_t i = 0; i + nlen <= nread; i++) {
        if (memcmp(buf + i, needle, nlen) == 0) { found = 1; break; }
    }
    free(buf);
    return found;
}

/* -----------------------------------------------------------------------
 * file_no_nonascii: check the first max_bytes (or entire file if <= 0) for
 * non-ASCII bytes. Only tab (0x09), LF (0x0A), CR (0x0D), and 0x20..0x7E
 * are accepted. Returns 1 if clean, 0 if dirty, -1 on error.
 * ----------------------------------------------------------------------- */
static int file_no_nonascii(const char *path, long max_bytes)
{
    FILE *f;
    int   c;
    long  count = 0;

    f = fopen(path, "rb");
    if (!f) { return -1; }

    while ((c = fgetc(f)) != EOF && (max_bytes <= 0 || count < max_bytes)) {
        count++;
        if (c != 0x09 && c != 0x0A && c != 0x0D &&
            (c < 0x20 || c > 0x7E)) {
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    return 1;
}

/* -----------------------------------------------------------------------
 * hex_str: format an unsigned long as a 0x... hex string into buf (>=20 bytes).
 * ----------------------------------------------------------------------- */
static void hex_str(unsigned long v, char *buf, size_t bufsz)
{
    snprintf(buf, bufsz, "0x%08lX", v);
}

/* -----------------------------------------------------------------------
 * Test 1: file exists, is readable, is ASCII-clean.
 * Ref: CLAUDE.md Rule 12.
 * ----------------------------------------------------------------------- */
static void test_file_exists_and_ascii(const char *path)
{
    FILE *f;
    int   r;

    f = fopen(path, "rb");
    CHECK(f != NULL, "hardware.json: file exists and is readable");
    if (f) fclose(f);

    r = file_no_nonascii(path, -1L);
    CHECK(r == 1, "hardware.json: ASCII-clean (Rule 12)");
}

/* -----------------------------------------------------------------------
 * Test 2: required top-level keys present.
 * MUST have: cpu, fpu, memory, video, toolchain_reproducibility, provenance,
 * schema_version, status.
 * ----------------------------------------------------------------------- */
static void test_required_keys(const char *path)
{
    int r;

    r = file_contains_substr(path, "\"cpu\"", -1);
    CHECK(r == 1, "hardware.json: \"cpu\" key present");

    r = file_contains_substr(path, "\"fpu\"", -1);
    CHECK(r == 1, "hardware.json: \"fpu\" key present");

    r = file_contains_substr(path, "\"memory\"", -1);
    CHECK(r == 1, "hardware.json: \"memory\" key present");

    r = file_contains_substr(path, "\"video\"", -1);
    CHECK(r == 1, "hardware.json: \"video\" key present");

    r = file_contains_substr(path, "\"toolchain_reproducibility\"", -1);
    CHECK(r == 1, "hardware.json: \"toolchain_reproducibility\" key present");

    r = file_contains_substr(path, "\"provenance\"", -1);
    CHECK(r == 1, "hardware.json: \"provenance\" key present");

    r = file_contains_substr(path, "\"schema_version\"", -1);
    CHECK(r == 1, "hardware.json: \"schema_version\" key present");

    r = file_contains_substr(path, "\"status\"", -1);
    CHECK(r == 1, "hardware.json: \"status\" key present");
}

/* -----------------------------------------------------------------------
 * Test 3: cpu.value == "486+ (386 desirable, non-blocking)"
 * ADR-0009 DEC-07; ADR-0001 (DEC-01 386+ origin -> DEC-02 486-class, 32-bit flat); PRD Sec 5.
 *
 * MUTATION GATE: with -DHARDWARE_SPEC_MUTANT we assert the WRONG value
 * (the superseded "386+") to prove the gate goes RED.
 *
 * Rule-8 deliberate spec change, 2026-09-25/26: ADR-0001 DEC-02 (operator
 * ratification, bead initech-qw0t, supersedes initech-fgs1) raised the
 * authentic hardware target to 486-class with 386 desirable/non-blocking.
 * The locked value moved from "386+" to the exact string below; the old
 * value is now the mutant.
 * ----------------------------------------------------------------------- */
static void test_cpu_value(const char *path)
{
    int r;

#ifndef HARDWARE_SPEC_MUTANT
    /* Correct expectation: cpu value is the ADR-0001 DEC-02 string */
    r = file_contains_substr(path, "\"486+ (386 desirable, non-blocking)\"", -1);
    CHECK(r == 1, "hardware.json: cpu.value == \"486+ (386 desirable, non-blocking)\" (ADR-0001 DEC-02; PRD Sec 5)");
#else
    /* MUTANT: assert the superseded "386+" -- this MUST fail (gate goes RED) */
    r = file_contains_substr(path, "\"386+\"", -1);
    CHECK(r == 1, "MUTANT: hardware.json: cpu.value == \"386+\" (should FAIL)");
#endif
}

/* -----------------------------------------------------------------------
 * Test 4: fpu.value must be one of {optional, required, absent} and for
 * this contract MUST == "optional".
 * ADR-0009 DEC-07: "optional" NOT "required" (most period PCs lacked an 8087).
 *
 * MUTATION GATE: with -DHARDWARE_SPEC_MUTANT we assert fpu=="required" to
 * prove the gate goes RED on the wrong value.
 * ----------------------------------------------------------------------- */
static void test_fpu_value(const char *path)
{
    int r;

#ifndef HARDWARE_SPEC_MUTANT
    /* Correct: fpu value is "optional" */
    r = file_contains_substr(path, "\"optional\"", -1);
    CHECK(r == 1, "hardware.json: fpu.value == \"optional\" (ADR-0009 DEC-07)");

    /* "required" must NOT appear as the fpu value
     * (anachronistic -- most period PCs had no 8087). */
    r = file_contains_substr(path, "\"required\"", -1);
    CHECK(r == 0, "hardware.json: fpu.value != \"required\" (ADR-0009 DEC-07: period-inauthentic)");
#else
    /* MUTANT: assert fpu=="required" -- this MUST fail (gate goes RED) */
    r = file_contains_substr(path, "\"required\"", -1);
    CHECK(r == 1, "MUTANT: hardware.json: fpu.value == \"required\" (should FAIL)");
#endif
}

/* -----------------------------------------------------------------------
 * Test 5: fpu.init_by_kernel == true (literal JSON true, not "true").
 *
 * Rule-8 deliberate spec change, 2026-10-03 (beads initech-zj6w /
 * initech-yjlo; operator ruling: hardware x87 for Initech 123, SAMIR stays
 * soft-float): the kernel now brings the x87 up in the boot path
 * (os/milton/sysinit.c sysinit_fpu_init). The locked value moved from false
 * (ADR-0009 DEC-01/DEC-07, "DOS never inits the FPU") to true; the old value
 * is now the mutant -- the same pattern as the cpu row's 386+ -> 486+ move.
 *
 * MUTATION GATE: with -DHARDWARE_SPEC_MUTANT we assert the superseded false,
 * proving the gate goes RED.
 * ----------------------------------------------------------------------- */
static void test_fpu_init_by_kernel(const char *path)
{
    int r;

#ifndef HARDWARE_SPEC_MUTANT
    /* Correct: init_by_kernel is JSON true */
    r = file_contains_substr(path, "\"init_by_kernel\": true", -1);
    if (r != 1) {
        /* also accept no-space variant */
        r = file_contains_substr(path, "\"init_by_kernel\":true", -1);
    }
    CHECK(r == 1,
          "hardware.json: fpu.init_by_kernel == true (beads initech-zj6w: kernel x87 bring-up)");
#else
    /* MUTANT: assert the superseded false -- this MUST fail (gate goes RED) */
    r = file_contains_substr(path, "\"init_by_kernel\": false", -1);
    if (r != 1) {
        r = file_contains_substr(path, "\"init_by_kernel\":false", -1);
    }
    CHECK(r == 1,
          "MUTANT: hardware.json: fpu.init_by_kernel == false (should FAIL)");
#endif
}

/* -----------------------------------------------------------------------
 * Test 5b: the x87 ruling rows (beads initech-zj6w / initech-yjlo).
 *   - fpu.boot_control_word is the value of X87_BOOT_CONTROL_WORD in
 *     spec/fpu_contract.h, rendered "0x%04X" -- the cross-file tie (the kernel
 *     loads the header; the emulator gate test-fpu reads the JSON).
 *   - X87_BOOT_CONTROL_WORD == 0x037F, Intel's FINIT value (SDM Vol 1 8.1.5;
 *     regression guard -- the kernel must not pick a precision).
 *   - os_boots_without_fpu == true (ADR-0001: 386 desirable, non-blocking).
 *   - required_by names "Initech 123" (the ruling).
 *   - SAMIR stays soft-float: ADR-0009 DEC-01 cited in the fpu block.
 *
 * MUTATION GATE: -DHARDWARE_SPEC_MUTANT_FPU (isolated: ONLY this arm flips)
 * or -DHARDWARE_SPEC_MUTANT asserts the 53-bit-precision word 0x027F --
 * absent from the JSON, so the gate goes RED.
 * ----------------------------------------------------------------------- */
static void test_fpu_ruling(const char *path)
{
    char want[48];
    int  r;

    CHECK(X87_BOOT_CONTROL_WORD == 0x037Fu,
          "spec/fpu_contract.h: X87_BOOT_CONTROL_WORD == 0x037F (FINIT value; regression guard)");

#if defined(HARDWARE_SPEC_MUTANT) || defined(HARDWARE_SPEC_MUTANT_FPU)
    snprintf(want, sizeof(want), "\"boot_control_word\": \"0x%04X\"", 0x027Fu);
    r = file_contains_substr(path, want, -1);
    CHECK(r == 1,
          "MUTANT: hardware.json: fpu.boot_control_word == 0x027F (should FAIL)");
#else
    snprintf(want, sizeof(want), "\"boot_control_word\": \"0x%04X\"",
             (unsigned)X87_BOOT_CONTROL_WORD);
    r = file_contains_substr(path, want, -1);
    CHECK(r == 1,
          "hardware.json: fpu.boot_control_word == X87_BOOT_CONTROL_WORD (spec/fpu_contract.h)");
#endif

    r = file_contains_substr(path, "\"os_boots_without_fpu\": true", -1);
    CHECK(r == 1, "hardware.json: fpu.os_boots_without_fpu == true (ADR-0001: 386/486SX boot)");

    r = file_contains_substr(path, "\"required_by\": [\"Initech 123\"]", -1);
    CHECK(r == 1, "hardware.json: fpu.required_by == [\"Initech 123\"] (ruling 2026-10-03)");

    r = file_contains_substr(path, "\"samir_numeric\": \"soft-float (ADR-0009 DEC-01, unchanged)\"", -1);
    CHECK(r == 1, "hardware.json: fpu.samir_numeric records SAMIR soft-float (ADR-0009 DEC-01)");

    r = file_contains_substr(path, "initech-zj6w", -1);
    CHECK(r == 1, "hardware.json: beads initech-zj6w cited (Rule-8 deliberate-act anchor)");
}

/* -----------------------------------------------------------------------
 * Test 6: memory window values match spec/memory_map.h.
 * We include memory_map.h (compiled with -Ispec) and check that PROGRAM_BASE
 * and PROGRAM_ALLOC_END, rendered as hex strings, appear in the JSON.
 * This is the key cross-file consistency check: if memory_map.h changes,
 * the gate goes RED until hardware.json is updated.
 * ----------------------------------------------------------------------- */
static void test_memory_window(const char *path)
{
    char base_hex[32];
    char ceil_hex[32];
    int  r;

    /* Render the constants from spec/memory_map.h as lowercase hex strings
     * in the form "0x00030000" (the format used in the JSON). */
    hex_str((unsigned long)PROGRAM_BASE,      base_hex, sizeof(base_hex));
    hex_str((unsigned long)PROGRAM_ALLOC_END, ceil_hex, sizeof(ceil_hex));

    /* PROGRAM_BASE must be 0x00040000 (spec/memory_map.h; raised 0x38000->0x40000
     * by beads initech-re30.2, the SECOND whole-map shift +0x8000 for kernel-window
     * headroom; was 0x38000 per initech-o0td, 0x30000 per initech-5pe). If this
     * regression guard fires, memory_map.h changed without updating this test --
     * it fails loud (Rule 2). */
    /* We use a plain runtime check so the failure is a named CHECK, not a
     * compile-time abort that obscures the spec mismatch. */
    CHECK(PROGRAM_BASE == 0x00040000u,
          "spec/memory_map.h: PROGRAM_BASE == 0x00040000 (regression guard; re30.2)");
    CHECK(PROGRAM_ALLOC_END == 0x00080000u,
          "spec/memory_map.h: PROGRAM_ALLOC_END == 0x00080000 (regression guard; re30.2)");

    /* Convert to lowercase for the JSON search (JSON uses lowercase 0x...): */
    {
        char base_lo[32], ceil_lo[32];
        size_t i;

        /* copy and lowercase */
        for (i = 0; base_hex[i] && i < sizeof(base_lo)-1; i++) {
            base_lo[i] = (char)((base_hex[i] >= 'A' && base_hex[i] <= 'F')
                                ? base_hex[i] - 'A' + 'a'
                                : base_hex[i]);
        }
        base_lo[i] = '\0';

        for (i = 0; ceil_hex[i] && i < sizeof(ceil_lo)-1; i++) {
            ceil_lo[i] = (char)((ceil_hex[i] >= 'A' && ceil_hex[i] <= 'F')
                                ? ceil_hex[i] - 'A' + 'a'
                                : ceil_hex[i]);
        }
        ceil_lo[i] = '\0';

        r = file_contains_substr(path, base_lo, -1);
        CHECK(r == 1,
              "hardware.json: contains PROGRAM_BASE value from spec/memory_map.h");

        r = file_contains_substr(path, ceil_lo, -1);
        CHECK(r == 1,
              "hardware.json: contains PROGRAM_ALLOC_END value from spec/memory_map.h");
    }

    /* Also verify the symbolic names are cited (the spec cites the header, not
     * just raw values): */
    r = file_contains_substr(path, "PROGRAM_BASE", -1);
    CHECK(r == 1, "hardware.json: references symbol PROGRAM_BASE (cites spec/memory_map.h)");

    r = file_contains_substr(path, "PROGRAM_ALLOC_END", -1);
    CHECK(r == 1, "hardware.json: references symbol PROGRAM_ALLOC_END (cites spec/memory_map.h)");
}

/* -----------------------------------------------------------------------
 * Test 7: libgcc provenance hook present (ADR-0009 DEC-02 / Rule 11).
 * The toolchain_reproducibility section must mention libgcc_pin and the
 * soft-float helper set.
 * ----------------------------------------------------------------------- */
static void test_libgcc_provenance(const char *path)
{
    int r;

    r = file_contains_substr(path, "\"libgcc_pin\"", -1);
    CHECK(r == 1, "hardware.json: libgcc_pin key present (ADR-0009 DEC-02)");

    /* The canonical soft-float entry-point names must be cited so the
     * provenance is unambiguous. Check one sentinel from each group. */
    r = file_contains_substr(path, "__adddf3", -1);
    CHECK(r == 1, "hardware.json: __adddf3 soft-float helper cited");

    r = file_contains_substr(path, "__divdi3", -1);
    CHECK(r == 1, "hardware.json: __divdi3 integer helper cited");
}

/* -----------------------------------------------------------------------
 * Test 8: ADR-0009 ratification line and DEC-07 reference present.
 * The file must explicitly cite its mandate (Law 1 / Rule 8 provenance).
 * ----------------------------------------------------------------------- */
static void test_adr_citation(const char *path)
{
    int r;

    r = file_contains_substr(path, "ADR-0009", -1);
    CHECK(r == 1, "hardware.json: ADR-0009 cited (mandate for this file)");

    r = file_contains_substr(path, "DEC-07", -1);
    CHECK(r == 1, "hardware.json: DEC-07 cited (ratification anchor)");

    r = file_contains_substr(path, "PRD Sec 5", -1);
    CHECK(r == 1, "hardware.json: PRD Sec 5 cited (hardware contract section)");
}

/* -----------------------------------------------------------------------
 * Test 9: flair_heap block present and consistent with spec/memory_map.h.
 * ADR-0004 DEC-03 / FO-5 / FO-G: the dedicated extended-memory FLAIR Toolbox
 * heap window + the INT 15h probe + the fail-loud-below-min contract are
 * recorded under memory.flair_heap. The values must match spec/memory_map.h
 * (FLAIR_HEAP_BASE / FLAIR_HEAP_SIZE / FLAIR_HEAP_MIN) exactly, so a Rule-8
 * change to the header that forgets to update the JSON goes RED.
 *
 * MUTATION GATE: with -DHARDWARE_SPEC_MUTANT we assert the WRONG base value
 * (the 2 MiB minority window 0x300000) to prove the gate goes RED.
 * ----------------------------------------------------------------------- */
static void test_flair_heap(const char *path)
{
    int  r;
    char base_lo[32], size_lo[32];
    char tmp[32];
    size_t i;

    /* Compile-time anchors: if memory_map.h changes the FLAIR window without
     * updating this test, the named CHECKs below fail loud (Rule 2). */
    CHECK(FLAIR_HEAP_BASE == 0x00100000u,
          "spec/memory_map.h: FLAIR_HEAP_BASE == 0x00100000 (regression guard)");
    CHECK(FLAIR_HEAP_SIZE == 0x00400000u,
          "spec/memory_map.h: FLAIR_HEAP_SIZE == 0x00400000 (4 MiB; regression guard)");
    CHECK(FLAIR_HEAP_MIN == 0x00400000u,
          "spec/memory_map.h: FLAIR_HEAP_MIN == 0x00400000 (regression guard)");

    /* The block + symbolic references must be present. */
    r = file_contains_substr(path, "\"flair_heap\"", -1);
    CHECK(r == 1, "hardware.json: memory.flair_heap block present (ADR-0004 DEC-03)");

    r = file_contains_substr(path, "FLAIR_HEAP_BASE", -1);
    CHECK(r == 1, "hardware.json: references symbol FLAIR_HEAP_BASE (cites memory_map.h)");

    r = file_contains_substr(path, "FLAIR_HEAP_SIZE", -1);
    CHECK(r == 1, "hardware.json: references symbol FLAIR_HEAP_SIZE");

    r = file_contains_substr(path, "FLAIR_HEAP_MIN", -1);
    CHECK(r == 1, "hardware.json: references symbol FLAIR_HEAP_MIN");

    /* The INT 15h probe contract + the three probe methods. */
    r = file_contains_substr(path, "INT 15h", -1);
    CHECK(r == 1, "hardware.json: flair_heap cites the INT 15h probe interface");

    r = file_contains_substr(path, "ext_mem_kb", -1);
    CHECK(r == 1, "hardware.json: flair_heap cites boot_info_t.ext_mem_kb");

    /* The fail-loud-below-min behavior (FO-G). */
    r = file_contains_substr(path, "fail_loud_below_min", -1);
    CHECK(r == 1, "hardware.json: flair_heap records fail_loud_below_min (FO-G)");

    /* The ADR-0004 DEC-03 provenance must be cited in the block. */
    r = file_contains_substr(path, "DEC-03", -1);
    CHECK(r == 1, "hardware.json: ADR-0004 DEC-03 cited (flair_heap mandate)");

    /* Render FLAIR_HEAP_BASE / FLAIR_HEAP_SIZE as lowercase hex (as the JSON
     * writes them) and confirm they appear -- the byte-value cross-check. */
    hex_str((unsigned long)FLAIR_HEAP_BASE, tmp, sizeof(tmp));
    for (i = 0; tmp[i] && i < sizeof(base_lo) - 1; i++) {
        base_lo[i] = (char)((tmp[i] >= 'A' && tmp[i] <= 'F')
                            ? tmp[i] - 'A' + 'a' : tmp[i]);
    }
    base_lo[i] = '\0';

    hex_str((unsigned long)FLAIR_HEAP_SIZE, tmp, sizeof(tmp));
    for (i = 0; tmp[i] && i < sizeof(size_lo) - 1; i++) {
        size_lo[i] = (char)((tmp[i] >= 'A' && tmp[i] <= 'F')
                            ? tmp[i] - 'A' + 'a' : tmp[i]);
    }
    size_lo[i] = '\0';

#ifndef HARDWARE_SPEC_MUTANT
    r = file_contains_substr(path, base_lo, -1);
    CHECK(r == 1, "hardware.json: flair_heap contains FLAIR_HEAP_BASE value (0x00100000)");

    r = file_contains_substr(path, size_lo, -1);
    CHECK(r == 1, "hardware.json: flair_heap contains FLAIR_HEAP_SIZE value (0x00400000)");
#else
    /* MUTANT: assert the rejected 2 MiB-window base (0x00300000) is present --
     * it is NOT, so the gate goes RED, proving this arm of the oracle bites. */
    r = file_contains_substr(path, "0x00300000", -1);
    CHECK(r == 1,
          "MUTANT: hardware.json: flair_heap base == 0x00300000 (should FAIL)");
#endif
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    const char *path;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <path-to-spec/hardware.json>\n", argv[0]);
        fprintf(stderr, "  e.g. %s spec/hardware.json\n", argv[0]);
        return 1;
    }
    path = argv[1];

#ifndef HARDWARE_SPEC_MUTANT
    fprintf(stdout, "test_hardware_spec: path = %s\n", path);
#else
    fprintf(stdout, "test_hardware_spec (MUTANT): path = %s\n", path);
    fprintf(stdout, "  -- expects failures; gate should exit non-zero\n");
#endif

    test_file_exists_and_ascii(path);
    test_required_keys(path);
    test_cpu_value(path);
    test_fpu_value(path);
    test_fpu_init_by_kernel(path);
    test_fpu_ruling(path);
    test_memory_window(path);
    test_libgcc_provenance(path);
    test_adr_citation(path);
    test_flair_heap(path);

    return TEST_SUMMARY("test_hardware_spec");
}
