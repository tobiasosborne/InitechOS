/* turbo_gate.c -- the Turbo Initech manuals' shipped inventory and its
 * documented-vs-shipped oracle (PUB-REF-0001 GC-01, GC-02, CP-18; bead
 * initech-ipu5.12).  Factory C only (CLAUDE.md Law 3).
 *
 * The inventory is GENERATED from the product, never typed:
 *   reserved   the words the SHIPPED lexer classifies as reserved: a probe
 *              TPSIN.PAS holding every quoted lower-case word of tps.pas and
 *              seed/lexer.c is lexed by the real Turbo Initech lexer (the
 *              caller passes its TPS-LEX dump); the seed lexer (seed/lexer.c,
 *              linked here) classifies the same words and must agree except
 *              for the one recorded divergence `case' (os/tps/PARSER-TRACE.md:
 *              reserved by TPS, an identifier to the seed);
 *   routine    the standard procedures and functions: the file verbs of
 *              tps.pas NameIsFileBuiltin, the reserved words ParseFactor
 *              parses as NAME(expression), and the reserved words
 *              ParseStatement hands to ParseWrite;
 *   production the grammar productions of tps.pas WriteProductionName;
 *   token      the token-kind names of tps.pas WriteTokenName (they appear in
 *              syntax-error messages);
 *   message    every diagnostic of FailLex / FailType / GenFail /
 *              WriteExpected (LEX-n, TYPE-n, GEN-n, PARSE-CATEGORY, with n
 *              the code constant of the source) and the run-time FILEIO
 *              diagnostic of seed/rt/fileio.asm; `messages' prints the
 *              exact text of each (GC-02);
 *   protocol   the TPS-... bracket and status lines of the compiler output;
 *   dumprow    the row forms of the lexer, parser and symbol dumps;
 *   limit      the fixed capacities (tps.pas constants ending in Max) WITH
 *              their values, so a changed capacity drifts the book;
 *   switch     the command-line options of the genesis compiler INITECHC
 *              (seed/initechc.c);
 *   rtlentry   the global entry points of the run-time objects
 *              (the seed/rt assembly sources) and the string intrinsics tps.pas emits.
 *
 * Modes:
 *   tps_gate inventory tps.pas lexdump initechc.c rt.asm...  -> NAME|KIND
 *   tps_gate messages  tps.pas fileio.asm                    -> ID|text
 *   tps_gate candidates tps.pas seed/lexer.c                 -> probe words
 *   tps_gate commands  shipped book.cmds...    (both directions, DRAFT aware)
 *   tps_gate msgcheck  messages book.msgs...   (verbatim, both directions)
 *   tps_gate mentions  shipped book.tpsmentions...
 * Every failure is one line `!!! test-pubs RED <TAG>: <where>: <detail>'
 * (the harness/pubs/pubs_gate.c convention) and exit 1 (Rule 2). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

#define MAXI 2048
#define MAXN 160
typedef struct { char s[MAXI][MAXN]; int n; } Set;

static void die(const char *tag, const char *where, const char *detail)
{
    fflush(stdout);
    fprintf(stderr, "!!! test-pubs RED %s: %s: %s\n", tag, where, detail);
    exit(1);
}
static int has(const Set *s, const char *x)
{
    for (int i = 0; i < s->n; i++) if (!strcmp(s->s[i], x)) return 1;
    return 0;
}
static void add(Set *s, const char *x)
{
    if (has(s, x)) return;
    if (!*x || strlen(x) >= MAXN || s->n == MAXI) die("SET", x, "empty, long or too many names");
    strcpy(s->s[s->n++], x);
}
static char *slurp(const char *p)
{
    FILE *f = fopen(p, "rb"); long n; char *b;
    if (!f) die("INPUT", p, "cannot open");
    fseek(f, 0, SEEK_END); n = ftell(f); rewind(f);
    if (n < 0 || n > 8 * 1024 * 1024) die("INPUT", p, "size");
    b = calloc((size_t)n + 1, 1);
    if (!b || fread(b, 1, (size_t)n, f) != (size_t)n) die("INPUT", p, "read");
    fclose(f);
    return b;
}
static int cmp(const void *a, const void *b) { return strcmp(a, b); }

static void line_of(const char *p, char *out, size_t cap)
{
    size_t k = 0;
    while (*p && *p != '\n' && k + 1 < cap) out[k++] = *p++;
    out[k] = 0;
}
/* The text of `procedure NAME(' .. the next line that is exactly "end;". */
static char *body(char *src, const char *head, const char *path)
{
    char *p = strstr(src, head), *e, l[256];
    /* skip forward declarations: the heading line ends in "forward;" */
    while (p) { line_of(p, l, sizeof l); if (!strstr(l, "forward;")) break; p = strstr(p + 1, head); }
    if (!p) die("INVENTORY", path, head);
    e = strstr(p, "\nend;\n");
    if (!e) die("INVENTORY", path, "unterminated routine");
    size_t n = (size_t)(e - p);
    char *b = malloc(n + 1);
    memcpy(b, p, n); b[n] = 0;
    return b;
}
/* Next quoted Pascal literal at or after p, into out; returns past it. */
static char *literal(char *p, char *out, size_t cap)
{
    char *q = strchr(p, '\'');
    if (!q) return NULL;
    size_t k = 0;
    for (q++; *q && *q != '\'' && *q != '\n'; q++) if (k + 1 < cap) out[k++] = *q;
    out[k] = 0;
    return *q == '\'' ? q + 1 : NULL;
}
static long constval(char *src, const char *name, const char *path)
{
    char pat[96];
    snprintf(pat, sizeof pat, "\n  %s = ", name);
    char *p = strstr(src, pat);
    if (!p) die("INVENTORY", path, name);
    return strtol(p + strlen(pat), NULL, 10);
}

