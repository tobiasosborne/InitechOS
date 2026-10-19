/* Ref: tdnl.88; system7-decomp/specs/toolbox/control-manager.md,
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
    return TEST_SUMMARY("test-dialog-tracking");
}
