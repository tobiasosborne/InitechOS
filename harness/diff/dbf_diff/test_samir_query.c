/*
 * harness/diff/dbf_diff/test_samir_query.c -- host oracle for the SAMIR
 * query/display module (os/samir/cmd/query.c): logical echo form, SET DATE /
 * SET CENTURY date rendering, and the bounded LOCATE scopes (RECORD n, NEXT n).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Drives the real dot-prompt
 * REPL (samir_repl) through a CAPTURING + SCRIPTING PAL, exactly like
 * test_samir_repl.c. The seed test_assert.h harness (CHECK/TEST_HARNESS/
 * TEST_SUMMARY); a non-zero exit on any failed check keeps the make gate from
 * false-greening (Law 2).
 *
 * WHY THIS ORACLE IS INDEPENDENT OF THE CODE UNDER TEST (Law 2):
 *   Every expected value here is a HAND-AUTHORED LITERAL transcribed from the
 *   real dBASE III PLUS 1.1 behavior corpus (../dbase3-decomp), NOT re-derived
 *   from q_render_val / q_locate. This is the direct antidote to the Law-2
 *   heresy that test_samir_repl.c previously carried (it graded logical output
 *   T/F against a q_render_val that also emitted T/F -- agreeing by
 *   construction, catching nothing).
 *
 * SESSION L (initech-3p9e -- the logical-echo heresy):
 *   `?` / `??` echo logicals DOTTED (".T."/".F.");  LIST/DISPLAY print logical
 *   COLUMNS bare ("T"/"F"). These are two DISTINCT presentation rules and the
 *   render path must honor both.
 *   Ground truth (Law 1):
 *     ../dbase3-decomp/specs/commands/navigation-query-display.md
 *       L.803: "? ... logicals as .T./.F."   (the ?/?? echo -> DOTTED)
 *       L.689: "logical fields print as T/F"  (LIST/DISPLAY column -> BARE)
 *
 * SESSION D (initech-ue7y -- SET DATE / SET CENTURY):
 *   `?`/LIST of a Date honors ctx->set_date_fmt and ctx->set_century. A fixed
 *   stored Date (2 Mar 1985) is rendered under several SET DATE formats; the
 *   value is format-independent so switching SET DATE cannot be self-fulfilling.
 *   Ground truth (Law 1):
 *     ../dbase3-decomp/specs/runtime/dates-and-century.md "The six III+ formats"
 *       AMERICAN mm/dd/yy 03/02/85 | BRITISH dd/mm/yy 02/03/85 | GERMAN dd.mm.yy
 *       02.03.85 | ANSI yy.mm.dd 85.03.02 ; SET CENTURY ON widens year to 4:
 *       BRITISH+CENTURY 02/03/1985.
 *
 * SESSION R (initech-wcb7 -- LOCATE RECORD n FOR):
 *   RECORD n is a SINGLE-record scope (dates-and-century sibling
 *   navigation-query-display.md L.80 "RECORD <n>  A single record"): LOCATE
 *   RECORD n tests ONLY record n. FOR false -> FOUND() .F., cursor stays at n.
 *
 * SESSION N (initech-qw4e -- LOCATE NEXT n FOR):
 *   NEXT n scans at most n records from the current one
 *   (navigation-query-display.md L.81 "NEXT <n>  <n> records starting at the
 *   CURRENT record"): LOCATE NEXT n stops after n records regardless of a later
 *   match.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/commands/navigation-query-display.md
 *   - ../dbase3-decomp/specs/runtime/dates-and-century.md
 *   - os/samir/samir_main.c (samir_repl), os/samir/cmd/query.c (under test).
 *   - harness/diff/dbf_diff/test_samir_repl.c (capturing+scripting PAL idiom).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "test_assert.h"
#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/dbf.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"

TEST_HARNESS();

/* pal_host.c surface (declared here -- not in a header). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* samir_main.c surface (the S5.8 entry point under test). */
extern int samir_repl(samir_pal_t *pal, xb_interp *ip);

/* =====================================================================
 * Capturing + scripting PAL (same idiom as test_samir_repl.c).
 * ===================================================================== */

#define CAP_BUF          65536
#define SCRIPT_MAX_LINES 128

