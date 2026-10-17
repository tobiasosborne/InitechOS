/*
 * os/iword/core/iword.h -- InitechWord's document core: a PIECE TABLE over
 * an original buffer and an append-only add buffer, the WordPerfect 5.1
 * unit walk, word wrap (reflow) and the line layout Face A draws, plus the
 * .WP5 codec (os/iword/fs/wp5.c) -- THE ARTIFACT (C, ADR-0002).
 *
 * bead: initech-l4tj (IWORD P1). Epic initech-fdxa. Plan:
 *       docs/plans/INITECH-WORD-plan.md Sec 4 (one core, two faces), Sec 6(a)
 *       and 6(c) (the oracles), Sec 7 P1.
 *
 * THE DOCUMENT IS THE .WP5 BODY. The piece table holds the body bytes
 * exactly as WordPerfect 5.1 stores them (spec/iword/wp5_format.h): text
 * bytes, the hard return 0x0A, the SOFT RETURN 0x0D that REPLACES the space
 * at a wrap (MINT01 @0x188), and every other function code as an opaque,
 * unbroken UNIT (a fixed group, a variable group, a single code byte). A
 * golden therefore reads and writes back byte-for-byte. The ATTRIBUTE
 * codes (bold / underline / italic / any attribute number) are understood
 * since P2 slice 1 (bead initech-jojk, os/iword/core/codes.c): their state,
 * WordPerfect's rules for inserting / moving / deleting them, and their
 * Reveal Codes labels. Since P2 slice 2 the tab ([Tab], C1 sub 02), the
 * indent ([->Indent], C2), the hard page with its D4/00 group and the C6 of an
 * indented paragraph are understood too (their columns, their geometry, WP's
 * bookkeeping: edit.c iw_ed_fixup), and so are Block / Move (block.c); every
 * other code stays opaque.
 *
 * UNITS. Every position the core hands out (cursor, insert point, delete
 * span) is a unit boundary. A unit's LAST byte identifies it from the right
 * (text 0x20..0x7F, a single code < 0x20 or 0x80..0xBF, a fixed group ends
 * with its own code 0xC0..0xCF, a variable group with its own code >= 0xD0
 * after a mirrored size), so walking backwards is as exact as forwards.
 * Edits insert only text, hard returns, whole groups and whole retrieved
 * blocks and delete only whole units, so a well-formed body (validated on
 * load, iw_wp5_read) stays well-formed (test-iword-codes property C2).
 *
 * WRAP (iw_reflow_para). Within a paragraph (the units between two hard
 * returns / hard pages) every SRt is a space; the paragraph is re-wrapped at
 * `width` columns: a line takes the most words that fit, and the space that
 * ends it becomes the SRt. Text and the hard space are one column each;
 * every other code is zero width in P1 (tabs, centering and margins are P2).
 * A word longer than the line is broken by the LAYOUT only (no byte
 * changes). Reflow rewrites single bytes in place (0x20 <-> 0x0D), so it
 * never moves the cursor or any other position.
 *
 * Freestanding C: no libc, no OS calls (the app does all I/O through the
 * Toolbox Gate). Deterministic (Rule 11). ASCII-clean (Rule 12).
 */
#ifndef INITECH_IWORD_H
#define INITECH_IWORD_H

#include <stdint.h>

/* ---- result codes --------------------------------------------------------- */
#define IW_OK          0
#define IW_E_FULL     (-1)   /* add buffer, piece array or max length exhausted */
#define IW_E_RANGE    (-2)   /* a position past the end / not a unit boundary   */
#define IW_E_FORMAT   (-3)   /* not a WordPerfect 5.1 document (header)          */
#define IW_E_CODE     (-4)   /* a body code the corpus has not verified / a
                              * group whose trailer does not mirror its header  */
#define IW_E_PROTECTED (-5)  /* the unit is an opaque code: P1 does not delete it */
#define IW_E_ASK      (-6)   /* Backspace / Delete met an attribute code on the
                              * editing screen: nothing changed, WordPerfect
                              * asks "Delete [code]? No (Yes)" (e->ask holds
                              * the code; iw_ed_answer; bead initech-jojk)      */

/* ---- the piece table ------------------------------------------------------ */
#define IW_PIECE_ADD 0x80000000u   /* off bit 31: the piece lies in the add buffer */

