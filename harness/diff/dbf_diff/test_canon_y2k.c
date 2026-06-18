/*
 * harness/diff/dbf_diff/test_canon_y2k.c
 *   -- the S7.1 CANON oracle: the Initech accounts-receivable aging app and its
 *      enforced Year-2000 bug (initech-586.1; plan S7.1; label:canon).
 *
 * FACTORY / GRADER code (CLAUDE.md Law 3): the C harness, NOT the artifact --
 * libc/stdio are fine here. It links the REAL os/samir engine read-only (it edits
 * NO engine source), seeds a fresh INVOICE table in /tmp via the engine writer,
 * runs the canon accounting program (canon/y2k_accounting.prg) through the
 * interpreter with a capturing host PAL and an injected clock, then diffs the
 * normalized aging-report stdout against the AUTHORED golden (canon/y2k_accounting.out)
 * and ASSERTS the specific Y2K-buggy values are present. Mirrors the structure of
 * xbase_prog_diff/prog_diff.c (the proven capturing-PAL + normalize + diff pattern).
 *
 * THE CANON BUG (Law 4 -- "the *wrong* rollover matches real dBASE; enforced, not
 * fixed"). This is the genuine DOS/dBASE-era Year-2000 failure mode, played
 * completely straight: the app stores and compares dates whose YEAR was keyed in
 * two digits. Under the dBASE III+ 1.1 default (SET CENTURY OFF), the engine parses
 * a two-digit data-entry year BASE-1900: CTOD('01/05/00') -> the year 1900, NOT
 * 2000 (os/samir/core/fn_builtins.c fn_ctod_impl line 514: "if (yearlen == 2)
 * yy += 1900;" -- no sliding window). So an invoice whose due date is a year-2000
 * date entered as "00" is stored as a 1900 date, and the aging arithmetic
 * (AGE = ASOF - DUEDATE, days) is wrong by ~100 years (~36500 days).
 *
 * The PROBE that produced the golden values (Law 2 -- the golden reflects what the
 * engine ACTUALLY does, confirmed before freezing):
 *   Reporting date ASOF = CTOD('01/31/00')  (intended 2000-01-31; parses to 1900).
 *   Invoice due dates keyed two-digit:
 *     A1001 12/15/99 (1999)   A1002 01/05/00 (2000->1900)
 *     A1003 11/30/99 (1999)   A1004 02/10/00 (2000->1900)
 *   The aging report the engine emits:
 *     A1001  12/15/99  -36477   CURRENT   <- a REAL overdue 1999 invoice, but its
 *     A1003  11/30/99  -36462   CURRENT      age computes as ~ -100 YEARS, so it
 *                                            is mislabeled CURRENT and DROPPED from
 *                                            the overdue total.
 *     A1002  01/05/00      26   CURRENT   <- a 2000 invoice, both ASOF and DUEDATE
 *     A1004  02/10/00     -10   CURRENT      misparsed to 1900, so its age is small
 *                                            but its TRUE age is also wrong.
 *     TOTAL UNPAID OVERDUE:       0.00    <- the headline failure: the report
 *                                            claims nothing is overdue while two
 *                                            large 1999 receivables are months past
 *                                            due. This is exactly how period
 *                                            accounting software under-reported
 *                                            aging at the century boundary.
 *   The app NEVER flags this as a bug; it is a straight-faced Initech AR report.
 *
 * THE BUG IS ENFORCED (Rule 6, Law-4 flavor) -- the mutant = the bug "FIXED":
 *   Compile the DRIVER with -DCANON_Y2K_FIXED. The engine is read-only (we cannot
 *   change CTOD's base-1900 rule), so the mutation is realized in the GRADER's data
 *   path -- the two places the canon bug lives in this app:
 *     (a) DATA ENTRY: the seeded INVOICE table stores the year-2000 due dates with
 *         their CORRECT century (jdn_from_ymd(2000,..)) instead of the base-1900
 *         misparse; and
 *     (b) THE AS-OF DATE: ASOF is injected as the correct 2000-01-31 instead of the
 *         base-1900 misparse of '01/31/00'.
 *   With both corrected, AGE = ASOF - DUEDATE is the TRUE aging, the 1999 invoices
 *   flag OVERDUE, and TOTAL UNPAID OVERDUE becomes non-zero -- so the normalized
 *   report NO LONGER matches the buggy canon golden and the gate goes RED. A "fix"
 *   to the rollover therefore BREAKS the canon gate: the wrong rollover is the
 *   contract (Law 4). The mutant build also asserts the corrected values are present
 *   (so a no-op mutant cannot pass green) and asserts that the buggy golden does NOT
 *   match -- proving the buggy values are load-bearing canon, not incidental.
 *
 * DETERMINISM (Rule 11): the PAL clock is injected; the table is seeded in a fixed
 * order; ASOF is a fixed literal. The report is a pure function of (program text,
 * seeded table, ASOF) -- no wall clock, no host paths, ASCII-clean (Rule 12).
 *
 * COLLISION-AVOIDANCE: this file is NEW; it edits no engine source and no Makefile.
 * The INVOICE schema (INVNO/CUST/AMOUNT/DUEDATE/PAID) is documented here and in the
 * .prg so the next canon bead (586.2, Bolton's salami virus) can reuse the table.
 * Operates ONLY on /tmp copies (the table is built fresh each run) -- it never
 * touches a corpus golden.
 *
 * Self-verify (paste in the report):
 *   ENG="os/samir/samir_main.c os/samir/cmd/workarea.c os/samir/cmd/nav.c \
 *        os/samir/cmd/flow.c os/samir/cmd/query.c os/samir/cmd/mutate.c \
 *        os/samir/cmd/set.c os/samir/cmd/proc.c os/samir/fs/dbf.c os/samir/fs/dbt.c \
 *        os/samir/fs/ndx.c os/samir/core/eval.c os/samir/core/parse.c \
 *        os/samir/core/lex.c os/samir/core/value.c os/samir/core/rt.c \
 *        os/samir/core/fn_builtins.c os/samir/pal/pal_host.c"
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Iseed \
 *      -Ios/samir/include -Ispec -o /tmp/test_canon_y2k \
 *      harness/diff/dbf_diff/test_canon_y2k.c $ENG && \
 *   /tmp/test_canon_y2k ../dbase3-decomp harness/diff/dbf_diff/canon
 *
 * argv: [1] = sister-corpus base (unused here except for a Tier-2 banner; default
 *             "../dbase3-decomp"); [2] = the canon dir holding y2k_accounting.{prg,out}
 *             (default: harness/diff/dbf_diff/canon).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S7.1 ("a real .prg over the engine;
 *     the deliberate 2-digit-year store/compare (00 < 99). Oracle: differential --
 *     the wrong rollover matches real dBASE (Law 4; enforced, not fixed)").
 *   - os/samir/core/fn_builtins.c fn_ctod_impl (base-1900 two-digit-year rule, the
 *     mechanism) + os/samir/core/rt.c jdn_from_ymd (the JDN aging arithmetic).
 *   - ../dbase3-decomp/specs/runtime/dates-and-century.md (SET CENTURY OFF default;
 *     two-digit year is base-1900).
 *   - harness/diff/dbf_diff/xbase_prog_diff/prog_diff.c (the capturing-PAL +
 *     normalize + diff pattern reused here).
 *   - os/samir/samir_main.c (the REPL: module registration; USE ownership).
 *   - os/samir/include/samir/{interp.h,workarea.h,dbf.h,value.h,rt.h,pal.h}.
 *   - canon/y2k_accounting.prg (the app) + canon/y2k_accounting.out (the golden).
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
/* is appended to a byte buffer. Mirrors prog_diff.c's cap_pal.              */
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
/* Normalization (identical rule to prog_diff.c): trim trailing whitespace   */
/* per line, strip trailing blank lines, canonical '\n'. Leading whitespace  */
/* PRESERVED (the STR width pad is load-bearing); the leading blank line from */
/* the first ? is PRESERVED. NUL-terminated result.                          */
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
/* The INVOICE ledger.                                                     */
/*                                                                         */
/* SCHEMA (reusable by 586.2 Bolton's salami virus -- documented here and   */
/* in canon/y2k_accounting.prg; new modules append fields, never renumber): */
/*   field 0  INVNO    C(5)     invoice number                              */
/*   field 1  CUST     C(10)    customer / account name                     */
/*   field 2  AMOUNT   N(10,2)  invoice amount, dollars and cents           */
/*   field 3  DUEDATE  D        payment due date                            */
/*   field 4  PAID     L        .T. once settled                            */
/* ===================================================================== */

