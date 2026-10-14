/*
 * os/i123/core/recalc.c -- RECALCULATION: natural, columnwise and rowwise
 * order, the iteration count, circular-reference detection (the CIRC
 * indicator), manual vs automatic, and the typed-entry verb for formulas
 * (THE ARTIFACT).
 *
 * bead: initech-94ah (I123 P2). See i123.h.
 *
 * Ref (Law 1): R2.2 Reference Manual, /Worksheet Global Recalculation,
 * p. 12-31..12-33 (PDF 275-277; OCR build/man/ocr/p275..p277.txt):
 *   Natural    "Before recalculating a particular formula, 1-2-3
 *              recalculates any other formulas on which that formula
 *              depends."
 *   Columnwise "Starting in A1, 1-2-3 moves column by column through the
 *              worksheet, recalculating all formulas."
 *   Rowwise    "Starting in A1, 1-2-3 moves row by row ..."
 *   Automatic  each change recalculates; Manual recalculates "only when you
 *              press CALC (F9). The CALC indicator appears ... whenever any
 *              entries have changed since the last recalculation."
 *   Iteration  "the number of recalculation passes (from 1 to 50) ... used
 *              only when the recalculation order is Columnwise or Rowwise,
 *              or when the recalculation order is Natural and a circular
 *              reference exists."
 *   Minimal recalculation (natural order recalculates only what changed and
 *   its dependents) yields the values a full recalculation does for a sheet
 *   without circular references; with one, 1-2-3 "does a full
 *   recalculation" (p. 12-33). This engine always recalculates fully.
 * CIRC (p. 1-6, PDF 22, Status Indicators): "The worksheet contains a
 * formula that refers to itself (occurs only when the recalculation order
 * is Natural ...)" -- so only a natural-order recalculation sets it.
 * The .WK1 bytes: CALCMODE 0xFF automatic / 0x00 manual, CALCORDER 0x00
 * natural / 0x01 columnwise, CALCCOUNT = passes (MINT04, spec/i123/
 * wk1_format.h). Rowwise's CALCORDER byte is NOT minted: 0xFF here
 * [oracle-resolves; the classic WK1 literature value].
 *
 * MANUAL-MODE ENTRY: a formula typed in Manual mode is computed when it is
 * entered and nothing else recalculates: MINT04 (/wgrm, then +A1+1, +B1*2,
 * +C2+1, +A2, +B2 typed in order) caches 2, 4, 1, 1, 1 -- every value the
 * entered cell had at its own entry (re/mint-results-001.md; CALC on the
 * shot MINT04.shots/02_entered.png).
 *
 * NATURAL ORDER WITH A CYCLE [inferred from ONE golden, CIRC01]. A depth-
 * first walk evaluates each formula after its precedents, sweeping the
 * formula cells in row-major order. A formula's reference to ITSELF is not
 * a precedent (it reads its current value) but is a circular reference.
 * When the walk meets a precedent already on the walk (a cycle), the
 * formula that met it is NOT evaluated now: it is left for later in the
 * sweep, and the formula below it on the walk continues. A new formula
 * cell's value starts at 0. Typing CIRC01's four cells in that order with
 * automatic recalculation (each entry a full natural recalculation, one
 * pass) reproduces all three cached numbers 123.EXE wrote (-38, -19, -8.5)
 * and its string ("small"); the alternative rule (skip the cycle edge and
 * evaluate) gives -47.2 / -23.6 / -6.9. This lane's model only: a second
 * circular golden would test it.
 *
 * Freestanding; ASCII-clean (Rule 12); deterministic (Rule 11).
 */
#include "i123.h"
#include "i123/wk1_format.h"
#include "i123/wk1_formula.h"

int i123_cell_set_cache(i123_sheet_t *sh, uint32_t idx, const uint8_t v[8],
                        int has_str, const uint8_t *s, uint32_t n);

#define ST_NEW    0u
#define ST_STACK  1u
#define ST_DONE   2u

