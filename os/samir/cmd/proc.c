/*
 * os/samir/cmd/proc.c -- SAMIR (InitechBase) procedures + scope + keyboard-I/O
 *                         command module. Step S5.7 (Phase 5 interpreter).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding (-ffreestanding -nostdlib). Uses
 * ONLY <stdint.h> and the samir/ engine headers. No libc, no int 0x21 -- every
 * byte of OS contact goes through the PAL carried by the interpreter (cooked
 * line input via pal->conin_line, single key via pal->conin_char, prompts via
 * pal->conout). Memory is fixed/static (no malloc). ASCII-clean (Rule 12).
 * Fail loud (Rule 2). Reproducible (Rule 11): pure function of (program text,
 * interp state, the PAL's scripted/injected console).
 *
 * dBASE III PLUS 1.1 ONLY (plan Sec 2.C).
 *
 * WHAT THIS IS (the S5.7 contract): a single xb_cmd_hook registered into the
 * executor's command-hook CHAIN (interp.h xb_interp_add_cmd_hook), the same shape
 * query.c (S5.4) / mutate.c (S5.5) / set.c (S5.6) use. It adds the procedure,
 * scope, and keyboard-I/O verbs WITHOUT editing flow.c:
 *
 *   DO <name> [WITH <arg list>]   call a PROCEDURE (or main program) by name
 *   PROCEDURE <name>              start a named routine (skipped at top level)
 *   PARAMETERS <name list>        bind the WITH args BY POSITION (PRIVATE)
 *   RETURN                        leave the current routine (clean early exit)
 *   PUBLIC  <name list>           declare level-0 globals (survive RETURN)
 *   PRIVATE <name list>           shadow caller/PUBLIC vars; restored on RETURN
 *   ACCEPT  [<prompt>] TO <m>     read a line -> always Character
 *   INPUT   [<prompt>] TO <m>     read a line -> EVALUATED, typed by the input
 *   WAIT    [<prompt>] [TO <m>]   read ONE key -> 1-char Character (or "")
 *   ON ERROR <command>            install a runtime-error trap; ON ERROR clears it
 *
 * THE SCOPE MODEL lives in flow.c (the memvar-table owner). This module DRIVES it:
 *   DO <name> -> xb_interp_scope_enter -> run the procedure body via samir_do ->
 *   xb_interp_scope_leave. PARAMETERS -> xb_interp_store_param (by position).
 *   PUBLIC/PRIVATE -> xb_interp_declare_public / _private. The auto-private rule
 *   (a STORE/= to an unseen name creates it at the current level) is in
 *   xb_interp_store. (memory-variables.md sec 3; the flow.c scope header.)
 *
 * PROGRAM / PROCEDURE SOURCE. dBASE runs a .prg top to bottom UNTIL the first
 * PROCEDURE / RETURN / EOF; the PROCEDURE blocks below are callable by DO. This
 * module is given the WHOLE source via proc_run(ip, prg): it registers the source
 * (so DO can scan it for PROCEDURE blocks) and runs the MAIN body (the slice
 * before the first top-level PROCEDURE) through samir_do. samir_do is RE-ENTRANT
 * (flow.c keeps line pools on the stack), so DO recurses: a PROCEDURE body that
 * itself DOes another PROCEDURE works to FLOW_MAX_SCOPE depth (Rule 2 fail-loud
 * cap past that).
 *
 * RETURN. A RETURN inside a procedure body (possibly nested in an IF/DO WHILE)
 * must leave the routine immediately. The RETURN hook returns the special
 * negative sentinel PROC_RC_RETURN; the flow executor bubbles it up out of any
 * enclosing IF/DO WHILE/DO CASE (run_block returns on rc != INTERP_OK), so the
 * samir_do running the body stops at RETURN; proc_call catches PROC_RC_RETURN and
 * treats it as a CLEAN return. A RETURN at the MAIN level is likewise clean.
 *
 * GATED (loud-skip in the oracle; plan Sec 7 / corpus Open-questions):
 *   - PARAMETERS by-REFERENCE write-back exactness (a bare WITH name aliasing the
 *     caller's var) -- we pass BY VALUE only (control-flow-and-procedures.md
 *     Open-q 6/7 + memory-variables.md Open-q 5).
 *   - The uninitialized PUBLIC/PRIVATE VALUE (Clipper .F.; III+ 1.1 parity
 *     unconfirmed) -- we init to XB_U (memory-variables.md Open-q 3).
 *   - DO-name precedence (open PROCEDURE vs disk <name>.prg) -- we resolve only
 *     against the registered source's PROCEDURE blocks
 *     (control-flow-and-procedures.md Open-q 1).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/commands/control-flow-and-procedures.md sec 7-8
 *     (DO/PROCEDURE/PARAMETERS/RETURN/DO WITH; by-ref/by-value; search order).
 *   - ../dbase3-decomp/specs/commands/programming-and-io.md sec 8-10,14
 *     (ACCEPT/INPUT/WAIT; ON ERROR trap install/clear + ERROR()).
 *   - ../dbase3-decomp/specs/language/memory-variables.md sec 2.1, sec 3
 *     (ACCEPT=C / INPUT=typed / WAIT=1-char; PUBLIC/PRIVATE/PARAMETERS scope).
 *   - spec/samir/dbase_msg_codes.tsv (#9 mismatch, #16 unrecognized, #37 not
 *     logical, #93 No PARAMETER statement found., #94 Wrong number of parameters.).
 *   - os/samir/include/samir/interp.h (the xb_cmd_hook chain + the S5.7 scope API).
 *   - os/samir/include/samir/eval.h (xb_lex/xb_parse/xb_eval -- INPUT evaluation).
 *   - os/samir/include/samir/pal.h (conin_line / conin_char / conout -- the only
 *     OS surface here).
 */

#include <stdint.h>

#include "samir/interp.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"
#include "samir/pal.h"

/* ===================================================================== */
/* Tunables (bounded -- Rule 2 fail-loud on overflow, never UB)            */
/* ===================================================================== */

#define P_MAX_TOKS     128   /* per-expression token pool (INPUT eval) */
#define P_MAX_NODES    128   /* per-expression node pool */
#define P_MAX_EXPR     254   /* one expression / prompt working copy */
#define P_MAX_NAME     11    /* dBASE memvar/procedure name: 10 + NUL */
#define P_MAX_ARGS     16    /* DO ... WITH arg list / PARAMETERS list cap */
#define P_MAX_LINEIN   256   /* one cooked input line (ACCEPT/INPUT) */
#define P_MAX_ONERR    254   /* the installed ON ERROR command text */
/* Concurrent interpreters with proc state. Pure dimensioning knob, tied to the
 * flow registry it mirrors: the Milton .COM build passes -DFLOW_MAX_REGISTRY=1
 * (ADR-0009 DEC-03) -- one interpreter -- and the 16-entry host default would
 * spend ~4 KiB of the full SAMIR.COM BSS reserve (samir.ld ASSERT) on slots
 * that can never be used (initech-39e5 added the ON ESCAPE trap text). */
