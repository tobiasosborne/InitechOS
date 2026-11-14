/* initech-8qgdz / i7zfp: a successful physical publication is monotonic.
 * Expected status is from the operator's commit rule, bytes from mtools. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "int21.h"
#include "fileio_fat.h"
#include "blockdev_file.h"
#include "psp.h"
#include "sft.h"
#include "test_assert.h"
TEST_HARNESS();
static blockdev_file_t disk;
static unsigned calls, after, cleanup_fault, faults, read_faults, reads_failed;
static char staged[128],dest[128];
static int write_fault(void *ctx,uint32_t l,uint32_t n,const void *p)
{
    calls++;
    if(calls==1) {
        if(after && disk.dev.write_sectors(ctx,l,n,p))return -1;
        faults++;return -1;
    }
    if((cleanup_fault==1 && calls==2) || (cleanup_fault==2 && calls>=2)) {
        faults++;return -1;
    }
    return disk.dev.write_sectors(ctx,l,n,p);
}
static int read_fault(void *ctx,uint32_t l,uint32_t n,void *p)
{
    if(after && calls && read_faults){read_faults--;reads_failed++;return -1;}
    return disk.dev.read_sectors(ctx,l,n,p);
}
int main(int argc,char **argv)
{
    if(argc!=7)return 2;
    after=atoi(argv[4]);cleanup_fault=atoi(argv[5]);read_faults=atoi(argv[6]);
    int rename=!strcmp(argv[2],"rename");
    snprintf(staged,sizeof staged,"%sSTAGE.BIN",strcmp(argv[3],"root")?"SUB\\":"");
    snprintf(dest,sizeof dest,"%sDEST.BIN",strcmp(argv[3],"root")?"SUB\\":"");
    blockdev_t dev;fat12_volume_t vol;uint8_t sector[512];
    if(blockdev_file_open_rw(&disk,argv[1]))return 2;
    dev=disk.dev;dev.write_sectors=write_fault;dev.read_sectors=read_fault;
    if(fat12_mount(&vol,&dev,sector) || fileio_fat_bind(&vol))return 2;
    psp_t p;psp_params_t params={0};params.alloc_end_linear=0x80000;
    if(psp_build(&p,&params))return 2;
    sft_init();int21_set_psp(&p);
    uint16_t status,cleanup=0;
    if(rename) {
        int_frame_t f={0};f.eax=0x5600;f.edx=(uintptr_t)staged;f.edi=(uintptr_t)dest;
        int21_dispatch(&f);status=(f.eflags&1)?f.eax&65535:0;
    } else status=int21_file_replace(staged,dest,&cleanup);
    CHECK(status==(after?0:rename?5:31),"physical publication determines success; unpersisted error is a refusal");
    CHECK(faults>=1,"commit error reached");
    if(after && cleanup_fault && !rename)CHECK(faults>=2,"postcommit cleanup fault reached");
    CHECK(read_faults==0,"reconciliation consumed requested transient read failures");
    if(after && cleanup_fault==2 && !rename) {
        /* A retained staging alias is no longer an allocation owner. Even a
         * later cleanup must not free the destination's adopted chain. */
        cleanup_fault=0;
        unsigned before=calls;
        int_frame_t f={0};f.eax=0x4100;f.edx=(uintptr_t)staged;int21_dispatch(&f);
        CHECK((f.eflags&1) && (f.eax&65535)==5 && calls==before,
              "late stage cleanup cannot free the adopted destination chain");
    }
    if(status) {
        /* The actual cleanup used by SDK/FILE COPY after a failure. It must
         * never free a chain already adopted by the destination. */
        cleanup_fault=0;read_faults=0;
        int_frame_t f={0};f.eax=0x4100;f.edx=(uintptr_t)staged;int21_dispatch(&f);
    }
    blockdev_file_close(&disk);
    printf("commit-outcome mode=%s after=%u cleanup=%u status=%u warning=%u writes=%u faults=%u read-faults=%u\n",
           argv[2],after,cleanup_fault,status,cleanup,calls,faults,reads_failed);
    return TEST_SUMMARY("commit-outcome");
}
