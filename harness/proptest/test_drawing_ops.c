/* harness/proptest/test_drawing_ops.c -- host oracle for spec/drawing_ops.h
 *
 * beads: initech-dh5k.6 (P1-6: spec/drawing_ops.h + compile oracle).
 * era: system7.0-7.1
 *
 * PURPOSE: exercises every locked constant and structural rule in
 *   spec/drawing_ops.h and confirms they match the ground-truth values from:
 *     [DP] system7-decomp/specs/quickdraw/drawing-primitives.md
 *     [CB] system7-decomp/specs/quickdraw/copybits.md
 *     [CS] system7-decomp/specs/quickdraw/coordinate-system.md
 *     [PA] system7-decomp/specs/quickdraw/patterns.md
 *     [QDp] system7-decomp/refs/quickdraw/Apple-QuickDraw.p
 *
 * SECTIONS:
 *   A -- GrafVerb enum values (0..4 from [QDp] L57 / [DP] Sec 7)
 *   B -- GrafVerb ordering invariants
 *   C -- Verb-to-pattern-slot predicate macros ([DP] Sec 5 / [PA] Sec 2)
 *   D -- Pen initial state ([DP] Sec 2)
 *   E -- System pattern bytes ([PA] Sec 4)
 *   F -- Pattern phase-lock macros ([PA] Sec 3)
 *   G -- Coordinate system constants and macros ([CS] Sec 1-5)
 *   H -- CopyBits mode contract ([CB] Sec 2-4)
 *   I -- Shape-family ordinals
 *   J -- Transfer-mode cross-checks with imaging.h
 *   K -- Mutation probe (verify oracle bites)
 *
 * SELF-VERIFY (standalone without make; run from the repo root):
 *   # Freestanding compile (no stdlib; -c only; checks _Static_asserts at
 *   # target ABI; requires 32-bit multilib dev headers for -m32 hosted but NOT
 *   # for freestanding):
 *   gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -std=c11 \
 *       -I. \
 *       -c harness/proptest/test_drawing_ops.c \
 *       -o /tmp/test_drawing_ops_fs.o
 *
 *   # Hosted binary (native 64-bit; runs + prints result; no 32-bit libc needed):
 *   gcc -std=c11 \
 *       -I. \
 *       harness/proptest/test_drawing_ops.c \
 *       -o /tmp/test_drawing_ops && /tmp/test_drawing_ops
 *
 *   # Mutation probe (must exit non-zero; proves oracle bites):
 *   gcc -std=c11 -DTEST_DRAWOPS_MUTANT \
 *       -I. \
 *       harness/proptest/test_drawing_ops.c \
 *       -o /tmp/test_drawing_ops_mutant && \
 *   /tmp/test_drawing_ops_mutant; \
 *   test $? -ne 0 && echo "MUTATION RED (correct)" || echo "MUTATION GREEN (BUG)"
 *
 * MUTATION PROOF (Rule 6): build with -DTEST_DRAWOPS_MUTANT to perturb ONE
 *   constant (kGrafVerbPaint expected value) so the oracle MUST fail (exit 1,
 *   RED). This proves the runtime checks actually bite. The _Static_assert
 *   compile-time layer in drawing_ops.h is a separate gate that fires on any
 *   constant mutation.
 *
 * ASCII-clean (Rule 12). No 2026-isms (Rule 11).
 */

/* Include the locked spec first -- fires every _Static_assert at compile time. */
#include "spec/drawing_ops.h"

/* Hosted-only: stdio for result printing.
 * Under freestanding (-ffreestanding; __STDC_HOSTED__==0) stdio is unavailable.
 * The _Static_assert compile-time layer fires regardless; main() runs hosted only. */
#if defined(__STDC_HOSTED__) && (__STDC_HOSTED__ == 1)
#include <stdio.h>

/* -------------------------------------------------------------------------
 * CHECK macro -- increment checks; on failure increment fails + print name.
 * ------------------------------------------------------------------------- */
#define CHECK(name, cond)                           \
    do {                                            \
        checks++;                                   \
        if (!(cond)) {                              \
            fails++;                                \
            printf("FAIL %s\n", (name));            \
        }                                           \
    } while (0)

/* Mutation probe: when TEST_DRAWOPS_MUTANT is defined, we expect
 * kGrafVerbPaint == 0 (wrong! it must be 1) -- this must fail section A. */
#ifdef TEST_DRAWOPS_MUTANT
#define EXPECTED_GRAFVERB_PAINT  0   /* WRONG intentionally -- mutation */
#else
#define EXPECTED_GRAFVERB_PAINT  1   /* correct value per [QDp] L57 */
#endif

