/*
 * os/samir/boot/softfp.c -- vendored soft-float + 64-bit integer runtime helpers
 *                           for the freestanding SAMIR.COM link (ADR-0009 DEC-02).
 *
 * THE ARTIFACT (Law 3): freestanding C (-msoft-float -mno-80387). No libc, no
 * x87. Provides the exact set of libgcc helpers the SAMIR engine references when
 * compiled -msoft-float, which the host toolchain on this box CANNOT supply: the
 * installed gcc-13 has NO usable static 32-bit libgcc.a (the x86_64 libgcc.a is
 * 64-bit; gcc-multilib / a static i386 libgcc is not installed; lib32gcc-s1 ships
 * only the shared libgcc_s.so, unusable in a flat freestanding link). DEC-02
 * anticipated exactly this: "as a fallback, vendor a small audited softfp.c."
 *
 * WHY VENDORED (DEC-02 / Rule 11, pinned-libgcc): a floating system libgcc would
 * be a reproducibility hole even if it existed; vendoring pins the helper code in
 * the repo so the SAMIR.COM bytes are a pure function of the source. The
 * provenance is THIS file (authored for initech-ap5g; no external binary blob),
 * mutation-proven against the host hardware double in the proof oracle.
 *
 * SCOPE -- only the 18 helpers the engine actually references (verified by
 * `nm` on every engine .o compiled -msoft-float -mno-80387; ADR-0009 confirms
 * the engine already needs the 64-bit integer helpers regardless of float
 * strategy):
 *   soft-float double arithmetic : __adddf3 __subdf3 __muldf3 __divdf3
 *   double compares              : __eqdf2 __nedf2 __ltdf2 __ledf2 __gtdf2 __gedf2
 *   conversions                  : __fixdfsi __fixdfdi __fixunsdfsi
 *                                  __floatsidf __floatdidf __floatunsidf
 *   64-bit integer               : __udivdi3 __umoddi3 __udivmoddi4
 *
 * The IEEE-754 binary64 implementation is the classic "decompose into sign /
 * 11-bit biased exponent / 52-bit mantissa, operate on integers, round-to-
 * nearest-even, recompose" algorithm. It handles: zero (+/-), subnormals,
 * infinity, NaN, and round-half-to-even. It is intentionally compact and
 * auditable; correctness is established by the differential proof oracle, which
 * fuzzes every helper against the host's hardware double over thousands of
 * random + edge-case inputs (Law 2 -- the oracle is the truth).
 *
 * Reproducible (Rule 11): no globals, no timestamps, pure functions. ASCII (12).
 */

#include <stdint.h>

/* ------------------------------------------------------------------------- *
 * IEEE-754 binary64 field access.
 * ------------------------------------------------------------------------- */

typedef union {
    double   d;
    uint64_t u;
} df_bits;

#define DF_SIGN_BIT   0x8000000000000000ULL
#define DF_EXP_MASK   0x7FF0000000000000ULL
#define DF_MAN_MASK   0x000FFFFFFFFFFFFFULL
#define DF_HID_BIT    0x0010000000000000ULL   /* implicit leading 1 (bit 52) */
#define DF_EXP_BIAS   1023
#define DF_EXP_MAX    2047

static inline uint64_t df_to_bits(double d) { df_bits b; b.d = d; return b.u; }
static inline double   df_from_bits(uint64_t u) { df_bits b; b.u = u; return b.d; }

static inline int      df_sign(uint64_t u) { return (int)(u >> 63); }
static inline int      df_exp(uint64_t u)  { return (int)((u >> 52) & 0x7FF); }
static inline uint64_t df_man(uint64_t u)  { return u & DF_MAN_MASK; }

static inline int df_is_nan(uint64_t u) {
    return df_exp(u) == DF_EXP_MAX && df_man(u) != 0;
}
static inline int df_is_inf(uint64_t u) {
    return df_exp(u) == DF_EXP_MAX && df_man(u) == 0;
}

/* A quiet NaN constant (sign 0, exp all-ones, top mantissa bit set). */
#define DF_QNAN  0x7FF8000000000000ULL
#define DF_PINF  0x7FF0000000000000ULL
#define DF_NINF  0xFFF0000000000000ULL

