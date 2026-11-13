/* Independent reader of the InitechMZ contract. Authority: the descriptor
 * specification in kernel/include/initechmz_extended.h and the documented
 * high-window accounting in kernel/loader_validation.c:loader_high_plan.
 * Deliberately shares no writer helpers, format headers or loader code.
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
    uint32_t tag=u16(b+28);
    if(!((b[0]=='M'&&b[1]=='Z')||(b[0]=='Z'&&b[1]=='M'))||
       (tag!=0x4943&&tag!=0x5849))fail("header/tag");
    int extended=tag==0x5849;
    uint32_t last=u16(b+2),pages=u16(b+4),n=u16(b+6),hdr=u16(b+8)*16;
    if(!pages||last>511)fail("page encoding");
    uint32_t end=(pages-1)*512+(last?last:512),table=u16(b+24),bss=u16(b+10)*16;
    if(hdr<32||hdr>end||end>size||end==hdr)fail("module extent");
    uint32_t image=end-hdr;
    uint32_t reserve=bss,heap=0,available=0;
    if(extended) {
        if(hdr<56)fail("extended descriptor extent");
        if(u32(b+32)!=1)fail("extended version");
        if(u32(b+36)!=24)fail("extended descriptor length");
        if(u32(b+48)!=65536)fail("extended stack");
        if(u32(b+52)!=4096)fail("extended guard");
        if(table<56||table>hdr||n>(hdr-table)/4)fail("extended relocation table extent");
        if(u16(b+22)*16+u16(b+20)>=image)fail("extended entry outside module");
        if(image>262144)fail("extended module maximum");
        reserve=u32(b+40);heap=u32(b+44);
        /* Window [600000,7ef000), followed by 4096 guard and 65536 stack.
         * Round occupied bytes up, cap heap by maxalloc, charge its 16-byte
         * native block header, then round the usable minimum heap up. */
        uint64_t occupied=((uint64_t)image+reserve+15)/16*16;
        if(occupied>0x1ef000)fail("extended reserve/window");
        available=0x1ef000-(uint32_t)occupied;
        uint32_t max=u16(b+12);
        if(max!=65535&&available>max*16)available=max*16;
        if(available<16)fail("extended heap header");
        if(bss>available)fail("extended minalloc");
        if(((uint64_t)heap+15)/16*16>available-16)fail("extended minimum heap");
    } else if(u16(b+20)||u16(b+22))fail("entry must be module offset 0");
    if(n&&(table<32||table>size||n>(size-table)/4))fail("relocation table extent");
    if(n&&!((table+n*4<=hdr)||(table>=end)))fail("table overlaps module");
    for(uint32_t i=0;i<n;i++) {
        uint32_t off=u32(b+table+4*i);
        if(image<4||off>image-4)fail("relocation site outside module");
        /* Extended startup takes the one-past-BSS address to clear a range.
         * The site is always inside the module; its pointer may name the
         * reserved tail (or its end). Legacy tenant policy stays unchanged. */
        if(extended ? (uint64_t)u32(b+hdr+off)>(uint64_t)image+reserve :
                      (uint64_t)u32(b+hdr+off)>=(uint64_t)image+reserve)
            fail("relocated pointer outside image+BSS");
        for(uint32_t j=0;j<i;j++){uint32_t prev=u32(b+table+4*j);if((off>prev?off-prev:prev-off)<4)fail("overlapping relocation sites");}
    }
    if(argc==4) {
        char *tail;unsigned long wanted=strtoul(argv[3],&tail,0);
        if(*tail||wanted>UINT32_MAX||
           (extended ? reserve!=wanted : bss!=((uint64_t)wanted+15)/16*16))fail("BSS recording");
        FILE *raw=fopen(argv[2],"rb");if(!raw)fail("raw module open");
        for(uint32_t i=0;i<image;i++)if(fgetc(raw)!=b[hdr+i])fail("module changed");
        if(fgetc(raw)!=EOF||ferror(raw)||fclose(raw))fail("module length");
    }
    printf("MZ valid: file=%u header=%u image=%u entry=0 relocations=%u BSS=%u\n",size,hdr,image,n,bss);
    if(extended)printf("extended: reserve=%u module-headroom=%u heap-headroom=%u stack=65536 guard=4096\n",
                       reserve,262144-image,available-16-heap);
    free(b);return 0;
}
