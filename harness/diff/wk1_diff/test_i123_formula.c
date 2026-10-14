/*
 * harness/diff/wk1_diff/test_i123_formula.c -- THE ORACLE for the Initech 123
 * formula engine: compiler, decompiler, evaluator, @functions and natural-
 * order recalculation (factory C; bead initech-94ah, I123 P2).
 *
 * Graded against INDEPENDENT authorities only (Law 2):
 *   (1) the BYTES the real Lotus 1-2-3 R2.2 compiled and the VALUES it
 *       cached: the 1090 FORMULA records of the 44 goldens in
 *       $LOTUS123_DECOMP/goldens (38 shipped sample sheets + 6 minted);
 *   (2) the TEXT typed to mint them ($LOTUS123_DECOMP/mint/NAME.steps);
 *   (3) the corpus' independent reader tools/wk1_ref.py, through
 *       wk1_formula_ref.py, which supplies every expected cached value
 *       (it parses no bytes itself);
 *   (4) the manual's precedence table (R2.2 Reference p. 1-30, PDF 47),
 *       re-typed HERE for the property generator, never read from
 *       spec/i123/wk1_formula.h.
 *
 * PARTS
 *   A  TYPED: the 37 formulas typed into the minted sheets compile to the
 *      exact bytes 123.EXE wrote, and decompile back to the typed text.
 *   B  ROUND TRIP: all 1090 golden code streams pass the structural check,
 *      decompile, and recompile to the same bytes (or, for the relative
 *      column offsets 123.EXE wrote modulo 256, to the same references).
 *   C  VALUES, LOCAL: every formula evaluated with its sheet's stored values
 *      as inputs reproduces the value real 1-2-3 cached -- bit for bit for
 *      numbers. Expected exceptions are named: CIRC01's circular cells and
 *      MINT04's manual-mode stale cell (re/mint-results-001.md).
 *   D  VALUES, FROM SCRATCH: every automatic natural-order golden with its
 *      formula caches POISONED, recalculated by i123_recalc, and written,
 *      is byte-identical to the file 123.EXE wrote.
 *   E  PROPERTY: 4000 random formulas from an independent generator: the
 *      compiler emits the generator's own RPN, and decompile -> compile is
 *      the identity.
 *
 * Mutants (Rule 6; test-i123-formula-mutant):
 *   I123_MUT_PREC_FLAT   one precedence for every operator -> B and E RED
 *   I123_MUT_ROUND_EVEN  @ROUND rounds halves to even  -> A/C RED (MINT03)
 *   I123_MUT_CACHE_ROUND the stored cache is rounded    -> D RED
 *
 * Usage: test_i123_formula <LOTUS123_DECOMP> <scratch-dir> <wk1_formula_ref.py>
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

static const char *g_corpus, *g_scratch, *g_ref;
static int g_typed_fail, g_rt_fail, g_val_fail, g_scratch_fail, g_prop_fail;

#define SHEET_MEM (1024u * 1024u)
static uint8_t g_mem[SHEET_MEM];
static uint8_t g_cmem[I123_CALC_BYTES(8192u) + 64u];
static i123_calc_t g_cx, g_cx_nested;
static uint8_t g_in[256u * 1024u], g_out[256u * 1024u];

static const char *SHIPPED[] = {
    "AWSAMPLE", "CAMBRIDG", "LONDON", "MONTREAL", "NEWYORK", "QSSOURCE",
    "QSTARGET", "QSTART1X", "QSTART2X", "QSTART4X", "QSTART5X", "SAMP1X",
    "SAMP2X", "SAMP3X", "SAMP4X", "SAMP6X", "SAMP7X", "SAMPBAL", "SAMPCASH",
    "SAMPINC", "SAMPMACS", "TUTOR10X", "TUTOR11X", "TUTOR12X", "TUTOR13X",
    "TUTOR14X", "TUTOR15X", "TUTOR17X", "TUTOR18X", "TUTOR19X", "TUTOR20X",
    "TUTOR2X", "TUTOR4X", "TUTOR5X", "TUTOR6X", "TUTOR7X", "TUTOR8X", "TUTOR9X"
};
static const char *MINTED[] = { "CIRC01", "MINT01", "MINT02", "MINT03", "MINT04", "MINT05" };

static uint32_t load(const char *path, uint8_t *buf, uint32_t cap)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f) { fprintf(stderr, "  cannot open %s\n", path); return 0; }
    n = fread(buf, 1, cap, f);
    fclose(f);
    return (uint32_t)n;
}

static void golden_path(char *out, size_t cap, const char *name, int minted)
{
    if (minted) snprintf(out, cap, "%s/goldens/minted/%s.WK1", g_corpus, name);
    else snprintf(out, cap, "%s/goldens/lotus-1-2-3-r2.2-allways/wk1-fixtures/%s.WK1", g_corpus, name);
}

static int read_golden(i123_sheet_t *sh, const char *name, int minted, uint32_t *len)
{
    char path[1024];
    golden_path(path, sizeof path, name, minted);
    *len = load(path, g_in, sizeof g_in);
    if (*len == 0u) return 0;
    if (i123_sheet_init(sh, g_mem, SHEET_MEM, 8192u) != I123_OK) return 0;
    return i123_wk1_read(sh, g_in, *len) == I123_OK;
}

static void hexs(char *out, const uint8_t *p, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) sprintf(out + 2 * i, "%02x", p[i]);
    out[2 * n] = '\0';
}

/* ---- PART A: the typed formulas ------------------------------------------ */
typedef struct { const char *file, *steps; uint16_t col, row; const char *typed; } typed_t;

