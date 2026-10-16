/*
 * harness/proptest/test_iword_codes.c -- THE PROPERTY SUITE of InitechWord's
 * codes as first-class entities (factory C; bead initech-jojk, IWORD P2
 * slices 1 and 2; plan docs/plans/INITECH-WORD-plan.md Sec 6(c): "a code's
 * insertion/removal never corrupts a neighboring run or code scope").
 *
 * The core (os/iword/core: piece.c + edit.c + codes.c + block.c) is graded
 * against a MODEL written here, never against itself. The model is a token
 * list -- a character, a hard return, ON(n), OFF(n), TAB, INDENT, a hard page
 * -- edited by WordPerfect 5.1's rules, restated below from their sources
 * (../wordperfect51-decomp; Reference Manual, Attributes p. 36-37):
 *   toggle(n)  (F6 / F8 / Ctrl-F8 2 n): n off at the cursor -> "both an on
 *              and off code are inserted, with the cursor resting just
 *              before the off code"; n on -> the first code of n at or after
 *              the cursor is the pair's OFF: it moves to the cursor, the
 *              cursor goes past it (MINT06); right at the cursor it is "press
 *              that keystroke once again ... to move beyond" it.
 *   normal     (Ctrl-F8 3): step past the OFF codes at the cursor, then
 *              toggle every attribute still on, lowest number first.
 *   Backspace / Delete on an attribute code -- Reveal Codes on: the code and
 *              its partner go (MINT08 03b). Off: WordPerfect ASKS "Delete
 *              [code]? No (Yes)" (MINT11 01_bs_prompt, 02_del_prompt); Yes
 *              deletes the pair ("Alpha" F6 BS y -> "Alpha"), No moves the
 *              cursor over the code -- before it for Backspace, past it for
 *              Delete (MINT11: "Gammay[BOLD][bold]", "Beta[BOLD][bold]x").
 *   Tab / F4 / Ctrl-Enter: a [Tab] / [->Indent] / hard page at the cursor;
 *              Backspace / Delete remove them without a question (MINT11
 *              05_bs_tab, 06_bs_hpg; MINT12 13_bs_indent).
 *   Block      (Alt-F4 .. Ctrl-F4 1 1/2/3, Delete y; MINT12..15): the span
 *              [min(anchor, cursor), max); its copy is self-contained (ON
 *              codes of the attributes on at its start, its tokens, OFF codes
 *              of those on at its end: MINT14 "[BOLD]RS[bold]", MINT13
 *              "ep[UND]u[und]"); a cut leaves where it was the codes that
 *              change between its ends, the cursor after them (MINT13
 *              "Dr[BOLD]|old"); a retrieve inserts the copy at the cursor,
 *              the cursor staying in front, then drops every code from there
 *              on that changes nothing (MINT14 "CxyDEF" bold; MINT15
 *              "[BOLD]vwxw[bold]yz").
 * THE BYTES the model serializes to are WordPerfect's, with the geometry an
 * INDEPENDENT reference layout computes here (spec rows restated from the
 * mints, not read from spec/iword/wp5_format.h): a tab / indent moves to the
 * next multiple of 5 columns s and stores 1200 + 120 s, 1200 + 120 s,
 * 10 + s (MINT09 @0x148 / @0x17B); an indent's u16 600 (MINT09 @0x190); C6
 * (9000, 1200 + 120 margin) before the HRt of an indented paragraph (MINT09
 * @0x1E8, @0x20B); the 32-byte D4/00 group before a hard page with 200 x
 * (line on page - 1) and the page number (MINT09 @0x21B, @0x246).
 *
 * PROPERTIES, after EVERY key of three random families (600 / 300 / 400
 * sessions, widths 8..67 and the WP 65; each family names its RED-REASON):
 *   codes     (slice 1's keys + the question): C1..C5 below;
 *   geometry  (tabs, indents, hard pages among text and attributes);
 *   block     (Alt-F4 and the Move menu among the edits):
 *   C1 MODEL      the body == the model serialized (soft returns read as
 *                 spaces; the geometry as above) and the cursor sits at the
 *                 model cursor's offset (a hard return's / hard page's offset
 *                 is that of the C6 / D4 WP writes before it);
 *   C2 WELL-FORMED the body is a sequence of verified units (iw_unit_len);
 *   C3 SCOPES     per attribute the codes alternate ON, OFF, ... balanced;
 *   C4 WRAP       the soft returns are where the reference layout puts them;
 *   C5 STATE      the attribute state at EVERY token boundary of the body
 *                 (iw_attr_step) == the model's replay; iw_attr_state at the
 *                 cursor == the model's; faces bold 1 / italic 2 / under 4;
 *   B1 CUT KEEPS  a cut leaves every surviving character in the attributes
 *                 it had (the model, by character identity).
 * A failing session is SHRUNK (one-key deletion to a fixpoint) and printed.
 *
 * Mutants (Rule 6; test-iword-codes-mutant, each RED for its named reason):
 *   IWORD_MUT_TOGGLE_DUP    the second F6 leaves the old [bold] -> codes
 *   IWORD_MUT_PAIR_HALF     Reveal Backspace strands the partner -> codes
 *   IWORD_MUT_NO_ASK        no "Delete [code]?" question        -> codes
 *   IWORD_MUT_ASK_STAY      "No" leaves the cursor               -> codes
 *   IWORD_MUT_TAB_FIXED     a tab is always 5 columns           -> geometry
 *   IWORD_MUT_COPY_BARE     the copy is not self-contained      -> block
 *   IWORD_MUT_CUT_NO_MEND   a cut leaves no codes behind        -> block
 *   IWORD_MUT_RETRIEVE_DUP  a retrieve keeps redundant codes    -> block
 *   (slice 3, the FORMAT family below: TAB_ABS / NO_LEAD / OLD_STALE /
 *   WRAP_65                                                  -> format)
 *
 * Deterministic (fixed seeds, Rule 11). ASCII-clean (Rule 12).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"
#include "iword.h"

TEST_HARNESS();

static int g_fail_codes, g_fail_geo, g_fail_block;

static uint32_t g_rng;
static uint32_t rnd(void) { g_rng ^= g_rng << 13; g_rng ^= g_rng >> 17; g_rng ^= g_rng << 5; return g_rng; }
static uint32_t rn(uint32_t n) { return n ? rnd() % n : 0; }

#define CAP  (256u * 1024u)
#define PCS  4096u
static uint8_t    g_add[CAP], g_orig[CAP];
static iw_piece_t g_pc[PCS];
static uint8_t    g_empty[1];
static uint8_t    g_buf[CAP], g_want[CAP];

/* The attribute numbers the keys produce, typed from the minted bytes
 * (MINT01 @0x19B `c3 0c c3` bold, @0x1AA `c3 0e c3` underline; MINT06
 * @0x14F `c3 08 c3` italic), not from spec/iword/wp5_format.h. */
#define A_BOLD   0x0C
#define A_UND    0x0E
#define A_ITAL   0x08

/* ---- the model -------------------------------------------------------------- */
#define T_HRT    0x100
#define T_ON     0x200
#define T_OFF    0x300
#define T_TAB    0x400
#define T_IND    0x500
#define T_HPG    0x600
#define T_KIND(t) ((t) & 0xF00)
#define MAXT     4096
static uint16_t m_t[MAXT];
static uint32_t m_id[MAXT], m_next_id;
static uint32_t m_n, m_cur;
static int      m_reveal;
static uint32_t m_blk;                 /* anchor token, or UINT32_MAX     */
static uint32_t m_ask;                 /* the code asked about, or MAX    */
static int      m_ask_del;
static uint16_t m_buf[MAXT];
static uint32_t m_buf_n;
static int      m_pending;             /* "Move cursor; press Enter ..." */

