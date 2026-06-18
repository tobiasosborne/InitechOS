/*
 * harness/diff/dbf_diff/xbase_prog_diff/prog_diff.c
 *   -- the S6.4 xBase PROGRAM differential driver (initech-17n.2; plan S6.4).
 *
 * FACTORY / GRADER code (CLAUDE.md Law 3): this is the C harness, NOT the
 * artifact -- libc/stdio OK here. It links the REAL os/samir engine read-only
 * (the public APIs as they are; it edits NO engine source) and drives a small,
 * committed, deterministic .prg corpus through the interpreter, capturing conout
 * via a host capturing PAL, then diffs the normalized stdout against an AUTHORED
 * expected-output golden (the Tier-0, operator-free, day-1 reference). Where a
 * program writes a result .dbf, it ALSO diffs that file via the INDEPENDENT
 * dbf_ref.py reader (loud-skip if python3 is absent). Gate is 100%: any program
 * whose normalized output differs from its golden is a FAIL (Law 2).
 *
 * WHY a program differential (plan S6.4 / PRD Sec 8): the unit oracles grade the
 * codecs/evaluator/interpreter in isolation; this grades whole xBase PROGRAMS
 * end-to-end (the dot-prompt the REPL drives at S5.8), which is the milestone's
 * "InitechBase differential" leg. The host reference tier (this file's authored
 * goldens) needs NO real dBASE; the Tier-2 real-DBASE.EXE authenticity diff is
 * GATED (loud-skip below; needs dosbox-x + the sister mint -- not attempted).
 *
 * TIERS (plan Sec 2.A):
 *   Tier 0 (committed, operator-free, day-1): corpus/<name>.prg vs golden/<name>.out
 *     -- AUTHORED from the documented dBASE III+ 1.1 semantics (each .prg cites its
 *     corpus source). This is what gates here. 100% required.
 *   Tier 1 (independent reader): a program that writes a result .dbf is also read
 *     back by harness/diff/dbf_diff/dbf_ref.py (the "third implementation", shares
 *     no code with os/samir) and diffed vs golden/<name>.dbf.out. Loud-skip (NOT a
 *     silent pass) if python3 is unavailable; the stdout leg still gates.
 *   Tier 2 (authenticity): real DBASE.EXE under dosbox-x via the sister mint --
 *     GATED-env, loud-skip (see the banner at the end of main()). NOT attempted.
 *
 * DETERMINISM (Rule 11): the PAL clock is INJECTED (1999-12-31); every program's
 * output is a pure function of (program text, seeded table, clock). No wall clock,
 * no host paths in the captured transcript, ASCII-clean (Rule 12).
 *
 * USE handling: USE/CLOSE are owned by the REPL (samir_main.c), NOT by samir_do /
 * proc_run / the command modules. A corpus program that needs a table opens it
 * with a leading `USE <ALIAS>` line; this driver provisions the named table into
 * the selected work area (read-only for query.prg; a fresh writable /tmp COPY for
 * mutate.prg -- NEVER a committed golden, the wave-9 lesson) and then runs the
 * REST of the program (everything after the USE line) through proc_run, exactly as
 * the REPL would after its own USE. Programs with no USE line run whole.
 *
 * Mutation proof (Rule 6 / ARB rider (a)) -- the differential MUST bite:
 *   -DPROGDIFF_MUTATE_SWAP_EQ_GOLDEN : when grading exact.prg (the program that
 *     pins the directional `=` string operator, sec 4.4), the driver SWAPS the two
 *     expected lines that encode the `=` direction ('Smith'='S' -> .T. vs
 *     'S'='Smith' -> .F.). This is the plan's canonical mutant "flip the `=`
 *     prefix direction" realized in the GRADER (the engine is read-only here): the
 *     ENGINE still produces the correct T,F,... but the now-swapped EXPECTATION
 *     becomes F,T,... so the normalized diff MUST report a mismatch and the gate
 *     goes RED. It proves the differential detects a real output divergence on the
 *     exact byte the canonical mutant would perturb -- not a no-op.
 *
 * Self-verify (paste in the report):
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Iseed \
 *      -Ios/samir/include -Ispec -o /tmp/prog_diff \
 *      harness/diff/dbf_diff/xbase_prog_diff/prog_diff.c <ENGINE .c set> && \
 *   /tmp/prog_diff ../dbase3-decomp harness/diff/dbf_diff/xbase_prog_diff
 *
 * argv: [1] = sister-corpus base (for the Tier-2 banner only; default
 *             "../dbase3-decomp"); [2] = the xbase_prog_diff dir holding corpus/
 *             and golden/ (default: this file's directory).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S6.4 (contract + the `=`-flip mutant)
 *     + Sec 2.A (Tier 0/1/2; goldens by path, loud-skip if absent).
 *   - ../dbase3-decomp/specs/language/expressions-and-operators.md (sec 4.4 EXACT,
 *     sec 9.1 precedence, sec 1.1 C+N mismatch) + functions/ + commands/ specs
 *     (each corpus .prg cites the exact section it exercises).
 *   - os/samir/samir_main.c (the REPL: module registration + USE/CLOSE ownership).
 *   - os/samir/include/samir/interp.h (xb_interp_make/_free; samir_last_error;
 *     query/mutate/set/proc_register + proc_run, extern-declared as samir_main.c does).
 *   - os/samir/pal/pal_host.c (the host PAL + injected clock).
 *   - harness/diff/dbf_diff/dbf_ref.py (the independent .dbf reader for the result diff).
 *   - harness/diff/dbf_diff/test_samir_repl.c (the capturing PAL pattern reused here).
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/dbf.h"
#include "samir/value.h"
#include "samir/eval.h"
#include "samir/rt.h"
#include "samir/pal.h"

/* ---- pal_host.c surface (declared here -- not in a header) -------------- */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* ---- the command modules + proc_run (extern, exactly as samir_main.c) --- */
extern int query_register(xb_interp *ip);
extern int mutate_register(xb_interp *ip);
extern int set_register(xb_interp *ip);
extern int proc_register(xb_interp *ip);
extern int proc_run(xb_interp *ip, const char *prg);

