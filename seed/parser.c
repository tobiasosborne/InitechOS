/*
 * parser.c -- recursive-descent parser for the seed Pascal front-end subset.
 *
 * beads: initech-znb ("Step A of the InitechOS seed cross-compiler")
 * Ref:   PRD Sec 6.7 (single-pass front end), PRD Sec 4 (same language as the
 *        resident compiler). CLAUDE.md Law 1 / Law 2 / Rule 2 / Rule 12.
 *
 * Grammar (EBNF) -- see parser.h for the canonical copy:
 *   program      = "program" ident ";" [ var-section ] block "." ;
 *   var-section  = "var" var-decl ";" { var-decl ";" } ;
 *   var-decl     = ident { "," ident } ":" "integer" ;
 *   block        = "begin" [ stmt { ";" stmt } ] "end" ;
 *   stmt         = assignment | block | write-stmt | (* empty *) ;
 *   assignment   = ident ":=" expr ;
 *   write-stmt   = ("write" | "writeln") "(" [ write-args ] ")" ;
 *   write-args   = write-arg { "," write-arg } ;
 *   write-arg    = string | expr ;
 *   expr         = term { ("+" | "-") term } ;
 *   term         = factor { ("*" | "div" | "mod") factor } ;
 *   factor       = integer | ident | "(" expr ")" | "-" factor ;
 *
 * Error strategy (consistent with the lexer, see header): single-error. The
 * first lexical or syntax fault is recorded with a location and parsing stops.
 * No longjmp, no abort (except arena OOM, which is a loud fatal -- Rule 2).
 *
 * ASCII-clean (Rule 12).
 */
#include "parser.h"
#include "lexer.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Parser state                                                       */
/* ------------------------------------------------------------------ */
typedef struct {
    Lexer     lx;
    AstArena *arena;
    Token     cur;     /* lookahead token */
    int       failed;  /* set once an error has been recorded */
    char      errmsg[PARSE_ERRMSG_CAP];
    int       errline;
    int       errcol;
} Parser;

/* Record the first error only; subsequent calls are no-ops so the original
 * (most relevant) location survives. */
static void fail_at(Parser *p, int line, int col, const char *msg)
{
    if (p->failed)
        return;
    p->failed = 1;
    p->errline = line;
    p->errcol = col;
    snprintf(p->errmsg, sizeof(p->errmsg), "%s", msg);
}

/* Advance the lookahead. A lexical error becomes a recorded parse error and the
 * current token is left as the ERROR token so callers' expectations fail too. */
static void advance(Parser *p)
{
    p->cur = lexer_next(&p->lx);
    if (p->cur.kind == TOK_ERROR)
        fail_at(p, p->cur.line, p->cur.col, p->cur.lexeme);
}

static int check(const Parser *p, TokenKind k)
{
    return p->cur.kind == k;
}

/* Consume the current token if it matches; else record an error. Returns 1 on
 * match, 0 on mismatch (and the error is recorded). */
static int expect(Parser *p, TokenKind k, const char *what)
{
    if (p->failed)
        return 0;
    if (p->cur.kind != k) {
        char msg[PARSE_ERRMSG_CAP];
        snprintf(msg, sizeof(msg), "expected %s but found %s",
                 what, token_kind_name(p->cur.kind));
        fail_at(p, p->cur.line, p->cur.col, msg);
        return 0;
    }
    advance(p);
    return 1;
}

/* ------------------------------------------------------------------ */
/* Forward decls                                                      */
/* ------------------------------------------------------------------ */
static AstNode *parse_block(Parser *p);
static AstNode *parse_statement(Parser *p);
static AstNode *parse_expr(Parser *p);
static AstNode *parse_simple_expr(Parser *p);
static AstNode *parse_term(Parser *p);
static AstNode *parse_factor(Parser *p);

