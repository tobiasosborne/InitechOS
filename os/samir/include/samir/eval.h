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
    XBT_NOT     = 24,   /* .NOT. */

    /* --- Argument separator (S3.5, function call lists) --- */
    XBT_COMMA   = 25    /* , (separates function arguments)               */
                        /* Ref: string-functions.md (SUBSTR(c,n1,n2)...)  */
                        /* + numeric-and-date-functions.md (IIF(l,e1,e2)) */

    /* S3.3 added: nothing lexical */

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
 * XBN_CALL is a function call (S3.5). The parser detects the IDENT '(' shape
 * and parses a comma-separated argument list. To support N args in an
 * allocation-free pool whose nodes have only kid[2], the argument list is an
 * XBN_ARG singly-linked list:
 *
 *   XBN_CALL : u.str   = function-name slice (case-preserved, into source)
 *              kid[0]  = index of the FIRST XBN_ARG, or -1 for zero args
 *              kid[1]  = unused (-1)
 *   XBN_ARG  : kid[0]  = index of this argument's expression root
 *              kid[1]  = index of the NEXT XBN_ARG, or -1 (end of list)
 *
 * This keeps the model deterministic (Rule 11): identical input always yields
 * identical node indices, because args are reduced left-to-right and the ARG
 * spine is built in source order. `()` (no args) -> XBN_CALL.kid[0] == -1.
 *
 * Ref (Law 1): docs/plans/SAMIR-implementation-plan.md sec.5 S3.5
 *   ("fn_* table; STR/VAL/CTOD/DTOC/UPPER/.../IIF/TYPE"); the per-function
 *   semantics live in the corpus:
 *   - ../dbase3-decomp/specs/functions/string-functions.md (SUBSTR 1-based,
 *     LEN, UPPER/LOWER/TRIM/LTRIM, SPACE, CHR/ASC, STR, VAL)
 *   - ../dbase3-decomp/specs/functions/numeric-and-date-functions.md
 *     (CTOD/DTOC/DAY/MONTH/YEAR/DATE; IIF same-type branches; DTOS NOT III+)
 *   - ../dbase3-decomp/specs/functions/system-and-database-functions.md (TYPE)
 */
