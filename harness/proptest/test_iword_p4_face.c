/*
 * harness/proptest/test_iword_p4_face.c -- the HOST oracle of WORD.EXE's P4
 * screens (factory C; bead initech-8q6j, IWORD P4): the real face_a.c +
 * face_p4.c over the recording Toolbox mock of test_iword_face.c, driven with
 * the KEYS of the real WordPerfect 5.1 mint sessions; every expected row, menu
 * line and prompt is typed here from the shots of those sessions
 * (../wordperfect51-decomp mint/shots_wpMINT33..38, shots_wpPRINT0/1) and every
 * expected file is a file the real WP.EXE wrote or printed (mint/work/WP51/
 * MINT34, MINT36, MINT37, MINT38, PRTMINT34.TXT) -- never face_p4.c's tables.
 *
 * PARTS (usage: test_iword_p4_face <WORDPERFECT51_DECOMP> <empty-dir> <part>):
 *   hf      Shift-F8 2 Format: Page, 3 Headers, A, Every Page, the header
 *           screen, F7, 4 Footers ... Ctrl-B ... (mint/wpMINT34.steps): the
 *           screens; the saved file == MINT34 (the stated W-20 mask)
 *   notes   Ctrl-F7 1 1 / 2 1 (mint/wpMINT36.steps): menus, note screens,
 *           "Alpha1 beta2 gamma1."; the saved file == MINT36
 *   cols    Alt-F7 1 3 F7 1 ... Ctrl-Enter ... Alt-F7 1 2 (mint/wpMINT37.steps):
 *           menus, the Text Column Definition screen, "Col 2 ... Pos 5.9"";
 *           the saved file == MINT37
 *   table   Alt-F7 2 1 3 Enter 2 Enter F7 ... Tab ... (mint/wpMINT38.steps):
 *           prompts, the Table Edit screen, the grid, "Cell A1 ... Ln 1.14"
 *           Pos 1.12""; the saved file == MINT38
 *   print   MINT34's keys, then Shift-F7 S 3 2 8 PRINT.TXT Enter F7 F7 1
 *           (mint/wpPRINT0/1.steps): the Print, Select Printer, Edit and Port
 *           screens; PRINT.TXT == PRTMINT34.TXT byte for byte; then Port 1
 *           (LPT 1) and 1: the gate refuses the device -- "PC LOAD LETTER"
 *   <part>-serial <capture>   the emulator's TENANT-GATE draws hold the screens
 *
 * Mutants (Rule 6; test-iword-p4-face-mutant, each RED for its part):
 *   IWORD_MUT_HF_STATUS    the header screen's status line names no header
 *   IWORD_MUT_NOTE_NUMBER  the note screen shows no number
 *   IWORD_MUT_CELL_TAB     Tab in a table types a [Tab]
 *   IWORD_MUT_PRINT_NOFAIL a refused LPT1 print is silently ignored
 *   IWORD_MUT_COL_STATUS   no "Col n" on the status line
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#define main iword_p4_face_first_main
#include "test_iword_face.c"
#undef main

static const char *g_corpus;

static void reset_face(void)
{
    g_mode = M_EDIT; g_reveal = 0; g_current[0] = 0; g_home = 0;
    doc_reset(); paint_all();
}

static void ctrl_key(char c) { ev(keyDown, (uint8_t)c, (int32_t)FLAIR_EVT_MOD_CONTROL_KEY); }
static void tab_key(int32_t mods) { ev(keyDown, (0x0F << 8) | '\t', mods); }
#define SHIFT_F 1
#define ALT   ((int32_t)FLAIR_EVT_MOD_OPTION_KEY)
#define CTRL  ((int32_t)FLAIR_EVT_MOD_CONTROL_KEY)
#define HOME  0x47
#define DOWN  0x50

/* A status line: `left` at column 0, the group `grp` ending at column 66 and
 * "Pos <pos>" at 68 -- the whole right part moved left by what a long Pos
 * would overflow (the shots' layout as test_iword_face.c status_line_is
 * states it; slice 3 / tdnl.51). */
