/* check_adr_citations.c -- test-adr-citations (initech-lk29, initech-0ni0).
 *
 * Ref: CLAUDE.md Law 1 (a governance doc cited by id must exist -- the lk29
 *      defect: ADR-0002 was cited as ratified architecture and never filed),
 *      Law 2, Rule 2 (fail loud), Rule 6 (mutation-proven), Rule 8 (locked
 *      decisions are numbered blocks, not prose); docs/adr/ADR-0001-*.md and
 *      docs/adr/ADR-0002-*.md Appendix R (the citation inventories this gate
 *      enforces); PRD Sec 8/14; WL-0161 Frictions (all-Record scope).
 *
 * The oracle is the CITING corpus, not the Record: it is written independently
 * of the ADR files (CLAUDE.md, the PRD, HANDOFF, the worklogs, spec/, os/,
 * harness/, the other ADRs), so a Record that renames, renumbers or drops a
 * block that anyone relies on goes RED (Law 2, no by-construction agreement).
 *
 * Contract, for every Record named by --scope (e.g. ADR-0001, ADR-0002):
 *   (F) FILED: if the Record id appears anywhere in the scanned tree, a file
 *       <adr-dir>/<id>-*.md exists.
 *   (D) RESOLVES: every citation "<id> DEC-nn" (also "<id>-DEC-nn",
 *       "<id> (DEC-nn, DEC-mm)", "<id> DEC-nn/DEC-mm", "<id> Amendment DEC-nn")
 *       names a DEC heading in one of that Record's files: a markdown heading
 *       whose text, after an optional section number, an optional
 *       "Sub-Decision " and an optional "<ID> Amendment ", BEGINS with the
 *       DEC token. A DEC merely mentioned inside some other heading does not
 *       define it.
 *
 * Usage: check_adr_citations --root DIR --adr-dir DIR --scope ID [--scope ID]
 *                            [--inventory FILE]
 * Exit 0 = green; exit 1 = RED with "!!! test-adr-citations FAIL:" lines;
 * exit 2 = usage / I/O error. Scan order is sorted (deterministic output).
 * Factory C (Law 3): host-only, never linked into the artifact. ASCII-only.
 */
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define ID_LEN 8 /* "ADR-0001" */
#define DEC_MAX 32

struct item {
    char rec[ID_LEN + 1];
    char dec[DEC_MAX];
    char *site; /* "file:line" */
};

struct list {
    struct item *v;
    size_t n, cap;
};

static char **g_scope;
static size_t g_nscope;
static struct list g_defs, g_cites;
static size_t *g_mentions;
static char **g_first_mention;
static size_t g_files_scanned;

static void die(const char *what, const char *arg)
{
    fprintf(stderr, "!!! test-adr-citations ERROR: %s%s%s\n", what, arg ? " " : "", arg ? arg : "");
    exit(2);
}

static char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (!p) die("out of memory", NULL);
    memcpy(p, s, n);
    return p;
}

static void push(struct list *l, const char *rec, const char *dec, const char *file, long line)
{
    char site[4096];
    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 256;
        l->v = realloc(l->v, l->cap * sizeof *l->v);
        if (!l->v) die("out of memory", NULL);
    }
    snprintf(site, sizeof site, "%s:%ld", file, line);
    memcpy(l->v[l->n].rec, rec, ID_LEN);
    l->v[l->n].rec[ID_LEN] = '\0';
    snprintf(l->v[l->n].dec, DEC_MAX, "%s", dec);
    l->v[l->n].site = xstrdup(site);
    l->n++;
}

static int scope_index(const char *rec)
{
    for (size_t i = 0; i < g_nscope; i++)
        if (!strncmp(g_scope[i], rec, ID_LEN)) return (int)i;
    return -1;
}

/* WL-0161 / initech-0ni0: every filed Record, including future additions;
 * no fixed scope ceiling may prevent the make-discovered set being checked. */
