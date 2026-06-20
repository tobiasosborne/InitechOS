/* console.c -- InitechDOS 80x25 LFB text console implementation.
 *
 * beads: initech-yqb. Ref: PRD Sec 5 (80x25 VGA 8x16 ROM font blitted into the
 *        LFB); docs/research/boot-to-text-ground-truth.md Sec 1.2 (glyph
 *        format: 1 byte/row, 16 rows, MSB=0x80=LEFTMOST pixel, set bit = ink) +
 *        Sec 5 Risk 2 (branch on lfb_bpp 32 vs 24; pixel addr = lfb +
 *        y*pitch + x*(bpp/8)). CLAUDE.md Law 2 (oracle is truth -- the blit
 *        math is host-tested), Rule 2 (fail loud), Rule 11 (reproducible,
 *        no timestamps), Rule 12 (ASCII source).
 * beads: initech-k8o5. Ref: ADR-0004 D-2 (console is a CLIENT of the one
 *        surface module; no second pixel writer). The pixel path is now owned
 *        by os/flair/surface.{c,h}; this file binds a bitmap_t over its LFB
 *        params and delegates all pixel writes to the surface primitives.
 *        The public API in console.h is BYTE-FOR-BYTE IDENTICAL to before;
 *        kmain.c and every other consumer is untouched.
 *
 * ARTIFACT code: freestanding, <stdint.h> only, no libc, no malloc. Compiles
 * BOTH under the kernel flags (gcc -m32 -ffreestanding -nostdlib -std=c11
 * -Wall -Wextra -Werror) AND hosted for the factory blit oracle (test_console.c
 * casts a heap framebuffer's address into a fake boot_info). No host headers
 * here -- the only dependency is <stdint.h>, which is freestanding-provided.
 *
 * Pixel format (Sec 5 Risk 2, mirrored from os/milton/kmain.c fb_clear and
 * stage2.asm fill32/fill24):
 *   32bpp -- one dword per pixel, XRGB8888 == little-endian 0x00RRGGBB, so the
 *            bytes at increasing addresses are B,G,R,X.
 *   24bpp -- three bytes per pixel: B,G,R (no padding within the pixel; the
 *            scanline may still be padded -- honor pitch, not width*bpp/8).
 */

#include "console.h"
#include "../flair/surface.h"

/* ---- color packing -------------------------------------------------------
 * Pack (r,g,b) for the bound bpp. Both formats place 0x00RRGGBB into the dword
 * (the 32bpp write stores it directly little-endian => B,G,R,X; the 24bpp write
 * stores the low three bytes B,G,R explicitly). One canonical representation
 * (0x00RRGGBB) keeps the host oracle's expectations simple. */
console_color_t console_pack_rgb(uint32_t bpp, uint8_t r, uint8_t g, uint8_t b)
{
    (void)bpp; /* representation is identical; the BLIT branches on bpp. */
    return ((console_color_t)r << 16) |
           ((console_color_t)g << 8)  |
           ((console_color_t)b);
}

/* Default console palette (documented choice):
 *   fg = light gray (period DOS console text), bg = SEAFOAM.
 * We background on SEAFOAM (NOT black) deliberately: the test-tracer-boot
 * screendump oracle (tools/ppm_seafoam_check.c) samples an 81-point grid and
 * asserts seafoam, so a full-screen clear to any other color would break that
 * gate (CLAUDE.md Law 2 -- do not weaken an oracle to ship). Keeping bg=seafoam
 * means the screen stays predominantly seafoam with text inked on top; the
 * banner task (initech-bea) may revisit this once the gate is updated.
 * SEAFOAM mirrors kmain.c SEAFOAM_R/G/B / stage2.asm. */
#define CONSOLE_FG_R 0xC0
#define CONSOLE_FG_G 0xC0
#define CONSOLE_FG_B 0xC0
#define CONSOLE_BG_R 0x6F   /* SEAFOAM_R */
#define CONSOLE_BG_G 0xA0   /* SEAFOAM_G */
#define CONSOLE_BG_B 0x8E   /* SEAFOAM_B */

