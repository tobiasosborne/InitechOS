/* Initech 123 B31 cell hit-testing oracle, bead initech-umfi (audit L017:
 * "The address and highlight remained A1" after a click on a worksheet cell).
 * A mouseDown in the worksheet content selects the visible cell under it,
 * over the real face_a.c + worksheet core and the recording Toolbox mock of
 * test_i123_quit.c (which serves CONTENTORIGIN as the GLOBAL (6,62)).
 *
 * WHERE THE EXPECTED VALUES COME FROM (Law 1 / Law 2 -- never from face_a.c):
 *   - the Face A screen as the R2.2 manual lays it out, pp. 1-3/1-4 (PDF
 *     19/20): three control-panel lines, the column letters in the top
 *     border, the row numbers in the left border, the worksheet area, the
 *     status line -- restated here as FIXED GEOMETRY CONSTANTS: 8 x 16 cells,
 *     cell (char c, line L) at content-local x 1 + 8c .. 8 + 8c, y 16L ..
 *     16L + 15 (spec/flair_i123_traces.mk GEOMETRY); control panel lines
 *     0..2, the column border line 3, rows on lines 4..22 (24 lines less 3
 *     panel lines, the border and the status line), status line 23; the row
 *     border chars 0..3, the area chars 4..75 holding WHOLE columns only
 *     (72 characters: 8 columns of the default width 9, MINT01);
 *   - column widths the test itself sets, and the window's first row/column
 *     the test itself reaches by Tab / PgDn (one screen: T_COLS / T_ROWS, the
 *     B11 rules test-i123-keys grades against the real R2.2);
 *   - the native document-window expectation of the audit (L017, from memory,
 *     AUTHORED): a click on a cell selects it, READY stays READY; a click
 *     while an entry is being typed or edited completes it first, as a
 *     pointer-movement key does (manual p. 1-16, PDF 32: "Press one of the
 *     pointer-movement keys to simultaneously complete the entry and move the
 *     cell pointer"); a value that does not parse stays in EDIT and the
 *     pointer stays (as with Tab/PgDn); borders, the control panel, the status
 *     line and the blank strip right of the last whole column select nothing.
 * Mutant (Rule 6; -D on the app build, test-i123-hit-mutant):
 *   I123_MUT_HIT_IGNORES_SCROLL  the hit maps as if the window sat at A1 -> H5
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main

#define T_ROWS (SCR_H - 5)              /* 19 rows on screen          */
#define T_COLS ((SCR_W - 4) / 9)        /* 8 default columns          */
#define L_ROW0 4                        /* row line 0                 */
#define C_AREA 4                        /* first area character       */
#define PX(c)  (1 + 8 * (c))            /* char c's first pixel        */
#define PY(l)  (16 * (l))               /* line l's first pixel        */

static void kev(int vk, int ch, int mods)
{
    tbx_event_t e;
    memset(&e, 0, sizeof e);
    e.what = keyDown; e.message = (vk << 8) | (ch & 0xFF); e.modifiers = mods;
    tbx_app_event(&e);
}
static void tab(void)  { kev(0x0F, '\t', 0); }
static void pgdn(void) { kev(0x51, 0, 0); }
static void down(void) { kev(0x50, 0, 0); }
static void home(void) { kev(0x47, 0, 0); }
static void f2(void)   { kev(0x3C, 0, 0); }

/* a mouseDown at CONTENT-LOCAL (x,y): the event carries the GLOBAL point */
static void click(int x, int y)
{
    tbx_event_t e;
    memset(&e, 0, sizeof e);
    e.what = mouseDown; e.whereh = M_ORG_H + x; e.wherev = M_ORG_V + y;
    tbx_app_event(&e);
}

