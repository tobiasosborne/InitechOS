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
#define P_REGISTRY     16    /* concurrent interpreters with proc state */

/* dBASE catalog ordinals (spec/samir/dbase_msg_codes.tsv -- the ENGINE table). */
#define P_MSG_NO_PARAM      93  /* "No PARAMETER statement found." */
#define P_MSG_WRONG_PARAM   94  /* "Wrong number of parameters." */
#define P_MSG_NOT_FOUND     16  /* an unknown DO target -> unrecognized */

/* RETURN sentinel: a negative rc the flow executor bubbles up out of nested
 * control flow; proc_call maps it to a clean return. Chosen WELL outside the
 * interp_err range (1..8) and the 1..151 dBASE catalog so it can never be
 * mistaken for a real error code recorded by samir_last_error. */
#define PROC_RC_RETURN  (-20000)

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
     * A PARAMETERS line in that routine's body binds these positionally. Saved /
     * restored around each nested p_call so recursion is well-defined. */
    xb_val      pending_args[P_MAX_ARGS];
    int         pending_nargs;
    int         params_seen;     /* 1 once a PARAMETERS line ran for this call */
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
    g_val[g_n].source        = (const char *)0;
    g_val[g_n].onerr[0]      = '\0';
    g_val[g_n].has_onerr     = 0;
    g_val[g_n].in_handler    = 0;
    g_val[g_n].pending_nargs = 0;
    g_val[g_n].params_seen   = 0;
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

/* case-insensitive equality of two NUL-terminated strings. */
static int p_ci_eq(const char *a, const char *b)
{
    int i = 0;
    for (;;) {
        char ca = p_up1(a[i]);
        char cb = p_up1(b[i]);
        if (ca != cb) return 0;
        if (ca == '\0') return 1;
        i++;
    }
}

/* skip leading blanks; return pointer past them. */
static const char *p_skip_ws(const char *s)
{
    while (*s != '\0' && p_isspace(*s)) s++;
    return s;
}

/*
 * p_ci_eq_prefix: does the text at `s` (bounded above by `le`) begin with the
 * keyword `kw` as a WHOLE WORD (kw matched case-insensitively, then end-of-line
 * or a blank)? Used to spot a "PROCEDURE <name>" line. Returns 1/0.
 */
static int p_ci_eq_prefix(const char *s, const char *kw, const char *le)
{
    int i = 0;
    while (kw[i] != '\0') {
        if (s + i >= le) return 0;
        if (p_up1(s[i]) != p_up1(kw[i])) return 0;
        i++;
    }
    /* the char after the keyword must be end-of-line or a blank. */
    if (s + i >= le) return 1;
    return p_isspace(s[i]);
}

/* ===================================================================== */
/* Expression evaluation through the interp ctx (INPUT + a prompt expC)    */
/* ===================================================================== */

/*
 * p_eval: lex+parse+eval the NUL-terminated `expr` through ip's ctx (which carries
 * flow's composed memvar+field resolver). 0 on success with *out; else negative
 * interp_err with *ec the catalog ordinal / stage detail.
 */
