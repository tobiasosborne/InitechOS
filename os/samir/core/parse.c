/*
 * os/samir/core/parse.c -- SAMIR xBase III+ expression parser (S3.2).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib -fno-stack-protector -fno-pic -std=c11 -Wall -Wextra -Werror).
 * Only <stdint.h> and rt.h/eval.h are included. No libc, no PAL, no INT 21h.
 *
 * Target: dBASE III PLUS 1.1 ONLY.
 *
 * Consumes the flat xb_token array produced by xb_lex (S3.1, lex.c) and
 * reduces it to an AST in a caller-provided xb_node pool (allocation-free;
 * children referenced by index). Recursive-descent / precedence-climbing
 * implementing the EXACT III+ ladder.
 *
 * -------------------------------------------------------------------------
 * THE TWO NON-STANDARD HEADLINE RULES (corpus mint, NOT math convention):
 *
 *   1. `^` / `**` are LEFT-associative:
 *        2^3^2  ==  (2^3)^2  ==  64     (NOT the math-standard 2^(3^2)=512)
 *      [verified: ../dbase3-decomp/re/mint-results-002.md
 *       "2^3^2 -> 64 -> ^ is LEFT-associative ((2^3)^2)"]
 *
 *   2. Unary minus binds TIGHTER than `^`:
 *        -2^2   ==  (-2)^2  ==  4       (NOT the math-standard -(2^2)=-4)
 *      [verified: ../dbase3-decomp/re/mint-results-002.md
 *       "-2^2 -> 4 -> unary minus binds tighter than ^ ((-2)^2)"]
 *
 * The static HELP surface (expressions-and-operators.md sec.9.1) left BOTH of
 * these [oracle-resolves] and even noted standard Xbase would do the opposite;
 * mint-002 is the live III+ 1.1 transcript that resolves them, and per Law 2
 * the oracle wins. Plan sec.5 S3.2 agrees with the mint (2^3^2=group-left,
 * -2^2 unary-binds-tighter). No discrepancy: spec was OPEN, mint+plan CLOSE it.
 *
 * -------------------------------------------------------------------------
 * PRECEDENCE LADDER (highest binds first) -- see eval.h S3.2 section:
 *   1  ( )                          grouping
 *   2  unary - , unary +           (TIGHTER than ^)   -- prefix, right-recurse
 *   3  ^ **                         exponentiation     -- LEFT-assoc
 *   4  * /                          mul/div            -- left-assoc
 *   5  + -                          add/sub/concat     -- left-assoc
 *   6  < > = <> # <= >= $           relational         -- left-assoc
 *   7  .NOT.                        logical negation   -- prefix
 *   8  .AND.                        logical and        -- left-assoc
 *   9  .OR.                         logical or         -- left-assoc (loosest)
 *
 * Grammar (each level descends to the next-tighter level):
 *   or_expr   := and_expr  ( .OR.  and_expr  )*
 *   and_expr  := not_expr  ( .AND. not_expr  )*
 *   not_expr  := .NOT. not_expr  |  rel_expr
 *   rel_expr  := add_expr  ( relop add_expr )*
 *   add_expr  := mul_expr  ( (+|-) mul_expr  )*
 *   mul_expr  := pow_expr  ( (*|/) pow_expr  )*
 *   pow_expr  := unary_expr ( (^|**) unary_expr )*   -- LEFT fold (mint-002)
 *   unary_expr:= (-|+) unary_expr | primary          -- unary TIGHTER than ^
 *   primary   := LIT_C | LIT_N | LIT_L | IDENT | call | '(' or_expr ')'
 *   call      := IDENT '(' [ or_expr ( ',' or_expr )* ] ')'   -- S3.5
 *
 * NOTE on rule 2 vs 3: `unary_expr` sits BELOW `pow_expr` in the descent so
 * that the unary prefix is parsed as a complete operand BEFORE `^` folds it.
 * Thus `-2^2` lexes to MINUS LIT_N(2) CARET LIT_N(2); pow_expr's first operand
 * is unary_expr = -(2), then `^ 2` folds -> (-2)^2 = 4. And `2^3^2` folds left
 * in pow_expr's loop -> (2^3)^2 = 64. Both headline rules fall out of the
 * grammar shape; no special-casing.
 *
 * -------------------------------------------------------------------------
 * MUTATION HOOK (Rule 6):
 *
 * Compile with -DXB_MUTATE_PARSE to enable a SINGLE perturbation: pow_expr
 * folds RIGHT-associatively instead of left. Then 2^3^2 parses as 2^(3^2)
 * (root ^ with RIGHT child = ^) and -2^2 parses as (-2)^2 still but the
 * 2^3^2 structural assertion goes RED, proving the oracle catches the
 * wrong grouping. Exactly one perturbed branch.
 *
 * -------------------------------------------------------------------------
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md sec.5 S3.2 (the ladder; mint-002)
 *   - ../dbase3-decomp/re/mint-results-002.md (2^3^2=64, -2^2=4 -- THE oracle)
 *   - ../dbase3-decomp/specs/language/expressions-and-operators.md sec.9.1
 *     (tiers 4-9 verified; tier 2/3 resolved by mint-002), sec.9.2 (unbalanced)
 *   - DBASE.MSG.strings.txt:8 "Unbalanced parenthesis." (XBPE_UNBALANCED)
 *   - os/samir/include/samir/eval.h (xb_node / xb_parse contract)
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 */

