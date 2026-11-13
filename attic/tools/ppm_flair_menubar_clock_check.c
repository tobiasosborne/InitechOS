/* tdnl.13 emu pixel gate: independent capture + original Chicago NFNT.
 * Same golden loader as the host gate, never the artifact's art or formatter. */
#include <stdio.h>
#include <stdint.h>
#include "menubar_clock_oracle.h"
static unsigned char pixels[640 * 480 * 3];
int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: ppm_flair_menubar_clock_check dump.ppm historical.png original.nfnt\n");
        return 2;
    }
    uint32_t want[20][80];
    if (menubar_clock_golden(argv[2], argv[3], want)) return 2;
    FILE *f = fopen(argv[1], "rb");
    int w, h, max;
    if (!f || fscanf(f, "P6 %d %d %d", &w, &h, &max) != 3 ||
        w != 640 || h != 480 || max != 255 || fgetc(f) == EOF ||
        fread(pixels, 1, sizeof pixels, f) != sizeof pixels) {
        if (f) fclose(f);
        fprintf(stderr, "FAIL clock MC5 missing/malformed 640x480 screendump\n");
        return 2;
    }
    fclose(f);
    int bad = 0;
    for (int y = 0; y < 20; y++) for (int x = 0; x < 80; x++) {
        unsigned char *p = pixels + 3 * (y * 640 + 520 + x);
        uint32_t got = (uint32_t)p[0] * 65536u + (uint32_t)p[1] * 256u + p[2];
        if (got != want[y][x]) {
            if (!bad) fprintf(stderr, "FAIL clock MC5 pixel (%d,%d): %06x != independent %06x\n",
                               520 + x, y, got, want[y][x]);
            bad++;
        }
    }
    if (bad) { fprintf(stderr, "FAIL clock MC5: %d/1600 clock-region pixels differ\n", bad); return 1; }
    printf(">>> ppm_flair_menubar_clock_check: green (1600 pixels, captured bar/anchor + original Chicago12)\n");
    return 0;
}
