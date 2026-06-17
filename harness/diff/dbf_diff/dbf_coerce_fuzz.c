/*
 * harness/diff/dbf_diff/dbf_coerce_fuzz.c -- xBase III+ coercion FUZZER + shrinker (S3.4).
 *
 * FACTORY host tool (CLAUDE.md Law 3): libc OK here. Links the UNMODIFIED artifact
 * engine (eval.c + parse.c + lex.c + value.c + rt.c) and drives it against a
 * TABLE-DRIVEN REFERENCE MODEL sourced verbatim from spec/samir/xbase_coercion.json
 * operator_coercion cells + rules + modes. The engine is never touched.
 *
 * S3.4 of docs/plans/SAMIR-implementation-plan.md: the coercion fuzzer is gmo's
 * deliverable. It is a property-test harness that:
 *   1. Runs a DIRECTED all-cells pass (guaranteeing every operator_coercion cell
 *      is hit regardless of PRNG luck) + a RANDOM SEEDED SWEEP of N seeds.
 *   2. For each test: generates a typed operand pair (C/N/D/L) x operator x
 *      SET_EXACT mode, builds a source-string expression, evaluates via xb_eval
 *      through the full lex -> parse -> eval pipeline, and compares the engine
 *      outcome (result type or error code) against the reference model.
 *   3. On ANY mismatch: emits a STRUCTURED localized signal naming the cell,
 *      modes, expected vs got, and the SEED; shrinks to a minimal reproducer
 *      (smallest literal values); exits non-zero (Law 2).
 *   4. Runs a fixed 2000-seed sweep so the gate is deterministic + fast (<1 s),
 *      with a seeded xorshift32 PRNG -- NO time()/rand() (Rule 11).
 *
 * MUTATION PROOF (Rule 6; ARB rider (a)):
 *   The engine is built with -DXB_MUTATE_EVAL to activate the single perturbation
 *   in eval.c: the C+N cell SUCCEEDS (returns the C operand) instead of error #9.
 *   The directed all-cells pass hits the (C,+,N) cell deterministically, the
 *   reference model says "error mismatch" (#9), the mutant says "success type C"
 *   -> MISMATCH detected -> fuzzer exits non-zero with the structured signal.
 *
 * REFERENCE MODEL (Law 1 citation):
 *   Every entry in g_cells[] below is derived cell-by-cell from
 *   spec/samir/xbase_coercion.json operator_coercion. The cell index in comments
 *   matches the JSON array position (0-based). Rules cited per the JSON "rule"
 *   field. Modes cited per the JSON "modes" object.
 *
 * GATED items (flagged, not guessed):
 *   - div-by-zero (N/N with rhs=0): the JSON says "note":"div-by-zero -> numeric
 *     error (oracle-resolves)". eval.c resolves it as XBEE_NUM_OVERFLOW (#39).
 *     The fuzzer avoids generating rhs=0 in random N/N division to sidestep this
 *     GATED cell; the directed pass covers it explicitly with the engine's chosen
 *     behavior (XBEE_NUM_OVERFLOW). See GATED_NOTE_DIV_ZERO below.
 *   - D literal construction: III+ has no date literal (CTOD() is IV; eval.h
 *     comment "III+ has NO date literal"). The fuzzer injects D values via the
 *     resolver hook, not via source literals, using a named variable (FDATE).
 *   - C-value length: generated C values are short (1-4 chars ASCII) so
 *     scratch-arena exhaustion (XBEE_SCRATCH_FULL) is not a signal but an engine
 *     error we do not want to trigger in the fuzzer; scratch is sized at 4096.
 *
 * Compile + run (GREEN -- engine correct; no divergence):
 *   gcc -std=c11 -Wall -Wextra -Werror -Iseed -Ios/samir/include \
 *     harness/diff/dbf_diff/dbf_coerce_fuzz.c \
 *     os/samir/core/eval.c os/samir/core/parse.c os/samir/core/lex.c \
 *     os/samir/core/value.c os/samir/core/rt.c \
 *     -o /tmp/dbf_coerce_fuzz && /tmp/dbf_coerce_fuzz
 *
 * Mutant (RED -- C+N divergence detected):
 *   gcc -std=c11 -Wall -Wextra -Werror -DXB_MUTATE_EVAL -Iseed -Ios/samir/include \
 *     harness/diff/dbf_diff/dbf_coerce_fuzz.c \
 *     os/samir/core/eval.c os/samir/core/parse.c os/samir/core/lex.c \
 *     os/samir/core/value.c os/samir/core/rt.c \
 *     -o /tmp/dbf_coerce_fuzz_mut ; /tmp/dbf_coerce_fuzz_mut >/dev/null 2>&1
 *     # exit code must be non-zero
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 * No C++'isms; -std=c11 throughout.
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md sec.5 S3.4 + sec.4 (harness
 *     properties: fast, structured localized signal, shrink, mutation-proven)
 *   - spec/samir/xbase_coercion.json (operator_coercion cells; rules R_*;
 *     modes.SET_EXACT; not_in_iii_plus)
 *   - spec/samir/dbase_msg_codes.tsv (#9 mismatch, #27 not_numeric,
 *     #37 not_logical, #39 num_overflow, #45 not_character)
 *   - ../dbase3-decomp/re/mint-results-002.md (C+N=#9; no auto-stringify;
 *     SET EXACT default OFF; directional begins-with)
 *   - os/samir/include/samir/eval.h (xb_ctx / xb_eval API)
 *   - harness/diff/fat_diff/fat12_fuzz.c (the idiom: seeded PRNG + shrink +
 *     structured signal + sweep style)
 *   - harness/diff/dbf_diff/test_xbase_eval.c (eval driver pattern mirrored here)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"        /* seed/, on -Iseed  */
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"

