/* Independent fixed-address bootstrap contract: boot/README.md references.
 * No product source or generated expected values are included. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_assert.h"
TEST_HARNESS();
#define ASSERT(c) do { int ok = !!(c); CHECK(ok, #c); if (!ok) exit(1); } while (0)

static unsigned char bytes[8193];
static size_t read_artifact(const char *path) {
    FILE *f = fopen(path, "rb");
    ASSERT(f != NULL);
    size_t n = fread(bytes, 1, sizeof bytes, f);
    ASSERT(!ferror(f));
    ASSERT(fclose(f) == 0);
    return n;
}
static int contains(size_t n, const unsigned char *needle, size_t len) {
    for (size_t i = 0; i + len <= n; i++)
        if (!memcmp(bytes + i, needle, len)) return 1;
    return 0;
}
int main(int argc, char **argv) {
    ASSERT(argc == 4);
    size_t n = read_artifact(argv[1]);
    ASSERT(n == 512);
    ASSERT(bytes[510] == 0x55 && bytes[511] == 0xaa);
    /* INT13 ES:BX destination and immediate far handoff must agree on 8000h. */
    const unsigned char dest[] = {0xbb, 0x00, 0x80};
    const unsigned char jump[] = {0xea, 0x00, 0x80, 0x00, 0x00};
    ASSERT(contains(n, dest, sizeof dest));
    ASSERT(contains(n, jump, sizeof jump));
    for (size_t i = 446; i < 510; i++) ASSERT(bytes[i] == 0);
    ASSERT(read_artifact(argv[2]) == 8192);
    n = read_artifact(argv[3]);
    ASSERT(n > 0 && n <= 4096);
    /* The flat kernel entry is a 32-bit far jump to selector 08:00500000. */
    const unsigned char kernel[] = {0xea, 0x00, 0x00, 0x50, 0x00, 0x08, 0x00};
    ASSERT(contains(n, kernel, sizeof kernel));
    return TEST_SUMMARY("layout");
}
