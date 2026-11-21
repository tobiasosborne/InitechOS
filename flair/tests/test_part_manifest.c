/* test_part_manifest.c -- every PART against a literal colour from its
 * reference, at every depth and for every registry skin.
 *
 * test_flair_part_map diffs a transcript against fixtures/flair_part_map.golden,
 * which was captured from the product's own former implementation. It detects
 * change, but a wrong entry captured into (or regenerated into) that fixture
 * passes it. This manifest is typed from the references below and never from
 * a program's output (CLAUDE.md, "The constitution", point 4).
 *
 * rgb  : the 0x00RRGGBB written at 15, 16, 24 and 32 bpp (sampled values).
 * idx8 : the palette index written at 8 bpp, by the color_canon.json
 *        allocation (named rows 0..30; a gray ramp row's index is its level).
 *
 * Sources, all outside flair/ (system7-decomp unless marked):
 *   wctb    goldens/resources/wctb_0_System_753.bin parts 0/1/2 (System 7).
 *   pin     specs/chrome/pinstripe.md rendered rows #F3F3F3 y=166, #969696 y=168.
 *   s7tb    specs/chrome/title-bar.md sec 2/2.1: inactive frame #777777,
 *           inactive title text #A5A5A5 (s7_get_info.png).
 *   pal     specs/sys8/platinum-palette.md sec 2 (gray ramp roles) and sec 4
 *           (alert bevel #FFB3B3/#FF8787, s8_alert_modal.png).
 *   menus   specs/sys8/menus.md sec 1.1 (bar W/E7/B3/black), sec 2.1 (panel
 *           face E7, highlight W, shadow B3, drop shadow #3F3F3F), sec 2.2
 *           (disabled ink #A5A5A5).
 *   list    goldens/captures/s8_finder_list_view.png header: sorted cell x=13
 *           #272727 and bottom row y=88 #666666; unsorted bottom row y=88
 *           #545454 (re/mint-results-008.md sec 6).
 *   mint8   re/mint-results-008.md sec 3: the Finder icon colour set, in the
 *           order the DESK_IC_* ordinals name them.
 *   w31     win31-decomp default-colors-cross-check.txt: ActiveTitle #000080,
 *           TitleText #FFFFFF, ButtonFace #C0C0C0, ButtonShadow #808080.
 *   teal    operator canon: Initech teal #8DDCDC replaces System 7 lavender
 *           (color_canon.json idx2). Its derived rows are authored with the
 *           operator: bevel #8DDCDC/#4E9BA3 (OQ-2), menu title #27666C/#003F45
 *           with 8 bpp fallback 2/63/0 (tests/goldens/menu_look_golden.h),
 *           scroll thumb white/teal/#4E9BA3, 8 bpp 1/2/63/63
 *           (tests/goldens/scrollbar_look_golden.h).
 *   role    authored role over an existing canon row with no period capture
 *           of that role (named in the row).
 *
 * Not asserted, because no reference fixes them: ICON_HILITE_FACE and
 * ICON_HILITE_SHADE. They cite Inside Macintosh VI 2-19 ("lowers the
 * brightness"), but halving white and #C0C0C0 gives #7F7F7F and #606060,
 * not their #777777 and #3F3F3F, and no capture shows the state. No product
 * source draws them. The transcript fixture still pins them.
 */
#include <stdint.h>
#include <stdio.h>

#include "flair_look.h"
#include "flair_skins.h"
#include "test_assert.h"

TEST_HARNESS();

typedef struct { int part; uint32_t rgb; unsigned idx8; const char *src; } want_part;

#define P(name, rgb, idx, src) { (int)FLAIR_PART_##name, rgb, idx, src }
#define NA 0xFFFFFFFFu   /* row recorded as a gap; value not asserted */

