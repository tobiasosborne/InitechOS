/*
 * boot_screen.h -- the FLAIR boot screen (bead initech-tdnl.105): the Mac OS
 * 8.1 "Welcome to Mac OS" startup panel, cell for cell, with the Initech logo
 * lockup in its logo cell and "Welcome to InitechOS." as its title.
 *
 * Ref: ../system7-decomp/goldens/captures/s8_startup_welcome.png and
 *      re/mint-results-009.md Sec 2 (the panel geometry, measured: outer frame
 *      x 109..530 y 47..369, bevel, face, the inset well x 144..494 y
 *      73..296, the logo cell, the title baseline) and Sec 1 (the 50 % gray
 *      screen: black where x+y is even; the rounded screen corners).
 *      spec/assets/boot_logo.h (the logo strike, AUTHORED from the measured
 *      geometry), spec/assets/chicago12.h via os/flair/text.h (the title
 *      font), docs/pubs/
 *      DESIGN-REFERENCE.md Sec 7.0 (the logo face colours).
 *      CLAUDE.md Law 1 (every coordinate below is a measured capture pixel),
 *      Law 3 (freestanding C; no host state), Rule 11, Rule 12 (ASCII).
 *
 * PURE: boot_screen_render() computes the 640x480 screen as palette INDICES,
 * one row at a time, top to bottom, and hands each row to a sink. The kernel
 * sink writes the live LFB (8bpp DAC indices or 24/32bpp RGB); the host oracle
 * sink writes a PPM that tools/ppm_flair_boot_screen_check.c grades against
 * the capture. The renderer never touches hardware.
 *
 * DEVIATIONS (deliberate, documented):
 *   - the title is Chicago 12 (FLAIR's system font), not the capture's
 *     Charcoal 12; it is centred on the panel and its caps sit on the
 *     capture's cap rows 317..325 (baseline 325).
 *   - the logo cell holds the Initech lockup, not the Mac OS logo.
 */
#ifndef INITECH_BOOT_SCREEN_H
#define INITECH_BOOT_SCREEN_H

#include <stdint.h>
#include "boot_logo.h"
/* Kernel size round 5 (WL-0168): the freestanding kernel keeps the RLE
 * stream packed (os/flair/asset_lz_data.h, generated from the unchanged
 * locked boot_logo.h) and unpacks it into BSS when the panel is rendered;
 * a hosted build (test-boot-screen) reads the locked table. test-asset-lz
 * proves decode == locked (Rule 8). */
#if !defined(__STDC_HOSTED__) || !__STDC_HOSTED__
#define ASSET_LZ_LOGO
#include "asset_lz.h"        /* -Ios/flair */
#include "asset_lz_data.h"
_Static_assert(sizeof boot_logo_rle == ASSET_LZ_LOGO_RAW, "boot_logo_rle[BOOT_LOGO_RLE_BYTES]");
#endif
#include "text.h"        /* chicago_cell_bits / chicago_advance: the ONE glyph
                            * row accessor (packed rows in the kernel)       */

#define BOOT_SCREEN_W 640
#define BOOT_SCREEN_H 480

/* POLICY: the FLAIR palette indices the boot screen paints (flair_dac.h; an
 * index >= 11 is the gray of that value). The panel greys are the capture's
 * ROM clut 8 values (#DDDDDD face, #AAAAAA shadow, #EEEEEE bevel corner); the
 * logo faces follow DESIGN-REFERENCE Sec 7.0 -- tops in the desktop canon
 * teal CIDX_DESKTOP (#8DDCDC) until operator decision initech-9222 rules on
 * Initech Cyan; left faces + logotype Initech Black, which Sec 7.1 specifies
 * as 0/0/0/100 (the 18/19/20 of Sec 7.0 is a photographed sample of it), so
 * CIDX_BLACK; right faces the logo light grey (16 % black; sampled 213/212/
 * 212 -> the gray-ramp index 212). Indices 0..25 are named canon slots
 * (several are icon colours), so the greys are taken from the ramp above
 * them, never from a low slot. */