typedef struct iw_piece {
    uint32_t off;        /* offset into orig, or (IW_PIECE_ADD | offset into add) */
    uint32_t len;        /* > 0 */
} iw_piece_t;

typedef struct iw_doc {
    const uint8_t *orig;     /* the original body (read only)   */
    uint32_t       orig_len;
    uint8_t       *add;      /* append only                      */
    uint32_t       add_len, add_cap;
    iw_piece_t    *pc;
    uint32_t       npc, pc_cap;
    uint32_t       len;      /* body length = sum of piece lengths */
    uint32_t       max_len;  /* inserts beyond this are refused (IW_E_FULL) */
} iw_doc_t;

/* Bind the buffers; the document is empty. */
int      iw_doc_init(iw_doc_t *d, uint8_t *add, uint32_t add_cap,
                     iw_piece_t *pc, uint32_t pc_cap, uint32_t max_len);
/* Replace the document by `n` bytes at `orig` (borrowed, not copied). The
 * add buffer and the pieces are reset. The caller validated the bytes. */
void     iw_doc_load(iw_doc_t *d, const uint8_t *orig, uint32_t n);
int      iw_doc_insert(iw_doc_t *d, uint32_t pos, const uint8_t *s, uint32_t n);
int      iw_doc_delete(iw_doc_t *d, uint32_t pos, uint32_t n);
/* Overwrite one byte in place (the reflow primitive). */
int      iw_doc_set(iw_doc_t *d, uint32_t pos, uint8_t b);
/* The byte at pos, or -1 past the end. */
int      iw_doc_byte(const iw_doc_t *d, uint32_t pos);
/* Copy bytes [pos, pos+n) to out; returns the count copied. */
uint32_t iw_doc_read(const iw_doc_t *d, uint32_t pos, uint8_t *out, uint32_t n);

/* Sequential reader (O(1) per byte). */
typedef struct iw_iter {
    const iw_doc_t *d;
    uint32_t pi, po, pos;
} iw_iter_t;
void     iw_iter_at(iw_iter_t *it, const iw_doc_t *d, uint32_t pos);
int      iw_iter_next(iw_iter_t *it);          /* byte, or -1 at the end */

/* ---- units ----------------------------------------------------------------- */
enum {
    IW_U_TEXT = 0,      /* one column, printable                            */
    IW_U_HRT,           /* hard return: ends the line and the paragraph     */
    IW_U_SRT,           /* soft return: ends the line (it is a space)       */
    IW_U_HPG,           /* hard page: ends the line, paragraph and page     */
    IW_U_HSPACE,        /* hard space: one column, never a break            */
    IW_U_CODE,          /* anything else: opaque, zero width (P1)           */
    IW_U_TAB,           /* C1 sub 02 [Tab]: to the next tab stop (slice 2)   */
    IW_U_INDENT,        /* C2 [->Indent]: to the next tab stop, and the rest
                         * of the paragraph wraps there (slice 2)            */
    IW_U_HIDDEN,        /* bookkeeping WP writes beside a code, never shown:
                         * C6 before the HRt of an indented paragraph, D4/00
                         * before a hard page (iw_ed_fixup keeps them)       */
    IW_U_DSRT,          /* 0x90 deletable soft return: ends the line (and,
                         * for the wrap, the paragraph) (slice 3, MINT22)    */
    IW_U_LEAD           /* D4/05: WP's unseen record at the START of a
                         * centered / right-justified line (slice 3; the
                         * cursor always rests after it)                     */
};
/* Length of the well-formed unit starting at b[pos] (n = bytes available),
 * or 0 when the bytes there are not a VERIFIED unit (spec/iword/
 * wp5_format.h; a group must mirror). The codec's validator. */
uint32_t iw_unit_len(const uint8_t *b, uint32_t n, uint32_t pos);
/* On a (well-formed) document: */
int      iw_unit_kind(const iw_doc_t *d, uint32_t pos);
uint32_t iw_unit_next(const iw_doc_t *d, uint32_t pos);   /* pos < len  */
uint32_t iw_unit_prev(const iw_doc_t *d, uint32_t pos);   /* pos > 0    */
int      iw_is_boundary(const iw_doc_t *d, uint32_t pos);

