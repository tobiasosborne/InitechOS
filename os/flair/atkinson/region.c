/*
 * os/flair/atkinson/region.c -- the ATKINSON region engine (the artifact).
 *
 * beads: initech-jmo (rep + normalize-on-construction) / initech-b5g (scanline
 *        merge + derived ops + complement + queries).
 * Ref:   PRD Sec 6.2 -- "the load-bearing math". A region is a pixel set in Z^2,
 *        represented per scanline by a sorted INVERSION LIST x0<x1<... at which
 *        membership toggles (a normalized RLE). Regions over a bounding rect
 *        form a Boolean algebra under union/intersect/diff/xor and complement;
 *        EVERY binary op is a SCANLINE MERGE of inversion lists. The normal form
 *        is the minimal inversion-point set (no redundant/adjacent toggles), and
 *        rasterization is a Boolean-algebra HOMOMORPHISM to the pixel powerset --
 *        the oracle (harness/proptest/test_region.c).
 * Ref:   spec/region_algebra.h -- the LOCKED contract: types, the 5 normal-form
 *        invariants (Sec 2), the 4 op truth tables (Sec 3), the storage caps
 *        (Sec 5), the explicit complement FRAME (Sec 3), the verbatim QuickDraw
 *        wrapper names (Sec 7). This file implements that header VERBATIM.
 *
 * DUAL-COMPILE (the console.c/int21.c/psp.c pattern; CLAUDE.md Law 3): compiles
 * freestanding for the kernel (gcc -m32 -ffreestanding -nostdlib -std=c11) AND
 * hosted for the property suite. NO host malloc: every region carries caller-
 * supplied rows[]/x_pool (arena/static-backed); a cap overflow FAILS LOUD
 * (Rule 2). The band-merge scratch is fixed-size on the stack (RGN_ROW_X_MAX).
 *
 * THE FAIL-LOUD PATH (Rule 2; psp.c idiom): an invariant violation or a cap
 * overflow is never silently truncated. Hosted: abort() (the oracle observes a
 * non-zero exit). Freestanding: __builtin_trap() raises #UD, which the IDT
 * routes to the panic path (serial register dump + the PC LOAD LETTER banner)
 * and halts. region.c stays dependency-free (no panic.c / console.c pull-in) so
 * it is purely host-testable.
 *
 * MUTANTS (Rule 6, -DRGN_MUTATE_*): RGN_MUTATE_NO_VRLE (skip vertical-RLE
 * collapse), RGN_MUTATE_PARITY_OFF1 (off-by-one in the rasterize/contains-point
 * parity), RGN_MUTATE_EMIT_NOCHANGE (emit x even when boolfn output did not
 * change), RGN_MUTATE_NO_XMERGE_CAP (drop the per-band xmerge output-capacity
 * bound -- bead initech-mswo; caught by test-region's over-cap fail-loud probe
 * under ASAN, which sees the scratch overrun that the bound would have stopped).
 * Each must drive test-region RED, then restore GREEN.
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include "region.h"

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
#include <stdlib.h>           /* abort */
#define RGN_FAIL_LOUD() abort()
#else
#define RGN_FAIL_LOUD() __builtin_trap()
#endif

/* ===========================================================================
 * Tiny freestanding helpers (no <string.h> assumed present).
 * ===========================================================================*/
static void rgn_zero(void *p, uint32_t n)
{
    uint8_t *d = (uint8_t *)p;
    for (uint32_t i = 0; i < n; i++) d[i] = 0;
}

/* Are two x-lists byte-identical (same length, same values)? (fwd-declared so
 * region_assert_normal can use it before its definition below). */
static int xlist_equal(const int16_t *a, uint16_t na, const int16_t *b, uint16_t nb);

/* ===========================================================================
 * boolfn -- the op truth table as a function. Pinned to the LOCKED 4-bit table
 * (spec Sec 3): output bit = (rgn_op_truth(op) >> (inA*2 + inB)) & 1. A static
 * assert keeps the spec machine-checkable; this re-derives it at runtime.
 * ===========================================================================*/
static int boolfn(rgn_op_t op, int inA, int inB)
{
    unsigned t = rgn_op_truth(op);
    unsigned idx = (unsigned)((inA ? 2 : 0) + (inB ? 1 : 0));
    return (int)((t >> idx) & 1u);
}

/* ===========================================================================
 * 6a (initech-jmo): rep + normalize-on-construction.
 * ===========================================================================*/

void region_set_empty(region_t *r)
{
    if (r == 0) { RGN_FAIL_LOUD(); return; }
    /* Canonical empty: is_empty=1, n_rows=0, null bbox. Leave rows/x_pool
     * capacity and base pointers as the caller attached them. */
    r->bbox.top = r->bbox.left = r->bbox.bottom = r->bbox.right = 0;
    r->n_rows = 0;
    r->x_pool_used = 0;
    r->is_rect = 0;
    r->is_empty = 1;
}

/* A rect is EMPTY iff right<=left or bottom<=top (spec Sec 1, half-open). */
static int rect_is_empty(rgn_rect_t rc)
{
    return (rc.right <= rc.left) || (rc.bottom <= rc.top);
}

/* Append a row (y_top + a copy of `xs[0..k)`) to region `r`, drawing x-storage
 * from r->x_pool. FAILS LOUD on row/x-pool overflow (Rule 2, spec Sec 5). */
static void region_push_row(region_t *r, int16_t y_top,
                            const int16_t *xs, uint16_t k)
{
    if (r->n_rows >= r->cap_rows) { RGN_FAIL_LOUD(); return; }
    if ((uint32_t)r->x_pool_used + k > r->x_pool_cap) { RGN_FAIL_LOUD(); return; }
    rgn_row_t *row = &r->rows[r->n_rows];
    row->y_top = y_top;
    row->x_count = k;
    if (k == 0) {
        row->x = &r->x_pool[r->x_pool_used]; /* valid 0-length slice */
    } else {
        row->x = &r->x_pool[r->x_pool_used];
        for (uint16_t i = 0; i < k; i++) row->x[i] = xs[i];
        r->x_pool_used += k;
    }
    r->n_rows++;
}

