/*
 * tools/ppm_flair_itext_check.c -- the [5/5] PIXEL leg of test-flair-itext,
 * graded against the INDEPENDENT Mac OS 8.1 SimpleText capture (bead
 * initech-uelk; re-keys the initech-tdnl.16 leg from Geneva 9 to Geneva 12).
 *
 * USAGE: ppm_flair_itext_check <dump.ppm> <s8_simpletext_doc.png> <NFNT_12112.bin>
 *
 * THE GOLDEN (Law 2 / ADR-0010 -- never our own render): ../system7-decomp
 * goldens/captures/s8_simpletext_doc.png (mint-007; specs/sys8/simpletext-
 * stdfile.md): SimpleText's untitled window holding "Saving tables to disk"
 * Return "PC LOAD LETTER" in Geneva 12, the word "tables" selected (black
 * band, white ink). Its text pen origin is (9,44) [MEAS Sec 1.2: content
 * (4,40) + (5,4)], its line pitch 16. The cell grid (pen advances) of both
 * images comes from the REAL NFNT 12112 owTable (mint/work/geneva12), not
 * from InitechOS's tables. Its text area holds nothing else (verified by
 * diffing it against s8_simpletext_doc_caret.png: the only other differing
 * pixels are the caret at x=109 and the parked pointer at x 619..629).
 *
 * THE DUMP: test-flair-itext's last frame (spec/flair_itext_traces.mk
 * FLAIR_ITEXT_SPEC): the same two lines plus the pasted line 2 "tables",
 * and -- the trace's last gesture -- "tables" on line 0 selected again, so
 * the scene is the capture's. Pen origin GLOBAL (15,70) (the TEXT window's
 * content corner (10,66) + the same [MEAS] (5,4)).
 *
 * LEGS (every failure names the cell and both numbers):
 *   C  per-cell INK BOX: for every glyph cell of lines 0 and 1, the bounding
 *      box of the pixels that are not the cell's background (white, or black
 *      inside the selection), relative to the cell's pen x and line top, is
 *      the capture's; and every pixel of the cell is exactly #000000 or
 *      #FFFFFF. Line 2 "tables" (dump only) is graded against the capture's
 *      "tables" cells of line 0. (Glyph SHAPES are hand-authored, so only the
 *      extents are compared -- spec/assets/geneva12.art provenance.)
 *   B  the SELECTION BAND: on line 0's top row (no glyph ink there) the black
 *      run spans the same pen columns as the capture's ("tables" = pen 45..83),
 *      and the band's first column (the 't' cell's blank lb column) is black
 *      for exactly the 16 line rows, white above and below -- as captured.
 *   W  WHITE elsewhere in the dump's text area: the left inset columns 10..14
 *      and the top inset rows 66..69 (the [MEAS] (5,4) inset), every line's
 *      tail from its pen end to the content edge (x 566), and the body below
 *      the third line (y 118..453).
 *
 * FACTORY CODE (hosted C + libpng, Law 3). ASCII-clean (Rule 12).
 */
#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 640
#define H 480
#define LINE_H 16            /* [MEAS] simpletext-stdfile.md Sec 1.2 */
#define CAP_X 9
#define CAP_Y 44
#define DMP_X 15
#define DMP_Y 70
#define CONT_L 10
#define CONT_T 66
#define CONT_R 567           /* exclusive: content x 10..566 */
#define CONT_B 454           /* exclusive: content y 66..453 */

static uint32_t g_dmp[W * H], g_cap[W * H];
static unsigned char g_nfnt[16384];
static int g_first, g_last;
static long g_ow;
static int g_fails, g_checks;

#define FAIL(...) do { g_fails++; fprintf(stderr, "FAIL itext-pixel "); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)

static int load_ppm(const char *path)
{
    FILE *f = fopen(path, "rb");
    int w, h, mx;
    if (!f) return 0;
    if (fscanf(f, "P6 %d %d %d", &w, &h, &mx) != 3 || w != W || h != H || mx != 255) { fclose(f); return 0; }
    fgetc(f);
    for (int i = 0; i < W * H; i++) {
        int r = fgetc(f), g = fgetc(f), b = fgetc(f);
        if (b == EOF) { fclose(f); return 0; }
        g_dmp[i] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }
    fclose(f);
    return 1;
}

