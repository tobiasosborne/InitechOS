/* Build-time storage packing, not a new font. Ref: PRD Sec 6.4;
 * spec/assets/chicago12.h remains the locked source of every glyph bit.
 * Deterministic first-seen row dictionary, trimmed blank rows per glyph.
 */
#include <stdint.h>
#include <stdio.h>
/* -DPACK_GENEVA12 (bead initech-uelk) packs the locked Geneva 12 strike with
 * the SAME algorithm into geneva12_pack_* names; the default build is the
 * unchanged Chicago packer (its output is byte-identical to before). */
#ifdef PACK_GENEVA12
#include "geneva12.h"
#define PK_ROWS   geneva12_rows
#define PK_GLYPHS GENEVA12_GLYPHS
#define PK_CELL_H GENEVA12_CELL_H
#define PK        "geneva"
#define PK_SRC    "geneva12.h"
#else
#include "chicago12.h"
#define PK_ROWS   chicago12_rows
#define PK_GLYPHS CHICAGO_GLYPHS
#define PK_CELL_H CHICAGO_CELL_H
#define PK        "chicago"
#define PK_SRC    "chicago12.h"
#endif
int main(void)
{
    uint16_t values[256]={0},offsets[PK_GLYPHS];
    uint8_t first[PK_GLYPHS],count[PK_GLYPHS];
    uint8_t rows[PK_GLYPHS*PK_CELL_H];
    unsigned nv=1,nr=0;
    for(unsigned g=0;g<PK_GLYPHS;g++) {
        unsigned a=0,z=PK_CELL_H;
        while(a<z && !PK_ROWS[g][a]) ++a;
        while(z>a && !PK_ROWS[g][z-1]) --z;
        first[g]=(uint8_t)a;count[g]=(uint8_t)(z-a);offsets[g]=(uint16_t)nr;
        for(unsigned r=a;r<z;r++) {
            uint16_t v=PK_ROWS[g][r]; unsigned i;
            for(i=0;i<nv && values[i]!=v;i++) { }
            if(i==nv) {
                if(nv==256) { fputs(PK "_pack: >256 unique rows\n",stderr); return 1; }
                values[nv++]=v;
            }
            rows[nr++]=(uint8_t)i;
        }
    }
    puts("/* Generated from the unchanged locked " PK_SRC "; no timestamps. */");
    printf("static const uint16_t " PK "_pack_values[%u]={",nv);
    for(unsigned i=0;i<nv;i++) printf("%s%u",i?",":"",values[i]);
    puts("};");
    printf("static const uint16_t " PK "_pack_offsets[%u]={",PK_GLYPHS);
    for(unsigned g=0;g<PK_GLYPHS;g++) printf("%s%u",g?",":"",offsets[g]);
    puts("};");
    printf("static const uint8_t " PK "_pack_first[%u]={",PK_GLYPHS);
    for(unsigned g=0;g<PK_GLYPHS;g++) printf("%s%u",g?",":"",first[g]);
    puts("};");
    printf("static const uint8_t " PK "_pack_count[%u]={",PK_GLYPHS);
    for(unsigned g=0;g<PK_GLYPHS;g++) printf("%s%u",g?",":"",count[g]);
    puts("};");
    printf("static const uint8_t " PK "_pack_rows[%u]={",nr);
    for(unsigned i=0;i<nr;i++) printf("%s%u",i?",":"",rows[i]);
    puts("};");
    fprintf(stderr,PK "_pack: %u unique rows, %u stored indices, %u B (was %u B)\n",
            nv,nr,2*nv+nr+4*PK_GLYPHS,2*PK_GLYPHS*PK_CELL_H);
    return ferror(stdout)?1:0;
}
