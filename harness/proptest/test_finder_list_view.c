/* tdnl.44 host oracle. Ref: PRD 6.3/6.5; design F1.3/F3.3;
 * finder_menu.c's existing item numbers; finder_windows.h's stated product
 * policy. ALL expected orders, bit patterns, byte records and coordinates
 * below are independently hand-authored. Geometry numbers are SYNTHETIC
 * arithmetic cases, never claimed Mac OS 8 metrics or pixel goldens.
 * The UI is blocked on a real Mac OS 8.1 Finder View > as List capture.
 * Rule 6 mutants change only finder_windows.c, never these expectations.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "finder_windows.h"
#include "menu.h"
#include "test_assert.h"

TEST_HARNESS();

static void modes(void)
{
    static const struct { uint16_t item, bits; } cases[] = {
        {1,0xa500}, {2,0xa502}, {3,0xa501}, {4,0xa505},
        {5,0xa509}, {7,0xa50d}
    };
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        CHECK(finder_win_view_choose(0xa50d, cases[i].item) == cases[i].bits,
              "V1 mode transitions preserve unrelated view bits");
        CHECK(finder_win_view_choose(cases[i].bits, cases[i].item) == cases[i].bits,
              "V1 reselecting a mode is idempotent");
    }
    for (unsigned i = 0; i < 14; i++) {
        if (i == 1 || i == 2 || i == 3 || i == 4 || i == 5 || i == 7) continue;
        CHECK(finder_win_view_choose(0xa505, (uint16_t)i) == 0xa505,
              "V1 Label, Buttons, Pop-up and non-mode items cannot change mode");
    }
}

static const finder_list_key_t KEYS[] = {
    {"Z.TXT",        9, 0x5021, 0x0100, 0x20},
    {"A.EXE",        9, 0x5021, 0x0200, 0x20},
    {"FOLDER",       0, 0x5022, 0x0000, 0x10},
    {"a.exe",        9, 0x5021, 0x0200, 0x20},
    {"A.TXT",        9, 0x5021, 0x0100, 0x20},
    {"12345678.XYZ", 0xffffffffu, 0x5020, 0x0000, 0x20},
    {"A",            0, 0x5022, 0x0000, 0x20}
};

static void sorts(void)
{
    static const struct { uint16_t bits; int order[7]; } cases[] = {
        {1,  {5,6,1,3,4,2,0}}, /* ASCII case folded, full dotted name */
        {5,  {6,2,1,3,4,0,5}}, /* unsigned bytes, name tie break */
        {9,  {2,5,6,4,0,1,3}}, /* existing kind enum: folder, file, app */
        {13, {5,4,0,1,3,6,2}}  /* packed FAT date then time, name ties */
    };
    for (unsigned c = 0; c < sizeof cases / sizeof cases[0]; c++) {
        for (int rank = 0; rank < 7; rank++) {
            CHECK(finder_win_list_rank(KEYS, 7, cases[c].order[rank],
                                        cases[c].bits) == rank,
                  "V2 stable rank matches independent 8.3 ordering");
        }
        for (int i = 0; i < 7; i++) {
            CHECK(finder_win_list_compare(&KEYS[i], &KEYS[i], cases[c].bits) == 0,
                  "V2 comparator is reflexive");
            for (int j = 0; j < 7; j++) {
                int a = finder_win_list_compare(&KEYS[i], &KEYS[j], cases[c].bits);
                int b = finder_win_list_compare(&KEYS[j], &KEYS[i], cases[c].bits);
                CHECK((a < 0) == (b > 0) && (a > 0) == (b < 0),
                      "V2 comparator is antisymmetric");
            }
        }
    }
    CHECK(finder_win_list_rank(NULL, 0, 0, 1) == -1 &&
          finder_win_list_rank(KEYS, 7, -1, 1) == -1 &&
          finder_win_list_rank(KEYS, 7, 7, 1) == -1,
          "V2 invalid rank input is refused");
}

static void columns(void)
{
    /* Synthetic widths 91,37,53,67. Header [30,230)x[20,33).
     * Scroll 11: logical edges 19,110,147,200,267, clipped to 30..230.
     * A one-pixel gap before Size must fail at x=110 (Rule 6). */
    const rgn_rect_t header = {20,30,33,230};
    const int16_t widths[4] = {91,37,53,67};
    static const int16_t edges[4][2] = {{30,110},{110,147},{147,200},{200,230}};
    for (int col = 0; col < 4; col++) {
        rgn_rect_t r;
        CHECK(finder_win_list_column(header, widths, 11, col, &r) == 1 &&
              r.left == edges[col][0] && r.right == edges[col][1] &&
              r.top == 20 && r.bottom == 33,
              "V3 column boundaries match independent half-open arithmetic");
    }
    CHECK(finder_win_list_header(0xa501, header, widths, 11, 110, 20) == 0xa505,
          "V4 Size header click changes sort and preserves unrelated bits");
    CHECK(finder_win_list_header(0xa505, header, widths, 11, 109, 32) == 0xa501 &&
          finder_win_list_header(0xa501, header, widths, 11, 147, 32) == 0xa509 &&
          finder_win_list_header(0xa501, header, widths, 11, 229, 32) == 0xa50d,
          "V4 each column owns its left edge and excludes its right edge");
    CHECK(finder_win_list_header(1, header, widths, 11, 230, 20) == 1 &&
          finder_win_list_header(1, header, widths, 11, 110, 33) == 1 &&
          finder_win_list_header(1, header, widths, 11, 29, 20) == 1 &&
          finder_win_list_header(0, header, widths, 11, 110, 20) == 0 &&
          finder_win_list_header(2, header, widths, 11, 110, 20) == 2,
          "V4 outside and non-list header clicks are inert");
    {
        rgn_rect_t r = {1,2,3,4};
        const int16_t bad[4] = {91,0,53,67};
        CHECK(!finder_win_list_column(header, bad, 0, 0, &r) &&
              !finder_win_list_column(header, widths, -1, 0, &r) &&
              !finder_win_list_column(header, widths, 0, 4, &r) &&
              !finder_win_list_column(header, widths, 200, 0, &r) &&
              r.top == 1 && r.left == 2 && r.bottom == 3 && r.right == 4,
              "V3 invalid or offscreen columns leave output untouched");
    }
}

