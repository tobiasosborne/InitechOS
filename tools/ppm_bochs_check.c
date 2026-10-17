/* ppm_bochs_check -- grade a Bochs capture against a REAL QEMU QMP capture of
 * the same scene (initech-u9gf; WL-0170; PRD Sec 8 / Rule 5).
 *
 * The golden is independent of the artifact and of Bochs: the 640x480 P6
 * screendump that test-flair-desktop-8bpp takes on QEMU (whose 8bpp capture
 * is graded equal to the 32bpp one there). Bochs supplies two captures of the
 * one stable frame (harness/emu/bochs.c):
 *
 * 1. EXACT: <name>.snap.ppm, converted from the BMP Bochs's own header-bar
 *    snapshot button writes (gui.cc snapshot_handler: the guest's raw 8bpp
 *    indices read through the VBE line offset, plus gui.cc's palette, which
 *    vgacore.cc fills from the emulated DAC shifted by dac_shift). Graded
 *    for EXACT RGB equality on all 307200 pixels, the bar QEMU's own
 *    32bpp == 8bpp gate sets.
 * 2. DISPLAYED: <name>.rfb.pgm, what the RFB display actually served.
 *
 * Bochs 2.7 can only SHOW the guest through BGR233 (Bochs source, tag
 * REL_2_7_FINAL): gui/rfb.cc ServerInit sends BGR233Format; for an 8bpp VBE
 * mode iodev/display/vga.cc update() converts each guest index through the
 * guest DAC with MAKE_COLOUR (iodev/display/vgacore.h: plain shifts) into
 * rfb.cc graphics_tile_info (red_shift 3 mask 07h, green_shift 6 mask 38h,
 * blue_shift 8 mask C0h). With the 8-bit DAC the guest negotiated, the code a
 * pixel of 8-bit colour (R,G,B) must show is therefore
 *     (R >> 5) | ((G >> 5) << 3) | ((B >> 6) << 6).
 * The guest picture sits at x=0, y=rfbHeaderbarY in the rfbWindowX x
 * (480 + rfbHeaderbarY + rfbStatusbarY) window, rfbStatusbarY == 18
 * (rfb.cc specific_init / dimension_update). This is the strongest grade the
 * emulator's only headless display admits: lossy by 3/3/2 bits, but EXACT on
 * every one of the 307200 pixels -- geometry, stride and DAC programming
 * errors that move a colour across a 3/3/2 cell all go RED.
 *
 * usage: ppm_bochs_check <golden.ppm 640x480 P6> <bochs.snap.ppm P6> <bochs.rfb.pgm P5>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GW 640u
#define GH 480u
#define STATUSBAR_Y 18u

/* Minimal netpbm header reader (no comments inside the header after line 1
 * except whole '#' lines, which both writers never emit). */
static unsigned char *read_pnm(const char *path, char kind, unsigned *w,
                               unsigned *h, unsigned ch)
{
    FILE *f = fopen(path, "rb");
    char line[160];
    int max;
    unsigned char *p = NULL;
    if (!f) return NULL;
    if (!fgets(line, sizeof line, f) || line[0] != 'P' || line[1] != kind) goto bad;
    do {
        if (!fgets(line, sizeof line, f)) goto bad;
    } while (line[0] == '#');
    if (sscanf(line, "%u %u", w, h) != 2 || *w == 0 || *h == 0 || *w > 4096 || *h > 4096)
        goto bad;
    if (!fgets(line, sizeof line, f) || sscanf(line, "%d", &max) != 1 || max != 255) goto bad;
    size_t n = (size_t)*w * *h * ch;
    p = malloc(n);
    if (!p || fread(p, 1, n, f) != n || fgetc(f) != EOF) goto bad;
    fclose(f);
    return p;
bad:
    free(p);
    fclose(f);
    return NULL;
}

