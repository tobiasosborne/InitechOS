/*
 * harness/proptest/test_te_surface.c -- the PROPERTY SUITE of FLAIR TextEdit's
 * editing surface (factory C; bead initech-tdnl.16, R4.2 InitechText).
 *
 * WHAT IS UNDER TEST. os/flair/textedit.c as a SimpleText-alike drives it:
 * typing, Backspace, the four arrows, click / shift-click / drag selection
 * (FlairTEClick + FlairTESetSelect(anchor, FlairTEGetOffset)), caret placement
 * (FlairTEGetPoint), the replace-range edits behind Cut / Clear / Paste, and
 * the non-panicking capacity probe FlairTECanReplace -- in Geneva 9 (txFont 1,
 * the InitechText face), word wrap on.
 *
 * THE ORACLE IS A REFERENCE IMPLEMENTATION (Law 2 -- never by construction):
 * a separate, deliberately naive model written from the RULES in textedit.h
 * "THE EDITING SURFACE" and ../system7-decomp/specs/toolbox/textedit.md Sec 2 /
 * Sec 4 (half-open selection; CR ends a line, the next starts past it; greedy
 * word wrap in pixels, a space never starts a wrap and hangs, a too-long word
 * hard-breaks before the char that does not fit). It shares NO code with
 * textedit.c: its own text buffer, its own layout, a BRUTE-FORCE hit test (try
 * every boundary of the line, keep the nearest; a tie goes right, the
 * midpoint rule), and Geneva 9 advances restated by hand as literals for the
 * suite's alphabet (spec/assets/geneva9.h rows 0x20 ' ' 4, 0x2E '.' 4, 0x57 'W'
 * 9, 0x61 'a' 6, 0x65 'e' 6, 0x69 'i' 4, 0x6C 'l' 3, 0x6D 'm' 8, 0x77 'w' 8)
 * so a drift in the font path cannot hide in both sides.
 *
 * PROPERTIES (each over thousands of random cases, deterministic LCG seeds):
 *   P1 EDIT   random op sequences (type / Backspace / arrows / click / shift-
 *             click / drag / replace / select-all): text, selStart, selEnd
 *             and the line starts equal the reference after every op.
 *   P2 HIT    FlairTEGetOffset(h, v) == the brute-force nearest boundary for
 *             random points over and around the text.
 *   P3 POINT  FlairTEGetOffset(FlairTEGetPoint(o)) == o for every boundary o
 *             (the caret is drawn where a click puts it), and GetPoint's h
 *             equals the reference prefix width.
 *   P4 PROBE  FlairTECanReplace agrees with the reference's "the result has
 *             <= 4096 chars and <= 128 lines" for random replacements, the
 *             line cap exercised by CR-heavy texts; a probe that says yes is
 *             followed by the real edit, which must not panic.
 * Hand literals (the anchor the random suite hangs off): "aim" widths 0 6 10
 * 18; "a\rw" offset 1 is before the CR, line 1 starts at 2.
 *
 * Mutants (Rule 6; -D on textedit.c, test-te-surface-mutant), each RED for
 * its named reason (the suite prints "RED-REASON <tag>" on the first miss):
 *   TE_MUT_HIT_FLOOR          the char under the point, not the nearest
 *                             boundary                          -> hit
 *   TE_MUT_HIT_PAST_CR        the caret reaches past a line's CR -> cr
 *   TE_MUT_ARROW_NO_COLLAPSE  left arrow over a range does not collapse
 *                                                               -> arrow
 *   TE_MUT_EXTEND_ANCHOR      shift-click always anchors selStart -> extend
 *   TE_MUT_PROBE_NO_LINES     the probe ignores the line cap     -> probe
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "textedit.h"
#include "text.h"

/* ---- counters + the first-miss reason ---------------------------------- */
static long g_checks, g_fails;
static int  g_reason_printed[8];
enum { R_EDIT, R_HIT, R_CR, R_ARROW, R_EXTEND, R_PROBE, R_POINT, R_LIT };
static const char *const g_rname[8] = { "edit", "hit", "cr", "arrow", "extend",
                                         "probe", "point", "literal" };
#define CHECK(c, r, ...) do { g_checks++; if (!(c)) { g_fails++;            \
    if (!g_reason_printed[r]) { g_reason_printed[r] = 1;                    \
        printf("RED-REASON %s: ", g_rname[r]); printf(__VA_ARGS__);         \
        printf(" (line %d)\n", __LINE__); } } } while (0)

