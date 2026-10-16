/*
 * os/apps/calc/calc_core.c -- the Calculator engine (THE ARTIFACT; bead
 * initech-tdnl.17). The rules, their ground truth and the mutant knobs are in
 * calc_core.h. Freestanding: no libc, no 64-bit division (uint16 limbs keep
 * every quotient a 32-by-32 one), no floating-point library call.
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#include "calc_core.h"

/* ===========================================================================
 * 1. THE EXACT DECIMAL EXPANSION OF A long double
 * ---------------------------------------------------------------------------
 * |v| = M * 2^k, M the 64-bit significand (x87 extended, explicit integer
 * bit). For k >= 0 the value is the integer M << k; for k < 0 it is
 * (M * 5^-k) / 10^-k. Either way a big integer B in base 2^16 is formed and
 * expanded to decimal by repeated division by 10^4 -- exactly, so the readout
 * rounds the true binary value (ties to even), never an approximation.
 * Sizes: k <= 16320 (64 + 16320 bits) and -k <= 16445 (64 + 16445*log2 5 =
 * 38248 bits) -> at most 2391 limbs, at most 11514 decimal digits.
 * ===========================================================================*/
#define BIG_LIMBS   2400
#define BIG_DIGITS  11600

static uint16_t g_big[BIG_LIMBS];
static char     g_dig[BIG_DIGITS];      /* most significant first, '0'..'9' */

typedef struct ld_bits {
    uint32_t lo, hi;      /* significand */
    uint16_t se;          /* sign << 15 | biased exponent */
} ld_bits_t;

static ld_bits_t ld_split(long double v)
{
    union { long double v; uint8_t b[16]; } u;
    ld_bits_t r;
    for (int i = 0; i < 16; i++) u.b[i] = 0u;
    u.v = v;
    r.lo = (uint32_t)u.b[0] | ((uint32_t)u.b[1] << 8) |
           ((uint32_t)u.b[2] << 16) | ((uint32_t)u.b[3] << 24);
    r.hi = (uint32_t)u.b[4] | ((uint32_t)u.b[5] << 8) |
           ((uint32_t)u.b[6] << 16) | ((uint32_t)u.b[7] << 24);
    r.se = (uint16_t)((uint32_t)u.b[8] | ((uint32_t)u.b[9] << 8));
    return r;
}

/* big *= m (m < 2^16); returns the new limb count. */
static int big_mul_small(int n, uint32_t m)
{
    uint32_t carry = 0u;
    for (int i = 0; i < n; i++) {
        uint32_t t = (uint32_t)g_big[i] * m + carry;
        g_big[i] = (uint16_t)(t & 0xFFFFu);
        carry = t >> 16;
    }
    while (carry != 0u) {
        g_big[n++] = (uint16_t)(carry & 0xFFFFu);
        carry >>= 16;
    }
    return n;
}

/* Expand |v| (finite, non-zero) into g_dig. Returns the digit count; *q is
 * the number of those digits that sit right of the decimal point. */
static int expand(const ld_bits_t *b, int *q)
{
    int exp = (int)(b->se & 0x7FFFu);
    int k = (exp == 0) ? (1 - 16383 - 63) : (exp - 16383 - 63);
    int n = 4;

    g_big[0] = (uint16_t)(b->lo & 0xFFFFu);
    g_big[1] = (uint16_t)(b->lo >> 16);
    g_big[2] = (uint16_t)(b->hi & 0xFFFFu);
    g_big[3] = (uint16_t)(b->hi >> 16);
    while (n > 0 && g_big[n - 1] == 0u) n--;

    if (k >= 0) {
        /* shift left by k bits: whole limbs, then the remainder */
        int limbs = k / 16, bits = k % 16;
        for (int i = n - 1; i >= 0; i--) g_big[i + limbs] = g_big[i];
        for (int i = 0; i < limbs; i++) g_big[i] = 0u;
        n += limbs;
        if (bits != 0) n = big_mul_small(n, (uint32_t)1u << bits);
        *q = 0;
    } else {
        int e5 = -k;
        while (e5 >= 6) { n = big_mul_small(n, 15625u); e5 -= 6; }  /* 5^6 */
        while (e5 > 0)  { n = big_mul_small(n, 5u);     e5 -= 1; }
        *q = -k;
    }

    /* decimal digits, least significant first, 4 at a time */
    int nd = 0;
    while (n > 0) {
        uint32_t rem = 0u;
        for (int i = n - 1; i >= 0; i--) {
            uint32_t cur = (rem << 16) | g_big[i];
            g_big[i] = (uint16_t)(cur / 10000u);
            rem = cur % 10000u;
        }
        while (n > 0 && g_big[n - 1] == 0u) n--;
        for (int j = 0; j < 4; j++) {
            g_dig[nd++] = (char)('0' + (int)(rem % 10u));
            rem /= 10u;
        }
    }
    while (nd > 1 && g_dig[nd - 1] == '0') nd--;      /* leading zeros */
    for (int i = 0, j = nd - 1; i < j; i++, j--) {    /* MS digit first */
        char t = g_dig[i]; g_dig[i] = g_dig[j]; g_dig[j] = t;
    }
    return nd;
}

