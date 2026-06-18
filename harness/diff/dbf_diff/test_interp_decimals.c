/*
 * harness/diff/dbf_diff/test_interp_decimals.c -- host oracle for the
 * SET DECIMALS effect on ? / ?? computed-numeric display (bead initech-7az.20).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Uses the seed
 * test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY). A non-zero
 * exit on any failed check ensures make gates cannot false-green (Law 2).
 *
 * WHAT THIS PROVES (the verified SET DECIMALS scope for ? display):
 *
 *   SET DECIMALS scope (numeric-and-string-formatting.md line 33,
 *   HELP.DBS:1413-1416 [verified: HELP.DBS]):
 *     "SET DECIMALS applies only to division, SQRT(), LOG(), VAL(), and EXP()"
 *
 *   The HEADLINE VERIFIED CASE (mint-results-002.md [verified]):
 *     ? 1/3  ->  0.33   (at SET DECIMALS default=2)
 *     [Source: mint-results-002.md "SET DECIMALS default = 2 (? 1/3 -> 0.33 ...)"]
 *
 *   This oracle proves:
 *     1. DEFAULT (no SET issued): ? 1/3  ->  0.33
 *        [verified: mint-results-002.md]
 *     2. SET DECIMALS TO 4:      ? 1/3  ->  0.3333
 *        [grounded: set-commands.md "minimum decimal places" + the verified default-2 case]
 *     3. SET DECIMALS TO 0:      ? 1/3  ->  0
 *        [grounded: SET DECIMALS is the floor; at 0, no fractional digits]
 *     4. INTEGER literals:       ? 5    ->  5
 *        [grounded: numeric-and-string-formatting.md S5.3 "a literal numeric is
 *         NOT widened to DECIMALS places merely by being ?-printed"; integral
 *         heuristic: fractional part <= 1e-9 -> dec=0 always]
 *     5. VAL of an integer string: ? VAL("42") -> 42
 *        [grounded: VAL("42") returns 42.0 (integral); same integral heuristic
 *         -> dec=0 even though VAL is in the DECIMALS scope]
 *     6. STR() is UNAFFECTED: a STR() call returns XB_C (not XB_N), so the
 *        numeric render path is NEVER taken for STR output. Verified separately
 *        in test_interp_setfmt.c.
 *
 * MUTATION PROOF (Rule 6 / CLAUDE.md Rule 6):
 *   Build with -DDEC_MUTATE_IGNORE_SETDEC: query.c's q_render_val() is forced
 *   to use dec=2 regardless of ctx->set_decimals. Then:
 *     - SET DECIMALS TO 4, ? 1/3  -> still "0.33" (wrong: want "0.3333") -> RED
 *     - SET DECIMALS TO 0, ? 1/3  -> still "0.33" (wrong: want "0")       -> RED
 *   These assertions go RED, proving the oracle catches the missing
 *   ctx->set_decimals read in the numeric display path.
 *   A non-zero exit is EXPECTED from the mutant build.
 *
 * GATED (loud-skip):
 *   - SQRT, LOG, EXP: not yet implemented (bead 7az.13). They are in the
 *     verified SET DECIMALS scope but cannot be tested until they land.
 *   - VAL("3.14159") display: VAL returns 3.14159 (non-integral); in our
 *     implementation it will display with set_decimals decimal places. However
 *     the exact dBASE III+ behavior for "how many decimals does VAL() return
 *     when the source string has more decimals than SET DECIMALS?" is
 *     [oracle-resolves] (does it show more places or clamp to DECIMALS?).
 *     GATED: we do not assert the exact decimal count for non-integer VAL results.
 *   - The integer-vs-derived distinction at the value layer: in our impl we use
 *     a fractional-part heuristic (fr > 1e-9 -> use set_decimals). Whether an
 *     integer result from a division (e.g. ? 6/2 = 3) would show trailing zeros
 *     under SET DECIMALS is [oracle-resolves]. GATED: not asserted here.
 *
 * Output capture: the same cap_pal wrapper pattern used by test_interp_list.c.
 *
 * Goldens: none needed -- this oracle is self-contained (pure expression tests;
 * no corpus .dbf files required). argv[1] (corpus path) is accepted but unused.
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/runtime/numeric-and-string-formatting.md
 *       line 33 (SET DECIMALS closed scope: division/SQRT/LOG/VAL/EXP [verified]);
 *       S5.1 (DECIMALS sets the MINIMUM; FIXED OFF = floor not fixed);
 *       S5.3 (what DECIMALS does NOT affect: STR, N-field display, PICTURE).
 *   - ../dbase3-decomp/re/mint-results-002.md
 *       "SET DECIMALS default = 2; ? 1/3 -> 0.33" [verified: real III+ under dosbox-x].
 *   - os/samir/include/samir/eval.h (xb_ctx.set_decimals; uint8_t; default 2).
 *   - os/samir/include/samir/set.h  (set_register; set_get_decimals).
 *   - os/samir/cmd/query.c          (q_render_val: the XB_N branch reads
 *                                    ctx->set_decimals for non-integral values).
 *   - seed/test_assert.h (CHECK / TEST_HARNESS / TEST_SUMMARY).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "test_assert.h"
#include "samir/interp.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"
#include "samir/set.h"

TEST_HARNESS();

/* pal_host.c surface (declared where used; not in a header). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* query.c registration entry point. */
int query_register(xb_interp *ip);

