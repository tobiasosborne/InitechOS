/*
 * harness/diff/dbf_diff/test_samir_softfp.c
 *   Differential proof oracle for os/samir/boot/softfp.c (ADR-0009 DEC-02).
 *
 * Law 2 GAP closure: softfp.c provides ALL of SAMIR's on-target numerics
 * (-msoft-float -mno-80387 link); the 176 host gates never exercise its code
 * paths (they compile the engine with the host's x87). This oracle closes that
 * gap by testing every one of the 18 helpers against the host's hardware double.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Uses seeded LCG for
 * reproducible randomised inputs (Rule 11 -- no time, no rand(), no OS entropy).
 * ASCII-clean (Rule 12).
 *
 * ------ Duplicate-symbol resolution (HOW THIS LINKS) ----------------------
 * softfp.c defines __adddf3, __ltdf2, etc. -- names that also live in the
 * host's libgcc.a. Two facts make this safe:
 *
 *   1. libgcc.a is a STATIC ARCHIVE. The linker includes only object files
 *      from it that are needed to resolve UNDEFINED references. If the same
 *      symbol is already DEFINED in our own object files (softfp.o), the
 *      linker never pulls that archive member in -- no duplicate-symbol error.
 *
 *   2. The linker processes object files left-to-right BEFORE archives. As
 *      long as we pass softfp.c before -lgcc (or rely on implicit end-of-link
 *      libgcc inclusion), our definition wins.
 *
 * Verification: the /tmp/tsym2.c smoke-test in session notes confirms that a
 * user-defined __adddf3 is called (called=1) rather than the libgcc version.
 *
 * The test calls the softfp helpers through explicitly-declared `extern`
 * prototypes (matching the libgcc ABI signatures). No wrapper renaming needed.
 * No -nostdlib (we need libc's printf). No -lm (we use bit-pattern comparison,
 * not mathlib functions). The host's hardware double is the reference:
 *   reference_result = (native C arithmetic / cast on the host)
 *   softfp_result    = sfp_XXX(...) -- the extern-declared helper
 *
 * ------ Mutant path (SOFTFP_MUTANT) ----------------------------------------
 * Compile with -DSOFTFP_MUTANT to inject a test-side perturbation that forces
 * the reference expectation wrong. Specifically: for __adddf3 tests we flip the
 * expected bit-pattern sign bit, so every addition check expects the WRONG sign.
 * This proves the gate goes RED (Law 2: the oracle bites) without editing
 * softfp.c (its logic is frozen). The mutant is test-side: it corrupts the
 * reference, not the implementation, which is the weaker of the two valid Rule-6
 * mutation forms. A stronger proof (perturbing softfp.c itself via a -D macro)
 * is deferred to a future session when softfp.c adds mutation hooks; this test's
 * structure makes such addition trivial.
 *
 * ------ Compile lines -------------------------------------------------------
 * Normal (should exit 0):
 *   gcc -std=c11 -Wall -Wextra -Werror \
 *       -Iseed \
 *       harness/diff/dbf_diff/test_samir_softfp.c \
 *       os/samir/boot/softfp.c \
 *       -o build/test_samir_softfp
 *   ./build/test_samir_softfp
 *
 * Mutant (should exit 1):
 *   gcc -std=c11 -Wall -Wextra -Werror \
 *       -Iseed -DSOFTFP_MUTANT \
 *       harness/diff/dbf_diff/test_samir_softfp.c \
 *       os/samir/boot/softfp.c \
 *       -o build/test_samir_softfp_mutant
 *   ./build/test_samir_softfp_mutant
 *   # Expected: multiple FAIL lines + exit code 1
 *
 * Ref (Law 1):
 *   ADR-0009 DEC-02 (soft-float vendoring mandate + proof oracle requirement).
 *   CLAUDE.md Law 2 (oracle is truth), Rule 6 (mutation-proven), Rule 11
 *   (deterministic -- seeded LCG), Rule 12 (ASCII).
 *   libgcc internals/soft-fp ABI: GCC source lib/gcc/soft-fp/ (the return-value
 *   contract for comparison helpers is the same used here).
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "test_assert.h"   /* seed/test_assert.h, compiled with -Iseed */

TEST_HARNESS();

/* =========================================================================
 * softfp helper declarations (libgcc ABI).
 * These match the signatures in softfp.c exactly.
 * ========================================================================= */