#define BOOT_IDX_BLACK   0u
#define BOOT_IDX_WHITE   1u
#define BOOT_IDX_TEAL    2u
#define BOOT_IDX_DARK    0u
#define BOOT_IDX_LIGHT   212u
#define BOOT_IDX_SHADOW  170u
#define BOOT_IDX_FACE    221u
#define BOOT_IDX_CORNER  238u

/* Geometry (inclusive pixels), s8_startup_welcome.png. */
#define BOOT_PANEL_L     109
#define BOOT_PANEL_T     47
#define BOOT_PANEL_R     530
#define BOOT_PANEL_B     369
#define BOOT_WELL_L      144      /* the well's shadow column/row        */
#define BOOT_WELL_T      73
#define BOOT_WELL_R      494      /* the well's highlight column/row     */
#define BOOT_WELL_B      296
/* The logo cell: the Mac OS logo's bbox x 219..416 y 92..272, centre
 * (317.5, 182); the strike is centred on it. */
#define BOOT_LOGO_X      (318 - BOOT_LOGO_W / 2)
#define BOOT_LOGO_Y      (182 - BOOT_LOGO_H / 2)
/* Title: the capture's caps occupy rows 317..325; the Chicago cell top sits
 * CHICAGO_CAP_TOP rows above the cap top. */
#define BOOT_TITLE_TOP   (317 - CHICAGO_CAP_TOP)
#define BOOT_TITLE       "Welcome to InitechOS."

typedef void (*boot_screen_sink_fn)(void *ctx, int y, const uint8_t *row);

static inline void boot_screen_fill(uint8_t *row, int x0, int x1, uint8_t idx)
{
    for (int x = x0; x <= x1; x++)
        row[x] = idx;
}

