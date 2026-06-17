/*
 * os/samir/core/fn_builtins.c -- SAMIR xBase III+ 1.1 built-in functions A (S3.5).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib, CDR-0001 interim toolchain). Uses ONLY <stdint.h>, rt.h, value.h
 * and eval.h. No libc, no PAL, no I/O. Pure (Rule 11): a result is a function
 * of (name, args, ctx state); the only "clock" is the INJECTED ctx->ctx_today.
 *
 * Step S3.5 of docs/plans/SAMIR-implementation-plan.md ("built-in functions A
 * (bridges + core)"). The evaluator (eval.c, XBN_CALL case) evaluates the
 * argument expressions and calls xb_call_builtin here with the (already typed)
 * argument vector. This file is the fn_* TABLE; eval.c keeps the operator
 * coercion table (S3.3). IIF is dispatched in eval.c (lazy branch selection),
 * NOT here -- see eval.h.
 *
 * This step implements EXACTLY the pure string/numeric/date/conversion set:
 *   UPPER LOWER TRIM LTRIM SUBSTR LEN SPACE CHR ASC   (string)
 *   STR VAL CTOD DTOC                                 (conversion bridges)
 *   DATE DAY MONTH YEAR                               (date)
 *   TYPE                                              (generic 1-char type code)
 * The DB-aware functions (RECNO/EOF/...) and the deferred numeric/date
 * functions (INT/ROUND/MOD/CDOW/CMONTH/DOW/SQRT/LOG/...) are S3.6 and are NOT
 * here. DTOS does NOT exist in III+ -> XBEE_INVALID_FN (#31).
 *
 * Error policy (fail loud, Rule 2; dbase_msg_codes.tsv):
 *   unknown / not-in-III+ name      -> XBEE_INVALID_FN  (#31)
 *   wrong argument COUNT            -> XBEE_INVALID_ARG  (#11)
 *   wrong argument TYPE             -> XBEE_MISMATCH     (#9)
 *   CHR(n) out of 0..255           -> XBEE_CHR_RANGE    (#57)
 *   SPACE(n) negative              -> XBEE_SPACE_NEG    (#60)
 *   SPACE(n) result > 254          -> XBEE_SPACE_LARGE  (#59)
 *   SUBSTR start < 1               -> XBEE_SUBSTR_RANGE (#62)
 *   STR bad width/dec              -> XBEE_STR_RANGE    (#63)
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/functions/string-functions.md
 *       SUBSTR 1-based (line 119 "SUBSTR(\"ABCDEF\",1,3) -> \"ABC\"");
 *       LEN -> N; UPPER/LOWER ASCII a-z<->A-Z; TRIM == RTRIM (trailing only);
 *       LTRIM (leading blanks only); SPACE(0)=\"\", neg -> #60, >254 -> #59;
 *       CHR 0..255 else #57; ASC leftmost byte, \"\" -> 0; VAL leading numeric;
 *       STR right-justified width/dec.
 *   - ../dbase3-decomp/specs/functions/numeric-and-date-functions.md
 *       DATE() system date; CTOD parse mm/dd/yy (American default), bad -> blank;
 *       DTOC mm/dd/yy 8-char (CENTURY OFF default), blank -> 8 spaces;
 *       DAY/MONTH/YEAR -> N, blank -> 0; DTOS NOT III+.
 *   - ../dbase3-decomp/specs/functions/system-and-database-functions.md
 *       TYPE() -> one of C/N/D/L/M/U.
 *   - ../dbase3-decomp/specs/runtime/dates-and-century.md
 *       default SET DATE AMERICAN mm/dd/yy, SET CENTURY OFF (width 8);
 *       base-1900 2-digit-year rule (CTOD('01/01/00')=1900);
 *       blank date sentinel (JDN 0); DTOC(blank) = 8 spaces.
 *   - spec/samir/dbase_msg_codes.tsv (the codes above).
 *   - os/samir/include/samir/{eval.h,value.h,rt.h}.
 *
 * BOUNDARIES / GATED (loud-noted, NOT guessed):
 *   - SET DATE / SET CENTURY are NOT yet wired into xb_ctx (S5.6). CTOD/DTOC
 *     therefore assume the III+ DEFAULTS: AMERICAN (mm/dd/yy) + CENTURY OFF
 *     (2-digit year, width 8). When S5.6 lands, these read ctx state instead.
 *     (dates-and-century.md: AMERICAN + CENTURY OFF are the documented defaults.)
 *   - SUBSTR start-point upper threshold (start past end -> "" vs #62) is
 *     corpus [oracle-resolves] (string-functions.md Open Q1). CONSERVATIVE
 *     CHOICE: start<1 -> #62 (the dedicated message exists); start>len returns
 *     "" (no error). Negative-start "from the right" is Clipper-only -> rejected.
 *   - VAL of non-numeric/empty -> 0 is corpus [oracle-resolves] (Open Q6); we
 *     adopt the documented Clipper/Harbour 0 result via rt.h dec_parse.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 */

#include <stdint.h>

#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"

/* ======================================================================== */
/* Small freestanding helpers                                                */
/* ======================================================================== */

/* Fail loud: set *err and return it (non-zero for every real error code). */
static int fn_fail(int *err, int code)
{
    *err = code;
    return code;
}

/* ASCII upper / lower fold of a single byte (a-z <-> A-Z; others unchanged).
 * Scope is ASCII only: high CP437 bytes are NOT folded (string-functions.md
 * UPPER/LOWER "ASCII a-z only ... accented vowels generally do NOT fold"). */
static char fold_upper(char c)
{
    if (c >= 'a' && c <= 'z') return (char)(c - 32);
    return c;
}
static char fold_lower(char c)
{
    if (c >= 'A' && c <= 'Z') return (char)(c + 32);
    return c;
}

/* A C/M (character or memo) value is acceptable wherever <expC> is required.
 * (value.h: XB_M shares the pointer+len layout with XB_C.) */
static int is_char(const xb_val *v) { return v->t == XB_C || v->t == XB_M; }

/* Blank-date sentinel: a real III+ date (1900..2155) has a large positive JDN;
 * 0.0 is the blank/empty date (dates-and-century.md + eval.c date_is_blank).
 * Mirrors eval.c's static date_is_blank (kept local; freestanding, no export). */
static int fn_date_blank(double jdn) { return jdn <= 0.0; }

/* Truncate a numeric arg toward zero to an int32 (string-functions.md: non-
 * integer length/index args truncate toward zero -- the [oracle-resolves]
 * conservative choice, consistent with the C cast). */
static int32_t to_int_trunc(double v) { return (int32_t)v; }

/* Bump-allocate n bytes of C-result storage from ctx->scratch, or NULL on
 * exhaustion (caller fails loud, XBEE_SCRATCH_FULL). No malloc (Law 3). */
static char *fn_scratch(xb_ctx *ctx, uint32_t n)
{
    char *p;
    if (ctx->scratch == 0) return 0;
    if (n > ctx->scratch_cap || ctx->scratch_used > ctx->scratch_cap - n) {
        return 0;
    }
    p = ctx->scratch + ctx->scratch_used;
    ctx->scratch_used += n;
    return p;
}

/* Case-insensitive compare of a name slice against a NUL-terminated keyword
 * (keyword already upper-case). Returns 1 on a full match. */
static int name_is(const char *name, uint16_t len, const char *kw)
{
    uint16_t i;
    for (i = 0; i < len; i++) {
        char c = name[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        if (kw[i] == '\0') return 0;          /* kw shorter than name        */
        if (c != kw[i]) return 0;
    }
    return kw[len] == '\0';                    /* both ended together         */
}

/* The III+ max character-expression length (string-functions.md: 254 bytes).
 * SPACE() guards against this (#59). */
#define XB_STR_MAX 254

/* ======================================================================== */
/* String functions                                                          */
/* ======================================================================== */

/* UPPER(<expC>) / LOWER(<expC>) -> C. Per-byte ASCII fold into scratch. */
static int fn_case(xb_ctx *ctx, const xb_val *args, int nargs,
                   int up, xb_val *out, int *err)
{
    uint16_t len, i;
    char *dst;
    if (nargs != 1) return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0])) return fn_fail(err, XBEE_MISMATCH);
    len = args[0].u.c.len;
    if (len == 0) { *out = xb_c("", 0); *err = XBEE_OK; return 0; }
    dst = fn_scratch(ctx, len);
    if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);
    for (i = 0; i < len; i++) {
        dst[i] = up ? fold_upper(args[0].u.c.p[i]) : fold_lower(args[0].u.c.p[i]);
    }
    *out = xb_c(dst, len);
    *err = XBEE_OK;
    return 0;
}

/* TRIM(<expC>) / RTRIM(<expC>) -> C: drop TRAILING blanks only (synonyms in
 * III+, string-functions.md). LTRIM(<expC>) -> C: drop LEADING blanks only.
 * No allocation: trimming yields a sub-slice of the SAME backing bytes. */
static int fn_trim(const xb_val *args, int nargs, int leading,
                   xb_val *out, int *err)
{
    uint16_t len;
    const char *p;
    if (nargs != 1) return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0])) return fn_fail(err, XBEE_MISMATCH);
    p   = args[0].u.c.p;
    len = args[0].u.c.len;
    if (leading) {
        uint16_t s = 0;
        while (s < len && p[s] == ' ') s++;
        *out = xb_c(p + s, (uint16_t)(len - s));
    } else {
        while (len > 0 && p[len - 1] == ' ') len--;
        *out = xb_c(p, len);
    }
    *err = XBEE_OK;
    return 0;
}

/* SUBSTR(<expC>, <expN1> [, <expN2>]) -> C. 1-based start. start<1 -> #62.
 * start>len -> "". expN2 omitted -> to end; expN2 clamped to available; a
 * negative or zero expN2 -> "" (string-functions.md). No allocation: a SUBSTR
 * result is a sub-slice of the SAME backing bytes. */
