/*
 * harness/proptest/test_iword_listfiles.c -- the HOST oracle of InitechWord's
 * List Files (F5) and Look (factory C; bead initech-qvgd, IWORD P3 remainder).
 *
 * WHAT IS UNDER TEST: the real os/iword/app/face_a.c over the real document
 * core and os/iword/core/listfiles.c, driven through tbx_app_event by the
 * recording Toolbox mock of test_iword_face.c, whose FILE_FIND / DISK_FREE /
 * CLOCK answer a FIXED directory: the List Files fixture volume of the real
 * WP 5.1 mint (../wordperfect51-decomp mint/work/LISTF/mkfixture.sh, v3),
 * typed into the table below by hand from that recipe (names, sizes,
 * attributes, stamps), its free bytes (1,313,280: mdir of lf005.img) and the
 * mint's DOS clock (03/01/1992 09:00, lf_root.conf). The files' bytes come
 * from argv[2], a host directory the Makefile fills from the same sources.
 *
 * WHERE THE EXPECTED SCREENS COME FROM (Law 2 -- never from face_a.c or
 * spec/iword/listfiles.h): argv[1], the mint's cell-exact transcriptions
 * grids/LF14_*.txt (decode2.py; md5-pinned by the Makefile).
 * THIS file maps WP's 80 x 25 onto the face's 76 x 24 with its OWN statement
 * of the adaptation rule (the face drops the list's last row and WP's blank
 * row 23; it deletes WP columns 59-60 of header row 1, 0/22/39/63 of a list
 * row, 46-48 of the F5 prompt, 50-53 of the Look header, 51 of the editing
 * screen's status line (face_a.c's tdnl.51 shift), and cuts every other
 * row at 76) and FIRST checks that every deleted / cut cell is blank on the
 * real screen -- so the comparison loses no WordPerfect cell. Colours: a red
 * WP cell (bg aa0000: band or selection bar) must be BLACK on GRAY; a bright
 * one (fg ffffff) bold; a List Files menu item other than 1 Retrieve and 6
 * Look (the brief's live pair) GRAY.
 *
 * usage: test_iword_listfiles <grids-dir> <host-files-dir>
 *        test_iword_listfiles <grids-dir> <host-files-dir> <serial> <free>
 *          the EMULATOR leg (test-flair-iword-listfiles): the TENANT-GATE
 *          DRAWCELLS / FILLRECT lines of the QEMU serial capture are replayed
 *          into the same recording mock, a snapshot taken as each keyDown is
 *          delivered (TENANT-EVT what=3), and every snapshot graded against
 *          the mint shot of the same key by the same comparisons. Two
 *          substitutions, both independent of the artifact: the Free figure
 *          is <free>, mdir's "bytes free" of the booted volume copy (the stock
 *          volume's APPS / DOS make it differ from the mint's); the clock is
 *          the mint's own 03-01-92 09:00a because the harness pins the guest
 *          RTC to 1992-03-01T09:00:00 (clock=vm) and kills the run at a 30 s
 *          wall-clock deadline (appl-boot --timeout-ms 30000), so the guest
 *          minute cannot leave 09:00.
 * Mutants (Rule 6): IWORD_MUT_LF_COLUMN (every right-aligned figure one
 * column right), IWORD_MUT_LF_ZERO_DATE (a zero DOS date printed as a stamp). ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#define main iword_p12_main
#include "test_iword_face.c"
#undef main

#define DD(y, m, d) ((uint16_t)((((y) - 1980) << 9) | ((m) << 5) | (d)))
#define TT(h, mi)   ((uint16_t)(((h) << 11) | ((mi) << 5)))
static const m_dirent_t k_dir[] = {
    /* the root (mkfixture.sh v3: the booted InitechOS data volume's root,
     * entry for entry, in its directory order; DESKTOP.DB and TRASH with
     * zero time and date words, as the kernel makes them) */
    { "", "README.TXT",   0x20, 0,       DD(1980, 1, 1), 169 },
    { "", "APPS",         0x10, TT(0, 0), DD(1980, 1, 1), 0 },
    { "", "DOS",          0x10, TT(0, 0), DD(1980, 1, 1), 0 },
    { "", "AUTOEXEC.BAT", 0x20, 0,       DD(1980, 1, 1), 24 },
    { "", "DOCS",         0x10, TT(8, 0), DD(1992, 2, 29), 0 },
    { "", "DESKTOP.DB",   0x22, 0,       0,              8 },
    { "", "TRASH",        0x12, 0,       0,              0 },
    /* DOCS, in its directory order; "." and ".." as DOS returns them */
    { "DOCS\\", ".",          0x10, TT(8, 0),   DD(1992, 2, 29), 0 },
    { "DOCS\\", "..",         0x10, TT(8, 0),   DD(1992, 2, 29), 0 },
    { "DOCS\\", "ARCHIVE",    0x10, TT(8, 10),  DD(1992, 2, 29), 0 },
    { "DOCS\\", "WORK",       0x10, TT(22, 15), DD(1992, 3, 1),  0 },
    { "DOCS\\", "MEMO",       0x20, TT(9, 5),   DD(1992, 3, 9),  495 },
    { "DOCS\\", "LETTER.WP5", 0x20, TT(23, 59), DD(1991, 12, 31), 483 },
    { "DOCS\\", "TABS.DOC",   0x20, TT(12, 0),  DD(1990, 10, 20), 627 },
    { "DOCS\\", "README.TXT", 0x20, TT(0, 30),  DD(1989, 7, 4),  16 },
    { "DOCS\\", "ZERO.TXT",   0x20, TT(0, 0),   DD(1980, 1, 1),  0 },
    { "DOCS\\", "BIG.DAT",    0x20, TT(13, 45), DD(1993, 1, 15), 12345 },
    { "DOCS\\", "HUGE.BIN",   0x20, TT(23, 58), DD(1999, 12, 31), 123456 },
    { "DOCS\\", "HIDDEN.SYS", 0x22, TT(1, 0),   DD(1990, 1, 1),  2 },
};

