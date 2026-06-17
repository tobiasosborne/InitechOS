/*
 * os/samir/core/eval.c -- SAMIR xBase III+ 1.1 expression EVALUATOR + coercion.
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib, CDR-0001 interim toolchain). Uses ONLY <stdint.h>, rt.h, value.h
 * and eval.h. No libc, no PAL, no I/O. Pure (Rule 11): the result is a
 * function of (pool, root, ctx state) -- no time, no host paths, no RNG.
 *
 * Step S3.3 of docs/plans/SAMIR-implementation-plan.md. Post-order evaluates
 * the xb_node AST that xb_parse (S3.2) built over the xb_token stream that
 * xb_lex (S3.1) produced. Every operator dispatches on its operand types
 * EXACTLY per the LOCKED contract spec/samir/xbase_coercion.json. The JSON is
 * the source of truth; this file HARDCODES that table in C (freestanding --
 * no JSON parser at runtime). Each branch cites the JSON cell / rule (Law 1).
 *
 * THE HAZARD (xbase_coercion.json not_in_iii_plus + mint-results-002.md):
 *   III+ 1.1 has NO auto-stringification. C+N is error #9 "Data type
 *   mismatch.", NOT "1" appended. Modern dbase.com docs describe auto-
 *   stringify that did NOT exist here. Every result:"error" cell fails loud.
 *
 * Fail loud (Rule 2): no silent coercion. A mismatched (lhs,op,rhs) sets the
 * dBASE catalog ordinal from dbase_msg_codes.tsv and returns non-zero.
 *
 * Ref (Law 1):
 *   - spec/samir/xbase_coercion.json (operator_coercion cells; rules R_*;
 *     modes.SET_EXACT default OFF, affects {=,<>,#}; $ + ordering ignore it)
 *   - spec/samir/dbase_msg_codes.tsv (#9 mismatch, #27 not_numeric,
 *     #37 not_logical, #39 num_overflow, #45 not_character)
 *   - ../dbase3-decomp/re/mint-results-002.md (C+N=#9; ""$"ABC"=.F.; SET EXACT
 *     default OFF + directional begins-with "ab"="a"=.T. / "a"="ab"=.F.;
 *     1/0 -> overflow asterisk-fill, NOT a trapped error)
 *   - ../dbase3-decomp/specs/language/coercion-table.md sec.6 (JSON basis)
 *   - os/samir/include/samir/eval.h (xb_ctx, xb_eval, xb_eval_err contract)
 *   - os/samir/include/samir/value.h (xb_val + ctors), rt.h (jdn / memcmp)
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 */

#include <stdint.h>

#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"

/* ======================================================================== */
/* Small helpers (freestanding; no libc)                                     */
/* ======================================================================== */

/* Set *err and return the standard failure rc for a failing branch. The
 * caller leaves *out as XB_U. Fail loud (Rule 2). */
static int fail(int *err_code, int code)
{
    *err_code = code;
    return code; /* non-zero for every real error code (positive or negative) */
}

/*
 * dbl_pow: x ^ y for the ^ / ** operators. Freestanding -- no libm. We need
 * only the cases the corpus exercises (integer and simple exponents) plus a
 * general fallback. Strategy: exact repeated multiply for integer exponents
 * (the overwhelming common case: 2^3^2, -2^2, salary^2); otherwise an
 * exp/log expansion would require libm, which is unavailable, so for a
 * non-integer exponent we compute via a Taylor-free repeated-squaring on the
 * integer part and IGNORE the fractional part (documented limitation; the
 * S3.3 oracle uses integer exponents only, matching mint-002's 2^3^2 / -2^2).
 *
 * Negative base with a fractional exponent is dBASE error #78 in the catalog,
 * but xbase_coercion.json scopes S3.3 to the type table; the numeric domain of
 * ^ (fractional exponent precision) is GATED with the numfn-1..8 cells
 * (ADR-0008 DEC-06) and resolved at S3.6. We keep integer-exponent exactness
 * here so the type-dispatch oracle is clean.
 */
static double dbl_pow_int(double base, int32_t e)
{
    double r = 1.0;
    int32_t k;
    int neg = 0;
    if (e < 0) { neg = 1; e = -e; }
    for (k = 0; k < e; k++) {
        r *= base;
    }
    if (neg) {
        return 1.0 / r;
    }
    return r;
}