#include <stdint.h>
#include "samir/rt.h"
#include "samir/eval.h"

/* ======================================================================== */
/* Parser state                                                              */
/* ======================================================================== */

typedef struct {
    const xb_token *toks;   /* token stream (terminated by XBT_EOI) */
    uint32_t        ntok;   /* number of tokens incl. the XBT_EOI   */
    uint32_t        pos;    /* current token index                  */

    xb_node        *pool;   /* caller node storage                  */
    uint32_t        cap;    /* pool capacity                        */
    uint32_t        used;   /* nodes allocated so far               */

    int             err;    /* xb_parse_err; XBPE_OK while healthy   */
} xb_parser;

/* Current token (never reads past the array; the XBT_EOI sentinel guards). */
static const xb_token *cur(const xb_parser *p)
{
    return &p->toks[p->pos];
}

/* Advance past the current token (clamped at the EOI sentinel). */
static void advance(xb_parser *p)
{
    if (p->pos + 1 < p->ntok) {
        p->pos++;
    }
}

/* Allocate a node from the pool. Returns its index, or -1 on overflow
 * (sets p->err = XBPE_POOL_FULL -- fail loud, Rule 2). */
static int32_t alloc_node(xb_parser *p, xb_node_type type)
{
    xb_node *n;
    if (p->used >= p->cap) {
        if (p->err == XBPE_OK) p->err = XBPE_POOL_FULL;
        return -1;
    }
    n = &p->pool[p->used];
    n->type    = type;
    n->op      = XBT_EOI;     /* default: not an operator */
    n->kid[0]  = -1;
    n->kid[1]  = -1;
    n->src_off = 0;
    n->u.num   = 0.0;
    return (int32_t)(p->used++);
}

/* Forward declaration: the top of the precedence ladder. */
static int32_t parse_or(xb_parser *p);

/* Forward declaration: function-call argument-list parser (S3.5). */
static int32_t parse_call(xb_parser *p, uint32_t name_off,
                          const char *name_p, uint16_t name_len);

/* ======================================================================== */
/* primary := LIT | IDENT | '(' or_expr ')'                                  */
/* ======================================================================== */

static int32_t parse_primary(xb_parser *p)
{
    const xb_token *t = cur(p);

    switch (t->type) {

    case XBT_LIT_C: {
        int32_t id = alloc_node(p, XBN_LIT_C);
        if (id < 0) return -1;
        p->pool[id].src_off  = t->offset;
        p->pool[id].u.str.p   = t->u.c.p;
        p->pool[id].u.str.len = t->u.c.len;
        advance(p);
        return id;
    }

    case XBT_LIT_N: {
        int32_t id = alloc_node(p, XBN_LIT_N);
        if (id < 0) return -1;
        p->pool[id].src_off = t->offset;
        p->pool[id].u.num   = t->u.n;
        advance(p);
        return id;
    }

    case XBT_LIT_L: {
        int32_t id = alloc_node(p, XBN_LIT_L);
        if (id < 0) return -1;
        p->pool[id].src_off = t->offset;
        p->pool[id].u.log   = t->u.l;
        advance(p);
        return id;
    }

    case XBT_IDENT: {
        uint32_t off = t->offset;
        const char *ip = t->u.ident.p;
        uint16_t    il = t->u.ident.len;
        advance(p);
        /* Function-call shape: IDENT '(' args ')' (S3.5).
         * An IDENT immediately followed by '(' is a function call; parse a
         * comma-separated argument list of full sub-expressions into an
         * XBN_ARG linked list (eval.h XBN_CALL/XBN_ARG contract). An IDENT NOT
         * followed by '(' is a field/memvar reference (XBN_IDENT). */
        if (cur(p)->type == XBT_LPAREN) {
            return parse_call(p, off, ip, il);
        }
        {
            int32_t id = alloc_node(p, XBN_IDENT);
            if (id < 0) return -1;
            p->pool[id].src_off  = off;
            p->pool[id].u.str.p   = ip;
            p->pool[id].u.str.len = il;
            return id;
        }
    }

    case XBT_LPAREN: {
        int32_t inner;
        advance(p);                 /* consume '(' */
        inner = parse_or(p);
        if (inner < 0) return -1;
        if (cur(p)->type != XBT_RPAREN) {
            if (p->err == XBPE_OK) p->err = XBPE_UNBALANCED;
            return -1;
        }
        advance(p);                 /* consume ')' */
        return inner;
    }

    case XBT_EOI:
        /* Operand expected but stream ended. */
        if (p->err == XBPE_OK) p->err = XBPE_EMPTY;
        return -1;

    case XBT_RPAREN:
        /* ')' with no matching '(' open at this point. */
        if (p->err == XBPE_OK) p->err = XBPE_UNBALANCED;
        return -1;

    default:
        /* An operator (or XBT_ERROR) where an operand was expected. */
        if (p->err == XBPE_OK) p->err = XBPE_UNEXPECTED;
        return -1;
    }
}