#define ORDER_NATURAL    0x00u
#define ORDER_COLUMNWISE 0x01u
#define MODE_MANUAL      0x00u

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

int i123_calc_init(i123_calc_t *cx, void *mem, uint32_t len, uint32_t cell_cap,
                   i123_calc_t *nested)
{
    uintptr_t p, end;
    if (cx == (i123_calc_t *)0 || mem == (void *)0 || cell_cap == 0u) return I123_E_ARG;
    p = (uintptr_t)mem;
    end = p + len;
    if (len < I123_CALC_BYTES(cell_cap)) return I123_E_ARG;
    p = (p + 7u) & ~(uintptr_t)7u;
    cx->frames = (i123_frame_t *)p;
    p += (uintptr_t)cell_cap * sizeof(i123_frame_t);
    cx->state = (uint8_t *)p;
    p += cell_cap;
    if (p > end) return I123_E_ARG;
    cx->cap = cell_cap;
    cx->arena_used = 0;
    cx->rand_state = 0x1A2B3C4Du;     /* fixed seed: deterministic @RAND */
    cx->have_now = 0;
    cx->now = 0.0;
    cx->work = 0;
    cx->nested = nested;
    return I123_OK;
}

/* Index of the cell at (col,row) in the store, or -1. */
static int32_t index_of(const i123_sheet_t *sh, uint16_t col, uint16_t row)
{
    const i123_cell_t *c = i123_find(sh, col, row);
    if (c == (const i123_cell_t *)0) return -1;
    return (int32_t)(c - sh->cells);
}

/* ---- storing a value ------------------------------------------------------ */
static const uint8_t ERR_BYTES[8] = { 0, 0, 0, 0, 0, 0, 0xF0, 0x7F };   /* +inf, 40/40 */
static const uint8_t NA_BYTES[8]  = { 0, 0, 0, 0, 0, 0, 0xF0, 0xFF };   /* -inf, 12/12 */
/* A string result caches a NaN; 123.EXE's own NaN payload is a private
 * string handle (110 distinct patterns in the goldens, e.g. MINT01
 * `fd 9f 0a 00 00 00 f0 7f`), so a cell that already holds a string keeps
 * its bytes and a new one takes this pattern of the same shape
 * (exponent all ones, bits 51..24 clear) [inferred]. */
static const uint8_t STR_BYTES[8] = { 0x01, 0, 0, 0, 0, 0, 0xF0, 0x7F };

static int is_nan_bytes(const uint8_t v[8])
{
    uint32_t ex = ((uint32_t)(v[7] & 0x7Fu) << 4) | (uint32_t)(v[6] >> 4);
    int mant0 = (v[6] & 0x0Fu) == 0u && v[5] == 0u && v[4] == 0u && v[3] == 0u &&
                v[2] == 0u && v[1] == 0u && v[0] == 0u;
    return ex == 0x7FFu && !mant0;
}

int i123_store_result(i123_sheet_t *sh, uint32_t idx, const i123_val_t *v)
{
    uint8_t bytes[8];
    const i123_cell_t *c;
    if (idx >= sh->ncells) return I123_E_ARG;
    c = &sh->cells[idx];
    if (c->kind != I123_K_FORMULA) return I123_E_ARG;
    switch (v->t) {
    case I123_V_NUM: {
        union { double d; uint8_t b[8]; } u;
        u.d = v->n;
#ifdef I123_MUT_CACHE_ROUND
        /* MUTANT (Rule 6; test-i123-formula-mutant): the cache is rounded
         * to 6 decimals -- 39.166666666666664 is stored as 39.166667, so
         * every non-terminating golden cache goes RED. NEVER real. */
        {
            double s = u.d * 1e6;
            s = (double)(int64_t)(s < 0.0 ? s - 0.5 : s + 0.5);
            u.d = s / 1e6;
        }
#endif
        for (int i = 0; i < 8; i++) bytes[i] = u.b[i];
        return i123_cell_set_cache(sh, idx, bytes, 0, (const uint8_t *)0, 0);
    }
    case I123_V_NA:
        return i123_cell_set_cache(sh, idx, NA_BYTES, 0, (const uint8_t *)0, 0);
    case I123_V_STR: {
        uint8_t text[I123_STR_MAX];
        uint32_t n = v->slen;
        if (n > I123_STR_MAX) return i123_cell_set_cache(sh, idx, ERR_BYTES, 0, (const uint8_t *)0, 0);
        for (uint32_t i = 0; i < n; i++) text[i] = v->s[i];   /* off the pool first */
        for (int i = 0; i < 8; i++) bytes[i] = (c->has_str && is_nan_bytes(c->v)) ? c->v[i] : STR_BYTES[i];
        return i123_cell_set_cache(sh, idx, bytes, 1, text, n);
    }
    default:
        return i123_cell_set_cache(sh, idx, ERR_BYTES, 0, (const uint8_t *)0, 0);
    }
}

