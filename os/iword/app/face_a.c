/*
 * os/iword/app/face_a.c -- WORD.EXE, InitechWord's Face A: the WordPerfect
 * 5.1 editing screen, drawn natively in a FLAIR window by a disk-launched
 * application (THE ARTIFACT).
 *
 * bead: initech-l4tj (IWORD P1, Face A skeleton). Epic initech-fdxa.
 * Plan: docs/plans/INITECH-WORD-plan.md Sec 4 Face A (one native FlairApp
 *       window, the WP 5.1 surface rendered NATIVELY in fixed character
 *       cells; ADR-0013 Sec 4: a native tenant, never a text console) and
 *       Sec 7 P1. Packaging: the C disk-application path Initech 123 opened
 *       (os/apps/tbx, Makefile tenant-exe-rules: an InitechMZ flat-32 .EXE
 *       through the INT 81h Toolbox Gate; WL-0094/WL-0095).
 *
 * THE SCREEN, laid out from the real WordPerfect 5.1 (../wordperfect51-
 * decomp/goldens/minted/MINT0x.shots, an 80 x 25 text screen): the editing
 * area from line 0, text from column 0 (the 1" left margin is the screen's
 * left edge), and the STATUS LINE on the last line -- at its left the
 * document's name once it has one ("C:\WP51\MINT01", MINT01.shots/
 * 04_saved.png), at its right "Doc 1 Pg 1 Ln 1" Pos 1"": the "Doc .. Ln"
 * group RIGHT-aligned to end at column 67 and "Pos" at column 69 whatever
 * the widths (01_edit "Ln 1"" starts at column 52, MINT02 02_typed "Ln
 * 1.17"" at 49; "Pos" at 69 in both). Caps Lock shows "POS" (Reference
 * Manual, Caps Lock "Status Line", PDF 74). The window is 77 x 24 cells
 * (FLAIR_SCREEN 640 x 480 less the menu bands and the frame), so 23 lines
 * of text show where the DOS screen shows 24; the 65-column line (1"/1"
 * margins at 10 pitch, spec/iword/wp5_format.h) and the whole status line
 * fit. WordPerfect's blue field is the Toolbox's NAVY with WHITE text
 * (spec/toolbox_gate.h Sec 7 tokens); the cursor is the cell in reverse
 * video (WP's is the hardware underline; a cell API has no underline --
 * authored adaptation). A hard page shows as a row of '=' (MINT03.shots/
 * 02_typed.png).
 *
 * KEYS. Typing, Enter (hard return), Backspace, Delete, the arrows, Home /
 * End (start / end of the line -- WP's Home is a prefix key; the shortcut
 * is authored). Word wrap as WordPerfect stores it (os/iword/core/edit.c).
 * Codes are protected in P1: Backspace / Delete step over them (P2,
 * initech-jojk, gives them meaning and WP's "Delete [code]?" prompt).
 *   F10 Save        "Document to be Saved: " (Reference Manual, Exit p. 195,
 *                   PDF 213; Save p. 564, PDF 582): the current name is
 *                   offered, a character typed first replaces it (Save p.
 *                   565, PDF 583), Enter saves; an existing file asks
 *                   "Replace A:\NAME? No (Yes)" (the prompt form of Macros,
 *                   PDF 399, "Replace YOURMAC.WPM? No (Yes)"): y replaces,
 *                   any other key asks for a name again (Save p. 565).
 *   Shift-F10       "Document to be Retrieved: " (Directories, PDF 137):
 *   Retrieve        "The document appears on the screen at the cursor"
 *                   (Retrieve p. 553, PDF 571) -- onto a clear screen it
 *                   becomes the document (its own prefix kept, so it saves
 *                   back byte for byte); into a document, its text is
 *                   inserted at the cursor. A missing file: "ERROR: File Not
 *                   Found" (p. 554, PDF 572); not a 5.1 document: "ERROR:
 *                   Incompatible File Format" (Appendix E, PDF 767; the
 *                   "ERROR: " prefix as the File Not Found message shows it).
 *   F7 Exit         THE ONE EXIT GUARD (bead initech-fvnj pattern), see below.
 *   F1 Cancel       backs out of any prompt to the document (Exit p. 193,
 *                   PDF 211: "press Cancel (F1) at the Save or Exit prompts
 *                   to return to the document"); Esc does the same
 *                   (authored convenience).
 * Drive "A:\" in the names shown stands for the data volume (as 123.EXE
 * shows "A:\*.wk1"); WordPerfect adds no extension (MINT01 was saved as
 * "MINT01", 04_saved.png).
 *
 * THE EXIT GUARD. Every way out -- F7, File > Exit (Ctrl-Q) and the close
 * box (TBX_GOAWAY_ASK) -- comes to request_exit(). F7 always asks, as WP
 * does (Exit p. 193: "Whenever you exit a WordPerfect document, you are
 * always asked if you want to save"); the Macintosh routes end at once when
 * nothing is unsaved and otherwise ask the same questions. The questions are
 * WordPerfect's own, on the status line (A Brief Lesson p. 21, PDF 38):
 * "Save Document? Yes (No)" -- with "(Text was not modified)" at the right
 * end when nothing changed (Exit p. 193) -- y / Enter (the default, "the
 * option not in parentheses") saves through the F10 flow, n goes on; then
 * "Exit WP? No (Yes)": y ends InitechWord, n / Enter clears the screen for a
 * new document (Exit p. 193 step 4). F1 / Esc at either keeps the document.
 *
 * Messages not in the manual (the size refusals) are authored, no local
 * reference. Freestanding C, ASCII-clean (Rule 12), deterministic (Rule 11).
 */
