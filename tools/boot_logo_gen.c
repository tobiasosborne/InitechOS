/* tools/boot_logo_gen.c -- factory-only generator for spec/assets/boot_logo.h,
 * the FLAIR boot-screen strike of the Initech logo (bead initech-tdnl.105).
 *
 * Ref: pubs/common/initech-logo.tex (the TikZ drawing: block faces, the mark
 *      shift, the logotype outlines and the 0.207 slant), docs/pubs/
 *      DESIGN-REFERENCE.md Sec 7.0 (the measured geometry: 2:1 dimetric,
 *      receding edges 1 in 2, cube edge 2 with a unit notch, mark 9u x 6.449u,
 *      lockup 15.264u x 10.459u). CLAUDE.md Law 1 (the strike is AUTHORED from
 *      the measured geometry, never extracted from an image), Rule 11
 *      (deterministic output: no paths, no timestamps), Rule 12 (ASCII).
 *
 * RASTER RULE (shared with the oracle, implemented differently there): a
 * strike pixel (i,j) takes a face's code iff its CENTRE (i+0.5, j+0.5) lies
 * inside the face polygon by the crossing rule (an edge crosses the centre's
 * scanline iff exactly one end lies strictly below it); faces are painted in
 * the TikZ painter's order (each block: light, dark, top; back block first,
 * then the left and right front blocks; then the seven letters). This tool
 * fills by SCANLINE spans over the sorted crossings and polygonises the C's
 * elliptic arcs; harness/proptest/test_boot_logo.c re-renders the same
 * geometry PER PIXEL with an analytic C and must agree pixel for pixel.
 *
 * Usage: boot_logo_gen > spec/assets/boot_logo.h
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define U      16          /* px per unit edge (the logo-cell scale)        */
#define LOCK_W 15.264      /* lockup width in u (initech-logo.tex)          */
#define LOCK_H 10.459      /* lockup height in u, baseline to apex          */
#define SLANT  0.207       /* \im@lg@letters xslant                         */
#define MAXV   32768

enum { CLEAR = 0, TOP = 1, DARK = 2, LIGHT = 3 };

typedef struct { double x, y; } pt;

static int W, H;
static double OX, OY;
static unsigned char img[256][256];

static void die(const char *why)
{
    fprintf(stderr, "boot_logo_gen: %s\n", why);
    exit(1);
}

/* Logo units (y up, letters' baseline origin) -> strike pixels (y down). */
static pt to_px(double x, double y)
{
    pt p;
    p.x = OX + x * U;
    p.y = OY + (LOCK_H - y) * U;
    return p;
}

static int cmp_d(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return x < y ? -1 : x > y;
}

/* Scanline fill of one polygon given in strike pixels. */
static void fill(const pt *v, int n, int code)
{
    static double xs[MAXV];
    for (int j = 0; j < H; j++) {
        double yc = j + 0.5;
        int k = 0;
        for (int a = 0, b = n - 1; a < n; b = a++) {
            if ((v[a].y > yc) != (v[b].y > yc)) {
                if (k >= MAXV) die("crossing overflow");
                xs[k++] = v[b].x + (yc - v[b].y) * (v[a].x - v[b].x) /
                                   (v[a].y - v[b].y);
            }
        }
        if (k & 1) die("odd crossing count");
        qsort(xs, (size_t)k, sizeof xs[0], cmp_d);
        for (int s = 0; s < k; s += 2) {
            for (int i = 0; i < W; i++) {
                double xc = i + 0.5;
                if (xc >= xs[s] && xc < xs[s + 1]) img[j][i] = (unsigned char)code;
            }
        }
    }
}

/* One face of a block: unit coordinates relative to the block origin. */
static void face(const double (*c)[2], int n, double sx, double sy, int code)
{
    pt v[8];
    for (int i = 0; i < n; i++) v[i] = to_px(c[i][0] + sx, c[i][1] + sy);
    fill(v, n, code);
}

/* One upright letter outline, then sheared by the logotype slant. */
static void letter(const double (*c)[2], int n)
{
    pt v[16];
    for (int i = 0; i < n; i++) v[i] = to_px(c[i][0] + SLANT * c[i][1], c[i][1]);
    fill(v, n, DARK);
}