/* ---- one transcribed WP screen ---------------------------------------- */
typedef struct wp_screen {
    unsigned char t[25][80];
    unsigned int fg[25][80], bg[25][80];
} wp_screen_t;
static const char *g_grids;
static int g_emu;
static void patch_free(struct wp_screen *s);

static unsigned int hex6(const char *p) { return (unsigned int)strtoul(p, 0, 16); }
static int load(const char *shot, wp_screen_t *s)
{
    char path[512], line[4096];
    FILE *f;
    int rows = 0;
    snprintf(path, sizeof path, "%s/%s.txt", g_grids, shot);
    f = fopen(path, "r");
    if (!f) { fprintf(stderr, "  cannot read %s\n", path); return 0; }
    memset(s, 0, sizeof *s);
    while (fgets(line, sizeof line, f)) {
        int r;
        if (line[0] >= '0' && line[0] <= '9' && line[2] == '|') {
            const char *p = line + 3;
            int c = 0;
            r = (line[0] - '0') * 10 + (line[1] - '0');
            while (c < 80 && *p) {
                if (p[0] == '\\' && p[1] == 'x') { s->t[r][c++] = (unsigned char)strtoul(p + 2, 0, 16) & 0xFFu; p += 4; }
                else s->t[r][c++] = (unsigned char)*p++;
            }
            rows++;
        } else if (line[0] >= '0' && line[0] <= '9' && line[2] == ' ') {
            char *p = line + 3;
            r = (line[0] - '0') * 10 + (line[1] - '0');
            while (*p && *p != '\n') {
                int a = (int)strtol(p, &p, 10), b;
                p++;                                   /* '-' */
                b = (int)strtol(p, &p, 10);
                p++;                                   /* ':' */
                unsigned int fg = hex6(p), bg = hex6(p + 7);
                p += 13;
                for (int c = a; c <= b && c < 80; c++) { s->fg[r][c] = fg; s->bg[r][c] = bg; }
                while (*p == ' ') p++;
            }
        }
    }
    fclose(f);
    if (g_emu && !memcmp(s->t[1] + 26, "Free:", 5)) patch_free(s);
    return rows == 25;
}

