/*
 * harness/diff/dbf_diff/test_xbase_eval.c -- unit oracle for eval.c (S3.3).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Uses the seed
 * test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY). A non-zero exit
 * on any failed check ensures the make gate can never false-green (Law 2: the
 * oracle is the truth).
 *
 * It lexes (S3.1 lex.c) -> parses (S3.2 parse.c) -> evaluates (S3.3 eval.c)
 * each expression and asserts the RESULT TYPE + VALUE (for success cells) and
 * the EXACT dBASE error CODE (for every result:"error" cell). Coverage is
 * driven cell-by-cell from spec/samir/xbase_coercion.json operator_coercion +
 * the rules R_* + the SET_EXACT mode -- a C mirror of the table, cited.
 *
 * -------------------------------------------------------------------------
 * THE HAZARD (Law 1 + xbase_coercion.json not_in_iii_plus + mint-002):
 *   dBASE III+ 1.1 has NO auto-stringification. C+N is error #9 "Data type
 *   mismatch.", NOT "A1". Modern dbase.com docs are WRONG for III+. The
 *   headline assertion is: lex+parse+eval of  "A" + 1  -> error, code == 9.
 *
 * MUTATION PROOF (Rule 6):
 *   Compile with -DXB_MUTATE_EVAL to activate the single perturbation in
 *   eval.c: the C+N cell SUCCEEDS (returns the C operand) instead of erroring.
 *   The "C+N -> error #9" assertion (section H) then goes RED, proving the
 *   assertion catches the regression. This is the strongest mutant because the
 *   HAZARD cell is precisely the one the spec warns about.
 *
 * Compile + run (self-grade, host):
 *   gcc -std=c11 -Wall -Wextra -Werror -Iseed -Ios/samir/include \
 *       os/samir/core/eval.c os/samir/core/parse.c os/samir/core/lex.c \
 *       os/samir/core/value.c os/samir/core/rt.c \
 *       harness/diff/dbf_diff/test_xbase_eval.c \
 *       -o /tmp/test_xbase_eval && /tmp/test_xbase_eval
 *
 * Mutant (must go RED on section H):
 *   gcc -std=c11 -Wall -Wextra -Werror -DXB_MUTATE_EVAL -Iseed \
 *       -Ios/samir/include <same srcs> \
 *       harness/diff/dbf_diff/test_xbase_eval.c \
 *       -o /tmp/test_xbase_eval_mut ; /tmp/test_xbase_eval_mut ; echo $?
 *
 * Freestanding compile check (eval.c only):
 *   gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -fno-pic \
 *       -std=c11 -Wall -Wextra -Werror -Ios/samir/include \
 *       -c os/samir/core/eval.c -o /tmp/eval_free.o
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md sec.5 S3.3 oracle contract
 *   - spec/samir/xbase_coercion.json (THE CONTRACT -- every cell asserted)
 *   - spec/samir/dbase_msg_codes.tsv (#9/#27/#37/#39/#45)
 *   - ../dbase3-decomp/re/mint-results-002.md (C+N=#9; ""$"ABC"=.F.;
 *     "ab"="a"=.T. / "a"="ab"=.F.; SET EXACT default OFF)
 *   - os/samir/include/samir/eval.h (xb_ctx / xb_eval API)
 *   - seed/test_assert.h (harness idiom)
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "test_assert.h"        /* seed/, on -Iseed                       */
#include "samir/eval.h"         /* os/samir/include/, on -Ios/samir/include */
#include "samir/value.h"
#include "samir/rt.h"

TEST_HARNESS();

#define TBUF 64    /* token buffer */
#define NBUF 64    /* node pool    */