/* ---- the deterministic generator ---------------------------------------- */
static uint32_t g_seed;
static uint32_t rnd(void) { g_seed = g_seed * 1103515245u + 12345u; return g_seed >> 8; }
static int rndn(int n) { return n <= 0 ? 0 : (int)(rnd() % (uint32_t)n); }

/* ---- THE REFERENCE MODEL ------------------------------------------------ */
#define RMAX   FLAIR_TE_TEXT_MAX
#define RLINES FLAIR_TE_MAX_LINES
#define LH     11                  /* Geneva 9 cell height, GENEVA9_CELL_H */

static int adv(char c)
{
    switch (c) {
    case ' ': return 4; case '.': return 4; case 'W': return 9;
    case 'a': return 6; case 'e': return 6; case 'i': return 4;
    case 'l': return 3; case 'm': return 8; case 'w': return 8;
    default:  return -1000;      /* outside the suite alphabet: poison */
    }
}

typedef struct ref {
    char t[RMAX + 64];
    int  n, s, e, width;
    int  starts[RMAX + 2], nl;     /* uncapped: the reference can over-run */
} ref_t;

/* Greedy word wrap, written from textedit.md Sec 4 by itself. */
static void ref_layout(ref_t *r)
{
    int ls = 0, i = 0;
    r->nl = 0;
    r->starts[r->nl++] = 0;
    while (i < r->n) {
        if (r->t[i] == '\r') { r->starts[r->nl++] = i + 1; ls = i + 1; i++; continue; }
        if (r->t[i] != ' ') {
            int w = 0;
            for (int k = ls; k <= i; k++) w += adv(r->t[k]);
            /* spaces hang: drop the trailing run of spaces before i from w?
             * No -- spaces before i are INSIDE the line; only a space itself
             * never triggers. The width of [ls, i] includes them. */
            if (w > r->width && i > ls) {
                int brk = -1;
                for (int k = i - 1; k >= ls; k--)
                    if (r->t[k] == ' ') { brk = k + 1; break; }
                if (brk <= ls) brk = i;           /* no space: hard break */
                r->starts[r->nl++] = brk;
                ls = brk;
                continue;                          /* re-check char i */
            }
        }
        i++;
    }
}

static int ref_line_of(const ref_t *r, int off)
{
    int L = 0;
    while (L + 1 < r->nl && r->starts[L + 1] <= off) L++;
    return L;
}

static int ref_reach(const ref_t *r, int L)
{
    if (L + 1 >= r->nl) return r->n;
    int e = r->starts[L + 1];
    if (e > 0 && r->t[e - 1] == '\r') e--;
    return e;
}

static int ref_x(const ref_t *r, int off)
{
    int L = ref_line_of(r, off), x = 0;
    for (int i = r->starts[L]; i < off; i++) x += adv(r->t[i]);
    return x;
}

/* Brute force: every boundary of the point's line, the nearest one; on an
 * exact tie the RIGHT one (a click exactly at a char's midpoint). */
static int ref_hit(const ref_t *r, int h, int v)
{
    int L = v < 0 ? 0 : v / LH, best = -1, bd = 0;
    if (L >= r->nl) L = r->nl - 1;
    for (int o = r->starts[L]; o <= ref_reach(r, L); o++) {
        int x = 0;
        for (int i = r->starts[L]; i < o; i++) x += adv(r->t[i]);
        int d = h - x; if (d < 0) d = -d;
        if (best < 0 || d <= bd) { best = o; bd = d; }
    }
    return best;
}

static void ref_set(ref_t *r, int a, int b)
{
    if (a < 0) a = 0;
    if (a > r->n) a = r->n;
    if (b < 0) b = 0;
    if (b > r->n) b = r->n;
    if (a > b) { int t = a; a = b; b = t; }
    r->s = a; r->e = b;
}

static void ref_replace(ref_t *r, const char *s, int len)
{
    memmove(r->t + r->s + len, r->t + r->e, (size_t)(r->n - r->e));
    memcpy(r->t + r->s, s, (size_t)len);
    r->n = r->n - (r->e - r->s) + len;
    r->s = r->e = r->s + len;
    ref_layout(r);
}

static void ref_arrow(ref_t *r, int ch)
{
    int o;
    if (ch == FLAIR_TE_CH_LEFT)       o = (r->s < r->e) ? r->s : r->s - 1;
    else if (ch == FLAIR_TE_CH_RIGHT) o = (r->s < r->e) ? r->e : r->e + 1;
    else {
        int from = (ch == FLAIR_TE_CH_UP) ? r->s : r->e;
        int L = ref_line_of(r, from), x = ref_x(r, from);
        if (ch == FLAIR_TE_CH_UP) o = (L == 0) ? 0 : ref_hit(r, x, (L - 1) * LH);
        else o = (L + 1 >= r->nl) ? r->n : ref_hit(r, x, (L + 1) * LH);
    }
    ref_set(r, o, o);
}

