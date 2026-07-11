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
 * B1 addition (beads initech-f0uc; ADR-0007 DEC-02): the six relational
 * operators, and the `boolean`/`and`/`or`/`not`/`true`/`false` keywords.
 * DECISION (report per the bead): ISO 7185 / Turbo Pascal treat True and
 * False as predefined CONSTANT identifiers of type Boolean, not reserved
 * words -- a real TP program could (unusually) declare a variable named
 * `true` and shadow the constant. This seed's minimal subset treats
 * true/false as RESERVED KEYWORDS instead: identifier-shadowing of a
 * predefined constant is exactly the kind of TP7-parity complexity ADR-0007
 * DEC-02 says to avoid (DR-2, minimality) and Turbo Initech's own source
 * does not need it. Documented here as a deliberate, minor divergence from
 * strict ISO/TP lexical rules, not an oversight.
 *
 * B2 addition (beads initech-80iw; ADR-0007 DEC-02 "if/then/else, while/do
 * as primitive control flow", "for/repeat/case ... pure sugar"): the
 * if/then/else and while/do keywords (primitive), plus for/to/downto and
 * repeat/until (sugar, desugared entirely in seed/parser.c -- see
 * parser.h's grammar comment). `case` is NOT implemented (optional per
 * ADR-0007 DEC-02's own text; the if-chain idiom already covers it, and B2's
 * bead explicitly lists it as "optional/absent").
 *
 * B3 addition (beads initech-7mo3; ADR-0007 DEC-02 "const declarations",
 * "char, ord, chr"): the `const` and `char` keywords, plus `ord`/`chr` as
 * RESERVED KEYWORDS (not ordinary identifiers that happen to name a
 * built-in function -- this subset has no general function-call syntax yet,
 * B4 lands that; ord/chr are recognized directly by the parser, mirroring
 * how true/false were made reserved at B1 for the same minimality reason,
 * ADR-0007 DEC-02/DR-2). A real TP program could declare a variable or
 * procedure named `ord`/`chr`/`const`/`char`; this seed's minimal subset
 * does not need that flexibility and reserving the words avoids a whole
 * class of shadowing complexity for zero self-host cost.
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
    /* B1 keywords (beads initech-f0uc) */
    TOK_KW_BOOLEAN,
    TOK_KW_AND,
    TOK_KW_OR,
    TOK_KW_NOT,
    TOK_KW_TRUE,
    TOK_KW_FALSE,
    /* B2 keywords (beads initech-80iw): primitive control flow (if/then/
     * else, while/do) plus the for/to/downto/repeat/until sugar family. */
    TOK_KW_IF,
    TOK_KW_THEN,
    TOK_KW_ELSE,
    TOK_KW_WHILE,
    TOK_KW_DO,
    TOK_KW_FOR,
    TOK_KW_TO,
    TOK_KW_DOWNTO,
    TOK_KW_REPEAT,
    TOK_KW_UNTIL,
    /* B3 keywords (beads initech-7mo3): const declarations, the char type,
     * and ord/chr (reserved -- see the header comment above). */
    TOK_KW_CONST,
    TOK_KW_CHAR,
    TOK_KW_ORD,
    TOK_KW_CHR,
    /* B4 keywords (beads initech-63ce): procedure/function declarations and
     * the `forward` directive (ADR-0007 DEC-02 + the ratification amendment
     * that made `forward` a required directive). Reserved words in this
     * subset (see lexer.c's B4 note). */
    TOK_KW_PROCEDURE,
    TOK_KW_FUNCTION,
    TOK_KW_FORWARD,

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
    TOK_STAR,       /* * */
    /* B1 relational operators (beads initech-f0uc). '=' is EQUALITY here
     * (never confused with TOK_ASSIGN ':='); '<>' is NOT-EQUAL. */
    TOK_EQ,         /* = */
    TOK_NE,         /* <> */
    TOK_LT,         /* < */
    TOK_LE,         /* <= */
    TOK_GT,         /* > */
    TOK_GE          /* >= */
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
