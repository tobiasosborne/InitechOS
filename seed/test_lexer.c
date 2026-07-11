/*
 * test_lexer.c -- unit tests for the seed Pascal lexer.
 *
 * beads: initech-znb ("Step A of the InitechOS seed cross-compiler")
 * Ref:   CLAUDE.md Rule 1 (Red->Green->Refactor: written to fail first),
 *        Law 2 (the oracle is the truth). Rule 12 (ASCII-clean).
 *
 * Asserts token streams for: keywords (case-insensitive), identifiers,
 * numbers, operators (incl. := div mod), both comment forms, string literals
 * with the '' escape, and two error cases (bad char, unterminated string).
 */
#include <string.h>
#include "test_assert.h"
#include "lexer.h"

TEST_HARNESS();

/* Lex `src` fully into `out` (up to cap tokens incl. trailing EOF). Returns
 * the number of tokens placed (including the terminating EOF/ERROR). */
static int lex_all(const char *src, Token *out, int cap)
{
    Lexer lx;
    int n = 0;
    lexer_init(&lx, src, strlen(src));
    for (;;) {
        Token t = lexer_next(&lx);
        if (n < cap) out[n++] = t;
        if (t.kind == TOK_EOF || t.kind == TOK_ERROR) break;
    }
    return n;
}

static void test_keywords_case_insensitive(void)
{
    Token t[16];
    int n = lex_all("Program VAR Begin EnD integer DIV Mod WriteLn write",
                    t, 16);
    CHECK(n == 10, "ten keyword tokens + EOF");
    CHECK(t[0].kind == TOK_KW_PROGRAM, "Program -> KW_PROGRAM");
    CHECK(t[1].kind == TOK_KW_VAR, "VAR -> KW_VAR");
    CHECK(t[2].kind == TOK_KW_BEGIN, "Begin -> KW_BEGIN");
    CHECK(t[3].kind == TOK_KW_END, "EnD -> KW_END");
    CHECK(t[4].kind == TOK_KW_INTEGER, "integer -> KW_INTEGER");
    CHECK(t[5].kind == TOK_KW_DIV, "DIV -> KW_DIV");
    CHECK(t[6].kind == TOK_KW_MOD, "Mod -> KW_MOD");
    CHECK(t[7].kind == TOK_KW_WRITELN, "WriteLn -> KW_WRITELN");
    CHECK(t[8].kind == TOK_KW_WRITE, "write -> KW_WRITE");
    CHECK(t[9].kind == TOK_EOF, "trailing EOF");
}

static void test_idents_and_numbers(void)
{
    Token t[16];
    int n = lex_all("foo Bar123 _x 42 0 1000", t, 16);
    CHECK(n == 7, "six tokens + EOF");
    CHECK(t[0].kind == TOK_IDENT && t[0].length == 3, "ident foo");
    CHECK(t[1].kind == TOK_IDENT && t[1].length == 6, "ident Bar123");
    CHECK(t[2].kind == TOK_IDENT && t[2].length == 2, "ident _x");
    CHECK(t[3].kind == TOK_INT && t[3].ivalue == 42, "int 42");
    CHECK(t[4].kind == TOK_INT && t[4].ivalue == 0, "int 0");
    CHECK(t[5].kind == TOK_INT && t[5].ivalue == 1000, "int 1000");
}

static void test_operators(void)
{
    Token t[24];
    int n = lex_all(":= : ; . , ( ) + - *", t, 24);
    CHECK(n == 11, "ten operator tokens + EOF");
    CHECK(t[0].kind == TOK_ASSIGN, ":=");
    CHECK(t[1].kind == TOK_COLON, ":");
    CHECK(t[2].kind == TOK_SEMI, ";");
    CHECK(t[3].kind == TOK_DOT, ".");
    CHECK(t[4].kind == TOK_COMMA, ",");
    CHECK(t[5].kind == TOK_LPAREN, "(");
    CHECK(t[6].kind == TOK_RPAREN, ")");
    CHECK(t[7].kind == TOK_PLUS, "+");
    CHECK(t[8].kind == TOK_MINUS, "-");
    CHECK(t[9].kind == TOK_STAR, "*");
}

/* B1 (beads initech-f0uc): the six relational operators. '=' is never
 * confused with ':=' (TOK_ASSIGN is scanned entirely under the ':' case,
 * see lexer.c) -- a bare '=' always lexes as TOK_EQ. */
static void test_relational_operators(void)
{
    Token t[16];
    int n = lex_all("= <> < <= > >=", t, 16);
    CHECK(n == 7, "six relational tokens + EOF");
    CHECK(t[0].kind == TOK_EQ, "= -> TOK_EQ");
    CHECK(t[1].kind == TOK_NE, "<> -> TOK_NE");
    CHECK(t[2].kind == TOK_LT, "< -> TOK_LT");
    CHECK(t[3].kind == TOK_LE, "<= -> TOK_LE");
    CHECK(t[4].kind == TOK_GT, "> -> TOK_GT");
    CHECK(t[5].kind == TOK_GE, ">= -> TOK_GE");
}

