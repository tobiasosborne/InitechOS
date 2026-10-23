/* iword_probe.c -- the SHIPPED SURFACE of InitechWord and the REPLAY ENGINE
 * of its manuals (PUB-REF-0001 GC-01, GC-02, GC-03; CP-16, CP-17; bead
 * initech-ipu5.10, the InitechWord manuals).
 *
 * WHAT IS UNDER TEST. The real os/iword/app/face_a.c (WORD.EXE, Face A),
 * compiled on the host over the real document core with the Toolbox Gate
 * replaced by a RECORDING MOCK (the harness/proptest/test_iword_face.c
 * pattern, bead initech-l4tj): every cell the application draws lands in a
 * 76 x 24 screen with its face and colours; the file verbs are served from a
 * host directory. Nothing here re-implements an InitechWord rule: every
 * answer is what the shipped code DOES.
 *
 * MODE 1 -- inventory <scratch-dir>. Prints the shipped inventory, one
 * NAME|KIND line per user-visible item, sorted (harness/pubs/pubs_gate.c
 * `commands` compares it with the books' .cmds lists in both directions):
 *   wpkey   a key or key sequence that DOES something on the editing screen
 *           (F1..F12 with nothing, Shift, Alt or Ctrl; Enter, Ctrl-Enter,
 *           Tab, Backspace, Delete, Esc, Ins, the cursor keys, PgUp/PgDn;
 *           the Home prefix sequences Home,<arrow> .. Home,Home,Home,<arrow>
 *           when they reach somewhere the shorter sequence does not), the
 *           keys that act on a block ("Block:F6"), and the keys a prompt
 *           answers to (F1 / Esc when they cancel one). A key "does
 *           something" when the state after it -- the screen (text, faces,
 *           reverse cells), the document bytes, the cursor, the block, the
 *           mode, Reveal Codes, the Home prefix, an exit -- differs from the
 *           state before it. Probed from a fixed document with the cursor in
 *           the middle of a line and a moved block in the Move buffer.
 *           P3 (bead initech-qvgd): the keys of the Search and Replace
 *           strings, "Srch:F6" (the -> Srch: / <- Srch: prompt, also the
 *           search string of Replace) and "Rplc:Tab" (Replace with:),
 *           probed from a string with the cursor inside it.
 *   wpmenu  every menu choice that does something: from each prompt a key
 *           opens, each digit 1..9 whose result differs both from the prompt
 *           unchanged (an ignored key) and from Cancel (F1). NAME is the
 *           keystroke path in WordPerfect's comma notation and the choice's
 *           name AS THE SCREEN SHOWS IT: "Shift-F8,1,3,2=Center",
 *           "Block:Ctrl-F4,1,1=Move". Menus are explored to any depth;
 *           Setup's Units of Measure answers with a letter,
 *           "Shift-F1,3,8,1,c=Centimeters".
 *   wppull  every pull-down menu item (the menus[] the application hands
 *           SETMBAR) whose TBX_EVT_MENU changes the state: "File,Save".
 *   wpgray  every key that shows the grey name of a WordPerfect feature
 *           InitechWord does not carry out (P3, spec/iword/fkeys.json
 *           "gray"), with the name the status line shows: "Shift-F7=Print".
 *   wpcode  every Reveal Codes label of the locked table spec/iword/
 *           reveal_codes.h, read from the header TEXT (each `#define IW_RV_*`
 *           string, the IW_RV_ATTR_INIT pairs); a label that is a prefix
 *           ("[Ln Spacing:") is listed with "...]" ("[Ln Spacing:...]"); the
 *           CP437 arrow 0x1A is written "->".
 *   wpmsg   every message the status line shows, observed by driving the
 *           application into it (fixed scenarios: the save and retrieve
 *           prompts, the exit questions, each ERROR, the block and code
 *           questions, the Reveal Codes restore line); the variable parts are
 *           normalized ("Replace A:\filename? No (Yes)", "Delete [code]? No
 *           (Yes)"). COMPLETENESS: every string literal face_a.c passes to
 *           error() or shows as a status prompt (`t = "..."`, `nm = "..."`)
 *           must be among the observed messages, so a message added to the
 *           source without a scenario is RED here (MESSAGE-UNPROBED).
 *
 * MODE 2 -- replay <script> <scratch-dir> <out-dir>. One launch of the
 * application (a fresh process: a pristine image, as a real launch loads),
 * driven by the script; files persist in <scratch-dir> between replays, so
 * a lesson can retrieve what an earlier lesson saved. Script lines:
 *   type <text>        the characters of <text>
 *   key <K> [<K> ...]  keys by name: F1..F12, Shift-/Alt-/Ctrl- + a key,
 *                      Enter, Tab, Backspace, Delete, Esc, Ins, Home, End,
 *                      Up, Down, Left, Right, PgUp, PgDn
 *   caps on|off        Caps Lock
 *   menu <Title>,<Item>  a pull-down menu choice
 *   close              the window's close box
 *   screen <name>      write the screen to <out-dir>/<name>.scr
 *   # ...              a comment
 * A .scr file is the 24 rows of 76 characters, a line "--", then 24 rows of
 * 76 attribute characters: '.' plain, '1'..'7' the face bits drawn (1 bold,
 * 2 italic, 4 underline), 'R' reverse video (the cursor), 'K' black on grey
 * (a block, the Reveal Codes ruler), 'G' grey on the blue field (the name of
 * a WordPerfect feature InitechWord does not perform, P3 spec/iword/
 * fkeys.json "gray"; the unavailable rows of Setup). The manuals print .scr files verbatim
 * (pubs/class/initech-wpc.sty \WordScreen) and the gate replays the script
 * that made them (GC-03).
 *
 * usage: iword_probe inventory <scratch-dir> [<face_a.c to scan>]
 *        iword_probe replay <script> <scratch-dir> <out-dir>
 * Host C only (Law 3). Deterministic (Rule 11). ASCII-clean (Rule 12).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "../../os/iword/app/face_a.c"   /* the application under test */

static void die(const char *tag, const char *what, const char *detail)
{
    fflush(stdout);
    fprintf(stderr, "!!! test-pubs RED %s: %s: %s\n", tag, what, detail);
    exit(1);
}

/* ---- the recording mock -------------------------------------------------- */
#define SW 76
#define SH 24
static char m_scr[SH][SW + 1];
static char m_att[SH][SW + 1];
static int  m_exit;
static const char *m_dir;
static FILE *m_fh[8];
static int  m_fail_create, m_fail_write;

