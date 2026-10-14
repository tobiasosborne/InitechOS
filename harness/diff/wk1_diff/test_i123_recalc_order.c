/*
 * harness/diff/wk1_diff/test_i123_recalc_order.c -- THE ORACLE for Initech
 * 123 recalculation ORDER, the iteration count, MANUAL vs AUTOMATIC and the
 * circular-reference (CIRC) indicator (factory C; bead initech-94ah, I123
 * P2).
 *
 * Authorities (Law 1/2):
 *   (1) the R2.2 Reference Manual, /Worksheet Global Recalculation, p. 12-31
 *       ..12-33 (PDF 275-277): Natural = precedents first; Columnwise =
 *       "Starting in A1 ... column by column"; Rowwise = "row by row";
 *       Iteration = passes, "used only when ... Columnwise or Rowwise, or
 *       when ... Natural and a circular reference exists"; Manual = only on
 *       CALC (F9), the CALC indicator showing. Parts O and I derive their
 *       expected values from those sentences by hand (written below, per
 *       fixture), never from the engine.
 *   (2) the real 123.EXE: CIRC01, MINT01 and MINT04 were minted by TYPING
 *       (mint/mint01.steps, mint/MINT04.steps; re/mint-results-001.md);
 *       part R replays the same keystrokes' entries through the engine and
 *       compares every formula's value with the value real 1-2-3 cached, as
 *       the corpus' independent reader wk1_ref.py reads it (through
 *       wk1_formula_ref.py).
 *
 * PARTS
 *   O  ORDER: two fixtures whose values differ between natural, columnwise
 *      and rowwise order, recalculated once from zero caches.
 *   I  ITERATION: the count applies to columnwise/rowwise always, to
 *      natural only with a cycle; a self-referencing counter.
 *   C  CIRC: the indicator lights on a cycle in natural order (and names a
 *      cell in it), never on an acyclic sheet, and never in columnwise or
 *      rowwise order ("occurs only when the recalculation order is
 *      Natural", p. 1-6, PDF 22).
 *   R  REPLAY: MINT01 (automatic), CIRC01 (automatic, circular: CIRC on and
 *      -38 / -19 / -8.5 / "small"), MINT04 (manual, columnwise, 7 passes:
 *      each entry computes only itself, CALC lights; then F9 iterates the
 *      A2->C2->B2 cycle as the manual says).
 *
 * Mutants (Rule 6; test-i123-recalc-order-mutant):
 *   I123_MUT_NO_PRECEDENTS  natural order ignores cell references -> O RED
 *   I123_MUT_CIRC_BLIND     the CIRC indicator never lights      -> C RED
 *
 * Usage: test_i123_recalc_order <LOTUS123_DECOMP> <wk1_formula_ref.py>
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"
#include "i123.h"
#include "i123/wk1_format.h"

TEST_HARNESS();

static const char *g_corpus, *g_ref;
static int g_order_fail, g_iter_fail, g_circ_fail, g_replay_fail;

#define SHEET_MEM (256u * 1024u)
static uint8_t g_mem[SHEET_MEM];
static uint8_t g_cmem[I123_CALC_BYTES(512u) + 64u];
static i123_calc_t g_cx, g_nest;

#define ORD_NATURAL 0x00u
#define ORD_COLUMN  0x01u
#define ORD_ROW     0xFFu

static double num_at(const i123_sheet_t *sh, uint16_t c, uint16_t r)
{
    i123_val_t v;
    i123_cell_value(sh, c, r, &v);
    return v.t == I123_V_NUM ? v.n : -999999.0;
}

static void fresh(i123_sheet_t *sh)
{
    CHECK(i123_sheet_init(sh, g_mem, SHEET_MEM, 512u) == I123_OK, "init");
}

/* A formula stored WITHOUT recalculation, cache 0 (the raw entry). */
static void put_formula(i123_sheet_t *sh, uint16_t c, uint16_t r, const char *text)
{
    uint8_t code[I123_CODE_MAX], zero[8] = { 0 };
    uint32_t n = 0;
    CHECK(i123_formula_compile(text, (uint32_t)strlen(text), c, r, code, sizeof code, &n) == I123_OK, text);
    CHECK(i123_set_formula_raw(sh, c, r, WK1_FMT_FRESH_CELL, zero, code, n, (const char *)0, 0) == I123_OK, text);
}

