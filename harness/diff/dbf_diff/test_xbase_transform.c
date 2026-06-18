/*
 * harness/diff/dbf_diff/test_xbase_transform.c -- unit oracle for TRANSFORM()
 * full PICTURE/FUNCTION template engine (initech-7az.14).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here.
 * Uses seed/test_assert.h (CHECK / TEST_HARNESS / TEST_SUMMARY).
 * Non-zero exit on any failed check (Law 2: oracle is the truth).
 *
 * Each case drives the FULL pipeline: lex -> parse -> eval (via xb_call_builtin
 * through the xb_eval XBN_CALL path), asserting RESULT TYPE + VALUE.
 *
 * -----------------------------------------------------------------------
 * GROUND TRUTH (Law 1):
 *
 * All asserted cases are derived from mint-results-002.md C2.TXT (verified
 * against real dBASE III+ 1.1). GATED cases are loud-skipped in comments.
 *
 * mint-results-002.md C2.TXT verified TRANSFORM results:
 *   TRANSFORM(-570,"9999")         -> "-570"       (default numeric picture)
 *   TRANSFORM(-570,"@( 9999")      -> "( 570)"     (@( parens for negatives)
 *   TRANSFORM(-570,"@X 9999")      -> " 570 DB"    (@X trailing " DB")
 *   TRANSFORM(570, "@C 9999")      -> " 570 CR"    (@C trailing " CR")
 *   TRANSFORM(-570,"@B 9999")      -> "-570"        (@B left-justify; -570 fills)
 *   TRANSFORM(-1234.56,"99,999.99")-> "-1,234.56"  (comma + decimal)
 *   TRANSFORM(-12.5,"$$,$$9.99")   -> "$$$-12.50"  (floating $)
 *   TRANSFORM(-570,"999-")          -> "***-"        (overflow; '-' is a literal)
 *
 * Character template (documented, string-functions.md lines 585-586):
 *   '!' -> uppercase the source char in-place.
 *   'X','A','N' -> passthrough (any char).
 *
 * -----------------------------------------------------------------------
 * GATED / LOUD-SKIPPED (not minted against real III+; [oracle-resolves]):
 *   @Z  (zero-blank)          -- transfrm.txt only; not minted vs III+
 *   @!  (force uppercase)     -- not minted against III+
 *   @R  (literal-insert mask) -- transfrm.txt marks as Harbour extension
 *   @S, @M, @E, @0, @L        -- display-state / Harbour; not III+ verified
 *   *   (float asterisk fill) -- in transfrm.txt; not minted against III+
 *   L, Y, 9, # for C-source picture -- [oracle-resolves]
 *   Combined @-flags ("@(B 9") -- mint only tested single flags
 *   @C with negative value     -- @C semantics for negatives [oracle-resolves]
 *   @X with positive value     -- @X semantics for positives [oracle-resolves]
 *   @B with value shorter than template -- padding exact [oracle-resolves]
 *
 * -----------------------------------------------------------------------
 * MUTATION PROOF (Rule 6):
 *   Compile with -DXB_MUTATE_TRANSFORM_PAREN to activate the perturbation
 *   in fn_builtins.c and fn_transform: @( does NOT parenthesize negatives.
 *   TRANSFORM(-570,"@( 9999")="( 570)" then goes RED, proving the oracle
 *   catches the @(-for-negatives rule (mint-002 C2.TXT "@( 9999" + -570).
 *   Mutant macro: XB_MUTATE_TRANSFORM_PAREN.
 *
 * -----------------------------------------------------------------------
 * Ref (Law 1):
 *   - ../dbase3-decomp/re/mint-results-002.md C2.TXT (TRANSFORM cases)
 *   - ../dbase3-decomp/specs/functions/string-functions.md lines 562-600
 *       (TRANSFORM spec: @-flags, template chars)
 *   - spec/samir/dbase_msg_codes.tsv (error codes #9/#11)
 *   - os/samir/include/samir/eval.h  (xb_ctx / xb_eval API)
 *   - os/samir/include/samir/value.h (xb_val / XB_C / xb_c)
 *   - seed/test_assert.h             (CHECK / TEST_HARNESS / TEST_SUMMARY)
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
 * TRANSFORM does not use today; fixture kept consistent with fn_a/fn_b/fn_c. */
#define TODAY_Y 1985
#define TODAY_M 8
#define TODAY_D 5

/* Scratch: 2 KB -- large enough for @( paren-wrap + @X/@C 3-byte suffixes.   */
static char g_scratch[2048];

