#include "tbx.h"
const char tbx_app_name[]="SDKCHECK";
int tbx_app_main(void) { return tbx_register(); }
void tbx_app_event(const tbx_event_t *event) { (void)event; }
