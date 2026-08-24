/*
 * test_desk_icons.c -- the desktop-icon strike + blitter oracle (THE ORACLE).
 *
 * beads: initech-tdnl.9 (R3 "STAPLER-DESK": VOLUME + TRASH 32x32 strikes, the
 *        finder_icon blit, and the mask hit-test).
 *
 * Ref: spec/assets/desk_icons.h (the LOCKED strikes; the ASCII maps this file's
 *      expectations are read off), os/flair/finder_icon.h (the contract under
 *      test), os/flair/flair_look.h (the ONE policy seam),
 *      spec/assets/color_canon.h (the canon values this file re-states BY HAND
 *      rather than reading -- see INDEPENDENCE below),
 *      docs/design/GUI-remediation-R3-finder-design.md F1.1/F1.2.
 *      CLAUDE.md Law 2 (the oracle is the truth; an oracle that computes its
 *      expectations from the artifact's own source is not an oracle), Rule 6
 *      (mutation-proven), Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * WHAT IT GRADES
 *   1. STRIKE INVARIANTS. ink & ~mask == 0, shade & ~mask == 0, ink & shade == 0
 *      on every row of both strikes (a malformed plane triple would render
 *      plausible-looking garbage).
 *   2. GEOMETRY. Blit each icon onto a scratch surface pre-filled with a
 *      SENTINEL, then check, per row, the number of INK pixels, SHADE pixels and
 *      OPAQUE pixels against a HAND TABLE read off the ASCII map -- plus a set of
 *      hand-picked PROBE PIXELS (corners, the chamfer diagonal, the shutter slot,
 *      the label rules, the basket rim and its taper).
 *   3. TRANSPARENCY. The sentinel survives EXACTLY where the map says clear:
 *      inside the cell at every '.' pixel, and everywhere outside the 32x32 cell.
 *   4. CLIPPING. With a clip region covering the icon's left half, nothing is
 *      written right of the clip, and the left half is unchanged from the
 *      unclipped blit.
 *   5. EDGES. Blits hanging off all four sides write only the visible part; a
 *      guard band past the end of the bitmap buffer is never touched (Rule 2).
 *   6. HIT TESTING. finder_icon_hit is the MASK, not the bounding box.
 *   7. BOTH DEPTHS. 8bpp indexed and 32bpp direct -- both surface_put_pixel
 *      branches, and both flair_look_pixel_depth branches.
 *
 * INDEPENDENCE (Law 2 / HER-02 boundary)
 *   - The per-row (ink, shade, opaque) tables and the probe pixels are read off
 *     the HAND-AUTHORED ASCII maps in spec/assets/desk_icons.h, never computed
 *     at runtime from the packed strike arrays. If a packed word ever drifts
 *     from the map it transcribes, these tables bite.
 *   - Spot-verified by hand, four rows per icon (the arithmetic is written out
 *     at each table below), so the table is not merely a machine echo.
 *   - The expected PIXEL VALUES are hand-restated canon, NOT read from
 *     color_canon.h or from flair_look: ink = index 0 / #000000 (CIDX_BLACK),
 *     face = index 1 / #FFFFFF (CIDX_WHITE), shade = index 6 / #C0C0C0
 *     (CIDX_CONTROL). A mis-keyed PART->canon row in flair_look.c goes RED here.
 *
 * MUTANTS (Rule 6): DESK_ICON_MUT_MASK_IGNORED (transparency dropped) and
 * DESK_ICON_MUT_ROW_OFF1 (strike rows shifted by one) are compiled into
 * finder_icon.c via -D; the Makefile gate asserts each goes RED.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "region_algebra.h"   /* the LOCKED region contract (-Ispec)           */
#include "region.h"           /* region_from_rects (-Ios/flair/atkinson)       */
#include "surface.h"          /* bitmap_t + surface_get_pixel (-Ios/flair)     */
#include "desk_icons.h"       /* the LOCKED strikes (-Ispec/assets)            */
#include "finder_icon.h"      /* the blitter under test (-Ios/flair)           */
#include "test_assert.h"      /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)      */

