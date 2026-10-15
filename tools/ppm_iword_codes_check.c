/*
 * tools/ppm_iword_codes_check.c -- grade InitechWord's STYLED CELLS on a
 * 640x480 P6 screendump, pixel for pixel (factory C; bead initech-jojk,
 * IWORD P2 slice 1; test-flair-iword-codes leg STYLE).
 *
 * THE SET-UP the gate's trace makes: the same text typed twice, on two
 * screen lines -- PLAIN on the first, then with F6 / F8 / Ctrl-F8 2 4 around
 * its words on the second. The glyph shapes are the VGA BIOS's own ROM 8x16
 * strike (spec/toolbox_gate.h Sec 7a), which the factory does not hold, so
 * the PLAIN line IS the reference strike: each styled cell must equal the
 * plain cell above it transformed by the synthesis rules, restated here from
 * their documentation (never from os/flair/text.h or tbxgate.c -- Law 2):
 *   BOLD      every ink pixel also inks the pixel to its right (the 1-px
 *             smear; ../system7-decomp specs/fonts/font-manager.md Sec 6.1);
 *   ITALIC    row r moves floor((B - r) / 5) px right (left for rows below
 *             the baseline), B the strike's baseline row (font-manager.md
 *             Sec 6.2 shear; the 1:5 ratio measured on Geneva 9 NFNT 6475 vs
 *             5415, os/flair/text.h's documented reading);
 *   UNDERLINE a 1-px line on row B + 1 under the whole run, missing within
 *             1 px of any ink on that row (the halo; QuickDraw2.p FMOutRec
 *             ulOffset / ulThick / ulShadow as InitechOS reads them);
 *   THE BOX   a styled run keeps its 8-px cells; ink that the smear or the
 *             shear would carry outside the run's [x, x + 8n) is not drawn
 *             (spec/toolbox_gate.h Sec 7a STYLE).
 * B is MEASURED here first: the plain run's letters (no descenders in the
 * gate's text) must end exactly on row B - 1 = 11 (leg BASELINE), which is
 * what spec/toolbox_gate.h TBX_CELL_BASELINE claims for the ROM strike.
 *
 * usage: ppm_iword_codes_check dump.ppm X0 YPLAIN YSTYLED BGX BGY SEG...
 *   X0       global x of column 0; YPLAIN / YSTYLED: global y of the two
 *            lines' cell tops; (BGX, BGY) a known-blank field pixel;
 *   SEG      "from:to:face" -- columns [from, to) styled with txFace face
 *            (1 bold, 2 italic, 4 underline, ORed).
 *   Every column between segments, and [0, last to), must be identical on
 *   the two lines (plain text and blanks stay plain).
 * Prints one line per leg; exit 0 green, 1 RED, 2 malformed input.
 * ASCII-clean (Rule 12). Deterministic.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 640
#define H 480
#define CELL_W 8
#define CELL_H 16
#define BASE 12                       /* the claim leg BASELINE measures */

static unsigned char *g_px;
static unsigned char g_bg[3];

static int read_uint(FILE *f, unsigned *out)
{
    int c;
    unsigned v = 0;
    do {
        c = fgetc(f);
        if (c == '#') { while (c != '\n' && c != EOF) c = fgetc(f); }
    } while (c == ' ' || c == '\n' || c == '\r' || c == '\t');
    if (c < '0' || c > '9') return 1;
    while (c >= '0' && c <= '9') { v = v * 10u + (unsigned)(c - '0'); c = fgetc(f); }
    *out = v;
    return 0;
}

static int ink(int x, int y)
{
    const unsigned char *p = g_px + ((size_t)y * W + (size_t)x) * 3u;
    return !(p[0] == g_bg[0] && p[1] == g_bg[1] && p[2] == g_bg[2]);
}

static int floordiv(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }

