/* tps_inventory.c -- the SHIPPED inventory of the TPS Report Generator's
 * user-visible messages, generated from the locked spec-data
 * spec/tps_ritual.json (the table TPSREP.COM is linted against by
 * test-tps-report leg [spec]), and the GC-01/GC-02 check of the TPS manual
 * (bead initech-ipu5.14) against it.  Host C only (CLAUDE.md Law 3).
 *
 * Ref: PUB-REF-0001 Sec 11.2 GC-01 (documented = shipped, both directions),
 * GC-02 (messages verbatim: the book prints each message from the TeX
 * table this tool generates from the spec, never retyped), CP-09 (F-OPS
 * completeness); bead initech-5igf (the seven presses are written ONLY in
 * the manual: the count must appear in the procedure, Form TPS-4, the study
 * questions and the final quiz).
 *
 * Inventory names.  A message array whose first line begins "TPS-nnn " is
 * named by that id (TPS-001 .. TPS-008); any other array is named
 * <row id>.<key> (R2.prompt, R2.refuse, R2.accept, R3.prompt, R4.prompt,
 * R5.prompt); the top-level end_line is END.  KIND is always `tps`.
 *
 *   tps_inventory inventory <tps_ritual.json>
 *       NAME|tps|line 1|line 2 ...   (one line per message, spec order)
 *   tps_inventory tex <tps_ritual.json>
 *       \TpsDefine{NAME}{nlines} \TpsDefineLine{NAME}{k}{text} ...
 *       \TpsDefineCount{n}           ({n} in a line becomes \ph{n})
 *   tps_inventory check <inventory> <book main.cmds> <DOS shipped NAME|KIND>
 *       tps pairs: documented => shipped unless DRAFT (DOC-NOT-SHIPPED);
 *       shipped => documented (SHIPPED-NOT-DOCUMENTED).  tpscount: the
 *       four required places (COUNT-PLACEMENT).  Every other pair (TPSREP
 *       external) must be in the DOS shipped inventory, and TPSREP|external
 *       must be documented.
 *   tps_inventory reports <book main.reports> <proved file>...
 *       every file the book prints with \ReportLines (path|first|last) is
 *       one of the proved files: a file a named gate compares byte for byte
 *       with what the running program writes or reads (REPORT-UNPROVED).
 * Diagnostics are "!!! test-pubs RED <TAG>: ..." (the mutants grep them). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXN 256
#define MAXL 8
#define MAXS 160

static void die(const char *tag, const char *path, const char *detail)
{
    fflush(stdout);
    fprintf(stderr, "!!! test-pubs RED %s: %s: %s\n", tag, path, detail);
    exit(1);
}

static char *readfile(const char *p)
{
    FILE *f = fopen(p, "rb");
    long n;
    char *s;
    if (!f) die("INPUT", p, "cannot open");
    if (fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0 || n > 4 * 1024 * 1024) die("INPUT", p, "size");
    rewind(f);
    s = calloc((size_t)n + 1, 1);
    if (!s || fread(s, 1, (size_t)n, f) != (size_t)n) die("INPUT", p, "read");
    fclose(f);
    return s;
}

/* ---- a minimal JSON reader: just enough for the locked spec ---------- */
static const char *J, *JPATH;
static void ws(void) { while (*J && isspace((unsigned char)*J)) J++; }
static void jstring(char *out, size_t cap)
{
    size_t n = 0;
    if (*J != '"') die("SPEC", JPATH, "string expected");
    J++;
    while (*J && *J != '"') {
        char c = *J++;
        if (c == '\\') {
            c = *J++;
            if (c == 'n') c = '\n';
            else if (c != '"' && c != '\\' && c != '/') die("SPEC", JPATH, "unsupported escape");
        }
        if ((unsigned char)c < 32 || (unsigned char)c > 126) die("SPEC", JPATH, "non-ASCII byte");
        if (out) {
            if (n + 1 >= cap) die("SPEC", JPATH, "string too long");
            out[n++] = c;
        }
    }
    if (*J != '"') die("SPEC", JPATH, "unterminated string");
    J++;
    if (out) out[n] = 0;
}

