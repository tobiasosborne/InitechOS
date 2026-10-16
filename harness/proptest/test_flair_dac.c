/* initech-2gva: equality, NOT a new color-value oracle (ADR-0010 CD-5).
 * The generated hardware table must agree at EVERY index with the locked
 * color_canon.json accessor. test-color-canon / test-clut independently grade
 * those locked values against the decomp binaries. No hand-kept index list.
 */
#include <stdio.h>
#include "color_canon.h"
#include "flair_dac.h"

int main(void)
{
    unsigned n = sizeof(flair_dac) / sizeof(flair_dac[0]);
    if (n != 256) {
        fprintf(stderr, "RED flair-dac count=%u expected=256\n", n);
        return 1;
    }
    for (unsigned i = 0; i < n; ++i) {
        unsigned got = (unsigned)flair_dac[i][0] << 16 |
                       (unsigned)flair_dac[i][1] << 8 | flair_dac[i][2];
        unsigned want = flair_canon_rgb((unsigned char)i);
        if (got != want) {
            fprintf(stderr, "RED flair-dac idx=%u got=#%06X expected=#%06X\n",
                    i, got, want);
            return 1;
        }
    }
    puts("GREEN test-flair-dac: all 256 generated DAC entries == locked canon");
    return 0;
}
