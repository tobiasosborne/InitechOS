/*
 * harness/diff/dbf_diff/test_interp_flow.c -- host oracle for S5.3: the statement
 * executor + control flow (DO WHILE/IF/DO CASE/LOOP/EXIT), memvars (STORE / =),
 * and the guard-must-be-Logical rule (error #37, no truthiness).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_interp_use.c /
 * test_interp_nav.c: the seed test_assert.h harness (CHECK / TEST_HARNESS /
 * TEST_SUMMARY), a host PAL via pal_host_make. A non-zero exit on any failed
 * check keeps `make test-interp-flow` from false-greening (Law 2).
 *
 * PURE HOST oracle (plan S5.3: "nesting + guard-type errors") -- control flow is
 * internal logic, so NO goldens are needed. observable effects are asserted via
 * memvars (accumulate a counter, assert its final value). One Tier-1 leg uses a
 * synthetic .dbf to prove field + memvar interplay in one expression.
 *
 * WHAT S5.3 IS (the SPINE that S5.4/S5.5/S5.6/S5.7 extend):
 *   - samir_do(ip, prg): tokenise -> dispatch -> run control flow.
 *   - control flow: DO WHILE/ENDDO, IF/ELSE/ENDIF, DO CASE/CASE/OTHERWISE/ENDCASE,
 *     LOOP, EXIT (with nesting).
 *   - memvars: STORE <expr> TO <name>; <name> = <expr>; resolve via the COMPOSED
 *     resolver (memvars + the work-area field delegate).
 *   - GUARD-MUST-BE-LOGICAL: a non-Logical IF/DO WHILE/CASE guard is fail-loud
 *     error #37 (XBEE_NOT_LOGICAL) -- NO truthiness.
 *   - malformed structure: unmatched ENDIF/ENDDO/ENDCASE -> fail loud.
 *
 * Mutation proof (Rule 6 / ARB rider (a)):
 *   -DFLOW_MUTATE_GUARD_TRUTHY: accept a non-Logical guard as truthiness instead
 *   of erroring #37. The guard-type-error checks (IF "abc", DO WHILE 1, CASE 5)
 *   then PASS the guard -> they no longer report #37 -> the oracle goes RED.
 *   (Alternative -DFLOW_MUTATE_DOCASE_ALL also bites via the DO CASE "first true
 *    CASE only" check.)
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.3 contract + oracle.
 *   - os/samir/include/samir/interp.h (samir_do + the S5.3 API).
 *   - os/samir/include/samir/eval.h   (XBEE_NOT_LOGICAL = #37).
 *   - spec/samir/dbase_msg_codes.tsv  (#37 "Not a Logical expression.").
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"          /* seed/, on -Iseed */
#include "samir/interp.h"         /* os/samir/include/, on -Ios/samir/include */
#include "samir/workarea.h"
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

/* ---- helpers ---- */

/* Fetch a numeric memvar's value into *out; CHECK it exists + is XB_N. */
static int memvar_num(xb_interp *ip, const char *name, double *out)
{
    xb_val v;
    if (xb_interp_get_memvar(ip, name, &v) != 0)
        return 0;
    if (v.t != XB_N)
        return 0;
    *out = v.u.n;
    return 1;
}

/* Run a program on a FRESH interp; return the samir_do rc; *last is the error. */
static int run_prog(samir_pal_t *pal, const char *prg, xb_interp **ip_out)
{
    xb_interp *ip = xb_interp_make(pal);
    int rc;
    if (!ip) { *ip_out = NULL; return -999; }
    rc = samir_do(ip, prg);
    *ip_out = ip;
    return rc;
}

/* =====================================================================
 * Group 1: DO WHILE counter loop runs the right number of iterations.
 * ===================================================================== */
