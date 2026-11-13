/* Routing oracle for M024/M025: invoke the REAL Calc tenant, capture its
 * Toolbox calls. PRD Appendix A/B literals are independent of generated data.
 * Scene bounds are the pre-existing kmain fd_win_bounds (authored policy).
 * No preview.webp RGB, no copy of Calc's sector algorithm. */
#define main calc_tenant_main
#include "test_calc_tenant.c"
#undef main
#include "film_scene.h"
#include "cursor.h"
static WindowRecord windows[2];
static int moves, sizes;
void MoveWindow(WindowMgr *wm, WindowPtr w, int16_t l, int16_t t)
{
    (void)wm; int i=w==&windows[1]; moves++;
    CHECK(l==(i?300:60)&&t==(i?120:80),"SCENE_POSITION existing authored origin");
}
void SizeWindow(WindowMgr *wm, WindowPtr w, int16_t x, int16_t y)
{
    (void)wm; int i=w==&windows[1]; sizes++;
    CHECK(x==(i?260:300)&&y==(i?240:220),"SCENE_SIZE existing authored extent");
}
int main(void)
{
    int rc=calc_tenant_main();
    /* Independent AM-4 hourglass literal, not the rendered asset table. Also
     * verifies shape switches erase old pixels at an unchanged hotspot. */
    static uint32_t fb[40*40];
    for(int i=0;i<40*40;i++) fb[i]=0x123456;
    flair_cursor_init(fb,160,40,40,32);
    flair_cursor_show(20,20);
    EventRecord escape={0}; escape.what=keyDown; escape.message=0x0100;
    CHECK(flair_film_dismiss(&escape),"scene Escape uses cooked vkey, not only ASCII");
    escape.message='a'; CHECK(!flair_film_dismiss(&escape),"scene ignores ordinary key");
    escape.what=mouseDown; escape.message=27; CHECK(!flair_film_dismiss(&escape),"scene mouse does not dismiss");
    flair_film_cursor(1);
    CHECK(fb[12*40+12]==0&&fb[12*40+27]==0&&fb[19*40+19]==0,
          "BUSY_HOURGLASS Saving selects hourglass at hotspot 8,8");
    flair_cursor_shield(); flair_film_cursor(0); flair_cursor_unshield();
    CHECK(fb[12*40+12]==0x123456,"BUSY_RESTORE save-under restored on cursor selection");
    flair_cursor_hide();
    for(int i=0;i<40*40;i++) CHECK(fb[i]==0x123456,"cursor erases without trails");
    flair_film_arrange(0,&windows[0],&windows[1]);
    CHECK(moves==2&&sizes==2,"SCENE_ROUTING both tenant windows arranged through WM");
    printf("test_film_scene: %d checks, %d failures\n",checks,failures);
    return rc||failures?1:0;
}
