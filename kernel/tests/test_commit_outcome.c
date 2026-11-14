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
static char other[128];
static uint8_t poison[700]; /* native DOS frame pointers must remain below 4 GiB */
static int_frame_t dos(unsigned ax,unsigned bx,unsigned cx,void *p)
{
    int_frame_t f={0};f.eax=ax;f.ebx=bx;f.ecx=cx;f.edx=(uintptr_t)p;
    int21_dispatch(&f);return f;
}
static int snapshot(const char *path)
{
    char copy[1024];unsigned char buf[16384];size_t n;
    if(snprintf(copy,sizeof copy,"%s.before",path)>=(int)sizeof copy)return -1;
    FILE *in=fopen(path,"rb"),*out=fopen(copy,"wb");
    if(!in||!out){if(in)fclose(in);if(out)fclose(out);return -1;}
    int failed=0;
    while((n=fread(buf,1,sizeof buf,in)))if(fwrite(buf,1,n,out)!=n){failed=1;break;}
    if(ferror(in))failed=1;
    if(fclose(in))failed=1;
    if(fclose(out))failed=1;
    return failed?-1:0;
}
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
    if(argc!=7 && argc!=8)return 2;
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
        if(argc==8) {
            const char *action=argv[7];memset(poison,'Z',sizeof poison);
            if(snapshot(argv[1]))return 2;
            before=calls;
            if(!strcmp(action,"create")) {
                f=dos(0x3c00,0,0,staged);
                CHECK((f.eflags&1) && (f.eax&65535)==5 && calls==before,
                      "CREATE retained alias refuses before freeing adopted allocation");
                /* Keep the reviewer's complete CREATE/write/read damage path. */
                if(!(f.eflags&1)) {
                    unsigned h=f.eax&65535;
                    (void)dos(0x4000,h,100,poison);(void)dos(0x3e00,h,0,0);
                }
            } else if(!strcmp(action,"truncate") || !strcmp(action,"write") ||
                      !strcmp(action,"grow")) {
                f=dos(0x3d02,0,0,staged);unsigned h=f.eax&65535;
                CHECK(!(f.eflags&1),"retained alias remains readable/openable");
                unsigned count=!strcmp(action,"truncate")?0:!strcmp(action,"write")?1:700;
                f=dos(0x4000,h,count,poison);
                CHECK((f.eflags&1) && (f.eax&65535)==5 && calls==before,
                      "alias truncate/overwrite/extension refuses before any sector write");
                f=dos(0x4201,h,0,0);
                CHECK(!(f.eflags&1) && f.eax==0 && (f.edx&65535)==0,
                      "refused alias write leaves handle position unchanged");
                f=dos(0x3f00,h,39,poison);
                CHECK(!(f.eflags&1) && f.eax==39 &&
                      !memcmp(poison,"independently minted replacement bytes\n",39),
                      "refused mutation preserves the cached allocation and readable bytes");
                (void)dos(0x3e00,h,0,0);
            } else if(!strcmp(action,"move") || !strcmp(action,"whole") ||
                      !strcmp(action,"whole-zero") || !strcmp(action,"resize")) {
                uint32_t fat_len,slot;void *fat=fileio_fat_fat_buffer(&fat_len);
                dir_entry_t sub,entry;uint8_t cluster[512];
                if(fat12_find(&vol,sector,"SUB",&sub))return 2;
                uint16_t parent=strcmp(argv[3],"root")?sub.start_cluster:0;
                if(fat12_find_slot_in(&vol,fat,fat_len,parent,sector,"STAGE.BIN",&entry,&slot))return 2;
                int error;
                if(!strcmp(action,"move"))
                    error=fat12_move_dirent(&vol,fat,fat_len,"STAGE.BIN",parent,
                              "MOVED.BIN",parent?0:sub.start_cluster,sector,cluster);
                else if(!strcmp(action,"resize"))
                    error=fat12_set_size(&vol,fat,fat_len,parent,slot,1,sector,cluster);
                else error=fat12_write_file(&vol,fat,fat_len,slot,poison,
                                  !strcmp(action,"whole")?1:0,sector,cluster);
                CHECK(error==FAT12_ERR_ACCESS && calls==before,
                      "direct mutation cannot change or move an adopted allocation alias");
            } else if(!strcmp(action,"replace-source") || !strcmp(action,"replace-dest")) {
                uint16_t warning;
                snprintf(other,sizeof other,"%sOTHER.BIN",strcmp(argv[3],"root")?"SUB\\":"");
                uint16_t error=!strcmp(action,"replace-source")?
                    int21_file_replace(staged,other,&warning):int21_file_replace(other,dest,&warning);
                CHECK(error==5 && calls==before,
                      "another replacement cannot adopt or reclaim a shared chain");
            } else return 2;
        }
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