/* Arithmetic */
extern double __adddf3(double a, double b);
extern double __subdf3(double a, double b);
extern double __muldf3(double a, double b);
extern double __divdf3(double a, double b);

/* Comparisons.
 * __eqdf2: 0 if a==b, nonzero otherwise (NaN -> nonzero).
 * __nedf2: 0 if a==b, nonzero otherwise (same ABI as eqdf2; GCC tests != 0).
 * __ltdf2: < 0 if a < b; NaN -> positive (unordered).
 * __ledf2: <= 0 if a <= b; NaN -> positive (unordered).
 * __gtdf2: > 0 if a > b; NaN -> negative (unordered).
 * __gedf2: >= 0 if a >= b; NaN -> negative (unordered). */
extern int __eqdf2(double a, double b);
extern int __nedf2(double a, double b);
extern int __ltdf2(double a, double b);
extern int __ledf2(double a, double b);
extern int __gtdf2(double a, double b);
extern int __gedf2(double a, double b);

/* Conversions */
extern int32_t  __fixdfsi(double a);
extern int64_t  __fixdfdi(double a);
extern uint32_t __fixunsdfsi(double a);
extern double   __floatsidf(int32_t v);
extern double   __floatdidf(int64_t v);
extern double   __floatunsidf(uint32_t v);

/* 64-bit integer */
extern uint64_t __udivdi3(uint64_t n, uint64_t d);
extern uint64_t __umoddi3(uint64_t n, uint64_t d);
extern uint64_t __udivmoddi4(uint64_t n, uint64_t d, uint64_t *rem);

/* =========================================================================
 * Bit-pattern helpers.
 * ========================================================================= */

static uint64_t to_bits(double d)
{
    uint64_t u;
    memcpy(&u, &d, 8);
    return u;
}

static double from_bits(uint64_t u)
{
    double d;
    memcpy(&d, &u, 8);
    return d;
}

static int is_nan_bits(uint64_t u)
{
    int exp = (int)((u >> 52) & 0x7FFu);
    uint64_t man = u & 0x000FFFFFFFFFFFFFull;
    return (exp == 0x7FF) && (man != 0);
}

/* bits_eq: bit-exact comparison EXCEPT that any NaN == any NaN. */
static int bits_eq(uint64_t got, uint64_t ref)
{
    if (is_nan_bits(got) && is_nan_bits(ref)) return 1;
    return got == ref;
}

/*
 * Under -DSOFTFP_MUTANT we corrupt the REFERENCE bit-pattern for arithmetic
 * checks by flipping the sign bit. This forces every arithmetic comparison to
 * be wrong, proving the gate goes RED without touching softfp.c.
 */
static uint64_t maybe_corrupt(uint64_t ref_bits)
{
#ifdef SOFTFP_MUTANT
    return ref_bits ^ 0x8000000000000000ull;  /* flip sign: reference is wrong */
#else
    return ref_bits;
#endif
}

/* =========================================================================
 * Seeded 64-bit LCG (Knuth/Numerical Recipes; deterministic per Rule 11).
 * Period 2^64; no state other than the single uint64_t seed.
 * ========================================================================= */

static uint64_t g_lcg = 0x123456789ABCDEF0ull;

static uint64_t lcg_next(void)
{
    g_lcg = g_lcg * 6364136223846793005ull + 1442695040888963407ull;
    return g_lcg;
}

/* random double in a wide but finite range (no inf/NaN from this path). */
static double lcg_double(void)
{
    /* take 52 bits for mantissa + a modest exponent range */
    uint64_t raw = lcg_next();
    int exp_biased = (int)((raw >> 52) & 0x3FFu) + 512; /* exp in [512,1535] */
    uint64_t man   = raw & 0x000FFFFFFFFFFFFFull;
    int sign       = (int)(raw >> 63);
    uint64_t bits  = ((uint64_t)sign << 63)
                   | ((uint64_t)exp_biased << 52)
                   | man;
    return from_bits(bits);
}

/* random uint64 value */
static uint64_t lcg_u64(void) { return lcg_next(); }

/* =========================================================================
 * Special bit patterns.
 * ========================================================================= */

#define DF_PINF  0x7FF0000000000000ull
#define DF_NINF  0xFFF0000000000000ull
#define DF_PZERO 0x0000000000000000ull
#define DF_NZERO 0x8000000000000000ull
#define DF_QNAN  0x7FF8000000000000ull
#define DF_SNAN  0x7FF4000000000000ull   /* signalling NaN (also a NaN for us) */

