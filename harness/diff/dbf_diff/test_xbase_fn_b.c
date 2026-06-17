/*
 * harness/diff/dbf_diff/test_xbase_fn_b.c -- unit oracle for built-in
 * functions B (S3.6a / initech-7az.11): ABS, INT, MOD, ROUND, MAX, MIN
 * (numeric, freestanding) + CDOW, CMONTH, DOW (date-name, from JDN).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Uses the seed
 * test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY). A non-zero
 * exit on any failed check ensures the make gate can never false-green (Law 2).
 *
 * Structure mirrors test_xbase_fn_a.c (S3.5) exactly: drives each case
 * through the FULL lex->parse->eval pipeline, asserting RESULT TYPE + VALUE
 * for success cells and the EXACT dBASE catalog CODE for error cells.
 *
 * Functions exercised (S3.6a FREESTANDING set):
 *   Numeric:  ABS INT MOD ROUND MAX MIN
 *   Date:     CDOW CMONTH DOW
 *   Errors:   wrong arity (#11), wrong type (#9)
 *
 * -------------------------------------------------------------------------
 * GATED DISCIPLINE (Law 1 / plan sec.7 "GATED register" / S3.6 line):
 *   The following cases are [oracle-resolves] (MINT-blocked, numfn-1..4);
 *   each prints a loud SKIP line and is NOT asserted:
 *     numfn-1: ROUND tie direction (ROUND(2.5,0), ROUND(-2.5,0))
 *     numfn-2: INT on negatives (INT(-3.7), INT(-3.0))
 *     numfn-3: MOD sign with negative operands; MOD(a,0) zero-divisor
 *     numfn-4: MAX/MIN on two Date args (return-type Date vs day-number)
 *   A SKIP is logged, never silent (the 86Box-leg discipline: a missing
 *   gate prints what is missing and why).
 *
 * -------------------------------------------------------------------------
 * MUTATION PROOF (Rule 6):
 *   Compile with -DXB_MUTATE_FN_DOW to activate the perturbation in
 *   fn_builtins.c (S3.6a): DOW shifts its result by 1, so
 *   DOW(CTOD('08/04/85'))=1 (Sunday) instead returns 2. The assertions for
 *   DOW of known-Sunday and known-Monday dates then go RED, proving the oracle
 *   catches an off-by-one in the 1=Sunday numbering rule (the SETTLED, non-GATED
 *   DOW numbering from numeric-and-date-functions.md / Harbour datetime.txt:262).
 *   The mutation targets the SETTLED case (not a GATED cell), as required by Rule 6.
 *
 *   Mutant macro: XB_MUTATE_FN_DOW (also defined in fn_builtins.c).
 *
 * -------------------------------------------------------------------------
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md sec.5 S3.6 oracle contract;
 *     sec.7 GATED register (numfn-1..4)
 *   - ../dbase3-decomp/specs/functions/numeric-and-date-functions.md
 *     ABS [verified: HELP.DBS 1292]; INT truncation-toward-zero [HELP.DBS 1297];
 *     MOD definition MOD(a,b)=a-b*INT(a/b) [HELP.DBS 1306];
 *     ROUND(<N>,<places>) [HELP.DBS 1307-1308];
 *     MAX/MIN same-type, two args [HELP.DBS 1302-1305];
 *     DOW 1=Sunday,7=Saturday [HELP.DBS 1248 + Harbour datetime.txt:262];
 *     CDOW English weekday name [HELP.DBS 1243 + Harbour datetime.txt:6-41];
 *     CMONTH English month name [HELP.DBS 1244 + Harbour datetime.txt:48-83]
 *   - spec/samir/dbase_msg_codes.tsv (#9 mismatch, #11 invalid argument)
 *   - os/samir/include/samir/eval.h (xb_ctx / xb_eval / xb_call_builtin API)
 *   - os/samir/include/samir/rt.h (jdn_from_ymd; ymd_from_jdn)
 *   - seed/test_assert.h (harness idiom)
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "test_assert.h"     /* seed/, on -Iseed                          */
#include "samir/eval.h"      /* os/samir/include/, on -Ios/samir/include  */
#include "samir/value.h"
#include "samir/rt.h"

TEST_HARNESS();

#define TBUF 64    /* token buffer */
#define NBUF 64    /* node pool    */

/* Injected "today": 1985-08-05 (JDN 2446283, a Monday).
 * Cross-reference with test_xbase_fn_a.c -- same fixture for consistency
 * (corpus TOURDATE date, rt.c verified). Reproducible (Rule 11). */
