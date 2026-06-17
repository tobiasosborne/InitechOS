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
/* PARSER (S3.2) -- placeholder section; populated by the next step          */
/* ======================================================================== */

/*
 * S3.2 will define xb_ast, xb_parse here.
 *
 * Precedence note for S3.2 (from expressions-and-operators.md sec.9.1):
 *   Tier 1: ( )  grouping
 *   Tier 2/3: unary -, unary +, ** ^ (relative order [oracle-resolves];
 *             minted: -2^2=4 so unary minus binds tighter than ^ in III+)
 *   Tier 4: * /
 *   Tier 5: binary + - (arithmetic and string concat; same token, type-dispatch
 *             is evaluator concern)
 *   Tier 6: relational  < > = <> # <= >= $
 *   Tier 7: .NOT.
 *   Tier 8: .AND.
 *   Tier 9: .OR.
 *   ^ is LEFT-associative: 2^3^2 = (2^3)^2 = 64 [verified: mint-results-002.md]
 */

/* ======================================================================== */
/* EVALUATOR (S3.3) -- placeholder section; populated at S3.3                */
/* ======================================================================== */

/*
 * S3.3 will define xb_ctx (SET EXACT state etc.), xb_eval, and the error
 * dispatch table consuming spec/samir/xbase_coercion.json.
 */

#endif /* INITECH_SAMIR_EVAL_H */
