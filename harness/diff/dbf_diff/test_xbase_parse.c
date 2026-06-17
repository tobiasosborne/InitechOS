/*
 * harness/diff/dbf_diff/test_xbase_parse.c -- unit oracle for parse.c (S3.2).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Uses the seed
 * test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY). A non-zero
 * exit on any failed check ensures the make gate can never false-green
 * (CLAUDE.md Law 2: the oracle is the truth).
 *
 * It lexes (S3.1 lex.c) then parses (S3.2 parse.c) each expression and
 * asserts the SHAPE of the resulting AST -- node types, operator tokens, and
 * child structure -- not just "no error". A test that only checked rc>=0
 * would not catch a wrong grouping, which is the whole point of S3.2.
 *
 * -------------------------------------------------------------------------
 * THE TWO HEADLINE STRUCTURAL ASSERTIONS (corpus mint-002, NOT math):
 *   - 2^3^2 = (2^3)^2 = 64  -> root ^ whose LEFT child is itself a ^ (sec 4)
 *   - -2^2  = (-2)^2  = 4   -> root ^ whose LEFT child is a unary-minus (sec 5)
 * These are the cases -DXB_MUTATE_PARSE perturbs (right-assoc ^), which flips
 * 2^3^2's tree so the LEFT-child-is-^ assertion goes RED.
 *
 * MUTATION PROOF (Rule 6):
 *   Compile with -DXB_MUTATE_PARSE to activate the single perturbation in
 *   parse.c: ^ folds RIGHT-associatively. The 2^3^2 structural assertion
 *   (section 4) then goes RED, proving the assertion catches the regression.
 *
 * Tests (by section):
 *   1. Literals (N, C, L) and identifier -> leaf node kinds + payloads.
 *   2. Each binary operator class produces XBN_BINOP with the right op token.
 *   3. Precedence: 2+3*4 = 2+(3*4) (mul tighter than add).
 *   4. HEADLINE: 2^3^2 = (2^3)^2 (^ LEFT-assoc). MUTANT goes RED here.
 *   5. HEADLINE: -2^2 = (-2)^2 (unary minus TIGHTER than ^).
 *   6. Parentheses override precedence: (2+3)*4.
 *   7. Logical precedence: a .AND. b .OR. c = (a .AND. b) .OR. c;
 *      .NOT. a .AND. b = (.NOT. a) .AND. b.
 *   8. Relational below arithmetic: a+1 > b = (a+1) > b.
 *   9. Unary stacking: --5, .NOT. .NOT. x.
 *  10. Realistic mixed expression: A > 0 .AND. .NOT. B = (A>0) .AND. (.NOT. B).
 *  11. Errors (fail loud, Rule 2): unbalanced paren, empty, trailing tokens,
 *      operator-with-no-operand, lone ')'.
 *  12. Function-call shape (IDENT '(' ) is a S3.5 placeholder -> XBPE_UNEXPECTED.
 *      GATED: argument parsing is S3.5; S3.2 only detects the shape.
 *  13. ** is a synonym for ^ and is also LEFT-assoc.
 *
 * Compile + run (self-grade, host):
 *   gcc -std=c11 -Wall -Wextra -Werror -Iseed -Ios/samir/include \
 *       os/samir/core/parse.c os/samir/core/lex.c os/samir/core/rt.c \
 *       harness/diff/dbf_diff/test_xbase_parse.c \
 *       -o /tmp/test_xbase_parse && /tmp/test_xbase_parse
 *
 * Mutant (must go RED on section 4):
 *   gcc -std=c11 -Wall -Wextra -Werror -DXB_MUTATE_PARSE -Iseed \
 *       -Ios/samir/include \
 *       os/samir/core/parse.c os/samir/core/lex.c os/samir/core/rt.c \
 *       harness/diff/dbf_diff/test_xbase_parse.c \
 *       -o /tmp/test_xbase_parse_mut ; \
 *       /tmp/test_xbase_parse_mut ; echo "mutant exit=$?"
 *
 * Freestanding compile check (parse.c only):
 *   gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -fno-pic \
 *       -std=c11 -Wall -Wextra -Werror -Ios/samir/include \
 *       -c os/samir/core/parse.c -o /tmp/parse_free.o
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md sec.5 S3.2 oracle contract
 *   - ../dbase3-decomp/re/mint-results-002.md (2^3^2=64, -2^2=4 -- THE oracle)
 *   - ../dbase3-decomp/specs/language/expressions-and-operators.md sec.9
 *   - os/samir/include/samir/eval.h (xb_node / xb_parse API)
 *   - seed/test_assert.h (harness idiom)
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "test_assert.h"        /* seed/, on -Iseed */
#include "samir/eval.h"         /* os/samir/include/, on -Ios/samir/include */

