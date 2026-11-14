/* GM2 read-only policy; IBM BIOS TR AH=03h status03; Microsoft FAT layout.
 * Expected refusals and complete-image identity come from the operator policy,
 * not from a second implementation of the guard. mtools mints every volume. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "absdisk_binding.h"
#include "blockdev_file.h"
#include "psp.h"
#include "sft.h"
#include "test_assert.h"
TEST_HARNESS();
fat12_volume_t g_absdisk_vol;
int int21_absdisk_read(uint32_t l,uint32_t n,void *p)
{ return fat12_read_sectors(&g_absdisk_vol,l,n,p); }
int int21_absdisk_write(uint32_t l,uint32_t n,const void *p)
{ return fat12_write_sectors(&g_absdisk_vol,l,n,p); }
static uint8_t buf[2048],sector[512];
static char path[]="CHAIN.BIN",other[]="OTHER.BIN";
static int_frame_t dos(unsigned ax,unsigned bx,unsigned cx,void *p)
{ int_frame_t f={0};f.eax=ax;f.ebx=bx;f.ecx=cx;f.edx=(uintptr_t)p;int21_dispatch(&f);return f; }
static int_frame_t physical(unsigned disk,uint32_t l,unsigned n,int write)
{
    int_frame_t f={0};unsigned c=l/(16*63),h=l/63%16,s=l%63+1;
    f.eax=(write?0x0300:0x0200)|n;f.ebx=(uintptr_t)buf;
    f.ecx=(c&255)<<8|((c>>2)&192)|s;f.edx=h<<8|disk;
    int13_dispatch(&f);return f;
}
int main(int argc,char **argv)
{
    if(argc!=4)return 2;
    blockdev_file_t disk;fat12_volume_t vol;int ro=atoi(argv[2]);unsigned letter=atoi(argv[3]);
    if(blockdev_file_open_rw(&disk,argv[1]) || fat12_mount(&vol,&disk.dev,sector) || fileio_fat_bind(&vol))return 2;
    CHECK(vol.fat_type==(ro?FAT_TYPE_FAT16:FAT_TYPE_FAT12),"mtools fixture FAT classification");
    unsigned drive=letter?0x80:0x81;
    block_device_bind(drive-0x80,&disk.dev,vol.base_lba+vol.total_sectors+8);
    if(!letter)bind_absdisk_volume(&vol,&disk.dev);
    else block_device_volume_policy(letter,drive,vol.base_lba,vol.total_sectors,ro);
    psp_t p;psp_params_t params={0};params.alloc_end_linear=0x80000;
    if(psp_build(&p,&params))return 2;
    sft_init();int21_set_psp(&p);
    int_frame_t f=dos(0x3d00,0,0,path);unsigned handle=f.eax&65535;
    CHECK(!(f.eflags&1),"FAT volume opens existing file");
    f=dos(0x3f00,handle,sizeof buf,buf);
    CHECK(!(f.eflags&1) && f.eax==24 && !memcmp(buf,"mtools independent data\n",24),"FAT volume reads independent bytes");
    (void)dos(0x3e00,handle,0,0);
    dir_entry_t e;CHECK(fat12_find(&vol,sector,path,&e)==0,"FAT volume lists existing name");
    if(ro) {
        unsigned before=disk.write_calls;
        const unsigned ops[]={0x3c00,0x5b00,0x4100,0x3900,0x3a00,0x4301};
        for(unsigned i=0;i<sizeof ops/sizeof ops[0];i++) {
            f=dos(ops[i],0,0,ops[i]==0x5b00 || ops[i]==0x3900 ? other : ops[i]==0x3a00 ? (void *)"SUB" : path);
            CHECK((f.eflags&1) && (f.eax&65535)==5,"FAT16 file mutation access denied");
        }
        f=dos(0x3d02,0,0,path);handle=f.eax&65535;
        if(!(f.eflags&1)) {
            f=dos(0x4000,handle,1,buf);CHECK((f.eflags&1) && (f.eax&65535)==5,"FAT16 handle write access denied");
            f=dos(0x5701,handle,0,0);CHECK((f.eflags&1) && (f.eax&65535)==5,"FAT16 timestamp access denied");
            (void)dos(0x3e00,handle,0,0);
        }
        f=(int_frame_t){0};f.eax=0x5600;f.edx=(uintptr_t)path;f.edi=(uintptr_t)other;int21_dispatch(&f);
        CHECK((f.eflags&1) && (f.eax&65535)==5,"FAT16 rename access denied");
        if(!letter) { f=(int_frame_t){0};f.ebx=(uintptr_t)buf;f.ecx=1;int26_dispatch(&f);
            CHECK((f.eflags&1) && (f.eax&65535)==0x0a00,"FAT16 INT26 write protected"); }
        CHECK(fat12_write_sectors(&vol,0,1,buf)!=0,"FAT16 shared sector writer refuses");
        CHECK(disk.write_calls==before,"all file and volume refusals precede backend writes");
    }
    memset(buf,0xa5,sizeof buf);
    const uint32_t starts[]={vol.base_lba,vol.base_lba+vol.total_sectors-1,vol.base_lba-1,vol.base_lba+vol.total_sectors-1};
    for(unsigned i=0;i<4;i++) {
        unsigned before=disk.write_calls;
        f=physical(drive,starts[i],i>=2?2:1,1);
        CHECK(ro ? (f.eflags&1) && ((f.eax>>8)&255)==3 && disk.write_calls==before
                 : !(f.eflags&1) && disk.write_calls==before+1,"INT13 whole request refuses FAT16 overlap; FAT12 writes");
    }
    f=physical(drive,vol.base_lba,1,0);CHECK(!(f.eflags&1),"INT13 read remains available");
    /* Adjacent sectors and the other physical disk remain writable; separate
     * invocation keeps the refused-image cmp independent of these controls. */
    blockdev_file_close(&disk);
    return TEST_SUMMARY("fat16-policy");
}
