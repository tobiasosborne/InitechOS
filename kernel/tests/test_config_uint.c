/* Independent arithmetic property of the existing saturating decimal parser.
 * Directive checks alone cannot distinguish overflow wrapping to0..3 because
 * FILES now refuses all those values. cfg_parse_uint's contract is saturation;
 * strtoull is a separate numeric implementation and every vector fits64bits.
 * No product hook/export is added: include the owned implementation. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "config_sys.c"
#include "test_assert.h"
TEST_HARNESS();
int main(void)
{
    const char *numbers[]={"0","5","20","255","4294967294","4294967295",
        "4294967296","4294967297","4294967298","4294967299","4294967301",
        "4294967316","42949672960","429496729600"};
    for(unsigned i=0;i<sizeof numbers/sizeof numbers[0];i++) {
        char *end;unsigned long long exact=strtoull(numbers[i],&end,10);
        uint32_t expected=exact>UINT32_MAX?UINT32_MAX:(uint32_t)exact;
        int any=0;
        uint32_t actual=cfg_parse_uint(numbers[i],strlen(numbers[i]),&any);
        CHECK(!*end && any,"whole independent decimal vector is recognized");
        CHECK(actual==expected,"decimal parser equals independent saturating64-bit conversion");
    }
    return TEST_SUMMARY("config-uint");
}
