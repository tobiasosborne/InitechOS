/*
 * os/iword/core/edit.c -- InitechWord's word wrap, line layout and editor
 * (THE ARTIFACT; bead initech-l4tj, IWORD P1; tabs, indents, hard pages and
 * WordPerfect's bookkeeping: bead initech-jojk, IWORD P2 slice 2).
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
 * would make it wider than the width (an over-long word). COLUMNS (bead
 * initech-jojk, P2 slice 2): text and the hard space take one; a TAB or an
 * INDENT moves to the next tab stop (every IW_WP5_TAB_EVERY columns, the
 * ruler's marks; MINT09 "abc" Tab -> column 5, "abcde" Tab -> column 10); an
 * indent also sets the left margin the rest of its paragraph wraps at (MINT09
 * 08_edit_top: "the indent here." under "Indented", both at column 5); every
 * other code is zero width. Wrap is measured in those columns: a line holds
 * what ends at or before the right margin (column `width`; MINT09 @0x1D7:
 * "...wraps under" ends at column 65 under a column-5 indent). PAGES:
 * IW_WP5_LINES_PER_PAGE lines a page, a hard page starts the next one (the
 * status line's Pg / Ln).
 *
 * BOOKKEEPING (iw_ed_fixup). WordPerfect writes -- and REWRITES as the text
 * moves (MINT10) -- the geometry inside its tab / indent / hard-page codes,
 * and two codes it never shows: C6 before the HRt of an indented paragraph,
 * D4/00 before every hard page (spec/iword/wp5_format.h "tabs, indents and
 * hard pages"; MINT03, MINT09). iw_ed_fixup does the same after every edit.
 *
 * CURSOR. Always a unit boundary. On a line that ends with a terminator the
 * cursor may sit ON the terminator (the end of the line); on a line the
 * layout broke it may not (that position is the next line's start). It
 * never rests between an unseen C6 / D4 and its HRt / HPg: Left, Right and
 * End step over them.
 *
 * Freestanding (no libc). Deterministic. ASCII-clean.
 */
#include "iword.h"
#include "iword/wp5_format.h"

#define NONE 0xFFFFFFFFu

/* The next tab stop after column c (the default stops every 0.5"). */
static uint32_t next_stop(uint32_t c)
{
#ifdef IWORD_MUT_TAB_FIXED
    /* MUTANT (Rule 6; test-iword-codes-mutant, test-iword-wp5-roundtrip-
     * mutant): a tab is always five columns wide, not "to the next stop".
     * NEVER real. */
    return c + IW_WP5_TAB_EVERY;
#else
    return (c / IW_WP5_TAB_EVERY + 1u) * IW_WP5_TAB_EVERY;
#endif
}

/* The column after a unit of `kind` that starts at column c. */
static uint32_t advance(int kind, uint32_t c)
{
    if (kind == IW_U_TEXT || kind == IW_U_HSPACE) return c + 1u;
    if (kind == IW_U_TAB || kind == IW_U_INDENT) return next_stop(c);
    return c;
}