void region_set_rect(region_t *r, rgn_rect_t rect)
{
    if (r == 0) { RGN_FAIL_LOUD(); return; }
    if (rect_is_empty(rect)) { region_set_empty(r); return; }

    /* Normal form of a rect: rows = [ (top, [left,right]), (bottom, []) ].
     * The top row carries the single span [left,right); the bottom row is the
     * closing/sentinel row (x_count==0, y_top==bbox.bottom). is_rect=1. */
    r->n_rows = 0;
    r->x_pool_used = 0;
    int16_t span[2] = { rect.left, rect.right };
    region_push_row(r, rect.top, span, 2);
    region_push_row(r, rect.bottom, 0, 0);
    r->bbox = rect;
    r->is_rect = 1;
    r->is_empty = 0;
}

/* ===========================================================================
 * Normal-form assertion (spec Sec 2). Returns 1 if normal; FAILS LOUD on
 * violation (it is the runtime guard at the top of every op, not a soft test).
 *
 * On invariant (4): an empty INTERIOR row is legal IFF it is a load-bearing
 * gap-closer -- i.e. it is preceded by a non-empty row (it turns a span OFF to
 * open a vertical hole) and it is NOT directly under another empty row (that
 * would be the redundant case (3)/(4) forbid). A LEADING empty row and an
 * empty-under-empty are violations. See region_normalize's banner for why the
 * gap-closer must be kept (the row model cannot otherwise encode a hole).
 * ===========================================================================*/
int region_assert_normal(const region_t *r)
{
    if (r == 0) { RGN_FAIL_LOUD(); return 0; }
    if (r->is_empty) {
        if (r->n_rows != 0) { RGN_FAIL_LOUD(); return 0; }
        return 1;
    }
    if (r->n_rows < 2) { RGN_FAIL_LOUD(); return 0; } /* >=1 live + closing row */
    /* The first row must be non-empty (no leading empty), the last must be the
     * empty closing row. */
    if (r->rows[0].x_count == 0) { RGN_FAIL_LOUD(); return 0; }
    if (r->rows[r->n_rows - 1].x_count != 0) { RGN_FAIL_LOUD(); return 0; }
    for (uint16_t i = 0; i < r->n_rows; i++) {
        const rgn_row_t *row = &r->rows[i];
        /* (2) EVEN length */
        if (row->x_count & 1u) { RGN_FAIL_LOUD(); return 0; }
        /* (1) STRICTLY INCREASING */
        for (uint16_t k = 1; k < row->x_count; k++)
            if (row->x[k] <= row->x[k - 1]) { RGN_FAIL_LOUD(); return 0; }
        /* (5) SORTED, UNIQUE y_top */
        if (i + 1 < r->n_rows && row->y_top >= r->rows[i + 1].y_top) {
            RGN_FAIL_LOUD(); return 0;
        }
    }
    /* (3)+(4) VERTICAL-RLE: no two consecutive rows share an identical x-list
     * (this also forbids the redundant empty-under-empty interior row). A single
     * empty interior row sitting between two DIFFERENT non-empty rows is a legal
     * gap-closer and is NOT flagged here. */
    for (uint16_t i = 0; i + 1 < r->n_rows; i++) {
        const rgn_row_t *a = &r->rows[i], *b = &r->rows[i + 1];
        if (xlist_equal(a->x, a->x_count, b->x, b->x_count)) { RGN_FAIL_LOUD(); return 0; }
    }
    return 1;
}

/* Are two x-lists byte-identical (same length, same values)? */
static int xlist_equal(const int16_t *a, uint16_t na, const int16_t *b, uint16_t nb)
{
    if (na != nb) return 0;
    for (uint16_t k = 0; k < na; k++) if (a[k] != b[k]) return 0;
    return 1;
}

/* ===========================================================================
 * region_normalize -- establish the normal form IN PLACE, idempotently.
 *
 * The row model (spec Sec 1): a row is valid for scanlines [y_top, next.y_top);
 * an EMPTY row (x_count==0) turns all spans OFF for its band, and the final
 * closing row (x_count==0, y_top==bbox.bottom) bounds the lowest live band. A
 * vertical GAP between two disjoint spans is therefore encoded by an empty row
 * at the gap's top -- that empty row is LOAD-BEARING and must be kept (it is the
 * only way the row model can represent a hole), so it is NOT a "redundant empty
 * interior row". Invariant (4) forbids ONLY redundant empties: a leading empty
 * (no live span opened yet) and a vertical-RLE-duplicate empty (an empty row
 * directly under another empty row). normalize drops exactly those, plus any
 * trailing empties beyond the single closing row.
 *   Ref: spec/region_algebra.h Sec 1 (row validity [y_top,next)) + Sec 2 inv.
 *        (3) vertical-RLE / (4) the closing-row clause. The literal phrase "no
 *        empty interior row" is scoped by its own justification ("a hole that
 *        (3) should have merged or normalize should have dropped") to REDUNDANT
 *        empties; a gap-closing empty is neither (3)-mergeable nor droppable,
 *        and without it the union of two vertically-disjoint rects -- a basic
 *        homomorphism-oracle case -- would be unrepresentable.
 *
 * Algorithm (single forward pass, idempotent):
 *   - Drop a row that is byte-identical to the previously-KEPT row (inv. 3,
 *     vertical-RLE) -- this also collapses empty-under-empty and span-under-
 *     identical-span.
 *   - Drop a LEADING empty row (nothing kept before it yet -- dead space above
 *     the first span).
 *   - Keep every other row, compacting its x-list to the front of the pool.
 *   - After the pass, trim any trailing run so EXACTLY ONE empty closing row
 *     remains (its y_top == the bbox bottom). An all-empty input -> canonical
 *     empty region.
 *   - Recompute the tight bbox; set is_rect / is_empty.
 * Because we only ever move x-data DOWN (dest offset <= source offset) and copy
 * via a scratch, the rewrite is in-place-safe; a second normalize finds nothing
 * to drop and re-compacts to byte-identical storage (idempotence, Rule 11).
 * ===========================================================================*/
