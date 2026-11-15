/* How much InitechCalc asks of the Toolbox to paint, and that painting less
 * draws the same thing (initech-kzras). Every FILLRECT and TEXTDRAW is an
 * INT 81h gate call: on the booted product each one is clipped against the
 * window's visible region and traced to serial, and since ee66540c the pie
 * and the ledger cost about 10,000 of them per smoke session, which took
 * the Bochs desktop journey from 51 s to 159 s against its 90 s limit.
 *
 * The bounds below are a budget, not a measurement of the code. The Bochs
 * journey must finish well inside its 90 s session limit on this machine at
 * its usual load: 10,021 traced gate calls took 159 s, the 1,673 before the
 * pie took 51 s (investigate-bochs-calc). The journey has six updates, and
 * each repaints both windows (FLAIR's flair_route_updates puts a WindowPtr
 * in the updateEvt, not the window's token, so Calc cannot tell which window
 * is damaged), and three keystrokes. One update at most UPDATE_BUDGET calls
 * and one keystroke at most KEY_BUDGET keep the session near 3,600 calls.
 * Seen red on the code before this budget: 1,472 per update, 380 per key.
 *
 * The property: after any keystroke, what the windows show (the replayed
 * calls, tests/replay.h) equals what a fresh full repaint shows. A partial
 * repaint that forgets a pixel (an overflowing label, the cell pointer's
 * old place) fails here. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tbx_host.h"
#include "host_windows.h"
static int32_t frame_origin(int32_t w)
{
    if(w<1||w>tbx_host.nwindows) return TBX_ERR_BADARG;
    const int l=tbx_host.windows[w-1].l+6,t=tbx_host.windows[w-1].t+22;
    return (int32_t)(((uint32_t)(uint16_t)t<<16)|(uint16_t)l);
}
static int32_t frame_size(int32_t w)
{
    if(w<1||w>tbx_host.nwindows) return TBX_ERR_BADARG;
    const int cw=tbx_host.windows[w-1].r-tbx_host.windows[w-1].l-host_inset_w(w);
    const int ch=tbx_host.windows[w-1].b-tbx_host.windows[w-1].t-host_inset_h(w);
    return (int32_t)(((uint32_t)(uint16_t)ch<<16)|(uint16_t)cw);
}
#define tbx_content_origin frame_origin
#define tbx_content_size frame_size
#include "../calc.c"
#undef tbx_content_origin
#undef tbx_content_size
#include "replay.h"

#define UPDATE_BUDGET 600u
#define KEY_BUDGET 80u
static int checks, failures;
static void check(int ok,const char *what,unsigned v)
{
    checks++;
    if(!ok) { failures++; printf("FAIL %s (%u)\n",what,v); }
}
static replay_screen_t shown[2], fresh[2];
static unsigned deliver(tbx_event_t e)
{
    unsigned from=tbx_host.nops;
    if(!tbx_host_push_event(&e)) { puts("event queue full"); exit(2); }
    (void)tbx_host_dispatch(tbx_app_event);
    if(tbx_host.dropped_ops) { puts("DROPPED ops"); exit(2); }
    unsigned n=0;
    for(int w=1;w<=2;w++) n=replay(&shown[w-1],w,from);
    tbx_host.nops=from;
    return n;
}
/* The windows as a full repaint shows them now. */
static int same_as_full_repaint(void)
{
    unsigned from=tbx_host.nops; tbx_event_t e={updateEvt,0,0,0,0,0};
    memset(fresh,0,sizeof fresh);
    if(!tbx_host_push_event(&e)) exit(2);
    (void)tbx_host_dispatch(tbx_app_event);
    for(int w=1;w<=2;w++) (void)replay(&fresh[w-1],w,from);
    tbx_host.nops=from;
    return !memcmp(fresh,shown,sizeof fresh);
}
static unsigned key(unsigned ch,unsigned vk) { return deliver((tbx_event_t){keyDown,(int32_t)(ch|vk<<8),0,0,0,0}); }
static void type(const char *s,const char *what)
{
    for(;*s;s++) {
        unsigned n=key((unsigned char)*s,0);
        check(n<=KEY_BUDGET,what,n);
        check(same_as_full_repaint(),what,0);
    }
}
static void press(unsigned ch,unsigned vk,const char *what)
{
    unsigned n=key(ch,vk);
    check(n<=KEY_BUDGET,what,n);
    check(same_as_full_repaint(),what,0);
}
int main(void)
{
    static char buf[65536]; FILE *f=fopen("data/LEDGER.TXT","rb"); size_t n;
    if(!f) { perror("data/LEDGER.TXT"); return 2; }
    n=fread(buf,1,sizeof buf,f); fclose(f);
    tbx_host_reset();
    if(!tbx_host_put_file("APPS\\LEDGER.TXT",buf,n)||tbx_app_main()!=0) { puts("LAUNCH failed"); return 1; }
    tbx_host.nops=0;
    unsigned u=deliver((tbx_event_t){updateEvt,0,0,0,0,0});
    printf("update: %u calls\n",u);
    check(u<=UPDATE_BUDGET,"one update within its budget",u);
    check(same_as_full_repaint(),"an update is a full repaint",0);
    /* The smoke's keys: 7, 3, Return into the first cell. */
    type("73","typing into the first cell");
    press('\n',0x1C,"Return");
    press(0,0x4D,"Right"); press(0,0x50,"Down"); press(0,0x4B,"Left"); press(0,0x48,"Up");
    /* A label that runs over its empty neighbours, then is cleared. */
    type("'A label that runs over the empty cells to its right","a long label");
    press('\r',0,"Return on the long label");
    press(8,0x0E,"Backspace with nothing typed");
    press(0,0x53,"Del clears the long label");
    /* The last, narrow column: a number runs past the grid's edge. */
    for(int i=0;i<9;i++) press('\t',0x0F,"Tab");
    type("123456789","a number in the narrow column");
    press(27,0x01,"Escape abandons it");
    type("987654321","again");
    press(0,0x50,"Down commits and moves");
    press(0,0x48,"Up");
    press(0,0x53,"Del");
    /* Down into the recovered rows, over the note strip, and back. */
    for(int i=0;i<25;i++) press(0,0x50,"Down");
    for(int i=0;i<25;i++) press(0,0x48,"Up");
    printf("test_calc_paint: %d checks, %d failures\n",checks,failures);
    return failures!=0;
}