/* shift-click: keep the end FARTHER from the point (ties keep selStart). */
static int ref_extend(ref_t *r, int off)
{
    int da = off - r->s, db = r->e - off;
    if (da < 0) da = -da;
    if (db < 0) db = -db;
    int anchor = (da >= db) ? r->s : r->e;
    ref_set(r, anchor, off);
    return anchor;
}

/* ---- comparing the artifact against the reference ----------------------- */
static int same_state(const FlairTE *te, const ref_t *r)
{
    if (te->teLength != r->n || te->selStart != r->s || te->selEnd != r->e) return 0;
    if (memcmp(te->text, r->t, (size_t)r->n) != 0) return 0;
    if (te->nLines != r->nl) return 0;
    for (int i = 0; i < r->nl; i++) if (te->lineStarts[i] != r->starts[i]) return 0;
    return 1;
}

static void fresh(FlairTE *te, ref_t *r, int width)
{
    FlairTE_init(te, (int16_t)width, FLAIR_TE_JUST_LEFT);
    te->txFont = 1;                                /* applFont: Geneva 9 */
    FlairTESetText(te, "", 0);
    memset(r, 0, sizeof *r);
    r->width = width;
    ref_layout(r);
}

static const char ALPHA[] = "aaeeiilmmww  W..\r";
static char rchar(void) { return ALPHA[rndn((int)sizeof ALPHA - 1)]; }

/* ---- P1 EDIT ------------------------------------------------------------ */
static FlairTE g_te;
static ref_t   g_ref;

static void p1_edit(void)
{
    for (int seq = 0; seq < 1500; seq++) {
        int width = 30 + rndn(220), anchor = 0;
        g_seed = 0x1000u + (uint32_t)seq;
        fresh(&g_te, &g_ref, width);
        for (int op = 0; op < 60; op++) {
            int k = rndn(100);
            int rule = R_EDIT;
            if (k < 45) {                                  /* type a char */
                char c = rchar();
                if (!FlairTECanReplace(&g_te, g_te.selStart, g_te.selEnd, &c, 1)) continue;
                FlairTEKey(&g_te, c);
                ref_replace(&g_ref, &c, 1);
            } else if (k < 55) {                           /* Backspace */
                FlairTEKey(&g_te, 0x08);
                if (g_ref.s < g_ref.e) ref_replace(&g_ref, "", 0);
                else if (g_ref.s > 0) { g_ref.s--; ref_replace(&g_ref, "", 0); }
            } else if (k < 70) {                           /* an arrow */
                char a = (char)(FLAIR_TE_CH_LEFT + rndn(4));
                FlairTEKey(&g_te, a);
                ref_arrow(&g_ref, a);
                rule = R_ARROW;
            } else if (k < 80) {                           /* click (+drag) */
                int h = rndn(width + 20) - 10, v = rndn((g_ref.nl + 1) * LH) - 5;
                anchor = FlairTEClick(&g_te, (int16_t)h, (int16_t)v, 0);
                int ra = ref_hit(&g_ref, h, v);
                ref_set(&g_ref, ra, ra);
                CHECK(anchor == ra, R_HIT, "click anchor %d != ref %d", anchor, ra);
                rule = R_HIT;
                if (rndn(2)) {                             /* drag samples */
                    for (int d = 0; d < 3; d++) {
                        int h2 = rndn(width + 20) - 10, v2 = rndn((g_ref.nl + 1) * LH) - 5;
                        FlairTESetSelect(&g_te, (int16_t)anchor,
                                         FlairTEGetOffset(&g_te, (int16_t)h2, (int16_t)v2));
                        ref_set(&g_ref, ra, ref_hit(&g_ref, h2, v2));
                    }
                }
            } else if (k < 88) {                           /* shift-click */
                int h = rndn(width + 20) - 10, v = rndn((g_ref.nl + 1) * LH) - 5;
                anchor = FlairTEClick(&g_te, (int16_t)h, (int16_t)v, 1);
                int ranch = ref_extend(&g_ref, ref_hit(&g_ref, h, v));
                CHECK(anchor == ranch, R_EXTEND, "extend anchor %d != ref %d", anchor, ranch);
                rule = R_EXTEND;
            } else if (k < 95) {                           /* replace (paste) */
                char run[24];
                int n = rndn(20);
                for (int i = 0; i < n; i++) run[i] = rchar();
                if (!FlairTECanReplace(&g_te, g_te.selStart, g_te.selEnd, run, (int16_t)n)) continue;
                FlairTEInsert(&g_te, run, (int16_t)n);
                ref_replace(&g_ref, run, n);
            } else if (k < 98) {                           /* Clear */
                FlairTEDelete(&g_te);
                if (g_ref.s < g_ref.e) ref_replace(&g_ref, "", 0);
            } else {                                       /* Select All */
                FlairTESetSelect(&g_te, 0, g_te.teLength);
                ref_set(&g_ref, 0, g_ref.n);
            }
            if (!same_state(&g_te, &g_ref)) {
                /* a CR-adjacent miss is the CR rule's; name it so */
                int cr = 0;
                for (int i = 0; i < g_ref.n; i++) if (g_ref.t[i] == '\r') cr = 1;
                CHECK(0, (rule == R_HIT && cr) ? R_CR : rule,
                      "seq %d op %d: te [%d,%d) n=%d lines=%d vs ref [%d,%d) n=%d lines=%d",
                      seq, op, g_te.selStart, g_te.selEnd, g_te.teLength, g_te.nLines,
                      g_ref.s, g_ref.e, g_ref.n, g_ref.nl);
                break;
            }
            g_checks++;
        }
    }
}

