/*
 * test_finder_desktop.c -- the R3.2 Finder DESKTOP MANAGER oracle (THE ORACLE).
 *
 * beads: initech-tdnl.9 (GUI remediation R3.2 "DesktopMgr").
 *
 * Ref: os/flair/finder_desktop.h (the contract under test, its stated
 *        deviations, and the serial-marker list the emu slice keys on),
 *      docs/design/GUI-remediation-R3-finder-design.md F1.1/F1.2/F1.3,
 *        F2.2/F2.3/F2-4/F2-5,
 *      spec/assets/desk_icons.h (the LOCKED strikes; the ASCII map this file's
 *        render probes are read off BY HAND),
 *      os/flair/window.h (the WindowMgr.desktop_underlay seam),
 *      harness/proptest/test_desk_icons.c (the sentinel/clip discipline this
 *        file's render leg follows), harness/proptest/test_drag.c (the
 *        arena-backed WindowMgr storage pattern).
 *      CLAUDE.md Law 2 (the oracle is the truth -- every expectation below is
 *        HAND-AUTHORED, never computed from the implementation it grades;
 *        HER-02), Rule 1 (red->green), Rule 6 (mutation-proven), Rule 11
 *        (deterministic), Rule 12 (ASCII-clean).
 *
 * WHAT IT GRADES
 *   L1 DEFAULT PLACEMENT. Seeding from the tenants-image desktop rect puts the
 *      volume sprite at (584,48) and the Trash sprite at (584,404) -- the two
 *      rectangles chosen to miss EVERY probe point of every locked emu trace on
 *      the 640x480 tenants image. Hard-coded here so a later edit that quietly
 *      relocates them goes RED against the recorded placement, not against a
 *      three-hour emulator run.
 *   L2 GEOMETRY. Label band is 13 rows, sits 2 px under the sprite, is centered
 *      on the sprite, and is clamped inside the desktop bounds.
 *   L3 HIT TEST. Sprite UNION label (not the bounding box), inclusive/exclusive
 *      half-open edges, and TOPMOST-WINS by record order.
 *   L4 SELECTION. click / shift-click extend / shift-click toggle-off /
 *      click-away clears.
 *   L5 MARQUEE. Hand-authored selected sets over a hand-authored layout,
 *      including the two edge cases the half-open +1 exists for, and a band in
 *      the dead corner beside a label (proves the bbox is NOT the target).
 *   L6 DOUBLE-CLICK SYNTHESIS. Hand-authored (target, point, tick) streams:
 *      20 ticks + 4 px slop is a double; 21 ticks or 5 px is two singles;
 *      a different target is two singles; a triple is double-then-single;
 *      bare desktop (target -1) never doubles.
 *   L7 DRAG. Commit / clamp-to-bounds / undraggable-reverts / zero-delta-noop,
 *      and the live outline agrees with the committed rect.
 *   L8 DB CODEC. encode() byte-for-byte against a HAND-AUTHORED 56-byte image
 *      (never read back off the writer), decode round-trip, and each corruption
 *      class rejected with fd left untouched.
 *   L9 COMPOSED DESKTOP. A real WindowMgr on a host bitmap, one window over the
 *      right half of an icon, the underlay installed: run the SAME scene twice,
 *      once with the underlay and once without, and assert (a) the two frames
 *      are IDENTICAL at every pixel the window's structure region owns -- icons
 *      can never paint on window pixels -- (b) they DIFFER somewhere, and (c)
 *      every differing pixel lies inside the icon's cell rect, with the
 *      hand-read strike probe reading ink where the icon is exposed and bare
 *      desktop teal where it is covered.
 *
 * INDEPENDENCE (Law 2 / HER-02 boundary)
 *   - Every rect, selected set, click classification and DB byte below is
 *     written out by hand from the design document and the header contract.
 *     Nothing is read back from finder_desktop.c's own structures.
 *   - The layout used by the tracker legs gives every icon an EMPTY label, so
 *     the label rect is exactly [cx-3, cx+3) x [y+34, y+47) by the header's
 *     constants -- fully hand-computable WITHOUT calling text_measure, so the
 *     expectations do not borrow the artifact's own font metrics.
 *   - The render leg's expected pixel VALUES are hand-restated canon (ink =
 *     index 0, desktop = index 2), not read from color_canon.h or flair_look.
 *
 * MUTANTS (Rule 6), all -D knobs on the IMPLEMENTATION TU:
 *   FINDER_DESK_MUT_DBLTICK_OFF        -- L6's doubles become singles.
 *   FINDER_DESK_MUT_MARQUEE_OFFBYONE   -- L5's edge sets shrink.
 *   FINDER_DESK_MUT_NO_UNDERLAY        -- L9's frames stop differing.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "region_algebra.h"    /* the LOCKED region contract (-Ispec)          */