TEST_HARNESS();

/* ===========================================================================
 * The scratch surface. Bigger than one icon cell so an off-cell write shows up,
 * plus a GUARD band past the end of the bitmap the surface must never touch.
 * ===========================================================================*/
enum { SW = 64, SH = 64, GUARD = 32 };

#define SENT8   0x11u          /* 8bpp sentinel: not 0 (ink), 1 (face), 6 (shade) */
#define SENT32  0x00112233u    /* 32bpp sentinel: not black/white/C0C0C0          */
#define GUARD8  0x5Au
#define GUARD32 0x005A5A5Au

/* HAND-RESTATED canon values (see INDEPENDENCE above). Not read from any
 * header the artifact also reads. */
#define WANT8_INK    0x00u     /* CIDX_BLACK   */
#define WANT8_FACE   0x01u     /* CIDX_WHITE   */
#define WANT8_SHADE  0x06u     /* CIDX_CONTROL */
#define WANT32_INK   0x00000000u   /* #000000 */
#define WANT32_FACE  0x00FFFFFFu   /* #FFFFFF */
#define WANT32_SHADE 0x00C0C0C0u   /* #C0C0C0 */

static uint8_t  g_buf8[SW * SH + GUARD];
static uint32_t g_buf32[SW * SH + GUARD];

static void mk_dst8(bitmap_t *bm)
{
    memset(bm, 0, sizeof *bm);
    bm->base = g_buf8;
    bm->bpp = 8u;
    bm->bytes_per_pixel = 1u;
    bm->width = SW;
    bm->height = SH;
    bm->pitch = SW;
    memset(g_buf8, SENT8, (size_t)SW * SH);
    memset(g_buf8 + (size_t)SW * SH, GUARD8, GUARD);
}

static void mk_dst32(bitmap_t *bm)
{
    int i;
    memset(bm, 0, sizeof *bm);
    bm->base = (volatile uint8_t *)g_buf32;
    bm->bpp = 32u;
    bm->bytes_per_pixel = 4u;
    bm->width = SW;
    bm->height = SH;
    bm->pitch = SW * 4u;
    for (i = 0; i < SW * SH; i++) g_buf32[i] = SENT32;
    for (i = 0; i < GUARD; i++) g_buf32[SW * SH + i] = GUARD32;
}

static uint32_t get8(int x, int y)  { return (uint32_t)g_buf8[y * SW + x]; }
static uint32_t get32(int x, int y) { return g_buf32[y * SW + x]; }

static int guard8_intact(void)
{
    int i;
    for (i = 0; i < GUARD; i++) if (g_buf8[SW * SH + i] != GUARD8) return 0;
    return 1;
}
static int guard32_intact(void)
{
    int i;
    for (i = 0; i < GUARD; i++) if (g_buf32[SW * SH + i] != GUARD32) return 0;
    return 1;
}

/* ===========================================================================
 * THE HAND TABLES -- read off the ASCII pixel maps in spec/assets/desk_icons.h.
 * Each entry is { ink, shade, opaque } for that map row.
 * ===========================================================================*/
typedef struct { int ink, shade, opaque; } rowcount_t;

/* VOLUME -- the 3.5-inch diskette. Four rows verified by hand:
 *   row  3 (body top edge)   : ink = cols 3..25            = 23; gray 0;
 *                              opaque 23.
 *   row  6 (shutter top)     : ink = body col 3 + shutter cols 11 and 21 +
 *                              body col 28                 =  4;
 *                              gray = shutter cols 12..20  =  9;
 *                              opaque = body cols 3..28    = 26.
 *   row 19 (first label rule): ink = cols 3, 7, 24, 28     =  4;
 *                              gray = rule cols 10..21     = 12; opaque 26.
 *   row 28 (body base)       : ink = cols 3..28            = 26; opaque 26. */
