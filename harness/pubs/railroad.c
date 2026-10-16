/* railroad.c -- the Turbo Initech Programmer's Guide grammar: railroad
 * diagrams and the two-way grammar oracle (PUB-REF-0001 NT railroad
 * dialect, CP-18, GC-01; bead initech-ipu5.12).  Factory C (Law 3).
 *
 * Input: an EBNF file (pubs/manuals/turbo-initech-programmers-guide/
 * grammar/tps.ebnf), one rule per production of the shipped parser.
 *
 *   railroad tikz GRAMMAR OUTDIR
 *       writes OUTDIR/rr-<rule>.tex, one TikZ picture per rule, drawn to
 *       the Borland convention: non-terminals in rectangles, symbols in
 *       circles, reserved words in rounded pills, square-cornered 0.5 pt
 *       lines with solid arrowheads, no caption, no number.  Each picture
 *       registers its production (\RegisterCommand, KIND production) so the
 *       documented-vs-shipped gate sees every diagram.
 *   railroad sentences GRAMMAR N OUTDIR
 *       writes N random programs (OUTDIR/s<k>.pas) derived from `program';
 *       generation is seeded deterministically and biased to cover every
 *       alternative, option and repetition; it fails loud if any branch of
 *       any rule was never exercised.  The gate runs the shipped parser on
 *       each: every one must reach TPS-PARSE-OK (grammar => parser).
 *   railroad recognize GRAMMAR LEXDUMP...
 *       reads TPS-LEX dumps of real programs (the replayed book examples,
 *       the corpus, tps.pas itself) and recognizes each token stream from
 *       `program' with the printed rules (packrat, ordered choice, greedy
 *       repetition): parser-accepted programs => grammar.
 * Every failure prints `!!! test-pubs RED <TAG>: ...' and exits 1. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void die(const char *tag, const char *where, const char *detail)
{
    fflush(stdout);
    fprintf(stderr, "!!! test-pubs RED %s: %s: %s\n", tag, where, detail);
    exit(1);
}

/* ---- grammar ------------------------------------------------------------ */
enum { E_TERM, E_NT, E_SEQ, E_ALT, E_OPT, E_REP };
typedef struct E { int k; char *s; struct E **c; int n; int id; } E;
#define MAXR 64
static char *rname[MAXR]; static E *rbody[MAXR]; static int nr;
static int nbranch;
static int lexmode; /* tikz-lexical: lexical rules, drawn only, never registered */

static E *mk(int k) { E *e = calloc(1, sizeof *e); e->k = k; e->id = nbranch++; return e; }
static void push(E *p, E *c) { p->c = realloc(p->c, sizeof(E *) * (size_t)(p->n + 1)); p->c[p->n++] = c; }

