/*
 * os/i123/app/face_a.c -- 123.EXE, Initech 123's Face A: the classic Lotus
 * 1-2-3 Release 2.2 worksheet screen, drawn natively in a FLAIR window by a
 * disk-launched application (THE ARTIFACT).
 *
 * beads: initech-9u8w (I123 P1, Face A skeleton), initech-w96l (P1b: it ships
 *        as an InitechMZ .EXE on the data disk, launched from the Finder
 *        through the INT 81h Toolbox Gate -- never compiled into the kernel,
 *        epic initech-68iw).
 * Plan: docs/plans/INITECH-123-plan.md Sec 3.3 Face A (one native FlairApp
 *       window, the R2.2 chrome rendered NATIVELY in fixed character cells;
 *       ADR-0013 Sec 4: a native tenant, never a text console).
 *
 * THE SCREEN, laid out from the real 1-2-3 (../lotus123-decomp/goldens/
 * minted/MINT0x.shots/02_entered.png -- an 80 x 25 cell screen):
 *   line 0   control panel 1: "<addr>: <content>" at the left, the MODE
 *            INDICATOR ("READY") in reverse video at the right edge;
 *   line 1   control panel 2: the entry being typed (the edit line);
 *   line 2   control panel 3 (menu descriptions);
 *   line 3   the column border, reverse video: 4 blank characters over the
 *            row border, then each column's letters centred in its width;
 *   lines 4- the rows: the row number LEFT-aligned in the 4-character reverse
 *            border, then the worksheet area (whole columns only);
 *   last     the status line.
 * The cell pointer is the current cell's full column width in reverse video.
 * Reverse video is the shots' cyan bar; here NAVY with WHITE text, the
 * Toolbox's own highlight pair (spec/toolbox_gate.h Sec 7 tokens), on a WHITE
 * document field with BLACK text -- a Macintosh document window, as plan Sec
 * 3.3 puts it ("what a period Mac 1-2-3-alike would have rendered").
 * The window here is 77 x 24 cells (FLAIR_SCREEN 640 x 480 less the menu
 * bands and the window frame), so 19 rows show where the 25-line DOS screen
 * shows 20; the worksheet area keeps the full 72 characters (8 columns of 9).
 *
 * KEYS (the R2.2 READY/LABEL/VALUE/EDIT modes; MINT shots show READY):
 *   arrows move the pointer (scrolling the window); Home goes to A1;
 *   a printable key starts an entry -- 0-9 + - . ( @ # $ make it a VALUE,
 *   anything else a LABEL (' " ^ \ | typed first are its prefix, else the
 *   sheet's default, LABELFMT); Enter stores it; an arrow stores it and
 *   moves; Esc abandons it; Backspace edits it. A VALUE that is not a number
 *   (a formula is P2, bead initech-94ah) switches to EDIT, as 1-2-3 beeps
 *   into EDIT. '/' opens the menu; /File Retrieve and Save use the file gate.
 *   File > Quit (Ctrl-Q) quits.
 * NOT YET (stated): the other slash commands (visible refusal), the clock
 * on the status line, the "(F3)"/"[W12]" descriptors on line 1, a
 * formula's text on line 1 (P2).
 *
 * Ref: os/i123/core/i123.h (the model, the display rules -- graded against
 * the minted screenshots by test-i123-wk1-roundtrip); os/apps/tbx/tbx.h.
 * Freestanding C; x87 doubles (ADR-0009 Amendment DEC-01a). ASCII-clean.
 */
#include "tbx.h"
#include "i123.h"
#include "i123/wk1_format.h"

const char tbx_app_name[] = "123";

/* ---- geometry ------------------------------------------------------------ */
#define WIN_L        0
#define WIN_T        40          /* the first row below both menu bands */
#define WIN_R        640
#define WIN_B        480
#define CW           8           /* TBX_CELL_W */
#define CH           16          /* TBX_CELL_H */
#define SCR_COLS     77          /* 618-px content / 8 */
#define SCR_LINES    24          /* 397-px content / 16 */
#define X0           1           /* content-local left margin (618 - 616) / 2 */
#define BORDER_W     4           /* the row-number border */
#define AREA_CHARS   72          /* I123_SCREEN_COLS_CHARS: whole columns */
#define LINE_PANEL1  0
#define LINE_PANEL2  1
#define LINE_PANEL3  2
#define LINE_BORDER  3
#define LINE_ROW0    4
#define VIS_ROWS     (SCR_LINES - LINE_ROW0 - 1)   /* 19 */
#define LINE_STATUS  (SCR_LINES - 1)

