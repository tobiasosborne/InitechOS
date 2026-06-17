/*
 * os/samir/include/samir/eval.h -- SAMIR expression engine public header.
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib). Depends ONLY on <stdint.h> and "samir/value.h".
 * No libc headers.
 *
 * This header is owned by the expression engine (gmo / bead initech-gmo.1).
 * It is structured for growth across three steps:
 *   S3.1  LEXER   -- defined in this file (section "LEXER (S3.1)")
 *   S3.2  PARSER  -- will extend this file with xb_ast, xb_parse
 *   S3.3  EVAL    -- will extend this file with xb_ctx, xb_eval
 *
 * Target: dBASE III PLUS 1.1 ONLY (SAMIR-implementation-plan.md sec.2.C).
 * The == operator is a dBASE IV feature and is a lex error here (sec.3.3).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md sec.2.C (III+1.1 only; == lex err)
 *   - docs/plans/SAMIR-implementation-plan.md S3.1 contract
 *   - ../dbase3-decomp/specs/language/expressions-and-operators.md sec.2,3,4,5
 *     (the III+ operator inventory, period-exact mined from HELP.DBS)
 *   - ../dbase3-decomp/specs/language/coercion-table.md sec.6 (operator set)
 *   - ../dbase3-decomp/specs/language/data-types.md sec.2,5 (literal syntax)
 *   - spec/samir/xbase_coercion.json not_in_iii_plus: ["==", "!=", "%", ...]
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 * Freestanding-legal (Law 3): only <stdint.h> and samir/value.h.
 */
#ifndef INITECH_SAMIR_EVAL_H
#define INITECH_SAMIR_EVAL_H

#include <stdint.h>
#include "samir/value.h"

/* ======================================================================== */
/* LEXER (S3.1)                                                               */
/* ======================================================================== */

/*
 * xb_token_type -- discriminator for every lexical unit produced by xb_lex.
 *
 * III+ operator inventory (period-exact from HELP.DBS @OPERATOR and its
 * sub-topics; refs below per group):
 *
 * Mathematical (#157, HELP.DBS @MATHEMATICAL OP):
 *   + - * / ** ^
 *   Both ** and ^ are exponentiation synonyms [verified: HELP.DBS @1791].
 *   Note: % (modulo) is NOT a III+ operator; use MOD() function.
 *   [verified: HELP.DBS @1786-1791 lists only + - * / ** ^;
 *    spec/samir/xbase_coercion.json not_in_iii_plus includes "%"]
 *
 * Relational (#158, HELP.DBS @RELATIONAL OP):
 *   < > = <> # <= >= $
 *   # is a synonym for <> (not-equal); both are distinct tokens here.
 *   [verified: HELP.DBS @1792-1803]
 *
 * Logical (#159, HELP.DBS @LOGICAL OP):
 *   .AND. .OR. .NOT.
 *   Dots are required; bare AND/OR/NOT are identifiers.
 *   [verified: HELP.DBS @1804-1809]
 *
 * String operators (#160): + and - (same tokens as arithmetic; type dispatch
 *   is the evaluator's job, not the lexer's).
 *
 * NOT in III+ -- all treated as lex errors:
 *   == (dBASE IV; SAMIR-implementation-plan.md sec.2.C + sec.3.3)
 *   != (not in III+; coercion-table.md not_in_iii_plus)
 *   %  (dBASE IV modulo operator; use MOD())
 *   [verified: HELP.DBS @RELATIONAL OP lists no == or !=;
 *    spec/samir/xbase_coercion.json not_in_iii_plus: ["==","!=","%",...]]
 *
 * Literal forms (data-types.md):
 *   Character: 'text', "text", [text]
 *     Single- and double-quote forms: verified.
 *     Bracket form [text]: documented in data-types.md sec.2 as valid III+
 *     syntax; GATED: exact nesting/escape behavior [oracle-resolves].
 *     Unterminated literal -> lex error (DBASE.MSG line 34 "Unterminated string.")
 *   Numeric: digits with optional single decimal point.
 *     A leading - or + is a separate token (unary operator), NOT part of the
 *     literal. [documented: data-types.md sec.3; consistent with III+ grammar]
 *   Logical: .T. .F. (verified). .Y. .N.:
 *     GATED: listed in data-types.md quick reference and section 5 as III+
 *     forms, but the dotted .Y./.N. token forms are [oracle-resolves] for
 *     use as source literals. CONSERVATIVE CHOICE: accept .Y. and .N. as
 *     true/false literals (same as .T./.F.) pending confirmation.
 *     [data-types.md sec.5 + expressions-and-operators.md sec.5.2 caveat]
 *   Date: no date literal in III+; CTOD() is the only constructor.
 *     [verified: data-types.md sec.4 "no {..} date literal in III+"]
 */