static const char *gp, *gpath;
static void skipws(void)
{
    for (;;) {
        while (isspace((unsigned char)*gp)) gp++;
        if (*gp == '#') { while (*gp && *gp != '\n') gp++; continue; }
        break;
    }
}
static E *alt(void);
static E *prim(void)
{
    skipws();
    if (*gp == '"') {
        const char *q = strchr(gp + 1, '"');
        if (!q) die("GRAMMAR", gpath, "unterminated literal");
        E *e = mk(E_TERM); e->s = strndup(gp + 1, (size_t)(q - gp - 1)); gp = q + 1; return e;
    }
    if (*gp == '[' || *gp == '{' || *gp == '(') {
        char close = *gp == '[' ? ']' : *gp == '{' ? '}' : ')';
        int kind = *gp == '[' ? E_OPT : *gp == '{' ? E_REP : -1;
        gp++;
        E *in = alt();
        skipws();
        if (*gp != close) die("GRAMMAR", gpath, "unbalanced bracket");
        gp++;
        if (kind < 0) return in;
        E *e = mk(kind); push(e, in); return e;
    }
    if (islower((unsigned char)*gp)) {
        const char *q = gp;
        while (islower((unsigned char)*q) || *q == '-') q++;
        E *e = mk(E_NT); e->s = strndup(gp, (size_t)(q - gp)); gp = q; return e;
    }
    return NULL;
}
static E *seq(void)
{
    E *s = mk(E_SEQ), *p;
    while ((p = prim())) push(s, p);
    if (s->n == 1) return s->c[0];
    return s;
}
static E *alt(void)
{
    E *a = mk(E_ALT);
    push(a, seq());
    skipws();
    while (*gp == '|') { gp++; push(a, seq()); skipws(); }
    if (a->n == 1) return a->c[0];
    return a;
}
static int rule_index(const char *n)
{
    for (int i = 0; i < nr; i++) if (!strcmp(rname[i], n)) return i;
    return -1;
}
static int lexical(const char *n)
{
    return !strcmp(n, "identifier") || !strcmp(n, "unsigned-integer") || !strcmp(n, "string-literal");
}
static void check_refs(E *e)
{
    if (e->k == E_NT && !lexical(e->s) && rule_index(e->s) < 0) die("GRAMMAR", gpath, e->s);
    for (int i = 0; i < e->n; i++) check_refs(e->c[i]);
}
static void load(const char *path)
{
    FILE *f = fopen(path, "rb"); static char buf[1 << 16]; size_t n;
    if (!f) die("GRAMMAR", path, "cannot open");
    n = fread(buf, 1, sizeof buf - 1, f); fclose(f); buf[n] = 0;
    gp = buf; gpath = path;
    for (;;) {
        skipws();
        if (!*gp) break;
        const char *q = gp;
        while (islower((unsigned char)*q) || *q == '-') q++;
        if (q == gp) die("GRAMMAR", path, "rule name expected");
        if (nr == MAXR) die("GRAMMAR", path, "too many rules");
        rname[nr] = strndup(gp, (size_t)(q - gp)); gp = q; skipws();
        if (*gp != '=') die("GRAMMAR", path, rname[nr]);
        gp++;
        rbody[nr] = alt(); skipws();
        if (*gp != '.') die("GRAMMAR", path, rname[nr]);
        gp++;
        if (rule_index(rname[nr]) >= 0) die("GRAMMAR", path, "rule defined twice");
        nr++;
    }
    if (!lexmode) for (int i = 0; i < nr; i++) check_refs(rbody[i]);
}

