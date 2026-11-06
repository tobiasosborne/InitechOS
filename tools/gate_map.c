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
        if (!strcmp(row.name, "pending") && row.paths.n)
            die("pending family cannot have automatic path triggers");
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
        /* Pending oracles are opt-in, like the E1 lane's pending family.
         * Keep ownership audited but forbid hiding a live gate here. */
        if (!strcmp(row->name, "pending")) {
            if (row->paths.n) red("pending family must have no path triggers");
            for (size_t p = 0; p < row->gates.n; p++) {
                const char *gate = row->gates.v[p];
                size_t n = strlen(gate);
                if (n < 8 || strcmp(gate + n - 8, "-pending"))
                    red("active gate %s in opt-in pending family", gate);
            }
        }
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
/* Consumer evidence comes from make, never from the path map. We interpret
 * data only: no recipe, shell/eval/file function, or recursive make is run. */
typedef struct { char *s; size_t n; } Text;
typedef struct { char *name, *value; } Variable;
typedef struct Rule {
    char *name;
    Text deps, recipe;
    struct Rule **links;
    size_t *sources, nlinks, nsources;
    unsigned visited;
    int prepared;
} Rule;
static Variable *variables;
static size_t nv;
static Rule *rules;
static size_t nr;
static unsigned walk_id;
static Strings tracked;
static unsigned char *coverage;
static size_t consumer_pairs, recipe_rules, unresolved;
static const char *expanding_rule;

