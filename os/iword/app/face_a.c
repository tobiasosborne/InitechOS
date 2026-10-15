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
 * KEYS. Typing, Enter (hard return), Backspace, Delete, the arrows, Home /
 * End (start / end of the line -- WP's Home is a prefix key; the shortcut
 * is authored). Word wrap as WordPerfect stores it (os/iword/core/edit.c).
 * ATTRIBUTE CODES (bead initech-jojk, P2 slice 1; os/iword/core/codes.c):
 *   F6 Bold, F8 Underline -- "both an on and off code are inserted, with the
 *                   cursor resting just before the off code"; the key again
 *                   moves past it (Reference Manual, Attributes p. 36-37).
 *   Ctrl-F8 Font    the Font menu on the status line, then 2 Appearance
 *                   (1 Bold, 2 Undln, 4 Italc) or 3 Normal; the menu lines
 *                   are the real WP 5.1's (spec/iword/reveal_codes.h). Size,
 *                   Base Font, Print Color and the other appearances have no
 *                   minted code number yet and return to the document.
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
 *                   Append are not in this slice (any other key: back).
 *   Delete / Bksp   with Block on: "Delete Block? No (Yes)" (09_delete_prompt):
 *                   y deletes it, any other key keeps it.
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
#include "iword.h"
#include "iword/wp5_format.h"
#include "iword/reveal_codes.h"

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
       M_DELCODE, M_DELBLOCK, M_MOVE, M_MOVEBLK, M_RETRIEVE };
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
    if (r->blk_len) put_cells(line, r->blk_col, r->t + r->blk_col, r->blk_len, TBX_COLOR_BLACK, TBX_COLOR_GRAY);
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
       ED_HPAGE, ED_ANSWER, ED_CUT, ED_COPY, ED_RETRIEVE };
