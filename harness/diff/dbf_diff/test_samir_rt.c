/*
 * harness/diff/dbf_diff/test_samir_rt.c -- host oracle for os/samir/core/rt.c.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Exercises the
 * freestanding runtime against known minted ground truth (Law 1).
 *
 * Test groups:
 *   A. rt_mem* / rt_str* -- basic sanity (not the focus; correctness only).
 *   B. jdn_from_ymd -- point checks against minted values.
 *   C. ymd_from_jdn -- round-trip over a sample of dates (1900-2099).
 *   D. dec_format -- the EXACT corpus tie values + overflow + alignment.
 *      Mutant doc: a round-half-away-from-zero implementation gives
 *        STR(-2.5,2,0)="-3" (wrong); a banker's-rounding implementation
 *        gives STR(2.5,2,0)="2" (wrong). Both are checked here, so either
 *        mutant goes RED for the right assertion. (S0.3 ARB rider.)
 *   E. dec_parse -- round-trip with dec_format + explicit cases.
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/re/mint-results-001.md (tie values, overflow).
 *   - ../dbase3-decomp/specs/file-formats/ndx.md sec 4.2 (TOURDATE JDNs).
 *   - docs/plans/SAMIR-implementation-plan.md S0.3 (oracle contract).
 *   - seed/test_assert.h (CHECK / TEST_HARNESS / TEST_SUMMARY idiom).
 *
 * ASCII-clean (Rule 12). No timestamps (Rule 11).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* The seed harness idiom (CLAUDE.md: mirroring test_fat12_bpb.c) */
#include "test_assert.h"   /* seed/test_assert.h, compiled with -Iseed */

/* The unit under test */
#include "samir/rt.h"      /* os/samir/include/samir/rt.h, on -Ios/samir/include */

TEST_HARNESS();

/* ---- helpers ---- */

/*
 * fmt_buf: call dec_format into a stack buffer and NUL-terminate for
 * comparison with string literals.  Returns a pointer to the static buf.
 * NOT re-entrant; use one call per CHECK.
 */
static char g_fmt_buf[64];
static const char *fmt(double v, int w, int d)
{
    int len = dec_format(v, w, d, g_fmt_buf);
    if (len < (int)sizeof(g_fmt_buf)) {
        g_fmt_buf[len] = '\0';
    } else {
        g_fmt_buf[sizeof(g_fmt_buf) - 1] = '\0';
    }
    return g_fmt_buf;
}

/* ======================================================================== */
/* Group A: memory / string utilities                                        */
/* ======================================================================== */

static void test_mem_str(void)
{
    /* rt_memcpy */
    {
        uint8_t src[4] = {0x10, 0x20, 0x30, 0x40};
        uint8_t dst[4] = {0};
        rt_memcpy(dst, src, 4);
        CHECK(dst[0] == 0x10 && dst[1] == 0x20 &&
              dst[2] == 0x30 && dst[3] == 0x40,
              "rt_memcpy: copies 4 bytes correctly");
    }

    /* rt_memset */
    {
        uint8_t buf[8];
        rt_memset(buf, 0xAB, 8);
        CHECK(buf[0] == 0xAB && buf[7] == 0xAB,
              "rt_memset: fills all bytes");
    }

    /* rt_memcmp */
    {
        CHECK(rt_memcmp("abc", "abc", 3) == 0,
              "rt_memcmp: equal regions -> 0");
        CHECK(rt_memcmp("abc", "abd", 3) < 0,
              "rt_memcmp: lesser region -> < 0");
        CHECK(rt_memcmp("abd", "abc", 3) > 0,
              "rt_memcmp: greater region -> > 0");
        CHECK(rt_memcmp("abcX", "abcY", 3) == 0,
              "rt_memcmp: stops at n");
    }

    /* rt_strlen */
    {
        CHECK(rt_strlen("") == 0,        "rt_strlen: empty string");
        CHECK(rt_strlen("hello") == 5,   "rt_strlen: 5-char string");
        CHECK(rt_strlen("a\0b") == 1,    "rt_strlen: stops at NUL");
    }

    /* rt_strncmp */
    {
        CHECK(rt_strncmp("abc", "abc", 3) == 0,  "rt_strncmp: equal");
        CHECK(rt_strncmp("abc", "abd", 3) < 0,   "rt_strncmp: lesser");
        CHECK(rt_strncmp("abc", "abc", 2) == 0,  "rt_strncmp: prefix only");
        CHECK(rt_strncmp("ab\0", "ab\0", 3) == 0, "rt_strncmp: NUL terminates");
    }
}