static void m_ins(uint32_t at, uint16_t t)
{
    memmove(m_t + at + 1, m_t + at, (m_n - at) * sizeof *m_t);
    memmove(m_id + at + 1, m_id + at, (m_n - at) * sizeof *m_id);
    m_t[at] = t;
    m_id[at] = ++m_next_id;
    m_n++;
}
static void m_del(uint32_t at)
{
    memmove(m_t + at, m_t + at + 1, (m_n - at - 1) * sizeof *m_t);
    memmove(m_id + at, m_id + at + 1, (m_n - at - 1) * sizeof *m_id);
    m_n--;
}
static int is_code(uint16_t t) { return T_KIND(t) == T_ON || T_KIND(t) == T_OFF; }
static uint32_t m_state(uint32_t at)
{
    uint32_t st = 0;
    for (uint32_t i = 0; i < at; i++) {
        if (T_KIND(m_t[i]) == T_ON) st |= 1u << (m_t[i] & 0x1F);
        if (T_KIND(m_t[i]) == T_OFF) st &= ~(1u << (m_t[i] & 0x1F));
    }
    return st;
}
static int m_on(uint32_t at, int a) { return (int)((m_state(at) >> a) & 1u); }
static void m_toggle(int a)
{
    if (!m_on(m_cur, a)) {
        m_ins(m_cur, (uint16_t)(T_ON | a));
        m_ins(m_cur + 1, (uint16_t)(T_OFF | a));
        m_cur++;
        return;
    }
    for (uint32_t j = m_cur; j < m_n; j++) {
        if (!is_code(m_t[j]) || (m_t[j] & 0xFF) != a) continue;
        if (m_t[j] == (T_OFF | a)) {
            if (j == m_cur) { m_cur++; return; }
            m_del(j);
            m_ins(m_cur, (uint16_t)(T_OFF | a));
            m_cur++;
            return;
        }
        break;
    }
    m_ins(m_cur, (uint16_t)(T_OFF | a));
    m_cur++;
}
static void m_normal(void)
{
    while (m_cur < m_n && T_KIND(m_t[m_cur]) == T_OFF) m_cur++;
    for (int a = 0; a < 32; a++) if (m_on(m_cur, a)) m_toggle(a);
}
static uint32_t m_partner(uint32_t i)
{
    int a = m_t[i] & 0xFF;
    if (T_KIND(m_t[i]) == T_ON) {
        for (uint32_t j = i + 1; j < m_n; j++)
            if (is_code(m_t[j]) && (m_t[j] & 0xFF) == a) return T_KIND(m_t[j]) == T_OFF ? j : UINT32_MAX;
    } else {
        for (uint32_t j = i; j-- > 0; )
            if (is_code(m_t[j]) && (m_t[j] & 0xFF) == a) return T_KIND(m_t[j]) == T_ON ? j : UINT32_MAX;
    }
    return UINT32_MAX;
}
static void m_kill(uint32_t i)
{
    uint32_t p = m_partner(i);
    if (p != UINT32_MAX && p > i) { m_del(p); m_del(i); m_cur = i; }
    else if (p != UINT32_MAX) { m_del(i); m_del(p); m_cur = i - 1; }
    else { m_del(i); m_cur = i; }
}
static void m_span(uint32_t *s, uint32_t *t)
{
    uint32_t a = m_blk == UINT32_MAX ? m_cur : m_blk;
    *s = a < m_cur ? a : m_cur;
    *t = a < m_cur ? m_cur : a;
}
static void m_copy(void)
{
    uint32_t s, t, ss, st;
    m_span(&s, &t);
    ss = m_state(s); st = m_state(t);
    m_buf_n = 0;
    for (int a = 0; a < 16; a++) if ((ss >> a) & 1u) m_buf[m_buf_n++] = (uint16_t)(T_ON | a);
    for (uint32_t i = s; i < t; i++) m_buf[m_buf_n++] = m_t[i];
    for (int a = 0; a < 16; a++) if ((st >> a) & 1u) m_buf[m_buf_n++] = (uint16_t)(T_OFF | a);
    m_blk = UINT32_MAX;
}
static void m_cut(void)
{
    uint32_t s, t, ss, st, diff, k = 0;
    m_span(&s, &t);
    ss = m_state(s); st = m_state(t); diff = ss ^ st;
    for (uint32_t i = s; i < t; i++) m_del(s);
    for (int a = 0; a < 16; a++) if (((diff & st) >> a) & 1u) m_ins(s + k++, (uint16_t)(T_ON | a));
    for (int a = 0; a < 16; a++) if (((diff & ~st) >> a) & 1u) m_ins(s + k++, (uint16_t)(T_OFF | a));
    m_cur = s + k;
    m_blk = UINT32_MAX;
}
static void m_retrieve(void)
{
    uint32_t at = m_cur, st;
    for (uint32_t i = 0; i < m_buf_n; i++) m_ins(at + i, m_buf[i]);
    st = m_state(at);
    for (uint32_t i = at; i < m_n; ) {
        uint16_t t = m_t[i];
        if (is_code(t)) {
            uint32_t bit = 1u << (t & 0x1F);
            int redundant = T_KIND(t) == T_ON ? (st & bit) != 0 : (st & bit) == 0;
            if (redundant) { m_del(i); continue; }
            st = T_KIND(t) == T_ON ? (st | bit) : (st & ~bit);
        }
        i++;
    }
    m_cur = at;
}

/* ---- the reference layout (independent of os/iword/core) --------------------- */
#define EVERY 5u
static uint32_t stop_after(uint32_t c) { return (c / EVERY + 1u) * EVERY; }
static uint32_t adv_t(uint16_t t, uint32_t c)
{
    if (t < 0x100) return c + 1u;
    if (t == T_TAB || t == T_IND) return stop_after(c);
    return c;
}
static uint8_t  r_srt[MAXT];           /* this space token is a soft return     */
static uint32_t r_col[MAXT];           /* the column the token starts at        */
static uint32_t r_page[MAXT], r_pline[MAXT]; /* page / line on page of its line */
static uint32_t r_marg[MAXT];          /* HRT: the paragraph's margin; 0 = none */
static uint8_t  r_ind[MAXT];           /* HRT: its paragraph holds an indent    */

static void ref_layout(uint32_t w)
{
    uint32_t i = 0;
    memset(r_srt, 0, m_n + 1);
    /* 1. the soft returns, paragraph by paragraph */
    while (i <= m_n) {
        uint32_t ps = i, pe = i, m = 0, ls;
        while (pe < m_n && m_t[pe] != T_HRT && m_t[pe] != T_HPG) pe++;
        ls = ps;
        for (;;) {
            uint32_t c = m, mm = m, mfit = m, mb = m, lastfit = UINT32_MAX, brk = UINT32_MAX, t;
            for (t = ls; t < pe; t++) {
                if (m_t[t] == ' ') {
                    if (c <= w) { lastfit = t; mfit = mm; }
                    else { if (lastfit != UINT32_MAX) { brk = lastfit; mb = mfit; } else { brk = t; mb = mm; } break; }
                }
                c = adv_t(m_t[t], c);
                if (m_t[t] == T_IND) mm = c;
            }
            if (brk == UINT32_MAX && c > w && lastfit != UINT32_MAX) { brk = lastfit; mb = mfit; }
            if (brk == UINT32_MAX) break;
            r_srt[brk] = 1;
            ls = brk + 1;
            m = mb;
        }
        i = pe + 1;
    }
    /* 2. the display lines: columns, pages, margins */
    {
        uint32_t c = 0, m = 0, page = 1, pl = 1, first = 1, ind = 0;
        for (i = 0; i < m_n; i++) {
            uint16_t t = m_t[i];
            int term = r_srt[i] || t == T_HRT || t == T_HPG;
            if (!term && t < 0x100 && c + 1u > w && !first) {     /* the layout's own break */
                if (pl >= 54u) { page++; pl = 1; } else pl++;
                c = m;
                first = 1;
            }
            r_col[i] = c;
            r_page[i] = page;
            r_pline[i] = pl;
            if (t == T_HRT) { r_marg[i] = m; r_ind[i] = (uint8_t)ind; }
            if (term) {
                int hpg = t == T_HPG;
                if (t == T_HRT || hpg) { m = 0; ind = 0; }
                if (hpg || pl >= 54u) { page++; pl = 1; } else pl++;
                c = m;
                first = 1;
                continue;
            }
            c = adv_t(t, c);
            if (t == T_IND) { m = c; ind = 1; }
            first = 0;
        }
    }
}

static uint32_t put16(uint8_t *o, uint32_t n, uint32_t v) { o[n] = (uint8_t)v; o[n + 1] = (uint8_t)(v >> 8); return n + 2; }

/* The model as WordPerfect's bytes (the reference's soft returns); off[i] = token
 * i's offset (a HRT / hard page at its C6 / D4); off[m_n] = the length. */
