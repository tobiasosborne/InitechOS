/* AH=40h CX=0 oracle. IBM DOS 3.30 Technical Reference p.6-140;
 * ADR-0003 Appendix A/B.1. Expected bytes come from mcopy, not FAT helpers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blockdev_file.h"
#include "fat12.h"
#include "int21.h"
#include "fileio_fat.h"
#include "psp.h"
#include "sft.h"
static unsigned char sector[512], cluster[512], fat[6144], saved[6144];
static unsigned char buf[2200];
static psp_t psp;
static void need(int ok, const char *why)
{ if (!ok) { fprintf(stderr,"RED truncate: %s\n",why); exit(1); } }
static int call(unsigned a,unsigned b,unsigned c,unsigned d)
{
    int_frame_t f={0}; f.eax=a; f.ebx=b; f.ecx=c; f.edx=d;
    int21_dispatch(&f); return f.eflags&1 ? -(int)(f.eax&65535):(int)f.eax;
}
static unsigned ptr(const void *p) { return (unsigned)(uintptr_t)p; }
static void trim(const char *name,unsigned size,int regrow)
{
    strcpy((char *)buf,name);
    int h=call(0x3d02,0,0,ptr(buf)); need(h>=0,"RDWR open");
    need(call(0x4200,h,0,size)==(int)size,"seek before truncate");
    need(call(0x4000,h,0,0)==0,"zero-count write");
    need(call(0x4201,h,0,0)==(int)size,"position preserved");
    need(call(0x4202,h,0,0)==(int)size,"AH40 size at current position");
    if(regrow) {
        memset(buf,'R',1000);
        need(call(0x4000,h,1000,ptr(buf))==1000,"regrow write");
        need(call(0x4202,h,0,0)==1600,"regrow size");
    }
    need(call(0x3e00,h,0,0)>=0,"close");
}
int main(int argc,char **argv)
{
    if(argc==2 && !strcmp(argv[1],"--body")) {
        for(unsigned i=0;i<sizeof buf;i++)putchar('a'+i%26);
        return 0;
    }
    if(argc<3)return 2;
    blockdev_file_t bf; fat12_volume_t v;
    need(!blockdev_file_open_rw(&bf,argv[1]),"open image");
    need(!fat12_mount(&v,&bf.dev,sector),"mount");
    unsigned fl=v.bpb.sectors_per_fat*512u;
    need(!fat12_read_fat(&v,fat,sizeof fat),"read FAT");
    if(!strcmp(argv[2],"matrix")) {
        psp_params_t p={0};p.alloc_end_linear=0x80000;
        psp_build(&psp,&p);sft_init();int21_set_psp(&psp);
        need(!fileio_fat_bind(&v),"bind");
        trim("SHRINK.BIN",600,0); trim("BOUND.BIN",512,0);
        trim("EMPTY.BIN",0,0); trim("GROW.BIN",600,1);
        trim("SUB\\CHILD.BIN",600,0);
        trim("EXTEND.BIN",3000,0); trim("SHRINK.BIN",600,0);
        strcpy((char *)buf,"RONLY.BIN");
        need(call(0x3d02,0,0,ptr(buf))==-5,"read-only file refused");
        strcpy((char *)buf,"GUARD.BIN");
        int h=call(0x3d00,0,0,ptr(buf));need(h>=0,"read open");
        need(call(0x4000,h,0,0)==-5,"read handle refused");
        call(0x3e00,h,0,0);
        need(call(0x4000,255,0,0)==-6,"bad handle refused");
    } else {
        dir_entry_t de; unsigned slot,wrote=99;
        need(!fat12_find_slot_in(&v,fat,fl,0,sector,"SHRINK.BIN",&de,&slot),"find");
        if(!strcmp(argv[2],"fault")) {
            if(argc!=4)return 2;
            memcpy(saved,fat,fl);
            blockdev_file_arm_write_fault(&bf,(unsigned)atoi(argv[3]));
            int rc=fat12_write_partial(&v,fat,fl,0,slot,600,NULL,0,sector,cluster,&wrote);
            need(blockdev_file_write_faulted(&bf),"fault actually fired");
            blockdev_file_arm_write_fault(&bf,0);
            need(rc==FAT12_ERR_WRITE && wrote==0,"fault returned loud");
            need(!memcmp(saved,fat,fl),"fault restored live FAT");
        } else if(!strcmp(argv[2],"corrupt")) {
            if(argc!=4)return 2;
            unsigned c=(unsigned)atoi(argv[3]);
            unsigned short val=c==0?0xfff:c==1?de.start_cluster:c==2?0:0xff7;
            need(!fat12_set_entry(fat,fl,de.start_cluster,val),"corrupt fixture");
            memcpy(saved,fat,fl);
            need(fat12_write_partial(&v,fat,fl,0,slot,600,NULL,0,sector,cluster,&wrote)==FAT12_ERR_CHAIN,
                 "inconsistent chain refused");
            need(!memcmp(saved,fat,fl),"corrupt chain left untouched");
        } else return 2;
    }
    blockdev_file_close(&bf);
    puts("GREEN truncate operations");return 0;
}