#include "region.h"            /* rgn_row_t caps (-Ios/flair/atkinson)         */
#include "surface.h"           /* bitmap_t + surface_get_pixel (-Ios/flair)    */
#include "window.h"            /* WindowMgr + NewWindow (-Ios/flair)           */
#include "desktop.h"           /* desktop_paint_all / _damage                  */
#include "desk_icons.h"        /* the LOCKED strikes (-Ispec/assets)           */
#include "finder_desktop.h"    /* the module under test                        */
#include "test_assert.h"       /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)     */

TEST_HARNESS();

/* ===========================================================================
 * Shared hand-authored fixtures.
 * ===========================================================================*/

/* The tenants-image usable desktop: everything below the TWO stacked 20px menu
 * bars, on the 640x480 native surface (ADR-0004 OD-3). Written out by hand. */
#define DESK_TOP     40
#define DESK_LEFT     0
#define DESK_BOTTOM 480
#define DESK_RIGHT  640

static rgn_rect_t mk_rect(int16_t t, int16_t l, int16_t b, int16_t r)
{
    rgn_rect_t x; x.top = t; x.left = l; x.bottom = b; x.right = r; return x;
}

static int rect_eq(rgn_rect_t a, int t, int l, int b, int r)
{
    return a.top == t && a.left == l && a.bottom == b && a.right == r;
}

/* The tracker layout: three icons with EMPTY labels, so every rect below is
 * hand-computable from the header constants alone (see INDEPENDENCE).
 *   sprite(i)  = (x, y) .. (x+32, y+32)
 *   label(i)   = (x+13, y+34) .. (x+19, y+47)      [ 6 px wide, 13 rows ]
 *   cellbbox(i)= (x,     y   ) .. (x+32, y+47)
 * A at (10,10); B at (60,10); C at (10,60); D at (20,20) -- D is added LAST so
 * it is TOPMOST and overlaps A. */
typedef struct fixture {
    finder_desk_t      fd;
    finder_desk_icon_t icons[FINDER_DESK_MAX_ICONS];
} fixture_t;

static void fixture_layout(fixture_t *f, int with_d)
{
    memset(f, 0, sizeof *f);
    finder_desk_init(&f->fd, f->icons, (uint16_t)FINDER_DESK_MAX_ICONS,
                     mk_rect(0, 0, 200, 200));
    (void)finder_desk_add(&f->fd, FINDER_ICON_VOLUME, "", 10, 10, 1u);   /* A */
    (void)finder_desk_add(&f->fd, FINDER_ICON_TRASH,  "", 60, 10, 0u);   /* B */
    (void)finder_desk_add(&f->fd, FINDER_ICON_VOLUME, "", 10, 60, 1u);   /* C */
    if (with_d)
        (void)finder_desk_add(&f->fd, FINDER_ICON_VOLUME, "", 20, 20, 1u); /* D */
}

/* ===========================================================================
 * L1 -- DEFAULT PLACEMENT (the emu no-go set, locked here)
 * ===========================================================================*/
static void leg_defaults(void)
{
    fixture_t f;
    memset(&f, 0, sizeof f);
    finder_desk_init(&f.fd, f.icons, (uint16_t)FINDER_DESK_MAX_ICONS,
                     mk_rect(DESK_TOP, DESK_LEFT, DESK_BOTTOM, DESK_RIGHT));
    CHECK(finder_desk_seed_defaults(&f.fd) == 0, "L1 seed defaults succeeds");
    CHECK(f.fd.n == 2u, "L1 exactly two default desktop icons (volume + Trash)");

    CHECK(f.fd.icons[0].kind == (uint8_t)FINDER_ICON_VOLUME,
          "L1 record 0 is the VOLUME (record order == z order == DB order)");
    CHECK(f.fd.icons[1].kind == (uint8_t)FINDER_ICON_TRASH,
          "L1 record 1 is the TRASH");
    CHECK(f.fd.icons[0].draggable == 1u, "L1 the volume icon is draggable");
    CHECK(f.fd.icons[1].draggable == 0u,
          "L1 the Trash is FIXED-position (design F1.1)");
    CHECK(strcmp(f.fd.icons[1].name, "Trash") == 0, "L1 Trash label is 'Trash'");

    /* THE RECORDED PLACEMENT (hand-derived to miss every locked emu probe /
     * click / park point on the tenants image; see the bead report). */
    CHECK(rect_eq(finder_desk_sprite_rect(&f.fd, 0), 48, 584, 80, 616),
          "L1 volume sprite is exactly (584,48)..(616,80)");
    CHECK(rect_eq(finder_desk_sprite_rect(&f.fd, 1), 404, 584, 436, 616),
          "L1 Trash sprite is exactly (584,404)..(616,436)");

    /* The Trash cell must end ABOVE the locked cursor PARK rect at
     * (620,460)..(636,476) (spec/flair_solid_traces.mk PARK CONVENTION). */
    CHECK(finder_desk_cell_rect(&f.fd, 1).bottom <= 460,
          "L1 Trash cell ends above the locked cursor park row (y=460)");
    /* Both cells must clear the two menu bars entirely. */
    CHECK(finder_desk_cell_rect(&f.fd, 0).top >= 40,
          "L1 volume cell is below both menu bars (y>=40)");
}

