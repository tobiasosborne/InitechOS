#include "tbx.h"
const char tbx_app_name[]="COPYTEST";
static int32_t w TBX_IN_IMAGE;
static unsigned completions TBX_IN_IMAGE;
static const char src[]="SOURCE.DAT",dst[]="TARGET.DAT",caption[]="Copying test data...";
int tbx_app_main(void)
{
    if(tbx_register()!=TBX_OK) return 1;
    w=tbx_new_window(180,170,470,330,1);
    return w>0?0:1;
}
void tbx_app_event(const tbx_event_t *ev)
{
    if(ev->what==updateEvt) (void)tbx_fill_rect(w,0,0,400,400,TBX_COLOR_WHITE);
    if(ev->what==keyDown && (ev->message&255u)=='s') (void)tbx_file_copy(caption,src,dst);
    if(ev->what==TBX_EVT_FILECOPY && ++completions>1) (void)tbx_exit(2);
    if(ev->what==keyDown && (ev->message&255u)=='q') (void)tbx_exit(completions==1?0:3);
}
