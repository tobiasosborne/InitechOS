/*
 * os/iword/core/page.c -- InitechWord's page features: headers / footers,
 * footnotes / endnotes, newspaper columns, tables, their Reveal Codes labels,
 * WordPerfect's bookkeeping in them, and the page stream the Standard Printer
 * prints (THE ARTIFACT; bead initech-8q6j, IWORD P4).
 *
 * GROUND TRUTH. spec/iword/wp5_page.h: every byte, label and printed line
 * there was read from what the REAL WP 5.1 (03-09-92) wrote and printed under
 * DOSBox-X for this bead (../wordperfect51-decomp MINT33..44,
 * PRTMINTnn.TXT; re/mint-results-005.md); the behaviour is the WordPerfect
 * 5.1 Reference's (pp. 74-79, 209-216, 269-274, 490-494, 543, 640-644).
 *
 * The line layout and the pagination (the walk) are edit.c's (iword_int.h);
 * this file builds the codes, keeps their payloads as WP does (iw__p4_fixup,
 * run inside iw_ed_fixup) and prints.
 *
 * Freestanding C (no libc). Deterministic (Rule 11). ASCII-clean (Rule 12).
 */
#include "iword.h"
#include "iword_int.h"
#include "iword/wp5_format.h"
#include "iword/wp5_page.h"
#include "iword/reveal_codes.h"

#define NONE 0xFFFFFFFFu

/* One scratch for the code builders and the printer's runs (never two at
 * once): the tenant's BSS is capped (spec/toolbox_gate.h TBX_TENANT_BSS_MAX). */
#define HF_MAX   1024u
#define NOTE_MAX 1024u
static uint8_t g_scr[IW_WP5_VAR_HDR + IW_HF_HEAD_LEN + HF_MAX + IW_WP5_VAR_TRAILER + 32u];

static uint32_t rd16(const iw_doc_t *d, uint32_t p)
{
    uint8_t g[2] = { 0, 0 };
    (void)iw_doc_read(d, p, g, 2u);
    return (uint32_t)g[0] | ((uint32_t)g[1] << 8);
}

static int byte_at(const iw_doc_t *d, uint32_t p)
{
    return iw_doc_byte(d, p);
}

static void put16(uint8_t *g, uint32_t v)
{
    g[0] = (uint8_t)(v & 0xFFu);
    g[1] = (uint8_t)((v >> 8) & 0xFFu);
}

/* Overwrite bytes that differ (no position moves). */
static int pput(iw_doc_t *d, uint32_t pos, const uint8_t *b, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        int have = iw_doc_byte(d, pos + i);
        if (have < 0) return IW_E_RANGE;
        if ((uint8_t)have != b[i]) {
            int rc = iw_doc_set(d, pos + i, b[i]);
            if (rc != IW_OK) return rc;
        }
    }
    return IW_OK;
}

uint32_t iw__digits(uint32_t n)
{
    uint32_t k = 1;
    while (n >= 10u) { n /= 10u; k++; }
    return k;
}

/* ---- headers and footers ----------------------------------------------------- */
static int hf_applies(uint32_t occ, int odd)
{
    return occ == IW_HF_EVERY || (occ == IW_HF_ODD && odd) || (occ == IW_HF_EVEN && !odd);
}

uint32_t iw__hf_space(const uint8_t *occ, const uint8_t *lines, int footer, int odd)
{
    uint32_t i0 = footer ? IW_HF_FOOTER_A : IW_HF_HEADER_A, i, best = 0;
    int any = 0;
    for (i = i0; i < i0 + 2u; i++) {
        if (!hf_applies(occ[i], odd)) continue;
        any = 1;
        if (lines[i] > best) best = lines[i];
    }
    if (!any) return 0;
#ifdef IWORD_MUT_HF_GAP
    /* MUTANT (Rule 6; test-iword-p4-mutant): no blank line between a header
     * and the text. NEVER real. */
    return best * IW_WP5_WPU_PER_LINE;
#else
    return best * IW_WP5_WPU_PER_LINE + IW_HF_GAP_WPU;
#endif
}

static int is_hf(const iw_doc_t *d, uint32_t p)
{
    int s = byte_at(d, p + 1u);
    return byte_at(d, p) == (int)IW_WP5_HF_GRP && s >= 0 && s < 4;
}

static int hf_disc(const iw_doc_t *d, uint32_t p)
{
    return rd16(d, p + 2u) == IW_HF_DISC_SIZE;
}

uint32_t iw_hf_find(const iw_doc_t *d, uint32_t cur, uint32_t which)
{
    uint32_t p = 0, before = NONE, after = NONE;
    while (p < d->len) {
        if (is_hf(d, p) && (uint32_t)byte_at(d, p + 1u) == which && !hf_disc(d, p)) {
            if (p < cur) before = p;
            else if (after == NONE) after = p;
        }
        p = iw_unit_next(d, p);
    }
    /* "searches backward ... If it does not find one, it then searches
     * forward" (Reference p. 273) */
    return before != NONE ? before : after;
}

uint32_t iw_hf_text(const iw_doc_t *d, uint32_t pos, uint8_t *out, uint32_t cap)
{
    uint32_t size, n, at;
    if (!is_hf(d, pos) || hf_disc(d, pos)) return 0;
    size = rd16(d, pos + 2u);
    n = size - IW_WP5_VAR_TRAILER - IW_HF_HEAD_LEN;
    at = pos + IW_WP5_VAR_HDR + IW_HF_HEAD_LEN;
    return iw_doc_read(d, at, out, n < cap ? n : cap);
}

void iw_hf_at(const iw_doc_t *d, uint32_t pos, iw_hfstate_t *s)
{
    fmt_t f;
    uint32_t i;
    iw__fmt_at(d, pos, &f);
    for (i = 0; i < 4u; i++) { s->occ[i] = f.hocc[i]; s->lines[i] = f.hlines[i]; }
}

static uint32_t hf_lines(const uint8_t *t, uint32_t n)
{
    uint32_t k = 1, i;
    for (i = 0; i < n; i++) if (t[i] == IW_WP5_HRT) k++;
    return k;
}

/* Insert n bytes at the cursor, the cursor after them; the paragraph
 * re-wrapped. */
static int put_at_cursor(iw_ed_t *e, const uint8_t *g, uint32_t n)
{
    int rc = iw_doc_insert(&e->doc, e->cur, g, n);
    if (rc != IW_OK) return rc;
    e->cur += n;
    e->dirty = 1;
    e->goal = NONE;
    return iw_reflow_para(&e->doc, e->cur, e->width);
}