/* =====================================================================
 * Capturing PAL: intercepts conout into a byte buffer.
 * Identical pattern to test_interp_list.c.
 * ===================================================================== */

#define CAP_BUF 4096

typedef struct {
    samir_pal_t  pal;
    samir_pal_t *inner;
    char         buf[CAP_BUF];
    uint32_t     len;
} cap_pal_t;

static cap_pal_t g_cap;

static pal_fd  dcap_open (samir_pal_t *p, const char *n, int m)
    { cap_pal_t *c=(cap_pal_t*)p; return c->inner->open(c->inner,n,m); }
static int     dcap_close(samir_pal_t *p, pal_fd fd)
    { cap_pal_t *c=(cap_pal_t*)p; return c->inner->close(c->inner,fd); }
static int32_t dcap_read (samir_pal_t *p, pal_fd fd, void *b, uint32_t n)
    { cap_pal_t *c=(cap_pal_t*)p; return c->inner->read(c->inner,fd,b,n); }
static int32_t dcap_write(samir_pal_t *p, pal_fd fd, const void *b, uint32_t n)
    { cap_pal_t *c=(cap_pal_t*)p; return c->inner->write(c->inner,fd,b,n); }
static int32_t dcap_seek (samir_pal_t *p, pal_fd fd, int32_t o, int w)
    { cap_pal_t *c=(cap_pal_t*)p; return c->inner->seek(c->inner,fd,o,w); }
static int     dcap_remove(samir_pal_t *p, const char *n)
    { cap_pal_t *c=(cap_pal_t*)p; return c->inner->remove(c->inner,n); }
static int     dcap_rename(samir_pal_t *p, const char *f, const char *t)
    { cap_pal_t *c=(cap_pal_t*)p; return c->inner->rename(c->inner,f,t); }

static void dcap_conout(samir_pal_t *p, const char *s, uint32_t n)
{
    cap_pal_t *c = (cap_pal_t *)p;
    uint32_t i;
    for (i = 0; i < n && c->len < (uint32_t)(CAP_BUF - 1); i++)
        c->buf[c->len++] = s[i];
    c->buf[c->len] = '\0';
}

static int32_t dcap_conin_line(samir_pal_t *p, char *b, uint32_t cap)
    { cap_pal_t *c=(cap_pal_t*)p; return c->inner->conin_line(c->inner,b,cap); }
static int32_t dcap_conin_char(samir_pal_t *p)
    { cap_pal_t *c=(cap_pal_t*)p; return c->inner->conin_char(c->inner); }
static void    dcap_gotoxy(samir_pal_t *p, uint8_t r, uint8_t col)
    { cap_pal_t *c=(cap_pal_t*)p; c->inner->gotoxy(c->inner,r,col); }