static const rowcount_t VOL_ROWS[DESK_ICON_DIM] = {
    {  0,  0,  0 },   /* row  0 */
    {  0,  0,  0 },   /* row  1 */
    {  0,  0,  0 },   /* row  2 */
    { 23,  0, 23 },   /* row  3 */
    {  2,  0, 24 },   /* row  4 */
    { 13,  0, 25 },   /* row  5 */
    {  4,  9, 26 },   /* row  6 */
    {  4,  6, 26 },   /* row  7 */
    {  4,  6, 26 },   /* row  8 */
    {  4,  6, 26 },   /* row  9 */
    {  4,  6, 26 },   /* row 10 */
    {  4,  6, 26 },   /* row 11 */
    {  4,  6, 26 },   /* row 12 */
    {  4,  9, 26 },   /* row 13 */
    { 13,  0, 26 },   /* row 14 */
    {  2,  0, 26 },   /* row 15 */
    { 20,  0, 26 },   /* row 16 */
    {  4,  0, 26 },   /* row 17 */
    {  4,  0, 26 },   /* row 18 */
    {  4, 12, 26 },   /* row 19 */
    {  4,  0, 26 },   /* row 20 */
    {  4, 12, 26 },   /* row 21 */
    {  4,  0, 26 },   /* row 22 */
    {  4, 12, 26 },   /* row 23 */
    {  4,  0, 26 },   /* row 24 */
    {  4,  0, 26 },   /* row 25 */
    { 20,  0, 26 },   /* row 26 */
    {  2,  0, 26 },   /* row 27 */
    { 26,  0, 26 },   /* row 28 */
    {  0,  0,  0 },   /* row 29 */
    {  0,  0,  0 },   /* row 30 */
    {  0,  0,  0 },   /* row 31 */
};

/* TRASH -- the ridged waste basket. Four rows verified by hand:
 *   row  5 (lid face)        : ink = cols 7 and 24         =  2;
 *                              gray = lid cols 8..23       = 16; opaque 18.
 *   row  9 (upper body)      : ink = walls cols 9 and 22   =  2;
 *                              gray = ridges cols 13,16,19 =  3;
 *                              opaque = cols 9..22         = 14.
 *   row 22 (last taper step) : ink = walls cols 11 and 20  =  2;
 *                              gray = ridges cols 13,16,19 =  3;
 *                              opaque = cols 11..20        = 10.
 *   row 29 (basket base)     : ink = cols 11..20           = 10; opaque 10. */
static const rowcount_t TRASH_ROWS[DESK_ICON_DIM] = {
    {  0,  0,  0 },   /* row  0 */
    {  0,  0,  0 },   /* row  1 */
    {  6,  0,  6 },   /* row  2 */
    {  2,  0,  2 },   /* row  3 */
    { 18,  0, 18 },   /* row  4 */
    {  2, 16, 18 },   /* row  5 */
    { 18,  0, 18 },   /* row  6 */
    { 14,  0, 14 },   /* row  7 */
    {  2,  0, 14 },   /* row  8 */
    {  2,  3, 14 },   /* row  9 */
    {  2,  3, 14 },   /* row 10 */
    {  2,  3, 14 },   /* row 11 */
    {  2,  3, 14 },   /* row 12 */
    {  2,  3, 14 },   /* row 13 */
    {  2,  3, 14 },   /* row 14 */
    {  2,  3, 12 },   /* row 15 */
    {  2,  3, 12 },   /* row 16 */
    {  2,  3, 12 },   /* row 17 */
    {  2,  3, 12 },   /* row 18 */
    {  2,  3, 12 },   /* row 19 */
    {  2,  3, 12 },   /* row 20 */
    {  2,  3, 12 },   /* row 21 */
    {  2,  3, 10 },   /* row 22 */
    {  2,  3, 10 },   /* row 23 */
    {  2,  3, 10 },   /* row 24 */
    {  2,  3, 10 },   /* row 25 */
    {  2,  3, 10 },   /* row 26 */
    {  2,  3, 10 },   /* row 27 */
    {  2,  0, 10 },   /* row 28 */
    { 10,  0, 10 },   /* row 29 */
    {  0,  0,  0 },   /* row 30 */
    {  0,  0,  0 },   /* row 31 */
};

