/* kit.h -- W4.1 full-screen kit (EXPERIMENT initech-7cbu8, exp/dbase-oneshot).
 * Built ONLY into SAMIRX.EXE (-DSAMIR_W4); never into the shipped SAMIR.EXE.
 * Ref: dBASE III PLUS full-screen conventions, Using III+ (initech-manuals-ref)
 * and dbase3-decomp/specs (screens: menu bar row 0, status bar row 22,
 * navigation/message rows 23-24). Plain 80x25 text, attribute per cell.
 * Output goes ONLY through the PAL (set_attr + paint / gotoxy); input ONLY
 * through pal->conin_char. Owner of the implementation: ui/kit.c.
 */
#ifndef SAMIR_KIT_H
#define SAMIR_KIT_H
#include <stdint.h>
#include "samir/pal.h"
#include "samir/interp.h"

#define KIT_ROWS 25
#define KIT_COLS 80
#define KIT_ATTR_NORMAL  0x07
#define KIT_ATTR_REVERSE 0x70
#define KIT_ATTR_BRIGHT  0x0F

/* Decoded keys returned by kit_key(). Printable ASCII is returned as itself
 * (0x20..0x7E). DOS extended keys (0,scan) and the dBASE control-key
 * equivalents (Ctrl-E = up, Ctrl-X = down, Ctrl-S = left, Ctrl-D = right,
 * Ctrl-R = PgUp, Ctrl-C = PgDn, Ctrl-W / Ctrl-End = save+exit, Ctrl-Q = abort,
 * Ctrl-U = toggle delete, Ctrl-N = insert, Ctrl-V = Ins toggle, Ctrl-G = Del)
 * are folded to these codes. */
enum {
    KIT_K_NONE = 0,
    KIT_K_ENTER = 0x0D, KIT_K_ESC = 0x1B, KIT_K_BKSP = 0x08, KIT_K_TAB = 0x09,
    KIT_K_UP = 0x100, KIT_K_DOWN, KIT_K_LEFT, KIT_K_RIGHT,
    KIT_K_PGUP, KIT_K_PGDN, KIT_K_HOME, KIT_K_END,
    KIT_K_INS, KIT_K_DEL, KIT_K_SAVE /* ^W/^End */, KIT_K_ABORT /* ^Q */,
    KIT_K_TOGGLE_DEL /* ^U */, KIT_K_F1, KIT_K_F10,
    KIT_K_CTRL_HOME, KIT_K_CTRL_END, KIT_K_CTRL_N, KIT_K_CTRL_Y,
    KIT_K_EOF = -1
};

typedef struct kit {
    samir_pal_t *pal;
    xb_interp *ip;
    char    ch[KIT_ROWS][KIT_COLS];
    uint8_t at[KIT_ROWS][KIT_COLS];
    uint8_t attr;          /* current attribute for kit_put */
    uint8_t cur_row, cur_col;
} kit;

/* Saved full screen (kit_save / kit_restore). */
typedef struct kit_saved {
    char    ch[KIT_ROWS][KIT_COLS];
    uint8_t at[KIT_ROWS][KIT_COLS];
    uint8_t cur_row, cur_col;
} kit_saved;

/* Lifecycle: kit_open clears the screen; kit_close clears it and leaves the
 * cursor at row 0 col 0 ready for the dot prompt. */
void kit_open(kit *k, xb_interp *ip);
void kit_close(kit *k);

/* Drawing (all clip to 80x25; nothing scrolls). */
void kit_clear(kit *k);
void kit_clear_rows(kit *k, int top, int bottom);          /* inclusive */
void kit_put(kit *k, int row, int col, const char *s, int n); /* n<0: strlen */
void kit_put_attr(kit *k, int row, int col, uint8_t attr, const char *s, int n);
void kit_fill(kit *k, int row, int col, int n, char c, uint8_t attr);
void kit_box(kit *k, int top, int left, int bottom, int right, int dbl);
void kit_cursor(kit *k, int row, int col);
void kit_flush_row(kit *k, int row);   /* re-push one row to the PAL */
void kit_save(kit *k, kit_saved *s);
void kit_restore(kit *k, const kit_saved *s);

/* Status bar (row 22, reverse video) as III+:
 * " <mode> |<drive>:|<file> |Rec: n/m |  | Ins | Caps"  -- fields may be NULL. */
void kit_status(kit *k, const char *mode, const char *file, uint32_t rec, uint32_t count, int ins);
/* Navigation / message lines (rows 23 and 24, centred normal video). */
void kit_message(kit *k, int row, const char *msg);

/* Input. */
int  kit_key(kit *k);   /* blocking, decoded (see enum), KIT_K_EOF on EOF */

/* Field editor: edit `buf` (width `w`, space-padded, NOT NUL-required) at
 * (row,col) in reverse video. type: 'C','N','D','L'. picture may be NULL
 * (supports '9','#','A','X','!','Y','L' template chars and '@!' function).
 * Returns the key that ended the edit (ENTER, UP, DOWN, PGUP, PGDN, ESC,
 * SAVE, ABORT, ...); *changed set nonzero if the user typed. Typing past
 * the end returns KIT_K_ENTER (auto-advance, SET CONFIRM OFF). */
int  kit_edit_field(kit *k, int row, int col, char *buf, int w, char type,
                    const char *picture, int *changed);

/* Menu bar + pulldowns (ASSIST style). titles[n] on row 0; items[i] is a
 * NUL-separated, double-NUL-terminated list for pulldown i. Left/Right move
 * between pads, Up/Down within a pulldown, Enter selects, Esc returns -1.
 * Returns (pad << 8) | item on selection. */
int  kit_menu(kit *k, const char *const *titles, const char *const *items,
              int n, int start_pad, const char *const *help_lines);

/* Simple vertical pick list in a box at (top,left); returns index or -1. */
int  kit_pick(kit *k, int top, int left, const char *const *items, int n, int start);

/* One-line prompt on row `row`: shows `prompt`, edits `buf` of width `w`;
 * returns 1 on Enter, 0 on Esc. buf is NUL-terminated (trailing blanks trimmed). */
int  kit_prompt(kit *k, int row, const char *prompt, char *buf, int w);
#endif