static void    dcap_set_attr(samir_pal_t *p, uint8_t a)
    { cap_pal_t *c=(cap_pal_t*)p; c->inner->set_attr(c->inner,a); }
static void    dcap_today(samir_pal_t *p, uint8_t *yy, uint8_t *mm, uint8_t *dd)
    { cap_pal_t *c=(cap_pal_t*)p; c->inner->today(c->inner,yy,mm,dd); }
static void   *dcap_alloc(samir_pal_t *p, uint32_t n)
    { cap_pal_t *c=(cap_pal_t*)p; return c->inner->alloc(c->inner,n); }
static void    dcap_reset(samir_pal_t *p, void *m)
    { cap_pal_t *c=(cap_pal_t*)p; c->inner->reset(c->inner,m); }

static samir_pal_t *cap_pal_make(samir_pal_t *inner)
{
    g_cap.inner      = inner;
    g_cap.len        = 0;
    g_cap.buf[0]     = '\0';
    g_cap.pal.open       = dcap_open;
    g_cap.pal.close      = dcap_close;
    g_cap.pal.read       = dcap_read;
    g_cap.pal.write      = dcap_write;
    g_cap.pal.seek       = dcap_seek;
    g_cap.pal.remove     = dcap_remove;
    g_cap.pal.rename     = dcap_rename;
    g_cap.pal.conout     = dcap_conout;
    g_cap.pal.conin_line = dcap_conin_line;
    g_cap.pal.conin_char = dcap_conin_char;
    g_cap.pal.gotoxy     = dcap_gotoxy;
    g_cap.pal.set_attr   = dcap_set_attr;
    g_cap.pal.today      = dcap_today;
    g_cap.pal.alloc      = dcap_alloc;
    g_cap.pal.reset      = dcap_reset;
    return &g_cap.pal;
}

/* cap_clear: reset the capture buffer before each ? command. */
static void cap_clear(void) { g_cap.len = 0; g_cap.buf[0] = '\0'; }

/*
 * cap_line: return the FIRST non-empty line from the capture buffer (skips the
 * leading '\n' that ? emits before the value). The "?" command outputs "\n<value>\n";
 * the first '\n' is the leading newline, and the value follows on the second line.
 * We return a pointer to the start of the value text (after the first '\n') and
 * NUL-terminate at the trailing '\n'.
 *
 * The result points into g_cap.buf (modifies in place -- only safe to call once
 * per capture interval).
 */
static const char *cap_line(void)
{
    char *p = g_cap.buf;
    char *nl;
    /* Skip the leading newline that "?" always emits. */
    if (*p == '\n') p++;
    /* Trim trailing newline. */
    nl = strchr(p, '\n');
    if (nl) *nl = '\0';
    return p;
}

/* =====================================================================
 * run_decimals_tests: the main oracle body.
 * ===================================================================== */

