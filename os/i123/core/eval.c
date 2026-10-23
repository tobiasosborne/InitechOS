/*
 * os/i123/core/eval.c -- the formula EVALUATOR and the @function library
 * (THE ARTIFACT).
 *
 * bead: initech-94ah (I123 P2). See i123.h; token table spec/i123/wk1_formula.h.
 *
 * Ref (Law 1). [C] = ../lotus123-decomp/specs/formulas/wk1-formula-bytecode.md,
 * [V] = ../lotus123-decomp/specs/functions/function-value-table.md,
 * [M] = Lotus 1-2-3 Release 2.2 Reference Manual (scanned; OCR'd by this lane,
 * build/man/ocr/pNNN.txt; cited as "p. <manual page> (PDF <n>)"). Every rule
 * below names its source; a rule no source settles says [inferred].
 *
 * VALUES. A number is an IEEE binary64 ([V] Sec 4: 0.1+0.2 caches
 * 0.30000000000000004, no decimal correction). ERR and NA are values that
 * "ripple through" every formula that uses them ([M] p. 13-11 (PDF 299),
 * p. 13-31 (PDF 319), p. 13-45 (PDF 333)); a result that is not finite is
 * ERR ([V] 1/0 caches +inf = ERR). When ERR and NA meet in one operation the
 * result here is ERR [inferred: GAPS G-6 leaves it open]. A label used as a
 * number is 0 ([M] p. 13-31 (PDF 319): "+A2+34 ... equals 34 if A2 contains
 * the label ERR"); so is a blank cell in math, logical and financial
 * functions ([M] p. 13-4 (PDF 292)). A literal or computed string in
 * arithmetic is ERR [inferred]. The comparison operators compare two strings
 * ignoring case ([M] p. 13-31 (PDF 319): @EXACT is "a more precise
 * alternative to the = operator", distinguishing case); a string against a
 * number compares the string as 0 [inferred].
 *
 * NUMERIC MECHANISM (ADR-0009 Amendment DEC-01a, x87 hardware float; its
 * Sec 4 left the control word to this bead). Every + - * / result is
 * assigned to a double (-std=c11 rounds the excess precision there). The
 * tenant runs with the kernel's boot word 0x037F (64-bit significand, round
 * to nearest; face_a.c loads it explicitly), settled by DIFFERENTIAL in
 * test-i123-formula: the oracle built for x87 under 0x037F reproduces all
 * 1090 golden formulas exactly as the SSE host build does, and under 0x027F
 * (53-bit) the cached values are ALSO all reproduced -- the goldens do not
 * discriminate the arithmetic -- but the literal parser (fmt.c, scaling in
 * extended) then stores 1E99 one ulp off MINT05's bytes, so 0x027F is
 * rejected. The transcendental functions run on the x87 itself (FSQRT,
 * FYL2X, F2XM1, FSCALE, FSIN, FCOS, FPTAN, FPATAN, FPREM) in extended
 * precision, then round once to binary64. No libm (freestanding).
 *
 * Freestanding; ASCII-clean (Rule 12); deterministic (Rule 11).
 */
#include "i123.h"
#include "i123/wk1_format.h"
#include "i123/wk1_formula.h"

extern const i123_token_t i123_tokens[WKF_NTOKENS];

/* ---- x87 primitives (inline asm: the same instructions on the i386 tenant
 * and the x86-64 host oracle) ---------------------------------------------- */
static long double x87_sqrt(long double x)
{ __asm__ ("fsqrt" : "+t"(x)); return x; }

static long double x87_fyl2x(long double y, long double x)   /* y * log2(x) */
{
    long double r;
    __asm__ ("fyl2x" : "=t"(r) : "0"(x), "u"(y) : "st(1)");
    return r;
}

static long double x87_ln2(void)  { long double r; __asm__ ("fldln2" : "=t"(r)); return r; }
static long double x87_lg2(void)  { long double r; __asm__ ("fldlg2" : "=t"(r)); return r; }
static long double x87_l2e(void)  { long double r; __asm__ ("fldl2e" : "=t"(r)); return r; }
static long double x87_pi(void)   { long double r; __asm__ ("fldpi"  : "=t"(r)); return r; }

static long double x87_rndint(long double x)   /* round to nearest (CW RC=00) */
{ __asm__ ("frndint" : "+t"(x)); return x; }

static long double x87_scale(long double x, long double n)   /* x * 2^trunc(n) */
{
    __asm__ ("fscale" : "+t"(x) : "u"(n));
    return x;
}

static long double x87_f2xm1(long double x)    /* 2^x - 1, |x| <= 1 */
{ __asm__ ("f2xm1" : "+t"(x)); return x; }

/* 2^t for finite t (the caller bounds |t|). */
static long double x87_exp2(long double t)
{
    long double n = x87_rndint(t);
    long double f = t - n;                       /* |f| <= 0.5 */
    return x87_scale(x87_f2xm1(f) + 1.0L, n);
}

static long double x87_sin(long double x) { __asm__ ("fsin" : "+t"(x)); return x; }
static long double x87_cos(long double x) { __asm__ ("fcos" : "+t"(x)); return x; }

static long double x87_tan(long double x)
{
    long double one;
    __asm__ ("fptan" : "=t"(one), "=u"(x) : "0"(x));
    (void)one;
    return x;
}

static long double x87_atan2(long double y, long double x)   /* atan(y/x), 4 quadrant */
{
    long double r;
    __asm__ ("fpatan" : "=t"(r) : "0"(x), "u"(y) : "st(1)");
    return r;
}

/* x REM y with the sign of x (FPREM truncates the quotient): exact. */
static long double x87_fprem(long double x, long double y)
{
    unsigned short sw;
    do {
        __asm__ ("fprem\n\tfnstsw %1" : "+t"(x), "=a"(sw) : "u"(y));
    } while (sw & 0x0400u);
    return x;
}

/* ---- small helpers ---------------------------------------------------------- */
static int finite_(double x) { return x == x && x - x == 0.0; }

static double trunc_(double x)
{
    if (!(x < 4.5e15 && x > -4.5e15)) return x;   /* already integral */
    return (double)(int64_t)x;
}

static int up(int ch) { return (ch >= 'a' && ch <= 'z') ? ch - 'a' + 'A' : ch; }
static int lo(int ch) { return (ch >= 'A' && ch <= 'Z') ? ch - 'A' + 'a' : ch; }

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

static double rd_double(const uint8_t *p)
{
    union { double d; uint8_t b[8]; } u;
    for (int i = 0; i < 8; i++) u.b[i] = p[i];
    return u.d;
}

/* The raw class of 8 cache bytes: 0 finite, 1 +inf (ERR), 2 -inf (NA),
 * 3 NaN (string) -- from the bytes, never an FPU load ([C]/wk1-format.md
 * Sec 4.7; a signalling NaN must not be touched). */
static int raw_class(const uint8_t v[8])
{
    uint32_t ex = ((uint32_t)(v[7] & 0x7Fu) << 4) | (uint32_t)(v[6] >> 4);
    int mant0 = (v[6] & 0x0Fu) == 0u && v[5] == 0u && v[4] == 0u && v[3] == 0u &&
                v[2] == 0u && v[1] == 0u && v[0] == 0u;
    if (ex != 0x7FFu) return 0;
    if (!mant0) return 3;
    return (v[7] & 0x80u) ? 2 : 1;
}

static void set_num(i123_val_t *v, double x)
{
    v->cell = 0;
    if (!finite_(x)) { v->t = I123_V_ERR; return; }
    v->t = I123_V_NUM;
    v->n = x;
}

static void set_err(i123_val_t *v) { v->t = I123_V_ERR; v->cell = 0; }
static void set_na(i123_val_t *v)  { v->t = I123_V_NA;  v->cell = 0; }

static int is_error(const i123_val_t *v) { return v->t == I123_V_ERR || v->t == I123_V_NA; }

/* ERR wins over NA [inferred, GAPS G-6]. */
static int pick_error(const i123_val_t *a, const i123_val_t *b, i123_val_t *out)
{
    if (a->t == I123_V_ERR || b->t == I123_V_ERR) { set_err(out); return 1; }
    if (a->t == I123_V_NA || b->t == I123_V_NA) { set_na(out); return 1; }
    return 0;
}

/* ---- cells ------------------------------------------------------------------- */
void i123_cell_value(const i123_sheet_t *sh, uint16_t col, uint16_t row, i123_val_t *v)
{
    const i123_cell_t *c = i123_find(sh, col, row);
    const uint8_t *b;
    v->cell = 1;
    v->slen = 0;
    v->s = (const uint8_t *)0;
    v->n = 0.0;
    if (c == (const i123_cell_t *)0 || c->kind == I123_K_BLANK) { v->t = I123_V_BLANK; return; }
    b = sh->pool + c->off;
    switch (c->kind) {
    case I123_K_INTEGER:
        v->t = I123_V_NUM;
        v->n = (double)(int16_t)rd16(c->v);
        return;
    case I123_K_NUMBER:
        if (raw_class(c->v)) {
            v->t = raw_class(c->v) == 2 ? I123_V_NA : I123_V_ERR;
            return;
        }
        v->t = I123_V_NUM;
        v->n = rd_double(c->v);
        return;
    case I123_K_LABEL:
        /* the text without its prefix character */
        v->t = I123_V_STR;
        v->s = b + 1;
        v->slen = (uint16_t)(c->len ? c->len - 1u : 0u);
        return;
    case I123_K_FORMULA:
        switch (raw_class(c->v)) {
        case 0: v->t = I123_V_NUM; v->n = rd_double(c->v); return;
        case 1: v->t = I123_V_ERR; return;
        case 2: v->t = I123_V_NA; return;
        default:
            if (c->has_str) { v->t = I123_V_STR; v->s = b + c->len; v->slen = c->slen; }
            else v->t = I123_V_ERR;
            return;
        }
    default:
        v->t = I123_V_BLANK;
        return;
    }
}