/* Transcribed from $LOTUS123_DECOMP/mint/NAME.steps (each text is checked
 * below to occur there as one entry); cells per the steps' cursor moves
 * (each checked to hold a FORMULA). CIRC01 is the MINT01 formulas typed
 * into row 1 (re/mint-results-001.md). */
static const typed_t TYPED[] = {
    { "MINT01", "mint01.steps", 0, 1, "+B1*2" },
    { "MINT01", "mint01.steps", 1, 1, "@SUM(B1..D1)" },
    { "MINT01", "mint01.steps", 2, 1, "@AVG(B1..D1)" },
    { "MINT01", "mint01.steps", 3, 1, "@IF(B1>100,\"big\",\"small\")" },
    { "CIRC01", "mint01.steps", 0, 0, "+B1*2" },
    { "CIRC01", "mint01.steps", 1, 0, "@SUM(B1..D1)" },
    { "CIRC01", "mint01.steps", 2, 0, "@AVG(B1..D1)" },
    { "CIRC01", "mint01.steps", 3, 0, "@IF(B1>100,\"big\",\"small\")" },
    { "MINT02", "MINT02.steps", 0, 3, "@DATE(92,3,9)" },
    { "MINT02", "MINT02.steps", 0, 4, "@SUM(A2..A3)" },
    { "MINT03", "MINT03.steps", 0, 0, "@PI" },
    { "MINT03", "MINT03.steps", 0, 1, "@SQRT(2)" },
    { "MINT03", "MINT03.steps", 0, 2, "@ROUND(2.5,0)" },
    { "MINT03", "MINT03.steps", 0, 3, "@ROUND(-2.5,0)" },
    { "MINT03", "MINT03.steps", 0, 4, "@INT(-7.5)" },
    { "MINT03", "MINT03.steps", 0, 5, "@MOD(7,3)" },
    { "MINT03", "MINT03.steps", 0, 6, "@MOD(-7,3)" },
    { "MINT03", "MINT03.steps", 0, 7, "@LENGTH(\"abc\")" },
    { "MINT03", "MINT03.steps", 0, 8, "@UPPER(\"ab\")" },
    { "MINT03", "MINT03.steps", 0, 9, "@STRING(1/3,5)" },
    { "MINT03", "MINT03.steps", 0, 10, "@TRUE" },
    { "MINT03", "MINT03.steps", 0, 11, "@NA" },
    { "MINT03", "MINT03.steps", 0, 12, "@ERR" },
    { "MINT03", "MINT03.steps", 0, 13, "1/0" },
    { "MINT03", "MINT03.steps", 0, 14, "@CHOOSE(1,\"a\",\"b\",\"c\")" },
    { "MINT03", "MINT03.steps", 0, 15, "@ABS(-3)+@MAX(1,2,3)" },
    { "MINT03", "MINT03.steps", 0, 16, "@EXP(1)" },
    { "MINT03", "MINT03.steps", 0, 17, "@LN(10)" },
    { "MINT03", "MINT03.steps", 0, 18, "@VALUE(\"12.5\")" },
    { "MINT04", "MINT04.steps", 1, 0, "+A1+1" },
    { "MINT04", "MINT04.steps", 2, 0, "+B1*2" },
    { "MINT04", "MINT04.steps", 0, 1, "+C2+1" },
    { "MINT04", "MINT04.steps", 1, 1, "+A2" },
    { "MINT04", "MINT04.steps", 2, 1, "+B2" },
    { "MINT05", "MINT05.steps", 0, 4, "0.1+0.2" },
    { "MINT05", "MINT05.steps", 0, 9, "2^53" },
    { "MINT05", "MINT05.steps", 0, 11, "1E99*10" },
};