/* ===========================================================================
 * L2 -- LABEL GEOMETRY
 * ===========================================================================*/
static void leg_geometry(void)
{
    fixture_t f;
    rgn_rect_t s, l, c;

    fixture_layout(&f, 0);

    s = finder_desk_sprite_rect(&f.fd, 0);
    l = finder_desk_label_rect(&f.fd, 0);
    c = finder_desk_cell_rect(&f.fd, 0);

    CHECK(rect_eq(s, 10, 10, 42, 42), "L2 sprite rect is 32x32 at the origin");
    CHECK(rect_eq(l, 44, 23, 57, 29),
          "L2 empty label band is 6x13 at (23,44) -- 2px under the sprite, "
          "centered on it");
    CHECK(l.top == s.bottom + FINDER_LABEL_GAP, "L2 label gap is FINDER_LABEL_GAP");
    CHECK(l.bottom - l.top == FINDER_LABEL_H, "L2 label band is FINDER_LABEL_H rows");
    CHECK(rect_eq(c, 10, 10, 57, 42), "L2 cell bbox is sprite UNION label");
    CHECK(c.bottom - c.top == FINDER_CELL_H, "L2 cell height is FINDER_CELL_H");

    /* A long label is clamped inside the bounds, never off the right edge. */
    {
        fixture_t g;
        rgn_rect_t gl;
        memset(&g, 0, sizeof g);
        finder_desk_init(&g.fd, g.icons, (uint16_t)FINDER_DESK_MAX_ICONS,
                         mk_rect(0, 0, 200, 200));
        (void)finder_desk_add(&g.fd, FINDER_ICON_VOLUME,
                              "WWWWWWWWWWWWW", 170, 10, 1u);
        gl = finder_desk_label_rect(&g.fd, 0);
        CHECK(gl.right <= 200, "L2 a long label is clamped to the right bound");
        CHECK(gl.left >= 0, "L2 a long label is clamped to the left bound");
    }
}

/* ===========================================================================
 * L3 -- HIT TEST
 * ===========================================================================*/
static void leg_hit(void)
{
    fixture_t f;
    fixture_layout(&f, 0);

    CHECK(finder_desk_hit(&f.fd, 10, 10) == 0, "L3 sprite top-left corner hits A");
    CHECK(finder_desk_hit(&f.fd, 41, 41) == 0, "L3 sprite bottom-right pixel hits A");
    CHECK(finder_desk_hit(&f.fd, 42, 41) == -1, "L3 one px right of the sprite misses");
    CHECK(finder_desk_hit(&f.fd, 41, 42) == -1, "L3 one px below the sprite misses");
    CHECK(finder_desk_hit(&f.fd,  9, 10) == -1, "L3 one px left of the sprite misses");

    CHECK(finder_desk_hit(&f.fd, 23, 44) == 0, "L3 label top-left pixel hits A");
    CHECK(finder_desk_hit(&f.fd, 28, 56) == 0, "L3 label bottom-right pixel hits A");
    CHECK(finder_desk_hit(&f.fd, 29, 50) == -1, "L3 one px right of the label misses");

    /* The DEAD CORNER beside the centered label: inside the cell BBOX, outside
     * both the sprite and the label. It must MISS (the bbox is not the target). */
    CHECK(finder_rect_contains(finder_desk_cell_rect(&f.fd, 0), 12, 50),
          "L3 (12,50) is inside the cell bounding box");
    CHECK(finder_desk_hit(&f.fd, 12, 50) == -1,
          "L3 the dead corner beside the label is NOT a hit (sprite UNION label, "
          "not the bbox)");

    CHECK(finder_desk_hit(&f.fd, 61, 11) == 1, "L3 B is hit at its own sprite");
    CHECK(finder_desk_hit(&f.fd, 11, 61) == 2, "L3 C is hit at its own sprite");

    /* TOPMOST WINS: D (index 3) overlaps A (index 0) at (25,25). */
    {
        fixture_t g;
        fixture_layout(&g, 1);
        CHECK(finder_desk_hit(&g.fd, 25, 25) == 3,
              "L3 topmost wins: the LATER record takes an overlapped point");
        CHECK(finder_desk_hit(&g.fd, 11, 11) == 0,
              "L3 the earlier record still owns the part D does not cover");
    }
}

/* ===========================================================================
 * L4 -- SELECTION
 * ===========================================================================*/