#include "tbx.h"
#include "iword.h"
#include "iword/wp5_format.h"

const char tbx_app_name[] = "WORD";

/* ---- geometry (the 123.EXE window, os/i123/app/face_a.c) ------------------ */
#define WIN_L        0
#define WIN_T        40
#define WIN_R        640
#define WIN_B        480
#define CW           8           /* TBX_CELL_W */
#define CH           16          /* TBX_CELL_H */
#define SCR_COLS     77
#define SCR_LINES    24
#define X0           1
#define EDIT_ROWS    (SCR_LINES - 1)     /* 23 */
#define LINE_STATUS  (SCR_LINES - 1)
#define STATUS_DOC_END 67                /* "... Ln 1"" ends at column 67 */
#define STATUS_POS   69                  /* "Pos" at column 69            */

#define FG           TBX_COLOR_WHITE
#define BG           TBX_COLOR_NAVY

#define MENU_FILE    129
#define ITEM_RETRIEVE 1
#define ITEM_SAVE    2
#define ITEM_EXIT    3

/* PS/2 set-1 make codes (event.c cooks message = vkey << 8 | ascii) */
#define VK_ESC       0x01
#define VK_BSPC      0x0E
#define VK_ENTER     0x1C
#define VK_F1        0x3B
#define VK_F7        0x41
#define VK_F10       0x44
#define VK_HOME      0x47
#define VK_UP        0x48
#define VK_LEFT      0x4B
#define VK_RIGHT     0x4D
#define VK_END       0x4F
#define VK_DOWN      0x50
#define VK_DEL       0x53

/* ---- the menu bar (spec/toolbox_gate.h Sec 9): WP 5.1's File menu
 * (Reference Manual, "Select Save from the File menu" p. 564, "Select
 * Retrieve from the File menu" p. 553, "Select Exit from the File menu"
 * p. 193). ^Q on Exit is the FLAIR Quit chord every tenant answers. */
static const tbx_menuitem_t file_items[] = {
    { "Retrieve", 0, 0,   0, 1, 0, { 0, 0, 0 } },
    { "Save",     0, 0,   0, 1, 0, { 0, 0, 0 } },
    { "Exit",     0, 'Q', 0, 1, 0, { 0, 0, 0 } },
};
static const tbx_menuinfo_t menus[] = {
    { MENU_FILE, 0, "File", file_items, 3, 0 },
};
static const tbx_menubar_t mbar = { menus, 1, 1, 0 };

/* ---- the document --------------------------------------------------------
 * Two 64 KB banks: one holds the current document's ORIGINAL (prefix +
 * body, the piece table's read-only buffer), the other is scratch -- a file
 * is read into it, a save is flattened into it, and on success it becomes
 * the original (so a save also compacts the piece table). All BSS (the
 * e_minalloc area): file buffers may be BSS (spec/toolbox_gate.h Sec 10). */