/* Round the expansion to its first `keep` digits (half to even). Writes the
 * kept digits (possibly one more after a carry) to out; returns their count
 * and sets *carry when the rounding added a leading digit. keep <= 0 rounds
 * against an implicit 0 digit. */
static int round_keep(int nd, int keep, char *out, int *carry)
{
    *carry = 0;
    if (keep >= nd) {
        for (int i = 0; i < nd; i++) out[i] = g_dig[i];
        for (int i = nd; i < keep; i++) out[i] = '0';
        return keep;
    }
    int up;
    if (keep < 0) {
        up = 0;
    } else {
        int d = g_dig[keep] - '0', sticky = 0;
        for (int i = keep + 1; i < nd; i++) if (g_dig[i] != '0') { sticky = 1; break; }
        int last_odd = (keep > 0) ? ((g_dig[keep - 1] - '0') & 1) : 0;
        up = (d > 5) || (d == 5 && (sticky || last_odd));
    }
    if (keep <= 0) {
        if (up) { out[0] = '1'; *carry = 1; return 1; }
        return 0;
    }
    for (int i = 0; i < keep; i++) out[i] = g_dig[i];
    if (up) {
        int i = keep - 1;
        while (i >= 0 && out[i] == '9') out[i--] = '0';
        if (i >= 0) {
            out[i]++;
        } else {
            for (int j = keep; j > 0; j--) out[j] = out[j - 1];
            out[0] = '1';
            *carry = 1;
            return keep + 1;
        }
    }
    return keep;
}

/* ===========================================================================
 * 2. THE READOUT
 * ===========================================================================*/
static int put_str(char *out, int at, const char *s)
{
    while (*s) out[at++] = *s++;
    return at;
}

static int put_int(char *out, int at, int v)
{
    char t[8];
    int n = 0;
    do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v > 0);
    while (n > 0) out[at++] = t[--n];
    return at;
}

static int exp_len(int x)
{
    int n = 1 + (x < 0 ? 1 : 0);   /* "E" and the sign */
    if (x < 0) x = -x;
    do { n++; x /= 10; } while (x > 0);
    return n;
}