static void add_scope(const char *id)
{
    size_t n = g_nscope + 1;
    if (scope_index(id) >= 0) return;
    g_scope = realloc(g_scope, n * sizeof *g_scope);
    g_mentions = realloc(g_mentions, n * sizeof *g_mentions);
    g_first_mention = realloc(g_first_mention, n * sizeof *g_first_mention);
    if (!g_scope || !g_mentions || !g_first_mention) die("out of memory", NULL);
    g_scope[g_nscope] = xstrdup(id);
    g_mentions[g_nscope] = 0;
    g_first_mention[g_nscope] = NULL;
    g_nscope = n;
}

/* "ADR-dddd" / "CDR-dddd" at s, bounded on both sides by non-alnum. */
static int is_record_id(const char *buf, size_t i, size_t len)
{
    const char *s = buf + i;
    if (i + ID_LEN > len) return 0;
    if (i > 0 && isalnum((unsigned char)buf[i - 1])) return 0;
    if (strncmp(s, "ADR-", 4) && strncmp(s, "CDR-", 4)) return 0;
    for (int k = 4; k < ID_LEN; k++)
        if (!isdigit((unsigned char)s[k])) return 0;
    if (i + ID_LEN < len && isalnum((unsigned char)s[ID_LEN])) return 0;
    return 1;
}

/* Parse "DEC-<digits>[a-z](.<digits>)*" at buf[i]; returns its length or 0. */
static size_t dec_token(const char *buf, size_t i, size_t len, char *out)
{
    size_t j = i + 4;
    if (i + 5 > len || strncmp(buf + i, "DEC-", 4)) return 0;
    if (i > 0 && isalnum((unsigned char)buf[i - 1])) return 0;
    if (!isdigit((unsigned char)buf[j])) return 0;
    while (j < len && isdigit((unsigned char)buf[j])) j++;
    if (j < len && islower((unsigned char)buf[j]) && !(j + 1 < len && isalnum((unsigned char)buf[j + 1]))) j++;
    while (j + 1 < len && buf[j] == '.' && isdigit((unsigned char)buf[j + 1])) {
        j++;
        while (j < len && isdigit((unsigned char)buf[j])) j++;
    }
    if (j < len && isalnum((unsigned char)buf[j])) return 0;
    if (j - i >= DEC_MAX) return 0;
    memcpy(out, buf + i, j - i);
    out[j - i] = '\0';
    return j - i;
}

static char *slurp(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    char *buf;
    long n;
    if (!f) die("cannot open", path);
    if (fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) die("cannot size", path);
    buf = malloc((size_t)n + 1);
    if (!buf) die("out of memory", NULL);
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) die("short read", path);
    fclose(f);
    buf[n] = '\0';
    *len = (size_t)n;
    return buf;
}

static long line_of(const char *buf, size_t upto)
{
    long line = 1;
    for (size_t k = 0; k < upto; k++)
        if (buf[k] == '\n') line++;
    return line;
}

static int in_set(char c, const char *set) { return c && strchr(set, c) != NULL; }

static size_t skip_word(const char *buf, size_t j, size_t len, const char *word)
{
    size_t w = strlen(word);
    if (j + w <= len && !strncmp(buf + j, word, w)) return j + w;
    return j;
}

/* End of a parenthetical opened at buf[j] == '(' (bounded, unnested, and
 * never spanning another record id), or j itself if there is none. */
static size_t paren_end(const char *buf, size_t len, size_t j)
{
    size_t k;
    if (j >= len || buf[j] != '(') return j;
    for (k = j + 1; k < len && k < j + 400; k++) {
        if (buf[k] == ')') return k;
        if (buf[k] == '(' || is_record_id(buf, k, len)) return j;
    }
    return j;
}

