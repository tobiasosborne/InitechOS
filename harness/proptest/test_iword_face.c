/*
 * harness/proptest/test_iword_face.c -- the HOST oracle of WORD.EXE, the
 * InitechWord Face A application (factory C; bead initech-l4tj, IWORD P1).
 *
 * WHAT IS UNDER TEST. The real os/iword/app/face_a.c, compiled on the host
 * over the real document core, with the Toolbox Gate replaced by a
 * RECORDING MOCK (the test_i123_quit.c pattern, bead initech-fvnj): the
 * cells the app draws land in a 77 x 24 screen with their colours, EXIT is
 * recorded, the file verbs are served from a host directory. Each LAUNCH is
 * a fork()ed child -- a pristine image, as a real launch loads -- and a
 * relaunch reads what an earlier launch saved.
 *
 * WHERE THE EXPECTED VALUES COME FROM (Law 1 / Law 2 -- never from
 * face_a.c), restated here by hand:
 *   - the screen: ../wordperfect51-decomp/goldens/minted/MINT0x.shots (an
 *     80-column screen; the status line "Doc 1 Pg 1 Ln 1" Pos 1"" starts at
 *     column 52 in 01_edit.png, "Doc 1 Pg 1 Ln 1.17" Pos 2.6"" at column 49
 *     in MINT02 02_typed.png, i.e. the Ln group ends at column 67 and "Pos"
 *     is at column 69; the saved name at the left, "C:\WP51\MINT01" in
 *     04_saved.png);
 *   - the wrap: MINT01.WP5 @0x188 (65 characters, then the space is the SRt);
 *   - the prompts and messages: the WordPerfect 5.1 Reference Manual
 *     (initech-manuals-ref/wordperfect51/WPDOS51Reference.pdf): "Document to
 *     be Saved:" (PDF 213), "Document to be Retrieved:" (PDF 137), "Save
 *     Document? Yes (No)" and "Exit WP? No (Yes)" (PDF 38), "(Text was not
 *     modified)" (PDF 211), "Replace <name>? No (Yes)" (PDF 399), "ERROR:
 *     File Not Found" (PDF 572), "Incompatible File Format" (PDF 767), Cancel
 *     F1 backs out of the Save / Exit prompts (PDF 211), "Pos" -> "POS" with
 *     Caps Lock (PDF 74);
 *   - the exit guard: F7 always asks (PDF 211); the Macintosh close box and
 *     File > Exit end a clean document at once (the initech-fvnj rule).
 * The saved bytes are judged INDEPENDENTLY by the Makefile with ../wordperfect
 * 51-decomp/tools/wp5_ref.py and the real MINT02.WP5 prefix.
 *
 * usage: test_iword_face <empty-scratch-dir>
 *        (built with -DIWORD_SMALL_BUFFERS: the compaction / size-limit leg)
 * F7 (bead initech-jojk, P2 slice 1) drives the attribute keys and Reveal
 * Codes; its expected strings are typed here from the real WP 5.1 screens
 * (../wordperfect51-decomp mint/shots_wpMINT06/01_font_menu.png,
 * 01_appearance_menu.png, 03_reveal.png; goldens/minted/MINT0x.shots/
 * 03_reveal.png) and its expected codes from the Reference Manual
 * (Attributes p. 36-37) and MINT06/MINT08 -- never from spec/iword/
 * reveal_codes.h or face_a.c. The saved CODES file is judged by the
 * Makefile with wp5_ref.py.
 *
 * Mutants (Rule 6; test-iword-face-mutant): IWORD_MUT_NO_WRAP (F1 rows),
 * IWORD_MUT_SAVE_NOTHING (F1 saved size), IWORD_MUT_NO_FACE (F7 faces).
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include "../../os/iword/app/face_a.c"   /* the application under test */

static volatile int *g_ctr;            /* [0] checks, [1] failures */
#define CHECK(cond, id, msg) do { g_ctr[0]++; if (!(cond)) { g_ctr[1]++; \
    fprintf(stderr, "  FAIL %s %s (line %d)\n", (id), (msg), __LINE__); } } while (0)

