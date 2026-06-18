/*
 * harness/diff/dbf_diff/test_xbase_fn_c.c -- unit oracle for built-in
 * functions C (initech-7az.12): LEFT RIGHT STUFF REPLICATE AT
 *                               ISALPHA ISUPPER ISLOWER (TRANSFORM GATED).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Uses seed/test_assert.h
 * (CHECK / TEST_HARNESS / TEST_SUMMARY). A non-zero exit on any failed check
 * ensures the make gate can never false-green (Law 2: oracle is the truth).
 *
 * Each case drives the FULL pipeline: lex -> parse -> eval (via xb_call_builtin
 * through the xb_eval XBN_CALL path), asserting RESULT TYPE + VALUE for
 * success cells and the EXACT dBASE catalog CODE for error cells.
 *
 * Functions exercised (all period-grounded from HELP.DBS strings):
 *   LEFT     HELP.DBS.strings.txt:1338  [verified: III+ surface]
 *   RIGHT    HELP.DBS.strings.txt:1349  [verified: III+ surface]
 *   STUFF    HELP.DBS.strings.txt:1353  [verified: III+ surface]
 *   REPLICATE HELP.DBS.strings.txt:1344 [verified: III+ surface]
 *   AT       HELP.DBS.strings.txt:1326  [verified: III+ surface]
 *   ISALPHA  HELP.DBS.strings.txt:1330  [verified: III+ surface]
 *   ISUPPER  HELP.DBS.strings.txt:1336  [verified: III+ surface]
 *   ISLOWER  HELP.DBS.strings.txt:1331  [verified: III+ surface]
 *
 * GATED / LOUD-SKIPPED:
 *   TRANSFORM -- exists in III+ (mint-results-002.md: TRANSFORM(-570,"9999")
 *     -> "-570", [verified: C2.TXT]). The full PICTURE/FUNCTION template
 *     language (@ clauses, literal chars, comma groups) requires a picture-walk
 *     formatter that is not yet corpus-verified at the detail level needed for
 *     mutation-proof assertions. This bead loud-skips TRANSFORM entirely and
 *     records it as a follow-up. The function IS registered in fn_builtins.c
 *     (pure '9' picture, no @ clause) but is not asserted here.
 *     Follow-up: initech-7az.TRANSFORM (GATED bead, pending MINT/DISASM).
 *
 * MUTATION PROOF (Rule 6):
 *   Compile with -DXB_MUTATE_FN_LEFT to activate the perturbation in
 *   fn_builtins.c: LEFT returns n+1 characters instead of n, so
 *   LEFT("ABCDE",3) returns "ABCD" (4 chars) instead of "ABC" (3 chars).
 *   The grounded assertion LEFT("ABCDE",3)="ABC" then goes RED, proving the
 *   oracle catches an off-by-one in the leftmost-n-characters rule
 *   (HELP.DBS:1338, a SETTLED non-GATED case).
 *   Mutant macro: XB_MUTATE_FN_LEFT.
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/archive/golden-mined/HELP.DBS.strings.txt
 *       @STRING FUNCTIONS (1321-1332) + @STR FUNC 2 (1333-1345) +
 *       @STR FUNC 3 (1346-1358): the period-exact III+ string function surface.
 *   - ../dbase3-decomp/re/mint-results-002.md: TRANSFORM confirmed to exist;
 *       ""$"ABC"=.F. (AT empty-needle behavior consistent).
 *   - spec/samir/dbase_msg_codes.tsv: error codes #9/#11.
 *   - os/samir/include/samir/eval.h: xb_ctx / xb_eval / xb_call_builtin API.
 *   - os/samir/include/samir/value.h: xb_val / XB_C / XB_L / xb_c / xb_l.
 *   - seed/test_assert.h: CHECK / TEST_HARNESS / TEST_SUMMARY harness idiom.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
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

/* Injected "today": 1985-08-05 (JDN 2446283). Reproducible (Rule 11).
 * Same fixture as test_xbase_fn_a.c and test_xbase_fn_b.c. */
#define TODAY_Y 1985
#define TODAY_M 8
#define TODAY_D 5

static char g_scratch[1024];

