/*
 * harness/proptest/test_apple_menu.c -- the HOST ORACLE of the system Apple
 * menu (factory C; beads initech-tdnl.47 / initech-tdnl.17).
 *
 *   A1 ROWS     apple_menu_build lists About This Computer..., a divider and
 *               exactly the INSTALLED accessories (rule F4-3: omit, never
 *               gray); nothing installed -> About alone, no divider.
 *   A2 ORDER    the locked table (spec/apple_menu_items.h) is alphabetical,
 *               as the real Apple menu lists Apple Menu Items
 *               (s8_apple_menu.png).
 *   A3 ACTION   a MenuSelect word maps back: item 1 About, an accessory row
 *               its program, the divider / a foreign menu / out of range
 *               nothing.
 *   A4 SLOT     menu.c's Apple slot: dead (-1) until a menu is registered
 *               (every older host oracle's view), FLAIR_MENU_APPLE_MI once
 *               one is, on a has_apple bar only; the File slot is unmoved.
 *   A5 CAPTURE  the pulled block (x 9..37) and the panel's left edge (x 9)
 *               measured off the real Mac OS 8.1 capture equal the Menu
 *               Manager's MenuBar_hilite_rect / MenuInfo_panel_rect (LOUD-SKIP
 *               when ../system7-decomp is absent).
 *   A6 TRACK    press the glyph, drag to the Calculator row, release ->
 *               (1<<16)|3; on the divider -> 0; a drag onto File switches
 *               the tracked menu (cross-menu drag, rl4v) and back.
 *
 * Mutants (Rule 6): MENU_MUT_APPLE_DEAD (menu.c; A4/A6 RED) and
 * APPLE_MENU_MUT_LIST_ALL (apple_menu.c; A1 RED).
 * Usage: test_apple_menu <system7-decomp root>. ASCII-clean (Rule 12).
 */
#define _POSIX_C_SOURCE 200809L
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "menu.h"
#include "apple_menu.h"

static int g_fail = 0, g_checks = 0;
#define CHECK(leg, cond, ...) do { g_checks++; if (!(cond)) { g_fail++; \
    printf("FAIL %s: ", leg); printf(__VA_ARGS__); printf("\n"); } } while (0)

static int g_calls = 0;
static int all_installed(void *u, const char *d, const char *f)
{ (void)u; g_calls++; return strcmp(d, "APPS") == 0 && strcmp(f, "CALCULAT.EXE") == 0; }
static int none_installed(void *u, const char *d, const char *f)
{ (void)u; (void)d; (void)f; return 0; }
static int three_installed(void *u, const char *d, const char *f)
{ (void)u; return !strcmp(d,"APPS") && (!strcmp(f,"CALCULAT.EXE") ||
    !strcmp(f,"NOTEPAD.EXE") || !strcmp(f,"SCRAPBK.EXE")); }

static MenuItem file_items[] = { { "Quit", 0, 'Q', 0u, 1u, 0u } };
static MenuInfo file_menu[] = { { 129, "File", file_items, 1u, 0 } };

static int png_load(const char *path, int *w, int *h, unsigned char **rgb)
{
    png_image p;
    memset(&p, 0, sizeof p);
    p.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&p, path)) return 0;
    p.format = PNG_FORMAT_RGB;
    *w = (int)p.width; *h = (int)p.height;
    *rgb = malloc(PNG_IMAGE_SIZE(p));
    return png_image_finish_read(&p, NULL, *rgb, 0, NULL) != 0;
}