int main(int argc, char **argv)
{
    char magic[3] = { 0, 0, 0 };
    unsigned w, h, maxv;
    FILE *f;
    int x0, yp, ys, bgx, bgy, nseg, red = 0, last_to = 0;
    int segf[16], segt[16], segface[16];
    long pix = 0, bad = 0;

    if (argc < 8) {
        fprintf(stderr, "usage: %s dump.ppm X0 YPLAIN YSTYLED BGX BGY from:to:face...\n", argv[0]);
        return 2;
    }
    x0 = atoi(argv[2]); yp = atoi(argv[3]); ys = atoi(argv[4]); bgx = atoi(argv[5]); bgy = atoi(argv[6]);
    nseg = argc - 7;
    if (nseg > 16) return 2;
    for (int i = 0; i < nseg; i++) {
        if (sscanf(argv[7 + i], "%d:%d:%d", &segf[i], &segt[i], &segface[i]) != 3 ||
            segf[i] < 0 || segt[i] <= segf[i] || segface[i] < 1 || segface[i] > 7) {
            fprintf(stderr, "ppm_iword_codes_check: bad segment %s\n", argv[7 + i]);
            return 2;
        }
        if (segt[i] > last_to) last_to = segt[i];
    }
    f = fopen(argv[1], "rb");
    if (f == NULL || fread(magic, 1, 2, f) != 2 || strcmp(magic, "P6") != 0 ||
        read_uint(f, &w) || read_uint(f, &h) || read_uint(f, &maxv) ||
        w != W || h != H || maxv != 255) {
        fprintf(stderr, "ppm_iword_codes_check: %s is not a 640x480x255 P6\n", argv[1]);
        return 2;
    }
    g_px = (unsigned char *)malloc((size_t)W * H * 3u);
    if (g_px == NULL || fread(g_px, 1, (size_t)W * H * 3u, f) != (size_t)W * H * 3u) {
        fprintf(stderr, "ppm_iword_codes_check: short raster\n");
        return 2;
    }
    fclose(f);
    if (x0 < 0 || x0 + last_to * CELL_W > W || yp < 0 || yp + CELL_H > H || ys < 0 || ys + CELL_H > H ||
        bgx < 0 || bgx >= W || bgy < 0 || bgy >= H) {
        fprintf(stderr, "ppm_iword_codes_check: geometry off the dump\n");
        return 2;
    }
    memcpy(g_bg, g_px + ((size_t)bgy * W + (size_t)bgx) * 3u, 3);

    /* BASELINE: the plain line's ink ends on row BASE - 1, rows BASE.. blank */
    {
        int lastrow = -1, inked = 0;
        for (int r = 0; r < CELL_H; r++)
            for (int x = 0; x < last_to * CELL_W; x++)
                if (ink(x0 + x, yp + r)) { lastrow = r; inked++; }
        if (inked < 100 || lastrow != BASE - 1) {
            printf("BASELINE RED: the plain run's last ink row is %d (want %d), %d ink px\n", lastrow, BASE - 1, inked);
            red = 1;
        } else printf("BASELINE green: the plain run (%d ink px) ends on row %d -> baseline row %d\n", inked, lastrow, BASE);
    }

    /* STYLE: each segment == its model; PLAIN: every other column identical */
    for (int i = 0; i < nseg && !red; i++) {
        int n = segt[i] - segf[i], face = segface[i];
        int bw = n * CELL_W;
        long segbad = 0;
        static unsigned char model[CELL_H][80 * CELL_W + 16];
        memset(model, 0, sizeof model);
        for (int r = 0; r < CELL_H; r++) {
            int shift = (face & 2) ? floordiv(BASE - r, 5) : 0;
            for (int x = 0; x < bw; x++) {
                if (!ink(x0 + segf[i] * CELL_W + x, yp + r)) continue;
                int nx = x + shift + 8;              /* +8: room for a left shear */
                model[r][nx] = 1;
                if (face & 1) model[r][nx + 1] = 1;
            }
        }
        if (face & 4) {
            /* the halo is judged against the GLYPH ink of that row only */
            static unsigned char glyph[80 * CELL_W + 16];
            int ur = BASE + 1;
            memcpy(glyph, model[ur], sizeof glyph);
            for (int x = 0; x < bw; x++) {
                int gap = 0;
                for (int k = -1; k <= 1; k++) if (glyph[x + 8 + k]) gap = 1;
                if (!gap) model[ur][x + 8] = 2;
            }
        }
        for (int r = 0; r < CELL_H; r++)
            for (int x = 0; x < bw; x++) {
                int want = model[r][x + 8] != 0, have = ink(x0 + segf[i] * CELL_W + x, ys + r);
                pix++;
                if (want != have) { segbad++; if (segbad <= 4) printf("  segment %d:%d face %d: (%d,%d) have %d want %d\n", segf[i], segt[i], face, x, r, have, want); }
            }
        bad += segbad;
        printf("STYLE %s: columns %d..%d face %d -- %ld of %d px differ from the synthesized plain cells\n",
               segbad ? "RED" : "green", segf[i], segt[i] - 1, face, segbad, bw * CELL_H);
        if (segbad) red = 1;
    }
    if (!red) {
        long plainbad = 0;
        for (int c = 0; c < last_to; c++) {
            int styled = 0;
            for (int i = 0; i < nseg; i++) if (c >= segf[i] && c < segt[i]) styled = 1;
            if (styled) continue;
            for (int r = 0; r < CELL_H; r++)
                for (int x = 0; x < CELL_W; x++)
                    if (ink(x0 + c * CELL_W + x, yp + r) != ink(x0 + c * CELL_W + x, ys + r)) plainbad++;
        }
        printf("PLAIN %s: the unstyled columns of the two lines differ in %ld px\n", plainbad ? "RED" : "green", plainbad);
        if (plainbad) red = 1;
    }
    printf("%s: %ld styled pixels graded, %ld wrong\n", red ? "RED" : "green", pix, bad);
    free(g_px);
    return red;
}