/* ---- the recording mock ------------------------------------------------- */
#define SCR_W 77
#define SCR_H 24
static char m_scr[SCR_H][SCR_W + 1];
static char m_inv[SCR_H][SCR_W];        /* drawn NAVY on WHITE (the cursor) */
static unsigned char m_face[SCR_H][SCR_W];   /* the txFace of the last plain-
                                         * colour draw of the cell (fg >> 8) */
static unsigned char m_col[SCR_H][SCR_W];    /* (fg token << 4) | bg token  */
static int  m_exit_calls, m_exit_rc;
static const char *m_dir;
static FILE *m_fh[8];

int32_t tbx_register(void) { return TBX_OK; }
int32_t tbx_new_window(int32_t l, int32_t t, int32_t r, int32_t b, int32_t g)
{ (void)l; (void)t; (void)r; (void)b; return g == (int32_t)TBX_GOAWAY_ASK ? 1 : TBX_ERR_BADARG; }
int32_t tbx_set_wtitle(int32_t w, const char *s) { (void)w; (void)s; return TBX_OK; }
int32_t tbx_draw_text(int32_t w, int32_t x, int32_t y, const char *s, uint32_t f, uint32_t b)
{ (void)w; (void)x; (void)y; (void)s; (void)f; (void)b; return TBX_OK; }
int32_t tbx_fill_rect(int32_t w, int32_t l, int32_t t, int32_t r, int32_t b, uint32_t c)
{
    (void)w; (void)l; (void)t; (void)r; (void)b; (void)c;
    for (int i = 0; i < SCR_H; i++) { memset(m_scr[i], '?', SCR_W); memset(m_inv[i], 0, SCR_W); }
    return TBX_OK;
}
int32_t tbx_set_mbar(const tbx_menubar_t *b) { (void)b; return TBX_OK; }
int32_t tbx_draw_menubar(void) { return TBX_OK; }
int32_t tbx_alert(const char *t, const char *d, const char *c, const char *o)
{ (void)t; (void)d; (void)c; (void)o; return TBX_ERR_BADCODE; }
int32_t tbx_trap(uint32_t c, uint32_t n, const uint32_t *a) { (void)c; (void)n; (void)a; return TBX_ERR_BADCODE; }

int32_t tbx_draw_cells(int32_t w, int32_t x, int32_t y, const char *s, uint32_t f, uint32_t b)
{
    int col = (x - 1) / 8, line = y / 16;
    (void)w;
    if (line < 0 || line >= SCR_H || col < 0 || (x - 1) % 8 || y % 16) return TBX_ERR_BADARG;
    if ((f & 0xFFu) >= TBX_COLOR_COUNT || (f >> 16) || ((f >> 8) & ~0x07u) || b >= TBX_COLOR_COUNT)
        return TBX_ERR_BADARG;                 /* spec/toolbox_gate.h Sec 7a STYLE */
    for (int i = 0; s[i] && col + i < SCR_W; i++) {
        int inv = (f == TBX_COLOR_NAVY && b == TBX_COLOR_WHITE);
        m_scr[line][col + i] = s[i];
        m_inv[line][col + i] = (char)inv;
        if (!inv) { m_face[line][col + i] = (unsigned char)(f >> 8); m_col[line][col + i] = (unsigned char)(((f & 0xFFu) << 4) | b); }
    }
    return TBX_OK;
}

int32_t tbx_exit(int32_t rc) { m_exit_calls++; m_exit_rc = rc; return TBX_OK; }