#define MENU_FILE    129
#define ITEM_QUIT    1

/* PS/2 set-1 make codes (event.c cooks message = vkey << 8 | ascii) */
#define VK_ESC       0x01
#define VK_BSPC      0x0E
#define VK_ENTER     0x1C
#define VK_HOME      0x47
#define VK_UP        0x48
#define VK_LEFT      0x4B
#define VK_RIGHT     0x4D
#define VK_DOWN      0x50

/* ---- the menu bar (spec/toolbox_gate.h Sec 9) ---------------------------- */
static const tbx_menuitem_t file_items[] = {
    { "Quit", 0, 'Q', 0, 1, 0, { 0, 0, 0 } },
};
static const tbx_menuinfo_t menus[] = {
    { MENU_FILE, 0, "File", file_items, 1, 0 },
};
static const tbx_menubar_t mbar = { menus, 1, 1, 0 };

/* ---- state ----------------------------------------------------------------
 * The sheet store is private (BSS: the e_minalloc area, no file bytes); the
 * line buffer is handed to the Toolbox, so it lives in the image. */
#define SHEET_CELLS  2048u
#define SHEET_MEM    (SHEET_CELLS * (uint32_t)sizeof(i123_cell_t) + I123_CARRY_CAP + 32768u)
/* initech-tdnl.91: read into the other bank, then swap only on success.
 * Authored, no local reference for these capacity/transaction policies.
 * Gate spans accept BSS, never automatic objects on the shared kernel stack. */
static uint8_t       g_mem[2][SHEET_MEM];
static i123_sheet_t  g_sheet;
static uint32_t     g_bank;
static uint8_t      g_wk[65536];
static uint32_t     g_wk_len;
static char         g_name[49], g_path[TBX_FILE_PATH_MAX], g_current[TBX_FILE_PATH_MAX];
static uint32_t     g_nlen;
static int          g_menu_index, g_replacing;
static const char  *g_notice;
static char         g_status[SCR_COLS + 1];

static int32_t  g_win TBX_IN_IMAGE = 0;
static char     g_line[SCR_COLS + 1] TBX_IN_IMAGE = { 0 };
static char     g_edit[SCR_COLS + 1] TBX_IN_IMAGE = { 0 };
static uint32_t g_elen TBX_IN_IMAGE = 0;

enum { M_READY, M_LABEL, M_VALUE, M_EDIT, M_MAIN, M_FILE, M_GET, M_SAVE, M_REPLACE, M_NOTICE };
static int      g_mode TBX_IN_IMAGE = M_READY;
static const char *const MODE_TEXT[] = { "READY", "LABEL", "VALUE", " EDIT",
    "MENU", "MENU", "FILES", "FILES", "MENU", "ERROR" };

/* Ref: R2.2 Reference Manual p. 5-1 menu tree, pp. 1-13/14 placement/keys;
 * ../lotus123-decomp/goldens/lotus-123-2.2-installed-ref/extracted/installed/
 * 123.RI: File line 0x15f, Retrieve description 0x2ef8/prompt 0x2f43,
 * Save description 0x2f63/prompt 0x2f92. Save screen: MINT01.shots/
 * 03_save_prompt.png. A: replaces the reference's C: (our data drive).
 * Refusal/error/typed-file guidance and the drive substitution are authored,
 * no local reference. No advertised command silently succeeds. */
static const char MAIN_LINE[] = "Worksheet  Range  Copy  Move  File  Print  Graph  Data  System  Add-In  Quit";
static const char FILE_LINE[] = "Retrieve  Save  Combine  Xtract  Erase  List  Import  Directory  Admin";
static const char REPLACE_LINE[] = "Cancel  Replace  Backup";
static const char *const MAIN_NAMES[] = { "Worksheet", "Range", "Copy", "Move", "File",
    "Print", "Graph", "Data", "System", "Add-In", "Quit" };
