/*
 * os/i123/core/fmt.c -- how a cell LOOKS: the 1-2-3 R2.2 display formats, the
 * label prefixes, a worksheet row, the control-panel content, the typed-value
 * parser (THE ARTIFACT).
 *
 * bead: initech-9u8w. See i123.h.
 * Ref (Law 1; every rule below names the screenshot or the record it was
 * read from, and every rule NOT read from the corpus says so):
 *   ../lotus123-decomp/specs/functions/function-value-table.md Sec 3 "Display
 *     facts (General format, column width 9, screenshots)";
 *   ../lotus123-decomp/goldens/minted/MINT0{1,2,3,5}.shots/02_entered.png
 *     (the real 1-2-3 screen; 80 columns, 4-character row border, columns of
 *     9, numbers right-aligned leaving the column's LAST character blank);
 *   ../lotus123-decomp/GAPS.md N-6, N-7 (General truncates; -0.00000).
 *
 * THE COLUMN'S LAST CHARACTER. Every number and right-aligned label in the
 * minted shots ends one character short of the column's right edge (MINT01
 * "123" ends at char 20 of B = chars 13..21; MINT02 "Right" ends at 14 of
 * col A = 4..15, width 12), EXCEPT the Percent sign, which takes that slot
 * (MINT02 "12.5%" ends at char 15). So a number is right-aligned in a FIELD
 * of width-1 characters followed by one trailing character (' ', or '%').
 * Overflow shows width asterisks (MINT05 r12 "*********", width 9).
 *
 * GENERAL (verified, width 9, MINT03/MINT05): the value to 15 significant
 * digits, trailing zeros trimmed (0.30000000000000004 -> "0.3"), then
 * TRUNCATED to the field (@PI -> "3.141592", 39.1666.. -> "39.16666",
 * -0.000001 -> "-0.00000"); scientific when the integer part does not fit
 * (1E10 -> "1.0E+10", 123456789012 -> "1.2E+11", 2^53 -> "9.0E+15", 1E-99 ->
 * "1.0E-99"), which reserves a sign slot (so one mantissa decimal at width 9)
 * and has no room for a three-digit exponent (1E100 -> asterisks).
 * [oracle-resolves] and chosen here: the exponent at which a SMALL value
 * turns scientific (-1E-06 is fixed, 1E-99 scientific; this file switches
 * below -field), and truncation (not rounding) of the scientific mantissa.
 *
 * FIXED / CURRENCY / COMMA / PERCENT (verified at one point each, MINT02:
 * Fixed3 1234.5 -> "1234.500", Currency2 1234.625 -> "$1,234.63" (rounds
 * half away), Percent1 0.125 -> "12.5%"). [inferred, not minted]: thousands
 * separators on Comma, negative Currency/Comma in parentheses with ')' in the
 * trailing slot, a value rounding to zero shown unsigned.
 *
 * DATES (verified one point, MINT02: D1 of 33672 -> "09-Mar-92" = the 1900
 * day serial, with 1900 counted as a leap year). D2 DD-MMM and D3 MMM-YY are
 * in shipped sheets but not on a screenshot [documented: wk1-format.md Sec
 * 4.8]. Formats the corpus never shows (+/-, Hidden is blank, Text, D4, D5,
 * T1..T4) display as General here -- a P1 limitation, recorded, not a claim.
 *
 * Freestanding (x87 doubles in the tenant, SSE doubles on the host oracle --
 * every intermediate is assigned to a double, which -std=c11's
 * -fexcess-precision=standard rounds to binary64). ASCII-clean (Rule 12).
 */
#include "i123.h"
#include "i123/wk1_format.h"

#define FMT_GENERAL ((uint8_t)((WK1_FT_SPECIAL << 4) | WK1_FS_GENERAL))

/* ---- small helpers --------------------------------------------------------- */
static void fill(char *out, uint32_t n, char ch)
{
    for (uint32_t i = 0; i < n; i++) out[i] = ch;
    out[n] = '\0';
}