static uint32_t m_off[MAXT + 1];
static uint32_t m_bytes(uint8_t *out)
{
    uint32_t n = 0;
    for (uint32_t i = 0; i < m_n; i++) {
        uint16_t t = m_t[i];
        m_off[i] = n;
        if (t < 0x100) out[n++] = (t == ' ' && r_srt[i]) ? 0x0D : (uint8_t)t;
        else if (t == T_HRT) {
            if (r_ind[i]) { out[n++] = 0xC6; n = put16(out, n, 9000); n = put16(out, n, 1200 + 120 * r_marg[i]); out[n++] = 0xC6; }
            out[n++] = 0x0A;
        } else if (t == T_TAB || t == T_IND) {
            uint32_t s = stop_after(r_col[i]);
            out[n++] = t == T_TAB ? 0xC1 : 0xC2;
            if (t == T_TAB) out[n++] = 0x02;
            else { out[n++] = 0x00; n = put16(out, n, 600); }
            n = put16(out, n, 1200 + 120 * s);
            n = put16(out, n, 1200 + 120 * s);
            n = put16(out, n, 10 + s);
            out[n++] = t == T_TAB ? 0xC1 : 0xC2;
        } else if (t == T_HPG) {
            static const uint8_t mid[18] = { 0,0,0,0,0,0,0,0, '0','*','0','*','0','*', 0xB0,0x04,0xB0,0x04 };
            out[n++] = 0xD4; out[n++] = 0x00; out[n++] = 0x1C; out[n++] = 0x00;
            out[n++] = 0x00; out[n++] = 0x16;
            n = put16(out, n, (r_pline[i] - 1u) * 200u);
            n = put16(out, n, r_page[i]);
            memcpy(out + n, mid, sizeof mid); n += sizeof mid;
            out[n++] = 0x1C; out[n++] = 0x00; out[n++] = 0x00; out[n++] = 0xD4;
            out[n++] = 0x0C;
        } else {
            uint8_t c = T_KIND(t) == T_ON ? 0xC3 : 0xC4;
            out[n++] = c; out[n++] = (uint8_t)(t & 0xFF); out[n++] = c;
        }
    }
    m_off[m_n] = n;
    return n;
}
static uint32_t tok_at(uint32_t b)
{
    for (uint32_t i = 0; i <= m_n; i++) if (m_off[i] == b) return i;
    return UINT32_MAX;
}

/* ---- the session ------------------------------------------------------------ */
enum { K_CHAR, K_SPACE, K_ENTER, K_BS, K_DEL, K_LEFT, K_RIGHT, K_UP, K_DOWN, K_HOME, K_END,
       K_F6, K_F8, K_ITAL, K_NORMAL, K_REVEAL, K_TAB, K_INDENT, K_HPG, K_BLOCK, K_MOVE, K_COPY,
       K_BDEL, K_YES, K_NO, K_N };
static const char *KNAME[K_N] = { "char", "space", "enter", "bs", "del", "left", "right", "up", "down",
                                  "home", "end", "F6", "F8", "Ctrl-F8 2 4", "Ctrl-F8 3", "Alt-F3",
                                  "Tab", "F4", "Ctrl-Enter", "Alt-F4", "Ctrl-F4 1 1", "Ctrl-F4 1 2",
                                  "Delete (block) y", "y", "n" };
typedef struct { uint8_t k, ch; } key_t3;
#define MAXKEYS 400
static key_t3 g_keys[MAXKEYS];

/* Compact as Face A does (one original, an empty add buffer): positions stay. */
static void compact(iw_ed_t *e)
{
    uint32_t n = iw_doc_read(&e->doc, 0, g_orig, CAP);
    uint32_t cur = e->cur, blk = e->blk, ask = e->ask, goal = e->goal;
    uint8_t rv = e->reveal, ad = e->ask_del;
    iw_doc_init(&e->doc, g_add, CAP, g_pc, PCS, CAP);
    iw_doc_load(&e->doc, g_orig, n);
    e->cur = cur; e->blk = blk; e->ask = ask; e->goal = goal; e->reveal = rv; e->ask_del = ad;
}

/* The family a failure is charged to. */
enum { FAM_CODES, FAM_GEO, FAM_BLOCK };
static const char *FAMNAME[3] = { "codes", "geometry", "block" };