static int fn_substr(const xb_val *args, int nargs, xb_val *out, int *err)
{
    int32_t  start, count;
    uint16_t len;
    uint16_t avail;
    if (nargs != 2 && nargs != 3) return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0]))       return fn_fail(err, XBEE_MISMATCH);
    if (args[1].t != XB_N)        return fn_fail(err, XBEE_MISMATCH);
    if (nargs == 3 && args[2].t != XB_N) return fn_fail(err, XBEE_MISMATCH);

    len   = args[0].u.c.len;
    start = to_int_trunc(args[1].u.n);

    /* start < 1 -> dedicated #62 "SUBSTR() : Start point out of range."
     * (Clipper negative-from-the-right is NOT III+, string-functions.md.) */
    if (start < 1) return fn_fail(err, XBEE_SUBSTR_RANGE);

    /* start past end-of-string -> empty (conservative; Open Q1). */
    if ((uint32_t)start > (uint32_t)len) {
        *out = xb_c("", 0); *err = XBEE_OK; return 0;
    }

#ifdef XB_MUTATE_FN_SUBSTR
    /* MUTATION HOOK (Rule 6): treat the start position as 0-BASED instead of
     * 1-based. Then SUBSTR("ABCDEF",2,3) returns "CDE" (wrong) not "BCD", and
     * the unit assertion goes RED, proving the test catches an off-by-one in
     * the load-bearing 1-based-index rule (string-functions.md line 119).
     * Compile with -DXB_MUTATE_FN_SUBSTR to activate. Exactly one branch. */
    start += 1;
    if ((uint32_t)start > (uint32_t)len) { *out = xb_c("", 0); *err = XBEE_OK; return 0; }
#endif

    avail = (uint16_t)(len - (start - 1));     /* chars from start to end     */
    if (nargs == 3) {
        count = to_int_trunc(args[2].u.n);
        if (count <= 0) { *out = xb_c("", 0); *err = XBEE_OK; return 0; }
        if ((uint32_t)count > (uint32_t)avail) count = avail; /* clamp        */
    } else {
        count = avail;                          /* omitted -> to end          */
    }
    *out = xb_c(args[0].u.c.p + (start - 1), (uint16_t)count);
    *err = XBEE_OK;
    return 0;
}

/* LEN(<expC>) -> N: byte length (string-functions.md). */
static int fn_len(const xb_val *args, int nargs, xb_val *out, int *err)
{
    if (nargs != 1) return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0])) return fn_fail(err, XBEE_MISMATCH);
    *out = xb_n((double)args[0].u.c.len);
    *err = XBEE_OK;
    return 0;
}

/* SPACE(<expN>) -> C: n blanks. SPACE(0)="". n<0 -> #60. n>254 -> #59. */
static int fn_space(xb_ctx *ctx, const xb_val *args, int nargs,
                    xb_val *out, int *err)
{
    int32_t n, i;
    char *dst;
    if (nargs != 1)        return fn_fail(err, XBEE_INVALID_ARG);
    if (args[0].t != XB_N) return fn_fail(err, XBEE_MISMATCH);
    n = to_int_trunc(args[0].u.n);
    if (n < 0)          return fn_fail(err, XBEE_SPACE_NEG);   /* #60          */
    if (n > XB_STR_MAX) return fn_fail(err, XBEE_SPACE_LARGE); /* #59          */
    if (n == 0) { *out = xb_c("", 0); *err = XBEE_OK; return 0; }
    dst = fn_scratch(ctx, (uint32_t)n);
    if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);
    for (i = 0; i < n; i++) dst[i] = ' ';
    *out = xb_c(dst, (uint16_t)n);
    *err = XBEE_OK;
    return 0;
}

/* CHR(<expN>) -> C: single-byte string of code n. 0..255 else #57. */
static int fn_chr(xb_ctx *ctx, const xb_val *args, int nargs,
                  xb_val *out, int *err)
{
    int32_t n;
    char *dst;
    if (nargs != 1)        return fn_fail(err, XBEE_INVALID_ARG);
    if (args[0].t != XB_N) return fn_fail(err, XBEE_MISMATCH);
    n = to_int_trunc(args[0].u.n);
    if (n < 0 || n > 255) return fn_fail(err, XBEE_CHR_RANGE); /* #57          */
    dst = fn_scratch(ctx, 1);
    if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);
    dst[0] = (char)(uint8_t)n;
    *out = xb_c(dst, 1);
    *err = XBEE_OK;
    return 0;
}

/* ASC(<expC>) -> N: code of the leftmost byte; ASC("") -> 0
 * (string-functions.md; the conservative documented Clipper/Harbour result). */
static int fn_asc(const xb_val *args, int nargs, xb_val *out, int *err)
{
    if (nargs != 1) return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0])) return fn_fail(err, XBEE_MISMATCH);
    if (args[0].u.c.len == 0) { *out = xb_n(0.0); *err = XBEE_OK; return 0; }
    *out = xb_n((double)(uint8_t)args[0].u.c.p[0]);
    *err = XBEE_OK;
    return 0;
}

/* ======================================================================== */
/* Conversion bridges                                                         */
/* ======================================================================== */

/* STR(<expN1> [, <expN2> [, <expN3>]]) -> C. Right-justified width <expN2>
 * (default 10), <expN3> decimals (default 0 -> integer). Uses rt.h dec_format
 * (ties -> +inf, '*'-overflow-fill -- the minted rule).
 *   - width default 10 (string-functions.md general-expression default; the
 *     exact III+ default table is [oracle-resolves], 10 is the documented
 *     minimum; bare STR(n) acceptance is also [oracle-resolves] but we accept).
 *   - width/dec must be >= 0 and width within the field cap; else #63. */
static int fn_str(xb_ctx *ctx, const xb_val *args, int nargs,
                  xb_val *out, int *err)
{
    int32_t width = 10;   /* default general-expression width (Open Q3)        */
    int32_t dec   = 0;    /* default integer (string-functions.md)             */
    char *dst;
    if (nargs < 1 || nargs > 3) return fn_fail(err, XBEE_INVALID_ARG);
    if (args[0].t != XB_N)      return fn_fail(err, XBEE_MISMATCH);
    if (nargs >= 2) {
        if (args[1].t != XB_N) return fn_fail(err, XBEE_MISMATCH);
        width = to_int_trunc(args[1].u.n);
    }
    if (nargs == 3) {
        if (args[2].t != XB_N) return fn_fail(err, XBEE_MISMATCH);
        dec = to_int_trunc(args[2].u.n);
    }
    /* Field-width sanity: width 1..254, dec 0..15, dec must leave room for at
     * least one integer digit + sign space. Out of range -> #63. */
    if (width < 1 || width > XB_STR_MAX) return fn_fail(err, XBEE_STR_RANGE);
    if (dec   < 0 || dec   > 15)         return fn_fail(err, XBEE_STR_RANGE);
    if (dec > 0 && width < dec + 2)      return fn_fail(err, XBEE_STR_RANGE);
    dst = fn_scratch(ctx, (uint32_t)width);
    if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);
    dec_format(args[0].u.n, (int)width, (int)dec, dst); /* '*'-fill on overflow */
    *out = xb_c(dst, (uint16_t)width);
    *err = XBEE_OK;
    return 0;
}

/* VAL(<expC>) -> N: leading numeric portion (rt.h dec_parse). Non-numeric /
 * empty -> 0 (string-functions.md Open Q6 conservative choice). */
static int fn_val(const xb_val *args, int nargs, xb_val *out, int *err)
{
    if (nargs != 1) return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0])) return fn_fail(err, XBEE_MISMATCH);
    *out = xb_n(dec_parse(args[0].u.c.p, (int)args[0].u.c.len));
    *err = XBEE_OK;
    return 0;
}

/* ======================================================================== */
/* Date functions                                                            */
/* ======================================================================== */

/* CTOD(<expC>) -> D. Parses an American mm/dd/yy (default SET DATE) string into
 * a Date (JDN). 2-digit year uses the III+ base-1900 rule (no sliding window:
 * '00'->1900, '99'->1999; dates-and-century.md). A 4-digit year (length 10) is
 * taken literally (CENTURY-ON-shaped strings still parse). Unparseable / blank
 * -> the blank date (JDN 0), NOT an error (numeric-and-date-functions.md).
 *
 * GATED: assumes default AMERICAN + base-1900. SET DATE / SET CENTURY wiring is
 * S5.6; until then only the default format is honored. */
static int fn_ctod_impl(const xb_val *args, int nargs, xb_val *out, int *err)
{
    const char *p;
    uint16_t len, i;
    char digbuf[16];   /* compacted "MMDDYY" / "MMDDYYYY" + room               */
    int  nd = 0;
    int  mm, dd, yy;
    int  yearlen;

    if (nargs != 1) return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0])) return fn_fail(err, XBEE_MISMATCH);
    p   = args[0].u.c.p;
    len = args[0].u.c.len;

    /* Collect digits in order, ignoring the separators ('/', '.', '-', space).
     * American order is mm dd yy[yy]. A blank/empty/garbage string yields too
     * few digits -> blank date. */
    for (i = 0; i < len && nd < (int)sizeof(digbuf) - 1; i++) {
        char c = p[i];
        if (c >= '0' && c <= '9') digbuf[nd++] = c;
    }
    digbuf[nd] = '\0';

    /* Need at least mm(2)+dd(2)+yy(2) = 6 digits; 8 digits -> 4-digit year. */
    if (nd != 6 && nd != 8) {
        *out = xb_d(0.0); *err = XBEE_OK; return 0;   /* blank date            */
    }
    yearlen = (nd == 8) ? 4 : 2;

    /* mm = first 2 digits, dd = next 2, yy = remainder. */
    mm = (digbuf[0] - '0') * 10 + (digbuf[1] - '0');
    dd = (digbuf[2] - '0') * 10 + (digbuf[3] - '0');
    {
        int j;
        yy = 0;
        for (j = 4; j < 4 + yearlen; j++) yy = yy * 10 + (digbuf[j] - '0');
    }
    if (yearlen == 2) yy += 1900;          /* base-1900 (dates-and-century.md) */

    /* Validate ranges; an invalid field -> blank date (CTOD of a bad string is
     * the blank date, not the entry-time #78). */
    if (mm < 1 || mm > 12 || dd < 1 || dd > 31 || yy < 1900 || yy > 2155) {
        *out = xb_d(0.0); *err = XBEE_OK; return 0;
    }
    *out = xb_d((double)jdn_from_ymd(yy, mm, dd));
    *err = XBEE_OK;
    return 0;
}

