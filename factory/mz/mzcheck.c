/* Independent reader of the legacy InitechMZ contract (kernel/mz.c and
 * kernel/loader.c). Deliberately shares no writer or SDK constants.
 * No kernel/attic source is compiled here. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
static uint32_t u16(const uint8_t *b) { return b[0]|(uint32_t)b[1]<<8; }
static uint32_t u32(const uint8_t *b) { return u16(b)|u16(b+2)<<16; }
static void fail(const char *s) { fprintf(stderr,"mzcheck: %s\n",s);exit(1); }
int main(int argc,char **argv)
{
    if(argc!=2&&argc!=4)fail("usage: mzcheck EXE [raw-module bss-bytes]");
    FILE *f=fopen(argv[1],"rb");if(!f)fail("open");
    if(fseek(f,0,SEEK_END))fail("seek");
    long length=ftell(f);if(length<32||length>INT32_MAX)fail("file size");
    rewind(f);uint32_t size=(uint32_t)length;uint8_t *b=malloc(size);if(!b)fail("memory");
    if(fread(b,1,size,f)!=size||fclose(f))fail("read");
    if(b[0]!='M'||b[1]!='Z'||u16(b+28)!=0x4943)fail("header/tag");
    uint32_t last=u16(b+2),pages=u16(b+4),n=u16(b+6),hdr=u16(b+8)*16;
    if(!pages||last>511)fail("page encoding");
    uint32_t end=(pages-1)*512+(last?last:512),table=u16(b+24),bss=u16(b+10)*16;
    if(hdr<32||hdr>end||end>size||end==hdr)fail("module extent");
    uint32_t image=end-hdr;
    if(u16(b+20)||u16(b+22))fail("entry must be module offset 0");
    if(n&&(table<32||table>size||n>(size-table)/4))fail("relocation table extent");
    if(n&&!((table+n*4<=hdr)||(table>=end)))fail("table overlaps module");
    for(uint32_t i=0;i<n;i++) {
        uint32_t off=u32(b+table+4*i);
        if(image<4||off>image-4)fail("relocation site outside module");
        if(u32(b+hdr+off)>=image+bss)fail("relocated pointer outside image+BSS");
        for(uint32_t j=0;j<i;j++){uint32_t prev=u32(b+table+4*j);if((off>prev?off-prev:prev-off)<4)fail("overlapping relocation sites");}
    }
    if(argc==4) {
        char *tail;unsigned long wanted=strtoul(argv[3],&tail,0);
        if(*tail||wanted>UINT32_MAX||bss!=((wanted+15)/16)*16)fail("BSS recording");
        FILE *raw=fopen(argv[2],"rb");if(!raw)fail("raw module open");
        for(uint32_t i=0;i<image;i++)if(fgetc(raw)!=b[hdr+i])fail("module changed");
        if(fgetc(raw)!=EOF||ferror(raw)||fclose(raw))fail("module length");
    }
    printf("MZ valid: file=%u header=%u image=%u entry=0 relocations=%u BSS=%u\n",size,hdr,image,n,bss);
    free(b);return 0;
}
