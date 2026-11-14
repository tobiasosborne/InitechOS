/* Decode every message against the independent locked DBASE.MSG extraction.
 * The expected bytes are never read from the packed artifact table.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_assert.h"
#include "ibase/messages.h"
TEST_HARNESS();
int main(int argc, char **argv)
{
    char line[1024], got[128], tiny[3] = {'a','b','c'};
    unsigned ordinal = 1;
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
        n = ibase_message(code, got, sizeof got);
        char message[128];
        snprintf(message,sizeof message,"catalogue ordinal %u equals independent DBASE.MSG bytes",code);
        CHECK(n == strlen(tab) && !strcmp(got,tab),message);
    }
    if (ferror(f) || fclose(f) || ordinal != 152) return 2;
    CHECK(!ibase_message(152, got, sizeof got) && !got[0] &&
          !ibase_message(1, tiny, 2) && tiny[2] == 'c' && !ibase_message(1, tiny, 0),
          "message catalogue respects destination and ordinal bounds");
    return TEST_SUMMARY("test-ibase-messages");
}
