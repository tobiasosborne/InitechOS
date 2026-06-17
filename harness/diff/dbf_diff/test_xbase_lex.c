/*
 * harness/diff/dbf_diff/test_xbase_lex.c -- unit oracle for lex.c (S3.1).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Uses the seed
 * test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY). A non-zero
 * exit on any failed check ensures the make gate can never false-green
 * (CLAUDE.md Law 2: the oracle is the truth).
 *
 * MUTATION PROOF (Rule 6):
 *   Compile with -DXB_MUTATE_LEX to activate the single perturbation in
 *   lex.c: == is accepted as EQ instead of raising XBLE_EQ_EQ. The test
 *   "== -> lex error XBLE_EQ_EQ" (section 11) then goes RED, proving the
 *   assertion catches the regression. No other test should change.
 *
 * Tests (by section):
 *
 *  1. Single-quoted string literal 'text' -> XBT_LIT_C, content bytes.
 *  2. Double-quoted string literal "text" -> XBT_LIT_C.
 *  3. Bracket string literal [text] -> XBT_LIT_C.
 *     GATED: bracket nesting/escape [oracle-resolves]; conservative behavior
 *     tested (literal match, no nesting support claimed).
 *  4. Empty string literals '' "" [].
 *  5. Numeric literals: integer, decimal, zero.
 *     GATED: .5 leading-dot form [oracle-resolves]; conservative (accept).
 *  6. Logical literals .T. .F. -> 1/0; case-insensitive .t. .f.
 *     GATED: .Y. .N. [oracle-resolves]; conservative (accept as true/false).
 *  7. Dotted operators .AND. .OR. .NOT. -- canonical upper case.
 *  8. Dotted operators -- lower case (.and. .or. .not.) -- case-insensitive.
 *  9. All single-char operators: + - * / ^ < > = # $ ( )
 * 10. Two-char operators: ** <= >= <>
 * 11. == -> lex error XBLE_EQ_EQ (THE headline behavior of S3.1;
 *     asserts both negative return and correct error code).
 * 12. != -> lex error XBLE_BANG_EQ.
 * 13. % -> lex error XBLE_PERCENT.
 * 14. Unterminated string literal -> XBLE_UNTERM_STR.
 * 15. Unknown character -> XBLE_UNKNOWN_CHAR.
 * 16. Identifiers: plain, underscore, mixed.
 * 17. Whitespace between tokens is skipped (spaces and tabs).
 * 18. Realistic expression: UPPER(LASTNAME) $ "SMITH" .AND. AGE > 30
 *     -- decomposed into expected token stream.
 * 19. EOI token is always the last token in a successful lex.
 * 20. Buffer overflow: cap=1 for a 2-token input -> XBLE_BUF_OVERFLOW.
 * 21. Leading sign is a separate operator (- 5 -> MINUS, LIT_N).
 *
 * GATED notes appear as GATED: ... inline comments on individual tests.
 *
 * Compile + run (self-grade, host):
 *   gcc -std=c11 -Wall -Wextra -Werror -Iseed -Ios/samir/include \
 *       os/samir/core/lex.c os/samir/core/rt.c \
 *       harness/diff/dbf_diff/test_xbase_lex.c \
 *       -o /tmp/test_xbase_lex && /tmp/test_xbase_lex
 *
 * Mutant (must go RED on section 11):
 *   gcc -std=c11 -Wall -Wextra -Werror -DXB_MUTATE_LEX -Iseed \
 *       -Ios/samir/include \
 *       os/samir/core/lex.c os/samir/core/rt.c \
 *       harness/diff/dbf_diff/test_xbase_lex.c \
 *       -o /tmp/test_xbase_lex_mut ; \
 *       /tmp/test_xbase_lex_mut ; echo "mutant exit=$?"
 *
 * Freestanding compile check (lex.c only):
 *   gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -fno-pic \
 *       -std=c11 -Wall -Wextra -Werror -Ios/samir/include \
 *       -c os/samir/core/lex.c -o /tmp/lex_free.o
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S3.1 oracle contract
 *   - os/samir/include/samir/eval.h (xb_lex API)
 *   - ../dbase3-decomp/specs/language/expressions-and-operators.md
 *   - ../dbase3-decomp/specs/language/data-types.md
 *   - spec/samir/xbase_coercion.json not_in_iii_plus
 *   - seed/test_assert.h (harness idiom)
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "test_assert.h"        /* seed/, on -Iseed */
#include "samir/eval.h"         /* os/samir/include/, on -Ios/samir/include */