/* -------------------------------------------------------------------------
 * Identifier resolver hook: binds a tiny fixed symbol table so the test can
 * feed typed operands (C/N/D/L) of every type into the evaluator. Phase 5
 * supplies the real field/memvar binding; this is the test stand-in.
 *
 * Symbols (case-insensitive, like dBASE):
 *   CSTR  -> C "AB"       NUM   -> N 1        LOGT  -> L .T.   LOGF -> L .F.
 *   DJAN  -> D 1985-08-05 (JDN 2446283)       DSEP  -> D 1985-09-07 (2446316)
 *   DBLANK-> D blank (JDN 0)
 *   CAB   -> C "AB"       CA    -> C "A"      CABSP -> C "AB  " (trailing blanks)
 *   CABC  -> C "ABC"      CBC   -> C "BC"     CEMPTY-> C ""
 * ------------------------------------------------------------------------- */
static int test_resolve(void *user, const char *name, uint16_t len, xb_val *out)
{
    char ub[32];
    uint16_t i;
    (void)user;
    if (len >= sizeof(ub)) return 1;
    for (i = 0; i < len; i++) {
        char c = name[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        ub[i] = c;
    }
    ub[len] = 0;

    if (strcmp(ub, "NUM")   == 0) { *out = xb_n(1.0);            return 0; }
    if (strcmp(ub, "LOGT")  == 0) { *out = xb_l(1);              return 0; }
    if (strcmp(ub, "LOGF")  == 0) { *out = xb_l(0);              return 0; }
    if (strcmp(ub, "DJAN")  == 0) { *out = xb_d((double)jdn_from_ymd(1985, 8, 5));  return 0; }
    if (strcmp(ub, "DSEP")  == 0) { *out = xb_d((double)jdn_from_ymd(1985, 9, 7));  return 0; }
    if (strcmp(ub, "DBLANK")== 0) { *out = xb_d(0.0);            return 0; }
    if (strcmp(ub, "CSTR")  == 0) { *out = xb_c("AB", 2);        return 0; }
    if (strcmp(ub, "CAB")   == 0) { *out = xb_c("AB", 2);        return 0; }
    if (strcmp(ub, "CA")    == 0) { *out = xb_c("A", 1);         return 0; }
    if (strcmp(ub, "CABSP") == 0) { *out = xb_c("AB  ", 4);      return 0; }
    if (strcmp(ub, "CABC")  == 0) { *out = xb_c("ABC", 3);       return 0; }
    if (strcmp(ub, "CBC")   == 0) { *out = xb_c("BC", 2);        return 0; }
    if (strcmp(ub, "CEMPTY")== 0) { *out = xb_c("", 0);          return 0; }
    return 1; /* not found -> XBEE_UNBOUND */
}

/* -------------------------------------------------------------------------
 * Evaluate a source expression with a given SET EXACT state. Returns the
 * xb_eval rc; fills *out and *err. Lex + parse are expected to succeed; a lex
 * or parse failure is reported as rc -1000 so the test notices it loudly.
 * ------------------------------------------------------------------------- */
static char g_scratch[512];

static int eval_expr(const char *s, int set_exact, xb_val *out, int *err)
{
    xb_token toks[TBUF];
    xb_node  pool[NBUF];
    xb_ctx   ctx;
    int lerr = 0, perr = 0, nt, root;

    nt = xb_lex(s, (uint32_t)strlen(s), toks, TBUF, &lerr);
    if (nt < 0) { *err = -1000; *out = xb_u(); return -1000; }
    root = xb_parse(toks, (uint32_t)nt, pool, NBUF, &perr);
    if (root < 0) { *err = -1001; *out = xb_u(); return -1001; }

    ctx.set_exact   = set_exact;
    ctx.resolve     = test_resolve;
    ctx.user        = NULL;
    ctx.scratch     = g_scratch;
    ctx.scratch_cap = (uint32_t)sizeof(g_scratch);
    ctx.scratch_used = 0;

    return xb_eval(pool, root, &ctx, out, err);
}

/* Helpers: assert success with a given type / value. */
static int ok_n(const char *s, double want, const char *msg)
{
    xb_val v; int err = 999;
    int rc = eval_expr(s, 0, &v, &err);
    int good = (rc == 0 && err == XBEE_OK && v.t == XB_N && v.u.n == want);
    CHECK(good, msg);
    return good;
}

static int ok_l(const char *s, int set_exact, int want, const char *msg)
{
    xb_val v; int err = 999;
    int rc = eval_expr(s, set_exact, &v, &err);
    int good = (rc == 0 && err == XBEE_OK && v.t == XB_L && v.u.l == (want ? 1 : 0));
    CHECK(good, msg);
    return good;
}

/* Assert an error cell: eval must FAIL with exactly `code`. */
static int err_is(const char *s, int set_exact, int code, const char *msg)
{
    xb_val v; int err = 999;
    int rc = eval_expr(s, set_exact, &v, &err);
    int good = (rc != 0 && err == code);
    CHECK(good, msg);
    return good;
}

int main(void)
{
    xb_val v; int err;

    /* =================================================================== */
    /* A. N arithmetic -> N   (xbase_coercion.json N{+ - * / ^ **}N -> N)    */
    /* =================================================================== */
    ok_n("2 + 3",   5.0,   "A1: N+N -> 5  [cell N+N]");
    ok_n("7 - 4",   3.0,   "A2: N-N -> 3  [cell N-N]");
    ok_n("6 * 7",  42.0,   "A3: N*N -> 42 [cell N*N]");
    ok_n("9 / 2",   4.5,   "A4: N/N -> 4.5 [cell N/N]");
    ok_n("2 ^ 10", 1024.0, "A5: N^N -> 1024 [cell N^N]");
    ok_n("2 ** 3",  8.0,   "A6: N**N -> 8 [cell N**N; ** == ^]");
    /* mint-002 precedence consequences flow through eval: */
    ok_n("2 ^ 3 ^ 2", 64.0, "A7: 2^3^2 -> 64 (^ left-assoc, mint-002)");
    ok_n("-2 ^ 2",     4.0, "A8: -2^2 -> 4 (unary tighter than ^, mint-002)");
    ok_n("2 + 3 * 4",  14.0, "A9: 2+3*4 -> 14 (mul tighter)");
    /* unary minus on N (xbase_coercion unary -N -> N): */
    ok_n("- 5", -5.0,  "A10: unary -5 -> -5");
    /* div-by-zero: DECISION -> XBEE_NUM_OVERFLOW (#39); mint-002 1/0 overflow */
    err_is("1 / 0", 0, XBEE_NUM_OVERFLOW, "A11: 1/0 -> num_overflow #39 [JSON note]");

    /* =================================================================== */
    /* B. C + C -> C concat (R_concat_plus: trailing blanks kept in place)   */
    /* =================================================================== */
    {
        int rc = eval_expr("'AB' + 'CD'", 0, &v, &err);
        CHECK(rc == 0 && err == XBEE_OK && v.t == XB_C && v.u.c.len == 4 &&
              memcmp(v.u.c.p, "ABCD", 4) == 0,
              "B1: 'AB'+'CD' -> 'ABCD' [cell C+C, R_concat_plus]");

        /* trailing blanks kept IN PLACE (not collapsed) */
        rc = eval_expr("'AB ' + 'C'", 0, &v, &err);
        CHECK(rc == 0 && err == XBEE_OK && v.t == XB_C && v.u.c.len == 4 &&
              memcmp(v.u.c.p, "AB C", 4) == 0,
              "B2: 'AB '+'C' -> 'AB C' (trailing blank kept in place)");
    }

    /* =================================================================== */
    /* C. C - C -> C reloc (R_concat_minus: LHS trailing blanks moved to end) */
    /* =================================================================== */
    {
        /* 'AB  ' - 'CD' : LHS trailing blanks (2) moved to end; len preserved
         * = 6. Result = "AB" ++ "CD" ++ "  " = "ABCD  ". */
        int rc = eval_expr("'AB  ' - 'CD'", 0, &v, &err);
        CHECK(rc == 0 && err == XBEE_OK && v.t == XB_C && v.u.c.len == 6 &&
              memcmp(v.u.c.p, "ABCD  ", 6) == 0,
              "C1: 'AB  '-'CD' -> 'ABCD  ' [cell C-C, R_concat_minus]");

        /* no trailing blanks on LHS -> plain concat, length preserved */
        rc = eval_expr("'AB' - 'CD'", 0, &v, &err);
        CHECK(rc == 0 && err == XBEE_OK && v.t == XB_C && v.u.c.len == 4 &&
              memcmp(v.u.c.p, "ABCD", 4) == 0,
              "C2: 'AB'-'CD' -> 'ABCD' (no LHS blanks)");
    }

    /* =================================================================== */
    /* D. Date arithmetic (R_date_plus_num / R_date_minus_date)              */
    /*    Uses the resolver-bound dates DJAN (1985-08-05) / DSEP (1985-09-07)*/
    /* =================================================================== */
    {
        double jjan = (double)jdn_from_ymd(1985, 8, 5);
        double jsep = (double)jdn_from_ymd(1985, 9, 7);

        /* D + N -> D (shift days forward) [cell D+N] */
        int rc = eval_expr("DJAN + 10", 0, &v, &err);
        CHECK(rc == 0 && err == XBEE_OK && v.t == XB_D && v.u.d == jjan + 10.0,
              "D1: D+N -> D shifted [cell D+N, R_date_plus_num]");

        /* N + D -> D [cell N+D] */
        rc = eval_expr("10 + DJAN", 0, &v, &err);
        CHECK(rc == 0 && err == XBEE_OK && v.t == XB_D && v.u.d == jjan + 10.0,
              "D2: N+D -> D shifted [cell N+D, R_date_plus_num]");

        /* D - N -> D (shift back) [cell D-N] */
        rc = eval_expr("DSEP - 5", 0, &v, &err);
        CHECK(rc == 0 && err == XBEE_OK && v.t == XB_D && v.u.d == jsep - 5.0,
              "D3: D-N -> D shifted back [cell D-N, R_date_plus_num]");

        /* D - D -> N (#days) [cell D-D, R_date_minus_date] */
        rc = eval_expr("DSEP - DJAN", 0, &v, &err);
        CHECK(rc == 0 && err == XBEE_OK && v.t == XB_N && v.u.n == (jsep - jjan),
              "D4: D-D -> N (#days) [cell D-D, R_date_minus_date]");

        /* D-D touching a blank date -> 0 (R_date_minus_date) */
        rc = eval_expr("DSEP - DBLANK", 0, &v, &err);
        CHECK(rc == 0 && err == XBEE_OK && v.t == XB_N && v.u.n == 0.0,
              "D5: D-blankD -> 0 (R_date_minus_date)");

        /* blank D + N stays blank (R_date_plus_num) */
        rc = eval_expr("DBLANK + 5", 0, &v, &err);
        CHECK(rc == 0 && err == XBEE_OK && v.t == XB_D && v.u.d == 0.0,
              "D6: blankD+N stays blank (R_date_plus_num)");
    }

    /* =================================================================== */
    /* E. Relational -> L; same-type cells (= <> # < > <= >=)                */
    /* =================================================================== */
    /* N = N / N <> N (cell N=N) */
    ok_l("3 = 3",  0, 1, "E1: 3=3 -> .T. [cell N=N]");
    ok_l("3 = 4",  0, 0, "E2: 3=4 -> .F. [cell N=N]");
    ok_l("3 <> 4", 0, 1, "E3: 3<>4 -> .T. [cell C<> generalized to N]");
    ok_l("3 # 3",  0, 0, "E4: 3#3 -> .F. (# alias of <>)");
    /* N ordering (cell N<N) */
    ok_l("3 < 4",  0, 1, "E5: 3<4 -> .T. [cell N<N]");
    ok_l("4 <= 4", 0, 1, "E6: 4<=4 -> .T.");
    ok_l("5 >= 9", 0, 0, "E7: 5>=9 -> .F.");
    ok_l("9 > 2",  0, 1, "E8: 9>2 -> .T.");
    /* L = L (cell L=L) and L ordering .F. < .T. (cell L<L) */
    ok_l("LOGT = LOGT", 0, 1, "E9: .T.=.T. -> .T. [cell L=L]");
    ok_l("LOGF < LOGT", 0, 1, "E10: .F.<.T. -> .T. [cell L<L, .F.<.T.]");
    ok_l("LOGT < LOGF", 0, 0, "E11: .T.<.F. -> .F. [cell L<L]");
    /* D = D (cell D=D) */
    ok_l("DJAN = DJAN", 0, 1, "E12: D=D equal -> .T. [cell D=D]");
    ok_l("DJAN = DSEP", 0, 0, "E13: D=D unequal -> .F. [cell D=D]");
    /* D < D (cell D<D) + R_blankdate_high: blank date is GREATER than any */
    ok_l("DJAN < DSEP",   0, 1, "E14: D<D ordering -> .T. [cell D<D]");
    ok_l("DSEP < DBLANK", 0, 1, "E15: D<blankD -> .T. (R_blankdate_high: blank is GREATER)");
    ok_l("DBLANK > DSEP", 0, 1, "E16: blankD>D -> .T. (R_blankdate_high)");
    ok_l("DBLANK < DSEP", 0, 0, "E17: blankD<D -> .F. (R_blankdate_high)");

    /* =================================================================== */
    /* F. C = C SET EXACT semantics (cell C=C: R_begins_with / R_exact_blankpad)*/
    /* =================================================================== */
    /* EXACT OFF (default): LEFT begins with RIGHT, directional. mint-002:    */
    ok_l("'ab' = 'a'",  0, 1, "F1: EXACT OFF 'ab'='a' -> .T. [R_begins_with, mint-002]");
    ok_l("'a' = 'ab'",  0, 0, "F2: EXACT OFF 'a'='ab' -> .F. (directional, mint-002)");
    ok_l("'ABC' = 'ABC'", 0, 1, "F3: EXACT OFF equal -> .T.");
    ok_l("'ABC' = ''",  0, 1, "F4: EXACT OFF rhs '' matches all [R_begins_with]");
    ok_l("'ABC' = 'X'", 0, 0, "F5: EXACT OFF 'ABC'='X' -> .F.");
    /* <> / # follow SET EXACT (negation of =) */
    ok_l("'ab' <> 'a'", 0, 0, "F6: EXACT OFF 'ab'<>'a' -> .F. (negation)");
    ok_l("'a' # 'ab'",  0, 1, "F7: EXACT OFF 'a'#'ab' -> .T. (negation)");
    /* EXACT ON: equal ignoring trailing blanks, case-sensitive (R_exact_blankpad)*/
    ok_l("'ab' = 'a'",   1, 0, "F8: EXACT ON 'ab'='a' -> .F. [R_exact_blankpad]");
    ok_l("'AB' = 'AB '", 1, 1, "F9: EXACT ON 'AB'='AB ' -> .T. (trailing blanks ignored)");
    ok_l("'AB ' = 'AB'", 1, 1, "F10: EXACT ON 'AB '='AB' -> .T. (either side)");
    ok_l("'AB' = 'ab'",  1, 0, "F11: EXACT ON case-sensitive 'AB'!='ab' -> .F.");
    /* C ordering (cell C<C) ignores SET EXACT, CP437 byte order */
    ok_l("'A' < 'B'", 0, 1, "F12: 'A'<'B' -> .T. [cell C<C, CP437]");
    ok_l("'AB' > 'A'", 0, 1, "F13: 'AB'>'A' -> .T. (longer with same prefix)");

    /* =================================================================== */
    /* G. $ substring -> L (cell C$C, R_substr: ignores SET EXACT)           */
    /* =================================================================== */
    ok_l("'BC' $ 'ABCD'", 0, 1, "G1: 'BC'$'ABCD' -> .T. [cell C$C, R_substr]");
    ok_l("'XY' $ 'ABCD'", 0, 0, "G2: 'XY'$'ABCD' -> .F.");
    ok_l("'' $ 'X'",      0, 0, "G3: ''$'X' -> .F. (empty NOT contained, mint-002)");
    ok_l("'ABCD' $ 'BC'", 0, 0, "G4: 'ABCD'$'BC' -> .F. (needle longer than hay)");
    /* $ ignores SET EXACT (same result with EXACT ON) -- R_substr */
    ok_l("'BC' $ 'ABCD'", 1, 1, "G5: $ ignores SET EXACT (EXACT ON same .T.) [R_substr]");

    /* =================================================================== */
    /* H. THE HAZARD: C+N / N+C -> error #9 (NO auto-stringify)              */
    /*    xbase_coercion.json C+N result:"error" error:"mismatch". MUTANT    */
    /*    (-DXB_MUTATE_EVAL) makes C+N succeed -> these go RED.              */
    /* =================================================================== */
    err_is("'A' + 1", 0, XBEE_MISMATCH, "H1: HAZARD C+N -> error #9 [cell C+N, mint-002 'A'+1]");
    err_is("1 + 'A'", 0, XBEE_MISMATCH, "H2: HAZARD N+C -> error #9 [cell N+C]");

    /* =================================================================== */
    /* I. Every remaining result:"error" operator_coercion cell             */
    /* =================================================================== */
    /* + error cells: C+D, D+D */
    err_is("'A' + DJAN",   0, XBEE_MISMATCH, "I1: C+D -> error #9 [cell C+D]");
    err_is("DJAN + DSEP",  0, XBEE_MISMATCH, "I2: D+D -> error #9 [cell D+D, mint-002]");
    /* - error cells: C-N, N-D */
    err_is("'A' - 5",      0, XBEE_MISMATCH, "I3: C-N -> error #9 [cell C-N, mint-002 'abc'-5]");
    err_is("5 - DJAN",     0, XBEE_MISMATCH, "I4: N-D -> error #9 [cell N-D; only D-N sanctioned]");
    /* relational cross-type cell: N<C (and the general R_order_same_type) */
    err_is("1 < 'A'",      0, XBEE_MISMATCH, "I5: N<C -> error #9 [cell N<C, R_order_same_type]");
    err_is("DJAN < 'x'",   0, XBEE_MISMATCH, "I6: D<C -> error #9 (mint-002 DATE()<'x')");
    /* $ cell with N: N$C */
    err_is("1 $ 'AB'",     0, XBEE_MISMATCH, "I7: N$C -> error #9 [cell N$C, mint-002 'x'$5]");
    /* logical cell with non-L operand: N .AND. L */
    err_is("1 .AND. LOGT", 0, XBEE_NOT_LOGICAL, "I8: N.AND.L -> not_logical #37 [cell N.AND.L, R_no_truthiness]");

    /* =================================================================== */
    /* J. Logical .AND. / .OR. / .NOT. (cells L.AND.L, L.OR.L; R_no_truthiness)*/
    /* =================================================================== */
    ok_l("LOGT .AND. LOGT", 0, 1, "J1: .T..AND..T. -> .T. [cell L.AND.L]");
    ok_l("LOGT .AND. LOGF", 0, 0, "J2: .T..AND..F. -> .F. [cell L.AND.L]");
    ok_l("LOGF .OR. LOGT",  0, 1, "J3: .F..OR..T. -> .T. [cell L.OR.L]");
    ok_l("LOGF .OR. LOGF",  0, 0, "J4: .F..OR..F. -> .F. [cell L.OR.L]");
    ok_l(".NOT. LOGT",      0, 0, "J5: .NOT..T. -> .F. [R_no_truthiness]");
    ok_l(".NOT. LOGF",      0, 1, "J6: .NOT..F. -> .T. [R_no_truthiness]");
    /* .OR. with non-L operand -> not_logical (R_no_truthiness) */
    err_is("LOGT .OR. 1",   0, XBEE_NOT_LOGICAL, "J7: L.OR.N -> not_logical #37 [R_no_truthiness]");
    /* .NOT. on non-L -> not_logical (R_no_truthiness) */
    err_is(".NOT. 1",       0, XBEE_NOT_LOGICAL, "J8: .NOT. N -> not_logical #37 [R_no_truthiness]");
    /* realistic mixed: (NUM>0) .AND. .NOT. LOGF -> .T. */
    ok_l("NUM > 0 .AND. .NOT. LOGF", 0, 1, "J9: (NUM>0).AND.(.NOT..F.) -> .T. (CLRDEP idiom)");

    /* =================================================================== */
    /* K. Unary error cells + identifier binding / unbound                   */
    /* =================================================================== */
    /* unary minus on non-N -> mismatch (xbase_coercion same-type) */
    err_is("- 'A'",  0, XBEE_MISMATCH, "K1: unary -C -> error #9 (unary minus needs N)");
    err_is("- LOGT", 0, XBEE_MISMATCH, "K2: unary -L -> error #9");
    /* identifier resolution: CSTR -> C "AB" */
    {
        int rc = eval_expr("CSTR", 0, &v, &err);
        CHECK(rc == 0 && err == XBEE_OK && v.t == XB_C && v.u.c.len == 2 &&
              memcmp(v.u.c.p, "AB", 2) == 0,
              "K3: identifier CSTR resolves to C 'AB'");
    }
    /* unbound identifier (no symbol) -> XBEE_UNBOUND (fail loud) */
    err_is("NOSUCHFIELD", 0, XBEE_UNBOUND, "K4: unbound ident -> XBEE_UNBOUND (fail loud)");
    /* identifier with no resolver at all -> XBEE_UNBOUND */
    {
        xb_token toks[TBUF]; xb_node pool[NBUF]; xb_ctx ctx;
        int lerr = 0, perr = 0, nt, root, rc;
        nt = xb_lex("NUM", 3, toks, TBUF, &lerr);
        root = xb_parse(toks, (uint32_t)nt, pool, NBUF, &perr);
        ctx.set_exact = 0; ctx.resolve = NULL; ctx.user = NULL;
        ctx.scratch = NULL; ctx.scratch_cap = 0; ctx.scratch_used = 0;
        rc = xb_eval(pool, root, &ctx, &v, &err);
        CHECK(rc != 0 && err == XBEE_UNBOUND,
              "K5: identifier with NULL resolver -> XBEE_UNBOUND");
    }

    /* =================================================================== */
    /* L. Scratch arena exhaustion (C+C with no room) -> XBEE_SCRATCH_FULL   */
    /*    Fail loud (Rule 2): a C result that does not fit must not overflow. */
    /* =================================================================== */
    {
        xb_token toks[TBUF]; xb_node pool[NBUF]; xb_ctx ctx;
        char tiny[2];
        int lerr = 0, perr = 0, nt, root, rc;
        nt = xb_lex("'AB' + 'CD'", 11, toks, TBUF, &lerr);
        root = xb_parse(toks, (uint32_t)nt, pool, NBUF, &perr);
        ctx.set_exact = 0; ctx.resolve = test_resolve; ctx.user = NULL;
        ctx.scratch = tiny; ctx.scratch_cap = (uint32_t)sizeof(tiny);
        ctx.scratch_used = 0;
        rc = xb_eval(pool, root, &ctx, &v, &err);
        CHECK(rc != 0 && err == XBEE_SCRATCH_FULL,
              "L1: C+C with no scratch room -> XBEE_SCRATCH_FULL (fail loud)");
    }

    return TEST_SUMMARY("test-xbase-eval");
}
