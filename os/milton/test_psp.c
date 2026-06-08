/* test_psp.c -- host unit oracle for PSP construction (beads initech-509.4).
 * Factory test: libc OK, reuses seed/test_assert.h. Compiles HOSTED against the
 * REAL artifact psp.c (the same psp_build the kernel runs; psp.c compiles both
 * freestanding and hosted).
 *
 * Ref: docs/research/psp-loader-ground-truth.md Sec 2 (the field-value map +
 *      the Option-B flat-mode handling of the vestigial segment fields);
 *      spec/dos_structs.h lines 91-107 (the LOCKED psp_t offsets);
 *      ADR-0003 DEC-05 / App B.2; os/milton/int21.h (INT21_HANDLE_STDOUT/STDERR
 *      convention). CLAUDE.md Law 2 (oracle is truth), Rule 1 (RED->GREEN),
 *      Rule 6 (mutation-prove), Rule 12 (ASCII).
 *
 * Strategy: build a PSP from KNOWN params into a buffer that has a GUARD byte
 * immediately past offset 0xFF, then assert every documented field and the
 * guard (proving no write past the 256-byte block). Tail clamp is proven with
 * an over-long tail: the dropped count is returned, the count byte saturates at
 * 126, the CR lands at offset 127, and the guard is untouched.
 *
 * MUTATION (Rule 6), driven by make:
 *   -DPSP_MUTATE_INT20         : int20 = CD 21 instead of CD 20 -> the int20
 *                                assertion goes RED.
 *   -DPSP_MUTATE_CMDTAIL_LEN   : the cmd_tail count byte is written off-by-one
 *                                (copy+1) -> the tail-length assertion goes RED.
 * A mutant that PASSES means the oracle is decoration.
 */

#include <stdint.h>
#include <string.h>

#include "psp.h"
#include "test_assert.h"

TEST_HARNESS();

/* A PSP buffer with a trailing GUARD byte. psp_build must never touch guard. */
typedef struct {
    psp_t   psp;
    uint8_t guard;
} guarded_psp_t;

#define GUARD_MAGIC 0xA5u

/* Known, distinct linear params so each fake-paragraph slot is unambiguous:
 *   alloc_end_linear  = 0x00070000 -> alloc_end_seg = 0x7000
 *   env_linear        = 0x00020200 -> env_seg       = 0x2020
 *   parent_psp_linear = 0x00010000 -> parent_psp    = 0x1000
 * (Ref Sec 2.2 / 2.5 / 2.7; Sec 3.2 worked values.) */
#define P_ALLOC_END   0x00070000u
#define P_ENV         0x00020200u
#define P_PARENT      0x00010000u
#define EXP_ALLOC_SEG 0x7000u
#define EXP_ENV_SEG   0x2020u
#define EXP_PARENT    0x1000u

static void run_build(guarded_psp_t *g, const char *tail, uint32_t taillen,
                      uint32_t *dropped_out)
{
    g->guard = GUARD_MAGIC;
    psp_params_t params;
    params.alloc_end_linear  = P_ALLOC_END;
    params.env_linear        = P_ENV;
    params.parent_psp_linear = P_PARENT;
    params.cmd_tail          = tail;
    params.cmd_tail_len      = taillen;
    uint32_t dropped = psp_build(&g->psp, &params);
    if (dropped_out) {
        *dropped_out = dropped;
    }
}

