/*
 * os/iword/core/edit.c -- InitechWord's word wrap, line layout and editor
 * (THE ARTIFACT; bead initech-l4tj, IWORD P1).
 *
 * WORD WRAP (iword.h "WRAP"). Ref: ../wordperfect51-decomp specs/file-
 * formats/wp5-body-codes.md Sec 2 -- MINT01.WP5 @0x188 `6c 0d 57`: WP 5.1
 * stores the line end of a wrapped paragraph as the SOFT RETURN 0x0D IN
 * PLACE OF the space it broke at, 65 characters into the line (the 1"/1"
 * default margins at 10 pitch, spec/iword/wp5_format.h IW_WP5_WRAP_COLS).
 * The rule implemented (stated, [inferred] beyond that one instance): from
 * the line start, if the rest of the paragraph fits the width it is the
 * last line; otherwise the line ends at the LAST space whose preceding text
 * fits, and that space becomes the SRt; with no such space (a word longer
 * than the line) at the first space after it. Every other SRt in the
 * paragraph is a space again. test-iword-wp5-roundtrip grades the rule
 * against WP's own bytes (re-wrapping the goldens changes nothing; typing
 * MINT01's sentence gives MINT01's bytes); test-iword-core against an
 * independent reference wrapper.
 *
 * LAYOUT. A line ends after a hard return, soft return or hard page, or --
 * the layout's own break, no byte involved -- before a one-column unit that
 * would make it wider than the width (an over-long word). Codes are zero
 * width in P1. PAGES: IW_WP5_LINES_PER_PAGE lines a page, a hard page starts
 * the next one (the status line's Pg / Ln).
 *
 * CURSOR. Always a unit boundary. On a line that ends with a terminator the
 * cursor may sit ON the terminator (the end of the line); on a line the
 * layout broke it may not (that position is the next line's start).
 *
 * Freestanding (no libc). Deterministic. ASCII-clean.
 */
#include "iword.h"
#include "iword/wp5_format.h"

#define NONE 0xFFFFFFFFu

static uint32_t unit_width(int kind)
{
    return (kind == IW_U_TEXT || kind == IW_U_HSPACE) ? 1u : 0u;
}

static int is_term(int kind)
{
    return kind == IW_U_HRT || kind == IW_U_SRT || kind == IW_U_HPG;
}

static int is_para_end(int kind)
{
    return kind == IW_U_HRT || kind == IW_U_HPG;
}

/* ---- paragraphs ------------------------------------------------------------ */
static uint32_t para_start(const iw_doc_t *d, uint32_t pos)
{
    while (pos > 0u) {
        uint32_t q = iw_unit_prev(d, pos);
        if (is_para_end(iw_unit_kind(d, q))) return pos;
        pos = q;
    }
    return 0;
}

static uint32_t para_end(const iw_doc_t *d, uint32_t pos)
{
    while (pos < d->len && !is_para_end(iw_unit_kind(d, pos))) pos = iw_unit_next(d, pos);
    return pos;
}

static int breakable(const iw_doc_t *d, uint32_t q, int kind)
{
    return kind == IW_U_SRT || (kind == IW_U_TEXT && iw_doc_byte(d, q) == ' ');
}

/* Make every SRt in [p, q) a space again. */
static int unsoft(iw_doc_t *d, uint32_t p, uint32_t q)
{
#ifdef IWORD_MUT_REFLOW_STALE
    /* MUTANT (Rule 6; test-iword-core-mutant): old soft returns are kept
     * where they were, so a re-wrapped paragraph keeps stale line ends.
     * NEVER real. */
    (void)d; (void)p; (void)q;
    return IW_OK;
#else
    while (p < q) {
        if (iw_unit_kind(d, p) == IW_U_SRT) {
            int rc = iw_doc_set(d, p, (uint8_t)' ');
            if (rc != IW_OK) return rc;
        }
        p = iw_unit_next(d, p);
    }
    return IW_OK;
#endif
}

int iw_reflow_para(iw_doc_t *d, uint32_t pos, uint32_t width)
{
    uint32_t p, end;
    int rc;
#ifdef IWORD_MUT_NO_WRAP
    /* MUTANT (Rule 6; test-flair-iword-mutant): word wrap is off -- no soft
     * return is ever placed. NEVER real. */
    (void)d; (void)pos; (void)width;
    return IW_OK;
#endif
#ifdef IWORD_MUT_WRAP_EARLY
    /* MUTANT (Rule 6; test-iword-wp5-roundtrip-mutant): a line ends one
     * column short of the margin. NEVER real. */
    if (width > 1u) width--;
#endif
    if (pos > d->len) return IW_E_RANGE;
    p = para_start(d, pos);
    end = para_end(d, pos);
    for (;;) {
        uint32_t c = 0, q = p, lastfit = NONE, brk = NONE;
        while (q < end) {
            int k = iw_unit_kind(d, q);
            if (breakable(d, q, k)) {
                if (c <= width) lastfit = q;
                else { brk = lastfit != NONE ? lastfit : q; break; }
            }
            c += (k == IW_U_SRT) ? 1u : unit_width(k);
            q = iw_unit_next(d, q);
        }
        if (brk == NONE && c > width && lastfit != NONE) brk = lastfit;
        if (brk == NONE) return unsoft(d, p, end);      /* the last line */
        rc = unsoft(d, p, brk);
        if (rc == IW_OK && iw_unit_kind(d, brk) != IW_U_SRT) rc = iw_doc_set(d, brk, IW_WP5_SRT);
        if (rc != IW_OK) return rc;
        p = iw_unit_next(d, brk);
    }
}

