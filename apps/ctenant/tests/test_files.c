#include "tbx_host.h"
#include "test_assert.h"
/* sdk/host lacks reserved DOS-device rejection. Adapt that single legacy
 * precondition here; every ordinary file operation uses the shared fake. */
static int32_t app_create(const char *s);
#define tbx_file_create app_create
#include "file_gate.c"
#undef tbx_file_create
static int32_t app_create(const char *s) {
 return !strcmp(s,"nul.txt") ? TBX_ERR_BADARG : tbx_file_create(s);
}
char __image_start[1]; /* link-only: the deliberate x crash is emulator-only */
TEST_HARNESS();
int main(void) {
 tbx_host_reset();CHECK(tbx_app_main()==0,"file fixture launch");
 tbx_event_t e={.what=keyDown,.message='q'};tbx_app_event(&e);
 const tbx_host_file_t *f=tbx_host_file("KEEP.DAT");
 CHECK(f && f->size==17 && !memcmp(f->data,"Gate file bytes\r\n",17),"independent file bytes");
 CHECK(!tbx_host_file("DELE.DAT"),"delete");
 memcpy(name,"KEEP.DAT",9);e.message='o';tbx_app_event(&e);
 CHECK(tbx_host.exit_calls==1 && tbx_host.exit_rc==0,"fifteen open handles then exit");
 return TEST_SUMMARY("C file example");
}
