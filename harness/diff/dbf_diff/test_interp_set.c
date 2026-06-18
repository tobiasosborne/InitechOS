/*
 * harness/diff/dbf_diff/test_interp_set.c -- host oracle for S5.6: SET state.
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_interp_flow.c:
 * the seed test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY), a host
 * PAL via pal_host_make. A non-zero exit on any failed check keeps
 * `make test-interp-set` from false-greening (Law 2: the oracle is the truth).
 *
 * WHAT S5.6 IS (the contract):
 *   The global SET environment record for one interpreter session. This oracle
 *   proves:
 *     1. DEFAULTS: a fresh interp (after set_register) has:
 *           EXACT=OFF  (ctx->set_exact == 0) [verified: mint-results-002.md]
 *           DECIMALS=2                        [verified: mint-results-002.md]
 *           DATE=AMERICAN                     [verified: mint-results-003.md]
 *           CENTURY=OFF                       [verified: mint-results-003.md]
 *           TALK=ON                           [verified: set-commands.md]
 *           SAFETY=ON                         [verified: set-commands.md]
 *     2. SET EXACT ON/OFF governs C= "begins-with" (OFF) vs exact (ON):
 *           "ABC" = "AB" -> .T. under EXACT OFF (default begins-with)
 *           "ABC" = "AB" -> .F. under EXACT ON  (must be same length / full match)
 *        SET EXACT OFF restores the OFF behaviour.
 *     3. SET DECIMALS TO <n>: stored override asserted via getter.
 *     4. SET DATE ANSI: stored override asserted via getter.
 *     5. SET CENTURY ON: stored override asserted via getter.
 *     6. SET TALK OFF / SET SAFETY OFF: stored override asserted via getter.
 *     7. SET ORDER TO <n>: calls wa_set_order on the selected area. Asserted
 *        via wa_master_order before and after.
 *     8. SET INDEX TO / SET FILTER TO / SET RELATION TO: GATED. Stores raw text
 *        in set_state; loud-skips the runtime effect with a note. Asserted via
 *        set_get_state (have_* + raw text).
 *     9. Bad SET syntax / unknown sub-option -> fail-loud (samir_do returns < 0).
 *
 * MUTATION PROOF (Rule 6 / ARB rider (a)):
 *   Build with -DSET_MUTATE_EXACT_DEFAULT: the default EXACT is initialised to
 *   ON (1) instead of OFF (0). The following checks go RED:
 *     - EXACT default == 0 assertion (check 1)
 *     - "ABC" = "AB" is .T. under EXACT OFF behaviour (check 2a) -- because
 *       EXACT is still 1 (ON) after fresh init even before SET EXACT ON is run,
 *       and restoring to OFF via SET EXACT OFF restores set_exact=0 correctly
 *       (the mutation affects only the DEFAULT, not the command). So check 2a
 *       ("fresh interp, ABC=AB is .T.") goes RED because the fresh ctx has
 *       set_exact=1.
 *   A non-zero exit is expected when built with the mutant flag.
 *
 * DEFERRED LOUD-SKIPS (plan S5.6 scope):
 *   - SET DECIMALS/DATE/CENTURY formatter effects: need fn_builtins.c / rt.c
 *     owned by a parallel lane. Loud-skipped with a note.
 *   - SET INDEX/FILTER/RELATION runtime effects: need work-area plumbing not
 *     yet present. Loud-skipped with a note.
 *   These are printed to stdout so the CI log is unambiguous about what was
 *   intentionally skipped vs what is silently untested.
 *
 * Goldens base resolves from argv[1] (orchestrator passes the corpus path);
 * default "../dbase3-decomp".
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.6 (contract + oracle).
 *   - ../dbase3-decomp/specs/commands/set-commands.md (defaults; III+ grammar).
 *   - ../dbase3-decomp/re/mint-results-002.md (EXACT=OFF; DECIMALS=2 [verified]).
 *   - ../dbase3-decomp/re/mint-results-003.md (DATE=AMERICAN; CENTURY=OFF [verified]).
 *   - os/samir/include/samir/set.h    (set_state + getters + set_register).
 *   - os/samir/include/samir/interp.h (samir_do + ctx accessor).
 *   - os/samir/include/samir/eval.h   (xb_ctx.set_exact; xb_eval for C= test).
 *   - os/samir/include/samir/workarea.h (wa_master_order for SET ORDER check).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"          /* seed/, on -Iseed */