void calc_format(long double v, char *out)
{
    ld_bits_t b = ld_split(v);
    int neg = (b.se & 0x8000u) != 0u;
    int expo = (int)(b.se & 0x7FFFu);
    int at = 0;
    char r[48];

    if (expo == 0x7FFF) {
        if ((b.hi & 0x7FFFFFFFu) != 0u || b.lo != 0u) {
            (void)put_str(out, 0, "not a number");
            out[12] = '\0';
            return;
        }
        at = put_str(out, 0, neg ? "-infinity" : "infinity");
        out[at] = '\0';
        return;
    }
    if (b.hi == 0u && b.lo == 0u) {           /* +0 and -0: "0" */
        out[0] = '0'; out[1] = '\0';
        return;
    }

    int q, nd = expand(&b, &q);
    int e = nd - 1 - q;                        /* 10^e <= |v| < 10^(e+1) */
    int s = neg ? 1 : 0;

    /* THE DECISION RECORD: |v| rounded to 12 significant digits (the SANE
     * num2dec record the real Calculator decides on -- probes u01/u02:
     * 99999999999.9 is NOT integral, 99999999999.99 IS; r04: 1/100, whose
     * extended value is 0.00999..., is FIXED ".01"). Its exponent and its
     * integrality choose fixed or scientific; the DIGITS shown are then the
     * exact binary value rounded to the width (v02/v03: 1.00000000005 shows
     * ...01 because its binary value lies above the tie, ...25 shows ...02
     * because its lies below). */
    int e_rec, integral = 1;
    {
        int rc;
        (void)round_keep(nd, CALC_DISPLAY_W, r, &rc);
        e_rec = e + rc;
        if (e_rec < 0) integral = 0;
        for (int i = e_rec + 1; i < CALC_DISPLAY_W; i++)
            if (i >= 0 && r[i] != '0') { integral = 0; break; }
    }

#if defined(CALC_MUT_FIXED_E3)
    int fixed_lo = -3;
#else
    int fixed_lo = -2;
#endif
    if (e_rec >= fixed_lo) {
        if (integral) {
            if (s + e_rec + 1 <= CALC_DISPLAY_W) {
                int carry, L = round_keep(nd, e + 1, r, &carry);
                if (neg) out[at++] = '-';
                for (int i = 0; i < L; i++) out[at++] = r[i];
                out[at] = '\0';
                return;
            }
        } else {
            int intdigits = (e_rec >= 0) ? e_rec + 1 : 0;
            int f = CALC_DISPLAY_W - s - intdigits - 1;
            if (f >= 1) {
                int carry, keep = e + 1 + f;
                int L = round_keep(nd, keep, r, &carry);
                /* r holds the value * 10^f as L digits (an all-zero round
                 * gives L == 0): split f fraction digits off the right. */
                int ip = L - f;
                if (neg) out[at++] = '-';
                for (int i = 0; i < ip; i++) out[at++] = r[i];
                int fs = at;
                out[at++] = '.';
                for (int i = 0; i < f; i++) {
                    int src = ip + i;
                    out[at++] = (src >= 0) ? r[src] : '0';
                }
                while (at > fs + 1 && out[at - 1] == '0') at--;
                if (at == fs + 1) at--;               /* bare point */
                if (at == s) out[at++] = '0';          /* rounded to zero */
                out[at] = '\0';
                return;
            }
        }
    }

    /* SCIENTIFIC: as many significant digits as fit in the width. */
    {
        int carry, L, x;
        int n = CALC_DISPLAY_W - s - exp_len(e_rec) - 1;
        L = round_keep(nd, n, r, &carry);
        x = e + carry;
        if (exp_len(x) != exp_len(e_rec)) {      /* the exponent's width moved */
            n = CALC_DISPLAY_W - s - exp_len(x) - 1;
            L = round_keep(nd, n, r, &carry);
            x = e + carry;
        }
        while (L > 1 && r[L - 1] == '0') L--;
        if (neg) out[at++] = '-';
        out[at++] = r[0];
        if (L > 1) {
            out[at++] = '.';
            for (int i = 1; i < L; i++) out[at++] = r[i];
        }
        out[at++] = 'E';
        if (x < 0) { out[at++] = '-'; x = -x; }
        at = put_int(out, at, x);
        out[at] = '\0';
    }
}

/* ===========================================================================
 * 3. THE STATE MACHINE
 * ===========================================================================*/
static long double entry_value(const calc_t *c)
{
    long double n = 0.0L, p = 1.0L;
    int frac = 0;
    for (int i = 0; i < (int)c->len; i++) {
        char ch = c->entry[i];
        if (ch == '.') { frac = 1; continue; }
        n = n * 10.0L + (long double)(ch - '0');
        if (frac) p *= 10.0L;
    }
    return n / p;                 /* both exact: one correctly rounded divide */
}

static long double apply(long double a, uint8_t op, long double b)
{
    switch (op) {
    case CALC_ADD: return a + b;
    case CALC_SUB: return a - b;
    case CALC_MUL: return a * b;
    case CALC_DIV:
#if defined(CALC_MUT_DIV0_ZERO)
        if (b == 0.0L) return 0.0L;
#endif
        return a / b;
    default:       return b;
    }
}

static void show_acc(calc_t *c)
{
    calc_format(c->acc, c->disp);
}

static void show_entry(calc_t *c)
{
    for (int i = 0; i < (int)c->len; i++) c->disp[i] = c->entry[i];
    c->disp[c->len] = '\0';
}

void calc_init(calc_t *c)
{
    c->acc = 0.0L;
    c->pending = (uint8_t)CALC_NONE;
    c->entering = 0u;
    c->len = 0u;
#if defined(CALC_MUT_PRECEDENCE)
    c->lo_acc = 0.0L;
    c->lo_op = (uint8_t)CALC_NONE;
#endif
    c->entry[0] = '\0';
    c->disp[0] = '0';
    c->disp[1] = '\0';
}

