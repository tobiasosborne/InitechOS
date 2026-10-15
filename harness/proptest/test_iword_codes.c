/*
 * harness/proptest/test_iword_codes.c -- THE PROPERTY SUITE of InitechWord's
 * codes as first-class entities (factory C; bead initech-jojk, IWORD P2
 * slice 1; plan docs/plans/INITECH-WORD-plan.md Sec 6(c): "a code's
 * insertion/removal never corrupts a neighboring run or code scope").
 *
 * The core (os/iword/core: piece.c + edit.c + codes.c) is graded against a
 * MODEL written here, never against itself. The model is a token list --
 * a character, a hard return, ON(n), OFF(n) -- edited by the WordPerfect 5.1
 * rules restated below from their sources:
 *   toggle(n)  (F6 / F8 / Ctrl-F8 2 n) -- Reference Manual, Attributes p.
 *              36-37: "When you select an attribute, both an on and off code
 *              are inserted, with the cursor resting just before the off
 *              code" (n off at the cursor); n on: the first code of n at or
 *              after the cursor is the pair's OFF -- it moves to the cursor
 *              and the cursor goes past it (the real WP.EXE, MINT06: F6
 *              after "both" with [und][bold] ahead gave "both[bold] under
 *              [und]"); when it is right at the cursor that is "press that
 *              keystroke once again ... to move beyond the attribute off
 *              code".
 *   normal     (Ctrl-F8 3) -- p. 37 "moves the cursor beyond existing off
 *              codes" then "inserts the necessary off codes": step past the
 *              OFF codes at the cursor, then toggle every attribute still on,
 *              lowest number first (iword.h: the order is authored).
 *   Backspace / Delete on a code -- Reveal Codes ON: the code AND its
 *              partner go (MINT08 03b_reveal_bs1.png: Backspace on [italc]
 *              left "Keptx", [ITALC] gone too); OFF: the cursor steps over
 *              it, nothing is deleted (P1's protection, kept).
 *
 * PROPERTIES, after EVERY key of 600 random sessions (characters, spaces,
 * Enter, Backspace, Delete, arrows, Home/End/Up/Down, F6, F8, Ctrl-F8 2 4,
 * Ctrl-F8 3, Alt-F3), widths 8..67 and the WP 65:
 *   C1 MODEL      the body, soft returns read as spaces, == the model
 *                 serialized (C3 n C3 / C4 n C4 / 0A / text), and the cursor
 *                 sits at the model cursor's byte offset;
 *   C2 WELL-FORMED the body is a sequence of verified units (the codec's own
 *                 validator iw_unit_len) -- no code is ever split;
 *   C3 SCOPES     for every attribute number the codes alternate ON, OFF,
 *                 ON, OFF ... and are balanced (every scope closed, none
 *                 nested in itself, none stranded);
 *   C4 WRAP       the soft returns are exactly where an independent wrapper
 *                 of the visible characters (codes zero width) puts them;
 *   C5 STATE      iw_attr_state at the cursor == the model's state, and
 *                 iw_attr_faces maps bold / italic / underline to faces
 *                 1 / 2 / 4.
 * A failing session is SHRUNK (one-key deletion to a fixpoint) and printed.
 *
 * Mutants (Rule 6; test-iword-codes-mutant, each RED for its named reason):
 *   IWORD_MUT_TOGGLE_DUP  the second F6 leaves the old [bold] behind
 *                         -> RED-REASON codes
 *   IWORD_MUT_PAIR_HALF   Reveal Codes Backspace strands the partner
 *                         -> RED-REASON codes
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

static int g_codes_fail;

static uint32_t g_rng;
static uint32_t rnd(void) { g_rng ^= g_rng << 13; g_rng ^= g_rng >> 17; g_rng ^= g_rng << 5; return g_rng; }
static uint32_t rn(uint32_t n) { return n ? rnd() % n : 0; }

#define CAP  (64u * 1024u)
#define PCS  4096u
static uint8_t    g_add[CAP];
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
#define T_KIND(t) ((t) & 0x300)
#define MAXT     4096
static uint16_t m_t[MAXT];
static uint32_t m_n, m_cur;
static int      m_reveal;

static void m_ins(uint32_t at, uint16_t t)
{
    memmove(m_t + at + 1, m_t + at, (m_n - at) * sizeof *m_t);
    m_t[at] = t;
    m_n++;
}
static void m_del(uint32_t at)
{
    memmove(m_t + at, m_t + at + 1, (m_n - at - 1) * sizeof *m_t);
    m_n--;
}
static int is_code(uint16_t t) { return T_KIND(t) == T_ON || T_KIND(t) == T_OFF; }
static int m_on(uint32_t at, int a)
{
    int on = 0;
    for (uint32_t i = 0; i < at; i++) {
        if (m_t[i] == (T_ON | a)) on = 1;
        if (m_t[i] == (T_OFF | a)) on = 0;
    }
    return on;
}
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

static uint32_t m_bytes(uint8_t *out, uint32_t *cur_off)
{
    uint32_t n = 0;
    for (uint32_t i = 0; i < m_n; i++) {
        uint16_t t = m_t[i];
        if (i == m_cur) *cur_off = n;
        if (t < 0x100) out[n++] = (uint8_t)t;
        else if (t == T_HRT) out[n++] = 0x0A;
        else { uint8_t c = T_KIND(t) == T_ON ? 0xC3 : 0xC4; out[n++] = c; out[n++] = (uint8_t)(t & 0xFF); out[n++] = c; }
    }
    if (m_cur == m_n) *cur_off = n;
    return n;
}

/* C4: the soft returns an independent wrapper puts in the visible text. The
 * rule (iword.h WRAP; MINT01 @0x188 and MINT06 @0x1B5 for the real bytes):
 * per paragraph, from the line start s, if the rest fits in w columns it is
 * the last line; else the LAST space b with b - s <= w ends the line (it
 * becomes the SRt); with none, the first space after s + w. Codes take no
 * column, so the rule runs over the characters alone. */
