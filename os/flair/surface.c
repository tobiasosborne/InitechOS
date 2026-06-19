/* os/flair/surface.c -- the ONE pixel-write module for InitechOS (FLAIR Layer 1).
 *
 * beads: initech-k8o5. Ref: ADR-0004 D-2 ("exactly one surface module; no second
 *        pixel writer"); ADR-0004 D-2 OD-2 (indexed-8 offscreen); ADR-0004 FO-1
 *        (fb-agree gate GREEN + mutation-proven before any Manager code).
 * Ref:   PRD Sec 5 (hardware contract: VBE 2.0 LFB, 8/24/32bpp);
 *        docs/research/boot-to-text-ground-truth.md Sec 1.2 (glyph format:
 *        MSB=0x80=leftmost pixel, set bit = ink) + Sec 5 Risk 2 (bpp branch;
 *        pixel addr = base + y*pitch + x*(bpp/8); 32bpp XRGB8888 LE; 24bpp BGR).
 *        CLAUDE.md Law 1 (ground truth before code), Law 2 (oracle is truth),
 *        Law 3 (freestanding; dual-compile for oracle), Rule 2 (fail loud -- no
 *        write past the buffer), Rule 11 (reproducible), Rule 12 (ASCII-clean).
 *
 * ARTIFACT code: freestanding, <stdint.h> only, no libc, no malloc. Compiles BOTH
 * under kernel flags (gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall -Wextra
 * -Werror) AND hosted for the factory fb-agree oracle.
 *
 * ONE-SURFACE INVARIANT (ADR-0004 D-2): surface_put_pixel (inline in surface.h)
 * is the only site that writes pixels. surface_fill_span and surface_blit call
 * through it. console.c is a CLIENT; no second pixel writer exists.
 */

#include "surface.h"

/* ---- surface_fill_span --------------------------------------------------- */
/* Horizontal run of `len` pixels starting at (x,y). Clips to bitmap width so
 * no write exceeds pitch*height (Rule 2 -- no overdraw). */
void surface_fill_span(const bitmap_t *bm,
                       uint32_t x, uint32_t y,
                       uint32_t len, uint32_t px)
{
    if (bm == 0 || bm->base == 0) {
        return;
    }
    if (y >= bm->height || x >= bm->width) {
        return;
    }
    /* Clip length to remaining columns. */
    uint32_t avail = bm->width - x;
    if (len > avail) {
        len = avail;
    }
    uint32_t off = y * bm->pitch + x * bm->bytes_per_pixel;
    for (uint32_t i = 0; i < len; i++) {
        surface_put_pixel(bm, off, px);
        off += bm->bytes_per_pixel;
    }
}

/* ---- surface_blit -------------------------------------------------------- */
/* Blit one 8-pixel-wide font glyph at pixel origin (x0, y0). The glyph data
 * is cell_h bytes; each byte is a row bitmap (MSB=0x80=leftmost pixel, set=ink).
 * Ink pixels get fg; background pixels get bg. Bounds-checked: cells clipped to
 * the bitmap boundary are silently truncated (Rule 2 -- never write past buffer).
 *
 * This is the load-bearing math lifted from console_draw_glyph. The inner loop
 * is structurally identical; the fb-agree oracle proves bit-exact agreement
 * between this primitive and the console path (ADR-0004 AM-2). */
void surface_blit(const bitmap_t *bm,
                  uint32_t x0, uint32_t y0,
                  const uint8_t *glyph,
                  uint32_t cell_w, uint32_t cell_h,
                  uint32_t fg, uint32_t bg)
{
    if (bm == 0 || bm->base == 0 || glyph == 0) {
        return;
    }
    /* Clip rows and columns to the bitmap. */
    uint32_t rows = cell_h;
    uint32_t cols = cell_w;
    if (y0 >= bm->height) return;
    if (x0 >= bm->width)  return;
    if (rows > bm->height - y0) rows = bm->height - y0;
    if (cols > bm->width  - x0) cols = bm->width  - x0;

    for (uint32_t gy = 0; gy < rows; gy++) {
        uint8_t  bits = glyph[gy];
        uint32_t off  = (y0 + gy) * bm->pitch + x0 * bm->bytes_per_pixel;
        for (uint32_t gx = 0; gx < cols; gx++) {
            /* MSB = leftmost pixel: column 0 is bit 0x80, column 7 is 0x01
             * (boot-to-text-ground-truth.md Sec 1.2). Set bit = ink (fg). */
            uint8_t mask = (uint8_t)(0x80u >> gx);
            uint32_t color = (bits & mask) ? fg : bg;
            surface_put_pixel(bm, off, color);
            off += bm->bytes_per_pixel;
        }
    }
}