#if defined(CALC_MUT_PRECEDENCE)
/* MUTANT (Rule 6): '*' and '/' bind tighter -- a deferred low-precedence
 * side. NEVER in a real build. */
static long double fold(calc_t *c, long double x, int to_low)
{
    if (c->pending == CALC_MUL || c->pending == CALC_DIV) {
        x = apply(c->acc, c->pending, x);
        c->pending = (uint8_t)CALC_NONE;
    }
    if (c->pending == CALC_ADD || c->pending == CALC_SUB) {
        if (!to_low) {                     /* defer: a '*' or '/' follows */
            c->lo_acc = c->acc;
            c->lo_op = c->pending;
            c->pending = (uint8_t)CALC_NONE;
            return x;
        }
        x = apply(c->acc, c->pending, x);
        c->pending = (uint8_t)CALC_NONE;
    }
    if (to_low && c->lo_op != CALC_NONE) {
        x = apply(c->lo_acc, c->lo_op, x);
        c->lo_op = (uint8_t)CALC_NONE;
    }
    return x;
}
#endif

static void operate(calc_t *c, calc_cmd_t op)
{
    if (c->entering) {
        long double x = entry_value(c);
#if defined(CALC_MUT_PRECEDENCE)
        c->acc = fold(c, x, op == CALC_ADD || op == CALC_SUB || op == CALC_EQ);
#else
        c->acc = (c->pending != CALC_NONE) ? apply(c->acc, c->pending, x) : x;
#endif
        c->entering = 0u;
    }
    /* With no operand typed: an operator replaces the pending one, '='
     * drops it (2+= shows 2; 2+3== shows 5). */
    c->pending = (op == CALC_EQ) ? (uint8_t)CALC_NONE : (uint8_t)op;
    show_acc(c);
}

static void type_char(calc_t *c, char ch)
{
    if (!c->entering) {
        c->entering = 1u;
        c->len = 0u;
    }
    if (ch == '.') {
        for (int i = 0; i < (int)c->len; i++)
            if (c->entry[i] == '.') return;          /* one point (q13) */
        if (c->len == 0u) c->entry[c->len++] = '0';  /* "." -> "0." (q14) */
    } else if (c->len == 1u && c->entry[0] == '0') {
        c->len = 0u;                                 /* "007" -> "7" (q15) */
    }
#if defined(CALC_MUT_ENTRY_CAP14)
    if (c->len >= CALC_ENTRY_MAX + 1) return;
#else
    if (c->len >= CALC_ENTRY_MAX) return;            /* 13 chars (p10/s05) */
#endif
    c->entry[c->len++] = ch;
    c->entry[c->len] = '\0';
    show_entry(c);
}

void calc_press(calc_t *c, calc_cmd_t k)
{
    if (k <= CALC_9) {
        type_char(c, (char)('0' + (int)k));
    } else if (k == CALC_DOT) {
        type_char(c, '.');
    } else if (k >= CALC_ADD && k <= CALC_EQ) {
        operate(c, k);
    } else if (k == CALC_CLEAR) {
        calc_init(c);
    }
}

const char *calc_display(const calc_t *c)
{
    return c->disp;
}

/* ===========================================================================
 * 4. THE KEYBOARD MAP  (message = (vkey << 8) | ascii; os/flair/event.c)
 * ===========================================================================*/
#define CALC_VKEY_ENTER   0x1Cu   /* main Enter and keypad Enter (E0 1C) */
#define CALC_VKEY_NUMLOCK 0x45u   /* the PC key where the Mac keypad has Clear */

calc_cmd_t calc_cmd_from_key(uint32_t message)
{
    char a = (char)(message & 0xFFu);
    uint32_t vk = (message >> 8) & 0xFFu;

    if (a >= '0' && a <= '9') return (calc_cmd_t)(a - '0');
    switch (a) {
    case '.': return CALC_DOT;
    case '+': return CALC_ADD;
    case '-': return CALC_SUB;
    case '*': return CALC_MUL;
    case '/': return CALC_DIV;
    case '=': return CALC_EQ;
    case 'c': case 'C': return CALC_CLEAR;
    default: break;
    }
    if (vk == CALC_VKEY_ENTER) return CALC_EQ;
    if (vk == CALC_VKEY_NUMLOCK) return CALC_CLEAR;
    return CALC_NONE;
}
