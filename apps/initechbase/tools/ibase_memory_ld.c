/* Factory: DEC-03a.4/.8, initech-fxolx. One contract feeds ld, emitter, PAL.
 * Write deterministic symbol assignments; no paths/timestamps in artifact. */
#include <stdio.h>
#include "memory_map.h"
#include "ibase/memory_contract.h"
int main(void)
{
    printf("IBASE_DECLARED_RESERVE = %u;\nIBASE_MODULE_CEILING = %u;\n"
           "IBASE_NATIVE_METADATA = %u;\nIBASE_GUARANTEED_HEAP = %u;\n"
           "IBASE_HIGH_WINDOW = %u;\nIBASE_PAL_REGION1 = %u;\n",
           IBASE_LOADER_RESERVE,IBASE_MODULE_MAX,INITECHMZ_NATIVE_HEADER_BYTES,
           IBASE_MINIMUM_HEAP_BYTES,EXT_APP_GUARD_BASE-EXT_APP_BASE,IBASE_PAL_REGION1_BYTES);
    return ferror(stdout) ? 1 : 0;
}