/* ---- wrap ------------------------------------------------------------------ */
/* Re-wrap the paragraph containing `pos` at `width` columns. */
int      iw_reflow_para(iw_doc_t *d, uint32_t pos, uint32_t width);
/* Re-wrap every paragraph. */
int      iw_reflow_all(iw_doc_t *d, uint32_t width);

/* ---- layout ---------------------------------------------------------------- */
typedef struct iw_line {
    uint32_t start;      /* first byte of the line                        */
    uint32_t end;        /* one past the line (after its terminator)      */
    uint32_t text_end;   /* where its terminator starts (== end if none)  */
    uint16_t cols;       /* columns of text on the line                   */
    uint8_t  term;       /* IW_U_HRT / IW_U_SRT / IW_U_HPG, or IW_U_TEXT for
                          * a layout break or the last line              */
    uint8_t  pad;
    uint16_t col0;       /* the column the line starts at: 0, or the left
                          * margin an indent earlier in its paragraph set */
    uint16_t margin;     /* the margin the NEXT line starts at (0 after a
                          * hard return / hard page)                     */
    /* slice 3 (bead initech-jojk; spec/iword/wp5_format.h "margins and
     * justification"): the columns above are RELATIVE to the left margin in
     * effect for the line; these place them on the paper. */
    uint16_t lm;         /* the left margin L (WPU from the paper's edge)  */
    uint16_t rm;         /* the right margin position 10200 - R (WPU)      */
    uint16_t x0;         /* where column col0 is on the paper (WPU): lm +
                          * 120 col0, or the centered / right-justified start */
    uint8_t  just;       /* IW_WP5_JUST_* of the line                      */
    uint8_t  fset;       /* a margin code is in effect (else `width` rules) */
    uint16_t sp;         /* the line spacing at its end, 8.8 fixed: how far
                          * down the next line starts (slice 3, MINT25)  */
} iw_line_t;
/* The paper position (WPU) of relative column `col` on line `ln`. */
#define IW_LINE_X(ln, col) ((uint32_t)(ln)->x0 + 120u * ((uint32_t)(col) - (uint32_t)(ln)->col0))
/* The screen column of paper position x: column 0 is 1" (rounded down). */
#define IW_SCREEN_COL(x) ((x) >= 1200u ? ((x) - 1200u) / 120u : 0u)
/* The line that starts at `start`. Every document has at least one line;
 * the last line ends at len with term IW_U_TEXT (it may be empty). */
void     iw_line_at(const iw_doc_t *d, uint32_t start, uint32_t width, iw_line_t *ln);

/* ---- the editor ------------------------------------------------------------ */
typedef struct iw_ed {
    iw_doc_t doc;
    uint32_t cur;        /* cursor: a unit boundary in [0, len]          */
    uint32_t width;      /* wrap columns (IW_WP5_WRAP_COLS for Face A)   */
    uint32_t goal;       /* the column Up/Down aim at                    */
    uint8_t  dirty;      /* changed since the last save / retrieve       */
    uint8_t  reveal;     /* Reveal Codes is on: Backspace / Delete remove an
                          * attribute code (and its partner) instead of
                          * asking (codes.c, initech-jojk)               */
    uint8_t  ask_del;    /* the pending question came from Delete (1) or
                          * Backspace (0)                                */
    uint32_t ask;        /* the code "Delete [code]?" is about           */
    uint32_t blk;        /* Block (Alt-F4): its anchor, or IW_NO_BLOCK   */
} iw_ed_t;
#define IW_NO_BLOCK 0xFFFFFFFFu

/* Read-only Search (Reference pp. 571-573). Query is a validated unit stream.
 * Returns the first complete match span in the chosen direction; no wrap to
 * the document's other end. No match / empty query returns IW_E_RANGE. */
int iw_search(const iw_doc_t *d, const iw_doc_t *query, uint32_t from, int back,
              uint32_t *start, uint32_t *end);

/* Where the cursor is: the line index from the top of the document, the
 * line's first byte and the cursor's column on it. */
typedef struct iw_where {
    uint32_t line, line_start, col;
    uint32_t page, page_line;   /* 1-based, IW_WP5_LINES_PER_PAGE per page */
    /* slice 3: the cursor on the paper and the format in effect there */
    uint32_t x;                 /* WPU from the paper's edge (Pos = x / 12) */
    uint32_t lm, rm;            /* the line's margins (WPU, iw_line_t)      */
    uint32_t just;              /* IW_WP5_JUST_* at the cursor              */
    uint32_t y;                 /* the line down its page, 1/256ths of a
                                 * line (Ln = 1" + y / 6 / 256; spacing)    */
    uint32_t sp;                /* the spacing in effect at the cursor (8.8
                                 * fixed; Format: Line shows it)           */
} iw_where_t;

