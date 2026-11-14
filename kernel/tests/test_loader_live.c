/* Product loader, including its real stack transfer, in an isolated 32-bit
 * host process. Only privileged IDT/port hardware is supplied by this harness.
 * Independent properties: resident ranges are conserved; freed high memory is
 * zero/unowned; loaded children see caller CWD and relocated own-image pointers.
 * Sources: IBM DOS3.30 TR EXEC/KEEP contracts; kernel README memory layout. */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/mman.h>
#define INITECH_IO_H
static inline uint8_t inb(uint16_t port) { (void)port;return 0x20; }
static inline void outb(uint16_t port,uint8_t value) { (void)port;(void)value; }
#include "loader.c"
#include "fileio_fat.h"
#include "blockdev_file.h"
#include "test_assert.h"
TEST_HARNESS();
static uint32_t vectors[256];
idt_gate_t idt_get_gate(uint8_t vec) { idt_gate_t g={0};g.offset_lo=vectors[vec];g.offset_hi=vectors[vec]>>16;return g; }
void idt_install_trap(uint8_t vec,void *handler) { vectors[vec]=(uintptr_t)handler; }
void panic_show_reason(panic_reason_t reason,const char *detail) { (void)reason;(void)detail;abort(); }
static const char *mode;
static unsigned guest_calls;
static uint16_t want_directory;
static sigjmp_buf recovery;
static volatile sig_atomic_t active;
static void fault(int sig, siginfo_t *info, void *ctx)
{
    (void)ctx;
    /* Only this exact runtime illegal NULL source copy is an assertion. A
     * startup/unrelated fault remains an ERROR, not a mutation kill. */
    if(active && sig==SIGSEGV && !info->si_addr) {
        active=0;siglongjmp(recovery,1);
    }
    _Exit(2);
}
static void guest(void)
{
    guest_calls++;
    psp_t *psp=int21_get_psp();
    CHECK(psp!=NULL,"executing child has a real PSP");
    if (!strcmp(mode,"cwd")) {
        int21_cwd_snapshot_t cwd=int21_cwd_save();
        CHECK(cwd.start_cluster==want_directory,"actual child inherits caller CWD");
    }
    if (!strcmp(mode,"resident-reloc")) {
        uint32_t base=(uintptr_t)psp+0x100u;
        CHECK(*(uint32_t *)(uintptr_t)(base+8)==base+0x10u,
              "actual resident-adjacent child pointer relocates to its own module");
    }
    int_frame_t f={0};f.eax=0x4c07;int21_dispatch(&f);
    abort();
}
static void put16(uint8_t *p,unsigned off,unsigned n) { p[off]=n;p[off+1]=n>>8; }
static void put32(uint8_t *p,unsigned off,uint32_t n) { for(unsigned i=0;i<4;i++)p[off+i]=n>>(i*8); }
static void save(const char *dir,const char *name,const void *data,unsigned n)
{
    char path[512];snprintf(path,sizeof path,"%s/%s",dir,name);
    FILE *f=fopen(path,"wb");if(!f || fwrite(data,1,n,f)!=n || fclose(f))exit(2);
}
static void fixtures(const char *dir)
{
    uint8_t code[16]={0xb8}, mz[68]={0}, high[80]={0};
    put32(code,1,(uintptr_t)guest);code[5]=0xff;code[6]=0xe0;put32(code,8,0x10);
    save(dir,"RAW.COM",code,sizeof code);
    put16(mz,0,0x5a4d);put16(mz,2,68);put16(mz,4,1);put16(mz,6,1);
    put16(mz,8,3);put16(mz,0x18,64);put16(mz,0x1c,MZ_INITECH_TAG);
    memcpy(mz+48,code,16);put32(mz,64,8);
    save(dir,"MZ.EXE",mz,sizeof mz);
    put16(high,0,0x5a4d);put16(high,2,80);put16(high,4,1);put16(high,8,4);
    put16(high,0x0c,0xffff);put16(high,0x18,56);put16(high,0x1c,0x5849);
    put32(high,32,1);put32(high,36,24);put32(high,40,65536);
    put32(high,44,32);put32(high,48,65536);put32(high,52,4096);
    memcpy(high+64,code,16);save(dir,"HIGH.EXE",high,sizeof high);
}
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    if(!strcmp(argv[1],"fixtures")) { fixtures(argv[2]);return 0; }
    mode=argv[1];
    void *low=mmap((void *)0x40000,0x50000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0);
    void *high=mmap((void *)0x600000,0x200000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0);
    if(low==MAP_FAILED || high==MAP_FAILED)return 2;
    sft_init();
    blockdev_file_t disk;fat12_volume_t vol;uint8_t sector[512];
    if(blockdev_file_open(&disk,argv[2]) || fat12_mount(&vol,&disk.dev,sector) || fileio_fat_bind(&vol))return 2;
    uint32_t len;const uint8_t *fat=fileio_fat_fat_buffer(&len);
    loader_bind_fat_volume(&vol,fat,len);
    struct sigaction action={0};action.sa_sigaction=fault;action.sa_flags=SA_SIGINFO;
    sigemptyset(&action.sa_mask);sigaction(SIGSEGV,&action,0);
    uint8_t rc=0;loader_status_t st;
    if(!strcmp(mode,"arena")) {
        uint8_t code[7]={0xb8};put32(code,1,(uintptr_t)guest);code[5]=0xff;code[6]=0xe0;
        loader_plan_t plan;
        st=loader_prepare(code,sizeof code,NULL,0,&plan);
        CHECK(st==LOADER_OK && plan.arena_present,"small program has heap");
        CHECK(plan.arena_base>=0x40100u+sizeof code+PROGRAM_BSS_RESERVE,
              "heap begins beyond complete running image and BSS");
    } else if(!strcmp(mode,"high-plan")) {
        mz_image_t img={0};loader_plan_t plan;
        img.load_module_len=16;img.max_alloc_paras=0xffff;
        st=loader_high_plan(&img,NULL,0,&plan);
        CHECK(st==LOADER_OK && plan.image_dst==0x600000,"extended image uses high address contract");
    } else {
        static uint16_t directory;
        static const char *name="RAW.COM";
        if(!strcmp(mode,"subdir") || !strcmp(mode,"cwd")) {
            dir_entry_t e;if(fat12_find(&vol,sector,"SUB",&e))return 2;
            want_directory=directory=e.start_cluster;
            if(!strcmp(mode,"cwd")) {
                int21_cwd_snapshot_t cwd={0};cwd.start_cluster=directory;
                int21_cwd_restore(&cwd);
            }
            name="ONLY.COM";
        }
        if(!strncmp(mode,"resident-",9)) {
            psp_t *resident=(psp_t *)(uintptr_t)0x40000;
            psp_params_t params={0};params.alloc_end_linear=0x80000;
            if(psp_build(resident,&params))return 2;
            int21_set_psp(resident);int21_set_mcb_arena(NULL,0,0);
            int_frame_t f={0};f.eax=0x3100;f.edx=32;int21_dispatch(&f);
            int21_set_psp(NULL);
            CHECK(int21_tsr_top()==0x40200,"resident fixture retains 32 paragraphs");
            uint8_t before[512];memcpy(before,low,sizeof before);
            name=!strcmp(mode,"resident-flat")?"RAW.COM":"MZ.EXE";
            st=load_program_from_fat(name,0,NULL,0,0,&rc);
            CHECK(!memcmp(before,low,sizeof before),"EXEC preserves entire retained PSP/image range");
            if(!strcmp(mode,"resident-flat")) {
                CHECK(st==LOADER_ERR_FIXED_BASE && guest_calls==0,"raw flat image refuses while resident owns fixed base");
            } else CHECK(st==LOADER_OK && rc==7 && guest_calls==1,"relocated child returns above resident");
        } else if(!strcmp(mode,"high-owner")) {
            memset(high,0x5a,0x200000);g_high_live=1;
            st=load_program_from_fat("HIGH.EXE",0,NULL,0,0,&rc);
            CHECK(st==LOADER_ERR_NOMEM && guest_calls==0,"second high image refuses occupied owner window");
            int intact=1;for(unsigned i=0;i<0x200000;i++)if(((uint8_t *)high)[i]!=0x5a)intact=0;
            CHECK(intact,"refused high EXEC preserves all existing owner bytes");
        } else if(!strcmp(mode,"high-reclaim")) {
            st=load_program_from_fat("HIGH.EXE",0,NULL,0,0,&rc);
            CHECK(st==LOADER_OK && rc==7 && guest_calls==1,"real high child executes and returns");
            CHECK(g_high_live==0,"returned high child releases single owner");
            int zero=1;for(unsigned i=0;i<0x200000;i++)if(((uint8_t *)high)[i])zero=0;
            CHECK(zero,"returned high child clears complete high window");
        } else if(sigsetjmp(recovery,1)) {
            CHECK(0,"in-place EXEC must never copy from its NULL image source");
        } else {
            active=1;st=load_program_from_fat(name,directory,NULL,0,0,&rc);active=0;
            CHECK(st==LOADER_OK && rc==7 && guest_calls==1,"disk child transfers and returns with literal exit7");
        }
    }
    blockdev_file_close(&disk);
    return TEST_SUMMARY("loader-live");
}