static const want_part WANT[] = {
    P(CONTENT,        0xFFFFFF,   1, "wctb part0"),
    P(FRAME,          0x000000,   0, "wctb part1"),
    P(TEXT,           0x000000,   4, "wctb part2"),
    P(DESKTOP,        0x8DDCDC,   2, "teal"),
    P(MENUBAR,        0xFFFFFF,   3, "wctb part0 (System 7 white bar)"),
    P(CAPTION_NAVY,   0x000080,   5, "w31 ActiveTitle"),
    P(BTNFACE,        0xC0C0C0,   6, "w31 ButtonFace"),
    P(PIN_LIGHT,      0xF3F3F3,   7, "pin y=166"),
    P(PIN_DARK,       0x969696,   8, "pin y=168"),
    P(BEVEL_LIGHT,    0x8DDCDC,   2, "teal bevel_light"),
    P(BEVEL_SHADOW,   0x4E9BA3,   4, "teal bevel_shadow; 8 bpp keeps idx4"),
    P(HILITE_FRAME,   0x777777, 119, "s7tb inactive frame"),
    P(HILITE_TEXT,    0xA5A5A5, 165, "s7tb inactive title text"),
    P(PLAT_STRIPE_DARK,    0x969696, 150, "pal title dark stripe"),
    P(PLAT_FRAME_FACE,     0xDADADA, 218, "pal active frame-bar face"),
    P(PLAT_FACE,           0xE7E7E7, 231, "pal dialog/inactive face"),
    P(PLAT_FRAME_SHADOW,   0xB3B3B3, 179, "pal active frame-bar shadow"),
    P(PLAT_WIDGET_EDGE,    0xA5A5A5, 165, "pal widget edge"),
    P(PLAT_DARK_RING,      0x3F3F3F,  63, "pal widget dark ring"),
    P(PLAT_INACTIVE_FRAME, 0x777777, 119, "pal inactive frame lines"),
    P(PLAT_INACTIVE_TEXT,  0x878787, 135, "pal inactive title ink"),
    P(PLAT_TROUGH,         0xF3F3F3, 243, "pal disabled/hollow trough"),
    P(PLAT_WELL,           0xC0C0C0, 192, "pal enabled page well"),
    P(PLAT_TILE_SHADOW,    0xCDCDCD, 205, "pal arrow-tile shadow"),
    P(MENU_BAR_HL,         0xFFFFFF,   1, "menus 1.1 y=0"),
    P(MENU_BAR_FACE,       0xE7E7E7, 231, "menus 1.1 y=1..17"),
    P(MENU_BAR_SHADOW,     0xB3B3B3, 179, "menus 1.1 y=18"),
    P(MENU_PANEL_FACE,     0xE7E7E7, 231, "menus 2.1 face"),
    P(MENU_PANEL_HL,       0xFFFFFF,   1, "menus 2.1 inner highlight"),
    P(MENU_PANEL_SHADOW,   0xB3B3B3, 179, "menus 2.1 inner shadow"),
    P(MENU_DROP_SHADOW,    0x3F3F3F,  63, "menus 2.1 drop shadow, not black"),
    P(MENU_DISABLED_INK,   0xA5A5A5, 165, "menus 2.2 disabled ink"),
    P(MENU_TITLE_HILITE_HL,     0x8DDCDC,  2, "teal for #8787DA"),
    P(MENU_TITLE_HILITE_FACE,   0x27666C, 63, "teal for #5454B3"),
    P(MENU_TITLE_HILITE_SHADOW, 0x003F45,  0, "teal for #0000A5"),
    P(SB_THUMB_HL,         0xFFFFFF,   1, "teal for #DADAFF"),
    P(SB_THUMB_FACE,       0x8DDCDC,   2, "teal for #B3B3FF"),
    P(SB_THUMB_SHADOW,     0x4E9BA3,  63, "teal for #8787DA"),
    P(SB_THUMB_GRIP,       0x4E9BA3,  63, "teal for #5454B3"),
    P(ICON_INK,            0x000000,   0, "role: icon outline, wctb black"),
    P(ICON_FACE,           0xFFFFFF,   1, "role: icon body, wctb white"),
    P(ICON_SHADE,          0xC0C0C0,   6, "role: icon detail, w31 ButtonFace"),
    P(ICON_HILITE_FACE,    NA, 0, "gap: no reference"),
    P(ICON_HILITE_SHADE,   NA, 0, "gap: no reference"),
    P(ALERT_LIGHT,         0xFFB3B3,   9, "pal sec 4 alert top/left"),
    P(ALERT_SHADOW,        0xFF8787,  10, "pal sec 4 alert bottom/right"),
    P(LIST_PRESSED_EDGE,   0x272727,  39, "list sorted cell x=13"),
    P(LIST_EDGE,           0x545454,  84, "list unsorted y=88"),
    P(LIST_PRESSED_BOTTOM, 0x666666, 102, "list sorted y=88"),
    /* FLAIR_PART_ICON_CLUT + k, DESK_IC_* order (mint8 sec 3). */
    { (int)FLAIR_PART_ICON_CLUT +  0, 0xFFFFFF,   1, "mint8 white" },
    { (int)FLAIR_PART_ICON_CLUT +  1, 0xF3F3F3, 243, "mint8 #F3F3F3" },
    { (int)FLAIR_PART_ICON_CLUT +  2, 0xE7E7E7, 231, "mint8 #E7E7E7" },
    { (int)FLAIR_PART_ICON_CLUT +  3, 0xDADADA, 218, "mint8 #DADADA" },
    { (int)FLAIR_PART_ICON_CLUT +  4, 0xCDCDCD, 205, "mint8 #CDCDCD" },
    { (int)FLAIR_PART_ICON_CLUT +  5, 0xC0C0C0, 192, "mint8 #C0C0C0" },
    { (int)FLAIR_PART_ICON_CLUT +  6, 0xA5A5A5, 165, "mint8 #A5A5A5" },
    { (int)FLAIR_PART_ICON_CLUT +  7, 0x969696, 150, "mint8 #969696" },
    { (int)FLAIR_PART_ICON_CLUT +  8, 0x878787, 135, "mint8 #878787" },
    { (int)FLAIR_PART_ICON_CLUT +  9, 0x777777, 119, "mint8 #777777" },
    { (int)FLAIR_PART_ICON_CLUT + 10, 0x666666, 102, "mint8 #666666; pressed rim" },
    { (int)FLAIR_PART_ICON_CLUT + 11, 0x545454,  84, "mint8 #545454" },
    { (int)FLAIR_PART_ICON_CLUT + 12, 0x3F3F3F,  63, "mint8 #3F3F3F" },
    { (int)FLAIR_PART_ICON_CLUT + 13, 0x272727,  39, "mint8 #272727" },
    { (int)FLAIR_PART_ICON_CLUT + 14, 0x000000,   0, "mint8 black" },
    { (int)FLAIR_PART_ICON_CLUT + 15, 0xDADAFF,  11, "mint8 folder face" },
    { (int)FLAIR_PART_ICON_CLUT + 16, 0xB3B3FF,  12, "mint8 folder bevel" },
    { (int)FLAIR_PART_ICON_CLUT + 17, 0x8787DA,  13, "mint8 folder edge" },
    { (int)FLAIR_PART_ICON_CLUT + 18, 0x545487,  14, "mint8 selected folder face" },
    { (int)FLAIR_PART_ICON_CLUT + 19, 0x000087,  15, "mint8 selected folder edge" },
    { (int)FLAIR_PART_ICON_CLUT + 20, 0xFFFF00,  16, "mint8 pencil/caution yellow" },
    { (int)FLAIR_PART_ICON_CLUT + 21, 0xDAB387,  17, "mint8 tan" },
    { (int)FLAIR_PART_ICON_CLUT + 22, 0xFFDAB3,  18, "mint8 flesh" },
    { (int)FLAIR_PART_ICON_CLUT + 23, 0xE70000,  19, "mint8 red" },
    { (int)FLAIR_PART_ICON_CLUT + 24, 0xFF87DA,  20, "mint8 pink" },
    { (int)FLAIR_PART_ICON_CLUT + 25, 0x878700,  21, "mint8 selected yellow" },
    { (int)FLAIR_PART_ICON_CLUT + 26, 0x875400,  22, "mint8 selected tan" },
    { (int)FLAIR_PART_ICON_CLUT + 27, 0x875454,  23, "mint8 selected flesh" },
    { (int)FLAIR_PART_ICON_CLUT + 28, 0x870000,  24, "mint8 selected red" },
    { (int)FLAIR_PART_ICON_CLUT + 29, 0x870054,  25, "mint8 selected pink" },
    P(FILECOPY_PROGRESS, 0x4559FF, 27, "goldens/frame/colours.tsv progress_fill"),
    P(SLICE, 0x4282FF, 28, "goldens/frame/colours.tsv wedge40_top"),
    P(SLICE_SIDE, 0x334A87, 29, "goldens/frame/colours.tsv wedge_side_bottom"),
};
#define N_WANT ((int)(sizeof WANT / sizeof WANT[0]))

