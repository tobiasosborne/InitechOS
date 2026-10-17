/*
 * os/iword/core/codes.c -- InitechWord's codes as first-class entities: the
 * attribute pairs (bold / underline / italic, and every other attribute
 * number a document carries), the WordPerfect 5.1 editing rules for them,
 * and the Reveal Codes rendering of a line and of the pane (THE ARTIFACT;
 * bead initech-jojk, IWORD P2 slice 1).
 *
 * THE MODEL. A code is a unit of the piece table's body like any character
 * (iword.h "UNITS"): the attribute pair is `C3 n C3` ... `C4 n C4` [B Sec 3;
 * spec/iword/wp5_format.h IW_WP5_ATTR_*]. Nothing is stored beside the body:
 * the attribute state at a position is DERIVED from the codes before it
 * (iw_attr_state), so a document WordPerfect wrote and a document typed here
 * are the same kind of thing, and the .WP5 writer has nothing to translate.
 *
 * THE RULES (iword.h "attribute codes"; Reference Manual, Attributes p. 36-37:
 * "When you select an attribute, both an on and off code are inserted, with
 * the cursor resting just before the off code ... press that keystroke once
 * again to turn off the feature, or press Right Arrow to move beyond the
 * attribute off code"; Normal Text: "This either inserts the necessary off
 * codes or moves the cursor beyond existing off codes"). What the manual
 * leaves open the real WP.EXE settled (../wordperfect51-decomp mint/
 * wpMINT06..08.steps, this bead): F6 pressed inside an underline nested in
 * bold MOVES [bold] to the cursor (MINT06 body "[BOLD]Bold[UND]both[bold]
 * under[und]"); Backspace on [italc] in Reveal Codes deletes [ITALC] too
 * (MINT08 03b_reveal_bs1.png).
 *
 * REVEAL CODES. spec/iword/reveal_codes.h holds every label and the pane
 * geometry, each transcribed from a real WP 5.1 screen.
 *
 * Freestanding C (no libc). Deterministic (Rule 11). ASCII-clean (Rule 12).
 */
#include "iword.h"
#include "iword/wp5_format.h"
#include "iword/reveal_codes.h"

#define NONE 0xFFFFFFFFu

static const uint8_t FIXED_LEN[16] = IW_WP5_FIXED_LEN_INIT;

typedef struct attr_label { uint8_t n; const char *on, *off; } attr_label_t;
static const attr_label_t ATTR_LABEL[IW_RV_ATTR_ROWS] = IW_RV_ATTR_INIT;

static uint32_t abit(uint32_t n) { return n < 32u ? 1u << n : 0u; }

/* ---- a forward unit scanner (O(1) per byte, the iterator) ---------------- */
typedef struct scan {
    iw_iter_t it;
    uint32_t  pos;           /* the next unit's start */
} scan_t;

static void scan_at(scan_t *s, const iw_doc_t *d, uint32_t pos)
{
    iw_iter_at(&s->it, d, pos);
    s->pos = pos;
}

/* The next unit: its start, length and first two bytes; 0 at the end. */
static int scan_next(scan_t *s, uint32_t *start, uint8_t *c0, uint8_t *c1)
{
    int b = iw_iter_next(&s->it), b1 = 0;
    uint32_t len = 1, i;
    if (b < 0) return 0;
    *start = s->pos;
    if (b >= (int)IW_WP5_FIXED_LO && b <= (int)IW_WP5_FIXED_HI) {
        len = FIXED_LEN[b - (int)IW_WP5_FIXED_LO];
        if (len == 0u) len = 1u;          /* never in a validated body */
    } else if (b >= (int)IW_WP5_VAR_LO) {
        int sub = iw_iter_next(&s->it), lo = iw_iter_next(&s->it), hi = iw_iter_next(&s->it);
        b1 = sub;
        len = IW_WP5_VAR_HDR + ((uint32_t)(lo < 0 ? 0 : lo) | ((uint32_t)(hi < 0 ? 0 : hi) << 8));
        for (i = 4; i < len; i++) (void)iw_iter_next(&s->it);
        *c0 = (uint8_t)b;
        *c1 = (uint8_t)b1;
        s->pos += len;
        return 1;
    }
    if (len > 1u) b1 = iw_iter_next(&s->it);
    for (i = 2; i < len; i++) (void)iw_iter_next(&s->it);
    *c0 = (uint8_t)b;
    *c1 = (uint8_t)(b1 < 0 ? 0 : b1);
    s->pos += len;
    return 1;
}

