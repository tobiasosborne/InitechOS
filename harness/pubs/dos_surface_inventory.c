/* PUB-REF-0001 GC-01 (bead initech-ipu5.7 integration): the batch
 * subcommands and CONFIG.SYS directives of the shipped build, asked of the
 * ACTUAL classifiers (batch_classify, config_sys_parse) rather than copied
 * into a second list.  Candidates are read from stdin, one word per line --
 * the Makefile feeds every quoted upper-case word of batch.c and
 * config_sys.c -- and a word is shipped iff the real code accepts it:
 *   NAME|batch   batch_classify("NAME X") is a directive, not BL_COMMAND,
 *                BL_BLANK or BL_LABEL
 *   NAME|config  config_sys_parse("NAME=<value>") recognizes the line for
 *                one of the probe values (ON, 20, A, X.SYS)
 * Both sources are pure (no I/O), so this host probe includes them whole,
 * the same single-TU pattern as harness/pubs/int21_inventory.c. */
#include <stdio.h>
#include <string.h>
#include "../../os/milton/batch.c"
#include "../../os/milton/config_sys.c"
int main(void)
{
    static const char *const vals[] = { "ON", "20", "A", "X.SYS" };
    char word[64], line[160];
    while (fgets(word, sizeof word, stdin)) {
        word[strcspn(word, "\r\n")] = '\0';
        if (!word[0]) continue;
        batch_parsed_t bp;
        snprintf(line, sizeof line, "%s X", word);
        batch_line_kind_t k = batch_classify(line, &bp);
        if (k != BL_COMMAND && k != BL_BLANK && k != BL_LABEL)
            printf("%s|batch\n", word);
        for (size_t i = 0; i < sizeof vals / sizeof vals[0]; i++) {
            dos_config_t cfg;
            int n = snprintf(line, sizeof line, "%s=%s\r\n", word, vals[i]);
            if (config_sys_parse(line, (uint32_t)n, &cfg) > 0) {
                printf("%s|config\n", word);
                break;
            }
        }
    }
    return ferror(stdout) ? 1 : 0;
}
