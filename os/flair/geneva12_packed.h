/* Geneva 12 storage decoder (bead initech-uelk). Ref: PRD Sec 6.4; the
 * unchanged locked spec/assets/geneva12.h, packed by tools/chicago_pack.c
 * -DPACK_GENEVA12 (the Chicago algorithm: a first-seen row dictionary,
 * blank rows trimmed per glyph). Freestanding C, no heap or host paths.
 * The complete locked row table is the host equality oracle
 * (test-geneva12-pack), never a value reconstructed through this decoder.
 * GENEVA12_PACK_MUT_ROW (Rule 6; test-geneva12-pack-mutant and the
 * test-flair-itext-mutant G12ROW kernel) flips the first image column of
 * row 2 of 'S' -- the first glyph of the SimpleText line; one row above its
 * cap line, so the glyph's ink box grows a row -- in the kernel's strike.
 * NEVER in a real build.
 */
#ifndef INITECH_GENEVA12_PACKED_H
#define INITECH_GENEVA12_PACKED_H
#include <stdint.h>
#include "geneva12_packed_data.h"
static inline uint16_t geneva12_packed_row(unsigned g,unsigned r)
{
    unsigned first=geneva_pack_first[g];
    uint16_t v=0;
    if(r>=first && r-first<geneva_pack_count[g])
        v=geneva_pack_values[geneva_pack_rows[geneva_pack_offsets[g]+r-first]];
#ifdef GENEVA12_PACK_MUT_ROW
    if(g==(unsigned)('S'-0x20) && r==2u) v^=0x8000u;
#endif
    return v;
}
#endif