static void host_name(const char *path, char *out)
{
    char *o;
    snprintf(out, 300, "%s/", m_dir);
    o = out + strlen(out);
    for (const char *p = path; *p; p++) *o++ = *p;   /* the app hands UPPER names */
    *o = 0;
}
static int32_t new_handle(FILE *f)
{
    for (int i = 0; i < 8; i++) if (!m_fh[i]) { m_fh[i] = f; return 5 + i; }
    fclose(f); return TBX_ERR_DOS(4);
}
static FILE *fh(int32_t h) { return (h >= 5 && h < 13) ? m_fh[h - 5] : 0; }
int32_t tbx_file_create(const char *path)
{ char hp[300]; FILE *f; host_name(path, hp); f = fopen(hp, "w+b"); return f ? new_handle(f) : TBX_ERR_DOS(5); }
int32_t tbx_file_open(const char *path, uint32_t mode)
{ char hp[300]; FILE *f; (void)mode; host_name(path, hp); f = fopen(hp, "r+b"); return f ? new_handle(f) : TBX_ERR_DOS(2); }
int32_t tbx_file_read(int32_t h, void *b, uint32_t n)
{ FILE *f = fh(h); return f ? (int32_t)fread(b, 1, n, f) : TBX_ERR_BADARG; }
int32_t tbx_file_write(int32_t h, const void *b, uint32_t n)
{ FILE *f = fh(h); return f ? (int32_t)fwrite(b, 1, n, f) : TBX_ERR_BADARG; }
int32_t tbx_file_seek(int32_t h, int32_t d, uint32_t o)
{
    FILE *f = fh(h);
    if (!f || fseek(f, d, o == TBX_FILE_FROM_END ? SEEK_END : o == TBX_FILE_FROM_HERE ? SEEK_CUR : SEEK_SET)) return TBX_ERR_BADARG;
    return (int32_t)ftell(f);
}
int32_t tbx_file_close(int32_t h)
{ FILE *f = fh(h); if (!f) return TBX_ERR_BADARG; fclose(f); m_fh[h - 5] = 0; return TBX_OK; }
int32_t tbx_file_delete(const char *path)
{ char hp[300]; host_name(path, hp); return remove(hp) == 0 ? TBX_OK : TBX_ERR_DOS(2); }

/* ---- driving the application ------------------------------------------- */
#define SHIFT ((int32_t)FLAIR_EVT_MOD_SHIFT_KEY)
static void ev(int32_t what, int32_t msg, int32_t mods)
{
    tbx_event_t e;
    memset(&e, 0, sizeof e);
    e.what = what; e.message = msg; e.modifiers = mods;
    tbx_app_event(&e);
}
static void tk(char c)
{
    if (c == '\r') ev(keyDown, (0x1C << 8) | '\r', 0);
    else if (c == 8) ev(keyDown, (0x0E << 8) | 8, 0);
    else ev(keyDown, (uint8_t)c, 0);
}
static void tks(const char *s) { while (*s) tk(*s++); }
static void fkey(int vk, int32_t mods) { ev(keyDown, vk << 8, mods); }
#define F1   0x3B
#define F3   0x3D
#define F6   0x40
#define F8   0x42
#define F7   0x41
#define F10  0x44
#define ESC  0x01
static __attribute__((unused)) void close_box(void) { ev(TBX_EVT_GOAWAY, 1, 0); }
static __attribute__((unused)) void menu(int item) { ev(TBX_EVT_MENU, TBX_MENU_WORD(129, item), 0); }

/* m_scr[line] == want padded with blanks to 77 columns */
static int row_is(int line, const char *want)
{
    char w[SCR_W + 1];
    size_t n = strlen(want);
    memset(w, ' ', SCR_W); w[SCR_W] = 0;
    memcpy(w, want, n > SCR_W ? SCR_W : n);
    if (strcmp(m_scr[line], w) == 0) return 1;
    fprintf(stderr, "    row %d is   [%s]\n    row %d want [%s]\n", line, m_scr[line], line, w);
    return 0;
}
/* The status line: `left` at column 0, then the Doc group ending at column
 * 67 and "Pos" at column 69 (the MINT shots). */
static __attribute__((unused)) int status_line_is(int line, const char *left, const char *ln, const char *pos, int caps)
{
    char w[SCR_W + 1], doc[40];
    memset(w, ' ', SCR_W); w[SCR_W] = 0;
    memcpy(w, left, strlen(left));
    snprintf(doc, sizeof doc, "Doc 1 Pg 1 Ln %s", ln);
    memcpy(w + 68 - strlen(doc), doc, strlen(doc));
    memcpy(w + 69, caps ? "POS " : "Pos ", 4);
    memcpy(w + 73, pos, strlen(pos));
    return row_is(line, w);
}
static __attribute__((unused)) int status_is(const char *left, const char *ln, const char *pos, int caps)
{
    return status_line_is(23, left, ln, pos, caps);
}
/* exactly one reverse-video cell, at (line, col) */
static __attribute__((unused)) int cursor_at(int line, int col)
{
    int n = 0, ok = 0;
    for (int i = 0; i < 23; i++) for (int j = 0; j < SCR_W; j++) if (m_inv[i][j]) { n++; if (i == line && j == col) ok = 1; }
    if (n == 1 && ok) return 1;
    fprintf(stderr, "    cursor: %d reverse cells; want one at (%d,%d)\n", n, line, col);
    return 0;
}
static long file_size(const char *name)
{ char hp[300]; FILE *f; long n; host_name(name, hp); f = fopen(hp, "rb"); if (!f) return -1; fseek(f, 0, SEEK_END); n = ftell(f); fclose(f); return n; }

