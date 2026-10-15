/*
 * harness/proptest/test_iword_core.c -- THE PROPERTY SUITE of InitechWord's
 * document core (factory C; bead initech-l4tj, IWORD P1; plan
 * docs/plans/INITECH-WORD-plan.md Sec 6(c)).
 *
 * The core (os/iword/core) is graded against MODELS written here, never
 * against itself:
 *   P1 PIECE TABLE  random insert / delete / overwrite sequences applied to
 *                   the piece table AND to a flat byte array; after every
 *                   step the document reads back exactly as the array (all
 *                   bytes, the length, every single byte, the iterator from a
 *                   random start). A failing sequence is SHRUNK (one-op
 *                   deletion to a fixpoint) and printed.
 *   P2 REFLOW       random paragraphs (word runs, space runs, hard returns,
 *                   over-long words) wrapped at a random width: (a) the
 *                   result equals an independent reference wrapper written
 *                   below from the rule stated in iword.h / wp5_format.h
 *                   (the most words that fit; the ending space becomes the
 *                   SRt; MINT01 @0x188); (b) reflow is IDEMPOTENT; (c) the
 *                   logical text (SRt read as a space) is unchanged; (d) no
 *                   layout line is wider than the width.
 *   P3 TYPING       random keystroke sessions on the editor (characters,
 *                   spaces, Enter, Backspace, Delete, arrows, Home/End): the
 *                   logical text equals a flat model edited at a model cursor
 *                   by the same keys, the bytes equal the reference wrap of
 *                   that text, and the CURSOR invariants hold after every key
 *                   (a unit boundary in [0, len]; where() -> pos_at() round-
 *                   trips; Up/Down move exactly one line, Left/Right one
 *                   unit). Failing sessions are shrunk and printed.
 *   P4 OPAQUE CODES documents holding fixed and variable groups (the C3/C4
 *                   attribute pairs and a D0 group, wp5-body-codes.md Sec 3/4):
 *                   the unit walk is the same forwards and backwards, codes
 *                   are zero-width, typing around them never splits one,
 *                   and Backspace / Delete leave them intact (P1 protects
 *                   codes; deleting them is P2, bead initech-jojk).
 *   P5 STATUS       iw_fmt_inches / Ln / Pos against the readings of the real
 *                   WordPerfect 5.1 screen: "Ln 1"" and "Pos 1"" on an empty
 *                   document, "Ln 1.17"" "Pos 2.6"" after "Plain text only."
 *                   on line 2 (MINT02.shots/02_typed.png), "Ln 2"" on line 7
 *                   (MINT01.shots/02_typed.png).
 *
 * Mutants (Rule 6; test-iword-core-mutant, each RED for its named reason):
 *   IWORD_MUT_SPLIT_OFF     a split piece's right half starts one byte late
 *                           -> RED-REASON model
 *   IWORD_MUT_REFLOW_STALE  reflow keeps the old soft returns -> RED-REASON reflow
 *   IWORD_MUT_PREV_VAR      the backward walk steps into a variable group
 *                           -> RED-REASON units
 *
 * Deterministic (fixed seeds, Rule 11). ASCII-clean (Rule 12).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"
#include "iword.h"
#include "iword/wp5_format.h"

TEST_HARNESS();

static int g_model_fail, g_reflow_fail, g_typing_fail, g_units_fail, g_status_fail;

/* ---- deterministic PRNG (xorshift32) ------------------------------------- */
static uint32_t g_rng;
static uint32_t rnd(void) { g_rng ^= g_rng << 13; g_rng ^= g_rng >> 17; g_rng ^= g_rng << 5; return g_rng; }
static uint32_t rn(uint32_t n) { return n ? rnd() % n : 0; }

#define CAP     (64u * 1024u)
#define PCS     4096u
static uint8_t    g_add[CAP];
static iw_piece_t g_pc[PCS];
static uint8_t    g_orig[CAP];
static uint8_t    g_buf[CAP], g_buf2[CAP];

static void doc_fresh(iw_doc_t *d, const uint8_t *orig, uint32_t n)
{
    iw_doc_init(d, g_add, CAP, g_pc, PCS, CAP);
    iw_doc_load(d, orig, n);
}

/* ======================================================================== *
 * P1: the piece table against a flat array
 * ======================================================================== */
enum { OP_INS, OP_DEL, OP_SET };
typedef struct { uint8_t kind; uint32_t a, b; uint8_t s[24]; } op_t;
#define MAXOPS 200
static op_t g_ops[MAXOPS];

