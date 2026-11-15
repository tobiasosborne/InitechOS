/* test_skin_manifest.c -- every slot of the three skin registry rows against
 * literal values read from the references, not from the registry.
 *
 * test_skin_era_frozen pins an FNV digest of the two locked rows. Its own
 * re-pin procedure takes the expected value from the program's live output,
 * so a wrong value that is re-pinned passes it. This manifest states each
 * palette index and RGB as a literal with its source, so a re-pinned wrong
 * slot still fails here (CLAUDE.md, "The constitution", point 4).
 *
 * Sources (all outside flair/):
 *   wctb    system7-decomp goldens/resources/wctb_0_System_753.bin parts
 *           0 (content/bar white), 1 (frame), 2 (text), via color_canon.json.
 *   pin     system7-decomp specs/chrome/pinstripe.md rendered rows
 *           #F3F3F3 (y=166) and #969696 (y=168), 1-px bevel rows.
 *   plat    system7-decomp specs/sys8/platinum-palette.md sec 2 and
 *           menus.md sec 1.1 (sampled #E7E7E7 face, #A5A5A5 widget edge).
 *   chrome  system7-decomp specs/sys8/window-chrome.md sec 1 (frame #000000),
 *           sec 2.2 (alternating stripes), sec 2.3 (title ink), sec 4
 *           (4-px raised body frame).
 *   w31     win31-decomp refs/win31-chrome/default-colors-cross-check.txt
 *           (ActiveTitle, TitleText, ButtonFace, ButtonShadow, ButtonHilight,
 *           WindowFrame, Window) and specs/chrome/color-scheme.md sec 3
 *           (the menu BAR interior is #FFFFFF in w31_client.png).
 *   teal    operator canon: Initech teal #8DDCDC replaces System 7 lavender
 *           (color_canon.json idx2, 2026-06-21) and its bevel pair
 *           #8DDCDC/#4E9BA3 (color_canon.json derived_rows, OQ-2).
 *   (pinstripe: 0x55 alternating rows per pin and chrome sec 2.2; the Win
 *   3.1 caption is solid ActiveTitle, so its row carries 0x00.)
 *   role    authored role assignment documented in flair_skins.h: the
 *           System 7 row fills btnface/btnshadow/btnhilight from canon rows
 *           (#C0C0C0, pinstripe shades). The value is period-sourced; no
 *           System 7 capture shows a 3-D button using it.
 * Indices are the color_canon.json allocation: named rows 0..8, and the
 * gray ramp, whose index is the gray level.
 *
 * Not asserted, because no reference fixes it: the Win 3.1 row's
 * bevel_width (2). win31 button-bevel.md says the structure is 1 px per side
 * and leaves the exact width open (GAPS bevel-1). The frozen digest still
 * pins it as a regression.
 */
#include "flair_skins.h"
#include "test_assert.h"

TEST_HARNESS();

typedef struct { const char *name; unsigned idx; uint32_t rgb; const char *src; } want_slot;
typedef struct {
    int era, heritage;
    want_slot s[11];
    unsigned pinstripe;
    int bevel_width;   /* -1: no reference, not asserted */
} want_row;

