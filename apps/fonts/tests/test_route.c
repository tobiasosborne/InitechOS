/* Independent literal specimen expectations from legacy-final font journey. */
#include "tbx_host.h"
#include "test_assert.h"
#include "../fonts.c"
TEST_HARNESS();
int main(void) {
 tbx_host_reset(); CHECK(tbx_app_main()==0,"launch");
 CHECK(tbx_host.nwindows==1,"single specimen");
 CHECK(!strcmp(tbx_host.windows[0].title,"Font Manager Styles Specimen"),"long title");
 tbx_event_t e={.what=updateEvt};tbx_app_event(&e);
 const unsigned faces[]={1,257,513,1,1025,1793,257};unsigned n=0;
 for(unsigned i=0;i<tbx_host.nops;i++)if(!strcmp(tbx_host.ops[i].name,"tbx_draw_text")) {
  CHECK(n<7,"seven runs");if(n<7)CHECK((unsigned)tbx_host.ops[i].a[3]==faces[n],"literal face bits"); n++;
 }
 CHECK(n==7,"all specimens rendered");
 CHECK(tbx_host.bar && tbx_host.bar->menus[1].n_items==4,"four Style specimens");
 e.what=TBX_EVT_MENU;e.message=TBX_MENU_WORD(129,1);tbx_app_event(&e);
 CHECK(tbx_host.exit_calls==1,"quit");return TEST_SUMMARY("fonts route");
}