void     iw_ed_init(iw_ed_t *e, uint32_t width);   /* doc already bound */
int      iw_ed_type(iw_ed_t *e, uint8_t ch);       /* insert + reflow    */
int      iw_ed_enter(iw_ed_t *e);                  /* hard return        */
int      iw_ed_backspace(iw_ed_t *e);
int      iw_ed_delete(iw_ed_t *e);
void     iw_ed_left(iw_ed_t *e);
void     iw_ed_right(iw_ed_t *e);
void     iw_ed_up(iw_ed_t *e);
void     iw_ed_down(iw_ed_t *e);
void     iw_ed_line_home(iw_ed_t *e);
void     iw_ed_line_end(iw_ed_t *e);
/* The Home prefix keys (slice 3; Reference Manual p. 103-104; MINT27):
 * iw_ed_line_home is Home, Left / Home, Home, Left (the first character,
 * past the codes), iw_ed_line_end Home, Right / End; iw_ed_line_start Home,
 * Home, Home, Left (before the codes); iw_ed_doc_top Home, Home, Up (after
 * the formatting codes at the top) / Home, Home, Home, Up (before_codes);
 * iw_ed_doc_end Home, Home, Down; iw_ed_to_line the line Home, Up / Home,
 * Down reach (the screen's top / bottom line, Face A's), the goal kept. */
void     iw_ed_line_start(iw_ed_t *e);
void     iw_ed_doc_top(iw_ed_t *e, int before_codes);
void     iw_ed_doc_end(iw_ed_t *e);
void     iw_ed_to_line(iw_ed_t *e, uint32_t line);
void     iw_ed_where(const iw_ed_t *e, iw_where_t *w);
/* The position on line `line` (0-based from the top) nearest column `col`. */
uint32_t iw_ed_pos_at(const iw_ed_t *e, uint32_t line, uint32_t col);
/* Lines [first, first + n) into out; returns how many exist. */
uint32_t iw_ed_lines(const iw_ed_t *e, uint32_t first, uint32_t n, iw_line_t *out);

/* ---- tabs, indents, hard pages (bead initech-jojk, P2 slice 2; edit.c) ---- *
 * WordPerfect 5.1's Tab, F4 (->Indent) and Ctrl-Enter (hard page), each a code
 * at the cursor, the cursor after it (spec/iword/wp5_format.h "tabs, indents
 * and hard pages"; mint/wpMINT09.steps). Columns: a tab or an indent moves to
 * the next stop (every IW_WP5_TAB_EVERY columns); an indent also sets the left
 * margin the rest of its paragraph wraps at.
 * iw_ed_fixup is WordPerfect's own bookkeeping, run after every edit: the
 * payloads of the tabs, indents and hard pages follow the layout (WP
 * recomputes them, MINT10), every hard page has its D4/00 group right before
 * it and every indented paragraph's HRt its C6 (and no stray ones remain);
 * the cursor keeps its place in the text. Editing ops leave this to it. */
int      iw_ed_tab(iw_ed_t *e);
int      iw_ed_indent(iw_ed_t *e);
int      iw_ed_hpage(iw_ed_t *e);
int      iw_ed_fixup(iw_ed_t *e);
/* The column of `pos` on line `ln` (tabs and indents expanded). */
uint32_t iw_line_col(const iw_doc_t *d, const iw_line_t *ln, uint32_t pos);

/* ---- the "Delete [code]? No (Yes)" question (slice 2; MINT11) -------------- *
 * On the editing screen Backspace / Delete meeting an attribute code return
 * IW_E_ASK with e->ask = the code. iw_ed_answer(e, yes): Yes deletes it and its
 * partner ("Alpha" F6 Backspace y -> "Alpha"); No moves the cursor over it --
 * before it for Backspace, past it for Delete (MINT11 L3 / L2). Tabs, indents
 * and hard pages are deleted without a question (MINT11 05_bs_tab, 06_bs_hpg;
 * MINT12 13_bs_indent). */