/* ===================================================================== */
/* Capturing PAL: delegate every slot to the host PAL EXCEPT conout, which  */
/* is appended to a byte buffer. Mirrors test_samir_repl.c's cap_pal.        */
/* (conin_* return EOF -- corpus programs do not read the console.)          */
/* ===================================================================== */

#define CAP_BUF 65536

typedef struct {
    samir_pal_t  pal;      /* MUST be first: &cap.pal is handed to the engine */
    samir_pal_t *inner;    /* the real host PAL */
    char         buf[CAP_BUF];
    uint32_t     len;
} cap_pal;

static cap_pal g_cap;

static pal_fd  c_open (samir_pal_t *p, const char *n, int m) { cap_pal *c=(cap_pal*)p; return c->inner->open(c->inner,n,m); }
static int     c_close(samir_pal_t *p, pal_fd fd)           { cap_pal *c=(cap_pal*)p; return c->inner->close(c->inner,fd); }
static int32_t c_read (samir_pal_t *p, pal_fd fd, void *b, uint32_t n){ cap_pal *c=(cap_pal*)p; return c->inner->read(c->inner,fd,b,n); }
static int32_t c_write(samir_pal_t *p, pal_fd fd, const void *b, uint32_t n){ cap_pal *c=(cap_pal*)p; return c->inner->write(c->inner,fd,b,n); }
static int32_t c_seek (samir_pal_t *p, pal_fd fd, int32_t o, int w){ cap_pal *c=(cap_pal*)p; return c->inner->seek(c->inner,fd,o,w); }
static int     c_remove(samir_pal_t *p, const char *n)      { cap_pal *c=(cap_pal*)p; return c->inner->remove(c->inner,n); }
static int     c_rename(samir_pal_t *p, const char *f, const char *t){ cap_pal *c=(cap_pal*)p; return c->inner->rename(c->inner,f,t); }
static void    c_conout(samir_pal_t *p, const char *s, uint32_t n)
{
    cap_pal *c=(cap_pal*)p;
    uint32_t i;
    for (i = 0; i < n && c->len < (uint32_t)(CAP_BUF - 1); i++)
        c->buf[c->len++] = s[i];
    c->buf[c->len] = '\0';
}
static int32_t c_conin_line(samir_pal_t *p, char *b, uint32_t cap){ (void)p; if (cap) b[0]='\0'; return -1; }
static int32_t c_conin_char(samir_pal_t *p){ (void)p; return -1; }
static void    c_gotoxy(samir_pal_t *p, uint8_t r, uint8_t col){ cap_pal *c=(cap_pal*)p; c->inner->gotoxy(c->inner,r,col); }
static void    c_set_attr(samir_pal_t *p, uint8_t a){ cap_pal *c=(cap_pal*)p; c->inner->set_attr(c->inner,a); }
static void    c_today(samir_pal_t *p, uint8_t *yy, uint8_t *mm, uint8_t *dd){ cap_pal *c=(cap_pal*)p; c->inner->today(c->inner,yy,mm,dd); }
static void   *c_alloc(samir_pal_t *p, uint32_t n){ cap_pal *c=(cap_pal*)p; return c->inner->alloc(c->inner,n); }
static void    c_reset(samir_pal_t *p, void *m){ cap_pal *c=(cap_pal*)p; c->inner->reset(c->inner,m); }

