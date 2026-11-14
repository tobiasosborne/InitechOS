/* OEM reservation: Microsoft MS-DOS2 SYSCALL.TXT, functions F8..FF.
 * Payload: existing console page ABI, independent literal MCB tiling. */
#define main original_fileio_main
#include "test_fileio.c"
#undef main
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "console.h"
static console_t page;
static void screen(int_frame_t *f)
{ console_page_dispatch(&page,f); }
static int_frame_t invoke_oem(unsigned ax,void *p)
{ int_frame_t f=fresh_frame();f.eax=ax;f.edx=(uintptr_t)p;int21_dispatch(&f);return f; }
static void restart_returns(void) { }
static void restart_exits(void) { _exit(0); }
int main(void)
{
    bind_standard_process();int21_set_break_flag(0);
    uint8_t *arena=alloc_low(1600),*buf=alloc_low(4003);
    CHECK(arena && buf,"low OEM fixtures");if(!arena || !buf)return 2;
    int21_set_mcb_arena(arena,100,(uintptr_t)arena);
    /* Exactly 3 headers, 10 owned + 20 free + 67 free data paragraphs. */
    mcb_t *a=(void *)arena,*b=(void *)(arena+176),*c=(void *)(arena+512);
    a->signature='M';a->owner=0x1000;a->size_paras=10;
    b->signature='M';b->owner=0;b->size_paras=20;
    c->signature='Z';c->owner=0;c->size_paras=67;
    int_frame_t f=invoke_oem(0xf900,0);
    CHECK(!frame_cf(&f) && f.eax==1552 && f.ebx==1392 && f.ecx==1072,
          "OEM arena summary excludes exactly three headers, reports total/free/largest bytes");
    c->signature='M';f=invoke_oem(0xf900,0);
    CHECK(frame_cf(&f) && (f.eax&65535)==7,"OEM arena summary refuses corrupt terminal");
    a->size_paras=65535;f=invoke_oem(0xf900,0);
    CHECK(frame_cf(&f) && (f.eax&65535)==7,"OEM arena summary refuses out-of-range block");
    int21_set_mcb_arena(0,0,0);f=invoke_oem(0xf900,0);
    CHECK(frame_cf(&f) && (f.eax&65535)==7,"OEM arena summary refuses missing arena");
    int21_set_console_page(screen);
    memset(&page,0,sizeof page);
    page.cells=alloc_low(4000);page.cols=80;page.rows=25;
    for(unsigned i=0;i<2000;i++){page.cells[2*i]='A'+i%26;page.cells[2*i+1]=0x17;}
    page.cur_row=7;page.cur_col=19;memset(buf,0xa5,4003);
    f=invoke_oem(0xfb00,buf);
    CHECK(!frame_cf(&f),"OEM save succeeds");
    int same=1;for(unsigned i=0;i<2000;i++)if(buf[2*i]!='A'+i%26 || buf[2*i+1]!=0x17)same=0;
    CHECK(same && buf[4000]==7 && buf[4001]==19 && buf[4002]==0xa5,"OEM save literal chars/attributes/cursor and trailing canary");
    /* No framebuffer: the character page remains observable independently. */
    buf[0]='Z';buf[1]=0x2e;buf[4000]=3;buf[4001]=11;
    f=invoke_oem(0xfb01,buf);
    CHECK(!frame_cf(&f) && page.cells[0]=='Z' && page.cells[1]==0x2e && page.cur_row==3 && page.cur_col==11,"OEM restore same payload");
    buf[2]='Q';buf[3]=0x47;buf[4000]=4;
    f=invoke_oem(0xfb02,buf);
    CHECK(!frame_cf(&f) && page.cells[2]=='Q' && page.cells[3]==0x47 && page.cur_row==4,"OEM update same payload");
    f=invoke_oem(0xfb03,buf);CHECK(frame_cf(&f) && (f.eax&65535)==1,"OEM bad screen operation");
    f=invoke_oem(0xfb00,0);CHECK(frame_cf(&f) && (f.eax&65535)==9,"OEM null screen buffer");
    f=invoke_oem(0xfb00,(void *)(uintptr_t)0xfffffff0u);CHECK(frame_cf(&f) && (f.eax&65535)==9,"OEM wrapped screen buffer");
    {
        for(unsigned mode=0;mode<3;mode++) {
            int21_set_restart(mode==0?0:mode==1?restart_returns:restart_exits);
            pid_t child=fork();int status;if(child<0)return 2;
            if(!child){invoke_oem(0xfa00,0);_exit(4);}
            waitpid(child,&status,0);
            CHECK(mode==2 ? WIFEXITED(status) && WEXITSTATUS(status)==0 : WIFSIGNALED(status) && WTERMSIG(status)==SIGSEGV,
                  "OEM restart transfers or stops forever, including absent/failed backend");
        }
        f=invoke_oem(0xfa01,0);CHECK(frame_cf(&f) && (f.eax&65535)==1,"OEM invalid restart operation refuses");
    }
    return TEST_SUMMARY("OEM-services");
}