/* KeywordKind: word -> Tk constant name. */
static int nkw;
static char kwword[64][24], kwtk[64][32];
static void keyword_table(char *src, const char *path)
{
    char *b = body(src, "function KeywordKind(", path), *p = b;
    while ((p = strstr(p, "if Text = '"))) {
        char w[24], tk[32];
        p = literal(p, w, sizeof w);
        if (!p) die("INVENTORY", path, "KeywordKind literal");
        char *t = strstr(p, "KeywordKind := Tk");
        if (!t || sscanf(t + 15, "%31[A-Za-z]", tk) != 1) die("INVENTORY", path, w);
        if (nkw == 64) die("INVENTORY", path, "keyword table too large");
        strcpy(kwword[nkw], w); strcpy(kwtk[nkw], tk); nkw++;
    }
    free(b);
    if (nkw < 30) die("INVENTORY", path, "KeywordKind table missing or truncated");
}
static const char *tk_word(const char *tk)
{
    for (int i = 0; i < nkw; i++) if (!strcmp(kwtk[i], tk)) return kwword[i];
    return NULL;
}

static void put(Set *out, const char *name, const char *kind)
{
    char b[MAXN];
    if (snprintf(b, sizeof b, "%s|%s", name, kind) >= (int)sizeof b) die("INVENTORY", name, "long");
    add(out, b);
}

/* Literal list of a writer routine: every '...' after Write( in its body,
 * except the fallback the routine prints for an unknown code. */
static void writer_literals(char *src, const char *head, const char *skip,
                            const char *kind, Set *out, const char *path)
{
    char *b = body(src, head, path), *p = b, w[MAXN];
    int n = 0;
    while ((p = strstr(p, "Write('"))) {
        p = literal(p, w, sizeof w);
        if (!p) die("INVENTORY", path, head);
        if (strcmp(w, skip)) { put(out, w, kind); n++; }
    }
    free(b);
    if (n < 10) die("INVENTORY", path, head);
}

static void lexdump_reserved(const char *dump, Set *kw)
{
    char *t = slurp(dump), *p = strstr(t, "TPS-LEX-BEGIN\n");
    if (!p || !strstr(t, "TPS-LEX-END")) die("INVENTORY", dump, "no clean TPS-LEX region");
    if (strstr(t, "TPS-LEX-ERROR")) die("INVENTORY", dump, "the probe did not lex cleanly");
    while ((p = strstr(p, "\nKW "))) {
        char w[MAXN];
        if (sscanf(p + 4, "%150s", w) != 1) die("INVENTORY", dump, "KW row");
        add(kw, w);
        p += 4;
    }
    free(t);
}

static int seed_reserved(const char *w)
{
    Lexer lx; Token t;
    lexer_init(&lx, w, strlen(w));
    t = lexer_next(&lx);
    if (t.kind == TOK_ERROR) die("INVENTORY", "seed/lexer.c", w);
    return t.kind != TOK_IDENT;
}

