/*
 * lexer.c -- single-pass lexer for the seed Pascal front-end subset.
 *
 * beads: initech-znb ("Step A of the InitechOS seed cross-compiler")
 * Ref:   PRD Sec 6.7 (Turbo-Pascal-flavoured subset, single-pass front end),
 *        PRD Sec 4 (seed targets the same language). CLAUDE.md Law 1 / Law 2 /
 *        Rule 12.
 *
 * Pascal is case-insensitive (keywords AND identifiers); we fold case on the
 * keyword check. Comments are { ... } and (* ... *). String literals are
 * '...' with '' as an escaped single quote. Error strategy: return a located
 * TOK_ERROR with a static message -- never longjmp, never abort (see header).
 *
 * ASCII-clean (Rule 12). The decoded payload of TOK_STRING lives in the
 * lexer's reused strbuf and is valid only until the next lexer_next() call.
 */
#include "lexer.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* Character helpers (ASCII; CLAUDE.md Rule 12)                       */
/* ------------------------------------------------------------------ */
static int is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f'
        || c == '\v';
}

static int is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static int is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_alnum(char c)
{
    return is_alpha(c) || is_digit(c);
}

static char to_lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

/* ------------------------------------------------------------------ */
/* Low-level cursor                                                   */
/* ------------------------------------------------------------------ */
static int at_end(const Lexer *lx)
{
    return lx->pos >= lx->len;
}

static char peek(const Lexer *lx)
{
    return at_end(lx) ? '\0' : lx->src[lx->pos];
}

static char peek2(const Lexer *lx)
{
    return (lx->pos + 1 >= lx->len) ? '\0' : lx->src[lx->pos + 1];
}

/* Advance one byte, maintaining 1-based line/col. */
static char advance(Lexer *lx)
{
    char c = lx->src[lx->pos++];
    if (c == '\n') {
        lx->line++;
        lx->col = 1;
    } else {
        lx->col++;
    }
    return c;
}

/* ------------------------------------------------------------------ */
/* Token construction                                                 */
/* ------------------------------------------------------------------ */
static Token make_simple(TokenKind kind, const char *start, size_t len,
                         int line, int col)
{
    Token t;
    t.kind = kind;
    t.lexeme = start;
    t.length = len;
    t.ivalue = 0;
    t.line = line;
    t.col = col;
    return t;
}

static Token make_error(const char *msg, int line, int col)
{
    Token t;
    t.kind = TOK_ERROR;
    t.lexeme = msg;            /* static, NUL-terminated */
    t.length = strlen(msg);
    t.ivalue = 0;
    t.line = line;
    t.col = col;
    return t;
}

void lexer_init(Lexer *lx, const char *src, size_t len)
{
    lx->src = src;
    lx->len = len;
    lx->pos = 0;
    lx->line = 1;
    lx->col = 1;
    lx->strpos = 0;
    lx->strbuf[0] = '\0';
}

/* ------------------------------------------------------------------ */
/* Whitespace + comments                                              */
/* ------------------------------------------------------------------ */
/*
 * Skip whitespace and both comment forms. Returns 0 normally, or non-zero if
 * an unterminated comment was hit; in that case *err is set to a located
 * TOK_ERROR for the caller to return.
 */
static int skip_trivia(Lexer *lx, Token *err)
{
    for (;;) {
        char c = peek(lx);
        if (c == '\0' && at_end(lx))
            return 0;

        if (is_space(c)) {
            advance(lx);
            continue;
        }

        /* Brace comment: { ... } */
        if (c == '{') {
            int sl = lx->line, sc = lx->col;
            advance(lx);
            while (!at_end(lx) && peek(lx) != '}')
                advance(lx);
            if (at_end(lx)) {
                *err = make_error("unterminated '{' comment", sl, sc);
                return 1;
            }
            advance(lx); /* consume '}' */
            continue;
        }

        /* Paren-star comment: (* ... *) */
        if (c == '(' && peek2(lx) == '*') {
            int sl = lx->line, sc = lx->col;
            advance(lx); /* ( */
            advance(lx); /* * */
            for (;;) {
                if (at_end(lx)) {
                    *err = make_error("unterminated '(*' comment", sl, sc);
                    return 1;
                }
                if (peek(lx) == '*' && peek2(lx) == ')') {
                    advance(lx); /* * */
                    advance(lx); /* ) */
                    break;
                }
                advance(lx);
            }
            continue;
        }

        return 0; /* real token ahead */
    }
}