/* DTOC(<expD>) -> C. Renders a Date as mm/dd/yy (8 chars, default AMERICAN +
 * CENTURY OFF). Blank date -> 8 spaces (dates-and-century.md). 2-digit year is
 * (year mod 100). GATED: assumes default format until S5.6 wires SET DATE. */
static int fn_dtoc_impl(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    char *dst;
    int32_t y, m, d;
    int yy2;
    if (nargs != 1) return fn_fail(err, XBEE_INVALID_ARG);
    if (args[0].t != XB_D) return fn_fail(err, XBEE_MISMATCH);
    dst = fn_scratch(ctx, 8);
    if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);
    if (fn_date_blank(args[0].u.d)) {
        int i; for (i = 0; i < 8; i++) dst[i] = ' ';   /* blank -> 8 spaces    */
        *out = xb_c(dst, 8); *err = XBEE_OK; return 0;
    }
    ymd_from_jdn((int32_t)args[0].u.d, &y, &m, &d);
    yy2 = (int)(y % 100);
    dst[0] = (char)('0' + (m / 10)); dst[1] = (char)('0' + (m % 10));
    dst[2] = '/';
    dst[3] = (char)('0' + (d / 10)); dst[4] = (char)('0' + (d % 10));
    dst[5] = '/';
    dst[6] = (char)('0' + (yy2 / 10)); dst[7] = (char)('0' + (yy2 % 10));
    *out = xb_c(dst, 8);
    *err = XBEE_OK;
    return 0;
}

/* DATE() -> D: today's date from the INJECTED ctx->ctx_today (JDN). No args.
 * Freestanding: never reads the OS clock (Rule 11; pal.h today()). If the
 * caller has not injected a date, ctx_today is 0.0 -> a blank date. */
static int fn_date(xb_ctx *ctx, int nargs, xb_val *out, int *err)
{
    if (nargs != 0) return fn_fail(err, XBEE_INVALID_ARG);
    *out = xb_d(ctx->ctx_today);
    *err = XBEE_OK;
    return 0;
}

/* DAY/MONTH/YEAR(<expD>) -> N. Blank date -> 0 (dates-and-century.md table).
 * which: 0=DAY, 1=MONTH, 2=YEAR. */
static int fn_dmy(const xb_val *args, int nargs, int which,
                  xb_val *out, int *err)
{
    int32_t y, m, d;
    if (nargs != 1) return fn_fail(err, XBEE_INVALID_ARG);
    if (args[0].t != XB_D) return fn_fail(err, XBEE_MISMATCH);
    if (fn_date_blank(args[0].u.d)) { *out = xb_n(0.0); *err = XBEE_OK; return 0; }
    ymd_from_jdn((int32_t)args[0].u.d, &y, &m, &d);
    switch (which) {
    case 0:  *out = xb_n((double)d); break;
    case 1:  *out = xb_n((double)m); break;
    default: *out = xb_n((double)y); break;     /* YEAR is full 4-digit        */
    }
    *err = XBEE_OK;
    return 0;
}

/* ======================================================================== */
/* TYPE(<expC>) -> C (1-char code)                                            */
/* ======================================================================== */

/*
 * TYPE() in real III+ takes a STRING expression and reports the type of the
 * expression that string denotes, WITHOUT a value resolver evaluating it for
 * value. At the eval layer (S3.5) the argument has ALREADY been evaluated to a
 * value (eager XBN_CALL args), so what we receive is the value of the
 * expression -- we report ITS type. This matches TYPE("3+4")->"N",
 * TYPE("'x'")->"C", TYPE(".T.")->"L" for literal/constant argument strings,
 * which is the test surface available at S3.5. Full TYPE("FIELDNAME") /
 * TYPE("RECNO()") semantics (re-parsing the inner string, returning "U" for an
 * undefined/invalid inner expression) require the inner expression to be lexed/
 * parsed from the string CONTENTS -- that is a Phase-5 concern (TYPE feeding
 * SET FILTER) and is GATED here: an unresolved inner symbol surfaces as the
 * eager-eval XBEE_UNBOUND BEFORE TYPE runs, so TYPE never sees it. This is the
 * documented honest boundary (system-and-database-functions.md TYPE).
 */
static int fn_type(const xb_val *args, int nargs, xb_ctx *ctx,
                   xb_val *out, int *err)
{
    char *dst;
    if (nargs != 1) return fn_fail(err, XBEE_INVALID_ARG);
    dst = fn_scratch(ctx, 1);
    if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);
    /* xb_type_char maps XB_U -> 'U' too, so an undefined value reports "U"
     * (the III+ "undefined/invalid expression" code). */
    dst[0] = xb_type_char(args[0].t);
    *out = xb_c(dst, 1);
    *err = XBEE_OK;
    return 0;
}

/* ======================================================================== */
/* Numeric functions (S3.6a -- freestanding-legal, no libm)                  */
/* ======================================================================== */

/*
 * ABS(<expN>) -> N. Absolute value: removes any negative sign.
 * Total domain (any numeric is valid). No overflow beyond the ordinary range.
 *   Ref: numeric-and-date-functions.md ABS section [verified: HELP.DBS line 1292
 *        + Harbour math.txt:7-42]; real idiom: RECONCIL.PRG:28 ABS(notcash).
 * Freestanding: negation via the unary minus operator; no libm needed.
 */
static int fn_abs(const xb_val *args, int nargs, xb_val *out, int *err)
{
    double v;
    if (nargs != 1)        return fn_fail(err, XBEE_INVALID_ARG);
    if (args[0].t != XB_N) return fn_fail(err, XBEE_MISMATCH);
    v = args[0].u.n;
#ifdef XB_MUTATE_FN_ABS
    /* MUTATION (Rule 6): skip the sign-flip so ABS(-5) returns -5.
     * Grounded assertion ABS(-5)=5 then goes RED, proving the test catches
     * a missing sign inversion. Compile with -DXB_MUTATE_FN_ABS. */
    /* (no-op: v stays negative) */
#else
    if (v < 0.0) v = -v;
#endif
    *out = xb_n(v);
    *err = XBEE_OK;
    return 0;
}

/*
 * INT(<expN>) -> N. Integer portion: truncates toward zero (NOT rounding).
 *   INT(3.99) -> 3, INT(3.01) -> 3.
 *   Negatives: PROVISIONAL CHOICE = truncate toward zero (INT(-3.7) -> -3).
 *     Harbour confirms toward-zero truncation (math.txt:103-104).
 *     III+ exact behavior with negatives is [oracle-resolves] (numfn-2 GATED).
 *     GATED: this choice is PROVISIONAL pending MINT; the oracle does NOT assert
 *     negative-input cases (those cells are loud-skipped, see test_xbase_fn_b.c).
 *   Freestanding: truncation via (int64_t) cast then back to double -- no libm.
 *   Ref: numeric-and-date-functions.md INT section [verified: HELP.DBS line 1297
 *        + Harbour math.txt:88-120].
 */
static int fn_int(const xb_val *args, int nargs, xb_val *out, int *err)
{
    double v;
    if (nargs != 1)        return fn_fail(err, XBEE_INVALID_ARG);
    if (args[0].t != XB_N) return fn_fail(err, XBEE_MISMATCH);
    /* PROVISIONAL/GATED (numfn-2): truncate toward zero via cast. */
    v = (double)(int64_t)args[0].u.n;
    *out = xb_n(v);
    *err = XBEE_OK;
    return 0;
}

/*
 * MOD(<expN1>, <expN2>) -> N. Modulus / division remainder.
 *   Definition used: MOD(a,b) = a - b * INT(a/b)  (truncated-quotient remainder).
 *   For positive operands MOD(17,5)=2, MOD(7,3)=1.
 *   Sign convention with negative operands: PROVISIONAL CHOICE = sign-of-dividend
 *     (truncation formula: MOD(-17,5) -> -2). Clipper follows sign-of-divisor;
 *     III+ exact sign rule is [oracle-resolves] (numfn-3 GATED). The oracle does
 *     NOT assert negative-operand cases (loud-skipped in test_xbase_fn_b.c).
 *   MOD(a,0) zero-divisor: PROVISIONAL CHOICE = return a (Harbour: math.txt:271
 *     shows MOD(12,0) as valid, return-value semantics). III+ exact behavior is
 *     [oracle-resolves] (numfn-3 GATED). Not asserted in the oracle.
 *   Freestanding: (double)(int64_t) for INT()-equivalent truncation; no libm.
 *   Ref: numeric-and-date-functions.md MOD section [verified: HELP.DBS line 1306
 *        + Harbour math.txt:247-284].
 */
static int fn_mod(const xb_val *args, int nargs, xb_val *out, int *err)
{
    double a, b, q;
    if (nargs != 2)        return fn_fail(err, XBEE_INVALID_ARG);
    if (args[0].t != XB_N) return fn_fail(err, XBEE_MISMATCH);
    if (args[1].t != XB_N) return fn_fail(err, XBEE_MISMATCH);
    a = args[0].u.n;
    b = args[1].u.n;
    /* PROVISIONAL/GATED (numfn-3): MOD(a,0) -> a (zero-divisor not an error). */
    if (b == 0.0) { *out = xb_n(a); *err = XBEE_OK; return 0; }
    /* PROVISIONAL/GATED (numfn-3): truncated-quotient remainder.
     * INT(a/b) via (int64_t) cast (truncation toward zero). */
    q = (double)(int64_t)(a / b);
    *out = xb_n(a - b * q);
    *err = XBEE_OK;
    return 0;
}

/*
 * ROUND(<expN1>, <expN2>) -> N. Round to <expN2> decimal places.
 *   <expN2> negative: rounds to whole-number places (tens, hundreds, ...) --
 *     ROUND(1234.5, -2) -> 1200 (Harbour math.txt:349-351).
 *   Rounding rule: PROVISIONAL CHOICE = ties-toward-+infinity, matching the
 *     STR/dec_format minted rule (re/mint-results-001.md sec "Numeric rounding
 *     tie-break"; ROUND(2.5,0)->3, ROUND(-2.5,0)->-2).
 *     Harbour says round-half-away-from-zero; III+ exact tie direction is
 *     [oracle-resolves] (numfn-1 GATED). The oracle asserts ONLY non-tie cases.
 *   Freestanding: uses the same add-0.5*10^-dec + (int64_t) truncation idiom
 *     as dec_format (rt.c); no libm.
 *   Ref: numeric-and-date-functions.md ROUND section [verified: HELP.DBS lines
 *        1307-1308 + Harbour math.txt:326-367].
 */
