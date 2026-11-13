/*
 * console_text.c -- read the 80x25 InitechDOS console back out of a QEMU
 * screendump as plain text (factory, C-only).
 *
 * Purpose: the Initech Publications toolchain (bead initech-ipu5.5) prints
 * screen reproductions in the manuals.  They must be REAL OS output (Law 2,
 * DESIGN-SYSTEM Sec 5 "screen captures from the emulator"), not text an author
 * typed into the manual.  The console is drawn into a linear framebuffer, so
 * the only capture the harness yields is a pixel screendump (P6 PPM).  This
 * tool turns those pixels back into the characters that were drawn.
 *
 * How (no guessing, no OCR): the kernel draws every cell with the 8x16 VGA
 * ROM font that stage2 copies out of the video BIOS (INT 10h AX=1130h BH=06h,
 * os/boot/stage2.asm FONT_STASH).  QEMU's standard VGA loads its BIOS from the
 * host (SeaBIOS vgabios-stdvga.bin), so the very same 4096-byte font table is
 * in that file.  We locate it, then for each of the 80x25 cells (8x16 px,
 * origin top-left, os/milton/console.h CONSOLE_COLS/ROWS/CELL_W/CELL_H) we
 * build the cell's ink bitmap (pixel == console fg) and look it up EXACTLY in
 * the font.  Every pixel of the console area must be either the console fg or
 * the console bg (os/milton/console.c CONSOLE_FG_* / CONSOLE_BG_*), and every
 * cell must match a printable ASCII glyph or be blank; anything else is a
 * loud failure (Rule 2), never a '?' in a printed manual.
 *
 * Usage: console_text <vgabios.bin> <screendump.ppm>
 * Output (stdout): exactly 25 lines, trailing blanks trimmed, '\n' endings.
 * Exit: 0 ok; 1 decode failure (named cell); 2 usage / I/O / format error.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLS 80
#define ROWS 25
#define CW 8
#define CH 16
#define FONT_BYTES (256 * CH)

/* Mirrored from os/milton/console.c (CONSOLE_FG_* light grey, CONSOLE_BG_*
 * seafoam).  The screendump is 32bpp -> exact values, no tolerance needed. */
static const unsigned char FG[3] = {0xC0, 0xC0, 0xC0};
static const unsigned char BG[3] = {0x6F, 0xA0, 0x8E};

static unsigned char *slurp(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "console_text: cannot open %s\n", path); return NULL; }
    size_t cap = 1 << 20, n = 0;
    unsigned char *buf = malloc(cap);
    if (!buf) { fclose(f); return NULL; }
    for (;;) {
        if (n == cap) {
            unsigned char *nb = realloc(buf, cap * 2);
            if (!nb) { free(buf); fclose(f); return NULL; }
            buf = nb; cap *= 2;
        }
        size_t r = fread(buf + n, 1, cap - n, f);
        if (r == 0) break;
        n += r;
    }
    fclose(f);
    *len = n;
    return buf;
}

/* Find the 8x16 font table: glyph 00h and 20h blank, glyph DBh (full block)
 * solid, glyph 'A' not blank.  Exactly one offset must qualify. */
static const unsigned char *find_font(const unsigned char *rom, size_t len)
{
    const unsigned char *hit = NULL;
    int hits = 0;
    for (size_t o = 0; o + FONT_BYTES <= len; o++) {
        const unsigned char *f = rom + o;
        int ok = 1;
        for (int y = 0; y < CH && ok; y++) {
            if (f[0x00 * CH + y] != 0 || f[0x20 * CH + y] != 0) ok = 0;
            if (f[0xDB * CH + y] != 0xFF) ok = 0;
        }
        if (!ok) continue;
        int ink = 0;
        for (int y = 0; y < CH; y++) ink |= f['A' * CH + y];
        if (!ink) continue;
        hit = f;
        hits++;
    }
    if (hits != 1) {
        fprintf(stderr, "console_text: expected exactly one 8x16 font table in the VGA BIOS, found %d\n", hits);
        return NULL;
    }
    return hit;
}