/* smallest positive subnormal */
#define DF_MIN_SUBNORM 0x0000000000000001ull
/* smallest positive normal */
#define DF_MIN_NORMAL  0x0010000000000000ull
/* largest finite */
#define DF_MAX_FINITE  0x7FEFFFFFFFFFFFFFull

static const uint64_t SPECIALS[] = {
    DF_PZERO, DF_NZERO,
    DF_PINF,  DF_NINF,
    DF_QNAN,  DF_SNAN,
    DF_MIN_SUBNORM, DF_MIN_SUBNORM | 0x8000000000000000ull,
    DF_MIN_NORMAL,  DF_MIN_NORMAL  | 0x8000000000000000ull,
    DF_MAX_FINITE,  DF_MAX_FINITE  | 0x8000000000000000ull,
    /* powers of two */
    0x3FF0000000000000ull,   /*  1.0 */
    0xBFF0000000000000ull,   /* -1.0 */
    0x4000000000000000ull,   /*  2.0 */
    0x3FE0000000000000ull,   /*  0.5 */
    /* round-half-to-even boundary: 1.5 (0x3FF8000000000000) */
    0x3FF8000000000000ull,
    /* 2.5 */
    0x4004000000000000ull,
    /* very small normal */
    0x0020000000000000ull,
    /* bit 9 and bit 10 of significand (the round/sticky boundary in softfp) */
    0x3FF0000000000200ull,   /* 1.0 + 1 ulp-at-bit-9 */
    0x3FF0000000000100ull,   /* 1.0 + 1 ulp-at-bit-8 */
};
#define N_SPECIALS ((int)(sizeof(SPECIALS)/sizeof(SPECIALS[0])))

/* =========================================================================
 * Group A: arithmetic (__adddf3 __subdf3 __muldf3 __divdf3)
 * ========================================================================= */

/*
 * arith_check: compare softfp op against host.
 * op: 0=add, 1=sub, 2=mul, 3=div.
 * Returns 1 if match, 0 if mismatch (also records in g_checks/g_fails).
 */
static void arith_check(uint64_t ua, uint64_t ub, int op)
{
    double a = from_bits(ua), b = from_bits(ub);
    double host_ref, sfp_res;
    const char *name;

    switch (op) {
    case 0: host_ref = a + b;       sfp_res = __adddf3(a, b); name = "__adddf3"; break;
    case 1: host_ref = a - b;       sfp_res = __subdf3(a, b); name = "__subdf3"; break;
    case 2: host_ref = a * b;       sfp_res = __muldf3(a, b); name = "__muldf3"; break;
    default:host_ref = a / b;       sfp_res = __divdf3(a, b); name = "__divdf3"; break;
    }

    uint64_t ref_bits = maybe_corrupt(to_bits(host_ref));
    uint64_t got_bits = to_bits(sfp_res);

    g_checks++;
    if (!bits_eq(got_bits, ref_bits)) {
        g_fails++;
        fprintf(stderr,
            "  FAIL %s:%d: %s(0x%016llx, 0x%016llx):\n"
            "        got  0x%016llx\n"
            "        want 0x%016llx\n",
            __FILE__, __LINE__, name,
            (unsigned long long)ua, (unsigned long long)ub,
            (unsigned long long)got_bits, (unsigned long long)ref_bits);
    }
}

static void test_arith(void)
{
    int i, j, op;

    /* All combinations of specials x specials */
    for (op = 0; op < 4; op++) {
        for (i = 0; i < N_SPECIALS; i++) {
            for (j = 0; j < N_SPECIALS; j++) {
                /* skip 0/0 and inf/inf for div -- host may raise FP exception;
                 * softfp returns QNaN/QNaN respectively per spec; we still check */
                arith_check(SPECIALS[i], SPECIALS[j], op);
            }
        }
    }

    /* Random inputs: 1000 per op */
    for (op = 0; op < 4; op++) {
        int k;
        for (k = 0; k < 1000; k++) {
            double a = lcg_double(), b = lcg_double();
            arith_check(to_bits(a), to_bits(b), op);
        }
    }

    /* Rounding-boundary: values where the round bit is exactly set */
    {
        /* 1.0 + 1 ULP (bit 0 of mantissa) = exactly representable */
        double a = 1.0, b = from_bits(0x0000000000000001ull); /* min subnormal */
        arith_check(to_bits(a), to_bits(b), 0);
    }
    {
        /* A value whose addition requires round-half-to-even */
        /* 1.5 + 0.5 = 2.0: exact */
        arith_check(0x3FF8000000000000ull, 0x3FE0000000000000ull, 0);
        /* 0.5 + 0.5 = 1.0: exact */
        arith_check(0x3FE0000000000000ull, 0x3FE0000000000000ull, 0);
    }
}