/* Hand-picked probe pixels, read off the same maps. (row, col, expected tone) */
typedef struct { int r, c; desk_tone_t tone; } probe_t;

static const probe_t VOL_PROBES[] = {
    {  0,  0, DESK_TONE_CLEAR },  /* every corner of the cell is transparent  */
    {  0, 31, DESK_TONE_CLEAR },
    { 31,  0, DESK_TONE_CLEAR },
    { 31, 31, DESK_TONE_CLEAR },
    {  3,  3, DESK_TONE_INK   },  /* body top-left corner                     */
    {  3, 26, DESK_TONE_CLEAR },  /* the bevelled top-right corner is cut     */
    {  4, 26, DESK_TONE_INK   },  /* ... and the bevel diagonal is ink        */
    {  5, 27, DESK_TONE_INK   },
    {  6, 28, DESK_TONE_INK   },  /* body right edge, below the bevel         */
    {  5, 11, DESK_TONE_INK   },  /* shutter top edge                         */
    {  7, 12, DESK_TONE_SHADE },  /* shutter metal                            */
    {  7, 13, DESK_TONE_FACE  },  /* the shutter's read slot                  */
    {  7, 16, DESK_TONE_SHADE },
    { 16,  7, DESK_TONE_INK   },  /* label top-left corner                    */
    { 17,  8, DESK_TONE_FACE  },  /* label paper                              */
    { 19, 10, DESK_TONE_SHADE },  /* first ruled line                         */
    { 19, 22, DESK_TONE_FACE  },  /* ... which stops short of the label edge  */
    { 27, 15, DESK_TONE_FACE  },
    { 28, 15, DESK_TONE_INK   }   /* body base                                */
};
enum { VOL_PROBE_N = (int)(sizeof VOL_PROBES / sizeof VOL_PROBES[0]) };

static const probe_t TRASH_PROBES[] = {
    {  0,  0, DESK_TONE_CLEAR },
    { 31, 15, DESK_TONE_CLEAR },
    {  2, 13, DESK_TONE_INK   },  /* lid handle arch                          */
    {  3, 15, DESK_TONE_CLEAR },  /* ... the arch is hollow                   */
    {  4,  7, DESK_TONE_INK   },  /* lid slab, left end                       */
    {  5, 10, DESK_TONE_SHADE },  /* lid face                                 */
    {  8,  9, DESK_TONE_INK   },  /* body wall, widest step                   */
    {  9, 12, DESK_TONE_FACE  },
    {  9, 13, DESK_TONE_SHADE },  /* first ridge                              */
    { 25, 11, DESK_TONE_INK   },  /* body wall, narrowest step                */
    { 25,  9, DESK_TONE_CLEAR },  /* ... the taper has left this column       */
    { 29, 15, DESK_TONE_INK   }   /* basket base                              */
};
enum { TRASH_PROBE_N = (int)(sizeof TRASH_PROBES / sizeof TRASH_PROBES[0]) };

static uint32_t want8(desk_tone_t t)
{
    if (t == DESK_TONE_INK)   return WANT8_INK;
    if (t == DESK_TONE_FACE)  return WANT8_FACE;
    if (t == DESK_TONE_SHADE) return WANT8_SHADE;
    return SENT8;                                   /* CLEAR: sentinel lives  */
}
static uint32_t want32(desk_tone_t t)
{
    if (t == DESK_TONE_INK)   return WANT32_INK;
    if (t == DESK_TONE_FACE)  return WANT32_FACE;
    if (t == DESK_TONE_SHADE) return WANT32_SHADE;
    return SENT32;
}

/* ===========================================================================
 * 1. Strike invariants (structural: no tone conflict, no tone outside the mask)
 * ===========================================================================*/