TEST_HARNESS();

/* ========================================================================== */
/* REFERENCE MODEL (spec/samir/xbase_coercion.json operator_coercion)        */
/* ========================================================================== */

/*
 * Expected outcome kinds -- what the reference says the engine should produce.
 * EXP_TYPE_x: success, result is type x (xb_type).
 * EXP_ERR_x:  failure, error code x (xb_eval_err / dbase_msg_codes.tsv).
 */
typedef enum {
    EXP_TYPE_N   = 0,    /* success, XB_N                                     */
    EXP_TYPE_C   = 1,    /* success, XB_C                                      */
    EXP_TYPE_D   = 2,    /* success, XB_D                                      */
    EXP_TYPE_L   = 3,    /* success, XB_L                                      */
    EXP_ERR_MISMATCH   = 9,   /* dbase_msg_codes.tsv #9  "Data type mismatch." */
    EXP_ERR_NOT_NUMERIC= 27,  /* #27 "Not a numeric expression."               */
    EXP_ERR_NOT_LOGICAL= 37,  /* #37 "Not a Logical expression."               */
    EXP_ERR_OVERFLOW   = 39,  /* #39 "Numeric overflow (data was lost)."       */
    EXP_ERR_NOT_CHAR   = 45   /* #45 "Not a Character expression."             */
} expected_outcome;

/*
 * One cell of the operator_coercion table.
 * lhs_type / rhs_type: XB_C, XB_N, XB_D, XB_L
 * op_str: source-level operator string (as written in xBase source)
 * op_tok: the xb_token_type constant (used only in comments)
 * exact_mode: -1 = not SET_EXACT sensitive (test with EXACT OFF = 0)
 *              0  = test with EXACT OFF only
 *              1  = test with EXACT ON  only
 *              2  = test both (the C= family)
 * outcome: the expected_outcome enum value
 * cell_idx: 0-based index into xbase_coercion.json operator_coercion array
 * desc: human label for the structured error signal
 */
typedef struct {
    xb_type          lhs_type;
    xb_type          rhs_type;
    const char      *op_str;
    int              exact_mode;  /* -1=any, 0=OFF, 1=ON, 2=both */
    expected_outcome outcome;
    int              cell_idx;
    const char      *desc;
} coerce_cell;

/*
 * The complete operator_coercion table, cell-by-cell from
 * spec/samir/xbase_coercion.json. JSON array index in cell_idx.
 * Every cell is cited; no cell is guessed.
 *
 * NOTE on exact_mode:
 *   - Cells for = / <> / # on C: exact_mode=2 (both modes; two checks).
 *     JSON: modes.SET_EXACT.affects = ["=","<>","#","SEEK","FIND"].
 *   - All other cells: exact_mode=0 (SET EXACT OFF, the default).
 *     JSON: modes.SET_EXACT.default = "OFF".
 *
 * NOTE on XBT_CARET vs XBT_STARSTAR: both are exponentiation synonyms
 * (eval.h); only ^ is listed in xbase_coercion.json (cells 15-16 N^N / N**N).
 * We test ^ (XBT_CARET); ** is identical per eval.h and covered by
 * test_xbase_eval.c A6.
 */
