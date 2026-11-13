/* initech-480p/dbp1: literal causes from Rule 2, DEC-08a.5, DOS L030/DAS
 * L001. Expected text/pixels never come from the production message table. */
#include <stdio.h>
#include <string.h>
#include "film_startup.h"

static int glyph(const unsigned char *font, const char *s, unsigned scale,
                 unsigned x, unsigned y, unsigned y0)
{
    unsigned width=(unsigned)strlen(s)*8u*scale, x0=(320u-width)/2u;
    if (x<x0 || x>=x0+width || y<y0 || y>=y0+16u*scale) return 0;
    x=(x-x0)/scale; y=(y-y0)/scale;
    return !!(font[(unsigned char)s[x/8u]*16u+y] & (128u>>(x%8u)));
}

int main(void)
{
    static const struct {
        int volume, apps, load;
        panic_reason_t reason;
        const char *text, *subject;
    } cases[] = {
        {FAT12_ERR_READ,FAT12_OK,TBX_OK,PANIC_REASON_DATA_VOLUME,"REQUIRED DATA DISK UNAVAILABLE","DATA DISK"},
        {FAT12_ERR_GEOMETRY,FAT12_OK,TBX_OK,PANIC_REASON_DATA_VOLUME,"REQUIRED DATA DISK UNAVAILABLE","DATA DISK"},
        {FAT12_OK,FAT12_ERR_NOT_FOUND,TBX_OK,PANIC_REASON_APPS,"REQUIRED APPS FOLDER UNAVAILABLE","APPS"},
        {FAT12_OK,FAT12_ERR_READ,TBX_OK,PANIC_REASON_APPS,"REQUIRED APPS FOLDER UNAVAILABLE","APPS"},
        {FAT12_OK,FAT12_OK,TBX_ERR_BADARG,PANIC_REASON_CALC_LOAD,"CANNOT LOAD REQUIRED CALC.EXE","CALC.EXE"},
        {FAT12_OK,FAT12_OK,TBX_ERR_BUSY,PANIC_REASON_CALC_LOAD,"CANNOT LOAD REQUIRED CALC.EXE","CALC.EXE"},
        {FAT12_OK,FAT12_OK,TBX_ERR_NOMEM,PANIC_REASON_HEAP,"FLAIR HEAP EXHAUSTED","CALC.EXE"}
    };
    for (unsigned i=0;i<sizeof cases/sizeof cases[0];i++) {
        if (film_startup_reason(cases[i].volume,cases[i].apps,cases[i].load)!=cases[i].reason ||
            strcmp(panic_reason_text(cases[i].reason),cases[i].text)) {
            printf("RED panic-reason-map: case=%u expected=%s\n",i,cases[i].text);
            return 1;
        }
    }
    if (film_startup_reason(FAT12_OK,FAT12_OK,TBX_OK)!=PANIC_REASON_NONE) return 1;
    static unsigned char font[4096], fb[4*320*200+7*200+32];
    for (unsigned c=33;c<127;c++) for (unsigned y=0;y<16;y++)
        font[c*16+y]=(unsigned char)((c+y)^(128u>>(y%8u)));
    for (unsigned i=0;i<=sizeof cases/sizeof cases[0];i++) {
        panic_reason_t reason=i ? cases[i-1].reason : PANIC_REASON_FOREIGN_MZ;
        const char *text=i ? cases[i-1].text : "UNSUPPORTED 16-BIT MZ";
        const char *subject=i ? cases[i-1].subject : "DOS16.EXE";
        for (unsigned bpp=8;bpp<=32;bpp+=8) {
            if (bpp==16) continue;
            unsigned bytes=bpp/8, pitch=320*bytes+7, size=pitch*200;
            memset(fb,0xa5,sizeof fb);
            panic_screen_reason(fb+16,pitch,bpp,320,200,font,reason,subject);
            for (unsigned y=0;y<200;y++) {
                for (unsigned x=0;x<320;x++) {
                    int ink=glyph(font,"PC LOAD LETTER",2,x,y,68) ||
                            glyph(font,text,1,x,y,116) || glyph(font,subject,1,x,y,140);
                    unsigned want=ink ? (bpp==8?15:192) : 0;
                    for (unsigned c=0;c<bytes;c++)
                        if (fb[16+y*pitch+x*bytes+c]!=(c==3?0:want)) {
                            printf("RED panic-reason-pixels: case=%u bpp=%u x=%u y=%u\n",i,bpp,x,y);
                            return 1;
                        }
                }
                for (unsigned x=320*bytes;x<pitch;x++)
                    if (fb[16+y*pitch+x]!=0xa5) return 1;
            }
            for (unsigned j=0;j<16;j++)
                if (fb[j]!=0xa5 || fb[16+size+j]!=0xa5) return 1;
        }
    }
    puts("GREEN panic-reasons: seven failure mappings + success; foreign program/reasons in 8/24/32bpp, padding/guards intact");
    return 0;
}
