/* Independent BIOS arithmetic/register oracle. IBM DOS TR pp9-8..9;
 * IBM BIOS Interface TR pp2-59..61,2-64,2-67; DEC-17 PM ruling.
 * Expected geometry and status bytes are literals, not generated spec values. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block_device.h"
#include "block_device_contract.h"
static unsigned calls, writes, notices, fail_io;
static uint32_t last;
static unsigned char buffer[1024]; /* -no-pie: DEC-04a's 32-bit EBX */
static void require(int ok, const char *rule)
{ if (!ok) { fprintf(stderr,"RED block-device %s\n",rule); exit(1); } }
static int rd(void *ctx, uint32_t l, uint32_t n, void *p)
{ (void)ctx; (void)p; require(n>0,"backend count"); calls++; last=l; return (int)fail_io; }
static int wr(void *ctx, uint32_t l, uint32_t n, const void *p)
{ writes++; return rd(ctx,l,n,(void *)p); }
static void notice(uint32_t l, uint32_t n, const void *p)
{ require(l==0 && n==1 && p==buffer,"cache volume-relative overlap"); notices++; }
static unsigned watch_letter, watch_n;
static uint32_t watch_lba;
static void watched(unsigned letter, uint32_t l, uint32_t n, const void *p)
{ require(p==buffer+512,"fixed watch buffer offset"); watch_letter=letter; watch_lba=l; watch_n=n; }
static int_frame_t frame(unsigned ax, unsigned cx, unsigned dx)
{ int_frame_t f={0}; f.eax=ax;f.ecx=cx;f.edx=dx;f.ebx=(uint32_t)(uintptr_t)buffer;f.eflags=0x202;return f; }
int main(void)
{
    require(BD_vector==19 && BD_master_drive==128 && BD_slave_drive==129 &&
            BD_floppy_controller==0 && BD_heads==16 && BD_sectors_per_track==63 &&
            BD_max_cylinders==1024 && BD_sector_bytes==512,"SPEC BIOS ABI constants");
    uint32_t l;
    for (unsigned c=0;c<1024;c+=7) for(unsigned h=0;h<16;h++) for(unsigned s=1;s<=63;s++) {
        unsigned cx=((c&255)<<8)|((c>>2)&192)|s;
        require(block_device_chs((uint16_t)cx,(uint8_t)h,1032192,&l)==0 &&
                l==(c*16+h)*63+s-1,"CHS heads/sector/cylinder arithmetic");
    }
    require(block_device_chs(0,0,1008,&l)!=0,"sector zero rejected");
    require(block_device_chs(1,16,1008,&l)!=0,"head 16 rejected");
    require(block_device_chs(0x100,0,1008,&l)!=0,"outside device rejected");
    require(block_device_status(0)==0 && block_device_status(-1)==1 &&
            block_device_status(-2)==0x80 && block_device_status(-3)==0x80 &&
            block_device_status(-4)==0x20,"ATA to BIOS status mapping");
    blockdev_t d={0,rd,wr};block_device_bind(0,&d,8064);block_device_bind(1,&d,4096);
    int_frame_t f=frame(0x0201,1,0x81);int13_dispatch(&f);
    require(!(f.eflags&1) && (f.eax&0xff00)==0 && calls==1 && last==0,"read device LBA zero");
    f=frame(0x0201,1,0x1081);int13_dispatch(&f);
    require((f.eflags&1) && ((f.eax>>8)&255)==4 && calls==1,"carry set on CHS error");
    f=frame(0x0200,1,0x81);int13_dispatch(&f);
    require((f.eflags&1) && ((f.eax>>8)&255)==1,"zero count rejected");
    f=frame(0x0202,0x403f,0x0081);int13_dispatch(&f);
    require(f.eflags&1,"crossing end rejected");
    for(unsigned dl=0;dl<2;dl++) {
        f=frame(0x0800,0,dl);int13_dispatch(&f);
        require((f.eflags&1) && ((f.eax>>8)&255)==1,"no contracted floppy");
    }
    f=frame(0x0800,0,0x80);int13_dispatch(&f);
    require(!(f.eflags&1) && (f.ecx&65535)==0x073f && (f.edx&65535)==0x0f02,"AH08 fixed translation maximums");
    f=frame(0x1500,0,0x80);int13_dispatch(&f);
    require(!(f.eflags&1) && (f.eax&0xff00)==0x0300 && (f.edx&65535)==8064 && !(f.ecx&65535),"AH15 fixed disk size");
    f=frame(0x4200,0,0x80);int13_dispatch(&f);
    require((f.eflags&1) && ((f.eax>>8)&255)==1,"no EDD or extra functions");
    fail_io=(unsigned)-3;f=frame(0x0201,1,0x81);int13_dispatch(&f);
    require((f.eflags&1) && ((f.eax>>8)&255)==0x80,"backend timeout carry/status");fail_io=0;
    block_device_mounted(0x81,63,2880,notice);
    f=frame(0x0301,1,0x0181);int13_dispatch(&f);
    require(!(f.eflags&1) && last==63 && writes==1 && notices==1,"raw write coherence at volume origin");
    f=frame(0x0301,33,0x81);int13_dispatch(&f);
    require(!(f.eflags&1) && last==32 && notices==1,"prefix write device relative");
    /* L042: D (BIOS 80h primary 2 at LBA 4032) hears a two-sector write
     * straddling its first sector, volume-relative, with its letter. */
    du_map dmap={0x80,2,4032,2880,0};block_device_watch(3,&dmap,watched);
    f=frame(0x0302,0x033f,0x0f80);int13_dispatch(&f); /* C3 H15 S63 = 4031 */
    require(!(f.eflags&1) && last==4031 && watch_letter==3 && watch_lba==0 && watch_n==1,"fixed volume raw write watch");
    watch_letter=0;f=frame(0x0301,1,0x81);int13_dispatch(&f);
    require(watch_letter==0,"watch is per physical disk");
    puts("GREEN test-block-device: CHS grid, BIOS statuses/CF, drive map, geometry, raw origin, coherence");
    return 0;
}