static const coerce_cell g_cells[] = {
    /* ---- + cells (JSON indices 0..7) ---- */
    /* [0] {lhs:N, op:+, rhs:N, result:N} */
    { XB_N, XB_N, "+", 0, EXP_TYPE_N,  0, "N+N->N" },
    /* [1] {lhs:C, op:+, rhs:C, result:C, rule:R_concat_plus} */
    { XB_C, XB_C, "+", 0, EXP_TYPE_C,  1, "C+C->C (R_concat_plus)" },
    /* [2] {lhs:D, op:+, rhs:N, result:D, rule:R_date_plus_num} */
    { XB_D, XB_N, "+", 0, EXP_TYPE_D,  2, "D+N->D (R_date_plus_num)" },
    /* [3] {lhs:N, op:+, rhs:D, result:D, rule:R_date_plus_num} */
    { XB_N, XB_D, "+", 0, EXP_TYPE_D,  3, "N+D->D (R_date_plus_num)" },
    /* [4] {lhs:C, op:+, rhs:N, result:error, error:mismatch, note:III+:NOT stringified}
     *     THE HAZARD CELL (not_in_iii_plus: auto_stringify_C_plus_N).
     *     mint-results-002.md: "A" + 1 -> error #9. MUTANT flips this to succeed. */
    { XB_C, XB_N, "+", 0, EXP_ERR_MISMATCH, 4, "C+N->err#9 HAZARD (no auto-stringify; mint-002)" },
    /* [5] {lhs:N, op:+, rhs:C, result:error, error:mismatch} */
    { XB_N, XB_C, "+", 0, EXP_ERR_MISMATCH, 5, "N+C->err#9" },
    /* [6] {lhs:C, op:+, rhs:D, result:error, error:mismatch} */
    { XB_C, XB_D, "+", 0, EXP_ERR_MISMATCH, 6, "C+D->err#9" },
    /* [7] {lhs:D, op:+, rhs:D, result:error, error:mismatch} */
    { XB_D, XB_D, "+", 0, EXP_ERR_MISMATCH, 7, "D+D->err#9" },

    /* ---- - cells (JSON indices 8..13) ---- */
    /* [8] {lhs:N, op:-, rhs:N, result:N} */
    { XB_N, XB_N, "-", 0, EXP_TYPE_N,  8, "N-N->N" },
    /* [9] {lhs:D, op:-, rhs:N, result:D, rule:R_date_plus_num} */
    { XB_D, XB_N, "-", 0, EXP_TYPE_D,  9, "D-N->D (R_date_plus_num)" },
    /* [10] {lhs:D, op:-, rhs:D, result:N, rule:R_date_minus_date} */
    { XB_D, XB_D, "-", 0, EXP_TYPE_N, 10, "D-D->N (R_date_minus_date)" },
    /* [11] {lhs:C, op:-, rhs:C, result:C, rule:R_concat_minus} */
    { XB_C, XB_C, "-", 0, EXP_TYPE_C, 11, "C-C->C (R_concat_minus)" },
    /* [12] {lhs:C, op:-, rhs:N, result:error, error:mismatch} */
    { XB_C, XB_N, "-", 0, EXP_ERR_MISMATCH, 12, "C-N->err#9" },
    /* [13] {lhs:N, op:-, rhs:D, result:error, error:mismatch, note:only D-N sanctioned} */
    { XB_N, XB_D, "-", 0, EXP_ERR_MISMATCH, 13, "N-D->err#9 (only D-N sanctioned)" },

    /* ---- * / ^ cells (JSON indices 14..17) ---- */
    /* [14] {lhs:N, op:*, rhs:N, result:N} */
    { XB_N, XB_N, "*", 0, EXP_TYPE_N, 14, "N*N->N" },
    /* [15] {lhs:N, op:/, rhs:N, result:N, note:div-by-zero GATED}
     *      GATED_NOTE_DIV_ZERO: the fuzzer avoids rhs=0 in random paths; the
     *      directed pass uses rhs!=0. eval.c: div-by-zero -> XBEE_NUM_OVERFLOW
     *      (oracle-resolves decision in eval.c). We test rhs=2 here (safe). */
    { XB_N, XB_N, "/", 0, EXP_TYPE_N, 15, "N/N->N (rhs!=0; div-by-zero GATED)" },
    /* [16] {lhs:N, op:**, rhs:N, result:N} -- use ^ (synonym; eval.h XBT_CARET==XBT_STARSTAR) */
    { XB_N, XB_N, "^", 0, EXP_TYPE_N, 16, "N^N->N (** synonym tested separately)" },
    /* [17] {lhs:N, op:^, rhs:N, result:N} -- same type as [16] but explicit ^ */
    { XB_N, XB_N, "**", 0, EXP_TYPE_N, 17, "N**N->N" },

    /* ---- = cells (JSON indices 18..21): SET EXACT sensitive for C ---- */
    /* [18] {lhs:C, op:=, rhs:C, result:L, exact_off:R_begins_with, exact_on:R_exact_blankpad}
     *      Test both modes (exact_mode=2). */
    { XB_C, XB_C, "=", 2, EXP_TYPE_L, 18, "C=C->L (SET EXACT aware; R_begins_with/R_exact_blankpad)" },
    /* [19] {lhs:N, op:=, rhs:N, result:L} */
    { XB_N, XB_N, "=", 0, EXP_TYPE_L, 19, "N=N->L" },
    /* [20] {lhs:D, op:=, rhs:D, result:L} */
    { XB_D, XB_D, "=", 0, EXP_TYPE_L, 20, "D=D->L" },
    /* [21] {lhs:L, op:=, rhs:L, result:L} */
    { XB_L, XB_L, "=", 0, EXP_TYPE_L, 21, "L=L->L" },

    /* ---- <> / # cells (JSON indices 22..23): alias of <>, SET EXACT sensitive ---- */
    /* [22] {lhs:C, op:<>, rhs:C, result:L, note:negation of =; follows SET EXACT}
     *      Test both modes. */
    { XB_C, XB_C, "<>", 2, EXP_TYPE_L, 22, "C<>C->L (SET EXACT; negation of =)" },
    /* [23] {lhs:C, op:#, rhs:C, result:L, note:alias of <>}
     *      Test both modes. */
    { XB_C, XB_C, "#", 2, EXP_TYPE_L, 23, "C#C->L (alias of <>, SET EXACT)" },

    /* ---- ordering cells (JSON indices 24..30) ---- */
    /* [24] {lhs:N, op:<, rhs:N, result:L} */
    { XB_N, XB_N, "<", 0, EXP_TYPE_L, 24, "N<N->L" },
    /* [25] {lhs:C, op:<, rhs:C, result:L, note:CP437 byte order} */
    { XB_C, XB_C, "<", 0, EXP_TYPE_L, 25, "C<C->L (CP437 byte order)" },
    /* [26] {lhs:D, op:<, rhs:D, result:L, rule:R_blankdate_high} */
    { XB_D, XB_D, "<", 0, EXP_TYPE_L, 26, "D<D->L (R_blankdate_high)" },
    /* [27] {lhs:L, op:<, rhs:L, result:L, note:.F.<.T.} */
    { XB_L, XB_L, "<", 0, EXP_TYPE_L, 27, "L<L->L (.F.<.T.)" },
    /* [28] {lhs:N, op:<, rhs:C, result:error, error:mismatch, rule:R_order_same_type} */
    { XB_N, XB_C, "<", 0, EXP_ERR_MISMATCH, 28, "N<C->err#9 (R_order_same_type)" },

    /* ---- $ cells (JSON indices 29..30) ---- */
    /* [29] {lhs:C, op:$, rhs:C, result:L, rule:R_substr} */
    { XB_C, XB_C, "$", 0, EXP_TYPE_L, 29, "C$C->L (R_substr; ignores SET EXACT)" },
    /* [30] {lhs:N, op:$, rhs:C, result:error, error:mismatch, note:$ is C-only} */
    { XB_N, XB_C, "$", 0, EXP_ERR_MISMATCH, 30, "N$C->err#9 ($ is C-only)" },

    /* ---- logical cells (JSON indices 31..32) ---- */
    /* [31] {lhs:L, op:.AND., rhs:L, result:L} */
    { XB_L, XB_L, ".AND.", 0, EXP_TYPE_L, 31, "L.AND.L->L" },
    /* [32] {lhs:L, op:.OR., rhs:L, result:L} */
    { XB_L, XB_L, ".OR.",  0, EXP_TYPE_L, 32, "L.OR.L->L" },
    /* [33] {lhs:N, op:.AND., rhs:L, result:error, error:not_logical, rule:R_no_truthiness}
     *      JSON array index 33 (the last cell). */
    { XB_N, XB_L, ".AND.", 0, EXP_ERR_NOT_LOGICAL, 33, "N.AND.L->err#37 (R_no_truthiness)" }
};