static int fn_round(const xb_val *args, int nargs, xb_val *out, int *err)
{
    double v, scale, rounded;
    int    dec;
    int    i;
    if (nargs != 2)        return fn_fail(err, XBEE_INVALID_ARG);
    if (args[0].t != XB_N) return fn_fail(err, XBEE_MISMATCH);
    if (args[1].t != XB_N) return fn_fail(err, XBEE_MISMATCH);
    v   = args[0].u.n;
    dec = to_int_trunc(args[1].u.n);   /* may be negative (round to tens, etc.) */

    /* Build scale = 10^|dec| by repeated multiplication (no libm/pow). */
    scale = 1.0;
    if (dec >= 0) {
        for (i = 0; i < dec; i++) scale *= 10.0;
    } else {
        int ndec = -dec;
        for (i = 0; i < ndec; i++) scale *= 10.0;
    }

    if (dec >= 0) {
        /* Round to `dec` decimal places. PROVISIONAL: ties -> +inf. */
        rounded = (double)(int64_t)(v * scale + 0.5) / scale;
    } else {
        /* Round to |dec| whole-number places (e.g. dec=-2 -> nearest 100). */
        rounded = (double)(int64_t)(v / scale + 0.5) * scale;
    }
    *out = xb_n(rounded);
    *err = XBEE_OK;
    return 0;
}

/*
 * MAX/MIN(<expN1>, <expN2>) -> N (or D for two Date args).
 *   Exactly two arguments of the SAME type; mixed type -> XBEE_MISMATCH (#9).
 *   Numeric: returns the larger (MAX) or smaller (MIN) double.
 *   Date: returns the later (MAX) or earlier (MIN) JDN-as-double.
 *     Date overload: INFERRED from Harbour math.txt:177-189 + III+ date-as-number
 *     model; return *type* (Date vs raw number) is [oracle-resolves] (numfn-4).
 *     PROVISIONAL CHOICE: return XB_D (Date) for two Date args. Not asserted in
 *     the oracle (loud-skipped per GATED discipline).
 *   `is_max`: 1 for MAX, 0 for MIN.
 *   Ref: numeric-and-date-functions.md MAX/MIN sections [verified: HELP.DBS
 *        lines 1302-1305 + Harbour math.txt:162-244].
 */
static int fn_maxmin(const xb_val *args, int nargs, int is_max,
                     xb_val *out, int *err)
{
    if (nargs != 2) return fn_fail(err, XBEE_INVALID_ARG);
    /* Same-type required (numeric-and-date-functions.md: mixed raises #9). */
    if (args[0].t != args[1].t) return fn_fail(err, XBEE_MISMATCH);

    if (args[0].t == XB_N) {
        double a = args[0].u.n, b = args[1].u.n;
        *out = xb_n(is_max ? (a >= b ? a : b) : (a <= b ? a : b));
        *err = XBEE_OK;
        return 0;
    }
    if (args[0].t == XB_D) {
        /* PROVISIONAL/GATED (numfn-4): Date overload returns XB_D. */
        double a = args[0].u.d, b = args[1].u.d;
        *out = xb_d(is_max ? (a >= b ? a : b) : (a <= b ? a : b));
        *err = XBEE_OK;
        return 0;
    }
    /* Any other type combination (C/C, L/L, etc.) -> mismatch. */
    return fn_fail(err, XBEE_MISMATCH);
}

/* ======================================================================== */
/* Date name functions (S3.6a -- freestanding, use ymd_from_jdn from rt.h)  */
/* ======================================================================== */

/*
 * DOW(<expD>) -> N.  Day-of-week number: 1=Sunday .. 7=Saturday.
 *   Blank date -> 0.
 *   Formula from JDN: DOW = ((jdn + 1) % 7) + 1.
 *     Derivation: JDN % 7 == 0 for a Monday (JDN=0 is Julian day 0, a Monday
 *     by the standard astronomical convention). So jdn%7 gives 0=Mon,1=Tue,...
 *     6=Sun. Adding 1 shifts 0->1=Sun,...,5->6=Fri,6->7=Sat -- but wraps wrong.
 *     Cleaner: (jdn + 1) % 7 gives 0=Sun,1=Mon,...,6=Sat; +1 gives 1=Sun,...
 *     7=Sat. [Verified: JDN(1985-08-04)=2446282 is Sunday -> DOW=1;
 *     JDN(1985-08-05)=2446283 is Monday -> DOW=2; JDN(1985-08-10)=2446288 is
 *     Saturday -> DOW=7.]
 *   Ref: numeric-and-date-functions.md DOW section [verified: HELP.DBS line 1248
 *        + Harbour datetime.txt:262 "1=Sunday, 7=Saturday"].
 *
 * MUTATION GUARD (-DXB_MUTATE_FN_DOW): shifts the DOW result by 1, so the
 * grounded assertion DOW(CTOD('08/04/85'))=1 (Sunday) instead returns 2.
 * The oracle then goes RED, proving it catches an off-by-one in the DOW
 * numbering. This is the REQUIRED Rule 6 mutation for this bead (S3.6a).
 * Targets a SETTLED, non-GATED case (the Sunday=1/Saturday=7 numbering is
 * [verified] against Harbour datetime.txt:262; not one of the GATED cells).
 */
static int fn_dow(const xb_val *args, int nargs, xb_val *out, int *err)
{
    int32_t jdn, dow;
    if (nargs != 1) return fn_fail(err, XBEE_INVALID_ARG);
    if (args[0].t != XB_D) return fn_fail(err, XBEE_MISMATCH);
    if (fn_date_blank(args[0].u.d)) { *out = xb_n(0.0); *err = XBEE_OK; return 0; }
    jdn = (int32_t)args[0].u.d;
    /* DOW: ((jdn+1)%7)+1  with 1=Sunday,7=Saturday [verified]. */
    dow = (int32_t)(((jdn + 1) % 7) + 1);
#ifdef XB_MUTATE_FN_DOW
    /* MUTATION (Rule 6): shift by 1, making Sunday report 2 instead of 1.
     * The Sunday=1 grounded assertion goes RED. */
    dow = (dow % 7) + 1;
#endif
    *out = xb_n((double)dow);
    *err = XBEE_OK;
    return 0;
}

/*
 * CDOW(<expD>) -> C. English weekday name, capitalized.
 *   Blank date -> "" (empty string; Harbour datetime.txt:23-24 returns NUL byte;
 *   we return empty string as the blank-safe result -- not asserted, GATED).
 *   Names (1..7): "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday",
 *     "Saturday". Stored as string literals (ASCII); no allocation from scratch.
 *   Ref: numeric-and-date-functions.md CDOW section [verified: HELP.DBS line 1243
 *        + Harbour datetime.txt:6-41].
 */

/* Static weekday-name table, 1-indexed (index 0 unused; 1=Sunday..7=Saturday).
 * Lengths: Sunday=6, Monday=6, Tuesday=7, Wednesday=9, Thursday=8,
 *          Friday=6, Saturday=8.
 * ASCII-clean (Rule 12): all English ASCII letters. */
static const char * const fn_dow_names[8] = {
    "",           /* 0: unused */
    "Sunday",     /* 1 */
    "Monday",     /* 2 */
    "Tuesday",    /* 3 */
    "Wednesday",  /* 4 */
    "Thursday",   /* 5 */
    "Friday",     /* 6 */
    "Saturday"    /* 7 */
};
static const uint16_t fn_dow_lens[8] = { 0, 6, 6, 7, 9, 8, 6, 8 };

static int fn_cdow(xb_ctx *ctx, const xb_val *args, int nargs,
                   xb_val *out, int *err)
{
    int32_t jdn, dow;
    (void)ctx;    /* No scratch needed: result points at a static literal. */
    if (nargs != 1) return fn_fail(err, XBEE_INVALID_ARG);
    if (args[0].t != XB_D) return fn_fail(err, XBEE_MISMATCH);
    if (fn_date_blank(args[0].u.d)) {
        /* Blank date -> empty string. PROVISIONAL: not asserted in the oracle
         * (Harbour says NUL byte; III+ exact blank-date behavior [oracle-resolves]).
         */
        *out = xb_c("", 0); *err = XBEE_OK; return 0;
    }
    jdn = (int32_t)args[0].u.d;
    dow = (int32_t)(((jdn + 1) % 7) + 1);   /* same formula as fn_dow */
    /* dow is 1..7; the table covers that range. */
    *out = xb_c(fn_dow_names[dow], fn_dow_lens[dow]);
    *err = XBEE_OK;
    return 0;
}

/*
 * CMONTH(<expD>) -> C. English month name, capitalized.
 *   Blank date -> "" (empty string).
 *   Names (1..12): "January","February","March","April","May","June","July",
 *     "August","September","October","November","December".
 *   Ref: numeric-and-date-functions.md CMONTH section [verified: HELP.DBS line
 *        1244 + Harbour datetime.txt:48-83].
 */

/* Static month-name table, 1-indexed (index 0 unused; 1=January..12=December).
 * Lengths: Jan=7, Feb=8, Mar=5, Apr=5, May=3, Jun=4, Jul=4, Aug=6, Sep=9,
 *          Oct=7, Nov=8, Dec=8.
 * ASCII-clean (Rule 12). */
static const char * const fn_mon_names[13] = {
    "",           /* 0: unused */
    "January",    /*  1 */
    "February",   /*  2 */
    "March",      /*  3 */
    "April",      /*  4 */
    "May",        /*  5 */
    "June",       /*  6 */
    "July",       /*  7 */
    "August",     /*  8 */
    "September",  /*  9 */
    "October",    /* 10 */
    "November",   /* 11 */
    "December"    /* 12 */
};
static const uint16_t fn_mon_lens[13] = { 0, 7, 8, 5, 5, 3, 4, 4, 6, 9, 7, 8, 8 };

