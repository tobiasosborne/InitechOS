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
#include "../paint.c"
#include "tbx_host.h"
#include "chrome_metrics.h"
#include "calc_ledger.h"

static unsigned checks, failures;
#define m_windows tbx_host.nwindows
#define exits tbx_host.exit_calls
#define bar tbx_host.bar
#define m_title tbx_host.windows[0].title
#define CHECK(c, s) do { checks++; if (!(c)) { failures++; \
    fprintf(stderr, "FAIL paint routing: %s\n", s); } } while (0)

int main(void)
{
    tbx_host_reset();
    char joined[128] = "";
    CHECK(tbx_app_main() == 0, "Paint launches");
    CHECK(tbx_host.windows[0].t==FLAIR_CHROME_MENUBAR_H,"initial frame meets menu bar");
    CHECK(m_windows == 1 && !strcmp(m_title, "Untitled-1"), "one blank document window");
    CHECK(bar != NULL && bar->n_menus == 8 && bar->has_apple == 0,
          "BAR_SHAPE eight menus, no Apple slot (the frame's bar has none)");
    if (bar) for (uint16_t m = 0; m < bar->n_menus; m++) {
        if (m) strcat(joined, " ");
        strcat(joined, bar->menus[m].title);
        CHECK(bar->menus[m].menu_id==128+(int)m,"sweep menu dispatch identity");
        CHECK(bar->menus[m].n_items == (m==0?2:0),
              "sweep File has Close/Quit; exactly seven empty titles");
        for (uint16_t i = 0; i < bar->menus[m].n_items; i++) {
            const tbx_menuitem_t *it = &bar->menus[m].items[i];
            CHECK(strlen(it->text) < TBX_MBAR_STR_MAX, "item text fits the MenuBar limit");
            CHECK(m==0 && i<2 && !strcmp(it->text,i?"Quit":"Close") &&
                  it->enabled && !it->is_divider && it->cmd_char==(i?'Q':'W'),
                  "sweep exact ordered enabled menu/shortcut set");
        }
    }
    CHECK(!strcmp(joined, "File Edit Image Layer Select View Window Help"),
          "BAR_TEXT File Edit Image Layer Select View Window Help (PRD Appendix A)");
    CHECK(!strcmp(joined, CALC_PAINT_MENU), "BAR_TEXT equals locked calc_ledger.json paint_menu");
    {
        tbx_event_t upd = { updateEvt, 1, 0, 0, 0, 0 };
        tbx_app_event(&upd);
        unsigned fills=0;
        CHECK(tbx_host.windows[0].goaway==TBX_GOAWAY_CLOSE,"canvas close ends Paint");
        for(unsigned i=0;i<tbx_host.nops;i++)if(!strcmp(tbx_host.ops[i].name,"tbx_fill_rect")) {
            fills++;CHECK(tbx_host.ops[i].a[5]==0,"canvas is white");
        }
        CHECK(fills == 1, "updateEvt paints the canvas");
        tbx_event_t other = { TBX_EVT_MENU, TBX_MENU_WORD(129, 2), 0, 0, 0, 0 };
        tbx_app_event(&other);
        CHECK(exits == 0, "a non-Quit menu word does nothing");
        tbx_event_t quit = { TBX_EVT_MENU, TBX_MENU_WORD(128, 2), 0, 0, 0, 0 };
        tbx_app_event(&quit);
        CHECK(exits == 1 && tbx_host.exit_rc==0, "File > Quit exits");
        exits=0;quit.message=TBX_MENU_WORD(128,1);tbx_app_event(&quit);
        CHECK(exits==1 && tbx_host.exit_rc==0,"File Close exits same as close box");
        unsigned before=tbx_host.nops;
        for(int m=129;m<=135;m++) {
            quit.message=TBX_MENU_WORD(m,1);tbx_app_event(&quit);
        }
        CHECK(tbx_host.nops==before,"empty titles dispatch no app command");
        CHECK(!tbx_host.sf && !tbx_host.alerts && !tbx_host.find_calls &&
              !tbx_host.ndeleted,"documentless Paint never touches files or prompts");
    }
    printf("test_paint_tenant: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