/* ------------------------------------------------------------------------- *
 * Unpacked form: a normalized significand in a 64-bit field with the binary
 * point just below bit 62, plus an unbiased exponent and a sign. Zero/inf/NaN
 * are flagged out-of-band so the core add/mul/div operate only on finite
 * non-zero values.
 * ------------------------------------------------------------------------- */

typedef struct {
    int      sign;     /* 0 or 1 */
    int      exp;      /* unbiased exponent of the MSB of `sig` (bit 62 weight) */
    uint64_t sig;      /* significand; for finite non-zero, bit 62 is the unit */
    int      cls;      /* 0 normal/subnormal-nonzero, 1 zero, 2 inf, 3 nan */
} df_unpacked;

/* Normalize a value into df_unpacked with a single clean convention:
 *   value = (-1)^sign * sig * 2^exp, where sig has its MSB at bit 62 (so sig is
 *   in [2^62, 2^63) for finite non-zero values). Zero/inf/NaN are flagged via
 *   `cls` so the core add/mul/div operate only on finite non-zero significands. */
static df_unpacked df_unpack2(uint64_t u)
{
    df_unpacked r;
    int e = df_exp(u);
    uint64_t m = df_man(u);
    r.sign = df_sign(u);

    if (e == DF_EXP_MAX) { r.cls = m ? 3 : 2; r.exp = 0; r.sig = 0; return r; }
    if (e == 0 && m == 0) { r.cls = 1; r.exp = 0; r.sig = 0; return r; }

    r.cls = 0;
    uint64_t sig;
    int unbiased;
    if (e == 0) {                       /* subnormal */
        sig = m;
        unbiased = 1 - DF_EXP_BIAS - 52; /* value = m * 2^(1-bias-52) */
    } else {
        sig = m | DF_HID_BIT;           /* 53-bit, MSB at bit 52 */
        unbiased = e - DF_EXP_BIAS - 52; /* value = sig * 2^(e-bias-52) */
    }
    /* Move MSB to bit 62: shift left by (62 - position_of_MSB). For a normal
     * the MSB is at 52 -> shift 10 -> exponent decreases by 10. */
    while ((sig & (1ULL << 62)) == 0) {
        sig <<= 1;
        unbiased--;
    }
    r.sig = sig;
    r.exp = unbiased;   /* value = (-1)^sign * sig * 2^exp, sig in [2^62,2^63) */
    return r;
}

/* Round a 64-bit significand (MSB at bit 62) + sticky info back into an IEEE
 * binary64. `extra` carries bits shifted out below bit 0 already folded so the
 * rounding only needs sig's low bits. We pass round/sticky explicitly instead. */
