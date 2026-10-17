/*
 * test_desk_icons.c -- the Finder icon BLITTER oracle (test-desk-icons).
 *
 * beads: initech-tdnl.9 (R3 desktop icon blit), initech-6k12 (the FULL Trash),
 *        initech-tdnl.53 (v2 strikes; this file re-keyed from the v1 hand
 *        tables to the independent mint-008 captures).
 *
 * WHAT IT GRADES: the MECHANICS of os/flair/finder_icon.c -- transparency,
 * clipping, off-surface edges, the hit test, the NULL guards, both depths --
 * on the desktop strikes (VOLUME, TRASH, TRASH_FULL). The expected pixels are
 * the mint-008 Mac OS 8.1 capture cells decoded at run time by
 * harness/render/icon_capture.h (Law 2: never the strike's own map); the art
 * itself, every strike and state, is test-finder-icon-art's job.
 *
 * MUTANTS (Rule 6; test-desk-icons-mutant):
 *   DESK_ICON_MUT_MASK_IGNORED  (finder_icon.c) clear nibbles painted -> the
 *                               sentinel checks go RED.
 *   DESK_ICON_MUT_ROW_OFF1      (finder_icon.c) rows read one off -> every
 *                               cell grade goes RED.
 *   DESK_ICONS_MUT_FULL_ROW     (here) the FULL Trash strike's row 5 -- its
 *                               paper tops -- blanked: the FULL grade must
 *                               print "TRASH_FULL draw: 21/3072 differing pixels".
 *
 * Usage: test_desk_icons <system7-decomp goldens/captures dir>
 * Ref: ../system7-decomp/re/mint-results-008.md; os/flair/finder_icon.h;
 *      CLAUDE.md Law 2, Law 3, Rule 2, Rule 6, Rule 12.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "finder_icon.h"
#include "flair_look.h"
#include "color_canon.h"
#include "desk_icons.h"

#include "../render/icon_capture.h"

#define SW 64
#define SH 48
#define SENT32 0x00FE01FDu
#define SENT8  0xEEu

static uint32_t g32[SW * SH];
static uint8_t  g8[SW * SH];
static int g_checks, g_fails;

#define CHECK(cond, msg) do { g_checks++; if (!(cond)) { g_fails++; \
    printf("  FAIL %s\n", (msg)); } } while (0)

static void mk32(bitmap_t *b)
{
    for (int i = 0; i < SW * SH; i++) g32[i] = SENT32;
    memset(b, 0, sizeof *b);
    b->base = (volatile uint8_t *)(void *)g32; b->width = SW; b->height = SH;
    b->bpp = 32; b->bytes_per_pixel = 4; b->pitch = SW * 4;
}

static void mk8(bitmap_t *b)
{
    for (int i = 0; i < SW * SH; i++) g8[i] = SENT8;
    memset(b, 0, sizeof *b);
    b->base = (volatile uint8_t *)(void *)g8; b->width = SW; b->height = SH;
    b->bpp = 8; b->bytes_per_pixel = 1; b->pitch = SW;
}

typedef struct rgn_store {
    region_t  r;
    rgn_row_t rows[RGN_ROWS_CAP];
    int16_t   pool[RGN_X_POOL_CAP];
} rgn_store_t;

static void store_attach(rgn_store_t *s)
{
    memset(s, 0, sizeof *s);
    s->r.rows       = s->rows;
    s->r.cap_rows   = RGN_ROWS_CAP;
    s->r.x_pool     = s->pool;
    s->r.x_pool_cap = RGN_X_POOL_CAP;
    region_set_empty(&s->r);
}

/* Expected value at surface (x,y) for a cell drawn at (ox,oy), visible only
 * inside [cl,cr) x [ct,cb): the capture's RGB on an opaque cell, else the
 * sentinel. Compares both depths; returns the number of differing pixels. */
static int grade_surface(const icap_cell_t *cap, int state, int ox, int oy,
                         int cl, int ct, int cr, int cb, const char *what)
{
    int diff = 0, shown = 0;
    for (int y = 0; y < SH; y++) {
        for (int x = 0; x < SW; x++) {
            int r = y - oy, c = x - ox;
            int in = r >= 0 && r < 32 && c >= 0 && c < 32 &&
                     x >= cl && x < cr && y >= ct && y < cb && cap->opaque[r][c];
            uint32_t want = in ? cap->rgb[state][r][c] : SENT32;
            uint32_t got32 = g32[y * SW + x] & 0x00FFFFFFu;
            uint8_t  got8 = g8[y * SW + x];
            int bad = (got32 != want) ||
                      (in ? (got8 == SENT8 || flair_canon_rgb(got8) != want) : got8 != SENT8);
            if (bad) {
                diff++;
                if (shown++ < 3)
                    printf("    %s (%d,%d): 32bpp #%06X 8bpp idx %u want #%06X\n",
                           what, x, y, (unsigned)got32, (unsigned)got8, (unsigned)want);
            }
        }
    }
    printf("  %s: %d/%d differing pixels\n", what, diff, SW * SH);
    return diff;
}