/* After a scoped record id ends at j, collect the DEC citations it introduces:
 *   <id> DEC-a[, /, +, and, or] DEC-b ...   (a chain; a "(...)" gloss after a
 *                                            DEC token is skipped, not ended)
 *   <id> (... DEC-a ... DEC-b ...)          (every DEC inside an immediately
 *                                            following parenthetical)        */
static void scan_chain(const char *buf, size_t len, size_t j, const char *rec, const char *file, long record_line)
{
    char dec[DEC_MAX];
    size_t origin = j;
    size_t k, t, p;
    /* "<id>-Some-File-Name.md" is a path, not a "<id>-DEC-nn" citation. */
    if (j < len && buf[j] == '-' && isalpha((unsigned char)buf[j + 1]) && strncmp(buf + j + 1, "DEC-", 4)) {
        while (j < len && (isalnum((unsigned char)buf[j]) || in_set(buf[j], "-_."))) j++;
    }
    for (k = j; k < len && k < j + 4 && in_set(buf[k], " *`"); k++) {}
    p = paren_end(buf, len, k);
    if (p != k) {
        for (size_t q = k + 1; q < p; q++) {
            size_t n = (buf[q] == 'D') ? dec_token(buf, q, len, dec) : 0;
            if (n) {
                push(&g_cites, rec, dec, file, record_line + line_of(buf + origin, q - origin) - 1);
                q += n - 1;
            }
        }
        return;
    }
    for (k = 0; k < 12 && j < len && in_set(buf[j], " \t\r\n*`#-:,"); k++) j++;
    j = skip_word(buf, j, len, "Amendment ");
    t = dec_token(buf, j, len, dec);
    while (t) {
        push(&g_cites, rec, dec, file, record_line + line_of(buf + origin, j - origin) - 1);
        j += t;
        for (k = 0; k < 4 && j < len && buf[j] == ' '; k++) j++;
        p = paren_end(buf, len, j);
        if (p != j) j = p + 1;
        for (k = 0; k < 12 && j < len && in_set(buf[j], " \t\r\n,/+&*`"); k++) j++;
        j = skip_word(buf, j, len, "and ");
        j = skip_word(buf, j, len, "or ");
        t = dec_token(buf, j, len, dec);
        /* Shorthand "DEC-01/03" == DEC-01 and DEC-03: exactly two digits
         * (plus an optional amendment letter) right after the slash. */
        if (!t && j >= 1 && buf[j - 1] == '/' && j + 2 <= len && isdigit((unsigned char)buf[j]) &&
            isdigit((unsigned char)buf[j + 1])) {
            size_t e = j + 2;
            if (e < len && islower((unsigned char)buf[e])) e++;
            if (e >= len || !isalnum((unsigned char)buf[e])) {
                snprintf(dec, DEC_MAX, "DEC-%.*s", (int)(e - j), buf + j);
                t = e - j;
            }
        }
    }
}

static void scan_file(const char *path)
{
    size_t len;
    long line = 1;
    char *buf = slurp(path, &len);
    g_files_scanned++;
    /* PRD Sec 8/14: keep the full corpus gate cheap. Count each file's lines
     * once; scan_chain counts only the short suffix following this record. */
    for (size_t i = 0; i + ID_LEN <= len; i++) {
        int s;
        if (buf[i] == '\n') line++;
        if (buf[i] != 'A' && buf[i] != 'C') continue;
        if (!is_record_id(buf, i, len)) continue;
        s = scope_index(buf + i);
        if (s < 0) continue;
        if (!g_mentions[s]++) {
            char site[4096];
            snprintf(site, sizeof site, "%s:%ld", path, line);
            g_first_mention[s] = xstrdup(site);
        }
        scan_chain(buf, len, i + ID_LEN, buf + i, path, line);
    }
    free(buf);
}

static int wanted_file(const char *name)
{
    static const char *ext[] = {".md", ".c", ".h", ".json", ".mk", ".asm", ".inc", ".pas", ".ld", ".sh", NULL};
    size_t n = strlen(name);
    if (!strcmp(name, "Makefile")) return 1;
    for (int k = 0; ext[k]; k++) {
        size_t e = strlen(ext[k]);
        if (n > e && !strcmp(name + n - e, ext[k])) return 1;
    }
    return 0;
}