static int run(const key_t3 *keys, int n, const uint8_t *skip, uint32_t width, int verbose)
{
    static iw_ed_t e;
    static uint8_t cbuf[CAP];
#define IDMAX (1u << 20)
    static uint32_t idst[IDMAX];        /* B1: state of each character id before a cut */
    uint32_t cbuf_n = 0;
    int step = 0;
    iw_doc_init(&e.doc, g_add, CAP, g_pc, PCS, CAP);
    iw_doc_load(&e.doc, g_empty, 0);
    iw_ed_init(&e, width);
    m_n = m_cur = 0;
    m_next_id = 0;
    m_reveal = 0;
    m_blk = m_ask = UINT32_MAX;
    m_buf_n = 0;
    m_pending = 0;
    for (int i = 0; i < n; i++) {
        const key_t3 *k = &keys[i];
        uint32_t len, mlen, mcur;
        int cut = 0, rc;
        if (skip && skip[i]) continue;
        if (m_n + 64 >= MAXT) break;
        /* the face's own rules decide which keys reach the core (face_a.c):
         * a pending question takes only its answer; while Block is on only
         * the cursor keys, Alt-F4, the Move menu and Delete; while a move
         * waits for Enter, the cursor keys and Enter */
        if (m_ask != UINT32_MAX && k->k != K_YES && k->k != K_NO) continue;
        if (m_ask == UINT32_MAX && (k->k == K_YES || k->k == K_NO)) continue;
        if (m_blk != UINT32_MAX && !(k->k >= K_LEFT && k->k <= K_END) && k->k != K_BLOCK &&
            k->k != K_MOVE && k->k != K_COPY && k->k != K_BDEL) continue;
        if (m_blk == UINT32_MAX && (k->k == K_MOVE || k->k == K_COPY || k->k == K_BDEL)) continue;
        if (m_pending && !(k->k >= K_LEFT && k->k <= K_END) && k->k != K_ENTER) continue;
        step++;
        if (verbose) printf("    %s%s%c%s\n", KNAME[k->k], k->k == K_CHAR ? " '" : "", k->k == K_CHAR ? k->ch : ' ', k->k == K_CHAR ? "'" : "");
        /* B1: the state of every character before a cut */
        if (k->k == K_MOVE || k->k == K_BDEL) {
            uint32_t st = 0;
            cut = 1;
            for (uint32_t q = 0; q < m_n; q++) {
                uint16_t t = m_t[q];
                if (T_KIND(t) == T_ON) st |= 1u << (t & 0x1F);
                else if (T_KIND(t) == T_OFF) st &= ~(1u << (t & 0x1F));
                if (m_id[q] < IDMAX) idst[m_id[q]] = st;
            }
        }
        switch (k->k) {
        case K_CHAR: case K_SPACE: {
            uint8_t c = k->k == K_SPACE ? ' ' : k->ch;
            if (iw_ed_type(&e, c) != IW_OK) return step;
            m_ins(m_cur++, c);
            break; }
        case K_ENTER:
            if (m_pending) {
                if (iw_ed_retrieve(&e, cbuf, cbuf_n) != IW_OK) return step;
                m_retrieve();
                m_pending = 0;
                break;
            }
            if (iw_ed_enter(&e) != IW_OK) return step;
            m_ins(m_cur++, T_HRT);
            break;
        case K_TAB:    if (iw_ed_tab(&e) != IW_OK) return step;    m_ins(m_cur++, T_TAB); break;
        case K_INDENT: if (iw_ed_indent(&e) != IW_OK) return step; m_ins(m_cur++, T_IND); break;
        case K_HPG:    if (iw_ed_hpage(&e) != IW_OK) return step;  m_ins(m_cur++, T_HPG); break;
        case K_BS:
            rc = iw_ed_backspace(&e);
            if (m_cur) {
                uint16_t t = m_t[m_cur - 1];
                if (!is_code(t)) { m_del(--m_cur); }
                else if (m_reveal) m_kill(m_cur - 1);
                else { m_ask = m_cur - 1; m_ask_del = 0; }
            }
            if (m_ask != UINT32_MAX && rc != IW_E_ASK) { if (verbose) printf("      no question asked (rc %d)\n", rc); return step; }
            break;
        case K_DEL:
            rc = iw_ed_delete(&e);
            if (m_cur < m_n) {
                uint16_t t = m_t[m_cur];
                if (!is_code(t)) m_del(m_cur);
                else if (m_reveal) m_kill(m_cur);
                else { m_ask = m_cur; m_ask_del = 1; }
            }
            if (m_ask != UINT32_MAX && rc != IW_E_ASK) { if (verbose) printf("      no question asked (rc %d)\n", rc); return step; }
            break;
        case K_YES: case K_NO:
            if (iw_ed_answer(&e, k->k == K_YES) != IW_OK) return step;
            if (k->k == K_YES) m_kill(m_ask);
            else m_cur = m_ask_del ? m_ask + 1 : m_ask;
            m_ask = UINT32_MAX;
            break;
        case K_LEFT:  iw_ed_left(&e);  if (m_cur) m_cur--; break;
        case K_RIGHT: iw_ed_right(&e); if (m_cur < m_n) m_cur++; break;
        case K_UP: case K_DOWN: case K_HOME: case K_END: {
            uint32_t t;
            if (k->k == K_UP) iw_ed_up(&e);
            else if (k->k == K_DOWN) iw_ed_down(&e);
            else if (k->k == K_HOME) iw_ed_line_home(&e);
            else iw_ed_line_end(&e);
            t = tok_at(e.cur);
            if (t == UINT32_MAX) { if (verbose) printf("      the cursor %u is inside a code or between WP's bookkeeping and its HRt / HPg\n", e.cur); return step; }
            m_cur = t;
            break; }
        case K_F6:     if (iw_ed_attr(&e, A_BOLD) != IW_OK) return step; m_toggle(A_BOLD); break;
        case K_F8:     if (iw_ed_attr(&e, A_UND) != IW_OK) return step;  m_toggle(A_UND);  break;
        case K_ITAL:   if (iw_ed_attr(&e, A_ITAL) != IW_OK) return step; m_toggle(A_ITAL); break;
        case K_NORMAL: if (iw_ed_normal(&e) != IW_OK) return step;       m_normal();       break;
        case K_REVEAL: e.reveal = (uint8_t)!e.reveal; m_reveal = !m_reveal; break;
        case K_BLOCK:
            iw_ed_block(&e);
            m_blk = m_blk == UINT32_MAX ? m_cur : UINT32_MAX;
            if ((e.blk == IW_NO_BLOCK) != (m_blk == UINT32_MAX)) return step;
            break;
        case K_COPY:
            if (iw_ed_block_copy(&e, cbuf, CAP, &cbuf_n) != IW_OK) return step;
            m_copy();
            m_pending = 1;
            break;
        case K_MOVE:
            if (iw_ed_block_cut(&e, cbuf, CAP, &cbuf_n) != IW_OK) return step;
            {
                uint32_t blk = m_blk;
                m_copy();
                m_blk = blk;
                m_cut();
            }
            m_pending = 1;
            break;
        case K_BDEL:
            if (iw_ed_block_cut(&e, 0, 0, 0) != IW_OK) return step;
            m_cut();
            break;
        }
        if (iw_ed_fixup(&e) != IW_OK) { if (verbose) printf("      fixup refused\n"); return step; }
        if (e.doc.npc > PCS / 2u || e.doc.add_len > CAP / 2u) compact(&e);
        /* C4 WRAP first (a misplaced soft return is named as such), then C1 */
        ref_layout(width);
        mlen = m_bytes(g_want);
        mcur = m_off[m_cur];
        len = iw_doc_read(&e.doc, 0, g_buf, CAP);
        if (len == mlen) {
            for (uint32_t q = 0; q < m_n; q++) {
                int is_srt = g_buf[m_off[q]] == 0x0D;
                if (m_t[q] == ' ' && (r_srt[q] != 0) != is_srt) { if (verbose) printf("      C4 token %u: SRt %d, reference %d\n", q, is_srt, r_srt[q]); return step; }
            }
        }
        /* C1 MODEL: every byte, the geometry and the soft returns included */
        if (len != e.doc.len || len != mlen) { if (verbose) printf("      C1 length %u, model %u\n", len, mlen); return step; }
        for (uint32_t q = 0; q < len; q++)
            if (g_buf[q] != g_want[q]) { if (verbose) printf("      C1 byte %u is 0x%02x, model 0x%02x\n", q, g_buf[q], g_want[q]); return step; }
        if (e.cur != mcur) { if (verbose) printf("      C1 cursor %u, model %u\n", e.cur, mcur); return step; }
        if (m_blk != UINT32_MAX && e.blk != m_off[m_blk]) { if (verbose) printf("      C1 block anchor %u, model %u\n", e.blk, m_off[m_blk]); return step; }
        /* C2 WELL-FORMED */
        for (uint32_t q = 0; q < len; ) {
            uint32_t u = iw_unit_len(g_buf, len, q);
            if (!u) { if (verbose) printf("      C2 malformed at %u\n", q); return step; }
            q += u;
        }
        /* C3 SCOPES: alternate ON / OFF, balanced, per attribute number */
        for (int a = 0; a < 16; a++) {
            int on = 0;
            for (uint32_t q = 0; q < m_n; q++) {
                if (m_t[q] == (T_ON | a)) { if (on) { if (verbose) printf("      C3 attribute 0x%02x on twice\n", a); return step; } on = 1; }
                if (m_t[q] == (T_OFF | a)) { if (!on) { if (verbose) printf("      C3 attribute 0x%02x off with no on\n", a); return step; } on = 0; }
            }
            if (on) { if (verbose) printf("      C3 attribute 0x%02x left open\n", a); return step; }
        }
        /* C5 STATE at every token boundary, and at the cursor */
        {
            uint32_t st = 0, mst = 0, p = 0, q = 0, f, wf = 0;
            while (q <= m_n) {
                while (p < m_off[q]) { st = iw_attr_step(&e.doc, p, st); p = iw_unit_next(&e.doc, p); }
                if (st != mst) { if (verbose) printf("      C5 state at token %u is 0x%x, model 0x%x\n", q, st, mst); return step; }
                if (q == m_n) break;
                if (T_KIND(m_t[q]) == T_ON) mst |= 1u << (m_t[q] & 0x1F);
                else if (T_KIND(m_t[q]) == T_OFF) mst &= ~(1u << (m_t[q] & 0x1F));
                q++;
            }
            st = iw_attr_state(&e.doc, e.cur);
            if (st != m_state(m_cur)) { if (verbose) printf("      C5 state at the cursor 0x%x, model 0x%x\n", st, m_state(m_cur)); return step; }
            f = iw_attr_faces(st);
            if (m_on(m_cur, A_BOLD)) wf |= 1u;
            if (m_on(m_cur, A_ITAL)) wf |= 2u;
            if (m_on(m_cur, A_UND))  wf |= 4u;
            if (f != wf) { if (verbose) printf("      C5 faces %u, model %u\n", f, wf); return step; }
        }
        /* B1 a cut keeps every surviving character's attributes */
        if (cut) {
            uint32_t st = 0;
            for (uint32_t q = 0; q < m_n; q++) {
                uint16_t t = m_t[q];
                if (T_KIND(t) == T_ON) st |= 1u << (t & 0x1F);
                else if (T_KIND(t) == T_OFF) st &= ~(1u << (t & 0x1F));
                else if (t < 0x100 && m_id[q] < IDMAX && idst[m_id[q]] != st) {
                    if (verbose) printf("      B1 character %u ('%c') was 0x%x, is 0x%x after the cut\n", q, t, idst[m_id[q]], st);
                    return step;
                }
            }
        }
    }
    return 0;
}

