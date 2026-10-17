/* Chicago storage decoder. Ref: PRD Sec 6.4; unchanged locked chicago12.h.
 * Packing is a factory step; this is freestanding C, no heap or host paths.
 * The complete original row table is the host equality oracle, not a value
 * reconstructed through this decoder. Historical pixel gates remain separate.
 */
#ifndef INITECH_CHICAGO_PACKED_H
#define INITECH_CHICAGO_PACKED_H
#include <stdint.h>
#include "chicago_packed_data.h"
static inline uint16_t chicago_packed_row(unsigned g,unsigned r)
{
    unsigned first=chicago_pack_first[g];
    if(r<first || r-first>=chicago_pack_count[g]) return 0;
    uint16_t v=chicago_pack_values[chicago_pack_rows[chicago_pack_offsets[g]+r-first]];
#ifdef CHICAGO_PACK_MUT_ROW
    if(g==1 && r==3) v^=1u;
#endif
    return v;
}
#endif