static int skip_dir(const char *name)
{
    /* Dot-dirs (.git, .beads, .claude/worktrees lanes) and build output. */
    return name[0] == '.' || !strcmp(name, "build");
}

static void walk(const char *dir)
{
    struct dirent **ents;
    int n = scandir(dir, &ents, NULL, alphasort);
    if (n < 0) die("cannot scan", dir);
    for (int k = 0; k < n; k++) {
        const char *name = ents[k]->d_name;
        char path[4096];
        struct stat st;
        if (!strcmp(name, ".") || !strcmp(name, "..")) { free(ents[k]); continue; }
        if (!strcmp(dir, "."))
            snprintf(path, sizeof path, "%s", name);
        else
            snprintf(path, sizeof path, "%s/%s", dir, name);
        if (lstat(path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (!skip_dir(name)) walk(path);
            } else if (S_ISREG(st.st_mode) && wanted_file(name)) {
                scan_file(path);
            }
        }
        free(ents[k]);
    }
    free(ents);
}

/* Heading text -> the DEC it DEFINES, if any (see contract (D)). */
static void heading_def(const char *rec, const char *line, size_t len, const char *file, long lineno)
{
    char dec[DEC_MAX];
    size_t j = 0;
    while (j < len && line[j] == '#') j++;
    while (j < len && line[j] == ' ') j++;
    if (j < len && isdigit((unsigned char)line[j])) {
        size_t s = j;
        while (j < len && (isdigit((unsigned char)line[j]) || line[j] == '.')) j++;
        if (j < len && line[j] == ' ') {
            while (j < len && line[j] == ' ') j++;
        } else {
            j = s;
        }
    }
    j = skip_word(line, j, len, "Sub-Decision ");
    if (is_record_id(line, j, len)) {
        size_t a = skip_word(line, j + ID_LEN, len, " Amendment ");
        if (a != j + ID_LEN) j = a;
    }
    if (dec_token(line, j, len, dec)) push(&g_defs, rec, dec, file, lineno);
}

static void load_defs(const char *adr_dir, int *filed)
{
    struct dirent **ents;
    int n = scandir(adr_dir, &ents, NULL, alphasort);
    if (n < 0) die("cannot scan", adr_dir);
    for (int k = 0; k < n; k++) {
        const char *name = ents[k]->d_name;
        size_t nl = strlen(name);
        int s;
        if (nl > ID_LEN + 4 && is_record_id(name, 0, nl) && name[ID_LEN] == '-' && !strcmp(name + nl - 3, ".md")) {
            char path[4096], *buf;
            size_t len, ls = 0;
            long line = 1;
            snprintf(path, sizeof path, "%s/%s", adr_dir, name);
            s = scope_index(name);
            if (s >= 0) filed[s] = 1;
            buf = slurp(path, &len);
            for (size_t i = 0; i <= len; i++) {
                if (i == len || buf[i] == '\n') {
                    if (buf[ls] == '#') heading_def(name, buf + ls, i - ls, path, line);
                    ls = i + 1;
                    line++;
                }
            }
            free(buf);
        }
        free(ents[k]);
    }
    free(ents);
}

static int defined(const char *rec, const char *dec)
{
    for (size_t k = 0; k < g_defs.n; k++)
        if (!strcmp(g_defs.v[k].rec, rec) && !strcmp(g_defs.v[k].dec, dec)) return 1;
    return 0;
}

static int cmp_item(const void *a, const void *b)
{
    const struct item *x = a, *y = b;
    int c = strcmp(x->rec, y->rec);
    if (!c) c = strcmp(x->dec, y->dec);
    return c;
}