/* A value as a number for arithmetic: NUM as is; BLANK and a cell's string 0
 * ([M] p. 13-4, p. 13-31); a literal/computed string ERR [inferred]. Returns
 * 0 and fills *err when the value is an error. */
static int as_num(const i123_val_t *v, double *x, i123_val_t *err)
{
    switch (v->t) {
    case I123_V_NUM:   *x = v->n; return 1;
    case I123_V_BLANK: *x = 0.0;  return 1;
    case I123_V_STR:
        if (v->cell) { *x = 0.0; return 1; }
        set_err(err);
        return 0;
    case I123_V_NA:    set_na(err); return 0;
    default:           set_err(err); return 0;
    }
}

/* A value as a string for string functions: a BLANK cell is ERR ([M] p.
 * 13-13 (PDF 301)); a number is ERR [M p. 13-20 @CODE "refers to a blank
 * cell or a value, @CODE returns the value ERR"]. */
static int as_str(const i123_val_t *v, i123_val_t *err)
{
    if (v->t == I123_V_STR) return 1;
    if (v->t == I123_V_NA) { set_na(err); return 0; }
    set_err(err);
    return 0;
}

/* ---- the string arena --------------------------------------------------------- */
static uint8_t *arena_take(i123_calc_t *cx, uint32_t n)
{
    uint8_t *p;
    if (n > I123_STR_MAX || cx->arena_used + n > I123_ARENA) return (uint8_t *)0;
    p = cx->arena + cx->arena_used;
    cx->arena_used += n;
    return p;
}

/* Make *out a string of n bytes copied from src (may alias the arena). */
static void set_str(i123_calc_t *cx, i123_val_t *out, const uint8_t *src, uint32_t n)
{
    uint8_t *p = arena_take(cx, n);
    if (p == (uint8_t *)0 && n != 0u) { set_err(out); return; }
    for (uint32_t i = 0; i < n; i++) p[i] = src[i];
    out->t = I123_V_STR;
    out->cell = 0;
    out->s = p;
    out->slen = (uint16_t)n;
}

/* ---- comparisons --------------------------------------------------------------- */

/* -1 / 0 / 1 */
static int cmp_str(const i123_val_t *a, const i123_val_t *b)
{
    uint32_t n = a->slen < b->slen ? a->slen : b->slen;
    for (uint32_t i = 0; i < n; i++) {
        int x = up(a->s[i]), y = up(b->s[i]);
        if (x != y) return x < y ? -1 : 1;
    }
    if (a->slen == b->slen) return 0;
    return a->slen < b->slen ? -1 : 1;
}

static void op_compare(uint8_t t, const i123_val_t *a, const i123_val_t *b, i123_val_t *out)
{
    int c;
    if (pick_error(a, b, out)) return;
    if (a->t == I123_V_STR && b->t == I123_V_STR) {
        c = cmp_str(a, b);
    } else {
        double x = (a->t == I123_V_NUM) ? a->n : 0.0;
        double y = (b->t == I123_V_NUM) ? b->n : 0.0;
        c = (x < y) ? -1 : (x > y) ? 1 : 0;
    }
    switch (t) {
    case WKF_EQ: c = (c == 0); break;
    case WKF_NE: c = (c != 0); break;
    case WKF_LE: c = (c <= 0); break;
    case WKF_GE: c = (c >= 0); break;
    case WKF_LT: c = (c < 0);  break;
    default:     c = (c > 0);  break;
    }
    set_num(out, c ? 1.0 : 0.0);
}

/* A condition is true when it is a nonzero number; blank and strings are
 * zero ([M] p. 13-36 (PDF 324) @IF). */
static int truth(const i123_val_t *v)
{
    return v->t == I123_V_NUM && v->n != 0.0;
}

/* ---- powers, logs, exponentials -------------------------------------------------- */
static void fn_pow(double x, double y, i123_val_t *out)
{
    long double r;
    if (y == 0.0) { set_num(out, 1.0); return; }
    if (x == 0.0) { if (y > 0.0) set_num(out, 0.0); else set_err(out); return; }
    if (y == trunc_(y) && y <= 1024.0 && y >= -1024.0) {
        /* integral exponent: binary powering in extended, one rounding
         * (MINT05 2^53 = 9007199254740992 exactly) */
        long double b = (long double)x, acc = 1.0L;
        int32_t k = (int32_t)(y < 0.0 ? -y : y);
        while (k) { if (k & 1) acc *= b; b *= b; k >>= 1; }
        r = (y < 0.0) ? 1.0L / acc : acc;
        set_num(out, (double)r);
        return;
    }
    if (x < 0.0) { set_err(out); return; }   /* no real root [inferred] */
    {
        long double t = x87_fyl2x((long double)y, (long double)x);
        if (!(t < 16384.0L)) { set_err(out); return; }        /* overflow: ERR */
        if (!(t > -16384.0L)) { set_num(out, 0.0); return; }  /* underflow: 0 */
        r = x87_exp2(t);
        set_num(out, (double)r);
    }
}

/* ---- the evaluation stack ---------------------------------------------------------- */
typedef struct {
    i123_calc_t        *cx;
    const i123_sheet_t *sh;
    uint16_t            col, row;
} ev_t;

static void resolve_ref(const ev_t *e, const uint8_t *p, int32_t *c, int32_t *r)
{
    *c = i123_ref_resolve(rd16(p), e->col, 1);
    *r = i123_ref_resolve(rd16(p + 2), e->row, 0);
    /* -1 is the ERR reference (ffff ffff, MOVE01C.WK1 E2 caches +inf;
     * initech-nn46); the callers test the row */
    if (*c < 0) *r = -1;
}

/* ---- ranges ------------------------------------------------------------------------ */

/* Visit every POPULATED cell of a range in row-major order (the store's
 * order) -- statistical functions ignore blanks ([M] p. 13-12 (PDF 300)). */
typedef int (*cell_fn)(void *ctx, const i123_cell_t *c, const i123_val_t *v);

/* The first store index whose row is >= r (the store is row-major). */
static uint32_t first_in_row(const i123_sheet_t *sh, uint16_t r)
{
    uint32_t lo = 0, hi = sh->ncells;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2u;
        if (sh->cells[mid].row < r) lo = mid + 1u; else hi = mid;
    }
    return lo;
}

static int range_each(const i123_sheet_t *sh, const i123_val_t *rg, cell_fn fn, void *ctx)
{
    for (uint32_t i = first_in_row(sh, rg->r0); i < sh->ncells; i++) {
        const i123_cell_t *c = &sh->cells[i];
        i123_val_t v;
        if (c->row < rg->r0) continue;
        if (c->row > rg->r1) break;
        if (c->col < rg->c0 || c->col > rg->c1 || c->kind == I123_K_BLANK) continue;
        i123_cell_value(sh, c->col, c->row, &v);
        if (fn(ctx, c, &v)) return 1;
    }
    return 0;
}

/* ---- statistical lists ([M] p. 13-11..13-12 (PDF 299-300)) ------------------------
 * Strings count as 0 and are included; blanks in ranges are ignored; a lone
 * cell argument counts even when blank (@COUNT, p. 13-21 (PDF 309)); ERR and
 * NA ripple through, except in @COUNT which counts them. */
typedef struct {
    uint8_t  fn;
    double   sum, sumsq, mn, mx;
    uint32_t n;
    uint8_t  err;      /* 0 none, 1 ERR, 2 NA */
} stat_t;

static void stat_add(stat_t *s, const i123_val_t *v)
{
    double x = 0.0;
    if (v->t == I123_V_ERR || v->t == I123_V_NA) {
        if (s->fn != 82u) {
            if (v->t == I123_V_ERR) s->err = 1;
            else if (s->err == 0u) s->err = 2;
        }
        s->n++;
        return;
    }
    if (v->t == I123_V_NUM) x = v->n;
    if (s->n == 0u || x < s->mn) s->mn = x;
    if (s->n == 0u || x > s->mx) s->mx = x;
    s->sum = s->sum + x;
    s->n++;
}

static int stat_cell(void *ctx, const i123_cell_t *c, const i123_val_t *v)
{
    (void)c;
    stat_add((stat_t *)ctx, v);
    return 0;
}

