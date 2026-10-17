/* R4.5 / initech-tdnl.19, Rule 8. Authored fixed disk contract; JSON companion.
 * Ref: period-gui-suite.md sec 5; INT21 AH=2A..2D; eight PAT#_0 entries.
 * RTC date/time live in CMOS, never restored from a stale disk timestamp. */
#ifndef INITECH_CP_RECORD_H
#define INITECH_CP_RECORD_H
#include <stdint.h>
#define CP_RECORD_BYTES 16u
#define CP_PATH "\\CPANELS.DAT"
typedef struct cp_settings { uint8_t pattern,tracking,double_click,sound,volume,clock; } cp_settings_t;
typedef struct cp_time { uint16_t year; uint8_t month,day,hour,minute,second,pad; } cp_time_t;
static inline void cp_defaults(cp_settings_t *s) { *s=(cp_settings_t){0,1,1,0,2,1}; }
static inline void cp_encode(const cp_settings_t *s,uint8_t b[16]) {
    b[0]='C'; b[1]='P'; b[2]='0'; b[3]='1'; b[4]=1;
    b[5]=s->pattern; b[6]=s->tracking; b[7]=s->double_click;
    b[8]=s->sound; b[9]=s->volume; b[10]=s->clock;
    b[11]=b[12]=b[13]=0;
    unsigned sum=0; for(unsigned i=0;i<14;i++) sum+=b[i];
    b[14]=(uint8_t)sum; b[15]=(uint8_t)(sum>>8);
}
static inline int cp_decode(cp_settings_t *s,const uint8_t b[16]) {
    unsigned sum=0; for(unsigned i=0;i<14;i++) sum+=b[i];
    if(b[0]!='C'||b[1]!='P'||b[2]!='0'||b[3]!='1'||b[4]!=1||b[5]>8||b[6]>2||
       b[7]>2||b[8]>2||b[9]>2||b[10]>1||b[11]||b[12]||b[13]||
       sum!=((unsigned)b[14]|((unsigned)b[15]<<8))) return 0;
    *s=(cp_settings_t){b[5],b[6],b[7],b[8],b[9],b[10]}; return 1;
}
/* Exact first eight System 7 PAT#_0.bin patterns, count BE16 then 8-byte tiles.
 * No palette is copied: set/clear bits use the existing desktop/face policy. */
static const uint8_t cp_patterns[8][8]={
 {255,255,255,255,255,255,255,255},{221,255,119,255,221,255,119,255},
 {221,119,221,119,221,119,221,119},{170,85,170,85,170,85,170,85},
 {85,255,85,255,85,255,85,255},{170,170,170,170,170,170,170,170},
 {238,221,187,119,238,221,187,119},{136,136,136,136,136,136,136,136}
};
#endif