static void gen(int n, int fam)
{
    for (int i = 0; i < n; i++) {
        uint32_t r = rn(100);
        key_t3 *k = &g_keys[i];
        k->ch = (uint8_t)('a' + rn(26));
        if (fam == FAM_CODES) {
            k->k = r < 32 ? K_CHAR : r < 44 ? K_SPACE : r < 47 ? K_ENTER : r < 54 ? K_BS :
                   r < 60 ? K_DEL : r < 65 ? K_LEFT : r < 70 ? K_RIGHT : r < 72 ? K_UP :
                   r < 74 ? K_DOWN : r < 75 ? K_HOME : r < 76 ? K_END : r < 82 ? K_F6 :
                   r < 86 ? K_F8 : r < 90 ? K_ITAL : r < 92 ? K_NORMAL : r < 94 ? K_REVEAL :
                   r < 97 ? K_YES : K_NO;
        } else if (fam == FAM_GEO) {
            k->k = r < 34 ? K_CHAR : r < 46 ? K_SPACE : r < 50 ? K_ENTER : r < 56 ? K_BS :
                   r < 60 ? K_DEL : r < 65 ? K_LEFT : r < 70 ? K_RIGHT : r < 73 ? K_UP :
                   r < 76 ? K_DOWN : r < 77 ? K_HOME : r < 79 ? K_END : r < 85 ? K_TAB :
                   r < 89 ? K_INDENT : r < 92 ? K_HPG : r < 94 ? K_F6 : r < 96 ? K_F8 :
                   r < 98 ? K_YES : K_NO;
        } else {
            k->k = r < 28 ? K_CHAR : r < 38 ? K_SPACE : r < 41 ? K_ENTER : r < 45 ? K_BS :
                   r < 48 ? K_DEL : r < 56 ? K_LEFT : r < 64 ? K_RIGHT : r < 66 ? K_UP :
                   r < 68 ? K_DOWN : r < 69 ? K_HOME : r < 71 ? K_END : r < 76 ? K_F6 :
                   r < 79 ? K_F8 : r < 81 ? K_ITAL : r < 86 ? K_BLOCK : r < 89 ? K_MOVE :
                   r < 92 ? K_COPY : r < 94 ? K_BDEL : r < 95 ? K_TAB : r < 96 ? K_HPG :
                   r < 98 ? K_YES : K_NO;
        }
    }
}

static void family(int fam, int sessions, uint32_t seed)
{
    int failed = 0;
    g_rng = seed;
    for (int t = 0; t < sessions && !failed; t++) {
        int n = 1 + (int)rn(MAXKEYS);
        uint32_t width = 8u + rn(60);
        if (t % 3 == 0) width = 66u;      /* WP's (slice 3, MINT22; was 65) */
        gen(n, fam);
        if (run(g_keys, n, 0, width, 0)) {
            static uint8_t skip[MAXKEYS];
            int live = n, changed = 1;
            memset(skip, 0, sizeof skip);
            while (changed) {
                changed = 0;
                for (int i = 0; i < n; i++) {
                    if (skip[i]) continue;
                    skip[i] = 1;
                    if (run(g_keys, n, skip, width, 0)) { changed = 1; live--; } else skip[i] = 0;
                }
            }
            fprintf(stderr, "  FAIL %s session %d (width %u): shrunk to %d key(s):\n", FAMNAME[fam], t, width, live);
            fflush(stderr);
            (void)run(g_keys, n, skip, width, 1);
            fflush(stdout);
            failed = 1;
        }
    }
    CHECK(!failed, "random sessions: model, well-formed units, balanced scopes, wrap, state, cuts");
    if (failed) { if (fam == FAM_CODES) g_fail_codes++; else if (fam == FAM_GEO) g_fail_geo++; else g_fail_block++; }
    printf("  %-8s %s: %d random sessions == the model; units whole; scopes balanced; wrap; state%s\n",
           FAMNAME[fam], failed ? "RED" : "green", sessions, fam == FAM_BLOCK ? "; cuts keep attributes" : "");
}

/* Fixed cases from the real WP.EXE, so the model itself is pinned to it. */
static void type_s(iw_ed_t *e, const char *s) { for (; *s; s++) (void)iw_ed_type(e, (uint8_t)*s); }
static void fresh(iw_ed_t *e)
{
    iw_doc_init(&e->doc, g_add, CAP, g_pc, PCS, CAP);
    iw_doc_load(&e->doc, g_empty, 0);
    iw_ed_init(e, 65);
}
static void fixed(void)
{
    static iw_ed_t e;
    static uint8_t cb[256];
    uint32_t len, cn = 0;
    int bad = 0, badb = 0;
    /* MINT06's second line: F6 Bold F8 both F6 " under" F8 -> the body
     * C3 0C C3 "Bold" C3 0E C3 "both" C4 0C C4 " under" C4 0E C4 (@0x16E). */
    static const uint8_t want6[] = { 0xC3,0x0C,0xC3,'B','o','l','d',0xC3,0x0E,0xC3,'b','o','t','h',
                                     0xC4,0x0C,0xC4,' ','u','n','d','e','r',0xC4,0x0E,0xC4 };
    fresh(&e);
    (void)iw_ed_attr(&e, A_BOLD);
    type_s(&e, "Bold");
    (void)iw_ed_attr(&e, A_UND);
    type_s(&e, "both");
    (void)iw_ed_attr(&e, A_BOLD);
    type_s(&e, " under");
    (void)iw_ed_attr(&e, A_UND);
    len = iw_doc_read(&e.doc, 0, g_buf, CAP);
    if (len != sizeof want6 || memcmp(g_buf, want6, len) != 0) { fprintf(stderr, "  FAIL fixed: F6/F8 overlap != MINT06 @0x16E\n"); bad = 1; }
    if (e.cur != len) { fprintf(stderr, "  FAIL fixed: the cursor is not past [und]\n"); bad = 1; }
    /* MINT08: "Kept" [ITALC] "x" [italc], Backspace in Reveal Codes -> "Keptx" */
    fresh(&e);
    type_s(&e, "Kept");
    (void)iw_ed_attr(&e, A_ITAL);
    (void)iw_ed_type(&e, 'x');
    (void)iw_ed_attr(&e, A_ITAL);
    e.reveal = 1;
    (void)iw_ed_backspace(&e);
    len = iw_doc_read(&e.doc, 0, g_buf, CAP);
    if (len != 5u || memcmp(g_buf, "Keptx", 5) != 0 || e.cur != 5u) { fprintf(stderr, "  FAIL fixed: Reveal Backspace on [italc] != MINT08 03b \"Keptx\"\n"); bad = 1; }
    /* MINT11 (re/mint-results-003.md): on the editing screen the code is
     * ASKED about. "Gamma" F6 BS n "y" -> "Gammay[BOLD][bold]" */
    e.reveal = 0;
    fresh(&e);
    type_s(&e, "Gamma");
    (void)iw_ed_attr(&e, A_BOLD);
    if (iw_ed_backspace(&e) != IW_E_ASK || e.doc.len != 11u) { fprintf(stderr, "  FAIL fixed: Backspace after [BOLD] did not ask\n"); bad = 1; }
    (void)iw_ed_answer(&e, 0);
    (void)iw_ed_type(&e, 'y');
    len = iw_doc_read(&e.doc, 0, g_buf, CAP);
    if (len != 12u || memcmp(g_buf, "Gammay\xC3\x0C\xC3\xC4\x0C\xC4", 12) != 0) { fprintf(stderr, "  FAIL fixed: BS n != MINT11 \"Gammay[BOLD][bold]\"\n"); bad = 1; }
    /* "Beta" F6 Del n "x" -> "Beta[BOLD][bold]x" */
    fresh(&e);
    type_s(&e, "Beta");
    (void)iw_ed_attr(&e, A_BOLD);
    if (iw_ed_delete(&e) != IW_E_ASK) { fprintf(stderr, "  FAIL fixed: Delete before [bold] did not ask\n"); bad = 1; }
    (void)iw_ed_answer(&e, 0);
    (void)iw_ed_type(&e, 'x');
    len = iw_doc_read(&e.doc, 0, g_buf, CAP);
    if (len != 11u || memcmp(g_buf, "Beta\xC3\x0C\xC3\xC4\x0C\xC4x", 11) != 0) { fprintf(stderr, "  FAIL fixed: Del n != MINT11 \"Beta[BOLD][bold]x\"\n"); bad = 1; }
    /* "Alpha" F6 BS y " one" -> "Alpha one" ; "Delta" F6 Del y "z" -> "Deltaz" */
    fresh(&e);
    type_s(&e, "Alpha");
    (void)iw_ed_attr(&e, A_BOLD);
    (void)iw_ed_backspace(&e);
    (void)iw_ed_answer(&e, 1);
    type_s(&e, " one");
    len = iw_doc_read(&e.doc, 0, g_buf, CAP);
    if (len != 9u || memcmp(g_buf, "Alpha one", 9) != 0) { fprintf(stderr, "  FAIL fixed: BS y != MINT11 \"Alpha one\"\n"); bad = 1; }
    fresh(&e);
    type_s(&e, "Delta");
    (void)iw_ed_attr(&e, A_BOLD);
    (void)iw_ed_delete(&e);
    (void)iw_ed_answer(&e, 1);
    (void)iw_ed_type(&e, 'z');
    len = iw_doc_read(&e.doc, 0, g_buf, CAP);
    if (len != 6u || memcmp(g_buf, "Deltaz", 6) != 0) { fprintf(stderr, "  FAIL fixed: Del y != MINT11 \"Deltaz\"\n"); bad = 1; }
    CHECK(!bad, "fixed cases from MINT06 / MINT08 / MINT11");
    if (bad) g_fail_codes++;
    printf("  fixed %s: the F6/F8 overlap of MINT06, the Reveal Codes pair delete of MINT08, the four MINT11 answers\n", bad ? "RED" : "green");
    /* MINT15: "[BOLD]vwxyz[bold]", Home (after [BOLD]) Right, Alt-F4, Right,
     * Ctrl-F4 1 2, Right, Enter -> "[BOLD]vwxw[bold]yz", the cursor on the
     * retrieved "w" (02_bold_into_bold_mid_reveal Pos 1.3"). */
    fresh(&e);
    (void)iw_ed_attr(&e, A_BOLD);
    type_s(&e, "vwxyz");
    (void)iw_ed_attr(&e, A_BOLD);
    iw_ed_line_home(&e);
    iw_ed_right(&e);
    iw_ed_block(&e);
    iw_ed_right(&e);
    if (iw_ed_block_copy(&e, cb, sizeof cb, &cn) != IW_OK || cn != 7u || memcmp(cb, "\xC3\x0C\xC3w\xC4\x0C\xC4", 7) != 0) { fprintf(stderr, "  FAIL fixed: the copy of \"w\" in bold is not [BOLD]w[bold] (MINT14 \"[BOLD]RS[bold]\")\n"); badb = 1; }
    iw_ed_right(&e);
    (void)iw_ed_retrieve(&e, cb, cn);
    len = iw_doc_read(&e.doc, 0, g_buf, CAP);
    if (len != 12u || memcmp(g_buf, "\xC3\x0C\xC3vwxw\xC4\x0C\xC4yz", 12) != 0 || e.cur != 6u) { fprintf(stderr, "  FAIL fixed: MINT15 retrieve != \"[BOLD]vwxw[bold]yz\" with the cursor on w\n"); badb = 1; }
    /* MINT13 line 3: "Drop[BOLD]bold here[bold] end.", Home Right x2, Alt-F4,
     * Right x4 (o, p, [BOLD], b), Delete y -> "Dr[BOLD]old here[bold] end.",
     * the cursor after [BOLD] (03c_deleted_reveal) */
    fresh(&e);
    type_s(&e, "Drop");
    (void)iw_ed_attr(&e, A_BOLD);
    type_s(&e, "bold here");
    (void)iw_ed_attr(&e, A_BOLD);
    type_s(&e, " end.");
    iw_ed_line_home(&e);
    iw_ed_right(&e); iw_ed_right(&e);
    iw_ed_block(&e);
    iw_ed_right(&e); iw_ed_right(&e); iw_ed_right(&e); iw_ed_right(&e);
    (void)iw_ed_block_cut(&e, 0, 0, 0);
    len = iw_doc_read(&e.doc, 0, g_buf, CAP);
    if (len != 21u || memcmp(g_buf, "Dr\xC3\x0C\xC3old here\xC4\x0C\xC4 end.", 21) != 0 || e.cur != 5u) { fprintf(stderr, "  FAIL fixed: MINT13 block delete != \"Dr[BOLD]|old here[bold] end.\"\n"); badb = 1; }
    CHECK(!badb, "fixed block cases from MINT13 / MINT14 / MINT15");
    if (badb) g_fail_block++;
    printf("  fixed %s: the self-contained copy (MINT14), the retrieve into bold (MINT15), the block delete (MINT13)\n", badb ? "RED" : "green");
}