int      iw_ed_answer(iw_ed_t *e, int yes);

/* ---- Block (Alt-F4) and Move (Ctrl-F4) (slice 2; os/iword/core/block.c) ---- *
 * The block runs from the anchor to the cursor, [min, max) (MINT12/13). Its
 * COPY is self-contained: an on code for every attribute on where it starts,
 * its bytes, an off code for every attribute on where it ends (MINT14 "RS"
 * copied out of bold came back "[BOLD]RS[bold]"; MINT13 "ep[UND]u" came back
 * "ep[UND]u[und]"). CUTTING it (Move, Delete) leaves every other character in
 * its attributes: the codes that change between its two ends are put where it
 * was, the cursor after them (MINT13 "Dr[BOLD]old", "her[und]nd"). RETRIEVING
 * (Enter at "Move cursor; press Enter to retrieve.") inserts the copy at the
 * cursor, the cursor staying in front of it (MINT12 06_moved Pos 2.2"), and
 * then drops every code that changes nothing from there on -- an on code of
 * an attribute already on, an off code of one already off (MINT14 "xy" into
 * bold is bold; MINT15 "[BOLD]w[bold]" into "vwx|yz" -> "vwxw[bold]yz"). */
void     iw_ed_block(iw_ed_t *e);                       /* Alt-F4: on / off */
void     iw_ed_block_span(const iw_ed_t *e, uint32_t *s, uint32_t *t);
/* The self-contained copy into buf (cap bytes); IW_E_FULL when it does not fit. */
int      iw_ed_block_copy(iw_ed_t *e, uint8_t *buf, uint32_t cap, uint32_t *n);
/* Cut the block (Move: buf receives the copy; Delete: buf == 0). Block off. */
int      iw_ed_block_cut(iw_ed_t *e, uint8_t *buf, uint32_t cap, uint32_t *n);
int      iw_ed_retrieve(iw_ed_t *e, const uint8_t *buf, uint32_t n);
/* An attribute given to the block (slice 3; Reference Manual p. 37; MINT28):
 * its on code at the block's start, its off code at the end, Block off; the
 * cursor keeps its end (after the off code at the end, after the on code at
 * the start). Without a block it is iw_ed_attr. */
int      iw_ed_block_attr(iw_ed_t *e, uint8_t attr);
/* Move without a block (Ctrl-F4 1 / 2 / 3; slice 3; Reference Manual p.
 * 440-442; MINT31): what 1 the sentence the cursor is on (through its
 * punctuation and up to three spaces after it; an [HRt] ends one), 2 the
 * paragraph (after the [HRt] before it, through its own [HRt]), 3 the page.
 * It becomes the block: the anchor at its start, the cursor at its end. */
int      iw_ed_select(iw_ed_t *e, int what);

/* ---- margins and justification (bead initech-jojk, P2 slice 3; edit.c) ---- *
 * Shift-F8 1 7 / Shift-F8 1 3 (spec/iword/wp5_format.h "margins and
 * justification"; mint/wpMINT21..24.steps). iw_ed_margins puts a [L/R Mar]
 * code at the cursor -- a HRt first when the cursor is not at the left
 * margin (Reference Manual p. 394; MINT21, MINT23) -- and re-wraps every
 * paragraph after it; iw_ed_just puts a [Just:] code there -- a [DSRt] first
 * when not at the left margin (MINT22). L and R in WPU (1200 an inch); j an
 * IW_WP5_JUST_* value. iw_ed_fixup keeps WP's bookkeeping: each code's OLD
 * values (the format before it, MINT24), the D4/05 line records (MINT22,
 * MINT24A), the tab / indent / C6 payloads measured from the margins in
 * effect (MINT21, MINT23).
 * Backspace / Delete meeting a [L/R Mar] or [Just:] code ask "Delete
 * [code]? No (Yes)" like an attribute code (MINT23 06/08, MINT24 04); Yes
 * deletes the code alone. */
int      iw_ed_margins(iw_ed_t *e, uint32_t left_wpu, uint32_t right_wpu);
int      iw_ed_just(iw_ed_t *e, uint32_t just);
/* Line spacing (Shift-F8 1 6; spec/iword/wp5_format.h "line spacing";
 * mint/wpMINT25/26/30.steps): a [Ln Spacing:] code at the cursor, never a
 * return before it (MINT25 "Mid[Ln Spacing:1]single"); sp 8.8 fixed
 * (IW_WP5_SPACING_ONE = single). Its old value is kept by iw_ed_fixup
 * (MINT26). Asked about before deletion like the others (Reference Manual,
 * Line Spacing p. 345). */
