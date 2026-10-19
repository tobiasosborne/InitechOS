/* Initech 123 B12 resized-worksheet geometry oracle, bead initech-89rc (audit
 * L018: "The address became E1, but the view showed only A..D and no cell
 * pointer"). Over the real face_a.c + worksheet core and the recording
 * Toolbox mock of test_i123_quit.c, which serves CONTENT_SIZE (spec/
 * toolbox_gate.h Sec 3) from m_cw x m_ch -- set here to several window sizes,
 * each followed by the updateEvt the shell's SizeWindow damage routes to the
 * tenant (MTE Listing 4-13/4-14: resize, then the update redraws the content
 * for the new size).
 *
 * WHERE THE EXPECTED VALUES COME FROM (Law 1 / Law 2 -- never from face_a.c):
 *   - the Face A screen of the R2.2 manual, pp. 1-3/1-4 (PDF 19/20): three
 *     control-panel lines, the column border, the rows, the status line LAST;
 *     the mode indicator at the RIGHT edge of line one; the row border 4
 *     characters; WHOLE columns only in the worksheet area;
 *   - the 8 x 16 cell grid of the Toolbox (spec/toolbox_gate.h Sec 7a
 *     TBX_CELL_W/H) with Face A's 1-px left margin: char c spans content x
 *     1 + 8c .. 8 + 8c, line L spans y 16L .. 16L + 15, so a content W x H
 *     px shows (W - 1) / 8 whole characters and H / 16 whole lines;
 *   - and from those, BY HAND, the table below (one row per window size; the
 *     frame sizes are the audit's and the B31 hit gate's, the content is the
 *     frame less 27 x 43 px of document chrome, tdnl.51):
 *       frame     content   chars lines  area  cols shown   rows  status
 *       640x440   613x397    76    24     72   8 (A..H)     19    line 23
 *       352x246   325x203    40    12     36   4 (A..D)      7    line 11
 *       440x340   413x297    51    18     47   5 (A..E)     13    line 17
 *       200x150   173x107    21     6     17   1 (A)         1    line 5
 *     (352x246 is the audit's "about 288x194" smaller than 640x440);
 *   - Tab / PgDn move "one screen" (manual pp. 1-9/1-10) -- the screen the
 *     window shows NOW; GOTO an off-screen target puts it at the upper left
 *     (p. 1-11; the B11 rules test-i123-keys grades against the real R2.2);
 *   - the audit's expectation (L018, AUTHORED, native-window visibility from
 *     memory): after every move and every resize the current cell is
 *     visible -- the window scrolls the least that shows it; a grow that
 *     already shows it does not scroll.
 *   - the scroll bars: 123 never gives them a range (spec Sec 15: a tenant's
 *     bars are DISABLED until it does), so they show as unavailable
 *     controls, never as a fake bar over a 256 x 8192 sheet.
 * Mutant (Rule 6; -D on the app build, test-i123-resize-mutant):
 *   I123_MUT_FIXED_GEOMETRY  the face ignores the content size and keeps
 *                            the 76 x 24 constants                    -> R3
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main

#define PX(c)  (1 + 8 * (c))            /* char c's first pixel        */
#define PY(l)  (16 * (l))               /* line l's first pixel        */

/* One row of the table above. */
typedef struct { int w, h, chars, lines, ncols, rows; } geo_t;
static const geo_t Z_FULL  = { 613, 397, 76, 24, 8, 19 };
static const geo_t Z_L018  = { 325, 203, 40, 12, 4,  7 };
static const geo_t Z_HIT   = { 413, 297, 51, 18, 5, 13 };
static const geo_t Z_NARROW = { 173, 107, 21, 6, 1,  1 };
/* The smallest frame the Window Manager allows, 96 x 64 (spec/chrome_metrics.h
 * FLAIR_CHROME_WINDOW_MIN_W/H): content 69 x 21 = 8 characters, 1 line. Too
 * small for the face: it must neither crash nor scribble; line one keeps
 * "A1:" + the mode in its 8 characters (chars 3..7). */
static const geo_t Z_MIN   = {  69,  21,  8, 1, 0,  0 };
static geo_t g_now;