/* ======================================================================== */
/* Group B: jdn_from_ymd -- minted point checks                             */
/* ======================================================================== */

static void test_jdn_points(void)
{
    /*
     * TOURDATE.NDX minted values (ndx.md sec 4.2, verified byte-check):
     *   1985-08-05 = 2446283
     *   1985-09-07 = 2446316
     *   1985-09-23 = 2446332
     * MTEST.MEM minted value (mint-results-001.md sec ".mem value encodings"):
     *   1999-12-31 = 2451544
     */
    CHECK(jdn_from_ymd(1985, 8, 5)  == 2446283,
          "jdn_from_ymd(1985-08-05) == 2446283 [minted TOURDATE.NDX]");
    CHECK(jdn_from_ymd(1985, 9, 7)  == 2446316,
          "jdn_from_ymd(1985-09-07) == 2446316 [minted TOURDATE.NDX]");
    CHECK(jdn_from_ymd(1985, 9, 23) == 2446332,
          "jdn_from_ymd(1985-09-23) == 2446332 [minted TOURDATE.NDX]");
    CHECK(jdn_from_ymd(1999, 12, 31) == 2451544,
          "jdn_from_ymd(1999-12-31) == 2451544 [minted MTEST.MEM]");

    /* Range boundary points */
    CHECK(jdn_from_ymd(1900, 1, 1)  == 2415021,
          "jdn_from_ymd(1900-01-01) == 2415021 [range low]");
    CHECK(jdn_from_ymd(2000, 1, 1)  == 2451545,
          "jdn_from_ymd(2000-01-01) == 2451545 [Y2K]");
    CHECK(jdn_from_ymd(2155, 12, 31) == 2508522,
          "jdn_from_ymd(2155-12-31) == 2508522 [range high]");
}

/* ======================================================================== */
/* Group C: ymd_from_jdn -- round-trip over sample dates 1900-2099         */
/* ======================================================================== */

static void test_jdn_roundtrip(void)
{
    /*
     * Step every ~37 days from 1900-01-01 to 2099-12-31.
     * jdn_from_ymd(y,m,d) -> j; ymd_from_jdn(j) -> y2,m2,d2; assert equal.
     * This exercises month-boundary transitions, leap years (incl. 2000,
     * 1900), and century transitions without being exhaustive (~1970 checks).
     */
    static const int32_t start_years[]  = {1900, 1970, 2000, 2050, 2099};
    static const int32_t start_months[] = {1,    7,    1,    6,    1};
    static const int32_t start_days[]   = {1,    1,    1,    15,   1};
    int si;

    /* Full sweep: every 37 days from JDN(1900-01-01) to JDN(2099-12-31) */
    {
        int32_t j0 = jdn_from_ymd(1900, 1, 1);
        int32_t j1 = jdn_from_ymd(2099, 12, 31);
        int32_t j;
        int     fails_before = g_fails;

        for (j = j0; j <= j1; j += 37) {
            int32_t y2, m2, d2;
            int32_t j_rt;
            ymd_from_jdn(j, &y2, &m2, &d2);
            j_rt = jdn_from_ymd(y2, m2, d2);
            /* One CHECK per range step -- report only the first failure
             * to keep output readable (if it fails, we stop checking). */
            if (j_rt != j) {
                g_checks++;
                g_fails++;
                fprintf(stderr,
                    "  FAIL %s:%d: JDN round-trip: JDN=%d -> %04d-%02d-%02d"
                    " -> JDN=%d\n",
                    __FILE__, __LINE__, (int)j,
                    (int)y2, (int)m2, (int)d2, (int)j_rt);
                break;
            }
        }
        if (g_fails == fails_before) {
            g_checks++;
            /* explicitly pass */
        }
        CHECK(g_fails == fails_before,
              "ymd_from_jdn / jdn_from_ymd round-trip every 37 days 1900-2099");
    }

    /* Also spot-check the minted dates via round-trip */
    (void)start_years; (void)start_months; (void)start_days; (void)si;
    {
        int32_t y, m, d;
        ymd_from_jdn(2446283, &y, &m, &d);
        CHECK(y == 1985 && m == 8 && d == 5,
              "ymd_from_jdn(2446283) -> 1985-08-05 [minted TOURDATE.NDX]");
        ymd_from_jdn(2451544, &y, &m, &d);
        CHECK(y == 1999 && m == 12 && d == 31,
              "ymd_from_jdn(2451544) -> 1999-12-31 [minted MTEST.MEM]");
        ymd_from_jdn(2451545, &y, &m, &d);
        CHECK(y == 2000 && m == 1 && d == 1,
              "ymd_from_jdn(2451545) -> 2000-01-01 [Y2K]");
    }
}

