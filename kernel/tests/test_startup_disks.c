/* Draft for kernel/tests: accepted C: delivery layout, DOS drive/CWD rules,
 * real mtools volumes. Only ATA transport is replaced in this host program. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ata.h"
#include "block_device.h"
#include "fileio_fat.h"
#include "int21.h"
#include "psp.h"
#include "sft.h"
#include "blockdev_file.h"
#include "test_assert.h"
TEST_HARNESS();
extern const fat12_volume_t *startup_disks_init(void) __attribute__((weak));
extern int fileio_fat_system_drive(void) __attribute__((weak));
static blockdev_file_t drives[2];
static blockdev_t missing;
static int have_a;
static char path[128];
static uint8_t bytes[512];
static psp_t process;
static unsigned critical_calls, critical_drive;
void serial_puts(const char *s) { (void)s; }
void serial_putc(char c) { (void)c; }
void serial_puti(int32_t n) { (void)n; }
void serial_putu(uint32_t n) { (void)n; }
void __wrap_ata_ctx_init_primary_master(ata_ctx_t *ctx)
{ memset(ctx,0,sizeof *ctx);ctx->drive_select=ATA_DRIVE_MASTER_LBA; }
void __wrap_ata_ctx_init_primary_slave(ata_ctx_t *ctx)
{ memset(ctx,0,sizeof *ctx);ctx->drive_select=ATA_DRIVE_SLAVE_LBA; }
void __wrap_ata_blockdev_init(blockdev_t *dev,ata_ctx_t *ctx)
{ *dev=ctx->drive_select==ATA_DRIVE_MASTER_LBA ? drives[0].dev : have_a ? drives[1].dev : missing; }
static int absent_read(void *ctx,uint32_t lba,uint32_t n,void *p)
{ (void)ctx;(void)lba;(void)n;(void)p;return ATA_ERR_NO_DRIVE; }
static uint32_t sectors(blockdev_file_t *drive)
{
    if(fseek(drive->fp,0,SEEK_END))return 0;
    long n=ftell(drive->fp);
    return n>0 ? (uint32_t)(n/512) : 0;
}
static int_frame_t call(unsigned ax,unsigned bx,unsigned cx,unsigned dx)
{
    int_frame_t f={0};f.eax=ax;f.ebx=bx;f.ecx=cx;f.edx=dx;int21_dispatch(&f);return f;
}
static int open_file(const char *name)
{
    strcpy(path,name);int_frame_t f=call(0x3d00,0,0,(uintptr_t)path);
    CHECK(!(f.eflags&1),"selected system volume opens its root payload");
    return f.eflags&1 ? -1 : (int)(f.eax&65535);
}
static void read_file(const char *name,const char *expected)
{
    int h=open_file(name);if(h<0)return;
    int_frame_t f=call(0x3f00,(unsigned)h,sizeof bytes,(uintptr_t)bytes);
    CHECK(!(f.eflags&1) && f.eax==strlen(expected) && !memcmp(bytes,expected,strlen(expected)),
          "system selection returns independent mtools payload bytes");
    f=call(0x3e00,(unsigned)h,0,0);CHECK(!(f.eflags&1),"system file closes");
}
static int fail_critical(int write,int error)
{
    CHECK(!write && error<0,"critical handler sees the injected read error");
    critical_calls++;critical_drive=int21_io_drive();return CRIT_BLOCKDEV_FAIL;
}
int main(int argc,char **argv)
{
    if(argc!=4)return 2;
    CHECK(startup_disks_init && fileio_fat_system_drive,"production disk startup and stable system identity exist");
    if(!startup_disks_init || !fileio_fat_system_drive)return TEST_SUMMARY("startup-disks");
    have_a=strcmp(argv[2],"-")!=0;
    unsigned want=(unsigned)atoi(argv[3]);
    if(blockdev_file_open_rw(&drives[0],argv[1]))return 2;
    if(have_a && blockdev_file_open_rw(&drives[1],argv[2]))return 2;
    missing=(blockdev_t){0,absent_read,0};
    block_device_bind(0,&drives[0].dev,sectors(&drives[0]));
    block_device_bind(1,have_a ? &drives[1].dev : &missing,have_a ? sectors(&drives[1]) : 0);
    psp_params_t params={0};params.alloc_end_linear=0x80000;
    if(psp_build(&process,&params))return 2;
    sft_init();int21_set_psp(&process);
    const fat12_volume_t *system=startup_disks_init();
    CHECK(system!=0,"production startup binds a working volume");
    if(!system)return TEST_SUMMARY("startup-disks");
    CHECK(int21_current_drive()==want && int21_io_drive()==want && fileio_fat_system_drive()==(int)want,
          "default, I/O and stable system identity select the installed system");
    CHECK(system->base_lba==(want==2 ? 63u : 0u),"system mount has the independently minted origin");
    CHECK(fileio_fat_drive_volume(want,0,0)==system,"named mount matches the startup result");
    read_file("ROOT.BIN",want==2 ? "system C bytes\n" : "legacy A bytes\n");
    du_map map;
    CHECK(!block_device_map(want,&map) && (map.flags&1),"the actual system pair is marked running");
    if(want==2) {
        if(have_a) {
            CHECK(!block_device_map(0,&map) && map.flags==2,"ATA A is data, independently of running C");
            (void)call(0x0e00,0,0,0);
            CHECK(int21_current_drive()==0,"optional ATA data drive can be selected");
        } else {
            (void)call(0x0e00,0,0,0);
            CHECK(int21_current_drive()==2 && !fileio_fat_drive_volume(0,0,0),"absent A cannot replace default C");
        }
        CHECK(fileio_fat_system_drive()==2,"later DOS drive selection never changes system identity");
        read_file("C:ROOT.BIN","system C bytes\n");
        int h=open_file("C:ROOT.BIN");
        if(h<0)return TEST_SUMMARY("startup-disks");
        crit_blockdev_set_hook(fail_critical);
        blockdev_file_arm_read_fault(&drives[0],1);
        int_frame_t f=call(0x3f00,(unsigned)h,sizeof bytes,(uintptr_t)bytes);
        CHECK((f.eflags&1) && blockdev_file_read_faulted(&drives[0]) && critical_calls==1 && critical_drive==2,
              "working C retains the critical file-I/O path previously bound on A");
        blockdev_file_arm_read_fault(&drives[0],0);
        (void)call(0x3e00,(unsigned)h,0,0);
        critical_calls=0;blockdev_file_arm_read_fault(&drives[0],1);
        f=(int_frame_t){0};f.eax=2;f.ebx=(uintptr_t)bytes;f.ecx=1;int25_dispatch(&f);
        CHECK((f.eflags&1) && blockdev_file_read_faulted(&drives[0]) && !critical_calls,
              "raw INT25 on C reports its error without invoking the file-I/O critical handler");
        blockdev_file_arm_read_fault(&drives[0],0);crit_blockdev_set_hook(0);
    }
    CHECK(!drives[0].write_calls && (!have_a || !drives[1].write_calls),"startup and reads never write either disk");
    blockdev_file_close(&drives[0]);if(have_a)blockdev_file_close(&drives[1]);
    return TEST_SUMMARY("startup-disks");
}