TEST_HARNESS();

#define TBUF 64    /* token buffer */
#define NBUF 64    /* node pool    */

/*
 * Lex + parse a string. Returns the root node index (>=0) or -1 on parse
 * error; *perr receives the xb_parse_err. The token + node buffers are
 * caller-provided so the test can inspect the tree afterward.
 */
static int parse_expr(const char *s, xb_token *toks, xb_node *pool, int *perr)
{
    int lerr = 0;
    int nt = xb_lex(s, (uint32_t)strlen(s), toks, TBUF, &lerr);
    if (nt < 0) {
        /* A lex error should not reach the parser in these tests. */
        *perr = -999;
        return -1;
    }
    return xb_parse(toks, (uint32_t)nt, pool, NBUF, perr);
}

/* Convenience: parse, expect success, return root index (or -1). */
static int parse_ok(const char *s, xb_token *toks, xb_node *pool)
{
    int perr = 0;
    int root = parse_expr(s, toks, pool, &perr);
    (void)perr;
    return root;
}

int main(void)
{
    xb_token toks[TBUF];
    xb_node  pool[NBUF];

    /* ------------------------------------------------------------------ */
    /* 1. Literals + identifier leaves                                      */
    /* ------------------------------------------------------------------ */
    {
        int r;

        r = parse_ok("42", toks, pool);
        CHECK(r >= 0,                          "1a: 42 parses");
        CHECK(pool[r].type == XBN_LIT_N,       "1a: root XBN_LIT_N");
        CHECK(pool[r].u.num == 42.0,           "1a: value 42.0");

        r = parse_ok("'HI'", toks, pool);
        CHECK(r >= 0,                          "1b: 'HI' parses");
        CHECK(pool[r].type == XBN_LIT_C,       "1b: root XBN_LIT_C");
        CHECK(pool[r].u.str.len == 2,          "1b: content len 2");
        CHECK(memcmp(pool[r].u.str.p, "HI", (size_t)2) == 0, "1b: content HI");

        r = parse_ok(".T.", toks, pool);
        CHECK(r >= 0,                          "1c: .T. parses");
        CHECK(pool[r].type == XBN_LIT_L,       "1c: root XBN_LIT_L");
        CHECK(pool[r].u.log == 1,              "1c: logical true");

        r = parse_ok("SALARY", toks, pool);
        CHECK(r >= 0,                          "1d: SALARY parses");
        CHECK(pool[r].type == XBN_IDENT,       "1d: root XBN_IDENT");
        CHECK(pool[r].u.str.len == 6,          "1d: ident len 6");
        CHECK(memcmp(pool[r].u.str.p, "SALARY", (size_t)6) == 0,
              "1d: ident SALARY");
    }

    /* ------------------------------------------------------------------ */
    /* 2. Each binary operator -> XBN_BINOP with the right op token         */
    /* ------------------------------------------------------------------ */
    {
        struct { const char *s; xb_token_type op; } cases[] = {
            { "1 + 2",  XBT_PLUS    },
            { "1 - 2",  XBT_MINUS   },
            { "1 * 2",  XBT_STAR    },
            { "1 / 2",  XBT_SLASH   },
            { "1 ^ 2",  XBT_CARET   },
            { "1 < 2",  XBT_LT      },
            { "1 > 2",  XBT_GT      },
            { "1 = 2",  XBT_EQ      },
            { "1 <> 2", XBT_NEQ     },
            { "1 # 2",  XBT_HASH    },
            { "1 <= 2", XBT_LE      },
            { "1 >= 2", XBT_GE      },
            { "A $ B",  XBT_DOLLAR  },
            { ".T. .AND. .F.", XBT_AND },
            { ".T. .OR. .F.",  XBT_OR  }
        };
        int k, nc = (int)(sizeof(cases) / sizeof(cases[0]));
        for (k = 0; k < nc; k++) {
            int r = parse_ok(cases[k].s, toks, pool);
            CHECK(r >= 0,                       "2: binop parses");
            CHECK(pool[r].type == XBN_BINOP,    "2: root XBN_BINOP");
            CHECK(pool[r].op == cases[k].op,    "2: correct op token");
            CHECK(pool[r].kid[0] >= 0 && pool[r].kid[1] >= 0,
                  "2: two children present");
        }
    }

    /* ------------------------------------------------------------------ */
    /* 3. Precedence: 2+3*4 = 2+(3*4)  (mul tighter than add)               */
    /*    root = +, left = 2 (lit), right = (3*4) binop                     */
    /* ------------------------------------------------------------------ */
    {
        int r = parse_ok("2 + 3 * 4", toks, pool);
        CHECK(r >= 0,                           "3: 2+3*4 parses");
        CHECK(pool[r].type == XBN_BINOP && pool[r].op == XBT_PLUS,
              "3: root is +");
        {
            int L = pool[r].kid[0];
            int R = pool[r].kid[1];
            CHECK(pool[L].type == XBN_LIT_N && pool[L].u.num == 2.0,
                  "3: left of + is 2");
            CHECK(pool[R].type == XBN_BINOP && pool[R].op == XBT_STAR,
                  "3: right of + is (3*4)");
        }
    }

    /* ------------------------------------------------------------------ */
    /* 4. HEADLINE: 2^3^2 = (2^3)^2  (^ LEFT-associative, mint-002)         */
    /*    root = ^, LEFT child is itself a ^ (the (2^3)), RIGHT child = 2.  */
    /*    Math-standard right-assoc would put the inner ^ on the RIGHT.    */
    /*    -DXB_MUTATE_PARSE makes ^ right-assoc -> this section goes RED.  */
    /* ------------------------------------------------------------------ */
    {
        int r = parse_ok("2 ^ 3 ^ 2", toks, pool);
        CHECK(r >= 0,                           "4: 2^3^2 parses");
        CHECK(pool[r].type == XBN_BINOP && pool[r].op == XBT_CARET,
              "4: root is ^");
        {
            int L = pool[r].kid[0];
            int R = pool[r].kid[1];
            /* LEFT-assoc: the LEFT child is the inner (2^3). */
            CHECK(pool[L].type == XBN_BINOP && pool[L].op == XBT_CARET,
                  "4: LEFT child is inner ^ (LEFT-assoc, mint-002)");
            /* RIGHT child is the plain literal 2. */
            CHECK(pool[R].type == XBN_LIT_N && pool[R].u.num == 2.0,
                  "4: RIGHT child is literal 2");
            /* Inner (2^3): left=2, right=3. */
            {
                int LL = pool[L].kid[0];
                int LR = pool[L].kid[1];
                CHECK(pool[LL].type == XBN_LIT_N && pool[LL].u.num == 2.0,
                      "4: inner left is 2");
                CHECK(pool[LR].type == XBN_LIT_N && pool[LR].u.num == 3.0,
                      "4: inner right is 3");
            }
        }
    }

    /* ------------------------------------------------------------------ */
    /* 5. HEADLINE: -2^2 = (-2)^2  (unary minus binds TIGHTER than ^)       */
    /*    root = ^, LEFT child is a unary-minus node, RIGHT child = 2.     */
    /*    Math-standard would be -(2^2): root = unary-minus over a ^.      */
    /* ------------------------------------------------------------------ */
    {
        int r = parse_ok("-2 ^ 2", toks, pool);
        CHECK(r >= 0,                           "5: -2^2 parses");
        CHECK(pool[r].type == XBN_BINOP && pool[r].op == XBT_CARET,
              "5: root is ^ (NOT unary-minus -- unary binds tighter)");
        {
            int L = pool[r].kid[0];
            int R = pool[r].kid[1];
            CHECK(pool[L].type == XBN_UNOP && pool[L].op == XBT_MINUS,
                  "5: LEFT child is unary minus (binds tighter, mint-002)");
            CHECK(pool[R].type == XBN_LIT_N && pool[R].u.num == 2.0,
                  "5: RIGHT child is literal 2");
            {
                int LC = pool[L].kid[0];
                CHECK(pool[LC].type == XBN_LIT_N && pool[LC].u.num == 2.0,
                      "5: unary-minus operand is literal 2");
            }
        }
    }

    /* ------------------------------------------------------------------ */
    /* 6. Parentheses override precedence: (2+3)*4                          */
    /*    root = *, left = (2+3) binop, right = 4                           */
    /* ------------------------------------------------------------------ */
    {
        int r = parse_ok("(2 + 3) * 4", toks, pool);
        CHECK(r >= 0,                           "6: (2+3)*4 parses");
        CHECK(pool[r].type == XBN_BINOP && pool[r].op == XBT_STAR,
              "6: root is *");
        {
            int L = pool[r].kid[0];
            int R = pool[r].kid[1];
            CHECK(pool[L].type == XBN_BINOP && pool[L].op == XBT_PLUS,
                  "6: left is (2+3)");
            CHECK(pool[R].type == XBN_LIT_N && pool[R].u.num == 4.0,
                  "6: right is 4");
        }
    }

    /* ------------------------------------------------------------------ */
    /* 7. Logical precedence                                                */
    /*    7a: a .AND. b .OR. c = (a .AND. b) .OR. c (.AND. tighter)        */
    /*    7b: .NOT. a .AND. b   = (.NOT. a) .AND. b (.NOT. tighter)        */
    /* ------------------------------------------------------------------ */
    {
        int r = parse_ok("A .AND. B .OR. C", toks, pool);
        CHECK(r >= 0,                           "7a: a AND b OR c parses");
        CHECK(pool[r].type == XBN_BINOP && pool[r].op == XBT_OR,
              "7a: root is .OR.");
        {
            int L = pool[r].kid[0];
            CHECK(pool[L].type == XBN_BINOP && pool[L].op == XBT_AND,
                  "7a: left of .OR. is (a .AND. b)");
        }

        r = parse_ok(".NOT. A .AND. B", toks, pool);
        CHECK(r >= 0,                           "7b: NOT a AND b parses");
        CHECK(pool[r].type == XBN_BINOP && pool[r].op == XBT_AND,
              "7b: root is .AND.");
        {
            int L = pool[r].kid[0];
            CHECK(pool[L].type == XBN_UNOP && pool[L].op == XBT_NOT,
                  "7b: left of .AND. is (.NOT. a)");
        }
    }

    /* ------------------------------------------------------------------ */
    /* 8. Relational below arithmetic: A + 1 > B = (A+1) > B               */
    /* ------------------------------------------------------------------ */
    {
        int r = parse_ok("A + 1 > B", toks, pool);
        CHECK(r >= 0,                           "8: A+1>B parses");
        CHECK(pool[r].type == XBN_BINOP && pool[r].op == XBT_GT,
              "8: root is >");
        {
            int L = pool[r].kid[0];
            CHECK(pool[L].type == XBN_BINOP && pool[L].op == XBT_PLUS,
                  "8: left of > is (A+1)");
        }
    }

    /* ------------------------------------------------------------------ */
    /* 9. Unary stacking                                                    */
    /*    9a: - - 5 = -(-(5)) (two unary minus nodes)                       */
    /*    9b: .NOT. .NOT. X = .NOT.(.NOT.(X))                               */
    /* ------------------------------------------------------------------ */
    {
        int r = parse_ok("- - 5", toks, pool);
        CHECK(r >= 0,                           "9a: --5 parses");
        CHECK(pool[r].type == XBN_UNOP && pool[r].op == XBT_MINUS,
              "9a: root unary minus");
        {
            int C = pool[r].kid[0];
            CHECK(pool[C].type == XBN_UNOP && pool[C].op == XBT_MINUS,
                  "9a: child is unary minus");
            CHECK(pool[pool[C].kid[0]].type == XBN_LIT_N,
                  "9a: innermost is literal 5");
        }

        r = parse_ok(".NOT. .NOT. X", toks, pool);
        CHECK(r >= 0,                           "9b: NOT NOT X parses");
        CHECK(pool[r].type == XBN_UNOP && pool[r].op == XBT_NOT,
              "9b: root .NOT.");
        {
            int C = pool[r].kid[0];
            CHECK(pool[C].type == XBN_UNOP && pool[C].op == XBT_NOT,
                  "9b: child .NOT.");
        }
    }

    /* ------------------------------------------------------------------ */
    /* 10. Realistic mixed: AGE > 0 .AND. .NOT. CLEARED                     */
    /*     = (AGE > 0) .AND. (.NOT. CLEARED)                                */
    /*     [shape verified vs CLRDEP.PRG:154 idiom, expressions sec.9.1]   */
    /* ------------------------------------------------------------------ */
    {
        int r = parse_ok("AGE > 0 .AND. .NOT. CLEARED", toks, pool);
        CHECK(r >= 0,                           "10: mixed expr parses");
        CHECK(pool[r].type == XBN_BINOP && pool[r].op == XBT_AND,
              "10: root is .AND.");
        {
            int L = pool[r].kid[0];
            int R = pool[r].kid[1];
            CHECK(pool[L].type == XBN_BINOP && pool[L].op == XBT_GT,
                  "10: left is (AGE > 0)");
            CHECK(pool[R].type == XBN_UNOP && pool[R].op == XBT_NOT,
                  "10: right is (.NOT. CLEARED)");
            CHECK(pool[pool[R].kid[0]].type == XBN_IDENT,
                  "10: .NOT. operand is identifier CLEARED");
        }
    }

    /* ------------------------------------------------------------------ */
    /* 11. Errors -- fail loud (Rule 2), no silent recovery                 */
    /* ------------------------------------------------------------------ */
    {
        int perr;

        /* unbalanced: missing ')' */
        int r = parse_expr("(1 + 2", toks, pool, &perr);
        CHECK(r < 0,                            "11a: (1+2 -> error");
        CHECK(perr == XBPE_UNBALANCED,          "11a: XBPE_UNBALANCED");

        /* unbalanced: lone ')' */
        r = parse_expr(")", toks, pool, &perr);
        CHECK(r < 0,                            "11b: ) -> error");
        CHECK(perr == XBPE_UNBALANCED,          "11b: XBPE_UNBALANCED");

        /* trailing tokens after a complete expression: "1 2" */
        r = parse_expr("1 2", toks, pool, &perr);
        CHECK(r < 0,                            "11c: 1 2 -> error");
        CHECK(perr == XBPE_TRAILING,            "11c: XBPE_TRAILING");

        /* trailing extra ')': "(1) )" */
        r = parse_expr("(1) )", toks, pool, &perr);
        CHECK(r < 0,                            "11d: (1) ) -> error");
        CHECK(perr == XBPE_UNBALANCED,          "11d: XBPE_UNBALANCED");

        /* operator with no operand: "1 +" */
        r = parse_expr("1 +", toks, pool, &perr);
        CHECK(r < 0,                            "11e: 1+ -> error");
        CHECK(perr == XBPE_EMPTY,               "11e: XBPE_EMPTY (no rhs)");

        /* empty expression */
        r = parse_expr("", toks, pool, &perr);
        CHECK(r < 0,                            "11f: empty -> error");
        CHECK(perr == XBPE_EMPTY,               "11f: XBPE_EMPTY");

        /* leading binary operator: "* 3" */
        r = parse_expr("* 3", toks, pool, &perr);
        CHECK(r < 0,                            "11g: *3 -> error");
        CHECK(perr == XBPE_UNEXPECTED,          "11g: XBPE_UNEXPECTED");
    }

    /* ------------------------------------------------------------------ */
    /* 12. Function calls -> XBN_CALL + XBN_ARG list (S3.5)                  */
    /*     The IDENT '(' shape is now a real call: u.str = name, kid[0] is  */
    /*     the head of an XBN_ARG linked list (kid[0]=expr, kid[1]=next).    */
    /*     [plan S3.5; eval.h XBN_CALL/XBN_ARG contract]                     */
    /* ------------------------------------------------------------------ */
    {
        int perr;
        int r;

        /* one-arg call: UPPER(NAME) -> CALL "UPPER" with a single ARG=IDENT. */
        r = parse_expr("UPPER(NAME)", toks, pool, &perr);
        CHECK(r >= 0,                           "12a: UPPER(NAME) parses");
        CHECK(pool[r].type == XBN_CALL,         "12a: root XBN_CALL");
        CHECK(pool[r].u.str.len == 5 &&
              memcmp(pool[r].u.str.p, "UPPER", (size_t)5) == 0,
              "12a: name UPPER");
        {
            int arg0 = pool[r].kid[0];
            CHECK(arg0 >= 0 && pool[arg0].type == XBN_ARG,
                  "12a: kid[0] is first XBN_ARG");
            CHECK(pool[arg0].kid[1] == -1,      "12a: single arg (next == -1)");
            CHECK(pool[pool[arg0].kid[0]].type == XBN_IDENT,
                  "12a: arg expr is XBN_IDENT");
        }

        /* zero-arg call: DATE() -> CALL "DATE", kid[0] == -1. */
        r = parse_expr("DATE()", toks, pool, &perr);
        CHECK(r >= 0,                           "12b: DATE() parses");
        CHECK(pool[r].type == XBN_CALL,         "12b: root XBN_CALL");
        CHECK(pool[r].kid[0] == -1,             "12b: zero args (kid[0] == -1)");

        /* three-arg call: SUBSTR('ABCDEF',2,3) -> ARG spine of length 3. */
        r = parse_expr("SUBSTR('ABCDEF',2,3)", toks, pool, &perr);
        CHECK(r >= 0,                           "12c: SUBSTR(...) parses");
        CHECK(pool[r].type == XBN_CALL,         "12c: root XBN_CALL");
        {
            int a0 = pool[r].kid[0];
            int a1 = (a0 >= 0) ? pool[a0].kid[1] : -1;
            int a2 = (a1 >= 0) ? pool[a1].kid[1] : -1;
            CHECK(a0 >= 0 && a1 >= 0 && a2 >= 0, "12c: three ARG cells");
            CHECK(a2 >= 0 && pool[a2].kid[1] == -1, "12c: spine ends at arg3");
            CHECK(pool[pool[a0].kid[0]].type == XBN_LIT_C,
                  "12c: arg1 is char literal");
            CHECK(pool[pool[a1].kid[0]].type == XBN_LIT_N &&
                  pool[pool[a1].kid[0]].u.num == 2.0,
                  "12c: arg2 is numeric 2");
            CHECK(pool[pool[a2].kid[0]].type == XBN_LIT_N &&
                  pool[pool[a2].kid[0]].u.num == 3.0,
                  "12c: arg3 is numeric 3");
        }

        /* nested call as an argument: VAL(STR(5,3)) parses (sub-expr arg). */
        r = parse_expr("VAL(STR(5,3))", toks, pool, &perr);
        CHECK(r >= 0,                           "12d: nested call parses");
        CHECK(pool[r].type == XBN_CALL,         "12d: outer root XBN_CALL");
        {
            int a0 = pool[r].kid[0];
            CHECK(a0 >= 0 && pool[pool[a0].kid[0]].type == XBN_CALL,
                  "12d: arg is itself a call (XBN_CALL)");
        }

        /* call inside an operator expression: LEN(NAME) > 0 (call as operand) */
        r = parse_expr("LEN(NAME) > 0", toks, pool, &perr);
        CHECK(r >= 0,                           "12e: call in rel-expr parses");
        CHECK(pool[r].type == XBN_BINOP && pool[r].op == XBT_GT,
              "12e: root is > with a call operand");
        CHECK(pool[pool[r].kid[0]].type == XBN_CALL,
              "12e: left operand is XBN_CALL");

        /* malformed: missing close paren -> loud (unbalanced). */
        r = parse_expr("UPPER(NAME", toks, pool, &perr);
        CHECK(r < 0,                            "12f: missing ')' -> error");
        CHECK(perr == XBPE_UNBALANCED,          "12f: XBPE_UNBALANCED");

        /* malformed: trailing comma / empty arg -> loud. After the ',' an
         * operand is expected; a bare ')' there is reported as XBPE_UNBALANCED
         * by primary (deterministic). The point is it fails LOUD (Rule 2). */
        r = parse_expr("UPPER(NAME,)", toks, pool, &perr);
        CHECK(r < 0,                            "12g: trailing comma -> error");
        CHECK(perr == XBPE_UNBALANCED,          "12g: XBPE_UNBALANCED (no operand)");
    }

    /* ------------------------------------------------------------------ */
    /* 13. ** is a synonym for ^ and is LEFT-assoc (mint-002)              */
    /*     2 ** 3 ** 2 = (2**3)**2 -> root **, LEFT child is **            */
    /* ------------------------------------------------------------------ */
    {
        int r = parse_ok("2 ** 3 ** 2", toks, pool);
        CHECK(r >= 0,                           "13: 2**3**2 parses");
        CHECK(pool[r].type == XBN_BINOP && pool[r].op == XBT_STARSTAR,
              "13: root is **");
        {
            int L = pool[r].kid[0];
            int R = pool[r].kid[1];
            CHECK(pool[L].type == XBN_BINOP && pool[L].op == XBT_STARSTAR,
                  "13: LEFT child is inner ** (LEFT-assoc)");
            CHECK(pool[R].type == XBN_LIT_N && pool[R].u.num == 2.0,
                  "13: RIGHT child is literal 2");
        }
    }

    return TEST_SUMMARY("test-xbase-parse");
}