static uint32_t step(uint32_t state, uint8_t c0, uint8_t c1)
{
    if (c0 == IW_WP5_ATTR_ON)  return state | abit(c1);
    if (c0 == IW_WP5_ATTR_OFF) return state & ~abit(c1);
    return state;
}

/* ---- the attribute state ---------------------------------------------------- */
int iw_code_attr(const iw_doc_t *d, uint32_t pos, int *on)
{
    int c = iw_doc_byte(d, pos);
    if (c != (int)IW_WP5_ATTR_ON && c != (int)IW_WP5_ATTR_OFF) return -1;
    if (on) *on = c == (int)IW_WP5_ATTR_ON;
    return iw_doc_byte(d, pos + 1u);
}

uint32_t iw_attr_state(const iw_doc_t *d, uint32_t pos)
{
    scan_t s;
    uint32_t st = 0, at;
    uint8_t c0, c1;
    scan_at(&s, d, 0);
    while (s.pos < pos && scan_next(&s, &at, &c0, &c1)) st = step(st, c0, c1);
    return st;
}

uint32_t iw_attr_step(const iw_doc_t *d, uint32_t pos, uint32_t state)
{
    int on, n = iw_code_attr(d, pos, &on);
    if (n < 0) return state;
    return step(state, on ? IW_WP5_ATTR_ON : IW_WP5_ATTR_OFF, (uint8_t)n);
}

uint32_t iw_attr_faces(uint32_t st)
{
    uint32_t f = 0;
#ifndef IWORD_MUT_NO_FACE
    if (st & abit(IW_WP5_ATTR_BOLD))      f |= IW_A_BOLD;
    if (st & abit(IW_WP5_ATTR_ITALIC))    f |= IW_A_ITALIC;
    if (st & abit(IW_WP5_ATTR_UNDERLINE)) f |= IW_A_UNDERLINE;
#else
    /* MUTANT (Rule 6; test-iword-face-mutant, test-flair-iword-codes-mutant):
     * the codes are kept but never drawn -- every face is plain. NEVER real. */
    (void)st;
#endif
    return f;
}

/* The first code of attribute n at or after pos: its start (NONE if none),
 * *is_off = it is an OFF code. */
static uint32_t first_code(const iw_doc_t *d, uint32_t pos, uint8_t n, int *is_off)
{
    scan_t s;
    uint32_t at;
    uint8_t c0, c1;
    scan_at(&s, d, pos);
    while (scan_next(&s, &at, &c0, &c1)) {
        if ((c0 == IW_WP5_ATTR_ON || c0 == IW_WP5_ATTR_OFF) && c1 == n) {
            *is_off = c0 == IW_WP5_ATTR_OFF;
            return at;
        }
    }
    *is_off = 0;
    return NONE;
}

/* Room for one code insert (3 bytes, up to 2 new pieces) and up to `dels`
 * code deletes (1 new piece each), checked BEFORE any change so a compound
 * edit is all or nothing. */
static int room(const iw_doc_t *d, uint32_t ins, uint32_t dels)
{
    if (d->npc + 2u * (ins ? 1u : 0u) + dels > d->pc_cap) return 0;
    if (d->add_len + ins > d->add_cap || d->len + ins > d->max_len) return 0;
    return 1;
}