typedef struct {
    samir_pal_t  pal;        /* MUST be first: &cap.pal is handed to the engine */
    samir_pal_t *inner;
    char         buf[CAP_BUF];
    uint32_t     len;
    const char  *lines[SCRIPT_MAX_LINES];
    int          nlines;
    int          lineidx;
} cap_pal;

static cap_pal g_cap;

static pal_fd  cap_open (samir_pal_t *p, const char *n, int m) { cap_pal *c=(cap_pal*)p; return c->inner->open(c->inner,n,m); }
static int     cap_close(samir_pal_t *p, pal_fd fd)           { cap_pal *c=(cap_pal*)p; return c->inner->close(c->inner,fd); }
static int32_t cap_read (samir_pal_t *p, pal_fd fd, void *b, uint32_t n){ cap_pal *c=(cap_pal*)p; return c->inner->read(c->inner,fd,b,n); }
static int32_t cap_write(samir_pal_t *p, pal_fd fd, const void *b, uint32_t n){ cap_pal *c=(cap_pal*)p; return c->inner->write(c->inner,fd,b,n); }
static int32_t cap_seek (samir_pal_t *p, pal_fd fd, int32_t o, int w){ cap_pal *c=(cap_pal*)p; return c->inner->seek(c->inner,fd,o,w); }
static int     cap_remove(samir_pal_t *p, const char *n)      { cap_pal *c=(cap_pal*)p; return c->inner->remove(c->inner,n); }
static int     cap_rename(samir_pal_t *p, const char *f, const char *t){ cap_pal *c=(cap_pal*)p; return c->inner->rename(c->inner,f,t); }
static void    cap_conout(samir_pal_t *p, const char *s, uint32_t n)
{
    cap_pal *c=(cap_pal*)p;
    uint32_t i;
    for (i = 0; i < n && c->len < (uint32_t)(CAP_BUF - 1); i++)
        c->buf[c->len++] = s[i];
    c->buf[c->len] = '\0';
}
static int32_t cap_conin_line(samir_pal_t *p, char *buf, uint32_t cap)
{
    cap_pal *c=(cap_pal*)p;
    const char *s;
    uint32_t k;
    if (c->lineidx >= c->nlines) { if (cap) buf[0] = '\0'; return -1; }
    s = c->lines[c->lineidx++];
    k = 0;
    while (s[k] != '\0' && k < cap - 1u) { buf[k] = s[k]; k++; }
    buf[k] = '\0';
    return (int32_t)k;
}
static int32_t cap_conin_char(samir_pal_t *p){ cap_pal *c=(cap_pal*)p; return c->inner->conin_char(c->inner); }
static void    cap_gotoxy(samir_pal_t *p, uint8_t r, uint8_t col){ cap_pal *c=(cap_pal*)p; c->inner->gotoxy(c->inner,r,col); }
static void    cap_set_attr(samir_pal_t *p, uint8_t a){ cap_pal *c=(cap_pal*)p; c->inner->set_attr(c->inner,a); }
static void    cap_today(samir_pal_t *p, uint8_t *yy, uint8_t *mm, uint8_t *dd){ cap_pal *c=(cap_pal*)p; c->inner->today(c->inner,yy,mm,dd); }
static void   *cap_alloc(samir_pal_t *p, uint32_t n){ cap_pal *c=(cap_pal*)p; return c->inner->alloc(c->inner,n); }
static void    cap_reset(samir_pal_t *p, void *m){ cap_pal *c=(cap_pal*)p; c->inner->reset(c->inner,m); }

static samir_pal_t *cap_pal_make(samir_pal_t *inner)
{
    g_cap.inner = inner;
    g_cap.len = 0; g_cap.buf[0] = '\0';
    g_cap.nlines = 0; g_cap.lineidx = 0;
    g_cap.pal.open       = cap_open;
    g_cap.pal.close      = cap_close;
    g_cap.pal.read       = cap_read;
    g_cap.pal.write      = cap_write;
    g_cap.pal.seek       = cap_seek;
    g_cap.pal.remove     = cap_remove;
    g_cap.pal.rename     = cap_rename;
    g_cap.pal.conout     = cap_conout;
    g_cap.pal.conin_line = cap_conin_line;
    g_cap.pal.conin_char = cap_conin_char;
    g_cap.pal.gotoxy     = cap_gotoxy;
    g_cap.pal.set_attr   = cap_set_attr;
    g_cap.pal.today      = cap_today;
    g_cap.pal.alloc      = cap_alloc;
    g_cap.pal.reset      = cap_reset;
    return &g_cap.pal;
}