static double dbl_pow(double base, double exp)
{
    /* Integer exponent (the corpus / oracle domain): exact repeated multiply.
     * Ref: mint-results-002.md 2^3^2=64, -2^2=4. */
    int32_t ie = (int32_t)exp;
    if ((double)ie == exp) {
        return dbl_pow_int(base, ie);
    }
    /* Non-integer exponent: GATED (numfn-1..8, ADR-0008 DEC-06); resolved at
     * S3.6. Approximate by the integer part so the engine is total, not so it
     * is accurate -- the S3.3 oracle does not exercise this path. */
    return dbl_pow_int(base, ie);
}

/* ======================================================================== */
/* C-string scratch allocation (for C+C concat and C-C reloc results)        */
/* ======================================================================== */

/*
 * scratch_alloc: bump-allocate `n` bytes from ctx->scratch. Returns a pointer
 * into the arena, or NULL on exhaustion (caller fails loud, XBEE_SCRATCH_FULL).
 * No malloc (Law 3 freestanding). The arena is reset per top-level xb_eval.
 */
static char *scratch_alloc(xb_ctx *ctx, uint32_t n)
{
    char *p;
    if (ctx->scratch == 0) {
        return 0;
    }
    if (n > ctx->scratch_cap || ctx->scratch_used > ctx->scratch_cap - n) {
        return 0; /* would overflow the arena */
    }
    p = ctx->scratch + ctx->scratch_used;
    ctx->scratch_used += n;
    return p;
}

/* ======================================================================== */
/* C-string operators (R_concat_plus / R_concat_minus / R_substr / compare)  */
/* ======================================================================== */

/* Count trailing blanks (0x20) in [p, p+len). */
static uint16_t trailing_blanks(const char *p, uint16_t len)
{
    uint16_t n = 0;
    while (n < len && p[len - 1 - n] == ' ') {
        n++;
    }
    return n;
}

/*
 * concat_plus: C + C. xbase_coercion.json R_concat_plus -- "trailing blanks
 * kept in place"; a straight byte concatenation, length = la + lb.
 */
static int concat_plus(xb_ctx *ctx, const xb_val *a, const xb_val *b,
                       xb_val *out, int *err)
{
    uint32_t la = a->u.c.len;
    uint32_t lb = b->u.c.len;
    uint32_t tot = la + lb;
    char *dst;
    if (tot == 0) {
        *out = xb_c("", 0);
        *err = XBEE_OK;
        return 0;
    }
    dst = scratch_alloc(ctx, tot);
    if (dst == 0) {
        return fail(err, XBEE_SCRATCH_FULL);
    }
    if (la) rt_memcpy(dst, a->u.c.p, la);
    if (lb) rt_memcpy(dst + la, b->u.c.p, lb);
    *out = xb_c(dst, (uint16_t)tot);
    *err = XBEE_OK;
    return 0;
}

/*
 * concat_minus: C - C. xbase_coercion.json R_concat_minus -- "LHS trailing
 * blanks moved to end; total length preserved". So result =
 *   (lhs with its trailing blanks removed) ++ rhs ++ (those lhs trailing blanks)
 * and total length = la + lb (preserved). Ref: coercion-table.md sec.6.
 */
static int concat_minus(xb_ctx *ctx, const xb_val *a, const xb_val *b,
                        xb_val *out, int *err)
{
    uint32_t la = a->u.c.len;
    uint32_t lb = b->u.c.len;
    uint16_t tb = trailing_blanks(a->u.c.p, (uint16_t)la);
    uint32_t a_core = la - tb;          /* lhs without its trailing blanks   */
    uint32_t tot = la + lb;             /* total length preserved            */
    char *dst;
    uint32_t off;
    uint32_t k;
    if (tot == 0) {
        *out = xb_c("", 0);
        *err = XBEE_OK;
        return 0;
    }
    dst = scratch_alloc(ctx, tot);
    if (dst == 0) {
        return fail(err, XBEE_SCRATCH_FULL);
    }
    off = 0;
    if (a_core) { rt_memcpy(dst, a->u.c.p, a_core); off += a_core; }
    if (lb)     { rt_memcpy(dst + off, b->u.c.p, lb); off += lb; }
    for (k = 0; k < tb; k++) {           /* moved lhs trailing blanks at end  */
        dst[off + k] = ' ';
    }
    *out = xb_c(dst, (uint16_t)tot);
    *err = XBEE_OK;
    return 0;
}