typedef enum {
    /* --- End / error sentinels --- */
    XBT_EOI     = 0,    /* end of input; always last token in a valid stream */
    XBT_ERROR   = 1,    /* lex error; check err_code for detail */

    /* --- Literals --- */
    XBT_LIT_C   = 2,    /* character string literal ('..', "..", [..]) */
    XBT_LIT_N   = 3,    /* numeric literal (digits[.digits]) */
    XBT_LIT_L   = 4,    /* logical literal (.T. .F. .Y. .N.) */

    /* --- Identifier / field name --- */
    XBT_IDENT   = 5,    /* letter-or-underscore start, alnum/underscore body */

    /* --- Grouping --- */
    XBT_LPAREN  = 6,    /* ( */
    XBT_RPAREN  = 7,    /* ) */

    /* --- Arithmetic / string operators --- */
    XBT_PLUS    = 8,    /* + (binary add / string concat) */
    XBT_MINUS   = 9,    /* - (binary sub / string concat-reloc / unary neg) */
    XBT_STAR    = 10,   /* * (multiply) */
    XBT_SLASH   = 11,   /* / (divide) */
    XBT_STARSTAR = 12,  /* ** (exponentiation; synonym of ^) */
    XBT_CARET   = 13,   /* ^ (exponentiation; synonym of **) */

    /* --- Relational operators --- */
    XBT_LT      = 14,   /* < */
    XBT_GT      = 15,   /* > */
    XBT_EQ      = 16,   /* = (equality; governed by SET EXACT for C) */
    XBT_NEQ     = 17,   /* <> (not-equal) */
    XBT_HASH    = 18,   /* # (not-equal; synonym of <>) */
    XBT_LE      = 19,   /* <= */
    XBT_GE      = 20,   /* >= */
    XBT_DOLLAR  = 21,   /* $ (substring containment; C only) */

    /* --- Logical operators (dotted; require surrounding dots) --- */
    XBT_AND     = 22,   /* .AND. */
    XBT_OR      = 23,   /* .OR. */
    XBT_NOT     = 24    /* .NOT. */

    /* S3.2 will add: XBT_COMMA (for function arg lists) etc. */
    /* S3.3 will add: nothing lexical */

} xb_token_type;

/*
 * xb_lex_err -- error codes set in *err_code when xb_lex returns negative.
 *
 * Distinct codes allow the caller (test harness / evaluator) to assert the
 * precise failure mode. Fail loud (Rule 2).
 */
typedef enum {
    XBLE_OK             = 0,  /* no error */
    XBLE_EQ_EQ          = 1,  /* == encountered (dBASE IV operator, NOT III+) */
    XBLE_BANG_EQ        = 2,  /* != encountered (not in III+) */
    XBLE_PERCENT        = 3,  /* % encountered (use MOD(); not a III+ operator) */
    XBLE_UNTERM_STR     = 4,  /* unterminated string literal (DBASE.MSG line 34) */
    XBLE_UNKNOWN_CHAR   = 5,  /* character that is not part of any III+ token */
    XBLE_BUF_OVERFLOW   = 6   /* token buffer too small (cap exceeded) */
} xb_lex_err;

