/* Factory size oracle. ADR-0009 DEC-03/04/05, samir.ld and memory_map.h:
 * loaded bytes + 64 KiB BSS + 0x4f70 PAL allocation + 0x20 MCB/alignment
 * must end before ENV_BLOCK. The PM additionally requires 4096 B headroom.
 * Operator ruling 2026-10-24 (initech-9swot; WL-0182) retires the lane's
 * arrival-recovery clause; the physical PAL-memory budget stays unchanged.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "memory_map.h"

enum { HEADROOM = 4096,
       BUDGET = ENV_BLOCK - PROGRAM_IMAGE - PROGRAM_BSS_RESERVE - 0x4f70 - 0x20 };

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
    if (st.st_size > BUDGET - HEADROOM) {
        fprintf(stderr, "RED SAMIR_SIZE: %ld B > %d B headroom cap\n",
                (long)st.st_size, BUDGET - HEADROOM);
        return 1;
    }
    printf("samir-size: %ld B; headroom %ld B (budget %d B)\n",
           (long)st.st_size,
           BUDGET - (long)st.st_size, BUDGET);
    return 0;
}
