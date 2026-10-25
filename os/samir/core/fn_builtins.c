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

#include "samir/lifetime.h"
#include <stdint.h>

#include "samir/eval.h"
#include "samir/pal.h"
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

#ifndef XB_MUTATE_FN_CTOD_CALVALID
/*
 * xb_is_leap_year: proleptic Gregorian leap-year rule.
 * Ref: the same Meeus Ch.7 Gregorian formula already cited for
 * jdn_from_ymd/ymd_from_jdn (os/samir/include/samir/rt.h S2). Divisible by 4,
 * EXCEPT century years not divisible by 400: 1900 is NOT leap, 2000 IS leap,
 * 2100 is NOT leap. All within the CTOD valid range 1900-2155 (rt.h).
 *
 * Compiled out entirely under -DXB_MUTATE_FN_CTOD_CALVALID (Rule 6 mutant;
 * see fn_ctod_impl below) -- its only caller, xb_days_in_month, is compiled
 * out too, so leaving this in would be an unused-function -Werror.
 */
static int xb_is_leap_year(int yy)
{
    return (yy % 4 == 0 && yy % 100 != 0) || (yy % 400 == 0);
}

/*
 * xb_days_in_month: calendar day cap for a 1-based month in year yy.
 * Ref: standard Gregorian calendar (same source as xb_is_leap_year above).
 * Caller must already have range-checked mm to [1..12]; out-of-range mm
 * returns 0 (fails any dd > 0 comparison, i.e. always calendar-invalid).
 */