static void inventory(int argc, char **argv)
{
    if (argc < 6) die("USAGE", "inventory", "tps.pas lexdump initechc.c rt.asm...");
    const char *tpsp = argv[2];
    char *src = slurp(tpsp), line[512], w[MAXN];
    Set out = {0}, kw = {0};
    keyword_table(src, tpsp);

    /* reserved: the shipped lexer's verdict; KeywordKind and the seed agree. */
    lexdump_reserved(argv[3], &kw);
    if (kw.n != nkw) {
        snprintf(line, sizeof line, "the lexer reserves %d words, KeywordKind lists %d", kw.n, nkw);
        die("RESERVED-TABLE", tpsp, line);
    }
    for (int i = 0; i < kw.n; i++) {
        if (!has(&kw, kwword[i])) die("RESERVED-TABLE", tpsp, kwword[i]);
        int seed = seed_reserved(kw.s[i]);
        if (!seed && strcmp(kw.s[i], "case"))
            die("SEED-DIVERGENCE", "seed/lexer.c", kw.s[i]);
        if (seed && !strcmp(kw.s[i], "case"))
            die("SEED-DIVERGENCE", "seed/lexer.c", "case is now reserved by the seed too; drop the recorded divergence");
        put(&out, kw.s[i], "reserved");
    }

    /* routine: file verbs, NAME( factors, and the write statements. */
    {
        char *b = body(src, "function NameIsFileBuiltin(", tpsp), *p = b; int n = 0;
        while ((p = strstr(p, "if DumpText = '"))) {
            p = literal(p, w, sizeof w);
            if (!p) die("INVENTORY", tpsp, "NameIsFileBuiltin");
            put(&out, w, "routine"); n++;
        }
        free(b);
        if (n < 5) die("INVENTORY", tpsp, "file verbs missing");
        b = body(src, "procedure ParseFactor(", tpsp); p = b; n = 0;
        while ((p = strstr(p, "TokKind = Tk"))) {
            char tk[32];
            sscanf(p + 10, "%31[A-Za-z]", tk);
            char *next = strstr(p + 12, "TokKind = Tk");
            char *lp = strstr(p, "ExpectToken(TkLParen");
            if (lp && (!next || lp < next) && tk_word(tk)) { put(&out, tk_word(tk), "routine"); n++; }
            p += 12;
        }
        free(b);
        if (n < 3) die("INVENTORY", tpsp, "NAME(expression) factors missing");
        b = body(src, "procedure ParseStatement(", tpsp); p = b; n = 0;
        while ((p = strstr(p, "TokKind = Tk"))) {
            char tk[32];
            sscanf(p + 10, "%31[A-Za-z]", tk);
            line_of(p, line, sizeof line);
            if (strstr(line, "ParseWrite()") && tk_word(tk)) { put(&out, tk_word(tk), "routine"); n++; }
            p += 12;
        }
        free(b);
        if (n != 2) die("INVENTORY", tpsp, "write statements missing");
    }

    writer_literals(src, "procedure WriteProductionName(", "unknown-production", "production", &out, tpsp);
    writer_literals(src, "procedure WriteTokenName(", "UNKNOWN", "token", &out, tpsp);

    /* message ids (texts in `messages'). */
    {
        static const char *heads[] = { "procedure FailLex(", "procedure FailType(",
                                       "procedure GenFail(", "procedure WriteExpected(" };
        static const char *ids[] = { "LEX", "TYPE", "GEN", "PARSE" };
        for (int h = 0; h < 4; h++) {
            char *b = body(src, heads[h], tpsp), *p = b; int n = 0;
            while ((p = strstr(p, "Code = "))) {
                char c[48], id[64];
                p += 7;
                if (sscanf(p, "%47[A-Za-z0-9]", c) != 1) continue;
                if (h == 3) {
                    char *q = strstr(p, "Write('");
                    if (!q || !literal(q, w, sizeof w)) die("INVENTORY", tpsp, c);
                    snprintf(id, sizeof id, "PARSE-%s", w);
                } else {
                    long v = isdigit((unsigned char)c[0]) ? strtol(c, NULL, 10) : constval(src, c, tpsp);
                    snprintf(id, sizeof id, "%s-%ld", ids[h], v);
                }
                put(&out, id, "message"); n++;
            }
            free(b);
            if (n < 4) die("INVENTORY", tpsp, heads[h]);
            if (h < 3) { snprintf(w, sizeof w, "%s-UNKNOWN", ids[h]); put(&out, w, "message"); }
        }
        put(&out, "RUN-FILEIO", "message");
    }

    /* protocol and dump rows: Write/Writeln literals opening with capitals. */
    {
        char *p = src;
        while ((p = strstr(p, "Write"))) {
            char *q = p + 5;
            if (!strncmp(q, "ln", 2)) q += 2;
            p = q;
            if (strncmp(q, "('", 2)) continue;
            if (!literal(q, w, sizeof w)) continue;
            size_t k = strspn(w, "ABCDEFGHIJKLMNOPQRSTUVWXYZ-");
            if (!k) continue;
            if (!strncmp(w, "TPS-", 4)) { w[k] = 0; put(&out, w, "protocol"); }
            else if (w[k] == ' ' && !strchr(w, '-')) { w[k] = 0; put(&out, w, "dumprow"); }
        }
    }

    /* limits with their values. */
    {
        char *p = src;
        while ((p = strstr(p, "Max = "))) {
            char *s = p;
            while (s > src && (isalnum((unsigned char)s[-1]))) s--;
            if (s[-1] == ' ' && s[-2] == ' ' && s[-3] == '\n') {
                char nm[48], up[64]; size_t k = (size_t)(p + 3 - s);
                if (k >= sizeof nm) die("INVENTORY", tpsp, "limit name");
                memcpy(nm, s, k); nm[k] = 0;
                for (size_t i = 0; i <= k; i++) up[i] = (char)toupper((unsigned char)nm[i]);
                /* the value in the books' form: thousands separated by
                 * commas (6,144), as period manuals print capacities */
                long v = strtol(p + 6, NULL, 10);
                char num[32], sep[40]; int len, j = 0;
                len = snprintf(num, sizeof num, "%ld", v);
                for (int d = 0; d < len; d++) {
                    if (d && (len - d) % 3 == 0) sep[j++] = ',';
                    sep[j++] = num[d];
                }
                sep[j] = 0;
                snprintf(w, sizeof w, "%s=%s", up, sep);
                put(&out, w, "limit");
            }
            p += 6;
        }
    }

    /* genesis-compiler switches. */
    {
        char *c = slurp(argv[4]), *p = c; int n = 0;
        while ((p = strstr(p, "strcmp(a, \""))) {
            p += 11;
            char *e = strchr(p, '"');
            if (!e || e - p >= MAXN) die("INVENTORY", argv[4], "switch");
            memcpy(w, p, (size_t)(e - p)); w[e - p] = 0;
            put(&out, w, "switch"); n++;
        }
        free(c);
        if (n < 2) die("INVENTORY", argv[4], "no switches");
    }

    /* run-time entry points. */
    for (int i = 5; i < argc; i++) {
        char *a = slurp(argv[i]), *p = a;
        while ((p = strstr(p, "global "))) {
            if (p == a || p[-1] == '\n') {
                if (sscanf(p + 7, "%150[A-Za-z0-9_]", w) == 1) put(&out, w, "rtlentry");
            }
            p += 7;
        }
        free(a);
    }
    {
        char *p = src;
        while ((p = strstr(p, "__str_"))) {
            size_t k = strspn(p, "abcdefghijklmnopqrstuvwxyz_");
            if (k < MAXN) { memcpy(w, p, k); w[k] = 0; put(&out, w, "rtlentry"); }
            p += k;
        }
    }
    free(src);
    qsort(out.s, (size_t)out.n, sizeof out.s[0], cmp);
    for (int i = 0; i < out.n; i++) puts(out.s[i]);
}

