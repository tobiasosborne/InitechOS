/* Genuine raw AH=3Ch/3Dh DOS 3.30 records; no COMMAND operand parser.
 * See fixtures/raw-name33/README.md. FAT/mtools readback is outside this tool. */
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
#include "blockdev_file.h"
#include "test_assert.h"
TEST_HARNESS();
static int_frame_t call(unsigned ax,unsigned bx,unsigned cx,uintptr_t dx)
{
    int_frame_t f={0};
    f.eax=ax;f.ebx=bx;f.ecx=cx;f.edx=(uint32_t)dx;f.eflags=0x202;
    int21_dispatch(&f);return f;
}
static unsigned word(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8; }
static void native_call(int_frame_t r,const uint8_t **reference)
{
    CHECK((r.eflags&1)==(word(*reference+2)&1),"namespace operation carry matches genuine DOS");
    *reference+=4;
}
static void namespace_operations(uint8_t *low,const uint8_t *reference)
{
    const uint8_t *next=reference;
    strcpy((char *)low,"\\DIRECTORYLONG");
    strcpy((char *)low+128,"ORIGINALNAME.LONG");
    strcpy((char *)low+256,"RENAMEDLONG.EXTLONG");
    strcpy((char *)low+384,"\\");
    int_frame_t r=call(0x3900,0,0,(uintptr_t)low);native_call(r,&next);
    r=call(0x3b00,0,0,(uintptr_t)low);native_call(r,&next);
    r=call(0x3c00,0,0,(uintptr_t)(low+128));native_call(r,&next);
    unsigned handle=r.eax&65535;
    r=call(0x4000,handle,7,(uintptr_t)(low+512));native_call(r,&next);
    CHECK(r.eax==word(reference+12),"namespace WRITE count matches native");
    r=call(0x3e00,handle,0,0);native_call(r,&next);
    r=call(0x4300,0,0,(uintptr_t)(low+128));native_call(r,&next);
    CHECK((r.ecx&32)!=0,"long-spelling attribute query finds the file");
    r=(int_frame_t){0};r.eax=0x5600;r.edx=(uint32_t)(uintptr_t)(low+128);
    r.edi=(uint32_t)(uintptr_t)(low+256);r.eflags=0x202;
    int21_dispatch(&r);native_call(r,&next);
    r=call(0x3d00,0,0,(uintptr_t)(low+256));native_call(r,&next);
    handle=r.eax&65535;
    r=call(0x3f00,handle,7,(uintptr_t)(low+1024));native_call(r,&next);
    CHECK(r.eax==word(reference+32) && !memcmp(low+1024,reference+52,7),
          "renamed long-spelling READ count/payload matches native");
    r=call(0x3e00,handle,0,0);native_call(r,&next);
    r=call(0x4100,0,0,(uintptr_t)(low+256));native_call(r,&next);
    r=call(0x3b00,0,0,(uintptr_t)(low+384));native_call(r,&next);
    r=call(0x3a00,0,0,(uintptr_t)low);native_call(r,&next);
}
int main(int argc,char **argv)
{
    if(argc!=5) return 2;
    unsigned index=(unsigned)strtoul(argv[4],0,10);
    if(index>20) return 2;
    FILE *f=fopen(argv[2],"rb"); uint8_t reference[59];
    unsigned size=index==20 ? 59u : 23u;
    if(!f || fseek(f,index==20 ? 0 : (long)index*23,SEEK_SET) ||
       fread(reference,1,size,f)!=size) return 2;
    fclose(f);
    blockdev_file_t disk;fat12_volume_t vol;uint8_t sector[512];
    psp_t psp;psp_params_t params={0};params.alloc_end_linear=0x70000;
    if(blockdev_file_open_rw(&disk,argv[1]) || fat12_mount(&vol,&disk.dev,sector) ||
       fileio_fat_bind(&vol) || psp_build(&psp,&params)) return 2;
    sft_init();int21_set_psp(&psp);
    uint8_t *low=mmap(0,4096,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    if(low==MAP_FAILED || strlen(argv[3])>=128) return 2;
    strcpy((char *)low,argv[3]);memcpy(low+512,"raw API",7);
    if(index==20) { namespace_operations(low,reference); goto done; }
    int_frame_t r=call(0x3c00,0,0,(uintptr_t)low);
    printf("case %u %s: CF=%u AX=%u (native CF=%u AX=%u)\n",index,argv[3],
        r.eflags&1,r.eax&65535,word(reference+2)&1,word(reference));
    CHECK((r.eflags&1)==(word(reference+2)&1),"CREAT carry matches genuine DOS");
    if(word(reference+2)&1) {
        CHECK((r.eax&65535)==word(reference),"CREAT refusal code matches genuine DOS");
    } else if(!(r.eflags&1)) {
        unsigned handle=r.eax&65535;
        r=call(0x4000,handle,7,(uintptr_t)(low+512));
        CHECK((r.eflags&1)==(word(reference+6)&1) && (r.eax&65535)==word(reference+4),
              "WRITE count/carry matches genuine DOS");
        r=call(0x3e00,handle,0,0);CHECK(!(r.eflags&1),"close created file");
        r=call(0x3d00,0,0,(uintptr_t)low);
        CHECK((r.eflags&1)==(word(reference+10)&1),"OPEN original long spelling matches genuine DOS");
        if(!(r.eflags&1)) {
            handle=r.eax&65535;
            r=call(0x3f00,handle,7,(uintptr_t)(low+1024));
            CHECK((r.eflags&1)==(word(reference+14)&1) &&
                  (r.eax&65535)==word(reference+12) && !memcmp(low+1024,reference+16,7),
                  "READ count/carry/payload matches genuine DOS");
            r=call(0x3e00,handle,0,0);CHECK(!(r.eflags&1),"close reopened file");
            r=call(0x5b00,0,0,(uintptr_t)low);
            CHECK((r.eflags&1) && (r.eax&65535)==80,"CREATE_NEW refuses existing canonical alias");
        }
    }
done:
    blockdev_file_close(&disk);munmap(low,4096);
    return TEST_SUMMARY("raw-names33");
}