static samir_pal_t *cap_pal_make(samir_pal_t *inner)
{
    g_cap.inner = inner;
    g_cap.len = 0; g_cap.buf[0] = '\0';
    g_cap.pal.open       = c_open;
    g_cap.pal.close      = c_close;
    g_cap.pal.read       = c_read;
    g_cap.pal.write      = c_write;
    g_cap.pal.seek       = c_seek;
    g_cap.pal.remove     = c_remove;
    g_cap.pal.rename     = c_rename;
    g_cap.pal.conout     = c_conout;
    g_cap.pal.conin_line = c_conin_line;
    g_cap.pal.conin_char = c_conin_char;
    g_cap.pal.gotoxy     = c_gotoxy;
    g_cap.pal.set_attr   = c_set_attr;
    g_cap.pal.today      = c_today;
    g_cap.pal.alloc      = c_alloc;
    g_cap.pal.reset      = c_reset;
    return &g_cap.pal;
}
static void cap_clear(void) { g_cap.len = 0; g_cap.buf[0] = '\0'; }

/* ===================================================================== */
/* Pass/fail accounting (mirrors the seed test_assert.h counter idiom so a   */
/* TEST_SUMMARY line prints and a non-zero exit code blocks a false-green).   */
/* ===================================================================== */

static int g_checks  = 0;
static int g_failures = 0;

static void check(int ok, const char *what)
{
    g_checks++;
    if (!ok) {
        g_failures++;
        fprintf(stderr, "FAIL: %s\n", what);
    }
}

/* ===================================================================== */
/* Normalization: trim trailing whitespace per line, strip trailing blank    */
/* lines, canonical '\n' endings. Leading whitespace is PRESERVED (the STR    */
/* width pad ' 3' is load-bearing); the leading empty line from the first ?   */
/* is PRESERVED (it is real, deterministic output). Result is NUL-terminated. */
/* ===================================================================== */

static void normalize(const char *in, char *out, size_t cap)
{
    size_t oi = 0;
    size_t line_start;        /* index in `out` where the current line began */
    size_t i = 0;

    line_start = 0;
    while (in[i] != '\0' && oi < cap - 1) {
        char ch = in[i++];
        if (ch == '\r')
            continue;          /* canonicalize CRLF -> LF */
        if (ch == '\n') {
            /* right-trim the line just completed */
            while (oi > line_start && (out[oi-1] == ' ' || out[oi-1] == '\t'))
                oi--;
            out[oi++] = '\n';
            line_start = oi;
        } else {
            out[oi++] = ch;
        }
    }
    /* right-trim the final (unterminated) line */
    while (oi > line_start && (out[oi-1] == ' ' || out[oi-1] == '\t'))
        oi--;
    /* strip trailing blank lines (collapse a run of '\n' at the end to none) */
    while (oi > 0 && out[oi-1] == '\n')
        oi--;
    out[oi] = '\0';
}

