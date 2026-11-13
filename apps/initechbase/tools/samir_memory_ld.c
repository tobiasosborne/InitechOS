/* Factory: DEC-03a.4/.8, initech-fxolx. One contract feeds ld, emitter, PAL.
 * Write deterministic symbol assignments; no paths/timestamps in artifact. */
#include <stdio.h>
#include "memory_map.h"
#include "samir/memory_contract.h"
int main(void)
{
    printf("SAMIR_DECLARED_RESERVE = %u;\nSAMIR_MODULE_CEILING = %u;\n"
           "SAMIR_NATIVE_METADATA = %u;\nSAMIR_GUARANTEED_HEAP = %u;\n"
           "SAMIR_HIGH_WINDOW = %u;\nSAMIR_PAL_REGION1 = %u;\n",
           SAMIR_LOADER_RESERVE,SAMIR_MODULE_MAX,INITECHMZ_NATIVE_HEADER_BYTES,
           SAMIR_MINIMUM_HEAP_BYTES,EXT_APP_GUARD_BASE-EXT_APP_BASE,SAMIR_PAL_REGION1_BYTES);
    return ferror(stdout) ? 1 : 0;
}