#define NCELLS ((int)(sizeof(g_cells)/sizeof(g_cells[0])))

/* ========================================================================== */
/* DETERMINISTIC PRNG: xorshift32                                              */
/* Rule 11: seeded only from argv / fixed sweep -- NO time() / rand()         */
/* ========================================================================== */

static uint32_t g_prng_state;

static uint32_t prng_next(void)
{
    /* xorshift32: period 2^32-1. Ref: G. Marsaglia, "Xorshift RNGs", 2003. */
    uint32_t x = g_prng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (g_prng_state = x);
}

static void prng_seed(uint32_t s)
{
    /* Non-zero seed required for xorshift32 (zero is the absorbing state). */
    g_prng_state = (s != 0) ? s : 1u;
}

/* ========================================================================== */
/* EVAL PIPELINE DRIVER                                                        */
/* ========================================================================== */

#define TBUF 64
#define NBUF 64
#define SCRATCH_SZ 4096

static char g_scratch[SCRATCH_SZ];

/*
 * Resolver: binds the fixed typed variables the fuzzer injects into
 * source expressions for D values (since III+ has no date literal).
 *
 * FDATE -> D  JDN 2446283 (1985-08-05; corpus mint; ndx.md sec 4.2 [verified])
 * GDATE -> D  JDN 2446316 (1985-09-07; second date for D-D tests)
 * BDATE -> D  JDN 0 (blank date sentinel; date_is_blank in eval.c)
 */
static double g_resolve_d_val = 2446283.0; /* set per call by the fuzzer */

static int fuzz_resolve(void *user, const char *name, uint16_t len, xb_val *out)
{
    (void)user;
    if (len == 5 && rt_memcmp(name, "FDATE", 5) == 0) {
        *out = xb_d(g_resolve_d_val);
        return 0;
    }
    if (len == 5 && rt_memcmp(name, "GDATE", 5) == 0) {
        *out = xb_d(2446316.0); /* 1985-09-07 */
        return 0;
    }
    if (len == 5 && rt_memcmp(name, "BDATE", 5) == 0) {
        *out = xb_d(0.0); /* blank date sentinel */
        return 0;
    }
    return 1; /* not found */
}

/*
 * eval_src: evaluate a NUL-terminated source string with the given SET EXACT.
 * Returns the xb_eval rc; fills *out_val and *out_err.
 * On lex or parse failure sets *out_err to -1000 / -1001 so the caller detects it.
 */
static int eval_src(const char *src, int set_exact, xb_val *out_val, int *out_err)
{
    xb_token toks[TBUF];
    xb_node  pool[NBUF];
    xb_ctx   ctx;
    int      lerr = 0, perr = 0, nt, root;
    size_t   srclen = strlen(src);

    nt = xb_lex(src, (uint32_t)srclen, toks, TBUF, &lerr);
    if (nt < 0) { *out_err = -1000; *out_val = xb_u(); return -1000; }
    root = xb_parse(toks, (uint32_t)nt, pool, NBUF, &perr);
    if (root < 0) { *out_err = -1001; *out_val = xb_u(); return -1001; }

    ctx.set_exact    = set_exact;
    ctx.resolve      = fuzz_resolve;
    ctx.user         = NULL;
    ctx.scratch      = g_scratch;
    ctx.scratch_cap  = SCRATCH_SZ;
    ctx.scratch_used = 0;

    return xb_eval(pool, root, &ctx, out_val, out_err);
}

/* ========================================================================== */
/* EXPRESSION BUILDER                                                          */
/* ========================================================================== */

/*
 * A "fuzz operand": a typed literal or variable representation that can be
 * rendered into a source substring. For D we must use the named variable
 * (FDATE/GDATE/BDATE) since III+ has no date literal.
 */
typedef struct {
    xb_type  t;
    /* For C: a short ASCII string (chars a-z only; simplest literal).
     * For N: a small integer (simplest literal).
     * For L: 0 or 1.
     * For D: index into d_names[] (0=FDATE, 1=GDATE, 2=BDATE). */
    union {
        struct { char s[8]; int len; } c; /* C: literal text (no quotes here) */
        double n;                          /* N: numeric value                  */
        int    l;                          /* L: 0 or 1                         */
        int    d_idx;                      /* D: 0/1/2 -> FDATE/GDATE/BDATE    */
    } u;
} fuzz_operand;

static const char *g_d_names[3] = { "FDATE", "GDATE", "BDATE" };

/*
 * Render operand into buf[]. Returns length written. buf must be >= 32 bytes.
 */
