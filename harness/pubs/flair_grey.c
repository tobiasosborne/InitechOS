/*
 * harness/pubs/flair_grey.c -- FLAIR screen reproductions for the manuals
 * (bead initech-ipu5.11; PUB-REF-0001 NT-20, NT-21, NT-22, GC-04).
 *
 Two modes:
 *
 *   flair_grey capture <clut.json> <color_canon.json> <in.ppm> <x> <y> <w> <h> <out.grey>
 *   flair_grey png <in.grey> <out.png>
 *
 * `capture` converts one QEMU screendump of the FLAIR desktop (binary PPM,
 * P6, the RGB the VBE framebuffer showed) into the GREY CAPTURE the manual
 * commits under pubs/manuals/<book>/screens/<name>.grey: an ASCII listing of
 * the grey pixels (the FLAIR counterpart of the DOS books' decoded .txt
 * screens; ASCII so the publications tree stays text, Rule 12 / GC-05):
 *
 *   FLAIRGREY 1 <w> <h> <x> <y>        (crop size, then its screen origin)
 *   <one line per pixel row: space-separated runs, each HH or HH*N -- the
 *    grey value in two hex digits, N its repeat count; a row identical to
 *    the row above is the single character =>
 *
 * `png` expands a .grey listing into the PNG the book prints (build/pubs/gen,
 * never committed).
 *
 * NT-22 (tone): FLAIR colour captures print through a FIXED palette-to-grey
 * table.  The table is generated here, every run, from the LOCKED palette
 * data: every "rgb"/"hex" value "#RRGGBB" of spec/assets/clut.json (the
 * 8-bit standard CLUT) and spec/assets/color_canon.json (the FLAIR colour
 * canon, whose rows carry the colours the desktop actually shows: the teal
 * desktop, the Platinum greys).  Each legal colour maps to its Rec. 601 luma,
 * round(0.299 R + 0.587 G + 0.114 B), in integer arithmetic.  A pixel whose
 * colour is in neither table is RED (Rule 2): the capture is not a FLAIR
 * screen (a BIOS text screen, a half-drawn frame, a palette drift), and no
 * guessed grey may reach print.
 *
 * NT-21 (pixels): no resampling.  The crop (x, y, w, h; "0 0 0 0" = the
 * whole screen) is copied pixel for pixel; the manual places the image at
 * an integer multiple of the 600 dpi device grid (initech-flair.sty), with
 * interpolation off, so each screen pixel prints as an exact square.
 *
 * PNG: 8-bit greyscale, no filter, deflate with fixed Huffman codes
 * and a greedy LZ77 match (written here: no zlib, no host image library --
 * Law 3, C only).  No ancillary chunks, no time chunk: the bytes depend on
 * the pixels only (Rule 11), so `make test-pubs-screens` can compare the
 * committed .grey listing with a fresh capture byte for byte, and the
 * PNG is a pure function of the listing.
 *
 * ASCII-clean (Rule 12).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static void die(const char *what, const char *detail)
{
    fprintf(stderr, "!!! test-pubs RED FLAIR-GREY: %s: %s\n", what, detail);
    exit(1);
}

/* ---- the legal colour table ------------------------------------------- */
#define MAXCOL 2048
static uint32_t g_col[MAXCOL];
static size_t g_ncol;