/* Does `text` occur as one typed entry of the steps file (consecutive
 * `type` lines joined until a `key` line)? */
static int typed_in_steps(const char *steps, const char *text)
{
    char path[1024], line[512], entry[1024];
    FILE *f;
    int found = 0;
    size_t n = 0;
    snprintf(path, sizeof path, "%s/mint/%s", g_corpus, steps);
    f = fopen(path, "r");
    if (!f) { fprintf(stderr, "  cannot open %s\n", path); return 0; }
    entry[0] = '\0';
    while (fgets(line, sizeof line, f)) {
        size_t l = strlen(line);
        while (l && (line[l - 1] == '\n' || line[l - 1] == '\r')) line[--l] = '\0';
        if (strncmp(line, "type ", 5) == 0) {
            if (n + (l - 5) < sizeof entry - 1) { memcpy(entry + n, line + 5, l - 5); n += l - 5; entry[n] = '\0'; }
        } else if (strncmp(line, "key ", 4) == 0) {
            if (strcmp(entry, text) == 0) found = 1;
            n = 0; entry[0] = '\0';
        }
    }
    fclose(f);
    return found;
}

/* The control-panel form of typed text: references and names uppercased,
 * literal strings kept. */
static void canon_typed(const char *t, char *out)
{
    int q = 0;
    for (; *t; t++) {
        char ch = *t;
        if (ch == '"') q = !q;
        if (!q && ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
        *out++ = ch;
    }
    *out = '\0';
}

static void part_a(void)
{
    static i123_sheet_t sh;
    int ok = 0, n = (int)(sizeof TYPED / sizeof *TYPED);
    for (int i = 0; i < n; i++) {
        const typed_t *t = &TYPED[i];
        const i123_cell_t *c;
        uint8_t code[I123_CODE_MAX];
        uint32_t len, cl = 0;
        char text[I123_FTEXT_MAX], want[256], hx[2 * I123_CODE_MAX + 1], hy[2 * I123_CODE_MAX + 1];
        int good = 1;
        if (!typed_in_steps(t->steps, t->typed)) {
            fprintf(stderr, "  FAIL A: \"%s\" is not an entry of mint/%s\n", t->typed, t->steps);
            good = 0;
        }
        if (!read_golden(&sh, t->file, 1, &len)) { CHECK(0, "read minted golden"); g_typed_fail++; continue; }
        c = i123_find(&sh, t->col, t->row);
        if (c == (const i123_cell_t *)0 || c->kind != I123_K_FORMULA) {
            fprintf(stderr, "  FAIL A: %s (%u,%u) is not a FORMULA\n", t->file, t->col, t->row);
            g_typed_fail++; CHECK(0, "typed cell is a formula"); continue;
        }
        if (i123_formula_compile(t->typed, (uint32_t)strlen(t->typed), t->col, t->row,
                                 code, sizeof code, &cl) != I123_OK) {
            fprintf(stderr, "  FAIL A: \"%s\" does not compile\n", t->typed);
            good = 0;
        } else if (cl != c->len || memcmp(code, i123_cell_bytes(&sh, c), cl) != 0) {
            hexs(hx, code, cl);
            hexs(hy, i123_cell_bytes(&sh, c), c->len);
            fprintf(stderr, "  FAIL A: %s \"%s\"\n        compiled: %s\n        123.EXE : %s\n",
                    t->file, t->typed, hx, hy);
            good = 0;
        }
        canon_typed(t->typed, want);
        if (i123_formula_text(i123_cell_bytes(&sh, c), c->len, t->col, t->row, text, sizeof text) != I123_OK ||
            strcmp(text, want) != 0) {
            fprintf(stderr, "  FAIL A: %s (%u,%u) decompiles to \"%s\", typed \"%s\"\n",
                    t->file, t->col, t->row, text, want);
            good = 0;
        }
        CHECK(good, "typed formula compiles to 123.EXE's bytes and decompiles to its text");
        if (good) ok++; else g_typed_fail++;
    }
    printf("  part A: %d/%d typed formulas compile to the bytes 123.EXE wrote and decompile to the typed text\n", ok, n);
}

/* ---- PART B: round trip of every golden code stream ---------------------- */

/* Rewrite every relative COLUMN offset to its in-range form (the target
 * column is the same, [C] Sec 2.1), for the canonical comparison. */
static void canon_code(uint8_t *code, uint32_t len, uint16_t col)
{
    uint32_t i = 0;
    while (i < len) {
        uint32_t n = i123_token_size(code, len, i);
        uint8_t t = code[i];
        if (n == 0u) return;
        if (t == 1u || t == 2u) {
            for (uint32_t r = 0; r < (t == 1u ? 1u : 2u); r++) {
                uint8_t *p = code + i + 1u + 4u * r;
                uint16_t w = (uint16_t)(p[0] | (p[1] << 8));
                if (w & 0x8000u) {
                    int32_t target = i123_ref_resolve(w, col, 1);
                    uint16_t nw = (uint16_t)(0x8000u | ((uint32_t)(target - (int32_t)col) & 0x3FFFu));
                    p[0] = (uint8_t)(nw & 0xFFu); p[1] = (uint8_t)(nw >> 8);
                }
            }
        }
        i += n;
    }
}

static void part_b(void)
{
    static i123_sheet_t sh;
    int total = 0, exact = 0, canon = 0, checked = 0;
    for (int minted = 0; minted < 2; minted++) {
        int nf = minted ? 6 : (int)(sizeof SHIPPED / sizeof *SHIPPED);
        for (int f = 0; f < nf; f++) {
            const char *name = minted ? MINTED[f] : SHIPPED[f];
            uint32_t len;
            if (!read_golden(&sh, name, minted, &len)) { CHECK(0, "read golden"); g_rt_fail++; continue; }
            for (uint32_t i = 0; i < sh.ncells; i++) {
                const i123_cell_t *c = &sh.cells[i];
                const uint8_t *code = i123_cell_bytes(&sh, c);
                uint8_t re[I123_CODE_MAX], a[I123_CODE_MAX];
                char text[I123_FTEXT_MAX];
                uint32_t rl = 0;
                if (c->kind != I123_K_FORMULA) continue;
                total++;
                if (i123_formula_check(code, c->len) == I123_OK) checked++;
                if (i123_formula_text(code, c->len, c->col, c->row, text, sizeof text) != I123_OK) {
                    fprintf(stderr, "  FAIL B: %s (%u,%u) does not decompile\n", name, c->col, c->row);
                    g_rt_fail++; continue;
                }
                if (i123_formula_compile(text, (uint32_t)strlen(text), c->col, c->row, re, sizeof re, &rl) != I123_OK) {
                    fprintf(stderr, "  FAIL B: %s (%u,%u) \"%s\" does not recompile\n", name, c->col, c->row, text);
                    g_rt_fail++; continue;
                }
                if (rl == c->len && memcmp(re, code, rl) == 0) { exact++; continue; }
                memcpy(a, code, c->len);
                canon_code(a, c->len, c->col);
                if (rl == c->len && memcmp(re, a, rl) == 0) { canon++; continue; }
                {
                    char hx[2 * I123_CODE_MAX + 1], hy[2 * I123_CODE_MAX + 1];
                    hexs(hx, re, rl); hexs(hy, code, c->len);
                    fprintf(stderr, "  FAIL B: %s (%u,%u) \"%s\"\n        recompiled: %s\n        123.EXE   : %s\n",
                            name, c->col, c->row, text, hx, hy);
                }
                g_rt_fail++;
            }
        }
    }
    CHECK(total == 1090, "all 1090 golden FORMULA records graded");
    CHECK(checked == total, "every golden code stream passes the structural check");
    CHECK(exact + canon == total, "every golden formula round-trips");
    printf("  part B: %d golden formulas: %d pass the structural check; text round trip %d byte-exact + %d same references (123.EXE's modulo-256 column offsets)\n",
           total, checked, exact, canon);
}

/* ---- PART C: local values against wk1_ref's reading of the caches -------- */
typedef struct { uint16_t c, r; char kind[4]; char payload[2 * I123_STR_MAX + 20]; } ref_t;
static ref_t g_refs[2048];

static int load_refs(const char *path, int *n)
{
    char cmd[2048], line[1024];
    FILE *p;
    *n = 0;
    snprintf(cmd, sizeof cmd, "LOTUS123_DECOMP='%s' python3 '%s' '%s'", g_corpus, g_ref, path);
    p = popen(cmd, "r");
    if (!p) return 0;
    while (fgets(line, sizeof line, p) && *n < 2048) {
        ref_t *r = &g_refs[*n];
        unsigned c, rr;
        r->payload[0] = '\0';
        if (sscanf(line, "F %u %u %3s %1000s", &c, &rr, r->kind, r->payload) >= 3) {
            r->c = (uint16_t)c; r->r = (uint16_t)rr;
            (*n)++;
        }
    }
    return pclose(p) == 0;
}

static void val_text(const i123_val_t *v, char *kind, char *payload)
{
    payload[0] = '\0';
    switch (v->t) {
    case I123_V_NUM: { uint8_t b[8]; memcpy(b, &v->n, 8); strcpy(kind, "NUM"); hexs(payload, b, 8); break; }
    case I123_V_ERR: strcpy(kind, "ERR"); break;
    case I123_V_NA:  strcpy(kind, "NA");  break;
    case I123_V_STR: strcpy(kind, "STR"); hexs(payload, v->s, v->slen); break;
    default:         strcpy(kind, "???"); break;
    }
}

/* The cells whose cache 1-2-3 did NOT compute from its current inputs, by
 * design of the mint (re/mint-results-001.md; wk1-formula-bytecode.md
 * Sec 6): CIRC01's circular cells (iterated, not a fixed point) and
 * MINT04's A2 (manual mode, stale). */
static int expected_exception(const char *name, uint16_t c, uint16_t r)
{
    if (strcmp(name, "CIRC01") == 0) return r == 0 && (c == 1 || c == 2);
    if (strcmp(name, "MINT04") == 0) return c == 0 && r == 1;
    return 0;
}

static int is_link(const i123_sheet_t *sh, const i123_cell_t *c)
{
    const uint8_t *b = i123_cell_bytes(sh, c);
    return c->len >= 4u && b[0] == 6u && b[1] == '<' && b[2] == '<' && b[c->len - 2u] == 115u;
}

static void part_c(void)
{
    static i123_sheet_t sh;
    int total = 0, same = 0, links = 0, excepted = 0, num = 0, str = 0, err = 0;
    for (int minted = 0; minted < 2; minted++) {
        int nf = minted ? 6 : (int)(sizeof SHIPPED / sizeof *SHIPPED);
        for (int f = 0; f < nf; f++) {
            const char *name = minted ? MINTED[f] : SHIPPED[f];
            char path[1024];
            uint32_t len;
            int nref = 0, k = 0;
            golden_path(path, sizeof path, name, minted);
            if (!read_golden(&sh, name, minted, &len) || !load_refs(path, &nref)) {
                CHECK(0, "read golden + wk1_ref values"); g_val_fail++; continue;
            }
            for (uint32_t i = 0; i < sh.ncells; i++) {
                const i123_cell_t *c = &sh.cells[i];
                i123_val_t v;
                char kind[4], payload[2 * I123_STR_MAX + 20];
                if (c->kind != I123_K_FORMULA) continue;
                total++;
                if (k >= nref || g_refs[k].c != c->col || g_refs[k].r != c->row) {
                    fprintf(stderr, "  FAIL C: %s formula (%u,%u) has no wk1_ref value in order\n", name, c->col, c->row);
                    g_val_fail++; k++; continue;
                }
                if (i123_eval(&g_cx, &sh, c->col, c->row, i123_cell_bytes(&sh, c), c->len, &v) != I123_OK) {
                    strcpy(kind, "!!!"); payload[0] = '\0';
                } else {
                    val_text(&v, kind, payload);
                }
                if (is_link(&sh, c)) { links++; k++; continue; }   /* keeps its cache: no evidence */
                if (strcmp(kind, g_refs[k].kind) == 0 && strcmp(payload, g_refs[k].payload) == 0) {
                    same++;
                    if (kind[0] == 'N' && kind[1] == 'U') num++;
                    else if (kind[0] == 'S') str++;
                    else err++;
                } else if (expected_exception(name, c->col, c->row)) {
                    excepted++;
                } else {
                    char text[I123_FTEXT_MAX];
                    if (i123_formula_text(i123_cell_bytes(&sh, c), c->len, c->col, c->row, text, sizeof text) != I123_OK)
                        strcpy(text, "?");
                    fprintf(stderr, "  FAIL C: %s (%u,%u) %s -> %s %s, 1-2-3 cached %s %s\n",
                            name, c->col, c->row, text, kind, payload, g_refs[k].kind, g_refs[k].payload);
                    g_val_fail++;
                }
                k++;
            }
        }
    }
    CHECK(g_val_fail == 0, "every formula reproduces the value real 1-2-3 cached");
    printf("  part C: %d formulas: %d reproduce the real 1-2-3 cache exactly (%d numbers bit-for-bit, %d strings, %d ERR/NA); %d file links keep their value (no evidence); %d named mint exceptions (CIRC01 B1 C1, MINT04 A2)\n",
           total, same, num, str, err, links, excepted);
}

/* ---- PART D: from scratch: poison, recalculate, write == the golden ------- */
static void part_d(void)
{
    static i123_sheet_t sh;
    int files = 0, identical = 0, cells = 0;
    for (int minted = 0; minted < 2; minted++) {
        int nf = minted ? 6 : (int)(sizeof SHIPPED / sizeof *SHIPPED);
        for (int f = 0; f < nf; f++) {
            const char *name = minted ? MINTED[f] : SHIPPED[f];
            uint32_t len, olen = 0;
            int nform = 0;
            static uint8_t orig[256u * 1024u];
            if (!read_golden(&sh, name, minted, &len)) { CHECK(0, "read golden"); g_scratch_fail++; continue; }
            /* recalculated by 123.EXE as saved: automatic, natural, no cycle */
            if (sh.calcmode != 0xFFu || sh.calcorder != 0x00u || strcmp(name, "CIRC01") == 0) continue;
            memcpy(orig, g_in, len);
            for (uint32_t i = 0; i < sh.ncells; i++) {
                i123_cell_t *c = &sh.cells[i];
                if (c->kind != I123_K_FORMULA || is_link(&sh, c)) continue;
                nform++;
                if (c->has_str) {
                    /* keep 123.EXE's private NaN handle, poison the text */
                    uint8_t *t = (uint8_t *)i123_cell_bytes(&sh, c) + c->len;
                    for (uint32_t k = 0; k < c->slen; k++) t[k] = '#';
                } else {
                    double poison = 12345.678;
                    memcpy(c->v, &poison, 8);
                }
            }
            if (nform == 0) continue;
            files++;
            cells += nform;
            if (i123_recalc(&g_cx, &sh) != I123_OK) { CHECK(0, "recalc"); g_scratch_fail++; continue; }
            CHECK(sh.circ == 0u, "no circular reference in an acyclic golden");
            if (i123_wk1_write(&sh, g_out, sizeof g_out, &olen) != I123_OK) { CHECK(0, "write"); g_scratch_fail++; continue; }
            if (olen == len && memcmp(g_out, orig, len) == 0) { identical++; continue; }
            {
                uint32_t k = 0;
                while (k < len && k < olen && g_out[k] == orig[k]) k++;
                fprintf(stderr, "  FAIL D: %s recalculated from poisoned caches is not byte-identical (len %u vs %u, first diff @0x%x)\n",
                        name, olen, len, k);
            }
            g_scratch_fail++;
            CHECK(0, "recalculated golden is byte-identical");
        }
    }
    printf("  part D: %d automatic natural-order goldens, %d formula caches poisoned and recalculated: %d files byte-identical to 123.EXE's\n",
           files, cells, identical);
}

/* ---- PART E: property -- random formulas from an independent generator ---- */
static uint32_t g_seed = 0x123u;
static uint32_t rnd(uint32_t n) { g_seed = g_seed * 1103515245u + 12345u; return (g_seed >> 8) % n; }

typedef struct { char text[1200]; uint8_t rpn[1200]; uint32_t rl; int prec; } gen_t;

/* The manual's table, p. 1-30 (PDF 47), re-typed: lower binds tighter. */
static const struct { const char *op; uint8_t tok; int prec; } BIN[] = {
    { "^", 13, 1 }, { "*", 11, 3 }, { "/", 12, 3 }, { "+", 9, 4 }, { "-", 10, 4 },
    { "=", 14, 5 }, { "<>", 15, 5 }, { "<=", 16, 5 }, { ">=", 17, 5 }, { "<", 18, 5 },
    { ">", 19, 5 }, { "#AND#", 20, 7 }, { "#OR#", 21, 7 }, { "&", 24, 7 },
};

static void g_cat(gen_t *g, const char *s) { strcat(g->text, s); }
static void g_tok(gen_t *g, uint8_t b) { g->rpn[g->rl++] = b; }

static void gen(gen_t *g, int depth, uint16_t hc, uint16_t hr)
{
    uint32_t k = depth <= 0 ? rnd(4) : rnd(9);
    g->text[0] = '\0'; g->rl = 0; g->prec = 0;
    if (k == 0) {                                   /* INT literal */
        uint32_t v = rnd(32768);
        sprintf(g->text, "%u", v);
        g_tok(g, 5); g_tok(g, (uint8_t)(v & 0xFF)); g_tok(g, (uint8_t)(v >> 8));
    } else if (k == 1) {                            /* relative / absolute / mixed cell */
        uint32_t c = rnd(256), r = rnd(8192), ac = rnd(2), ar = rnd(2);
        char nm[4];
        uint16_t cw = ac ? (uint16_t)c : (uint16_t)(0x8000u | ((c - hc) & 0x3FFFu));
        uint16_t rw = ar ? (uint16_t)r : (uint16_t)(0x8000u | ((r - hr) & 0x3FFFu));
        i123_col_name((uint16_t)c, nm);
        sprintf(g->text, "%s%s%s%u", ac ? "$" : "", nm, ar ? "$" : "", r + 1u);
        g_tok(g, 1); g_tok(g, (uint8_t)(cw & 0xFF)); g_tok(g, (uint8_t)(cw >> 8));
        g_tok(g, (uint8_t)(rw & 0xFF)); g_tok(g, (uint8_t)(rw >> 8));
    } else if (k == 2) {                            /* string literal */
        static const char *const S[] = { "abc", "", "Hello World", "x y" };
        const char *s = S[rnd(4)];
        sprintf(g->text, "\"%s\"", s);
        g_tok(g, 6);
        for (const char *p = s; *p; p++) g_tok(g, (uint8_t)*p);
        g_tok(g, 0);
    } else if (k == 3) {                            /* zero-argument function */
        static const struct { const char *n; uint8_t t; } Z[] = { { "@PI", 38 }, { "@NA", 31 }, { "@TRUE", 52 } };
        uint32_t z = rnd(3);
        strcpy(g->text, Z[z].n);
        g_tok(g, Z[z].t);
    } else if (k <= 6) {                            /* binary operator */
        gen_t *a = malloc(sizeof *a), *b = malloc(sizeof *b);
        uint32_t o = rnd(sizeof BIN / sizeof *BIN);
        int p = BIN[o].prec, pl, pr;
        gen(a, depth - 1, hc, hr);
        gen(b, depth - 1, hc, hr);
        /* left needs a group when looser than p; right when not tighter */
        pl = a->prec > p || (a->prec != 0 && rnd(10) == 0);
        pr = (b->prec != 0 && b->prec >= p) || (b->prec != 0 && rnd(10) == 0) || b->prec == 2;
        if (pl) g_cat(g, "(");
        g_cat(g, a->text);
        if (pl) g_cat(g, ")");
        g_cat(g, BIN[o].op);
        if (pr) g_cat(g, "(");
        g_cat(g, b->text);
        if (pr) g_cat(g, ")");
        memcpy(g->rpn, a->rpn, a->rl); g->rl = a->rl;
        if (pl) g_tok(g, 4);
        memcpy(g->rpn + g->rl, b->rpn, b->rl); g->rl += b->rl;
        if (pr) g_tok(g, 4);
        g_tok(g, BIN[o].tok);
        g->prec = p;
        free(a); free(b);
    } else if (k == 7) {                            /* unary minus: tighter than all but ^ */
        gen_t *a = malloc(sizeof *a);
        int paren;
        gen(a, depth - 1, hc, hr);
        paren = a->prec >= 2;
        g_cat(g, "-");
        if (paren) g_cat(g, "(");
        g_cat(g, a->text);
        if (paren) g_cat(g, ")");
        memcpy(g->rpn, a->rpn, a->rl); g->rl = a->rl;
        if (paren) g_tok(g, 4);
        g_tok(g, 8);
        g->prec = 2;
        free(a);
    } else {                                        /* a function call */
        gen_t *a = malloc(sizeof *a), *b = malloc(sizeof *b);
        uint32_t w = rnd(3);
        gen(a, depth - 1, hc, hr);
        gen(b, depth - 1, hc, hr);
        if (w == 0) {
            g_cat(g, "@MOD("); g_cat(g, a->text); g_cat(g, ","); g_cat(g, b->text); g_cat(g, ")");
            memcpy(g->rpn, a->rpn, a->rl); memcpy(g->rpn + a->rl, b->rpn, b->rl);
            g->rl = a->rl + b->rl; g_tok(g, 47);
        } else if (w == 1) {
            g_cat(g, "@MAX("); g_cat(g, a->text); g_cat(g, ","); g_cat(g, b->text); g_cat(g, ")");
            memcpy(g->rpn, a->rpn, a->rl); memcpy(g->rpn + a->rl, b->rpn, b->rl);
            g->rl = a->rl + b->rl; g_tok(g, 84); g_tok(g, 2);
        } else {
            g_cat(g, "@IF("); g_cat(g, a->text); g_cat(g, ","); g_cat(g, b->text); g_cat(g, ",1)");
            memcpy(g->rpn, a->rpn, a->rl); memcpy(g->rpn + a->rl, b->rpn, b->rl);
            g->rl = a->rl + b->rl; g_tok(g, 5); g_tok(g, 1); g_tok(g, 0); g_tok(g, 59);
        }
        g->prec = 0;
        free(a); free(b);
    }
}

static void part_e(void)
{
    int n = 4000, ok = 0, rt = 0, tried = 0;
    for (int i = 0; i < n; i++) {
        gen_t *g = malloc(sizeof *g);
        uint16_t hc = (uint16_t)rnd(256), hr = (uint16_t)rnd(8192);
        uint8_t code[I123_CODE_MAX], re[I123_CODE_MAX];
        char text[1300], back[I123_FTEXT_MAX];
        uint32_t cl = 0, rl = 0;
        gen(g, 4, hc, hr);
        /* the entry: a leading '+' when it would start like a label, and
         * one more when it starts with '+' (the compiler drops one) */
        if ((g->text[0] >= 'A' && g->text[0] <= 'Z') || g->text[0] == '"' || g->text[0] == '$' || g->text[0] == '+')
            snprintf(text, sizeof text, "+%s", g->text);
        else snprintf(text, sizeof text, "%s", g->text);
        g->rpn[g->rl++] = 3;
        if (strlen(text) > 240u) { free(g); continue; }
        tried++;
        if (i123_formula_compile(text, (uint32_t)strlen(text), hc, hr, code, sizeof code, &cl) != I123_OK ||
            cl != g->rl || memcmp(code, g->rpn, cl) != 0) {
            if (g_prop_fail < 5) {
                char hx[2 * I123_CODE_MAX + 1], hy[2 * I123_CODE_MAX + 1];
                hexs(hx, code, cl); hexs(hy, g->rpn, g->rl);
                fprintf(stderr, "  FAIL E: \"%s\" at (%u,%u)\n        compiled : %s\n        generator: %s\n", text, hc, hr, hx, hy);
            }
            g_prop_fail++;
            free(g);
            continue;
        }
        ok++;
        if (i123_formula_text(code, cl, hc, hr, back, sizeof back) == I123_OK &&
            i123_formula_compile(back, (uint32_t)strlen(back), hc, hr, re, sizeof re, &rl) == I123_OK &&
            rl == cl && memcmp(re, code, cl) == 0) rt++;
        else {
            if (g_prop_fail < 5) fprintf(stderr, "  FAIL E: decompile/compile of \"%s\" -> \"%s\" is not the identity\n", text, back);
            g_prop_fail++;
        }
        free(g);
    }
    CHECK(g_prop_fail == 0, "property: compile == generator RPN; decompile->compile == identity");
    printf("  part E: %d random formulas: %d compile to the generator's RPN, %d survive decompile -> compile\n", tried, ok, rt);
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: %s <LOTUS123_DECOMP> <scratch-dir> <wk1_formula_ref.py>\n", argv[0]);
        return 2;
    }
    g_corpus = argv[1];
    g_scratch = argv[2];
    g_ref = argv[3];
#ifdef I123_TEST_X87_CW
    {
        /* the x87 build of this oracle (-mfpmath=387): the tenant's control
         * word under test (ADR-0009 DEC-01a Sec 4) */
        unsigned short cw = (unsigned short)I123_TEST_X87_CW;
        __asm__ __volatile__ ("fldcw %0" : : "m"(cw));
        printf("  x87 arithmetic, control word 0x%04x\n", cw);
    }
#endif
    {
        static uint8_t nested_mem[I123_CALC_BYTES(16u) + 64u];
        CHECK(i123_calc_init(&g_cx_nested, nested_mem, sizeof nested_mem, 16u, (i123_calc_t *)0) == I123_OK, "nested calc");
        CHECK(i123_calc_init(&g_cx, g_cmem, sizeof g_cmem, 8192u, &g_cx_nested) == I123_OK, "calc init");
    }
    part_a();
    part_b();
    part_c();
    part_d();
    part_e();
    if (g_typed_fail) fprintf(stderr, "  RED-REASON typed: %d typed formulas differ from 123.EXE\n", g_typed_fail);
    if (g_rt_fail) fprintf(stderr, "  RED-REASON round-trip: %d golden formulas do not round-trip\n", g_rt_fail);
    if (g_val_fail) fprintf(stderr, "  RED-REASON values: %d formulas differ from the real 1-2-3 cache\n", g_val_fail);
    if (g_scratch_fail) fprintf(stderr, "  RED-REASON recalc-file: %d recalculated goldens differ from 123.EXE's file\n", g_scratch_fail);
    if (g_prop_fail) fprintf(stderr, "  RED-REASON property: %d random formulas fail\n", g_prop_fail);
    return TEST_SUMMARY("test-i123-formula");
}