/* ---- railroad diagrams -------------------------------------------------- */
/* Helvetica (Heros) advance widths, 1/1000 em, a-z then the symbols. */
static const int hreg[26] = {556,556,500,556,556,278,556,556,222,222,500,222,833,556,556,556,556,333,500,278,556,500,722,500,500,500};
static const int hbold[26] = {556,611,556,611,556,333,611,611,278,278,556,278,889,611,611,611,611,389,556,333,611,556,778,556,556,500};
#define FS 8.5
static double textw(const char *s, int bold)
{
    double w = 0;
    for (; *s; s++) {
        int c = (unsigned char)*s;
        if (c >= 'a' && c <= 'z') w += (bold ? hbold : hreg)[c - 'a'];
        else if (c == '-') w += 333;
        else if (c == ' ') w += 278;
        else w += 600;
    }
    return w * FS / 1000.0;
}
typedef struct { double w, up, dn; } Box;
#define GAP 7.0
#define RAIL 6.0
#define BH 12.0
#define VS 6.0
static int is_word(const char *s) { return isalpha((unsigned char)s[0]); }
static const char *label(const char *s)
{
    if (!strcmp(s, "unsigned-integer")) return "unsigned integer";
    if (!strcmp(s, "string-literal")) return "string literal";
    return s;
}
static Box measure(E *e)
{
    Box b = {0, 0, 0};
    switch (e->k) {
    case E_TERM:
        if (is_word(e->s)) b.w = textw(e->s, 1) + 12;
        else { b.w = textw(e->s, 0) + 7; if (b.w < BH) b.w = BH; }
        b.up = b.dn = BH / 2; break;
    case E_NT:
        b.w = textw(label(e->s), 0) + 10; b.up = b.dn = BH / 2; break;
    case E_SEQ:
        for (int i = 0; i < e->n; i++) {
            Box c = measure(e->c[i]);
            b.w += c.w + (i ? GAP : 0);
            if (c.up > b.up) b.up = c.up;
            if (c.dn > b.dn) b.dn = c.dn;
        }
        break;
    case E_ALT: {
        double maxw = 0, dn = 0;
        for (int i = 0; i < e->n; i++) {
            Box c = measure(e->c[i]);
            if (c.w > maxw) maxw = c.w;
            if (i == 0) { b.up = c.up; dn = c.dn; }
            else dn += VS + c.up + c.dn;
        }
        b.w = maxw + 4 * RAIL; b.dn = dn; break; }
    case E_OPT: {
        Box c = measure(e->c[0]);
        b.w = c.w + 4 * RAIL; b.up = c.up + VS; b.dn = c.dn; break; }
    case E_REP: {
        Box c = measure(e->c[0]);
        b.w = c.w + 4 * RAIL; b.up = c.up + VS; b.dn = c.dn + VS; break; }
    }
    return b;
}
static FILE *out;
static void hline(double x1, double y, double x2, int arrow)
{
    if (x2 - x1 < 0.01) return;
    fprintf(out, "\\draw%s (%.2f,%.2f) -- (%.2f,%.2f);\n", arrow ? "[rr-arrow]" : "", x1, y, x2, y);
}
static void path(const char *s) { fprintf(out, "\\draw %s;\n", s); }
static void draw(E *e, double x, double y);
static void draw_term(E *e, double x, double y, Box b)
{
    if (e->k == E_NT)
        fprintf(out, "\\node[rr-nt,minimum width=%.2fpt] at (%.2f,%.2f) {%s};\n", b.w, x + b.w / 2, y, label(e->s));
    else if (is_word(e->s))
        fprintf(out, "\\node[rr-rw,minimum width=%.2fpt] at (%.2f,%.2f) {%s};\n", b.w, x + b.w / 2, y, e->s);
    else {
        fprintf(out, "\\node[rr-sym,minimum width=%.2fpt] at (%.2f,%.2f) {\\rrsym{", b.w, x + b.w / 2, y);
        for (const char *p = e->s; *p; p++) fprintf(out, "\\char%d\\relax ", (unsigned char)*p);
        fprintf(out, "}};\n");
    }
}
/* Every element is drawn from its entry (x,y) to its exit (x+w,y); the
 * line INTO a box carries the arrowhead (it is drawn by the caller). */