/*
 * xb_token -- a single lexical token.
 *
 * Source span (offset + len): byte positions into the source string passed
 * to xb_lex. Always refers to the original source bytes; the lexer never
 * copies or allocates.
 *
 * For XBT_LIT_C:
 *   span covers the entire delimited literal including the delimiters.
 *   The decoded content (without delimiters) is c.p / c.len. c.p points
 *   directly into the source buffer (no copy).
 *
 * For XBT_LIT_N:
 *   span covers the digit sequence. n holds the pre-decoded double value
 *   (convenience; the parser may reuse directly).
 *
 * For XBT_LIT_L:
 *   span covers the .T./.F./.Y./.N. token. l is 1 (true) or 0 (false).
 *
 * For XBT_IDENT:
 *   span covers the identifier bytes. ident.p + ident.len point into the
 *   source. Case is preserved here; callers fold case for comparison
 *   (dBASE identifiers are case-insensitive).
 *
 * For XBT_EOI: span is (offset=end-of-input, len=0).
 * For XBT_ERROR: span covers the offending byte(s).
 * For all operator/paren tokens: span covers the operator bytes.
 */
typedef struct {
    xb_token_type type;
    uint32_t      offset; /* byte offset into source */
    uint16_t      len;    /* byte length of the token in source */
    union {
        /* XBT_LIT_C: pointer+len into source (no copy; excludes delimiters) */
        struct { const char *p; uint16_t len; } c;
        /* XBT_LIT_N: decoded value */
        double n;
        /* XBT_LIT_L: 1 = true (.T./.Y.), 0 = false (.F./.N.) */
        uint8_t l;
        /* XBT_IDENT: pointer+len into source (case-preserved) */
        struct { const char *p; uint16_t len; } ident;
    } u;
} xb_token;

/*
 * xb_lex -- tokenize an xBase III+ expression.
 *
 * src:      the source bytes (need not be NUL-terminated).
 * len:      number of source bytes.
 * out:      caller-provided token buffer. The lexer fills it left-to-right.
 *           The last token written is always XBT_EOI (on success) or XBT_ERROR
 *           (on failure), and that token does NOT occupy an extra slot beyond
 *           what the function reports.
 * cap:      capacity of `out` in tokens (must be >= 1).
 * err_code: output; set to an xb_lex_err value. Set to XBLE_OK on success,
 *           or to the relevant error code on failure.
 *
 * Returns:
 *   >= 0: number of tokens produced (including the terminal XBT_EOI or
 *         XBT_ERROR). On success the last token is XBT_EOI.
 *   < 0:  fatal error (-1). *err_code names the reason. The partial token
 *         stream in `out` (up to the error) is not meaningful.
 *
 * On XBLE_EQ_EQ: the == is a III+ lex error per plan sec.2.C and sec.3.3.
 *   *err_code = XBLE_EQ_EQ, returns -1. This is THE headline behavior of S3.1.
 *
 * Allocation: none. The lexer is pure and freestanding.
 * Uses: rt.h helpers (rt_strlen). No libc, no PAL, no INT 21h.
 */
int xb_lex(const char *src, uint32_t len,
           xb_token *out, uint32_t cap,
           int *err_code);

/* ======================================================================== */
/* PARSER (S3.2)                                                              */
/* ======================================================================== */