#ifdef FLOW_MAX_REGISTRY
# define P_REGISTRY    FLOW_MAX_REGISTRY
#else
# define P_REGISTRY    16
#endif

/* dBASE catalog ordinals (spec/samir/dbase_msg_codes.tsv -- the ENGINE table). */
#define P_MSG_NO_PARAM      93  /* "No PARAMETER statement found." */
#define P_MSG_WRONG_PARAM   94  /* "Wrong number of parameters." */
#define P_MSG_NOT_FOUND     16  /* an unknown DO target -> unrecognized */

/* RETURN sentinel: a negative rc the flow executor bubbles up out of nested
 * control flow; proc_call maps it to a clean return. Chosen WELL outside the
 * interp_err range (1..8) and the 1..151 dBASE catalog so it can never be
 * mistaken for a real error code recorded by samir_last_error. */
#define PROC_RC_RETURN  (-20000)
/* RETURN TO MASTER (Using III+ U5-183): unwinds DO frames until the frame
 * called FROM the highest-level program, which resumes after its DO. */
#define PROC_RC_RETURN_MASTER (-20001)
/* RETRY raised in a routine body (Using III+ U5-182). p_call turns it into
 * INTERP_RC_RETRY (re-run the caller's DO) or, for the routine an ON ERROR
 * trap called, PROC_RC_RETRY_TRAP (re-run the failed statement). */
#define PROC_RC_RETRY         (-20002)
#define PROC_RC_RETRY_TRAP    (-20004)

/* DO nesting cap. Using III+ U7-4: "DOs nested too deep. [103] You can only
 * have a maximum of 20 nested DO commands." Real III+ 1.1 (mint samirctl
 * NEST.TXT, dosbox-x): the top-level program counts as level 1 and the DO
 * that would open level 21 fails with ERROR()=103, MESSAGE()="DOs nested too
 * deep." (TRAP 19 103). The cap must also FIT: every level now costs ~2 KiB of
 * the 64 KiB SAMIR.COM stack (initech-39e5, audit L003). */
#ifndef P_MAX_DO_NEST
#define P_MAX_DO_NEST       20
#endif
#define P_MSG_DO_DEEP       103 /* "DOs nested too deep." */
#define P_MAX_PRGNAME       16

/* ===================================================================== */
/* Per-interpreter proc state (registered source + ON ERROR trap).         */
/* Bounded static registry, the same idiom as flow.c / query.c -- the       */
/* engine is single-threaded cooperative; no malloc.                        */
/* ===================================================================== */

typedef struct {
    const char *source;          /* the registered program source (proc_run) */
    char        onerr[P_MAX_ONERR + 1]; /* installed ON ERROR command, "" = none */
    int         has_onerr;       /* 1 = an ON ERROR trap is installed */
    int         in_handler;      /* 1 = currently running the trap (no re-entry) */

    /* the pending DO ... WITH arg vector for the routine CURRENTLY being entered.
     * A PARAMETERS line in that routine's body binds these positionally. The
     * vector lives in the calling p_do frame (alive for the whole call); only
     * the pointer/count are saved/restored around a nested p_call. */
    xb_val     *pending_args;
    int         pending_nargs;
    int         params_seen;     /* 1 once a PARAMETERS line ran for this call */

    /* initech-39e5 program control. */
    char        onesc[P_MAX_ONERR + 1]; /* installed ON ESCAPE command */
    int         has_onesc;
    int         in_esc;          /* running the ON ESCAPE trap (no re-entry) */
    int         do_depth;        /* active DO levels (top program = 1) */
    int         master_depth;    /* do_depth of the highest-level program */
    int         handler_depth;   /* do_depth when the ON ERROR trap fired */
    int         run_depth;       /* nested proc_run calls (1 = outermost) */
    char        prgname[P_MAX_PRGNAME]; /* running program, "" = dot prompt */
    int       (*suspend)(xb_interp *); /* nested dot prompt for "S" */
} proc_state;

static xb_interp  *g_key[P_REGISTRY];
static proc_state  g_val[P_REGISTRY];
static int         g_n;

static proc_state *proc_state_for(xb_interp *ip)
{
    int i;
    if (!ip)
        return (proc_state *)0;
    for (i = 0; i < g_n; i++) {
        if (g_key[i] == ip)
            return &g_val[i];
    }
    if (g_n >= P_REGISTRY)
        return (proc_state *)0;          /* too many live interpreters: fail loud */
    g_key[g_n] = ip;
    /* every field starts empty/zero: no source, no ON ERROR / ON ESCAPE
     * trap, no pending WITH args, depth 0, dot prompt (prgname ""), no
     * suspend hook -- one fill instead of a store per field. */
    rt_memset(&g_val[g_n], 0, (uint32_t)sizeof(g_val[0]));
    return &g_val[g_n++];
}

/* ===================================================================== */
/* Small ASCII helpers (freestanding; no libc)                            */
/* ===================================================================== */

static char p_up1(char c)
{
    if (c >= 'a' && c <= 'z')
        return (char)(c - 'a' + 'A');
    return c;
}

static int p_isspace(char c) { return c == ' ' || c == '\t'; }

/* (case-insensitive keyword equality is rt_kw_index -- samir/rt.h) */

/* skip leading blanks; return pointer past them. */
static const char *p_skip_ws(const char *s)
{
    while (*s != '\0' && p_isspace(*s)) s++;
    return s;
}

/*
 * p_ci_eq_prefix: does the text at `s` (bounded above by `le`) begin with the
 * keyword `kw` as a WHOLE WORD (kw matched case-insensitively, then end-of-line
 * or a blank)? Used to spot a "PROCEDURE <name>" line. Returns typed length/0.
 */
static int p_ci_eq_prefix(const char *s, const char *kw, const char *le)
{
    int n = rt_kw_match(s, kw);
    return s + n <= le ? n : 0;
}

/* ===================================================================== */
/* Expression evaluation through the interp ctx (INPUT + a prompt expC)    */
/* ===================================================================== */