#ifndef IWORD_SMALL_BUFFERS
#define BANK        65536u
#define ADD_CAP     65536u
#define PIECES      8192u
#define ROOM_PIECES 2048u        /* compact below this many free pieces   */
#define ROOM_ADD    8192u        /* ... or this many free add bytes       */
#else
/* The host face test only (test-iword-face): tiny buffers, so a few
 * thousand keys exercise compaction and the size refusal. NEVER shipped. */
#define BANK        4096u
#define ADD_CAP     1024u
#define PIECES      96u
#define ROOM_PIECES 32u
#define ROOM_ADD    256u
#endif
static uint8_t       g_bank[2][BANK];
static uint32_t      g_live;                /* the bank holding the original */
static uint8_t       g_add[ADD_CAP];
static iw_piece_t    g_pc[PIECES];
static iw_ed_t       g_ed;
static const uint8_t *g_prefix;
static uint32_t      g_prefix_len;
static uint32_t      g_top;                 /* first line on the screen */

/* ---- the screen ---------------------------------------------------------- */
static int32_t  g_win TBX_IN_IMAGE = 0;
static char     g_line[SCR_COLS + 1] TBX_IN_IMAGE = { 0 };
static char     g_shadow[SCR_LINES][SCR_COLS + 1];
static uint8_t  g_shadow_ok[SCR_LINES];
static int      g_cur_row = -1, g_cur_col;
static uint8_t  g_caps;

/* ---- prompts --------------------------------------------------------------- */
enum { M_EDIT, M_SAVE_NAME, M_GET_NAME, M_REPLACE, M_SAVEQ, M_EXITQ, M_ERROR };
static int      g_mode = M_EDIT;
static int      g_err_return;               /* the prompt an ERROR returns to */
static const char *g_err;
static char     g_name[16];                 /* the name being typed        */
static uint32_t g_nlen;
static uint8_t  g_fresh;                    /* the offered name: a key replaces it */
static char     g_path[TBX_FILE_PATH_MAX];  /* "NAME" for the file verbs     */
static char     g_current[24];              /* "A:\NAME" once saved/retrieved */
static char     g_shown[24];                /* "A:\NAME" of the name being used */
static uint8_t  g_exiting;                  /* the save is part of F7 Exit  */

/* ---- small helpers --------------------------------------------------------- */
static uint32_t slen(const char *s) { uint32_t n = 0; while (s[n]) n++; return n; }
static void scopy(char *d, const char *s) { uint32_t i = 0; do { d[i] = s[i]; } while (s[i++]); }
static char upper(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 32) : c; }

static void put_cells(int line, int col, const char *s, uint32_t n, uint32_t fg, uint32_t bg)
{
    uint32_t i;
    if (n > SCR_COLS) n = SCR_COLS;
    for (i = 0; i < n; i++) g_line[i] = s[i];
    g_line[n] = '\0';
    (void)tbx_draw_cells(g_win, X0 + col * CW, line * CH, g_line, fg, bg);
}

/* Draw row `line` as `text` (SCR_COLS chars) unless the screen has it. */
static void put_row(int line, const char *text)
{
    int same = g_shadow_ok[line];
    for (uint32_t i = 0; same && i < SCR_COLS; i++) if (g_shadow[line][i] != text[i]) same = 0;
    if (same) return;
    for (uint32_t i = 0; i < SCR_COLS; i++) g_shadow[line][i] = text[i];
    g_shadow[line][SCR_COLS] = 0;
    g_shadow_ok[line] = 1;
    put_cells(line, 0, text, SCR_COLS, FG, BG);
}

/* ---- the document's bytes in a bank ---------------------------------------- */
static void doc_reset(void)
{
    iw_doc_init(&g_ed.doc, g_add, ADD_CAP, g_pc, PIECES, BANK - IW_WP5_DEFAULT_DOCSTART);
    iw_ed_init(&g_ed, IW_WP5_WRAP_COLS);
    g_prefix = iw_wp5_default_prefix(&g_prefix_len);
    g_top = 0;
}