/* ------------------------------------------------------------------ */
/* Keyword lookup (case-insensitive)                                  */
/* ------------------------------------------------------------------ */
static TokenKind keyword_kind(const char *s, size_t n)
{
    /* Case-fold into a small fixed buffer; identifiers longer than the
     * longest keyword cannot be keywords. */
    char buf[16];
    if (n >= sizeof(buf))
        return TOK_IDENT;
    for (size_t i = 0; i < n; i++)
        buf[i] = to_lower(s[i]);
    buf[n] = '\0';

    if (strcmp(buf, "program") == 0) return TOK_KW_PROGRAM;
    if (strcmp(buf, "var") == 0)     return TOK_KW_VAR;
    if (strcmp(buf, "begin") == 0)   return TOK_KW_BEGIN;
    if (strcmp(buf, "end") == 0)     return TOK_KW_END;
    if (strcmp(buf, "integer") == 0) return TOK_KW_INTEGER;
    if (strcmp(buf, "div") == 0)     return TOK_KW_DIV;
    if (strcmp(buf, "mod") == 0)     return TOK_KW_MOD;
    if (strcmp(buf, "write") == 0)   return TOK_KW_WRITE;
    if (strcmp(buf, "writeln") == 0) return TOK_KW_WRITELN;
    /* B1 (beads initech-f0uc): boolean type + and/or/not + true/false.
     * true/false are RESERVED KEYWORDS in this subset -- see the token.h
     * DECISION note (a deliberate divergence from ISO/TP predefined-constant
     * treatment, for minimality per ADR-0007 DEC-02/DR-2). */
    if (strcmp(buf, "boolean") == 0) return TOK_KW_BOOLEAN;
    if (strcmp(buf, "and") == 0)     return TOK_KW_AND;
    if (strcmp(buf, "or") == 0)      return TOK_KW_OR;
    if (strcmp(buf, "not") == 0)     return TOK_KW_NOT;
    if (strcmp(buf, "true") == 0)    return TOK_KW_TRUE;
    if (strcmp(buf, "false") == 0)   return TOK_KW_FALSE;
    /* B2 (beads initech-80iw): if/then/else/while/do (primitive) plus
     * for/to/downto/repeat/until (sugar, desugared in parser.c). */
    if (strcmp(buf, "if") == 0)      return TOK_KW_IF;
    if (strcmp(buf, "then") == 0)    return TOK_KW_THEN;
    if (strcmp(buf, "else") == 0)    return TOK_KW_ELSE;
    if (strcmp(buf, "while") == 0)   return TOK_KW_WHILE;
    if (strcmp(buf, "do") == 0)      return TOK_KW_DO;
    if (strcmp(buf, "for") == 0)     return TOK_KW_FOR;
    if (strcmp(buf, "to") == 0)      return TOK_KW_TO;
    if (strcmp(buf, "downto") == 0)  return TOK_KW_DOWNTO;
    if (strcmp(buf, "repeat") == 0)  return TOK_KW_REPEAT;
    if (strcmp(buf, "until") == 0)   return TOK_KW_UNTIL;
    /* B3 (beads initech-7mo3): const/char, plus ord/chr reserved as
     * built-in-function keywords (see token.h's B3 DECISION note). */
    if (strcmp(buf, "const") == 0)   return TOK_KW_CONST;
    if (strcmp(buf, "char") == 0)    return TOK_KW_CHAR;
    if (strcmp(buf, "ord") == 0)     return TOK_KW_ORD;
    if (strcmp(buf, "chr") == 0)     return TOK_KW_CHR;
    return TOK_IDENT;
}

