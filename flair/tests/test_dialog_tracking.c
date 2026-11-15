/* Ref:; system7-decomp/specs/toolbox/control-manager.md,
 * TrackControl releases inside the ORIGINAL control. Explicit event stream. */
#define main legacy_dialog_main
#include "test_dialog.c"
#undef main

/* The still's boxes, read from goldens/frame/geometry.tsv (640x480 columns
 * X0_640..Y1_480), never from the renderer. */
typedef struct { int x0,y0,x1,y1,found; } still_box;
static still_box still(const char *id)
{
    still_box b={0,0,0,0,0};
    char line[512];
    FILE *f=fopen(FRAME_GEOMETRY,"r");
    if(!f){perror(FRAME_GEOMETRY);exit(2);}
    while(fgets(line,sizeof line,f)){
        char *field[16];int n=0;
        for(char *p=line;n<16;){field[n++]=p;p=strchr(p,'\t');if(!p)break;*p++=0;}
        if(n>=14&&!strcmp(field[0],id)){
            b.x0=atoi(field[10]);b.y0=atoi(field[11]);b.x1=atoi(field[12]);b.y1=atoi(field[13]);b.found=1;
        }
    }
    fclose(f);
    if(!b.found){fprintf(stderr,"geometry.tsv: no %s row\n",id);exit(2);}
    return b;
}

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
    /* Frame-owner L1: the live copy sits at the still's FILE COPY box. The
     * still has no Cancel; GM3 keeps one, so the box may only grow downward
     * to hold it below the bar, inside the frame. */
    still_box copy=still("COPY"),cap=still("COPYCAPTION"),bar=still("COPYBAR");
    rgn_rect_t box=dp->window.port.portRect,c=g_mdl_items[2].rect;
    rgn_rect_t t=g_mdl_items[0].rect,b=g_mdl_items[1].rect;
    CHECK(box.left==copy.x0 && box.top==copy.y0 && box.right==copy.x1 && box.bottom>=copy.y1,
          "live copy box is the still's FILE COPY box");
    CHECK(t.left==cap.x0 && t.top==cap.y0 && t.right>=cap.x1 && t.right<=box.right-8,
          "caption starts at the still's caption and stays inside the box");
    CHECK(b.left==bar.x0 && b.right==bar.x1 && b.top+b.bottom==bar.y0+bar.y1,
          "bar spans the still's fill, centred on its rows");
    CHECK(c.top>=bar.y1 && c.left>box.left && c.right<box.right && c.bottom<box.bottom &&
          box.bottom-c.bottom==10 && box.right-c.right==14,
          "Cancel lies below the bar within the live dialog");
    CHECK(region_get_bbox(&g_mdl_sruc.r).bottom==box.bottom &&
          region_get_bbox(&g_mdl_sruc.r).left==box.left,"structure region matches the box");
    hit=0; ev.what=keyDown; ev.message=0x100;
    CHECK(DialogHandleEvent(dp,&ev,0,&hit) && hit==3,"physical Escape selects same Cancel item");
    hit=0; ev.what=mouseDown; ev.where.h=(int16_t)((c.left+c.right)/2); ev.where.v=(int16_t)((c.top+c.bottom)/2);
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit),"Cancel button waits for release");
    ev.what=mouseUp;
    CHECK(DialogHandleEvent(dp,&ev,0,&hit) && hit==3,"Cancel button releases into same completion");
    hit=0; ev.what=mouseDown; ev.where.h=(int16_t)(box.left+10); ev.where.v=(int16_t)(box.top+10);
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit),"close box waits for release");
    ev.what=nullEvent; ev.where.h=(int16_t)(box.left+40);
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit),"close tracking can leave box");
    ev.what=mouseUp;
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit) && !hit,"close release outside preserves operation");
    ev.what=mouseDown; ev.where.h=(int16_t)(box.left+10);
    (void)DialogHandleEvent(dp,&ev,0,&hit);
    ev.what=mouseUp;
    CHECK(DialogHandleEvent(dp,&ev,0,&hit) && hit==3,"close box releases into same completion");
    hit=0;
    CHECK(!DialogHandleEvent(dp,&ev,0,&hit) && !hit,"orphan close release cannot complete twice");
    return TEST_SUMMARY("test-dialog-tracking");
}
