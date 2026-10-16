/*
 * harness/pubs/flair_shot.c -- the pixel route of PUB-REF-0001 NT-21/NT-22
 * for FLAIR screens (factory C; bead initech-ipu5.9, GC-04).
 *
 * A FLAIR screen figure in an Initech manual is a QEMU screendump of the
 * running system (pubs/screens/flair_capture.sh drives the harness with a
 * locked trace), cropped to a rectangle named in the book's screens/FLAIR
 * list, converted to grey and replicated at an integer scale -- nothing
 * redrawn, resampled or smoothed (NT-20, NT-21):
 *
 *   flair_shot png <dump.ppm> <x0> <y0> <x1> <y1> <scale> <out.png>
 *                  <clut.json> <color_canon.json>
 *       crop [x0,x1) x [y0,y1) of the 640 x 480 P6 dump; every pixel must be
 *       a colour named in spec/assets/clut.json (the 8-bit device CLUT) or
 *       spec/assets/color_canon.json (the FLAIR chrome canon) -- any other
 *       value is a loud failure (Rule 2), never a guessed grey.  Grey is the
 *       fixed table generated from those two files: CCIR 601 luma,
 *       (299 R + 587 G + 114 B + 500) / 1000 (NT-22: colour becomes black
 *       screen only; the reverse-video navy prints near-solid black).  Each
 *       source pixel becomes a scale x scale block; the PNG is 8-bit grey,
 *       non-interlaced, carries pHYs = 600 dpi so that the PDF places one
 *       source pixel on exactly `scale` device pixels of a 600 dpi grid, and
 *       no time or text chunk (Rule 11).
 *   flair_shot b64 <in.png> <out.b64>  /  flair_shot unb64 <in.b64> <out.png>
 *       the committed form of a figure.  pubs/ holds printable ASCII only
 *       (GC-05, Rule 12; test-pubs-build and test-pubs-lint scan every file
 *       of it), so a figure is committed as screens/<name>.b64, the PNG in
 *       RFC 1421 base64, 76 characters a line; the build decodes it into
 *       build/pubs/gen before typesetting.  unb64 refuses any character
 *       outside the alphabet (Rule 2).
 *   flair_shot same <committed.png> <recaptured.png>
 *       the two images decode to the same size and the same grey bytes
 *       (independent of the zlib that wrote them); else
 *       "!!! test-pubs RED SCREEN-DRIFT: ..." and exit 1.
 *
 * libpng, as the other factory image tools (harness/menubar_clock_oracle.c).
 * ASCII-clean (Rule 12).
 */
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void die(const char *tag, const char *a, const char *b)
{ fflush(stdout); fprintf(stderr, "!!! test-pubs RED %s: %s: %s\n", tag, a, b); exit(1); }

static unsigned char *slurp(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb"); long n; unsigned char *b;
    if (!f) die("SCREEN-INPUT", path, "cannot open");
    if (fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0) die("SCREEN-INPUT", path, "size");
    rewind(f); b = malloc((size_t)n + 1);
    if (!b || fread(b, 1, (size_t)n, f) != (size_t)n) die("SCREEN-INPUT", path, "read");
    fclose(f); b[n] = 0; *len = (size_t)n; return b;
}

/* The allowed colours: every "#RRGGBB" string of the two spec files. */
static unsigned g_rgb[1024]; static int g_n;
static int hexv(int c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; }
static void load_colours(const char *path)
{
    size_t n; unsigned char *s = slurp(path, &n); int found = 0;
    for (size_t i = 0; i + 8 < n; i++) {
        if (s[i] != '"' || s[i + 1] != '#' || s[i + 8] != '"') continue;
        unsigned v = 0; int ok = 1;
        for (int k = 0; k < 6; k++) { int h = hexv(s[i + 2 + k]); if (h < 0) ok = 0; v = v * 16 + (unsigned)(h < 0 ? 0 : h); }
        if (!ok) continue;
        if (g_n == 1024) die("SCREEN-INPUT", path, "too many colours");
        g_rgb[g_n++] = v; found++;
    }
    if (!found) die("SCREEN-INPUT", path, "no #RRGGBB colours");
    free(s);
}
static int grey_of(unsigned r, unsigned g, unsigned b, int x, int y, const char *path)
{
    unsigned v = r << 16 | g << 8 | b;
    for (int i = 0; i < g_n; i++) if (g_rgb[i] == v)
        return (int)((299 * r + 587 * g + 114 * b + 500) / 1000);
    char d[96]; snprintf(d, sizeof d, "pixel (%d,%d) #%06X is not a spec colour", x, y, v);
    die("SCREEN-COLOUR", path, d); return 0;
}