static void append(Text *t, const char *s, size_t n)
{
    if (t->n + n > 16 * 1024 * 1024) die("expanded make text exceeds 16 MiB in %s", expanding_rule ? expanding_rule : "database");
    t->s = resize(t->s, t->n + n + 1);
    memcpy(t->s + t->n, s, n); t->n += n; t->s[t->n] = 0;
}
static char *slice(const char *s, size_t n)
{
    char *p = resize(NULL, n + 1);
    memcpy(p, s, n); p[n] = 0; return p;
}
static char *trim(char *s)
{
    size_t n;
    while (isspace((unsigned char)*s)) s++;
    n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = 0;
    return s;
}
static char *variable(const char *name)
{
    for (size_t i = nv; i; i--)
        if (!strcmp(name, variables[i - 1].name)) return variables[i - 1].value;
    return NULL;
}
static void set_variable(const char *name, const char *value)
{
    variables = resize(variables, (nv + 1) * sizeof *variables);
    variables[nv].name = strdup(name); variables[nv].value = strdup(value); nv++;
}
static int rule_cmp(const void *a, const void *b)
{
    return strcmp(((const Rule *)a)->name, ((const Rule *)b)->name);
}
static Rule *find_rule(const char *name)
{
    Rule key = {0}; key.name = (char *)name;
    return bsearch(&key, rules, nr, sizeof *rules, rule_cmp);
}
static void load_database(const char *path)
{
    FILE *f = fopen(path, "r");
    char *line = NULL, *define_name = NULL;
    size_t cap = 0;
    Text body = {0};
    Rule *current = NULL;
    int files = 0;
    if (!f) die("open make database %s: %s", path, strerror(errno));
    while (getline(&line, &cap, f) >= 0) {
        char *eq, *colon;
        if (define_name) {
            if (!strcmp(line, "endef\n") || !strcmp(line, "endef")) {
                set_variable(define_name, body.s ? body.s : "");
                free(define_name); define_name = NULL; free(body.s); body = (Text){0};
            } else append(&body, line, strlen(line));
            continue;
        }
        if (!strcmp(line, "# Files\n")) { files = 1; continue; }
        if (!strncmp(line, "# files hash-table stats:", 25)) break;
        if (!files) {
            if (!strncmp(line, "define ", 7)) { define_name = strdup(trim(line + 7)); continue; }
            if (line[0] == '#' || isspace((unsigned char)line[0])) continue;
            eq = strchr(line, '=');
            if (eq) {
                char *end = eq;
                if (end > line && strchr(":+?", end[-1])) end--;
                *end = 0;
                set_variable(trim(line), trim(eq + 1));
            }
            continue;
        }
        if (line[0] == '\t') {
            if (current) append(&current->recipe, line + 1, strlen(line + 1));
            continue;
        }
        if (line[0] == '#' || line[0] == '\n') continue;
        current = NULL;
        colon = strchr(line, ':');
        if (!colon) continue;
        *colon = 0;
        /* Target-specific variable rows are not prerequisite edges. */
        eq = strchr(colon + 1, '=');
        if (eq) {
            char *q = colon + 1;
            while (*q && *q != '=' && !isspace((unsigned char)*q)) q++;
            if (*q == '=' || strstr(colon + 1, " =") || strstr(colon + 1, " :=")) continue;
        }
        if (strpbrk(line, " \t%$") || !*line) continue;
        /* GNU make prints :: rules separately; union their evidence below. */
        rules = resize(rules, (nr + 1) * sizeof *rules);
        current = &rules[nr++]; *current = (Rule){0}; current->name = strdup(line);
        append(&current->deps, colon + 1 + (colon[1] == ':'), strlen(colon + 1 + (colon[1] == ':')));
    }
    if (ferror(f) || fclose(f)) die("read make database %s", path);
    free(line);
    if (!files || !nr || define_name) die("incomplete make database %s", path);
    qsort(rules, nr, sizeof *rules, rule_cmp);
    for (size_t i = 1; i < nr; i++) if (!strcmp(rules[i - 1].name, rules[i].name)) {
        append(&rules[i].deps, " ", 1);
        append(&rules[i].deps, rules[i - 1].deps.s ? rules[i - 1].deps.s : "", rules[i - 1].deps.n);
        append(&rules[i].recipe, "\n", 1);
        append(&rules[i].recipe, rules[i - 1].recipe.s ? rules[i - 1].recipe.s : "", rules[i - 1].recipe.n);
        free(rules[i - 1].name); free(rules[i - 1].deps.s); free(rules[i - 1].recipe.s);
        memmove(&rules[i - 1], &rules[i], (nr - i) * sizeof *rules); nr--; i--;
    }
}
static char *expand_make(const char *s, const Strings *args, unsigned depth);
static Strings arguments(char *s)
{
    Strings a = {0};
    char *start = s;
    unsigned nesting = 0;
    for (char *p = s; ; p++) {
        if ((*p == ',' && !nesting) || !*p) {
            int last = !*p;
            *p = 0; add(&a, strdup(trim(start))); start = p + 1;
            if (last) break;
        } else if (p[0] == '$' && (p[1] == '(' || p[1] == '{')) { nesting++; p++; }
        else if (nesting && (*p == ')' || *p == '}')) nesting--;
    }
    return a;
}
static void free_strings(Strings *s)
{
    for (size_t i = 0; i < s->n; i++) free(s->v[i]);
    free(s->v); *s = (Strings){0};
}
static int percent_match(const char *pattern, const char *word)
{
    const char *pct = strchr(pattern, '%');
    size_t n = strlen(word), prefix, suffix;
    if (!pct) return !strcmp(pattern, word);
    prefix = (size_t)(pct - pattern); suffix = strlen(pct + 1);
    return n >= prefix + suffix && !strncmp(pattern, word, prefix) && !strcmp(pct + 1, word + n - suffix);
}
static char *make_function(const char *name, Strings *a)
{
    Text out = {0};
    if ((!strcmp(name, "filter") || !strcmp(name, "filter-out")) && a->n == 2) {
        char *save, *word = strtok_r(a->v[1], " \t\n", &save);
        for (; word; word = strtok_r(NULL, " \t\n", &save)) {
            char *patterns = strdup(a->v[0]), *psave, *p = strtok_r(patterns, " \t\n", &psave);
            int match = 0;
            for (; p; p = strtok_r(NULL, " \t\n", &psave)) if (percent_match(p, word)) match = 1;
            if (match == !strcmp(name, "filter")) { append(&out, word, strlen(word)); append(&out, " ", 1); }
            free(patterns);
        }
    } else if (!strcmp(name, "patsubst") && a->n == 3) {
        char *save, *word = strtok_r(a->v[2], " \t\n", &save);
        for (; word; word = strtok_r(NULL, " \t\n", &save)) {
            char *pct = strchr(a->v[0], '%'), *replace = strchr(a->v[1], '%');
            if (percent_match(a->v[0], word)) {
                if (pct && replace) {
                    size_t prefix = (size_t)(pct - a->v[0]);
                    append(&out, a->v[1], (size_t)(replace - a->v[1]));
                    append(&out, word + prefix, strlen(word) - prefix - strlen(pct + 1));
                    append(&out, replace + 1, strlen(replace + 1));
                } else append(&out, a->v[1], strlen(a->v[1]));
            } else append(&out, word, strlen(word));
            append(&out, " ", 1);
        }
    } else if (!strcmp(name, "subst") && a->n == 3 && *a->v[0]) {
        const char *s = a->v[2], *p;
        while ((p = strstr(s, a->v[0])) != NULL) {
            append(&out, s, (size_t)(p - s)); append(&out, a->v[1], strlen(a->v[1])); s = p + strlen(a->v[0]);
        }
        append(&out, s, strlen(s));
    } else if ((!strcmp(name, "firstword") || !strcmp(name, "words") || !strcmp(name, "strip") ||
                !strcmp(name, "sort") || !strcmp(name, "basename") || !strcmp(name, "notdir") || !strcmp(name, "dir")) && a->n == 1) {
        char *save, *word = strtok_r(a->v[0], " \t\n", &save);
        size_t n = 0;
        for (; word; word = strtok_r(NULL, " \t\n", &save)) {
            char *slash = strrchr(word, '/'), *dot = strrchr(word, '.');
            n++;
            if (!strcmp(name, "words")) continue;
            if (!strcmp(name, "basename") && dot && (!slash || dot > slash)) *dot = 0;
            if (!strcmp(name, "notdir") && slash) word = slash + 1;
            if (!strcmp(name, "dir")) {
                if (slash) slash[1] = 0; else word = "./";
            }
            if (out.n) append(&out, " ", 1);
            append(&out, word, strlen(word));
            if (!strcmp(name, "firstword")) break;
        }
        if (!strcmp(name, "words")) {
            char number[32]; snprintf(number, sizeof number, "%zu", n); append(&out, number, strlen(number));
        }
    } else return NULL;
    return out.s ? out.s : strdup("");
}