static int edit_once(int op, uint8_t ch)
{
    switch (op) {
    case ED_TYPE:      return iw_ed_type(&g_ed, ch);
    case ED_ENTER:     return iw_ed_enter(&g_ed);
    case ED_BACKSPACE: return iw_ed_backspace(&g_ed);
    case ED_ATTR:      return iw_ed_attr(&g_ed, ch);      /* ch = attribute number */
    case ED_NORMAL:    return iw_ed_normal(&g_ed);
    case ED_TAB:       return iw_ed_tab(&g_ed);
    case ED_INDENT:    return iw_ed_indent(&g_ed);
    case ED_HPAGE:     return iw_ed_hpage(&g_ed);
    case ED_ANSWER:    return iw_ed_answer(&g_ed, ch);   /* ch = yes */
    case ED_CUT:       return iw_ed_block_cut(&g_ed, ch ? g_clip : 0, sizeof g_clip, &g_clip_n);
    case ED_COPY:      return iw_ed_block_copy(&g_ed, g_clip, sizeof g_clip, &g_clip_n);
    case ED_RETRIEVE:  return iw_ed_retrieve(&g_ed, g_clip, g_clip_n);
    default:           return iw_ed_delete(&g_ed);
    }
}
static int edit(int op, uint8_t ch)
{
    uint32_t before = g_ed.doc.len;
    int rc, fx;
    ensure_room();
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
static void row_text(const iw_line_t *ln, uint32_t *state, row_t *r, uint32_t bs, uint32_t bt)
{
    uint32_t p = ln->start, c = ln->col0, b0 = SCR_COLS, b1 = 0;
    row_clear(r);
    while (p < ln->end) {
        int k = iw_unit_kind(&g_ed.doc, p);
        uint32_t q = iw_unit_next(&g_ed.doc, p), c1 = c;
        if (p < ln->text_end) {
            if (k == IW_U_TEXT || k == IW_U_HSPACE) {
                if (c < SCR_COLS) {
                    r->t[c] = k == IW_U_TEXT ? (char)iw_doc_byte(&g_ed.doc, p) : ' ';
                    r->a[c] = (uint8_t)iw_attr_faces(*state);
                }
                c1 = c + 1u;
            } else if (k == IW_U_TAB || k == IW_U_INDENT) {
                c1 = iw_line_col(&g_ed.doc, ln, q);      /* blank, plain */
            }
            if (p >= bs && p < bt && c1 > c) {
                if (c < b0) b0 = c;
                if (c1 > b1) b1 = c1;
            }
        }
        if (k == IW_U_CODE) *state = iw_attr_step(&g_ed.doc, p, *state);
        c = c1;
        p = q;
    }
    if (b1 > SCR_COLS) b1 = SCR_COLS;
    if (b0 < b1) { r->blk_col = (uint8_t)b0; r->blk_len = (uint8_t)(b1 - b0); }
}

/* Keep the cursor's line among the `rows` screen rows (a hard page's bar
 * takes a row of its own). */
static void follow(const iw_where_t *w, uint32_t rows)
{
    static iw_line_t lines[EDIT_ROWS];
    if (w->line < g_top) g_top = w->line;
    for (;;) {
        uint32_t n, got, k, used = 0;
        if (w->line >= g_top + rows) g_top = w->line - (rows - 1u);
        n = w->line - g_top + 1u;
        got = iw_ed_lines(&g_ed, g_top, n, lines);
        for (k = 0; k < got; k++) used += (k + 1u < got && lines[k].term == IW_U_HPG) ? 2u : 1u;
        if (used <= rows || g_top >= w->line) return;
        g_top++;
    }
}

static void paint_status(const iw_where_t *w, int line)
{
    char s[SCR_COLS + 1], num[12], doc[32];
    uint32_t n, i, k;
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
        k = iw_fmt_inches(iw_ln_hundredths(w->page_line), num);
        for (i = 0; i < k; i++) doc[n++] = num[i];
        for (i = 0; i < n; i++) s[STATUS_DOC_END + 1 - n + i] = doc[i];
        s[STATUS_POS] = 'P';
        s[STATUS_POS + 1] = g_caps ? 'O' : 'o';
        s[STATUS_POS + 2] = g_caps ? 'S' : 's';
        k = iw_fmt_inches(iw_pos_hundredths(w->col), num);
        for (i = 0; i < k && STATUS_POS + 4 + i < SCR_COLS; i++) s[STATUS_POS + 4 + i] = num[i];
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
        else if (g_mode == M_FONT) t = IW_FONT_MENU;
        else if (g_mode == M_APPEAR) t = IW_APPEAR_MENU;
        else if (g_mode == M_MOVE) t = IW_MOVE_MENU;
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
        if (g_mode == M_SAVE_NAME || g_mode == M_GET_NAME)
            for (i = 0; i < g_nlen && n < SCR_COLS; i++) s[n++] = g_name[i];
        if (g_mode == M_SAVEQ && !g_ed.dirty) {
            const char *nm = "(Text was not modified)";
            k = slen(nm);
            for (i = 0; i < k; i++) s[SCR_COLS - k + i] = nm[i];
        }
    }
    put_text_row(line, s);
}

/* The document rows 0 .. rows-1, the cursor in reverse video, a hard
 * page's bar below its line, the block (if on) in its run. */
static void paint_doc(const iw_where_t *w, uint32_t rows)
{
    static iw_line_t lines[EDIT_ROWS];
    static row_t r;
    uint32_t got, li, i = 0, state = 0, bs = 1u, bt = 0;
    int ccol;
    follow(w, rows);
    got = iw_ed_lines(&g_ed, g_top, rows, lines);
    if (got) state = iw_attr_state(&g_ed.doc, lines[0].start);
    if (g_ed.blk != IW_NO_BLOCK) iw_ed_block_span(&g_ed, &bs, &bt);
    ccol = (int)(w->col < SCR_COLS ? w->col : SCR_COLS - 1u);
    for (li = 0; li < got && i < rows; li++) {
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
        put_row((int)i++, &r);
        if (lines[li].term == IW_U_HPG && i < rows) {
            row_clear(&r);
            for (uint32_t c = 0; c < SCR_COLS; c++) r.t[c] = '=';
            put_row((int)i++, &r);
        }
    }
    while (i < rows) { row_clear(&r); put_row((int)i++, &r); }
}

