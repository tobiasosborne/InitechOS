/*
 * harness/diff/dbf_diff/test_interp_proc.c -- host oracle for S5.7: procedures +
 * scope + keyboard I/O + ON ERROR (initech-7az.8).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Mirrors test_interp_flow.c /
 * test_interp_set.c: the seed test_assert.h harness (CHECK / TEST_HARNESS /
 * TEST_SUMMARY), a host PAL via pal_host_make. A non-zero exit on any failed
 * check keeps `make test-interp-proc` from false-greening (Law 2).
 *
 * SCRIPTED CONSOLE. ACCEPT/INPUT read a line; WAIT reads one key. The engine
 * touches the console ONLY through the PAL (pal->conin_line / conin_char). For a
 * DETERMINISTIC (Rule 11) host run we build the PAL with pal_host_make (for the
 * arena/file/clock) and then OVERRIDE its conin_line / conin_char slots with
 * scripted readers that pull from a file-static script buffer. No real stdin.
 *
 * WHAT S5.7 IS (the procedure/scope/IO module proc.c adds to the chain):
 *   DO <name> [WITH ...] / PROCEDURE / PARAMETERS / RETURN / PUBLIC / PRIVATE /
 *   ACCEPT / INPUT / WAIT / ON ERROR. proc_run(ip, prg) runs a program whose
 *   PROCEDURE blocks DO resolves by name; the scope model (flow.c) stacks
 *   PRIVATE/PARAMETERS at the DO-call level and restores shadows on RETURN.
 *
 * Mutation proof (Rule 6 / ARB rider (a)):
 *   -DPROC_MUTATE_PRIVATE_NORESTORE: a PRIVATE callee does NOT restore the
 *     shadowed caller var on RETURN -> the "outer X unchanged after the call"
 *     assertion goes RED.
 *   -DPROC_MUTATE_PARAM_ORDER: PARAMETERS binds the WITH args in REVERSE -> the
 *     "param N == the N-th WITH value" assertion goes RED.
 *
 * GATED (loud-skip + cite plan Sec 7 -- NOT asserted):
 *   - PARAMETERS by-REFERENCE write-back exactness (we pass by value).
 *   - the uninitialized-PUBLIC VALUE (we init to XB_U / undefined).
 *   - DO-name precedence (open PROCEDURE vs disk <name>.prg).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.7 contract + Sec 7 GATED register.
 *   - os/samir/include/samir/interp.h (samir_do + the S5.7 scope API).
 *   - os/samir/cmd/proc.c (proc_register / proc_run / proc_fire_onerror).
 *   - ../dbase3-decomp/specs/language/memory-variables.md sec 3 (scope).
 *   - ../dbase3-decomp/specs/commands/control-flow-and-procedures.md sec 7-8.
 *   - ../dbase3-decomp/specs/commands/programming-and-io.md sec 8-10,14.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"          /* seed/, on -Iseed */
#include "samir/interp.h"         /* os/samir/include/ */
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"
#include "samir/pal.h"

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

/* proc.c surface (the S5.7 module entry points; declared here). */
int proc_register(xb_interp *ip);
int proc_run(xb_interp *ip, const char *prg);

/* ===================================================================== */
/* Scripted console: a queue of input lines + a queue of single keys.      */
/* The overridden conin_line pops the next line; conin_char pops the next   */
/* key. Both are file-static so the override functions need no PAL state.   */
/* ===================================================================== */

#define SCRIPT_MAX_LINES 32
static const char *g_lines[SCRIPT_MAX_LINES];
static int         g_nlines;
static int         g_lineidx;

static const char *g_keys;        /* a NUL-terminated string of keystrokes */
static int         g_keyidx;

static void script_reset(void)
{
    g_nlines = 0; g_lineidx = 0;
    g_keys = ""; g_keyidx = 0;
}

static void script_push_line(const char *s)
{
    if (g_nlines < SCRIPT_MAX_LINES)
        g_lines[g_nlines++] = s;
}

static void script_set_keys(const char *keys)
{
    g_keys = keys ? keys : "";
    g_keyidx = 0;
}

