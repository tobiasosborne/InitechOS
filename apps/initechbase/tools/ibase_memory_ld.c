/* Factory: DEC-03a.4/.8, initech-fxolx. One contract feeds ld, emitter, PAL.
 * Write deterministic symbol assignments; no paths/timestamps in artifact. */
#include <stdio.h>
#include <string.h>
#include "memory_map.h"
#include "ibase/memory_contract.h"
int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--mz-args") == 0) {
        printf("--extended --reserve %u --minimum-heap %u --minalloc %u\n",
               IBASE_LOADER_RESERVE, IBASE_MINIMUM_HEAP_BYTES, IBASE_MINALLOC_PARAS);
        return ferror(stdout) ? 1 : 0;
    }
    if (argc != 1) return 1;
    printf("IBASE_DECLARED_RESERVE = %u;\nIBASE_MODULE_CEILING = %u;\n"
           "IBASE_NATIVE_METADATA = %u;\nIBASE_GUARANTEED_HEAP = %u;\n"
           "IBASE_HIGH_WINDOW = %u;\nIBASE_PAL_REGION1 = %u;\n",
           IBASE_LOADER_RESERVE,IBASE_MODULE_MAX,INITECHMZ_NATIVE_HEADER_BYTES,
           IBASE_MINIMUM_HEAP_BYTES,EXT_APP_GUARD_BASE-EXT_APP_BASE,IBASE_PAL_REGION1_BYTES);
    return ferror(stdout) ? 1 : 0;
}