/* ======================================================================== */
/* Group D: dec_format -- corpus tie values + width overflow + alignment    */
/* ======================================================================== */

static void test_dec_format(void)
{
    /*
     * The EXACT minted tie values (re/mint-results-001.md sec
     * "Numeric rounding tie-break"). [verified: minted BATLOG]
     *
     * Rule: ties toward +infinity (add 0.5*10^-dec, truncate toward zero).
     *   STR(2.5, 2, 0) = " 3"   (width=2, dec=0)
     *   STR(-2.5, 2, 0)= "-2"
     *   STR(0.5, 1, 0) = "1"    (width=1, dec=0)
     *   STR(1.5, 2, 0) = " 2"   (width=2, dec=0)
     *   STR(0.125, 4, 2)= "0.13" (width=4, dec=2)
     *
     * MUTANT DETECTION:
     *   - round-half-away-from-zero gives STR(-2.5,2,0)="-3" -> D2 would FAIL
     *   - banker's rounding gives STR(2.5,2,0)="2" -> D1 would FAIL
     * Both mutations produce different failures, so either implementation
     * defect is caught.
     */

    /* D1: STR(2.5,2,0) = " 3" -- catches banker's rounding (would give " 2") */
    CHECK_STR_EQ(fmt(2.5, 2, 0), " 3",
        "dec_format(2.5,w=2,d=0)=\" 3\" -- ties toward +inf [minted BATLOG]");

    /* D2: STR(-2.5,2,0) = "-2" -- catches round-half-away (would give "-3") */
    CHECK_STR_EQ(fmt(-2.5, 2, 0), "-2",
        "dec_format(-2.5,w=2,d=0)=\"-2\" -- ties toward +inf not away-from-zero"
        " [minted BATLOG]");

    /* D3: STR(0.5,1,0) = "1" */
    CHECK_STR_EQ(fmt(0.5, 1, 0), "1",
        "dec_format(0.5,w=1,d=0)=\"1\" [minted BATLOG]");

    /* D4: STR(1.5,2,0) = " 2" */
    CHECK_STR_EQ(fmt(1.5, 2, 0), " 2",
        "dec_format(1.5,w=2,d=0)=\" 2\" [minted BATLOG]");

    /* D5: STR(0.125,4,2) = "0.13" */
    CHECK_STR_EQ(fmt(0.125, 4, 2), "0.13",
        "dec_format(0.125,w=4,d=2)=\"0.13\" [minted BATLOG]");

    /* Width overflow -> '*'-fill (minted OTEST.DBF: SEATS_OPEN N4 with 99999) */
    /* [verified: minted OTEST.DBF, re/mint-results-001.md] */
    CHECK_STR_EQ(fmt(99999.0, 4, 0), "****",
        "dec_format(99999,w=4,d=0)=\"****\" -- overflow '*'-fill [minted OTEST.DBF]");

    CHECK_STR_EQ(fmt(-100.0, 3, 0), "***",
        "dec_format(-100,w=3,d=0)=\"***\" -- overflow (needs 4, has 3)");

    /* Right-justification and zero */
    CHECK_STR_EQ(fmt(0.0, 1, 0), "0",
        "dec_format(0,w=1,d=0)=\"0\"");
    CHECK_STR_EQ(fmt(0.0, 4, 0), "   0",
        "dec_format(0,w=4,d=0)=\"   0\" -- right-justified with spaces");
    CHECK_STR_EQ(fmt(42.0, 6, 0), "    42",
        "dec_format(42,w=6,d=0)=\"    42\" -- right-justified");

    /* Decimal point present when dec > 0 */
    CHECK_STR_EQ(fmt(3.14, 6, 2), "  3.14",
        "dec_format(3.14,w=6,d=2)=\"  3.14\"");
    CHECK_STR_EQ(fmt(-1.5, 5, 1), " -1.5",
        "dec_format(-1.5,w=5,d=1)=\" -1.5\"");

    /* Integer (dec=0), negative */
    CHECK_STR_EQ(fmt(-570.0, 4, 0), "-570",
        "dec_format(-570,w=4,d=0)=\"-570\" -- leading minus (minted STR check)");

    /* Exact fit: no overflow, no pad */
    CHECK_STR_EQ(fmt(123.0, 3, 0), "123",
        "dec_format(123,w=3,d=0)=\"123\" -- exact fit");
}