/* ------------------------------------------------------------------ */
/* Expressions                                                        */
/* ------------------------------------------------------------------ */
/*
 * B1 (beads initech-f0uc; ADR-0007 DEC-02) reshapes this from a single
 * additive/multiplicative ladder into the full ISO 7185 / Turbo Pascal
 * precedence ladder:
 *
 *   parse_expr        expression   -- relational, LOWEST, non-chaining
 *   parse_simple_expr  simple-expr  -- + - or  (left-assoc)
 *   parse_term         term         -- * div mod and  (left-assoc)
 *   parse_factor       factor       -- unary - / not / primary, HIGHEST
 *
 * 'and' is a MULTIPLYING operator (same level as * div mod); 'or' is an
 * ADDING operator (same level as + -). This is exactly the ISO 7185 Sec
 * 6.7.2 grammar (simple-expression = term {adding-operator term}; term =
 * factor {multiplying-operator factor}), which Turbo Pascal also follows.
 */
static AstNode *parse_factor(Parser *p)
{
    if (p->failed)
        return NULL;
    int line = p->cur.line, col = p->cur.col;

    if (check(p, TOK_MINUS)) {
        advance(p);
        AstNode *operand = parse_factor(p);
        if (p->failed)
            return NULL;
        AstNode *n = ast_new(p->arena, AST_UNOP, line, col);
        n->as.unop.op = OP_NEG;
        n->as.unop.operand = operand;
        return n;
    }
    if (check(p, TOK_KW_NOT)) {
        advance(p);
        AstNode *operand = parse_factor(p);
        if (p->failed)
            return NULL;
        AstNode *n = ast_new(p->arena, AST_UNOP, line, col);
        n->as.unop.op = OP_NOT;
        n->as.unop.operand = operand;
        return n;
    }
    if (check(p, TOK_KW_TRUE)) {
        AstNode *n = ast_new(p->arena, AST_BOOLLIT, line, col);
        n->as.boollit.value = 1;
        advance(p);
        return n;
    }
    if (check(p, TOK_KW_FALSE)) {
        AstNode *n = ast_new(p->arena, AST_BOOLLIT, line, col);
        n->as.boollit.value = 0;
        advance(p);
        return n;
    }
    if (check(p, TOK_INT)) {
        AstNode *n = ast_new(p->arena, AST_INTLIT, line, col);
        n->as.intlit.value = p->cur.ivalue;
        advance(p);
        return n;
    }
    if (check(p, TOK_IDENT)) {
        AstNode *n = ast_new(p->arena, AST_VARREF, line, col);
        n->as.varref.name = ast_arena_strndup(p->arena, p->cur.lexeme,
                                              p->cur.length);
        advance(p);
        return n;
    }
    if (check(p, TOK_LPAREN)) {
        advance(p);
        AstNode *inner = parse_expr(p);  /* full expression, incl. relational */
        if (!expect(p, TOK_RPAREN, "')'"))
            return NULL;
        return inner;
    }

    fail_at(p, line, col, "expected an expression");
    return NULL;
}

static AstNode *parse_term(Parser *p)
{
    AstNode *lhs = parse_factor(p);
    while (!p->failed
           && (check(p, TOK_STAR) || check(p, TOK_KW_DIV)
               || check(p, TOK_KW_MOD) || check(p, TOK_KW_AND))) {
        int line = p->cur.line, col = p->cur.col;
        AstOp op = check(p, TOK_STAR) ? OP_MUL
                 : check(p, TOK_KW_DIV) ? OP_DIV
                 : check(p, TOK_KW_MOD) ? OP_MOD : OP_AND;
        advance(p);
        AstNode *rhs = parse_factor(p);
        if (p->failed)
            return NULL;
        AstNode *n = ast_new(p->arena, AST_BINOP, line, col);
        n->as.binop.op = op;
        n->as.binop.lhs = lhs;
        n->as.binop.rhs = rhs;
        lhs = n;
    }
    return p->failed ? NULL : lhs;
}

