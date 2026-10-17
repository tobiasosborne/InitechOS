/* harness/render/icon_capture.h -- the INDEPENDENT Finder-icon golden
 * (factory only; beads initech-tdnl.53 / tdnl.74 / tdnl.87).
 *
 * Ref: ../system7-decomp/re/mint-results-008.md (mint-008: the Mac OS 8.1
 *      Finder icon captures, their 32x32 cells and the opacity evidence);
 *      harness/render/cursor_capture.h (the WL-0126 pattern this follows);
 *      CLAUDE.md Law 1 (ground truth), Law 2 (an oracle must not read the
 *      artifact it grades), Rule 2 (missing / malformed captures FAIL).
 *
 * WHAT IT DOES: decodes the named s8_icons_*.png captures and recovers, for
 * each icon kind and state, the 32x32 cell the Mac OS 8.1 Finder drew --
 * per cell OPAQUE + RGB, or CLEAR. A cell is opaque iff, in ANY capture of
 * that kind (any state), its pixel differs from the background behind it
 * there: the window content #FFFFFF, the alert face #E7E7E7, or the desktop
 * picture (a 128x128 tile, sampled clean from s8_icons_trash_full_sel.png at
 * (128 + x mod 128, 148 + (y - 148) mod 128); mint-008 Sec 2). Every capture
 * of one state must agree on every opaque cell, or the golden itself FAILS.
 *
 * NO ARTIFACT DATA ENTERS HERE: this header includes no spec/assets strike,
 * no palette table, no os/flair source. The expected values are the PNG
 * bytes. The capture directory is an explicit argument (no environment
 * override); a missing capture FAILS, never skips.
 */
#ifndef INITECH_HARNESS_ICON_CAPTURE_H
#define INITECH_HARNESS_ICON_CAPTURE_H

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    ICAP_VOLUME = 0, ICAP_HARDDISK, ICAP_TRASH, ICAP_TRASH_FULL,
    ICAP_FOLDER, ICAP_DOC, ICAP_APP, ICAP_CAUTION, ICAP__KINDS
};
enum { ICAP_UNSEL = 0, ICAP_SEL = 1, ICAP__STATES = 2 };
enum { ICAP_BG_WALL = 0, ICAP_BG_WHITE, ICAP_BG_FACE };

typedef struct {
    int      known[ICAP__STATES];       /* 1 if the state was captured     */
    unsigned char opaque[32][32];
    unsigned int  rgb[ICAP__STATES][32][32];   /* 0x00RRGGBB, opaque cells */
} icap_cell_t;

typedef struct {
    int kind, state;
    const char *file;
    int x0, y0, bg;
} icap_src_t;