/* ------------------------------------------------------------------ */
/* Scanners for compound tokens                                       */
/* ------------------------------------------------------------------ */
static Token scan_ident_or_kw(Lexer *lx, int line, int col)
{
    size_t start = lx->pos;
    while (is_alnum(peek(lx)))
        advance(lx);
    size_t len = lx->pos - start;
    TokenKind k = keyword_kind(lx->src + start, len);
    return make_simple(k, lx->src + start, len, line, col);
}

static Token scan_number(Lexer *lx, int line, int col)
{
    size_t start = lx->pos;
    long value = 0;
    while (is_digit(peek(lx))) {
        value = value * 10 + (advance(lx) - '0');
    }
    Token t = make_simple(TOK_INT, lx->src + start, lx->pos - start,
                          line, col);
    t.ivalue = value;
    return t;
}

/*
 * Scan a '...' string literal into the lexer scratch buffer, decoding the ''
 * escape to a single '. The leading quote has already been consumed by the
 * caller's dispatch (we re-consume here for clarity -- see scan dispatch).
 */
static Token scan_string(Lexer *lx, int line, int col)
{
    /* On entry the opening quote is at peek(); consume it. We decode into a
     * fresh slice of the pool starting at the current write cursor, so prior
     * string tokens stay valid (see header lifetime note). The slice needs
     * room for the decoded bytes plus a NUL; if the TAIL of the pool (from
     * the write cursor to LEXER_STRBUF_CAP) runs out mid-decode, we relocate
     * the bytes already decoded for THIS string to the pool start and keep
     * going there (initech-2hg9: the wrap used to only trigger when zero
     * bytes of the current string had been written yet, so a string that
     * had already stored >=1 byte before hitting the tail was wrongly
     * rejected as "too long" even though it fit from pool start -- see
     * seed/test_lexer.c test_string_pool_wrap_mid_string). Relocating a
     * live in-progress slice is safe: per the header's lifetime note, an
     * OLDER token's slice is only guaranteed valid until the pool wraps
     * around it anyway, and this wrap is the same "cursor returns to pool
     * start" event that already existed for the out==0 case -- we're just
     * making it fire at the right time instead of missing it whenever
     * out>0. */
    advance(lx);

    char *dst = lx->strbuf + lx->strpos;
    size_t avail = LEXER_STRBUF_CAP - lx->strpos; /* bytes before the NUL slot */
    size_t out = 0;

    for (;;) {
        if (at_end(lx))
            return make_error("unterminated string literal", line, col);
        char c = peek(lx);
        if (c == '\'') {
            /* '' is an escaped single quote; a lone ' ends the string. */
            if (peek2(lx) == '\'') {
                advance(lx); /* first ' */
                advance(lx); /* second ' */
                c = '\'';
            } else {
                advance(lx); /* closing ' */
                break;
            }
        } else if (c == '\n') {
            return make_error("unterminated string literal", line, col);
        } else {
            advance(lx);
        }

        if (out >= avail) {
            /* The tail from the write cursor to LEXER_STRBUF_CAP is
             * exhausted. If we are not already decoding at the pool start,
             * relocate the `out` bytes decoded so far for THIS string down
             * to strbuf[0] and continue there -- a full CAP bytes are then
             * available regardless of how much tail we started with. Only
             * a string that still can't fit starting from pool start (i.e.
             * we are already at pool start and still ran out) is a genuine
             * over-capacity "too long". Guard against relocation being a
             * same-pointer no-op memmove when we're already at start. */
            if (dst != lx->strbuf) {
                memmove(lx->strbuf, dst, out);
                dst = lx->strbuf;
                avail = LEXER_STRBUF_CAP;
                /* Keep lx->strpos in lockstep with dst rather than leaving
                 * it stale until the end-of-function bookkeeping (which
                 * would still net out correctly here since strpos0 + out +
                 * 1 is guaranteed > CAP whenever a relocation fired -- but
                 * an explicit reset keeps the invariant local and obvious
                 * instead of resting on that off-site arithmetic). */
                lx->strpos = 0;
            } else {
                return make_error("string literal too long", line, col);
            }
        }
        dst[out++] = c;
    }

    dst[out] = '\0';
    Token t = make_simple(TOK_STRING, dst, out, line, col);
    lx->strpos += out + 1; /* reserve this slice incl. its NUL */
    if (lx->strpos > LEXER_STRBUF_CAP)
        lx->strpos = 0;     /* exhausted; next string wraps */
    return t;
}

