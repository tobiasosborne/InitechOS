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
#include "mcb.h"
#include "blockdev_file.h"
#include "test_assert.h"
TEST_HARNESS();
static uint32_t vectors[256];
idt_gate_t idt_get_gate(uint8_t vec) { idt_gate_t g={0};g.offset_lo=vectors[vec];g.offset_hi=vectors[vec]>>16;return g; }
void idt_install_trap(uint8_t vec,void *handler) { vectors[vec]=(uintptr_t)handler; }
void panic_show_reason(panic_reason_t reason,const char *detail) { (void)reason;(void)detail;abort(); }
static const char *mode;
static unsigned guest_calls;
static uintptr_t parent_control;
static uint8_t saved_controls[0x1110], saved_high[0x1ef000];
static uint16_t want_directory;
static uint8_t dos_parent[128], dos_child[128], shell_env[4096], parent_env[4096];
static size_t dos_parent_len, dos_child_len;
static uint8_t *parent_env_addr;
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

/* Independent consumer, as in turbo/tests/ide/runtime/raw-psp.c. Read the
 * published AH=62h PSP bytes, then compare against a real DOS capture. */
static uint8_t *guest_environment(void)
{
    int_frame_t f = {0};
    f.eax = 0x6200;
    int21_dispatch(&f);
    uintptr_t psp = (f.ebx & 0xffffu) << 4;
    if (psp < 0x3f000u || psp > 0x8ff00u) return NULL;
    const uint8_t *raw = (const uint8_t *)psp;
    uintptr_t env = ((uintptr_t)raw[0x2c] | ((uintptr_t)raw[0x2d] << 8)) << 4;
    if (env < 0x3f000u || env > 0x8f000u) return NULL;
    return (uint8_t *)env;
}

static uint16_t exec_from_test_disk(const char *name, uint16_t directory,
                                    const char *tail, uint32_t tail_len,
                                    uint32_t env, uint8_t *rc)
{
    uint32_t fat_len;
    const uint8_t *fat = fileio_fat_fat_buffer(&fat_len);
    loader_bind_fat_volume(fileio_fat_current_volume(), fat, fat_len);
    return load_program_from_fat(name, directory, tail, tail_len, env, rc) == LOADER_OK
           ? 0u : INT21_ERR_BAD_FORMAT;
}

static void test_environment(void)
{
    uint8_t *env = guest_environment();
    if (guest_calls == 1) {
        parent_env_addr = env;
        CHECK(env && !memcmp(env, dos_parent, dos_parent_len),
              "parent PSP publishes the complete DOS 3.30 environment capture");
        if (env) memcpy(parent_env, env, sizeof parent_env);
        static const char *paths[] = {"sub\\dump.com", "c:\\sub\\..\\sub\\dump.com"};
        for (unsigned i = 0; i < sizeof paths / sizeof paths[0]; i++) {
            int_frame_t f = {0};
            f.eax = 0x4b00;
            f.edx = (uintptr_t)paths[i];
            int21_dispatch(&f);
            CHECK(!(f.eflags & 1u) && guest_calls == i + 2u,
                  "AH=4Bh transfers to the child on the resolved drive");
            CHECK(env && !memcmp(env, parent_env, sizeof parent_env),
                  "nested EXEC and a write to the child's environment preserve every parent byte");
        }
    } else {
        size_t drive_offset = 50u; /* first byte after the captured 01 00 word */
        uint8_t drive = guest_calls == 2 ? 'A' : 'C';
        CHECK(env && !memcmp(env, dos_child, drive_offset) && env[drive_offset] == drive &&
              !memcmp(env + drive_offset + 1u, dos_child + drive_offset + 1u,
                      dos_child_len - drive_offset - 1u),
              "child inherits only the variables and publishes its own DOS 3.30 executable path");
        CHECK(env && env != parent_env_addr, "child has a private environment block");
        if (env) env[0] = 'X';
    }
}