/* pointer at (c,r), window upper-left at (l,t) -- 0-based */
static int at(int c, int r, int l, int t)
{
    return i123_cursor_col(&g_sheet) == c && i123_cursor_row(&g_sheet) == r &&
           i123_view_left(&g_sheet) == l && i123_view_top(&g_sheet) == t;
}
static int starts(int line, const char *s) { return strncmp(m_scr[line], s, strlen(s)) == 0; }
static int ready_mode(void)
{
    return g_mode == M_READY && strcmp(m_scr[0] + SCR_W - 5, "READY") == 0;
}
static int label_is(int c, int r, const char *text)
{
    char got[512];
    cell((uint16_t)c, (uint16_t)r, got, sizeof got);
    return strcmp(got, text) == 0;
}
/* The highlight: on the row lines, area chars [a, a + w) of `line` are NAVY
 * and NO other area character of any row line is. */
static int highlight_only(int line, int a, int w)
{
    for (int l = L_ROW0; l < L_ROW0 + T_ROWS; l++)
        for (int c = C_AREA; c < SCR_W; c++) {
            int in = l == line && c >= a && c < a + w;
            if ((m_bg[l][c] == 'N') != in) return 0;
        }
    return 1;
}

/* H1..H4: first / last visible cell, the borders, the partial edge. */
static void t_bounds(void)
{
    down(); down(); kev(0x4D, 0, 0); kev(0x4D, 0, 0);       /* C3 */
    CHECK(at(2, 2, 0, 0), "H0", "setup: the pointer on C3");
    click(PX(C_AREA), PY(L_ROW0));
    CHECK(at(0, 0, 0, 0), "H1", "the top-left pixel of the first visible cell selects A1");
    CHECK(ready_mode() && starts(0, "A1: "), "H1", "line one shows A1, READY stays READY");
    CHECK(highlight_only(L_ROW0, C_AREA, 9), "H1", "the highlight moves to A1 and leaves C3");
    click(PX(C_AREA + 4), PY(L_ROW0 + 2) + 7);
    CHECK(at(0, 2, 0, 0) && starts(0, "A3: "), "H1", "a click inside A3 selects A3");
    click(PX(C_AREA + 8) + 7, PY(L_ROW0) + 15);
    CHECK(at(0, 0, 0, 0), "H1", "the bottom-right pixel of A1 is still A1");
    click(PX(C_AREA + 9), PY(L_ROW0));
    CHECK(at(1, 0, 0, 0) && starts(0, "B1: "), "H1", "the next pixel to the right is B1");
    /* the last whole cell: column H (chars 67..75), row 19 (line 22) */
    click(PX(C_AREA + 7 * 9), PY(L_ROW0 + T_ROWS - 1));
    CHECK(at(7, T_ROWS - 1, 0, 0) && starts(0, "H19: "), "H2", "the top-left pixel of the last visible cell selects H19");
    CHECK(highlight_only(L_ROW0 + T_ROWS - 1, C_AREA + 63, 9), "H2", "the highlight sits on H19");
    home();
    click(PX(C_AREA + 8 * 9 - 1) + 7, PY(L_ROW0 + T_ROWS - 1) + 15);
    CHECK(at(7, T_ROWS - 1, 0, 0), "H2", "the bottom-right pixel of H19 selects H19 without scrolling");
    /* H3: nothing outside the worksheet area selects anything */
    home(); down(); down(); down(); down(); kev(0x4D, 0, 0); kev(0x4D, 0, 0); kev(0x4D, 0, 0);   /* D5 */
    {
        static const int pts[][2] = {
            { PX(0), PY(L_ROW0 + 6) },                 /* row border, left  */
            { PX(3) + 7, PY(L_ROW0 + 6) + 8 },         /* row border, right */
            { 0, PY(L_ROW0 + 6) },                     /* the 1-px margin   */
            { PX(C_AREA + 20), PY(3) },                /* column border     */
            { PX(C_AREA + 20), PY(3) + 15 },
            { PX(C_AREA + 2), PY(0) },                 /* control panel 1   */
            { PX(C_AREA + 2), PY(1) + 8 },             /* control panel 2   */
            { PX(C_AREA + 2), PY(2) + 15 },            /* control panel 3   */
            { PX(C_AREA + 2), PY(SCR_H - 1) },         /* the status line   */
            { PX(C_AREA + 2), PY(SCR_H) + 5 },         /* below the status  */
            { PX(C_AREA + 2), -1 },                    /* above the content */
            { -1, PY(L_ROW0 + 1) },                    /* left of it        */
        };
        for (unsigned i = 0; i < sizeof pts / sizeof pts[0]; i++) {
            click(pts[i][0], pts[i][1]);
            if (!at(3, 4, 0, 0) || !ready_mode() || !starts(0, "D5: ") || !highlight_only(L_ROW0 + 4, C_AREA + 27, 9)) {
                CHECK(0, "H3", "a click on a border / the control panel / the status line moved the pointer");
                fprintf(stderr, "    (point %u: %d,%d)\n", i, pts[i][0], pts[i][1]);
            }
        }
        CHECK(1, "H3", "borders, control panel and status line ignored");
    }
    /* H4: the partial edge -- right of the last whole column */
    click(PX(C_AREA + 72), PY(L_ROW0 + 1));
    click(PX(SCR_W) + 3, PY(L_ROW0 + 1));                /* the 5 spare pixels */
    CHECK(at(3, 4, 0, 0), "H4", "the strip right of the 72-character area selects nothing");
    (void)i123_set_colwidth(&g_sheet, 2, 20);             /* C 20: A B C D E F = 65 chars */
    ev(updateEvt, 0);
    click(PX(C_AREA + 65), PY(L_ROW0 + 1));
    click(PX(C_AREA + 71) + 7, PY(L_ROW0 + 1));
    CHECK(at(3, 4, 0, 0), "H4", "with C 20 wide, chars 65..71 (G would not fit whole) select nothing");
    click(PX(C_AREA + 64) + 7, PY(L_ROW0 + 1));
    CHECK(at(5, 1, 0, 0) && starts(0, "F2: "), "H4", "the last char of F (chars 56..64) selects F2");
}