/* ------------------------------------------------------------------ */
/* lexer_next                                                         */
/* ------------------------------------------------------------------ */
Token lexer_next(Lexer *lx)
{
    Token err;
    if (skip_trivia(lx, &err))
        return err;

    int line = lx->line, col = lx->col;

    if (at_end(lx))
        return make_simple(TOK_EOF, lx->src + lx->pos, 0, line, col);

    char c = peek(lx);

    if (is_alpha(c))
        return scan_ident_or_kw(lx, line, col);
    if (is_digit(c))
        return scan_number(lx, line, col);
    if (c == '\'')
        return scan_string(lx, line, col);

    /* Single- and two-char punctuation. */
    switch (c) {
    case ';': advance(lx); return make_simple(TOK_SEMI, lx->src + lx->pos - 1, 1, line, col);
    case '.': advance(lx); return make_simple(TOK_DOT, lx->src + lx->pos - 1, 1, line, col);
    case ',': advance(lx); return make_simple(TOK_COMMA, lx->src + lx->pos - 1, 1, line, col);
    case '(': advance(lx); return make_simple(TOK_LPAREN, lx->src + lx->pos - 1, 1, line, col);
    case ')': advance(lx); return make_simple(TOK_RPAREN, lx->src + lx->pos - 1, 1, line, col);
    case '+': advance(lx); return make_simple(TOK_PLUS, lx->src + lx->pos - 1, 1, line, col);
    case '-': advance(lx); return make_simple(TOK_MINUS, lx->src + lx->pos - 1, 1, line, col);
    case '*': advance(lx); return make_simple(TOK_STAR, lx->src + lx->pos - 1, 1, line, col);
    case ':':
        advance(lx);
        if (peek(lx) == '=') {
            advance(lx);
            return make_simple(TOK_ASSIGN, lx->src + lx->pos - 2, 2, line, col);
        }
        return make_simple(TOK_COLON, lx->src + lx->pos - 1, 1, line, col);
    /* B1 relational operators (beads initech-f0uc). '=' is ALWAYS equality
     * in this subset -- Pascal ':=' (assignment) is a distinct two-char
     * token already handled above under ':', so a lone '=' can never be
     * confused with it (the lexer never needs to "disambiguate" '=' vs
     * ':=' -- they simply start on different characters). */
    case '=': advance(lx); return make_simple(TOK_EQ, lx->src + lx->pos - 1, 1, line, col);
    case '<':
        advance(lx);
        if (peek(lx) == '>') {
            advance(lx);
            return make_simple(TOK_NE, lx->src + lx->pos - 2, 2, line, col);
        }
        if (peek(lx) == '=') {
            advance(lx);
            return make_simple(TOK_LE, lx->src + lx->pos - 2, 2, line, col);
        }
        return make_simple(TOK_LT, lx->src + lx->pos - 1, 1, line, col);
    case '>':
        advance(lx);
        if (peek(lx) == '=') {
            advance(lx);
            return make_simple(TOK_GE, lx->src + lx->pos - 2, 2, line, col);
        }
        return make_simple(TOK_GT, lx->src + lx->pos - 1, 1, line, col);
    default:
        break;
    }

    /* Unknown byte: located error, do not advance past it ambiguously. */
    advance(lx);
    return make_error("unexpected character", line, col);
}