/* ---- P2 HIT + P3 POINT -------------------------------------------------- */
static void p2_p3(void)
{
    for (int c = 0; c < 2000; c++) {
        int width = 30 + rndn(220), n = 1 + rndn(120);
        g_seed = 0x77000u + (uint32_t)c;
        fresh(&g_te, &g_ref, width);
        char buf[160];
        for (int i = 0; i < n; i++) buf[i] = rchar();
        if (!FlairTECanReplace(&g_te, 0, 0, buf, (int16_t)n)) continue;
        FlairTESetText(&g_te, buf, (int16_t)n);
        memcpy(g_ref.t, buf, (size_t)n); g_ref.n = n; ref_layout(&g_ref);
        int has_cr = memchr(buf, '\r', (size_t)n) != NULL;
        for (int p = 0; p < 40; p++) {
            int h = rndn(width + 30) - 15, v = rndn((g_ref.nl + 2) * LH) - 8;
            int got = FlairTEGetOffset(&g_te, (int16_t)h, (int16_t)v);
            int want = ref_hit(&g_ref, h, v);
            int L = v < 0 ? 0 : v / LH; if (L >= g_ref.nl) L = g_ref.nl - 1;
            int crline = L + 1 < g_ref.nl && g_ref.t[g_ref.starts[L + 1] - 1] == '\r';
            CHECK(got == want, (has_cr && crline && got > want) ? R_CR : R_HIT,
                  "case %d GetOffset(%d,%d) = %d, ref %d", c, h, v, got, want);
        }
        for (int o = 0; o <= n; o++) {
            int16_t h, v;
            FlairTEGetPoint(&g_te, (int16_t)o, &h, &v);
            CHECK(h == ref_x(&g_ref, o) && v == ref_line_of(&g_ref, o) * LH, R_POINT,
                  "case %d GetPoint(%d) = (%d,%d), ref (%d,%d)", c, o, h, v,
                  ref_x(&g_ref, o), ref_line_of(&g_ref, o) * LH);
            int back = FlairTEGetOffset(&g_te, h, v);
            int crcase = (o > 0 && g_ref.t[o - 1] == '\r') || (o < n && g_ref.t[o] == '\r');
            CHECK(back == o, crcase ? R_CR : R_HIT, "case %d GetOffset(GetPoint(%d)) = %d", c, o, back);
        }
    }
}