static int grade_exact(const unsigned char *g, const char *path)
{
    unsigned w, h;
    unsigned char *s = read_pnm(path, '6', &w, &h, 3);
    if (!s || w != GW || h != GH) {
        fprintf(stderr, "RED bochs-exact invalid/missing 640x480 snapshot %s\n", path);
        free(s);
        return 2;
    }
    unsigned long bad = 0;
    for (unsigned i = 0; i < GW * GH; i++) {
        const unsigned char *a = g + (size_t)i * 3, *b = s + (size_t)i * 3;
        if (a[0] != b[0] || a[1] != b[1] || a[2] != b[2]) {
            if (!bad)
                fprintf(stderr, "RED bochs-exact RGB (%u,%u) qemu=#%02X%02X%02X bochs=#%02X%02X%02X\n",
                        i % GW, i / GW, a[0], a[1], a[2], b[0], b[1], b[2]);
            bad++;
        }
    }
    free(s);
    if (bad) {
        fprintf(stderr, "RED bochs-exact %lu/%u pixels differ\n", bad, GW * GH);
        return 1;
    }
    printf("GREEN bochs-exact: all %u RGB pixels of the Bochs snapshot equal the QEMU golden\n",
           GW * GH);
    return 0;
}

static int grade_rfb(const unsigned char *g, const char *path)
{
    unsigned bw, bh;
    unsigned char *b = read_pnm(path, '5', &bw, &bh, 1);
    if (!b) {
        fprintf(stderr, "RED bochs-rfb invalid/missing RFB capture %s\n", path);
        return 2;
    }
    if (bw < GW || bh < GH + STATUSBAR_Y) {
        fprintf(stderr, "RED bochs-rfb window %ux%u cannot hold a 640x480 guest\n", bw, bh);
        free(b);
        return 2;
    }
    unsigned top = bh - GH - STATUSBAR_Y;    /* rfbHeaderbarY */
    unsigned long bad = 0;
    unsigned seen[256];
    memset(seen, 0, sizeof seen);
    for (unsigned y = 0; y < GH; y++) {
        for (unsigned x = 0; x < GW; x++) {
            const unsigned char *px = g + ((size_t)y * GW + x) * 3;
            unsigned want = (px[0] >> 5) | ((px[1] >> 5) << 3) | ((px[2] >> 6) << 6);
            unsigned got = b[(size_t)(top + y) * bw + x];
            seen[got] = 1;
            if (got != want) {
                if (!bad)
                    fprintf(stderr,
                            "RED bochs-rfb BGR233 (%u,%u) qemu=#%02X%02X%02X want=0x%02X bochs=0x%02X\n",
                            x, y, px[0], px[1], px[2], want, got);
                bad++;
            }
        }
    }
    free(b);
    unsigned distinct = 0;
    for (unsigned i = 0; i < 256; i++) distinct += seen[i];
    if (bad) {
        fprintf(stderr, "RED bochs-rfb %lu/%u pixels differ\n", bad, GW * GH);
        return 1;
    }
    printf("GREEN bochs-rfb: all %u displayed pixels equal the QEMU golden through Bochs "
           "BGR233 (window %ux%u, guest at y=%u, %u distinct codes)\n",
           GW * GH, bw, bh, top, distinct);
    return 0;
}

int main(int argc, char **argv)
{
    unsigned gw, gh;
    if (argc != 4) {
        fprintf(stderr, "usage: %s <golden.ppm> <bochs.snap.ppm> <bochs.rfb.pgm>\n", argv[0]);
        return 2;
    }
    unsigned char *g = read_pnm(argv[1], '6', &gw, &gh, 3);
    if (!g || gw != GW || gh != GH) {
        fprintf(stderr, "RED bochs-check invalid/missing 640x480 QEMU golden %s\n", argv[1]);
        free(g);
        return 2;
    }
    /* Both grades always run (a RED in one must not hide the other). */
    int e = grade_exact(g, argv[2]);
    int r = grade_rfb(g, argv[3]);
    free(g);
    return e > r ? e : r;
}