static int eval_store(i123_calc_t *cx, i123_sheet_t *sh, uint32_t idx)
{
    i123_val_t v;
    const i123_cell_t *c = &sh->cells[idx];
    int rc = i123_eval(cx, sh, c->col, c->row, sh->pool + c->off, c->len, &v);
    if (rc == I123_E_FORMAT) { v.t = I123_V_ERR; v.cell = 0; }   /* undecodable: ERR */
    else if (rc != I123_OK) return rc;
    return i123_store_result(sh, idx, &v);
}

/* ---- precedents ------------------------------------------------------------- */

/* The next formula precedent of frame f, advancing f; -1 when none left.
 * A reference to f's own cell sets *self. */
static int32_t next_precedent(const i123_sheet_t *sh, i123_frame_t *f, int *self)
{
    const i123_cell_t *c = &sh->cells[f->cell];
    const uint8_t *code = sh->pool + c->off;
    uint32_t len = c->len;
    for (;;) {
        uint8_t t;
        uint32_t n;
        if (f->inrange) {
            /* scanning a RANGE: f->ri is the store index to continue at; the
             * corners are re-derived from the token at f->pc */
            const uint8_t *p = code + f->pc + 1u;
            int32_t c0 = i123_ref_resolve(rd16(p), c->col, 1), r0 = i123_ref_resolve(rd16(p + 2), c->row, 0);
            int32_t c1 = i123_ref_resolve(rd16(p + 4), c->col, 1), r1 = i123_ref_resolve(rd16(p + 6), c->row, 0);
            int32_t lc = c0 < c1 ? c0 : c1, hc = c0 < c1 ? c1 : c0;
            int32_t lr = r0 < r1 ? r0 : r1, hr = r0 < r1 ? r1 : r0;
            uint32_t k = f->ri;
            for (; k < sh->ncells; k++) {
                const i123_cell_t *q = &sh->cells[k];
                if ((int32_t)q->row < lr || r0 < 0 || r1 < 0) continue;
                if ((int32_t)q->row > hr) { k = sh->ncells; break; }
                if ((int32_t)q->col < lc || (int32_t)q->col > hc || q->kind != I123_K_FORMULA) continue;
                if (k == f->cell) { *self = 1; continue; }
                f->ri = k + 1u;
                return (int32_t)k;
            }
            f->inrange = 0;
            f->pc = (uint16_t)(f->pc + 9u);
            continue;
        }
        if (f->pc >= len) return -1;
        t = code[f->pc];
        n = i123_token_size(code, len, f->pc);
        if (t == WKF_RETURN || n == 0u) { f->pc = (uint16_t)len; return -1; }
        if (t == WKF_VAR) {
            int32_t cc = i123_ref_resolve(rd16(code + f->pc + 1u), c->col, 1);
            int32_t rr = i123_ref_resolve(rd16(code + f->pc + 3u), c->row, 0);
            f->pc = (uint16_t)(f->pc + n);
            if (rr >= 0) {
                int32_t k = index_of(sh, (uint16_t)cc, (uint16_t)rr);
#ifdef I123_MUT_NO_PRECEDENTS
                /* MUTANT (Rule 6; test-i123-recalc-order-mutant): VAR
                 * references are not precedents, so natural order degrades
                 * to the sweep -- a forward reference reads a stale value.
                 * NEVER in a real build. */
                k = -1;
#endif
                if (k >= 0 && sh->cells[k].kind == I123_K_FORMULA) {
                    if ((uint32_t)k == f->cell) { *self = 1; continue; }
                    return k;
                }
            }
            continue;
        }
        if (t == WKF_RANGE) {
            /* find the first store index at or after row lr (binary search
             * through i123_find's order: rows are the major key) */
            const uint8_t *p = code + f->pc + 1u;
            int32_t r0 = i123_ref_resolve(rd16(p + 2), c->row, 0);
            int32_t r1 = i123_ref_resolve(rd16(p + 6), c->row, 0);
            uint32_t lo = 0, hi = sh->ncells;
            int32_t lr = r0 < r1 ? r0 : r1;
            if (r0 < 0 || r1 < 0) { f->pc = (uint16_t)(f->pc + n); continue; }
            while (lo < hi) {
                uint32_t mid = lo + (hi - lo) / 2u;
                if ((int32_t)sh->cells[mid].row < lr) lo = mid + 1u; else hi = mid;
            }
            f->ri = lo;
            f->inrange = 1;
            continue;
        }
        f->pc = (uint16_t)(f->pc + n);
    }
}