/* Assert the fields that are INDEPENDENT of the command tail. */
static void check_static_fields(const psp_t *p)
{
    /* 00h int20[2] = CD 20. */
    CHECK(p->int20[0] == 0xCD, "int20[0] must be 0xCD (Sec 2.1)");
    CHECK(p->int20[1] == 0x20, "int20[1] must be 0x20 (Sec 2.1)");

    /* 02h alloc_end_seg = flat ceiling >> 4 (Option B). */
    CHECK(p->alloc_end_seg == EXP_ALLOC_SEG,
          "alloc_end_seg must be alloc_end_linear>>4 (Sec 2.2)");

    /* 16h parent_psp = parent linear >> 4 (Option B). */
    CHECK(p->parent_psp == EXP_PARENT,
          "parent_psp must be parent_psp_linear>>4 (Sec 2.5)");

    /* 18h jft: 0x00,0x01,0x01 then 0xFF x17 (Sec 2.6, aligned to int21.h). */
    CHECK(p->jft[0] == 0x00, "jft[0] (stdin) must be 0x00 (Sec 2.6)");
    CHECK(p->jft[1] == 0x01, "jft[1] (stdout) must be 0x01 (Sec 2.6)");
    CHECK(p->jft[2] == 0x01, "jft[2] (stderr) must be 0x01 (Sec 2.6)");
    int jft_unused_ok = 1;
    for (int i = 3; i < 20; i++) {
        if (p->jft[i] != 0xFF) {
            jft_unused_ok = 0;
        }
    }
    CHECK(jft_unused_ok, "jft[3..19] must all be 0xFF (unused; Sec 2.6)");

    /* 2Ch env_seg = env linear >> 4 (Option B). */
    CHECK(p->env_seg == EXP_ENV_SEG,
          "env_seg must be env_linear>>4 (Sec 2.7)");

    /* 50h int21_entry = CD 21 CB then zero (Sec 2.9). */
    CHECK(p->int21_entry[0] == 0xCD, "int21_entry[0] must be 0xCD (Sec 2.9)");
    CHECK(p->int21_entry[1] == 0x21, "int21_entry[1] must be 0x21 (Sec 2.9)");
    CHECK(p->int21_entry[2] == 0xCB, "int21_entry[2] must be 0xCB (RETF; Sec 2.9)");
    int i21_tail_zero = 1;
    for (int i = 3; i < 12; i++) {
        if (p->int21_entry[i] != 0x00) {
            i21_tail_zero = 0;
        }
    }
    CHECK(i21_tail_zero, "int21_entry[3..11] must be zero (Sec 2.9)");

    /* Deferred/reserved regions must be zero (Sec 2.3/2.4/2.8/2.10). */
    int z;
    z = 1; for (int i = 0; i < 6;  i++) if (p->reserved_04[i])    z = 0;
    CHECK(z, "reserved_04[6] must be zero (Sec 2.3)");
    z = 1; for (int i = 0; i < 12; i++) if (p->saved_vectors[i])  z = 0;
    CHECK(z, "saved_vectors[12] must be zero (deferred 509.8; Sec 2.4)");
    z = 1; for (int i = 0; i < 34; i++) if (p->reserved_2e[i])    z = 0;
    CHECK(z, "reserved_2e[34] must be zero (Sec 2.8)");
    z = 1; for (int i = 0; i < 16; i++) if (p->reserved_5c[i])    z = 0;
    CHECK(z, "FCB#1 reserved_5c[16] must be zero (deferred 509.9; Sec 2.10)");
    z = 1; for (int i = 0; i < 20; i++) if (p->reserved_6c[i])    z = 0;
    CHECK(z, "FCB#2 reserved_6c[20] must be zero (deferred 509.9; Sec 2.10)");
}