static int load_png(const char *path)
{
    png_image im;
    png_bytep buf;
    memset(&im, 0, sizeof im);
    im.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&im, path)) return 0;
    im.format = PNG_FORMAT_RGB;
    if (im.width != W || im.height != H) { png_image_free(&im); return 0; }
    buf = malloc(PNG_IMAGE_SIZE(im));
    if (!buf || !png_image_finish_read(&im, NULL, buf, 0, NULL)) { free(buf); return 0; }
    for (int i = 0; i < W * H; i++)
        g_cap[i] = ((uint32_t)buf[3 * i] << 16) | ((uint32_t)buf[3 * i + 1] << 8) | buf[3 * i + 2];
    free(buf);
    return 1;
}

static int load_nfnt(const char *path)
{
    FILE *f = fopen(path, "rb");
    long n;
    if (!f) return 0;
    n = (long)fread(g_nfnt, 1, sizeof g_nfnt, f);
    fclose(f);
    if (n < 26) return 0;
    g_first = (int16_t)((g_nfnt[2] << 8) | g_nfnt[3]);
    g_last = (int16_t)((g_nfnt[4] << 8) | g_nfnt[5]);
    g_ow = 16 + 2L * (int16_t)((g_nfnt[16] << 8) | g_nfnt[17]);
    return g_ow + 2L * (g_last - g_first + 2) <= n;
}

static int aw(int c) { return g_nfnt[g_ow + 2L * (c - g_first) + 1]; }

typedef struct { int l, r, t, b, stray; } box_t;

/* The ink box of the cell [px, px + w) x [top, top + 16) of image img,
 * relative to (px, top); ink = not bg. stray = pixels neither #000/#FFF. */
static box_t cell_box(const uint32_t *img, int px, int top, int w, uint32_t bg)
{
    box_t e = { 99, -1, 99, -1, 0 };
    for (int y = 0; y < LINE_H; y++)
        for (int x = 0; x < w; x++) {
            uint32_t p = img[(top + y) * W + px + x];
            if (p != 0x000000u && p != 0xFFFFFFu) e.stray++;
            if (p != bg) {
                if (x < e.l) e.l = x;
                if (x > e.r) e.r = x;
                if (y < e.t) e.t = y;
                if (y > e.b) e.b = y;
            }
        }
    return e;
}

static int pen_of(const char *s, int k)
{
    int x = 0;
    for (int i = 0; i < k; i++) x += aw((unsigned char)s[i]);
    return x;
}

/* Grade the dump's line (string s, top dt, selection [sa,sb)) against the
 * capture's line (string cs, top ct, selection [ca,cb)), cell k of the dump
 * vs cell k + coff of the capture. */
static void grade_line(const char *name, const char *s, int dt, int sa, int sb,
                       const char *cs, int ct, int ca, int cb, int coff)
{
    for (int k = 0; s[k]; k++) {
        int c = (unsigned char)s[k], w = aw(c), kc = k + coff;
        uint32_t dbg = (k >= sa && k < sb) ? 0x000000u : 0xFFFFFFu;
        uint32_t cbg = (kc >= ca && kc < cb) ? 0x000000u : 0xFFFFFFu;
        box_t d = cell_box(g_dmp, DMP_X + pen_of(s, k), dt, w, dbg);
        box_t r = cell_box(g_cap, CAP_X + pen_of(cs, kc), ct, w, cbg);
        g_checks++;
        if ((unsigned char)cs[kc] != c) { FAIL("%s cell %d: capture glyph mismatch (harness error)", name, k); continue; }
        if (d.stray || r.stray)
            FAIL("%s cell %d '%c': %d dump / %d capture pixels neither #000000 nor #FFFFFF", name, k, c, d.stray, r.stray);
        if (d.l != r.l || d.r != r.r || d.t != r.t || d.b != r.b)
            FAIL("%s cell %d '%c': dump ink x %d..%d y %d..%d, capture x %d..%d y %d..%d",
                 name, k, c, d.l, d.r, d.t, d.b, r.l, r.r, r.t, r.b);
    }
}

static int white_rect(int l, int t, int r, int b)
{
    int n = 0;
    for (int y = t; y < b; y++)
        for (int x = l; x < r; x++)
            n += g_dmp[y * W + x] != 0xFFFFFFu;
    return n;
}

