/*
 * harness/diff/wk1_diff/test_i123_wk1_roundtrip.c -- THE ORACLE for the
 * Initech 123 worksheet model and .WK1 codec (factory C; bead initech-9u8w).
 *
 * Graded against two INDEPENDENT authorities (Law 2), never against itself:
 *   (1) the real Lotus 1-2-3 R2.2: the 38 sample sheets shipped on its
 *       distribution and the 6 sheets minted by driving the real 123.EXE under
 *       DOSBox-X, with the screenshots of what it displayed
 *       ($LOTUS123_DECOMP/goldens/..., re/mint-results-001.md);
 *   (2) the corpus' independent reader $LOTUS123_DECOMP/tools/wk1_ref.py,
 *       through wk1_canon.py (it parses no .WK1 bytes itself).
 *
 * PARTS
 *   A  every golden: C read -> C write is BYTE-IDENTICAL to the file 123.EXE
 *      wrote, and the C model's listing equals wk1_ref.py's reading of it.
 *   B  sheets BUILT through the entry API (typed values through the value
 *      parser, formulas carried as hand-assembled RPN) are byte-identical to
 *      the real MINT01 / MINT04 / MINT05 files 123.EXE wrote for the same
 *      entries -- the writer's template, record order, extents, WINDOW1,
 *      settings and the typed-literal INTEGER/NUMBER rule, all at once.
 *   C  a synthetic sheet (IV8192, every label prefix, widths, settings):
 *      written, read by wk1_ref.py (== the C listing), read back by C (==
 *      the C listing: lossless), written again (== the first bytes).
 *   D  displayed values: rows of MINT01/02/03/05 rendered from the goldens
 *      equal the characters on the real 1-2-3 screen (transcribed from
 *      goldens/minted/MINT0x.shots/02_entered.png, positions read off the
 *      80-column grid: 4-char border, then the columns).
 *   E  refusals (Rule 2): malformed files are refused and leave a FRESH sheet.
 *
 * Mutants (Rule 6; test-i123-wk1-roundtrip-mutant):
 *   I123_MUT_LABEL_OPCODE  the writer emits LABEL with STRING's record id ->
 *                          part A rewrite and part C wk1_ref listing go RED.
 *   I123_MUT_GENERAL_ROUNDS  General rounds its last digit instead of
 *                          truncating -> part D goes RED ("3.141593").
 *
 * Usage: test_i123_wk1_roundtrip <LOTUS123_DECOMP> <scratch-dir> <wk1_canon.py>
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

static const char *g_corpus, *g_scratch, *g_canon;
static int g_rewrite_fail, g_ref_fail, g_display_fail;

#define SHEET_MEM (1024u * 1024u)
static uint8_t g_mem[SHEET_MEM];
static uint8_t g_mem2[SHEET_MEM];
static uint8_t g_out[256u * 1024u];
static uint8_t g_out2[256u * 1024u];

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
    if (minted)
        snprintf(out, cap, "%s/goldens/minted/%s.WK1", g_corpus, name);
    else
        snprintf(out, cap, "%s/goldens/lotus-1-2-3-r2.2-allways/wk1-fixtures/%s.WK1",
                 g_corpus, name);
}

/* ---- the C model's listing (the format wk1_canon.py prints) ------------- */
static void hex(FILE *f, const uint8_t *p, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) fprintf(f, "%02x", p[i]);
}

static const char *u8_name(uint16_t op)
{
    switch (op) {
    case WK1_OP_CALCMODE: return "CALCMODE";
    case WK1_OP_CALCORDER: return "CALCORDER";
    case WK1_OP_SPLIT: return "SPLIT";
    case WK1_OP_SYNC: return "SYNC";
    case WK1_OP_PROTEC: return "PROTEC";
    case WK1_OP_LABELFMT: return "LABELFMT";
    case WK1_OP_CALCCOUNT: return "CALCCOUNT";
    case WK1_OP_UNFORMATTED: return "UNFORMATTED";
    default: return 0;
    }
}

static uint8_t u8_val(const i123_sheet_t *sh, uint16_t op)
{
    switch (op) {
    case WK1_OP_CALCMODE: return sh->calcmode;
    case WK1_OP_CALCORDER: return sh->calcorder;
    case WK1_OP_SPLIT: return sh->split;
    case WK1_OP_SYNC: return sh->sync;
    case WK1_OP_PROTEC: return sh->protec;
    case WK1_OP_LABELFMT: return sh->labelfmt;
    case WK1_OP_CALCCOUNT: return sh->calccount;
    default: return sh->unformatted;
    }
}