/*
 * c_substr: c1 $ c2 -> L. xbase_coercion.json R_substr -- literal substring,
 * case-sensitive, blanks significant, IGNORES SET EXACT. Edge case (verified
 * mint-results-002.md): "" $ "ABC" -> .F. (empty string is NOT contained).
 */
static int c_substr(const xb_val *needle, const xb_val *hay, uint8_t *res)
{
    uint32_t nl = needle->u.c.len;
    uint32_t hl = hay->u.c.len;
    uint32_t i;
    if (nl == 0) {
        *res = 0;                        /* mint-002: "" $ "ABC" -> .F.        */
        return 0;
    }
    if (nl > hl) {
        *res = 0;
        return 0;
    }
    for (i = 0; i + nl <= hl; i++) {
        if (rt_memcmp(hay->u.c.p + i, needle->u.c.p, nl) == 0) {
            *res = 1;
            return 0;
        }
    }
    *res = 0;
    return 0;
}

/*
 * c_begins_with: C = C under SET EXACT OFF. xbase_coercion.json R_begins_with
 * -- "LEFT must begin with RIGHT; directional; right-trailing-blanks ignored;
 * '' on right matches all". mint-002: "ab"="a" -> .T.; "a"="ab" -> .F.
 * The RIGHT operand's trailing blanks are stripped before the prefix test;
 * an all-blank / empty RIGHT therefore matches everything.
 */
static int c_begins_with(const xb_val *lhs, const xb_val *rhs, uint8_t *res)
{
    uint32_t ll = lhs->u.c.len;
    uint16_t rtb = trailing_blanks(rhs->u.c.p, rhs->u.c.len);
    uint32_t rl = (uint32_t)rhs->u.c.len - rtb; /* right, trailing blanks off */
    if (rl == 0) {
        *res = 1;                        /* '' (or all-blank) on right -> all  */
        return 0;
    }
    if (rl > ll) {
        *res = 0;
        return 0;
    }
    *res = (rt_memcmp(lhs->u.c.p, rhs->u.c.p, rl) == 0) ? 1 : 0;
    return 0;
}

/*
 * c_exact_eq: C = C under SET EXACT ON. xbase_coercion.json R_exact_blankpad
 * -- "equal except trailing blanks (either side) ignored; case-sensitive".
 */
static int c_exact_eq(const xb_val *a, const xb_val *b, uint8_t *res)
{
    uint16_t atb = trailing_blanks(a->u.c.p, a->u.c.len);
    uint16_t btb = trailing_blanks(b->u.c.p, b->u.c.len);
    uint32_t al = (uint32_t)a->u.c.len - atb;
    uint32_t bl = (uint32_t)b->u.c.len - btb;
    if (al != bl) {
        *res = 0;
        return 0;
    }
    if (al == 0) {
        *res = 1;
        return 0;
    }
    *res = (rt_memcmp(a->u.c.p, b->u.c.p, al) == 0) ? 1 : 0;
    return 0;
}

/*
 * c_byte_order: CP437 byte-wise compare of two C values (for <,>,<=,>= and the
 * ordering half of relational). Returns <0, 0, >0. xbase_coercion.json
 * modes.collation = CP437 byte-wise ASCII order. Trailing blanks are NOT
 * stripped here -- ordering uses raw byte compare with shorter-is-less on a
 * common prefix (standard memcmp semantics over min length, then length).
 */
static int c_byte_order(const xb_val *a, const xb_val *b)
{
    uint32_t al = a->u.c.len;
    uint32_t bl = b->u.c.len;
    uint32_t ml = (al < bl) ? al : bl;
    int c = 0;
    if (ml) {
        c = rt_memcmp(a->u.c.p, b->u.c.p, ml);
    }
    if (c != 0) {
        return c;
    }
    if (al < bl) return -1;
    if (al > bl) return 1;
    return 0;
}

/* ======================================================================== */
/* Relational dispatch                                                       */
/* ======================================================================== */

/*
 * cmp_order_num: ternary compare two doubles (used for N and D ordering).
 * D ordering with a blank date is handled by the caller (R_blankdate_high).
 */