#define CONSOLE_TAB_WIDTH 8

/* ---- bitmap_t helper ----------------------------------------------------- */
/* Bind a bitmap_t from a console_t. Used internally so all surface calls
 * receive the same descriptor (ADR-0004 D-2: one surface, no duplication). */
static bitmap_t con_bitmap(const console_t *con)
{
    bitmap_t bm;
    bm.base            = con->lfb;
    bm.pitch           = con->pitch;
    bm.bpp             = con->bpp;
    bm.bytes_per_pixel = con->bytes_per_pixel;
    bm.width           = con->width;
    bm.height          = con->height;
    return bm;
}

/* ---- glyph blit (the load-bearing math) ---------------------------------- */
void console_draw_glyph(console_t *con, uint32_t col, uint32_t row,
                        uint8_t ch, console_color_t fg, console_color_t bg)
{
    if (con == 0 || con->lfb == 0 || con->font == 0) {
        return;
    }
    /* Bounds: a cell outside the grid is a no-op (Rule 2 -- never write past
     * the framebuffer; the classic overdraw bug). */
    if (col >= con->cols || row >= con->rows) {
        return;
    }

    const uint8_t *glyph = con->font + (uint32_t)ch * CONSOLE_CELL_H;
    uint32_t x0 = col * CONSOLE_CELL_W;
    uint32_t y0 = row * CONSOLE_CELL_H;

    bitmap_t bm = con_bitmap(con);
    /* Delegate to the one surface blit primitive (ADR-0004 D-2). */
    surface_blit(&bm, x0, y0, glyph, CONSOLE_CELL_W, CONSOLE_CELL_H, fg, bg);
}

/* ---- clear --------------------------------------------------------------- */
void console_clear(console_t *con)
{
    if (con == 0 || con->lfb == 0) {
        return;
    }
    bitmap_t bm = con_bitmap(con);
    /* Fill every pixel of every scanline with bg. Honor pitch by using
     * surface_fill_span which takes (x,y,len) and strides by pitch. Only
     * write the `width` visible pixels per row (padding bytes are off-screen;
     * leaving them avoids relying on pitch==width*bpp/8). */
    for (uint32_t y = 0; y < con->height; y++) {
        surface_fill_span(&bm, 0u, y, con->width, con->bg);
    }
}

/* ---- scroll -------------------------------------------------------------- */
/* Scroll the 80x25 text area up by one cell row (16 px). Rows 1..N-1 of pixels
 * within the text band move up CONSOLE_CELL_H scanlines; the freed last text
 * row is cleared to bg. We move full scanlines (width pixels), reading only
 * within the framebuffer (Rule 2 -- no read past the LFB). */
void console_scroll(console_t *con)
{
    if (con == 0 || con->lfb == 0) {
        return;
    }
    bitmap_t bm = con_bitmap(con);
    uint32_t text_h = con->rows * CONSOLE_CELL_H; /* 400 px */
    if (text_h > con->height) {
        text_h = con->height;
    }
    uint32_t move_rows = (text_h >= CONSOLE_CELL_H) ? (text_h - CONSOLE_CELL_H) : 0u;

    /* Copy scanline y+CONSOLE_CELL_H -> y for y in [0, move_rows). Per-pixel so
     * the same code path serves both bpp; src is always below dst so a forward
     * copy is safe (no overlap hazard for an upward move). */
    for (uint32_t y = 0; y < move_rows; y++) {
        uint32_t dst_off = y * bm.pitch;
        uint32_t src_off = (y + CONSOLE_CELL_H) * bm.pitch;
        for (uint32_t x = 0; x < bm.width; x++) {
            uint32_t so = src_off + x * bm.bytes_per_pixel;
            uint32_t px = surface_get_pixel(&bm, so);
            surface_put_pixel(&bm, dst_off + x * bm.bytes_per_pixel, px);
        }
    }

    /* Clear the freed last text row (the bottom CONSOLE_CELL_H scanlines of the
     * text band) to bg. */
    for (uint32_t y = move_rows; y < text_h; y++) {
        surface_fill_span(&bm, 0u, y, bm.width, con->bg);
    }
}