/* ======================================================================== */
/* call := IDENT '(' [ or_expr ( ',' or_expr )* ] ')'           (S3.5)        */
/*                                                                            */
/* The leading IDENT and the '(' have NOT been consumed by the caller        */
/* (parse_primary consumed only the IDENT; cur(p) is the '(' on entry).      */
/* Builds an XBN_CALL node whose kid[0] is the head of an XBN_ARG list, with  */
/* each XBN_ARG.kid[0] = an argument's expression root and kid[1] = the next  */
/* XBN_ARG (or -1). Zero args -> kid[0] == -1. Each argument is a FULL        */
/* sub-expression (parse_or), so nested calls and operators work             */
/* (e.g. SUBSTR(cnum, VAL(SUBSTR(s,2,1))*7-13, 7) -- the NUMWORDS idiom).     */
/*                                                                            */
/* Determinism (Rule 11): the CALL node is allocated FIRST (lowest index for  */
/* the call), then arguments are parsed left-to-right and their ARG cells are */
/* appended in source order, so the same input always yields the same node    */
/* indices.                                                                   */
/* ======================================================================== */

static int32_t parse_call(xb_parser *p, uint32_t name_off,
                          const char *name_p, uint16_t name_len)
{
    int32_t call;
    int32_t prev_arg = -1;   /* index of the last XBN_ARG appended            */

    /* Allocate the CALL node first (stable lowest index for the call). */
    call = alloc_node(p, XBN_CALL);
    if (call < 0) return -1;
    p->pool[call].src_off   = name_off;
    p->pool[call].u.str.p   = name_p;
    p->pool[call].u.str.len = name_len;
    /* kid[0] (first arg) defaults to -1 via alloc_node; set when first arg
     * is parsed. kid[1] is unused for XBN_CALL (stays -1). */

    advance(p);   /* consume '(' */

    /* Zero-arg form: '()' immediately. */
    if (cur(p)->type == XBT_RPAREN) {
        advance(p);            /* consume ')' */
        return call;           /* kid[0] stays -1 (no args) */
    }

    /* One-or-more arguments: each a full sub-expression, comma-separated. */
    for (;;) {
        int32_t arg_expr;
        int32_t arg_cell;

        arg_expr = parse_or(p);
        if (arg_expr < 0) return -1;       /* parse_or set p->err loudly */

        arg_cell = alloc_node(p, XBN_ARG);
        if (arg_cell < 0) return -1;
        p->pool[arg_cell].src_off = p->pool[arg_expr].src_off;
        p->pool[arg_cell].kid[0]  = arg_expr;
        p->pool[arg_cell].kid[1]  = -1;     /* end of list until linked       */

        if (prev_arg < 0) {
            p->pool[call].kid[0] = arg_cell;     /* first arg -> CALL.kid[0]  */
        } else {
            p->pool[prev_arg].kid[1] = arg_cell; /* link onto the spine       */
        }
        prev_arg = arg_cell;

        if (cur(p)->type == XBT_COMMA) {
            advance(p);                     /* consume ',' and parse next arg */
            continue;
        }
        break;
    }

    /* The argument list MUST close with ')'. Anything else is loud. */
    if (cur(p)->type != XBT_RPAREN) {
        if (p->err == XBPE_OK) {
            p->err = (cur(p)->type == XBT_EOI) ? XBPE_UNBALANCED
                                               : XBPE_UNEXPECTED;
        }
        return -1;
    }
    advance(p);   /* consume ')' */
    return call;
}