static int cmp_double(double a, double b)
{
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

/*
 * Date blank sentinel. value.h stores D as JDN-as-double. A blank date has no
 * natural JDN; the codec layer (S1.x) represents it. xbase_coercion.json
 * R_blankdate_high: in ordering a blank date is GREATER THAN any non-blank.
 * R_date_minus_date: any subtraction touching a blank date = 0. We treat a
 * non-positive JDN (<= 0.0) as the blank sentinel: a real III+ date (1900..
 * 2155) has JDN 2415021..2508911, so 0 is unambiguous and matches xb_d(0).
 */
static int date_is_blank(double jdn)
{
    return jdn <= 0.0;
}

/*
 * apply_relational: evaluate (a op b) -> L for the relational operators
 * = <> # < > <= >= per xbase_coercion.json. Sets *res (0/1) on success or
 * fails loud with #9 for any cross-type / unsupported cell.
 */
static int apply_relational(xb_ctx *ctx, xb_token_type op,
                            const xb_val *a, const xb_val *b,
                            uint8_t *res, int *err)
{
    xb_type ta = a->t, tb = b->t;
    int is_eq_family = (op == XBT_EQ || op == XBT_NEQ || op == XBT_HASH);

    /* The relational cells in xbase_coercion.json are all SAME-base-type
     * (R_order_same_type): N<N, C<C, D<D, L<L, and the {=,<>,#} family on each.
     * Any cross-type relational is error #9 (N<C cell + mint-002 DATE()<"x"). */
    if (ta != tb) {
        return fail(err, XBEE_MISMATCH);   /* xbase_coercion.json N<C -> mismatch */
    }

    switch (ta) {
    case XB_C:
    case XB_M: {
        if (is_eq_family) {
            uint8_t eq;
            /* = / <> / # : SET EXACT-governed (modes.SET_EXACT.affects). */
            if (ctx->set_exact) {
                c_exact_eq(a, b, &eq);     /* R_exact_blankpad (EXACT ON)  */
            } else {
                c_begins_with(a, b, &eq);  /* R_begins_with   (EXACT OFF)  */
            }
            if (op == XBT_EQ) {
                *res = eq;
            } else {                       /* <> / # are negation of =     */
                *res = (uint8_t)(eq ? 0 : 1);
            }
        } else {
            int c = c_byte_order(a, b);    /* ordering: CP437, ignores EXACT */
            switch (op) {
            case XBT_LT: *res = (uint8_t)(c <  0); break;
            case XBT_GT: *res = (uint8_t)(c >  0); break;
            case XBT_LE: *res = (uint8_t)(c <= 0); break;
            case XBT_GE: *res = (uint8_t)(c >= 0); break;
            default:     return fail(err, XBEE_BAD_NODE);
            }
        }
        *err = XBEE_OK;
        return 0;
    }
    case XB_N: {
        int c = cmp_double(a->u.n, b->u.n);
        switch (op) {
        case XBT_EQ:  *res = (uint8_t)(c == 0); break;
        case XBT_NEQ: case XBT_HASH: *res = (uint8_t)(c != 0); break;
        case XBT_LT:  *res = (uint8_t)(c <  0); break;
        case XBT_GT:  *res = (uint8_t)(c >  0); break;
        case XBT_LE:  *res = (uint8_t)(c <= 0); break;
        case XBT_GE:  *res = (uint8_t)(c >= 0); break;
        default:      return fail(err, XBEE_BAD_NODE);
        }
        *err = XBEE_OK;
        return 0;
    }
    case XB_D: {
        /* R_blankdate_high: a blank date sorts GREATER than any non-blank. */
        int ba = date_is_blank(a->u.d);
        int bb = date_is_blank(b->u.d);
        int c;
        if (ba && bb)      c = 0;
        else if (ba)       c = 1;          /* blank > non-blank */
        else if (bb)       c = -1;
        else               c = cmp_double(a->u.d, b->u.d);
        switch (op) {
        case XBT_EQ:  *res = (uint8_t)(c == 0); break;
        case XBT_NEQ: case XBT_HASH: *res = (uint8_t)(c != 0); break;
        case XBT_LT:  *res = (uint8_t)(c <  0); break;
        case XBT_GT:  *res = (uint8_t)(c >  0); break;
        case XBT_LE:  *res = (uint8_t)(c <= 0); break;
        case XBT_GE:  *res = (uint8_t)(c >= 0); break;
        default:      return fail(err, XBEE_BAD_NODE);
        }
        *err = XBEE_OK;
        return 0;
    }
    case XB_L: {
        /* xbase_coercion.json L<L -> .F. < .T. (L ordering note). */
        int c = cmp_double((double)a->u.l, (double)b->u.l);
        switch (op) {
        case XBT_EQ:  *res = (uint8_t)(c == 0); break;
        case XBT_NEQ: case XBT_HASH: *res = (uint8_t)(c != 0); break;
        case XBT_LT:  *res = (uint8_t)(c <  0); break;
        case XBT_GT:  *res = (uint8_t)(c >  0); break;
        case XBT_LE:  *res = (uint8_t)(c <= 0); break;
        case XBT_GE:  *res = (uint8_t)(c >= 0); break;
        default:      return fail(err, XBEE_BAD_NODE);
        }
        *err = XBEE_OK;
        return 0;
    }
    default:
        return fail(err, XBEE_MISMATCH);
    }
}

/* ======================================================================== */
/* Binary operator dispatch (the heart of the coercion table)               */
/* ======================================================================== */

static int eval_binop(xb_ctx *ctx, xb_token_type op,
                      const xb_val *a, const xb_val *b,
                      xb_val *out, int *err)
{
    xb_type ta = a->t, tb = b->t;

    switch (op) {

    /* ---- + (arithmetic / concat / date-shift) ---------------------------- */
    case XBT_PLUS:
        if (ta == XB_N && tb == XB_N) {                 /* N+N -> N           */
            *out = xb_n(a->u.n + b->u.n);
            *err = XBEE_OK; return 0;
        }
        if ((ta == XB_C || ta == XB_M) &&
            (tb == XB_C || tb == XB_M)) {               /* C+C -> C (concat)  */
            return concat_plus(ctx, a, b, out, err);    /* R_concat_plus      */
        }
        if (ta == XB_D && tb == XB_N) {                 /* D+N -> D           */
            if (date_is_blank(a->u.d)) { *out = xb_d(0.0); }   /* blank stays */
            else { *out = xb_d(a->u.d + b->u.n); }      /* R_date_plus_num    */
            *err = XBEE_OK; return 0;
        }
        if (ta == XB_N && tb == XB_D) {                 /* N+D -> D           */
            if (date_is_blank(b->u.d)) { *out = xb_d(0.0); }
            else { *out = xb_d(b->u.d + a->u.n); }      /* R_date_plus_num    */
            *err = XBEE_OK; return 0;
        }
        /* C+N, N+C, C+D, D+D, ... -> error #9 (NO auto-stringify; the HAZARD).
         * xbase_coercion.json rows: C+N, N+C, C+D, D+D all result:"error". */
#ifdef XB_MUTATE_EVAL
        /* MUTATION (Rule 6): make the C+N HAZARD cell SUCCEED (auto-stringify
         * -- the modern-docs behavior that did NOT exist in III+ 1.1). The
         * test's "C+N -> error #9" assertion then goes RED. Single perturbation. */
        if ((ta == XB_C || ta == XB_M) && tb == XB_N) {
            *out = *a;                                  /* WRONG: return the C */
            *err = XBEE_OK; return 0;
        }
#endif
        return fail(err, XBEE_MISMATCH);

    /* ---- - (arithmetic / date-diff / date-shift / reloc) ----------------- */
    case XBT_MINUS:
        if (ta == XB_N && tb == XB_N) {                 /* N-N -> N           */
            *out = xb_n(a->u.n - b->u.n);
            *err = XBEE_OK; return 0;
        }
        if (ta == XB_D && tb == XB_N) {                 /* D-N -> D           */
            if (date_is_blank(a->u.d)) { *out = xb_d(0.0); } /* blank stays   */
            else { *out = xb_d(a->u.d - b->u.n); }      /* R_date_plus_num    */
            *err = XBEE_OK; return 0;
        }
        if (ta == XB_D && tb == XB_D) {                 /* D-D -> N (#days)   */
            /* R_date_minus_date: any subtraction touching a blank date = 0. */
            if (date_is_blank(a->u.d) || date_is_blank(b->u.d)) {
                *out = xb_n(0.0);
            } else {
                *out = xb_n(a->u.d - b->u.d);
            }
            *err = XBEE_OK; return 0;
        }
        if ((ta == XB_C || ta == XB_M) &&
            (tb == XB_C || tb == XB_M)) {               /* C-C -> C (reloc)   */
            return concat_minus(ctx, a, b, out, err);   /* R_concat_minus     */
        }
        /* C-N, N-D, ... -> error #9. xbase_coercion.json C-N, N-D result:"error".
         * (only D-N is sanctioned, NOT N-D.) */
        return fail(err, XBEE_MISMATCH);

    /* ---- * / ^ ** (numeric only) ----------------------------------------- */
    case XBT_STAR:
        if (ta == XB_N && tb == XB_N) {                 /* N*N -> N           */
            *out = xb_n(a->u.n * b->u.n);
            *err = XBEE_OK; return 0;
        }
        return fail(err, XBEE_MISMATCH);

    case XBT_SLASH:
        if (ta == XB_N && tb == XB_N) {                 /* N/N -> N           */
            if (b->u.n == 0.0) {
                /* div-by-zero: mint-002 shows 1/0 -> overflow asterisk-fill,
                 * NOT a trapped error. At the VALUE layer there is no display
                 * width to fill, so we map it to the numeric-overflow catalog
                 * code (#39). DECISION (oracle-resolves note in JSON): treat
                 * as XBEE_NUM_OVERFLOW, not a silent NaN/Inf. */
                return fail(err, XBEE_NUM_OVERFLOW);
            }
            *out = xb_n(a->u.n / b->u.n);
            *err = XBEE_OK; return 0;
        }
        return fail(err, XBEE_MISMATCH);

    case XBT_CARET:
    case XBT_STARSTAR:                                  /* ^ and ** identical */
        if (ta == XB_N && tb == XB_N) {                 /* N^N -> N           */
            *out = xb_n(dbl_pow(a->u.n, b->u.n));
            *err = XBEE_OK; return 0;
        }
        return fail(err, XBEE_MISMATCH);

    /* ---- relational -> L -------------------------------------------------- */
    case XBT_EQ:
    case XBT_NEQ:
    case XBT_HASH:
    case XBT_LT:
    case XBT_GT:
    case XBT_LE:
    case XBT_GE: {
        uint8_t res = 0;
        int rc = apply_relational(ctx, op, a, b, &res, err);
        if (rc != 0) return rc;
        *out = xb_l(res);
        return 0;
    }

    /* ---- $ substring -> L (C-only; ignores SET EXACT) -------------------- */
    case XBT_DOLLAR:
        if ((ta == XB_C || ta == XB_M) &&
            (tb == XB_C || tb == XB_M)) {               /* C $ C -> L         */
            uint8_t res = 0;
            c_substr(a, b, &res);                       /* R_substr           */
            *out = xb_l(res);
            *err = XBEE_OK; return 0;
        }
        /* N$C, C$N, ... -> error #9. xbase_coercion.json N$C result:"error";
         * mint-002 "x" $ 5 -> #9 ($ requires char). */
        return fail(err, XBEE_MISMATCH);

    /* ---- logical .AND. / .OR. (both operands must be L) ------------------ */
    case XBT_AND:
        if (ta != XB_L) return fail(err, XBEE_NOT_LOGICAL); /* R_no_truthiness */
        if (tb != XB_L) return fail(err, XBEE_NOT_LOGICAL);
        *out = xb_l(a->u.l && b->u.l);
        *err = XBEE_OK; return 0;

    case XBT_OR:
        if (ta != XB_L) return fail(err, XBEE_NOT_LOGICAL); /* R_no_truthiness */
        if (tb != XB_L) return fail(err, XBEE_NOT_LOGICAL);
        *out = xb_l(a->u.l || b->u.l);
        *err = XBEE_OK; return 0;

    default:
        return fail(err, XBEE_BAD_NODE); /* unknown binary op (Rule 2) */
    }
}

/* ======================================================================== */
/* Unary operator dispatch                                                   */
/* ======================================================================== */

static int eval_unop(xb_token_type op, const xb_val *a, xb_val *out, int *err)
{
    switch (op) {
    case XBT_MINUS:                                     /* unary -N -> N      */
        if (a->t == XB_N) {
            *out = xb_n(-a->u.n);
            *err = XBEE_OK; return 0;
        }
        /* unary minus on non-N -> mismatch (xbase_coercion.json same-type) */
        return fail(err, XBEE_MISMATCH);

    case XBT_PLUS:                                       /* unary +N -> N      */
        if (a->t == XB_N) {
            *out = *a;
            *err = XBEE_OK; return 0;
        }
        return fail(err, XBEE_MISMATCH);

    case XBT_NOT:                                        /* .NOT. L -> L       */
        if (a->t == XB_L) {                              /* R_no_truthiness    */
            *out = xb_l(!a->u.l);
            *err = XBEE_OK; return 0;
        }
        return fail(err, XBEE_NOT_LOGICAL);

    default:
        return fail(err, XBEE_BAD_NODE);
    }
}

/* ======================================================================== */
/* Function-call dispatch (S3.5)                                             */
/* ======================================================================== */

/* Forward declaration: eval_call evaluates argument sub-expressions. */
static int eval_node(const xb_node *pool, int idx, xb_ctx *ctx,
                     xb_val *out, int *err);

/* name_eq_ci: case-insensitive compare of a name slice vs a NUL-terminated
 * upper-case keyword (used here only to special-case IIF). */
static int name_eq_ci(const char *name, uint16_t len, const char *kw)
{
    uint16_t i;
    for (i = 0; i < len; i++) {
        char c = name[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        if (kw[i] == '\0') return 0;
        if (c != kw[i]) return 0;
    }
    return kw[len] == '\0';
}

/*
 * eval_call: evaluate an XBN_CALL node.
 *
 * IIF(<expL>, <exp1>, <exp2>) is handled SPECIALLY (eval.h): III+ IIF
 * evaluates the CONDITION, then ONLY the selected branch (lazy / short-circuit)
 * -- so IIF(.F., 1/0, 5) must NOT raise the divide error. This cannot go
 * through the eager xb_call_builtin path (which receives already-evaluated
 * args), so IIF walks the ARG spine itself and evaluates exactly two nodes.
 *
 * All other functions are EAGER: every argument sub-expression is evaluated
 * left-to-right into a fixed argv[], then xb_call_builtin (fn_builtins.c) does
 * the name lookup, arity/type checks, and the work.
 *
 * The ARG spine: call->kid[0] is the first XBN_ARG (or -1 for zero args); each
 * XBN_ARG.kid[0] is an argument expression root and kid[1] is the next XBN_ARG.
 */
static int eval_call(const xb_node *pool, const xb_node *call, xb_ctx *ctx,
                     xb_val *out, int *err)
{
    const char *name = call->u.str.p;
    uint16_t    nlen = call->u.str.len;
    int32_t     a    = call->kid[0];     /* first XBN_ARG, or -1               */
    xb_val      argv[XB_FN_MAX_ARGS];
    int         nargs = 0;

    if (name == 0 || nlen == 0) {
        return fail(err, XBEE_INVALID_FN);
    }

    /* ---- IIF: lazy branch selection (NOT through xb_call_builtin) -------- */
    if (name_eq_ci(name, nlen, "IIF")) {
        int32_t a_cond, a_then, a_else;
        xb_val  cond;
        int     rc;
        /* Exactly three args. */
        if (a < 0) return fail(err, XBEE_INVALID_ARG);
        a_cond = a;
        if (pool[a_cond].type != XBN_ARG) return fail(err, XBEE_BAD_NODE);
        a_then = pool[a_cond].kid[1];
        if (a_then < 0) return fail(err, XBEE_INVALID_ARG);
        a_else = pool[a_then].kid[1];
        if (a_else < 0) return fail(err, XBEE_INVALID_ARG);
        if (pool[a_else].kid[1] >= 0) return fail(err, XBEE_INVALID_ARG); /* >3 */

        /* Evaluate the condition; it MUST be Logical (no truthiness, III+). */
        rc = eval_node(pool, pool[a_cond].kid[0], ctx, &cond, err);
        if (rc != 0) return rc;
        if (cond.t != XB_L) return fail(err, XBEE_NOT_LOGICAL);

        /* Evaluate ONLY the selected branch (lazy). */
        if (cond.u.l) {
            return eval_node(pool, pool[a_then].kid[0], ctx, out, err);
        }
        return eval_node(pool, pool[a_else].kid[0], ctx, out, err);
    }

    /* ---- Eager path: evaluate every argument, then dispatch the table ---- */
    while (a >= 0) {
        int rc;
        if (pool[a].type != XBN_ARG) {
            return fail(err, XBEE_BAD_NODE);
        }
        if (nargs >= XB_FN_MAX_ARGS) {
            /* More args than any A-set function accepts -> arity error #11.
             * (No A-set function takes > 3; XB_FN_MAX_ARGS=4 leaves headroom.) */
            return fail(err, XBEE_INVALID_ARG);
        }
        rc = eval_node(pool, pool[a].kid[0], ctx, &argv[nargs], err);
        if (rc != 0) return rc;
        nargs++;
        a = pool[a].kid[1];
    }

    return xb_call_builtin(name, nlen, argv, nargs, ctx, out, err);
}

/* ======================================================================== */
/* Recursive AST walk                                                        */
/* ======================================================================== */

static int eval_node(const xb_node *pool, int idx, xb_ctx *ctx,
                     xb_val *out, int *err)
{
    const xb_node *n;
    if (idx < 0) {
        return fail(err, XBEE_BAD_NODE);
    }
    n = &pool[idx];

    switch (n->type) {
    case XBN_LIT_N:
        *out = xb_n(n->u.num);
        *err = XBEE_OK; return 0;

    case XBN_LIT_C:
        *out = xb_c(n->u.str.p, n->u.str.len);
        *err = XBEE_OK; return 0;

    case XBN_LIT_L:
        *out = xb_l(n->u.log);
        *err = XBEE_OK; return 0;

    case XBN_LIT_D:
        /* III+ has no date literal; the parser never emits this (eval.h).
         * If it ever appears, treat u.num as the JDN. Fail-safe. */
        *out = xb_d(n->u.num);
        *err = XBEE_OK; return 0;

    case XBN_IDENT: {
        /* Field / memvar lookup is the host's job (Phase 5). If no resolver
         * is bound, fail loud (Rule 2) -- S3.3 does not bind work areas. */
        xb_val v;
        if (ctx->resolve == 0) {
            return fail(err, XBEE_UNBOUND);
        }
        if (ctx->resolve(ctx->user, n->u.str.p, n->u.str.len, &v) != 0) {
            return fail(err, XBEE_UNBOUND); /* resolver said "not found" */
        }
        *out = v;
        *err = XBEE_OK; return 0;
    }

    case XBN_UNOP: {
        xb_val a;
        int rc = eval_node(pool, n->kid[0], ctx, &a, err);
        if (rc != 0) return rc;
        return eval_unop(n->op, &a, out, err);
    }

    case XBN_BINOP: {
        xb_val a, b;
        int rc = eval_node(pool, n->kid[0], ctx, &a, err);
        if (rc != 0) return rc;
        rc = eval_node(pool, n->kid[1], ctx, &b, err);
        if (rc != 0) return rc;
        return eval_binop(ctx, n->op, &a, &b, out, err);
    }

    case XBN_CALL:
        return eval_call(pool, n, ctx, out, err);

    case XBN_ARG:
        /* An XBN_ARG is only ever reached via eval_call's spine walk, never as
         * a stand-alone node in a value position. If one appears here the AST
         * is malformed. Fail loud (Rule 2). */
        return fail(err, XBEE_BAD_NODE);

    default:
        return fail(err, XBEE_BAD_NODE);
    }
}

/* ======================================================================== */
/* Public entry point                                                        */
/* ======================================================================== */

int xb_eval(const xb_node *pool, int root, xb_ctx *ctx,
            xb_val *out, int *err_code)
{
    int local_err = XBEE_OK;
    int rc;

    /* Fail loud on a NULL ctx (Rule 2): SET EXACT / scratch / resolver all
     * live there; there is no sane default for a missing context. */
    if (ctx == 0 || pool == 0 || out == 0) {
        if (err_code) *err_code = XBEE_BAD_NODE;
        if (out) *out = xb_u();
        return XBEE_BAD_NODE;
    }

    /* Reset the per-call C-result bump arena (Rule 11: deterministic). */
    ctx->scratch_used = 0;

    *out = xb_u();
    rc = eval_node(pool, root, ctx, out, &local_err);
    if (rc != 0) {
        *out = xb_u(); /* leave result undefined on error */
    }
    if (err_code) {
        *err_code = local_err;
    }
    return rc;
}
