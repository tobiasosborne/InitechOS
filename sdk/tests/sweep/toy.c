/* A toy tenant for sdk/tests/sweep.c: File > Beep (Command-B) beeps and
 * File > Quit (Command-Q) exits. SWEEP_TOY_DEAD makes Beep do nothing, the
 * dead command the GOLDEN-MASTER item 4 sweep must find. Test code only. */
#include "tbx.h"
const char tbx_app_name[]="SWEEPTOY";
static const tbx_menuitem_t file_items[]={{"Beep",0,'B',0,1,0,{0,0,0}},{"Quit",0,'Q',0,1,0,{0,0,0}}};
static const tbx_menuinfo_t menus[]={{1,0,"File",file_items,2,0}};
static const tbx_menubar_t bar={menus,1,0,0};
int tbx_app_main(void)
{
    if(tbx_register()!=0||tbx_set_mbar(&bar)!=0)return 1;
    return tbx_new_window(20,40,300,200,1)==1?0:1;
}
void tbx_app_event(const tbx_event_t *e)
{
    if(e->what!=TBX_EVT_MENU||(uint32_t)e->message>>16!=1u)return;
    switch(e->message&0xffff) {
    case 1: if(!SWEEP_TOY_DEAD)(void)tbx_beep(); break;
    case 2: (void)tbx_exit(0); break;
    }
}