int32_t tbx_register(void) { return TBX_OK; }
int32_t tbx_new_window(int32_t l, int32_t t, int32_t r, int32_t b, int32_t g)
{ (void)l; (void)t; (void)r; (void)b; (void)g; return 1; }
int32_t tbx_set_wtitle(int32_t w, const char *s) { (void)w; (void)s; return TBX_OK; }
int32_t tbx_draw_text(int32_t w, int32_t x, int32_t y, const char *s, uint32_t f, uint32_t b)
{ (void)w; (void)x; (void)y; (void)s; (void)f; (void)b; return TBX_OK; }
int32_t tbx_fill_rect(int32_t w, int32_t l, int32_t t, int32_t r, int32_t b, uint32_t c)
{
    (void)w; (void)l; (void)t; (void)r; (void)b; (void)c;
    for (int i = 0; i < SH; i++) { memset(m_scr[i], ' ', SW); memset(m_att[i], '.', SW); }
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
    uint32_t fg = f & 0xFFu, face = (f >> 8) & 0x07u;
    char a;
    (void)w;
    if (line < 0 || line >= SH || col < 0 || (x - 1) % 8 || y % 16) die("IWORD-PROBE", "DRAWCELLS", "a cell off the 76 x 24 grid");
    if (fg == TBX_COLOR_NAVY && b == TBX_COLOR_WHITE) a = 'R';
    else if (fg == TBX_COLOR_BLACK && b == TBX_COLOR_GRAY) a = 'K';
    else if (fg == TBX_COLOR_WHITE && b == TBX_COLOR_NAVY) a = face ? (char)('0' + face) : '.';
    else if (fg == TBX_COLOR_GRAY && b == TBX_COLOR_NAVY && !face) a = 'G';
    else die("IWORD-PROBE", "DRAWCELLS", "a colour pair the face never draws");
    for (int i = 0; s[i] && col + i < SW; i++) { m_scr[line][col + i] = s[i]; m_att[line][col + i] = a; }
    return TBX_OK;
}
int32_t tbx_exit(int32_t rc) { (void)rc; m_exit++; return TBX_OK; }

static void host_name(const char *path, char *out)
{ snprintf(out, 600, "%s/%s", m_dir, path); }
static int32_t new_handle(FILE *f)
{
    for (int i = 0; i < 8; i++) if (!m_fh[i]) { m_fh[i] = f; return 5 + i; }
    fclose(f); return TBX_ERR_DOS(4);
}
static FILE *fh(int32_t h) { return (h >= 5 && h < 13) ? m_fh[h - 5] : 0; }
int32_t tbx_file_create(const char *path)
{
    char hp[600]; FILE *f;
    if (m_fail_create) return TBX_ERR_DOS(5);
    host_name(path, hp); f = fopen(hp, "w+b"); return f ? new_handle(f) : TBX_ERR_DOS(5);
}
int32_t tbx_file_open(const char *path, uint32_t mode)
{ char hp[600]; FILE *f; (void)mode; host_name(path, hp); f = fopen(hp, "r+b"); return f ? new_handle(f) : TBX_ERR_DOS(2); }
int32_t tbx_file_read(int32_t h, void *b, uint32_t n)
{ FILE *f = fh(h); return f ? (int32_t)fread(b, 1, n, f) : TBX_ERR_BADARG; }
int32_t tbx_file_write(int32_t h, const void *b, uint32_t n)
{ FILE *f = fh(h); if (m_fail_write) return 0; return f ? (int32_t)fwrite(b, 1, n, f) : TBX_ERR_BADARG; }
int32_t tbx_file_seek(int32_t h, int32_t d, uint32_t o)
{
    FILE *f = fh(h);
    if (!f || fseek(f, d, o == TBX_FILE_FROM_END ? SEEK_END : o == TBX_FILE_FROM_HERE ? SEEK_CUR : SEEK_SET)) return TBX_ERR_BADARG;
    return (int32_t)ftell(f);
}
int32_t tbx_file_close(int32_t h)
{ FILE *f = fh(h); if (!f) return TBX_ERR_BADARG; fclose(f); m_fh[h - 5] = 0; return TBX_OK; }
int32_t tbx_file_delete(const char *path)
{ char hp[600]; host_name(path, hp); return remove(hp) == 0 ? TBX_OK : TBX_ERR_DOS(2); }
/* Sec 10b RENAME (initech-mcep): DOS 3.3 AH=56h -- an existing target is
 * access denied, a missing source is file not found. */
int32_t tbx_file_rename(const char *o, const char *n)
{
    char ho[600], hn[600]; FILE *f;
    host_name(o, ho); host_name(n, hn);
    if ((f = fopen(hn, "rb")) != NULL) { fclose(f); return TBX_ERR_DOS(5); }
    return rename(ho, hn) == 0 ? TBX_OK : TBX_ERR_DOS(2);
}
/* The List Files verbs (initech-qvgd) and the clock: the probe replays the
 * lesson launches only, so an empty directory, a fixed free figure and a
 * zero clock keep every printed screen as before. */
int32_t tbx_file_find(const char *pattern, uint32_t attr, void *recs, uint32_t max)
{ (void)pattern; (void)attr; (void)recs; (void)max; return 0; }
int32_t tbx_disk_free(void) { return 0; }
int32_t tbx_get_clock(void *t) { (void)t; return TBX_ERR_BADARG; }

/* ---- driving ---------------------------------------------------------------- */
static int32_t g_caps_mod;
static void ev(int32_t what, int32_t msg, int32_t mods)
{
    tbx_event_t e;
    memset(&e, 0, sizeof e);
    e.what = (uint16_t)what; e.message = msg; e.modifiers = (uint16_t)(mods | g_caps_mod);
    tbx_app_event(&e);
}
static void type_char(char c)
{
    if (c == '\r' || c == '\n') ev(keyDown, (0x1C << 8) | '\r', 0);
    else ev(keyDown, (uint8_t)c, 0);
}

typedef struct keyname { const char *name; uint8_t vk; uint8_t ch; } keyname_t;
static const keyname_t KEYS[] = {
    { "F1", 0x3B, 0 }, { "F2", 0x3C, 0 }, { "F3", 0x3D, 0 }, { "F4", 0x3E, 0 },
    { "F5", 0x3F, 0 }, { "F6", 0x40, 0 }, { "F7", 0x41, 0 }, { "F8", 0x42, 0 },
    { "F9", 0x43, 0 }, { "F10", 0x44, 0 }, { "F11", 0x57, 0 }, { "F12", 0x58, 0 },
    { "Enter", 0x1C, '\r' }, { "Tab", 0x0F, '\t' }, { "Backspace", 0x0E, 8 },
    { "Delete", 0x53, 0 }, { "Esc", 0x01, 0x1B }, { "Ins", 0x52, 0 },
    { "Home", 0x47, 0 }, { "End", 0x4F, 0 }, { "Up", 0x48, 0 }, { "Down", 0x50, 0 },
    { "Left", 0x4B, 0 }, { "Right", 0x4D, 0 }, { "PgUp", 0x49, 0 }, { "PgDn", 0x51, 0 },
};
#define NKEYS ((int)(sizeof KEYS / sizeof KEYS[0]))