/* ---- natural order --------------------------------------------------------------- */
typedef struct {
    uint8_t  circ;
    uint16_t cc, cr;
} circ_t;

static void note_circ(const i123_sheet_t *sh, circ_t *ci, uint32_t idx)
{
    if (!ci->circ) { ci->cc = sh->cells[idx].col; ci->cr = sh->cells[idx].row; }
    ci->circ = 1;
}

/* One natural-order pass (evaluate = 0 would only detect cycles). */
static int natural_pass(i123_calc_t *cx, i123_sheet_t *sh, int evaluate, circ_t *ci)
{
    uint32_t budget = 64u * sh->ncells + 1024u, work = 0;
    for (uint32_t i = 0; i < sh->ncells; i++) cx->state[i] = ST_NEW;
    for (uint32_t x = 0; x < sh->ncells; x++) {
        uint32_t sp = 0;
        if (sh->cells[x].kind != I123_K_FORMULA || cx->state[x] == ST_DONE) continue;
        cx->frames[0].cell = x; cx->frames[0].pc = 0; cx->frames[0].inrange = 0; cx->frames[0].ri = 0;
        cx->state[x] = ST_STACK;
        sp = 1;
        while (sp > 0u) {
            i123_frame_t *f = &cx->frames[sp - 1u];
            int self = 0, abandon = 0;
            int32_t p;
            for (;;) {
                p = next_precedent(sh, f, &self);
                if (self) { note_circ(sh, ci, f->cell); self = 0; }
                if (p < 0) break;
                if (cx->state[p] == ST_DONE) continue;
                if (cx->state[p] == ST_STACK) {
                    note_circ(sh, ci, (uint32_t)p);
                    if (work < budget) { abandon = 1; break; }
                    continue;   /* over budget: evaluate with what is there */
                }
                break;
            }
            if (abandon) {
                cx->state[f->cell] = ST_NEW;     /* left for later in the sweep */
                sp--;
                continue;
            }
            if (p >= 0) {
                if (sp >= cx->cap) return I123_E_FULL;
                work++;
                cx->frames[sp].cell = (uint32_t)p; cx->frames[sp].pc = 0;
                cx->frames[sp].inrange = 0; cx->frames[sp].ri = 0;
                cx->state[p] = ST_STACK;
                sp++;
                continue;
            }
            if (evaluate) {
                int rc = eval_store(cx, sh, f->cell);
                if (rc != I123_OK) return rc;
            }
            cx->state[f->cell] = ST_DONE;
            sp--;
        }
    }
    return I123_OK;
}

