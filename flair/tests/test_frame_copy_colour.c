/* GM3: the live FILE COPY's body and progress fill are the surviving still's
 * white and blue. The expected colours are read at run time from the frame's
 * colour sheet (FRAME_COLOURS, goldens/frame/colours.tsv: filecopy_content and
 * progress_fill, relative to the photographed white), never from the painter
 * or the palette source. Graded at 32 bpp, at 8 bpp through the palette, and
 * through the DAC table the boot loads. The generic FileCopyDialog, built
 * into the same record afterwards, keeps the captured Platinum E7 face.
 * The Toolbox's slice face and side (PARTs SLICE and SLICE_SIDE, what
 * TBX_COLOR_SLICE and TBX_COLOR_SLICE_SIDE draw) are graded the same way
 * against the sheet's wedge40_top and wedge_side_bottom: the largest sample
 * of each role, and the area-weighted median of its samples, which is what
 * InitechCalc's booted check grades the pie against. */
#define main old_dialog_main
#include "test_dialog.c"
#undef main
#include "flair_dac.h"
#include "palette.h"
#include "flair_look.h"

static uint32_t direct[640 * 480];
static uint8_t indexed[640 * 480];

/* The sample's relative colour, scaled to full white. */
static void reference(const char *path, const char *sample, unsigned out[3])
{
    FILE *f = fopen(path, "r");
    char line[1024];
    if (!f) {
        perror(path);
        exit(2);
    }
    while (fgets(line, sizeof line, f)) {
        char *fields[12], *p = line;
        unsigned n = 0;
        do {
            fields[n++] = p;
            p = strchr(p, '\t');
            if (p) *p++ = 0;
        } while (p && n < 12);
        if (n >= 9 && !strcmp(fields[0], sample)) {
            double r, g, b;
            if (sscanf(fields[8], "%lf %lf %lf", &r, &g, &b) != 3) exit(2);
            out[0] = (unsigned)(r * 255 + 0.5);
            out[1] = (unsigned)(g * 255 + 0.5);
            out[2] = (unsigned)(b * 255 + 0.5);
            fclose(f);
            return;
        }
    }
    fclose(f);
    fprintf(stderr, "no frame colour %s\n", sample);
    exit(2);
}

static void grade(uint32_t rgb, const unsigned want[3], unsigned tolerance,
                  const char *what)
{
    for (unsigned k = 0; k < 3; k++) {
        int d = (int)((rgb >> (16 - k * 8)) & 255) - (int)want[k];
        if (d < 0) d = -d;
        CHECK((unsigned)d <= tolerance, what);
    }
}

static void point_port(DialogPtr dp, bitmap_t bm)
{
    dp->window.port.portBits.bm = bm;
    dp->window.port.visRgn = 0;
    dp->window.port.clipRgn = 0;
}

int main(void)
{
    unsigned white[3], blue[3];
    reference(FRAME_COLOURS, "filecopy_content", white);
    reference(FRAME_COLOURS, "progress_fill", blue);
    for (unsigned depth = 8; depth <= 32; depth += 24) {
        static ControlRecord controls[2];
        bitmap_t bm = { .base = depth == 8 ? (void *)indexed : (void *)direct,
                        .pitch = 640 * (depth / 8), .width = 640, .height = 480,
                        .bpp = depth, .bytes_per_pixel = depth / 8 };
        rgn_store_init(&g_mdl_sruc);
        rgn_store_init(&g_mdl_scont);
        rgn_store_init(&g_mdl_supd);
        DialogPtr dp = FileCopyCancelableDialog(&g_mdl_dr, g_mdl_items, controls,
                                                &g_mdl_sruc.r, &g_mdl_scont.r,
                                                &g_mdl_supd.r);
        point_port(dp, bm);
        DrawDialog(dp);
        /* (300,205): body above the caption; (170,240): the bar's filled
         * run at the canon initial value (FLAIR_CANON_FILECOPY_PROGRESS). */
        if (depth == 32) {
            grade(direct[205 * 640 + 300], white, 0,
                  "live body matches photographed white");
            grade(direct[240 * 640 + 170], blue, 1,
                  "live fill matches photographed blue ratios");
        } else {
            unsigned body = indexed[205 * 640 + 300];
            unsigned fill = indexed[240 * 640 + 170];
            grade(flair_palette_rgb((uint8_t)body), white, 0,
                  "indexed body matches photographed white");
            grade(flair_palette_rgb((uint8_t)fill), blue, 1,
                  "indexed fill matches photographed blue");
            uint32_t dac = ((uint32_t)flair_dac[fill][0] << 16) |
                           ((uint32_t)flair_dac[fill][1] << 8) |
                           flair_dac[fill][2];
            grade(dac, blue, 1, "the DAC entry matches the still's blue");
        }
        /* The same record rebuilt as the generic FILE COPY: E7 again. */
        dp = FileCopyDialog(&g_mdl_dr, g_mdl_items, controls, &g_mdl_sruc.r,
                            &g_mdl_scont.r, &g_mdl_supd.r);
        point_port(dp, bm);
        DrawDialog(dp);
        CHECK(depth == 8 ? indexed[244 * 640 + 300] == 231
                         : direct[244 * 640 + 300] == 0xe7e7e7,
              "generic dialog retains captured Platinum face");
    }
    {
        static const struct { int part; const char *sample, *what; } slice[2] = {
            { FLAIR_PART_SLICE, "wedge40_top", "slice face" },
            { FLAIR_PART_SLICE_SIDE, "wedge_side_bottom", "slice side" },
        };
        for (unsigned k = 0; k < 2; k++) {
            unsigned want[3];
            reference(FRAME_COLOURS, slice[k].sample, want);
            uint32_t idx = flair_look_pixel_depth(8, slice[k].part);
            uint32_t dac = ((uint32_t)flair_dac[idx & 255u][0] << 16) |
                           ((uint32_t)flair_dac[idx & 255u][1] << 8) |
                           flair_dac[idx & 255u][2];
            grade(flair_look_pixel_depth(32, slice[k].part), want, 1, slice[k].what);
            grade(flair_palette_rgb((uint8_t)idx), want, 1, slice[k].what);
            grade(dac, want, 1, slice[k].what);
            CHECK(idx != flair_look_pixel_depth(8, FLAIR_PART_CAPTION_NAVY),
                  "a slice colour has its own palette slot, not navy's");
        }
    }
    return TEST_SUMMARY("test-frame-copy-colour");
}