/* ===================================================================== */
/* The " TO " keyword splitter (ACCEPT/INPUT/WAIT prompt-vs-target)        */
/* ===================================================================== */

/*
 * p_find_to: locate a whole-word " TO " at the TOP level (outside string
 * literals) in `s`. On a hit, *to_at = the offset of the 'T'. Returns 1/0.
 */
static int p_find_to(const char *s, uint32_t *to_at)
{
    int in_s = 0, in_d = 0, in_b = 0;
    uint32_t i;
    char prev = ' ';
    for (i = 0; s[i] != '\0'; i++) {
        char c = s[i];
        if (in_s) { if (c == '\'') in_s = 0; prev = c; continue; }
        if (in_d) { if (c == '"')  in_d = 0; prev = c; continue; }
        if (in_b) { if (c == ']')  in_b = 0; prev = c; continue; }
        if (c == '\'') { in_s = 1; prev = c; continue; }
        if (c == '"')  { in_d = 1; prev = c; continue; }
        if (c == '[')  { in_b = 1; prev = c; continue; }
        if (p_isspace(prev) && p_up1(c) == 'T' && p_up1(s[i + 1]) == 'O' &&
            (s[i + 2] == '\0' || p_isspace(s[i + 2]))) {
            *to_at = i;
            return 1;
        }
        prev = c;
    }
    return 0;
}

/* copy the first blank-delimited name (upper-cased) from `s` into nm[P_MAX_NAME].
 * returns the name length, or 0 if none. */
static uint32_t p_take_name(const char *s, char *nm)
{
    uint32_t n = 0;
    s = p_skip_ws(s);
    while (*s != '\0' && !p_isspace(*s) && *s != ',') {
        char c = *s;
        int wordch = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') || c == '_';
        if (!wordch) break;
        if (n < (uint32_t)(P_MAX_NAME - 1)) nm[n++] = p_up1(c);
        s++;
    }
    nm[n] = '\0';
    return n;
}

/* ===================================================================== */
/* ACCEPT / INPUT / WAIT                                                   */
/* ===================================================================== */

/*
 * p_emit_prompt: evaluate the prompt expression `pexpr` (an expC, may be empty)
 * and conout it (no trailing newline). An empty prompt emits nothing. A non-C
 * prompt is fail-loud (#9) -- the prompt MUST be a character expression
 * (programming-and-io.md ACCEPT/INPUT/WAIT). Returns 0 or negative.
 */
static int p_emit_prompt(xb_interp *ip, const char *pexpr, int *ec)
{
    xb_val v;
    int rc;
    const char *p = p_skip_ws(pexpr);
    if (*p == '\0') { if (ec) *ec = 0; return INTERP_OK; }   /* no prompt */
    rc = xb_interp_eval(ip, p, &v, ec);
    if (rc != INTERP_OK)
        return rc;
    if (v.t != XB_C && v.t != XB_M) {
        if (ec) *ec = XBEE_MISMATCH;        /* prompt must be Character (#9) */
        return -INTERP_ERR_EVAL;
    }
    if(v.u.c.p) xb_interp_output(ip,v.u.c.p,(uint32_t)v.u.c.len);
    if (ec) *ec = 0;
    return INTERP_OK;
}

/*
 * p_split_prompt_to: parse "[<prompt>] TO <m>" -- the ACCEPT/INPUT grammar. On a
 * found " TO ", copy the prompt slice (trimmed) into pbuf and the target name
 * into nm, return 1. If there is no " TO ", return 0 (the caller fails loud).
 */
static int p_split_prompt_to(const char *args, char *pbuf, uint32_t pcap,
                             char *nm)
{
    uint32_t to_at, plen, i;
    const char *tgt;
    if (!p_find_to(args, &to_at))
        return 0;
    plen = to_at;
    while (plen > 0u && p_isspace(args[plen - 1])) plen--;
    if (plen >= pcap) plen = pcap - 1u;
    for (i = 0; i < plen; i++) pbuf[i] = args[i];
    pbuf[plen] = '\0';
    tgt = args + to_at + 2;                 /* past "TO" */
    if (p_take_name(tgt, nm) == 0u)
        return 0;
    return 1;
}

/* read one cooked line into buf (cap), stripping a trailing newline. Returns the
 * length (>=0), or <0 on EOF. */
static int32_t p_readline(xb_interp *ip, char *buf, uint32_t cap)
{
    samir_pal_t *pal = xb_interp_pal(ip);
    int32_t n;
    if (!pal || !pal->conin_line) { buf[0] = '\0'; return -1; }
    n = xb_interp_read_line(ip,buf,cap);
    if (n < 0) { buf[0] = '\0'; return -1; }
    if ((uint32_t)n >= cap) n = (int32_t)(cap - 1u);
    buf[n] = '\0';
    return n;
}

/* ACCEPT [<prompt>] TO <memvarC>: store the raw line as Character. */
static int p_accept(xb_interp *ip, const char *args, int *ec)
{
    char pbuf[P_MAX_EXPR + 1];
    char nm[P_MAX_NAME];
    char line[P_MAX_LINEIN];
    int32_t len;
    xb_val v;
    int rc;

    if (!p_split_prompt_to(args, pbuf, sizeof(pbuf), nm)) {
        if (ec) *ec = 0;                     /* ACCEPT without TO <m> */
        return -INTERP_ERR_SYNTAX;
    }
    rc = p_emit_prompt(ip, pbuf, ec);
    if (rc != INTERP_OK) return rc;

    len = p_readline(ip, line, sizeof(line));
    if (len < 0) len = 0;                    /* EOF -> "" (null string) */
    /* ACCEPT is ALWAYS Character (programming-and-io.md sec 8). xb_interp_store
     * copies the C bytes into the memvar arena, so `line` may be on the stack. */
    v = xb_c(line, (uint16_t)len);
    rc = xb_interp_store(ip, nm, &v);
    if (rc != INTERP_OK) { if (ec) *ec = 0; return rc; }
    if (ec) *ec = 0;
    return INTERP_OK;
}