/* '=' must never be swallowed into an assignment: distinct from ':=' both
 * lexically (different leading character) and by kind. */
static void test_eq_not_confused_with_assign(void)
{
    Token t[8];
    int n = lex_all("x := y = z", t, 8);
    CHECK(n == 6, "ident := ident = ident + EOF");
    CHECK(t[0].kind == TOK_IDENT, "x");
    CHECK(t[1].kind == TOK_ASSIGN, ":= is TOK_ASSIGN");
    CHECK(t[2].kind == TOK_IDENT, "y");
    CHECK(t[3].kind == TOK_EQ, "= is TOK_EQ, not TOK_ASSIGN");
    CHECK(t[4].kind == TOK_IDENT, "z");
}

/* B1 keywords: boolean type + and/or/not + true/false (reserved keywords in
 * this subset -- see the token.h DECISION note). */
static void test_boolean_keywords(void)
{
    Token t[16];
    int n = lex_all("boolean And Or NOT True False", t, 16);
    CHECK(n == 7, "six boolean-family keyword tokens + EOF");
    CHECK(t[0].kind == TOK_KW_BOOLEAN, "boolean -> KW_BOOLEAN");
    CHECK(t[1].kind == TOK_KW_AND, "And -> KW_AND (case-insensitive)");
    CHECK(t[2].kind == TOK_KW_OR, "Or -> KW_OR (case-insensitive)");
    CHECK(t[3].kind == TOK_KW_NOT, "NOT -> KW_NOT (case-insensitive)");
    CHECK(t[4].kind == TOK_KW_TRUE, "True -> KW_TRUE (case-insensitive)");
    CHECK(t[5].kind == TOK_KW_FALSE, "False -> KW_FALSE (case-insensitive)");
}

/* B3 (beads initech-7mo3): const/char, plus ord/chr reserved as built-in-
 * function keywords (see token.h's B3 DECISION note). */
static void test_char_b3_keywords(void)
{
    Token t[16];
    int n = lex_all("Const CHAR Ord chr", t, 16);
    CHECK(n == 5, "four B3 keyword tokens + EOF");
    CHECK(t[0].kind == TOK_KW_CONST, "Const -> KW_CONST (case-insensitive)");
    CHECK(t[1].kind == TOK_KW_CHAR, "CHAR -> KW_CHAR (case-insensitive)");
    CHECK(t[2].kind == TOK_KW_ORD, "Ord -> KW_ORD (case-insensitive)");
    CHECK(t[3].kind == TOK_KW_CHR, "chr -> KW_CHR");
}

static void test_comments(void)
{
    Token t[8];
    /* Both comment forms, including one spanning content. */
    int n = lex_all("a { brace comment } b (* paren comment *) c", t, 8);
    CHECK(n == 4, "three idents + EOF, comments skipped");
    CHECK(t[0].kind == TOK_IDENT && t[0].length == 1, "a");
    CHECK(t[1].kind == TOK_IDENT && t[1].length == 1, "b");
    CHECK(t[2].kind == TOK_IDENT && t[2].length == 1, "c");
    CHECK(t[3].kind == TOK_EOF, "EOF after comments");
}

static void test_comment_line_tracking(void)
{
    Token t[4];
    /* A brace comment containing a newline; the token after must be on line 2. */
    int n = lex_all("{ first\nline } x", t, 4);
    CHECK(n == 2, "one ident + EOF");
    CHECK(t[0].kind == TOK_IDENT, "x after multiline comment");
    CHECK(t[0].line == 2, "line advanced through comment newline");
}

static void test_strings(void)
{
    Token t[8];
    /* Plain string, and one with a '' escaped quote (-> a single '). */
    int n = lex_all("'hello' 'it''s'", t, 8);
    CHECK(n == 3, "two strings + EOF");
    CHECK(t[0].kind == TOK_STRING, "first is a string");
    CHECK(t[0].length == 5 && strncmp(t[0].lexeme, "hello", 5) == 0,
          "decoded hello");
    CHECK(t[1].kind == TOK_STRING, "second is a string");
    CHECK(t[1].length == 4 && strncmp(t[1].lexeme, "it's", 4) == 0,
          "'' decodes to a single quote");
}

static void test_empty_string(void)
{
    Token t[4];
    int n = lex_all("''", t, 4);
    CHECK(n == 2, "empty string + EOF");
    CHECK(t[0].kind == TOK_STRING && t[0].length == 0, "empty string literal");
}

static void test_error_bad_char(void)
{
    Token t[4];
    int n = lex_all("a @ b", t, 4);
    /* a, then ERROR on '@' (lexing stops in lex_all on ERROR). */
    CHECK(n == 2, "ident then ERROR token");
    CHECK(t[0].kind == TOK_IDENT, "a before bad char");
    CHECK(t[1].kind == TOK_ERROR, "bad char yields ERROR");
    CHECK(t[1].line == 1 && t[1].col == 3, "error located at the '@'");
}

