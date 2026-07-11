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
#include "typecheck.h"

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

/* Parse THEN typecheck `src` (B1, beads initech-f0uc). Returns 0 iff both
 * succeed. On success `buf` gets the AST dump (proving typecheck doesn't
 * mutate the tree shape); on a parse OR type error, `buf` gets the located
 * message with a "PARSE" or "TYPE" tag so callers can tell which stage
 * failed. */
static int check_to_str(const char *src, char *buf, size_t cap)
{
    AstArena arena;
    ParseResult r;
    int rc;
    ast_arena_init(&arena);
    rc = parse_program(src, strlen(src), &arena, &r);
    if (rc != 0) {
        snprintf(buf, cap, "PARSE_ERR@%d:%d %s", r.line, r.col, r.error);
        ast_arena_free(&arena);
        return rc;
    }
    TypeCheckResult tc;
    rc = typecheck_program(r.ast, &tc);
    if (rc != 0) {
        snprintf(buf, cap, "TYPE_ERR@%d:%d %s", tc.line, tc.col, tc.error);
        ast_arena_free(&arena);
        return rc;
    }
    FILE *fp = fmemopen(buf, cap, "w");
    ast_dump(r.ast, fp);
    fclose(fp);
    ast_arena_free(&arena);
    return 0;
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

/* ------------------------------------------------------------------ */
/* B1 (beads initech-f0uc; ADR-0007 DEC-02/DEC-03): relational + boolean +
 * and/or/not, and the precedence reshape.                             */
/* ------------------------------------------------------------------ */
static void test_boolean_var_decl_and_literals(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var p, q : boolean;\n"
        "begin p := true; q := false end.",
        buf, sizeof buf);
    CHECK(rc == 0, "boolean var-decl + true/false literals parse");
    CHECK_STR_EQ(buf,
        "(program P "
        "(var (varref p):boolean (varref q):boolean) "
        "(block (assign p (bool true)) (assign q (bool false))))",
        "boolean var-decl dumps :boolean; true/false dump as (bool ..)");
}

static void test_relational_all_six(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var a, b : integer;\n"
        "begin writeln(a = b); writeln(a <> b); writeln(a < b); "
        "writeln(a <= b); writeln(a > b); writeln(a >= b) end.",
        buf, sizeof buf);
    CHECK(rc == 0, "all six relational operators parse");
    CHECK_STR_EQ(buf,
        "(program P "
        "(var (varref a):integer (varref b):integer) "
        "(block "
        "(writeln (= (varref a) (varref b))) "
        "(writeln (<> (varref a) (varref b))) "
        "(writeln (< (varref a) (varref b))) "
        "(writeln (<= (varref a) (varref b))) "
        "(writeln (> (varref a) (varref b))) "
        "(writeln (>= (varref a) (varref b)))))",
        "each relational op dumps with its own operator name");
}

/* 'and' joins the MULTIPLYING operators (same level as * div mod): "a and b
 * or c" must parse as (a and b) or c, i.e. 'and' binds tighter than 'or',
 * exactly mirroring '*' binding tighter than '+' (test_precedence_mul_
 * over_add above). */
static void test_and_binds_like_mul(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var a, b, c : boolean;\n"
        "begin writeln(a and b or c) end.",
        buf, sizeof buf);
    CHECK(rc == 0, "a and b or c parses");
    CHECK_STR_EQ(buf,
        "(program P "
        "(var (varref a):boolean (varref b):boolean (varref c):boolean) "
        "(block (writeln (or (and (varref a) (varref b)) (varref c)))))",
        "'and' binds tighter than 'or' (mul-class vs add-class)");
}

/* 'not' binds tighter than 'and': "not a and b" is (not a) and b, exactly
 * as unary '-' binds tighter than '+'/'*' for arithmetic. */
static void test_not_binds_tighter_than_and(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var a, b : boolean;\n"
        "begin writeln(not a and b) end.",
        buf, sizeof buf);
    CHECK(rc == 0, "not a and b parses");
    CHECK_STR_EQ(buf,
        "(program P "
        "(var (varref a):boolean (varref b):boolean) "
        "(block (writeln (and (not (varref a)) (varref b)))))",
        "'not' binds tighter than 'and'");
}

/* Relational is the OUTERMOST (lowest-precedence) level: "1 + 2 < 2 * 2"
 * must parse as (1+2) < (2*2), never e.g. 1 + (2 < 2) * 2. */
