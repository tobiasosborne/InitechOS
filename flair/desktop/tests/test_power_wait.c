/* IM VI6-36/6-37 Listing6-8: quit acknowledgement follows the user's answer,
 * never the first callback return. The test supplies an independent app
 * response script; actual Finder orchestration and shutdown dialog run here. */
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "finder_actions.h"
#include "test_assert.h"
TEST_HARNESS();
/* Replace privileged hardware bindings only in this host translation unit. */
#define kernel_reset unused_hardware_reset
#define kernel_halt unused_hardware_halt
#include "power.h"
#undef kernel_reset
#undef kernel_halt
uint32_t kernel_reset(void);
void kernel_halt(void);
#include "../power.c"
int g_desktop_exit,g_finder_db_dirty;
flair_raw_ring_t g_flair_kbd_ring;
static int resident,waiting,ask_result,asks,services,notices,persists,resets,safe_screen;
static jmp_buf reset_jump;
int tbx_ask_quit(void){asks++;return ask_result;}
const char *tbx_resident_name(void){return resident?"WORD.EXE":NULL;}
int tbx_quit_pending(void){return resident&&waiting;}
void flair_live_tenant_heard(flair_live_ctx_t *ctx,const boot_info_t *bi){(void)ctx;(void)bi;services++;}
void finder_desk_persist(void){persists++;}
void serial_puts(const char *s){if(!strcmp(s,"FLAIR-SHUTDOWN-SAFE\n"))safe_screen++;}
void serial_puthex32(uint32_t n){(void)n;}
void serial_puti(int32_t n){(void)n;}
void serial_putu(uint32_t n){(void)n;}
void serial_putc(char c){(void)c;}
uint32_t kernel_reset(void){resets++;longjmp(reset_jump,1);}
void kernel_halt(void){exit(2);}
void flair_live_cursor_track(const EventRecord *ev){(void)ev;}
void flair_live_emit_evt(const EventRecord *ev){(void)ev;}
void flair_desktop_present(const boot_info_t *b,const bitmap_t *off){(void)b;(void)off;}
int WaitNextEvent(flair_raw_ring_t *ring,uint16_t mask,EventRecord *ev,uint32_t sleep)
{(void)ring;(void)mask;(void)sleep;memset(ev,0,sizeof *ev);ev->what=keyDown;ev->message='\r';return 1;}
void fta_rgn(finder_win_rgn_t *s)
{memset(s,0,sizeof *s);s->r.rows=s->rows;s->r.cap_rows=FINDER_WIN_RGN_ROWS;s->r.x_pool=s->pool;s->r.x_pool_cap=FINDER_WIN_RGN_POOL;region_set_empty(&s->r);}
int finder_exit_dos_alert_text(char lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX])
{strcpy(lines[0],"Exit");return 1;}
void flair_live_alert_open(flair_live_ctx_t *ctx,const boot_info_t *bi,const char *const *text,int n,const char *const *buttons,int nb,int draw)
{(void)ctx;(void)bi;(void)buttons;(void)nb;(void)draw;if(n==3){notices++;CHECK(!strcmp(text[1],"WORD.EXE"),"cancellation names the resident app");}}
uint16_t flair_live_alert_run(flair_live_ctx_t *ctx,const boot_info_t *bi){(void)ctx;(void)bi;return 1;}
static void fresh(void)
{finder_power_reset();resident=waiting=ask_result=1;asks=services=notices=persists=resets=safe_screen=g_desktop_exit=0;}
int main(void)
{
 static uint8_t pixels[640*480];
 flair_live_ctx_t ctx={0};ctx.off=(bitmap_t){.base=pixels,.pitch=640,.bpp=8,.bytes_per_pixel=1,.width=640,.height=480};
 fresh();finder_power(&ctx,0,FCMD_RESTART,0);
 CHECK(asks==1&&services==1&&!notices&&!persists&&!resets,"Restart waits for the application after the callback yields");
 for(int k=0;k<100;k++)finder_power_poll(&ctx,0);
 CHECK(!notices&&!persists&&!resets,"elapsed pump turns never imply refusal");
 finder_power(&ctx,0,FCMD_SHUTDOWN,0);CHECK(asks==1,"another power request cannot replace the pending action");
 waiting=0;finder_power_poll(&ctx,0);
 CHECK(notices==1&&!persists&&!resets&&resident,"explicit cancellation leaves the app and cancels Restart");
 finder_power_poll(&ctx,0);CHECK(notices==1,"cancellation is consumed exactly once");
 fresh();finder_power(&ctx,0,FCMD_RESTART,0);resident=waiting=0;
 if(!setjmp(reset_jump))finder_power_poll(&ctx,0);
 CHECK(persists==1&&resets==1&&!notices&&!safe_screen,"completed quit continues Restart exactly once");
 fresh();finder_power(&ctx,0,FCMD_SHUTDOWN,0);
 CHECK(!safe_screen&&!persists,"Shut Down has no visible/physical effect before quit answer");
 resident=waiting=0;if(!setjmp(reset_jump))finder_power_poll(&ctx,0);
 CHECK(safe_screen==1&&persists==1&&resets==1,"accepted Shut Down reaches its safe screen and Restart button");
 fresh();finder_exit_dos(&ctx,0,0);CHECK(!g_desktop_exit&&!notices,"confirmed Exit to DOS still awaits app answer");
 resident=waiting=0;finder_power_poll(&ctx,0);
 CHECK(g_desktop_exit&&persists==1&&!resets,"completed app quit continues Exit to DOS");
 fresh();ask_result=-1;waiting=0;finder_power(&ctx,0,FCMD_RESTART,0);
 CHECK(notices==1&&!services&&!persists&&!resets,"unaskable application cancels without termination");
 return TEST_SUMMARY("test-power-wait");
}
