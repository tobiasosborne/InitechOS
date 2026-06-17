/*
 * os/samir/core/value.c -- SAMIR typed value constructors + operations.
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib, CDR-0001 interim toolchain). Uses ONLY rt.h helpers and
 * value.h; no libc. No PAL dependency -- this file is pure (no I/O).
 *
 * Covers step S0.6 of docs/plans/SAMIR-implementation-plan.md.
 *
 * Design notes:
 *   - All constructors are stack-return (xb_val is a small struct; the
 *     compiler will pass it in registers on i386 with -m32 when <= 8 bytes,
 *     and on the stack otherwise -- either way, no heap). value.c never
 *     allocates or frees.
 *   - xb_l normalises any non-zero integer to 1 so that the L payload is
 *     always 0 or 1 and xb_eq works without a second normalisation step.
 *   - xb_eq for C/M uses rt_memcmp (rt.h) -- the only rt.h helper used here.
 *     No other rt utility is needed at the value layer.
 *   - The U == U case returns 0 deliberately: undefined values are not equal,
 *     matching dBASE .NULL. semantics (two uninitialised variables are not
 *     considered equal).
 *
 * Freestanding-legal (Law 3): no #include other than <stdint.h> (via the
 * included headers) and our own headers.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 * Fail loud (Rule 2): invalid type tag in xb_type_char -> returns '?' which
 * is a visible sentinel; caller can assert it is not '?'.
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md sec.8.2 (the xb_val sketch), S0.6.
 *   - docs/plans/SAMIR-implementation-plan.md sec.2.F + sec.3.3 (numeric = IEEE
 *     double; date = JDN as double; corpus mint-004).
 *   - os/samir/include/samir/rt.h (rt_memcmp).
 *   - os/samir/include/samir/value.h (the contract).
 */

#include "samir/value.h"
#include "samir/rt.h"

/* ---- Constructors ---- */

xb_val xb_c(const char *p, uint16_t len)
{
    xb_val v;
    v.t     = XB_C;
    v.u.c.p = (char *)p; /* drop const: caller owns; engine treats as read-only */
    v.u.c.len = len;
    return v;
}

xb_val xb_n(double val)
{
    xb_val v;
    v.t   = XB_N;
    v.u.n = val;
    return v;
}

xb_val xb_d(double jdn)
{
    xb_val v;
    v.t   = XB_D;
    v.u.d = jdn;
    return v;
}

xb_val xb_l(int truth)
{
    xb_val v;
    v.t   = XB_L;
    v.u.l = (truth != 0) ? (uint8_t)1 : (uint8_t)0;
    return v;
}

xb_val xb_m(const char *p, uint16_t len)
{
    xb_val v;
    v.t     = XB_M;
    v.u.c.p = (char *)p; /* same layout as XB_C (shared union member) */
    v.u.c.len = len;
    return v;
}

xb_val xb_u(void)
{
    xb_val v;
    v.t = XB_U;
    /* No payload; zero the union to avoid any uninitialised-bytes noise that
     * could confuse a future binary-equality check on the struct. */
    v.u.n = 0.0;
    return v;
}

/* ---- Introspection ---- */

xb_type xb_typeof(const xb_val *v)
{
    return v->t;
}

char xb_type_char(xb_type t)
{
    /* Ref: SAMIR-implementation-plan.md S0.6; the TYPE() built-in (S3.5). */
    switch (t) {
    case XB_C: return 'C';
    case XB_N: return 'N';
    case XB_D: return 'D';
    case XB_L: return 'L';
    case XB_M: return 'M';
    case XB_U: return 'U';
    default:   return '?'; /* fail-loud sentinel (Rule 2); should never happen */
    }
}

/* ---- Equality ---- */

/*
 * xb_eq: raw per-type equality. See value.h for the full contract.
 *
 * Implementation notes:
 *   C / M: both lengths must match, then rt_memcmp over that length.
 *          (SET EXACT truncation is the evaluator's concern, not ours.)
 *   N:     exact IEEE double ==.
 *   D:     JDN double ==.
 *   L:     both payloads are already normalised to 0/1 by the constructors,
 *          so a simple == suffices.
 *   U:     always 0 (undefined != undefined).
 *   Cross-type: always 0.
 */
int xb_eq(const xb_val *a, const xb_val *b)
{
    if (a->t != b->t) {
        return 0; /* cross-type: never equal */
    }
    switch (a->t) {
    case XB_C:
    case XB_M:
        /* Both are char-slice types sharing the `c` union member. */
        if (a->u.c.len != b->u.c.len) {
            return 0;
        }
        if (a->u.c.len == 0) {
            return 1; /* two empty strings are equal */
        }
        return rt_memcmp(a->u.c.p, b->u.c.p, (uint32_t)a->u.c.len) == 0;
    case XB_N:
        return a->u.n == b->u.n;
    case XB_D:
        return a->u.d == b->u.d;
    case XB_L:
        /* Payloads normalised to 0/1 by xb_l; direct compare is safe. */
        return a->u.l == b->u.l;
    case XB_U:
        return 0; /* undefined is never equal to anything, including undefined */
    default:
        return 0; /* unreachable; fail-safe (Rule 2) */
    }
}