/* the overriding conin_line: copy the next scripted line into buf (no newline). */
static int32_t scripted_conin_line(samir_pal_t *p, char *buf, uint32_t cap)
{
    const char *s;
    uint32_t n;
    (void)p;
    if (g_lineidx >= g_nlines) { if (cap) buf[0] = '\0'; return -1; }  /* EOF */
    s = g_lines[g_lineidx++];
    n = 0;
    while (s[n] != '\0' && n < cap - 1u) { buf[n] = s[n]; n++; }
    buf[n] = '\0';
    return (int32_t)n;
}

/* the overriding conin_char: return the next scripted key, or -1 at end. */
static int32_t scripted_conin_char(samir_pal_t *p)
{
    (void)p;
    if (g_keys[g_keyidx] == '\0') return -1;
    return (int32_t)(unsigned char)g_keys[g_keyidx++];
}

/* install the scripted readers over a host PAL (pal.h struct is fully defined). */
static void install_scripted(samir_pal_t *pal)
{
    pal->conin_line = scripted_conin_line;
    pal->conin_char = scripted_conin_char;
}

/* ===================================================================== */
/* helpers                                                                */
/* ===================================================================== */

static int memvar_num(xb_interp *ip, const char *name, double *out)
{
    xb_val v;
    if (xb_interp_get_memvar(ip, name, &v) != 0) return 0;
    if (v.t != XB_N) return 0;
    *out = v.u.n;
    return 1;
}

/* make a fresh interp with proc.c registered. */
static xb_interp *make_ip(samir_pal_t *pal)
{
    xb_interp *ip = xb_interp_make(pal);
    if (ip) proc_register(ip);
    return ip;
}

/* =====================================================================
 * Group 1: DO calls a PROCEDURE; RETURN returns; the body ran.
 * ===================================================================== */