/* INPUT [<prompt>] TO <memvar>: EVALUATE the line; the var takes its type. */
static int p_input(xb_interp *ip, const char *args, int *ec)
{
    char pbuf[P_MAX_EXPR + 1];
    char nm[P_MAX_NAME];
    char line[P_MAX_LINEIN];
    int32_t len;
    xb_val v;
    int rc;

    if (!p_split_prompt_to(args, pbuf, sizeof(pbuf), nm)) {
        if (ec) *ec = 0;
        return -INTERP_ERR_SYNTAX;
    }
    rc = p_emit_prompt(ip, pbuf, ec);
    if (rc != INTERP_OK) return rc;

    len = p_readline(ip, line, sizeof(line));
    if (len <= 0) {
        /* Empty input: III+ exact behavior is GATED (programming-and-io.md sec 9
         * Open-q). We store a null Character string so the memvar exists. */
        v = xb_c(line, 0);
        rc = xb_interp_store(ip, nm, &v);
        if (rc != INTERP_OK) { if (ec) *ec = 0; return rc; }
        if (ec) *ec = 0;
        return INTERP_OK;
    }
    /* The typed text is an expression: "12.5"->N, .T.->L, 'abc'/"abc"->C. */
    rc = xb_interp_eval(ip, line, &v, ec);
    if (rc != INTERP_OK)
        return rc;                          /* a bad expression fails loud */
    rc = xb_interp_store(ip, nm, &v);
    if (rc != INTERP_OK) { if (ec) *ec = 0; return rc; }
    if (ec) *ec = 0;
    return INTERP_OK;
}

/* WAIT [<prompt>] [TO <memvarC>]: read ONE key -> 1-char Character (or ""). */
static int p_wait(xb_interp *ip, const char *args, int *ec)
{
    samir_pal_t *pal = xb_interp_pal(ip);
    char pbuf[P_MAX_EXPR + 1];
    char nm[P_MAX_NAME];
    int has_to;
    int32_t key;
    char ch[1];
    xb_val v;
    int rc;

    has_to = p_split_prompt_to(args, pbuf, sizeof(pbuf), nm);
    if (!has_to) {
        /* No " TO ": the entire argument is the (optional) prompt expression. */
        const char *p = p_skip_ws(args);
        uint32_t i = 0;
        while (*p != '\0' && i < (uint32_t)P_MAX_EXPR) pbuf[i++] = *p++;
        pbuf[i] = '\0';
        nm[0] = '\0';
    }
    rc = p_emit_prompt(ip, pbuf, ec);
    if (rc != INTERP_OK) return rc;

    /* read a single key (no echo). */
    key = (pal && pal->conin_char) ? pal->conin_char(pal) : -1;

    if (has_to) {
        /* store the key: a printable key -> 1-char C; a non-printing key / EOF
         * -> null string "" (programming-and-io.md sec 10; memory-variables.md
         * sec 2.1). */
        if (key >= 0x20 && key <= 0x7E) {
            ch[0] = (char)key;
            v = xb_c(ch, 1);
        } else {
            v = xb_c(ch, 0);                /* "" for control / EOF keys */
        }
        rc = xb_interp_store(ip, nm, &v);
        if (rc != INTERP_OK) { if (ec) *ec = 0; return rc; }
    }
    if (ec) *ec = 0;
    return INTERP_OK;
}

/* ===================================================================== */
/* PUBLIC / PRIVATE / PARAMETERS  (name-list parsing)                     */
/* ===================================================================== */

/*
 * p_for_each_name: call `each(ip, name, ud)` for every comma-separated name in
 * `args`. Returns INTERP_OK, or the first negative rc `each` produced. A name
 * list with zero names is a syntax error.
 */
static int p_for_each_name(xb_interp *ip, const char *args,
                           int (*each)(xb_interp *, const char *, void *),
                           void *ud, int *ec)
{
    const char *q = args;
    int any = 0, rc;
    for (;;) {
        char nm[P_MAX_NAME];
        uint32_t n;
        q = p_skip_ws(q);
        n = p_take_name(q, nm);
        if (n > 0u) {
            any = 1;
            rc = each(ip, nm, ud);
            if (rc != INTERP_OK) { if (ec) *ec = 0; return rc; }
        }
        /* advance to the next comma. */
        while (*q != '\0' && *q != ',') q++;
        if (*q == ',') { q++; continue; }
        break;
    }
    if (!any) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    if (ec) *ec = 0;
    return INTERP_OK;
}

static int p_each_public(xb_interp *ip, const char *nm, void *ud)
{
    (void)ud;
    return xb_interp_declare_public(ip, nm);
}

static int p_each_private(xb_interp *ip, const char *nm, void *ud)
{
    (void)ud;
    return xb_interp_declare_private(ip, nm);
}

/* PARAMETERS state threaded through p_for_each_name: the WITH-arg vector. */
typedef struct {
    xb_val *args;
    int     nargs;
    int     idx;       /* next positional arg to bind */
} param_bind;

static int p_each_param(xb_interp *ip, const char *nm, void *ud)
{
    param_bind *pb = (param_bind *)ud;
    if (pb->idx < pb->nargs) {
        xb_val v = pb->args[pb->idx];
#ifdef PROC_MUTATE_PARAM_ORDER
        /* MUTANT (Rule 6 -- -DPROC_MUTATE_PARAM_ORDER): bind the args in REVERSE
         * positional order. PARAMETERS is positional (control-flow-and-
         * procedures.md sec 8.3), so a reversed bind swaps the param values ->
         * the oracle's "param N has the N-th WITH value" check goes RED. */
        v = pb->args[pb->nargs - 1 - pb->idx];
#endif
        pb->idx++;
        return xb_interp_store_param(ip, nm, &v);
    }
    /* FEWER WITH args than PARAMETERS names: the trailing names are declared
     * (uninitialized). The exact III+ 1.1 init value is GATED (XB_U here). */
    pb->idx++;
    return xb_interp_declare_private(ip, nm);
}

/* ===================================================================== */
/* DO <name> [WITH <arg list>]  + PROCEDURE-block lookup in the source     */
/* ===================================================================== */

/*
 * p_find_procedure: scan the registered source for a line "PROCEDURE <name>"
 * (case-insensitive, whole-word) and return a pointer to the FIRST byte AFTER
 * that line (the start of the procedure body), or NULL if not found.
 */
static const char *p_find_procedure(const char *src, const char *name)
{
    const char *p = src;
    if (!src)
        return (const char *)0;
    while (*p != '\0') {
        const char *ls = p;                 /* line start */
        const char *le;                     /* line end (the LF or NUL) */
        const char *t;
        /* find this physical line's end. */
        le = ls;
        while (*le != '\0' && *le != '\n') le++;
        /* does the trimmed line start with "PROCEDURE <name>"? */
        t = p_skip_ws(ls);
        if (le > t && (p_up1(t[0]) == 'P') &&
            p_ci_eq_prefix(t, "PROCEDURE", le)) {
            const char *nm = p_skip_ws(t + rt_kw_match(t, "PROCEDURE"));
            /* compare the name token (whole word) against `name`. */
            int i = 0;
            while (nm + i < le && name[i] != '\0' &&
                   p_up1(nm[i]) == p_up1(name[i]))
                i++;
            if (name[i] == '\0') {
                const char *after = nm + i;
                /* Real DOS .PRG files use CR LF (mint samirdml/clear).
                 * CR terminates the procedure name just as LF does. */
                if (after >= le || *after == '\r' || p_isspace(*after)) {
                    /* match: body starts on the NEXT physical line. */
                    return (*le == '\n') ? le + 1 : le;
                }
            }
        }
        p = (*le == '\n') ? le + 1 : le;
    }
    return (const char *)0;
}