int iw_ed_hf(iw_ed_t *e, uint32_t which, uint32_t occ, const uint8_t *text, uint32_t n)
{
    uint8_t *g = g_scr;
    static const uint8_t disc[7] = IW_HF_DISC_TAIL_INIT;
    iw_doc_t *d = &e->doc;
    uint32_t p = 0, prev = NONE, seq = 0, i, o, size;
    if (which > IW_HF_FOOTER_B || n > HF_MAX) return IW_E_RANGE;
    if (occ != 0u && occ != IW_HF_EVERY && occ != IW_HF_ODD && occ != IW_HF_EVEN) return IW_E_RANGE;
    /* the definition of the letter before the cursor (the "old" bytes) and
     * the definitions before it in the document (seq) -- spec/iword/
     * wp5_page.h; MINT35 @0x1E3, MINT34 / MINT35 seq */
    while (p < e->cur && p < d->len) {
        if (is_hf(d, p)) {
            if ((uint32_t)byte_at(d, p + 1u) == which) prev = p;
            if (!hf_disc(d, p)) seq++;
        }
        p = iw_unit_next(d, p);
    }
    o = 0;
    g[o++] = IW_WP5_HF_GRP;
    g[o++] = (uint8_t)which;
    o += 2u;                                         /* the size, below */
    for (i = 0; i < IW_HF_OLD_LEN; i++) g[o + i] = 0;
    if (prev != NONE && !hf_disc(d, prev)) {
        /* its occurrence, height and seq; then 3 zeros */
        (void)iw_doc_read(d, prev + IW_WP5_VAR_HDR + IW_HF_OLD_LEN, g + o, 4u);
    }
    o += IW_HF_OLD_LEN;
    if (occ == 0u) {
        for (i = 0; i < sizeof disc; i++) g[o++] = disc[i];
    } else {
        uint32_t oc = occ;
#ifdef IWORD_MUT_HF_OCC
        /* MUTANT (Rule 6; test-iword-p4-mutant): Odd and Even swapped.
         * NEVER real. */
        if (oc == IW_HF_ODD) oc = IW_HF_EVEN; else if (oc == IW_HF_EVEN) oc = IW_HF_ODD;
#endif
        g[o++] = (uint8_t)oc;
        put16(g + o, hf_lines(text, n) * IW_WP5_WPU_PER_LINE); o += 2u;
        g[o++] = (uint8_t)seq;
        for (i = 0; i < 5u; i++) g[o++] = 0;
        put16(g + o, IW_HF_X); o += 2u;
        for (i = 0; i < n; i++) g[o++] = text[i];
    }
    size = o - IW_WP5_VAR_HDR + IW_WP5_VAR_TRAILER;
    put16(g + 2, size);
    put16(g + o, size); o += 2u;
    g[o++] = (uint8_t)which;
    g[o++] = IW_WP5_HF_GRP;
    if (d->len + o > d->max_len) return IW_E_FULL;
    return put_at_cursor(e, g, o);
}

int iw_ed_hf_edit(iw_ed_t *e, uint32_t pos, const uint8_t *text, uint32_t n)
{
    uint8_t *g = g_scr;
    iw_doc_t *d = &e->doc;
    uint32_t oldlen, o, size, i, which;
    int rc;
    if (!is_hf(d, pos) || hf_disc(d, pos) || n > HF_MAX) return IW_E_RANGE;
    which = (uint32_t)byte_at(d, pos + 1u);
    oldlen = iw_unit_next(d, pos) - pos;
    (void)iw_doc_read(d, pos, g, IW_WP5_VAR_HDR + IW_HF_HEAD_LEN);
    put16(g + IW_WP5_VAR_HDR + IW_HF_OLD_LEN + 1u, hf_lines(text, n) * IW_WP5_WPU_PER_LINE);
    o = IW_WP5_VAR_HDR + IW_HF_HEAD_LEN;
    for (i = 0; i < n; i++) g[o++] = text[i];
    size = o - IW_WP5_VAR_HDR + IW_WP5_VAR_TRAILER;
    put16(g + 2, size);
    put16(g + o, size); o += 2u;
    g[o++] = (uint8_t)which;
    g[o++] = IW_WP5_HF_GRP;
    if (d->len - oldlen + o > d->max_len) return IW_E_FULL;
    rc = iw_doc_delete(d, pos, oldlen);
    if (rc == IW_OK) rc = iw_doc_insert(d, pos, g, o);
    if (rc != IW_OK) return rc;
    if (e->cur > pos) e->cur = e->cur >= pos + oldlen ? e->cur - oldlen + o : pos + o;
    e->dirty = 1;
    return IW_OK;
}

/* ---- footnotes and endnotes ---------------------------------------------------- */
static int is_note(const iw_doc_t *d, uint32_t p)
{
    int s = byte_at(d, p + 1u);
    return byte_at(d, p) == (int)IW_WP5_NOTE_GRP && (s == (int)IW_NOTE_FOOT || s == (int)IW_NOTE_END);
}

static uint32_t note_head(const iw_doc_t *d, uint32_t p)
{
    return byte_at(d, p + 1u) == (int)IW_NOTE_FOOT ? IW_FOOT_HEAD_LEN : IW_END_HEAD_LEN;
}

uint32_t iw_note_number(const iw_doc_t *d, uint32_t pos)
{
    return is_note(d, pos) ? rd16(d, pos + IW_WP5_VAR_HDR + 1u) : 0u;
}

/* The note's text wrapped as the note screen wraps it (the first line after
 * "first" columns: the 0.5" tab and the number, shots_wpMINT42/01_note_wrap:
 * "     2A second ... a second line" / "of the note."): a word fits when it
 * starts at or before the right margin, 66 columns at 1"/1" (the body's
 * rule, spec/iword/wp5_format.h WRAP). In place: a space at a break becomes
 * the SRt 0x0D (MINT42 @0x16D). Returns the lines. */
static uint32_t note_wrap(uint8_t *t, uint32_t n, uint32_t first)
{
    uint32_t i, c = first, lastsp = NONE, lines = 1, start = 0;
    for (i = 0; i < n; i++) if (t[i] == IW_WP5_SRT) t[i] = ' ';
    for (i = 0; i < n; i++) {
        if (t[i] == IW_WP5_HRT) { lines++; c = 0; lastsp = NONE; start = i + 1u; continue; }
        if (t[i] == ' ') {
            if (c <= IW_WP5_WRAP_COLS) lastsp = i;
        }
        c++;
        if (c > IW_WP5_WRAP_COLS + 1u && lastsp != NONE && lastsp > start) {
            t[lastsp] = IW_WP5_SRT;
            lines++;
            start = lastsp + 1u;
            c = i - lastsp;
            lastsp = NONE;
        }
    }
    return lines;
}

int iw_ed_note(iw_ed_t *e, uint32_t kind, const uint8_t *text, uint32_t n)
{
    uint8_t *g = g_scr;
    iw_doc_t *d = &e->doc;
    uint32_t p = 0, num = 1, o = 0, i, size, lines, head;
    if ((kind != IW_NOTE_FOOT && kind != IW_NOTE_END) || n > NOTE_MAX) return IW_E_RANGE;
    while (p < e->cur && p < d->len) {
        if (is_note(d, p) && (uint32_t)byte_at(d, p + 1u) == kind) num++;
        p = iw_unit_next(d, p);
    }
    head = kind == IW_NOTE_FOOT ? IW_FOOT_HEAD_LEN : IW_END_HEAD_LEN;
    g[o++] = IW_WP5_NOTE_GRP;
    g[o++] = (uint8_t)kind;
    o += 2u;
    for (i = 0; i < head; i++) g[o + i] = 0;
    put16(g + o + 1u, num);
    put16(g + o + head - 2u, IW_HF_X);
    o += head;
    g[o++] = IW_WP5_NOTE_NUM;
    for (i = 0; i < n; i++) g[o + i] = text[i];
    lines = note_wrap(g + o, n, kind == IW_NOTE_FOOT ? IW_NOTE_INDENT_COLS + iw__digits(num) : iw__digits(num) + 1u);
    o += n;
    if (kind == IW_NOTE_FOOT) put16(g + IW_WP5_VAR_HDR + 4u, lines * IW_WP5_WPU_PER_LINE);
    size = o - IW_WP5_VAR_HDR + IW_WP5_VAR_TRAILER;
    put16(g + 2, size);
    put16(g + o, size); o += 2u;
    g[o++] = (uint8_t)kind;
    g[o++] = IW_WP5_NOTE_GRP;
    if (d->len + o > d->max_len) return IW_E_FULL;
    return put_at_cursor(e, g, o);
}

/* The note's text (after [Note Num]) and its length. */
static uint32_t note_text(const iw_doc_t *d, uint32_t p, uint32_t *at)
{
    uint32_t size = rd16(d, p + 2u), head = note_head(d, p);
    *at = p + IW_WP5_VAR_HDR + head + 1u;
    return size - IW_WP5_VAR_TRAILER - head - 1u;
}