int iw_reflow_all(iw_doc_t *d, uint32_t width)
{
    uint32_t p = 0;
    for (;;) {
        uint32_t e;
        int rc = iw_reflow_para(d, p, width);
        if (rc != IW_OK) return rc;
        e = para_end(d, p);
        if (e >= d->len) return IW_OK;
        p = iw_unit_next(d, e);
    }
}

/* ---- layout ------------------------------------------------------------------ */
void iw_line_at(const iw_doc_t *d, uint32_t start, uint32_t width, iw_line_t *ln)
{
    uint32_t p = start, cols = 0;
    ln->start = start;
    ln->pad = 0;
    while (p < d->len) {
        int k = iw_unit_kind(d, p);
        uint32_t w = unit_width(k);
        if (is_term(k)) {
            ln->text_end = p;
            ln->end = iw_unit_next(d, p);
            ln->cols = (uint16_t)cols;
            ln->term = (uint8_t)k;
            return;
        }
        if (w && cols + w > width) break;           /* the layout's own break */
        cols += w;
        p = iw_unit_next(d, p);
    }
    ln->text_end = p;
    ln->end = p;
    ln->cols = (uint16_t)cols;
    ln->term = IW_U_TEXT;
}

static int is_last(const iw_doc_t *d, const iw_line_t *ln)
{
    return ln->term == IW_U_TEXT && ln->end >= d->len;
}

/* The furthest cursor position on a line. */
static uint32_t line_max(const iw_doc_t *d, const iw_line_t *ln)
{
    if (ln->term != IW_U_TEXT || is_last(d, ln)) return ln->text_end;
    return ln->end > ln->start ? iw_unit_prev(d, ln->end) : ln->start;
}

/* Walk the lines from the top. visit() returns nonzero to stop. */
typedef struct walk {
    iw_line_t ln;
    uint32_t  index, page, page_line;
} walk_t;

static void walk_first(const iw_ed_t *e, walk_t *w)
{
    iw_line_at(&e->doc, 0, e->width, &w->ln);
    w->index = 0;
    w->page = 1;
    w->page_line = 1;
}

/* Advance to the next line; 0 at the last line. */
static int walk_next(const iw_ed_t *e, walk_t *w)
{
    uint8_t term = w->ln.term;
    if (is_last(&e->doc, &w->ln)) return 0;
    iw_line_at(&e->doc, w->ln.end, e->width, &w->ln);
    w->index++;
    if (term == IW_U_HPG || w->page_line >= IW_WP5_LINES_PER_PAGE) { w->page++; w->page_line = 1; }
    else w->page_line++;
    return 1;
}

static int in_line(const iw_doc_t *d, const iw_line_t *ln, uint32_t pos)
{
    return pos < ln->end || is_last(d, ln) || (ln->term != IW_U_TEXT && pos == ln->text_end);
}

static uint32_t cols_between(const iw_doc_t *d, uint32_t p, uint32_t q)
{
    uint32_t c = 0;
    while (p < q) { c += unit_width(iw_unit_kind(d, p)); p = iw_unit_next(d, p); }
    return c;
}

void iw_ed_where(const iw_ed_t *e, iw_where_t *wh)
{
    walk_t w;
    walk_first(e, &w);
    while (!in_line(&e->doc, &w.ln, e->cur) && walk_next(e, &w)) { }
    wh->line = w.index;
    wh->line_start = w.ln.start;
    wh->col = cols_between(&e->doc, w.ln.start, e->cur);
    wh->page = w.page;
    wh->page_line = w.page_line;
}

static int find_line(const iw_ed_t *e, uint32_t line, walk_t *w)
{
    walk_first(e, w);
    while (w->index < line) if (!walk_next(e, w)) return 0;
    return 1;
}

static uint32_t pos_in_line(const iw_doc_t *d, const iw_line_t *ln, uint32_t col)
{
    uint32_t p = ln->start, c = 0, max = line_max(d, ln);
    while (p < max && c < col) {
        uint32_t w = unit_width(iw_unit_kind(d, p));
        if (c + w > col) break;
        c += w;
        p = iw_unit_next(d, p);
    }
    return p;
}

uint32_t iw_ed_pos_at(const iw_ed_t *e, uint32_t line, uint32_t col)
{
    walk_t w;
    if (!find_line(e, line, &w)) return e->doc.len;
    return pos_in_line(&e->doc, &w.ln, col);
}

uint32_t iw_ed_lines(const iw_ed_t *e, uint32_t first, uint32_t n, iw_line_t *out)
{
    walk_t w;
    uint32_t got = 0;
    if (!find_line(e, first, &w)) return 0;
    while (got < n) {
        out[got++] = w.ln;
        if (!walk_next(e, &w)) break;
    }
    return got;
}