static int p_eval(xb_interp *ip, const char *expr, xb_val *out, int *ec)
{
    xb_token toks[P_MAX_TOKS];
    xb_node  pool[P_MAX_NODES];
    xb_ctx  *ctx = xb_interp_ctx(ip);
    int ntok, root, rc, e = 0;
    uint32_t len = 0u;

    while (expr[len] != '\0') len++;

    ntok = xb_lex(expr, len, toks, (uint32_t)P_MAX_TOKS, &e);
    if (ntok < 0) { if (ec) *ec = e; return -INTERP_ERR_LEX; }

    root = xb_parse(toks, (uint32_t)ntok, pool, (uint32_t)P_MAX_NODES, &e);
    if (root < 0) { if (ec) *ec = e; return -INTERP_ERR_PARSE; }

    rc = xb_eval(pool, root, ctx, out, &e);
    if (rc != 0) { if (ec) *ec = e; return -INTERP_ERR_EVAL; }

    if (ec) *ec = 0;
    return INTERP_OK;
}

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
    samir_pal_t *pal = xb_interp_pal(ip);
    const char *p = p_skip_ws(pexpr);
    if (*p == '\0') { if (ec) *ec = 0; return INTERP_OK; }   /* no prompt */
    rc = p_eval(ip, p, &v, ec);
    if (rc != INTERP_OK)
        return rc;
    if (v.t != XB_C && v.t != XB_M) {
        if (ec) *ec = XBEE_MISMATCH;        /* prompt must be Character (#9) */
        return -INTERP_ERR_EVAL;
    }
    if (pal && pal->conout && v.u.c.len > 0u && v.u.c.p)
        pal->conout(pal, v.u.c.p, (uint32_t)v.u.c.len);
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
    n = pal->conin_line(pal, buf, cap);
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
    rc = p_eval(ip, line, &v, ec);
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
            const char *nm = p_skip_ws(t + 9);   /* past "PROCEDURE" */
            /* compare the name token (whole word) against `name`. */
            int i = 0;
            while (nm + i < le && name[i] != '\0' &&
                   p_up1(nm[i]) == p_up1(name[i]))
                i++;
            if (name[i] == '\0') {
                const char *after = nm + i;
                if (after >= le || p_isspace(*after)) {
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
 * p_extract_body: copy the procedure body starting at `body` up to (but not
 * including) the next top-level PROCEDURE line, into dst[cap] NUL-terminated.
 * RETURN lines are LEFT IN (the RETURN hook stops the body cleanly). Returns 0,
 * or -1 on overflow (fail loud at the caller).
 */
static int p_extract_body(const char *body, char *dst, uint32_t cap)
{
    const char *p = body;
    uint32_t o = 0;
    while (*p != '\0') {
        const char *ls = p;
        const char *le = ls;
        const char *t;
        while (*le != '\0' && *le != '\n') le++;
        t = p_skip_ws(ls);
        if (le > t && p_up1(t[0]) == 'P' &&
            p_ci_eq_prefix(t, "PROCEDURE", le))
            break;                          /* next PROCEDURE: body ends here */
        /* copy this physical line + its LF. */
        while (ls < le) {
            if (o >= cap - 1u) return -1;
            dst[o++] = *ls++;
        }
        if (*le == '\n') {
            if (o >= cap - 1u) return -1;
            dst[o++] = '\n';
            p = le + 1;
        } else {
            p = le;
        }
    }
    dst[o] = '\0';
    return 0;
}

/* a procedure body can be large; size for a generous routine. */
#define P_BODY_CAP 8192

/*
 * p_call: run the procedure `name` with the (already evaluated) WITH args.
 * Enters a scope, runs the body via samir_do, leaves the scope. Maps the RETURN
 * sentinel to a clean return. Returns INTERP_OK or a negative rc (with *ec set).
 */
static int p_call(xb_interp *ip, proc_state *ps, const char *name,
                  xb_val *args, int nargs, int *ec)
{
    const char *body;
    /* The body TEXT is extracted into a STACK buffer (NOT static): a PROCEDURE
     * that DOes another recurses through p_call, and a single static buffer would
     * be clobbered by the nested extraction. The stack copy makes p_call
     * re-entrant to FLOW_MAX_SCOPE depth (each frame ~8 KiB). */
    char stackbody[P_BODY_CAP];
    int rc;

    body = p_find_procedure(ps->source, name);
    if (!body) {
        if (ec) *ec = P_MSG_NOT_FOUND;      /* unknown DO target */
        return -INTERP_ERR_UNKNOWN_CMD;
    }
    if (p_extract_body(body, stackbody, sizeof(stackbody)) != 0) {
        if (ec) *ec = 0;
        return -INTERP_ERR_DEPTH;           /* procedure body too large */
    }

    /* push a DO-call scope; bind PARAMETERS happens inside the body via the
     * PARAMETERS verb, which reads the pending arg vector below. */
    rc = xb_interp_scope_enter(ip);
    if (rc < 0) { if (ec) *ec = 0; return rc; }   /* DO nested too deep */

    /* stash the pending WITH args so a PARAMETERS line in the body can bind them
     * positionally (proc_state-level, since the body runs via a fresh samir_do). */
    {
        int saved_n = ps->pending_nargs;
        xb_val saved_args[P_MAX_ARGS];
        int saved_seen = ps->params_seen;
        int i;
        for (i = 0; i < saved_n && i < P_MAX_ARGS; i++)
            saved_args[i] = ps->pending_args[i];

        for (i = 0; i < nargs && i < P_MAX_ARGS; i++)
            ps->pending_args[i] = args[i];
        ps->pending_nargs = nargs;
        ps->params_seen   = 0;

        rc = samir_do(ip, stackbody);
        if (rc == PROC_RC_RETURN)
            rc = INTERP_OK;                 /* a clean RETURN out of the body */

        /* DO ... WITH to a routine with NO PARAMETERS line is error #93. */
        if (rc == INTERP_OK && nargs > 0 && !ps->params_seen) {
            rc  = -INTERP_ERR_SYNTAX;
            if (ec) *ec = P_MSG_NO_PARAM;
        }

        /* restore the caller's pending-arg context. */
        for (i = 0; i < saved_n && i < P_MAX_ARGS; i++)
            ps->pending_args[i] = saved_args[i];
        ps->pending_nargs = saved_n;
        ps->params_seen   = saved_seen;
    }

    /* leave the scope: release this routine's PRIVATE/PARAMETERS vars, restore
     * anything they shadowed. (Done even on an error -- the RETURN sentinel was
     * already mapped to INTERP_OK above.) */
    xb_interp_scope_leave(ip);

    return rc;                              /* INTERP_OK or a real error */
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
    int nargs = 0;
    uint32_t nlen;

    nlen = p_take_name(p, name);
    if (nlen == 0u) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
    p += nlen;
    p = p_skip_ws(p);

    /* optional WITH <expr>[, <expr>...] */
    if (p_up1(p[0]) == 'W' && p_up1(p[1]) == 'I' && p_up1(p[2]) == 'T' &&
        p_up1(p[3]) == 'H' && (p[4] == '\0' || p_isspace(p[4]))) {
        p = p_skip_ws(p + 4);
        /* split the comma list at the top level (outside string literals/parens)
         * and evaluate each argument expression by value. */
        while (*p != '\0') {
            char ebuf[P_MAX_EXPR + 1];
            uint32_t eo = 0;
            int in_s = 0, in_d = 0, in_b = 0, depth = 0;
            xb_val v;
            int rc;
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
                if (eo >= (uint32_t)P_MAX_EXPR) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
                ebuf[eo++] = c;
                p++;
            }
            while (eo > 0u && p_isspace(ebuf[eo - 1])) eo--;
            ebuf[eo] = '\0';
            if (nargs >= P_MAX_ARGS) { if (ec) *ec = P_MSG_WRONG_PARAM; return -INTERP_ERR_SYNTAX; }
            rc = p_eval(ip, ebuf, &v, ec);
            if (rc != INTERP_OK)
                return rc;
            argv[nargs++] = v;
            if (*p == ',') { p++; p = p_skip_ws(p); continue; }
            break;
        }
    }

    return p_call(ip, ps, name, argv, nargs, ec);
}

/* ===================================================================== */
/* ON ERROR <command> | ON ERROR (clear)                                  */
/* ===================================================================== */

static int p_on(xb_interp *ip, proc_state *ps, const char *args, int *ec)
{
    const char *p = p_skip_ws(args);
    char w[16];
    uint32_t n = 0;
    (void)ip;

    /* the ON option keyword: ERROR / ESCAPE / KEY. We implement ERROR; ESCAPE /
     * KEY parse-and-store as no-op traps (they never fire in the host oracle). */
    while (p[n] != '\0' && !p_isspace(p[n]) && n < (uint32_t)(sizeof(w) - 1)) {
        w[n] = p_up1(p[n]); n++;
    }
    w[n] = '\0';
    p = p_skip_ws(p + n);

    if (p_ci_eq(w, "ERROR")) {
        if (*p == '\0') {
            ps->has_onerr = 0;              /* ON ERROR (bare) clears the trap */
            ps->onerr[0]  = '\0';
        } else {
            uint32_t i = 0;
            while (p[i] != '\0' && i < (uint32_t)P_MAX_ONERR) { ps->onerr[i] = p[i]; i++; }
            ps->onerr[i]  = '\0';
            ps->has_onerr = 1;
        }
        if (ec) *ec = 0;
        return INTERP_OK;
    }
    if (p_ci_eq(w, "ESCAPE") || p_ci_eq(w, "KEY")) {
        /* parsed + accepted; ESCAPE/KEY traps never fire in a scripted host run.
         * (programming-and-io.md sec 14 -- precedence ERROR>ESCAPE>KEY.) */
        if (ec) *ec = 0;
        return INTERP_OK;
    }
    if (ec) *ec = 0;
    return -INTERP_ERR_SYNTAX;              /* unknown ON option */
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

    if (p_ci_eq(verb, "DO")) {
        /* DO WHILE / DO CASE are the flow spine's (handled before the hook); only
         * DO <name> reaches here. */
        int rc = p_do(ip, ps, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (p_ci_eq(verb, "PROCEDURE")) {
        /* A PROCEDURE line reached during execution (e.g. trailing the main body
         * if proc_run is bypassed) ENDS the current routine -- treat it as a
         * clean RETURN so we never fall into the next routine's body. */
        return PROC_RC_RETURN;
    }
    if (p_ci_eq(verb, "PARAMETERS") || p_ci_eq(verb, "PARAMETER")) {
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
    if (p_ci_eq(verb, "RETURN")) {
        /* RETURN [TO MASTER] -- we treat both as a single-frame clean return
         * (TO MASTER full unwind is not modelled; the body stops either way). */
        return PROC_RC_RETURN;
    }
    if (p_ci_eq(verb, "PUBLIC")) {
        int rc = p_for_each_name(ip, args, p_each_public, (void *)0, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (p_ci_eq(verb, "PRIVATE")) {
        int rc = p_for_each_name(ip, args, p_each_private, (void *)0, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (p_ci_eq(verb, "ACCEPT")) {
        int rc = p_accept(ip, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (p_ci_eq(verb, "INPUT")) {
        int rc = p_input(ip, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (p_ci_eq(verb, "WAIT")) {
        int rc = p_wait(ip, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }
    if (p_ci_eq(verb, "ON")) {
        int rc = p_on(ip, ps, args, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    return CMD_UNKNOWN;                      /* not a proc/IO verb */
}

/* ===================================================================== */
/* Public registration + the proc_run driver                              */
/* ===================================================================== */

int proc_register(xb_interp *ip);
int proc_run(xb_interp *ip, const char *prg);
int proc_fire_onerror(xb_interp *ip, int err_code);

int proc_register(xb_interp *ip)
{
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
    char mainbody[P_BODY_CAP];
    int rc;

    if (!ps || !prg)
        return -INTERP_ERR_NOMEM;
    /* The caller registers the hook ONCE (proc_register) after xb_interp_make;
     * proc_run only registers the SOURCE so DO can find PROCEDURE blocks. It does
     * NOT re-add the hook (that would fill the bounded chain across calls). */
    ps->source = prg;

    /* the main body is everything up to the first top-level PROCEDURE line. */
    if (p_extract_body(prg, mainbody, sizeof(mainbody)) != 0)
        return -INTERP_ERR_DEPTH;

    rc = samir_do(ip, mainbody);
    if (rc == PROC_RC_RETURN)
        rc = INTERP_OK;                     /* a top-level RETURN is clean */

    /* ON ERROR: on a runtime error, fire the installed handler once. */
    if (rc != INTERP_OK && ps->has_onerr && !ps->in_handler) {
        int herr = samir_last_error(ip);
        (void)herr;
        proc_fire_onerror(ip, herr);
    }
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
    int rc;
    (void)err_code;
    if (!ps || !ps->has_onerr || ps->in_handler)
        return INTERP_OK;
    ps->in_handler = 1;
    rc = samir_do(ip, ps->onerr);           /* almost always "DO <handler>" */
    if (rc == PROC_RC_RETURN)
        rc = INTERP_OK;
    ps->in_handler = 0;
    return rc;
}