/* Flatten prefix + body into the spare bank; *n = its length. */
static int flatten(uint32_t *n)
{
    return iw_wp5_write(g_prefix, g_prefix_len, &g_ed.doc, g_bank[1u - g_live], BANK, n);
}

/* The spare bank (prefix + body, n bytes, already validated or flattened)
 * becomes the original. Positions do not move: the bytes are the same. */
static void adopt(uint32_t n, uint32_t prefix_len)
{
    uint32_t cur = g_ed.cur;
    uint8_t dirty = g_ed.dirty;
    uint32_t goal = g_ed.goal;
    g_live = 1u - g_live;
    g_prefix = g_bank[g_live];
    g_prefix_len = prefix_len;
    iw_doc_init(&g_ed.doc, g_add, ADD_CAP, g_pc, PIECES, BANK - prefix_len);
    iw_doc_load(&g_ed.doc, g_bank[g_live] + prefix_len, n - prefix_len);
    g_ed.cur = cur;
    g_ed.dirty = dirty;
    g_ed.goal = goal;
}

/* Compaction: the document's bytes become one original (one piece, an
 * empty add buffer). Always fits: the document never outgrows a bank. */
static void compact(void)
{
    uint32_t n;
    if (flatten(&n) == IW_OK) adopt(n, g_prefix_len);
}

/* Keep room for the next edit (the piece table grows with every edit). */
static void ensure_room(void)
{
    if (g_ed.doc.pc_cap - g_ed.doc.npc >= ROOM_PIECES && g_ed.doc.add_cap - g_ed.doc.add_len >= ROOM_ADD) return;
    compact();
}

/* One edit. IW_E_FULL either refused the edit (nothing changed: compact
 * and try once more) or stopped the re-wrap after it (the length changed:
 * compact and finish the wrap -- reflow is idempotent). */
enum { ED_TYPE, ED_ENTER, ED_BACKSPACE, ED_DELETE };
static int edit_once(int op, uint8_t ch)
{
    switch (op) {
    case ED_TYPE:      return iw_ed_type(&g_ed, ch);
    case ED_ENTER:     return iw_ed_enter(&g_ed);
    case ED_BACKSPACE: return iw_ed_backspace(&g_ed);
    default:           return iw_ed_delete(&g_ed);
    }
}
static int edit(int op, uint8_t ch)
{
    uint32_t before = g_ed.doc.len;
    int rc;
    ensure_room();
    rc = edit_once(op, ch);
    if (rc != IW_E_FULL) return rc;
    compact();
    if (g_ed.doc.len == before) return edit_once(op, ch);
    if (g_ed.cur > 0u) rc = iw_reflow_para(&g_ed.doc, g_ed.cur - 1u, g_ed.width);
    if (rc == IW_OK || rc == IW_E_FULL) rc = iw_reflow_para(&g_ed.doc, g_ed.cur, g_ed.width);
    return rc;
}

/* ---- painting ------------------------------------------------------------------ */
static void row_text(const iw_line_t *ln, char *out)
{
    uint32_t p = ln->start, n = 0;
    while (p < ln->text_end && n < SCR_COLS) {
        int k = iw_unit_kind(&g_ed.doc, p);
        if (k == IW_U_TEXT) out[n++] = (char)iw_doc_byte(&g_ed.doc, p);
        else if (k == IW_U_HSPACE) out[n++] = ' ';
        p = iw_unit_next(&g_ed.doc, p);
    }
    while (n < SCR_COLS) out[n++] = ln->term == IW_U_HPG ? '=' : ' ';
    out[SCR_COLS] = 0;
}

/* Keep the cursor's line on the screen. */
static void follow(const iw_where_t *w)
{
    if (w->line < g_top) g_top = w->line;
    if (w->line >= g_top + EDIT_ROWS) g_top = w->line - (EDIT_ROWS - 1u);
}

