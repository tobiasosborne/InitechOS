/* initech-vlhp: DEC-16 Sec 3.3/C-6; MS-DOS 3.3 Reference p.38
 * (PDF123). Real dispatcher; independent abort + keyboard witnesses. */
#include <stdio.h>
#include <string.h>
#include "int21.h"

static int pending, polls, exits;
static int poll_key(void) { polls++; int c=pending; pending=-1; return c; }
static int get_key(void) { return poll_key(); }
static void exit_hook(uint8_t code) { (void)code; exits++; }
static int check(int yes, const char *why)
{
    if (!yes) fprintf(stderr,"RED dos-break: %s\n",why);
    return !yes;
}
int main(void)
{
    int bad=0;
    const unsigned calls[]={0x30,0x36,0x3f,0x40};
    int21_set_exit(exit_hook);
    int21_set_conin(get_key,poll_key);
    for (unsigned i=0;i<sizeof(calls)/sizeof(calls[0]);i++) {
        int_frame_t f={0}; f.eax=calls[i]<<8; f.ebx=0xffff;
        int21_set_break_flag(0); pending=3; polls=exits=0;
        int21_dispatch(&f);
        bad+=check(!exits && !polls && pending==3,"OFF must leave disk-call Ctrl-C queued");
        f=(int_frame_t){0}; f.eax=calls[i]<<8; f.ebx=0xffff;
        int21_set_break_flag(1); pending=3; polls=exits=0;
        int21_dispatch(&f);
        bad+=check(exits==1 && pending==-1,"ON must abort non-console call before service");
    }
    int_frame_t legacy={0}; pending=3; polls=exits=0;
    int21_dispatch(&legacy);
    bad+=check(exits==1 && polls==1 && pending==-1,
        "legacy AH=00h is a non-console checkpoint too");
    int_frame_t f={0}; f.eax=0x3000; pending='x'; exits=0;
    int21_dispatch(&f);
    bad+=check(!exits && (f.eax&0xffff)==0x1e03,"ordinary typeahead must not abort/version must run");
    f.eax=0x0700; int21_dispatch(&f);
    bad+=check((f.eax&255)=='x',"ordinary typeahead must survive non-console check");
    pending=3; f.eax=0x0700; int21_dispatch(&f);
    bad+=check((f.eax&255)==3 && !exits,"direct AH=07h remains raw");
    if (bad) return 1;
    puts("GREEN test-dos-break: ON aborts disk/non-console entry; OFF/raw preserve Ctrl-C; typeahead preserved");
    return 0;
}