/* ---- P4 PROBE ----------------------------------------------------------- */
static void p4_probe(void)
{
    static char big[RMAX + 64];
    for (int c = 0; c < 400; c++) {
        g_seed = 0xBEEF0u + (uint32_t)c;
        int width = 40 + rndn(200);
        fresh(&g_te, &g_ref, width);
        /* a CR-heavy text near the line cap, or a long text near the length cap */
        int n = (c & 1) ? 236 + rndn(40) : 3900 + rndn(150);
        for (int i = 0; i < n; i++) big[i] = (c & 1) ? ((i % 2) ? '\r' : 'a') : rchar();
        if (c & 1) n = n - (n % 2);
        /* load through the probe (a refusal is fine: it must agree) */
        memcpy(g_ref.t, big, (size_t)n); g_ref.n = n; ref_layout(&g_ref);
        int ref_ok = g_ref.n <= RMAX && g_ref.nl <= RLINES;
        int ok = FlairTECanReplace(&g_te, 0, 0, big, (int16_t)n);
        CHECK(ok == ref_ok, R_PROBE, "case %d load probe %d, ref %d (n=%d lines=%d)", c, ok, ref_ok, n, g_ref.nl);
        if (!ok || !ref_ok) continue;
        FlairTESetText(&g_te, big, (int16_t)n);
        for (int k = 0; k < 30; k++) {
            int a = rndn(n + 1), b = rndn(n + 1), len = rndn(60);
            char run[64];
            for (int i = 0; i < len; i++) run[i] = rndn(3) ? rchar() : '\r';
            ref_t *r = &g_ref;
            ref_t save;
            memcpy(&save, r, sizeof save);
            ref_set(r, a, b);
            ref_replace(r, run, len);
            int rok = r->n <= RMAX && r->nl <= RLINES;
            memcpy(r, &save, sizeof save);
            int got = FlairTECanReplace(&g_te, (int16_t)a, (int16_t)b, run, (int16_t)len);
            CHECK(got == rok, R_PROBE, "case %d replace [%d,%d)+%d probe %d, ref %d", c, a, b, len, got, rok);
            if (got && rok) {                      /* the real edit must fit */
                FlairTESetSelect(&g_te, (int16_t)a, (int16_t)b);
                FlairTEInsert(&g_te, run, (int16_t)len);
                ref_set(r, a, b);
                ref_replace(r, run, len);
                n = r->n;
                CHECK(same_state(&g_te, r), R_EDIT, "case %d edit after a yes-probe diverged", c);
            }
        }
    }
}

/* ---- the hand literals --------------------------------------------------- */
static __attribute__((unused)) void literals(void)
{
    int16_t h, v;
    fresh(&g_te, &g_ref, 200);
    FlairTESetText(&g_te, "aim", 3);
    FlairTEGetPoint(&g_te, 0, &h, &v); CHECK(h == 0 && v == 0, R_LIT, "aim@0 (%d,%d)", h, v);
    FlairTEGetPoint(&g_te, 1, &h, &v); CHECK(h == 6, R_LIT, "aim@1 h=%d want 6", h);
    FlairTEGetPoint(&g_te, 2, &h, &v); CHECK(h == 10, R_LIT, "aim@2 h=%d want 10", h);
    FlairTEGetPoint(&g_te, 3, &h, &v); CHECK(h == 18, R_LIT, "aim@3 h=%d want 18", h);
    CHECK(FlairTEGetOffset(&g_te, 2, 3) == 0, R_HIT, "h=2 in 'a' (6 wide) -> 0");
    CHECK(FlairTEGetOffset(&g_te, 3, 3) == 1, R_HIT, "h=3 = the midpoint of 'a' -> 1");
    CHECK(FlairTEGetOffset(&g_te, 100, 3) == 3, R_HIT, "past the end -> 3");
    FlairTESetText(&g_te, "a\rw", 3);
    CHECK(g_te.nLines == 2 && g_te.lineStarts[1] == 2, R_LIT, "a CR w: line 1 starts at 2");
    CHECK(FlairTEGetOffset(&g_te, 100, 3) == 1, R_CR, "past the end of line 0 -> before the CR (1)");
    CHECK(FlairTEGetOffset(&g_te, 100, 14) == 3, R_HIT, "past the end of line 1 -> 3");
    FlairTEGetPoint(&g_te, 2, &h, &v); CHECK(h == 0 && v == 11, R_LIT, "offset 2 at (0,11), got (%d,%d)", h, v);
    FlairTESetSelect(&g_te, 0, 3);
    FlairTEKey(&g_te, FLAIR_TE_CH_LEFT);
    CHECK(g_te.selStart == 0 && g_te.selEnd == 0, R_ARROW, "left over [0,3) -> caret 0");
    FlairTESetSelect(&g_te, 1, 3);
    FlairTEKey(&g_te, FLAIR_TE_CH_LEFT);
    CHECK(g_te.selStart == 1 && g_te.selEnd == 1, R_ARROW, "left over [1,3) -> caret 1, got %d", g_te.selStart);
    FlairTESetSelect(&g_te, 1, 2);
    (void)FlairTEClick(&g_te, 0, 0, 1);
    CHECK(g_te.selStart == 0 && g_te.selEnd == 2, R_EXTEND, "shift-click at 0 over [1,2) -> [0,2), got [%d,%d)", g_te.selStart, g_te.selEnd);
}

int main(void)
{
#ifndef TE_SURFACE_NO_LITERALS
    literals();
#endif
    p1_edit();
    p2_p3();
    p4_probe();
    printf("test_te_surface: %ld checks, %ld failures\n", g_checks, g_fails);
    return g_fails == 0 ? 0 : 1;
}