static void leg_selection(void)
{
    fixture_t f;
    fixture_layout(&f, 0);

    CHECK(finder_desk_selection_count(&f.fd) == 0u, "L4 nothing selected at init");

    finder_desk_select_only(&f.fd, 1);
    CHECK(finder_desk_selection_count(&f.fd) == 1u, "L4 plain click selects one");
    CHECK(f.fd.icons[1].selected == 1u && f.fd.icons[0].selected == 0u,
          "L4 plain click selects exactly the clicked icon");

    finder_desk_select_extend(&f.fd, 0);
    CHECK(finder_desk_selection_count(&f.fd) == 2u, "L4 shift-click EXTENDS");
    CHECK(f.fd.icons[0].selected == 1u && f.fd.icons[1].selected == 1u,
          "L4 shift-click keeps the previous selection");

    finder_desk_select_extend(&f.fd, 0);
    CHECK(finder_desk_selection_count(&f.fd) == 1u,
          "L4 shift-click on a SELECTED icon toggles it off");
    CHECK(f.fd.icons[1].selected == 1u, "L4 the other icon survives the toggle");

    finder_desk_select_only(&f.fd, 2);
    CHECK(finder_desk_selection_count(&f.fd) == 1u,
          "L4 a plain click REPLACES the selection");

    finder_desk_deselect_all(&f.fd);
    CHECK(finder_desk_selection_count(&f.fd) == 0u, "L4 click-away clears all");
}

/* ===========================================================================
 * L5 -- MARQUEE (hand-authored selected sets)
 * ===========================================================================*/
static int sel_mask(const finder_desk_t *fd)
{
    int m = 0;
    for (int i = 0; i < (int)fd->n; i++) if (fd->icons[i].selected) m |= (1 << i);
    return m;
}

static void leg_marquee(void)
{
    fixture_t f;
    rgn_rect_t band;

    fixture_layout(&f, 0);

    /* Half-open normalisation: the band CONTAINS both endpoint pixels. */
    band = finder_band_rect(30, 40, 10, 20);
    CHECK(rect_eq(band, 20, 10, 41, 31),
          "L5 band normalises to (10,20)..(31,41) -- both endpoints inside");

    /* EXCLUSIVE edge: a band ending one pixel short of A's sprite selects
     * nothing. Hand-authored expected set: {}. */
    (void)finder_desk_marquee_select(&f.fd, finder_band_rect(0, 0, 9, 9));
    CHECK(sel_mask(&f.fd) == 0, "L5 band ending at (9,9) selects NOTHING");

    /* INCLUSIVE edge: extend by one pixel and A is caught. Hand-authored: {A}.
     * This pair is exactly what the half-open +1 exists for -- the
     * MARQUEE_OFFBYONE mutant loses it. */
    CHECK(finder_desk_marquee_select(&f.fd, finder_band_rect(0, 0, 10, 10)) == 1u,
          "L5 band ending at (10,10) selects exactly one icon");
    CHECK(sel_mask(&f.fd) == 0x1, "L5 ... and that icon is A");

    /* A band across the top row catches A and B, never C. Hand-authored: {A,B}. */
    CHECK(finder_desk_marquee_select(&f.fd, finder_band_rect(0, 0, 100, 45)) == 2u,
          "L5 a top-row band selects two icons");
    CHECK(sel_mask(&f.fd) == 0x3, "L5 ... and they are A and B");

    /* A band over A's LABEL only (never its sprite). Hand-authored: {A}. */
    CHECK(finder_desk_marquee_select(&f.fd, finder_band_rect(24, 45, 26, 50)) == 1u,
          "L5 a band over the label alone selects the icon");
    CHECK(sel_mask(&f.fd) == 0x1, "L5 ... and it is A (labels are part of the target)");

    /* A band in the DEAD CORNER beside A's label selects nothing (the bbox is
     * not the target). Hand-authored: {}. */
    CHECK(finder_desk_marquee_select(&f.fd, finder_band_rect(10, 45, 20, 50)) == 0u,
          "L5 a band in the dead corner beside the label selects NOTHING");

    /* A marquee REPLACES the selection. */
    finder_desk_select_only(&f.fd, 2);
    CHECK(finder_desk_marquee_select(&f.fd, finder_band_rect(0, 0, 10, 10)) == 1u,
          "L5 a marquee replaces the previous selection");
    CHECK(sel_mask(&f.fd) == 0x1, "L5 ... leaving only the banded icon");
}

/* ===========================================================================
 * L6 -- DOUBLE-CLICK SYNTHESIS (hand-authored event streams)
 * ===========================================================================*/