/* The seven PARTs whose colour comes from the selected skin row, per skin
 * (System 7, Win 3.1, Platinum). Values are the cited slot literals of
 * test_skin_manifest; a PART reading the wrong slot fails here. */
typedef struct { int part; uint32_t rgb[3]; unsigned idx8[3]; } want_skin_part;
static const want_skin_part WANT_SKIN[] = {
    { (int)FLAIR_PART_CONTENT,      {0xFFFFFF,0xFFFFFF,0xFFFFFF}, {  1,  1,  1} },
    { (int)FLAIR_PART_FRAME,        {0x000000,0x000000,0x000000}, {  0,  0,  0} },
    { (int)FLAIR_PART_TEXT,         {0x000000,0xFFFFFF,0x000000}, {  4,  1,  4} },
    { (int)FLAIR_PART_DESKTOP,      {0x8DDCDC,0x8DDCDC,0x8DDCDC}, {  2,  2,  2} },
    { (int)FLAIR_PART_CAPTION_NAVY, {0x000080,0x000080,0x000080}, {  5,  5,  5} },
    { (int)FLAIR_PART_PLAT_FACE,    {0xC0C0C0,0xC0C0C0,0xE7E7E7}, {  6,  6,231} },
    { (int)FLAIR_PART_PLAT_WIDGET_EDGE,{0x969696,0x808080,0xA5A5A5}, {  8,128,165} },
};
#define N_SKIN ((int)(sizeof WANT_SKIN / sizeof WANT_SKIN[0]))