static uint32_t slen(const char *s)
{
    uint32_t n = 0;
    while (s[n] != '\0') n++;
    return n;
}

/* 10^n for 0 <= n <= 22 is exact in binary64. */
static const double P10[23] = {
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12,
    1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22
};

static double p10(int n)
{
    double r = 1.0;
    while (n > 22) { r = r * 1e22; n -= 22; }
    r = r * P10[n];
    return r;
}

/* Scale a by 10^k (k may be negative), in steps that never overflow first. */
static double scale10(double a, int k)
{
    while (k > 300)  { a = a * p10(300); k -= 300; }
    while (k < -300) { a = a / p10(300); k += 300; }
    if (k >= 0) a = a * p10(k);
    else        a = a / p10(-k);
    return a;
}

/* Write the decimal digits of an integer-valued double 0 <= d < 1e16 into
 * out (no leading zeros except "0"); returns the count. Two 32-bit halves:
 * hi = d / 1e8 truncated is exact for integers below 1e16 (the quotient's
 * fractional part is <= 1 - 1e-8, far above its rounding error), and
 * lo = d - hi*1e8 is an exact integer. No 64-bit division (no libgcc). */
static uint32_t int_digits(double d, char *out)
{
    uint32_t hi = (uint32_t)(d / 1e8);
    double   hd = (double)hi * 1e8;
    uint32_t lo = (uint32_t)(d - hd);
    char tmp[20];
    uint32_t n = 0;
    if (hi == 0u) {
        do { tmp[n++] = (char)('0' + lo % 10u); lo /= 10u; } while (lo != 0u);
    } else {
        for (int i = 0; i < 8; i++) { tmp[n++] = (char)('0' + lo % 10u); lo /= 10u; }
        do { tmp[n++] = (char)('0' + hi % 10u); hi /= 10u; } while (hi != 0u);
    }
    for (uint32_t i = 0; i < n; i++) out[i] = tmp[n - 1u - i];
    out[n] = '\0';
    return n;
}

/* The 15 significant digits of a > 0 (finite), rounded half up, and the
 * decimal exponent e: a ~= d0.d1d2...d14 x 10^e. */
static void sig15(double a, char d[16], int *e_out)
{
    int e = 0;
    double s, r;
    if (a >= 1.0) {
        while (e < 308 && a >= p10(e + 1)) e++;
    } else {
        e = -1;
        while (e > -330 && scale10(a, -e) < 1.0) e--;
    }
    for (int tries = 0; tries < 3; tries++) {
        s = scale10(a, 14 - e);
        r = s + 0.5;
        r = (double)(int64_t)r;
        if (r >= 1e15) { e++; continue; }
        if (r < 1e14)  { e--; continue; }
        break;
    }
    if (r >= 1e15) r = 999999999999999.0;
    if (r < 1e14)  r = 1e14;
    (void)int_digits(r, d);   /* exactly 15 digits */
    *e_out = e;
}

static uint8_t resolve_fmt(const i123_sheet_t *sh, uint8_t fmt)
{
    if (WK1_FMT_TYPE(fmt) == WK1_FT_SPECIAL && WK1_FMT_LOW(fmt) == WK1_FS_DEFAULT)
        return sh->win1[WK1_W1_FMT];
    return fmt;
}

/* Right-align `body` in a field of width-1 and put `trail` in the last slot;
 * asterisks when it does not fit. */
static void place_number(const char *body, char trail, uint32_t width, char *out)
{
    uint32_t n = slen(body), field;
    if (width == 0u) { out[0] = '\0'; return; }
    field = width - 1u;
    if (n > field) { fill(out, width, '*'); return; }
    fill(out, width, ' ');
    for (uint32_t i = 0; i < n; i++) out[field - n + i] = body[i];
    out[field] = trail;
}

