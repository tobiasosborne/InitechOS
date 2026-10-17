/* Flat adapter; ADR-0003 Appendix A (07/40/4C), DEC-04a/08/17/18.
 * spec/memory_map.h PSP tail; no duplicated drive-letter policy. */
#include "diskutil.h"
#include "drive_letters.h"
#include "memory_map.h"
static int map(unsigned letter,du_map *m){
 unsigned a=(DL_function<<8)|letter,b=(unsigned)m;unsigned char cf;
 __asm__ volatile("int $0x13":"=@ccc"(cf),"+a"(a),"+b"(b)::"memory","cc");return cf?-1:0;
}
static int sector(unsigned drive,uint32_t l,void *p,int write){
 unsigned cyl=l/(63*16),head=(l/63)%16;
 if(cyl>=1024)return -1;
 unsigned a=write?0x0301:0x0201,b=(unsigned)p,c=((cyl&255)<<8)|((cyl>>2)&192)|(l%63+1),d=(head<<8)|drive;unsigned char cf;
 __asm__ volatile("int $0x13":"=@ccc"(cf),"+a"(a),"+b"(b),"+c"(c),"+d"(d)::"memory","cc");return cf?-1:0;
}
static int get(void){unsigned a=0x0700;__asm__ volatile("int $0x21":"+a"(a)::"memory","cc");return (int)(a&255);}
static void put(const char *p){unsigned n=0;while(p[n])n++;unsigned a=0x4000;__asm__ volatile("int $0x21":"+a"(a):"b"(1),"c"(n),"d"(p):"memory","cc");}
int diskutil_entry(void){const du_io io={map,sector,get,put};char tail[128];volatile unsigned char *psp=(volatile unsigned char *)PROGRAM_BASE;unsigned n=psp[0x80];if(n>126)n=126;for(unsigned i=0;i<n;i++)tail[i]=(char)psp[0x81+i];tail[n]=0;return diskutil_run(&io,DU_KIND,tail);}