/* mint-008 Sec 2 (re/mint-results-008.md), row for row. */
static const icap_src_t ICAP_SRC[] = {
    { ICAP_VOLUME,     ICAP_UNSEL, "s8_icons_boot.png",            576, 156, ICAP_BG_WALL  },
    { ICAP_VOLUME,     ICAP_UNSEL, "s8_icons_desktop_empty.png",   576, 156, ICAP_BG_WALL  },
    { ICAP_VOLUME,     ICAP_SEL,   "s8_icons_floppy_sel.png",      576, 156, ICAP_BG_WALL  },
    { ICAP_HARDDISK,   ICAP_UNSEL, "s8_icons_desktop_empty.png",   576,  28, ICAP_BG_WALL  },
    { ICAP_HARDDISK,   ICAP_SEL,   "s8_icons_hd_sel.png",          576,  28, ICAP_BG_WALL  },
    { ICAP_TRASH,      ICAP_UNSEL, "s8_icons_desktop_empty.png",   576, 412, ICAP_BG_WALL  },
    { ICAP_TRASH,      ICAP_SEL,   "s8_icons_trash_empty_sel.png", 576, 412, ICAP_BG_WALL  },
    { ICAP_TRASH_FULL, ICAP_UNSEL, "s8_icons_trash_full.png",      576, 412, ICAP_BG_WALL  },
    { ICAP_TRASH_FULL, ICAP_UNSEL, "s8_icons_boot.png",            576, 412, ICAP_BG_WALL  },
    { ICAP_TRASH_FULL, ICAP_SEL,   "s8_icons_trash_full_sel.png",  576, 412, ICAP_BG_WALL  },
    { ICAP_FOLDER,     ICAP_UNSEL, "s8_icons_floppy_window.png",   285,  72, ICAP_BG_WHITE },
    { ICAP_FOLDER,     ICAP_UNSEL, "s8_icons_on_desktop.png",      460, 174, ICAP_BG_WALL  },
    { ICAP_FOLDER,     ICAP_SEL,   "s8_icons_folder_sel.png",      285,  72, ICAP_BG_WHITE },
    { ICAP_FOLDER,     ICAP_SEL,   "s8_icons_desk_folder_sel.png", 460, 174, ICAP_BG_WALL  },
    { ICAP_DOC,        ICAP_UNSEL, "s8_icons_floppy_window.png",   157,  72, ICAP_BG_WHITE },
    { ICAP_DOC,        ICAP_UNSEL, "s8_icons_on_desktop.png",      460, 382, ICAP_BG_WALL  },
    { ICAP_DOC,        ICAP_SEL,   "s8_icons_doc_sel.png",         157,  72, ICAP_BG_WHITE },
    { ICAP_DOC,        ICAP_SEL,   "s8_icons_desk_doc_sel.png",    460, 382, ICAP_BG_WALL  },
    { ICAP_APP,        ICAP_UNSEL, "s8_icons_floppy_window.png",    29,  72, ICAP_BG_WHITE },
    { ICAP_APP,        ICAP_UNSEL, "s8_icons_on_desktop.png",      460, 285, ICAP_BG_WALL  },
    { ICAP_APP,        ICAP_SEL,   "s8_icons_app_sel.png",          29,  72, ICAP_BG_WHITE },
    { ICAP_APP,        ICAP_SEL,   "s8_icons_desk_app_sel.png",    460, 285, ICAP_BG_WALL  },
    { ICAP_CAUTION,    ICAP_UNSEL, "s8_icons_alert_caution.png",   156, 100, ICAP_BG_FACE  },
    { ICAP_CAUTION,    ICAP_UNSEL, "s8_icons_alert_caution_1item.png", 156, 100, ICAP_BG_FACE }
};
#define ICAP_NSRC ((int)(sizeof ICAP_SRC / sizeof ICAP_SRC[0]))
#define ICAP_WALL_REF "s8_icons_trash_full_sel.png"

/* Opaque-cell counts measured in mint-008 Sec 2: a population check that
 * catches a wrong crop or a changed capture instead of grading against it. */
static const int ICAP_OPAQUE_COUNT[ICAP__KINDS] = {
    934, 316, 644, 725, 609, 797, 619, 572
};

static const char *const ICAP_KIND_NAME[ICAP__KINDS] = {
    "VOLUME", "HARDDISK", "TRASH", "TRASH_FULL", "FOLDER", "DOC", "APP", "CAUTION"
};

static inline unsigned char *icap_png(const char *dir, const char *name)
{
    char path[1024];
    png_image im;
    unsigned char *pixels;
    snprintf(path, sizeof path, "%s/%s", dir, name);
    memset(&im, 0, sizeof im);
    im.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&im, path)) {
        fprintf(stderr, "FAIL independent icon capture: %s: %s\n", path, im.message);
        png_image_free(&im);
        return NULL;
    }
    im.format = PNG_FORMAT_RGB;
    if (im.width != 640 || im.height != 480) {
        fprintf(stderr, "FAIL independent icon capture dimensions: %s\n", path);
        png_image_free(&im);
        return NULL;
    }
    pixels = malloc(PNG_IMAGE_SIZE(im));
    if (!pixels || !png_image_finish_read(&im, NULL, pixels, 0, NULL)) {
        fprintf(stderr, "FAIL independent icon capture decode: %s\n", path);
        free(pixels);
        pixels = NULL;
    }
    png_image_free(&im);
    return pixels;
}

static inline unsigned int icap_px(const unsigned char *img, int x, int y)
{
    const unsigned char *p = img + 3 * (y * 640 + x);
    return ((unsigned int)p[0] << 16) | ((unsigned int)p[1] << 8) | p[2];
}

/* Recover every kind's cell. Returns 0 on success, 1 (with a message) on a
 * missing / malformed capture, a disagreement or a population mismatch. */