void region_normalize(region_t *r)
{
    if (r == 0) { RGN_FAIL_LOUD(); return; }

    uint16_t out_rows = 0;
    uint32_t out_used = 0;
    int have_prev = 0;
    const int16_t *prev_x = 0;
    uint16_t prev_n = 0;

    for (uint16_t i = 0; i < r->n_rows; i++) {
        const int16_t *cur_x = r->rows[i].x;
        uint16_t cur_n = r->rows[i].x_count;
        int16_t  cur_y = r->rows[i].y_top;

        /* (3) vertical RLE: identical x-list to the previously-kept row. This
         * subsumes empty-under-empty (two closers/gaps in a row) and span-
         * under-identical-span -- the redundant row is dropped. */
#ifndef RGN_MUTATE_NO_VRLE
        if (have_prev && xlist_equal(prev_x, prev_n, cur_x, cur_n)) {
            continue;
        }
#endif
        /* A leading empty row (no live span opened yet) is dead space above the
         * region -- drop it (it would otherwise be a spurious interior empty). */
        if (!have_prev && cur_n == 0) {
            continue;
        }

        /* Keep this row; copy its x-list down to out_used (scratch-buffered so an
         * overlapping in-place move is safe). */
        int16_t tmp[RGN_ROW_X_MAX];
        if (cur_n > RGN_ROW_X_MAX) { RGN_FAIL_LOUD(); return; }
        for (uint16_t k = 0; k < cur_n; k++) tmp[k] = cur_x[k];
        if ((uint32_t)out_used + cur_n > r->x_pool_cap) { RGN_FAIL_LOUD(); return; }
        if (out_rows >= r->cap_rows) { RGN_FAIL_LOUD(); return; }

        int16_t *dst = &r->x_pool[out_used];
        for (uint16_t k = 0; k < cur_n; k++) dst[k] = tmp[k];
        r->rows[out_rows].y_top = cur_y;
        r->rows[out_rows].x_count = cur_n;
        r->rows[out_rows].x = dst;
        out_used += cur_n;
        out_rows++;

        prev_x = dst;
        prev_n = cur_n;
        have_prev = 1;
    }
#ifdef RGN_MUTATE_NO_VRLE
    /* The vertical-RLE collapse above is #ifdef'd OUT for this mutant; prev_x/
     * prev_n are then only written, never read. Mark them used so the mutant
     * still COMPILES (it must build to be able to go RED at runtime). */
    (void)prev_x; (void)prev_n;
#endif

    /* Trim trailing empty rows to exactly ONE closing row. After the pass any
     * empties at the tail are either the single closer (keep) or redundant
     * (drop). The (3) pass already collapsed empty-under-empty, so at most one
     * trailing empty survives -- but be defensive and trim any run. */
    while (out_rows >= 2 &&
           r->rows[out_rows - 1].x_count == 0 &&
           r->rows[out_rows - 2].x_count == 0) {
        out_rows--;   /* second-to-last is also empty -> last closer is redundant */
    }

    /* Count live (non-empty) rows. */
    uint16_t live = 0;
    for (uint16_t i = 0; i < out_rows; i++) if (r->rows[i].x_count) live++;
    if (live == 0) { region_set_empty(r); return; }

    /* The last row MUST be the empty closing row (the lowest span has a bottom).
     * If the input lacked one (a span running to +infinity), that is malformed. */
    if (r->rows[out_rows - 1].x_count != 0) { RGN_FAIL_LOUD(); return; }

    r->n_rows = out_rows;
    r->x_pool_used = out_used;

    /* Recompute the tight bbox: top = first row y_top, bottom = closing y_top,
     * left = min x[0], right = max x[last] over the live (non-empty) rows. */
    int16_t top = r->rows[0].y_top;
    int16_t bottom = r->rows[out_rows - 1].y_top;
    int16_t left = 0, right = 0;
    int first = 1;
    for (uint16_t i = 0; i < r->n_rows; i++) {
        if (r->rows[i].x_count == 0) continue;
        int16_t lx = r->rows[i].x[0];
        int16_t rx = r->rows[i].x[r->rows[i].x_count - 1];
        if (first) { left = lx; right = rx; first = 0; }
        else { if (lx < left) left = lx; if (rx > right) right = rx; }
    }
    r->bbox.top = top;
    r->bbox.left = left;
    r->bbox.bottom = bottom;
    r->bbox.right = right;
    r->is_empty = 0;

    /* is_rect: exactly one live row + the closer, whose single span spans the
     * full bbox width (rgnSize==10 analogue). */
    r->is_rect = (r->n_rows == 2 &&
                  r->rows[0].x_count == 2 &&
                  r->rows[0].x[0] == left && r->rows[0].x[1] == right) ? 1 : 0;
}

/* ===========================================================================
 * 6b (initech-b5g): the scanline merge.
 *
 * region_op(out, A, B, op) = a y-band sweep over the union of A's and B's row
 * y_tops; within each band [y, y_next) both regions present a FIXED inversion
 * list, so a per-band x-merge under boolfn(op) yields the band's output list.
 * Consecutive identical bands collapse in region_normalize.
 * ===========================================================================*/

/* The active x-list of region R for the band whose top scanline is `y`: the
 * x-list of the row with the greatest y_top <= y (or empty if y precedes R or R
 * is empty). Returns the count and sets *xp. */
static uint16_t active_xlist(const region_t *R, int16_t y, const int16_t **xp)
{
    *xp = 0;
    if (R->is_empty || R->n_rows == 0) return 0;
    /* rows are sorted by y_top; find the last row with y_top <= y. */
    const rgn_row_t *found = 0;
    for (uint16_t i = 0; i < R->n_rows; i++) {
        if (R->rows[i].y_top <= y) found = &R->rows[i];
        else break;
    }
    if (found == 0) return 0;          /* y is above the first row */
    *xp = found->x;
    return found->x_count;
}

