/* Independent WP 5.1 keyboard oracle. Ref: Reference printed pp. 963-966,
 * Template pp. 5-6; Search pp. 571-573. Names transcribed from the Reference,
 * NOT from spec/iword/fkeys.json. Uses the existing recording Toolbox mock.
 * initech-qvgd. Factory C only. */
#define main iword_p12_main
#include "test_iword_face.c"
#undef main

static const char *const names[4][12] = {
    { "Cancel", "Search", "Help", "Indent", "List", "Bold", "Exit", "Underline", "End Field", "Save", "Reveal Codes", "Block" },
    { "Setup", "Search", "Switch", "Indent", "Date/Outline", "Center", "Print", "Format", "Merge Codes", "Retrieve", "", "" },
    { "Thesaurus", "Replace", "Reveal Codes", "Block", "Mark Text", "Flush Right", "Columns/Table", "Style", "Graphics", "Macro", "", "" },
    { "Shell", "Spell", "Screen", "Move", "Text In/Out", "Tab Align", "Footnote", "Font", "Merge/Sort", "Macro Define", "", "" }
};
static const int32_t modifiers[4] = { 0, FLAIR_EVT_MOD_SHIFT_KEY, FLAIR_EVT_MOD_OPTION_KEY, FLAIR_EVT_MOD_CONTROL_KEY };
static void reset_face(void)
{
    g_mode = M_EDIT; g_reveal = 0; g_current[0] = 0; g_home = 0;
    doc_reset(); paint_all();
}
static int vk_at(int k) { return k < 10 ? 0x3B + k : 0x57 + k - 10; }
static void template_test(void)
{
    int dispatched = 0;
    for (int m = 0; m < 4; m++) for (int k = 0; k < 12; k++) {
        /* RE-KEYED 2026-10-17 (bead initech-8q6j, IWORD P4, stated): Shift-F7
         * Print, Alt-F7 Columns/Table and Ctrl-F7 Footnote are live (their
         * screens: ../wordperfect51-decomp shots_wpPRINT0/01_print_menu,
         * shots_wpMINT37/01_coltab_menu, shots_wpMINT36/01_footnote_menu) */
        int live = (m == 0 && (k == 0 || k == 1 || k == 3 || k == 5 || k == 6 || k == 7 || k >= 9)) ||
                   (m == 1 && (k == 0 || k == 1 || k == 6 || k == 7 || k == 9)) ||
                   (m == 2 && (k == 1 || k == 2 || k == 3 || k == 6)) ||
                   (m == 3 && (k == 3 || k == 6 || k == 7));
        reset_face();
        CHECK(!strcmp(names[m][k], IW_FKEYS[m][k].name), "template", "all command names equal the independent Reference transcription");
        fkey(vk_at(k), modifiers[m] | FLAIR_EVT_MOD_ACTIVE_FLAG);
        if (!live) {
            int unmapped = names[m][k][0] == 0;
            CHECK(unmapped ? status_is("", "1\"", "1\"", 0) : row_is(23, names[m][k]), "template", "manual name or unchanged unmapped status");
            CHECK(m_col[23][0] == (unmapped ? 0x03 : 0x23), "template", "gray command or unchanged unmapped colors");
            CHECK(g_ed.doc.len == 0 && !m_exit_calls, "template", "grayed key leaves document and tenant alive");
        } else {
            /* Real action effects, independently specified. No test-only dispatch counter. */
            int ok = 0;
            if (m == 0 && k == 0) ok = row_is(23, "Undelete 1 Restore; 2 Previous Deletion: 0");
            if (m == 1 && k == 0) ok = row_is(0, "Setup");
            if (m == 2 && k == 1) ok = row_is(23, "w/Confirm? No (Yes)");
            if (m == 0 && k == 3) ok = g_ed.doc.len == 11;
            if (m == 0 && k == 5) ok = g_ed.doc.len == 6;
            if (m == 0 && k == 7) ok = g_ed.doc.len == 6;
            if (m == 0 && k == 6) ok = g_mode == M_SAVEQ;
            if (m == 0 && k == 9) ok = g_mode == M_SAVE_NAME;
            if ((m == 1 && k == 9)) ok = g_mode == M_GET_NAME;
            if ((m == 0 && k == 10) || (m == 2 && k == 2)) ok = g_reveal != 0;
            if ((m == 0 && k == 11) || (m == 2 && k == 3)) ok = g_ed.blk == 0;
            if (m == 3 && k == 3) ok = g_mode == M_MOVENB;
            if (m == 3 && k == 7) ok = g_mode == M_FONT;
            if (m == 1 && k == 7) ok = g_mode == M_FORMAT;
            if (m == 1 && k == 6) ok = row_is(0, "Print") && row_is(2, "     1 - Full Document");
            if (m == 2 && k == 6) ok = row_is(23, "1 Columns; 2 Tables; 3 Math: 0");
            if (m == 3 && k == 6) ok = row_is(23, "1 Footnote; 2 Endnote; 3 Endnote Placement: 0");
            if (k == 1 && m < 2) ok = strncmp(m_scr[23], m ? "<- Srch:" : "-> Srch:", 8) == 0;
            CHECK(ok, "template", "functional key performs its action");
        }
        dispatched++;
    }
    printf("template: %d/48 key slots driven through the real event dispatcher\n", dispatched);
}

