/* Initech 123 B11 keyboard oracle, bead initech-69mz: EDIT (F2), GOTO (F5),
 * PGDN / PGUP and TAB / SHIFT-TAB over the real face_a.c + worksheet core and
 * the recording Toolbox mock of test_i123_quit.c.
 *
 * WHERE THE EXPECTED VALUES COME FROM (Law 1 / Law 2 -- never from face_a.c):
 *   - Lotus 1-2-3 R2.2 Reference Manual pp. 1-8..1-11 (PDF 24..27: the key
 *     tables), 1-16 (a pointer-movement key completes an entry), 1-22/1-23
 *     (EDIT mode keys: Esc "Erases all characters in entry");
 *   - 123.RI (../lotus123-decomp/goldens/lotus-123-2.2-installed-ref/
 *     extracted/installed/123.RI): 0x5a5a "Enter address to go to: ",
 *     0x5ece "Invalid cell or range address";
 *   - the REAL R2.2 driven under DOSBox-X, fixtures/i123-keys/keys*.steps and
 *     their captures (named per check below). R2.2 shows 20 rows x 8 columns;
 *     the RULES read off the captures are applied here to the native window
 *     the mock records: T_ROWS = the mock's 24 lines less 3 panel lines, the
 *     border and the status line; T_COLS = the 72-character area / the
 *     default width 9 (wk1 default, MINT01).
 * Mutants (Rule 6; -D on the app build, test-i123-keys-mutant):
 *   I123_MUT_TAB_ONE_CELL      Tab moves one cell              -> K18
 *   I123_MUT_GOTO_FOLLOW       GOTO scrolls like an arrow      -> K8
 *   I123_MUT_EDIT_ESC_CANCELS  one Esc leaves EDIT             -> K1
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main

#define T_ROWS (SCR_H - 5)              /* 19 */
#define T_COLS ((SCR_W - 4) / 9)        /* 8  */

static void kev(int vk, int ch, int mods)
{
    tbx_event_t e;
    memset(&e, 0, sizeof e);
    e.what = keyDown; e.message = (vk << 8) | (ch & 0xFF); e.modifiers = mods;
    tbx_app_event(&e);
}
static void f2(void)     { kev(0x3C, 0, 0); }
static void f5(void)     { kev(0x3F, 0, 0); }
static void pgdn(void)   { kev(0x51, 0, 0); }
static void pgup(void)   { kev(0x49, 0, 0); }
static void tab(void)    { kev(0x0F, '\t', 0); }
static void btab(void)   { kev(0x0F, '\t', 0x0200); }   /* shiftKey, MTE 2-2 */
static void up(void)     { kev(0x48, 0, 0); }
static void down(void)   { kev(0x50, 0, 0); }
static void left(void)   { kev(0x4B, 0, 0); }
static void home(void)   { kev(0x47, 0, 0); }
static void go(const char *a) { f5(); tks(a); tk('\r'); }

/* pointer at (c,r), window upper-left at (l,t) -- 0-based */
static int at(int c, int r, int l, int t)
{
    return i123_cursor_col(&g_sheet) == c && i123_cursor_row(&g_sheet) == r &&
           i123_view_left(&g_sheet) == l && i123_view_top(&g_sheet) == t;
}
static int starts(int line, const char *s) { return strncmp(m_scr[line], s, strlen(s)) == 0; }
static int mode_is(const char *m)
{
    const char *e = m_scr[0] + SCR_W - strlen(m);
    return strcmp(e, m) == 0;
}
static int label_is(int c, int r, const char *text)
{
    char got[512];
    cell((uint16_t)c, (uint16_t)r, got, sizeof got);
    return strcmp(got, text) == 0;
}

