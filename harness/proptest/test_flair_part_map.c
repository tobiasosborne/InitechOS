/* harness/proptest/test_flair_part_map.c -- the PART resolution table, before
 * and after the round-5 storage compaction of flair_part_map (WL-0168).
 *
 * Prints every PART (plus both out-of-range neighbours) resolved at every
 * destination depth through flair_look_pixel_depth, and through
 * flair_look_pixel_for_skin for each of the three registry skins. The Makefile
 * diffs the transcript against harness/proptest/fixtures/flair_part_map.golden,
 * which was captured ONCE from the pre-change flair_look.c (the 8-byte-row
 * map, commit 4d907b1f) -- a differential against the original implementation,
 * not a value recomputed from the new table (Law 2). The colour authority
 * itself stays graded by test-color-canon-gen / the pixel gates.
 */
#include <stdio.h>
#include <string.h>

#include "flair_look.h"
#include "flair_skins.h"

int main(void)
{
    static const uint32_t bpps[5] = { 8u, 15u, 16u, 24u, 32u };
    static const int skins[3][2] = { { ERA_SYS7_0_1, HERITAGE_QUICKDRAW },
                                     { ERA_WIN31, HERITAGE_GDI },
                                     { ERA_SYS8_PLATINUM, HERITAGE_QUICKDRAW } };
    GrafPort port;

    memset(&port, 0, sizeof port);
    for (int part = -1; part <= (int)FLAIR_PART__COUNT; part++) {
        for (unsigned b = 0; b < 5u; b++) {
            printf("part=%d bpp=%u depth=%08X", part, bpps[b],
                   flair_look_pixel_depth(bpps[b], part));
            port.portBits.bm.bpp = bpps[b];
            for (unsigned k = 0; k < 3u; k++) {
                const flair_skin_t *skin = flair_skin_resolve(skins[k][0], skins[k][1]);
                if (part >= 0 && part < (int)FLAIR_PART__COUNT)
                    printf(" skin%u=%08X", k, flair_look_pixel_for_skin(&port, skin, part));
            }
            printf("\n");
        }
    }
    return 0;
}