static void check_invariants(const FLAIRDeskIcon *icon, const char *name)
{
    int r;
    int bad_ink = 0, bad_shade = 0, bad_both = 0, empty = 1;
    for (r = 0; r < DESK_ICON_DIM; r++) {
        if (icon->ink[r]   & ~icon->mask[r]) bad_ink++;
        if (icon->shade[r] & ~icon->mask[r]) bad_shade++;
        if (icon->ink[r]   &  icon->shade[r]) bad_both++;
        if (icon->mask[r]) empty = 0;
    }
    (void)name;
    CHECK(bad_ink == 0,   "strike has ink outside the mask");
    CHECK(bad_shade == 0, "strike has shade outside the mask");
    CHECK(bad_both == 0,  "strike has a pixel that is both ink and shade");
    CHECK(empty == 0,     "strike is entirely empty");
}

/* ===========================================================================
 * 2/3. Blit at (ox,oy) and grade rows, probes and sentinel survival (8bpp).
 * ===========================================================================*/
static void check_blit8(const FLAIRDeskIcon *icon, const rowcount_t *tab,
                        const probe_t *probes, int nprobes, int ox, int oy)
{
    bitmap_t bm;
    int r, c, i;
    int row_bad = 0, outside_bad = 0, probe_bad = 0;

    mk_dst8(&bm);
    finder_icon_draw(&bm, (int16_t)ox, (int16_t)oy, icon, (const region_t *)0);

    /* Per-row counts inside the cell. */
    for (r = 0; r < DESK_ICON_DIM; r++) {
        int ink = 0, shade = 0, opaque = 0;
        for (c = 0; c < DESK_ICON_DIM; c++) {
            uint32_t v = get8(ox + c, oy + r);
            if (v == SENT8) continue;
            opaque++;
            if (v == WANT8_INK) ink++;
            else if (v == WANT8_SHADE) shade++;
            else if (v != WANT8_FACE) opaque += 1000;   /* an unknown value   */
        }
        if (ink != tab[r].ink || shade != tab[r].shade || opaque != tab[r].opaque) {
            if (row_bad == 0) {
                fprintf(stderr, "  row %d: got ink=%d shade=%d opaque=%d,"
                                " want ink=%d shade=%d opaque=%d\n",
                        r, ink, shade, opaque,
                        tab[r].ink, tab[r].shade, tab[r].opaque);
            }
            row_bad++;
        }
    }
    CHECK(row_bad == 0, "per-row ink/shade/opaque counts match the hand table");

    /* Nothing outside the 32x32 cell was touched. */
    for (r = 0; r < SH; r++) {
        for (c = 0; c < SW; c++) {
            int in_cell = (r >= oy && r < oy + DESK_ICON_DIM &&
                           c >= ox && c < ox + DESK_ICON_DIM);
            if (!in_cell && get8(c, r) != SENT8) outside_bad++;
        }
    }
    CHECK(outside_bad == 0, "no pixel outside the icon cell was written");

    /* Probe pixels. */
    for (i = 0; i < nprobes; i++) {
        uint32_t got = get8(ox + probes[i].c, oy + probes[i].r);
        if (got != want8(probes[i].tone)) {
            if (probe_bad == 0) {
                fprintf(stderr, "  probe (r=%d,c=%d): got 0x%02X want 0x%02X\n",
                        probes[i].r, probes[i].c, got, want8(probes[i].tone));
            }
            probe_bad++;
        }
    }
    CHECK(probe_bad == 0, "probe pixels carry the hand-read tone");
    CHECK(guard8_intact(), "8bpp guard band untouched");
}