/* Gray names must have been drawn by the real tenant, in gray. */
static void serial_template(const char *path)
{
    char line[1200];
    int seen[4][12] = {{0}}, up = 0;
    FILE *f = fopen(path, "r");
    CHECK(f != 0, "template", "serial capture exists");
    if (!f) return;
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "TENANT-REGISTER ok app=WORD")) up = 1;
        if (!strstr(line, "TENANT-GATE ax=0x0033 win=1 x=1 y=368 s=")) continue;
        for (int m = 0; m < 4; m++) for (int k = 0; k < 12; k++) {
            char want[100];
            snprintf(want, sizeof want, "s=\"%-76s\" fg=2 bg=3", names[m][k]);
            if (strstr(line, want)) seen[m][k] = 1;
        }
    }
    fclose(f);
    CHECK(up, "template", "WORD registered in emulator");
    CHECK(!seen[1][10], "template", "unmapped keys do not clear the status line");
    /* (P4, 2026-10-17: the three F7 chords are live, not gray) */
    const unsigned gray[4] = { (1u<<2)|(1u<<4)|(1u<<8),
        (1u<<2)|(1u<<3)|(1u<<4)|(1u<<5)|(1u<<8),
        0x3FFu ^ ((1u<<1)|(1u<<2)|(1u<<3)|(1u<<6)), 0x3FFu ^ ((1u<<3)|(1u<<6)|(1u<<7)) };
    for (int m = 0; m < 4; m++) for (int k = 0; k < 12; k++) if (gray[m] & (1u<<k)) {
        if (!seen[m][k]) fprintf(stderr, "missing gray key row %d F%d '%s'\n", m, k+1, names[m][k]);
        CHECK(seen[m][k], "template", "emulator drew the grayed manual name");
    }
}

/* Reference pp. 571-573. Inputs use mint-proven P2 operations; every
 * search must preserve document bytes. Expectations are independent. */