/*
 * eval_expr -- lex + parse + eval with EXACT OFF and injected today.
 * Returns -1000 on lex failure, -1001 on parse failure.
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
    /* III+ defaults for formatter context fields (eval.h xb_ctx). */
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

/* Assert a Numeric result equals want (exact double equality). */
static int ok_n(const char *s, double want, const char *msg)
{
    xb_val v; int err = 999;
    int rc = eval_expr(s, &v, &err);
    int good = (rc == 0 && err == XBEE_OK && v.t == XB_N && v.u.n == want);
    CHECK(good, msg);
    return good;
}

/* Assert a Logical result equals want (1=.T., 0=.F.). */
static int ok_l(const char *s, int want, const char *msg)
{
    xb_val v; int err = 999;
    int rc = eval_expr(s, &v, &err);
    int good = (rc == 0 && err == XBEE_OK && v.t == XB_L &&
                (int)v.u.l == (want ? 1 : 0));
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
    /* ================================================================ */
    /* A. LEFT                                                           */
    /*    Ref: HELP.DBS:1338 "leftmost <expN> characters from <expC>"   */
    /*    [verified: HELP.DBS @STR FUNC 2 line 1338]                    */
    /* ================================================================ */
    ok_c("LEFT('ABCDE',3)",  "ABC", 3, "A1: LEFT basic [MUTANT TARGET]");
    ok_c("LEFT('ABCDE',5)",  "ABCDE", 5, "A2: LEFT n=len -> full string");
    ok_c("LEFT('ABCDE',9)",  "ABCDE", 5, "A3: LEFT n>len -> clamp to full");
    ok_c("LEFT('ABCDE',0)",  "",     0, "A4: LEFT n=0 -> ''");
    ok_c("LEFT('ABCDE',1)",  "A",    1, "A5: LEFT first char");
    ok_c("LEFT('',3)",       "",     0, "A6: LEFT empty string -> ''");
    err_is("LEFT('A',1,2)",  XBEE_INVALID_ARG, "A7: LEFT wrong arity -> #11");
    err_is("LEFT(3,'9')",    XBEE_MISMATCH,    "A8: LEFT bad arg types -> #9");

    /* ================================================================ */
    /* B. RIGHT                                                          */
    /*    Ref: HELP.DBS:1349 "<expN> characters from the right of <expC>"*/
    /*    [verified: HELP.DBS @STR FUNC 3 line 1349]                    */
    /* ================================================================ */
    ok_c("RIGHT('ABCDE',3)", "CDE",   3, "B1: RIGHT basic");
    ok_c("RIGHT('ABCDE',5)", "ABCDE", 5, "B2: RIGHT n=len -> full string");
    ok_c("RIGHT('ABCDE',9)", "ABCDE", 5, "B3: RIGHT n>len -> clamp to full");
    ok_c("RIGHT('ABCDE',0)", "",      0, "B4: RIGHT n=0 -> ''");
    ok_c("RIGHT('ABCDE',1)", "E",     1, "B5: RIGHT last char");
    ok_c("RIGHT('',2)",      "",      0, "B6: RIGHT empty string -> ''");
    err_is("RIGHT(3,'9')",   XBEE_MISMATCH, "B7: RIGHT bad arg types -> #9");

    /* ================================================================ */
    /* C. REPLICATE                                                       */
    /*    Ref: HELP.DBS:1344 "<expN> repetitions of the <expC>"         */
    /*    [verified: HELP.DBS @STR FUNC 2 line 1344]                    */
    /* ================================================================ */
    ok_c("REPLICATE('AB',3)", "ABABAB", 6, "C1: REPLICATE 3x");
    ok_c("REPLICATE('X',5)",  "XXXXX",  5, "C2: REPLICATE single char");
    ok_c("REPLICATE('A',1)",  "A",      1, "C3: REPLICATE n=1");
    ok_c("REPLICATE('AB',0)", "",       0, "C4: REPLICATE n=0 -> ''");
    ok_c("REPLICATE('',5)",   "",       0, "C5: REPLICATE empty src -> ''");
    err_is("REPLICATE(3,2)",  XBEE_MISMATCH,    "C6: REPLICATE bad types -> #9");
    err_is("REPLICATE('A')",  XBEE_INVALID_ARG, "C7: REPLICATE wrong arity -> #11");

    /* ================================================================ */
    /* D. AT                                                             */
    /*    Ref: HELP.DBS:1326 "position of <expC1> inside <expC2>;       */
    /*    Zero if <expC1> isn't there."                                  */
    /*    [verified: HELP.DBS @STRING FUNCTIONS line 1326]              */
    /* ================================================================ */
    ok_n("AT('o','World')",   2.0, "D1: AT basic (1-based pos)");
    ok_n("AT('z','World')",   0.0, "D2: AT not found -> 0");
    ok_n("AT('Wor','World')", 1.0, "D3: AT needle at start");
    ok_n("AT('rld','World')", 3.0, "D4: AT needle at end");
    ok_n("AT('World','World')", 1.0, "D5: AT needle=haystack");
    ok_n("AT('','World')",    0.0, "D6: AT empty needle -> 0");
    ok_n("AT('ABC','AB')",    0.0, "D7: AT needle longer than haystack -> 0");
    /* Case-sensitive (consistent with $ operator, same source). */
    ok_n("AT('w','World')",   0.0, "D8: AT case-sensitive (lowercase not found)");
    ok_n("AT('W','World')",   1.0, "D9: AT case-sensitive (uppercase found)");
    err_is("AT('A',3)",       XBEE_MISMATCH,    "D10: AT wrong type -> #9");
    err_is("AT('A')",         XBEE_INVALID_ARG, "D11: AT wrong arity -> #11");

    /* ================================================================ */
    /* E. STUFF                                                          */
    /*    Ref: HELP.DBS:1353 "Overlay <expC1> with <expC2>, starting at */
    /*    <expN1> for <expN2> characters."                               */
    /*    [verified: HELP.DBS @STR FUNC 3 line 1353]                    */
    /* ================================================================ */
    /* Core: delete 3 chars at pos 2, insert "XY". */
    ok_c("STUFF('ABCDE',2,3,'XY')",  "AXYE",  4, "E1: STUFF basic overlay");
    /* Delete only (empty replacement). */
    ok_c("STUFF('ABCDE',2,3,'')",    "AE",    2, "E2: STUFF delete only");
    /* Insert only (delete 0 chars). */
    ok_c("STUFF('ABCDE',2,0,'XY')",  "AXYBCDE", 7, "E3: STUFF insert only");
    /* Replace entire string from position 1. */
    ok_c("STUFF('ABCDE',1,5,'ZZ')",  "ZZ",    2, "E4: STUFF replace all");
    /* Start beyond end -> append. */
    ok_c("STUFF('ABC',10,2,'ZZ')",   "ABCZZ", 5, "E5: STUFF start past end -> append");
    /* Delete more than available -> delete to end. */
    ok_c("STUFF('ABCDE',3,99,'Z')",  "ABZ",   3, "E6: STUFF del overflows -> clip");
    /* Delete 0 at start of string -> pure prepend. */
    ok_c("STUFF('ABCDE',1,0,'Z')",   "ZABCDE", 6, "E7: STUFF prepend");
    err_is("STUFF('A',1,1)",    XBEE_INVALID_ARG, "E8: STUFF wrong arity -> #11");
    err_is("STUFF(1,1,1,'X')",  XBEE_MISMATCH,    "E9: STUFF bad types -> #9");

    /* ================================================================ */
    /* F. ISALPHA                                                        */
    /*    Ref: HELP.DBS:1330 ".T. if the first character of <expC>      */
    /*    is a letter."                                                  */
    /*    [verified: HELP.DBS @STRING FUNCTIONS line 1330]              */
    /* ================================================================ */
    ok_l("ISALPHA('A')",   1, "F1: ISALPHA uppercase letter -> .T.");
    ok_l("ISALPHA('z')",   1, "F2: ISALPHA lowercase letter -> .T.");
    ok_l("ISALPHA('1')",   0, "F3: ISALPHA digit -> .F.");
    ok_l("ISALPHA('!')",   0, "F4: ISALPHA punctuation -> .F.");
    ok_l("ISALPHA('')",    0, "F5: ISALPHA empty string -> .F.");
    /* First char is what matters, not subsequent chars. */
    ok_l("ISALPHA('A123')", 1, "F6: ISALPHA checks first char only (A)");
    ok_l("ISALPHA('1ABC')", 0, "F7: ISALPHA checks first char only (1)");
    err_is("ISALPHA(1)",    XBEE_MISMATCH,    "F8: ISALPHA bad type -> #9");
    err_is("ISALPHA()",     XBEE_INVALID_ARG, "F9: ISALPHA wrong arity -> #11");

    /* ================================================================ */
    /* G. ISUPPER                                                        */
    /*    Ref: HELP.DBS:1336 ".T. if the first character of <expC>      */
    /*    is an uppercase letter."                                       */
    /*    [verified: HELP.DBS @STR FUNC 2 line 1336]                   */
    /* ================================================================ */
    ok_l("ISUPPER('A')",   1, "G1: ISUPPER uppercase A -> .T.");
    ok_l("ISUPPER('Z')",   1, "G2: ISUPPER uppercase Z -> .T.");
    ok_l("ISUPPER('a')",   0, "G3: ISUPPER lowercase -> .F.");
    ok_l("ISUPPER('1')",   0, "G4: ISUPPER digit -> .F.");
    ok_l("ISUPPER('')",    0, "G5: ISUPPER empty string -> .F.");
    ok_l("ISUPPER('Az')",  1, "G6: ISUPPER first char uppercase (rest ignored)");
    ok_l("ISUPPER('aZ')",  0, "G7: ISUPPER first char lowercase (rest ignored)");
    err_is("ISUPPER(1)",   XBEE_MISMATCH,    "G8: ISUPPER bad type -> #9");
    err_is("ISUPPER()",    XBEE_INVALID_ARG, "G9: ISUPPER wrong arity -> #11");

    /* ================================================================ */
    /* H. ISLOWER                                                        */
    /*    Ref: HELP.DBS:1331 ".T. if the first character of <expC>      */
    /*    is a lowercase letter."                                        */
    /*    [verified: HELP.DBS @STRING FUNCTIONS line 1331]              */
    /* ================================================================ */
    ok_l("ISLOWER('a')",   1, "H1: ISLOWER lowercase a -> .T.");
    ok_l("ISLOWER('z')",   1, "H2: ISLOWER lowercase z -> .T.");
    ok_l("ISLOWER('A')",   0, "H3: ISLOWER uppercase -> .F.");
    ok_l("ISLOWER('1')",   0, "H4: ISLOWER digit -> .F.");
    ok_l("ISLOWER('')",    0, "H5: ISLOWER empty string -> .F.");
    ok_l("ISLOWER('aZ')",  1, "H6: ISLOWER first char lowercase (rest ignored)");
    ok_l("ISLOWER('Az')",  0, "H7: ISLOWER first char uppercase (rest ignored)");
    err_is("ISLOWER(1)",   XBEE_MISMATCH,    "H8: ISLOWER bad type -> #9");
    err_is("ISLOWER()",    XBEE_INVALID_ARG, "H9: ISLOWER wrong arity -> #11");

    /* ================================================================ */
    /* I. TRANSFORM -- GATED (LOUD SKIP)                                 */
    /*    TRANSFORM confirmed to exist in III+ 1.1 (NOT in HELP_topics   */
    /*    index but functional: mint-results-002.md C2.TXT:              */
    /*    TRANSFORM(-570,"9999") -> "-570").                             */
    /*    The full PICTURE/FUNCTION template language (@ clauses, comma  */
    /*    groups, literal mask chars) requires a picture-walk formatter  */
    /*    not yet corpus-verified at the detail level needed for         */
    /*    mutation-proof assertions.                                     */
    /*    GATED: no assertions issued. Follow-up bead: initech-7az.TRANSFORM. */
    /* ================================================================ */
    printf("SKIP [initech-7az.TRANSFORM] TRANSFORM PICTURE/@ formatting "
           "not fully grounded -- follow-up bead required. "
           "TRANSFORM is registered (fn_transform) but its full template "
           "parser is GATED pending MINT/DISASM verification of @ clauses, "
           "comma groups, and character-picture semantics.\n");

    return TEST_SUMMARY("test-xbase-fn-c");
}
