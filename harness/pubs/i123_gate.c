/*
 * harness/pubs/i123_gate.c -- GC-01 for the Initech 123 books (factory C;
 * bead initech-ipu5.9; PUB-REF-0001 GC-01, CP-14, D-14, Sec 11.4).
 *
 * Inputs: the shipped inventory (NAME|KIND, harness/pubs/i123_face.c
 * inventory: KIND menu, function or key, every line obtained by driving
 * the shipped application) and the refusal list (NAME|refused, i123_face
 * refusals: the menu leaves that answer "Command not implemented"), and the
 * books' <job>.cmds lists (NAME|KIND|STATE|ENTRY, initech-syntax.sty
 * \RegisterCommand, the one registration mechanism).
 *
 *   i123_gate commands SHIPPED REFUSED K+K=book.cmds... other=book.cmds...
 *     both directions, by NAME|KIND pair:
 *       DOC-NOT-SHIPPED         a documented pair the build lacks, unless
 *                               its entry is DRAFT
 *       SHIPPED-NOT-DOCUMENTED  a shipped pair of KIND k missing from the
 *                               books given as "menu+function+key=book.cmds" (each
 *                               such book must document every shipped item
 *                               of those KINDs; "other=" books need not)
 *     and the state of every documented menu path against the build:
 *       DOC-GATED-REFUSED       documented as working (GATED) but the
 *                               application refuses it
 *       STALE-DRAFT             documented as DRAFT but the application
 *                               ships it working -- the book is behind the
 *                               software (lockstep, Sec 11.3)
 *     A documented @function or key that IS shipped may not be DRAFT either.
 *   i123_gate glossary BOOK.terms BOOK.gloss
 *     CP-03: every bold defined term (\term) has a Glossary entry and every
 *     Glossary entry is a defined term (TERM-NOT-IN-GLOSSARY,
 *     GLOSSARY-TERM-UNUSED); case is ignored.
 *   i123_gate mentions SHIPPED MENTIONS...
 *     every NAME|KIND an inline \slashcmd / \fn / \fkey wrote is shipped
 *     (MENTION-NOT-SHIPPED).
 * ASCII-clean (Rule 12).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXN 4096
#define MAXL 192
typedef struct { char s[MAXN][MAXL]; char st[MAXN]; int n; } set_t;

static void red(const char *tag, const char *where, const char *what)
{ fflush(stdout); fprintf(stderr, "!!! test-pubs RED %s: %s: %s\n", tag, where, what); exit(1); }
static int find(const set_t *s, const char *k) { for (int i = 0; i < s->n; i++) if (!strcmp(s->s[i], k)) return i; return -1; }
static int add(set_t *s, const char *k, char st)
{
    int i = find(s, k);
    if (i >= 0) { if (st == 'G') s->st[i] = 'G'; return i; }
    if (s->n == MAXN || strlen(k) >= MAXL) red("I123-SET", k, "too many or too long");
    strcpy(s->s[s->n], k); s->st[s->n] = st; return s->n++;
}
/* NAME|KIND lines (any trailing fields ignored); '#' comments. */
static void read_pairs(const char *path, set_t *s, int fields_min)
{
    FILE *f = fopen(path, "rb"); char line[512];
    if (!f) red("I123-INPUT", path, "cannot open");
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!line[0] || line[0] == '#') continue;
        char *b1 = strchr(line, '|');
        if (!b1) red("I123-INPUT", path, line);
        char *b2 = strchr(b1 + 1, '|');
        if (fields_min == 2) { if (b2) *b2 = 0; add(s, line, 'G'); continue; }
        /* a .cmds line: NAME|KIND|STATE|ENTRY */
        char *b3 = b2 ? strchr(b2 + 1, '|') : 0;
        if (!b2 || !b3) red("COMMAND-INDEX", path, line);
        *b3 = 0; *b2 = 0;
        char st = !strcmp(b2 + 1, "GATED") ? 'G' : !strcmp(b2 + 1, "DRAFT") ? 'D' : 0;
        if (!st) red("COMMAND-INDEX", path, line);
        if (strcmp(b1 + 1, "menu") && strcmp(b1 + 1, "function") && strcmp(b1 + 1, "key"))
            red("COMMAND-INDEX", path, line);
        add(s, line, st);
    }
    fclose(f);
}
static const char *kind_of(const char *pair) { const char *b = strrchr(pair, '|'); return b ? b + 1 : ""; }
static int kind_in(const char *kinds, const char *k)
{
    char buf[128]; snprintf(buf, sizeof buf, "+%s+", kinds);
    char want[40]; snprintf(want, sizeof want, "+%s+", k);
    return strstr(buf, want) != 0;
}

