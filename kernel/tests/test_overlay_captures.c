/* Genuine DOS3.30 capture grades product state after equivalent flat-format
 * probe. Only the container/tag and relocation width adapt to InitechMZ;
 * capture values determine load, factor, expected modified addresses/values. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "loader.h"
#include "mz.h"
#include "test_assert.h"
TEST_HARNESS();
static uint8_t file[64], memory[256], scratch[LOADER_OVL_SCRATCH];
static unsigned a, b;
static int deny_factor, accept_all;
static unsigned word(const uint8_t *p) { return p[0] | ((unsigned)p[1]<<8); }
static void put16(unsigned off,unsigned value) { file[off]=value;file[off+1]=value>>8; }
static int read_bytes(void *ctx,uint32_t off,uint32_t len,uint8_t *dst)
{ (void)ctx; if(off>64 || len>64-off) return 1;memcpy(dst,file+off,len);return 0; }
static int check_file(void *ctx) { (void)ctx;return 0; }
static int owns(uint32_t base,uint32_t len)
{ return accept_all || (base>=a && base<=a+16 && len<=a+16-base) ||
         (!deny_factor && base>=b && base<=b+4 && len<=b+4-base); }
static void probe(const char *path,int equal)
{
    uint8_t capture[12];
    FILE *f=fopen(path,"rb");
    CHECK(f!=NULL,"DOS capture opens"); if(!f)return;
    int valid=fread(capture,1,12,f)==12 && fgetc(f)==EOF;fclose(f);
    CHECK(valid,"DOS capture is six words"); if(!valid)return;
    a=word(capture+4);b=word(capture+6);
    CHECK(b>a && b-a+4<sizeof memory,"captured allocations are disjoint and fit probe");
    if(b<=a || b-a+4>=sizeof memory)return;
    memset(file,0,sizeof file);memset(memory,0x55,sizeof memory);
    /* overlay.asm: 48-byte header, 16-byte module, initial 0010, reloc at 0. */
    put16(0,0x5a4d);put16(2,64);put16(4,1);put16(6,1);put16(8,3);
    put16(0x18,32);put16(0x1c,MZ_INITECH_TAG);put16(48,0x10);
    memory[b-a]=0x11;memory[b-a+1]=0x11;memory[b-a+2]=memory[b-a+3]=0;
    uint32_t got=0xdead;
    loader_status_t st=loader_overlay_core(read_bytes,0,sizeof file,memory,a,equal?a:b,
                                          owns,check_file,scratch,&got);
    CHECK((st==LOADER_OK)==(word(capture)==0),"product status matches DOS captured carry");
    CHECK(got==16,"product copied the captured module footprint");
    printf("DOS33 %s: product A=%04x B=%04x, capture A=%04x B=%04x\n",
           equal?"equal":"unequal",word(memory),word(memory+b-a),word(capture+8),word(capture+10));
    CHECK(word(memory)==word(capture+8),"product load-block word matches genuine DOS capture");
    CHECK(word(memory+b-a)==word(capture+10),"product factor-block word matches genuine DOS capture");
    CHECK(memory[2]==0 && memory[3]==0 && memory[b-a+2]==0 && memory[b-a+3]==0,
          "flat fixups retain their full 32-bit zero extension for these values");
    if (!equal) {
        uint8_t before[sizeof memory];
        memcpy(before,memory,sizeof memory);deny_factor=1;got=0xdead;
        st=loader_overlay_core(read_bytes,0,sizeof file,memory,a,b,
                               owns,check_file,scratch,&got);
        CHECK(st==LOADER_ERR_NOT_OWNED,"factor block requires its own ownership proof");
        CHECK(got==0xdead && !memcmp(before,memory,sizeof memory),
              "unowned factor refusal changes neither allocation");
        deny_factor=0;accept_all=1;
        /* Valid module-relative offset, impossible full-width flat address. */
        put16(32,4);
        st=loader_overlay_core(read_bytes,0,sizeof file,memory,a,0xfffffffeu,
                               owns,check_file,scratch,&got);
        CHECK(st==LOADER_ERR_NOT_OWNED && !memcmp(before,memory,sizeof memory),
              "factor address wrap refuses before any destination write");
        accept_all=0;
    }
}
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    probe(argv[1],0);probe(argv[2],1);
    return TEST_SUMMARY("overlay-product-captures");
}
