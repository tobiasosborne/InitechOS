/* Ref: system7-decomp goldens/captures/s8_desktop_menubar.png (12:56 am,
 * cap rows5..13, right ink592); goldens/resources/NFNT_5478.bin (Chicago12).
 * The requested Chicago substitution follows PRD Sec 6.4. Expected glyphs
 * come from independent Apple reference bytes, not the shipped authored art.
 * C-only factory; paths are arguments; missing references fail closed. */
#include <stdio.h>
#include <string.h>
#include <png.h>
#include "menubar_clock_oracle.h"

static unsigned be16(const unsigned char *b, unsigned i)
{ return (unsigned)b[i] * 256u + b[i + 1]; }

int menubar_clock_golden(const char *capture, const char *nfnt,
                         uint32_t out[20][80])
{
    png_image im;
    unsigned char rgb[640 * 480 * 3];
    memset(&im, 0, sizeof im);
    im.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&im, capture)) {
        fprintf(stderr, "FAIL clock reference: missing/unreadable PNG %s\n", capture);
        return 1;
    }
    im.format = PNG_FORMAT_RGB;
    if (im.width != 640 || im.height != 480 ||
        !png_image_finish_read(&im, NULL, rgb, 0, NULL)) {
        png_image_free(&im);
        fprintf(stderr, "FAIL clock reference: malformed 640x480 PNG\n");
        return 1;
    }
    png_image_free(&im);
    /* Unobstructed column530 is the independent four-row-class bar crop.
     * Check the source clock is present at the independently measured edge. */
    int edge = 0;
    for (int y = 5; y <= 13; y++) {
        unsigned char *p = rgb + 3 * (y * 640 + 592);
        edge += p[0] == 0 && p[1] == 0 && p[2] == 0;
    }
    if (!edge) { fprintf(stderr, "FAIL clock reference: captured clock missing\n"); return 1; }
    for (int y = 0; y < 20; y++) {
        unsigned char *p = rgb + 3 * (y * 640 + 530);
        uint32_t color = (uint32_t)p[0] * 65536u + (uint32_t)p[1] * 256u + p[2];
        for (int x = 0; x < 80; x++) out[y][x] = color;
    }
    /* The independent original strike, shared with dialog-client oracles. */
    return oracle_chicago_text(nfnt,"12:56 am",&out[0][0],80,20,-1,2);
}

/* x==-1: the clock's right-aligned anchor. Otherwise x/y are the top-left
 * text cell. No artifact font tables, metrics or drawing calls are used. */
int oracle_chicago_text(const char *nfnt,const char *text,uint32_t *out,
                         int width,int height,int x,int y0)
{
    unsigned char font[4096];
    FILE *f = fopen(nfnt, "rb");
    if (!f) { fprintf(stderr, "FAIL clock reference: missing NFNT %s\n", nfnt); return 1; }
    size_t n = fread(font, 1, sizeof font, f);
    fclose(f);
    if (n != 3098 || be16(font, 2) != 0 || be16(font, 4) != 240 ||
        be16(font, 14) != 15 || be16(font, 24) != 70) {
        fprintf(stderr, "FAIL clock reference: not original Chicago12 NFNT5478\n");
        return 1;
    }
    unsigned loc = 2126, ow = 2612;
    const char *s = text;
    int advance = 0;
    for (const char *c = s; *c; c++) advance += font[ow + 2u * (unsigned char)*c + 1];
    /* Original 'm': advance12, bearing1, bitmap10 -> last ink end-2.
     * Clock's captured right ink592 thus gives the exclusive cell end594. */
    int pen = x==-1 ? 74-advance : x;
    for (; *s; s++) {
        unsigned c = (unsigned char)*s;
        unsigned start = be16(font, loc + 2u * c), end = be16(font, loc + 2u * c + 2);
        int bearing = (int)font[ow + 2u * c] + (int)(short)be16(font, 8);
        for (unsigned y = 0; y < 15; y++) for (unsigned k = start; k < end; k++) {
            int x = pen + bearing + (int)(k - start);
            if ((font[26 + y * 140 + k / 8] & (128u >> (k % 8))) && x >= 0 && x < width && (int)y+y0>=0 && (int)y+y0<height)
                out[((int)y+y0)*width+x] = 0;
        }
        pen += font[ow + 2u * c + 1];
    }
    return 0;
}
