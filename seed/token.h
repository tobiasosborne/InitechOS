/*
 * token.h -- token kinds and the Token struct for the seed Pascal front end.
 *
 * beads: initech-znb ("Step A of the InitechOS seed cross-compiler")
 * Ref:   PRD Sec 6.7 (Turbo Initech: Turbo-Pascal-flavoured subset, single
 *        pass), PRD Sec 4 (seed vs resident -- same language). CLAUDE.md
 *        Law 1 (cite sources), Rule 12 (ASCII-clean).
 *
 * Scope NOTE: this is the MINIMAL scaffold subset only (issue initech-znb).
 * Keywords/tokens for if/while/for/procedure/record/pointer are deliberately
 * NOT present yet -- later steps grow the set. See parser.h for the grammar.
 *
 * Pascal is case-insensitive: keywords and identifiers are matched
 * case-insensitively by the lexer. The Token stores the source lexeme verbatim
 * (a span into the source buffer); semantic comparison is done case-folded.
 */
#ifndef SEED_TOKEN_H
#define SEED_TOKEN_H

#include <stddef.h>

typedef enum {
    TOK_EOF = 0,    /* end of source */
    TOK_ERROR,      /* a lexical error; .lexeme points at an error message */

    /* literals / identifiers */
    TOK_IDENT,      /* identifier (case-insensitive) */
    TOK_INT,        /* integer literal; .ivalue holds the value */
    TOK_STRING,     /* string literal; .lexeme/.length is the DECODED text */

    /* keywords (Pascal, case-insensitive) */
    TOK_KW_PROGRAM,
    TOK_KW_VAR,
    TOK_KW_BEGIN,
    TOK_KW_END,
    TOK_KW_INTEGER,
    TOK_KW_DIV,
    TOK_KW_MOD,
    TOK_KW_WRITE,
    TOK_KW_WRITELN,

    /* punctuation / operators */
    TOK_SEMI,       /* ; */
    TOK_DOT,        /* . */
    TOK_COMMA,      /* , */
    TOK_COLON,      /* : */
    TOK_ASSIGN,     /* := */
    TOK_LPAREN,     /* ( */
    TOK_RPAREN,     /* ) */
    TOK_PLUS,       /* + */
    TOK_MINUS,      /* - */
    TOK_STAR        /* * */
} TokenKind;

/*
 * A Token. For most kinds .lexeme points into the source buffer (NOT NUL
 * terminated) with .length giving its extent. For TOK_STRING the decoded
 * payload is owned by the lexer's scratch buffer (see lexer.c) and is valid
 * until the next lexer_next() call -- the parser copies what it needs. For
 * TOK_ERROR, .lexeme points at a static, NUL-terminated diagnostic message.
 *
 * line/col are 1-based and point at the first character of the token.
 */
typedef struct {
    TokenKind   kind;
    const char *lexeme;   /* span start (source) or message (errors/strings) */
    size_t      length;   /* span length in bytes (excl. NUL) */
    long        ivalue;   /* value for TOK_INT */
    int         line;     /* 1-based line of the token start */
    int         col;      /* 1-based column of the token start */
} Token;

/* Human-readable name for a token kind (for dumps and error messages). */
const char *token_kind_name(TokenKind kind);

#endif /* SEED_TOKEN_H */
