/* Real FAT backend through INT21. Sources: IBM DOS 3.30 TR pp.6-215,
 * 6-137..140 and 6-42; review-kernel.md reproducers. Reported success must
 * describe what the same handle can read; media errors cannot mean absence. */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "int21.h"
#include "fileio_fat.h"
#include "sft.h"
#include "psp.h"
#include "devices.h"
#include "blockdev_file.h"
#include "test_assert.h"
TEST_HARNESS();
static int (*mkdir_base_write)(void *,uint32_t,uint32_t,const void *);
static unsigned mkdir_write_count,mkdir_fail_at,mkdir_faulted,mkdir_after;
static int mkdir_fault_write(void *ctx,uint32_t lba,uint32_t count,const void *data)
{
    const uint8_t *bytes=data;
    for(unsigned i=0;i<count;i++) {
        unsigned hit=++mkdir_write_count==mkdir_fail_at;
        if(hit && !mkdir_after) { mkdir_faulted=1;return -1; }
        if(mkdir_base_write(ctx,lba+i,1,bytes+i*512)) return -1;
        if(hit) { mkdir_faulted=1;return -1; }
    }
    return 0;
}
static int_frame_t call(unsigned ax, unsigned bx, unsigned cx, uintptr_t dx)
{
    int_frame_t f={0};
    f.eax=ax; f.ebx=bx; f.ecx=cx; f.edx=(uint32_t)dx; f.eflags=0x202;
    int21_dispatch(&f);
    return f;
}
static int clock_fixture(uint16_t *year, uint8_t *mon, uint8_t *day,
                          uint8_t *hh, uint8_t *mi, uint8_t *ss, uint8_t *dow)
{
    *year=1992; *mon=3; *day=2; *hh=9; *mi=30; *ss=0; *dow=1;
    return 1;
}
int main(int argc, char **argv)
{
    blockdev_file_t disk;
    fat12_volume_t vol;
    uint8_t sector[512];
    psp_t psp;
    psp_params_t params={0};
    CHECK(argc>=4,"image mode fault [capture]");
    if(argc<4) return 2;
    unsigned fault=(unsigned)strtoul(argv[3],0,10);
    if(blockdev_file_open_rw(&disk,argv[1]) || fat12_mount(&vol,&disk.dev,sector) ||
       fileio_fat_bind(&vol)) return 2;
    params.alloc_end_linear=0x70000;
    if(psp_build(&psp,&params)) return 2;
    sft_init(); int21_set_psp(&psp);
    uint8_t *low=mmap(0,4096,PROT_READ|PROT_WRITE,
                      MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    if(low==MAP_FAILED) return 2;
    strcpy((char *)low,"TARGET.BIN");
    memset(low+512,'X',512);
    if(!strcmp(argv[2],"devices")) {
        FILE *capture=argc==5 ? fopen(argv[4],"rb") : NULL;
        if(!capture) return 2;
        devices_init();
        int21_set_devices(devices_head(),NULL);
        char line[128];
        unsigned cases=0;
        while(fgets(line,sizeof line,capture)) {
            line[strcspn(line,"\r\n")]=0;
            if(!strcmp(line,"DEVICES-DONE")) break;
            char *expected=strrchr(line,':');
            if(!expected) return 2;
            *expected++=0;
            strcpy((char *)low,line);
            int_frame_t f=call(0x3d02,0,0,(uintptr_t)low);
            const char *actual="ERROR";
            if(!(f.eflags&1)) {
                unsigned handle=f.eax&65535;
                f=call(0x4400,handle,0,0);
                actual=!(f.eflags&1) && (f.edx&0x80) ? "DEVICE" : "FILE";
                (void)call(0x3e00,handle,0,0);
            }
            printf("%s:%s (DOS 3.30 %s)\n",line,actual,expected);
            CHECK(!strcmp(actual,expected),"qualified device result matches genuine DOS 3.30");
            cases++;
        }
        CHECK(cases==8,"all captured qualified device cases ran");
        fclose(capture);
    } else if(!strcmp(argv[2],"mkdir-stamp") || !strcmp(argv[2],"mkdir-stamp-sub")) {
        int21_set_clock(clock_fixture,0);
        strcpy((char *)low,!strcmp(argv[2],"mkdir-stamp") ? "NEWDIR" : "SUB\\NEWDIR");
        mkdir_base_write=disk.dev.write_sectors;
        disk.dev.write_sectors=mkdir_fault_write;
        mkdir_fail_at=fault;
        mkdir_after=argc==5 && !strcmp(argv[4],"after");
        int_frame_t f=call(0x3900,0,0,(uintptr_t)low);
        printf("MKDIR_RESULT %s\n",f.eflags&1 ? "failed" : "success");
        printf("WRITE_COUNT %u\n",mkdir_write_count);
        CHECK(mkdir_faulted==(fault!=0),"requested physical MKDIR write fault fired");
    } else if(!strcmp(argv[2],"creat-stamp")) {
        int21_set_clock(clock_fixture,0);
        blockdev_file_arm_write_fault(&disk,fault);
        int_frame_t f=call(0x3c00,0,0,(uintptr_t)low);
        if(f.eflags&1) puts("CREATE_RESULT failed");
        else {
            sft_entry_t *entry=sft_from_handle(&psp,(uint8_t)f.eax);
            CHECK(entry && entry->dir_entry.mtime==0x4bc0 && entry->dir_entry.mdate==0x1862,
                  "successful CREAT publishes runtime DOS timestamp in the same entry");
            puts("CREATE_RESULT success");
        }
    } else if(!strcmp(argv[2],"new")) {
        blockdev_file_arm_read_fault(&disk,fault);
        int_frame_t f=call(0x5b00,0,0,(uintptr_t)low);
        CHECK(f.eflags&1,"CREATE_NEW never succeeds for an existing name, even on lookup error");
        CHECK((f.eax&65535)==(fault?31u:80u),"existence refusal differs from media failure");
        CHECK(blockdev_file_read_faulted(&disk)==(fault!=0),"requested lookup fault fired");
    } else if(!strcmp(argv[2],"readerr")) {
        int_frame_t f=call(0x3d02,0,0,(uintptr_t)low);
        CHECK(!(f.eflags&1),"open existing file");
        unsigned handle=f.eax&65535;
        blockdev_file_arm_read_fault(&disk,fault);
        f=call(0x3f00,handle,1,(uintptr_t)(low+1024));
        CHECK((f.eflags&1) && (f.eax&65535)!=2,"media failure cannot report file-not-found");
        CHECK(blockdev_file_read_faulted(&disk),"read fault fired");
    } else if(!strcmp(argv[2],"refresh")) {
        int_frame_t f=call(0x3d02,0,0,(uintptr_t)low);
        CHECK(!(f.eflags&1),"open empty file");
        unsigned handle=f.eax&65535;
        blockdev_file_arm_read_fault(&disk,fault);
        f=call(0x4000,handle,512,(uintptr_t)(low+512));
        CHECK(blockdev_file_read_faulted(&disk)==(fault!=0),"refresh fault fired");
        if(fault) CHECK(f.eflags&1,"WRITE reports failed metadata refresh");
        else {
            CHECK(!(f.eflags&1) && f.eax==512,"control acknowledges 512 bytes");
            (void)call(0x4200,handle,0,0);
            f=call(0x3f00,handle,512,(uintptr_t)(low+1024));
            CHECK(!(f.eflags&1) && f.eax==512 && !memcmp(low+512,low+1024,512),
                  "same handle reads the acknowledged bytes");
        }
    } else if(!strcmp(argv[2],"rootfull")) {
        unsigned char reference[8];
        FILE *capture=argc==5 ? fopen(argv[4],"rb") : NULL;
        if(!capture || fread(reference,1,sizeof reference,capture)!=sizeof reference)
            return 2;
        fclose(capture);
        unsigned i=0;
        for(unsigned op=0x3c00;op<=0x5b00;op+=0x1f00) {
            int_frame_t f=call(op,0,0,(uintptr_t)low);
            unsigned ax=reference[i]|reference[i+1]<<8;
            unsigned flags=reference[i+2]|reference[i+3]<<8;
            CHECK((f.eflags&1)==(flags&1) && (f.eax&65535)==ax,
                  "full-root error matches real DOS33 CREAT/CREATE_NEW capture");
            i+=4;
        }
    } else return 2;
    blockdev_file_close(&disk);
    return TEST_SUMMARY("metadata-errors");
}