/* ======================================================================== *
 * FORMAT (bead initech-jojk, P2 slice 3): margins and justification.
 * The editor is driven by random keys (typing, Enter, Tab, F4, Backspace /
 * Delete with the question answered, the cursor keys, Shift-F8 1 7 L R,
 * Shift-F8 1 3 n) and after EVERY key the body is read back by an
 * INDEPENDENT REFERENCE written here from the slice 3 rows restated from
 * the mints (../wordperfect51-decomp re/mint-results-004.md; not from
 * spec/iword/wp5_format.h):
 *   F1 WELL-FORMED  verified units only; the cursor on a unit boundary and
 *                   never right before a D4/05 (the line's text follows it);
 *   F2 OLD VALUES   every [L/R Mar] (D0/01) holds the margins in effect
 *                   before it as its first pair (MINT21 @0x259, MINT23
 *                   @0x19B), every [Just:] (D0/06) the justification before
 *                   it (MINT24: rewritten after a deletion);
 *   F3 WRAP         a line holds (10200 - R - L) / 120 + 1 characters once a
 *                   margin code is in effect (MINT21: 56; MINT22: 66), the
 *                   free width before: every soft-returned line fits and the
 *                   next word would not have;
 *   F4 D4/05        one at the start of each line whose justification is
 *                   Center / Right, one after each [Just:] whose old or new
 *                   value is, no other (MINT22, MINT24A); x = Center L +
 *                   (10200 - R - L - w) / 2, Right 10200 - R - w, else L; t =
 *                   x / 120 (MINT22 28/3420, MINT24A 11/1320);
 *   F5 TABS         a tab / indent stores the next stop after the cursor's
 *                   column counted from the left margin, every 5 columns:
 *                   a = L + 120 s, t = a / 120 (MINT21 @0x1CF, MINT23 @0x1AE);
 *                   C6 = 10200 - R, L + 120 m (MINT21 @0x24E).
 *   F6 SPACING      every [Ln Spacing] (D0/02) holds the spacing before it as
 *                   its old value (MINT25 @0x1AE / @0x1D4; MINT26: rewritten
 *                   after a deletion), 1/256ths of a line, 1 at the top.
 * The sessions also press the slice-3 keys that only move the cursor or the
 * text -- Line Spacing, Home x3 Left, Home Home (Home) Up, Home Home Down,
 * and Move without a block (Ctrl-F4 1/2/3 then Move + retrieve, Copy +
 * retrieve or Delete; re/mint-results-004.md MINT27, MINT31) -- and every
 * rule above must hold after each.
 * Mutants: TAB_ABS, NO_LEAD, OLD_STALE, WRAP_65 -> RED-REASON format.
 * ======================================================================== */
enum { F_CHAR, F_SPACE, F_ENTER, F_BS, F_DEL, F_LEFT, F_RIGHT, F_UP, F_DOWN, F_HOME, F_END,
       F_TAB, F_INDENT, F_MAR, F_JUST, F_SPACING, F_HHHL, F_TOP, F_BOT, F_MOVE, F_NKEYS };
static const char *const FNAME[F_NKEYS] = { "char", "space", "Enter", "Backspace", "Delete", "Left", "Right",
    "Up", "Down", "Home", "End", "Tab", "F4", "Shift-F8 1 7", "Shift-F8 1 3", "Shift-F8 1 6",
    "Home Home Home Left", "Home Home (Home) Up", "Home Home Down", "Ctrl-F4 n m" };
static const uint32_t FM_SP[] = { 100, 133, 150, 200, 225, 300 };   /* hundredths (MINT25/26/30) */
static uint8_t g_clipf[4096];
static uint32_t g_clipf_n;
typedef struct { uint8_t k, a, b; } fkey_t;
static fkey_t g_fkeys[MAXKEYS];
static int g_fail_format;
static const uint32_t FM_L[] = { 1200, 1320, 1500, 1560, 1800, 2400 };
static const uint32_t FM_R[] = { 1200, 1380, 2040, 2400, 3000 };

static uint32_t rd16(const uint8_t *b, uint32_t i) { return (uint32_t)b[i] | ((uint32_t)b[i + 1] << 8); }

