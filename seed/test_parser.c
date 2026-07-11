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

/* ------------------------------------------------------------------ */
/* B2 (beads initech-80iw; ADR-0007 DEC-02): if/while/for/repeat        */
/* ------------------------------------------------------------------ */
static void test_if_else_ast(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var a : integer;\n"
        "begin if a = 1 then a := 2 else a := 3 end.",
        buf, sizeof buf);
    CHECK(rc == 0, "if/then/else parses");
    CHECK_STR_EQ(buf,
        "(program P (var (varref a):integer) "
        "(block (if (= (varref a) (int 1)) (assign a (int 2)) "
        "(assign a (int 3)))))",
        "if/then/else AST shape");
}

static void test_if_no_else_ast(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var a : integer;\n"
        "begin if a = 1 then a := 2 end.",
        buf, sizeof buf);
    CHECK(rc == 0, "if with no else parses");
    CHECK_STR_EQ(buf,
        "(program P (var (varref a):integer) "
        "(block (if (= (varref a) (int 1)) (assign a (int 2)))))",
        "if with no else has no trailing else_stmt in the dump");
}

/* Dangling-else: must bind to the NEAREST unmatched 'if' -- the inner if's
 * else, not the outer's (parser.h's grammar comment; ISO 7185 / Turbo
 * Pascal canonical resolution). If this ever bound to the OUTER if instead,
 * the dump would show the else attached to the wrong (if ...) node -- a
 * real, structural difference, not a coincidence. */
static void test_dangling_else_binds_nearest_ast(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var a, b : integer;\n"
        "begin if a = 1 then if b = 2 then a := 3 else a := 4 end.",
        buf, sizeof buf);
    CHECK(rc == 0, "nested if with dangling else parses");
    CHECK_STR_EQ(buf,
        "(program P (var (varref a):integer (varref b):integer) "
        "(block (if (= (varref a) (int 1)) "
        "(if (= (varref b) (int 2)) (assign a (int 3)) (assign a (int 4))))))",
        "dangling else binds to the nearest (inner) if, not the outer");
}

static void test_while_ast(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var i : integer;\n"
        "begin while i < 10 do i := i + 1 end.",
        buf, sizeof buf);
    CHECK(rc == 0, "while/do parses");
    CHECK_STR_EQ(buf,
        "(program P (var (varref i):integer) "
        "(block (while (< (varref i) (int 10)) "
        "(assign i (+ (varref i) (int 1))))))",
        "while AST shape");
}

/* for .. to: SUGAR (ADR-0007 DEC-02) desugared entirely at parse time into
 * "v := e1; <limit> := e2; while v <= limit do begin S; v := v+1 end" (see
 * parser.c's parse_for). The synthesized limit variable ("__forlim_1") is
 * appended to the program's decls AFTER the user's own var-section -- this
 * test pins that exact desugared shape down so a future refactor of the
 * desugar can't silently change the once-evaluated-bound structure. */
static void test_for_to_desugars_to_while(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var i, x : integer;\n"
        "begin for i := 1 to 5 do x := x + i end.",
        buf, sizeof buf);
    CHECK(rc == 0, "for .. to parses");
    CHECK_STR_EQ(buf,
        "(program P (var (varref i):integer (varref x):integer) "
        "(var (varref __forlim_1):integer) "
        "(block (block (assign i (int 1)) (assign __forlim_1 (int 5)) "
        "(while (<= (varref i) (varref __forlim_1)) "
        "(block (assign x (+ (varref x) (varref i))) "
        "(assign i (+ (varref i) (int 1))))))))",
        "for .. to desugars to a once-evaluated-limit while loop");
}

/* for .. downto: guard is ">=" and the step is "v - 1" (mirrors the 'to'
 * case with the two swaps DEC-02's dialect pinning calls for). */
static void test_for_downto_desugars_to_while(void)
{
    char buf[1024];
    int rc = parse_to_str(
        "program P; var i, x : integer;\n"
        "begin for i := 5 downto 1 do x := x + i end.",
        buf, sizeof buf);
    CHECK(rc == 0, "for .. downto parses");
    CHECK_STR_EQ(buf,
        "(program P (var (varref i):integer (varref x):integer) "
        "(var (varref __forlim_1):integer) "
        "(block (block (assign i (int 5)) (assign __forlim_1 (int 1)) "
        "(while (>= (varref i) (varref __forlim_1)) "
        "(block (assign x (+ (varref x) (varref i))) "
        "(assign i (- (varref i) (int 1))))))))",
        "for .. downto uses >= and a decrementing step");
}