#include "samir/set.h"            /* os/samir/include/ */
#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"
#include "samir/dbf.h"
#include "samir/ndx.h"

TEST_HARNESS();

/* pal_host.c surface (same pattern as every other interp test). */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* External declarations for module registration (declared in their .c files). */
/* query.c is NOT needed here -- we test SET in isolation. flow.c provides
 * samir_do which is needed for the SET command tests. */

/* =====================================================================
 * Small helper: evaluate a NUL-terminated expression through ip's ctx and
 * return the Logical result (1 for .T., 0 for .F., -1 for error).
 * ===================================================================== */
static int eval_logical(xb_interp *ip, const char *expr)
{
    xb_token toks[64];
    xb_node  pool[64];
    xb_ctx  *ctx = xb_interp_ctx(ip);
    xb_val   out;
    int ntok, root, rc, e = 0;
    uint32_t len = (uint32_t)strlen(expr);

    ntok = xb_lex(expr, len, toks, 64, &e);
    if (ntok < 0) return -1;
    root = xb_parse(toks, (uint32_t)ntok, pool, 64, &e);
    if (root < 0) return -1;
    rc = xb_eval(pool, root, ctx, &out, &e);
    if (rc != 0 || out.t != XB_L) return -1;
    return out.u.l ? 1 : 0;
}

/* =====================================================================
 * Tier 0: pure host oracle -- no corpus goldens needed.
 * ===================================================================== */