/*
 * p_body_len: the length of the routine body starting at `body`: up to (not
 * including) the next top-level PROCEDURE line, or the end of the source.
 * RETURN lines are LEFT IN (the RETURN hook stops the body cleanly). The body
 * is then run IN PLACE (samir_do_n) -- it used to be copied into an 8 KiB
 * stack buffer per DO level, which overran the 64 KiB SAMIR.COM stack under
 * recursion (audit 2026-10-23-samir L003, initech-39e5).
 */
static uint32_t p_body_len(const char *body)
{
    const char *p = body;
    while (*p != '\0') {
        const char *le = p;
        const char *t;
        while (*le != '\0' && *le != '\n') le++;
        t = p_skip_ws(p);
        if (le > t && p_up1(t[0]) == 'P' &&
            p_ci_eq_prefix(t, "PROCEDURE", le))
            break;                          /* next PROCEDURE: body ends here */
        p = (*le == '\n') ? le + 1 : le;
    }
    return (uint32_t)(p - body);
}

/*
 * p_call: run the procedure `name` with the (already evaluated) WITH args.
 * Enters a scope, runs the body via samir_do, leaves the scope. Maps the RETURN
 * sentinel to a clean return. Returns INTERP_OK or a negative rc (with *ec set).
 */
static int p_call(xb_interp *ip, proc_state *ps, const char *name,
                  xb_val *args, int nargs, int *ec)
{
    const char *body;
    int rc, depth_before;

    body = p_find_procedure(ps->source, name);
    if (!body) {
        if (ec) *ec = P_MSG_NOT_FOUND;      /* unknown DO target */
        return -INTERP_ERR_UNKNOWN_CMD;
    }
    /* Using III+ U7-4 [103] + mint samirctl NEST.TXT: at most 20 DO levels. */
#ifndef PROC_MUTATE_NO_DO_CAP
    if (ps->do_depth >= P_MAX_DO_NEST) {
        if (ec) *ec = P_MSG_DO_DEEP;
        return -INTERP_ERR_DEPTH;
    }
#endif

    /* push a DO-call scope; bind PARAMETERS happens inside the body via the
     * PARAMETERS verb, which reads the pending arg vector below. */
    rc = xb_interp_scope_enter(ip);
    if (rc < 0) { if (ec) *ec = 0; return rc; }   /* DO nested too deep */

    /* stash the pending WITH args so a PARAMETERS line in the body can bind them
     * positionally (proc_state-level, since the body runs via a fresh samir_do). */
    {
        int saved_n = ps->pending_nargs;
        xb_val *saved_args = ps->pending_args;
        int saved_seen = ps->params_seen;

        ps->pending_args  = args;
        ps->pending_nargs = nargs;
        ps->params_seen   = 0;

        depth_before = ps->do_depth;
        ps->do_depth++;
        { uint8_t dot=xb_interp_ctx(ip)->dot_command;
          xb_interp_ctx(ip)->dot_command=0;
          rc = samir_do_n(ip, body, p_body_len(body));
          xb_interp_ctx(ip)->dot_command=dot; }
        ps->do_depth = depth_before;
        if (rc == PROC_RC_RETURN)
            rc = INTERP_OK;                 /* a clean RETURN out of the body */
#ifndef PROC_MUTATE_MASTER_ONE_LEVEL
        /* RETURN TO MASTER: only the frame called FROM the highest-level
         * program resumes; deeper frames keep unwinding (U5-183; mint
         * samirctl MASTER.TXT: MASTER FIRST SECOND MASTER RESUMED). */
        if (rc == PROC_RC_RETURN_MASTER && depth_before <= ps->master_depth)
            rc = INTERP_OK;
#else
        if (rc == PROC_RC_RETURN_MASTER) rc = INTERP_OK;
#endif
        /* RETRY (U5-182): back to the caller, re-running the calling DO; for
         * the routine an ON ERROR trap called, re-run the failed command. */
        if (rc == PROC_RC_RETRY)
            rc = (ps->in_handler && depth_before == ps->handler_depth)
                 ? PROC_RC_RETRY_TRAP : INTERP_RC_RETRY;

        /* DO ... WITH to a routine with NO PARAMETERS line is error #93. */
        if (rc == INTERP_OK && nargs > 0 && !ps->params_seen) {
            rc  = -INTERP_ERR_SYNTAX;
            if (ec) *ec = P_MSG_NO_PARAM;
        }

        /* restore the caller's pending-arg context. */
        ps->pending_args  = saved_args;
        ps->pending_nargs = saved_n;
        ps->params_seen   = saved_seen;
    }

    /* leave the scope: release this routine's PRIVATE/PARAMETERS vars, restore
     * anything they shadowed. (Done even on an error -- the RETURN sentinel was
     * already mapped to INTERP_OK above.) */
    xb_interp_scope_leave(ip);

    return rc;                              /* INTERP_OK or a real error */
}

static void p_release_args(samir_pal_t *pal, char **blk, int n)
{
    while (n > 0) pal->release(pal, blk[--n]);
}

/*
 * p_do: handle DO <name> [WITH <arg expr list>]. Evaluates the WITH args (by
 * value -- by-ref is GATED), then calls the named procedure.
 */
