/*
 * harness/diff/dbf_diff/test_canon_salami.c
 *   -- the S7.2 CANON oracle: Michael Bolton's salami-slicing rounding-error
 *      routine and its enforced "too much too fast" bug
 *      (initech-586.2; plan S7.2; label:canon). Builds ON 586.1 (the S7.1 AR
 *      accounting app + INVOICE.DBF schema, already merged + green).
 *
 * FACTORY / GRADER code (CLAUDE.md Law 3): the C harness, NOT the artifact --
 * libc/stdio are fine here. It links the REAL os/samir engine read-only (it edits
 * NO engine source), seeds a fresh INVOICE table in /tmp via the engine writer
 * (the S7.1 schema, reused verbatim), runs the canon finance-charge program
 * (canon/salami.prg) through the interpreter with a capturing host PAL and an
 * injected clock, then diffs the normalized posting-report stdout against the
 * AUTHORED golden (canon/salami.out) and ASSERTS the specific too-much-too-fast
 * skim value is present. Mirrors test_canon_y2k.c (the proven capturing-PAL +
 * normalize + diff pattern) one-for-one.
 *
 * THE CANON BUG (Law 4 -- "Michael Bolton's rounding-error virus"; enforced, not
 * fixed). The Office Space salami-slicing scheme, played completely straight: a
 * finance-charge routine skims the sub-cent rounding remainder from every posting
 * into a hidden suspense account ("BOLTON"). The canonical bug is the movie's
 * panic -- a MISPLACED DECIMAL makes the skim accumulate DOLLARS-scale, not
 * sub-cent-scale, so the suspense balance balloons absurdly fast ("too much too
 * fast"). The routine NEVER says virus/bug/steal; it reads as a straight-faced
 * Initech month-end rounding-adjustment sweep.
 *
 * THE MECHANISM (the misplaced decimal). The program posts each charge ROUNDed to
 * the SCALE memvar (the statement's billing precision in decimal places) and
 * sweeps the remainder below that precision:
 *     CHARGE = AMOUNT * RATE          (full precision, sub-cent fractions)
 *     POSTED = ROUND(CHARGE, SCALE)   (what the customer is billed)
 *     ADJUST = CHARGE - POSTED        (swept into BOLTON)
 * The driver predefines SCALE. The CORRECT statement precision is 2 (cents): then
 * ADJUST is a true SUB-CENT remainder (|ADJUST| <= 0.005) and BOLTON foots to a
 * few thousandths of a dollar. The CANON BUG is a misplaced decimal -- SCALE keyed
 * as 0 (whole dollars) instead of 2 -- so POSTED rounds to the nearest DOLLAR and
 * ADJUST captures the entire SUB-DOLLAR remainder (|ADJUST| up to 0.5), TWO
 * decimal places (100x) too large per posting. BOLTON balloons ~100x faster than
 * a sub-cent skim ever could: that is the "too much too fast" panic.
 *
 * The PROBE that produced the golden values (Law 2 -- the golden reflects what the
 * engine ACTUALLY computes, confirmed before freezing). RATE = 0.015 (1.5%/mo).
 * Of the four S7.1 invoices, A1003 is PAID (.T.) and is skipped; the three open
 * invoices post:
 *     A1001  AMOUNT 1250.00 -> CHARGE 18.7500 POSTED 19.00 ADJUST -0.2500
 *     A1002  AMOUNT  875.50 -> CHARGE 13.1325 POSTED 13.00 ADJUST  0.1325
 *     A1004  AMOUNT   99.99 -> CHARGE  1.4998 POSTED  1.00 ADJUST  0.4998  (*)
 *       (*) the engine evaluates CHARGE = 99.99*0.015 to full precision; the IEEE
 *           double lands a hair below 1.49985, so STR(.,10,4) renders "    1.4998"
 *           and ADJUST = CHARGE - 1 renders "    0.4998" (round-half-+inf at the
 *           4th place applied to the actual double). [probe-confirmed -- the golden
 *           is frozen from THIS engine output, not from hand arithmetic (Law 2)]
 *     BOLTON SUSPENSE ACCOUNT:  0.38   <- -0.25 + 0.1325 + ~0.49985 ~= 0.382,
 *           STR(.,12,2) -> "        0.38". DOLLARS-scale skim off three invoices.
 * Under the CORRECT SCALE=2 the same three invoices sweep only sub-cent
 * remainders and BOLTON foots to 0.00 (probe -DCANON_SALAMI_FIXED: ADJUSTs
 * 0.0000 + 0.0025 - 0.0002 -> STR(.,12,2) "        0.00"). The headline 0.38 vs
 * 0.00 is the misplaced decimal made visible.
 *
 * THE BUG IS ENFORCED (Rule 6, Law-4 flavor) -- the mutant = the bug "FIXED":
 *   Compile the DRIVER with -DCANON_SALAMI_FIXED. The engine is read-only; the
 *   mutation is realized in the GRADER's data path -- the single memvar that
 *   carries the misplaced decimal: SCALE. The UNIT build predefines SCALE=0 (the
 *   misplaced decimal -> dollars-scale skim); the MUTANT build predefines SCALE=2
 *   (the CORRECT cents precision -> the honest sub-cent skim, BOLTON 0.00). With
 *   SCALE corrected the posting report no longer matches the buggy canon golden
 *   and the gate goes RED. A "fix" to the decimal therefore BREAKS the canon gate:
 *   the misplaced decimal is the contract (Law 4 -- enforced, not fixed). The
 *   mutant build also confirms (stderr diagnostic, not a check()) that the honest
 *   BOLTON 0.00 is what now appears -- proving the buggy 0.38 is load-bearing
 *   canon, not incidental.
 *
 * DETERMINISM (Rule 11): the PAL clock is injected; the table is seeded in a fixed
 * order; RATE and SCALE are fixed literals. The report is a pure function of
 * (program text, seeded table, RATE, SCALE) -- no wall clock, no host paths,
 * ASCII-clean (Rule 12).
 *
 * COLLISION-AVOIDANCE: this file is NEW; it edits no engine source and no Makefile.
 * It reuses the S7.1 INVOICE schema (INVNO/CUST/AMOUNT/DUEDATE/PAID) and seeds the
 * table fresh in /tmp each run -- it never touches a corpus golden, and never
 * touches the S7.1 files.
 *
 * Self-verify (paste in the report):
 *   ENG="os/samir/samir_main.c os/samir/cmd/workarea.c os/samir/cmd/nav.c \
 *        os/samir/cmd/flow.c os/samir/cmd/query.c os/samir/cmd/mutate.c \
 *        os/samir/cmd/set.c os/samir/cmd/proc.c os/samir/fs/dbf.c os/samir/fs/dbt.c \
 *        os/samir/fs/ndx.c os/samir/core/eval.c os/samir/core/parse.c \
 *        os/samir/core/lex.c os/samir/core/value.c os/samir/core/rt.c \
 *        os/samir/core/fn_builtins.c os/samir/pal/pal_host.c"
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Iseed \
 *      -Ios/samir/include -Ispec -o /tmp/test_canon_salami \
 *      harness/diff/dbf_diff/test_canon_salami.c $ENG && \
 *   /tmp/test_canon_salami ../dbase3-decomp harness/diff/dbf_diff/canon
 *
 * argv: [1] = sister-corpus base (unused here except for a Tier-2 banner; default
 *             "../dbase3-decomp"); [2] = the canon dir holding salami.{prg,out}
 *             (default: harness/diff/dbf_diff/canon).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S7.2 ("skims the sub-cent rounding
 *     remainder; the canonical too-much-too-fast bug. Oracle: differential
 *     rounding-remainder skim against the in-universe app only").
 *   - os/samir/core/rt.c dec_format (round half toward +inf) + os/samir/core/
 *     fn_builtins.c fn_round / fn_str (the posting + render math; the mechanism).
 *   - harness/diff/dbf_diff/test_canon_y2k.c (the S7.1 oracle; this is its sibling
 *     and reuses its capturing-PAL + normalize + diff pattern and INVOICE schema).
 *   - os/samir/samir_main.c (the REPL: module registration; USE ownership).
 *   - os/samir/include/samir/{interp.h,workarea.h,dbf.h,value.h,rt.h,pal.h}.
 *   - canon/salami.prg (the app) + canon/salami.out (the golden).
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
/* Capturing PAL: delegate every slot to the host PAL EXCEPT conout, which   */
/* is appended to a byte buffer. Mirrors test_canon_y2k.c's cap_pal.         */
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
/* Pass/fail accounting (the seed test_assert.h counter idiom so a           */
/* TEST_SUMMARY line prints and a non-zero exit code blocks a false-green).   */
/* ===================================================================== */