static void ref_srt(uint32_t w, uint8_t *srt /* per token: 1 = this space is an SRt */)
{
    static uint32_t vis[MAXT];                 /* visible char index -> token */
    uint32_t nv = 0;
    memset(srt, 0, m_n);
    for (uint32_t i = 0; i <= m_n; i++) {
        if (i < m_n && m_t[i] != T_HRT) { if (m_t[i] < 0x100) vis[nv++] = i; continue; }
        /* a paragraph: vis[0..nv) */
        uint32_t s = 0, e = nv;
        while (e - s > w) {
            uint32_t brk = UINT32_MAX;
            for (uint32_t k = s; k <= s + w && k < e; k++) if (m_t[vis[k]] == ' ') brk = k;
            if (brk == UINT32_MAX) {
                uint32_t k = s + w + 1u;
                while (k < e && m_t[vis[k]] != ' ') k++;
                if (k >= e) break;
                brk = k;
            }
            srt[vis[brk]] = 1;
            s = brk + 1u;
        }
        nv = 0;
    }
}

/* ---- the session ------------------------------------------------------------ */
enum { K_CHAR, K_SPACE, K_ENTER, K_BS, K_DEL, K_LEFT, K_RIGHT, K_UP, K_DOWN, K_HOME, K_END,
       K_F6, K_F8, K_ITAL, K_NORMAL, K_REVEAL, K_N };
static const char *KNAME[K_N] = { "char", "space", "enter", "bs", "del", "left", "right", "up", "down",
                                  "home", "end", "F6", "F8", "Ctrl-F8 2 4", "Ctrl-F8 3", "Alt-F3" };
typedef struct { uint8_t k, ch; } key_t3;
#define MAXKEYS 400
static key_t3 g_keys[MAXKEYS];

/* model token index of byte offset b, or UINT32_MAX when b is not a token start */
static uint32_t tok_at(uint32_t b)
{
    uint32_t off = 0;
    for (uint32_t i = 0; i <= m_n; i++) {
        if (off == b) return i;
        if (i == m_n) break;
        off += is_code(m_t[i]) ? 3u : 1u;
    }
    return UINT32_MAX;
}

