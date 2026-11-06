/* Locked gate selection: initech-fw3x0; PRD Sec 8/9/14, CLAUDE.md Rules 1/6/8.
 * Schema: {"families": {name: {"gates": [glob, ...], "paths": [glob, ...]}}}.
 * Gate globs use fnmatch; path * stays in one component, ** crosses slashes.
 * Input paths are newline records (git diff --no-renames --name-only).
 * No expected gate inventory is embedded here: make -pq supplies it.
 */
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <fnmatch.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { char **v; size_t n; } Strings;
typedef struct {
    char *name;
    Strings gates, paths;
    size_t begin, end, gates_begin;
    int selected;
} Family;
static Family *families;
static size_t nf, pos, length;
static char *json;
static unsigned errors;

static void die(const char *fmt, ...)
{
    va_list ap;
    fputs("gate-map RED: ", stderr);
    va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
    fputc('\n', stderr); exit(2);
}
static void red(const char *fmt, ...)
{
    va_list ap;
    fputs("gate-map RED: ", stderr);
    va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
    fputc('\n', stderr); errors++;
}
static void *resize(void *p, size_t n)
{
    void *q = realloc(p, n);
    if (!q) die("out of memory");
    return q;
}
static void add(Strings *s, char *v)
{
    s->v = resize(s->v, (s->n + 1) * sizeof *s->v);
    s->v[s->n++] = v;
}
static void ws(void)
{
    while (pos < length && json[pos] && strchr(" \t\r\n", json[pos])) pos++;
}
static int take(char c)
{
    ws();
    if (pos < length && json[pos] == c) { pos++; return 1; }
    return 0;
}
static void need(char c)
{
    if (!take(c)) die("malformed JSON at byte %zu: expected '%c'", pos, c);
}
static char *string(void)
{
    char *s;
    size_t n = 0, cap = 64;
    need('"');
    s = resize(NULL, cap);
    while (pos < length) {
        unsigned char c = (unsigned char)json[pos++];
        if (c == '"') { s[n] = 0; return s; }
        if (c == '\\') {
            unsigned v = 0;
            if (pos == length) break;
            c = (unsigned char)json[pos++];
            switch (c) {
            case '"': case '\\': case '/': break;
            case 'b': c = '\b'; break;
            case 'f': c = '\f'; break;
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            case 'u':
                for (unsigned i = 0; i < 4; i++) {
                    unsigned char h;
                    if (pos == length) die("truncated JSON unicode escape");
                    h = (unsigned char)json[pos++];
                    if (!isxdigit(h)) die("invalid JSON unicode escape");
                    v = v * 16 + (isdigit(h) ? (unsigned)(h - '0') : (unsigned)tolower(h) - 'a' + 10);
                }
                if (v < 32 || v > 126) die("only printable ASCII strings are allowed");
                c = (unsigned char)v; break;
            default: die("invalid JSON escape at byte %zu", pos - 1);
            }
        }
        if (c < 32 || c > 126) die("only printable ASCII strings are allowed");
        if (n + 1 == cap) { cap *= 2; s = resize(s, cap); }
        s[n++] = (char)c;
    }
    die("unterminated JSON string"); return NULL;
}
static Strings array(void)
{
    Strings s = {0};
    need('[');
    if (take(']')) return s;
    do {
        char *v = string();
        if (!*v) die("empty glob at byte %zu", pos);
        add(&s, v);
        if (take(']')) return s;
        need(',');
    } while (1);
}
static void load_map(const char *path)
{
    FILE *f = fopen(path, "rb");
    long size;
    char *key;
    if (!f) die("open %s: %s", path, strerror(errno));
    if (fseek(f, 0, SEEK_END) || (size = ftell(f)) < 0 || size > 8 * 1024 * 1024 ||
        fseek(f, 0, SEEK_SET)) die("cannot size JSON %s (limit 8 MiB)", path);
    length = (size_t)size; json = resize(NULL, length + 1);
    if (fread(json, 1, length, f) != length || fclose(f)) die("read JSON %s", path);
    json[length] = 0;
    need('{'); key = string();
    if (strcmp(key, "families")) die("expected sole root member 'families'");
    free(key); need(':'); need('{');
    if (!take('}')) do {
        Family row = {0};
        unsigned seen = 0;
        ws(); row.begin = pos; row.name = string();
        if (!*row.name) die("empty family name");
        for (size_t i = 0; i < nf; i++)
            if (!strcmp(families[i].name, row.name)) die("duplicate family %s", row.name);
        need(':'); need('{');
        do {
            unsigned bit;
            key = string(); need(':');
            if (!strcmp(key, "gates")) bit = 1;
            else if (!strcmp(key, "paths")) bit = 2;
            else die("unknown member %s in family %s", key, row.name);
            if (seen & bit) die("duplicate member %s in family %s", key, row.name);
            seen |= bit; free(key);
            if (bit == 1) { ws(); row.gates_begin = pos; row.gates = array(); }
            else row.paths = array();
            if (take('}')) break;
            need(',');
        } while (1);
        if (seen != 3) die("family %s needs gates and paths", row.name);
        row.end = pos;
        families = resize(families, (nf + 1) * sizeof *families);
        families[nf++] = row;
        if (take('}')) break;
        need(',');
    } while (1);
    need('}'); ws();
    if (pos != length) die("trailing JSON at byte %zu", pos);
}
/* Expand ** only at path-component boundaries. All other components use
 * POSIX fnmatch with FNM_PATHNAME (including ?, [] and escaped literals). */