static void cap_clear(void) { g_cap.len = 0; g_cap.buf[0] = '\0'; }
static void script_reset(void) { g_cap.nlines = 0; g_cap.lineidx = 0; }
static void script_push(const char *s) { if (g_cap.nlines < SCRIPT_MAX_LINES) g_cap.lines[g_cap.nlines++] = s; }
static int cap_has(const char *needle) { return strstr(g_cap.buf, needle) != NULL; }

/* =====================================================================
 * Table builders.
 * ===================================================================== */

/* Build CODE C(3) + AMT N(5,0) + OK L(1) with `nrows` rows on disk (flushed,
 * closed). code[i]/amt[i]/ok[i] supply the rows. Returns 0 or -1. */
static int build_cal_table(samir_pal_t *pal, const char *path,
                           const char *const *code, const int *amt,
                           const int *ok, int nrows)
{
    dbf_field_spec fs[3];
    dbf_table *tbl = NULL;
    int rc, i;

    fs[0].name = "CODE"; fs[0].type = 'C'; fs[0].field_len = 3; fs[0].dec = 0;
    fs[1].name = "AMT";  fs[1].type = 'N'; fs[1].field_len = 5; fs[1].dec = 0;
    fs[2].name = "OK";   fs[2].type = 'L'; fs[2].field_len = 1; fs[2].dec = 0;

    rc = dbf_create(pal, path, fs, 3, &tbl);
    if (rc != DBF_OK || !tbl) return -1;
    for (i = 0; i < nrows; i++) {
        xb_val r[3];
        r[0] = xb_c(code[i], (uint16_t)strlen(code[i]));
        r[1] = xb_n((double)amt[i]);
        r[2] = xb_l(ok[i]);
        if (dbf_append_rec(tbl, r, 0) != DBF_OK) { dbf_close(tbl); return -1; }
    }
    if (dbf_flush(tbl) != DBF_OK) { dbf_close(tbl); return -1; }
    dbf_close(tbl);
    return 0;
}

/* Build a one-row table TAG C(1) + DT D(8); the Date is a FIXED stored value
 * (format-independent YYYYMMDD on disk) so switching SET DATE cannot fulfill the
 * render assertion by construction (Law 2). Returns 0 or -1. */
static int build_date_table(samir_pal_t *pal, const char *path,
                            int y, int m, int d)
{
    dbf_field_spec fs[2];
    dbf_table *tbl = NULL;
    xb_val r[2];
    int rc;

    fs[0].name = "TAG"; fs[0].type = 'C'; fs[0].field_len = 1; fs[0].dec = 0;
    fs[1].name = "DT";  fs[1].type = 'D'; fs[1].field_len = 8; fs[1].dec = 0;

    rc = dbf_create(pal, path, fs, 2, &tbl);
    if (rc != DBF_OK || !tbl) return -1;
    r[0] = xb_c("X", 1);
    r[1] = xb_d((double)jdn_from_ymd(y, m, d));
    if (dbf_append_rec(tbl, r, 0) != DBF_OK) { dbf_close(tbl); return -1; }
    if (dbf_flush(tbl) != DBF_OK) { dbf_close(tbl); return -1; }
    dbf_close(tbl);
    return 0;
}

/* =====================================================================
 * SESSION L: the logical-echo distinction (initech-3p9e).
 *   ? / ?? echo logicals DOTTED (.T./.F.);  LIST columns are BARE (T/F).
 * ===================================================================== */
