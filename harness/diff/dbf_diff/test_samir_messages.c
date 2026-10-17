/* Decode every message against the independent locked DBASE.MSG extraction.
 * The expected bytes are never read from the packed artifact table.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "samir/messages.h"
int main(int argc, char **argv)
{
    char line[1024], got[128], tiny[3] = {'a','b','c'};
    unsigned ordinal = 1, failures = 0;
    FILE *f;
    if (argc != 2 || !(f = fopen(argv[1], "rb"))) return 2;
    while (fgets(line, sizeof line, f)) {
        char *tab;
        unsigned code, n;
        if (line[0] == '#' || !strncmp(line, "code\t", 5)) continue;
        tab = strchr(line, '\t');
        if (!tab) return 2;
        code = (unsigned)strtoul(line, NULL, 10);
        if (code != ordinal++) return 2;
        tab++;
        tab[strcspn(tab, "\r\n")] = 0;
        n = samir_message(code, got, sizeof got);
        if (n != strlen(tab) || strcmp(got, tab)) {
            fprintf(stderr, "RED SAMIR_MSG_PAIR: catalogue ordinal %u differs from DBASE.MSG\n", code);
            failures++;
        }
    }
    if (ferror(f) || fclose(f) || ordinal != 152) return 2;
    if (samir_message(152, got, sizeof got) || got[0] ||
        samir_message(1, tiny, 2) || tiny[2] != 'c' || samir_message(1, tiny, 0)) {
        fputs("RED SAMIR_MSG_PAIR: bounds\n", stderr);
        failures++;
    }
    if (!failures) puts("samir-messages: all 151 ordinals byte-identical to DBASE.MSG; bounds green");
    return failures ? 1 : 0;
}
