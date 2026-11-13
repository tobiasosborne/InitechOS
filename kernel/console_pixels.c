

#include "console_pixels.h"

void console_pixels_clear(const console_bitmap_t *bm, uint32_t px)
{
    for (uint32_t y = 0; y < bm->height; y++)
        console_pixels_fill_span(bm, 0, y, bm->width, px);
}



void console_pixels_fill_span(const console_bitmap_t *bm,
                       uint32_t x, uint32_t y,
                       uint32_t len, uint32_t px)
{
    if (bm == 0 || bm->base == 0) {
        return;
    }
    if (y >= bm->height || x >= bm->width) {
        return;
    }
    
    uint32_t avail = bm->width - x;
    if (len > avail) {
        len = avail;
    }
    uint32_t off = y * bm->pitch + x * bm->bytes_per_pixel;
    for (uint32_t i = 0; i < len; i++) {
        console_pixels_put_pixel(bm, off, px);
        off += bm->bytes_per_pixel;
    }
}



void console_pixels_blit(const console_bitmap_t *bm,
                  uint32_t x0, uint32_t y0,
                  const uint8_t *glyph,
                  uint32_t cell_w, uint32_t cell_h,
                  uint32_t fg, uint32_t bg)
{
    if (bm == 0 || bm->base == 0 || glyph == 0) {
        return;
    }
    
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
            
            uint8_t mask = (uint8_t)(0x80u >> gx);
            uint32_t color = (bits & mask) ? fg : bg;
            console_pixels_put_pixel(bm, off, color);
            off += bm->bytes_per_pixel;
        }
    }
}