static void launch(const char *name, void (*script)(void))
{
    pid_t p;
    fflush(stdout); fflush(stderr);
    p = fork();
    if (p < 0) { perror("fork"); exit(2); }
    if (p == 0) {
        for (int i = 0; i < SCR_H; i++) { memset(m_scr[i], ' ', SCR_W); m_scr[i][SCR_W] = 0; }
        if (tbx_app_main() != 0) { CHECK(0, "F0", "tbx_app_main failed"); _exit(0); }
        ev(updateEvt, 0, 0);
        script();
        _exit(0);
    }
    int st = 0;
    waitpid(p, &st, 0);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) { g_ctr[0]++; g_ctr[1]++; fprintf(stderr, "  FAIL %s: the launch died (status 0x%x)\n", name, st); }
}

#ifndef IWORD_SMALL_BUFFERS
#define PARA "Initech word processing is here. The quick brown fox jumps over the lazy dog."
#define ROW0 "Initech word processing is here. The quick brown fox jumps over"
#define ROW1 "the lazy dog."

/* F1 THE GATE'S FLOW (what test-flair-iword drives in the emulator): type a
 * paragraph that wraps, F10 save, F7 n n (a new document), Shift-F10
 * retrieve -- the text comes back. */
static void t_flow(void)
{
    CHECK(status_is("", "1\"", "1\"", 0), "F1", "empty document: Doc 1 Pg 1 Ln 1\" Pos 1\" (01_edit.png)");
    CHECK(cursor_at(0, 0), "F1", "the cursor at the top left");
    tks(PARA);
    CHECK(row_is(0, ROW0) && row_is(1, ROW1) && row_is(2, ""), "F1", "the paragraph wraps after \"over\" (65 columns)");
    CHECK(status_is("", "1.17\"", "2.3\"", 0), "F1", "line 2, column 13: Ln 1.17\" Pos 2.3\"");
    CHECK(cursor_at(1, 13), "F1", "the cursor after \"dog.\"");
    fkey(F10, 0);
    CHECK(row_is(23, "Document to be Saved: "), "F1", "F10 asks \"Document to be Saved: \" (PDF 213)");
    tks("memo");
    CHECK(row_is(23, "Document to be Saved: memo"), "F1", "the typed name shows");
    tk('\r');
    CHECK(status_is("A:\\MEMO", "1.17\"", "2.3\"", 0), "F1", "saved: the name at the left of the status line (04_saved.png)");
    CHECK(file_size("MEMO") == 327 + 77, "F1", "MEMO holds the 327-byte prefix + the 77-byte body");
    fkey(F7, 0);
    CHECK(row_is(23, "Save Document? Yes (No)                               (Text was not modified)"), "F1",
          "F7 asks, with (Text was not modified) at the right end (PDF 211)");
    tk('n');
    CHECK(row_is(23, "Exit WP? No (Yes)"), "F1", "n: \"Exit WP? No (Yes)\" (PDF 38)");
    tk('n');
    CHECK(m_exit_calls == 0, "F1", "Exit WP? n does not end InitechWord");
    CHECK(row_is(0, "") && row_is(1, "") && status_is("", "1\"", "1\"", 0), "F1", "n: a clear screen, a new document");
    fkey(F10, SHIFT);
    CHECK(row_is(23, "Document to be Retrieved: "), "F1", "Shift-F10 asks \"Document to be Retrieved: \" (PDF 137)");
    tks("memo\r");
    CHECK(row_is(0, ROW0) && row_is(1, ROW1), "F1", "the retrieved text shows again, wrapped where WP wraps");
    CHECK(status_is("A:\\MEMO", "1\"", "1\"", 0), "F1", "retrieved onto a clear screen: its name, the cursor at the top");
    CHECK(cursor_at(0, 0), "F1", "the cursor at the top of the retrieved document");
    CHECK(m_exit_calls == 0, "F1", "still running");
}