static int stat_is(int line, const char *left, const char *grp, const char *pos)
{
    char w[SCR_W + 1];
    size_t sh = 72 + strlen(pos) > SCR_W ? 72 + strlen(pos) - SCR_W : 0;
    memset(w, ' ', SCR_W); w[SCR_W] = 0;
    memcpy(w, left, strlen(left));
    memcpy(w + 67 - sh - strlen(grp), grp, strlen(grp));
    memcpy(w + 68 - sh, "Pos ", 4);
    memcpy(w + 72 - sh, pos, strlen(pos));
    return row_is(line, w);
}

static uint32_t slurp(const char *path, uint8_t *b, uint32_t cap)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f) return 0;
    n = fread(b, 1, cap, f);
    fclose(f);
    return (uint32_t)n;
}

/* The saved host file == the mint file; mask_x: the X u16 of every header /
 * footer definition but the first (W-20, stated in test_iword_p4.c). */
static int saved_equals(const char *name, const char *mint, int mask_x)
{
    static uint8_t a[65536], b[65536];
    char p1[600], p2[600];
    uint32_t na, nb, i, seen = 0;
    snprintf(p1, sizeof p1, "%s/%s", m_dir, name);
    snprintf(p2, sizeof p2, "%s/mint/work/WP51/%s", g_corpus, mint);
    na = slurp(p1, a, sizeof a);
    nb = slurp(p2, b, sizeof b);
    if (!na || na != nb) { fprintf(stderr, "    %s: %u bytes, %s: %u\n", name, na, mint, nb); return 0; }
    if (mask_x) {
        for (i = 327; i + 4u < na; ) {
            uint32_t L = iw_unit_len(a, na, i);
            if (!L) break;
            if (a[i] == 0xD5 && L > 22u && a[i + 2] != 0x12) {
                if (seen++) { a[i + 20] = b[i + 20] = 0; a[i + 21] = b[i + 21] = 0; }
            }
            i += L;
        }
    }
    for (i = 0; i < na; i++) if (a[i] != b[i]) { fprintf(stderr, "    %s != %s at 0x%X\n", name, mint, i); return 0; }
    return 1;
}

/* mint/wpMINT34.steps: Header A every page "Initech", Footer A every page
 * "Page ^B", "One.", Ctrl-Enter, "Two." */
static void keys_mint34(int check)
{
    fkey(F8, SHIFT); tk('2');
    if (check) {
        /* shots_wpMINT33/01_format_page.png */
        CHECK(row_is(0, "Format: Page") && row_is(2, "     1 - Center Page (top to bottom)    No") &&
              row_is(4, "     2 - Force Odd/Even Page") && row_is(6, "     3 - Headers") && row_is(8, "     4 - Footers") &&
              row_is(10, "     5 - Margins - Top                  1\"") &&
              row_is(11, "                   Bottom               1\"") && row_is(13, "     6 - Page Numbering") &&
              row_is(15, "     7 - Paper Size                     8.5\" x 11\"") &&
              row_is(16, "               Type                     Standard") && row_is(17, "               Labels") &&
              row_is(19, "     8 - Suppress (this page only)") && row_is(23, "Selection: 0"), "hf", "Format: Page (MINT33 01)");
    }
    tk('3');
    if (check) CHECK(row_is(23, "1 Header A; 2 Header B: 0"), "hf", "Headers: the A/B question (MINT33 02)");
    tk('1');
    if (check) CHECK(row_is(23, "1 Discontinue; 2 Every Page; 3 Odd Pages; 4 Even Pages; 5 Edit: 0"), "hf",
                     "Header A: the placement (MINT33 03)");
    tk('2');
    tks("Initech");
    if (check) {
        /* shots_wpMINT33/05_header_typed.png (MINT34 01 the same screen) */
        CHECK(row_is(0, "Initech") && stat_is(23, "Header A:  Press Exit when done", "Ln 1\"", "1.7\""),
              "hf", "the header screen: its text, \"Header A:  Press Exit when done\", Ln / Pos");
    }
    fkey(F7, 0);
    if (check) CHECK(row_is(6, "     3 - Headers                        HA Every page"), "hf", "back on Format: Page: HA Every page (MINT33 06)");
    tk('4'); tk('1'); tk('2');
    tks("Page ");
    ctrl_key('b');
    if (check) CHECK(row_is(0, "Page ^B"), "hf", "Ctrl-B shows ^B (MINT34 02_footer_typed)");
    fkey(F7, 0);
    if (check) CHECK(row_is(8, "     4 - Footers                        FA Every page"), "hf", "FA Every page (MINT34 03_format_page)");
    fkey(F7, 0);
    tks("One.");
    ev(keyDown, (0x1C << 8) | '\r', CTRL);
    tks("Two.");
}

