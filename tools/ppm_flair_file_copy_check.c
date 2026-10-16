/*
 * tools/ppm_flair_file_copy_check.c -- the FILE COPY emulator screendump
 * grader (factory C, host; bead initech-bf2).
 *
 *   mid  dump.ppm   the box is UP mid-copy (screendump at TENANT-FILECOPY-MID):
 *     LEG BAR  the Platinum CDEF 5 bar at FileCopyDialog's item rect
 *              (154,249)-(486,263) is PARTIAL (0 < f < W, f measured from the
 *              dump's own fill edge) and EVERY one of its 332 x 14 pixels is
 *              the value the MINTED captures give for that f -- column-class
 *              extension of ../system7-decomp/goldens/captures/
 *              s8_progress_determinate{_start,,_full}.png through the ratified
 *              lavender->teal map at 8bpp (harness/render/progress_golden.h).
 *              Never the artifact's spec/progress_platinum.h (Law 2).
 *     LEG TEXT the caption rect (154,225)-(486,241) holds EXACTLY the ink of
 *              "Saving tables to disk..." in the locked Chicago 12 strike
 *              (spec/assets/chicago12.h set bits; PRD Appendix A literal typed
 *              here), and nothing but white/face elsewhere in the rect.
 *   done dump.ppm   the box is GONE after completion (screendump at
 *              FLAIR-LIVE-OK): the bar's black frame row no longer spans the
 *              item rect.
 *
 * Exit 0 PASS, 1 a named FAIL, 2 unusable input. ASCII-clean (Rule 12).
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "progress_golden.h"
#include "chicago12.h"

#define BAR_L 154
#define BAR_T 249
#define BAR_W 332
#define TXT_L 154
#define TXT_T 225
#define TXT_R 486
#define TXT_B 241
static const char CAPTION[] = "Saving tables to disk...";   /* PRD App. A */

static unsigned char pixels[640 * 480 * 3];
static uint32_t px(int x, int y)
{
    if (x < 0 || x >= 640 || y < 0 || y >= 480) return 0xFFFFFFFFu;
    const unsigned char *p = pixels + 3 * (y * 640 + x);
    return (uint32_t)p[0] * 65536u + (uint32_t)p[1] * 256u + p[2];
}

static int leg_bar(void)
{
    pg_golden_t g;
    int f = 0, bad = 0, fx = -1, fy = -1;
    uint32_t fw = 0, fg = 0;
    if (!pg_golden_load(&g)) return 2;
    /* The fill edge: the first black column on interior row 0 (golden row
     * 135: the fill and its cap are #5454B3 there, only the edge is black). */
    for (int x = BAR_L + 2; x <= BAR_L + BAR_W - 3; x++)
        if (px(x, BAR_T + 2) == 0x000000u) { f = x - (BAR_L + 2) + 1; break; }
    if (f <= 0 || f >= BAR_W - 4) {
        fprintf(stderr, "FAIL file-copy BAR_PARTIAL: fill count %d is not strictly inside 0..%d\n",
                f, BAR_W - 4);
        pg_golden_free(&g);
        return 1;
    }
    for (int dy = 0; dy < PG_H; dy++)
        for (int dx = 0; dx < BAR_W; dx++) {
            uint32_t want = pg_expected(&g, BAR_W, f, dx, dy, 8);
            if (want == PG_NONE) continue;
            uint32_t got = px(BAR_L + dx, BAR_T + dy);
            if (got != want) {
                if (fx < 0) { fx = BAR_L + dx; fy = BAR_T + dy; fw = want; fg = got; }
                bad++;
            }
        }
    pg_golden_free(&g);
    if (bad) {
        fprintf(stderr, "FAIL file-copy BAR_GOLDEN: %d of %d bar pixels differ from the minted "
                "CDEF 5 captures at f=%d (first (%d,%d) want #%06X got #%06X)\n",
                bad, BAR_W * PG_H, f, fx, fy, (unsigned)fw, (unsigned)fg);
        return 1;
    }
    printf("file-copy BAR PASS: f=%d of %d, all %d pixels = the minted Platinum bar\n",
           f, BAR_W - 4, BAR_W * PG_H - 2);
    return 0;
}

static int leg_text(void)
{
    long want = 0, black = 0, other = 0;
    for (const char *s = CAPTION; *s; s++) {
        const uint16_t *rows = chicago12_rows[chicago12_index((int)(unsigned char)*s)];
        for (int r = 0; r < CHICAGO_CELL_H; r++)
            for (uint16_t b = rows[r]; b; b &= (uint16_t)(b - 1u)) want++;
    }
    for (int y = TXT_T; y < TXT_B; y++)
        for (int x = TXT_L; x < TXT_R; x++) {
            uint32_t c = px(x, y);
            if (c == 0x000000u) black++;
            else if (c != 0xFFFFFFu && c != 0xE7E7E7u) other++;
        }
    if (black != want || other != 0) {
        fprintf(stderr, "FAIL file-copy CAPTION: \"%s\" has %ld ink px in the locked strike; "
                "the caption rect holds %ld black and %ld foreign px\n",
                CAPTION, want, black, other);
        return 1;
    }
    printf("file-copy CAPTION PASS: %ld ink px = \"%s\" in Chicago 12\n", want, CAPTION);
    return 0;
}

int main(int argc, char **argv)
{
    FILE *f;
    int w, h, max, rc = 0;
    if (argc != 3 || (strcmp(argv[1], "mid") && strcmp(argv[1], "done"))) {
        fprintf(stderr, "usage: ppm_flair_file_copy_check mid|done dump.ppm\n");
        return 2;
    }
    f = fopen(argv[2], "rb");
    if (!f || fscanf(f, "P6 %d %d %d", &w, &h, &max) != 3 || w != 640 || h != 480 ||
        max != 255 || fgetc(f) == EOF || fread(pixels, 1, sizeof pixels, f) != sizeof pixels) {
        fprintf(stderr, "FAIL file-copy: missing or malformed 640x480 PPM %s\n", argv[2]);
        if (f) fclose(f);
        return 2;
    }
    fclose(f);
    if (!strcmp(argv[1], "mid")) {
        int a = leg_bar(), b = leg_text();
        if (a == 2) return 2;
        rc = a || b;
    } else {
        int frame = 0;
        for (int x = BAR_L + 1; x <= BAR_L + BAR_W - 2; x++) frame += px(x, BAR_T + 1) == 0x000000u;
        if (frame > (BAR_W - 2) / 2) {
            fprintf(stderr, "FAIL file-copy GONE: the bar frame row is still on screen (%d black px)\n", frame);
            rc = 1;
        } else {
            printf("file-copy GONE PASS: the box was disposed and its footprint restored\n");
        }
    }
    return rc;
}
