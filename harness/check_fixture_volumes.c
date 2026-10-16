/* Ref: PRD Sec 8/14; CLAUDE.md Rule 2/6; initech-tflp.
 * Read GNU make's database, then ask make to EXPAND (never execute) recipes.
 * This sees eval-generated rules, nested boot helpers and automatic $@ names.
 * The resulting inventory is also kept for review in build/fixture-volumes.*.
 */
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void fail(const char *why)
{
    fprintf(stderr, "!!! test-fixture-volumes FAIL: %s\n", why);
    exit(1);
}

static FILE *open_file(const char *name, const char *mode)
{
    FILE *f = fopen(name, mode);
    if (!f) { perror(name); fail("inventory I/O error"); }
    return f;
}

static int image(const char *s)
{
    size_t n = strlen(s);
    return n >= 4 && !strcmp(s + n - 4, ".img");
}

static int probe(const char *db, const char *out, const char *manifest)
{
    FILE *in = open_file(db, "r"), *mk = open_file(out, "w");
    char *line = NULL, target[4096] = "", first[4096] = "";
    size_t cap = 0;
    int section = 0, recipe = 0, count = 0, continuation = 0;
    fprintf(mk, "# Generated expansion-only probe; no original commands run.\n");
    while (getline(&line, &cap, in) >= 0) {
        if (!strcmp(line, "# Implicit Rules\n") || !strcmp(line, "# Files\n")) section = 1;
        if (!section) continue;
        if (line[0] != '\t' && recipe && !continuation) { fprintf(mk, "endef\n"); recipe = 0; }
        if ((line[0] == '\t' || continuation) && target[0]) {
            if (!recipe) {
                ++count;
                fprintf(mk, "tflp_target_%d := %s\ntflp_first_%d := %s\ndefine tflp_recipe_%d\n",
                        count, target, count, first, count);
                recipe = 1;
            }
            fputs(line + (line[0] == '\t'), mk);
            size_t len = strlen(line);
            continuation = len > 1 && line[len-2] == '\\';
        } else if (line[0] != '#' && !isspace((unsigned char)line[0])) {
            char *colon = strchr(line, ':');
            target[0] = first[0] = 0;
            if (!colon || strchr(line, '=')) continue;
            *colon = 0;
            if (strlen(line) >= sizeof(target) || strpbrk(line, " \t$")) {
                fprintf(stderr, "database target: %s\n", line);
                fail("unsupported target in make database");
            }
            strcpy(target, line);
            (void)sscanf(colon + 1, "%4095s", first);
        }
    }
    if (ferror(in)) fail("reading make database");
    if (recipe) fprintf(mk, "endef\n");
    fprintf(mk, ".PHONY: fixture-volume-probe\nfixture-volume-probe: .EXTRA_PREREQS =\nfixture-volume-probe:\n");
    fprintf(mk, "\t$(file >%s,)\n", manifest);
    fprintf(mk, "\t$(foreach v,$(FIXTURE_VOLUME_NAMES),$(file >>%s,@@contract $(v) $(FIXTURE_CONTENTS_$(v))))\n", manifest);
    for (int i = 1; i <= count; ++i)
        fprintf(mk, "\t$(foreach @,$(tflp_target_%d),$(foreach <,$(or $(tflp_first_%d),__tflp_empty__),$(file >>%s,@@ $@)$(file >>%s,$(tflp_recipe_%d))))\n",
                i, i, manifest, manifest, i);
    fprintf(mk, "\t@:\n");
    free(line);
    fclose(in);
    if (fclose(mk)) fail("writing expansion probe");
    if (!count) fail("no recipes found in make database");
    return 0;
}

typedef struct { char *path, *owner; } Name;
static Name fixtures[4096], scratch[4096], contracts[4096], minted[4096], verified[4096], seeded[4096], volumes[4096];
static size_t nf, ns, nc, nm, nv, ni, nd;

/* Shell-loop variables denote a family; compare conservatively as globs.
 * Make pattern rules use %, also converted to a glob. */