static uint16_t r16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

static void listing(const i123_sheet_t *sh, FILE *f)
{
    uint32_t p = 0;
    fprintf(f, "BOF 0x%04x\n", sh->bof_version);
    while (p < sh->carry_len) {
        uint16_t op = r16(sh->carry + p), ln = r16(sh->carry + p + 2);
        if (ln != 0xFFFFu) { p += 4u + ln; continue; }
        p += 4u;
        if (u8_name(op)) {
            fprintf(f, "U8 %s %02x\n", u8_name(op), u8_val(sh, op));
        } else if (op == WK1_OP_RANGE) {
            fprintf(f, "RANGE %d %d %d %d\n", sh->range[0], sh->range[1], sh->range[2], sh->range[3]);
        } else if (op == WK1_OP_HIDVEC1) {
            uint8_t body[1536];
            uint32_t n = 0;
            for (int c = 0; c < 256; c++) {
                if (!sh->hid_set[c]) continue;
                body[n++] = (uint8_t)c; body[n++] = 0;
                body[n++] = (uint8_t)(sh->hid_first[c] & 0xFF); body[n++] = (uint8_t)(sh->hid_first[c] >> 8);
                body[n++] = (uint8_t)(sh->hid_last[c] & 0xFF); body[n++] = (uint8_t)(sh->hid_last[c] >> 8);
            }
            fprintf(f, "HIDVEC1 ");
            if (n <= 48u) hex(f, body, n);
            else { hex(f, body, 48u); fprintf(f, "..."); }
            fprintf(f, "\n");
        } else if (op == WK1_OP_WINDOW1) {
            fprintf(f, "WINDOW1 %u %u fmt=%02x colw=%u ncols=%u nrows=%u left=%u top=%u\n",
                    r16(sh->win1 + 0), r16(sh->win1 + 2), sh->win1[4], r16(sh->win1 + 6),
                    i123_cols_fitting(sh, r16(sh->win1 + 12), I123_SCREEN_COLS_CHARS),
                    r16(sh->win1 + 10), r16(sh->win1 + 12), r16(sh->win1 + 14));
            for (int c = 0; c < 256; c++)
                if (sh->colw[c]) fprintf(f, "COLW1 %d %u\n", c, sh->colw[c]);
        }
    }
    for (uint32_t i = 0; i < sh->ncells; i++) {
        const i123_cell_t *c = &sh->cells[i];
        const uint8_t *b = i123_cell_bytes(sh, c);
        fprintf(f, "CELL %u %u ", c->col, c->row);
        switch (c->kind) {
        case I123_K_BLANK: fprintf(f, "BLANK fmt=%02x\n", c->fmt); break;
        case I123_K_INTEGER: fprintf(f, "INT fmt=%02x int=%d\n", c->fmt, (int16_t)r16(c->v)); break;
        case I123_K_NUMBER: fprintf(f, "NUM fmt=%02x num=", c->fmt); hex(f, c->v, 8); fprintf(f, "\n"); break;
        case I123_K_LABEL: fprintf(f, "LABEL fmt=%02x text=", c->fmt); hex(f, b, c->len); fprintf(f, "\n"); break;
        case I123_K_FORMULA:
            fprintf(f, "FORMULA fmt=%02x cache=", c->fmt); hex(f, c->v, 8);
            fprintf(f, " code="); hex(f, b, c->len); fprintf(f, "\n");
            if (c->has_str) {
                fprintf(f, "CELL %u %u STRING fmt=%02x text=", c->col, c->row, c->str_fmt);
                hex(f, b + c->len, c->slen);
                fprintf(f, "\n");
            }
            break;
        default: fprintf(f, "?\n"); break;
        }
    }
}

static int write_listing(const i123_sheet_t *sh, const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    listing(sh, f);
    fclose(f);
    return 1;
}

static int save(const char *path, const uint8_t *b, uint32_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fwrite(b, 1, n, f);
    fclose(f);
    return 1;
}