static int path_match(const char *pattern, const char *path)
{
    const char *p = pattern, *star = NULL;
    while ((p = strstr(p, "**")) != NULL) {
        if ((p == pattern || p[-1] == '/') && (p[2] == '/' || p[2] == 0)) { star = p; break; }
        p += 2;
    }
    if (star) {
        size_t prefix = (size_t)(star - pattern);
        char *head = resize(NULL, prefix + 1);
        int found = 0;
        memcpy(head, pattern, prefix); head[prefix] = 0;
        for (size_t i = 0; i <= strlen(path); i++) {
            char *part;
            if (i && path[i - 1] != '/') continue;
            part = resize(NULL, i + 1); memcpy(part, path, i); part[i] = 0;
            if (fnmatch(head, part, FNM_PATHNAME) == 0) {
                const char *tail = path + i;
                if (!star[2]) found = 1;
                else do {
                    if (path_match(star + 3, tail)) { found = 1; break; }
                    tail = strchr(tail, '/');
                    if (tail) tail++;
                } while (tail);
            }
            free(part);
            if (found) break;
        }
        free(head); return found;
    }
    return fnmatch(pattern, path, FNM_PATHNAME) == 0;
}
static Strings lines(FILE *f, int gate_names)
{
    Strings s = {0};
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;
    while ((n = getline(&line, &cap, f)) >= 0) {
        if (n && line[n - 1] == '\n') line[--n] = 0;
        if (!n) die("empty input record");
        if ((size_t)n != strlen(line)) die("NUL in input record");
        for (ssize_t i = 0; i < n; i++)
            if ((unsigned char)line[i] < 32 || (unsigned char)line[i] > 126)
                die("non-printable/non-ASCII input record");
        if (gate_names) {
            if (strncmp(line, "test-", 5)) die("invalid gate name %s", line);
            for (ssize_t i = 5; i < n; i++)
                if (!isalnum((unsigned char)line[i]) && !strchr("_.+-", line[i]))
                    die("unsafe gate name %s", line);
        } else {
            if (line[0] == '/' || !strcmp(line, "..") || !strncmp(line, "../", 3) ||
                strstr(line, "/../") || strstr(line, "//") || strstr(line, "/./") ||
                (n >= 3 && !strcmp(line + n - 3, "/..")) ||
                !strncmp(line, "./", 2)) die("non-canonical repo path %s", line);
        }
        add(&s, strdup(line));
        if (!s.v[s.n - 1]) die("out of memory");
    }
    free(line);
    if (ferror(f)) die("error reading input records");
    return s;
}
static int cmp(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}
static int covered_path(const char *p)
{
    const char *dirs[] = {"os/", "harness/", "spec/", "seed/", "tools/", "pubs/"};
    size_t n = strlen(p);
    if (!strcmp(p, "Makefile") || (n >= 3 && !strcmp(p + n - 3, ".mk"))) return 1;
    for (size_t i = 0; i < sizeof dirs / sizeof dirs[0]; i++)
        if (!strncmp(p, dirs[i], strlen(dirs[i]))) return 1;
    return 0;
}
static int has_gate(const Strings *inventory, const char *name)
{
    return bsearch(&name, inventory->v, inventory->n, sizeof *inventory->v, cmp) != NULL;
}
static void validate(const Strings *inventory, int *owners)
{
    size_t unmapped = 0;
    int always = 0;
    const char *mandatory[] = {"test-makefile-vars", "test-gate-honesty", "test-gate-map"};
    for (size_t f = 0; f < nf; f++) {
        Family *row = &families[f];
        size_t hits = 0;
        row->selected = !strcmp(row->name, "always");
        if (row->selected) always++;
        for (size_t p = 0; p < row->gates.n; p++) {
            size_t matched = 0;
            for (size_t g = 0; g < inventory->n; g++) {
                if (fnmatch(row->gates.v[p], inventory->v[g], 0)) continue;
                matched++; hits++;
                if (owners[g] >= 0) red("gate %s listed twice (%s, %s)", inventory->v[g],
                    families[owners[g]].name, row->name);
                else owners[g] = (int)f;
            }
            if (!matched) red("unknown gate/glob %s in family %s", row->gates.v[p], row->name);
        }
        if (!hits) red("family %s has zero matching gates", row->name);
    }
    for (size_t g = 0; g < inventory->n; g++) if (owners[g] < 0) {
        red("unmapped gate %s", inventory->v[g]); unmapped++;
    }
    if (unmapped) fprintf(stderr, "gate-map RED: %zu unmapped gates\n", unmapped);
    if (always != 1) red("missing family always");
    for (size_t m = 0; m < sizeof mandatory / sizeof mandatory[0]; m++) {
        int ok = 0;
        for (size_t g = 0; g < inventory->n; g++)
            if (!strcmp(inventory->v[g], mandatory[m]) && owners[g] >= 0 &&
                !strcmp(families[owners[g]].name, "always")) ok = 1;
        if (!ok) red("%s must belong to always", mandatory[m]);
    }
    {
        const char *repro[] = {"test-kernel-repro", "test-kernel-flair-repro", "test-ovl-repro"};
        for (size_t m = 0; m < sizeof repro / sizeof repro[0]; m++) {
            int ok = 0;
            for (size_t g = 0; g < inventory->n; g++)
                if (!strcmp(inventory->v[g], repro[m]) && owners[g] >= 0 &&
                    !strcmp(families[owners[g]].name, "build-system")) ok = 1;
            if (!ok) red("%s must belong to build-system", repro[m]);
        }
    }
    /* Directly associated mutant/emulator/Bochs legs inherit one owner. */
    for (size_t g = 0; g < inventory->n; g++) {
        const char *suffixes[] = {"-mutant", "-mutants", "-emu", "-bochs"};
        for (size_t s = 0; s < sizeof suffixes / sizeof suffixes[0]; s++) {
            size_t n = strlen(inventory->v[g]), k = strlen(suffixes[s]);
            char *base;
            if (n <= k || strcmp(inventory->v[g] + n - k, suffixes[s])) continue;
            base = strdup(inventory->v[g]); if (!base) die("out of memory");
            base[n - k] = 0;
            if (has_gate(inventory, base)) {
                char **b = bsearch(&base, inventory->v, inventory->n, sizeof *inventory->v, cmp);
                size_t i = (size_t)(b - inventory->v);
                if (owners[g] >= 0 && owners[i] >= 0 && owners[g] != owners[i])
                    red("twins %s and %s belong to different families", base, inventory->v[g]);
            }
            free(base);
        }
    }
}
static void mutant(const char *kind)
{
    /* Mutate parsed row spans, preserving every other byte of locked data. */
    if (!nf) die("cannot mutate an empty map");
    if (!strcmp(kind, "drop-family")) {
        size_t end = families[0].end;
        while (end < length && strchr(" \r\n\t", json[end])) end++;
        if (json[end] == ',') end++;
        fwrite(json, 1, families[0].begin, stdout);
        fwrite(json + end, 1, length - end, stdout);
    } else if (!strcmp(kind, "duplicate-gate")) {
        size_t at = families[0].gates_begin;
        if (!families[0].gates.n) die("cannot duplicate empty gates");
        fwrite(json, 1, at + 1, stdout);
        putchar('"');
        for (const char *p = families[0].gates.v[0]; *p; p++) {
            if (*p == '"' || *p == '\\') putchar('\\');
            putchar(*p);
        }
        fputs("\",", stdout);
        fwrite(json + at + 1, 1, length - at - 1, stdout);
    } else die("unknown mutant %s", kind);
}
int main(int argc, char **argv)
{
    Strings inventory, paths;
    FILE *f;
    int *owners;
    int audit;
    if (argc == 4 && !strcmp(argv[1], "--mutate")) {
        load_map(argv[3]); mutant(argv[2]); return ferror(stdout) ? 2 : 0;
    }
    if (argc != 4 || (strcmp(argv[1], "--audit") && strcmp(argv[1], "--resolve")))
        die("usage: gate_map --audit|--resolve MAP INVENTORY < paths; --mutate KIND MAP");
    audit = !strcmp(argv[1], "--audit"); load_map(argv[2]);
    f = fopen(argv[3], "r"); if (!f) die("open inventory %s: %s", argv[3], strerror(errno));
    inventory = lines(f, 1); if (fclose(f)) die("close inventory");
    if (!inventory.n) die("empty gate inventory");
    qsort(inventory.v, inventory.n, sizeof *inventory.v, cmp);
    for (size_t i = 1; i < inventory.n; i++)
        if (!strcmp(inventory.v[i - 1], inventory.v[i])) die("duplicate inventory gate %s", inventory.v[i]);
    owners = resize(NULL, inventory.n * sizeof *owners);
    for (size_t g = 0; g < inventory.n; g++) owners[g] = -1;
    validate(&inventory, owners);
    paths = lines(stdin, 0);
    for (size_t p = 0; p < paths.n; p++) {
        int matched = 0, build_system = 0;
        size_t n = strlen(paths.v[p]);
        int needs_repro = !strcmp(paths.v[p], "Makefile") ||
            (n >= 3 && !strcmp(paths.v[p] + n - 3, ".mk")) ||
            !strncmp(paths.v[p], "tools/", 6) || !strncmp(paths.v[p], "os/milton/", 10) ||
            !strncmp(paths.v[p], "os/flair/", 9);
        for (size_t fidx = 0; fidx < nf; fidx++)
            for (size_t k = 0; k < families[fidx].paths.n; k++)
                if (path_match(families[fidx].paths.v[k], paths.v[p])) {
                    if (!strcmp(families[fidx].name, "build-system")) build_system = 1;
                    families[fidx].selected = 1; matched = 1; break;
                }
        if (!matched && covered_path(paths.v[p])) red("unmapped path %s", paths.v[p]);
        if (needs_repro && !build_system) red("path %s must trigger build-system", paths.v[p]);
    }
    if (errors) return 1;
    if (audit) {
        printf(">>> test-gate-map: green (%zu gates, %zu families, %zu tracked paths)\n",
               inventory.n, nf, paths.n);
    } else {
        size_t selected = 0;
        for (size_t g = 0; g < inventory.n; g++) if (families[owners[g]].selected) {
            puts(inventory.v[g]); selected++;
        }
        if (!selected) die("empty derived gate list");
    }
    return ferror(stdout) ? 2 : 0;
}