static uint64_t df_round_pack(int sign, int exp, uint64_t sig,
                              uint64_t lost, int lost_bits)
{
    /* sig has MSB at bit 62 (value = sig * 2^exp). value = (sig / 2^62) * 2^(exp+
     * 62), so the value-of-the-MSB exponent is E = exp + 62, i.e. value =
     * 1.frac * 2^E with the leading bit at bit 62. We round exactly ONCE here:
     * double-rounding (round to 53 bits, then re-round into a subnormal) is a
     * 1-ULP bug. We pick the number of mantissa bits to retain from E (normal:
     * 53 incl hidden; subnormal: fewer), find the round/sticky split in `sig`,
     * and round-half-to-even a single time. `lost` (with lost_bits>0) folds any
     * bits dropped BEFORE round_pack (mul/div remainders) into sticky. */
    int E = exp + 62;
    int biased = E + DF_EXP_BIAS;
    uint64_t extra_sticky = (lost_bits > 0 && lost != 0) ? 1ULL : 0ULL;

    /* keepbits = number of significand bits to retain above the round point.
     * Normal range (biased in [1, 2046]): keep 53 bits (hidden + 52). The round
     * bit is bit (62 - 53) = bit 9, sticky = bits [8..0]. For a subnormal
     * (biased <= 0) we keep fewer bits: the result's mantissa has (52 - (1 -
     * biased)) explicit bits and NO hidden bit, so keepbits = 53 - (1 - biased)
     * = 52 + biased. If keepbits <= 0 the value underflows to signed zero. */
    if (biased >= DF_EXP_MAX) {
        return ((uint64_t)sign << 63) | DF_PINF;           /* overflow -> inf */
    }

    if (biased <= 0) {
        /* SUBNORMAL (or underflow). Convention: value = sig * 2^exp with sig's MSB
         * at bit 62. A subnormal encodes value = field * 2^-1074 (field is the
         * 52-bit mantissa, no hidden bit), so field = sig * 2^(exp + 1074), i.e.
         * a RIGHT shift of sig by `shift = -(exp + 1074)` bits. (exp is deeply
         * negative for subnormals, so `shift` is a modest non-negative count for
         * representable subnormals.) This is the exact inverse of df_unpack2's
         * normalization -- verified by the unpack/round_pack round-trip identity
         * in the differential oracle. We round-to-nearest-even at that shift. */
        int shift = -(exp + 1074);     /* bring LSB (weight 2^-1074) into place */
        if (shift >= 64) {
            /* far below the min subnormal: round bit is above sig -> zero (no tie). */
            return ((uint64_t)sign << 63);
        }
        uint64_t field, round_bit, sticky;
        if (shift <= 0) {
            /* should not happen for biased<=0, but guard: value at/above min normal
             * boundary -> treat as the (handled) normal path by falling through is
             * unsafe here; clamp to min subnormal-aligned with no shift. */
            field = sig;
            round_bit = 0;
            sticky = extra_sticky;
        } else {
            field = sig >> shift;
            round_bit = (sig >> (shift - 1)) & 1ULL;
            sticky = ((sig & ((1ULL << (shift - 1)) - 1ULL)) ? 1ULL : 0ULL)
                     | extra_sticky;
        }
        if (round_bit && (sticky || (field & 1ULL)))
            field++;
        /* field may have carried up to 2^52 (-> smallest normal, exp field 1,
         * mantissa 0) -- DF_MAN_MASK masks the mantissa; the +1<<52 from the carry
         * lands in the exponent field automatically. */
        return ((uint64_t)sign << 63) | (((uint64_t)field) & (DF_MAN_MASK | (1ULL << 52)));
    }

    /* NORMAL. Keep 53 significand bits (hidden + 52); round bit at bit 9. */
    uint64_t mant = sig >> 10;                       /* 53 bits, hidden at bit 52 */
    uint64_t round_bit = (sig >> 9) & 1ULL;
    uint64_t sticky = ((sig & 0x1FFULL) ? 1ULL : 0ULL) | extra_sticky;
    if (round_bit && (sticky || (mant & 1ULL)))
        mant++;
    if (mant & (1ULL << 53)) {                       /* hidden-bit overflow */
        mant >>= 1;
        biased++;
        if (biased >= DF_EXP_MAX)
            return ((uint64_t)sign << 63) | DF_PINF;
    }
    return ((uint64_t)sign << 63)
         | ((uint64_t)biased << 52)
         | (mant & DF_MAN_MASK);
}

/* ------------------------------------------------------------------------- *
 * 128-bit helpers for multiply/divide (built from 64-bit integer ops).
 * ------------------------------------------------------------------------- */

typedef struct { uint64_t hi, lo; } u128;

static u128 mul_u64(uint64_t a, uint64_t b)
{
    uint64_t al = a & 0xFFFFFFFFULL, ah = a >> 32;
    uint64_t bl = b & 0xFFFFFFFFULL, bh = b >> 32;
    uint64_t ll = al * bl;
    uint64_t lh = al * bh;
    uint64_t hl = ah * bl;
    uint64_t hh = ah * bh;
    uint64_t mid = (ll >> 32) + (lh & 0xFFFFFFFFULL) + (hl & 0xFFFFFFFFULL);
    u128 r;
    r.lo = (ll & 0xFFFFFFFFULL) | (mid << 32);
    r.hi = hh + (lh >> 32) + (hl >> 32) + (mid >> 32);
    return r;
}

/* ------------------------------------------------------------------------- *
 * Core arithmetic.
 * ------------------------------------------------------------------------- */

static uint64_t df_add_sub(uint64_t ua, uint64_t ub, int sub);

double __adddf3(double a, double b)
{
    return df_from_bits(df_add_sub(df_to_bits(a), df_to_bits(b), 0));
}
double __subdf3(double a, double b)
{
    return df_from_bits(df_add_sub(df_to_bits(a), df_to_bits(b), 1));
}