typedef struct { char name[112]; int n; char line[MAXL][MAXS]; } Msg;
static Msg msgs[MAXN];
static int nmsgs, count = -1;

static void value(const char *rowid, const char *key, int depth);

/* Parse one value and record nothing (the first pass of an object). */
static void skipvalue(void)
{
    ws();
    if (*J == '{' || *J == '[') {
        char close = *J == '{' ? '}' : ']';
        J++;
        ws();
        while (*J != close) {
            if (close == '}') { jstring(NULL, 0); ws(); if (*J != ':') die("SPEC", JPATH, "colon expected"); J++; }
            skipvalue();
            ws();
            if (*J == ',') { J++; ws(); }
            else if (*J != close) die("SPEC", JPATH, "comma expected");
        }
        J++;
    } else if (*J == '"') jstring(NULL, 0);
    else if (isdigit((unsigned char)*J) || *J == '-') { strtol(J, (char **)&J, 10); }
    else if (!strncmp(J, "true", 4)) J += 4;
    else if (!strncmp(J, "false", 5)) J += 5;
    else if (!strncmp(J, "null", 4)) J += 4;
    else die("SPEC", JPATH, "unexpected token");
}

/* An array of strings under a message key becomes one inventory message. */
static void msgarray(const char *rowid, const char *key)
{
    Msg *m;
    if (nmsgs == MAXN) die("SPEC", JPATH, "too many messages");
    m = &msgs[nmsgs++];
    memset(m, 0, sizeof *m);
    J++;
    ws();
    while (*J != ']') {
        if (m->n == MAXL) die("SPEC", JPATH, "too many lines in one message");
        jstring(m->line[m->n++], MAXS);
        ws();
        if (*J == ',') { J++; ws(); }
    }
    J++;
    if (!m->n) die("SPEC", JPATH, "empty message");
    if (!strncmp(m->line[0], "TPS-", 4) && isdigit((unsigned char)m->line[0][4]) &&
        isdigit((unsigned char)m->line[0][5]) && isdigit((unsigned char)m->line[0][6]) &&
        m->line[0][7] == ' ')
        { memcpy(m->name, m->line[0], 7); m->name[7] = 0; }
    else {
        if (!*rowid) die("SPEC", JPATH, "message array outside a row with an id");
        snprintf(m->name, sizeof m->name, "%s.%s", rowid, key);
    }
    for (int i = 0; i < nmsgs - 1; i++)
        if (!strcmp(msgs[i].name, m->name)) die("SPEC", JPATH, m->name);
}

static int ismsgkey(const char *k)
{
    return !strcmp(k, "prompt") || !strcmp(k, "refuse") || !strcmp(k, "accept") ||
           !strcmp(k, "refuse_missing_key");
}

static void object(int depth)
{
    /* Two passes over the object: find its "id" first, then descend. */
    char rowid[32] = "";
    const char *start = J, *p;
    char key[64];
    J++;
    ws();
    while (*J != '}') {
        jstring(key, sizeof key);
        ws();
        if (*J != ':') die("SPEC", JPATH, "colon expected");
        J++;
        ws();
        if (!strcmp(key, "id") && *J == '"') jstring(rowid, sizeof rowid);
        else skipvalue();
        ws();
        if (*J == ',') { J++; ws(); }
    }
    p = J + 1;
    J = start + 1;
    ws();
    while (*J != '}') {
        jstring(key, sizeof key);
        ws();
        J++;
        ws();
        if (ismsgkey(key) && *J == '[') msgarray(rowid, key);
        else if (!strcmp(key, "end_line") && depth == 0) {
            Msg *m = &msgs[nmsgs++];
            memset(m, 0, sizeof *m);
            strcpy(m->name, "END");
            jstring(m->line[0], MAXS);
            m->n = 1;
        } else if (!strcmp(key, "count") && depth > 0 && isdigit((unsigned char)*J)) {
            if (count >= 0) die("SPEC", JPATH, "two counts");
            count = (int)strtol(J, (char **)&J, 10);
        } else value(rowid, key, depth + 1);
        ws();
        if (*J == ',') { J++; ws(); }
    }
    J = p;
}