static char *expand_ref(char *ref, const Strings *args, unsigned depth)
{
    char *space = strpbrk(ref, " \t\n"), *value, *name;
    Text out = {0};
    char *function_result;
    if (space) {
        Strings raw, expanded = {0};
        *space++ = 0; raw = arguments(space);
        for (size_t i = 0; i < raw.n; i++) add(&expanded, expand_make(raw.v[i], args, depth + 1));
        if ((function_result = make_function(ref, &expanded)) != NULL) {
            append(&out, function_result, strlen(function_result)); free(function_result);
        } else if (!strcmp(ref, "call") && expanded.n) {
            value = variable(trim(expanded.v[0]));
            if (value) {
                Strings local = {expanded.v + 1, expanded.n - 1};
                char *s = expand_make(value, &local, depth + 1);
                append(&out, s, strlen(s)); free(s);
            } else unresolved++;
        } else if ((!strcmp(ref, "addprefix") || !strcmp(ref, "addsuffix")) && expanded.n == 2) {
            char *save, *word = strtok_r(expanded.v[1], " \t\n", &save);
            for (; word; word = strtok_r(NULL, " \t\n", &save)) {
                const char *a = !strcmp(ref, "addprefix") ? expanded.v[0] : word;
                const char *b = !strcmp(ref, "addprefix") ? word : expanded.v[0];
                append(&out, a, strlen(a)); append(&out, b, strlen(b)); append(&out, " ", 1);
            }
        } else if (!strcmp(ref, "foreach") && raw.n == 3) {
            size_t saved_nv = nv;
            char *save, *word = strtok_r(expanded.v[1], " \t\n", &save);
            for (; word; word = strtok_r(NULL, " \t\n", &save)) {
                char *s;
                set_variable(trim(expanded.v[0]), word);
                s = expand_make(raw.v[2], args, depth + 1);
                append(&out, s, strlen(s)); append(&out, " ", 1); free(s);
                free(variables[nv - 1].name); free(variables[nv - 1].value); nv = saved_nv;
            }
        } else {
            /* An over-approximation: preserve input paths from ALL branches.
             * Never execute $(shell ...), $(eval ...), or $(file ...). */
            unresolved++;
            for (size_t i = 0; i < expanded.n; i++) {
                append(&out, expanded.v[i], strlen(expanded.v[i])); append(&out, " ", 1);
            }
        }
        free_strings(&raw); free_strings(&expanded);
        return out.s ? out.s : strdup("");
    }
    if (args && isdigit((unsigned char)*ref)) {
        char *end;
        unsigned long i = strtoul(ref, &end, 10);
        if (!*end) return strdup(i && i <= args->n ? args->v[i - 1] : "");
    }
    name = expand_make(ref, args, depth + 1);
    value = variable(name); free(name);
    return value ? expand_make(value, args, depth + 1) : strdup("");
}
static char *expand_make(const char *s, const Strings *args, unsigned depth)
{
    Text out = {0};
    if (depth > 64) { unresolved++; return strdup(""); }
    while (*s) {
        if (s[0] == '$' && s[1] == '$') { append(&out, "$", 1); s += 2; }
        else if (s[0] == '$' && (s[1] == '(' || s[1] == '{')) {
            const char *start = s + 2, *p = start;
            unsigned nesting = 1;
            char close = s[1] == '(' ? ')' : '}';
            for (; *p; p++) {
                if (*p == s[1]) nesting++;
                else if (*p == close && !--nesting) break;
            }
            if (!*p) { unresolved++; append(&out, s, strlen(s)); break; }
            char *ref = slice(start, (size_t)(p - start));
            char *value = expand_ref(ref, args, depth + 1);
            append(&out, value, strlen(value)); free(value); free(ref); s = p + 1;
        } else if (s[0] == '$' && s[1]) {
            char ref[2] = {s[1], 0};
            char *value = expand_ref(ref, args, depth + 1);
            append(&out, value, strlen(value)); free(value); s += 2;
        } else { append(&out, s, 1); s++; }
    }
    return out.s ? out.s : strdup("");
}
static int source_scope(const char *s)
{
    const char *dirs[] = {"os/", "harness/", "spec/", "seed/", "tools/", "pubs/"};
    for (size_t i = 0; i < sizeof dirs / sizeof dirs[0]; i++)
        if (!strncmp(s, dirs[i], strlen(dirs[i]))) return 1;
    return 0;
}
static void consume_token(Rule *consumer, const char *s, int prerequisite)
{
    char **found;
    Rule *r;
    if (!strncmp(s, "./", 2)) s += 2;
    found = bsearch(&s, tracked.v, tracked.n, sizeof *tracked.v, cmp);
    if (found && source_scope(s)) {
        size_t p = (size_t)(found - tracked.v);
        consumer->sources = resize(consumer->sources, (consumer->nsources + 1) * sizeof *consumer->sources);
        consumer->sources[consumer->nsources++] = p;
    }
    r = prerequisite || strchr(s, '/') ? find_rule(s) : NULL;
    if (r) {
        consumer->links = resize(consumer->links, (consumer->nlinks + 1) * sizeof *consumer->links);
        consumer->links[consumer->nlinks++] = r;
    }
}
static void consume_text(Rule *consumer, const char *s, int prerequisite)
{
    while (*s) {
        const char *start;
        char *word;
        while (*s && !isalnum((unsigned char)*s) && !strchr("_./", *s)) s++;
        start = s;
        while (*s && (isalnum((unsigned char)*s) || strchr("_./+*-?[]", *s))) s++;
        if (s == start) continue;
        word = slice(start, (size_t)(s - start));
        consume_token(consumer, word, prerequisite);
        if (strchr(word, '/') && strpbrk(word, "*?[")) {
            for (size_t i = 0; i < tracked.n; i++)
                if (!fnmatch(word, tracked.v[i], FNM_PATHNAME)) consume_token(consumer, tracked.v[i], prerequisite);
        }
        free(word);
    }
}
static void walk_rule(Rule *r, size_t gate)
{
    if (r->visited == walk_id) return;
    r->visited = walk_id;
    if (!r->prepared) {
        r->prepared = 1;
        if (r->deps.s) consume_text(r, r->deps.s, 1);
        if (r->recipe.n) {
            char *expanded;
            expanding_rule = r->name;
            expanded = expand_make(r->recipe.s, NULL, 0); recipe_rules++;
            consume_text(r, expanded, 0); free(expanded);
        }
    }
    for (size_t i = 0; i < r->nsources; i++) coverage[gate * tracked.n + r->sources[i]] = 1;
    for (size_t i = 0; i < r->nlinks; i++) walk_rule(r->links[i], gate);
}
static void audit_consumers(const char *database, const Strings *inventory, const int *owners, Strings paths)
{
    unsigned char *matches;
    tracked = paths; qsort(tracked.v, tracked.n, sizeof *tracked.v, cmp);
    load_database(database);
    coverage = calloc(inventory->n, tracked.n);
    matches = calloc(nf, tracked.n);
    if (!coverage || !matches) die("out of memory");
    for (size_t f = 0; f < nf; f++) for (size_t p = 0; p < tracked.n; p++)
        for (size_t k = 0; k < families[f].paths.n; k++)
            if (path_match(families[f].paths.v[k], tracked.v[p])) { matches[f * tracked.n + p] = 1; break; }
    for (size_t g = 0; g < inventory->n; g++) {
        Rule *r = find_rule(inventory->v[g]);
        if (!r) { red("no database rule for %s", inventory->v[g]); continue; }
        walk_id++; walk_rule(r, g);
    }
    for (size_t g = 0; g < inventory->n; g++) {
        if (owners[g] < 0) continue;
        /* Dispatchers deliberately have no automatic selection. Their
         * concrete gates are audited individually, including all twins.
         * Pending gates are named opt-ins, forbidden path triggers above;
         * they still have an inventory owner and remain runnable by name. */
        if (!strcmp(families[owners[g]].name, "aggregates") ||
            !strcmp(families[owners[g]].name, "pending")) continue;
        for (size_t p = 0; p < tracked.n; p++) if (coverage[g * tracked.n + p]) {
            int ok = !strcmp(families[owners[g]].name, "always") || matches[(size_t)owners[g] * tracked.n + p];
            consumer_pairs++;
            if (!ok) red("consumer %s -> %s (family %s not selected)", tracked.v[p], inventory->v[g], families[owners[g]].name);
        }
    }
    if (!errors) printf(">>> test-gate-map: green (consumers: %zu source/gate pairs, %zu recipe visits; %zu conservative/unresolved expansions)\n", consumer_pairs, recipe_rules, unresolved);
    free(matches);
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
    } else if (!strcmp(kind, "pending-active")) {
        const char *name = "\"test-bochs-pending\"";
        char *at = strstr(json, name);
        if (!at) die("missing pending gate for mutant");
        fwrite(json, 1, (size_t)(at - json), stdout);
        fputs("\"test-boot\"", stdout);
        fwrite(at + strlen(name), 1, length - (size_t)(at + strlen(name) - json), stdout);
    } else if (!strcmp(kind, "drop-consumer")) {
        const char *path = "\"harness/proptest/serial_trace_fixture.c\"";
        char *at = NULL, *end;
        for (size_t i = 0; i < nf; i++) if (!strcmp(families[i].name, "harness")) {
            at = strstr(json + families[i].begin, path);
            if (!at || at >= json + families[i].end) die("missing harness consumer path for mutant");
            break;
        }
        if (!at) die("missing harness family for mutant");
        end = at + strlen(path);
        while (*end && isspace((unsigned char)*end)) end++;
        if (*end != ',') die("consumer mutant requires a following path");
        fwrite(json, 1, (size_t)(at - json), stdout);
        fwrite(end + 1, 1, length - (size_t)(end + 1 - json), stdout);
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
    if (!((argc == 4 && (!strcmp(argv[1], "--audit") || !strcmp(argv[1], "--resolve"))) ||
          (argc == 5 && !strcmp(argv[1], "--audit-consumers"))))
        die("usage: gate_map --audit|--resolve MAP INVENTORY < paths; --audit-consumers MAP INVENTORY DATABASE < tracked; --mutate KIND MAP");
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
    if (!strcmp(argv[1], "--audit-consumers")) {
        audit_consumers(argv[4], &inventory, owners, paths);
        return errors ? 1 : 0;
    }
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