static void cmd_png(char **a)
{
    size_t n; unsigned char *d = slurp(a[2], &n), *p = d;
    int w, h, mx, x0 = atoi(a[3]), y0 = atoi(a[4]), x1 = atoi(a[5]), y1 = atoi(a[6]), sc = atoi(a[7]);
    load_colours(a[9]); load_colours(a[10]);
    if (sscanf((char *)d, "P6 %d %d %d", &w, &h, &mx) != 3 || mx != 255) die("SCREEN-INPUT", a[2], "not a P6/255 dump");
    /* the header is four whitespace-separated fields; data after one more byte */
    for (int f = 0; f < 4; f++) { while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++; while (*p > ' ') p++; }
    p++;
    if ((size_t)(p - d) + (size_t)w * (size_t)h * 3 > n) die("SCREEN-INPUT", a[2], "truncated");
    if (x0 < 0 || y0 < 0 || x1 > w || y1 > h || x0 >= x1 || y0 >= y1 || sc < 1 || sc > 8) die("SCREEN-CROP", a[2], "bad rectangle or scale");
    int ow = (x1 - x0) * sc, oh = (y1 - y0) * sc;
    unsigned char *row = malloc((size_t)ow);
    FILE *o = fopen(a[8], "wb");
    if (!row || !o) die("SCREEN-OUTPUT", a[8], "cannot write");
    png_structp ps = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    png_infop pi = png_create_info_struct(ps);
    if (!ps || !pi || setjmp(png_jmpbuf(ps))) die("SCREEN-OUTPUT", a[8], "libpng");
    png_init_io(ps, o);
    png_set_compression_level(ps, 9);
    png_set_IHDR(ps, pi, (png_uint_32)ow, (png_uint_32)oh, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_pHYs(ps, pi, 23622, 23622, PNG_RESOLUTION_METER);   /* 600 dpi */
    png_write_info(ps, pi);
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            const unsigned char *q = p + ((size_t)y * (size_t)w + (size_t)x) * 3;
            int g = grey_of(q[0], q[1], q[2], x, y, a[2]);
            memset(row + (size_t)(x - x0) * (size_t)sc, g, (size_t)sc);
        }
        for (int k = 0; k < sc; k++) png_write_row(ps, row);
    }
    png_write_end(ps, pi);
    png_destroy_write_struct(&ps, &pi);
    if (fclose(o)) die("SCREEN-OUTPUT", a[8], "close");
    free(row); free(d);
}

static unsigned char *read_grey(const char *path, png_uint_32 *w, png_uint_32 *h)
{
    png_image im; memset(&im, 0, sizeof im); im.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&im, path)) die("SCREEN-DRIFT", path, "missing or unreadable PNG");
    im.format = PNG_FORMAT_GRAY;
    unsigned char *b = malloc(PNG_IMAGE_SIZE(im));
    if (!b || !png_image_finish_read(&im, NULL, b, 0, NULL)) die("SCREEN-DRIFT", path, "decode");
    *w = im.width; *h = im.height; png_image_free(&im); return b;
}
static void cmd_same(char **a)
{
    png_uint_32 w1, h1, w2, h2;
    unsigned char *p = read_grey(a[2], &w1, &h1), *q = read_grey(a[3], &w2, &h2);
    if (w1 != w2 || h1 != h2) {
        char d[96]; snprintf(d, sizeof d, "size %ux%u, recaptured %ux%u", (unsigned)w1, (unsigned)h1, (unsigned)w2, (unsigned)h2);
        die("SCREEN-DRIFT", a[2], d);
    }
    for (size_t i = 0; i < (size_t)w1 * h1; i++) if (p[i] != q[i]) {
        char d[128]; snprintf(d, sizeof d, "pixel (%zu,%zu) %d, recaptured %d (%s)", i % w1, i / w1, p[i], q[i], a[3]);
        die("SCREEN-DRIFT", a[2], d);
    }
    free(p); free(q);
}

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static void cmd_b64(char **a)
{
    size_t n; unsigned char *d = slurp(a[2], &n);
    FILE *o = fopen(a[3], "wb"); int col = 0;
    if (!o) die("SCREEN-OUTPUT", a[3], "cannot write");
    for (size_t i = 0; i < n; i += 3) {
        unsigned v = (unsigned)d[i] << 16 | (i + 1 < n ? (unsigned)d[i + 1] << 8 : 0) | (i + 2 < n ? d[i + 2] : 0);
        char q[4] = { B64[v >> 18 & 63], B64[v >> 12 & 63], i + 1 < n ? B64[v >> 6 & 63] : '=', i + 2 < n ? B64[v & 63] : '=' };
        fwrite(q, 1, 4, o); col += 4;
        if (col == 76) { fputc('\n', o); col = 0; }
    }
    if (col) fputc('\n', o);
    if (fclose(o)) die("SCREEN-OUTPUT", a[3], "close");
    free(d);
}
static void cmd_unb64(char **a)
{
    size_t n; unsigned char *d = slurp(a[2], &n);
    FILE *o = fopen(a[3], "wb"); unsigned v = 0; int k = 0, pad = 0;
    if (!o) die("SCREEN-OUTPUT", a[3], "cannot write");
    for (size_t i = 0; i < n; i++) {
        int c = d[i], x;
        if (c == '\n') continue;
        if (c == '=') { pad++; x = 0; }
        else { const char *p = strchr(B64, c); if (!c || !p || pad) die("SCREEN-INPUT", a[2], "not base64"); x = (int)(p - B64); }
        v = v << 6 | (unsigned)x;
        if (++k == 4) {
            unsigned char b[3] = { (unsigned char)(v >> 16), (unsigned char)(v >> 8), (unsigned char)v };
            fwrite(b, 1, (size_t)(3 - pad), o); v = 0; k = 0;
        }
    }
    if (k) die("SCREEN-INPUT", a[2], "truncated base64");
    if (fclose(o)) die("SCREEN-OUTPUT", a[3], "close");
    free(d);
}

int main(int argc, char **argv)
{
    if (argc == 11 && !strcmp(argv[1], "png")) cmd_png(argv);
    else if (argc == 4 && !strcmp(argv[1], "same")) cmd_same(argv);
    else if (argc == 4 && !strcmp(argv[1], "b64")) cmd_b64(argv);
    else if (argc == 4 && !strcmp(argv[1], "unb64")) cmd_unb64(argv);
    else die("USAGE", "flair_shot", "png DUMP X0 Y0 X1 Y1 SCALE OUT CLUT CANON | same A B | b64 PNG B64 | unb64 B64 PNG");
    return 0;
}