/* ======================================================================== */
/* unary_expr := (-|+) unary_expr | primary       (TIGHTER than ^)           */
/* ======================================================================== */

static int32_t parse_unary(xb_parser *p)
{
    const xb_token *t = cur(p);

    if (t->type == XBT_MINUS || t->type == XBT_PLUS) {
        xb_token_type op = t->type;
        uint32_t off = t->offset;
        int32_t child, id;
        advance(p);
        child = parse_unary(p);     /* right-recurse: allow -- +- etc. */
        if (child < 0) return -1;
        id = alloc_node(p, XBN_UNOP);
        if (id < 0) return -1;
        p->pool[id].op      = op;
        p->pool[id].kid[0]  = child;
        p->pool[id].src_off = off;
        return id;
    }
    return parse_primary(p);
}

/* ======================================================================== */
/* pow_expr := unary_expr ( (^|**) unary_expr )*    LEFT-associative          */
/* mint-002: 2^3^2 = (2^3)^2 = 64.                                            */
/* ======================================================================== */

static int32_t parse_pow(xb_parser *p)
{
    int32_t left = parse_unary(p);
    if (left < 0) return -1;

    while (cur(p)->type == XBT_CARET || cur(p)->type == XBT_STARSTAR) {
        xb_token_type op = cur(p)->type;
        uint32_t off = cur(p)->offset;
        int32_t right, node;
        advance(p);

#ifdef XB_MUTATE_PARSE
        /*
         * MUTATION HOOK (Rule 6): fold ^ RIGHT-associatively. The right
         * operand recurses back into parse_pow instead of parse_unary, so
         * 2^3^2 parses as 2^(3^2) and the structural assertion (root.^.left
         * is itself a ^) goes RED. Exactly one perturbed branch.
         */
        right = parse_pow(p);
#else
        right = parse_unary(p);
#endif
        if (right < 0) return -1;

        node = alloc_node(p, XBN_BINOP);
        if (node < 0) return -1;
        p->pool[node].op      = op;
        p->pool[node].kid[0]  = left;
        p->pool[node].kid[1]  = right;
        p->pool[node].src_off = off;
        left = node;
    }
    return left;
}

/* ======================================================================== */
/* mul_expr := pow_expr ( (*|/) pow_expr )*          left-associative         */
/* ======================================================================== */

static int32_t parse_mul(xb_parser *p)
{
    int32_t left = parse_pow(p);
    if (left < 0) return -1;

    while (cur(p)->type == XBT_STAR || cur(p)->type == XBT_SLASH) {
        xb_token_type op = cur(p)->type;
        uint32_t off = cur(p)->offset;
        int32_t right, node;
        advance(p);
        right = parse_pow(p);
        if (right < 0) return -1;
        node = alloc_node(p, XBN_BINOP);
        if (node < 0) return -1;
        p->pool[node].op      = op;
        p->pool[node].kid[0]  = left;
        p->pool[node].kid[1]  = right;
        p->pool[node].src_off = off;
        left = node;
    }
    return left;
}

/* ======================================================================== */
/* add_expr := mul_expr ( (+|-) mul_expr )*          left-associative         */
/* ======================================================================== */

static int32_t parse_add(xb_parser *p)
{
    int32_t left = parse_mul(p);
    if (left < 0) return -1;

    while (cur(p)->type == XBT_PLUS || cur(p)->type == XBT_MINUS) {
        xb_token_type op = cur(p)->type;
        uint32_t off = cur(p)->offset;
        int32_t right, node;
        advance(p);
        right = parse_mul(p);
        if (right < 0) return -1;
        node = alloc_node(p, XBN_BINOP);
        if (node < 0) return -1;
        p->pool[node].op      = op;
        p->pool[node].kid[0]  = left;
        p->pool[node].kid[1]  = right;
        p->pool[node].src_off = off;
        left = node;
    }
    return left;
}

/* ======================================================================== */
/* rel_expr := add_expr ( relop add_expr )*          left-associative         */
/* relop = < > = <> # <= >= $   (Tier 6)                                      */
/* ======================================================================== */

static int is_relop(xb_token_type t)
{
    switch (t) {
    case XBT_LT: case XBT_GT: case XBT_EQ: case XBT_NEQ:
    case XBT_HASH: case XBT_LE: case XBT_GE: case XBT_DOLLAR:
        return 1;
    default:
        return 0;
    }
}