/* ---- newspaper columns --------------------------------------------------------- */
int iw_ed_coldef(iw_ed_t *e, uint32_t ncols)
{
    uint8_t *g = g_scr;
    const uint32_t glen = IW_WP5_VAR_HDR + IW_COLDEF_PAYLOAD + IW_WP5_VAR_TRAILER;
    iw_doc_t *d = &e->doc;
    fmt_t f;
    uint32_t i, o, w, room;
    if (ncols < 2u || ncols > IW_COLDEF_MAX) return IW_E_RANGE;
    iw__fmt_at(d, e->cur, &f);
    if (f.colon) return IW_E_RANGE;      /* "you cannot enter a new column definition
                                          * while Columns are on" (p. 77) */
    room = f.rm > f.lm + IW_COL_GAP_WPU * (ncols - 1u) ? f.rm - f.lm - IW_COL_GAP_WPU * (ncols - 1u) : 0u;
    w = room / ncols;
    if (w < IW_WP5_WPU_PER_COL) return IW_E_RANGE;
    for (i = 0; i < glen; i++) g[i] = 0;
    g[0] = IW_WP5_COLDEF_GRP;
    g[1] = IW_WP5_COLDEF_SUB;
    put16(g + 2, IW_COLDEF_PAYLOAD + IW_WP5_VAR_TRAILER);
    if (f.coldef != NONE)                /* the old definition */
        (void)iw_doc_read(d, f.coldef + IW_WP5_VAR_HDR + IW_COLDEF_DEF_LEN, g + IW_WP5_VAR_HDR, IW_COLDEF_DEF_LEN);
    o = IW_WP5_VAR_HDR + IW_COLDEF_DEF_LEN;
    g[o++] = (uint8_t)ncols;
    for (i = 0; i < ncols; i++) {
        /* equal columns 0.5" apart between the margins: 1"-4", 4.5"-7.5"
         * (shots_wpMINT37/03_col_define; MINT37 @0x147) */
        uint32_t l = f.lm + i * (w + IW_COL_GAP_WPU);
        put16(g + o, l); o += 2u;
        put16(g + o, i + 1u == ncols ? f.rm : l + w); o += 2u;
    }
    o = IW_WP5_VAR_HDR + IW_COLDEF_PAYLOAD;
    put16(g + o, IW_COLDEF_PAYLOAD + IW_WP5_VAR_TRAILER);
    g[o + 2u] = IW_WP5_COLDEF_SUB;
    g[o + 3u] = IW_WP5_COLDEF_GRP;
    if (d->len + glen > d->max_len) return IW_E_FULL;
    return put_at_cursor(e, g, glen);
}

int iw_ed_col_on(iw_ed_t *e)
{
    fmt_t f;
    uint8_t b = IW_WP5_COL_ON;
    iw__fmt_at(&e->doc, e->cur, &f);
    if (f.coldef == NONE || f.colon) return IW_E_RANGE;   /* "an error message is
                                                            * displayed" (p. 76) */
    if (e->doc.len + 1u > e->doc.max_len) return IW_E_FULL;
    {
        int rc = put_at_cursor(e, &b, 1u);
        if (rc == IW_OK) rc = iw_reflow_from(&e->doc, e->cur, e->width);
        return rc;
    }
}

int iw_ed_col_off(iw_ed_t *e)
{
    fmt_t f;
    uint8_t b = IW_WP5_COL_OFF;
    int rc;
    iw__fmt_at(&e->doc, e->cur, &f);
    if (!f.colon) return IW_E_RANGE;
    if (e->doc.len + 1u + IW_COLOFF_GRP_LEN > e->doc.max_len) return IW_E_FULL;
    rc = put_at_cursor(e, &b, 1u);
    if (rc == IW_OK) rc = iw_reflow_from(&e->doc, e->cur, e->width);
    return rc;
}

/* ---- tables ------------------------------------------------------------------- */
#define TDEF_NEW(p)   ((p) + IW_WP5_VAR_HDR + IW_TBL_OLD_LEN)

uint32_t iw__cell_left(const iw_doc_t *d, uint32_t tdef, uint32_t idx)
{
    uint32_t x = rd16(d, TDEF_NEW(tdef) + 5u), i;
    uint32_t n = (uint32_t)(byte_at(d, TDEF_NEW(tdef) + 1u) & 0xFF);
    for (i = 0; i < idx && i < n; i++) x += rd16(d, TDEF_NEW(tdef) + 23u + 2u * i);
    return x;
}

static uint32_t tdef_len(uint32_t n)
{
    return IW_WP5_VAR_HDR + IW_TBL_OLD_LEN + 23u + 5u * n + 2u + IW_WP5_VAR_TRAILER;
}

static uint32_t row_info_len(uint32_t n) { return 4u + 2u * n; }

/* Insert g[0..n) at *at (the table under construction), *at after it. */
static int tbl_put(iw_doc_t *d, uint32_t *at, const uint8_t *g, uint32_t n)
{
    int rc;
    if (d->len + n > d->max_len) return IW_E_FULL;
    rc = iw_doc_insert(d, *at, g, n);
    if (rc == IW_OK) *at += n;
    return rc;
}

int iw_ed_table(iw_ed_t *e, uint32_t cols, uint32_t rows)
{
    static const uint8_t old[] = IW_TBL_OLD_INIT;
    uint8_t *g = g_scr;
    iw_doc_t *d = &e->doc;
    fmt_t f;
    uint32_t o, i, r, c, size, w, at = e->cur, first_cell = 0;
    int rc = IW_OK;
    if (cols < 1u || cols > IW_TBL_MAX_COLS || rows < 1u) return IW_E_RANGE;
    iw__fmt_at(d, e->cur, &f);
    if (f.tbl || f.colon) return IW_E_RANGE;   /* no tables in tables or columns (p. 643) */
    /* [Tbl Def] (MINT38 / MINT40 / MINT41 @0x147) */
    size = tdef_len(cols) - IW_WP5_VAR_HDR;
    for (i = 0; i < tdef_len(cols); i++) g[i] = 0;
    o = 0;
    g[o++] = IW_WP5_COLDEF_GRP; g[o++] = IW_WP5_TBL_SUB; put16(g + o, size); o += 2u;
    for (i = 0; i < sizeof old; i++) g[o + i] = old[i];
    o += IW_TBL_OLD_LEN;
    g[o + 0] = 0x0A; g[o + 1] = (uint8_t)cols; g[o + 3] = 0x21;
    put16(g + o + 5u, f.lm);
    put16(g + o + 7u, IW_TBL_CELL_MARGIN); put16(g + o + 9u, IW_TBL_CELL_MARGIN); put16(g + o + 11u, IW_TBL_TEXT_DY);
    g[o + 19u] = 0x01;
    w = (f.rm - f.lm) / cols;            /* (9000 - 1200) / n: 2600, 3900, 1950 */
#ifdef IWORD_MUT_TBL_WIDTH
    /* MUTANT (Rule 6; test-iword-p4-mutant): the columns one WPU off equal.
     * NEVER real. */
    w++;
#endif
    for (i = 0; i < cols; i++) put16(g + o + 23u + 2u * i, w);
    for (i = 0; i < cols; i++) g[o + 23u + 4u * cols + i] = 0x20;
    o += 23u + 5u * cols + 2u;
    put16(g + o, size); g[o + 2u] = IW_WP5_TBL_SUB; g[o + 3u] = IW_WP5_COLDEF_GRP; o += 4u;
    rc = tbl_put(d, &at, g, o);
    /* the rows and cells, a row at a time; their payloads are iw__p4_fixup's */
    for (r = 0; r < rows && rc == IW_OK; r++) {
        uint32_t rs = 2u * row_info_len(cols) + 2u + IW_WP5_VAR_TRAILER;
        for (i = 0; i < IW_WP5_VAR_HDR + rs; i++) g[i] = 0;
        o = 0;
        g[o] = IW_WP5_TROW_GRP; g[o + 1u] = IW_WP5_TROW_SUB; put16(g + o + 2u, rs);
        o += IW_WP5_VAR_HDR + rs - IW_WP5_VAR_TRAILER;
        put16(g + o, rs); g[o + 2u] = IW_WP5_TROW_SUB; g[o + 3u] = IW_WP5_TROW_GRP; o += 4u;
        for (c = 0; c < cols; c++) {
            for (i = 0; i < 19u; i++) g[o + i] = 0;
            g[o] = IW_WP5_TROW_GRP; g[o + 1u] = IW_WP5_TCELL_SUB; put16(g + o + 2u, 15u);
            g[o + 5u] = (uint8_t)c; g[o + 6u] = 1; g[o + 7u] = 1; g[o + 14u] = 0x20;
            put16(g + o + 15u, 15u); g[o + 17u] = IW_WP5_TCELL_SUB; g[o + 18u] = IW_WP5_TROW_GRP;
            o += 19u;
            if (r == 0u && c == 0u) first_cell = at + o;
        }
        rc = tbl_put(d, &at, g, o);
    }
    if (rc == IW_OK) {
        uint32_t ts = row_info_len(cols) + 7u + IW_WP5_VAR_TRAILER;
        for (i = 0; i < IW_WP5_VAR_HDR + ts; i++) g[i] = 0;
        o = 0;
        g[o] = IW_WP5_TROW_GRP; g[o + 1u] = IW_WP5_TOFF_SUB; put16(g + o + 2u, ts);
        o += IW_WP5_VAR_HDR + ts - IW_WP5_VAR_TRAILER;
        put16(g + o, ts); g[o + 2u] = IW_WP5_TOFF_SUB; g[o + 3u] = IW_WP5_TROW_GRP; o += 4u;
        rc = tbl_put(d, &at, g, o);
    }
    if (rc != IW_OK) {
        /* a table too large for the document: take back what went in */
        if (at > e->cur) (void)iw_doc_delete(d, e->cur, at - e->cur);
        return rc;
    }
    /* the cursor in cell A1 (shots_wpMINT38/04_table_edit "Cell A1") */
    e->cur = first_cell;
    e->dirty = 1;
    e->goal = NONE;
    return IW_OK;
}