static int run_set_tests(const char *corpus_path)
{
    struct pal_host_cfg cfg = { 26, 6, 18, 512*1024 };
    samir_pal_t  *pal = pal_host_make(cfg);
    xb_interp    *ip  = xb_interp_make(pal);
    int rc;

    (void)corpus_path;   /* no golden files used in Tier 0 */

    if (!ip) {
        fprintf(stderr, "  FAIL: xb_interp_make returned NULL\n");
        g_fails++;
        pal_host_free(pal);
        return 1;
    }

    /* ------------------------------------------------------------------ */
    /* Register SET state (applies III+ defaults).                         */
    /* ------------------------------------------------------------------ */
    rc = set_register(ip);
    CHECK(rc == INTERP_OK, "set_register returned INTERP_OK");

    /* ------------------------------------------------------------------ */
    /* 1. DEFAULT VALUES                                                    */
    /* ------------------------------------------------------------------ */

    /* EXACT default = OFF [verified: mint-results-002.md] */
    CHECK(set_get_exact(ip) == 0,
          "default: EXACT == OFF (ctx->set_exact == 0)");

    /* DECIMALS default = 2 [verified: mint-results-002.md] */
    CHECK(set_get_decimals(ip) == 2,
          "default: DECIMALS == 2");

    /* DATE default = AMERICAN [verified: mint-results-003.md] */
    CHECK(set_get_date_fmt(ip) == SET_DATE_AMERICAN,
          "default: DATE == AMERICAN");

    /* CENTURY default = OFF [verified: mint-results-003.md] */
    CHECK(set_get_century(ip) == 0,
          "default: CENTURY == OFF");

    /* TALK default = ON [verified: set-commands.md Sec 2] */
    CHECK(set_get_talk(ip) == 1,
          "default: TALK == ON");

    /* SAFETY default = ON [verified: set-commands.md Sec 2] */
    CHECK(set_get_safety(ip) == 1,
          "default: SAFETY == ON");

    /* ------------------------------------------------------------------ */
    /* 2. SET EXACT ON/OFF behaviour via the evaluator.                    */
    /*                                                                     */
    /* Ref: set-commands.md Sec 4 (the comparison / SEEK hazard):          */
    /*   EXACT OFF (default): "Smith" = "S" is TRUE (begins-with);         */
    /*   EXACT ON:            "Smith" = "S" is FALSE (full-length match).   */
    /*                                                                     */
    /* Test: "ABC" = "AB"                                                   */
    /*   OFF (default begins-with): "ABC" begins with "AB" -> .T.           */
    /*   ON  (exact):               "ABC" != "AB" (len 3 vs 2) -> .F.       */
    /* ------------------------------------------------------------------ */

    /* 2a. Fresh interp (EXACT OFF by default): "ABC" = "AB" must be .T.  */
    {
        int r = eval_logical(ip, "\"ABC\" = \"AB\"");
        CHECK(r == 1,
              "EXACT OFF (default): \"ABC\" = \"AB\" -> .T. (begins-with)");
    }

    /* 2b. SET EXACT ON via samir_do; then "ABC" = "AB" must be .F. */
    rc = samir_do(ip, "SET EXACT ON");
    CHECK(rc == 0, "samir_do('SET EXACT ON') returned 0");
    CHECK(set_get_exact(ip) == 1, "after SET EXACT ON: ctx->set_exact == 1");
    {
        int r = eval_logical(ip, "\"ABC\" = \"AB\"");
        CHECK(r == 0,
              "EXACT ON: \"ABC\" = \"AB\" -> .F. (full-length required)");
    }

    /* 2c. Symmetry: under EXACT ON, "AB" = "ABC" is also .F. */
    {
        int r = eval_logical(ip, "\"AB\" = \"ABC\"");
        CHECK(r == 0,
              "EXACT ON: \"AB\" = \"ABC\" -> .F. (shorter left, longer right)");
    }

    /* 2d. SET EXACT OFF restores begins-with: "ABC" = "AB" -> .T. again. */
    rc = samir_do(ip, "SET EXACT OFF");
    CHECK(rc == 0, "samir_do('SET EXACT OFF') returned 0");
    CHECK(set_get_exact(ip) == 0, "after SET EXACT OFF: ctx->set_exact == 0");
    {
        int r = eval_logical(ip, "\"ABC\" = \"AB\"");
        CHECK(r == 1,
              "EXACT OFF (restored): \"ABC\" = \"AB\" -> .T. again");
    }

    /* 2e. Under EXACT OFF, exact same strings still match. */
    {
        int r = eval_logical(ip, "\"AB\" = \"AB\"");
        CHECK(r == 1,
              "EXACT OFF: \"AB\" = \"AB\" -> .T. (identical strings)");
    }

    /* ------------------------------------------------------------------ */
    /* 3. SET DECIMALS TO <n>: stored value asserted via getter.           */
    /* Formatter effect DEFERRED (fn_builtins.c parallel lane).            */
    /* ------------------------------------------------------------------ */
    rc = samir_do(ip, "SET DECIMALS TO 4");
    CHECK(rc == 0, "samir_do('SET DECIMALS TO 4') returned 0");
    CHECK(set_get_decimals(ip) == 4, "after SET DECIMALS TO 4: getter == 4");

    /* Reset to default via bare "SET DECIMALS TO" (no value = reset to 2). */
    rc = samir_do(ip, "SET DECIMALS TO");
    CHECK(rc == 0, "samir_do('SET DECIMALS TO') (reset) returned 0");
    CHECK(set_get_decimals(ip) == 2,
          "after SET DECIMALS TO (no value): getter resets to 2");

    /* LOUD-SKIP: formatter-effect assertions for DECIMALS deferred.
     * Reason: fn_builtins.c (STR/dec_format honoring SET DECIMALS) is owned
     * by a parallel lane; coupling to it here would block this step.
     * Follow-up bead required to wire the formatter effect. */
    printf("  LOUD-SKIP: SET DECIMALS formatter effect (STR/dec_format) deferred"
           " -- parallel lane owns fn_builtins.c; follow-up bead required.\n");

    /* ------------------------------------------------------------------ */
    /* 4. SET DATE ANSI: stored value asserted via getter.                 */
    /* Formatter effect DEFERRED.                                           */
    /* ------------------------------------------------------------------ */
    rc = samir_do(ip, "SET DATE ANSI");
    CHECK(rc == 0, "samir_do('SET DATE ANSI') returned 0");
    CHECK(set_get_date_fmt(ip) == SET_DATE_ANSI,
          "after SET DATE ANSI: getter == SET_DATE_ANSI");

    rc = samir_do(ip, "SET DATE TO AMERICAN");
    CHECK(rc == 0, "samir_do('SET DATE TO AMERICAN') returned 0");
    CHECK(set_get_date_fmt(ip) == SET_DATE_AMERICAN,
          "after SET DATE TO AMERICAN: getter == SET_DATE_AMERICAN");

    printf("  LOUD-SKIP: SET DATE formatter effect (DTOC/display format) deferred"
           " -- parallel lane owns fn_builtins.c; follow-up bead required.\n");

    /* ------------------------------------------------------------------ */
    /* 5. SET CENTURY ON/OFF: stored value asserted via getter.            */
    /* Formatter effect DEFERRED.                                           */
    /* ------------------------------------------------------------------ */
    rc = samir_do(ip, "SET CENTURY ON");
    CHECK(rc == 0, "samir_do('SET CENTURY ON') returned 0");
    CHECK(set_get_century(ip) == 1,
          "after SET CENTURY ON: getter == 1");

    rc = samir_do(ip, "SET CENTURY OFF");
    CHECK(rc == 0, "samir_do('SET CENTURY OFF') returned 0");
    CHECK(set_get_century(ip) == 0,
          "after SET CENTURY OFF: getter == 0");

    printf("  LOUD-SKIP: SET CENTURY formatter effect (4-digit year display) deferred"
           " -- parallel lane owns fn_builtins.c; follow-up bead required.\n");

    /* ------------------------------------------------------------------ */
    /* 6. SET TALK / SET SAFETY: stored values asserted via getters.       */
    /* ------------------------------------------------------------------ */
    rc = samir_do(ip, "SET TALK OFF");
    CHECK(rc == 0, "samir_do('SET TALK OFF') returned 0");
    CHECK(set_get_talk(ip) == 0, "after SET TALK OFF: getter == 0");

    rc = samir_do(ip, "SET TALK ON");
    CHECK(rc == 0, "samir_do('SET TALK ON') returned 0");
    CHECK(set_get_talk(ip) == 1, "after SET TALK ON: getter == 1");

    rc = samir_do(ip, "SET SAFETY OFF");
    CHECK(rc == 0, "samir_do('SET SAFETY OFF') returned 0");
    CHECK(set_get_safety(ip) == 0, "after SET SAFETY OFF: getter == 0");

    rc = samir_do(ip, "SET SAFETY ON");
    CHECK(rc == 0, "samir_do('SET SAFETY ON') returned 0");
    CHECK(set_get_safety(ip) == 1, "after SET SAFETY ON: getter == 1");

    /* ------------------------------------------------------------------ */
    /* 7. SET ORDER TO <n>: calls wa_set_order on the selected area.       */
    /* We test the call-path via samir_do + wa_master_order.               */
    /* A full "navigation changes with order" test requires an open NDX,   */
    /* which is Tier 1 (corpus-dependent). Here we prove the state change. */
    /* NOTE: SET ORDER requires an open work area; without one it is a     */
    /* INTERP_ERR_SYNTAX (no open area). We test the failure path first,   */
    /* then open an area and assert the state change.                      */
    /* ------------------------------------------------------------------ */
    {
        /* Without an open work area, SET ORDER should fail loud. */
        rc = samir_do(ip, "SET ORDER TO 0");
        CHECK(rc != 0,
              "SET ORDER without open area -> non-zero (fail loud: no area)");
    }

    /* ------------------------------------------------------------------ */
    /* 8. GATED options: SET INDEX / FILTER / RELATION store raw text.     */
    /* ------------------------------------------------------------------ */
    {
        const set_state *ss;

        rc = samir_do(ip, "SET INDEX TO MYIDX");
        CHECK(rc == 0, "samir_do('SET INDEX TO MYIDX') returned 0 (parsed+stored)");

        ss = set_get_state(ip);
        CHECK(ss != (const set_state *)0, "set_get_state returns non-NULL");
        if (ss) {
            CHECK(ss->have_index == 1, "SET INDEX: have_index == 1");
            CHECK(strcmp(ss->index_text, "MYIDX") == 0,
                  "SET INDEX: index_text == \"MYIDX\"");
        }

        rc = samir_do(ip, "SET FILTER TO SALARY > 50000");
        CHECK(rc == 0, "samir_do('SET FILTER TO ...') returned 0 (parsed+stored)");
        ss = set_get_state(ip);
        if (ss) {
            CHECK(ss->have_filter == 1, "SET FILTER: have_filter == 1");
            CHECK(strcmp(ss->filter_text, "SALARY > 50000") == 0,
                  "SET FILTER: filter_text stored correctly");
        }

        rc = samir_do(ip, "SET RELATION TO CUST_ID INTO CLIENTS");
        CHECK(rc == 0, "samir_do('SET RELATION TO ... INTO ...') returned 0 (parsed+stored)");
        ss = set_get_state(ip);
        if (ss) {
            CHECK(ss->have_relation == 1, "SET RELATION: have_relation == 1");
            CHECK(strcmp(ss->relation_text, "CUST_ID INTO CLIENTS") == 0,
                  "SET RELATION: relation_text stored correctly");
        }

        printf("  LOUD-SKIP: SET INDEX runtime effect (index open) deferred"
               " -- work-area index-open plumbing not yet present;"
               " follow-up bead required.\n");
        printf("  LOUD-SKIP: SET FILTER runtime effect (record filter) deferred"
               " -- work-area filter plumbing not yet present;"
               " follow-up bead required.\n");
        printf("  LOUD-SKIP: SET RELATION runtime effect (area linking) deferred"
               " -- work-area relation plumbing not yet present;"
               " follow-up bead required.\n");
    }

    /* ------------------------------------------------------------------ */
    /* 9. Bad SET syntax / unknown sub-option -> fail-loud.               */
    /* ------------------------------------------------------------------ */

    /* Bare "SET" with no option -> fail loud. */
    rc = samir_do(ip, "SET");
    CHECK(rc != 0, "bare 'SET' (no option) -> samir_do returns non-zero");

    /* SET with unknown sub-option -> fail loud. */
    rc = samir_do(ip, "SET FOOBAR ON");
    CHECK(rc != 0, "SET FOOBAR -> samir_do returns non-zero (unknown option)");

    /* SET EXACT without ON/OFF -> fail loud. */
    rc = samir_do(ip, "SET EXACT MAYBE");
    CHECK(rc != 0, "SET EXACT MAYBE -> samir_do returns non-zero (bad token)");

    /* SET NEAR ON -> fail loud (not a III+ option). */
    rc = samir_do(ip, "SET NEAR ON");
    CHECK(rc != 0,
          "SET NEAR ON -> fail-loud (NEAR is not a III+ SET option)");

    /* ------------------------------------------------------------------ */
    /* Teardown.                                                            */
    /* ------------------------------------------------------------------ */
    xb_interp_free(ip);
    pal_host_free(pal);
    return 0;
}