/* ---- EDIT (F2) ----------------------------------------------------------- */
static void t_edit(void)
{
    char want[SCR_W + 1];
    tks("Hello\r");
    f2();
    /* K1 fixtures 02_f2: "'Hello" on line two, EDIT; 03/04: Esc erases, Esc again READY */
    snprintf(want, sizeof want, "%-*s", SCR_W, "'Hello_");
    CHECK(g_mode == M_EDIT && mode_is("EDIT") && !strcmp(m_scr[1], want), "K1", "F2 puts the current cell in EDIT on line two");
    CHECK(starts(0, "A1: 'Hello"), "K1", "line one keeps the cell's contents");
    tk(27);
    snprintf(want, sizeof want, "%-*s", SCR_W, "_");
    CHECK(g_mode == M_EDIT && !strcmp(m_scr[1], want), "K1", "the first Esc erases the line and stays in EDIT (p. 1-23, 03_f2_esc1)");
    tk(27);
    CHECK(g_mode == M_READY && mode_is("READY") && label_is(0, 0, "'Hello"), "K1", "the second Esc returns READY, the cell unchanged (04_f2_esc2)");
    /* K2 05_f2_commit: F2, type X, Enter -> 'HelloX */
    f2(); tk('X'); tk('\r');
    CHECK(g_mode == M_READY && label_is(0, 0, "'HelloX"), "K2", "Enter stores the edited label");
    /* K3 44_f2_value: a value edits as its digits and stays a value */
    right(); tks("123\r"); f2();
    CHECK(g_mode == M_EDIT && starts(1, "123_"), "K3", "F2 on a value shows its digits");
    tk('4'); tk('\r');
    { const i123_cell_t *c = i123_find(&g_sheet, 1, 0); i123_val_t v; i123_cell_value(&g_sheet, 1, 0, &v);
      CHECK(c && (c->kind == I123_K_INTEGER || c->kind == I123_K_NUMBER) && v.t == I123_V_NUM && v.n == 1234, "K3", "the edited value commits as the number 1234"); }
    /* K4 45/46: F2, Esc, Enter leaves the cell as it was */
    f2(); tk(27); tk('\r');
    CHECK(g_mode == M_READY && label_is(1, 0, "1234"), "K4", "an emptied EDIT line completes as no entry (45_f2_cleared_enter)");
    /* K5 47/48: F2 on an empty cell is an empty EDIT line; Enter stores nothing */
    right(); f2();
    CHECK(g_mode == M_EDIT && starts(1, "_ "), "K5", "F2 on an empty cell: EDIT, empty line (47_f2_empty)");
    tk('\r');
    CHECK(g_mode == M_READY && !i123_find(&g_sheet, 2, 0), "K5", "Enter stores nothing (48_f2_empty_enter)");
    /* K6 49_edit_down: Down completes EDIT and moves the pointer */
    home(); f2(); tk('Z'); down();
    CHECK(g_mode == M_READY && label_is(0, 0, "'HelloXZ") && at(0, 1, 0, 0), "K6", "EDIT + Down stores and moves (p. 1-8)");
    /* K7 p. 1-11 / 1-22: F2 switches EDIT <-> LABEL/VALUE */
    up(); f2(); f2();
    CHECK(g_mode == M_LABEL && mode_is("LABEL"), "K7", "F2 in EDIT on a label entry switches to LABEL (p. 1-11)");
    f2();
    CHECK(g_mode == M_EDIT, "K7", "F2 while typing switches to EDIT (p. 1-22)");
    tk(27); tk(27);
    tks("12"); f2();
    CHECK(g_mode == M_EDIT, "K7", "F2 during a VALUE entry switches to EDIT");
    f2();
    CHECK(g_mode == M_VALUE && mode_is("VALUE"), "K7", "F2 in EDIT on a value entry switches to VALUE");
    tk(27);
    /* K7b: Tab/Left/Right inside EDIT do not move the pointer (cursor keys) */
    f2(); tab(); left();
    CHECK(g_mode == M_EDIT && at(0, 0, 0, 0), "K7", "EDIT keeps Tab/Left to itself; the pointer stays");
    tk(27); tk(27);
}

/* K22 fixtures 70..75 + T240.WK1: F2 on a 240-character label shows prefix +
 * 239 characters (the line holds 240), refuses more, and Enter stores the
 * 239 characters -- exactly what the real R2.2 wrote to T240.WK1. */
static void t_edit240(void)
{
    char typed[241], want[242], got[512];
    for (int i = 0; i < 240; i++) typed[i] = (char)('a' + i % 26);
    typed[240] = 0;
    tks(typed); tk('\r');
    cell(0, 0, got, sizeof got);
    CHECK(strlen(got) == 241, "K22", "240 typed characters are stored behind the prefix");
    f2();
    CHECK(g_mode == M_EDIT && g_elen == 240 && g_edit[0] == '\'', "K22", "F2 line holds 240 characters, prefix included (72_f2_240)");
    tk('Q');
    CHECK(g_elen == 240 && m_beeps == 1, "K22", "a 241st character beeps (73_f2_240_typeq)");
    tk('\r');
    want[0] = '\''; memcpy(want + 1, typed, 239); want[240] = 0;
    CHECK(label_is(0, 0, want), "K22", "Enter stores prefix + 239 characters (T240.WK1)");
}