static void fn_stat(const ev_t *e, uint8_t t, const i123_val_t *a, uint32_t argc, i123_val_t *out)
{
    stat_t s;
    s.fn = t; s.sum = 0.0; s.sumsq = 0.0; s.mn = 0.0; s.mx = 0.0; s.n = 0; s.err = 0;
    for (uint32_t i = 0; i < argc; i++) {
        if (a[i].t == I123_V_RANGE) (void)range_each(e->sh, &a[i], stat_cell, &s);
        else stat_add(&s, &a[i]);
    }
    if (s.err == 1u) { set_err(out); return; }
    if (s.err == 2u) { set_na(out); return; }
    switch (t) {
    case 80u: set_num(out, s.sum); return;                         /* @SUM */
    case 81u:                                                      /* @AVG */
        if (s.n == 0u) { set_err(out); return; }
        set_num(out, s.sum / (double)s.n);
        return;
    case 82u: set_num(out, (double)s.n); return;                   /* @COUNT */
    case 83u: set_num(out, s.mn); return;                          /* @MIN */
    case 84u: set_num(out, s.mx); return;                          /* @MAX */
    default: {                                                     /* @VAR @STD */
        /* population (n) method, [M] p. 13-55 / 13-62 (PDF 343 / 350) */
        double avg, acc = 0.0;
        if (s.n == 0u) { set_err(out); return; }
        avg = s.sum / (double)s.n;
        /* second pass over the same arguments */
        for (uint32_t i = 0; i < argc; i++) {
            if (a[i].t == I123_V_RANGE) {
                for (uint32_t k = first_in_row(e->sh, a[i].r0); k < e->sh->ncells; k++) {
                    const i123_cell_t *c = &e->sh->cells[k];
                    i123_val_t v;
                    double x, d;
                    if (c->row > a[i].r1) break;
                    if (c->col < a[i].c0 || c->col > a[i].c1 || c->kind == I123_K_BLANK) continue;
                    i123_cell_value(e->sh, c->col, c->row, &v);
                    x = (v.t == I123_V_NUM) ? v.n : 0.0;
                    d = x - avg;
                    acc = acc + d * d;
                }
            } else {
                double x = (a[i].t == I123_V_NUM) ? a[i].n : 0.0, d = x - avg;
                acc = acc + d * d;
            }
        }
        acc = acc / (double)s.n;
        if (t == 88u) acc = (double)x87_sqrt((long double)acc);
        set_num(out, acc);
        return;
    }
    }
}

/* ---- database @functions ([M] p. 13-5..13-6 (PDF 293-294), Chapter 4
 * "Writing Criteria" p. 4-23..4-26 (PDF 97-100)) -----------------------------
 * The criteria range's top row holds field names copied from the input
 * range's top row; each row below is one OR alternative whose non-blank
 * cells must all match (AND). A label criterion matches with ? and * and
 * a leading ~ (negation), ignoring case (the default collating sequence),
 * never a blank record cell; a number matches equal values; a formula is
 * evaluated as written for the FIRST record, its relative references
 * shifted to each record. A blank criteria row selects every record. */
static int wild(const uint8_t *p, uint32_t pn, const uint8_t *s, uint32_t sn)
{
    uint32_t i = 0, k = 0;
    while (i < pn) {
        if (p[i] == '*') return 1;                 /* "matches all characters to the end" */
        if (k >= sn) return 0;
        if (p[i] != '?' && up(p[i]) != up(s[k])) return 0;
        i++; k++;
    }
    return k == sn;
}

static int db_match_cell(const ev_t *e, const i123_cell_t *cc, const i123_val_t *crit,
                         uint16_t fcol, uint16_t rec_row, uint16_t first_row)
{
    i123_val_t rv;
    i123_cell_value(e->sh, fcol, rec_row, &rv);
    if (cc->kind == I123_K_FORMULA) {
        i123_val_t r;
        int32_t shift = (int32_t)rec_row - (int32_t)first_row;
        int32_t hr = (int32_t)cc->row + shift;
        i123_calc_t *sub = e->cx->nested;
        /* a nested evaluation needs its own stack: the caller's spare
         * context (one level; a criteria formula inside a criteria
         * formula's @D function is ERR -- authored limit) */
        if (sub == (i123_calc_t *)0 || hr < 0 || hr >= (int32_t)I123_MAX_ROWS) return -1;
        sub->nested = (i123_calc_t *)0;
        sub->have_now = e->cx->have_now; sub->now = e->cx->now;
        if (i123_eval(sub, e->sh, cc->col, (uint16_t)hr, e->sh->pool + cc->off, cc->len, &r) != I123_OK)
            return -1;
        return truth(&r);
    }
    if (crit->t == I123_V_STR) {
        int neg = crit->slen > 0u && crit->s[0] == '~';
        int m;
        if (rv.t != I123_V_STR) return 0;           /* never a blank or value */
        m = wild(crit->s + neg, crit->slen - (uint32_t)neg, rv.s, rv.slen);
        return neg ? !m : m;
    }
    if (crit->t == I123_V_NUM) return rv.t == I123_V_NUM && rv.n == crit->n;
    return 1;
}

static void fn_db(const ev_t *e, uint8_t t, const i123_val_t *a, i123_val_t *out)
{
    const i123_val_t *in = &a[0], *cr = &a[2];
    double fo;
    uint16_t fcol;
    stat_t s;
    if (in->t != I123_V_RANGE || cr->t != I123_V_RANGE) { set_err(out); return; }
    if (!as_num(&a[1], &fo, out)) return;
    fo = trunc_(fo);
    if (fo < 0.0 || fo > (double)(in->c1 - in->c0)) { set_err(out); return; }  /* [M] PDF 294 */
    fcol = (uint16_t)(in->c0 + (uint16_t)fo);
    s.fn = (uint8_t)(t == 93u ? 82u : t == 91u ? 80u : t == 92u ? 81u : t == 94u ? 83u :
                     t == 95u ? 84u : t == 96u ? 87u : 88u);
    s.sum = 0.0; s.sumsq = 0.0; s.mn = 0.0; s.mx = 0.0; s.n = 0; s.err = 0;
    {
        for (uint32_t pass = 0; pass < 2u; pass++) {
            double avg = s.n ? s.sum / (double)s.n : 0.0, acc = 0.0;
            for (uint32_t r = (uint32_t)in->r0 + 1u; r <= in->r1; r++) {
                int any_row = 0, sel = 0;
                for (uint32_t cr_r = (uint32_t)cr->r0 + 1u; cr_r <= cr->r1 && !sel; cr_r++) {
                    int ok = 1, nonblank = 0;
                    for (uint32_t cc = cr->c0; cc <= cr->c1 && ok; cc++) {
                        const i123_cell_t *ccell = i123_find(e->sh, (uint16_t)cc, (uint16_t)cr_r);
                        i123_val_t name, crit;
                        uint16_t fc = 0xFFFFu;
                        if (ccell == (const i123_cell_t *)0 || ccell->kind == I123_K_BLANK) continue;
                        nonblank = 1;
                        i123_cell_value(e->sh, (uint16_t)cc, (uint16_t)cr->r0, &name);
                        if (name.t == I123_V_STR) {
                            for (uint32_t ic = in->c0; ic <= in->c1; ic++) {
                                i123_val_t h;
                                i123_cell_value(e->sh, (uint16_t)ic, in->r0, &h);
                                if (h.t == I123_V_STR && cmp_str(&h, &name) == 0) { fc = (uint16_t)ic; break; }
                            }
                        }
                        i123_cell_value(e->sh, (uint16_t)cc, (uint16_t)cr_r, &crit);
                        if (ccell->kind != I123_K_FORMULA && fc == 0xFFFFu) continue;  /* unknown field */
                        {
                            int m = db_match_cell(e, ccell, &crit, fc == 0xFFFFu ? (uint16_t)cc : fc,
                                                  (uint16_t)r, (uint16_t)(in->r0 + 1u));
                            if (m < 0) { set_err(out); return; }
                            if (!m) ok = 0;
                        }
                    }
                    if (!nonblank) { sel = 1; any_row = 1; break; }   /* a blank row: all */
                    if (ok) sel = 1;
                }
                (void)any_row;
                if (!sel) continue;
                {
                    i123_val_t v;
                    i123_cell_value(e->sh, fcol, (uint16_t)r, &v);
                    if (v.t == I123_V_BLANK) continue;   /* @DCOUNT "counts nonblank cells" */
                    if (pass == 0u) stat_add(&s, &v);
                    else {
                        double x = (v.t == I123_V_NUM) ? v.n : 0.0, d = x - avg;
                        acc = acc + d * d;
                    }
                }
            }
            if (pass == 1u) s.sumsq = acc;
            if (s.fn != 87u && s.fn != 88u) break;
        }
    }
    if (s.err == 1u) { set_err(out); return; }
    if (s.err == 2u) { set_na(out); return; }
    switch (s.fn) {
    case 80u: set_num(out, s.sum); return;
    case 81u: if (!s.n) { set_err(out); return; } set_num(out, s.sum / (double)s.n); return;
    case 82u: set_num(out, (double)s.n); return;
    case 83u: set_num(out, s.mn); return;
    case 84u: set_num(out, s.mx); return;
    default: {
        double var;
        if (!s.n) { set_err(out); return; }
        var = s.sumsq / (double)s.n;
        if (s.fn == 88u) var = (double)x87_sqrt((long double)var);
        set_num(out, var);
        return;
    }
    }
}

/* ---- dates and times ([M] p. 13-7..13-8 (PDF 295-296), p. 13-22..13-23
 * (PDF 310-311)): day 1 = 1 Jan 1900, 29 Feb 1900 exists (serial 60),
 * 73050 = 31 Dec 2099; year argument 0..199. [V] Sec 1: @DATE(92,3,9) =
 * 33672 and six shipped dates. ---------------------------------------------- */