/* wk1_ref.py's reading of `file` == the C listing in `cdump`. */
static int ref_agrees(const char *file, const char *cdump)
{
    char cmd[2048];
    snprintf(cmd, sizeof cmd, "LOTUS123_DECOMP='%s' python3 '%s' compare '%s' '%s'",
             g_corpus, g_canon, file, cdump);
    return system(cmd) == 0;
}

/* ---- PART A: every golden ------------------------------------------------- */
static void part_a(void)
{
    static i123_sheet_t sh;
    int nfiles = 0, rewrites = 0, agreed = 0;
    for (int minted = 0; minted < 2; minted++) {
        int n = minted ? (int)(sizeof MINTED / sizeof *MINTED) : (int)(sizeof SHIPPED / sizeof *SHIPPED);
        for (int i = 0; i < n; i++) {
            const char *name = minted ? MINTED[i] : SHIPPED[i];
            char path[1024], dump[1024];
            uint32_t len, olen = 0;
            int rc;
            static uint8_t in[256u * 1024u];
            golden_path(path, sizeof path, name, minted);
            len = load(path, in, sizeof in);
            CHECK(len > 0u, "golden present (a missing golden is a RED, never a skip)");
            if (len == 0u) continue;
            nfiles++;
            CHECK(i123_sheet_init(&sh, g_mem, SHEET_MEM, 8192u) == I123_OK, "sheet init");
            rc = i123_wk1_read(&sh, in, len);
            if (rc != I123_OK) {
                fprintf(stderr, "  FAIL read %s rc=%d at offset 0x%x\n", name, rc, sh.err_off);
                CHECK(0, "C reads the golden");
                continue;
            }
            rc = i123_wk1_write(&sh, g_out, sizeof g_out, &olen);
            CHECK(rc == I123_OK, "C writes the golden back");
            if (rc == I123_OK && olen == len && memcmp(g_out, in, len) == 0) {
                rewrites++;
            } else {
                uint32_t k = 0;
                while (k < len && k < olen && g_out[k] == in[k]) k++;
                fprintf(stderr, "  FAIL golden rewrite not byte-identical: %s (len %u vs %u, first diff @0x%x)\n",
                        name, olen, len, k);
                g_rewrite_fail++;
                CHECK(0, "golden rewrite is byte-identical");
            }
            snprintf(dump, sizeof dump, "%s/i123_%s.clist", g_scratch, name);
            CHECK(write_listing(&sh, dump), "write the C listing");
            if (ref_agrees(path, dump)) {
                agreed++;
            } else {
                fprintf(stderr, "  FAIL wk1_ref disagrees with the C model on %s\n", name);
                g_ref_fail++;
                CHECK(0, "C model == wk1_ref reading of the golden");
            }
        }
    }
    printf("  part A: %d goldens; %d rewrites byte-identical; %d agree with wk1_ref.py\n",
           nfiles, rewrites, agreed);
    CHECK(nfiles == 44, "all 44 goldens graded (38 shipped + 6 minted)");
}

/* ---- PART B: entry-built sheets == the real 123.EXE files ----------------- */
static void set_typed(i123_sheet_t *sh, uint16_t c, uint16_t r, const char *typed)
{
    double x = 0.0;
    CHECK(i123_parse_value(typed, (uint32_t)strlen(typed), &x) == I123_OK, typed);
    CHECK(i123_set_number(sh, c, r, x) == I123_OK, "set number");
}

static void set_label(i123_sheet_t *sh, uint16_t c, uint16_t r, char pfx, const char *t)
{
    CHECK(i123_set_label(sh, c, r, (uint8_t)pfx, t, (uint32_t)strlen(t)) == I123_OK, "set label");
}

static void le_double(double d, uint8_t out[8])
{
    memcpy(out, &d, 8);
}

/* A formula carried as 123.EXE compiled it: RPN hand-assembled from
 * specs/formulas/wk1-formula-bytecode.md (tokens 0 f64, 1 VAR, 2 RANGE,
 * 3 RETURN, 5 INT, 6 STRING, 9 +, 11 *, 13 ^, 19 >, 59 @IF, 80 @SUM/n,
 * 81 @AVG/n; a reference word has bit 15 set when relative, the low 14 bits a
 * signed offset). The cache is the value 123.EXE computed (P2 evaluates). */
