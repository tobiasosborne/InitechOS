/* Compare the retained REALDOS report with installed-volume output. FCB text
 * records are padded to 128 bytes with Ctrl-Z. Only the directory census is
 * different: the product ships four tables, the original capture shipped one.
 * Assert that exact census, and compare every other meaningful byte. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static size_t load(const char *path, unsigned char *b)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    size_t n = fread(b, 1, 8192, f);
    if (ferror(f) || !feof(f) || !n || n % 128) exit(1);
    fclose(f);
    size_t end = n;
    while (end && b[end-1] == 26) end--;
    if (memchr(b, 26, end)) exit(1);
    b[end] = 0;
    return end;
}
int main(int argc, char **argv)
{
    unsigned char reference[8193], actual[8193], expected[8193];
    const char from[] = "TABLES ON FILE ....... TPS.DBF\r\n";
    const char to[] = "TABLES ON FILE ....... CLIENTS.DBF, INVOICE.DBF, PARTS.DBF, TPS.DBF\r\n";
    if (argc != 3) return 1;
    size_t rn = load(argv[1], reference), an = load(argv[2], actual);
    unsigned char *census = (unsigned char *)strstr((char *)reference, from);
    if (!census) return 1;
    size_t prefix = (size_t)(census - reference), fn = sizeof from - 1, tn = sizeof to - 1;
    memcpy(expected, reference, prefix);
    memcpy(expected + prefix, to, tn);
    memcpy(expected + prefix + tn, census + fn, rn - prefix - fn);
    size_t en = rn - fn + tn;
    if (an != en || memcmp(actual, expected, en)) {
        fprintf(stderr, "FAIL: installed TPS report differs from retained real-DOS report\n");
        return 1;
    }
    puts("ok TPS: all real-DOS report bytes; exact installed four-table census; FCB padding validated");
    return 0;
}