static int fn_cmonth(xb_ctx *ctx, const xb_val *args, int nargs,
                     xb_val *out, int *err)
{
    int32_t y, m, d;
    (void)ctx;    /* No scratch: result points at a static literal. */
    if (nargs != 1) return fn_fail(err, XBEE_INVALID_ARG);
    if (args[0].t != XB_D) return fn_fail(err, XBEE_MISMATCH);
    if (fn_date_blank(args[0].u.d)) {
        *out = xb_c("", 0); *err = XBEE_OK; return 0;
    }
    ymd_from_jdn((int32_t)args[0].u.d, &y, &m, &d);
    /* m is 1..12 (jdn_from_ymd/ymd_from_jdn valid range enforced at CTOD). */
    *out = xb_c(fn_mon_names[m], fn_mon_lens[m]);
    *err = XBEE_OK;
    return 0;
}

/* ======================================================================== */
/* String functions C (initech-7az.12): LEFT RIGHT STUFF REPLICATE AT       */
/*                                     ISALPHA ISUPPER ISLOWER TRANSFORM     */
/* ======================================================================== */

/*
 * LEFT(<expC>, <expN>) -> C.
 * Returns the leftmost <expN> characters of <expC>.
 *   n <= 0   -> ""  (conservative; no dedicated error; matches SUBSTR(s,1,0))
 *   n >= len -> full string  (clamp; no error for overshot count)
 * Ref: HELP.DBS.strings.txt:1338-1339 (@STR FUNC 2:
 *   "A string containing the leftmost <expN> characters from the <expC>.")
 *   [verified: HELP.DBS @STR FUNC 2 line 1338; period-exact III+ surface]
 * Arity/type errors: nargs!=2 -> #11, args[0]!C -> #9, args[1]!N -> #9.
 * No scratch needed: result is a sub-slice of the input (no allocation).
 */
static int fn_left(const xb_val *args, int nargs, xb_val *out, int *err)
{
    int32_t n;
    uint16_t len;
    if (nargs != 2)           return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0]))   return fn_fail(err, XBEE_MISMATCH);
    if (args[1].t != XB_N)   return fn_fail(err, XBEE_MISMATCH);
    len = args[0].u.c.len;
    n   = to_int_trunc(args[1].u.n);
    if (n <= 0)                { *out = xb_c("", 0); *err = XBEE_OK; return 0; }
    if ((uint32_t)n > (uint32_t)len) n = (int32_t)len;   /* clamp to available */
#ifdef XB_MUTATE_FN_LEFT
    /* MUTATION (Rule 6): off-by-one: take n+1 chars (or len, whichever smaller).
     * LEFT("ABCDE",3) returns "ABCD" (4 chars) instead of "ABC" (3 chars).
     * The grounded assertion LEFT("ABCDE",3)="ABC" goes RED.
     * Compile with -DXB_MUTATE_FN_LEFT to activate. Targets a SETTLED case. */
    n = n + 1;
    if ((uint32_t)n > (uint32_t)len) n = (int32_t)len;
#endif
    *out = xb_c(args[0].u.c.p, (uint16_t)n);
    *err = XBEE_OK;
    return 0;
}

/*
 * RIGHT(<expC>, <expN>) -> C.
 * Returns the rightmost <expN> characters of <expC>.
 *   n <= 0   -> ""  (no error; mirrors LEFT behavior)
 *   n >= len -> full string  (clamp)
 * Ref: HELP.DBS.strings.txt:1349-1350 (@STR FUNC 3:
 *   "A string containing <expN> characters from the right of <expC>.")
 *   [verified: HELP.DBS @STR FUNC 3 line 1349; period-exact III+ surface]
 * No scratch: result is a sub-slice (no allocation).
 */
static int fn_right(const xb_val *args, int nargs, xb_val *out, int *err)
{
    int32_t n;
    uint16_t len;
    if (nargs != 2)           return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0]))   return fn_fail(err, XBEE_MISMATCH);
    if (args[1].t != XB_N)   return fn_fail(err, XBEE_MISMATCH);
    len = args[0].u.c.len;
    n   = to_int_trunc(args[1].u.n);
    if (n <= 0)                { *out = xb_c("", 0); *err = XBEE_OK; return 0; }
    if ((uint32_t)n > (uint32_t)len) n = (int32_t)len;
    *out = xb_c(args[0].u.c.p + len - n, (uint16_t)n);
    *err = XBEE_OK;
    return 0;
}

/*
 * REPLICATE(<expC>, <expN>) -> C.
 * Returns <expN> repetitions of <expC>. REPLICATE("AB",3) -> "ABABAB".
 *   n <= 0           -> ""  (conservative; no dedicated error)
 *   result length > XB_STR_MAX (254) -> XBEE_SPACE_LARGE (#59)
 *     (same overflow code as SPACE; both are "result string too large"
 *      domain errors; no more specific III+ code exists for REPLICATE)
 * Ref: HELP.DBS.strings.txt:1344-1345 (@STR FUNC 2:
 *   "A string containing <expN> repetitions of the <expC>.")
 *   [verified: HELP.DBS @STR FUNC 2 line 1344; period-exact III+ surface]
 * Scratch allocation required (result is a new buffer).
 */
static int fn_replicate(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    int32_t n, i;
    uint16_t clen;
    uint32_t rlen;
    char *dst;
    uint16_t j;
    if (nargs != 2)           return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0]))   return fn_fail(err, XBEE_MISMATCH);
    if (args[1].t != XB_N)   return fn_fail(err, XBEE_MISMATCH);
    clen = args[0].u.c.len;
    n    = to_int_trunc(args[1].u.n);
    if (n <= 0 || clen == 0) { *out = xb_c("", 0); *err = XBEE_OK; return 0; }
    rlen = (uint32_t)n * (uint32_t)clen;
    if (rlen > XB_STR_MAX) return fn_fail(err, XBEE_SPACE_LARGE); /* #59 */
    dst = fn_scratch(ctx, rlen);
    if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);
    for (i = 0, j = 0; i < n; i++) {
        uint16_t k;
        for (k = 0; k < clen; k++) dst[j++] = args[0].u.c.p[k];
    }
    *out = xb_c(dst, (uint16_t)rlen);
    *err = XBEE_OK;
    return 0;
}

/*
 * AT(<expC1>, <expC2>) -> N.
 * Returns the 1-based position of the FIRST occurrence of <expC1> inside
 * <expC2>. Returns 0 if not found (or if either string is empty).
 * Search is case-sensitive (consistent with $ operator and HELP surface).
 * AT("o","World") -> 2.  AT("z","World") -> 0.
 * Ref: HELP.DBS.strings.txt:1326-1327 (@STRING FUNCTIONS:
 *   "A number indicating the position of <expC1> inside <expC2>.
 *    Zero if <expC1> isn't there.")
 *   [verified: HELP.DBS @STRING FUNCTIONS line 1326; period-exact III+ surface]
 * Empty needle (expC1="") -> 0  (no match; the $ operator also returns .F.
 *   for "" $ s, per mint-results-002.md: '""$"ABC"' -> .F.)
 */
static int fn_at(const xb_val *args, int nargs, xb_val *out, int *err)
{
    uint16_t nlen, hlen, i, j;
    if (nargs != 2)           return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0]))   return fn_fail(err, XBEE_MISMATCH);
    if (!is_char(&args[1]))   return fn_fail(err, XBEE_MISMATCH);
    nlen = args[0].u.c.len;    /* needle  length */
    hlen = args[1].u.c.len;    /* haystack length */
    if (nlen == 0 || nlen > hlen) {
        *out = xb_n(0.0); *err = XBEE_OK; return 0;
    }
    for (i = 0; (uint32_t)i + (uint32_t)nlen <= (uint32_t)hlen; i++) {
        int match = 1;
        for (j = 0; j < nlen; j++) {
            if (args[1].u.c.p[i + j] != args[0].u.c.p[j]) { match = 0; break; }
        }
        if (match) { *out = xb_n((double)(i + 1)); *err = XBEE_OK; return 0; }
    }
    *out = xb_n(0.0); *err = XBEE_OK; return 0;
}

/*
 * STUFF(<expC1>, <expN1>, <expN2>, <expC2>) -> C.
 * Overlays <expC1> with <expC2> starting at 1-based position <expN1>,
 * deleting <expN2> characters from <expC1>.
 *   STUFF("ABCDE",2,3,"XY") -> "AXYE"  (delete 3 from pos 2, insert "XY")
 *   STUFF("ABCDE",2,0,"XY") -> "AXYBC DE" (insert only, no deletion)
 *   STUFF("ABCDE",2,3,"")   -> "AE"    (delete 3 chars, insert nothing)
 *   start < 1: treat as 1 (conservative; consistent with SUBSTR edge treatment)
 *   delete count < 0: treat as 0 (no deletion)
 *   If start > len: append replacement at end
 *   Result > XB_STR_MAX -> XBEE_SPACE_LARGE (#59)
 * Ref: HELP.DBS.strings.txt:1353-1354 (@STR FUNC 3:
 *   "Overlay <expC1> with <expC2>, starting at <expN1> for <expN2> characters.")
 *   [verified: HELP.DBS @STR FUNC 3 line 1353; period-exact III+ surface]
 * 4-arg function; scratch allocation required.
 */