#define TODAY_Y 1985
#define TODAY_M 8
#define TODAY_D 5

static char g_scratch[512];

/* -------------------------------------------------------------------------
 * eval_expr: lex + parse + eval a source expression with EXACT OFF and
 * the injected today. Returns xb_eval rc; lex/parse failures surface as
 * rc -1000/-1001 so failures are loud and unambiguous. Mirrors fn-a oracle.
 */
static int eval_expr(const char *s, xb_val *out, int *err)
{
    xb_token toks[TBUF];
    xb_node  pool[NBUF];
    xb_ctx   ctx;
    int lerr = 0, perr = 0, nt, root;

    nt = xb_lex(s, (uint32_t)strlen(s), toks, TBUF, &lerr);
    if (nt < 0) { *err = -1000; *out = xb_u(); return -1000; }
    root = xb_parse(toks, (uint32_t)nt, pool, NBUF, &perr);
    if (root < 0) { *err = -1001; *out = xb_u(); return -1001; }

    ctx.set_exact    = 0;
    ctx.resolve      = NULL;
    ctx.user         = NULL;
    ctx.scratch      = g_scratch;
    ctx.scratch_cap  = (uint32_t)sizeof(g_scratch);
    ctx.scratch_used = 0;
    ctx.ctx_today    = (double)jdn_from_ymd(TODAY_Y, TODAY_M, TODAY_D);

    return xb_eval(pool, root, &ctx, out, err);
}

/* Assert a Numeric result equals want (exact IEEE ==). */
static int ok_n(const char *s, double want, const char *msg)
{
    xb_val v; int err = 999;
    int rc = eval_expr(s, &v, &err);
    int good = (rc == 0 && err == XBEE_OK && v.t == XB_N && v.u.n == want);
    CHECK(good, msg);
    return good;
}

/* Assert a Character result equals want[0..wlen). */
static int ok_c(const char *s, const char *want, int wlen, const char *msg)
{
    xb_val v; int err = 999;
    int rc = eval_expr(s, &v, &err);
    int good = (rc == 0 && err == XBEE_OK && v.t == XB_C &&
                v.u.c.len == (uint16_t)wlen &&
                (wlen == 0 || memcmp(v.u.c.p, want, (size_t)wlen) == 0));
    CHECK(good, msg);
    return good;
}

/* Assert an error cell: eval fails with exactly `code`. */
static int err_is(const char *s, int code, const char *msg)
{
    xb_val v; int err = 999;
    int rc = eval_expr(s, &v, &err);
    int good = (rc != 0 && err == code);
    CHECK(good, msg);
    return good;
}

/* GATED loud-skip: print the skip line (never silent) and return without
 * asserting. Mirrors the 86Box-leg idiom from the fat-diff harness. */
static void skip_gated(const char *id, const char *reason)
{
    fprintf(stderr, "  SKIP [GATED:%s] %s -- pending MINT (numfn-1..4)\n",
            id, reason);
}