/* =========================================================================
 * Group B: comparisons (__eqdf2 __nedf2 __ltdf2 __ledf2 __gtdf2 __gedf2)
 * ========================================================================= */

/* sign_of: -1, 0, or 1 */
static int sign_of(int v) { return (v > 0) - (v < 0); }

static void cmp_check(uint64_t ua, uint64_t ub)
{
    double a = from_bits(ua), b = from_bits(ub);

    /* Determine host ordering */
    int nan_involved = is_nan_bits(ua) || is_nan_bits(ub);

    /* __eqdf2: 0 iff equal, nonzero on NaN */
    {
        int sfp = __eqdf2(a, b);
        int ref = (nan_involved) ? 1 : ((a == b) ? 0 : 1);
        g_checks++;
        if ((sfp == 0) != (ref == 0)) {
            g_fails++;
            fprintf(stderr,
                "  FAIL %s:%d: __eqdf2(0x%016llx, 0x%016llx): got %d want %d-class\n",
                __FILE__, __LINE__,
                (unsigned long long)ua, (unsigned long long)ub, sfp, ref);
        }
    }

    /* __nedf2: 0 iff equal (same contract as eqdf2) */
    {
        int sfp = __nedf2(a, b);
        int ref = (nan_involved) ? 1 : ((a == b) ? 0 : 1);
        g_checks++;
        if ((sfp == 0) != (ref == 0)) {
            g_fails++;
            fprintf(stderr,
                "  FAIL %s:%d: __nedf2(0x%016llx, 0x%016llx): got %d want %d-class\n",
                __FILE__, __LINE__,
                (unsigned long long)ua, (unsigned long long)ub, sfp, ref);
        }
    }

    /* __ltdf2: < 0 iff a<b; NaN -> positive (>= 0 means "unordered or >=") */
    {
        int sfp = __ltdf2(a, b);
        g_checks++;
        if (nan_involved) {
            /* NaN: result must be > 0 so "< 0" test is false */
            if (sfp <= 0) {
                g_fails++;
                fprintf(stderr,
                    "  FAIL %s:%d: __ltdf2 with NaN: got %d, want > 0 (unordered)\n",
                    __FILE__, __LINE__, sfp);
            }
        } else {
            int expected_sign = (a < b) ? -1 : (a == b) ? 0 : 1;
            if (sign_of(sfp) != expected_sign) {
                g_fails++;
                fprintf(stderr,
                    "  FAIL %s:%d: __ltdf2(0x%016llx, 0x%016llx): got sign %d want %d\n",
                    __FILE__, __LINE__,
                    (unsigned long long)ua, (unsigned long long)ub,
                    sign_of(sfp), expected_sign);
            }
        }
    }

    /* __ledf2: <= 0 iff a<=b; NaN -> positive */
    {
        int sfp = __ledf2(a, b);
        g_checks++;
        if (nan_involved) {
            if (sfp <= 0) {
                g_fails++;
                fprintf(stderr,
                    "  FAIL %s:%d: __ledf2 with NaN: got %d, want > 0 (unordered)\n",
                    __FILE__, __LINE__, sfp);
            }
        } else {
            /* a <= b iff sfp <= 0 */
            int host_le = (a <= b) ? 1 : 0;
            int sfp_le  = (sfp <= 0) ? 1 : 0;
            if (host_le != sfp_le) {
                g_fails++;
                fprintf(stderr,
                    "  FAIL %s:%d: __ledf2(0x%016llx, 0x%016llx): "
                    "sfp=%d host_le=%d sfp_le=%d\n",
                    __FILE__, __LINE__,
                    (unsigned long long)ua, (unsigned long long)ub,
                    sfp, host_le, sfp_le);
            }
        }
    }

    /* __gtdf2: > 0 iff a>b; NaN -> negative */
    {
        int sfp = __gtdf2(a, b);
        g_checks++;
        if (nan_involved) {
            if (sfp >= 0) {
                g_fails++;
                fprintf(stderr,
                    "  FAIL %s:%d: __gtdf2 with NaN: got %d, want < 0 (unordered)\n",
                    __FILE__, __LINE__, sfp);
            }
        } else {
            int host_gt = (a > b) ? 1 : 0;
            int sfp_gt  = (sfp > 0) ? 1 : 0;
            if (host_gt != sfp_gt) {
                g_fails++;
                fprintf(stderr,
                    "  FAIL %s:%d: __gtdf2(0x%016llx, 0x%016llx): "
                    "sfp=%d host_gt=%d sfp_gt=%d\n",
                    __FILE__, __LINE__,
                    (unsigned long long)ua, (unsigned long long)ub,
                    sfp, host_gt, sfp_gt);
            }
        }
    }

    /* __gedf2: >= 0 iff a>=b; NaN -> negative */
    {
        int sfp = __gedf2(a, b);
        g_checks++;
        if (nan_involved) {
            if (sfp >= 0) {
                g_fails++;
                fprintf(stderr,
                    "  FAIL %s:%d: __gedf2 with NaN: got %d, want < 0 (unordered)\n",
                    __FILE__, __LINE__, sfp);
            }
        } else {
            int host_ge = (a >= b) ? 1 : 0;
            int sfp_ge  = (sfp >= 0) ? 1 : 0;
            if (host_ge != sfp_ge) {
                g_fails++;
                fprintf(stderr,
                    "  FAIL %s:%d: __gedf2(0x%016llx, 0x%016llx): "
                    "sfp=%d host_ge=%d sfp_ge=%d\n",
                    __FILE__, __LINE__,
                    (unsigned long long)ua, (unsigned long long)ub,
                    sfp, host_ge, sfp_ge);
            }
        }
    }
}