/* 32bpp probes + the same sentinel-survival rule (the other depth branch). */
static void check_blit32(const FLAIRDeskIcon *icon,
                         const probe_t *probes, int nprobes)
{
    bitmap_t bm;
    int i, probe_bad = 0, clear_bad = 0, r, c;

    mk_dst32(&bm);
    finder_icon_draw(&bm, 0, 0, icon, (const region_t *)0);

    for (i = 0; i < nprobes; i++) {
        uint32_t got = get32(probes[i].c, probes[i].r);
        if (got != want32(probes[i].tone)) {
            if (probe_bad == 0) {
                fprintf(stderr, "  probe32 (r=%d,c=%d): got 0x%08X want 0x%08X\n",
                        probes[i].r, probes[i].c, got, want32(probes[i].tone));
            }
            probe_bad++;
        }
    }
    for (r = DESK_ICON_DIM; r < SH; r++)
        for (c = 0; c < SW; c++)
            if (get32(c, r) != SENT32) clear_bad++;

    CHECK(probe_bad == 0, "32bpp probe pixels carry the hand-read tone");
    CHECK(clear_bad == 0, "32bpp: nothing below the icon cell was written");
    CHECK(guard32_intact(), "32bpp guard band untouched");
}

/* ===========================================================================
 * 4. Clipping: a clip covering the icon's left half must gate every write.
 * ===========================================================================*/
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

static void check_clip(const FLAIRDeskIcon *icon)
{
    static uint8_t unclipped[SW * SH];
    bitmap_t bm;
    rgn_store_t clip;
    rgn_rect_t half;
    int r, c, left_bad = 0, right_bad = 0;
    const int ox = 4, oy = 6;
    const int split = ox + DESK_ICON_DIM / 2;   /* clip covers [ox, split)    */

    /* Reference: the same blit with no clip. */
    mk_dst8(&bm);
    finder_icon_draw(&bm, (int16_t)ox, (int16_t)oy, icon, (const region_t *)0);
    memcpy(unclipped, g_buf8, sizeof unclipped);

    store_attach(&clip);
    half.top = 0; half.left = 0; half.bottom = (int16_t)SH; half.right = (int16_t)split;
    region_from_rects(&clip.r, &half, 1u);

    mk_dst8(&bm);
    finder_icon_draw(&bm, (int16_t)ox, (int16_t)oy, icon, &clip.r);

    for (r = 0; r < SH; r++) {
        for (c = 0; c < SW; c++) {
            uint32_t got = get8(c, r);
            if (c < split) {
                if (got != (uint32_t)unclipped[r * SW + c]) left_bad++;
            } else {
                if (got != SENT8) right_bad++;
            }
        }
    }
    CHECK(left_bad == 0,  "inside the clip the clipped blit equals the unclipped one");
    CHECK(right_bad == 0, "outside the clip NOTHING was written");
    CHECK(guard8_intact(), "clip case: guard band untouched");
}

/* ===========================================================================
 * 5. Edges: blits hanging off each side write only the visible part.
 * ===========================================================================*/
static void check_edges(const FLAIRDeskIcon *icon)
{
    bitmap_t bm;
    int ok = 1;
    int r, c;
    const int offs[4][2] = { { -8, -8 }, { SW - 8, -8 },
                             { -8, SH - 8 }, { SW - 8, SH - 8 } };
    int i;

    for (i = 0; i < 4; i++) {
        mk_dst8(&bm);
        finder_icon_draw(&bm, (int16_t)offs[i][0], (int16_t)offs[i][1], icon,
                         (const region_t *)0);
        if (!guard8_intact()) ok = 0;
        for (r = 0; r < SH; r++) {
            for (c = 0; c < SW; c++) {
                int sr = r - offs[i][1];
                int sc = c - offs[i][0];
                int opaque_here = 0;
                if (sr >= 0 && sr < DESK_ICON_DIM && sc >= 0 && sc < DESK_ICON_DIM) {
                    opaque_here = (icon->mask[sr] & (1u << (31 - sc))) != 0u;
                }
                if (!opaque_here && get8(c, r) != SENT8) ok = 0;
                if (opaque_here && get8(c, r) == SENT8) ok = 0;
            }
        }
    }
    CHECK(ok == 1, "partly off-surface blits draw exactly the visible mask");
}

/* ===========================================================================
 * 6. Hit testing is the MASK, not the bounding box.
 * ===========================================================================*/