static int hexval(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static char *slurp(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    long n;
    char *s;
    if (!f) die("cannot open", path);
    if (fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0) die("cannot size", path);
    rewind(f);
    s = malloc((size_t)n + 1);
    if (!s || fread(s, 1, (size_t)n, f) != (size_t)n) die("cannot read", path);
    s[n] = 0;
    fclose(f);
    if (len) *len = (size_t)n;
    return s;
}

/* Every `"rgb": "#RRGGBB"` and `"hex": "#RRGGBB"` value of a palette file. */
static void load_palette(const char *path)
{
    char *s = slurp(path, NULL), *p = s;
    size_t before = g_ncol;
    while ((p = strstr(p, "\"#")) != NULL) {
        const char *k = p;
        int ok = 1;
        uint32_t v = 0;
        /* the key in front: ... "rgb": "#  or  "hex": "# */
        while (k > s && (k[-1] == ' ' || k[-1] == ':' || k[-1] == '\t')) k--;
        if (!(k - s >= 5 && (!strncmp(k - 5, "\"rgb\"", 5) || !strncmp(k - 5, "\"hex\"", 5)))) {
            p += 2;
            continue;
        }
        for (int i = 0; i < 6; i++) {
            int h = hexval((unsigned char)p[2 + i]);
            if (h < 0) { ok = 0; break; }
            v = (v << 4) | (uint32_t)h;
        }
        if (ok && p[8] == '"') {
            size_t j;
            for (j = 0; j < g_ncol && g_col[j] != v; j++) ;
            if (j == g_ncol) {
                if (g_ncol == MAXCOL) die(path, "too many colours");
                g_col[g_ncol++] = v;
            }
        }
        p += 2;
    }
    if (g_ncol == before) die(path, "no \"rgb\"/\"hex\" colours found");
    free(s);
}

static int grey_of(uint32_t rgb, uint8_t *out)
{
    for (size_t j = 0; j < g_ncol; j++)
        if (g_col[j] == rgb) {
            uint32_t r = rgb >> 16, g = (rgb >> 8) & 255u, b = rgb & 255u;
            *out = (uint8_t)((299u * r + 587u * g + 114u * b + 500u) / 1000u);
            return 1;
        }
    return 0;
}

/* ---- PPM ---------------------------------------------------------------- */
static const char *ppm_token(const char *p, const char *end, long *v)
{
    while (p < end) {
        if (*p == '#') { while (p < end && *p != '\n') p++; }
        else if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
        else break;
    }
    if (p >= end || *p < '0' || *p > '9') die("ppm", "bad header");
    *v = 0;
    while (p < end && *p >= '0' && *p <= '9') *v = *v * 10 + (*p++ - '0');
    return p;
}

/* ---- deflate (fixed Huffman, greedy LZ77) ------------------------------- */
typedef struct { uint8_t *b; size_t n, cap; uint32_t acc; int nbits; } bits_t;

static void put_byte(bits_t *o, uint8_t c)
{
    if (o->n == o->cap) {
        o->cap = o->cap ? o->cap * 2 : 65536;
        o->b = realloc(o->b, o->cap);
        if (!o->b) die("deflate", "out of memory");
    }
    o->b[o->n++] = c;
}
static void put_bits(bits_t *o, uint32_t v, int n)       /* LSB first */
{
    o->acc |= v << o->nbits;
    o->nbits += n;
    while (o->nbits >= 8) { put_byte(o, (uint8_t)o->acc); o->acc >>= 8; o->nbits -= 8; }
}
static void put_huff(bits_t *o, uint32_t code, int len)  /* MSB first */
{
    uint32_t r = 0;
    for (int i = 0; i < len; i++) r |= ((code >> i) & 1u) << (len - 1 - i);
    put_bits(o, r, len);
}
static void lit(bits_t *o, int v)
{
    if (v <= 143) put_huff(o, 0x30u + (uint32_t)v, 8);
    else if (v <= 255) put_huff(o, 0x190u + (uint32_t)(v - 144), 9);
    else if (v <= 279) put_huff(o, (uint32_t)(v - 256), 7);
    else put_huff(o, 0xC0u + (uint32_t)(v - 280), 8);
}
static const int LBASE[29] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
static const int LEXT[29]  = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
static const int DBASE[30] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
static const int DEXT[30]  = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static void match(bits_t *o, int len, int dist)
{
    int i = 28, d = 29;
    while (LBASE[i] > len) i--;
    lit(o, 257 + i);
    if (LEXT[i]) put_bits(o, (uint32_t)(len - LBASE[i]), LEXT[i]);
    while (DBASE[d] > dist) d--;
    put_huff(o, (uint32_t)d, 5);
    if (DEXT[d]) put_bits(o, (uint32_t)(dist - DBASE[d]), DEXT[d]);
}

#define HBITS 15
#define WIN 32768
static void deflate_fixed(bits_t *o, const uint8_t *s, size_t n)
{
    int32_t *head = malloc(sizeof(int32_t) << HBITS);
    int32_t *prev = malloc(sizeof(int32_t) * (n ? n : 1));
    size_t i = 0;
    if (!head || !prev) die("deflate", "out of memory");
    for (size_t k = 0; k < ((size_t)1 << HBITS); k++) head[k] = -1;
    put_bits(o, 1, 1);          /* BFINAL */
    put_bits(o, 1, 2);          /* BTYPE = 01, fixed Huffman */
#define HASH(p) ((((uint32_t)s[p] << 10) ^ ((uint32_t)s[(p) + 1] << 5) ^ s[(p) + 2]) & ((1u << HBITS) - 1u))
    while (i < n) {
        int best = 0, bdist = 0;
        if (i + 2 < n) {
            uint32_t h = HASH(i);
            int32_t c = head[h];
            int chain = 64;
            while (c >= 0 && (size_t)(i - (size_t)c) <= WIN && chain-- > 0) {
                int l = 0, max = (int)(n - i < 258 ? n - i : 258);
                while (l < max && s[(size_t)c + (size_t)l] == s[i + (size_t)l]) l++;
                if (l > best) { best = l; bdist = (int)(i - (size_t)c); if (l == max) break; }
                c = prev[c];
            }
        }
        if (best >= 3) {
            match(o, best, bdist);
            for (int k = 0; k < best; k++, i++)
                if (i + 2 < n) { uint32_t h = HASH(i); prev[i] = head[h]; head[h] = (int32_t)i; }
        } else {
            lit(o, s[i]);
            if (i + 2 < n) { uint32_t h = HASH(i); prev[i] = head[h]; head[h] = (int32_t)i; }
            i++;
        }
    }
    lit(o, 256);
    if (o->nbits) put_bits(o, 0, 8 - o->nbits);
    free(head);
    free(prev);
}

/* ---- PNG ----------------------------------------------------------------- */
static uint32_t crc_table[256];
static uint32_t crc32_of(const uint8_t *p, size_t n, uint32_t c)
{
    c ^= 0xFFFFFFFFu;
    while (n--) c = crc_table[(c ^ *p++) & 255u] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}
static void be32(uint8_t *p, uint32_t v)
{ p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v; }
static void chunk(FILE *f, const char *type, const uint8_t *data, size_t n)
{
    uint8_t hdr[8], tail[4];
    uint32_t c;
    be32(hdr, (uint32_t)n);
    memcpy(hdr + 4, type, 4);
    c = crc32_of(hdr + 4, 4, 0);
    c = crc32_of(data, n, c);
    be32(tail, c);
    if (fwrite(hdr, 1, 8, f) != 8 || (n && fwrite(data, 1, n, f) != n) || fwrite(tail, 1, 4, f) != 4)
        die("png", "write failed");
}

static void write_png(const char *path, const uint8_t *pix, long cw, long ch)
{
    size_t rawn = (size_t)ch * (size_t)(cw + 1);
    uint8_t *raw = malloc(rawn);
    if (!raw) die("memory", "raw");
    for (long y = 0; y < ch; y++) {
        raw[(size_t)y * (size_t)(cw + 1)] = 0;          /* filter: none */
        memcpy(raw + (size_t)y * (size_t)(cw + 1) + 1, pix + (size_t)y * (size_t)cw, (size_t)cw);
    }
    bits_t z = { 0 };
    put_byte(&z, 0x78);
    put_byte(&z, 0x01);
    deflate_fixed(&z, raw, rawn);
    {
        uint32_t a = 1, b = 0;
        for (size_t i = 0; i < rawn; i++) { a = (a + raw[i]) % 65521u; b = (b + a) % 65521u; }
        uint8_t ad[4];
        be32(ad, (b << 16) | a);
        for (int i = 0; i < 4; i++) put_byte(&z, ad[i]);
    }
    FILE *f = fopen(path, "wb");
    static const uint8_t sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    uint8_t ihdr[13];
    if (!f) die("cannot write", path);
    be32(ihdr, (uint32_t)cw);
    be32(ihdr + 4, (uint32_t)ch);
    ihdr[8] = 8; ihdr[9] = 0; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;  /* 8-bit grey */
    if (fwrite(sig, 1, 8, f) != 8) die("png", "write failed");
    chunk(f, "IHDR", ihdr, 13);
    chunk(f, "IDAT", z.b, z.n);
    chunk(f, "IEND", (const uint8_t *)"", 0);
    if (fclose(f)) die("png", "close failed");
    free(z.b);
    free(raw);
}

static void write_grey(const char *path, const uint8_t *pix, long cw, long ch, long x0, long y0)
{
    FILE *f = fopen(path, "wb");
    if (!f) die("cannot write", path);
    fprintf(f, "FLAIRGREY 1 %ld %ld %ld %ld\n", cw, ch, x0, y0);
    for (long y = 0; y < ch; y++) {
        const uint8_t *row = pix + (size_t)y * (size_t)cw;
        if (y > 0 && !memcmp(row, row - cw, (size_t)cw)) { fputs("=\n", f); continue; }
        for (long x = 0; x < cw;) {
            long r = 1;
            while (x + r < cw && row[x + r] == row[x]) r++;
            if (x) fputc(' ', f);
            if (r > 1) fprintf(f, "%02X*%ld", row[x], r); else fprintf(f, "%02X", row[x]);
            x += r;
        }
        fputc('\n', f);
    }
    if (fclose(f)) die("grey", "close failed");
}

/* Parse a .grey listing; fail loud on any malformed token (Rule 2). */
static uint8_t *read_grey(const char *path, long *cw, long *ch)
{
    char *s = slurp(path, NULL), *p = s, *e;
    long x0, y0, ver;
    if (strncmp(p, "FLAIRGREY ", 10)) die(path, "not a FLAIRGREY listing");
    p += 10;
    ver = strtol(p, &e, 10); p = e;
    *cw = strtol(p, &e, 10); p = e;
    *ch = strtol(p, &e, 10); p = e;
    x0 = strtol(p, &e, 10); p = e;
    y0 = strtol(p, &e, 10); p = e;
    if (ver != 1 || *cw <= 0 || *ch <= 0 || *cw > 4096 || *ch > 4096 || x0 < 0 || y0 < 0 || *p != '\n')
        die(path, "bad FLAIRGREY header");
    p++;
    uint8_t *pix = malloc((size_t)*cw * (size_t)*ch);
    if (!pix) die("memory", "pixels");
    for (long y = 0; y < *ch; y++) {
        uint8_t *row = pix + (size_t)y * (size_t)*cw;
        if (*p == '=' && p[1] == '\n') {
            if (!y) die(path, "first row cannot repeat");
            memcpy(row, row - *cw, (size_t)*cw);
            p += 2;
            continue;
        }
        long x = 0;
        while (*p != '\n') {
            int hi = hexval((unsigned char)p[0]), lo = hexval((unsigned char)p[1]);
            long r = 1;
            if (hi < 0 || lo < 0) die(path, "bad pixel token");
            p += 2;
            if (*p == '*') { r = strtol(p + 1, &e, 10); if (e == p + 1 || r < 2) die(path, "bad run"); p = e; }
            if (x + r > *cw) die(path, "row too long");
            memset(row + x, (hi << 4) | lo, (size_t)r);
            x += r;
            if (*p == ' ') p++;
            else if (*p != '\n') die(path, "bad separator");
        }
        if (x != *cw) die(path, "row too short");
        p++;
    }
    if (*p) die(path, "trailing data after the last row");
    free(s);
    return pix;
}

int main(int argc, char **argv)
{
    size_t len;
    long w, h, maxv, x0, y0, cw, ch;
    char *ppm;
    const char *p, *end;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++) c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crc_table[i] = c;
    }
    if (argc == 4 && !strcmp(argv[1], "png")) {
        uint8_t *pix = read_grey(argv[2], &cw, &ch);
        write_png(argv[3], pix, cw, ch);
        free(pix);
        return 0;
    }
    if (argc != 10 || strcmp(argv[1], "capture")) {
        fprintf(stderr, "usage: flair_grey capture <clut.json> <color_canon.json> <in.ppm> <x> <y> <w> <h> <out.grey>\n"
                        "       flair_grey png <in.grey> <out.png>\n");
        return 2;
    }
    load_palette(argv[2]);
    load_palette(argv[3]);
    ppm = slurp(argv[4], &len);
    end = ppm + len;
    if (len < 2 || ppm[0] != 'P' || ppm[1] != '6') die(argv[4], "not a binary PPM (P6)");
    p = ppm_token(ppm + 2, end, &w);
    p = ppm_token(p, end, &h);
    p = ppm_token(p, end, &maxv);
    if (maxv != 255 || p >= end) die(argv[4], "not an 8-bit PPM");
    p++;
    if ((size_t)(end - p) < (size_t)(w * h * 3)) die(argv[4], "truncated pixels");
    x0 = atol(argv[5]); y0 = atol(argv[6]); cw = atol(argv[7]); ch = atol(argv[8]);
    if (cw == 0 && ch == 0 && x0 == 0 && y0 == 0) { cw = w; ch = h; }
    if (x0 < 0 || y0 < 0 || cw <= 0 || ch <= 0 || x0 + cw > w || y0 + ch > h)
        die(argv[4], "crop rectangle outside the screen");
    uint8_t *pix = malloc((size_t)cw * (size_t)ch);
    if (!pix) die("memory", "pixels");
    for (long y = 0; y < ch; y++)
        for (long x = 0; x < cw; x++) {
            const uint8_t *px = (const uint8_t *)p + ((size_t)(y0 + y) * (size_t)w + (size_t)(x0 + x)) * 3u;
            uint32_t rgb = ((uint32_t)px[0] << 16) | ((uint32_t)px[1] << 8) | px[2];
            if (!grey_of(rgb, &pix[(size_t)y * (size_t)cw + (size_t)x])) {
                char d[96];
                snprintf(d, sizeof d, "pixel (%ld,%ld) is #%06X, in no locked palette", x0 + x, y0 + y, (unsigned)rgb);
                die(argv[4], d);
            }
        }
    write_grey(argv[9], pix, cw, ch, x0, y0);
    free(pix);
    free(ppm);
    return 0;
}