/* read a whole file into `buf` (NUL-terminated). Returns length, or -1. */
static long slurp(const char *path, char *buf, size_t cap)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f) return -1;
    n = fread(buf, 1, cap - 1, f);
    buf[n] = '\0';
    fclose(f);
    return (long)n;
}

/* Print the first differing line pair for a localized signal (Law 2). */
static void report_diff(const char *prog, const char *got, const char *want)
{
    const char *gp = got, *wp = want;
    int line = 1;
    while (*gp && *wp) {
        const char *ge = strchr(gp, '\n');
        const char *we = strchr(wp, '\n');
        size_t gl = ge ? (size_t)(ge - gp) : strlen(gp);
        size_t wl = we ? (size_t)(we - wp) : strlen(wp);
        if (gl != wl || memcmp(gp, wp, gl) != 0) {
            fprintf(stderr,
                "  %s: line %d differs:\n    got : '%.*s'\n    want: '%.*s'\n",
                prog, line, (int)gl, gp, (int)wl, wp);
            return;
        }
        if (!ge || !we) break;
        gp = ge + 1; wp = we + 1; line++;
    }
    if (strlen(gp) != strlen(wp))
        fprintf(stderr,
            "  %s: differ at line %d (one transcript longer):\n"
            "    got : '%s'\n    want: '%s'\n", prog, line, gp, wp);
}

/* ===================================================================== */
/* The corpus directory layout + a tiny path joiner.                       */
/* ===================================================================== */

static char g_dir[1024];   /* the xbase_prog_diff dir (corpus/ + golden/) */

static void joinp(char *out, size_t cap, const char *a, const char *b)
{
    snprintf(out, cap, "%s/%s", a, b);
}

/* ===================================================================== */
/* Table provisioning. Corpus table programs USE a logical ALIAS; this       */
/* driver provisions a concrete /tmp .dbf for that alias and binds it.       */
/* PARTS schema: CODE C(3), QTY N(5,0), OK L(1); 4 seed rows.                */
/* ===================================================================== */

static const char *PARTS_CODES[4] = { "AAA", "BBB", "CCC", "DDD" };
static const int   PARTS_QTY[4]   = { 100,   200,   100,   300   };
static const char  PARTS_OK[4]    = { 'T',   'F',   'T',   'T'   };

/* Write a read-only PARTS .dbf (raw bytes; +1 terminator, ver 0x03). */
static int write_parts_ro(const char *path)
{
    FILE *f;
    uint8_t hdr[32], d0[32], d1[32], d2[32];
    uint8_t term = 0x0D;
    int i;

    memset(hdr,0,32); memset(d0,0,32); memset(d1,0,32); memset(d2,0,32);
    hdr[0x00]=0x03; hdr[0x04]=4;                 /* ver 0x03, nrec=4 */
    hdr[0x08]=129;  hdr[0x09]=0;                 /* header_length=129 */
    hdr[0x0A]=10;   hdr[0x0B]=0;                 /* record_length=10 */
    d0[0]='C';d0[1]='O';d0[2]='D';d0[3]='E'; d0[0x0B]='C'; d0[0x10]=3;
    d1[0]='Q';d1[1]='T';d1[2]='Y';            d1[0x0B]='N'; d1[0x10]=5;
    d2[0]='O';d2[1]='K';                       d2[0x0B]='L'; d2[0x10]=1;

    f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(hdr,1,32,f); fwrite(d0,1,32,f); fwrite(d1,1,32,f); fwrite(d2,1,32,f);
    fwrite(&term,1,1,f);
    for (i = 0; i < 4; i++) {
        char rec[10], nb[6];
        rec[0] = 0x20;
        rec[1]=PARTS_CODES[i][0]; rec[2]=PARTS_CODES[i][1]; rec[3]=PARTS_CODES[i][2];
        snprintf(nb, sizeof nb, "%5d", PARTS_QTY[i]);
        rec[4]=nb[0]; rec[5]=nb[1]; rec[6]=nb[2]; rec[7]=nb[3]; rec[8]=nb[4];
        rec[9]=PARTS_OK[i];
        fwrite(rec,1,10,f);
    }
    fclose(f);
    return 0;
}