/* H5: after a horizontal Tab scroll; H6: after a vertical PgDn. */
static void t_scroll(void)
{
    tab();
    CHECK(at(T_COLS, 0, T_COLS, 0), "H0", "setup: Tab puts I first");
    click(PX(C_AREA), PY(L_ROW0));
    CHECK(at(T_COLS, 0, T_COLS, 0) && starts(0, "I1: "), "H5", "after Tab, the first visible cell is I1");
    click(PX(C_AREA + 4 * 9) + 3, PY(L_ROW0 + 3) + 8);
    CHECK(at(T_COLS + 4, 3, T_COLS, 0) && starts(0, "M4: "), "H5", "after Tab, the fifth column, row 4 is M4 (unscrolled: E4)");
    CHECK(highlight_only(L_ROW0 + 3, C_AREA + 36, 9), "H5", "the highlight sits on M4");
    home(); pgdn();
    CHECK(at(0, T_ROWS, 0, T_ROWS), "H0", "setup: PgDn puts row 20 first");
    click(PX(C_AREA), PY(L_ROW0));
    CHECK(at(0, T_ROWS, 0, T_ROWS) && starts(0, "A20: "), "H6", "after PgDn, the first visible cell is A20");
    click(PX(C_AREA + 4 * 9), PY(L_ROW0 + T_ROWS - 1) + 15);
    CHECK(at(4, 2 * T_ROWS - 1, 0, T_ROWS) && starts(0, "E38: "), "H6", "after PgDn, the last row's fifth column is E38");
    tab();
    click(PX(C_AREA + 9) + 1, PY(L_ROW0 + 1));
    CHECK(at(T_COLS + 1, T_ROWS + 1, T_COLS, T_ROWS) && starts(0, "J21: "), "H6", "after PgDn and Tab, the second column, second row is J21");
}