int main(void)
{
    /* Belt-and-suspenders with the spec _Static_assert (dos_structs.h:107). */
    CHECK(sizeof(psp_t) == 256, "sizeof(psp_t) must be 256 (App B.2)");

    /* ---- Case 1: command tail "ABC" --------------------------------------- */
    {
        guarded_psp_t g;
        uint32_t dropped = 999;
        run_build(&g, "ABC", 3, &dropped);
        const psp_t *p = &g.psp;
        check_static_fields(p);
        CHECK(dropped == 0, "tail 'ABC' must not be clamped (dropped==0)");
        /* 80h cmd_tail: count=3, "ABC", CR at offset 4 (Sec 2.11). */
        CHECK(p->cmd_tail[0] == 3,   "cmd_tail[0] count must be 3 for 'ABC'");
        CHECK(p->cmd_tail[1] == 'A', "cmd_tail[1] must be 'A'");
        CHECK(p->cmd_tail[2] == 'B', "cmd_tail[2] must be 'B'");
        CHECK(p->cmd_tail[3] == 'C', "cmd_tail[3] must be 'C'");
        CHECK(p->cmd_tail[4] == 0x0D, "cmd_tail[4] must be CR (0x0D)");
        int rest_zero = 1;
        for (int i = 5; i < 128; i++) {
            if (p->cmd_tail[i] != 0x00) rest_zero = 0;
        }
        CHECK(rest_zero, "cmd_tail[5..127] must be zero for 'ABC'");
        CHECK(g.guard == GUARD_MAGIC, "guard byte must be untouched (no overflow)");
    }

    /* ---- Case 2: empty command tail (no-argument launch) ------------------ */
    {
        guarded_psp_t g;
        uint32_t dropped = 999;
        run_build(&g, "", 0, &dropped);
        const psp_t *p = &g.psp;
        check_static_fields(p);
        CHECK(dropped == 0, "empty tail must not be clamped (dropped==0)");
        /* No-args: count=0, CR at offset 1 (Sec 2.11). */
        CHECK(p->cmd_tail[0] == 0x00, "empty tail: cmd_tail[0] count must be 0");
        CHECK(p->cmd_tail[1] == 0x0D, "empty tail: cmd_tail[1] must be CR (0x0D)");
        int rest_zero = 1;
        for (int i = 2; i < 128; i++) {
            if (p->cmd_tail[i] != 0x00) rest_zero = 0;
        }
        CHECK(rest_zero, "empty tail: cmd_tail[2..127] must be zero");
        CHECK(g.guard == GUARD_MAGIC, "guard byte must be untouched (no overflow)");
    }

    /* ---- Case 3: NULL command tail with zero length (also no-args) -------- */
    {
        guarded_psp_t g;
        uint32_t dropped = 999;
        run_build(&g, NULL, 0, &dropped);
        const psp_t *p = &g.psp;
        CHECK(dropped == 0, "NULL/0 tail must not be clamped");
        CHECK(p->cmd_tail[0] == 0x00, "NULL/0 tail: count must be 0");
        CHECK(p->cmd_tail[1] == 0x0D, "NULL/0 tail: cmd_tail[1] must be CR");
        CHECK(g.guard == GUARD_MAGIC, "guard byte must be untouched");
    }

    /* ---- Case 4: over-long tail -> clamp, no overflow past 0xFF ----------- */
    {
        /* 200 chars of 'X' -- exceeds PSP_CMD_TAIL_MAX_TEXT (126). */
        char big[201];
        for (int i = 0; i < 200; i++) big[i] = 'X';
        big[200] = '\0';
        guarded_psp_t g;
        uint32_t dropped = 0;
        run_build(&g, big, 200, &dropped);
        const psp_t *p = &g.psp;
        /* Dropped = 200 - 126 = 74. */
        CHECK(dropped == (200u - PSP_CMD_TAIL_MAX_TEXT),
              "over-long tail must report dropped == len - 126 (loud clamp)");
        CHECK(p->cmd_tail[0] == PSP_CMD_TAIL_MAX_TEXT,
              "clamped count byte must saturate at 126");
        /* Text occupies offsets 1..126; CR at offset 127. */
        CHECK(p->cmd_tail[1]   == 'X', "clamped tail: first text byte 'X'");
        CHECK(p->cmd_tail[126] == 'X', "clamped tail: last text byte at off 126");
        CHECK(p->cmd_tail[127] == 0x0D,
              "clamped tail: CR must land at offset 127 (last byte of region)");
        CHECK(g.guard == GUARD_MAGIC,
              "guard byte must be untouched -- NO write past offset 0xFF");
        /* The static fields must still be correct after a clamped tail. */
        check_static_fields(p);
    }

    return TEST_SUMMARY("test_psp");
}