static const want_row WANT[3] = {
    { ERA_SYS7_0_1, HERITAGE_QUICKDRAW, {
        { "desktop",      2, 0x8DDCDC, "teal" },
        { "caption_navy", 5, 0x000080, "w31 ActiveTitle (canon accent)" },
        { "caption_ink",  4, 0x000000, "wctb part2" },
        { "menubar",      3, 0xFFFFFF, "wctb part0" },
        { "btnface",      6, 0xC0C0C0, "role; w31 ButtonFace" },
        { "btnshadow",    8, 0x969696, "role; pin y=168" },
        { "btnhilight",   7, 0xF3F3F3, "role; pin y=166" },
        { "frame_ink",    0, 0x000000, "wctb part1" },
        { "content",      1, 0xFFFFFF, "wctb part0" },
        { "bevel_light",  2, 0x8DDCDC, "teal bevel_light" },
        { "bevel_shadow", 2, 0x4E9BA3, "teal bevel_shadow" } },
      0x55u, 1 },
    { ERA_WIN31, HERITAGE_GDI, {
        { "desktop",      2, 0x8DDCDC, "teal" },
        { "caption_navy", 5, 0x000080, "w31 ActiveTitle" },
        { "caption_ink",  1, 0xFFFFFF, "w31 TitleText" },
        { "menubar",      1, 0xFFFFFF, "w31 color-scheme.md sec 3" },
        { "btnface",      6, 0xC0C0C0, "w31 ButtonFace" },
        { "btnshadow",  128, 0x808080, "w31 ButtonShadow" },
        { "btnhilight",   1, 0xFFFFFF, "w31 ButtonHilight" },
        { "frame_ink",    0, 0x000000, "w31 WindowFrame" },
        { "content",      1, 0xFFFFFF, "w31 Window" },
        { "bevel_light",  1, 0xFFFFFF, "w31 ButtonHilight" },
        { "bevel_shadow",128, 0x808080, "w31 ButtonShadow" } },
      0x00u, -1 },
    { ERA_SYS8_PLATINUM, HERITAGE_QUICKDRAW, {
        { "desktop",      2, 0x8DDCDC, "teal" },
        { "caption_navy", 5, 0x000080, "w31 ActiveTitle (canon accent)" },
        { "caption_ink",  4, 0x000000, "chrome sec 2.3" },
        { "menubar",    231, 0xE7E7E7, "plat; menus.md sec 1.1" },
        { "btnface",    231, 0xE7E7E7, "plat push-button face" },
        { "btnshadow",  165, 0xA5A5A5, "plat widget edge" },
        { "btnhilight",   1, 0xFFFFFF, "plat 3-D highlight" },
        { "frame_ink",    0, 0x000000, "chrome sec 1" },
        { "content",      1, 0xFFFFFF, "plat Finder content field" },
        { "bevel_light",  2, 0x8DDCDC, "teal bevel_light" },
        { "bevel_shadow", 2, 0x4E9BA3, "teal bevel_shadow" } },
      0x55u, 4 },
};

static const flair_skin_slot_t *slot_of(const flair_skin_t *r, int k)
{
    const flair_skin_slot_t *v[11] = {
        &r->desktop, &r->caption_navy, &r->caption_ink, &r->menubar,
        &r->btnface, &r->btnshadow, &r->btnhilight, &r->frame_ink,
        &r->content, &r->bevel_light, &r->bevel_shadow };
    return v[k];
}

int main(void)
{
    char msg[160];
    CHECK(FLAIR_SKIN_REGISTRY_COUNT == 3u, "registry has the three manifest rows");
    for (int i = 0; i < 3; i++) {
        const want_row *w = &WANT[i];
        const flair_skin_t *r = 0;
        for (unsigned j = 0; j < FLAIR_SKIN_REGISTRY_COUNT; j++)
            if (flair_skin_registry[j].era == w->era &&
                flair_skin_registry[j].heritage == w->heritage)
                r = &flair_skin_registry[j];
        snprintf(msg, sizeof msg, "row era=%d heritage=%d is present", w->era, w->heritage);
        CHECK(r != 0, msg);
        if (!r) continue;
        for (int k = 0; k < 11; k++) {
            const flair_skin_slot_t *s = slot_of(r, k);
            snprintf(msg, sizeof msg, "era %d %s = idx %u #%06X (%s); got idx %u #%06X",
                     w->era, w->s[k].name, w->s[k].idx, (unsigned)w->s[k].rgb,
                     w->s[k].src, (unsigned)s->idx, (unsigned)s->rgb);
            CHECK(s->idx == w->s[k].idx && s->rgb == w->s[k].rgb, msg);
        }
        snprintf(msg, sizeof msg, "era %d pinstripe byte 0x%02X", w->era, w->pinstripe);
        CHECK(r->pinstripe_pattern == w->pinstripe, msg);
        if (w->bevel_width >= 0) {
            snprintf(msg, sizeof msg, "era %d bevel width %d px", w->era, w->bevel_width);
            CHECK(r->bevel_width == w->bevel_width, msg);
        }
    }
    return TEST_SUMMARY("test-skin-manifest");
}