static int parse_ppm(const unsigned char *b, size_t len, int *w, int *h, size_t *off)
{
    int maxv;
    int consumed = 0;
    char head[64];
    size_t n = len < sizeof(head) - 1 ? len : sizeof(head) - 1;
    memcpy(head, b, n);
    head[n] = '\0';
    if (sscanf(head, "P6 %d %d %d%n", w, h, &maxv, &consumed) != 3 || maxv != 255) {
        fprintf(stderr, "console_text: not a binary 8-bit PPM (P6, maxval 255)\n");
        return -1;
    }
    *off = (size_t)consumed + 1; /* single whitespace after maxval */
    if (*w < COLS * CW || *h < ROWS * CH ||
        *off + (size_t)(*w) * (size_t)(*h) * 3 > len) {
        fprintf(stderr, "console_text: screendump %dx%d is smaller than the 640x400 console\n", *w, *h);
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: console_text <vgabios.bin> <screendump.ppm>\n");
        return 2;
    }
    size_t rlen = 0, plen = 0;
    unsigned char *rom = slurp(argv[1], &rlen);
    unsigned char *ppm = slurp(argv[2], &plen);
    if (!rom || !ppm) return 2;
    const unsigned char *font = find_font(rom, rlen);
    if (!font) return 2;
    int w, h;
    size_t off;
    if (parse_ppm(ppm, plen, &w, &h, &off) != 0) return 2;
    const unsigned char *px = ppm + off;

    /* Below the console (y >= 400) the framebuffer must be plain background:
     * a different console geometry would otherwise decode as nonsense. */
    for (int y = ROWS * CH; y < h; y++)
        for (int x = 0; x < w; x++) {
            const unsigned char *p = px + ((size_t)y * (size_t)w + (size_t)x) * 3;
            if (memcmp(p, BG, 3) != 0) {
                fprintf(stderr, "console_text: FAIL non-background pixel below the 80x25 console at (%d,%d)\n", x, y);
                return 1;
            }
        }

    char lines[ROWS][COLS + 1];
    int cursor_seen = 0;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            unsigned char cell[CH];
            for (int y = 0; y < CH; y++) {
                unsigned char bits = 0;
                for (int x = 0; x < CW; x++) {
                    const unsigned char *p = px + ((size_t)(r * CH + y) * (size_t)w + (size_t)(c * CW + x)) * 3;
                    if (memcmp(p, FG, 3) == 0) bits |= (unsigned char)(0x80u >> x);
                    else if (memcmp(p, BG, 3) != 0) {
                        fprintf(stderr, "console_text: FAIL cell row %d col %d: pixel (%d,%d) is neither console fg nor bg\n",
                                r + 1, c + 1, c * CW + x, r * CH + y);
                        return 1;
                    }
                }
                cell[y] = bits;
            }
            int ch = -1;
            int blank = 1;
            for (int y = 0; y < CH; y++) if (cell[y]) blank = 0;
            if (blank) ch = ' ';
            for (int g = 0x21; ch < 0 && g <= 0x7E; g++)
                if (memcmp(cell, font + g * CH, CH) == 0) ch = g;
            /* The blinking underline cursor (os/milton/console.c
             * console_cursor_wait: fg ink on cell rows 14-15, all 8 px) may be
             * caught in either phase.  Accept it at most once and print the
             * character underneath, so a capture does not depend on the blink
             * phase at the instant of the screendump. */
            if (ch < 0 && cell[CH - 2] == 0xFF && cell[CH - 1] == 0xFF && !cursor_seen) {
                int under_blank = 1;
                for (int y = 0; y < CH - 2; y++) if (cell[y]) under_blank = 0;
                if (under_blank) ch = ' ';
                for (int g = 0x21; ch < 0 && g <= 0x7E; g++)
                    if (memcmp(cell, font + g * CH, CH - 2) == 0) ch = g;
                if (ch >= 0) cursor_seen = 1;
            }
            if (ch < 0) {
                fprintf(stderr, "console_text: FAIL cell row %d col %d matches no printable ASCII glyph of the VGA font\n",
                        r + 1, c + 1);
                return 1;
            }
            lines[r][c] = (char)ch;
        }
        int end = COLS;
        while (end > 0 && lines[r][end - 1] == ' ') end--;
        lines[r][end] = '\0';
    }
    for (int r = 0; r < ROWS; r++) printf("%s\n", lines[r]);
    free(rom);
    free(ppm);
    return 0;
}