/*
 * S3.2 -- xBase III+ expression precedence parser -> AST.
 *
 * Builds an abstract syntax tree over the flat xb_token array that xb_lex
 * produces. Allocation-free: nodes live in a caller-provided pool (an array
 * of xb_node) and reference their children by INDEX, mirroring the lexer's
 * caller-provided-buffer model. This is freestanding-friendly (no malloc, no
 * recursion-into-the-heap) and reproducible (Rule 11): no pointers into the
 * pool are stored, so the same input always yields the same node indices.
 *
 * ------------------------------------------------------------------------
 * PRECEDENCE LADDER (highest binds first) -- the EXACT III+ 1.1 ladder.
 *
 * The static HELP surface leaves the unary/^ interaction and ^ associativity
 * [oracle-resolves] (expressions-and-operators.md sec.9.1 lines 754-759, which
 * even *notes* that standard Xbase/Clipper would give -2^2 = -(2^2) = -4 and
 * right-assoc 2^3^2). The corpus MINT against real dBASE III+ 1.1 SETTLES both
 * the other way, and per Law 2 the live oracle wins:
 *
 *   Tier 1  ( )  grouping / (function call placeholder -> S3.5)
 *   Tier 2  unary .NOT. is NOT here; unary arithmetic prefix:
 *           unary `-`, unary `+`   -- bind TIGHTER than `^`
 *           [verified: mint-results-002.md "-2^2 = 4 -> unary minus binds
 *            tighter than ^ ((-2)^2)"]
 *   Tier 3  `^` / `**`  (exponentiation)  -- LEFT-associative
 *           [verified: mint-results-002.md "2^3^2 = 64 -> ^ is LEFT-
 *            associative ((2^3)^2)"]
 *   Tier 4  `*` `/`                         left-assoc
 *   Tier 5  binary `+` `-`                  left-assoc (arith AND string concat;
 *                                           type-dispatch is the evaluator's job)
 *   Tier 6  relational `< > = <> # <= >= $` left-assoc (chaining)
 *   Tier 7  `.NOT.`  (logical negation, prefix)
 *   Tier 8  `.AND.`                         left-assoc
 *   Tier 9  `.OR.`                          left-assoc (loosest)
 *
 * Consequences (all from mint-002 / sec.9.1 verified idioms):
 *   2+3*4   = 2+(3*4)               (Tier 4 over Tier 5)
 *   2^3^2   = (2^3)^2 = 64          (^ LEFT-assoc)
 *   -2^2    = (-2)^2 = 4            (unary minus TIGHTER than ^)
 *   a > 0 .AND. .NOT. b = ((a>0) .AND. (.NOT. b))   (CLRDEP.PRG:154)
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md sec.5 S3.2 (the ladder; mint-002)
 *   - ../dbase3-decomp/re/mint-results-002.md (THE authority: 2^3^2=64, -2^2=4)
 *   - ../dbase3-decomp/specs/language/expressions-and-operators.md sec.9.1
 *     (tiers 4-9 verified; tier 2/3 left open there, resolved by mint-002)
 *   - DBASE.MSG.strings.txt:8 "Unbalanced parenthesis." (XBPE_UNBALANCED)
 */

/*
 * xb_node_type -- the AST node kinds.
 *
 * Literal nodes carry a decoded payload compatible with samir/value.h so the
 * S3.3 evaluator can lift them straight into an xb_val:
 *   XBN_LIT_N  numeric  -> xb_n(node.u.num)
 *   XBN_LIT_C  char     -> xb_c(node.u.str.p, node.u.str.len)
 *   XBN_LIT_L  logical  -> xb_l(node.u.log)
 *   XBN_LIT_D  date     -- reserved; III+ has NO date literal (CTOD() only),
 *              so the parser NEVER emits XBN_LIT_D. The kind exists so the
 *              node model is type-complete for S3.3 (data-types.md sec.4).
 * XBN_IDENT carries a slice into the source (field / memvar name).
 * XBN_UNOP / XBN_BINOP carry the operator's xb_token_type in `op` and child
 * node indices (kid[0] for unary; kid[0]=left, kid[1]=right for binary).
 * XBN_CALL is a function-call placeholder for S3.5; the parser detects the
 * IDENT '(' shape but does NOT implement argument parsing here (it returns
 * XBPE_UNEXPECTED for a call-shaped input -- calls are S3.5).
 */
typedef enum {
    XBN_LIT_C  = 0,  /* character literal  -> u.str (slice into source)   */
    XBN_LIT_N  = 1,  /* numeric literal    -> u.num (decoded double)      */
    XBN_LIT_L  = 2,  /* logical literal    -> u.log (0/1)                 */
    XBN_LIT_D  = 3,  /* date literal       -- RESERVED; never emitted     */
    XBN_IDENT  = 4,  /* identifier / field -> u.str (slice into source)   */
    XBN_UNOP   = 5,  /* unary  op: op + kid[0]                            */
    XBN_BINOP  = 6,  /* binary op: op + kid[0]=left, kid[1]=right         */
    XBN_CALL   = 7   /* function call placeholder (S3.5; not implemented) */
} xb_node_type;

