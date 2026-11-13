/*
 * tools/ppm_flair_finder_icons_check.c -- the booted-image Finder icon grader
 * (factory; beads initech-tdnl.53 / tdnl.74 / tdnl.87).
 *
 * Grades 32x32 icon cells of a QEMU screendump (binary PPM P6, 640x480)
 * against the INDEPENDENT mint-008 Mac OS 8.1 captures, decoded at run time
 * by harness/render/icon_capture.h -- cell for cell, ZERO tolerance: an
 * opaque cell must carry the capture's RGB for that kind and state, a clear
 * cell the background the caller names (the desktop teal, a window's white,
 * the alert face). No artifact header is included here (Law 2): the
 * expected values are the PNG bytes plus the stated background.
 *
 * Usage:
 *   ppm_flair_finder_icons_check <captures dir> <dump.ppm> SPEC...
 *   SPEC = KIND:STATE@X,Y/BG     e.g.  TRASH_FULL:UNSEL@584,404/8DDCDC
 *     KIND  VOLUME HARDDISK TRASH TRASH_FULL FOLDER DOC APP CAUTION
 *     STATE UNSEL | SEL (SEL = the selected / drop-target form)
 *     X,Y   the cell's top-left on the screen; BG the clear-cell RGB (hex)
 * Exit 0 iff every SPEC grades 0/1024; each line prints
 *   "<KIND> <STATE> at (X,Y): N/1024 differing cells".
 * Ref: ../system7-decomp/re/mint-results-008.md; CLAUDE.md Law 1/2, Rule 2.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "icon_capture.h"

static unsigned char *read_ppm(const char *path, int *w, int *h)
{
    FILE *f = fopen(path, "rb");
    char magic[3] = { 0 };
    int maxv = 0;
    unsigned char *buf;
    if (!f) { fprintf(stderr, "FAIL: cannot open %s\n", path); return NULL; }
    if (fscanf(f, "%2s %d %d %d", magic, w, h, &maxv) != 4 || strcmp(magic, "P6") != 0 ||
        maxv != 255 || *w <= 0 || *h <= 0) {
        fprintf(stderr, "FAIL: %s is not a binary 8-bit PPM\n", path);
        fclose(f);
        return NULL;
    }
    fgetc(f);
    buf = malloc((size_t)*w * (size_t)*h * 3u);
    if (!buf || fread(buf, 3u, (size_t)*w * (size_t)*h, f) != (size_t)*w * (size_t)*h) {
        fprintf(stderr, "FAIL: %s is truncated\n", path);
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    return buf;
}

static int kind_of(const char *s, size_t n)
{
    for (int k = 0; k < ICAP__KINDS; k++)
        if (strlen(ICAP_KIND_NAME[k]) == n && strncmp(ICAP_KIND_NAME[k], s, n) == 0) return k;
    return -1;
}

int main(int argc, char **argv)
{
    static icap_cell_t cap[ICAP__KINDS];
    unsigned char *frame;
    int w = 0, h = 0, bad = 0;

    if (argc < 4) {
        fprintf(stderr, "usage: %s <captures dir> <dump.ppm> KIND:STATE@X,Y/BG...\n", argv[0]);
        return 2;
    }
    if (icap_load(argv[1], cap)) {
        printf("!!! ppm_flair_finder_icons_check FAIL: the mint-008 golden is unavailable\n");
        return 1;
    }
    frame = read_ppm(argv[2], &w, &h);
    if (!frame) return 1;
    for (int i = 3; i < argc; i++) {
        const char *s = argv[i];
        const char *colon = strchr(s, ':'), *at = strchr(s, '@'), *slash = strchr(s, '/');
        int kind, state, x, y;
        unsigned int bg;
        char label[64];
        if (!colon || !at || !slash || at < colon || slash < at) {
            fprintf(stderr, "FAIL: malformed SPEC '%s'\n", s);
            free(frame);
            return 2;
        }
        kind = kind_of(s, (size_t)(colon - s));
        state = strncmp(colon + 1, "SEL@", 4) == 0 ? ICAP_SEL :
                strncmp(colon + 1, "UNSEL@", 6) == 0 ? ICAP_UNSEL : -1;
        if (kind < 0 || state < 0 || sscanf(at + 1, "%d,%d", &x, &y) != 2 ||
            sscanf(slash + 1, "%x", &bg) != 1 || !cap[kind].known[state]) {
            fprintf(stderr, "FAIL: bad SPEC '%s' (kind/state/position/background, or an "
                    "uncaptured state)\n", s);
            free(frame);
            return 2;
        }
        snprintf(label, sizeof label, "%s %s", ICAP_KIND_NAME[kind], state ? "SEL" : "UNSEL");
        {
            int d = icap_grade_frame(frame, w, h, x, y, &cap[kind], state, bg, label);
            printf("  %s at (%d,%d) on #%06X: %d/1024 differing cells\n", label, x, y, bg, d);
            if (d) bad++;
        }
    }
    free(frame);
    if (bad) {
        printf("!!! ppm_flair_finder_icons_check FAIL: %d icon cell(s) differ from the mint-008 captures\n", bad);
        return 1;
    }
    printf(">>> ppm_flair_finder_icons_check: %d icon cell(s) == the mint-008 captures (zero tolerance)\n",
           argc - 3);
    return 0;
}