static void hf_test(void)
{
    reset_face();
    keys_mint34(1);
    fkey(F10, 0); tks("hf\r");
    CHECK(saved_equals("HF", "MINT34", 1), "hf", "the saved file == MINT34 (W-20 X masked)");
    /* the second pair: B and Odd / Even (MINT33 09_format_page_after) */
    reset_face();
    fkey(F8, SHIFT); tk('2'); tk('3'); tk('1'); tk('2'); tks("A"); fkey(F7, 0);
    tk('3'); tk('2'); tk('4'); tks("B"); fkey(F7, 0);
    tk('4'); tk('2'); tk('3'); tks("C"); fkey(F7, 0);
    CHECK(row_is(6, "     3 - Headers                        HA Every page, HB Even pages") &&
          row_is(8, "     4 - Footers                        FB Odd pages"), "hf", "HA Every page, HB Even pages / FB Odd pages (MINT33 09)");
    /* Discontinue (MINT35 04_after_discontinue) */
    tk('3'); tk('1'); tk('1');
    CHECK(row_is(6, "     3 - Headers                        HB Even pages"), "hf", "1 Discontinue: Header A no longer listed (MINT35 04)");
    fkey(F7, 0);
    CHECK(g_mode == M_EDIT, "hf", "F7 back to the document");
}

static void notes_test(void)
{
    reset_face();
    tks("Alpha");
    fkey(F7, CTRL);
    CHECK(row_is(23, "1 Footnote; 2 Endnote; 3 Endnote Placement: 0"), "notes", "Ctrl-F7 (MINT36 01)");
    tk('1');
    CHECK(row_is(23, "Footnote: 1 Create; 2 Edit; 3 New Number; 4 Options: 0"), "notes", "1 Footnote (MINT36 02)");
    tk('1');
    tks("First note.");
    /* shots_wpMINT36/04_note_typed: "     1First note." (the Ln of WP's note
     * screen, 1.5", is not reproduced -- reduction stated in face_p4.c) */
    CHECK(row_is(0, "     1First note.") && !strncmp(m_scr[23], "Footnote:  Press Exit when done", 31) &&
          !strncmp(m_scr[23] + 68, "Pos 2.7\"", 8), "notes", "the note screen: number, text, status (MINT36 04)");
    fkey(F7, 0);
    CHECK(row_is(0, "Alpha1"), "notes", "the number in the text (MINT36 05_doc)");
    tks(" beta"); fkey(F7, CTRL); tk('1'); tk('1'); tks("Second note."); fkey(F7, 0);
    tks(" gamma"); fkey(F7, CTRL); tk('2');
    CHECK(row_is(23, "Endnote: 1 Create; 2 Edit; 3 New Number; 4 Options: 0"), "notes", "2 Endnote (MINT36 06)");
    tk('1');
    CHECK(row_is(0, "1.") && stat_is(23, "Endnote:  Press Exit when done", "Ln 1\"", "1.2\""),
          "notes", "the endnote screen (MINT36 07)");
    tks("End note."); fkey(F7, 0); tks(".");
    CHECK(row_is(0, "Alpha1 beta2 gamma1."), "notes", "Alpha1 beta2 gamma1. (MINT36 08_typed)");
    fkey(F10, 0); tks("notes\r");
    CHECK(saved_equals("NOTES", "MINT36", 0), "notes", "the saved file == MINT36");
}