static AstNode *parse_simple_expr(Parser *p)
{
    AstNode *lhs = parse_term(p);
    while (!p->failed
           && (check(p, TOK_PLUS) || check(p, TOK_MINUS) || check(p, TOK_KW_OR))) {
        int line = p->cur.line, col = p->cur.col;
#ifdef SEED_MUT_PARSE_ADD_AS_MUL
        /* MUTATION HOOK (Rule 6; beads initech-tf3c, restoring the
         * initech-znb one-off manual "OP_ADD->OP_MUL in the IR" perturbation
         * as a repeatable gate). Compile with -DSEED_MUT_PARSE_ADD_AS_MUL to
         * build a '+' AST_BINOP node with op=OP_MUL instead of OP_ADD.
         * test-seed-mutant asserts this makes
         * test_precedence_mul_over_add's AST S-expression check ("(+ (int 1)
         * (* (int 2) (int 3)))") go RED. Scoped to '+' only -- '-'/'or' are
         * unaffected by this historical mutant. */
        AstOp op = check(p, TOK_PLUS) ? OP_MUL
                 : check(p, TOK_MINUS) ? OP_SUB : OP_OR;
#else
        AstOp op = check(p, TOK_PLUS) ? OP_ADD
                 : check(p, TOK_MINUS) ? OP_SUB : OP_OR;
#endif
        advance(p);
        AstNode *rhs = parse_term(p);
        if (p->failed)
            return NULL;
        AstNode *n = ast_new(p->arena, AST_BINOP, line, col);
        n->as.binop.op = op;
        n->as.binop.lhs = lhs;
        n->as.binop.rhs = rhs;
        lhs = n;
    }
    return p->failed ? NULL : lhs;
}

static int check_relop(const Parser *p)
{
    return check(p, TOK_EQ) || check(p, TOK_NE) || check(p, TOK_LT)
        || check(p, TOK_LE) || check(p, TOK_GT) || check(p, TOK_GE);
}

static AstOp relop_to_ast_op(TokenKind k)
{
    switch (k) {
    case TOK_EQ: return OP_EQ;
    case TOK_NE: return OP_NE;
    case TOK_LT: return OP_LT;
    case TOK_LE: return OP_LE;
    case TOK_GT: return OP_GT;
    default:     return OP_GE; /* TOK_GE, the only remaining case reachable
                                * via check_relop's contract; never reached
                                * for any other token. */
    }
}

/*
 * expression = simple-expr [ relational-op simple-expr ] ;
 *
 * Pascal relations do NOT chain: after the optional single relational
 * operator, seeing ANOTHER one immediately (e.g. "a < b = c") is a syntax
 * error, not "= c" silently trailing. We fail loud with a specific,
 * located diagnostic rather than falling through to the generic "expected
 * ';' but found '='" message a caller would otherwise produce -- Rule 2
 * (fail fast, fail loud) applied to a genuinely common Pascal-newcomer
 * mistake.
 */
static AstNode *parse_expr(Parser *p)
{
    AstNode *lhs = parse_simple_expr(p);
    if (p->failed)
        return NULL;
    if (!check_relop(p))
        return lhs; /* no relational operator: pass through unchanged */

    int line = p->cur.line, col = p->cur.col;
    AstOp op = relop_to_ast_op(p->cur.kind);
    advance(p);
    AstNode *rhs = parse_simple_expr(p);
    if (p->failed)
        return NULL;

    AstNode *n = ast_new(p->arena, AST_BINOP, line, col);
    n->as.binop.op = op;
    n->as.binop.lhs = lhs;
    n->as.binop.rhs = rhs;

    if (check_relop(p)) {
        fail_at(p, p->cur.line, p->cur.col,
                "relational operators do not chain in Pascal "
                "('a op b op c' is not valid; parenthesize and combine "
                "with 'and'/'or' instead)");
        return NULL;
    }
    return n;
}