static void boot_screen_render(boot_screen_sink_fn sink, void *ctx)
{
    static const uint8_t corner_w[5] = {5, 3, 2, 1, 1};
    static const uint8_t role_idx[4] = {0u, BOOT_IDX_TEAL, BOOT_IDX_DARK,
                                        BOOT_IDX_LIGHT};
    uint8_t row[BOOT_SCREEN_W];
#if !defined(__STDC_HOSTED__) || !__STDC_HOSTED__
    static uint8_t logo_store[BOOT_LOGO_RLE_BYTES];
    const uint8_t *rle = logo_store;
    asset_lz_unpack(logo_store, ASSET_LZ_LOGO_RAW, asset_lz_logo);
#else
    const uint8_t *rle = boot_logo_rle;
#endif
    int title_w = 0, title_x;

    for (const char *s = BOOT_TITLE; *s; s++)
        title_w += chicago_advance((unsigned char)*s);
    title_x = BOOT_PANEL_L + (BOOT_PANEL_R - BOOT_PANEL_L + 1 - title_w) / 2;

    for (int y = 0; y < BOOT_SCREEN_H; y++) {
        /* The 50 % gray: black where x + y is even. */
        for (int x = 0; x < BOOT_SCREEN_W; x++)
            row[x] = ((x + y) & 1) ? BOOT_IDX_WHITE : BOOT_IDX_BLACK;
        /* The rounded screen corners (black, widths 5,3,2,1,1). */
        {
            int r = y < 5 ? y : BOOT_SCREEN_H - 1 - y;
            if (r < 5) {
                boot_screen_fill(row, 0, corner_w[r] - 1, BOOT_IDX_BLACK);
                boot_screen_fill(row, BOOT_SCREEN_W - corner_w[r],
                                 BOOT_SCREEN_W - 1, BOOT_IDX_BLACK);
            }
        }

        if (y >= BOOT_PANEL_T && y <= BOOT_PANEL_B) {
            if (y == BOOT_PANEL_T || y == BOOT_PANEL_B) {
                boot_screen_fill(row, BOOT_PANEL_L, BOOT_PANEL_R, BOOT_IDX_BLACK);
            } else {
                row[BOOT_PANEL_L] = BOOT_IDX_BLACK;
                row[BOOT_PANEL_R] = BOOT_IDX_BLACK;
                if (y == BOOT_PANEL_T + 1) {
                    boot_screen_fill(row, BOOT_PANEL_L + 1, BOOT_PANEL_R - 2, BOOT_IDX_WHITE);
                    row[BOOT_PANEL_R - 1] = BOOT_IDX_CORNER;
                } else if (y == BOOT_PANEL_B - 1) {
                    row[BOOT_PANEL_L + 1] = BOOT_IDX_CORNER;
                    boot_screen_fill(row, BOOT_PANEL_L + 2, BOOT_PANEL_R - 1, BOOT_IDX_SHADOW);
                } else {
                    row[BOOT_PANEL_L + 1] = BOOT_IDX_WHITE;
                    boot_screen_fill(row, BOOT_PANEL_L + 2, BOOT_PANEL_R - 2, BOOT_IDX_FACE);
                    row[BOOT_PANEL_R - 1] = BOOT_IDX_SHADOW;
                    /* The inset well: shadow top/left, black frame, highlight
                     * right/bottom, white interior. */
                    if (y == BOOT_WELL_T) {
                        boot_screen_fill(row, BOOT_WELL_L, BOOT_WELL_R - 1, BOOT_IDX_SHADOW);
                    } else if (y > BOOT_WELL_T && y < BOOT_WELL_B) {
                        row[BOOT_WELL_L] = BOOT_IDX_SHADOW;
                        if (y == BOOT_WELL_T + 1 || y == BOOT_WELL_B - 1) {
                            boot_screen_fill(row, BOOT_WELL_L + 1, BOOT_WELL_R - 1, BOOT_IDX_BLACK);
                        } else {
                            row[BOOT_WELL_L + 1] = BOOT_IDX_BLACK;
                            boot_screen_fill(row, BOOT_WELL_L + 2, BOOT_WELL_R - 2, BOOT_IDX_WHITE);
                            row[BOOT_WELL_R - 1] = BOOT_IDX_BLACK;
                        }
                        row[BOOT_WELL_R] = BOOT_IDX_WHITE;
                    } else if (y == BOOT_WELL_B) {
                        boot_screen_fill(row, BOOT_WELL_L + 1, BOOT_WELL_R, BOOT_IDX_WHITE);
                    }
                }
            }
        }

        /* The logo strike, one RLE row (rows are consumed in order). */
        if (y >= BOOT_LOGO_Y && y < BOOT_LOGO_Y + BOOT_LOGO_H) {
            int x = 0;
            while (x < BOOT_LOGO_W) {
                int code = *rle >> 6, len = (*rle & 63) + 1;
                rle++;
#ifndef FLAIR_BOOT_MUT_NO_LOGO
                if (code != BOOT_LOGO_CODE_CLEAR)
                    boot_screen_fill(row, BOOT_LOGO_X + x, BOOT_LOGO_X + x + len - 1,
                                     role_idx[code]);
#else
                (void)code; (void)role_idx;
#endif
                x += len;
            }
        }

        /* The title, Chicago 12, black ink on the face. */
        if (y >= BOOT_TITLE_TOP && y < BOOT_TITLE_TOP + CHICAGO_CELL_H) {
            int pen = title_x;
            for (const char *s = BOOT_TITLE; *s; s++) {
                int c = (unsigned char)*s;
                int aw = chicago_advance(c);
                unsigned bits = chicago_cell_bits(c, y - BOOT_TITLE_TOP);
                for (int col = 0; col < aw; col++)
                    if (bits & (0x8000u >> col))
                        row[pen + col] = BOOT_IDX_BLACK;
                pen += aw;
            }
        }

        sink(ctx, y, row);
    }
}

#endif /* INITECH_BOOT_SCREEN_H */