/*
 * eval_expr -- lex + parse + eval with EXACT OFF, no work area, injected today.
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
    ctx.dbcur        = NULL;   /* no work area; TRANSFORM does not need one */
    ctx.dbcur_user   = NULL;

    return xb_eval(pool, root, &ctx, out, err);
}

/* Assert a Character result of length wlen equals want[0..wlen). */
static int ok_c(const char *s, const char *want, int wlen, const char *msg)
{
    xb_val v; int err = 999;
    int rc = eval_expr(s, &v, &err);
    int good = (rc == 0 && err == XBEE_OK && v.t == XB_C &&
                v.u.c.len == (uint16_t)wlen &&
                (wlen == 0 || memcmp(v.u.c.p, want, (size_t)wlen) == 0));
    if (!good) {
        fprintf(stderr, "  FAIL %s (ok_c):\n", msg);
        if (rc != 0 || err != XBEE_OK) {
            fprintf(stderr, "        rc=%d err=%d (expected rc=0 err=0)\n",
                    rc, err);
        } else if (v.t != XB_C) {
            fprintf(stderr, "        type=%d (expected XB_C=%d)\n", v.t, XB_C);
        } else {
            fprintf(stderr, "        got  [len=%d]: %.*s\n",
                    (int)v.u.c.len, (int)v.u.c.len, v.u.c.p);
            fprintf(stderr, "        want [len=%d]: %.*s\n", wlen, wlen, want);
        }
    }
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
    /* A. Default numeric picture (no @ clause)                          */
    /*                                                                   */
    /* Ref: mint-results-002.md C2.TXT [verified: real III+ 1.1]:       */
    /*   TRANSFORM(-570,"9999") -> "-570"                                */
    /* Template chars '9' and '#' are digit slots (string-functions.md  */
    /* line 581: "9/#(digit)"). The value is right-justified; the sign   */
    /* goes in the first empty slot to the left of the leftmost digit.   */
    /* ================================================================ */

    /* A1: The core minted case (initech-7az.12 skeleton; now full engine). */
    ok_c("TRANSFORM(-570,'9999')", "-570", 4,
         "A1: TRANSFORM(-570,'9999')='-570' [MUTANT TARGET: @( paren]"
         " [verified: mint-002 C2.TXT]");

    /* A2: Positive value -- no sign placed. */
    ok_c("TRANSFORM(570,'9999')", " 570", 4,
         "A2: TRANSFORM(570,'9999')=' 570' (positive, right-justified)"
         " [inferred from III+ right-justify rule + mint-002 context]");

    /* A3: Zero -- right-justified, no sign. */
    ok_c("TRANSFORM(0,'9999')", "   0", 4,
         "A3: TRANSFORM(0,'9999')='   0' (zero, right-justified)"
         " [inferred from III+ right-justify rule]");

    /* A4: '#' behaves identically to '9' (both are digit slots).
     * [string-functions.md line 581: "9/#(digit)"] */
    ok_c("TRANSFORM(-570,'####')", "-570", 4,
         "A4: TRANSFORM(-570,'####')='-570' (# == 9 as digit slot)"
         " [string-functions.md:581]");

    /* A5: Overflow -- all digit slots fill with '*', literals pass through.
     * [verified: mint-002 C2.TXT "999-" + -570 -> "***-"]               */
    ok_c("TRANSFORM(-570,'999-')", "***-", 4,
         "A5: TRANSFORM(-570,'999-')='***-' (overflow; '-' is literal)"
         " [verified: mint-002 C2.TXT]");

    /* A6: Comma group separator + decimal point.
     * [verified: mint-002 C2.TXT "99,999.99" + -1234.56 -> "-1,234.56"] */
    ok_c("TRANSFORM(-1234.56,'99,999.99')", "-1,234.56", 9,
         "A6: TRANSFORM(-1234.56,'99,999.99')='-1,234.56'"
         " [verified: mint-002 C2.TXT]");

    /* A7: Floating dollar fill.
     * [verified: mint-002 C2.TXT "$$,$$9.99" + -12.5 -> "$$$-12.50"]    */
    ok_c("TRANSFORM(-12.5,'$$,$$9.99')", "$$$-12.50", 9,
         "A7: TRANSFORM(-12.5,'$$,$$9.99')='$$$-12.50'"
         " [verified: mint-002 C2.TXT]");

    /* ================================================================ */
    /* B. @( flag: parens for negatives                                  */
    /*                                                                   */
    /* Ref: mint-results-002.md C2.TXT [verified: real III+ 1.1]:       */
    /*   TRANSFORM(-570,"@( 9999") -> "( 570)"                          */
    /* Output length = tlen + 2 ('(' prefix, ')' suffix).               */
    /* No minus sign in the digit slots.                                 */
    /* ================================================================ */

    /* B1: @( negative -- THE MUTATION TARGET. */
    ok_c("TRANSFORM(-570,'@( 9999')", "( 570)", 6,
         "B1: TRANSFORM(-570,'@( 9999')='( 570)' [MUTANT TARGET]"
         " [verified: mint-002 C2.TXT]");

    /* B2: @( positive -- no parens; normal right-justified output. */
    ok_c("TRANSFORM(570,'@( 9999')", " 570", 4,
         "B2: TRANSFORM(570,'@( 9999')=' 570' (positive: no parens)"
         " [inferred: @( is only for negatives per III+ semantics]");

    /* ================================================================ */
    /* C. @X flag: trailing " DB" for negatives                          */
    /*                                                                   */
    /* Ref: mint-results-002.md C2.TXT [verified: real III+ 1.1]:       */
    /*   TRANSFORM(-570,"@X 9999") -> " 570 DB"                         */
    /* @X suppresses the leading minus (debit conveyed by " DB").        */
    /* Output length = tlen + 3 (" DB" appended).                       */
    /* ================================================================ */

    /* C1: @X negative -- no leading minus, " DB" suffix. */
    ok_c("TRANSFORM(-570,'@X 9999')", " 570 DB", 7,
         "C1: TRANSFORM(-570,'@X 9999')=' 570 DB'"
         " [verified: mint-002 C2.TXT]");

    /* ================================================================ */
    /* D. @C flag: trailing " CR" for positives                          */
    /*                                                                   */
    /* Ref: mint-results-002.md C2.TXT [verified: real III+ 1.1]:       */
    /*   TRANSFORM(570,"@C 9999") -> " 570 CR"                          */
    /* @C appends " CR" for positive values. Output = tlen + 3.         */
    /* ================================================================ */

    /* D1: @C positive -- " CR" suffix. */
    ok_c("TRANSFORM(570,'@C 9999')", " 570 CR", 7,
         "D1: TRANSFORM(570,'@C 9999')=' 570 CR'"
         " [verified: mint-002 C2.TXT]");

    /* ================================================================ */
    /* E. @B flag: left-justify within the field                         */
    /*                                                                   */
    /* Ref: mint-results-002.md C2.TXT [verified: real III+ 1.1]:       */
    /*   TRANSFORM(-570,"@B 9999") -> "-570"                            */
    /* @B left-justifies: shift non-space content to the left, pad       */
    /* right with spaces. -570 fills all 4 slots so no padding needed.  */
    /* Output length = tlen (no extra bytes).                            */
    /* ================================================================ */

    /* E1: @B with 4-slot field exactly filled. */
    ok_c("TRANSFORM(-570,'@B 9999')", "-570", 4,
         "E1: TRANSFORM(-570,'@B 9999')='-570' (@B left-justify, full field)"
         " [verified: mint-002 C2.TXT]");

    /* ================================================================ */
    /* F. Character-source picture                                        */
    /*                                                                   */
    /* Ref: string-functions.md lines 585-586 (documented, not minted): */
    /*   '!' -> uppercase the source char at this position               */
    /*   'X','A','N' -> passthrough (any char)                           */
    /* ================================================================ */

    /* F1: '!' uppercases each source char in position.
     * [documented: string-functions.md:586 "Template '!' (uppercase)"] */
    ok_c("TRANSFORM('hello','!!!!!')", "HELLO", 5,
         "F1: TRANSFORM('hello','!!!!!')='HELLO' ('!' uppercases)"
         " [documented: string-functions.md:586]");

    /* F2: 'X' is a passthrough slot (any char).
     * [documented: string-functions.md:586 "A/N/X (any char)"] */
    ok_c("TRANSFORM('abc','XXX')", "abc", 3,
         "F2: TRANSFORM('abc','XXX')='abc' ('X' passthrough)"
         " [documented: string-functions.md:586]");

    /* F3: 'A' is a passthrough slot (any char). */
    ok_c("TRANSFORM('abc','AAA')", "abc", 3,
         "F3: TRANSFORM('abc','AAA')='abc' ('A' passthrough)"
         " [documented: string-functions.md:586]");

    /* F4: 'N' is a passthrough slot (any char). */
    ok_c("TRANSFORM('abc','NNN')", "abc", 3,
         "F4: TRANSFORM('abc','NNN')='abc' ('N' passthrough)"
         " [documented: string-functions.md:586]");

    /* F5: Mixed '!' and 'X' -- uppercase some positions, pass through others. */
    ok_c("TRANSFORM('hello','!XXXX')", "Hello", 5,
         "F5: TRANSFORM('hello','!XXXX')='Hello' (mixed ! and X)"
         " [documented: string-functions.md:586]");

    /* F6: Literal char in char template: template char placed in output
     * when it is not a recognized slot char. Source chars are consumed
     * positionally; the literal at position 1 emits '-' but source[1]
     * is skipped (positional alignment), so source[2]='c' appears at
     * template position 2 ('X' slot).
     * [documented: string-functions.md:585 "other chars -> literal"] */
    ok_c("TRANSFORM('abc','X-X')", "a-c", 3,
         "F6: TRANSFORM('abc','X-X')='a-c' (literal '-' in template, positional)"
         " [documented: string-functions.md:585]");

    /* F7: Source shorter than template: space-fill remaining positions.
     * [conservative: exact III+ short-source behavior GATED but space-fill
     *  is the natural behavior matching HELP.DBS @SAY PICTURE semantics] */
    ok_c("TRANSFORM('hi','XXXXX')", "hi   ", 5,
         "F7: TRANSFORM('hi','XXXXX')='hi   ' (short source -> space pad)"
         " [conservative; exact III+ behavior GATED/[oracle-resolves]]");

    /* ================================================================ */
    /* G. Arity and type errors                                          */
    /*                                                                   */
    /* Ref: spec/samir/dbase_msg_codes.tsv                               */
    /*   #11 XBEE_INVALID_ARG = wrong arity                              */
    /*   #9  XBEE_MISMATCH    = wrong argument types                     */
    /* ================================================================ */

    err_is("TRANSFORM(-570)", XBEE_INVALID_ARG,
           "G1: TRANSFORM one arg -> #11 (XBEE_INVALID_ARG)");
    err_is("TRANSFORM(-570,'9999',0)", XBEE_INVALID_ARG,
           "G2: TRANSFORM three args -> #11 (XBEE_INVALID_ARG)");
    err_is("TRANSFORM(-570,9)", XBEE_MISMATCH,
           "G3: TRANSFORM numeric picture -> #9 (XBEE_MISMATCH)");
    err_is("TRANSFORM(.T.,'9999')", XBEE_MISMATCH,
           "G4: TRANSFORM logical source -> #9 (XBEE_MISMATCH)");

    /* ================================================================ */
    /* H. GATED -- loud-skip (not asserted; present as documentation)   */
    /*                                                                   */
    /* The following clauses are NOT asserted because they are not        */
    /* minted against real dBASE III+ 1.1. Uncomment ONLY after minting.*/
    /* ================================================================ */
    /*
     * GATED: @Z (zero-blank) -- HELP.DBS mentions it; not in mint-002 C2.TXT.
     *   TRANSFORM(0,"@Z 9999") -> ?
     *
     * GATED: @! (force uppercase on whole result) -- not minted.
     *   TRANSFORM("hello","@! XXXXX") -> ?
     *
     * GATED: @R (literal-insert mask) -- transfrm.txt says Harbour extension.
     *   TRANSFORM("12345","@R (999) 999-9999") -> ?
     *
     * GATED: @C with negative -- @C for is_neg semantics [oracle-resolves].
     *   TRANSFORM(-570,"@C 9999") -> ?
     *
     * GATED: @X with positive -- @X for !is_neg semantics [oracle-resolves].
     *   TRANSFORM(570,"@X 9999") -> ?
     *
     * GATED: @B with wider field -- padding exact behavior.
     *   TRANSFORM(-570,"@B 99999") -> ?   (trailing spaces count?)
     *
     * GATED: Combined @-flags -- mint only tested single flags.
     *   TRANSFORM(-570,"@(B 9999") -> ?
     *
     * GATED: L, Y, 9, # in char-source picture -- [oracle-resolves].
     *   TRANSFORM("Y","L") -> ?
     *
     * GATED: * (float asterisk fill) -- in transfrm.txt; not minted vs III+.
     *   TRANSFORM(-570,"***-") -> ?
     */

    return TEST_SUMMARY("test_xbase_transform");
}