static void test_cmp(void)
{
    int i, j;

    /* Specials x specials */
    for (i = 0; i < N_SPECIALS; i++) {
        for (j = 0; j < N_SPECIALS; j++) {
            cmp_check(SPECIALS[i], SPECIALS[j]);
        }
    }

    /* Same value (should be equal) */
    cmp_check(0x3FF0000000000000ull, 0x3FF0000000000000ull);   /* 1.0 == 1.0 */
    cmp_check(DF_PZERO, DF_NZERO);                             /* +0 == -0 */

    /* Random pairs */
    {
        int k;
        for (k = 0; k < 500; k++) {
            double a = lcg_double(), b = lcg_double();
            cmp_check(to_bits(a), to_bits(b));
        }
    }
}

/* =========================================================================
 * Group C: conversions
 * ========================================================================= */

static void conv_check_fixdfsi(uint64_t ua)
{
    double a = from_bits(ua);
    int32_t sfp = __fixdfsi(a);
    int32_t ref;

    /* Reproduce host truncation toward zero with saturation */
    if (is_nan_bits(ua)) {
        ref = 0;
    } else if (a >= (double)INT32_MAX + 1.0) {
        ref = INT32_MAX;
    } else if (a <= (double)INT32_MIN - 1.0) {
        ref = INT32_MIN;
    } else {
        ref = (int32_t)a;    /* C truncation toward zero */
    }

#ifdef SOFTFP_MUTANT
    /* corrupt reference to prove gate bites */
    ref = ~ref;
#endif

    g_checks++;
    if (sfp != ref) {
        g_fails++;
        fprintf(stderr,
            "  FAIL %s:%d: __fixdfsi(0x%016llx [%g]): got %d want %d\n",
            __FILE__, __LINE__,
            (unsigned long long)ua, a, sfp, ref);
    }
}

static void conv_check_fixdfdi(uint64_t ua)
{
    double a = from_bits(ua);
    int64_t sfp = __fixdfdi(a);
    int64_t ref;

    if (is_nan_bits(ua)) {
        ref = 0;
    } else if (ua == DF_PINF || a >= 9.223372036854776e18 /* 2^63 */) {
        ref = INT64_MAX;
    } else if (ua == DF_NINF || a <= -9.223372036854776e18) {
        ref = INT64_MIN;
    } else {
        ref = (int64_t)a;
    }

    g_checks++;
    if (sfp != ref) {
        g_fails++;
        fprintf(stderr,
            "  FAIL %s:%d: __fixdfdi(0x%016llx [%g]): got %lld want %lld\n",
            __FILE__, __LINE__,
            (unsigned long long)ua, a,
            (long long)sfp, (long long)ref);
    }
}