static const char *const FILE_NAMES[] = { "Retrieve", "Save", "Combine", "Xtract", "Erase",
    "List", "Import", "Directory", "Admin" };
static const char *const REPLACE_NAMES[] = { "Cancel", "Replace", "Backup" };

/* ---- small helpers ------------------------------------------------------- */
static uint32_t slen(const char *s)
{
    uint32_t n = 0;
    while (s[n]) n++;
    return n;
}

static void put_line(int line, int col, const char *s, uint32_t n, uint32_t fg, uint32_t bg)
{
    uint32_t i;
    if (n > SCR_COLS) n = SCR_COLS;
    for (i = 0; i < n; i++) g_line[i] = s[i];
    g_line[n] = '\0';
    (void)tbx_draw_cells(g_win, X0 + col * CW, line * CH, g_line, fg, bg);
}

static void pad_into(char *dst, uint32_t width, const char *src)
{
    uint32_t i = 0;
    for (; src[i] && i < width; i++) dst[i] = src[i];
    for (; i < width; i++) dst[i] = ' ';
    dst[width] = '\0';
}

static void copy_text(char *dst, const char *src)
{ uint32_t i = 0; do { dst[i] = src[i]; } while (src[i++]); }

static void panel_line(int line, const char *s)
{
    char tmp[SCR_COLS + 1];
    pad_into(tmp, SCR_COLS, s);
    put_line(line, 0, tmp, SCR_COLS, TBX_COLOR_BLACK, TBX_COLOR_WHITE);
}

static void paint_command(void)
{
    if (g_mode == M_MAIN || g_mode == M_FILE || g_mode == M_REPLACE) {
        const char *const *names = g_mode == M_MAIN ? MAIN_NAMES : g_mode == M_FILE ? FILE_NAMES : REPLACE_NAMES;
        const char *line = g_mode == M_MAIN ? MAIN_LINE : g_mode == M_FILE ? FILE_LINE : REPLACE_LINE;
        uint32_t x = 0;
        panel_line(LINE_PANEL2, line);
        for (int i = 0; i < g_menu_index; i++) x += slen(names[i]) + 2;
        put_line(LINE_PANEL2, (int)x, names[g_menu_index], slen(names[g_menu_index]), TBX_COLOR_WHITE, TBX_COLOR_NAVY);
        const char *desc = "Command not implemented";
        if (g_mode == M_MAIN && g_menu_index == 4) desc = FILE_LINE;
        if (g_mode == M_MAIN && g_menu_index == 10) desc = "End the 1-2-3 session";
        if (g_mode == M_FILE && g_menu_index == 0) desc = "Erase the current worksheet from memory and display the selected worksheet";
        if (g_mode == M_FILE && g_menu_index == 1) desc = "Store the entire worksheet in a worksheet file";
        if (g_mode == M_REPLACE && g_menu_index == 0) desc = "Cancel command -- Leave existing file on disk intact";
        if (g_mode == M_REPLACE && g_menu_index == 1) desc = "Update the file on disk with the current file"; /* 123.RI 0xa9d */
        panel_line(LINE_PANEL3, desc);
    } else if (g_mode == M_GET || g_mode == M_SAVE) {
        const char *prompt = g_mode == M_GET ? "Name of file to retrieve: " : "Enter name of file to save: ";
        const char *name = g_nlen ? g_name : g_mode == M_SAVE && g_current[0] ? g_current : "A:\\*.wk1";
        char tmp[SCR_COLS + 1]; uint32_t n = 0;
        for (uint32_t i = 0; prompt[i] && n < SCR_COLS; i++) tmp[n++] = prompt[i];
        for (uint32_t i = 0; name[i] && n < SCR_COLS; i++) tmp[n++] = name[i];
        if (g_nlen && n < SCR_COLS) tmp[n++] = '_';
        tmp[n] = 0; panel_line(LINE_PANEL2, tmp);
        panel_line(LINE_PANEL3, "Type a file name; Enter accepts, Esc backs up");
    } else {
        panel_line(LINE_PANEL2, g_notice);
        panel_line(LINE_PANEL3, "Press Esc to return to the worksheet");
    }
}

/* ---- the view ------------------------------------------------------------- */
static uint32_t area_cols(void)
{
    return i123_cols_fitting(&g_sheet, i123_view_left(&g_sheet), AREA_CHARS);
}

