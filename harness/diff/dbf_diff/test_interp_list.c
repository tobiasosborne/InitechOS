/*
 * harness/diff/dbf_diff/test_interp_list.c -- host oracle for S5.4: the query /
 * display module (LIST/DISPLAY with scope/FIELDS/FOR/WHILE/OFF, ?/??, LOCATE/
 * CONTINUE, SEEK/FIND).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_interp_flow.c /
 * test_interp_nav.c: the seed test_assert.h harness (CHECK / TEST_HARNESS /
 * TEST_SUMMARY), a host PAL via pal_host_make. A non-zero exit on any failed
 * check keeps `make test-interp-list` from false-greening (Law 2).
 *
 * Output capture: a CAPTURING PAL wrapper (cap_pal) sits in front of pal_host.
 * It delegates every slot to the real host PAL EXCEPT conout, which it appends
 * into a byte buffer. After running a LIST/DISPLAY/? program we assert against
 * the captured transcript. This is the "minted golden vs transcript" oracle of
 * plan S5.4 done host-side -- the values asserted are byte-grounded against
 * dbf_ref.py (TOURS/CLIENTS records).
 *
 * TIERS (plan Sec 2.A):
 *   Tier 0 (committed, operator-free): a SYNTHETIC C+N table in /tmp drives
 *     LIST (ALL/scope/FOR/WHILE/OFF), DISPLAY (current record), ?/??, and the
 *     LOCATE/CONTINUE loop -- the -DQUERY_MUTATE_CONTINUE_SCOPE mutant bites here
 *     (CONTINUE must find the NEXT match, not re-find the first).
 *   Tier 1 (golden-diff): TOURS + TOURDATE (date index) and CLIENTS + NAMES
 *     (char index) exercise SEEK/FIND on a real master index. Absent goldens ->
 *     LOUD skip; Tier 0 still runs.
 *
 * Mutation proof (Rule 6 / ARB rider (a)):
 *   -DQUERY_MUTATE_CONTINUE_SCOPE: CONTINUE wrongly restarts from GO TOP (re-
 *   applying a scope/WHILE-like bound) instead of resuming the FOR-only scan
 *   from the record after the current one. The LOCATE/CONTINUE loop then re-finds
 *   the SAME first match each time -> the "CONTINUE finds the next recno" checks
 *   go RED. (Exactly the edge the plan flags: "CONTINUE re-applies FOR not
 *   WHILE/scope".)
 *
 * GATED (loud-skip; plan Sec 7 / GAPS secP):
 *   - SET FILTER / SET DELETED interaction with scope (those SET verbs are S5.6;
 *     not yet implemented -- the walk sees every physical record).
 *   - The exact dot-prompt column layout / inter-? separator widths
 *     ([oracle-resolves], navigation-query-display.md Open questions 6/7).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.4 contract + oracle.
 *   - ../dbase3-decomp/specs/commands/navigation-query-display.md (LIST/DISPLAY,
 *     ?/??, LOCATE/CONTINUE, SEEK/FIND; scope/FOR/WHILE; FOUND/EOF effects).
 *   - os/samir/include/samir/interp.h (samir_do + the command-hook chain).
 *   - os/samir/include/samir/workarea.h (wa_recno/wa_eof/wa_found; wa_set_open).
 *   - harness/diff/dbf_diff/dbf_ref.py (TOURS/CLIENTS records -- the value source).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "test_assert.h"          /* seed/, on -Iseed */
#include "samir/interp.h"         /* os/samir/include/, on -Ios/samir/include */
#include "samir/workarea.h"
#include "samir/nav.h"
#include "samir/dbf.h"
#include "samir/ndx.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"

TEST_HARNESS();

/* pal_host.c surface (not declared in a header; declare what we use). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* query.c module registration (the S5.4 entry point). */
int query_register(xb_interp *ip);

/* =====================================================================
 * Capturing PAL: delegate everything to the host PAL, intercept conout.
 * ===================================================================== */

#define CAP_BUF 16384