static void conv_check_fixunsdfsi(uint64_t ua)
{
    double a = from_bits(ua);
    uint32_t sfp = __fixunsdfsi(a);
    uint32_t ref;

    if (is_nan_bits(ua) || a < 0.0) {
        ref = 0;
    } else if (a >= (double)UINT32_MAX + 1.0) {
        ref = UINT32_MAX;
    } else {
        ref = (uint32_t)a;
    }

    g_checks++;
    if (sfp != ref) {
        g_fails++;
        fprintf(stderr,
            "  FAIL %s:%d: __fixunsdfsi(0x%016llx [%g]): got %u want %u\n",
            __FILE__, __LINE__,
            (unsigned long long)ua, a, sfp, ref);
    }
}

static void conv_check_floatsidf(int32_t v)
{
    double sfp = __floatsidf(v);
    double ref = (double)v;
    uint64_t ref_bits = maybe_corrupt(to_bits(ref));
    uint64_t sfp_bits = to_bits(sfp);

    g_checks++;
    if (!bits_eq(sfp_bits, ref_bits)) {
        g_fails++;
        fprintf(stderr,
            "  FAIL %s:%d: __floatsidf(%d): got 0x%016llx want 0x%016llx\n",
            __FILE__, __LINE__, v,
            (unsigned long long)sfp_bits, (unsigned long long)ref_bits);
    }
}

static void conv_check_floatdidf(int64_t v)
{
    double sfp = __floatdidf(v);
    double ref = (double)v;
    uint64_t ref_bits = maybe_corrupt(to_bits(ref));
    uint64_t sfp_bits = to_bits(sfp);

    g_checks++;
    if (!bits_eq(sfp_bits, ref_bits)) {
        g_fails++;
        fprintf(stderr,
            "  FAIL %s:%d: __floatdidf(%lld): got 0x%016llx want 0x%016llx\n",
            __FILE__, __LINE__, (long long)v,
            (unsigned long long)sfp_bits, (unsigned long long)ref_bits);
    }
}

static void conv_check_floatunsidf(uint32_t v)
{
    double sfp = __floatunsidf(v);
    double ref = (double)v;
    uint64_t ref_bits = maybe_corrupt(to_bits(ref));
    uint64_t sfp_bits = to_bits(sfp);

    g_checks++;
    if (!bits_eq(sfp_bits, ref_bits)) {
        g_fails++;
        fprintf(stderr,
            "  FAIL %s:%d: __floatunsidf(%u): got 0x%016llx want 0x%016llx\n",
            __FILE__, __LINE__, v,
            (unsigned long long)sfp_bits, (unsigned long long)ref_bits);
    }
}

