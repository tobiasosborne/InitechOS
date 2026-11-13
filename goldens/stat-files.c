/* Factory cache key: hashes, resolved paths and nanosecond file identity.
 * ctime catches content edits even when a caller restores size and mtime. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <inttypes.h>
int main(void)
{
    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    while ((len = getline(&line, &cap, stdin)) >= 0) {
        struct stat st;
        if (len < 67 || line[64] != ' ' || line[65] != ' ') return 1;
        line[len - 1] = '\0';
        if (stat(line + 66, &st) || !S_ISREG(st.st_mode)) {
            fprintf(stderr, "FAIL corpus: missing regular file %s\n", line + 66);
            free(line);
            return 1;
        }
        printf("%s %ju %ju %ju %jd.%09ld %jd.%09ld\n", line,
               (uintmax_t)st.st_dev, (uintmax_t)st.st_ino, (uintmax_t)st.st_size,
               (intmax_t)st.st_mtim.tv_sec, st.st_mtim.tv_nsec,
               (intmax_t)st.st_ctim.tv_sec, st.st_ctim.tv_nsec);
    }
    free(line);
    return ferror(stdin) || ferror(stdout);
}