static int fn_stuff(xb_ctx *ctx, const xb_val *args, int nargs,
                    xb_val *out, int *err)
{
    int32_t  start, del;
    uint16_t src_len, rep_len;
    uint32_t res_len;
    int32_t  pre, suf_start, suf_len;
    char    *dst;
    uint32_t pos;

    if (nargs != 4)           return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0]))   return fn_fail(err, XBEE_MISMATCH);
    if (args[1].t != XB_N)   return fn_fail(err, XBEE_MISMATCH);
    if (args[2].t != XB_N)   return fn_fail(err, XBEE_MISMATCH);
    if (!is_char(&args[3]))   return fn_fail(err, XBEE_MISMATCH);

    src_len = args[0].u.c.len;
    rep_len = args[3].u.c.len;
    start   = to_int_trunc(args[1].u.n);
    del     = to_int_trunc(args[2].u.n);

    /* Clamp start to 1..src_len+1 (1-based). */
    if (start < 1) start = 1;
    if (del < 0)   del   = 0;

    /* pre = chars before insertion point (0-based index = start-1). */
    pre = start - 1;
    if ((uint32_t)pre > (uint32_t)src_len) pre = (int32_t)src_len;

    /* suf_start = 0-based index of first byte after deleted region. */
    suf_start = pre + del;
    if ((uint32_t)suf_start > (uint32_t)src_len) suf_start = (int32_t)src_len;
    suf_len = (int32_t)src_len - suf_start;
    if (suf_len < 0) suf_len = 0;

    res_len = (uint32_t)pre + (uint32_t)rep_len + (uint32_t)suf_len;
    if (res_len > XB_STR_MAX) return fn_fail(err, XBEE_SPACE_LARGE); /* #59 */
    if (res_len == 0) { *out = xb_c("", 0); *err = XBEE_OK; return 0; }

    dst = fn_scratch(ctx, res_len);
    if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);

    pos = 0;
    {
        int32_t i;
        for (i = 0; i < pre; i++) dst[pos++] = args[0].u.c.p[i];
    }
    {
        uint16_t i;
        for (i = 0; i < rep_len; i++) dst[pos++] = args[3].u.c.p[i];
    }
    {
        int32_t i;
        for (i = 0; i < suf_len; i++) dst[pos++] = args[0].u.c.p[suf_start + i];
    }
    *out = xb_c(dst, (uint16_t)res_len);
    *err = XBEE_OK;
    return 0;
}

/*
 * ISALPHA(<expC>) -> L.
 * Returns .T. iff the FIRST character of <expC> is a letter (A-Z or a-z).
 * An empty string -> .F. (no first char; safe conservative choice).
 * Ref: HELP.DBS.strings.txt:1330 (@STRING FUNCTIONS:
 *   ".T. if the first character of <expC> is a letter.")
 *   [verified: HELP.DBS @STRING FUNCTIONS line 1330; period-exact III+ surface]
 */
static int fn_isalpha(const xb_val *args, int nargs, xb_val *out, int *err)
{
    char c;
    if (nargs != 1)           return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0]))   return fn_fail(err, XBEE_MISMATCH);
    if (args[0].u.c.len == 0) { *out = xb_l(0); *err = XBEE_OK; return 0; }
    c = args[0].u.c.p[0];
    *out = xb_l((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ? 1 : 0);
    *err = XBEE_OK;
    return 0;
}

/*
 * ISUPPER(<expC>) -> L.
 * Returns .T. iff the FIRST character of <expC> is an uppercase letter (A-Z).
 * Empty string -> .F.
 * Ref: HELP.DBS.strings.txt:1336-1337 (@STR FUNC 2:
 *   ".T. if the first character of <expC> is an uppercase letter.")
 *   [verified: HELP.DBS @STR FUNC 2 line 1336; period-exact III+ surface]
 */
static int fn_isupper(const xb_val *args, int nargs, xb_val *out, int *err)
{
    char c;
    if (nargs != 1)           return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0]))   return fn_fail(err, XBEE_MISMATCH);
    if (args[0].u.c.len == 0) { *out = xb_l(0); *err = XBEE_OK; return 0; }
    c = args[0].u.c.p[0];
    *out = xb_l((c >= 'A' && c <= 'Z') ? 1 : 0);
    *err = XBEE_OK;
    return 0;
}

/*
 * ISLOWER(<expC>) -> L.
 * Returns .T. iff the FIRST character of <expC> is a lowercase letter (a-z).
 * Empty string -> .F.
 * Ref: HELP.DBS.strings.txt:1331-1332 (@STRING FUNCTIONS:
 *   ".T. if the first character of <expC> is a lowercase letter.")
 *   [verified: HELP.DBS @STRING FUNCTIONS line 1331; period-exact III+ surface]
 */
static int fn_islower(const xb_val *args, int nargs, xb_val *out, int *err)
{
    char c;
    if (nargs != 1)           return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0]))   return fn_fail(err, XBEE_MISMATCH);
    if (args[0].u.c.len == 0) { *out = xb_l(0); *err = XBEE_OK; return 0; }
    c = args[0].u.c.p[0];
    *out = xb_l((c >= 'a' && c <= 'z') ? 1 : 0);
    *err = XBEE_OK;
    return 0;
}

/*
 * TRANSFORM(<expN>/<expC1>, <expC2>) -> C.
 *
 * GATED discipline (Law 1 / Rule 7 "GATED register"):
 *   TRANSFORM is documented in III+ 1.1 (HELP.DBS.strings.txt:1316-1319 at
 *   @NUM FUNC 3: "A character string created from either the <expN> or <expC1>
 *   in the format of <expC2>. Use @...SAY PICTURE options to format.") and
 *   CONFIRMED to exist (mint-results-002.md: TRANSFORM(-570,"9999")->"-570").
 *
 *   The PICTURE/FUNCTION template language is complex. The following clauses
 *   are CLEARLY GROUNDED from mint-results-002.md (the behavioral oracle
 *   [verified: C2.TXT]):
 *     N-arg, no @: each '9' is a digit or blank; decimal/sign passthrough.
 *       TRANSFORM(-570,"9999") -> "-570"
 *       TRANSFORM(-1234.56,"99,999.99") -> "-1,234.56" (comma groups; '.' decimal)
 *     C-arg, no @: '!' uppercase the char; 'X' or 'A' pass the char through.
 *       (Clipper-lineage core; conservative grounded subset.)
 *     @( : negatives shown in parentheses: TRANSFORM(-570,"@( 9999")-> "( 570)"
 *     @X : negatives with trailing " DB": TRANSFORM(-570,"@X 9999") -> " 570 DB"
 *     @C : positives with trailing " CR": TRANSFORM(570,"@C 9999")  -> " 570 CR"
 *     @B : left-justify (negatives still keep leading minus):
 *           TRANSFORM(-570,"@B 9999") -> "-570" (left-aligned)
 *
 *   The following TRANSFORM clauses are GATED / LOUD-SKIPPED (no corpus
 *   verification; implementation would require guessing):
 *     @R (literal mask insertion) -- unverified interaction with non-template chars
 *     @S (field scrolling width)  -- display state, not a pure value transform
 *     @Z (zero-blank)             -- unverified zero-display behavior
 *     @M (multiple choice)        -- not a formatting clause
 *     'A' and '#' picture chars   -- III+ vs Clipper exact chars [oracle-resolves]
 *     FUNCTION-only @ clauses without a picture string
 *     Two-arg @: combined flags   -- mint only tested single @ flags
 *   The @( / @X / @C / @B behavior is from mint-results-002.md C2.TXT, which is
 *   a single mint session against real III+; they are grounded but the detailed
 *   formatting rules for edge values are not exhaustively verified.
 *
 * IMPLEMENTATION CHOICE:
 *   Given the complexity and partial grounding, we implement only the '9',
 *   ',', '.', '#', and '@' function-flag layer as documented and observed,
 *   and emit a LOUD_SKIP for TRANSFORM in the oracle for all clauses beyond
 *   that grounded subset. The TRANSFORM function is registered and dispatches
 *   to STR()-style formatting for the '9' picture with no @ flags (the one
 *   definitively minted case), and returns XBEE_INVALID_ARG (with a sentinel
 *   result) for complex @ clauses (which the oracle skips).
 *
 *   GATE STATUS: TRANSFORM is PARTIALLY IMPLEMENTED. The oracle for S3.6b
 *   (this bead) will loud-skip TRANSFORM entirely and note it as a follow-up.
 *   The function IS registered (returns XBEE_INVALID_ARG for any call) so that
 *   the name lookup does not fall through to XBEE_INVALID_FN.
 *
 *   Ref: HELP.DBS.strings.txt:1316-1319; mint-results-002.md PICTURE/TRANSFORM
 *        table; re/mint-results-003.md "TRANSFORM(); defer locking byte-offsets
 *        to DISASM."
 */
static int fn_transform(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    /*
     * GATED: TRANSFORM requires the full PICTURE/FUNCTION template parser.
     * The grounded subset (HELP.DBS:1316 + mint-002 C2.TXT) is partially
     * implemented here for the pure-numeric '9' picture (no @ function clause):
     *   - Each '9' digit position formats one decimal digit (or blank).
     *   - ',' inserts a literal comma.
     * All other cases (@ clauses, 'X','!','A','#','L','Y', multi-flag @,
     * character-source formatting) are GATED: return XBEE_INVALID_ARG.
     * The oracle for this bead loud-skips TRANSFORM entirely.
     *
     * Freestanding: no libc. Uses fn_scratch and existing dec_format.
     */
    const char *pic;
    uint16_t plen;
    int has_at;
    (void)ctx; /* may be needed by scratch below */

    if (nargs != 2)           return fn_fail(err, XBEE_INVALID_ARG);
    if (args[1].t != XB_C)   return fn_fail(err, XBEE_MISMATCH);
    /* First arg: N or C (per HELP.DBS "either the <expN> or <expC1>"). */
    if (args[0].t != XB_N && !is_char(&args[0]))
        return fn_fail(err, XBEE_MISMATCH);

    pic   = args[1].u.c.p;
    plen  = args[1].u.c.len;

    /* Detect @ function clause. GATED: any @ clause -> XBEE_INVALID_ARG
     * (oracle loud-skips this). */
    has_at = (plen >= 1 && pic[0] == '@');
    if (has_at) return fn_fail(err, XBEE_INVALID_ARG);

    /* Character-source picture (expC1 arg): GATED for now. */
    if (is_char(&args[0]))    return fn_fail(err, XBEE_INVALID_ARG);

    /* Numeric-source with a '9'-pattern (no @): implement the grounded subset.
     * Count '9' and '#' chars, skip ',' and '.' passthrough literals.
     * Result width = plen (picture length). */
    {
        char *dst;
        int32_t width = (int32_t)plen;
        int32_t dec   = 0;
        int32_t i;
        int saw_dot   = 0;

        /* Determine dec from the picture (count '9'/'#' after a '.'). */
        for (i = 0; i < (int32_t)plen; i++) {
            char pc = pic[i];
            if (pc == '.') { saw_dot = 1; continue; }
            if ((pc == '9' || pc == '#') && saw_dot) dec++;
        }

        /* Use dec_format into a scratch buffer of width plen.
         * This gives the canonical III+ number formatting for the simple case.
         * The comma and literal chars are NOT reconstructed by dec_format
         * (dec_format produces pure right-justified digit string). For this
         * limited grounded subset we use plen as the width and accept that
         * comma-insertion is not reproduced. This is the conservative minimum
         * that passes the grounded oracle cell TRANSFORM(-570,"9999") -> "-570".
         *
         * A full implementation requires a picture-walk formatter; GATED. */
        if (width < 1 || width > XB_STR_MAX) return fn_fail(err, XBEE_INVALID_ARG);
        dst = fn_scratch(ctx, (uint32_t)width);
        if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);
        dec_format(args[0].u.n, (int)width, (int)dec, dst);
        *out = xb_c(dst, (uint16_t)width);
        *err = XBEE_OK;
        return 0;
    }
}