/* ---- cursor advance + put ------------------------------------------------ */
static void advance_line(console_t *con)
{
    con->cur_col = 0;
    if (con->cur_row + 1 >= con->rows) {
        console_scroll(con);
        con->cur_row = con->rows - 1; /* stay on the last row after scroll */
    } else {
        con->cur_row++;
    }
}

void console_putc(console_t *con, char ch)
{
    if (con == 0 || con->lfb == 0 || con->font == 0) {
        return;
    }
    switch (ch) {
    case '\n':
        advance_line(con);
        return;
    case '\r':
        con->cur_col = 0;
        return;
    case '\t': {
        /* Advance to the next tab stop; wrap/scroll if it runs off the row. */
        uint32_t next = (con->cur_col / CONSOLE_TAB_WIDTH + 1) * CONSOLE_TAB_WIDTH;
        if (next >= con->cols) {
            advance_line(con);
        } else {
            con->cur_col = next;
        }
        return;
    }
    case '\b':
        if (con->cur_col > 0) {
            con->cur_col--;
        }
        return;
    default:
        break;
    }

    /* Printable: blit at the cursor with the CURRENT attribute (beads
     * initech-p96i), then advance with wrap + scroll. cur_fg/cur_bg default to
     * fg/bg (set at console_init), so with no ANSI SGR active this is byte-for-
     * byte identical to drawing with fg/bg (CLAUDE.md Law 4). */
    console_draw_glyph(con, con->cur_col, con->cur_row, (uint8_t)ch,
                       con->cur_fg, con->cur_bg);
    con->cur_col++;
    if (con->cur_col >= con->cols) {
        advance_line(con);
    }
}

void console_write(console_t *con, const char *buf, uint32_t len)
{
    if (con == 0 || buf == 0) {
        return;
    }
    for (uint32_t i = 0; i < len; i++) {
        console_putc(con, buf[i]);
    }
}

void console_puts(console_t *con, const char *s)
{
    if (con == 0 || s == 0) {
        return;
    }
    while (*s) {
        console_putc(con, *s++);
    }
}

