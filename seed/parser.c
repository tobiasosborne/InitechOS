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
/* B6 (beads initech-rug7): the parser's own RECORD-TYPE-NAME registry. Only
 * EXISTENCE (a name -> "yes, this is a declared record type") is tracked
 * here -- enough to disambiguate an identifier as a type name wherever a
 * var-decl/param/array-element TYPE is expected (parse_type_name below).
 * Full field validation (names, per-field types, layout) is typecheck.c's
 * job (mirrors how parse_array_bound only needs "is this const registered"
 * at parse time, while the array's actual bounds-use is validated later).
 * Record type names live in their OWN namespace in this subset -- NOT
 * cross-checked against var/const/proc names (DECISION, report: a minor,
 * deliberate divergence from strict ISO Pascal's single flat identifier
 * namespace; colliding a type name with a variable name is confusing style
 * but is not a documented self-host need to reject via cross-namespace
 * collision detection, and checking it would require reordering this
 * parser's single forward pass against typecheck's separate symbol table). */
#define PARSER_MAX_RECTYPES 64
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
    /* B6 (beads initech-rug7): the record-TYPE-name registry (see above).
     * Global only -- there is no local `type` section in this subset,
     * mirroring the B3 const-section's local-declaration deferral, so this
     * is never saved/restored around a routine body the way the const-shadow
     * set is. */
    char        rectypes[PARSER_MAX_RECTYPES][PARSER_CONST_NAME_CAP];
    int         nrectypes;
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