/* H7: unequal column widths -- A 3, B 15, C 9, D 1, E 9 ... */
static void t_widths(void)
{
    (void)i123_set_colwidth(&g_sheet, 0, 3);
    (void)i123_set_colwidth(&g_sheet, 1, 15);
    (void)i123_set_colwidth(&g_sheet, 3, 1);
    ev(updateEvt, 0);
    /* chars: A 4..6, B 7..21, C 22..30, D 31, E 32..40 */
    static const struct { int ch, col; const char *addr; } w[] = {
        { C_AREA + 2, 0, "A6: " }, { C_AREA + 3, 1, "B6: " }, { C_AREA + 17, 1, "B6: " },
        { C_AREA + 18, 2, "C6: " }, { C_AREA + 26, 2, "C6: " }, { C_AREA + 27, 3, "D6: " },
        { C_AREA + 28, 4, "E6: " },
    };
    for (unsigned i = 0; i < sizeof w / sizeof w[0]; i++) {
        click(PX(w[i].ch) + (i & 1 ? 7 : 0), PY(L_ROW0 + 5) + 4);
        CHECK(at(w[i].col, 5, 0, 0) && starts(0, w[i].addr), "H7", "unequal widths: the char's column, from the widths set here");
    }
    click(PX(C_AREA + 3), PY(L_ROW0 + 5));
    CHECK(highlight_only(L_ROW0 + 5, C_AREA + 3, 15), "H7", "the highlight spans B's 15 characters");
}

/* H8: a click while an entry is typed / edited completes it first. */
static void t_entry(void)
{
    tks("abc");
    CHECK(g_mode == M_LABEL, "H0", "setup: typing a label");
    click(PX(C_AREA + 2 * 9), PY(L_ROW0 + 4));
    CHECK(ready_mode() && label_is(0, 0, "'abc") && at(2, 4, 0, 0) && starts(0, "C5: "), "H8", "LABEL + click stores 'abc in A1 and selects C5 (p. 1-16)");
    tks("12");
    click(PX(C_AREA), PY(L_ROW0));
    CHECK(ready_mode() && label_is(2, 4, "12") && at(0, 0, 0, 0), "H8", "VALUE + click stores 12 in C5 and selects A1");
    f2(); tk('X');
    click(PX(C_AREA + 9), PY(L_ROW0 + 1));
    CHECK(ready_mode() && label_is(0, 0, "'abcX") && at(1, 1, 0, 0), "H8", "EDIT + click stores 'abcX and selects B2");
    tks("(1");
    click(PX(C_AREA), PY(L_ROW0 + 6));
    CHECK(g_mode == M_EDIT && at(1, 1, 0, 0) && !i123_find(&g_sheet, 1, 1), "H8", "a value that does not parse stays in EDIT; the pointer stays");
    tk(27); tk(27);
    tks("zz");
    click(PX(1), PY(L_ROW0 + 6));
    CHECK(g_mode == M_LABEL && g_elen == 2 && at(1, 1, 0, 0), "H8", "a click on the border during an entry is ignored: the entry stays");
    tk(27);
    tk('/');
    click(PX(C_AREA), PY(L_ROW0 + 6));
    CHECK(g_mode == M_MAIN && at(1, 1, 0, 0), "H8", "a click in MENU mode selects nothing");
    tk(27);
    CHECK(ready_mode(), "H8", "Esc leaves the menu");
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    m_dir = argv[1];
    g_ctr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (g_ctr == MAP_FAILED) return 2;
    launch("BOUNDS", t_bounds);
    launch("SCROLL", t_scroll);
    launch("WIDTHS", t_widths);
    launch("ENTRY", t_entry);
    printf("i123-hit: %d checks, %d failures\n", g_ctr[0], g_ctr[1]);
    return g_ctr[1] ? 1 : 0;
}