int main(int argc, char **argv)
{
    const char *root = NULL, *adr_dir = NULL, *inventory = NULL;
    int *filed;
    int red = 0;
    for (int k = 1; k < argc; k++) {
        if (!strcmp(argv[k], "--root") && k + 1 < argc) root = argv[++k];
        else if (!strcmp(argv[k], "--adr-dir") && k + 1 < argc) adr_dir = argv[++k];
        else if (!strcmp(argv[k], "--inventory") && k + 1 < argc) inventory = argv[++k];
        else if (!strcmp(argv[k], "--scope") && k + 1 < argc) {
            const char *id = argv[++k];
            if (strlen(id) != ID_LEN || !is_record_id(id, 0, ID_LEN)) die("bad --scope id", id);
            add_scope(id);
        } else die("usage: --root DIR --adr-dir DIR --scope ID... [--inventory FILE]; bad arg", argv[k]);
    }
    if (!root || !adr_dir || !g_nscope) die("usage: --root DIR --adr-dir DIR --scope ID... [--inventory FILE]", NULL);

    filed = calloc(g_nscope, sizeof *filed);
    if (!filed) die("out of memory", NULL);
    load_defs(adr_dir, filed);
    walk(root);
    {
        /* Stable sort by (rec, dec) so sites keep scan (sorted-path) order. */
        size_t n = g_cites.n;
        struct item *tmp = malloc((n ? n : 1) * sizeof *tmp);
        size_t *ord = malloc((n ? n : 1) * sizeof *ord);
        if (!tmp || !ord) die("out of memory", NULL);
        for (size_t k = 0; k < n; k++) ord[k] = k;
        /* insertion sort: n is small (hundreds) and this is stable */
        for (size_t k = 1; k < n; k++) {
            size_t v = ord[k], m = k;
            while (m > 0 && cmp_item(&g_cites.v[ord[m - 1]], &g_cites.v[v]) > 0) { ord[m] = ord[m - 1]; m--; }
            ord[m] = v;
        }
        for (size_t k = 0; k < n; k++) tmp[k] = g_cites.v[ord[k]];
        memcpy(g_cites.v, tmp, n * sizeof *tmp);
        free(tmp);
        free(ord);
    }

    for (size_t s = 0; s < g_nscope; s++) {
        if (g_mentions[s] && !filed[s]) {
            printf("!!! test-adr-citations FAIL: %s is cited at %zu site(s) (first: %s) but no %s/%s-*.md is filed\n",
                   g_scope[s], g_mentions[s], g_first_mention[s], adr_dir, g_scope[s]);
            red = 1;
        }
    }
    for (size_t k = 0; k < g_cites.n;) {
        size_t e = k;
        while (e < g_cites.n && !cmp_item(&g_cites.v[k], &g_cites.v[e])) e++;
        if (!defined(g_cites.v[k].rec, g_cites.v[k].dec)) {
            printf("!!! test-adr-citations FAIL: %s %s is cited at %zu site(s) (first: %s) but no '%s' heading exists in %s/%s-*.md\n",
                   g_cites.v[k].rec, g_cites.v[k].dec, e - k, g_cites.v[k].site, g_cites.v[k].dec, adr_dir, g_cites.v[k].rec);
            red = 1;
        }
        k = e;
    }
    if (inventory) {
        FILE *f = fopen(inventory, "w");
        if (!f) die("cannot write", inventory);
        for (size_t k = 0; k < g_cites.n; k++)
            fprintf(f, "%s %s %s %s\n", g_cites.v[k].rec, g_cites.v[k].dec,
                    defined(g_cites.v[k].rec, g_cites.v[k].dec) ? "OK " : "RED", g_cites.v[k].site);
        fclose(f);
    }
    if (red) return 1;

    printf(">>> test-adr-citations: green (%zu files scanned; %zu Records; %zu DEC citation(s) resolve to DEC headings)\n",
           g_files_scanned, g_nscope, g_cites.n);
    return 0;
}