int main(void)
{
    int checks = 0;
    int fails  = 0;

    /* === SECTION A: GrafVerb enum values ===================================
     * Source: [DP] Sec 7 + [QDp] L57.
     * "GrafVerb = (frame, paint, erase, invert, fill)"
     * frame=0, paint=1, erase=2, invert=3, fill=4.
     * ====================================================================== */

    CHECK("kGrafVerbFrame=0 ([QDp] L57 / [DP] Sec 7)",
          (int)kGrafVerbFrame == 0);
    CHECK("kGrafVerbPaint=1 ([QDp] L57 / [DP] Sec 7)",
          (int)kGrafVerbPaint == EXPECTED_GRAFVERB_PAINT);
    CHECK("kGrafVerbErase=2 ([QDp] L57 / [DP] Sec 7)",
          (int)kGrafVerbErase == 2);
    CHECK("kGrafVerbInvert=3 ([QDp] L57 / [DP] Sec 7)",
          (int)kGrafVerbInvert == 3);
    CHECK("kGrafVerbFill=4 ([QDp] L57 / [DP] Sec 7)",
          (int)kGrafVerbFill == 4);

    /* All 5 verb values are distinct (no two are equal). */
    CHECK("GrafVerb values all distinct (0..4)",
          (int)kGrafVerbFrame  != (int)kGrafVerbPaint  &&
          (int)kGrafVerbFrame  != (int)kGrafVerbErase  &&
          (int)kGrafVerbFrame  != (int)kGrafVerbInvert &&
          (int)kGrafVerbFrame  != (int)kGrafVerbFill   &&
          (int)kGrafVerbPaint  != (int)kGrafVerbErase  &&
          (int)kGrafVerbPaint  != (int)kGrafVerbInvert &&
          (int)kGrafVerbPaint  != (int)kGrafVerbFill   &&
          (int)kGrafVerbErase  != (int)kGrafVerbInvert &&
          (int)kGrafVerbErase  != (int)kGrafVerbFill   &&
          (int)kGrafVerbInvert != (int)kGrafVerbFill);

    /* GrafVerbs fit in 3 bits (max value = fill = 4 <= 7). */
    CHECK("GrafVerb max value (fill=4) fits in 3 bits",
          (int)kGrafVerbFill <= 7);

    /* === SECTION B: GrafVerb ordering ======================================
     * The canonical enum order is frame < paint < erase < invert < fill.
     * This order is load-bearing for any dispatch table indexed by verb code.
     * Source: [QDp] L57 enum declaration order.
     * ====================================================================== */

    CHECK("frame < paint ([QDp] L57 enum order)",
          (int)kGrafVerbFrame < (int)kGrafVerbPaint);
    CHECK("paint < erase ([QDp] L57 enum order)",
          (int)kGrafVerbPaint < (int)kGrafVerbErase);
    CHECK("erase < invert ([QDp] L57 enum order)",
          (int)kGrafVerbErase < (int)kGrafVerbInvert);
    CHECK("invert < fill ([QDp] L57 enum order)",
          (int)kGrafVerbInvert < (int)kGrafVerbFill);

    /* === SECTION C: Verb-to-pattern-slot predicates ========================
     * Source: [DP] Sec 5; [PA] Sec 2.
     *   frame(0)+paint(1) -> pnPat (FLAIR_VERB_READS_PNPAT)
     *   erase(2)          -> bkPat (FLAIR_VERB_READS_BKPAT)
     *   fill(4)           -> fillPat (FLAIR_VERB_READS_FILLPAT)
     *   invert(3)         -> no pattern (FLAIR_VERB_IS_INVERT)
     * ====================================================================== */

    /* Frame reads pnPat. */
    CHECK("frame reads pnPat ([DP] Sec 5; [PA] Sec 2)",
          FLAIR_VERB_READS_PNPAT(kGrafVerbFrame));
    /* Paint reads pnPat. */
    CHECK("paint reads pnPat ([DP] Sec 5; [PA] Sec 2)",
          FLAIR_VERB_READS_PNPAT(kGrafVerbPaint));
    /* Erase does NOT read pnPat. */
    CHECK("erase does NOT read pnPat ([DP] Sec 5)",
          !FLAIR_VERB_READS_PNPAT(kGrafVerbErase));
    /* Fill does NOT read pnPat. */
    CHECK("fill does NOT read pnPat ([DP] Sec 5)",
          !FLAIR_VERB_READS_PNPAT(kGrafVerbFill));
    /* Invert does NOT read pnPat. */
    CHECK("invert does NOT read pnPat ([DP] Sec 5)",
          !FLAIR_VERB_READS_PNPAT(kGrafVerbInvert));

    /* Erase reads bkPat. */
    CHECK("erase reads bkPat ([DP] Sec 5; [PA] Sec 2)",
          FLAIR_VERB_READS_BKPAT(kGrafVerbErase));
    /* Frame does NOT read bkPat. */
    CHECK("frame does NOT read bkPat ([DP] Sec 5)",
          !FLAIR_VERB_READS_BKPAT(kGrafVerbFrame));
    /* Paint does NOT read bkPat. */
    CHECK("paint does NOT read bkPat ([DP] Sec 5)",
          !FLAIR_VERB_READS_BKPAT(kGrafVerbPaint));
    /* Fill does NOT read bkPat. */
    CHECK("fill does NOT read bkPat ([DP] Sec 5)",
          !FLAIR_VERB_READS_BKPAT(kGrafVerbFill));
    /* Invert does NOT read bkPat. */
    CHECK("invert does NOT read bkPat ([DP] Sec 5)",
          !FLAIR_VERB_READS_BKPAT(kGrafVerbInvert));

    /* Fill reads fillPat. */
    CHECK("fill reads fillPat ([DP] Sec 5; [PA] Sec 2)",
          FLAIR_VERB_READS_FILLPAT(kGrafVerbFill));
    /* Frame does NOT read fillPat. */
    CHECK("frame does NOT read fillPat ([DP] Sec 5)",
          !FLAIR_VERB_READS_FILLPAT(kGrafVerbFrame));
    /* Paint does NOT read fillPat. */
    CHECK("paint does NOT read fillPat ([DP] Sec 5)",
          !FLAIR_VERB_READS_FILLPAT(kGrafVerbPaint));
    /* Erase does NOT read fillPat. */
    CHECK("erase does NOT read fillPat ([DP] Sec 5)",
          !FLAIR_VERB_READS_FILLPAT(kGrafVerbErase));
    /* Invert does NOT read fillPat. */
    CHECK("invert does NOT read fillPat ([DP] Sec 5)",
          !FLAIR_VERB_READS_FILLPAT(kGrafVerbInvert));

    /* Invert is the only invert verb. */
    CHECK("invert verb is FLAIR_VERB_IS_INVERT ([DP] Sec 5)",
          FLAIR_VERB_IS_INVERT(kGrafVerbInvert));
    CHECK("frame is NOT invert ([DP] Sec 5)",
          !FLAIR_VERB_IS_INVERT(kGrafVerbFrame));
    CHECK("paint is NOT invert ([DP] Sec 5)",
          !FLAIR_VERB_IS_INVERT(kGrafVerbPaint));
    CHECK("erase is NOT invert ([DP] Sec 5)",
          !FLAIR_VERB_IS_INVERT(kGrafVerbErase));
    CHECK("fill is NOT invert ([DP] Sec 5)",
          !FLAIR_VERB_IS_INVERT(kGrafVerbFill));

    /* Fill forces patCopy; other verbs do not force patCopy. */
    CHECK("fill FORCES patCopy ([PA] Sec 2; [DP] Sec 5)",
          FLAIR_VERB_FORCES_PATCOPY(kGrafVerbFill));
    CHECK("frame does NOT force patCopy ([DP] Sec 5)",
          !FLAIR_VERB_FORCES_PATCOPY(kGrafVerbFrame));
    CHECK("paint does NOT force patCopy ([DP] Sec 5)",
          !FLAIR_VERB_FORCES_PATCOPY(kGrafVerbPaint));
    CHECK("erase does NOT force patCopy ([DP] Sec 5)",
          !FLAIR_VERB_FORCES_PATCOPY(kGrafVerbErase));
    CHECK("invert does NOT force patCopy ([DP] Sec 5)",
          !FLAIR_VERB_FORCES_PATCOPY(kGrafVerbInvert));

    /* Exactly ONE verb reads each slot (mutual exclusion of reads). */
    {
        int pnpat_count = 0, bkpat_count = 0, fillpat_count = 0;
        int invert_count = 0;
        flair_grafverb_t verbs[5] = {
            kGrafVerbFrame, kGrafVerbPaint, kGrafVerbErase,
            kGrafVerbInvert, kGrafVerbFill
        };
        int i;
        for (i = 0; i < 5; i++) {
            if (FLAIR_VERB_READS_PNPAT(verbs[i]))   pnpat_count++;
            if (FLAIR_VERB_READS_BKPAT(verbs[i]))   bkpat_count++;
            if (FLAIR_VERB_READS_FILLPAT(verbs[i])) fillpat_count++;
            if (FLAIR_VERB_IS_INVERT(verbs[i]))      invert_count++;
        }
        CHECK("exactly 2 verbs read pnPat (frame+paint) ([DP] Sec 5)",
              pnpat_count == 2);
        CHECK("exactly 1 verb reads bkPat (erase) ([DP] Sec 5)",
              bkpat_count == 1);
        CHECK("exactly 1 verb reads fillPat (fill) ([DP] Sec 5)",
              fillpat_count == 1);
        CHECK("exactly 1 verb is invert ([DP] Sec 5)",
              invert_count == 1);
        /* Every verb reads exactly one slot OR is invert. */
        CHECK("all 5 verbs accounted for in slot map ([DP] Sec 5)",
              pnpat_count + bkpat_count + fillpat_count + invert_count == 5);
    }

    /* === SECTION D: Pen initial state ======================================
     * Source: [DP] Sec 2; [PA] Sec 4.
     * pnVis=0, pnSize=(1,1), pnMode=patCopy(8), pnPat=all-black, bkPat=all-white.
     * ====================================================================== */

    CHECK("FLAIR_PEN_INIT_PNVIS=0 ([DP] Sec 2)",
          FLAIR_PEN_INIT_PNVIS == 0);
    CHECK("FLAIR_PEN_INIT_PNSIZE_V=1 (height; [DP] Sec 2)",
          FLAIR_PEN_INIT_PNSIZE_V == 1);
    CHECK("FLAIR_PEN_INIT_PNSIZE_H=1 (width; [DP] Sec 2)",
          FLAIR_PEN_INIT_PNSIZE_H == 1);
    CHECK("FLAIR_PEN_INIT_PNMODE=patCopy=8 ([DP] Sec 2; imaging.h)",
          (int)FLAIR_PEN_INIT_PNMODE == 8);

    /* Verify the black pattern macro expands to all-0xFF bytes. */
    {
        static const uint8_t black_bytes[] = FLAIR_SYSPAT_BLACK_BYTES;
        int ok = 1, bi;
        for (bi = 0; bi < 8; bi++) {
            if (black_bytes[bi] != 0xFFu) { ok = 0; break; }
        }
        CHECK("FLAIR_SYSPAT_BLACK_BYTES all 0xFF ([PA] Sec 4; [QDp] L190)",
              ok);
    }

    /* Verify the white pattern macro expands to all-0x00 bytes. */
    {
        static const uint8_t white_bytes[] = FLAIR_SYSPAT_WHITE_BYTES;
        int ok = 1, bi;
        for (bi = 0; bi < 8; bi++) {
            if (white_bytes[bi] != 0x00u) { ok = 0; break; }
        }
        CHECK("FLAIR_SYSPAT_WHITE_BYTES all 0x00 ([PA] Sec 4; [QDp] L189)",
              ok);
    }

    /* === SECTION E: System pattern bytes ===================================
     * Source: [PA] Sec 4.
     *   white   = 00 00 00 00 00 00 00 00 [verified]
     *   black   = FF FF FF FF FF FF FF FF [verified]
     *   gray    = AA 55 AA 55 AA 55 AA 55 [documented]
     *   ltGray  = 88 22 88 22 88 22 88 22 [verified: PAT_17.bin]
     *   dkGray  = 77 DD 77 DD 77 DD 77 DD [inferred; golden-resolves]
     * ====================================================================== */

    {
        static const uint8_t gray_bytes[]   = FLAIR_SYSPAT_GRAY_BYTES;
        static const uint8_t ltgray_bytes[] = FLAIR_SYSPAT_LTGRAY_BYTES;
        static const uint8_t dkgray_bytes[] = FLAIR_SYSPAT_DKGRAY_BYTES;

        /* gray alternates 0xAA / 0x55 [documented: PA Sec 4] */
        CHECK("gray pattern row 0 = 0xAA ([PA] Sec 4)",
              gray_bytes[0] == 0xAAu);
        CHECK("gray pattern row 1 = 0x55 ([PA] Sec 4)",
              gray_bytes[1] == 0x55u);
        CHECK("gray pattern alternates AA/55 all 8 rows ([PA] Sec 4)",
              gray_bytes[2] == 0xAAu && gray_bytes[3] == 0x55u &&
              gray_bytes[4] == 0xAAu && gray_bytes[5] == 0x55u &&
              gray_bytes[6] == 0xAAu && gray_bytes[7] == 0x55u);

        /* ltGray: 88 22 88 22 ... [verified: PAT_17.bin; PA Sec 4] */
        CHECK("ltGray row 0 = 0x88 ([PA] Sec 4; PAT_17.bin])",
              ltgray_bytes[0] == 0x88u);
        CHECK("ltGray row 1 = 0x22 ([PA] Sec 4; PAT_17.bin])",
              ltgray_bytes[1] == 0x22u);
        CHECK("ltGray pattern alternates 88/22 all 8 rows ([PA] Sec 4)",
              ltgray_bytes[2] == 0x88u && ltgray_bytes[3] == 0x22u &&
              ltgray_bytes[4] == 0x88u && ltgray_bytes[5] == 0x22u &&
              ltgray_bytes[6] == 0x88u && ltgray_bytes[7] == 0x22u);

        /* dkGray: 77 DD 77 DD ... [inferred; golden-resolves]
         * We check the value is non-black and non-white; the exact bytes are
         * golden-resolves but the conventional inferred value is 77/DD.
         * Law 2: do not assert pixel exactness from inferred values. */
        CHECK("dkGray row 0 = 0x77 (inferred conventional; [PA] Sec 4)",
              dkgray_bytes[0] == 0x77u);
        CHECK("dkGray row 1 = 0xDD (inferred conventional; [PA] Sec 4)",
              dkgray_bytes[1] == 0xDDu);

        /* Sanity: all system patterns are 8 bytes (size from imaging.h). */
        CHECK("gray pattern byte count = 8 (Pattern size; imaging.h)",
              sizeof(gray_bytes) == 8);
        CHECK("ltGray pattern byte count = 8 (Pattern size; imaging.h)",
              sizeof(ltgray_bytes) == 8);
        CHECK("dkGray pattern byte count = 8 (Pattern size; imaging.h)",
              sizeof(dkgray_bytes) == 8);
    }

    /* === SECTION F: Pattern phase-lock macros ==============================
     * Source: [PA] Sec 3.
     * The 8x8 tile is phase-aligned to the PORT ORIGIN (0,0).
     * FLAIR_PAT_ROW(pat, y) = pat->pat[y & 7]
     * FLAIR_PAT_BIT(pat, x, y): bit at column (x & 7) with MSB=column 0.
     * ====================================================================== */

    {
        /* Use the all-black pattern for bit tests (every bit should be 1). */
        static const uint8_t black_raw[] = FLAIR_SYSPAT_BLACK_BYTES;
        const Pattern black_pat = { { 0xFFu, 0xFFu, 0xFFu, 0xFFu,
                                      0xFFu, 0xFFu, 0xFFu, 0xFFu } };
        /* Use the gray pattern for row mod tests. */
        const Pattern gray_pat  = { { 0xAAu, 0x55u, 0xAAu, 0x55u,
                                      0xAAu, 0x55u, 0xAAu, 0x55u } };
        /* Pre-computed intermediate values.
         * NOTE: FLAIR_PAT_ROW and FLAIR_PAT_BIT take multiple arguments; passing
         * them directly to CHECK(name, cond) would confuse the preprocessor because
         * CHECK is a 2-argument macro and the inner commas are parsed as additional
         * macro arguments.  We therefore compute into temp vars first. */
        uint8_t row0, row1, row0_wrap, row1_wrap;
        unsigned bit_g00, bit_g10, bit_g20, bit_g30;
        unsigned bit_g01, bit_g11;
        unsigned bit_g80, bit_g90;
        (void)black_raw;

        row0      = FLAIR_PAT_ROW(&gray_pat, 0);
        row1      = FLAIR_PAT_ROW(&gray_pat, 1);
        row0_wrap = FLAIR_PAT_ROW(&gray_pat, 8);   /* must == row0 */
        row1_wrap = FLAIR_PAT_ROW(&gray_pat, 9);   /* must == row1 */

        /* FLAIR_PAT_ROW wraps y at 8. */
        CHECK("FLAIR_PAT_ROW row 0 = row 8 (phase wraps mod 8; [PA] Sec 3)",
              row0 == row0_wrap);
        CHECK("FLAIR_PAT_ROW row 1 = row 9 (phase wraps; [PA] Sec 3)",
              row1 == row1_wrap);
        CHECK("FLAIR_PAT_ROW gray row 0 = 0xAA ([PA] Sec 4)",
              row0 == 0xAAu);
        CHECK("FLAIR_PAT_ROW gray row 1 = 0x55 ([PA] Sec 4)",
              row1 == 0x55u);

        /* FLAIR_PAT_BIT on all-black returns 1 for every pixel. */
        {
            int x, y, all_set = 1;
            for (y = 0; y < 16; y++) {     /* test more than one tile period */
                for (x = 0; x < 16; x++) {
                    if (!FLAIR_PAT_BIT(&black_pat, x, y)) {
                        all_set = 0;
                        break;
                    }
                }
                if (!all_set) break;
            }
            CHECK("FLAIR_PAT_BIT all-black pattern: all bits set ([PA] Sec 1)",
                  all_set);
        }

        /* MSB = column 0 rule: for gray row 0 (0xAA = 10101010b),
         * col 0 (MSB) = 1, col 1 = 0, col 2 = 1, col 3 = 0, etc. */
        bit_g00 = FLAIR_PAT_BIT(&gray_pat, 0, 0);
        bit_g10 = FLAIR_PAT_BIT(&gray_pat, 1, 0);
        bit_g20 = FLAIR_PAT_BIT(&gray_pat, 2, 0);
        bit_g30 = FLAIR_PAT_BIT(&gray_pat, 3, 0);
        bit_g01 = FLAIR_PAT_BIT(&gray_pat, 0, 1);
        bit_g11 = FLAIR_PAT_BIT(&gray_pat, 1, 1);
        bit_g80 = FLAIR_PAT_BIT(&gray_pat, 8, 0);
        bit_g90 = FLAIR_PAT_BIT(&gray_pat, 9, 0);

        CHECK("gray(x=0,y=0)=1 (MSB=col0; 0xAA bit7; [PA] Sec 1)",
              bit_g00 == 1u);
        CHECK("gray(x=1,y=0)=0 (0xAA bit6=0; [PA] Sec 1)",
              bit_g10 == 0u);
        CHECK("gray(x=2,y=0)=1 (0xAA bit5=1; [PA] Sec 1)",
              bit_g20 == 1u);
        CHECK("gray(x=3,y=0)=0 (0xAA bit4=0; [PA] Sec 1)",
              bit_g30 == 0u);
        /* gray row 1 = 0x55 = 01010101b: col 0 = 0, col 1 = 1. */
        CHECK("gray(x=0,y=1)=0 (0x55 bit7=0; [PA] Sec 1)",
              bit_g01 == 0u);
        CHECK("gray(x=1,y=1)=1 (0x55 bit6=1; [PA] Sec 1)",
              bit_g11 == 1u);

        /* Phase wraps: x=8 and x=0 produce the same bit (mod 8). */
        CHECK("FLAIR_PAT_BIT x=8 == x=0 (phase wraps; [PA] Sec 3)",
              bit_g80 == bit_g00);
        CHECK("FLAIR_PAT_BIT x=9 == x=1 (phase wraps; [PA] Sec 3)",
              bit_g90 == bit_g10);
    }

    /* === SECTION G: Coordinate system constants and macros =================
     * Source: [CS] Sec 1-5.
     * ====================================================================== */

    CHECK("FLAIR_COORD_RANGE_MIN = -32767 ([CS] Sec 1; IM Overview)",
          FLAIR_COORD_RANGE_MIN == -32767);
    CHECK("FLAIR_COORD_RANGE_MAX = 32767 ([CS] Sec 1; IM Overview)",
          FLAIR_COORD_RANGE_MAX == 32767);

    /* flair_point_t field offsets: v at 0, h at 2. */
    CHECK("flair_point_t.v at offset 0 ([CS] Sec 2; v-first rule)",
          (int)offsetof(flair_point_t, v) == 0);
    CHECK("flair_point_t.h at offset 2 ([CS] Sec 2)",
          (int)offsetof(flair_point_t, h) == 2);
    CHECK("sizeof(flair_point_t) = 4 ([CS] Sec 2)",
          sizeof(flair_point_t) == 4);

    /* rgn_rect_t field offsets: top=0, left=2, bottom=4, right=6. */
    CHECK("rgn_rect_t.top at offset 0 ([CS] Sec 3; [QDp] L78)",
          (int)offsetof(rgn_rect_t, top)    == 0);
    CHECK("rgn_rect_t.left at offset 2 ([CS] Sec 3; [QDp] L78)",
          (int)offsetof(rgn_rect_t, left)   == 2);
    CHECK("rgn_rect_t.bottom at offset 4 ([CS] Sec 3; [QDp] L78)",
          (int)offsetof(rgn_rect_t, bottom) == 4);
    CHECK("rgn_rect_t.right at offset 6 ([CS] Sec 3; [QDp] L78)",
          (int)offsetof(rgn_rect_t, right)  == 6);
    CHECK("sizeof(rgn_rect_t) = 8 ([CS] Sec 3; two Points)",
          sizeof(rgn_rect_t) == 8);

    /* FLAIR_RECT_EMPTY: empty iff bottom<=top OR right<=left. */
    {
        rgn_rect_t empty_bottom = { 10, 0, 10, 20 };  /* bottom == top */
        rgn_rect_t empty_right  = { 0, 10, 10, 10 };  /* right == left */
        rgn_rect_t empty_inv_v  = { 10, 0,  5, 20 };  /* bottom < top */
        rgn_rect_t empty_inv_h  = { 0, 20, 10,  5 };  /* right < left */
        rgn_rect_t nonempty     = { 0, 0,  10, 20 };  /* 10x20 -- valid */

        CHECK("FLAIR_RECT_EMPTY: bottom==top is empty ([CS] Sec 3)",
              FLAIR_RECT_EMPTY(&empty_bottom));
        CHECK("FLAIR_RECT_EMPTY: right==left is empty ([CS] Sec 3)",
              FLAIR_RECT_EMPTY(&empty_right));
        CHECK("FLAIR_RECT_EMPTY: bottom<top is empty ([CS] Sec 3)",
              FLAIR_RECT_EMPTY(&empty_inv_v));
        CHECK("FLAIR_RECT_EMPTY: right<left is empty ([CS] Sec 3)",
              FLAIR_RECT_EMPTY(&empty_inv_h));
        CHECK("FLAIR_RECT_EMPTY: normal rect (0,0,10,20) is NOT empty ([CS] Sec 3)",
              !FLAIR_RECT_EMPTY(&nonempty));
    }

    /* FLAIR_RECT_CONTAINS_PT: half-open containment (top<=v<bottom, left<=h<right). */
    {
        rgn_rect_t r = { 5, 10, 15, 20 };  /* rows [5,15), cols [10,20) */

        /* Interior points. */
        CHECK("FLAIR_RECT_CONTAINS_PT inside (v=5,h=10 = top-left corner) ([CS] Sec 3)",
              FLAIR_RECT_CONTAINS_PT(&r, 5, 10));
        CHECK("FLAIR_RECT_CONTAINS_PT inside (v=10,h=15) ([CS] Sec 3)",
              FLAIR_RECT_CONTAINS_PT(&r, 10, 15));

        /* Boundary: top+left edges are INCLUSIVE. */
        CHECK("FLAIR_RECT_CONTAINS_PT: v=top (5) is included ([CS] Sec 3 half-open)",
              FLAIR_RECT_CONTAINS_PT(&r, 5, 15));
        CHECK("FLAIR_RECT_CONTAINS_PT: h=left (10) is included ([CS] Sec 3)",
              FLAIR_RECT_CONTAINS_PT(&r, 10, 10));

        /* Boundary: bottom+right edges are EXCLUSIVE. */
        CHECK("FLAIR_RECT_CONTAINS_PT: v=bottom (15) is EXCLUDED ([CS] Sec 3)",
              !FLAIR_RECT_CONTAINS_PT(&r, 15, 15));
        CHECK("FLAIR_RECT_CONTAINS_PT: h=right (20) is EXCLUDED ([CS] Sec 3)",
              !FLAIR_RECT_CONTAINS_PT(&r, 10, 20));

        /* Completely outside. */
        CHECK("FLAIR_RECT_CONTAINS_PT: v=4 (above top) is outside ([CS] Sec 3)",
              !FLAIR_RECT_CONTAINS_PT(&r, 4, 15));
        CHECK("FLAIR_RECT_CONTAINS_PT: h=9 (left of left) is outside ([CS] Sec 3)",
              !FLAIR_RECT_CONTAINS_PT(&r, 10, 9));
    }

    /* FLAIR_RECT_WIDTH / FLAIR_RECT_HEIGHT. */
    {
        rgn_rect_t r = { 2, 3, 7, 10 };  /* height=5, width=7 */
        CHECK("FLAIR_RECT_WIDTH = right-left = 7 ([CS] Sec 3)",
              FLAIR_RECT_WIDTH(&r) == 7);
        CHECK("FLAIR_RECT_HEIGHT = bottom-top = 5 ([CS] Sec 3)",
              FLAIR_RECT_HEIGHT(&r) == 5);
    }

    /* === SECTION H: CopyBits mode contract =================================
     * Source: [CB] Sec 2-4.
     * ====================================================================== */

    CHECK("FLAIR_COPYBITS_DITHER=64 (0x40; System-7-only; [CB] Sec 4)",
          FLAIR_COPYBITS_DITHER == 64);

    /* Valid CopyBits modes are 0..7 (src* modes from imaging.h). */
    CHECK("srcCopy=0 valid for CopyBits ([CB] Sec 4)",
          FLAIR_COPYBITS_MODE_VALID(0));
    CHECK("srcOr=1 valid for CopyBits ([CB] Sec 4)",
          FLAIR_COPYBITS_MODE_VALID(1));
    CHECK("srcXor=2 valid for CopyBits ([CB] Sec 4)",
          FLAIR_COPYBITS_MODE_VALID(2));
    CHECK("srcBic=3 valid for CopyBits ([CB] Sec 4)",
          FLAIR_COPYBITS_MODE_VALID(3));
    CHECK("notSrcCopy=4 valid for CopyBits ([CB] Sec 4)",
          FLAIR_COPYBITS_MODE_VALID(4));
    CHECK("notSrcOr=5 valid for CopyBits ([CB] Sec 4)",
          FLAIR_COPYBITS_MODE_VALID(5));
    CHECK("notSrcXor=6 valid for CopyBits ([CB] Sec 4)",
          FLAIR_COPYBITS_MODE_VALID(6));
    CHECK("notSrcBic=7 valid for CopyBits ([CB] Sec 4)",
          FLAIR_COPYBITS_MODE_VALID(7));

    /* Pattern modes (8..15) are NOT valid for CopyBits. */
    CHECK("patCopy=8 NOT valid for CopyBits ([CB] Sec 4)",
          !FLAIR_COPYBITS_MODE_VALID(8));
    CHECK("patOr=9 NOT valid for CopyBits ([CB] Sec 4)",
          !FLAIR_COPYBITS_MODE_VALID(9));
    CHECK("notPatBic=15 NOT valid for CopyBits ([CB] Sec 4)",
          !FLAIR_COPYBITS_MODE_VALID(15));

    /* ditherCopy OR'd with srcCopy is valid (System 7). */
    CHECK("srcCopy+ditherCopy (0+64) valid for CopyBits ([CB] Sec 4 System-7)",
          FLAIR_COPYBITS_MODE_VALID(0 | FLAIR_COPYBITS_DITHER));
    CHECK("srcXor+ditherCopy (2+64) valid for CopyBits ([CB] Sec 4 System-7)",
          FLAIR_COPYBITS_MODE_VALID(2 | FLAIR_COPYBITS_DITHER));
    /* ditherCopy alone (with no src mode) = 64, which after masking = 0 = srcCopy.
     * Still valid -- it means srcCopy+dither. */
    CHECK("ditherCopy alone (64) is valid (srcCopy+dither; [CB] Sec 4)",
          FLAIR_COPYBITS_MODE_VALID(FLAIR_COPYBITS_DITHER));

    /* FLAIR_COPYBITS_COLORIZE formula: src_bit=1 -> fg, src_bit=0 -> bk. */
    CHECK("FLAIR_COPYBITS_COLORIZE(1,fg,bk) = fg ([CB] Sec 3 step 3)",
          FLAIR_COPYBITS_COLORIZE(1, 42, 7) == 42);
    CHECK("FLAIR_COPYBITS_COLORIZE(0,fg,bk) = bk ([CB] Sec 3 step 3)",
          FLAIR_COPYBITS_COLORIZE(0, 42, 7) == 7);

    /* === SECTION I: Shape-family ordinals ==================================
     * Source: [DP] Sec 6.
     * ====================================================================== */

    CHECK("kShapeRect=0 ([DP] Sec 6)",  (int)kShapeRect  == 0);
    CHECK("kShapeRRect=1 ([DP] Sec 6)", (int)kShapeRRect == 1);
    CHECK("kShapeOval=2 ([DP] Sec 6)",  (int)kShapeOval  == 2);
    CHECK("kShapeArc=3 ([DP] Sec 6)",   (int)kShapeArc   == 3);
    CHECK("kShapePoly=4 ([DP] Sec 6)",  (int)kShapePoly  == 4);
    CHECK("kShapeRgn=5 ([DP] Sec 6)",   (int)kShapeRgn   == 5);
    CHECK("6 shape families (0..5)",
          (int)kShapeRgn - (int)kShapeRect + 1 == 6);

    /* === SECTION J: Transfer-mode cross-check with imaging.h ===============
     * imaging.h already asserts all 16 values; we recheck the ones
     * referenced by drawing_ops.h to confirm the include chain is consistent.
     * Source: imaging.h + [CB] Sec 4 + [QDp].
     * ====================================================================== */

    CHECK("srcCopy=0 (imaging.h flair_xfer_mode_t; [CB] Sec 4)",
          (int)srcCopy     == 0);
    CHECK("srcOr=1 (imaging.h)",    (int)srcOr      == 1);
    CHECK("srcXor=2 (imaging.h)",   (int)srcXor     == 2);
    CHECK("srcBic=3 (imaging.h)",   (int)srcBic     == 3);
    CHECK("notSrcBic=7 (imaging.h; last src mode; [CB] Sec 4)",
          (int)notSrcBic   == 7);
    CHECK("patCopy=8 (imaging.h; pen default mode; [DP] Sec 2)",
          (int)patCopy     == 8);
    CHECK("notPatBic=15 (imaging.h; last pat mode)",
          (int)notPatBic   == 15);

    /* Pattern size from imaging.h. */
    CHECK("sizeof(Pattern)=8 (imaging.h; [PA] Sec 1; [QDp] L54)",
          sizeof(Pattern) == 8);

    /* === FINAL RESULT ====================================================== */

    printf("test-drawing-ops: %d checks, %d failures, %s\n",
           checks, fails, fails == 0 ? "green" : "RED");

    return fails == 0 ? 0 : 1;
}

#endif /* __STDC_HOSTED__ */