static int xb_days_in_month(int mm, int yy)
{
    static const int days_tbl[13] =
        { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (mm < 1 || mm > 12) return 0;
    if (mm == 2 && xb_is_leap_year(yy)) return 29;
    return days_tbl[mm];
}
#endif

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

/* Private fn_* handlers receive arguments validated by fn_signature in the
 * dispatcher below. Keep domain checks and environment queries in handlers. */

/* ======================================================================== */
/* String functions                                                          */
/* ======================================================================== */

/* UPPER(<expC>) / LOWER(<expC>) -> C. Per-byte ASCII fold into scratch. */
static int fn_case(xb_ctx *ctx, const xb_val *args,
                   int up, xb_val *out, int *err)
{
    uint16_t len, i;
    char *dst;

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
static int fn_trim(const xb_val *args, int leading,
                   xb_val *out, int *err)
{
    uint16_t len;
    const char *p;

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
static int fn_len(const xb_val *args, xb_val *out, int *err)
{
    *out = xb_n((double)args[0].u.c.len);
    *err = XBEE_OK;
    return 0;
}

/* SPACE(<expN>) -> C: n blanks. SPACE(0)="". n<0 -> #60. n>254 -> #59. */
static int fn_space(xb_ctx *ctx, const xb_val *args,
                    xb_val *out, int *err)
{
    int32_t n, i;
    char *dst;

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
static int fn_chr(xb_ctx *ctx, const xb_val *args,
                  xb_val *out, int *err)
{
    int32_t n;
    char *dst;

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
static int fn_asc(const xb_val *args, xb_val *out, int *err)
{
    if (args[0].u.c.len == 0) { *out = xb_n(0.0); *err = XBEE_OK; return 0; }
    *out = xb_n((double)(uint8_t)args[0].u.c.p[0]);
    *err = XBEE_OK;
    return 0;
}

/* ======================================================================== */
/* Conversion bridges                                                         */
/* ======================================================================== */

/* STR(<expN1> [, <expN2> [, <expN3>]]) -> C. Right-justified width <expN2>
 * (default 10), <expN3> decimals (default 0). Uses rt.h dec_format (ties ->
 * +inf, '*'-overflow-fill -- the minted rule).
 *   - width default 10 (string-functions.md general-expression default).
 *   - dec default: ALWAYS 0, for ALL call forms that omit <expN3>.
 *     STR() does NOT read ctx->set_decimals under any form.
 *     [verified: ../dbase3-decomp/specs/runtime/numeric-and-string-formatting.md
 *      lines 11-13 "default decimals 0"; line 33 "SET DECIMALS scope is the
 *      closed list division/SQRT/LOG/VAL/EXP -- does NOT affect STR".
 *      Corroborated: re/mint-results-001.md STR(-570,4)='-570' (no-dec form
 *      -> 0 decimals confirmed on real III+).]
 *   - width/dec must be >= 0 and width within the field cap; else #63.
 *
 * NOTE on SET DECIMALS and mutation:
 *   ctx->set_decimals is STORED in the ctx (eval.h / set.c) for future use by
 *   division/SQRT/LOG/VAL/computed-display (numeric-and-string-formatting.md
 *   line 33 -- the correct scope, a separate follow-up bead). STR ignores it.
 *   The mutation hook for the date formatter path is -DXB_MUTATE_SETFMT_IGNORE
 *   (see fn_ctod_impl / fn_dtoc_impl below); that macro no longer gates STR.
 */
static int fn_str(xb_ctx *ctx, const xb_val *args, int nargs,
                  xb_val *out, int *err)
{
    int32_t width = 10;   /* default general-expression width (Open Q3)        */
    int32_t dec   = 0;    /* default 0 -- STR never reads SET DECIMALS [verified] */
    char *dst;

    if (nargs >= 2) {
        width = to_int_trunc(args[1].u.n);
        /* 2-arg STR(n, width): dec stays 0. SET DECIMALS does NOT govern STR
         * in any call form (numeric-and-string-formatting.md lines 11-13, 33). */
    }
    /* 1-arg STR(n): dec stays 0. STR never reads ctx->set_decimals.
     * [verified: numeric-and-string-formatting.md lines 11-13 + mint-001] */
    if (nargs == 3) {
        dec = to_int_trunc(args[2].u.n);   /* 3-arg always overrides */
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

/* VAL(<expC>) -> N: leading numeric portion (rt.h dec_parse). A first-byte
 * '*' is the overflow sentinel, DBL_MAX (real III+ datatype-mint DT5 I7).
 * Other non-numeric / empty strings -> 0. */
static int fn_val(const xb_val *args, xb_val *out, int *err)
{
    *out = xb_n(dec_parse(args[0].u.c.p, (int)args[0].u.c.len));
#ifndef SAMIR_DT_MUTANT_valstar
    /* Do not trim: real III+ distinguishes "*" from " *" (DT6 V01/V03). */
    if (args[0].u.c.len && args[0].u.c.p[0] == '*')
        *out = xb_n(1.7976931348623157e308);
#endif
    /* PROG-funcs real III+ mint: VAL('42') prints 42.00 (five columns). */
    out->num_dec=2; out->num_width=args[0].u.c.len+3;
    *err = XBEE_OK;
    return 0;
}

/* ======================================================================== */
/* Date functions                                                            */
/* ======================================================================== */

/*
 * ctod_parse_digits -- helper: collect digit bytes from a date string.
 * Strips all non-digit bytes (separators '/', '.', '-', space). Returns count.
 * buf must be at least 12 bytes. Ref: dates-and-century.md (separator stripping).
 */
static int ctod_collect_digits(const char *p, uint16_t len, char *buf, int cap)
{
    int nd = 0;
    uint16_t i;
    for (i = 0; i < len && nd < cap - 1; i++) {
        char c = p[i];
        if (c >= '0' && c <= '9') buf[nd++] = c;
    }
    buf[nd] = '\0';
    return nd;
}

/* CTOD(<expC>) -> D. Parses a date string using ctx->set_date_fmt and
 * ctx->set_century. Unparseable / blank -> the blank date (JDN 0), NOT an
 * error (numeric-and-date-functions.md).
 *
 * Format order (set-commands.md Sec 3.2; dates-and-century.md):
 *   AMERICAN (0): MM/DD/YY[YY]  -- III+ default
 *   ANSI     (1): YY[YY].MM.DD
 *   BRITISH  (2): DD/MM/YY[YY]
 *   ITALIAN  (3): DD-MM-YY[YY]
 *   FRENCH   (4): DD/MM/YY[YY]  (same picture as BRITISH)
 *   GERMAN   (5): DD.MM.YY[YY]
 *   JAPAN    (6): YY[YY]/MM/DD
 *   USA      (7): MM-DD-YY[YY]
 *
 * 2-digit year rule: base-1900 (no sliding window; '00'->1900, '99'->1999).
 * [verified: dates-and-century.md base-1900 rule]
 *
 * CENTURY ON (ctx->set_century != 0): accept 10-char 4-digit year strings.
 * CENTURY OFF (default): accept 8-char 2-digit year strings.
 * A 4-digit-year string is accepted even under CENTURY OFF (CTOD is lenient;
 * dates-and-century.md "4-digit year still parses under CENTURY OFF").
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/commands/set-commands.md Sec 3.2 (format table).
 *   - ../dbase3-decomp/specs/runtime/dates-and-century.md (base-1900; format
 *     widths; CTOD blank-date for bad input).
 *   - ../dbase3-decomp/re/mint-results-003.md (DATE=AMERICAN default [verified]).
 *
 * MUTATION GUARD (-DXB_MUTATE_SETFMT_IGNORE):
 *   Force AMERICAN + CENTURY OFF regardless of ctx, so SET DATE ANSI +
 *   CTOD('1985.08.05') returns blank instead of the correct date, and the
 *   unit assertion goes RED.
 */
static int fn_ctod_impl(const xb_val *args,
                        xb_ctx *ctx, xb_val *out, int *err)
{
    const char *p;
    uint16_t len;
    char digbuf[12];
    int  nd;
    int  mm, dd, yy;
    int  yearlen;
    uint8_t fmt, century;

    p   = args[0].u.c.p;
    len = args[0].u.c.len;

#ifdef XB_MUTATE_SETFMT_IGNORE
    (void)ctx;      /* ctx not read under mutation; suppress -Wunused-parameter */
    fmt     = (uint8_t)XB_DATE_AMERICAN;
    century = 0;
#else
    fmt     = ctx->set_date_fmt;
    century = ctx->set_century;
#endif
    (void)century;  /* used below for yearlen selection only */

    /* Collect digits; strip separators. */
    nd = ctod_collect_digits(p, len, digbuf, (int)sizeof(digbuf));

    /* Need exactly 6 (2-digit year) or 8 (4-digit year) digit bytes.
     * Ref: dates-and-century.md (format width table). */
    if (nd != 6 && nd != 8) {
        *out = xb_d(0.0); *err = XBEE_OK; return 0;   /* blank date */
    }
    yearlen = (nd == 8) ? 4 : 2;

    /*
     * Map the three two-digit groups (plus optional extra year digits) to
     * mm/dd/yy according to the format.
     *
     * Layout of digbuf[]:
     *   6-digit (CENTURY OFF): [g0-1][g1-2][g2-3] = three 2-digit groups
     *   8-digit (CENTURY ON):  [g0-1][g1-2][g2-5] = two 2-digit + one 4-digit
     *
     * For formats where the year is FIRST (ANSI, JAPAN), yearlen digits start
     * at position 0 (not 4). For formats where year is LAST (all others),
     * yearlen digits start at position 4.
     *
     * Ref: set-commands.md Sec 3.2 (format table; ANSI/JAPAN year-first).
     */
    {
        int g0, g1, gyy;
        int year_first = (fmt == XB_DATE_ANSI || fmt == XB_DATE_JAPAN);

        if (year_first) {
            /* YY[YY].MM.DD or YY[YY]/MM/DD: year is first group. */
            int j;
            gyy = 0;
            for (j = 0; j < yearlen; j++) gyy = gyy * 10 + (digbuf[j] - '0');
            g0 = (digbuf[yearlen]     - '0') * 10 + (digbuf[yearlen + 1] - '0');
            g1 = (digbuf[yearlen + 2] - '0') * 10 + (digbuf[yearlen + 3] - '0');
            /* ANSI: YY.MM.DD -> g0=mm, g1=dd.
             * JAPAN: YY/MM/DD -> g0=mm, g1=dd. */
            mm = g0; dd = g1; yy = gyy;
        } else {
            /* All other formats: first two groups are [f0][f1], year is last. */
            g0 = (digbuf[0] - '0') * 10 + (digbuf[1] - '0');
            g1 = (digbuf[2] - '0') * 10 + (digbuf[3] - '0');
            {
                int j;
                gyy = 0;
                for (j = 4; j < 4 + yearlen; j++) gyy = gyy * 10 + (digbuf[j] - '0');
            }
            yy = gyy;
            /* Determine which group is MM and which is DD. */
            switch (fmt) {
            case XB_DATE_AMERICAN:  /* MM/DD/YY */
            case XB_DATE_USA:       /* MM-DD-YY */
                mm = g0; dd = g1; break;
            case XB_DATE_BRITISH:   /* DD/MM/YY */
            case XB_DATE_ITALIAN:   /* DD-MM-YY */
            case XB_DATE_FRENCH:    /* DD/MM/YY */
            case XB_DATE_GERMAN:    /* DD.MM.YY */
            default:
                dd = g0; mm = g1; break;
            }
        }

        if (yearlen == 2) yy += 1900;  /* base-1900 [verified: dates-and-century.md] */

        /* Validate. An invalid field -> blank date (CTOD of a bad string). */
        if (mm < 1 || mm > 12 || dd < 1 || dd > 31 || yy < 1900 || yy > 2155) {
            *out = xb_d(0.0); *err = XBEE_OK; return 0;
        }
#ifndef XB_MUTATE_FN_CTOD_CALVALID
        /*
         * initech-9u0f: the range check above (dd in [1..31]) is necessary
         * but NOT sufficient -- April has 30 days, February has 28 or 29.
         * Without this cap, CTOD('02/31/90') silently rolls forward into
         * jdn_from_ymd(1990,2,31), producing 1990-03-03 instead of the
         * blank date. Real CTOD treats a calendar-invalid string the same
         * as any other "improper" string -> blank date, NOT a rollover.
         * Ref: ../dbase3-decomp/specs/functions/numeric-and-date-functions.md
         *   lines 462-469 ("Invalid string -> blank date"; Harbour
         *   datetime.txt:146-147 "If an improper character string is passed
         *   to the function, an empty date value will be returned").
         * Ref (calendar rule): xb_is_leap_year/xb_days_in_month above, same
         *   Meeus Ch.7 Gregorian source already cited for jdn_from_ymd.
         *
         * MUTATION GUARD (-DXB_MUTATE_FN_CTOD_CALVALID): drop this cap,
         * reverting to the pre-fix behavior (calendar-invalid dd rolls
         * forward instead of returning blank). Rule 6.
         */
        if (dd > xb_days_in_month(mm, yy)) {
            *out = xb_d(0.0); *err = XBEE_OK; return 0;
        }
#endif
        *out = xb_d((double)jdn_from_ymd(yy, mm, dd));
        *err = XBEE_OK;
        return 0;
    }
}

/* DTOC(<expD>) -> C. Renders a Date using ctx->set_date_fmt and ctx->set_century.
 * Blank date -> blank digits with the picture's separators retained.
 * [real III+ datatype-mint DT2 E1/E2, DT3 G10; DT6 D01..D05]
 *
 * CENTURY OFF (default): 2-digit year, width=8.
 * CENTURY ON:            4-digit year, width=10 (extra 2 chars for full year).
 *
 * Format table (CENTURY OFF / CENTURY ON):
 *   AMERICAN:  MM/DD/YY (8) / MM/DD/YYYY (10)
 *   ANSI:      YY.MM.DD (8) / YYYY.MM.DD (10)
 *   BRITISH:   DD/MM/YY (8) / DD/MM/YYYY (10)
 *   ITALIAN:   DD-MM-YY (8) / DD-MM-YYYY (10)
 *   FRENCH:    DD/MM/YY (8) / DD/MM/YYYY (10)
 *   GERMAN:    DD.MM.YY (8) / DD.MM.YYYY (10)
 *   JAPAN:     YY/MM/DD (8) / YYYY/MM/DD (10)
 *   USA:       MM-DD-YY (8) / MM-DD-YYYY (10)
 * [verified: set-commands.md Sec 3.2; dates-and-century.md format table]
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/commands/set-commands.md Sec 3.2 (format table).
 *   - ../dbase3-decomp/specs/runtime/dates-and-century.md (widths).
 *     Its inferred blank-spaces rule is superseded by the real III+ mint.
 *   - ../dbase3-decomp/re/mint-results-003.md (AMERICAN + CENTURY OFF default
 *     [verified]).
 *
 * MUTATION GUARD (-DXB_MUTATE_SETFMT_IGNORE):
 *   Force AMERICAN + CENTURY OFF regardless of ctx, so SET CENTURY ON +
 *   DTOC(date) returns 8-char 2-digit-year output instead of 10-char 4-digit,
 *   and the unit assertion goes RED.
 */
static int fn_dtoc_impl(xb_ctx *ctx, const xb_val *args,
                        xb_val *out, int *err)
{
    char *dst;
    uint8_t fmt, century;
    uint32_t width;

#ifdef XB_MUTATE_SETFMT_IGNORE
    fmt     = (uint8_t)XB_DATE_AMERICAN;
    century = 0;
#else
    fmt     = ctx->set_date_fmt;
    century = ctx->set_century;
#endif

    width = century ? 10u : 8u;
    dst = fn_scratch(ctx, width);
    if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);

    rt_date_format(args[0].u.d, fmt, century, dst, 0);
#ifdef SAMIR_DT_MUTANT_blankdtoc
    if (fn_date_blank(args[0].u.d)) rt_memset(dst, ' ', width);
#endif
    *out = xb_c(dst, (uint16_t)width);
    *err = XBEE_OK;
    return 0;
}

/* DATE() -> D: today's date from the INJECTED ctx->ctx_today (JDN). No args.
 * Freestanding: never reads the OS clock (Rule 11; pal.h today()). If the
 * caller has not injected a date, ctx_today is 0.0 -> a blank date. */
static int fn_date(xb_ctx *ctx, xb_val *out, int *err)
{
    *out = xb_d(ctx->ctx_today);
    *err = XBEE_OK;
    return 0;
}

/* DAY/MONTH/YEAR(<expD>) -> N. Blank date -> 0 (dates-and-century.md table).
 * which: 0=DAY, 1=MONTH, 2=YEAR. */
static int fn_dmy(const xb_val *args, int which,
                  xb_val *out, int *err)
{
    int32_t y, m, d;

    if (fn_date_blank(args[0].u.d)) { *out = xb_n(0.0); *err = XBEE_OK; return 0; }
    ymd_from_jdn((int32_t)args[0].u.d, &y, &m, &d);
    switch (which) {
    case 0:  *out = xb_n((double)d); break;
    case 1:  *out = xb_n((double)m); break;
    default: *out = xb_n((double)y); break;     /* YEAR is full 4-digit        */
    }
    /* Real III+ PROG-funcs YEAR result occupies five columns. */
    if(which==2) out->num_width=5;
    *err = XBEE_OK;
    return 0;
}

/* ======================================================================== */
/* TYPE(<expC>) -> C (1-char code)                                            */
/* ======================================================================== */

/* Using III+ U6-86/87: evaluate the expression contained in the Character
 * argument. Invalid or undefined expressions return U without raising them.
 * DML-01 is the real CLEAR MEMORY / TYPE transcript. */
static int fn_type(const xb_val *args, xb_ctx *ctx,
                   xb_val *out, int *err)
{
    char *dst, code='U';
    xb_token toks[128];
    xb_node nodes[128];
    xb_ctx inner;
    xb_val value;
    int nt,root,e;
    static unsigned depth;

    /* Using III+ U6-86/87: the Character argument contains an expression;
     * an absent name or invalid expression reports U, without raising it.
     * DML-01 real III+ transcript pins TYPE("N") after CLEAR MEMORY. */

    if(depth<8) {
        nt=xb_lex(args[0].u.c.p,args[0].u.c.len,toks,128,&e);
        root=nt<0?-1:xb_parse(toks,(uint32_t)nt,nodes,128,&e);
        if(root>=0) {
            inner=*ctx;
            if(inner.scratch) inner.scratch+=ctx->scratch_used;
            inner.scratch_cap-=ctx->scratch_used;
            inner.scratch_used=0;
            depth++;
            if(xb_eval(nodes,root,&inner,&value,&e)==0) code=xb_type_char(value.t);
            depth--;
        }
    }
    dst = fn_scratch(ctx, 1);
    if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);
    /* xb_type_char maps XB_U -> 'U' too, so an undefined value reports "U"
     * (the III+ "undefined/invalid expression" code). */
#ifdef SAMIR_FNS_MUTANT_type
    code='C';
#endif
    dst[0] = code;
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
static int fn_abs(const xb_val *args, xb_val *out, int *err)
{
    double v;

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
static int fn_int(const xb_val *args, xb_val *out, int *err)
{
    double v;

    /* PROVISIONAL/GATED (numfn-2): truncate toward zero via cast. */
    v = (double)(int64_t)args[0].u.n;
    *out = xb_n(v);
    out->num_width=args[0].num_width;
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
static int fn_mod(const xb_val *args, xb_val *out, int *err)
{
    double a, b, q;

    a = args[0].u.n;
    b = args[1].u.n;
    /* PROVISIONAL/GATED (numfn-3): MOD(a,0) -> a (zero-divisor not an error). */
    if (b == 0.0) { *out = xb_n(a); *err = XBEE_OK; return 0; }
    /* PROVISIONAL/GATED (numfn-3): truncated-quotient remainder.
     * INT(a/b) via (int64_t) cast (truncation toward zero). */
    q = (double)(int64_t)(a / b);
    *out = xb_n(a - b * q);
    out->num_width=(args[0].num_width>args[1].num_width?args[0].num_width:args[1].num_width)+1;
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
 *   Freestanding: uses the SAME add-0.5*10^-dec + rt_floor64 idiom as
 *     dec_format (rt.c; rt.h S3) -- rt_floor64, NOT a bare (int64_t) cast.
 *     initech-eyig: a bare (int64_t) cast TRUNCATES TOWARD ZERO, which only
 *     coincides with floor for non-negative operands. For v<0 it rounds the
 *     wrong way -- (int64_t)(-2.7*1+0.5) = (int64_t)(-2.2) = -2 (truncation-
 *     toward-zero), but the ties-toward-+infinity rule this function claims
 *     to follow requires floor(-2.2) = -3, i.e. ROUND(-2.7,0) must be -3.0,
 *     not -2.0. rt_floor64 (rt.h) is the SAME primitive dec_format already
 *     uses for exactly this reason; no libm.
 *   Ref: numeric-and-date-functions.md ROUND section [verified: HELP.DBS lines
 *        1307-1308 + Harbour math.txt:326-367].
 */
static int fn_round(const xb_val *args, xb_val *out, int *err)
{
    double v, scale, rounded;
    int    dec;
    int    i;

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

#ifdef XB_MUTATE_FN_ROUND_TRUNC
    /*
     * MUTATION (Rule 6, initech-eyig): revert to the pre-fix bare (int64_t)
     * truncating cast (truncates toward zero instead of flooring), so
     * negative-operand ROUND cases go wrong again -- ROUND(-2.7,0) would
     * return -2.0 instead of the correct -3.0. Compile with
     * -DXB_MUTATE_FN_ROUND_TRUNC. Positive-operand cases are UNAFFECTED by
     * this mutant (truncation == floor for v >= 0), matching the fact that
     * the original bug never manifested on positive input.
     */
    if (dec >= 0) {
        rounded = (double)(int64_t)(v * scale + 0.5) / scale;
    } else {
        rounded = (double)(int64_t)(v / scale + 0.5) * scale;
    }
#else
    if (dec >= 0) {
        /* Round to `dec` decimal places. PROVISIONAL: ties -> +inf. */
        rounded = (double)rt_floor64(v * scale + 0.5) / scale;
    } else {
        /* Round to |dec| whole-number places (e.g. dec=-2 -> nearest 100). */
        rounded = (double)rt_floor64(v / scale + 0.5) * scale;
    }
#endif
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
static int fn_maxmin(const xb_val *args, int is_max,
                     xb_val *out, int *err)
{
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
static int fn_dow(const xb_val *args, xb_val *out, int *err)
{
    int32_t jdn, dow;

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

static int fn_cdow(xb_ctx *ctx, const xb_val *args,
                   xb_val *out, int *err)
{
    int32_t jdn, dow;
    (void)ctx;    /* No scratch needed: result points at a static literal. */

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

static int fn_cmonth(xb_ctx *ctx, const xb_val *args,
                     xb_val *out, int *err)
{
    int32_t y, m, d;
    (void)ctx;    /* No scratch: result points at a static literal. */

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
static int fn_left(const xb_val *args, xb_val *out, int *err)
{
    int32_t n;
    uint16_t len;

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
static int fn_right(const xb_val *args, xb_val *out, int *err)
{
    int32_t n;
    uint16_t len;

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
static int fn_replicate(xb_ctx *ctx, const xb_val *args,
                        xb_val *out, int *err)
{
    int32_t n, i;
    uint16_t clen;
    uint32_t rlen;
    char *dst;
    uint16_t j;

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
static int fn_at(const xb_val *args, xb_val *out, int *err)
{
    uint16_t nlen, hlen, i, j;

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
static int fn_stuff(xb_ctx *ctx, const xb_val *args,
                    xb_val *out, int *err)
{
    int32_t  start, del;
    uint16_t src_len, rep_len;
    uint32_t res_len;
    int32_t  pre, suf_start, suf_len;
    char    *dst;
    uint32_t pos;

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
static int fn_isalpha(const xb_val *args, xb_val *out, int *err)
{
    char c;

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
static int fn_isupper(const xb_val *args, xb_val *out, int *err)
{
    char c;

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
static int fn_islower(const xb_val *args, xb_val *out, int *err)
{
    char c;

    if (args[0].u.c.len == 0) { *out = xb_l(0); *err = XBEE_OK; return 0; }
    c = args[0].u.c.p[0];
    *out = xb_l((c >= 'a' && c <= 'z') ? 1 : 0);
    *err = XBEE_OK;
    return 0;
}

/*
 * TRANSFORM(<expN>/<expC1>, <expC2>) -> C   (initech-7az.14 -- full engine).
 *
 * Ref (Law 1):
 *   - HELP.DBS.strings.txt:1316-1319 @NUM FUNC 3: "A character string created
 *     from either the <expN> or <expC1> in the format of <expC2>. Use @...SAY
 *     PICTURE options to format." [verified: mined HELP lines 1316-1319]
 *   - ../dbase3-decomp/re/mint-results-002.md C2.TXT [verified vs real III+]:
 *       TRANSFORM(-570,"9999")       -> "-570"
 *       TRANSFORM(-570,"@( 9999")   -> "( 570)"   (@( parens for negatives)
 *       TRANSFORM(-570,"@X 9999")   -> " 570 DB"  (@X trailing " DB")
 *       TRANSFORM(570, "@C 9999")   -> " 570 CR"  (@C trailing " CR")
 *       TRANSFORM(-570,"@B 9999")   -> "-570    " (@B left-justify in field)
 *       TRANSFORM(-1234.56,"99,999.99") -> "-1,234.56" (comma grouping + decimal)
 *       TRANSFORM(-12.5,"$$,$$9.99")   -> "$$$-12.50"  (floating $ fill)
 *       TRANSFORM(-570,"999-")       -> "***-"  (overflow; '-' is a literal)
 *   - ../dbase3-decomp/specs/functions/string-functions.md lines 562-600:
 *       Numeric template chars (III+ verified): 9, #, ., ,, $
 *       @-flags (III+ core, verified): @X, @C, @(, @B
 *       Character template chars (documented): ! (uppercase in place)
 *       Passthrough for C-source (documented): X, A, N (any char)
 *
 * GATED / LOUD-SKIPPED (not minted against real III+):
 *   @Z  (zero-blank)         -- listed in transfrm.txt; not minted vs III+
 *   @!  (force uppercase)    -- not minted against III+; [oracle-resolves]
 *   @R  (literal-insert mask)-- transfrm.txt marks as Harbour extension
 *   @S, @M, @E, @0, @L       -- display-state / Harbour; not III+ verified
 *   *   (float asterisk fill)-- in transfrm.txt; not minted against III+
 *   L, Y, 9, # for C-source picture -- [oracle-resolves]
 *   Combined @-flags ("@(B")  -- mint only tested single flags
 *
 * MUTATION (Rule 6):
 *   -DXB_MUTATE_TRANSFORM_PAREN: @( does NOT parenthesize negatives (returns
 *   the plain minus form). The grounded assertion
 *   TRANSFORM(-570,"@( 9999")="( 570)" then goes RED.
 *
 * Freestanding (Law 3): <stdint.h> + samir/ only. No libc, no libm.
 * ASCII-clean (Rule 12). Reproducible (Rule 11).
 */

/* Flags from tr_parse_at_flags: which @ clauses are present in the picture. */
#define TR_AT_PAREN  0x01   /* @( parens for negatives [verified: mint-002] */
#define TR_AT_DEBIT  0x02   /* @X trailing " DB"       [verified: mint-002] */
#define TR_AT_CREDIT 0x04   /* @C trailing " CR"       [verified: mint-002] */
#define TR_AT_LEFT   0x08   /* @B left-justify          [verified: mint-002] */

/* '9' and '#' both occupy numeric digit slots.
 * [verified: mint-002 "9999"; string-functions.md:581 "9/#(digit)"] */
static int tr_is_digit_slot(char c) { return c == '9' || c == '#'; }

/* '$' occupies a digit slot AND enables floating-dollar fill mode.
 * [verified: mint-002 "$$,$$9.99" -> "$$$-12.50"] */
static int tr_is_dollar_slot(char c) { return c == '$'; }

/*
 * tr_parse_at_flags -- parse the @ function segment of a picture string.
 *
 * If pic begins with '@', the flag chars follow '@' until the first space or
 * end; the template segment starts after that space.  Recognised flags:
 *   '(' -> TR_AT_PAREN    '(B')  -> TR_AT_PAREN | TR_AT_LEFT  (etc.)
 *   'X' -> TR_AT_DEBIT    'x'/'X' both accepted (fold to upper)
 *   'C' -> TR_AT_CREDIT
 *   'B' -> TR_AT_LEFT
 * Unknown letters in the @ segment are silently skipped (GATED: exact error
 * policy for unknown @ flags is [oracle-resolves]).
 *
 * Ref: mint-results-002.md C2.TXT: "@( 9999", "@X 9999", "@C 9999", "@B 9999"
 * all use exactly one space between flags and template.
 */
static int tr_parse_at_flags(const char *pic, uint16_t plen,
                              uint16_t *tmpl_start)
{
    int flags = 0;
    uint16_t i;

    if (plen == 0 || pic[0] != '@') { *tmpl_start = 0; return 0; }
    i = 1; /* skip '@' */
    while (i < plen && pic[i] != ' ') {
        char fc = pic[i];
        if (fc >= 'a' && fc <= 'z') fc = (char)(fc - 32);
        if      (fc == '(') flags |= TR_AT_PAREN;
        else if (fc == 'X') flags |= TR_AT_DEBIT;
        else if (fc == 'C') flags |= TR_AT_CREDIT;
        else if (fc == 'B') flags |= TR_AT_LEFT;
        /* GATED: @!, @R, @Z, @S, @M, @E -- not minted; silently skip. */
        i++;
    }
    if (i < plen && pic[i] == ' ') i++; /* skip separator space */
    *tmpl_start = i;
    return flags;
}

/*
 * tr_format_numeric -- picture-walk numeric formatter.
 *
 * Implements the grounded III+ picture algorithm for a numeric source value v
 * against template tmpl[0..tlen). at_flags selects @-clause post-processing.
 * suffix_extra = extra bytes needed after tlen (pre-computed by caller):
 *   0 = no suffix; 1 = ')' for @( negative; 3 = " DB" or " CR".
 *
 * Algorithm (grounded on mint-002 C2.TXT + string-functions.md):
 *
 * 1. Scan template: count digit slots (9, #, $), find decimal '.', dollar mode.
 * 2. Convert abs(v) at SET DECIMALS precision, then truncate/pad to n_dec.
 *    Ref: real III+ datatype-mint DT6 T01..T13/A1..D2.
 *    If fmtbuf[0]=='*': overflow. All digit slots -> '*'; literals pass through.
 *    [verified: mint-002 "999-" + -570 -> "***-"]
 * 3. No overflow: extract digit chars from fmtbuf (skip spaces and '.').
 *    Map digits RIGHT-TO-LEFT into output digit slots.  Leading unfilled slots
 *    stay ' ' (or '$' in dollar_fill mode after Step D).
 * 4. Sign placement (unless overflow):
 *    - Default/@B: put '-' in the first EMPTY digit slot to the left of the
 *      first actual digit. No room -> overflow.
 *    - @( negative: replace the leftmost non-digit-slot literal ' ' with '(';
 *      append ')' at dst[tlen].  No literal ' ' available -> overflow.
 *    - @( positive: no sign; dst[tlen] stays ' ' (for @( positive suffix byte).
 *    [verified: mint-002 "( 570)" from "@( 9999" + -570]
 * 5. Dollar fill: replace remaining ' ' in $-slots with '$'.
 *    [verified: mint-002 "$$$-12.50" from "$$,$$9.99" + -12.5]
 * 6. @B left-justify: shift all non-space content left, pad right with ' '.
 *    [verified: mint-002 "-570    " from "@B 9999" + -570]
 * 7. @X/@C suffixes: append " DB" (negative) or " CR" (positive) past tlen.
 *    [verified: mint-002 " 570 DB" from "@X 9999" + -570;
 *               " 570 CR" from "@C 9999" + 570]
 */
static int tr_format_numeric(xb_ctx *ctx, double v,
                             const char *tmpl, uint16_t tlen,
                             int at_flags, int32_t suffix_extra,
                             xb_val *out, int *err)
{
    /*
     * suffix_extra layout (pre-computed by fn_transform):
     *   0 = no suffix
     *   2 = @( negative: '(' prefix at dst[0], formatted field at dst[1..tlen],
     *          ')' at dst[tlen+1]  [output = tlen+2 chars]
     *   3 = @X negative: " DB" at dst[tlen..tlen+2]  [output = tlen+3 chars]
     *       @C positive: " CR" at dst[tlen..tlen+2]
     *
     * The @( case uses suffix_extra=2 and a paren_wrap layout: the output buffer
     * has '(' at dst[0], the tlen-char template field at dst[1..tlen], then ')'
     * at dst[tlen+1]. The digit-slot mapping writes into dst[1..tlen] (offset 1).
     *
     * All other cases (suffix_extra=0 or 3) write the template field at
     * dst[0..tlen-1] and any suffix at dst[tlen..].
     */
    int32_t  i;
    int32_t  n_digit = 0;
    int32_t  n_dec   = 0;
    int32_t  fmt_w;       /* dec_format width: digit slots + the '.' column */
    int      dot_seen    = 0;
    int      dollar_fill = 0;
    char     fmtbuf[256]; /* abs(v) formatted; n_digit <= 254 */
    int      is_neg, overflow;
    int      paren_wrap; /* 1 if @( negative with suffix_extra==2 */
    uint32_t out_len;
    char    *dst;
    int32_t  field_off; /* byte offset into dst where the tlen-char field starts */

    /* Step 1: scan template. */
    for (i = 0; i < (int32_t)tlen; i++) {
        char tc = tmpl[i];
        if (tc == '.') { dot_seen = 1; continue; }
        if (tr_is_digit_slot(tc))  { n_digit++; if (dot_seen) n_dec++; continue; }
        if (tr_is_dollar_slot(tc)) {
            n_digit++; dollar_fill = 1;
            if (dot_seen) n_dec++;
        }
    }

    /* Step 2: format abs(v). */
    is_neg     = (v < 0.0);
    paren_wrap = (suffix_extra == 2); /* @( negative layout */
    field_off  = paren_wrap ? 1 : 0;

    /* The template's '.' is a column of its own: "999.99" holds 123.40, so
     * dec_format gets n_digit digits PLUS the point. Formatting into n_digit
     * columns star-filled every value that exactly fits (real III+
     * TRANSFORM(123.4,"999.99") = "123.40", (0.12,"9.99") = "0.12",
     * (5,"9.9") = "5.0": datatype-mint DT1 C1..C11; audit 2026-10-23-samir
     * L007; initech-ylqs). Mutant XB_MUTATE_TR_DOTWIDTH drops the column. */
#ifndef XB_MUTATE_TR_DOTWIDTH
    fmt_w = n_digit + (n_dec > 0 ? 1 : 0);
#else
    fmt_w = n_digit;
#endif
    if (n_digit > 0) {
        double absv = is_neg ? -v : v;
#ifndef XB_MUTATE_TR_ROUNDING
        char full[256];
        int dec = ctx->set_decimals, start = 0, end, digits, need, at;
        if (dec > 18) dec = 18;
        /* III+ converts at SET DECIMALS precision FIRST, then copies the
         * picture's fractional slots (truncating or zero-padding). DT1 C10
         * and DT6 T01..T11/A1..D2 distinguish this from direct rounding at
         * n_dec, truncation of the original value, and nearest-even at n_dec.
         * E.g. 12.5 -> "12", 2.999 -> "3", and 1.23456 -> "1.2300" at
         * DECIMALS 2, but "1.2350" at DECIMALS 3. */
        dec_format_even(absv, 255, dec, full);
        while (start < 255 && full[start] == ' ') start++;
        end = 255 - (dec ? dec + 1 : 0);
        digits = end - start;
        need = digits + (n_dec ? n_dec + 1 : 0);
        if (full[0] == '*' || need > fmt_w) {
            for (i = 0; i < fmt_w; i++) fmtbuf[i] = '*';
        } else {
            at = fmt_w - need;
            for (i = 0; i < at; i++) fmtbuf[i] = ' ';
            for (i = start; i < end; i++) fmtbuf[at++] = full[i];
            if (n_dec) {
                fmtbuf[at++] = '.';
                for (i = 0; i < n_dec; i++)
                    fmtbuf[at++] = i < dec ? full[end + 1 + i] : '0';
            }
        }
#else
        dec_format(absv, (int)fmt_w, (int)n_dec, fmtbuf);
#endif
    }
    overflow = (n_digit > 0 && fmtbuf[0] == '*');

    /* Allocate output buffer: tlen + suffix_extra. */
    out_len = (uint32_t)tlen + (uint32_t)suffix_extra;
    if (out_len > XB_STR_MAX) return fn_fail(err, XBEE_SPACE_LARGE);
    dst = fn_scratch(ctx, out_len > 0 ? out_len : 1);
    if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);

    if (n_digit == 0) {
        /* No digit slots: copy template literally. */
        for (i = 0; i < (int32_t)tlen; i++) dst[field_off + i] = tmpl[i];
        if (paren_wrap) { dst[0] = ' '; dst[tlen+1] = ' '; } /* no digits -> spaces */
        *out = xb_c(dst, (uint16_t)out_len); *err = XBEE_OK; return 0;
    }

    if (overflow) {
        /* Overflow: digit slots -> '*', literals pass through.
         * [verified: mint-002 "999-" + -570 -> "***-"]                          */
        if (paren_wrap) { dst[0] = ' '; }
        for (i = 0; i < (int32_t)tlen; i++) {
            char tc = tmpl[i];
            dst[field_off + i] = (tr_is_digit_slot(tc) || tr_is_dollar_slot(tc)) ? '*' : tc;
        }
        if (paren_wrap) { dst[tlen+1] = ' '; } /* no paren on overflow */
        /* @X/@C suffix: suppress on overflow. */
    } else {
        /* Step 3a: lay out literals and decimal point; digit slots -> ' '. */
        for (i = 0; i < (int32_t)tlen; i++) {
            char tc = tmpl[i];
            if (tc == '.')                          dst[field_off + i] = '.';
            else if (tr_is_digit_slot(tc)
                  || tr_is_dollar_slot(tc))         dst[field_off + i] = ' ';
            else                                    dst[field_off + i] = tc;
        }

        /* Step 3b: extract digit chars from fmtbuf; map right-to-left. */
        {
            char     digits[256];
            int      nd = 0;
            int32_t  fj;
            int32_t  dpos;
            int32_t  tpos;
            int32_t  first_filled = -1;
            int      sign_placed  = 0;

            for (fj = 0; fj < fmt_w; fj++) {
                char fc = fmtbuf[fj];
                if (fc >= '0' && fc <= '9') digits[nd++] = fc;
            }

            dpos = nd - 1;
            for (tpos = (int32_t)tlen - 1; tpos >= 0 && dpos >= 0; tpos--) {
                char tc = tmpl[tpos];
                if (!tr_is_digit_slot(tc) && !tr_is_dollar_slot(tc)) continue;
                dst[field_off + tpos] = digits[dpos--];
                first_filled = tpos;
            }

            /* Step 4: sign handling. */

            /* @( paren wrap for negative: '(' at dst[0], ')' at dst[tlen+1].
             * No minus sign in digit slots. No sign needed for positive.
             * [verified: mint-002 "@( 9999" + -570 -> "( 570)"]               */
            if (paren_wrap) {
#ifndef XB_MUTATE_TRANSFORM_PAREN
                dst[0]        = '(';
                dst[tlen + 1] = ')';
                sign_placed   = 1;
#else
                /* MUTATION (Rule 6): @( does NOT parenthesize negatives.
                 * Leave paren_wrap layout but use space at [0] and [tlen+1],
                 * then fall through to plain sign in the digit slots.
                 * The assertion TRANSFORM(-570,"@( 9999")="( 570)" goes RED. */
                dst[0]        = ' ';
                dst[tlen + 1] = ' ';
                /* sign_placed stays 0 -> plain sign fires below */
#endif
            } else if ((at_flags & TR_AT_PAREN) && !is_neg) {
                /* @( positive: no parens needed, and positive has no sign to place. */
                sign_placed = 1;
            }
            /* @( with is_neg and paren_wrap==0: sign_placed stays 0 -> plain sign.
             * This path is taken under XB_MUTATE_TRANSFORM_PAREN (suffix_extra=0
             * so paren_wrap=0) to produce "-570" instead of "( 570)".             */

            /* @X (TR_AT_DEBIT) for negatives: sign conveyed by " DB" suffix --
             * NO leading minus in the digit slots.
             * [verified: mint-002 "@X 9999" + -570 -> " 570 DB" (no minus)]   */
            if (!sign_placed && is_neg && (at_flags & TR_AT_DEBIT))
                sign_placed = 1;

            /* Plain sign: '-' in the first fill slot left of first_filled.
             * [verified: mint-002 "9999" + -570 -> "-570"]                     */
            if (!sign_placed && is_neg) {
                int32_t sp;
                int placed = 0;
                if (first_filled > 0) {
                    for (sp = first_filled - 1; sp >= 0; sp--) {
                        char tc = tmpl[sp];
                        if ((tr_is_digit_slot(tc) || tr_is_dollar_slot(tc))
                            && dst[field_off + sp] == ' ') {
                            dst[field_off + sp] = '-'; placed = 1; break;
                        }
                    }
                }
                if (!placed) {
                    /* No room for sign -> overflow digit slots. */
                    for (i = 0; i < (int32_t)tlen; i++) {
                        char tc = tmpl[i];
                        if (tr_is_digit_slot(tc) || tr_is_dollar_slot(tc))
                            dst[field_off + i] = '*';
                    }
                }
            }

            /* Step 5: dollar fill.
             * Fill remaining ' ' in $-slots with '$'; also fill comma group
             * separators that are entirely within the fill zone (i.e. to the
             * left of any actual digit or sign char that was placed).
             *
             * Algorithm: find the leftmost non-' ' position in the digit/$ slots
             * (that's the fill boundary); for all positions strictly left of that,
             * replace $-slots and ',' group separators with '$'.
             *
             * [verified: mint-002 "$$,$$9.99" + -12.5 -> "$$$-12.50"
             *   (comma at template[2] -> '$' because no digit to its left)]      */
            if (dollar_fill) {
                int32_t fill_boundary = (int32_t)tlen; /* leftmost non-fill pos */
                int32_t di;
                for (di = 0; di < (int32_t)tlen; di++) {
                    char tc = tmpl[di];
                    if ((tr_is_digit_slot(tc) || tr_is_dollar_slot(tc))
                        && dst[field_off + di] != ' ') {
                        fill_boundary = di;
                        break;
                    }
                }
                for (i = 0; i < fill_boundary; i++) {
                    char tc = tmpl[i];
                    if (tr_is_dollar_slot(tc) && dst[field_off + i] == ' ')
                        dst[field_off + i] = '$';      /* $-slot in fill zone */
                    else if (tc == ',')
                        dst[field_off + i] = '$';      /* comma group sep in fill zone */
                }
            }

            /* Step 6: @B left-justify within the field.
             * [verified: mint-002 "@B 9999" + -570 -> "-570    "]              */
            if (at_flags & TR_AT_LEFT) {
                int32_t first_ns = -1, oi;
                for (oi = 0; oi < (int32_t)tlen; oi++) {
                    if (dst[field_off + oi] != ' ') { first_ns = oi; break; }
                }
                if (first_ns > 0) {
                    int32_t ri = first_ns, wi = 0;
                    while (ri < (int32_t)tlen)
                        dst[field_off + wi++] = dst[field_off + ri++];
                    while (wi < (int32_t)tlen) dst[field_off + wi++] = ' ';
                }
            }
        }

        /* Step 7: @X/@C suffix.
         * [verified: mint-002 "@X 9999" + -570 -> " 570 DB";
         *            "@C 9999" + 570  -> " 570 CR"]                            */
        if (suffix_extra == 3) {
            if ((at_flags & TR_AT_DEBIT) && is_neg) {
                dst[tlen] = ' '; dst[tlen+1] = 'D'; dst[tlen+2] = 'B';
            } else if ((at_flags & TR_AT_CREDIT) && !is_neg) {
                dst[tlen] = ' '; dst[tlen+1] = 'C'; dst[tlen+2] = 'R';
            } else {
                dst[tlen] = ' '; dst[tlen+1] = ' '; dst[tlen+2] = ' ';
            }
        }
    }

    *out = xb_c(dst, (uint16_t)out_len);
    *err = XBEE_OK;
    return 0;
}

/*
 * tr_format_char -- apply a character-source picture to a C value.
 *
 * Grounded template chars (string-functions.md lines 585-586):
 *   '!' -> uppercase the source char at this position.
 *          [documented: string-functions.md:586 "Template '!' (uppercase)"]
 *   'X', 'A', 'N' -> passthrough (any char).
 *          [documented: string-functions.md:586 "A/N/X (any char)"]
 *   other -> literal: the template char is placed in the output.
 *
 * Result length = tlen. Source shorter than tlen: space-fill remaining positions
 * (conservative; exact III+ short-source behavior is [oracle-resolves], GATED).
 * Source longer than tlen: truncated.
 *
 * GATED: '@!' (force-uppercase on whole char result) -- not minted vs III+.
 * 'L', 'Y', '9', '#' for char source: [oracle-resolves].
 */
static int tr_format_char(xb_ctx *ctx,
                          const char *src, uint16_t slen,
                          const char *tmpl, uint16_t tlen,
                          xb_val *out, int *err)
{
    char    *dst;
    int32_t  i;

    if (tlen == 0) { *out = xb_c("", 0); *err = XBEE_OK; return 0; }
    if (tlen > XB_STR_MAX) return fn_fail(err, XBEE_SPACE_LARGE);
    dst = fn_scratch(ctx, (uint32_t)tlen);
    if (dst == 0) return fn_fail(err, XBEE_SCRATCH_FULL);

    for (i = 0; i < (int32_t)tlen; i++) {
        char tc = tmpl[i];
        char sc = (i < (int32_t)slen) ? src[i] : ' ';
        if      (tc == '!')                           dst[i] = fold_upper(sc);
        else if (tc == 'X' || tc == 'A' || tc == 'N') dst[i] = sc;
        else                                          dst[i] = tc;
    }
    *out = xb_c(dst, tlen);
    *err = XBEE_OK;
    return 0;
}

static int fn_transform(xb_ctx *ctx, const xb_val *args,
                        xb_val *out, int *err)
{
    /*
     * TRANSFORM(<expN>/<expC1>, <expC2>) -> C.
     *
     * Ref: HELP.DBS.strings.txt:1316-1319; mint-results-002.md C2.TXT;
     *      string-functions.md lines 562-600.
     *
     * Parse @ flags from the picture; dispatch to tr_format_numeric or
     * tr_format_char.
     */
    const char *pic;
    uint16_t    plen;
    uint16_t    tmpl_start;
    int         at_flags;
    const char *tmpl;
    uint16_t    tlen;

    pic  = args[1].u.c.p;
    plen = args[1].u.c.len;

    at_flags = tr_parse_at_flags(pic, plen, &tmpl_start);
    tmpl     = pic + tmpl_start;
    tlen     = (plen >= tmpl_start) ? (uint16_t)(plen - tmpl_start) : 0;

    if (is_char(&args[0])) {
        /* Character-source: use template char semantics (!,X,A,N).
         * GATED: @( @X @C @B behavior for char source is [oracle-resolves]. */
        return tr_format_char(ctx,
                              args[0].u.c.p, args[0].u.c.len,
                              tmpl, tlen, out, err);
    }

    /* Numeric source: pre-compute suffix_extra, then format. */
    {
        int     is_neg       = (args[0].u.n < 0.0);
        int32_t suffix_extra = 0;

        if (at_flags & TR_AT_DEBIT)  { if (is_neg)  suffix_extra = 3; } /* " DB" */
        if (at_flags & TR_AT_CREDIT) { if (!is_neg)  suffix_extra = 3; } /* " CR" */
        /* @( paren wrap: 2 extra bytes: '(' at dst[0] (prefix), ')' at dst[tlen+1].
         * tr_format_numeric detects suffix_extra==2 as paren_wrap layout.
         * [verified: mint-002 "@( 9999" + -570 -> "( 570)" = tlen(4) + 2 = 6 chars] */
#ifndef XB_MUTATE_TRANSFORM_PAREN
        if (at_flags & TR_AT_PAREN)  { if (is_neg)  suffix_extra = 2; } /* "(" prefix + ")" suffix */
#endif
        return tr_format_numeric(ctx, args[0].u.n,
                                 tmpl, tlen,
                                 at_flags, suffix_extra,
                                 out, err);
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
static int fn_recno(xb_ctx *ctx, xb_val *out, int *err)
{
    uint32_t r;

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
static int fn_reccount(xb_ctx *ctx, xb_val *out, int *err)
{
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
static int fn_eofbof(xb_ctx *ctx, int is_eof, xb_val *out, int *err)
{
    int flag;

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
static int fn_found(xb_ctx *ctx, xb_val *out, int *err)
{
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
static int fn_deleted(xb_ctx *ctx, xb_val *out, int *err)
{
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
static int fn_field(xb_ctx *ctx, const xb_val *args,
                    xb_val *out, int *err)
{
    int32_t  idx;
    char     namebuf[FN_FIELD_NAME_CAP];
    int      nl;
    char    *dst;
    int      i;

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
static int fn_dbf(xb_ctx *ctx, xb_val *out, int *err)
{
    char  namebuf[WA_DBF_NAME_CAP];
    int   nl;
    char *dst;
    int   i;

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
static int fn_file(xb_ctx *ctx, const xb_val *args,
                   xb_val *out, int *err)
{
    uint16_t len, i;
    char    *nm;
    int      exists;

    /* FILE is an environment query, not a cursor operation. Using III+
     * U6-33; ADR-0008 DEC-02 (initech-52ph/u5mw). */
#ifdef SAMIR_CORE_MUTATE_FILE_NEEDS_USE
    if (!need_dbcur(ctx, err)) return *err;
#endif
    len = args[0].u.c.len;
    nm  = fn_scratch(ctx, (uint32_t)len + 1u);
    if (nm == 0) return fn_fail(err, XBEE_SCRATCH_FULL);
    for (i = 0; i < len; i++) nm[i] = args[0].u.c.p[i];
    nm[len] = '\0';

    if (ctx->dbcur && ctx->dbcur->file_exists) {
        exists = ctx->dbcur->file_exists(ctx->dbcur_user, nm);
    } else if (ctx->file_pal) {
        pal_fd fd = ctx->file_pal->open(ctx->file_pal, nm, PAL_RD);
        exists = fd >= 0;
        if (exists) (void)ctx->file_pal->close(ctx->file_pal, fd);
    } else {
        return fn_fail(err, XBEE_INVALID_ARG); /* missing environment binding */
    }
    *out = xb_l(exists ? 1 : 0);
    *err = XBEE_OK;
    return 0;
}

/* ======================================================================== */
/* The dispatch table (case-insensitive name -> handler)                     */
/* ======================================================================== */

/*
 * The built-in names, one NUL-terminated entry each, in FN_* order; an empty
 * entry ends the table. One lookup loop over the table replaces a chain of
 * per-name compare calls (SAMIR size round 3, initech-vn4i): the lookup is the
 * same exact case-insensitive name_is match, and since every name is distinct
 * the first-match order of the old chain cannot change which handler runs.
 */
enum {
    FN_RECSIZE, FN_ERROR, FN_MESSAGE,
    FN_UPPER, FN_LOWER, FN_TRIM, FN_RTRIM, FN_LTRIM, FN_SUBSTR, FN_LEN,
    FN_SPACE, FN_CHR, FN_ASC,
    FN_STR, FN_VAL, FN_CTOD, FN_DTOC,
    FN_DATE, FN_DAY, FN_MONTH, FN_YEAR,
    FN_TYPE,
    FN_ABS, FN_INT, FN_MOD, FN_ROUND, FN_MAX, FN_MIN,
    FN_CDOW, FN_CMONTH, FN_DOW,
    FN_RECNO, FN_RECCOUNT, FN_EOF, FN_BOF, FN_FOUND, FN_DELETED, FN_FIELD,
    FN_DBF, FN_FILE,
    FN_LEFT, FN_RIGHT, FN_STUFF, FN_REPLICATE, FN_AT, FN_ISALPHA, FN_ISUPPER,
    FN_ISLOWER, FN_TRANSFORM,
    FN_EXP, FN_LOG, FN_SQRT, FN_TIME, FN_LUPDATE, FN_NDX, FN_ROW, FN_COL,
    FN_PROW, FN_PCOL, FN_INKEY, FN_READKEY, FN_LASTKEY, FN_ISCOLOR, FN_FKLABEL,
    FN_OS, FN_VERSION, FN_GETENV, FN_DISKSPACE
};
static const char fn_names[] =
    "RECSIZE\0ERROR\0MESSAGE\0"
    "UPPER\0LOWER\0TRIM\0RTRIM\0LTRIM\0SUBSTR\0LEN\0"
    "SPACE\0CHR\0ASC\0"
    "STR\0VAL\0CTOD\0DTOC\0"
    "DATE\0DAY\0MONTH\0YEAR\0"
    "TYPE\0"
    "ABS\0INT\0MOD\0ROUND\0MAX\0MIN\0"
    "CDOW\0CMONTH\0DOW\0"
    "RECNO\0RECCOUNT\0EOF\0BOF\0FOUND\0DELETED\0FIELD\0"
    "DBF\0FILE\0"
    "LEFT\0RIGHT\0STUFF\0REPLICATE\0AT\0ISALPHA\0ISUPPER\0"
    "ISLOWER\0TRANSFORM\0"
    "EXP\0LOG\0SQRT\0TIME\0LUPDATE\0NDX\0ROW\0COL\0"
    "PROW\0PCOL\0INKEY\0READKEY\0LASTKEY\0ISCOLOR\0FKLABEL\0"
    "OS\0VERSION\0GETENV\0DISKSPACE\0";

/* Same arity/type preconditions formerly repeated in each private handler.
 * Ref: the III+ function specifications cited at each handler above. Check
 * counts before reading args, and types before any allocation or operation.
 * C accepts C/M, c only C, X accepts N/C/M; MAX/MIN keep their paired check.
 * RECSIZE/ERROR/MESSAGE retain their special dispatch (and allocation order). */
static const struct { uint8_t min, max; char types[4]; } fn_signature[] = {
    [FN_UPPER] = {1, 1, "C"},
    [FN_LOWER] = {1, 1, "C"},
    [FN_TRIM] = {1, 1, "C"},
    [FN_RTRIM] = {1, 1, "C"},
    [FN_LTRIM] = {1, 1, "C"},
    [FN_SUBSTR] = {2, 3, "CNN"},
    [FN_LEN] = {1, 1, "C"},
    [FN_SPACE] = {1, 1, "N"},
    [FN_CHR] = {1, 1, "N"},
    [FN_ASC] = {1, 1, "C"},
    [FN_STR] = {1, 3, "NNN"},
    [FN_VAL] = {1, 1, "C"},
    [FN_CTOD] = {1, 1, "C"},
    [FN_DTOC] = {1, 1, "D"},
    [FN_DATE] = {0, 0, ""},
    [FN_DAY] = {1, 1, "D"},
    [FN_MONTH] = {1, 1, "D"},
    [FN_YEAR] = {1, 1, "D"},
    [FN_TYPE] = {1, 1, "c"},
    [FN_ABS] = {1, 1, "N"},
    [FN_INT] = {1, 1, "N"},
    [FN_MOD] = {2, 2, "NN"},
    [FN_ROUND] = {2, 2, "NN"},
    [FN_MAX] = {2, 2, ""},
    [FN_MIN] = {2, 2, ""},
    [FN_CDOW] = {1, 1, "D"},
    [FN_CMONTH] = {1, 1, "D"},
    [FN_DOW] = {1, 1, "D"},
    [FN_RECNO] = {0, 0, ""},
    [FN_RECCOUNT] = {0, 0, ""},
    [FN_EOF] = {0, 0, ""},
    [FN_BOF] = {0, 0, ""},
    [FN_FOUND] = {0, 0, ""},
    [FN_DELETED] = {0, 0, ""},
    [FN_FIELD] = {1, 1, "N"},
    [FN_DBF] = {0, 0, ""},
    [FN_FILE] = {1, 1, "C"},
    [FN_LEFT] = {2, 2, "CN"},
    [FN_RIGHT] = {2, 2, "CN"},
    [FN_STUFF] = {4, 4, "CNNC"},
    [FN_REPLICATE] = {2, 2, "CN"},
    [FN_AT] = {2, 2, "CC"},
    [FN_ISALPHA] = {1, 1, "C"},
    [FN_ISUPPER] = {1, 1, "C"},
    [FN_ISLOWER] = {1, 1, "C"},
    [FN_TRANSFORM] = {2, 2, "Xc"},
    [FN_EXP] = {1, 1, "N"},
    [FN_LOG] = {1, 1, "N"},
    [FN_SQRT] = {1, 1, "N"},
    [FN_TIME] = {0, 0, ""},
    [FN_LUPDATE] = {0, 0, ""},
    [FN_NDX] = {1, 1, "N"},
    [FN_ROW] = {0, 0, ""},
    [FN_COL] = {0, 0, ""},
    [FN_PROW] = {0, 0, ""},
    [FN_PCOL] = {0, 0, ""},
    [FN_INKEY] = {0, 0, ""},
    [FN_READKEY] = {0, 0, ""},
    [FN_LASTKEY] = {0, 0, ""},
    [FN_ISCOLOR] = {0, 0, ""},
    [FN_FKLABEL] = {1, 1, "N"},
    [FN_OS] = {0, 0, ""},
    /* SF-version: real III+ accepts one numeric arg (internal build string),
     * despite the HELP-derived spec describing only the no-argument form. */
    [FN_VERSION] = {0, 1, "N"},
    [FN_GETENV] = {1, 1, "C"},
    [FN_DISKSPACE] = {0, 0, ""},
};

/* ---- fan-out wave 2026-10-24: one function per lane; replace the stub
body only ---- */
/* Ref: ../dbase3-decomp/specs/functions/{numeric-and-date-functions,
 * system-and-database-functions}.md and the per-name real fns-mint captures.
 * The signatures above record III+ arity/types; these reserved handlers
 * bypass shared validation so EVERY call still returns #31, including the
 * inventory's zero-argument probes. Each implementing lane must validate
 * its arguments in its own body before using them. INKEY is zero-argument
 * in III+; LASTKEY is absent and must remain a rejection (SF-lastkey).
 * No clock, keyboard, printer or other environment behavior is fabricated. */
static int fn_exp_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    double a, result;
    if (nargs != 1 || args[0].t != XB_N)
        return fn_fail(err, XBEE_INVALID_ARG); /* III+ EXP mint: error 11 */

    a = args[0].u.n;
    /* Ref: ../dbase3-decomp/specs/functions/numeric-and-date-functions.md
     * EXP; HELP.DBS @NUMERIC FUNCTIONS line 1293 and @SET DECIMALS line 1416.
     * Compute e^a as 2^(a*log2(e)), following os/i123/core/eval.c x87_exp2.
     * Memory operands keep SAMIR's soft-float ABI; the x87 stack is balanced. */
    __asm__ volatile (
        "fldl2e\n\t"
        "fmull %1\n\t"
        "fld %%st(0)\n\t"
        "frndint\n\t"
        "fxch %%st(1)\n\t"
        "fsub %%st(1), %%st(0)\n\t"
        "f2xm1\n\t"
        "fld1\n\t"
        "faddp\n\t"
        "fscale\n\t"
        "fstp %%st(1)\n\t"
        "fstpl %0"
        : "=m" (result)
        : "m" (a));
#if defined(SAMIR_FNS_MUTANT_EXP) || defined(SAMIR_FNS_MUTANT_exp)
    result += 1.0; /* Rule 6: the EXP transcript must catch this perturbation. */
#endif
    *out = xb_n(result);
    out->num_dec = ctx->set_decimals;
    out->num_width = (uint16_t)(10 + (ctx->set_decimals ? ctx->set_decimals + 1 : 0));
    *err = XBEE_OK;
    return 0;
}

static int fn_log_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    (void)ctx; (void)args; (void)nargs; (void)out;
    return fn_fail(err, XBEE_INVALID_FN);
}

static int fn_sqrt_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    (void)ctx; (void)args; (void)nargs; (void)out;
    return fn_fail(err, XBEE_INVALID_FN);
}

static int fn_time_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    /* Ref: ../dbase3-decomp/archive/golden-mined/HELP.DBS.strings.txt:1284,
     * TIME() returns "hh:mm:ss"; fns-mint/time/GOLD.OUT pins C/8/colons. */
    uint8_t hour = 0, minute = 0, second = 0;
    char *dst;
    (void)args;
    if (nargs) return fn_fail(err, XBEE_INVALID_ARG);
    if (!ctx->file_pal || !ctx->file_pal->time)
        return fn_fail(err, XBEE_INVALID_ARG);
    ctx->file_pal->time(ctx->file_pal, &hour, &minute, &second);
    dst = fn_scratch(ctx, 8);
    if (!dst) return fn_fail(err, XBEE_SCRATCH_FULL);
    dst[0] = (char)('0' + hour / 10u); dst[1] = (char)('0' + hour % 10u);
    dst[2] = ':';
#if defined(SAMIR_FNS_MUTANT_TIME) || defined(SAMIR_FNS_MUTANT_time)
    dst[2] = ';'; /* The named gate passes the lower-case case name. */
#endif
    dst[3] = (char)('0' + minute / 10u); dst[4] = (char)('0' + minute % 10u);
    dst[5] = ':';
    dst[6] = (char)('0' + second / 10u); dst[7] = (char)('0' + second % 10u);
    *out = xb_c(dst, 8);
    *err = XBEE_OK;
    return 0;
}

static int fn_lupdate_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    double date;
    (void)args;
    if (nargs != 0) return fn_fail(err, XBEE_INVALID_ARG);
    /* Rule 2: no work-area cursor is #52, as for RECNO() and its siblings;
     * a bound cursor with no file in USE answers the blank date (GOLD.OUT). */
    if (!need_dbcur(ctx, err)) return *err;
    date = ctx->dbcur->last_update
        ? ctx->dbcur->last_update(ctx->dbcur_user) : 0.0;
#ifdef SAMIR_FNS_MUTANT_lupdate
    /* Rule 6: perturb the returned date by one day; the independent mint bites. */
    if (date > 0.0) date += 1.0;
#endif
    *out = xb_d(date);
    *err = XBEE_OK;
    return 0;
}

static int fn_ndx_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    (void)ctx; (void)args; (void)nargs; (void)out;
    return fn_fail(err, XBEE_INVALID_FN);
}

static int fn_row_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    (void)ctx; (void)args; (void)nargs; (void)out;
    return fn_fail(err, XBEE_INVALID_FN);
}

static int fn_col_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    (void)ctx; (void)args; (void)nargs; (void)out;
    return fn_fail(err, XBEE_INVALID_FN);
}

static int fn_prow_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    (void)ctx; (void)args; (void)nargs; (void)out;
    return fn_fail(err, XBEE_INVALID_FN);
}

static int fn_pcol_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    (void)ctx; (void)args; (void)nargs; (void)out;
    return fn_fail(err, XBEE_INVALID_FN);
}

static int fn_inkey_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    int32_t key = -1;
    (void)args;
    /* Ref: real III+ fns-mint/inkey/GOLD.OUT: INKEY(1), INKEY("X")
     * and INKEY(1,2) all raise #11. Using III+ U6-40/41 Table 6-1
     * (docs/audits/2026-10-15-look-pass6/evidence/dbase-manual.txt):
     * nonblocking FIFO poll, IBM characters and control-key equivalents. */
    if (nargs != 0) return fn_fail(err, XBEE_INVALID_ARG);
    if (ctx && ctx->file_pal && ctx->file_pal->conin_poll)
        key = ctx->file_pal->conin_poll(ctx->file_pal);
    if (key < 0) key = 0;
    else if (key >= 256) {
        switch (key & 255) {
        case 0x4D: key = 4; break;   /* Right / Ctrl-D */
        case 0x4B: key = 19; break;  /* Left / Ctrl-S */
        case 0x48: key = 5; break;   /* Up / Ctrl-E */
        case 0x50: key = 24; break;  /* Down / Ctrl-X */
        case 0x74: key = 2; break;   /* Ctrl-Right / Ctrl-B */
        case 0x73: key = 26; break;  /* Ctrl-Left / Ctrl-Z */
        case 0x52: key = 22; break;  /* Ins / Ctrl-V */
        case 0x53: key = 7; break;   /* Del / Ctrl-G */
        case 0x47: key = 1; break;   /* Home / Ctrl-A */
        case 0x4F: key = 6; break;   /* End / Ctrl-F */
        case 0x49: key = 18; break;  /* PgUp / Ctrl-R */
        case 0x51: key = 3; break;   /* PgDn / Ctrl-C */
        case 0x77: key = 29; break;  /* Ctrl-Home / Ctrl-] */
        case 0x75: key = 23; break;  /* Ctrl-End / Ctrl-W */
        case 0x84: key = 31; break;  /* Ctrl-PgUp / Ctrl-_ */
        case 0x76: key = 30; break;  /* Ctrl-PgDn / Ctrl-^ */
        default: key = 0; break;    /* No Table 6-1 equivalent. */
        }
    } else if (key == 17) key = 27; /* Ctrl-Q / Esc */
#ifdef SAMIR_FNS_MUTANT_inkey
    key++;                         /* Rule 6: empty queue must return zero. */
#endif
    *out = xb_n((double)key);
    out->num_width = 3;             /* Real III+ "no-key   0 N". */
    *err = XBEE_OK;
    return 0;
}

static int fn_readkey_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    /* Ref: system-and-database-functions.md READKEY; HELP.DBS 1281-1282;
     * fns-mint/readkey/GOLD.OUT: before any full-screen command, repeated
     * calls return zero in three numeric columns. READKEY is exit history,
     * not a keyboard poll: do not consume type-ahead or wait for a key.
     * SAMIR currently has no full-screen READ/EDIT/BROWSE command producer. */
    (void)ctx; (void)args;
    if (nargs != 0) return fn_fail(err, XBEE_INVALID_ARG);
    *out = xb_n(0.0);
#ifdef SAMIR_FNS_MUTANT_readkey
    out->u.n = 1.0;
#endif
    out->num_width = 3;
    *err = XBEE_OK;
    return 0;
}

static int fn_lastkey_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    (void)ctx; (void)args; (void)nargs; (void)out;
    return fn_fail(err, XBEE_INVALID_FN);
}

static int fn_iscolor(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    /* Ref: system-and-database-functions.md ISCOLOR; HELP.DBS line 1270;
     * fns-mint/iscolor/GOLD.OUT: logical, unchanged by SET COLOR, #11 on args. */
    samir_pal_t *pal = ctx ? ctx->file_pal : (samir_pal_t *)0;
    int color;
    (void)args;
    if (nargs != 0) return fn_fail(err, XBEE_INVALID_ARG);
    if (!pal || !pal->iscolor) return fn_fail(err, XBEE_INVALID_ARG);
    color = pal->iscolor(pal);
#if defined(SAMIR_FNS_MUTANT_iscolor) || defined(SAMIR_FNS_MUTANT_ISCOLOR)
    color = !color; /* Rule 6: invert only the capability result. */
#endif
    *out = xb_l(color);
    *err = XBEE_OK;
    return 0;
}

static int fn_fklabel_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    /* Ref: system-and-database-functions.md FKLABEL; HELP.DBS line 1261.
     * Real III+ fns-mint/fklabel/GOLD.OUT pins 1..9 to F2..F10, 0 and 10
     * to empty strings, and both bad arity and bad type to #11 (not #9).
     * These key names are fixed; no keyboard state or PAL query is needed. */
    static const char labels[10][4] = {
        "", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10"
    };
    int key = 0;
    (void)ctx;
    if (nargs != 1 || args[0].t != XB_N)
        return fn_fail(err, XBEE_INVALID_ARG);
    /* Bound before converting so large numeric arguments cannot overflow. */
    if (args[0].u.n >= 1.0 && args[0].u.n < 10.0)
        key = to_int_trunc(args[0].u.n);
#if defined(SAMIR_FNS_MUTANT_FKLABEL) || defined(SAMIR_FNS_MUTANT_fklabel)
    /* Rule 6: the generic gates supply the lowercase case-name spelling.
     * Perturb only FKLABEL(1), returning F3 in place of the real F2. */
    if (key == 1) key = 2;
#endif
    *out = xb_c(labels[key], (uint16_t)(key == 0 ? 0 : key == 9 ? 3 : 2));
    *err = XBEE_OK;
    return 0;
}

static int fn_os_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    /* Ref: system-and-database-functions.md OS(), HELP.DBS line 1273;
     * LANE-BRIEF operator ruling / ADR-0003 DEC-12: InitechDOS branding.
     * The minted zero-argument signature rejects extra arguments (#11). */
    uint16_t version;
    char *text;
    (void)args;
    if (nargs != 0) return fn_fail(err, XBEE_INVALID_ARG);
    if (!ctx->file_pal || !ctx->file_pal->os_version)
        return fn_fail(err, XBEE_INVALID_ARG);
    version = ctx->file_pal->os_version(ctx->file_pal);
    text = fn_scratch(ctx, 15u);
    if (!text) return fn_fail(err, XBEE_SCRATCH_FULL);
    rt_memcpy(text, "InitechDOS 0.00", 15u);
    text[11] = (char)('0' + (version & 0xffu));
    text[13] = (char)('0' + (version >> 8) / 10u);
    text[14] = (char)('0' + (version >> 8) % 10u);
    /* The existing gate template spells its case macro in lowercase. */
#if defined(SAMIR_FNS_MUTANT_OS) || defined(SAMIR_FNS_MUTANT_os)
    text[14] = '1'; /* Rule 6: wrong version, 3.31 instead of 3.30. */
#endif
    *out = xb_c(text, 15u);
    *err = XBEE_OK;
    return 0;
}

static int fn_version(xb_ctx *ctx, const xb_val *args, int nargs,
                      xb_val *out, int *err)
{
    /* Ref: system-and-database-functions.md VERSION; HELP.DBS 1287-1288.
     * fns-mint/version accepts an optional number despite the prose spec.
     * Operator ruling initech-ugpsk: the internal build string is read as
     * the same fixed Initech identity for both forms (Rule 11). */
    static const char version[] = "InitechBase version 3.30";
    (void)ctx;
    if (nargs < 0 || nargs > 1 || (nargs == 1 && args[0].t != XB_N))
        return fn_fail(err, XBEE_INVALID_ARG);
#if defined(SAMIR_FNS_MUTANT_VERSION) || defined(SAMIR_FNS_MUTANT_version)
    *out = xb_c(version, (uint16_t)(sizeof version - 2u));
#else
    *out = xb_c(version, (uint16_t)(sizeof version - 1u));
#endif
    return 0;
}

static int fn_getenv_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    char *name, *value;
    int32_t n;
    uint16_t i, len;
    /* Ref: dbase3-decomp system-and-database-functions.md GETENV and
     * HELP.DBS.strings.txt GETENV(); the PAL reads the DOS environment. */
    if (nargs != 1) return fn_fail(err, XBEE_INVALID_ARG);
    /* III+'s GETENV type failure is #11 in the independent function golden. */
    if (!is_char(&args[0])) return fn_fail(err, XBEE_INVALID_ARG);
    len = args[0].u.c.len;
    name = fn_scratch(ctx, (uint32_t)len + 1u);
    value = fn_scratch(ctx, XB_STR_MAX + 1u);
    if (!name || !value) return fn_fail(err, XBEE_SCRATCH_FULL);
    for (i = 0; i < len; i++) name[i] = args[0].u.c.p[i];
    name[len] = '\0';
    n = (ctx->file_pal && ctx->file_pal->getenv) ?
        ctx->file_pal->getenv(ctx->file_pal, name, value, XB_STR_MAX + 1u) :
        -(int32_t)PAL_ENOENT;
    if (n < 0) n = 0;
#ifdef SAMIR_FNS_MUTANT_getenv
    if (n > 0) n--;
#endif
    *out = xb_c(value, (uint16_t)n);
    *err = XBEE_OK;
    return 0;
}

static int fn_diskspace_stub(xb_ctx *ctx, const xb_val *args, int nargs,
                        xb_val *out, int *err)
{
    uint32_t bytes;
    (void)args;
    /* Ref: system-and-database-functions.md DISKSPACE; HELP.DBS:1257.
     * III+ takes no drive argument. fns-mint/diskspace pins N, >0 and #11. */
    if (nargs != 0) return fn_fail(err, XBEE_INVALID_ARG);
    if (!ctx || !ctx->file_pal || !ctx->file_pal->diskspace)
        return fn_fail(err, XBEE_UNBOUND);
    if (ctx->file_pal->diskspace(ctx->file_pal, &bytes) != PAL_OK)
        return fn_fail(err, 29); /* dbase_msg_codes.tsv: File is not accessible. */
#if defined(SAMIR_FNS_MUTANT_DISKSPACE) || defined(SAMIR_FNS_MUTANT_diskspace)
    bytes = 0u; /* Rule 6: the independent III+ >0 line must reject this. */
#endif
    *out = xb_n((double)bytes);
    return 0;
}

/* ---- end fan-out wave ---- */

int xb_call_builtin(const char *name, uint16_t namelen,
                    const xb_val *args, int nargs,
                    xb_ctx *ctx, xb_val *out, int *err_code)
{
    samir_ovl_validate_ctx(ctx);
    const char *kw = fn_names;
    int k = 0;

    *err_code = XBEE_OK;
    *out = xb_u();

    if (name == 0 || namelen == 0) return fn_fail(err_code, XBEE_INVALID_FN);

    while (*kw != '\0' && !name_is(name, namelen, kw)) {
        while (*kw++ != '\0') { }
        k++;
    }
    if (*kw == '\0') k = -1;                    /* no such built-in -> #31 */

    if (k >= FN_UPPER && k < FN_EXP) {
        unsigned i;
        if (nargs < fn_signature[k].min || nargs > fn_signature[k].max)
            return fn_fail(err_code, XBEE_INVALID_ARG);
        for (i = 0; i < (unsigned)nargs; i++) {
            char t = fn_signature[k].types[i];
            if ((t == 'C' && !is_char(&args[i])) ||
                (t == 'c' && args[i].t != XB_C) ||
                (t == 'N' && args[i].t != XB_N) ||
                (t == 'D' && args[i].t != XB_D) ||
                (t == 'X' && args[i].t != XB_N && !is_char(&args[i])))
                return fn_fail(err_code, XBEE_MISMATCH);
        }
    }

    switch (k) {
    /* Using III+ U6-69; SF-recsize pins the deletion-byte inclusion and
     * the documented zero when no table is in use. */
#ifndef SAMIR_FNS_MUTANT_recsize
    case FN_RECSIZE:
        if(nargs) return fn_fail(err_code,XBEE_INVALID_ARG);
        *out=xb_n(ctx->record_size?(double)ctx->record_size(ctx->dbcur_user):0.0);
        return 0;
#endif

    /* Programming III+ N5-30 (PDF 418): number of the ON ERROR condition. */
    case FN_ERROR:
        if (nargs != 0) return fn_fail(err_code, XBEE_INVALID_ARG);
        *out = xb_n(ctx ? (double)ctx->error_number : 0.0);
        return 0;
    /* Using III+ U6-54: MESSAGE() is the catalogue text of the error
     * ERROR() reports (SF-errors, real III+ 1.1). */
    case FN_MESSAGE: {
        char *m = fn_scratch(ctx, 128u);
        unsigned n = 0;
        if (nargs != 0) return fn_fail(err_code, XBEE_INVALID_ARG);
        if (m == 0) return fn_fail(err_code, XBEE_SCRATCH_FULL);
        if (ctx->message && ctx->error_number > 0)
            n = ctx->message((unsigned)ctx->error_number, m, 128u);
        *out = xb_c(m, (uint16_t)n);
        return 0;
    }

    /* String functions */
    case FN_UPPER:  return fn_case(ctx, args, 1, out, err_code);
    case FN_LOWER:  return fn_case(ctx, args, 0, out, err_code);
    case FN_TRIM:   return fn_trim(args, 0, out, err_code);
    case FN_RTRIM:  return fn_trim(args, 0, out, err_code); /* synonym */
    case FN_LTRIM:  return fn_trim(args, 1, out, err_code);
    case FN_SUBSTR: return fn_substr(args, nargs, out, err_code);
    case FN_LEN:    return fn_len(args, out, err_code);
    case FN_SPACE:  return fn_space(ctx, args, out, err_code);
    case FN_CHR:    return fn_chr(ctx, args, out, err_code);
    case FN_ASC:    return fn_asc(args, out, err_code);

    /* Conversion bridges */
    case FN_STR:    return fn_str(ctx, args, nargs, out, err_code);
    case FN_VAL:    return fn_val(args, out, err_code);
    case FN_CTOD:   return fn_ctod_impl(args, ctx, out, err_code);
    case FN_DTOC:   return fn_dtoc_impl(ctx, args, out, err_code);

    /* Date functions */
    case FN_DATE:   return fn_date(ctx, out, err_code);
    case FN_DAY:    return fn_dmy(args, 0, out, err_code);
    case FN_MONTH:  return fn_dmy(args, 1, out, err_code);
    case FN_YEAR:   return fn_dmy(args, 2, out, err_code);

    /* Generic */
    case FN_TYPE:   return fn_type(args, ctx, out, err_code);

    /* Numeric functions (S3.6a freestanding half) */
    case FN_ABS:    return fn_abs(args, out, err_code);
    case FN_INT:    return fn_int(args, out, err_code);
    case FN_MOD:    return fn_mod(args, out, err_code);
    case FN_ROUND:  return fn_round(args, out, err_code);
    case FN_MAX:    return fn_maxmin(args, 1, out, err_code);
    case FN_MIN:    return fn_maxmin(args, 0, out, err_code);

    /* Date name functions (S3.6a freestanding half) */
    case FN_CDOW:   return fn_cdow(ctx, args, out, err_code);
    case FN_CMONTH: return fn_cmonth(ctx, args, out, err_code);
    case FN_DOW:    return fn_dow(args, out, err_code);

    /* Database / work-area functions (S3.6b, initech-7az.10). These reach the
     * SELECTED work area's cursor through ctx->dbcur (eval.h xb_dbcursor) -- NOT
     * any workarea.c symbol -- so this TU stays link-clean against the engine
     * core. ctx->dbcur == NULL (no work area) -> XBEE_NO_DATABASE (#52) fail loud.
     * Refs: system-and-database-functions.md sec 2 (RECNO/RECCOUNT/EOF/BOF/FOUND/
     *   DELETED/FIELD/DBF) + sec 3 (FILE); dbase_msg_codes.tsv #52. */
    case FN_RECNO:    return fn_recno(ctx, out, err_code);
    case FN_RECCOUNT: return fn_reccount(ctx, out, err_code);
    case FN_EOF:      return fn_eofbof(ctx, 1, out, err_code);
    case FN_BOF:      return fn_eofbof(ctx, 0, out, err_code);
    case FN_FOUND:    return fn_found(ctx, out, err_code);
    case FN_DELETED:  return fn_deleted(ctx, out, err_code);
    case FN_FIELD:    return fn_field(ctx, args, out, err_code);
    case FN_DBF:      return fn_dbf(ctx, out, err_code);
    case FN_FILE:     return fn_file(ctx, args, out, err_code);

    /* String functions C (initech-7az.12): LEFT RIGHT STUFF REPLICATE AT
     * ISALPHA ISUPPER ISLOWER TRANSFORM.
     * Refs: HELP.DBS.strings.txt @STRING FUNCTIONS + @STR FUNC 2/3 (lines
     *   1326-1358); mint-results-002.md (TRANSFORM exists, "9999" picture). */
    case FN_LEFT:      return fn_left(args, out, err_code);
    case FN_RIGHT:     return fn_right(args, out, err_code);
    case FN_STUFF:     return fn_stuff(ctx, args, out, err_code);
    case FN_REPLICATE: return fn_replicate(ctx, args, out, err_code);
    case FN_AT:        return fn_at(args, out, err_code);
    case FN_ISALPHA:   return fn_isalpha(args, out, err_code);
    case FN_ISUPPER:   return fn_isupper(args, out, err_code);
    case FN_ISLOWER:   return fn_islower(args, out, err_code);
    case FN_TRANSFORM: return fn_transform(ctx, args, out, err_code);
    case FN_EXP: return fn_exp_stub(ctx, args, nargs, out, err_code);
    case FN_LOG: return fn_log_stub(ctx, args, nargs, out, err_code);
    case FN_SQRT: return fn_sqrt_stub(ctx, args, nargs, out, err_code);
    case FN_TIME: return fn_time_stub(ctx, args, nargs, out, err_code);
    case FN_LUPDATE: return fn_lupdate_stub(ctx, args, nargs, out, err_code);
    case FN_NDX: return fn_ndx_stub(ctx, args, nargs, out, err_code);
    case FN_ROW: return fn_row_stub(ctx, args, nargs, out, err_code);
    case FN_COL: return fn_col_stub(ctx, args, nargs, out, err_code);
    case FN_PROW: return fn_prow_stub(ctx, args, nargs, out, err_code);
    case FN_PCOL: return fn_pcol_stub(ctx, args, nargs, out, err_code);
    case FN_INKEY: return fn_inkey_stub(ctx, args, nargs, out, err_code);
    case FN_READKEY: return fn_readkey_stub(ctx, args, nargs, out, err_code);
    case FN_LASTKEY: return fn_lastkey_stub(ctx, args, nargs, out, err_code);
    case FN_ISCOLOR: return fn_iscolor(ctx, args, nargs, out, err_code);
    case FN_FKLABEL: return fn_fklabel_stub(ctx, args, nargs, out, err_code);
    case FN_OS: return fn_os_stub(ctx, args, nargs, out, err_code);
    case FN_VERSION: return fn_version(ctx, args, nargs, out, err_code);
    case FN_GETENV: return fn_getenv_stub(ctx, args, nargs, out, err_code);
    case FN_DISKSPACE: return fn_diskspace_stub(ctx, args, nargs, out, err_code);
    default:           break;
    }

    /* DTOS exists in dBASE IV / Clipper but NOT in III+ 1.1 -> #31 (the same
     * path as any unknown name; the test asserts DTOS specifically).
     * (numeric-and-date-functions.md: DTOS absent from @DATE FUNCTIONS.)
     * Under SAMIR_FNS_MUTANT_recsize RECSIZE lands here too, as before. */
    return fn_fail(err_code, XBEE_INVALID_FN);   /* #31 "Invalid function name." */
}