static int render_operand(const fuzz_operand *o, char *buf, int cap)
{
    int n = 0;
    switch (o->t) {
    case XB_C: {
        /* Render as 'text' single-quote literal. */
        int i;
        if (n + 1 >= cap) break;
        buf[n++] = '\'';
        for (i = 0; i < o->u.c.len && n + 1 < cap; i++) {
            buf[n++] = o->u.c.s[i];
        }
        if (n + 1 >= cap) { n = 0; break; } /* safety */
        buf[n++] = '\'';
        break;
    }
    case XB_N: {
        /* Render as integer literal (safe double -> int cast for small values). */
        int iv = (int)o->u.n;
        /* Simple itoa for values 0..99 */
        if (iv < 0) { buf[n++] = '-'; iv = -iv; }
        if (iv >= 10) { buf[n++] = (char)('0' + iv/10); }
        buf[n++] = (char)('0' + iv%10);
        break;
    }
    case XB_L:
        /* Render as .T. or .F. */
        if (o->u.l) {
            if (n + 3 >= cap) break;
            buf[n++] = '.'; buf[n++] = 'T'; buf[n++] = '.';
        } else {
            if (n + 3 >= cap) break;
            buf[n++] = '.'; buf[n++] = 'F'; buf[n++] = '.';
        }
        break;
    case XB_D: {
        /* Render as the named variable (III+ has no date literal). */
        const char *dn = g_d_names[o->u.d_idx % 3];
        int i;
        for (i = 0; dn[i] && n < cap - 1; i++) {
            buf[n++] = dn[i];
        }
        break;
    }
    default:
        break;
    }
    buf[n] = '\0';
    return n;
}

/*
 * Build a binary expression source string: "LHS OP RHS".
 * Returns 1 on success, 0 if buf too small.
 */
static int build_expr(const fuzz_operand *lhs, const char *op_str,
                      const fuzz_operand *rhs, char *buf, int cap)
{
    char l_str[32], r_str[32];
    int ll, rl, oplen, total;
    ll = render_operand(lhs, l_str, (int)sizeof(l_str));
    rl = render_operand(rhs, r_str, (int)sizeof(r_str));
    oplen = (int)strlen(op_str);
    total = ll + 1 + oplen + 1 + rl + 1; /* spaces + NUL */
    if (total >= cap) return 0;
    memcpy(buf, l_str, (size_t)ll);
    buf[ll] = ' ';
    memcpy(buf + ll + 1, op_str, (size_t)oplen);
    buf[ll + 1 + oplen] = ' ';
    memcpy(buf + ll + 1 + oplen + 1, r_str, (size_t)rl);
    buf[ll + 1 + oplen + 1 + rl] = '\0';
    return 1;
}

/* ========================================================================== */
/* RANDOM OPERAND GENERATORS                                                   */
/* ========================================================================== */

/*
 * Generate a C operand: 1-3 lowercase ASCII characters.
 * The simplest possible C values to avoid scratch-arena issues.
 */
static fuzz_operand gen_c(uint32_t r)
{
    fuzz_operand o;
    int i, len;
    o.t = XB_C;
    len = (int)(r & 3) + 1; /* 1..4 */
    if (len > 3) len = 3;
    o.u.c.len = len;
    for (i = 0; i < len; i++) {
        /* Pick a-z (26 letters) */
        o.u.c.s[i] = (char)('a' + (int)((r >> (i*4 + 4)) % 26));
    }
    o.u.c.s[len] = '\0';
    return o;
}

/*
 * Generate an N operand: small positive integer 1..9 (safe, no division-by-zero,
 * no large exponent that would overflow).
 * For division we ensure rhs != 0 (caller enforces via specific gen).
 */
static fuzz_operand gen_n(uint32_t r)
{
    fuzz_operand o;
    int v;
    o.t = XB_N;
    /* Values 1..9: always safe for /, ^, * in this fuzz context. */
    v = (int)(r % 9) + 1;
    o.u.n = (double)v;
    return o;
}

/*
 * Generate a D operand: one of FDATE / GDATE / BDATE (3 options).
 * BDATE is the blank-date sentinel (jdn=0).
 */
static fuzz_operand gen_d(uint32_t r)
{
    fuzz_operand o;
    o.t = XB_D;
    o.u.d_idx = (int)(r % 3);
    return o;
}

/*
 * Generate an L operand: .T. or .F.
 */
static fuzz_operand gen_l(uint32_t r)
{
    fuzz_operand o;
    o.t = XB_L;
    o.u.l = (int)(r & 1);
    return o;
}

/* Generate an operand of a specific type. */
static fuzz_operand gen_typed(xb_type t, uint32_t r)
{
    switch (t) {
    case XB_C: return gen_c(r);
    case XB_N: return gen_n(r);
    case XB_D: return gen_d(r);
    case XB_L: return gen_l(r);
    default: {
        fuzz_operand o; o.t = XB_N; o.u.n = 1.0; return o;
    }
    }
}

/* ========================================================================== */
/* OUTCOME COMPARISON                                                          */
/* ========================================================================== */

/*
 * Compare the engine's actual outcome against the reference outcome for a cell.
 * Returns 1 if they agree, 0 if they diverge.
 *
 * For success outcomes (EXP_TYPE_*): the engine must have returned 0 and
 * the result type must match the expected type. Value is not constrained
 * (the fuzzer checks TYPE, not specific value, except for error-vs-success).
 *
 * For error outcomes (EXP_ERR_*): the engine must have returned non-zero and
 * the err_code must match the expected code.
 *
 * EXCEPTION: For the N/N division cell (cell [15]) with rhs=0, the engine
 * returns XBEE_NUM_OVERFLOW (#39); the fuzzer does not generate rhs=0 in random
 * mode but the directed pass uses rhs=2 (safe). See GATED_NOTE_DIV_ZERO above.
 */