/* Per-band x-merge: walk a[] and b[] (both strictly-increasing even lists) in
 * sorted order, tracking the (inA,inB) parity state. Emit an inversion point
 * into out[] EXACTLY when boolfn(op, inA, inB) changes value. Returns the
 * out-count (always even). `cap` is out's capacity in int16 slots; every emit
 * is bounds-checked against it BEFORE the write.
 *
 * THE PROVEN PER-BAND BOUND (initech-44ab RIDER 2, committee 2026-07-10 --
 * this RESOLVES the old 256-vs-640 cap question this comment used to defer):
 * one loop iteration consumes ONE DISTINCT x position -- a coincident A/B
 * edge is consumed in a single step (the take_a = take_b = 1 branch below),
 * toggling BOTH parities at one x -- and each iteration emits AT MOST one
 * point (only on a boolfn output change; a coincident double-toggle whose
 * output does not change emits NOTHING). Therefore
 *   out_count <= |distinct x in a UNION b|,
 * NOT the loose na+nb (which double-counts coincident edges and cancelled
 * toggles). Both input rows draw their coordinates from the same locked
 * [0,640] domain (ADR-0004 OD-3, 640x480 native), whose maximum even
 * inversion-point count is 640 ({0,1,...,639} = 320 unit spans), so a single
 * band's merged output is DOMAIN-BOUNDED: out_count <= 640 == RGN_ROW_X_MAX.
 * region_op's scratch[RGN_ROW_X_MAX] therefore provably suffices for every
 * domain-respecting pair of normal-form rows -- at the pre-44ab cap (256) that
 * claim was FALSE and realizable (bead initech-mswo), which is why the cap was
 * raised to EXACTLY the domain bound as a deliberate Rule-8 locked-spec change
 * (initech-44ab; spec/region_algebra.h Sec 5 "two distinct bounds" carries the
 * same derivation; ADR-0005 Sec 3.5 amendment).
 *
 * FAIL-LOUD BOUND (Rule 2; bead initech-mswo -- the check STAYS): the engine
 * does not clamp coordinates to the domain, so out-of-domain or malformed
 * inputs could still emit more; checking `no >= cap` before each write makes
 * the (cap+1)-th slot unreachable -- fail loud, never the pre-mswo silent
 * stack overrun. Consistent with region_normalize, which already fail-louds a
 * row exceeding RGN_ROW_X_MAX. Ref: PRD Sec 6.2; spec/region_algebra.h Sec 2
 * (normal form) + Sec 5 (storage caps + the two-bounds note). */
_Static_assert(RGN_ROW_X_MAX == 640u,
               "region_op's per-band scratch bound == the OD-3 640px domain "
               "bound; revisit the RIDER-2 derivation above if this moves "
               "(initech-44ab)");
static uint16_t xmerge(int16_t *out, uint16_t cap, rgn_op_t op,
                       const int16_t *a, uint16_t na,
                       const int16_t *b, uint16_t nb)
{
    uint16_t ia = 0, ib = 0, no = 0;
    int inA = 0, inB = 0;
    int prev_out = boolfn(op, 0, 0);    /* membership left of everything */
    /* boolfn(op,0,0) is 0 for all four ops (truth tables have LSB 0); the region
     * is bounded, so we start OUTSIDE. We assert that to be safe. */
    if (prev_out != 0) { RGN_FAIL_LOUD(); return 0; }
#ifdef RGN_MUTATE_NO_XMERGE_CAP
    /* The per-write `no >= cap` bound below is #ifdef'd OUT for this mutant;
     * `cap` is then never read. Mark it used so the mutant still COMPILES (it
     * must build to go RED at runtime under ASAN -- Rule 6). */
    (void)cap;
#endif

    while (ia < na || ib < nb) {
        int16_t xa = (ia < na) ? a[ia] : 0;
        int16_t xb = (ib < nb) ? b[ib] : 0;
        int16_t x;
        int take_a = 0, take_b = 0;
        if (ia < na && ib < nb) {
            if (xa < xb)       { x = xa; take_a = 1; }
            else if (xb < xa)  { x = xb; take_b = 1; }
            else               { x = xa; take_a = take_b = 1; } /* coincident */
        } else if (ia < na)    { x = xa; take_a = 1; }
        else                   { x = xb; take_b = 1; }

        if (take_a) { inA ^= 1; ia++; }
        if (take_b) { inB ^= 1; ib++; }

        int now = boolfn(op, inA, inB);
#ifdef RGN_MUTATE_EMIT_NOCHANGE
        /* MUTANT: emit at EVERY boundary, even when the output did not change.
         * This produces redundant toggles (e.g. coincident A/B edges emit a
         * zero-width span) -> rasterize diverges from the pixel ground truth. */
#ifndef RGN_MUTATE_NO_XMERGE_CAP
        if (no >= cap) { RGN_FAIL_LOUD(); return 0; } /* bead initech-mswo */
#endif
        out[no++] = x;
        prev_out = now;   /* keep prev_out coherent so the closing check holds */
#else
        if (now != prev_out) {
#ifndef RGN_MUTATE_NO_XMERGE_CAP
            if (no >= cap) { RGN_FAIL_LOUD(); return 0; } /* bead initech-mswo */
#endif
            out[no++] = x;
            prev_out = now;
        }
#endif
    }
    /* After the last boundary the output must be back OUTSIDE (even count). */
    if (prev_out != 0) { RGN_FAIL_LOUD(); return 0; }
    return no;
}

void region_op(region_t *out, const region_t *A, const region_t *B, rgn_op_t op)
{
    if (out == 0 || A == 0 || B == 0) { RGN_FAIL_LOUD(); return; }
    /* Guard: inputs must be in normal form before any merge (Rule 2). */
    region_assert_normal(A);
    region_assert_normal(B);

    out->n_rows = 0;
    out->x_pool_used = 0;
    out->is_rect = 0;
    out->is_empty = 1;

    /* Collect the sorted set of distinct band-top y values from A and B rows.
     * A band runs from one y to the next; in each band both regions' active
     * x-lists are fixed. We sweep these y values in order. */
    int A_empty = (A->is_empty || A->n_rows == 0);
    int B_empty = (B->is_empty || B->n_rows == 0);
    if (A_empty && B_empty) { region_set_empty(out); return; }

    uint16_t ia = 0, ib = 0;
    int16_t prev_y = 0;
    int have_prev = 0;
    /* Per-band merge scratch, sized to the PROVEN per-band bound (initech-44ab
     * RIDER 2; spec Sec 5 two-bounds note): RGN_ROW_X_MAX == 640 is exactly
     * the [0,640] OD-3 domain bound on a band merge's output -- xmerge
     * collapses coincident edges at emit, so out <= distinct x positions <=
     * 640 (see xmerge's banner + its _Static_assert). xmerge fail-louds (bead
     * initech-mswo) on anything more (out-of-domain/malformed inputs), so this
     * is never silently overrun. 1280 B of stack at the 640 cap -- audited
     * against the 64 KiB kernel stack (the working-set banner below). */
    int16_t scratch[RGN_ROW_X_MAX];

    for (;;) {
        int has_a = (ia < A->n_rows);
        int has_b = (ib < B->n_rows);
        if (!has_a && !has_b) break;

        int16_t ya = has_a ? A->rows[ia].y_top : 0;
        int16_t yb = has_b ? B->rows[ib].y_top : 0;
        int16_t y;
        if (has_a && has_b) {
            if (ya < yb)      { y = ya; ia++; }
            else if (yb < ya) { y = yb; ib++; }
            else              { y = ya; ia++; ib++; } /* coincident y */
        } else if (has_a)     { y = ya; ia++; }
        else                  { y = yb; ib++; }

        /* Skip duplicate y (coincident not fully consumed above can repeat). */
        if (have_prev && y == prev_y) continue;

        /* Emit the band that ENDS at y (i.e. starts at prev_y) for prev's state.
         * We compute the output x-list at scanline prev_y. */
        if (have_prev) {
            const int16_t *ax, *bx;
            uint16_t an = active_xlist(A, prev_y, &ax);
            uint16_t bn = active_xlist(B, prev_y, &bx);
            uint16_t no = xmerge(scratch, RGN_ROW_X_MAX, op, ax, an, bx, bn);
            region_push_row(out, prev_y, scratch, no);
        }
        prev_y = y;
        have_prev = 1;
    }

    /* The final boundary y closes the last band. Append a closing row at the
     * last y so the lowest live band has a bottom. After this band both regions
     * are exhausted (their last rows are empty closers), so the output there is
     * empty -- the natural closing row. */
    if (have_prev) {
        const int16_t *ax, *bx;
        uint16_t an = active_xlist(A, prev_y, &ax);
        uint16_t bn = active_xlist(B, prev_y, &bx);
        uint16_t no = xmerge(scratch, RGN_ROW_X_MAX, op, ax, an, bx, bn);
        region_push_row(out, prev_y, scratch, no);
    }

    region_normalize(out);
}