/* ---- the editor: F6 / F8 / Ctrl-F8 ------------------------------------------- */
int iw_ed_attr(iw_ed_t *e, uint8_t n)
{
    iw_doc_t *d = &e->doc;
    const uint8_t pair[6] = { IW_WP5_ATTR_ON, n, IW_WP5_ATTR_ON, IW_WP5_ATTR_OFF, n, IW_WP5_ATTR_OFF };
    int rc, is_off;
    uint32_t q;
    e->goal = NONE;
    if (!(iw_attr_state(d, e->cur) & abit(n))) {
        /* OFF here: the pair goes in, the cursor rests before the off code */
        if (!room(d, 6u, 0u)) return IW_E_FULL;
        rc = iw_doc_insert(d, e->cur, pair, 6u);
        if (rc != IW_OK) return rc;
        e->cur += IW_WP5_ATTR_LEN;
        e->dirty = 1;
        return IW_OK;
    }
    /* ON here: the pair's off code comes to the cursor, the cursor past it */
    q = first_code(d, e->cur, n, &is_off);
    if (q == e->cur && is_off) {             /* it is right here: step past */
        e->cur += IW_WP5_ATTR_LEN;
        return IW_OK;
    }
    if (!is_off) q = NONE;                    /* an unpaired on code: add one */
    if (!room(d, 3u, q != NONE ? 1u : 0u)) return IW_E_FULL;
    rc = iw_doc_insert(d, e->cur, pair + 3, 3u);
    if (rc != IW_OK) return rc;
#ifndef IWORD_MUT_TOGGLE_DUP
    /* MUTANT knob (Rule 6; test-iword-codes-mutant): IWORD_MUT_TOGGLE_DUP
     * leaves the old off code where it was -- a duplicate [bold]. */
    if (q != NONE) {
        rc = iw_doc_delete(d, q + IW_WP5_ATTR_LEN, IW_WP5_ATTR_LEN);
        if (rc != IW_OK) return rc;
    }
#endif
    e->cur += IW_WP5_ATTR_LEN;
    e->dirty = 1;
    return IW_OK;
}

int iw_ed_normal(iw_ed_t *e)
{
    uint32_t st, n;
    int rc;
    e->goal = NONE;
    while (e->cur < e->doc.len && iw_doc_byte(&e->doc, e->cur) == (int)IW_WP5_ATTR_OFF)
        e->cur += IW_WP5_ATTR_LEN;
    st = iw_attr_state(&e->doc, e->cur);
    for (n = 0; n < 32u; n++) {
        if (!(st & abit(n))) continue;
        rc = iw_ed_attr(e, (uint8_t)n);
        if (rc != IW_OK) return rc;
    }
    return IW_OK;
}

/* ---- deleting a code in Reveal Codes ------------------------------------------ */
/* The partner of the attribute code at q, or NONE. */
static uint32_t partner(const iw_doc_t *d, uint32_t q)
{
    int on, is_off;
    int n = iw_code_attr(d, q, &on);
    uint32_t p;
    if (n < 0) return NONE;
    if (on) {
        p = first_code(d, q + IW_WP5_ATTR_LEN, (uint8_t)n, &is_off);
        return (p != NONE && is_off) ? p : NONE;
    } else {
        scan_t s;
        uint32_t at, last = NONE;
        uint8_t c0, c1, lastc = 0;
        scan_at(&s, d, 0);
        while (s.pos < q && scan_next(&s, &at, &c0, &c1)) {
            if ((c0 == IW_WP5_ATTR_ON || c0 == IW_WP5_ATTR_OFF) && c1 == (uint8_t)n) { last = at; lastc = c0; }
        }
        return (last != NONE && lastc == IW_WP5_ATTR_ON) ? last : NONE;
    }
}

/* A [L/R Mar], [Just:] or [Ln Spacing:] code (slice 3; spec/iword/
 * wp5_format.h "margins and justification", "line spacing"): WP asks before
 * deleting one (MINT23, MINT24, MINT26 05_bs_question). */
int iw_code_is_format(const iw_doc_t *d, uint32_t pos)
{
    uint8_t g[4];
    uint32_t size;
    if (iw_doc_read(d, pos, g, sizeof g) != sizeof g || g[0] != IW_WP5_MARGIN_GRP) return 0;
    size = (uint32_t)g[2] | ((uint32_t)g[3] << 8);
    return (g[1] == IW_WP5_MARGIN_SUB && size == 12u) || (g[1] == IW_WP5_JUST_SUB && size == 6u) ||
           (g[1] == IW_WP5_SPACING_SUB && size == 8u);
}