/* Run ops[0..n) with `skip` removed (skip[i] != 0); return the first failing
 * step (1-based) or 0. The ops' positions are taken MODULO the current
 * length, so any subsequence is still a valid program (that is what lets
 * the shrinker delete ops freely). */
static uint8_t g_model[CAP];
static int run_p1(const uint8_t *orig, uint32_t olen, const op_t *ops, int n,
                  const uint8_t *skip, int verbose)
{
    iw_doc_t d;
    uint32_t mlen = olen;
    int step = 0;
    memcpy(g_model, orig, olen);
    doc_fresh(&d, orig, olen);
    for (int i = 0; i < n; i++) {
        const op_t *o = &ops[i];
        if (skip && skip[i]) continue;
        step++;
        if (o->kind == OP_INS) {
            uint32_t pos = o->a % (mlen + 1u), k = o->b;
            if (mlen + k > CAP) continue;
            if (verbose) printf("    ins @%u \"%.*s\"\n", pos, (int)k, (const char *)o->s);
            if (iw_doc_insert(&d, pos, o->s, k) != IW_OK) return step;
            memmove(g_model + pos + k, g_model + pos, mlen - pos);
            memcpy(g_model + pos, o->s, k);
            mlen += k;
        } else if (o->kind == OP_DEL) {
            if (mlen == 0u) continue;
            uint32_t pos = o->a % mlen, k = 1u + o->b % (mlen - pos);
            if (verbose) printf("    del @%u n=%u\n", pos, k);
            if (iw_doc_delete(&d, pos, k) != IW_OK) return step;
            memmove(g_model + pos, g_model + pos + k, mlen - pos - k);
            mlen -= k;
        } else {
            if (mlen == 0u) continue;
            uint32_t pos = o->a % mlen;
            if (verbose) printf("    set @%u '%c'\n", pos, o->s[0]);
            if (iw_doc_set(&d, pos, o->s[0]) != IW_OK) return step;
            g_model[pos] = o->s[0];
        }
        if (d.len != mlen) return step;
        if (iw_doc_read(&d, 0, g_buf, CAP) != mlen || memcmp(g_buf, g_model, mlen) != 0) return step;
        if (mlen) {
            uint32_t p = rn(mlen);
            if (iw_doc_byte(&d, p) != (int)g_model[p]) return step;
            iw_iter_t it;
            iw_iter_at(&it, &d, p);
            for (uint32_t q = p; q < mlen; q++) if (iw_iter_next(&it) != (int)g_model[q]) return step;
            if (iw_iter_next(&it) != -1) return step;
        }
        if (iw_doc_byte(&d, mlen) != -1) return step;
    }
    return 0;
}

static void gen_ops(int n)
{
    for (int i = 0; i < n; i++) {
        op_t *o = &g_ops[i];
        uint32_t r = rn(10);
        o->kind = r < 5 ? OP_INS : r < 8 ? OP_DEL : OP_SET;
        o->a = rnd();
        o->b = o->kind == OP_INS ? 1u + rn(sizeof o->s) : rn(40);
        for (uint32_t k = 0; k < sizeof o->s; k++) o->s[k] = (uint8_t)('a' + rn(26));
    }
}

/* Delete ops one at a time while the program still fails: a 1-minimal
 * failing subsequence (shrinking, the region-suite discipline). */
static int shrink(const uint8_t *orig, uint32_t olen, int n, uint8_t *skip)
{
    int changed = 1, live = n;
    while (changed) {
        changed = 0;
        for (int i = 0; i < n; i++) {
            if (skip[i]) continue;
            skip[i] = 1;
            if (run_p1(orig, olen, g_ops, n, skip, 0)) { changed = 1; live--; }
            else skip[i] = 0;
        }
    }
    return live;
}

