/* initech-vlhp: DEC-16 Sec 3.3/C-6; MS-DOS 3.3 Reference p.38
 * (PDF123). Real dispatcher; independent abort + keyboard witnesses. */
#include <stdio.h>
#include <string.h>
#include "int21.h"
#include "psp.h"
#include "sft.h"
#include "devices.h"

static psp_t resident;
static unsigned char chunk[128];
static char nul_name[] = "NUL";

static int pending, polls, exits;
static char display[64];
static unsigned display_n;
static void sink(char c) { if(display_n+1<sizeof display)display[display_n++]=c; display[display_n]=0; }
static int poll_key(void) { polls++; int c=pending; pending=-1; return c; }
static int get_key(void) { return poll_key(); }
static void exit_hook(uint8_t code) { (void)code; exits++; }
static int check(int yes, const char *why)
{
    if (!yes) fprintf(stderr,"RED dos-break: %s\n",why);
    return !yes;
}
static int_frame_t call(unsigned ax, unsigned bx, unsigned cx, void *p)
{
    int_frame_t f={0};
    f.eax=ax; f.ebx=bx; f.ecx=cx; f.edx=(uint32_t)(uintptr_t)p;
    int21_dispatch(&f);
    return f;
}

/* initech-c80y: User's Guide printed p.159 / PDF244: ^C aborts the
 * current command. The resident keeps its JFT until CLOSE/DUP2 unwind it;
 * a second queued ^C must not interrupt those cleanup calls. */
