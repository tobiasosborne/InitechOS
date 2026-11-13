/* PUB-REF-0001 GC-01 / CP-08. Call the actual dispatcher's controlled
 * register, rather than maintaining the test-40oq mirror a second time.
 * Dead sections let this read-only probe link without kernel I/O stubs. */
#include <stdio.h>
#include "../../os/milton/int21.c"
int main(void)
{
    for (unsigned ah=0; ah<256; ++ah)
        if (ah_is_listed((uint8_t)ah)) printf("INT21:%02X\n",ah);
    return ferror(stdout)?1:0;
}