/* ---- init ---------------------------------------------------------------- */
int console_init(console_t *con, const boot_info_t *bi)
{
    if (con == 0 || bi == 0) {
        return CONSOLE_ERR_NULL;
    }
    /* Zero the struct so a rejected init leaves it unusable (no stale state). */
    {
        uint8_t *p = (uint8_t *)con;
        for (uint32_t i = 0; i < (uint32_t)sizeof(*con); i++) {
            p[i] = 0;
        }
    }

    if (bi->lfb_addr == 0) {
        return CONSOLE_ERR_ADDR;
    }
    if (!(bi->lfb_bpp == 8 || bi->lfb_bpp == 24 || bi->lfb_bpp == 32)) {
        return CONSOLE_ERR_BPP;
    }
    if (bi->font_addr == 0) {
        return CONSOLE_ERR_FONT;
    }
    /* Reject degenerate geometry (bcg.11; console.h promise "refuse bad input,
     * do not scribble"). A zero width/height yields a console that reports OK
     * but draws nothing; a pitch shorter than one row of pixels
     * (width * bytes_per_pixel) makes every per-row write overrun into the next
     * scanline, and pitch==0 aliases every scanline to row 0 -> silent
     * framebuffer corruption. bpp is already validated to {8,24,32}, so bpp/8
     * is 1/3/4. The struct was zeroed, so a rejected init stays unusable. */
    if (bi->lfb_width == 0 || bi->lfb_height == 0 ||
        bi->lfb_pitch < bi->lfb_width * (bi->lfb_bpp / 8u)) {
        return CONSOLE_ERR_GEOMETRY;
    }

    con->lfb             = (volatile uint8_t *)(uintptr_t)bi->lfb_addr;
    con->pitch           = bi->lfb_pitch;
    con->bpp             = bi->lfb_bpp;
    con->width           = bi->lfb_width;
    con->height          = bi->lfb_height;
    con->bytes_per_pixel = bi->lfb_bpp / 8u;   /* 8->1, 24->3, 32->4 */
    con->font            = (const uint8_t *)(uintptr_t)bi->font_addr;

    /* Cell grid sized to the framebuffer, capped at the canonical 80x25. The
     * 640x480 LFB yields exactly 80x25 (480/16=30 capped to 25, the 400px text
     * band) -- unchanged from before. The 320x200 mode-0x13 fallback yields
     * 40x12, which the cap leaves untouched. Deriving (instead of hardcoding
     * 80x25) keeps the blit from ever addressing past a smaller framebuffer
     * (Rule 2 -- no overdraw). */
    con->cols = bi->lfb_width  / CONSOLE_CELL_W;
    con->rows = bi->lfb_height / CONSOLE_CELL_H;
    if (con->cols > CONSOLE_COLS) con->cols = CONSOLE_COLS;
    if (con->rows > CONSOLE_ROWS) con->rows = CONSOLE_ROWS;
    con->cur_col = 0;
    con->cur_row = 0;

    if (con->bpp == 8) {
        /* Mode 0x13: fg/bg are palette indices; the kernel programs the DAC so
         * CONSOLE_BG_IDX renders seafoam and CONSOLE_FG_IDX renders light gray. */
        con->fg = CONSOLE_FG_IDX;
        con->bg = CONSOLE_BG_IDX;
    } else {
        con->fg = console_pack_rgb(con->bpp, CONSOLE_FG_R, CONSOLE_FG_G, CONSOLE_FG_B);
        con->bg = console_pack_rgb(con->bpp, CONSOLE_BG_R, CONSOLE_BG_G, CONSOLE_BG_B);
    }

    /* CURRENT attribute starts EQUAL to the default ink/paper (beads
     * initech-p96i). console_putc draws with cur_fg/cur_bg; initialising them to
     * fg/bg keeps the no-ANSI / no-SGR rendering pixel-identical to before this
     * field existed (CLAUDE.md Law 4). */
    con->cur_fg = con->fg;
    con->cur_bg = con->bg;

    console_clear(con);
    return CONSOLE_OK;
}

/* ===========================================================================
 * ANSI.SYS support (beads initech-p96i -- wiring the ANSI FSM into CON).
 *
 * Ref (Law 1): IBM PC Technical Reference (1984) Appendix B "Display Adapter"
 *   -- the 16-entry CGA/EGA text palette. Each attribute nibble (0..15) selects
 *   an (R,G,B) triple. The low 8 entries use 0xAA on each set channel (brown,
 *   index 6, is the special case AA5500 -- green halved); the high 8 are the
 *   "bright" variants with 0x55 added (so 0x55 floor, 0xFF on set channels),
 *   and bright black (8) is dark grey 0x555555. This is the canonical IBM
 *   palette every period DOS ANSI.SYS screen renders against.
 * =========================================================================*/

/* CGA 16-colour palette as (R,G,B) bytes, indexed by the 4-bit attribute
 * nibble (fg = attr bits 3:0, bg = attr bits 7:4 when blink is reused as the
 * bright-bg bit). Ref: IBM PC Technical Reference App B Table "Color Codes". */