static int p_do(xb_interp *ip, proc_state *ps, const char *args, int *ec)
{
    char name[P_MAX_NAME];
    const char *p = p_skip_ws(args);
    xb_val argv[P_MAX_ARGS];
    /* WITH values outlive each argument's expression buffer and scratch
     * reset. Using III+ U5-111: arbitrary expressions, including literals.
     * Their bytes go to one PAL heap block per call (acquire/release), not a
     * 4 KiB stack array per DO level (initech-39e5 stack budget). */
    samir_pal_t *pal = xb_interp_pal(ip);
    char *argblk[P_MAX_ARGS];               /* one exact-size block per C arg */
    int nblk = 0;
    int nargs = 0, crc, ecode;
    uint32_t nlen;

    nlen = p_take_name(p, name);
    if (nlen == 0u) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    p += nlen;
    p = p_skip_ws(p);

    /* optional WITH <expr>[, <expr>...] */
    if (rt_kw_word(p, "WITH\0", (int *)0) == 0) {
        p = p_skip_ws(p + 4);
        /* split the comma list at the top level (outside string literals/parens)
         * and evaluate each argument expression by value. */
        while (*p != '\0') {
            char ebuf[P_MAX_EXPR + 1];
            uint32_t eo = 0;
            int in_s = 0, in_d = 0, in_b = 0, depth = 0;
            xb_val v;
            while (*p != '\0') {
                char c = *p;
                if (in_s) { if (c == '\'') in_s = 0; }
                else if (in_d) { if (c == '"') in_d = 0; }
                else if (in_b) { if (c == ']') in_b = 0; }
                else if (c == '\'') in_s = 1;
                else if (c == '"') in_d = 1;
                else if (c == '[') in_b = 1;
                else if (c == '(') depth++;
                else if (c == ')') { if (depth > 0) depth--; }
                else if (c == ',' && depth == 0) break;
                if (eo >= (uint32_t)P_MAX_EXPR) { ecode = 0; goto syntax; }
                ebuf[eo++] = c;
                p++;
            }
            while (eo > 0u && p_isspace(ebuf[eo - 1])) eo--;
            ebuf[eo] = '\0';
            if (nargs >= P_MAX_ARGS) { ecode = P_MSG_WRONG_PARAM; goto syntax; }
            crc = xb_interp_eval(ip, ebuf, &v, ec);
            if (crc != INTERP_OK)
                goto out;
            if(v.t==XB_C || v.t==XB_M) {
                if(v.u.c.len>254){ecode=18;goto syntax;}
                argblk[nblk]=(pal && pal->acquire)?(char *)pal->acquire(pal,(uint32_t)v.u.c.len+1u):(char *)0;
                if(!argblk[nblk]){ecode=43;crc=-INTERP_ERR_NOMEM;goto fail;} /* #43 */
                rt_memcpy(argblk[nblk],v.u.c.p,(uint32_t)v.u.c.len);
#ifndef SAMIR_FNS_MUTANT_with
                v.u.c.p=argblk[nblk];
#endif
                nblk++;
            }
            argv[nargs++] = v;
            if (*p == ',') { p++; p = p_skip_ws(p); continue; }
            break;
        }
    }

    crc = p_call(ip, ps, name, argv, nargs, ec);
out:
    p_release_args(pal, argblk, nblk);  /* every exit frees the WITH blocks */
    return crc;
syntax:
    crc = -INTERP_ERR_SYNTAX;
fail:
    if (ec) *ec = ecode;
    goto out;
}

/* ===================================================================== */
/* ON ERROR <command> | ON ERROR (clear)                                  */
/* ===================================================================== */

static int p_on(xb_interp *ip, proc_state *ps, const char *args, int *ec)
{
    const char *p = p_skip_ws(args);
    char w[16];
    uint32_t n = 0;
    int k;
    (void)ip;

    /* the ON option keyword: ERROR / ESCAPE / KEY. We implement ERROR; ESCAPE /
     * KEY parse-and-store as no-op traps (they never fire in the host oracle). */
    while (p[n] != '\0' && !p_isspace(p[n]) && n < (uint32_t)(sizeof(w) - 1)) {
        w[n] = p_up1(p[n]); n++;
    }
    w[n] = '\0';
    p = p_skip_ws(p + n);

    k = rt_kw_index(w, "ERROR\0ESCAPE\0KEY\0");
    if (ec) *ec = 0;
    if (k < 0)
        return -INTERP_ERR_SYNTAX;          /* unknown ON option */
    if (k < 2) {
        /* ON ERROR <command> installs the error trap; ON ESCAPE <command>
         * (Using III+ U5-160) runs when Esc is pressed (SET ESCAPE ON) and
         * is fired by proc_escape (mint samirctl ESCTRAP.TXT: the trap ends
         * a DO WHILE). The bare form clears either trap. */
        char *dst = k ? ps->onesc : ps->onerr;
        uint32_t i = 0;
        while (p[i] != '\0' && i < (uint32_t)P_MAX_ONERR) { dst[i] = p[i]; i++; }
        dst[i] = '\0';
        *(k ? &ps->has_onesc : &ps->has_onerr) = (*p != '\0');
    }
    /* ON KEY: parsed + accepted; the ON KEY trap is not modelled.
     * (programming-and-io.md sec 14 -- precedence ERROR>ESCAPE>KEY.) */
    return INTERP_OK;
}

/* ===================================================================== */
/* The command hook (the S5.7 module entry point)                         */
/* ===================================================================== */

