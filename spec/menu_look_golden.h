/* Independent menu crops, transcribed 2026-10-15 for tdnl.93/.94/.95/.70.
 * SOURCE: system7-decomp s8_menu_dropped.png x34 y0..19 and NFNT_5478.bin
 * code18: locTable @2126, loc[18]=18, loc[19]=27, ow @2648=0x010B.
 * No artifact header or renderer supplies these expectations.
 * Authored lavender->teal mapping: 8787DA -> 8DDCDC; 5454B3 -> 27666C;
 * 0000A5 -> 003F45. Indexed fallback: existing idx2/63/0. The indexed
 * palette has only one teal, as documented by the scroll lane (abd7679).
 * Face/white contrast: historical6.37, authored6.56, indexed10.53 to1.
 */
#ifndef INITECH_MENU_LOOK_GOLDEN_H
#define INITECH_MENU_LOOK_GOLDEN_H
#include <stdint.h>
static const uint32_t ML_source_band[20] = {
    0x8787DA,0x5454B3,0x5454B3,0x5454B3,0x5454B3,
    0x5454B3,0x5454B3,0x5454B3,0x5454B3,0x5454B3,
    0x5454B3,0x5454B3,0x5454B3,0x5454B3,0x5454B3,
    0x5454B3,0x5454B3,0x5454B3,0x0000A5,0x000000
};
static const uint16_t ML_check[16] = {
    0,0,0,0,0x0040,0x00C0,0x0180,0x0300,
    0x4600,0x6C00,0x3800,0x1000,0,0,0,0
};
/* NFNT F: loc[70..71]=231/236, ow lb1/aw7, cap rows3..11.
 * Used for white ink; no expected count is borrowed from our own File art. */
static const uint16_t ML_file_F[9] = {
    0x7C00,0x6000,0x6000,0x6000,0x7800,0x6000,0x6000,0x6000,0x6000
};
static inline uint32_t ml_band_rgb(int y,int bpp)
{
    if(y==19) return 0;
    if(y==0) return 0x8DDCDC;
    if(y==18) return bpp==8?0:0x003F45;
    return bpp==8?0x3F3F3F:0x27666C;
}
#endif