/* B6 (beads initech-rug7): is this case-folded name a declared record type? */
static int rectype_find(const Parser *p, const char *name_lc)
{
    for (int i = 0; i < p->nrectypes; i++)
        if (strcmp(p->rectypes[i], name_lc) == 0)
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
/* B5 (beads initech-54uu): array indexing. */
static AstNode *parse_index(Parser *p, char *name, int line, int col);
/* B6 (beads initech-rug7): a `type` section of named record types, and the
 * shared scalar-or-record-name TYPE parser used by var-decls/params/array
 * element types. */
static void     parse_type_section(Parser *p, AstList *decls);
static int      parse_type_name(Parser *p, AstVarType *out, char **out_rectype);

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
        /* B5 (beads initech-54uu): `name[expr]` is an array-element r-value.
         * A const can never reach here (the const-fold branch above already
         * returned), so this is unambiguously an array reference. */
        AstNode *desig;
        if (check(p, TOK_LBRACKET))
            desig = parse_index(p, name, line, col);
        else {
            desig = ast_new(p->arena, AST_VARREF, line, col);
            desig->as.varref.name = name;
        }
        if (p->failed)
            return NULL;
        /* B6 (beads initech-rug7; ADR-0007 DEC-02 "field access"): an
         * OPTIONAL trailing ".field" makes this a FIELD access -- `name.f`
         * or `name[expr].f`. ONE level only (fields are scalar-only, no
         * record-of-record nesting -- ast.h's B6 AST_FIELD comment); a
         * second consecutive '.' surfaces as an ordinary syntax/type error
         * downstream (there is no field-of-a-field designator to build). */
        if (check(p, TOK_DOT)) {
            int fline = p->cur.line, fcol = p->cur.col;
            advance(p); /* '.' */
            if (!check(p, TOK_IDENT)) {
                fail_at(p, p->cur.line, p->cur.col,
                        "expected a field name after '.'");
                return NULL;
            }
            char *field = ast_arena_strndup(p->arena, p->cur.lexeme,
                                            p->cur.length);
            advance(p);
            AstNode *fn = ast_new(p->arena, AST_FIELD, fline, fcol);
            fn->as.field.base = desig;
            fn->as.field.field = field;
            fn->as.field.field_index = -1; /* resolved by typecheck.c */
            return fn;
        }
        return desig;
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
 *
 * B5 (beads initech-54uu): an optional "[" expr "]" between the identifier
 * and ':=' makes this an INDEXED (array-element) assignment target --
 * `name[index] := expr`. Reuses AST_ASSIGN (its `index` field is NULL for
 * every plain scalar assignment, non-NULL here) -- see ast.h's B5 comment.
 *
 * B6 (beads initech-rug7): an optional ".field" AFTER the identifier/index
 * makes this a FIELD assignment -- `name.field := expr` or
 * `name[index].field := expr` -- reusing AST_ASSIGN's new `field`/
 * `field_index` members (see ast.h's B6 comment) rather than a sibling
 * statement kind, mirroring how the B5 indexed case reused `index`. When
 * NEITHER `index` NOR `field` is present and the target resolves (at
 * typecheck) to a RECORD-typed variable, `name := expr` is a WHOLE-RECORD
 * assignment (`r1 := r2`) -- typecheck.c sets `rec_fields` in that case; the
 * parser does not need to know the target's type to build the right AST
 * shape, only whether a `.field` followed.
 */
static AstNode *parse_assignment(Parser *p)
{
    int line = p->cur.line, col = p->cur.col;
    char *name = ast_arena_strndup(p->arena, p->cur.lexeme, p->cur.length);
    advance(p); /* ident */
    if (check(p, TOK_LPAREN))
        return parse_call(p, name, line, col); /* procedure call statement */
    AstNode *index = NULL;
    if (check(p, TOK_LBRACKET)) {
        advance(p); /* '[' */
        index = parse_expr(p);
        if (p->failed)
            return NULL;
        if (!expect(p, TOK_RBRACKET, "']' after an array index"))
            return NULL;
    }
    char *field = NULL;
    if (check(p, TOK_DOT)) {
        advance(p); /* '.' */
        if (!check(p, TOK_IDENT)) {
            fail_at(p, p->cur.line, p->cur.col,
                    "expected a field name after '.'");
            return NULL;
        }
        field = ast_arena_strndup(p->arena, p->cur.lexeme, p->cur.length);
        advance(p);
    }
    if (!expect(p, TOK_ASSIGN, "':='"))
        return NULL;
    AstNode *value = parse_expr(p);
    if (p->failed)
        return NULL;
    AstNode *n = ast_new(p->arena, AST_ASSIGN, line, col);
    n->as.assign.name = name;
    n->as.assign.index = index;
    n->as.assign.field = field;
    n->as.assign.field_index = -1;
    n->as.assign.rec_fields = 0;
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

/*
 * index-expr = ident "[" expr "]" ;   (B5, beads initech-54uu)
 *
 * `name` is already arena-owned and the current token is the '['. Builds an
 * AST_INDEX used as an r-value (an ordinary expression) or as a `var`
 * call-argument (typecheck.c's check_call resolves the latter; codegen.c's
 * gen_addr_of passes the element's ADDRESS in that case). The index
 * expression may be arbitrarily complex (a nested call, another index,
 * etc.) -- typecheck enforces only that it is INTEGER-typed.
 */
static AstNode *parse_index(Parser *p, char *name, int line, int col)
{
    advance(p); /* '[' */
    AstNode *idx = parse_expr(p);
    if (p->failed)
        return NULL;
    if (!expect(p, TOK_RBRACKET, "']' after an array index"))
        return NULL;
    AstNode *n = ast_new(p->arena, AST_INDEX, line, col);
    n->as.arrayindex.name = name;
    n->as.arrayindex.index = idx;
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
/*
 * array-bound = [ "-" ] integer | ident ;   (B5, beads initech-54uu)
 *
 * A static array's bound is a PARSE-TIME CONSTANT ONLY: an (optionally
 * negated) integer literal, or the name of an already-registered `const`
 * INTEGER (parser.c's own const table -- see the ParserConst comment near
 * the top of this file). NEVER a general expression -- a static array's
 * extent must be known here, for frame-slot/.bss sizing (codegen.c). A
 * negated const reference (`-SIZE`) is not supported (report: minor, no
 * self-host need identified). Returns 1 on success (with *out set), 0 with
 * a located error otherwise.
 */
static int parse_array_bound(Parser *p, long *out)
{
    int neg = 0;
    if (check(p, TOK_MINUS)) {
        neg = 1;
        advance(p);
    }
    if (check(p, TOK_INT)) {
        long v = p->cur.ivalue;
        advance(p);
        *out = neg ? -v : v;
        return 1;
    }
    if (!neg && check(p, TOK_IDENT)) {
        char lc[PARSER_CONST_NAME_CAP];
        lower_span(lc, sizeof(lc), p->cur.lexeme, p->cur.length);
        int cidx = const_find(p, lc);
        if (cidx >= 0 && p->consts[cidx].ctype == AST_TY_INTEGER) {
            *out = p->consts[cidx].value;
            advance(p);
            return 1;
        }
    }
    fail_at(p, p->cur.line, p->cur.col,
            "expected an integer literal or an integer constant for an "
            "array bound");
    return 0;
}

/*
 * B6 (beads initech-rug7; ADR-0007 DEC-02 "records ... arrays of records").
 * type-name = "integer" | "boolean" | "char" | record-type-ident ;
 *
 * The SHARED "what type is this" parser used everywhere a var-decl,
 * parameter, or array ELEMENT type is expected: the three scalar keywords,
 * or the name of an ALREADY-DECLARED `type ... = record ... end;` (this
 * parser's own rectypes registry -- see its comment near PARSER_MAX_RECTYPES
 * above). A record type name is therefore usable only AFTER its `type`
 * declaration has been parsed (single-pass, declare-before-use -- the same
 * discipline `const` already uses for array bounds). Returns 1 on success
 * (with *out and *out_rectype set; *out_rectype is NULL for a scalar type, an
 * arena-owned original-spelling name for a record type), 0 with a located
 * error otherwise.
 */
static int parse_type_name(Parser *p, AstVarType *out, char **out_rectype)
{
    *out_rectype = NULL;
    if (check(p, TOK_KW_INTEGER)) { *out = AST_TY_INTEGER; advance(p); return 1; }
    if (check(p, TOK_KW_BOOLEAN)) { *out = AST_TY_BOOLEAN; advance(p); return 1; }
    if (check(p, TOK_KW_CHAR))    { *out = AST_TY_CHAR;    advance(p); return 1; }
    if (check(p, TOK_IDENT)) {
        char lc[PARSER_CONST_NAME_CAP];
        lower_span(lc, sizeof(lc), p->cur.lexeme, p->cur.length);
        if (rectype_find(p, lc) >= 0) {
            *out = AST_TY_RECORD;
            *out_rectype = ast_arena_strndup(p->arena, p->cur.lexeme,
                                             p->cur.length);
            advance(p);
            return 1;
        }
    }
    fail_at(p, p->cur.line, p->cur.col,
            "expected 'integer', 'boolean', 'char', or a declared record "
            "type name");
    return 0;
}

/*
 * array-type = "array" "[" array-bound ".." array-bound "]" "of"
 *              type-name ;   (B5, beads initech-54uu; B6 extends the element
 *              type to also allow a record type name, beads initech-rug7)
 *
 * The current token is 'array' on entry. `lo <= hi` is enforced HERE, at
 * parse/fold time (Rule 2 -- fail loud, never a codegen-time surprise).
 */
static int parse_array_type(Parser *p, long *out_lo, long *out_hi,
                            AstVarType *out_elem, char **out_elem_rectype)
{
    advance(p); /* 'array' */
    if (!expect(p, TOK_LBRACKET, "'[' after 'array'"))
        return 0;
    long lo, hi;
    if (!parse_array_bound(p, &lo))
        return 0;
    if (!expect(p, TOK_DOTDOT, "'..' in an array bound"))
        return 0;
    if (!parse_array_bound(p, &hi))
        return 0;
    if (!expect(p, TOK_RBRACKET, "']' after an array bound"))
        return 0;
    if (lo > hi) {
        fail_at(p, p->cur.line, p->cur.col,
                "array lower bound must be <= upper bound "
                "(lo <= hi, ADR-0007 DEC-02)");
        return 0;
    }
    if (!expect(p, TOK_KW_OF, "'of' after an array bound"))
        return 0;
    AstVarType elem;
    char *elem_rectype;
    if (!parse_type_name(p, &elem, &elem_rectype))
        return 0;
    *out_lo = lo;
    *out_hi = hi;
    *out_elem = elem;
    *out_elem_rectype = elem_rectype;
    return 1;
}

/* Parse "name { , name } : integer" (or ": boolean", B1 beads initech-f0uc,
 * or ": array[lo..hi] of T", B5 beads initech-54uu) into one AST_VARDECL.
 * The trailing ';' is consumed by the caller (the var-section loop). This
 * ONE function serves BOTH global (program-level) and LOCAL (a routine's own
 * var-section, parse_proc_or_func) declarations -- so a local array works
 * exactly like a global one, through the identical parse path. */
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
    if (check(p, TOK_KW_ARRAY)) {
        /* B5 (beads initech-54uu; ADR-0007 DEC-02 "static arrays"). B6
         * (beads initech-rug7): the element type may now also be a record
         * type name (array-of-record). */
        long lo, hi;
        AstVarType elem;
        char *elem_rectype;
        if (!parse_array_type(p, &lo, &hi, &elem, &elem_rectype))
            return NULL;
        vd->as.vardecl.is_array = 1;
        vd->as.vardecl.lo = lo;
        vd->as.vardecl.hi = hi;
        vd->as.vardecl.vtype = elem;
        vd->as.vardecl.rectype = elem_rectype;
    } else {
        /* B6 (beads initech-rug7): a scalar type keyword OR a declared
         * record type name (`r: Token;`) -- parse_type_name is the shared
         * "what type is this" parser (see its comment above). */
        AstVarType t;
        char *rt;
        if (!parse_type_name(p, &t, &rt))
            return NULL;
        vd->as.vardecl.vtype = t;
        vd->as.vardecl.rectype = rt;
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
/* B6 (beads initech-rug7): `type` sections of named record types      */
/* ------------------------------------------------------------------ */
/*
 * type-section = "type" type-decl ";" { type-decl ";" } ;
 * type-decl    = ident "=" record-type ;
 * record-type  = "record" field-list "end" ;
 * field-list   = field-group { ";" field-group } [ ";" ] ;
 * field-group  = ident { "," ident } ":" ("integer" | "boolean" | "char") ;
 *
 * `record` is the ONLY type-constructor in this subset (ADR-0007 DEC-02's
 * own text; see ast.h's B6 AST_TYPEDECL comment for the "why a `type`
 * section, why record-only" design note). A field's type is SCALAR ONLY --
 * no nested records, no array-typed fields (parse_type_name is deliberately
 * NOT used for a field's type: only the three bare scalar keywords are
 * accepted here). Global (top-level) ONLY -- there is no local `type`
 * section, mirroring the B3 const-section's local-declaration deferral
 * (parse_proc_or_func never calls this). `type`/`const`/`var` sections may
 * repeat and interleave in any order, exactly like const/var already do
 * (parse_program_root's top-level loop) -- so a var-decl referencing a
 * record type is valid the moment that type's declaration has been parsed
 * (single-pass declare-before-use, the same discipline array bounds use for
 * const references).
 */
static void parse_type_section(Parser *p, AstList *decls)
{
    if (!check(p, TOK_KW_TYPE))
        return;
    advance(p); /* 'type' */

    for (;;) {
        if (!check(p, TOK_IDENT)) {
            fail_at(p, p->cur.line, p->cur.col,
                    "expected a type name after 'type'");
            return;
        }
        int line = p->cur.line, col = p->cur.col;
        char *name = ast_arena_strndup(p->arena, p->cur.lexeme, p->cur.length);
        char name_lc[PARSER_CONST_NAME_CAP];
        lower_span(name_lc, sizeof(name_lc), p->cur.lexeme, p->cur.length);
        advance(p);

        if (!expect(p, TOK_EQ, "'=' in a type declaration"))
            return;
        if (!expect(p, TOK_KW_RECORD,
                    "'record' (the only type-constructor in this subset)"))
            return;

        AstNode *td = ast_new(p->arena, AST_TYPEDECL, line, col);
        td->as.typedecl.name = name;
        ast_list_init(&td->as.typedecl.fields);

        while (!check(p, TOK_KW_END) && !p->failed) {
            AstNode *fg = ast_new(p->arena, AST_VARDECL, p->cur.line,
                                  p->cur.col);
            ast_list_init(&fg->as.vardecl.names);
            for (;;) {
                if (!check(p, TOK_IDENT)) {
                    fail_at(p, p->cur.line, p->cur.col,
                            "expected a field name");
                    return;
                }
                AstNode *ref = ast_new(p->arena, AST_VARREF, p->cur.line,
                                       p->cur.col);
                ref->as.varref.name = ast_arena_strndup(p->arena,
                                                        p->cur.lexeme,
                                                        p->cur.length);
                ast_list_push(p->arena, &fg->as.vardecl.names, ref);
                advance(p);
                if (check(p, TOK_COMMA)) {
                    advance(p);
                    continue;
                }
                break;
            }
            if (!expect(p, TOK_COLON, "':' in a field declaration"))
                return;
            AstVarType ft;
            if (check(p, TOK_KW_INTEGER)) {
                ft = AST_TY_INTEGER;
                advance(p);
            } else if (check(p, TOK_KW_BOOLEAN)) {
                ft = AST_TY_BOOLEAN;
                advance(p);
            } else if (check(p, TOK_KW_CHAR)) {
                ft = AST_TY_CHAR;
                advance(p);
            } else {
                fail_at(p, p->cur.line, p->cur.col,
                        "expected 'integer', 'boolean', or 'char' (record "
                        "fields are scalar-only in this subset -- no nested "
                        "records or array-typed fields)");
                return;
            }
            fg->as.vardecl.vtype = ft;
            fg->as.vardecl.is_array = 0;
            fg->as.vardecl.rectype = NULL;
            ast_list_push(p->arena, &td->as.typedecl.fields, fg);
            if (!expect(p, TOK_SEMI, "';' after a field declaration"))
                return;
        }
        if (!expect(p, TOK_KW_END, "'end' to close a record type"))
            return;

        if (p->nrectypes >= PARSER_MAX_RECTYPES) {
            fail_at(p, line, col,
                    "too many record type declarations "
                    "(PARSER_MAX_RECTYPES exceeded)");
            return;
        }
        snprintf(p->rectypes[p->nrectypes], PARSER_CONST_NAME_CAP, "%s",
                 name_lc);
        p->nrectypes++;

        ast_list_push(p->arena, decls, td);

        if (!expect(p, TOK_SEMI, "';' after a type declaration"))
            return;
        if (check(p, TOK_IDENT))
            continue; /* another type-decl in the same `type` section */
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
        /* B6 (beads initech-rug7; ADR-0007 DEC-02 "records ... field
         * access"): a parameter's type may now also be a declared record
         * type name. DECISION (report, ADR silent -- the bead's own "decide
         * minimal, record" point): a VALUE (non-`var`) parameter of record
         * type is REJECTED LOUDLY here, at parse time -- Turbo Pascal would
         * copy the whole record on every call (expensive, and this seed's
         * own self-host source can always use `var` instead); only a `var`
         * parameter of record type is supported (the record's address is
         * passed -- needed for symbol-table-style helpers that mutate a
         * caller's record in place). See ast.h's B6 AST_TYPEDECL comment. */
        AstVarType t;
        char *rt;
        if (!parse_type_name(p, &t, &rt))
            return;
        if (t == AST_TY_RECORD && !is_var) {
            fail_at(p, p->cur.line, p->cur.col,
                    "value parameter of record type is not supported in "
                    "this subset (Turbo Pascal would copy the whole record "
                    "on every call; pass it as a `var` parameter instead)");
            return;
        }
        for (size_t i = group_start; i < params->count; i++) {
            params->items[i]->as.param.ptype = t;
            params->items[i]->as.param.rectype = rt;
        }
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
     * next" loop and it matches full TP dialect behaviour rather than an
     * artificially narrower one. B6 (beads initech-rug7) adds `type`
     * sections to this same repeated/interleaved loop -- a var-decl
     * referencing a record type just needs that type's `type` section to
     * have been parsed EARLIER in this same single forward pass.
     */
    for (;;) {
        if (check(p, TOK_KW_CONST)) {
            parse_const_section(p, &prog->as.program.decls);
        } else if (check(p, TOK_KW_VAR)) {
            parse_var_section(p, &prog->as.program.decls);
        } else if (check(p, TOK_KW_TYPE)) {
            parse_type_section(p, &prog->as.program.decls);
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
    p.nrectypes = 0;      /* B6 (beads initech-rug7): the record-type registry starts empty */
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