static set_t g_ship, g_all, g_complete, g_need;
/* g_complete: kept for the report only; completeness is checked per book. */
int main(int argc, char **argv)
{
    if (argc >= 5 && !strcmp(argv[1], "commands")) {
        static set_t refused, one;
        static char kinds_req[256] = "+";
        read_pairs(argv[2], &g_ship, 2);
        read_pairs(argv[3], &refused, 2);
        if (g_ship.n < 100 || refused.n < 1) red("I123-INPUT", argv[2], "empty or truncated inventory");
        for (int i = 4; i < argc; i++) {
            char spec[512]; snprintf(spec, sizeof spec, "%s", argv[i]);
            char *eq = strchr(spec, '=');
            if (!eq) red("USAGE", argv[i], "want KINDS=path or other=path");
            *eq = 0;
            int complete = strcmp(spec, "other") != 0;
            one.n = 0; read_pairs(eq + 1, &one, 4);
            for (int k = 0; k < one.n; k++) {
                add(&g_all, one.s[k], one.st[k]);
                if (complete && kind_in(spec, kind_of(one.s[k]))) add(&g_complete, one.s[k], one.st[k]);
            }
            if (complete) {
                strncat(kinds_req, spec, sizeof kinds_req - strlen(kinds_req) - 2); strcat(kinds_req, "+");
                for (int k = 0; k < g_ship.n; k++) {
                    if (!kind_in(spec, kind_of(g_ship.s[k]))) continue;
                    add(&g_need, g_ship.s[k], 'G');
                    if (find(&one, g_ship.s[k]) < 0) red("SHIPPED-NOT-DOCUMENTED", eq + 1, g_ship.s[k]);
                }
            }
        }
        if (!g_all.n) red("COMMAND-INDEX", argv[4], "no documented items");
        int drafts = 0;
        for (int k = 0; k < g_all.n; k++) {
            const char *p = g_all.s[k]; int shipped = find(&g_ship, p) >= 0;
            char name[MAXL]; snprintf(name, sizeof name, "%s", p); name[strcspn(name, "|")] = 0;
            char rkey[MAXL + 10]; snprintf(rkey, sizeof rkey, "%s|refused", name);
            int is_refused = !strcmp(kind_of(p), "menu") && find(&refused, rkey) >= 0;
            if (g_all.st[k] == 'D') drafts++;
            if (!shipped && g_all.st[k] != 'D') red("DOC-NOT-SHIPPED", argv[4], p);
            if (!strcmp(kind_of(p), "menu") && shipped) {
                if (is_refused && g_all.st[k] == 'G') red("DOC-GATED-REFUSED", argv[4], p);
                if (!is_refused && g_all.st[k] == 'D') red("STALE-DRAFT", argv[4], p);
            } else if (shipped && g_all.st[k] == 'D') red("STALE-DRAFT", argv[4], p);
        }
        printf(">>> test-pubs-i123: green (%d shipped [%d refusing menu leaves], %d documented, %d DRAFT, %d required of the complete books, both directions)\n",
               g_ship.n, refused.n, g_all.n, drafts, g_need.n);
        return 0;
    }
    if (argc >= 4 && !strcmp(argv[1], "mentions")) {
        static set_t m;
        read_pairs(argv[2], &g_ship, 2);
        for (int i = 3; i < argc; i++) read_pairs(argv[i], &m, 2);
        for (int k = 0; k < m.n; k++) if (find(&g_ship, m.s[k]) < 0) red("MENTION-NOT-SHIPPED", argv[3], m.s[k]);
        printf(">>> test-pubs-i123-mentions: green (%d distinct inline items, all shipped)\n", m.n);
        return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "glossary")) {
        /* CP-03: every \term is a glossary entry, every entry is used. */
        static set_t t, g; FILE *f; char line[512];
        for (int pass = 0; pass < 2; pass++) {
            if (!(f = fopen(argv[2 + pass], "rb"))) red("I123-INPUT", argv[2 + pass], "cannot open");
            while (fgets(line, sizeof line, f)) {
                line[strcspn(line, "\r\n")] = 0;
                for (char *p = line; *p; p++) if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
                if (line[0]) add(pass ? &g : &t, line, 'G');
            }
            fclose(f);
        }
        for (int k = 0; k < t.n; k++) if (find(&g, t.s[k]) < 0) red("TERM-NOT-IN-GLOSSARY", argv[2], t.s[k]);
        for (int k = 0; k < g.n; k++) if (find(&t, g.s[k]) < 0) red("GLOSSARY-TERM-UNUSED", argv[3], g.s[k]);
        printf(">>> test-pubs-i123-glossary: green (%d defined terms == %d glossary entries)\n", t.n, g.n);
        return 0;
    }
    red("USAGE", "i123_gate", "glossary TERMS GLOSS | commands SHIPPED REFUSED K+K=book.cmds... other=book.cmds... | mentions SHIPPED FILES...");
    return 2;
}