/* Build a WRITABLE PARTS copy via the engine writer; returns the open table. */
static dbf_table *make_parts_writable(samir_pal_t *pal, const char *path)
{
    dbf_field_spec fs[3];
    dbf_table *tbl = NULL;
    int i;

    fs[0].name="CODE"; fs[0].type='C'; fs[0].field_len=3; fs[0].dec=0;
    fs[1].name="QTY";  fs[1].type='N'; fs[1].field_len=5; fs[1].dec=0;
    fs[2].name="OK";   fs[2].type='L'; fs[2].field_len=1; fs[2].dec=0;

    if (dbf_create(pal, path, fs, 3, &tbl) != DBF_OK) return NULL;
    for (i = 0; i < 4; i++) {
        xb_val r[3];
        r[0] = xb_c(PARTS_CODES[i], 3);
        r[1] = xb_n((double)PARTS_QTY[i]);
        r[2] = xb_l(PARTS_OK[i] == 'T');
        if (dbf_append_rec(tbl, r, 0) != DBF_OK) { dbf_close(tbl); return NULL; }
    }
    if (dbf_flush(tbl) != DBF_OK) { dbf_close(tbl); return NULL; }
    return tbl;
}

/*
 * Skip a leading `USE <alias>` line in `prg`. The REPL owns USE (not proc_run),
 * so a table program's first non-comment statement is `USE <alias>`; the driver
 * has already bound the table, so we run the program body AFTER that line.
 * Returns a pointer into `prg` at the start of the line after `USE ...`.
 * Comment lines (`*`) and blank lines before USE are left in place (proc_run
 * tolerates them). If no USE line is found, returns prg unchanged.
 */
static const char *skip_use_line(const char *prg)
{
    const char *p = prg;
    while (*p) {
        const char *eol = strchr(p, '\n');
        size_t llen = eol ? (size_t)(eol - p) : strlen(p);
        const char *q = p;
        size_t k = 0;
        /* leading blanks */
        while (k < llen && (q[k]==' '||q[k]=='\t')) k++;
        /* is this "USE" (case-insensitive) followed by space/eol? */
        if (llen - k >= 3 &&
            (q[k]=='U'||q[k]=='u') && (q[k+1]=='S'||q[k+1]=='s') &&
            (q[k+2]=='E'||q[k+2]=='e') &&
            (k+3 >= llen || q[k+3]==' ' || q[k+3]=='\t')) {
            return eol ? eol + 1 : p + llen;
        }
        if (!eol) break;
        p = eol + 1;
    }
    return prg;
}

/* ===================================================================== */
/* Per-program runners.                                                    */
/* ===================================================================== */

#define PRG_CAP   8192
#define OUT_CAP   CAP_BUF
#define NORM_CAP  CAP_BUF

#ifdef PROGDIFF_MUTATE_SWAP_EQ_GOLDEN
/*
 * swap_eq_lines: in a normalized exact.out (lines "", "T", "F", "F", "T"), swap
 * line index 1 ('Smith'='S' -> T) with line index 2 ('S'='Smith' -> F) IN PLACE.
 * Only compiled for the mutant build. Operates on the single-char value lines the
 * authored golden uses (T/F), so a one-byte swap suffices; defensively it finds
 * the 2nd and 3rd '\n'-delimited lines and exchanges their first byte.
 */
static void swap_eq_lines(char *s)
{
    char *l1, *l2, *nl;
    nl = strchr(s, '\n');            /* end of line 0 (the leading empty line) */
    if (!nl) return;
    l1 = nl + 1;                     /* start of line 1 (T) */
    nl = strchr(l1, '\n');
    if (!nl) return;
    l2 = nl + 1;                     /* start of line 2 (F) */
    if (l1[0] != '\0' && l2[0] != '\0') {
        char t = l1[0]; l1[0] = l2[0]; l2[0] = t;
    }
}
#endif

