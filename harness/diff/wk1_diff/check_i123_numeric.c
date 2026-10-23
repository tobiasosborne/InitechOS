/* Independent stopped-image WK1 checker. Does not link the worksheet code.
 * WK1 0x000e NUMBER, 0x0010 FORMULA: corpus wk1-format.md Sec 4.
 * The trace pins 09-Mar-1992 noon, waits two guest seconds, then F9.
 * Its kernel pump is bounded to 30 guest seconds, so both snapshots must
 * remain within that known interval and advance by at least two seconds.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static unsigned rd(const uint8_t *b) { return b[0] | (unsigned)b[1] << 8; }
static int read_now(const char *path, double *now)
{
    FILE *f = fopen(path, "rb"); uint8_t h[4], b[65536];
    unsigned number = 0, formula = 0, eof = 0;
    if (!f) return 0;
    while (fread(h, 1, 4, f) == 4) {
        unsigned op = rd(h), len = rd(h + 2);
        if (fread(b, 1, len, f) != len) break;
        if (op == 1 && len == 0) { eof = 1; break; }
        if (op == 14 && len == 13 && rd(b + 1) == 0 && rd(b + 3) == 0) {
            double x; memcpy(&x, b + 5, 8);
            number = b[0] == 0xff && x == 12.34;
        }
        if (op == 16 && len == 17 && rd(b + 1) == 1 && rd(b + 3) == 0 &&
            rd(b + 13) == 2 && b[15] == 55 && b[16] == 3) {
            memcpy(now, b + 5, 8); formula = 1;
        }
    }
    int end = fgetc(f); fclose(f);
    return eof && end == EOF && number && formula;
}
int main(int argc, char **argv)
{
    double a = 0, b = 0, base = 33672.5;
    if (argc != 3) return 2;
    if (!read_now(argv[1], &a) || !read_now(argv[2], &b) ||
        !(a >= base && b < base + 30.0 / 86400.0 && b >= a)) {
        fputs("FAIL numeric WK1: currency number/default format or NOW date/cache/bytecode\n", stderr); return 1;
    }
    int sa = (int)((a - base) * 86400 + .5), sb = (int)((b - base) * 86400 + .5);
    if (sb - sa < 2) { fputs("FAIL numeric WK1: F9 did not refresh after two guest seconds\n", stderr); return 1; }
    printf("numeric WK1: $12.34 -> NUMBER/default; NOW seconds %d -> %d after F9\n", sa, sb);
    return 0;
}