/* repeat-until: SUGAR, desugars to "the body block, once, followed by a
 * while-not-guard loop over the SAME (shared) body block" (see parser.c's
 * parse_repeat) -- runs at least once, guard tested AFTER the body. */
static void test_repeat_until_desugars_to_while(void)
{
    char buf[2048];
    int rc = parse_to_str(
        "program P; var i, s : integer;\n"
        "begin i := 1; s := 0; "
        "repeat s := s + i; i := i + 1 until i > 3 end.",
        buf, sizeof buf);
    CHECK(rc == 0, "repeat/until parses");
    CHECK_STR_EQ(buf,
        "(program P (var (varref i):integer (varref s):integer) "
        "(block (assign i (int 1)) (assign s (int 0)) "
        "(block (block (assign s (+ (varref s) (varref i))) "
        "(assign i (+ (varref i) (int 1)))) "
        "(while (not (> (varref i) (int 3))) "
        "(block (assign s (+ (varref s) (varref i))) "
        "(assign i (+ (varref i) (int 1))))))))",
        "repeat/until runs the body once inline then loops on 'not guard'");
}

static void test_typecheck_if_requires_boolean(void)
{
    char buf[1024];
    int rc = check_to_str(
        "program P; var a : integer;\n"
        "begin if a then a := 1 end.",
        buf, sizeof buf);
    CHECK(rc != 0, "an integer 'if' condition is a type error");
    CHECK(strstr(buf, "TYPE_ERR@") != NULL, "error is tagged as a type error");
    CHECK(strstr(buf, "'if' condition must be boolean") != NULL,
          "diagnostic names the problem");
}

static void test_typecheck_while_requires_boolean(void)
{
    char buf[1024];
    int rc = check_to_str(
        "program P; var i : integer;\n"
        "begin while i do i := i + 1 end.",
        buf, sizeof buf);
    CHECK(rc != 0, "an integer 'while' condition is a type error");
    CHECK(strstr(buf, "TYPE_ERR@") != NULL, "error is tagged as a type error");
    CHECK(strstr(buf, "'while' condition must be boolean") != NULL,
          "diagnostic names the problem");
}

/* repeat's guard must be boolean too -- enforced for free by the desugared
 * 'not' (check_unop already requires a boolean operand), so the located
 * diagnostic is 'not's, not a repeat-specific message (see typecheck.h's
 * B2 note). */
static void test_typecheck_repeat_guard_requires_boolean(void)
{
    char buf[1024];
    int rc = check_to_str(
        "program P; var i : integer;\n"
        "begin repeat i := i + 1 until i end.",
        buf, sizeof buf);
    CHECK(rc != 0, "an integer repeat-guard is a type error");
    CHECK(strstr(buf, "TYPE_ERR@") != NULL, "error is tagged as a type error");
    CHECK(strstr(buf, "'not' requires a boolean operand") != NULL,
          "diagnostic surfaces via the desugared 'not' check");
}

/* for-bounds must be integer -- enforced for free by the desugared
 * assignment (v := e1 requires e1's type to match v's declared type). */
static void test_typecheck_for_bound_requires_integer(void)
{
    char buf[1024];
    int rc = check_to_str(
        "program P; var i : integer; done : boolean;\n"
        "begin for i := 1 to done do i := i end.",
        buf, sizeof buf);
    CHECK(rc != 0, "a boolean for-loop bound is a type error");
    CHECK(strstr(buf, "TYPE_ERR@") != NULL, "error is tagged as a type error");
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
    test_if_else_ast();
    test_if_no_else_ast();
    test_dangling_else_binds_nearest_ast();
    test_while_ast();
    test_for_to_desugars_to_while();
    test_for_downto_desugars_to_while();
    test_repeat_until_desugars_to_while();
    test_typecheck_if_requires_boolean();
    test_typecheck_while_requires_boolean();
    test_typecheck_repeat_guard_requires_boolean();
    test_typecheck_for_bound_requires_integer();
    return TEST_SUMMARY("test_parser");
}
