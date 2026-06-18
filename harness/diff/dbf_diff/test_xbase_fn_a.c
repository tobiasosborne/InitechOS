/*
 * harness/diff/dbf_diff/test_xbase_fn_a.c -- unit oracle for built-in
 * functions A (S3.5 / initech-7az.1).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Uses the seed
 * test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY). A non-zero exit
 * on any failed check ensures the make gate can never false-green (Law 2: the
 * oracle is the truth). The mutant guard greps for "checks," in the summary.
 *
 * It drives each case through the FULL pipeline: lex (S3.1) -> parse (S3.2,
 * now with XBN_CALL/XBN_ARG) -> eval (S3.3 + S3.5 dispatch + fn_builtins.c).
 * Coverage is one-or-more cases per function, asserting the RESULT TYPE +
 * VALUE for success cells and the EXACT dBASE catalog CODE for error cells.
 *
 * Functions exercised (S3.5 set):
 *   String:  UPPER LOWER TRIM RTRIM LTRIM SUBSTR LEN SPACE CHR ASC
 *   Convert: STR VAL CTOD DTOC
 *   Date:    DATE DAY MONTH YEAR
 *   Generic: IIF TYPE
 *   Errors:  unknown fn (#31), DTOS-not-in-III+ (#31), wrong arity (#11),
 *            wrong type (#9), CHR range (#57), SPACE neg (#60), SUBSTR start (#62)
 *
 * -------------------------------------------------------------------------
 * MUTATION PROOF (Rule 6):
 *   Compile with -DXB_MUTATE_FN_SUBSTR to activate the single perturbation in
 *   fn_builtins.c: SUBSTR treats its start as 0-based instead of 1-based, so
 *   SUBSTR("ABCDEF",2,3) returns "CDE" not "BCD". The 1-based assertions then
 *   go RED, proving the oracle catches an off-by-one in the load-bearing
 *   SUBSTR index rule (string-functions.md line 119).
 *
 * Compile + run (self-grade, host):
 *   cc -std=c11 -Wall -Wextra -Werror -Iseed -Ios/samir/include -Ispec \
 *      os/samir/core/eval.c os/samir/core/parse.c os/samir/core/lex.c \
 *      os/samir/core/value.c os/samir/core/rt.c os/samir/core/fn_builtins.c \
 *      harness/diff/dbf_diff/test_xbase_fn_a.c \
 *      -o /tmp/test_xbase_fn_a && /tmp/test_xbase_fn_a
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md sec.5 S3.5 oracle contract
 *   - ../dbase3-decomp/specs/functions/string-functions.md (SUBSTR 1-based;
 *     LEN/UPPER/LOWER/TRIM/LTRIM/SPACE/CHR/ASC/STR/VAL semantics + error codes)
 *   - ../dbase3-decomp/specs/functions/numeric-and-date-functions.md
 *     (CTOD/DTOC/DAY/MONTH/YEAR/DATE; IIF same-type branch; DTOS NOT III+)
 *   - ../dbase3-decomp/specs/functions/system-and-database-functions.md (TYPE)
 *   - ../dbase3-decomp/specs/runtime/dates-and-century.md (mm/dd/yy, base-1900)
 *   - spec/samir/dbase_msg_codes.tsv (#9/#11/#31/#57/#60/#62)
 *   - os/samir/include/samir/eval.h (xb_ctx / xb_eval / xb_call_builtin API)
 *   - seed/test_assert.h (harness idiom)
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "test_assert.h"        /* seed/, on -Iseed                          */
#include "samir/eval.h"         /* os/samir/include/, on -Ios/samir/include  */
#include "samir/value.h"
#include "samir/rt.h"

TEST_HARNESS();

#define TBUF 64    /* token buffer */
#define NBUF 64    /* node pool    */

/* A fixed injected "today" for DATE(): 1985-08-05 (the corpus TOURDATE date,
 * JDN 2446283 -- rt.c verified). Reproducible (Rule 11). */
#define TODAY_Y 1985
#define TODAY_M 8
#define TODAY_D 5

static char g_scratch[512];