/* Character offset of column `col` in the worksheet area, or -1 off-view. */
static int col_x(uint16_t col)
{
    uint16_t left = i123_view_left(&g_sheet);
    uint32_t x = 0, n = area_cols();
    if (col < left || col >= left + n) return -1;
    for (uint16_t c = left; c < col; c++) x += i123_colwidth(&g_sheet, c);
    return (int)x;
}

/* Keep the pointer on screen; returns 1 when the view moved. */
static int follow_pointer(void)
{
    uint16_t cc = i123_cursor_col(&g_sheet), cr = i123_cursor_row(&g_sheet);
    uint16_t left = i123_view_left(&g_sheet), top = i123_view_top(&g_sheet);
    uint16_t nl = left, nt = top;
    if (cr < nt) nt = cr;
    if (cr >= nt + VIS_ROWS) nt = (uint16_t)(cr - (VIS_ROWS - 1));
    if (cc < nl) nl = cc;
    while (cc >= nl + i123_cols_fitting(&g_sheet, nl, AREA_CHARS)) nl++;
    if (nl == left && nt == top) return 0;
    i123_set_view(&g_sheet, nl, nt);
    return 1;
}

/* ---- painting --------------------------------------------------------------- */
static void paint_panel(void)
{
    char buf[SCR_COLS + 1], addr[16], content[96];
    const i123_cell_t *c;
    uint32_t n, k;
    const char *mode = MODE_TEXT[g_mode];
    uint32_t mlen = slen(mode);

    /* line 1: "A1: 'Hello" ... "READY" (the shots: address, colon, space) */
    n = i123_addr_text(i123_cursor_col(&g_sheet), i123_cursor_row(&g_sheet), addr);
    for (k = 0; k < n; k++) buf[k] = addr[k];
    buf[n++] = ':';
    buf[n++] = ' ';
    c = i123_find(&g_sheet, i123_cursor_col(&g_sheet), i123_cursor_row(&g_sheet));
    i123_cell_content(&g_sheet, c, content, sizeof content);
    for (k = 0; content[k] && n < SCR_COLS - mlen - 1u; k++) buf[n++] = content[k];
    buf[n] = '\0';
    {
        char tmp[SCR_COLS + 1];
        pad_into(tmp, SCR_COLS - mlen, buf);
        put_line(LINE_PANEL1, 0, tmp, SCR_COLS - mlen, TBX_COLOR_BLACK, TBX_COLOR_WHITE);
    }
    put_line(LINE_PANEL1, (int)(SCR_COLS - mlen), mode, mlen, TBX_COLOR_WHITE, TBX_COLOR_NAVY);
}

static void paint_edit_line(void)
{
    if (g_mode >= M_MAIN) { paint_command(); return; }
    char tmp[SCR_COLS + 1], src[SCR_COLS + 1];
    uint32_t i;
    for (i = 0; i < g_elen && i < SCR_COLS - 1u; i++) src[i] = g_edit[i];
    if (g_mode != M_READY) src[i++] = '_';          /* the edit cursor */
    src[i] = '\0';
    pad_into(tmp, SCR_COLS, src);
    put_line(LINE_PANEL2, 0, tmp, SCR_COLS, TBX_COLOR_BLACK, TBX_COLOR_WHITE);
    panel_line(LINE_PANEL3, "");
}

static void paint_border(void)
{
    char tmp[SCR_COLS + 1];
    uint32_t x = BORDER_W, n = area_cols();
    uint16_t left = i123_view_left(&g_sheet);
    for (uint32_t i = 0; i < SCR_COLS; i++) tmp[i] = ' ';
    tmp[SCR_COLS] = '\0';
    for (uint32_t k = 0; k < n; k++) {
        char name[4];
        uint32_t w = i123_colwidth(&g_sheet, (uint16_t)(left + k));
        uint32_t nl = i123_col_name((uint16_t)(left + k), name);
        uint32_t at = x + (w > nl ? (w - nl) / 2u : 0u);
        for (uint32_t j = 0; j < nl && at + j < SCR_COLS; j++) tmp[at + j] = name[j];
        x += w;
    }
    put_line(LINE_BORDER, 0, tmp, SCR_COLS, TBX_COLOR_WHITE, TBX_COLOR_NAVY);
}