#define INV_N 4

static const char *INV_NO[INV_N]   = { "A1001", "A1002", "A1003", "A1004" };
static const char *INV_CUST[INV_N] = { "INITROingp", "ACME CORP ", "GLOBEX INC", "SOYLENT CO" };
static const double INV_AMT[INV_N]  = { 1250.00,  875.50,  4400.00,   99.99 };
/* The clerk keys each due date as MM/DD/YY (two-digit year). The INTENDED
 * calendar years are these; the year-2000 invoices were keyed as "00". */
static const int INV_YEAR[INV_N]   = { 1999,  2000,  1999,  2000 };
static const int INV_MON [INV_N]   = {   12,     1,    11,     2 };
static const int INV_DAY [INV_N]   = {   15,     5,    30,    10 };
static const char INV_PAID[INV_N]  = { 'F',   'F',   'T',   'F' };

/*
 * make_invoices: build the INVOICE.DBF ledger in /tmp via the engine writer and
 * return the open writable table. The DUEDATE field is where the Y2K bug is
 * seeded at "data entry":
 *
 *   DEFAULT (the canon bug, enforced): a year-2000 due date keyed as "00" is
 *     parsed BASE-1900 by CTOD (SET CENTURY OFF), so it is STORED as a 1900 date.
 *     We replicate exactly that: year 2000 -> jdn_from_ymd(1900, ..).
 *
 *   -DCANON_Y2K_FIXED (the mutant = the bug "fixed"): store the year-2000 due
 *     dates with their CORRECT century. The aging then becomes correct and the
 *     report no longer matches the buggy golden -> RED (canon enforced).
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
        int store_year = INV_YEAR[i];
        r[0] = xb_c(INV_NO[i], 5);
        r[1] = xb_c(INV_CUST[i], 10);
        r[2] = xb_n(INV_AMT[i]);
#ifdef CANON_Y2K_FIXED
        /* MUTANT: data entry stores the TRUE century for year-2000 dates. */
        /* (store_year stays the true year) */