/* ---- General ----------------------------------------------------------------- */
static void general(double x, uint32_t width, char *out)
{
    char d[16], s[48];
    uint32_t field, n = 0, nd;
    int e, neg;
    double a;

    if (width == 0u) { out[0] = '\0'; return; }
    field = width - 1u;
    if (x == 0.0) { place_number("0", ' ', width, out); return; }
    neg = x < 0.0;
    a = neg ? -x : x;
    sig15(a, d, &e);
    nd = 15u;
    while (nd > 1u && d[nd - 1u] == '0') nd--;

    /* fixed notation fits? */
    if ((e >= 0 && (uint32_t)(e + 1 + neg) <= field) ||
        (e < 0 && e >= -(int)field)) {
        if (neg) s[n++] = '-';
        if (e >= 0) {
            for (int i = 0; i <= e; i++) s[n++] = (i < 15) ? d[i] : '0';
            if (nd > (uint32_t)(e + 1)) {
                s[n++] = '.';
                for (uint32_t i = (uint32_t)(e + 1); i < nd && n < 47u; i++) s[n++] = d[i];
            }
        } else {
            s[n++] = '0';
            s[n++] = '.';
            for (int i = 0; i < -e - 1 && n < 47u; i++) s[n++] = '0';
            for (uint32_t i = 0; i < nd && n < 47u; i++) s[n++] = d[i];
        }
        /* truncate to the field (General truncates, N-7) */
#ifdef I123_MUT_GENERAL_ROUNDS
        /* MUTANT (Rule 6; test-i123-wk1-roundtrip-mutant): round the last
         * shown digit up instead of truncating -- @PI shows "3.141593" where
         * the real screen (MINT03 shot) shows "3.141592". NEVER in a real
         * build. */
        if (n > field && s[field] >= '5' && s[field - 1u] >= '0' && s[field - 1u] < '9')
            s[field - 1u]++;
#endif
        if (n > field) n = field;
        if (n > 0u && s[n - 1u] == '.') n--;
        s[n] = '\0';
        place_number(s, ' ', width, out);
        return;
    }
    /* scientific: [sign slot] d . ddd E +xx  = 7 + decimals characters */
    {
        int dec = (int)field - 7;
        int ae = e < 0 ? -e : e;
        if (dec < 0 || ae > 99) { fill(out, width, '*'); return; }
        if (neg) s[n++] = '-';
        s[n++] = d[0];
        if (dec > 0) {
            s[n++] = '.';
            for (int i = 1; i <= dec && i < 15; i++) s[n++] = d[i];
        }
        s[n++] = 'E';
        s[n++] = (e < 0) ? '-' : '+';
        s[n++] = (char)('0' + ae / 10);
        s[n++] = (char)('0' + ae % 10);
        s[n] = '\0';
        place_number(s, ' ', width, out);
    }
}

/* ---- Fixed / Currency / Comma / Percent ------------------------------------- */
static void fixed_family(double x, uint32_t type, uint32_t decs, uint32_t width,
                         char *out)
{
    char dig[24], s[48];
    double v = x, a, r;
    uint32_t nd, n = 0, intlen, k;
    int neg;
    char trail = ' ';

    if (type == WK1_FT_PERCENT) v = x * 100.0;
    neg = v < 0.0;
    a = neg ? -v : v;
    a = scale10(a, (int)decs);
    if (!(a < 9.0e15)) { fill(out, width, '*'); return; }
    r = a + 0.5;
    r = (double)(int64_t)r;
    nd = int_digits(r, dig);
    if (r == 0.0) neg = 0;
    /* pad to at least decs+1 digits */
    if (nd < decs + 1u) {
        uint32_t pad = decs + 1u - nd;
        for (int i = (int)nd; i >= 0; i--) dig[(uint32_t)i + pad] = dig[i];
        for (uint32_t i = 0; i < pad; i++) dig[i] = '0';
        nd += pad;
    }
    intlen = nd - decs;
    if (neg && (type == WK1_FT_CURRENCY || type == WK1_FT_COMMA)) {
        s[n++] = '(';
        trail = ')';
    } else if (neg) {
        s[n++] = '-';
    }
    if (type == WK1_FT_CURRENCY) s[n++] = '$';
    for (k = 0; k < intlen; k++) {
        if ((type == WK1_FT_CURRENCY || type == WK1_FT_COMMA) && k > 0u &&
            (intlen - k) % 3u == 0u)
            s[n++] = ',';
        s[n++] = dig[k];
    }
    if (decs > 0u) {
        s[n++] = '.';
        for (k = intlen; k < nd; k++) s[n++] = dig[k];
    }
    s[n] = '\0';
    if (type == WK1_FT_PERCENT) trail = '%';
    place_number(s, trail, width, out);
}