static void paint_row(uint32_t vis)
{
    char num[8], tmp[SCR_COLS + 1], row[AREA_CHARS + 1];
    uint16_t r = (uint16_t)(i123_view_top(&g_sheet) + vis);
    int line = LINE_ROW0 + (int)vis;
    uint32_t n = 0;
    {
        char a[16];
        uint32_t k = i123_addr_text(0, r, a);   /* "A<row>": skip the letter */
        for (uint32_t j = 1; j < k; j++) num[n++] = a[j];
        num[n] = '\0';
    }
    pad_into(tmp, BORDER_W, num);
    put_line(line, 0, tmp, BORDER_W, TBX_COLOR_WHITE, TBX_COLOR_NAVY);
    i123_render_row(&g_sheet, r, i123_view_left(&g_sheet), AREA_CHARS, row);
    pad_into(tmp, SCR_COLS - BORDER_W, row);
    put_line(line, BORDER_W, tmp, SCR_COLS - BORDER_W, TBX_COLOR_BLACK, TBX_COLOR_WHITE);
}

static void paint_pointer(void)
{
    uint16_t cc = i123_cursor_col(&g_sheet), cr = i123_cursor_row(&g_sheet);
    uint32_t w = i123_colwidth(&g_sheet, cc), spill;
    int x = col_x(cc);
    char text[256];
    if (x < 0 || cr < i123_view_top(&g_sheet)) return;
    i123_cell_text(&g_sheet, i123_find(&g_sheet, cc, cr), w, text, &spill);
    put_line(LINE_ROW0 + (int)(cr - i123_view_top(&g_sheet)), BORDER_W + x, text, w,
             TBX_COLOR_WHITE, TBX_COLOR_NAVY);
}

static void paint_rows(void)
{
    for (uint32_t v = 0; v < VIS_ROWS; v++) paint_row(v);
    paint_pointer();
}

static void paint_all(void)
{
    char blank[SCR_COLS + 1];
    (void)tbx_fill_rect(g_win, 0, 0, WIN_R, WIN_B, TBX_COLOR_WHITE);
    paint_panel();
    paint_edit_line();
    pad_into(blank, SCR_COLS, "");
    paint_border();
    paint_rows();
    panel_line(LINE_STATUS, g_status);
}