static void draw_both(const FLAIRDeskIcon *s, int ox, int oy, const region_t *clip, int sel)
{
    bitmap_t b32, b8;
    mk32(&b32); mk8(&b8);
    if (sel) {
        finder_icon_draw_hilite(&b32, (int16_t)ox, (int16_t)oy, s, clip);
        finder_icon_draw_hilite(&b8, (int16_t)ox, (int16_t)oy, s, clip);
    } else {
        finder_icon_draw(&b32, (int16_t)ox, (int16_t)oy, s, clip);
        finder_icon_draw(&b8, (int16_t)ox, (int16_t)oy, s, clip);
    }
}

int main(int argc, char **argv)
{
    static icap_cell_t cap[ICAP__KINDS];
    FLAIRDeskIcon full = FLAIR_DESK_ICON_TRASH_FULL;
    char what[96];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <system7-decomp goldens/captures dir>\n", argv[0]);
        return 2;
    }
    if (icap_load(argv[1], cap)) {
        printf("!!! test-desk-icons FAIL: the independent mint-008 golden is unavailable\n");
        return 1;
    }
#if defined(DESK_ICONS_MUT_FULL_ROW)
    /* MUTANT (Rule 6): the FULL strike's row 5 (the paper tops) blanked. */
    memset(full.px[5], 0, sizeof full.px[5]);
    printf("  MUTANT (Rule 6): FLAIR_DESK_ICON_TRASH_FULL row 5 blanked\n");
#endif
    {
        const struct { const char *n; const FLAIRDeskIcon *s; int k; } D[] = {
            { "VOLUME", &FLAIR_DESK_ICON_VOLUME, ICAP_VOLUME },
            { "TRASH", &FLAIR_DESK_ICON_TRASH, ICAP_TRASH },
            { "TRASH_FULL", &full, ICAP_TRASH_FULL }
        };
        for (int i = 0; i < 3; i++) {
            for (int sel = 0; sel < 2; sel++) {
                /* 1. a whole cell on the surface: every cell + nothing else. */
                draw_both(D[i].s, 16, 8, (const region_t *)0, sel);
                snprintf(what, sizeof what, "%s %sdraw", D[i].n, sel ? "selected " : "");
                CHECK(grade_surface(&cap[D[i].k], sel, 16, 8, 0, 0, SW, SH, what) == 0, what);
                /* 2. off the top-left and the bottom-right edges. */
                draw_both(D[i].s, -10, -7, (const region_t *)0, sel);
                snprintf(what, sizeof what, "%s %soff top-left", D[i].n, sel ? "selected " : "");
                CHECK(grade_surface(&cap[D[i].k], sel, -10, -7, 0, 0, SW, SH, what) == 0, what);
                draw_both(D[i].s, SW - 9, SH - 13, (const region_t *)0, sel);
                snprintf(what, sizeof what, "%s %soff bottom-right", D[i].n, sel ? "selected " : "");
                CHECK(grade_surface(&cap[D[i].k], sel, SW - 9, SH - 13, 0, 0, SW, SH, what) == 0, what);
            }
            /* 3. a clip: only the clipped part of the cell is written. */
            {
                rgn_store_t clip;
                rgn_rect_t rc;
                store_attach(&clip);
                rc.left = 24; rc.top = 12; rc.right = 40; rc.bottom = 36;
                region_from_rects(&clip.r, &rc, 1u);
                draw_both(D[i].s, 16, 8, &clip.r, 0);
                snprintf(what, sizeof what, "%s clipped to (24,12)-(40,36)", D[i].n);
                CHECK(grade_surface(&cap[D[i].k], 0, 16, 8, 24, 12, 40, 36, what) == 0, what);
            }
            /* 4. the hit test is the capture's opacity, and nothing outside. */
            {
                int bad = 0;
                for (int r = -2; r < 34; r++)
                    for (int c = -2; c < 34; c++) {
                        int want = (r >= 0 && r < 32 && c >= 0 && c < 32) ? cap[D[i].k].opaque[r][c] : 0;
                        if (finder_icon_hit((int16_t)(300 + c), (int16_t)(200 + r), D[i].s, 300, 200) != want) bad++;
                    }
                snprintf(what, sizeof what, "%s hit test == capture opacity (36x36 probe)", D[i].n);
                CHECK(bad == 0, what);
            }
        }
    }
    /* 5. NULL guards: no write, no crash, no hit. */
    {
        bitmap_t b32;
        int bad = 0;
        mk32(&b32);
        finder_icon_draw(&b32, 0, 0, (const FLAIRDeskIcon *)0, (const region_t *)0);
        finder_icon_draw((const bitmap_t *)0, 0, 0, &FLAIR_DESK_ICON_VOLUME, (const region_t *)0);
        for (int i = 0; i < SW * SH; i++) if (g32[i] != SENT32) bad++;
        CHECK(bad == 0, "NULL strike / NULL destination write nothing");
        CHECK(finder_icon_hit(5, 5, (const FLAIRDeskIcon *)0, 0, 0) == 0, "NULL strike is never hit");
    }
    printf("test-desk-icons: %d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) {
        printf("!!! test-desk-icons FAIL\n");
        return 1;
    }
    printf(">>> test-desk-icons: blitter green against the mint-008 captures\n");
    return 0;
}