/*
 * xb_node -- one AST node in the caller's pool.
 *
 * Children are referenced by index into the same pool (>= 0). An unused child
 * slot is -1. `op` is meaningful only for XBN_UNOP / XBN_BINOP and holds the
 * operator's xb_token_type (e.g. XBT_CARET, XBT_PLUS, XBT_NOT, XBT_MINUS for
 * unary negate). For literals/identifiers `op` is unused (set to XBT_EOI).
 *
 * Literal payloads mirror samir/value.h so S3.3 can construct xb_val directly:
 *   u.num : XBN_LIT_N decoded value (from the lexer's xb_token.u.n)
 *   u.str : XBN_LIT_C / XBN_IDENT slice (pointer+len into the ORIGINAL source
 *           bytes passed to xb_lex; not owned, not copied)
 *   u.log : XBN_LIT_L 0/1 (from the lexer's xb_token.u.l)
 */
typedef struct {
    xb_node_type  type;
    xb_token_type op;        /* operator token for UNOP/BINOP; else XBT_EOI  */
    int32_t       kid[2];    /* child node indices; unused slot = -1         */
    uint32_t      src_off;   /* source byte offset of the token that began    */
                             /* this node (for diagnostics; deterministic)    */
    union {
        double num;                          /* XBN_LIT_N                    */
        struct { const char *p; uint16_t len; } str; /* XBN_LIT_C / XBN_IDENT */
        uint8_t log;                         /* XBN_LIT_L                    */
    } u;
} xb_node;

/*
 * xb_parse_err -- error codes set in *err_code when xb_parse returns negative.
 * Fail loud, no silent recovery (Rule 2).
 */
typedef enum {
    XBPE_OK          = 0,  /* no error                                       */
    XBPE_UNEXPECTED  = 1,  /* unexpected token (e.g. operator where operand   */
                           /* expected, or a function-call shape -> S3.5)     */
    XBPE_UNBALANCED  = 2,  /* unbalanced parenthesis (DBASE.MSG.strings.txt:8)*/
    XBPE_POOL_FULL   = 3,  /* node pool capacity exceeded                     */
    XBPE_EMPTY       = 4,  /* empty expression (no operand at all)            */
    XBPE_TRAILING    = 5   /* leftover tokens after a complete expression     */
} xb_parse_err;

/*
 * xb_parse -- parse a flat xb_token stream into an AST.
 *
 * toks:     token array produced by xb_lex (terminated by an XBT_EOI token).
 * ntok:     number of tokens in `toks` INCLUDING the terminal XBT_EOI.
 * pool:     caller-provided node storage. The parser fills it left-to-right
 *           as it reduces sub-expressions; nodes reference children by index.
 * cap:      capacity of `pool` in nodes (must be >= 1).
 * err_code: output; set to an xb_parse_err. XBPE_OK on success.
 *
 * Returns:
 *   >= 0: the ROOT node index into `pool`.
 *   < 0:  parse error (-1). *err_code names the reason. Partial pool contents
 *         are not meaningful.
 *
 * The parser does NOT call the lexer; it consumes a token stream the caller
 * already produced. It rejects an XBT_ERROR token in the stream as
 * XBPE_UNEXPECTED (the lexer should have failed first, but fail loud anyway).
 *
 * Allocation: none. Uses: <stdint.h>, rt.h, eval.h only. No libc, no PAL.
 */
int xb_parse(const xb_token *toks, uint32_t ntok,
             xb_node *pool, uint32_t cap,
             int *err_code);

/* ======================================================================== */
/* EVALUATOR (S3.3) -- placeholder section; populated at S3.3                */
/* ======================================================================== */

/*
 * S3.3 will define xb_ctx (SET EXACT state etc.), xb_eval, and the error
 * dispatch table consuming spec/samir/xbase_coercion.json.
 */

#endif /* INITECH_SAMIR_EVAL_H */
