/* Initech 123 entry boundary/state oracle, bead initech-twxf (B13).
 * Real face_a.c + worksheet core over the existing recording Toolbox mock.
 * Ref: Lotus 1-2-3 R2.2 Reference Manual printed pp. 1-16/1-17 (PDF 32/33):
 * 240-character entries, Enter stores, Esc cancels; p. 1-22: Backspace;
 * p. 1-23: Esc in EDIT erases the line first (B11, initech-69mz).
 * p. 1-4 places the current entry on control-panel line two.
 * No long-entry capture exists in the lotus123-decomp mint step files. The PM's
 * B13 brief specifies tail scrolling and beep/ignore on character 241;
 * the cited manual pages do not specify the scroll increment or overflow
 * feedback. These expectations are explicit product requirements, not a
 * fabricated Lotus golden. Width 76 is the native window's recording mock.
 * Expected bytes/lengths below never come from the implementation's limit.
 */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main

static unsigned entry_length;

static void pattern(char *s, unsigned n)
{
    for (unsigned i = 0; i < n; i++) s[i] = (char)('a' + i % 26);
    s[n] = 0;
}

static void expect_line(const char *s, unsigned n, const char *id)
{
    char want[77];
    unsigned visible = n > 75 ? 75 : n;
    memset(want, ' ', 76);
    memcpy(want, s + n - visible, visible);
    want[visible] = '_'; want[76] = 0;
    CHECK(!strcmp(m_scr[1], want), id, "edit line shows the entry tail and cursor");
}

static void expect_cell(const char *s, unsigned n, const char *id)
{
    const i123_cell_t *c = i123_find(&g_sheet, 0, 0);
    CHECK(c && c->kind == I123_K_LABEL && c->len == n + 1 &&
          g_sheet.pool[c->off] == '\'' && !memcmp(g_sheet.pool + c->off + 1, s, n),
          id, "commit stores every accepted character in A1");
    CHECK(g_mode == M_READY && g_dirty && g_elen == 0 && g_edit[0] == 0,
          id, "commit returns READY with unsaved work and no pending entry");
}

static void boundary(void)
{
    char typed[242];
    unsigned accepted = entry_length > 240 ? 240 : entry_length;
    pattern(typed, entry_length);
    CHECK(sizeof g_edit == 241, "E0", "entry buffer holds 240 characters plus NUL");
    tks(typed);
    CHECK(g_elen == accepted, "E1", "accept through 240, refuse character 241");
    CHECK(g_elen == accepted && !memcmp(g_edit, typed, accepted) && g_edit[g_elen] == 0,
          "E2", "accepted entry bytes and NUL remain intact");
    CHECK(g_mode == M_LABEL && !g_dirty && !i123_find(&g_sheet, 0, 0),
          "E3", "typing remains pending LABEL data");
    CHECK(m_beeps == (entry_length == 241 ? 1u : 0u), "E4", "only excess input beeps");
    expect_line(typed, accepted, "E5");
    tk('\r');
    expect_cell(typed, accepted, "E6");
}

static void recovery(void)
{
    char typed[241];
    pattern(typed, 240); tks(typed); tk('!'); tk('?');
    CHECK(m_beeps == 2 && g_elen == 240, "E7", "each overflow key is refused explicitly");
    tk(8); typed[239] = 0;
    CHECK(g_elen == 239 && g_edit[g_elen] == 0, "E7", "Backspace frees one position and terminates");
    expect_line(typed, 239, "E7");
    tk('Z'); typed[239] = 'Z'; typed[240] = 0;
    CHECK(g_elen == 240 && m_beeps == 2, "E7", "replacement after Backspace is accepted");
    expect_line(typed, 240, "E7"); tk('\r'); expect_cell(typed, 240, "E7");
    tks("old"); tk(27); tk('x');
    CHECK(g_elen == 1 && g_edit[1] == 0, "E8", "cancel and restart do not retain old suffix");
    expect_line("x", 1, "E8"); tk(27);
    expect_cell(typed, 240, "E8");
}

static void value_and_edit(void)
{
    char typed[241];
    pattern(typed, 240); typed[0] = '+'; tks(typed);
    CHECK(g_mode == M_VALUE && g_elen == 240, "E9", "VALUE uses the full entry capacity");
    expect_line(typed, 240, "E9"); tk('\r');
    CHECK(g_mode == M_EDIT && g_elen == 240, "E9", "invalid long formula stays intact in EDIT");
    tk('!'); CHECK(g_elen == 240 && m_beeps == 1, "E9", "EDIT refuses excess input too");
    expect_line(typed, 240, "E9"); tk(27);
    /* B11 (initech-69mz): in EDIT the first Esc erases the line and stays
     * in EDIT, the second returns to READY -- manual p. 1-23 ("ESC Erases
     * all characters in entry") and the real R2.2 after a bad value,
     * fixtures/i123-keys/50_bad_value.png -> 51_bad_value_esc.png. */
    CHECK(g_mode == M_EDIT && g_elen == 0 && !g_dirty && !i123_find(&g_sheet, 0, 0),
          "E9", "Esc erases the invalid entry, stays in EDIT, stores nothing");
    tk(27);
    CHECK(g_mode == M_READY && !g_dirty && !i123_find(&g_sheet, 0, 0),
          "E9", "a second Esc abandons the entry without storing it");
}