static void test_logical_echo(samir_pal_t *pal)
{
    const char *path = "/tmp/test_samir_query_L.dbf";
    static const char *code[3] = { "AAA", "BBB", "CCC" };
    static const int   amt[3]  = { 100, 200, 300 };
    static const int   ok[3]   = { 1, 0, 1 };            /* T F T */
    xb_interp *ip;
    static char useln[160];

    CHECK(build_cal_table(pal, path, code, amt, ok, 3) == 0,
          "logic: build CODE/AMT/OK table");

    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "logic: xb_interp_make");
    if (!ip) { remove(path); return; }

    script_reset();
    cap_clear();
    snprintf(useln, sizeof useln, "USE %s", path);
    script_push(useln);
    script_push("? OK");     /* rec1 OK=.T. -> ?-echo DOTTED ".T."   */
    script_push("GO 2");
    script_push("? OK");     /* rec2 OK=.F. -> ?-echo DOTTED ".F."   */
    script_push("LIST");     /* every row: OK column BARE T/F        */
    script_push("QUIT");

    CHECK(samir_repl(pal, ip) == INTERP_OK, "logic: samir_repl clean exit");

    /* ?/?? echo form: DOTTED (III+ navigation-query-display.md L.803).
     * RED on the pre-fix code: q_render_val emitted bare "T"/"F" for ? too, so
     * the buffer held "\nT"/"\nF", never "\n.T."/"\n.F." -> these two FAIL. */
    CHECK(cap_has("\n.T."), "logic: ? OK echoes .T. (dotted) at rec1 [corpus L.803]");
    CHECK(cap_has("\n.F."), "logic: ? OK echoes .F. (dotted) at rec2 [corpus L.803]");

    /* LIST column form: BARE (III+ navigation-query-display.md L.689). A DOTTED
     * column would read "100 .T." / "200 .F.", so "100 T"/"200 F" (space then a
     * bare letter) is present iff the column is bare. This leg is the regression
     * guard that the ?-dotting fix does NOT leak into LIST columns. */
    CHECK(cap_has("100 T"), "logic: LIST rec1 OK column is bare T [corpus L.689]");
    CHECK(cap_has("200 F"), "logic: LIST rec2 OK column is bare F [corpus L.689]");

    xb_interp_free(ip);
    remove(path);
}

/* =====================================================================
 * SESSION D: SET DATE / SET CENTURY govern ?/LIST Date rendering (initech-ue7y).
 *   The stored Date is 2 Mar 1985 (JDN, format-independent). Each SET DATE /
 *   SET CENTURY setting must reorder / reseparate / rewiden the SAME value.
 *   Expected literals are HAND-AUTHORED from the corpus "six III+ formats"
 *   table (Law 2 independence): AMERICAN 03/02/85, BRITISH 02/03/85, GERMAN
 *   02.03.85, ANSI 85.03.02, BRITISH+CENTURY 02/03/1985.
 * ===================================================================== */
static void test_date_render(samir_pal_t *pal)
{
    const char *path = "/tmp/test_samir_query_D.dbf";
    xb_interp *ip;
    static char useln[160];

    /* 2 Mar 1985 -- the exact example row in dates-and-century.md's format table. */
    CHECK(build_date_table(pal, path, 1985, 3, 2) == 0,
          "date: build TAG/DT table (DT = 1985-03-02)");

    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "date: xb_interp_make");
    if (!ip) { remove(path); return; }

    script_reset();
    cap_clear();
    snprintf(useln, sizeof useln, "USE %s", path);
    script_push(useln);
    script_push("SET DATE AMERICAN");
    script_push("? DT");               /* 03/02/85  (control: default format)   */
    script_push("SET DATE BRITISH");
    script_push("? DT");               /* 02/03/85  (RED pre-fix: still 03/02/85)*/
    script_push("SET DATE GERMAN");
    script_push("? DT");               /* 02.03.85  (RED pre-fix)                */
    script_push("SET DATE ANSI");
    script_push("? DT");               /* 85.03.02  (RED pre-fix)                */
    script_push("SET DATE BRITISH");
    script_push("SET CENTURY ON");
    script_push("? DT");               /* 02/03/1985 (RED pre-fix: 2-digit year) */
    script_push("QUIT");

    CHECK(samir_repl(pal, ip) == INTERP_OK, "date: samir_repl clean exit");

    /* Hand-authored III+ corpus literals (dates-and-century.md "The six III+
     * formats" table, example 2 Mar 1985); all five are mutually non-substring. */
    CHECK(cap_has("03/02/85"),  "date: AMERICAN mm/dd/yy -> 03/02/85 [corpus]");
    CHECK(cap_has("02/03/85"),  "date: BRITISH  dd/mm/yy -> 02/03/85 [corpus]");
    CHECK(cap_has("02.03.85"),  "date: GERMAN   dd.mm.yy -> 02.03.85 [corpus]");
    CHECK(cap_has("85.03.02"),  "date: ANSI     yy.mm.dd -> 85.03.02 [corpus]");
    CHECK(cap_has("02/03/1985"),"date: BRITISH + CENTURY ON -> 02/03/1985 [corpus]");

    xb_interp_free(ip);
    remove(path);
}

