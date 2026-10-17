/*
 * test_boot_screen.c -- host render of the FLAIR boot screen (bead
 * initech-tdnl.105): runs the SAME pure painter the kernel runs
 * (os/milton/boot_screen.h) through a PPM sink, mapping each index through the
 * ONE runtime index->RGB accessor (spec/assets/color_canon.h flair_canon_rgb,
 * the 24/32bpp LFB path's mapping). The written PPM is graded by
 * tools/ppm_flair_boot_screen_check.c against the independent Mac OS 8.1
 * capture s8_startup_welcome.png (make test-boot-screen). Built with
 * -DFLAIR_BOOT_MUT_NO_LOGO it is the host twin of the emu NO_LOGO mutant.
 *
 * Usage: test_boot_screen <out.ppm>
 * Ref: CLAUDE.md Law 2 (graded by an independent golden), Rule 12 (ASCII).
 */
#include <stdio.h>
#include <stdint.h>

#include "color_canon.h"   /* flair_canon_rgb (-Ispec/assets) */
#include "boot_screen.h"   /* the painter under test (-Ios/milton) */

static int g_rows;

static void ppm_sink(void *ctx, int y, const uint8_t *row)
{
    FILE *f = (FILE *)ctx;
    (void)y;
    for (int x = 0; x < BOOT_SCREEN_W; x++) {
        uint32_t c = flair_canon_rgb(row[x]);
        fputc((int)((c >> 16) & 0xFFu), f);
        fputc((int)((c >> 8) & 0xFFu), f);
        fputc((int)(c & 0xFFu), f);
    }
    g_rows++;
}

int main(int argc, char **argv)
{
    FILE *f;
    if (argc != 2) {
        fprintf(stderr, "usage: %s <out.ppm>\n", argv[0]);
        return 2;
    }
    f = fopen(argv[1], "wb");
    if (!f) {
        fprintf(stderr, "test_boot_screen: cannot write %s\n", argv[1]);
        return 2;
    }
    fprintf(f, "P6\n%d %d\n255\n", BOOT_SCREEN_W, BOOT_SCREEN_H);
    boot_screen_render(ppm_sink, f);
    fclose(f);
    if (g_rows != BOOT_SCREEN_H) {
        fprintf(stderr, "test_boot_screen: %d rows emitted, want %d\n", g_rows, BOOT_SCREEN_H);
        return 1;
    }
    printf("test_boot_screen: rendered %dx%d -> %s\n", BOOT_SCREEN_W, BOOT_SCREEN_H, argv[1]);
    return 0;
}