static void p1_piece_table(void)
{
    int trials = 300, failed = 0;
    g_rng = 0x1F0Du;
    for (int t = 0; t < trials && !failed; t++) {
        uint32_t olen = rn(200);
        int n = 1 + (int)rn(MAXOPS);
        for (uint32_t i = 0; i < olen; i++) g_orig[i] = (uint8_t)('A' + rn(26));
        gen_ops(n);
        int at = run_p1(g_orig, olen, g_ops, n, 0, 0);
        if (at) {
            static uint8_t skip[MAXOPS];
            memset(skip, 0, sizeof skip);
            int live = shrink(g_orig, olen, n, skip);
            fprintf(stderr, "  FAIL P1 trial %d: the piece table disagrees with the flat model; "
                    "shrunk to %d op(s) on a %u-byte original:\n", t, live, olen);
            (void)run_p1(g_orig, olen, g_ops, n, skip, 1);
            failed = 1;
        }
    }
    CHECK(!failed, "P1 piece table == flat model over random edit programs");
    if (failed) g_model_fail++;
    /* A long session in one table: thousands of small edits. */
    {
        iw_doc_t d;
        uint32_t mlen = 0;
        int bad = 0;
        g_rng = 0xBEEFu;
        doc_fresh(&d, g_orig, 0);
        for (int i = 0; i < 3000 && !bad; i++) {
            uint32_t pos = rn(mlen + 1u);
            uint8_t ch = (uint8_t)('a' + rn(26));
            if (rn(4) == 0 && mlen) {
                pos = rn(mlen);
                if (iw_doc_delete(&d, pos, 1) != IW_OK) { bad = 1; break; }
                memmove(g_model + pos, g_model + pos + 1, mlen - pos - 1); mlen--;
            } else {
                if (iw_doc_insert(&d, pos, &ch, 1) != IW_OK) { bad = 1; break; }
                memmove(g_model + pos + 1, g_model + pos, mlen - pos); g_model[pos] = ch; mlen++;
            }
            if (iw_doc_read(&d, 0, g_buf, CAP) != mlen || memcmp(g_buf, g_model, mlen)) bad = 1;
        }
        CHECK(!bad, "P1 a 3000-keystroke session in one table == the flat model");
        if (bad) g_model_fail++;
    }
    /* Refusals: past the end, over the max length, out of pieces. */
    {
        iw_doc_t d;
        static iw_piece_t few[4];
        static uint8_t small_add[8];
        uint8_t x = 'x';
        iw_doc_init(&d, g_add, CAP, g_pc, PCS, 4);
        iw_doc_load(&d, (const uint8_t *)"abc", 3);
        CHECK(iw_doc_insert(&d, 4, &x, 1) == IW_E_RANGE, "insert past the end refused");
        CHECK(iw_doc_delete(&d, 2, 2) == IW_E_RANGE, "delete past the end refused");
        CHECK(iw_doc_insert(&d, 3, &x, 1) == IW_OK, "insert up to max_len");
        CHECK(iw_doc_insert(&d, 0, &x, 1) == IW_E_FULL, "insert beyond max_len refused");
        iw_doc_init(&d, small_add, sizeof small_add, few, 4, CAP);
        iw_doc_load(&d, (const uint8_t *)"abcdef", 6);
        int rc = IW_OK, k;
        for (k = 0; k < 8 && rc == IW_OK; k++) rc = iw_doc_insert(&d, (uint32_t)(1 + k * 2) % (d.len + 1u), &x, 1);
        CHECK(rc == IW_E_FULL, "running out of pieces is refused, not overrun");
        CHECK(iw_doc_read(&d, 0, g_buf, CAP) == d.len, "the refused table still reads whole");
    }
    printf("  P1 %s: piece table == flat model (300 random programs, 3000-key session, refusals)\n", g_model_fail ? "RED" : "green");
}

/* ======================================================================== *
 * P2: reflow against an independent reference wrapper
 * ======================================================================== */
/* THE REFERENCE RULE, restated from first principles for plain text (bytes
 * 0x20..0x7E, 0x0A, 0x0D): split into paragraphs at 0x0A; in each, read
 * 0x0D as a space; then, from the line start s, if the rest of the
 * paragraph (e - s) fits in w columns the paragraph ends there; else the
 * break is the LAST space b with s <= b and b - s <= w (the text before it
 * fits); that space becomes 0x0D and the next line starts at b + 1. With no
 * such space, the line runs on (the layout breaks it; no byte changes) to
 * the next space after s + w, which then breaks it. */
static void ref_wrap(uint8_t *b, uint32_t n, uint32_t w)
{
    uint32_t p = 0;
    for (uint32_t i = 0; i < n; i++) if (b[i] == 0x0D) b[i] = ' ';
    while (p <= n) {
        uint32_t e = p;
        while (e < n && b[e] != 0x0A) e++;
        uint32_t s = p;
        while (e - s > w) {
            uint32_t brk = UINT32_MAX;
            for (uint32_t k = s; k <= s + w && k < e; k++) if (b[k] == ' ') brk = k;
            if (brk == UINT32_MAX) {
                uint32_t k = s + w + 1u;
                while (k < e && b[k] != ' ') k++;
                if (k >= e) break;
                brk = k;
            }
            b[brk] = 0x0D;
            s = brk + 1u;
        }
        p = e + 1u;
    }
}