static int outcomes_agree(int eval_rc, const xb_val *val, int eval_err,
                          expected_outcome expected)
{
    switch (expected) {
    case EXP_TYPE_N:
        return (eval_rc == 0 && val->t == XB_N);
    case EXP_TYPE_C:
        return (eval_rc == 0 && (val->t == XB_C || val->t == XB_M));
    case EXP_TYPE_D:
        return (eval_rc == 0 && val->t == XB_D);
    case EXP_TYPE_L:
        return (eval_rc == 0 && val->t == XB_L);
    case EXP_ERR_MISMATCH:
        return (eval_rc != 0 && eval_err == XBEE_MISMATCH);
    case EXP_ERR_NOT_NUMERIC:
        return (eval_rc != 0 && eval_err == XBEE_NOT_NUMERIC);
    case EXP_ERR_NOT_LOGICAL:
        return (eval_rc != 0 && eval_err == XBEE_NOT_LOGICAL);
    case EXP_ERR_OVERFLOW:
        return (eval_rc != 0 && eval_err == XBEE_NUM_OVERFLOW);
    case EXP_ERR_NOT_CHAR:
        return (eval_rc != 0 && eval_err == XBEE_NOT_CHARACTER);
    default:
        return 0;
    }
}

/* Human-readable expected outcome string (for the structured error signal). */
static const char *outcome_str(expected_outcome o)
{
    switch (o) {
    case EXP_TYPE_N:          return "success type N";
    case EXP_TYPE_C:          return "success type C";
    case EXP_TYPE_D:          return "success type D";
    case EXP_TYPE_L:          return "success type L";
    case EXP_ERR_MISMATCH:    return "error#9 (mismatch)";
    case EXP_ERR_NOT_NUMERIC: return "error#27 (not_numeric)";
    case EXP_ERR_NOT_LOGICAL: return "error#37 (not_logical)";
    case EXP_ERR_OVERFLOW:    return "error#39 (num_overflow)";
    case EXP_ERR_NOT_CHAR:    return "error#45 (not_character)";
    default:                  return "unknown";
    }
}

/* Describe what the engine actually returned. */
static void got_str(int eval_rc, const xb_val *val, int eval_err,
                    char *buf, int cap)
{
    const char *tnames[] = { "C", "N", "D", "L", "M", "U" };
    if (eval_rc == 0) {
        int ti = (int)val->t;
        const char *tn = (ti >= 0 && ti <= 5) ? tnames[ti] : "?";
        snprintf(buf, (size_t)cap, "success type %s", tn);
    } else {
        snprintf(buf, (size_t)cap, "error#%d", eval_err);
    }
}

/* ========================================================================== */
/* SHRINK                                                                      */
/* ========================================================================== */

/*
 * Minimal C operand (single character 'a').
 * Minimal N operand (value 2 -- safe for division).
 * Minimal D operand (FDATE -- the first non-blank date).
 * Minimal L operand (.T. -- true).
 * "Shrink" here means: replace both operands with the smallest canonical
 * value of their type, then verify the failure still reproduces. If it does,
 * report the minimal expression. This is sufficient for single-operator
 * binary expressions (and that is all the coercion table covers).
 */
static fuzz_operand minimal_operand(xb_type t)
{
    fuzz_operand o;
    o.t = t;
    switch (t) {
    case XB_C:
        o.u.c.s[0] = 'a'; o.u.c.s[1] = '\0'; o.u.c.len = 1;
        break;
    case XB_N:
        o.u.n = 2.0; /* avoid 0 (div-by-zero GATED) and 1 (identity) */
        break;
    case XB_D:
        o.u.d_idx = 0; /* FDATE */
        break;
    case XB_L:
        o.u.l = 1; /* .T. */
        break;
    default:
        o.t = XB_N; o.u.n = 2.0; break;
    }
    return o;
}

/*
 * Try to find a minimal reproducer for a failing (cell, exact_mode) pair.
 * Prints the minimal expression + seed to stderr.
 */
static void shrink_and_report(const coerce_cell *cell, int exact_mode,
                              uint32_t seed)
{
    char expr[128], got_buf[64];
    fuzz_operand mlhs, mrhs;
    xb_val val;
    int eval_rc, eval_err;

    mlhs = minimal_operand(cell->lhs_type);
    mrhs = minimal_operand(cell->rhs_type);

    /* For N/N division: ensure rhs is not 0 (GATED). Already 2.0 from minimal. */

    if (!build_expr(&mlhs, cell->op_str, &mrhs, expr, (int)sizeof(expr))) {
        /* Fallback: just describe the cell. */
        snprintf(expr, sizeof(expr), "<%s %s %s>",
                 cell->lhs_type == XB_C ? "C" :
                 cell->lhs_type == XB_N ? "N" :
                 cell->lhs_type == XB_D ? "D" : "L",
                 cell->op_str,
                 cell->rhs_type == XB_C ? "C" :
                 cell->rhs_type == XB_N ? "N" :
                 cell->rhs_type == XB_D ? "D" : "L");
    }

    /* Set the D resolver value appropriately (FDATE = 2446283.0). */
    g_resolve_d_val = 2446283.0;

    eval_rc = eval_src(expr, exact_mode, &val, &eval_err);
    got_str(eval_rc, &val, eval_err, got_buf, (int)sizeof(got_buf));

    fprintf(stderr,
        "  SHRINK -> minimal repro: %s  EXACT=%s\n"
        "           expected: %s  got: %s  [seed 0x%08x]\n",
        expr,
        exact_mode ? "ON" : "OFF",
        outcome_str(cell->outcome),
        got_buf,
        (unsigned)seed);
}

