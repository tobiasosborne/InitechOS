/*
 * os/samir/core/rt.c -- SAMIR freestanding runtime.
 *
 * THE ARTIFACT (CLAUDE.md Law 3): this file MUST compile freestanding
 * (-ffreestanding -nostdlib -fno-stack-protector -fno-pic -std=c11
 *  -Wall -Wextra -Werror -c). No libc includes, no <math.h>.
 *
 * Implements the contract in os/samir/include/samir/rt.h:
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
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S0.3.
 *   - dbase3-decomp/re/mint-results-001.md (rounding, overflow).
 *   - dbase3-decomp/specs/file-formats/ndx.md sec 4.2 (JDN).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 */

#include <stdint.h>

#include "samir/rt.h"

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
 */
static int64_t rt_floor64(double x)
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
int dec_format(double v, int width, int dec, char *out)
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

    /* Round: scale up, add 0.5, floor (= round-half-up = ties toward +inf).
     * Safe for |v| < 2^53 / 10^dec -- well within dBASE N-field range. */
    scaled     = rt_floor64(v * (double)pow10_tbl[dec] + 0.5);
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

    /* Total columns: sign (if neg) + int_digits + (dec > 0 ? 1 + dec : 0) */
    total_need = (neg ? 1 : 0) + int_digits + (dec > 0 ? 1 + dec : 0);

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
    if (emit_digits(out + pos, int_digits, int_part) != 0) {
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