#else
        /* CANON BUG: a year-2000 date keyed "00" parses base-1900 (CTOD rule). */
        if (store_year == 2000) store_year = 1900;
#endif
        r[3] = xb_d((double)jdn_from_ymd(store_year, INV_MON[i], INV_DAY[i]));
        r[4] = xb_l(INV_PAID[i] == 'T');
        if (dbf_append_rec(t, r, 0) != DBF_OK) { dbf_close(t); return NULL; }
    }
    if (dbf_flush(t) != DBF_OK) { dbf_close(t); return NULL; }
    return t;
}

/*
 * set_asof: predefine the reporting "as of" date the program reads from the ASOF
 * memvar (the program does NOT hardcode it -- the operator / batch supplies it).
 *
 *   DEFAULT (the canon bug): ASOF is keyed two-digit ('01/31/00') and CTOD parses
 *     it base-1900 -> the 2000-01-31 as-of date is stored as 1900-01-31.
 *
 *   -DCANON_Y2K_FIXED (the mutant): ASOF is the TRUE 2000-01-31 (so the corrected
 *     aging is computed against the right century).
 *
 * The line is run on the SAME interp before the program body; the memvar persists
 * (verified in the S7.1 probe).
 */
static int set_asof(xb_interp *ip)
{
#ifdef CANON_Y2K_FIXED
    /* MUTANT: the true 2000-01-31 as-of date (SET CENTURY ON so the 4-digit year
     * parses to 2000). */
    return proc_run(ip,
        "SET CENTURY ON\n"
        "ASOF = CTOD('01/31/2000')\n"
        "SET CENTURY OFF\n");
#else
    /* CANON BUG: the two-digit as-of date -> base-1900 misparse. */
    return proc_run(ip, "ASOF = CTOD('01/31/00')\n");
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
 * whole-transcript diff so a buggy value is named explicitly in the report).
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
    const char *wpath = "/tmp/test_canon_y2k_INVOICE.dbf";
    dbf_table *tbl;
    xb_interp *ip;
    wa_env *env;
    int ok_stdout;

    joinp(prgpath,  sizeof prgpath,  canon_dir, "y2k_accounting.prg");
    joinp(goldpath, sizeof goldpath, canon_dir, "y2k_accounting.out");

    if (slurp(prgpath, prg, sizeof prg) < 0) {
        check(0, "canon-y2k: read y2k_accounting.prg");
        return 0;
    }
    if (slurp(goldpath, gold, sizeof gold) < 0) {
        check(0, "canon-y2k: read y2k_accounting.out");
        return 0;
    }

    /* Seed the INVOICE ledger fresh in /tmp (never a corpus golden). */
    tbl = make_invoices(pal, wpath);
    if (!tbl) { check(0, "canon-y2k: build INVOICE ledger"); return 0; }

    ip = xb_interp_make(pal);
    if (!ip) { check(0, "canon-y2k: xb_interp_make"); dbf_close(tbl); remove(wpath); return 0; }
    query_register(ip); mutate_register(ip); set_register(ip); proc_register(ip);

    env = xb_interp_env(ip);
    /* The driver owns USE (as the REPL does); adopt the writable ledger into
     * area 1 and select it, then the program body walks records. */
    if (wa_adopt_table(env, 1, tbl, NULL, "INVOICE", wpath, NULL, 0) != WA_OK) {
        check(0, "canon-y2k: wa_adopt_table INVOICE");
        xb_interp_free(ip); remove(wpath); return 0;
    }
    wa_select(env, 1);

    /* Predefine the reporting date (the program reads the ASOF memvar). */
    (void)set_asof(ip);

    cap_clear();
    (void)proc_run(ip, prg);

    normalize(g_cap.buf, ngot,  sizeof ngot);
    normalize(gold,      nwant, sizeof nwant);

    /* ----------------------------------------------------------------------
     * THE CANON GATE. The SAME assertions run in BOTH builds: the aging report
     * MUST match the buggy canon golden, and the specific Y2K-buggy values MUST
     * be present. These are what enforce the bug (Law 4).
     *
     *   UNIT build (bug present): the engine + 2-digit-parsed data produce the
     *     buggy report -> these assertions PASS (green).
     *   MUTANT build (-DCANON_Y2K_FIXED, the bug "fixed"): the corrected data
     *     path produces the TRUE aging (A1001/A1003 flag OVERDUE, overdue total
     *     1250.00), which does NOT match the buggy golden -> these SAME assertions
     *     go RED. A "fix" therefore breaks the gate: the wrong rollover is the
     *     contract (Law 4 -- enforced, not fixed).
     * --------------------------------------------------------------------- */
    ok_stdout = (strcmp(ngot, nwant) == 0);
    check(ok_stdout, "canon-y2k: aging report matches the canon golden (Tier 0)");
    if (!ok_stdout) report_diff("canon-y2k", ngot, nwant);

    /* The load-bearing buggy values (cited to the S7.1 probe):
     *   - a REAL 1999 overdue invoice computes a ~ -100-year age (-36477 / -36462)
     *     and is mislabeled CURRENT (the rollover failure); */
    check(substr_present(ngot, "A1001  12/15/99  -36477   CURRENT"),
          "canon-y2k BUG: A1001 (1999) mis-ages to -36477 days, mislabeled CURRENT");
    check(substr_present(ngot, "A1003  11/30/99  -36462   CURRENT"),
          "canon-y2k BUG: A1003 (1999) mis-ages to -36462 days, mislabeled CURRENT");
    /*   - the headline failure: the overdue total is 0.00 while two large 1999
     *     receivables are months past due. */
    check(substr_present(ngot, "TOTAL UNPAID OVERDUE:       0.00"),
          "canon-y2k BUG: overdue total wrongly reports 0.00 (Y2K mis-aging hides 1999 debt)");