static const uint8_t CGA_PALETTE[16][3] = {
    { 0x00, 0x00, 0x00 },  /*  0 black            */
    { 0x00, 0x00, 0xAA },  /*  1 blue             */
    { 0x00, 0xAA, 0x00 },  /*  2 green            */
    { 0x00, 0xAA, 0xAA },  /*  3 cyan             */
    { 0xAA, 0x00, 0x00 },  /*  4 red              */
    { 0xAA, 0x00, 0xAA },  /*  5 magenta          */
    { 0xAA, 0x55, 0x00 },  /*  6 brown/yellow     */
    { 0xAA, 0xAA, 0xAA },  /*  7 light grey       */
    { 0x55, 0x55, 0x55 },  /*  8 dark grey        */
    { 0x55, 0x55, 0xFF },  /*  9 bright blue      */
    { 0x55, 0xFF, 0x55 },  /* 10 bright green     */
    { 0x55, 0xFF, 0xFF },  /* 11 bright cyan      */
    { 0xFF, 0x55, 0x55 },  /* 12 bright red       */
    { 0xFF, 0x55, 0xFF },  /* 13 bright magenta   */
    { 0xFF, 0xFF, 0x55 },  /* 14 bright yellow    */
    { 0xFF, 0xFF, 0xFF }   /* 15 bright white     */
};

void console_set_attr(console_t *con, uint8_t cga_attr)
{
    if (con == 0) {
        return;
    }
    /* CGA attribute byte (IBM PC Tech Ref App B):
     *   bit  3:0  foreground colour nibble (0..7) + bit 3 = bright -> 0..15
     *   bit  6:4  background colour nibble (0..7)
     *   bit  7    blink / bright-background.
     * The ANSI default 0x07 (light grey on black) therefore yields cur_fg ==
     * the light-grey palette entry and cur_bg == black. */
    uint8_t fg_idx = (uint8_t)(cga_attr & 0x0Fu);          /* 0..15 (incl bright) */
    uint8_t bg_idx = (uint8_t)((cga_attr >> 4) & 0x07u);   /* 0..7 (no bright bg) */

    /* PIXEL-IDENTITY GUARD (CLAUDE.md Law 4): the ANSI default / reset attribute
     * 0x07 (light grey on black) maps back to the CONSOLE'S OWN default ink/paper
     * -- which is light-gray on SEAFOAM, NOT CGA black. The initial FSM state and
     * every ESC[0m / ESC[m therefore leave the project's seafoam background
     * untouched (the test-tracer-boot seafoam screendump oracle stays green), and
     * only an EXPLICIT colour SGR (ESC[31m, ESC[44m, ...) yields a CGA colour.
     * Without this, an ESC[0m would repaint the background black and break the
     * seafoam gate. Ref: ppm_seafoam_check.c (the seafoam screendump oracle);
     * console.h cur_fg/cur_bg contract. */
    if (cga_attr == 0x07u) {
        con->cur_fg = con->fg;
        con->cur_bg = con->bg;
        return;
    }

    if (con->bpp == 8) {
        /* Mode 0x13: only CONSOLE_FG_IDX / CONSOLE_BG_IDX are programmed in the
         * DAC. We cannot render arbitrary CGA colours without reprogramming 16
         * DAC entries (a deferred follow-up that touches the VGA DAC). Fall back
         * to the default ink/paper so 8bpp output never corrupts; a default-
         * attribute (0x07) still maps to the default pair exactly. */
        con->cur_fg = con->fg;
        con->cur_bg = con->bg;
        return;
    }

    con->cur_fg = console_pack_rgb(con->bpp, CGA_PALETTE[fg_idx][0],
                                   CGA_PALETTE[fg_idx][1], CGA_PALETTE[fg_idx][2]);
    con->cur_bg = console_pack_rgb(con->bpp, CGA_PALETTE[bg_idx][0],
                                   CGA_PALETTE[bg_idx][1], CGA_PALETTE[bg_idx][2]);
}

void console_set_cursor(console_t *con, uint32_t row, uint32_t col)
{
    if (con == 0) {
        return;
    }
    /* Clamp to the grid (Rule 2 -- never let the cursor address past the cells;
     * a subsequent draw is also bounds-checked, but clamping keeps cur_row/col a
     * valid coordinate for the next advance). cols/rows are >= 1 (console_init). */
    if (con->cols == 0u || con->rows == 0u) {
        return;
    }
    con->cur_col = (col >= con->cols) ? (con->cols - 1u) : col;
    con->cur_row = (row >= con->rows) ? (con->rows - 1u) : row;
}

