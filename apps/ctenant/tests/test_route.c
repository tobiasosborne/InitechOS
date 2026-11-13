/* Input and repaint literals from the claimed legacy example contract. */
#include "tbx_host.h"
#include "test_assert.h"
#include "../ctenant.c"
TEST_HARNESS();
static int drawn(const char *verb,const char *text) {
 for(unsigned i=0;i<tbx_host.nops;i++)if(!strcmp(tbx_host.ops[i].name,verb)&&!strcmp(tbx_host.ops[i].text,text))return 1;
 return 0;
}
int main(void) {
 tbx_host_reset();CHECK(tbx_app_main()==0,"launch");
 tbx_event_t e={.what=keyDown,.message='a'};tbx_app_event(&e);
 CHECK(drawn("tbx_draw_text","Key a"),"key proportional output");
 CHECK(drawn("tbx_draw_cells","Key a"),"key fixed-grid output");
 e.what=mouseDown;tbx_app_event(&e);CHECK(drawn("tbx_draw_cells","Click"),"click");
 tbx_host.nops=0;e.what=updateEvt;tbx_app_event(&e);
 CHECK(drawn("tbx_draw_text","Key a"),"retain key after update");
 CHECK(drawn("tbx_draw_cells","Click"),"retain click after update");
 e.what=TBX_EVT_MENU;e.message=TBX_MENU_WORD(129,1);tbx_app_event(&e);
 CHECK(tbx_host.exit_calls==1,"quit");return TEST_SUMMARY("C example route");
}