static void leg_dblclick(void)
{
    finder_click_track_t t;

    /* Stream 1: two clicks on icon 0, 20 ticks apart, 2px/1px slop => DOUBLE.
     * Expected stream: SINGLE, DOUBLE. */
    finder_click_reset(&t);
    CHECK(finder_click_classify(&t, 0, 100, 100, 1000u) == FINDER_CLICK_SINGLE,
          "L6 the first click is always a SINGLE");
    CHECK(finder_click_classify(&t, 0, 102, 101, 1020u) == FINDER_CLICK_DOUBLE,
          "L6 20 ticks (== FINDER_DBLCLICK_TICKS) + 2px slop IS a double");

    /* Stream 2: a third click right after the double is a fresh SINGLE. */
    CHECK(finder_click_classify(&t, 0, 102, 101, 1030u) == FINDER_CLICK_SINGLE,
          "L6 a triple click is double-then-single (the double DISARMS)");

    /* Stream 3: 21 ticks is too slow => SINGLE, SINGLE. */
    finder_click_reset(&t);
    CHECK(finder_click_classify(&t, 0, 100, 100, 1000u) == FINDER_CLICK_SINGLE,
          "L6 stream 3 click 1 is a SINGLE");
    CHECK(finder_click_classify(&t, 0, 100, 100, 1021u) == FINDER_CLICK_SINGLE,
          "L6 21 ticks is NOT a double");

    /* Stream 4: 5 px horizontal slop is too far => SINGLE, SINGLE. */
    finder_click_reset(&t);
    (void)finder_click_classify(&t, 0, 100, 100, 1000u);
    CHECK(finder_click_classify(&t, 0, 105, 100, 1005u) == FINDER_CLICK_SINGLE,
          "L6 5 px horizontal slop is NOT a double");

    /* Stream 5: 5 px vertical slop is too far => SINGLE, SINGLE. */
    finder_click_reset(&t);
    (void)finder_click_classify(&t, 0, 100, 100, 1000u);
    CHECK(finder_click_classify(&t, 0, 100, 105, 1005u) == FINDER_CLICK_SINGLE,
          "L6 5 px vertical slop is NOT a double");

    /* Stream 6: 4 px in BOTH axes is exactly at the limit => DOUBLE. */
    finder_click_reset(&t);
    (void)finder_click_classify(&t, 0, 100, 100, 1000u);
    CHECK(finder_click_classify(&t, 0, 104, 96, 1004u) == FINDER_CLICK_DOUBLE,
          "L6 exactly 4 px in both axes IS a double");

    /* Stream 7: two different icons => SINGLE, SINGLE. */
    finder_click_reset(&t);
    (void)finder_click_classify(&t, 0, 100, 100, 1000u);
    CHECK(finder_click_classify(&t, 1, 100, 100, 1005u) == FINDER_CLICK_SINGLE,
          "L6 clicking a DIFFERENT icon is never a double");

    /* Stream 8: bare desktop (target -1) never doubles, however fast. */
    finder_click_reset(&t);
    (void)finder_click_classify(&t, -1, 300, 300, 1000u);
    CHECK(finder_click_classify(&t, -1, 300, 300, 1001u) == FINDER_CLICK_SINGLE,
          "L6 two clicks on bare desktop are never a double");
}

/* ===========================================================================
 * L7 -- ICON DRAG
 * ===========================================================================*/
static void leg_drag(void)
{
    fixture_t f;
    rgn_rect_t old_r, new_r;

    fixture_layout(&f, 0);

    /* Zero delta is a NOOP (that gesture was a click). */
    CHECK(finder_desk_drag_commit(&f.fd, 0, 0, 0, &old_r, &new_r) ==
          FINDER_DROP_NOOP, "L7 a zero-delta drop is a NOOP");
    CHECK(f.fd.icons[0].x == 10 && f.fd.icons[0].y == 10,
          "L7 a NOOP drop leaves the origin alone");

    /* An ordinary move commits, and reports both damage rects. */
    CHECK(finder_desk_drag_commit(&f.fd, 0, 20, 30, &old_r, &new_r) ==
          FINDER_DROP_MOVED, "L7 an in-bounds drop MOVES");
    CHECK(f.fd.icons[0].x == 30 && f.fd.icons[0].y == 40,
          "L7 the new origin is old + delta");
    CHECK(rect_eq(old_r, 10, 10, 57, 42), "L7 out_old is the pre-drag cell bbox");
    CHECK(rect_eq(new_r, 40, 30, 87, 62), "L7 out_new is the post-drag cell bbox");

    /* The Trash is fixed-position: the drop is REFUSED and nothing moves. */
    CHECK(finder_desk_drag_commit(&f.fd, 1, 5, 5, &old_r, &new_r) ==
          FINDER_DROP_REVERT, "L7 dragging the fixed Trash REVERTS");
    CHECK(f.fd.icons[1].x == 60 && f.fd.icons[1].y == 10,
          "L7 a REVERT leaves the Trash where it was");

    /* Clamping: the whole cell stays inside the 200x200 bounds. The maxima are
     * hand-computed: x <= 200-32 = 168, y <= 200-FINDER_CELL_H(47) = 153. */
    CHECK(finder_desk_drag_commit(&f.fd, 2, 500, 500, &old_r, &new_r) ==
          FINDER_DROP_MOVED, "L7 an out-of-bounds drop still commits (clamped)");
    CHECK(f.fd.icons[2].x == 168 && f.fd.icons[2].y == 153,
          "L7 the drop is clamped to (168,153) -- the whole cell stays on-desktop");

    CHECK(finder_desk_drag_commit(&f.fd, 2, -500, -500, &old_r, &new_r) ==
          FINDER_DROP_MOVED, "L7 a negative out-of-bounds drop commits (clamped)");
    CHECK(f.fd.icons[2].x == 0 && f.fd.icons[2].y == 0,
          "L7 the drop is clamped to the top-left bound");

    /* The live outline agrees with what the commit will do. */
    {
        fixture_t g;
        rgn_rect_t outline, committed;
        fixture_layout(&g, 0);
        outline = finder_desk_drag_outline(&g.fd, 0, 500, 500);
        (void)finder_desk_drag_commit(&g.fd, 0, 500, 500, &old_r, &committed);
        CHECK(rect_eq(outline, committed.top, committed.left,
                      committed.bottom, committed.right),
              "L7 the save-under outline rect equals the committed cell rect");
    }
}

