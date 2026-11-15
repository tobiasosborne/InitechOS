#include <string.h>
/* Integration through the shared Toolbox transport; golden disk record from
 * legacy test_cpanels, pattern choice 3 changes bytes 5 and checksum 14. */
#include "tbx_host.h"
#include "chrome_metrics.h"
#include "../cpanels.c"
#include "test_assert.h"
TEST_HARNESS();
int main(void) {
 tbx_host_reset();
 CHECK(tbx_app_main()==0,"launch");
 CHECK(tbx_host.windows[0].t==FLAIR_CHROME_MENUBAR_H,"initial frame meets menu bar");
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
 /* initech-tgt0: the Date & Time panel and a clock that keeps running. The
  * shown time is the clock's until the user changes it; OK sets the clock
  * only when the date or time was changed (OK applies changes: "stage
  * changes, OK to persist/apply", README); an edit is never overwritten by
  * a later reading. Expected times are the host clock's, set here. */
 {
  tbx_event_t k={.what=keyDown,.message=13}, u={.what=updateEvt};
  /* untouched: open at 12:00:00, the clock runs to 12:01:30 */
  tbx_host_reset(); CHECK(tbx_app_main()==0,"launch");
  tbx_host.dialog_result=3; tbx_app_event(&k);         /* Date & Time */
  tbx_host.dialog_result=0;
  CHECK(!strcmp(state.text[0],"Time: 12:00"),"Date & Time opens on the clock");
  tbx_host.clock.minute=1; tbx_host.clock.second=30;
  tbx_app_event(&u);
  CHECK(!strcmp(state.text[0],"Time: 12:01"),"an untouched time shows the running clock");
  tbx_host.dialog_result=15; tbx_app_event(&k);        /* Menu Bar Clock tab */
  tbx_host.dialog_result=7; tbx_app_event(&k);         /* clock off */
  tbx_host.clock.minute=2; tbx_host.clock.second=5;
  tbx_host.dialog_result=1; tbx_app_event(&k);         /* OK */
  CHECK(state.panel==0 && tbx_host.settings.clock==0,"OK applies the Menu Bar Clock change");
  CHECK(tbx_host.clock.hour==12 && tbx_host.clock.minute==2 && tbx_host.clock.second==5,
        "OK over an untouched time leaves the running clock alone");
  /* Cancel over an untouched time */
  tbx_host.dialog_result=3; tbx_app_event(&k);
  tbx_host.clock.minute=3;
  tbx_host.dialog_result=2; tbx_app_event(&k);
  CHECK(tbx_host.clock.minute==3,"Cancel leaves the running clock alone");
  /* edited: Hour + at 12:03 shows 13:03; the clock runs on; the edit stays */
  tbx_host.dialog_result=3; tbx_app_event(&k);
  tbx_host.dialog_result=4; tbx_app_event(&k);         /* Hour + */
  CHECK(!strcmp(state.text[0],"Time: 13:03"),"Hour + edits the time shown");
  tbx_host.clock.minute=9;
  tbx_app_event(&u);
  CHECK(!strcmp(state.text[0],"Time: 13:03"),"a reading never overwrites an edit");
  tbx_host.dialog_result=1; tbx_app_event(&k);         /* OK */
  CHECK(tbx_host.clock.hour==13 && tbx_host.clock.minute==3,"OK sets the edited time");
  tbx_host.dialog_result=0;
 }
 return TEST_SUMMARY("controlpanels route");
}