static void command_paint(void) { paint_panel(); paint_edit_line(); }
static void notice(const char *s) { g_notice = s; g_mode = M_NOTICE; command_paint(); }
static void ready(void)
{ g_mode = M_READY; g_elen = 0; g_edit[0] = 0; g_nlen = 0; command_paint(); }
static void file_error(int32_t rc)
{
    notice(rc == TBX_ERR_DOS(2) ? "File not found" :
           rc == TBX_ERR_DOS(3) ? "Path not found" : "File I/O error");
}
static void file_status(const char *verb)
{
    uint32_t n = 0;
    while (verb[n]) { g_status[n] = verb[n]; n++; }
    for (uint32_t i = 0; g_path[i] && n < SCR_COLS; i++) g_status[n++] = g_path[i];
    g_status[n] = 0;
}
static int file_name(void)
{
    const char *s = g_nlen ? g_name : g_current;
    uint32_t n = 0, dot = 0;
    if (!s[0]) { notice("Enter a file name"); return 0; }
    while (s[n]) {
        if (s[n] == '*' || s[n] == '?' || s[n] == ' ') { notice("Invalid file name"); return 0; }
        if (s[n] == '\\' || s[n] == '/' || s[n] == ':') dot = 0;
        if (s[n] == '.') dot = 1;
        g_path[n] = s[n]; n++;
    }
    /* Ref: Reference Manual p. 5-21 (default .WK1 extension). */
    if (!dot) { const char *ext = ".wk1"; for (uint32_t i = 0; i < 4; i++) g_path[n++] = ext[i]; }
    g_path[n] = 0; return 1;
}
static void retrieve(void)
{
    if (!file_name()) return;
    int32_t h = tbx_file_open(g_path, TBX_FILE_READ_ONLY);
    if (h < 0) { file_error(h); return; }
    int32_t n = tbx_file_seek(h, 0, TBX_FILE_FROM_END);
    int32_t rc = n < 0 ? n : tbx_file_seek(h, 0, TBX_FILE_FROM_START);
    uint32_t pos = 0;
    if (rc >= 0 && n <= (int32_t)sizeof g_wk) {
        while (pos < (uint32_t)n) {
            rc = tbx_file_read(h, g_wk + pos, (uint32_t)n - pos);
            if (rc <= 0 || (uint32_t)rc > (uint32_t)n-pos) break;
            pos += (uint32_t)rc;
        }
    }
    int32_t closed = tbx_file_close(h);
    if (n > (int32_t)sizeof g_wk) { notice("File too large"); return; }
    if (rc < 0 || closed < 0) { file_error(rc < 0 ? rc : closed); return; }
    if (pos != (uint32_t)n) { notice("File read incomplete"); return; }
    i123_sheet_t next;
#ifdef I123_MUT_GET_CLOBBER
    /* Mutant: malformed Retrieve destroys the active sheet before refusal. */
    (void)i123_wk1_read(&g_sheet, g_wk, (uint32_t)n);
#endif
    rc = i123_sheet_init(&next, g_mem[1-g_bank], sizeof g_mem[0], SHEET_CELLS);
    if (rc == I123_OK) rc = i123_wk1_read(&next, g_wk, (uint32_t)n);
    if (rc != I123_OK) { notice(rc == I123_E_FULL ? "Not enough worksheet memory" : "Not a valid WK1 file"); return; }
    /* Manual p. 5-19: retrieved data/window settings replace the worksheet.
     * Authored transaction: failures above leave the old bank intact. */
    g_sheet = next; g_bank = 1-g_bank;
    copy_text(g_current, g_path); file_status("Retrieved "); ready(); paint_all();
}
static void write_sheet(void)
{
    int32_t h = tbx_file_create(g_path);
    if (h < 0) { file_error(h); return; }
    uint32_t pos = 0; int32_t rc = 0;
    while (pos < g_wk_len) {
        rc = tbx_file_write(h, g_wk + pos, g_wk_len-pos);
        if (rc <= 0 || (uint32_t)rc > g_wk_len-pos) break;
        pos += (uint32_t)rc;
    }
    int32_t closed = tbx_file_close(h);
    if (pos != g_wk_len || closed < 0) {
        if (!g_replacing) (void)tbx_file_delete(g_path);
        file_error(rc < 0 ? rc : closed); return;
    }
    copy_text(g_current, g_path); file_status("Saved "); ready(); paint_all();
}
static void save(void)
{
    if (!file_name()) return;
    /* Preflight codec/capacity before CREATE can truncate. Authored policy. */
#ifdef I123_MUT_SAVE_EMPTY
    i123_sheet_t empty = g_sheet; empty.ncells = 0;
    int rc = i123_wk1_write(&empty, g_wk, sizeof g_wk, &g_wk_len);
#else
    int rc = i123_wk1_write(&g_sheet, g_wk, sizeof g_wk, &g_wk_len);
#endif
    if (rc != I123_OK) { notice("Worksheet too large to save"); return; }
    int32_t h = tbx_file_open(g_path, TBX_FILE_READ_ONLY);
    if (h >= 0) {
        rc = tbx_file_close(h);
        if (rc < 0) { file_error(rc); return; }
        /* Reference Manual p. 5-22: Cancel, Replace, Backup. */
        g_replacing = 1; g_mode = M_REPLACE; g_menu_index = 0; command_paint();
    } else if (h == TBX_ERR_DOS(2)) { g_replacing = 0; write_sheet(); }
    else file_error(h);
}
static void unsupported(void)
{
#ifdef I123_MUT_COMMAND_SILENT
    ready();
#else
    notice("Command not implemented");
#endif
}
static void menu_select(void)
{
    if (g_mode == M_MAIN && g_menu_index == 4) { g_mode = M_FILE; g_menu_index = 0; }
    else if (g_mode == M_MAIN && g_menu_index == 10) { (void)tbx_exit(0); return; }
    else if (g_mode == M_FILE && g_menu_index <= 1) {
        g_mode = g_menu_index == 0 ? M_GET : M_SAVE; g_nlen = 0; g_name[0] = 0;
    } else if (g_mode == M_REPLACE && g_menu_index == 0) { ready(); return; }
    else if (g_mode == M_REPLACE && g_menu_index == 1) { write_sheet(); return; }
    else { unsupported(); return; }
    command_paint();
}
static void command_key(char ch, uint8_t vk)
{
    if (vk == VK_ESC || ch == 0x1b) {
        if (g_mode == M_FILE) { g_mode = M_MAIN; g_menu_index = 4; }
        else if (g_mode == M_GET || g_mode == M_SAVE) { g_mode = M_FILE; g_menu_index = 0; }
        else if (g_mode == M_REPLACE) { g_mode = M_SAVE; g_nlen = 0; }
        else { ready(); return; }
        command_paint(); return;
    }
    if (g_mode == M_NOTICE) return;
    if (g_mode == M_GET || g_mode == M_SAVE) {
        if (vk == VK_ENTER || ch == '\r') { if (g_mode == M_GET) retrieve(); else save(); return; }
        if (vk == VK_BSPC || ch == 8) { if (g_nlen) g_nlen--; }
        else if (ch >= ' ' && ch <= '~') {
            if (g_nlen >= sizeof g_name-1) { notice("File name too long"); return; }
            g_name[g_nlen++] = ch;
        }
        g_name[g_nlen] = 0; command_paint(); return;
    }
    uint32_t count = g_mode == M_MAIN ? 11 : g_mode == M_FILE ? 9 : 3;
    const char *const *names = g_mode == M_MAIN ? MAIN_NAMES : g_mode == M_FILE ? FILE_NAMES : REPLACE_NAMES;
    if (vk == VK_ENTER || ch == '\r') { menu_select(); return; }
    if (vk == VK_LEFT) g_menu_index = (g_menu_index + (int)count - 1) % (int)count;
    else if (vk == VK_RIGHT || ch == ' ') g_menu_index = (g_menu_index + 1) % (int)count;
    else if (vk == VK_HOME) g_menu_index = 0;
    else {
        if (ch >= 'a' && ch <= 'z') ch -= 'a'-'A';
        for (uint32_t i = 0; i < count; i++) {
            if (names[i][0] == ch) { g_menu_index = (int)i; menu_select(); return; }
        }
    }
    command_paint();
}