TEST_HARNESS();

/* Token buffer size for most tests. */
#define TBUF 64

/* Helper: lex a string, assert no error, return token count (incl. EOI). */
static int lex_ok(const char *s, xb_token *buf, uint32_t cap)
{
    int err = 0;
    int n = xb_lex(s, (uint32_t)strlen(s), buf, cap, &err);
    (void)err;
    return n;
}

/* Helper: lex a string, expect a negative return and a specific error code. */
static int lex_err(const char *s, int *err_out)
{
    xb_token buf[TBUF];
    int err = 0;
    int n = xb_lex(s, (uint32_t)strlen(s), buf, TBUF, &err);
    *err_out = err;
    return n;
}

int main(void)
{
    xb_token buf[TBUF];

    /* ------------------------------------------------------------------ */
    /* 1. Single-quoted string literal                                      */
    /* ------------------------------------------------------------------ */
    {
        int n = lex_ok("'HELLO'", buf, TBUF);
        /* tokens: LIT_C, EOI */
        CHECK(n == 2,                           "1: single-quote -> 2 tokens");
        CHECK(buf[0].type == XBT_LIT_C,         "1: token[0] = XBT_LIT_C");
        CHECK(buf[0].u.c.len == 5,              "1: content len == 5");
        CHECK(memcmp(buf[0].u.c.p, "HELLO", (size_t)5) == 0, "1: content == HELLO");
        CHECK(buf[0].offset == 0,               "1: offset == 0");
        CHECK(buf[0].len == 7,                  "1: span len == 7 (incl delims)");
        CHECK(buf[1].type == XBT_EOI,           "1: token[1] = XBT_EOI");
    }

    /* ------------------------------------------------------------------ */
    /* 2. Double-quoted string literal                                      */
    /* ------------------------------------------------------------------ */
    {
        int n = lex_ok("\"WORLD\"", buf, TBUF);
        CHECK(n == 2,                           "2: double-quote -> 2 tokens");
        CHECK(buf[0].type == XBT_LIT_C,         "2: token[0] = XBT_LIT_C");
        CHECK(buf[0].u.c.len == 5,              "2: content len == 5");
        CHECK(memcmp(buf[0].u.c.p, "WORLD", (size_t)5) == 0, "2: content == WORLD");
    }

    /* ------------------------------------------------------------------ */
    /* 3. Bracket string literal [text]                                     */
    /* GATED: bracket nesting/escape [oracle-resolves; data-types.md sec.2] */
    /* Conservative: accept bracket strings literally, no nesting claimed.  */
    /* ------------------------------------------------------------------ */
    {
        /* GATED: [text] form; conservative choice: accept as C literal */
        int n = lex_ok("[INITECH]", buf, TBUF);
        CHECK(n == 2,                           "3(GATED): bracket -> 2 tokens");
        CHECK(buf[0].type == XBT_LIT_C,         "3(GATED): token[0] = XBT_LIT_C");
        CHECK(buf[0].u.c.len == 7,              "3(GATED): content len == 7");
        CHECK(memcmp(buf[0].u.c.p, "INITECH", (size_t)7) == 0, "3(GATED): content == INITECH");
    }

    /* ------------------------------------------------------------------ */
    /* 4. Empty string literals                                             */
    /* ------------------------------------------------------------------ */
    {
        int n;

        n = lex_ok("''", buf, TBUF);
        CHECK(n == 2,                     "4a: empty single-quote -> 2 tokens");
        CHECK(buf[0].type == XBT_LIT_C,   "4a: XBT_LIT_C");
        CHECK(buf[0].u.c.len == 0,        "4a: content len == 0");

        n = lex_ok("\"\"", buf, TBUF);
        CHECK(n == 2,                     "4b: empty double-quote -> 2 tokens");
        CHECK(buf[0].u.c.len == 0,        "4b: content len == 0");

        n = lex_ok("[]", buf, TBUF);
        CHECK(n == 2,                     "4c: empty bracket -> 2 tokens");
        CHECK(buf[0].u.c.len == 0,        "4c: content len == 0");
    }

    /* ------------------------------------------------------------------ */
    /* 5. Numeric literals                                                  */
    /* ------------------------------------------------------------------ */
    {
        int n;
        double eps = 1e-12;

        /* integer */
        n = lex_ok("42", buf, TBUF);
        CHECK(n == 2,                     "5a: integer -> 2 tokens");
        CHECK(buf[0].type == XBT_LIT_N,   "5a: XBT_LIT_N");
        CHECK(buf[0].u.n == 42.0,         "5a: value == 42.0");

        /* decimal */
        n = lex_ok("3.14", buf, TBUF);
        CHECK(n == 2,                     "5b: decimal -> 2 tokens");
        CHECK(buf[0].type == XBT_LIT_N,   "5b: XBT_LIT_N");
        {
            double diff = buf[0].u.n - 3.14;
            if (diff < 0) diff = -diff;
            CHECK(diff < eps,             "5b: value ~= 3.14");
        }

        /* zero */
        n = lex_ok("0", buf, TBUF);
        CHECK(buf[0].u.n == 0.0,          "5c: zero");

        /* 0.00 */
        n = lex_ok("0.00", buf, TBUF);
        CHECK(buf[0].u.n == 0.0,          "5d: 0.00");

        /* GATED: .5 leading-dot numeric form [oracle-resolves; data-types.md
         * sec.3 does not document .5 form]. Conservative: accept. */
        n = lex_ok(".5", buf, TBUF);
        CHECK(n == 2,                     "5e(GATED): .5 -> 2 tokens");
        CHECK(buf[0].type == XBT_LIT_N,   "5e(GATED): XBT_LIT_N");
        {
            double diff = buf[0].u.n - 0.5;
            if (diff < 0) diff = -diff;
            CHECK(diff < eps,             "5e(GATED): value ~= 0.5");
        }
    }

    /* ------------------------------------------------------------------ */
    /* 6. Logical literals .T. .F. (verified) + .Y. .N. (GATED)            */
    /* ------------------------------------------------------------------ */
    {
        int n;

        /* .T. -> true */
        n = lex_ok(".T.", buf, TBUF);
        CHECK(n == 2,                     "6a: .T. -> 2 tokens");
        CHECK(buf[0].type == XBT_LIT_L,   "6a: XBT_LIT_L");
        CHECK(buf[0].u.l == 1,            "6a: .T. -> true (1)");
        CHECK(buf[0].len == 3,            "6a: span 3");

        /* .F. -> false */
        n = lex_ok(".F.", buf, TBUF);
        CHECK(buf[0].u.l == 0,            "6b: .F. -> false (0)");

        /* case-insensitive: .t. .f. */
        n = lex_ok(".t.", buf, TBUF);
        CHECK(buf[0].type == XBT_LIT_L && buf[0].u.l == 1,
              "6c: .t. -> true (case-insensitive)");

        n = lex_ok(".f.", buf, TBUF);
        CHECK(buf[0].type == XBT_LIT_L && buf[0].u.l == 0,
              "6d: .f. -> false (case-insensitive)");

        /* GATED: .Y. -> true [oracle-resolves; data-types.md Open Q5]
         * Conservative: accept as true */
        n = lex_ok(".Y.", buf, TBUF);
        CHECK(n == 2,                     "6e(GATED): .Y. -> 2 tokens");
        CHECK(buf[0].type == XBT_LIT_L,   "6e(GATED): XBT_LIT_L");
        CHECK(buf[0].u.l == 1,            "6e(GATED): .Y. -> true");

        /* GATED: .N. -> false [oracle-resolves] */
        n = lex_ok(".N.", buf, TBUF);
        CHECK(n == 2,                     "6f(GATED): .N. -> 2 tokens");
        CHECK(buf[0].type == XBT_LIT_L,   "6f(GATED): XBT_LIT_L");
        CHECK(buf[0].u.l == 0,            "6f(GATED): .N. -> false");
    }

    /* ------------------------------------------------------------------ */
    /* 7. Dotted operators .AND. .OR. .NOT. -- upper case                   */
    /* ------------------------------------------------------------------ */
    {
        int n;

        n = lex_ok(".AND.", buf, TBUF);
        CHECK(n == 2,                     "7a: .AND. -> 2 tokens");
        CHECK(buf[0].type == XBT_AND,     "7a: XBT_AND");
        CHECK(buf[0].len == 6,            "7a: span 6");

        n = lex_ok(".OR.", buf, TBUF);
        CHECK(buf[0].type == XBT_OR,      "7b: XBT_OR");
        CHECK(buf[0].len == 5,            "7b: span 5");

        n = lex_ok(".NOT.", buf, TBUF);
        CHECK(buf[0].type == XBT_NOT,     "7c: XBT_NOT");
        CHECK(buf[0].len == 6,            "7c: span 6");
    }

    /* ------------------------------------------------------------------ */
    /* 8. Dotted operators -- lower case (case-insensitive)                 */
    /* ------------------------------------------------------------------ */
    {
        lex_ok(".and.", buf, TBUF);
        CHECK(buf[0].type == XBT_AND,     "8a: .and. -> XBT_AND");

        lex_ok(".or.", buf, TBUF);
        CHECK(buf[0].type == XBT_OR,      "8b: .or. -> XBT_OR");

        lex_ok(".not.", buf, TBUF);
        CHECK(buf[0].type == XBT_NOT,     "8c: .not. -> XBT_NOT");
    }

    /* ------------------------------------------------------------------ */
    /* 9. Single-char operators                                             */
    /* ------------------------------------------------------------------ */
    {
        struct { const char *s; xb_token_type t; } cases[] = {
            { "+", XBT_PLUS   },
            { "-", XBT_MINUS  },
            { "*", XBT_STAR   },
            { "/", XBT_SLASH  },
            { "^", XBT_CARET  },
            { "<", XBT_LT     },
            { ">", XBT_GT     },
            { "=", XBT_EQ     },
            { "#", XBT_HASH   },
            { "$", XBT_DOLLAR },
            { "(", XBT_LPAREN },
            { ")", XBT_RPAREN }
        };
        int k, nc = (int)(sizeof(cases) / sizeof(cases[0]));
        for (k = 0; k < nc; k++) {
            int n = lex_ok(cases[k].s, buf, TBUF);
            CHECK(n == 2,                 "9: single-op -> 2 tokens");
            CHECK(buf[0].type == cases[k].t, "9: correct token type");
            CHECK(buf[0].len == 1,        "9: span == 1");
        }
    }

    /* ------------------------------------------------------------------ */
    /* 10. Two-char operators: ** <= >= <>                                  */
    /* ------------------------------------------------------------------ */
    {
        int n;

        n = lex_ok("**", buf, TBUF);
        CHECK(n == 2,                     "10a: ** -> 2 tokens");
        CHECK(buf[0].type == XBT_STARSTAR,"10a: XBT_STARSTAR");
        CHECK(buf[0].len == 2,            "10a: span 2");

        n = lex_ok("<=", buf, TBUF);
        CHECK(buf[0].type == XBT_LE,      "10b: XBT_LE");
        CHECK(buf[0].len == 2,            "10b: span 2");

        n = lex_ok(">=", buf, TBUF);
        CHECK(buf[0].type == XBT_GE,      "10c: XBT_GE");

        n = lex_ok("<>", buf, TBUF);
        CHECK(buf[0].type == XBT_NEQ,     "10d: XBT_NEQ");
    }

    /* ------------------------------------------------------------------ */
    /* 11. == -> lex error XBLE_EQ_EQ                                       */
    /*                                                                      */
    /* THE HEADLINE BEHAVIOR OF S3.1.                                       */
    /* dBASE III PLUS 1.1 has no == operator (it is dBASE IV).             */
    /* Plan sec.2.C: "the lexer treats == as a III+ lex error".             */
    /* Refs: spec/samir/xbase_coercion.json not_in_iii_plus: ["==",...]    */
    /*       expressions-and-operators.md sec.4.2 verified: no == in III+   */
    /*       HELP.DBS @RELATIONAL OP lists < > = <> # <= >= $ -- no ==     */
    /*                                                                      */
    /* When compiled with -DXB_MUTATE_LEX, the == is ACCEPTED as EQ and    */
    /* this test goes RED, proving the mutation can be caught.              */
    /* ------------------------------------------------------------------ */
    {
        int err = 0;
        int n = lex_err("==", &err);
        CHECK(n < 0,                      "11: == -> negative return (error)");
        CHECK(err == XBLE_EQ_EQ,          "11: err code == XBLE_EQ_EQ");
    }

    /* Also test == embedded in a valid expression prefix */
    {
        int err = 0;
        int n = lex_err("A == B", &err);
        CHECK(n < 0,                      "11b: A==B -> error");
        CHECK(err == XBLE_EQ_EQ,          "11b: err code == XBLE_EQ_EQ");
    }

    /* ------------------------------------------------------------------ */
    /* 12. != -> lex error XBLE_BANG_EQ                                     */
    /* [spec/samir/xbase_coercion.json not_in_iii_plus: ["==","!=",...]]    */
    /* ------------------------------------------------------------------ */
    {
        int err = 0;
        int n = lex_err("!=", &err);
        CHECK(n < 0,                      "12: != -> negative return");
        CHECK(err == XBLE_BANG_EQ,        "12: err code == XBLE_BANG_EQ");
    }

    /* ------------------------------------------------------------------ */
    /* 13. % -> lex error XBLE_PERCENT                                      */
    /* [coercion.json not_in_iii_plus: ["==","!=","%",...]; use MOD()]      */
    /* ------------------------------------------------------------------ */
    {
        int err = 0;
        int n = lex_err("%", &err);
        CHECK(n < 0,                      "13: % -> negative return");
        CHECK(err == XBLE_PERCENT,        "13: err code == XBLE_PERCENT");
    }

    /* ------------------------------------------------------------------ */
    /* 14. Unterminated string literal -> XBLE_UNTERM_STR                   */
    /* [DBASE.MSG line 34 "Unterminated string."]                           */
    /* ------------------------------------------------------------------ */
    {
        int err = 0;
        int n;

        n = lex_err("'hello", &err);
        CHECK(n < 0,                      "14a: unterminated single -> error");
        CHECK(err == XBLE_UNTERM_STR,     "14a: XBLE_UNTERM_STR");

        n = lex_err("\"world", &err);
        CHECK(n < 0,                      "14b: unterminated double -> error");
        CHECK(err == XBLE_UNTERM_STR,     "14b: XBLE_UNTERM_STR");

        n = lex_err("[initech", &err);
        CHECK(n < 0,                      "14c: unterminated bracket -> error");
        CHECK(err == XBLE_UNTERM_STR,     "14c: XBLE_UNTERM_STR");
    }

    /* ------------------------------------------------------------------ */
    /* 15. Unknown character -> XBLE_UNKNOWN_CHAR                           */
    /* A bare dot (no recognized continuation) also lands here.            */
    /* ------------------------------------------------------------------ */
    {
        int err = 0;
        int n;

        n = lex_err("@", &err);
        CHECK(n < 0,                      "15a: @ -> error");
        CHECK(err == XBLE_UNKNOWN_CHAR,   "15a: XBLE_UNKNOWN_CHAR");

        n = lex_err(".", &err);
        CHECK(n < 0,                      "15b: bare dot -> error");
        CHECK(err == XBLE_UNKNOWN_CHAR,   "15b: XBLE_UNKNOWN_CHAR");
    }

    /* ------------------------------------------------------------------ */
    /* 16. Identifiers                                                       */
    /* ------------------------------------------------------------------ */
    {
        int n;

        /* plain identifier */
        n = lex_ok("SALARY", buf, TBUF);
        CHECK(n == 2,                         "16a: SALARY -> 2 tokens");
        CHECK(buf[0].type == XBT_IDENT,       "16a: XBT_IDENT");
        CHECK(buf[0].u.ident.len == 6,        "16a: len == 6");
        CHECK(memcmp(buf[0].u.ident.p, "SALARY", (size_t)6) == 0,
              "16a: ident == SALARY");

        /* lower case preserved */
        lex_ok("lastname", buf, TBUF);
        CHECK(buf[0].type == XBT_IDENT,       "16b: lower-case ident");
        CHECK(memcmp(buf[0].u.ident.p, "lastname", (size_t)8) == 0,
              "16b: case preserved");

        /* underscore start */
        lex_ok("_tmp", buf, TBUF);
        CHECK(buf[0].type == XBT_IDENT,       "16c: underscore start");

        /* mixed: letter+digit+underscore */
        lex_ok("F1_VAL", buf, TBUF);
        CHECK(buf[0].type == XBT_IDENT,       "16d: mixed ident");
        CHECK(buf[0].u.ident.len == 6,        "16d: len == 6");
    }

    /* ------------------------------------------------------------------ */
    /* 17. Whitespace between tokens is skipped                             */
    /* ------------------------------------------------------------------ */
    {
        int n;

        /* spaces */
        n = lex_ok("A + B", buf, TBUF);
        CHECK(n == 4,                     "17a: A + B -> 4 tokens");
        CHECK(buf[0].type == XBT_IDENT,   "17a: IDENT A");
        CHECK(buf[1].type == XBT_PLUS,    "17a: PLUS");
        CHECK(buf[2].type == XBT_IDENT,   "17a: IDENT B");
        CHECK(buf[3].type == XBT_EOI,     "17a: EOI");

        /* tab + spaces */
        n = lex_ok("X\t*\t Y", buf, TBUF);
        CHECK(n == 4,                     "17b: X * Y (tab-sep) -> 4 tokens");
        CHECK(buf[1].type == XBT_STAR,    "17b: STAR");
    }

    /* ------------------------------------------------------------------ */
    /* 18. Realistic expression:                                            */
    /*   UPPER(LASTNAME) $ "SMITH" .AND. AGE > 30                         */
    /* Expected tokens:                                                     */
    /*   IDENT(UPPER) LPAREN IDENT(LASTNAME) RPAREN DOLLAR LIT_C("SMITH") */
    /*   AND IDENT(AGE) GT LIT_N(30) EOI                                  */
    /* ------------------------------------------------------------------ */
    {
        const char *expr = "UPPER(LASTNAME) $ \"SMITH\" .AND. AGE > 30";
        int n = lex_ok(expr, buf, TBUF);
        CHECK(n == 11,                        "18: 11 tokens total");
        CHECK(buf[0].type == XBT_IDENT,       "18[0]: IDENT UPPER");
        CHECK(buf[0].u.ident.len == 5,        "18[0]: len 5");
        CHECK(memcmp(buf[0].u.ident.p, "UPPER", (size_t)5) == 0, "18[0]: == UPPER");
        CHECK(buf[1].type == XBT_LPAREN,      "18[1]: LPAREN");
        CHECK(buf[2].type == XBT_IDENT,       "18[2]: IDENT LASTNAME");
        CHECK(buf[2].u.ident.len == 8,        "18[2]: len 8");
        CHECK(buf[3].type == XBT_RPAREN,      "18[3]: RPAREN");
        CHECK(buf[4].type == XBT_DOLLAR,      "18[4]: DOLLAR");
        CHECK(buf[5].type == XBT_LIT_C,       "18[5]: LIT_C");
        CHECK(buf[5].u.c.len == 5,            "18[5]: content len 5");
        CHECK(memcmp(buf[5].u.c.p, "SMITH", (size_t)5) == 0, "18[5]: == SMITH");
        CHECK(buf[6].type == XBT_AND,         "18[6]: AND");
        CHECK(buf[7].type == XBT_IDENT,       "18[7]: IDENT AGE");
        CHECK(buf[8].type == XBT_GT,          "18[8]: GT");
        CHECK(buf[9].type == XBT_LIT_N,       "18[9]: LIT_N");
        CHECK(buf[9].u.n == 30.0,             "18[9]: value 30.0");
        CHECK(buf[10].type == XBT_EOI,        "18[10]: EOI");
    }

    /* ------------------------------------------------------------------ */
    /* 19. EOI is always the last token in a successful lex                 */
    /* ------------------------------------------------------------------ */
    {
        int n;

        /* empty input */
        n = lex_ok("", buf, TBUF);
        CHECK(n == 1,                     "19a: empty input -> 1 token");
        CHECK(buf[0].type == XBT_EOI,     "19a: that token is EOI");

        /* single operator */
        n = lex_ok("+", buf, TBUF);
        CHECK(n == 2,                     "19b: single op -> 2 tokens");
        CHECK(buf[n-1].type == XBT_EOI,   "19b: last token is EOI");

        /* expression */
        n = lex_ok("1 + 2", buf, TBUF);
        CHECK(buf[n-1].type == XBT_EOI,   "19c: last token is EOI");
    }

    /* ------------------------------------------------------------------ */
    /* 20. Buffer overflow: cap too small -> XBLE_BUF_OVERFLOW              */
    /* ------------------------------------------------------------------ */
    {
        xb_token small[1];
        int err = 0;
        /* "A + B" needs 4 tokens but cap=1 */
        int n = xb_lex("A + B", 5, small, 1, &err);
        CHECK(n < 0,                      "20: cap=1 for 4-token input -> error");
        CHECK(err == XBLE_BUF_OVERFLOW,   "20: XBLE_BUF_OVERFLOW");
    }

    /* ------------------------------------------------------------------ */
    /* 21. Leading sign is a separate operator (- 5 -> MINUS, LIT_N)       */
    /* The sign of a numeric literal is NOT part of the literal token.     */
    /* [documented: data-types.md sec.3; consistent with III+ grammar]     */
    /* ------------------------------------------------------------------ */
    {
        int n;

        n = lex_ok("-5", buf, TBUF);
        /* MINUS followed by LIT_N(5), then EOI */
        CHECK(n == 3,                     "21a: -5 -> 3 tokens");
        CHECK(buf[0].type == XBT_MINUS,   "21a: token[0] = MINUS");
        CHECK(buf[1].type == XBT_LIT_N,   "21a: token[1] = LIT_N");
        CHECK(buf[1].u.n == 5.0,          "21a: LIT_N value == 5.0");

        n = lex_ok("+10", buf, TBUF);
        CHECK(n == 3,                     "21b: +10 -> 3 tokens");
        CHECK(buf[0].type == XBT_PLUS,    "21b: token[0] = PLUS");
        CHECK(buf[1].type == XBT_LIT_N,   "21b: token[1] = LIT_N");
        CHECK(buf[1].u.n == 10.0,         "21b: LIT_N value == 10.0");
    }

    return TEST_SUMMARY("test-xbase-lex");
}