static uint32_t gen_para_text(uint8_t *b, uint32_t cap)
{
    uint32_t n = 0, words = rn(60);
    for (uint32_t i = 0; i < words && n + 100 < cap; i++) {
        uint32_t wl = rn(10) == 0 ? 1u + rn(90) : 1u + rn(12);
        for (uint32_t k = 0; k < wl; k++) b[n++] = (uint8_t)('a' + rn(26));
        uint32_t r = rn(12);
        if (r == 0) b[n++] = 0x0A;
        else if (r == 1) { b[n++] = ' '; b[n++] = ' '; }
        else if (r == 2) b[n++] = 0x0D;        /* a stale soft return */
        else b[n++] = ' ';
    }
    return n;
}

static void p2_reflow(void)
{
    int bad = 0;
    g_rng = 0x5EEDu;
    for (int t = 0; t < 2000 && !bad; t++) {
        uint32_t n = gen_para_text(g_orig, CAP), w = 5u + rn(70);
        iw_doc_t d;
        memcpy(g_buf2, g_orig, n);
        ref_wrap(g_buf2, n, w);
        doc_fresh(&d, g_orig, n);
        if (iw_reflow_all(&d, w) != IW_OK) { fprintf(stderr, "  FAIL P2 trial %d: reflow refused\n", t); bad = 1; break; }
        iw_doc_read(&d, 0, g_buf, CAP);
        if (memcmp(g_buf, g_buf2, n) != 0) {
            uint32_t k = 0; while (g_buf[k] == g_buf2[k]) k++;
            fprintf(stderr, "  FAIL P2 trial %d (w=%u): reflow != reference wrap at byte %u (got 0x%02x want 0x%02x)\n",
                    t, w, k, g_buf[k], g_buf2[k]);
            bad = 1; break;
        }
        if (iw_reflow_all(&d, w) != IW_OK || iw_doc_read(&d, 0, g_buf2, CAP) != n || memcmp(g_buf, g_buf2, n)) {
            fprintf(stderr, "  FAIL P2 trial %d: reflow is not idempotent\n", t); bad = 1; break;
        }
        for (uint32_t i = 0; i < n; i++) {
            uint8_t a = g_buf[i] == 0x0D ? ' ' : g_buf[i], c = g_orig[i] == 0x0D ? ' ' : g_orig[i];
            if (a != c) { fprintf(stderr, "  FAIL P2 trial %d: the logical text changed at %u\n", t, i); bad = 1; break; }
        }
        /* (d) layout: no line wider than w; lines tile the document. */
        {
            uint32_t s = 0, guard = 0;
            for (;;) {
                iw_line_t ln;
                iw_line_at(&d, s, w, &ln);
                if (ln.cols > w || ln.start != s || ln.end < ln.text_end || ln.text_end < s) {
                    fprintf(stderr, "  FAIL P2 trial %d: layout line @%u has %u cols (w %u)\n", t, s, ln.cols, w);
                    bad = 1; break;
                }
                if (ln.end >= d.len && ln.term == IW_U_TEXT) break;
                if (ln.end == s || ++guard > 100000u) { fprintf(stderr, "  FAIL P2: layout stuck\n"); bad = 1; break; }
                s = ln.end;
            }
        }
    }
    /* The reflow of a paragraph touches that paragraph only. */
    if (!bad) {
        iw_doc_t d;
        const char *txt = "aaaa bbbb cccc\ndddd eeee ffff gggg\nhhhh iiii";
        uint32_t n = (uint32_t)strlen(txt);
        memcpy(g_orig, txt, n);
        doc_fresh(&d, g_orig, n);
        CHECK(iw_reflow_para(&d, 20, 9) == IW_OK, "reflow one paragraph");
        iw_doc_read(&d, 0, g_buf, CAP);
        CHECK(memcmp(g_buf, "aaaa bbbb cccc\ndddd eeee\rffff gggg\nhhhh iiii", n) == 0,
              "only the middle paragraph was wrapped (width 9)");
    }
    CHECK(!bad, "P2 reflow == reference wrap, idempotent, logical text kept, lines fit");
    if (bad) g_reflow_fail++;
    printf("  P2 %s: reflow == the reference wrapper over 2000 random paragraphs; idempotent; layout fits\n", g_reflow_fail ? "RED" : "green");
}

/* ======================================================================== *
 * P3: typing sessions on the editor
 * ======================================================================== */
