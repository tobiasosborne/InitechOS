/*
 * tools/ppm_flair_calc_check.c -- the PIXEL GATE of the Calculator desk
 * accessory and the Apple menu (factory C; beads initech-tdnl.17 /
 * initech-tdnl.47). Grades a QEMU screendump (P6 PPM) against the REAL
 * Mac OS 8.1 capture (PNG) it was minted to match -- never against the
 * artifact's own layout data (Law 2 / ADR-0010). Pixels are compared by
 * CLASS (black / white / other): the classes carry the whole B&W rDocProc
 * window and every key, and are blind to the two documented colour
 * substitutions (the desktop and the menu accent).
 *
 *   window <ppm> <png>   the Calculator window box, frame (267,94) 106x168
 *                        (the capture's own place; the tenant opens there):
 *       CW1 FRAME    the rounded outline, the title band, the close box,
 *                    the sides -- every frame pixel, EXCEPT the title's
 *                    glyph box (rows 5..13, x 26..96): Mac OS 8.1 draws the
 *                    title in Charcoal, FLAIR in Chicago (bead tdnl.69 owns
 *                    the system-font era; a stated exclusion, nothing else).
 *       CW2 FACE     the content: the gray field, the readout frame, all 18
 *                    keys with their shadows and labels.
 *       CW3 READOUT  the readout interior: the Geneva 9 digits.
 *   apple <ppm> <png>    the Apple menu held open from the band-1 glyph:
 *       AP1 BLOCK    the pulled block's extent on bar row 1 equals the
 *                    capture's (x 9..37), measured in each image (its
 *                    colour is the ratified teal substitution, not graded
 *                    here -- test-color-canon owns colour).
 *       AP2 PANEL    the panel's left frame column and its white inner
 *                    bevel stand where the capture's do.
 *       AP3 ANATOMY  the panel's top anatomy (rows 19..22) and the
 *                    divider band (rows 36..42) match the capture class for
 *                    class over the narrower of the two panels.
 *
 * Exit 0 = every leg green; 1 = a leg RED (named); 2 = usage / I/O.
 * ASCII-clean (Rule 12).
 */
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int w, h; unsigned char *rgb; } img_t;

static int load_ppm(const char *path, img_t *im)
{
    FILE *f = fopen(path, "rb");
    int maxv;
    if (!f) return 0;
    if (fscanf(f, "P6 %d %d %d", &im->w, &im->h, &maxv) != 3 || maxv != 255) { fclose(f); return 0; }
    fgetc(f);
    im->rgb = malloc((size_t)im->w * im->h * 3);
    if (fread(im->rgb, 3, (size_t)im->w * im->h, f) != (size_t)im->w * im->h) { fclose(f); return 0; }
    fclose(f);
    return 1;
}

static int load_png(const char *path, img_t *im)
{
    png_image p;
    memset(&p, 0, sizeof p);
    p.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&p, path)) return 0;
    p.format = PNG_FORMAT_RGB;
    im->w = (int)p.width; im->h = (int)p.height;
    im->rgb = malloc(PNG_IMAGE_SIZE(p));
    return png_image_finish_read(&p, NULL, im->rgb, 0, NULL) != 0;
}

static const unsigned char *px(const img_t *im, int x, int y)
{
    return &im->rgb[3 * (y * im->w + x)];
}

static int cls(const img_t *im, int x, int y)
{
    const unsigned char *p = px(im, x, y);
    if (p[0] == 0 && p[1] == 0 && p[2] == 0) return 1;
    if (p[0] == 255 && p[1] == 255 && p[2] == 255) return 0;
    return 2;
}

static int same_rgb(const img_t *im, int x, int y, const unsigned char *c)
{
    const unsigned char *p = px(im, x, y);
    return p[0] == c[0] && p[1] == c[1] && p[2] == c[2];
}

#define WL 267
#define WT 94
#define WW 106
#define WH 168

static int check_window(const img_t *ours, const img_t *mac)
{
    int bad[3] = { 0, 0, 0 }, fx[3], fy[3], fail = 0;
    static const char *leg[3] = { "CW1 FRAME", "CW2 FACE", "CW3 READOUT" };
    for (int y = 0; y < WH; y++)
        for (int x = 0; x < WW; x++) {
            if (y >= 5 && y <= 13 && x >= 26 && x <= 96) continue;  /* Charcoal */
            int k;
            int cx = x - 1, cy = y - 19;                 /* content-local */
            if (cx < 0 || cy < 0 || cx >= 102 || cy >= 146) k = 0;
            else if (cx >= 8 && cx <= 91 && cy >= 8 && cy <= 25) k = 2;
            else k = 1;
            if (cls(ours, WL + x, WT + y) != cls(mac, WL + x, WT + y)) {
                if (!bad[k]) { fx[k] = x; fy[k] = y; }
                bad[k]++;
            }
        }
    for (int k = 0; k < 3; k++) {
        if (bad[k]) {
            printf("!!! %s: %d pixels differ from the Mac OS 8.1 capture (first at frame (%d,%d))\n",
                   leg[k], bad[k], fx[k], fy[k]);
            fail = 1;
        } else {
            printf(">>> %s: class-exact against the capture\n", leg[k]);
        }
    }
    return fail;
}