/* ---- dates (1900 day serial, 1900 a leap year as 1-2-3 counts it) ----------- */
static const char *const MON[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static int is_leap(uint32_t y)
{
    return (y % 4u == 0u && y % 100u != 0u) || y % 400u == 0u;
}

static int serial_ymd(double x, uint32_t *y, uint32_t *m, uint32_t *d)
{
    static const uint8_t ML[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int32_t n;
    /* 1 = 01-Jan-1900; 73050 = 31-Dec-2099, the last date 1-2-3 R2 accepts
     * [inferred: @DATE's year range 0..199] */
    if (!(x >= 1.0 && x < 73051.0)) return 0;
    n = (int32_t)x;
    if (n == 60) { *y = 1900; *m = 2; *d = 29; return 1; }   /* the 1900 bug */
    if (n > 60) n--;
    n--;                                                      /* days after 1-Jan-1900 */
    *y = 1900;
    for (;;) {
        int32_t yl = is_leap(*y) ? 366 : 365;
        if (n < yl) break;
        n -= yl;
        (*y)++;
    }
    for (*m = 1; *m <= 12u; (*m)++) {
        int32_t ml = ML[*m - 1u] + ((*m == 2u && is_leap(*y)) ? 1 : 0);
        if (n < ml) break;
        n -= ml;
    }
    *d = (uint32_t)n + 1u;
    return 1;
}

static void date_fmt(double x, uint32_t which, uint32_t width, char *out)
{
    uint32_t y, m, d, n = 0;
    char s[16];
    if (!serial_ymd(x, &y, &m, &d)) { fill(out, width, '*'); return; }
    if (which != WK1_FS_D3) {
        s[n++] = (char)('0' + d / 10u);
        s[n++] = (char)('0' + d % 10u);
        s[n++] = '-';
    }
    s[n++] = MON[m - 1u][0];
    s[n++] = MON[m - 1u][1];
    s[n++] = MON[m - 1u][2];
    if (which != WK1_FS_D2) {
        s[n++] = '-';
        s[n++] = (char)('0' + (y % 100u) / 10u);
        s[n++] = (char)('0' + y % 10u);
    }
    s[n] = '\0';
    place_number(s, ' ', width, out);
}

void i123_format_number(double x, uint8_t fmt, uint32_t width, char *out)
{
    uint32_t t = WK1_FMT_TYPE(fmt), lo = WK1_FMT_LOW(fmt);
    if (width == 0u) { out[0] = '\0'; return; }
    switch (t) {
    case WK1_FT_FIXED:
    case WK1_FT_CURRENCY:
    case WK1_FT_COMMA:
    case WK1_FT_PERCENT:
        fixed_family(x, t, lo, width, out);
        return;
    case WK1_FT_SPECIAL:
        if (lo == WK1_FS_D1 || lo == WK1_FS_D2 || lo == WK1_FS_D3) {
            date_fmt(x, lo, width, out);
            return;
        }
        if (lo == WK1_FS_HIDDEN) { fill(out, width, ' '); return; }
        break;
    default:
        break;
    }
    general(x, width, out);
}

/* ---- cells ------------------------------------------------------------------- */
static double cell_double(const uint8_t v[8])
{
    union { double d; uint8_t b[8]; } u;
    for (int i = 0; i < 8; i++) u.b[i] = v[i];
    return u.d;
}

/* 0 finite, 1 +inf, 2 -inf, 3 NaN -- from the raw bytes (no FPU load, so a
 * signalling NaN is never touched). */
static int raw_class(const uint8_t v[8])
{
    uint32_t ex = ((uint32_t)(v[7] & 0x7Fu) << 4) | (uint32_t)(v[6] >> 4);
    int mant0 = (v[6] & 0x0Fu) == 0u && v[5] == 0u && v[4] == 0u && v[3] == 0u &&
                v[2] == 0u && v[1] == 0u && v[0] == 0u;
    if (ex != 0x7FFu) return 0;
    if (!mant0) return 3;
    return (v[7] & 0x80u) ? 2 : 1;
}

static void left_text(const uint8_t *t, uint32_t n, uint32_t width, char *out,
                      uint32_t *spill)
{
    fill(out, width, ' ');
    for (uint32_t i = 0; i < n && i < width; i++) out[i] = (char)t[i];
    *spill = (n > width) ? n : 0u;
}

void i123_cell_text(const i123_sheet_t *sh, const i123_cell_t *c,
                    uint32_t width, char *out, uint32_t *spill)
{
    const uint8_t *b;
    *spill = 0;
    if (c == (const i123_cell_t *)0 || width == 0u) { fill(out, width, ' '); return; }
    b = sh->pool + c->off;
    switch (c->kind) {
    case I123_K_LABEL: {
        uint8_t pfx = c->len ? b[0] : (uint8_t)WK1_PFX_LEFT;
        const uint8_t *t = b + 1;
        uint32_t n = c->len ? (uint32_t)c->len - 1u : 0u;
        if (pfx == WK1_PFX_REPEAT && n > 0u) {
            /* \ repeats the text to fill the column exactly [documented:
             * wk1-format.md Sec 4.4 names the prefix; display unminted] */
            for (uint32_t i = 0; i < width; i++) out[i] = (char)t[i % n];
            out[width] = '\0';
            return;
        }
        if (pfx == WK1_PFX_RIGHT && n <= width - 1u) {
            fill(out, width, ' ');
            for (uint32_t i = 0; i < n; i++) out[width - 1u - n + i] = (char)t[i];
            return;
        }
        if (pfx == WK1_PFX_CENTER && n <= width) {
            /* centred in the column [inferred: unminted] */
            uint32_t at = (width - n) / 2u;
            fill(out, width, ' ');
            for (uint32_t i = 0; i < n; i++) out[at + i] = (char)t[i];
            return;
        }
        left_text(t, n, width, out, spill);
        return;
    }
    case I123_K_INTEGER: {
        int16_t iv = (int16_t)(uint16_t)(c->v[0] | (c->v[1] << 8));
        i123_format_number((double)iv, resolve_fmt(sh, c->fmt), width, out);
        return;
    }
    case I123_K_NUMBER:
        i123_format_number(cell_double(c->v), resolve_fmt(sh, c->fmt), width, out);
        return;
    case I123_K_FORMULA: {
        int k = raw_class(c->v);
        if (k == 3 && c->has_str) {   /* string result, left like a label (MINT01 D2 "big") */
            left_text(b + c->len, c->slen, width, out, spill);
            return;
        }
        if (k == 1 || k == 3) { place_number("ERR", ' ', width, out); return; }  /* MINT03 r13/r14 */
        if (k == 2) { place_number("NA", ' ', width, out); return; }             /* MINT03 r12 */
        i123_format_number(cell_double(c->v), resolve_fmt(sh, c->fmt), width, out);
        return;
    }
    default:
        fill(out, width, ' ');
        return;
    }
}

static int is_empty(const i123_sheet_t *sh, uint16_t col, uint16_t row)
{
    const i123_cell_t *c = i123_find(sh, col, row);
    return c == (const i123_cell_t *)0 || c->kind == I123_K_BLANK;
}

void i123_render_row(const i123_sheet_t *sh, uint16_t row, uint16_t left,
                     uint32_t nchars, char *line)
{
    char cell[256];
    uint32_t x = 0, col = left;
    fill(line, nchars, ' ');
    while (col < I123_MAX_COLS) {
        uint32_t w = i123_colwidth(sh, (uint16_t)col);
        const i123_cell_t *c;
        uint32_t spill = 0;
        if (w == 0u || x + w > nchars || w > 240u) break;
        c = i123_find(sh, (uint16_t)col, row);
        if (c != (const i123_cell_t *)0) {
            i123_cell_text(sh, c, w, cell, &spill);
            for (uint32_t i = 0; i < w; i++) line[x + i] = cell[i];
            if (spill > w) {
                /* a long label runs on into EMPTY cells to the right, whole
                 * visible columns only [inferred: the classic behaviour, not
                 * on a minted shot] */
                const uint8_t *t = (c->kind == I123_K_FORMULA)
                    ? sh->pool + c->off + c->len : sh->pool + c->off + 1;
                uint32_t pos = w, nx = x + w, nc = col + 1u;
                while (pos < spill && nc < I123_MAX_COLS) {
                    uint32_t w2 = i123_colwidth(sh, (uint16_t)nc);
                    if (nx + w2 > nchars || !is_empty(sh, (uint16_t)nc, row)) break;
                    for (uint32_t i = 0; i < w2 && pos < spill; i++) line[nx + i] = (char)t[pos++];
                    nx += w2;
                    nc++;
                }
            }
        }
        x += w;
        col++;
    }
}

uint32_t i123_col_name(uint16_t col, char *out)
{
    uint32_t n = 0;
    if (col >= 26u) out[n++] = (char)('A' + col / 26u - 1u);
    out[n++] = (char)('A' + col % 26u);
    out[n] = '\0';
    return n;
}

uint32_t i123_addr_text(uint16_t col, uint16_t row, char *out)
{
    char dig[24];
    uint32_t n = i123_col_name(col, out);
    uint32_t k = int_digits((double)row + 1.0, dig);
    for (uint32_t i = 0; i < k; i++) out[n++] = dig[i];
    out[n] = '\0';
    return n;
}

/* The control panel shows a number to its 15 significant digits, trimmed
 * [inferred: 1-2-3's internal precision; no minted shot of a number cell's
 * line 1]. */
static void content_number(double x, char *out, uint32_t cap)
{
    char d[16], s[48];
    uint32_t n = 0, nd;
    int e, neg;
    double a;
    if (x == 0.0) { s[n++] = '0'; goto done; }
    neg = x < 0.0;
    a = neg ? -x : x;
    sig15(a, d, &e);
    nd = 15u;
    while (nd > 1u && d[nd - 1u] == '0') nd--;
    if (neg) s[n++] = '-';
    if (e >= 0 && e < 15) {
        for (int i = 0; i <= e; i++) s[n++] = d[i];
        if (nd > (uint32_t)(e + 1)) {
            s[n++] = '.';
            for (uint32_t i = (uint32_t)(e + 1); i < nd; i++) s[n++] = d[i];
        }
    } else if (e < 0 && e >= -6) {
        s[n++] = '0';
        s[n++] = '.';
        for (int i = 0; i < -e - 1; i++) s[n++] = '0';
        for (uint32_t i = 0; i < nd; i++) s[n++] = d[i];
    } else {
        int ae = e < 0 ? -e : e;
        s[n++] = d[0];
        if (nd > 1u) {
            s[n++] = '.';
            for (uint32_t i = 1; i < nd; i++) s[n++] = d[i];
        }
        s[n++] = 'E';
        s[n++] = e < 0 ? '-' : '+';
        if (ae >= 100) s[n++] = (char)('0' + ae / 100);
        s[n++] = (char)('0' + (ae / 10) % 10);
        s[n++] = (char)('0' + ae % 10);
    }
done:
    s[n] = '\0';
    for (uint32_t i = 0; i + 1u < cap; i++) { out[i] = s[i]; if (s[i] == '\0') return; }
    if (cap) out[cap - 1u] = '\0';
}

void i123_cell_content(const i123_sheet_t *sh, const i123_cell_t *c,
                       char *out, uint32_t cap)
{
    uint32_t n = 0;
    if (cap == 0u) return;
    out[0] = '\0';
    if (c == (const i123_cell_t *)0) return;
    switch (c->kind) {
    case I123_K_LABEL: {
        const uint8_t *b = sh->pool + c->off;
        for (uint32_t i = 0; i < c->len && n + 1u < cap; i++) out[n++] = (char)b[i];
        out[n] = '\0';
        return;
    }
    case I123_K_INTEGER: {
        int16_t iv = (int16_t)(uint16_t)(c->v[0] | (c->v[1] << 8));
        content_number((double)iv, out, cap);
        return;
    }
    case I123_K_NUMBER:
        content_number(cell_double(c->v), out, cap);
        return;
    default:
        /* BLANK shows nothing; a FORMULA's text needs the RPN decompiler,
         * which is P2 (bead initech-94ah) -- line 1 shows the address only */
        return;
    }
}

/* ---- typed values -------------------------------------------------------- */
int i123_parse_value(const char *s, uint32_t len, double *out)
{
    uint32_t i = 0;
    int neg = 0, any = 0, exp10 = 0, eneg = 0, ev = 0, sig = 0;
    double m = 0.0, v;

    if (i < len && (s[i] == '+' || s[i] == '-')) { neg = (s[i] == '-'); i++; }
    while (i < len && s[i] >= '0' && s[i] <= '9') {
        any = 1;
        if (sig < 17) { m = m * 10.0 + (double)(s[i] - '0'); if (m != 0.0) sig++; }
        else exp10++;
        i++;
    }
    if (i < len && s[i] == '.') {
        i++;
        while (i < len && s[i] >= '0' && s[i] <= '9') {
            any = 1;
            if (sig < 17) { m = m * 10.0 + (double)(s[i] - '0'); exp10--; if (m != 0.0) sig++; }
            i++;
        }
    }
    if (!any) return I123_E_VALUE;
    if (i < len && (s[i] == 'e' || s[i] == 'E')) {
        int ed = 0;
        i++;
        if (i < len && (s[i] == '+' || s[i] == '-')) { eneg = (s[i] == '-'); i++; }
        while (i < len && s[i] >= '0' && s[i] <= '9') {
            if (ev < 10000) ev = ev * 10 + (s[i] - '0');
            ed = 1;
            i++;
        }
        if (!ed) return I123_E_VALUE;
        exp10 += eneg ? -ev : ev;
    }
    if (i != len) return I123_E_VALUE;
    {
        /* Scale in 80-bit extended (the x87 format, on the tenant and on the
         * x86 host alike) so a power of ten above 1e22 -- inexact in binary64
         * -- does not cost the typed value its last bit (MINT05 1E99 / 1E-99
         * must store the correctly rounded double, graded byte-exact). */
        long double lv = (long double)m;
        int k = exp10;
        while (k >= 22)  { lv = lv * 1e22L; k -= 22; }
        while (k <= -22) { lv = lv / 1e22L; k += 22; }
        if (k > 0) lv = lv * (long double)P10[k];
        if (k < 0) lv = lv / (long double)P10[-k];
        v = (double)lv;
    }
    /* MINT05: the literal 1E99 is accepted and 1E300 refused by the parser;
     * the exact ceiling is [oracle-resolves] -- here a literal must stay
     * below 1E100, the first magnitude General cannot show. */
    if (!(v < 1e100)) return I123_E_VALUE;
    *out = neg ? -v : v;
    return I123_OK;
}