static void draw(E *e, double x, double y)
{
    Box b = measure(e);
    switch (e->k) {
    case E_TERM: case E_NT: draw_term(e, x, y, b); break;
    case E_SEQ: {
        double cx = x;
        for (int i = 0; i < e->n; i++) {
            Box c = measure(e->c[i]);
            if (i) { hline(cx, y, cx + GAP, e->c[i]->k <= E_NT); cx += GAP; }
            draw(e->c[i], cx, y);
            cx += c.w;
        }
        break; }
    case E_ALT: {
        double by = y, inner = b.w - 4 * RAIL;
        char p[256];
        for (int i = 0; i < e->n; i++) {
            Box c = measure(e->c[i]);
            if (i) by -= VS + c.up;
            if (i) {
                snprintf(p, sizeof p, "(%.2f,%.2f) -- (%.2f,%.2f) -- (%.2f,%.2f)", x + RAIL, y, x + RAIL, by, x + 2 * RAIL, by);
                path(p);
                hline(x + 2 * RAIL, by, x + 2 * RAIL, 0);
                snprintf(p, sizeof p, "(%.2f,%.2f) -- (%.2f,%.2f) -- (%.2f,%.2f)", x + 2 * RAIL + c.w, by, x + b.w - RAIL, by, x + b.w - RAIL, y);
                path(p);
            } else {
                hline(x, y, x + 2 * RAIL, 0);
                hline(x + 2 * RAIL + c.w, y, x + b.w, 0);
            }
            if (e->c[i]->k <= E_NT) fprintf(out, "\\draw[rr-arrow] (%.2f,%.2f) -- (%.2f,%.2f);\n", x + RAIL + 0.01, by, x + 2 * RAIL, by);
            draw(e->c[i], x + 2 * RAIL, by);
            if (i == 0 && c.w < inner) hline(x + 2 * RAIL + c.w, y, x + b.w - 2 * RAIL, 0);
            by -= c.dn;
        }
        (void)inner;
        break; }
    case E_OPT: {
        Box c = measure(e->c[0]);
        char p[256];
        double ty = y + c.up + VS;
        snprintf(p, sizeof p, "(%.2f,%.2f) -- (%.2f,%.2f) -- (%.2f,%.2f) -- (%.2f,%.2f)",
                 x + RAIL, y, x + RAIL, ty, x + b.w - RAIL, ty, x + b.w - RAIL, y);
        path(p);
        hline(x, y, x + 2 * RAIL, e->c[0]->k <= E_NT);
        draw(e->c[0], x + 2 * RAIL, y);
        hline(x + 2 * RAIL + c.w, y, x + b.w, 0);
        break; }
    case E_REP: {
        Box c = measure(e->c[0]);
        char p[320];
        double ty = y + c.up + VS, ly = y - c.dn - VS;
        snprintf(p, sizeof p, "(%.2f,%.2f) -- (%.2f,%.2f) -- (%.2f,%.2f) -- (%.2f,%.2f)",
                 x + RAIL, y, x + RAIL, ty, x + b.w - RAIL, ty, x + b.w - RAIL, y);
        path(p);
        snprintf(p, sizeof p, "[rr-arrow] (%.2f,%.2f) -- (%.2f,%.2f) -- (%.2f,%.2f)",
                 x + b.w - RAIL, y, x + b.w - RAIL, ly, x + b.w / 2, ly);
        path(p);
        snprintf(p, sizeof p, "(%.2f,%.2f) -- (%.2f,%.2f) -- (%.2f,%.2f)",
                 x + b.w / 2, ly, x + RAIL, ly, x + RAIL, y);
        path(p);
        hline(x, y, x + 2 * RAIL, e->c[0]->k <= E_NT);
        draw(e->c[0], x + 2 * RAIL, y);
        hline(x + 2 * RAIL + c.w, y, x + b.w, 0);
        break; }
    }
}
/* A top-level sequence wider than the measure is broken into rows; the
 * line drops from the end of one row and returns to the start of the next. */
#define MAXW 285.0
static void diagram(int r, const char *dir)
{
    char fn[512];
    snprintf(fn, sizeof fn, "%s/rr-%s.tex", dir, rname[r]);
    out = fopen(fn, "w");
    if (!out) die("TIKZ", fn, "cannot write");
    fprintf(out, "%% Generated by harness/pubs/railroad.c from the printed grammar; do not edit.\n");
    fprintf(out, "\\%s{%s}{%%\n", lexmode ? "LexicalDiagram" : "RailroadDiagram", rname[r]);
    E *body = rbody[r];
    E one = { E_SEQ, NULL, &body, 1, -1 };
    E *s = body->k == E_SEQ ? body : &one;
    double y = 0, x = 0, lw = textw(rname[r], 0) + 8;
    fprintf(out, "\\node[rr-label,anchor=east] at (%.2f,0) {%s};\n", lw - 4, rname[r]);
    x = lw;
    double startx = lw, rowup = 0, rowdn = 0;
    hline(x, y, x + GAP, s->c[0]->k <= E_NT); x += GAP;
    for (int i = 0; i < s->n; i++) {
        Box c = measure(s->c[i]);
        if (i && x + GAP + c.w > MAXW) {
            /* wrap: down from the row end, back left, down to the new row */
            double ny = y - rowdn - 2 * VS - BH / 2 - 8;
            Box nb = c;
            double mid = y - rowdn - VS;
            char p[320];
            snprintf(p, sizeof p, "(%.2f,%.2f) -- (%.2f,%.2f) -- (%.2f,%.2f) -- (%.2f,%.2f) -- (%.2f,%.2f)",
                     x, y, x + RAIL, y, x + RAIL, mid, startx, mid, startx, ny - 0 + nb.up - nb.up);
            path(p);
            y = ny; x = startx; rowup = rowdn = 0;
            hline(x, y, x + GAP, s->c[i]->k <= E_NT); x += GAP;
        } else if (i) { hline(x, y, x + GAP, s->c[i]->k <= E_NT); x += GAP; }
        draw(s->c[i], x, y);
        x += c.w;
        if (c.up > rowup) rowup = c.up;
        if (c.dn > rowdn) rowdn = c.dn;
    }
    fprintf(out, "\\draw[rr-arrow] (%.2f,%.2f) -- (%.2f,%.2f);\n", x, y, x + GAP + 3, y);
    fprintf(out, "}\n");
    fclose(out);
}