static void expect(int *fails, const char *what, double got, double want)
{
    if (got != want) {
        fprintf(stderr, "  FAIL %s: got %.17g, want %.17g\n", what, got, want);
        (*fails)++;
    }
    CHECK(got == want, what);
}

/* ---- PART O: order ---------------------------------------------------------
 * Fixture F1: A1 = 1; B1 = +A2*10; A2 = +A1+1. B1 reads A2, which lies
 * AFTER B1 row by row but BEFORE it column by column. From zero caches,
 * one pass:
 *   natural    A2 first (B1 depends on it): A2 = 2, B1 = 20
 *   columnwise A1, A2, B1:                  A2 = 2, B1 = 20
 *   rowwise    A1, B1, A2:                  B1 = 0*10 = 0, A2 = 2
 * Fixture F2: C1 = 3; B1 = +C1+1; A2 = +B1*2. A2 reads B1, which lies
 * BEFORE A2 row by row but AFTER it column by column:
 *   natural    B1 = 4, A2 = 8
 *   columnwise A2 = 0*2 = 0, then B1 = 4
 *   rowwise    B1 = 4, A2 = 8 */
static void build_f1(i123_sheet_t *sh, uint8_t order)
{
    fresh(sh);
    CHECK(i123_set_number(sh, 0, 0, 1.0) == I123_OK, "A1");
    put_formula(sh, 1, 0, "+A2*10");
    put_formula(sh, 0, 1, "+A1+1");
    sh->calcorder = order;
    sh->calccount = 1;
}

static void build_f2(i123_sheet_t *sh, uint8_t order)
{
    fresh(sh);
    CHECK(i123_set_number(sh, 2, 0, 3.0) == I123_OK, "C1");
    put_formula(sh, 1, 0, "+C1+1");
    put_formula(sh, 0, 1, "+B1*2");
    sh->calcorder = order;
    sh->calccount = 1;
}

static void part_o(void)
{
    static i123_sheet_t sh;
    static const struct { uint8_t order; const char *name; double f1_b1, f1_a2, f2_b1, f2_a2; } T[] = {
        { ORD_NATURAL, "natural",    20.0, 2.0, 4.0, 8.0 },
        { ORD_COLUMN,  "columnwise", 20.0, 2.0, 4.0, 0.0 },
        { ORD_ROW,     "rowwise",     0.0, 2.0, 4.0, 8.0 },
    };
    for (int i = 0; i < 3; i++) {
        char w[96];
        build_f1(&sh, T[i].order);
        CHECK(i123_recalc(&g_cx, &sh) == I123_OK, "recalc F1");
        snprintf(w, sizeof w, "O F1 %s B1", T[i].name); expect(&g_order_fail, w, num_at(&sh, 1, 0), T[i].f1_b1);
        snprintf(w, sizeof w, "O F1 %s A2", T[i].name); expect(&g_order_fail, w, num_at(&sh, 0, 1), T[i].f1_a2);
        build_f2(&sh, T[i].order);
        CHECK(i123_recalc(&g_cx, &sh) == I123_OK, "recalc F2");
        snprintf(w, sizeof w, "O F2 %s B1", T[i].name); expect(&g_order_fail, w, num_at(&sh, 1, 0), T[i].f2_b1);
        snprintf(w, sizeof w, "O F2 %s A2", T[i].name); expect(&g_order_fail, w, num_at(&sh, 0, 1), T[i].f2_a2);
    }
    printf("  part O: natural / columnwise / rowwise give the manual's values on the two order fixtures (%d wrong)\n", g_order_fail);
}

/* ---- PART I: iteration ------------------------------------------------------
 *   F1 rowwise, Iteration 2: pass 1 B1 = 0, A2 = 2; pass 2 B1 = 20.
 *   F1 natural, Iteration 5: B1 = 20; a counter F6 = +F6+1 added makes a
 *     cycle, and the recalculation then makes 5 passes (F6 = 5).
 *   Counter A1 = +A1+1 (a self reference), natural, Iteration 5: 5 passes
 *     from 0 -> 5; Iteration 1 -> 1; recalculated again (Iteration 5) -> 10. */