static void test_error_unterminated_string(void)
{
    Token t[4];
    int n = lex_all("x 'no close", t, 4);
    CHECK(n == 2, "ident then ERROR");
    CHECK(t[0].kind == TOK_IDENT, "x before bad string");
    CHECK(t[1].kind == TOK_ERROR, "unterminated string yields ERROR");
}

/*
 * initech-2hg9: scan_string's pool-wrap only fired when out==0 (i.e. no
 * bytes of the CURRENT string had been written yet). If the pool cursor
 * (lx.strpos) sits near LEXER_STRBUF_CAP with only a small tail left, and
 * the string needs more than that tail but still fits from the start of
 * the pool, the old code wrongly returned "string literal too long" as
 * soon as it had written >=1 byte and hit the tail boundary.
 *
 * Ref: CLAUDE.md Law 2 (oracle asserts real token output, not
 * by-construction) + Rule 1 (this test must fail RED on the unfixed
 * scan_string before the fix lands). Lexer/Lexer.strpos/strbuf are plain
 * struct fields (lexer.h), so the test seeds pool state directly rather
 * than lexing thousands of throwaway strings to walk the cursor there --
 * that keeps the oracle a direct, independent check of the wrap math
 * itself instead of an indirect proof through unrelated lexer behavior.
 */
static void test_string_pool_wrap_mid_string(void)
{
    Lexer lx;
    lexer_init(&lx, "'hello'", strlen("'hello'"));

    /* Seed the pool cursor so only 4 bytes remain before LEXER_STRBUF_CAP:
     * avail = CAP - strpos = 4. "hello" is 5 bytes, so the 5th byte lands
     * exactly on the old out==0-only wrap check with out=4 (not 0). */
    lx.strpos = LEXER_STRBUF_CAP - 4;

    Token t = lexer_next(&lx);
    CHECK(t.kind == TOK_STRING, "5-byte string must lex with a 4-byte tail");
    CHECK(t.length == 5 && strncmp(t.lexeme, "hello", 5) == 0,
          "wrapped string decodes correctly (no truncation/corruption)");
}

/* A string of exactly LEXER_STRBUF_CAP bytes must always fit, regardless of
 * where the pool cursor starts (it wraps to pool-start first if needed). */
static void test_string_pool_exact_cap(void)
{
    static char src[LEXER_STRBUF_CAP + 3]; /* ' + CAP bytes + ' + NUL */
    size_t i;
    src[0] = '\'';
    for (i = 0; i < LEXER_STRBUF_CAP; i++)
        src[1 + i] = 'x';
    src[1 + LEXER_STRBUF_CAP] = '\'';
    src[2 + LEXER_STRBUF_CAP] = '\0';

    Lexer lx;
    lexer_init(&lx, src, strlen(src));
    lx.strpos = LEXER_STRBUF_CAP - 4; /* same tight tail as above */

    Token t = lexer_next(&lx);
    CHECK(t.kind == TOK_STRING, "exactly-CAP string must lex, not error");
    CHECK(t.length == LEXER_STRBUF_CAP, "exactly-CAP string keeps full length");
}

/* A string of CAP+1 bytes can never fit in the pool (even from a fresh
 * pool-start) and must still, correctly, error "too long". */
static void test_string_pool_over_cap_still_errors(void)
{
    static char src[LEXER_STRBUF_CAP + 4]; /* ' + CAP+1 bytes + ' + NUL */
    size_t i;
    src[0] = '\'';
    for (i = 0; i < LEXER_STRBUF_CAP + 1; i++)
        src[1 + i] = 'x';
    src[2 + LEXER_STRBUF_CAP] = '\'';
    src[3 + LEXER_STRBUF_CAP] = '\0';

    Lexer lx;
    lexer_init(&lx, src, strlen(src));
    /* Fresh pool (strpos == 0) -- the largest possible tail -- so a failure
     * here can only be the genuine over-capacity case, not a wrap bug. */

    Token t = lexer_next(&lx);
    CHECK(t.kind == TOK_ERROR, "CAP+1 string must still error (too long)");
}

static void test_error_unterminated_comment(void)
{
    Token t[4];
    int n = lex_all("a { never closes", t, 4);
    CHECK(n == 2, "ident then ERROR");
    CHECK(t[0].kind == TOK_IDENT, "a before bad comment");
    CHECK(t[1].kind == TOK_ERROR, "unterminated comment yields ERROR");
}

int main(void)
{
    test_keywords_case_insensitive();
    test_idents_and_numbers();
    test_operators();
    test_relational_operators();
    test_eq_not_confused_with_assign();
    test_boolean_keywords();
    test_char_b3_keywords();
    test_comments();
    test_comment_line_tracking();
    test_strings();
    test_empty_string();
    test_string_pool_wrap_mid_string();
    test_string_pool_exact_cap();
    test_string_pool_over_cap_still_errors();
    test_error_bad_char();
    test_error_unterminated_string();
    test_error_unterminated_comment();
    return TEST_SUMMARY("test_lexer");
}