void region_complement(region_t *out, const region_t *A, rgn_rect_t frame)
{
    if (out == 0 || A == 0) { RGN_FAIL_LOUD(); return; }
    /* complement(A, frame) := frame XOR A, over the frame (spec Sec 3). We need
     * a region for `frame`; build it in a tiny local store. The frame fits one
     * rect (2 rows, 2 x-entries). */
    region_t fr;
    rgn_row_t fr_rows[2];
    int16_t   fr_pool[2];
    rgn_zero(&fr, sizeof fr);
    fr.rows = fr_rows;
    fr.cap_rows = 2;
    fr.x_pool = fr_pool;
    fr.x_pool_cap = 2;
    region_set_rect(&fr, frame);
    region_op(out, &fr, A, RGN_OP_XOR);
}

/* ===========================================================================
 * THE WORKING SET (DUAL-MODE) + the fail-loud in-use guard
 * (bead initech-44ab RIDER 3 + ROUND 3, committee 2026-07-11).
 *
 * Pre-44ab, region_from_rects carved THREE full working regions (acc/one/tmp:
 * 3 x region_t + 3 x rows[RGN_ROWS_CAP] + 3 x pool[RGN_X_POOL_CAP]) from its
 * stack frame, and each query helper (region_rect_fully_in / _overlaps /
 * region_intersects) carved one more. The initech-44ab kernel-stack audit
 * showed that at the post-44ab caps (RGN_ROW_X_MAX=640, RGN_X_POOL_CAP=2048)
 * this breaks the committee's stack ceilings, so the working regions moved
 * OFF the stack into a file-scope working set behind a fail-loud in-use guard
 * (a nested acquire is a reentrancy bug -- cooperative FLAIR has exactly ONE
 * region call in flight at a time, and the ISRs never call the engine; Rule 2).
 *
 * WHY DUAL-MODE (ROUND 3): a plain static value array (round 2) cost 18.4 KiB
 * of kernel .bss and busted the LARGEST kernel variant's link -- kernel_shell
 * (COMMAND.COM + full FLAIR) exceeded the _kernel_end < PROGRAM_BASE (0x40000)
 * ld ASSERT by 8340 B (the guard did its job). The engine stays HEAP-AGNOSTIC:
 *   HOSTED (__STDC_HOSTED__): the working set is a static value array (the C
 *     runtime zeroes BSS; the property suite needs no init call).
 *   FREESTANDING: g_rgn_ws holds BOUND POINTERS (8 B of .bss instead of
 *     18.4 KiB); kmain.c allocates the two backing slots from the FLAIR heap
 *     (FLAIR_CLASS_REGION, ADR-0004 DEC-03 -- the flair_desktop_alloc_region
 *     idiom; allocation failure -> flair_desktop_oom, boot-time fail-loud) and
 *     binds them via region_engine_bind_ws(). BOOT ORDER IS LOAD-BEARING:
 *       flair_heap_init -> region_engine_bind_ws -> region_engine_reset ->
 *       shell_build_scene (the first Toolbox region call).
 *     rgn_ws_acquire fail-louds on unbound/NULL slots (tweak W1) -- a naive
 *     pointer swap would otherwise crash silently pre-init.
 * The slot shape (rgn_ws_slot_t) is exposed in region.h (tweak W3) so kmain.c
 * sizes the heap blocks with sizeof, never hand-computed byte math.
 *
 * SLOT MAP (tweak W2): TWO full-cap slots only. from_rects' acc -> slot 0,
 * tmp -> slot 1 (both ACCUMULATE, so they must be full-size); its single-rect
 * scratch 'one' needs just 2 rows + 2 x-slots and lives on the stack (the
 * region_complement pattern -- ~24 B). The query helpers' result region shares
 * slot 0 with acc (they never run concurrently -- the guard proves it).
 *
 * KERNEL STACK AUDIT (initech-44ab RIDER 3; -m32 sizes: rgn_row_t = 8 B,
 * region_t = 32 B, int16 = 2 B; caps RGN_ROWS_CAP=256, RGN_X_POOL_CAP=2048,
 * RGN_ROW_X_MAX=640). The kernel stack is kstart.asm's esp = 0x0009FFFC,
 * region [0x90000, 0xA0000) = 64 KiB, ONE stack shared with the IRQ handlers.
 * Cap-sized stack arrays, POST-44ab:
 *   region_op        scratch[RGN_ROW_X_MAX] = 1280 B  (stack; small -- stays)
 *   region_normalize tmp[RGN_ROW_X_MAX]     = 1280 B  (stack; small -- stays)
 *   region_from_rects                       = ~150 B  (WAS 18528 B = 18.1 KiB
 *                                             at the new caps -- the single
 *                                             > 8 KiB/frame offender; acc/tmp
 *                                             now in the working set, 'one'
 *                                             is the 2/2 stack pattern)
 *   region_rect_fully_in/_overlaps/
 *     region_intersects                     = ~90 B   (WAS ~6228 B each at the
 *                                             new caps; currently DEAD CODE in
 *                                             the shipped kernel -- no os/
 *                                             caller -- converted anyway for
 *                                             consistency)
 *   region_complement                       = ~52 B   (never cap-sized)
 * Deepest region-engine chain (shell.c/window.c -> region_op ->
 * region_normalize): 1280 + 1280 + small = ~2.6 KiB, well inside the
 * committee's 16 KiB region-engine ceiling. Working-set footprint: 2 x
 * sizeof(rgn_ws_slot_t) = 2 x 6144 = 12288 B -- FLAIR heap in the kernel,
 * hosted .bss in the suite.
 *
 * BSS WARNING (tweak T3): kstart.asm does NOT zero .bss, so g_rgn_ws_in_use
 * (and, freestanding, the slot pointers) CANNOT be assumed 0 at boot. kmain.c
 * binds then calls region_engine_reset() before the first Toolbox region call
 * (the flair_heap_init explicit-init discipline); without it a garbage flag
 * would false-panic the very first boot-time region_from_rects. Hosted test
 * binaries get zeroed BSS from the C runtime.
 *
 * GUARD DISCIPLINE (tweak T1): acquire ONLY immediately before the first touch
 * of the working set -- every argument-validation / bbox-reject early return
 * happens BEFORE the acquire -- and release on the ONE exit path of the
 * guarded section, so no return can leak the flag.
 * ===========================================================================*/

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
/* HOSTED: static value slots, statically bound (round 3: "g_rgn_ws stays a
 * static value array; no hosted test changes"). */