enum { K_CHAR, K_SPACE, K_ENTER, K_BS, K_DEL, K_LEFT, K_RIGHT, K_UP, K_DOWN, K_HOME, K_END, K_N };
typedef struct { uint8_t k; uint8_t ch; } key_t2;
#define MAXKEYS 400
static key_t2 g_keys[MAXKEYS];
static const char *KNAME[K_N] = { "char", "space", "enter", "bs", "del", "left", "right", "up", "down", "home", "end" };

/* The model: logical text (SRt read as a space) and a cursor index. All
 * units are single bytes here, so a byte index IS a unit index. */
static uint8_t g_ml[CAP];

static int where_ok(const iw_ed_t *e, char *why, size_t whysz)
{
    iw_where_t w;
    if (e->cur > e->doc.len || !iw_is_boundary(&e->doc, e->cur)) { snprintf(why, whysz, "cursor %u off a boundary", e->cur); return 0; }
    iw_ed_where(e, &w);
    if (iw_ed_pos_at(e, w.line, w.col) != e->cur) {
        snprintf(why, whysz, "where(line %u col %u) -> pos_at = %u, cursor %u", w.line, w.col,
                 iw_ed_pos_at(e, w.line, w.col), e->cur);
        return 0;
    }
    return 1;
}

static int run_p3(const key_t2 *keys, int n, const uint8_t *skip, uint32_t width, int verbose)
{
    static iw_ed_t e;
    uint32_t mlen = 0, mcur = 0;
    int step = 0, edited = 0;
    char why[160];
    iw_doc_init(&e.doc, g_add, CAP, g_pc, PCS, CAP);
    iw_doc_load(&e.doc, g_orig, 0);
    iw_ed_init(&e, width);
    for (int i = 0; i < n; i++) {
        const key_t2 *k = &keys[i];
        iw_where_t before, after;
        if (skip && skip[i]) continue;
        step++;
        if (verbose) printf("    %s%s%c%s\n", KNAME[k->k], k->k == K_CHAR ? " '" : "", k->k == K_CHAR ? k->ch : ' ', k->k == K_CHAR ? "'" : "");
        iw_ed_where(&e, &before);
        switch (k->k) {
        case K_CHAR: case K_SPACE: {
            uint8_t c = k->k == K_SPACE ? ' ' : k->ch;
            if (iw_ed_type(&e, c) != IW_OK) { if (verbose) printf("      type refused\n"); return step; }
            memmove(g_ml + mcur + 1, g_ml + mcur, mlen - mcur); g_ml[mcur++] = c; mlen++;
            edited = 1;
            break; }
        case K_ENTER:
            if (iw_ed_enter(&e) != IW_OK) return step;
            memmove(g_ml + mcur + 1, g_ml + mcur, mlen - mcur); g_ml[mcur++] = 0x0A; mlen++;
            edited = 1;
            break;
        case K_BS:
            (void)iw_ed_backspace(&e);
            if (mcur) { memmove(g_ml + mcur - 1, g_ml + mcur, mlen - mcur); mcur--; mlen--; edited = 1; }
            break;
        case K_DEL:
            (void)iw_ed_delete(&e);
            if (mcur < mlen) { memmove(g_ml + mcur, g_ml + mcur + 1, mlen - mcur - 1); mlen--; edited = 1; }
            break;
        case K_LEFT:  iw_ed_left(&e);  if (mcur) mcur--; break;
        case K_RIGHT: iw_ed_right(&e); if (mcur < mlen) mcur++; break;
        case K_UP: case K_DOWN: case K_HOME: case K_END:
            if (k->k == K_UP) iw_ed_up(&e);
            else if (k->k == K_DOWN) iw_ed_down(&e);
            else if (k->k == K_HOME) iw_ed_line_home(&e);
            else iw_ed_line_end(&e);
            iw_ed_where(&e, &after);
            if (k->k == K_UP && after.line != (before.line ? before.line - 1u : 0u)) { if (verbose) printf("      up: line %u -> %u\n", before.line, after.line); return step; }
            if (k->k == K_HOME && (after.line != before.line || after.col != 0u)) { if (verbose) printf("      home: not column 0 of the same line\n"); return step; }
            if (k->k == K_END && after.line != before.line) { if (verbose) printf("      end: left the line\n"); return step; }
            if (k->k == K_DOWN && after.line != before.line && after.line != before.line + 1u) { if (verbose) printf("      down: line %u -> %u\n", before.line, after.line); return step; }
            mcur = e.cur;
            break;
        }
        if (e.cur != mcur) { if (verbose) printf("      cursor %u, model %u\n", e.cur, mcur); return step; }
        if (e.doc.len != mlen) { if (verbose) printf("      len %u, model %u\n", e.doc.len, mlen); return step; }
        iw_doc_read(&e.doc, 0, g_buf, CAP);
        for (uint32_t q = 0; q < mlen; q++) {
            uint8_t a = g_buf[q] == 0x0D ? ' ' : g_buf[q];
            if (a != g_ml[q]) { if (verbose) printf("      logical text differs at %u\n", q); return step; }
        }
        memcpy(g_buf2, g_ml, mlen);
        ref_wrap(g_buf2, mlen, width);
        if (memcmp(g_buf, g_buf2, mlen) != 0) { if (verbose) printf("      bytes != reference wrap of the model\n"); return step; }
        if (!where_ok(&e, why, sizeof why)) { if (verbose) printf("      %s\n", why); return step; }
        if (edited && !e.dirty) { if (verbose) printf("      an edit did not mark the document dirty\n"); return step; }
    }
    return 0;
}

