/*
 * os/ibase/core/rt.c -- IBASE freestanding runtime.
 *
 * THE ARTIFACT (CLAUDE.md CLAUDE.md, The constitution): this file MUST compile freestanding
 * (-ffreestanding -nostdlib -fno-stack-protector -fno-pic -std=c11
 *  -Wall -Wextra -Werror -c). No libc includes, no <math.h>.
 *
 * Implements the contract in os/ibase/include/ibase/rt.h:
 *   1. rt_memcpy/rt_memset/rt_memcmp/rt_strlen/rt_strncmp -- byte utilities.
 *   2. jdn_from_ymd / ymd_from_jdn -- Gregorian <-> Julian Day Number
 *      (Meeus Ch.7; proleptic Gregorian formula).
 *      Verified against minted TOURDATE.NDX values:
 *        JDN(1985-08-05)=2446283, JDN(1985-09-07)=2446316,
 *        JDN(1985-09-23)=2446332, JDN(1999-12-31)=2451544.
 *      [verified: minted goldens, mint-results-001.md and ndx.md sec 4.2]
 *   3. dec_format / dec_parse -- N-field ASCII decimal codec.
 *      Rounding: ties toward +infinity (add 0.5 * 10^dec_places, then cast
 *      to int64_t which truncates toward zero).  Source: minted dBASE III+
 *      transcript -- STR(2.5,2,0)="3", STR(-2.5,2,0)="-2". [verified]
 *      Overflow: '*'-fill (minted OTEST.DBF). [verified]
 *
 * Ref (CLAUDE.md, The constitution):
 *   - docs/plans/IBASE-implementation-plan.md S0.3.
 *   - dbase3-decomp/re/mint-results-001.md (rounding, overflow).
 *   - dbase3-decomp/specs/file-formats/ndx.md sec 4.2 (JDN).
 *
 * ASCII-clean (CLAUDE.md, The constitution). No timestamps / host paths (CLAUDE.md, The constitution).
 */

#include <stdint.h>

#include "ibase/rt.h"
#include "ibase/eval.h"

/* ======================================================================== */
/* 1. Memory / string utilities                                              */
/* ======================================================================== */

void *rt_memcpy(void *dst, const void *src, uint32_t n)
{
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    uint32_t i;
    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

void *rt_memset(void *dst, int c, uint32_t n)
{
    uint8_t  *d = (uint8_t *)dst;
    uint8_t   b = (uint8_t)(c & 0xFF);
    uint32_t  i;
    for (i = 0; i < n; i++) {
        d[i] = b;
    }
    return dst;
}

int rt_memcmp(const void *a, const void *b, uint32_t n)
{
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    uint32_t i;
    for (i = 0; i < n; i++) {
        if (pa[i] != pb[i]) {
            return (pa[i] < pb[i]) ? -1 : 1;
        }
    }
    return 0;
}

uint32_t rt_strlen(const char *s)
{
    uint32_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

int rt_strncmp(const char *a, const char *b, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb) {
            return (ca < cb) ? -1 : 1;
        }
        if (ca == '\0') {
            break;
        }
    }
    return 0;
}

/* III+ interpreter-and-execution-model Sec 3; real abbrev/abbrfull DML
 * mints: >=4 matching prefix letters, short keywords whole, bad suffixes
 * rejected. Only call at grammar keyword positions, never for names/data. */
