/*
 * lexer.h -- single-pass lexer over a source buffer for the seed front end.
 *
 * beads: initech-znb ("Step A of the InitechOS seed cross-compiler")
 * Ref:   PRD Sec 6.7 (single-pass front end), CLAUDE.md Law 1 / Rule 12.
 *
 * Error-handling strategy (DECIDED, kept consistent across the front end):
 * the lexer NEVER longjmps and never aborts. On a lexical fault it returns a
 * TOK_ERROR token whose .lexeme is a static diagnostic and whose line/col
 * locate the fault. The driver/parser decide what to do. This keeps the lexer
 * a pure function of position and makes the tests deterministic (CLAUDE.md
 * Law 2). A TOK_EOF is returned at end of input and on every subsequent call.
 *
 * Lifetime: the Lexer borrows the source buffer; the caller owns it and must
 * keep it alive for the lexer's lifetime. The lexer owns a small internal
 * scratch pool for decoded TOK_STRING payloads: each string token gets a
 * distinct slice of the pool (a write cursor advances per token), so multiple
 * string tokens can be held live at once. The pool is finite (LEXER_STRBUF_CAP
 * bytes total across all live strings); when it would overflow, the lexer
 * wraps the cursor back to the start -- so a TOK_STRING stays valid until
 * roughly LEXER_STRBUF_CAP bytes of later string literals have been decoded.
 * A single-pass parser that consumes (or copies, e.g. into the AST arena) each
 * string before lexing far past it never sees stale data.
 */
#ifndef SEED_LEXER_H
#define SEED_LEXER_H

#include <stddef.h>
#include "token.h"

#define LEXER_STRBUF_CAP 1024  /* max decoded string-literal length */

typedef struct {
    const char *src;    /* borrowed source buffer */
    size_t      len;    /* length of src in bytes */
    size_t      pos;    /* current byte offset */
    int         line;   /* 1-based current line */
    int         col;    /* 1-based current column */
    size_t      strpos; /* write cursor into strbuf (per-token slices) */
    char        strbuf[LEXER_STRBUF_CAP + 1]; /* decoded TOK_STRING pool */
} Lexer;

/* Initialise a lexer over [src, src+len). src must outlive the lexer. */
void lexer_init(Lexer *lx, const char *src, size_t len);

/* Produce the next token. Skips whitespace and comments. Returns TOK_EOF at
 * end and TOK_ERROR (with located message) on a lexical fault. */
Token lexer_next(Lexer *lx);

#endif /* SEED_LEXER_H */
