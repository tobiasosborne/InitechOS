
#ifndef INITECH_CONSOLE_PIXELS_H
#define INITECH_CONSOLE_PIXELS_H

#include <stdint.h>


typedef struct {
    volatile uint8_t *base;      
    uint32_t          pitch;     
    uint32_t          bpp;       
    uint32_t          bytes_per_pixel; 
    uint32_t          width;     
    uint32_t          height;    
} console_bitmap_t;


static inline uint32_t console_pixels_pack_rgb(uint32_t bpp,
                                        uint8_t r, uint8_t g, uint8_t b)
{
    (void)bpp;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}


static inline void console_pixels_put_pixel(const console_bitmap_t *bm,
                                     uint32_t off, uint32_t px)
{
    if (bm->bpp == 8) {
        bm->base[off] = (uint8_t)(px & 0xFFu);
    } else if (bm->bpp == 32) {
        *(volatile uint32_t *)(bm->base + off) = px;
    } else {
        
        bm->base[off + 0u] = (uint8_t)(px & 0xFFu);          
        bm->base[off + 1u] = (uint8_t)((px >> 8) & 0xFFu);   
        bm->base[off + 2u] = (uint8_t)((px >> 16) & 0xFFu);  
    }
}


static inline uint32_t console_pixels_get_pixel(const console_bitmap_t *bm, uint32_t off)
{
    if (bm->bpp == 8) {
        return (uint32_t)bm->base[off];
    } else if (bm->bpp == 32) {
        return *(volatile uint32_t *)(bm->base + off);
    } else {
        return ((uint32_t)bm->base[off + 2u] << 16) |
               ((uint32_t)bm->base[off + 1u] << 8)  |
               ((uint32_t)bm->base[off + 0u]);
    }
}


void console_pixels_fill_span(const console_bitmap_t *bm,
                       uint32_t x, uint32_t y,
                       uint32_t len, uint32_t px);


void console_pixels_clear(const console_bitmap_t *bm, uint32_t px);


void console_pixels_blit(const console_bitmap_t *bm,
                  uint32_t x0, uint32_t y0,
                  const uint8_t *glyph,
                  uint32_t cell_w, uint32_t cell_h,
                  uint32_t fg, uint32_t bg);

#endif 