/* Repaint after the pointer moved from (oc,or): the old row loses its
 * pointer, the view may have scrolled. */
static void moved(uint16_t oc, uint16_t orow)
{
    (void)oc;
    if (follow_pointer()) {
        paint_border();
        paint_rows();
    } else {
        uint16_t top = i123_view_top(&g_sheet);
        if (orow >= top && orow < top + VIS_ROWS) paint_row((uint32_t)(orow - top));
        paint_pointer();
    }
    paint_panel();
}

/* ---- entry ----------------------------------------------------------------- */
static int is_value_start(char ch)
{
    return (ch >= '0' && ch <= '9') || ch == '+' || ch == '-' || ch == '.' ||
           ch == '(' || ch == '@' || ch == '#' || ch == '$';
}

static int is_prefix(char ch)
{
    return ch == '\'' || ch == '"' || ch == '^' || ch == '\\' || ch == '|';
}

/* Store the entry in the current cell; 0 on success (back to READY). */
static int commit(void)
{
    uint16_t cc = i123_cursor_col(&g_sheet), cr = i123_cursor_row(&g_sheet);
    uint16_t top = i123_view_top(&g_sheet);
    int rc;
    if (g_mode == M_LABEL) {
        if (g_elen > 0u && is_prefix(g_edit[0]))
            rc = i123_set_label(&g_sheet, cc, cr, (uint8_t)g_edit[0], g_edit + 1, g_elen - 1u);
        else
            rc = i123_set_label(&g_sheet, cc, cr, g_sheet.labelfmt, g_edit, g_elen);
    } else {
        double x;
        rc = i123_parse_value(g_edit, g_elen, &x);
        if (rc == I123_OK) rc = i123_set_number(&g_sheet, cc, cr, x);
    }
    if (rc != I123_OK) {
        g_mode = M_EDIT;   /* 1-2-3 beeps and drops into EDIT */
        paint_panel();
        paint_edit_line();
        return 1;
    }
    g_mode = M_READY;
    g_elen = 0;
    g_edit[0] = '\0';
    if (cr >= top && cr < top + VIS_ROWS) paint_row((uint32_t)(cr - top));
    paint_pointer();
    paint_panel();
    paint_edit_line();
    return 0;
}

