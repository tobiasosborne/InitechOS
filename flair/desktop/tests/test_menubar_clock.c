/* tdnl.13, host oracle FIRST. Literal hours / boundary readings are independent
 * of MenuClock_format; pixel expectations use the original reference NFNT.
 * Ref: s8_desktop_menubar.png "12:56 am", s8_menu_dropped.png "1:00 am";
 * PRD Sec 6.3/6.4; GUI-remediation-plan R3.6. ASCII-only, no environment. */
#include <stdio.h>
#include <string.h>
#include "menu.h"
#include "render.h"
#include "menubar_clock_oracle.h"
#include "test_assert.h"
TEST_HARNESS();

int main(int argc, char **argv)
{
    static const char *hours[24] = {
        "12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11",
        "12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"
    };
    if (argc != 3) return 2;
    int bad = 0;
    for (unsigned h = 0; h < 24; h++) for (unsigned m = 0; m < 60; m++) {
        char actual[9], want[9];
        snprintf(want, sizeof want, "%s:%02u %s", hours[h], m, h < 12 ? "am" : "pm");
        bad += !MenuClock_format(h, m, actual) || strcmp(actual, want) != 0;
    }
    CHECK(!bad, "MC1 1440 readings: twelve-hour, unpadded hour, two-digit minute, lowercase am/pm");
    char text[9] = "canary";
    CHECK(!MenuClock_format(24, 0, text) && !strcmp(text, "canary"), "MC2 invalid hour preserves buffer");
    CHECK(!MenuClock_format(0, 60, text) && !MenuClock_format(0, 0, NULL), "MC2 invalid minute/null refused");
    CHECK(MenuClock_set(0, 55) && !strcmp(MenuClock_text(), "12:55 am"), "MC3 initial RTC minute");
    CHECK(!MenuClock_set(0, 55), "MC3 unchanged minute does not repaint");
    CHECK(MenuClock_set(0, 56) && !strcmp(MenuClock_text(), "12:56 am"), "MC4 minute tick refreshes clock");
    CHECK(MenuClock_set(12, 56) && !strcmp(MenuClock_text(), "12:56 pm"), "MC4 same minute at noon changes suffix");
    CHECK(MenuClock_set(0, 56), "MC4 midnight changes suffix");
    uint32_t want[20][80];
    if (menubar_clock_golden(argv[1], argv[2], want)) return 1;
    for (unsigned bpp = 8; bpp <= 32; bpp += 24) {
        render_ctx_t r;
        render_boot_info_t b = { 0, 0, bpp, 640, 480 };
        if (render_ctx_init(&r, &b)) return 2;
        MenuBar empty = { NULL, 0, 0 };
        MenuClock_attach(&empty);
        DrawMenuBar(&r.port, &empty, -1, NULL);
        bad = 0;
        for (unsigned y = 0; y < 20; y++) for (unsigned x = 0; x < 80; x++) {
            uint32_t got = render_pixel_rgb(&r, 520 + x, y);
            if (got != want[y][x]) {
                if (bad < 16) fprintf(stderr, "MC5 bpp%u (%u,%u) got%06x want%06x\n",
                                      bpp, 520 + x, y, got, want[y][x]);
                bad++;
            }
        }
        if (bad) fprintf(stderr, "MC5 bpp%u: %d/1600 differences\n", bpp, bad);
        CHECK(!bad, "MC5 sampled Mac bar/anchor + independent original Chicago12 pixels (both depths)");
        MenuBar tenant = { NULL, 0, 0 };
        DrawMenuBar(&r.port, &tenant, -1, NULL);
        bad = 0;
        for (unsigned y = 1; y < 18; y++) for (unsigned x = 0; x < 80; x++)
            bad += render_pixel_rgb(&r, 520 + x, y) != want[y][0];
        CHECK(!bad, "MC6 unbound tenant bar has no duplicate clock");
        rgn_rect_t rect = { 7, 550, 12, 560 };
        region_set_rect(&r.clipRgn, rect);
        MenuClock_draw(&r.port, &r.clipRgn);
        bad = 0;
        for (unsigned y = 1; y < 18; y++) for (unsigned x = 0; x < 80; x++) {
            uint32_t expected = (x >= 30 && x < 40 && y >= 7 && y < 12)
                                ? want[y][x] : want[y][0];
            bad += render_pixel_rgb(&r, 520 + x, y) != expected;
        }
        CHECK(!bad, "MC7 clock erase and glyphs respect the effective clip");
        MenuClock_attach(NULL);
        render_ctx_free(&r);
    }
    return TEST_SUMMARY("test-menubar-clock");
}