static inline int icap_load(const char *dir, icap_cell_t cells[ICAP__KINDS])
{
    unsigned char *wall = icap_png(dir, ICAP_WALL_REF);
    unsigned char *img[ICAP_NSRC];
    int bad = 0;
    memset(cells, 0, sizeof(icap_cell_t) * ICAP__KINDS);
    memset(img, 0, sizeof img);
    if (!wall) return 1;
    for (int s = 0; s < ICAP_NSRC; s++) {
        for (int t = 0; t < s; t++) {
            if (strcmp(ICAP_SRC[t].file, ICAP_SRC[s].file) == 0) { img[s] = img[t]; break; }
        }
        if (!img[s]) img[s] = icap_png(dir, ICAP_SRC[s].file);
        if (!img[s]) bad = 1;
    }
    if (bad) goto out;
    /* Pass 1: opacity = union of background-difference evidence. */
    for (int s = 0; s < ICAP_NSRC; s++) {
        const icap_src_t *src = &ICAP_SRC[s];
        for (int r = 0; r < 32; r++) {
            for (int c = 0; c < 32; c++) {
                int x = src->x0 + c, y = src->y0 + r;
                unsigned int p = icap_px(img[s], x, y), b;
                if (src->bg == ICAP_BG_WHITE)     b = 0xFFFFFFu;
                else if (src->bg == ICAP_BG_FACE) b = 0xE7E7E7u;
                else b = icap_px(wall, 128 + (x % 128), 148 + (((y - 148) % 128) + 128) % 128);
                if (p != b) cells[src->kind].opaque[r][c] = 1;
            }
        }
    }
    /* Pass 2: per-state colour; every source of a state must agree. */
    for (int s = 0; s < ICAP_NSRC; s++) {
        const icap_src_t *src = &ICAP_SRC[s];
        icap_cell_t *k = &cells[src->kind];
        int first = !k->known[src->state];
        for (int r = 0; r < 32; r++) {
            for (int c = 0; c < 32; c++) {
                unsigned int p = icap_px(img[s], src->x0 + c, src->y0 + r);
                if (!k->opaque[r][c]) continue;
                if (first) k->rgb[src->state][r][c] = p;
                else if (k->rgb[src->state][r][c] != p) {
                    fprintf(stderr, "FAIL independent icon golden: %s state %d disagrees "
                            "between captures at cell (%d,%d) in %s\n",
                            ICAP_KIND_NAME[src->kind], src->state, r, c, src->file);
                    bad = 1;
                }
            }
        }
        k->known[src->state] = 1;
    }
    for (int k = 0; k < ICAP__KINDS; k++) {
        int n = 0;
        for (int r = 0; r < 32; r++)
            for (int c = 0; c < 32; c++) n += cells[k].opaque[r][c];
        if (n != ICAP_OPAQUE_COUNT[k]) {
            fprintf(stderr, "FAIL independent icon crop: %s has %d opaque cells, "
                    "mint-008 measured %d\n", ICAP_KIND_NAME[k], n, ICAP_OPAQUE_COUNT[k]);
            bad = 1;
        }
    }
out:
    for (int s = 0; s < ICAP_NSRC; s++) {
        int dup = 0;
        for (int t = 0; t < s; t++) if (img[t] == img[s]) { dup = 1; break; }
        if (!dup) free(img[s]);
    }
    free(wall);
    return bad;
}

/* Grade a 32x32 cell of a 24-bit RGB frame (w x h, 3 bytes a pixel, row
 * major -- a PPM body) against kind/state: opaque cells must equal the
 * capture's RGB, clear cells must equal `bg`. Returns the number of
 * differing cells and prints the first few (`label` names the site). */
static inline int icap_grade_frame(const unsigned char *frame, int w, int h,
                            int x0, int y0, const icap_cell_t *k, int state,
                            unsigned int bg, const char *label)
{
    int diff = 0, shown = 0;
    for (int r = 0; r < 32; r++) {
        for (int c = 0; c < 32; c++) {
            int x = x0 + c, y = y0 + r;
            unsigned int got, want;
            if (x < 0 || y < 0 || x >= w || y >= h) { diff++; continue; }
            got = ((unsigned int)frame[3 * (y * w + x)] << 16) |
                  ((unsigned int)frame[3 * (y * w + x) + 1] << 8) |
                  frame[3 * (y * w + x) + 2];
            want = k->opaque[r][c] ? k->rgb[state][r][c] : bg;
            if (got != want) {
                diff++;
                if (shown < 4) {
                    printf("    %s cell (%d,%d) at (%d,%d): got #%06X want #%06X (%s)\n",
                           label, r, c, x, y, got, want, k->opaque[r][c] ? "opaque" : "clear");
                    shown++;
                }
            }
        }
    }
    return diff;
}

#endif /* INITECH_HARNESS_ICON_CAPTURE_H */