/* "Shift-F8", "Ctrl-Enter", "Home" -> one keyDown; 0 = not a key name */
static int press(const char *name)
{
    int32_t mods = 0;
    for (;;) {
        if (!strncmp(name, "Shift-", 6)) { mods |= (int32_t)FLAIR_EVT_MOD_SHIFT_KEY; name += 6; }
        else if (!strncmp(name, "Alt-", 4)) { mods |= (int32_t)FLAIR_EVT_MOD_OPTION_KEY; name += 4; }
        else if (!strncmp(name, "Ctrl-", 5)) { mods |= (int32_t)FLAIR_EVT_MOD_CONTROL_KEY; name += 5; }
        else break;
    }
    for (int i = 0; i < NKEYS; i++) if (!strcmp(name, KEYS[i].name)) {
        ev(keyDown, (KEYS[i].vk << 8) | KEYS[i].ch, mods);
        return 1;
    }
    return 0;
}
/* "Home Home Up" or WordPerfect's "Home,Home,Up" -- every key of it */
static void press_seq(const char *seq, const char *where)
{
    char buf[256], *p, *tok;
    snprintf(buf, sizeof buf, "%s", seq);
    for (p = buf; *p; p++) if (*p == ',') *p = ' ';
    for (tok = strtok(buf, " "); tok; tok = strtok(0, " "))
        if (!press(tok)) die("IWORD-SCRIPT", where, tok);
}
static void pull(const char *spec, const char *where)
{
    char t[64], it[64];
    const char *comma = strchr(spec, ',');
    if (!comma || (size_t)(comma - spec) >= sizeof t) die("IWORD-SCRIPT", where, spec);
    memcpy(t, spec, (size_t)(comma - spec)); t[comma - spec] = 0;
    snprintf(it, sizeof it, "%s", comma + 1);
    for (unsigned m = 0; m < mbar.n_menus; m++) {
        if (strcmp(mbar.menus[m].title, t)) continue;
        for (unsigned i = 0; i < mbar.menus[m].n_items; i++)
            if (!strcmp(mbar.menus[m].items[i].text, it)) {
                ev(TBX_EVT_MENU, TBX_MENU_WORD(mbar.menus[m].menu_id, i + 1), 0);
                return;
            }
    }
    die("IWORD-SCRIPT", where, spec);
}

static void launch_app(void)
{
    for (int i = 0; i < SH; i++) { memset(m_scr[i], ' ', SW); memset(m_att[i], '.', SW); m_scr[i][SW] = m_att[i][SW] = 0; }
    if (tbx_app_main() != 0) die("IWORD-PROBE", "launch", "tbx_app_main failed");
    ev(updateEvt, 0, 0);
}

static void write_screen(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) die("IWORD-REPLAY", path, "cannot create");
    /* The two CP437 glyphs Face A draws -- the ruler's tab mark 0x1E and the
     * [->Indent] arrow 0x1A -- are written '^' and '`' (the book draws them
     * as the glyphs); a '^' or '`' on the screen itself would be ambiguous,
     * and any other byte outside printable ASCII is refused (Rule 2). */
    for (int i = 0; i < SH; i++) {
        char row[SW + 1];
        for (int j = 0; j < SW; j++) {
            unsigned char c = (unsigned char)m_scr[i][j];
            if (c == '^' || c == '`') die("IWORD-REPLAY", path, "a ^ or ` on the screen (reserved for the CP437 glyphs)");
            if (c == 0x1E) c = '^';
            else if (c == 0x1A) c = '`';
            else if (c < 0x20 || c > 0x7E) die("IWORD-REPLAY", path, "a byte outside printable ASCII on the screen");
            row[j] = (char)c;
        }
        row[SW] = 0;
        fprintf(f, "%s\n", row);
    }
    fprintf(f, "--\n");
    for (int i = 0; i < SH; i++) fprintf(f, "%s\n", m_att[i]);
    if (fclose(f)) die("IWORD-REPLAY", path, "write");
}

/* One script line. */
static void run_line(char *line, const char *where, const char *outdir)
{
    size_t n = strlen(line);
    while (n && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
    if (!n || line[0] == '#') return;
    if (!strncmp(line, "type ", 5)) { for (const char *p = line + 5; *p; p++) type_char(*p); return; }
    if (!strncmp(line, "key ", 4)) { press_seq(line + 4, where); return; }
    if (!strcmp(line, "caps on")) { g_caps_mod = (int32_t)FLAIR_EVT_MOD_ALPHA_LOCK; return; }
    if (!strcmp(line, "caps off")) { g_caps_mod = 0; return; }
    if (!strncmp(line, "menu ", 5)) { pull(line + 5, where); return; }
    if (!strcmp(line, "close")) { ev(TBX_EVT_GOAWAY, 1, 0); return; }
    if (!strncmp(line, "screen ", 7)) {
        char path[1024];
        const char *nm = line + 7;
        for (const char *p = nm; *p; p++)
            if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '-' || *p == '_'))
                die("IWORD-SCRIPT", where, "a screen name is [a-z0-9_-]");
        if (!outdir) die("IWORD-SCRIPT", where, "screen outside a replay");
        snprintf(path, sizeof path, "%s/%s.scr", outdir, nm);
        write_screen(path);
        return;
    }
    die("IWORD-SCRIPT", where, line);
}

static int replay(const char *script, const char *outdir)
{
    FILE *f = fopen(script, "rb");
    char line[1024];
    int ln = 0;
    if (!f) die("IWORD-REPLAY", script, "cannot open");
    launch_app();
    while (fgets(line, sizeof line, f)) {
        char where[1100];
        ln++;
        snprintf(where, sizeof where, "%s:%d", script, ln);
        run_line(line, where, outdir);
    }
    fclose(f);
    return 0;
}

/* ---- the inventory ------------------------------------------------------- */
typedef struct state {
    uint64_t sig;
    int mode;
    char row[SH][SW + 1];
} state_t;