static pt cpath[MAXV];
static int cn;

static void c_point(double x, double y)
{
    if (cn >= MAXV) die("C path overflow");
    cpath[cn++] = to_px(x + SLANT * y, y);
}

static void c_arc(double cx, double cy, double rx, double ry, double a0, double a1)
{
    const int K = 4096;
    for (int k = 0; k <= K; k++) {
        double a = (a0 + (a1 - a0) * k / (double)K) * 3.14159265358979323846 / 180.0;
        c_point(cx + rx * cos(a), cy + ry * sin(a));
    }
}

int main(void)
{
    /* initech-logo.tex \im@lg@block: the six faces of one block. */
    static const double L1[6][2] = {{2,-1},{0,-2},{0,-0.775},{1,-0.275},{1,0.949},{2,1.449}};
    static const double L2[4][2] = {{0,0.225},{-1,-0.275},{-1,0.949},{0,1.449}};
    static const double D1[6][2] = {{-2,-1},{0,-2},{0,-0.775},{-1,-0.275},{-1,0.949},{-2,1.449}};
    static const double D2[4][2] = {{0,0.225},{1,-0.275},{1,0.949},{0,1.449}};
    static const double T1[6][2] = {{0,2.449},{2,1.449},{1,0.949},{0,1.449},{-1,0.949},{-2,1.449}};
    static const double T2[4][2] = {{0,0.225},{1,-0.275},{0,-0.775},{-1,-0.275}};
    /* \im@lg@mark block shifts, inside the lockup's mark shift (7.632, 8.010). */
    static const double blk[3][2] = {{0,0},{-2.5,-2},{2.5,-2}};
    /* \im@lg@letters upright outlines (I N I T E -- C below -- H). */
    static const double I1[4][2] = {{0,0},{0.763,0},{0.763,2.536},{0,2.536}};
    static const double N1[10][2] = {{1.161,0},{1.908,0},{1.908,1.407},{2.668,0},{3.435,0},
        {3.435,2.536},{2.687,2.536},{2.687,1.145},{1.956,2.536},{1.161,2.536}};
    static const double I2[4][2] = {{3.848,0},{4.611,0},{4.611,2.536},{3.848,2.536}};
    static const double T3[8][2] = {{5.549,0},{6.325,0},{6.325,1.884},{7.025,1.884},
        {7.025,2.536},{4.863,2.536},{4.863,1.884},{5.549,1.884}};
    static const double E1[12][2] = {{7.314,0},{9.413,0},{9.413,0.660},{8.078,0.660},
        {8.078,0.986},{9.286,0.986},{9.286,1.582},{8.078,1.582},{8.078,1.884},
        {9.398,1.884},{9.398,2.536},{7.314,2.536}};
    static const double H1[12][2] = {{12.482,0},{13.246,0},{13.246,1.018},{13.961,1.018},
        {13.961,0},{14.740,0},{14.740,2.536},{13.961,2.536},{13.961,1.654},
        {13.246,1.654},{13.246,2.536},{12.482,2.536}};

    W = (int)ceil(LOCK_W * U);
    H = (int)ceil(LOCK_H * U);
    if (W > 256 || H > 256) die("strike too large");
    /* Snap the mark's centre line to a pixel boundary: every vertical block
     * edge (X = 7.632 + k/2) then falls BETWEEN pixel columns (crisp edges, no
     * centre ever on an edge). Rows are centred in the strike. */
    OX = floor(7.632 * U + 0.5) - 7.632 * U;
    OY = (H - LOCK_H * U) / 2.0;

    for (int b = 0; b < 3; b++) {
        double sx = 7.632 + blk[b][0], sy = 8.010 + blk[b][1];
        face(L1, 6, sx, sy, LIGHT);
        face(L2, 4, sx, sy, LIGHT);
        face(D1, 6, sx, sy, DARK);
        face(D2, 4, sx, sy, DARK);
        face(T1, 6, sx, sy, TOP);
        face(T2, 4, sx, sy, TOP);
    }
    letter(I1, 4);
    letter(N1, 10);
    letter(I2, 4);
    letter(T3, 8);
    letter(E1, 12);
    /* The C (initech-logo.tex): outer ellipse arc, the lower terminal, the
     * four quarter-ellipse counter arcs and its straight left side. */
    cn = 0;
    c_arc(10.908, 1.272, 1.209, 1.304, 15.563, 347.320);
    c_point(11.385, 0.986);
    c_arc(10.908, 0.986, 0.477, 0.350, 0.0, -90.0);
    c_arc(10.908, 1.018, 0.413, 0.382, 270.0, 180.0);
    c_point(10.495, 1.590);
    c_arc(10.908, 1.590, 0.413, 0.334, 180.0, 90.0);
    c_arc(10.908, 1.622, 0.477, 0.302, 90.0, 0.0);
    fill(cpath, cn, DARK);
    letter(H1, 12);

    /* RLE: one byte per run, code << 6 | (len - 1), len 1..64; runs never
     * cross a row, so every row's runs sum to W. */
    static unsigned char rle[65536];
    int nb = 0;
    for (int j = 0; j < H; j++) {
        int i = 0;
        while (i < W) {
            int k = i;
            while (k < W && img[j][k] == img[j][i] && k - i < 64) k++;
            rle[nb++] = (unsigned char)((img[j][i] << 6) | (k - i - 1));
            i = k;
        }
    }

    printf("/*\n");
    printf(" * spec/assets/boot_logo.h -- GENERATED by tools/boot_logo_gen.c\n");
    printf(" * (make gen-boot-logo). DO NOT EDIT: change the geometry there.\n");
    printf(" *\n");
    printf(" * The Initech logo lockup (three-cube mark + INITECH logotype) for the FLAIR\n");
    printf(" * boot screen (bead initech-tdnl.105), an 8-bit-index strike at u = %d px.\n", U);
    printf(" *\n");
    printf(" * PROVENANCE (Law 1): AUTHORED from the measured geometry -- the TikZ\n");
    printf(" * drawing pubs/common/initech-logo.tex and docs/pubs/DESIGN-REFERENCE.md\n");
    printf(" * Sec 7.0 (2:1 dimetric, receding edges 1 in 2, cube edge 2 with a unit\n");
    printf(" * notch, mark 9u x 6.449u, lockup 15.264u x 10.459u, logotype oblique\n");
    printf(" * 0.207, black-weight extended grotesque outlines). NEVER extracted from\n");
    printf(" * any image. The drift tooth is harness/proptest/test_boot_logo.c, which\n");
    printf(" * re-renders the geometry independently and must equal this strike.\n");
    printf(" *\n");
    printf(" * FORMAT: boot_logo_rle[] -- rows top to bottom; each byte is one run,\n");
    printf(" * (code << 6) | (len - 1), len 1..64; a row's runs sum to BOOT_LOGO_W.\n");
    printf(" * Codes are ROLES (the palette index is the boot screen's policy):\n");
    printf(" *   0 CLEAR (the well shows)  1 TOP (cube tops + notch floors)\n");
    printf(" *   2 DARK (left faces + logotype)  3 LIGHT (right faces)\n");
    printf(" */\n");
    printf("#ifndef INITECH_BOOT_LOGO_H\n#define INITECH_BOOT_LOGO_H\n\n");
    printf("#include <stdint.h>\n\n");
    printf("#define BOOT_LOGO_U          %d\n", U);
    printf("#define BOOT_LOGO_W          %d\n", W);
    printf("#define BOOT_LOGO_H          %d\n", H);
    printf("#define BOOT_LOGO_CODE_CLEAR 0\n");
    printf("#define BOOT_LOGO_CODE_TOP   1\n");
    printf("#define BOOT_LOGO_CODE_DARK  2\n");
    printf("#define BOOT_LOGO_CODE_LIGHT 3\n");
    printf("#define BOOT_LOGO_RLE_BYTES  %d\n\n", nb);
    printf("static const uint8_t boot_logo_rle[BOOT_LOGO_RLE_BYTES] = {\n");
    for (int i = 0; i < nb; i++) {
        printf("%s0x%02X,%s", (i % 12) ? " " : "    ", rle[i],
               (i % 12 == 11 || i == nb - 1) ? "\n" : "");
    }
    printf("};\n\n#endif /* INITECH_BOOT_LOGO_H */\n");
    return 0;
}