static void search_test(void)
{
    uint8_t before[512], after[512];
    uint32_t n;
    reset_face(); tks("Jim jim JIM");
    n = iw_doc_read(&g_ed.doc, 0, before, sizeof before);
    g_ed.cur = 0;
    fkey(0x3C, 0);
    CHECK(row_is(23, "-> Srch:"), "search", "forward prompt");
    tks("jim"); fkey(0x3C, 0);
    CHECK(g_ed.cur == 3, "search", "lower-case query finds capitalized Jim");
    fkey(0x3C, 0); fkey(0x3C, 0);
    CHECK(g_ed.cur == 7, "search", "F2 twice continues");
    fkey(0x3C, SHIFT); tks("Jim"); fkey(0x3C, 0);
    CHECK(g_ed.cur == 0, "search", "backward capital query excludes lower-case jim");
    fkey(0x3C, 0); tks("missing"); fkey(0x3C, 0);
    CHECK(row_is(23, "* Not found *"), "search", "not-found message");
    CHECK(g_ed.cur == 0, "search", "not-found keeps cursor");
    fkey(F1, 0); g_ed.cur = 4; fkey(0x3C, 0); tks("JIM"); fkey(F1, 0);
    CHECK(g_ed.cur == 4 && g_mode == M_EDIT, "search", "Cancel preserves cursor");
    CHECK(iw_doc_read(&g_ed.doc, 0, after, sizeof after) == n && !memcmp(before, after, n), "search", "search and Cancel preserve all document bytes");
    reset_face(); tks("plain "); fkey(F6, 0); tks("bold"); fkey(F6, 0); tks(" end");
    n = iw_doc_read(&g_ed.doc, 0, before, sizeof before);
    g_ed.cur = 0; fkey(0x3C, 0); fkey(F6, 0);
    CHECK(row_is(23, "-> Srch:[BOLD]"), "search", "F6 inserts code token");
    CHECK(m_inv[23][14], "search", "query cursor follows the complete code label");
    fkey(0x3C, 0);
    CHECK(g_ed.cur == 9, "search", "search finds whole Bold On unit");
    fkey(0x3C, 0); fkey(0x3C, 0);
    CHECK(row_is(23, "* Not found *") && g_ed.cur == 9, "search", "Bold Off cannot match a Bold On query");
    fkey(F1, 0);
    g_ed.cur = 0; fkey(0x3C, 0); fkey(F6, 0); fkey(F6, 0); fkey(0x4B, 0); tk(8);
    CHECK(row_is(23, "-> Srch:[bold]"), "search", "Left/Backspace remove on token to search for off token");
    fkey(0x3C, 0);
    CHECK(g_ed.cur == 16, "search", "search finds whole Bold Off unit");
    CHECK(iw_doc_read(&g_ed.doc, 0, after, sizeof after) == n && !memcmp(before, after, n), "search", "code search preserves document bytes");
    reset_face(); tks("one two one"); g_ed.cur = 0;
    fkey(0x3C, SHIFT); fkey(0x50, 0); tks("two"); fkey(ESC, 0);
    CHECK(g_ed.cur == 7, "search", "Down changes to forward; Esc executes");
    fkey(0x3C, 0); fkey(0x48, 0); tks("one"); fkey(0x3C, SHIFT);
    CHECK(g_ed.cur == 0, "search", "Up changes to backward; completion key keeps direction");
}

static void serial_search(const char *path)
{
    char line[1200];
    int forward = 0, backward = 0, missing = 0, pendf = 0, pendb = 0;
    FILE *f = fopen(path, "r");
    CHECK(f != 0, "search", "serial capture exists");
    if (!f) return;
    while (fgets(line, sizeof line, f)) {
        if (!strstr(line, "ax=0x0033 win=1 x=1 y=368 s=")) continue;
        if (strstr(line, "s=\"-> Srch:[BOLD]")) pendf = 1;
        else if (pendf) {
            forward = strstr(line, "Doc 1 Pg 1 Ln 1\" Pos 1.6\"") != 0;
            pendf = 0;
        }
        if (strstr(line, "s=\"<- Srch:plain")) pendb = 1;
        else if (pendb) {
            backward = strstr(line, "Doc 1 Pg 1 Ln 1\" Pos 1\"") != 0;
            pendb = 0;
        }
        if (strstr(line, "s=\"* Not found * ")) missing = 1;
    }
    fclose(f);
    CHECK(forward, "search", "emulator forward code match moves cursor to Pos 1.6");
    CHECK(backward, "search", "emulator backward text match moves cursor to Pos 1");
    CHECK(missing, "search", "emulator draws exact not-found message");
}

int main(int argc, char **argv)
{
    static int counters[2];
    g_ctr = counters;
    if (argc < 2) return 2;
    if (tbx_app_main()) return 2;
    if (!strcmp(argv[1], "fkeys")) template_test();
    else if (!strcmp(argv[1], "search")) search_test();
    else if (!strcmp(argv[1], "search-serial") && argc == 3) serial_search(argv[2]);
    else if (!strcmp(argv[1], "fkeys-serial") && argc == 3) serial_template(argv[2]);
    else return 2;
    printf("test-iword-%s: %d checks, %d failures\n", argv[1], counters[0], counters[1]);
    if (counters[1]) fprintf(stderr, "RED-REASON %s: Reference keyboard/screen contract\n", argv[1]);
    return counters[1] ? 1 : 0;
}