/* ---- sentence generation ----------------------------------------------- */
static unsigned long rng = 1992;
static unsigned rnd(unsigned n) { rng = rng * 6364136223846793005UL + 1442695040888963407UL; return (unsigned)((rng >> 33) % n); }
static int hits[4096];
#define DLIM 12
static int mindepth_rule[MAXR];
static int mind(E *e)
{
    int m = 0, v;
    switch (e->k) {
    case E_TERM: return 0;
    case E_NT: if (lexical(e->s)) return 0; v = mindepth_rule[rule_index(e->s)]; return v >= 1000 ? 1000 : v + 1;
    case E_SEQ: for (int i = 0; i < e->n; i++) { v = mind(e->c[i]); if (v > m) m = v; } return m;
    case E_ALT: m = 1000; for (int i = 0; i < e->n; i++) { v = mind(e->c[i]); if (v < m) m = v; } return m;
    default: return 0;
    }
}
static char sbuf[1 << 16]; static size_t slen;
static void emit(const char *t)
{
    size_t n = strlen(t);
    if (slen + n + 2 >= sizeof sbuf) die("SENTENCE", "generator", "sentence too long");
    memcpy(sbuf + slen, t, n); slen += n; sbuf[slen++] = ' ';
    if (!strcmp(t, ";")) sbuf[slen - 1] = '\n';
}
static const char *idents[] = { "a", "b", "total", "x1", "rec", "Count", "my_name", "Q" };
static void gen(E *e, int depth)
{
    switch (e->k) {
    case E_TERM: emit(e->s); break;
    case E_NT:
        if (!strcmp(e->s, "identifier")) emit(idents[rnd(8)]);
        else if (!strcmp(e->s, "unsigned-integer")) { static const char *v[] = { "0", "7", "42", "2147483647" }; emit(v[rnd(4)]); }
        else if (!strcmp(e->s, "string-literal")) { static const char *v[] = { "'A'", "'it''s'", "''", "'Hello, world'" }; emit(v[rnd(4)]); }
        else gen(rbody[rule_index(e->s)], depth + 1);
        break;
    case E_SEQ: for (int i = 0; i < e->n; i++) gen(e->c[i], depth); break;
    case E_ALT: {
        int pick = -1;
        if (depth > DLIM) {
            int best = 100000;
            for (int i = 0; i < e->n; i++) { int v = mind(e->c[i]); if (v < best) { best = v; pick = i; } }
        } else {
            for (int i = 0; i < e->n && pick < 0; i++) if (!hits[e->c[i]->id]) pick = i;
            if (pick < 0) pick = (int)rnd((unsigned)e->n);
        }
        hits[e->c[pick]->id]++;
        gen(e->c[pick], depth);
        break; }
    case E_OPT: {
        int take = depth > DLIM ? 0 : (!hits[e->id] ? 1 : (int)rnd(2));
        if (take) { hits[e->id]++; gen(e->c[0], depth); } else hits[e->id + 2048]++;
        break; }
    case E_REP: {
        int k = depth > DLIM ? 0 : (!hits[e->id] ? 1 + (int)rnd(2) : (int)rnd(3));
        if (k) hits[e->id]++; else hits[e->id + 2048]++;
        for (int i = 0; i < k; i++) gen(e->c[0], depth);
        break; }
    }
}
static void coverage(E *e, const char *rule)
{
    char d[160];
    if (e->k == E_ALT) for (int i = 0; i < e->n; i++) if (!hits[e->c[i]->id]) {
        snprintf(d, sizeof d, "alternative %d of an alternation never generated", i + 1); die("GRAMMAR-COVERAGE", rule, d); }
    if ((e->k == E_OPT || e->k == E_REP) && (!hits[e->id] || !hits[e->id + 2048]))
        die("GRAMMAR-COVERAGE", rule, "an option or repetition was never both taken and skipped");
    for (int i = 0; i < e->n; i++) coverage(e->c[i], rule);
}
static void sentences(const char *n_s, const char *dir)
{
    int n = atoi(n_s);
    for (int i = 0; i < nr; i++) mindepth_rule[i] = 1000;
    for (int pass = 0; pass < nr + 2; pass++)
        for (int i = 0; i < nr; i++) { int v = mind(rbody[i]); if (v < mindepth_rule[i]) mindepth_rule[i] = v; }
    int prog = rule_index("program");
    if (prog < 0) die("GRAMMAR", gpath, "no rule `program'");
    for (int k = 0; k < n; k++) {
        char fn[512];
        slen = 0;
        gen(rbody[prog], 0);
        sbuf[slen] = 0;
        snprintf(fn, sizeof fn, "%s/s%03d.pas", dir, k);
        FILE *f = fopen(fn, "w");
        if (!f) die("SENTENCE", fn, "cannot write");
        fputs(sbuf, f); fputc('\n', f); fclose(f);
    }
    for (int i = 0; i < nr; i++) coverage(rbody[i], rname[i]);
    printf(">>> railroad: %d programs generated from %d rules; every alternative, option and repetition exercised\n", n, nr);
}