static int g_checks   = 0;
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
/* Normalization (identical rule to test_canon_y2k.c): trim trailing          */
/* whitespace per line, strip trailing blank lines, canonical '\n'. Leading   */
/* whitespace PRESERVED (the STR width pad is load-bearing); the leading blank */
/* line from the first ? is PRESERVED. NUL-terminated result.                 */
/* ===================================================================== */

static void normalize(const char *in, char *out, size_t cap)
{
    size_t oi = 0;
    size_t line_start = 0;
    size_t i = 0;
    while (in[i] != '\0' && oi < cap - 1) {
        char ch = in[i++];
        if (ch == '\r')
            continue;
        if (ch == '\n') {
            while (oi > line_start && (out[oi-1] == ' ' || out[oi-1] == '\t'))
                oi--;
            out[oi++] = '\n';
            line_start = oi;
        } else {
            out[oi++] = ch;
        }
    }
    while (oi > line_start && (out[oi-1] == ' ' || out[oi-1] == '\t'))
        oi--;
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

/* tiny path joiner */
static void joinp(char *out, size_t cap, const char *a, const char *b)
{
    snprintf(out, cap, "%s/%s", a, b);
}

/* ===================================================================== */
/* The INVOICE ledger (the S7.1 schema, reused verbatim).                  */
/*                                                                         */
/* SCHEMA (shared with the S7.1 AR app -- see test_canon_y2k.c):            */
/*   field 0  INVNO    C(5)     invoice number                              */
/*   field 1  CUST     C(10)    customer / account name                     */
/*   field 2  AMOUNT   N(10,2)  invoice amount, dollars and cents           */
/*   field 3  DUEDATE  D        payment due date                            */
/*   field 4  PAID     L        .T. once settled                            */
/*                                                                         */
/* DUEDATE here is fixed-but-irrelevant to the salami report (the finance-   */
/* charge routine does not age); it is seeded with valid dates so the table  */
/* is well-formed. PAID drives which invoices the routine posts: A1003 is    */
/* PAID and is skipped, so three invoices post.                             */
/* ===================================================================== */

#define INV_N 4

static const char *INV_NO[INV_N]   = { "A1001", "A1002", "A1003", "A1004" };
static const char *INV_CUST[INV_N] = { "INITROingp", "ACME CORP ", "GLOBEX INC", "SOYLENT CO" };
static const double INV_AMT[INV_N]  = { 1250.00,  875.50,  4400.00,   99.99 };
static const int INV_YEAR[INV_N]   = { 1999,  1999,  1999,  1999 };
static const int INV_MON [INV_N]   = {   12,     1,    11,     2 };
static const int INV_DAY [INV_N]   = {   15,     5,    30,    10 };
static const char INV_PAID[INV_N]  = { 'F',   'F',   'T',   'F' };

/*
 * make_invoices: build the INVOICE.DBF ledger in /tmp via the engine writer and
 * return the open writable table. The salami report reads AMOUNT and PAID; the
 * seeded data is identical in both builds (the misplaced decimal lives in the
 * SCALE memvar, not in the table -- see set_scale).
 */
static dbf_table *make_invoices(samir_pal_t *pal, const char *path)
{
    dbf_field_spec fs[5];
    dbf_table *t = NULL;
    int i;

    fs[0].name = "INVNO";   fs[0].type = 'C'; fs[0].field_len = 5;  fs[0].dec = 0;
    fs[1].name = "CUST";    fs[1].type = 'C'; fs[1].field_len = 10; fs[1].dec = 0;
    fs[2].name = "AMOUNT";  fs[2].type = 'N'; fs[2].field_len = 10; fs[2].dec = 2;
    fs[3].name = "DUEDATE"; fs[3].type = 'D'; fs[3].field_len = 8;  fs[3].dec = 0;
    fs[4].name = "PAID";    fs[4].type = 'L'; fs[4].field_len = 1;  fs[4].dec = 0;

    if (dbf_create(pal, path, fs, 5, &t) != DBF_OK) return NULL;

    for (i = 0; i < INV_N; i++) {
        xb_val r[5];
        r[0] = xb_c(INV_NO[i], 5);
        r[1] = xb_c(INV_CUST[i], 10);
        r[2] = xb_n(INV_AMT[i]);
        r[3] = xb_d((double)jdn_from_ymd(INV_YEAR[i], INV_MON[i], INV_DAY[i]));
        r[4] = xb_l(INV_PAID[i] == 'T');
        if (dbf_append_rec(t, r, 0) != DBF_OK) { dbf_close(t); return NULL; }
    }
    if (dbf_flush(t) != DBF_OK) { dbf_close(t); return NULL; }
    return t;
}

/*
 * set_params: predefine the memvars the program reads -- the monthly RATE and the
 * billing-precision SCALE -- on the SAME interp before the program body (the
 * memvars persist; verified by the S7.1 ASOF probe).
 *
 *   RATE is fixed in both builds (0.015 = 1.5%/mo).
 *
 *   SCALE carries the misplaced decimal (the only thing that differs):
 *     DEFAULT (the canon bug): SCALE = 0 -- the misplaced decimal. POSTED rounds
 *       to whole DOLLARS, so ADJUST sweeps the sub-DOLLAR remainder (100x too
 *       large) and BOLTON balloons to dollars-scale (0.38). too much too fast.
 *     -DCANON_SALAMI_FIXED (the mutant = the bug "fixed"): SCALE = 2 -- the
 *       CORRECT cents precision. POSTED rounds to the cent the customer is billed,
 *       ADJUST is a true sub-cent remainder, BOLTON foots to 0.00. The report then
 *       no longer matches the buggy canon golden -> RED (canon enforced).
 */
static int set_params(xb_interp *ip)
{
#ifdef CANON_SALAMI_FIXED
    /* MUTANT: the CORRECT statement precision (cents). */
    return proc_run(ip,
        "RATE = 0.015\n"
        "SCALE = 2\n");
#else
    /* CANON BUG: the misplaced decimal -- whole-dollar precision. */
    return proc_run(ip,
        "RATE = 0.015\n"
        "SCALE = 0\n");
#endif
}

/* ===================================================================== */
/* The oracle.                                                             */
/* ===================================================================== */

#define PRG_CAP   8192
#define OUT_CAP   CAP_BUF
#define NORM_CAP  CAP_BUF

/*
 * substr_present: 1 iff `needle` occurs anywhere in `hay` (a localized
 * "this load-bearing buggy line is present" assertion, independent of the
 * whole-transcript diff so the canon skim value is named explicitly).
 */
static int substr_present(const char *hay, const char *needle)
{
    return strstr(hay, needle) != NULL;
}

static int run_canon(samir_pal_t *pal, const char *canon_dir)
{
    char prgpath[1024], goldpath[1024];
    char prg[PRG_CAP];
    char gold[OUT_CAP];
    char ngot[NORM_CAP], nwant[NORM_CAP];
    const char *wpath = "/tmp/test_canon_salami_INVOICE.dbf";
    dbf_table *tbl;
    xb_interp *ip;
    wa_env *env;
    int ok_stdout;

    joinp(prgpath,  sizeof prgpath,  canon_dir, "salami.prg");
    joinp(goldpath, sizeof goldpath, canon_dir, "salami.out");

    if (slurp(prgpath, prg, sizeof prg) < 0) {
        check(0, "canon-salami: read salami.prg");
        return 0;
    }
    if (slurp(goldpath, gold, sizeof gold) < 0) {
        check(0, "canon-salami: read salami.out");
        return 0;
    }

    /* Seed the INVOICE ledger fresh in /tmp (never a corpus golden). */
    tbl = make_invoices(pal, wpath);
    if (!tbl) { check(0, "canon-salami: build INVOICE ledger"); return 0; }

    ip = xb_interp_make(pal);
    if (!ip) { check(0, "canon-salami: xb_interp_make"); dbf_close(tbl); remove(wpath); return 0; }
    query_register(ip); mutate_register(ip); set_register(ip); proc_register(ip);

    env = xb_interp_env(ip);
    /* The driver owns USE (as the REPL does); adopt the writable ledger into
     * area 1 and select it, then the program body walks records. */
    if (wa_adopt_table(env, 1, tbl, NULL, "INVOICE", wpath, NULL, 0) != WA_OK) {
        check(0, "canon-salami: wa_adopt_table INVOICE");
        xb_interp_free(ip); remove(wpath); return 0;
    }
    wa_select(env, 1);

    /* Predefine the posting parameters (RATE + the misplaced-decimal SCALE). */
    (void)set_params(ip);

    cap_clear();
    (void)proc_run(ip, prg);

    normalize(g_cap.buf, ngot,  sizeof ngot);
    normalize(gold,      nwant, sizeof nwant);

    /* ----------------------------------------------------------------------
     * THE CANON GATE. The SAME assertions run in BOTH builds: the posting
     * report MUST match the buggy canon golden, and the specific too-much-too-
     * fast skim value MUST be present. These are what enforce the bug (Law 4).
     *
     *   UNIT build (bug present, SCALE=0): the engine + whole-dollar rounding
     *     produce the dollars-scale skim -> these assertions PASS (green).
     *   MUTANT build (-DCANON_SALAMI_FIXED, SCALE=2): the corrected precision
     *     produces the honest sub-cent skim (BOLTON 0.00), which does NOT match
     *     the buggy golden -> these SAME assertions go RED. A "fix" therefore
     *     breaks the gate: the misplaced decimal is the contract (Law 4).
     * --------------------------------------------------------------------- */
    ok_stdout = (strcmp(ngot, nwant) == 0);
    check(ok_stdout, "canon-salami: posting report matches the canon golden (Tier 0)");
    if (!ok_stdout) report_diff("canon-salami", ngot, nwant);

    /* The load-bearing too-much-too-fast values (cited to the S7.2 probe):
     *   - a per-posting ADJUST of dollars-scale (0.4998 off a $99.99 invoice),
     *     two decimal places too large for a sub-cent rounding remainder; */
    check(substr_present(ngot, "A1004      1.4998        1.00      0.4998"),
          "canon-salami BUG: A1004 sweeps 0.4998 (dollars-scale, not sub-cent)");
    /*   - the headline: BOLTON balloons to 0.38 off just three postings -- a
     *     sub-cent skim could not exceed a few thousandths of a dollar. */
    check(substr_present(ngot, "BOLTON SUSPENSE ACCOUNT:         0.38"),
          "canon-salami BUG: BOLTON balloons to 0.38 (too much too fast)");

#ifdef CANON_SALAMI_FIXED
    /*
     * MUTANT-only DIAGNOSTIC (stderr; NOT a check() -- it must not affect the
     * pass/fail count). Confirms the mutation was LOAD-BEARING: with the decimal
     * "fixed" (SCALE=2) the honest sub-cent skim actually appears (BOLTON 0.00),
     * so the canon assertions above failed for the RIGHT reason -- the corrected
     * report, not a broken harness. (Probe -DCANON_SALAMI_FIXED produced exactly
     * BOLTON SUSPENSE ACCOUNT:         0.00.)
     */
    fprintf(stderr,
        "  canon-salami MUTANT (bug fixed): honest sub-cent skim present? "
        "BOLTON=0.00:%d  (the canon golden no longer matches -> RED, as required).\n",
        substr_present(ngot, "BOLTON SUSPENSE ACCOUNT:         0.00"));
#endif

    xb_interp_free(ip);
    remove(wpath);
    return ok_stdout;
}

int main(int argc, char **argv)
{
    const char *corpus_base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    const char *canon_dir   = (argc > 2) ? argv[2] : "harness/diff/dbf_diff/canon";
    struct pal_host_cfg cfg;
    samir_pal_t *host, *pal;

    memset(&cfg, 0, sizeof cfg);
    /* Injected clock (Rule 11). The salami report does not use DATE(); the clock
     * is pinned only for full determinism. */
    cfg.date_yy   = 99;   /* 1999-12-31 */
    cfg.date_mm   = 12;
    cfg.date_dd   = 31;
    cfg.heap_size = 8u * 1024u * 1024u;

    host = pal_host_make(cfg);
    if (!host) { fprintf(stderr, "FATAL: pal_host_make returned NULL\n"); return 2; }
    pal = cap_pal_make(host);

    run_canon(pal, canon_dir);

    pal_host_free(host);

    /* Tier-2 banner: the real-DBASE.EXE authenticity diff is GATED-env (the plan
     * scopes the salami oracle to the in-universe app only); not attempted here. */
    fprintf(stderr,
        "  SKIP (LOUD): Tier-2 real-DBASE.EXE authenticity diff is GATED-env "
        "(S7.2 oracle is in-universe-app-only; needs dosbox-x + the sister mint "
        "at '%s'); NOT attempted here. Tier-0 canon golden gated above.\n",
        corpus_base);

    /* TEST_SUMMARY-style line on STDOUT (matching every other SAMIR harness) so a
     * Make mutant gate's liveness check sees it; the non-zero exit blocks a
     * false-green. */
    printf("canon-salami: %d checks, %d failures\n", g_checks, g_failures);

    return g_failures == 0 ? 0 : 1;
}