static void cols_test(void)
{
    reset_face();
    fkey(F7, ALT);
    CHECK(row_is(23, "1 Columns; 2 Tables; 3 Math: 0"), "cols", "Alt-F7 (MINT37 01)");
    tk('1');
    CHECK(row_is(23, "Columns: 1 On; 2 Off; 3 Define: 0"), "cols", "1 Columns (MINT37 02)");
    tk('3');
    CHECK(row_is(0, "Text Column Definition") && row_is(2, "     1 - Type                                Newspaper") &&
          row_is(4, "     2 - Number of Columns                   2") && row_is(6, "     3 - Distance Between Columns") &&
          row_is(8, "     4 - Margins") &&
          row_is(10, "     Column    Left       Right      Column    Left       Right") &&
          row_is(11, "      1:       1\"         4\"         13:") && row_is(12, "      2:       4.5\"       7.5\"       14:") &&
          row_is(22, "     12:                             24:") && row_is(23, "Selection: 0"),
          "cols", "the Text Column Definition screen (MINT37 03)");
    fkey(F7, 0);
    CHECK(row_is(23, "Columns: 1 On; 2 Off; 3 Define: 0"), "cols", "F7: back to the Columns menu (MINT37 04)");
    tk('1');
    tks("The quick brown fox jumps over the lazy dog.");
    CHECK(row_is(0, "The quick brown fox jumps over") && row_is(1, "the lazy dog."), "cols",
          "the text wraps at the column (MINT37 06: 30 columns)");
    ev(keyDown, (0x1C << 8) | '\r', CTRL);
    tks("Second column.");
    CHECK(stat_is(23, "", "Col 2 Doc 1 Pg 1 Ln 1\"", "5.9\""), "cols", "Col 2 Doc 1 Pg 1 Ln 1\" Pos 5.9\" (MINT37 07)");
    fkey(F7, ALT); tk('1'); tk('2');
    tks("After.");
    CHECK(stat_is(23, "", "Doc 1 Pg 1 Ln 1.33\"", "1.6\""), "cols", "After.: below the longest column, Ln 1.33\" (MINT37 08)");
    fkey(F10, 0); tks("cols\r");
    CHECK(saved_equals("COLS", "MINT37", 0), "cols", "the saved file == MINT37");
}

/* The grid rows of a table of cell edges (screen columns) e[0..n], kind 0 top,
 * 1 between, 2 bottom, 3 the text row with texts t[] at e[i] + 1 -- the CP437
 * rules of shots_wpMINT38/07_after.png, typed here from the shot's lines. */
static void grid_row(char *w, const int *e, int n, int kind, const char *const *t)
{
    static const char L[4] = { (char)0xC9, (char)0xC7, (char)0xC8, (char)0xBA },
                      M[4] = { (char)0xD1, (char)0xC5, (char)0xCF, (char)0xB3 },
                      R[4] = { (char)0xBB, (char)0xB6, (char)0xBC, (char)0xBA },
                      H[4] = { (char)0xCD, (char)0xC4, (char)0xCD, ' ' };
    int i;
    memset(w, ' ', SCR_W); w[SCR_W] = 0;
    for (i = 0; i <= e[n]; i++) w[i] = H[kind];
    w[0] = L[kind];
    for (i = 1; i < n; i++) w[e[i]] = M[kind];
    w[e[n]] = R[kind];
    if (kind == 3) for (i = 0; i < n; i++) if (t[i]) memcpy(w + e[i] + 1, t[i], strlen(t[i]));
}

