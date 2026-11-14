/* GM2, operator 2026-11-14: FAT16 import volumes are immutable.
 * FAT type is the data-cluster count, not the BPB's descriptive string.
 * Microsoft FAT specification, FAT Type Determination (4085/65525).
 * This reader also handles cluster sizes unsupported by FORMAT/SYS. */
#ifndef DOS_FAT_POLICY_H
#define DOS_FAT_POLICY_H
#include <stdint.h>
static inline unsigned dos_fat_word(const uint8_t *p)
{ return p[0]|((unsigned)p[1]<<8); }
static inline int dos_fat16(const uint8_t b[512])
{
    unsigned spc=b[13],reserved=dos_fat_word(b+14),spf=dos_fat_word(b+22);
    uint32_t total=dos_fat_word(b+19);
    if(!total)total=dos_fat_word(b+32)|((uint32_t)dos_fat_word(b+34)<<16);
    uint32_t data=reserved+b[16]*spf+(dos_fat_word(b+17)+15)/16;
    if(dos_fat_word(b+11)!=512 || !spc || (spc&(spc-1)) ||
       !reserved || !spf || !b[16] || !dos_fat_word(b+17) || data>=total)
        return 0;
    uint32_t clusters=(total-data)/spc;
    return clusters>=4085 && clusters<65525;
}
#endif