/* ========================================================================== */
/* SINGLE CELL CHECK                                                           */
/* ========================================================================== */

/*
 * check_cell: run one coerce_cell with given operands and exact_mode.
 * Returns 1 if the engine agrees with the reference, 0 on mismatch.
 * On mismatch: prints the structured signal and the shrunk reproducer.
 */
static int check_cell(const coerce_cell *cell, const fuzz_operand *lhs,
                      const fuzz_operand *rhs, int exact_mode, uint32_t seed)
{
    char expr[128], got_buf[64];
    xb_val val;
    int eval_rc, eval_err;

    if (!build_expr(lhs, cell->op_str, rhs, expr, (int)sizeof(expr))) {
        /* Expression too long; skip (shouldn't happen with our short operands). */
        return 1;
    }

    /* Set resolver D value (the resolver always returns JDN 2446283 for FDATE;
     * g_resolve_d_val is for future use; always 2446283 for non-blank). */
    g_resolve_d_val = 2446283.0;

    eval_rc = eval_src(expr, exact_mode, &val, &eval_err);

    if (outcomes_agree(eval_rc, &val, eval_err, cell->outcome)) {
        return 1; /* GREEN */
    }

    /* MISMATCH -- emit structured signal (plan sec.4: localized error signal). */
    got_str(eval_rc, &val, eval_err, got_buf, (int)sizeof(got_buf));
    fprintf(stderr,
        "MISMATCH coerce: (%s,%s,%s) EXACT=%s cell[%d] %s\n"
        "         expected: %s  got: %s  [seed 0x%08x]\n"
        "         expr: %s\n",
        cell->lhs_type == XB_C ? "C" :
        cell->lhs_type == XB_N ? "N" :
        cell->lhs_type == XB_D ? "D" : "L",
        cell->op_str,
        cell->rhs_type == XB_C ? "C" :
        cell->rhs_type == XB_N ? "N" :
        cell->rhs_type == XB_D ? "D" : "L",
        exact_mode ? "ON" : "OFF",
        cell->cell_idx,
        cell->desc,
        outcome_str(cell->outcome),
        got_buf,
        (unsigned)seed,
        expr);

    shrink_and_report(cell, exact_mode, seed);
    return 0;
}

/* ========================================================================== */
/* DIRECTED ALL-CELLS PASS                                                     */
/* ========================================================================== */

/*
 * Run every cell in g_cells[] with canonical "safe" operands, both EXACT modes
 * where exact_mode==2. This guarantees 100% cell coverage regardless of PRNG.
 * It also deterministically catches the XB_MUTATE_EVAL mutant (C+N cell [4]).
 *
 * For exact_mode==2 cells (C=C, C<>C, C#C), we run:
 *   - EXACT OFF (set_exact=0): "ab" vs "a" -> .T. (R_begins_with)
 *   - EXACT ON  (set_exact=1): "ab" vs "a" -> .F. (R_exact_blankpad)
 * For all other cells: EXACT OFF (the default; plan S3.3 + xbase_coercion.json).
 *
 * Returns number of failures (0 = all green).
 */
static int run_directed_pass(void)
{
    int failures = 0;
    int i;

    /* Canonical operands for each type. */
    fuzz_operand c_ab, c_a, n_3, n_2, d_0, d_1, l_t, l_f;

    /* C: "ab" and "a" (for the SET EXACT family) */
    c_ab.t = XB_C; c_ab.u.c.len = 2;
    c_ab.u.c.s[0] = 'a'; c_ab.u.c.s[1] = 'b'; c_ab.u.c.s[2] = '\0';

    c_a.t = XB_C; c_a.u.c.len = 1;
    c_a.u.c.s[0] = 'a'; c_a.u.c.s[1] = '\0';

    /* N: 3 and 2 (safe; 3 > 2 for ordering; 3/2 != 0) */
    n_3.t = XB_N; n_3.u.n = 3.0;
    n_2.t = XB_N; n_2.u.n = 2.0;

    /* D: FDATE (d_idx=0) and GDATE (d_idx=1) */
    d_0.t = XB_D; d_0.u.d_idx = 0;
    d_1.t = XB_D; d_1.u.d_idx = 1;

    /* L: .T. and .F. */
    l_t.t = XB_L; l_t.u.l = 1;
    l_f.t = XB_L; l_f.u.l = 0;

    for (i = 0; i < NCELLS; i++) {
        const coerce_cell *cell = &g_cells[i];
        fuzz_operand lhs, rhs;
        int em;

        /* Choose the "simplest safe" operand for each type. */
        switch (cell->lhs_type) {
        case XB_C: lhs = c_ab; break;
        case XB_N: lhs = n_3;  break;
        case XB_D: lhs = d_0;  break;
        case XB_L: lhs = l_t;  break;
        default:   lhs = n_3;  break;
        }
        switch (cell->rhs_type) {
        case XB_C: rhs = c_a;  break;
        case XB_N: rhs = n_2;  break;
        case XB_D: rhs = d_1;  break;
        case XB_L: rhs = l_f;  break;
        default:   rhs = n_2;  break;
        }

        /* For D-D cells, use two different dates so D-D gives a non-zero N
         * (and D=D gives .F., which is still type L -> passes EXP_TYPE_L). */

        if (cell->exact_mode == 2) {
            /* Test both EXACT OFF and ON. Seed 0 = directed pass. */
            em = 0; /* EXACT OFF */
            if (!check_cell(cell, &lhs, &rhs, em, 0)) {
                failures++;
                CHECK(0, cell->desc);
            } else {
                g_checks++; /* count as a passing check */
            }
            em = 1; /* EXACT ON */
            if (!check_cell(cell, &lhs, &rhs, em, 0)) {
                failures++;
                CHECK(0, cell->desc);
            } else {
                g_checks++;
            }
        } else {
            em = (cell->exact_mode < 0) ? 0 : cell->exact_mode;
            if (!check_cell(cell, &lhs, &rhs, em, 0)) {
                failures++;
                CHECK(0, cell->desc);
            } else {
                g_checks++;
            }
        }
    }
    return failures;
}