static void gen_keys(int n)
{
    for (int i = 0; i < n; i++) {
        uint32_t r = rn(100);
        g_keys[i].ch = (uint8_t)('a' + rn(26));
        g_keys[i].k = r < 45 ? K_CHAR : r < 60 ? K_SPACE : r < 64 ? K_ENTER : r < 70 ? K_BS :
                      r < 75 ? K_DEL : r < 80 ? K_LEFT : r < 85 ? K_RIGHT : r < 90 ? K_UP :
                      r < 95 ? K_DOWN : r < 97 ? K_HOME : K_END;
    }
}

static void p3_typing(void)
{
    int failed = 0;
    g_rng = 0x7E57u;
    for (int t = 0; t < 400 && !failed; t++) {
        int n = 1 + (int)rn(MAXKEYS);
        uint32_t width = 8u + rn(60);
        if (t % 4 == 0) width = IW_WP5_WRAP_COLS;
        gen_keys(n);
        if (run_p3(g_keys, n, 0, width, 0)) {
            static uint8_t skip[MAXKEYS];
            int live = n, changed = 1;
            memset(skip, 0, sizeof skip);
            while (changed) {
                changed = 0;
                for (int i = 0; i < n; i++) {
                    if (skip[i]) continue;
                    skip[i] = 1;
                    if (run_p3(g_keys, n, skip, width, 0)) { changed = 1; live--; } else skip[i] = 0;
                }
            }
            fprintf(stderr, "  FAIL P3 session %d (width %u): shrunk to %d key(s):\n", t, width, live);
            (void)run_p3(g_keys, n, skip, width, 1);
            failed = 1;
        }
    }
    CHECK(!failed, "P3 typing sessions == the model; cursor invariants hold");
    if (failed) g_typing_fail++;
    printf("  P3 %s: 400 random typing sessions == the flat model + reference wrap; cursor invariants\n", g_typing_fail ? "RED" : "green");
}

/* ======================================================================== *
 * P4: opaque codes
 * ======================================================================== */