int main(int argc, char **argv)
{
    const char *root = argc > 1 ? argv[1] : "../system7-decomp";

    /* A1 ROWS */
    const MenuInfo *m = apple_menu_build(none_installed, NULL);
    CHECK("A1 ROWS", m->n_items == 1u && m->menuID == FLAIR_MENU_ID_APPLE,
          "nothing installed: %u rows (want About alone)", m->n_items);
    m = apple_menu_build(all_installed, NULL);
    CHECK("A1 ROWS", m->n_items == 3u, "Calculator installed: %u rows (want 3)", m->n_items);
    CHECK("A1 ROWS", m->n_items >= 1u && strcmp(m->items[0].text, "About This Computer\311") == 0 &&
          m->items[0].enabled && !m->items[0].is_divider, "row 1 is not an enabled About This Computer...");
    CHECK("A1 ROWS", m->n_items >= 2u && m->items[1].is_divider && !m->items[1].enabled, "row 2 is not a divider");
    CHECK("A1 ROWS", m->n_items >= 3u && strcmp(m->items[2].text, "Calculator") == 0 && m->items[2].enabled,
          "row 3 is not an enabled Calculator");
    CHECK("A1 ROWS", g_calls >= 1, "the installed question was never asked");

    /* A2 ORDER */
    int n;
    const apple_da_t *t = apple_menu_table(&n);
    for (int i = 1; i < n; i++)
        CHECK("A2 ORDER", strcmp(t[i - 1].title, t[i].title) < 0,
              "\"%s\" before \"%s\": not alphabetical", t[i - 1].title, t[i].title);

    /* A3 ACTION */
    const apple_da_t *da = NULL;
    CHECK("A3 ACTION", apple_menu_action(MenuResult(FLAIR_MENU_ID_APPLE, 1), &da) == APPLE_ACT_ABOUT, "item 1 is not About");
    CHECK("A3 ACTION", apple_menu_action(MenuResult(FLAIR_MENU_ID_APPLE, 3), &da) == APPLE_ACT_LAUNCH &&
          da && strcmp(da->file83, "CALCULAT.EXE") == 0 && strcmp(da->dir83, "APPS") == 0, "item 3 does not launch APPS\\CALCULAT.EXE");
    CHECK("A3 ACTION", apple_menu_action(MenuResult(FLAIR_MENU_ID_APPLE, 2), &da) == APPLE_ACT_NONE, "the divider acts");
    CHECK("A3 ACTION", apple_menu_action(MenuResult(FLAIR_MENU_ID_APPLE, 4), &da) == APPLE_ACT_NONE, "item 4 acts");
    CHECK("A3 ACTION", apple_menu_action(MenuResult(129, 1), &da) == APPLE_ACT_NONE, "a File choice acts as Apple");

    /* A4 SLOT */
    MenuBar bar = { file_menu, 1u, 1u };
    MenuBar plain = { file_menu, 1u, 0u };
    MenuBar_set_apple_menu(NULL);
    CHECK("A4 SLOT", MenuBar_hit(&bar, 20) == -1, "no Apple menu registered, yet the slot hits");
    MenuBar_set_apple_menu(m);
    CHECK("A4 SLOT", MenuBar_hit(&bar, 20) == FLAIR_MENU_APPLE_MI, "the Apple slot (x 20) hits %d", MenuBar_hit(&bar, 20));
    CHECK("A4 SLOT", MenuBar_hit(&bar, 0) == FLAIR_MENU_APPLE_MI, "the Apple slot (x 0) hits %d", MenuBar_hit(&bar, 0));
    CHECK("A4 SLOT", MenuBar_hit(&bar, FLAIR_MENU_APPLE_W) == 0, "File no longer starts at the slot end");
    CHECK("A4 SLOT", MenuBar_hit(&plain, 5) == 0, "a bar without the glyph drops the Apple menu");
    CHECK("A4 SLOT", MenuBar_menu_id(&bar, FLAIR_MENU_APPLE_MI) == FLAIR_MENU_ID_APPLE, "the slot's menu id");

    /* A5 CAPTURE */
    {
        char path[512];
        int w, h;
        unsigned char *rgb;
        snprintf(path, sizeof path, "%s/goldens/captures/s8_apple_menu.png", root);
        if (png_load(path, &w, &h, &rgb)) {
            const unsigned char *c = &rgb[3 * (1 * w + 12)];
            int l = 12, r = 12;
            while (l > 0 && !memcmp(&rgb[3 * (1 * w + l - 1)], c, 3)) l--;
            while (r < w - 1 && !memcmp(&rgb[3 * (1 * w + r + 1)], c, 3)) r++;
            rgn_rect_t hr = MenuBar_hilite_rect(&bar, FLAIR_MENU_APPLE_MI);
            CHECK("A5 CAPTURE", hr.left == l && hr.right == r + 1,
                  "pulled block [%d,%d), the capture's [%d,%d)", hr.left, hr.right, l, r + 1);
            int pl = -1;
            for (int x = 0; x < 40; x++) {
                const unsigned char *p = &rgb[3 * (60 * w + x)];
                if (!p[0] && !p[1] && !p[2]) { pl = x; break; }
            }
            rgn_rect_t pr = MenuInfo_panel_rect(&bar, FLAIR_MENU_APPLE_MI);
            CHECK("A5 CAPTURE", pr.left == pl, "panel left %d, the capture's %d", pr.left, pl);
            CHECK("A5 CAPTURE", pr.top == 19, "panel top %d, the capture's 19 (bar baseline)", pr.top);
            free(rgb);
        } else {
            printf("LOUD-SKIP A5 CAPTURE: %s absent (override SYSTEM7_DECOMP=...); 3 checks NOT graded\n", path);
        }
    }

    /* A6 TRACK */
    {
        flair_point_t start = { 9, 20 };           /* (v, h) */
        flair_point_t calc[] = { { 30, 40 }, { 50, 60 } };
        flair_point_t divider[] = { { 39, 60 } };
        flair_point_t cross[] = { { 9, 45 }, { 9, 20 }, { 50, 60 } };
        uint32_t r = MenuSelect(&bar, start, calc, 2);
        CHECK("A6 TRACK", r == MenuResult(FLAIR_MENU_ID_APPLE, 3), "glyph -> Calculator gives 0x%08X", r);
        r = MenuSelect(&bar, start, divider, 1);
        CHECK("A6 TRACK", r == 0u, "a release on the divider gives 0x%08X", r);
        MenuTrackState st;
        MenuTrack_begin(&bar, start, &st);
        CHECK("A6 TRACK", st.mi == FLAIR_MENU_APPLE_MI, "the press drops %d", st.mi);
        MenuTrack_step(&bar, &st, cross[0]);
        CHECK("A6 TRACK", st.mi == 0, "a drag onto File tracks %d", st.mi);
        MenuTrack_step(&bar, &st, cross[1]);
        MenuTrack_step(&bar, &st, cross[2]);
        CHECK("A6 TRACK", MenuTrack_result(&bar, &st) == MenuResult(FLAIR_MENU_ID_APPLE, 3),
              "back onto the glyph and down to Calculator gives 0x%08X", MenuTrack_result(&bar, &st));
    }

    /* R4.4: installed DA rows and actions, independently named. */
    m=apple_menu_build(three_installed,NULL);
    CHECK("A1 ROWS",m->n_items==5,"three DAs must make five rows");
    CHECK("A3 ACTION",apple_menu_action(MenuResult(FLAIR_MENU_ID_APPLE,4),&da)==APPLE_ACT_LAUNCH &&
          da && !strcmp(da->file83,"NOTEPAD.EXE") && !strcmp(da->title,"Note Pad"),"Note Pad row/action");
    CHECK("A3 ACTION",apple_menu_action(MenuResult(FLAIR_MENU_ID_APPLE,5),&da)==APPLE_ACT_LAUNCH &&
          da && !strcmp(da->file83,"SCRAPBK.EXE") && !strcmp(da->title,"Scrapbook"),"Scrapbook row/action");
    printf("test_apple_menu: %d checks, %d failures\n", g_checks, g_fail);
    return g_fail ? 1 : 0;
}