#ifdef CANON_Y2K_FIXED
    /*
     * MUTANT-only DIAGNOSTIC (stderr; NOT a check() -- it must not affect the
     * pass/fail count). Confirms the mutation was LOAD-BEARING: with the rollover
     * "fixed", the corrected aging actually appears (A1001 47 OVERDUE, A1003 62
     * OVERDUE, total 1250.00), so the four canon assertions above failed for the
     * RIGHT reason -- the corrected report, not a broken harness. (Probe
     * -DCANON_Y2K_FIXED produced exactly these lines.)
     */
    fprintf(stderr,
        "  canon-y2k MUTANT (bug fixed): corrected report present? "
        "A1001=47/OVERDUE:%d  A1003=62/OVERDUE:%d  TOTAL=1250.00:%d "
        "(the canon golden no longer matches -> RED, as required).\n",
        substr_present(ngot, "A1001  12/15/99      47   OVERDUE"),
        substr_present(ngot, "A1003  11/30/99      62   OVERDUE"),
        substr_present(ngot, "TOTAL UNPAID OVERDUE:    1250.00"));
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
    /* Injected clock (Rule 11). The host PAL maps yy 00..99 -> 1900..1999, so the
     * canon app does NOT use DATE(); it takes the reporting date from the ASOF
     * memvar the driver predefines. The clock is pinned for full determinism. */
    cfg.date_yy   = 99;   /* 1999-12-31 */
    cfg.date_mm   = 12;
    cfg.date_dd   = 31;
    cfg.heap_size = 8u * 1024u * 1024u;

    host = pal_host_make(cfg);
    if (!host) { fprintf(stderr, "FATAL: pal_host_make returned NULL\n"); return 2; }
    pal = cap_pal_make(host);

    run_canon(pal, canon_dir);

    pal_host_free(host);

    /* Tier-2 banner: the real-DBASE.EXE authenticity diff (the *wrong* rollover
     * also occurs in real dBASE III+ 1.1) is GATED-env; not attempted here. */
    fprintf(stderr,
        "  SKIP (LOUD): Tier-2 real-DBASE.EXE authenticity diff is GATED-env "
        "(needs dosbox-x + the sister mint at '%s'); NOT attempted here. "
        "Tier-0 canon golden gated above.\n", corpus_base);

    /* TEST_SUMMARY-style line on STDOUT (matching every other SAMIR harness) so a
     * Make mutant gate's liveness check sees it; the non-zero exit blocks a
     * false-green. */
    printf("canon-y2k: %d checks, %d failures\n", g_checks, g_failures);

    return g_failures == 0 ? 0 : 1;
}
