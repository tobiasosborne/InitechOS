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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* B3 (beads initech-7mo3): the parser's own const table.              */
/* ------------------------------------------------------------------ */
/* FOLDING (front-end, no runtime storage -- see ast.h's B3 comment and
 * parse_const_section below). Every "const NAME = <literal>;" registers one
 * entry here (case-insensitive name, declared type, raw ordinal value);
 * parse_factor's TOK_IDENT branch consults this table BEFORE ever building
 * an AST_VARREF, so a const reference becomes a fresh literal node at each
 * use site and never reaches codegen as a variable read. Small fixed-
 * capacity array (consistent with typecheck.c's own TC_MAX_SYMBOLS choice
 * for the same "no hashing, insertion-ordered, fail-loud on overflow"
 * reasons -- Rule 2, and ADR-0007 DEC-04's determinism discipline even
 * though DEC-04 itself binds the resident compiler, not the seed). */
#define PARSER_MAX_CONSTS 256
#define PARSER_CONST_NAME_CAP 128
/* B4 (beads initech-63ce): while a procedure/function BODY is being parsed,
 * its parameter and local names SHADOW any same-named program-level const so
 * that a reference resolves to the parameter/local, not the folded const
 * literal (const folding is a PARSE-TIME rewrite -- see parse_factor -- so
 * shadowing must be known at parse time, unlike var shadowing which is
 * resolved later by name-lookup in typecheck/codegen). The shadow set is a
 * simple save/restore stack over this array (see parse_proc_or_func). */
#define PARSER_MAX_SHADOW 128

typedef struct {
    char       name_lc[PARSER_CONST_NAME_CAP]; /* case-folded (lower) name */
    AstVarType ctype;
    long       value; /* raw ordinal: the int value, or 0/1 (boolean), or
                        * 0..255 (char) -- interpreted per ctype when
                        * folding at a use site. */
} ParserConst;

/* Case-fold a SPAN (not necessarily NUL-terminated -- a raw TOK_IDENT lexeme
 * is a span into the source buffer, see token.h) into a NUL-terminated
 * lower-case buffer, truncated at cap-1 bytes. Mirrors typecheck.c's
 * lower_copy, but takes an explicit length instead of assuming a
 * NUL-terminated C string, since a use-site identifier hasn't been
 * arena-strndup'd yet at the point parse_factor needs to check the const
 * table (see the TOK_IDENT branch below). */
static void lower_span(char *dst, size_t cap, const char *src, size_t n)
{
    size_t i = 0;
    for (; i < n && i + 1 < cap; i++)
        dst[i] = (char)tolower((unsigned char)src[i]);
    dst[i] = '\0';
}

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
    /* B3 (beads initech-7mo3): the const table (see above). */
    ParserConst consts[PARSER_MAX_CONSTS];
    int         nconsts;
    /* B4 (beads initech-63ce): the const-shadow set (see PARSER_MAX_SHADOW).
     * Names (case-folded) of the params/locals of the proc/func body
     * currently being parsed; parse_factor skips const folding for any name
     * in this set. Empty (nshadow==0) at program-body level. */
    char        shadow[PARSER_MAX_SHADOW][PARSER_CONST_NAME_CAP];
    int         nshadow;
    /* B2 (beads initech-80iw): the program's top-level var-decl list, set
     * once by parse_program_root before the block is parsed, so a nested
     * "for" statement (however deeply it's nested in begin/end/if/while) can
     * append a synthesized hidden limit variable -- see synth_intvar. This
     * is the ONE place decls are collected outside parse_var_section; the
     * front end still has a single flat symbol table (typecheck.c), so a
     * synthesized name colliding with a user identifier surfaces as
     * typecheck's ordinary "duplicate variable declaration" fail-loud error,
     * never a silent misresolution -- the same accepted-risk shape already
     * documented for lower_copy's truncation in typecheck.c. */
    AstList  *top_decls;
    /* ONE threaded counter for synthesized for-loop limit variable names,
     * incremented in source order as each "for ... to/downto ..." is
     * parsed -- deterministic (Rule 11), never re-derived by a second walk
     * (the DEC-04 "one shared counter" mechanism, applied here at parse
     * time to synthesized names the same way codegen.c applies it to
     * emitted labels). */
    int       synth_count;
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

/* Look up a case-folded name in the const table; -1 if not a const. */
static int const_find(const Parser *p, const char *name_lc)
{
    for (int i = 0; i < p->nconsts; i++)
        if (strcmp(p->consts[i].name_lc, name_lc) == 0)
            return i;
    return -1;
}

/* B4 (beads initech-63ce): is this case-folded name a param/local of the
 * proc/func body currently being parsed (and therefore shadowing any const)? */
static int shadow_find(const Parser *p, const char *name_lc)
{
    for (int i = 0; i < p->nshadow; i++)
        if (strcmp(p->shadow[i], name_lc) == 0)
            return 1;
    return 0;
}

/* Push one case-folded name onto the shadow set (Rule 2: fail loud on
 * overflow rather than silently un-shadowing a name). */
