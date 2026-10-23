/* SAMIR data typing vs real dBASE III PLUS (initech-ylqs).
 *
 * Audit docs/audits/2026-10-23-samir/REPORT.md:
 *   L001 REPLACE <date> WITH a blank/invalid CTOD() wrote ",)/-1124" into the
 *        D field (the in-memory blank date, JDN 0, was formatted as a date).
 *   L006 an asterisk-filled (overflowed) N field read back as an ordinary 0.0.
 *   L016 INDEX ON <date> failed with #9 when a record held a blank date (the
 *        codec handed a blank D field to the interpreter as XB_U, not a date).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. The dot-prompt session
 * (samir_repl) replays the SAME commands the real III+ mint ran, on the SAME
 * input fixture, in a fresh build/samirdt/<case> directory; the results are
 * graded against REAL III+ bytes, never against SAMIR output (Law 2):
 *   harness/diff/dbf_diff/xbase_prog_diff/datatype-mint/  (datatype_mint.sh:
 *   DBASE.EXE from ../dbase3-decomp/mint/work under dosbox-x; DT1..DT5.PRG;
 *   EDGE.DBF/.DBT = the audit's evidence/fixtures input; outputs E1/E2/E4/E5
 *   .DBF, BLANKDAT.NDX, DT1..DT5LOG.TXT, pinned by sha256.txt).
 * NDX comparison uses the independent ndx_ref.py (ADR-0008 DEC-05), as
 * test_samir_dml.c does.
 *
 * Real III+ facts graded here (all minted, see the DTnLOG.TXT files):
 *   - REPLACE WHEN WITH CTOD("  /  /  ") / CTOD("") stores 8 spaces (E4 rec 1,2;
 *     DT2 E1/E2) -- also the locked spec/samir/xbase_coercion.json
 *     assignment_coercion D<-D "blank":"8 spaces" and dbf.md sec 5 D.
 *   - A blank D field is still a Date: TYPE("WHEN") = "D" (DT3 G8).
 *   - "****" in N(4,1) reads back as the largest double, 1.7976931348623157E308
 *     (DT5 I1: x/10^300 = 179769313.49; I2 x > 10^300; I7 = VAL("****")), so
 *     ? SMALL prints "****" and SMALL = 0 is .F. (DT1 B4/B5, DT2 F2).
 *   - INDEX ON WHEN over a table with a blank date succeeds; the blank key is
 *     the double 1E+100 and sorts after every real date (BLANKDAT.NDX; DT1 D1).
 *
 * Mutants (Rule 6), compiled into the SAMIR sources:
 *   SAMIR_DT_MUTANT_blankwrite  dbf.c writes the JDN-0 blank as a date again
 *   SAMIR_DT_MUTANT_starread    dbf.c reads "****" as 0.0 again
 *   SAMIR_DT_MUTANT_blankread   dbf.c reads a blank D field as XB_U again
 *   SAMIR_DT_MUTANT_blankkey    mutate.c keys a blank date as 0.0, not 1E+100
 * ASCII-clean (Rule 12). No timestamps/host paths in the artifact (Rule 11).
 */
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_assert.h"
#include "samir/interp.h"
TEST_HARNESS();
struct pal_host_cfg { uint8_t date_yy,date_mm,date_dd; uint32_t heap_size; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(samir_pal_t *);

#define GOLD "harness/diff/dbf_diff/xbase_prog_diff/datatype-mint"
#define REC_WHEN 294u   /* EDGE.DBF field offsets, incl. the delete flag */
#define REC_SMALL 290u

static char captured[65536];
static unsigned used, line_no;
static const char *const *script;
static xb_interp *state_ip;
static const char *state_name;

static void output(samir_pal_t *p, const char *s, uint32_t n)
{
    (void)p;
    if (n < sizeof captured - used) { memcpy(captured + used, s, n); used += n; captured[used] = 0; }
}

static const char *const blankdate_cmds[] = {
    "USE E4", "GO 1", "REPLACE WHEN WITH CTOD(\"  /  /  \")",
    "GO 2", "REPLACE WHEN WITH CTOD(\"\")",
    "GO 3", "REPLACE WHEN WITH CTOD(\"02/30/92\")",
    "USE", "USE E4", "GO 1", "W = WHEN", NULL };
static const char *const overflow_cmds[] = {
    "USE E5", "GO 1", "? \"B1 [\", SMALL, \"]\"", "REPLACE SMALL WITH 99999",
    "USE", "USE E5", "GO 1", "X = SMALL", "? \"B4 [\", SMALL, \"]\"",
    "? \"B5 \", SMALL = 0", "? \"F2 \", SMALL > 1000000", NULL };
static const char *const dateindex_cmds[] = {
    "USE E3", "INDEX ON WHEN TO BLANKDAT", "USE", NULL };
/* Same table, record pointer ON the blank record first: III+ INDEX builds
 * over the whole file (Using III+ U5-131), so the key type is the field's
 * Date type whichever record is current; graded against the same golden. */
static const char *const dateindex_blank_cmds[] = {
    "USE E3", "GO 3", "INDEX ON WHEN TO BLANKDAT", "USE", NULL };

static int32_t input(samir_pal_t *p, char *s, uint32_t n)
{
    const char *v = script[line_no];
    (void)p;
    if (v) { line_no++; snprintf(s, n, "%s", v); return (int32_t)strlen(s); }
    /* end of script, before QUIT closes the work areas: grade memvars. */
    if (!strcmp(state_name, "blankdate")) {
        xb_val w;
        CHECK(xb_interp_get_memvar(state_ip, "W", &w) == 0 && w.t == XB_D && w.u.d <= 0.0,
              "L001 blank D field reads as a blank Date (real III+ TYPE(\"WHEN\")=\"D\", DT3 G8)");
    } else if (!strcmp(state_name, "overflow")) {
        xb_val x;
        CHECK(xb_interp_get_memvar(state_ip, "X", &x) == 0 && x.t == XB_N && x.u.n == DBL_MAX,
              "L006 \"****\" N field reads as 1.7976931348623157E308 (real III+ DT5 I1/I2/I7)");
    }
    return -1;
}

static int command(const char *s)
{
    int rc = system(s);
    if (rc) fprintf(stderr, "command failed: %s\n", s);
    CHECK(rc == 0, "factory command succeeded");
    return rc;
}

static size_t readfile(const char *path, unsigned char *s, size_t cap)
{
    FILE *f = fopen(path, "rb"); size_t n = 0;
    if (f) { n = fread(s, 1, cap - 1, f); fclose(f); }
    s[n] = 0;
    return n;
}

static unsigned u16(const unsigned char *p) { return p[0] + 256u * p[1]; }

/* bytes [off, off+len) of physical record `rec` (1-based) of a .dbf image */
static const unsigned char *rec_bytes(const unsigned char *d, size_t n, unsigned rec, unsigned off, unsigned len)
{
    size_t at;
    if (n < 12) return NULL;
    at = u16(d + 8) + (size_t)(rec - 1u) * u16(d + 10) + off;
    return at + len <= n ? d + at : NULL;
}

/* the whole line of `text` that starts with `prefix`, into `out` */
static int line_of(const char *text, const char *prefix, char *out, size_t cap)
{
    const char *p = text;
    size_t k = strlen(prefix);
    while (p && *p) {
        const char *e = strpbrk(p, "\r\n");
        size_t len = e ? (size_t)(e - p) : strlen(p);
        if (len >= k && !memcmp(p, prefix, k) && len < cap) { memcpy(out, p, len); out[len] = 0; return 1; }
        p = e ? e + 1 : NULL;
    }
    return 0;
}

/* captured SAMIR output carries the same line the real III+ log does. `?`
 * emits CR LF BEFORE its items (III+), so the next dot prompt ". " follows
 * the text on the same captured line; only prompt bytes may trail it. */
static int line_matches(const char *got, const char *want)
{
    size_t k = strlen(want);
    if (strncmp(got, want, k)) return 0;
    for (got += k; *got; got++) if (*got != '.' && *got != ' ') return 0;
    return 1;
}
/* captured SAMIR output carries the same line the real III+ log does */
static void same_line(const char *log, const char *prefix, const char *why)
{
    static unsigned char gold[8192];
    char path[512], want[256], got[256];
    snprintf(path, sizeof path, GOLD "/%s", log);
    readfile(path, gold, sizeof gold);
    if (!line_of((char *)gold, prefix, want, sizeof want)) { CHECK(0, "golden log line present"); return; }
    if (!line_of(captured, prefix, got, sizeof got) || !line_matches(got, want))
        fprintf(stderr, "  %s: got [%s] want [%s]\n", why, line_of(captured, prefix, got, sizeof got) ? got : "(none)", want);
    CHECK(line_of(captured, prefix, got, sizeof got) && line_matches(got, want), why);
}

static int valid_d_bytes(const unsigned char *b)
{
    static const int mdays[13] = { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int i, y = 0, m, d;
    if (!memcmp(b, "        ", 8)) return 1;
    for (i = 0; i < 8; i++) if (b[i] < '0' || b[i] > '9') return 0;
    for (i = 0; i < 4; i++) y = y * 10 + (b[i] - '0');
    m = (b[4] - '0') * 10 + (b[5] - '0');
    d = (b[6] - '0') * 10 + (b[7] - '0');
    if (m < 1 || m > 12 || d < 1 || d > mdays[m]) return 0;
    if (m == 2 && d == 29 && !((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 0;
    return 1;
}

int main(int argc, char **argv)
{
    const char *name = argc > 1 ? argv[1] : "blankdate";
    const char *table;
    char dir[512], cmd[4096], path[1024], cwd[2048];
    static unsigned char got[65536], want[65536];
    size_t gn, wn;
    samir_pal_t *pal; xb_interp *ip;

    if (!strcmp(name, "blankdate"))            { script = blankdate_cmds; table = "E4"; }
    else if (!strcmp(name, "overflow"))        { script = overflow_cmds; table = "E5"; }
    else if (!strcmp(name, "dateindex"))       { script = dateindex_cmds; table = "E3"; }
    else if (!strcmp(name, "dateindex-blank")) { script = dateindex_blank_cmds; table = "E3"; }
    else return 2;
    state_name = name;
    snprintf(path, sizeof path, GOLD "/sha256.txt");
    if (!readfile(path, got, sizeof got)) { fprintf(stderr, "RED: MINT required: " GOLD " (datatype_mint.sh)\n"); return 2; }
    snprintf(cmd, sizeof cmd, "cd " GOLD " && sha256sum --quiet -c sha256.txt");
    if (command(cmd)) { fprintf(stderr, "RED: committed real III+ goldens do not match sha256.txt\n"); return 2; }

    snprintf(dir, sizeof dir, "build/samirdt/%s", name);
    snprintf(cmd, sizeof cmd, "rm -rf %s && mkdir -p %s && cp -f " GOLD "/EDGE.DBF %s/%s.DBF && cp -f " GOLD "/EDGE.DBT %s/%s.DBT",
             dir, dir, dir, table, dir, table);
    if (command(cmd)) return 2;

    if (!getcwd(cwd, sizeof cwd) || chdir(dir)) return 2;
    pal = pal_host_make((struct pal_host_cfg){86, 7, 24, 2 * 1024 * 1024});
    if (!pal) return 2;
    pal->conout = output; pal->conin_line = input;
    ip = xb_interp_make(pal); state_ip = ip;
    CHECK(ip && samir_repl(pal, ip) == 0, "dot-prompt session returned");
    if (ip) xb_interp_free(ip);
    pal_host_free(pal);
    if (chdir(cwd)) return 2;
    snprintf(path, sizeof path, "%s/captured.out", dir);
    { FILE *f = fopen(path, "wb"); if (f) { fwrite(captured, 1, strlen(captured), f); fclose(f); } }

    snprintf(path, sizeof path, "%s/%s.DBF", dir, table); gn = readfile(path, got, sizeof got);

    if (!strcmp(name, "blankdate")) {
        const unsigned char *g1, *w1, *g3;
        unsigned rec;
        wn = readfile(GOLD "/E4.DBF", want, sizeof want);
        for (rec = 1; rec <= 2; rec++) {
            g1 = rec_bytes(got, gn, rec, 0, 313); w1 = rec_bytes(want, wn, rec, 0, 313);
            CHECK(g1 && w1 && !memcmp(g1 + REC_WHEN, "        ", 8),
                  "L001 REPLACE WHEN WITH a blank CTOD() stores 8 spaces (dbf.md sec 5 D)");
            CHECK(g1 && w1 && !memcmp(g1, w1, 313),
                  "L001 whole record matches real III+ E4.DBF after the blank REPLACE");
        }
        g3 = rec_bytes(got, gn, 3, REC_WHEN, 8);
        if (g3 && !valid_d_bytes(g3)) fprintf(stderr, "  rec 3 WHEN bytes [%.8s]\n", (const char *)g3);
        CHECK(g3 && valid_d_bytes(g3),
              "L001 REPLACE WHEN WITH CTOD(\"02/30/92\") leaves valid D bytes (blank or YYYYMMDD)");
    } else if (!strcmp(name, "overflow")) {
        const unsigned char *g = rec_bytes(got, gn, 1, REC_SMALL, 4), *w;
        wn = readfile(GOLD "/E5.DBF", want, sizeof want);
        w = rec_bytes(want, wn, 1, REC_SMALL, 4);
        CHECK(g && w && !memcmp(g, w, 4) && !memcmp(g, "****", 4),
              "L006 REPLACE SMALL WITH 99999 stores **** like real III+ E5.DBF");
        same_line("DT1LOG.TXT", "B4 [", "L006 ? SMALL prints **** after reopen (real III+ DT1 B4/B6)");
        same_line("DT1LOG.TXT", "B5 ", "L006 SMALL = 0 is .F. (real III+ DT1 B5)");
        same_line("DT2LOG.TXT", "F2 ", "L006 SMALL > 1000000 is .T. (real III+ DT2 F2)");
        same_line("DT1LOG.TXT", "B1 [", "control: an ordinary N(4,1) still prints 1.2 (real III+ DT1 B1)");
    } else {
        snprintf(path, sizeof path, "%s/BLANKDAT.NDX", dir); gn = readfile(path, got, sizeof got);
        wn = readfile(GOLD "/BLANKDAT.NDX", want, sizeof want);
        CHECK(!strstr(captured, "mismatch"), "L016 INDEX ON a date with a blank date raises no #9 (real III+ DT1 D1)");
        CHECK(gn >= 24 && wn >= 24 && !memcmp(got + 12, want + 12, 8),
              "L016 NDX key_length/keys_per_page/key_type/group_length match real III+ BLANKDAT.NDX");
        snprintf(cmd, sizeof cmd,
                 "python3 -I harness/diff/dbf_diff/ndx_ref.py --index-dump %s/BLANKDAT.NDX > %s/got.dump && "
                 "python3 -I harness/diff/dbf_diff/ndx_ref.py --index-dump " GOLD "/BLANKDAT.NDX > %s/want.dump && "
                 "cmp %s/got.dump %s/want.dump", dir, dir, dir, dir, dir);
        CHECK(gn > 0 && system(cmd) == 0,
              "L016 BLANKDAT.NDX entries (blank key 1E+100 last) match real III+ via ndx_ref.py");
    }
    return TEST_SUMMARY("test-samir-datatype");
}