/* ------------------------------------------------------------------ */
/* Statements                                                         */
/* ------------------------------------------------------------------ */
static AstNode *parse_write(Parser *p, int is_newline)
{
    int line = p->cur.line, col = p->cur.col;
    advance(p); /* consume write/writeln */

    AstNode *n = ast_new(p->arena, is_newline ? AST_WRITELN : AST_WRITE,
                         line, col);
    n->as.write.is_newline = is_newline;
    ast_list_init(&n->as.write.args);

    /* A bare `writeln` (no parens) is legal; `()` is an empty arg list. */
    if (!check(p, TOK_LPAREN))
        return n;
    advance(p); /* ( */

    if (!check(p, TOK_RPAREN)) {
        for (;;) {
            if (check(p, TOK_STRING)) {
                AstNode *s = ast_new(p->arena, AST_STRLIT,
                                     p->cur.line, p->cur.col);
                s->as.strlit.text = ast_arena_strndup(p->arena, p->cur.lexeme,
                                                      p->cur.length);
                s->as.strlit.length = p->cur.length;
                advance(p);
                ast_list_push(p->arena, &n->as.write.args, s);
            } else {
                AstNode *e = parse_expr(p);
                if (p->failed)
                    return NULL;
                ast_list_push(p->arena, &n->as.write.args, e);
            }
            if (check(p, TOK_COMMA)) {
                advance(p);
                continue;
            }
            break;
        }
    }
    if (!expect(p, TOK_RPAREN, "')'"))
        return NULL;
    return n;
}

static AstNode *parse_assignment(Parser *p)
{
    int line = p->cur.line, col = p->cur.col;
    char *name = ast_arena_strndup(p->arena, p->cur.lexeme, p->cur.length);
    advance(p); /* ident */
    if (!expect(p, TOK_ASSIGN, "':='"))
        return NULL;
    AstNode *value = parse_expr(p);
    if (p->failed)
        return NULL;
    AstNode *n = ast_new(p->arena, AST_ASSIGN, line, col);
    n->as.assign.name = name;
    n->as.assign.value = value;
    return n;
}

/* A statement may be empty (e.g. trailing ';' before 'end'). Returns NULL with
 * !failed to signal "no statement here". */
static AstNode *parse_statement(Parser *p)
{
    if (p->failed)
        return NULL;
    if (check(p, TOK_KW_BEGIN))
        return parse_block(p);
    if (check(p, TOK_KW_WRITE))
        return parse_write(p, 0);
    if (check(p, TOK_KW_WRITELN))
        return parse_write(p, 1);
    if (check(p, TOK_IDENT))
        return parse_assignment(p);
    return NULL; /* empty statement */
}

static AstNode *parse_block(Parser *p)
{
    int line = p->cur.line, col = p->cur.col;
    if (!expect(p, TOK_KW_BEGIN, "'begin'"))
        return NULL;

    AstNode *blk = ast_new(p->arena, AST_BLOCK, line, col);
    ast_list_init(&blk->as.block.stmts);

    /* Statements separated by ';'. Empty statements are tolerated. */
    if (!check(p, TOK_KW_END)) {
        for (;;) {
            AstNode *s = parse_statement(p);
            if (p->failed)
                return NULL;
            if (s)
                ast_list_push(p->arena, &blk->as.block.stmts, s);
            if (check(p, TOK_SEMI)) {
                advance(p);
                continue;
            }
            break;
        }
    }
    if (!expect(p, TOK_KW_END, "'end'"))
        return NULL;
    return blk;
}

/* ------------------------------------------------------------------ */
/* Declarations                                                       */
/* ------------------------------------------------------------------ */
/* Parse "name { , name } : integer" (or ": boolean", B1 beads initech-f0uc)
 * into one AST_VARDECL. The trailing ';' is consumed by the caller (the
 * var-section loop). */
