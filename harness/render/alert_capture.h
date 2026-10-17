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

#endif
