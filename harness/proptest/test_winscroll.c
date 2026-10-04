/*
 * harness/proptest/test_winscroll.c -- host oracle for the STANDARD WINDOW
 * SCROLL BARS (bead initech-tdnl.35; audit F02).
 *
 * WHAT IS GRADED, and where every expected value comes from (Law 2): the
 * numbers are HAND-DERIVED below from the default Finder frame
 * (finder_windows.h Sec 3: (20,60)..(380,280)) and spec/chrome_metrics.h --
 * frame 1, body rail 4, bar 16, arrow tile 16, thumb 15, grow offsets 19,
 * title band 22 -- and from the Inside Macintosh value<->thumb rule
 * (control-manager.md). Nothing is read back out of winscroll.h to build an
 * expectation.
 *
 *   W1 GEOMETRY   both bar rects of the default frame
 *                   V: x [380-21, 380-5) = [359,375), y [60+21, 280-20) = [81,260)
 *                   H: y [280-21, 280-5) = [259,275), x [20+5, 380-20) = [25,360)
 *   W2 THUMB      V track: lo = 81+16 = 97; span = (260-16-15) - 97 = 132.
 *                 max 100: value 0 -> 97, 50 -> 97+66 = 163, 100 -> 229;
 *                 inverse rounds: pos 163 -> 50; and value->pos->value is the
 *                 identity for every value when span >= max (property).
 *   W3 PARTS      max 100 value 50 (thumb [163,178)) at x = 367:
 *                 y 85 UP(20), 96 UP, 97 PAGEUP(22), 162 PAGEUP, 163 THUMB(129),
 *                 177 THUMB, 178 PAGEDOWN(23), 243 PAGEDOWN, 244 DOWN(21), 259 DOWN;
 *                 a DISABLED bar (max 0) answers 0 everywhere.
 *   W4 STEPS      extent: content 316 in a view of 177 -> max 139, page 161;
 *                 down from 0 -> 16; page down from 16 -> 139 (clamped);
 *                 page up from 139 -> 0 (clamped); up at 0 changes nothing.
 *   W5 FINDWINDOW every pixel of both bands of a live document window answers
 *                 inContent, NEVER inDrag (audit F02); the title row above the
 *                 vertical bar stays inDrag; the right body rail stays inDrag.
 *   W6 FINDCONTROL WindowScrollFindPart: no record -> 0; record with max 0 -> 0;
 *                 enabled -> the W3 parts; an INACTIVE window -> 0;
 *                 DisposeWindow detaches the record.
 *
 * MUTANTS (Rule 6; Makefile test-winscroll-mutant):
 *   WINDOW_MUTATE_SCROLL_DRAG   bands fall through to inDrag   -> W5 RED
 *   WSCROLL_MUT_THUMB_UNSCALED  thumb ignores the range        -> W2 RED
 *   WSCROLL_MUT_PAGE_WRONG_DIR  page regions scroll backwards  -> W4 RED
 *
 * Ref: os/flair/winscroll.h, os/flair/window.h Sec 4b;
 *      ../system7-decomp/specs/sys8/scrollbars.md Sec 1-3;
 *      ../system7-decomp/specs/toolbox/control-manager.md.
 * ASCII-clean (Rule 12).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "region_algebra.h"
#include "region.h"
#include "window_record.h"
#include "chrome_metrics.h"
#include "window.h"
#include "test_assert.h"

TEST_HARNESS();

typedef struct rgn_store {
    region_t   r;
    rgn_row_t  rows[RGN_ROWS_CAP];
    int16_t    pool[RGN_X_POOL_CAP];
} rgn_store_t;

static void store_attach(rgn_store_t *s)
{
    memset(s, 0, sizeof *s);
    s->r.rows = s->rows;
    s->r.cap_rows = RGN_ROWS_CAP;
    s->r.x_pool = s->pool;
    s->r.x_pool_cap = RGN_X_POOL_CAP;
    region_set_empty(&s->r);
}

static WindowRecord g_rec;
static rgn_store_t g_struc, g_cont, g_upd, g_desk, g_sa, g_sb, g_sc;
static WindowMgr g_wm;

static flair_point_t P(int h, int v)
{
    flair_point_t p;
    p.h = (int16_t)h;
    p.v = (int16_t)v;
    return p;
}

static WindowScrollAxis axis_of(int value, int max)
{
    WindowScrollAxis a;
    memset(&a, 0, sizeof a);
    a.value = (int16_t)value;
    a.max = (int16_t)max;
    a.line = 16;
    a.drag_pos = -1;
    return a;
}

int main(void)
{
    rgn_rect_t frame = { 60, 20, 280, 380 };   /* top,left,bottom,right */

    /* ---- W1 GEOMETRY ---------------------------------------------------- */
    {
        rgn_rect_t v = wscroll_bar_rect(frame, WSCROLL_V);
        rgn_rect_t h = wscroll_bar_rect(frame, WSCROLL_H);
        CHECK(v.left == 359 && v.right == 375 && v.top == 81 && v.bottom == 260,
              "W1 vertical bar = x[359,375) y[81,260) (the chrome.c gutter lines)");
        CHECK(h.top == 259 && h.bottom == 275 && h.left == 25 && h.right == 360,
              "W1 horizontal bar = x[25,360) y[259,275)");
    }

    /* ---- W2 THUMB ------------------------------------------------------- */
    {
        rgn_rect_t v = wscroll_bar_rect(frame, WSCROLL_V);
        WindowScrollAxis a = axis_of(0, 100);
        int ok = wscroll_thumb_pos(v, WSCROLL_V, &a) == 97;
        a.value = 50;  ok = ok && wscroll_thumb_pos(v, WSCROLL_V, &a) == 163;
        a.value = 100; ok = ok && wscroll_thumb_pos(v, WSCROLL_V, &a) == 229;
        CHECK(ok, "W2 thumb leading edge = 97 + value*132/100: 97, 163, 229");
        CHECK(wscroll_value_from_thumb(v, WSCROLL_V, &a, 163) == 50 &&
              wscroll_value_from_thumb(v, WSCROLL_V, &a, 40) == 0 &&
              wscroll_value_from_thumb(v, WSCROLL_V, &a, 400) == 100,
              "W2 inverse: 163 -> 50; above/below the track clamp to 0/100");
        int rt_bad = 0;
        for (int max = 1; max <= 132 && !rt_bad; max++) {
            WindowScrollAxis b = axis_of(0, max);
            for (int val = 0; val <= max; val++) {
                b.value = (int16_t)val;
                int pos = wscroll_thumb_pos(v, WSCROLL_V, &b);
                if (wscroll_value_from_thumb(v, WSCROLL_V, &b, pos) != val) {
                    rt_bad = 1;
                    break;
                }
            }
        }
        CHECK(!rt_bad, "W2 value -> thumb -> value is the identity whenever span >= max");
        WindowScrollAxis z = axis_of(0, 0);
        CHECK(wscroll_thumb_pos(v, WSCROLL_V, &z) == 97 && !wscroll_enabled(&z),
              "W2 max 0: DISABLED, no range");
    }

    /* ---- W3 PARTS ------------------------------------------------------- */
    {
        rgn_rect_t v = wscroll_bar_rect(frame, WSCROLL_V);
        WindowScrollAxis a = axis_of(50, 100);
        static const struct { int y; int part; } T[] = {
            {  85, 20 }, {  96, 20 }, {  97, 22 }, { 162, 22 }, { 163, 129 },
            { 177, 129 }, { 178, 23 }, { 243, 23 }, { 244, 21 }, { 259, 21 }
        };
        int bad = 0;
        for (unsigned i = 0; i < sizeof T / sizeof T[0]; i++)
            if (wscroll_test_part(v, WSCROLL_V, &a, P(367, T[i].y)) != T[i].part) {
                printf("    W3 y=%d want %d got %d\n", T[i].y, T[i].part,
                       wscroll_test_part(v, WSCROLL_V, &a, P(367, T[i].y)));
                bad = 1;
            }
        CHECK(!bad, "W3 part codes along the vertical bar (IM inUpButton..inThumb)");
        CHECK(wscroll_test_part(v, WSCROLL_V, &a, P(358, 150)) == 0 &&
              wscroll_test_part(v, WSCROLL_V, &a, P(375, 150)) == 0,
              "W3 outside the bar's columns is no part");
        WindowScrollAxis z = axis_of(0, 0);
        int zbad = 0;
        for (int y = 81; y < 260; y++)
            if (wscroll_test_part(v, WSCROLL_V, &z, P(367, y)) != 0) zbad = 1;
        CHECK(!zbad, "W3 a DISABLED bar has no parts (inactive controls are never hit)");
        rgn_rect_t h = wscroll_bar_rect(frame, WSCROLL_H);
        WindowScrollAxis b = axis_of(0, 10);
        CHECK(wscroll_test_part(h, WSCROLL_H, &b, P(30, 267)) == 20 &&
              wscroll_test_part(h, WSCROLL_H, &b, P(41, 267)) == 129 &&
              wscroll_test_part(h, WSCROLL_H, &b, P(200, 267)) == 23 &&
              wscroll_test_part(h, WSCROLL_H, &b, P(350, 267)) == 21,
              "W3 horizontal bar: left arrow, thumb at 41 (value 0), page right, right arrow");
    }

    /* ---- W4 STEPS ------------------------------------------------------- */
    {
        WindowScrollAxis a = axis_of(0, 0);
        wscroll_set_extent(&a, 316, 177);
        CHECK(a.max == 139 && a.page == 161 && a.value == 0,
              "W4 extent 316 in 177: max 139, page 161");
        CHECK(wscroll_step(&a, 21) == 1 && a.value == 16, "W4 down arrow: 0 -> 16");
        CHECK(wscroll_step(&a, 23) == 1 && a.value == 139,
              "W4 page down: 16 -> 139 (clamped at max)");
        CHECK(wscroll_step(&a, 23) == 0 && a.value == 139, "W4 page down at max: no change");
        CHECK(wscroll_step(&a, 22) == 1 && a.value == 0, "W4 page up: 139 -> 0 (clamped)");
        CHECK(wscroll_step(&a, 20) == 0 && a.value == 0, "W4 up arrow at 0: no change");
        a.value = 139;
        wscroll_set_extent(&a, 200, 177);
        CHECK(a.max == 23 && a.value == 23, "W4 content shrinks: the value re-clamps to the new max");
    }

    /* ---- W5 FINDWINDOW + W6 FINDCONTROL ---------------------------------- */
    {
        rgn_rect_t desk = { 0, 0, 480, 640 };
        store_attach(&g_desk); store_attach(&g_sa); store_attach(&g_sb); store_attach(&g_sc);
        WindowMgr_init(&g_wm, desk, &g_desk.r, &g_sa.r, &g_sb.r, &g_sc.r);
        memset(&g_rec, 0, sizeof g_rec);
        store_attach(&g_struc); store_attach(&g_cont); store_attach(&g_upd);
        g_rec.strucRgn = &g_struc.r;
        g_rec.contRgn = &g_cont.r;
        g_rec.updateRgn = &g_upd.r;
        NewDocumentWindow(&g_wm, &g_rec, frame, documentKind, 1);

        WindowPtr hit = NULL;
        int band_bad = 0, drag_in_band = 0;
        for (int y = 82; y < 260 && !band_bad; y++)
            for (int x = 359; x < 375; x++) {
                flair_part_code_t pc = FindWindow(&g_wm, P(x, y), &hit);
                if (pc == inDrag) drag_in_band = 1;
                if (pc != inContent || hit != &g_rec ||
                    WindowScrollBand(&g_rec, P(x, y)) != WSCROLL_V) { band_bad = 1; break; }
            }
        for (int y = 259; y < 275 && !band_bad; y++)
            for (int x = 25; x < 359; x++) {
                flair_part_code_t pc = FindWindow(&g_wm, P(x, y), &hit);
                if (pc == inDrag) drag_in_band = 1;
                if (pc != inContent || WindowScrollBand(&g_rec, P(x, y)) != WSCROLL_H) {
                    band_bad = 1;
                    break;
                }
            }
        CHECK(!band_bad && !drag_in_band,
              "W5 every pixel of both scroll bars is inContent, never inDrag (audit F02)");
        CHECK(FindWindow(&g_wm, P(367, 81), &hit) == inDrag &&
              WindowScrollBand(&g_rec, P(367, 81)) == -1,
              "W5 the title band's shared bottom row (y 81) stays the drag region");
        CHECK(FindWindow(&g_wm, P(376, 150), &hit) == inDrag &&
              FindWindow(&g_wm, P(200, 150), &hit) == inContent &&
              WindowScrollBand(&g_rec, P(200, 150)) == -1,
              "W5 the right body rail stays inDrag; real content is content, not a band");
        CHECK(!region_contains_point(g_rec.contRgn, 200, 265) &&
              region_contains_point(g_rec.contRgn, 200, 258),
              "W5 contRgn stops at the horizontal bar (y 259)");

        int axis = 9;
        CHECK(WindowScrollFindPart(&g_wm, &g_rec, P(367, 200), &axis) == 0 &&
              axis == WSCROLL_V,
              "W6 no scroll record: the bar is DISABLED, no part");
        static WindowScroll ws;
        WindowScrollAttach(&g_wm, &ws, &g_rec);
        CHECK(WindowScrollOf(&g_wm, &g_rec) == &ws &&
              WindowScrollFindPart(&g_wm, &g_rec, P(367, 200), &axis) == 0,
              "W6 attached with max 0: still DISABLED");
        ws.axis[WSCROLL_V].max = 100;
        ws.axis[WSCROLL_V].value = 50;
        CHECK(WindowScrollFindPart(&g_wm, &g_rec, P(367, 85), &axis) == 20 &&
              WindowScrollFindPart(&g_wm, &g_rec, P(367, 170), &axis) == 129 &&
              WindowScrollFindPart(&g_wm, &g_rec, P(367, 200), &axis) == 23 &&
              axis == WSCROLL_V,
              "W6 enabled: up arrow, thumb, page down at the W3 coordinates");
        g_rec.hilited = 0;
        CHECK(WindowScrollFindPart(&g_wm, &g_rec, P(367, 200), &axis) == 0,
              "W6 an INACTIVE window's bar has no part (the click only activates)");
        g_rec.hilited = 1;
        DisposeWindow(&g_wm, &g_rec);
        CHECK(WindowScrollOf(&g_wm, &g_rec) == NULL && g_wm.scrolls == NULL,
              "W6 DisposeWindow detaches the scroll record");
    }

    return TEST_SUMMARY("test-winscroll");
}
