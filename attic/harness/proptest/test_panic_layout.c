/* PRD Sec 5 / Appendix B; initech-s25 lane's two-line panel contract.
 * Literal independently calculated origins, not a call to another renderer. */
#include <stdio.h>
#include <string.h>
#include "panic_screen.h"
int main(void)
{
    static const unsigned cases[][6] = {
        {640,480,14,2,208,208}, {800,600,14,2,288,268},
        {832,624,14,2,304,280}, {320,200,14,2,48,68},
        {641,481,14,2,208,208}, {640,480,24,1,224,256},
        {320,200,24,1,64,116}, {800,600,24,1,304,316}
    };
    for (unsigned i=0; i<sizeof cases/sizeof cases[0]; i++) {
        const unsigned *c=cases[i];
        const char *s=c[2]==14 ? "PC LOAD LETTER" : "ERROR 06 -- SEE OPERATOR";
        panic_origin_t p=panic_origin(c[0],c[1],s,c[3],c[3]==1);
        if (p.x!=c[4] || p.y!=c[5]) {
            printf("RED panic-layout: off-centre case=%u got=%u,%u want=%u,%u\n",
                   i,p.x,p.y,c[4],c[5]);
            return 1;
        }
    }
    /* Synthetic font and padded rows isolate direct 8/24/32 byte writes.
     * This complements the independent real-ROM emulator checker. */
    static unsigned char fb[4*320*200+7*200+32], font[4096];
    font['P'*16]=128;
    for (unsigned bpp=8;bpp<=32;bpp+=8) {
        if (bpp==16) continue;
        unsigned bytes=bpp/8, pitch=320*bytes+7, size=pitch*200;
        memset(fb,0xa5,sizeof fb);
        panic_screen_paint(fb+16,pitch,bpp,320,200,font,6);
        for (unsigned y=0;y<200;y++) {
            for (unsigned x=0;x<320;x++) {
                unsigned char *p=fb+16+y*pitch+x*bytes;
                int ink=(x>=48 && x<50 && y>=68 && y<70) ||
                        (x==200 && y==116); /* P in OPERATOR, status col 17 */
                unsigned want=ink ? (bpp==8?15:192) : 0;
                for (unsigned c=0;c<bytes;c++) {
                    if (p[c]!=(c==3?0:want)) {
                        printf("RED panic-paint: bpp=%u x=%u y=%u byte=%u\n",bpp,x,y,c);
                        return 1;
                    }
                }
            }
            for (unsigned x=320*bytes;x<pitch;x++)
                if (fb[16+y*pitch+x]!=0xa5) return 1;
        }
        for (unsigned i=0;i<16;i++)
            if (fb[i]!=0xa5 || fb[16+size+i]!=0xa5) return 1;
    }
    puts("GREEN panic-layout: 8 literal message/status origins, including VGA13 and odd sizes");
    puts("GREEN panic-paint: direct 8/24/32bpp glyph writes; row padding and framebuffer guards intact");
    return 0;
}
