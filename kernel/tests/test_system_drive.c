/* Delivery design: C: is the fixed system volume, A: optional ATA data.
 * IBM DOS 3.30 TR AH=0Eh/19h and per-drive CWD; mtools owns fixture bytes.
 * These resident bindings must not confuse the system and frame volumes.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "fileio_fat.h"
#include "int21.h"
#include "psp.h"
#include "sft.h"
#include "blockdev_file.h"
#include "test_assert.h"
TEST_HARNESS();
/* Weak only in this host regression: absence produces the initial red verdict. */
extern int fileio_fat_bind_drive(unsigned, fat12_volume_t *) __attribute__((weak));
extern const fat12_volume_t *fileio_fat_drive_volume(unsigned, void **, uint32_t *) __attribute__((weak));
static uint8_t sector[512], data[128], raw_fat[6144];
static char path[128];
static psp_t process;

static int_frame_t call(unsigned ax, unsigned bx, unsigned cx, unsigned dx)
{
    int_frame_t f={0}; f.eax=ax; f.ebx=bx; f.ecx=cx; f.edx=dx;
    int21_dispatch(&f); return f;
}
static int path_call(unsigned ax, const char *name)
{
    strcpy(path,name);
    int_frame_t f=call(ax,0,0,(uintptr_t)path);
    return (f.eflags&1) ? -1 : ax==0x3d00 ? (int)(f.eax&65535) : 0;
}
static void read_file(const char *name,const char *expected)
{
    int h=path_call(0x3d00,name);
    CHECK(h>=0,"OPEN finds file on the selected volume");
    if(h<0)return;
    int_frame_t f=call(0x3f00,(unsigned)h,sizeof data,(uintptr_t)data);
    CHECK(!(f.eflags&1) && f.eax==strlen(expected) && !memcmp(data,expected,strlen(expected)),
          "READ returns independent bytes from the correct volume");
    f=call(0x3e00,(unsigned)h,0,0);
    CHECK(!(f.eflags&1),"CLOSE selected file");
}
static unsigned le16(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8; }
static int independent_fat(const char *image,unsigned base,void *cache,uint32_t bytes)
{
    FILE *fp=fopen(image,"rb"); uint8_t bpb[512];
    if(!fp)return -1;
    int rc=-1;
    if(!fseek(fp,(long)base*512,SEEK_SET) && fread(bpb,1,512,fp)==512) {
        unsigned reserved=le16(bpb+14), length=le16(bpb+22)*512;
        if(length<=sizeof raw_fat && !fseek(fp,(long)(base+reserved)*512,SEEK_SET) &&
           fread(raw_fat,1,length,fp)==length)
            rc=cache && bytes==length && !memcmp(cache,raw_fat,length);
    }
    if(fclose(fp))return -1;
    return rc;
}
static void query_keeps_state(unsigned letter,const fat12_volume_t *expected)
{
    int21_cwd_snapshot_t before=int21_cwd_save();
    const fat12_volume_t *current=fileio_fat_current_volume();
    void *cache=(void *)(uintptr_t)1; uint32_t bytes=123;
    CHECK(fileio_fat_drive_volume(letter,&cache,&bytes)==expected,"explicit query returns the requested mount");
    if(!expected)CHECK(!cache && !bytes,"absent mount clears optional cache outputs");
    int21_cwd_snapshot_t after=int21_cwd_save();
    CHECK(before.current_drive==after.current_drive && before.io_drive==after.io_drive &&
          !memcmp(before.starts,after.starts,sizeof before.starts) &&
          !memcmp(before.paths,after.paths,sizeof before.paths) &&
          current==fileio_fat_current_volume(),"volume query preserves default, I/O volume and every CWD");
}
int main(int argc,char **argv)
{
    if(argc!=4)return 2;
    CHECK(fileio_fat_bind_drive && fileio_fat_drive_volume,"explicit initial-drive and volume-query bindings exist");
    if(!fileio_fat_bind_drive || !fileio_fat_drive_volume)return TEST_SUMMARY("system-drive");
    blockdev_file_t disks[3]; fat12_volume_t volumes[3];
    for(unsigned i=0;i<3;i++) {
        if(blockdev_file_open_rw(&disks[i],argv[i+1]) || fat12_mount(&volumes[i],&disks[i].dev,sector))return 2;
    }
    psp_params_t params={0}; params.alloc_end_linear=0x80000;
    if(psp_build(&process,&params))return 2;
    sft_init(); int21_set_psp(&process);
    CHECK(fileio_fat_bind_drive(2,&volumes[0])==0,"C binds before any A volume exists");
    if(g_fails)return TEST_SUMMARY("system-drive");
    CHECK(fileio_fat_current_volume()==&volumes[0],"initial backend volume is C");
    /* SYSINIT selects the DOS default after installing the file binding. */
    (void)call(0x0e00,0,0,2);
    CHECK(int21_current_drive()==2 && int21_io_drive()==2 && fileio_fat_current_volume()==&volumes[0],
          "initial default and I/O volume are C");
    read_file("ROOT.BIN","system C bytes\n");
    query_keeps_state(0,0); query_keeps_state(1,0); query_keeps_state(4,0);
    CHECK(fileio_fat_drive_volume(2,0,0)==&volumes[0],"query allows omitted cache outputs");
    void *cfat=0,*afat=0; uint32_t clen=0,alen=0;
    (void)fileio_fat_drive_volume(2,&cfat,&clen);
    int match=independent_fat(argv[1],63,cfat,clen); if(match<0)return 2;
    CHECK(match,"C cache equals raw mtools primary FAT at partition base63");
    CHECK(fileio_fat_add_drive(0,&volumes[1])==0,"optional A can be added after C");
    (void)fileio_fat_drive_volume(0,&afat,&alen);
    match=independent_fat(argv[2],0,afat,alen); if(match<0)return 2;
    CHECK(match && afat!=cfat,"A owns a separate matching primary FAT cache");
    CHECK(path_call(0x3b00,"C:\\SYS")==0,"C has its own current directory");
    CHECK(path_call(0x3b00,"A:\\USER")==0,"A has its own current directory");
    (void)call(0x0e00,0,0,0);
    CHECK(int21_current_drive()==0 && int21_io_drive()==0,"DOS can select optional A");
    query_keeps_state(2,&volumes[0]);
    read_file("DOC.BIN","data directory bytes\n");
    read_file("C:DOC.BIN","system directory bytes\n");
    CHECK(fileio_fat_add_drive(3,&volumes[2])==0,"windowed FAT16 mount can be added independently");
    void *dfat=(void *)(uintptr_t)1; uint32_t dlen=123;
    CHECK(fileio_fat_drive_volume(3,&dfat,&dlen)==&volumes[2] && !dfat && !dlen,
          "FAT16 query has no stale whole-FAT cache");
    query_keeps_state(3,&volumes[2]);
    CHECK(fileio_fat_bind_drive(1,&volumes[0])<0,"B cannot be an initial mounted drive");
    CHECK(fileio_fat_bind_drive(4,&volumes[0])<0,"unassigned letter cannot be initial drive");
    CHECK(fileio_fat_bind_drive(2,0)<0,"null volume cannot become the system drive");
    query_keeps_state(0,&volumes[1]); query_keeps_state(2,&volumes[0]);
    CHECK(fileio_fat_bind(&volumes[1])==0,"legacy A binding remains supported");
    CHECK(int21_current_drive()==0 && int21_io_drive()==0,"legacy binding selects A");
    read_file("ROOT.BIN","data A bytes\n"); query_keeps_state(2,0); query_keeps_state(3,0);
    for(unsigned i=0;i<3;i++) {
        CHECK(disks[i].write_calls==0,"binding, selection, queries and reads never write the disk");
        blockdev_file_close(&disks[i]);
    }
    return TEST_SUMMARY("system-drive");
}