/* F2 RELAUNCH: MEMO is on disk; retrieve it in a fresh launch, keep editing. */
static void t_relaunch(void)
{
    fkey(F10, SHIFT);
    tks("MEMO\r");
    CHECK(row_is(0, ROW0) && row_is(1, ROW1), "F2", "a fresh launch retrieves MEMO");
    /* End, Down, End: the cursor walks the lines */
    fkey(0x4F, 0);
    CHECK(cursor_at(0, 63) && status_is("A:\\MEMO", "1\"", "7.3\"", 0), "F2", "End: the end of line 1 (column 63)");
    fkey(0x50, 0);
    CHECK(cursor_at(1, 13), "F2", "Down from column 63 lands at the end of the short line 2");
    /* F10 offers the current name; save as COPY (a character replaces it) */
    fkey(F10, 0);
    CHECK(row_is(23, "Document to be Saved: MEMO"), "F2", "F10 offers the current name (PDF 583)");
    tks("copy\r");
    CHECK(status_is("A:\\COPY", "1.17\"", "2.3\"", 0), "F2", "saved as COPY");
    /* Home, Up: to the very start; type X; F10 Enter: Replace? */
    fkey(0x48, 0);
    fkey(0x47, 0);
    tk('X');
    CHECK(row_is(0, "X" ROW0), "F2", "X typed at the start (the line still fits: 64 columns)");
    fkey(F10, 0);
    tk('\r');
    CHECK(row_is(23, "Replace A:\\COPY? No (Yes)"), "F2", "an existing file asks Replace? No (Yes) (PDF 399/583)");
    tk('n');
    CHECK(row_is(23, "Document to be Saved: COPY"), "F2", "n: prompted for a filename again (PDF 583)");
    tk('\r');
    tk('y');
    CHECK(status_is("A:\\COPY", "1\"", "1.1\"", 0), "F2", "y: replaced; back to the document");
}

/* F3 THE EXIT GUARD. */
static void t_clean_close(void)
{
    close_box();
    CHECK(m_exit_calls == 1 && m_exit_rc == 0, "F3", "a clean document: the close box ends at once");
}
static void t_clean_menu(void)
{
    menu(3);
    CHECK(m_exit_calls == 1 && m_exit_rc == 0, "F3", "a clean document: File > Exit (Ctrl-Q) ends at once");
}
static void t_clean_f7(void)
{
    fkey(F7, 0);
    CHECK(m_exit_calls == 0 && row_is(23, "Save Document? Yes (No)                               (Text was not modified)"),
          "F3", "F7 always asks (PDF 211)");
    tk('n');
    tk('y');
    CHECK(m_exit_calls == 1 && m_exit_rc == 0, "F3", "n then Exit WP? y ends InitechWord");
}
static void t_dirty_close_cancel(void)
{
    tks("Unsaved work");
    close_box();
    CHECK(m_exit_calls == 0 && row_is(23, "Save Document? Yes (No)"), "F3", "dirty: the close box asks, no (Text was not modified)");
    fkey(F1, 0);
    CHECK(m_exit_calls == 0 && row_is(0, "Unsaved work") && status_is("", "1\"", "2.2\"", 0), "F3", "F1 Cancel keeps the document (PDF 211)");
    menu(3);
    CHECK(row_is(23, "Save Document? Yes (No)"), "F3", "dirty: File > Exit asks");
    fkey(ESC, 0);
    CHECK(m_exit_calls == 0 && row_is(0, "Unsaved work"), "F3", "Esc cancels too");
    fkey(F7, 0);
    tk('n');
    fkey(F1, 0);
    CHECK(m_exit_calls == 0 && row_is(0, "Unsaved work"), "F3", "F1 at Exit WP? keeps the document");
}
static void t_dirty_save_exit(void)
{
    tks("Keep me");
    menu(3);
    tk('y');
    CHECK(row_is(23, "Document to be Saved: "), "F3", "Save Document? y: the save prompt");
    tks("guard\r");
    CHECK(m_exit_calls == 0 && row_is(23, "Exit WP? No (Yes)"), "F3", "saved, then Exit WP? No (Yes)");
    CHECK(file_size("GUARD") == 327 + 7, "F3", "GUARD saved before the exit question");
    tk('y');
    CHECK(m_exit_calls == 1 && m_exit_rc == 0, "F3", "y ends InitechWord");
}
static void t_dirty_enter_default(void)
{
    tks("Default");
    fkey(F7, 0);
    tk('\r');
    CHECK(row_is(23, "Document to be Saved: "), "F3", "Enter = Yes, the option not in parentheses (PDF 38)");
    fkey(F1, 0);
    CHECK(m_exit_calls == 0 && row_is(0, "Default") && status_is("", "1\"", "1.7\"", 0), "F3", "F1 at the save prompt returns to the document");
}