int iw_in_table(const iw_doc_t *d, uint32_t pos)
{
    fmt_t f;
    iw__fmt_at(d, pos, &f);
    return f.tbl != 0;
}

int iw_ed_cell(iw_ed_t *e, int dir)
{
    iw_doc_t *d = &e->doc;
    uint32_t p, here = NONE, prev = NONE;
    if (!iw_in_table(d, e->cur)) return IW_E_RANGE;
    if (dir > 0) {
        /* Tab: after the next [Cell] (Reference p. 642) */
        p = e->cur;
        while (p < d->len) {
            int k = iw_unit_kind(d, p);
            if (k == IW_U_CELL) { e->cur = iw_unit_next(d, p); e->goal = NONE; return IW_OK; }
            if (k == IW_U_ROW && byte_at(d, p + 1u) == (int)IW_WP5_TOFF_SUB) return IW_E_RANGE;
            p = iw_unit_next(d, p);
        }
        return IW_E_RANGE;
    }
    /* Shift-Tab: after the [Cell] before the cursor's own */
    p = 0;
    while (p < e->cur && p < d->len) {
        if (iw_unit_kind(d, p) == IW_U_CELL) { prev = here; here = p; }
        else if (iw_unit_kind(d, p) == IW_U_ROW && byte_at(d, p + 1u) == (int)IW_WP5_TOFF_SUB) prev = here = NONE;
        else if (byte_at(d, p) == (int)IW_WP5_COLDEF_GRP && byte_at(d, p + 1u) == (int)IW_WP5_TBL_SUB) prev = here = NONE;
        p = iw_unit_next(d, p);
    }
    if (prev == NONE) return IW_E_RANGE;
    e->cur = iw_unit_next(d, prev);
    e->goal = NONE;
    return IW_OK;
}

/* ---- WordPerfect's bookkeeping in the P4 codes (iw_ed_fixup) --------------------- */
static uint32_t cell_word(uint32_t r, uint32_t rows, uint32_t c, uint32_t cols)
{
    /* bits 0-2 top, 3-5 left, 8-10 bottom, 11-13 right; 1 single, 2 double:
     * double outside, single inside, an inside bottom / right none (MINT38,
     * MINT40 every word) */
    uint32_t t = r == 0u ? 2u : 1u, l = c == 0u ? 2u : 1u;
    uint32_t b = r + 1u == rows ? 2u : 0u, rt = c + 1u == cols ? 2u : 0u;
    return t | (l << 3) | (b << 8) | (rt << 11);
}

static uint32_t row_h(uint32_t r, uint32_t rows)
{
#ifdef IWORD_MUT_TBL_ROWH
    /* MUTANT (Rule 6; test-iword-p4-mutant): every row the middle row's
     * height. NEVER real. */
    (void)r; (void)rows;
    return IW_TBL_ROW_H_BASE;
#else
    return IW_TBL_ROW_H_BASE + (r == 0u ? IW_TBL_ROW_H_TOP2 : 0u) + (r + 1u == rows ? IW_TBL_ROW_H_BOT2 : 0u);
#endif
}

static uint32_t row_info(uint8_t *g, uint32_t r, uint32_t rows, uint32_t cols)
{
    uint32_t o = 0, c;
    g[o++] = 0x03;
    put16(g + o, row_h(r, rows)); o += 2u;
    g[o++] = (uint8_t)(0x80u + 2u * cols);
    for (c = 0; c < cols; c++) { put16(g + o, cell_word(r, rows, c, cols)); o += 2u; }
    return o;
}

/* The table whose [Tbl Def] is at t, its top `top` WPU below the top margin. */
static int fix_table(iw_doc_t *d, uint32_t t, uint32_t top)
{
    uint8_t g[160], gi[80];
    uint32_t cols = (uint32_t)(byte_at(d, TDEF_NEW(t) + 1u) & 0xFF), rows = 0, p, r, rtop, prevtop;
    int rc;
    if (cols < 1u || cols > IW_TBL_MAX_COLS) return IW_OK;
    /* count the rows */
    p = iw_unit_next(d, t);
    while (p < d->len && !(byte_at(d, p) == (int)IW_WP5_TROW_GRP && byte_at(d, p + 1u) == (int)IW_WP5_TOFF_SUB)) {
        if (byte_at(d, p) == (int)IW_WP5_TROW_GRP && byte_at(d, p + 1u) == (int)IW_WP5_TROW_SUB) rows++;
        p = iw_unit_next(d, p);
    }
    if (!rows || p >= d->len) return IW_OK;
    put16(g, top);
    rc = pput(d, TDEF_NEW(t) + 17u, g, 2u);
    if (rc != IW_OK) return rc;
    p = iw_unit_next(d, t);
    r = 0;
    rtop = prevtop = top;
    while (p < d->len && rc == IW_OK) {
        int b = byte_at(d, p), s = byte_at(d, p + 1u);
        if (b == (int)IW_WP5_TROW_GRP && s == (int)IW_WP5_TROW_SUB) {
            uint32_t L, o = 0;
            if (r > 0u) { prevtop = rtop; rtop += row_h(r - 1u, rows); }
            if (r > 0u) o = row_info(g, r - 1u, rows, cols);
            L = row_info(gi, r, rows, cols);
            if (r == 0u) { uint32_t i; for (i = 0; i < L; i++) g[i] = gi[i]; o = L; }
            { uint32_t i; for (i = 0; i < L; i++) g[o + i] = gi[i]; o += L; }
            put16(g + o, r == 0u ? top : prevtop); o += 2u;
            if (iw_unit_next(d, p) - p == IW_WP5_VAR_HDR + o + IW_WP5_VAR_TRAILER)
                rc = pput(d, p + IW_WP5_VAR_HDR, g, o);
            r++;
        } else if (b == (int)IW_WP5_TROW_GRP && s == (int)IW_WP5_TCELL_SUB) {
            uint32_t c = (uint32_t)(byte_at(d, p + 5u) & 0xFF);
            put16(g, c == 0u ? 0u : rtop + row_h(r - 1u, rows));
            rc = pput(d, p + IW_WP5_VAR_HDR + 4u, g, 2u);
        } else if (b == (int)IW_WP5_TROW_GRP && s == (int)IW_WP5_TOFF_SUB) {
            uint32_t o = row_info(g, rows - 1u, rows, cols);
            put16(g + o, rows - 1u); o += 2u;
            put16(g + o, rtop); o += 2u;
            g[o++] = 0; g[o++] = 0;
            g[o++] = (uint8_t)cols;
            if (iw_unit_next(d, p) - p == IW_WP5_VAR_HDR + o + IW_WP5_VAR_TRAILER)
                rc = pput(d, p + IW_WP5_VAR_HDR, g, o);
            break;
        }
        p = iw_unit_next(d, p);
    }
    return rc;
}

