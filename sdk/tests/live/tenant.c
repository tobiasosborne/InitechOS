#include "tbx.h"
#include "cases.h"
const char tbx_app_name[]="SDKCHECK";
int tbx_app_main(void)
{
    cases();
    int h=tbx_file_create("SDKRES.BIN");
    if(h>=5) { (void)tbx_file_write(h,results,nr*4);(void)tbx_file_close(h); }
    if(!postexit())__builtin_trap();
    return 0;
}
void tbx_app_event(const tbx_event_t *e) { (void)e; }
