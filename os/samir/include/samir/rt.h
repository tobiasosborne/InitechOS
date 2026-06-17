/*
 * os/samir/include/samir/rt.h -- SAMIR freestanding runtime declarations.
 *
 * THE ARTIFACT (CLAUDE.md Law 3): this header is compiled freestanding
 * (-ffreestanding -nostdlib) alongside the shipped SAMIR engine. It MUST NOT
 * include any libc header other than <stdint.h> (the same convention as pal.h,
 * os/milton/blockdev.h, etc.).
 *
 * Sections:
 *   1. Memory / string utilities (rt_memcpy, rt_memset, rt_memcmp,
 *                                  rt_strlen, rt_strncmp)
 *   2. Julian Day Number <-> Gregorian calendar (valid range 1900-2155).
 *      Date keys in .ndx files are stored as JDN-as-double (corpus ndx.md
 *      sec 4.2; minted TOURDATE.NDX: JDN(1985-08-05)=2446283 [verified]).
 *   3. Decimal formatter / parser for N-field I/O.
 *      dec_format: produces right-justified ASCII text of `width` chars;
 *        rounding rule = ties toward +infinity (add 0.5 * 10^-dec, then
 *        truncate toward zero). Source: re/mint-results-001.md sec
 *        "Numeric rounding tie-break" -- STR(2.5,2,0)="3",
 *        STR(-2.5,2,0)="-2", STR(0.5,1,0)="1", STR(1.5,2,0)="2",
 *        STR(0.125,4,2)="0.13". [verified: minted BATLOG]
 *        Width overflow: fills the output with '*' per dBASE behavior
 *        (re/mint-results-001.md sec "Numeric overflow" -- OTEST.DBF). [verified]
 *      dec_parse: convert an ASCII N-field back to double; leading/trailing
 *        spaces are ignored, empty/all-spaces -> 0.0.
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S0.3 contract.
 *   - ../dbase3-decomp/re/mint-results-001.md (rounding + overflow evidence).
 *   - ../dbase3-decomp/specs/file-formats/ndx.md sec 4.2 (JDN).
 *   - os/samir/include/samir/pal.h (int-type convention: <stdint.h>).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 * Freestanding-legal (Law 3): only <stdint.h>.
 */
#ifndef INITECH_SAMIR_RT_H
#define INITECH_SAMIR_RT_H

#include <stdint.h>

/* ---- 1. Memory / string utilities ---- */

/*
 * rt_memcpy: copy `n` bytes from `src` to `dst` (regions must not overlap).
 * Returns `dst`. Freestanding replacement for memcpy.
 */
void *rt_memcpy(void *dst, const void *src, uint32_t n);

/*
 * rt_memset: fill `n` bytes at `dst` with byte value `c`.
 * Returns `dst`. Freestanding replacement for memset.
 */
void *rt_memset(void *dst, int c, uint32_t n);

/*
 * rt_memcmp: compare `n` bytes. Returns 0 if equal, <0 or >0 otherwise.
 * Freestanding replacement for memcmp.
 */
int rt_memcmp(const void *a, const void *b, uint32_t n);

/*
 * rt_strlen: return the number of bytes before the first NUL in `s`.
 * Freestanding replacement for strlen.
 */
uint32_t rt_strlen(const char *s);

/*
 * rt_strncmp: compare up to `n` bytes of `a` and `b`. Returns 0 if equal,
 * <0 or >0 otherwise. Freestanding replacement for strncmp.
 */
int rt_strncmp(const char *a, const char *b, uint32_t n);

/* ---- 2. Julian Day Number (Gregorian calendar, 1900-2155) ---- */

/*
 * jdn_from_ymd: compute the Julian Day Number for a Gregorian calendar date.
 *
 * y = full year (e.g. 1985, 2000), m = 1-based month (1-12),
 * d = 1-based day (1-31).
 *
 * Valid range: 1900-01-01 .. 2155-12-31 (the dBASE III+ date range).
 * Behavior outside this range is undefined.
 *
 * Ref: standard astronomical Gregorian-to-JDN formula (Meeus Ch.7).
 * Verified against minted TOURDATE.NDX: JDN(1985-08-05)=2446283 [verified],
 * JDN(1985-09-07)=2446316 [verified], JDN(1985-09-23)=2446332 [verified],
 * and against the .mem minted value JDN(1999-12-31)=2451544 [verified].
 */
int32_t jdn_from_ymd(int32_t y, int32_t m, int32_t d);

/*
 * ymd_from_jdn: recover the Gregorian date from a Julian Day Number.
 *
 * Writes the full year to *y, 1-based month to *m, 1-based day to *d.
 * Valid range: JDN values for 1900-01-01 .. 2155-12-31.
 *
 * Ref: inverse of the Meeus Gregorian JDN formula.
 */
void ymd_from_jdn(int32_t jdn, int32_t *y, int32_t *m, int32_t *d);

/* ---- 3. Decimal formatter / parser ---- */

/*
 * dec_format: format a double `v` as a right-justified decimal ASCII string
 * of exactly `width` characters into `out` (NOT NUL-terminated).
 *
 * `dec` = number of digits after the decimal point (0 = integer, >= 0).
 * `width` = total field width including sign, digits, and decimal point.
 *
 * Rounding rule: ties toward +infinity. Implementation: compute
 *   rounded = (int64_t)(v * 10^dec + 0.5) / 10^dec
 * (C truncation-toward-zero on the cast produces the correct tie direction:
 * +2.5 -> +3.0, -2.5 -> -2.0). Source: re/mint-results-001.md. [verified]
 *
 * Output format:
 *   - If dec == 0: no decimal point.
 *   - If dec > 0: decimal point with exactly `dec` fractional digits.
 *   - Right-justified; left-padded with spaces.
 *   - Negative numbers: leading '-' counted in `width`.
 *   - Width overflow (formatted value does not fit in `width` columns):
 *     fill `out` with '*' (per dBASE OTEST.DBF minted evidence). [verified]
 *
 * Returns the number of bytes written (always `width`).
 * `out` must have room for at least `width` bytes.
 */
int dec_format(double v, int width, int dec, char *out);

/*
 * dec_parse: parse an ASCII decimal string `s` of `len` bytes into a double.
 *
 * Leading and trailing ASCII spaces (0x20) are skipped. An empty or
 * all-spaces field yields 0.0. A leading '-' or '+' is recognized.
 * No exponent notation is handled (dBASE N fields are fixed-format decimal).
 *
 * Returns the parsed value. On malformed input (non-digit, non-space,
 * non-sign, non-decimal-point character) the behaviour is unspecified
 * beyond returning some double -- callers that need strict validation
 * should sanitize first.
 */
double dec_parse(const char *s, int len);

#endif /* INITECH_SAMIR_RT_H */