/* ID|text for every diagnostic (GC-02). */
static void messages(int argc, char **argv)
{
    if (argc != 4) die("USAGE", "messages", "tps.pas fileio.asm");
    char *src = slurp(argv[2]), w[MAXN], out[MAXI][MAXN]; int n = 0;
    static const char *heads[] = { "procedure FailLex(", "procedure FailType(",
                                   "procedure GenFail(", "procedure WriteExpected(" };
    static const char *ids[] = { "LEX", "TYPE", "GEN", "PARSE" };
    for (int h = 0; h < 4; h++) {
        char *b = body(src, heads[h], argv[2]), *p = b;
        while ((p = strstr(p, "Code = "))) {
            char c[48];
            p += 7;
            if (sscanf(p, "%47[A-Za-z0-9]", c) != 1) continue;
            char *q = strstr(p, "Write");
            if (!q || !literal(q, w, sizeof w)) die("MESSAGES", argv[2], c);
            char *t = w; while (*t == ' ') t++;
            if (h == 3) snprintf(out[n++], MAXN, "PARSE-%s|expected=%s", t, t);
            else {
                long v = isdigit((unsigned char)c[0]) ? strtol(c, NULL, 10) : constval(src, c, argv[2]);
                snprintf(out[n++], MAXN, "%s-%ld|%s", ids[h], v, t);
            }
        }
        if (h < 3) {
            /* the fallback: the last Writeln literal of the routine */
            char *q = b, *last = NULL;
            while ((q = strstr(q, "else"))) { char *r = strstr(q, "Write"); if (r) last = r; q += 4; }
            if (!last || !literal(last, w, sizeof w)) die("MESSAGES", argv[2], heads[h]);
            char *t = w; while (*t == ' ') t++;
            snprintf(out[n++], MAXN, "%s-UNKNOWN|%s", ids[h], t);
        }
        free(b);
    }
    {
        char *a = slurp(argv[3]), *p = strstr(a, "db '");
        if (!p) die("MESSAGES", argv[3], "no FILEIO diagnostic");
        p += 4;
        char *e = strchr(p, '\'');
        if (!e || e - p >= MAXN) die("MESSAGES", argv[3], "FILEIO diagnostic");
        memcpy(w, p, (size_t)(e - p)); w[e - p] = 0;
        snprintf(out[n++], MAXN, "RUN-FILEIO|%s", w);
        free(a);
    }
    free(src);
    qsort(out, (size_t)n, sizeof out[0], cmp);
    for (int i = 0; i < n; i++) puts(out[i]);
}