static int is_hidden(const iw_doc_t *d, uint32_t p)
{
    return p < d->len && iw_unit_kind(d, p) == IW_U_HIDDEN;
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
    uint32_t p, end, m = 0;
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
        /* c: the column, from the line's margin m; mq: the margin the
         * indents before q set -- the next line's, should it break at q */
        uint32_t c = m, q = p, lastfit = NONE, brk = NONE, mq = m, mfit = m, mbrk = m;
        while (q < end) {
            int k = iw_unit_kind(d, q);
            if (breakable(d, q, k)) {
                if (c <= width) { lastfit = q; mfit = mq; }
                else if (lastfit != NONE) { brk = lastfit; mbrk = mfit; break; }
                else { brk = q; mbrk = mq; break; }
            }
            c = (k == IW_U_SRT) ? c + 1u : advance(k, c);
            if (k == IW_U_INDENT) mq = c;
            q = iw_unit_next(d, q);
        }
        if (brk == NONE && c > width && lastfit != NONE) { brk = lastfit; mbrk = mfit; }
        if (brk == NONE) return unsoft(d, p, end);      /* the last line */
        rc = unsoft(d, p, brk);
        if (rc == IW_OK && iw_unit_kind(d, brk) != IW_U_SRT) rc = iw_doc_set(d, brk, IW_WP5_SRT);
        if (rc != IW_OK) return rc;
        p = iw_unit_next(d, brk);
        m = mbrk;
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
/* The line that starts at `start` at column col0 (its paragraph's margin). */
static void line_from(const iw_doc_t *d, uint32_t start, uint32_t col0, uint32_t width, iw_line_t *ln)
{
    uint32_t p = start, c = col0, m = col0;
    ln->start = start;
    ln->pad = 0;
    ln->col0 = (uint16_t)col0;
    while (p < d->len) {
        int k = iw_unit_kind(d, p);
        if (is_term(k)) {
            ln->text_end = p;
            ln->end = iw_unit_next(d, p);
            ln->cols = (uint16_t)c;
            ln->term = (uint8_t)k;
            ln->margin = (uint16_t)(k == IW_U_SRT ? m : 0u);
            return;
        }
        /* the layout's own break (never before the line's first unit, so a
         * margin wider than the line still makes progress) */
        if ((k == IW_U_TEXT || k == IW_U_HSPACE) && c + 1u > width && p > start) break;
        c = advance(k, c);
        if (k == IW_U_INDENT) m = c;
        p = iw_unit_next(d, p);
    }
    ln->text_end = p;
    ln->end = p;
    ln->cols = (uint16_t)c;
    ln->term = IW_U_TEXT;
    ln->margin = (uint16_t)m;
}

void iw_line_at(const iw_doc_t *d, uint32_t start, uint32_t width, iw_line_t *ln)
{
    /* the margin at `start`: walk its paragraph's lines up to it */
    uint32_t p = para_start(d, start), m = 0;
    while (p < start) {
        iw_line_t t;
        line_from(d, p, m, width, &t);
        if (t.end > start || t.end <= p) break;
        m = t.margin;
        p = t.end;
    }
    line_from(d, start, m, width, ln);
}

uint32_t iw_line_col(const iw_doc_t *d, const iw_line_t *ln, uint32_t pos)
{
    uint32_t p = ln->start, c = ln->col0;
    while (p < pos && p < d->len) {
        c = advance(iw_unit_kind(d, p), c);
        p = iw_unit_next(d, p);
    }
    return c;
}

static int is_last(const iw_doc_t *d, const iw_line_t *ln)
{
    return ln->term == IW_U_TEXT && ln->end >= d->len;
}

/* The furthest cursor position on a line. */
static uint32_t line_max(const iw_doc_t *d, const iw_line_t *ln)
{
    if (ln->term != IW_U_TEXT || is_last(d, ln)) {
        /* before the codes WP keeps unseen in front of the HRt / HPg */
        uint32_t q = ln->text_end;
        while (q > ln->start) {
            uint32_t r = iw_unit_prev(d, q);
            if (!is_hidden(d, r)) break;
            q = r;
        }
        return q;
    }
    return ln->end > ln->start ? iw_unit_prev(d, ln->end) : ln->start;
}

/* Walk the lines from the top. visit() returns nonzero to stop. */
typedef struct walk {
    iw_line_t ln;
    uint32_t  index, page, page_line;
} walk_t;

static void walk_first(const iw_ed_t *e, walk_t *w)
{
    line_from(&e->doc, 0, 0, e->width, &w->ln);
    w->index = 0;
    w->page = 1;
    w->page_line = 1;
}

/* Advance to the next line; 0 at the last line. */
static int walk_next(const iw_ed_t *e, walk_t *w)
{
    uint8_t term = w->ln.term;
    if (is_last(&e->doc, &w->ln)) return 0;
    line_from(&e->doc, w->ln.end, w->ln.margin, e->width, &w->ln);
    w->index++;
    if (term == IW_U_HPG || w->page_line >= IW_WP5_LINES_PER_PAGE) { w->page++; w->page_line = 1; }
    else w->page_line++;
    return 1;
}

static int in_line(const iw_doc_t *d, const iw_line_t *ln, uint32_t pos)
{
    return pos < ln->end || is_last(d, ln) || (ln->term != IW_U_TEXT && pos == ln->text_end);
}

void iw_ed_where(const iw_ed_t *e, iw_where_t *wh)
{
    walk_t w;
    walk_first(e, &w);
    while (!in_line(&e->doc, &w.ln, e->cur) && walk_next(e, &w)) { }
    wh->line = w.index;
    wh->line_start = w.ln.start;
    wh->col = iw_line_col(&e->doc, &w.ln, e->cur);
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
    uint32_t p = ln->start, c = ln->col0, max = line_max(d, ln);
    while (p < max && c < col) {
        uint32_t nc = advance(iw_unit_kind(d, p), c);
        if (nc > col) break;
        c = nc;
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
    e->reveal = 0;
    e->ask_del = 0;
    e->ask = NONE;
    e->blk = IW_NO_BLOCK;
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

/* Backspace / Delete on the opaque code at q: an attribute code goes with
 * its partner in Reveal Codes (MINT08) and is ASKED about on the editing
 * screen (MINT11 01_bs_prompt "Delete [BOLD]? No (Yes)", 02_del_prompt
 * "Delete [bold]? No (Yes)"); any other opaque code is stepped over (P1
 * protection: no mint shows what WP does with it). */
static int on_code(iw_ed_t *e, uint32_t q, int del)
{
    if (iw_code_attr(&e->doc, q, 0) >= 0) {
        if (e->reveal) return iw_ed_kill_code(e, q);
#ifdef IWORD_MUT_NO_ASK
        /* MUTANT (Rule 6; test-iword-codes-mutant): the editing screen keeps
         * P1's silent step-over -- no question is ever asked. NEVER real. */
        e->cur = del ? iw_unit_next(&e->doc, q) : q;
        return IW_E_PROTECTED;
#endif
        e->ask = q;
        e->ask_del = (uint8_t)(del != 0);
        return IW_E_ASK;
    }
    e->cur = del ? iw_unit_next(&e->doc, q) : q;
    return IW_E_PROTECTED;
}

int iw_ed_backspace(iw_ed_t *e)
{
    iw_doc_t *d = &e->doc;
    uint32_t q;
    int k, rc;
    e->goal = NONE;
    if (e->cur == 0u) return IW_OK;
    q = iw_unit_prev(d, e->cur);
    while (q > 0u && is_hidden(d, q)) q = iw_unit_prev(d, q);
    k = iw_unit_kind(d, q);
    if (k == IW_U_HIDDEN) { e->cur = q; return IW_E_PROTECTED; }
    if (k == IW_U_CODE) return on_code(e, q, 0);
    /* text, a return, a tab, an indent or a hard page: deleted without a
     * question (MINT11 05_bs_tab / 06_bs_hpg, MINT12 13_bs_indent); a hard
     * page's D4/00 group goes with it (iw_ed_fixup; MINT11 "Pages") */
    rc = iw_doc_delete(d, q, iw_unit_next(d, q) - q);
    if (rc != IW_OK) return rc;
    e->cur = q;
    e->dirty = 1;
    return reflow_at(e, q);
}

int iw_ed_delete(iw_ed_t *e)
{
    iw_doc_t *d = &e->doc;
    uint32_t q = e->cur;
    int k, rc;
    e->goal = NONE;
    while (q < d->len && is_hidden(d, q)) q = iw_unit_next(d, q);
    if (q >= d->len) return IW_OK;
    k = iw_unit_kind(d, q);
    if (k == IW_U_CODE) return on_code(e, q, 1);
    rc = iw_doc_delete(d, q, iw_unit_next(d, q) - q);
    if (rc != IW_OK) return rc;
    e->dirty = 1;
    return reflow_at(e, e->cur);
}

int iw_ed_answer(iw_ed_t *e, int yes)
{
    uint32_t q = e->ask;
    e->goal = NONE;
    e->ask = NONE;
    if (q == NONE || iw_code_attr(&e->doc, q, 0) < 0) return IW_E_RANGE;
    /* Yes: the code and its partner (MINT11 "Alpha" F6 Backspace y, "Delta"
     * F6 Delete y -- both pairs gone, the text joined) */
    if (yes) return iw_ed_kill_code(e, q);
#ifdef IWORD_MUT_ASK_STAY
    /* MUTANT (Rule 6; test-iword-codes-mutant): "No" leaves the cursor where
     * it was. NEVER real. */
    return IW_OK;
#endif
    /* No: over the code -- before it for Backspace ("Gamma" F6 BS n "y" ->
     * "Gammay[BOLD][bold]"), past it for Delete ("Beta" F6 Del n "x" ->
     * "Beta[BOLD][bold]x") */
    e->cur = e->ask_del ? iw_unit_next(&e->doc, q) : q;
    return IW_OK;
}

void iw_ed_left(iw_ed_t *e)
{
    iw_doc_t *d = &e->doc;
    uint32_t q;
    e->goal = NONE;
    if (e->cur == 0u) return;
    q = iw_unit_prev(d, e->cur);
    while (q > 0u) {                      /* over the unseen codes too */
        uint32_t r = iw_unit_prev(d, q);
        if (!is_hidden(d, r)) break;
        q = r;
    }
    e->cur = q;
}

void iw_ed_right(iw_ed_t *e)
{
    iw_doc_t *d = &e->doc;
    uint32_t q = e->cur;
    e->goal = NONE;
    while (is_hidden(d, q)) q = iw_unit_next(d, q);
    if (q < d->len) q = iw_unit_next(d, q);
    e->cur = q;
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
    walk_t w;
    uint32_t p, max;
    e->goal = NONE;
    iw_ed_where(e, &wh);
    p = wh.line_start;
    /* WordPerfect's Home, Left: the line's first character, past the
     * zero-width codes in front of it (MINT15 01_home_left_reveal: on "v"
     * after [BOLD]); a tab or an indent is a column and is not passed */
    if (find_line(e, wh.line, &w)) {
        max = line_max(&e->doc, &w.ln);
        while (p < max && iw_unit_kind(&e->doc, p) == IW_U_CODE) p = iw_unit_next(&e->doc, p);
    }
    e->cur = p;
}

void iw_ed_line_end(iw_ed_t *e)
{
    iw_where_t wh;
    walk_t w;
    e->goal = NONE;
    iw_ed_where(e, &wh);
    if (find_line(e, wh.line, &w)) e->cur = line_max(&e->doc, &w.ln);
}

/* ---- tabs, indents, hard pages (bead initech-jojk, P2 slice 2) ---------------- */
static int insert_code(iw_ed_t *e, const uint8_t *g, uint32_t n, int ends_para)
{
    int rc = iw_doc_insert(&e->doc, e->cur, g, n);
    if (rc != IW_OK) return rc;
    e->cur += n;
    e->dirty = 1;
    e->goal = NONE;
    if (ends_para) {
        rc = reflow_at(e, e->cur - 1u);  /* the paragraph the hard page ends */
        if (rc != IW_OK) return rc;
    }
    return reflow_at(e, e->cur);
}

int iw_ed_tab(iw_ed_t *e)
{
    /* the payload is the layout's (iw_ed_fixup); MINT01 / MINT09 */
    static const uint8_t g[IW_WP5_C1_LEN] = { IW_WP5_C1, IW_WP5_C1_TAB, 0, 0, 0, 0, 0, 0, IW_WP5_C1 };
    return insert_code(e, g, sizeof g, 0);
}

int iw_ed_indent(iw_ed_t *e)
{
    static const uint8_t g[IW_WP5_INDENT_LEN] = { IW_WP5_INDENT, 0x00,
        (uint8_t)(IW_WP5_INDENT_STEP & 0xFFu), (uint8_t)(IW_WP5_INDENT_STEP >> 8), 0, 0, 0, 0, 0, 0, IW_WP5_INDENT };
    return insert_code(e, g, sizeof g, 0);
}

int iw_ed_hpage(iw_ed_t *e)
{
    /* Ctrl-Enter: D4/00 then 0x0C (MINT09 @0x21B / @0x23B) */
    static const uint8_t pg[IW_WP5_PAGE_GRP_LEN] = IW_WP5_PAGE_GRP_INIT;
    uint8_t g[IW_WP5_PAGE_GRP_LEN + 1u];
    uint32_t i;
    for (i = 0; i < IW_WP5_PAGE_GRP_LEN; i++) g[i] = pg[i];
    g[IW_WP5_PAGE_GRP_LEN] = IW_WP5_HPG;
#ifdef IWORD_MUT_NO_D4
    /* MUTANT (Rule 6; test-iword-wp5-roundtrip-mutant, test-flair-iword-
     * block-mutant): the hard page goes in bare, without its D4/00 group (and
     * iw_ed_fixup does not add one). NEVER real. */
    return insert_code(e, g + IW_WP5_PAGE_GRP_LEN, 1u, 1);
#endif
    return insert_code(e, g, sizeof g, 1);
}

/* ---- iw_ed_fixup: WordPerfect's bookkeeping ---------------------------------------- */
static int is_page_grp(const iw_doc_t *d, uint32_t p)
{
    return iw_doc_byte(d, p) == (int)IW_WP5_PAGE_GRP && iw_doc_byte(d, p + 1u) == (int)IW_WP5_PAGE_SUB;
}

static int room_for(const iw_doc_t *d, uint32_t add)
{
    return d->npc + 3u <= d->pc_cap && d->add_len + add <= d->add_cap && d->len + add <= d->max_len;
}

static uint32_t pos_del(uint32_t at, uint32_t p, uint32_t n)
{
    if (at == NONE || at <= p) return at;
    return at >= p + n ? at - n : p;
}

static uint32_t pos_ins(uint32_t at, uint32_t p, uint32_t n)
{
    return (at != NONE && at > p) ? at + n : at;
}

static int fix_delete(iw_ed_t *e, uint32_t p, uint32_t n)
{
    int rc;
    if (!room_for(&e->doc, 0u)) return IW_E_FULL;
    rc = iw_doc_delete(&e->doc, p, n);
    if (rc != IW_OK) return rc;
    e->cur = pos_del(e->cur, p, n);
    e->blk = pos_del(e->blk, p, n);
    return IW_OK;
}

static int fix_insert(iw_ed_t *e, uint32_t p, const uint8_t *g, uint32_t n)
{
    int rc;
    if (!room_for(&e->doc, n)) return IW_E_FULL;
    rc = iw_doc_insert(&e->doc, p, g, n);
    if (rc != IW_OK) return rc;
    e->cur = pos_ins(e->cur, p, n);       /* a cursor right before them stays */
    e->blk = pos_ins(e->blk, p, n);
    return IW_OK;
}

/* Every hard page has its D4/00 right before it; every HRt that ends a
 * paragraph holding an indent has its C6 right before it; no other D4/00 or
 * C6 remains (MINT11: Backspace on the hard page took its group too). */
static int fix_structure(iw_ed_t *e)
{
    static const uint8_t pg[IW_WP5_PAGE_GRP_LEN] = IW_WP5_PAGE_GRP_INIT;
    static const uint8_t c6[IW_WP5_END_INDENT_LEN] = { IW_WP5_END_INDENT,
        (uint8_t)(IW_WP5_RIGHT_WPU & 0xFFu), (uint8_t)(IW_WP5_RIGHT_WPU >> 8), 0, 0, IW_WP5_END_INDENT };
    iw_doc_t *d = &e->doc;
    uint32_t p = 0;
    int indent = 0, rc;
    (void)pg;
    while (p < d->len) {
        int k = iw_unit_kind(d, p);
        uint32_t n = iw_unit_next(d, p);
        if (k == IW_U_HIDDEN) {
            int keep = is_page_grp(d, p) ? (n < d->len && iw_doc_byte(d, n) == (int)IW_WP5_HPG)
                                         : (indent && n < d->len && iw_doc_byte(d, n) == (int)IW_WP5_HRT);
            if (!keep) {
                rc = fix_delete(e, p, n - p);
                if (rc != IW_OK) return rc;
                continue;
            }
        } else if (k == IW_U_HPG) {
#ifndef IWORD_MUT_NO_D4
            /* (IWORD_MUT_NO_D4, see iw_ed_hpage: no group is added) */
            if (p == 0u || !is_page_grp(d, iw_unit_prev(d, p))) {
                rc = fix_insert(e, p, pg, sizeof pg);
                if (rc != IW_OK) return rc;
                p += sizeof pg;
            }
#endif
            indent = 0;
            n = iw_unit_next(d, p);
        } else if (k == IW_U_HRT) {
            if (indent && (p == 0u || iw_doc_byte(d, iw_unit_prev(d, p)) != (int)IW_WP5_END_INDENT)) {
                rc = fix_insert(e, p, c6, sizeof c6);
                if (rc != IW_OK) return rc;
                p += sizeof c6;
            }
            indent = 0;
            n = iw_unit_next(d, p);
        } else if (k == IW_U_INDENT) {
            indent = 1;
        }
        p = n;
    }
    return IW_OK;
}

/* Overwrite n bytes at pos when they differ (no position moves; a run of
 * iw_doc_set costs at most two pieces, none in the add buffer). */
static int put_bytes(iw_doc_t *d, uint32_t pos, const uint8_t *b, uint32_t n)
{
    uint8_t have[16];
    uint32_t i;
    int rc;
    if (iw_doc_read(d, pos, have, n) != n) return IW_E_RANGE;
    for (i = 0; i < n; i++) {
        if (have[i] == b[i]) continue;
        rc = iw_doc_set(d, pos + i, b[i]);
        if (rc != IW_OK) return rc;
    }
    return IW_OK;
}

static void u16put(uint8_t *g, uint32_t v)
{
    g[0] = (uint8_t)(v & 0xFFu);
    g[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static uint32_t wpu(uint32_t col) { return IW_WP5_LEFT_WPU + IW_WP5_WPU_PER_COL * col; }

/* The geometry inside the codes, from the layout (spec/iword/wp5_format.h). */
static int fix_payloads(iw_ed_t *e)
{
    iw_doc_t *d = &e->doc;
    walk_t w;
    int geo = 1, rc = IW_OK;
    walk_first(e, &w);
    for (;;) {
        uint32_t p = w.ln.start, c = w.ln.col0, m = w.ln.col0;
        while (p < w.ln.end && rc == IW_OK) {
            int k = iw_unit_kind(d, p), b0 = iw_doc_byte(d, p);
            uint8_t g[10];
            if (geo && (k == IW_U_TAB || k == IW_U_INDENT)) {
                uint32_t s = next_stop(c), o = 0;
                if (k == IW_U_TAB) g[o++] = IW_WP5_C1_TAB;
                else { g[o++] = 0x00; u16put(g + o, IW_WP5_INDENT_STEP); o += 2u; }
                u16put(g + o, wpu(s)); o += 2u;
                u16put(g + o, wpu(s)); o += 2u;
                u16put(g + o, IW_WP5_LEFT_TENTHS + s); o += 2u;
                rc = put_bytes(d, p + 1u, g, o);
            } else if (geo && k == IW_U_HIDDEN && b0 == (int)IW_WP5_END_INDENT) {
                u16put(g, IW_WP5_RIGHT_WPU);
                u16put(g + 2, wpu(m));
                rc = put_bytes(d, p + 1u, g, 4u);
            } else if (geo && k == IW_U_HIDDEN) {
                u16put(g, (w.page_line - 1u) * IW_WP5_WPU_PER_LINE);
                u16put(g + 2, w.page);
                rc = put_bytes(d, p + IW_WP5_PAGE_VPOS_OFF, g, 4u);
            }
            if (b0 == (int)IW_WP5_MARGIN_GRP && iw_doc_byte(d, p + 1u) == (int)IW_WP5_MARGIN_SUB) geo = 0;
            c = advance(k, c);
            if (k == IW_U_INDENT) m = c;
            p = iw_unit_next(d, p);
        }
        if (rc != IW_OK) return rc;
        if (!walk_next(e, &w)) return IW_OK;
    }
}

/* A position right after an unseen code (between it and its HRt / HPg) is
 * moved in front of it: the end of the line's text. */
static uint32_t before_hidden(const iw_doc_t *d, uint32_t at)
{
    while (at != NONE && at > 0u) {
        uint32_t r = iw_unit_prev(d, at);
        if (!is_hidden(d, r)) break;
        at = r;
    }
    return at;
}

int iw_ed_fixup(iw_ed_t *e)
{
    int rc = fix_structure(e);
    if (rc == IW_OK) rc = fix_payloads(e);
    e->cur = before_hidden(&e->doc, e->cur);
    e->blk = before_hidden(&e->doc, e->blk);
    return rc;
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
