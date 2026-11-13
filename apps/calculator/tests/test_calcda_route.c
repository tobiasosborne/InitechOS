/* initech-0xs6: real Calculator tenant over a recording Toolbox mock.
 * Mac guide printed p.125: Close ends an accessory; IM VI PDF p.345:
 * shutdown asks the application's own quit routine. Our existing gate asks
 * an enabled literal Quit item. Close W must remain available as well.
 * No clipboard syntax is invented here (B13 still needs a real-Mac mint). */
#include <stdio.h>
#include <string.h>
#include "../calcda.c"
#include "tbx_host.h"

static int fails, checks;
#define installed tbx_host.bar
#define exits tbx_host.exit_calls
#define CHECK(c, why) do { checks++; if (!(c)) { puts("FAIL " why); fails++; } } while (0)

static void menu(int32_t word)
{
    tbx_event_t e={0};e.what=TBX_EVT_MENU;e.message=word;tbx_app_event(&e);
}
int main(void)
{
    tbx_host_reset();
    int32_t quit=0;
    CHECK(tbx_app_main()==0 && installed,"launch");
    CHECK(!strcmp(calc_display(&g_calc),"0"),"initial readout");
    CHECK(installed && installed->menus[0].items[0].enabled &&
          installed->menus[0].items[0].cmd_char=='W' &&
          !strcmp(installed->menus[0].items[0].text,"Close"),"Close W retained");
    /* The supported shutdown contract, as documented by tbxgate.h:
     * explicitly registered Quit; never infer consent from a close box. */
    if(installed) for(unsigned m=0;m<installed->n_menus;m++) {
        const tbx_menuinfo_t *mi=&installed->menus[m];
        for(unsigned k=0;k<mi->n_items;k++) {
            const tbx_menuitem_t *it=&mi->items[k];
            if(it->enabled && !it->is_divider && !strcmp(it->text,"Quit"))
                quit=TBX_MENU_WORD(mi->menu_id,k+1);
        }
    }
    CHECK(quit==0x00810002,"shutdown has an explicit enabled Quit route");
    menu(quit);
    CHECK(exits==1,"shutdown quit closes the documentless Calculator");
    exits=0;menu(0x00810001);
    CHECK(exits==1,"Close W closes Calculator");
    exits=0;menu(0x00810003);
    CHECK(exits==0,"unrelated menu does not close Calculator");
    printf("test_calcda_route: %d checks, %d failures\n",checks,fails);
    return fails?1:0;
}