/*
 * eval_expr: lex + parse + eval a source expression with EXACT OFF and the
 * injected today. Fills *out / *err and returns the xb_eval rc. A lex or parse
 * failure is reported as rc -1000/-1001 so the test notices it loudly.
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
    /* SET DECIMALS/DATE/CENTURY: III+ defaults (set_decimals=2, date_fmt=0=AMERICAN,
     * century=0=OFF). Initialised here so fn_builtins.c reads correct values.
     * Ref: eval.h xb_ctx.set_decimals/set_date_fmt/set_century. */
    ctx.set_decimals = 2;
    ctx.set_date_fmt = 0; /* XB_DATE_AMERICAN */
    ctx.set_century  = 0; /* OFF */
    ctx.resolve      = NULL;
    ctx.user         = NULL;
    ctx.scratch      = g_scratch;
    ctx.scratch_cap  = (uint32_t)sizeof(g_scratch);
    ctx.scratch_used = 0;
    ctx.ctx_today    = (double)jdn_from_ymd(TODAY_Y, TODAY_M, TODAY_D);
    ctx.dbcur        = NULL;
    ctx.dbcur_user   = NULL;

    return xb_eval(pool, root, &ctx, out, err);
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

/* Assert a Numeric result equals want. */
static int ok_n(const char *s, double want, const char *msg)
{
    xb_val v; int err = 999;
    int rc = eval_expr(s, &v, &err);
    int good = (rc == 0 && err == XBEE_OK && v.t == XB_N && v.u.n == want);
    CHECK(good, msg);
    return good;
}