static void set_formula(i123_sheet_t *sh, uint16_t c, uint16_t r, double cache,
                        const char *code_hex, const char *str)
{
    uint8_t code[128], v[8];
    uint32_t n = (uint32_t)strlen(code_hex) / 2u;
    for (uint32_t i = 0; i < n; i++) {
        unsigned b = 0;
        sscanf(code_hex + 2 * i, "%2x", &b);
        code[i] = (uint8_t)b;
    }
    le_double(cache, v);
    CHECK(i123_set_formula_raw(sh, c, r, WK1_FMT_FRESH_CELL, v, code, n, str,
                               str ? (uint32_t)strlen(str) : 0u) == I123_OK, "set formula");
}

/* NORMALIZATION (stated, the .dbf last-update-date precedent): MINT04's
 * session wrote two FONTSIZE (0x68) records that no entry of the fixture
 * makes -- Allways add-in state carried in the "2.2 with Allways" files
 * (wk1-format.md Sec 5: "later-release style (Allways) data ... Law 3: do not
 * generalise to plain 2.2"). They are removed from the GOLDEN before the
 * compare; nothing else is. A fresh sheet (MINT01, MINT05) has none. */
static uint32_t strip_op(uint8_t *b, uint32_t len, uint16_t op, int *stripped)
{
    uint32_t o = 0, w = 0;
    *stripped = 0;
    while (o + 4u <= len) {
        uint16_t rop = r16(b + o), ln = r16(b + o + 2);
        if (rop == op) { (*stripped)++; o += 4u + ln; continue; }
        memmove(b + w, b + o, 4u + ln);
        w += 4u + ln;
        o += 4u + ln;
    }
    return w;
}

static void cmp_with_golden(const i123_sheet_t *sh, const char *name)
{
    char path[1024];
    static uint8_t in[64u * 1024u];
    uint32_t len, olen = 0;
    int stripped = 0;
    golden_path(path, sizeof path, name, 1);
    len = load(path, in, sizeof in);
    if (strcmp(name, "MINT04") == 0) {
        len = strip_op(in, len, 0x0068u, &stripped);
        CHECK(stripped == 2, "MINT04 carries exactly the two FONTSIZE records the normalization names");
    }
    CHECK(i123_wk1_write(sh, g_out, sizeof g_out, &olen) == I123_OK, "write the built sheet");
    if (!(len > 0u && olen == len && memcmp(g_out, in, len) == 0)) {
        uint32_t k = 0;
        while (k < len && k < olen && g_out[k] == in[k]) k++;
        fprintf(stderr, "  FAIL built sheet != real %s.WK1 (len %u vs %u, first diff @0x%x)\n",
                name, olen, len, k);
        CHECK(0, "entry-built sheet is byte-identical to the 123.EXE file");
    } else {
        printf("  part B: entry-built sheet == real %s.WK1 (%u bytes%s)\n", name, len,
               stripped ? "; its 2 Allways FONTSIZE records normalized out" : "");
    }
}