static void paint_status(const iw_where_t *w)
{
    char s[SCR_COLS + 1], num[12], doc[32];
    uint32_t n, i, k;
    for (i = 0; i < SCR_COLS; i++) s[i] = ' ';
    s[SCR_COLS] = 0;
    if (g_mode == M_EDIT) {
        /* left: the document's name */
        for (i = 0; g_current[i] && i < 40u; i++) s[i] = g_current[i];
        /* "Doc 1 Pg <p> Ln <ln>" ending at column STATUS_DOC_END */
        scopy(doc, "Doc 1 Pg ");
        n = slen(doc);
        {
            uint32_t pg = w->page, t = 0;
            char tmp[12];
            do { tmp[t++] = (char)('0' + pg % 10u); pg /= 10u; } while (pg);
            while (t) doc[n++] = tmp[--t];
        }
        doc[n++] = ' '; doc[n++] = 'L'; doc[n++] = 'n'; doc[n++] = ' ';
        k = iw_fmt_inches(iw_ln_hundredths(w->page_line), num);
        for (i = 0; i < k; i++) doc[n++] = num[i];
        for (i = 0; i < n; i++) s[STATUS_DOC_END + 1 - n + i] = doc[i];
        s[STATUS_POS] = 'P';
        s[STATUS_POS + 1] = g_caps ? 'O' : 'o';
        s[STATUS_POS + 2] = g_caps ? 'S' : 's';
        k = iw_fmt_inches(iw_pos_hundredths(w->col), num);
        for (i = 0; i < k && STATUS_POS + 4 + i < SCR_COLS; i++) s[STATUS_POS + 4 + i] = num[i];
    } else {
        const char *t = "";
        char replace[48];
        if (g_mode == M_SAVE_NAME) t = "Document to be Saved: ";
        else if (g_mode == M_GET_NAME) t = "Document to be Retrieved: ";
        else if (g_mode == M_SAVEQ) t = "Save Document? Yes (No)";
        else if (g_mode == M_EXITQ) t = "Exit WP? No (Yes)";
        else if (g_mode == M_ERROR) t = g_err;
        else if (g_mode == M_REPLACE) {
            n = 0;
            scopy(replace, "Replace ");
            n = slen(replace);
            for (i = 0; g_shown[i] && n < 40u; i++) replace[n++] = g_shown[i];
            scopy(replace + n, "? No (Yes)");
            t = replace;
        }
        n = 0;
        for (i = 0; t[i] && n < SCR_COLS; i++) s[n++] = t[i];
        if (g_mode == M_SAVE_NAME || g_mode == M_GET_NAME)
            for (i = 0; i < g_nlen && n < SCR_COLS; i++) s[n++] = g_name[i];
        if (g_mode == M_SAVEQ && !g_ed.dirty) {
            const char *nm = "(Text was not modified)";
            k = slen(nm);
            for (i = 0; i < k; i++) s[SCR_COLS - k + i] = nm[i];
        }
    }
    put_row(LINE_STATUS, s);
}

static void paint(void)
{
    static iw_line_t lines[EDIT_ROWS];
    char row[SCR_COLS + 1];
    iw_where_t w;
    uint32_t got, i;
    int crow, ccol;
    iw_ed_where(&g_ed, &w);
    follow(&w);
    got = iw_ed_lines(&g_ed, g_top, EDIT_ROWS, lines);
    crow = (int)(w.line - g_top);
    ccol = (int)(w.col < SCR_COLS ? w.col : SCR_COLS - 1u);
    /* the cell the cursor left gets its row redrawn */
    if (g_cur_row >= 0 && (g_cur_row != crow || g_cur_col != ccol)) g_shadow_ok[g_cur_row] = 0;
    for (i = 0; i < EDIT_ROWS; i++) {
        if (i < got) row_text(&lines[i], row);
        else { for (uint32_t j = 0; j < SCR_COLS; j++) row[j] = ' '; row[SCR_COLS] = 0; }
        if ((int)i == crow) {
            int same = g_shadow_ok[i];
            for (uint32_t j = 0; same && j < SCR_COLS; j++) if (g_shadow[i][j] != row[j]) same = 0;
            if (!same || g_cur_row != crow || g_cur_col != ccol) {
                g_shadow_ok[i] = 0;
                put_row((int)i, row);
                put_cells((int)i, ccol, &row[ccol], 1, BG, FG);
            }
        } else {
            put_row((int)i, row);
        }
    }
    g_cur_row = crow;
    g_cur_col = ccol;
    paint_status(&w);
}