/* ======================================================================== */
/* Group E: dec_parse                                                        */
/* ======================================================================== */

static void test_dec_parse(void)
{
    /* Basic integer */
    CHECK(dec_parse("42", 2) == 42.0,
          "dec_parse(\"42\") == 42.0");

    /* Leading/trailing spaces (dBASE N-field is space-padded) */
    CHECK(dec_parse("  42  ", 6) == 42.0,
          "dec_parse(\"  42  \") == 42.0 -- spaces trimmed");

    /* Empty / all spaces -> 0.0 */
    CHECK(dec_parse("    ", 4) == 0.0,
          "dec_parse(\"    \") == 0.0");
    CHECK(dec_parse("", 0) == 0.0,
          "dec_parse(\"\") == 0.0");

    /* Negative */
    CHECK(dec_parse("-123", 4) == -123.0,
          "dec_parse(\"-123\") == -123.0");

    /* Decimal point */
    CHECK(dec_parse("3.14", 4) == 3.14,
          "dec_parse(\"3.14\") == 3.14");
    CHECK(dec_parse("-0.5", 4) == -0.5,
          "dec_parse(\"-0.5\") == -0.5");

    /* Round-trip: dec_format then dec_parse should recover original value */
    {
        double vals[] = {0.0, 1.0, -1.0, 42.5, -123.45, 279.0};
        int    i;
        for (i = 0; i < 6; i++) {
            char buf[16];
            int  len = dec_format(vals[i], 10, 2, buf);
            double got = dec_parse(buf, len);
            /*
             * Exact double comparison is safe here: we format at dec=2,
             * so the maximum representable precision is 2 decimal places;
             * the round-trip stays within IEEE-754 round-trip for these values.
             */
            if (got != vals[i]) {
                g_checks++;
                g_fails++;
                fprintf(stderr,
                    "  FAIL %s:%d: dec_parse(dec_format(%g)) -> %g (expected %g)\n",
                    __FILE__, __LINE__, vals[i], got, vals[i]);
            } else {
                g_checks++;
            }
        }
    }
}

/* ======================================================================== */
/* main                                                                      */
/* ======================================================================== */

int main(void)
{
    test_mem_str();
    test_jdn_points();
    test_jdn_roundtrip();
    test_dec_format();
    test_dec_parse();

    return TEST_SUMMARY("test_samir_rt");
}
