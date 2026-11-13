/*
 * harness/diff/dbf_diff/test_samir_value.c -- unit oracle for value.c (S0.6).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Uses the seed
 * test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY). A non-zero
 * exit on any failed check ensures `make test-dbase` can never false-green
 * (CLAUDE.md Law 2: the oracle is the truth).
 *
 * Tests:
 *   1. xb_c:   construct, typeof -> XB_C, type_char -> 'C', payload round-trip.
 *   2. xb_n:   construct, typeof -> XB_N, type_char -> 'N', payload round-trip.
 *   3. xb_d:   construct, typeof -> XB_D, type_char -> 'D', JDN payload check.
 *   4. xb_l:   construct (true/false + normalisation), typeof -> XB_L,
 *              type_char -> 'L', payload 0/1.
 *   5. xb_m:   construct, typeof -> XB_M, type_char -> 'M', payload round-trip.
 *   6. xb_u:   construct, typeof -> XB_U, type_char -> 'U'.
 *   7. xb_eq within-type -- equal cases:
 *              C (same bytes), N (exact double), D (same JDN), L (both true),
 *              M (same bytes), empty C, empty M.
 *   8. xb_eq within-type -- unequal cases:
 *              C (different content), C (same prefix, different length),
 *              N (different double), D (different JDN), L (true vs false).
 *   9. xb_eq cross-type: (C,N), (N,D), (D,L), (L,M), (M,U), (U,C) -> all 0.
 *  10. xb_eq U == U: always 0 (undefined != undefined).
 *  11. xb_l normalisation: xb_l(42) and xb_l(1) are equal; xb_l(0) and
 *      xb_l(-7) -> xb_l(-7) is true (non-zero), xb_l(0) is false: not equal.
 *  12. xb_type_char for all six types; sentinel '?' for an out-of-range tag.
 *
 * Compile + run (self-grade command, host, NOT make):
 *   gcc -std=c11 -Wall -Wextra -Werror \
 *       -I os/samir/include -I seed \
 *       os/samir/core/value.c os/samir/core/rt.c \
 *       harness/diff/dbf_diff/test_samir_value.c \
 *       -o /tmp/test_samir_value && /tmp/test_samir_value
 *
 * Freestanding compile check (value.c only; no libc, no rt.c needed for -c):
 *   gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -fno-pic \
 *       -std=c11 -Wall -Wextra -Werror \
 *       -I os/samir/include -c os/samir/core/value.c
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S0.6 oracle contract.
 *   - os/samir/include/samir/value.h (the API under test).
 *   - seed/test_assert.h (the harness idiom).
 *   - harness/diff/fat_diff/test_fat12_bpb.c (the reference test structure).
 */

#include <stdio.h>
#include <string.h>

#include "test_assert.h"       /* seed/, on -Iseed */
#include "samir/value.h"       /* os/samir/include/, on -Ios/samir/include */

TEST_HARNESS();

/* ---- helpers ---- */

/* Cast xb_type to int for CHECK messages (avoids printf-format warnings). */
static int type_int(xb_type t) { return (int)t; }

