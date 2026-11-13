/* ppm_grey_png.c -- the PIXEL ROUTE of a FLAIR screen figure (PUB-REF-0001
 * NT-21 (b), NT-22; bead initech-ipu5.10, the InitechWord manuals).
 *
 * Reads a QEMU screendump (binary PPM, P6, maxval 255), crops the rectangle
 * x0,y0 .. x1,y1 (exclusive), converts every pixel to grey through a FIXED
 * palette-to-grey table and writes an 8-bit greyscale PNG at 1:1 -- one
 * screen pixel, one image pixel; the book places it at an integer scale on a
 * 600 dpi grid (3 x 3: 0.127 mm a pixel) with interpolation off, so nothing
 * is resampled or smoothed.
 *
 * THE TABLE. NT-22 names spec/assets/clut.json (the System 7 / Mac OS 8
 * 8BitStd device CLUT). The shipped FLAIR desktop also paints the locked
 * Win-3.1 peer accents of spec/assets/color_canon.json (caption navy
 * #000080, button face #C0C0C0 and the Platinum greys; ADR-0004-AMENDMENT-
 * DEC-09 D-9b), so the table is the UNION of the colours of both locked
 * files [house ruling, stated in WL-0135: the capture is the truth and every
 * colour it holds is locked spec data]. Grey = round(0.299 R + 0.587 G +
 * 0.114 B) (Rec. 601 luma). A pixel whose colour is in neither file is RED
 * (Rule 2): an unlocked colour reaching a printed figure is a defect, never
 * guessed at.
 *
 * PNG: one IHDR (greyscale, depth 8), one IDAT, IEND; no time or text
 * chunk. Every row but the first carries PNG filter Up (a row equal to the
 * one above becomes zeros); the zlib stream is ONE deflate block with the
 * FIXED Huffman codes, each byte a literal or, after a run of 3 or more equal
 * bytes, a match of distance 1 (no compression library; deterministic, Rule
 * 11). The figures live in fixtures/pubs/ (binary; pubs/ is ASCII only,
 * GC-05).
 *
 * usage: ppm_grey_png <clut.json> <color_canon.json> <in.ppm> x0 y0 x1 y1 <out.png>
 * Host C only (Law 3). ASCII-clean (Rule 12).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static void die(const char *what, const char *detail)
{
    fprintf(stderr, "!!! test-pubs RED FLAIR-PIXELS: %s: %s\n", what, detail);
    exit(1);
}

static unsigned char *slurp(const char *path, long *n)
{
    FILE *f = fopen(path, "rb");
    unsigned char *b;
    if (!f) die(path, "cannot open");
    if (fseek(f, 0, SEEK_END) || (*n = ftell(f)) < 0) die(path, "size");
    rewind(f);
    b = malloc((size_t)*n + 1);
    if (!b || fread(b, 1, (size_t)*n, f) != (size_t)*n) die(path, "read");
    b[*n] = 0;
    fclose(f);
    return b;
}

/* Every "#RRGGBB" in a locked colour file is one table colour. */
static uint32_t g_rgb[2048];
static int g_n;
static int hexv(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
static void load_colours(const char *path)
{
    long n, i;
    unsigned char *s = slurp(path, &n);
    int before = g_n;
    for (i = 0; i + 7 <= n; i++) {
        uint32_t v = 0;
        int k, ok = s[i] == '#';
        for (k = 1; ok && k <= 6; k++) { int h = hexv(s[i + k]); if (h < 0) ok = 0; else v = v << 4 | (uint32_t)h; }
        if (!ok || (i + 7 < n && hexv(s[i + 7]) >= 0)) continue;
        for (k = 0; k < g_n && g_rgb[k] != v; k++) ;
        if (k == g_n) { if (g_n == 2048) die(path, "too many colours"); g_rgb[g_n++] = v; }
    }
    if (g_n == before) die(path, "no #RRGGBB colours");
    free(s);
}

static uint32_t crc_table[256];
static void crc_init(void)
{
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crc_table[n] = c;
    }
}
static uint32_t crc(uint32_t c, const unsigned char *b, size_t n)
{
    c ^= 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) c = crc_table[(c ^ b[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}
static void be32(unsigned char *p, uint32_t v) { p[0] = (unsigned char)(v >> 24); p[1] = (unsigned char)(v >> 16); p[2] = (unsigned char)(v >> 8); p[3] = (unsigned char)v; }
static void chunk(FILE *f, const char *type, const unsigned char *data, size_t n)
{
    unsigned char hdr[8], tail[4];
    uint32_t c;
    be32(hdr, (uint32_t)n);
    memcpy(hdr + 4, type, 4);
    c = crc(0, hdr + 4, 4);
    c = crc(c, data, n);
    be32(tail, c);
    fwrite(hdr, 1, 8, f);
    if (n) fwrite(data, 1, n, f);
    fwrite(tail, 1, 4, f);
}

int main(int argc, char **argv)
{
    long n, i;
    int w, h, maxv, x0, y0, x1, y1, cw, ch, x, y, off = 0;
    unsigned char *ppm, *px, *raw, *z;
    size_t rawn, zn, zp = 0;
    uint32_t a = 1, b = 0;
    FILE *out;
    if (argc != 9) die("usage", "ppm_grey_png clut.json color_canon.json in.ppm x0 y0 x1 y1 out.png");
    load_colours(argv[1]);
    load_colours(argv[2]);
    ppm = slurp(argv[3], &n);
    if (sscanf((char *)ppm, "P6 %d %d %d%n", &w, &h, &maxv, &off) != 3 || maxv != 255 || off <= 0) die(argv[3], "not a P6 maxval-255 PPM");
    px = ppm + off + 1;
    if ((long)(off + 1) + (long)w * h * 3 > n) die(argv[3], "truncated");
    x0 = atoi(argv[4]); y0 = atoi(argv[5]); x1 = atoi(argv[6]); y1 = atoi(argv[7]);
    if (x0 < 0 || y0 < 0 || x1 > w || y1 > h || x0 >= x1 || y0 >= y1) die(argv[3], "crop outside the screen");
    cw = x1 - x0; ch = y1 - y0;
    rawn = (size_t)ch * (size_t)(cw + 1);
    raw = malloc(rawn);
    if (!raw) die("memory", "raw");
    for (y = 0; y < ch; y++) {
        raw[(size_t)y * (size_t)(cw + 1)] = 0;                    /* filter: none */
        for (x = 0; x < cw; x++) {
            const unsigned char *p = px + ((size_t)(y + y0) * (size_t)w + (size_t)(x + x0)) * 3;
            uint32_t v = (uint32_t)p[0] << 16 | (uint32_t)p[1] << 8 | p[2];
            int k;
            for (k = 0; k < g_n && g_rgb[k] != v; k++) ;
            if (k == g_n) {
                char d[96];
                snprintf(d, sizeof d, "pixel (%d,%d) #%06X is in neither locked colour file", x + x0, y + y0, (unsigned)v);
                die(argv[3], d);
            }
            raw[(size_t)y * (size_t)(cw + 1) + 1 + (size_t)x] =
                (unsigned char)((299u * p[0] + 587u * p[1] + 114u * p[2] + 500u) / 1000u);
        }
    }
    /* PNG filter Up on every row after the first (the filter byte 2) */
    for (y = ch - 1; y >= 1; y--) {
        unsigned char *r = raw + (size_t)y * (size_t)(cw + 1), *u = r - (cw + 1);
        r[0] = 2;
        for (x = 1; x <= cw; x++) r[x] = (unsigned char)(r[x] - u[x]);
    }
    /* adler32 of the filtered bytes */
    for (i = 0; i < (long)rawn; i++) { a = (a + raw[i]) % 65521u; b = (b + a) % 65521u; }
    z = malloc(rawn * 2 + 64);
    if (!z) die("memory", "zlib");
    zn = rawn * 2 + 64;
    z[zp++] = 0x78; z[zp++] = 0x01;
    {
        uint32_t bitbuf = 0; int nbits = 0;
#define PUTBITS(v, n) do { bitbuf |= (uint32_t)(v) << nbits; nbits += (n); \
            while (nbits >= 8) { if (zp >= zn) die("zlib", "overflow"); z[zp++] = (unsigned char)bitbuf; bitbuf >>= 8; nbits -= 8; } } while (0)
        /* a Huffman code goes MSB first: reverse it into the LSB-first stream */
#define PUTCODE(c, n) do { uint32_t rv = 0; for (int q = 0; q < (n); q++) rv |= (((uint32_t)(c) >> q) & 1u) << ((n) - 1 - q); PUTBITS(rv, n); } while (0)
        static const int lbase[29] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258 };
        static const int lext[29]  = { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0 };
        size_t at = 0;
        PUTBITS(1, 1);            /* BFINAL */
        PUTBITS(1, 2);            /* BTYPE = 01, fixed Huffman */
        while (at < rawn) {
            int c = raw[at];
            size_t run = 0;
            if (at > 0) while (at + run < rawn && raw[at + run] == raw[at - 1] && run < 258) run++;
            if (run >= 3) {
                int k = 28;
                while (lbase[k] > (int)run) k--;
                int sym = 257 + k;
                if (sym <= 279) PUTCODE(sym - 256, 7); else PUTCODE(0xC0 + (sym - 280), 8);
                if (lext[k]) PUTBITS((uint32_t)((int)run - lbase[k]), lext[k]);
                PUTCODE(0, 5);    /* distance code 0: distance 1 */
                at += (size_t)lbase[k] + ((int)run - lbase[k]);
                continue;
            }
            if (c <= 143) PUTCODE(0x30 + c, 8); else PUTCODE(0x190 + (c - 144), 9);
            at++;
        }
        PUTCODE(0, 7);            /* end of block (256) */
        if (nbits) PUTBITS(0, 8 - nbits);
#undef PUTBITS
#undef PUTCODE
    }
    if (zp + 4 > zn) die("zlib", "overflow");
    be32(z + zp, b << 16 | a);
    zp += 4;
    crc_init();
    out = fopen(argv[8], "wb");
    if (!out) die(argv[8], "cannot create");
    {
        static const unsigned char sig[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
        unsigned char ihdr[13];
        fwrite(sig, 1, 8, out);
        be32(ihdr, (uint32_t)cw); be32(ihdr + 4, (uint32_t)ch);
        ihdr[8] = 8; ihdr[9] = 0; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
        chunk(out, "IHDR", ihdr, 13);
        chunk(out, "IDAT", z, zp);
        chunk(out, "IEND", NULL, 0);
    }
    if (fclose(out)) die(argv[8], "write");
    printf(">>> ppm_grey_png: %s %dx%d grey from (%d,%d) of %s (%d locked colours)\n", argv[8], cw, ch, x0, y0, argv[3], g_n);
    free(raw); free(z); free(ppm);
    return 0;
}