static char *pattern(const char *s)
{
    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd))) fail("cannot resolve working directory");
    size_t size = strlen(s) + strlen(cwd) + 2;
    char *out = malloc(size), *p = out;
    if (!out) fail("out of memory");
    if (*s != '/') { p += sprintf(p, "%s/", cwd); }
    while (*s) {
        if (*s == '$') {
            *p++ = '*'; ++s;
            if (*s == '{') { while (*s && *s != '}') ++s; if (*s) ++s; }
            else { while (isalnum((unsigned char)*s) || *s == '_') ++s; }
        } else { *p++ = *s == '%' ? '*' : *s; ++s; }
    }
    *p = 0;
    /* Compare actual path aliases too: ./build/x and build/sub/../x must
     * collide with build/x. All comparisons are absolute and lexical; the
     * images need not exist for this static gate. */
    char *raw = strdup(out), *part, *save = NULL, *parts[4096];
    size_t count = 0;
    if (!raw) fail("out of memory");
    for (part = strtok_r(raw, "/", &save); part; part = strtok_r(NULL, "/", &save)) {
        if (!strcmp(part, ".")) continue;
        if (!strcmp(part, "..")) { if (count) --count; continue; }
        if (count == 4096) fail("too many path components");
        parts[count++] = part;
    }
    p = out;
    for (size_t i = 0; i < count; ++i) p += sprintf(p, "/%s", parts[i]);
    *p = 0;
    free(raw);
    return out;
}

static void add(Name *names, size_t *n, const char *path, const char *owner)
{
    if (!image(path)) return;
    for (size_t i = 0; i < *n; ++i)
        if (!strcmp(names[i].path, path) && !strcmp(names[i].owner, owner)) return;
    if (*n == 4096) fail("image inventory capacity exceeded");
    names[*n].path = strdup(path);
    names[*n].owner = strdup(owner);
    if (!names[*n].path || !names[*n].owner) fail("out of memory");
    ++*n;
}

/* Tokenize shell words without executing substitutions or loops. Quoted
 * diagnostic strings stay a single word, so they cannot invent cp commands. */
static char *word(char **cursor)
{
    char *s = *cursor, *start, *p;
    while (*s && (isspace((unsigned char)*s) || strchr(";|&", *s))) ++s;
    if (!*s) { *cursor = s; return NULL; }
    start = p = s;
    int quote = 0;
    while (*s) {
        if (!quote && (isspace((unsigned char)*s) || strchr(";|&", *s))) break;
        if ((*s == '\'' || *s == '"') && (!quote || quote == *s)) {
            quote = quote ? 0 : *s; ++s;
        } else if (*s == '\\' && s[1]) { ++s; *p++ = *s++; }
        else *p++ = *s++;
    }
    if (*s) ++s;
    *p = 0;
    *cursor = s;
    return start;
}

