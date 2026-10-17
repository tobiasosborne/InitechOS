/* Build-time storage packing, not a new font. Ref: PRD Sec 6.4;
 * spec/assets/chicago12.h remains the locked source of every glyph bit.
 * Deterministic first-seen row dictionary, trimmed blank rows per glyph.
 */
#include <stdint.h>
#include <stdio.h>
#include "chicago12.h"
int main(void)
{
    uint16_t values[256]={0},offsets[CHICAGO_GLYPHS];
    uint8_t first[CHICAGO_GLYPHS],count[CHICAGO_GLYPHS];
    uint8_t rows[CHICAGO_GLYPHS*CHICAGO_CELL_H];
    unsigned nv=1,nr=0;
    for(unsigned g=0;g<CHICAGO_GLYPHS;g++) {
        unsigned a=0,z=CHICAGO_CELL_H;
        while(a<z && !chicago12_rows[g][a]) ++a;
        while(z>a && !chicago12_rows[g][z-1]) --z;
        first[g]=(uint8_t)a;count[g]=(uint8_t)(z-a);offsets[g]=(uint16_t)nr;
        for(unsigned r=a;r<z;r++) {
            uint16_t v=chicago12_rows[g][r]; unsigned i;
            for(i=0;i<nv && values[i]!=v;i++) { }
            if(i==nv) {
                if(nv==256) { fputs("chicago_pack: >256 unique rows\n",stderr); return 1; }
                values[nv++]=v;
            }
            rows[nr++]=(uint8_t)i;
        }
    }
    puts("/* Generated from the unchanged locked chicago12.h; no timestamps. */");
    printf("static const uint16_t chicago_pack_values[%u]={",nv);
    for(unsigned i=0;i<nv;i++) printf("%s%u",i?",":"",values[i]);
    puts("};");
    printf("static const uint16_t chicago_pack_offsets[%u]={",CHICAGO_GLYPHS);
    for(unsigned g=0;g<CHICAGO_GLYPHS;g++) printf("%s%u",g?",":"",offsets[g]);
    puts("};");
    printf("static const uint8_t chicago_pack_first[%u]={",CHICAGO_GLYPHS);
    for(unsigned g=0;g<CHICAGO_GLYPHS;g++) printf("%s%u",g?",":"",first[g]);
    puts("};");
    printf("static const uint8_t chicago_pack_count[%u]={",CHICAGO_GLYPHS);
    for(unsigned g=0;g<CHICAGO_GLYPHS;g++) printf("%s%u",g?",":"",count[g]);
    puts("};");
    printf("static const uint8_t chicago_pack_rows[%u]={",nr);
    for(unsigned i=0;i<nr;i++) printf("%s%u",i?",":"",rows[i]);
    puts("};");
    fprintf(stderr,"chicago_pack: %u unique rows, %u stored indices, %u B (was %u B)\n",
            nv,nr,2*nv+nr+4*CHICAGO_GLYPHS,2*CHICAGO_GLYPHS*CHICAGO_CELL_H);
    return ferror(stdout)?1:0;
}