static uint64_t df_add_sub(uint64_t ua, uint64_t ub, int sub)
{
    if (df_is_nan(ua) || df_is_nan(ub)) return DF_QNAN;

    df_unpacked A = df_unpack2(ua);
    df_unpacked B = df_unpack2(ub);
    if (sub) B.sign ^= 1;

    /* inf handling */
    if (A.cls == 2 || B.cls == 2) {
        if (A.cls == 2 && B.cls == 2) {
            if (A.sign != B.sign) return DF_QNAN;   /* inf - inf */
            return A.sign ? DF_NINF : DF_PINF;
        }
        if (A.cls == 2) return A.sign ? DF_NINF : DF_PINF;
        return B.sign ? DF_NINF : DF_PINF;
    }
    /* zeros */
    if (A.cls == 1 && B.cls == 1) {
        /* -0 + -0 = -0; else +0 (round-to-nearest) */
        return (A.sign && B.sign) ? DF_SIGN_BIT : 0;
    }
    if (A.cls == 1) return ub ^ (sub ? DF_SIGN_BIT : 0);
    if (B.cls == 1) return ua;

    /* Align exponents: bring both significands to the larger exponent. Keep a
     * sticky bit for bits shifted out. */
    int exp;
    uint64_t lostA = 0, lostB = 0;
    if (A.exp >= B.exp) {
        exp = A.exp;
        int sh = A.exp - B.exp;
        if (sh >= 64) { lostB = B.sig; B.sig = 0; }
        else if (sh) { lostB = B.sig & ((1ULL << sh) - 1); B.sig >>= sh; }
    } else {
        exp = B.exp;
        int sh = B.exp - A.exp;
        if (sh >= 64) { lostA = A.sig; A.sig = 0; }
        else if (sh) { lostA = A.sig & ((1ULL << sh) - 1); A.sig >>= sh; }
    }
    uint64_t lost = lostA | lostB;

    int sign;
    uint64_t sig;
    if (A.sign == B.sign) {
        sig = A.sig + B.sig;
        sign = A.sign;
        if (sig & (1ULL << 63)) {       /* carry: shift right 1, exp++ */
            lost |= (sig & 1) ? 1 : 0;
            sig >>= 1;
            exp++;
        }
    } else {
        /* subtract smaller magnitude from larger */
        if (A.sig > B.sig || (A.sig == B.sig && lostA >= lostB)) {
            sig = A.sig - B.sig - ((lostB > lostA) ? 1 : 0);
            sign = A.sign;
        } else {
            sig = B.sig - A.sig - ((lostA > lostB) ? 1 : 0);
            sign = B.sign;
        }
        if (sig == 0 && lost == 0) return 0;   /* exact cancellation -> +0 */
        /* renormalize: MSB back to bit 62 */
        while ((sig & (1ULL << 62)) == 0) {
            sig <<= 1;
            exp--;
            if (sig == 0) break;
        }
    }

    return df_round_pack(sign, exp, sig, lost, 1);
}