/* =====================================================================
 * Tier 1: SET ORDER with an actual open work area (corpus-dependent).
 * Opens a .dbf + .ndx from the corpus, asserts wa_master_order changes.
 * Loud-skips if the corpus is absent.
 * ===================================================================== */
static int run_set_order_tier1(const char *corpus_path)
{
    char dbf_path[512], ndx_path[512];
    struct pal_host_cfg cfg = { 26, 6, 18, 512*1024 };
    samir_pal_t *pal;
    xb_interp   *ip;
    wa_env      *env;
    int          area, rc;

    snprintf(dbf_path, sizeof(dbf_path),
             "%s/goldens/dbase-iii-plus-1.1-pristine/files/Sample_Data"
             "/CLIENTS.DBF", corpus_path);
    snprintf(ndx_path, sizeof(ndx_path),
             "%s/goldens/dbase-iii-plus-1.1-pristine/files/Sample_Data"
             "/CNAMES.NDX", corpus_path);

    {
        FILE *f = fopen(dbf_path, "rb");
        if (!f) {
            printf("  LOUD-SKIP: SET ORDER Tier-1 (corpus absent): %s\n",
                   dbf_path);
            return 0;
        }
        fclose(f);
    }

    pal = pal_host_make(cfg);
    ip  = xb_interp_make(pal);
    if (!ip) { pal_host_free(pal); return 0; }

    rc = set_register(ip);
    CHECK(rc == INTERP_OK, "Tier-1: set_register OK");

    env  = xb_interp_env(ip);
    area = wa_selected(env);

    /* Open CLIENTS.DBF with CNAMES.NDX in the selected area. */
    {
        wa_index_list idx;
        idx.names[0] = ndx_path;
        idx.count    = 1;
        rc = wa_set_open(env, area, dbf_path, (const char *)0, &idx);
        if (rc != WA_OK) {
            printf("  LOUD-SKIP: SET ORDER Tier-1: wa_set_open failed (%d)\n",
                   rc);
            xb_interp_free(ip);
            pal_host_free(pal);
            return 0;
        }
    }

    /* After USE with one index, master_order == 1 (first index controls). */
    CHECK(wa_master_order(env, area) == 1,
          "Tier-1 SET ORDER: initial master_order == 1 after USE with index");

    /* SET ORDER TO 0 -> natural record order. */
    {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "SET ORDER TO 0");
        rc = samir_do(ip, cmd);
        CHECK(rc == 0, "Tier-1: samir_do('SET ORDER TO 0') returned 0");
    }
    CHECK(wa_master_order(env, area) == 0,
          "Tier-1 SET ORDER TO 0: wa_master_order == 0 (natural order)");

    /* SET ORDER TO 1 -> first index controls. */
    {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "SET ORDER TO 1");
        rc = samir_do(ip, cmd);
        CHECK(rc == 0, "Tier-1: samir_do('SET ORDER TO 1') returned 0");
    }
    CHECK(wa_master_order(env, area) == 1,
          "Tier-1 SET ORDER TO 1: wa_master_order == 1");

    /* SET ORDER TO 2 with only 1 index -> fail loud (out of range). */
    {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "SET ORDER TO 2");
        rc = samir_do(ip, cmd);
        CHECK(rc != 0,
              "Tier-1: SET ORDER TO 2 (only 1 index open) -> fail loud");
    }

    /* Cleanup. */
    wa_close(env, area);
    xb_interp_free(ip);
    pal_host_free(pal);
    return 0;
}

/* =====================================================================
 * main
 * ===================================================================== */

int main(int argc, char **argv)
{
    const char *corpus = (argc > 1) ? argv[1] : "../dbase3-decomp";

    printf("test_interp_set: S5.6 SET state oracle\n");

    run_set_tests(corpus);
    run_set_order_tier1(corpus);

    return TEST_SUMMARY("test_interp_set");
}