static void test_relational_is_lowest_precedence(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; begin writeln(1 + 2 < 2 * 2) end.", buf, sizeof buf);
    CHECK(rc == 0, "1 + 2 < 2 * 2 parses");
    CHECK_STR_EQ(buf,
        "(program P (block "
        "(writeln (< (+ (int 1) (int 2)) (* (int 2) (int 2))))))",
        "relational is outermost: arithmetic fully groups on each side");
}

/* Pascal relations do not chain: "a < b = c" must be a located syntax
 * error, not silently parse as ((a<b)=c) or (a<(b=c)). */
static void test_relational_chaining_is_error(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var a, b, c : integer;\n"
        "begin writeln(a < b = c) end.",
        buf, sizeof buf);
    CHECK(rc != 0, "'a < b = c' is a syntax error (relations don't chain)");
    CHECK(strstr(buf, "ERR@") != NULL, "error carries a location");
    CHECK(strstr(buf, "chain") != NULL,
          "diagnostic specifically names the chaining problem, not a "
          "generic 'expected token' message");
}

/* ------------------------------------------------------------------ */
/* B1 typecheck (seed/typecheck.c): minimal sound type discipline.     */
/* ------------------------------------------------------------------ */
static void test_typecheck_ok_program(void)
{
    char buf[1024];
    int rc = check_to_str(
        "program P; var a : integer; p : boolean;\n"
        "begin a := 3; p := a < 5; writeln(p) end.",
        buf, sizeof buf);
    CHECK(rc == 0, "a well-typed B1 program passes typecheck");
    CHECK_STR_EQ(buf,
        "(program P "
        "(var (varref a):integer) (var (varref p):boolean) "
        "(block (assign a (int 3)) (assign p (< (varref a) (int 5))) "
        "(writeln (varref p))))",
        "typecheck does not alter the AST dump shape");
}

static void test_typecheck_assign_type_mismatch(void)
{
    char buf[1024];
    int rc = check_to_str(
        "program P; var a : integer;\n"
        "begin a := true end.",
        buf, sizeof buf);
    CHECK(rc != 0, "assigning a boolean literal to an integer var is a type error");
    CHECK(strstr(buf, "TYPE_ERR@") != NULL, "error is tagged as a type error");
    CHECK(strstr(buf, "mismatch") != NULL, "diagnostic names the mismatch");
}

static void test_typecheck_and_requires_boolean(void)
{
    char buf[1024];
    int rc = check_to_str(
        "program P; var p : boolean;\n"
        "begin p := 1 and 2 end.",
        buf, sizeof buf);
    CHECK(rc != 0, "'and' on integer operands is a type error");
    CHECK(strstr(buf, "TYPE_ERR@") != NULL, "error is tagged as a type error");
}

static void test_typecheck_relational_operand_mismatch(void)
{
    char buf[1024];
    int rc = check_to_str(
        "program P; var a : integer; p : boolean;\n"
        "begin p := (a = true) end.",
        buf, sizeof buf);
    CHECK(rc != 0, "comparing integer to boolean is a type error");
    CHECK(strstr(buf, "TYPE_ERR@") != NULL, "error is tagged as a type error");
}

static void test_typecheck_undeclared_variable(void)
{
    char buf[1024];
    int rc = check_to_str(
        "program P; begin writeln(x) end.", buf, sizeof buf);
    CHECK(rc != 0, "referencing an undeclared variable is a type error");
    CHECK(strstr(buf, "undeclared") != NULL, "diagnostic names the problem");
}

static void test_typecheck_duplicate_declaration(void)
{
    char buf[1024];
    int rc = check_to_str(
        "program P; var a : integer; a : boolean;\n"
        "begin a := true end.",
        buf, sizeof buf);
    CHECK(rc != 0, "redeclaring a variable name is a type error");
    CHECK(strstr(buf, "duplicate") != NULL, "diagnostic names the problem");
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
    test_boolean_var_decl_and_literals();
    test_relational_all_six();
    test_and_binds_like_mul();
    test_not_binds_tighter_than_and();
    test_relational_is_lowest_precedence();
    test_relational_chaining_is_error();
    test_typecheck_ok_program();
    test_typecheck_assign_type_mismatch();
    test_typecheck_and_requires_boolean();
    test_typecheck_relational_operand_mismatch();
    test_typecheck_undeclared_variable();
    test_typecheck_duplicate_declaration();
    return TEST_SUMMARY("test_parser");
}