static uint64_t fnv(uint64_t h, const void *p, size_t n)
{
    const unsigned char *b = p;
    for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 0x100000001B3ull; }
    return h;
}
static void snapshot(state_t *s)
{
    uint64_t h = 0xCBF29CE484222325ull;
    uint8_t buf[256];
    uint32_t pos = 0, k;
    for (int i = 0; i < SH; i++) { h = fnv(h, m_scr[i], SW); h = fnv(h, m_att[i], SW); }
    while ((k = iw_doc_read(&g_ed.doc, pos, buf, sizeof buf)) > 0) { h = fnv(h, buf, k); pos += k; }
    h = fnv(h, &g_ed.cur, sizeof g_ed.cur);
    h = fnv(h, &g_ed.blk, sizeof g_ed.blk);
    h = fnv(h, &g_mode, sizeof g_mode);
    h = fnv(h, &g_reveal, sizeof g_reveal);
    h = fnv(h, &g_home, sizeof g_home);
    h = fnv(h, &m_exit, sizeof m_exit);
    s->sig = h;
    s->mode = g_mode;
    for (int i = 0; i < SH; i++) memcpy(s->row[i], m_scr[i], SW + 1);
}

/* The probe document: two lines, the cursor inside the first one, the Move
 * buffer holding a word (so Retrieve has something to put back). */
static const char *const SETUP[] = {
    /* a [L/R Mar] code at the top (Home,Home,Home,Up goes before it) */
    "key Shift-F8", "type 17", "type 1.5", "key Enter Enter F7",
    "type Alpha beta gamma.",
    "key Enter",
    /* a [BOLD] code at the start of a line (Home,Home,Home,Left) */
    "key F6", "type Delta epsilon zeta.", "key F6 Enter",
    "key F6", "type Line 03 of the probe document.", "key F6 Enter", "key F6", "type Line 04 of the probe document.", "key F6 Enter",
    "key F6", "type Line 05 of the probe document.", "key F6 Enter", "key F6", "type Line 06 of the probe document.", "key F6 Enter",
    "key F6", "type Line 07 of the probe document.", "key F6 Enter", "key F6", "type Line 08 of the probe document.", "key F6 Enter",
    "key F6", "type Line 09 of the probe document.", "key F6 Enter", "key F6", "type Line 10 of the probe document.", "key F6 Enter",
    "key F6", "type Line 11 of the probe document.", "key F6 Enter", "key F6", "type Line 12 of the probe document.", "key F6 Enter",
    "key F6", "type Line 13 of the probe document.", "key F6 Enter", "key F6", "type Line 14 of the probe document.", "key F6 Enter",
    "key F6", "type Line 15 of the probe document.", "key F6 Enter", "key F6", "type Line 16 of the probe document.", "key F6 Enter",
    "key F6", "type Line 17 of the probe document.", "key F6 Enter", "key F6", "type Line 18 of the probe document.", "key F6 Enter",
    "key F6", "type Line 19 of the probe document.", "key F6 Enter", "key F6", "type Line 20 of the probe document.", "key F6 Enter",
    "key F6", "type Line 21 of the probe document.", "key F6 Enter", "key F6", "type Line 22 of the probe document.", "key F6 Enter",
    "key F6", "type Line 23 of the probe document.", "key F6 Enter", "key F6", "type Line 24 of the probe document.", "key F6 Enter",
    "key F6", "type Line 25 of the probe document.", "key F6 Enter", "key F6", "type Line 26 of the probe document.", "key F6 Enter",
    "key F6", "type Line 27 of the probe document.", "key F6 Enter", "key F6", "type Line 28 of the probe document.", "key F6 Enter",
    "key F6", "type Line 29 of the probe document.", "key F6 Enter", "key F6", "type Line 30 of the probe document.", "key F6 Enter",
    "key F6", "type Line 31 of the probe document.", "key F6 Enter", "key F6", "type Line 32 of the probe document.", "key F6",
    /* the Move buffer holds "Lin" (Block, Ctrl-F4 1 Block, 2 Copy; Esc) */
    "key Home Left Alt-F4 Right Right Right Ctrl-F4",
    "type 12",
    "key Esc",
    /* the cursor inside line 2, then down into the middle of the screen */
    "key Home Home Up Down Right Right Right Down Down Down Down Down Down Down Down Down Down Down Down",
    /* two deletions apart (P3 Undelete keeps each as a group: a cursor move
     * between them separates them), so Undelete's Restore and Previous
     * Deletion both do something */
    "key Delete Right Right Delete",
    0
};

/* Run (in a child) `lines` from the probe document (setup = 1) or from a
 * pristine launch (setup = 0), and report the state. The probe document is
 * typed ONCE, in this process, the first time a probe needs it; every probe
 * after that forks from it (so every pristine probe must come first). */
static int g_setup_done;
static state_t probe(int setup, const char *const *lines, int nlines)
{
    int fd[2];
    pid_t p;
    state_t s;
    if (setup && !g_setup_done) {
        char buf[1024];
        launch_app();
        for (int i = 0; SETUP[i]; i++) { snprintf(buf, sizeof buf, "%s", SETUP[i]); run_line(buf, "setup", 0); }
        g_setup_done = 1;
    }
    if (!setup && g_setup_done) die("IWORD-PROBE", "probe", "a pristine probe after the probe document was typed");
    if (pipe(fd)) die("IWORD-PROBE", "pipe", "failed");
    fflush(stdout); fflush(stderr);
    p = fork();
    if (p < 0) die("IWORD-PROBE", "fork", "failed");
    if (p == 0) {
        char buf[1024];
        close(fd[0]);
        if (!setup) launch_app();
        for (int i = 0; i < nlines; i++) { snprintf(buf, sizeof buf, "%s", lines[i]); run_line(buf, "probe", 0); }
        snapshot(&s);
        if (write(fd[1], &s, sizeof s) != (ssize_t)sizeof s) _exit(3);
        _exit(0);
    }
    close(fd[1]);
    {
        size_t got = 0;
        while (got < sizeof s) {
            ssize_t r = read(fd[0], (char *)&s + got, sizeof s - got);
            if (r <= 0) break;
            got += (size_t)r;
        }
        close(fd[0]);
        int st = 0;
        waitpid(p, &st, 0);
        if (got != sizeof s || !WIFEXITED(st) || WEXITSTATUS(st) != 0) die("IWORD-PROBE", lines && nlines ? lines[nlines - 1] : "setup", "the probe launch died");
    }
    return s;
}

static char g_out[4096][160];
static int g_nout;
static void emit(const char *name, const char *kind)
{
    char line[160];
    if (snprintf(line, sizeof line, "%s|%s", name, kind) >= (int)sizeof line) die("IWORD-PROBE", name, "name too long");
    for (int i = 0; i < g_nout; i++) if (!strcmp(g_out[i], line)) return;
    if (g_nout == 4096) die("IWORD-PROBE", "inventory", "too many items");
    strcpy(g_out[g_nout++], line);
}
static int cmpline(const void *a, const void *b) { return strcmp(a, b); }

