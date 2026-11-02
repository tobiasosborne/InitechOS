/* Independent DOS 3.30 captures, not values computed by the artifact.
 * Ref: README.txt / DEC-19.3 pending-ratification deviation; initech-i15. */
#include <stdint.h>
#include <stdio.h>

static unsigned word(const unsigned char *p)
{
    return p[0] | ((unsigned)p[1] << 8);
}

static int check(const char *path, int equal)
{
    unsigned char b[12];
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return 1; }
    int valid = fread(b, 1, sizeof b, f) == sizeof b && fgetc(f) == EOF;
    fclose(f);
    if (!valid) { fprintf(stderr, "bad capture length: %s\n", path); return 1; }
    unsigned a = word(b + 4), factor = equal ? a : word(b + 6);
    unsigned expect_a = equal ? (0x0010u + factor) & 0xFFFFu : 0x0010u;
    unsigned expect_b = equal ? 0x1111u : (0x1111u + factor) & 0xFFFFu;
#ifdef MUTATE_EXPECT_LOAD_PATCH
    /* Rule 6: falsely expect the InitechDOS load-based fixup location. */
    expect_a = (0x0010u + factor) & 0xFFFFu;
    expect_b = 0x1111u;
#endif
    if (word(b) != 0u || a == word(b + 6) ||
        word(b + 8) != expect_a || word(b + 10) != expect_b) {
        fprintf(stderr, "FAIL DOS33 %s-factor fixup address\n", equal ? "equal" : "different");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    int failed = check(argv[1], 0) | check(argv[2], 1);
    if (!failed) puts("DOS33 overlay captures: factor-based patch address confirmed");
    return failed;
}