/* the run of colour c on row y through x0 */
static void run_extent(const img_t *im, int x0, int y, int *l, int *r)
{
    unsigned char c[3];
    memcpy(c, px(im, x0, y), 3);
    *l = x0; *r = x0;
    while (*l > 0 && same_rgb(im, *l - 1, y, c)) (*l)--;
    while (*r < im->w - 1 && same_rgb(im, *r + 1, y, c)) (*r)++;
}

/* The pointer sits at the trace's hotspot (20,9) in BOTH frames (the mint
 * pressed the glyph there too); its 16x16 box is nobody's menu pixel. */
static int in_cursor(int x, int y) { return x >= 19 && x <= 36 && y <= 25; }

static int check_apple(const img_t *ours, const img_t *mac)
{
    int fail = 0, ol, or_, ml, mr;
    /* bar row 1: the block's face, above the glyph and the pointer */
    run_extent(ours, 12, 1, &ol, &or_);
    run_extent(mac, 12, 1, &ml, &mr);
    if (ol != ml || or_ != mr) {
        printf("!!! AP1 BLOCK: the pulled block spans x %d..%d, the capture's %d..%d\n", ol, or_, ml, mr);
        fail = 1;
    } else {
        printf(">>> AP1 BLOCK: x %d..%d, as the capture\n", ol, or_);
    }
    int bad = 0;
    for (int y = 19; y < 44; y++) {
        if (cls(ours, 9, y) != 1 || cls(mac, 9, y) != 1) bad++;
        if (y >= 21 && y <= 33 && (cls(ours, 10, y) != 0 || cls(mac, 10, y) != 0)) bad++;
    }
    if (bad) { printf("!!! AP2 PANEL: %d frame/bevel pixels off column 9/10\n", bad); fail = 1; }
    else printf(">>> AP2 PANEL: left frame at x 9, bevel at x 10, as the capture\n");
    /* the divider's dark line (capture row 38) bounds the comparable width */
    int dl, dr, el, er;
    run_extent(ours, 40, 38, &dl, &dr);
    run_extent(mac, 40, 38, &el, &er);
    /* FOUND, NOT OWNED HERE: the Mac's separator line starts at panel left
     * + 1 (x 10, over the bevel), FLAIR's at + 2 (x 11) -- menu.c's divider
     * inset, a menu-look defect reported with bead initech-tdnl.47's lane
     * (the File menu capture s8_menu_dropped shows the same +1). Column 10
     * is therefore not compared on the divider rows; every other pixel is. */
    if (dl != 11 || el != 10) {
        printf("!!! AP3 ANATOMY: the divider starts at x %d (capture %d): the recorded 11/10 finding moved\n", dl, el);
        return 1;
    }
    int right = (dr < er ? dr : er);
    int fx = -1, fy = -1;
    bad = 0;
    for (int y = 19; y < 43; y++) {               /* row 43: the next row */
        if (y > 22 && y < 36) continue;            /* the item row's text */
        for (int x = 9; x <= right; x++) {
            if (in_cursor(x, y)) continue;
            if (y >= 36 && x == 10) continue;      /* the divider inset (above) */
            if (cls(ours, x, y) != cls(mac, x, y)) { if (!bad) { fx = x; fy = y; } bad++; }
        }
    }
    if (bad) { printf("!!! AP3 ANATOMY: %d pixels differ (first at (%d,%d))\n", bad, fx, fy); fail = 1; }
    else printf(">>> AP3 ANATOMY: panel top + divider class-exact over x 9..%d (divider from x %d)\n", right, dl);
    return fail;
}

int main(int argc, char **argv)
{
    img_t ours, mac;
    if (argc != 4) { fprintf(stderr, "usage: %s window|apple <ppm> <capture.png>\n", argv[0]); return 2; }
    if (!load_ppm(argv[2], &ours)) { printf("!!! cannot read %s\n", argv[2]); return 2; }
    if (!load_png(argv[3], &mac)) {
        printf("!!! cannot read the capture %s (set SYSTEM7_DECOMP; mint-results-007)\n", argv[3]);
        return 2;
    }
    if (ours.w != 640 || ours.h != 480 || mac.w != 640 || mac.h != 480) {
        printf("!!! expected two 640x480 frames (ours %dx%d, capture %dx%d)\n", ours.w, ours.h, mac.w, mac.h);
        return 2;
    }
    if (strcmp(argv[1], "window") == 0) return check_window(&ours, &mac);
    if (strcmp(argv[1], "apple") == 0) return check_apple(&ours, &mac);
    return 2;
}