static void check_hit(const FLAIRDeskIcon *icon,
                      const probe_t *probes, int nprobes)
{
    int i, bad = 0;
    const int ox = 100, oy = 40;

    for (i = 0; i < nprobes; i++) {
        int want = (probes[i].tone != DESK_TONE_CLEAR);
        int got = finder_icon_hit((int16_t)(ox + probes[i].c),
                                  (int16_t)(oy + probes[i].r), icon,
                                  (int16_t)ox, (int16_t)oy);
        if (got != want) bad++;
    }
    CHECK(bad == 0, "finder_icon_hit agrees with the hand-read mask at every probe");

    CHECK(finder_icon_hit((int16_t)(ox - 1), (int16_t)oy, icon,
                          (int16_t)ox, (int16_t)oy) == 0,
          "a point left of the cell is not a hit");
    CHECK(finder_icon_hit((int16_t)(ox + DESK_ICON_DIM), (int16_t)oy, icon,
                          (int16_t)ox, (int16_t)oy) == 0,
          "a point right of the cell is not a hit");
    CHECK(finder_icon_hit((int16_t)ox, (int16_t)(oy + DESK_ICON_DIM), icon,
                          (int16_t)ox, (int16_t)oy) == 0,
          "a point below the cell is not a hit");
    CHECK(finder_icon_hit((int16_t)ox, (int16_t)oy,
                          (const FLAIRDeskIcon *)0, (int16_t)ox, (int16_t)oy) == 0,
          "a NULL strike is never hit");
}

/* ===========================================================================
 * 7. Fail-soft: a NULL destination / NULL strike must not write or crash.
 * ===========================================================================*/
static void check_failsoft(const FLAIRDeskIcon *icon)
{
    bitmap_t bm;
    int r, c, bad = 0;

    mk_dst8(&bm);
    finder_icon_draw((const bitmap_t *)0, 0, 0, icon, (const region_t *)0);
    finder_icon_draw(&bm, 0, 0, (const FLAIRDeskIcon *)0, (const region_t *)0);
    for (r = 0; r < SH; r++)
        for (c = 0; c < SW; c++)
            if (get8(c, r) != SENT8) bad++;
    CHECK(bad == 0, "NULL dst / NULL strike write nothing");
}

int main(void)
{
    check_invariants(&FLAIR_DESK_ICON_VOLUME, "VOLUME");
    check_invariants(&FLAIR_DESK_ICON_TRASH,  "TRASH");

    /* Blit at the origin and at an offset -- the offset case catches an origin
     * that is silently ignored or doubled. */
    check_blit8(&FLAIR_DESK_ICON_VOLUME, VOL_ROWS, VOL_PROBES, VOL_PROBE_N, 0, 0);
    check_blit8(&FLAIR_DESK_ICON_VOLUME, VOL_ROWS, VOL_PROBES, VOL_PROBE_N, 11, 7);
    check_blit8(&FLAIR_DESK_ICON_TRASH, TRASH_ROWS, TRASH_PROBES, TRASH_PROBE_N, 0, 0);
    check_blit8(&FLAIR_DESK_ICON_TRASH, TRASH_ROWS, TRASH_PROBES, TRASH_PROBE_N, 30, 25);

    check_blit32(&FLAIR_DESK_ICON_VOLUME, VOL_PROBES, VOL_PROBE_N);
    check_blit32(&FLAIR_DESK_ICON_TRASH, TRASH_PROBES, TRASH_PROBE_N);

    check_clip(&FLAIR_DESK_ICON_VOLUME);
    check_clip(&FLAIR_DESK_ICON_TRASH);

    check_edges(&FLAIR_DESK_ICON_VOLUME);
    check_edges(&FLAIR_DESK_ICON_TRASH);

    check_hit(&FLAIR_DESK_ICON_VOLUME, VOL_PROBES, VOL_PROBE_N);
    check_hit(&FLAIR_DESK_ICON_TRASH, TRASH_PROBES, TRASH_PROBE_N);

    check_failsoft(&FLAIR_DESK_ICON_VOLUME);

    return TEST_SUMMARY("test-desk-icons");
}
