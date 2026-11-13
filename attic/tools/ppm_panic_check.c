/* Independent full-frame oracle: PRD Appendix B + s25 lane panel layout.
 * DO NOT include panic_screen.h. Font input is the pre-fault boot ROM bytes
 * written by PANIC.COM via DOS, never reconstructed from the framebuffer.
 * This grades every glyph bit and every background pixel, including no dump.
 * VGA13 may be scan-doubled by the emulator; logical dimensions are explicit.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
static unsigned number(FILE *f)
{
    int c;
    do {
        c=fgetc(f);
        if (c=='#') while ((c=fgetc(f))!='\n' && c!=EOF) {}
    } while (isspace(c));
    unsigned n=0;
    while (c>='0' && c<='9') { n=n*10+(unsigned)(c-'0'); c=fgetc(f); }
    return n;
}
static int ink(const unsigned char *font, const char *s, unsigned scale,
               unsigned x, unsigned y, unsigned w, unsigned h, int status)
{
    unsigned len=(unsigned)strlen(s), x0=(w-len*8*scale)/2;
    unsigned y0=(h-64)/2+(status?48:0);
    if (x<x0 || y<y0 || x>=x0+len*8*scale || y>=y0+16*scale) return 0;
    x=(x-x0)/scale; y=(y-y0)/scale;
    return !!(font[(unsigned char)s[x/8]*16+y] & (128u>>(x%8)));
}
int main(int argc,char **argv)
{
    unsigned char font[4096], pixel[3];
    if (argc!=6 && argc!=7) { fprintf(stderr,"usage: %s ppm font width height vector | reason subject\n",argv[0]); return 2; }
    unsigned w=(unsigned)atoi(argv[3]), h=(unsigned)atoi(argv[4]);
    unsigned vec=(unsigned)atoi(argv[5]);
    if (w<224 || h<64 || (argc==6 && vec>31) || (argc==7 && h<88)) return 2;
    FILE *f=fopen(argv[2],"rb");
    if (!f || fread(font,1,4096,f)!=4096 || fgetc(f)!=EOF) return 2;
    fclose(f);
    /* Reject a missing/stub font rather than accepting a blank-screen golden. */
    unsigned bits=0; for (unsigned i=0;i<16;i++) bits|=font['P'*16+i];
    if (!bits) return 2;
    f=fopen(argv[1],"rb"); if (!f) return 2;
    if (fgetc(f)!='P' || fgetc(f)!='6') return 2;
    unsigned pw=number(f), ph=number(f), max=number(f);
    if (pw%w || ph%h || pw/w!=ph/h || pw/w<1 || pw/w>2 || max!=255) return 2;
    unsigned zoom=pw/w;
    char status[32]; snprintf(status,sizeof status,"ERROR %02u -- SEE OPERATOR",vec);
    const char *reason=argc==7 ? argv[5] : status;
    const char *subject=argc==7 ? argv[6] : "";
    if (strlen(reason)*8>w || strlen(subject)*8>w) return 2;
    for (unsigned y=0;y<ph;y++) for (unsigned x=0;x<pw;x++) {
        if (fread(pixel,1,3,f)!=3) return 2;
        int expected=ink(font,"PC LOAD LETTER",2,x/zoom,y/zoom,w,h,0) ||
                     ink(font,reason,1,x/zoom,y/zoom,w,h,1) ||
                     (argc==7 && y/zoom>=24 &&
                      ink(font,subject,1,x/zoom,y/zoom-24,w,h,1));
        for (unsigned ch=0;ch<3;ch++) {
            if ((!expected && pixel[ch]!=0) ||
                (expected && abs((int)pixel[ch]-192)>3)) {
                printf("RED panic-pixels: x=%u y=%u got=%u,%u,%u expected=%s\n",
                       x,y,pixel[0],pixel[1],pixel[2],expected?"glyph":"field");
                return 1;
            }
        }
    }
    if (fgetc(f)!=EOF) return 2;
    fclose(f);
    printf("GREEN panic-pixels: %ux%u full field, VGA ROM 2x message + %s %s, no other ink\n",w,h,reason,subject);
    return 0;
}
