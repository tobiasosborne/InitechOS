/* Factory-only deterministic fixtures and printable-text QMP key encoder.
 * Ref: MS-DOS 3.3 User's Reference pp. 50-51, 56; audit K01-K04/K15.
 * Expected disk bytes originate here, independently of the artifact. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "pattern") == 0) {
        unsigned long n = strtoul(argv[2], NULL, 10);
        for (unsigned long i = 0; i < n; ++i)
            if (putchar((int)((i * 31u + i / 512u + 7u) & 255u)) == EOF) return 1;
    } else if (argc == 3 && strcmp(argv[1], "origins") == 0) {
        /* Independent IDB1 wire reader: 8-byte header, 24-byte records,
         * kind at +0 and ASCIIZ name at +4. Finder design F1.3/F1.5.
         * Never link the artifact's codec to judge its own persistence. */
        unsigned char header[8], rec[24];
        unsigned count = 0, want = (unsigned)strtoul(argv[2], NULL, 10);
        if (fread(header, 1, 8, stdin) != 8 || memcmp(header, "IDB1\1\0", 6)) return 1;
        unsigned n = header[6] | ((unsigned)header[7] << 8);
        for (unsigned i = 0; i < n; i++) {
            if (fread(rec, 1, 24, stdin) != 24) return 1;
            if (rec[0] == 5) {
                count++;
                if (want == 1 && memcmp(rec + 4, "APPS\0", 5)) return 1;
            }
        }
        if (getchar() != EOF || count != want) return 1;
        printf("IDB1: %u retained origin records\n", count);
    } else if (argc == 2 && strcmp(argv[1], "keys") == 0) {
        int c, first = 1;
        while ((c = getchar()) != EOF) {
            const char *token = NULL;
            char letter[2] = {0, 0};
            if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
                letter[0] = (char)c; token = letter;
            } else switch (c) {
                case '\n': token = "ret"; break;
                case ' ': token = "spc"; break;
                case '.': token = "dot"; break;
                case '\\': token = "bsl"; break;
                case ':': token = "shift-semicolon"; break;
                case '*': token = "shift-8"; break;
                case '?': token = "shift-slash"; break;
                default: fprintf(stderr, "unsupported key byte %d\n", c); return 1;
            }
            printf("%s%s", first ? "" : ",", token); first = 0;
        }
        putchar('\n');
    } else return 2;
    return ferror(stdout) ? 1 : 0;
}
