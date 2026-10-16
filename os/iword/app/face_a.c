/*
 * os/iword/app/face_a.c -- WORD.EXE, InitechWord's Face A: the WordPerfect
 * 5.1 editing screen, drawn natively in a FLAIR window by a disk-launched
 * application (THE ARTIFACT).
 *
 * bead: initech-l4tj (IWORD P1, Face A skeleton); initech-jojk (P2 slice 1:
 *       attribute codes + Reveal Codes; slice 2: tabs, indents, hard pages,
 *       Block / Move, the "Delete [code]?" question). Epic initech-fdxa.
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
 * Manual, Caps Lock "Status Line", PDF 74). The window is 76 x 24 cells
 * (FLAIR_SCREEN 640 x 480 less the menu bands and the frame), so 23 lines
 * of text show where the DOS screen shows 24; the 65-column line (1"/1"
 * margins at 10 pitch, spec/iword/wp5_format.h) and the whole status line
 * fit. WordPerfect's blue field is the Toolbox's NAVY with WHITE text
 * (spec/toolbox_gate.h Sec 7 tokens); the cursor is the cell in reverse
 * video (WP's is the hardware underline; a cell API has no underline --
 * authored adaptation). A hard page shows as a row of '=' BELOW its line,
 * a screen row of its own that is no document line (MINT03.shots/
 * 02_typed.png: the empty hard-page line, then the bar; ../wordperfect51-
 * decomp mint/shots_wpMINT09/08_edit_top.png: "Page one." then the bar).
 *
 * KEYS. Typing, Enter (hard return), Backspace, Delete, the arrows, End (the
 * end of the line) and WP's HOME PREFIX (slice 3, below). Word wrap as
 * WordPerfect stores it (os/iword/core/edit.c).
 * ATTRIBUTE CODES (bead initech-jojk, P2 slice 1; os/iword/core/codes.c):
 *   F6 Bold, F8 Underline -- "both an on and off code are inserted, with the
 *                   cursor resting just before the off code"; the key again
 *                   moves past it (Reference Manual, Attributes p. 36-37).
 *   Ctrl-F8 Font    the Font menu on the status line, then 1 Size (all
 *                   seven), 2 Appearance (all nine) or 3 Normal; the menu
 *                   lines and the sixteen attribute numbers are the real WP
 *                   5.1's (spec/iword/reveal_codes.h; slice 3, MINT28). Base
 *                   Font and Print Color (their own screens) return to the
 *                   document.
 *   Alt-F3 Reveal   the split screen: the document, the status line, the
 *   Codes           ruler, the codes pane (every code by its label, the
 *                   cursor's unit highlighted; 10 rows, the 11th blank as
 *                   WP leaves it, MINT16) and "Press Reveal Codes to restore
 *                   screen". In Reveal Codes, Backspace / Delete on an
 *                   attribute code delete it and its partner (MINT08); on the
 *                   editing screen WP asks "Delete [BOLD]? No (Yes)" (the
 *                   code's label; MINT11 01_bs_prompt / 02_del_prompt): y
 *                   deletes the pair, any other key steps over the code.
 * SLICE 2 (bead initech-jojk; the real WP.EXE in ../wordperfect51-decomp
 * mint/wpMINT09..16.steps, re/mint-results-003.md; os/iword/core/block.c):
 *   Tab, F4         [Tab] / [->Indent]: to the next tab stop; an indent
 *                   holds the rest of its paragraph at that margin (MINT09).
 *   Ctrl-Enter      a hard page: the bar below its line (MINT09).
 *   Alt-F4 Block    on / off; "Block on" at the left of the status line
 *                   (MINT12 01_block_on; WP blinks it, drawn steady here),
 *                   the block from the anchor to the cursor highlighted
 *                   (WP's red band; the Toolbox has GRAY: BLACK on GRAY,
 *                   authored), the cursor cell outside it. While Block is on
 *                   only the cursor keys, Alt-F4, Ctrl-F4, Delete /
 *                   Backspace, Alt-F3 and F1 / Esc (Block off) act.
 *   Ctrl-F4 Move    with Block on: "Move: 1 Block; 2 Tabular Column; 3
 *                   Rectangle: 0", then 1: "1 Move; 2 Copy; 3 Delete; 4
 *                   Append: 0" (MINT12 03/04). 1 Move cuts and 2 Copy copies
 *                   the block, then "Move cursor; press Enter to retrieve."
 *                   (05_move_prompt, with Doc .. Pos at the right): the
 *                   cursor keys move, Enter puts it in front of the cursor;
 *                   3 Delete deletes it. Tabular Column, Rectangle and
 *                   Append are later (any other key: back).
 *   Delete / Bksp   with Block on: "Delete Block? No (Yes)" (09_delete_prompt):
 *                   y deletes it, any other key keeps it.
 * SLICE 3 (bead initech-jojk; the real WP.EXE in ../wordperfect51-decomp
 * mint/wpMINT21..24.steps, re/mint-results-004.md):
 *   Shift-F8 Format the Format screen (mint/shots_wpMINT21/01_format_menu),
 *                   1 Line: the Format: Line screen (02_format_line) with the
 *                   values at the cursor; 3 Justification "1 Left; 2 Center;
 *                   3 Right; 4 Full: 0" on the bottom row (MINT22
 *                   02_just_menu); 7 Margins: the Left then the Right field
 *                   take a measurement in inches (04_left_typed "1.3_",
 *                   05_right_field), Enter after each; F7 back to the
 *                   document, Enter / 0 back to Format, Enter / 0 there to
 *                   the document. The other selections are later slices
 *                   (the key is ignored). Each setting goes in as WP puts
 *                   it (os/iword/core/edit.c iw_ed_margins / iw_ed_just).
 *   The document sits on the paper as WP shows it: screen column 0 is 1"
 *                   whatever the margins (MINT21 text at column 3 under
 *                   1.3"), a centered / right-justified line where the
 *                   justification puts it (MINT22 04_typed), Pos the paper
 *                   position (MINT23 "Pos 1.25""); the Reveal Codes ruler
 *                   follows the margins at the cursor (spec/iword/
 *                   reveal_codes.h "the ruler").
 *   6 Line Spacing  a number, Enter (MINT25 01/02, MINT30): [Ln Spacing:]
 *                   at the cursor. A line takes its spacing rounded to the
 *                   nearest whole row, the extra rows blank below it, and the
 *                   screen scrolls by rows (MINT25 03_typed / 04_reveal,
 *                   MINT26, MINT30); Ln counts the spacing (MINT25 2.83").
 *   Home            a PREFIX (Reference Manual p. 103-104; MINT27): Home,
 *                   Left / Home, Home, Left the line's first character past
 *                   its codes, Home x3, Left before them, Home, Right the
 *                   line's end, Home, Up / Home, Down the screen's top /
 *                   bottom line, Home, Home, Up / Down the document's top
 *                   (after its formatting codes; x3 before them) / end.
 *   Block + F6, F8, Ctrl-F8 ("Attribute: 1 Size; 2 Appearance: 0")
 *                   the attribute given to the block (p. 37; MINT28).
 *   Ctrl-F4         with Block off: "Move: 1 Sentence; 2 Paragraph; 3 Page;
 *                   4 Retrieve: 0" -- 1..3 highlight the text and offer the
 *                   block's menu (MINT31), 4 "Retrieve: 1 Block; ..." puts
 *                   the last moved / copied text back (MINT29 14, MINT31 15).
 * The text between the codes is drawn in its face through DRAWCELLS (bold,
 * italic, underline synthesized by the Font Manager rules, spec/
 * toolbox_gate.h Sec 7a STYLE). WordPerfect itself shows attributes as
 * colours on a colour text screen (MINT01.shots/02_typed.png: bold bright
 * white, underline on grey; MINT06: italic yellow); the four Toolbox colour
 * tokens cannot, so Face A draws the face itself -- authored adaptation. The
 * ruler band is BLACK on GRAY where WP's is red (same reason).
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
#include "../../../spec/iword/fkeys.h"
#include "iword.h"
#include "iword/wp5_format.h"
#include "iword/reveal_codes.h"
#include "iword/settings.h"

const char tbx_app_name[] = "WORD";

/* ---- geometry (the 123.EXE window, os/i123/app/face_a.c) ------------------ */
#define WIN_L        0
#define WIN_T        40
#define WIN_R        640
#define WIN_B        480
#define CW           8           /* TBX_CELL_W */
#define CH           16          /* TBX_CELL_H */
/* RE-KEYED 2026-10-16 (bead initech-tdnl.51, stated): 613-px content since
 * the content starts inside the left body rail (frame.left + 6), so 76 whole
 * cells (was 77, 618 px); X0 stays 1. */
#define SCR_COLS     76
#define SCR_LINES    24
#define X0           1
#define EDIT_ROWS    (SCR_LINES - 1)     /* 23 */
#define LINE_STATUS  (SCR_LINES - 1)
/* RE-KEYED 2026-10-16 (bead initech-tdnl.51, stated): one column left of the
 * measured WordPerfect columns (Doc..Ln ending at 67, "Pos" at 69) because the
 * field is 76 cells now, not 77: "Pos 2.3"" (8 cells) must still end inside
 * it. The group keeps its measured shape and spacing. */
#define STATUS_DOC_END 66                /* "... Ln 1"" ends at column 66 */
#define STATUS_POS   68                  /* "Pos" at column 68            */
/* REVEAL CODES (Alt-F3; spec/iword/reveal_codes.h). WordPerfect's 25-line
 * screen splits into the document (lines 0-10), the status line (11), the
 * ruler (12), the 11-row codes pane (13-23) and "Press Reveal Codes to
 * restore screen" (24). The window has 24 lines, so the document part is one
 * line shorter (10 rows) and the rest is WordPerfect's, row for row. */
#define RV_TOP_ROWS  10
#define RV_STATUS    10
#define RV_RULER     11
#define RV_PANE      12                  /* rows 12..22 */
#define RV_MESSAGE   23

#define FG           TBX_COLOR_WHITE
#define BG           TBX_COLOR_NAVY

#define MENU_FILE    129
#define ITEM_RETRIEVE 1
#define ITEM_SAVE    2
#define ITEM_EXIT    3

/* PS/2 set-1 make codes (event.c cooks message = vkey << 8 | ascii) */
#define VK_ESC       0x01
#define VK_BSPC      0x0E
#define VK_TAB       0x0F
#define VK_ENTER     0x1C
#define VK_F1        0x3B
#define VK_F2        0x3C
#define VK_F3        0x3D
#define VK_F4        0x3E
#define VK_F6        0x40
#define VK_F7        0x41
#define VK_F8        0x42
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
static uint32_t      g_skip;                /* ... and how many of its rows are
                                             * above the screen (a spaced line's
                                             * blank rows; slice 3, MINT25
                                             * 04_reveal: WP scrolls by rows) */

/* ---- the screen ----------------------------------------------------------
 * A row is its text, the face of every cell (IW_A_* == the txFace bits the
 * Toolbox synthesizes: bold 1, italic 2, underline 4), its colours and one
 * reverse-video run (the cursor). A row is drawn only when it differs from
 * what the screen has: the whole row plain in one DRAWCELLS run, then every
 * styled stretch over it in its face (fg | face << 8, spec/toolbox_gate.h
 * Sec 7/7a), then the reverse run -- so a row with no codes draws exactly as
 * P1 drew it. */
