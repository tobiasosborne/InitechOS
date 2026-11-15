#include <stdio.h>
#include "capture_state.h"
static int checks,failures;
#define CHECK(x) do{checks++;if(!(x)){failures++;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);}}while(0)
int main(void)
{
    copy_cursor_state s={0};s.bound=s.visible=s.drawn=1;s.x=520;s.y=350;
    const uint32_t idle=0x1000;
    CHECK(!copy_capture_ready(0x4000,idle,2,&s)); /* erase has not cleared drawn yet */
    CHECK(copy_capture_ready(idle+1,idle,2,&s)); /* ordinary HLT yield */
    CHECK(!copy_capture_ready(idle+2,idle,2,&s));
    s.depth=1;CHECK(!copy_capture_ready(idle,idle,2,&s));s.depth=0;
    s.drawn=0;CHECK(!copy_capture_ready(idle,idle,2,&s));s.drawn=1;
    s.visible=0;CHECK(!copy_capture_ready(idle,idle,2,&s));s.visible=1;
    s.bound=0;CHECK(!copy_capture_ready(idle,idle,2,&s));s.bound=1;
    s.x=519;CHECK(!copy_capture_ready(idle,idle,2,&s));s.x=520;
    s.y=351;CHECK(!copy_capture_ready(idle,idle,2,&s));s.y=350;
    CHECK(copy_capture_ready(idle,idle,2,&s));
    printf("INITECH_CHECKS %d failures=%d\n",checks,failures);return failures?1:0;
}