static AstNode *parse_one_vardecl(Parser *p)
{
    int line = p->cur.line, col = p->cur.col;
    AstNode *vd = ast_new(p->arena, AST_VARDECL, line, col);
    ast_list_init(&vd->as.vardecl.names);

    for (;;) {
        if (!check(p, TOK_IDENT)) {
            fail_at(p, p->cur.line, p->cur.col, "expected a variable name");
            return NULL;
        }
        AstNode *ref = ast_new(p->arena, AST_VARREF, p->cur.line, p->cur.col);
        ref->as.varref.name = ast_arena_strndup(p->arena, p->cur.lexeme,
                                               p->cur.length);
        ast_list_push(p->arena, &vd->as.vardecl.names, ref);
        advance(p);
        if (check(p, TOK_COMMA)) {
            advance(p);
            continue;
        }
        break;
    }
    if (!expect(p, TOK_COLON, "':'"))
        return NULL;
    if (check(p, TOK_KW_INTEGER)) {
        vd->as.vardecl.vtype = AST_TY_INTEGER;
        advance(p);
    } else if (check(p, TOK_KW_BOOLEAN)) {
        vd->as.vardecl.vtype = AST_TY_BOOLEAN;
        advance(p);
    } else {
        fail_at(p, p->cur.line, p->cur.col,
                "expected 'integer' or 'boolean'");
        return NULL;
    }
    return vd;
}

/* Parse an optional var-section, appending each decl group to `decls`. */
static void parse_var_section(Parser *p, AstList *decls)
{
    if (!check(p, TOK_KW_VAR))
        return;
    advance(p); /* 'var' */

    /* One or more "names : integer ;" groups. The first is mandatory after
     * 'var'; subsequent groups continue while an identifier follows. */
    for (;;) {
        AstNode *vd = parse_one_vardecl(p);
        if (p->failed)
            return;
        ast_list_push(p->arena, decls, vd);
        if (!expect(p, TOK_SEMI, "';'"))
            return;
        if (check(p, TOK_IDENT))
            continue; /* another decl group in the same var-section */
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Program                                                            */
/* ------------------------------------------------------------------ */
static AstNode *parse_program_root(Parser *p)
{
    int line = p->cur.line, col = p->cur.col;
    if (!expect(p, TOK_KW_PROGRAM, "'program'"))
        return NULL;
    if (!check(p, TOK_IDENT)) {
        fail_at(p, p->cur.line, p->cur.col, "expected program name");
        return NULL;
    }
    char *name = ast_arena_strndup(p->arena, p->cur.lexeme, p->cur.length);
    advance(p);
    if (!expect(p, TOK_SEMI, "';'"))
        return NULL;

    AstNode *prog = ast_new(p->arena, AST_PROGRAM, line, col);
    prog->as.program.name = name;
    ast_list_init(&prog->as.program.decls);

    parse_var_section(p, &prog->as.program.decls);
    if (p->failed)
        return NULL;

    prog->as.program.block = parse_block(p);
    if (p->failed)
        return NULL;

    if (!expect(p, TOK_DOT, "'.'"))
        return NULL;
    if (!check(p, TOK_EOF)) {
        fail_at(p, p->cur.line, p->cur.col, "trailing tokens after 'end.'");
        return NULL;
    }
    return prog;
}

/* ------------------------------------------------------------------ */
/* Public entry                                                       */
/* ------------------------------------------------------------------ */
int parse_program(const char *src, size_t len, AstArena *arena,
                  ParseResult *out)
{
    Parser p;
    lexer_init(&p.lx, src, len);
    p.arena = arena;
    p.failed = 0;
    p.errmsg[0] = '\0';
    p.errline = 0;
    p.errcol = 0;
    advance(&p); /* prime lookahead */

    AstNode *root = parse_program_root(&p);

    if (p.failed) {
        out->ok = 0;
        out->ast = NULL;
        snprintf(out->error, sizeof(out->error), "%s", p.errmsg);
        out->line = p.errline;
        out->col = p.errcol;
        return 1;
    }
    out->ok = 1;
    out->ast = root;
    out->error[0] = '\0';
    out->line = 0;
    out->col = 0;
    return 0;
}