int main(void)
{
    /* Pre-compute some JDN constants used across sections.
     * All dates cited per corpus: jdn_from_ymd is verified by rt.c tests.
     * [verified: minted TOURDATE.NDX: JDN(1985-08-05)=2446283]. */
    double j850804 = (double)jdn_from_ymd(1985, 8, 4); /* Sunday   (DOW=1) */
    double j850805 = (double)jdn_from_ymd(1985, 8, 5); /* Monday   (DOW=2) */
    double j850810 = (double)jdn_from_ymd(1985, 8, 10);/* Saturday (DOW=7) */

    /* ================================================================ */
    /* A. ABS -- absolute value (total domain, freestanding)             */
    /*    Ref: numeric-and-date-functions.md [verified: HELP.DBS 1292]   */
    /* ================================================================ */
    ok_n("ABS(5)",    5.0,  "A1: ABS(5)=5 [verified]");
    ok_n("ABS(-5)",   5.0,  "A2: ABS(-5)=5 -- removes sign [MUTANT TARGET]");
    ok_n("ABS(0)",    0.0,  "A3: ABS(0)=0");
    ok_n("ABS(3.14)", 3.14, "A4: ABS(3.14)=3.14 (positive unchanged)");
    /* Error: wrong arity (#11) and wrong type (#9). */
    err_is("ABS()",    XBEE_INVALID_ARG, "A5: ABS() no args -> #11");
    err_is("ABS(1,2)", XBEE_INVALID_ARG, "A6: ABS(1,2) too many -> #11");
    err_is("ABS('x')", XBEE_MISMATCH,    "A7: ABS non-numeric arg -> #9");

    /* ================================================================ */
    /* B. INT -- integer truncation toward zero (positive only asserted) */
    /*    Ref: numeric-and-date-functions.md [verified: HELP.DBS 1297]   */
    /* Positive-only cases are unambiguous: INT(3.99)->3, INT(3.01)->3.  */
    /* Negative cases are GATED (numfn-2 -- INT-on-negatives).           */
    /* ================================================================ */
    ok_n("INT(3.99)", 3.0, "B1: INT(3.99)=3 (truncation, not rounding) [verified]");
    ok_n("INT(3.01)", 3.0, "B2: INT(3.01)=3 [verified]");
    ok_n("INT(0)",    0.0, "B3: INT(0)=0");
    ok_n("INT(7)",    7.0, "B4: INT(7)=7 (already integer)");

    skip_gated("numfn-2", "INT(-3.7): truncate-toward-zero vs floor unconfirmed");
    skip_gated("numfn-2", "INT(-3.0): same gate");

    err_is("INT()",    XBEE_INVALID_ARG, "B5: INT() no args -> #11");
    err_is("INT('x')", XBEE_MISMATCH,    "B6: INT non-numeric -> #9");

    /* ================================================================ */
    /* C. MOD -- modulus (positive operands only asserted; sign GATED)   */
    /*    Ref: numeric-and-date-functions.md [verified: HELP.DBS 1306]   */
    /*    Definition: MOD(a,b) = a - b*INT(a/b).                         */
    /*    For positive: MOD(7,3)=1, MOD(17,5)=2. [corpus-unambiguous]   */
    /* ================================================================ */
    ok_n("MOD(7,3)",   1.0, "C1: MOD(7,3)=1 [verified positive-only]");
    ok_n("MOD(17,5)",  2.0, "C2: MOD(17,5)=2 [verified positive-only]");
    ok_n("MOD(10,2)",  0.0, "C3: MOD(10,2)=0 (exact division)");
    ok_n("MOD(1,5)",   1.0, "C4: MOD(1,5)=1 (dividend < divisor)");

    skip_gated("numfn-3", "MOD(-17,5): sign-of-dividend vs sign-of-divisor unconfirmed");
    skip_gated("numfn-3", "MOD(17,-5): same gate");
    skip_gated("numfn-3", "MOD(a,0): zero-divisor behavior (return a vs error) unconfirmed");

    err_is("MOD(1)",    XBEE_INVALID_ARG, "C5: MOD(1) wrong arity -> #11");
    err_is("MOD('x',2)",XBEE_MISMATCH,    "C6: MOD non-numeric first -> #9");
    err_is("MOD(1,'y')",XBEE_MISMATCH,    "C7: MOD non-numeric second -> #9");

    /* ================================================================ */
    /* D. ROUND -- round to N decimal places (non-tie cases only)        */
    /*    Ref: numeric-and-date-functions.md [verified: HELP.DBS 1307-8] */
    /*    Tie direction is GATED (numfn-1). Only assert unambiguous cases.*/
    /* ================================================================ */
    ok_n("ROUND(3.14159,2)", 3.14,    "D1: ROUND(3.14159,2)=3.14 (non-tie) [verified]");
    ok_n("ROUND(3.145,2)",   3.15,    "D2: ROUND(3.145,2)=3.15 (rounds up, non-tie)");
    ok_n("ROUND(3.0,0)",     3.0,     "D3: ROUND(3.0,0)=3 (already integer)");
    ok_n("ROUND(1234,0)",    1234.0,  "D4: ROUND(1234,0)=1234");
    /* Negative dec: round to whole-number places (Harbour math.txt:349-351). */
    ok_n("ROUND(1234,-2)",   1200.0,  "D5: ROUND(1234,-2)=1200 [Harbour: negative dec]");

    skip_gated("numfn-1", "ROUND(2.5,0): tie direction (toward-+inf vs away-from-zero) unconfirmed");
    skip_gated("numfn-1", "ROUND(-2.5,0): same gate");

    err_is("ROUND(1)",    XBEE_INVALID_ARG, "D6: ROUND(1) wrong arity -> #11");
    err_is("ROUND('x',1)",XBEE_MISMATCH,    "D7: ROUND non-numeric first -> #9");
    err_is("ROUND(1,'x')",XBEE_MISMATCH,    "D8: ROUND non-numeric second -> #9");

    /* ================================================================ */
    /* E. MAX / MIN -- larger/smaller of two same-type values            */
    /*    Ref: numeric-and-date-functions.md [verified: HELP.DBS 1302-5] */
    /*    Numeric: asserted. Date overload: GATED (numfn-4).             */
    /* ================================================================ */
    ok_n("MAX(3,7)",   7.0,  "E1: MAX(3,7)=7 [verified]");
    ok_n("MAX(7,3)",   7.0,  "E2: MAX(7,3)=7 (order-independent)");
    ok_n("MAX(-1,-5)", -1.0, "E3: MAX(-1,-5)=-1 (larger negative)");
    ok_n("MAX(4,4)",   4.0,  "E4: MAX(4,4)=4 (equal)");
    ok_n("MIN(3,7)",   3.0,  "E5: MIN(3,7)=3 [verified]");
    ok_n("MIN(7,3)",   3.0,  "E6: MIN(7,3)=3");
    ok_n("MIN(-1,-5)", -5.0, "E7: MIN(-1,-5)=-5 (smaller negative)");
    ok_n("MIN(4,4)",   4.0,  "E8: MIN(4,4)=4 (equal)");

    skip_gated("numfn-4", "MAX(date1,date2): return-type (Date vs day-number) unconfirmed");
    skip_gated("numfn-4", "MIN(date1,date2): same gate");

    err_is("MAX(1)",    XBEE_INVALID_ARG, "E9: MAX(1) wrong arity -> #11");
    err_is("MIN(1,2,3)",XBEE_INVALID_ARG, "E10: MIN(1,2,3) wrong arity -> #11");
    err_is("MAX('a',1)",XBEE_MISMATCH,    "E11: MAX mixed C/N -> #9");
    err_is("MAX(1,'b')",XBEE_MISMATCH,    "E12: MAX mixed N/C -> #9");

    /* ================================================================ */
    /* F. DOW -- day-of-week number (1=Sunday .. 7=Saturday)             */
    /*    Ref: numeric-and-date-functions.md DOW [verified: HELP.DBS 1248 */
    /*         + Harbour datetime.txt:262 "1=Sunday, 7=Saturday"]        */
    /* Ground truth: JDN(1985-08-04)=Sunday -> DOW=1 [MUTANT TARGET],   */
    /*               JDN(1985-08-05)=Monday -> DOW=2,                   */
    /*               JDN(1985-08-10)=Saturday -> DOW=7.                 */
    /* ================================================================ */
    {
        /* Sunday 1985-08-04: CTOD('08/04/85') parses to j850804. */
        ok_n("DOW(CTOD('08/04/85'))", 1.0, "F1: DOW(Sunday)=1 [MUTANT TARGET: verified]");
        ok_n("DOW(CTOD('08/05/85'))", 2.0, "F2: DOW(Monday)=2 [MUTANT TARGET: verified]");
        ok_n("DOW(CTOD('08/10/85'))", 7.0, "F3: DOW(Saturday)=7 [verified]");
        /* Blank date -> 0 [numeric-and-date-functions.md, blank-date note]. */
        ok_n("DOW(CTOD('  /  /  '))", 0.0, "F4: DOW(blank)=0 [verified]");
        /* Mid-week spot check: Tuesday 1985-08-06 -> DOW=3. */
        ok_n("DOW(CTOD('08/06/85'))", 3.0, "F5: DOW(Tuesday)=3 [verified]");
        /* Cross-check: DOW and date() alignment (today=1985-08-05=Monday). */
        ok_n("DOW(DATE())", 2.0, "F6: DOW(DATE())=2 (injected Monday) [verified]");

        /* Suppress "unused variable" warning for j850804/j850805/j850810 since
         * they are used in CDOW/CMONTH sections too. */
        (void)j850804; (void)j850805; (void)j850810;
    }

    err_is("DOW()",      XBEE_INVALID_ARG, "F7: DOW() no args -> #11");
    err_is("DOW('x')",   XBEE_MISMATCH,    "F8: DOW non-date arg -> #9");

    /* ================================================================ */
    /* G. CDOW -- character weekday name                                 */
    /*    Ref: numeric-and-date-functions.md CDOW [verified: HELP.DBS    */
    /*         1243 + Harbour datetime.txt:6-41]                         */
    /*    Names: "Sunday","Monday","Tuesday","Wednesday","Thursday",      */
    /*           "Friday","Saturday" (ASCII, capitalized).               */
    /* ================================================================ */
    ok_c("CDOW(CTOD('08/04/85'))", "Sunday",    6, "G1: CDOW(Sunday) [verified]");
    ok_c("CDOW(CTOD('08/05/85'))", "Monday",    6, "G2: CDOW(Monday) [verified]");
    ok_c("CDOW(CTOD('08/06/85'))", "Tuesday",   7, "G3: CDOW(Tuesday)");
    ok_c("CDOW(CTOD('08/07/85'))", "Wednesday", 9, "G4: CDOW(Wednesday)");
    ok_c("CDOW(CTOD('08/08/85'))", "Thursday",  8, "G5: CDOW(Thursday)");
    ok_c("CDOW(CTOD('08/09/85'))", "Friday",    6, "G6: CDOW(Friday)");
    ok_c("CDOW(CTOD('08/10/85'))", "Saturday",  8, "G7: CDOW(Saturday) [verified]");
    /* CDOW agrees with DOW: both derive from the same JDN formula. */
    ok_c("CDOW(DATE())", "Monday", 6, "G8: CDOW(DATE())='Monday' (injected) [verified]");

    err_is("CDOW()",      XBEE_INVALID_ARG, "G9: CDOW() no args -> #11");
    err_is("CDOW('x')",   XBEE_MISMATCH,    "G10: CDOW non-date arg -> #9");

    /* ================================================================ */
    /* H. CMONTH -- character month name                                 */
    /*    Ref: numeric-and-date-functions.md CMONTH [verified: HELP.DBS  */
    /*         1244 + Harbour datetime.txt:48-83]                        */
    /*    Names: "January".."December" (ASCII, capitalized).             */
    /* ================================================================ */
    ok_c("CMONTH(CTOD('01/15/85'))", "January",   7, "H1: CMONTH(Jan) [verified]");
    ok_c("CMONTH(CTOD('02/15/85'))", "February",  8, "H2: CMONTH(Feb) [verified]");
    ok_c("CMONTH(CTOD('03/15/85'))", "March",     5, "H3: CMONTH(Mar)");
    ok_c("CMONTH(CTOD('04/15/85'))", "April",     5, "H4: CMONTH(Apr)");
    ok_c("CMONTH(CTOD('05/15/85'))", "May",       3, "H5: CMONTH(May)");
    ok_c("CMONTH(CTOD('06/15/85'))", "June",      4, "H6: CMONTH(Jun)");
    ok_c("CMONTH(CTOD('07/15/85'))", "July",      4, "H7: CMONTH(Jul)");
    ok_c("CMONTH(CTOD('08/15/85'))", "August",    6, "H8: CMONTH(Aug) -- today's month [verified]");
    ok_c("CMONTH(CTOD('09/15/85'))", "September", 9, "H9: CMONTH(Sep)");
    ok_c("CMONTH(CTOD('10/15/85'))", "October",   7, "H10: CMONTH(Oct)");
    ok_c("CMONTH(CTOD('11/15/85'))", "November",  8, "H11: CMONTH(Nov)");
    ok_c("CMONTH(CTOD('12/15/85'))", "December",  8, "H12: CMONTH(Dec)");
    /* Cross-check: CMONTH(DATE()) is the month we injected (August). */
    ok_c("CMONTH(DATE())", "August", 6, "H13: CMONTH(DATE())='August' (injected) [verified]");

    err_is("CMONTH()",      XBEE_INVALID_ARG, "H14: CMONTH() no args -> #11");
    err_is("CMONTH('x')",   XBEE_MISMATCH,    "H15: CMONTH non-date arg -> #9");

    /* ================================================================ */
    /* I. Realistic composed idioms combining S3.6a functions            */
    /* ================================================================ */
    /* ABS used inside an expression (RECONCIL.PRG:28 idiom). */
    ok_n("ABS(-100) + ABS(50)", 150.0, "I1: ABS(-100)+ABS(50)=150 (RECONCIL idiom)");
    /* INT used in the MOD definition: MOD(7,3)=7-3*INT(7/3)=7-3*2=1. */
    ok_n("7 - 3 * INT(7/3)", 1.0, "I2: 7-3*INT(7/3)=1 (MOD formula spelled out)");
    /* ROUND + MAX cross: MAX(ROUND(3.14159,2), 3). */
    ok_n("MAX(ROUND(3.14159,2), 3.0)", 3.14, "I3: MAX(ROUND(3.14159,2),3.0)=3.14");
    /* DOW + CDOW consistency: DOW and CDOW produce matching day indices. */
    ok_n("DOW(CTOD('08/07/85'))",     4.0,    "I4: DOW(Wed 1985-08-07)=4 [verified]");
    ok_c("CDOW(CTOD('08/07/85'))", "Wednesday", 9, "I5: CDOW(Wed)='Wednesday' [verified]");

    return TEST_SUMMARY("test-xbase-fn-b");
}
