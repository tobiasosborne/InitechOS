/* console.h -- InitechDOS 80x25 LFB text console (the VGA 8x16 ROM font path).
 *
 * beads: initech-yqb ("8x16 text console over the linear framebuffer").
 * Ref:   PRD Sec 5 ("Text: 80x25 rendered by blitting the VGA 8x16 ROM font
 *        into the LFB"); docs/research/boot-to-text-ground-truth.md Sec 1.2
 *        (glyph format: 256 glyphs x 16 bytes, 1 byte/row, MSB=0x80=leftmost
 *        pixel, set bit = ink) + Sec 5 Risk 2 (the blitter MUST branch on
 *        lfb_bpp -- 32 vs 24 -- never assume 32; Bochs/86Box may give 24);
 *        os/milton/boot_info.h (the handoff contract). CLAUDE.md Law 2 (oracle
 *        is truth), Rule 2 (fail loud), Rule 11 (reproducible), Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only, no
 * libc, no malloc. The SAME translation unit (console.c) compiles hosted for
 * the factory blit-math oracle (harness reuses test_assert.h) -- so this header
 * is hosted-clean too (stdint only). The console binds the framebuffer params
 * and font pointer from boot_info; all drawing goes through a single
 * bpp-branching glyph blit (the load-bearing pixel math).
 */
#ifndef INITECH_CONSOLE_H
#define INITECH_CONSOLE_H

#include <stdint.h>
#include "boot_info.h"

/* 80x25 cells, 8x16 px each => 640x400 px, fits the 640x480 LFB (Sec, brief). */
#define CONSOLE_COLS      80
#define CONSOLE_ROWS      25
#define CONSOLE_CELL_W    8
#define CONSOLE_CELL_H    16
#define CONSOLE_FONT_GLYPHS 256
#define CONSOLE_FONT_BYTES  (CONSOLE_FONT_GLYPHS * CONSOLE_CELL_H) /* 4096 */

/* Return codes (Rule 2 -- init refuses rather than scribbles on bad input). */
#define CONSOLE_OK         0
#define CONSOLE_ERR_NULL   1  /* NULL console or boot_info                 */
#define CONSOLE_ERR_ADDR   2  /* lfb_addr == 0 (no framebuffer)            */
#define CONSOLE_ERR_BPP    3  /* lfb_bpp not in {8, 24, 32}                */
#define CONSOLE_ERR_FONT   4  /* font_addr == 0 (no font stash)            */

/* 8bpp standard-VGA fallback (initech-6pj): when no VBE LFB is available (e.g.
 * on Bochs, where the BIOS offers no 640x480 linear mode), stage2 falls back to
 * standard VGA mode 0x13 (320x200x256, linear @ 0xA0000). In that mode each
 * framebuffer byte is a PALETTE INDEX, not packed RGB. The console writes these
 * two indices; the kernel programs the VGA DAC so index BG renders as seafoam
 * and index FG as the light-gray ink (the 24/32bpp paths pack RGB directly and
 * ignore these). Ref: docs/research + the Bochs ground-truth (bd memory
 * bochs-boot-solved-initech-6pj). */
#define CONSOLE_BG_IDX     1u   /* seafoam    (kernel programs DAC reg 1)   */
#define CONSOLE_FG_IDX     15u  /* light gray (kernel programs DAC reg 15)  */

/* A packed framebuffer pixel value. For 32bpp it is the XRGB8888 dword
 * (0x00RRGGBB); for 24bpp the low 24 bits hold 0xRRGGBB (stored B,G,R). The
 * pack helper produces the right value for the bound bpp; the blit writes it
 * per the bpp branch. */
typedef uint32_t console_color_t;

typedef struct {
    /* Framebuffer params, copied from boot_info at init (stable thereafter). */
    volatile uint8_t *lfb;      /* linear framebuffer base (lfb_addr)        */
    uint32_t  pitch;            /* bytes per scanline                        */
    uint32_t  bpp;              /* 8, 24 or 32 (validated at init)           */
    uint32_t  width;            /* pixels (640 LFB / 320 mode-0x13 fallback) */
    uint32_t  height;           /* pixels (480 LFB / 200 mode-0x13 fallback) */
    uint32_t  bytes_per_pixel;  /* bpp / 8 (1, 3 or 4)                        */

    const uint8_t *font;        /* 4096-byte ROM font (font_addr)            */

    uint32_t  cols;             /* CONSOLE_COLS                              */
    uint32_t  rows;             /* CONSOLE_ROWS                              */
    uint32_t  cur_col;          /* cursor column (0..cols-1)                 */
    uint32_t  cur_row;          /* cursor row    (0..rows-1)                 */

    console_color_t fg;         /* packed foreground (ink)                   */
    console_color_t bg;         /* packed background                         */
} console_t;

/* Pack an (r,g,b) triple into the framebuffer pixel format for the given bpp.
 * Branches on bpp exactly like the blit (Sec 5 Risk 2). For 24bpp the result
 * holds 0x00RRGGBB; the blit writes the B,G,R bytes from it. */
console_color_t console_pack_rgb(uint32_t bpp, uint8_t r, uint8_t g, uint8_t b);

/* Bind framebuffer + font from boot_info, set default colors, clear, home the
 * cursor. Returns CONSOLE_OK or a CONSOLE_ERR_* (Rule 2 -- refuse bad input,
 * do not scribble). On error the console is left zeroed (unusable). */
int console_init(console_t *con, const boot_info_t *bi);

/* Blit one 8x16 glyph at cell (col,row): ink (fg) where the font bit is set,
 * bg elsewhere. Branches on bpp. This is the load-bearing pixel math. Cells
 * outside the grid are ignored (no overdraw past the framebuffer). */
void console_draw_glyph(console_t *con, uint32_t col, uint32_t row,
                        uint8_t ch, console_color_t fg, console_color_t bg);

/* Fill the entire framebuffer with the current background color. */
void console_clear(console_t *con);

/* Scroll the text area up one row: rows 1..N-1 move to 0..N-2, last row clears
 * to bg. Framebuffer memory move; never reads past the framebuffer. */
void console_scroll(console_t *con);

/* Emit one character at the cursor. Handles printable glyphs (draw + advance),
 * '\n' (col=0,row++), '\r' (col=0), '\t' (advance to next 8-col tab stop),
 * '\b' (backspace one cell). Wraps at col==cols; scrolls at row==rows. */
void console_putc(console_t *con, char ch);

/* Emit a NUL-terminated string / an explicit-length buffer. */
void console_puts(console_t *con, const char *s);
void console_write(console_t *con, const char *buf, uint32_t len);

#endif /* INITECH_CONSOLE_H */