/* The name of choice `d` on a menu screen: status-line menus "1 Size; 2
 * Appearance; ...: 0", "1 Bold 2 Undln ...: 0"; full-screen menus
 * "1 - Line", "7 - Margins - Left". */
static int choice_name(const state_t *s, char d, char *out, size_t cap)
{
    for (int r = SH - 1; r >= 0; r--) {
        const char *row = s->row[r];
        for (int c = 0; c < SW - 2; c++) {
            if (row[c] != d) continue;
            if (c > 0 && row[c - 1] != ' ' && row[c - 1] != ';' && row[c - 1] != ':') continue;
            const char *p = row + c + 1;
            if (!strncmp(p, " - ", 3)) p += 3;
            else if (*p == ' ' && p[1] != ' ' && p[1] != 0) p += 1;
            else continue;
            size_t n = 0;
            while (p[n] && p[n] != ';' && p[n] != ':' && n < cap - 1) {
                if (!strncmp(p + n, " - ", 3) || !strncmp(p + n, "  ", 2)) break;
                if (p[n] == ' ' && p[n + 1] >= '1' && p[n + 1] <= '9' && p[n + 2] == ' ') break;
                n++;
            }
            if (!n) continue;
            memcpy(out, p, n);
            out[n] = 0;
            return 1;
        }
    }
    return 0;
}

/* modes that take typing (a name, a measurement) or a yes / no answer:
 * not menus */
static int is_field_mode(int m)
{
    return m == M_SAVE_NAME || m == M_GET_NAME || m == M_FMTMARL || m == M_FMTMARR || m == M_FMTSP ||
           m == M_DELCODE || m == M_DELBLOCK || m == M_SAVEQ || m == M_EXITQ || m == M_REPLACE || m == M_ERROR ||
           /* P3 (initech-qvgd): the Search / Replace strings, Replace's two
            * questions, Setup's unit letter and backup question */
           m == M_SEARCH || m == M_RSEARCH || m == M_RTEXT || m == M_RCONF || m == M_RASK ||
           m == M_UNITVALUE || m == M_BACKUPVALUE;
}

/* Explore the menu reached by lines[0..n) (whose state is `at`); `label` is
 * the keystroke path so far. */
static void explore(const char *prefix, const char *label, const char **lines, int n, const state_t *at, int depth, uint64_t seen)
{
    const char *l2[16];
    state_t cancel;
    if (depth > 6 || n > 14) die("IWORD-PROBE", label, "menus nested too deep");
    for (int i = 0; i < n; i++) l2[i] = lines[i];
    l2[n] = "key F1";
    cancel = probe(1, l2, n + 1);
    for (char d = '1'; d <= '9'; d++) {
        static char typed[9][8];
        char name[96], lab[200], full[300];
        state_t s;
        snprintf(typed[d - '1'], sizeof typed[0], "type %c", d);
        l2[n] = typed[d - '1'];
        s = probe(1, l2, n + 1);
        if (s.sig == at->sig || s.sig == cancel.sig) continue;
        if (!choice_name(at, d, name, sizeof name)) {
            snprintf(full, sizeof full, "choice %c of %s does something but the screen names no such choice", d, label);
            die("IWORD-PROBE", prefix, full);
        }
        snprintf(lab, sizeof lab, "%s,%c", label, d);
        snprintf(full, sizeof full, "%s%s=%s", prefix, lab, name);
        emit(full, "wpmenu");
        /* Setup's Units of Measure answers with a LETTER (i c p w u, the
         * letters its screen names): every letter that does something is a
         * choice of its own, "Shift-F1,3,8,1,c=Centimeters" */
        if (s.mode == M_UNITVALUE) {
            state_t ucancel, u;
            static char ul[8];
            l2[n] = typed[d - '1'];
            l2[n + 1] = "key F1";
            ucancel = probe(1, l2, n + 2);
            for (char c = 'a'; c <= 'z'; c++) {
                char uname[96], ulab[220], ufull[320];
                snprintf(ul, sizeof ul, "type %c", c);
                l2[n + 1] = ul;
                u = probe(1, l2, n + 2);
                if (u.sig == s.sig || u.sig == ucancel.sig) continue;
                if (!choice_name(&s, c, uname, sizeof uname)) {
                    snprintf(ufull, sizeof ufull, "letter %c of %s does something but the screen names no such choice", c, lab);
                    die("IWORD-PROBE", prefix, ufull);
                }
                snprintf(ulab, sizeof ulab, "%s,%c", lab, c);
                snprintf(ufull, sizeof ufull, "%s%s=%s", prefix, ulab, uname);
                emit(ufull, "wpmenu");
            }
        }
        /* a submenu (not a menu already on the path: Justification's choices
         * return to Format: Line) */
        if (s.mode != M_EDIT && !is_field_mode(s.mode) && s.mode != M_RETRIEVE &&
            !(seen & (1ull << s.mode)) && s.mode != at->mode) {
            char *keep = malloc(8);
            if (!keep) die("IWORD-PROBE", "memory", "");
            snprintf(keep, 8, "type %c", d);
            l2[n] = keep;
            explore(prefix, lab, l2, n + 1, &s, depth + 1, seen | (1ull << at->mode));
            free(keep);
        }
    }
}

static const char *const MODS[] = { "", "Shift-", "Alt-", "Ctrl-" };

/* Every key (and modifier) that does something at the prompt `pre` leaves:
 * "<label><key>", or "<label><key>=<plain key>" when it acts as the key
 * alone. */
static void prompt_keys(const char *label, const char *const *pre, int npre)
{
    const char *l[12];
    static state_t pl[NKEYS];
    state_t at, s;
    char cmd[80], buf[64], nm[128];
    for (int i = 0; i < npre; i++) l[i] = pre[i];
    at = probe(1, l, npre);
    if (at.mode != M_SEARCH && at.mode != M_RSEARCH && at.mode != M_RTEXT)
        die("IWORD-PROBE", label, "the prompt keys were probed outside a Search or Replace string");
    for (int m = 0; m < 4; m++) for (int k = 0; k < NKEYS; k++) {
        snprintf(buf, sizeof buf, "%s%s", MODS[m], KEYS[k].name);
        snprintf(cmd, sizeof cmd, "key %s", buf);
        l[npre] = cmd;
        s = probe(1, l, npre + 1);
        if (m == 0) pl[k] = s;
        if (s.sig == at.sig) continue;
        if (m > 0 && s.sig == pl[k].sig) snprintf(nm, sizeof nm, "%s%s=%s", label, buf, KEYS[k].name);
        else snprintf(nm, sizeof nm, "%s%s", label, buf);
        emit(nm, "wpkey");
    }
}