/* ---- the adaptation, stated here (see the header) ---------------------- */
enum { K_PLAIN, K_HDR1, K_LIST, K_PROMPT, K_LOOKHDR, K_STATUS };
static int deleted(int kind, int c)
{
    switch (kind) {
    case K_HDR1:    return c == 59 || c == 60;
    case K_LIST:    return c == 0 || c == 22 || c == 39 || c == 63;
    case K_PROMPT:  return c >= 46 && c <= 48;
    case K_LOOKHDR: return c >= 50 && c <= 53;
    /* the editing screen's status line: face_a.c's stated tdnl.51 shift of
     * the "Doc .. Pos" group one cell left (the blank before "Doc") */
    case K_STATUS:  return c == 51;
    default:        return 0;
    }
}
/* WP row `w` of `s` fitted: out[76] text, fo/bo colours, and whether every
 * dropped cell was blank */
static int fit(const wp_screen_t *s, int w, int kind, unsigned char *out, unsigned int *fo, unsigned int *bo)
{
    int n = 0, lossless = 1;
    for (int c = 0; c < 80; c++) {
        if (deleted(kind, c) || n >= SCR_W) { if (s->t[w][c] != ' ') lossless = 0; continue; }
        out[n] = s->t[w][c]; fo[n] = s->fg[w][c]; bo[n] = s->bg[w][c]; n++;
    }
    while (n < SCR_W) { out[n] = ' '; fo[n] = bo[n] = 0x0000aa; n++; }
    return lossless;
}

/* The face's row `f` against WP row `w`: text cell for cell; with colours
 * (`attrs`), red -> BLACK on GRAY, bright -> bold; on a menu row (`menu`) the
 * items other than 1 and 6 GRAY. */
static void cmp_row(const char *id, const wp_screen_t *s, int f, int w, int kind, int attrs, int menu)
{
    unsigned char t[SCR_W];
    unsigned int fo[SCR_W], bo[SCR_W];
    char gray[SCR_W];
    char msg[160];
    int ok = 1, bad = -1;
    CHECK(fit(s, w, kind, t, fo, bo), id, "the face's cut / deleted WP cells are blank on the real screen");
    for (int c = 0; c < SCR_W; c++) if ((unsigned char)m_scr[f][c] != t[c]) { ok = 0; if (bad < 0) bad = c; }
    snprintf(msg, sizeof msg, "row %d == WP row %d cell for cell", f, w);
    if (!ok) fprintf(stderr, "  %s face row %d: |%.76s|\n  %s mint row %d: |%.76s| (first diff col %d)\n",
                     id, f, m_scr[f], id, w, (const char *)t, bad);
    CHECK(ok, id, msg);
    if (!attrs) return;
    memset(gray, 0, sizeof gray);
    if (menu) {
        /* an item: a bright digit or N followed by a blank, to its ';' / ':' */
        for (int c = 0; c + 1 < SCR_W; c++) {
            int e;
            if (fo[c] != 0xffffffu || t[c + 1] != ' ' || !((t[c] >= '1' && t[c] <= '9') || t[c] == 'N')) continue;
            for (e = c + 2; e < SCR_W && t[e] != ';' && t[e] != ':'; e++) {}
            if (e >= SCR_W) continue;
            if (t[c] != '1' && t[c] != '6') for (int k = c; k <= e; k++) gray[k] = 1;
        }
    }
    ok = 1; bad = -1;
    for (int c = 0; c < SCR_W; c++) {
        int want_col, want_bold;
        if (bo[c] == 0xaa0000u) { want_col = 0x12; want_bold = 0; }
        else { want_col = gray[c] ? 0x23 : 0x03; want_bold = fo[c] == 0xffffffu && !gray[c]; }
        if (m_inv[f][c]) continue;                      /* the cursor cell */
        if (m_col[f][c] != want_col || (!gray[c] && t[c] != ' ' && (m_face[f][c] & 1) != want_bold)) {
            ok = 0; if (bad < 0) bad = c;
        }
    }
    snprintf(msg, sizeof msg, "row %d colours: red -> BLACK on GRAY, bright -> bold, unperformed items GRAY", f);
    if (!ok) fprintf(stderr, "  %s row %d col %d: col 0x%02x face %d (mint fg %06x bg %06x)\n", id, f, bad,
                     m_col[f][bad], m_face[f][bad], fo[bad], bo[bad]);
    CHECK(ok, id, msg);
}