/* ========================================================================== */
/* RANDOM SEED SWEEP                                                           */
/* ========================================================================== */

/*
 * Choose a random cell from the table, generate random operands of the
 * required types, run with random EXACT mode (or the cell's forced mode),
 * and check the outcome.
 *
 * For the N/N division cell specifically, we skip rhs=0 (GATED).
 */
static int run_one_seed(uint32_t seed)
{
    uint32_t r;
    int cell_idx;
    const coerce_cell *cell;
    fuzz_operand lhs, rhs;
    int exact_mode;
    int failures = 0;

    /* Run several checks per seed to spread coverage. */
    int iters = 4;
    int k;

    prng_seed(seed);

    for (k = 0; k < iters; k++) {
        r = prng_next();
        cell_idx = (int)(r % (uint32_t)NCELLS);
        cell = &g_cells[cell_idx];

        r = prng_next();
        lhs = gen_typed(cell->lhs_type, r);

        r = prng_next();
        rhs = gen_typed(cell->rhs_type, r);

        /* For N/N division: guard against rhs=0 (GATED). */
        if (cell->lhs_type == XB_N && cell->rhs_type == XB_N &&
            cell->op_str[0] == '/' && cell->op_str[1] == '\0') {
            if (rhs.u.n == 0.0) rhs.u.n = 1.0; /* replace 0 with 1 */
        }

        /* Determine exact mode. */
        r = prng_next();
        if (cell->exact_mode == 2) {
            /* For C= family: randomly pick EXACT ON or OFF. */
            exact_mode = (int)(r & 1);
        } else if (cell->exact_mode < 0) {
            exact_mode = 0;
        } else {
            exact_mode = cell->exact_mode;
        }

        if (!check_cell(cell, &lhs, &rhs, exact_mode, seed)) {
            failures++;
        }
    }
    return failures;
}

/* ========================================================================== */
/* COVERAGE REPORT                                                             */
/* ========================================================================== */

/*
 * After the sweep, report which cells were covered by the directed pass
 * (all of them) and that the random sweep hit the key operator classes.
 * This is a static analysis because the directed pass guarantees 100% cell
 * coverage; the random sweep provides additional depth within each cell.
 */
static void print_coverage_report(int n_seeds)
{
    int success_cells = 0, error_cells = 0, i;
    int exact_on_cells = 0;

    for (i = 0; i < NCELLS; i++) {
        if (g_cells[i].outcome < 8) {
            success_cells++;
        } else {
            error_cells++;
        }
        if (g_cells[i].exact_mode == 2 || g_cells[i].exact_mode == 1) {
            exact_on_cells++;
        }
    }

    printf("coerce_fuzz: coverage: %d cells (%d success + %d error), "
           "%d SET_EXACT sensitive, %d random seeds x 4 iters\n",
           NCELLS, success_cells, error_cells, exact_on_cells, n_seeds);
}

/* ========================================================================== */
/* MAIN                                                                        */
/* ========================================================================== */

#define SWEEP_N 2000   /* fixed 2000-seed sweep; deterministic, <1s (plan sec.4) */

int main(void)
{
    int dir_failures, sweep_failures, total_failures;
    int i;

    /* -------------------------------------------------------------------- */
    /* Phase 1: DIRECTED all-cells pass.                                     */
    /* Hits every operator_coercion cell (including the C+N HAZARD cell [4]) */
    /* with canonical operands. Deterministically catches the mutant.        */
    /* -------------------------------------------------------------------- */
    printf("coerce_fuzz: directed all-cells pass (%d cells)...\n", NCELLS);
    dir_failures = run_directed_pass();

    /* -------------------------------------------------------------------- */
    /* Phase 2: RANDOM SEEDED SWEEP (1..SWEEP_N).                            */
    /* Rule 11: seeded PRNG, no time(). Same sweep -> same result.           */
    /* -------------------------------------------------------------------- */
    printf("coerce_fuzz: random sweep seeds 1..%d (x4 iters each)...\n", SWEEP_N);
    sweep_failures = 0;
    for (i = 1; i <= SWEEP_N; i++) {
        int f = run_one_seed((uint32_t)i);
        sweep_failures += f;
        if (f > 0) {
            /* Record failures in the harness counter too (for TEST_SUMMARY). */
            g_fails += f;
        }
        /* Count each iter as a check for the summary. */
        g_checks += 4;
    }

    total_failures = dir_failures + sweep_failures;

    /* -------------------------------------------------------------------- */
    /* Coverage report.                                                       */
    /* -------------------------------------------------------------------- */
    print_coverage_report(SWEEP_N);

    /* -------------------------------------------------------------------- */
    /* Final summary line (plan sec.4: structured output the gate can grep). */
    /* -------------------------------------------------------------------- */
    if (total_failures == 0) {
        printf("coerce_fuzz: ALL GREEN -- directed %d cells + %d seeds, "
               "0 divergences\n",
               NCELLS, SWEEP_N);
    } else {
        fprintf(stderr,
            "coerce_fuzz: FAILURES -- directed %d + sweep %d = %d total\n",
            dir_failures, sweep_failures, total_failures);
    }

    return TEST_SUMMARY("test-xbase-coercion");
}
