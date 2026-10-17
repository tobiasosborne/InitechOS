/* CPANELS.EXE -- one disk tenant, five Dialog Manager clients (tdnl.19).
 * Ref: period-gui-suite.md sec5; calcda.c one launch/callback/yield pattern;
 * cp_dialog.h transport; cpanels/record.json persistence; no native layout
 * captures: composed existing controls, not a native pixel replica. */
#include "tbx.h"
#include "cpanels.h"
const char tbx_app_name[]="CPANELS";
static cp_state_t state TBX_IN_IMAGE={0};
static cp_item_t items[CP_MAX_ITEMS] TBX_IN_IMAGE={0};
static uint8_t record[CP_RECORD_BYTES] TBX_IN_IMAGE={0};
static int32_t win;
static tbx_event_t event TBX_IN_IMAGE={0};
static const tbx_menuitem_t file_items[]={{"Quit",0,'Q',0,1,0,{0,0,0}}};
static const tbx_menuinfo_t menus[]={{129,0,"File",file_items,1,0}};
static const tbx_menubar_t bar={menus,1,1,0};
static int32_t settings(unsigned mode) {uint32_t a[]={mode,(uint32_t)(uintptr_t)record};return tbx_trap(TBX_SETTINGS,2,a);}
static int32_t clock(unsigned mode) {uint32_t a[]={mode,(uint32_t)(uintptr_t)&state.time};return tbx_trap(TBX_CLOCK,2,a);}
static int save(void *u,const cp_settings_t *s) {
 (void)u;cp_encode(s,record);int h=tbx_file_create(CP_PATH);if(h<0)return 0;
 int n=tbx_file_write(h,record,CP_RECORD_BYTES);int c=tbx_file_close(h);return n==CP_RECORD_BYTES && c==0;
}
static int apply(void *u,const cp_settings_t *s){(void)u;cp_encode(s,record);return settings(1)==0;}
static int setclock(void *u,const cp_time_t *t){(void)u;(void)t;return clock(1)==0;}
static void beep(void *u) {
 (void)u;
 /* Preview is temporary, never a persisted setting. Restore the live pump
  * parameters before yielding, so Cancel leaves no changed global state. */
 cp_encode(&state.edit,record);(void)settings(1);(void)tbx_trap(TBX_BEEP,0,0);
 cp_encode(&state.live,record);(void)settings(1);
}
static const cp_ops_t ops={save,apply,setclock,beep,0};
static int dialog(const tbx_event_t *e) {
 unsigned n=cp_items(&state,items);uint32_t a[]={(uint32_t)win,(uint32_t)(uintptr_t)items,n,1,2,(uint32_t)(uintptr_t)e};
 return tbx_trap(TBX_DIALOG,6,a);
}
static void paint(void) {
 (void)dialog(0);
 if(state.panel==2) {
  /* QuickDraw's MSB-left 8x8 PAT# resource, preview samples only; existing
   * FILLRECT writer. Same phase as desktop.c; solid teal remains choice 0. */
  for(unsigned p=1;p<=8;p++)for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++) {
   int l=24+120*(int)(p%3)+(int)x,t=72+40*(int)(p/3)+(int)y;
   (void)tbx_fill_rect(win,l,t,l+1,t+1,(cp_patterns[p-1][y]&(128u>>x))?TBX_COLOR_BLACK:TBX_COLOR_WHITE);
  }
 }
}
int tbx_app_main(void) {
 cp_settings_t p;cp_defaults(&p);
 if(tbx_register()!=0)return 1;
 if(settings(0)==0)(void)cp_decode(&p,record);
 cp_init(&state,&p);(void)clock(0);
 {uint32_t a[]={(uint32_t)(uintptr_t)state.memory};(void)tbx_trap(TBX_MEMORY,1,a);}
 (void)tbx_set_mbar(&bar);win=tbx_new_window(120,90,520,390,TBX_GOAWAY_ASK);
 if(win<=0)return 1;
 (void)tbx_set_wtitle(win,"Control Panels");return 0;
}
void tbx_app_event(const tbx_event_t *e) {
 if(e->what==TBX_EVT_GOAWAY || (e->what==TBX_EVT_MENU && e->message==TBX_MENU_WORD(129,1))) {(void)tbx_exit(0);return;}
 if(e->what==updateEvt){paint();return;}
 if(e->what!=mouseDown && e->what!=keyDown && e->what!=autoKey)return;
 event=*e;unsigned before=state.panel;int hit=dialog(&event);
 if(hit>0) {
  if(!before)(void)clock(0);
  cp_hit(&state,(unsigned)hit,&ops);
  if(state.closed){(void)tbx_exit(0);return;}
  paint();
 }
}