static void paint_all(void)
{
    for (int i = 0; i < SCR_LINES; i++) g_shadow_ok[i] = 0;
    g_cur_row = -1;
    (void)tbx_fill_rect(g_win, 0, 0, WIN_R, WIN_B, BG);
    paint();
}

/* ---- files ------------------------------------------------------------------------ */
static void error(const char *msg, int back)
{
    g_err = msg;
    g_err_return = back;
    g_mode = M_ERROR;
}

static void offer_name(void)
{
    /* the current name is offered; the first character typed replaces it */
    uint32_t i = 0;
    const char *src = g_current[0] ? g_current + 3 : "";   /* skip "A:\" */
    for (; src[i] && i < sizeof g_name - 1u; i++) g_name[i] = src[i];
    g_name[i] = 0;
    g_nlen = i;
    g_fresh = 1;
}

/* g_name -> g_path ("NAME") and the shown "A:\NAME"; 0 = refused. */
static int take_name(void)
{
    uint32_t i, dot = 0, base = 0, ext = 0;
    if (g_nlen == 0u) return 0;
    for (i = 0; i < g_nlen; i++) {
        char c = upper(g_name[i]);
        int ok = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        if (!ok) return 0;
        if (c == '.') { if (dot) return 0; dot = 1; continue; }
        if (dot) ext++; else base++;
    }
    /* DOS 8.3 (Save p. 564, PDF 582: "up to 8 characters followed by an
     * optional period and up to 3 more characters") */
    if (base == 0u || base > 8u || ext > 3u) return 0;
    for (i = 0; i < g_nlen; i++) g_path[i] = upper(g_name[i]);
    g_path[i] = 0;
    g_shown[0] = 'A'; g_shown[1] = ':'; g_shown[2] = '\\';
    for (i = 0; i <= g_nlen; i++) g_shown[3 + i] = g_path[i];
    return 1;
}

static void after_save(void)
{
    scopy(g_current, g_shown);
    g_ed.dirty = 0;
    if (g_exiting) { g_mode = M_EXITQ; return; }
    g_mode = M_EDIT;
}

static void write_file(void)
{
    uint32_t n = 0, wn, pos = 0;
    int32_t h, rc = 0, closed;
    /* Flatten BEFORE create can truncate an existing file (the 123.EXE
     * preflight, initech-tdnl.91). */
    if (flatten(&n) != IW_OK) { error("ERROR: Document too large", M_EDIT); return; }
    wn = n;
#ifdef IWORD_MUT_SAVE_NOTHING
    /* MUTANT (Rule 6; test-flair-iword-mutant): the save writes no byte --
     * an empty file is left behind. NEVER real. */
    wn = 0;
#endif
    h = tbx_file_create(g_path);
    if (h < 0) { error("ERROR: File I/O error", M_SAVE_NAME); return; }
    while (pos < wn) {
        rc = tbx_file_write(h, g_bank[1u - g_live] + pos, wn - pos);
        if (rc <= 0 || (uint32_t)rc > wn - pos) break;
        pos += (uint32_t)rc;
    }
    closed = tbx_file_close(h);
    if (pos != wn || closed < 0) {
        /* Appendix E, PDF 763 */
        error("ERROR: Disk Full--Press Any Key to Continue", M_SAVE_NAME);
        return;
    }
    adopt(n, g_prefix_len);          /* the saved bytes are the new original */
    after_save();
}

static void save_named(void)
{
    int32_t h;
    if (!take_name()) { error("ERROR: Invalid file name", M_SAVE_NAME); return; }
    h = tbx_file_open(g_path, TBX_FILE_READ_ONLY);
    if (h >= 0) {
        (void)tbx_file_close(h);
        g_mode = M_REPLACE;
        return;
    }
    if (h != TBX_ERR_DOS(2)) { error("ERROR: File I/O error", M_SAVE_NAME); return; }
    write_file();
}