static void part_i(void)
{
    static i123_sheet_t sh;
    build_f1(&sh, ORD_ROW);
    sh.calccount = 2;
    CHECK(i123_recalc(&g_cx, &sh) == I123_OK, "recalc");
    expect(&g_iter_fail, "I F1 rowwise x2 B1", num_at(&sh, 1, 0), 20.0);

    build_f1(&sh, ORD_NATURAL);
    sh.calccount = 5;
    CHECK(i123_recalc(&g_cx, &sh) == I123_OK, "recalc");
    expect(&g_iter_fail, "I natural x5 acyclic B1", num_at(&sh, 1, 0), 20.0);
    put_formula(&sh, 5, 5, "+F6+1");       /* a self-referencing counter: a cycle */
    CHECK(i123_recalc(&g_cx, &sh) == I123_OK, "recalc");
    expect(&g_iter_fail, "I natural x5 with a cycle: the counter makes 5 passes", num_at(&sh, 5, 5), 5.0);
    expect(&g_iter_fail, "I natural x5 with a cycle: B1", num_at(&sh, 1, 0), 20.0);

    fresh(&sh);
    put_formula(&sh, 0, 0, "+A1+1");
    sh.calccount = 5;
    CHECK(i123_recalc(&g_cx, &sh) == I123_OK, "recalc");
    expect(&g_iter_fail, "I counter natural x5", num_at(&sh, 0, 0), 5.0);
    CHECK(i123_recalc(&g_cx, &sh) == I123_OK, "recalc");
    expect(&g_iter_fail, "I counter natural x5 again", num_at(&sh, 0, 0), 10.0);
    sh.calccount = 1;
    CHECK(i123_recalc(&g_cx, &sh) == I123_OK, "recalc");
    expect(&g_iter_fail, "I counter natural x1", num_at(&sh, 0, 0), 11.0);
    printf("  part I: iteration passes counted as the manual states (%d wrong)\n", g_iter_fail);
}

/* ---- PART C: the CIRC indicator ------------------------------------------------ */
static void circ_expect(const i123_sheet_t *sh, int want, const char *what)
{
    if ((sh->circ != 0) != want) {
        fprintf(stderr, "  FAIL C %s: CIRC %s\n", what, sh->circ ? "lit" : "dark");
        g_circ_fail++;
    }
    CHECK((sh->circ != 0) == want, what);
}

static void part_c(void)
{
    static i123_sheet_t sh;
    static const uint8_t ORD[3] = { ORD_NATURAL, ORD_COLUMN, ORD_ROW };
    for (int i = 0; i < 3; i++) {
        build_f1(&sh, ORD[i]);
        CHECK(i123_recalc(&g_cx, &sh) == I123_OK, "recalc");
        circ_expect(&sh, 0, "acyclic F1: dark in every order");
        /* close a three-cell loop: A1 = +B1 (A1 -> B1 -> A2 -> A1). CIRC
         * "occurs only when the recalculation order is Natural" (p. 1-6,
         * PDF 22): lit in natural, dark in columnwise and rowwise. */
        put_formula(&sh, 0, 0, "+B1");
        CHECK(i123_recalc(&g_cx, &sh) == I123_OK, "recalc");
        circ_expect(&sh, ORD[i] == ORD_NATURAL, "A1->B1->A2->A1: lit only in natural order");
        if (ORD[i] == ORD_NATURAL) CHECK(sh.circ_row <= 1u && sh.circ_col <= 1u, "CIRC names a cell of the cycle");
    }
    fresh(&sh);
    put_formula(&sh, 2, 2, "@SUM(A1..C3)");     /* a range that contains itself */
    CHECK(i123_recalc(&g_cx, &sh) == I123_OK, "recalc");
    circ_expect(&sh, 1, "@SUM over a range holding the formula: lit");
    CHECK(sh.circ_col == 2u && sh.circ_row == 2u, "CIRC names C3");
    fresh(&sh);
    put_formula(&sh, 2, 2, "@SUM(A1..B2)");
    CHECK(i123_recalc(&g_cx, &sh) == I123_OK, "recalc");
    circ_expect(&sh, 0, "@SUM over a range beside it: dark");
    printf("  part C: CIRC lights on every natural-order cycle, stays dark on acyclic sheets and in columnwise/rowwise order (%d wrong)\n", g_circ_fail);
}

/* ---- PART R: replays of the minted typing ----------------------------------- */
typedef struct { uint16_t c, r; char kind[4]; char payload[512]; } ref_t;