static void inspect(char *line, const char *owner)
{
    char *cursor = line, *w;
    while ((w = word(&cursor))) {
        while (*w == '@' || *w == '-' || *w == '+') ++w;
        if (!strcmp(w, "sh")) {
            char *script = word(&cursor);
            if (script && !strcmp(script, "harness/verify_fixture_volume.sh")) {
                char *phase = word(&cursor), *path = word(&cursor);
                if (phase && path && !strcmp(phase, "recipe")) add(verified, &nv, path, owner);
            }
        } else if (!strcmp(w, "cp")) {
            char *src = word(&cursor), *dst;
            while (src && src[0] == '-') src = word(&cursor);
            dst = word(&cursor);
            if (src && dst && !image(owner)) add(scratch, &ns, dst, owner);
        } else if (!strcmp(w, "mcopy")) {
            if (image(owner)) add(fixtures, &nf, owner, owner);
            else {
                while ((w = word(&cursor))) {
                    if (!strcmp(w, "-i")) {
                        char *path = word(&cursor), *src = word(&cursor), *dst = word(&cursor);
                        if (path && src && dst && !strncmp(dst, "::", 2)) add(seeded, &ni, path, owner);
                        break;
                    }
                }
            }
        } else if (!strcmp(w, "mformat") ||
                   !strcmp(w, "mattrib") || !strcmp(w, "mmd") || !strcmp(w, "mmove")) {
            if (image(owner)) add(volumes, &nd, owner, owner);
            /* Directory-only/blank FAT goldens are deliberately reformatted
             * in place by host tests; seeded file/attribute fixtures are the
             * volumes that a scratch writer must never replace. */
            if (image(owner) && (!strcmp(w, "mcopy") || !strcmp(w, "mattrib") || !strcmp(w, "mmove")))
                add(fixtures, &nf, owner, owner);
            /* Inline minters also derive writable per-gate image names. */
            if (!strcmp(w, "mformat") && !image(owner)) {
                while ((w = word(&cursor))) {
                    if (!strcmp(w, "-i")) {
                        char *dst = word(&cursor);
                        if (dst) add(minted, &nm, dst, owner);
                        break;
                    }
                }
            }
        } else if (!strcmp(w, "dd") && !image(owner)) {
            while ((w = word(&cursor))) {
                if (!strncmp(w, "of=", 3)) { add(minted, &nm, w + 3, owner); break; }
                if (strchr(w, ';')) break;
            }
        }
    }
}

static int overlap(const char *a, const char *b)
{
    /* Two '*' globs intersect iff their fixed prefixes and suffixes agree.
     * The factory's derived paths use only '*' (no [] or ? patterns). */
    const char *as = strchr(a, '*'), *bs = strchr(b, '*');
    if (!as) return fnmatch(b, a, 0) == 0;
    if (!bs) return fnmatch(a, b, 0) == 0;
    size_t ap = (size_t)(as-a), bp = (size_t)(bs-b);
    if (strncmp(a, b, ap < bp ? ap : bp)) return 0;
    as = strrchr(a, '*') + 1; bs = strrchr(b, '*') + 1;
    size_t an = strlen(as), bn = strlen(bs), n = an < bn ? an : bn;
    return !strcmp(as + an - n, bs + bn - n);
}

static int unchecked_tail(const char *owner, int checked)
{
    for (size_t i = 0; i < nf; ++i) {
        if (!strcmp(owner, fixtures[i].path) && !checked) {
            fprintf(stderr, "!!! test-fixture-volumes FAIL: fixture recipe %s does not end with verification\n", owner);
            return 1;
        }
    }
    return 0;
}