static void guest(void)
{
    guest_calls++;
    psp_t *psp=int21_get_psp();
    CHECK(psp!=NULL,"executing child has a real PSP");
    if (!strcmp(mode, "environment")) test_environment();
    if(!strcmp(mode,"low-high") && guest_calls==1) {
        memcpy(saved_high,(void *)0x40000,0x10110);
        uint32_t total_before,largest_before,total_after,largest_after;
        CHECK(int21_mcb_stats(&total_before,&largest_before),"conventional caller has its own live chain");
        uint8_t child_rc=0;
        loader_status_t child=load_program_from_fat("HIGH.EXE",0,NULL,0,0,&child_rc);
        CHECK(child==LOADER_OK && child_rc==7 && guest_calls==2,"extended child executes under a conventional caller");
        CHECK(int21_get_psp()==psp && g_load.depth==1 && !g_high_live &&
              !memcmp(saved_high,(void *)0x40000,0x10110),"extended child preserves complete conventional PSP/image/BSS");
        CHECK(int21_mcb_stats(&total_after,&largest_after) && total_after==total_before && largest_after==largest_before,
              "extended child preserves conventional caller's entire allocation capacity");
    }
    if(!strcmp(mode,"high-canonical") && guest_calls==1) {
        parent_control=(uintptr_t)psp;
        CHECK(parent_control==0x8ef00u,"extended controls are outside flat image, BSS and fixed stack windows");
        mcb_t *owner=(mcb_t *)(parent_control-16u);
        CHECK(owner->signature=='Z' && owner->owner==0x8ef0 && owner->size_paras==0x110,
              "reserved controls have a real terminal MCB owned by their parent");
        memcpy(saved_controls,(void *)(parent_control-16u),sizeof saved_controls);
        memcpy(saved_high,(void *)0x600000,sizeof saved_high);
        uint32_t total_before,largest_before,total_after,largest_after;
        CHECK(int21_mcb_stats(&total_before,&largest_before),"parent arena is valid before nested fixed child");
        uint8_t child_rc=0;
        loader_status_t child=load_program_from_fat("RAW.COM",0,NULL,0,0,&child_rc);
        CHECK(child==LOADER_OK && child_rc==7 && guest_calls==2,
              "fixed-base child actually executes under extended parent and returns exit7");
        CHECK(int21_get_psp()==psp && g_load.depth==1 && g_high_live,
              "extended parent regains its PSP and active high context");
        CHECK(!memcmp(saved_controls,(void *)(parent_control-16u),sizeof saved_controls),
              "child preserves complete parent PSP/environment and owning MCB");
        CHECK(!memcmp(saved_high,(void *)0x600000,sizeof saved_high),
              "child preserves entire extended image, reserve and heap (excluding active stack)");
        CHECK(int21_mcb_stats(&total_after,&largest_after) && total_after==total_before && largest_after==largest_before,
              "nested fixed child returns all its conventional allocation");
    } else if(!strcmp(mode,"high-canonical")) {
        uintptr_t sp;__asm__ volatile("movl %%esp,%0":"=r"(sp));
        CHECK((uintptr_t)psp==0x40000u,"nested fixed child sees canonical PSP40000 and image40100");
        CHECK(sp>0x40110u && sp<0x6f000u,"initial child stack is inside its own conventional allocation");
        /* Native flat CRTs zero BSS through6f000 and reset ESP to7fffc.
         * Sources: turbo/compiler/NATIVE.md, seed/rt/start_dos.asm. */
        *(uint32_t *)(uintptr_t)0x6effc=0x89abcdefu;
        *(uint32_t *)(uintptr_t)0x7fff0=0x76543210u;
    }
    if(!strncmp(mode,"nested-",7) && guest_calls==1) {
        /* Occupy the lowest available block so refusal/PIC tests continue to
         * exercise a DIFFERENT address after canonical IDE placement works. */
        uint32_t held=0,held_end=0;
        CHECK(int21_mcb_exec_alloc(0,&held,&held_end)==0 && int21_mcb_exec_trim(held,held+0x1100u)==0,
              "fixture owns the lower image region before a relocated child");
        *(uint32_t *)(uintptr_t)held=0x2468ace0u;
        psp_t saved=*psp;
        uint8_t child_rc=0;
        uint32_t total_before,largest_before,total_after,largest_after;
        CHECK(int21_mcb_stats(&total_before,&largest_before),"parent owns a valid arena");
        *(uint32_t *)(uintptr_t)(EXT_APP_BASE+4096u)=0x13579bdfu;
        if(!strcmp(mode,"nested-pic")) {
            loader_status_t child=load_program_from_fat("PIC.COM",0,NULL,0,0,&child_rc);
            CHECK(child==LOADER_OK && child_rc==7 && guest_calls==2,
                  "explicit PIC child transfers at a different base and returns");
        } else {
            unsigned count=!strcmp(mode,"nested-invalid")?16:1;
            for(unsigned i=0;i<count;i++) {
                char name[16];snprintf(name,sizeof name,"BAD%u.COM",i);
                loader_status_t child=load_program_from_fat(count==1?"RAW.COM":name,0,NULL,0,0,&child_rc);
                CHECK(child==LOADER_ERR_BAD_FORMAT && guest_calls==1,
                      "undeclared or damaged PIC declaration refuses before transfer");
            }
        }
        CHECK(int21_get_psp()==psp && !memcmp(psp,&saved,sizeof saved) && g_load.depth==1,
              "child EXEC preserves complete parent PSP and active EXEC frame");
        CHECK(*(uint32_t *)(uintptr_t)(EXT_APP_BASE+4096u)==0x13579bdfu && g_high_live,
              "child EXEC preserves extended parent's owned data and residency");
        CHECK(int21_mcb_stats(&total_after,&largest_after) && total_after==total_before && largest_after==largest_before,
              "child completion or refusal restores parent's free arena without a leak");
        CHECK(*(uint32_t *)(uintptr_t)held==0x2468ace0u,"child preserves the occupied lower allocation");
        int21_mcb_exec_free(held);
    }
    if (!strcmp(mode,"cwd")) {
        int21_cwd_snapshot_t cwd=int21_cwd_save();
        CHECK(cwd.start_cluster==want_directory,"actual child inherits caller CWD");
        static const char empty_env[] = "\0\0\x01\0A:\\SUB\\ONLY.COM";
        uint8_t *env = guest_environment();
        CHECK(env && !memcmp(env, empty_env, sizeof empty_env),
              "an empty environment is followed by the count and the CWD-relative executable path");
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
    uint8_t pic[32]={0xeb,0x0e,'I','N','I','T','E','C','H','P',1,16,1,0,0,0};
    memcpy(pic+16,code,sizeof code);save(dir,"PIC.COM",pic,sizeof pic);
    for(unsigned i=0;i<16;i++) {
        char name[16];snprintf(name,sizeof name,"BAD%u.COM",i);
        pic[i]^=1;save(dir,name,pic,sizeof pic);pic[i]^=1;
    }
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

static size_t read_capture(const char *path, uint8_t *buf, size_t capacity)
{
    FILE *f = fopen(path, "rb");
    if (!f) exit(2);
    size_t n = fread(buf, 1, capacity, f);
    if (ferror(f) || !feof(f) || fclose(f) || n <= 51u) exit(2);
    return n;
}

int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    if(!strcmp(argv[1],"fixtures")) { fixtures(argv[2]);return 0; }
    mode=argv[1];
    void *low_map=mmap((void *)0x3f000,0x51000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0);
    void *low=(void *)0x40000;
    void *high=mmap((void *)0x600000,0x200000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0);
    if(low_map==MAP_FAILED || high==MAP_FAILED)return 2;
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
    } else if (!strcmp(mode, "environment")) {
        dos_parent_len = read_capture("kernel/tests/fixtures/dos33_env/PARENT.BIN",
                                      dos_parent, sizeof dos_parent);
        dos_child_len = read_capture("kernel/tests/fixtures/dos33_env/CHILD.BIN",
                                     dos_child, sizeof dos_child);
        if (fileio_fat_add_drive(2u, &vol)) return 2;
        int21_set_exec_backend(exec_from_test_disk);
        memset(shell_env, 0xa5, sizeof shell_env);
        memcpy(shell_env, dos_parent, 48u); /* captured variables and both NULs */
        memcpy((void *)(uintptr_t)ENV_BLOCK, shell_env, sizeof shell_env);
        st = load_program_from_fat("parent.com", 0, NULL, 0, ENV_BLOCK, &rc);
        CHECK(st == LOADER_OK && rc == 7 && guest_calls == 3,
              "parent and both INT21 children complete");
        CHECK(!memcmp((void *)(uintptr_t)ENV_BLOCK, shell_env, sizeof shell_env),
              "EXEC preserves the shell's entire environment source block");
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
            name=!strcmp(mode,"resident-flat")?"RAW.COM":!strcmp(mode,"resident-high")?"HIGH.EXE":"MZ.EXE";
            st=load_program_from_fat(name,0,NULL,0,0,&rc);
            CHECK(!memcmp(before,low,sizeof before),"EXEC preserves entire retained PSP/image range");
            if(!strcmp(mode,"resident-flat")) {
                CHECK(st==LOADER_ERR_FIXED_BASE && guest_calls==0,"raw flat image refuses while resident owns fixed base");
            } else CHECK(st==LOADER_OK && rc==7 && guest_calls==1,"relocated child returns above resident");
        } else if(!strcmp(mode,"high-owner")) {
            memset(high,0x5a,0x200000);memset((void *)0x8eef0,0xa5,0x1110);g_high_live=1;
            st=load_program_from_fat("HIGH.EXE",0,NULL,0,0,&rc);
            CHECK(st==LOADER_ERR_NOMEM && guest_calls==0,"second high image refuses occupied owner window");
            int intact=1;for(unsigned i=0;i<0x200000;i++)if(((uint8_t *)high)[i]!=0x5a)intact=0;
            CHECK(intact,"refused high EXEC preserves all existing owner bytes");
            intact=1;for(unsigned i=0;i<0x1110;i++)if(*(uint8_t *)(uintptr_t)(0x8eef0+i)!=0xa5)intact=0;
            CHECK(intact,"refused high EXEC preserves complete existing low controls");
        } else if(!strncmp(mode,"high-reclaim",12) || !strncmp(mode,"nested-",7) || !strcmp(mode,"high-canonical")) {
            int failed=!strcmp(mode,"high-reclaim-failure");
            if(failed)memset((void *)0x8eef0,0xa5,0x1110);
            st=load_program_from_fat("HIGH.EXE",0,NULL,0,failed?0x1234:0,&rc);
            if(failed) CHECK(st==LOADER_ERR_BAD_ENV && guest_calls==0 && g_load.depth==0,
                            "failed pre-transfer preparation reclaims both owned windows");
            else CHECK(st==LOADER_OK && rc==7 && guest_calls==(!strcmp(mode,"nested-pic") || !strcmp(mode,"high-canonical")?2u:1u),
                       "real high parent returns without an undeclared transfer");
            CHECK(g_high_live==0,"returned high child releases single owner");
            int zero=1;for(unsigned i=0;i<0x200000;i++)if(((uint8_t *)high)[i])zero=0;
            CHECK(zero,"returned high child clears complete high window");
            zero=1;for(unsigned i=0;i<0x1110;i++)if(*(uint8_t *)(uintptr_t)(0x8eef0+i))zero=0;
            CHECK(zero,"successful or failed high EXEC clears its entire separate low control window");
        } else if(sigsetjmp(recovery,1)) {
            CHECK(0,"in-place EXEC must never copy from its NULL image source");
        } else {
            active=1;st=load_program_from_fat(name,directory,NULL,0,0,&rc);active=0;
            CHECK(st==LOADER_OK && rc==7 && guest_calls==(!strcmp(mode,"low-high")?2u:1u),
                  "disk child transfers and returns with literal exit7");
        }
    }
    blockdev_file_close(&disk);
    return TEST_SUMMARY("loader-live");
}