static void test_dowhile_counter(samir_pal_t *pal)
{
    /* I = 0 ; while I < 5: I = I + 1 ; assert I == 5 (5 iterations). */
    const char *prg =
        "STORE 0 TO I\n"
        "DO WHILE I < 5\n"
        "  I = I + 1\n"
        "ENDDO\n";
    xb_interp *ip;
    double v = -1.0;
    char msg[160];
    int rc = run_prog(pal, prg, &ip);
    CHECK(ip != NULL, "dowhile: interp made");
    if (!ip) return;
    snprintf(msg, sizeof(msg), "dowhile: samir_do rc=%d ec=%d (want 0)", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(memvar_num(ip, "I", &v), "dowhile: I is numeric memvar");
    snprintf(msg, sizeof(msg), "dowhile: I==5 after 5 iterations (got %g)", v);
    CHECK(v == 5.0, msg);
    xb_interp_free(ip);
}

/* =====================================================================
 * Group 2: IF/ELSE/ENDIF takes the right branch (both directions).
 * ===================================================================== */
static void test_if_else(samir_pal_t *pal)
{
    xb_interp *ip;
    double v;
    char msg[160];
    int rc;

    /* true branch */
    rc = run_prog(pal,
        "STORE 0 TO R\n"
        "IF 1 = 1\n"
        "  R = 10\n"
        "ELSE\n"
        "  R = 20\n"
        "ENDIF\n", &ip);
    CHECK(ip != NULL && rc == INTERP_OK, "if: true-branch rc");
    if (ip) {
        v = -1; CHECK(memvar_num(ip, "R", &v), "if: R set (true)");
        snprintf(msg, sizeof(msg), "if: true branch -> R==10 (got %g)", v);
        CHECK(v == 10.0, msg);
        xb_interp_free(ip);
    }

    /* false branch -> ELSE */
    rc = run_prog(pal,
        "STORE 0 TO R\n"
        "IF 1 = 2\n"
        "  R = 10\n"
        "ELSE\n"
        "  R = 20\n"
        "ENDIF\n", &ip);
    CHECK(ip != NULL && rc == INTERP_OK, "if: false-branch rc");
    if (ip) {
        v = -1; CHECK(memvar_num(ip, "R", &v), "if: R set (false)");
        snprintf(msg, sizeof(msg), "if: false branch -> R==20 (got %g)", v);
        CHECK(v == 20.0, msg);
        xb_interp_free(ip);
    }

    /* IF with no ELSE, false -> body skipped, R stays 0 */
    rc = run_prog(pal,
        "STORE 0 TO R\n"
        "IF 1 = 2\n"
        "  R = 99\n"
        "ENDIF\n", &ip);
    CHECK(ip != NULL && rc == INTERP_OK, "if: no-else false rc");
    if (ip) {
        v = -1; CHECK(memvar_num(ip, "R", &v), "if: R present (no-else)");
        snprintf(msg, sizeof(msg), "if: no-ELSE false -> R==0 (got %g)", v);
        CHECK(v == 0.0, msg);
        xb_interp_free(ip);
    }
}

/* =====================================================================
 * Group 3: DO CASE picks the FIRST true CASE (else OTHERWISE).
 * The -DFLOW_MUTATE_DOCASE_ALL mutant breaks "first true only".
 * ===================================================================== */
static void test_docase(samir_pal_t *pal)
{
    xb_interp *ip;
    double v;
    char msg[160];
    int rc;

    /* Two CASEs are TRUE; only the FIRST must run. R must be 1, not 2 and not 3. */
    rc = run_prog(pal,
        "STORE 0 TO R\n"
        "DO CASE\n"
        "  CASE 1 = 1\n"
        "    R = 1\n"
        "  CASE 2 = 2\n"
        "    R = 2\n"
        "  OTHERWISE\n"
        "    R = 3\n"
        "ENDCASE\n", &ip);
    CHECK(ip != NULL && rc == INTERP_OK, "docase: first-true rc");
    if (ip) {
        v = -1; CHECK(memvar_num(ip, "R", &v), "docase: R set (first-true)");
        snprintf(msg, sizeof(msg), "docase: first true CASE only -> R==1 (got %g)", v);
        CHECK(v == 1.0, msg);   /* MUTANT FLOW_MUTATE_DOCASE_ALL -> R==2 here */
        xb_interp_free(ip);
    }

    /* No CASE true -> OTHERWISE runs. */
    rc = run_prog(pal,
        "STORE 0 TO R\n"
        "DO CASE\n"
        "  CASE 1 = 2\n"
        "    R = 1\n"
        "  CASE 2 = 3\n"
        "    R = 2\n"
        "  OTHERWISE\n"
        "    R = 9\n"
        "ENDCASE\n", &ip);
    CHECK(ip != NULL && rc == INTERP_OK, "docase: otherwise rc");
    if (ip) {
        v = -1; CHECK(memvar_num(ip, "R", &v), "docase: R set (otherwise)");
        snprintf(msg, sizeof(msg), "docase: no CASE true -> OTHERWISE R==9 (got %g)", v);
        CHECK(v == 9.0, msg);
        xb_interp_free(ip);
    }

    /* No CASE true, no OTHERWISE -> nothing runs, R stays 0. */
    rc = run_prog(pal,
        "STORE 0 TO R\n"
        "DO CASE\n"
        "  CASE 1 = 2\n"
        "    R = 1\n"
        "ENDCASE\n", &ip);
    CHECK(ip != NULL && rc == INTERP_OK, "docase: none rc");
    if (ip) {
        v = -1; CHECK(memvar_num(ip, "R", &v), "docase: R present (none)");
        snprintf(msg, sizeof(msg), "docase: no CASE no OTHERWISE -> R==0 (got %g)", v);
        CHECK(v == 0.0, msg);
        xb_interp_free(ip);
    }
}

/* =====================================================================
 * Group 4: nested DO WHILE + IF (a multiplication-by-addition table sum).
 * ===================================================================== */
static void test_nested(samir_pal_t *pal)
{
    /* Sum of even numbers 1..10 = 2+4+6+8+10 = 30, via a DO WHILE with an IF
     * gating the accumulation (uses MOD via "I/2*2 = I" since % is not III+). */
    const char *prg =
        "STORE 0 TO I\n"
        "STORE 0 TO S\n"
        "DO WHILE I < 10\n"
        "  I = I + 1\n"
        "  IF INT(I/2)*2 = I\n"
        "    S = S + I\n"
        "  ENDIF\n"
        "ENDDO\n";
    xb_interp *ip;
    double s = -1.0, ii = -1.0;
    char msg[160];
    int rc = run_prog(pal, prg, &ip);
    CHECK(ip != NULL, "nested: interp made");
    if (!ip) return;
    snprintf(msg, sizeof(msg), "nested: samir_do rc=%d ec=%d", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(memvar_num(ip, "I", &ii) && ii == 10.0, "nested: I reached 10");
    CHECK(memvar_num(ip, "S", &s), "nested: S numeric");
    snprintf(msg, sizeof(msg), "nested: sum of evens 1..10 == 30 (got %g)", s);
    CHECK(s == 30.0, msg);
    xb_interp_free(ip);
}

/* =====================================================================
 * Group 5: LOOP skips to next iteration; EXIT breaks the loop.
 * ===================================================================== */
static void test_loop_exit(samir_pal_t *pal)
{
    xb_interp *ip;
    double v, c;
    char msg[160];
    int rc;

    /* LOOP: count only ODD I in 1..6 by skipping evens; C must be 3 (1,3,5). */
    rc = run_prog(pal,
        "STORE 0 TO I\n"
        "STORE 0 TO C\n"
        "DO WHILE I < 6\n"
        "  I = I + 1\n"
        "  IF INT(I/2)*2 = I\n"
        "    LOOP\n"
        "  ENDIF\n"
        "  C = C + 1\n"
        "ENDDO\n", &ip);
    CHECK(ip != NULL && rc == INTERP_OK, "loop: rc");
    if (ip) {
        c = -1; CHECK(memvar_num(ip, "C", &c), "loop: C numeric");
        snprintf(msg, sizeof(msg), "loop: LOOP skips evens -> C==3 (got %g)", c);
        CHECK(c == 3.0, msg);
        v = -1; CHECK(memvar_num(ip, "I", &v) && v == 6.0, "loop: I reached 6");
        xb_interp_free(ip);
    }

    /* EXIT: break at I==3; I must be 3, not 10. */
    rc = run_prog(pal,
        "STORE 0 TO I\n"
        "DO WHILE I < 10\n"
        "  I = I + 1\n"
        "  IF I = 3\n"
        "    EXIT\n"
        "  ENDIF\n"
        "ENDDO\n", &ip);
    CHECK(ip != NULL && rc == INTERP_OK, "exit: rc");
    if (ip) {
        v = -1; CHECK(memvar_num(ip, "I", &v), "exit: I numeric");
        snprintf(msg, sizeof(msg), "exit: EXIT breaks at I==3 (got %g)", v);
        CHECK(v == 3.0, msg);
        xb_interp_free(ip);
    }
}

/* =====================================================================
 * Group 6: memvars -- STORE then = then arithmetic.
 * ===================================================================== */
static void test_memvars(samir_pal_t *pal)
{
    /* STORE 5 TO X ; X = X + 1 ; Y = X * 2 ; assert X==6, Y==12. */
    const char *prg =
        "STORE 5 TO X\n"
        "X = X + 1\n"
        "Y = X * 2\n";
    xb_interp *ip;
    double x = -1.0, y = -1.0;
    char msg[160];
    int rc = run_prog(pal, prg, &ip);
    CHECK(ip != NULL, "memvar: interp made");
    if (!ip) return;
    snprintf(msg, sizeof(msg), "memvar: samir_do rc=%d", rc);
    CHECK(rc == INTERP_OK, msg);
    CHECK(memvar_num(ip, "X", &x), "memvar: X present");
    snprintf(msg, sizeof(msg), "memvar: X==6 (got %g)", x);
    CHECK(x == 6.0, msg);
    CHECK(memvar_num(ip, "Y", &y), "memvar: Y present");
    snprintf(msg, sizeof(msg), "memvar: Y==12 (got %g)", y);
    CHECK(y == 12.0, msg);

    /* case-insensitive: "x" finds X. */
    {
        xb_val cv;
        CHECK(xb_interp_get_memvar(ip, "x", &cv) == 0 && cv.t == XB_N && cv.u.n == 6.0,
              "memvar: case-insensitive lookup x==X==6");
    }

    /* a Character memvar survives (bytes copied into the memvar arena). */
    xb_interp_free(ip);

    rc = run_prog(pal, "STORE 'HELLO' TO MSG\n", &ip);
    CHECK(ip != NULL && rc == INTERP_OK, "memvar: char store rc");
    if (ip) {
        xb_val cv;
        CHECK(xb_interp_get_memvar(ip, "MSG", &cv) == 0 && cv.t == XB_C &&
              cv.u.c.len == 5u && memcmp(cv.u.c.p, "HELLO", 5) == 0,
              "memvar: MSG=='HELLO' (C bytes stable)");
        xb_interp_free(ip);
    }
}

/* =====================================================================
 * Group 7: GUARD-TYPE error -- a non-Logical guard fails loud #37.
 * THIS is what -DFLOW_MUTATE_GUARD_TRUTHY breaks (it would accept truthiness).
 * ===================================================================== */
static void test_guard_type(samir_pal_t *pal)
{
    xb_interp *ip;
    char msg[160];
    int rc;

    /* IF "abc" -> #37 (a Character guard is NOT truthy). */
    rc = run_prog(pal,
        "IF \"abc\"\n"
        "  X = 1\n"
        "ENDIF\n", &ip);
    CHECK(ip != NULL, "guard: IF-char interp");
    if (ip) {
        snprintf(msg, sizeof(msg), "guard: IF \"abc\" fails loud (rc=%d)", rc);
        CHECK(rc != INTERP_OK, msg);
        snprintf(msg, sizeof(msg), "guard: IF \"abc\" -> #37 (got %d)", samir_last_error(ip));
        CHECK(samir_last_error(ip) == XBEE_NOT_LOGICAL, msg);
        xb_interp_free(ip);
    }

    /* DO WHILE 1 -> numeric guard -> #37 (NOT "while nonzero"). */
    rc = run_prog(pal,
        "DO WHILE 1\n"
        "  X = 1\n"
        "ENDDO\n", &ip);
    CHECK(ip != NULL, "guard: DO WHILE-num interp");
    if (ip) {
        snprintf(msg, sizeof(msg), "guard: DO WHILE 1 fails loud (rc=%d)", rc);
        CHECK(rc != INTERP_OK, msg);
        snprintf(msg, sizeof(msg), "guard: DO WHILE 1 -> #37 (got %d)", samir_last_error(ip));
        CHECK(samir_last_error(ip) == XBEE_NOT_LOGICAL, msg);
        xb_interp_free(ip);
    }

    /* DO CASE with a non-Logical CASE expr -> #37. */
    rc = run_prog(pal,
        "DO CASE\n"
        "  CASE 5\n"
        "    X = 1\n"
        "ENDCASE\n", &ip);
    CHECK(ip != NULL, "guard: CASE-num interp");
    if (ip) {
        snprintf(msg, sizeof(msg), "guard: DO CASE/CASE 5 fails loud (rc=%d)", rc);
        CHECK(rc != INTERP_OK, msg);
        snprintf(msg, sizeof(msg), "guard: CASE 5 -> #37 (got %d)", samir_last_error(ip));
        CHECK(samir_last_error(ip) == XBEE_NOT_LOGICAL, msg);
        xb_interp_free(ip);
    }

    /* Sanity: a PROPER Logical guard does NOT error (so the mutant cannot pass
     * merely by never erroring). IF .T. runs the body. */
    rc = run_prog(pal,
        "STORE 0 TO X\n"
        "IF .T.\n"
        "  X = 7\n"
        "ENDIF\n", &ip);
    CHECK(ip != NULL && rc == INTERP_OK, "guard: IF .T. is fine rc");
    if (ip) {
        double v = -1;
        CHECK(memvar_num(ip, "X", &v) && v == 7.0, "guard: IF .T. ran body (X==7)");
        xb_interp_free(ip);
    }
}

/* =====================================================================
 * Group 8: malformed structure -- unmatched terminators fail loud.
 * ===================================================================== */
static void test_malformed(samir_pal_t *pal)
{
    xb_interp *ip;
    char msg[160];
    int rc;

    /* unmatched ENDIF (no IF). */
    rc = run_prog(pal, "ENDIF\n", &ip);
    CHECK(ip != NULL, "malformed: lone ENDIF interp");
    if (ip) {
        snprintf(msg, sizeof(msg), "malformed: lone ENDIF -> STRUCT (rc=%d ec=%d)",
                 rc, samir_last_error(ip));
        CHECK(rc != INTERP_OK && samir_last_error(ip) == INTERP_ERR_STRUCT, msg);
        xb_interp_free(ip);
    }

    /* IF with no ENDIF (unterminated). */
    rc = run_prog(pal, "IF 1 = 1\n  X = 1\n", &ip);
    CHECK(ip != NULL, "malformed: IF no ENDIF interp");
    if (ip) {
        snprintf(msg, sizeof(msg), "malformed: IF no ENDIF -> STRUCT (rc=%d ec=%d)",
                 rc, samir_last_error(ip));
        CHECK(rc != INTERP_OK && samir_last_error(ip) == INTERP_ERR_STRUCT, msg);
        xb_interp_free(ip);
    }

    /* lone ENDDO. */
    rc = run_prog(pal, "ENDDO\n", &ip);
    CHECK(ip != NULL, "malformed: lone ENDDO interp");
    if (ip) {
        snprintf(msg, sizeof(msg), "malformed: lone ENDDO -> STRUCT (rc=%d ec=%d)",
                 rc, samir_last_error(ip));
        CHECK(rc != INTERP_OK && samir_last_error(ip) == INTERP_ERR_STRUCT, msg);
        xb_interp_free(ip);
    }

    /* DO CASE with missing ENDCASE. */
    rc = run_prog(pal, "DO CASE\n  CASE 1 = 1\n    X = 1\n", &ip);
    CHECK(ip != NULL, "malformed: DO CASE no ENDCASE interp");
    if (ip) {
        snprintf(msg, sizeof(msg), "malformed: DO CASE no ENDCASE -> STRUCT (rc=%d ec=%d)",
                 rc, samir_last_error(ip));
        CHECK(rc != INTERP_OK && samir_last_error(ip) == INTERP_ERR_STRUCT, msg);
        xb_interp_free(ip);
    }

    /* EXIT outside a loop -> structure error. */
    rc = run_prog(pal, "EXIT\n", &ip);
    CHECK(ip != NULL, "malformed: EXIT outside loop interp");
    if (ip) {
        snprintf(msg, sizeof(msg), "malformed: EXIT outside loop -> STRUCT (rc=%d ec=%d)",
                 rc, samir_last_error(ip));
        CHECK(rc != INTERP_OK && samir_last_error(ip) == INTERP_ERR_STRUCT, msg);
        xb_interp_free(ip);
    }

    /* unrecognised verb -> fail loud #16. */
    rc = run_prog(pal, "FROBNICATE THE WIDGET\n", &ip);
    CHECK(ip != NULL, "malformed: bad verb interp");
    if (ip) {
        snprintf(msg, sizeof(msg), "malformed: unknown verb -> UNKNOWN_CMD (rc=%d ec=%d)",
                 rc, samir_last_error(ip));
        CHECK(rc != INTERP_OK && samir_last_error(ip) == 16, msg);
        xb_interp_free(ip);
    }
}

/* =====================================================================
 * Group 9: field + memvar interplay in one expression (synthetic .dbf).
 *
 * USE a 1-field N(4) table whose record 1 holds 100, then in a program
 * reference both the FIELD (QTY) and a memvar (BONUS) in one expression.
 * The composed resolver must bind both.
 * ===================================================================== */
static int write_num_dbf(const char *path)
{
    /* 1-field N(4,0) table, 1 record holding "0100". */
    FILE *f;
    uint8_t hdr[32], desc[32], tail[6];

    memset(hdr, 0, sizeof(hdr));
    memset(desc, 0, sizeof(desc));
    memset(tail, 0, sizeof(tail));

    hdr[0x00] = 0x03u;
    hdr[0x04] = 1;                            /* nrec=1 LE */
    hdr[0x08] = 65; hdr[0x09] = 0;            /* header_length = 65 */
    hdr[0x0A] = 5;  hdr[0x0B] = 0;            /* record_length = 1 + 4 */

    desc[0x00] = 'Q'; desc[0x01] = 'T'; desc[0x02] = 'Y'; desc[0x03] = 0;
    desc[0x0B] = 'N';                         /* type N */
    desc[0x10] = 4;                           /* field_len = 4 */
    desc[0x11] = 0;                           /* dec_count = 0 */
    desc[0x14] = 0x01u;

    tail[0] = 0x0Du;
    tail[1] = 0x20u;                          /* live */
    tail[2] = '0'; tail[3] = '1'; tail[4] = '0'; tail[5] = '0';  /* "0100" = 100 */

    f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(hdr, 1, 32, f);
    fwrite(desc, 1, 32, f);
    fwrite(tail, 1, 6, f);
    fclose(f);
    return 0;
}

static void test_field_memvar(samir_pal_t *pal)
{
    const char *pa = "/tmp/test_interp_flow_qty.dbf";
    xb_interp *ip;
    wa_env *env;
    double total = -1.0;
    char msg[160];
    int rc;

    CHECK(write_num_dbf(pa) == 0, "interplay: write QTY table");

    ip = xb_interp_make(pal);
    CHECK(ip != NULL, "interplay: interp made");
    if (!ip) { remove(pa); return; }
    env = xb_interp_env(ip);

    rc = wa_set_open(env, 1, pa, NULL, NULL);
    snprintf(msg, sizeof(msg), "interplay: USE QTY rc=%d", rc);
    CHECK(rc == WA_OK, msg);
    if (rc != WA_OK) { xb_interp_free(ip); remove(pa); return; }
    wa_select(env, 1);

    /* program: BONUS memvar = 25 ; TOTAL = QTY + BONUS ; expect 125. */
    rc = samir_do(ip,
        "STORE 25 TO BONUS\n"
        "TOTAL = QTY + BONUS\n");
    snprintf(msg, sizeof(msg), "interplay: samir_do rc=%d ec=%d", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(memvar_num(ip, "TOTAL", &total), "interplay: TOTAL present");
    snprintf(msg, sizeof(msg), "interplay: QTY(100)+BONUS(25)==125 (got %g)", total);
    CHECK(total == 125.0, msg);

    /* a guard mixing field + memvar: IF QTY > BONUS -> true (100 > 25). */
    rc = samir_do(ip,
        "STORE 0 TO FLAG\n"
        "IF QTY > BONUS\n"
        "  FLAG = 1\n"
        "ENDIF\n");
    CHECK(rc == INTERP_OK, "interplay: field/memvar guard rc");
    {
        double flag = -1;
        CHECK(memvar_num(ip, "FLAG", &flag) && flag == 1.0,
              "interplay: IF QTY>BONUS took true branch (FLAG==1)");
    }

    xb_interp_free(ip);
    remove(pa);
}

/* =====================================================================
 * main
 * ===================================================================== */
int main(int argc, char **argv)
{
    struct pal_host_cfg cfg;
    samir_pal_t *pal;
    (void)argc; (void)argv;   /* pure host oracle: no corpus needed */

    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy   = 99;       /* injected fixed date (Rule 11) */
    cfg.date_mm   = 12;
    cfg.date_dd   = 31;
    cfg.heap_size = 4u * 1024u * 1024u;  /* generous: many fresh interps */

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }

    test_dowhile_counter(pal);
    test_if_else(pal);
    test_docase(pal);
    test_nested(pal);
    test_loop_exit(pal);
    test_memvars(pal);
    test_guard_type(pal);
    test_malformed(pal);
    test_field_memvar(pal);

    pal_host_free(pal);
    return TEST_SUMMARY("test-interp-flow");
}