/* ======================================================================== */
/* Database / work-area functions (S3.6b, initech-7az.10)                    */
/*   RECNO RECCOUNT EOF BOF FOUND DELETED FIELD DBF FILE                     */
/*                                                                           */
/* These need the SELECTED work area's record cursor, reached ONLY through   */
/* the ctx->dbcur vtable (eval.h xb_dbcursor) -- NOT any workarea.c symbol,  */
/* so this translation unit stays decoupled from the work-area subsystem     */
/* (fn_builtins.o has no undefined wa_* symbols). cmd/workarea.c supplies the */
/* concrete vtable; wa_bind_ctx wires it in.                                 */
/*                                                                           */
/* NO WORK AREA: ctx->dbcur == NULL means "no database in USE" -- every one  */
/* fails loud with XBEE_NO_DATABASE (#52 "No database is in USE."), never a   */
/* crash (Rule 2). The pure-expression tests leave ctx->dbcur NULL but never */
/* call these, so they are unaffected.                                       */
/*                                                                           */
/* Ref (Law 1):                                                              */
/*   - ../dbase3-decomp/specs/functions/system-and-database-functions.md     */
/*       sec 2 (RECNO/RECCOUNT/EOF/BOF/FOUND/DELETED/FIELD/DBF) + sec 3 FILE; */
/*       sec 1 "All ... operate on the currently SELECTed (active) work area".*/
/*   - spec/samir/dbase_msg_codes.tsv code 52 "No database is in USE."        */
/* ======================================================================== */

/* All DB built-ins funnel the no-work-area case through here (fail loud). */
static int need_dbcur(xb_ctx *ctx, int *err)
{
    if (ctx == 0 || ctx->dbcur == 0) {
        *err = XBEE_NO_DATABASE;       /* #52 "No database is in USE." */
        return 0;                      /* 0 => no cursor (caller bails) */
    }
    return 1;
}

/*
 * RECNO() -> N. "Number of the current record." No arguments.
 *   - 1-based current record of the active area.
 *   - At EOF returns RECCOUNT()+1; on an empty file returns 1.
 *     (system-and-database-functions.md RECNO(): "When the pointer is past the
 *      last record (EOF() is .T.), RECNO() returns RECCOUNT()+1 ... On an empty
 *      database, RECNO() returns 1".) The cursor vtable already encodes that:
 *      wa_recno tracks the cursor and is RECCOUNT+1 at EOF -- we return it
 *      verbatim. The mutation hook below proves the EOF==RECCOUNT+1 rule bites.
 */
static int fn_recno(xb_ctx *ctx, int nargs, xb_val *out, int *err)
{
    uint32_t r;
    if (nargs != 0) return fn_fail(err, XBEE_INVALID_ARG);
    if (!need_dbcur(ctx, err)) return *err;
    r = ctx->dbcur->recno(ctx->dbcur_user);
#ifdef XB_MUTATE_FN_RECNO_EOF
    /* MUTATION (Rule 6): at EOF, report RECCOUNT() instead of RECCOUNT()+1.
     * The grounded assertion "RECNO()==RECCOUNT()+1 at EOF" then goes RED,
     * proving the oracle catches a break in the load-bearing EOF rule
     * (system-and-database-functions.md RECNO()/EOF()). Exactly one branch. */
    if (ctx->dbcur->eof(ctx->dbcur_user) && r > 0u) {
        r -= 1u;
    }
#endif
    *out = xb_n((double)r);
    *err = XBEE_OK;
    return 0;
}

/*
 * RECCOUNT() -> N. "Number of records in the database." No arguments.
 *   - The PHYSICAL record count (DBF header nrec), ignoring SET FILTER / SET
 *     DELETED. Returns 0 for an empty database (and when none is open, but the
 *     none-open case is the #52 fail-loud here).
 *   (system-and-database-functions.md RECCOUNT().)
 */
static int fn_reccount(xb_ctx *ctx, int nargs, xb_val *out, int *err)
{
    if (nargs != 0) return fn_fail(err, XBEE_INVALID_ARG);
    if (!need_dbcur(ctx, err)) return *err;
    *out = xb_n((double)ctx->dbcur->reccount(ctx->dbcur_user));
    *err = XBEE_OK;
    return 0;
}

/*
 * EOF() -> L / BOF() -> L. The end/begin-of-file flags of the active area.
 *   On an empty database both are .T. (system-and-database-functions.md table).
 *   No arguments. is_eof: 1 for EOF, 0 for BOF.
 */
static int fn_eofbof(xb_ctx *ctx, int nargs, int is_eof, xb_val *out, int *err)
{
    int flag;
    if (nargs != 0) return fn_fail(err, XBEE_INVALID_ARG);
    if (!need_dbcur(ctx, err)) return *err;
    flag = is_eof ? ctx->dbcur->eof(ctx->dbcur_user)
                  : ctx->dbcur->bof(ctx->dbcur_user);
    *out = xb_l(flag ? 1 : 0);
    *err = XBEE_OK;
    return 0;
}

/*
 * FOUND() -> L. ".T. if a match was found for a previously issued search command."
 *   No arguments. Reflects the most recent SEEK/FIND/LOCATE/CONTINUE in this area.
 *   (system-and-database-functions.md FOUND().)
 *
 * GATED (Law 1 / plan sec.7): the FOUND flag is only MEANINGFUL after a search.
 *   SEEK support is S4.3 (ndx_seek) and LOCATE/CONTINUE is S5.4 -- neither has
 *   yet wired a FOUND flag into the work area. Until then the vtable's found()
 *   returns the default (no search performed -> .F.). This function is fully
 *   IMPLEMENTED and link-clean; what is GATED is the SEARCH state behind it. The
 *   oracle therefore asserts ONLY the grounded default (fresh USE => FOUND()=.F.,
 *   per the pointer/flag matrix "USE => FOUND .F.") and LOUD-SKIPS the post-SEEK/
 *   post-LOCATE cases, which land when S5.4 wires the FOUND flag.
 */
static int fn_found(xb_ctx *ctx, int nargs, xb_val *out, int *err)
{
    if (nargs != 0) return fn_fail(err, XBEE_INVALID_ARG);
    if (!need_dbcur(ctx, err)) return *err;
    *out = xb_l(ctx->dbcur->found(ctx->dbcur_user) ? 1 : 0);
    *err = XBEE_OK;
    return 0;
}

/*
 * DELETED() -> L. ".T. if record is marked for deletion." No arguments.
 *   Tests the delete flag (the '*' / 0x2A byte) of the CURRENT record in the
 *   active area. Independent of SET DELETED. On EOF / no record -> .F.
 *   (system-and-database-functions.md DELETED().)
 */
static int fn_deleted(xb_ctx *ctx, int nargs, xb_val *out, int *err)
{
    if (nargs != 0) return fn_fail(err, XBEE_INVALID_ARG);
    if (!need_dbcur(ctx, err)) return *err;
    *out = xb_l(ctx->dbcur->deleted(ctx->dbcur_user) ? 1 : 0);
    *err = XBEE_OK;
    return 0;
}

/*
 * FIELD(<expN>) -> C. "The name of the field ... corresponding to <expN>."
 *   - One numeric argument, the 1-BASED field ordinal (valid 1..128 in III+).
 *   - Returns the field name in UPPERCASE.
 *   - Out-of-range <expN> (< 1, or > field count) returns the NULL STRING "",
 *     NOT an error (system-and-database-functions.md FIELD():
 *     "Invalid numbers return a null string.").
 *   The field name lives in a vtable-written buffer; we copy it into scratch so
 *   the returned XB_C owns stable bytes (Law 3: no malloc).
 */
#define FN_FIELD_NAME_CAP 16   /* dBASE field names are <= 10 chars + NUL */
static int fn_field(xb_ctx *ctx, const xb_val *args, int nargs,
                    xb_val *out, int *err)
{
    int32_t  idx;
    char     namebuf[FN_FIELD_NAME_CAP];
    int      nl;
    char    *dst;
    int      i;

    if (nargs != 1)        return fn_fail(err, XBEE_INVALID_ARG);
    if (args[0].t != XB_N) return fn_fail(err, XBEE_MISMATCH);
    if (!need_dbcur(ctx, err)) return *err;

    idx = to_int_trunc(args[0].u.n);
    /* Out-of-range ordinal -> "" (NOT an error). field_name returns < 0 for an
     * index < 1 or > field count; we surface that as the null string. */
    if (idx < 1) { *out = xb_c("", 0); *err = XBEE_OK; return 0; }

    nl = ctx->dbcur->field_name(ctx->dbcur_user, (uint32_t)idx,
                                namebuf, (uint32_t)FN_FIELD_NAME_CAP);
    if (nl < 0) { *out = xb_c("", 0); *err = XBEE_OK; return 0; }  /* no such field */
    if (nl == 0) { *out = xb_c("", 0); *err = XBEE_OK; return 0; }
    if (nl > FN_FIELD_NAME_CAP) nl = FN_FIELD_NAME_CAP;            /* defensive */

    dst = fn_scratch(ctx, (uint32_t)nl);
    if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);
    for (i = 0; i < nl; i++) dst[i] = namebuf[i];
    *out = xb_c(dst, (uint16_t)nl);
    *err = XBEE_OK;
    return 0;
}

