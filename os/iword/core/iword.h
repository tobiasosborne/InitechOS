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
 * golden therefore reads and writes back byte-for-byte without the core
 * understanding its codes (those are P2, bead initech-jojk).
 *
 * UNITS. Every position the core hands out (cursor, insert point, delete
 * span) is a unit boundary. A unit's LAST byte identifies it from the right
 * (text 0x20..0x7F, a single code < 0x20 or 0x80..0xBF, a fixed group ends
 * with its own code 0xC0..0xCF, a variable group with its own code >= 0xD0
 * after a mirrored size), so walking backwards is as exact as forwards.
 * Edits insert only text and hard returns and delete only whole units, so a
 * well-formed body (validated on load, iw_wp5_read) stays well-formed.
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
    IW_U_CODE           /* anything else: opaque, zero width (P1)           */
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
} iw_line_t;
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
} iw_ed_t;

/* Where the cursor is: the line index from the top of the document, the
 * line's first byte and the cursor's column on it. */
typedef struct iw_where {
    uint32_t line, line_start, col;
    uint32_t page, page_line;   /* 1-based, IW_WP5_LINES_PER_PAGE per page */
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
void     iw_ed_where(const iw_ed_t *e, iw_where_t *w);
/* The position on line `line` (0-based from the top) nearest column `col`. */
uint32_t iw_ed_pos_at(const iw_ed_t *e, uint32_t line, uint32_t col);
/* Lines [first, first + n) into out; returns how many exist. */
uint32_t iw_ed_lines(const iw_ed_t *e, uint32_t first, uint32_t n, iw_line_t *out);

/* WordPerfect's status-line measures (hundredths of an inch, 1" margins):
 * Ln = 1" + (line on page - 1) / 6", Pos = 1" + column / 10". Formats
 * "1", "1.17", "2.6" (trailing zeros and a bare point dropped) + '"'. */
uint32_t iw_ln_hundredths(uint32_t page_line);
uint32_t iw_pos_hundredths(uint32_t col);
uint32_t iw_fmt_inches(uint32_t hundredths, char *out);   /* returns length */

/* ---- the .WP5 codec (os/iword/fs/wp5.c) ------------------------------------ */
typedef struct iw_wp5 {
    const uint8_t *prefix;   uint32_t prefix_len;   /* header + prefix area */
    const uint8_t *body;     uint32_t body_len;
    uint32_t err_off;                               /* where a refusal hit  */
} iw_wp5_t;
/* Validate a whole file (header [H Sec 1] and every body unit). */
int      iw_wp5_read(const uint8_t *file, uint32_t n, iw_wp5_t *out);
/* prefix + body into out; IW_E_FULL when it does not fit. */
int      iw_wp5_write(const uint8_t *prefix, uint32_t prefix_len, const iw_doc_t *d,
                      uint8_t *out, uint32_t cap, uint32_t *out_len);
/* The default prefix a new document is written with (327 bytes). */
const uint8_t *iw_wp5_default_prefix(uint32_t *len);
/* The document's text as WordPerfect's own reader sees it: the text bytes
 * outside every group, concatenated. Returns the full length (out holds at
 * most cap bytes). */
uint32_t iw_wp5_text(const iw_doc_t *d, char *out, uint32_t cap);

#endif /* INITECH_IWORD_H */