static const uint32_t DEPTHS[5] = { 8u, 15u, 16u, 24u, 32u };

static void check_value(const char *what, int part, uint32_t bpp, uint32_t got,
                        uint32_t rgb, unsigned idx8, const char *src)
{
    char msg[200];
    uint32_t want = bpp == 8u ? idx8 : rgb;
    snprintf(msg, sizeof msg, "%s part %d at %u bpp = %06X (%s); got %06X",
             what, part, (unsigned)bpp, (unsigned)want, src, (unsigned)got);
    CHECK(got == want, msg);
}

int main(void)
{
    static const int keys[3][2] = { { ERA_SYS7_0_1, HERITAGE_QUICKDRAW },
                                    { ERA_WIN31, HERITAGE_GDI },
                                    { ERA_SYS8_PLATINUM, HERITAGE_QUICKDRAW } };
    int seen[FLAIR_PART__COUNT] = { 0 };
    GrafPort port = { 0 };

    CHECK(FLAIR_PART__COUNT == 82, "82 PARTs, all listed in this manifest");
    CHECK(N_WANT == (int)FLAIR_PART__COUNT, "manifest has one row per PART");
    for (int i = 0; i < N_WANT; i++) {
        const want_part *w = &WANT[i];
        if (w->part < 0 || w->part >= (int)FLAIR_PART__COUNT || seen[w->part]++) {
            CHECK(0, "manifest PART is in range and listed once");
            continue;
        }
        if (w->rgb == NA) continue;
        int slotted = 0;
        for (int k = 0; k < N_SKIN; k++) slotted |= WANT_SKIN[k].part == w->part;
        for (unsigned b = 0; b < 5u; b++) {
            check_value("depth", w->part, DEPTHS[b],
                        flair_look_pixel_depth(DEPTHS[b], w->part), w->rgb, w->idx8, w->src);
            port.portBits.bm.bpp = DEPTHS[b];
            check_value("port", w->part, DEPTHS[b],
                        flair_look_pixel(&port, w->part), w->rgb, w->idx8, w->src);
            if (slotted) continue;
            for (unsigned s = 0; s < 3u; s++)
                check_value("skin", w->part, DEPTHS[b],
                            flair_look_pixel_for_skin(&port,
                                flair_skin_resolve(keys[s][0], keys[s][1]), w->part),
                            w->rgb, w->idx8, w->src);
        }
    }
    for (int k = 0; k < N_SKIN; k++)
        for (unsigned b = 0; b < 5u; b++) {
            port.portBits.bm.bpp = DEPTHS[b];
            for (unsigned s = 0; s < 3u; s++)
                check_value("skin-slot", WANT_SKIN[k].part, DEPTHS[b],
                            flair_look_pixel_for_skin(&port,
                                flair_skin_resolve(keys[s][0], keys[s][1]),
                                WANT_SKIN[k].part),
                            WANT_SKIN[k].rgb[s], WANT_SKIN[k].idx8[s], "skin slot");
        }
    /* Out-of-range PARTs fail soft to frame black at every depth
     * (flair_look.h, explicit error handling). */
    for (unsigned b = 0; b < 5u; b++) {
        check_value("range", -1, DEPTHS[b], flair_look_pixel_depth(DEPTHS[b], -1),
                    0, 0, "fail-soft black");
        check_value("range", (int)FLAIR_PART__COUNT, DEPTHS[b],
                    flair_look_pixel_depth(DEPTHS[b], (int)FLAIR_PART__COUNT),
                    0, 0, "fail-soft black");
    }
    return TEST_SUMMARY("test-part-manifest");
}
