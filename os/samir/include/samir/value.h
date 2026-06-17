/*
 * os/samir/include/samir/value.h -- SAMIR typed value model.
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib). Depends ONLY on <stdint.h> (the int-type convention from pal.h).
 * No libc headers.
 *
 * This is step S0.6 of the SAMIR implementation plan. It provides the internal
 * typed value representation used throughout the SAMIR engine (evaluator,
 * codecs, interpreter). The design is taken directly from the sec.8.2 sketch in
 * docs/plans/SAMIR-implementation-plan.md.
 *
 * Types and encoding (Law 1 sources):
 *   XB_C  Character string. Internal rep: pointer + byte length. The pointer
 *         is NOT owned by the xb_val -- the caller (or the PAL arena) owns the
 *         backing bytes. value.c NEVER copies or frees string storage.
 *   XB_N  Numeric. Internal rep: IEEE-754 double. Source: corpus mint-004
 *         (internal rep = IEEE double, not BCD).
 *         Ref: docs/plans/SAMIR-implementation-plan.md sec.2.F + sec.3.3.
 *   XB_D  Date. Internal rep: Julian Day Number stored as double.
 *         Source: corpus mint-004; ndx.md sec 4.2 (verified: JDN(1985-08-05)
 *         = 2446283). Ref: os/samir/include/samir/rt.h jdn_from_ymd.
 *   XB_L  Logical. Internal rep: uint8_t 0 (false) or 1 (true). dBASE stores
 *         'T'/'F'/'Y'/'N' on disk; the codec normalises to 0/1 on read.
 *   XB_M  Memo. Exactly like XB_C on the value level (pointer + len); the
 *         distinction is preserved so TYPE() returns 'M' and xb_typeof() can
 *         dispatch correctly. The pointer is an arena string, NOT the raw
 *         10-byte .dbt block-number field from the .dbf record.
 *   XB_U  Undefined / null. No payload. Returned by xb_u() and used wherever
 *         a variable has not been initialised (dBASE PUBLIC without assignment).
 *
 * Equality contract (xb_eq):
 *   This is RAW per-type equality -- it is NOT the xBase `=` operator.
 *   C/M: byte-equal over len (lengths must match; does NOT implement SET EXACT
 *        begins-with truncation -- that is the evaluator's job at S3.3).
 *   N:   double == (exact IEEE comparison, no epsilon).
 *   D:   JDN double == (same rule as N).
 *   L:   boolean == (both 0 or both non-zero after normalisation to 0/1).
 *   U:   never equal (two undefined values are not equal, matching dBASE .NULL.
 *        semantics).
 *   Cross-type: always 0 (not equal), regardless of values.
 *
 * TYPE() char mapping (xb_type_char):
 *   XB_C -> 'C', XB_N -> 'N', XB_D -> 'D', XB_L -> 'L', XB_M -> 'M',
 *   XB_U -> 'U'. Used by the TYPE() built-in function (S3.5).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 * Freestanding-legal (Law 3): only <stdint.h>.
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md sec.8.2 (the xb_val sketch), S0.6.
 *   - docs/plans/SAMIR-implementation-plan.md sec.2.C (dBASE III PLUS 1.1 only).
 *   - docs/plans/SAMIR-implementation-plan.md sec.2.F + sec.3.3 (numeric = IEEE
 *     double; date = JDN as double; corpus mint-004).
 *   - os/samir/include/samir/rt.h (jdn_from_ymd; rt_memcmp used in value.c).
 */
#ifndef INITECH_SAMIR_VALUE_H
#define INITECH_SAMIR_VALUE_H

#include <stdint.h>

/* ---- Type tag ---- */

/*
 * xb_type: discriminator for the six internal value kinds.
 * Matches the sec.8.2 enum exactly; the integer values are NOT part of the
 * on-disk format (disk types are handled in the codec layer).
 */
typedef enum {
    XB_C = 0,   /* Character  -- char *p + uint16_t len */
    XB_N = 1,   /* Numeric    -- IEEE-754 double         */
    XB_D = 2,   /* Date       -- JDN as double           */
    XB_L = 3,   /* Logical    -- uint8_t 0/1             */
    XB_M = 4,   /* Memo       -- char *p + uint16_t len  */
    XB_U = 5    /* Undefined  -- no payload              */
} xb_type;

/* ---- Value struct ---- */

/*
 * xb_val: a tagged union holding one typed value.
 *
 * The `c` member is shared by XB_C and XB_M (both are pointer+len slices;
 * the type tag distinguishes them). The pointer is NOT owned: the backing
 * bytes live in the PAL arena or in a caller-managed buffer. value.c never
 * copies or frees string/memo storage.
 *
 * Layout matches the sec.8.2 sketch verbatim (field names preserved).
 */
typedef struct {
    xb_type t;
    union {
        struct { char *p; uint16_t len; } c;  /* XB_C / XB_M */
        double  n;                             /* XB_N         */
        double  d;                             /* XB_D (JDN)   */
        uint8_t l;                             /* XB_L (0/1)   */
    } u;
} xb_val;

/* ---- Constructors ---- */

/*
 * xb_c: construct a Character value from a pointer and byte length.
 * The pointer is NOT copied. The caller (or arena) owns the backing bytes
 * for the lifetime of the value. value.c is pure (no allocation).
 */
xb_val xb_c(const char *p, uint16_t len);

/*
 * xb_n: construct a Numeric value from an IEEE-754 double.
 */
xb_val xb_n(double v);

/*
 * xb_d: construct a Date value from a Julian Day Number (as double).
 * Use jdn_from_ymd (rt.h) to obtain the JDN from a calendar date.
 */
xb_val xb_d(double jdn);

/*
 * xb_l: construct a Logical value. Any non-zero `truth` is normalised to 1.
 */
xb_val xb_l(int truth);

/*
 * xb_m: construct a Memo value from a pointer and byte length.
 * Same ownership contract as xb_c: the pointer is NOT copied.
 */
xb_val xb_m(const char *p, uint16_t len);

/*
 * xb_u: construct an Undefined value. No payload.
 */
xb_val xb_u(void);

/* ---- Introspection ---- */

/*
 * xb_typeof: return the type tag of a value.
 */
xb_type xb_typeof(const xb_val *v);

/*
 * xb_type_char: map an xb_type to the dBASE TYPE() one-character code:
 *   XB_C -> 'C'   XB_N -> 'N'   XB_D -> 'D'
 *   XB_L -> 'L'   XB_M -> 'M'   XB_U -> 'U'
 * Used by the TYPE() built-in (S3.5). The returned char is always ASCII.
 */
char xb_type_char(xb_type t);

/* ---- Equality ---- */

/*
 * xb_eq: raw per-type equality. Returns 1 if equal, 0 if not.
 *
 * This is NOT the xBase `=` operator. In particular:
 *   - C and M equality is byte-exact over the full length (both lengths must
 *     match). SET EXACT begins-with truncation is the evaluator's concern
 *     (S3.3), not this function.
 *   - N equality is exact IEEE double ==. No epsilon, no rounding.
 *   - D equality is JDN double == (same rule as N).
 *   - L equality: both normalised to 0/1, then compared.
 *   - U is never equal to anything, including another U.
 *   - Cross-type always returns 0.
 *
 * Ref: docs/plans/SAMIR-implementation-plan.md S0.6 contract;
 *      os/samir/include/samir/rt.h rt_memcmp (used in the implementation).
 */
int xb_eq(const xb_val *a, const xb_val *b);

#endif /* INITECH_SAMIR_VALUE_H */