/* Probe words: every quoted lower-case word of tps.pas and seed/lexer.c. */
static void candidates(int argc, char **argv)
{
    Set s = {0};
    for (int i = 2; i < argc; i++) {
        char *t = slurp(argv[i]), *p = t;
        char q = strstr(argv[i], ".pas") ? '\'' : '"';
        while ((p = strchr(p, q))) {
            char *e = p + 1;
            while (*e >= 'a' && *e <= 'z') e++;
            if (*e == q && e - p > 1 && e - p < 32) {
                char w[32]; memcpy(w, p + 1, (size_t)(e - p - 1)); w[e - p - 1] = 0;
                add(&s, w);
                p = e + 1;
            } else p++;
        }
        free(t);
    }
    qsort(s.s, (size_t)s.n, sizeof s.s[0], cmp);
    for (int i = 0; i < s.n; i++) puts(s.s[i]);
}

static Set readlist(const char *path)
{
    Set s = {0};
    char *t = slurp(path), *l = t;
    while (*l) {
        char *e = strchr(l, '\n'); if (e) *e = 0;
        /* a trailing blank is not part of a name or a message text */
        for (char *z = l + strlen(l); z > l && z[-1] == ' '; z--) z[-1] = 0;
        if (*l && *l != '#') add(&s, l);
        if (!e) break;
        l = e + 1;
    }
    free(t);
    return s;
}

/* The books' <job>.cmds (initech-syntax.sty \RegisterCommand):
 * NAME|KIND|STATE|ENTRY.  Shipped NAME|KIND pairs must all be documented;
 * documented pairs must be shipped unless the entry is DRAFT (D-14). */
