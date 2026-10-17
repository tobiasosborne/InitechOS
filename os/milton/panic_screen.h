/* initech-s25. PRD Sec 5 (VGA 8x16), Appendix B (printer-error pastiche).
 * Lane brief: 2x centred message; 1x operator status; one solid field.
 * Pure layout seam, shared with the host oracle; no heap or Toolbox. */
#ifndef INITECH_PANIC_SCREEN_H
#define INITECH_PANIC_SCREEN_H
#include <stdint.h>
typedef struct { uint32_t x, y; } panic_origin_t;
static inline panic_origin_t panic_origin(uint32_t w, uint32_t h,
                                         const char *text, uint32_t scale,
                                         uint32_t line)
{
    uint32_t chars=0;
    while (text[chars]) chars++;
    uint32_t width=chars*8u*scale;
    panic_origin_t p = {w>=width ? (w-width)/2u : 0u,
                        h>=64u ? (h-64u)/2u+line*48u : 0u};
#ifdef PANIC_MUTATE_OFF_CENTRE
    p.x+=8u;
#endif
    return p;
}

/* Pixel-direct, including 24bpp BIOSes. Only black (0) and light gray (192)
 * are used. In indexed modes the caller programs DAC indices 0/15 first.
 * Ref: console.h's byte order; boot-to-text-ground-truth.md Sec 1.2 (ROM
 * glyph format). The lane brief chooses the two-line 1991 printer idiom;
 * PRD Appendix A's frame has no panic screen to extract a strike from.
 * No console state, heap, surface/Toolbox calls, IRQs, BIOS calls or scrolling.
 */
static inline void panic_pixel(volatile uint8_t *p, uint32_t bpp, int ink)
{
    uint8_t c=ink ? 192u : 0u;
    if (bpp==8u) { *p=ink ? 15u : 0u; return; }
    p[0]=c; p[1]=c; p[2]=c;
    if (bpp==32u) p[3]=0;
}
static inline void panic_screen_paint(volatile uint8_t *lfb, uint32_t pitch,
                                     uint32_t bpp, uint32_t w, uint32_t h,
                                     const uint8_t *font, uint32_t vector)
{
    uint32_t bytes=bpp/8u;
    if (!lfb || !font || w<224u || h<64u || vector>31u ||
        (bpp!=8u && bpp!=24u && bpp!=32u) || pitch/bytes<w) return;
#ifdef PANIC_MUTATE_NO_8BPP
    if (bpp==8u) return;
#endif
    for (uint32_t y=0;y<h;y++)
        for (uint32_t x=0;x<w;x++)
            panic_pixel(lfb+y*pitch+x*bytes,bpp,0);
    char status[]="ERROR 00 -- SEE OPERATOR";
    status[6]=(char)('0'+vector/10u);
    status[7]=(char)('0'+vector%10u);
    for (uint32_t line=0;line<2u;line++) {
        const char *text=line ? status : "PC LOAD LETTER";
        uint32_t scale=line ? 1u : 2u;
        panic_origin_t p=panic_origin(w,h,text,scale,line);
        for (uint32_t i=0;text[i];i++)
            for (uint32_t y=0;y<16u*scale;y++)
                for (uint32_t x=0;x<8u*scale;x++)
                    if (font[(uint8_t)text[i]*16u+y/scale] & (128u>>(x/scale)))
                        panic_pixel(lfb+(p.y+y)*pitch+
                                    (p.x+i*8u*scale+x)*bytes,bpp,1);
    }
}
#endif