static void kev(int vk, int ch, int mods)
{
    tbx_event_t e;
    memset(&e, 0, sizeof e);
    e.what = keyDown; e.message = (vk << 8) | (ch & 0xFF); e.modifiers = mods;
    tbx_app_event(&e);
}
static void rt(int n)  { while (n--) kev(0x4D, 0, 0); }
static void dn(int n)  { while (n--) kev(0x50, 0, 0); }
static void home(void) { kev(0x47, 0, 0); }
static void tab(void)  { kev(0x0F, '\t', 0); }
static void pgdn(void) { kev(0x51, 0, 0); }
static void click(int x, int y)
{
    tbx_event_t e;
    memset(&e, 0, sizeof e);
    e.what = mouseDown; e.whereh = M_ORG_H + x; e.wherev = M_ORG_V + y;
    tbx_app_event(&e);
}

/* The shell resized the window: wipe the mock screen to '.', then the
 * updateEvt. Whatever the face draws for the new size lands on the dots. */
static void resize(geo_t z)
{
    g_now = z; m_cw = z.w; m_ch = z.h;
    for (int i = 0; i < SCR_H; i++) { memset(m_scr[i], '.', SCR_W); memset(m_bg[i], ' ', SCR_W); }
    ev(updateEvt, 0);
}

static int at(int c, int r, int l, int t)
{
    return i123_cursor_col(&g_sheet) == c && i123_cursor_row(&g_sheet) == r &&
           i123_view_left(&g_sheet) == l && i123_view_top(&g_sheet) == t;
}
static int starts(int line, const char *s) { return strncmp(m_scr[line], s, strlen(s)) == 0; }
/* "READY" in the LAST five characters the window shows on line one. */
static int ready_at_edge(void) { return strncmp(m_scr[0] + g_now.chars - 5, "READY", 5) == 0; }
/* The visible highlight: on the visible row lines (4 .. 4+rows-1) and the
 * visible area characters (4 .. chars-1), exactly [a, a+w) of `line` is NAVY. */
static int pointer_only(int line, int a, int w)
{
    for (int l = 4; l < 4 + g_now.rows; l++)
        for (int c = 4; c < g_now.chars; c++) {
            int in = l == line && c >= a && c < a + w;
            if ((m_bg[l][c] == 'N') != in) return 0;
        }
    return 1;
}
/* The column border shows exactly `letters` (one letter each, width 9:
 * centred at area offset 4 + 9k), nothing else within the window. */
static int border_is(const char *letters)
{
    int n = (int)strlen(letters);
    if (n != g_now.ncols) return 0;
    for (int c = 4; c < g_now.chars; c++) {
        int k = (c - 4) / 9, want = ' ';
        if (k < n && (c - 4) % 9 == 4) want = letters[k];
        if (m_scr[3][c] != want) return 0;
    }
    return 1;
}
/* Nothing drawn outside the window's content (ONE geometry: the face does
 * not paint a 76 x 24 screen into a smaller window and let it clip). */
static int nothing_outside(void)
{
    for (int l = 0; l < SCR_H; l++)
        for (int c = 0; c < SCR_W; c++)
            if ((l >= g_now.lines || c >= g_now.chars) && m_scr[l][c] != '.') return 0;
    return 1;
}
/* For a window too short for the face (Z_MIN): only the column bound. */
static int nothing_outside_cols(void)
{
    for (int l = 0; l < SCR_H; l++)
        for (int c = g_now.chars; c < SCR_W; c++)
            if (m_scr[l][c] != '.') return 0;
    return 1;
}
/* The row border of visible row line `vis` shows row number `r1` (1-based). */
static int rownum(int vis, int r1)
{
    char want[8];
    snprintf(want, sizeof want, "%-4d", r1);
    return strncmp(m_scr[4 + vis], want, 4) == 0;
}

/* R1: full size, the 76 x 24 face, unchanged by B12. */
static void t_full(void)
{
    resize(Z_FULL);
    CHECK(ready_at_edge() && starts(0, "A1: "), "R1", "full size: A1 and READY at chars 71..75");
    CHECK(border_is("ABCDEFGH"), "R1", "full size: the border shows A..H");
    CHECK(rownum(0, 1) && rownum(18, 19) && pointer_only(4, 4, 9), "R1", "full size: rows 1..19, the pointer on A1");
}