static int check(const char *manifest, const char *inventory)
{
    FILE *in = open_file(manifest, "r"), *out = open_file(inventory, "w");
    char *line = NULL, owner[4096] = "";
    size_t cap = 0;
    int last_check = 0, bad = 0;
    while (getline(&line, &cap, in) >= 0) {
        if (!strncmp(line, "@@contract ", 11)) {
            char path[4096];
            if (sscanf(line+11, "%4095s", path) != 1) fail("empty fixture contract");
            char *spec = line+11+strlen(path);
            while (isspace((unsigned char)*spec)) ++spec;
            if (!*spec) fail("fixture contract has no expected paths");
            add(contracts, &nc, path, spec);
        } else if (!strncmp(line, "@@ ", 3)) {
            bad |= unchecked_tail(owner, last_check);
            line[strcspn(line, "\n")] = 0;
            if (strlen(line+3) >= sizeof(owner)) fail("target name too long");
            strcpy(owner, line+3);
            last_check = 0;
        } else if (*owner) {
            char *p = line;
            while (isspace((unsigned char)*p) || *p == '@') ++p;
            if (*p && *p != '#') last_check = strstr(p, "sh harness/verify_fixture_volume.sh recipe ") == p;
            inspect(line, owner);
        }
    }
    if (ferror(in)) fail("reading expanded recipes");
    bad |= unchecked_tail(owner, last_check);
    free(line); fclose(in);
    if (!nf || !ns) fail("empty fixture or scratch inventory");
    /* Protect directory-only and blank mtools goldens too. They have no
     * copied-file contract, and host tests intentionally reformat them. */
    for (size_t i = 0; i < nd; ++i) {
        size_t j;
        for (j = 0; j < nc; ++j) if (!strcmp(volumes[i].path, contracts[j].path)) break;
        if (j == nc) add(contracts, &nc, volumes[i].path, "@unseeded\n");
    }
    for (size_t i = 0; i < nf; ++i) {
        fprintf(out, "fixture\t%s\t%s\n", fixtures[i].owner, fixtures[i].path);
        size_t j;
        for (j = 0; j < nc; ++j) if (!strcmp(fixtures[i].path, contracts[j].path)) break;
        /* initech-a5sr: a SEEDED fixture (mcopy/mattrib/mmove) must carry a
         * declared contract. The @unseeded fallback above exists for blank
         * and directory-only goldens; a seeded recipe that also runs mformat
         * was filed there too, which masked three undeclared InitechWord
         * mutant volumes (6865fc0 / 8080833) until emu-gate time. */
        if (j == nc || !strcmp(contracts[j].owner, "@unseeded\n")) {
            fprintf(stderr, "!!! test-fixture-volumes FAIL: fixture %s has no declared contract\n", fixtures[i].path);
            bad = 1;
        }
    }
    for (size_t i = 0; i < ni; ++i) {
        size_t j, k;
        for (j = 0; j < nc; ++j) if (!strcmp(seeded[i].path, contracts[j].path)) break;
        for (k = 0; k < nv; ++k)
            if (!strcmp(seeded[i].path, verified[k].path) && !strcmp(seeded[i].owner, verified[k].owner)) break;
        if (j == nc || k == nv) {
            fprintf(stderr, "!!! test-fixture-volumes FAIL: inline fixture %s (gate %s) lacks a contract or recipe verification\n", seeded[i].path, seeded[i].owner);
            bad = 1;
        }
    }
    for (size_t i = 0; i < nc; ++i) fprintf(out, "contract\t%s\t%s", contracts[i].path, contracts[i].owner);
    for (size_t i = 0; i < ns; ++i) fprintf(out, "scratch\t%s\t%s\n", scratch[i].owner, scratch[i].path);
    for (size_t i = 0; i < nm; ++i) fprintf(out, "mint\t%s\t%s\n", minted[i].owner, minted[i].path);
    if (fclose(out)) fail("writing inventory");
    for (size_t i = 0; i < ns + nm; ++i) {
        Name *write = i < ns ? &scratch[i] : &minted[i-ns];
        char *s = pattern(write->path);
        for (size_t j = 0; j < nc; ++j) {
            char *f = pattern(contracts[j].path);
            if (overlap(s, f)) {
                /* Inline fixture minters own their names: they author the
                 * declared fixture AND verify it in the same recipe. A copy
                 * helper is never exempt (the historical QUIT overwrite). */
                size_t k;
                for (k = 0; k < nv; ++k)
                    if (!strcmp(verified[k].path, write->path) && !strcmp(verified[k].owner, write->owner)) break;
                if (i < ns || (k == nv && strcmp(contracts[j].owner, "@unseeded\n"))) {
                    fprintf(stderr, "!!! test-fixture-volumes FAIL: scratch %s (gate %s) collides with fixture %s\n",
                            write->path, write->owner, contracts[j].path);
                    bad = 1;
                }
            }
            free(f);
        }
        free(s);
    }
    if (!bad) printf(">>> test-fixture-volumes: green (%zu scratch names vs %zu fixture names; %zu inline image writers)\n", ns, nc, nm);
    return bad;
}

int main(int argc, char **argv)
{
    if (argc == 5 && !strcmp(argv[1], "--probe")) return probe(argv[2], argv[3], argv[4]);
    if (argc == 4 && !strcmp(argv[1], "--check")) return check(argv[2], argv[3]);
    fail("usage: --probe database probe.mk expanded | --check expanded inventory.tsv");
    return 1;
}
