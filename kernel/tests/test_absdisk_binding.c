/* initech-yg5r: exercise the production binding then the real INT26 gate.
 * Literal write-protect result is pinned by tests/data/absdisk_int2526.json.
 * Product FAT12-only writer policy, not a historical FAT16 restriction.
 */
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include "absdisk_binding.h"
#include "test_assert.h"
TEST_HARNESS();
fat12_volume_t g_absdisk_vol;
static unsigned writes;
static uint8_t disk[1024];
int int21_absdisk_read(uint32_t l,uint32_t n,void *p)
{ memcpy(p,disk+l*512,n*512); return 0; }
int int21_absdisk_write(uint32_t l,uint32_t n,const void *p)
{ writes++; memcpy(disk+l*512,p,n*512); return 0; }
void fileio_fat_absdisk_written(uint32_t l,uint32_t n,const void *p)
{ (void)l;(void)n;(void)p; }
void block_device_mounted(uint8_t d,uint32_t b,uint32_t n,
                          void (*f)(uint32_t,uint32_t,const void *))
{ (void)d;(void)b;(void)n;(void)f; }
static int writable(void *c,uint32_t l,uint32_t n,const void *p)
{ (void)c;return int21_absdisk_write(l,n,p); }
int main(void)
{
    uint8_t *buf=mmap(0,512,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    CHECK(buf!=MAP_FAILED && (uintptr_t)buf<=UINT32_MAX,"low transfer buffer");
    if(buf==MAP_FAILED)return 1;
    memset(buf,0x5a,512);
    blockdev_t raw={0}; raw.write_sectors=writable;
    fat12_volume_t vol={0};vol.total_sectors=2;
    for(unsigned i=0;i<2;i++) {
        vol.fat_type=i?FAT_TYPE_FAT12:FAT_TYPE_FAT16;
        bind_absdisk_volume(&vol,&raw); memset(disk,0xa5,sizeof disk); writes=0;
        int_frame_t f={0};f.ebx=(uint32_t)(uintptr_t)buf;f.ecx=1;f.edx=1;
        int26_dispatch(&f);
        if(!i) {
            CHECK((f.eflags&1) && (f.eax&65535)==0x0a00,"yg5r FAT16 A write-protected");
            CHECK(!writes,"yg5r FAT16 makes zero backend write calls");
            unsigned same=1;for(unsigned j=0;j<sizeof disk;j++)if(disk[j]!=0xa5)same=0;
            CHECK(same,"yg5r FAT16 sectors unchanged");
        } else CHECK(!(f.eflags&1) && writes==1 && !memcmp(disk+512,buf,512),"yg5r FAT12 still writes");
    }
    return TEST_SUMMARY("absdisk-binding");
}
