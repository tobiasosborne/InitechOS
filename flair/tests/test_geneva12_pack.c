/* Storage oracle: the LOCKED Geneva 12 row table
 * (spec/assets/geneva12.h, transcribed from geneva12.art) is the independent
 * input; no expected row is computed through the decoder under test.
 * PRD Sec 6.4; the test_chicago_pack.c pattern. */
#include <stdint.h>
#include <stdio.h>
#include "geneva12.h"
#include "geneva12_packed.h"
int main(void)
{
    unsigned checks = 0;
    for(unsigned g=0;g<GENEVA12_GLYPHS;g++)
        for(unsigned r=0;r<GENEVA12_CELL_H;r++) {
            uint16_t got=geneva12_packed_row(g,r);
            checks++;
            if(got!=geneva12_rows[g][r]) {
                printf("  FAIL geneva12-pack: glyph=%u row=%u got=%04X expected=%04X\n",
                       g,r,got,geneva12_rows[g][r]);
                printf("INITECH_CHECKS %u failures=1\n", checks);
                return 1;
            }
        }
    printf("INITECH_CHECKS %u failures=0\n", checks);
    printf(">>> test-geneva12-pack: green (all %u decoded rows == unchanged locked strike)\n",
           (unsigned)(GENEVA12_GLYPHS*GENEVA12_CELL_H));
    return 0;
}