typedef struct {
    samir_pal_t  pal;        /* MUST be first: we hand &cap.pal to the engine */
    samir_pal_t *inner;      /* the real host PAL */
    char         buf[CAP_BUF];
    uint32_t     len;
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
static int32_t cap_conin_line(samir_pal_t *p, char *b, uint32_t cap){ cap_pal *c=(cap_pal*)p; return c->inner->conin_line(c->inner,b,cap); }
static int32_t cap_conin_char(samir_pal_t *p){ cap_pal *c=(cap_pal*)p; return c->inner->conin_char(c->inner); }
static void    cap_gotoxy(samir_pal_t *p, uint8_t r, uint8_t col){ cap_pal *c=(cap_pal*)p; c->inner->gotoxy(c->inner,r,col); }
static void    cap_set_attr(samir_pal_t *p, uint8_t a){ cap_pal *c=(cap_pal*)p; c->inner->set_attr(c->inner,a); }
static void    cap_today(samir_pal_t *p, uint8_t *yy, uint8_t *mm, uint8_t *dd){ cap_pal *c=(cap_pal*)p; c->inner->today(c->inner,yy,mm,dd); }
static void   *cap_alloc(samir_pal_t *p, uint32_t n){ cap_pal *c=(cap_pal*)p; return c->inner->alloc(c->inner,n); }
static void    cap_reset(samir_pal_t *p, void *m){ cap_pal *c=(cap_pal*)p; c->inner->reset(c->inner,m); }

static samir_pal_t *cap_pal_make(samir_pal_t *inner)
{
    g_cap.inner = inner;
    g_cap.len   = 0;
    g_cap.buf[0] = '\0';
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
static const char *cap_text(void) { return g_cap.buf; }

/* substring present? */
static int cap_has(const char *needle) { return strstr(g_cap.buf, needle) != NULL; }
/* count occurrences of needle. */
static int cap_count(const char *needle)
{
    int n = 0;
    const char *p = g_cap.buf;
    size_t l = strlen(needle);
    if (l == 0) return 0;
    while ((p = strstr(p, needle)) != NULL) { n++; p += l; }
    return n;
}

/* =====================================================================
 * Helpers
 * ===================================================================== */

static int file_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}
static char *join(char *buf, size_t cap, const char *base, const char *rel)
{
    snprintf(buf, cap, "%s/%s", base, rel);
    return buf;
}
#define SP_PATH "goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities"

/* =====================================================================
 * Tier 0 synthetic table: 6 records, fields CODE C(3) + AMT N(5,0) + OK L(1).
 *
 *   recno  CODE  AMT   OK
 *     1    AAA   100   T
 *     2    BBB   200   F
 *     3    CCC   100   T
 *     4    DDD   300   F
 *     5    EEE   100   T
 *     6    FFF   400   T
 *
 * AMT==100 matches recnos 1,3,5 (the LOCATE/CONTINUE target set).
 * ===================================================================== */

static int write_q_dbf(const char *path)
{
    FILE *f;
    uint8_t hdr[32], d0[32], d1[32], d2[32], term[1];
    int i;
    /* reclen = 1 (flag) + 3 (CODE) + 5 (AMT) + 1 (OK) = 10. hdrlen = 32 + 3*32 + 1 = 129. */
    static const char *codes[6] = { "AAA","BBB","CCC","DDD","EEE","FFF" };
    static const int   amts[6]  = { 100,  200,  100,  300,  100,  400  };
    static const char  oks[6]   = { 'T',  'F',  'T',  'F',  'T',  'T'  };

    memset(hdr,0,sizeof hdr); memset(d0,0,sizeof d0);
    memset(d1,0,sizeof d1);   memset(d2,0,sizeof d2);

    hdr[0x00]=0x03;
    hdr[0x04]=6;                              /* nrec=6 */
    hdr[0x08]=129; hdr[0x09]=0;              /* header_length=129 */
    hdr[0x0A]=10;  hdr[0x0B]=0;              /* record_length=10 */

    d0[0]='C'; d0[1]='O'; d0[2]='D'; d0[3]='E'; d0[4]=0;
    d0[0x0B]='C'; d0[0x10]=3; d0[0x11]=0;

    d1[0]='A'; d1[1]='M'; d1[2]='T'; d1[3]=0;
    d1[0x0B]='N'; d1[0x10]=5; d1[0x11]=0;

    d2[0]='O'; d2[1]='K'; d2[2]=0;
    d2[0x0B]='L'; d2[0x10]=1; d2[0x11]=0;

    term[0]=0x0D;

    f = fopen(path,"wb");
    if (!f) return -1;
    fwrite(hdr,1,32,f); fwrite(d0,1,32,f); fwrite(d1,1,32,f); fwrite(d2,1,32,f);
    fwrite(term,1,1,f);
    for (i=0;i<6;i++) {
        char rec[10];
        char nbuf[6];
        rec[0]=0x20;                          /* live */
        rec[1]=codes[i][0]; rec[2]=codes[i][1]; rec[3]=codes[i][2];
        snprintf(nbuf,sizeof nbuf,"%5d",amts[i]);  /* right-justified 5 */
        rec[4]=nbuf[0]; rec[5]=nbuf[1]; rec[6]=nbuf[2]; rec[7]=nbuf[3]; rec[8]=nbuf[4];
        rec[9]=oks[i];
        fwrite(rec,1,10,f);
    }
    fclose(f);
    return 0;
}

/* =====================================================================
 * Tier 0: LIST / DISPLAY / scope / FOR / WHILE / OFF / ?/?? / LOCATE / CONTINUE.
 * ===================================================================== */

static void test_tier0(samir_pal_t *pal)
{
    const char *pa = "/tmp/test_interp_list_q.dbf";
    xb_interp *ip;
    wa_env *env;
    int rc;
    char msg[256];

    CHECK(write_q_dbf(pa) == 0, "tier0: write synthetic table");

    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "tier0: xb_interp_make");
    if (!ip) { remove(pa); return; }
    rc = query_register(ip);
    CHECK(rc == INTERP_OK, "tier0: query_register");
    env = xb_interp_env(ip);

    rc = wa_set_open(env, 1, pa, "Q", NULL);
    snprintf(msg,sizeof msg,"tier0: USE rc=%d",rc);
    CHECK(rc == WA_OK, msg);
    if (rc != WA_OK) { xb_interp_free(ip); remove(pa); return; }
    wa_select(env, 1);

    /* ---- LIST (ALL): every record + recno column ---- */
    cap_clear();
    rc = samir_do(ip, "LIST\n");
    snprintf(msg,sizeof msg,"list-all: samir_do rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(cap_has("AAA"), "list-all: shows AAA");
    CHECK(cap_has("FFF"), "list-all: shows FFF");
    CHECK(cap_count("\n") == 6, "list-all: 6 output lines");
    /* recno column present (record 1 right-justified, then 6). */
    CHECK(cap_has("       1 AAA"), "list-all: recno col '1' + CODE");
    CHECK(cap_has("       6 FFF"), "list-all: recno col '6' + last CODE");
    /* LIST ALL leaves the pointer at EOF. */
    CHECK(wa_eof(env, 1) == 1, "list-all: pointer at EOF after walk");

    /* ---- LIST OFF: suppress the recno column ---- */
    cap_clear();
    rc = samir_do(ip, "LIST OFF\n");
    CHECK(rc == INTERP_OK, "list-off: rc");
    /* No leading-recno '       1 ' before AAA now: first line begins with AAA. */
    CHECK(strncmp(cap_text(), "AAA", 3) == 0, "list-off: first line starts with CODE (no recno col)");
    CHECK(!cap_has("       1 AAA"), "list-off: recno column suppressed");

    /* ---- LIST FIELDS subset (CODE only) ---- */
    cap_clear();
    rc = samir_do(ip, "LIST CODE OFF\n");
    CHECK(rc == INTERP_OK, "list-field: rc");
    CHECK(cap_count("\n") == 6, "list-field: 6 lines");
    /* CODE-only output: a line is exactly "AAA\n" etc (3 chars + newline). */
    CHECK(strncmp(cap_text(), "AAA\n", 4) == 0, "list-field: CODE-only line 'AAA'");

    /* ---- LIST FOR <cond>: AMT = 100 -> recnos 1,3,5 (3 rows) ---- */
    cap_clear();
    rc = samir_do(ip, "LIST FOR AMT = 100\n");
    snprintf(msg,sizeof msg,"list-for: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(cap_count("\n") == 3, "list-for: AMT=100 matches 3 records");
    CHECK(cap_has("       1 AAA"), "list-for: recno 1 (AMT 100)");
    CHECK(cap_has("       3 CCC"), "list-for: recno 3 (AMT 100)");
    CHECK(cap_has("       5 EEE"), "list-for: recno 5 (AMT 100)");
    CHECK(!cap_has("BBB"), "list-for: BBB (AMT 200) excluded");
    /* FOR visits EVERY record (does NOT stop at first false): recno 6 reached. */

    /* ---- LIST WHILE <cond>: from TOP, stop at first false. AMT=100 is true
     *      only for recno 1, false at recno 2 -> WHILE stops -> just recno 1. ---- */
    cap_clear();
    rc = samir_do(ip, "GO TOP\nLIST WHILE AMT = 100\n");
    CHECK(rc == INTERP_OK, "list-while: rc");
    CHECK(cap_count("\n") == 1, "list-while: stops at first false (1 row)");
    CHECK(cap_has("       1 AAA"), "list-while: only recno 1 before WHILE goes false");

    /* ---- DISPLAY (current record only): GO 4 then DISPLAY ---- */
    cap_clear();
    rc = samir_do(ip, "GO 4\nDISPLAY\n");
    snprintf(msg,sizeof msg,"display-cur: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(cap_count("\n") == 1, "display-cur: just one record");
    CHECK(cap_has("       4 DDD"), "display-cur: current record (recno 4 DDD)");
    /* DISPLAY current record does NOT move the pointer. */
    CHECK(wa_recno(env, 1) == 4u, "display-cur: pointer unchanged (recno 4)");

    /* ---- DISPLAY ALL: like LIST ALL (6 rows) ---- */
    cap_clear();
    rc = samir_do(ip, "DISPLAY ALL\n");
    CHECK(rc == INTERP_OK, "display-all: rc");
    CHECK(cap_count("\n") == 6, "display-all: 6 rows");

    /* ---- scope NEXT n: GO 2, LIST NEXT 3 CODE -> recnos 2,3,4 (CODE only) ---- */
    cap_clear();
    rc = samir_do(ip, "GO 2\nLIST NEXT 3 CODE OFF\n");
    CHECK(rc == INTERP_OK, "list-next: rc");
    CHECK(cap_count("\n") == 3, "list-next: NEXT 3 visits 3 records");
    CHECK(strncmp(cap_text(), "BBB\nCCC\nDDD\n", 12) == 0, "list-next: BBB,CCC,DDD (CODE field list)");

    /* ---- scope RECORD n: LIST RECORD 5 -> just recno 5 ---- */
    cap_clear();
    rc = samir_do(ip, "LIST RECORD 5\n");
    CHECK(rc == INTERP_OK, "list-record: rc");
    CHECK(cap_count("\n") == 1, "list-record: single record");
    CHECK(cap_has("       5 EEE"), "list-record: recno 5 EEE");

    /* ---- scope REST: GO 5, LIST REST CODE -> recnos 5,6 (CODE only) ---- */
    cap_clear();
    rc = samir_do(ip, "GO 5\nLIST REST CODE OFF\n");
    CHECK(rc == INTERP_OK, "list-rest: rc");
    CHECK(cap_count("\n") == 2, "list-rest: REST from recno 5 -> 2 records");
    CHECK(strncmp(cap_text(), "EEE\nFFF\n", 8) == 0, "list-rest: EEE,FFF (CODE field list)");

    /* ---- ? <expr>: a string literal + a field + a function call ---- */
    cap_clear();
    rc = samir_do(ip, "GO 1\n? 'CODE=' + CODE\n");
    snprintf(msg,sizeof msg,"q-expr: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(cap_has("CODE=AAA"), "q-expr: ? 'CODE=' + CODE -> CODE=AAA");
    /* ? prints a leading newline then the value. */
    CHECK(cap_text()[0] == '\n', "q-expr: ? emits a leading newline");

    /* ? of a numeric field (AMT=100 at recno 1) + a function (LEN). */
    cap_clear();
    rc = samir_do(ip, "? AMT\n? LEN(CODE)\n");
    CHECK(rc == INTERP_OK, "q-num: rc");
    CHECK(cap_has("100"), "q-num: ? AMT -> 100");
    CHECK(cap_has("3"),   "q-num: ? LEN(CODE) -> 3");

    /* ?? : no leading newline (continues on the same line). */
    cap_clear();
    rc = samir_do(ip, "?? 'X'\n?? 'Y'\n");
    CHECK(rc == INTERP_OK, "qq: rc");
    CHECK(strcmp(cap_text(), "XY") == 0, "qq: ?? has no leading/trailing newline -> 'XY'");

    /* ---- LOCATE / CONTINUE: FOR AMT = 100 finds recnos 1,3,5 in turn ---- */
    cap_clear();
    rc = samir_do(ip, "LOCATE FOR AMT = 100\n");
    snprintf(msg,sizeof msg,"locate: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(wa_recno(env, 1) == 1u, "locate: first match recno 1");
    CHECK(wa_found(env, 1) == 1,  "locate: FOUND() .T. on match");
    CHECK(wa_eof(env, 1) == 0,    "locate: EOF() .F. on match");

    rc = samir_do(ip, "CONTINUE\n");
    snprintf(msg,sizeof msg,"continue1: rc=%d recno=%u",rc,wa_recno(env,1));
    CHECK(rc == INTERP_OK, msg);
    /* MUTANT -DQUERY_MUTATE_CONTINUE_SCOPE makes this re-find recno 1. */
    CHECK(wa_recno(env, 1) == 3u, "continue1: next match recno 3 (NOT re-find 1)");
    CHECK(wa_found(env, 1) == 1,  "continue1: FOUND() .T.");

    rc = samir_do(ip, "CONTINUE\n");
    snprintf(msg,sizeof msg,"continue2: rc=%d recno=%u",rc,wa_recno(env,1));
    CHECK(rc == INTERP_OK, msg);
    CHECK(wa_recno(env, 1) == 5u, "continue2: next match recno 5");
    CHECK(wa_found(env, 1) == 1,  "continue2: FOUND() .T.");

    rc = samir_do(ip, "CONTINUE\n");
    CHECK(rc == INTERP_OK, "continue3: rc (no further match -> at EOF)");
    CHECK(wa_found(env, 1) == 0, "continue3: no further match -> FOUND() .F.");
    CHECK(wa_eof(env, 1) == 1,   "continue3: no further match -> EOF() .T.");

    /* CONTINUE without a prior LOCATE in a FRESH interp -> #42. */
    {
        xb_interp *ip2 = xb_interp_make(pal);
        if (ip2) {
            wa_env *e2 = xb_interp_env(ip2);
            query_register(ip2);
            wa_set_open(e2, 1, pa, "Q", NULL);
            wa_select(e2, 1);
            rc = samir_do(ip2, "CONTINUE\n");
            snprintf(msg,sizeof msg,"continue-noloc: rc=%d ec=%d",rc,samir_last_error(ip2));
            CHECK(rc != INTERP_OK && samir_last_error(ip2) == 42, msg);
            xb_interp_free(ip2);
        }
    }

    /* ---- a non-Logical FOR is fail-loud #37 (no truthiness) ---- */
    rc = samir_do(ip, "LIST FOR AMT\n");
    snprintf(msg,sizeof msg,"for-type: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc != INTERP_OK && samir_last_error(ip) == XBEE_NOT_LOGICAL, msg);

    /* ---- SEEK on a non-indexed table -> #26 "Database is not indexed." ---- */
    rc = samir_do(ip, "SEEK 'AAA'\n");
    snprintf(msg,sizeof msg,"seek-noidx: rc=%d ec=%d",rc,samir_last_error(ip));
    CHECK(rc != INTERP_OK && samir_last_error(ip) == 26, msg);

    /* GATED (loud-skip): SET FILTER / SET DELETED + scope (S5.6). */
    fprintf(stderr,
        "  SKIP (LOUD): SET FILTER / SET DELETED interaction with scope (S5.6; GAPS secP) -- not asserted.\n");
    fprintf(stderr,
        "  SKIP (LOUD): exact dot-prompt column layout / ? inter-expr separator width "
        "([oracle-resolves] nav-query-display Open Q6/7) -- not asserted.\n");

    xb_interp_free(ip);
    remove(pa);
}

/* =====================================================================
 * Tier 1: SEEK / FIND on corpus master indexes (loud-skip if absent).
 * ===================================================================== */

static void test_tier1(samir_pal_t *pal, const char *base)
{
    char tours[1024], tourdate[1024], clients[1024], names[1024];
    xb_interp *ip;
    wa_env *env;
    wa_index_list il;
    int rc;
    char msg[256];

    join(tours,    sizeof tours,    base, SP_PATH "/TOURS.DBF");
    join(tourdate, sizeof tourdate, base, SP_PATH "/TOURDATE.NDX");
    join(clients,  sizeof clients,  base, SP_PATH "/CLIENTS.DBF");
    join(names,    sizeof names,    base, SP_PATH "/NAMES.NDX");

    if (!file_exists(tours) || !file_exists(tourdate) ||
        !file_exists(clients) || !file_exists(names)) {
        fprintf(stderr,
            "  SKIP (LOUD): corpus golden(s) absent under base '%s'\n"
            "               need: %s %s %s %s\n"
            "               (Tier-0 ran; -DQUERY_MUTATE_CONTINUE_SCOPE bites Tier 0)\n",
            base, tours, tourdate, clients, names);
        return;
    }

    /* ---- TOURS + TOURDATE (date master index). DEPARTURE of recno 1 is
     *      1985-08-05; SEEK CTOD('08/05/85') must land on recno 1, FOUND .T.
     *
     * NOTE: TOURS has a MEMO field, so wa_set_open opens the sibling .dbt. A
     * pre-existing latent bug in fs/dbt.c (dbt_open stores arena_mark = NULL, so
     * dbt_close resets the PAL arena to BASE -- not to the dbt-open mark) means
     * closing a memo table on a LIVE interpreter corrupts the interp's own ctx.
     * To stay correct (and to avoid masking that bug), each Tier-1 table uses its
     * OWN fresh interp here -- we never close+reopen a memo table on a shared
     * interp. (Follow-up: dbt.c should take a real mark; flagged to the
     * orchestrator.) ---- */
    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "tier1: xb_interp_make (TOURS)");
    if (!ip) return;
    query_register(ip);
    env = xb_interp_env(ip);

    il.names[0] = tourdate;
    il.count = 1;
    rc = wa_set_open(env, 1, tours, NULL, &il);
    snprintf(msg,sizeof msg,"tier1: USE TOURS INDEX TOURDATE rc=%d",rc);
    CHECK(rc == WA_OK, msg);
    if (rc != WA_OK) { xb_interp_free(ip); return; }
    wa_select(env, 1);
    CHECK(wa_master_order(env, 1) == 1, "tier1: TOURS master order = 1");

    rc = samir_do(ip, "SEEK CTOD('08/05/85')\n");
    snprintf(msg,sizeof msg,"seek-date: rc=%d ec=%d recno=%u",rc,samir_last_error(ip),wa_recno(env,1));
    CHECK(rc == INTERP_OK, msg);
    CHECK(wa_found(env, 1) == 1, "seek-date: FOUND() .T. (1985-08-05 present)");
    CHECK(wa_recno(env, 1) == 1u, "seek-date: lands on recno 1 (earliest departure)");

    /* SEEK a date not in the index -> FOUND .F., EOF .T. (no SOFTSEEK in III+). */
    rc = samir_do(ip, "SEEK CTOD('01/01/70')\n");
    CHECK(rc == INTERP_OK, "seek-miss: rc");
    CHECK(wa_found(env, 1) == 0, "seek-miss: FOUND() .F. (date absent)");
    CHECK(wa_eof(env, 1) == 1,   "seek-miss: EOF() .T. (failed seek lands at EOF)");

    xb_interp_free(ip);
    wa_nav_reset(1);

    /* ---- CLIENTS + NAMES (char master index, key = LASTNAME+FIRSTNAME).
     *      The first index key is "Adams" (recno 15). SEEK 'Adams' (EXACT OFF,
     *      begins-with) finds recno 15; FIND Adams (bare literal) does too.
     *      CLIENTS has NO memo, but a fresh interp keeps each table isolated. ---- */
    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "tier1: xb_interp_make (CLIENTS)");
    if (!ip) return;
    query_register(ip);
    env = xb_interp_env(ip);

    il.names[0] = names;
    il.count = 1;
    rc = wa_set_open(env, 1, clients, NULL, &il);
    snprintf(msg,sizeof msg,"tier1: USE CLIENTS INDEX NAMES rc=%d",rc);
    CHECK(rc == WA_OK, msg);
    if (rc != WA_OK) { xb_interp_free(ip); return; }
    wa_select(env, 1);

    rc = samir_do(ip, "SEEK 'Adams'\n");
    snprintf(msg,sizeof msg,"seek-char: rc=%d ec=%d recno=%u",rc,samir_last_error(ip),wa_recno(env,1));
    CHECK(rc == INTERP_OK, msg);
    CHECK(wa_found(env, 1) == 1, "seek-char: FOUND() .T. ('Adams' begins-with, EXACT OFF)");
    CHECK(wa_recno(env, 1) == 15u, "seek-char: lands on recno 15 (Adams, Nathan)");

    /* FIND with a bare unquoted literal (legacy form). */
    rc = samir_do(ip, "FIND Adams\n");
    snprintf(msg,sizeof msg,"find-char: rc=%d recno=%u",rc,wa_recno(env,1));
    CHECK(rc == INTERP_OK, msg);
    CHECK(wa_found(env, 1) == 1, "find-char: FOUND() .T. (FIND bare literal)");
    CHECK(wa_recno(env, 1) == 15u, "find-char: FIND Adams -> recno 15");

    /* SEEK a name not present -> FOUND .F. */
    rc = samir_do(ip, "SEEK 'Zzzzzz'\n");
    CHECK(rc == INTERP_OK, "seek-char-miss: rc");
    CHECK(wa_found(env, 1) == 0, "seek-char-miss: FOUND() .F. (no such key)");
    CHECK(wa_eof(env, 1) == 1,   "seek-char-miss: EOF() .T.");

    /* LIST on the indexed table walks in INDEX order: first row is recno 15. */
    cap_clear();
    rc = samir_do(ip, "GO TOP\nLIST OFF\n");
    CHECK(rc == INTERP_OK, "tier1: LIST in index order rc");
    /* First listed record (index order) is Nathan Adams -> FIRSTNAME 'Nathan'. */
    CHECK(strstr(cap_text(), "Nathan") != NULL, "tier1: index-order LIST first row has 'Nathan' (recno 15)");

    xb_interp_free(ip);
    wa_nav_reset(1);
}

/* =====================================================================
 * main
 * ===================================================================== */
int main(int argc, char **argv)
{
    const char *base = (argc > 1) ? argv[1] : "../dbase3-decomp";
    struct pal_host_cfg cfg;
    samir_pal_t *host, *pal;
    char path[1024];

    memset(&cfg, 0, sizeof cfg);
    cfg.date_yy   = 99;
    cfg.date_mm   = 12;
    cfg.date_dd   = 31;
    cfg.heap_size = 4u * 1024u * 1024u;

    host = pal_host_make(cfg);
    if (!host) { fprintf(stderr, "FATAL: pal_host_make returned NULL\n"); return 2; }
    pal = cap_pal_make(host);     /* the capturing PAL wraps the host PAL */

    test_tier0(pal);

    join(path, sizeof path, base, SP_PATH "/TOURS.DBF");
    if (!file_exists(path))
        fprintf(stderr,
            "  SKIP (LOUD): no corpus goldens under base '%s' (pass corpus base as argv[1]; Tier-0 ran)\n",
            base);
    test_tier1(pal, base);

    pal_host_free(host);
    return TEST_SUMMARY("test-interp-list");
}