static int resident_boundaries(void)
{
    int bad=0;
    const unsigned boundaries[]={0x3f00,0x4000,0x0800};
    for (unsigned mode=0;mode<4;mode++) {
        for (unsigned i=0;i<3;i++) {
            psp_params_t params={0}; params.alloc_end_linear=0x80000;
            psp_build(&resident,&params); sft_init(); int21_set_psp(&resident);
            devices_init(); int21_set_devices(devices_head(),NULL);
            pending=-1; exits=0; int21_set_break_flag(1);
            uint8_t before[20]; memcpy(before,resident.jft,sizeof before);
            uint16_t refs[SFT_FIRST_FILE];
            for(unsigned j=0;j<SFT_FIRST_FILE;j++)refs[j]=g_sft[j].ref_count;
            int h=(int)call(0x3d02,0,0,nul_name).eax;
            int saved[2]={-1,-1};
            for (unsigned j=0;j<2;j++) if (mode&(1u<<j)) {
                saved[j]=(int)(call(0x4500,j,0,NULL).eax&255u);
                call(0x4600,(unsigned)h,j,NULL);
            }
            uint8_t redirected[20]; memcpy(redirected,resident.jft,sizeof redirected);
            int21_break_recovery_t scope={&resident,0};
            int21_break_recovery_t *old=int21_set_break_recovery(&scope);
            memset(chunk,0xa5,sizeof chunk); pending=3;
            /* AH=08h is DIR /P's wait; unlike disk entry it checks even OFF. */
            if (i==2) int21_set_break_flag(0);
            (void)call(boundaries[i],(unsigned)h,sizeof chunk,chunk);
            bad+=check(scope.interrupted && !exits,"resident read/write/pause must latch cancellation without exit");
            bad+=check(!memcmp(redirected,resident.jft,sizeof redirected),"cancel must preserve redirected JFT");
            for (unsigned j=0;j<sizeof chunk;j++)
                bad+=check(chunk[j]==0xa5,"interrupted read must not manufacture file bytes");
            int21_set_break_flag(1); pending=3; /* another key during cleanup */
            for (unsigned j=0;j<2;j++) if (saved[j]>=0) {
                int_frame_t f=call(0x4600,(unsigned)saved[j],j,NULL);
                bad+=check(!(f.eflags&1),"cancel cleanup DUP2 must complete");
                call(0x3e00,(unsigned)saved[j],0,NULL);
            }
            call(0x3e00,(unsigned)h,0,NULL);
            bad+=check(!memcmp(before,resident.jft,sizeof before),"cancel cleanup must CLOSE and restore stdin/stdout on every path");
            for(unsigned j=0;j<SFT_FIRST_FILE;j++)
                bad+=check(refs[j]==g_sft[j].ref_count,"cancel cleanup must balance resident SFT references");
            for(unsigned j=SFT_FIRST_FILE;j<SFT_MAX_ENTRIES;j++)
                bad+=check(g_sft[j].kind==SFT_KIND_FREE,"cancel cleanup must not leak owned SFT slots");
            bad+=check(pending==3,"cleanup must leave the next Ctrl-C for the prompt");
            int21_set_break_recovery(old); pending=-1;
        }
    }
    /* Even an already interrupted parent scope must not shelter a child. */
    {
        static psp_t child;
        psp_params_t params={0}; params.alloc_end_linear=0x80000;
        params.parent_jft=resident.jft;
        psp_build(&child,&params); sft_inherit(&child);
        uint8_t before[20]; memcpy(before,resident.jft,sizeof before);
        int21_break_recovery_t scope={&resident,1};
        int21_break_recovery_t *old=int21_set_break_recovery(&scope);
        int21_set_psp(&child); pending=3; exits=0;
        call(0x3f00,0xffff,sizeof chunk,chunk);
        bad+=check(exits==1 && scope.interrupted==1,"child INT23 must terminate even while parent recovery is latched");
        bad+=check(!memcmp(before,resident.jft,sizeof before),"child abort must preserve parent handles");
        int21_set_break_recovery(old); pending=-1;
    }
    int21_set_psp(NULL);
    return bad;
}
int main(void)
{
    int bad=0;
    const unsigned calls[]={0x30,0x36,0x3f,0x40};
    int21_set_exit(exit_hook);
    int21_set_sink(sink);
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
    int_frame_t legacy={0}; pending=3; polls=exits=0; display_n=0;
    int21_dispatch(&legacy);
    /* Entry poll plus one per displayed caret/CR/LF byte (Guide p.159).
     * The original pre-caret test incorrectly counted only the entry poll. */
    bad+=check(exits==1 && polls==5 && pending==-1 && !strcmp(display,"^C\r\n"),
        "legacy AH=00h is a non-console checkpoint too");
    int_frame_t f={0}; f.eax=0x3000; pending='x'; exits=0;
    int21_dispatch(&f);
    bad+=check(!exits && (f.eax&0xffff)==0x1e03,"ordinary typeahead must not abort/version must run");
    f.eax=0x0700; int21_dispatch(&f);
    bad+=check((f.eax&255)=='x',"ordinary typeahead must survive non-console check");
    pending=3; f.eax=0x0700; int21_dispatch(&f);
    bad+=check((f.eax&255)==3 && !exits,"direct AH=07h remains raw");
    bad+=resident_boundaries();
    /* 0qw1: ECHO/GOTO has no blocking input; the boundary poll must still
     * detect Ctrl-C with BREAK OFF and preserve ordinary queued input. */
    {
        psp_params_t params={0};params.alloc_end_linear=0x80000;
        psp_build(&resident,&params);sft_init();int21_set_psp(&resident);
        int21_break_recovery_t scope={&resident,0};
        int21_break_recovery_t *old=int21_set_break_recovery(&scope);
        int21_set_break_flag(0);pending='q';exits=0;
        int21_batch_checkpoint();
        f=call(0x0700,0,0,NULL);
        bad+=check(!scope.interrupted && (f.eax&255)=='q',"batch checkpoint must preserve ordinary typeahead");
        pending=3;int21_batch_checkpoint();
        bad+=check(scope.interrupted && !exits,"batch checkpoint must recover Ctrl-C with BREAK OFF");
        pending=3;polls=0;int21_batch_checkpoint();
        bad+=check(!polls && pending==3,"batch checkpoint must protect an unwinding scope");
        int21_set_break_recovery(old);int21_set_psp(NULL);pending=-1;
    }
    if (bad) return 1;
    puts("GREEN test-dos-break: ON/OFF/raw/typeahead; resident read/write/pause; CLOSE, stdin/stdout and SFT cleanup");
    return 0;
}