static int run_decimals_tests(const char *corpus_path)
{
    struct pal_host_cfg cfg = { 85, 8, 5, 512*1024 };
    samir_pal_t  *host = pal_host_make(cfg);
    samir_pal_t  *pal  = cap_pal_make(host);
    xb_interp    *ip   = xb_interp_make(pal);
    int           rc;
    const char   *got;

    (void)corpus_path;   /* no corpus files needed for these pure-expression tests */

    if (!ip) {
        fprintf(stderr, "  FAIL: xb_interp_make returned NULL\n");
        g_fails++;
        pal_host_free(host);
        return 1;
    }

    /* Wire SET (for SET DECIMALS TO) and query (for ? display). */
    rc = set_register(ip);
    CHECK(rc == INTERP_OK, "set_register returned INTERP_OK");
    rc = query_register(ip);
    CHECK(rc == INTERP_OK, "query_register returned INTERP_OK");

    /* Confirm the III+ default: SET DECIMALS = 2.
     * [verified: mint-results-002.md "SET DECIMALS default = 2"]              */
    CHECK(set_get_decimals(ip) == 2,
          "fresh interp: set_decimals default == 2 [verified: mint-results-002.md]");

    /* ------------------------------------------------------------------ */
    /* 1. ? 1/3 at DEFAULT DECIMALS=2 -> "0.33"                           */
    /*                                                                     */
    /* THE HEADLINE VERIFIED CASE.                                         */
    /* Ref: mint-results-002.md "? 1/3 -> 0.33 (SET DECIMALS default=2)"  */
    /* [verified: real dBASE III+ 1.1 under dosbox-x, C2.TXT transcript]  */
    /* ------------------------------------------------------------------ */
    cap_clear();
    rc = samir_do(ip, "? 1/3");
    CHECK(rc == 0, "? 1/3 (default DECIMALS=2): samir_do returned 0");
    got = cap_line();
    CHECK(strcmp(got, "0.33") == 0,
          "? 1/3 (default DECIMALS=2): output == '0.33' [verified: mint-002 C2.TXT] [MUTANT]");

    /* ------------------------------------------------------------------ */
    /* 2. ? 5 (integer literal) -> "5"                                    */
    /*                                                                     */
    /* An integral literal is NOT widened by SET DECIMALS.                */
    /* Ref: numeric-and-string-formatting.md S5.3 "a literal numeric      */
    /* memvar is NOT widened to DECIMALS places merely by being ?-printed" */
    /* Ground: fractional part <= 1e-9 -> dec=0 regardless of set_decimals*/
    /* ------------------------------------------------------------------ */
    cap_clear();
    rc = samir_do(ip, "? 5");
    CHECK(rc == 0, "? 5 (integer literal, DECIMALS=2): samir_do returned 0");
    got = cap_line();
    CHECK(strcmp(got, "5") == 0,
          "? 5 (integer literal, DECIMALS=2): output == '5' (integral -> dec=0)");

    /* ------------------------------------------------------------------ */
    /* 3. ? VAL("42") -> "42" (VAL of an integer string = integral)       */
    /*                                                                     */
    /* VAL is in the SET DECIMALS scope (verified scope list), but        */
    /* VAL("42") parses to 42.0 -- the fractional part is 0, so the       */
    /* integral heuristic applies and dec=0 regardless of DECIMALS.       */
    /* Ref: numeric-and-string-formatting.md S4.3 "decimal-place count    */
    /* follows the digits actually parsed"; 42 has 0 fractional digits.   */
    /* ------------------------------------------------------------------ */
    cap_clear();
    rc = samir_do(ip, "? VAL('42')");
    CHECK(rc == 0, "? VAL('42') (DECIMALS=2): samir_do returned 0");
    got = cap_line();
    CHECK(strcmp(got, "42") == 0,
          "? VAL('42') (DECIMALS=2): output == '42' (integral -> dec=0)");

    /* ------------------------------------------------------------------ */
    /* 4. SET DECIMALS TO 4, then ? 1/3 -> "0.3333"                       */
    /*                                                                     */
    /* SET DECIMALS TO 4 raises the minimum to 4 decimal places.          */
    /* Ground: set-commands.md S3.1 "SET DECIMALS sets the MINIMUM number  */
    /* of decimal places" + the verified default-2 case scaled to 4.      */
    /* ------------------------------------------------------------------ */
    rc = samir_do(ip, "SET DECIMALS TO 4");
    CHECK(rc == 0, "SET DECIMALS TO 4: samir_do returned 0");
    CHECK(set_get_decimals(ip) == 4, "SET DECIMALS TO 4: getter == 4");

    cap_clear();
    rc = samir_do(ip, "? 1/3");
    CHECK(rc == 0, "? 1/3 (DECIMALS=4): samir_do returned 0");
    got = cap_line();
    CHECK(strcmp(got, "0.3333") == 0,
          "? 1/3 (DECIMALS=4): output == '0.3333' [MUTANT]");

    /* Integer literal still unaffected. */
    cap_clear();
    rc = samir_do(ip, "? 5");
    CHECK(rc == 0, "? 5 (DECIMALS=4): samir_do returned 0");
    got = cap_line();
    CHECK(strcmp(got, "5") == 0,
          "? 5 (DECIMALS=4): output == '5' (integral -> dec=0, unaffected)");

    /* ------------------------------------------------------------------ */
    /* 5. SET DECIMALS TO 0, then ? 1/3 -> "0"                            */
    /*                                                                     */
    /* At DECIMALS=0, the minimum decimal places is zero, so 1/3 rounds   */
    /* to 0 (the nearest integer, displayed with no fractional digits).   */
    /* Ground: set-commands.md S3.1 "Range is 0..the practical width      */
    /* limit" + dec_format(1/3, 20, 0) = "0".                             */
    /* ------------------------------------------------------------------ */
    rc = samir_do(ip, "SET DECIMALS TO 0");
    CHECK(rc == 0, "SET DECIMALS TO 0: samir_do returned 0");
    CHECK(set_get_decimals(ip) == 0, "SET DECIMALS TO 0: getter == 0");

    cap_clear();
    rc = samir_do(ip, "? 1/3");
    CHECK(rc == 0, "? 1/3 (DECIMALS=0): samir_do returned 0");
    got = cap_line();
    CHECK(strcmp(got, "0") == 0,
          "? 1/3 (DECIMALS=0): output == '0' [MUTANT]");

    /* ------------------------------------------------------------------ */
    /* 6. Restore to default (SET DECIMALS TO 2), re-check ? 1/3 -> "0.33"*/
    /* ------------------------------------------------------------------ */
    rc = samir_do(ip, "SET DECIMALS TO 2");
    CHECK(rc == 0, "SET DECIMALS TO 2 (restore): samir_do returned 0");
    CHECK(set_get_decimals(ip) == 2, "SET DECIMALS TO 2 (restore): getter == 2");

    cap_clear();
    rc = samir_do(ip, "? 1/3");
    CHECK(rc == 0, "? 1/3 (DECIMALS=2 restored): samir_do returned 0");
    got = cap_line();
    CHECK(strcmp(got, "0.33") == 0,
          "? 1/3 (DECIMALS=2 restored): output == '0.33' (round-trip)");

    /* ------------------------------------------------------------------ */
    /* GATED BEHAVIORS -- loud-skip (not asserted; [oracle-resolves])      */
    /* ------------------------------------------------------------------ */

    fprintf(stderr,
        "  GATE: SQRT/LOG/EXP SET DECIMALS display -- bead 7az.13 (not implemented);"
        " gated pending those functions landing.\n");

    fprintf(stderr,
        "  GATE: VAL of a non-integer string (e.g. VAL('3.14159')) display with"
        " set_decimals -- the exact 'shows more places if naturally present' vs"
        " 'clamps to DECIMALS' behavior is [oracle-resolves] (numeric-and-string-"
        "formatting.md S5.1 FIXED OFF semantics needs live DBASE.EXE).\n");

    fprintf(stderr,
        "  GATE: ? 6/2 (integer-valued division) trailing zeros under SET DECIMALS"
        " -- whether an exact-integer division result shows decimals is"
        " [oracle-resolves] (fractional-part heuristic gives dec=0 for 6/2=3.0).\n");

    fprintf(stderr,
        "  GATE: STR() unaffected by SET DECIMALS -- verified and proven in"
        " test_interp_setfmt.c; not re-proven here (STR returns XB_C, never"
        " reaches the XB_N render branch).\n");

    xb_interp_free(ip);
    pal_host_free(host);
    return 0;
}

/* =====================================================================
 * main
 * ===================================================================== */
int main(int argc, char **argv)
{
    const char *corpus = (argc > 1) ? argv[1] : "../dbase3-decomp";
    int rc;

    printf("test_interp_decimals: SET DECIMALS effect on ? display\n");
    printf("  Ground: mint-results-002.md '? 1/3 -> 0.33 (DECIMALS=2)' [verified]\n");
    printf("  Scope:  numeric-and-string-formatting.md:33 (division/SQRT/LOG/VAL/EXP)\n");

    rc = run_decimals_tests(corpus);

    (void)rc;
    return TEST_SUMMARY("test_interp_decimals");
}