/* F4 ERRORS and Caps Lock. */
static void t_errors(void)
{
    FILE *f;
    char hp[300];
    fkey(F10, SHIFT);
    tks("nofile\r");
    CHECK(row_is(23, "ERROR: File Not Found"), "F4", "a missing file: ERROR: File Not Found (PDF 572)");
    tk(' ');
    CHECK(row_is(23, "Document to be Retrieved: nofile"), "F4", "then the filename is redisplayed (PDF 572)");
    fkey(F1, 0);
    host_name("JUNK", hp);
    f = fopen(hp, "wb"); fputs("not a WordPerfect document\r\n", f); fclose(f);
    fkey(F10, SHIFT);
    tks("junk\r");
    CHECK(row_is(23, "ERROR: Incompatible File Format"), "F4", "not a 5.1 document: Incompatible File Format (PDF 767)");
    fkey(F1, 0);
    fkey(F1, 0);
    fkey(F10, 0);
    tks("a.b.c\r");
    CHECK(row_is(23, "ERROR: Invalid file name"), "F4", "a name that is not 8.3 is refused");
    fkey(F1, 0);
    fkey(F1, 0);
    ev(keyDown, 'a', (int32_t)FLAIR_EVT_MOD_ALPHA_LOCK);
    CHECK(status_is("", "1\"", "1.1\"", 1), "F4", "Caps Lock: POS (PDF 74)");
    CHECK(m_exit_calls == 0, "F4", "errors never end InitechWord");
}

/* F5 RETRIEVE INTO A DOCUMENT: at the cursor (PDF 571). */
static void t_into(void)
{
    tks("Hi ");
    fkey(F10, SHIFT);
    tks("memo\r");
    CHECK(row_is(0, "Hi Initech word processing is here. The quick brown fox jumps") &&
          row_is(1, "over the lazy dog."), "F5", "the file's text inserted at the cursor, re-wrapped");
    CHECK(status_is("", "1\"", "1.3\"", 0), "F5", "the on-screen document keeps no name of its own; cursor stays");
}


/* F7 CODES (bead initech-jojk): F6 Bold, F8 Underline, Ctrl-F8 2 4 Italic,
 * Ctrl-F8 3 Normal; Alt-F3 Reveal Codes; Backspace on a code in Reveal
 * Codes takes its partner; the document saves with its codes. */
