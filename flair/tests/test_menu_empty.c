/* Authored GM4 policy: Inside Macintosh IV, printed IV-55, 128K ROM empty
 * menus highlight their title and never pull down a panel. Literal Paint
 * shape; independent before/after pixels and empty-set/damage invariants. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "menu.h"
#include "render.h"
#include "test_assert.h"
TEST_HARNESS();
static const MenuItem file_items[]={
 {"Close",0,'W',0,1,0},{"Quit",0,'Q',0,1,0}
};
static MenuInfo menus[]={
 {1,"File",file_items,2,0},{2,"Edit",0,0,0},{3,"Image",0,0,0},
 {4,"Layer",0,0,0},{5,"Select",0,0,0},{6,"View",0,0,0},
 {7,"Window",0,0,0},{8,"Help",0,0,0}
};
static int zero(rgn_rect_t r) { return !r.top && !r.left && !r.bottom && !r.right; }
int main(void)
{
 MenuBar bar={menus,8,1};
 render_boot_info_t boot={0};
 boot.lfb_bpp=32; boot.lfb_width=640; boot.lfb_height=140;
 render_ctx_t ctx;
 CHECK(render_ctx_init(&ctx,&boot)==0,"empty-menu render surface");
 DrawMenuBar(&ctx.port,&bar,-1,0);
 const bitmap_t *bm=&ctx.fb.bm;
 size_t bytes=(size_t)bm->pitch*bm->height;
 uint8_t *before=malloc(bytes); CHECK(before!=0,"pixel snapshot");
 memcpy(before,(const void *)bm->base,bytes);
 for(int mi=1;mi<8;mi++) {
  menus[mi].menuWidth=300; /* a stale width cache must not create a panel */
  CHECK(zero(MenuInfo_panel_rect(&bar,mi)),"zero rows have exactly zero panel rectangle");
  CHECK(zero(MenuInfo_panel_footprint_rect(&bar,mi)),"zero rows have no shadow/damage footprint");
  int x=MenuBar_title_x(&bar,mi)+12;
  CHECK(MenuBar_hit(&bar,x)==mi,"empty title remains hittable");
  MenuTrackState st;
  MenuTrack_begin(&bar,(flair_point_t){10,(int16_t)x},&st);
  CHECK(st.mi==mi && st.hi==-1,"hold empty title tracks its highlight");
  DrawMenuBar(&ctx.port,&bar,st.mi,0);
  flair_draw_menu_panel(&ctx.port,&bar,st.mi,st.hi,0);
  CHECK(!memcmp((const uint8_t *)bm->base+20*bm->pitch,before+20*bm->pitch,bytes-20*bm->pitch),
        "held empty title changes no below-bar pixel, including shadow");
  rgn_rect_t hr=MenuBar_hilite_rect(&bar,mi);
  CHECK(render_pixel_rgb(&ctx,(uint32_t)(hr.left+1),8)==0x27666c,
        "held empty title has the authored pulled-title face");
  MenuTrack_step(&bar,&st,(flair_point_t){40,(int16_t)x});
  CHECK(st.hi==-1 && MenuTrack_result(&bar,&st)==0 && MenuInfo_item_at(&bar,mi,x,20)==-1,
        "release below empty title cannot select a dummy item");
  DrawMenuBar(&ctx.port,&bar,-1,0);
  CHECK(!memcmp((const void *)bm->base,before,bytes),"release restores complete idle image");
 }
 MenuTrackState st;
 MenuTrack_begin(&bar,(flair_point_t){10,90},&st); /* Edit */
 CHECK(st.mi==1 && zero(MenuInfo_panel_footprint_rect(&bar,st.mi)),"empty drag starts with no damage");
 MenuTrack_step(&bar,&st,(flair_point_t){10,52}); /* File */
 rgn_rect_t file=MenuInfo_panel_footprint_rect(&bar,st.mi);
 CHECK(st.mi==0 && file.right>file.left && file.bottom>file.top,"drag into File opens its real panel");
 MenuTrack_step(&bar,&st,(flair_point_t){10,90});
 CHECK(st.mi==1 && zero(MenuInfo_panel_footprint_rect(&bar,st.mi)) && MenuTrack_result(&bar,&st)==0,
       "drag back into empty title leaves only the previous File footprint to erase");
 /* No item pointer may be read for an empty system menu either. */
 MenuBar_set_apple_menu(&menus[1]);
 CHECK(zero(MenuInfo_panel_rect(&bar,FLAIR_MENU_APPLE_MI)),"empty system menu shares zero-row guard");
 MenuBar_set_apple_menu(0);
 free(before); render_ctx_free(&ctx);
 return TEST_SUMMARY("test-menu-empty");
}