static void table_test(void)
{
    static const int E[4] = { 0, 21, 43, 64 };   /* 1200, 3800, 6400, 9000 WPU */
    static const char *const R1[3] = { "A1", "B1", "C1" }, *const R2[3] = { "A2", "B2", "C2" },
                             *const NONE3[3] = { 0, 0, 0 };
    char w[SCR_W + 1];
    reset_face();
    fkey(F7, ALT); tk('2');
    CHECK(row_is(23, "Table: 1 Create; 2 Edit: 0"), "table", "2 Tables (MINT38 01)");
    tk('1');
    CHECK(row_is(23, "Number of Columns: 3"), "table", "Number of Columns: 3 (MINT38 02)");
    tk('3'); tk('\r');
    CHECK(row_is(23, "Number of Rows: 1"), "table", "Number of Rows: 1 (MINT38 03)");
    tk('2'); tk('\r');
    /* shots_wpMINT38/04_table_edit.png */
    grid_row(w, E, 3, 0, 0); CHECK(row_is(0, w), "table", "Table Edit: the top rule");
    grid_row(w, E, 3, 3, NONE3); CHECK(row_is(1, w), "table", "Table Edit: row 1");
    grid_row(w, E, 3, 1, 0); CHECK(row_is(2, w), "table", "Table Edit: the rule between");
    grid_row(w, E, 3, 2, 0); CHECK(row_is(4, w), "table", "Table Edit: the bottom rule");
    {
        char d[SCR_W + 1];
        memset(d, (char)0xCD, SCR_W); d[SCR_W] = 0;
        CHECK(stat_is(20, "Table Edit:  Press Exit when done", "Cell A1 Doc 1 Pg 1 Ln 1.14\"", "1.12\"") &&
              row_is(21, d) && row_is(22, "Ctrl-Arrows Column Widths; Ins Insert; Del Delete; Move Move/Copy;") &&
              row_is(23, "1 Size; 2 Format; 3 Lines; 4 Header; 5 Math; 6 Options; 7 Join; 8 Split: 0"),
              "table", "the Table Edit status and menu lines (MINT38 04)");
    }
    fkey(F7, 0);
    CHECK(stat_is(23, "", "Cell A1 Doc 1 Pg 1 Ln 1.14\"", "1.12\""), "table", "F7: Cell A1 ... (MINT38 05)");
    tks("A1"); tab_key(0); tks("B1"); tab_key(0); tks("C1"); tab_key(0);
    tks("A2"); tab_key(0); tks("B2"); tab_key(0); tks("C2");
    fkey(HOME, 0); fkey(HOME, 0); fkey(DOWN, 0);
    tks("After.");
    /* shots_wpMINT38/07_after.png */
    grid_row(w, E, 3, 3, R1); CHECK(row_is(1, w), "table", "row 1: A1 B1 C1 between the rules (MINT38 07)");
    grid_row(w, E, 3, 3, R2); CHECK(row_is(3, w), "table", "row 2: A2 B2 C2 (MINT38 07)");
    CHECK(row_is(5, "After.") && stat_is(23, "", "Doc 1 Pg 1 Ln 1.63\"", "1.6\""), "table",
          "After. below the table: Ln 1.63\" (MINT38 07)");
    fkey(F10, 0); tks("tbl\r");
    CHECK(saved_equals("TBL", "MINT38", 0), "table", "the saved file == MINT38");
}