static int32_t parse_rel(xb_parser *p)
{
    int32_t left = parse_add(p);
    if (left < 0) return -1;

    while (is_relop(cur(p)->type)) {
        xb_token_type op = cur(p)->type;
        uint32_t off = cur(p)->offset;
        int32_t right, node;
        advance(p);
        right = parse_add(p);
        if (right < 0) return -1;
        node = alloc_node(p, XBN_BINOP);
        if (node < 0) return -1;
        p->pool[node].op      = op;
        p->pool[node].kid[0]  = left;
        p->pool[node].kid[1]  = right;
        p->pool[node].src_off = off;
        left = node;
    }
    return left;
}

/* ======================================================================== */
/* not_expr := .NOT. not_expr | rel_expr             prefix (Tier 7)          */
/* ======================================================================== */

static int32_t parse_not(xb_parser *p)
{
    if (cur(p)->type == XBT_NOT) {
        uint32_t off = cur(p)->offset;
        int32_t child, id;
        advance(p);
        child = parse_not(p);       /* right-recurse: .NOT. .NOT. x */
        if (child < 0) return -1;
        id = alloc_node(p, XBN_UNOP);
        if (id < 0) return -1;
        p->pool[id].op      = XBT_NOT;
        p->pool[id].kid[0]  = child;
        p->pool[id].src_off = off;
        return id;
    }
    return parse_rel(p);
}

/* ======================================================================== */
/* and_expr := not_expr ( .AND. not_expr )*          left-associative (Tier 8)*/
/* ======================================================================== */

static int32_t parse_and(xb_parser *p)
{
    int32_t left = parse_not(p);
    if (left < 0) return -1;

    while (cur(p)->type == XBT_AND) {
        uint32_t off = cur(p)->offset;
        int32_t right, node;
        advance(p);
        right = parse_not(p);
        if (right < 0) return -1;
        node = alloc_node(p, XBN_BINOP);
        if (node < 0) return -1;
        p->pool[node].op      = XBT_AND;
        p->pool[node].kid[0]  = left;
        p->pool[node].kid[1]  = right;
        p->pool[node].src_off = off;
        left = node;
    }
    return left;
}

/* ======================================================================== */
/* or_expr := and_expr ( .OR. and_expr )*            left-associative (Tier 9)*/
/* ======================================================================== */

static int32_t parse_or(xb_parser *p)
{
    int32_t left = parse_and(p);
    if (left < 0) return -1;

    while (cur(p)->type == XBT_OR) {
        uint32_t off = cur(p)->offset;
        int32_t right, node;
        advance(p);
        right = parse_and(p);
        if (right < 0) return -1;
        node = alloc_node(p, XBN_BINOP);
        if (node < 0) return -1;
        p->pool[node].op      = XBT_OR;
        p->pool[node].kid[0]  = left;
        p->pool[node].kid[1]  = right;
        p->pool[node].src_off = off;
        left = node;
    }
    return left;
}

/* ======================================================================== */
/* xb_parse -- entry point                                                   */
/* ======================================================================== */

int xb_parse(const xb_token *toks, uint32_t ntok,
             xb_node *pool, uint32_t cap,
             int *err_code)
{
    xb_parser p;
    int32_t root;

    *err_code = XBPE_OK;

    /* Guard: a valid stream always has at least the XBT_EOI sentinel. */
    if (ntok == 0 || cap == 0) {
        *err_code = XBPE_EMPTY;
        return -1;
    }

    p.toks = toks;
    p.ntok = ntok;
    p.pos  = 0;
    p.pool = pool;
    p.cap  = cap;
    p.used = 0;
    p.err  = XBPE_OK;

    /* Empty expression: just the EOI sentinel. */
    if (cur(&p)->type == XBT_EOI) {
        *err_code = XBPE_EMPTY;
        return -1;
    }

    root = parse_or(&p);
    if (root < 0 || p.err != XBPE_OK) {
        *err_code = (p.err != XBPE_OK) ? p.err : XBPE_UNEXPECTED;
        return -1;
    }

    /* A complete expression must consume everything up to the EOI sentinel.
     * Leftover tokens (e.g. an unbalanced ')' or "1 2") are a loud error. */
    if (cur(&p)->type != XBT_EOI) {
        if (cur(&p)->type == XBT_RPAREN) {
            *err_code = XBPE_UNBALANCED;
        } else {
            *err_code = XBPE_TRAILING;
        }
        return -1;
    }

    *err_code = XBPE_OK;
    return root;
}
