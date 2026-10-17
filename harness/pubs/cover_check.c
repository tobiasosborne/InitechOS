/* cover_check.c -- the identity leg of test-pubs-families (PUB-REF-0001
 * D-8 as revised, PK-01 Initech Cyan, PK-02 logo cell and line band;
 * worklog WL-0132).
 *
 * Reads a binary PPM (P6) rendered by pdftoppm from a cover PDF and grades
 * one rectangle of it, given in millimetres from the top-left trim corner:
 *
 *   cover_check <ppm> <dpi> logo <x> <y> <w> <h>   the logo cell: the three
 *       logo inks must all be present -- Initech Cyan cube tops, near-black
 *       left faces and letters, light-grey right faces;
 *   cover_check <ppm> <dpi> band <x> <y> <w> <h>   the line band: the
 *       rectangle must be Initech Cyan almost throughout.
 *
 * INDEPENDENT GOLDEN (Law 2): the expected cyan is the colour MEASURED on
 * the cube tops of the clean logo still in the private identity library
 * (median of nine samples, RGB 87/177/159, #57B19F), written here as a
 * constant.  The artifact carries its own, separately derived CMYK press
 * build (initech-manual.cls InitechCyan); the gate never reads the class.
 * A cover printed with any other ink -- the desktop teal #8DDCDC, the old
 * family colour, a mistyped CMYK -- renders outside the tolerance and goes
 * RED.  Tolerance: 10 per channel, for the proof renderer's CMYK-to-RGB
 * conversion and anti-aliasing (poppler 24.02 renders the press build as
 * 87/179/160).
 *
 * Exit 0 and a ">>>" line when green; exit 1 and a "!!! test-pubs RED
 * COVER-LOGO / COVER-CYAN / COVER-LINE:" line otherwise.  C only (Law 3). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CYAN_R 87
#define CYAN_G 177
#define CYAN_B 159
#define TOL 10

static int skipws(FILE *f)
{
    int c;
    do {
        c = fgetc(f);
        if (c == '#') while (c != '\n' && c != EOF) c = fgetc(f);
    } while (c == ' ' || c == '\n' || c == '\r' || c == '\t');
    return c;
}
static long readnum(FILE *f)
{
    int c = skipws(f);
    long v = 0;
    if (c < '0' || c > '9') return -1;
    while (c >= '0' && c <= '9') { v = v * 10 + (c - '0'); c = fgetc(f); }
    return v;
}
static int near(int a, int b) { return a - b <= TOL && b - a <= TOL; }

int main(int argc, char **argv)
{
    if (argc != 8 || (strcmp(argv[3], "logo") && strcmp(argv[3], "band"))) {
        fprintf(stderr, "usage: cover_check <ppm> <dpi> logo|band <x> <y> <w> <h> (mm)\n");
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { printf("!!! test-pubs RED COVER-LOGO: cannot open %s\n", argv[1]); return 1; }
    if (fgetc(f) != 'P' || fgetc(f) != '6') { printf("!!! test-pubs RED COVER-LOGO: %s is not a P6 PPM\n", argv[1]); return 1; }
    long w = readnum(f), h = readnum(f), max = readnum(f);
    if (w <= 0 || h <= 0 || max != 255) { printf("!!! test-pubs RED COVER-LOGO: bad PPM header in %s\n", argv[1]); return 1; }
    unsigned char *px = malloc((size_t)(w * h * 3));
    if (!px || fread(px, 1, (size_t)(w * h * 3), f) != (size_t)(w * h * 3)) {
        printf("!!! test-pubs RED COVER-LOGO: short PPM %s\n", argv[1]); return 1; }
    fclose(f);
    double dpi = atof(argv[2]), mm = dpi / 25.4;
    long x0 = (long)(atof(argv[4]) * mm), y0 = (long)(atof(argv[5]) * mm);
    long x1 = x0 + (long)(atof(argv[6]) * mm), y1 = y0 + (long)(atof(argv[7]) * mm);
    if (x0 < 0 || y0 < 0 || x1 > w || y1 > h || x1 <= x0 || y1 <= y0) {
        printf("!!! test-pubs RED COVER-LOGO: rectangle outside the page\n"); return 1; }
    long n = 0, cyan = 0, dark = 0, light = 0, teal = 0;
    for (long y = y0; y < y1; y++)
        for (long x = x0; x < x1; x++) {
            unsigned char *p = px + 3 * (y * w + x);
            int r = p[0], g = p[1], b = p[2];
            n++;
            if (near(r, CYAN_R) && near(g, CYAN_G) && near(b, CYAN_B)) cyan++;
            else if (g > r + 25 && b > r + 25) teal++;   /* a cyan-family ink, wrong value */
            if (r < 60 && g < 60 && b < 60) dark++;
            if (r >= 195 && r <= 232 && abs(r - g) <= 8 && abs(g - b) <= 8) light++;
        }
    free(px);
    double pc = 100.0 * cyan / n, pd = 100.0 * dark / n, pl = 100.0 * light / n, pt = 100.0 * teal / n;
    if (!strcmp(argv[3], "band")) {
        if (pc < 90.0) {
            printf("!!! test-pubs RED COVER-LINE: %s: line band %.1f%% Initech Cyan #57B19F (need >= 90%%; other cyan-family ink %.1f%%)\n",
                   argv[1], pc, pt);
            return 1;
        }
        printf(">>> cover_check: line band %.1f%% Initech Cyan (#57B19F +-%d)\n", pc, TOL);
        return 0;
    }
    if (pd < 4.0 || pl < 2.0) {
        printf("!!! test-pubs RED COVER-LOGO: %s: no logo in the publisher-mark cell (dark %.1f%%, light %.1f%%, cyan %.1f%%)\n",
               argv[1], pd, pl, pc);
        return 1;
    }
    if (pc < 4.0) {
        printf("!!! test-pubs RED COVER-CYAN: %s: logo cube tops are not Initech Cyan #57B19F (cyan %.1f%%, other cyan-family ink %.1f%%)\n",
               argv[1], pc, pt);
        return 1;
    }
    printf(">>> cover_check: logo cell dark %.1f%%, light %.1f%%, Initech Cyan %.1f%% (#57B19F +-%d)\n", pd, pl, pc, TOL);
    return 0;
}