/* R2: shrink to the audit's size -- the face reflows to 40 x 12. */
static void t_shrink(void)
{
    resize(Z_L018);
    CHECK(ready_at_edge(), "R2", "352x246: READY moves to the content's right edge (chars 35..39)");
    CHECK(border_is("ABCD"), "R2", "352x246: the border shows exactly A..D");
    CHECK(rownum(0, 1) && rownum(6, 7) && pointer_only(4, 4, 9), "R2", "352x246: rows 1..7, the pointer on A1");
    CHECK(m_scr[11][0] != '.' && nothing_outside(), "R2", "352x246: the status line is line 11; nothing drawn past 40 x 12");
}

/* R3: THE L018 SCENE -- small window, Right to E1: the view follows. */
static void t_l018(void)
{
    resize(Z_L018);
    rt(4);
    CHECK(at(4, 0, 1, 0) && starts(0, "E1: "), "R3", "352x246, Right x4: E1 with column B first (the least scroll)");
    CHECK(border_is("BCDE"), "R3", "352x246, E1: the border shows B..E");
    CHECK(pointer_only(4, 31, 9), "R3", "352x246, E1: the pointer is visible on E (chars 31..39 of line 4)");
    CHECK(ready_at_edge(), "R3", "352x246, E1: READY at the right edge");
    dn(7);
    CHECK(at(4, 7, 1, 1) && starts(0, "E8: "), "R4", "352x246, Down x7: E8 with row 2 first (7 rows show)");
    CHECK(rownum(0, 2) && rownum(6, 8) && pointer_only(10, 31, 9), "R4", "352x246, E8: rows 2..8, the pointer on the last row line");
    /* R5: grow back -- the pointer is already visible, nothing scrolls */
    resize(Z_FULL);
    CHECK(at(4, 7, 1, 1), "R5", "grown to 640x440: the view is kept (the pointer was visible)");
    CHECK(border_is("BCDEFGHI") && rownum(0, 2) && rownum(18, 20), "R5", "640x440: B..I and rows 2..20");
    CHECK(pointer_only(10, 31, 9) && ready_at_edge() && starts(0, "E8: "), "R5", "640x440: the pointer still on E8, READY at char 71");
}

/* R6: shrink while the pointer sits where the small window cannot show it. */
static void t_shrink_follows(void)
{
    rt(7); dn(18);
    CHECK(at(7, 18, 0, 0) && starts(0, "H19: "), "R0", "setup: full size, the pointer on H19, view at A1");
    resize(Z_L018);
    CHECK(at(7, 18, 4, 12), "R6", "shrunk to 352x246: the view scrolls to E13 so H19 shows");
    CHECK(border_is("EFGH") && rownum(0, 13) && rownum(6, 19), "R6", "352x246: E..H and rows 13..19");
    CHECK(pointer_only(10, 31, 9) && starts(0, "H19: ") && ready_at_edge(), "R6", "352x246: the pointer on H19 (line 10, chars 31..39)");
    resize(Z_NARROW);
    CHECK(at(7, 18, 7, 18) && border_is("H") && rownum(0, 19) && pointer_only(4, 4, 9), "R11", "200x150: one column, one row: H19 alone");
    CHECK(ready_at_edge() && m_scr[5][0] != '.' && nothing_outside(), "R11", "200x150: READY at chars 16..20, status line 5, nothing past 21 x 6");
    resize(Z_FULL);
    CHECK(at(7, 18, 7, 18) && border_is("HIJKLMNO") && pointer_only(4, 4, 9), "R11", "640x440 again: the view kept at H19");
}