static void print_test(void)
{
    static uint8_t a[65536], b[65536];
    char p1[600], p2[600];
    uint32_t na, nb;
    reset_face();
    keys_mint34(0);
    fkey(F7, SHIFT);
    /* shots_wpPRINT0/07_after_f7b.png */
    CHECK(row_is(0, "Print") && row_is(2, "     1 - Full Document") && row_is(3, "     2 - Page") &&
          row_is(4, "     3 - Document on Disk") && row_is(5, "     4 - Control Printer") &&
          row_is(6, "     5 - Multiple Pages") && row_is(7, "     6 - View Document") &&
          row_is(8, "     7 - Initialize Printer") && row_is(11, "Options") &&
          row_is(13, "     S - Select Printer                 Standard Printer") &&
          row_is(14, "     B - Binding Offset                 0\"") &&
          row_is(15, "     N - Number of Copies               1") &&
          row_is(16, "     U - Multiple Copies Generated by   WordPerfect") &&
          row_is(17, "     G - Graphics Quality               Medium") &&
          row_is(18, "     T - Text Quality                   High") && row_is(23, "Selection: 0"),
          "print", "the Print screen (PRINT0 07)");
    tk('s');
    CHECK(row_is(0, "Print: Select Printer") && row_is(2, "* Standard Printer") &&
          row_is(23, "1 Select; 2 Additional Printers; 3 Edit; 4 Copy; 5 Delete; 6 Help; 7 Update: 1"),
          "print", "Select Printer (PRINT0 06)");
    tk('3');
    /* shots_wpPRINT1/01_port_menu.png (before the menu) */
    CHECK(row_is(0, "Select Printer: Edit") && row_is(2, "         Filename                       STANDARD.PRS") &&
          row_is(4, "     1 - Name                           Standard Printer") &&
          row_is(6, "     2 - Port                           LPT1:") &&
          row_is(8, "     3 - Sheet Feeder                   None") &&
          row_is(10, "     4 - Cartridges/Fonts/Print Wheels") &&
          row_is(12, "     5 - Initial Base Font              Courier 10 Pitch") &&
          row_is(14, "     6 - Path for Downloadable") && row_is(15, "           Fonts and Printer") &&
          row_is(16, "           Command Files") && row_is(18, "     7 - Print to Hardware Port         No"),
          "print", "Select Printer: Edit (PRINT1 01)");
    tk('2');
    CHECK(row_is(23, "Port: 1 LPT 1; 2 LPT 2; 3 LPT 3; 4 COM 1; 5 COM 2; 6 COM 3; 7 COM 4; 8 Other: 0"), "print", "the Port menu (PRINT1 01)");
    tk('8');
    tks("PRINT.TXT");
    CHECK(row_is(23, "Device or Filename: PRINT.TXT"), "print", "Device or Filename: PRINT.TXT (PRINT1 03)");
    tk('\r');
    CHECK(row_is(6, "     2 - Port                           PRINT.TXT") && row_is(18, "     7 - Print to Hardware Port"),
          "print", "the port is the file (PRINT1 04)");
    fkey(F7, 0); fkey(F7, 0);
    CHECK(row_is(0, "Print"), "print", "F7 F7: back to Print (PRINT1 06)");
    tk('1');
    CHECK(g_mode == M_EDIT, "print", "1 Full Document: back to the document (PRINT1 08)");
    snprintf(p1, sizeof p1, "%s/PRINT.TXT", m_dir);
    snprintf(p2, sizeof p2, "%s/mint/work/WP51/PRTMINT34.TXT", g_corpus);
    na = slurp(p1, a, sizeof a);
    nb = slurp(p2, b, sizeof b);
    CHECK(na && na == nb && !memcmp(a, b, na), "print", "PRINT.TXT == the real WP 5.1 print of MINT34 (PRTMINT34.TXT)");
    /* LPT 1: the device; the gate refuses it -- PC LOAD LETTER */
    m_created_device[0] = 0;
    fkey(F7, SHIFT); tk('s'); tk('3'); tk('2'); tk('1');
    CHECK(row_is(6, "     2 - Port                           LPT1:"), "print", "Port 1: LPT1:");
    fkey(F7, 0); fkey(F7, 0); tk('1');
    CHECK(!strcmp(m_created_device, "PRN") && row_is(23, "PC LOAD LETTER"), "print",
          "a print to LPT1 asks the gate for PRN, is refused, and says PC LOAD LETTER (bead acceptance)");
    tk(' ');
    CHECK(g_mode == M_EDIT && m_exit_calls == 0, "print", "any key: back to the document, still running");
}

/* ---- the emulator captures -------------------------------------------------------- */
static int serial_has(const char *path, const char *const *want, int n, unsigned *seen)
{
    FILE *f = fopen(path, "r");
    char line[1400];
    *seen = 0;
    if (!f) return 0;
    while (fgets(line, sizeof line, f))
        for (int i = 0; i < n; i++) if (strstr(line, want[i])) *seen |= 1u << i;
    fclose(f);
    return *seen == (1u << n) - 1u;
}