/* ---- the editor ---------------------------------------------------------------- */
void iw_ed_init(iw_ed_t *e, uint32_t width)
{
    e->cur = 0;
    e->width = width;
    e->goal = NONE;
    e->dirty = 0;
}

static int reflow_at(iw_ed_t *e, uint32_t pos)
{
    return iw_reflow_para(&e->doc, pos, e->width);
}

int iw_ed_type(iw_ed_t *e, uint8_t ch)
{
    int rc;
    if (ch < IW_WP5_TEXT_LO || ch > 0x7Eu) return IW_E_RANGE;
    rc = iw_doc_insert(&e->doc, e->cur, &ch, 1);
    if (rc != IW_OK) return rc;
    e->cur++;
    e->dirty = 1;
    e->goal = NONE;
    return reflow_at(e, e->cur - 1u);
}

int iw_ed_enter(iw_ed_t *e)
{
    uint8_t hrt = IW_WP5_HRT;
    int rc = iw_doc_insert(&e->doc, e->cur, &hrt, 1);
    if (rc != IW_OK) return rc;
    e->cur++;
    e->dirty = 1;
    e->goal = NONE;
    rc = reflow_at(e, e->cur - 1u);      /* the paragraph the HRt now ends */
    if (rc == IW_OK) rc = reflow_at(e, e->cur);
    return rc;
}

int iw_ed_backspace(iw_ed_t *e)
{
    uint32_t q;
    int rc;
    e->goal = NONE;
    if (e->cur == 0u) return IW_OK;
    q = iw_unit_prev(&e->doc, e->cur);
    if (iw_unit_kind(&e->doc, q) == IW_U_CODE) { e->cur = q; return IW_E_PROTECTED; }
    rc = iw_doc_delete(&e->doc, q, e->cur - q);
    if (rc != IW_OK) return rc;
    e->cur = q;
    e->dirty = 1;
    return reflow_at(e, q);
}

int iw_ed_delete(iw_ed_t *e)
{
    uint32_t q;
    int rc;
    e->goal = NONE;
    if (e->cur >= e->doc.len) return IW_OK;
    q = iw_unit_next(&e->doc, e->cur);
    if (iw_unit_kind(&e->doc, e->cur) == IW_U_CODE) { e->cur = q; return IW_E_PROTECTED; }
    rc = iw_doc_delete(&e->doc, e->cur, q - e->cur);
    if (rc != IW_OK) return rc;
    e->dirty = 1;
    return reflow_at(e, e->cur);
}

void iw_ed_left(iw_ed_t *e)
{
    e->goal = NONE;
    if (e->cur > 0u) e->cur = iw_unit_prev(&e->doc, e->cur);
}

void iw_ed_right(iw_ed_t *e)
{
    e->goal = NONE;
    if (e->cur < e->doc.len) e->cur = iw_unit_next(&e->doc, e->cur);
}

static void vertical(iw_ed_t *e, int dir)
{
    iw_where_t wh;
    walk_t w;
    iw_ed_where(e, &wh);
    if (e->goal == NONE) e->goal = wh.col;
    if (dir < 0 && wh.line == 0u) return;
    if (!find_line(e, dir < 0 ? wh.line - 1u : wh.line + 1u, &w)) return;
    e->cur = pos_in_line(&e->doc, &w.ln, e->goal);
}

void iw_ed_up(iw_ed_t *e)   { vertical(e, -1); }
void iw_ed_down(iw_ed_t *e) { vertical(e, 1); }

void iw_ed_line_home(iw_ed_t *e)
{
    iw_where_t wh;
    e->goal = NONE;
    iw_ed_where(e, &wh);
    e->cur = wh.line_start;
}

void iw_ed_line_end(iw_ed_t *e)
{
    iw_where_t wh;
    walk_t w;
    e->goal = NONE;
    iw_ed_where(e, &wh);
    if (find_line(e, wh.line, &w)) e->cur = line_max(&e->doc, &w.ln);
}

/* ---- the status line ------------------------------------------------------------ */
uint32_t iw_ln_hundredths(uint32_t page_line)
{
    uint32_t k = page_line ? page_line - 1u : 0u;
    return 100u + (k * 100u + IW_WP5_LINES_PER_INCH / 2u) / IW_WP5_LINES_PER_INCH;
}

uint32_t iw_pos_hundredths(uint32_t col)
{
    return 100u + col * 10u;
}

uint32_t iw_fmt_inches(uint32_t h, char *out)
{
    char tmp[12];
    uint32_t n = 0, ip = h / 100u, fr = h % 100u, t = 0;
    do { tmp[t++] = (char)('0' + ip % 10u); ip /= 10u; } while (ip);
    while (t) out[n++] = tmp[--t];
    if (fr) {
        out[n++] = '.';
        out[n++] = (char)('0' + fr / 10u);
        if (fr % 10u) out[n++] = (char)('0' + fr % 10u);
    }
    out[n++] = '"';
    out[n] = 0;
    return n;
}
