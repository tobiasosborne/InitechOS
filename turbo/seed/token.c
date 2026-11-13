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
    case TOK_KW_BOOLEAN: return "KW_BOOLEAN";
    case TOK_KW_AND:     return "KW_AND";
    case TOK_KW_OR:      return "KW_OR";
    case TOK_KW_NOT:     return "KW_NOT";
    case TOK_KW_TRUE:    return "KW_TRUE";
    case TOK_KW_FALSE:   return "KW_FALSE";
    case TOK_KW_IF:      return "KW_IF";
    case TOK_KW_THEN:    return "KW_THEN";
    case TOK_KW_ELSE:    return "KW_ELSE";
    case TOK_KW_WHILE:   return "KW_WHILE";
    case TOK_KW_DO:      return "KW_DO";
    case TOK_KW_FOR:     return "KW_FOR";
    case TOK_KW_TO:      return "KW_TO";
    case TOK_KW_DOWNTO:  return "KW_DOWNTO";
    case TOK_KW_REPEAT:  return "KW_REPEAT";
    case TOK_KW_UNTIL:   return "KW_UNTIL";
    case TOK_KW_CONST:   return "KW_CONST";
    case TOK_KW_CHAR:    return "KW_CHAR";
    case TOK_KW_ORD:     return "KW_ORD";
    case TOK_KW_CHR:     return "KW_CHR";
    case TOK_KW_PROCEDURE: return "KW_PROCEDURE";
    case TOK_KW_FUNCTION:  return "KW_FUNCTION";
    case TOK_KW_FORWARD:   return "KW_FORWARD";
    case TOK_KW_ARRAY:   return "KW_ARRAY";
    case TOK_KW_OF:      return "KW_OF";
    case TOK_KW_TYPE:    return "KW_TYPE";
    case TOK_KW_RECORD:  return "KW_RECORD";
    case TOK_KW_STRING:  return "KW_STRING";  /* B7 (beads initech-39k2) */
    case TOK_KW_LENGTH:  return "KW_LENGTH";  /* B7 (beads initech-39k2) */
    case TOK_KW_FILE:    return "KW_FILE";    /* B8 (beads initech-ogxv) */
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
    case TOK_EQ:         return "EQ";
    case TOK_NE:         return "NE";
    case TOK_LT:         return "LT";
    case TOK_LE:         return "LE";
    case TOK_GT:         return "GT";
    case TOK_GE:         return "GE";
    case TOK_LBRACKET:   return "LBRACKET";
    case TOK_RBRACKET:   return "RBRACKET";
    case TOK_DOTDOT:     return "DOTDOT";
    }
    return "?";
}