/* ---- GOTO (F5) ------------------------------------------------------------ */
static void t_goto(void)
{
    char want[SCR_W + 1];
    f5();
    /* K8 06_f5_prompt / 07_f5_typed: the prompt and the current address, POINT; typing -> EDIT */
    CHECK(g_mode == M_RANGE && starts(1, "Enter address to go to: A1") && mode_is("POINT"),
          "K8", "F5 prompts with 123.RI 0x5a5a and the current address in POINT");
    tks("z100");
    CHECK(starts(1, "Enter address to go to: z100_") && mode_is("EDIT"), "K8", "typing the address shows EDIT (07_f5_typed)");
    tk('\r');
    /* 08_f5_z100: an off-screen target becomes the upper-left cell */
    CHECK(g_mode == M_READY && at(25, 99, 25, 99), "K8", "GOTO Z100 puts Z100 at the window's upper-left");
    CHECK(starts(0, "Z100: ") && starts(4, "100 ") && m_scr[3][4 + 4] == 'Z' && m_scr[3][4 + 9 + 3] == 'A',
          "K8", "the screen shows column Z first and row 100 on top");
    /* K9 an on-screen target only moves the pointer (COPY01 08_show_C20) */
    go("ab105");
    CHECK(at(27, 104, 25, 99), "K9", "GOTO to a visible cell does not scroll");
    /* K10 37_goto_a21 / 60_goto_d21: one row past the bottom is off screen */
    home();
    { char a[8]; snprintf(a, sizeof a, "a%d", T_ROWS + 1); go(a); }
    CHECK(at(0, T_ROWS, 0, T_ROWS), "K10", "the row just below the window becomes the top row");
    home();
    { char a[8]; snprintf(a, sizeof a, "d%d", T_ROWS + 1); go(a); }
    CHECK(at(3, T_ROWS, 3, T_ROWS), "K10", "D past the bottom: both axes move to the target (60_goto_d21)");
    /* K11 10_f5_bad / 11_f5_bad_esc: an invalid address */
    { unsigned b = m_beeps;
      f5(); tks("zz1"); tk('\r');
      CHECK(m_beeps == b + 1 && g_mode == M_NOTICE && mode_is("ERROR"), "K11", "an invalid address beeps into ERROR");
      CHECK(starts(SCR_H - 1, "Invalid cell or range address"), "K11", "the 123.RI 0x5ece message is on the status line");
      CHECK(starts(1, "Enter address to go to: zz1"), "K11", "the prompt and the typed text stay on line two");
      CHECK(at(3, T_ROWS, 3, T_ROWS), "K11", "the pointer stays");
      tk('\r');
      CHECK(g_mode == M_READY && mode_is("READY") && !starts(SCR_H - 1, "Invalid"), "K11", "Enter clears the error (p. 1-5)");
      f5(); tks("iw1"); tk('\r'); tk(27);
      CHECK(g_mode == M_READY && !starts(SCR_H - 1, "Invalid") && at(3, T_ROWS, 3, T_ROWS), "K11", "Esc clears it too; pointer unchanged");
      CHECK(m_beeps == b + 2, "K11", "IW1 is past IV: refused with a beep"); }
    /* K12 69_goto_default: F5 Enter stays */
    f5(); tk('\r');
    CHECK(g_mode == M_READY && at(3, T_ROWS, 3, T_ROWS), "K12", "F5 Enter goes to the current cell");
    /* K16 42_a8190 / 43_a8190_pgdn: past row 8192 the window shows empty rows */
    go("a8190");
    CHECK(at(0, 8189, 0, 8189), "K16", "GOTO A8190 tops the window at row 8190");
    snprintf(want, sizeof want, "%-*s", SCR_W, "");
    memcpy(want, "    ", 4);
    CHECK(starts(4 + 2, "8192") && !strcmp(m_scr[4 + 3], want), "K16", "rows past 8192 are blank, border included");
    pgdn();
    CHECK(at(0, 8191, 0, 8189), "K16", "PgDn near the end stops on row 8192, window unmoved");
    pgdn();
    CHECK(at(0, 8191, 0, 8189), "K16", "PgDn on row 8192 stays");
}