static void shadow_push(Parser *p, int line, int col, const char *name_lc)
{
    if (p->nshadow >= PARSER_MAX_SHADOW) {
        fail_at(p, line, col,
                "too many parameters/locals in one routine "
                "(PARSER_MAX_SHADOW exceeded)");
        return;
    }
    snprintf(p->shadow[p->nshadow], PARSER_CONST_NAME_CAP, "%s", name_lc);
    p->nshadow++;
}

/* Register one const declaration (B3, beads initech-7mo3). Overflow is a
 * located, fail-loud diagnostic (Rule 2), never silent truncation -- see
 * parse_const_section's header comment on why a DUPLICATE name is
 * deliberately NOT rejected here (typecheck.c's collect_decls is the single
 * authority for the one-declaration rule, across const AND var names). */
static void const_register(Parser *p, int line, int col,
                            const char *name_lc, AstVarType ctype, long value)
{
    if (p->nconsts >= PARSER_MAX_CONSTS) {
        fail_at(p, line, col,
                "too many const declarations (PARSER_MAX_CONSTS exceeded)");
        return;
    }
    snprintf(p->consts[p->nconsts].name_lc, PARSER_CONST_NAME_CAP,
             "%s", name_lc);
    p->consts[p->nconsts].ctype = ctype;
    p->consts[p->nconsts].value = value;
    p->nconsts++;
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
/* B2 (beads initech-80iw): if/while are primitive; for/repeat are sugar,
 * desugared entirely inside their own parse_* functions (see below). */
static AstNode *parse_if(Parser *p);
static AstNode *parse_while(Parser *p);
static AstNode *parse_for(Parser *p);
static AstNode *parse_repeat(Parser *p);
/* B4 (beads initech-63ce): calls and proc/func declarations. */
static AstNode *parse_call(Parser *p, char *name, int line, int col);
static AstNode *parse_proc_or_func(Parser *p);
static void     parse_var_section(Parser *p, AstList *decls);

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
    /*
     * B3 (beads initech-7mo3; ADR-0007 DEC-02 "char"). DISAMBIGUATION
     * DECISION (see ast.h's B3 comment for the full citation): a TOK_STRING
     * reached here (an EXPRESSION context) must be exactly one character --
     * that is what makes it a char CONSTANT rather than a string. A
     * write/writeln string argument never reaches this path (parse_write
     * special-cases TOK_STRING before calling parse_expr at all), so this
     * length check can never reject a legitimate multi-character write
     * string -- only a genuine attempt to use a string as an expression
     * value, which this subset does not support (fixed/ShortString strings
     * land at B7).
     */
    if (check(p, TOK_STRING)) {
        if (p->cur.length != 1) {
            fail_at(p, line, col,
                    "a string literal used as an expression value must be "
                    "exactly one character (a char constant, e.g. 'A'); "
                    "this subset has no string-typed expressions yet "
                    "(only write/writeln string arguments may be longer)");
            return NULL;
        }
        AstNode *n = ast_new(p->arena, AST_CHARLIT, line, col);
        n->as.charlit.value = (unsigned char)p->cur.lexeme[0];
        advance(p);
        return n;
    }
    /* B3: ord(expr) / chr(expr), reserved keywords, each parsing a single
     * parenthesized argument -- see token.h's B3 note on why these are
     * keywords rather than ordinary calls (no function-call syntax yet). */
    if (check(p, TOK_KW_ORD) || check(p, TOK_KW_CHR)) {
        AstOp op = check(p, TOK_KW_ORD) ? OP_ORD : OP_CHR;
        const char *what = (op == OP_ORD) ? "ord" : "chr";
        advance(p);
        char msg[64];
        snprintf(msg, sizeof(msg), "'(' after '%s'", what);
        if (!expect(p, TOK_LPAREN, msg))
            return NULL;
        AstNode *operand = parse_expr(p);
        if (p->failed)
            return NULL;
        if (!expect(p, TOK_RPAREN, "')'"))
            return NULL;
        AstNode *n = ast_new(p->arena, AST_UNOP, line, col);
        n->as.unop.op = op;
        n->as.unop.operand = operand;
        return n;
    }
    if (check(p, TOK_INT)) {
        AstNode *n = ast_new(p->arena, AST_INTLIT, line, col);
        n->as.intlit.value = p->cur.ivalue;
        advance(p);
        return n;
    }
    if (check(p, TOK_IDENT)) {
        /* B3: a const-table hit folds directly to a fresh literal node at
         * THIS use site (no runtime storage) -- see this file's ParserConst
         * comment and ast.h's B3 note. Only reached if the identifier is
         * NOT a const; ordinary variables fall through to AST_VARREF
         * exactly as before B3. B4 (beads initech-63ce): a name that is a
         * param/local of the routine body currently being parsed SHADOWS a
         * same-named const and must NOT fold (see shadow_find). */
        char lc[PARSER_CONST_NAME_CAP];
        lower_span(lc, sizeof(lc), p->cur.lexeme, p->cur.length);
        int cidx = const_find(p, lc);
        if (cidx >= 0 && !shadow_find(p, lc)) {
            AstNode *n;
            switch (p->consts[cidx].ctype) {
            case AST_TY_INTEGER:
                n = ast_new(p->arena, AST_INTLIT, line, col);
                n->as.intlit.value = p->consts[cidx].value;
                break;
            case AST_TY_CHAR:
                n = ast_new(p->arena, AST_CHARLIT, line, col);
                n->as.charlit.value = (int)p->consts[cidx].value;
                break;
            case AST_TY_BOOLEAN:
                n = ast_new(p->arena, AST_BOOLLIT, line, col);
                n->as.boollit.value = (int)p->consts[cidx].value;
                break;
            default:
                fail_at(p, line, col,
                        "internal: const table entry has an invalid type");
                return NULL;
            }
            advance(p);
            return n;
        }
        /* B4 (beads initech-63ce): an identifier followed by '(' is a
         * FUNCTION CALL (calls always use parens in this subset -- see
         * ast.h's B4 DECISION note); otherwise it is an ordinary variable
         * (or function-result) reference. */
        char *name = ast_arena_strndup(p->arena, p->cur.lexeme, p->cur.length);
        advance(p);
        if (check(p, TOK_LPAREN))
            return parse_call(p, name, line, col);
        AstNode *n = ast_new(p->arena, AST_VARREF, line, col);
        n->as.varref.name = name;
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

/*
 * A statement that begins with an identifier is EITHER an assignment
 * (`name := expr`) OR a procedure call (`name(args)` -- B4, beads
 * initech-63ce). We read the identifier, then disambiguate on the next
 * token: '(' -> a call statement (routed to parse_call, which builds the
 * same AST_CALL node a function call in an expression would); otherwise a
 * ':=' assignment. Assignment to a function name inside its own body writes
 * the function RESULT (the target resolves to the result variable in
 * typecheck/codegen -- see ast.h's B4 note).
 */
static AstNode *parse_assignment(Parser *p)
{
    int line = p->cur.line, col = p->cur.col;
    char *name = ast_arena_strndup(p->arena, p->cur.lexeme, p->cur.length);
    advance(p); /* ident */
    if (check(p, TOK_LPAREN))
        return parse_call(p, name, line, col); /* procedure call statement */
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

/*
 * call = ident "(" [ expr { "," expr } ] ")" ;   (B4, beads initech-63ce)
 *
 * `name` is already arena-owned and the current token is the '('. Builds an
 * AST_CALL whether the call is a function call in an expression or a
 * procedure call as a statement -- typecheck decides which is legal from the
 * callee's signature (a function must be called in an expression; a
 * procedure only as a statement -- ast.h's B4 note). Whether each argument
 * is passed by value or by address is likewise a callee-signature decision
 * made in typecheck/codegen, NOT here (the argument is always parsed as an
 * ordinary expression; a `var` parameter additionally requires its argument
 * to be a plain variable, enforced in typecheck.c).
 */
static AstNode *parse_call(Parser *p, char *name, int line, int col)
{
    advance(p); /* '(' */
    AstNode *n = ast_new(p->arena, AST_CALL, line, col);
    n->as.call.name = name;
    ast_list_init(&n->as.call.args);
    if (!check(p, TOK_RPAREN)) {
        for (;;) {
            AstNode *arg = parse_expr(p);
            if (p->failed)
                return NULL;
            ast_list_push(p->arena, &n->as.call.args, arg);
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

/* ------------------------------------------------------------------ */
/* B2 (beads initech-80iw; ADR-0007 DEC-02) control flow               */
/* ------------------------------------------------------------------ */
/* Small AST-node constructor helpers used both directly (if/while) and by
 * the for/repeat DESUGARING below -- building an AST_ASSIGN/AST_VARREF/
 * AST_BINOP by hand is exactly what parse_assignment/parse_factor/parse_term
 * already do inline; factoring it here keeps the desugar functions readable
 * without duplicating ast_new boilerplate at every call site. */
static AstNode *mk_varref(Parser *p, char *name, int line, int col)
{
    AstNode *n = ast_new(p->arena, AST_VARREF, line, col);
    n->as.varref.name = name; /* already arena-owned (strndup'd or synth'd) */
    return n;
}

static AstNode *mk_assign(Parser *p, char *name, AstNode *value,
                          int line, int col)
{
    AstNode *n = ast_new(p->arena, AST_ASSIGN, line, col);
    n->as.assign.name = name;
    n->as.assign.value = value;
    return n;
}

static AstNode *mk_binop(Parser *p, AstOp op, AstNode *lhs, AstNode *rhs,
                         int line, int col)
{
    AstNode *n = ast_new(p->arena, AST_BINOP, line, col);
    n->as.binop.op = op;
    n->as.binop.lhs = lhs;
    n->as.binop.rhs = rhs;
    return n;
}

static AstNode *mk_intlit(Parser *p, long value, int line, int col)
{
    AstNode *n = ast_new(p->arena, AST_INTLIT, line, col);
    n->as.intlit.value = value;
    return n;
}

/* Synthesize a fresh, never-user-writable-looking integer variable name
 * ("__forlim_<N>"), append its AST_VARDECL to the program's top-level decls
 * (p->top_decls, wired up in parse_program_root), and return the arena-owned
 * name string. Used ONLY by the "for" desugar (below) to hold the
 * once-evaluated loop limit. ONE threaded counter (p->synth_count), never
 * re-derived -- DEC-04's mechanism, applied at parse time (see the Parser
 * struct comment). */
static char *synth_intvar(Parser *p, int line, int col)
{
    char namebuf[32];
    p->synth_count++;
    snprintf(namebuf, sizeof(namebuf), "__forlim_%d", p->synth_count);
    char *name = ast_arena_strndup(p->arena, namebuf, strlen(namebuf));

    AstNode *ref = ast_new(p->arena, AST_VARREF, line, col);
    ref->as.varref.name = name;

    AstNode *vd = ast_new(p->arena, AST_VARDECL, line, col);
    ast_list_init(&vd->as.vardecl.names);
    ast_list_push(p->arena, &vd->as.vardecl.names, ref);
    vd->as.vardecl.vtype = AST_TY_INTEGER;

    ast_list_push(p->arena, p->top_decls, vd);
    return name;
}

/*
 * if-stmt = "if" expr "then" stmt [ "else" stmt ] ;
 *
 * Dangling-else binds to the NEAREST unmatched "if": after parsing its own
 * then-branch, THIS call checks for a trailing "else" immediately, before
 * returning control to whatever enclosing if-parse (if any) is waiting on
 * its own else-check. That's the entire mechanism -- no special-casing
 * needed, it falls out of plain recursive descent (ISO 7185 / Turbo Pascal
 * canonical resolution; see parser.h's grammar comment).
 */
static AstNode *parse_if(Parser *p)
{
    int line = p->cur.line, col = p->cur.col;
    advance(p); /* 'if' */

    AstNode *cond = parse_expr(p);
    if (p->failed)
        return NULL;
    if (!expect(p, TOK_KW_THEN, "'then'"))
        return NULL;

    AstNode *then_stmt = parse_statement(p);
    if (p->failed)
        return NULL;

    AstNode *else_stmt = NULL;
    if (check(p, TOK_KW_ELSE)) {
        advance(p);
        else_stmt = parse_statement(p);
        if (p->failed)
            return NULL;
    }

    AstNode *n = ast_new(p->arena, AST_IF, line, col);
    n->as.ifstmt.cond = cond;
    n->as.ifstmt.then_stmt = then_stmt;
    n->as.ifstmt.else_stmt = else_stmt;
    return n;
}

/* while-stmt = "while" expr "do" stmt ; */
static AstNode *parse_while(Parser *p)
{
    int line = p->cur.line, col = p->cur.col;
    advance(p); /* 'while' */

    AstNode *cond = parse_expr(p);
    if (p->failed)
        return NULL;
    if (!expect(p, TOK_KW_DO, "'do'"))
        return NULL;

    AstNode *body = parse_statement(p);
    if (p->failed)
        return NULL;

    AstNode *n = ast_new(p->arena, AST_WHILE, line, col);
    n->as.whilestmt.cond = cond;
    n->as.whilestmt.body = body;
    return n;
}

/*
 * for-stmt = "for" ident ":=" expr ("to"|"downto") expr "do" stmt ;
 *
 * SUGAR (ADR-0007 DEC-02): desugars entirely to primitive nodes, here, at
 * parse time -- codegen never sees a "for". `ident` must already be a
 * declared integer variable (checked by ordinary AST_ASSIGN typechecking on
 * the desugared "ident := e1", not by any for-specific rule).
 *
 * Both bound expressions are evaluated EXACTLY ONCE, at loop entry (ISO 7185
 * Sec 6.8.3.9 / Turbo Pascal Language Guide, "for statement": the initial-
 * and final-value expressions are evaluated once, before the loop's first
 * iteration) -- so e2 is assigned into a synthesized hidden limit variable
 * ONCE, and the while-guard reads that variable, never re-evaluating e2, so
 * a loop body that reassigns a variable e2 referenced cannot change the
 * bound mid-loop (see control.pas's FORONCE fixture tag).
 *
 * for v := e1 to e2 do S      desugars to:
 *   begin
 *     v := e1;
 *     __forlim_N := e2;
 *     while v <= __forlim_N do begin S; v := v + 1 end
 *   end
 * ("downto" swaps to ">=" and "v - 1"). The loop-control variable's value
 * once the loop terminates NORMALLY is left FORMALLY UNDEFINED by both ISO
 * 7185 and Turbo Pascal -- no fixture may depend on it (see parser.h).
 */
static AstNode *parse_for(Parser *p)
{
    int line = p->cur.line, col = p->cur.col;
    advance(p); /* 'for' */

    if (!check(p, TOK_IDENT)) {
        fail_at(p, p->cur.line, p->cur.col,
                "expected a variable name after 'for'");
        return NULL;
    }
    char *varname = ast_arena_strndup(p->arena, p->cur.lexeme, p->cur.length);
    advance(p);

    if (!expect(p, TOK_ASSIGN, "':='"))
        return NULL;
    AstNode *e1 = parse_expr(p);
    if (p->failed)
        return NULL;

    int is_downto;
    if (check(p, TOK_KW_TO)) {
        is_downto = 0;
        advance(p);
    } else if (check(p, TOK_KW_DOWNTO)) {
        is_downto = 1;
        advance(p);
    } else {
        fail_at(p, p->cur.line, p->cur.col, "expected 'to' or 'downto'");
        return NULL;
    }

    AstNode *e2 = parse_expr(p);
    if (p->failed)
        return NULL;
    if (!expect(p, TOK_KW_DO, "'do'"))
        return NULL;

    AstNode *body = parse_statement(p);
    if (p->failed)
        return NULL;

    char *limit = synth_intvar(p, line, col);

    AstNode *init_v = mk_assign(p, varname, e1, line, col);
    AstNode *init_lim = mk_assign(p, limit, e2, line, col);

    AstNode *guard = mk_binop(p, is_downto ? OP_GE : OP_LE,
                              mk_varref(p, varname, line, col),
                              mk_varref(p, limit, line, col), line, col);

    AstNode *step = mk_assign(p, varname,
                              mk_binop(p, is_downto ? OP_SUB : OP_ADD,
                                       mk_varref(p, varname, line, col),
                                       mk_intlit(p, 1, line, col),
                                       line, col),
                              line, col);

    AstNode *loop_body = ast_new(p->arena, AST_BLOCK, line, col);
    ast_list_init(&loop_body->as.block.stmts);
    if (body)
        ast_list_push(p->arena, &loop_body->as.block.stmts, body);
    ast_list_push(p->arena, &loop_body->as.block.stmts, step);

    AstNode *wh = ast_new(p->arena, AST_WHILE, line, col);
    wh->as.whilestmt.cond = guard;
    wh->as.whilestmt.body = loop_body;

    AstNode *outer = ast_new(p->arena, AST_BLOCK, line, col);
    ast_list_init(&outer->as.block.stmts);
    ast_list_push(p->arena, &outer->as.block.stmts, init_v);
    ast_list_push(p->arena, &outer->as.block.stmts, init_lim);
    ast_list_push(p->arena, &outer->as.block.stmts, wh);
    return outer;
}

/*
 * repeat-stmt = "repeat" stmt-seq "until" expr ;
 * stmt-seq    = stmt { ";" stmt } ;   -- NOTE: no begin/end wrapper needed;
 *                                        the terminator is 'until', not 'end'.
 *
 * SUGAR (ADR-0007 DEC-02): the body runs AT LEAST once (repeat-until tests
 * the guard AFTER the body, unlike while-do which tests BEFORE). Desugars to
 * "the body block, once, followed by a while-not-guard loop over the SAME
 * body block" -- the body AstNode is intentionally shared (referenced from
 * two positions) rather than duplicated: codegen/typecheck are pure,
 * read-only tree walks (no per-node "visited" state, no mutation beyond the
 * idempotent expression `type` annotation), so walking the same subtree from
 * two call sites is equivalent to, and cheaper than, cloning it, and is
 * still fully deterministic (Rule 11) -- both walks visit it in the same
 * fixed, source-derived order every time.
 *
 * repeat S1; S2 until B      desugars to:
 *   begin
 *     begin S1; S2 end;                { runs once, unconditionally }
 *     while not B do begin S1; S2 end  { same body block, shared }
 *   end
 */
static AstNode *parse_repeat(Parser *p)
{
    int line = p->cur.line, col = p->cur.col;
    advance(p); /* 'repeat' */

    AstNode *body = ast_new(p->arena, AST_BLOCK, line, col);
    ast_list_init(&body->as.block.stmts);
    if (!check(p, TOK_KW_UNTIL)) {
        for (;;) {
            AstNode *s = parse_statement(p);
            if (p->failed)
                return NULL;
            if (s)
                ast_list_push(p->arena, &body->as.block.stmts, s);
            if (check(p, TOK_SEMI)) {
                advance(p);
                continue;
            }
            break;
        }
    }
    if (!expect(p, TOK_KW_UNTIL, "'until'"))
        return NULL;

    AstNode *guard = parse_expr(p);
    if (p->failed)
        return NULL;

    AstNode *not_guard = ast_new(p->arena, AST_UNOP, line, col);
    not_guard->as.unop.op = OP_NOT;
    not_guard->as.unop.operand = guard;

    AstNode *wh = ast_new(p->arena, AST_WHILE, line, col);
    wh->as.whilestmt.cond = not_guard;
    wh->as.whilestmt.body = body; /* shared -- see header comment above */

    AstNode *outer = ast_new(p->arena, AST_BLOCK, line, col);
    ast_list_init(&outer->as.block.stmts);
    ast_list_push(p->arena, &outer->as.block.stmts, body); /* run once */
    ast_list_push(p->arena, &outer->as.block.stmts, wh);
    return outer;
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
    if (check(p, TOK_KW_IF))
        return parse_if(p);
    if (check(p, TOK_KW_WHILE))
        return parse_while(p);
    if (check(p, TOK_KW_FOR))
        return parse_for(p);
    if (check(p, TOK_KW_REPEAT))
        return parse_repeat(p);
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
    } else if (check(p, TOK_KW_CHAR)) {
        /* B3 (beads initech-7mo3; ADR-0007 DEC-02 "char"). */
        vd->as.vardecl.vtype = AST_TY_CHAR;
        advance(p);
    } else {
        fail_at(p, p->cur.line, p->cur.col,
                "expected 'integer', 'boolean', or 'char'");
        return NULL;
    }
    return vd;
}

/*
 * B3 (beads initech-7mo3; ADR-0007 DEC-02 "const declarations"):
 *
 *   const-section = "const" const-decl ";" { const-decl ";" } ;
 *   const-decl    = ident "=" const-literal ;
 *   const-literal = [ "-" ] integer | char-literal | "true" | "false" ;
 *
 * DECISION (report): const-decls use a bare "=" (ISO 7185 Sec 6.3 / Turbo
 * Pascal Language Guide "Constant declarations": "identifier = constant"),
 * never ":=" -- there is no lexical ambiguity (':=' is scanned as one token
 * starting with ':', exactly like the B1 note on '=' vs ':=' for relational
 * ops). Only LITERAL values are accepted (no const-referencing-const, no
 * general constant-expression evaluator) -- the ADR's subset table asks
 * only for "const declarations"; typed consts (Turbo Pascal's
 * `const X : T = value`, which is really a pre-initialized variable) are
 * explicitly OUT of scope per the bead.
 *
 * FOLDING (front-end, no runtime storage -- ast.h's B3 comment has the full
 * story): each decl is registered in the parser's OWN const table
 * (const_register) as it is parsed; every later identifier reference that
 * matches folds to a fresh literal node at that use site (parse_factor's
 * TOK_IDENT branch) instead of ever becoming an AST_VARREF. An
 * AST_CONSTDECL (name + type only, no value) is still appended to `decls`
 * so typecheck.c can enforce the same case-insensitive one-declaration rule
 * across const AND var names.
 *
 * DUPLICATE NAMES: this parser-level table does not itself reject a
 * duplicate const name -- see const_register's comment; typecheck.c's
 * collect_decls is the single authority that rejects it (a "duplicate
 * variable declaration" error), so a duplicate can never silently produce a
 * working binary.
 */
static void parse_const_section(Parser *p, AstList *decls)
{
    if (!check(p, TOK_KW_CONST))
        return;
    advance(p); /* 'const' */

    for (;;) {
        if (!check(p, TOK_IDENT)) {
            fail_at(p, p->cur.line, p->cur.col,
                    "expected a constant name after 'const'");
            return;
        }
        int line = p->cur.line, col = p->cur.col;
        char *name = ast_arena_strndup(p->arena, p->cur.lexeme, p->cur.length);
        advance(p);

        if (!expect(p, TOK_EQ, "'=' in const declaration"))
            return;

        AstVarType ctype;
        long value;
        if (check(p, TOK_MINUS)) {
            /* Only integers may be negated -- a negative char/boolean
             * constant is not meaningful in this subset. */
            advance(p);
            if (!check(p, TOK_INT)) {
                fail_at(p, p->cur.line, p->cur.col,
                        "expected an integer after unary '-' in a const "
                        "declaration");
                return;
            }
            value = -(long)p->cur.ivalue;
            ctype = AST_TY_INTEGER;
            advance(p);
        } else if (check(p, TOK_INT)) {
            value = p->cur.ivalue;
            ctype = AST_TY_INTEGER;
            advance(p);
        } else if (check(p, TOK_KW_TRUE)) {
            value = 1;
            ctype = AST_TY_BOOLEAN;
            advance(p);
        } else if (check(p, TOK_KW_FALSE)) {
            value = 0;
            ctype = AST_TY_BOOLEAN;
            advance(p);
        } else if (check(p, TOK_STRING)) {
            /* Same one-character disambiguation rule as parse_factor's
             * expression-context char literal -- see ast.h's B3 comment. */
            if (p->cur.length != 1) {
                fail_at(p, p->cur.line, p->cur.col,
                        "a const char value must be exactly one character "
                        "(e.g. 'A'); this subset has no string-typed "
                        "constants yet");
                return;
            }
            value = (unsigned char)p->cur.lexeme[0];
            ctype = AST_TY_CHAR;
            advance(p);
        } else {
            fail_at(p, p->cur.line, p->cur.col,
                    "expected an integer, char, or boolean literal after "
                    "'=' (typed consts are not in this subset)");
            return;
        }

        char lc[PARSER_CONST_NAME_CAP];
        lower_span(lc, sizeof(lc), name, strlen(name));
        const_register(p, line, col, lc, ctype, value);
        if (p->failed)
            return;

        AstNode *cd = ast_new(p->arena, AST_CONSTDECL, line, col);
        cd->as.constdecl.name = name;
        cd->as.constdecl.ctype = ctype;
        ast_list_push(p->arena, decls, cd);

        if (!expect(p, TOK_SEMI, "';'"))
            return;
        if (check(p, TOK_IDENT))
            continue; /* another const-decl in the same section */
        break;
    }
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
/* B4 (beads initech-63ce): procedures / functions / calls             */
/* ------------------------------------------------------------------ */
/* Parse one scalar type keyword (integer / boolean / char). Shared by the
 * parameter list and the function result type. Returns 1 on success (and
 * consumes the keyword), 0 with a located error on anything else. */
static int parse_type_kw(Parser *p, AstVarType *out)
{
    if (check(p, TOK_KW_INTEGER)) { *out = AST_TY_INTEGER; advance(p); return 1; }
    if (check(p, TOK_KW_BOOLEAN)) { *out = AST_TY_BOOLEAN; advance(p); return 1; }
    if (check(p, TOK_KW_CHAR))    { *out = AST_TY_CHAR;    advance(p); return 1; }
    fail_at(p, p->cur.line, p->cur.col,
            "expected 'integer', 'boolean', or 'char'");
    return 0;
}

/*
 * param-list  = "(" [ param-group { ";" param-group } ] ")" ;
 * param-group = [ "var" ] ident { "," ident } ":" type ;
 *
 * Each name in a group becomes ONE AST_PARAM (the group is FLATTENED) so
 * every parameter owns a deterministic frame offset [ebp+8+4i] in codegen.
 * A `var` prefix marks the whole group by-reference. The current token is
 * the '(' on entry.
 */
static void parse_param_list(Parser *p, AstList *params)
{
    advance(p); /* '(' */
    if (check(p, TOK_RPAREN)) { /* an explicit empty () -- accepted */
        advance(p);
        return;
    }
    for (;;) {
        int is_var = 0;
        if (check(p, TOK_KW_VAR)) {
            is_var = 1;
            advance(p);
        }
        size_t group_start = params->count;
        for (;;) {
            if (!check(p, TOK_IDENT)) {
                fail_at(p, p->cur.line, p->cur.col,
                        "expected a parameter name");
                return;
            }
            AstNode *pn = ast_new(p->arena, AST_PARAM, p->cur.line, p->cur.col);
            pn->as.param.name = ast_arena_strndup(p->arena, p->cur.lexeme,
                                                  p->cur.length);
            pn->as.param.is_var = is_var;
            pn->as.param.ptype = AST_TY_UNKNOWN; /* backfilled after ':' */
            ast_list_push(p->arena, params, pn);
            advance(p);
            if (check(p, TOK_COMMA)) {
                advance(p);
                continue;
            }
            break;
        }
        if (!expect(p, TOK_COLON, "':' in a parameter group"))
            return;
        AstVarType t;
        if (!parse_type_kw(p, &t))
            return;
        for (size_t i = group_start; i < params->count; i++)
            params->items[i]->as.param.ptype = t;
        if (check(p, TOK_SEMI)) { /* another parameter group */
            advance(p);
            continue;
        }
        break;
    }
    expect(p, TOK_RPAREN, "')' after the parameter list");
}

/*
 * proc-or-func = ("procedure" ident [param-list] ";"
 *               | "function"  ident [param-list] ":" type ";")
 *                ( "forward" ";" | [var-section] block ";" ) ;
 *
 * TOP-LEVEL (flat) only -- see ast.h's B4 DECISION note: a routine has its
 * own scope over its params + locals (which may shadow a global) plus the
 * program globals; lexically-nested procedure declarations with uplevel
 * addressing are NOT in this subset.
 *
 * `forward` (ADR-0007 DEC-02 ratification amendment): a forward declaration
 * carries the full signature and no body; the defining occurrence later must
 * REPEAT an identical signature (validated in typecheck.c -- this is the
 * FPC-compatible form; the TP shorthand of omitting the repeated header is
 * not in this subset). Enables mutual recursion.
 *
 * SCOPE DECISION (report): while the body is parsed, the routine's params and
 * locals are pushed on the const-shadow set (parse_factor won't fold a
 * same-named const to a literal), and p->top_decls points at the routine's
 * OWN decls so a `for`-loop's synthesized limit variable lands as a LOCAL
 * (frame-resident, hence re-entrant under recursion), not a program global.
 * LOCAL `const` sections are DEFERRED in this subset (only local `var`); a
 * global const remains usable inside a routine body.
 */
static AstNode *parse_proc_or_func(Parser *p)
{
    int line = p->cur.line, col = p->cur.col;
    int is_func = check(p, TOK_KW_FUNCTION);
    advance(p); /* 'procedure' | 'function' */

    if (!check(p, TOK_IDENT)) {
        fail_at(p, p->cur.line, p->cur.col,
                "expected a procedure/function name");
        return NULL;
    }
    char *name = ast_arena_strndup(p->arena, p->cur.lexeme, p->cur.length);
    advance(p);

    AstNode *pf = ast_new(p->arena, is_func ? AST_FUNCDECL : AST_PROCDECL,
                          line, col);
    pf->as.procfunc.name = name;
    pf->as.procfunc.has_result = is_func;
    pf->as.procfunc.rettype = AST_TY_UNKNOWN;
    pf->as.procfunc.body = NULL;
    pf->as.procfunc.is_forward = 0;
    ast_list_init(&pf->as.procfunc.params);
    ast_list_init(&pf->as.procfunc.decls);

    if (check(p, TOK_LPAREN))
        parse_param_list(p, &pf->as.procfunc.params);
    if (p->failed)
        return NULL;

    if (is_func) {
        if (!expect(p, TOK_COLON, "':' before the function result type"))
            return NULL;
        AstVarType rt;
        if (!parse_type_kw(p, &rt))
            return NULL;
        pf->as.procfunc.rettype = rt;
    }

    if (!expect(p, TOK_SEMI, "';' after the routine header"))
        return NULL;

    if (check(p, TOK_KW_FORWARD)) {
        advance(p);
        pf->as.procfunc.is_forward = 1;
        if (!expect(p, TOK_SEMI, "';' after 'forward'"))
            return NULL;
        return pf;
    }

    /* Defining occurrence. Enter the routine's parse-time scope. */
    int saved_shadow = p->nshadow;
    AstList *saved_top = p->top_decls;
    p->top_decls = &pf->as.procfunc.decls;

    for (size_t i = 0; i < pf->as.procfunc.params.count; i++) {
        AstNode *pn = pf->as.procfunc.params.items[i];
        char plc[PARSER_CONST_NAME_CAP];
        lower_span(plc, sizeof(plc), pn->as.param.name,
                   strlen(pn->as.param.name));
        shadow_push(p, pn->line, pn->col, plc);
    }
    if (is_func) {
        /* the function name denotes a writable RESULT variable in its body */
        char flc[PARSER_CONST_NAME_CAP];
        lower_span(flc, sizeof(flc), name, strlen(name));
        shadow_push(p, line, col, flc);
    }

    /* Local declarations (var only; const deferred). */
    parse_var_section(p, &pf->as.procfunc.decls);
    if (p->failed) {
        p->nshadow = saved_shadow;
        p->top_decls = saved_top;
        return NULL;
    }
    for (size_t i = 0; i < pf->as.procfunc.decls.count; i++) {
        AstNode *vd = pf->as.procfunc.decls.items[i];
        if (vd->kind != AST_VARDECL)
            continue;
        for (size_t j = 0; j < vd->as.vardecl.names.count; j++) {
            AstNode *vr = vd->as.vardecl.names.items[j];
            char vlc[PARSER_CONST_NAME_CAP];
            lower_span(vlc, sizeof(vlc), vr->as.varref.name,
                       strlen(vr->as.varref.name));
            shadow_push(p, vr->line, vr->col, vlc);
        }
    }
    if (p->failed) {
        p->nshadow = saved_shadow;
        p->top_decls = saved_top;
        return NULL;
    }

    pf->as.procfunc.body = parse_block(p);
    if (p->failed) {
        p->nshadow = saved_shadow;
        p->top_decls = saved_top;
        return NULL;
    }
    if (!expect(p, TOK_SEMI, "';' after the routine body")) {
        p->nshadow = saved_shadow;
        p->top_decls = saved_top;
        return NULL;
    }

    p->nshadow = saved_shadow;
    p->top_decls = saved_top;
    return pf;
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

    /* B2 (beads initech-80iw): wire the top-level decls list into the
     * parser state before anything that might parse a "for" statement
     * (parse_var_section itself never does, but the block below can, at any
     * nesting depth) -- see synth_intvar / the Parser struct comment. */
    p->top_decls = &prog->as.program.decls;

    /*
     * B3 (beads initech-7mo3; ADR-0007 DEC-02 "const declarations").
     * DECISION (report): Turbo Pascal / ISO 7185 allow const/var/type
     * sections to repeat and interleave in ANY order ("const ... ; var
     * ... ; const ... ;" is legal TP). The bead's floor requirement is only
     * "implement at least const-then-var"; this seed goes past that floor
     * and accepts REPEATED, INTERLEAVED const/var sections, because the
     * extra generality costs nothing beyond a "which section keyword is
     * next" loop (there is no type-section to support yet -- no enums/
     * records/arrays are in the B3 scope) and it matches full TP dialect
     * behaviour rather than an artificially narrower one.
     */
    for (;;) {
        if (check(p, TOK_KW_CONST)) {
            parse_const_section(p, &prog->as.program.decls);
        } else if (check(p, TOK_KW_VAR)) {
            parse_var_section(p, &prog->as.program.decls);
        } else if (check(p, TOK_KW_PROCEDURE) || check(p, TOK_KW_FUNCTION)) {
            /* B4 (beads initech-63ce): top-level procedure/function
             * declarations follow the const/var sections (TP declaration
             * order). Each is appended to program.decls in source order --
             * the same list globals live in; codegen/typecheck filter by
             * node kind (emit_bss skips them, a dedicated proc pass emits
             * them, exactly as AST_CONSTDECL is skipped at B3). */
            AstNode *pf = parse_proc_or_func(p);
            if (p->failed)
                return NULL;
            ast_list_push(p->arena, &prog->as.program.decls, pf);
        } else {
            break;
        }
        if (p->failed)
            return NULL;
    }

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
    p.top_decls = NULL;   /* set by parse_program_root before any "for" can be reached */
    p.synth_count = 0;
    p.nconsts = 0;        /* B3 (beads initech-7mo3): the const table starts empty */
    p.nshadow = 0;        /* B4 (beads initech-63ce): no active routine scope yet */
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