double __muldf3(double a, double b)
{
    uint64_t ua = df_to_bits(a), ub = df_to_bits(b);
    if (df_is_nan(ua) || df_is_nan(ub)) return df_from_bits(DF_QNAN);

    df_unpacked A = df_unpack2(ua);
    df_unpacked B = df_unpack2(ub);
    int sign = A.sign ^ B.sign;

    if (A.cls == 2 || B.cls == 2) {        /* inf */
        if (A.cls == 1 || B.cls == 1) return df_from_bits(DF_QNAN); /* inf*0 */
        return df_from_bits(sign ? DF_NINF : DF_PINF);
    }
    if (A.cls == 1 || B.cls == 1)          /* zero */
        return df_from_bits((uint64_t)sign << 63);

    /* multiply significands: each in [2^62,2^63); product in [2^124,2^126). */
    u128 p = mul_u64(A.sig, B.sig);
    int exp = A.exp + B.exp;
    /* Bring MSB to bit 62 of a single 64-bit word. Product MSB is at bit 124 or
     * 125. Take the high 64 bits (bits 64..127) and fold the low 64 into sticky.
     * value = product * 2^exp, product = p.hi*2^64 + p.lo. We want sig with MSB
     * at bit 62, so sig = product >> (msb_pos - 62). */
    /* Find top bit position of p.hi (it is >=124-64=60). */
    uint64_t hi = p.hi;
    int top = 63;
    while ((hi & (1ULL << top)) == 0) top--;
    /* sig should keep MSB at 62; shift hi right by (top-62), folding lost. */
    int rsh = top - 62;
    uint64_t sig, lost;
    if (rsh > 0) {
        lost = (hi & ((1ULL << rsh) - 1)) | (p.lo ? 1 : 0);
        sig = hi >> rsh;
    } else {
        /* top <= 62: bring more bits up from p.lo (rsh<=0 means shift left) */
        int lsh = -rsh;
        sig = (hi << lsh) | (p.lo >> (64 - lsh));
        lost = p.lo & ((1ULL << (64 - lsh)) - 1);
    }
    /* exponent. Convention: value = sig * 2^exp with sig's MSB at bit 62. The
     * integer product P = A.sig*B.sig (= A.sig*2^A.exp * B.sig*2^B.exp / 2^(A.exp+
     * B.exp)) is a 128-bit integer whose MSB is at bit (64+top). Re-anchoring sig
     * to MSB-at-62 means sig = P >> ((64+top) - 62), so
     *   value = sig * 2^((64+top)-62) * 2^(A.exp+B.exp).
     * Hence exp = A.exp + B.exp + (64+top) - 62 = A.exp + B.exp + top + 2. */
    exp = A.exp + B.exp + top + 2;

    return df_from_bits(df_round_pack(sign, exp, sig, lost, 1));
}

double __divdf3(double a, double b)
{
    uint64_t ua = df_to_bits(a), ub = df_to_bits(b);
    if (df_is_nan(ua) || df_is_nan(ub)) return df_from_bits(DF_QNAN);

    df_unpacked A = df_unpack2(ua);
    df_unpacked B = df_unpack2(ub);
    int sign = A.sign ^ B.sign;

    if (A.cls == 2 && B.cls == 2) return df_from_bits(DF_QNAN);   /* inf/inf */
    if (A.cls == 2) return df_from_bits(sign ? DF_NINF : DF_PINF);
    if (B.cls == 2) return df_from_bits((uint64_t)sign << 63);    /* x/inf=0 */
    if (B.cls == 1) {
        if (A.cls == 1) return df_from_bits(DF_QNAN);             /* 0/0 */
        return df_from_bits(sign ? DF_NINF : DF_PINF);            /* x/0=inf */
    }
    if (A.cls == 1) return df_from_bits((uint64_t)sign << 63);    /* 0/x=0 */

    /* Both significands have MSB at bit 62, so the real ratio A.sig/B.sig lies in
     * (0.5, 2). We compute an integer quotient q ~= (A.sig << 62) / B.sig by
     * binary long division: q gets ~63 significant bits, with MSB at bit 61 or 62
     * depending on whether the ratio is < 1 or >= 1. We then anchor q's MSB to
     * bit 62 and set the exponent.
     *
     * Convention: value = q * 2^-62 * 2^(A.exp-B.exp). We generate q one bit at a
     * time from bit 62 down to bit 0, tracking the running remainder. */
    uint64_t den = B.sig;
    uint64_t rem = A.sig;       /* current remainder, < 2^63 (sig < 2^63) */
    uint64_t q = 0;
    int bit;
    for (bit = 62; bit >= 0; bit--) {
        if (rem >= den) { q |= (1ULL << bit); rem -= den; }
        rem <<= 1;              /* bring down the next (zero) numerator bit */
    }
    /* q is the quotient scaled by 2^62: value = q * 2^-62 * 2^(A.exp-B.exp).
     * q's MSB is at bit 61 (ratio<1) or 62 (ratio>=1). rem!=0 => inexact. */
    uint64_t lost = rem ? 1 : 0;
    int top = 62;
    while (top >= 0 && (q & (1ULL << top)) == 0) top--;
    if (top < 0) return df_from_bits((uint64_t)sign << 63);
    /* anchor MSB to bit 62 */
    int lsh = 62 - top;         /* top is 61 or 62 -> lsh is 0 or 1 */
    if (lsh > 0) q <<= lsh;
    /* The long-division loop produced q = floor(A.sig / B.sig) refined bit-by-bit
     * down to bit 0 -- i.e. q is the integer ratio, NOT scaled by 2^62. For
     * A.sig==B.sig this gives q=1<<62 only because A.sig's MSB is at 62; in
     * general q's MSB sits at bit 61 (ratio<1) or 62 (ratio>=1), and after the
     * lsh anchoring q's MSB is at bit 62, with q in [2^62, 2^63). df_round_pack's
     * convention is value = sig * 2^exp where sig (MSB at 62) ~= 2^62, i.e. it
     * folds in a factor 2^62. The ratio's true scale is (A.sig/B.sig)*2^(A.exp-
     * B.exp); since both sigs carry a 2^62 unit, the 2^62 factors cancel in the
     * ratio, so we must subtract the 62 that df_round_pack re-adds:
     *   exp = (A.exp - B.exp) - lsh - 62. */
    int exp = A.exp - B.exp - lsh - 62;

    return df_from_bits(df_round_pack(sign, exp, q, lost, 1));
}

