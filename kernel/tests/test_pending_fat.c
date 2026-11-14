/* Independent FAT/mtools properties for migrated mutations. Microsoft FAT
 * BPB/directory layout; IBM DOS3.30 TR pp5-11,6-139/140,6-206. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "fileio_fat.h"
#include "int21.h"
#include "psp.h"
#include "sft.h"
#include "blockdev_file.h"
#include "test_assert.h"
TEST_HARNESS();
static uint8_t sector[512], cluster[512], buffer[4096];
static char path[128];
static int_frame_t call(unsigned ax,unsigned bx,unsigned cx,void *ptr)
{ int_frame_t f={0};f.eax=ax;f.ebx=bx;f.ecx=cx;f.edx=(uintptr_t)ptr;int21_dispatch(&f);return f; }
static void cd(const char *s)
{ strcpy(path,s);int_frame_t f=call(0x3b00,0,0,path);CHECK(!(f.eflags&1),"CHDIR accepts independently minted path"); }
static void expect_file(const char *s,const char *value)
{
    strcpy(path,s);int_frame_t f=call(0x3d00,0,0,path);
    CHECK(!(f.eflags&1),"OPEN finds independently minted current-directory file");
    if(f.eflags&1)return;
    unsigned h=f.eax&65535;f=call(0x3f00,h,sizeof buffer,buffer);
    CHECK(!(f.eflags&1) && f.eax==strlen(value) && !memcmp(buffer,value,strlen(value)),
          "READ bytes identify the correct directory");
    f=call(0x3e00,h,0,0);CHECK(!(f.eflags&1),"CLOSE");
}
int main(int argc,char **argv)
{
    if(argc<3)return 2;
    blockdev_file_t disk;fat12_volume_t vol;
    if(blockdev_file_open_rw(&disk,argv[2]) || fat12_mount(&vol,&disk.dev,sector) || fileio_fat_bind(&vol))return 2;
    uint32_t len;uint8_t *fat=fileio_fat_fat_buffer(&len);
    psp_t process;psp_params_t params={0};params.alloc_end_linear=0x80000;
    if(psp_build(&process,&params))return 2;
    sft_init();int21_set_psp(&process);
    dir_entry_t e;uint32_t slot;
    const char *mode=argv[1];
    if(!strcmp(mode,"names")) {
        const char *bad[]={"BAD?.BIN","BAD*.BIN","BAD+.BIN","BAD;.BIN","BAD[.BIN","BAD].BIN","BAD|.BIN","BAD,.BIN","BAD=.BIN","BAD<.BIN","BAD>.BIN","BAD\".BIN"};
        for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++)
            CHECK(fat12_create(&vol,fat,len,bad[i],0x20,0,sector,cluster,&e,&slot)==FAT12_ERR_NOT_FOUND,
                  "strict FAT creator rejects forbidden punctuation before allocation");
    } else if(!strcmp(mode,"hide")) {
        int rc=fat12_hide_dir(&vol,fat,len,"SUB",0,sector);
        CHECK(rc==FAT12_OK,"hide existing directory succeeds");
        CHECK(fat12_find(&vol,sector,"SUB",&e)==FAT12_OK && (e.attribute&2),
              "hidden directory attribute persists on disk");
    } else if(!strcmp(mode,"rename")) {
        CHECK(fat12_rename(&vol,fat,len,"SUB","RENAMED",0,sector)==FAT12_OK,
              "same-parent directory rename is supported");
    } else if(!strcmp(mode,"cwd")) {
        cd("\\SUB");cd("DEEP");expect_file("DOC.BIN","deep bytes\n");
        cd("\\");expect_file("ROOT.BIN","root bytes\n");
    } else if(!strcmp(mode,"space")) {
        int_frame_t f=call(0x3600,0,0,0);
        /* Gate mints exactly root files2clusters + SUB and DEEP2clusters. */
        CHECK(f.eax==1 && f.ecx==512 && f.edx==2847 && f.ebx==2843,
              "free space equals independently minted four-cluster ownership");
    } else if(!strcmp(mode,"grow")) {
        if(fat12_find(&vol,sector,"TAIL",&e))return 2;
        uint16_t parent=e.start_cluster;
        CHECK(parent==2848,"mtools placed directory in last cluster");
        CHECK(fat12_create(&vol,fat,len,"NEW.BIN",0x20,parent,sector,cluster,&e,&slot)==FAT12_OK,
              "full directory grows into free cluster preceding its tail");
        uint16_t next=0;
        CHECK(fat12_next_cluster(&vol,fat,len,parent,&next)==FAT12_OK && next==2,
              "directory wrapped allocator reuses the only free cluster2");
        int_frame_t f=call(0x3c00,0,0,(void *)"TARGET.BIN");
        CHECK(!(f.eflags&1),"zero-length root entry fits full allocation map");
        unsigned h=f.eax&65535;buffer[0]='X';f=call(0x4000,h,1,buffer);
        CHECK(!(f.eflags&1) && f.eax==0,"disk-full WRITE reports zero-byte short success per DOS contract");
        (void)call(0x3e00,h,0,0);
    } else if(!strcmp(mode,"labels")) {
        char label[12];
        if(disk.dev.read_sectors(disk.dev.ctx,0,1,sector))return 2;
        memcpy(sector+43,"BOOT LABEL ",11);
        if(disk.dev.write_sectors(disk.dev.ctx,0,1,sector))return 2;
        CHECK(fileio_fat_volume_label(label)==0 && !strcmp(label,"ROOT LABEL"),
              "directory label takes precedence over deliberately different BPB label");
        /* Independent raw reader removes only the label entry. */
        unsigned root=(sector[14]|sector[15]<<8)+sector[16]*(sector[22]|sector[23]<<8);
        int found=0;
        for(unsigned sec=0;sec<14 && !found;sec++) {
            if(disk.dev.read_sectors(disk.dev.ctx,root+sec,1,sector))return 2;
            for(unsigned i=0;i<512;i+=32)if(sector[i]!=0xe5 && sector[i] && sector[i+11]==8) {
                sector[i]=0xe5;found=1;
                if(disk.dev.write_sectors(disk.dev.ctx,root+sec,1,sector))return 2;
                break;
            }
        }
        if(!found)return 2;
        CHECK(fileio_fat_volume_label(label)==0 && !strcmp(label,"BOOT LABEL"),
              "absent directory label falls back to current disk BPB");
        if(disk.dev.read_sectors(disk.dev.ctx,0,1,sector))return 2;
        memcpy(sector+43,"LIVE LABEL ",11);
        if(disk.dev.write_sectors(disk.dev.ctx,0,1,sector))return 2;
        CHECK(fileio_fat_volume_label(label)==0 && !strcmp(label,"LIVE LABEL"),
              "label read observes external BPB write after mount");
    } else if(!strcmp(mode,"fat16")) {
        if(argc!=4)return 2;
        FILE *reference=fopen(argv[3],"rb");if(!reference)return 2;
        int_frame_t f=call(0x3d00,0,0,(void *)"CHAIN.BIN");
        CHECK(!(f.eflags&1),"FAT16 OPEN");unsigned h=f.eax&65535,total=0;
        for(;;) {
            uint8_t expected[4096];size_t n=fread(expected,1,sizeof expected,reference);
            f=call(0x3f00,h,sizeof buffer,buffer);
            CHECK(!(f.eflags&1) && f.eax==n && !memcmp(buffer,expected,n),
                  "windowed FAT16 bytes equal independent mtools fixture at every chunk");
            total+=n;if(!n)break;
        }
        CHECK(total>20000,"FAT16 read crosses FAT-sector boundaries and corruption threshold");
        fclose(reference);(void)call(0x3e00,h,0,0);
    } else return 2;
    blockdev_file_close(&disk);
    return TEST_SUMMARY("pending-fat");
}
