#include <string.h>
/* Integration through the shared Toolbox transport; golden disk record from
 * legacy test_cpanels, pattern choice 3 changes bytes 5 and checksum 14. */
#include "tbx_host.h"
#include "../cpanels.c"
#include "test_assert.h"
TEST_HARNESS();
int main(void) {
 tbx_host_reset();
 CHECK(tbx_app_main()==0,"launch");
 tbx_event_t e={.what=keyDown,.message=13};
 tbx_host.dialog_result=4; tbx_app_event(&e); /* Desktop Patterns */
 tbx_host.dialog_result=6; tbx_app_event(&e); /* Stipple */
 tbx_host.dialog_result=1; tbx_app_event(&e); /* OK saves and applies */
 const tbx_host_file_t *f=tbx_host_file("CPANELS.DAT");
 const unsigned char expected[16]={'C','P','0','1',1,3,1,1,0,2,1,0,0,0,0xfd,0};
 CHECK(f && f->size==16 && !memcmp(f->data,expected,16),"saved record");
 CHECK(tbx_host.settings.pattern==3,"apply");
 tbx_host.dialog_result=0;
 e.what=TBX_EVT_MENU;e.message=TBX_MENU_WORD(129,1);tbx_app_event(&e);
 CHECK(tbx_host.exit_calls==1,"quit");
 return TEST_SUMMARY("controlpanels route");
}