static rgn_ws_slot_t  g_rgn_ws_store[2];
static rgn_ws_slot_t *g_rgn_ws[2] = { &g_rgn_ws_store[0], &g_rgn_ws_store[1] };
#else
/* FREESTANDING: bound pointers only -- the backing lives on the FLAIR heap
 * (kmain.c allocates + binds; see the banner). Unbound until
 * region_engine_bind_ws(); rgn_ws_acquire fail-louds on NULL (W1). */
static rgn_ws_slot_t *g_rgn_ws[2];
#endif
static uint8_t g_rgn_ws_in_use;        /* NOT trusted at boot -- see T3 above  */

static void rgn_ws_acquire(void)
{
    /* W1: unbound slots are a boot-order bug (bind must precede the first
     * Toolbox region call) -- fail loud, never a silent wild-pointer write.
     * Statically true in the hosted build; load-bearing freestanding. */
    if (g_rgn_ws[0] == 0 || g_rgn_ws[1] == 0) { RGN_FAIL_LOUD(); return; }
    if (g_rgn_ws_in_use) { RGN_FAIL_LOUD(); return; }  /* nested acquire */
    g_rgn_ws_in_use = 1;
}

static void rgn_ws_release(void)
{
    g_rgn_ws_in_use = 0;
}

/* Attach region header `r` to working-set slot `slot` (canonical empty). */
static void rgn_ws_attach(region_t *r, rgn_ws_slot_t *slot)
{
    rgn_zero(r, sizeof *r);
    r->rows       = slot->rows;
    r->cap_rows   = RGN_ROWS_CAP;
    r->x_pool     = slot->pool;
    r->x_pool_cap = RGN_X_POOL_CAP;
    region_set_empty(r);
}

#if !defined(__STDC_HOSTED__) || !__STDC_HOSTED__
/* FREESTANDING ONLY (round 3): bind the two FLAIR-heap-backed slots. Distinct,
 * non-NULL; called by kmain.c strictly BEFORE region_engine_reset() and the
 * first Toolbox region call. Not compiled hosted (the suite's slots are
 * statically bound). */
void region_engine_bind_ws(rgn_ws_slot_t *slot_a, rgn_ws_slot_t *slot_b)
{
    if (slot_a == 0 || slot_b == 0 || slot_a == slot_b) { RGN_FAIL_LOUD(); return; }
    g_rgn_ws[0] = slot_a;
    g_rgn_ws[1] = slot_b;
}
#endif

/* Explicit engine init (spec Sec 6; tweaks T3 + round 3): clear the working-
 * set guard ONLY -- allocation/binding is deliberately NOT folded in (the
 * committee kept this signature universal across both modes). MUST run once
 * at kernel startup, AFTER region_engine_bind_ws and BEFORE the first Toolbox
 * region call, because kstart.asm does not zero .bss. */
void region_engine_reset(void)
{
    g_rgn_ws_in_use = 0;
}

/* ===========================================================================
 * region_from_rects -- union of n rects (repeated union; normalized).
 * ===========================================================================*/
void region_from_rects(region_t *r, const rgn_rect_t *rects, uint16_t n)
{
    if (r == 0) { RGN_FAIL_LOUD(); return; }
    region_set_empty(r);
    if (n == 0 || rects == 0) return;   /* pre-acquire: cannot leak the guard */

    /* Accumulate into r via the working set (initech-44ab RIDER 3 + round 3:
     * the old stack triple was 18.1 KiB at the post-44ab caps -- see the audit
     * banner above). acc (slot 0) holds the running union; tmp (slot 1) the
     * step result -- both accumulate, so both are full-cap slots. one holds
     * ONE rect: 2 rows + 2 x-slots on the stack (the region_complement
     * pattern; tweak W2). Acquire HERE (first touch), release at the ONE exit. */
    rgn_ws_acquire();
    region_t acc, one, tmp;             /* 32 B headers; big storage is bound */
    rgn_row_t one_rows[2];
    int16_t   one_pool[2];
    rgn_ws_attach(&acc, g_rgn_ws[0]);
    rgn_ws_attach(&tmp, g_rgn_ws[1]);
    rgn_zero(&one, sizeof one);
    one.rows = one_rows; one.cap_rows = 2;
    one.x_pool = one_pool; one.x_pool_cap = 2;
    region_set_empty(&one);

    for (uint16_t i = 0; i < n; i++) {
        region_set_rect(&one, rects[i]);
        region_op(&tmp, &acc, &one, RGN_OP_UNION);
        /* copy tmp -> acc */
        region_op(&acc, &tmp, &tmp, RGN_OP_UNION); /* self-union == identity copy */
    }
    /* copy acc -> r */
    region_op(r, &acc, &acc, RGN_OP_UNION);
    rgn_ws_release();
}

/* ===========================================================================
 * Queries.
 * ===========================================================================*/
rgn_rect_t region_get_bbox(const region_t *r) { return r->bbox; }

