/* Independent runtime oracle input for tdnl.99/.100 and L023 (Law 2).
 * Ref: ../system7-decomp/goldens/captures/s8_alert_modal.png, 640x480;
 * sys8/controls.md Sec 5.1 and 6. Never include artifact drawing tables here.
 */
#ifndef INITECH_HARNESS_ALERT_CAPTURE_H
#define INITECH_HARNESS_ALERT_CAPTURE_H

#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char *alert_capture_load_path(const char *path)
{
    png_image im;
    unsigned char *pixels;
    memset(&im, 0, sizeof im);
    im.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&im, path)) {
        fprintf(stderr, "FAIL independent alert capture: %s: %s\n", path, im.message);
        png_image_free(&im);
        return NULL;
    }
    im.format = PNG_FORMAT_RGB;
    if (im.width != 640 || im.height != 480) {
        fprintf(stderr, "FAIL independent alert capture dimensions: %s\n", path);
        png_image_free(&im);
        return NULL;
    }
    pixels = malloc(PNG_IMAGE_SIZE(im));
    if (!pixels || !png_image_finish_read(&im, NULL, pixels, 0, NULL)) {
        fprintf(stderr, "FAIL independent alert capture decode: %s\n", path);
        free(pixels);
        pixels = NULL;
    }
    png_image_free(&im);
    return pixels;
}

static unsigned char *alert_capture_load(void)
{
    return alert_capture_load_path("../system7-decomp/goldens/captures/s8_alert_modal.png");
}

static uint32_t alert_capture_rgb(const unsigned char *pixels, int x, int y)
{
    const unsigned char *p = pixels + 3 * (y * 640 + x);
    return (uint32_t)p[0] * 65536u + (uint32_t)p[1] * 256u + p[2];
}

/* tdnl.88: the Mac OS 8.1 PRESSED default OK (system7-decomp
 * re/mint-results-010.md). Two real captures of the same held button with
 * the Mac pointer at two DISJOINT places: A = s8_alert_ok_pressed.png
 * (hotspot 464,168), B = s8_alert_ok_pressed_tl.png (hotspot 440,161). The
 * pointer-free pressed button is A with A's pointer box taken from B. The
 * boxes are the measured footprints plus a 1-px margin (mint-010 Sec 0). */
#define PRESSED_A_BOX_X0 462
#define PRESSED_A_BOX_X1 474
#define PRESSED_A_BOX_Y0 166
#define PRESSED_A_BOX_Y1 183
#define PRESSED_B_BOX_X0 438
#define PRESSED_B_BOX_X1 450
#define PRESSED_B_BOX_Y0 159
#define PRESSED_B_BOX_Y1 176
/* The graded window: the OK button, its 2-px moat and its default ring. */
#define PRESSED_X0 432
#define PRESSED_X1 496
#define PRESSED_Y0 155
#define PRESSED_Y1 180
/* The label zone: inside the pressed rims and their corner steps, where the native (Charcoal) and
 * FLAIR (Chicago) glyph shapes may differ; only the two inks are allowed. */
#define PRESSED_LBL_X0 440
#define PRESSED_LBL_X1 488
#define PRESSED_LBL_Y0 162
#define PRESSED_LBL_Y1 173

static __attribute__((unused)) int pressed_in_box(int x, int y, int x0, int x1, int y0, int y1)
{
    return x >= x0 && x <= x1 && y >= y0 && y <= y1;
}

/* Compose the pointer-free pressed golden into `out` (640x480 RGB, a copy of
 * A). Returns the number of pixels of the alert (x133..507, y87..191) where
 * A and B disagree OUTSIDE both pointer boxes: must be 0, or the pair is not
 * two captures of one state and nothing may be graded against it. */
static __attribute__((unused)) int pressed_capture_compose(const unsigned char *a,
                                   const unsigned char *b,
                                   unsigned char *out)
{
    int bad = 0;
    memcpy(out, a, (size_t)640 * 480 * 3);
    for (int y = 87; y <= 191; y++)
        for (int x = 133; x <= 507; x++) {
            int ia = pressed_in_box(x, y, PRESSED_A_BOX_X0, PRESSED_A_BOX_X1,
                                    PRESSED_A_BOX_Y0, PRESSED_A_BOX_Y1);
            int ib = pressed_in_box(x, y, PRESSED_B_BOX_X0, PRESSED_B_BOX_X1,
                                    PRESSED_B_BOX_Y0, PRESSED_B_BOX_Y1);
            if (ia) memcpy(out + 3 * (y * 640 + x), b + 3 * (y * 640 + x), 3);
            else if (!ib) bad += alert_capture_rgb(a, x, y) != alert_capture_rgb(b, x, y);
        }
    return bad;
}

#endif