/* the List Files screen (WP rows 0..21 -> face 0..21; 23, 24 -> 22, 23) */
static void cmp_list(const char *shot)
{
    wp_screen_t s;
    if (!load(shot, &s)) { CHECK(0, shot, "mint grid readable"); return; }
    for (int w = 0; w < 25; w++) {
        int f = w <= 21 ? w : w == 22 ? -1 : w - 1;
        if (f < 0) { unsigned char t[SCR_W]; unsigned int a[SCR_W], b[SCR_W];
                     (void)fit(&s, 22, K_LIST, t, a, b);
                     CHECK(t[0] == ' ' && !memcmp(t, t + 1, SCR_W - 1), shot, "the dropped list row 22 is blank on the real screen");
                     continue; }
        cmp_row(shot, &s, f, w, w == 1 ? K_HDR1 : (w >= 3 && w <= 22) ? K_LIST : K_PLAIN,
                1, w >= 23);
    }
}

/* a document screen with a status / prompt row: WP rows 0..22 -> face 0..22,
 * WP row 24 -> face 23 (WP's row 23 is the editing area's 24th row) */
static void cmp_doc(const char *shot, int prompt_kind, int status_attrs)
{
    wp_screen_t s;
    if (!load(shot, &s)) { CHECK(0, shot, "mint grid readable"); return; }
    for (int w = 0; w <= 22; w++) cmp_row(shot, &s, w, w, K_PLAIN, 0, 0);
    if (prompt_kind >= 0) cmp_row(shot, &s, 23, 24, prompt_kind, status_attrs, 0);
}

static void cmp_look(const char *shot)
{
    wp_screen_t s;
    if (!load(shot, &s)) { CHECK(0, shot, "mint grid readable"); return; }
    cmp_row(shot, &s, 0, 0, K_LOOKHDR, 1, 0);
    for (int w = 1; w <= 22; w++) cmp_row(shot, &s, w, w, K_PLAIN, 0, 0);
    cmp_row(shot, &s, 23, 24, K_PLAIN, 1, 0);
}

#define DOWN  0x50
#define UP    0x48
#define LEFT  0x4B
#define RIGHT 0x4D
#define ENTER 0x1C
#define F5    0x3F

/* LF14 (lf_root.conf on fixture v3, A:\; the keys of LF13): every key of the mint session, the shot taken
 * after it, and how the screen is graded: 'p' the document + the F5 prompt,
 * 'l' the List Files screen, 'k' Look, 'd' the document + its status line. */