/* ===========================================================================
 * L8 -- THE DESKTOP.DB RECORD CODEC (hand-authored bytes)
 * ===========================================================================*/

/* HAND-AUTHORED expected image for a two-icon desktop:
 *   volume "INITECH" at (584,48)   -- 584 = 0x0248, 48 = 0x0030
 *   trash  "Trash"   at (584,404)  -- 404 = 0x0194
 * Layout per finder_desktop.h Sec 9 (design F1.3): 8-byte header, then 24-byte
 * records at +0 kind, +1 flags, +2 dir_start, +4 name83[13], +17 grid_x,
 * +19 grid_y, +21 view_bits, +23 pad. Little-endian throughout.
 * Written out byte by byte -- NEVER produced by the encoder it grades. */
static const uint8_t DB_GOLDEN[56] = {
    /* header */
    0x49, 0x44, 0x42, 0x31,         /* 'I' 'D' 'B' '1'                        */
    0x01, 0x00,                     /* version = 1                            */
    0x02, 0x00,                     /* n_records = 2                          */
    /* record 0 -- volume */
    0x01,                           /* kind = 1 (volume-pos)                  */
    0x00,                           /* flags                                  */
    0x00, 0x00,                     /* dir_start = 0 (root)                   */
    0x49, 0x4E, 0x49, 0x54, 0x45, 0x43, 0x48,   /* "INITECH"                  */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,         /* name83 zero fill to 13     */
    0x48, 0x02,                     /* grid_x = 584                           */
    0x30, 0x00,                     /* grid_y = 48                            */
    0x00, 0x00,                     /* view_bits                              */
    0x00,                           /* pad                                    */
    /* record 1 -- trash */
    0x02,                           /* kind = 2 (trash-pos)                   */
    0x00,                           /* flags                                  */
    0x00, 0x00,                     /* dir_start                              */
    0x54, 0x72, 0x61, 0x73, 0x68,               /* "Trash"                    */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* zero fill to 13        */
    0x48, 0x02,                     /* grid_x = 584                           */
    0x94, 0x01,                     /* grid_y = 404                           */
    0x00, 0x00,                     /* view_bits                              */
    0x00                            /* pad                                    */
};

static void seed_two(fixture_t *f, int16_t vx, int16_t vy, int16_t tx, int16_t ty)
{
    memset(f, 0, sizeof *f);
    finder_desk_init(&f->fd, f->icons, (uint16_t)FINDER_DESK_MAX_ICONS,
                     mk_rect(DESK_TOP, DESK_LEFT, DESK_BOTTOM, DESK_RIGHT));
    (void)finder_desk_add(&f->fd, FINDER_ICON_VOLUME, "INITECH", vx, vy, 1u);
    (void)finder_desk_add(&f->fd, FINDER_ICON_TRASH,  "Trash",   tx, ty, 0u);
}