int iw_ed_kill_code(iw_ed_t *e, uint32_t q)
{
    iw_doc_t *d = &e->doc;
    uint32_t p;
    int rc;
    if (iw_code_is_format(d, q)) {
        /* a format code has no partner: it alone goes (MINT24 "Delete
         * [Just:Center]? No (Yes)" y), and the text after it takes the format
         * before it -- every paragraph after it re-wraps */
        uint32_t n = iw_unit_next(d, q) - q;
        if (!room(d, 0u, 1u)) return IW_E_FULL;
        rc = iw_doc_delete(d, q, n);
        if (rc != IW_OK) return rc;
        e->cur = q;
        e->dirty = 1;
        e->goal = NONE;
        return iw_reflow_from(d, q, e->width);
    }
    if (iw_code_attr(d, q, 0) < 0) return IW_E_PROTECTED;
    p = partner(d, q);
#ifdef IWORD_MUT_PAIR_HALF
    /* MUTANT (Rule 6; test-iword-codes-mutant): only the code itself goes --
     * its partner is stranded. NEVER real. */
    p = NONE;
#endif
    if (!room(d, 0u, p != NONE ? 2u : 1u)) return IW_E_FULL;
    if (p != NONE && p > q) {
        rc = iw_doc_delete(d, p, IW_WP5_ATTR_LEN);
        if (rc == IW_OK) rc = iw_doc_delete(d, q, IW_WP5_ATTR_LEN);
        e->cur = q;
    } else {
        rc = iw_doc_delete(d, q, IW_WP5_ATTR_LEN);
        if (rc == IW_OK && p != NONE) rc = iw_doc_delete(d, p, IW_WP5_ATTR_LEN);
        e->cur = p != NONE ? q - IW_WP5_ATTR_LEN : q;
    }
    e->dirty = 1;
    e->goal = NONE;
    return rc;
}

/* ---- Reveal Codes -------------------------------------------------------------- */
static uint32_t put(char *out, uint32_t cap, uint32_t n, const char *s)
{
    while (*s) { if (n + 1u < cap) out[n] = *s; n++; s++; }
    return n;
}

static uint32_t u16at(const uint8_t *g, uint32_t i)
{
    return (uint32_t)g[i] | ((uint32_t)g[i + 1u] << 8);
}