static const struct { char key; const char *shot; char kind; } k_lf14[] = {
    { 'F', "LF14_01_after_f5", 'p' },      { 'E', "LF14_02_after_return", 'l' },
    { 'D', "LF14_03_after_down", 'l' },    { 'R', "LF14_04_after_right", 'l' },
    { 'E', "LF14_05_after_return", 'l' },  { 'E', "LF14_06_after_return", 'l' },
    { 'D', "LF14_07_after_down", 'l' },    { 'D', "LF14_08_after_down", 'l' },
    { 'D', "LF14_09_after_down", 'l' },    { '6', "LF14_10_after_6", 'k' },
    { 'X', "LF14_11_after_escape", 'l' },  { '1', "LF14_12_after_1", 'd' },
    { 'F', "LF14_13_after_f5", 'p' },      { 'E', "LF14_14_after_return", 'l' },
    { 'D', "LF14_15_after_down", 'l' },    { 'R', "LF14_16_after_right", 'l' },
    { 'E', "LF14_17_after_return", 'l' },  { 'E', "LF14_18_after_return", 'l' },
    { 'D', "LF14_19_after_down", 'l' },    { 'D', "LF14_20_after_down", 'l' },
    { 'D', "LF14_21_after_down", 'l' },    { '1', "LF14_22_after_1", 'l' },
    { 'n', "LF14_23_after_n", 'l' },       { '1', "LF14_24_after_1", 'l' },
    { 'y', "LF14_25_after_y", 'd' },
};
#define LF14_KEYS ((int)(sizeof k_lf14 / sizeof k_lf14[0]))
static void press(char k)
{
    switch (k) {
    case 'F': fkey(F5, 0); break;
    case 'E': fkey(ENTER, 0); break;
    case 'D': fkey(DOWN, 0); break;
    case 'R': fkey(RIGHT, 0); break;
    case 'X': fkey(ESC, 0); break;
    default:  tk(k); break;
    }
}
static void grade(int i)
{
    const char *shot = k_lf14[i].shot;
    switch (k_lf14[i].kind) {
    case 'p': cmp_doc(shot, K_PROMPT, 1);
              CHECK(m_inv[23][4], shot, "the cursor on the path's first cell (mint cursor (24,4))"); break;
    case 'l': cmp_list(shot); break;
    case 'k': cmp_look(shot); break;
    default:  cmp_doc(shot, K_STATUS, 0); break;
    }
}

static void listfiles_test(void)
{
    m_dirents = k_dir; m_ndirents = (int)(sizeof k_dir / sizeof k_dir[0]);
    m_free = 1313280;                    /* mdir of lf005.img v3 */
    for (int i = 0; i < LF14_KEYS; i++) {
        press(k_lf14[i].key);
        grade(i);
        if (i == 22) {
            /* host-only, on the list after n (each must leave LF14_23): Esc
             * does nothing (LF03_16), Space leaves Look (LF04_07), the gray
             * items' keys are ignored */
            fkey(ESC, 0);            cmp_list("LF14_23_after_n");
            tk('6'); tk(' ');        cmp_list("LF14_23_after_n");
            tk('2'); tk('3'); tk('N'); tk('9'); cmp_list("LF14_23_after_n");
        }
    }
    CHECK(g_mode == M_EDIT && g_ed.dirty, "LF14_25", "y retrieved into the document (now modified)");
    printf("listfiles: %d mint screens compared\n", LF14_KEYS + 3);
}