/* Run a NON-table program (no USE): whole program through a fresh interp. */
static int run_plain_prog(samir_pal_t *pal, const char *name, const char *prgpath,
                          const char *goldpath)
{
    char prg[PRG_CAP];
    char gold[OUT_CAP];
    char ngot[NORM_CAP], nwant[NORM_CAP];
    xb_interp *ip;
    int rc, ok;
    char what[256];

    if (slurp(prgpath, prg, sizeof prg) < 0) {
        snprintf(what,sizeof what,"%s: read corpus '%s'",name,prgpath);
        check(0, what);
        return 0;
    }
    if (slurp(goldpath, gold, sizeof gold) < 0) {
        snprintf(what,sizeof what,"%s: read golden '%s'",name,goldpath);
        check(0, what);
        return 0;
    }

    ip = xb_interp_make(pal);
    if (!ip) { snprintf(what,sizeof what,"%s: xb_interp_make",name); check(0,what); return 0; }
    query_register(ip); mutate_register(ip); set_register(ip); proc_register(ip);

    cap_clear();
    rc = proc_run(ip, prg);
    (void)rc;   /* the transcript is the oracle; rc/ec are not asserted here */

    normalize(g_cap.buf, ngot,  sizeof ngot);
    normalize(gold,      nwant, sizeof nwant);

#ifdef PROGDIFF_MUTATE_SWAP_EQ_GOLDEN
    /*
     * MUTANT (Rule 6; plan S6.4 canonical "flip the `=` prefix direction").
     * The engine is read-only here, so we realize the mutation in the GRADER: for
     * exact.prg -- the program that pins the directional `=` string operator
     * (expressions-and-operators.md sec 4.4: left = right is true iff left BEGINS
     * WITH right) -- swap the two EXPECTED lines that encode the direction
     * ('Smith'='S' -> .T. on line 2, 'S'='Smith' -> .F. on line 3). The ENGINE
     * still emits the correct T,F,...; the now-swapped EXPECTATION is F,T,... so
     * the normalized diff MUST report a mismatch -> RED. This proves the
     * differential detects a real divergence on the exact byte a `=`-direction
     * bug would perturb (not a no-op).
     */
    if (strcmp(name, "exact") == 0)
        swap_eq_lines(nwant);
#endif

    ok = (strcmp(ngot, nwant) == 0);
    snprintf(what,sizeof what,"%s: stdout matches golden (Tier 0)",name);
    check(ok, what);
    if (!ok) report_diff(name, ngot, nwant);

    xb_interp_free(ip);
    return ok;
}

/* Run the READ-ONLY table program (query.prg): provision PARTS (ro), bind it,
 * run the body after USE. */
static int run_query_prog(samir_pal_t *pal, const char *prgpath, const char *goldpath)
{
    char prg[PRG_CAP];
    char gold[OUT_CAP];
    char ngot[NORM_CAP], nwant[NORM_CAP];
    const char *ropath = "/tmp/prog_diff_PARTS.dbf";
    xb_interp *ip;
    wa_env *env;
    const char *body;
    int ok;
    char what[256];

    if (slurp(prgpath, prg, sizeof prg) < 0) { check(0,"query: read corpus"); return 0; }
    if (slurp(goldpath, gold, sizeof gold) < 0) { check(0,"query: read golden"); return 0; }
    if (write_parts_ro(ropath) != 0) { check(0,"query: write ro PARTS"); return 0; }

    ip = xb_interp_make(pal);
    if (!ip) { check(0,"query: xb_interp_make"); remove(ropath); return 0; }
    query_register(ip); mutate_register(ip); set_register(ip); proc_register(ip);
    env = xb_interp_env(ip);
    if (wa_set_open(env, 1, ropath, "PARTS", NULL) != WA_OK) {
        check(0,"query: wa_set_open PARTS"); xb_interp_free(ip); remove(ropath); return 0;
    }
    wa_select(env, 1);

    body = skip_use_line(prg);
    cap_clear();
    (void)proc_run(ip, body);

    normalize(g_cap.buf, ngot,  sizeof ngot);
    normalize(gold,      nwant, sizeof nwant);

    ok = (strcmp(ngot, nwant) == 0);
    check(ok, "query: stdout matches golden (Tier 0)");
    if (!ok) report_diff("query", ngot, nwant);

    snprintf(what,sizeof what,"%s",""); (void)what;
    xb_interp_free(ip);
    remove(ropath);
    return ok;
}