static void inventory_keys(void)
{
    state_t base = probe(1, 0, 0), s;
    char buf[64], cmd[80];
    const char *l[8];
    /* single keys from the editing screen, every modifier. A modified key
     * that leaves exactly the state its plain key leaves is listed as an
     * ALIAS, "Ctrl-F10=F10" (face_a.c does not look at the modifier there) */
    static state_t plain[NKEYS];
    for (int m = 0; m < 4; m++) for (int k = 0; k < NKEYS; k++) {
        /* (Home alone only arms the prefix; its sequences are probed below) */
        snprintf(buf, sizeof buf, "%s%s", MODS[m], KEYS[k].name);
        snprintf(cmd, sizeof cmd, "key %s", buf);
        l[0] = cmd;
        s = probe(1, l, 1);
        if (m == 0) plain[k] = s;
        if (s.sig == base.sig) continue;
        /* a key that names a WordPerfect feature InitechWord does not perform
         * (P3, spec/iword/fkeys.json "gray"): the name drawn in grey on the
         * status line and nothing else -- "Shift-F7=Print" */
        if (s.mode == M_GRAY) {
            char nm[SW + 1], g[160];
            int e = SW;
            memcpy(nm, s.row[SH - 1], SW + 1);
            while (e > 0 && nm[e - 1] == ' ') nm[--e] = 0;
            if (!e || nm[0] == ' ') die("IWORD-PROBE", buf, "a grey key with no name at the left of the status line");
            snprintf(g, sizeof g, "%s=%s", buf, nm);
            emit(g, "wpgray");
            continue;
        }
        if (m > 0 && s.sig == plain[k].sig) {
            char alias[96];
            snprintf(alias, sizeof alias, "%s=%s", buf, KEYS[k].name);
            emit(alias, "wpkey");
            continue;
        }
        emit(buf, "wpkey");
        static const char *const cancel_keys[] = { "key F1", "key Esc" };
        if (s.mode != M_EDIT && !is_field_mode(s.mode) && s.mode != M_RETRIEVE) explore("", buf, l, 1, &s, 0, 0);
        if (s.mode != M_EDIT)
            for (int c = 0; c < 2; c++) {
                const char *l2[2] = { cmd, cancel_keys[c] };
                state_t t = probe(1, l2, 2);
                if (t.sig != s.sig && t.mode == M_EDIT) emit(cancel_keys[c] + 4, "wpkey");
            }
    }
    /* the Home prefix: Home alone, then Home x k + an arrow when it reaches
     * somewhere Home x (k - 1) + the same arrow does not */
    l[0] = "key Home";
    s = probe(1, l, 1);
    if (s.sig != base.sig) emit("Home", "wpkey");
    static const char *const arrows[] = { "Left", "Right", "Up", "Down" };
    for (int a = 0; a < 4; a++) {
        state_t prev;
        snprintf(cmd, sizeof cmd, "key %s", arrows[a]);
        l[0] = cmd;
        prev = probe(1, l, 1);
        for (int k = 1; k <= 3; k++) {
            char seq[64] = "", c2[96];
            for (int i = 0; i < k; i++) strcat(seq, "Home,");
            strcat(seq, arrows[a]);
            snprintf(c2, sizeof c2, "key %s", seq);
            l[0] = c2;
            s = probe(1, l, 1);
            if (s.sig != prev.sig) emit(seq, "wpkey");
            prev = s;
        }
    }
    /* keys that act on a block (Block on, three characters highlighted) */
    {
        static const char *const blk = "key Alt-F4 Right Right Right";
        state_t b;
        l[0] = blk;
        b = probe(1, l, 1);
        static state_t bplain[NKEYS];
        for (int m = 0; m < 4; m++) for (int k = 0; k < NKEYS; k++) {
            static char c3[96];
            char full[96];
            const char *nm = KEYS[k].name;
            if (!strcmp(nm, "Home") || !strcmp(nm, "End") || !strcmp(nm, "Up") || !strcmp(nm, "Down") ||
                !strcmp(nm, "Left") || !strcmp(nm, "Right") || !strcmp(nm, "PgUp") || !strcmp(nm, "PgDn"))
                continue;                                    /* the cursor keys stretch it */
            snprintf(buf, sizeof buf, "%s%s", MODS[m], nm);
            snprintf(c3, sizeof c3, "key %s", buf);
            l[0] = blk; l[1] = c3;
            s = probe(1, l, 2);
            if (m == 0) bplain[k] = s;
            if (s.sig == b.sig) continue;
            if (m > 0 && s.sig == bplain[k].sig) {
                snprintf(full, sizeof full, "Block:%s=%s", buf, nm);
                emit(full, "wpkey");
                continue;
            }
            snprintf(full, sizeof full, "Block:%s", buf);
            emit(full, "wpkey");
            if (s.mode != M_EDIT && !is_field_mode(s.mode) && s.mode != M_RETRIEVE) explore("Block:", buf, l, 2, &s, 0, 0);
        }
    }
    /* the keys of the Search and Replace strings (P3, initech-qvgd): from
     * each prompt with a string typed and the cursor inside it, every key
     * and modifier; "Srch:F6" (both directions of Search and Replace's
     * search string), "Rplc:Tab" (Replace with:); a modified key that acts
     * as the key alone is an alias, "Srch:Shift-F2=F2" */
    {
        static const char *const fwd[] = { "key F2", "type document", "key Left Left" };
        static const char *const bwd[] = { "key Shift-F2", "type document", "key Left Left" };
        static const char *const rsr[] = { "key Alt-F2", "type n", "type document", "key Left Left" };
        static const char *const rtx[] = { "key Alt-F2", "type n", "type document", "key F2", "type paper", "key Left Left" };
        prompt_keys("Srch:", fwd, 3);
        prompt_keys("Srch:", bwd, 3);
        prompt_keys("Srch:", rsr, 4);
        prompt_keys("Rplc:", rtx, 6);
    }
}

static void inventory_pull(void)
{
    state_t base = probe(1, 0, 0);
    for (unsigned m = 0; m < mbar.n_menus; m++)
        for (unsigned i = 0; i < mbar.menus[m].n_items; i++) {
            char cmd[160], name[128];
            const char *l[1];
            snprintf(name, sizeof name, "%s,%s", mbar.menus[m].title, mbar.menus[m].items[i].text);
            snprintf(cmd, sizeof cmd, "menu %s", name);
            l[0] = cmd;
            if (probe(1, l, 1).sig != base.sig) emit(name, "wppull");
        }
}