static int leap(int32_t y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }
static const uint8_t MDAYS[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static int date_serial(int32_t y, int32_t m, int32_t d, double *out)
{
    int32_t yy = 1900 + y, n = 0, ml;
    if (y < 0 || y > 199 || m < 1 || m > 12 || d < 1) return 0;
    ml = MDAYS[m - 1] + ((m == 2 && (leap(yy) || yy == 1900)) ? 1 : 0);
    if (d > ml) return 0;
    for (int32_t k = 1900; k < yy; k++) n += leap(k) ? 366 : 365;
    for (int32_t k = 1; k < m; k++) n += MDAYS[k - 1] + ((k == 2 && leap(yy)) ? 1 : 0);
    n += d;                                   /* 1 Jan 1900 = 1 */
    if (yy > 1900 || m > 2) n += 1;           /* the phantom 29 Feb 1900 */
    if (yy == 1900 && m == 2 && d == 29) n = 60;
    *out = (double)n;
    return 1;
}

static int serial_parts(double x, int32_t *y, int32_t *m, int32_t *d)
{
    int32_t n;
    if (!(x >= 1.0 && x < 73051.0)) return 0;
    n = (int32_t)x;
    if (n == 60) { *y = 0; *m = 2; *d = 29; return 1; }
    if (n > 60) n--;
    n--;
    *y = 1900;
    for (;;) { int32_t yl = leap(*y) ? 366 : 365; if (n < yl) break; n -= yl; (*y)++; }
    for (*m = 1; *m <= 12; (*m)++) {
        int32_t ml = MDAYS[*m - 1] + ((*m == 2 && leap(*y)) ? 1 : 0);
        if (n < ml) break;
        n -= ml;
    }
    *d = n + 1;
    *y -= 1900;
    return 1;
}

static const char MONS[] = "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";

/* Parse an unsigned integer at s[*i]; returns digits read. */
static uint32_t uint_at(const uint8_t *s, uint32_t n, uint32_t *i, int32_t *v)
{
    uint32_t k = 0;
    *v = 0;
    while (*i < n && s[*i] >= '0' && s[*i] <= '9' && k < 6u) { *v = *v * 10 + (s[*i] - '0'); (*i)++; k++; }
    return k;
}

/* @DATEVALUE: the Date formats D1 DD-MMM-YY, D2 DD-MMM, D3 MMM-YY, D4
 * MM/DD/YY, D5 MM/DD ([M] p. 13-23 (PDF 311): "String must be in one of the
 * 1-2-3 Date formats"); D2/D5 take the current year (needs the clock). */
static void fn_datevalue(const ev_t *e, const i123_val_t *a, i123_val_t *out)
{
    const uint8_t *s = a->s;
    uint32_t n = a->slen, i = 0;
    int32_t d = 1, m = 0, y = -1, v;
    while (n > 0u && s[n - 1u] == ' ') n--;
    while (i < n && s[i] == ' ') i++;
    if (i < n && s[i] >= '0' && s[i] <= '9') {
        int32_t first;
        (void)uint_at(s, n, &i, &first);
        if (i < n && s[i] == '-') {               /* D1 / D2 */
            i++;
            if (i + 3u > n) { set_err(out); return; }
            for (int k = 0; k < 12; k++)
                if (up(s[i]) == MONS[3 * k] && up(s[i + 1u]) == MONS[3 * k + 1] && up(s[i + 2u]) == MONS[3 * k + 2])
                    m = k + 1;
            i += 3u;
            d = first;
            if (i < n && s[i] == '-') { i++; if (!uint_at(s, n, &i, &y)) { set_err(out); return; } }
        } else if (i < n && s[i] == '/') {        /* D4 / D5 */
            i++;
            m = first;
            if (!uint_at(s, n, &i, &d)) { set_err(out); return; }
            if (i < n && s[i] == '/') { i++; if (!uint_at(s, n, &i, &y)) { set_err(out); return; } }
        } else { set_err(out); return; }
    } else if (i + 3u <= n) {                     /* D3 MMM-YY */
        for (int k = 0; k < 12; k++)
            if (up(s[i]) == MONS[3 * k] && up(s[i + 1u]) == MONS[3 * k + 1] && up(s[i + 2u]) == MONS[3 * k + 2])
                m = k + 1;
        i += 3u;
        if (i >= n || s[i] != '-') { set_err(out); return; }
        i++;
        if (!uint_at(s, n, &i, &y)) { set_err(out); return; }
    }
    if (i != n || m == 0) { set_err(out); return; }
    if (y < 0) {
        int32_t cy, cm, cd;
        if (!e->cx->have_now || !serial_parts(e->cx->now, &cy, &cm, &cd)) { set_err(out); return; }
        y = cy;
    }
    if (y >= 1900) y -= 1900;
    {
        double r;
        if (!date_serial(y, m, d, &r)) { set_err(out); return; }
        (void)v;
        set_num(out, r);
    }
}

/* @TIMEVALUE: T1 HH:MM:SS AM/PM, T2 HH:MM AM/PM, T3 HH:MM:SS, T4 HH:MM
 * ([M] p. 13-60 (PDF 348)). */
static void fn_timevalue(const i123_val_t *a, i123_val_t *out)
{
    const uint8_t *s = a->s;
    uint32_t n = a->slen, i = 0;
    int32_t h, m, sec = 0;
    while (n > 0u && s[n - 1u] == ' ') n--;
    while (i < n && s[i] == ' ') i++;
    if (!uint_at(s, n, &i, &h) || i >= n || s[i] != ':') { set_err(out); return; }
    i++;
    if (!uint_at(s, n, &i, &m)) { set_err(out); return; }
    if (i < n && s[i] == ':') { i++; if (!uint_at(s, n, &i, &sec)) { set_err(out); return; } }
    while (i < n && s[i] == ' ') i++;
    if (i + 2u == n && up(s[i + 1u]) == 'M' && (up(s[i]) == 'A' || up(s[i]) == 'P')) {
        if (h < 1 || h > 12) { set_err(out); return; }
        if (h == 12) h = 0;
        if (up(s[i]) == 'P') h += 12;
        i += 2u;
    }
    if (i != n || h > 23 || m > 59 || sec > 59) { set_err(out); return; }
    set_num(out, ((double)h * 3600.0 + (double)m * 60.0 + (double)sec) / 86400.0);
}

/* @VALUE ([M] p. 13-61 (PDF 349)): standard, scientific, mixed number
 * ("49 3/4"), formatted ("$32.85", "85%", thousands separators); spaces
 * around ignored; empty -> 0; anything else ERR. */
static void fn_value(const i123_val_t *a, i123_val_t *out)
{
    char t[I123_STR_MAX + 1];
    const uint8_t *s = a->s;
    uint32_t n = a->slen, i = 0, k = 0;
    int neg = 0, pct = 0;
    double x, w = 0.0;
    while (n > 0u && s[n - 1u] == ' ') n--;
    while (i < n && s[i] == ' ') i++;
    if (i == n) { set_num(out, 0.0); return; }
    if (s[i] == '-' || s[i] == '+') { neg = s[i] == '-'; i++; }
    if (i < n && s[i] == '$') i++;
    if (n > i && s[n - 1u] == '%') { pct = 1; n--; }
    /* mixed number: "<int> <num>/<den>" or "<num>/<den>" */
    for (uint32_t j = i; j < n; j++) {
        if (s[j] == '/') {
            uint32_t p = i;
            int32_t whole = 0, num, den;
            uint32_t sp = i;
            while (sp < j && s[sp] != ' ') sp++;
            if (sp < j) { if (!uint_at(s, sp, &p, &whole) || p != sp) { set_err(out); return; } p = sp + 1u; }
            if (!uint_at(s, j, &p, &num) || p != j) { set_err(out); return; }
            p = j + 1u;
            if (!uint_at(s, n, &p, &den) || p != n || den == 0) { set_err(out); return; }
            w = (double)whole + (double)num / (double)den;
            if (neg) w = -w;
            if (pct) w = w / 100.0;
            set_num(out, w);
            return;
        }
    }
    for (; i < n && k < I123_STR_MAX; i++) {
        if (s[i] == ',') continue;
        t[k++] = (char)s[i];
    }
    if (k == 0u || i123_parse_value(t, k, &x) != I123_OK) { set_err(out); return; }
    if (neg) x = -x;
    if (pct) x = x / 100.0;
    set_num(out, x);
}

/* ---- lookups ([M] p. 13-34..13-35 (PDF 322-323), p. 13-62..13-64 (PDF
 * 350-352), p. 13-36..13-37 (PDF 324-325)) ------------------------------------ */
static void cell_result(const ev_t *e, uint16_t c, uint16_t r, i123_val_t *out)
{
    i123_cell_value(e->sh, c, r, out);
    if (out->t == I123_V_BLANK) { out->t = I123_V_NUM; out->n = 0.0; }   /* [inferred] */
}

static void fn_lookup(const ev_t *e, int vertical, const i123_val_t *a, i123_val_t *out)
{
    const i123_val_t *x = &a[0], *rg = &a[1];
    double off;
    uint32_t len, found = 0xFFFFFFFFu;
    if (rg->t != I123_V_RANGE) { set_err(out); return; }
    if (is_error(x)) { *out = *x; return; }
    if (!as_num(&a[2], &off, out)) return;
    off = trunc_(off);
    if (off < 0.0 || off > (double)(vertical ? rg->c1 - rg->c0 : rg->r1 - rg->r0)) { set_err(out); return; }
    len = vertical ? (uint32_t)(rg->r1 - rg->r0) + 1u : (uint32_t)(rg->c1 - rg->c0) + 1u;
    for (uint32_t i = 0; i < len; i++) {
        i123_val_t k;
        uint16_t c = (uint16_t)(vertical ? rg->c0 : rg->c0 + i);
        uint16_t r = (uint16_t)(vertical ? rg->r0 + i : rg->r0);
        i123_cell_value(e->sh, c, r, &k);
        if (x->t == I123_V_STR) {
            /* a string key matches exactly, ignoring case [inferred] */
            if (k.t == I123_V_STR && cmp_str(&k, x) == 0) { found = i; break; }
            continue;
        }
        {
            /* a label in the key row/column is the value 0, like a label
             * anywhere in arithmetic: SAMP7X G5..M5 @HLOOKUP($D$14,...)
             * with D14 = 0 over a top row starting with the label
             * "'   Y #" caches 0 = (+$D$20)*<the cell under the label>,
             * where skipping the label gives ERR */
            double xv = (x->t == I123_V_NUM) ? x->n : 0.0;
            double kv = (k.t == I123_V_NUM) ? k.n : 0.0;
            if (is_error(&k)) continue;
            if (kv > xv) break;          /* ascending: closest not larger */
            found = i;
        }
    }
    if (found == 0xFFFFFFFFu) { set_err(out); return; }
    if (vertical) cell_result(e, (uint16_t)(rg->c0 + (uint16_t)off), (uint16_t)(rg->r0 + found), out);
    else          cell_result(e, (uint16_t)(rg->c0 + found), (uint16_t)(rg->r0 + (uint16_t)off), out);
}

/* ---- @CELL / @CELLPOINTER ([M] p. 13-17..13-19 (PDF 305-307)) ------------------ */
static void fn_cell(const ev_t *e, const i123_val_t *attr, uint16_t c, uint16_t r, i123_val_t *out)
{
    static const char ATTR[10][9] = { "address", "col", "contents", "filename", "format",
                                        "prefix", "protect", "row", "type", "width" };
    const i123_cell_t *cell = i123_find(e->sh, c, r);
    int which = -1;
    uint8_t buf[24] = { 0 };
    uint32_t n = 0;
    if (!as_str(attr, out)) return;
    for (int k = 0; k < 10; k++) {
        const char *a = ATTR[k];
        uint32_t i = 0;
        while (i < attr->slen && a[i] && lo(attr->s[i]) == a[i]) i++;
        if (i == attr->slen && a[i] == '\0') { which = k; break; }
    }
    switch (which) {
    case 0: {
        char t[16];
        uint32_t k;
        buf[n++] = '$';
        k = i123_col_name(c, t);
        for (uint32_t i = 0; i < k; i++) buf[n++] = (uint8_t)t[i];
        buf[n++] = '$';
        k = i123_addr_text(0, r, t);
        for (uint32_t i = 1; i < k; i++) buf[n++] = (uint8_t)t[i];
        set_str(e->cx, out, buf, n);
        return;
    }
    case 1: set_num(out, (double)c + 1.0); return;
    case 2: cell_result(e, c, r, out); return;
    case 3: set_str(e->cx, out, buf, 0); return;            /* no file name in the core */
    case 4: {
        uint8_t f = cell ? cell->fmt : (uint8_t)WK1_FMT_FRESH_CELL;
        uint32_t ty = WK1_FMT_TYPE(f), d = WK1_FMT_LOW(f);
        if (ty == WK1_FT_SPECIAL && d == WK1_FS_DEFAULT) {
            f = e->sh->win1[WK1_W1_FMT];
            ty = WK1_FMT_TYPE(f); d = WK1_FMT_LOW(f);
        }
        if (ty == WK1_FT_SPECIAL) {
            static const char SP[13][3] = { "+", "G", "D1", "D2", "D3", "T", "H", "D4", "D5",
                                              "D6", "D7", "D8", "D9" };
            const char *p = d < 13u ? SP[d] : "G";
            while (*p) buf[n++] = (uint8_t)*p++;
        } else {
            static const char LET[5] = { 'F', 'S', 'C', 'P', ',' };
            buf[n++] = (uint8_t)LET[ty];
            if (d >= 10u) buf[n++] = (uint8_t)('0' + d / 10u);
            buf[n++] = (uint8_t)('0' + d % 10u);
        }
        set_str(e->cx, out, buf, n);
        return;
    }
    case 5:
        if (cell && cell->kind == I123_K_LABEL && cell->len) buf[n++] = e->sh->pool[cell->off];
        set_str(e->cx, out, buf, n);
        return;
    case 6: set_num(out, (cell ? cell->fmt : WK1_FMT_FRESH_CELL) & WK1_FMT_PROTECT ? 1.0 : 0.0); return;
    case 7: set_num(out, (double)r + 1.0); return;
    case 8:
        buf[0] = (uint8_t)(!cell || cell->kind == I123_K_BLANK ? 'b' :
                           cell->kind == I123_K_LABEL ? 'l' : 'v');
        set_str(e->cx, out, buf, 1);
        return;
    case 9: set_num(out, (double)i123_colwidth(e->sh, c)); return;
    default: set_err(out); return;
    }
}

/* ---- the evaluator ------------------------------------------------------------------ */
static void do_function(const ev_t *e, uint8_t t, i123_val_t *a, uint32_t argc, i123_val_t *out)
{
    i123_calc_t *cx = e->cx;
    double x = 0.0, y = 0.0, z = 0.0, w = 0.0;
    out->cell = 0;

    switch (t) {
    /* -- argument-free ------------------------------------------------------- */
    case 31u: set_na(out); return;                                   /* @NA  [V] -inf */
    case 32u: set_err(out); return;                                  /* @ERR [V] +inf */
    case 38u: set_num(out, (double)x87_pi()); return;                /* @PI [V] 3.141592653589793 */
    case 51u: set_num(out, 0.0); return;                             /* @FALSE [M] PDF 320 */
    case 52u: set_num(out, 1.0); return;                             /* @TRUE [V] */
    case 53u:                                                        /* @RAND [M] PDF 338 */
        /* 1-2-3's generator is not documented: a 32-bit LCG (Numerical
         * Recipes constants), deterministic from the calc seed [inferred]. */
        cx->rand_state = cx->rand_state * 1664525u + 1013904223u;
        set_num(out, (double)cx->rand_state / 4294967296.0);
        return;
    case 55u:                                                        /* @NOW [M] PDF 334 */
        if (!cx->have_now) { set_err(out); return; }
        set_num(out, cx->now);
        return;
    default: break;
    }

    /* -- predicates that never ripple ---------------------------------------- */
    switch (t) {
    case 49u: set_num(out, a[0].t == I123_V_NA ? 1.0 : 0.0); return;     /* @ISNA  PDF 328 */
    case 50u: set_num(out, a[0].t == I123_V_ERR ? 1.0 : 0.0); return;    /* @ISERR PDF 328 */
    case 68u:                                                            /* @ISNUMBER PDF 328 */
        set_num(out, (a[0].t == I123_V_NUM || a[0].t == I123_V_BLANK || is_error(&a[0])) ? 1.0 : 0.0);
        return;
    case 69u: set_num(out, a[0].t == I123_V_STR ? 1.0 : 0.0); return;    /* @ISSTRING PDF 329 */
    case 59u:                                                            /* @IF PDF 324 */
        if (is_error(&a[0])) { *out = a[0]; out->cell = 0; return; }
        *out = truth(&a[0]) ? a[1] : a[2];
        if (out->t == I123_V_BLANK) { out->t = I123_V_NUM; out->n = 0.0; }
        out->cell = 0;
        return;
    case 48u: {                                                          /* @CHOOSE PDF 307 */
        if (!as_num(&a[0], &x, out)) return;
        x = trunc_(x);
        if (x < 0.0 || x > (double)(argc - 2u)) { set_err(out); return; }
        *out = a[1u + (uint32_t)x];
        if (out->t == I123_V_RANGE) { set_err(out); return; }
        if (out->t == I123_V_BLANK) { out->t = I123_V_NUM; out->n = 0.0; }
        out->cell = 0;
        return;
    }
    case 80u: case 81u: case 82u: case 83u: case 84u: case 87u: case 88u:
        fn_stat(e, t, a, argc, out);
        return;
    case 91u: case 92u: case 93u: case 94u: case 95u: case 96u: case 97u:
        fn_db(e, t, a, out);
        return;
    case 85u: fn_lookup(e, 1, a, out); return;                           /* @VLOOKUP */
    case 90u: fn_lookup(e, 0, a, out); return;                           /* @HLOOKUP */
    case 98u: {                                                          /* @INDEX PDF 324 */
        if (a[0].t != I123_V_RANGE) { set_err(out); return; }
        if (!as_num(&a[1], &x, out) || !as_num(&a[2], &y, out)) return;
        x = trunc_(x); y = trunc_(y);
        if (x < 0.0 || y < 0.0 || x > (double)(a[0].c1 - a[0].c0) || y > (double)(a[0].r1 - a[0].r0)) {
            set_err(out); return;
        }
        cell_result(e, (uint16_t)(a[0].c0 + (uint16_t)x), (uint16_t)(a[0].r0 + (uint16_t)y), out);
        out->cell = 0;
        return;
    }
    case 99u:                                                            /* @COLS PDF 309 */
        if (a[0].t != I123_V_RANGE) { set_err(out); return; }
        set_num(out, (double)(a[0].c1 - a[0].c0) + 1.0);
        return;
    case 100u:                                                           /* @ROWS PDF 341 */
        if (a[0].t != I123_V_RANGE) { set_err(out); return; }
        set_num(out, (double)(a[0].r1 - a[0].r0) + 1.0);
        return;
    case 111u: {                                                         /* @S PDF 341 */
        if (a[0].t != I123_V_RANGE) { set_err(out); return; }
        i123_cell_value(e->sh, a[0].c0, a[0].r0, out);
        if (out->t != I123_V_STR) { out->t = I123_V_STR; out->s = (const uint8_t *)""; out->slen = 0; }
        out->cell = 0;
        return;
    }
    case 112u: {                                                         /* @N PDF 333 */
        if (a[0].t != I123_V_RANGE) { set_err(out); return; }
        i123_cell_value(e->sh, a[0].c0, a[0].r0, out);
        if (out->t != I123_V_NUM && !is_error(out)) { out->t = I123_V_NUM; out->n = 0.0; }
        out->cell = 0;
        return;
    }
    case 108u:                                                           /* @CELL PDF 305 */
        if (a[1].t != I123_V_RANGE) { set_err(out); return; }
        fn_cell(e, &a[0], a[1].c0, a[1].r0, out);
        return;
    case 79u:                                                            /* @CELLPOINTER PDF 307 */
        fn_cell(e, &a[0], i123_cursor_col(e->sh), i123_cursor_row(e->sh), out);
        return;
    case WKF_LINK: {                                                     /* @@ PDF 302 */
        const i123_cell_t *self;
        if (a[0].t == I123_V_STR && a[0].cell == 0u && a[0].slen >= 2u &&
            a[0].s[0] == '<' && a[0].s[1] == '<') {
            /* a file link: its value is refreshed only by /File Retrieve or
             * /File Admin Link-Refresh ([M] p. 1-41..1-43 (PDF 57-59)) --
             * between those it keeps the value it holds */
            i123_cell_value(e->sh, e->col, e->row, out);
            self = i123_find(e->sh, e->col, e->row);
            if (self == (const i123_cell_t *)0 || self->kind != I123_K_FORMULA) set_na(out);
            out->cell = 0;
            return;
        }
        /* @@(location): the label in location names a cell */
        if (a[0].t != I123_V_STR || !a[0].cell) { set_err(out); return; }
        {
            uint8_t code[I123_CODE_MAX];
            uint32_t cl;
            if (i123_formula_compile((const char *)a[0].s, a[0].slen, e->col, e->row,
                                     code, sizeof code, &cl) != I123_OK || code[0] != WKF_VAR) {
                set_err(out);
                return;
            }
            if (cl != 6u) { set_err(out); return; }
            {
                int32_t c, r;
                resolve_ref(e, code + 1, &c, &r);
                if (r < 0) { set_err(out); return; }
                cell_result(e, (uint16_t)c, (uint16_t)r, out);
                out->cell = 0;
            }
        }
        return;
    }
    default: break;
    }

    /* -- string functions ([M] p. 13-12..13-13 (PDF 300-301)) ---------------- */
    switch (t) {
    case 70u:                                                        /* @LENGTH [V] */
        if (!as_str(&a[0], out)) return;
        set_num(out, (double)a[0].slen);
        return;
    case 102u: case 103u: case 107u: {                               /* @UPPER [V] @LOWER @PROPER */
        uint8_t buf[I123_STR_MAX];
        int start = 1;
        if (!as_str(&a[0], out)) return;
        for (uint32_t i = 0; i < a[0].slen; i++) {
            int ch = a[0].s[i];
            int alpha = (up(ch) >= 'A' && up(ch) <= 'Z');
            if (t == 102u) ch = up(ch);
            else if (t == 103u) ch = lo(ch);
            else { ch = start ? up(ch) : lo(ch); }
            start = !alpha;
            buf[i] = (uint8_t)ch;
        }
        set_str(cx, out, buf, a[0].slen);
        return;
    }
    case 109u: case 110u: {                                          /* @TRIM @CLEAN */
        uint8_t buf[I123_STR_MAX];
        uint32_t n = 0;
        if (!as_str(&a[0], out)) return;
        for (uint32_t i = 0; i < a[0].slen; i++) {
            uint8_t ch = a[0].s[i];
            if (t == 110u) { if (ch >= 32u) buf[n++] = ch; continue; }
            if (ch == ' ' && (n == 0u || buf[n - 1u] == ' ')) continue;
            buf[n++] = ch;
        }
        if (t == 109u && n > 0u && buf[n - 1u] == ' ') n--;
        set_str(cx, out, buf, n);
        return;
    }
    case 75u:                                                        /* @CODE PDF 308 */
        if (!as_str(&a[0], out)) return;
        if (a[0].slen == 0u) { set_err(out); return; }
        set_num(out, (double)a[0].s[0]);
        return;
    case 71u:                                                        /* @VALUE [V] PDF 349 */
        if (a[0].t == I123_V_BLANK) { set_num(out, 0.0); return; }
        if (!as_str(&a[0], out)) return;
        fn_value(&a[0], out);
        return;
    case 77u:                                                        /* @DATEVALUE */
        if (!as_str(&a[0], out)) return;
        fn_datevalue(e, &a[0], out);
        return;
    case 78u:                                                        /* @TIMEVALUE */
        if (!as_str(&a[0], out)) return;
        fn_timevalue(&a[0], out);
        return;
    case 74u: {                                                      /* @CHAR PDF 307 */
        uint8_t ch;
        if (!as_num(&a[0], &x, out)) return;
        x = trunc_(x);
        if (x < 1.0 || x > 255.0) { set_err(out); return; }
        ch = (uint8_t)x;
        set_str(cx, out, &ch, 1);
        return;
    }
    case 104u: case 105u: {                                          /* @LEFT @RIGHT PDF 329/340 */
        if (!as_str(&a[0], out) || !as_num(&a[1], &x, out)) return;
        x = trunc_(x);
        if (x < 0.0) { set_err(out); return; }
        if (x > (double)a[0].slen) x = (double)a[0].slen;
        if (t == 104u) set_str(cx, out, a[0].s, (uint32_t)x);
        else set_str(cx, out, a[0].s + a[0].slen - (uint32_t)x, (uint32_t)x);
        return;
    }
    case 73u: {                                                      /* @MID PDF 331 */
        if (!as_str(&a[0], out) || !as_num(&a[1], &x, out) || !as_num(&a[2], &y, out)) return;
        x = trunc_(x); y = trunc_(y);
        if (x < 0.0 || y < 0.0) { set_err(out); return; }
        if (x >= (double)a[0].slen) { set_str(cx, out, a[0].s, 0); return; }
        if (y > (double)a[0].slen - x) y = (double)a[0].slen - x;
        set_str(cx, out, a[0].s + (uint32_t)x, (uint32_t)y);
        return;
    }
    case 76u: {                                                      /* @FIND PDF 320 */
        if (!as_str(&a[0], out) || !as_str(&a[1], out) || !as_num(&a[2], &x, out)) return;
        x = trunc_(x);
        if (x < 0.0) { set_err(out); return; }
        for (uint32_t i = (uint32_t)(x > 65535.0 ? 65535.0 : x); i + a[0].slen <= a[1].slen; i++) {
            uint32_t k = 0;
            while (k < a[0].slen && a[1].s[i + k] == a[0].s[k]) k++;   /* case sensitive */
            if (k == a[0].slen) { set_num(out, (double)i); return; }
        }
        set_err(out);
        return;
    }
    case 101u: {                                                     /* @REPEAT PDF 339 */
        uint8_t buf[I123_STR_MAX];
        uint32_t n = 0;
        if (!as_str(&a[0], out) || !as_num(&a[1], &x, out)) return;
        x = trunc_(x);
        if (x < 0.0) { set_err(out); return; }
        for (uint32_t r = 0; r < (uint32_t)(x > 240.0 ? 241.0 : x); r++) {
            for (uint32_t i = 0; i < a[0].slen; i++) {
                if (n >= I123_STR_MAX) { set_err(out); return; }
                buf[n++] = a[0].s[i];
            }
        }
        set_str(cx, out, buf, n);
        return;
    }
    case 106u: {                                                     /* @REPLACE PDF 339 */
        uint8_t buf[I123_STR_MAX];
        uint32_t n = 0, st, cnt;
        if (!as_str(&a[0], out) || !as_num(&a[1], &x, out) || !as_num(&a[2], &y, out) ||
            !as_str(&a[3], out)) return;
        x = trunc_(x); y = trunc_(y);
        if (x < 0.0 || y < 0.0) { set_err(out); return; }
        st = x > (double)a[0].slen ? a[0].slen : (uint32_t)x;
        cnt = y > (double)(a[0].slen - st) ? a[0].slen - st : (uint32_t)y;
        if ((uint32_t)a[0].slen - cnt + a[3].slen > I123_STR_MAX) { set_err(out); return; }
        for (uint32_t i = 0; i < st; i++) buf[n++] = a[0].s[i];
        for (uint32_t i = 0; i < a[3].slen; i++) buf[n++] = a[3].s[i];
        for (uint32_t i = st + cnt; i < a[0].slen; i++) buf[n++] = a[0].s[i];
        set_str(cx, out, buf, n);
        return;
    }
    case 113u: {                                                     /* @EXACT PDF 319 */
        uint32_t same;
        if (!as_str(&a[0], out) || !as_str(&a[1], out)) return;
        same = a[0].slen == a[1].slen;
        for (uint32_t i = 0; same && i < a[0].slen; i++) same = a[0].s[i] == a[1].s[i];
        set_num(out, same ? 1.0 : 0.0);
        return;
    }
    case 72u: {                                                      /* @STRING [V] PDF 344 */
        char buf[48];
        uint32_t n;
        if (!as_num(&a[0], &x, out) || !as_num(&a[1], &y, out)) return;
        y = trunc_(y);
        if (y < 0.0 || y > 15.0) { set_err(out); return; }
        n = i123_fixed_text(x, (uint32_t)y, buf, sizeof buf);
        if (n == 0u) { set_err(out); return; }
        set_str(cx, out, (const uint8_t *)buf, n);
        return;
    }
    default: break;
    }

    /* -- numeric functions: every argument a number (blank 0, [M] PDF 292) --- */
    {
        double v[4] = { 0.0, 0.0, 0.0, 0.0 };
        for (uint32_t i = 0; i < argc && i < 4u; i++) {
            if (a[i].t == I123_V_RANGE) { set_err(out); return; }
            if (!as_num(&a[i], &v[i], out)) {
                /* ERR over NA across the arguments [inferred, G-6] */
                for (uint32_t k = i + 1u; k < argc && k < 4u; k++)
                    if (a[k].t == I123_V_ERR) set_err(out);
                return;
            }
        }
        x = v[0]; y = v[1]; z = v[2]; w = v[3];
    }
    switch (t) {
    case 33u: set_num(out, x < 0.0 ? -x : x); return;               /* @ABS [V] */
    case 34u: set_num(out, trunc_(x)); return;                      /* @INT [V] toward zero */
    case 35u:                                                       /* @SQRT [V]; <0 ERR PDF 343 */
        if (x < 0.0) { set_err(out); return; }
        set_num(out, (double)x87_sqrt((long double)x));
        return;
    case 36u: case 37u:                                             /* @LOG @LN; x>0 PDF 330 */
        if (!(x > 0.0)) { set_err(out); return; }
        set_num(out, (double)x87_fyl2x(t == 36u ? x87_lg2() : x87_ln2(), (long double)x));
        return;
    case 46u:                                                       /* @EXP PDF 320 */
        if (x > 709.0) { set_err(out); return; }
        if (x < -709.0) { set_num(out, 0.0); return; }
        set_num(out, (double)x87_exp2((long double)x * x87_l2e()));
        return;
    case 39u: case 40u: case 41u: {                                 /* @SIN @COS @TAN */
        long double r;
        if (x > 9.2e18 || x < -9.2e18) { set_err(out); return; }    /* FSIN range */
        r = (t == 39u) ? x87_sin((long double)x) : (t == 40u) ? x87_cos((long double)x)
                                                              : x87_tan((long double)x);
        set_num(out, (double)r);
        return;
    }
    case 43u: set_num(out, (double)x87_atan2((long double)x, 1.0L)); return;   /* @ATAN */
    case 42u:                                                       /* @ATAN2(x,y) PDF 304 */
        /* "If y is 0, @ATAN2 returns 0; if both x and y are 0, @ATAN2
         * returns the value ERR" (p. 13-16) -- as written, for x < 0 too */
        if (x == 0.0 && y == 0.0) { set_err(out); return; }
        if (y == 0.0) { set_num(out, 0.0); return; }
        set_num(out, (double)x87_atan2((long double)y, (long double)x));
        return;
    case 44u: case 45u: {                                           /* @ASIN @ACOS PDF 303-304 */
        long double c;
        if (x < -1.0 || x > 1.0) { set_err(out); return; }
        c = x87_sqrt(1.0L - (long double)x * (long double)x);
        set_num(out, (double)(t == 44u ? x87_atan2((long double)x, c) : x87_atan2(c, (long double)x)));
        return;
    }
    case 47u:                                                       /* @MOD [V]; y=0 ERR PDF 332 */
        if (y == 0.0) { set_err(out); return; }
        if (x == 0.0) { set_num(out, 0.0); return; }
        set_num(out, (double)x87_fprem((long double)x, (long double)y));
        return;
    case 63u: {                                                     /* @ROUND [V] PDF 340 */
        /* halves away from zero (MINT03 2.5 -> 3, -2.5 -> -3); n -15..15 */
        double s, q, r;
        y = trunc_(y);
        if (y < -15.0 || y > 15.0) { set_err(out); return; }
        s = 1.0;
        for (int i = 0; i < (int)(y < 0 ? -y : y); i++) s = s * 10.0;
        q = (y >= 0.0) ? x * s : x / s;
        r = trunc_((q < 0.0 ? -q : q) + 0.5);
#ifdef I123_MUT_ROUND_EVEN
        /* MUTANT (Rule 6; test-i123-formula-mutant): halves round to even
         * (banker's rounding) -- @ROUND(2.5,0) = 2 where MINT03 caches 3.
         * NEVER in a real build. */
        {
            double aq = q < 0.0 ? -q : q;
            if (aq - trunc_(aq) == 0.5 && trunc_(r / 2.0) * 2.0 != r) r = r - 1.0;
        }
#endif
        if (q < 0.0) r = -r;
        set_num(out, (y >= 0.0) ? r / s : r * s);
        return;
    }
    case 54u: {                                                     /* @DATE [V] PDF 310 */
        double r;
        if (x != trunc_(x) || y != trunc_(y) || z != trunc_(z) ||
            !date_serial((int32_t)x, (int32_t)y, (int32_t)z, &r)) { set_err(out); return; }
        set_num(out, r);
        return;
    }
    case 60u: case 61u: case 62u: {                                 /* @DAY @MONTH @YEAR */
        int32_t yy, mm, dd;
        if (!serial_parts(x, &yy, &mm, &dd)) { set_err(out); return; }
        set_num(out, (double)(t == 60u ? dd : t == 61u ? mm : yy));
        return;
    }
    case 64u:                                                       /* @TIME PDF 347 */
        x = trunc_(x); y = trunc_(y); z = trunc_(z);
        if (x < 0.0 || x > 23.0 || y < 0.0 || y > 59.0 || z < 0.0 || z > 59.0) { set_err(out); return; }
        set_num(out, (x * 3600.0 + y * 60.0 + z) / 86400.0);
        return;
    case 65u: case 66u: case 67u: {                                 /* @HOUR @MINUTE @SECOND */
        double f, secs;
        uint32_t s;
        if (x < 0.0) { set_err(out); return; }
        f = x - trunc_(x);
        secs = f * 86400.0;
        s = (uint32_t)trunc_(secs + 0.5);          /* nearest second [inferred] */
        if (s >= 86400u) s = 86399u;
        set_num(out, (double)(t == 65u ? s / 3600u : t == 66u ? (s / 60u) % 60u : s % 60u));
        return;
    }
    case 56u: case 57u: case 58u: {                                 /* @PMT @PV @FV PDF 321/336/337 */
        /* @PMT(principal,int,term) prin*int/(1-(1+int)^-n);
         * @PV(pmt,int,term)       pmt*(1-(1+int)^-n)/int;
         * @FV(pmt,int,term)       pmt*((1+int)^n-1)/int */
        i123_val_t p;
        double f;
        if (y <= -1.0) { set_err(out); return; }
        if (y == 0.0) {
            if (t == 56u) { if (z == 0.0) { set_err(out); return; } set_num(out, x / z); }
            else set_num(out, x * z);
            return;
        }
        fn_pow(1.0 + y, t == 58u ? z : -z, &p);
        if (p.t != I123_V_NUM) { *out = p; return; }
        f = p.n;
        if (t == 56u) { double d = 1.0 - f; if (d == 0.0) { set_err(out); return; } set_num(out, x * y / d); }
        else if (t == 57u) set_num(out, x * (1.0 - f) / y);
        else set_num(out, x * (f - 1.0) / y);
        return;
    }
    case 116u: {                                                    /* @RATE PDF 338 */
        i123_val_t p;
        if (y == 0.0 || z == 0.0) { set_err(out); return; }
        fn_pow(x / y, 1.0 / z, &p);
        if (p.t != I123_V_NUM) { *out = p; return; }
        set_num(out, p.n - 1.0);
        return;
    }
    case 117u: {                                                    /* @TERM PDF 346 */
        double a1, b1;
        if (x == 0.0 || y <= -1.0 || y == 0.0) { set_err(out); return; }
        a1 = 1.0 + z * y / x;
        b1 = 1.0 + y;
        if (!(a1 > 0.0)) { set_err(out); return; }
        set_num(out, (double)(x87_fyl2x(x87_ln2(), (long double)a1) / x87_fyl2x(x87_ln2(), (long double)b1)));
        return;
    }
    case 118u: {                                                    /* @CTERM PDF 310 */
        /* @CTERM(interest,future-value,present-value) = ln(fv/pv)/ln(1+int) */
        double q;
        if (x <= -1.0 || x == 0.0 || z == 0.0) { set_err(out); return; }
        q = y / z;
        if (!(q > 0.0)) { set_err(out); return; }
        set_num(out, (double)(x87_fyl2x(x87_ln2(), (long double)q) /
                              x87_fyl2x(x87_ln2(), 1.0L + (long double)x)));
        return;
    }
    case 119u:                                                      /* @SLN PDF 342 */
        if (z == 0.0) { set_err(out); return; }
        set_num(out, (x - y) / z);
        return;
    case 120u:                                                      /* @SYD PDF 345 */
        if (z < 1.0 || w < 1.0) { set_err(out); return; }
        set_num(out, (x - y) * (z - w + 1.0) / (z * (z + 1.0) / 2.0));
        return;
    case 121u: {                                                    /* @DDB PDF 314 */
        /* (bv*2)/n per period; stops at salvage */
        double bv = x, d = 0.0;
        if (x < y || z <= 2.0 || w < 1.0) { set_err(out); return; }
        for (uint32_t p = 1; p <= (uint32_t)trunc_(w) && p <= 10000u; p++) {
            d = bv * 2.0 / z;
            if (bv - d < y) d = bv - y;
            if (d < 0.0) d = 0.0;
            bv = bv - d;
        }
        set_num(out, d);
        return;
    }
    default:
        set_err(out);  /* an unimplemented token is refused, never guessed */
        return;
    }
    (void)w;
}

/* @NPV / @IRR walk their range in row-major order -- the corpus has no use;
 * the manual ([M] p. 13-47 (PDF 335), p. 13-38 (PDF 326)) states the
 * formulas; @IRR counts blanks as 0 and stops after 30 iterations at
 * 1E-7. */
typedef struct { double v[256]; uint32_t n; uint8_t err; } flows_t;

static void flows_of(const ev_t *e, const i123_val_t *rg, flows_t *f, int blanks)
{
    f->n = 0; f->err = 0;
    for (uint32_t r = rg->r0; r <= rg->r1; r++) {
        for (uint32_t c = rg->c0; c <= rg->c1; c++) {
            i123_val_t v;
            i123_cell_value(e->sh, (uint16_t)c, (uint16_t)r, &v);
            if (v.t == I123_V_ERR) { f->err = 1; return; }
            if (v.t == I123_V_NA && !f->err) { f->err = 2; return; }
            if (v.t == I123_V_BLANK && !blanks) continue;
            if (f->n >= 256u) { f->err = 1; return; }
            f->v[f->n++] = (v.t == I123_V_NUM) ? v.n : 0.0;
        }
    }
}

static void fn_npv_irr(const ev_t *e, uint8_t t, const i123_val_t *a, i123_val_t *out)
{
    flows_t f;
    double rate;
    if (a[1].t != I123_V_RANGE) { set_err(out); return; }
    if (!as_num(&a[0], &rate, out)) return;
    flows_of(e, &a[1], &f, t == 89u);
    if (f.err) { if (f.err == 1u) set_err(out); else set_na(out); return; }
    if (t == 86u) {
        double acc = 0.0, disc = 1.0;
        if (rate <= -1.0) { set_err(out); return; }
        for (uint32_t i = 0; i < f.n; i++) { disc = disc * (1.0 + rate); acc = acc + f.v[i] / disc; }
        set_num(out, acc);
        return;
    }
    /* @IRR: Newton on NPV(r) with the first flow at time 0 */
    for (uint32_t it = 0; it < 30u; it++) {
        double npv = 0.0, d = 0.0, disc = 1.0, nr;
        for (uint32_t i = 0; i < f.n; i++) {
            npv = npv + f.v[i] / disc;
            d = d - (double)i * f.v[i] / (disc * (1.0 + rate));
            disc = disc * (1.0 + rate);
        }
        if (d == 0.0 || rate <= -1.0) break;
        nr = rate - npv / d;
        if ((nr - rate < 0 ? rate - nr : nr - rate) < 1e-7) { set_num(out, nr); return; }
        rate = nr;
    }
    set_err(out);
}

int i123_eval(i123_calc_t *cx, const i123_sheet_t *sh, uint16_t col, uint16_t row,
              const uint8_t *code, uint32_t len, i123_val_t *out)
{
    ev_t e;
    uint32_t sp = 0, i = 0;
    i123_val_t *st = cx->stk;

    e.cx = cx; e.sh = sh; e.col = col; e.row = row;
    cx->arena_used = 0;
    cx->home_col = col;
    cx->home_row = row;
    if (i123_formula_check(code, len) != I123_OK) return I123_E_FORMAT;

    while (i < len) {
        uint8_t t = code[i];
        const i123_token_t *tk = &i123_tokens[t];
        uint32_t n = i123_token_size(code, len, i);
        if (t == WKF_RETURN) break;
        if (sp >= I123_EVAL_STACK - 1u) return I123_E_FULL;
        switch (tk->cls) {
        case WKF_K_OPERAND: {
            i123_val_t *v = &st[sp++];
            v->cell = 0;
            switch (t) {
            case WKF_CONST: v->t = I123_V_NUM; v->n = rd_double(code + i + 1u); break;
            case WKF_INT:   v->t = I123_V_NUM; v->n = (double)(int16_t)rd16(code + i + 1u); break;
            case WKF_STRING: {
                uint32_t k = 0;
                while (code[i + 1u + k]) k++;
                v->t = I123_V_STR; v->s = code + i + 1u; v->slen = (uint16_t)k;
                break;
            }
            case WKF_VAR: {
                int32_t c, r;
                resolve_ref(&e, code + i + 1u, &c, &r);
                if (r < 0) set_err(v);
                else i123_cell_value(sh, (uint16_t)c, (uint16_t)r, v);
                break;
            }
            default: {  /* RANGE: normalise the corners */
                int32_t c0, r0, c1, r1;
                resolve_ref(&e, code + i + 1u, &c0, &r0);
                resolve_ref(&e, code + i + 5u, &c1, &r1);
                if (r0 < 0 || r1 < 0) { set_err(v); break; }
                v->t = I123_V_RANGE;
                v->c0 = (uint16_t)(c0 < c1 ? c0 : c1); v->c1 = (uint16_t)(c0 < c1 ? c1 : c0);
                v->r0 = (uint16_t)(r0 < r1 ? r0 : r1); v->r1 = (uint16_t)(r0 < r1 ? r1 : r0);
                break;
            }
            }
            break;
        }
        case WKF_K_CTRL:      /* PAREN: no stack effect ([C] Sec 4) */
            break;
        case WKF_K_UNARY: {
            i123_val_t *a = &st[sp - 1u];
            double x;
            if (t == WKF_UPLUS) {
                /* identity, strings included (SAMP6X +" "&... caches strings) */
                if (a->t == I123_V_BLANK) { a->t = I123_V_NUM; a->n = 0.0; }
                a->cell = 0;
                break;
            }
            if (!as_num(a, &x, a)) break;
            if (t == WKF_NEG) set_num(a, -x);
            else set_num(a, x == 0.0 ? 1.0 : 0.0);      /* #NOT# */
            break;
        }
        case WKF_K_BINARY: {
            i123_val_t *a = &st[sp - 2u], *b = &st[sp - 1u];
            double x, y;
            sp--;
            if (t >= WKF_EQ && t <= WKF_GT) { op_compare(t, a, b, a); break; }
            if (pick_error(a, b, a)) break;
            if (t == WKF_CAT) {
                /* '&' joins two strings ([M] p. 1-28 (PDF 44)); a number or
                 * a blank operand is ERR [inferred] */
                uint8_t buf[I123_STR_MAX];
                uint32_t n2;
                if (a->t != I123_V_STR || b->t != I123_V_STR) { set_err(a); break; }
                n2 = (uint32_t)a->slen + b->slen;
                if (n2 > I123_STR_MAX) { set_err(a); break; }
                for (uint32_t k = 0; k < a->slen; k++) buf[k] = a->s[k];
                for (uint32_t k = 0; k < b->slen; k++) buf[a->slen + k] = b->s[k];
                set_str(cx, a, buf, n2);
                break;
            }
            if (!as_num(a, &x, a)) break;
            if (!as_num(b, &y, a)) break;
            switch (t) {
            case WKF_ADD: set_num(a, x + y); break;
            case WKF_SUB: set_num(a, x - y); break;
            case WKF_MUL: set_num(a, x * y); break;
            case WKF_DIV:
                if (y == 0.0) set_err(a);                   /* [V] 1/0 = ERR */
                else set_num(a, x / y);
                break;
            case WKF_POW: fn_pow(x, y, a); break;
            case WKF_AND: set_num(a, (x != 0.0 && y != 0.0) ? 1.0 : 0.0); break;
            case WKF_OR:  set_num(a, (x != 0.0 || y != 0.0) ? 1.0 : 0.0); break;
            default: set_err(a); break;
            }
            break;
        }
        case WKF_K_FIXED:
        case WKF_K_COUNTED: {
            uint32_t argc = (tk->cls == WKF_K_COUNTED) ? code[i + 1u] : tk->arity;
            i123_val_t *a = &st[sp - argc];
            i123_val_t r;
            r.cell = 0; r.slen = 0; r.s = (const uint8_t *)0; r.n = 0.0;
            if (t == 86u || t == 89u) fn_npv_irr(&e, t, a, &r);
            else do_function(&e, t, a, argc, &r);
            sp -= argc;
            st[sp++] = r;
            break;
        }
        default:
            return I123_E_FORMAT;
        }
        i += n;
    }
    if (sp != 1u) return I123_E_FORMAT;
    *out = st[0];
    if (out->t == I123_V_BLANK) { out->t = I123_V_NUM; out->n = 0.0; }   /* +A1 of a blank = 0 */
    if (out->t == I123_V_RANGE) { out->t = I123_V_ERR; }
    if (out->t == I123_V_STR && out->slen > I123_STR_MAX) out->t = I123_V_ERR;
    return I123_OK;
}