int main(int argc, char **argv)
{
    static const char *L0 = "Saving tables to disk", *L1 = "PC LOAD LETTER", *L2 = "tables";
    int n, run_d0 = -1, run_d1 = -1, run_c0 = -1, run_c1 = -1;
    if (argc != 4) { fprintf(stderr, "usage: ppm_flair_itext_check dump.ppm capture.png NFNT_12112.bin\n"); return 2; }
    if (!load_ppm(argv[1])) { fprintf(stderr, "FAIL itext-pixel: cannot read the dump %s\n", argv[1]); return 2; }
    if (!load_png(argv[2])) { fprintf(stderr, "FAIL itext-pixel: cannot read the capture %s (the independent golden; fail loud)\n", argv[2]); return 2; }
    if (!load_nfnt(argv[3])) { fprintf(stderr, "FAIL itext-pixel: cannot read the NFNT %s (fail loud)\n", argv[3]); return 2; }

    /* C: every cell's ink box, the capture's */
    grade_line("L0", L0, DMP_Y, 7, 13, L0, CAP_Y, 7, 13, 0);
    grade_line("L1", L1, DMP_Y + LINE_H, 0, 0, L1, CAP_Y + LINE_H, 0, 0, 0);
    grade_line("L2", L2, DMP_Y + 2 * LINE_H, 0, 0, L0, CAP_Y, 7, 13, 7);

    /* B: the band on line 0's top row, and its first column */
    for (int x = 0; x < CONT_R - DMP_X; x++)
        if (g_dmp[DMP_Y * W + DMP_X + x] == 0x000000u) { if (run_d0 < 0) run_d0 = x; run_d1 = x; }
    for (int x = 0; x < 556 - CAP_X; x++)
        if (g_cap[CAP_Y * W + CAP_X + x] == 0x000000u) { if (run_c0 < 0) run_c0 = x; run_c1 = x; }
    g_checks++;
    if (run_d0 != run_c0 || run_d1 != run_c1 || run_c0 < 0)
        FAIL("band top row: dump black pen %d..%d, capture %d..%d", run_d0, run_d1, run_c0, run_c1);
    for (int y = -1; y <= LINE_H; y++) {
        int pc = pen_of(L0, 7);
        uint32_t d = g_dmp[(DMP_Y + y) * W + DMP_X + pc], c = g_cap[(CAP_Y + y) * W + CAP_X + pc];
        g_checks++;
        if (d != c) FAIL("band column pen %d row %d: dump #%06X, capture #%06X", pc, y, (unsigned)d, (unsigned)c);
    }

    /* W: white elsewhere in the dump's text area */
    g_checks += 6;
    if ((n = white_rect(CONT_L, CONT_T, DMP_X, DMP_Y + 3 * LINE_H))) FAIL("left inset x %d..%d: %d non-white px", CONT_L, DMP_X - 1, n);
    if ((n = white_rect(CONT_L, CONT_T, CONT_R, DMP_Y))) FAIL("top inset y %d..%d: %d non-white px", CONT_T, DMP_Y - 1, n);
    if ((n = white_rect(DMP_X + pen_of(L0, 21), DMP_Y, CONT_R, DMP_Y + LINE_H))) FAIL("line 0 tail: %d non-white px", n);
    if ((n = white_rect(DMP_X + pen_of(L1, 14), DMP_Y + LINE_H, CONT_R, DMP_Y + 2 * LINE_H))) FAIL("line 1 tail: %d non-white px", n);
    if ((n = white_rect(DMP_X + pen_of(L2, 6), DMP_Y + 2 * LINE_H, CONT_R, DMP_Y + 3 * LINE_H))) FAIL("line 2 tail: %d non-white px", n);
    if ((n = white_rect(CONT_L, DMP_Y + 3 * LINE_H, CONT_R, CONT_B))) FAIL("body y %d..%d: %d non-white px", DMP_Y + 3 * LINE_H, CONT_B - 1, n);

    if (g_fails) { fprintf(stderr, "FAIL itext-pixel: %d of %d checks\n", g_fails, g_checks); return 1; }
    printf("itext-pixel PASS: %d checks -- 41 Geneva 12 cell ink boxes == s8_simpletext_doc.png, \"tables\" band pen %d..%d x 16 rows as captured, inset/tails/body white\n",
           g_checks, run_d0, run_d1);
    return 0;
}