/* ---- codes: the locked table's text ---------------------------------------- */
static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    long n;
    char *b;
    if (!f) die("IWORD-PROBE", path, "cannot open");
    fseek(f, 0, SEEK_END); n = ftell(f); rewind(f);
    b = malloc((size_t)n + 1);
    if (!b || fread(b, 1, (size_t)n, f) != (size_t)n) die("IWORD-PROBE", path, "read");
    b[n] = 0;
    fclose(f);
    return b;
}
/* One C string literal at *pp (escapes \x1A, \", \\), its value appended to
 * out with 0x1A written "->"; returns 0 when *pp is not a literal. */
static int literal(const char **pp, char *out, size_t cap)
{
    const char *p = *pp;
    size_t n = strlen(out);
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return 0;
    for (p++; *p && *p != '"'; p++) {
        int c = (unsigned char)*p;
        if (c == '\\') {
            p++;
            if (*p == 'x') { c = (int)strtol(p + 1, (char **)&p, 16); p--; }
            else c = (unsigned char)*p;
        }
        if (c == 0x1A) { if (n + 2 < cap) { out[n++] = '-'; out[n++] = '>'; } }
        else if (n + 1 < cap) out[n++] = (char)c;
    }
    out[n] = 0;
    *pp = *p ? p + 1 : p;
    return 1;
}
static void inventory_codes(const char *header)
{
    char *h = slurp(header);
    const char *p = h;
    int found = 0;
    while ((p = strstr(p, "#define IW_RV_"))) {
        char name[64] = "", val[128] = "";
        const char *q = p + 8;
        size_t n = strcspn(q, " \t(");
        if (n >= sizeof name) die("IWORD-PROBE", header, "long macro name");
        memcpy(name, q, n); name[n] = 0;
        q += n;
        p = q;
        if (!strcmp(name, "IW_RV_ATTR_INIT")) {
            const char *end = strstr(q, "}  \\\n}");
            if (!end) end = q + strlen(q);
            while ((q = strchr(q, '{')) && q < end) {
                char on[64] = "", off[64] = "";
                q = strchr(q, '"');
                if (!q || q > end) break;
                if (!literal(&q, on, sizeof on)) break;
                while (*q == ',' || *q == ' ') q++;
                if (!literal(&q, off, sizeof off)) die("IWORD-PROBE", header, "an attribute row without its off label");
                emit(on, "wpcode"); emit(off, "wpcode");
                found += 2;
            }
            continue;
        }
        while (literal(&q, val, sizeof val)) ;
        if (!val[0] || val[0] != '[') continue;            /* not a label */
        if (val[strlen(val) - 1] != ']') strcat(val, "...]");
        emit(val, "wpcode");
        found++;
    }
    if (found < 20) die("IWORD-PROBE", header, "fewer than 20 code labels: the table was not read");
    free(h);
}

/* ---- messages ------------------------------------------------------------------ */
static char g_msgs[64][96];
static int g_nmsgs;
static void observe(const state_t *s, int row, const char *from, const char *to)
{
    const char *r = s->row[row];
    int c = 0;
    while (c < SW) {
        char seg[96];
        size_t n = 0;
        while (c < SW && r[c] == ' ') c++;
        if (c >= SW) break;
        while (c < SW && !(r[c] == ' ' && (c + 1 >= SW || r[c + 1] == ' ')) && n < sizeof seg - 1) seg[n++] = r[c++];
        seg[n] = 0;
        if (!strncmp(seg, "Doc ", 4) || !strncmp(seg, "A:\\", 3)) continue;
        if (from) {
            char *at = strstr(seg, from), tmp[96];
            if (at) {
                *at = 0;
                snprintf(tmp, sizeof tmp, "%s%s%s", seg, to, at + strlen(from));
                snprintf(seg, sizeof seg, "%s", tmp);
            }
        }
        int k;
        for (k = 0; k < g_nmsgs && strcmp(g_msgs[k], seg); k++) ;
        if (k == g_nmsgs) { if (g_nmsgs == 64) die("IWORD-PROBE", "messages", "too many"); strcpy(g_msgs[g_nmsgs++], seg); }
        emit(seg, "wpmsg");
    }
}
static state_t scenario(int setup, int fail_create, int fail_write, const char *const *lines, int n)
{
    state_t s;
    m_fail_create = fail_create;
    m_fail_write = fail_write;
    s = probe(setup, lines, n);
    m_fail_create = m_fail_write = 0;
    return s;
}
/* the one pristine probe: F7 on an untouched document */
static state_t g_clean_f7;
static void pristine_probes(void)
{
    const char *c[] = { "key F7" };
    g_clean_f7 = scenario(0, 0, 0, c, 1);
}
/* Blank every line of a named mutant's region (#ifdef IWORD_MUT_* up to
 * its #else or #endif; the #else of an #ifndef IWORD_MUT_*), so a literal
 * only a mutant build shows ("Search:", "Not found") is not demanded of the
 * shipped program. Nesting is followed; the face nests none. */