/* ---- the emulator leg ---------------------------------------------------- */
typedef struct snap {
    char scr[SCR_H][SCR_W + 1], inv[SCR_H][SCR_W];
    unsigned char face[SCR_H][SCR_W], col[SCR_H][SCR_W];
} snap_t;
#define MAX_SNAP 32
static snap_t g_snap[MAX_SNAP];
static void take(int k)
{
    if (k < 0 || k >= MAX_SNAP) return;
    memcpy(g_snap[k].scr, m_scr, sizeof m_scr); memcpy(g_snap[k].inv, m_inv, sizeof m_inv);
    memcpy(g_snap[k].face, m_face, sizeof m_face); memcpy(g_snap[k].col, m_col, sizeof m_col);
}
static void restore(int k)
{
    memcpy(m_scr, g_snap[k].scr, sizeof m_scr); memcpy(m_inv, g_snap[k].inv, sizeof m_inv);
    memcpy(m_face, g_snap[k].face, sizeof m_face); memcpy(m_col, g_snap[k].col, sizeof m_col);
}
static uint32_t g_emu_free;
/* the mint shot with its Free figure replaced by the emulator volume's */
static void patch_free(wp_screen_t *s)
{
    char num[16];
    uint32_t v = g_emu_free;
    int n = 0, k = 0, c;
    char t[16];
    do { if (k == 3) { t[n++] = ','; k = 0; } t[n++] = (char)('0' + v % 10u); v /= 10u; k++; } while (v);
    for (c = 0; c < n; c++) num[c] = t[n - 1 - c];
    for (c = 31; c <= 41; c++) s->t[1][c] = ' ';           /* "Free:" is 26..30 */
    for (c = 0; c < n; c++) s->t[1][42 - n + c] = (unsigned char)num[c];
}
#define EMU_KEYS LF14_KEYS
static void listfiles_serial(const char *path)
{
    static char line[8192];
    FILE *f = fopen(path, "r");
    int keys = 0, up = 0;
    CHECK(f != 0, "listfiles-serial", "serial capture exists");
    if (!f) return;
    for (int i = 0; i < SCR_H; i++) { memset(m_scr[i], '?', SCR_W); m_scr[i][SCR_W] = 0; }
    while (fgets(line, sizeof line, f)) {
        unsigned x, y, fg, bg;
        char *q, *e;
        if (strstr(line, "TENANT-REGISTER ok app=WORD")) up = 1;
        if (!up) continue;
        if (!strncmp(line, "TENANT-EVT what=3", 17)) { take(keys); keys++; continue; }
        if (!strncmp(line, "TENANT-GATE ax=0x0031 win=1 ", 28)) { (void)tbx_fill_rect(1, 0, 0, 0, 0, 0); continue; }
        if (strncmp(line, "TENANT-GATE ax=0x0033 win=1 x=", 30)) continue;
        if (sscanf(line + 30, "%u y=%u", &x, &y) != 2 || !(q = strstr(line, " s=\"")) || !(e = strstr(q, "\" fg=")) ||
            sscanf(e, "\" fg=%u bg=%u", &fg, &bg) != 2) { CHECK(0, "listfiles-serial", "a DRAWCELLS trace line parses"); continue; }
        *e = 0;
        (void)tbx_draw_cells(1, (int32_t)x, (int32_t)y, q + 4, fg, bg);
    }
    fclose(f);
    take(keys);
    CHECK(up, "listfiles-serial", "WORD registered in the emulator");
    if (keys != EMU_KEYS) fprintf(stderr, "  %d keyDown events delivered to WORD, the locked trace has %d\n", keys, EMU_KEYS);
    CHECK(keys == EMU_KEYS, "listfiles-serial", "every key of the locked trace reached WORD");
    if (keys != EMU_KEYS) return;
    for (int i = 0; i < LF14_KEYS; i++) {
        restore(i + 1);                    /* the screen after key i + 1 */
        grade(i);
    }
    printf("listfiles-serial: %d keys, %d mint screens compared\n", keys, LF14_KEYS);
}

int main(int argc, char **argv)
{
    static int counters[2];
    g_ctr = counters;
    if (argc != 3 && argc != 5) return 2;
    g_grids = argv[1];
    m_dir = argv[2];
    if (argc == 5) {
        g_emu = 1;
        g_emu_free = (uint32_t)strtoul(argv[4], 0, 10);
        listfiles_serial(argv[3]);
        printf("test-iword-listfiles (emulator): %d checks, %d failures\n", counters[0], counters[1]);
        if (counters[1]) fprintf(stderr, "RED-REASON listfiles-serial: the emulator's List Files screens vs the real WP 5.1 (mint 005)\n");
        return counters[1] ? 1 : 0;
    }
    if (tbx_app_main()) return 2;
    paint_all();
    listfiles_test();
    printf("test-iword-listfiles: %d checks, %d failures\n", counters[0], counters[1]);
    if (counters[1]) fprintf(stderr, "RED-REASON listfiles: the real WP 5.1 List Files screens (mint 005)\n");
    return counters[1] ? 1 : 0;
}
