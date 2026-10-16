/*
 * os/apps/calcda/calc_core.h -- the Calculator desk accessory's ENGINE: the
 * four-function state machine and the 12-character display formatter
 * (THE ARTIFACT; bead initech-tdnl.17, GUI remediation R4.3).
 *
 * Pure, freestanding C (no libc, no 64-bit division -- the i386 tenant links
 * no libgcc): compiled into CALCULAT.EXE (os/apps/calcda/calcda.c) and, hosted, into
 * the oracle harness/proptest/test_calcda.c. The window, the keys and the font
 * live in calc.c; everything a key DOES lives here, so the click path and the
 * keyboard path are the SAME call (calc_press) by construction of the tenant
 * and by test (the parity leg).
 *
 * GROUND TRUTH (Law 1). The behaviour is the real Mac OS 8.1 Calculator run
 * under Basilisk II (../system7-decomp re/mint-results-007.md, captures
 * goldens/captures/s8_calc_* and s8_calcprobe_*), 101 key sequences typed into
 * it and the 12-character readout read back. Every rule below cites the probe
 * that settles it; spec/calcda/calcda_vectors.tsv is the transcription and the
 * oracle's golden.
 *
 *   CHAIN, NOT PRECEDENCE   2+3*4= shows 20 (p01): each operator applies the
 *                           pending one to the accumulator, left to right.
 *   OPERATOR REPLACES       5+*2= shows 10 (p14): an operator typed right
 *                           after an operator replaces it.
 *   '=' WITHOUT OPERAND     2+= and 2*= show 2 (s06/s07); 2+3== shows 5
 *                           (p06): '=' never repeats, it drops a pending
 *                           operator that has no operand.
 *   AFTER '='               a digit or '.' starts a new number (q11, s16); an
 *                           operator continues from the result (q12).
 *   ENTRY                   at most 13 characters as shown, the leading "0."
 *                           counted (p10, q23, s05); a second '.' is ignored
 *                           (q13); "." shows "0." (q14); leading zeros fold
 *                           (q15, s13); trailing zeros are kept while typing
 *                           ("1.00", s03).
 *   DIVISION BY ZERO        1/0= "infinity", 0-1/0= "-infinity", 0/0= "not a
 *                           number" (p03, t01, t02); arithmetic continues
 *                           through them (q20); a digit starts afresh (p04).
 *   CLEAR                   C / c / Clear (NumLock on the PC, Basilisk's
 *                           Clear) reset to "0" (p00, s10, s11); Escape,
 *                           Backspace and Return do nothing (s09, q18, q16).
 *   KEYPAD ENTER            is '=' (q17). The PC cooks keypad Enter and the
 *                           main Enter key to the SAME message (vkey 0x1C,
 *                           '\r'; os/flair/event.c), so both act as '='.
 *
 * THE READOUT (calc_format; the probes q01..v05):
 *   width 12 characters, sign included;
 *   THE DECISION RECORD: |v| rounded to 12 significant digits (SANE's
 *     decimal record) gives the exponent e and the integrality that choose
 *     the form -- 99999999999.9 is not integral (u01 "1E11"), 99999999999.99
 *     is (u02 "100000000000"); 1/100, whose extended value is 0.00999...,
 *     is e = -2 (r04 ".01").
 *   FIXED when e >= -2:
 *     integral:     all its digits, if they fit (100000000000 r01; 1E12 r12;
 *                   -1E11 s18 does not fit with its sign);
 *     otherwise:    f = 12 - sign - intdigits - 1 fraction digits (intdigits
 *                   0 when |v| < 1, the point leads: .33333333333 p08),
 *                   f >= 1 required (99999999999.9 -> 1E11 s19), trailing
 *                   zeros and point stripped (.3 q06, 1.5 r20);
 *   else SCIENTIFIC: d.ddddE[-]x with as many significant digits as fit in
 *     12 (9.9999998E15 p16, 3.3333333E-8 q24, -1.428571E-4 r21), trailing
 *     zeros and point stripped (1E-3 r05, 1E26 t05). 1E-3 is scientific
 *     although ".001" would fit (r05): the fixed range is by exponent.
 *   THE DIGITS are the EXACT binary value rounded to the width, ties to even:
 *     1.00000000005 shows 1.0000000001 (its binary value lies above the tie,
 *     v02), 1.00000000025 shows 1.0000000002 (below, v03), 1234567850000 the
 *     even 1.2345678E12 (an exact tie, v01).
 *   NaN "not a number" (unsigned, t02); +-inf "infinity" / "-infinity".
 *   [inferred] a negative zero shows "0" (no capture produces one).
 *
 * ARITHMETIC: IEEE/x87 extended (long double, 64-bit significand) -- the
 * SANE extended format the real Calculator computed in, and the same x87
 * operations on the i386 tenant and the x86 host oracle. Decimal entry is
 * converted exactly (13 digits fit the significand: N / 10^k is one correctly
 * rounded division) and the readout is rounded from the EXACT binary value
 * (calc_format's big-integer expansion; ties to even), never from a scaled
 * approximation.
 *
 * MUTANT KNOBS (Rule 6; -D on this TU only, harness/proptest/test_calcda.c and
 * the CALCULAT.EXE mutant builds):
 *   CALCDA_MUT_PRECEDENCE  -- '*' and '/' bind tighter than '+' and '-' (the
 *                           pocket-calculator-with-memory rule, not the Mac's).
 *   CALCDA_MUT_DIV0_ZERO   -- division by zero yields 0 instead of infinity.
 *   CALCDA_MUT_ENTRY_CAP14 -- the entry accepts 14 characters.
 *   CALCDA_MUT_FIXED_E3    -- the fixed range starts at e >= -3 (.001 fixed).
 *
 * ASCII-clean (Rule 12). No timestamps, no nondeterminism (Rule 11).
 */
