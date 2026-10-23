/* Flat COM adapter, ADR-0003 Appendix A / DEC-04a / DEC-08 / DEC-17.
 * BIOS interface: IBM BIOS TR pp2-60..61,2-67; console AH07/40, exit AH4C.
 * All current FAT-sourced externals load from primary slave (kmain.c).
 * No host paths, clock reads or host runtime. */
#include "fdisk.h"
#include "diskutil.h"      /* du_map: DEC-17 AH60h map record */
#include "drive_letters.h" /* spec/drive_letters.json (DEC-18) */
#include "power.h" /* existing period AT 8042 reset mechanism */
static int sector(unsigned drive,uint8_t *p,int write)
{
    uint32_t a=write?0x0301:0x0201,b=(uint32_t)p,c=1,d=drive;unsigned char cf;
    __asm__ volatile("int $0x13":"=@ccc"(cf),"+a"(a),"+b"(b),"+c"(c),"+d"(d)::"memory","cc");
    return cf?-1:0;
}
static int read_disk(unsigned d,uint8_t p[512]){return sector(d,p,0);}
static int write_disk(unsigned d,const uint8_t p[512]){return sector(d,(uint8_t *)p,1);}
static uint32_t capacity(unsigned drive)
{
    uint32_t a=0x1500,c=0,d=drive;unsigned char cf;
    __asm__ volatile("int $0x13":"=@ccc"(cf),"+a"(a),"+c"(c),"+d"(d)::"memory","cc");
    return cf || ((a>>8)&255)!=3?0:((c&65535)<<16)|(d&65535);
}
/* Ask the kernel's DEC-18 resolver (INT13 AH60h, AL = zero-based letter)
 * which letter owns this primary entry; FDISK never decides it itself. */
static char letter(unsigned drive,unsigned partition)
{
    for(unsigned l=0;l<4;l++) {
        du_map m;uint32_t a=(DL_function<<8)|l,b=(uint32_t)&m;unsigned char cf;
        __asm__ volatile("int $0x13":"=@ccc"(cf),"+a"(a),"+b"(b)::"memory","cc");
        if(!cf && m.disk==drive && m.partition==partition)return (char)('A'+l);
    }
    return 0;
}
static int get(void)
{
    unsigned a=0x0700;
    __asm__ volatile("int $0x21":"+a"(a)::"memory","cc");return (int)(a&255);
}
static void put(const char *p)
{
    unsigned n=0;while(p[n])n++;
    unsigned a=0x4000;
    __asm__ volatile("int $0x21":"+a"(a):"b"(1),"c"(n),"d"(p):"memory","cc");
}
int fdisk_entry(void)
{
    extern void fdisk_restart(void);
    const fdisk_io io={read_disk,write_disk,capacity,get,put,0x81,fdisk_restart,letter};
    return fdisk_run(&io);
}
void fdisk_restart(void)
{
    (void)kernel_reset();
    /* A reset that returns cannot resume DOS with stale partition state. */
    kernel_halt();
}