typedef enum {
    XBN_LIT_C  = 0,  /* character literal  -> u.str (slice into source)   */
    XBN_LIT_N  = 1,  /* numeric literal    -> u.num (decoded double)      */
    XBN_LIT_L  = 2,  /* logical literal    -> u.log (0/1)                 */
    XBN_LIT_D  = 3,  /* date literal       -- RESERVED; never emitted     */
    XBN_IDENT  = 4,  /* identifier / field -> u.str (slice into source)   */
    XBN_UNOP   = 5,  /* unary  op: op + kid[0]                            */
    XBN_BINOP  = 6,  /* binary op: op + kid[0]=left, kid[1]=right         */
    XBN_CALL   = 7,  /* function call: u.str=name, kid[0]=first XBN_ARG   */
    XBN_ARG    = 8   /* arg list cell: kid[0]=expr, kid[1]=next XBN_ARG   */
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
/* EVALUATOR (S3.3)                                                           */
/* ======================================================================== */

/*
 * S3.3 -- xBase III+ 1.1 expression EVALUATOR + operator type-coercion.
 *
 * Post-order evaluates the xb_node AST that xb_parse produced, dispatching
 * every binary/unary operator on its operand types EXACTLY per the LOCKED
 * contract spec/samir/xbase_coercion.json (operator_coercion cells + rules +
 * the SET EXACT mode). The JSON is the source of truth; eval.c HARDCODES the
 * dispatch in C (the engine is freestanding -- it does NOT parse JSON at
 * runtime). Every dispatch branch cites the JSON cell / rule (Law 1).
 *
 * HEADLINE III+ DELTA (corpus mint-results-002.md + xbase_coercion.json
 *   not_in_iii_plus): there is NO auto-stringification. C+N, N+C, C+D, D+D,
 *   C-N, N-D, N<C, N$C, N .AND. L ... are ERRORS, NOT silent coercions.
 *   "A" + 1 -> error #9 "Data type mismatch." [verified: mint-002 C1.TXT].
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md sec.5 S3.3 (the contract;
 *     "dispatch on (lhs,op,rhs) from xbase_coercion.json; SET EXACT-aware C=;
 *      D/N arithmetic; $; error#9. Oracle: the sec.3 dispatch-table cells")
 *   - spec/samir/xbase_coercion.json (THE CONTRACT -- every operator_coercion
 *     cell, every rule R_*, the SET_EXACT mode)
 *   - spec/samir/dbase_msg_codes.tsv (numeric error catalog; #9 mismatch,
 *     #27 not_numeric, #37 not_logical, #39 num_overflow, #45 not_character)
 *   - ../dbase3-decomp/re/mint-results-002.md (C+N=error #9; ""$"ABC"=.F.;
 *     SET EXACT default OFF + directional begins-with; 1/0 -> overflow-fill)
 */

/*
 * xb_eval_err -- error codes set in *err_code when xb_eval returns non-zero.
 *
 * These are the dBASE III+ 1.1 error CATALOG ordinals (the numbers a real
 * DBASE.EXE prints), mapped from spec/samir/dbase_msg_codes.tsv so the engine
 * reports the period-authentic code. The xbase_coercion.json error keys map:
 *   "mismatch"      -> XBEE_MISMATCH      (#9  "Data type mismatch.")
 *   "not_numeric"   -> XBEE_NOT_NUMERIC   (#27 "Not a numeric expression.")
 *   "not_logical"   -> XBEE_NOT_LOGICAL   (#37 "Not a Logical expression.")
 *   "num_overflow"  -> XBEE_NUM_OVERFLOW  (#39 "Numeric overflow (data was lost).")
 *   "not_character" -> XBEE_NOT_CHARACTER (#45 "Not a Character expression.")
 * Plus engine-internal conditions that are NOT III+ catalog codes (negative,
 * so they can never collide with a real 1..151 ordinal):
 *   XBEE_OK             0   success
 *   XBEE_UNBOUND      -1   identifier with no resolver (Phase 5 binds these)
 *   XBEE_SCRATCH_FULL -2   C+C / C-C result has no room in ctx scratch arena
 *   XBEE_BAD_NODE     -3   malformed AST (fail loud, Rule 2)
 *
 * Fail loud (Rule 2): every result:"error" cell in xbase_coercion.json sets
 * one of the positive catalog codes; NO silent coercion ever happens.
 */
typedef enum {
    XBEE_OK            = 0,
    XBEE_MISMATCH      = 9,   /* dbase_msg_codes.tsv code 9  */
    XBEE_INVALID_ARG   = 11,  /* dbase_msg_codes.tsv code 11 "Invalid function */
                              /*   argument." -- wrong arg count, bad value   */
                              /*   (S3.5: arity mismatch, negative SPACE n,   */
                              /*   negative REPLICATE count, etc.)            */
    XBEE_NOT_NUMERIC   = 27,  /* dbase_msg_codes.tsv code 27 */
    XBEE_INVALID_FN    = 31,  /* dbase_msg_codes.tsv code 31 "Invalid function */
                              /*   name." -- unknown OR not-in-III+ function   */
                              /*   (S3.5: DTOS, and any name not in the table) */
    XBEE_NOT_LOGICAL   = 37,  /* dbase_msg_codes.tsv code 37 */
    XBEE_NUM_OVERFLOW  = 39,  /* dbase_msg_codes.tsv code 39 */
    XBEE_NOT_CHARACTER = 45,  /* dbase_msg_codes.tsv code 45 */
    XBEE_CHR_RANGE     = 57,  /* dbase_msg_codes.tsv code 57 "CHR() : Out of   */
                              /*   range." -- CHR(n) with n outside 0..255     */
    XBEE_SPACE_LARGE   = 59,  /* dbase_msg_codes.tsv code 59 "SPACE() : Too    */
                              /*   large." -- SPACE(n) result > 254            */
    XBEE_SPACE_NEG     = 60,  /* dbase_msg_codes.tsv code 60 "SPACE() :        */
                              /*   Negative." -- SPACE(n) with n < 0           */
    XBEE_SUBSTR_RANGE  = 62,  /* dbase_msg_codes.tsv code 62 "SUBSTR() : Start */
                              /*   point out of range." -- start < 1           */
    XBEE_STR_RANGE     = 63,  /* dbase_msg_codes.tsv code 63 "STR() : Out of   */
                              /*   range." -- bad width/dec argument           */
    XBEE_UNBOUND       = -1,  /* identifier, no ctx->resolve hook bound      */
    XBEE_SCRATCH_FULL  = -2,  /* C result bytes do not fit ctx scratch arena */
    XBEE_BAD_NODE      = -3   /* malformed AST node (should be unreachable)  */
} xb_eval_err;

/*
 * xb_ctx -- evaluation context (mutable state + host hooks).
 *
 * set_exact: the SET EXACT mode. 0 = OFF (the III+ DEFAULT; xbase_coercion.json
 *   modes.SET_EXACT.default = "OFF"), non-zero = ON. Affects ONLY the C=C /
 *   C<>C / C#C operators (the JSON modes.affects list: =,<>,#,SEEK,FIND).
 *   The $ operator and the ordering operators <,>,<=,>= IGNORE it
 *   (modes.SET_EXACT.note + R_substr + R_order_same_type).
 *
 * resolve / user: OPTIONAL identifier-resolver hook. When the evaluator meets
 *   an XBN_IDENT node (a field or memvar name), it calls
 *       resolve(user, name, len, &out)
 *   which must return 0 and fill *out with the bound value, or non-zero to
 *   signal "not found". If resolve is NULL, an identifier is a FAIL-LOUD error
 *   (XBEE_UNBOUND) -- full work-area / memvar binding is Phase 5 (S5.x), NOT
 *   this step. The test harness and the Phase-5 interpreter supply this hook.
 *   `name` points into the ORIGINAL source bytes (case-preserved, NOT NUL-
 *   terminated); `len` is its byte length. The callee folds case as needed
 *   (dBASE names are case-insensitive).
 *
 * scratch / scratch_cap / scratch_used: a caller-provided bump arena for the
 *   bytes produced by the C+C concat and C-C reloc operators (xbase_coercion
 *   R_concat_plus / R_concat_minus) AND by the S3.5 string-producing built-in
 *   functions (STR, UPPER, LOWER, TRIM, LTRIM, SUBSTR, SPACE, CHR, DTOC, ...).
 *   value.h Character values are pointer+len with NO ownership, so a
 *   synthesized C result must live somewhere the evaluator does not own and
 *   cannot malloc (freestanding, Law 3). The evaluator bump-allocates from
 *   [scratch, scratch+scratch_cap); on exhaustion it fails loud with
 *   XBEE_SCRATCH_FULL rather than overflow. scratch_used is reset to 0 by
 *   xb_eval at the start of every top-level call. If a tree has no C
 *   concat/reloc/function, the arena is never touched and may be NULL/0.
 *
 * ctx_today: the INJECTABLE current date as a Julian Day Number (double),
 *   consumed by the DATE() built-in (S3.5). The engine is freestanding and
 *   MUST NOT read the OS clock; the caller (the dot-prompt REPL on the host
 *   PAL, or the Phase-5 interpreter) injects today's JDN here for
 *   reproducibility (Rule 11) -- exactly the PAL's injectable-clock contract
 *   (pal.h today()). 0.0 is the "blank date" sentinel (rt.h date_is_blank);
 *   a caller that has not set a date leaves it 0.0 and DATE() returns a blank
 *   date. Use (double)jdn_from_ymd(y,m,d) to set it.
 *   Ref: numeric-and-date-functions.md DATE() "system date"; pal.h today().
 */
typedef struct xb_ctx {
    int   set_exact;          /* 0 = OFF (III+ default); !=0 = ON */

    int  (*resolve)(void *user, const char *name, uint16_t len, xb_val *out);
    void  *user;

    char     *scratch;        /* bump arena for C+C / C-C / fn result bytes */
    uint32_t  scratch_cap;    /* capacity in bytes                     */
    uint32_t  scratch_used;   /* bytes consumed (reset per xb_eval)    */

    double    ctx_today;      /* INJECTABLE today as JDN double; DATE() (S3.5) */
} xb_ctx;

/*
 * xb_eval -- evaluate the AST rooted at `root` in pool[].
 *
 * pool / root: the node pool and root index returned by xb_parse.
 * ctx:      evaluation context (SET EXACT, resolver hook, scratch arena).
 *           Must be non-NULL. ctx->scratch_used is reset to 0 on entry.
 * out:      receives the result xb_val on success.
 * err_code: output; set to an xb_eval_err. XBEE_OK (0) on success, else the
 *           dBASE catalog ordinal (positive) or an engine code (negative).
 *
 * Returns:
 *   0  success; *out holds the result, *err_code == XBEE_OK.
 *   != 0 failure; *err_code names the reason (a result:"error" cell fired, an
 *        identifier was unbound, or the scratch arena was exhausted). *out is
 *        left as XB_U (undefined).
 *
 * Dispatch (xbase_coercion.json operator_coercion, exhaustive):
 *   N {+ - * / ^ **} N -> N   ;  C + C -> C (R_concat_plus)
 *   C - C -> C (R_concat_minus); D +/- N, N + D -> D (R_date_plus_num)
 *   D - D -> N (R_date_minus_date); relational -> L (SET EXACT-aware for C=C)
 *   C $ C -> L (R_substr); L {.AND. .OR.} L -> L; .NOT. L -> L; unary -N -> N.
 *   Every other (lhs,op,rhs) combination is a result:"error" cell -> fail loud.
 *
 * Allocation: none beyond the caller's scratch arena. Uses: <stdint.h>, rt.h,
 * value.h, eval.h. No libc, no PAL, no I/O. Reproducible (Rule 11): pure
 * function of (pool, root, ctx state) -- no time, no host paths, no RNG.
 */
int xb_eval(const xb_node *pool, int root, xb_ctx *ctx,
            xb_val *out, int *err_code);

/* ======================================================================== */
/* BUILT-IN FUNCTIONS A (S3.5)                                                */
/* ======================================================================== */

/*
 * S3.5 -- xBase III+ 1.1 built-in function dispatch (the "A" set: the pure
 * string / numeric / date / conversion functions). The DB-aware functions
 * (RECNO/EOF/RECCOUNT/...) and the deferred numeric/date functions
 * (INT/ROUND/MOD/CDOW/...) are S3.6 (built-in functions B) and are NOT here.
 *
 * The evaluator (eval.c, XBN_CALL case) evaluates each argument expression to
 * an xb_val, then calls xb_call_builtin with the function name slice and the
 * argument vector. The implementation lives in fn_builtins.c (a separate
 * freestanding translation unit, owned by S3.5). This keeps eval.c's coercion
 * table (S3.3) cleanly separated from the function table (S3.5).
 *
 * Name lookup is CASE-INSENSITIVE (dBASE folds case). An unknown name -- OR a
 * name that exists in a later dialect but NOT in III+ 1.1 (the canonical case:
 * DTOS) -- fails loud with XBEE_INVALID_FN (#31 "Invalid function name."),
 * exactly the corpus-documented III+ behavior
 * (numeric-and-date-functions.md: DTOS absent from @DATE FUNCTIONS).
 *
 * Arity / argument-type violations fail loud:
 *   - wrong argument COUNT            -> XBEE_INVALID_ARG  (#11)
 *   - argument of the wrong TYPE      -> XBEE_MISMATCH     (#9)
 *   - a value out of a function's domain uses that function's DEDICATED catalog
 *     code where one exists: CHR -> #57, SPACE neg -> #60 / too large -> #59,
 *     SUBSTR start -> #62, STR -> #63 (dbase_msg_codes.tsv).
 *
 * IIF(<expL>, <exp1>, <exp2>) is handled SPECIALLY by the evaluator (eval.c),
 * NOT by xb_call_builtin: III+ IIF evaluates its CONDITION first and then only
 * the SELECTED branch (lazy / short-circuit), so the unselected branch's
 * sub-expression must NOT be evaluated (e.g. IIF(.F., 1/0, 5) must not raise
 * the divide error). Eager evaluation of both branches would be wrong, so IIF
 * cannot go through the eager xb_call_builtin path. (system-and-database-
 * functions.md IIF: branch selected by the logical condition.)
 *
 * Functions in this set (xb_call_builtin):
 *   String:  UPPER LOWER TRIM LTRIM SUBSTR LEN SPACE CHR ASC
 *   Convert: STR VAL CTOD DTOC
 *   Date:    DATE DAY MONTH YEAR
 *   Generic: TYPE   (1-char type code of a sub-expression's result type)
 * (IIF is in this step but is dispatched in eval.c, see above.)
 *
 * args:     argument values, already evaluated, in source order.
 * nargs:    number of arguments (0..XB_FN_MAX_ARGS).
 * ctx:      evaluation context; supplies the scratch arena (for C results)
 *           and ctx_today (for DATE()).
 * out:      receives the result xb_val on success.
 * err_code: set to XBEE_OK on success, else a positive catalog ordinal.
 *
 * Returns 0 on success, non-zero on failure (and *err_code names the reason).
 * Allocation: none beyond ctx->scratch. Freestanding (Law 3): <stdint.h>,
 * rt.h, value.h, eval.h only. Reproducible (Rule 11): pure function of
 * (name, args, ctx state) -- the only "clock" is the injected ctx_today.
 */
#define XB_FN_MAX_ARGS 4   /* III+ A-set max arity (STUFF=4 is S3.6+; A-set
                            * peaks at SUBSTR/STR/IIF = 3 -- 4 leaves headroom) */

int xb_call_builtin(const char *name, uint16_t namelen,
                    const xb_val *args, int nargs,
                    xb_ctx *ctx, xb_val *out, int *err_code);

#endif /* INITECH_SAMIR_EVAL_H */
