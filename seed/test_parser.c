/*
 * test_parser.c -- unit tests for the seed Pascal recursive-descent parser.
 *
 * beads: initech-znb ("Step A of the InitechOS seed cross-compiler")
 * Ref:   CLAUDE.md Rule 1 (Red->Green->Refactor), Law 2 (oracle is truth),
 *        Rule 12 (ASCII-clean). PRD Sec 6.7 (the language subset).
 *
 * Asserts AST shape via the compact S-expression dump for: a full minimal
 * program; expression precedence/associativity; a writeln with mixed
 * string+int args; and two syntax-error cases (missing ';', missing 'end.').
 */
#include <string.h>
#include <stdio.h>
#include "test_assert.h"
#include "parser.h"

TEST_HARNESS();

/* Parse `src`; on success render the AST S-expression into `buf`. Returns the
 * parser rc (0 = ok). On error, copies the located message into buf instead. */
static int parse_to_str(const char *src, char *buf, size_t cap)
{
    AstArena arena;
    ParseResult r;
    int rc;
    ast_arena_init(&arena);
    rc = parse_program(src, strlen(src), &arena, &r);
    if (rc == 0) {
        FILE *fp = fmemopen(buf, cap, "w");
        ast_dump(r.ast, fp);
        fclose(fp); /* fmemopen NUL-terminates on close/flush within cap */
    } else {
        snprintf(buf, cap, "ERR@%d:%d %s", r.line, r.col, r.error);
    }
    ast_arena_free(&arena);
    return rc;
}

static void test_minimal_program(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program Hello;\n"
        "var x : integer;\n"
        "begin\n"
        "  x := 5;\n"
        "  writeln('InitechOS seed OK')\n"
        "end.\n",
        buf, sizeof buf);
    CHECK(rc == 0, "minimal program parses");
    CHECK_STR_EQ(buf,
        "(program Hello "
        "(var (varref x):integer) "
        "(block "
        "(assign x (int 5)) "
        "(writeln (str \"InitechOS seed OK\"))))",
        "minimal program AST shape");
}

static void test_multi_var_decl(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var a, b : integer; c : integer;\n"
        "begin a := 1; b := 2; c := 3 end.",
        buf, sizeof buf);
    CHECK(rc == 0, "multi var-decl parses");
    CHECK_STR_EQ(buf,
        "(program P "
        "(var (varref a):integer (varref b):integer) "
        "(var (varref c):integer) "
        "(block (assign a (int 1)) (assign b (int 2)) (assign c (int 3))))",
        "two var-decl groups, multi-name first");
}

static void test_precedence_mul_over_add(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; begin writeln(1 + 2 * 3) end.", buf, sizeof buf);
    CHECK(rc == 0, "1+2*3 parses");
    CHECK_STR_EQ(buf,
        "(program P (block "
        "(writeln (+ (int 1) (* (int 2) (int 3))))))",
        "* binds tighter than +");
}

static void test_precedence_parens(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; begin writeln((1 + 2) * 3) end.", buf, sizeof buf);
    CHECK(rc == 0, "(1+2)*3 parses");
    CHECK_STR_EQ(buf,
        "(program P (block "
        "(writeln (* (+ (int 1) (int 2)) (int 3)))))",
        "parens override precedence");
}

static void test_unary_minus(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var x : integer; begin writeln(-x + 1) end.",
        buf, sizeof buf);
    CHECK(rc == 0, "-x + 1 parses");
    CHECK_STR_EQ(buf,
        "(program P "
        "(var (varref x):integer) "
        "(block (writeln (+ (neg (varref x)) (int 1)))))",
        "unary minus binds tighter than +");
}

static void test_left_assoc_sub(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; begin writeln(10 - 3 - 2) end.", buf, sizeof buf);
    CHECK(rc == 0, "10-3-2 parses");
    CHECK_STR_EQ(buf,
        "(program P (block "
        "(writeln (- (- (int 10) (int 3)) (int 2)))))",
        "- is left-associative");
}

static void test_div_mod(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; begin writeln(7 div 2 mod 2) end.", buf, sizeof buf);
    CHECK(rc == 0, "7 div 2 mod 2 parses");
    CHECK_STR_EQ(buf,
        "(program P (block "
        "(writeln (mod (div (int 7) (int 2)) (int 2)))))",
        "div and mod same level, left-assoc");
}

static void test_writeln_mixed_args(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var n : integer; "
        "begin n := 42; writeln('n = ', n, ' done') end.",
        buf, sizeof buf);
    CHECK(rc == 0, "mixed string+int writeln parses");
    CHECK_STR_EQ(buf,
        "(program P "
        "(var (varref n):integer) "
        "(block (assign n (int 42)) "
        "(writeln (str \"n = \") (varref n) (str \" done\"))))",
        "writeln with string, var, string args");
}

static void test_write_vs_writeln(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; begin write('a'); writeln('b') end.", buf, sizeof buf);
    CHECK(rc == 0, "write and writeln parse");
    CHECK_STR_EQ(buf,
        "(program P (block "
        "(write (str \"a\")) (writeln (str \"b\"))))",
        "write distinct from writeln");
}

static void test_err_missing_semi(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var x : integer\n"   /* missing ';' after decl */
        "begin x := 1 end.",
        buf, sizeof buf);
    CHECK(rc != 0, "missing ';' is a syntax error");
    CHECK(strstr(buf, "ERR@") != NULL, "error carries a location");
}

static void test_err_missing_end_dot(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; begin x := 1 end",   /* missing trailing '.' */
        buf, sizeof buf);
    CHECK(rc != 0, "missing 'end.' is a syntax error");
    CHECK(strstr(buf, "ERR@") != NULL, "error carries a location");
}

static void test_err_lexical_propagates(void)
{
    char buf[1024];
    /* A bad char inside the program must surface as a parse error, not crash. */
    int rc = parse_to_str(
        "program P; begin x := @ end.", buf, sizeof buf);
    CHECK(rc != 0, "lexical error surfaces through the parser");
}

int main(void)
{
    test_minimal_program();
    test_multi_var_decl();
    test_precedence_mul_over_add();
    test_precedence_parens();
    test_unary_minus();
    test_left_assoc_sub();
    test_div_mod();
    test_writeln_mixed_args();
    test_write_vs_writeln();
    test_err_missing_semi();
    test_err_missing_end_dot();
    test_err_lexical_propagates();
    return TEST_SUMMARY("test_parser");
}