static void part_b(void)
{
    static i123_sheet_t sh;
    uint8_t nan_cache[8] = { 0xfd, 0x9f, 0x0a, 0x00, 0x00, 0x00, 0xf0, 0x7f };

    /* MINT01 (re/mint-results-001.md; shot MINT01.shots/02_entered.png):
     * A1 Hello, B1 123, C1 1.5, D1 -7; A2 +B1*2, B2 @SUM(B1..D1),
     * C2 @AVG(B1..D1), D2 @IF(B1>100,"big","small"); cursor D2. */
    CHECK(i123_sheet_init(&sh, g_mem, SHEET_MEM, 64u) == I123_OK, "init");
    set_label(&sh, 0, 0, '\'', "Hello");
    set_typed(&sh, 1, 0, "123");
    set_typed(&sh, 2, 0, "1.5");
    set_typed(&sh, 3, 0, "-7");
    set_formula(&sh, 0, 1, 246.0, "010180ffbf0502000b03", 0);
    set_formula(&sh, 1, 1, 117.5, "020080ffbf0280ffbf500103", 0);
    set_formula(&sh, 2, 1, (123.0 + 1.5 - 7.0) / 3.0, "02ffbfffbf0180ffbf510103", 0);
    {
        /* The string formula: its cache is the NaN 123.EXE writes, a raw
         * golden constant (MINT01 @0x613) -- the one value no document gives. */
        uint8_t code[] = { 0x01, 0xfe, 0xbf, 0xff, 0xbf, 0x05, 0x64, 0x00, 0x13,
                           0x06, 'b', 'i', 'g', 0, 0x06, 's', 'm', 'a', 'l', 'l', 0,
                           0x3b, 0x03 };
        CHECK(i123_set_formula_raw(&sh, 3, 1, WK1_FMT_FRESH_CELL, nan_cache, code,
                                   sizeof code, "big", 3u) == I123_OK, "string formula");
    }
    i123_set_cursor(&sh, 3, 1);
    cmp_with_golden(&sh, "MINT01");

    /* MINT04: /wgrm (manual), /wgrc (columnwise), /wgri7, /wgpe (protect);
     * A1 1, B1 +A1+1, C1 +B1*2, A2 +C2+1, B2 +A2, C2 +B2 (a cycle through
     * row 2 -- the iteration case); cursor C2. */
    CHECK(i123_sheet_init(&sh, g_mem, SHEET_MEM, 64u) == I123_OK, "init");
    sh.calcmode = 0x00; sh.calcorder = 0x01; sh.calccount = 7; sh.protec = 0xFF;
    set_typed(&sh, 0, 0, "1");
    set_formula(&sh, 1, 0, 2.0, "01ffbf00800501000903", 0);
    set_formula(&sh, 2, 0, 4.0, "01ffbf00800502000b03", 0);
    set_formula(&sh, 0, 1, 1.0, "01028000800501000903", 0);
    set_formula(&sh, 1, 1, 1.0, "01ffbf008003", 0);
    set_formula(&sh, 2, 1, 1.0, "01ffbf008003", 0);
    i123_set_cursor(&sh, 2, 1);
    cmp_with_golden(&sh, "MINT04");

    /* MINT05: the numeric edges, TYPED (so the value parser and the
     * INTEGER/NUMBER rule are graded too); cursor A15. */
    CHECK(i123_sheet_init(&sh, g_mem, SHEET_MEM, 64u) == I123_OK, "init");
    set_typed(&sh, 0, 0, "1E10");
    set_typed(&sh, 0, 1, "-1E-06");
    set_typed(&sh, 0, 2, "123456789012");
    set_typed(&sh, 0, 3, "1.23456789012345");
    set_formula(&sh, 0, 4, 0.1 + 0.2, "009a9999999999b93f009a9999999999c93f0903", 0);
    set_typed(&sh, 0, 5, "32767");
    set_typed(&sh, 0, 6, "32768");
    set_typed(&sh, 0, 7, "-32768");
    set_typed(&sh, 0, 8, "-32769");
    set_formula(&sh, 0, 9, 9007199254740992.0, "0502000535000d03", 0);
    set_typed(&sh, 0, 10, "1E99");
    set_formula(&sh, 0, 11, 1e100, "002e9f87a2ae427d54050a000b03", 0);
    set_typed(&sh, 0, 12, "1E-99");
    set_typed(&sh, 0, 13, "1E20");
    i123_set_cursor(&sh, 0, 14);
    cmp_with_golden(&sh, "MINT05");
}

