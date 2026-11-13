/* Storage oracle: the LOCKED Chicago row table stays the independent input;
 * no expected row is computed through the decoder under test. PRD Sec 6.4. */
#include <stdint.h>
#include <stdio.h>
#include "chicago12.h"
#include "chicago_packed.h"
int main(void)
{
    for(unsigned g=0;g<CHICAGO_GLYPHS;g++)
        for(unsigned r=0;r<CHICAGO_CELL_H;r++) {
            uint16_t got=chicago_packed_row(g,r);
            if(got!=chicago12_rows[g][r]) {
                printf("!!! chicago-pack FAIL: glyph=%u row=%u got=%04X expected=%04X\n",
                       g,r,got,chicago12_rows[g][r]);
                return 1;
            }
        }
    puts(">>> test-chicago-pack: green (all 1568 decoded rows == unchanged locked strike)");
    return 0;
}