static void serial_check(const char *part, const char *path)
{
    static const char *const HF[] = {
        "y=0 s=\"Format: Page ",
        "y=368 s=\"1 Header A; 2 Header B: 0 ",
        "y=368 s=\"Header A:  Press Exit when done ",
        "y=96 s=\"     3 - Headers                        HA Every page ",
        "y=128 s=\"     4 - Footers                        FA Every page ",
        "y=0 s=\"Page ^B",
        "s=\"[Header A:Every page;Initech][Footer A:Every page;Page ^B]One.[HPg]",
    };
    static const char *const NOTES[] = {
        "y=368 s=\"1 Footnote; 2 Endnote; 3 Endnote Placement: 0 ",
        "y=368 s=\"Footnote: 1 Create; 2 Edit; 3 New Number; 4 Options: 0 ",
        "y=0 s=\"     1First note. ",
        "y=368 s=\"Endnote:  Press Exit when done ",
        "y=0 s=\"Alpha1 beta2 gamma1. ",
        "s=\"Alpha[Footnote:1;[Note Num]First note.] beta[Footnote:2;[Note Num]Second note.] gamma[Endno",
    };
    static const char *const COLS[] = {
        "y=368 s=\"1 Columns; 2 Tables; 3 Math: 0 ",
        "y=0 s=\"Text Column Definition ",
        "y=368 s=\"Columns: 1 On; 2 Off; 3 Define: 0 ",
        "Col 2 Doc 1 Pg 1 Ln 1\\\" Pos 5.9\\\"",
        "s=\"[Col Def:Newspaper;2;1\\\",4\\\";4.5\\\",7.5\\\"][Col On]The quick brown fox jumps o",
    };
    static const char *const TABLE[] = {
        "y=368 s=\"Table: 1 Create; 2 Edit: 0 ",
        "y=368 s=\"Number of Columns: 3",
        "y=368 s=\"Number of Rows: 1",
        "y=320 s=\"Table Edit:  Press Exit when done ",
        "Cell A1 Doc 1 Pg 1 Ln 1.14\\\" Pos 1.12\\\"",
        "s=\"[Row][Cell]A1[Cell]B1[Cell]C1",
    };
    static const char *const PRINT[] = {
        "y=0 s=\"Print ",
        "y=0 s=\"Print: Select Printer ",
        "y=0 s=\"Select Printer: Edit ",
        "y=368 s=\"Device or Filename: PRINT.TXT",
        "y=96 s=\"     2 - Port                           PRINT.TXT ",
        "TENANT-GATE ax=0x0080 path=\"PRINT.TXT\" -> ",
        "TENANT-GATE ax=0x0080 path=\"PRN\" -> -",
        "y=368 s=\"PC LOAD LETTER ",
    };
    unsigned seen = 0;
    int ok = 0, n = 0;
    const char *const *w = 0;
    if (!strcmp(part, "hf")) { w = HF; n = (int)(sizeof HF / sizeof HF[0]); }
    else if (!strcmp(part, "notes")) { w = NOTES; n = (int)(sizeof NOTES / sizeof NOTES[0]); }
    else if (!strcmp(part, "cols")) { w = COLS; n = (int)(sizeof COLS / sizeof COLS[0]); }
    else if (!strcmp(part, "table")) { w = TABLE; n = (int)(sizeof TABLE / sizeof TABLE[0]); }
    else if (!strcmp(part, "print")) { w = PRINT; n = (int)(sizeof PRINT / sizeof PRINT[0]); }
    if (w) ok = serial_has(path, w, n, &seen);
    if (!ok) for (int i = 0; i < n; i++) if (!(seen & (1u << i))) fprintf(stderr, "  missing draw: %s\n", w[i]);
    CHECK(ok, part, "the emulator drew every screen of the session");
}

int main(int argc, char **argv)
{
    static int ctr[2];
    g_ctr = ctr;
    if (argc != 4) { fprintf(stderr, "usage: %s <WORDPERFECT51_DECOMP> <dir|capture> <part>\n", argv[0]); return 2; }
    g_corpus = argv[1];
    if (strstr(argv[3], "-serial")) {
        char part[16];
        snprintf(part, sizeof part, "%.*s", (int)(strstr(argv[3], "-serial") - argv[3]), argv[3]);
        serial_check(part, argv[2]);
    } else {
        m_dir = argv[2];
        if (tbx_app_main()) return 2;
        if (!strcmp(argv[3], "hf")) hf_test();
        else if (!strcmp(argv[3], "notes")) notes_test();
        else if (!strcmp(argv[3], "cols")) cols_test();
        else if (!strcmp(argv[3], "table")) table_test();
        else if (!strcmp(argv[3], "print")) print_test();
        else return 2;
    }
    printf("test-iword-p4-face %s: %d checks, %d failures\n", argv[3], ctr[0], ctr[1]);
    if (ctr[1]) fprintf(stderr, "RED-REASON %s: the P4 screens / files differ from the real WP 5.1\n", argv[3]);
    return ctr[1] ? 1 : 0;
}