int region_is_empty(const region_t *r)
{
    return (r->is_empty || r->n_rows == 0) ? 1 : 0;
}

int region_contains_point(const region_t *r, int16_t h, int16_t v)
{
    if (r == 0 || r->is_empty || r->n_rows == 0) return 0;
    /* bbox reject */
    if (v < r->bbox.top || v >= r->bbox.bottom) return 0;
    if (h < r->bbox.left || h >= r->bbox.right) return 0;
    /* find the row covering scanline v: greatest y_top <= v */
    const rgn_row_t *found = 0;
    for (uint16_t i = 0; i < r->n_rows; i++) {
        if (r->rows[i].y_top <= v) found = &r->rows[i];
        else break;
    }
    if (found == 0 || found->x_count == 0) return 0;
    /* parity scan: inside iff h in [x[2k], x[2k+1)) for some k */
    int inside = 0;
    for (uint16_t k = 0; k < found->x_count; k++) {
#ifdef RGN_MUTATE_PARITY_OFF1
        if (h <= found->x[k]) break;   /* MUTANT: <= instead of < -> off-by-one */
#else
        if (h < found->x[k]) break;
#endif
        inside ^= 1;
    }
    return inside;
}

int region_equal(const region_t *A, const region_t *B)
{
    if (A == 0 || B == 0) return 0;
    int ae = (A->is_empty || A->n_rows == 0);
    int be = (B->is_empty || B->n_rows == 0);
    if (ae || be) return ae && be;
    /* normal form is canonical -> structural compare. */
    if (A->n_rows != B->n_rows) return 0;
    if (A->bbox.top != B->bbox.top || A->bbox.left != B->bbox.left ||
        A->bbox.bottom != B->bbox.bottom || A->bbox.right != B->bbox.right) return 0;
    for (uint16_t i = 0; i < A->n_rows; i++) {
        if (A->rows[i].y_top != B->rows[i].y_top) return 0;
        if (!xlist_equal(A->rows[i].x, A->rows[i].x_count,
                         B->rows[i].x, B->rows[i].x_count)) return 0;
    }
    return 1;
}

/* CONTAINMENT: 1 iff every pixel of `rect` is in `r` (rect DIFF r == empty).
 * Renamed from region_rect_in_region per ADR-0005 Amendment AM-4; retained for
 * window-clip contexts that genuinely want containment. NOT the RectInRgn/
 * RectInRegion semantic -- those are OVERLAP (region_rect_overlaps). */
int region_rect_fully_in(const region_t *r, rgn_rect_t rect)
{
    if (rect_is_empty(rect)) return 1;          /* empty rect is trivially in */
    /* bbox reject */
    if (rect.left < r->bbox.left || rect.right > r->bbox.right ||
        rect.top < r->bbox.top || rect.bottom > r->bbox.bottom) return 0;
    /* rect DIFF r == empty  <=>  every pixel of rect is in r. The DIFF result
     * needs full caps -> static slot 0 (initech-44ab RIDER 3; was ~6.1 KiB of
     * stack at the new caps -- dead code in the shipped kernel today, T4 --
     * converted for consistency). Guard acquired at first static touch, ONE
     * guarded exit (T1). The 2-slot rect region stays on the stack (24 B). */
    region_t rr, df;
    rgn_row_t rr_rows[2];
    int16_t   rr_pool[2];
    rgn_zero(&rr, sizeof rr); rr.rows = rr_rows; rr.cap_rows = 2; rr.x_pool = rr_pool; rr.x_pool_cap = 2;
    rgn_ws_acquire();
    rgn_ws_attach(&df, g_rgn_ws[0]);
    region_set_rect(&rr, rect);
    region_op(&df, &rr, r, RGN_OP_DIFF);
    int ret = region_is_empty(&df);
    rgn_ws_release();
    return ret;
}

/* OVERLAP: 1 iff `rect` and `r` share at least one pixel (the documented
 * RectInRgn/RectInRegion semantic of BOTH heritages -- ADR-0005 Amendment AM-4 /
 * C-9; "any overlap" / "at least partially inside"). bbox-reject then build a
 * rect region and INTERSECT-non-empty. An empty rect overlaps nothing. */
int region_rect_overlaps(const region_t *r, rgn_rect_t rect)
{
    if (rect_is_empty(rect)) return 0;          /* empty rect overlaps nothing */
    if (r == 0 || region_is_empty(r)) return 0;
    /* bbox reject (rect vs r->bbox, half-open) */
    if (rect.right <= r->bbox.left || r->bbox.right <= rect.left ||
        rect.bottom <= r->bbox.top || r->bbox.bottom <= rect.top) return 0;
    /* rect INTERSECT r non-empty <=> they share a pixel. Static slot 0 for the
     * INTERSECT result (initech-44ab RIDER 3; dead code in the shipped kernel
     * today, T4). Acquire at first static touch, ONE guarded exit (T1). */
    region_t rr, in;
    rgn_row_t rr_rows[2];
    int16_t   rr_pool[2];
    rgn_zero(&rr, sizeof rr); rr.rows = rr_rows; rr.cap_rows = 2; rr.x_pool = rr_pool; rr.x_pool_cap = 2;
    rgn_ws_acquire();
    rgn_ws_attach(&in, g_rgn_ws[0]);
    region_set_rect(&rr, rect);
    region_op(&in, &rr, r, RGN_OP_INTERSECT);
    int ret = region_is_empty(&in) ? 0 : 1;
    rgn_ws_release();
    return ret;
}

int region_intersects(const region_t *A, const region_t *B)
{
    if (region_is_empty(A) || region_is_empty(B)) return 0;
    /* bbox reject */
    if (A->bbox.right <= B->bbox.left || B->bbox.right <= A->bbox.left ||
        A->bbox.bottom <= B->bbox.top || B->bbox.bottom <= A->bbox.top) return 0;
    /* Static slot 0 for the INTERSECT result (initech-44ab RIDER 3; dead code
     * in the shipped kernel today, T4). Acquire at first static touch, ONE
     * guarded exit (T1) -- the bbox rejects above are pre-acquire. */
    region_t df;
    rgn_ws_acquire();
    rgn_ws_attach(&df, g_rgn_ws[0]);
    region_op(&df, A, B, RGN_OP_INTERSECT);
    int ret = region_is_empty(&df) ? 0 : 1;
    rgn_ws_release();
    return ret;
}