/* ---- PART C: a synthetic sheet through wk1_ref and back ------------------- */
static void part_c(void)
{
    static i123_sheet_t a, b;
    uint32_t n1 = 0, n2 = 0;
    char file[1024], dump_a[1024], dump_b[1024];
    char longlab[241];

    CHECK(i123_sheet_init(&a, g_mem, SHEET_MEM, 4096u) == I123_OK, "init a");
    set_label(&a, 0, 0, '\'', "Initech 123 round trip");
    set_label(&a, 1, 1, '"', "Right");
    set_label(&a, 2, 1, '^', "Mid");
    set_label(&a, 3, 1, '\\', "-=");
    set_label(&a, 4, 1, '|', "skip");
    set_label(&a, 5, 1, '\'', "");
    memset(longlab, 'L', 240); longlab[240] = '\0';
    set_label(&a, 6, 2, '\'', longlab);
    set_typed(&a, 0, 3, "0");
    set_typed(&a, 1, 3, "-32767");
    set_typed(&a, 2, 3, "3.14159265358979");
    set_typed(&a, 3, 3, "-2.5E-50");
    set_typed(&a, 4, 3, "6.02E23");
    set_typed(&a, 255, 8191, "42");          /* IV8192, the far corner */
    set_label(&a, 255, 0, '\'', "IV1");
    set_label(&a, 0, 8191, '\'', "A8192");
    CHECK(i123_set_cell_format(&a, 7, 7, 0x22) == I123_OK, "formatted blank");
    CHECK(i123_set_cell_format(&a, 2, 3, 0x83) == I123_OK, "reformat a number");
    CHECK(i123_set_colwidth(&a, 0, 24) == I123_OK, "width A");
    CHECK(i123_set_colwidth(&a, 6, 1) == I123_OK, "width G");
    CHECK(i123_set_colwidth(&a, 255, 72) == I123_OK, "width IV");
    a.calcmode = 0x00; a.calccount = 3; a.protec = 0xFF;
    i123_set_cursor(&a, 255, 8191);
    i123_set_view(&a, 250, 8180);
    /* overwrite + erase exercise the pool */
    set_label(&a, 1, 5, '\'', "temporary");
    CHECK(i123_erase(&a, 1, 5) == I123_OK, "erase");
    set_typed(&a, 1, 6, "7");
    set_label(&a, 1, 6, '\'', "now a label");

    CHECK(i123_wk1_write(&a, g_out, sizeof g_out, &n1) == I123_OK, "write synthetic");
    snprintf(file, sizeof file, "%s/i123_synth.wk1", g_scratch);
    snprintf(dump_a, sizeof dump_a, "%s/i123_synth_a.clist", g_scratch);
    snprintf(dump_b, sizeof dump_b, "%s/i123_synth_b.clist", g_scratch);
    CHECK(save(file, g_out, n1), "save synthetic");
    CHECK(write_listing(&a, dump_a), "listing a");
    if (!ref_agrees(file, dump_a)) {
        g_ref_fail++;
        CHECK(0, "wk1_ref reads the synthetic sheet as the C model holds it");
    }
    CHECK(i123_sheet_init(&b, g_mem2, SHEET_MEM, 4096u) == I123_OK, "init b");
    CHECK(i123_wk1_read(&b, g_out, n1) == I123_OK, "C reads its own file");
    CHECK(write_listing(&b, dump_b), "listing b");
    {
        char cmd[2200];
        snprintf(cmd, sizeof cmd, "cmp -s '%s' '%s'", dump_a, dump_b);
        CHECK(system(cmd) == 0, "lossless: the read-back sheet lists identically");
    }
    CHECK(i123_wk1_write(&b, g_out2, sizeof g_out2, &n2) == I123_OK, "rewrite");
    CHECK(n1 == n2 && memcmp(g_out, g_out2, n1) == 0, "write(read(write(S))) == write(S)");
    {
        const i123_cell_t *c = i123_find(&b, 255, 8191);
        CHECK(c && c->kind == I123_K_INTEGER && r16(c->v) == 42, "IV8192 survives");
        c = i123_find(&b, 1, 5);
        CHECK(c == 0, "the erased cell stays erased");
        c = i123_find(&b, 6, 2);
        CHECK(c && c->len == 241u, "a 240-character label survives");
    }
    printf("  part C: synthetic sheet (%u bytes) read by wk1_ref.py, read back losslessly, rewritten identically\n", n1);
}

/* ---- PART D: what the real 1-2-3 screen showed ---------------------------- */
static void expect_row(const i123_sheet_t *sh, uint16_t row, const char *const cols[],
                       int ncols, uint32_t nchars, const char *what)
{
    char want[256], got[256];
    uint32_t n = 0;
    for (int i = 0; i < ncols; i++) {
        size_t k = strlen(cols[i]);
        memcpy(want + n, cols[i], k);
        n += (uint32_t)k;
    }
    while (n < nchars) want[n++] = ' ';
    want[n] = '\0';
    i123_render_row(sh, row, 0, nchars, got);
    if (strcmp(got, want) != 0) {
        fprintf(stderr, "  FAIL display %s row %u\n        got : [%s]\n        want: [%s]\n",
                what, row + 1u, got, want);
        g_display_fail++;
        CHECK(0, "rendered row == the real 1-2-3 screen");
    } else {
        g_checks++;
    }
}