typedef struct row {
    char    t[SCR_COLS + 1];
    uint8_t a[SCR_COLS];
    uint8_t fg, bg, inv_col, inv_len;
    uint8_t blk_col, blk_len;           /* the block's run (BLACK on GRAY) */
} row_t;
static int32_t  g_win TBX_IN_IMAGE = 0;
static char     g_line[SCR_COLS + 1] TBX_IN_IMAGE = { 0 };
static row_t    g_shadow[SCR_LINES];
static uint8_t  g_shadow_ok[SCR_LINES];
static uint8_t  g_caps;
static uint8_t  g_reveal;                   /* Alt-F3: Reveal Codes is on */
static uint8_t  g_clip[BANK];               /* the Move / Copy buffer        */
static uint32_t g_clip_n;

/* ---- prompts --------------------------------------------------------------- */
enum { M_EDIT, M_SAVE_NAME, M_GET_NAME, M_REPLACE, M_SAVEQ, M_EXITQ, M_ERROR, M_FONT, M_APPEAR,
       M_DELCODE, M_DELBLOCK, M_MOVE, M_MOVEBLK, M_RETRIEVE,
       M_FORMAT, M_FMTLINE, M_FMTJUST, M_FMTMARL, M_FMTMARR,
       M_SIZE, M_FONTBLK, M_MOVENB, M_RETMENU, M_FMTSP, M_GRAY, M_SEARCH,
       M_RCONF, M_RSEARCH, M_RTEXT, M_RASK, M_UNDO,
       M_SETUP, M_ENV, M_UNITS, M_UNITVALUE, M_BACKUP, M_BACKUPVALUE };
static int      g_mode = M_EDIT;
static int      g_err_return;               /* the prompt an ERROR returns to */
static const char *g_err;
/* WP 5.1 Reference pp. 963-966, Template pp. 5-6. Unavailable functions
 * show the manual's name in the period-grayed status line. The six unmapped
 * modified F11/F12 slots have no command name (p. 966 note 4). */
static const char *g_gray;
static uint8_t g_search_back;
/* Search strings are unit streams, distinct from the document's paired-code
 * insertion model. F6 twice gives [BOLD][bold] (Reference p. 572). */
#define QUERY_CAP 256u
static uint8_t g_query[QUERY_CAP];
static uint32_t g_query_n, g_query_cur;
static uint8_t g_query_fresh;
/* Replace: Reference pp. 551-553. Input streams keep complete code units.
 * Search and replacement are separate; inserted text is never searched again. */
static uint8_t g_find[QUERY_CAP], g_replacement[QUERY_CAP];
static uint32_t g_find_n, g_replacement_n, g_rs, g_re;
static uint8_t g_confirm;
/* Reference pp. 697-698: three deletion GROUPS, delimited by cursor motion.
 * Backspace/Delete paired attribute codes are deliberately not retained.
 * Block copies use the MINT12..15 self-contained unit stream. */
/* Fixed arenas respect the existing 512 KiB tenant BSS contract. A group
 * beyond this arena is refused before deletion, never silently discarded. */
#define UNDO_CAP (BANK / 2u)
static uint8_t g_undo[3][UNDO_CAP], g_deleted[UNDO_CAP];
#define g_preview (g_bank[1u - g_live])
static uint32_t g_undo_n[3], g_undo_count, g_undo_sel, g_undo_at;
static uint8_t g_undo_join;
static uint8_t g_settings[IW_SETTINGS_SIZE], g_settings_io[IW_SETTINGS_SIZE+1];
static uint8_t g_unit_target;
static uint32_t query_label(char *out, uint32_t cap);
static char     g_name[16];                 /* the name being typed        */
static uint32_t g_nlen;
static uint8_t  g_fresh;                    /* the offered name: a key replaces it */
static char     g_path[TBX_FILE_PATH_MAX];  /* "NAME" for the file verbs     */
static char     g_current[24];              /* "A:\NAME" once saved/retrieved */
static char     g_shown[24];                /* "A:\NAME" of the name being used */
static uint8_t  g_exiting;                  /* the save is part of F7 Exit  */
/* the Format: Line margin fields (Shift-F8 1 7): the text typed so far, the
 * left value taken (WPU), whether anything was typed */
static char     g_field[12];
static uint32_t g_flen;
static uint32_t g_fleft, g_fright;
static uint8_t  g_ftyped;
static uint32_t g_fsp;                      /* the spacing typed (8.8 fixed) */
static uint8_t  g_home;                     /* Home presses pending: WP's Home
                                             * is a prefix key (slice 3)     */
static uint8_t  g_sel;                      /* the block is Ctrl-F4 1/2/3's  */
static uint32_t g_hin;                      /* the Homes pressed before this key */

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

static void row_clear(row_t *r)
{
    for (uint32_t i = 0; i < SCR_COLS; i++) { r->t[i] = ' '; r->a[i] = 0; }
    r->t[SCR_COLS] = 0;
    r->fg = (uint8_t)FG;
    r->bg = (uint8_t)BG;
    r->inv_col = r->inv_len = 0;
    r->blk_col = r->blk_len = 0;
}

/* Draw row `line` as `r` unless the screen has it. */
static void put_row(int line, const row_t *r)
{
    const row_t *o = &g_shadow[line];
    int same = g_shadow_ok[line] && o->fg == r->fg && o->bg == r->bg &&
               o->inv_col == r->inv_col && o->inv_len == r->inv_len &&
               o->blk_col == r->blk_col && o->blk_len == r->blk_len;
    uint32_t c;
    for (c = 0; same && c < SCR_COLS; c++) if (o->t[c] != r->t[c] || o->a[c] != r->a[c]) same = 0;
    if (same) return;
    g_shadow[line] = *r;
    g_shadow_ok[line] = 1;
    put_cells(line, 0, r->t, SCR_COLS, r->fg, r->bg);
    for (c = 0; c < SCR_COLS; ) {
        uint8_t f = r->a[c];
        uint32_t s0 = c;
        if (!f) { c++; continue; }
        while (c < SCR_COLS && r->a[c] == f) c++;
        put_cells(line, (int)s0, r->t + s0, c - s0, (uint32_t)r->fg | TBX_TEXT_FACE(f), r->bg);
    }
    if (r->blk_len) put_cells(line, r->blk_col, r->t + r->blk_col, r->blk_len,
                             g_mode == M_UNDO ? BG : TBX_COLOR_BLACK,
                             g_mode == M_UNDO ? FG : TBX_COLOR_GRAY);
    if (r->inv_len) put_cells(line, r->inv_col, r->t + r->inv_col, r->inv_len, r->bg, r->fg);
}

/* A row of plain text (the status line, the messages). */
static void put_text_row(int line, const char *s)
{
    static row_t r;
    row_clear(&r);
    for (uint32_t i = 0; s[i] && i < SCR_COLS; i++) r.t[i] = s[i];
    put_row(line, &r);
}

/* ---- the document's bytes in a bank ---------------------------------------- */
static void doc_reset(void)
{
    iw_doc_init(&g_ed.doc, g_add, ADD_CAP, g_pc, PIECES, BANK - IW_WP5_DEFAULT_DOCSTART);
    iw_ed_init(&g_ed, IW_WP5_WRAP_COLS);
    g_prefix = iw_wp5_default_prefix(&g_prefix_len);
    g_top = 0;
    g_skip = 0;
    g_undo_count = 0; g_undo_join = 0;
}