static void mutant_regions_out(char *src)
{
    char *p = src;
    int depth = 0, mut = 0, mutdepth = 0;
    while (*p) {
        char *eol = strchr(p, '\n');
        char *q = p;
        size_t len = eol ? (size_t)(eol - p) : strlen(p);
        int blank = 0;
        while (*q == ' ' || *q == '\t') q++;
        if (!strncmp(q, "#if", 3)) {
            depth++;
            if (!mut && !strncmp(q, "#ifdef IWORD_MUT_", 17)) { mut = 1; mutdepth = depth; }
            else if (!mut && !strncmp(q, "#ifndef IWORD_MUT_", 18)) { mut = 2; mutdepth = depth; }
        } else if (!strncmp(q, "#else", 5) && mut && depth == mutdepth) {
            mut = mut == 1 ? 2 : 3;
        } else if (!strncmp(q, "#endif", 6)) {
            if (mut && depth == mutdepth) mut = 0;
            depth--;
        } else if (mut == 1 || mut == 3) blank = 1;
        if (blank) memset(p, ' ', len);
        if (!eol) break;
        p = eol + 1;
    }
}
static void inventory_messages(const char *face_src)
{
    observe(&g_clean_f7, 23, 0, 0);
    char path[700];
    FILE *f;
    /* a file that is not a WordPerfect document, and one too large */
    snprintf(path, sizeof path, "%s/JUNK", m_dir);
    f = fopen(path, "wb"); if (!f) die("IWORD-PROBE", path, "create"); fputs("Not a document.\r\n", f); fclose(f);
    snprintf(path, sizeof path, "%s/BIGFILE", m_dir);
    f = fopen(path, "wb"); if (!f) die("IWORD-PROBE", path, "create");
    for (int i = 0; i < 70000; i++) fputc('x', f);
    fclose(f);
    {
        const char *a[] = { "key F10" };                        observe((state_t[]){ scenario(1, 0, 0, a, 1) }, 23, 0, 0);
        const char *b[] = { "key Shift-F10" };                  observe((state_t[]){ scenario(1, 0, 0, b, 1) }, 23, 0, 0);
        const char *d[] = { "key F7", "type n" };               observe((state_t[]){ scenario(1, 0, 0, d, 2) }, 23, 0, 0);
        const char *e[] = { "key Shift-F10", "type NOFILE", "key Enter" };   observe((state_t[]){ scenario(1, 0, 0, e, 3) }, 23, 0, 0);
        const char *g[] = { "key Shift-F10", "type JUNK", "key Enter" };     observe((state_t[]){ scenario(1, 0, 0, g, 3) }, 23, 0, 0);
        const char *h[] = { "key F10", "type A+B", "key Enter" };            observe((state_t[]){ scenario(1, 0, 0, h, 3) }, 23, 0, 0);
        const char *i[] = { "key F10", "type FILENAME", "key Enter", "key F10", "key Enter" };
        observe((state_t[]){ scenario(1, 0, 0, i, 5) }, 23, "FILENAME", "filename");
        const char *j[] = { "key F10", "type NEWFILE", "key Enter" };        observe((state_t[]){ scenario(1, 1, 0, j, 3) }, 23, 0, 0);
        const char *k[] = { "key F10", "type NEWFILE", "key Enter" };        observe((state_t[]){ scenario(1, 0, 1, k, 3) }, 23, 0, 0);
        const char *l[] = { "key Shift-F10", "type BIGFILE", "key Enter" };  observe((state_t[]){ scenario(1, 0, 0, l, 3) }, 23, 0, 0);
        const char *m[] = { "key Alt-F4" };                     observe((state_t[]){ scenario(1, 0, 0, m, 1) }, 23, 0, 0);
        const char *o[] = { "key Alt-F4 Right Ctrl-F4", "type 11" };   observe((state_t[]){ scenario(1, 0, 0, o, 2) }, 23, 0, 0);
        const char *q[] = { "key Alt-F4 Right Delete" };        observe((state_t[]){ scenario(1, 0, 0, q, 1) }, 23, 0, 0);
        const char *r[] = { "key F8 Delete" };                  observe((state_t[]){ scenario(1, 0, 0, r, 1) }, 23, "[und]", "[code]");
        const char *t[] = { "key Alt-F3" };                     observe((state_t[]){ scenario(1, 0, 0, t, 1) }, 23, 0, 0);
        /* P3 (initech-qvgd): Search, Replace, Undelete, Setup */
        const char *u[] = { "key F2" };                         observe((state_t[]){ scenario(1, 0, 0, u, 1) }, 23, 0, 0);
        const char *v[] = { "key Shift-F2" };                   observe((state_t[]){ scenario(1, 0, 0, v, 1) }, 23, 0, 0);
        const char *w[] = { "key F2", "type nowhere", "key F2" };            observe((state_t[]){ scenario(1, 0, 0, w, 3) }, 23, 0, 0);
        const char *x[] = { "key Alt-F2" };                     observe((state_t[]){ scenario(1, 0, 0, x, 1) }, 23, 0, 0);
        const char *y[] = { "key Alt-F2", "type n", "type document", "key F2" };   observe((state_t[]){ scenario(1, 0, 0, y, 4) }, 23, "document", "");
        const char *z[] = { "key Alt-F2", "type y", "type document", "key F2", "type paper", "key F2" };
        observe((state_t[]){ scenario(1, 0, 0, z, 6) }, 23, 0, 0);
        const char *a2[] = { "key F1" };                        observe((state_t[]){ scenario(1, 0, 0, a2, 1) }, 23, 0, 0);
        const char *b2[] = { "key Shift-F1" };                  observe((state_t[]){ scenario(1, 0, 0, b2, 1) }, 23, 0, 0);
        const char *c2[] = { "key Shift-F1", "type 381" };      observe((state_t[]){ scenario(1, 0, 0, c2, 2) }, 23, 0, 0);
        const char *d2[] = { "key Shift-F1", "type 312" };      observe((state_t[]){ scenario(1, 0, 0, d2, 2) }, 23, 0, 0);
        const char *e2[] = { "key Shift-F1", "type 381i" };     observe((state_t[]){ scenario(1, 1, 0, e2, 2) }, 23, 0, 0);
    }
    /* completeness: the literals the source shows on the status line */
    {
        char *src = slurp(face_src);
        static const char *const pats[] = { "error(\"", " t = \"", " nm = \"", "setup_row(23, \"" };
        mutant_regions_out(src);
        for (int pi = 0; pi < 4; pi++) {
            const char *p = src;
            while ((p = strstr(p, pats[pi]))) {
                char val[128] = "";
                const char *q = p + strlen(pats[pi]) - 1;
                p = q + 1;
                if (!literal(&q, val, sizeof val)) continue;
                size_t n = strlen(val);
                while (n && val[n - 1] == ' ') val[--n] = 0;    /* "Document to be Saved: " */
                int k;
                for (k = 0; k < g_nmsgs && strcmp(g_msgs[k], val); k++) ;
                if (k == g_nmsgs) die("MESSAGE-UNPROBED", face_src, val);
            }
        }
        free(src);
    }
}

int main(int argc, char **argv)
{
    if ((argc == 3 || argc == 4) && !strcmp(argv[1], "inventory")) {
        m_dir = argv[2];
        pristine_probes();
        inventory_keys();
        inventory_pull();
        inventory_codes("spec/iword/reveal_codes.h");
        /* the source whose status-line literals must all be observed (a
         * third argument names another copy: test-pubs-iword-mutant) */
        inventory_messages(argc == 4 ? argv[3] : "os/iword/app/face_a.c");
        qsort(g_out, (size_t)g_nout, sizeof g_out[0], cmpline);
        for (int i = 0; i < g_nout; i++) puts(g_out[i]);
        return 0;
    }
    if (argc == 5 && !strcmp(argv[1], "replay")) {
        m_dir = argv[3];
        return replay(argv[2], argv[4]);
    }
    die("USAGE", "iword_probe", "inventory <scratch> | replay <script> <scratch> <out>");
    return 2;
}