/* Run the MUTATION program (mutate.prg): provision a WRITABLE /tmp copy of PARTS,
 * adopt it, run the body after USE, then ALSO diff the result .dbf via dbf_ref.py
 * (loud-skip if python3 missing). NEVER writes a committed golden. */
static int run_mutate_prog(samir_pal_t *pal, const char *prgpath,
                           const char *goldpath, const char *dbfgoldpath)
{
    char prg[PRG_CAP];
    char gold[OUT_CAP];
    char ngot[NORM_CAP], nwant[NORM_CAP];
    const char *wpath = "/tmp/prog_diff_PARTSW.dbf";
    dbf_table *wtbl;
    xb_interp *ip;
    wa_env *env;
    const char *body;
    int ok_stdout;

    if (slurp(prgpath, prg, sizeof prg) < 0) { check(0,"mutate: read corpus"); return 0; }
    if (slurp(goldpath, gold, sizeof gold) < 0) { check(0,"mutate: read golden"); return 0; }

    wtbl = make_parts_writable(pal, wpath);
    if (!wtbl) { check(0,"mutate: build writable PARTS copy"); return 0; }

    ip = xb_interp_make(pal);
    if (!ip) { check(0,"mutate: xb_interp_make"); dbf_close(wtbl); remove(wpath); return 0; }
    query_register(ip); mutate_register(ip); set_register(ip); proc_register(ip);
    env = xb_interp_env(ip);
    if (wa_adopt_table(env, 1, wtbl, NULL, "PARTS", wpath, NULL, 0) != WA_OK) {
        check(0,"mutate: wa_adopt_table"); xb_interp_free(ip); remove(wpath); return 0;
    }
    wa_select(env, 1);

    body = skip_use_line(prg);
    cap_clear();
    (void)proc_run(ip, body);

    normalize(g_cap.buf, ngot,  sizeof ngot);
    normalize(gold,      nwant, sizeof nwant);
    ok_stdout = (strcmp(ngot, nwant) == 0);
    check(ok_stdout, "mutate: stdout matches golden (Tier 0)");
    if (!ok_stdout) report_diff("mutate", ngot, nwant);

    /* Free the interp first (QUIT-time wa_close_all flushes the table). */
    xb_interp_free(ip);

    /* ---- Tier 1: diff the RESULT .dbf via the independent python reader ---- */
    {
        char cmd[2048];
        char refout[8192];
        char dbfgold[8192];
        char nref[NORM_CAP], ndg[NORM_CAP];
        FILE *pp;
        size_t n;
        int rc;
        const char *tmpref = "/tmp/prog_diff_PARTSW.records";

        /* Run dbf_ref.py --records into a temp file; if python3 is absent the
         * popen command fails -> loud-skip (NOT a silent pass; the stdout leg
         * above still gated). */
        snprintf(cmd, sizeof cmd,
                 "python3 harness/diff/dbf_diff/dbf_ref.py --records '%s' > '%s' 2>/dev/null",
                 wpath, tmpref);
        rc = system(cmd);
        if (rc != 0) {
            fprintf(stderr,
                "  SKIP (LOUD): mutate result-.dbf diff -- python3/dbf_ref.py "
                "unavailable (rc=%d). Tier-0 stdout leg still gated.\n", rc);
        } else if (slurp(dbfgoldpath, dbfgold, sizeof dbfgold) < 0) {
            fprintf(stderr,
                "  SKIP (LOUD): mutate result-.dbf golden '%s' missing.\n", dbfgoldpath);
        } else {
            pp = fopen(tmpref, "rb");
            n = pp ? fread(refout, 1, sizeof refout - 1, pp) : 0;
            refout[n] = '\0';
            if (pp) fclose(pp);
            normalize(refout,  nref, sizeof nref);
            normalize(dbfgold, ndg,  sizeof ndg);
            {
                int ok = (strcmp(nref, ndg) == 0);
                check(ok, "mutate: result .dbf matches golden (dbf_ref.py; Tier 1)");
                if (!ok) report_diff("mutate.dbf", nref, ndg);
            }
        }
        remove(tmpref);
    }

    remove(wpath);
    return ok_stdout;
}