int      iw_ed_spacing(iw_ed_t *e, uint32_t sp);
/* Screen rows a line of spacing sp takes on the editing screen: the
 * spacing rounded to the nearest whole line, at least one (MINT25 1.5 -> 2,
 * MINT26 1.33 -> 1, MINT30 2.25 -> 2). */
uint32_t iw_spacing_rows(uint32_t sp);
/* A spacing typed in hundredths ("1.33" = 133) as WP stores it, 8.8 fixed
 * (MINT30: rounded up). */
uint32_t iw_spacing_from_hundredths(uint32_t h);
/* 1 when the unit at pos is a [L/R Mar], [Just:] or [Ln Spacing:] code
 * (asked about). */
int      iw_code_is_format(const iw_doc_t *d, uint32_t pos);
/* Re-wrap every paragraph from the one holding pos to the end. */
int      iw_reflow_from(iw_doc_t *d, uint32_t pos, uint32_t width);

/* WordPerfect's status-line measures (hundredths of an inch, 1" margins):
 * Ln = 1" + (line on page - 1) / 6", Pos = 1" + column / 10". Formats
 * "1", "1.17", "2.6" (trailing zeros and a bare point dropped) + '"'. */
uint32_t iw_ln_hundredths(uint32_t page_line);
/* Ln from y (iw_where_t.y): 1" + y / 256 / 6 inches, rounded (MINT25
 * "Ln 2.83"", MINT26 "Ln 2.33""). */
uint32_t iw_ln_y_hundredths(uint32_t y);
uint32_t iw_pos_hundredths(uint32_t col);
uint32_t iw_fmt_inches(uint32_t hundredths, char *out);   /* returns length */

/* ---- attribute codes (os/iword/core/codes.c; bead initech-jojk) ----------- *
 * An attribute is a PAIR of fixed groups, `C3 n C3` (on) ... `C4 n C4` (off),
 * n the WordPerfect attribute number (spec/iword/wp5_format.h IW_WP5_ATTR_*).
 * The state at a position is read from the codes before it: attribute n is ON
 * when the last code of n before the position is an ON code. The editor's
 * rules are WordPerfect 5.1's (Reference Manual, Attributes p. 36-37, PDF
 * page "ATTRIBUTES 37"; the real WP.EXE in MINT01/06/07/08):
 *   iw_ed_attr (F6 Bold, F8 Underline, Ctrl-F8 2 n): n OFF at the cursor ->
 *     "both an on and off code are inserted, with the cursor resting just
 *     before the off code"; n ON -> the off code of the pair the cursor is in
 *     is MOVED to the cursor and the cursor goes past it (MINT06: F6 typed
 *     after "both" with [und][bold] ahead moved [bold] to the cursor; when the
 *     off code is right at the cursor that is "press that keystroke once
 *     again ... to move beyond the attribute off code"). No off code ahead
 *     (an unpaired on code) -> one off code is inserted at the cursor.
 *   iw_ed_normal (Ctrl-F8 3 Normal): "This either inserts the necessary off
 *     codes or moves the cursor beyond existing off codes" -- the cursor steps
 *     past the off codes at it, then every attribute still on is turned off as
 *     iw_ed_attr does, in ascending attribute number [authored order; MINT04
 *     verifies the step-past case].
 *   Backspace / Delete on an attribute code with `reveal` set delete the code
 *   AND its partner (MINT08 03b: Backspace on [italc] removed [ITALC] too).
 *   Without `reveal` they ask WordPerfect's question (IW_E_ASK, slice 2,
 *   MINT11); on any other opaque code they still step over it (P1). */
#define IW_A_BOLD       0x01u   /* the Face A draw faces == TBX txFace bits */
#define IW_A_ITALIC     0x02u
#define IW_A_UNDERLINE  0x04u
/* The attribute number of the C3/C4 unit at pos (on = 1 for C3), or -1. */
int      iw_code_attr(const iw_doc_t *d, uint32_t pos, int *on);
/* Bit n set = attribute n is ON at pos (the codes in [0, pos)). */
uint32_t iw_attr_state(const iw_doc_t *d, uint32_t pos);
/* Attribute-number state -> the IW_A_* faces Face A draws (bold, italic,
 * underline; every other attribute draws plain). */