#define CTRL ((int32_t)FLAIR_EVT_MOD_CONTROL_KEY)
#define ALT  ((int32_t)FLAIR_EVT_MOD_OPTION_KEY)
/* faces of row `line` cols [from, to) all == f */
static int faces_are(int line, int from, int to, int f)
{
    for (int c = from; c < to; c++)
        if (m_face[line][c] != f) { fprintf(stderr, "    row %d col %d face %d, want %d\n", line, c, m_face[line][c], f); return 0; }
    return 1;
}
/* the reverse-video run of row `line`: [col, col + n) and nothing else */
static int inv_run(int line, int col, int n)
{
    for (int c = 0; c < SCR_W; c++) {
        int want = c >= col && c < col + n;
        if ((m_inv[line][c] != 0) != want) { fprintf(stderr, "    row %d col %d reverse %d, want %d\n", line, c, m_inv[line][c], want); return 0; }
    }
    return 1;
}
static void t_codes(void)
{
    tks("Plain ");
    fkey(F6, 0);                         /* [BOLD][bold], the cursor between */
    tks("bold");
    fkey(F6, 0);                         /* past [bold] */
    tk(' ');
    fkey(F8, 0);
    tks("under");
    fkey(F8, 0);
    tk(' ');
    fkey(F8, CTRL);
    CHECK(row_is(23, "1 Size; 2 Appearance; 3 Normal; 4 Base Font; 5 Print Color: 0"), "F7",
          "Ctrl-F8: the Font menu on the status line (01_font_menu.png)");
    tk('2');
    CHECK(row_is(23, "1 Bold 2 Undln 3 Dbl Und 4 Italc 5 Outln 6 Shadw 7 Sm Cap 8 Redln 9 Stkout: 0"), "F7",
          "2: the Appearance menu (01_appearance_menu.png)");
    tk('4');
    tks("slant");
    fkey(F8, CTRL); tk('2'); tk('4');    /* re-selecting turns it off (p. 36) */
    CHECK(row_is(0, "Plain bold under slant"), "F7", "the codes take no column");
    CHECK(faces_are(0, 0, 6, 0) && faces_are(0, 6, 10, 1) && faces_are(0, 10, 11, 0) &&
          faces_are(0, 11, 16, 4) && faces_are(0, 16, 17, 0) && faces_are(0, 17, 22, 2), "F7",
          "faces: bold 1 on \"bold\", underline 4 on \"under\", italic 2 on \"slant\", plain between");
    CHECK(status_is("", "1\"", "3.2\"", 0), "F7", "Pos counts text only: column 22 -> Pos 3.2\"");
    /* Ctrl-F8 3 Normal: F6 x, Normal, y -> x bold, y plain */
    tk(' ');
    fkey(F6, 0); tk('x');
    fkey(F8, CTRL); tk('3');
    tk('y');
    CHECK(row_is(0, "Plain bold under slant xy") && faces_are(0, 23, 24, 1) && faces_are(0, 24, 25, 0), "F7",
          "Ctrl-F8 3 Normal steps past [bold]: x bold, y plain");
    fkey(F10, 0);
    tks("codes\r");
    CHECK(file_size("CODES") == 327 + 49, "F7", "CODES: the prefix + 25 characters + 8 codes (24 bytes)");
    {
        char hp[300];
        unsigned char b[400];
        FILE *f;
        size_t k;
        host_name("CODES", hp);
        f = fopen(hp, "rb");
        k = f ? fread(b, 1, sizeof b, f) : 0;
        if (f) fclose(f);
        CHECK(k == 376u && b[0x103] == 0x08, "F7", "an italic body: 0x08 at 0x103, as WP wrote MINT06");
    }
    /* Alt-F3: Reveal Codes (MINT0x 03_reveal.png layout, 24 lines) */
    fkey(F3, ALT);
    CHECK(row_is(0, "Plain bold under slant xy") && row_is(9, ""), "F7", "Alt-F3: the document on top");
    CHECK(status_line_is(10, "A:\\CODES", "1\"", "3.5\"", 0), "F7", "the status line above the ruler");
    {
        char ruler[SCR_W + 1];
        memset(ruler, ' ', SCR_W); ruler[SCR_W] = 0;
        for (int c = 5; c < SCR_W; c += 5) ruler[c] = 0x1E;
        ruler[0] = '{'; ruler[65] = '}';
        CHECK(row_is(11, ruler) && m_col[11][0] == ((TBX_COLOR_BLACK << 4) | TBX_COLOR_GRAY), "F7",
              "the ruler: { at the left margin, a tab mark every 5 columns, } at column 65 (03_reveal.png)");
    }
    CHECK(row_is(12, "Plain [BOLD]bold[bold] [UND]under[und] [ITALC]slant[italc] [BOLD]x[bold]y") &&
          row_is(13, "") && row_is(22, ""), "F7", "the codes pane: every code by its WP 5.1 label");
    CHECK(inv_run(12, 73, 1), "F7", "the pane cursor: one cell after the last label (end of document)");
    CHECK(row_is(23, "Press Reveal Codes to restore screen"), "F7", "the restore message on the last line");
    /* Left x4 (y, [bold], x, [BOLD]): the cursor on [BOLD]; Right: on x;
     * Backspace: [BOLD] goes, [bold] with it */
    fkey(0x4B, 0); fkey(0x4B, 0); fkey(0x4B, 0); fkey(0x4B, 0);
    CHECK(inv_run(12, 59, 6), "F7", "the cursor on a code highlights its whole label ([BOLD], 05_reveal_top.png)");
    fkey(0x4D, 0);
    CHECK(inv_run(12, 65, 1), "F7", "Right: the cursor on x");
    tk(8);
    CHECK(row_is(12, "Plain [BOLD]bold[bold] [UND]under[und] [ITALC]slant[italc] xy") && inv_run(12, 59, 1),
          "F7", "Backspace on [BOLD] in Reveal Codes deletes [bold] too (MINT08 03b)");
    CHECK(faces_are(0, 23, 25, 0), "F7", "x is plain again");
    fkey(F3, ALT);
    CHECK(row_is(10, "") && row_is(12, "") && status_is("A:\\CODES", "1\"", "3.3\"", 0), "F7",
          "Alt-F3 again: the full editing screen");
    /* the edit screen keeps P1's protection: Backspace steps over a code */
    fkey(0x4F, 0);
    tk(8); tk(8); tk(8); tk(8);
    CHECK(row_is(0, "Plain bold under slant") && faces_are(0, 17, 22, 2), "F7",
          "outside Reveal Codes Backspace deletes text, never a code ([italc] stays)");
    CHECK(m_exit_calls == 0, "F7", "still running");
}