/* =====================================================================
 * SESSION R: LOCATE RECORD n FOR <cond> tests ONLY record n (initech-wcb7).
 *   RECORD n is a SINGLE-record scope. Table of 5 rows with AMT=99 ONLY at
 *   rec 3. LOCATE RECORD 2 FOR AMT=99 must MISS (rec2 AMT!=99): FOUND() .F.,
 *   cursor stays at rec 2. The pre-fix code scanned rec2..EOF and wrongly
 *   landed on rec 3 (FOUND .T.). A positive control (RECORD 3) confirms a real
 *   single-record hit still works.
 *   Ground truth (Law 1): navigation-query-display.md L.80 "RECORD <n>  A
 *   single record" + L.124 evaluation order (RECORD n IS the start record).
 * ===================================================================== */
static void test_locate_record(samir_pal_t *pal)
{
    const char *path = "/tmp/test_samir_query_R.dbf";
    static const char *code[5] = { "R1", "R2", "R3", "R4", "R5" };
    static const int   amt[5]  = { 10, 20, 99, 40, 50 };   /* AMT=99 ONLY at rec3 */
    static const int   ok[5]   = { 1, 1, 1, 1, 1 };
    xb_interp *ip;
    static char useln[160];

    CHECK(build_cal_table(pal, path, code, amt, ok, 5) == 0,
          "locR: build 5-row table (AMT=99 only at rec3)");

    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "locR: xb_interp_make");
    if (!ip) { remove(path); return; }

    script_reset();
    cap_clear();
    snprintf(useln, sizeof useln, "USE %s", path);
    script_push(useln);
    /* RECORD 2 scope: test ONLY rec2 (AMT=20). MISS. */
    script_push("LOCATE RECORD 2 FOR AMT=99");
    script_push("? FOUND()");          /* .F. (RED pre-fix: wrongly .T.)        */
    script_push("? RECNO()");          /* 2   (RED pre-fix: wrongly 3)          */
    /* Positive control: RECORD 3 scope hits (AMT=99 at rec3). */
    script_push("LOCATE RECORD 3 FOR AMT=99");
    script_push("? FOUND()");          /* .T. */
    script_push("? RECNO()");          /* 3   */
    script_push("QUIT");

    CHECK(samir_repl(pal, ip) == INTERP_OK, "locR: samir_repl clean exit");

    /* RED signals (pre-fix these are absent because RECORD 2 wrongly matched
     * rec3 -> FOUND .T., RECNO 3). Hand-reasoned from the single-record scope
     * rule; independent of q_locate's internals. */
    CHECK(cap_has("\n.F."), "locR: LOCATE RECORD 2 FOR AMT=99 -> FOUND() .F. [single-rec scope]");
    CHECK(cap_has("\n2"),   "locR: cursor STAYS at rec 2 on the miss (RECNO 2)");
    /* Positive control (guard both pre/post): RECORD 3 is a genuine hit. */
    CHECK(cap_has("\n.T."), "locR: LOCATE RECORD 3 FOR AMT=99 -> FOUND() .T.");
    CHECK(cap_has("\n3"),   "locR: the RECORD 3 hit lands on rec 3 (RECNO 3)");

    xb_interp_free(ip);
    remove(path);
}