static void test_conversions(void)
{
    int k;

    /* --- fixdfsi --- */
    {
        /* Specials */
        conv_check_fixdfsi(DF_PZERO);
        conv_check_fixdfsi(DF_NZERO);
        conv_check_fixdfsi(DF_PINF);
        conv_check_fixdfsi(DF_NINF);
        conv_check_fixdfsi(DF_QNAN);
        conv_check_fixdfsi(to_bits( 1.0));
        conv_check_fixdfsi(to_bits(-1.0));
        conv_check_fixdfsi(to_bits( 0.9999999));
        conv_check_fixdfsi(to_bits(-0.9999999));
        conv_check_fixdfsi(to_bits((double)INT32_MAX));
        conv_check_fixdfsi(to_bits((double)INT32_MIN));
        conv_check_fixdfsi(to_bits((double)INT32_MAX + 1.0));  /* saturate */
        conv_check_fixdfsi(to_bits((double)INT32_MIN - 1.0));  /* saturate */
        conv_check_fixdfsi(to_bits(1.5));
        conv_check_fixdfsi(to_bits(-1.5));
        conv_check_fixdfsi(to_bits(2147483647.0));
        conv_check_fixdfsi(to_bits(-2147483648.0));
        /* Random */
        for (k = 0; k < 500; k++) {
            /* Use lcg to generate values in [-2^32, 2^32] range */
            int32_t iv = (int32_t)(lcg_u64() >> 32);
            conv_check_fixdfsi(to_bits((double)iv));
        }
    }

    /* --- fixdfdi --- */
    {
        conv_check_fixdfdi(DF_PZERO);
        conv_check_fixdfdi(DF_NZERO);
        conv_check_fixdfdi(DF_PINF);
        conv_check_fixdfdi(DF_NINF);
        conv_check_fixdfdi(DF_QNAN);
        conv_check_fixdfdi(to_bits(1.0));
        conv_check_fixdfdi(to_bits(-1.0));
        conv_check_fixdfdi(to_bits(0.5));
        conv_check_fixdfdi(to_bits(-0.5));
        conv_check_fixdfdi(to_bits((double)INT64_MAX));   /* large; saturates */
        conv_check_fixdfdi(to_bits((double)INT64_MIN));
        conv_check_fixdfdi(to_bits(1e18));
        conv_check_fixdfdi(to_bits(-1e18));
        /* subnormal -> 0 */
        conv_check_fixdfdi(DF_MIN_SUBNORM);
    }

    /* --- fixunsdfsi --- */
    {
        conv_check_fixunsdfsi(DF_PZERO);
        conv_check_fixunsdfsi(DF_NZERO);
        conv_check_fixunsdfsi(DF_PINF);
        conv_check_fixunsdfsi(DF_NINF);
        conv_check_fixunsdfsi(DF_QNAN);
        conv_check_fixunsdfsi(to_bits(-1.0));       /* negative -> 0 */
        conv_check_fixunsdfsi(to_bits(1.0));
        conv_check_fixunsdfsi(to_bits((double)UINT32_MAX));
        conv_check_fixunsdfsi(to_bits((double)UINT32_MAX + 1.0));  /* saturate */
        conv_check_fixunsdfsi(to_bits(0.9));        /* truncate to 0 */
        for (k = 0; k < 300; k++) {
            uint32_t uv = (uint32_t)(lcg_u64() >> 32);
            conv_check_fixunsdfsi(to_bits((double)uv));
        }
    }

    /* --- floatsidf --- */
    {
        conv_check_floatsidf(0);
        conv_check_floatsidf(1);
        conv_check_floatsidf(-1);
        conv_check_floatsidf(INT32_MAX);
        conv_check_floatsidf(INT32_MIN);
        conv_check_floatsidf(1000000);
        conv_check_floatsidf(-1000000);
        for (k = 0; k < 500; k++) {
            conv_check_floatsidf((int32_t)(lcg_u64() >> 32));
        }
    }

    /* --- floatdidf --- */
    {
        conv_check_floatdidf(0);
        conv_check_floatdidf(1);
        conv_check_floatdidf(-1);
        conv_check_floatdidf(INT64_MAX);
        conv_check_floatdidf(INT64_MIN);
        conv_check_floatdidf(1000000000000LL);
        conv_check_floatdidf(-1000000000000LL);
        /* values requiring rounding (>53 significant bits) */
        conv_check_floatdidf((int64_t)0x7FFFFFFFFFFFFFFFLL);
        conv_check_floatdidf((int64_t)0x4000000000000001LL); /* needs rounding */
        for (k = 0; k < 500; k++) {
            conv_check_floatdidf((int64_t)lcg_u64());
        }
    }

    /* --- floatunsidf --- */
    {
        conv_check_floatunsidf(0u);
        conv_check_floatunsidf(1u);
        conv_check_floatunsidf(UINT32_MAX);
        conv_check_floatunsidf(0x80000000u);
        conv_check_floatunsidf(0x7FFFFFFFu);
        for (k = 0; k < 500; k++) {
            conv_check_floatunsidf((uint32_t)(lcg_u64() >> 32));
        }
    }
}

/* =========================================================================
 * Group D: 64-bit integer (__udivdi3 __umoddi3 __udivmoddi4)
 * ========================================================================= */

