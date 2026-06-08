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
    test_comments();
    test_comment_line_tracking();
    test_strings();
    test_empty_string();
    test_error_bad_char();
    test_error_unterminated_string();
    test_error_unterminated_comment();
    return TEST_SUMMARY("test_lexer");
}
