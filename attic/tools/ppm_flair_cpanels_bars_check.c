/* B05 / initech-c8w7: PRD 6.3, shell.h compositor order, DAS audit L027.
 * Exact complete strips against captures made BEFORE the preference fix.
 * No artifact headers, palettes, font tables or rendering calls are linked.
 * The optional hidden-clock expectation copies the captured blank bar face;
 * every pixel, including the clock's frame and both bar corners, is graded.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

static unsigned char actual[640 * 480 * 3], expected[640 * 40 * 3];

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: %s frame.ppm baseline.png shown|hidden|capture\n", argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    int w, h, max;
    if (!f || fscanf(f, "P6 %d %d %d", &w, &h, &max) != 3 ||
        w != 640 || h != 480 || max != 255 || fgetc(f) != '\n' ||
        fread(actual, 1, sizeof actual, f) != sizeof actual) {
        if (f) fclose(f);
        fprintf(stderr, "FAIL CP_BARS malformed 640x480 PPM\n");
        return 2;
    }
    fclose(f);
    png_image im = {0};
    im.version = PNG_IMAGE_VERSION;
    im.format = PNG_FORMAT_RGB;
    if (!strcmp(argv[3], "capture")) {
        im.width = 640; im.height = 40;
        return png_image_write_to_file(&im, argv[2], 0, actual, 0, NULL) ? 0 : 2;
    }
    if (strcmp(argv[3], "shown") && strcmp(argv[3], "hidden")) return 2;
    if (!png_image_begin_read_from_file(&im, argv[2])) return 2;
    im.format = PNG_FORMAT_RGB;
    if (im.width != 640 || im.height != 40 ||
        !png_image_finish_read(&im, NULL, expected, 0, NULL)) return 2;
    png_image_free(&im);
    if (!strcmp(argv[3], "hidden")) {
        /* The captured x=510 is unobstructed. The clock owns [520,600)x[1,18)
         * (independently documented by menubar_clock_oracle.c / Mac capture).
         * Hiding it restores that face, without removing any graded pixels. */
        for (int y = 1; y < 18; y++) for (int x = 520; x < 600; x++)
            memcpy(expected + (y * 640 + x) * 3, expected + (y * 640 + 510) * 3, 3);
    }
    unsigned bad[2] = {0, 0};
    for (int y = 0; y < 40; y++) for (int x = 0; x < 640; x++) {
        unsigned at = (unsigned)(y * 640 + x) * 3;
        if (memcmp(actual + at, expected + at, 3)) {
            if (!bad[y / 20]) fprintf(stderr, "CP_BARS first diff bar=%d x=%d y=%d\n", y / 20 + 1, x, y);
            bad[y / 20]++;
        }
    }
    if (bad[0] || bad[1]) {
        fprintf(stderr, "FAIL CP_BARS system=%u/12800 foreground=%u/12800\n", bad[0], bad[1]);
        return 1;
    }
    puts("CP_BARS GREEN 25600 pixels (complete system/foreground strips and clock)");
    return 0;
}
