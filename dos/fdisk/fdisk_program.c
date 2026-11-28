/* Flat COM adapter, ADR-0003 Appendix A / DEC-04a / DEC-08 / DEC-17.
 * BIOS interface: IBM BIOS TR pp2-60..61,2-67; console AH07/40, exit AH4C.
 * No host paths, clock reads or host runtime. */
#include "fdisk.h"
#include "fdisk_gates.h"
#include "diskutil.h"      /* du_map: DEC-17 AH60h map record */
#include "dos_messages.h"
#include "drive_letters.h" /* spec/drive_letters.json (DEC-18) */
#include "fat_policy.h"
static int sector(unsigned drive,uint8_t *p,int write)
{
    unsigned c=1,d=drive;
    return fdisk_int13(write?0x0301:0x0201,(uintptr_t)p,&c,&d);
}
static int read_disk(unsigned d,uint8_t p[512]){return sector(d,p,0);}
static int write_disk(unsigned d,const uint8_t p[512]){return sector(d,(uint8_t *)p,1);}
static uint32_t capacity(unsigned drive)
{
    unsigned c=0,d=drive;
    return fdisk_int13(0x1500,0,&c,&d)!=3?0:((c&65535)<<16)|(d&65535);
}
static int map(unsigned letter,du_map *m)
{
    unsigned c=0,d=0;
    return fdisk_int13((DL_function<<8)|letter,(uintptr_t)m,&c,&d);
}
/* CLAUDE.md "Two interfaces": the published map carries the running
 * volume's physical disk even after COMMAND changes the selected drive. */
static unsigned running_disk(void)
{
    unsigned disk=0;
    for(unsigned l=0;l<4;l++) {
        du_map m;
        if(map(l,&m) || !(m.flags&DU_RUNNING))continue;
        if((m.disk!=0x80 && m.disk!=0x81) || (disk && disk!=m.disk))
            return 0;
        disk=m.disk;
    }
    return disk;
}
/* Ask the kernel's DEC-18 resolver (INT13 AH60h, AL = zero-based letter)
 * which letter owns this primary entry; FDISK never decides it itself. */
static char letter(unsigned drive,unsigned partition)
{
    for(unsigned l=0;l<4;l++) {
        du_map m;
        if(!map(l,&m) && m.disk==drive && m.partition==partition)return (char)('A'+l);
    }
    return 0;
}
static char protected_letter(unsigned drive,unsigned partition)
{
    for(unsigned l=0;l<4;l++) {
        du_map m;
        if(map(l,&m) || m.disk!=drive || m.partition!=partition)continue;
        uint8_t boot[512];unsigned cyl=m.base/(63*16),head=(m.base/63)%16;
        unsigned c=((cyl&255)<<8)|((cyl>>2)&192)|(m.base%63+1),d=(head<<8)|drive;
        /* An unreadable mounted extent is never safe to reassign. */
        if(fdisk_int13(0x0201,(uintptr_t)boot,&c,&d) || dos_fat16(boot))return (char)('A'+l);
    }
    return 0;
}
static int get(void)
{
    return (int)(fdisk_int21(0x0700,0,0,0)&255);
}
static void put(const char *p)
{
    unsigned n=0;while(p[n])n++;
    fdisk_int21(0x4000,1,n,(uintptr_t)p);
}
int fdisk_entry(const char *tail,unsigned length)
{
    /* DOS 3.30 FDISKZ ignored its tail. GOLDEN-MASTER item 2 requires
     * unsupported switches to be refused before entering the editor. */
    for(unsigned i=0;i<length;i++){
        if(tail[i]!=' ' && tail[i]!='\t'){
            put(MSG_DOS_0064 "\r\n");
            return 1;
        }
    }
    const unsigned disk=running_disk();
    if(!disk)return 1;
    const fdisk_io io={read_disk,write_disk,capacity,get,put,disk,fdisk_restart,letter,protected_letter};
    return fdisk_run(&io);
}