static int run(const key_t3 *keys, int n, const uint8_t *skip, uint32_t width, int verbose)
{
    static iw_ed_t e;
    static uint8_t srt[MAXT];
    int step = 0;
    iw_doc_init(&e.doc, g_add, CAP, g_pc, PCS, CAP);
    iw_doc_load(&e.doc, g_empty, 0);
    iw_ed_init(&e, width);
    m_n = m_cur = 0;
    m_reveal = 0;
    for (int i = 0; i < n; i++) {
        const key_t3 *k = &keys[i];
        uint32_t len, mlen, mcur = 0;
        if (skip && skip[i]) continue;
        step++;
        if (verbose) printf("    %s%s%c%s\n", KNAME[k->k], k->k == K_CHAR ? " '" : "", k->k == K_CHAR ? k->ch : ' ', k->k == K_CHAR ? "'" : "");
        if (m_n + 4 >= MAXT) break;
        switch (k->k) {
        case K_CHAR: case K_SPACE: {
            uint8_t c = k->k == K_SPACE ? ' ' : k->ch;
            if (iw_ed_type(&e, c) != IW_OK) return step;
            m_ins(m_cur++, c);
            break; }
        case K_ENTER:
            if (iw_ed_enter(&e) != IW_OK) return step;
            m_ins(m_cur++, T_HRT);
            break;
        case K_BS:
            (void)iw_ed_backspace(&e);
            if (m_cur) {
                uint16_t t = m_t[m_cur - 1];
                if (!is_code(t)) { m_del(--m_cur); }
                else if (m_reveal) m_kill(m_cur - 1);
                else m_cur--;
            }
            break;
        case K_DEL:
            (void)iw_ed_delete(&e);
            if (m_cur < m_n) {
                uint16_t t = m_t[m_cur];
                if (!is_code(t)) m_del(m_cur);
                else if (m_reveal) m_kill(m_cur);
                else m_cur++;
            }
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
            if (t == UINT32_MAX) { if (verbose) printf("      the cursor %u is inside a code\n", e.cur); return step; }
            m_cur = t;
            break; }
        case K_F6:     if (iw_ed_attr(&e, A_BOLD) != IW_OK) return step; m_toggle(A_BOLD); break;
        case K_F8:     if (iw_ed_attr(&e, A_UND) != IW_OK) return step;  m_toggle(A_UND);  break;
        case K_ITAL:   if (iw_ed_attr(&e, A_ITAL) != IW_OK) return step; m_toggle(A_ITAL); break;
        case K_NORMAL: if (iw_ed_normal(&e) != IW_OK) return step;       m_normal();       break;
        case K_REVEAL: e.reveal = (uint8_t)!e.reveal; m_reveal = !m_reveal; break;
        }
        /* C1 MODEL */
        mlen = m_bytes(g_want, &mcur);
        len = iw_doc_read(&e.doc, 0, g_buf, CAP);
        if (len != e.doc.len || len != mlen) { if (verbose) printf("      C1 length %u, model %u\n", len, mlen); return step; }
        for (uint32_t q = 0; q < len; q++) {
            uint8_t a = g_buf[q] == 0x0D ? ' ' : g_buf[q];
            if (a != g_want[q]) { if (verbose) printf("      C1 byte %u is 0x%02x, model 0x%02x\n", q, g_buf[q], g_want[q]); return step; }
        }
        if (e.cur != mcur) { if (verbose) printf("      C1 cursor %u, model %u\n", e.cur, mcur); return step; }
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
        /* C4 WRAP */
        ref_srt(width, srt);
        {
            uint32_t b = 0;
            for (uint32_t q = 0; q < m_n; q++) {
                int want_srt = srt[q] != 0, is_srt = g_buf[b] == 0x0D;
                if (want_srt != is_srt) { if (verbose) printf("      C4 token %u: SRt %d, reference %d\n", q, is_srt, want_srt); return step; }
                b += is_code(m_t[q]) ? 3u : 1u;
            }
        }
        /* C5 STATE */
        {
            uint32_t st = iw_attr_state(&e.doc, e.cur), f = iw_attr_faces(st), wf = 0;
            for (int a = 0; a < 32; a++) {
                int on = m_on(m_cur, a);
                if (((st >> a) & 1u) != (uint32_t)on) { if (verbose) printf("      C5 attribute 0x%02x state %u, model %d\n", a, (st >> a) & 1u, on); return step; }
            }
            if (m_on(m_cur, A_BOLD)) wf |= 1u;
            if (m_on(m_cur, A_ITAL)) wf |= 2u;
            if (m_on(m_cur, A_UND))  wf |= 4u;
            if (f != wf) { if (verbose) printf("      C5 faces %u, model %u\n", f, wf); return step; }
        }
    }
    return 0;
}

static void gen(int n)
{
    for (int i = 0; i < n; i++) {
        uint32_t r = rn(100);
        g_keys[i].ch = (uint8_t)('a' + rn(26));
        g_keys[i].k = r < 34 ? K_CHAR : r < 46 ? K_SPACE : r < 49 ? K_ENTER : r < 55 ? K_BS :
                      r < 60 ? K_DEL : r < 66 ? K_LEFT : r < 72 ? K_RIGHT : r < 74 ? K_UP :
                      r < 76 ? K_DOWN : r < 77 ? K_HOME : r < 78 ? K_END : r < 84 ? K_F6 :
                      r < 89 ? K_F8 : r < 94 ? K_ITAL : r < 96 ? K_NORMAL : K_REVEAL;
    }
}