static void value(const char *rowid, const char *key, int depth)
{
    (void)rowid; (void)key;
    ws();
    if (*J == '{') object(depth);
    else if (*J == '[') {
        J++;
        ws();
        while (*J != ']') {
            value("", "", depth);
            ws();
            if (*J == ',') { J++; ws(); }
        }
        J++;
    } else if (*J == '"') jstring(NULL, 0);
    else if (isdigit((unsigned char)*J) || *J == '-') { strtol(J, (char **)&J, 10); }
    else if (!strncmp(J, "true", 4)) J += 4;
    else if (!strncmp(J, "false", 5)) J += 5;
    else if (!strncmp(J, "null", 4)) J += 4;
    else die("SPEC", JPATH, "unexpected token");
}

static void load(const char *path)
{
    char *s = readfile(path);
    J = s;
    JPATH = path;
    ws();
    if (*J != '{') die("SPEC", path, "object expected");
    object(0);
    if (nmsgs < 10) die("SPEC", path, "too few messages (truncated spec?)");
    if (count < 1) die("SPEC", path, "no alignment count (R2 \"count\")");
}

/* TeX-escape one spec line; "{n}" (the program's fill-in) becomes \ph{n}. */
static void texline(const char *s)
{
    for (; *s; s++) {
        if (!strncmp(s, "{n}", 3)) { fputs("\\ph{n}", stdout); s += 2; continue; }
        switch (*s) {
        case '#': case '$': case '%': case '&': case '_': case '{': case '}':
            printf("\\%c", *s); break;
        case '\\': fputs("\\textbackslash{}", stdout); break;
        case '~': fputs("\\textasciitilde{}", stdout); break;
        case '^': fputs("\\textasciicircum{}", stdout); break;
        case ' ': fputs("\\ ", stdout); break;
        default: putchar(*s);
        }
    }
}

/* ---- the check -------------------------------------------------------- */
typedef struct { char item[MAXN * 4][MAXS]; int n; } Set;
static int has(const Set *s, const char *x)
{
    for (int i = 0; i < s->n; i++) if (!strcmp(s->item[i], x)) return 1;
    return 0;
}
static void add(Set *s, const char *x)
{
    if (has(s, x)) return;
    if (s->n == MAXN * 4 || strlen(x) >= MAXS) die("SET", x, "too many / too long");
    strcpy(s->item[s->n++], x);
}
static void lines(const char *path, Set *s, int fields)
{
    char *t = readfile(path), *l = t;
    while (*l) {
        char *e = strchr(l, '\n');
        if (e) *e = 0;
        if (*l && *l != '#') {
            /* keep the first `fields` |-separated fields */
            char buf[MAXS];
            int k = 0;
            size_t i = 0;
            for (; l[i] && i < sizeof buf - 1; i++) {
                if (l[i] == '|' && ++k == fields) break;
                buf[i] = l[i];
            }
            buf[i] = 0;
            add(s, buf);
        }
        if (!e) break;
        l = e + 1;
    }
    free(t);
}