/* ---- columnwise / rowwise ------------------------------------------------------ */
static int sweep_pass(i123_calc_t *cx, i123_sheet_t *sh, int columnwise)
{
    if (!columnwise) {
        for (uint32_t i = 0; i < sh->ncells; i++) {
            if (sh->cells[i].kind != I123_K_FORMULA) continue;
            int rc = eval_store(cx, sh, i);
            if (rc != I123_OK) return rc;
        }
        return I123_OK;
    }
    /* column-major: column by column from A, each top to bottom (the store
     * is row-major, so one scan per occupied column) */
    for (uint32_t col = 0; col < I123_MAX_COLS; col++) {
        for (uint32_t i = 0; i < sh->ncells; i++) {
            if (sh->cells[i].col != col || sh->cells[i].kind != I123_K_FORMULA) continue;
            int rc = eval_store(cx, sh, i);
            if (rc != I123_OK) return rc;
        }
    }
    return I123_OK;
}

int i123_recalc(i123_calc_t *cx, i123_sheet_t *sh)
{
    circ_t ci;
    uint32_t passes = sh->calccount;
    int rc = I123_OK;
    if (sh->ncells > cx->cap) return I123_E_FULL;
    ci.circ = 0; ci.cc = 0; ci.cr = 0;
    if (passes == 0u) passes = 1u;
    if (passes > 50u) passes = 50u;          /* p. 12-32: 1 to 50 */
    if (sh->calcorder == ORDER_NATURAL) {
        rc = natural_pass(cx, sh, 1, &ci);
        /* iterate only when a circular reference exists (p. 12-32) */
        for (uint32_t k = 1; rc == I123_OK && ci.circ && k < passes; k++)
            rc = natural_pass(cx, sh, 1, &ci);
    } else {
        /* no CIRC outside natural order: the indicator "occurs only when the
         * recalculation order is Natural" (manual p. 1-6, PDF 22) */
        for (uint32_t k = 0; rc == I123_OK && k < passes; k++)
            rc = sweep_pass(cx, sh, sh->calcorder == ORDER_COLUMNWISE);
    }
    if (rc != I123_OK) return rc;
#ifdef I123_MUT_CIRC_BLIND
    /* MUTANT (Rule 6; test-i123-recalc-order-mutant): the CIRC indicator
     * never lights. NEVER in a real build. */
    ci.circ = 0;
#endif
    sh->circ = ci.circ;
    sh->circ_col = ci.cc;
    sh->circ_row = ci.cr;
    sh->calc = 0;
    return I123_OK;
}

int i123_recalc_cell(i123_calc_t *cx, i123_sheet_t *sh, uint16_t col, uint16_t row)
{
    int32_t k = index_of(sh, col, row);
    if (k < 0 || sh->cells[k].kind != I123_K_FORMULA) return I123_E_ARG;
    return eval_store(cx, sh, (uint32_t)k);
}

int i123_after_entry(i123_calc_t *cx, i123_sheet_t *sh)
{
    if (sh->calcmode == MODE_MANUAL) { sh->calc = 1; return I123_OK; }
    return i123_recalc(cx, sh);
}

int i123_enter_formula(i123_calc_t *cx, i123_sheet_t *sh, uint16_t col, uint16_t row,
                       const char *text, uint32_t len)
{
    uint8_t code[I123_CODE_MAX];
    uint32_t n;
    static const uint8_t ZERO[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };   /* a new formula starts at 0 */
    const i123_cell_t *old = i123_find(sh, col, row);
    uint8_t fmt = old ? old->fmt : (uint8_t)WK1_FMT_FRESH_CELL;
    int rc = i123_formula_compile(text, len, col, row, code, sizeof code, &n);
    if (rc != I123_OK) return rc;
    rc = i123_set_formula_raw(sh, col, row, fmt, ZERO, code, n, (const char *)0, 0);
    if (rc != I123_OK) return rc;
    if (sh->calcmode == MODE_MANUAL) {
        rc = i123_recalc_cell(cx, sh, col, row);
        sh->calc = 1;
        return rc;
    }
    return i123_recalc(cx, sh);
}