/* The label of the unit at pos (len bytes, first bytes in g). */
static uint32_t label(const iw_doc_t *d, uint32_t pos, char *out, uint32_t cap, uint32_t n)
{
    uint8_t g[16];
    uint32_t got = iw_doc_read(d, pos, g, sizeof g);
    uint8_t c = got ? g[0] : 0;
    if (c >= IW_WP5_TEXT_LO && c <= IW_WP5_TEXT_HI) {
        char t[2];
        t[0] = (char)c;
        t[1] = 0;
        return put(out, cap, n, t);
    }
    switch (c) {
    case IW_WP5_HRT:        return put(out, cap, n, IW_RV_HRT);
    case IW_WP5_SRT:        return put(out, cap, n, IW_RV_SRT);
    case IW_WP5_HPG:        return put(out, cap, n, IW_RV_HPG);
    case IW_WP5_HSPACE:     return put(out, cap, n, IW_RV_HSPACE);
    case IW_WP5_DSRT:       return put(out, cap, n, IW_RV_DSRT);     /* MINT22 */
    case IW_WP5_END_CENTER: return n;                 /* R1/R3: no label */
    case IW_WP5_ATTR_ON:
    case IW_WP5_ATTR_OFF:
        for (uint32_t i = 0; i < IW_RV_ATTR_ROWS; i++) {
            if (ATTR_LABEL[i].n != g[1]) continue;
#ifdef IWORD_MUT_LABEL_ITALC
            /* MUTANT (Rule 6; test-iword-reveal-codes-mutant): italic shown
             * with a made-up 5.0-style label. NEVER real. */
            if (g[1] == IW_WP5_ATTR_ITALIC) return put(out, cap, n, c == IW_WP5_ATTR_ON ? "[ITAL]" : "[ital]");
#endif
#ifdef IWORD_MUT_LABEL_SM_CAP
            /* MUTANT (Rule 6; test-iword-reveal-codes-mutant): Small Caps
             * labelled as the menu spells it ("Sm Cap" -> [SMCAP]), not as
             * the pane does. NEVER real. */
            if (g[1] == 0x0Fu) return put(out, cap, n, c == IW_WP5_ATTR_ON ? "[SMCAP]" : "[smcap]");
#endif
            return put(out, cap, n, c == IW_WP5_ATTR_ON ? ATTR_LABEL[i].on : ATTR_LABEL[i].off);
        }
        return put(out, cap, n, IW_RV_UNKNOWN);
    case 0xC1u:
        if (g[1] == 0xE0u) return put(out, cap, n, IW_RV_CENTER);
        if (g[1] == 0x60u) return put(out, cap, n, IW_RV_FLSH_RGT);
        if (g[1] == 0x02u) return put(out, cap, n, IW_RV_TAB);
        return put(out, cap, n, IW_RV_UNKNOWN);
    case 0xD0u:
        if (g[1] == 0x01u && got >= 16u && u16at(g, 2) == 12u) {
            /* [L/R Mar:<new left>,<new right>]: payload old L, old R, new L,
             * new R in 1200ths of an inch (wp5-body-codes.md Sec 4.1) */
            char num[12];
            n = put(out, cap, n, IW_RV_LR_MAR);
            (void)iw_fmt_inches(u16at(g, 8) / 12u, num);
            n = put(out, cap, n, num);
            n = put(out, cap, n, ",");
            (void)iw_fmt_inches(u16at(g, 10) / 12u, num);
            n = put(out, cap, n, num);
            return put(out, cap, n, "]");
        }
        if (g[1] == 0x06u && got >= 10u && u16at(g, 2) == 6u) {
            /* [Just:<new value>] (MINT04 Left; slice 3: MINT22 05_reveal
             * Left / Center, MINT23 Left, MINT24A 02_reveal Right, MINT24
             * 04_bs_question Center; Full from the menu, MINT22 02) */
            static const char *const J[4] = { IW_RV_JUST_LEFT, IW_RV_JUST_FULL, IW_RV_JUST_CENTER, IW_RV_JUST_RIGHT };
#ifdef IWORD_MUT_LABEL_JUST
            /* MUTANT (Rule 6; test-iword-reveal-codes-mutant): every
             * justification shows as Left. NEVER real. */
            return put(out, cap, n, J[0]);
#endif
            if (g[5] < 4u) return put(out, cap, n, J[g[5]]);
        }
        if (g[1] == IW_WP5_SPACING_SUB && got >= IW_WP5_SPACING_LEN && u16at(g, 2) == 8u) {
            /* [Ln Spacing:<new>] (MINT25 04_reveal "[Ln Spacing:1.5]",
             * "[Ln Spacing:1]"; MINT26 03_reveal "[Ln Spacing:3]",
             * 05_bs_question "[Ln Spacing:1.33]"): the 8.8 value to the
             * hundredth, trailing zeros dropped */
            char num[12];
            uint32_t k = iw_fmt_inches((u16at(g, 6) * 100u + IW_WP5_SPACING_ONE / 2u) / IW_WP5_SPACING_ONE, num);
            num[k - 1u] = 0;                              /* no inch mark */
            n = put(out, cap, n, IW_RV_LN_SPACING);
            n = put(out, cap, n, num);
            return put(out, cap, n, "]");
        }
        return put(out, cap, n, IW_RV_UNKNOWN);
    case 0xD4u:
        if (g[1] == 0x00u) return n;                  /* R3, MINT09: no label */
        if (g[1] == 0x05u) return n;                  /* MINT22 05_reveal: no label */
        return put(out, cap, n, IW_RV_UNKNOWN);
    case IW_WP5_INDENT:     return put(out, cap, n, IW_RV_INDENT);   /* MINT09 */
    case IW_WP5_END_INDENT: return n;                 /* MINT09: no label */
    default: {
        /* P4 (bead initech-8q6j; os/iword/core/page.c, spec/iword/wp5_page.h):
         * [HRt-SPg], [SPg], headers / footers, notes, columns, tables */
        uint32_t r = iw_p4_label(d, pos, out, cap, n);
        if (r != NONE) return r;
        return put(out, cap, n, IW_RV_UNKNOWN);
    }
    }
}

