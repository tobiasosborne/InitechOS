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
 * The window opens at 76 x 24 cells (FLAIR_SCREEN 640 x 480 less the menu
 * bands and the window frame), so 19 rows show where the 25-line DOS screen
 * shows 20; the worksheet area keeps the full 72 characters (8 columns of 9).
 * B12 (bead initech-89rc, audit L018): the window can be grown and shrunk,
 * so that screen is the LARGEST one; the face lays itself out for the
 * window's ACTUAL content (THE GEOMETRY below), as the DOS screen is laid
 * out for the display it has: the status line is the last whole line, the
 * mode indicator ends at the last whole character, the area shows the whole
 * columns and rows that fit, and the pointer is always among them.
 *
 * KEYS (the R2.2 READY/LABEL/VALUE/EDIT modes; MINT shots show READY):
 *   arrows move the pointer (scrolling the window); Home goes to A1;
 *   PgUp/PgDn move a screen of rows, Tab/Shift-Tab a screen of columns; F5
 *   (GOTO) jumps to a typed address; F2 (EDIT) edits the current cell (bead
 *   initech-69mz, B11; "moving the pointer" below);
 *   a printable key starts an entry -- 0-9 + - . ( @ # $ make it a VALUE,
 *   anything else a LABEL (' " ^ \ | typed first are its prefix, else the
 *   sheet's default, LABELFMT); Enter stores it; an arrow stores it and
 *   moves; Esc abandons it; Backspace edits it. A VALUE is a number or a
 *   FORMULA (bead initech-94ah: compiled by the core's formula engine and
 *   recalculated by the sheet's /Worksheet Global Recalculation settings);
 *   one 1-2-3 would refuse switches to EDIT, as 1-2-3 beeps into EDIT. F9
 *   (CALC) recalculates. '/' opens the menu; /File Retrieve and Save use the
 *   file gate.
 *   File > Quit (Ctrl-Q), the close box and /Quit Yes all pass THE ONE EXIT
 *   GUARD (request_quit, bead initech-fvnj): a clean sheet quits, unsaved
 *   work asks first (see "THE EXIT GUARD" below).
 * THE STATUS LINE carries, at its right end, the CALC and CIRC status
 * indicators (manual p. 1-5..1-6, PDF 21-22: "The status line ... the
 * status indicators"; CALC "Formulas ... need to be recalculated; press
 * CALC (F9)", CIRC a circular reference) as highlighted words; their exact
 * column is [inferred] (the look pass is bead initech-68iw.1).
 * P3 (initech-uqah): the RI-sourced slash tree, range formats/labels/names,
 * erase/protection/value/transpose, Copy/Move, F2 and F5. Every deferred leaf
 * refuses visibly; spec/i123/era_shortfalls.md records exact scope. Menus
 * load from APPS/123.MNU to keep the executable under the V1 64 KiB cap.
 * The clock and "(F3)"/"[W12]" descriptors remain deferred.
 *
 * Ref: os/i123/core/i123.h (the model, the display rules -- graded against
 * the minted screenshots by test-i123-wk1-roundtrip); os/apps/tbx/tbx.h.
 * Freestanding C; x87 doubles (ADR-0009 Amendment DEC-01a). ASCII-clean.
 */
#include "tbx.h"
#include "tbx_save.h"   /* initech-mcep: the atomic document save */
#include "i123.h"
#include "menu.h"
#include "i123/wk1_format.h"
#include "cpanels_record.h"

const char tbx_app_name[] = "123";

/* ---- geometry ------------------------------------------------------------ */
#define WIN_L        0
#define WIN_T        40          /* the first row below both menu bands */
#define WIN_R        640
#define WIN_B        480
#define CW           8           /* TBX_CELL_W */
#define CH           16          /* TBX_CELL_H */
/* RE-KEYED 2026-10-16 (bead initech-tdnl.51, stated): the document content
 * starts inside the left body rail (frame.left + 6, CalcDocContentRect), so
 * the 640-px window's content is 613 px wide (was 618): 76 whole cells (was
 * 77). X0 stays the 1-px left margin (613 - 608 = 5 spare), so every
 * content-local x of the face is unchanged except the right-anchored items
 * (the MODE indicator, CIRC/CALC), which move one cell left. */
#define SCR_COLS     76          /* 613-px content / 8: the widest face */
#define SCR_LINES    24          /* 397-px content / 16: the tallest   */
#define X0           1           /* content-local left margin (613 - 608 = 5 spare) */
#define BORDER_W     4           /* the row-number border */
#define LINE_PANEL1  0
#define LINE_PANEL2  1
#define LINE_PANEL3  2
#define LINE_BORDER  3
#define LINE_ROW0    4

/* ---- THE GEOMETRY (B12, bead initech-89rc) ----------------------------------
 * ONE geometry for paint, pointer-follow and hit-test: g_cols whole
 * characters by g_lines whole lines of the window's content NOW, read through
 * the Toolbox (spec/toolbox_gate.h Sec 3 CONTENT_SIZE -- the tenant's
 * portRect; the shell grows the window and the updateEvt follows, MTE
 * Listing 4-13/4-14) on every updateEvt. Character c spans content x 1 + 8c
 * .. 8 + 8c and line L spans y 16L .. 16L + 15 (the 8 x 16 cells, Sec 7a), so
 * a W x H content holds (W - X0) / 8 characters and H / 16 lines. From them,
 * by the R2.2 screen (manual pp. 1-3/1-4): the area is the characters right
 * of the row border, the rows are the lines between the column border and
 * the status line, the status line is the last line. Bounds (authored): at
 * most the 76 x 24 face the buffers hold (the largest content the desktop
 * allows); at least GEO_MIN_COLS characters -- the smallest frame the Window
 * Manager allows, FLAIR_CHROME_WINDOW_MIN_W 96 px, has a 69-px content of 8
 * -- and one row, so the arithmetic below never underflows (line one keeps
 * room for "A1:" and a 5-letter mode); a smaller window just clips. */
#define GEO_MIN_COLS 8u
static uint8_t g_cols = SCR_COLS, g_lines = SCR_LINES;
static uint32_t cols(void) { return g_cols; }
static uint32_t vis_rows(void) { return g_lines - (LINE_ROW0 + 1u); }
#define AREA_CHARS   (cols() - BORDER_W)
#define VIS_ROWS     vis_rows()
#define LINE_STATUS  ((int)(LINE_ROW0 + vis_rows()))

/* Ref: Lotus 1-2-3 R2.2 Reference Manual pp. 1-16/1-17 (PDF 32/33):
 * an entry may contain 240 characters, independent of the screen width. */
#define ENTRY_MAX    240u
#ifdef I123_MUT_ENTRY_75
/* Rule 6 / test-i123-entry-mutant: restore the former 75-character cap. */
#define ENTRY_LIMIT  75u
#else
#define ENTRY_LIMIT  ENTRY_MAX
#endif

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
#define VK_F9        0x43        /* CALC (manual p. 12-32, PDF 276) */
/* B11 (initech-69mz): Ref manual pp. 1-9/1-10/1-11 (PDF 25..27). Tab is
 * cooked '\t' (event.c); PgUp/PgDn arrive E0-prefixed, ascii 0. */
#define VK_TAB       0x0F
#define VK_F2        0x3C        /* EDIT */
#define VK_F5        0x3F        /* GOTO */
#define VK_PGUP      0x49
#define VK_PGDN      0x51

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
/* bead initech-94ah: the recalculation workspace (one, for whichever bank is
 * live) and the spare context @D* criteria formulas evaluate in. */
static uint8_t       g_calc_mem[I123_CALC_BYTES(SHEET_CELLS)];
static uint8_t       g_nest_mem[I123_CALC_BYTES(16u)];
static i123_calc_t   g_cx, g_cx_nest;
/* TBX_CLOCK writes only to module/BSS, never the shared kernel stack.
 * Ref: tbx.h clock contract; Lotus Reference p. 13-46 (@NOW). */
#ifndef I123_MUT_CLOCK_MISSING
static cp_time_t     g_clock;
#endif

static void refresh_clock(void)
{
#ifdef I123_MUT_CLOCK_FROZEN
    if (g_cx.have_now) return;
#endif
    g_cx.have_now = 0;
#ifndef I123_MUT_CLOCK_MISSING
    if (tbx_get_clock(&g_clock) == TBX_OK)
        i123_calc_clock(&g_cx, g_clock.year, g_clock.month, g_clock.day,
                        g_clock.hour, g_clock.minute, g_clock.second);
#endif
}
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
/* Private workspace: put_line copies through g_line before drawing. Ref:
 * tbx.h memory rule / Toolbox Gate Sec 8: the loader zeroes private .bss. */
static char     g_edit[ENTRY_MAX + 1];
static uint32_t g_elen TBX_IN_IMAGE = 0;

enum { M_READY, M_LABEL, M_VALUE, M_EDIT, M_MAIN, M_FILE, M_GET, M_SAVE, M_REPLACE, M_QUIT, M_NOTICE, M_RANGE };
static int      g_mode TBX_IN_IMAGE = M_READY;
static const char MODE_TEXT[][6] = { "READY", "LABEL", "VALUE", " EDIT",
    "MENU", "MENU", "FILES", "FILES", "MENU", "MENU", "ERROR", "POINT" };
/* B11: an error 1-2-3 shows on the STATUS line (manual p. 1-5, PDF 21: "in
 * place of the date-and-time indicator"; ESC or ENTER clears it). */
static const char *g_error;

/* THE EXIT GUARD's state (bead initech-fvnj). g_dirty: a committed edit not
 * yet in a file -- set by every successful commit, cleared ONLY by a
 * successful Save or Retrieve. g_asking: the alert is up (one question at a
 * time). g_quit_after_save: the alert's Save started the /File Save flow, so
 * its success ends the session; any other way out of the flow (Esc, Replace
 * > Cancel, an error notice) drops it and the sheet stays open and dirty. */
static uint8_t  g_dirty, g_asking, g_quit_after_save;

/* Ref: R2.2 Reference Manual p. 5-1 menu tree, pp. 1-13/14 placement/keys;
 * ../lotus123-decomp/goldens/lotus-123-2.2-installed-ref/extracted/installed/
 * 123.RI: File line 0x15f, Retrieve description 0x2ef8/prompt 0x2f43,
 * Save description 0x2f63/prompt 0x2f92. Save screen: MINT01.shots/
 * 03_save_prompt.png. A: replaces the reference's C: (our data drive).
 * Refusal/error/typed-file guidance and the drive substitution are authored,
 * no local reference. No advertised command silently succeeds. */
static const char REPLACE_LINE[] = "Cancel  Replace  Backup";
/* THE ALERT'S WORDS (spec/toolbox_gate.h Sec 11; CR separates lines). Line 1
 * is the real 1-2-3's own warning, 123.RI 0x370 "WORKSHEET CHANGES NOT
 * SAVED!  End 1-2-3 anyway?", first sentence verbatim. Lines 2-3 and the
 * button words are the Macintosh Save-changes idiom: a PRODUCT ADAPTATION
 * (bead initech-fvnj; the DOS 1-2-3 has no dialog box), authored, no local
 * reference for their wording. The mapping onto the period workflow (p. 9-1:
 * "If you selected Yes and you have changed data in the worksheet but not
 * saved it, 1-2-3 displays another No/Yes menu"):
 *   Cancel     == that second No  ("Returns to READY mode so you can use
 *                 /File Save to save the worksheet");
 *   Don't Save == that second Yes ("Ends 1-2-3 session without saving");
 *   Save       == the Mac addition: /File Save now, then end the session. */
static const char QUIT_TEXT[] =
    "WORKSHEET CHANGES NOT SAVED!\rSave changes to the worksheet\rbefore quitting?";
static const char REPLACE_NAMES[3][8] = { "Cancel", "Replace", "Backup" };
/* One menu stack, preserving the highlighted item on Esc. */
static uint16_t g_level, g_levels[12], g_indexes[12];
static uint32_t g_depth;
static int g_top, g_action, g_stage, g_arg, g_dc, g_dr;
static uint8_t g_format;
static i123_rect_t g_from;
static char g_range_text[77], g_named[16];
static uint32_t g_rlen;
static const char *g_prompt;
enum { A_COPY=1, A_MOVE, A_VALUE, A_TRANS, A_ERASE, A_FORMAT, A_LABEL,
       A_PROT, A_UNPROT, A_NAME, A_NAME_DELETE, A_NAME_LABELS, A_NAME_TABLE,
       A_WIDTH, A_GLOBAL_WIDTH, A_ITERATION, A_GOTO };

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
    /* B12: nothing past the window's last whole character (g_cols <= 76) */
    if ((uint32_t)col >= cols()) return;
    if (col + n > cols()) n = cols() - (uint32_t)col;
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
    if (g_mode == M_MAIN || g_mode == M_FILE || g_mode == M_QUIT) {
        const i123_menu_level_t *lv = &i123_menu_levels[g_level];
        char line[512]; uint32_t n = 0, x = 0;
        for (uint32_t i = 0; i < lv->count; i++) {
            const char *nm = i123_menu_string(i123_menu_items[lv->first+i].name);
            if ((int)i == g_menu_index) x = n;
            for (uint32_t k = 0; nm[k]; k++) line[n++] = nm[k];
            if (i+1 < lv->count) { line[n++] = ' '; line[n++] = ' '; }
        }
        line[n] = 0;
        const i123_menu_item_t *it = &i123_menu_items[lv->first+g_menu_index];
        const char *nm = i123_menu_string(it->name);
        uint32_t skip = x+slen(nm)>cols() ? x : 0;
        panel_line(LINE_PANEL2, line+skip);
        put_line(LINE_PANEL2, (int)(x-skip), nm, slen(nm), TBX_COLOR_WHITE, TBX_COLOR_NAVY);
        panel_line(LINE_PANEL3, i123_menu_string(it->desc));
    } else if (g_mode == M_RANGE || g_error) {
        /* B11: the GOTO error keeps the prompt and its text on line two
         * (mint keys 10_f5_bad, fixtures/i123-keys). */
        char line[SCR_COLS+1]; uint32_t n = 0;
        for (uint32_t k = 0; g_prompt[k] && n < SCR_COLS; k++) line[n++] = g_prompt[k];
        if (g_rlen) {
            for (uint32_t k = 0; k < g_rlen && n < SCR_COLS; k++) line[n++] = g_range_text[k];
        } else {
            char addr[16]; uint32_t k = i123_addr_text(i123_cursor_col(&g_sheet), i123_cursor_row(&g_sheet), addr);
            for (uint32_t j = 0; j < k && n < SCR_COLS; j++) line[n++] = addr[j];
        }
        if (n < SCR_COLS) line[n++] = '_';
        line[n] = 0; panel_line(LINE_PANEL2, line); panel_line(LINE_PANEL3, "");
    } else if (g_mode == M_REPLACE) {
        panel_line(LINE_PANEL2, REPLACE_LINE);
        uint32_t x = 0;
        for (int i = 0; i < g_menu_index; i++) x += slen(REPLACE_NAMES[i])+2;
        put_line(LINE_PANEL2,(int)x,REPLACE_NAMES[g_menu_index],slen(REPLACE_NAMES[g_menu_index]),TBX_COLOR_WHITE,TBX_COLOR_NAVY);
        panel_line(LINE_PANEL3,g_menu_index==0?"Cancel command -- Leave existing file on disk intact":
            g_menu_index==1?"Update the file on disk with the current file":
            "Backup existing file (with a .BAK extension) and save the current file on disk");
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

/* ---- the view -------------------------------------------------------------
 * B06 (bead initech-3qtt, audit L008): a first column wider than the area is
 * SHOWN, clipped to the area -- heading centred, value aligned, pointer drawn
 * over the visible part only -- as the real 1-2-3 R2.2 shows a 76- or 240-
 * wide column in its 72-character area (fixtures/i123-width 03_w76, 05_w240,
 * 13_w240_labels, 15_back_a1); a later column that does not fit WHOLE is not
 * shown (11_w67: B is absent beside a 67-wide A). The ONE visible width,
 * i123_colwidth clipped to the area, is what border, rows, pointer and the
 * hit-test walk; only the view's first column can ever be clipped. */
static uint32_t area_cols(void)
{
    uint32_t n = i123_cols_fitting(&g_sheet, i123_view_left(&g_sheet), AREA_CHARS);
#ifndef I123_MUT_WIDE_HIDDEN
    if (!n) n = 1u;
#else
    /* MUTANT (Rule 6; test-i123-resize-mutant W1, test-i123-wk1-roundtrip-
     * mutant display, test-flair-i123-wide-mutant): an oversized first
     * column shows as nothing at all (audit L008). NEVER real. */
#endif
    return n;
}
static uint32_t vis_width(uint16_t col)
{
    uint32_t w = i123_colwidth(&g_sheet, col);
    return w < AREA_CHARS ? w : AREA_CHARS;
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
    if (nl > cc) nl = cc;   /* B12: a column wider than the window stays first */
    if (nl == left && nt == top) return 0;
    i123_set_view(&g_sheet, nl, nt);
    return 1;
}

/* B12: THE GEOMETRY from the content size (see above). A refused query keeps
 * the last geometry; a collapsed window (0) takes the minimum until the
 * update that comes with its re-expansion. */
static void geometry(void)
{
#ifndef I123_MUT_FIXED_GEOMETRY
    int32_t sz = tbx_content_size(g_win);
    uint32_t w = (uint32_t)sz & 0xFFFFu, h = (uint32_t)sz >> 16;
    if (sz < 0) return;
    w = w > X0 ? (w - X0) / CW : 0u;
    h /= CH;
    if (w < GEO_MIN_COLS) w = GEO_MIN_COLS;
    if (w > SCR_COLS) w = SCR_COLS;
    if (h < LINE_ROW0 + 2u) h = LINE_ROW0 + 2u;
    if (h > SCR_LINES) h = SCR_LINES;
    g_cols = (uint8_t)w;
    g_lines = (uint8_t)h;
#else
    /* MUTANT (Rule 6; test-i123-resize-mutant, test-flair-i123-resize-
     * mutant): the face ignores the content and keeps the 76 x 24 screen, so
     * a resize only clips it (audit L018). NEVER real. */
#endif
}

/* ---- painting --------------------------------------------------------------- */
static void paint_panel(void)
{
    char buf[SCR_COLS + 1], addr[16], content[I123_FTEXT_MAX];
    const i123_cell_t *c;
    uint32_t n, k;
    /* Prompted for an address, 1-2-3 shows POINT until a character is
     * typed, then EDIT (manual p. 1-6; mint keys 06/07, COPY01 04_copy_from). */
    const char *mode = MODE_TEXT[g_mode == M_RANGE && g_rlen ? M_EDIT : g_mode];
    uint32_t mlen = slen(mode);

    /* line 1: "A1: 'Hello" ... "READY" (the shots: address, colon, space) */
    n = i123_addr_text(i123_cursor_col(&g_sheet), i123_cursor_row(&g_sheet), addr);
    for (k = 0; k < n; k++) buf[k] = addr[k];
    buf[n++] = ':';
    buf[n++] = ' ';
    c = i123_find(&g_sheet, i123_cursor_col(&g_sheet), i123_cursor_row(&g_sheet));
    i123_cell_content(&g_sheet, c, content, sizeof content);
    for (k = 0; content[k] && n < cols() - mlen - 1u; k++) buf[n++] = content[k];
    buf[n] = '\0';
    {
        char tmp[SCR_COLS + 1];
        pad_into(tmp, cols() - mlen, buf);
        put_line(LINE_PANEL1, 0, tmp, cols() - mlen, TBX_COLOR_BLACK, TBX_COLOR_WHITE);
    }
    put_line(LINE_PANEL1, (int)(cols() - mlen), mode, mlen, TBX_COLOR_WHITE, TBX_COLOR_NAVY);
}

static void paint_edit_line(void)
{
    if (g_mode >= M_MAIN) { paint_command(); return; }
    char tmp[SCR_COLS + 1], src[SCR_COLS + 1];
    /* Ref: manual p. 1-4/1-16: the entry occupies control-panel line two.
     * B13 (initech-twxf) requires the tail to follow the typing cursor.
     * No long-entry capture in the Lotus corpus; one-character scrolling
     * is the native viewport policy, not a measured R2.2 scroll increment. */
    uint32_t i, first = g_elen > cols() - 1u ? g_elen - (cols() - 1u) : 0u;
    for (i = first; i < g_elen; i++) src[i - first] = g_edit[i];
    i -= first;
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
        uint32_t w = vis_width((uint16_t)(left + k));
        uint32_t nl = i123_col_name((uint16_t)(left + k), name);
        uint32_t at = x + (w > nl ? (w - nl) / 2u : 0u);
        for (uint32_t j = 0; j < nl && at + j < SCR_COLS; j++) tmp[at + j] = name[j];
        x += w;
    }
    put_line(LINE_BORDER, 0, tmp, SCR_COLS, TBX_COLOR_WHITE, TBX_COLOR_NAVY);
}

static void paint_row(uint32_t vis)
{
    char num[8], tmp[SCR_COLS + 1], row[SCR_COLS + 1];
    uint16_t r = (uint16_t)(i123_view_top(&g_sheet) + vis);
    int line = LINE_ROW0 + (int)vis;
    uint32_t n = 0;
    /* B11: a window placed by GOTO may run past row 8192; those lines show
     * an empty border and no cells (mint keys 42_a8190). */
    if (r >= I123_MAX_ROWS) { panel_line(line, ""); put_line(line, 0, "    ", BORDER_W, TBX_COLOR_WHITE, TBX_COLOR_NAVY); return; }
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
    uint32_t w = vis_width(cc), spill;
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

/* The status line: the notice at the left, the CALC / CIRC indicators at
 * the right end (see THE STATUS LINE above). */
static void paint_status(void)
{
    panel_line(LINE_STATUS, g_error ? g_error : g_status);
    if (g_sheet.circ)
        put_line(LINE_STATUS, (int)cols() - 4, "CIRC", 4, TBX_COLOR_WHITE, TBX_COLOR_NAVY);
    if (g_sheet.calc)
        put_line(LINE_STATUS, (int)cols() - 9, "CALC", 4, TBX_COLOR_WHITE, TBX_COLOR_NAVY);
}

/* After a recalculation every visible value may have moved. */
static int sheet_has_formula(void)
{
    for (uint32_t i = 0; i < g_sheet.ncells; i++)
        if (g_sheet.cells[i].kind == I123_K_FORMULA) return 1;
    return 0;
}

static void paint_all(void)
{
    (void)tbx_fill_rect(g_win, 0, 0, WIN_R, WIN_B, TBX_COLOR_WHITE);
    paint_panel();
    paint_edit_line();
    paint_border();
    paint_rows();
    paint_status();
}

/* ---- moving the pointer (B11, initech-69mz) ---------------------------------
 * THE one viewport path for every pointer-movement key: arrows, Home, GOTO,
 * PgUp/PgDn, Tab/Shift-Tab. Put the pointer at (c,r) with the window's
 * upper-left at (left,top), let follow_pointer keep the pointer visible,
 * and repaint what changed. Ground truth: the real 1-2-3 R2.2 driven under
 * DOSBox-X, fixtures/i123-keys/ (keys*.steps + the captures). */
static void place(uint16_t c, uint16_t r, uint16_t left, uint16_t top)
{
    uint16_t orow = i123_cursor_row(&g_sheet);
    uint16_t ol = i123_view_left(&g_sheet), ot = i123_view_top(&g_sheet);
    i123_set_cursor(&g_sheet, c, r);
    i123_set_view(&g_sheet, left, top);
    (void)follow_pointer();
    if (i123_view_left(&g_sheet) != ol || i123_view_top(&g_sheet) != ot) {
        paint_border();
        paint_rows();
    } else {
        if (orow >= ot && orow < ot + VIS_ROWS) paint_row((uint32_t)(orow - ot));
        paint_pointer();
    }
    paint_panel();
}
static void here(uint16_t c, uint16_t r)
{ place(c, r, i123_view_left(&g_sheet), i123_view_top(&g_sheet)); }

/* GOTO (F5, manual p. 1-11): a target already on screen only moves the
 * pointer (COPY01 08_show_C20: C20 from A1, no scroll); one off screen
 * becomes the window's upper-left cell (mint keys 08_f5_z100, 09, 37_goto_a21,
 * 60_goto_d21 -- both axes, even one row past the bottom). */
static void go_to(uint16_t c, uint16_t r)
{
    uint16_t left = i123_view_left(&g_sheet), top = i123_view_top(&g_sheet);
#ifndef I123_MUT_GOTO_FOLLOW
    if (c < left || c >= left + area_cols() || r < top || r >= top + VIS_ROWS) { left = c; top = r; }
#else
    /* MUTANT (Rule 6; test-i123-keys-mutant): GOTO scrolls only as far as
     * an arrow would. NEVER real. */
#endif
    place(c, r, left, top);
}

static void command_paint(void) { paint_panel(); paint_edit_line(); }
static void notice(const char *s)
{
#ifdef I123_MUT_SAVE_FAIL_QUITS
    /* MUTANT (Rule 6; test-i123-quit-mutant Q8): a Save the alert started
     * fails, and the session ends anyway -- the work is lost. NEVER real. */
    if (g_quit_after_save) { (void)tbx_exit(0); return; }
#endif
    g_quit_after_save = 0; g_notice = s; g_mode = M_NOTICE; command_paint();
}
static void ready(void)
{
    g_quit_after_save = 0; g_mode = M_READY; g_elen = 0; g_edit[0] = 0; g_nlen = 0; command_paint();
    if (g_error) { g_error = 0; paint_status(); }
}
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
    g_dirty = 0;   /* initech-fvnj: the sheet IS the file now */
    copy_text(g_current, g_path); file_status("Retrieved "); ready(); paint_all();
}
static void write_sheet(void)
{
    /* initech-mcep (audit 2026-10-23-i123 L001): the atomic save -- a failed
     * Replace keeps the previously saved worksheet, a failed new name leaves
     * no file (tbx_save.h removes its temporary). */
    int32_t rc = tbx_file_save(g_path, g_wk, g_wk_len);
    if (rc != (int32_t)g_wk_len) { file_error(rc < 0 ? rc : 0); return; }
    copy_text(g_current, g_path); file_status("Saved ");
    g_dirty = 0;   /* initech-fvnj: only a successful Save (or Retrieve) cleans */
    if (g_quit_after_save) { (void)tbx_exit(0); return; }   /* the alert's Save */
    ready(); paint_all();
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
    notice("Command not implemented");
}
static void request_quit(void);
static int same(const char *a, const char *b)
{ while (*a && *a == *b) { a++; b++; } return *a == *b; }
static int spare(i123_sheet_t *next)
{
    int rc = i123_sheet_init(next,g_mem[1-g_bank],sizeof g_mem[0],SHEET_CELLS);
    return rc ? rc : i123_sheet_clone(&g_sheet,next);
}
static void changed(i123_sheet_t *next)
{
    /* Recalc the candidate bank BEFORE publishing it; failure keeps the old
     * worksheet. All command mutations use the same unsaved-work guard. */
    refresh_clock();
    if (i123_after_entry(&g_cx,next)) { notice("Not enough worksheet memory"); return; }
    g_sheet = *next; g_bank = 1-g_bank; g_dirty = 1; ready(); paint_all();
}
static void prompt(int action, const char *text)
{ g_action=action; g_stage=0; g_prompt=text; g_rlen=0; g_range_text[0]=0; g_mode=M_RANGE; command_paint(); }
static void range_accept(void)
{
    char addr[16]; const char *input=g_range_text; i123_rect_t r;
    i123_sheet_t next; int rc=I123_OK;
    if (!g_rlen) { i123_addr_text(i123_cursor_col(&g_sheet),i123_cursor_row(&g_sheet),addr); input=addr; }
    if ((g_action==A_NAME && g_stage==0) || g_action==A_NAME_DELETE) {
        if (!g_rlen || g_rlen>15) { notice("Invalid range name"); return; }
        copy_text(g_named,input);
        if (g_action==A_NAME) { g_stage=1; g_prompt="Enter range: "; g_rlen=0; command_paint(); return; }
        rc=spare(&next); if (!rc) rc=i123_name_delete(&next,g_named);
    } else if (g_action==A_WIDTH || g_action==A_GLOBAL_WIDTH || g_action==A_ITERATION ||
               (g_action==A_FORMAT && g_stage==0 && g_arg)) {
        double x;
        if (!g_rlen || i123_parse_value(input,g_rlen,&x) || x<0 || x>240 || (int)x!=x ||
            (g_action==A_FORMAT && x>15) || (g_action==A_ITERATION && (x<1||x>50)) ||
            ((g_action==A_WIDTH||g_action==A_GLOBAL_WIDTH)&&x<1)) { notice("Illegal number"); return; }
        if (g_action==A_FORMAT) { g_format=(uint8_t)(g_format+(uint8_t)x); if(g_top==0){rc=spare(&next);if(!rc){next.win1[WK1_W1_FMT]=g_format;changed(&next);}return;} g_stage=1; g_rlen=0; g_prompt="Enter range to format: "; command_paint(); return; }
        rc=spare(&next);
        if (!rc) {
            if(g_action==A_WIDTH) rc=i123_set_colwidth(&next,i123_cursor_col(&g_sheet),(uint8_t)x);
            if(g_action==A_GLOBAL_WIDTH) {next.win1[WK1_W1_WIDTH]=(uint8_t)x;next.win1[WK1_W1_WIDTH+1]=0;}
            if(g_action==A_ITERATION)next.calccount=(uint8_t)x;
        }
    } else {
        if (i123_range_parse(&g_sheet,input,&r)) {
            /* B11: an unusable GOTO address beeps, shows ERROR and the R2.2
             * message (123.RI 0x5ece, verbatim) on the status line, and leaves
             * the pointer where it was (mint keys 10_f5_bad/11_f5_bad_esc). */
            if(g_action==A_GOTO){(void)tbx_trap(TBX_BEEP,0,0);g_error="Invalid cell or range address";g_mode=M_NOTICE;paint_panel();paint_status();return;}
            notice("Invalid range"); return;
        }
        if(g_action==A_GOTO) {ready();go_to(r.c0,r.r0);return;}
        if(g_action<=A_TRANS && !g_stage) {
            g_from=r;g_stage=1;g_rlen=0;
            g_prompt=g_action==A_MOVE?"Enter range to move TO: ":"Enter range to copy TO: ";
            command_paint();return;
        }
        rc=spare(&next);
        if(!rc && g_action<=A_TRANS) {
            if(r.c0!=r.c1||r.r0!=r.r1) {notice("Specify the upper left destination cell");return;}
            rc=i123_transfer(&g_sheet,&next,g_from,r.c0,r.r0,g_action-1);
        } else if(!rc && g_action==A_NAME)rc=i123_name_set(&next,g_named,r);
        else if(!rc && g_action==A_NAME_LABELS)rc=i123_name_labels(&next,r,g_dc,g_dr);
        else if(!rc && g_action==A_NAME_TABLE)rc=i123_name_table(&next,r.c0,r.r0);
        else if(!rc) {
            for(uint32_t row=r.r0;!rc&&row<=r.r1;row++)for(uint32_t col=r.c0;!rc&&col<=r.c1;col++) {
                const i123_cell_t *c=i123_find(&next,(uint16_t)col,(uint16_t)row);
                uint8_t fmt=c?c->fmt:WK1_FMT_FRESH_CELL;
                if((g_action==A_ERASE||g_action==A_LABEL)&&next.protec&&(fmt&128)) {rc=I123_E_ARG;break;}
                /* Manual p. 10-3 (PDF 221): erase data, keep format/protection. */
                if(g_action==A_ERASE&&c) {
                    rc=i123_erase(&next,(uint16_t)col,(uint16_t)row);
                    if(!rc)rc=i123_set_cell_format(&next,(uint16_t)col,(uint16_t)row,fmt);
                }
                if(g_action==A_FORMAT)rc=i123_set_cell_format(&next,(uint16_t)col,(uint16_t)row,(fmt&128)|g_format);
                if(g_action==A_PROT||g_action==A_UNPROT)rc=i123_set_cell_format(&next,(uint16_t)col,(uint16_t)row,
                    g_action==A_PROT?fmt|128:fmt&127);
                if(g_action==A_LABEL&&c&&c->kind==I123_K_LABEL)next.pool[c->off]=(uint8_t)g_arg;
            }
        }
    }
    if(rc) {notice(rc==I123_E_FULL?"Not enough worksheet memory":"Command refused");return;}
    changed(&next);
}
static void menu_mode(void)
{ g_mode=g_depth==1&&g_top==4?M_FILE:g_depth==1&&g_top==10?M_QUIT:M_MAIN; }
static void menu_select(void)
{
    if(g_mode==M_REPLACE) {
        if(g_menu_index==0)ready();else if(g_menu_index==1)write_sheet();else unsupported();return;
    }
    const i123_menu_level_t *lv=&i123_menu_levels[g_level];
    const i123_menu_item_t *it=&i123_menu_items[lv->first+g_menu_index];
    const char *nm=i123_menu_string(it->name);
    if(g_depth==0)g_top=g_menu_index;
    if(it->child!=65535) {
        if(g_depth>=12){notice("Menu nesting error");return;}
        g_levels[g_depth]=g_level;g_indexes[g_depth++]=(uint16_t)g_menu_index;
        g_level=it->child;g_menu_index=0;menu_mode();command_paint();return;
    }
    if(g_top==10) {int yes=g_menu_index==1;ready();if(yes)request_quit();return;}
    if(g_depth==0 && (g_top==2||g_top==3)) {prompt(g_top==2?A_COPY:A_MOVE,g_top==2?"Enter range to copy FROM: ":"Enter range to move FROM: ");return;}
    if(g_top==4 && g_depth==1 && g_menu_index<=1) {g_mode=g_menu_index?M_SAVE:M_GET;g_nlen=0;g_name[0]=0;command_paint();return;}
    if(same(nm,"Quit")) {ready();return;}
    if(g_top==1) {
        if(g_depth==1) {
            if(same(nm,"Erase")) {prompt(A_ERASE,"Enter range to erase: ");return;}
            if(same(nm,"Prot")||same(nm,"Unprot")) {prompt(same(nm,"Prot")?A_PROT:A_UNPROT,same(nm,"Prot")?"Enter range to protect: ":"Enter range to unprotect: ");return;}
            if(same(nm,"Value")||same(nm,"Trans")) {prompt(same(nm,"Value")?A_VALUE:A_TRANS,"Enter range to copy FROM: ");return;}
        }
        /* /Range Format (including Date and Time); supported formats only. */
        if(g_depth>=2 && g_indexes[1]==0) {
            if(g_depth==2) {
                static const uint8_t f[]={0x00,0x10,0x20,0x40,0x71,0x70,0x30,0,0x75,0x76,0x7f};
                g_format=f[g_menu_index];g_arg=g_menu_index<4||g_menu_index==6;
                prompt(A_FORMAT,g_arg?"Enter number of decimal places (0..15): ":"Enter range to format: ");return;
            }
            if(g_depth==3&&g_menu_index<3) {g_format=(uint8_t)(0x72+g_menu_index);g_arg=0;prompt(A_FORMAT,"Enter range to format: ");return;}
        }
        if(g_depth==2&&g_indexes[1]==1) {g_arg=g_menu_index==0?'\'':g_menu_index==1?'"':'^';prompt(A_LABEL,"Enter range of labels: ");return;}
        if(g_depth==2&&g_indexes[1]==3) {
            if(g_menu_index==0){prompt(A_NAME,"Enter name: ");return;}
            if(g_menu_index==1){prompt(A_NAME_DELETE,"Enter name to delete: ");return;}
            if(g_menu_index==3){i123_sheet_t next;if(spare(&next)){notice("Not enough worksheet memory");return;}i123_name_delete(&next,0);changed(&next);return;}
            if(g_menu_index==4){prompt(A_NAME_TABLE,"Enter range for table: ");return;}
        }
        if(g_depth==3&&g_indexes[1]==3&&g_indexes[2]==2) {
            g_dc=g_menu_index==0?1:g_menu_index==2?-1:0;g_dr=g_menu_index==1?1:g_menu_index==3?-1:0;
            prompt(A_NAME_LABELS,"Enter label range: ");return;
        }
    }
    if(g_top==0) {
        if(g_depth>=3&&g_indexes[1]==0&&g_indexes[2]==0) {
            if(g_depth==3) {
                static const uint8_t f[]={0,0x10,0x20,0x40,0x71,0x70,0x30,0,0x75,0x76};
                g_format=f[g_menu_index];g_arg=g_menu_index<4||g_menu_index==6;
                if(g_arg){prompt(A_FORMAT,"Enter number of decimal places (0..15): ");return;}
            } else if(g_depth==4&&g_menu_index<3)g_format=(uint8_t)(0x72+g_menu_index);
            else {unsupported();return;}
            i123_sheet_t next;if(!spare(&next)){next.win1[WK1_W1_FMT]=g_format;changed(&next);}return;
        }
        if(g_depth==3&&g_indexes[1]==0&&g_indexes[2]==1) {
            i123_sheet_t next;if(!spare(&next)){next.labelfmt=(uint8_t)(g_menu_index==0?'\'':g_menu_index==1?'"':'^');changed(&next);}return;
        }
        if(g_depth==2&&g_indexes[1]==4) {
            if(g_menu_index==0){ready();return;}
            i123_sheet_t next;if(!i123_sheet_init(&next,g_mem[1-g_bank],sizeof g_mem[0],SHEET_CELLS))changed(&next);return;
        }
        if(g_depth==2&&g_indexes[1]==3) {
            if(g_menu_index==0){prompt(A_WIDTH,"Enter column width (1..240): ");return;}
            if(g_menu_index==1){i123_sheet_t next;if(!spare(&next)){i123_set_colwidth(&next,i123_cursor_col(&g_sheet),0);changed(&next);}return;}
        }
        if(g_depth==2&&g_indexes[1]==0&&g_menu_index==2){prompt(A_GLOBAL_WIDTH,"Enter global column width (1..240): ");return;}
        if(g_depth==3&&g_indexes[1]==0&&g_indexes[2]==3) {
            if(g_menu_index==5){prompt(A_ITERATION,"Enter iteration count (1..50): ");return;}
            i123_sheet_t next;if(spare(&next)){notice("Not enough worksheet memory");return;}
            if(g_menu_index<3)next.calcorder=(uint8_t)(g_menu_index==2?255:g_menu_index);
            else next.calcmode=(uint8_t)(g_menu_index==3?255:0);
            changed(&next);return;
        }
        if(g_depth==3&&g_indexes[1]==0&&g_indexes[2]==4) {
            i123_sheet_t next;if(spare(&next)){notice("Not enough worksheet memory");return;}
            next.protec=(uint8_t)(g_menu_index==0?255:0);changed(&next);return;
        }
    }
    unsupported();
}
static void menu_back(void)
{
    if(!g_depth) {ready();return;}
    g_depth--;g_level=g_levels[g_depth];g_menu_index=g_indexes[g_depth];menu_mode();command_paint();
}
static void command_key(char ch, uint8_t vk)
{
    if(ch==3 || vk==0x46) {ready();return;} /* Ctrl-Break/Ctrl-C */
    if(vk==VK_ESC||ch==0x1b) {
        if(g_mode==M_MAIN||g_mode==M_FILE||g_mode==M_QUIT){menu_back();return;}
        if(g_mode==M_SAVE&&g_quit_after_save){ready();return;}
        if(g_mode==M_GET||g_mode==M_SAVE||g_mode==M_RANGE){menu_mode();command_paint();return;}
        if(g_mode==M_REPLACE){g_mode=M_SAVE;g_nlen=0;command_paint();return;}
        ready();return;
    }
    if(g_mode==M_NOTICE){if(g_error&&(vk==VK_ENTER||ch=='\r'))ready();return;}
    if(g_mode==M_RANGE) {
        if(vk==VK_ENTER||ch=='\r'){range_accept();return;}
        if(vk==VK_BSPC||ch==8){if(g_rlen)g_rlen--;}
        else if(ch>=' '&&ch<='~'&&g_rlen<sizeof g_range_text-1)g_range_text[g_rlen++]=ch;
        else if(vk==VK_UP||vk==VK_DOWN||vk==VK_LEFT||vk==VK_RIGHT) {
            int c=i123_cursor_col(&g_sheet),r=i123_cursor_row(&g_sheet);
            c+=vk==VK_RIGHT?1:vk==VK_LEFT?-1:0;r+=vk==VK_DOWN?1:vk==VK_UP?-1:0;
            if(c>=0&&c<256&&r>=0&&r<8192){i123_set_cursor(&g_sheet,(uint16_t)c,(uint16_t)r);follow_pointer();paint_all();}
        }
        g_range_text[g_rlen]=0;command_paint();return;
    }
    if(g_mode==M_GET||g_mode==M_SAVE) {
        if(vk==VK_ENTER||ch=='\r'){if(g_mode==M_GET)retrieve();else save();return;}
        if(vk==VK_BSPC||ch==8){if(g_nlen)g_nlen--;}
        else if(ch>=' '&&ch<='~') {
            if(g_nlen>=sizeof g_name-1){notice("File name too long");return;}
            g_name[g_nlen++]=ch;
        }
        g_name[g_nlen]=0;command_paint();return;
    }
    uint32_t count=g_mode==M_REPLACE?3:i123_menu_levels[g_level].count;
    if(vk==VK_ENTER||ch=='\r'){menu_select();return;}
    if(vk==VK_LEFT||vk==VK_UP)g_menu_index=(g_menu_index+(int)count-1)%(int)count;
    else if(vk==VK_RIGHT||vk==VK_DOWN||ch==' ')g_menu_index=(g_menu_index+1)%(int)count;
    else if(vk==VK_HOME)g_menu_index=0;
    else {
        if(ch>='a'&&ch<='z')ch-=32;
        for(uint32_t i=0;i<count;i++) {
            const char *nm=g_mode==M_REPLACE?REPLACE_NAMES[i]:i123_menu_string(i123_menu_items[i123_menu_levels[g_level].first+i].name);
            if(nm[0]==ch){g_menu_index=(int)i;menu_select();return;}
        }
    }
    command_paint();
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
    const i123_cell_t *locked=i123_find(&g_sheet,cc,cr);
    /* B02 review (initech-3s12): an ABSENT cell is the default 0xFF, bit 7
     * set -- protected under /WGP Enable unless /Range Unprot gave it a
     * BLANK (wk1-format.md Sec 4.1 / Format byte bit 7; manual p. 10-3). */
#ifndef I123_MUT_ABSENT_UNPROTECTED
    if(g_sheet.protec&&((locked?locked->fmt:WK1_FMT_FRESH_CELL)&128)) {notice("Cell is protected");return 1;}
#else
    /* MUTANT (Rule 6; test-i123-cells-mutant C9): an absent cell counts as
     * unprotected. NEVER real. */
    if(g_sheet.protec&&locked&&(locked->fmt&128)) {notice("Cell is protected");return 1;}
#endif
    refresh_clock();
    /* EDIT completes as a label unless the entry starts like a value (p. 1-16:
     * the first character decides; mint keys 05 "'HelloX", 49 "HelloZ"). */
    if (g_mode == M_LABEL || (g_mode == M_EDIT && !is_value_start(g_edit[0]))) {
        if (g_elen > 0u && is_prefix(g_edit[0]))
            rc = i123_set_label(&g_sheet, cc, cr, (uint8_t)g_edit[0], g_edit + 1, g_elen - 1u);
        else
            rc = i123_set_label(&g_sheet, cc, cr, g_sheet.labelfmt, g_edit, g_elen);
        if (rc == I123_OK) rc = i123_after_entry(&g_cx, &g_sheet);
    } else {
        double x;
        /* Reference p. 1-19: a leading dollar enters a number, independently
         * of its display format. Strip it only at the entry boundary, so
         * absolute references (+$A$1) keep their formula meaning. */
        uint32_t prefix = 0;
#ifndef I123_MUT_CURRENCY_REJECT
        /* Real NUMENTRY.WK1: $-12.34 / $+12.34 are numbers; -$ / +$
         * are refused. $A$1 remains a formula after numeric parsing fails. */
        if (g_elen > 1 && g_edit[0] == '$') prefix = 1;
#endif
        rc = i123_parse_value(g_edit + prefix, g_elen - prefix, &x);
        if (rc == I123_OK) {
            rc = i123_set_number(&g_sheet, cc, cr, x);
#ifndef I123_MUT_NO_RECALC
            if (rc == I123_OK) rc = i123_after_entry(&g_cx, &g_sheet);
#else
            /* MUTANT (Rule 6; test-flair-i123-formula-mutant): a typed
             * number recalculates nothing, so the formulas over it keep
             * stale values in AUTOMATIC mode. NEVER in a real build. */
#endif
        } else {
            /* not a number: a formula (bead initech-94ah) -- compiled,
             * stored and recalculated by the sheet's mode, or refused */
            rc = i123_enter_formula(&g_cx, &g_sheet, cc, cr, g_edit, g_elen);
        }
    }
    if (rc != I123_OK) {
        g_mode = M_EDIT;   /* 1-2-3 beeps and drops into EDIT */
        paint_panel();
        paint_edit_line();
        return 1;
    }
#ifndef I123_MUT_NEVER_DIRTY
    g_dirty = 1;        /* initech-fvnj: a committed edit is unsaved work */
#else
    /* MUTANT (Rule 6; test-i123-quit-mutant Q2): commits never mark the
     * sheet dirty, so Quit discards them silently -- L008. NEVER real. */
#endif
    g_mode = M_READY;
    g_elen = 0;
    g_edit[0] = '\0';
    if (sheet_has_formula()) paint_rows();          /* values may have moved */
    else if (cr >= top && cr < top + VIS_ROWS) paint_row((uint32_t)(cr - top));
    paint_pointer();
    paint_panel();
    paint_edit_line();
    paint_status();
    return 0;
}

/* CALC (F9): recalculate now (manual p. 12-32, PDF 276). */
static void calc_key(void)
{
    refresh_clock();
    if (i123_recalc(&g_cx, &g_sheet) != I123_OK) {
        notice("Not enough memory to recalculate");
        return;
    }
    paint_rows();
    paint_status();
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
    here((uint16_t)nc, (uint16_t)nr);
}

/* PGDN / PGUP (manual p. 1-10: "one screen"): the pointer AND the window move
 * VIS_ROWS rows, the pointer keeping its place on screen (mint keys 12_pgdn
 * A1->A21 rows 21-40 on the 20-row DOS screen, 15_pgdn_mid A4->A24, 39 A20->
 * A40, 65 A25->A5); each clamps at row 1 (66: A7 with top 5 -> A1, top 1).
 * Near row 8192 the pointer stops on the last row (43_a8190_pgdn: A8190 ->
 * A8192, window unmoved). */
static void page_rows(int down)
{
    uint16_t c = i123_cursor_col(&g_sheet), r = i123_cursor_row(&g_sheet);
    uint16_t left = i123_view_left(&g_sheet), top = i123_view_top(&g_sheet);
    if (down) {
        if ((uint32_t)r + VIS_ROWS < I123_MAX_ROWS) { r = (uint16_t)(r + VIS_ROWS); top = (uint16_t)(top + VIS_ROWS); }
        else r = (uint16_t)(I123_MAX_ROWS - 1u);
    } else {
        r = (uint16_t)(r >= VIS_ROWS ? r - VIS_ROWS : 0);
        top = (uint16_t)(top >= VIS_ROWS ? top - VIS_ROWS : 0);
    }
    place(c, r, left, top);
}

/* TAB / SHIFT-TAB in READY (manual pp. 1-9/1-10: "right [left] one screen",
 * not one cell): the window moves by the columns it shows and the pointer
 * lands on its FIRST column (mint keys 16 A1->I1, 32 D1->I1, 63 H1->P1;
 * 18 Q1->I1, 36 K1->C1, 62 H1->A1; at IV, 41: no move). Authored where no
 * capture reaches: unequal widths count the columns now shown. */
static void page_cols(int right)
{
#ifdef I123_MUT_TAB_ONE_CELL
    /* MUTANT (Rule 6; test-i123-keys-mutant, test-flair-i123-keys-mutant):
     * Tab moves one cell. NEVER real. */
    move_by(right ? 1 : -1, 0);
#else
    uint32_t left = i123_view_left(&g_sheet), n = area_cols();
    if (right) left = left + n < I123_MAX_COLS ? left + n : I123_MAX_COLS - 1u;
    else left = left > n ? left - n : 0;
    place((uint16_t)left, i123_cursor_row(&g_sheet), (uint16_t)left, i123_view_top(&g_sheet));
#endif
}

/* Complete the entry being typed or edited; 0 = READY now. An EDIT line
 * emptied (by Esc) completes as no entry: the cell keeps what it had (mint
 * keys 45/46 F2, Esc, Enter; 48 F2 on an empty cell, Enter). */
static int finish(void)
{
    if (g_mode == M_EDIT && !g_elen) { g_mode = M_READY; paint_panel(); paint_edit_line(); return 0; }
    return commit();
}

/* EDIT (F2, manual p. 1-11): from READY, the current cell's contents go on
 * the edit line in EDIT mode (mint keys 02_f2 "'Hello", 44 "123", 47 empty);
 * in EDIT it switches to LABEL or VALUE by the entry's first character, and
 * while typing it switches to EDIT (p. 1-22 "Editing As You Type"). The
 * in-line cursor keys of EDIT (p. 1-22/1-23: Left/Right/Home/End/Tab/Del/Ins)
 * are not implemented: the cursor stays at the end (Backspace works). */
static void edit_key(void)
{
    if (g_mode == M_READY) {
        i123_cell_content(&g_sheet, i123_find(&g_sheet, i123_cursor_col(&g_sheet), i123_cursor_row(&g_sheet)),
                          g_edit, sizeof g_edit);
        g_elen = slen(g_edit);
        g_mode = M_EDIT;
    } else if (g_mode != M_EDIT) g_mode = M_EDIT;
    else if (g_elen) g_mode = is_value_start(g_edit[0]) ? M_VALUE : M_LABEL;
    paint_panel();
    paint_edit_line();
}

static void key(int32_t message, int32_t mods)
{
    char ch = (char)(message & 0xFF);
    uint8_t vk = (uint8_t)((message >> 8) & 0xFF);
    int mv = 0;   /* 1 Up 2 Down 3 Left 4 Right 5 PgUp 6 PgDn 7 Tab 8 Shift-Tab */

    if (ch==3 || vk==0x46) {ready();return;}
    /* 123.RI 0x5a5a, verbatim; mint keys 06_f5_prompt (+ current address). */
    if (vk==VK_F5 && !ch && g_mode==M_READY) {prompt(A_GOTO,"Enter address to go to: ");return;}
    if (g_mode >= M_MAIN) { command_key(ch, vk); return; }
    if (vk == VK_F2 && !ch) { edit_key(); return; }

    if (ch == 0) {
        mv = vk == VK_UP ? 1 : vk == VK_DOWN ? 2 : vk == VK_LEFT ? 3 : vk == VK_RIGHT ? 4 :
             vk == VK_PGUP ? 5 : vk == VK_PGDN ? 6 : 0;
        if (vk == VK_F9 && g_mode == M_READY) { calc_key(); return; }
        if (vk == VK_HOME && g_mode == M_READY) { here(0, 0); return; }
    }
    if (vk == VK_TAB && ch == '\t') mv = (mods & FLAIR_EVT_MOD_SHIFT_KEY) ? 8 : 7;
    if (mv) {
        /* Typing, a pointer-movement key completes the entry and moves (p.
         * 1-16; mint keys 67 "abc" Tab -> I1, 68 "xyz" PgDn -> A21). In EDIT
         * only Up/Down/PgUp/PgDn complete (pp. 1-8/1-10, mint keys 49);
         * Left/Right/Tab move its cursor (not implemented, see edit_key). */
        if (g_mode == M_EDIT && (mv == 3 || mv == 4 || mv > 6)) return;
        if (g_mode != M_READY && finish() != 0) return;   /* a bad value stays in EDIT */
        if (mv > 6) page_cols(mv == 7);
        else if (mv > 4) page_rows(mv == 6);
        else move_by(mv == 3 ? -1 : mv == 4 ? 1 : 0, mv == 1 ? -1 : mv == 2 ? 1 : 0);
        return;
    }
    if (vk == VK_ESC || ch == 0x1B) {
        if (g_mode != M_READY) {
            /* EDIT: Esc "Erases all characters in entry" (p. 1-23); Esc on the
             * empty line returns to READY, the cell unchanged (mint keys
             * 03_f2_esc1 / 04_f2_esc2; 51_bad_value_esc after a bad value). */
#ifndef I123_MUT_EDIT_ESC_CANCELS
            if (g_mode != M_EDIT || !g_elen)
#else
            /* MUTANT (Rule 6; test-i123-keys-mutant): one Esc leaves EDIT. */
#endif
                g_mode = M_READY;
            g_elen = 0;
            g_edit[0] = '\0';
            paint_panel();
            paint_edit_line();
        }
        return;
    }
    if (g_mode == M_READY) {
        if (ch == '/' || ch == '<') { g_mode = M_MAIN; g_level=0;g_depth=0; g_menu_index = 0; command_paint(); return; }
        if (ch < ' ' || ch > '~') return;
        g_mode = is_value_start(ch) ? M_VALUE : M_LABEL;
        g_edit[0] = ch;
        g_elen = 1;
        g_edit[g_elen] = '\0';
        paint_panel();
        paint_edit_line();
        return;
    }
    if (vk == VK_ENTER || ch == '\r') {
        (void)finish();
        return;
    }
    if (vk == VK_BSPC || ch == 0x08) {
        if (g_elen > 0u) g_elen--;
        g_edit[g_elen] = '\0';
        paint_edit_line();
        return;
    }
    if (ch >= ' ' && ch <= '~') {
        /* Ref: manual pp. 1-16/1-17 cap; B13 brief specifies beep/ignore
         * for excess input. spec/cp_dialog.h supplies the speaker gate. */
        if (g_elen >= ENTRY_LIMIT) { (void)tbx_trap(TBX_BEEP, 0, 0); return; }
        g_edit[g_elen++] = ch;
        g_edit[g_elen] = '\0';
        paint_edit_line();
    }
}

/* ---- the mouse (B31, bead initech-umfi; astra audit L017) -------------------
 * A mouseDown in the worksheet area selects the visible cell under it. The
 * R2.2 manual is mouse-less: direct selection is the native document-window
 * expectation of the audit, AUTHORED (from memory, no local reference).
 * The screen it reads is pp. 1-3/1-4 (PDF 19/20): the control panel (lines
 * 0..2), the column letters in the top border, the row numbers in the left
 * border, the status line -- none of these selects anything. The cell comes
 * from THE geometry paint uses: the window's first column/row (Tab, PgDn,
 * GOTO and the arrows move it), per-column widths walked over area_cols()'s
 * WHOLE columns (paint_border's walk; B06: an oversized first column at its
 * visible width), VIS_ROWS rows; the blank strip right
 * of the last whole column selects nothing. Since B12 (bead initech-89rc)
 * those are THE GEOMETRY of the window's actual content, the same numbers
 * paint and pointer-follow use, so after a resize a click maps to what the
 * window now shows (a click on the moved-up status line selects nothing).
 * A click while an entry is typed or edited completes it first, as a
 * pointer-movement key does (manual p. 1-16, PDF 32: "simultaneously
 * complete the entry and move the cell pointer"); EDIT completes too
 * (AUTHORED: the click is not one of EDIT's in-line cursor keys); a value
 * that does not parse stays in EDIT and the pointer stays. Menus, prompts
 * and notices ignore clicks. */
static void cell_click(const tbx_event_t *ev)
{
    int32_t org = tbx_content_origin(g_win);
    int ch = (ev->whereh - (int16_t)org - X0) / CW - BORDER_W;
    int line = (ev->wherev - (org >> 16)) / CH - LINE_ROW0;
    uint16_t c = i123_view_left(&g_sheet), r = i123_view_top(&g_sheet);
#ifdef I123_MUT_HIT_IGNORES_SCROLL
    /* MUTANT (Rule 6; test-i123-hit-mutant, test-flair-i123-hit-mutant): the
     * hit maps as if the window sat at A1. NEVER real. */
    c = 0; r = 0;
#endif
    if (org < 0 || g_mode >= M_MAIN || ch < 0 || line < 0 || line >= (int)VIS_ROWS) return;
    r = (uint16_t)(r + line);
    if (r >= I123_MAX_ROWS) return;
    for (uint32_t n = area_cols(); n; n--, c++)
        if ((ch -= (int)vis_width(c)) < 0) {
            if (g_mode == M_READY || !finish()) here(c, r);
            return;
        }
}

/* ---- THE EXIT GUARD (bead initech-fvnj; astra audit L008) --------------------
 * Every user route out of the session comes HERE: File > Quit and its Ctrl-Q
 * (one TBX_EVT_MENU, spec/toolbox_gate.h Sec 6a), the window's close box (an
 * asking close box, TBX_GOAWAY_ASK -> TBX_EVT_GOAWAY, Sec 11) and /Quit Yes.
 * Unsaved work = a committed edit not yet in a file (g_dirty) OR an entry
 * still being typed (LABEL / VALUE / EDIT: the R2.2 modes in which keystrokes
 * are building a cell). A clean sheet ends at once -- through TBX_EXIT, the
 * normal teardown, never suppressed. Unsaved work asks with the Toolbox alert
 * (Sec 11), whose answer arrives as TBX_EVT_ALERT (quit_answer). If the
 * alert cannot be shown the session does NOT end (fail safe, Rule 2: a
 * refused question is never a silent discard) and the status line says so. */
static int entry_pending(void)
{
#ifdef I123_MUT_PENDING_IGNORED
    /* MUTANT (Rule 6; test-i123-quit-mutant Q5): the entry being typed is not
     * counted as work, so Quit throws it away silently. NEVER real. */
    return 0;
#else
    return g_mode == M_LABEL || g_mode == M_VALUE || g_mode == M_EDIT;
#endif
}

static void request_quit(void)
{
    if (!g_dirty && !entry_pending()) { (void)tbx_exit(0); return; }
    if (g_asking) return;
    if (tbx_alert(QUIT_TEXT, "Save", "Cancel", "Don't Save") != TBX_OK) {
        copy_text(g_status, "Quit cancelled: the save question could not be shown");
        panel_line(LINE_STATUS, g_status);
        return;
    }
    g_asking = 1;
}

/* The alert's answer (spec Sec 11: 1 default = Save, 2 cancel, 3 other). The
 * shell has already repainted what the alert covered (updateEvt). Anything
 * but Save / Don't Save is Cancel: the safe reading. */
static void quit_answer(int32_t item)
{
    g_asking = 0;
    if (item == (int32_t)TBX_ALERT_OTHER) { (void)tbx_exit(0); return; }   /* Don't Save */
    if (item != (int32_t)TBX_ALERT_DEFAULT) {
#ifdef I123_MUT_CANCEL_QUITS
        /* MUTANT (Rule 6; test-i123-quit-mutant Q3): Cancel ends the session
         * anyway. NEVER real. */
        (void)tbx_exit(0);
#endif
        return;      /* Cancel: the sheet, the mode and the typed entry stay */
    }
    /* Save. A pending entry is part of the work: store it first, exactly as
     * Enter would; a value that does not parse drops into EDIT (1-2-3's beep)
     * and the quit is off. Authored policy, no local reference. */
    if (entry_pending() && finish() != 0) return;
    /* THE /File Save flow of initech-tdnl.91, unchanged (the prompt offers the
     * current file; an existing file asks Cancel / Replace / Backup). Its
     * success ends the session (write_sheet); every other way out keeps it. */
    g_quit_after_save = 1;
    g_mode = M_SAVE; g_menu_index = 1; g_nlen = 0; g_name[0] = 0;
    command_paint();
}

/* ---- the application ------------------------------------------------------- */
int tbx_app_main(void)
{
    if (tbx_register() != TBX_OK) return 1;
    if (i123_sheet_init(&g_sheet, g_mem[0], (uint32_t)sizeof g_mem[0], SHEET_CELLS) != I123_OK ||
        i123_calc_init(&g_cx_nest, g_nest_mem, (uint32_t)sizeof g_nest_mem, 16u, (i123_calc_t *)0) != I123_OK ||
        i123_calc_init(&g_cx, g_calc_mem, (uint32_t)sizeof g_calc_mem, SHEET_CELLS, &g_cx_nest) != I123_OK) {
        (void)tbx_exit(1);
        return 1;
    }
    {
        /* The x87 control word Initech 123 computes under (ADR-0009
         * Amendment DEC-01a Sec 4, settled by test-i123-formula): 0x037F,
         * the kernel's boot word -- all exceptions masked, 64-bit
         * significand, round to nearest. Loaded explicitly: the policy is
         * the application's. */
        static const uint16_t cw = 0x037Fu;
        __asm__ __volatile__ ("fldcw %0" : : "m"(cw));
    }
#ifdef I123_MENU_DISK
    int32_t mh=tbx_file_open("A:\\APPS\\123.MNU",TBX_FILE_READ_ONLY);
    uint32_t got=0;int32_t mn=0;
    if(mh>=0) {
        do {mn=tbx_file_read(mh,g_wk+got,(uint32_t)sizeof g_wk-got);if(mn>0)got+=(uint32_t)mn;}while(mn>0&&got<sizeof g_wk);
        if(tbx_file_close(mh)<0)mn=-1;
    }
    if(mh<0||mn<0||i123_menu_load(g_wk,got)) {(void)tbx_exit(1);return 1;}
#else
    i123_menu_init();
#endif
    (void)tbx_set_mbar(&mbar);
    /* initech-fvnj: the close box ASKS (spec Sec 11) -- the guard decides. */
    g_win = tbx_new_window(WIN_L, WIN_T, WIN_R, WIN_B, (int32_t)TBX_GOAWAY_ASK);
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
        /* B12: the shell may have grown or shrunk the window since the last
         * update (MTE Listing 4-14 MyResizeWindow: adjust the contents to the
         * new size, then draw): re-read the content, keep the pointer in the
         * view it allows, then paint. */
        geometry();
        (void)follow_pointer();
        paint_all();
        break;
    case keyDown:
    case autoKey:
        key(ev->message, ev->modifiers);
        break;
    case mouseDown:
        cell_click(ev);
        break;
    case TBX_EVT_MENU:
        if (ev->message == TBX_MENU_WORD(MENU_FILE, ITEM_QUIT)) request_quit();
        break;
    case TBX_EVT_GOAWAY:
        request_quit();
        break;
    case TBX_EVT_ALERT:
        quit_answer(ev->message);
        break;
    default:
        break;
    }
}
