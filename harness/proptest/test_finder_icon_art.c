/*
 * test_finder_icon_art.c -- the Finder icon ART oracle (test-finder-icon-art).
 *
 * beads: initech-tdnl.53 (Platinum icon art), initech-tdnl.74 (the FULL
 *        Trash), initech-tdnl.87 (the alert caution icon).
 *
 * WHAT IT GRADES: every LOCKED strike (spec/assets/desk_icons.h,
 * finder_icons.h, alert_icons.h), drawn by the ARTIFACT blitter
 * (os/flair/finder_icon.c) through the ARTIFACT policy seam (flair_look ->
 * color_canon), cell for cell against the INDEPENDENT mint-008 Mac OS 8.1
 * captures decoded at run time by harness/render/icon_capture.h -- both the
 * unselected draw (finder_icon_draw) and the selected / drop-target draw
 * (finder_icon_draw_hilite), at 32bpp (the canon RGB) and at 8bpp (the
 * palette index, resolved through flair_canon_rgb as the DAC is). ZERO
 * tolerance: every one of the 1024 cells, opaque cells by RGB and clear cells
 * by the untouched sentinel (the WL-0126 arrow-cursor pattern).
 *
 * INDEPENDENCE (Law 2): the expected values are the PNG bytes; icon_capture.h
 * includes no artifact header. This file reads the strikes only to hand them
 * to the artifact blitter (the value under test) and to assert the format
 * invariants -- never to compute an expected pixel.
 *
 * MUTATION (Rule 6): -DICON_ART_MUT_STRIKE=<k> grades a copy of strike k with
 * its first row that differs from the next replaced by that next row ("one
 * strike row off"); that strike must go RED
 * (the Makefile checks the exact line) while the others stay green.
 * Artifact-side knobs DESK_ICON_MUT_ROW_OFF1 / DESK_ICON_MUT_NO_SELECT live
 * in finder_icon.c and are driven by test-desk-icons-mutant / this target's
 * mutant twin.
 *
 * Usage: test_finder_icon_art <system7-decomp goldens/captures dir>
 * Ref: ../system7-decomp/re/mint-results-008.md; CLAUDE.md Law 1/2, Rule 6,
 *      Rule 12; docs/worklog/WL-0140.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "finder_icon.h"      /* the artifact blitter                       */
#include "flair_look.h"
#include "color_canon.h"
#include "desk_icons.h"
#include "finder_icons.h"
#include "alert_icons.h"

#include "../render/icon_capture.h"   /* the independent golden */

static int g_fail;

typedef struct {
    const char *name;
    const FLAIRDeskIcon *strike;
    int kind;
} art_t;

static const art_t ART[] = {
    { "FLAIR_DESK_ICON_VOLUME",     &FLAIR_DESK_ICON_VOLUME,     ICAP_VOLUME     },
    { "FLAIR_DESK_ICON_HARDDISK",   &FLAIR_DESK_ICON_HARDDISK,   ICAP_HARDDISK   },
    { "FLAIR_DESK_ICON_TRASH",      &FLAIR_DESK_ICON_TRASH,      ICAP_TRASH      },
    { "FLAIR_DESK_ICON_TRASH_FULL", &FLAIR_DESK_ICON_TRASH_FULL, ICAP_TRASH_FULL },
    { "FLAIR_FINDER_ICON_FOLDER",   &FLAIR_FINDER_ICON_FOLDER,   ICAP_FOLDER     },
    { "FLAIR_FINDER_ICON_DOC",      &FLAIR_FINDER_ICON_DOC,      ICAP_DOC        },
    { "FLAIR_FINDER_ICON_APP",      &FLAIR_FINDER_ICON_APP,      ICAP_APP        },
    { "FLAIR_ALERT_ICON_CAUTION",   &FLAIR_ALERT_ICON_CAUTION,   ICAP_CAUTION    }
};
#define NART ((int)(sizeof ART / sizeof ART[0]))

#define SENTINEL32 0x00FE01FDu   /* in no capture (not a clut 0x11 value) */
#define SENTINEL8  0xEEu

static int check_invariants(const art_t *a)
{
    const FLAIRDeskIcon *s = a->strike;
    int bad = 0;
    int used[DESK_ICON_TONES] = { 0 };
    if (s->tone[0] != DESK_IC_CLEAR) {
        printf("  FAIL %s: tone[0] is %d, not DESK_IC_CLEAR\n", a->name, s->tone[0]);
        bad++;
    }
    for (int r = 0; r < DESK_ICON_DIM; r++)
        for (int c = 0; c < DESK_ICON_DIM; c++)
            used[(s->px[r][c >> 3] >> (28 - 4 * (c & 7))) & 0xFu] = 1;
    for (int n = 1; n < DESK_ICON_TONES; n++) {
        int k = s->tone[n];
        if (k >= (int)DESK_IC__COUNT) {
            printf("  FAIL %s: tone[%d] = %d out of range\n", a->name, n, k);
            bad++;
        }
        if (!used[n]) continue;
        if (k == (int)DESK_IC_CLEAR) {
            printf("  FAIL %s: nibble %d is used but its tone is CLEAR\n", a->name, n);
            bad++;
        } else if (!desk_ic_observed_input(k)) {
            printf("  FAIL %s: nibble %d names colour %d, which no capture shows as an "
                   "unselected input (its selected form is unmeasured)\n", a->name, n, k);
            bad++;
        }
    }
    return bad;
}