static void leg_db(void)
{
    fixture_t f, g;
    uint8_t buf[FINDER_DB_MAX_BYTES];
    uint8_t bad[FINDER_DB_MAX_BYTES];
    uint32_t n;

    seed_two(&f, 584, 48, 584, 404);
    memset(buf, 0xAA, sizeof buf);
    n = finder_desk_db_encode(&f.fd, buf, (uint32_t)sizeof buf);
    CHECK(n == 56u, "L8 encode writes 8 + 2*24 = 56 bytes");
    CHECK(memcmp(buf, DB_GOLDEN, sizeof DB_GOLDEN) == 0,
          "L8 the encoded image is BYTE-IDENTICAL to the hand-authored golden");

    CHECK(finder_desk_db_encode(&f.fd, buf, 55u) == 0u,
          "L8 encode refuses a too-small buffer (never overflows, Rule 2)");

    /* Round-trip: apply the golden to a desktop seeded at DIFFERENT positions
     * and the positions must land exactly on the hand-authored ones. */
    seed_two(&g, 100, 100, 200, 200);
    CHECK(finder_desk_db_apply(&g.fd, DB_GOLDEN, sizeof DB_GOLDEN) == FINDER_DB_OK,
          "L8 the golden applies cleanly");
    CHECK(g.fd.icons[0].x == 584 && g.fd.icons[0].y == 48,
          "L8 the volume position round-trips");
    CHECK(g.fd.icons[1].x == 584 && g.fd.icons[1].y == 404,
          "L8 the Trash position round-trips");

    /* Re-encoding the applied state reproduces the golden (fixpoint). */
    memset(buf, 0, sizeof buf);
    (void)finder_desk_db_encode(&g.fd, buf, (uint32_t)sizeof buf);
    CHECK(memcmp(buf, DB_GOLDEN, sizeof DB_GOLDEN) == 0,
          "L8 encode(apply(golden)) == golden (the codec is a fixpoint)");

    /* ---- corruption classes: rejected, and fd left UNTOUCHED (regen path) -- */
    seed_two(&g, 100, 100, 200, 200);

    memcpy(bad, DB_GOLDEN, sizeof DB_GOLDEN);
    bad[0] = 'X';
    CHECK(finder_desk_db_validate(bad, sizeof DB_GOLDEN) == FINDER_DB_ERR_MAGIC,
          "L8 a flipped magic byte is ERR_MAGIC");
    CHECK(finder_desk_db_apply(&g.fd, bad, sizeof DB_GOLDEN) == FINDER_DB_ERR_MAGIC,
          "L8 apply refuses a bad magic");
    CHECK(g.fd.icons[0].x == 100 && g.fd.icons[0].y == 100,
          "L8 a refused apply leaves the icons untouched (regen defaults)");

    memcpy(bad, DB_GOLDEN, sizeof DB_GOLDEN);
    bad[4] = 0x02;
    CHECK(finder_desk_db_validate(bad, sizeof DB_GOLDEN) == FINDER_DB_ERR_VER,
          "L8 a future version is ERR_VER");

    memcpy(bad, DB_GOLDEN, sizeof DB_GOLDEN);
    CHECK(finder_desk_db_validate(bad, 55u) == FINDER_DB_ERR_LEN,
          "L8 a truncated record array is ERR_LEN");
    CHECK(finder_desk_db_validate(bad, 7u) == FINDER_DB_ERR_LEN,
          "L8 a short-of-header image is ERR_LEN");

    memcpy(bad, DB_GOLDEN, sizeof DB_GOLDEN);
    bad[6] = 0x63; bad[7] = 0x00;   /* n_records = 99 */
    CHECK(finder_desk_db_validate(bad, sizeof DB_GOLDEN) == FINDER_DB_ERR_COUNT,
          "L8 n_records past the fixed cap is ERR_COUNT (fail loud, Rule 2)");

    /* An unknown record kind is IGNORED, not an error (item-pos / folder-view /
     * trash-origin belong to later slices; design F1.3). */
    memcpy(bad, DB_GOLDEN, sizeof DB_GOLDEN);
    bad[8] = 0x03;                  /* record 0 becomes item-pos              */
    seed_two(&g, 100, 100, 200, 200);
    CHECK(finder_desk_db_apply(&g.fd, bad, sizeof DB_GOLDEN) == FINDER_DB_OK,
          "L8 an unknown record kind is ignored, not an error");
    CHECK(g.fd.icons[0].x == 100, "L8 ... and it moves nothing");
    CHECK(g.fd.icons[1].y == 404, "L8 ... while the known record still applies");
}

/* ===========================================================================
 * L9 -- THE COMPOSED DESKTOP (the underlay seam)
 * ===========================================================================*/
enum { SCRW = 200, SCRH = 160 };

/* HAND-RESTATED canon (see INDEPENDENCE): the strike's INK tone resolves to
 * palette index 0 and the desktop background to index 2. Not read from
 * color_canon.h or flair_look.c. */
#define WANT_INK      0x00u
#define WANT_DESKTOP  0x02u

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

typedef struct win_store {
    WindowRecord rec;
    rgn_store_t  struc, cont, upd;
} win_store_t;

static void win_attach(win_store_t *w)
{
    memset(&w->rec, 0, sizeof w->rec);
    store_attach(&w->struc);
    store_attach(&w->cont);
    store_attach(&w->upd);
    w->rec.strucRgn   = &w->struc.r;
    w->rec.contRgn    = &w->cont.r;
    w->rec.updateRgn  = &w->upd.r;
    w->rec.nextWindow = NULL;
}

typedef struct scene {
    WindowMgr   wm;
    rgn_store_t desk, sa, sb, sc, comp;
    win_store_t win;
    fixture_t   f;
    uint8_t     px[SCRW * SCRH];
    bitmap_t    bm;
} scene_t;