/* ------------------------------------------------------------------------- *
 * Comparisons. libgcc semantics: __eqdf2/__nedf2 return 0 when equal; the
 * ordered compares return -1/0/1; NaN makes equality fail and orderings return
 * a value that makes the generated branch take the "unordered" path. The engine
 * uses the conventional GCC lowering, so we mirror the standard return contract:
 *   __eqdf2(a,b) == 0  iff a == b   (nonzero otherwise)
 *   __nedf2(a,b) != 0  iff a != b
 *   __ltdf2 < 0 iff a < b ; __ledf2 <= 0 iff a <= b
 *   __gtdf2 > 0 iff a > b ; __gedf2 >= 0 iff a >= b
 * For NaN, all return a value indicating "not <, not <=, not >, not >=, not ==".
 * ------------------------------------------------------------------------- */

/* core ordered compare: returns -1 (a<b), 0 (a==b), 1 (a>b), 2 (unordered). */
static int df_cmp(double a, double b)
{
    uint64_t ua = df_to_bits(a), ub = df_to_bits(b);
    if (df_is_nan(ua) || df_is_nan(ub)) return 2;

    int sa = df_sign(ua), sb = df_sign(ub);
    /* +0 == -0 */
    uint64_t ma = ua & ~DF_SIGN_BIT, mb = ub & ~DF_SIGN_BIT;
    if (ma == 0 && mb == 0) return 0;

    if (sa != sb) return sa ? -1 : 1;     /* negative < positive */

    /* same sign: compare magnitude via the raw bits (monotone for same sign). */
    if (ma == mb) return 0;
    int mag = (ma < mb) ? -1 : 1;
    return sa ? -mag : mag;               /* both negative: larger mag is smaller */
}

int __eqdf2(double a, double b) { int c = df_cmp(a, b); return (c == 0) ? 0 : 1; }
int __nedf2(double a, double b) { int c = df_cmp(a, b); return (c == 0) ? 0 : 1; }
int __ltdf2(double a, double b) { int c = df_cmp(a, b); return (c == 2) ? 1 : c; }
int __ledf2(double a, double b) { int c = df_cmp(a, b); return (c == 2) ? 1 : c; }
int __gtdf2(double a, double b) { int c = df_cmp(a, b); return (c == 2) ? -1 : c; }
int __gedf2(double a, double b) { int c = df_cmp(a, b); return (c == 2) ? -1 : c; }

/* ------------------------------------------------------------------------- *
 * Conversions.
 * ------------------------------------------------------------------------- */

/* double -> signed 64-bit (truncate toward zero). */
int64_t __fixdfdi(double a)
{
    uint64_t u = df_to_bits(a);
    if (df_is_nan(u)) return 0;
    int sign = df_sign(u);
    int e = df_exp(u);
    uint64_t m = df_man(u);
    if (e == DF_EXP_MAX) return sign ? INT64_MIN : INT64_MAX;  /* inf */
    if (e < DF_EXP_BIAS) return 0;                              /* |x| < 1 */
    int shift = e - DF_EXP_BIAS;                                /* 0..  */
    uint64_t mant = (e == 0) ? m : (m | DF_HID_BIT);           /* bit 52 = unit */
    int64_t val;
    if (shift >= 63) {
        /* saturate */
        return sign ? INT64_MIN : INT64_MAX;
    }
    if (shift >= 52) val = (int64_t)(mant << (shift - 52));
    else             val = (int64_t)(mant >> (52 - shift));
    return sign ? -val : val;
}