static void commands(int argc, char **argv)
{
    Set ship = readlist(argv[2]), doc = {0}, draft = {0};
    char pair[MAXN], line[MAXN * 2];
    for (int i = 3; i < argc; i++) {
        Set c = readlist(argv[i]);
        for (int j = 0; j < c.n; j++) {
            char f[4][MAXN]; int k = 0; const char *q = c.s[j];
            for (; k < 4; k++) {
                size_t n = strcspn(q, "|");
                if (n >= MAXN) die("COMMAND-INDEX", argv[i], c.s[j]);
                memcpy(f[k], q, n); f[k][n] = 0; q += n;
                if (*q == '|') q++; else { k++; break; }
            }
            if (k != 4 || *q) die("COMMAND-INDEX", argv[i], c.s[j]);
            if (strcmp(f[2], "GATED") && strcmp(f[2], "DRAFT")) die("COMMAND-INDEX", argv[i], c.s[j]);
            snprintf(pair, sizeof pair, "%s|%s", f[0], f[1]);
            add(&doc, pair);
            if (!strcmp(f[2], "DRAFT")) add(&draft, pair);
        }
    }
    if (!ship.n || !doc.n) die("COMMAND-INDEX", argv[3], "empty inventory or empty books");
    for (int i = 0; i < doc.n; i++)
        if (!has(&ship, doc.s[i]) && !has(&draft, doc.s[i])) die("DOC-NOT-SHIPPED", argv[3], doc.s[i]);
    int kinds[16] = {0}; const char *kn[] = { "reserved", "routine", "production", "token", "message",
        "protocol", "dumprow", "limit", "switch", "rtlentry" };
    for (int i = 0; i < ship.n; i++) {
        if (!has(&doc, ship.s[i])) die("SHIPPED-NOT-DOCUMENTED", argv[3], ship.s[i]);
        for (int k = 0; k < 10; k++) {
            snprintf(line, sizeof line, "|%s", kn[k]);
            size_t a = strlen(ship.s[i]), b = strlen(line);
            if (a > b && !strcmp(ship.s[i] + a - b, line)) kinds[k]++;
        }
    }
    for (int i = 0; i < draft.n; i++)
        printf("!!! test-pubs DRAFT-ENTRY: %s (documented as DRAFT; GC-12 blocks release)\n", draft.s[i]);
    printf(">>> test-pubs-turbo-commands: green (%d shipped == documented: %d reserved, %d routines, "
           "%d productions, %d tokens, %d messages, %d protocol, %d dump rows, %d limits, %d switches, "
           "%d run-time entries; %d DRAFT)\n", ship.n, kinds[0], kinds[1], kinds[2], kinds[3], kinds[4],
           kinds[5], kinds[6], kinds[7], kinds[8], kinds[9], draft.n);
}

/* Printed message texts (<job>.msgs, ID|text) equal the shipped texts. */
static void msgcheck(int argc, char **argv)
{
    Set ship = readlist(argv[2]), seen = {0};
    for (int i = 3; i < argc; i++) {
        Set m = readlist(argv[i]);
        for (int j = 0; j < m.n; j++) {
            if (!has(&ship, m.s[j])) die("MESSAGE-TEXT", argv[i], m.s[j]);
            add(&seen, m.s[j]);
        }
    }
    for (int i = 0; i < ship.n; i++) if (!has(&seen, ship.s[i])) die("MESSAGE-UNPRINTED", argv[2], ship.s[i]);
    printf(">>> test-pubs-turbo-messages: green (%d diagnostics printed verbatim)\n", ship.n);
}

/* Inline reserved words and standard names (\rw, \std) are shipped. */
static void mentions(int argc, char **argv)
{
    Set ship = readlist(argv[2]), seen = {0};
    for (int i = 3; i < argc; i++) {
        Set m = readlist(argv[i]);
        for (int j = 0; j < m.n; j++) {
            if (!has(&ship, m.s[j])) die("DOC-NOT-SHIPPED", argv[i], m.s[j]);
            add(&seen, m.s[j]);
        }
    }
    printf(">>> test-pubs-turbo-mentions: green (%d distinct inline Pascal names, all shipped)\n", seen.n);
}

int main(int argc, char **argv)
{
    if (argc >= 2 && !strcmp(argv[1], "inventory")) inventory(argc, argv);
    else if (argc >= 2 && !strcmp(argv[1], "messages")) messages(argc, argv);
    else if (argc >= 3 && !strcmp(argv[1], "candidates")) candidates(argc, argv);
    else if (argc >= 4 && !strcmp(argv[1], "commands")) commands(argc, argv);
    else if (argc >= 4 && !strcmp(argv[1], "msgcheck")) msgcheck(argc, argv);
    else if (argc >= 4 && !strcmp(argv[1], "mentions")) mentions(argc, argv);
    else die("USAGE", "tps_gate", "inventory|messages|candidates|commands|msgcheck|mentions");
    return 0;
}