static int load_refs(const char *name, ref_t *refs, int cap)
{
    char cmd[2048], line[1024];
    FILE *p;
    int n = 0;
    snprintf(cmd, sizeof cmd, "LOTUS123_DECOMP='%s' python3 '%s' '%s/goldens/minted/%s.WK1'",
             g_corpus, g_ref, g_corpus, name);
    p = popen(cmd, "r");
    if (!p) return -1;
    while (fgets(line, sizeof line, p) && n < cap) {
        unsigned c, r;
        refs[n].payload[0] = '\0';
        if (sscanf(line, "F %u %u %3s %500s", &c, &r, refs[n].kind, refs[n].payload) >= 3) {
            refs[n].c = (uint16_t)c; refs[n].r = (uint16_t)r; n++;
        }
    }
    if (pclose(p) != 0) return -1;
    return n;
}

static int same_as_ref(const i123_sheet_t *sh, const ref_t *rf)
{
    i123_val_t v;
    char payload[512];
    i123_cell_value(sh, rf->c, rf->r, &v);
    payload[0] = '\0';
    if (strcmp(rf->kind, "NUM") == 0) {
        uint8_t b[8];
        if (v.t != I123_V_NUM) return 0;
        memcpy(b, &v.n, 8);
        for (int i = 0; i < 8; i++) sprintf(payload + 2 * i, "%02x", b[i]);
    } else if (strcmp(rf->kind, "STR") == 0) {
        if (v.t != I123_V_STR) return 0;
        for (uint32_t i = 0; i < v.slen; i++) sprintf(payload + 2 * i, "%02x", v.s[i]);
    } else if (strcmp(rf->kind, "ERR") == 0) {
        return v.t == I123_V_ERR;
    } else if (strcmp(rf->kind, "NA") == 0) {
        return v.t == I123_V_NA;
    }
    return strcmp(payload, rf->payload) == 0;
}

static void replay_check(const i123_sheet_t *sh, const char *name)
{
    ref_t refs[16];
    int n = load_refs(name, refs, 16), ok = 0;
    CHECK(n > 0, "wk1_ref values of the minted golden");
    for (int i = 0; i < n; i++) {
        if (same_as_ref(sh, &refs[i])) ok++;
        else {
            fprintf(stderr, "  FAIL R %s (%u,%u): the replay differs from the real 1-2-3 cache %s %s\n",
                    name, refs[i].c, refs[i].r, refs[i].kind, refs[i].payload);
            g_replay_fail++;
        }
    }
    CHECK(ok == n, "replayed values == the real 1-2-3 caches");
    printf("  part R: %s replayed by typing: %d/%d formula values equal the real 1-2-3 caches\n", name, ok, n);
}

static void type_formula(i123_sheet_t *sh, uint16_t c, uint16_t r, const char *t)
{
    CHECK(i123_enter_formula(&g_cx, sh, c, r, t, (uint32_t)strlen(t)) == I123_OK, t);
}

static void type_number(i123_sheet_t *sh, uint16_t c, uint16_t r, double x)
{
    CHECK(i123_set_number(sh, c, r, x) == I123_OK, "number");
    CHECK(i123_after_entry(&g_cx, sh) == I123_OK, "after entry");
}

