/* Ref:; system7-decomp/specs/toolbox/control-manager.md,
 * TrackControl releases inside the ORIGINAL control. Explicit event stream. */
#define main legacy_dialog_main
#include "test_dialog.c"
#undef main

int main(void)
{
    DialogPtr dp=setup_modal_dialog(1,3);
    EventRecord ev={0};
    uint16_t hit=0;
    ev.what=mouseDown; ev.where.h=200; ev.where.v=135;
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit) && hit==0,
          "RELEASE_INSIDE press does not fire");
    ev.what=nullEvent; ev.where.h=300;
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit),"RELEASE_INSIDE drag outside waits");
    ev.what=mouseUp;
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit) && hit==0,
          "RELEASE_INSIDE release outside cancels");
    ev.what=mouseUp; ev.where.h=200;
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit),"RELEASE_INSIDE orphan release ignored");
    ev.what=mouseDown;
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit),"RELEASE_INSIDE second press waits");
    ev.what=nullEvent; ev.where.h=300;
    (void)DialogHandleEvent(dp,&ev,0,&hit);
    ev.where.h=200;
    (void)DialogHandleEvent(dp,&ev,0,&hit);
    ev.what=mouseUp;
    CHECK(DialogHandleEvent(dp,&ev,0,&hit) && hit==1,
          "RELEASE_INSIDE reenter then release fires once");
    static ControlRecord copy_controls[2];
    dp=FileCopyCancelableDialog(&g_mdl_dr,g_mdl_items,copy_controls,
                                &g_mdl_sruc.r,&g_mdl_scont.r,&g_mdl_supd.r);
    CHECK(dp && dp->itemCount==3 && dp->cancelItem==3 && dp->window.goAwayFlag,
          "live copy has one enabled Cancel item and a close box");
    CHECK(g_mdl_items[2].rect.top==278 && g_mdl_items[2].rect.left==414 &&
          g_mdl_items[2].rect.bottom==298 && g_mdl_items[2].rect.right==486,
          "Cancel lies below the bar within the extended live dialog");
    hit=0; ev.what=keyDown; ev.message=0x100;
    CHECK(DialogHandleEvent(dp,&ev,0,&hit) && hit==3,"physical Escape selects same Cancel item");
    hit=0; ev.what=mouseDown; ev.where.h=450; ev.where.v=288;
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit),"Cancel button waits for release");
    ev.what=mouseUp;
    CHECK(DialogHandleEvent(dp,&ev,0,&hit) && hit==3,"Cancel button releases into same completion");
    hit=0; ev.what=mouseDown; ev.where.h=150; ev.where.v=210;
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit),"close box waits for release");
    ev.what=nullEvent; ev.where.h=180;
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit),"close tracking can leave box");
    ev.what=mouseUp;
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit) && !hit,"close release outside preserves operation");
    ev.what=mouseDown; ev.where.h=150;
    (void)DialogHandleEvent(dp,&ev,0,&hit);
    ev.what=mouseUp;
    CHECK(DialogHandleEvent(dp,&ev,0,&hit) && hit==3,"close box releases into same completion");
    hit=0;
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit) && !hit,"orphan close release cannot complete twice");
    return TEST_SUMMARY("test-dialog-tracking");
}
