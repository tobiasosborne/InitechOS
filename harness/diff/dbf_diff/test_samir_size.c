/* Factory size oracle. ADR-0009 DEC-03/04/05, samir.ld and memory_map.h:
 * loaded bytes + 64 KiB BSS + 0x4f70 PAL allocation + 0x20 MCB/alignment
 * must end before ENV_BLOCK. The PM additionally requires 4096 B headroom
 * and 8192 B recovery against the measured 106367 B arrival image.
 * This fixed arrival measurement is independent of the image under test.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "memory_map.h"

enum { ARRIVAL = 106367, RECOVER = 8192, HEADROOM = 4096,
       BUDGET = ENV_BLOCK - PROGRAM_IMAGE - PROGRAM_BSS_RESERVE - 0x4f70 - 0x20,
       LIMIT = ARRIVAL - RECOVER };

int main(int argc, char **argv)
{
    struct stat st;
    if (argc != 2 || stat(argv[1], &st) || st.st_size <= 0) {
        fputs("samir-size: missing/nonempty image required\n", stderr);
        return 2;
    }
    if (BUDGET != 106352) {
        fputs("samir-size: memory contract changed; review ADR-0009 arithmetic\n", stderr);
        return 2;
    }
    if (st.st_size > BUDGET - HEADROOM || st.st_size > LIMIT) {
        fprintf(stderr, "RED SAMIR_SIZE: %ld B > %d B reclaim cap / %d B headroom cap\n",
                (long)st.st_size, LIMIT, BUDGET - HEADROOM);
        return 1;
    }
    printf("samir-size: %ld B; reclaimed %ld B; headroom %ld B (budget %d B)\n",
           (long)st.st_size, ARRIVAL - (long)st.st_size,
           BUDGET - (long)st.st_size, BUDGET);
    return 0;
}