static void retrieve_named(void)
{
    uint8_t *buf;
    int32_t h, size, rc = 0, closed;
    uint32_t pos = 0;
    iw_wp5_t w;
    if (!take_name()) { error("ERROR: Invalid file name", M_GET_NAME); return; }
    /* Into a document the file's text is copied into the add buffer: make
     * the room FIRST, while the spare bank is still free to compact into
     * (the file is read into that bank next). */
    if (g_ed.doc.len != 0u) compact();
    buf = g_bank[1u - g_live];
    h = tbx_file_open(g_path, TBX_FILE_READ_ONLY);
    if (h < 0) { error("ERROR: File Not Found", M_GET_NAME); return; }
    size = tbx_file_seek(h, 0, TBX_FILE_FROM_END);
    if (size >= 0) rc = tbx_file_seek(h, 0, TBX_FILE_FROM_START);
    if (size >= 0 && rc >= 0 && (uint32_t)size <= BANK) {
        while (pos < (uint32_t)size) {
            rc = tbx_file_read(h, buf + pos, (uint32_t)size - pos);
            if (rc <= 0 || (uint32_t)rc > (uint32_t)size - pos) break;
            pos += (uint32_t)rc;
        }
    }
    closed = tbx_file_close(h);
    if (size < 0 || (uint32_t)size > BANK) { error("ERROR: Document too large", M_GET_NAME); return; }
    if (pos != (uint32_t)size || closed < 0) { error("ERROR: File Not Found", M_GET_NAME); return; }
    if (iw_wp5_read(buf, pos, &w) != IW_OK) { error("ERROR: Incompatible File Format", M_GET_NAME); return; }
    if (g_ed.doc.len == 0u) {
        /* onto a clear screen: the file IS the document now */
        g_ed.cur = 0;
        g_ed.goal = 0xFFFFFFFFu;
        g_ed.dirty = 0;
        adopt(pos, w.prefix_len);
        g_ed.dirty = 0;
        g_top = 0;
        scopy(g_current, g_shown);
    } else {
        /* into the document, at the cursor (Retrieve p. 553); the
         * on-screen document keeps its own name and prefix (p. 555) */
        uint32_t at = g_ed.cur;
        if (iw_doc_insert(&g_ed.doc, at, w.body, w.body_len) != IW_OK) {
            error("ERROR: Document too large", M_EDIT);
            return;
        }
        (void)iw_reflow_para(&g_ed.doc, at + w.body_len, g_ed.width);
        (void)iw_reflow_para(&g_ed.doc, at, g_ed.width);
        g_ed.dirty = 1;
    }
    g_mode = M_EDIT;
}

/* ---- the exit guard ------------------------------------------------------------- */
enum { FROM_F7, FROM_MAC };
static void request_exit(int from)
{
    if (g_mode != M_EDIT) return;                 /* one question at a time */
    if (from == FROM_MAC && !g_ed.dirty) { (void)tbx_exit(0); return; }
    g_mode = M_SAVEQ;
}

static void clear_document(void)
{
    doc_reset();
    g_current[0] = 0;
    g_mode = M_EDIT;
}

/* ---- keys ----------------------------------------------------------------------------- */
static void name_key(char ch, uint8_t vk)
{
    if (vk == VK_ENTER || ch == '\r') {
        if (g_mode == M_SAVE_NAME) save_named(); else retrieve_named();
        return;
    }
    if (vk == VK_BSPC || ch == 8) { g_fresh = 0; if (g_nlen) g_name[--g_nlen] = 0; return; }
    if (ch > ' ' && ch <= '~') {
        if (g_fresh) { g_nlen = 0; g_fresh = 0; }
        if (g_nlen < sizeof g_name - 1u) { g_name[g_nlen++] = ch; g_name[g_nlen] = 0; }
    }
}