uint32_t iw_attr_faces(uint32_t state);
/* State after the unit at pos (pos a unit start): `state` updated. */
uint32_t iw_attr_step(const iw_doc_t *d, uint32_t pos, uint32_t state);
int      iw_ed_attr(iw_ed_t *e, uint8_t attr);
int      iw_ed_normal(iw_ed_t *e);
/* Delete the attribute code at pos and its partner (Reveal Codes Backspace /
 * Delete); IW_E_PROTECTED when the unit is not an attribute code. */
int      iw_ed_kill_code(iw_ed_t *e, uint32_t pos);

/* ---- Reveal Codes (os/iword/core/codes.c; spec/iword/reveal_codes.h) ------- *
 * iw_reveal_line: the labels of document line `ln` (text as itself, every
 * code as its Reveal Codes label, the terminator's label last) into out
 * (NUL-terminated, at most cap - 1 chars); returns the length. When `cur`
 * lies on the line, *cur_off / *cur_len are the run the pane highlights: the
 * label (or character) of the unit at the cursor, or one cell at the end of
 * the line's labels when the cursor is at the end of the document; otherwise
 * *cur_len = 0 and *cur_off = 0xFFFFFFFF. While Block is on (blk != IW_NO_BLOCK)
 * the label "[Block]" stands at the anchor (MINT13 01b_block_reveal). */
#define IW_REVEAL_LINE_MAX 1024u
uint32_t iw_reveal_line(const iw_doc_t *d, const iw_line_t *ln, uint32_t cur, uint32_t blk,
                        char *out, uint32_t cap, uint32_t *cur_off, uint32_t *cur_len);
/* The Reveal Codes label of the unit at pos (NUL-terminated); its length. */
uint32_t iw_code_label(const iw_doc_t *d, uint32_t pos, char *out, uint32_t cap);
/* The pane: `nrows` rows of `width` columns (row r at rows + r * stride, each
 * NUL-terminated, width < stride; rows past the document are ""), starting at
 * the anchor line (IW_RV_PANE_ABOVE lines above the cursor's). */
typedef struct iw_pane {
    uint32_t first_line;             /* the document line on row 0          */
    uint32_t cur_row, cur_col, cur_len;   /* the highlight; cur_len 0 = none  */
} iw_pane_t;
void     iw_reveal_pane(const iw_ed_t *e, uint32_t width, uint32_t nrows,
                        char *rows, uint32_t stride, iw_pane_t *pane);

/* ---- the .WP5 codec (os/iword/fs/wp5.c) ------------------------------------ */
typedef struct iw_wp5 {
    const uint8_t *prefix;   uint32_t prefix_len;   /* header + prefix area */
    const uint8_t *body;     uint32_t body_len;
    uint32_t err_off;                               /* where a refusal hit  */
} iw_wp5_t;
/* Validate a whole file (header [H Sec 1] and every body unit). */
int      iw_wp5_read(const uint8_t *file, uint32_t n, iw_wp5_t *out);
/* prefix + body into out; IW_E_FULL when it does not fit. `cur` is the
 * cursor at the save -- the default prefix records whether it was before the
 * end (spec/iword/wp5_format.h IW_WP5_PFX_CURSOR_OFF; bead initech-jojk
 * slice 2) -- or IW_WP5_CUR_KEEP to write the prefix exactly as given. */
#define IW_WP5_CUR_KEEP 0xFFFFFFFFu
int      iw_wp5_write(const uint8_t *prefix, uint32_t prefix_len, const iw_doc_t *d, uint32_t cur,
                      uint8_t *out, uint32_t cap, uint32_t *out_len);
/* The default prefix a new document is written with (327 bytes). */
const uint8_t *iw_wp5_default_prefix(uint32_t *len);
/* The document's text as WordPerfect's own reader sees it: the text bytes
 * outside every group, concatenated. Returns the full length (out holds at
 * most cap bytes). */
uint32_t iw_wp5_text(const iw_doc_t *d, char *out, uint32_t cap);

#endif /* INITECH_IWORD_H */