int main(void)
{
    /* ------------------------------------------------------------------ */
    /* 1. XB_C constructor + typeof + type_char + payload                  */
    /* ------------------------------------------------------------------ */
    {
        const char text[] = "HELLO";
        xb_val v = xb_c(text, 5);
        CHECK(xb_typeof(&v) == XB_C,           "xb_c: typeof == XB_C");
        CHECK(xb_type_char(XB_C) == 'C',       "xb_type_char(XB_C) == 'C'");
        CHECK(v.u.c.len == 5,                  "xb_c: len == 5");
        CHECK(v.u.c.p == text,                 "xb_c: p is the original pointer");
        CHECK(memcmp(v.u.c.p, "HELLO", 5) == 0, "xb_c: payload bytes match");
    }

    /* ------------------------------------------------------------------ */
    /* 2. XB_N constructor + typeof + type_char + payload                  */
    /* ------------------------------------------------------------------ */
    {
        xb_val v = xb_n(3.14);
        CHECK(xb_typeof(&v) == XB_N,           "xb_n: typeof == XB_N");
        CHECK(xb_type_char(XB_N) == 'N',       "xb_type_char(XB_N) == 'N'");
        CHECK(v.u.n == 3.14,                   "xb_n: payload == 3.14");
    }

    /* ------------------------------------------------------------------ */
    /* 3. XB_D constructor + typeof + type_char + JDN payload              */
    /* ------------------------------------------------------------------ */
    {
        /* JDN(1985-08-05) = 2446283, verified in rt.h / mint-results-001. */
        xb_val v = xb_d(2446283.0);
        CHECK(xb_typeof(&v) == XB_D,           "xb_d: typeof == XB_D");
        CHECK(xb_type_char(XB_D) == 'D',       "xb_type_char(XB_D) == 'D'");
        CHECK(v.u.d == 2446283.0,              "xb_d: JDN payload == 2446283");
    }

    /* ------------------------------------------------------------------ */
    /* 4. XB_L constructor + normalisation + typeof + type_char + payload  */
    /* ------------------------------------------------------------------ */
    {
        xb_val vtrue  = xb_l(1);
        xb_val vfalse = xb_l(0);
        xb_val vnorm  = xb_l(42); /* non-zero -> normalised to 1 */

        CHECK(xb_typeof(&vtrue) == XB_L,       "xb_l: typeof == XB_L");
        CHECK(xb_type_char(XB_L) == 'L',       "xb_type_char(XB_L) == 'L'");
        CHECK(vtrue.u.l  == 1,                 "xb_l(1): payload == 1");
        CHECK(vfalse.u.l == 0,                 "xb_l(0): payload == 0");
        CHECK(vnorm.u.l  == 1,                 "xb_l(42): normalised to 1");
    }

    /* ------------------------------------------------------------------ */
    /* 5. XB_M constructor + typeof + type_char + payload                  */
    /* ------------------------------------------------------------------ */
    {
        const char memo[] = "A memo body";
        xb_val v = xb_m(memo, 11);
        CHECK(xb_typeof(&v) == XB_M,           "xb_m: typeof == XB_M");
        CHECK(xb_type_char(XB_M) == 'M',       "xb_type_char(XB_M) == 'M'");
        CHECK(v.u.c.len == 11,                 "xb_m: len == 11");
        CHECK(v.u.c.p == memo,                 "xb_m: p is the original pointer");
        CHECK(memcmp(v.u.c.p, "A memo body", 11) == 0, "xb_m: payload bytes match");
    }

    /* ------------------------------------------------------------------ */
    /* 6. XB_U constructor + typeof + type_char                            */
    /* ------------------------------------------------------------------ */
    {
        xb_val v = xb_u();
        CHECK(xb_typeof(&v) == XB_U,           "xb_u: typeof == XB_U");
        CHECK(xb_type_char(XB_U) == 'U',       "xb_type_char(XB_U) == 'U'");
        (void)type_int; /* used below; suppress potential unused warning here */
    }

    /* ------------------------------------------------------------------ */
    /* 7. xb_eq within-type -- equal cases                                 */
    /* ------------------------------------------------------------------ */
    {
        /* C equal */
        xb_val c1 = xb_c("FOO", 3);
        xb_val c2 = xb_c("FOO", 3);
        CHECK(xb_eq(&c1, &c2) == 1,            "xb_eq C: same bytes -> 1");

        /* C empty */
        xb_val ce1 = xb_c("", 0);
        xb_val ce2 = xb_c("", 0);
        CHECK(xb_eq(&ce1, &ce2) == 1,          "xb_eq C empty: -> 1");

        /* N equal */
        xb_val n1 = xb_n(1.25);
        xb_val n2 = xb_n(1.25);
        CHECK(xb_eq(&n1, &n2) == 1,            "xb_eq N: same double -> 1");

        /* N zero */
        xb_val nz1 = xb_n(0.0);
        xb_val nz2 = xb_n(0.0);
        CHECK(xb_eq(&nz1, &nz2) == 1,          "xb_eq N: 0.0 == 0.0 -> 1");

        /* D equal */
        xb_val d1 = xb_d(2446283.0);
        xb_val d2 = xb_d(2446283.0);
        CHECK(xb_eq(&d1, &d2) == 1,            "xb_eq D: same JDN -> 1");

        /* L both true */
        xb_val lt1 = xb_l(1);
        xb_val lt2 = xb_l(1);
        CHECK(xb_eq(&lt1, &lt2) == 1,          "xb_eq L: true == true -> 1");

        /* L both false */
        xb_val lf1 = xb_l(0);
        xb_val lf2 = xb_l(0);
        CHECK(xb_eq(&lf1, &lf2) == 1,          "xb_eq L: false == false -> 1");

        /* M equal */
        xb_val m1 = xb_m("NOTE", 4);
        xb_val m2 = xb_m("NOTE", 4);
        CHECK(xb_eq(&m1, &m2) == 1,            "xb_eq M: same bytes -> 1");

        /* M empty */
        xb_val me1 = xb_m("", 0);
        xb_val me2 = xb_m("", 0);
        CHECK(xb_eq(&me1, &me2) == 1,          "xb_eq M empty: -> 1");
    }

    /* ------------------------------------------------------------------ */
    /* 8. xb_eq within-type -- unequal cases                               */
    /* ------------------------------------------------------------------ */
    {
        /* C different content (same length) */
        xb_val ca = xb_c("BAR", 3);
        xb_val cb = xb_c("BAZ", 3);
        CHECK(xb_eq(&ca, &cb) == 0,            "xb_eq C: diff content -> 0");

        /* C same prefix, different length (SET EXACT off would match;
         * raw xb_eq does NOT do begins-with -- that is the evaluator's job) */
        xb_val cs = xb_c("HELLO", 5);
        xb_val cl = xb_c("HELLO WORLD", 11);
        CHECK(xb_eq(&cs, &cl) == 0,            "xb_eq C: prefix != full -> 0");

        /* N different */
        xb_val na = xb_n(1.0);
        xb_val nb = xb_n(2.0);
        CHECK(xb_eq(&na, &nb) == 0,            "xb_eq N: 1.0 != 2.0 -> 0");

        /* D different JDN */
        xb_val da = xb_d(2446283.0);
        xb_val db = xb_d(2446284.0);
        CHECK(xb_eq(&da, &db) == 0,            "xb_eq D: different JDN -> 0");

        /* L true vs false */
        xb_val ltrue  = xb_l(1);
        xb_val lfalse = xb_l(0);
        CHECK(xb_eq(&ltrue, &lfalse) == 0,     "xb_eq L: true != false -> 0");
    }

    /* ------------------------------------------------------------------ */
    /* 9. xb_eq cross-type: always 0                                       */
    /* ------------------------------------------------------------------ */
    {
        xb_val vc = xb_c("X", 1);
        xb_val vn = xb_n(88.0);
        xb_val vd = xb_d(2446283.0);
        xb_val vl = xb_l(1);
        xb_val vm = xb_m("Y", 1);
        xb_val vu = xb_u();

        CHECK(xb_eq(&vc, &vn) == 0, "xb_eq cross C,N -> 0");
        CHECK(xb_eq(&vn, &vd) == 0, "xb_eq cross N,D -> 0");
        CHECK(xb_eq(&vd, &vl) == 0, "xb_eq cross D,L -> 0");
        CHECK(xb_eq(&vl, &vm) == 0, "xb_eq cross L,M -> 0");
        CHECK(xb_eq(&vm, &vu) == 0, "xb_eq cross M,U -> 0");
        CHECK(xb_eq(&vu, &vc) == 0, "xb_eq cross U,C -> 0");
        /* Symmetric */
        CHECK(xb_eq(&vn, &vc) == 0, "xb_eq cross N,C (sym) -> 0");
        CHECK(xb_eq(&vl, &vd) == 0, "xb_eq cross L,D (sym) -> 0");
    }

    /* ------------------------------------------------------------------ */
    /* 10. xb_eq U == U: always 0 (undefined != undefined)                 */
    /* ------------------------------------------------------------------ */
    {
        xb_val u1 = xb_u();
        xb_val u2 = xb_u();
        CHECK(xb_eq(&u1, &u2) == 0, "xb_eq U,U -> 0 (undefined != undefined)");
    }

    /* ------------------------------------------------------------------ */
    /* 11. xb_l normalisation: xb_l(42) == xb_l(1); xb_l(-7) != xb_l(0)   */
    /* ------------------------------------------------------------------ */
    {
        xb_val v42  = xb_l(42);
        xb_val v1   = xb_l(1);
        xb_val vn7  = xb_l(-7);
        xb_val vz   = xb_l(0);

        CHECK(xb_eq(&v42, &v1)  == 1, "xb_l normalisation: xb_l(42) == xb_l(1)");
        CHECK(xb_eq(&vn7, &v1)  == 1, "xb_l normalisation: xb_l(-7) == xb_l(1)");
        CHECK(xb_eq(&vn7, &vz)  == 0, "xb_l normalisation: xb_l(-7) != xb_l(0)");
        CHECK(xb_eq(&v42, &vz)  == 0, "xb_l normalisation: xb_l(42) != xb_l(0)");
    }

    /* ------------------------------------------------------------------ */
    /* 12. xb_type_char for all six types; check the out-of-range sentinel  */
    /* ------------------------------------------------------------------ */
    {
        CHECK(xb_type_char(XB_C) == 'C', "type_char XB_C");
        CHECK(xb_type_char(XB_N) == 'N', "type_char XB_N");
        CHECK(xb_type_char(XB_D) == 'D', "type_char XB_D");
        CHECK(xb_type_char(XB_L) == 'L', "type_char XB_L");
        CHECK(xb_type_char(XB_M) == 'M', "type_char XB_M");
        CHECK(xb_type_char(XB_U) == 'U', "type_char XB_U");
        /* Out-of-range tag should return the fail-loud sentinel '?' (Rule 2). */
        CHECK(xb_type_char((xb_type)99) == '?', "type_char unknown -> '?'");
    }

    /* ------------------------------------------------------------------ */
    /* Also verify that XB_C != XB_M even for identical payload bytes      */
    /* (type tag distinguishes them; C and M are different field types).   */
    /* ------------------------------------------------------------------ */
    {
        xb_val vc = xb_c("DATA", 4);
        xb_val vm = xb_m("DATA", 4);
        CHECK(xb_eq(&vc, &vm) == 0, "xb_eq C vs M (same bytes) -> 0 (cross-type)");
        CHECK(xb_type_char(XB_C) != xb_type_char(XB_M),
              "type_char: C != M");
    }

    return TEST_SUMMARY("test-samir-value");
}