/* Same arena-backed WindowMgr pattern as test_finder_windows.c; no second
 * persistence mechanism. Restart dispatch notes the existing kind=4 set;
 * the existing codec writes it, and a fresh shell reloads and opens it.
 * This is a HOST persistence leg, not an emulator Restart acceptance claim. */
typedef struct scene {
    WindowMgr wm;
    finder_win_rgn_t desk, a, b, c;
    finder_shell_t sh;
    FinderCtx ctx;
} scene_t;

static void attach(finder_win_rgn_t *s)
{
    s->r.rows = s->rows; s->r.cap_rows = FINDER_WIN_RGN_ROWS;
    s->r.x_pool = s->pool; s->r.x_pool_cap = FINDER_WIN_RGN_POOL;
    region_set_empty(&s->r);
}

static int empty_enum(void *u, uint16_t dir, finder_enum_cb cb, void *cu)
{ (void)u; (void)dir; (void)cb; (void)cu; return 0; }

static void init_scene(scene_t *s)
{
    const rgn_rect_t screen = {0,0,480,640};
    finder_fs_t fs;
    memset(s, 0, sizeof *s); memset(&fs, 0, sizeof fs);
    attach(&s->desk); attach(&s->a); attach(&s->b); attach(&s->c);
    WindowMgr_init(&s->wm, screen, &s->desk.r, &s->a.r, &s->b.r, &s->c.r);
    s->sh.wm = &s->wm;
    finder_desk_init(&s->sh.desk, s->sh.desk_icons, FINDER_DESK_MAX_ICONS, screen);
    fs.enumerate = empty_enum;
    finder_shell_bind_fs(&s->sh, &fs);
    finder_shell_bind_ctx(&s->sh, &s->ctx);
}

static void persistence(void)
{
    static scene_t before, after;
    uint8_t db[FINDER_DB_MAX_BYTES];
    /* Two independent 24-byte kind=4 records. No desktop icons in this scene.
     * APPS cluster7 at20,60: Size list (5); OTHER cluster8 at40,80: small (2). */
    static const uint8_t golden[56] = {
        'I','D','B','1',1,0,2,0,
        4,0,7,0,'A','P','P','S',0,0,0,0,0,0,0,0,0,20,0,60,0,5,0,0,
        4,0,8,0,'O','T','H','E','R',0,0,0,0,0,0,0,0,40,0,80,0,2,0,0
    };
    int apps = -1, other = -1;
    uint32_t len;
    init_scene(&before);
    CHECK(finder_win_open(&before.sh, 0, 7, "APPS", 0, &apps, NULL) == FINDER_WIN_OK &&
          finder_win_open(&before.sh, 0, 8, "OTHER", 0, &other, NULL) == FINDER_WIN_OK,
          "V5 two independent folder windows open");
    if (apps < 0 || other < 0) return;
    before.sh.windows[apps].view_bits = finder_win_view_choose(0, 4);
    before.sh.windows[other].view_bits = finder_win_view_choose(0, 2);
    finder_dispatch(&before.ctx, MenuResult(515, 6), "mouse");
    CHECK(before.sh.n_views == 2 && before.sh.windows[apps].open &&
          before.sh.windows[other].open,
          "V5 Restart dispatch saves both views without closing them");
    len = finder_desk_db_encode_all(&before.sh.desk, before.sh.views,
                                    before.sh.n_views, db, sizeof db);
    CHECK(len == sizeof golden && memcmp(db, golden, sizeof golden) == 0,
          "V5 Restart kind=4 bytes match independent records");
    init_scene(&after);
    CHECK(finder_shell_load_views(&after.sh, golden, sizeof golden) == 2,
          "V5 fresh shell loads independent kind=4 records");
    CHECK(finder_win_open(&after.sh, 0, 8, "OTHER", 0, &other, NULL) == FINDER_WIN_OK &&
          finder_win_open(&after.sh, 0, 7, "APPS", 0, &apps, NULL) == FINDER_WIN_OK,
          "V5 fresh shell reopens in opposite slot order");
    if (apps < 0 || other < 0) return;
    CHECK(after.sh.windows[apps].view_bits == 5 && after.sh.windows[other].view_bits == 2,
          "V5 per-folder view bits survive reload independent of window slot");
}

int main(void)
{
    modes(); sorts(); columns(); persistence();
    return TEST_SUMMARY("test-finder-list-view");
}