/* Draw `s` (selected or not) at 32bpp and 8bpp; count cells differing from
 * the capture cell. */
static int grade(const char *name, const FLAIRDeskIcon *s, const icap_cell_t *cap,
                 int state)
{
    static uint32_t b32[32 * 32];
    static uint8_t  b8[32 * 32];
    bitmap_t d32, d8;
    int diff32 = 0, diff8 = 0, shown = 0;

    for (int i = 0; i < 32 * 32; i++) { b32[i] = SENTINEL32; b8[i] = SENTINEL8; }
    memset(&d32, 0, sizeof d32);
    memset(&d8, 0, sizeof d8);
    d32.base = (volatile uint8_t *)(void *)b32; d32.width = 32; d32.height = 32;
    d32.bpp = 32; d32.bytes_per_pixel = 4; d32.pitch = 128;
    d8.base = (volatile uint8_t *)(void *)b8; d8.width = 32; d8.height = 32;
    d8.bpp = 8; d8.bytes_per_pixel = 1; d8.pitch = 32;
    if (state == ICAP_SEL) {
        finder_icon_draw_hilite(&d32, 0, 0, s, (const region_t *)0);
        finder_icon_draw_hilite(&d8, 0, 0, s, (const region_t *)0);
    } else {
        finder_icon_draw(&d32, 0, 0, s, (const region_t *)0);
        finder_icon_draw(&d8, 0, 0, s, (const region_t *)0);
    }
    for (int r = 0; r < 32; r++) {
        for (int c = 0; c < 32; c++) {
            uint32_t g32 = b32[r * 32 + c] & 0x00FFFFFFu;
            uint8_t  g8  = b8[r * 32 + c];
            int op = cap->opaque[r][c];
            uint32_t want = op ? cap->rgb[state][r][c] : SENTINEL32;
            int bad32 = g32 != want;
            int bad8  = op ? (g8 == SENTINEL8 || flair_canon_rgb(g8) != want)
                           : (g8 != SENTINEL8);
            diff32 += bad32;
            diff8  += bad8;
            if ((bad32 || bad8) && shown < 4) {
                printf("    %s %s cell (%d,%d): 32bpp got #%06X, 8bpp idx %u, want %s #%06X\n",
                       name, state ? "SEL" : "UNSEL", r, c, (unsigned)g32, (unsigned)g8,
                       op ? "opaque" : "clear", (unsigned)want);
                shown++;
            }
        }
    }
    printf("  %-28s %-5s : %d/1024 differing cells at 32bpp, %d/1024 at 8bpp\n",
           name, state ? "SEL" : "UNSEL", diff32, diff8);
    return diff32 + diff8;
}

int main(int argc, char **argv)
{
    static icap_cell_t cap[ICAP__KINDS];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <system7-decomp goldens/captures dir>\n", argv[0]);
        return 2;
    }
    printf("test-finder-icon-art: grading %d locked strikes against the mint-008 "
           "captures in %s\n", NART, argv[1]);
    if (icap_load(argv[1], cap)) {
        printf("!!! test-finder-icon-art FAIL: the independent mint-008 golden could not "
               "be recovered (see above)\n");
        return 1;
    }
    for (int i = 0; i < NART; i++) {
        const art_t *a = &ART[i];
        FLAIRDeskIcon s = *a->strike;
        if (check_invariants(a)) g_fail++;
#if defined(ICON_ART_MUT_STRIKE)
        if (i == ICON_ART_MUT_STRIKE) {
            /* The first row (from 1) that differs from the row below it is
             * replaced by that row: the strike is one row off there. */
            int mr = 1;
            while (mr < DESK_ICON_DIM - 1 &&
                   memcmp(s.px[mr], s.px[mr + 1], sizeof s.px[mr]) == 0) mr++;
            memcpy(s.px[mr], s.px[mr + 1], sizeof s.px[mr]);
            printf("  MUTANT (Rule 6): %s row %d replaced by row %d\n", a->name, mr, mr + 1);
        }
#endif
        for (int st = 0; st < ICAP__STATES; st++) {
            if (!cap[a->kind].known[st]) continue;
            if (grade(a->name, &s, &cap[a->kind], st) != 0) {
                printf("  RED %s %s\n", a->name, st ? "SEL" : "UNSEL");
                g_fail++;
            }
        }
    }
    /* The drawn opacity is also the hit test (finder_icon_hit). */
    for (int i = 0; i < NART; i++) {
        int bad = 0;
        for (int r = 0; r < 32; r++)
            for (int c = 0; c < 32; c++)
                if (finder_icon_hit((int16_t)(100 + c), (int16_t)(200 + r), ART[i].strike,
                                    100, 200) != cap[ART[i].kind].opaque[r][c]) bad++;
        if (bad) {
            printf("  FAIL %s: finder_icon_hit disagrees with the capture's opacity on %d cells\n",
                   ART[i].name, bad);
            g_fail++;
        }
    }
    if (g_fail) {
        printf("!!! test-finder-icon-art FAIL: %d strike/state grades RED\n", g_fail);
        return 1;
    }
    printf(">>> test-finder-icon-art: green (%d strikes, every state captured, "
           "0 differing cells, zero tolerance)\n", NART);
    return 0;
}