static void move_by(int dc, int dr)
{
    uint16_t oc = i123_cursor_col(&g_sheet), orow = i123_cursor_row(&g_sheet);
    int nc = (int)oc + dc, nr = (int)orow + dr;
#ifdef I123_MUT_ARROWS_DEAD
    /* MUTANT (Rule 6; test-flair-i123-mutant): the pointer never moves, so
     * every typed entry lands in A1. NEVER in a real build. */
    nc = (int)oc; nr = (int)orow;
#endif
    if (nc < 0) nc = 0;
    if (nr < 0) nr = 0;
    if (nc >= (int)I123_MAX_COLS) nc = (int)I123_MAX_COLS - 1;
    if (nr >= (int)I123_MAX_ROWS) nr = (int)I123_MAX_ROWS - 1;
    i123_set_cursor(&g_sheet, (uint16_t)nc, (uint16_t)nr);
    moved(oc, orow);
}

static void key(int32_t message)
{
    char ch = (char)(message & 0xFF);
    uint8_t vk = (uint8_t)((message >> 8) & 0xFF);
    int dc = 0, dr = 0, arrow = 0;

    if (g_mode >= M_MAIN) { command_key(ch, vk); return; }

    if (ch == 0) {
        if (vk == VK_UP)    { dr = -1; arrow = 1; }
        if (vk == VK_DOWN)  { dr = 1;  arrow = 1; }
        if (vk == VK_LEFT)  { dc = -1; arrow = 1; }
        if (vk == VK_RIGHT) { dc = 1;  arrow = 1; }
        if (vk == VK_HOME && g_mode == M_READY) {
            uint16_t oc = i123_cursor_col(&g_sheet), orow = i123_cursor_row(&g_sheet);
            i123_set_cursor(&g_sheet, 0, 0);
            moved(oc, orow);
            return;
        }
    }
    if (arrow) {
        if (g_mode == M_LABEL || g_mode == M_VALUE) {
            if (commit() != 0) return;   /* a bad value stays in EDIT */
        }
        if (g_mode == M_READY) move_by(dc, dr);
        return;
    }
    if (vk == VK_ESC || ch == 0x1B) {
        if (g_mode != M_READY) {
            g_mode = M_READY;
            g_elen = 0;
            g_edit[0] = '\0';
            paint_panel();
            paint_edit_line();
        }
        return;
    }
    if (g_mode == M_READY) {
        if (ch == '/' || ch == '<') { g_mode = M_MAIN; g_menu_index = 0; command_paint(); return; }
        if (ch < ' ' || ch > '~') return;
        g_mode = is_value_start(ch) ? M_VALUE : M_LABEL;
        g_edit[0] = ch;
        g_elen = 1;
        paint_panel();
        paint_edit_line();
        return;
    }
    if (vk == VK_ENTER || ch == '\r') {
        (void)commit();
        return;
    }
    if (vk == VK_BSPC || ch == 0x08) {
        if (g_elen > 0u) g_elen--;
        paint_edit_line();
        return;
    }
    if (ch >= ' ' && ch <= '~' && g_elen < SCR_COLS - 2u) {
        g_edit[g_elen++] = ch;
        paint_edit_line();
    }
}

/* ---- the application ------------------------------------------------------- */
int tbx_app_main(void)
{
    if (tbx_register() != TBX_OK) return 1;
    if (i123_sheet_init(&g_sheet, g_mem[0], (uint32_t)sizeof g_mem[0], SHEET_CELLS) != I123_OK) {
        (void)tbx_exit(1);
        return 1;
    }
    (void)tbx_set_mbar(&mbar);
    g_win = tbx_new_window(WIN_L, WIN_T, WIN_R, WIN_B, 1);
    if (g_win <= 0) {
        (void)tbx_exit(1);
        return 1;
    }
    (void)tbx_set_wtitle(g_win, "Initech 123");
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
        key(ev->message);
        break;
    case TBX_EVT_MENU:
        if (ev->message == TBX_MENU_WORD(MENU_FILE, ITEM_QUIT)) (void)tbx_exit(0);
        break;
    default:
        break;
    }
}