/* double -> signed 32-bit (truncate toward zero). */
int32_t __fixdfsi(double a)
{
    int64_t v = __fixdfdi(a);
    if (v > INT32_MAX) return INT32_MAX;
    if (v < INT32_MIN) return INT32_MIN;
    return (int32_t)v;
}

/* double -> unsigned 32-bit (truncate toward zero; negative -> 0). */
uint32_t __fixunsdfsi(double a)
{
    int64_t v = __fixdfdi(a);
    if (v < 0) return 0;
    if (v > (int64_t)UINT32_MAX) return UINT32_MAX;
    return (uint32_t)v;
}

/* signed 64-bit -> double (round to nearest even). */
double __floatdidf(int64_t v)
{
    if (v == 0) return df_from_bits(0);
    int sign = 0;
    uint64_t a;
    if (v < 0) { sign = 1; a = (uint64_t)(-(v + 1)) + 1ULL; }   /* avoid UB on MIN */
    else a = (uint64_t)v;
    /* normalize MSB to bit 62: value = a * 2^0. */
    int top = 63;
    while ((a & (1ULL << top)) == 0) top--;
    int exp;
    uint64_t sig;
    if (top >= 62) { sig = a >> (top - 62); exp = (top - 62); }
    else           { sig = a << (62 - top); exp = -(62 - top); }
    uint64_t lost = (top > 62) ? (a & ((1ULL << (top - 62)) - 1)) : 0;
    /* value = a = sig * 2^(top-62) ... but sig has MSB at 62, weight 2^(top-62).
     * df_round_pack treats value = sig * 2^exp with sig MSB at 62. So exp must be
     * (top - 62). */
    exp = top - 62;
    return df_from_bits(df_round_pack(sign, exp, sig, lost, 1));
}

/* signed 32-bit -> double (always exact). */
double __floatsidf(int32_t v)
{
    return __floatdidf((int64_t)v);
}

/* unsigned 32-bit -> double (always exact). */
double __floatunsidf(uint32_t v)
{
    if (v == 0) return df_from_bits(0);
    uint64_t a = v;
    int top = 31;
    while ((a & (1ULL << top)) == 0) top--;
    uint64_t sig;
    int exp;
    if (top >= 62) { sig = a >> (top - 62); }
    else           { sig = a << (62 - top); }
    exp = top - 62;
    return df_from_bits(df_round_pack(0, exp, sig, 0, 0));
}

/* ------------------------------------------------------------------------- *
 * 64-bit unsigned integer divide / modulo (the engine's __udivdi3 / __umoddi3).
 * Classic shift-subtract long division.
 * ------------------------------------------------------------------------- */

static uint64_t udivmod64(uint64_t n, uint64_t d, uint64_t *rem)
{
    uint64_t q = 0, r = 0;
    int i;
    if (d == 0) { if (rem) *rem = 0; return ~0ULL; }   /* avoid trap; UB anyway */
    for (i = 63; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1ULL);
        if (r >= d) { r -= d; q |= (1ULL << i); }
    }
    if (rem) *rem = r;
    return q;
}

uint64_t __udivdi3(uint64_t n, uint64_t d)
{
    return udivmod64(n, d, 0);
}

uint64_t __umoddi3(uint64_t n, uint64_t d)
{
    uint64_t r;
    udivmod64(n, d, &r);
    return r;
}

/* __udivmoddi4: combined quotient+remainder. gcc emits THIS (not the separate
 * __udivdi3/__umoddi3) at -Os when one operand pair needs both -- e.g. dec_format's
 * digit extraction (integration fix, ADR-0009 DEC-02; the -Os SAMIR.COM profile).
 * It is exactly udivmod64's contract: return the quotient, store the remainder
 * through *rem (libgcc signature: __udivmoddi4(num, den, uint64_t *rem)). */
uint64_t __udivmoddi4(uint64_t n, uint64_t d, uint64_t *rem)
{
    return udivmod64(n, d, rem);
}
