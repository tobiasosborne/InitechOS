/*
 * harness/diff/dbf_diff/test_samir_repl.c -- host oracle for S5.8: the dot-prompt
 * REPL (os/samir/samir_main.c, samir_repl). initech-7az.9.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_interp_list.c
 * (the CAPTURING PAL that intercepts conout) + test_interp_proc.c (the SCRIPTED
 * conin_line that feeds a queue of typed lines). The seed test_assert.h harness
 * (CHECK / TEST_HARNESS / TEST_SUMMARY); a non-zero exit on any failed check
 * keeps `make test-samir-repl` from false-greening (Law 2).
 *
 * WHAT THIS DRIVES (plan S5.8 oracle: "a scripted session over the host PAL"):
 *   A real dot-prompt session is fed line by line to samir_repl through a PAL
 *   whose conin_line pops the next scripted line and whose conout is captured
 *   into a byte buffer. We assert the captured transcript at each step.
 *
 * SESSION 1 (the convergence session -- USE..LOCATE..REPLACE..STORE..DO..QUIT):
 *   The REPL registers query/mutate/set/proc itself, so the only setup the test
 *   does is pre-adopt a WRITABLE table (dbf_create + wa_adopt_table) into area 1
 *   so the session's REPLACE has a writable target -- wa_set_open (the engine's
 *   USE-from-disk path) opens READ-ONLY and there is no dbf_open_rw yet, so a
 *   REPLACE after a plain USE would fail loud #41 (a real, current engine gap,
 *   flagged as a follow-up). Area 1 is the writable adopted table; the session
 *   also USEs a second, read-only /tmp table into area 2 to exercise the REPL's
 *   own USE-verb parser + a read path (LIST / LOCATE / FOUND).
 *
 *     USE <ro /tmp copy> in area 2  (the REPL's USE parser + wa_set_open)
 *     ? RECNO()                     -> "1"   (fresh USE positions at record 1)
 *     LIST                          -> all rows of the ro table
 *     LOCATE FOR AMT = 200          -> lands on the matching record
 *     ? FOUND()                     -> ".T." (LOCATE matched)
 *     SELECT 1                      (the writable adopted area)
 *     REPLACE CODE WITH 'ZZ'        (mutate module; needs the writable table)
 *     ? CODE                        -> "ZZ " (the NEW value, padded to width 3)
 *     STORE 7 TO COUNTER ; ? COUNTER-> "7"   (memvar STORE + ?)
 *     QUIT                          -> clean stop
 *
 * SESSION 2 (SET EXACT effect): a fresh REPL.
 *     ? 'AB' = 'A'    EXACT OFF (default) -> .T. (begins-with)
 *     SET EXACT ON
 *     ? 'AB' = 'A'    EXACT ON             -> .F. (must match fully)
 *
 * SESSION 3 (PROCEDURE via DO): a fresh REPL.
 *     PUBLIC R ; DO BUMP ; ? R   with  PROCEDURE BUMP / R=41 / RETURN  -> "41".
 *     (The whole program is typed as multi-line text; proc_run is invoked via the
 *      DO module which the REPL registered. We feed it as ONE line that contains
 *      the program, since samir_do runs a multi-line block.)
 *
 * SESSION 4 (error rendering + the loop CONTINUES): a fresh REPL.
 *     FROBNICATE          (an unknown verb) -> "16  *** Unrecognized command verb."
 *     ? 'A' + 1           (a type mismatch) -> "9  Data type mismatch."
 *     ? 'STILL HERE'      (the session did NOT abort) -> "STILL HERE" still prints
 *     QUIT
 *   Asserts the catalog message text for each code AND that a later line still
 *   runs (the REPL did not abort on the bad lines).
 *
 * SESSION 5 (EOF terminates cleanly): a fresh REPL with an EMPTY script -> EOF on
 *   the first conin_line -> samir_repl returns INTERP_OK with no work done.
 *
 * Mutation proof (Rule 6 / ARB rider (a)) -- the Make gate uses
 *   -DREPL_MUTATE_NO_MUTATE_MODULE: samir_main.c then does NOT register the mutate
 *   module, so the session's "REPLACE CODE WITH 'ZZ'" is unrecognized (#16) and
 *   "? CODE" still shows the OLD value -> the "REPLACE wrote ZZ" assertion RED.
 *   The alternative -DREPL_MUTATE_ERR_RENDER renders code+1, so SESSION 4's
 *   "9  Data type mismatch." text assertion goes RED.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.8 contract + oracle; Sec 8.2.
 *   - os/samir/samir_main.c (samir_repl; the catalog renderer; USE/CLOSE).
 *   - os/samir/include/samir/interp.h (samir_repl / samir_do / the hook chain).
 *   - os/samir/include/samir/workarea.h (wa_adopt_table / wa_set_open; the env).
 *   - spec/samir/dbase_msg_codes.tsv (#9, #16 -- the asserted catalog text).
 *   - harness/diff/dbf_diff/test_interp_list.c (capturing PAL pattern).
 *   - harness/diff/dbf_diff/test_interp_proc.c (scripted conin pattern).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "test_assert.h"          /* seed/, on -Iseed */