static void prompt_key(char ch, uint8_t vk)
{
    if (g_mode == M_ERROR) { g_mode = g_err_return; return; }
    if (vk == VK_F1 || vk == VK_ESC || ch == 0x1B) {   /* Cancel */
        g_exiting = 0;
        g_mode = M_EDIT;
        return;
    }
    switch (g_mode) {
    case M_SAVE_NAME:
    case M_GET_NAME:
        name_key(ch, vk);
        break;
    case M_REPLACE:
        if (ch == 'y' || ch == 'Y') write_file();
        else g_mode = M_SAVE_NAME;              /* "prompted for a new filename" */
        break;
    case M_SAVEQ:
        if (ch == 'y' || ch == 'Y' || vk == VK_ENTER || ch == '\r') {
            g_exiting = 1;
            offer_name();
            g_mode = M_SAVE_NAME;
        } else if (ch == 'n' || ch == 'N') {
            g_mode = M_EXITQ;
        }
        break;
    case M_EXITQ:
        if (ch == 'y' || ch == 'Y') { (void)tbx_exit(0); return; }
        if (ch == 'n' || ch == 'N' || vk == VK_ENTER || ch == '\r') { g_exiting = 0; clear_document(); }
        break;
    default:
        g_mode = M_EDIT;
        break;
    }
}

static void edit_key(char ch, uint8_t vk, int32_t mods)
{
    int shift = (mods & (int32_t)FLAIR_EVT_MOD_SHIFT_KEY) != 0;
    int rc = IW_OK;
    if (ch == 0) {                       /* function and cursor keys */
        switch (vk) {
        case VK_F10:
            g_exiting = 0;
            if (shift) { g_nlen = 0; g_name[0] = 0; g_fresh = 0; g_mode = M_GET_NAME; }
            else { offer_name(); g_mode = M_SAVE_NAME; }
            break;
        case VK_F7:    request_exit(FROM_F7); break;
        case VK_UP:    iw_ed_up(&g_ed); break;
        case VK_DOWN:  iw_ed_down(&g_ed); break;
        case VK_LEFT:  iw_ed_left(&g_ed); break;
        case VK_RIGHT: iw_ed_right(&g_ed); break;
        case VK_HOME:  iw_ed_line_home(&g_ed); break;
        case VK_END:   iw_ed_line_end(&g_ed); break;
        case VK_DEL:   rc = edit(ED_DELETE, 0); break;
        default: break;
        }
    } else if (ch == '\r' || vk == VK_ENTER) {
        rc = edit(ED_ENTER, 0);
    } else if (ch == 8 || vk == VK_BSPC) {
        rc = edit(ED_BACKSPACE, 0);
    } else if (ch >= ' ' && ch <= '~') {
        rc = edit(ED_TYPE, (uint8_t)ch);
    }
    if (rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
}

static void key(int32_t message, int32_t mods)
{
    char ch = (char)(message & 0xFF);
    uint8_t vk = (uint8_t)((message >> 8) & 0xFF);
    g_caps = (mods & (int32_t)FLAIR_EVT_MOD_ALPHA_LOCK) != 0;
    if (g_mode == M_EDIT) edit_key(ch, vk, mods);
    else prompt_key(ch, vk);
    paint();
}

/* ---- the application ---------------------------------------------------------------- */
int tbx_app_main(void)
{
    if (tbx_register() != TBX_OK) return 1;
    doc_reset();
    (void)tbx_set_mbar(&mbar);
    g_win = tbx_new_window(WIN_L, WIN_T, WIN_R, WIN_B, (int32_t)TBX_GOAWAY_ASK);
    if (g_win <= 0) {
        (void)tbx_exit(1);
        return 1;
    }
    (void)tbx_set_wtitle(g_win, "InitechWord");
    return 0;
}

void tbx_app_event(const tbx_event_t *ev)
{
    switch (ev->what) {
    case updateEvt:
        paint_all();
        break;
    case keyDown:
    case autoKey:
        key(ev->message, ev->modifiers);
        break;
    case TBX_EVT_MENU:
        if (g_mode == M_EDIT) {
            if (ev->message == TBX_MENU_WORD(MENU_FILE, ITEM_EXIT)) request_exit(FROM_MAC);
            else if (ev->message == TBX_MENU_WORD(MENU_FILE, ITEM_SAVE)) { offer_name(); g_exiting = 0; g_mode = M_SAVE_NAME; }
            else if (ev->message == TBX_MENU_WORD(MENU_FILE, ITEM_RETRIEVE)) { g_nlen = 0; g_name[0] = 0; g_mode = M_GET_NAME; }
            paint();
        }
        break;
    case TBX_EVT_GOAWAY:
        request_exit(FROM_MAC);
        paint();
        break;
    default:
        break;
    }
}