#ifndef INITECH_APPS_CALC_CORE_H
#define INITECH_APPS_CALC_CORE_H

#include <stdint.h>

/* The readout width (characters) and the entry cap (characters as shown). */
#define CALC_DISPLAY_W   12
#define CALC_ENTRY_MAX   13
#define CALC_DISPLAY_CAP 16      /* buffer: 13 entry chars + NUL, rounded up */

/* The commands. A key on the window and a key on the keyboard both become one
 * of these and go through calc_press -- there is no second path. */
typedef enum calc_cmd {
    CALC_0 = 0, CALC_1, CALC_2, CALC_3, CALC_4,
    CALC_5, CALC_6, CALC_7, CALC_8, CALC_9,
    CALC_DOT,       /* 10 */
    CALC_ADD,       /* 11 */
    CALC_SUB,       /* 12 */
    CALC_MUL,       /* 13 */
    CALC_DIV,       /* 14 */
    CALC_EQ,        /* 15 */
    CALC_CLEAR,     /* 16 */
    CALC_NONE       /* 17: a key the Calculator ignores */
} calc_cmd_t;

typedef struct calc {
    long double acc;                 /* the accumulator (last result)        */
    uint8_t     pending;             /* CALC_ADD..CALC_DIV, or CALC_NONE     */
    uint8_t     entering;            /* 1 while a number is being typed      */
    uint8_t     len;                 /* entry length (chars, as shown)       */
#if defined(CALCDA_MUT_PRECEDENCE)
    long double lo_acc;              /* mutant only: the deferred + / - side */
    uint8_t     lo_op;
#endif
    char        entry[CALC_DISPLAY_CAP];
    char        disp[CALC_DISPLAY_CAP];   /* what the readout shows          */
} calc_t;

/* Power-on / Clear: the readout shows "0". */
void calc_init(calc_t *c);

/* One key. Updates c->disp. CALC_NONE is a no-op. */
void calc_press(calc_t *c, calc_cmd_t k);

/* The readout string (NUL-terminated, at most CALC_ENTRY_MAX chars). */
const char *calc_display(const calc_t *c);

/* The keyboard map: an EventRecord keyDown message ((vkey << 8) | ascii,
 * os/flair/event.c) -> the command, or CALC_NONE. */
calc_cmd_t calc_cmd_from_key(uint32_t message);

/* The readout formatter, exposed for the oracle: writes at most 12 chars +
 * NUL into out (>= CALC_DISPLAY_CAP bytes). */
void calc_format(long double v, char *out);

#endif /* INITECH_APPS_CALC_CORE_H */
