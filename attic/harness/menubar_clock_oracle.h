/* Factory only, tdnl.13: independent Mac OS 8 bar + original Chicago NFNT.
 * No artifact headers, art, metrics or renderer calls enter this oracle. */
#ifndef INITECH_MENUBAR_CLOCK_ORACLE_H
#define INITECH_MENUBAR_CLOCK_ORACLE_H
#include <stdint.h>
/* Sampled crop x520..599, y0..19. PRD Sec 6.4 requires Chicago 12 where
 * Mac OS 8 uses its narrower clock strike. Keep the captured baseline and
 * right ink edge592; grade the substituted glyphs against ORIGINAL NFNT5478,
 * never spec/assets/chicago12.h. This is the menu-look substitution pattern. */
int oracle_chicago_text(const char *nfnt,const char *text,uint32_t *out,
                         int width,int height,int x,int y);
int menubar_clock_golden(const char *capture, const char *nfnt,
                         uint32_t out[20][80]);
#endif