/* =====================================================================
 * SESSION N: LOCATE NEXT n FOR <cond> scans AT MOST n records (initech-qw4e).
 *   100-row table with the FOR predicate true ONLY at rec 50. From rec 1,
 *   LOCATE NEXT 3 must scan only rec 1..3 and MISS (FOUND() .F.) -- it must NOT
 *   run on to rec 50. The pre-fix code passed no limit into q_locate_scan and
 *   looped to EOF, wrongly landing on rec 50 (FOUND .T.). A positive control
 *   (NEXT 60, whose window covers rec 50) confirms a within-window match still
 *   works, so the fix does not over-restrict.
 *   Ground truth (Law 1): navigation-query-display.md L.81 "NEXT <n>  <n>
 *   records starting at the CURRENT record".
 * ===================================================================== */
static void test_locate_next(samir_pal_t *pal)
{
    const char *path = "/tmp/test_samir_query_N.dbf";
    const char *codes[100];
    int amts[100], oks[100];
    int i;
    xb_interp *ip;
    static char useln[160];

    for (i = 0; i < 100; i++) {
        codes[i] = "RR";
        amts[i]  = (i == 49) ? 777 : 0;   /* AMT=777 ONLY at rec 50 (index 49) */
        oks[i]   = 1;
    }
    CHECK(build_cal_table(pal, path, codes, amts, oks, 100) == 0,
          "locN: build 100-row table (AMT=777 only at rec 50)");

    /* ---- Sub-session A: the bug. NEXT 3 from rec 1 must MISS. ---- */
    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "locN: xb_interp_make (A)");
    if (!ip) { remove(path); return; }
    script_reset();
    cap_clear();
    snprintf(useln, sizeof useln, "USE %s", path);
    script_push(useln);
    script_push("GO TOP");
    script_push("LOCATE NEXT 3 FOR AMT=777");  /* window rec1..3 -> no match     */
    script_push("? FOUND()");                  /* .F. (RED pre-fix: ran to rec50)*/
    script_push("QUIT");
    CHECK(samir_repl(pal, ip) == INTERP_OK, "locN: samir_repl clean exit (A)");
    /* The definitive signal: a NEXT 3 that stops at 3 cannot have matched rec50,
     * so FOUND() is .F. Pre-fix the unbounded scan reached rec50 -> FOUND .T.,
     * so ".F." is absent -> RED. */
    CHECK(cap_has("\n.F."), "locN: LOCATE NEXT 3 FOR AMT=777 -> FOUND() .F. (stopped at 3)");
    xb_interp_free(ip);

    /* ---- Sub-session B: control. NEXT 60 window covers rec 50 -> HIT. ---- */
    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "locN: xb_interp_make (B)");
    if (!ip) { remove(path); return; }
    script_reset();
    cap_clear();
    script_push(useln);
    script_push("GO TOP");
    script_push("LOCATE NEXT 60 FOR AMT=777"); /* window rec1..60 includes rec50 */
    script_push("? FOUND()");                  /* .T. */
    script_push("? RECNO()");                  /* 50  */
    script_push("QUIT");
    CHECK(samir_repl(pal, ip) == INTERP_OK, "locN: samir_repl clean exit (B)");
    CHECK(cap_has("\n.T."), "locN: LOCATE NEXT 60 FOR AMT=777 -> FOUND() .T. (rec50 in window)");
    CHECK(cap_has("\n50"),  "locN: the NEXT 60 hit lands on rec 50 (RECNO 50)");
    xb_interp_free(ip);

    remove(path);
}

/* ===================================================================== */
/* main                                                                   */
/* ===================================================================== */
int main(int argc, char **argv)
{
    struct pal_host_cfg cfg;
    samir_pal_t *host, *pal;
    (void)argc; (void)argv;   /* self-contained: hand-authored corpus literals */

    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy   = 99;
    cfg.date_mm   = 12;
    cfg.date_dd   = 31;
    cfg.heap_size = 16u * 1024u * 1024u;

    host = pal_host_make(cfg);
    if (!host) { fprintf(stderr, "FATAL: pal_host_make returned NULL\n"); return 2; }
    pal = cap_pal_make(host);

    test_logical_echo(pal);
    test_date_render(pal);
    test_locate_record(pal);
    test_locate_next(pal);

    pal_host_free(host);
    return TEST_SUMMARY("test-samir-query");
}
