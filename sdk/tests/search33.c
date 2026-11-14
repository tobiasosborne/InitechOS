/* Independent DOS 3.30 capture, shared with kernel/tests/test_search33.c.
 * Provenance, probe and pinned hashes: kernel/tests/fixtures/search33/README.md.
 * Exact-name eligibility and wildcard-extension results are counted separately. */
#include "tbx_host.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    unsigned char expected[896];
    if (argc != 2) return 2;
    FILE *ref = fopen(argv[1], "rb");
    if (!ref) return 2;
    if (fread(expected, 1, sizeof expected, ref) != sizeof expected || fgetc(ref) != EOF) return 2;
    fclose(ref);
    static const unsigned attrs[] = {0x20, 0x22, 0x24, 0x26, 0x10, 8, 0x16};
    tbx_host_reset();
    for (unsigned i = 0; i < 7; i++) {
        char name[4]; snprintf(name, sizeof name, "A%u", i);
        if (!tbx_host_put_file(name, "literal", 7)) return 2;
        tbx_host.files[i].attr = attrs[i];
    }
    unsigned at = 0, failures[2] = {0, 0};
    for (unsigned mask = 0; mask < 64; mask++) for (unsigned i = 0; i < 7; i++) for (unsigned wild = 0; wild < 2; wild++) {
        char path[16]; snprintf(path, sizeof path, "A%u%s", i, wild ? ".*" : "");
        tbx_find_rec_t records[8];
        int want = expected[at++] == 0 ? 1 : 0;
        int got = tbx_file_find(path, mask, records, 8);
        char name[4]; snprintf(name, sizeof name, "A%u", i);
        if (got != want || (got == 1 && (strcmp(records[0].name, name) || records[0].attr != attrs[i]))) {
            fprintf(stderr, "search33 %s mask=%02x attr=%02x expected=%d got=%d\n", path, mask, attrs[i], want, got);
            failures[wild]++;
        }
    }
    printf("SDK search33: exact 448 checks, %u failures; wildcard 448 checks, %u failures\n", failures[0], failures[1]);
    tbx_host_reset();
    return failures[0] || failures[1];
}
