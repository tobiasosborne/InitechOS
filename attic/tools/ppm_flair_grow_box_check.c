/* h6y1 factory oracle. The expectation is decoded from the independent
 * Mac OS 8.1 PNG, never from chrome_metrics or the artifact's colour table.
 * Ref: sys8/window-chrome.md Sec 5 (461,296,18,18); DEC-10.
 * Locked geometry: spec/flair_grow_box_trace.mk. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <png.h>

static unsigned char pixels[640 * 480 * 3];

static uint32_t rgb(const unsigned char *p)
{
    return (uint32_t)p[0] * 65536u + (uint32_t)p[1] * 256u + p[2];
}

static uint32_t pixel(int x, int y)
{
    return rgb(pixels + 3 * (y * 640 + x));
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: ppm_flair_grow_box_check dump.ppm independent-active.png\n");
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    int w, h, max;
    if (!f || fscanf(f, "P6 %d %d %d", &w, &h, &max) != 3 ||
        w != 640 || h != 480 || max != 255 || fgetc(f) == EOF ||
        fread(pixels, 1, sizeof pixels, f) != sizeof pixels) {
        fprintf(stderr, "FAIL grow-box: missing/malformed 640x480 PPM\n");
        if (f) fclose(f);
        return 2;
    }
    fclose(f);
    png_image im;
    memset(&im, 0, sizeof im);
    im.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&im, argv[2])) {
        fprintf(stderr, "FAIL grow-box: missing independent PNG\n");
        return 2;
    }
    im.format = PNG_FORMAT_RGB;
    unsigned char *gold = malloc(PNG_IMAGE_SIZE(im));
    if (!gold || !png_image_finish_read(&im, NULL, gold, 0, NULL) ||
        im.width != 640 || im.height != 480) {
        fprintf(stderr, "FAIL grow-box: independent PNG decode/dimensions\n");
        free(gold);
        png_image_free(&im);
        return 2;
    }
    int bad = 0;
    for (int y = 0; y < 18; y++)
        for (int x = 0; x < 18; x++)
            bad += pixel(420 + x, 320 + y) !=
                   rgb(gold + 3 * ((296 + y) * 640 + 461 + x));
    free(gold);
    png_image_free(&im);
    if (bad) fprintf(stderr, "FAIL grow-box active crop: %d/324 pixels differ\n", bad);

    /* All four endpoints and the new body must agree with the committed
     * (40,80)..(440,340) frame, independently of the proposed-size marker.
     * Body/frame/shadow cross-sections: window-chrome.md Sec 1/4.
     * Empty new right strip: finder_windows.h Sec 2/3 grid (four columns). */
    int frame = pixel(120, 80) != 0 || pixel(40, 200) != 0 ||
                pixel(439, 200) != 0 || pixel(120, 339) != 0 ||
                pixel(440, 200) != 0 || pixel(120, 340) != 0 ||
                pixel(399, 310) != 0xFFFFFFu;
    if (frame) fprintf(stderr, "FAIL grow-box committed frame: expected (40,80)..(440,340)\n");
    if (bad || frame) return 1;
    printf("grow-box PASS: 324/324 independent PNG pixels; committed400x260 frame\n");
    return 0;
}
