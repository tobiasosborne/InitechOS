/* InitechPaint routing, initech-tdnl.20 (bar carrier). Factory C, host: the
 * REAL Paint tenant with the Toolbox calls recorded.
 * Ref: PRD 6.5 (InitechPaint carries the Photoshop-exact bar) and Appendix A
 * ("File Edit Image Layer Select View Window Help"); spec/toolbox_gate.h
 * Sec 6a/9. The bar is compared with a literal typed here AND with the
 * locked spec/assets/calc_ledger.json paint_menu (calc_ledger.h) -- never
 * with menu_canon.h, which the tenant itself reads. Emulator pixels are
 * test-flair-paint's. */
#include <stdio.h>
#include <string.h>
#include "../../os/apps/paint/paint.c"
#include "calc_ledger.h"

static unsigned checks, failures, windows, exits, fills;
static const tbx_menubar_t *bar;
static char title[64];
#define CHECK(c, s) do { checks++; if (!(c)) { failures++; \
    fprintf(stderr, "FAIL paint routing: %s\n", s); } } while (0)

int32_t tbx_register(void) { return TBX_OK; }
int32_t tbx_new_window(int32_t l, int32_t t, int32_t r, int32_t b, int32_t g)
{ (void)l; (void)t; (void)r; (void)b; CHECK(g == TBX_GOAWAY_CLOSE, "canvas close box ends Paint"); return (int32_t)++windows; }
int32_t tbx_set_wtitle(int32_t w, const char *s)
{ (void)w; snprintf(title, sizeof title, "%s", s); return TBX_OK; }
int32_t tbx_set_mbar(const tbx_menubar_t *b) { bar = b; return TBX_OK; }
int32_t tbx_fill_rect(int32_t w, int32_t l, int32_t t, int32_t r, int32_t b, uint32_t c)
{ (void)w; (void)l; (void)t; (void)r; (void)b; CHECK(c == TBX_COLOR_WHITE, "the canvas is white"); fills++; return TBX_OK; }
int32_t tbx_exit(int32_t rc) { CHECK(rc == 0, "exit status 0"); exits++; return TBX_OK; }

int main(void)
{
    char joined[128] = "";
    CHECK(tbx_app_main() == 0, "Paint launches");
    CHECK(windows == 1 && !strcmp(title, "Untitled-1"), "one blank document window");
    CHECK(bar != NULL && bar->n_menus == 8 && bar->has_apple == 0,
          "BAR_SHAPE eight menus, no Apple slot (the frame's bar has none)");
    if (bar) for (uint16_t m = 0; m < bar->n_menus; m++) {
        if (m) strcat(joined, " ");
        strcat(joined, bar->menus[m].title);
        CHECK(bar->menus[m].n_items >= 1 && bar->menus[m].n_items <= TBX_MBAR_MAX_ITEMS,
              "every menu has 1..16 items");
        for (uint16_t i = 0; i < bar->menus[m].n_items; i++) {
            const tbx_menuitem_t *it = &bar->menus[m].items[i];
            int quit = m == 0 && !strcmp(it->text, "Quit");
            CHECK(strlen(it->text) < TBX_MBAR_STR_MAX, "item text fits the MenuBar limit");
            if (quit) CHECK(it->enabled && it->cmd_char == 'Q', "File > Quit is live, Cmd/Ctrl-Q");
            else CHECK(!it->enabled, "GRAYED every other item is period-grayed");
        }
    }
    CHECK(!strcmp(joined, "File Edit Image Layer Select View Window Help"),
          "BAR_TEXT File Edit Image Layer Select View Window Help (PRD Appendix A)");
    CHECK(!strcmp(joined, CALC_PAINT_MENU), "BAR_TEXT equals locked calc_ledger.json paint_menu");
    {
        tbx_event_t upd = { updateEvt, 1, 0, 0, 0, 0 };
        tbx_app_event(&upd);
        CHECK(fills == 1, "updateEvt paints the canvas");
        tbx_event_t other = { TBX_EVT_MENU, TBX_MENU_WORD(129, 2), 0, 0, 0, 0 };
        tbx_app_event(&other);
        CHECK(exits == 0, "a non-Quit menu word does nothing");
        tbx_event_t quit = { TBX_EVT_MENU, TBX_MENU_WORD(128, 5), 0, 0, 0, 0 };
        tbx_app_event(&quit);
        CHECK(exits == 1, "File > Quit exits");
    }
    printf("test_paint_tenant: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
