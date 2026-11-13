/* tdnl.44 factory transcriber. Both independent Mac OS8.1 list captures
 * separate opaque art from background: Name's sorted body is E7E7E7, Size's
 * unsorted Name body F3F3F3. Every opaque pixel must agree across both.
 * Ref: mint-results-008 Sec6. No ROM or font resources are read. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <png.h>
#include "color_canon.h"
static unsigned char a[640*480*3],b[640*480*3];
static uint32_t pal[32];static int np=1;
static uint32_t rgb(unsigned char*p){return (uint32_t)p[0]*65536u+(uint32_t)p[1]*256u+p[2];}
static int load(char *s,unsigned char*p){png_image i={0};i.version=PNG_IMAGE_VERSION;if(!png_image_begin_read_from_file(&i,s))return 0;i.format=PNG_FORMAT_RGB;return i.width==640&&i.height==480&&png_image_finish_read(&i,0,p,0,0);}
int main(int n,char**v)
{
    if(n!=3||!load(v[1],a)||!load(v[2],b))return 2;
    unsigned char art[3][256];int rows[3][2]={{0,4},{2,3},{1,5}};
    for(int k=0;k<3;k++)for(int y=0;y<16;y++)for(int x=0;x<16;x++){
        uint32_t p=rgb(a+3*((90+rows[k][0]*19+y)*640+41+x));
        uint32_t q=rgb(b+3*((90+rows[k][1]*19+y)*640+41+x));
        int c=0;
        if(p!=0xe7e7e7u||q!=0xf3f3f3u){
            if(p!=q){fprintf(stderr,"FAIL opaque small icon mismatch %d %d %d\n",k,x,y);return 1;}
            for(c=1;c<np&&pal[c]!=p;c++);
            if(c==np){if(np==32)return 2;pal[np++]=p;}
        }
        art[k][y*16+x]=(unsigned char)c;
    }
    puts("/* tdnl.44 LOCKED 16x16 art, minted by tools/mint_finder_small_icons.c.\n * Source: s8_finder_list_view{,_bysize}.png, mint-results-008 Sec6.\n * Paired backgrounds independently separate clear from opaque pixels.\n * These are native small strikes, NOT a downsample of the old 32px art. */\n#ifndef INITECH_FINDER_SMALL_ICONS_H\n#define INITECH_FINDER_SMALL_ICONS_H\n#include <stdint.h>");
    printf("#define FINDER_SMALL_COLORS %d\n",np);
    printf("static const uint8_t finder_small_canon[%d] = {",np);
    for(int c=0;c<np;c++) {
        unsigned i;for(i=0;i<256 && flair_canon_rgb((uint8_t)i)!=pal[c];i++);
        if(i==256){fprintf(stderr,"FAIL native small colour missing in canon\n");return 1;}
        printf("%u,",i);
    }
    puts("};");
    puts("static const uint8_t finder_small_art[3][256] = {");
    for(int k=0;k<3;k++){puts("{");for(int y=0;y<16;y++){for(int x=0;x<16;x++)printf("%d,",art[k][y*16+x]);puts("");}puts("},");}puts("};\n#endif");return 0;
}
