/*
 * token.c -- human-readable token-kind names for dumps and diagnostics.
 *
 * beads: initech-znb ("Step A of the InitechOS seed cross-compiler")
 * Ref:   PRD Sec 6.7, CLAUDE.md Law 1 / Rule 12 (ASCII-clean).
 */
#include "token.h"

const char *token_kind_name(TokenKind kind)
{
    switch (kind) {
    case TOK_EOF:        return "EOF";
    case TOK_ERROR:      return "ERROR";
    case TOK_IDENT:      return "IDENT";
    case TOK_INT:        return "INT";
    case TOK_STRING:     return "STRING";
    case TOK_KW_PROGRAM: return "KW_PROGRAM";
    case TOK_KW_VAR:     return "KW_VAR";
    case TOK_KW_BEGIN:   return "KW_BEGIN";
    case TOK_KW_END:     return "KW_END";
    case TOK_KW_INTEGER: return "KW_INTEGER";
    case TOK_KW_DIV:     return "KW_DIV";
    case TOK_KW_MOD:     return "KW_MOD";
    case TOK_KW_WRITE:   return "KW_WRITE";
    case TOK_KW_WRITELN: return "KW_WRITELN";
    case TOK_SEMI:       return "SEMI";
    case TOK_DOT:        return "DOT";
    case TOK_COMMA:      return "COMMA";
    case TOK_COLON:      return "COLON";
    case TOK_ASSIGN:     return "ASSIGN";
    case TOK_LPAREN:     return "LPAREN";
    case TOK_RPAREN:     return "RPAREN";
    case TOK_PLUS:       return "PLUS";
    case TOK_MINUS:      return "MINUS";
    case TOK_STAR:       return "STAR";
    }
    return "?";
}