static void part_r(void)
{
    static i123_sheet_t sh;

    /* MINT01 (mint/mint01.steps): row 1 Hello 123 1.5 -7, then row 2. */
    fresh(&sh);
    CHECK(i123_set_label(&sh, 0, 0, '\'', "Hello", 5) == I123_OK, "Hello");
    type_number(&sh, 1, 0, 123.0);
    type_number(&sh, 2, 0, 1.5);
    type_number(&sh, 3, 0, -7.0);
    type_formula(&sh, 0, 1, "+B1*2");
    type_formula(&sh, 1, 1, "@SUM(B1..D1)");
    type_formula(&sh, 2, 1, "@AVG(B1..D1)");
    type_formula(&sh, 3, 1, "@IF(B1>100,\"big\",\"small\")");
    replay_check(&sh, "MINT01");
    if (sh.circ) { fprintf(stderr, "  FAIL R MINT01: CIRC lit\n"); g_replay_fail++; }
    CHECK(!sh.circ, "MINT01: no CIRC");

    /* CIRC01 (re/mint-results-001.md): the same entries, the formulas typed
     * into ROW 1 over the numbers -- real 1-2-3 showed CIRC (shot
     * CIRC01.shots/02_entered.png) and cached -38 / -19 / -8.5 / "small". */
    fresh(&sh);
    CHECK(i123_set_label(&sh, 0, 0, '\'', "Hello", 5) == I123_OK, "Hello");
    type_number(&sh, 1, 0, 123.0);
    type_number(&sh, 2, 0, 1.5);
    type_number(&sh, 3, 0, -7.0);
    type_formula(&sh, 0, 0, "+B1*2");
    type_formula(&sh, 1, 0, "@SUM(B1..D1)");
    type_formula(&sh, 2, 0, "@AVG(B1..D1)");
    type_formula(&sh, 3, 0, "@IF(B1>100,\"big\",\"small\")");
    replay_check(&sh, "CIRC01");
    if (!sh.circ) { fprintf(stderr, "  FAIL R CIRC01: CIRC dark\n"); g_circ_fail++; }
    CHECK(sh.circ, "CIRC01: CIRC lit, as on the real screen");

    /* MINT04 (mint/MINT04.steps): /wgrm /wgrc /wgri7, then 1, +A1+1, +B1*2,
     * +C2+1, +A2, +B2 -- MANUAL: each entry computes only itself. */
    fresh(&sh);
    sh.calcmode = 0x00; sh.calcorder = ORD_COLUMN; sh.calccount = 7;
    type_number(&sh, 0, 0, 1.0);
    type_formula(&sh, 1, 0, "+A1+1");
    type_formula(&sh, 2, 0, "+B1*2");
    type_formula(&sh, 0, 1, "+C2+1");
    type_formula(&sh, 1, 1, "+A2");
    type_formula(&sh, 2, 1, "+B2");
    replay_check(&sh, "MINT04");
    CHECK(sh.calc, "MINT04: CALC lit after manual entries (shot 02_entered.png)");
    /* F9: columnwise, 7 passes over the A2 -> C2 -> B2 -> A2 cycle, from
     * A2 = B2 = C2 = 1. Pass k (A1, A2, B1, B2, C1, C2): A2 = C2 + 1 = k+1,
     * B2 = A2, C2 = B2 -> after 7 passes 8, 8, 8; B1 = 2, C1 = 4. */
    CHECK(i123_recalc(&g_cx, &sh) == I123_OK, "F9");
    expect(&g_replay_fail, "R MINT04 F9 A2", num_at(&sh, 0, 1), 8.0);
    expect(&g_replay_fail, "R MINT04 F9 B2", num_at(&sh, 1, 1), 8.0);
    expect(&g_replay_fail, "R MINT04 F9 C2", num_at(&sh, 2, 1), 8.0);
    expect(&g_replay_fail, "R MINT04 F9 B1", num_at(&sh, 1, 0), 2.0);
    expect(&g_replay_fail, "R MINT04 F9 C1", num_at(&sh, 2, 0), 4.0);
    CHECK(!sh.calc, "F9 clears CALC");
    /* columnwise: no CIRC although A2 -> C2 -> B2 -> A2 is a cycle (p. 1-6) */
    if (sh.circ) { fprintf(stderr, "  FAIL R MINT04: CIRC lit in columnwise order\n"); g_circ_fail++; }
    CHECK(!sh.circ, "MINT04: CIRC dark (columnwise)");
    /* a number typed in MANUAL mode recalculates nothing */
    type_number(&sh, 0, 0, 100.0);
    expect(&g_replay_fail, "R manual: B1 keeps its value", num_at(&sh, 1, 0), 2.0);
    CHECK(sh.calc, "R manual: CALC lit");
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <LOTUS123_DECOMP> <wk1_formula_ref.py>\n", argv[0]);
        return 2;
    }
    g_corpus = argv[1];
    g_ref = argv[2];
    {
        static uint8_t nm[I123_CALC_BYTES(16u) + 64u];
        CHECK(i123_calc_init(&g_nest, nm, sizeof nm, 16u, (i123_calc_t *)0) == I123_OK, "nested");
        CHECK(i123_calc_init(&g_cx, g_cmem, sizeof g_cmem, 512u, &g_nest) == I123_OK, "calc init");
    }
    part_o();
    part_i();
    part_c();
    part_r();
    if (g_order_fail) fprintf(stderr, "  RED-REASON order: %d values differ from the manual's order\n", g_order_fail);
    if (g_iter_fail) fprintf(stderr, "  RED-REASON iteration: %d values differ from the manual's passes\n", g_iter_fail);
    if (g_circ_fail) fprintf(stderr, "  RED-REASON circ: %d CIRC indicator states wrong\n", g_circ_fail);
    if (g_replay_fail) fprintf(stderr, "  RED-REASON replay: %d replayed values differ from real 1-2-3\n", g_replay_fail);
    return TEST_SUMMARY("test-i123-recalc-order");
}
