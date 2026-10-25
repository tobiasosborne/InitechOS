/* Ref: PRD 6.6; F4 review (initech-q8j); CLAUDE.md Rules 2/6/11.
 * Union merges preserve text, not registry invariants. Validate the actual
 * source files before the merge vector accepts them. Factory C only.
 */
#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define SET_PATH "os/samir/cmd/set_options.def"
#define MINT_PATH "harness/diff/dbf_diff/xbase_prog_diff/fns-mint"
#define LINE_CAP 4096
#define MAX_OPTIONS 256
/* set_cmd_hook extracts at most sizeof(opt)-1 == 31 keyword bytes. */
#define KEY_CAP 32

static void fail(const char *path, unsigned line, const char *why)
{
    fprintf(stderr, "!!! registry RED: %s:%u: %s\n", path, line, why);
    exit(1);
}

/* Read physical lines, detecting NUL and overlong input instead of allowing
 * strlen/fgets to hide an invalid suffix. All registry source is ASCII.
 */
static int read_line(FILE *f, const char *path, unsigned *number, char *line)
{
    size_t n = 0;
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == 0 || c >= 127 || (c < 32 && c != '\t' && c != '\r' && c != '\n'))
            fail(path, *number + 1, "invalid source byte");
        if (c == '\n') break;
        if (n + 1 >= LINE_CAP) fail(path, *number + 1, "line too long");
        line[n++] = (char)c;
    }
    if (ferror(f)) fail(path, *number + 1, "read failed");
    if (c == EOF && n == 0) return 0;
    line[n] = 0;
    ++*number;
    return 1;
}

static FILE *open_source(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) fail(path, 0, "cannot open source");
    return f;
}

static const char *space(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\r') ++p;
    return p;
}

static int ident_first(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

static int ident(int c)
{
    return ident_first(c) || (c >= '0' && c <= '9');
}

/* Comments are recognized only outside string literals: a keyword containing
 * comment delimiters or escapes must fail, never be rewritten into a key.
 */
static void uncomment(const char *in, char *out, int *block)
{
    int quote = 0;
    while (*in) {
        if (*block) {
            if (in[0] == '*' && in[1] == '/') { *block = 0; in += 2; }
            else ++in;
        } else if (!quote && in[0] == '/' && in[1] == '*') {
            *block = 1; in += 2; *out++ = ' ';
        } else if (!quote && in[0] == '/' && in[1] == '/') {
            break;
        } else {
            if (*in == '"') quote = !quote;
            *out++ = *in++;
        }
    }
    *out = 0;
}

static unsigned check_options(void)
{
    FILE *f = open_source(SET_PATH);
    char line[LINE_CAP], clean[LINE_CAP], keys[MAX_OPTIONS][KEY_CAP];
    unsigned number = 0, count = 0;
    int block = 0;
    while (read_line(f, SET_PATH, &number, line)) {
        const char *p, *start;
        size_t n;
        unsigned i;
        char key[KEY_CAP];
        uncomment(line, clean, &block);
        p = space(clean);
        if (!*p) continue;
        if (strncmp(p, "SET_OPT", 7)) goto malformed;
        p = space(p + 7);
        if (*p++ != '(') goto malformed;
        p = space(p);
        if (*p++ != '"') goto malformed;
        start = p;
        if (*p < 'A' || *p > 'Z') goto malformed;
        while ((*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_') ++p;
        n = (size_t)(p - start);
        if (*p++ != '"' || n >= KEY_CAP) goto malformed;
        memcpy(key, start, n); key[n] = 0;
        p = space(p);
        if (*p++ != ',') goto malformed;
        p = space(p);
        if (strncmp(p, "set_opt_", 8) || !ident_first(p[8])) goto malformed;
        p += 9;
        while (ident(*p)) ++p;
        p = space(p);
        if (*p++ != ')' || *space(p)) goto malformed;
        for (i = 0; i < count; ++i) {
            if (!strcmp(keys[i], key)) {
                fprintf(stderr, "!!! registry RED: %s:%u: duplicate SET key %s\n",
                        SET_PATH, number, key);
                exit(1);
            }
        }
        if (count == MAX_OPTIONS) fail(SET_PATH, number, "too many SET options");
        strcpy(keys[count++], key);
        continue;
malformed:
        fail(SET_PATH, number, "malformed SET_OPT row");
    }
    if (block) fail(SET_PATH, number, "unterminated comment");
    if (!count) fail(SET_PATH, number, "empty SET option table");
    if (fclose(f)) fail(SET_PATH, number, "close failed");
    return count;
}

static void path_join(char *out, size_t cap, const char *dir, const char *name)
{
    int n = snprintf(out, cap, "%s/%s", dir, name);
    if (n < 0 || (size_t)n >= cap) fail(dir, 0, "path too long");
}

static int marker(const char *dir, const char *name)
{
    struct stat st;
    char path[LINE_CAP];
    path_join(path, sizeof path, dir, name);
    if (lstat(path, &st)) {
        if (errno == ENOENT) return 0;
        fail(path, 0, "cannot inspect marker");
    }
    if (!S_ISREG(st.st_mode) || st.st_size != 0)
        fail(path, 0, "marker must be a regular empty file");
    return 1;
}

static unsigned check_markers(void)
{
    DIR *dir = opendir(MINT_PATH);
    struct dirent *entry;
    unsigned count = 0;
    if (!dir) fail(MINT_PATH, 0, "cannot open case directory");
    for (;;) {
        struct stat st;
        char path[LINE_CAP];
        int accepted, pending;
        errno = 0;
        entry = readdir(dir);
        if (!entry) {
            if (errno) fail(MINT_PATH, 0, "cannot read case directory");
            break;
        }
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        path_join(path, sizeof path, MINT_PATH, entry->d_name);
        if (lstat(path, &st)) fail(path, 0, "cannot inspect case");
        if (S_ISLNK(st.st_mode)) fail(path, 0, "case directory must not be a symlink");
        if (!S_ISDIR(st.st_mode)) continue;
        accepted = marker(path, "ACCEPTED");
        pending = marker(path, "PENDING");
        if (accepted && pending) fail(path, 0, "both ACCEPTED and PENDING markers");
        ++count; /* Neither marker is valid for evidence-only cases. */
    }
    if (closedir(dir)) fail(MINT_PATH, 0, "cannot close case directory");
    return count;
}

/* Fail closed: a future union path needs an explicit schema before the
 * merge vector can accept it, even when a glob currently matches no files.
 */
static void check_attributes(void)
{
    FILE *f = open_source(".gitattributes");
    char line[LINE_CAP];
    unsigned number = 0;
    while (read_line(f, ".gitattributes", &number, line)) {
        char *p = (char *)space(line), *path, *save, *token;
        int is_union = 0;
        if (!*p || *p == '#') continue;
        path = strtok_r(p, " \t\r", &save);
        while ((token = strtok_r(NULL, " \t\r", &save)))
            if (!strcmp(token, "merge=union")) is_union = 1;
        if (is_union && strcmp(path, SET_PATH))
            fail(".gitattributes", number, "union path has no registry validator");
    }
    if (fclose(f)) fail(".gitattributes", number, "close failed");
}

int main(int argc, char **argv)
{
    unsigned options, cases;
    if (argc != 2) {
        fprintf(stderr, "usage: %s <source root>\n", argv[0]);
        return 2;
    }
    if (chdir(argv[1])) fail(argv[1], 0, "cannot enter source root");
    check_attributes();
    options = check_options();
    cases = check_markers();
    printf(">>> registries: green (%u SET rows, %u case directories)\n", options, cases);
    return 0;
}