static void test_do_call_return(samir_pal_t *pal)
{
    /* main sets FLAG=0, DOes SETIT (sets FLAG=1 then RETURN), asserts FLAG==1.
     * FLAG is visible from the callee (created at main level), so the callee's
     * assignment modifies the caller's var (auto-private rule 1). */
    const char *prg =
        "STORE 0 TO FLAG\n"
        "DO SETIT\n"
        "PROCEDURE SETIT\n"
        "  FLAG = 1\n"
        "  RETURN\n";
    xb_interp *ip = make_ip(pal);
    double v = -1.0;
    char msg[160];
    int rc;
    CHECK(ip != NULL, "do/return: interp made");
    if (!ip) return;
    rc = proc_run(ip, prg);
    snprintf(msg, sizeof(msg), "do/return: proc_run rc=%d ec=%d (want 0)", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(memvar_num(ip, "FLAG", &v), "do/return: FLAG numeric");
    snprintf(msg, sizeof(msg), "do/return: callee ran -> FLAG==1 (got %g)", v);
    CHECK(v == 1.0, msg);
    xb_interp_free(ip);
}

/* =====================================================================
 * Group 2: DO WITH + PARAMETERS binds BY POSITION.
 *   -DPROC_MUTATE_PARAM_ORDER reverses the bind -> RED.
 * ===================================================================== */
static void test_params_positional(samir_pal_t *pal)
{
    /* DO ADD3 WITH 10, 3 ; PROCEDURE ADD3 PARAMETERS A,B ; RES = A*100 + B.
     * positional: A=10,B=3 -> RES=1003. reversed (mutant): A=3,B=10 -> 310. */
    const char *prg =
        "PUBLIC RES\n"
        "DO ADD3 WITH 10, 3\n"
        "PROCEDURE ADD3\n"
        "  PARAMETERS A, B\n"
        "  RES = A * 100 + B\n"
        "  RETURN\n";
    xb_interp *ip = make_ip(pal);
    double v = -1.0;
    char msg[160];
    int rc;
    CHECK(ip != NULL, "params: interp made");
    if (!ip) return;
    rc = proc_run(ip, prg);
    snprintf(msg, sizeof(msg), "params: proc_run rc=%d ec=%d", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(memvar_num(ip, "RES", &v), "params: RES numeric (PUBLIC, survives RETURN)");
    snprintf(msg, sizeof(msg), "params: A=10,B=3 positional -> RES==1003 (got %g)", v);
    CHECK(v == 1003.0, msg);  /* MUTANT PROC_MUTATE_PARAM_ORDER -> 310 here */
    xb_interp_free(ip);
}

/* =====================================================================
 * Group 3: nested DO recurses (samir_do re-entrant).
 * ===================================================================== */
static void test_nested_do(samir_pal_t *pal)
{
    /* main DOes A; A bumps PUBLIC N then DOes B; B bumps N again -> N==2. */
    const char *prg =
        "PUBLIC N\n"
        "N = 0\n"
        "DO A\n"
        "PROCEDURE A\n"
        "  N = N + 1\n"
        "  DO B\n"
        "  RETURN\n"
        "PROCEDURE B\n"
        "  N = N + 1\n"
        "  RETURN\n";
    xb_interp *ip = make_ip(pal);
    double v = -1.0;
    char msg[160];
    int rc;
    CHECK(ip != NULL, "nested: interp made");
    if (!ip) return;
    rc = proc_run(ip, prg);
    snprintf(msg, sizeof(msg), "nested: proc_run rc=%d ec=%d", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(memvar_num(ip, "N", &v), "nested: N numeric");
    snprintf(msg, sizeof(msg), "nested: A->B both bumped N -> N==2 (got %g)", v);
    CHECK(v == 2.0, msg);
    xb_interp_free(ip);
}

/* =====================================================================
 * Group 4: PRIVATE shadows an outer var and is RESTORED on RETURN.
 *   -DPROC_MUTATE_PRIVATE_NORESTORE breaks the restore -> RED.
 * ===================================================================== */
static void test_private_shadow(samir_pal_t *pal)
{
    /* main: X=7 ; DO SHADOWER ; assert X still 7 (the callee's PRIVATE X is
     * dropped on RETURN, unhiding main's X==7). Inside, the callee set X=99 on
     * its OWN private copy -- it must NOT leak out. */
    const char *prg =
        "STORE 7 TO X\n"
        "DO SHADOWER\n"
        "PROCEDURE SHADOWER\n"
        "  PRIVATE X\n"
        "  X = 99\n"
        "  RETURN\n";
    xb_interp *ip = make_ip(pal);
    double v = -1.0;
    char msg[160];
    int rc;
    CHECK(ip != NULL, "private: interp made");
    if (!ip) return;
    rc = proc_run(ip, prg);
    snprintf(msg, sizeof(msg), "private: proc_run rc=%d ec=%d", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(memvar_num(ip, "X", &v), "private: outer X still present");
    snprintf(msg, sizeof(msg), "private: outer X restored to 7 after RETURN (got %g)", v);
    CHECK(v == 7.0, msg);  /* MUTANT PROC_MUTATE_PRIVATE_NORESTORE -> X gone/99 */
    xb_interp_free(ip);
}

/* =====================================================================
 * Group 5: auto-private -- a NEW name in a callee does NOT leak to the caller.
 * ===================================================================== */
static void test_auto_private(samir_pal_t *pal)
{
    /* main DOes MK (which creates TEMP=5, a NEW name -> auto-private to MK).
     * After RETURN, TEMP must NOT be visible at the main level. */
    const char *prg =
        "DO MK\n"
        "PROCEDURE MK\n"
        "  TEMP = 5\n"
        "  RETURN\n";
    xb_interp *ip = make_ip(pal);
    xb_val v;
    char msg[160];
    int rc;
    CHECK(ip != NULL, "autopriv: interp made");
    if (!ip) return;
    rc = proc_run(ip, prg);
    snprintf(msg, sizeof(msg), "autopriv: proc_run rc=%d ec=%d", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(xb_interp_get_memvar(ip, "TEMP", &v) != 0,
          "autopriv: callee's auto-private TEMP NOT visible after RETURN");
    xb_interp_free(ip);
}

/* =====================================================================
 * Group 6: PUBLIC is visible across procedures and survives RETURN.
 * ===================================================================== */
static void test_public_visible(samir_pal_t *pal)
{
    /* PUBLIC G ; G=11 ; DO BUMP (G=G+1, visible across) ; assert G==12. */
    const char *prg =
        "PUBLIC G\n"
        "G = 11\n"
        "DO BUMP\n"
        "PROCEDURE BUMP\n"
        "  G = G + 1\n"
        "  RETURN\n";
    xb_interp *ip = make_ip(pal);
    double v = -1.0;
    char msg[160];
    int rc;
    CHECK(ip != NULL, "public: interp made");
    if (!ip) return;
    rc = proc_run(ip, prg);
    snprintf(msg, sizeof(msg), "public: proc_run rc=%d ec=%d", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(memvar_num(ip, "G", &v), "public: G numeric");
    snprintf(msg, sizeof(msg), "public: PUBLIC G visible in BUMP -> G==12 (got %g)", v);
    CHECK(v == 12.0, msg);
    xb_interp_free(ip);
}

/* =====================================================================
 * Group 7: ACCEPT stores a Character; INPUT evaluates + types the value.
 * ===================================================================== */
static void test_accept_input(samir_pal_t *pal)
{
    xb_interp *ip;
    xb_val cv;
    double n = -1.0;
    char msg[160];
    int rc;

    /* ACCEPT: the scripted line is stored RAW as Character (even "42"). */
    ip = make_ip(pal);
    CHECK(ip != NULL, "accept: interp made");
    if (ip) {
        script_reset();
        script_push_line("42");
        rc = proc_run(ip, "ACCEPT \"name? \" TO NM\n");
        snprintf(msg, sizeof(msg), "accept: proc_run rc=%d ec=%d", rc, samir_last_error(ip));
        CHECK(rc == INTERP_OK, msg);
        CHECK(xb_interp_get_memvar(ip, "NM", &cv) == 0 && cv.t == XB_C &&
              cv.u.c.len == 2u && memcmp(cv.u.c.p, "42", 2) == 0,
              "accept: \"42\" stored as Character \"42\" (NOT numeric)");
        xb_interp_free(ip);
    }

    /* INPUT: the scripted line "12.5" is EVALUATED -> Numeric 12.5. */
    ip = make_ip(pal);
    CHECK(ip != NULL, "input-num: interp made");
    if (ip) {
        script_reset();
        script_push_line("12.5");
        rc = proc_run(ip, "INPUT \"amt? \" TO AMT\n");
        snprintf(msg, sizeof(msg), "input-num: proc_run rc=%d ec=%d", rc, samir_last_error(ip));
        CHECK(rc == INTERP_OK, msg);
        CHECK(memvar_num(ip, "AMT", &n), "input-num: AMT is Numeric");
        snprintf(msg, sizeof(msg), "input-num: INPUT 12.5 -> AMT==12.5 (got %g)", n);
        CHECK(n == 12.5, msg);
        xb_interp_free(ip);
    }

    /* INPUT: the scripted line 'HI' (quoted) is EVALUATED -> Character "HI". */
    ip = make_ip(pal);
    CHECK(ip != NULL, "input-c: interp made");
    if (ip) {
        script_reset();
        script_push_line("'HI'");
        rc = proc_run(ip, "INPUT TO S\n");
        snprintf(msg, sizeof(msg), "input-c: proc_run rc=%d ec=%d", rc, samir_last_error(ip));
        CHECK(rc == INTERP_OK, msg);
        CHECK(xb_interp_get_memvar(ip, "S", &cv) == 0 && cv.t == XB_C &&
              cv.u.c.len == 2u && memcmp(cv.u.c.p, "HI", 2) == 0,
              "input-c: INPUT 'HI' -> Character \"HI\"");
        xb_interp_free(ip);
    }

    /* INPUT: the scripted line ".T." is EVALUATED -> Logical true. */
    ip = make_ip(pal);
    CHECK(ip != NULL, "input-l: interp made");
    if (ip) {
        xb_val lv;
        script_reset();
        script_push_line(".T.");
        rc = proc_run(ip, "INPUT TO OK\n");
        CHECK(rc == INTERP_OK, "input-l: proc_run rc");
        CHECK(xb_interp_get_memvar(ip, "OK", &lv) == 0 && lv.t == XB_L && lv.u.l == 1,
              "input-l: INPUT .T. -> Logical true");
        xb_interp_free(ip);
    }
}

/* =====================================================================
 * Group 8: WAIT TO <k> stores the single scripted key.
 * ===================================================================== */
static void test_wait(samir_pal_t *pal)
{
    xb_interp *ip = make_ip(pal);
    xb_val cv;
    char msg[160];
    int rc;
    CHECK(ip != NULL, "wait: interp made");
    if (!ip) return;
    script_reset();
    script_set_keys("Y");                 /* one keystroke: 'Y' */
    rc = proc_run(ip, "WAIT \"press: \" TO K\n");
    snprintf(msg, sizeof(msg), "wait: proc_run rc=%d ec=%d", rc, samir_last_error(ip));
    CHECK(rc == INTERP_OK, msg);
    CHECK(xb_interp_get_memvar(ip, "K", &cv) == 0 && cv.t == XB_C &&
          cv.u.c.len == 1u && cv.u.c.p[0] == 'Y',
          "wait: single key 'Y' stored as 1-char Character");
    xb_interp_free(ip);
}

/* =====================================================================
 * Group 9: ON ERROR fires on a runtime error (the handler ran).
 * ===================================================================== */
static void test_on_error(samir_pal_t *pal)
{
    /* PUBLIC ERRSEEN ; ON ERROR DO TRAP ; force a #37 (IF 1 -- non-Logical guard)
     * in the main body. The trap sets ERRSEEN=1. The handler must have run. */
    const char *prg =
        "PUBLIC ERRSEEN\n"
        "ERRSEEN = 0\n"
        "ON ERROR DO TRAP\n"
        "IF 1\n"
        "  ERRSEEN = 5\n"
        "ENDIF\n"
        "PROCEDURE TRAP\n"
        "  ERRSEEN = 1\n"
        "  RETURN\n";
    xb_interp *ip = make_ip(pal);
    double v = -1.0;
    char msg[160];
    int rc;
    CHECK(ip != NULL, "onerror: interp made");
    if (!ip) return;
    rc = proc_run(ip, prg);
    /* the main body errors (rc != OK) but the trap fires, setting ERRSEEN=1. */
    snprintf(msg, sizeof(msg), "onerror: main body raised an error (rc=%d ec=%d)",
             rc, samir_last_error(ip));
    CHECK(rc != INTERP_OK, msg);
    CHECK(memvar_num(ip, "ERRSEEN", &v), "onerror: ERRSEEN numeric");
    snprintf(msg, sizeof(msg), "onerror: handler fired -> ERRSEEN==1 (got %g)", v);
    CHECK(v == 1.0, msg);
    xb_interp_free(ip);
}

/* =====================================================================
 * Group 10: GATED edges -- loud-skip, do NOT assert (plan Sec 7).
 * ===================================================================== */
static void test_gated_skips(samir_pal_t *pal)
{
    (void)pal;
    fprintf(stdout,
        "SKIP [GATED plan Sec 7 / corpus Open-q]: PARAMETERS by-REFERENCE "
        "write-back exactness -- proc.c passes WITH args BY VALUE; a bare-name "
        "by-ref alias is corpus-open (memory-variables.md Open-q 5). NOT asserted.\n");
    fprintf(stdout,
        "SKIP [GATED plan Sec 7 / corpus Open-q]: uninitialized PUBLIC/PRIVATE "
        "VALUE (Clipper inits .F.; III+ 1.1 parity unconfirmed) -- proc.c inits "
        "to XB_U (memory-variables.md Open-q 3). Its VALUE is NOT asserted.\n");
    fprintf(stdout,
        "SKIP [GATED plan Sec 7 / corpus Open-q]: DO-name precedence (an open "
        "PROCEDURE vs a disk <name>.prg) -- proc.c resolves only against the "
        "registered source (control-flow-and-procedures.md Open-q 1). NOT asserted.\n");
}

/* ===================================================================== */
/* main                                                                   */
/* ===================================================================== */
int main(int argc, char **argv)
{
    struct pal_host_cfg cfg;
    samir_pal_t *pal;
    (void)argc; (void)argv;   /* pure host oracle: no corpus needed */

    memset(&cfg, 0, sizeof(cfg));
    cfg.date_yy   = 99;
    cfg.date_mm   = 12;
    cfg.date_dd   = 31;
    cfg.heap_size = 8u * 1024u * 1024u;   /* generous: many fresh interps */

    pal = pal_host_make(cfg);
    if (!pal) {
        fprintf(stderr, "FATAL: pal_host_make returned NULL\n");
        return 2;
    }
    install_scripted(pal);                /* override conin_line / conin_char */
    script_reset();

    test_do_call_return(pal);
    test_params_positional(pal);
    test_nested_do(pal);
    test_private_shadow(pal);
    test_auto_private(pal);
    test_public_visible(pal);
    test_accept_input(pal);
    test_wait(pal);
    test_on_error(pal);
    test_gated_skips(pal);

    pal_host_free(pal);
    return TEST_SUMMARY("test-interp-proc");
}