/* Build + render one scene. `with_underlay` selects the ONLY difference. */
static void render_scene(scene_t *s, int with_underlay)
{
    rgn_rect_t frame = mk_rect(0, 0, (int16_t)SCRH, (int16_t)SCRW);
    rgn_rect_t wb    = mk_rect(20, 90, 140, 190);
    rgn_rect_t wc    = mk_rect(40, 92, 138, 188);

    memset(s->px, 0, sizeof s->px);
    s->bm.base            = s->px;
    s->bm.pitch           = (uint32_t)SCRW;
    s->bm.width           = (uint32_t)SCRW;
    s->bm.height          = (uint32_t)SCRH;
    s->bm.bpp             = 8u;
    s->bm.bytes_per_pixel = 1u;

    store_attach(&s->desk); store_attach(&s->sa);
    store_attach(&s->sb);   store_attach(&s->sc);
    store_attach(&s->comp);
    WindowMgr_init(&s->wm, frame, &s->desk.r, &s->sa.r, &s->sb.r, &s->sc.r);

    /* ONE icon, hand-placed so the window covers its right half:
     * sprite (70,30)..(102,62); the window's structure starts at x=90. */
    memset(&s->f, 0, sizeof s->f);
    finder_desk_init(&s->f.fd, s->f.icons, (uint16_t)FINDER_DESK_MAX_ICONS,
                     frame);
    (void)finder_desk_add(&s->f.fd, FINDER_ICON_VOLUME, "", 70, 30, 1u);

    if (with_underlay) finder_desk_install_underlay(&s->f.fd, &s->wm);

    win_attach(&s->win);
    NewWindow(&s->wm, &s->win.rec, wb, wc, documentKind, documentProc, 1);

    /* Full composite (exercises the desktop_paint_all underlay site, NULL clip),
     * then a damage cycle over a rect that spans the icon AND the window
     * (exercises the desktop_paint_damage site with the real desktop clip). */
    desktop_paint_all(&s->wm, &s->bm, &s->comp.r);
    WindowMgr_invalidate_desktop(&s->wm, mk_rect(20, 60, 120, 160));
    desktop_paint_damage(&s->wm, &s->bm, &s->comp.r);
}

static void leg_render(void)
{
    static scene_t A, B;
    int diffs = 0;
    int diff_outside_cell = 0;
    int diff_in_window = 0;
    rgn_rect_t cell;

    render_scene(&A, 1);
    render_scene(&B, 0);

    cell = finder_desk_cell_rect(&A.f.fd, 0);

    for (int y = 0; y < SCRH; y++) {
        for (int x = 0; x < SCRW; x++) {
            uint8_t a = A.px[y * SCRW + x];
            uint8_t b = B.px[y * SCRW + x];
            if (a == b) continue;
            diffs++;
            if (region_contains_point(A.win.rec.strucRgn, (int16_t)x, (int16_t)y))
                diff_in_window++;
            if (!finder_rect_contains(cell, (int16_t)x, (int16_t)y))
                diff_outside_cell++;
        }
    }

    CHECK(diffs > 0,
          "L9 installing the underlay CHANGES the composed desktop (the icons "
          "actually reach the frame through desktop.c's seam)");
    CHECK(diff_in_window == 0,
          "L9 NOT ONE pixel the window's structure region owns differs -- icons "
          "can never paint over window chrome (the clip is load-bearing)");
    CHECK(diff_outside_cell == 0,
          "L9 every changed pixel lies inside the icon's own cell rect (no "
          "over-repaint, ADR-0004 D-5)");

    /* Hand-read strike probes (spec/assets/desk_icons.h ASCII map, VOLUME):
     * row 3 col 3 is the body's top-left INK corner; the icon origin is
     * (70,30), so that pixel is (73,33) -- desktop-owned (x < 90). */
    CHECK(A.px[33 * SCRW + 73] == WANT_INK,
          "L9 the strike's row-3/col-3 ink corner is INK on the composed desktop");
    CHECK(B.px[33 * SCRW + 73] == WANT_DESKTOP,
          "L9 ... and is bare desktop teal without the underlay");

    /* Row 0 of the strike is entirely clear, so the icon's own top row must
     * still read desktop teal even WITH the underlay (transparency survives
     * the composite). */
    CHECK(A.px[30 * SCRW + 73] == WANT_DESKTOP,
          "L9 a CLEAR strike pixel still reads bare desktop (transparency)");

    /* Row 16 col 24 is '#' (ink) on the map and lands at x = 70+24 = 94, which
     * the window owns -- so it must read whatever the window painted, IDENTICAL
     * in both frames (already covered by diff_in_window, restated as a probe). */
    CHECK(A.px[46 * SCRW + 94] == B.px[46 * SCRW + 94],
          "L9 an icon pixel under the window is byte-identical with and without "
          "the underlay");

    /* Rule 11: the same scene rendered twice is byte-identical. */
    {
        static scene_t C;
        render_scene(&C, 1);
        CHECK(memcmp(A.px, C.px, sizeof A.px) == 0,
              "L9 the underlay is DETERMINISTIC -- two runs are byte-identical "
              "(the solid leg-D whole-frame identity gate depends on it)");
    }
}

/* ===========================================================================
 * main
 * ===========================================================================*/
int main(void)
{
    leg_defaults();
    leg_geometry();
    leg_hit();
    leg_selection();
    leg_marquee();
    leg_dblclick();
    leg_drag();
    leg_db();
    leg_render();
    return TEST_SUMMARY("test-finder-desktop");
}
