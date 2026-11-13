/* M009 independent per-cell oracle (PRD Sec 6.3; CLAUDE.md Laws 1/2).
 * Ref: system7-decomp/goldens/captures/s8_alert_modal.png and
 * s8_desktop_menubar.png, identical wallpaper patch [229,28,245,44).
 * Subtract the cursor-free capture: unchanged = clear, black = ink,
 * white = mask-only paper. No artifact assets or palette tables enter here.
 * Paths are explicit factory arguments; missing/malformed captures FAIL.
 */
#ifndef INITECH_HARNESS_CURSOR_CAPTURE_H
#define INITECH_HARNESS_CURSOR_CAPTURE_H

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char *cursor_capture_png(const char *path)
{
    png_image im;
    unsigned char *pixels;
    memset(&im, 0, sizeof im);
    im.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&im, path)) {
        fprintf(stderr, "FAIL independent cursor capture: %s: %s\n", path, im.message);
        png_image_free(&im);
        return NULL;
    }
    im.format = PNG_FORMAT_RGB;
    if (im.width != 640 || im.height != 480) {
        fprintf(stderr, "FAIL independent cursor capture dimensions: %s\n", path);
        png_image_free(&im);
        return NULL;
    }
    pixels = malloc(PNG_IMAGE_SIZE(im));
    if (!pixels || !png_image_finish_read(&im, NULL, pixels, 0, NULL)) {
        fprintf(stderr, "FAIL independent cursor capture decode: %s\n", path);
        free(pixels);
        pixels = NULL;
    }
    png_image_free(&im);
    return pixels;
}

static int cursor_capture_cells(const char *arrow, const char *wallpaper,
                                char cells[256])
{
    unsigned char *a = cursor_capture_png(arrow);
    unsigned char *b = cursor_capture_png(wallpaper);
    int ink = 0, paper = 0, clear = 0, bad = 0;
    if (!a || !b) { free(a); free(b); return 1; }
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            const unsigned char *p = a + 3 * ((28 + y) * 640 + 229 + x);
            const unsigned char *q = b + 3 * ((28 + y) * 640 + 229 + x);
            char cell = '?';
            if (!memcmp(p, q, 3)) { cell = '.'; clear++; }
            else if (p[0] == 0 && p[1] == 0 && p[2] == 0) { cell = '#'; ink++; }
            else if (p[0] == 255 && p[1] == 255 && p[2] == 255) { cell = 'o'; paper++; }
            else bad++;
            cells[y * 16 + x] = cell;
        }
    }
    free(a);
    free(b);
    /* Audit REPORT.md M009: population check also detects a wrong crop or
     * changed wallpaper instead of silently accepting it as transparency. */
    if (bad || ink != 54 || paper != 43 || clear != 159 || cells[17] != '#') {
        fprintf(stderr, "FAIL independent cursor crop: ink=%d paper=%d clear=%d invalid=%d\n",
                ink, paper, clear, bad);
        return 1;
    }
    return 0;
}

#endif