/* Does the document hold a note or a table (one pass over its units)? The
 * walk below is needed only then. */
static int has_notes_or_tables(const iw_doc_t *d)
{
    uint32_t p = 0;
    while (p < d->len) {
        int b = byte_at(d, p);
        if (b == (int)IW_WP5_NOTE_GRP || b == (int)IW_WP5_TROW_GRP ||
            (b == (int)IW_WP5_COLDEF_GRP && byte_at(d, p + 1u) == (int)IW_WP5_TBL_SUB)) return 1;
        p = iw_unit_next(d, p);
    }
    return 0;
}

int iw__p4_fixup(iw_ed_t *e)
{
    iw_doc_t *d = &e->doc;
    walk_t w;
    uint32_t fn = 0, en = 0, page = 0, onpage = 0;
    int rc = IW_OK;
    if (!has_notes_or_tables(d)) return IW_OK;
    iw__walk_first(e, &w);
    for (;;) {
        uint32_t p = w.ln.start;
        if (w.page != page) { page = w.page; onpage = 0; }
        while (p < w.ln.end && p < d->len && rc == IW_OK) {
            if (is_note(d, p)) {
                uint8_t g[8];
                uint32_t at, n = note_text(d, p, &at), i, lines = 1;
                int foot = byte_at(d, p + 1u) == (int)IW_NOTE_FOOT;
                uint32_t num = foot ? ++fn : ++en;
#ifdef IWORD_MUT_NOTE_RENUM
                /* MUTANT (Rule 6; test-iword-p4-mutant): notes keep the
                 * number they were typed with. NEVER real. */
                num = rd16(d, p + IW_WP5_VAR_HDR + 1u);
#endif
                put16(g, num);
                rc = pput(d, p + IW_WP5_VAR_HDR + 1u, g, 2u);
                if (foot && rc == IW_OK) {
                    for (i = 0; i < n; i++) {
                        int c = byte_at(d, at + i);
                        if (c == (int)IW_WP5_SRT || c == (int)IW_WP5_HRT) lines++;
                    }
                    /* h (MINT36 / MINT42: 200 a line); y, where its line is
                     * below the page's text top (MINT05 @0x156 200: "Footnote
                     * host." on line 2; MINT36 / MINT42 0); off: 600 for each
                     * note before it on its page (MINT36 0 / 600, MINT42 0 /
                     * 600 / 1200 -- whatever their heights) [inferred, W-22] */
                    put16(g, lines * IW_WP5_WPU_PER_LINE);
                    put16(g + 2, IW_Y_TO_WPU(w.y - w.top));
                    put16(g + 4, 600u * onpage);
                    rc = pput(d, p + IW_WP5_VAR_HDR + 4u, g, 6u);
                    onpage++;
                }
            } else if (byte_at(d, p) == (int)IW_WP5_COLDEF_GRP && byte_at(d, p + 1u) == (int)IW_WP5_TBL_SUB) {
                rc = fix_table(d, p, IW_Y_TO_WPU(w.y));
            }
            p = iw_unit_next(d, p);
        }
        if (rc != IW_OK) return rc;
        if (!iw__walk_next(e, &w)) return IW_OK;
    }
}

/* ---- Reveal Codes labels ---------------------------------------------------------- */
static uint32_t put(char *out, uint32_t cap, uint32_t n, const char *s)
{
    while (*s) { if (n + 1u < cap) out[n] = *s; n++; s++; }
    return n;
}