/* ---- PGDN / PGUP ------------------------------------------------------------ */
static void t_page(void)
{
    char want[8];
    /* K13 12_pgdn / 13 / 14: both pointer and window move one screen */
    pgdn();
    CHECK(at(0, T_ROWS, 0, T_ROWS), "K13", "PgDn from A1: pointer and window one screen down");
    snprintf(want, sizeof want, "%d", T_ROWS + 1);
    CHECK(starts(4, want), "K13", "the top row number is the next screen's first row");
    pgdn();
    CHECK(at(0, 2 * T_ROWS, 0, 2 * T_ROWS), "K13", "PgDn again");
    pgup();
    CHECK(at(0, T_ROWS, 0, T_ROWS), "K13", "PgUp back one screen");
    /* K14 15_pgdn_mid: the pointer keeps its place on screen */
    home(); down(); down(); down(); pgdn();
    CHECK(at(0, T_ROWS + 3, 0, T_ROWS), "K14", "PgDn from A4: A4 + one screen, window one screen");
    /* K15 64/65/66: PgUp clamps each at row 1 */
    home(); go("a30"); up(); up(); up(); up(); up();
    CHECK(at(0, 24, 0, 24), "K15", "GOTO A30 then Up x5: A25 on top");
    pgup();
    CHECK(at(0, 24 - T_ROWS, 0, 24 - T_ROWS), "K15", "PgUp: both one screen up");
    down(); down(); pgup();
    CHECK(at(0, 0, 0, 0), "K15", "PgUp near the top clamps pointer and window at row 1");
    /* K17 21_pgup_a1 */
    pgup();
    CHECK(at(0, 0, 0, 0), "K17", "PgUp at A1 stays");
    /* K19 68_label_pgdn: typing, PgDn completes the entry and moves */
    tks("xyz"); pgdn();
    CHECK(g_mode == M_READY && label_is(0, 0, "'xyz") && at(0, T_ROWS, 0, T_ROWS), "K19", "PgDn stores 'xyz and pages");
    /* K20 p. 1-10: EDIT + PgUp completes editing and moves one screen up */
    f2(); pgup();
    CHECK(g_mode == M_READY && at(0, 0, 0, 0), "K20", "EDIT + PgUp completes and pages up");
}

/* ---- TAB / SHIFT-TAB ------------------------------------------------------- */
static void t_tab(void)
{
    /* K18 16/17/18: Tab right one screen, Shift-Tab back -- not one cell */
    tab();
    CHECK(at(T_COLS, 0, T_COLS, 0), "K18", "Tab from A1 lands on I1 with I first on screen");
    CHECK(starts(0, "I1: ") && m_scr[3][4 + 4] == 'I', "K18", "the screen shows I1 and column I first");
    tab();
    CHECK(at(2 * T_COLS, 0, 2 * T_COLS, 0), "K18", "Tab again: Q1");
    btab();
    CHECK(at(T_COLS, 0, T_COLS, 0), "K18", "Shift-Tab back: I1");
    /* 32_d1_tab: the pointer goes to the new screen's FIRST column */
    home(); right(); right(); right(); tab();
    CHECK(at(T_COLS, 0, T_COLS, 0), "K18", "Tab from D1: I1, not L1");
    /* 62/63: H1 with H first */
    home(); tab(); left();
    CHECK(at(T_COLS - 1, 0, T_COLS - 1, 0), "K18", "Tab, Left: H1 with H first");
    btab();
    CHECK(at(0, 0, 0, 0), "K18", "Shift-Tab from that window: A1 (62_h1_backtab)");
    tab(); left(); tab();
    CHECK(at(2 * T_COLS - 1, 0, 2 * T_COLS - 1, 0), "K18", "Tab from that window: P1 (63_h1_tab)");
    /* 36_k1_backtab */
    home(); go("k1"); btab();
    CHECK(at(10 - T_COLS, 0, 10 - T_COLS, 0), "K18", "Shift-Tab from K1 first: C1");
    /* 41_iv1_tab, 20_backtab_a1 */
    go("iv1"); tab();
    CHECK(at(255, 0, 255, 0), "K18", "Tab at IV stays");
    home(); btab();
    CHECK(at(0, 0, 0, 0), "K18", "Shift-Tab at A1 stays");
    /* K19 67_label_tab: typing, Tab completes the entry and moves */
    tks("abc"); tab();
    CHECK(g_mode == M_READY && label_is(0, 0, "'abc") && at(T_COLS, 0, T_COLS, 0), "K19", "Tab stores 'abc and moves a screen");
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    m_dir = argv[1];
    g_ctr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (g_ctr == MAP_FAILED) return 2;
    launch("EDIT", t_edit);
    launch("EDIT 240", t_edit240);
    launch("GOTO", t_goto);
    launch("PGDN/PGUP", t_page);
    launch("TAB", t_tab);
    printf("i123-keys: %d checks, %d failures\n", g_ctr[0], g_ctr[1]);
    return g_ctr[1] ? 1 : 0;
}