static void sessions(void)
{
    int failed = 0;
    g_rng = 0xC0D35u;
    for (int t = 0; t < 600 && !failed; t++) {
        int n = 1 + (int)rn(MAXKEYS);
        uint32_t width = 8u + rn(60);
        if (t % 3 == 0) width = 65u;
        gen(n);
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
            fprintf(stderr, "  FAIL codes session %d (width %u): shrunk to %d key(s):\n", t, width, live);
            fflush(stderr);
            (void)run(g_keys, n, skip, width, 1);
            fflush(stdout);
            failed = 1;
        }
    }
    CHECK(!failed, "600 sessions: model, well-formed units, balanced scopes, wrap, state");
    if (failed) g_codes_fail++;
    printf("  sessions %s: 600 random sessions (F6/F8/Ctrl-F8/Alt-F3 among the edits) == the model; units whole; scopes balanced; wrap; state\n",
           failed ? "RED" : "green");
}

/* Fixed cases from the real WP.EXE, so the model itself is pinned to it. */
static void fixed(void)
{
    static iw_ed_t e;
    uint32_t len;
    int bad = 0;
    /* MINT06's second line: F6 Bold F8 both F6 " under" F8 -> the body
     * C3 0C C3 "Bold" C3 0E C3 "both" C4 0C C4 " under" C4 0E C4 (@0x16E). */
    static const uint8_t want6[] = { 0xC3,0x0C,0xC3,'B','o','l','d',0xC3,0x0E,0xC3,'b','o','t','h',
                                     0xC4,0x0C,0xC4,' ','u','n','d','e','r',0xC4,0x0E,0xC4 };
    iw_doc_init(&e.doc, g_add, CAP, g_pc, PCS, CAP);
    iw_doc_load(&e.doc, g_empty, 0);
    iw_ed_init(&e, 65);
    (void)iw_ed_attr(&e, A_BOLD);
    for (const char *s = "Bold"; *s; s++) (void)iw_ed_type(&e, (uint8_t)*s);
    (void)iw_ed_attr(&e, A_UND);
    for (const char *s = "both"; *s; s++) (void)iw_ed_type(&e, (uint8_t)*s);
    (void)iw_ed_attr(&e, A_BOLD);
    for (const char *s = " under"; *s; s++) (void)iw_ed_type(&e, (uint8_t)*s);
    (void)iw_ed_attr(&e, A_UND);
    len = iw_doc_read(&e.doc, 0, g_buf, CAP);
    if (len != sizeof want6 || memcmp(g_buf, want6, len) != 0) { fprintf(stderr, "  FAIL fixed: F6/F8 overlap != MINT06 @0x16E\n"); bad = 1; }
    if (e.cur != len) { fprintf(stderr, "  FAIL fixed: the cursor is not past [und]\n"); bad = 1; }
    /* MINT08: "Kept" [ITALC] "x" [italc], Backspace in Reveal Codes -> "Keptx" */
    iw_doc_init(&e.doc, g_add, CAP, g_pc, PCS, CAP);
    iw_doc_load(&e.doc, g_empty, 0);
    iw_ed_init(&e, 65);
    for (const char *s = "Kept"; *s; s++) (void)iw_ed_type(&e, (uint8_t)*s);
    (void)iw_ed_attr(&e, A_ITAL);
    (void)iw_ed_type(&e, 'x');
    (void)iw_ed_attr(&e, A_ITAL);
    e.reveal = 1;
    (void)iw_ed_backspace(&e);
    len = iw_doc_read(&e.doc, 0, g_buf, CAP);
    if (len != 5u || memcmp(g_buf, "Keptx", 5) != 0 || e.cur != 5u) { fprintf(stderr, "  FAIL fixed: Reveal Backspace on [italc] != MINT08 03b \"Keptx\"\n"); bad = 1; }
    /* Without Reveal Codes the code is stepped over, not deleted (P1). */
    e.reveal = 0;
    (void)iw_ed_attr(&e, A_BOLD);              /* "Keptx" [BOLD]|[bold] */
    (void)iw_ed_backspace(&e);                 /* steps before [BOLD]   */
    len = iw_doc_read(&e.doc, 0, g_buf, CAP);
    if (len != 11u || e.cur != 5u) { fprintf(stderr, "  FAIL fixed: Backspace outside Reveal Codes deleted a code\n"); bad = 1; }
    CHECK(!bad, "fixed cases from MINT06 / MINT08");
    if (bad) g_codes_fail++;
    printf("  fixed %s: the F6/F8 overlap of MINT06, the Reveal Codes pair delete of MINT08, P1 protection\n", bad ? "RED" : "green");
}

int main(void)
{
    fixed();
    sessions();
    if (g_codes_fail) fprintf(stderr, "  RED-REASON codes: the code stream disagrees with the WordPerfect rules\n");
    return TEST_SUMMARY("test-iword-codes");
}