static void p4_codes(void)
{
    /* "Ab" [BOLD on] "cd" [bold off] " e" [D0/01 margins] "f" HRt "g"
     * C3 0C C3 / C4 0C C4 = MINT01 @0x19B / @0x1A2; the D0/01 group =
     * MINT04 @0x148 (size 12 = 8 payload + 4 trailer). */
    static const uint8_t doc[] = {
        'A','b', 0xC3,0x0C,0xC3, 'c','d', 0xC4,0x0C,0xC4, ' ','e',
        0xD0,0x01,0x0C,0x00, 0xB0,0x04,0xB0,0x04,0x60,0x09,0x60,0x09, 0x0C,0x00,0x01,0xD0,
        'f', 0x0A, 'g' };
    static const uint32_t starts[] = { 0,1,2,5,6,7,10,11,12,28,29,30,31 };
    const uint32_t nb = (uint32_t)sizeof doc, ns = (uint32_t)(sizeof starts / sizeof *starts);
    static iw_ed_t e;
    int bad = 0;
    memcpy(g_orig, doc, nb);
    iw_doc_init(&e.doc, g_add, CAP, g_pc, PCS, CAP);
    iw_doc_load(&e.doc, g_orig, nb);
    iw_ed_init(&e, 65);
    for (uint32_t i = 0; i + 1u < ns; i++) {
        if (iw_unit_next(&e.doc, starts[i]) != starts[i + 1]) { fprintf(stderr, "  FAIL P4 next(%u) = %u want %u\n", starts[i], iw_unit_next(&e.doc, starts[i]), starts[i + 1]); bad = 1; }
        if (iw_unit_prev(&e.doc, starts[i + 1]) != starts[i]) { fprintf(stderr, "  FAIL P4 prev(%u) = %u want %u\n", starts[i + 1], iw_unit_prev(&e.doc, starts[i + 1]), starts[i]); bad = 1; }
    }
    for (uint32_t p = 0; p <= nb; p++) {
        int want = 0;
        for (uint32_t i = 0; i < ns; i++) if (starts[i] == p) want = 1;
        if (iw_is_boundary(&e.doc, p) != want) { fprintf(stderr, "  FAIL P4 boundary(%u)\n", p); bad = 1; }
    }
    CHECK(iw_unit_len(doc, nb, 12) == 16u, "the D0 group is one 16-byte unit");
    CHECK(iw_unit_len(doc, nb, 2) == 3u, "the C3 group is one 3-byte unit");
    CHECK(iw_unit_kind(&e.doc, 2) == IW_U_CODE && iw_unit_kind(&e.doc, 12) == IW_U_CODE, "groups are opaque codes");
    {
        iw_line_t ln;
        iw_line_at(&e.doc, 0, 65, &ln);
        CHECK(ln.cols == 7u && ln.term == IW_U_HRT && ln.end == 30u, "codes are zero width: \"Abcd ef\" is 7 columns");
    }
    /* Walk right across the line: positions are exactly the unit starts. */
    e.cur = 0;
    for (uint32_t i = 1; i < ns; i++) { iw_ed_right(&e); if (e.cur != starts[i]) { fprintf(stderr, "  FAIL P4 right -> %u want %u\n", e.cur, starts[i]); bad = 1; } }
    for (uint32_t i = ns - 1u; i > 0; i--) { iw_ed_left(&e); if (e.cur != starts[i - 1]) { fprintf(stderr, "  FAIL P4 left -> %u want %u\n", e.cur, starts[i - 1]); bad = 1; } }
    /* Backspace right after [BOLD] and Delete right before the D0 group:
     * the codes stay. An attribute code is ASKED about (WP 5.1's "Delete
     * [BOLD]? No (Yes)", bead initech-jojk slice 2, ../wordperfect51-decomp
     * mint/shots_wpMINT11/01_bs_prompt.png): nothing changes until the
     * answer; any other code is stepped over (P1 protection). */
    e.cur = 5;
    CHECK(iw_ed_backspace(&e) == IW_E_ASK, "Backspace on an attribute code asks");
    CHECK(e.doc.len == nb && e.cur == 5u && e.ask == 2u && e.ask_del == 0, "nothing changes until the answer; the question is about [BOLD]");
    e.cur = 12;
    CHECK(iw_ed_delete(&e) == IW_E_PROTECTED, "Delete on a code is refused");
    CHECK(e.doc.len == nb && e.cur == 28u, "the group stays; the cursor steps past it");
    /* Typing between codes never splits one; the bytes stay well-formed. */
    g_rng = 0xC0DEu;
    for (int k = 0; k < 300 && !bad; k++) {
        uint32_t r = rn(6);
        if (r == 0) iw_ed_left(&e); else if (r == 1) iw_ed_right(&e);
        else if (r == 2) (void)iw_ed_backspace(&e);
        else (void)iw_ed_type(&e, (uint8_t)('a' + rn(26)));
        uint32_t p = 0, len = e.doc.len, n = iw_doc_read(&e.doc, 0, g_buf, CAP);
        if (n != len) bad = 1;
        while (p < len) { uint32_t u = iw_unit_len(g_buf, len, p); if (!u) { fprintf(stderr, "  FAIL P4 malformed at %u after %d keys\n", p, k); bad = 1; break; } p += u; }
        if (!iw_is_boundary(&e.doc, e.cur)) { fprintf(stderr, "  FAIL P4 cursor off a boundary\n"); bad = 1; }
    }
    {
        uint32_t n3 = 0, n4 = 0, nd = 0, len = iw_doc_read(&e.doc, 0, g_buf, CAP);
        for (uint32_t p = 0; p < len; ) {
            uint32_t u = iw_unit_len(g_buf, len, p);
            if (!u) break;
            if (g_buf[p] == 0xC3) n3++;
            if (g_buf[p] == 0xC4) n4++;
            if (g_buf[p] == 0xD0) nd++;
            p += u;
        }
        CHECK(n3 == 1 && n4 == 1 && nd == 1, "after 300 random keys every code is still there, whole");
    }
    /* Refused units (the validator): unverified fixed code, broken mirror,
     * truncated group, unseen single byte. */
    {
        static const uint8_t c5[] = { 0xC5, 1, 2, 3, 0xC5 };
        static const uint8_t badmirror[] = { 0xD0, 0x01, 0x06, 0x00, 0x01, 0x00, 0x06, 0x00, 0x02, 0xD0 };
        static const uint8_t trunc[] = { 0xC1, 0xE0, 0xE0 };
        static const uint8_t unseen = 0x09;
        CHECK(iw_unit_len(c5, sizeof c5, 0) == 0u, "C5 (unverified length) is refused");
        CHECK(iw_unit_len(badmirror, sizeof badmirror, 0) == 0u, "a variable group whose trailer does not mirror is refused");
        CHECK(iw_unit_len(trunc, sizeof trunc, 0) == 0u, "a truncated fixed group is refused");
        CHECK(iw_unit_len(&unseen, 1, 0) == 0u, "an unseen single byte (0x09) is refused");
    }
    CHECK(!bad, "P4 opaque codes: units walk both ways, are kept whole and protected");
    if (bad) g_units_fail++;
    printf("  P4 %s: opaque codes walk both ways, are zero width, protected and never split\n", g_units_fail ? "RED" : "green");
}