/* The reference's unit length (verified rows only). */
static uint32_t r_len(const uint8_t *b, uint32_t n, uint32_t p)
{
    uint8_t c = b[p];
    static const uint8_t fixed[16] = { 4, 9, 11, 3, 3, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    if (c >= 0x20 && c <= 0x7E) return 1;
    if (c == 0x0A || c == 0x0D || c == 0x0C || c == 0x90 || c == 0xA0 || c == 0x83) return 1;
    if (c >= 0xC0 && c <= 0xCF) {
        uint32_t L = fixed[c - 0xC0];
        return (L && p + L <= n && b[p + L - 1] == c) ? L : 0;
    }
    if (c >= 0xD0 && p + 8 <= n) {
        uint32_t L = 4 + rd16(b, p + 2);
        if (p + L <= n && b[p + L - 1] == c && b[p + L - 2] == b[p + 1]) return L;
    }
    return 0;
}

#define R_TEXT(c) (((c) >= 0x20 && (c) <= 0x7E) || (c) == 0xA0)
static int r_lead(const uint8_t *b, uint32_t p) { return b[p] == 0xD4 && b[p + 1] == 0x05; }
static int r_mar(const uint8_t *b, uint32_t p) { return b[p] == 0xD0 && b[p + 1] == 0x01; }
static int r_jst(const uint8_t *b, uint32_t p) { return b[p] == 0xD0 && b[p + 1] == 0x06; }
static int r_spc(const uint8_t *b, uint32_t p) { return b[p] == 0xD0 && b[p + 1] == 0x02; }
static int r_tab(const uint8_t *b, uint32_t p) { return (b[p] == 0xC1 && b[p + 1] == 0x02) || b[p] == 0xC2; }
static int r_term(uint8_t c) { return c == 0x0A || c == 0x0D || c == 0x0C || c == 0x90; }
static int r_cr(uint32_t j) { return j == 2 || j == 3; }

/* The reference check of body b[0..n) with the cursor at cur; 0 = good. */
static int r_check(const uint8_t *b, uint32_t n, uint32_t cur, uint32_t width0, int verbose)
{
    uint32_t p = 0, L = 1200, R = 1200, J = 1, set = 0, m = 0, S = 256;
    int cur_ok = cur == n, tail = 0;   /* tail: the line continues a layout break */
    /* F1: units */
    for (p = 0; p < n; ) {
        uint32_t u = r_len(b, n, p);
        if (!u) { if (verbose) printf("      F1 no verified unit at %u (0x%02x)\n", p, b[p]); return 1; }
        if (p == cur) {
            cur_ok = 1;
            if (r_lead(b, p)) { if (verbose) printf("      F1 the cursor rests before a D4/05 at %u\n", p); return 1; }
        }
        p += u;
    }
    if (!cur_ok) { if (verbose) printf("      F1 the cursor %u is not on a unit boundary\n", cur); return 1; }
    /* lines */
    p = 0;
    for (;;) {
        uint32_t s = p, c = m, lw, lL, lR, need, run = J, cols, q, after_m = m, mm = m;
        uint32_t L0 = L, R0 = R, set0 = set;
        int layout_break = 0, placed = 0, prev_jst = 0;
        /* the line's leading format codes format it */
        lL = L; lR = R;
        q = s;
        while (q < n && !r_term(b[q])) {
            uint32_t u = r_len(b, n, q);
            if (R_TEXT(b[q]) || r_tab(b, q)) break;
            if (r_mar(b, q)) { lL = rd16(b, q + 8); lR = rd16(b, q + 10); set0 = 1; }
            q += u;
        }
        lw = set0 ? (10200u - lR - lL) / 120u + 1u : width0;
        (void)L0; (void)R0;
        /* walk the line */
        need = r_cr(J);
        q = s;
        while (q < n && !r_term(b[q])) {
            uint32_t u = r_len(b, n, q);
            /* the layout's own break: a character past the line, once a
             * unit other than a D4/05 is on it */
            if ((R_TEXT(b[q])) && c + 1u > lw && placed) { layout_break = 1; break; }
            if (r_lead(b, q) && placed && !prev_jst) {
                /* D4/05 records right before such a break start the next line
                 * -- except the one right after a [Just:], which is that
                 * code's own (stated 2026-10-16; no mint breaks there) */
                uint32_t z = q;
                while (z < n && r_lead(b, z)) z += r_len(b, n, z);
                if (z < n && R_TEXT(b[z]) && c + 1u > lw) { layout_break = 1; break; }
            }
            if (!r_lead(b, q)) placed = 1;
            prev_jst = r_jst(b, q);
            if (need) {
                if (!r_lead(b, q)) { if (verbose) printf("      F4 a D4/05 is missing at %u\n", q); return 1; }
                need = 0;
            } else if (r_lead(b, q)) { if (verbose) printf("      F4 a stray D4/05 at %u\n", q); return 1; }
            if (r_lead(b, q)) {
                /* its x: the text up to the next one or the line's end */
                uint32_t w = 0, z = q + u, cc = 0, x, room = (10200u - lR) > lL ? 10200u - lR - lL : 0u;
                while (z < n && !r_term(b[z]) && !r_lead(b, z)) {
                    if (R_TEXT(b[z])) cc++;
                    else if (r_tab(b, z)) cc = (cc / 5u + 1u) * 5u;
                    z += r_len(b, n, z);
                }
                w = cc * 120u;
                x = lL;
                if (run == 2 && w <= room) x = lL + (room - w) / 2u;
                else if (run == 3 && w <= room) x = 10200u - lR - w;
                if (rd16(b, q + 4) != x / 120u || rd16(b, q + 6) != x) {
                    if (verbose) printf("      F4 the D4/05 at %u holds %u/%u, the reference %u/%u\n", q, rd16(b, q + 4), rd16(b, q + 6), x / 120u, x);
                    return 1;
                }
            }
            if (r_mar(b, q)) {
                if (rd16(b, q + 4) != L || rd16(b, q + 6) != R) {
                    if (verbose) printf("      F2 the [L/R Mar] at %u holds old %u,%u, the margins before it are %u,%u\n", q, rd16(b, q + 4), rd16(b, q + 6), L, R);
                    return 1;
                }
                L = rd16(b, q + 8); R = rd16(b, q + 10); set = 1;
            }
            if (r_spc(b, q)) {
                if (rd16(b, q + 4) != S) {
                    if (verbose) printf("      F6 the [Ln Spacing] at %u holds old %u, the spacing before it is %u\n", q, rd16(b, q + 4), S);
                    return 1;
                }
                S = rd16(b, q + 6);
            }
            if (r_jst(b, q)) {
                if (b[q + 4] != J) { if (verbose) printf("      F2 the [Just:] at %u holds old %u, the justification before it is %u\n", q, b[q + 4], J); return 1; }
                need = r_cr(J) || r_cr(b[q + 5]);
                J = b[q + 5];
                run = J;
            }
            if (r_tab(b, q)) {
                uint32_t st = (c / 5u + 1u) * 5u, a = lL + 120u * st;
                uint32_t off = b[q] == 0xC2 ? 4 : 2;
                if (rd16(b, q + off) != a || rd16(b, q + off + 2) != a || rd16(b, q + off + 4) != a / 120u) {
                    if (verbose) printf("      F5 the tab / indent at %u holds %u, the reference stop %u (column %u from L %u)\n", q, rd16(b, q + off), a, st, lL);
                    return 1;
                }
                c = st;
                if (b[q] == 0xC2) mm = c;
            } else if (b[q] == 0xC6) {
                if (rd16(b, q + 1) != 10200u - lR || rd16(b, q + 3) != lL + 120u * mm) {
                    if (verbose) printf("      F5 C6 at %u holds %u,%u, the reference %u,%u\n", q, rd16(b, q + 1), rd16(b, q + 3), 10200u - lR, lL + 120u * mm);
                    return 1;
                }
            } else if (R_TEXT(b[q])) c++;
            q += u;
        }
        if (need) { if (verbose) printf("      F4 a D4/05 is missing at the end of the line at %u\n", q); return 1; }
        cols = c;
        after_m = mm;
        /* (the tail of a word longer than the line ends at its SRt: the P1
         * rule, not graded for fit) */
        if (q < n && b[q] == 0x0D && !layout_break && !tail) {
            /* F3: it fits, and the next word would not have */
            uint32_t z = q + 1, wd = 0;
            /* (a single word longer than the line keeps its own line:
             * the P1 rule, edit.c WRAP) */
            int one_word = 1;
            for (uint32_t z2 = s; z2 < q; z2 += r_len(b, n, z2)) if (b[z2] == ' ') one_word = 0;
            if (cols > lw && !one_word) { if (verbose) printf("      F3 the line at %u is %u columns, it holds %u\n", s, cols, lw); return 1; }
            while (z < n && !r_term(b[z]) && b[z] != ' ') { if (R_TEXT(b[z])) wd++; else if (r_tab(b, z)) wd = 1000; z += r_len(b, n, z); }
            /* (a run of spaces at the wrap and a word after a tab are not
             * minted: not graded) */
            if (wd > 0 && wd < 1000 && cols + 1u + wd <= lw) {
                if (verbose) printf("      F3 the line at %u (%u columns, holds %u) could take the next %u-column word\n", s, cols, lw, wd);
                return 1;
            }
        }
        if (q >= n) return 0;
        tail = layout_break;
        if (layout_break) { p = q; m = after_m; continue; }
        m = b[q] == 0x0D ? after_m : 0u;
        if (b[q] != 0x0D) mm = 0;
        p = q + 1;
    }
}

static int frun(const fkey_t *keys, int n, const uint8_t *skip, uint32_t width, int verbose)
{
    static iw_ed_t e;
    iw_doc_init(&e.doc, g_add, CAP, g_pc, PCS, CAP);
    iw_doc_load(&e.doc, g_empty, 0);
    iw_ed_init(&e, width);
    for (int i = 0; i < n; i++) {
        const fkey_t *k = &keys[i];
        int rc = IW_OK, edit = 1;
        uint32_t len;
        if (skip && skip[i]) continue;
        if (verbose) printf("    %s %u %u\n", FNAME[k->k], k->a, k->b);
        switch (k->k) {
        case F_CHAR:  rc = iw_ed_type(&e, (uint8_t)('a' + k->a % 26u)); break;
        case F_SPACE: rc = iw_ed_type(&e, ' '); break;
        case F_ENTER: rc = iw_ed_enter(&e); break;
        case F_BS: case F_DEL:
            rc = k->k == F_BS ? iw_ed_backspace(&e) : iw_ed_delete(&e);
            if (rc == IW_E_ASK) rc = iw_ed_answer(&e, k->a & 1u);
            if (rc == IW_E_PROTECTED) rc = IW_OK;
            break;
        case F_LEFT:  iw_ed_left(&e); edit = 0; break;
        case F_RIGHT: iw_ed_right(&e); edit = 0; break;
        case F_UP:    iw_ed_up(&e); edit = 0; break;
        case F_DOWN:  iw_ed_down(&e); edit = 0; break;
        case F_HOME:  iw_ed_line_home(&e); edit = 0; break;
        case F_END:   iw_ed_line_end(&e); edit = 0; break;
        case F_TAB:   rc = iw_ed_tab(&e); break;
        case F_INDENT: rc = iw_ed_indent(&e); break;
        case F_MAR:   rc = iw_ed_margins(&e, FM_L[k->a % 6u], FM_R[k->b % 5u]); break;
        case F_SPACING: rc = iw_ed_spacing(&e, iw_spacing_from_hundredths(FM_SP[k->a % 6u])); break;
        case F_HHHL:  iw_ed_line_start(&e); edit = 0; break;
        case F_TOP:   iw_ed_doc_top(&e, k->a & 1u); edit = 0; break;
        case F_BOT:   iw_ed_doc_end(&e); edit = 0; break;
        case F_MOVE:
            /* Ctrl-F4 1/2/3 (Sentence, Paragraph, Page), then 1 Move +
             * Enter, 2 Copy + Enter, or 3 Delete; nothing to select is a
             * refusal WP shows as nothing happening */
            if (iw_ed_select(&e, 1 + (int)(k->a % 3u)) != IW_OK) { edit = 0; break; }
            if (k->b % 3u == 0u) {
                rc = iw_ed_block_cut(&e, g_clipf, sizeof g_clipf, &g_clipf_n);
                if (rc == IW_OK) rc = iw_ed_retrieve(&e, g_clipf, g_clipf_n);
            } else if (k->b % 3u == 1u) {
                rc = iw_ed_block_copy(&e, g_clipf, sizeof g_clipf, &g_clipf_n);
                if (rc == IW_OK) rc = iw_ed_retrieve(&e, g_clipf, g_clipf_n);
            } else {
                rc = iw_ed_block_cut(&e, 0, 0, 0);
            }
            if (rc == IW_E_FULL) { rc = IW_OK; e.blk = IW_NO_BLOCK; }
            break;
        default:      rc = iw_ed_just(&e, k->a % 4u); break;
        }
        if (rc != IW_OK) { if (verbose) printf("      key refused rc=%d\n", rc); return i + 1; }
        if (edit && iw_ed_fixup(&e) != IW_OK) { if (verbose) printf("      fixup refused\n"); return i + 1; }
        len = iw_doc_read(&e.doc, 0, g_buf, CAP);
        if (r_check(g_buf, len, e.cur, width, verbose)) return i + 1;
    }
    return 0;
}

static void format_family(int sessions, uint32_t seed)
{
    int failed = 0;
    g_rng = seed;
    for (int t = 0; t < sessions && !failed; t++) {
        int n = 1 + (int)rn(MAXKEYS);
        uint32_t width = (t % 2 == 0) ? 66u : 20u + rn(47);
        for (int i = 0; i < n; i++) {
            uint32_t r = rn(100);
            fkey_t *k = &g_fkeys[i];
            k->a = (uint8_t)rn(256);
            k->b = (uint8_t)rn(256);
            k->k = (uint8_t)(r < 38 ? F_CHAR : r < 52 ? F_SPACE : r < 57 ? F_ENTER : r < 62 ? F_BS :
                   r < 66 ? F_DEL : r < 70 ? F_LEFT : r < 74 ? F_RIGHT : r < 76 ? F_UP : r < 78 ? F_DOWN :
                   r < 79 ? F_HOME : r < 80 ? F_END : r < 82 ? F_TAB : r < 84 ? F_INDENT :
                   r < 88 ? F_MAR : r < 91 ? F_JUST : r < 93 ? F_SPACING : r < 94 ? F_HHHL :
                   r < 95 ? F_TOP : r < 96 ? F_BOT : F_MOVE);
        }
        if (frun(g_fkeys, n, 0, width, 0)) {
            static uint8_t skip[MAXKEYS];
            int live = n, changed = 1;
            memset(skip, 0, sizeof skip);
            while (changed) {
                changed = 0;
                for (int i = 0; i < n; i++) {
                    if (skip[i]) continue;
                    skip[i] = 1;
                    if (frun(g_fkeys, n, skip, width, 0)) { changed = 1; live--; } else skip[i] = 0;
                }
            }
            fprintf(stderr, "  FAIL format session %d (width %u): shrunk to %d key(s):\n", t, width, live);
            fflush(stderr);
            (void)frun(g_fkeys, n, skip, width, 1);
            fflush(stdout);
            failed = 1;
        }
    }
    CHECK(!failed, "format sessions: units, old values, wrap, D4/05, tabs under the margins, spacing, Home keys, Move without a block");
    if (failed) g_fail_format++;
    printf("  format   %s: %d random sessions == the reference; units whole; old values; wrap under the margins; D4/05; tabs / C6; spacing old values; Home keys; Move without a block\n",
           failed ? "RED" : "green", sessions);
}

int main(void)
{
    fixed();
    family(FAM_CODES, 600, 0xC0D35u);
    family(FAM_GEO, 300, 0x6E0u);
    family(FAM_BLOCK, 400, 0xB10Cu);
#ifndef FORMAT_SESSIONS
#define FORMAT_SESSIONS 400
#endif
    format_family(FORMAT_SESSIONS, 0xF0A7u);
    if (g_fail_codes) fprintf(stderr, "  RED-REASON codes: the code stream disagrees with the WordPerfect rules\n");
    if (g_fail_geo)   fprintf(stderr, "  RED-REASON geometry: tabs / indents / hard pages disagree with WordPerfect's layout and bytes\n");
    if (g_fail_block) fprintf(stderr, "  RED-REASON block: a block copy / cut / retrieve disagrees with WordPerfect's rules\n");
    if (g_fail_format) fprintf(stderr, "  RED-REASON format: margins / justification disagree with WordPerfect's bytes and layout\n");
    return TEST_SUMMARY("test-iword-codes");
}