/* R7: the mouse maps through the SAME geometry (Z_HIT: 51 x 18). */
static void t_hit(void)
{
    resize(Z_HIT);
    CHECK(border_is("ABCDE") && ready_at_edge(), "R7", "440x340: A..E, READY at chars 46..50");
    click(PX(4 + 4 * 9), PY(4 + 12) + 8);
    CHECK(at(4, 12, 0, 0) && starts(0, "E13: "), "R7", "440x340: the last row line, fifth column is E13");
    click(PX(4), PY(17) + 4);
    CHECK(at(4, 12, 0, 0), "R7", "440x340: a click on the status line (line 17) selects nothing");
    click(PX(49), PY(5));
    click(PX(50) + 7, PY(5));
    CHECK(at(4, 12, 0, 0), "R7", "440x340: the strip right of E (chars 49..50) selects nothing");
    click(PX(4), PY(4));
    CHECK(at(0, 0, 0, 0) && pointer_only(4, 4, 9), "R7", "440x340: the first visible cell is A1");
}

/* R8: Tab / PgDn move one screen of the window as it is NOW; R9 GOTO. */
static void t_pages(void)
{
    resize(Z_L018);
    pgdn();
    CHECK(at(0, 7, 0, 7) && starts(0, "A8: ") && rownum(0, 8), "R8", "352x246, PgDn: A8 with row 8 first (7 rows a screen)");
    tab();
    CHECK(at(4, 7, 4, 7) && starts(0, "E8: ") && border_is("EFGH"), "R8", "352x246, Tab: E8 with column E first (4 columns a screen)");
    home();
    kev(0x3F, 0, 0); tks("F3\r");
    CHECK(at(5, 2, 5, 2) && starts(0, "F3: ") && pointer_only(4, 4, 9), "R9", "352x246, GOTO F3 (off the A..D view): F3 at the upper left");
    home();
    kev(0x3F, 0, 0); tks("C3\r");
    CHECK(at(2, 2, 0, 0) && pointer_only(6, 22, 9), "R9", "352x246, GOTO C3 (on screen): the pointer moves, no scroll");
}

/* R12: the minimum window, with a 60-character label in the current cell
 * (the content line one would show): no crash, nothing past 8 characters,
 * and the full face back on the next grow. */
static void t_min(void)
{
    tks("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefgh\r");
    CHECK(starts(0, "A1: 'abcdefgh"), "R0", "setup: A1 holds the 60-character label");
    resize(Z_MIN);
    CHECK(strncmp(m_scr[0], "A1:READY", 8) == 0, "R12", "96x64: line one is \"A1:\" + READY in the 8 characters");
    CHECK(nothing_outside_cols(), "R12", "96x64: nothing drawn past character 8");
    rt(1); dn(1);
    CHECK(at(1, 1, 1, 1), "R12", "96x64: Right, Down -> B2 with the view following");
    /* a collapsed (window-shaded) window: CONTENT_SIZE answers 0 (spec Sec 3) */
    m_cw = 0; m_ch = 0; ev(updateEvt, 0);
    CHECK(at(1, 1, 1, 1), "R12", "collapsed: an update with no content moves nothing");
    resize(Z_FULL);
    CHECK(at(1, 1, 1, 1) && border_is("BCDEFGHI") && pointer_only(4, 4, 9) && ready_at_edge(), "R12", "640x440 again: B..I, the pointer on B2");
}

/* R10: no fake bars -- through resizes, moves, pages and clicks 123 never
 * calls SETSCROLL, so the Toolbox keeps both bars DISABLED (spec Sec 15). */
static void t_bars(void)
{
    resize(Z_L018); rt(9); dn(30); pgdn(); tab();
    resize(Z_HIT); click(PX(4), PY(4));
    resize(Z_NARROW); home();
    resize(Z_FULL); rt(3);
    CHECK(m_setscroll == 0, "R10", "123 never gives the scroll bars a range: they stay unavailable");
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    m_dir = argv[1];
    g_ctr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (g_ctr == MAP_FAILED) return 2;
    g_now = Z_FULL;
    launch("FULL", t_full);
    launch("SHRINK", t_shrink);
    launch("L018", t_l018);
    launch("FOLLOW", t_shrink_follows);
    launch("HIT", t_hit);
    launch("PAGES", t_pages);
    launch("MIN", t_min);
    launch("BARS", t_bars);
    printf("i123-resize: %d checks, %d failures\n", g_ctr[0], g_ctr[1]);
    return g_ctr[1] ? 1 : 0;
}