#else
/* F6 SMALL BUFFERS: thousands of keys through compaction to the size limit;
 * the saved text is the typed text (judged here by the model: SRt = ' '). */
static char g_model[8192];
static void t_small(void)
{
    uint32_t n = 0, r = 0x1234u;
    int errors = 0;
    for (int i = 0; i < 6000; i++) {
        char c;
        r ^= r << 13; r ^= r >> 17; r ^= r << 5;
        c = (r % 7u == 0u) ? ' ' : (char)('a' + r % 26u);
        tk(c);
        if (m_scr[23][0] == 'E') { errors++; tk(' '); /* any key: back */ break; }
        g_model[n++] = c;
    }
    CHECK(errors == 1, "F6", "the size limit is refused visibly (ERROR: Document too large)");
    CHECK(n == 4096u - 327u, "F6", "the document filled to the bank exactly");
    fkey(F10, 0);
    tks("big\r");
    CHECK(file_size("BIG") == 4096, "F6", "saved: prefix + the full body");
    {
        char hp[300];
        static unsigned char b[8192];
        FILE *f;
        size_t k;
        host_name("BIG", hp);
        f = fopen(hp, "rb");
        k = f ? fread(b, 1, sizeof b, f) : 0;
        if (f) fclose(f);
        int same = k == 4096u;
        for (uint32_t i = 0; same && i < n; i++) {
            char c = (char)(b[327 + i] == 0x0D ? ' ' : b[327 + i]);
            if (c != g_model[i]) same = 0;
        }
        CHECK(same, "F6", "the saved text is the typed text across compactions");
    }
}
#endif

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "usage: %s <scratch-dir>\n", argv[0]); return 2; }
    m_dir = argv[1];
    g_ctr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (g_ctr == MAP_FAILED) { perror("mmap"); return 2; }
    g_ctr[0] = g_ctr[1] = 0;
#ifdef IWORD_SMALL_BUFFERS
    launch("small", t_small);
#else
    launch("flow", t_flow);
    launch("relaunch", t_relaunch);
    launch("clean_close", t_clean_close);
    launch("clean_menu", t_clean_menu);
    launch("clean_f7", t_clean_f7);
    launch("dirty_close_cancel", t_dirty_close_cancel);
    launch("dirty_save_exit", t_dirty_save_exit);
    launch("dirty_enter_default", t_dirty_enter_default);
    launch("errors", t_errors);
    launch("into", t_into);
    launch("codes", t_codes);
#endif
    printf("test_iword_face: %d checks, %d failures\n", g_ctr[0], g_ctr[1]);
    return g_ctr[1] == 0 ? 0 : 1;
}