#include "samir/interp.h"         /* os/samir/include/ */
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
 * Capturing + scripting PAL: delegate every slot to the host PAL EXCEPT
 * conout (captured into a buffer) and conin_line (pops the next scripted
 * line). Mirrors test_interp_list.c (capture) + test_interp_proc.c (script).
 * ===================================================================== */

#define CAP_BUF          32768
#define SCRIPT_MAX_LINES 64

typedef struct {
    samir_pal_t  pal;        /* MUST be first: &cap.pal is handed to the engine */
    samir_pal_t *inner;      /* the real host PAL */
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
/* the overriding conin_line: copy the next scripted line into buf (no newline). */
static int32_t cap_conin_line(samir_pal_t *p, char *buf, uint32_t cap)
{
    cap_pal *c=(cap_pal*)p;
    const char *s;
    uint32_t k;
    if (c->lineidx >= c->nlines) { if (cap) buf[0] = '\0'; return -1; }  /* EOF */
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
static const char *cap_text(void) { return g_cap.buf; }
static int cap_has(const char *needle) { return strstr(g_cap.buf, needle) != NULL; }

/* =====================================================================
 * Helpers.
 * ===================================================================== */

/* Build a writable table CODE C(3) + AMT N(5,0) + OK L(1) with seed rows. */
typedef struct { const char *code; double amt; int ok; } qrow;

static dbf_table *make_writable_table(samir_pal_t *pal, const char *path,
                                      const qrow *rows, int nrows)
{
    dbf_field_spec fs[3];
    dbf_table *tbl = NULL;
    int rc, i;

    fs[0].name = "CODE"; fs[0].type = 'C'; fs[0].field_len = 3; fs[0].dec = 0;
    fs[1].name = "AMT";  fs[1].type = 'N'; fs[1].field_len = 5; fs[1].dec = 0;
    fs[2].name = "OK";   fs[2].type = 'L'; fs[2].field_len = 1; fs[2].dec = 0;

    rc = dbf_create(pal, path, fs, 3, &tbl);
    if (rc != DBF_OK) return NULL;
    for (i = 0; i < nrows; i++) {
        xb_val r[3];
        r[0] = xb_c(rows[i].code, (uint16_t)strlen(rows[i].code));
        r[1] = xb_n(rows[i].amt);
        r[2] = xb_l(rows[i].ok);
        if (dbf_append_rec(tbl, r, 0) != DBF_OK) { dbf_close(tbl); return NULL; }
    }
    if (dbf_flush(tbl) != DBF_OK) { dbf_close(tbl); return NULL; }
    return tbl;
}

/*
 * Write a read-only on-disk table (raw .dbf bytes) for the REPL's USE parser to
 * open via wa_set_open. CODE C(3) + AMT N(5,0) + OK L(1), 3 rows.
 *   recno CODE AMT OK
 *     1   AAA  100  T
 *     2   BBB  200  F
 *     3   CCC  300  T
 */
static int write_ro_dbf(const char *path)
{
    FILE *f;
    uint8_t hdr[32], d0[32], d1[32], d2[32], term[1];
    int i;
    static const char *codes[3] = { "AAA","BBB","CCC" };
    static const int   amts[3]  = { 100,  200,  300  };
    static const char  oks[3]   = { 'T',  'F',  'T'  };

    memset(hdr,0,sizeof hdr); memset(d0,0,sizeof d0);
    memset(d1,0,sizeof d1);   memset(d2,0,sizeof d2);
    hdr[0x00]=0x03; hdr[0x04]=3;            /* version 0x03, nrec=3 */
    hdr[0x08]=129; hdr[0x09]=0;             /* header_length=129 */
    hdr[0x0A]=10;  hdr[0x0B]=0;             /* record_length=10 */
    d0[0]='C';d0[1]='O';d0[2]='D';d0[3]='E';d0[0x0B]='C';d0[0x10]=3;
    d1[0]='A';d1[1]='M';d1[2]='T';d1[0x0B]='N';d1[0x10]=5;
    d2[0]='O';d2[1]='K';d2[0x0B]='L';d2[0x10]=1;
    term[0]=0x0D;

    f = fopen(path,"wb");
    if (!f) return -1;
    fwrite(hdr,1,32,f); fwrite(d0,1,32,f); fwrite(d1,1,32,f); fwrite(d2,1,32,f);
    fwrite(term,1,1,f);
    for (i=0;i<3;i++) {
        char rec[10], nbuf[6];
        rec[0]=0x20;
        rec[1]=codes[i][0]; rec[2]=codes[i][1]; rec[3]=codes[i][2];
        snprintf(nbuf,sizeof nbuf,"%5d",amts[i]);
        rec[4]=nbuf[0]; rec[5]=nbuf[1]; rec[6]=nbuf[2]; rec[7]=nbuf[3]; rec[8]=nbuf[4];
        rec[9]=oks[i];
        fwrite(rec,1,10,f);
    }
    fclose(f);
    return 0;
}

/* read a C field's bytes from a writable table (post-REPLACE check). */
static void read_c_field(dbf_table *tbl, uint32_t recno, int fld, char *out, int cap)
{
    xb_val rec[8];
    int del = 0;
    out[0] = '\0';
    if (dbf_read_rec(tbl, recno, rec, &del) != DBF_OK) return;
    if (rec[fld].t == XB_C) {
        int n = rec[fld].u.c.len; if (n > cap-1) n = cap-1;
        memcpy(out, rec[fld].u.c.p, (size_t)n); out[n] = '\0';
    }
}

/* =====================================================================
 * SESSION 1: the convergence session (USE/?/LIST/LOCATE/REPLACE/STORE/QUIT).
 * ===================================================================== */
static void test_session_convergence(samir_pal_t *pal)
{
    const char *wpath = "/tmp/test_samir_repl_w.dbf";
    const char *rpath = "/tmp/test_samir_repl_ro.dbf";
    static const qrow seed[3] = { {"AAA",100,1}, {"BBB",200,0}, {"CCC",100,1} };
    dbf_table *wtbl;
    xb_interp *ip;
    wa_env *env;
    char cbuf[16];
    int rc;
    char msg[256];

    CHECK(write_ro_dbf(rpath) == 0, "conv: write read-only table");
    wtbl = make_writable_table(pal, wpath, seed, 3);
    CHECK(wtbl != NULL, "conv: build writable table");
    if (!wtbl) { remove(rpath); return; }

    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "conv: xb_interp_make");
    if (!ip) { dbf_close(wtbl); remove(rpath); remove(wpath); return; }
    env = xb_interp_env(ip);

    /* pre-adopt the writable table into area 1 (REPLACE needs writable;
     * the REPL registers the modules itself, so we do NOT register here). */
    rc = wa_adopt_table(env, 1, wtbl, NULL, "W", wpath, NULL, 0);
    snprintf(msg,sizeof msg,"conv: wa_adopt_table rc=%d",rc);
    CHECK(rc == WA_OK, msg);
    if (rc != WA_OK) { xb_interp_free(ip); remove(rpath); remove(wpath); return; }

    /* the scripted dot-prompt session. */
    script_reset();
    cap_clear();
    {
        static char useln[128];
        snprintf(useln, sizeof useln, "USE %s ALIAS RO", rpath);
        script_push("SELECT 2");        /* spine SELECT */
        script_push(useln);             /* REPL's USE parser -> wa_set_open (ro) */
        script_push("? RECNO()");       /* -> 1 */
        script_push("LIST");            /* all ro rows */
        script_push("LOCATE FOR AMT = 200"); /* -> recno 2 */
        script_push("? FOUND()");       /* -> .T. */
        script_push("SELECT 1");        /* the writable area */
        script_push("REPLACE CODE WITH 'ZZ'"); /* mutate module */
        script_push("? CODE");          /* -> ZZ (the new value) */
        script_push("STORE 7 TO COUNTER");
        script_push("? COUNTER");       /* -> 7 */
        script_push("QUIT");
    }

    rc = samir_repl(pal, ip);
    snprintf(msg,sizeof msg,"conv: samir_repl rc=%d (want 0)",rc);
    CHECK(rc == INTERP_OK, msg);

    /* the dot prompt was emitted each line (at least once). */
    CHECK(cap_has(". "), "conv: dot prompt emitted");
    /* ? RECNO() on the freshly-USEd ro table = 1 (rendered after a leading NL). */
    CHECK(cap_has("\n1"), "conv: ? RECNO() shows 1 (fresh USE positions at rec 1)");
    /* LIST showed the ro rows with the recno column. */
    CHECK(cap_has("       1 AAA"), "conv: LIST shows recno 1 / AAA (ro table)");
    CHECK(cap_has("       2 BBB"), "conv: LIST shows recno 2 / BBB");
    CHECK(cap_has("       3 CCC"), "conv: LIST shows recno 3 / CCC");
    /* LOCATE FOR AMT=200 matched -> ? FOUND() renders the logical "T" (the engine
     * prints LIST/? logicals as T/F, not .T./.F.). A match here proves the
     * REPL's USE + LOCATE + FOUND convergence on the ro table. (We assert on the
     * transcript, NOT post-session wa state, because QUIT closes every area.) */
    CHECK(cap_has("\nT"), "conv: ? FOUND() shows T after LOCATE matched AMT=200");
    /* REPLACE wrote 'ZZ' into the writable area-1 record 1 -> ? CODE shows it.
     * This transcript check is the mutation-proof bite point: with
     * -DREPL_MUTATE_NO_MUTATE_MODULE the REPLACE verb is unrecognized (#16) and
     * ? CODE then shows the OLD 'AAA' (no "\nZZ" in the transcript) -> RED. */
    CHECK(cap_has("\nZZ"), "conv: ? CODE shows the REPLACEd value 'ZZ'");
    /* STORE 7 TO COUNTER ; ? COUNTER -> 7 (memvar survived the session). */
    {
        xb_val cv;
        CHECK(xb_interp_get_memvar(ip, "COUNTER", &cv) == 0 && cv.t == XB_N &&
              cv.u.n == 7.0, "conv: STORE 7 TO COUNTER -> memvar==7");
    }
    CHECK(cap_has("\n7"), "conv: ? COUNTER shows 7");

    /* the interp (and its adopted/closed tables) are gone after samir_repl's
     * QUIT-time wa_close_all; free the handle and verify the REPLACE PERSISTED by
     * re-opening the file fresh from disk (mutate flushes after each verb). */
    xb_interp_free(ip);
    {
        dbf_table *verify = NULL;
        if (dbf_open(pal, wpath, &verify) == DBF_OK) {
            read_c_field(verify, 1, 0, cbuf, sizeof cbuf);
            snprintf(msg,sizeof msg,"conv: rec1 CODE persisted == 'ZZ ' (got '%s')",cbuf);
            CHECK(strcmp(cbuf, "ZZ ") == 0, msg);  /* MUTANT NO_MUTATE_MODULE -> 'AAA' */
            dbf_close(verify);
        } else {
            CHECK(0, "conv: re-open writable table to verify REPLACE persisted");
        }
    }
    remove(rpath);
    remove(wpath);
}

/* =====================================================================
 * SESSION 2: SET EXACT effect.
 * ===================================================================== */
static void test_session_set_exact(samir_pal_t *pal)
{
    xb_interp *ip = xb_interp_make(pal);
    int rc;
    char msg[160];
    CHECK(ip != NULL, "exact: xb_interp_make");
    if (!ip) return;

    script_reset();
    cap_clear();
    script_push("? 'AB' = 'A'");   /* EXACT OFF default: begins-with -> .T. */
    script_push("SET EXACT ON");
    script_push("? 'AB' = 'A'");   /* EXACT ON: full match required -> .F. */
    script_push("QUIT");

    rc = samir_repl(pal, ip);
    snprintf(msg,sizeof msg,"exact: samir_repl rc=%d",rc);
    CHECK(rc == INTERP_OK, msg);
    /* Logicals render as T/F (engine LIST/? form). EXACT OFF gives 'AB'='A' true,
     * EXACT ON gives false. Both a "T" and an "F" value (each after the ?'s
     * leading NL) must appear, proving SET EXACT changed the comparison. */
    CHECK(cap_has("\nT"), "exact: EXACT OFF -> 'AB'='A' is T (begins-with)");
    CHECK(cap_has("\nF"), "exact: SET EXACT ON -> 'AB'='A' is F (full match)");

    xb_interp_free(ip);
}

/* =====================================================================
 * SESSION 3: a PROCEDURE invoked via DO (the proc module).
 * ===================================================================== */
static void test_session_procedure(samir_pal_t *pal)
{
    xb_interp *ip = xb_interp_make(pal);
    int rc;
    char msg[160];
    CHECK(ip != NULL, "proc: xb_interp_make");
    if (!ip) return;

    script_reset();
    cap_clear();
    /* The DO module's proc_run runs the main body then PROCEDURE blocks; the
     * whole little program is fed as one multi-line line (samir_do splits it). */
    script_push(
        "PUBLIC R\n"
        "DO BUMP\n"
        "? R\n"
        "PROCEDURE BUMP\n"
        "  R = 41\n"
        "  RETURN\n");
    script_push("QUIT");

    rc = samir_repl(pal, ip);
    snprintf(msg,sizeof msg,"proc: samir_repl rc=%d",rc);
    CHECK(rc == INTERP_OK, msg);
    CHECK(cap_has("41"), "proc: DO BUMP set R=41 -> ? R shows 41");
    {
        xb_val rv;
        CHECK(xb_interp_get_memvar(ip, "R", &rv) == 0 && rv.t == XB_N &&
              rv.u.n == 41.0, "proc: PUBLIC R == 41 after DO");
    }

    xb_interp_free(ip);
}

/* =====================================================================
 * SESSION 4: error rendering via the catalog + the loop CONTINUES.
 * ===================================================================== */
static void test_session_errors(samir_pal_t *pal)
{
    xb_interp *ip = xb_interp_make(pal);
    int rc;
    char msg[160];
    CHECK(ip != NULL, "err: xb_interp_make");
    if (!ip) return;

    script_reset();
    cap_clear();
    script_push("FROBNICATE");        /* unknown verb -> #16 */
    script_push("? 'A' + 1");         /* type mismatch -> #9 */
    script_push("? 'STILL HERE'");    /* the session did NOT abort */
    script_push("QUIT");

    rc = samir_repl(pal, ip);
    snprintf(msg,sizeof msg,"err: samir_repl rc=%d (clean exit even after errors)",rc);
    CHECK(rc == INTERP_OK, msg);

    /* the catalog message text for #16 and #9 (period-authentic). */
    CHECK(cap_has("*** Unrecognized command verb."),
          "err: #16 catalog text rendered for FROBNICATE");  /* MUTANT ERR_RENDER -> RED */
    CHECK(cap_has("16  *** Unrecognized command verb."),
          "err: #16 rendered as 'N  <message>' form");
    CHECK(cap_has("Data type mismatch."),
          "err: #9 catalog text rendered for 'A' + 1");      /* MUTANT ERR_RENDER -> RED */
    CHECK(cap_has("9  Data type mismatch."),
          "err: #9 rendered as 'N  <message>' form");
    /* the loop CONTINUED past both bad lines -> the last good line ran. */
    CHECK(cap_has("STILL HERE"),
          "err: session continued after errors (later ? still printed)");

    xb_interp_free(ip);
}

/* =====================================================================
 * SESSION 5: EOF terminates cleanly (empty script -> EOF first read).
 * ===================================================================== */
static void test_session_eof(samir_pal_t *pal)
{
    xb_interp *ip = xb_interp_make(pal);
    int rc;
    char msg[160];
    CHECK(ip != NULL, "eof: xb_interp_make");
    if (!ip) return;

    script_reset();   /* no lines pushed -> conin_line returns -1 immediately */
    cap_clear();
    rc = samir_repl(pal, ip);
    snprintf(msg,sizeof msg,"eof: samir_repl rc=%d (EOF -> clean stop)",rc);
    CHECK(rc == INTERP_OK, msg);
    /* the prompt was still emitted (the REPL prompts before reading). */
    CHECK(cap_has(". "), "eof: prompt emitted before the EOF read");
    /* nothing executed -> no value/error lines beyond the prompt. */
    CHECK(strcmp(cap_text(), ". ") == 0,
          "eof: only the prompt was written, then clean EOF stop");

    xb_interp_free(ip);
}

/* ===================================================================== */
/* main                                                                   */
/* ===================================================================== */
int main(int argc, char **argv)
{
    struct pal_host_cfg cfg;
    samir_pal_t *host, *pal;
    (void)argc; (void)argv;   /* corpus path optional; this oracle is self-contained */

    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy   = 99;
    cfg.date_mm   = 12;
    cfg.date_dd   = 31;
    cfg.heap_size = 8u * 1024u * 1024u;   /* generous: several fresh interps */

    host = pal_host_make(cfg);
    if (!host) { fprintf(stderr, "FATAL: pal_host_make returned NULL\n"); return 2; }
    pal = cap_pal_make(host);             /* capture conout + script conin_line */

    test_session_convergence(pal);
    test_session_set_exact(pal);
    test_session_procedure(pal);
    test_session_errors(pal);
    test_session_eof(pal);

    pal_host_free(host);
    return TEST_SUMMARY("test-samir-repl");
}