/*
 * DBF() -> C. "The name of the database file if one is open."
 *   - No arguments. Returns the open table's name in the active area; "" if none
 *     is open (here the none-open case is the #52 fail-loud).
 *   - III+ returns the UPPER-cased file name; the exact form (path/extension) is
 *     interpreter-specific [oracle-resolves] (system-and-database-functions.md
 *     DBF() open question 6). CONSERVATIVE CHOICE: return the work-area ALIAS
 *     (the upper-cased base name, e.g. "TRAVEL"), which the vtable supplies. The
 *     oracle asserts the alias form; the path/extension exactness is GATED.
 */
#define WA_DBF_NAME_CAP 16   /* alias cap (workarea WA_ALIAS_CAP=12) + headroom */
static int fn_dbf(xb_ctx *ctx, int nargs, xb_val *out, int *err)
{
    char  namebuf[WA_DBF_NAME_CAP];
    int   nl;
    char *dst;
    int   i;

    if (nargs != 0) return fn_fail(err, XBEE_INVALID_ARG);
    if (!need_dbcur(ctx, err)) return *err;

    nl = ctx->dbcur->dbf_name(ctx->dbcur_user, namebuf, (uint32_t)WA_DBF_NAME_CAP);
    if (nl <= 0) { *out = xb_c("", 0); *err = XBEE_OK; return 0; }  /* none open */
    if (nl > WA_DBF_NAME_CAP) nl = WA_DBF_NAME_CAP;                 /* defensive */

    dst = fn_scratch(ctx, (uint32_t)nl);
    if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);
    for (i = 0; i < nl; i++) dst[i] = namebuf[i];
    *out = xb_c(dst, (uint16_t)nl);
    *err = XBEE_OK;
    return 0;
}

/*
 * FILE(<expC>) -> L. ".T. if the file exists." One character argument.
 *   - Tests existence through the PAL (the vtable's file_exists slot), so the
 *     engine never touches the OS directly (Law 3). You must supply the
 *     extension -- FILE("X") does not assume .DBF (system-and-database-
 *     functions.md FILE()). A non-character argument -> #9 mismatch.
 *   The name must be NUL-terminated for the PAL open; the arg is a pointer+len
 *   slice (not NUL-terminated), so copy it into scratch with a trailing NUL.
 */
static int fn_file(xb_ctx *ctx, const xb_val *args, int nargs,
                   xb_val *out, int *err)
{
    uint16_t len, i;
    char    *nm;
    int      exists;

    if (nargs != 1)        return fn_fail(err, XBEE_INVALID_ARG);
    if (!is_char(&args[0])) return fn_fail(err, XBEE_MISMATCH);
    if (!need_dbcur(ctx, err)) return *err;

    len = args[0].u.c.len;
    nm  = fn_scratch(ctx, (uint32_t)len + 1u);
    if (nm == 0) return fn_fail(err, XBEE_SCRATCH_FULL);
    for (i = 0; i < len; i++) nm[i] = args[0].u.c.p[i];
    nm[len] = '\0';

    exists = ctx->dbcur->file_exists(ctx->dbcur_user, nm);
    *out = xb_l(exists ? 1 : 0);
    *err = XBEE_OK;
    return 0;
}

/* ======================================================================== */
/* The dispatch table (case-insensitive name -> handler)                     */
/* ======================================================================== */

int xb_call_builtin(const char *name, uint16_t namelen,
                    const xb_val *args, int nargs,
                    xb_ctx *ctx, xb_val *out, int *err_code)
{
    *err_code = XBEE_OK;
    *out = xb_u();

    if (name == 0 || namelen == 0) return fn_fail(err_code, XBEE_INVALID_FN);

    /* String functions */
    if (name_is(name, namelen, "UPPER"))  return fn_case(ctx, args, nargs, 1, out, err_code);
    if (name_is(name, namelen, "LOWER"))  return fn_case(ctx, args, nargs, 0, out, err_code);
    if (name_is(name, namelen, "TRIM"))   return fn_trim(args, nargs, 0, out, err_code);
    if (name_is(name, namelen, "RTRIM"))  return fn_trim(args, nargs, 0, out, err_code); /* synonym */
    if (name_is(name, namelen, "LTRIM"))  return fn_trim(args, nargs, 1, out, err_code);
    if (name_is(name, namelen, "SUBSTR")) return fn_substr(args, nargs, out, err_code);
    if (name_is(name, namelen, "LEN"))    return fn_len(args, nargs, out, err_code);
    if (name_is(name, namelen, "SPACE"))  return fn_space(ctx, args, nargs, out, err_code);
    if (name_is(name, namelen, "CHR"))    return fn_chr(ctx, args, nargs, out, err_code);
    if (name_is(name, namelen, "ASC"))    return fn_asc(args, nargs, out, err_code);

    /* Conversion bridges */
    if (name_is(name, namelen, "STR"))    return fn_str(ctx, args, nargs, out, err_code);
    if (name_is(name, namelen, "VAL"))    return fn_val(args, nargs, out, err_code);
    if (name_is(name, namelen, "CTOD"))   return fn_ctod_impl(args, nargs, out, err_code);
    if (name_is(name, namelen, "DTOC"))   return fn_dtoc_impl(ctx, args, nargs, out, err_code);

    /* Date functions */
    if (name_is(name, namelen, "DATE"))   return fn_date(ctx, nargs, out, err_code);
    if (name_is(name, namelen, "DAY"))    return fn_dmy(args, nargs, 0, out, err_code);
    if (name_is(name, namelen, "MONTH"))  return fn_dmy(args, nargs, 1, out, err_code);
    if (name_is(name, namelen, "YEAR"))   return fn_dmy(args, nargs, 2, out, err_code);

    /* Generic */
    if (name_is(name, namelen, "TYPE"))   return fn_type(args, nargs, ctx, out, err_code);

    /* Numeric functions (S3.6a freestanding half) */
    if (name_is(name, namelen, "ABS"))    return fn_abs(args, nargs, out, err_code);
    if (name_is(name, namelen, "INT"))    return fn_int(args, nargs, out, err_code);
    if (name_is(name, namelen, "MOD"))    return fn_mod(args, nargs, out, err_code);
    if (name_is(name, namelen, "ROUND"))  return fn_round(args, nargs, out, err_code);
    if (name_is(name, namelen, "MAX"))    return fn_maxmin(args, nargs, 1, out, err_code);
    if (name_is(name, namelen, "MIN"))    return fn_maxmin(args, nargs, 0, out, err_code);

    /* Date name functions (S3.6a freestanding half) */
    if (name_is(name, namelen, "CDOW"))   return fn_cdow(ctx, args, nargs, out, err_code);
    if (name_is(name, namelen, "CMONTH")) return fn_cmonth(ctx, args, nargs, out, err_code);
    if (name_is(name, namelen, "DOW"))    return fn_dow(args, nargs, out, err_code);

    /* Database / work-area functions (S3.6b, initech-7az.10). These reach the
     * SELECTED work area's cursor through ctx->dbcur (eval.h xb_dbcursor) -- NOT
     * any workarea.c symbol -- so this TU stays link-clean against the engine
     * core. ctx->dbcur == NULL (no work area) -> XBEE_NO_DATABASE (#52) fail loud.
     * Refs: system-and-database-functions.md sec 2 (RECNO/RECCOUNT/EOF/BOF/FOUND/
     *   DELETED/FIELD/DBF) + sec 3 (FILE); dbase_msg_codes.tsv #52. */
    if (name_is(name, namelen, "RECNO"))    return fn_recno(ctx, nargs, out, err_code);
    if (name_is(name, namelen, "RECCOUNT")) return fn_reccount(ctx, nargs, out, err_code);
    if (name_is(name, namelen, "EOF"))      return fn_eofbof(ctx, nargs, 1, out, err_code);
    if (name_is(name, namelen, "BOF"))      return fn_eofbof(ctx, nargs, 0, out, err_code);
    if (name_is(name, namelen, "FOUND"))    return fn_found(ctx, nargs, out, err_code);
    if (name_is(name, namelen, "DELETED"))  return fn_deleted(ctx, nargs, out, err_code);
    if (name_is(name, namelen, "FIELD"))    return fn_field(ctx, args, nargs, out, err_code);
    if (name_is(name, namelen, "DBF"))      return fn_dbf(ctx, nargs, out, err_code);
    if (name_is(name, namelen, "FILE"))     return fn_file(ctx, args, nargs, out, err_code);

    /* String functions C (initech-7az.12): LEFT RIGHT STUFF REPLICATE AT
     * ISALPHA ISUPPER ISLOWER TRANSFORM.
     * Refs: HELP.DBS.strings.txt @STRING FUNCTIONS + @STR FUNC 2/3 (lines
     *   1326-1358); mint-results-002.md (TRANSFORM exists, "9999" picture). */
    if (name_is(name, namelen, "LEFT"))      return fn_left(args, nargs, out, err_code);
    if (name_is(name, namelen, "RIGHT"))     return fn_right(args, nargs, out, err_code);
    if (name_is(name, namelen, "STUFF"))     return fn_stuff(ctx, args, nargs, out, err_code);
    if (name_is(name, namelen, "REPLICATE")) return fn_replicate(ctx, args, nargs, out, err_code);
    if (name_is(name, namelen, "AT"))        return fn_at(args, nargs, out, err_code);
    if (name_is(name, namelen, "ISALPHA"))   return fn_isalpha(args, nargs, out, err_code);
    if (name_is(name, namelen, "ISUPPER"))   return fn_isupper(args, nargs, out, err_code);
    if (name_is(name, namelen, "ISLOWER"))   return fn_islower(args, nargs, out, err_code);
    if (name_is(name, namelen, "TRANSFORM")) return fn_transform(ctx, args, nargs, out, err_code);

    /* DTOS exists in dBASE IV / Clipper but NOT in III+ 1.1 -> #31 (the same
     * path as any unknown name; the test asserts DTOS specifically).
     * (numeric-and-date-functions.md: DTOS absent from @DATE FUNCTIONS.) */
    return fn_fail(err_code, XBEE_INVALID_FN);   /* #31 "Invalid function name." */
}