int rt_kw_match(const char *s, const char *kw)
{
    int i = 0;
    while (s[i] && s[i] != ' ' && s[i] != '\t' && s[i] != '\r' && s[i] != '\n') {
        char c = s[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        if (!kw[i] || c != kw[i]) return 0;
        i++;
    }
    return i && (!kw[i] || i >= 4) ? i : 0;
}

int rt_kw_index(const char *s, const char *list)
{
    int k = 0;
    while (*list != '\0') {
        int n = rt_kw_match(s, list);
        if (n && !s[n]) return k;
        while (*list++ != '\0') { }
        k++;
    }
    return -1;
}

int rt_kw_word(const char *s, const char *list, int *len)
{
    int k = 0;
    while (*list != '\0') {
        int n = rt_kw_match(s, list);
        if (n) {
            if (len) *len = n;
            return k;
        }
        while (*list++ != '\0') { }
        k++;
    }
    return -1;
}

int rt_copy_span(char *dst, uint32_t cap, const char *from, const char *to)
{
    uint32_t n = 0;
    while (from < to && *from != '\0') {
        if (n >= cap) return -1;
        dst[n++] = *from++;
    }
    while (n > 0u && (dst[n - 1] == ' ' || dst[n - 1] == '\t')) n--;
    dst[n] = '\0';
    return (int)n;
}

/* Ref: III+ string-functions.md literals; query/mutation/procedure grammar
 * already treats the three quote states as mutually exclusive. Share only
 * that state transition, including the closing delimiter's literal status. */
int rt_literal_byte(char c, char *quote)
{
    if (*quote) {
        if (c == *quote) *quote = 0;
        return 1;
    }
    if (c == '\'' || c == '"' || c == '[') {
        *quote = c == '[' ? ']' : c;
        return 1;
    }
    return 0;
}

/* Ref: programming-and-io.md WITH lists, and navigation-query-display.md
 * expression lists: commas inside quoted text or function calls stay data. */
int rt_copy_expr(const char **src, char *dst, uint32_t cap)
{
    const char *s = *src;
    uint32_t n = 0;
    char quote = 0;
    int depth = 0;
    while (*s) {
        char c = *s;
        if (!rt_literal_byte(c, &quote)) {
            if (c == '(') depth++;
            else if (c == ')') { if (depth > 0) depth--; }
            else if (c == ',' && depth == 0) break;
        }
        if (n >= cap) return -1;
        dst[n++] = c;
        s++;
    }
    while (n && (dst[n - 1] == ' ' || dst[n - 1] == '\t')) n--;
    dst[n] = 0;
    *src = s;
    return (int)n;
}

/* ======================================================================== */
/* 2. Julian Day Number -- Gregorian calendar (valid 1900-2155)             */
/* ======================================================================== */

/*
 * jdn_from_ymd: Gregorian -> Julian Day Number.
 *
 * Formula: Meeus "Astronomical Algorithms" Ch.7, proleptic Gregorian variant.
 * Verified: JDN(1985-08-05)=2446283; JDN(1985-09-07)=2446316;
 *           JDN(1985-09-23)=2446332; JDN(1999-12-31)=2451544.
 * [verified: re/mint-results-001.md + ndx.md sec 4.2]
 */
int32_t jdn_from_ymd(int32_t y, int32_t m, int32_t d)
{
    int32_t a, yy, mm;

    a  = (14 - m) / 12;
    yy = y + 4800 - a;
    mm = m + 12 * a - 3;

    return d
         + (153 * mm + 2) / 5
         + 365 * yy
         + yy / 4
         - yy / 100
         + yy / 400
         - 32045;
}

/*
 * ymd_from_jdn: Julian Day Number -> Gregorian.
 *
 * Inverse of jdn_from_ymd; same Meeus proleptic Gregorian formula.
 * Valid range: JDN values for 1900-01-01 (2415021) to 2155-12-31 (2508522).
 */
void ymd_from_jdn(int32_t jdn, int32_t *y, int32_t *m, int32_t *d)
{
    int32_t a, b, c, dd, e, mm;

    a  = jdn + 32044;
    b  = (4 * a + 3) / 146097;
    c  = a - (146097 * b) / 4;
    dd = (4 * c + 3) / 1461;
    e  = c - (1461 * dd) / 4;
    mm = (5 * e + 2) / 153;

    *d = e - (153 * mm + 2) / 5 + 1;
    *m = mm + 3 - 12 * (mm / 10);
    *y = 100 * b + dd - 4800 + mm / 10;
}

/* Ref: dates-and-century.md's eight pictures and the real III+ DT1/DT2/DT3/
 * DT6 captures. One field-order table serves DTOC, ? and LIST/DISPLAY; the
 * callers still own allocation, bounded appends and mutation hooks. */
uint32_t rt_date_format(double jdn, unsigned fmt, int century, char *out, int query)
{
    static const uint8_t order[8][3] = {
        {1,2,0}, {0,1,2}, {2,1,0}, {2,1,0},
        {2,1,0}, {2,1,0}, {0,1,2}, {1,2,0}
    };
    static const char separators[] = "/./-/./-";
    int32_t parts[3] = {0,0,0};
    unsigned picture = fmt <= XB_DATE_USA ? fmt : query ? XB_DATE_AMERICAN : XB_DATE_BRITISH;
    uint32_t n = 0;
    int blank = jdn <= 0.0;
    char sep = separators[picture];
    if (!blank) ymd_from_jdn((int32_t)jdn, &parts[0], &parts[1], &parts[2]);
    for (unsigned i = 0; i < 3; i++) {
        unsigned field = order[picture][i];
        int32_t v = parts[field];
        int divisor = !field && century ? 1000 : 10;
        if (!field && !century) v %= 100;
        if (i) out[n++] = sep;
        while (divisor) {
            int digit = v / divisor;
            if (query || divisor != (!field && century ? 1000 : 10)) digit %= 10;
            out[n++] = blank ? ' ' : (char)('0' + digit);
            divisor /= 10;
        }
    }
    return n;
}

/* ======================================================================== */
/* 3. Decimal formatter / parser                                             */
/* ======================================================================== */

/*
 * Powers of 10 as int64, indexed [0..18].  Used for scaling in dec_format
 * and dec_parse so we avoid <math.h> pow(). The N-field in dBASE III+ is
 * at most 20 characters wide with at most 18 decimal places (spec cap),
 * so this table covers all valid inputs.
 */
static const int64_t pow10_tbl[19] = {
    1LL,                    /* 10^0  */
    10LL,                   /* 10^1  */
    100LL,                  /* 10^2  */
    1000LL,                 /* 10^3  */
    10000LL,                /* 10^4  */
    100000LL,               /* 10^5  */
    1000000LL,              /* 10^6  */
    10000000LL,             /* 10^7  */
    100000000LL,            /* 10^8  */
    1000000000LL,           /* 10^9  */
    10000000000LL,          /* 10^10 */
    100000000000LL,         /* 10^11 */
    1000000000000LL,        /* 10^12 */
    10000000000000LL,       /* 10^13 */
    100000000000000LL,      /* 10^14 */
    1000000000000000LL,     /* 10^15 */
    10000000000000000LL,    /* 10^16 */
    100000000000000000LL,   /* 10^17 */
    1000000000000000000LL   /* 10^18 */
};

/*
 * emit_digits: write the decimal representation of the non-negative integer
 * `val` into buf[0..n-1], right-justified, space-padded on the left.
 * `n` is the number of digit columns available (NOT the total width; the
 * caller allocates the full `width` area and calls this only for the digit
 * portion). Returns 0 on success, -1 if `val` does not fit in `n` columns.
 */
static int emit_digits(char *buf, int n, uint64_t val)
{
    int i;
    /* Fill right to left */
    for (i = n - 1; i >= 0; i--) {
        buf[i] = (char)('0' + (int)(val % 10u));
        val /= 10u;
    }
    /* If val still has digits it did not fit */
    return (val == 0u) ? 0 : -1;
}

/*
 * rt_floor64: floor(x) as int64_t, without <math.h>.
 *
 * C casts double->int64_t truncate toward zero.  For x >= 0, truncation IS
 * floor.  For x < 0, truncation is ceiling, so we subtract 1 if and only
 * if the value is not already an integer.
 *
 * Exported (declared in rt.h) so other freestanding IBASE engine files
 * (fn_builtins.c fn_round -- initech-eyig) can share the SAME floor
 * primitive dec_format uses below, instead of re-deriving (and getting
 * wrong) their own truncating cast.
 */
int64_t rt_floor64(double x)
{
    int64_t t = (int64_t)x;            /* truncate toward zero */
    if (x < 0.0 && (double)t != x) {  /* negative non-integer -> floor = t-1 */
        t -= 1;
    }
    return t;
}

/*
 * dec_format: format double v as right-justified decimal ASCII of width chars.
 *
 * Rounding rule (ties toward +infinity) = standard "round half up":
 *   scaled = floor(v * pow10(dec) + 0.5)
 * Examples (all minted [verified: mint-results-001.md]):
 *    2.5 * 1 + 0.5 =  3.0  -> floor( 3.0) =  3  -> " 3"
 *   -2.5 * 1 + 0.5 = -2.0  -> floor(-2.0) = -2  -> "-2"
 *   -100 * 1 + 0.5 = -99.5 -> floor(-99.5)= -100 -> valid
 *
 * Width overflow -> '*'-fill.
 * [verified: minted OTEST.DBF, mint-results-001.md]
 */
/* Exact binary64 -> scaled integer for display. Multiplying in double loses
 * the low bit below a tie (matrixb SQRT(.999999999)); even an x87 extended
 * product can land on a false tie at 18 digits (numdisplay SQRT(10^9)).
 * Multiply the 53-bit significand by 5^dec in three 32-bit limbs, then shift
 * by the binary exponent + dec, retaining guard/sticky bits for nearest-even.
 * The caller has checked that the rounded magnitude fits signed 64 bits.
 * This is digit generation precision, independent of the x87 rounding mode.
 */
static int64_t display_scaled(double v, int dec)
{
    union { double n; uint64_t bits; } u;
    uint32_t limb[3];
    uint64_t significand, carry, result;
    int exponent, i, j, guard = 0, sticky = 0;
    u.n = v;
    exponent = (int)((u.bits >> 52) & 2047u);
    significand = u.bits & 0x000fffffffffffffULL;
    if (exponent) significand |= 0x0010000000000000ULL;
    else exponent = 1;
    exponent += dec - 1075;
    limb[0] = (uint32_t)significand;
    limb[1] = (uint32_t)(significand >> 32);
    limb[2] = 0;
    for (i = 0; i < dec; i++) {
        carry = 0;
        for (j = 0; j < 3; j++) {
            carry += (uint64_t)limb[j] * 5u;
            limb[j] = (uint32_t)carry;
            carry >>= 32;
        }
    }
    if (exponent <= -96) return 0;
    for (; exponent < 0; exponent++) {
        sticky |= guard;
        guard = limb[0] & 1u;
        limb[0] = (limb[0] >> 1) | (limb[1] << 31);
        limb[1] = (limb[1] >> 1) | (limb[2] << 31);
        limb[2] >>= 1;
    }
    result = ((uint64_t)limb[1] << 32) | limb[0];
    result = (result << exponent) + (guard && (sticky || (result & 1u)));
    return v < 0.0 ? -(int64_t)result : (int64_t)result;
}

static int dec_format_core(double v, int width, int dec, char *out, int even)
{
    int      neg;
    int64_t  scaled;
    uint64_t abs_scaled;
    uint64_t int_part;
    uint64_t frac_part;
    int      int_digits;   /* columns for the integer part (digits only) */
    int      total_need;   /* total columns needed: sign + int_digits + '.' + dec */
    int      pad;
    int      pos;

    /* Clamp dec to table range (engine should never pass > 18, but be safe) */
    if (dec < 0)  dec = 0;
    if (dec > 18) dec = 18;

    /* A rounded, scaled magnitude that cannot convert to int64 below
     * (>= 2^63 = 9223372036854775808) is star-filled instead of hitting an
     * undefined double->int64 conversion. Doubles near 2^63 are 1024 apart,
     * so "< 2^63" admits exactly the values up to 2^63-1024 that DO convert
     * (9.2E18 and 9223372036854774784 still print in N(19,0): regression in
     * test-ibase-datatype "boundary"). NaN also star-fills. The value an
     * overflowed "****" field reads back as (1.7976931348623157E308, dbf.c;
     * real III+ datatype-mint DT5) lands here and prints "****" at any
     * width, as real III+ does (DT1 B4). initech-ylqs. */
    {
        double mag = v < 0.0 ? -v : v;
        if (!(mag * (double)pow10_tbl[dec] + 0.5 < 9223372036854775808.0)) {
            int i;
            for (i = 0; i < width; i++) out[i] = '*';
            return width;
        }
    }

    /* Round: scale up, add 0.5, floor (= round-half-up = ties toward +inf).
     * Safe for |v| < 2^53 / 10^dec -- well within dBASE N-field range. */
    if (even == 2) {
        scaled = display_scaled(v, dec);
    } else if (even) {
        /* The DBF/picture conversion uses nearest-even, unlike STR.
         * Real III+ datatype-mint DT6: N1..N4 (+/-1.25 -> +/-1.2),
         * T01/T03 and A3/A4 (binary-exact ties vs values above a tie). */
        double x = v * (double)pow10_tbl[dec];
        double fraction;
        scaled = rt_floor64(x);
        fraction = x - (double)scaled;
        if (fraction > 0.5 || (fraction == 0.5 && scaled % 2 != 0)) scaled++;
    } else {
        scaled = rt_floor64(v * (double)pow10_tbl[dec] + 0.5);
    }
    neg        = (scaled < 0);
    abs_scaled = (uint64_t)(neg ? -scaled : scaled);

    /* Split into integer and fractional parts */
    int_part  = abs_scaled / (uint64_t)pow10_tbl[dec];
    frac_part = abs_scaled % (uint64_t)pow10_tbl[dec];

    /* Count integer columns needed */
    {
        uint64_t tmp = int_part;
        int_digits = 0;
        do {
            int_digits++;
            tmp /= 10u;
        } while (tmp > 0u);
    }

    /* Total columns: sign (if neg) + integer digits + fractional picture. */
    total_need = (neg ? 1 : 0) + int_digits + (dec > 0 ? 1 + dec : 0);

    /* Ref: ../dbase3-decomp/specs/runtime/numeric-and-string-formatting.md
     * Sec 3.4 (total width), and the
     * independent fns-mint/log/GOLD.OUT (-.69) / division/GOLD.OUT (-0.33).
     * A fraction's integer zero is optional only when its full picture
     * does not fit. Preserve it whenever there is room, e.g. -0.6 in four
     * columns; omit it before sacrificing requested decimal places. */
    if (int_part == 0u && dec > 0 && total_need > width) {
        int_digits = 0;
        total_need--;
    }

    /* Overflow check */
    if (total_need > width) {
        int i;
        for (i = 0; i < width; i++) {
            out[i] = '*';
        }
        return width;
    }

    /* Build the output right-justified: spaces then sign then digits */
    pad = width - total_need;
    pos = 0;

    /* Left-pad with spaces */
    {
        int i;
        for (i = 0; i < pad; i++) {
            out[pos++] = ' ';
        }
    }

    /* Sign */
    if (neg) {
        out[pos++] = '-';
    }

    /* Integer digits */
    if (int_digits && emit_digits(out + pos, int_digits, int_part) != 0) {
        /* Should not happen after the total_need check -- but fail safe */
        int i;
        for (i = 0; i < width; i++) {
            out[i] = '*';
        }
        return width;
    }
    pos += int_digits;

    /* Fractional part */
    if (dec > 0) {
        out[pos++] = '.';
        if (emit_digits(out + pos, dec, frac_part) != 0) {
            int i;
            for (i = 0; i < width; i++) {
                out[i] = '*';
            }
            return width;
        }
        pos += dec;
    }

    (void)pos; /* all bytes written */
    return width;
}

int dec_format(double v, int width, int dec, char *out)
{
    return dec_format_core(v, width, dec, out, 0);
}

int dec_format_even(double v, int width, int dec, char *out)
{
    return dec_format_core(v, width, dec, out, 1);
}

int dec_format_fit(double v, int width, int dec, char *out, int ties_even)
{
    /* Real III+ DT2 F12/F13 and E5.DBF: sacrifice fractional precision
     * before star-filling. Retry the original value so a carry can consume
     * another integer column; both APPEND and REPLACE use this codec. */
    if (dec < 0) dec = 0;
    if (dec > 18) dec = 18;
    for (;;) {
        dec_format_core(v, width, dec, out, ties_even);
        if (width <= 0 || out[0] != '*' || dec == 0) return width;
        dec--;
    }
}

/* General display, including callers in the STATUS overlay. This must stay
 * resident: an overlay cannot swap the single slot while returning through
 * its caller (ovl.h). Ref: real III+ matrixb/numfmt/numround captures. */
int dec_format_display(double v, int width, int dec, char *out)
{
    int field = width > 19 ? 19 : width;
    int pad = width - field, exponent = 0, digits, i;
    double mag = v < 0.0 ? -v : v;
    uint64_t mantissa;
    for (i = 0; i < pad; i++) out[i] = ' ';
    out += pad;
    dec_format_fit(v, field, dec, out, 2);
    if (field < 7 || out[0] != '*' || !(mag > 0.0 && mag < 1e99))
        return width;
    while (mag >= 1.0) { mag /= 10.0; exponent++; }
    digits = field - 6;
    /* Scientific output truncates the mantissa; it does not round it.
     * One sign column (a blank for positive), '.', digits and E+nn. */
    mantissa = (uint64_t)(int64_t)(mag * (double)pow10_tbl[digits]);
    out[0] = v < 0.0 ? '-' : ' ';
    out[1] = '.';
    emit_digits(out + 2, digits, mantissa);
    out[digits + 2] = 'E';
    out[digits + 3] = '+';
    out[digits + 4] = (char)('0' + exponent / 10);
    out[digits + 5] = (char)('0' + exponent % 10);
    return width;
}

/*
 * dec_parse: parse a fixed-format ASCII decimal into a double.
 *
 * Leading/trailing spaces skipped; empty/all-spaces -> 0.0.
 * No exponent; handles optional leading '-' or '+'.
 * No libc strtod (freestanding). Simple integer accumulation avoids
 * any fp dependency until the final scaling step.
 */
double dec_parse(const char *s, int len)
{
    int     i    = 0;
    int     end  = len;
    int     neg  = 0;
    int64_t ipart = 0;
    int64_t fpart = 0;
    int     fdec  = 0;
    int     in_frac = 0;
    double  result;

    /* Skip leading spaces */
    while (i < end && s[i] == ' ') { i++; }
    /* Skip trailing spaces */
    while (end > i && s[end - 1] == ' ') { end--; }

    if (i >= end) {
        return 0.0;
    }

    /* Optional sign */
    if (s[i] == '-') { neg = 1; i++; }
    else if (s[i] == '+') { i++; }

    /* Digits */
    for (; i < end; i++) {
        char c = s[i];
        if (c >= '0' && c <= '9') {
            if (!in_frac) {
                ipart = ipart * 10 + (c - '0');
            } else {
                if (fdec < 18) {
                    fpart = fpart * 10 + (c - '0');
                    fdec++;
                }
                /* extra digits beyond 18 are silently dropped */
            }
        } else if (c == '.') {
            in_frac = 1;
        } else {
            /* non-digit/non-point: stop (caller responsible for clean input) */
            break;
        }
    }

    /* Combine: ipart + fpart * 10^-fdec */
    result = (double)ipart;
    if (fdec > 0) {
        result += (double)fpart / (double)pow10_tbl[fdec];
    }
    if (neg) {
        result = -result;
    }
    return result;
}

/* Persistent codec buffers must not participate in interpreter scratch resets.
 * Lifetime policy authored, no local reference. PAL acquire/release contract. */
#include "ibase/pal.h"
#include "ibase/lifetime.h"
typedef struct pal_owned_block { struct pal_owned_block *next; } pal_owned_block;
void *pal_owned_alloc(ibase_pal_t *pal, void **owner, uint32_t n)
{
    pal_owned_block *b;
    ibase_ovl_validate_pal(pal); IBASE_RETAIN(owner);
    if (!pal->acquire || n > 0xffffffffu - sizeof(*b)) return (void *)0;
    b=(pal_owned_block *)pal->acquire(pal,n+(uint32_t)sizeof(*b));
    if (!b) return (void *)0;
    IBASE_RETAIN(b); IBASE_RETAIN(*owner);
    b->next=(pal_owned_block *)*owner; *owner=b; return b+1;
}
void pal_owned_free(ibase_pal_t *pal, void **owner)
{
    pal_owned_block *b=(pal_owned_block *)*owner;
    *owner=(void *)0;
    while(b){pal_owned_block *next=b->next;pal->release(pal,b);b=next;}
}

/* Stream control motion matches console_putc/ANSI; positioned SAY counts
 * literal cells (real III+ at-say-edge-controls/controls). */
void rt_cursor_advance(uint16_t *row,uint16_t *col,const char *s,uint32_t n,int screen)
{
    uint32_t i;
    for(i=0;i<n;i++) {
        if(screen==2) { if(++*col==80) { *col=0;++*row; } }
        else if(s[i]=='\r') *col=0;
        else if(s[i]=='\n') { if(*row<65535u) ++*row;if(screen)*col=0; }
        else if(s[i]=='\f') { *row=*col=0; }
        else if(screen && s[i]=='\a') { }
        else if(screen && s[i]=='\t') {
            *col=(uint16_t)((*col/8+1)*8);
            if(*col>=80) { *col=0;++*row; }
        }
        else if(s[i]=='\b') { if(*col) --*col; }
        else {
            if(*col<65535u) ++*col;
            if(screen && *col==80) { *col=0;++*row; }
        }
        /* Real III+ at-say-edge/position: positioned SAY carries row 25;
         * ordinary scrolling output still stops at row 24. */
        if(screen && *row>(screen==2?25u:24u)) *row=(uint16_t)(screen==2?25u:24u);
    }
}