/* ===========================================================================
 * 7. VERBATIM QUICKDRAW WRAPPERS (spec Sec 7) -- thin shims over region_op.
 * ===========================================================================*/
void UnionRgn(const region_t *srcA, const region_t *srcB, region_t *dstRgn)
{ region_op(dstRgn, srcA, srcB, RGN_OP_UNION); }
void SectRgn(const region_t *srcA, const region_t *srcB, region_t *dstRgn)
{ region_op(dstRgn, srcA, srcB, RGN_OP_INTERSECT); }
void DiffRgn(const region_t *srcA, const region_t *srcB, region_t *dstRgn)
{ region_op(dstRgn, srcA, srcB, RGN_OP_DIFF); }
void XorRgn(const region_t *srcA, const region_t *srcB, region_t *dstRgn)
{ region_op(dstRgn, srcA, srcB, RGN_OP_XOR); }

void SetRectRgn(region_t *rgn, int16_t left, int16_t top, int16_t right, int16_t bottom)
{ rgn_rect_t rc = { top, left, bottom, right }; region_set_rect(rgn, rc); }
void SetEmptyRgn(region_t *rgn) { region_set_empty(rgn); }

int PtInRgn(int16_t h, int16_t v, const region_t *rgn)
{ return region_contains_point(rgn, h, v); }
/* RectInRgn -> OVERLAP (ADR-0005 Amendment AM-4 / C-9). Was wrongly containment
 * (region_rect_in_region) pre-AM-4; both heritages document "any overlap". */
int RectInRgn(rgn_rect_t rect, const region_t *rgn)
{ return region_rect_overlaps(rgn, rect); }
int EmptyRgn(const region_t *rgn) { return region_is_empty(rgn); }
int EqualRgn(const region_t *rgnA, const region_t *rgnB) { return region_equal(rgnA, rgnB); }

/* ===========================================================================
 * 7b. GDI/HRGN HERITAGE FACADE (spec Sec 7b) -- thin shims over region_op, a
 * strict PEER of the QuickDraw facade above. ZERO new region math, ZERO second
 * engine: every GDI combine bottoms out in the ONE region_op (constraint C-7,
 * ADR-0005 Amendment AM-1/AM-2). The region-type classifier reads the is_empty/
 * is_rect flags region_normalize already maintains -- it does not recompute
 * geometry.
 * ===========================================================================*/

/* Classify a NORMALIZED region into its GDI region-type code (spec Sec 7b):
 * empty -> NULLREGION, single rect -> SIMPLEREGION, else COMPLEXREGION.
 * RGN_TYPE_ERROR is reserved for invalid args (handled by the callers). */
static int region_type_classify(const region_t *r)
{
    if (region_is_empty(r)) return NULLREGION;
    if (r->is_rect)         return SIMPLEREGION;
    return COMPLEXREGION;
}

int CombineRgn(region_t *dst, const region_t *src1, const region_t *src2, int mode)
{
    if (dst == 0 || src1 == 0) return RGN_TYPE_ERROR;
    if (mode == RGN_COPY) {
        /* copy of src1, normalized -- the self-union identity copy idiom. */
        region_op(dst, src1, src1, RGN_OP_UNION);
        return region_type_classify(dst);
    }
    if (mode != RGN_AND && mode != RGN_OR &&
        mode != RGN_XOR && mode != RGN_DIFF) return RGN_TYPE_ERROR;
    if (src2 == 0) return RGN_TYPE_ERROR;
#ifdef GDI_MUTATE_DISPATCH
    /* MUTANT (ADR-0005 Amendment AM-1 Sec 7.4, M-DISPATCH): force ONE mode to
     * the WRONG op at RUNTIME, bypassing the compile-checked RGN_OP_FROM_MODE
     * map. RGN_OR is wired to INTERSECT here. This must drive BOTH the L1
     * cross-family equality leg (CombineRgn(...,RGN_OR) vs UnionRgn) AND the L2
     * wine differential (CombineRgn RGN_OR vs gdi_ref_union_region) RED --
     * proving the mode->op SEMANTICS are graded, not just the plumbing. */
    {
        rgn_op_t op = (mode == RGN_OR) ? RGN_OP_INTERSECT
                                       : rgn_op_from_combine_mode(mode);
        region_op(dst, src1, src2, op);
    }
#else
    region_op(dst, src1, src2, rgn_op_from_combine_mode(mode));
#endif
    return region_type_classify(dst);
}

int GetRgnBox(const region_t *rgn, rgn_rect_t *out)
{
    if (rgn == 0 || out == 0) return RGN_TYPE_ERROR;
    *out = region_get_bbox(rgn);
    return region_type_classify(rgn);
}

int PtInRegion(const region_t *rgn, int16_t x, int16_t y)
{ return region_contains_point(rgn, x, y); }

int RectInRegion(const region_t *rgn, rgn_rect_t rect)
{
#ifdef GDI_MUTATE_RECTIN
    /* MUTANT (ADR-0005 Amendment AM-1 Sec 7.4, M-RECTIN): wire RectInRegion to
     * CONTAINMENT (region_rect_fully_in) instead of OVERLAP -- re-introducing
     * the pre-AM-4 deep bug. The RectInRegion-vs-wine-rect_in_region row of L2
     * must go RED, proving the overlap value is graded against the EXTERNAL wine
     * authority ("at least partially inside"), NOT by construction. */
    return region_rect_fully_in(rgn, rect);
#else
    return region_rect_overlaps(rgn, rect);
#endif
}

/* ===========================================================================
 * 9. HOSTED-ONLY TEST HOOK (factory-facing; NEVER compiled freestanding)
 * ===========================================================================*/
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
/* Rule-6 probe: prove the working-set in-use guard BITES. Acquires the guard
 * as if an engine call were in flight, then calls region_from_rects -- whose
 * nested rgn_ws_acquire MUST fail loud (hosted: abort()). On a correct engine
 * this NEVER RETURNS; returning at all means the guard is decoration.
 * test_region.c calls this from a fork()ed CHILD and asserts the child dies by
 * SIGABRT (the over-cap probe idiom) -- and, per the committee's verification
 * round, the parent NEVER forks while itself inside a guarded call. */
void region_engine_probe_nested_acquire(region_t *scratch)
{
    rgn_ws_acquire();
    rgn_rect_t rc = { 0, 0, 1, 1 };                 /* top,left,bottom,right */
    region_from_rects(scratch, &rc, 1);             /* nested acquire -> abort */
}
#endif