/* Assert a Date result has JDN want. */
static int ok_d(const char *s, double want, const char *msg)
{
    xb_val v; int err = 999;
    int rc = eval_expr(s, &v, &err);
    int good = (rc == 0 && err == XBEE_OK && v.t == XB_D && v.u.d == want);
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

int main(void)
{
    /* =================================================================== */
    /* A. Case functions: UPPER / LOWER  (ASCII a-z<->A-Z; others kept)     */
    /* =================================================================== */
    ok_c("UPPER('abcXYZ')", "ABCXYZ", 6, "A1: UPPER lowercases->upper");
    ok_c("UPPER('a1!z')",   "A1!Z",   4, "A2: UPPER leaves non-letters");
    ok_c("LOWER('ABCxyz')", "abcxyz", 6, "A3: LOWER uppercases->lower");
    ok_c("UPPER('')",       "",       0, "A4: UPPER('') -> ''");

    /* =================================================================== */
    /* B. Trimming: TRIM/RTRIM (trailing only), LTRIM (leading only)        */
    /* =================================================================== */
    ok_c("TRIM('AB  ')",   "AB",   2, "B1: TRIM drops trailing blanks");
    ok_c("RTRIM('AB  ')",  "AB",   2, "B2: RTRIM == TRIM (synonym)");
    ok_c("TRIM('  AB')",   "  AB", 4, "B3: TRIM keeps leading blanks");
    ok_c("LTRIM('  AB')",  "AB",   2, "B4: LTRIM drops leading blanks");
    ok_c("LTRIM('  AB ')", "AB ",  3, "B5: LTRIM keeps trailing blank");
    ok_c("TRIM('   ')",    "",     0, "B6: TRIM all-blank -> ''");

    /* =================================================================== */
    /* C. SUBSTR -- 1-BASED (the load-bearing rule + the mutant target)     */
    /*    string-functions.md line 119: SUBSTR("ABCDEF",1,3) -> "ABC"       */
    /* =================================================================== */
    ok_c("SUBSTR('ABCDEF',1,3)", "ABC", 3, "C1: SUBSTR 1-based start [MUTANT]");
    ok_c("SUBSTR('ABCDEF',2,3)", "BCD", 3, "C2: SUBSTR(s,2,3)='BCD' [MUTANT]");
    ok_c("SUBSTR('ABCDEF',4)",   "DEF", 3, "C3: SUBSTR 2-arg -> to end");
    ok_c("SUBSTR('ABCDEF',5,9)", "EF",  2, "C4: count clamped to available");
    ok_c("SUBSTR('ABCDEF',7,2)", "",    0, "C5: start past end -> ''");
    ok_c("SUBSTR('ABCDEF',2,0)", "",    0, "C6: zero count -> ''");
    err_is("SUBSTR('ABCDEF',0,3)", XBEE_SUBSTR_RANGE, "C7: start<1 -> #62");

    /* =================================================================== */
    /* D. LEN -> N                                                          */
    /* =================================================================== */
    ok_n("LEN('ABCDE')", 5.0, "D1: LEN -> 5");
    ok_n("LEN('')",      0.0, "D2: LEN('') -> 0");
    ok_n("LEN(SPACE(3))",3.0, "D3: LEN(SPACE(3)) -> 3 (nested)");

    /* =================================================================== */
    /* E. SPACE -> C (SPACE(0)='', neg -> #60, >254 -> #59)                 */
    /* =================================================================== */
    ok_c("SPACE(3)", "   ", 3, "E1: SPACE(3) -> 3 blanks");
    ok_c("SPACE(0)", "",    0, "E2: SPACE(0) -> ''");
    err_is("SPACE(-1)",  XBEE_SPACE_NEG,   "E3: SPACE(-1) -> #60");
    err_is("SPACE(300)", XBEE_SPACE_LARGE, "E4: SPACE(300) -> #59 (>254)");

    /* =================================================================== */
    /* F. CHR / ASC round-trip  (CHR 0..255 else #57; ASC leftmost byte)    */
    /* =================================================================== */
    ok_c("CHR(65)",     "A", 1, "F1: CHR(65) -> 'A'");
    ok_n("ASC('A')",    65.0,   "F2: ASC('A') -> 65");
    ok_n("ASC('ABC')",  65.0,   "F3: ASC leftmost byte only");
    ok_n("ASC('')",     0.0,    "F4: ASC('') -> 0");
    ok_n("ASC(CHR(176))",176.0, "F5: ASC(CHR(176))=176 (round-trip, CP437)");
    err_is("CHR(256)", XBEE_CHR_RANGE, "F6: CHR(256) -> #57 (out of range)");
    err_is("CHR(-1)",  XBEE_CHR_RANGE, "F7: CHR(-1) -> #57");

    /* =================================================================== */
    /* G. STR -- numeric->char, right-justified width/dec, ties->+inf       */
    /*    (rt.c dec_format minted rule; string-functions.md STR(Amt,10,2))  */
    /* =================================================================== */
    ok_c("STR(3.14159,5,2)", " 3.14", 5, "G1: STR(3.14159,5,2)=' 3.14'");
    ok_c("STR(42,4)",        "  42",  4, "G2: STR(42,4)='  42' (int, no dec)");
    ok_c("STR(2.5,2,0)",     " 3",    2, "G3: STR(2.5,2,0)=' 3' (tie -> +inf)");
    ok_c("STR(-2.5,2,0)",    "-2",    2, "G4: STR(-2.5,2,0)='-2' (tie -> +inf)");
    ok_c("STR(7,1)",         "7",     1, "G5: STR(7,1)='7'");
    /* width too small -> '*'-fill (minted overflow rule, dec_format) */
    ok_c("STR(12345,3)",     "***",   3, "G6: STR overflow -> '*'-fill");
    err_is("STR(1,0)", XBEE_STR_RANGE, "G7: STR width<1 -> #63");

    /* =================================================================== */
    /* H. VAL -- char->numeric (leading numeric portion; non-numeric -> 0)  */
    /* =================================================================== */
    ok_n("VAL('31421')",   31421.0, "H1: VAL('31421') -> 31421");
    ok_n("VAL('12.5ABC')", 12.5,    "H2: VAL stops at non-numeric");
    ok_n("VAL('ABC')",     0.0,     "H3: VAL('ABC') -> 0");
    ok_n("VAL('')",        0.0,     "H4: VAL('') -> 0");
    ok_n("VAL(STR(99,5))", 99.0,    "H5: VAL(STR(99,5))=99 (STR/VAL inverse)");

    /* =================================================================== */
    /* I. CTOD / DTOC -- date<->char (default mm/dd/yy, base-1900)          */
    /* =================================================================== */
    {
        double j850805 = (double)jdn_from_ymd(1985, 8, 5);
        double j000101 = (double)jdn_from_ymd(1900, 1, 1);  /* base-1900 '00' */
        ok_d("CTOD('08/05/85')", j850805, "I1: CTOD mm/dd/yy parse");
        ok_d("CTOD('01/01/00')", j000101, "I2: CTOD '00' -> 1900 (base-1900)");
        ok_d("CTOD('  /  /  ')", 0.0,     "I3: CTOD blank -> blank date (JDN 0)");
        ok_d("CTOD('garbage')",  0.0,     "I4: CTOD unparseable -> blank date");
        ok_d("CTOD('13/40/85')", 0.0,     "I5: CTOD invalid m/d -> blank date");
        ok_c("DTOC(CTOD('08/05/85'))", "08/05/85", 8, "I6: DTOC mm/dd/yy 8-char");
        ok_c("DTOC(CTOD('  /  /  '))", "        ", 8, "I7: DTOC(blank) -> 8 spaces");
    }

    /* =================================================================== */
    /* J. DATE / DAY / MONTH / YEAR  (DATE() from injected ctx_today)        */
    /* =================================================================== */
    {
        double today = (double)jdn_from_ymd(TODAY_Y, TODAY_M, TODAY_D);
        ok_d("DATE()",        today, "J1: DATE() -> injected today (1985-08-05)");
        ok_n("DAY(DATE())",   5.0,   "J2: DAY(DATE()) -> 5");
        ok_n("MONTH(DATE())", 8.0,   "J3: MONTH(DATE()) -> 8");
        ok_n("YEAR(DATE())",  1985.0,"J4: YEAR(DATE()) -> 1985 (full 4-digit)");
        ok_n("DAY(CTOD('  /  /  '))",   0.0, "J5: DAY(blank) -> 0");
        ok_n("MONTH(CTOD('  /  /  '))", 0.0, "J6: MONTH(blank) -> 0");
        ok_n("YEAR(CTOD('  /  /  '))",  0.0, "J7: YEAR(blank) -> 0");
    }

    /* =================================================================== */
    /* K. IIF -- type-generic, LAZY branch selection                        */
    /*    (eval.c evaluates the condition then only the selected branch)    */
    /* =================================================================== */
    ok_c("IIF(.T.,'a','b')", "a", 1, "K1: IIF(.T.,'a','b') -> 'a'");
    ok_c("IIF(.F.,'a','b')", "b", 1, "K2: IIF(.F.,'a','b') -> 'b'");
    ok_n("IIF(1<2, 10, 20)", 10.0,  "K3: IIF(1<2,10,20) -> 10");
    /* LAZY: the unselected branch (1/0 div-by-zero) MUST NOT be evaluated. */
    ok_n("IIF(.T., 5, 1/0)", 5.0,   "K4: IIF lazy -> unselected 1/0 not evaluated");
    ok_n("IIF(.F., 1/0, 7)", 7.0,   "K5: IIF lazy other branch");
    /* condition must be Logical (no truthiness, III+) */
    err_is("IIF(1,'a','b')", XBEE_NOT_LOGICAL, "K6: IIF non-L cond -> #37");
    err_is("IIF(.T.,1)",     XBEE_INVALID_ARG, "K7: IIF wrong arity -> #11");

    /* =================================================================== */
    /* L. TYPE -- 1-char type code of the (eagerly-evaluated) argument       */
    /* =================================================================== */
    ok_c("TYPE('x')",       "C", 1, "L1: TYPE char literal -> 'C'");
    ok_c("TYPE(3+4)",       "N", 1, "L2: TYPE numeric expr -> 'N'");
    ok_c("TYPE(.T.)",       "L", 1, "L3: TYPE logical -> 'L'");
    ok_c("TYPE(DATE())",    "D", 1, "L4: TYPE date -> 'D'");
    ok_c("TYPE('AB'$'XAB')","L", 1, "L5: TYPE of a $ expr -> 'L'");

    /* =================================================================== */
    /* M. Error cells: unknown fn, DTOS (not in III+), arity, type          */
    /* =================================================================== */
    err_is("NOSUCHFN(1)",      XBEE_INVALID_FN, "M1: unknown fn -> #31");
    err_is("DTOS(DATE())",     XBEE_INVALID_FN, "M2: DTOS not in III+ -> #31");
    err_is("UPPER('a','b')",   XBEE_INVALID_ARG,"M3: UPPER wrong arity -> #11");
    err_is("UPPER(5)",         XBEE_MISMATCH,   "M4: UPPER non-char arg -> #9");
    err_is("LEN(5)",           XBEE_MISMATCH,   "M5: LEN non-char arg -> #9");
    err_is("CHR('a')",         XBEE_MISMATCH,   "M6: CHR non-num arg -> #9");
    err_is("DAY('x')",         XBEE_MISMATCH,   "M7: DAY non-date arg -> #9");
    err_is("DATE(1)",          XBEE_INVALID_ARG,"M8: DATE() takes no args -> #11");

    /* =================================================================== */
    /* N. Realistic composed idiom: 'X='+STR(n,2) (the C+N bridge)          */
    /*    string-functions.md: must STR() before '+'; bare C+N is #9 (S3.3) */
    /* =================================================================== */
    ok_c("'X=' + STR(7,2)",            "X= 7",   4, "N1: C + STR(n) bridge");
    ok_c("'D:' + DTOC(CTOD('01/02/85'))", "D:01/02/85", 10, "N2: C + DTOC bridge");
    ok_c("UPPER(TRIM('ab  '))",        "AB",     2, "N3: UPPER(TRIM(...)) nested");

    return TEST_SUMMARY("test-xbase-fn-a");
}