/* ======================================================================== *
 * P5: the status-line measures
 * ======================================================================== */
static int inch_is(uint32_t h, const char *want)
{
    char b[16];
    uint32_t n = iw_fmt_inches(h, b);
    b[n] = 0;
    return strcmp(b, want) == 0;
}

static void p5_status(void)
{
    static iw_ed_t e;
    iw_where_t w;
    int bad = 0;
    bad |= !inch_is(iw_ln_hundredths(1), "1\"");
    bad |= !inch_is(iw_pos_hundredths(0), "1\"");
    bad |= !inch_is(iw_ln_hundredths(2), "1.17\"");      /* MINT02.shots/02_typed */
    bad |= !inch_is(iw_pos_hundredths(16), "2.6\"");     /* MINT02.shots/02_typed */
    bad |= !inch_is(iw_ln_hundredths(7), "2\"");         /* MINT01.shots/02_typed */
    bad |= !inch_is(iw_ln_hundredths(4), "1.5\"");
    bad |= !inch_is(iw_pos_hundredths(65), "7.5\"");
    /* MINT02 typed: Return, "Plain text only." -> Ln 1.17" Pos 2.6". */
    iw_doc_init(&e.doc, g_add, CAP, g_pc, PCS, CAP);
    iw_doc_load(&e.doc, g_orig, 0);
    iw_ed_init(&e, IW_WP5_WRAP_COLS);
    iw_ed_where(&e, &w);
    bad |= !(w.page == 1 && w.page_line == 1 && w.col == 0);
    (void)iw_ed_enter(&e);
    for (const char *s = "Plain text only."; *s; s++) (void)iw_ed_type(&e, (uint8_t)*s);
    iw_ed_where(&e, &w);
    bad |= !(w.page == 1 && w.page_line == 2 && w.col == 16 && w.line == 1);
    /* Page 2 starts after IW_WP5_LINES_PER_PAGE lines. */
    for (uint32_t i = 0; i < IW_WP5_LINES_PER_PAGE; i++) (void)iw_ed_enter(&e);
    iw_ed_where(&e, &w);
    bad |= !(w.page == 2 && w.page_line == 2);
    CHECK(!bad, "P5 Ln/Pos/Pg read as the real WordPerfect 5.1 status line does");
    if (bad) g_status_fail++;
    printf("  P5 %s: status measures: Ln 1\" / 1.17\" / 2\", Pos 1\" / 2.6\", Pg after %u lines\n", g_status_fail ? "RED" : "green", IW_WP5_LINES_PER_PAGE);
}

int main(void)
{
    p1_piece_table();
    p2_reflow();
    p3_typing();
    p4_codes();
    p5_status();
    if (g_model_fail)  fprintf(stderr, "  RED-REASON model: the piece table disagrees with the flat model\n");
    if (g_reflow_fail) fprintf(stderr, "  RED-REASON reflow: wrap differs from the reference / not idempotent\n");
    if (g_typing_fail) fprintf(stderr, "  RED-REASON typing: an editing session disagrees with the model\n");
    if (g_units_fail)  fprintf(stderr, "  RED-REASON units: the unit walk / code protection is wrong\n");
    if (g_status_fail) fprintf(stderr, "  RED-REASON status: Ln/Pos/Pg differ from the WP 5.1 screen\n");
    return TEST_SUMMARY("test-iword-core");
}