uint32_t iw_code_label(const iw_doc_t *d, uint32_t pos, char *out, uint32_t cap)
{
    uint32_t n = label(d, pos, out, cap, 0);
    if (cap) out[n < cap ? n : cap - 1u] = 0;
    return n < cap ? n : (cap ? cap - 1u : 0u);
}

uint32_t iw_reveal_line(const iw_doc_t *d, const iw_line_t *ln, uint32_t cur, uint32_t blk,
                        char *out, uint32_t cap, uint32_t *cur_off, uint32_t *cur_len)
{
    uint32_t p = ln->start, n = 0;
    *cur_off = NONE;
    *cur_len = 0;
#ifdef IWORD_MUT_BLOCK_LABEL
    /* MUTANT (Rule 6; test-iword-reveal-codes-mutant): the block's anchor is
     * not shown. NEVER real. */
    blk = IW_NO_BLOCK;
#endif
    while (p < ln->end) {
        uint32_t before;
        /* the block's anchor (MINT13 01b_block_reveal "word[Block]s[bold]") */
        if (blk != IW_NO_BLOCK && p == blk) n = put(out, cap, n, IW_RV_BLOCK);
        before = n;
        n = label(d, p, out, cap, n);
        if (p == cur) { *cur_off = before; *cur_len = n - before; }
        p = iw_unit_next(d, p);
    }
    if (blk != IW_NO_BLOCK && blk == d->len && ln->end >= d->len && ln->term == IW_U_TEXT)
        n = put(out, cap, n, IW_RV_BLOCK);
    /* the end of the document: one cell after the last label */
    if (cur == d->len && ln->end >= d->len && ln->term == IW_U_TEXT) {
        *cur_off = n;
        *cur_len = 1;
    }
    if (cap) out[n < cap ? n : cap - 1u] = 0;
    return n < cap ? n : cap - 1u;
}

void iw_reveal_pane(const iw_ed_t *e, uint32_t width, uint32_t nrows,
                    char *rows, uint32_t stride, iw_pane_t *pane)
{
    static char buf[IW_REVEAL_LINE_MAX];
    static iw_line_t lines[32];
    iw_where_t w;
    uint32_t r = 0, got, li;
    for (uint32_t i = 0; i < nrows; i++) rows[i * stride] = 0;
    /* WP fills IW_RV_PANE_ROWS rows; any row below stays blank (MINT16) */
    if (nrows > IW_RV_PANE_ROWS) nrows = IW_RV_PANE_ROWS;
    iw_ed_where(e, &w);
#ifdef IWORD_MUT_PANE_ANCHOR
    /* MUTANT (Rule 6; test-iword-reveal-codes-mutant): two lines above the
     * cursor instead of three. NEVER real. */
    pane->first_line = w.line >= 2u ? w.line - 2u : 0u;
#else
    pane->first_line = w.line >= IW_RV_PANE_ABOVE ? w.line - IW_RV_PANE_ABOVE : 0u;
#endif
    pane->cur_row = pane->cur_col = pane->cur_len = 0;
    got = iw_ed_lines(e, pane->first_line, nrows < 32u ? nrows : 32u, lines);
    for (li = 0; li < got && r < nrows; li++) {
        uint32_t off, clen, n, k;
        n = iw_reveal_line(&e->doc, &lines[li], e->cur, e->blk, buf, sizeof buf, &off, &clen);
        /* the rows this line takes (at least one; the end-of-document cell
         * may open a row of its own) */
        k = 0;
        do {
            char *row = rows + (r + k) * stride;
            uint32_t a = k * width, j;
            for (j = 0; j < width && a + j < n; j++) row[j] = buf[a + j];
            row[j] = 0;
            k++;
        } while (r + k < nrows && k * width < n);
        if (clen && off != NONE) {
            uint32_t cr = r + off / width;
            if (cr < nrows) {
                pane->cur_row = cr;
                pane->cur_col = off % width;
                pane->cur_len = clen > width - pane->cur_col ? width - pane->cur_col : clen;
                if (cr >= r + k) k = cr - r + 1u;
            }
        }
        r += k;
    }
}