static void udiv_check(uint64_t n, uint64_t d)
{
    uint64_t q_div, r_mod;
    uint64_t r_dm;
    uint64_t q_dm;

    if (d == 0) {
        /* softfp returns ~0ULL and rem=0 for d==0 (avoid-trap contract).
         * We don't call the host with d==0 (UB). Just verify the helpers
         * don't crash and return the contract value. */
        q_div = __udivdi3(n, 0);
        r_mod = __umoddi3(n, 0);
        q_dm  = __udivmoddi4(n, 0, &r_dm);
        g_checks++;
        if (q_div != ~0ULL) {
            g_fails++;
            fprintf(stderr,
                "  FAIL %s:%d: __udivdi3(%llu,0): got 0x%016llx want ~0\n",
                __FILE__, __LINE__,
                (unsigned long long)n, (unsigned long long)q_div);
        }
        g_checks++;
        if (r_mod != 0) {
            g_fails++;
            fprintf(stderr,
                "  FAIL %s:%d: __umoddi3(%llu,0): got %llu want 0\n",
                __FILE__, __LINE__,
                (unsigned long long)n, (unsigned long long)r_mod);
        }
        (void)q_dm; (void)r_dm;
        return;
    }

    uint64_t ref_q = n / d;
    uint64_t ref_r = n % d;

    /* __udivdi3 */
    {
        q_div = __udivdi3(n, d);
        g_checks++;
        if (q_div != ref_q) {
            g_fails++;
            fprintf(stderr,
                "  FAIL %s:%d: __udivdi3(%llu,%llu): got %llu want %llu\n",
                __FILE__, __LINE__,
                (unsigned long long)n, (unsigned long long)d,
                (unsigned long long)q_div, (unsigned long long)ref_q);
        }
    }

    /* __umoddi3 */
    {
        r_mod = __umoddi3(n, d);
        g_checks++;
        if (r_mod != ref_r) {
            g_fails++;
            fprintf(stderr,
                "  FAIL %s:%d: __umoddi3(%llu,%llu): got %llu want %llu\n",
                __FILE__, __LINE__,
                (unsigned long long)n, (unsigned long long)d,
                (unsigned long long)r_mod, (unsigned long long)ref_r);
        }
    }

    /* __udivmoddi4 */
    {
        q_dm = __udivmoddi4(n, d, &r_dm);
        g_checks++;
        if (q_dm != ref_q) {
            g_fails++;
            fprintf(stderr,
                "  FAIL %s:%d: __udivmoddi4(%llu,%llu) quot: got %llu want %llu\n",
                __FILE__, __LINE__,
                (unsigned long long)n, (unsigned long long)d,
                (unsigned long long)q_dm, (unsigned long long)ref_q);
        }
        g_checks++;
        if (r_dm != ref_r) {
            g_fails++;
            fprintf(stderr,
                "  FAIL %s:%d: __udivmoddi4(%llu,%llu) rem: got %llu want %llu\n",
                __FILE__, __LINE__,
                (unsigned long long)n, (unsigned long long)d,
                (unsigned long long)r_dm, (unsigned long long)ref_r);
        }
        /* consistency: q*d + r == n */
        g_checks++;
        if (q_dm * d + r_dm != n) {
            g_fails++;
            fprintf(stderr,
                "  FAIL %s:%d: __udivmoddi4 consistency: q*d+r != n "
                "(n=%llu d=%llu q=%llu r=%llu)\n",
                __FILE__, __LINE__,
                (unsigned long long)n, (unsigned long long)d,
                (unsigned long long)q_dm, (unsigned long long)r_dm);
        }
    }
}

static void test_udiv(void)
{
    int k;

    /* Edge cases */
    udiv_check(0, 1);
    udiv_check(1, 1);
    udiv_check(UINT64_MAX, 1);
    udiv_check(UINT64_MAX, UINT64_MAX);
    udiv_check(UINT64_MAX, 2);
    udiv_check(UINT64_MAX, 3);
    udiv_check(100, 10);
    udiv_check(100, 7);
    udiv_check(1000000007ull, 1000003ull);
    udiv_check(0, 0);           /* d=0 guard */
    udiv_check(1, 0);
    udiv_check(UINT64_MAX, 0);
    /* Powers of two */
    udiv_check(1ull << 32, 1ull << 16);
    udiv_check(1ull << 63, 1ull << 31);
    /* Large values */
    udiv_check(0xFFFFFFFFFFFFFFFEull, 0xFFFFFFFFFFFFFFFFull);
    udiv_check(0x8000000000000000ull, 3);

    /* Random pairs */
    for (k = 0; k < 2000; k++) {
        uint64_t n = lcg_u64();
        uint64_t d = lcg_u64();
        /* occasionally produce small divisors to exercise the loop path well */
        if (k % 10 == 0) d = (d & 0xFFFF) + 1;
        if (d == 0) d = 1;  /* exclude d=0 from random (already covered above) */
        udiv_check(n, d);
    }
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(void)
{
#ifdef SOFTFP_MUTANT
    printf("test_samir_softfp (MUTANT): reference corrupted -- expects failures\n");
#endif

    test_arith();
    test_cmp();
    test_conversions();
    test_udiv();

    return TEST_SUMMARY("test_samir_softfp");
}