static void load_minted(i123_sheet_t *sh, const char *name)
{
    char path[1024];
    static uint8_t in[64u * 1024u];
    uint32_t len;
    golden_path(path, sizeof path, name, 1);
    len = load(path, in, sizeof in);
    CHECK(i123_sheet_init(sh, g_mem, SHEET_MEM, 512u) == I123_OK, "init");
    CHECK(i123_wk1_read(sh, in, len) == I123_OK, name);
}

static void part_d(void)
{
    static i123_sheet_t sh;
    int before = g_display_fail;

    /* MINT01.shots/02_entered.png -- 8 columns of 9 = 72 characters */
    load_minted(&sh, "MINT01");
    { const char *r[] = { "Hello    ", "     123 ", "     1.5 ", "      -7 " };
      expect_row(&sh, 0, r, 4, 72, "MINT01"); }
    { const char *r[] = { "     246 ", "   117.5 ", "39.16666 ", "big      " };
      expect_row(&sh, 1, r, 4, 72, "MINT01"); }

    /* MINT02.shots/02_entered.png -- A is 12 wide; 7 whole columns (66) */
    load_minted(&sh, "MINT02");
    { const char *r[] = { "Sales       " };        expect_row(&sh, 0, r, 1, 72, "MINT02"); }
    { const char *r[] = { "   1234.500 " };        expect_row(&sh, 1, r, 1, 72, "MINT02"); }
    { const char *r[] = { "       12.5%" };        expect_row(&sh, 2, r, 1, 72, "MINT02"); }
    { const char *r[] = { "  09-Mar-92 " };        expect_row(&sh, 3, r, 1, 72, "MINT02"); }
    { const char *r[] = { "  $1,234.63 ", "Note     " }; expect_row(&sh, 4, r, 2, 72, "MINT02"); }
    { const char *r[] = { "      Right " };        expect_row(&sh, 5, r, 1, 72, "MINT02"); }

    /* MINT03.shots/02_entered.png -- column A, rows 1..19 */
    load_minted(&sh, "MINT03");
    {
        static const char *const a[19] = {
            "3.141592 ", "1.414213 ", "       3 ", "      -3 ", "      -7 ",
            "       1 ", "      -1 ", "       3 ", "AB       ", "0.33333  ",
            "       1 ", "      NA ", "     ERR ", "     ERR ", "b        ",
            "       6 ", "2.718281 ", "2.302585 ", "    12.5 " };
        for (uint16_t r = 0; r < 19; r++) { const char *x[] = { a[r] }; expect_row(&sh, r, x, 1, 72, "MINT03"); }
    }

    /* MINT05.shots/02_entered.png -- column A, rows 1..14 */
    load_minted(&sh, "MINT05");
    {
        static const char *const a[14] = {
            " 1.0E+10 ", "-0.00000 ", " 1.2E+11 ", "1.234567 ", "     0.3 ",
            "   32767 ", "   32768 ", "  -32768 ", "  -32769 ", " 9.0E+15 ",
            " 1.0E+99 ", "*********", " 1.0E-99 ", " 1.0E+20 " };
        for (uint16_t r = 0; r < 14; r++) { const char *x[] = { a[r] }; expect_row(&sh, r, x, 1, 72, "MINT05"); }
    }
    printf("  part D: %d displayed-value rows checked against the minted screenshots (%d wrong)\n",
           2 + 6 + 19 + 14, g_display_fail - before);

    /* the control panel and the address helpers */
    {
        char t[64];
        load_minted(&sh, "MINT02");
        i123_cell_content(&sh, i123_find(&sh, 1, 4), t, sizeof t);
        CHECK(strcmp(t, "'Note") == 0, "line 1 of B5 is 'Note (MINT02 shot: \"B5: 'Note\")");
        i123_addr_text(1, 4, t);   CHECK(strcmp(t, "B5") == 0, "B5");
        i123_addr_text(255, 8191, t); CHECK(strcmp(t, "IV8192") == 0, "IV8192");
        i123_addr_text(26, 0, t);  CHECK(strcmp(t, "AA1") == 0, "AA1");
    }
}