/* B07: Reference p. 1-19 (audit manual-pdf-035.txt). Currency is an
 * entry prefix; formatting is a separate operation, and $ in an absolute
 * reference must still reach the formula compiler. */
static void currency_entry(void)
{
    i123_val_t v;
    char out[32];
    uint32_t spill;
    tks("$12.34\r");
    i123_cell_value(&g_sheet, 0, 0, &v);
    CHECK(g_mode == M_READY && v.t == I123_V_NUM && v.n == 12.34,
          "E10", "leading dollar enters numeric 12.34");
    const i123_cell_t *c = i123_find(&g_sheet, 0, 0);
    CHECK(c && c->kind == I123_K_NUMBER && c->fmt == 0xff,
          "E10", "currency entry retains the default format");
    if (!c) return;
    i123_cell_text(&g_sheet, c, 16, out, &spill);
    CHECK(!strchr(out, '$'), "E10", "dollar is absent until currency format is selected");
    i123_set_cell_format(&g_sheet, 0, 0, 0xa2);
    i123_cell_text(&g_sheet, i123_find(&g_sheet, 0, 0), 16, out, &spill);
    CHECK(!strcmp(out, "         $12.34 "), "E10", "currency display is a separate choice");
    right(); tks("+$A$1\r");
    c = i123_find(&g_sheet, 1, 0);
    i123_cell_value(&g_sheet, 1, 0, &v);
    CHECK(c && c->kind == I123_K_FORMULA && v.t == I123_V_NUM && v.n == 12.34,
          "E11", "absolute reference remains a formula");
    right(); tks("12$34\r");
    CHECK(g_mode == M_EDIT && !i123_find(&g_sheet, 2, 0),
          "E11", "embedded dollar is refused without creating a cell");
    tk(27); tk(27); tks("/fsCURRENCY\r");
}

static void currency_grammar(void)
{
    /* Independently captured by real R2.2: fixtures/i123-numeric/NUMENTRY.WK1.
     * No currency grammar is added to the core formula/@VALUE parser. */
    static const struct { const char *s; int kind; double value; } cases[] = {
        {"$12.34", I123_K_NUMBER, 12.34}, {"-$12.34", 0, 0}, {"+$12.34", 0, 0},
        {"$-12.34", I123_K_NUMBER, -12.34}, {"$+12.34", I123_K_NUMBER, 12.34},
        {"$12.5%", I123_K_NUMBER, .125}, {"$1.25E2", I123_K_INTEGER, 125},
        {"$$12", 0, 0}, {"12$34", 0, 0}, {"$", 0, 0},
        {"$A$1", I123_K_FORMULA, 12.34}, {"+$A$1", I123_K_FORMULA, 12.34},
        {"$0", I123_K_INTEGER, 0}, {"$.5", I123_K_NUMBER, .5}
    };
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        i123_set_cursor(&g_sheet, 0, (uint16_t)i);
        tks(cases[i].s); tk('\r');
        const i123_cell_t *c = i123_find(&g_sheet, 0, (uint16_t)i);
        if (!cases[i].kind) {
            CHECK(g_mode == M_EDIT && !c, "E12", "invalid currency grammar is refused");
            tk(27); tk(27);
        } else {
            i123_val_t v; i123_cell_value(&g_sheet, 0, (uint16_t)i, &v);
            CHECK(g_mode == M_READY && c && c->kind == cases[i].kind &&
                  v.t == I123_V_NUM && v.n == cases[i].value, "E12", cases[i].s);
        }
    }
    double x;
    CHECK(i123_parse_value("$12.34", 6, &x) == I123_E_VALUE, "E12", "currency prefix belongs only to entry boundary");
    CHECK(i123_parse_value("-1.25E2", 7, &x) == I123_OK && x == -125, "E12", "existing signed exponent is preserved");
    CHECK(i123_parse_value("12.5%", 5, &x) == I123_OK && x == .125, "E12", "existing percent is preserved");
}

int main(int argc, char **argv)
{
    const unsigned lengths[] = {75, 104, 240, 241};
    if (argc != 2) return 2;
    m_dir = argv[1];
    g_ctr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (g_ctr == MAP_FAILED) return 2;
    for (unsigned i = 0; i < sizeof lengths / sizeof lengths[0]; i++) {
        entry_length = lengths[i]; launch("entry boundary", boundary);
    }
    launch("overflow recovery", recovery);
    launch("VALUE and EDIT", value_and_edit);
    launch("currency entry", currency_entry);
    launch("currency grammar", currency_grammar);
    printf("i123-entry: %d checks, %d failures\n", g_ctr[0], g_ctr[1]);
    return g_ctr[1] ? 1 : 0;
}
