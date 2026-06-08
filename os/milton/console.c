/* console.c -- InitechDOS 80x25 LFB text console implementation.
 *
 * beads: initech-yqb. Ref: PRD Sec 5 (80x25 VGA 8x16 ROM font blitted into the
 *        LFB); docs/research/boot-to-text-ground-truth.md Sec 1.2 (glyph
 *        format: 1 byte/row, 16 rows, MSB=0x80=LEFTMOST pixel, set bit = ink) +
 *        Sec 5 Risk 2 (branch on lfb_bpp 32 vs 24; pixel addr = lfb +
 *        y*pitch + x*(bpp/8)). CLAUDE.md Law 2 (oracle is truth -- the blit
 *        math is host-tested), Rule 2 (fail loud), Rule 11 (reproducible,
 *        no timestamps), Rule 12 (ASCII source).
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

/* ---- low-level pixel write ----------------------------------------------- */
/* Write one packed pixel at byte offset `off` into the framebuffer. Branches on
 * bpp (Sec 5 Risk 2). For 24bpp we decompose the canonical 0x00RRGGBB into the
 * B,G,R byte triple. */
static inline void put_pixel(console_t *con, uint32_t off, console_color_t px)
{
    if (con->bpp == 32) {
        /* The dword is XRGB8888 (little-endian 0x00RRGGBB => bytes B,G,R,X). */
        *(volatile uint32_t *)(con->lfb + off) = px;
    } else {
        /* 24bpp: store B,G,R at increasing addresses (stage2 fill24). */
        con->lfb[off + 0u] = (uint8_t)(px & 0xFFu);          /* B */
        con->lfb[off + 1u] = (uint8_t)((px >> 8) & 0xFFu);   /* G */
        con->lfb[off + 2u] = (uint8_t)((px >> 16) & 0xFFu);  /* R */
    }
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

    for (uint32_t gy = 0; gy < CONSOLE_CELL_H; gy++) {
        uint8_t  bits = glyph[gy];
        uint32_t off  = (y0 + gy) * con->pitch + x0 * con->bytes_per_pixel;
        for (uint32_t gx = 0; gx < CONSOLE_CELL_W; gx++) {
            /* MSB = leftmost pixel: column 0 is bit 0x80, column 7 is 0x01
             * (Sec 1.2). A set bit is ink (fg); a clear bit is bg. */
            uint8_t mask = (uint8_t)(0x80u >> gx);
            console_color_t px = (bits & mask) ? fg : bg;
            put_pixel(con, off, px);
            off += con->bytes_per_pixel;
        }
    }
}

/* ---- clear --------------------------------------------------------------- */
void console_clear(console_t *con)
{
    if (con == 0 || con->lfb == 0) {
        return;
    }
    /* Fill every pixel of every scanline with bg. Honor pitch but only write
     * the `width` visible pixels per row (padding bytes are off-screen; leaving
     * them avoids relying on pitch==width*bpp/8). */
    for (uint32_t y = 0; y < con->height; y++) {
        uint32_t off = y * con->pitch;
        for (uint32_t x = 0; x < con->width; x++) {
            put_pixel(con, off, con->bg);
            off += con->bytes_per_pixel;
        }
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
    uint32_t text_h = con->rows * CONSOLE_CELL_H; /* 400 px */
    if (text_h > con->height) {
        text_h = con->height;
    }
    uint32_t move_rows = (text_h >= CONSOLE_CELL_H) ? (text_h - CONSOLE_CELL_H) : 0u;

    /* Copy scanline y+CONSOLE_CELL_H -> y for y in [0, move_rows). Per-pixel so
     * the same code path serves both bpp; src is always below dst so a forward
     * copy is safe (no overlap hazard for an upward move). */
    for (uint32_t y = 0; y < move_rows; y++) {
        uint32_t dst_off = y * con->pitch;
        uint32_t src_off = (y + CONSOLE_CELL_H) * con->pitch;
        for (uint32_t x = 0; x < con->width; x++) {
            console_color_t px;
            uint32_t so = src_off + x * con->bytes_per_pixel;
            if (con->bpp == 32) {
                px = *(volatile uint32_t *)(con->lfb + so);
            } else {
                px = ((console_color_t)con->lfb[so + 2u] << 16) |
                     ((console_color_t)con->lfb[so + 1u] << 8)  |
                     ((console_color_t)con->lfb[so + 0u]);
            }
            put_pixel(con, dst_off + x * con->bytes_per_pixel, px);
        }
    }

    /* Clear the freed last text row (the bottom CONSOLE_CELL_H scanlines of the
     * text band) to bg. */
    for (uint32_t y = move_rows; y < text_h; y++) {
        uint32_t off = y * con->pitch;
        for (uint32_t x = 0; x < con->width; x++) {
            put_pixel(con, off, con->bg);
            off += con->bytes_per_pixel;
        }
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

    /* Printable: blit at the cursor, then advance with wrap + scroll. */
    console_draw_glyph(con, con->cur_col, con->cur_row, (uint8_t)ch,
                       con->fg, con->bg);
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
    if (!(bi->lfb_bpp == 24 || bi->lfb_bpp == 32)) {
        return CONSOLE_ERR_BPP;
    }
    if (bi->font_addr == 0) {
        return CONSOLE_ERR_FONT;
    }

    con->lfb             = (volatile uint8_t *)(uintptr_t)bi->lfb_addr;
    con->pitch           = bi->lfb_pitch;
    con->bpp             = bi->lfb_bpp;
    con->width           = bi->lfb_width;
    con->height          = bi->lfb_height;
    con->bytes_per_pixel = bi->lfb_bpp / 8u;
    con->font            = (const uint8_t *)(uintptr_t)bi->font_addr;

    con->cols    = CONSOLE_COLS;
    con->rows    = CONSOLE_ROWS;
    con->cur_col = 0;
    con->cur_row = 0;

    con->fg = console_pack_rgb(con->bpp, CONSOLE_FG_R, CONSOLE_FG_G, CONSOLE_FG_B);
    con->bg = console_pack_rgb(con->bpp, CONSOLE_BG_R, CONSOLE_BG_G, CONSOLE_BG_B);

    console_clear(con);
    return CONSOLE_OK;
}