static uint32_t put_num(char *out, uint32_t cap, uint32_t n, uint32_t v)
{
    char t[12];
    uint32_t k = 0;
    do { t[k++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    while (k) { char c[2]; c[0] = t[--k]; c[1] = 0; n = put(out, cap, n, c); }
    return n;
}

static uint32_t put_inch(char *out, uint32_t cap, uint32_t n, uint32_t wpu)
{
    char t[12];
    (void)iw_fmt_inches((wpu + 6u) / 12u, t);
    return put(out, cap, n, t);
}

/* A code's text in its label: the first 50 characters, then "..."
 * (Reference p. 271); ^B as "^B", returns as their own labels. */
static uint32_t put_text(const iw_doc_t *d, uint32_t at, uint32_t len, char *out, uint32_t cap, uint32_t n)
{
    uint32_t i, shown = 0;
    for (i = 0; i < len; i++) {
        int c = byte_at(d, at + i);
        if (shown >= IW_HF_LABEL_MAX) return put(out, cap, n, "...");
        if (c == (int)IW_HF_PAGE_NUM) n = put(out, cap, n, "^B");
        else if (c == (int)IW_WP5_HRT) n = put(out, cap, n, IW_RV_HRT);
        else if (c == (int)IW_WP5_SRT) n = put(out, cap, n, IW_RV_SRT);
        else if (c == (int)IW_WP5_NOTE_NUM) n = put(out, cap, n, IW_RV_NOTE_NUM);
        else if (c >= 0x20 && c <= 0x7E) { char t[2]; t[0] = (char)c; t[1] = 0; n = put(out, cap, n, t); }
        shown++;
    }
    return n;
}

uint32_t iw_p4_label(const iw_doc_t *d, uint32_t pos, char *out, uint32_t cap, uint32_t n)
{
    static const char *const NAMES[4] = IW_HF_NAMES;
    int b = byte_at(d, pos), s = byte_at(d, pos + 1u);
    if (b == (int)IW_WP5_HRT_SPG) return put(out, cap, n, IW_RV_HRT_SPG);
    if (b == (int)IW_WP5_SRT_SPG) return put(out, cap, n, IW_RV_SRT_SPG);
    if (b == (int)IW_WP5_COL_ON) return put(out, cap, n, IW_RV_COL_ON);
    if (b == (int)IW_WP5_COL_OFF) return put(out, cap, n, IW_RV_COL_OFF);
    if (is_hf(d, pos)) {
        /* "[Header A:Every page;Initech]" / "[Header A:Discontinue]" (MINT33,
         * MINT34, MINT35 reveal shots) */
        uint32_t occ, at, len;
        n = put(out, cap, n, "[");
        n = put(out, cap, n, NAMES[s]);
        n = put(out, cap, n, ":");
        if (hf_disc(d, pos)) return put(out, cap, n, IW_HF_DISC_LABEL "]");
        occ = (uint32_t)byte_at(d, pos + IW_WP5_VAR_HDR + IW_HF_OLD_LEN);
        n = put(out, cap, n, occ == IW_HF_ODD ? IW_HF_OCC_LABEL_ODD : occ == IW_HF_EVEN ? IW_HF_OCC_LABEL_EVEN
                                                                       : IW_HF_OCC_LABEL_EVERY);
        n = put(out, cap, n, ";");
        at = pos + IW_WP5_VAR_HDR + IW_HF_HEAD_LEN;
        len = rd16(d, pos + 2u) - IW_WP5_VAR_TRAILER - IW_HF_HEAD_LEN;
        n = put_text(d, at, len, out, cap, n);
        return put(out, cap, n, "]");
    }
    if (is_note(d, pos)) {
        /* "[Footnote:1;[Note Num]First note.]" (MINT36 10_reveal) */
        uint32_t at, len = note_text(d, pos, &at);
        n = put(out, cap, n, s == (int)IW_NOTE_FOOT ? IW_RV_FOOTNOTE : IW_RV_ENDNOTE);
        n = put_num(out, cap, n, iw_note_number(d, pos));
        n = put(out, cap, n, ";" IW_RV_NOTE_NUM);
        n = put_text(d, at, len, out, cap, n);
        return put(out, cap, n, "]");
    }
    if (b == (int)IW_WP5_COLDEF_GRP && s == (int)IW_WP5_COLDEF_SUB) {
        /* "[Col Def:Newspaper;2;1",4";4.5",7.5"]" (MINT37 10_reveal) */
        uint32_t at = pos + IW_WP5_VAR_HDR + IW_COLDEF_DEF_LEN, k = (uint32_t)(byte_at(d, at) & 0xFF), i;
        n = put(out, cap, n, IW_RV_COL_DEF);
        n = put_num(out, cap, n, k);
        for (i = 0; i < k && i < IW_COLDEF_MAX; i++) {
            n = put(out, cap, n, ";");
            n = put_inch(out, cap, n, rd16(d, at + 1u + 4u * i));
            n = put(out, cap, n, ",");
            n = put_inch(out, cap, n, rd16(d, at + 3u + 4u * i));
        }
        return put(out, cap, n, "]");
    }
    if (b == (int)IW_WP5_COLDEF_GRP && s == (int)IW_WP5_TBL_SUB) {
        /* "[Tbl Def:I;3,2.17",2.17",2.17"]" (MINT38 09_reveal) */
        uint32_t k = (uint32_t)(byte_at(d, TDEF_NEW(pos) + 1u) & 0xFF), i;
        n = put(out, cap, n, IW_RV_TBL_DEF);
        n = put_num(out, cap, n, k);
        for (i = 0; i < k; i++) {
            n = put(out, cap, n, ",");
            n = put_inch(out, cap, n, rd16(d, TDEF_NEW(pos) + 23u + 2u * i));
        }
        return put(out, cap, n, "]");
    }
    if (b == (int)IW_WP5_TROW_GRP && s == (int)IW_WP5_TROW_SUB) return put(out, cap, n, IW_RV_ROW);
    if (b == (int)IW_WP5_TROW_GRP && s == (int)IW_WP5_TCELL_SUB) return put(out, cap, n, IW_RV_CELL);
    if (b == (int)IW_WP5_TROW_GRP && s == (int)IW_WP5_TOFF_SUB) return put(out, cap, n, IW_RV_TBL_OFF);
    return NONE;
}

/* ---- printing: the Standard Printer's stream ------------------------------------------ *
 * spec/iword/wp5_page.h "printing". A page is built as 66 print lines of up
 * to PL layers (a layer prints over the one before it after a "\r": Header
 * A over Header B, PRTMINT33 line 73), then emitted. */
#define PW     96u                  /* print columns kept (75 at 1"/1")   */
#define PL     2u                   /* layers a line: A over B            */
typedef struct pline {
    char     t[PL][PW];
    uint16_t len[PL];               /* columns used (0: empty)            */
    uint8_t  raw;                   /* the note separator (below)         */
} pline_t;
static pline_t g_pg[IW_PG_LINES + 1u];

typedef struct pout {
    iw_sink_t sink;
    void     *ctx;
    uint8_t   buf[512];
    uint32_t  n;
    int       rc;
} pout_t;

static void flush(pout_t *o)
{
    if (o->n && o->rc == 0) o->rc = o->sink(o->ctx, o->buf, o->n);
    o->n = 0;
}

static void emit(pout_t *o, const char *s, uint32_t k)
{
    uint32_t i;
    for (i = 0; i < k; i++) {
        if (o->n == sizeof o->buf) flush(o);
        o->buf[o->n++] = (uint8_t)s[i];
    }
}

static void emit_c(pout_t *o, char c) { emit(o, &c, 1u); }

static void page_clear(void)
{
    uint32_t L, k;
    for (L = 0; L <= IW_PG_LINES; L++) {
        for (k = 0; k < PL; k++) g_pg[L].len[k] = 0;
        g_pg[L].raw = 0;
    }
}

/* A character at print column col of print line L (1..66): on the first
 * layer that has the column free. */
static void pchar_on(uint32_t L, uint32_t k, uint32_t col, char c)
{
    uint32_t i;
    pline_t *pl;
    if (L < 1u || L > IW_PG_LINES || col >= PW || k >= PL) return;
    pl = &g_pg[L];
    for (i = pl->len[k]; i < col; i++) pl->t[k][i] = ' ';
    pl->t[k][col] = c;
    if (col + 1u > pl->len[k]) pl->len[k] = (uint16_t)(col + 1u);
}

/* The body's characters: the first layer (columns side by side share it). */
static void pchar(uint32_t L, uint32_t col, char c)
{
    pchar_on(L, 0, col, c);
}

/* The layer a header / footer's text starts on line L: the first one still
 * empty there (Header A, then Header B over it: PRTMINT33 lines 73, 192). */
static uint32_t free_layer(uint32_t L)
{
    uint32_t k;
    if (L < 1u || L > IW_PG_LINES) return 0;
    for (k = 0; k < PL; k++) if (!g_pg[L].len[k]) return k;
    return PL - 1u;
}

/* The column a text at paper position x prints at (spec/iword/wp5_page.h
 * "WHERE A TEXT PRINTS"). */
static uint32_t pcol(uint32_t x)
{
    return (x + IW_PRT_X_ROUND) / IW_PRT_COL_WPU;
}

/* The print line of a position y (y units below the top margin). */
static uint32_t pline_of(uint32_t y)
{
    return IW_PG_TOP_LINES + 1u + (IW_Y_TO_WPU(y) + IW_PRT_LINE_WPU / 2u) / IW_PRT_LINE_WPU;
}

static void page_emit(pout_t *o)
{
    uint32_t L, cur = 1, k, i, last = 0;
    emit_c(o, (char)IW_PRT_CR);
    for (L = 1; L <= IW_PG_LINES; L++) {
        pline_t *pl = &g_pg[L];
        int any = pl->raw;
        for (k = 0; k < PL; k++) if (pl->len[k]) any = 1;
        if (!any) continue;
        for (; cur < L; cur++) { emit_c(o, (char)IW_PRT_CR); emit_c(o, (char)IW_PRT_LF); }
        if (pl->raw) {
            /* the note separator (PRTMINT36 line 56): spaces to its right
             * end, CR, spaces to its left end and the 2" line */
            for (i = 0; i < IW_WP5_LEFT_WPU / IW_PRT_COL_WPU + IW_NOTE_SEP_COLS; i++) emit_c(o, ' ');
            emit_c(o, (char)IW_PRT_CR);
            for (i = 0; i < IW_WP5_LEFT_WPU / IW_PRT_COL_WPU; i++) emit_c(o, ' ');
            for (i = 0; i < IW_NOTE_SEP_COLS; i++) emit_c(o, '_');
        } else {
            int first = 1;
            for (k = 0; k < PL; k++) {
                if (!pl->len[k]) continue;
                if (!first) emit_c(o, (char)IW_PRT_CR);
                emit(o, pl->t[k], pl->len[k]);
                first = 0;
            }
        }
        last = L;
    }
    (void)last;
#ifndef IWORD_MUT_PRT_PAGEEND
    /* (IWORD_MUT_PRT_PAGEEND, Rule 6, test-iword-p4-mutant: no CR after the
     * page's last line. NEVER real.) */
    emit_c(o, (char)IW_PRT_CR);
#endif
    for (; cur <= IW_PG_LINES; cur++) { emit_c(o, (char)IW_PRT_CR); emit_c(o, (char)IW_PRT_LF); }
}

/* A run of text (header / footer / note text) on print line L from paper
 * position x: characters; ^B the page number. Returns x after it. */
static uint32_t ptext(const uint8_t *t, uint32_t n, uint32_t L, uint32_t x, uint32_t page)
{
    uint32_t i, layer = free_layer(L);
    for (i = 0; i < n; i++) {
        if (t[i] == IW_HF_PAGE_NUM) {
            char num[12];
            uint32_t k = 0, v = page;
            do { num[k++] = (char)('0' + v % 10u); v /= 10u; } while (v);
            while (k) { pchar_on(L, layer, pcol(x), num[--k]); x += IW_PRT_COL_WPU; }
        } else if (t[i] >= 0x20 && t[i] <= 0x7E) {
            if (t[i] != ' ') pchar_on(L, layer, pcol(x), (char)t[i]);
            x += IW_PRT_COL_WPU;
        }
    }
    return x;
}

/* The header / footer definitions a page prints: per letter the position of
 * the definition (NONE: none / discontinued). */
typedef struct hfpos { uint32_t at[4]; } hfpos_t;

static void hf_track(const iw_doc_t *d, uint32_t from, uint32_t to, hfpos_t *h, int leading_only)
{
    uint32_t p = from;
    while (p < to && p < d->len) {
        int k = iw_unit_kind(d, p);
        if (leading_only && k != IW_U_CODE && k != IW_U_HIDDEN && k != IW_U_LEAD) return;
        if (is_hf(d, p)) h->at[byte_at(d, p + 1u)] = hf_disc(d, p) ? NONE : p;
        p = iw_unit_next(d, p);
    }
}

static void print_hf(const iw_doc_t *d, const hfpos_t *h, uint32_t page, uint32_t footer_line0)
{
    uint8_t *t = g_scr;
    uint32_t i;
    int odd = (int)(page & 1u);
    for (i = 0; i < 4u; i++) {
        uint32_t at = h->at[i], n, L, s, j;
        if (at == NONE || !hf_applies((uint32_t)byte_at(d, at + IW_WP5_VAR_HDR + IW_HF_OLD_LEN), odd)) continue;
        n = iw_hf_text(d, at, t, HF_MAX);
        L = i < 2u ? IW_PG_TOP_LINES + 1u : footer_line0;
        if (i >= 2u) L = IW_PG_TOP_LINES + IW_PG_TEXT_LINES - hf_lines(t, n) + 1u;
        for (s = 0, j = 0; j <= n; j++) {
            if (j == n || t[j] == IW_WP5_HRT) {
                (void)ptext(t + s, j - s, L, IW_WP5_LEFT_WPU, page);
                L++;
                s = j + 1u;
            }
        }
    }
}

/* The units of a body line onto print line L (spec/iword/wp5_page.h; the
 * line's own geometry -- margins, centering, tabs, columns -- is the
 * layout's; a table row's cells at their left edge + 100; a fully
 * justified line's spare width over its spaces). */
static void print_line(const iw_doc_t *d, const iw_line_t *ln, uint32_t L)
{
    uint32_t p, c = ln->col0, x, gaps = 0, spare = 0, k = 0, nch = 0;
    int just = 0;
    /* the spare width of a soft-returned Full line */
    if (ln->tbl == IW_LT_NONE && ln->just == IW_WP5_JUST_FULL && ln->term == IW_U_SRT) {
        uint32_t cc = ln->col0, end;
        for (p = ln->start; p < ln->text_end; p = iw_unit_next(d, p)) {
            int kk = iw_unit_kind(d, p);
            if (kk == IW_U_TEXT && byte_at(d, p) == ' ') gaps++;
            cc = iw_line_col(d, ln, iw_unit_next(d, p));
        }
        end = IW_LINE_X(ln, cc);
        if (gaps && ln->rm > end) { spare = ln->rm - end; just = 1; }
    }
    x = IW_LINE_X(ln, c);
    for (p = ln->start; p < ln->text_end; p = iw_unit_next(d, p)) {
        int kk = iw_unit_kind(d, p);
        if (kk == IW_U_CELL && ln->tdef != NONE) {
            x = iw__cell_left(d, ln->tdef, (uint32_t)(byte_at(d, p + 5u) & 0xFF)) + IW_TBL_CELL_MARGIN;
            continue;
        }
        if (kk == IW_U_TEXT || kk == IW_U_HSPACE) {
            int ch = kk == IW_U_TEXT ? byte_at(d, p) : ' ';
            if (ch == ' ') k++;
            else {
                uint32_t col = just ? ((x + IW_PRT_X_ROUND) * gaps + k * spare) / (IW_PRT_COL_WPU * gaps) : pcol(x);
                pchar(L, col, (char)ch);
            }
            x += IW_PRT_COL_WPU;
            nch++;
        } else if (kk == IW_U_NOTE) {
            uint32_t v = iw_note_number(d, p), dg = iw__digits(v), i;
            char num[12];
            for (i = 0; i < dg; i++) { num[dg - 1u - i] = (char)('0' + v % 10u); v /= 10u; }
            for (i = 0; i < dg; i++) { pchar(L, pcol(x), num[i]); x += IW_PRT_COL_WPU; }
        } else if (kk == IW_U_TAB || kk == IW_U_INDENT) {
            x = IW_LINE_X(ln, iw_line_col(d, ln, iw_unit_next(d, p)));
        }
    }
    (void)nch;
}

/* A note's text on paper (spec/iword/wp5_page.h "THE PRINTED NOTES"): wrapped
 * again, a word fitting when it ENDS by the right margin (W-25), each line
 * but the last fully justified. Into lines L0.. when `draw`; returns the
 * lines it takes. */
static uint32_t print_note(const iw_doc_t *d, uint32_t at, uint32_t len, uint32_t x0, const char *lead,
                           uint32_t L0, int draw, uint32_t rm)
{
    uint8_t *t = g_scr;
    uint32_t n = 0, i, s, lines = 0, x, ls;
    for (i = 0; lead[i]; i++) t[n++] = (uint8_t)lead[i];
    ls = n;
    for (i = 0; i < len && n < NOTE_MAX + 16u; i++) {
        int c = byte_at(d, at + i);
        t[n++] = (uint8_t)(c == (int)IW_WP5_SRT ? ' ' : c);
    }
    s = 0;
    x = x0;
    (void)ls;
    while (s < n) {
        /* the words that end by rm */
        uint32_t e = s, lastgap = NONE, j, xe = x, gaps = 0, spare, k;
        int hard = 0;
        for (j = s; j < n; j++) {
            if (t[j] == IW_WP5_HRT) { e = j; hard = 1; break; }
            if (t[j] == ' ' && j > s) lastgap = j;
            if (x + (j - s + 1u) * IW_PRT_COL_WPU > rm && t[j] != ' ') {
                if (lastgap != NONE) { e = lastgap; break; }
            }
            e = j + 1u;
        }
        if (j >= n) { e = n; hard = 1; }
        /* drop the trailing gap */
        while (e > s && t[e - 1u] == ' ') e--;
        for (j = s; j < e; j++) if (t[j] == ' ') gaps++;
        xe = x + (e - s) * IW_PRT_COL_WPU;
        spare = (!hard && gaps && rm > xe) ? rm - xe : 0u;
        if (draw) {
            for (j = s, k = 0; j < e; j++) {
                uint32_t xx = x + (j - s) * IW_PRT_COL_WPU;
                if (t[j] == ' ') { k++; continue; }
                pchar(L0 + lines, spare ? ((xx + IW_PRT_X_ROUND) * gaps + k * spare) / (IW_PRT_COL_WPU * gaps)
                                        : pcol(xx), (char)t[j]);
            }
        }
        lines++;
        s = e;
        while (s < n && (t[s] == ' ' || (hard && t[s] == IW_WP5_HRT))) { if (t[s] == IW_WP5_HRT) { s++; break; } s++; }
        x = IW_WP5_LEFT_WPU;            /* continuation lines at the margin */
    }
    return lines ? lines : 1u;
}

#define NOTES_MAX 64u
static pout_t g_po;
static iw_ed_t g_pe;                   /* the paper's copy of the document */
static iw_piece_t g_ppc[4];

int iw_print(const iw_ed_t *src, uint8_t *scratch, uint32_t scap, iw_sink_t sink, void *ctx)
{
    const iw_doc_t *d;
    const iw_ed_t *e = &g_pe;
    pout_t *op = &g_po;
#define o (*op)
    walk_t w;
    hfpos_t track, pg;
    uint32_t notes[NOTES_MAX], nnotes = 0, ends = 0, i, page, last_body = 0, len = src->doc.len;
    int more = 1, rc;
    if (len > scap) return IW_E_FULL;
    /* the copy: one piece in an add buffer of its own, so the paper's wrap
     * rewrites its bytes in place (iw_doc_set) and never the document's */
    (void)iw_doc_read(&src->doc, 0, scratch, len);
    iw_doc_init(&g_pe.doc, scratch, scap, g_ppc, 4u, len);
    g_pe.doc.add_len = len;
    g_pe.doc.len = len;
    g_pe.doc.npc = len ? 1u : 0u;
    g_ppc[0].off = IW_PIECE_ADD;
    g_ppc[0].len = len;
    iw_ed_init(&g_pe, src->width);
    iw__print_wrap(1);
    rc = iw_reflow_all(&g_pe.doc, g_pe.width);
    if (rc != IW_OK) { iw__print_wrap(0); return rc; }
    d = &g_pe.doc;
    o.sink = sink; o.ctx = ctx; o.n = 0; o.rc = 0;
    for (i = 0; i < 4u; i++) track.at[i] = NONE;
    iw__walk_first(e, &w);
    while (more) {
        page = w.page;
        page_clear();
        nnotes = 0;
        last_body = 0;
        pg = track;
        hf_track(d, w.ln.start, w.ln.text_end, &pg, 1);
        /* the page's lines */
        for (;;) {
            uint32_t L, p;
            if (w.ln.tbl == IW_LT_ROW) L = pline_of(w.y + IW_WPU_TO_Y(IW_TBL_TEXT_DY));
            else L = pline_of(w.y);
            if (w.ln.tbl != IW_LT_DEF) {
                uint32_t before[PL], k;
                int has = 0;
                for (k = 0; k < PL; k++) before[k] = g_pg[L < IW_PG_LINES ? L : IW_PG_LINES].len[k];
                print_line(d, &w.ln, L);
                for (k = 0; k < PL; k++) if (L <= IW_PG_LINES && g_pg[L].len[k] != before[k]) has = 1;
                if (has && L > last_body) last_body = L;
            }
            for (p = w.ln.start; p < w.ln.end && p < d->len; p = iw_unit_next(d, p)) {
                if (is_note(d, p)) {
                    if (byte_at(d, p + 1u) == (int)IW_NOTE_FOOT) { if (nnotes < NOTES_MAX) notes[nnotes++] = p; }
                    else ends++;
                }
            }
            hf_track(d, w.ln.start, w.ln.end, &track, 0);
            more = iw__walk_next(e, &w);
            if (!more || w.page != page) break;
        }
        print_hf(d, &pg, page, 0);
        if (nnotes) {
            /* the separator, then each note after a blank line, the last on
             * the page's last text line (PRTMINT36, PRTMINT42) */
            uint32_t total = 1, bottom, L, at, len, x0, rm = IW_WP5_RIGHT_WPU, fs;
            static char lead[16];
            uint8_t occ[4], lines[4];
            for (i = 0; i < 4u; i++) {
                occ[i] = pg.at[i] != NONE ? (uint8_t)byte_at(d, pg.at[i] + IW_WP5_VAR_HDR + IW_HF_OLD_LEN) : 0u;
                lines[i] = pg.at[i] != NONE ? (uint8_t)(rd16(d, pg.at[i] + IW_WP5_VAR_HDR + IW_HF_OLD_LEN + 1u) /
                                                       IW_WP5_WPU_PER_LINE) : 0u;
            }
            fs = iw__hf_space(occ, lines, 1, (int)(page & 1u));
            for (i = 0; i < nnotes; i++) {
                uint32_t v = iw_note_number(d, notes[i]), k, dg = iw__digits(v);
                len = note_text(d, notes[i], &at);
                for (k = 0; k < dg; k++) { lead[dg - 1u - k] = (char)('0' + v % 10u); v /= 10u; }
                lead[dg] = 0;
                total += 1u + print_note(d, at, len, IW_WP5_LEFT_WPU + IW_NOTE_INDENT_COLS * IW_PRT_COL_WPU, lead, 0, 0, rm);
            }
            bottom = IW_PG_TOP_LINES + IW_PG_TEXT_LINES - fs / IW_PRT_LINE_WPU;
            L = bottom + 1u - total;
#ifdef IWORD_MUT_NOTE_TOP
            /* MUTANT (Rule 6; test-iword-p4-mutant): the notes printed right
             * under the text, not at the page's bottom. NEVER real. */
            L = last_body + 2u;
#endif
            g_pg[L].raw = 1;
            L += 2u;
            for (i = 0; i < nnotes; i++) {
                uint32_t v = iw_note_number(d, notes[i]), k, dg = iw__digits(v);
                len = note_text(d, notes[i], &at);
                for (k = 0; k < dg; k++) { lead[dg - 1u - k] = (char)('0' + v % 10u); v /= 10u; }
                lead[dg] = 0;
                x0 = IW_WP5_LEFT_WPU + IW_NOTE_INDENT_COLS * IW_PRT_COL_WPU;
                L += print_note(d, at, len, x0, lead, L, 1, rm) + 1u;
            }
            x0 = 0;
            (void)x0;
        }
        page_emit(&o);
    }
    if (ends) {
        /* the endnotes on a page of their own after the last (PRTMINT36 page
         * 2: "1.End note." on its first text line), a blank line between
         * [inferred: one endnote minted] */
        uint32_t L = IW_PG_TOP_LINES + 1u, p, at, len;
        static char lead[16];
        page_clear();
        for (p = 0; p < d->len; p = iw_unit_next(d, p)) {
            if (!is_note(d, p) || byte_at(d, p + 1u) != (int)IW_NOTE_END) continue;
            {
                uint32_t v = iw_note_number(d, p), k, dg = iw__digits(v);
                for (k = 0; k < dg; k++) { lead[dg - 1u - k] = (char)('0' + v % 10u); v /= 10u; }
                lead[dg] = '.';
                lead[dg + 1u] = 0;
            }
            len = note_text(d, p, &at);
            L += print_note(d, at, len, IW_WP5_LEFT_WPU, lead, L, 1, IW_WP5_RIGHT_WPU) + 1u;
        }
        page_emit(&o);
    }
    iw__print_wrap(0);
    flush(&o);
    return o.rc;
#undef o
}