/* ---- PART E: refusals ------------------------------------------------------ */
static void expect_refused(const uint8_t *buf, uint32_t len, int want, const char *what)
{
    static i123_sheet_t sh;
    int rc;
    CHECK(i123_sheet_init(&sh, g_mem, SHEET_MEM, 64u) == I123_OK, "init");
    i123_set_number(&sh, 0, 0, 5.0);   /* must be gone after the refusal */
    rc = i123_wk1_read(&sh, buf, len);
    if (rc != want) fprintf(stderr, "  FAIL refusal %s: rc=%d want %d\n", what, rc, want);
    CHECK(rc == want, what);
    CHECK(sh.ncells == 0u && sh.carry_cellpos == sh.carry_len, "refused -> a fresh sheet");
}

static void part_e(void)
{
    char path[1024];
    static uint8_t in[64u * 1024u], bad[64u * 1024u];
    uint32_t len;
    golden_path(path, sizeof path, "MINT01", 1);
    len = load(path, in, sizeof in);

    expect_refused(in, len - 4u, I123_E_FORMAT, "no EOF");
    memcpy(bad, in, len); bad[len] = 0;
    expect_refused(bad, len + 1u, I123_E_FORMAT, "a byte after EOF");
    memcpy(bad, in, len); bad[4] = 0x04;
    expect_refused(bad, len, I123_E_FORMAT, "BOF 0x0404 (Release 2.01) is not Release 2.2");
    /* a second CALCMODE: splice MINT01's own (@0x33, 5 bytes) after BOF */
    memcpy(bad, in, 6); memcpy(bad + 6, in + 0x33, 5); memcpy(bad + 11, in + 6, len - 6);
    expect_refused(bad, len + 5u, I123_E_DUP, "a model-owned record twice");
    /* the STRING record without its FORMULA: drop the formula @0x60e (42 B) */
    memcpy(bad, in, 0x60e); memcpy(bad + 0x60e, in + 0x638, len - 0x638);
    expect_refused(bad, len - 0x2a, I123_E_FORMAT, "a STRING not after its FORMULA");
    /* LABEL "Hello" without its NUL (@0x57c: last body byte 0 -> 'x') */
    memcpy(bad, in, len); bad[0x57c + 4 + 11] = 'x';
    expect_refused(bad, len, I123_E_FORMAT, "a LABEL without its NUL");
    /* row 8192 (@0x58c INTEGER row word) */
    memcpy(bad, in, len); bad[0x58c + 4 + 3] = 0x00; bad[0x58c + 4 + 4] = 0x20;
    expect_refused(bad, len, I123_E_FORMAT, "a cell off the 8192-row sheet");
    /* the same cell twice: B1's INTEGER (11 B @0x58c) spliced in again */
    memcpy(bad, in, 0x597); memcpy(bad + 0x597, in + 0x58c, 11); memcpy(bad + 0x5a2, in + 0x597, len - 0x597);
    expect_refused(bad, len + 11u, I123_E_DUP, "one cell twice");
    {
        double x;
        CHECK(i123_parse_value("1E300", 5, &x) == I123_E_VALUE, "1E300 refused (MINT05 parser beep)");
        CHECK(i123_parse_value("+B1*2", 5, &x) == I123_E_VALUE, "a formula is P2: refused");
        CHECK(i123_parse_value("12.5", 4, &x) == I123_OK && x == 12.5, "12.5");
        CHECK(i123_parse_value(".5", 2, &x) == I123_OK && x == 0.5, ".5");
        CHECK(i123_parse_value("1e", 2, &x) == I123_E_VALUE, "1e without digits");
    }
    printf("  part E: malformed files refused fail-loud, sheet left fresh\n");
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: %s <LOTUS123_DECOMP> <scratch-dir> <wk1_canon.py>\n", argv[0]);
        return 2;
    }
    g_corpus = argv[1];
    g_scratch = argv[2];
    g_canon = argv[3];
    part_a();
    part_b();
    part_c();
    part_d();
    part_e();
    if (g_rewrite_fail) fprintf(stderr, "  RED-REASON golden-rewrite: %d goldens not byte-identical\n", g_rewrite_fail);
    if (g_ref_fail) fprintf(stderr, "  RED-REASON wk1-ref-disagrees: %d listings differ from wk1_ref.py\n", g_ref_fail);
    if (g_display_fail) fprintf(stderr, "  RED-REASON display: %d rows differ from the 1-2-3 screen\n", g_display_fail);
    return TEST_SUMMARY("test-i123-wk1-roundtrip");
}