/* ---- recognition ---------------------------------------------------------- */
static char **tk; static int ntk;
static const char *symtok(const char *s)
{
    static const char *map[][2] = { {";","SEMI"},{".","DOT"},{",","COMMA"},{":","COLON"},{":=","ASSIGN"},
        {"(","LPAREN"},{")","RPAREN"},{"+","PLUS"},{"-","MINUS"},{"*","STAR"},{"=","EQ"},{"<>","NE"},
        {"<","LT"},{"<=","LE"},{">","GT"},{">=","GE"},{"[","LBRACKET"},{"]","RBRACKET"},{"..","DOTDOT"} };
    for (size_t i = 0; i < sizeof map / sizeof map[0]; i++) if (!strcmp(map[i][0], s)) return map[i][1];
    die("GRAMMAR", gpath, s);
    return NULL;
}
static int *memo; /* per (rule,pos): 0 unknown, -1 fail, else end+1 */
static int match(E *e, int pos);
static int rule_at(int r, int pos)
{
    int *m = &memo[r * (ntk + 1) + pos];
    if (*m == -2) return -1;          /* left recursion guard */
    if (*m) return *m > 0 ? *m - 1 : -1;
    *m = -2;
    int end = match(rbody[r], pos);
    *m = end >= 0 ? end + 1 : -1;
    return end;
}
static int match(E *e, int pos)
{
    char want[64];
    switch (e->k) {
    case E_TERM:
        if (pos >= ntk) return -1;
        if (is_word(e->s)) snprintf(want, sizeof want, "KW %s", e->s);
        else snprintf(want, sizeof want, "%s", symtok(e->s));
        return !strcmp(tk[pos], want) ? pos + 1 : -1;
    case E_NT:
        if (lexical(e->s)) {
            if (pos >= ntk) return -1;
            const char *pfx = !strcmp(e->s, "identifier") ? "IDENT " : !strcmp(e->s, "unsigned-integer") ? "INTEGER " : "STRING ";
            return !strncmp(tk[pos], pfx, strlen(pfx)) ? pos + 1 : -1;
        }
        return rule_at(rule_index(e->s), pos);
    case E_SEQ:
        for (int i = 0; i < e->n && pos >= 0; i++) pos = match(e->c[i], pos);
        return pos;
    case E_ALT:
        for (int i = 0; i < e->n; i++) { int r = match(e->c[i], pos); if (r >= 0) return r; }
        return -1;
    case E_OPT: { int r = match(e->c[0], pos); return r >= 0 ? r : pos; }
    case E_REP:
        for (;;) { int r = match(e->c[0], pos); if (r < 0 || r == pos) return pos; pos = r; }
    }
    return -1;
}
static int furthest;
static void recognize(int argc, char **argv)
{
    int prog = rule_index("program"), total = 0;
    for (int f = 3; f < argc; f++) {
        FILE *fp = fopen(argv[f], "rb");
        if (!fp) die("RECOGNIZE", argv[f], "cannot open");
        char line[1024]; int in = 0;
        ntk = 0; tk = NULL;
        while (fgets(line, sizeof line, fp)) {
            line[strcspn(line, "\r\n")] = 0;
            if (!strcmp(line, "TPS-LEX-BEGIN")) { in = 1; continue; }
            if (!strcmp(line, "TPS-LEX-END")) break;
            if (!in) continue;
            if (!strncmp(line, "TPS-LEX-ERROR", 13)) die("RECOGNIZE", argv[f], "the program does not lex");
            if (!strcmp(line, "EOF")) continue;
            tk = realloc(tk, sizeof(char *) * (size_t)(ntk + 1));
            tk[ntk++] = strdup(line);
        }
        fclose(fp);
        if (!ntk) die("RECOGNIZE", argv[f], "no TPS-LEX region");
        memo = calloc((size_t)nr * (size_t)(ntk + 1), sizeof *memo);
        int end = rule_at(prog, 0);
        if (end != ntk) {
            char d[256]; int at = end < 0 ? 0 : end;
            /* report the furthest token any rule consumed up to */
            furthest = 0;
            for (int i = 0; i < nr * (ntk + 1); i++) if (memo[i] > 0 && memo[i] - 1 > furthest) furthest = memo[i] - 1;
            snprintf(d, sizeof d, "the printed grammar does not derive this program (stops near token %d of %d: %s)",
                     furthest > at ? furthest : at, ntk, tk[furthest < ntk ? furthest : ntk - 1]);
            die("GRAMMAR-INCOMPLETE", argv[f], d);
        }
        total += ntk;
        free(memo);
        for (int i = 0; i < ntk; i++) free(tk[i]);
        free(tk);
    }
    printf(">>> railroad: %d programs (%d tokens) derived by the printed grammar\n", argc - 3, total);
}

int main(int argc, char **argv)
{
    if (argc < 3) die("USAGE", "railroad", "tikz|sentences|recognize|rules GRAMMAR ...");
    if (!strcmp(argv[1], "tikz-lexical")) lexmode = 1;
    load(argv[2]);
    if ((!strcmp(argv[1], "tikz") || lexmode) && argc == 4) { for (int i = 0; i < nr; i++) diagram(i, argv[3]); }
    else if (!strcmp(argv[1], "sentences") && argc == 5) sentences(argv[3], argv[4]);
    else if (!strcmp(argv[1], "recognize") && argc >= 4) recognize(argc, argv);
    else if (!strcmp(argv[1], "rules")) { for (int i = 0; i < nr; i++) puts(rname[i]); }
    else die("USAGE", "railroad", "tikz|sentences|recognize|rules GRAMMAR ...");
    return 0;
}
