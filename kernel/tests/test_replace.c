/* Ruling 1 / initech-7mb66: named bytes survive every precommit error.
 * Physical writes are faulted before AND after persistence. FAT layout and
 * ownership are independently decoded by mtools, fsck.fat and the gate reader. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "int21.h"
#include "fileio_fat.h"
#include "blockdev_file.h"
static blockdev_file_t disk;
static unsigned calls, fail_at, after, fired;
static int fault_write(void *ctx, uint32_t lba, uint32_t n, const void *p)
{
    for (unsigned i=0;i<n;i++) {
        int hit=++calls==fail_at;
        if ((!hit || after) && disk.dev.write_sectors(ctx,lba+i,1,(const char *)p+i*512)) return -1;
        if(hit) { fired++; return -1; }
    }
    return 0;
}
static void need(int ok,const char *msg) { if(!ok) { fprintf(stderr,"FAIL replace: %s\n",msg);exit(1); } }
int main(int argc,char **argv)
{
    blockdev_t dev;
    fat12_volume_t vol;
    unsigned char sector[512];
    uint16_t cleanup=0;
    need(argc==7,"image stage dest fail-at after expected");
    fail_at=strtoul(argv[4],0,10); after=strtoul(argv[5],0,10);
    need(!blockdev_file_open_rw(&disk,argv[1]),"open");
    dev=disk.dev;dev.write_sectors=fault_write;
    need(!fat12_mount(&vol,&dev,sector) && !fileio_fat_bind(&vol),"mount and bind");
    /* Same geometry on a separately selected drive still must refuse. */
    need(!fileio_fat_add_drive(2,&vol),"bind C control");
    uint16_t rc=int21_file_replace(argv[2],argv[3],&cleanup);
    need(rc==(unsigned)strtoul(argv[6],0,10),"operation status");
    need(!fail_at || fired==1,"write fault reached");
    need(rc==0 || cleanup==0,"no cleanup before commit");
    need(fail_at || rc==0 || calls==0,"refusal issues no writes");
    /* Refresh live cache independently from the disk, without hiding mismatch. */
    unsigned char actual[6144];uint32_t len;
    void *cache=fileio_fat_fat_buffer(&len);
    need(!fat12_read_fat(&vol,actual,sizeof actual),"read disk FAT");
    need(len && !memcmp(cache,actual,len),"live FAT equals disk");
    blockdev_file_close(&disk);
    printf("PASS replace: writes=%u rc=%u cleanup=%u\n",calls,rc,cleanup);
    return 0;
}