/* Flatten prefix + body into the spare bank; *n = its length. */
static int flatten(uint32_t *n)
{
    /* the prefix records whether the cursor is before the end (0x103) */
    return iw_wp5_write(g_prefix, g_prefix_len, &g_ed.doc, g_ed.cur, g_bank[1u - g_live], BANK, n);
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
enum { ED_TYPE, ED_ENTER, ED_BACKSPACE, ED_DELETE, ED_ATTR, ED_NORMAL, ED_TAB, ED_INDENT,
       ED_HPAGE, ED_ANSWER, ED_CUT, ED_COPY, ED_RETRIEVE, ED_MARGINS, ED_JUST, ED_SPACING };
static int edit_once(int op, uint8_t ch)
{
    switch (op) {
    case ED_TYPE:      return iw_ed_type(&g_ed, ch);
    case ED_ENTER:     return iw_ed_enter(&g_ed);
    case ED_BACKSPACE: return iw_ed_backspace(&g_ed);
    case ED_ATTR:      return iw_ed_block_attr(&g_ed, ch); /* ch = attribute number;
                                                          * Block on: given to it */
    case ED_NORMAL:    return iw_ed_normal(&g_ed);
    case ED_TAB:       return iw_ed_tab(&g_ed);
    case ED_INDENT:    return iw_ed_indent(&g_ed);
    case ED_HPAGE:     return iw_ed_hpage(&g_ed);
    case ED_ANSWER:    return iw_ed_answer(&g_ed, ch);   /* ch = yes */
    case ED_CUT:       return iw_ed_block_cut(&g_ed, ch ? g_clip : 0, sizeof g_clip, &g_clip_n);
    case ED_COPY:      return iw_ed_block_copy(&g_ed, g_clip, sizeof g_clip, &g_clip_n);
    case ED_RETRIEVE:  return iw_ed_retrieve(&g_ed, g_clip, g_clip_n);
    case ED_MARGINS:   return iw_ed_margins(&g_ed, g_fleft, g_fright);
    case ED_JUST:      return iw_ed_just(&g_ed, ch);      /* ch = IW_WP5_JUST_* */
    case ED_SPACING:   return iw_ed_spacing(&g_ed, g_fsp);
    default:           return iw_ed_delete(&g_ed);
    }
}
static int edit(int op, uint8_t ch)
{
    uint32_t before = g_ed.doc.len;
    uint32_t ds = g_ed.cur, dt = ds, dn = 0;
    int rc, fx;
    ensure_room();
    if (op == ED_BACKSPACE && ds) ds = iw_unit_prev(&g_ed.doc, ds);
    else if (op == ED_DELETE && dt < before) dt = iw_unit_next(&g_ed.doc, dt);
    else if (op == ED_ANSWER && ch) { ds = g_ed.ask; dt = iw_unit_next(&g_ed.doc, ds); }
    else if (op == ED_CUT && !ch && g_ed.blk != IW_NO_BLOCK) {
        uint32_t anchor = g_ed.blk;
        iw_ed_block_span(&g_ed, &ds, &dt);
        int copied = iw_ed_block_copy(&g_ed, g_deleted, UNDO_CAP, &dn);
#ifndef IWORD_MUT_UNDO_BLOCK
        g_ed.blk = anchor;
#else
        (void)anchor;
#endif
        if (copied != IW_OK) return IW_E_FULL;
    }
    if (dt > ds && !dn) {
        if (dt-ds > UNDO_CAP) return IW_E_FULL;
        /* Paired attributes removed with Backspace/Delete are not restored
         * (p. 697). Other complete units retain their P2 byte identity. */
        for (uint32_t p = ds; p < dt; p = iw_unit_next(&g_ed.doc, p)) {
            uint32_t n = iw_unit_next(&g_ed.doc, p)-p;
            int c = iw_doc_byte(&g_ed.doc, p);
            if (c == 0xC3 || c == 0xC4) continue;
            dn += iw_doc_read(&g_ed.doc, p, g_deleted+dn, n);
        }
    }
    if (dn && g_undo_join && g_undo_count && (ds == g_undo_at || dt == g_undo_at) &&
        g_undo_n[0] + dn > UNDO_CAP) return IW_E_FULL;
    rc = edit_once(op, ch);
    if (rc == IW_E_FULL) {
        compact();
        if (g_ed.doc.len == before) rc = edit_once(op, ch);
        else {
            rc = IW_OK;
            if (g_ed.cur > 0u) rc = iw_reflow_para(&g_ed.doc, g_ed.cur - 1u, g_ed.width);
            if (rc == IW_OK || rc == IW_E_FULL) rc = iw_reflow_para(&g_ed.doc, g_ed.cur, g_ed.width);
        }
    }
    /* WordPerfect's bookkeeping after every edit (iw_ed_fixup: the tab /
     * indent / hard-page geometry, C6, D4) -- idempotent, so a refusal for
     * room is compacted away and run again */
    fx = iw_ed_fixup(&g_ed);
    if (fx == IW_E_FULL) { compact(); fx = iw_ed_fixup(&g_ed); }
    if (rc == IW_OK && dn && g_ed.doc.len < before) {
        uint32_t limit = 3;
#ifdef IWORD_MUT_UNDO_LEVEL
        limit = 2;
#endif
        int join = g_undo_join && g_undo_count && (ds == g_undo_at || dt == g_undo_at);
        if (!join) {
            if (g_undo_count < limit) g_undo_count++;
            for (uint32_t j = g_undo_count-1; j; j--) {
                g_undo_n[j] = g_undo_n[j-1];
                for (uint32_t i = 0; i < g_undo_n[j]; i++) g_undo[j][i] = g_undo[j-1][i];
            }
            g_undo_n[0] = 0;
        }
        if (dn + g_undo_n[0] <= UNDO_CAP) {
            int prepend = join && dt == g_undo_at && ds < dt;
#ifdef IWORD_MUT_UNDO_ORDER
            prepend = 0;
#endif
            if (prepend) {
                for (uint32_t i = g_undo_n[0]; i; i--) g_undo[0][i+dn-1] = g_undo[0][i-1];
                for (uint32_t i = 0; i < dn; i++) g_undo[0][i] = g_deleted[i];
            } else for (uint32_t i = 0; i < dn; i++) g_undo[0][g_undo_n[0]+i] = g_deleted[i];
            g_undo_n[0] += dn; g_undo_at = ds; g_undo_join = 1;
        }
    }
    return rc == IW_OK ? fx : rc;
}

/* ---- painting ------------------------------------------------------------------ */
/* Line `ln` into row `r`; `*state` is the attribute state at ln->start and
 * leaves as the state at ln->end (the codes are walked, zero width). */
/* Line `ln` into row `r`, every unit at its column (a tab or an indent is
 * blank up to its stop; the line starts at its margin, ln->col0); `*state`
 * is the attribute state at ln->start and leaves as the state at ln->end
 * (the codes are walked, zero width). With Block on, the cells of the units
 * in [bs, bt) are the block's run. */
/* The screen column of relative column c on line ln (slice 3: the paper
 * position, screen column 0 at 1"). */
static uint32_t scol(const iw_line_t *ln, uint32_t c)
{
    return IW_SCREEN_COL(IW_LINE_X(ln, c));
}

static void row_text(const iw_line_t *ln, uint32_t *state, row_t *r, uint32_t bs, uint32_t bt)
{
    uint32_t p = ln->start, c = ln->col0, b0 = SCR_COLS, b1 = 0;
    row_clear(r);
    while (p < ln->end) {
        int k = iw_unit_kind(&g_ed.doc, p);
        uint32_t q = iw_unit_next(&g_ed.doc, p), c1 = c;
        if (p < ln->text_end) {
            uint32_t sc = scol(ln, c);
            if (k == IW_U_TEXT || k == IW_U_HSPACE) {
                if (sc < SCR_COLS) {
                    r->t[sc] = k == IW_U_TEXT ? (char)iw_doc_byte(&g_ed.doc, p) : ' ';
                    r->a[sc] = (uint8_t)iw_attr_faces(*state);
                }
                c1 = c + 1u;
            } else if (k == IW_U_TAB || k == IW_U_INDENT) {
                c1 = iw_line_col(&g_ed.doc, ln, q);      /* blank, plain */
            }
            if (p >= bs && p < bt && c1 > c) {
                if (sc < b0) b0 = sc;
                if (scol(ln, c1) > b1) b1 = scol(ln, c1);
            }
        }
        if (k == IW_U_CODE) *state = iw_attr_step(&g_ed.doc, p, *state);
        c = c1;
        p = q;
    }
    if (b1 > SCR_COLS) b1 = SCR_COLS;
    if (b0 < b1) { r->blk_col = (uint8_t)b0; r->blk_len = (uint8_t)(b1 - b0); }
}

/* The screen rows line `ln` takes: its own, the blank rows of its line
 * spacing below it -- the spacing rounded to the nearest whole row (spec/
 * iword/wp5_format.h "line spacing" SCREEN: MINT25 1.5 -> 2, MINT26 1.33 ->
 * 1, 3 -> 3; MINT30 2.25 -> 2) -- and a hard page's bar (MINT09 08_edit_top;
 * the bar after the blank rows is inferred, W-17). `last` = the document's
 * last line, which has nothing below it. */
static uint32_t line_rows(const iw_line_t *ln, int last, uint32_t *bar)
{
    uint32_t n = 1u;
    *bar = 0;
    if (last) return n;
    n += iw_spacing_rows(ln->sp) - 1u;
    if (ln->term == IW_U_HPG) { *bar = 1u; n++; }
    return n;
}

/* Keep the cursor's line among the `rows` screen rows, scrolling by rows as
 * WordPerfect does (MINT25 04_reveal: the window may start on a spaced
 * line's blank row; never on a hard page's bar -- a line and its bar go
 * together, as before slice 3). */
static void follow(const iw_where_t *w, uint32_t rows)
{
    static iw_line_t lines[EDIT_ROWS + 1];
    if (w->line < g_top || (w->line == g_top && g_skip)) { g_top = w->line; g_skip = 0; }
    if (w->line >= g_top + rows) { g_top = w->line - (rows - 1u); g_skip = 0; }
    for (;;) {
        uint32_t n = w->line - g_top, got, k, r = 0, bar;
        int32_t d;
        if (n > EDIT_ROWS) n = EDIT_ROWS;
        got = iw_ed_lines(&g_ed, g_top, n, lines);
        for (k = 0; k < got && k < n; k++) r += line_rows(&lines[k], 0, &bar);
        r -= g_skip;
        if (r < rows) return;
        d = (int32_t)(r - (rows - 1u));
        /* the top moves down d rows */
        while (d > 0) {
            uint32_t nr;
            (void)iw_ed_lines(&g_ed, g_top, 1u, lines);
            nr = line_rows(&lines[0], 0, &bar);
#ifdef IWORD_MUT_LINE_SCROLL
            /* MUTANT (Rule 6; test-iword-face-mutant): the window always
             * starts on a line, never on a blank row. NEVER real. */
            bar = nr - 1u;
#endif
            if (g_skip + (uint32_t)d <= nr - bar - 1u) { g_skip += (uint32_t)d; d = 0; }
            else { d -= (int32_t)(nr - g_skip); g_top++; g_skip = 0; }
        }
    }
}

/* The first and last lines whose text row is on the screen (Home, Up /
 * Home, Down). */
static void screen_lines(uint32_t rows, uint32_t *first, uint32_t *last)
{
    static iw_line_t lines[EDIT_ROWS + 1];
    uint32_t got = iw_ed_lines(&g_ed, g_top, rows + 1u, lines), k, bar;
    int32_t r = -(int32_t)g_skip;
    int any = 0;
    *first = *last = g_top;
    for (k = 0; k < got && r < (int32_t)rows; k++) {
        if (r >= 0) {
            if (!any) *first = g_top + k;
            *last = g_top + k;
            any = 1;
        }
        r += (int32_t)line_rows(&lines[k], k + 1u == got && got < rows + 1u, &bar);
    }
}

static void paint_status(const iw_where_t *w, int line)
{
    char s[SCR_COLS + 1], num[12], doc[32];
    uint32_t n, i, k, query_cursor = 0;
    for (i = 0; i < SCR_COLS; i++) s[i] = ' ';
    s[SCR_COLS] = 0;
    if (g_mode == M_EDIT || g_mode == M_RETRIEVE) {
        /* left: the document's name; "Block on" while Block is on (MINT12
         * 01_block_on); the Move prompt while a block waits (05_move_prompt) */
        const char *left = g_mode == M_RETRIEVE ? IW_MOVE_RETRIEVE :
                           g_ed.blk != IW_NO_BLOCK ? IW_BLOCK_ON : g_current;
#ifdef IWORD_MUT_NO_BLOCK_ON
        /* MUTANT (Rule 6; test-iword-face-mutant): Block on is not shown --
         * the document's name stays. NEVER real. */
        if (g_mode == M_EDIT) left = g_current;
#endif
        for (i = 0; left[i] && i < 40u; i++) s[i] = left[i];
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
        /* Ln from the line's height down the page (slice 3: the spacing,
         * MINT25 "Ln 2.83""; single spacing gives P1's 1" + lines / 6) */
        k = iw_fmt_inches(iw_ln_y_hundredths(w->y), num);
        for (i = 0; i < k; i++) doc[n++] = num[i];
        {
            /* Pos: the paper position (slice 3, MINT23 "Pos 1.25""; at 1"/1"
             * it is 1" + column / 10 as before). RE-KEYED 2026-10-16 (bead
             * initech-jojk slice 3, stated): a value with two decimals
             * ("Pos 3.85"", MINT32 03_typed) is one cell longer than the
             * 76-cell field leaves after column 68 -- the whole group (Doc ..
             * Ln and Pos) moves left by the overflow so the inch mark stays,
             * its shape and spacing kept (authored adaptation of WP's 80
             * columns, as tdnl.51's one-column move) */
            char pos[12];
            uint32_t pk = iw_fmt_inches(w->x / 12u, pos);
            uint32_t sh = STATUS_POS + 4u + pk > SCR_COLS ? STATUS_POS + 4u + pk - SCR_COLS : 0u;
            for (i = 0; i < n; i++) s[STATUS_DOC_END + 1 - sh - n + i] = doc[i];
            s[STATUS_POS - sh] = 'P';
            s[STATUS_POS - sh + 1] = g_caps ? 'O' : 'o';
            s[STATUS_POS - sh + 2] = g_caps ? 'S' : 's';
            for (i = 0; i < pk; i++) s[STATUS_POS - sh + 4 + i] = pos[i];
        }
        if (g_mode == M_RETRIEVE) {
            /* "Enter" bright in WP's prompt: drawn bold here */
            static row_t r;
            row_clear(&r);
            for (i = 0; i < SCR_COLS; i++) r.t[i] = s[i];
            for (i = 0; i < 5u; i++) r.a[IW_MOVE_ENTER_COL + i] = IW_A_BOLD;
            put_row(line, &r);
            return;
        }
    } else {
        const char *t = "";
        char replace[48];
        if (g_mode == M_SAVE_NAME) t = "Document to be Saved: ";
        else if (g_mode == M_GET_NAME) t = "Document to be Retrieved: ";
        else if (g_mode == M_SAVEQ) t = "Save Document? Yes (No)";
        else if (g_mode == M_EXITQ) t = "Exit WP? No (Yes)";
        else if (g_mode == M_ERROR) t = g_err;
        else if (g_mode == M_GRAY) t = g_gray;
        else if (g_mode == M_RCONF) {
            t = "w/Confirm? No (Yes)";
#ifdef IWORD_MUT_REPLACE_PROMPT
            t = "Confirm?";
#endif
        }
        else if (g_mode == M_RASK) t = "Replace? Yes (No)";
        else if (g_mode == M_UNDO) t = "Undelete 1 Restore; 2 Previous Deletion: 0";
        else if (g_mode == M_RTEXT) t = "Replace with:";
        else if (g_mode == M_SEARCH || g_mode == M_RSEARCH) {
            t = g_search_back ? "<- Srch:" : "-> Srch:";
#ifdef IWORD_MUT_SEARCH_PROMPT
            t = "Search:";
#endif
        }
        else if (g_mode == M_FONT) t = IW_FONT_MENU;
        else if (g_mode == M_FONTBLK) t = IW_FONT_BLK_MENU;   /* MINT28 07 */
        else if (g_mode == M_SIZE) t = IW_SIZE_MENU;           /* MINT28 11 / 18 */
        else if (g_mode == M_APPEAR) t = IW_APPEAR_MENU;
        else if (g_mode == M_MOVE) t = IW_MOVE_MENU;
        else if (g_mode == M_MOVENB) t = IW_MOVE_NB_MENU;      /* MINT31 02 */
        else if (g_mode == M_RETMENU) t = IW_RETRIEVE_MENU;    /* MINT29 14 */
        else if (g_mode == M_MOVEBLK) t = IW_MOVE_BLK_MENU;
        else if (g_mode == M_DELBLOCK) t = IW_DEL_BLOCK;
        else if (g_mode == M_DELCODE) {
            /* "Delete [BOLD]? No (Yes)": the code's own Reveal label */
            char lab[24];
            scopy(replace, IW_DEL_CODE_PRE);
            n = slen(replace);
            (void)iw_code_label(&g_ed.doc, g_ed.ask, lab, sizeof lab);
            for (i = 0; lab[i] && n < 40u; i++) replace[n++] = lab[i];
            scopy(replace + n, IW_DEL_CODE_POST);
            t = replace;
        }
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
        if (g_mode == M_SEARCH || g_mode == M_RSEARCH || g_mode == M_RTEXT)
            query_cursor = n + query_label(s + n, SCR_COLS - n + 1);
        if (g_mode == M_SAVE_NAME || g_mode == M_GET_NAME)
            for (i = 0; i < g_nlen && n < SCR_COLS; i++) s[n++] = g_name[i];
        if (g_mode == M_SAVEQ && !g_ed.dirty) {
            const char *nm = "(Text was not modified)";
            k = slen(nm);
            for (i = 0; i < k; i++) s[SCR_COLS - k + i] = nm[i];
        }
    }
    if (g_mode == M_GRAY || g_mode == M_SEARCH || g_mode == M_RSEARCH || g_mode == M_RTEXT) {
        row_t r;
        row_clear(&r);
        for (i = 0; i < SCR_COLS; i++) r.t[i] = s[i];
        if (g_mode == M_GRAY) r.fg = TBX_COLOR_GRAY;
        else {
            r.inv_col = (uint8_t)query_cursor; r.inv_len = 1;
#ifdef IWORD_MUT_SEARCH_CURSOR
            r.inv_len = 0;
#endif
        }
        put_row(line, &r);
    } else put_text_row(line, s);
}

/* The document rows 0 .. rows-1, the cursor in reverse video, a spaced
 * line's blank rows and a hard page's bar below its line, the block (if on)
 * in its run. */
static void paint_doc(const iw_where_t *w, uint32_t rows)
{
    static iw_line_t lines[EDIT_ROWS + 1];
    static row_t r;
    uint32_t got, li, i = 0, state = 0, bs = 1u, bt = 0, skip;
    int ccol;
    follow(w, rows);
    skip = g_skip;
    got = iw_ed_lines(&g_ed, g_top, rows + 1u, lines);
    if (got) state = iw_attr_state(&g_ed.doc, lines[0].start);
    if (g_ed.blk != IW_NO_BLOCK) iw_ed_block_span(&g_ed, &bs, &bt);
    ccol = (int)(IW_SCREEN_COL(w->x) < SCR_COLS ? IW_SCREEN_COL(w->x) : SCR_COLS - 1u);
    for (li = 0; li < got && i < rows; li++) {
        uint32_t bar, n = line_rows(&lines[li], li + 1u == got && got < rows + 1u, &bar), k;
        row_text(&lines[li], &state, &r, bs, bt);
        if (g_top + li == w->line) {
            r.inv_col = (uint8_t)ccol;
            r.inv_len = 1;
            /* the cursor cell is never in the block's band (MINT12 02/12a) */
            if (r.blk_len && ccol >= r.blk_col && ccol < r.blk_col + r.blk_len) {
                if (ccol == r.blk_col) { r.blk_col++; r.blk_len--; }
                else if (ccol == r.blk_col + r.blk_len - 1) r.blk_len--;
            }
        }
        for (k = 0; k < n && i < rows; k++) {
            if (skip) { skip--; continue; }
            if (k == 0) { put_row((int)i++, &r); continue; }
            row_clear(&r);
            if (bar && k == n - 1u)
                for (uint32_t c = 0; c < SCR_COLS; c++) r.t[c] = '=';
            put_row((int)i++, &r);
        }
    }
    while (i < rows) { row_clear(&r); put_row((int)i++, &r); }
}

/* Reveal Codes: the ruler, the codes pane and the restore message. */
static char g_pane[IW_RV_PANE_ROWS][SCR_COLS + 1];
static void paint_reveal(const iw_where_t *w)
{
    static row_t r;
    iw_pane_t pane;
    uint32_t i, x;
    row_clear(&r);
    r.fg = (uint8_t)TBX_COLOR_BLACK;           /* authored: WP's band is red */
    r.bg = (uint8_t)TBX_COLOR_GRAY;
    /* the tab marks every 0.5" from the left margin - 1" (relative tabs),
     * the margins as braces (on a stop) or brackets (spec/iword/
     * reveal_codes.h "the ruler"; MINT21 09_reveal, MINT23 03_reveal_mid) */
    x = w->lm >= 2u * IW_WP5_TAB_STEP_WPU ? w->lm - 2u * IW_WP5_TAB_STEP_WPU : w->lm % IW_WP5_TAB_STEP_WPU;
    for (; IW_SCREEN_COL(x) < SCR_COLS; x += IW_WP5_TAB_STEP_WPU)
        if (x >= IW_WP5_LEFT_WPU) r.t[IW_SCREEN_COL(x)] = (char)IW_RV_RULER_TAB;
    if (IW_SCREEN_COL(w->lm) < SCR_COLS) r.t[IW_SCREEN_COL(w->lm)] = IW_RV_RULER_LEFT;
    if (IW_SCREEN_COL(w->rm) < SCR_COLS)
        r.t[IW_SCREEN_COL(w->rm)] = (w->rm >= w->lm && (w->rm - w->lm) % IW_WP5_TAB_STEP_WPU == 0u)
                                    ? IW_RV_RULER_RIGHT : IW_RV_RULER_RBRACK;
    put_row(RV_RULER, &r);
    iw_reveal_pane(&g_ed, SCR_COLS, IW_RV_PANE_ROWS, &g_pane[0][0], SCR_COLS + 1u, &pane);
    for (i = 0; i < IW_RV_PANE_ROWS; i++) {
        row_clear(&r);
        for (uint32_t c = 0; g_pane[i][c] && c < SCR_COLS; c++) r.t[c] = g_pane[i][c];
        if (pane.cur_len && pane.cur_row == i) { r.inv_col = (uint8_t)pane.cur_col; r.inv_len = (uint8_t)pane.cur_len; }
        put_row(RV_PANE + (int)i, &r);
    }
    for (; RV_PANE + (int)i < RV_MESSAGE; i++) put_text_row(RV_PANE + (int)i, "");   /* WP's blank line 23 */
    put_text_row(RV_MESSAGE, IW_RV_MESSAGE);
}

/* ---- the Format screens (Shift-F8; slice 3) ------------------------------------ */
typedef struct fmt_row { uint8_t row, col; const char *text; } fmt_row_t;
static const fmt_row_t FMT_MAIN[] = IW_FMT_INIT;
_Static_assert(sizeof FMT_MAIN / sizeof FMT_MAIN[0] == IW_FMT_ROWS, "IW_FMT_ROWS == the rows of IW_FMT_INIT");
static const fmt_row_t FMT_LINE[IW_FMT_LINE_ROWS] = IW_FMT_LINE_INIT;

static void put_at(row_t *r, uint32_t col, const char *t)
{
    for (uint32_t i = 0; t[i] && col + i < SCR_COLS; i++) r->t[col + i] = t[i];
}

/* A margin as the Format: Line screen shows it ("1"", "1.3"") */
static void put_inches(row_t *r, uint32_t col, uint32_t wpu)
{
    char num[12];
    (void)iw_fmt_inches(wpu / 12u, num);
    put_at(r, col, num);
}

static void paint_format(const iw_where_t *w)
{
    static row_t rows[SCR_LINES];
    static const char *const JN[4] = IW_FMT_JUST_NAMES;
    uint32_t i, n = g_mode == M_FORMAT ? IW_FMT_ROWS : IW_FMT_LINE_ROWS;
    const fmt_row_t *t = g_mode == M_FORMAT ? FMT_MAIN : FMT_LINE;
    char sel[16];
    for (i = 0; i < SCR_LINES; i++) row_clear(&rows[i]);
    for (i = 0; i < n; i++) {
        /* WP's 25 rows in 24: the Format screen drops its blank row 1 */
        uint32_t r = t[i].row;
        if (!t[i].text || !t[i].text[0]) continue;
        if (g_mode == M_FORMAT && r > 0u) r--;
        put_at(&rows[r], t[i].col, t[i].text);
    }
    if (g_mode != M_FORMAT) {
        put_at(&rows[2], IW_FMT_VALUE_COL, IW_FMT_V_HYPH);
        put_at(&rows[4], IW_FMT_VALUE_COL, IW_FMT_V_ZONE_L);
        put_at(&rows[5], IW_FMT_VALUE_COL, IW_FMT_V_ZONE_R);
        put_at(&rows[7], IW_FMT_VALUE_COL, JN[w->just < 4u ? w->just : 1u]);
        put_at(&rows[9], IW_FMT_VALUE_COL, IW_FMT_V_HEIGHT);
        put_at(&rows[11], IW_FMT_VALUE_COL, IW_FMT_V_NUMBER);
        {
            /* the spacing at the cursor (slice 3; MINT25 02_spacing_set "2",
             * MINT30 02_spacing_225 "2.25"): hundredths, trailing zeros
             * dropped (IW_FMT_V_SPACING "1" at rest) */
            char num[12];
            uint32_t k = iw_fmt_inches((w->sp * 100u + IW_WP5_SPACING_ONE / 2u) / IW_WP5_SPACING_ONE, num);
            num[k - 1u] = 0;
            put_at(&rows[13], IW_FMT_VALUE_COL, num);
        }
        put_inches(&rows[15], IW_FMT_VALUE_COL, w->lm);
        put_inches(&rows[16], IW_FMT_VALUE_COL, IW_WP5_PAPER_WPU - w->rm);
        put_at(&rows[18], IW_FMT_VALUE_COL, IW_FMT_V_TABS);
        put_at(&rows[20], IW_FMT_VALUE_COL, IW_FMT_V_WIDOW);
        if (g_mode == M_FMTMARL || g_mode == M_FMTMARR || g_mode == M_FMTSP) {
            /* the field being typed: the value until a key replaces it, then
             * the text typed and the cursor after it (04_left_typed "1.3_";
             * the spacing field likewise, MINT25 01_spacing_field) */
            row_t *fr = &rows[g_mode == M_FMTMARL ? 15 : g_mode == M_FMTMARR ? 16 : 13];
            if (g_mode == M_FMTMARR) put_inches(&rows[15], IW_FMT_VALUE_COL, g_fleft);
            if (g_ftyped) {
                for (i = IW_FMT_VALUE_COL; i < SCR_COLS; i++) fr->t[i] = ' ';
                for (i = 0; i < g_flen; i++) fr->t[IW_FMT_VALUE_COL + i] = g_field[i];
                fr->inv_col = (uint8_t)(IW_FMT_VALUE_COL + g_flen);
            } else {
                fr->inv_col = (uint8_t)IW_FMT_VALUE_COL;
            }
            fr->inv_len = 1;
        }
    }
    if (g_mode == M_FMTJUST) {
        put_at(&rows[SCR_LINES - 1], 0, IW_FMT_JUST_PROMPT);
    } else {
        scopy(sel, IW_FMT_SELECTION);
        i = slen(sel);
        sel[i] = (g_mode == M_FMTMARL || g_mode == M_FMTMARR) ? '7' : g_mode == M_FMTSP ? '6' : '0';
        sel[i + 1] = 0;
        put_at(&rows[SCR_LINES - 1], 0, sel);
        if (g_mode == M_FORMAT || g_mode == M_FMTLINE) {
            rows[SCR_LINES - 1].inv_col = (uint8_t)i;
            rows[SCR_LINES - 1].inv_len = 1;
        }
    }
    for (i = 0; i < SCR_LINES; i++) put_row((int)i, &rows[i]);
}

static int format_mode(void)
{
    return g_mode == M_FORMAT || g_mode == M_FMTLINE || g_mode == M_FMTJUST || g_mode == M_FMTMARL || g_mode == M_FMTMARR ||
           g_mode == M_FMTSP;
}

/* Minimal Setup, Reference pp. 574-575; Environment 3 / Backup 1 (p. 42),
 * Units 8 and its two independent selections (pp. 698-700). Unavailable
 * entries retain their printed names in gray. Layout fits the native face. */
static int setup_mode(void) { return g_mode >= M_SETUP && g_mode <= M_BACKUPVALUE; }
static const char *unit_name(uint8_t u)
{
    switch (u) {
    case 'i': return "Inches"; case 'c': return "Centimeters"; case 'p': return "Points";
    case 'w': return "1200ths of an inch"; case 'u': return "WordPerfect 4.2 units";
    default: return "?";
    }
}
static void setup_row(int line, const char *text, int gray)
{
    row_t r; row_clear(&r); put_at(&r, 0, text);
    if (gray) r.fg = TBX_COLOR_GRAY;
    put_row(line, &r);
}
static void setup_value(int line, const char *text, const char *value)
{
    row_t r; row_clear(&r); put_at(&r, 0, text); put_at(&r, slen(text)+2, value); put_row(line, &r);
}
static void paint_setup(void)
{
    for (int i = 0; i < SCR_LINES; i++) put_text_row(i, "");
    if (g_mode == M_SETUP) {
        setup_row(0, "Setup", 0);
        setup_row(4, "2 Display", 1); setup_row(6, "3 Environment", 0);
        setup_row(8, "4 Initial Settings", 1); setup_row(10, "5 Keyboard Layout", 1);
        setup_row(12, "6 Location of Files", 1);
    } else if (g_mode == M_ENV) {
        setup_row(0, "Setup: Environment", 0);
        setup_row(4, "1 Backup Options", 0); setup_row(6, "8 Units of Measure", 0);
    } else if (g_mode == M_UNITS || g_mode == M_UNITVALUE) {
        setup_row(0, "Setup: Units of Measure", 0);
        setup_value(4, "1 Display and Entry of Numbers for Margins, Tabs, etc.", unit_name(g_settings[4]));
        setup_value(6, "2 Status Line Display", unit_name(g_settings[5]));
        setup_row(10, "i Inches; c Centimeters; p Points", 0);
        setup_row(12, "w 1200ths of an inch; u WordPerfect 4.2 units", 0);
    } else {
        setup_row(0, "Setup: Backup", 0);
        setup_row(4, "1 Timed Document Backup", 1);
        setup_value(6, "2 Original Document Backup", g_settings[6] ? "Yes" : "No");
    }
    if (g_mode == M_UNITVALUE) setup_row(23, "Units of Measure:", 0);
    else if (g_mode == M_BACKUPVALUE) setup_row(23, "Original Document Backup? No (Yes)", 0);
    else setup_row(23, "Selection: 0", 0);
}

static void paint(void)
{
    iw_where_t w;
    if (setup_mode()) { paint_setup(); return; }
    if (g_mode == M_UNDO) {
        /* Preview is a borrowed temporary document. The live piece table,
         * add arena, original bytes, cursor and dirty flag never change. */
        iw_ed_t saved = g_ed;
        iw_piece_t pc;
        uint32_t n = g_undo_count ? g_undo_n[g_undo_sel] : 0, at = g_ed.cur;
        if (g_ed.doc.len+n <= BANK) {
            (void)iw_doc_read(&g_ed.doc, 0, g_preview, at);
            for (uint32_t i = 0; i < n; i++) g_preview[at+i] = g_undo[g_undo_sel][i];
            (void)iw_doc_read(&g_ed.doc, at, g_preview+at+n, g_ed.doc.len-at);
            iw_doc_init(&g_ed.doc, 0, 0, &pc, 1, BANK);
            iw_doc_load(&g_ed.doc, g_preview, saved.doc.len+n);
            g_ed.blk = at; g_ed.cur = at+n;
#ifdef IWORD_MUT_UNDO_PREVIEW
            g_ed.blk = IW_NO_BLOCK;
#endif
        }
        iw_ed_where(&g_ed, &w); paint_doc(&w, EDIT_ROWS); paint_status(&w, LINE_STATUS);
        g_ed = saved;
        return;
    }
    iw_ed_where(&g_ed, &w);
    if (format_mode()) {
        paint_format(&w);
        return;
    }
    if (!g_reveal) {
        paint_doc(&w, EDIT_ROWS);
        paint_status(&w, LINE_STATUS);
        return;
    }
    paint_doc(&w, RV_TOP_ROWS);
    paint_status(&w, RV_STATUS);
    paint_reveal(&w);
}

static void paint_all(void)
{
    for (int i = 0; i < SCR_LINES; i++) g_shadow_ok[i] = 0;
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

static void settings_defaults(void)
{
    for (uint32_t i = 0; i < 4; i++) g_settings[i] = IW_SETTINGS_MAGIC[i];
    g_settings[4] = g_settings[5] = IW_SETTINGS_DEFAULT_UNIT;
    g_settings[6] = g_settings[7] = 0;
}
static void settings_load(void)
{
    int32_t h, n, cl;
    uint32_t pos = 0;
    settings_defaults();
    h = tbx_file_open(IW_SETTINGS_PATH, TBX_FILE_READ_ONLY);
    if (h < 0) return;
    do {
        n = tbx_file_read(h, g_settings_io+pos, sizeof g_settings_io-pos);
        if (n <= 0 || (uint32_t)n > sizeof g_settings_io-pos) break;
        pos += (uint32_t)n;
    } while (pos < sizeof g_settings_io);
    cl = tbx_file_close(h);
    if (n < 0 || pos != IW_SETTINGS_SIZE || cl < 0) return;
    for (uint32_t i = 0; i < 4; i++) if (g_settings_io[i] != IW_SETTINGS_MAGIC[i]) return;
#ifndef IWORD_MUT_SETUP_VALIDATE
    if (g_settings_io[6] > 1 || g_settings_io[7] || unit_name(g_settings_io[4])[0] == '?' ||
        unit_name(g_settings_io[5])[0] == '?') return;
#endif
#ifndef IWORD_MUT_SETUP_LOAD
    for (uint32_t i = 0; i < IW_SETTINGS_SIZE; i++) g_settings[i] = g_settings_io[i];
#endif
}
static int settings_save(void)
{
    int32_t h, n, cl;
    uint32_t pos = 0;
    for (uint32_t i = 0; i < IW_SETTINGS_SIZE; i++) g_settings_io[i] = g_settings[i];
#ifdef IWORD_MUT_SETUP_SAVE
    g_settings_io[4] = 'i';
#endif
    h = tbx_file_create(IW_SETTINGS_PATH);
    if (h < 0) return IW_E_FULL;
    while (pos < IW_SETTINGS_SIZE) {
        n = tbx_file_write(h, g_settings_io+pos, IW_SETTINGS_SIZE-pos);
        if (n <= 0 || (uint32_t)n > IW_SETTINGS_SIZE-pos) break;
        pos += (uint32_t)n;
#ifdef IWORD_MUT_SETUP_SHORT
        break;
#endif
    }
    cl = tbx_file_close(h);
    return pos == IW_SETTINGS_SIZE && cl >= 0 ? IW_OK : IW_E_FULL;
}
static void setup_key(char ch, uint8_t vk)
{
    if (vk == VK_F1 || vk == VK_ESC) { g_mode = M_EDIT; paint_all(); return; }
    if (vk == VK_F7) {
        g_mode = g_mode == M_SETUP ? M_EDIT : g_mode == M_ENV ? M_SETUP : M_ENV;
        paint_all(); return;
    }
    if (g_mode == M_SETUP && ch == '3') g_mode = M_ENV;
    else if (g_mode == M_ENV && ch == '8') g_mode = M_UNITS;
    else if (g_mode == M_ENV && ch == '1') g_mode = M_BACKUP;
    else if (g_mode == M_UNITS && (ch == '1' || ch == '2')) { g_unit_target = ch == '1' ? 4 : 5; g_mode = M_UNITVALUE; }
    else if (g_mode == M_BACKUP && ch == '2') g_mode = M_BACKUPVALUE;
    else if (g_mode == M_UNITVALUE || g_mode == M_BACKUPVALUE) {
        uint8_t at = g_mode == M_UNITVALUE ? g_unit_target : 6, old = g_settings[at], value;
        if (g_mode == M_UNITVALUE) {
            value = (uint8_t)ch;
            if (unit_name(value)[0] == '?') return;
        } else {
            if (ch != 'y' && ch != 'Y' && ch != 'n' && ch != 'N') return;
            value = ch == 'y' || ch == 'Y';
        }
        g_settings[at] = value;
        if (settings_save() != IW_OK) {
            g_settings[at] = old; error("ERROR: File I/O error", M_EDIT); return;
        }
        g_mode = at == 6 ? M_BACKUP : M_UNITS;
    }
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
    g_skip = 0;
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
    int font_rc = IW_OK;
    if (g_mode == M_ERROR) { g_mode = g_err_return; return; }
    if (vk == VK_F1 || vk == VK_ESC || ch == 0x1B) {   /* Cancel */
        g_exiting = 0;
        g_ed.ask = 0xFFFFFFFFu;                         /* a question is dropped */
        /* Ctrl-F4 1/2/3's highlight goes, the cursor stays at its end
         * (MINT31 13_page_cancel) */
        if (g_sel) { g_ed.blk = IW_NO_BLOCK; g_sel = 0; }
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
    case M_FONT:
        /* IW_FONT_MENU: 1 Size, 2 Appearance, 3 Normal. Base Font and Print
         * Color (MINT28 25 / 27: their own screens) are a later slice: the
         * key returns to the document, as Cancel does. */
        g_mode = M_EDIT;
        if (ch == '1') g_mode = M_SIZE;
        else if (ch == '2') g_mode = M_APPEAR;
        else if (ch == '3') font_rc = edit(ED_NORMAL, 0);
        break;
    case M_FONTBLK:
        /* Block on: "Attribute: 1 Size; 2 Appearance: 0" (MINT28 07) */
        g_mode = M_EDIT;
        if (ch == '1') g_mode = M_SIZE;
        else if (ch == '2') g_mode = M_APPEAR;
        break;
    case M_SIZE:
    case M_APPEAR: {
        /* every Size / Appearance attribute, its number from the real WP's
         * bytes (MINT28; spec/iword/reveal_codes.h IW_SIZE_ATTRS /
         * IW_APPEAR_ATTRS); with Block on it is given to the block (p. 37) */
        static const uint8_t SZ[7] = IW_SIZE_ATTRS;
        static const uint8_t AP[9] = IW_APPEAR_ATTRS;
        int last = g_mode == M_SIZE ? 7 : 9;
        g_mode = M_EDIT;
        if (ch >= '1' && ch <= (char)('0' + last))
            font_rc = edit(ED_ATTR, last == 7 ? SZ[ch - '1'] : AP[ch - '1']);
        break;
    }
    case M_DELCODE:
        /* "Delete [code]? No (Yes)": y deletes the pair; any other key is No
         * and steps over the code (MINT11; the option not in parentheses is
         * the default, A Brief Lesson p. 21) */
        g_mode = M_EDIT;
        font_rc = edit(ED_ANSWER, (uint8_t)(ch == 'y' || ch == 'Y'));
        break;
    case M_DELBLOCK:
        /* "Delete Block? No (Yes)": y deletes it (MINT12 09/10); No keeps
         * Block on [authored: the shot shows y only] */
        g_mode = M_EDIT;
        if (ch == 'y' || ch == 'Y') font_rc = edit(ED_CUT, 0);
        break;
    case M_MOVE:
        /* 1 Block; Tabular Column and Rectangle are not in this slice */
        g_mode = ch == '1' ? M_MOVEBLK : M_EDIT;
        break;
    case M_MOVENB:
        /* Block off: "Move: 1 Sentence; 2 Paragraph; 3 Page; 4 Retrieve: 0"
         * (MINT31 02): 1..3 highlight the text (iw_ed_select) and offer the
         * block's menu; 4 the Retrieve menu */
        g_mode = M_EDIT;
        if (ch >= '1' && ch <= '3') {
            if (iw_ed_select(&g_ed, ch - '0') == IW_OK) { g_sel = 1; g_mode = M_MOVEBLK; }
        } else if (ch == '4') {
            g_mode = M_RETMENU;
        }
        break;
    case M_RETMENU:
        /* "Retrieve: 1 Block; ..." (MINT29 14): 1 puts the last moved /
         * copied text at the cursor again, the cursor in front (MINT31 15);
         * Tabular Column and Rectangle are not in this slice */
        g_mode = M_EDIT;
        if (ch == '1' && g_clip_n) font_rc = edit(ED_RETRIEVE, 0);
        break;
    case M_MOVEBLK:
        /* 1 Move, 2 Copy: the block goes to the buffer, then the Move prompt;
         * 3 Delete; 4 Append (to a file) is not in this slice */
        g_mode = M_EDIT;
        if (ch == '1') { font_rc = edit(ED_CUT, 1); if (font_rc == IW_OK) g_mode = M_RETRIEVE; }
        else if (ch == '2') { font_rc = edit(ED_COPY, 0); if (font_rc == IW_OK) g_mode = M_RETRIEVE; }
        else if (ch == '3') font_rc = edit(ED_CUT, 0);
        else if (g_sel) g_ed.blk = IW_NO_BLOCK;   /* a selection does not stay */
        g_sel = 0;
        break;
    default:
        g_mode = M_EDIT;
        break;
    }
    if (font_rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
}

/* A measurement typed in inches ("2", "1.3", "1.25", an optional '"') ->
 * WPU; 0 when it is not one [authored: fractions like "1 1/2" (Reference
 * Manual p. 394) are not taken]. Hundredths of an inch, 12 WPU each. */
static uint32_t parse_inches(void)
{
    uint32_t i = 0, ip = 0, fr = 0, nf = 0, digits = 0;
    while (i < g_flen && g_field[i] >= '0' && g_field[i] <= '9') { ip = ip * 10u + (uint32_t)(g_field[i] - '0'); i++; digits++; }
    if (i < g_flen && g_field[i] == '.') {
        i++;
        while (i < g_flen && g_field[i] >= '0' && g_field[i] <= '9') {
            if (nf < 2u) { fr = fr * 10u + (uint32_t)(g_field[i] - '0'); nf++; }
            i++; digits++;
        }
    }
    if (i < g_flen && g_field[i] == '"') i++;
    if (i != g_flen || digits == 0u || ip > 8u) return 0;
    if (nf == 1u) fr *= 10u;
    return (ip * 100u + fr) * 12u;
}

static void field_start(void)
{
    g_flen = 0;
    g_field[0] = 0;
    g_ftyped = 0;
}

/* Shift-F8: the Format screen, Format: Line, its prompt and fields. */
static void format_key(char ch, uint8_t vk)
{
    int rc = IW_OK;
    int cancel = vk == VK_F1 || vk == VK_ESC || ch == 0x1B;
    int enter = vk == VK_ENTER || ch == '\r';
    iw_where_t w;
    switch (g_mode) {
    case M_FORMAT:
        if (ch == '1') g_mode = M_FMTLINE;
        else if (cancel || enter || ch == '0' || vk == VK_F7) g_mode = M_EDIT;
        break;
    case M_FMTLINE:
        if (vk == VK_F7) g_mode = M_EDIT;                    /* Exit (F7) */
        else if (cancel || enter || ch == '0') g_mode = M_FORMAT;
        else if (ch == '3') g_mode = M_FMTJUST;
        else if (ch == '7') { iw_ed_where(&g_ed, &w); g_fleft = w.lm; g_fright = IW_WP5_PAPER_WPU - w.rm; g_mode = M_FMTMARL; field_start(); }
        else if (ch == '6') { g_mode = M_FMTSP; field_start(); }     /* MINT25 01_spacing_field */
        break;
    case M_FMTJUST:
        /* "1 Left; 2 Center; 3 Right; 4 Full" (MINT22 02_just_menu) */
        g_mode = M_FMTLINE;
        if (ch == '1') rc = edit(ED_JUST, IW_WP5_JUST_LEFT);
        else if (ch == '2') rc = edit(ED_JUST, IW_WP5_JUST_CENTER);
        else if (ch == '3') rc = edit(ED_JUST, IW_WP5_JUST_RIGHT);
        else if (ch == '4') rc = edit(ED_JUST, IW_WP5_JUST_FULL);
        break;
    case M_FMTSP:
        /* Line Spacing: a number, Enter (MINT25: "2", "1.5", "1"; MINT30
         * "1.33", "2.25"); the code goes in when one was typed */
        if (cancel) { g_mode = M_FMTLINE; break; }
        if (enter) {
            uint32_t h = g_ftyped ? parse_inches() / 12u : 0u;    /* hundredths */
            g_mode = M_FMTLINE;
            if (h) { g_fsp = iw_spacing_from_hundredths(h); rc = edit(ED_SPACING, 0); }
            break;
        }
        if (vk == VK_BSPC || ch == 8) { if (g_flen) g_field[--g_flen] = 0; break; }
        if (((ch >= '0' && ch <= '9') || ch == '.') && g_flen < 10u) {
            g_field[g_flen++] = ch;
            g_field[g_flen] = 0;
            g_ftyped = 1;
        }
        break;
    case M_FMTMARL:
    case M_FMTMARR:
        if (cancel) { g_mode = M_FMTLINE; break; }
        if (enter) {
            uint32_t v = g_ftyped ? parse_inches() : 0u;
            static uint8_t any;
            if (g_mode == M_FMTMARL) any = 0;
            if (v) { if (g_mode == M_FMTMARL) g_fleft = v; else g_fright = v; }
            if (g_ftyped) any = 1;
            if (g_mode == M_FMTMARL) { g_mode = M_FMTMARR; field_start(); break; }
            /* both entered: the code goes in (when a value was typed) */
            g_mode = M_FMTLINE;
            if (any) rc = edit(ED_MARGINS, 0);
            break;
        }
        if (vk == VK_BSPC || ch == 8) { if (g_flen) g_field[--g_flen] = 0; break; }
        /* "WordPerfect allows ten input characters for margins" (p. 394) */
        if (((ch >= '0' && ch <= '9') || ch == '.' || ch == '"') && g_flen < 10u) {
            g_field[g_flen++] = ch;
            g_field[g_flen] = 0;
            g_ftyped = 1;
        }
        break;
    default:
        g_mode = M_EDIT;
        break;
    }
    if (rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
    if (g_mode == M_EDIT) paint_all();
}

/* The cursor keys (the editing screen, Block, the Move prompt). HOME IS A
 * PREFIX KEY (slice 3; Reference Manual, Cursor Movement p. 103-104; the
 * real WP.EXE in mint/wpMINT27.steps): the arrow after one, two or three
 * Homes says where to go. `homes` = the Homes pressed just before. */
static int cursor_key(uint8_t vk, uint32_t homes)
{
    uint32_t rows = g_reveal ? RV_TOP_ROWS : EDIT_ROWS, first, last;
    iw_where_t w;
    if (vk == VK_HOME) { g_home = (uint8_t)(homes < 3u ? homes + 1u : 3u); return 1; }
    if (homes && (vk == VK_UP || vk == VK_DOWN)) {
        if (homes >= 2u) {
            /* Home, Home, Up: the top after the formatting codes; Home x3,
             * Up before them (MINT27 10 / 12); Home, Home, Down: the end */
            if (vk == VK_UP) iw_ed_doc_top(&g_ed, homes >= 3u);
            else iw_ed_doc_end(&g_ed);
            return 1;
        }
        /* Home, Up / Home, Down: the top / bottom line on the screen, the
         * column kept (MINT27 15 / 16); already there, one screen further
         * [inferred, W-18] */
        iw_ed_where(&g_ed, &w);
        screen_lines(rows, &first, &last);
        if (vk == VK_UP) iw_ed_to_line(&g_ed, w.line != first ? first : (first > rows - 1u ? first - (rows - 1u) : 0u));
        else iw_ed_to_line(&g_ed, w.line != last ? last : last + (rows - 1u));
        return 1;
    }
    if (homes && vk == VK_LEFT) {
        /* Home, Left and Home, Home, Left: past the codes in front of the
         * line's first character; Home x3, Left before them (MINT27 03 / 06
         * / 08) */
        if (homes >= 3u) iw_ed_line_start(&g_ed); else iw_ed_line_home(&g_ed);
        return 1;
    }
    switch (vk) {
    case VK_UP:    iw_ed_up(&g_ed); return 1;
    case VK_DOWN:  iw_ed_down(&g_ed); return 1;
    case VK_LEFT:  iw_ed_left(&g_ed); return 1;
    case VK_RIGHT: if (homes) iw_ed_line_end(&g_ed); else iw_ed_right(&g_ed); return 1;   /* Home, Right: MINT27 04 */
    case VK_END:   iw_ed_line_end(&g_ed); return 1;     /* End = Home, Home, Right */
    default:       return 0;
    }
}

static void toggle_reveal(void)
{
    g_reveal = (uint8_t)!g_reveal;
    g_ed.reveal = g_reveal;
    paint_all();
}

/* Block on: the cursor keys stretch it; Alt-F4 / F1 / Esc turn it off;
 * Ctrl-F4 the Move menu; Delete / Backspace "Delete Block?"; Alt-F3. */
static void block_key(char ch, uint8_t vk, int32_t mods)
{
    int ctrl = (mods & (int32_t)FLAIR_EVT_MOD_CONTROL_KEY) != 0;
    int alt  = (mods & (int32_t)FLAIR_EVT_MOD_OPTION_KEY) != 0;
    int shift = (mods & (int32_t)FLAIR_EVT_MOD_SHIFT_KEY) != 0;
    int rc = IW_OK;
    if (ch == 0 && cursor_key(vk, g_hin)) return;
    if (vk == VK_F4 && alt) { iw_ed_block(&g_ed); return; }
    if (vk == VK_F4 && ctrl) { g_mode = M_MOVE; return; }
    /* an attribute given to the block (slice 3; p. 37; MINT28): F6, F8,
     * Ctrl-F8 "Attribute: 1 Size; 2 Appearance: 0" */
    if (vk == VK_F6 && !ctrl && !alt && !shift) rc = edit(ED_ATTR, IW_WP5_ATTR_BOLD);
    else if (vk == VK_F8 && !ctrl && !alt && !shift) rc = edit(ED_ATTR, IW_WP5_ATTR_UNDERLINE);
    else if (vk == VK_F8 && ctrl && !alt && !shift) { g_mode = M_FONTBLK; return; }
    if (rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
    if (vk == VK_F6 || vk == VK_F8) return;
    if (vk == VK_F3 && alt) { toggle_reveal(); return; }
    if (vk == VK_DEL || vk == VK_BSPC || ch == 8) { g_mode = M_DELBLOCK; return; }
    if (vk == VK_F1 || vk == VK_ESC || ch == 0x1B) { iw_ed_block(&g_ed); return; }
}

/* "Move cursor; press Enter to retrieve.": the cursor keys, Enter; F1 /
 * Esc leave the text unretrieved [authored]. */
static void retrieve_key(char ch, uint8_t vk, int32_t mods)
{
    int alt = (mods & (int32_t)FLAIR_EVT_MOD_OPTION_KEY) != 0;
    int rc;
    if (ch == 0 && cursor_key(vk, g_hin)) return;
    if (vk == VK_F3 && alt) { toggle_reveal(); return; }
    if (vk == VK_ENTER || ch == '\r') {
        g_mode = M_EDIT;
        rc = edit(ED_RETRIEVE, 0);
        if (rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
        return;
    }
    if (vk == VK_F1 || vk == VK_ESC || ch == 0x1B) g_mode = M_EDIT;
}

/* All 48 slots enter here; the table controls dispatch, including aliases.
 * Ref: spec/iword/fkeys.json's local Reference citations. No modified F7
 * accidentally falls through to Exit, nor Ctrl/Alt-F10 to Save. */
static int function_key(uint8_t vk, int32_t mods)
{
    uint32_t k, m = 0, count = 0;
    int rc = IW_OK;
    const iw_fkey_t *f;
    if (vk >= VK_F1 && vk <= VK_F10) k = vk - VK_F1;
    else if (vk == 0x57 || vk == 0x58) k = vk - 0x57u + 10u;
    else return 0;
    if (mods & FLAIR_EVT_MOD_SHIFT_KEY) { m = 1; count++; }
    if (mods & FLAIR_EVT_MOD_OPTION_KEY) { m = 2; count++; }
    if (mods & FLAIR_EVT_MOD_CONTROL_KEY) { m = 3; count++; }
    if (count > 1) return 1;   /* outside the four rows of the WP template */
#ifdef IWORD_MUT_FKEY_DROP
    if (m == 0 && k == 8) return 1; /* named mutant: drop plain F9 */
#endif
    f = &IW_FKEYS[m][k];
    switch (f->action) {
    case IW_FK_setup: g_mode = M_SETUP; break;
    case IW_FK_undelete:
        g_undo_sel = 0; g_undo_join = 0; g_mode = M_UNDO; break;
    case IW_FK_replace:
        g_search_back = 0; g_confirm = 0; g_mode = M_RCONF; break;
    case IW_FK_search_forward: case IW_FK_search_backward:
        g_search_back = f->action == IW_FK_search_backward;
        g_query_cur = g_query_n; g_query_fresh = 1; g_mode = M_SEARCH; break;
    case IW_FK_indent:    rc = edit(ED_INDENT, 0); break;
    case IW_FK_bold:      rc = edit(ED_ATTR, IW_WP5_ATTR_BOLD); break;
    case IW_FK_underline: rc = edit(ED_ATTR, IW_WP5_ATTR_UNDERLINE); break;
    case IW_FK_exit:      request_exit(FROM_F7); break;
    case IW_FK_save:      g_exiting = 0; offer_name(); g_mode = M_SAVE_NAME; break;
    case IW_FK_retrieve:  g_exiting = 0; g_nlen = 0; g_name[0] = 0; g_fresh = 0; g_mode = M_GET_NAME; break;
    case IW_FK_reveal:    toggle_reveal(); break;
    case IW_FK_block:     iw_ed_block(&g_ed); break;
    case IW_FK_format:    g_mode = M_FORMAT; break;
    case IW_FK_move:      g_mode = M_MOVENB; break;
    case IW_FK_font:      g_mode = M_FONT; break;
    case IW_FK_unmapped:
        /* Reference p. 966 note 4: no command is mapped to these keys.
         * Keep the editing screen/status, rather than inventing a blank
         * disabled command. */
#ifdef IWORD_MUT_FKEY_UNMAPPED
        g_gray = f->name; g_mode = M_GRAY;
#endif
        break;
    default:             g_gray = f->name; g_mode = M_GRAY; break;
    }
    if (rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
    return 1;
}

static void edit_key(char ch, uint8_t vk, int32_t mods)
{
    int ctrl  = (mods & (int32_t)FLAIR_EVT_MOD_CONTROL_KEY) != 0;
    int rc = IW_OK;
    if (g_ed.blk != IW_NO_BLOCK) {
        /* p. 966 note 3: the aliases work in Block as well. */
        int chord = mods & (FLAIR_EVT_MOD_SHIFT_KEY | FLAIR_EVT_MOD_CONTROL_KEY | FLAIR_EVT_MOD_OPTION_KEY);
        if (vk == 0x57 && !chord) { vk = VK_F3; mods |= FLAIR_EVT_MOD_OPTION_KEY; }
        if (vk == 0x58 && !chord) { vk = VK_F4; mods |= FLAIR_EVT_MOD_OPTION_KEY; }
        block_key(ch, vk, mods); return;
    }
    if (!ch && function_key(vk, mods)) return;
    if (ch == '\t' || vk == VK_TAB) {   /* Tab: [Tab] (MINT09) */
        rc = edit(ED_TAB, 0);
    } else if (ch == 0) {                /* function and cursor keys */
        switch (vk) {
        case VK_DEL:   rc = edit(ED_DELETE, 0); break;
        default:       (void)cursor_key(vk, g_hin); break;
        }
    } else if (ch == '\r' || ch == '\n' || vk == VK_ENTER) {
        rc = edit(ctrl ? ED_HPAGE : ED_ENTER, 0);      /* Ctrl-Enter: hard page */
    } else if (ch == 8 || vk == VK_BSPC) {
        rc = edit(ED_BACKSPACE, 0);
    } else if (ch >= ' ' && ch <= '~') {
        rc = edit(ED_TYPE, (uint8_t)ch);
    }
    if (rc == IW_E_ASK) g_mode = M_DELCODE;             /* "Delete [code]?" */
    if (rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
}

/* A borrowed read-only view over the query. Never uses the document's add
 * buffer, pairing, fixup, dirty flag or reflow. Reference pp. 572-573. */
static void query_view(iw_doc_t *q, iw_piece_t *pc)
{
    iw_doc_init(q, 0, 0, pc, 1, QUERY_CAP);
    iw_doc_load(q, g_query, g_query_n);
}

static uint32_t query_label(char *out, uint32_t cap)
{
    iw_doc_t q; iw_piece_t pc;
    /* Native 76-column adaptation of the Reference's editable Search field
     * (p. 572): scroll the query to keep the insertion cell in view. Labels
     * and cursor use complete units; no code's bytes become text. */
    static char text[QUERY_CAP * 16];
    uint32_t p = 0, n = 0, cur = 0, left;
    query_view(&q, &pc);
    while (p < q.len && n + 48 < sizeof text) {
        if (p == g_query_cur) cur = n;
        if (iw_unit_kind(&q, p) == IW_U_TEXT) text[n++] = (char)iw_doc_byte(&q, p);
        else {
            char lab[48];
            (void)iw_code_label(&q, p, lab, sizeof lab);
            for (uint32_t i = 0; lab[i]; i++) text[n++] = lab[i];
        }
        p = iw_unit_next(&q, p);
    }
    if (g_query_cur == q.len) cur = n;
    left = cur >= cap - 1 ? cur - (cap - 2) : 0;
    for (uint32_t i = left; i < n && i - left + 1 < cap; i++) out[i-left] = text[i];
    return cur - left;
}

static void query_insert(const uint8_t *s, uint32_t n)
{
    if (g_query_fresh) { g_query_n = g_query_cur = 0; g_query_fresh = 0; }
    if (g_query_n + n > QUERY_CAP) return;
    for (uint32_t i = g_query_n; i > g_query_cur; i--) g_query[i + n - 1] = g_query[i - 1];
    for (uint32_t i = 0; i < n; i++) g_query[g_query_cur + i] = s[i];
    g_query_cur += n; g_query_n += n;
}

/* One complete span. The old P2 model remains the sole byte-format writer:
 * reflow and fixup recompute position-bearing codes (MINT09..32). Lowercase
 * replacement preserves the found word's case (Reference p. 552). */
static int replace_match(void)
{
    uint8_t r[QUERY_CAP];
    uint32_t letters = 0, capitals = 0, first = 0, lower = 1;
    for (uint32_t i = 0; i < g_find_n; i++)
        if (g_find[i] >= 'A' && g_find[i] <= 'Z') lower = 0;
    for (uint32_t i = 0; i < g_replacement_n; i++) {
        r[i] = g_replacement[i];
        if (r[i] >= 'A' && r[i] <= 'Z') lower = 0;
    }
    for (uint32_t p = g_rs; p < g_re; p = iw_unit_next(&g_ed.doc, p)) {
        int c = iw_doc_byte(&g_ed.doc, p);
        if (iw_unit_kind(&g_ed.doc, p) == IW_U_TEXT &&
            ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
            if (!letters) first = c >= 'A' && c <= 'Z';
            letters++; capitals += c >= 'A' && c <= 'Z';
        }
    }
#ifndef IWORD_MUT_REPLACE_CASE
    if (lower && letters) {
        int initial = 1;
        for (uint32_t p = 0; p < g_replacement_n; ) {
            uint32_t n = iw_unit_len(r, g_replacement_n, p);
            if (n == 1 && r[p] >= 'a' && r[p] <= 'z') {
                if (capitals == letters || (initial && first)) r[p] -= 32;
                initial = 0;
            }
            p += n;
        }
    }
#else
    (void)lower; (void)letters; (void)capitals; (void)first;
#endif
    ensure_room();
    /* Refuse BEFORE the delete if an insert cannot fit. The small fixed
     * query cap and P2's room reserve also leave room for code fixup. */
    if (g_ed.doc.len - (g_re-g_rs) + g_replacement_n > g_ed.doc.max_len ||
        g_ed.doc.add_cap - g_ed.doc.add_len < g_replacement_n ||
        g_ed.doc.pc_cap - g_ed.doc.npc < 4u) return IW_E_FULL;
#ifndef IWORD_MUT_REPLACE_CODE
    (void)iw_doc_delete(&g_ed.doc, g_rs, g_re-g_rs);
#else
    if (g_find[0] >= 0xC0) { g_ed.cur = g_re; return IW_OK; }
    (void)iw_doc_delete(&g_ed.doc, g_rs, g_re-g_rs);
#endif
    if (g_replacement_n && iw_doc_insert(&g_ed.doc, g_rs, r, g_replacement_n) != IW_OK) return IW_E_FULL;
    g_ed.cur = g_search_back ? g_rs : g_rs + g_replacement_n;
#ifdef IWORD_MUT_REPLACE_SKIP
    if (!g_search_back && g_ed.cur < g_ed.doc.len) g_ed.cur = iw_unit_next(&g_ed.doc, g_ed.cur);
#endif
    g_ed.blk = IW_NO_BLOCK; g_ed.dirty = 1; g_ed.goal = 0xFFFFFFFFu;
    (void)iw_reflow_all(&g_ed.doc, g_ed.width);
    return iw_ed_fixup(&g_ed);
}

static void replace_next(void)
{
    iw_doc_t q; iw_piece_t pc;
    iw_doc_init(&q, 0, 0, &pc, 1, QUERY_CAP); iw_doc_load(&q, g_find, g_find_n);
    g_ed.blk = IW_NO_BLOCK;
    while (iw_search(&g_ed.doc, &q, g_ed.cur, g_search_back, &g_rs, &g_re) == IW_OK) {
        if (g_confirm) { g_ed.blk = g_rs; g_ed.cur = g_re; g_mode = M_RASK; return; }
        if (replace_match() != IW_OK) { error("ERROR: Document too large", M_EDIT); return; }
    }
    error("* Not found *", M_EDIT);
}

static void search_key(char ch, uint8_t vk, int32_t mods)
{
    iw_doc_t q; iw_piece_t pc;
    uint32_t p, n, s, e;
    int plain = !(mods & (FLAIR_EVT_MOD_SHIFT_KEY | FLAIR_EVT_MOD_OPTION_KEY | FLAIR_EVT_MOD_CONTROL_KEY));
    query_view(&q, &pc);
    if (vk == VK_F1) {
#ifdef IWORD_MUT_SEARCH_CANCEL
        g_ed.cur = 0;
#endif
        g_mode = M_EDIT; return;
    }
    if (vk == VK_F2 || vk == VK_ESC || ch == 0x1B) {
        if (g_mode == M_RSEARCH) {
            if (!g_query_n) { g_mode = M_EDIT; return; }
            g_find_n = g_query_n;
            for (uint32_t i = 0; i < g_find_n; i++) g_find[i] = g_query[i];
            g_query_n = g_query_cur = 0; g_query_fresh = 0; g_mode = M_RTEXT; return;
        }
        if (g_mode == M_RTEXT) {
            g_replacement_n = g_query_n;
            for (uint32_t i = 0; i < g_replacement_n; i++) g_replacement[i] = g_query[i];
            /* Retain the search string for F2/F2, never the replacement. */
            g_query_n = g_find_n; g_query_cur = g_query_n;
            for (uint32_t i = 0; i < g_find_n; i++) g_query[i] = g_find[i];
            replace_next(); return;
        }
        if (iw_search(&g_ed.doc, &q, g_ed.cur, g_search_back, &s, &e) == IW_OK) {
            g_ed.cur = g_search_back ? s : e; g_ed.goal = 0xFFFFFFFFu; g_mode = M_EDIT;
        } else {
#ifdef IWORD_MUT_SEARCH_NOTFOUND
            error("Not found", M_EDIT);
#else
            error("* Not found *", M_EDIT);
#endif
        }
        return;
    }
    if ((vk == VK_UP || vk == VK_DOWN) && g_mode != M_RTEXT) { g_search_back = vk == VK_UP; return; }
    if (vk == VK_LEFT || vk == VK_RIGHT) {
        g_query_fresh = 0;
        if (vk == VK_LEFT && g_query_cur) g_query_cur = iw_unit_prev(&q, g_query_cur);
        if (vk == VK_RIGHT && g_query_cur < q.len) g_query_cur = iw_unit_next(&q, g_query_cur);
        return;
    }
    if (vk == VK_BSPC || ch == 8 || vk == VK_DEL) {
        g_query_fresh = 0;
        p = g_query_cur;
        if (vk != VK_DEL) { if (!p) return; p = iw_unit_prev(&q, p); }
        if (p == q.len) return;
        n = iw_unit_next(&q, p) - p;
        for (uint32_t i = p; i + n < g_query_n; i++) g_query[i] = g_query[i + n];
        g_query_n -= n; g_query_cur = p; return;
    }
    /* Reference p. 553 allows Tab, Indent and Hard Page in replacement.
     * Their position-bearing payloads are the existing MINT09/P2 codes;
     * replace_match's fixup fills the coordinates at the destination. */
    if (vk == VK_TAB || ch == '\t') {
        static const uint8_t tab[9] = {0xC1,2,0,0,0,0,0,0,0xC1};
        query_insert(tab, sizeof tab); return;
    }
    if (vk == VK_F4 && plain) {
        static const uint8_t indent[11] = {0xC2,0,0x58,2,0,0,0,0,0,0,0xC2};
        query_insert(indent, sizeof indent); return;
    }
    if (vk == VK_ENTER && (mods & FLAIR_EVT_MOD_CONTROL_KEY)) {
        uint8_t pg = IW_WP5_HPG; query_insert(&pg, 1); return;
    }
    /* Appearance in replacement text needs WP's Alt-R macro (p. 553),
     * which P3 does not claim. Codes ARE searchable/deletable as tokens. */
    if (plain && (vk == VK_F6 || vk == VK_F8) && g_mode != M_RTEXT) {
        uint8_t attr = vk == VK_F6 ? IW_WP5_ATTR_BOLD : IW_WP5_ATTR_UNDERLINE;
        uint8_t code[3];
        uint32_t state = g_query_fresh ? 0 : iw_attr_state(&q, g_query_cur);
        code[0] = code[2] = (state & (1u << attr)) ? 0xC4 : 0xC3; code[1] = attr;
        query_insert(code, 3); return;
    }
    if (ch >= ' ' && ch <= '~') { uint8_t c = (uint8_t)ch; query_insert(&c, 1); }
    else if (vk == VK_ENTER) { uint8_t c = 0x0A; query_insert(&c, 1); }
    else if (ch == 13 && (mods & FLAIR_EVT_MOD_CONTROL_KEY)) { uint8_t c = 0x0D; query_insert(&c, 1); }
}

static void replace_key(char ch, uint8_t vk)
{
    if (vk == VK_F1) { g_ed.blk = IW_NO_BLOCK; g_mode = M_EDIT; return; }
    if (g_mode == M_RCONF) {
        if (ch != 'y' && ch != 'Y' && ch != 'n' && ch != 'N' && vk != VK_ENTER) return;
        g_confirm = ch == 'y' || ch == 'Y';
        g_query_n = g_query_cur = 0; g_query_fresh = 0; g_mode = M_RSEARCH; return;
    }
    if (ch == 'y' || ch == 'Y' || ch == 'n' || ch == 'N' || vk == VK_ENTER) {
        int yes = ch != 'n' && ch != 'N';
#ifdef IWORD_MUT_REPLACE_CONFIRM
        yes = 1;
#endif
        g_ed.blk = IW_NO_BLOCK;
        if (yes) {
            if (replace_match() != IW_OK) { error("ERROR: Document too large", M_EDIT); return; }
        } else g_ed.cur = g_search_back ? g_rs : g_re;
        replace_next();
    }
}

static void undo_key(char ch, uint8_t vk)
{
    int restore = ch == '1';
    if (vk == VK_F1 || vk == VK_ESC) {
#ifdef IWORD_MUT_UNDO_CANCEL
        restore = 1;
#else
        g_mode = M_EDIT; return;
#endif
    }
    if (restore) {
        g_mode = M_EDIT;
        if (g_undo_count) {
            ensure_room();
            int rc = iw_ed_retrieve(&g_ed, g_undo[g_undo_sel], g_undo_n[g_undo_sel]);
#ifndef IWORD_MUT_UNDO_FIXUP
            if (rc == IW_OK) rc = iw_ed_fixup(&g_ed);
#endif
            if (rc != IW_OK) error("ERROR: Document too large", M_EDIT);
        }
    } else if ((ch == '2' || vk == VK_UP) && g_undo_sel+1 < g_undo_count) g_undo_sel++;
    else if (vk == VK_DOWN && g_undo_sel) g_undo_sel--;
}

static void key(int32_t message, int32_t mods)
{
    char ch = (char)(message & 0xFF);
    uint8_t vk = (uint8_t)((message >> 8) & 0xFF);
    uint32_t cur_before = g_ed.cur;
    int mode_before = g_mode;
    g_caps = (mods & (int32_t)FLAIR_EVT_MOD_ALPHA_LOCK) != 0;
    g_ed.reveal = g_reveal;                       /* a new document resets it */
    g_hin = g_home;                               /* Home is a prefix: it waits */
    g_home = 0;                                   /* for the next key (slice 3) */
    if (vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN || vk == VK_HOME || vk == VK_END)
        g_undo_join = 0;
    if (g_mode == M_GRAY) g_mode = M_EDIT;
    if (g_mode == M_EDIT) edit_key(ch, vk, mods);
    else if (g_mode == M_SEARCH || g_mode == M_RSEARCH || g_mode == M_RTEXT) search_key(ch, vk, mods);
    else if (g_mode == M_RCONF || g_mode == M_RASK) replace_key(ch, vk);
    else if (g_mode == M_UNDO) undo_key(ch, vk);
    else if (setup_mode()) setup_key(ch, vk);
    else if (format_mode()) format_key(ch, vk);
    else if (g_mode == M_RETRIEVE) retrieve_key(ch, vk, mods);
    else prompt_key(ch, vk);
    /* Reference p. 697: ANY cursor motion ends a deletion group, including
     * Search. The motion caused by the deletion itself remains in its group. */
#ifndef IWORD_MUT_UNDO_MOVEMENT
    if (g_ed.cur != cur_before &&
        !((mode_before == M_EDIT || mode_before == M_GRAY) && (vk == VK_DEL || vk == VK_BSPC || ch == 8)) &&
        !((mode_before == M_DELCODE || mode_before == M_DELBLOCK) && (ch == 'y' || ch == 'Y')) &&
        !(mode_before == M_MOVEBLK && ch == '3')) g_undo_join = 0;
#else
    (void)cur_before; (void)mode_before;
#endif
    paint();
}

/* ---- the application ---------------------------------------------------------------- */
int tbx_app_main(void)
{
    if (tbx_register() != TBX_OK) return 1;
    doc_reset();
    settings_load();
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