void console_get_cursor(const console_t *con, uint32_t *row, uint32_t *col)
{
    if (con == 0) {
        if (row) *row = 0u;
        if (col) *col = 0u;
        return;
    }
    if (row) *row = con->cur_row;
    if (col) *col = con->cur_col;
}

/* Fill a run of whole CELLS on one text row with the current paper (cur_bg).
 * (col0..col1) inclusive, clamped. Used by the erase ops. Each cell is the
 * CONSOLE_CELL_W x CONSOLE_CELL_H pixel block; we fill its scanlines via the
 * surface span primitive (honours pitch). */
static void console_fill_cells(console_t *con, uint32_t row,
                               uint32_t col0, uint32_t col1)
{
    if (col1 >= con->cols) col1 = con->cols - 1u;
    if (col0 > col1) {
        return;
    }
    bitmap_t bm = con_bitmap(con);
    uint32_t x0 = col0 * CONSOLE_CELL_W;
    uint32_t span = (col1 - col0 + 1u) * CONSOLE_CELL_W;
    uint32_t y0 = row * CONSOLE_CELL_H;
    for (uint32_t dy = 0; dy < CONSOLE_CELL_H; dy++) {
        uint32_t y = y0 + dy;
        if (y >= con->height) {
            break;
        }
        surface_fill_span(&bm, x0, y, span, con->cur_bg);
    }
}

void console_erase_line(console_t *con, int mode)
{
    if (con == 0 || con->lfb == 0) {
        return;
    }
    uint32_t row = con->cur_row;
    uint32_t cur = con->cur_col;
    uint32_t last = (con->cols > 0u) ? (con->cols - 1u) : 0u;
    /* Ref: MS-DOS 3.3 Tech Ref Ch 4 "EL". 0 = cursor..end, 1 = start..cursor,
     * 2 = whole line. The cursor does NOT move. */
    if (mode == 1) {
        console_fill_cells(con, row, 0u, cur);
    } else if (mode == 2) {
        console_fill_cells(con, row, 0u, last);
    } else { /* mode 0 (and any unknown -> default to 0, like DOS) */
        console_fill_cells(con, row, cur, last);
    }
}

void console_erase_display(console_t *con, int mode)
{
    if (con == 0 || con->lfb == 0) {
        return;
    }
    uint32_t row = con->cur_row;
    uint32_t last_row = (con->rows > 0u) ? (con->rows - 1u) : 0u;
    uint32_t last_col = (con->cols > 0u) ? (con->cols - 1u) : 0u;
    /* Ref: MS-DOS 3.3 Tech Ref Ch 4 "ED". 0 = cursor..end-of-screen, 1 =
     * start-of-screen..cursor, 2 = whole screen. Cursor does NOT move. Erase is
     * with the CURRENT paper (cur_bg) -- NOT necessarily the default bg. */
    if (mode == 2) {
        for (uint32_t r = 0; r < con->rows; r++) {
            console_fill_cells(con, r, 0u, last_col);
        }
    } else if (mode == 1) {
        /* start-of-screen .. cursor (inclusive). Full rows above, partial line. */
        for (uint32_t r = 0; r < row; r++) {
            console_fill_cells(con, r, 0u, last_col);
        }
        console_fill_cells(con, row, 0u, con->cur_col);
    } else { /* mode 0 (and any unknown) */
        /* cursor .. end-of-screen. Partial current line, full rows below. */
        console_fill_cells(con, row, con->cur_col, last_col);
        for (uint32_t r = row + 1u; r <= last_row && con->rows > 0u; r++) {
            console_fill_cells(con, r, 0u, last_col);
        }
    }
}