/* Reveal Codes: the ruler, the codes pane and the restore message. */
static char g_pane[IW_RV_PANE_ROWS][SCR_COLS + 1];
static void paint_reveal(void)
{
    static row_t r;
    iw_pane_t pane;
    uint32_t i;
    row_clear(&r);
    r.fg = (uint8_t)TBX_COLOR_BLACK;           /* authored: WP's band is red */
    r.bg = (uint8_t)TBX_COLOR_GRAY;
    for (i = IW_RV_RULER_EVERY; i < SCR_COLS; i += IW_RV_RULER_EVERY) r.t[i] = (char)IW_RV_RULER_TAB;
    r.t[0] = IW_RV_RULER_LEFT;
    r.t[IW_WP5_WRAP_COLS] = IW_RV_RULER_RIGHT;
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

static void paint(void)
{
    iw_where_t w;
    iw_ed_where(&g_ed, &w);
    if (!g_reveal) {
        paint_doc(&w, EDIT_ROWS);
        paint_status(&w, LINE_STATUS);
        return;
    }
    paint_doc(&w, RV_TOP_ROWS);
    paint_status(&w, RV_STATUS);
    paint_reveal();
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
    int font_rc = IW_OK;
    if (g_mode == M_ERROR) { g_mode = g_err_return; return; }
    if (vk == VK_F1 || vk == VK_ESC || ch == 0x1B) {   /* Cancel */
        g_exiting = 0;
        g_ed.ask = 0xFFFFFFFFu;                         /* a question is dropped */
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
        /* IW_FONT_MENU: 2 Appearance, 3 Normal. Size, Base Font and Print
         * Color are not in this slice: any other key returns to the
         * document, as Cancel does. */
        g_mode = M_EDIT;
        if (ch == '2') g_mode = M_APPEAR;
        else if (ch == '3') font_rc = edit(ED_NORMAL, 0);
        break;
    case M_APPEAR:
        /* IW_APPEAR_MENU: 1 Bold, 2 Undln, 4 Italc (the attribute numbers
         * the mints verify, spec/iword/wp5_format.h); the other six have no
         * verified number and return to the document unapplied. */
        g_mode = M_EDIT;
        if (ch == '1') font_rc = edit(ED_ATTR, IW_WP5_ATTR_BOLD);
        else if (ch == '2') font_rc = edit(ED_ATTR, IW_WP5_ATTR_UNDERLINE);
        else if (ch == '4') font_rc = edit(ED_ATTR, IW_WP5_ATTR_ITALIC);
        break;
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
    case M_MOVEBLK:
        /* 1 Move, 2 Copy: the block goes to the buffer, then the Move prompt;
         * 3 Delete; 4 Append (to a file) is not in this slice */
        g_mode = M_EDIT;
        if (ch == '1') { font_rc = edit(ED_CUT, 1); if (font_rc == IW_OK) g_mode = M_RETRIEVE; }
        else if (ch == '2') { font_rc = edit(ED_COPY, 0); if (font_rc == IW_OK) g_mode = M_RETRIEVE; }
        else if (ch == '3') font_rc = edit(ED_CUT, 0);
        break;
    default:
        g_mode = M_EDIT;
        break;
    }
    if (font_rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
}

/* The cursor keys (the editing screen, Block, the Move prompt). */
static int cursor_key(uint8_t vk)
{
    switch (vk) {
    case VK_UP:    iw_ed_up(&g_ed); return 1;
    case VK_DOWN:  iw_ed_down(&g_ed); return 1;
    case VK_LEFT:  iw_ed_left(&g_ed); return 1;
    case VK_RIGHT: iw_ed_right(&g_ed); return 1;
    case VK_HOME:  iw_ed_line_home(&g_ed); return 1;
    case VK_END:   iw_ed_line_end(&g_ed); return 1;
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
    if (ch == 0 && cursor_key(vk)) return;
    if (vk == VK_F4 && alt) { iw_ed_block(&g_ed); return; }
    if (vk == VK_F4 && ctrl) { g_mode = M_MOVE; return; }
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
    if (ch == 0 && cursor_key(vk)) return;
    if (vk == VK_F3 && alt) { toggle_reveal(); return; }
    if (vk == VK_ENTER || ch == '\r') {
        g_mode = M_EDIT;
        rc = edit(ED_RETRIEVE, 0);
        if (rc == IW_E_FULL) error("ERROR: Document too large", M_EDIT);
        return;
    }
    if (vk == VK_F1 || vk == VK_ESC || ch == 0x1B) g_mode = M_EDIT;
}

static void edit_key(char ch, uint8_t vk, int32_t mods)
{
    int shift = (mods & (int32_t)FLAIR_EVT_MOD_SHIFT_KEY) != 0;
    int ctrl  = (mods & (int32_t)FLAIR_EVT_MOD_CONTROL_KEY) != 0;
    int alt   = (mods & (int32_t)FLAIR_EVT_MOD_OPTION_KEY) != 0;
    int rc = IW_OK;
    if (g_ed.blk != IW_NO_BLOCK) { block_key(ch, vk, mods); return; }
    if (ch == '\t' || vk == VK_TAB) {   /* Tab: [Tab] (MINT09) */
        rc = edit(ED_TAB, 0);
    } else if (ch == 0) {                /* function and cursor keys */
        switch (vk) {
        case VK_F6:                      /* Bold; Shift-F6 Center etc. are later slices */
            if (!shift && !ctrl && !alt) rc = edit(ED_ATTR, IW_WP5_ATTR_BOLD);
            break;
        case VK_F8:                      /* Underline; Ctrl-F8 Font */
            if (ctrl && !shift && !alt) g_mode = M_FONT;
            else if (!shift && !ctrl && !alt) rc = edit(ED_ATTR, IW_WP5_ATTR_UNDERLINE);
            break;
        case VK_F3:                      /* Alt-F3 Reveal Codes (F3 Help: not here) */
            if (alt && !shift && !ctrl) toggle_reveal();
            break;
        case VK_F4:                      /* F4 ->Indent, Alt-F4 Block, Ctrl-F4 Move */
            if (alt && !shift && !ctrl) iw_ed_block(&g_ed);
            else if (!shift && !ctrl && !alt) rc = edit(ED_INDENT, 0);
            /* Ctrl-F4 without a block (Sentence / Paragraph / Page) is not
             * in this slice */
            break;
        case VK_F10:
            g_exiting = 0;
            if (shift) { g_nlen = 0; g_name[0] = 0; g_fresh = 0; g_mode = M_GET_NAME; }
            else { offer_name(); g_mode = M_SAVE_NAME; }
            break;
        case VK_F7:    request_exit(FROM_F7); break;
        case VK_DEL:   rc = edit(ED_DELETE, 0); break;
        default:       (void)cursor_key(vk); break;
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

static void key(int32_t message, int32_t mods)
{
    char ch = (char)(message & 0xFF);
    uint8_t vk = (uint8_t)((message >> 8) & 0xFF);
    g_caps = (mods & (int32_t)FLAIR_EVT_MOD_ALPHA_LOCK) != 0;
    g_ed.reveal = g_reveal;                       /* a new document resets it */
    if (g_mode == M_EDIT) edit_key(ch, vk, mods);
    else if (g_mode == M_RETRIEVE) retrieve_key(ch, vk, mods);
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