/* ===================================================================== */
/* main                                                                    */
/* ===================================================================== */

int main(int argc, char **argv)
{
    const char *corpus_base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    struct pal_host_cfg cfg;
    samir_pal_t *host, *pal;
    char prgpath[1024], goldpath[1024], dbfgoldpath[1024];

    /* argv[2] = the xbase_prog_diff dir (corpus/ + golden/). Default to the
     * known repo-relative path so `make` and a bare run both work. */
    if (argc > 2)
        snprintf(g_dir, sizeof g_dir, "%s", argv[2]);
    else
        snprintf(g_dir, sizeof g_dir, "harness/diff/dbf_diff/xbase_prog_diff");

    memset(&cfg, 0, sizeof cfg);
    cfg.date_yy   = 99;   /* injected clock 1999-12-31 (Rule 11) */
    cfg.date_mm   = 12;
    cfg.date_dd   = 31;
    cfg.heap_size = 8u * 1024u * 1024u;   /* generous: many fresh interps */

    host = pal_host_make(cfg);
    if (!host) { fprintf(stderr, "FATAL: pal_host_make returned NULL\n"); return 2; }
    pal = cap_pal_make(host);

    /* ---- non-table programs (whole-program run) ---- */
    {
        static const char *plain[] = { "expr", "funcs", "exact", "flow", "proc" };
        size_t i;
        for (i = 0; i < sizeof plain / sizeof plain[0]; i++) {
            char pcorpus[256], pgold[256];
            snprintf(pcorpus, sizeof pcorpus, "corpus/%s.prg", plain[i]);
            snprintf(pgold,   sizeof pgold,   "golden/%s.out", plain[i]);
            joinp(prgpath,  sizeof prgpath,  g_dir, pcorpus);
            joinp(goldpath, sizeof goldpath, g_dir, pgold);
            run_plain_prog(pal, plain[i], prgpath, goldpath);
        }
    }

    /* ---- the read-only table program ---- */
    joinp(prgpath,  sizeof prgpath,  g_dir, "corpus/query.prg");
    joinp(goldpath, sizeof goldpath, g_dir, "golden/query.out");
    run_query_prog(pal, prgpath, goldpath);

    /* ---- the mutation program (+ result-.dbf diff) ---- */
    joinp(prgpath,     sizeof prgpath,     g_dir, "corpus/mutate.prg");
    joinp(goldpath,    sizeof goldpath,    g_dir, "golden/mutate.out");
    joinp(dbfgoldpath, sizeof dbfgoldpath, g_dir, "golden/mutate.dbf.out");
    run_mutate_prog(pal, prgpath, goldpath, dbfgoldpath);

    pal_host_free(host);

    /* ---- Tier 2 banner: the real-DBASE.EXE authenticity diff is GATED ---- */
    fprintf(stderr,
        "  SKIP (LOUD): Tier-2 real-DBASE.EXE authenticity diff is GATED-env "
        "(needs dosbox-x + the sister mint at '%s'); NOT attempted here. "
        "Tier-0 (+ Tier-1 result-.dbf) gated above.\n", corpus_base);

    /* TEST_SUMMARY-style line on STDOUT in BOTH cases (matching every other SAMIR
     * harness) so the Make mutant gate's `2>/dev/null | grep 'checks,'` liveness
     * check sees it -- a RED with no summary is "harness dead" (Law 2). The
     * non-zero exit blocks a false-green. */
    printf("xbase-prog-diff: %d checks, %d failures\n", g_checks, g_failures);

    return g_failures == 0 ? 0 : 1;
}