static void check(const char *inv, const char *cmds, const char *dos)
{
    Set ship = {0}, dosship = {0}, doc = {0}, draft = {0}, places = {0};
    static const char *need[] = { "TPS-P-03", "FORM-TPS-4", "STUDY", "QUIZ" };
    char *t, *l;
    int tpsdoc = 0;
    lines(inv, &ship, 2);
    lines(dos, &dosship, 2);
    if (ship.n < 10) die("TPS-INVENTORY", inv, "empty or truncated");
    if (!has(&dosship, "TPSREP|external")) die("TPS-INVENTORY", dos, "TPSREP.COM is not on the shipped volume");
    t = readfile(cmds);
    for (l = t; *l;) {
        char *e = strchr(l, '\n');
        char f[4][MAXS];
        int k = 0;
        const char *q = l;
        if (e) *e = 0;
        if (*l && *l != '#') {
            for (; k < 4; k++) {
                size_t n = strcspn(q, "|");
                if (n >= MAXS) die("COMMAND-INDEX", cmds, l);
                memcpy(f[k], q, n);
                f[k][n] = 0;
                q += n;
                if (*q == '|') q++; else { k++; break; }
            }
            if (k != 4 || *q) die("COMMAND-INDEX", cmds, l);
            if (strcmp(f[2], "GATED") && strcmp(f[2], "DRAFT")) die("COMMAND-INDEX", cmds, l);
            char pair[2 * MAXS + 2];
            snprintf(pair, sizeof pair, "%s|%s", f[0], f[1]);
            if (!strcmp(f[1], "tpscount")) add(&places, f[0]);
            else if (!strcmp(f[1], "tps")) {
                tpsdoc++;
                add(&doc, pair);
                if (!strcmp(f[2], "DRAFT")) add(&draft, pair);
                if (!has(&ship, pair) && strcmp(f[2], "DRAFT")) die("DOC-NOT-SHIPPED", cmds, pair);
            } else {
                add(&doc, pair);
                if (!has(&dosship, pair) && strcmp(f[2], "DRAFT")) die("DOC-NOT-SHIPPED", cmds, pair);
            }
        }
        if (!e) break;
        l = e + 1;
    }
    free(t);
    for (int i = 0; i < ship.n; i++)
        if (!has(&doc, ship.item[i])) die("SHIPPED-NOT-DOCUMENTED", cmds, ship.item[i]);
    if (!has(&doc, "TPSREP|external")) die("SHIPPED-NOT-DOCUMENTED", cmds, "TPSREP|external");
    for (size_t i = 0; i < sizeof need / sizeof need[0]; i++)
        if (!has(&places, need[i])) die("COUNT-PLACEMENT", cmds, need[i]);
    for (int i = 0; i < draft.n; i++)
        printf("!!! test-pubs DRAFT-ENTRY: %s (documented as DRAFT; GC-12 blocks release)\n", draft.item[i]);
    printf(">>> test-pubs-tps: green (%d shipped TPS messages, all documented; %d registrations, %d DRAFT; "
           "alignment count written in %d places; TPSREP documented)\n", ship.n, tpsdoc, draft.n, places.n);
}

static void reports(int argc, char **argv)
{
    char *t = readfile(argv[2]), *l = t;
    int n = 0;
    while (*l) {
        char *e = strchr(l, '\n');
        if (e) *e = 0;
        if (*l) {
            char *bar = strchr(l, '|');
            int ok = 0;
            if (!bar) die("REPORT-INDEX", argv[2], l);
            *bar = 0;
            for (int i = 3; i < argc; i++) if (!strcmp(l, argv[i])) ok = 1;
            if (!ok) die("REPORT-UNPROVED", argv[2], l);
            n++;
        }
        if (!e) break;
        l = e + 1;
    }
    free(t);
    printf(">>> test-pubs-tps: green (%d printed extracts of program files, each file compared with the running program by its gate)\n", n);
}

int main(int argc, char **argv)
{
    if (argc == 3 && !strcmp(argv[1], "inventory")) {
        load(argv[2]);
        for (int i = 0; i < nmsgs; i++) {
            printf("%s|tps", msgs[i].name);
            for (int k = 0; k < msgs[i].n; k++) printf("|%s", msgs[i].line[k]);
            putchar('\n');
        }
    } else if (argc == 3 && !strcmp(argv[1], "tex")) {
        load(argv[2]);
        printf("%% GENERATED by harness/pubs/tps_inventory.c from %s -- do not edit.\n", argv[2]);
        for (int i = 0; i < nmsgs; i++) {
            printf("\\TpsDefine{%s}{%d}\n", msgs[i].name, msgs[i].n);
            for (int k = 0; k < msgs[i].n; k++) {
                printf("\\TpsDefineLine{%s}{%d}{", msgs[i].name, k + 1);
                texline(msgs[i].line[k]);
                fputs("}\n", stdout);
            }
        }
        printf("\\TpsDefineCount{%d}\n", count);
    } else if (argc == 5 && !strcmp(argv[1], "check")) {
        check(argv[2], argv[3], argv[4]);
    } else if (argc >= 4 && !strcmp(argv[1], "reports")) {
        reports(argc, argv);
    } else die("USAGE", "tps_inventory", "inventory|tex <spec> | check <inventory> <cmds> <shipped> | reports <list> <file>...");
    return 0;
}
