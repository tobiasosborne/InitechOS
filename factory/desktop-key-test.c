/* Independent RFC6143 byte contract: release a tap before any host wait. */
#define main desktop_driver_main
#include "desktop-bochs.c"
#undef main
int main(void)
{
    int fd[2];if(socketpair(AF_UNIX,SOCK_STREAM,0,fd))return 2;
    RfbShadow rfb={.fd=fd[0]};
    BochsTraceStep step={.kind=BX_KEY,.shift=true};
    bool down=false;
    if(!bochs_key_step(&rfb,&step,'a',&down,BOCHS_TRACE_TAP_KEYS) || down) {
        puts("FAIL desktop key: tap left held across a host wait");return 1;
    }
    const unsigned char expected[]={
        4,1,0,0,0,0,255,225, 4,1,0,0,0,0,0,97,
        4,0,0,0,0,0,0,97, 4,0,0,0,0,0,255,225};
    unsigned char got[sizeof expected];size_t n=0;
    while(n<sizeof got) {ssize_t r=read(fd[1],got+n,sizeof got-n);if(r<=0)return 2;n+=(size_t)r;}
    if(memcmp(got,expected,sizeof got)) {puts("FAIL desktop key: modifier/tap wire ordering");return 1;}
    /* Ordinary Bochs callers still hold until their second replay step. */
    down=false;
    if(!bochs_key_step(&rfb,&step,'a',&down,false) || !down)return 1;
    if(!bochs_key_step(&rfb,&step,'a',&down,false) || down)return 1;
    close(fd[0]);close(fd[1]);
    puts("ok desktop key: atomic tap and modifier release before wait; ordinary two-step keys preserved");return 0;
}