static int proc_cmd_hook(void *user, xb_interp *ip,
                         const char *verb, const char *args, int *err_code)
{
    proc_state *ps;
    (void)user;
    if (err_code) *err_code = 0;
    if (!ip || !verb)
        return CMD_UNKNOWN;

    ps = proc_state_for(ip);
    if (!ps) { if (err_code) *err_code = 0; return -INTERP_ERR_NOMEM; }

    switch (rt_kw_index(verb, "DO\0PROCEDURE\0PARAMETERS\0PARAMETER\0RETURN\0RETRY\0"
                              "CANCEL\0QUIT\0PUBLIC\0PRIVATE\0ACCEPT\0INPUT\0WAIT\0ON\0")) {
    case 0: { /* DO */
        /* DO WHILE / DO CASE are the flow spine's (handled before the hook); only
         * DO <name> reaches here. */
        int rc = p_do(ip, ps, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    case 1: { /* PROCEDURE */
        /* A PROCEDURE line reached during execution (e.g. trailing the main body
         * if proc_run is bypassed) ENDS the current routine -- treat it as a
         * clean RETURN so we never fall into the next routine's body. */
        return PROC_RC_RETURN;
    }
    case 2: case 3: { /* PARAMETERS, PARAMETER */
        param_bind pb;
        int rc;
        ps->params_seen = 1;
        pb.args  = ps->pending_args;
        pb.nargs = ps->pending_nargs;
        pb.idx   = 0;
        /* error #94 if MORE WITH args than PARAMETERS names: counted after the
         * bind by comparing idx (names consumed) against nargs. */
        rc = p_for_each_name(ip, args, p_each_param, &pb, err_code);
        if (rc != INTERP_OK) return rc;
        if (pb.idx < pb.nargs) {            /* names < WITH args -> too many */
            if (err_code) *err_code = P_MSG_WRONG_PARAM;
            return -INTERP_ERR_SYNTAX;
        }
        return CMD_OK;
    }
    case 4: { /* RETURN */
        /* RETURN [TO MASTER] (Using III+ U5-183). TO MASTER unwinds to the
         * highest-level program (audit L041; mint samirctl MASTER.TXT). */
        const char *a = p_skip_ws(args);
        /* (a bare "RETURN TO" leaves "" -- not MASTER -- either way) */
        if (rt_kw_word(a, "TO\0", (int *)0) == 0 &&
            rt_kw_index(p_skip_ws(a + 2), "MASTER\0") == 0)
            return PROC_RC_RETURN_MASTER;
        return PROC_RC_RETURN;
    }
    case 5: { /* RETRY */
        /* Using III+ U5-182; audit L022; mint samirctl RETRYM.TXT. RETRY
         * clears ERROR(). */
        xb_interp_ctx(ip)->error_number = 0;
        return PROC_RC_RETRY;
    }
    case 6: { /* CANCEL */
        /* Using III+ U5-43: CANCEL ends the program, back to the dot prompt
         * (a no-op typed at the session prompt; it ends a SUSPENDed one). */
        return INTERP_RC_CANCEL;
    }
    case 7: { /* QUIT */
        /* Using III+ U5-168: QUIT closes all files and leaves dBASE, also
         * from a program (audit L013; mint samirctl QUITP.TXT: AFTER QUIT is
         * never printed). The dot prompt does the closing on this code. */
#ifndef PROC_MUTATE_NO_QUIT
        return INTERP_RC_QUIT;
#else
        return CMD_UNKNOWN;                 /* MUTANT: the pre-fix #16 */
#endif
    }
    case 8: { /* PUBLIC */
        int rc = p_for_each_name(ip, args, p_each_public, (void *)0, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    case 9: { /* PRIVATE */
        int rc = p_for_each_name(ip, args, p_each_private, (void *)0, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    case 10: { /* ACCEPT */
        int rc = p_accept(ip, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    case 11: { /* INPUT */
        int rc = p_input(ip, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    case 12: { /* WAIT */
        int rc = p_wait(ip, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    case 13: { /* ON */
        int rc = p_on(ip, ps, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    default:
        break;
    }

    return CMD_UNKNOWN;                      /* not a proc/IO verb */
}

/* ===================================================================== */
/* Public registration + the proc_run driver                              */
/* ===================================================================== */

int proc_register(xb_interp *ip);
int proc_run(xb_interp *ip, const char *prg);
int proc_run_program(xb_interp *ip, const char *prg, const char *name);
void proc_set_suspend(xb_interp *ip, int (*fn)(xb_interp *));
int proc_fire_onerror(xb_interp *ip, int err_code);

/* The xb_ctx.on_error trap: run the installed ON ERROR command for `code`
 * and report it handled, so the failing statement is skipped and the
 * program resumes (Using III+ U5-160; real III+ 1.1 SF-errors). A handler
 * that executed RETRY returns 2 (re-run the failed statement, U5-182); a
 * handler that cancelled / quit / RETURNed TO MASTER returns that negative
 * code so the executor unwinds. */
static int proc_trap(void *vip, int code)
{
    xb_interp *ip = (xb_interp *)vip;
    proc_state *ps = proc_state_for(ip);
    int rc;
    if (!ps || !ps->has_onerr || ps->in_handler)
        return 0;
    rc = proc_fire_onerror(ip, code);
    if (rc == PROC_RC_RETRY_TRAP || rc == PROC_RC_RETRY)
        return 2;
    if (rc == INTERP_RC_QUIT || rc == INTERP_RC_CANCEL ||
        rc == PROC_RC_RETURN_MASTER)
        return rc;
    return 1;
}

static void p_out(xb_interp *ip, const char *s)
{
    uint32_t n = 0;
    while (s[n] != '\0') n++;
    xb_interp_output(ip, s, n);
}

/*
 * proc_escape: the xb_ctx.on_escape hook (Esc seen between two program
 * statements, SET ESCAPE ON). With an ON ESCAPE trap installed, run it and
 * resume at the next statement (Using III+ U5-160; mint samirctl
 * ESCTRAP.TXT). Otherwise the real III+ 1.1 interrupt dialogue, byte for byte
 * from mint samirctl ESC.TXT / ESCS.TXT (dosbox-x, SET ALTERNATE capture):
 *   *** INTERRUPTED ***
 *   Called from - C:ESC.prg              (one line per active DO level)
 *   Cancel, Ignore, or Suspend? (C, I, or S) Cancel|Ignore|Suspend
 *   Do cancelled | Do suspended
 * Cancel unwinds to the dot prompt, Ignore resumes, Suspend opens a nested
 * dot prompt until RESUME (CANCEL / QUIT there unwind instead). Other keys
 * are ignored (the prompt waits). Audit 2026-10-23-samir L002.
 */
static int proc_escape(void *vip)
{
    xb_interp *ip = (xb_interp *)vip;
    proc_state *ps = proc_state_for(ip);
    samir_pal_t *pal = xb_interp_pal(ip);
    int rc, lvl;
    if (!ps)
        return INTERP_OK;
    if (ps->has_onesc) {
        if (ps->in_esc)
            return INTERP_OK;
        ps->in_esc = 1;
        rc = samir_do(ip, ps->onesc);
        ps->in_esc = 0;
        if (rc == INTERP_RC_QUIT || rc == INTERP_RC_CANCEL)
            return rc;
        return INTERP_OK;                   /* the trap ran; resume */
    }
    p_out(ip, "*** INTERRUPTED ***\r\n");
    for (lvl = ps->do_depth; lvl > 0 && ps->prgname[0]; lvl--) {
        char d[3];
        int drv = (pal && pal->curdrive) ? pal->curdrive(pal) : -1;
        p_out(ip, "Called from - ");
        if (drv >= 'A' && drv <= 'Z') {
            d[0] = (char)drv; d[1] = ':'; d[2] = '\0';
            p_out(ip, d);
        }
        p_out(ip, ps->prgname);
        p_out(ip, ".prg\r\n");
    }
    p_out(ip, "Cancel, Ignore, or Suspend? (C, I, or S) ");
    for (;;) {
        int32_t k = (pal && pal->conin_char) ? pal->conin_char(pal) : -1;
        char c = (char)p_up1((char)k);
        if (k < 0 || c == 'C') {            /* no keyboard left: cancel */
            p_out(ip, "Cancel\r\nDo cancelled\r\n");
            return INTERP_RC_CANCEL;
        }
        if (c == 'I') {
            p_out(ip, "Ignore\r\n");
            return INTERP_OK;
        }
        if (c == 'S') {
            p_out(ip, "Suspend\r\nDo suspended\r\n");
            if (!ps->suspend)
                return INTERP_OK;           /* no dot prompt bound: resume */
            return ps->suspend(ip);         /* RESUME -> OK; CANCEL/QUIT unwind */
        }
    }
}

void proc_set_suspend(xb_interp *ip, int (*fn)(xb_interp *))
{
    proc_state *ps = proc_state_for(ip);
    if (ps) ps->suspend = fn;
}

int proc_register(xb_interp *ip)
{
    /* Registration is per-interpreter construction: drop any state a freed
     * interpreter at the same (reused-arena) address left behind -- its ON
     * ERROR trap must not fire in the new one (initech-x5cc). */
#ifndef PROC_MUTATE_KEEP_STALE
    proc_state *ps = proc_state_for(ip);
    if (!ps)
        return -INTERP_ERR_NOMEM;
    rt_memset(ps, 0, (uint32_t)sizeof(*ps));   /* as proc_state_for's fresh slot */
#endif
    xb_interp_ctx(ip)->on_error = proc_trap;
    xb_interp_ctx(ip)->on_escape = proc_escape;
    return xb_interp_add_cmd_hook(ip, proc_cmd_hook, (void *)0);
}

/*
 * proc_run: run a whole program source. Registers `prg` so DO can find its
 * PROCEDURE blocks, then runs the MAIN body (the slice before the first top-level
 * PROCEDURE) through samir_do. A RETURN in the main body stops it cleanly. If a
 * runtime error fires and an ON ERROR trap is installed, the trap command is run
 * once. Returns INTERP_OK or the (negative) error rc from samir_do.
 */
int proc_run(xb_interp *ip, const char *prg)
{
    proc_state *ps = proc_state_for(ip);
    const char *saved_source;
    int rc;

    if (!ps || !prg)
        return -INTERP_ERR_NOMEM;
    /* The caller registers the hook ONCE (proc_register) after xb_interp_make;
     * proc_run only registers the SOURCE so DO can find PROCEDURE blocks. It does
     * NOT re-add the hook (that would fill the bounded chain across calls).
     * The previous source is restored on return: a command typed at a
     * SUSPENDed program's dot prompt must not orphan that program's
     * PROCEDURE blocks (initech-39e5). */
    saved_source = ps->source;
    ps->source = prg;
    ps->run_depth++;

    /* the main body is everything up to the first top-level PROCEDURE line,
     * run in place (no 8 KiB stack copy; initech-39e5). */
    rc = samir_do_n(ip, prg, p_body_len(prg));
    if (rc == PROC_RC_RETURN || rc == PROC_RC_RETURN_MASTER ||
        rc == PROC_RC_RETRY || rc == INTERP_RC_RETRY)
        rc = INTERP_OK;                     /* a top-level RETURN is clean */
    if (rc == INTERP_RC_CANCEL && ps->run_depth == 1)
        rc = INTERP_OK;                     /* "Do cancelled": back at the dot */

    /* ON ERROR: on a runtime error, fire the installed handler once. */
    if (rc != INTERP_OK && rc != INTERP_RC_QUIT && rc != INTERP_RC_CANCEL &&
        ps->has_onerr && !ps->in_handler) {
        int herr = samir_last_error(ip);
        (void)herr;
        proc_fire_onerror(ip, herr);
    }
    ps->run_depth--;
    if (ps->run_depth > 0)
        ps->source = saved_source;          /* back to the suspended program */
    return rc;
}

/*
 * proc_run_program: DO <name> of a disk program from the dot prompt. Same as
 * proc_run, but the program is a DO level of its own (the top level: real
 * III+ 1.1 counts it, mint samirctl NEST.TXT) and its name feeds the Esc
 * trace "Called from - C:<NAME>.prg" (mint samirctl ESC.TXT).
 */
int proc_run_program(xb_interp *ip, const char *prg, const char *name)
{
    proc_state *ps = proc_state_for(ip);
    char saved_name[P_MAX_PRGNAME];
    int saved_master, rc, i;
    if (!ps || !prg || !name)
        return -INTERP_ERR_NOMEM;
    if (ps->do_depth >= P_MAX_DO_NEST)
        return -INTERP_ERR_DEPTH;           /* a suspended stack is full */
    for (i = 0; i < P_MAX_PRGNAME; i++) saved_name[i] = ps->prgname[i];
    for (i = 0; name[i] != '\0' && i < P_MAX_PRGNAME - 1; i++)
        ps->prgname[i] = p_up1(name[i]);
    ps->prgname[i] = '\0';
    saved_master = ps->master_depth;
    ps->do_depth++;
    ps->master_depth = ps->do_depth;
    rc = proc_run(ip, prg);
    ps->do_depth--;
    ps->master_depth = saved_master;
    for (i = 0; i < P_MAX_PRGNAME; i++) ps->prgname[i] = saved_name[i];
    return rc;
}

/*
 * proc_fire_onerror: run the installed ON ERROR command once (guarded against
 * re-entry). Used by proc_run and available to the REPL (S5.8). Returns the
 * handler's rc, or INTERP_OK if no trap is installed.
 */
int proc_fire_onerror(xb_interp *ip, int err_code)
{
    proc_state *ps = proc_state_for(ip);
    xb_ctx *ctx = xb_interp_ctx(ip);
    int rc, saved_error;
    if (!ps || !ps->has_onerr || ps->in_handler)
        return INTERP_OK;
    ps->in_handler = 1;
    /* Programming III+ N5-30 (PDF 418): ERROR() sees the condition being
     * handled, even while the handler evaluates a new expression. */
    saved_error = ctx->error_number;
    ctx->error_number = err_code;
    ps->handler_depth = ps->do_depth;
    rc = samir_do(ip, ps->onerr);           /* almost always "DO <handler>" */
    if (rc == PROC_RC_RETURN)
        rc = INTERP_OK;
    ps->in_handler = 0;
    /* ERROR()/MESSAGE() keep the handled condition after the handler and
     * later successful commands (real III+ 1.1 SF-errors). RETRY clears it
     * (Using III+ U5-182). */
    (void)saved_error;
    ctx->error_number = (rc == PROC_RC_RETRY_TRAP || rc == PROC_RC_RETRY)
                        ? 0 : err_code;
    return rc;
}
