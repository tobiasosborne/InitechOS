/*
 * os/samir/cmd/flow.c -- SAMIR (InitechBase) statement executor + control flow.
 *                        Step S5.3 (Phase 5 interpreter SPINE).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding (-ffreestanding -nostdlib). Uses
 * ONLY <stdint.h> and the samir/ engine headers. No libc, no int 0x21 -- all OS
 * contact is through the PAL carried by the interpreter. Memory is bump-allocated
 * from the PAL arena (no malloc). ASCII-clean (Rule 12). Fail loud (Rule 2).
 * Reproducible (Rule 11): pure function of (program text, interp state).
 *
 * dBASE III PLUS 1.1 ONLY (plan Sec 2.C).
 *
 * WHAT THIS IS (the S5.3 contract; see interp.h for the full doc):
 *   samir_do(ip, prg) is the dot-prompt interpreter's STATEMENT layer. It splits
 *   the program text into logical lines, dispatches each command line on its
 *   leading verb, and runs the structured control-flow constructs:
 *       DO WHILE <logical> ... [LOOP] [EXIT] ... ENDDO
 *       IF <logical> ... [ELSE ...] ENDIF
 *       DO CASE  CASE <logical> ... [CASE ...] [OTHERWISE ...] ENDCASE
 *   plus the memvar verbs STORE <expr> TO <name> and "<name> = <expr>".
 *   Other verbs go to a registered command HOOK (S5.4..S5.7 extension point).
 *
 * GUARD-MUST-BE-LOGICAL: the condition of IF / DO WHILE / each DO CASE CASE is
 *   evaluated and MUST be Logical (XB_L). A non-Logical guard is fail-loud error
 *   #37 "Not a Logical expression." (XBEE_NOT_LOGICAL). NO truthiness.
 *
 * MEMVARS off workarea.c: at samir_do entry we save the ctx's work-area
 *   resolve/user and install a COMPOSED resolver (memvars first, else delegate);
 *   restored on return. Every other ctx field is left untouched.
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.3 contract + Phase 5 header.
 *   - os/samir/include/samir/interp.h  (the S5.3 API this file implements).
 *   - os/samir/include/samir/eval.h    (xb_lex/xb_parse/xb_eval; xb_ctx.resolve;
 *                                       XBEE_NOT_LOGICAL = #37).
 *   - os/samir/include/samir/workarea.h(wa_resolve -- the delegate).
 *   - spec/samir/dbase_msg_codes.tsv   (#16 unrecognized verb, #37 not logical).
 */

#include <stdint.h>

#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"
#include "samir/pal.h"

/* ===================================================================== */
/* Tunables (bounded -- Rule 2 fail-loud on overflow, never UB)           */
/* ===================================================================== */

#define FLOW_MAX_LINES     512   /* logical lines per program */
#define FLOW_MAX_MEMVARS   256   /* live memory variables */
#define FLOW_MEMVAR_ARENA  16384u/* bytes for memvar names + C/M payloads */
#define FLOW_MAX_NAME      11    /* dBASE memvar name: 10 chars + NUL */
#define FLOW_MAX_TOKS      128   /* per-expression token pool */
#define FLOW_MAX_NODES     128   /* per-expression node pool */
#define FLOW_MAX_LINELEN   254   /* a working copy of one line (dBASE cap 254) */
#define FLOW_MAX_REGISTRY  16    /* concurrent interpreters with flow state */

/* dBASE catalog ordinal used by the spine (not in eval.h). */
#define FLOW_MSG_UNRECOGNIZED 16  /* "Unrecognized command verb." */

/* ===================================================================== */
/* Flow state -- per interpreter, kept OUT of the opaque xb_interp struct  */
/* (workarea.c owns that file). A small static registry maps an xb_interp* */
/* to its flow_state, allocated lazily from the interp's PAL arena. The    */
/* engine is single-threaded cooperative; the registry is bounded + static.*/
/* ===================================================================== */

typedef struct {
    char     name[FLOW_MAX_NAME];  /* upper-cased, NUL-terminated */
    xb_val   val;                  /* C/M bytes live in the memvar arena */
    int      live;                 /* 0 = released slot */
} memvar;

typedef struct {
    /* memvar table */
    memvar    vars[FLOW_MAX_MEMVARS];
    int       nvars;               /* high-water count (slots may be !live) */
    char     *arena;               /* memvar byte arena (names copies + C/M) */
    uint32_t  arena_cap;
    uint32_t  arena_used;

    /* the registered command-hook CHAIN (S5.4..S5.7 extension point). Each
     * module appends one hook via xb_interp_add_cmd_hook; exec_command tries
     * them in order, each returning CMD_UNKNOWN to pass to the next. */
    xb_cmd_hook hooks[INTERP_MAX_CMD_HOOKS];
    void       *hook_users[INTERP_MAX_CMD_HOOKS];
    int         nhooks;

    /* composed resolver bookkeeping: the saved work-area resolver */
    int  (*saved_resolve)(void *user, const char *name, uint16_t len, xb_val *out);
    void  *saved_user;

    int       last_error;          /* dBASE ordinal / interp_err of last run */
} flow_state;

/* The registry. */
static xb_interp  *g_reg_key[FLOW_MAX_REGISTRY];
static flow_state *g_reg_val[FLOW_MAX_REGISTRY];
static int         g_reg_n;

/* flow_lookup: find the flow_state for `ip`, or NULL. */
static flow_state *flow_lookup(xb_interp *ip)
{
    int i;
    for (i = 0; i < g_reg_n; i++) {
        if (g_reg_key[i] == ip)
            return g_reg_val[i];
    }
    return (flow_state *)0;
}

/* flow_reset: clear a flow_state's memvar table + hook to the fresh-interp state. */
static void flow_reset(flow_state *fs)
{
    int i;
    fs->nvars        = 0;
    fs->arena_cap    = FLOW_MEMVAR_ARENA;
    fs->arena_used   = 0u;
    fs->nhooks       = 0;
    for (i = 0; i < INTERP_MAX_CMD_HOOKS; i++) {
        fs->hooks[i]      = (xb_cmd_hook)0;
        fs->hook_users[i] = (void *)0;
    }
    fs->saved_resolve = (int (*)(void *, const char *, uint16_t, xb_val *))0;
    fs->saved_user   = (void *)0;
    fs->last_error   = 0;
    for (i = 0; i < FLOW_MAX_MEMVARS; i++)
        fs->vars[i].live = 0;
}

/*
 * flow_state_for: get-or-create the flow_state for `ip`.
 *
 * The flow_state + memvar arena live in a STATIC pool (not the interp's PAL
 * arena) so they are never clobbered by an arena reset on xb_interp_free, and so
 * pointer-reuse (a remade interp at the same address) is handled deterministically
 * below. Static BSS is freestanding-legal (no malloc); the pool is bounded.
 *
 * Pointer-reuse / fresh-interp detection (the load-bearing correctness point):
 *   A freshly xb_interp_make'd interp has ctx->resolve == wa_resolve (set by
 *   wa_bind_ctx). Once this file drives it (samir_do / a memvar store), we install
 *   flow_resolve and LEAVE it installed -- it delegates to the saved wa_resolve
 *   for fields, so field resolution is unchanged, and memvars persist across
 *   multiple samir_do calls on the SAME interp. Therefore:
 *     - ctx->resolve == wa_resolve  -> this is a pristine interp (first use) OR a
 *       reused address for a NEW interp -> RESET the flow_state (drop stale memvars).
 *     - ctx->resolve == flow_resolve -> mid-lifetime continuation -> KEEP it.
 *   This distinguishes "second samir_do on the same interp" (keep) from "a remade
 *   interp at the same pointer" (reset) without touching the opaque xb_interp.
 *
 * Returns NULL on registry overflow (fail loud at the call site).
 */
static int flow_resolve(void *user, const char *name, uint16_t len, xb_val *out);

static flow_state *flow_state_for(xb_interp *ip)
{
    static flow_state s_pool[FLOW_MAX_REGISTRY];
    static char       s_arena[FLOW_MAX_REGISTRY][FLOW_MEMVAR_ARENA];
    flow_state *fs;
    xb_ctx *ctx;
    int slot;

    if (!ip)
        return (flow_state *)0;

    ctx = xb_interp_ctx(ip);

    fs = flow_lookup(ip);
    if (fs) {
        /* a HIT: reset iff the interp looks pristine/remade (resolve restored to
         * the work-area resolver), else keep it (memvars persist within a life). */
        if (ctx && ctx->resolve != flow_resolve) {
            char *keep_arena = fs->arena;
            flow_reset(fs);
            fs->arena = keep_arena;
        }
        return fs;
    }

    if (g_reg_n >= FLOW_MAX_REGISTRY)
        return (flow_state *)0;     /* too many live interpreters: fail loud */

    slot = g_reg_n;
    fs = &s_pool[slot];
    fs->arena = s_arena[slot];
    flow_reset(fs);

    g_reg_key[g_reg_n] = ip;
    g_reg_val[g_reg_n] = fs;
    g_reg_n++;
    return fs;
}

/* ===================================================================== */
/* Small ASCII helpers (freestanding; no libc)                            */
/* ===================================================================== */

static char fl_up1(char c)
{
    if (c >= 'a' && c <= 'z')
        return (char)(c - 'a' + 'A');
    return c;
}

static int fl_isspace(char c)
{
    return c == ' ' || c == '\t';
}

/* case-insensitive equality of two NUL-terminated strings. */
static int fl_ci_eq(const char *a, const char *b)
{
    uint32_t i = 0;
    for (;;) {
        char ca = fl_up1(a[i]);
        char cb = fl_up1(b[i]);
        if (ca != cb)
            return 0;
        if (ca == '\0')
            return 1;
        i++;
    }
}

/* case-insensitive equality of NUL-terminated `a` vs byte-slice (b,blen). */
static int fl_ci_eq_n(const char *a, const char *b, uint32_t blen)
{
    uint32_t i;
    for (i = 0; i < blen; i++) {
        if (a[i] == '\0')
            return 0;
        if (fl_up1(a[i]) != fl_up1(b[i]))
            return 0;
    }
    return a[i] == '\0';
}

/* ===================================================================== */
/* Memvar table                                                           */
/* ===================================================================== */

/* memvar_find: index of a live memvar named `name`(NUL-term), or -1. */
static int memvar_find(flow_state *fs, const char *name)
{
    int i;
    for (i = 0; i < fs->nvars; i++) {
        if (fs->vars[i].live && fl_ci_eq(fs->vars[i].name, name))
            return i;
    }
    return -1;
}

/* memvar_find_n: index of a live memvar matching the byte-slice (name,len). */
static int memvar_find_n(flow_state *fs, const char *name, uint32_t len)
{
    int i;
    for (i = 0; i < fs->nvars; i++) {
        if (fs->vars[i].live && fl_ci_eq_n(fs->vars[i].name, name, len))
            return i;
    }
    return -1;
}

/*
 * memvar_set: create-or-update memvar `name` with value `v`. C/M bytes are copied
 * into the memvar arena (so the memvar outlives the eval scratch). Returns
 * INTERP_OK or a negative interp_err.
 */
static int memvar_set(flow_state *fs, const char *name, const xb_val *v)
{
    int slot, i;
    uint32_t nlen;

    /* validate the name length (dBASE memvar names are <= 10 chars). */
    for (nlen = 0; name[nlen] != '\0'; nlen++) { }
    if (nlen == 0u || nlen >= FLOW_MAX_NAME)
        return -INTERP_ERR_SYNTAX;

    slot = memvar_find(fs, name);
    if (slot < 0) {
        /* reuse a released slot if any, else extend. */
        for (i = 0; i < fs->nvars; i++) {
            if (!fs->vars[i].live) { slot = i; break; }
        }
        if (slot < 0) {
            if (fs->nvars >= FLOW_MAX_MEMVARS)
                return -INTERP_ERR_DEPTH;
            slot = fs->nvars++;
        }
        /* copy the (upper-cased) name into the slot. */
        for (i = 0; (uint32_t)i < nlen; i++)
            fs->vars[slot].name[i] = fl_up1(name[i]);
        fs->vars[slot].name[nlen] = '\0';
        fs->vars[slot].live = 1;
    }

    /* copy the value; for C/M, copy the bytes into the memvar arena. */
    if (v->t == XB_C || v->t == XB_M) {
        uint16_t len = v->u.c.len;
        char *dst;
        if ((uint32_t)len > fs->arena_cap - fs->arena_used)
            return -INTERP_ERR_NOMEM;
        dst = fs->arena + fs->arena_used;
        if (len > 0u && v->u.c.p)
            rt_memcpy(dst, v->u.c.p, (uint32_t)len);
        fs->arena_used += (uint32_t)len;
        fs->vars[slot].val.t = v->t;
        fs->vars[slot].val.u.c.p = dst;
        fs->vars[slot].val.u.c.len = len;
    } else {
        fs->vars[slot].val = *v;
    }
    return INTERP_OK;
}

/* ===================================================================== */
/* The COMPOSED resolver: memvars first, else the saved work-area resolver */
/* ===================================================================== */

/*
 * flow_resolve: the xb_ctx.resolve hook installed for the duration of samir_do.
 * `user` is the flow_state*. Checks the memvar table first (memvars shadow
 * fields by dBASE precedence in a program context? -- dBASE actually resolves
 * FIELDS before memvars when a table is open; but a name that is a memvar and NOT
 * a field must still resolve. We check the field delegate FIRST so that an open
 * table's field wins on a name collision, matching dBASE's field-precedence, then
 * fall back to memvars). If the delegate binds the name, use it; else look up the
 * memvar table; else unbound.
 */
static int flow_resolve(void *user, const char *name, uint16_t len, xb_val *out)
{
    flow_state *fs = (flow_state *)user;
    int slot;

    if (!fs || !out)
        return 1;

    /* 1) fields of the selected open table win (dBASE field precedence). */
    if (fs->saved_resolve) {
        if (fs->saved_resolve(fs->saved_user, name, len, out) == 0)
            return 0;
    }

    /* 2) otherwise, a memory variable. */
    slot = memvar_find_n(fs, name, (uint32_t)len);
    if (slot >= 0) {
        *out = fs->vars[slot].val;
        return 0;
    }

    return 1;  /* unbound */
}

/* ===================================================================== */
/* Expression evaluation through the (already installed) ctx              */
/* ===================================================================== */

/*
 * eval_expr: lex+parse+eval the NUL-terminated expression `expr` through ip's
 * ctx (which carries flow_resolve while samir_do runs). On success returns
 * INTERP_OK with *out set; else a negative interp_err with *ec the stage detail.
 */
static int eval_expr(xb_interp *ip, const char *expr, xb_val *out, int *ec)
{
    xb_token toks[FLOW_MAX_TOKS];
    xb_node  pool[FLOW_MAX_NODES];
    xb_ctx  *ctx = xb_interp_ctx(ip);
    uint32_t len = 0u;
    int ntok, root, rc, e = 0;

    while (expr[len] != '\0')
        len++;

    ntok = xb_lex(expr, len, toks, (uint32_t)FLOW_MAX_TOKS, &e);
    if (ntok < 0) { if (ec) *ec = e; return -INTERP_ERR_LEX; }

    root = xb_parse(toks, (uint32_t)ntok, pool, (uint32_t)FLOW_MAX_NODES, &e);
    if (root < 0) { if (ec) *ec = e; return -INTERP_ERR_PARSE; }

    rc = xb_eval(pool, root, ctx, out, &e);
    if (rc != 0) { if (ec) *ec = e; return -INTERP_ERR_EVAL; }

    if (ec) *ec = 0;
    return INTERP_OK;
}

/*
 * eval_guard: evaluate `expr` and REQUIRE a Logical result. *truth gets 0/1.
 * A non-Logical result is fail-loud error #37 (XBEE_NOT_LOGICAL): returns
 * -INTERP_ERR_EVAL with *ec = XBEE_NOT_LOGICAL. THE headline S5.3 rule -- no
 * truthiness. (plan S5.3; dbase_msg_codes.tsv #37.)
 */
static int eval_guard(xb_interp *ip, const char *expr, int *truth, int *ec)
{
    xb_val v;
    int rc = eval_expr(ip, expr, &v, ec);
    if (rc != INTERP_OK)
        return rc;
#ifdef FLOW_MUTATE_GUARD_TRUTHY
    /* MUTANT (Rule 6 -- -DFLOW_MUTATE_GUARD_TRUTHY): accept a non-Logical guard
     * as truthiness (N != 0, non-empty C) instead of failing loud #37. This
     * breaks the guard-must-be-Logical rule -> the oracle's guard-type-error
     * checks go RED. */
    switch (v.t) {
    case XB_L: *truth = v.u.l ? 1 : 0; break;
    case XB_N: *truth = (v.u.n != 0.0) ? 1 : 0; break;
    case XB_C: *truth = (v.u.c.len > 0u) ? 1 : 0; break;
    default:   *truth = 0; break;
    }
    if (ec) *ec = 0;
    return INTERP_OK;
#else
    if (v.t != XB_L) {
        if (ec) *ec = XBEE_NOT_LOGICAL;     /* #37 "Not a Logical expression." */
        return -INTERP_ERR_EVAL;
    }
    *truth = v.u.l ? 1 : 0;
    if (ec) *ec = 0;
    return INTERP_OK;
#endif
}

/* ===================================================================== */
/* Line model: split the program into trimmed logical lines               */
/* ===================================================================== */

typedef struct {
    const char *p;     /* start of the trimmed line content (into source) */
    uint32_t    len;   /* trimmed byte length (no leading/trailing blanks) */
} flow_line;

/*
 * is_comment_line: a dBASE comment line begins with '*' (after leading blanks) or
 * with the "NOTE" keyword. A "&&" inline comment is stripped by trim_line.
 */
static int is_comment_line(const char *p, uint32_t len)
{
    if (len == 0u)
        return 1;                       /* blank line: skip */
    if (p[0] == '*')
        return 1;
    if (len >= 4u && fl_up1(p[0]) == 'N' && fl_up1(p[1]) == 'O' &&
        fl_up1(p[2]) == 'T' && fl_up1(p[3]) == 'E' &&
        (len == 4u || fl_isspace(p[4])))
        return 1;
    return 0;
}

/*
 * split_lines: fill `lines[]` with the program's non-blank, non-comment logical
 * lines. Inline "&&" comments are dropped; surrounding blanks are trimmed; '&&'
 * inside a string literal is preserved. Returns the line count, or -1 if the
 * program exceeds FLOW_MAX_LINES (fail loud).
 */
static int split_lines(const char *prg, flow_line *lines, int cap)
{
    int n = 0;
    uint32_t i = 0;

    while (prg[i] != '\0') {
        uint32_t start = i, end;
        int in_squote = 0, in_dquote = 0, in_bracket = 0;
        uint32_t content_end = 0;  /* end before any '&&' inline comment */
        int saw_comment = 0;
        uint32_t j;

        /* find the end of this physical line (LF) tracking string state. */
        while (prg[i] != '\0' && prg[i] != '\n') {
            char c = prg[i];
            if (!saw_comment) {
                if (in_squote) {
                    if (c == '\'') in_squote = 0;
                } else if (in_dquote) {
                    if (c == '"') in_dquote = 0;
                } else if (in_bracket) {
                    if (c == ']') in_bracket = 0;
                } else if (c == '\'') {
                    in_squote = 1;
                } else if (c == '"') {
                    in_dquote = 1;
                } else if (c == '[') {
                    in_bracket = 1;
                } else if (c == '&' && prg[i + 1] == '&') {
                    saw_comment = 1;
                    content_end = i;    /* comment starts here */
                }
            }
            i++;
        }
        end = saw_comment ? content_end : i;
        if (prg[i] == '\n')
            i++;                        /* consume the LF */

        /* trim trailing CR + blanks. */
        while (end > start &&
               (prg[end - 1] == '\r' || fl_isspace(prg[end - 1])))
            end--;
        /* trim leading blanks. */
        j = start;
        while (j < end && fl_isspace(prg[j]))
            j++;

        if (is_comment_line(prg + j, end - j))
            continue;                   /* blank or comment: skip */

        if (n >= cap)
            return -1;                  /* program too large: fail loud */
        lines[n].p   = prg + j;
        lines[n].len = end - j;
        n++;
    }
    return n;
}

/*
 * copy_line: copy line `ln` into `buf` (cap bytes) as a NUL-terminated working
 * string. Returns 0, or -1 if it would overflow (fail loud).
 */
static int copy_line(const flow_line *ln, char *buf, uint32_t cap)
{
    if (ln->len + 1u > cap)
        return -1;
    rt_memcpy(buf, ln->p, ln->len);
    buf[ln->len] = '\0';
    return 0;
}

/* ===================================================================== */
/* Verb extraction                                                        */
/* ===================================================================== */

/*
 * split_verb: from a NUL-terminated trimmed line, copy the leading word
 * (upper-cased) into `verb` (vcap bytes) and set *args to the remainder (the
 * pointer past the verb and the following blanks). Verb chars are letters; a
 * '?'/'??' verb is taken whole. Returns the verb length.
 */
static uint32_t split_verb(const char *line, char *verb, uint32_t vcap,
                           const char **args)
{
    uint32_t i = 0, v = 0;

    /* skip leading blanks (caller trimmed, but be safe). */
    while (line[i] != '\0' && fl_isspace(line[i]))
        i++;

    /* the '?' / '??' display verbs are punctuation, not letters. */
    if (line[i] == '?') {
        if (v < vcap - 1) verb[v++] = '?';
        i++;
        if (line[i] == '?') {
            if (v < vcap - 1) verb[v++] = '?';
            i++;
        }
    } else {
        while (line[i] != '\0' && !fl_isspace(line[i]) &&
               line[i] != '=' ) {
            /* '=' terminates the verb so "X=1" splits as verb "X" -- but an
             * assignment is detected before this is called; here a verb is an
             * alpha run. Stop at the first non-alnum/underscore too. */
            char c = line[i];
            int wordch = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                         (c >= '0' && c <= '9') || c == '_';
            if (!wordch)
                break;
            if (v < vcap - 1)
                verb[v++] = fl_up1(c);
            i++;
        }
    }
    verb[v] = '\0';

    while (line[i] != '\0' && fl_isspace(line[i]))
        i++;
    *args = line + i;
    return v;
}

/* ===================================================================== */
/* Assignment detection: "<name> = <expr>"                                */
/* ===================================================================== */

/*
 * detect_assign: if `line` is a memvar assignment "<name> = <expr>", write the
 * name (upper-cased, NUL-terminated) into `name` (ncap) and set *rhs to the
 * expression (past '='). Returns 1 if it is an assignment, 0 otherwise.
 *
 * A bare identifier followed by a single '=' (not "==", and not a relational at
 * top level) is an assignment. We require: an identifier (letter/underscore start)
 * then blanks then exactly one '=' that is NOT followed by '=' (== is a lex error
 * anyway) and not preceded by < > (handled because we stop at '=').
 */
static int detect_assign(const char *line, char *name, uint32_t ncap,
                         const char **rhs)
{
    uint32_t i = 0, n = 0;
    char c0;

    while (line[i] != '\0' && fl_isspace(line[i]))
        i++;

    c0 = line[i];
    if (!((c0 >= 'A' && c0 <= 'Z') || (c0 >= 'a' && c0 <= 'z') || c0 == '_'))
        return 0;                       /* not an identifier start */

    while (line[i] != '\0') {
        char c = line[i];
        int wordch = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') || c == '_';
        if (!wordch)
            break;
        if (n < ncap - 1)
            name[n++] = fl_up1(c);
        i++;
    }
    name[n] = '\0';

    while (line[i] != '\0' && fl_isspace(line[i]))
        i++;

    if (line[i] != '=')
        return 0;                       /* no '=' after the name */
    if (line[i + 1] == '=')
        return 0;                       /* "==" is not III+ (lex error elsewhere) */
    i++;                                /* consume '=' */
    while (line[i] != '\0' && fl_isspace(line[i]))
        i++;
    *rhs = line + i;
    return 1;
}

/* ===================================================================== */
/* STORE <expr> TO <name>[, <name>...]                                    */
/* ===================================================================== */

/*
 * exec_store: handle STORE <expr> TO <name>. `args` is everything after STORE.
 * Returns INTERP_OK or a negative error (with *ec the detail).
 *
 * Grammar: STORE <expr> TO <namelist>. We scan for the " TO " keyword at the top
 * level (outside string literals), split, evaluate the expression, and store to
 * each comma-separated name.
 */
static int find_to_kw(const char *s, uint32_t *to_at)
{
    int in_s = 0, in_d = 0, in_b = 0;
    uint32_t i = 0;
    char prev = ' ';
    for (i = 0; s[i] != '\0'; i++) {
        char c = s[i];
        if (in_s) { if (c == '\'') in_s = 0; prev = c; continue; }
        if (in_d) { if (c == '"')  in_d = 0; prev = c; continue; }
        if (in_b) { if (c == ']')  in_b = 0; prev = c; continue; }
        if (c == '\'') { in_s = 1; prev = c; continue; }
        if (c == '"')  { in_d = 1; prev = c; continue; }
        if (c == '[')  { in_b = 1; prev = c; continue; }
        /* match a whole-word " TO " (blank-delimited). */
        if ((fl_isspace(prev)) &&
            (fl_up1(c) == 'T') && (fl_up1(s[i + 1]) == 'O') &&
            (fl_isspace(s[i + 2]))) {
            *to_at = i;
            return 1;
        }
        prev = c;
    }
    return 0;
}

static int exec_store(xb_interp *ip, flow_state *fs, const char *args, int *ec)
{
    char exprbuf[FLOW_MAX_LINELEN + 1];
    const char *namelist;
    uint32_t to_at, elen;
    xb_val v;
    int rc;

    if (!find_to_kw(args, &to_at)) {
        if (ec) *ec = 0;
        return -INTERP_ERR_SYNTAX;       /* STORE without TO */
    }

    /* expression = args[0 .. to_at), trimmed. */
    elen = to_at;
    while (elen > 0u && fl_isspace(args[elen - 1]))
        elen--;
    if (elen == 0u || elen > FLOW_MAX_LINELEN)
        return -INTERP_ERR_SYNTAX;
    rt_memcpy(exprbuf, args, elen);
    exprbuf[elen] = '\0';

    rc = eval_expr(ip, exprbuf, &v, ec);
    if (rc != INTERP_OK)
        return rc;

    /* namelist = past " TO " . */
    namelist = args + to_at + 3;        /* skip "TO " (T,O,space) */
    while (*namelist != '\0' && fl_isspace(*namelist))
        namelist++;

    /* store to each comma-separated name. */
    for (;;) {
        char nm[FLOW_MAX_NAME];
        uint32_t n = 0;
        while (*namelist != '\0' && fl_isspace(*namelist))
            namelist++;
        while (*namelist != '\0' && *namelist != ',' && !fl_isspace(*namelist)) {
            if (n < FLOW_MAX_NAME - 1)
                nm[n++] = fl_up1(*namelist);
            namelist++;
        }
        nm[n] = '\0';
        if (n == 0u)
            return -INTERP_ERR_SYNTAX;
        rc = memvar_set(fs, nm, &v);
        if (rc != INTERP_OK) { if (ec) *ec = 0; return rc; }
        /* skip blanks to a comma or end. */
        while (*namelist != '\0' && fl_isspace(*namelist))
            namelist++;
        if (*namelist == ',') { namelist++; continue; }
        break;
    }
    if (ec) *ec = 0;
    return INTERP_OK;
}

/* ===================================================================== */
/* Block matching: find the structural partner of an opener               */
/* ===================================================================== */

/* Line-keyword classification (the leading verb of a trimmed line). */
typedef enum {
    LK_OTHER = 0,
    LK_IF, LK_ELSE, LK_ENDIF,
    LK_DOWHILE, LK_ENDDO,
    LK_DOCASE, LK_CASE, LK_OTHERWISE, LK_ENDCASE,
    LK_LOOP, LK_EXIT
} line_kind;

/*
 * line_classify: identify a control-flow keyword at the start of `line`
 * (NUL-terminated, trimmed). Returns LK_OTHER for a plain command line.
 * Sets *rest to the bytes after the keyword (the guard expression for
 * IF/DO WHILE/CASE), trimmed of leading blanks.
 */
static line_kind line_classify(const char *line, const char **rest)
{
    char verb[16];
    const char *args;
    (void)split_verb(line, verb, sizeof(verb), &args);
    *rest = args;

    if (fl_ci_eq(verb, "IF"))        return LK_IF;
    if (fl_ci_eq(verb, "ELSE"))      return LK_ELSE;
    if (fl_ci_eq(verb, "ENDIF"))     return LK_ENDIF;
    if (fl_ci_eq(verb, "ENDDO"))     return LK_ENDDO;
    if (fl_ci_eq(verb, "CASE"))      return LK_CASE;
    if (fl_ci_eq(verb, "OTHERWISE")) return LK_OTHERWISE;
    if (fl_ci_eq(verb, "ENDCASE"))   return LK_ENDCASE;
    if (fl_ci_eq(verb, "LOOP"))      return LK_LOOP;
    if (fl_ci_eq(verb, "EXIT"))      return LK_EXIT;
    if (fl_ci_eq(verb, "DO")) {
        /* DO WHILE / DO CASE vs DO <proc> (a hook verb). */
        char w2[16];
        const char *a2;
        (void)split_verb(args, w2, sizeof(w2), &a2);
        if (fl_ci_eq(w2, "WHILE")) { *rest = a2; return LK_DOWHILE; }
        if (fl_ci_eq(w2, "CASE"))  { *rest = a2; return LK_DOCASE; }
        return LK_OTHER;            /* DO <proc> -> S5.7 hook */
    }
    return LK_OTHER;
}

/*
 * The executor walks the line array recursively. A block runs from a start index
 * to (but not including) a terminator. We pre-resolve nesting by scanning.
 */

/* Control-signal returned up the recursion to implement LOOP / EXIT. */
typedef enum {
    SIG_NONE = 0,   /* normal completion of the statement list */
    SIG_LOOP,       /* a LOOP fired: restart the innermost DO WHILE */
    SIG_EXIT        /* an EXIT fired: break the innermost DO WHILE */
} flow_signal;

/* Forward decl: classified line array shared by the executor. Guard/operand text
 * is re-derived per use from the line (line_classify), so it is not cached here. */
typedef struct {
    xb_interp  *ip;
    flow_state *fs;
    flow_line  *lines;
    line_kind  *kind;          /* per-line classification */
    int         nlines;
} flow_prog;

/* Forward decl of the recursive runner. */
static int run_block(flow_prog *fp, int from, int to, flow_signal *sig, int *ec);

/*
 * match_block_end: given an opener at index `start`, return the index of its
 * matching terminator (ENDIF for IF, ENDDO for DO WHILE, ENDCASE for DO CASE),
 * accounting for nesting. Returns -1 (unmatched) -> fail loud structure error.
 */
static int match_block_end(flow_prog *fp, int start)
{
    line_kind open = fp->kind[start];
    int depth = 0, i;
    line_kind want_end;
    if (open == LK_IF)        want_end = LK_ENDIF;
    else if (open == LK_DOWHILE) want_end = LK_ENDDO;
    else if (open == LK_DOCASE)  want_end = LK_ENDCASE;
    else return -1;

    for (i = start; i < fp->nlines; i++) {
        line_kind k = fp->kind[i];
        if (k == LK_IF || k == LK_DOWHILE || k == LK_DOCASE) {
            if (i != start) depth++;
        } else if (k == LK_ENDIF || k == LK_ENDDO || k == LK_ENDCASE) {
            if (depth == 0) {
                return (k == want_end) ? i : -1;  /* mismatched terminator */
            }
            depth--;
        }
    }
    return -1;
}

/*
 * find_top_level: within [from,to), find the first line whose kind is one of
 * `a`/`b`/`c` at the SAME nesting depth as `from` (depth 0 relative to from).
 * Used to find ELSE (within an IF body) and CASE/OTHERWISE (within DO CASE).
 * Returns the index, or -1 if none at top level. Pass LK_OTHER for an unused
 * slot -- LK_OTHER is NEVER matched (it is the "plain command line" kind, which
 * must not be a search target, or every body line would falsely match).
 */
static int find_top_level(flow_prog *fp, int from, int to,
                          line_kind a, line_kind b, line_kind c)
{
    int depth = 0, i;
    for (i = from; i < to; i++) {
        line_kind k = fp->kind[i];
        if (k == LK_IF || k == LK_DOWHILE || k == LK_DOCASE) {
            depth++;
        } else if (k == LK_ENDIF || k == LK_ENDDO || k == LK_ENDCASE) {
            depth--;
        } else if (depth == 0 && k != LK_OTHER &&
                   (k == a || k == b || k == c)) {
            return i;
        }
    }
    return -1;
}

/* ===================================================================== */
/* Command dispatch for a single (non-control-flow) statement             */
/* ===================================================================== */

/*
 * exec_command: dispatch one plain command line (already known to be LK_OTHER).
 *   1. assignment "<name> = <expr>"     -> memvar store
 *   2. STORE <expr> TO <namelist>       -> memvar store
 *   3. SELECT n / SELECT <alias>        -> work-area select (spine has the engine)
 *   4. RELEASE ALL                      -> drop memvars
 *   5. otherwise -> the registered command hook (S5.4..S5.7); CMD_UNKNOWN ->
 *      fail loud #16.
 */
static int exec_command(flow_prog *fp, const char *line, int *ec)
{
    xb_interp  *ip = fp->ip;
    flow_state *fs = fp->fs;
    char verb[16];
    char name[FLOW_MAX_NAME];
    const char *args, *rhs;
    int rc;

    if (ec) *ec = 0;

    /* (1) assignment? */
    if (detect_assign(line, name, sizeof(name), &rhs)) {
        xb_val v;
        rc = eval_expr(ip, rhs, &v, ec);
        if (rc != INTERP_OK)
            return rc;
        rc = memvar_set(fs, name, &v);
        if (rc != INTERP_OK) { if (ec) *ec = 0; return rc; }
        return INTERP_OK;
    }

    (void)split_verb(line, verb, sizeof(verb), &args);

    /* (2) STORE */
    if (fl_ci_eq(verb, "STORE"))
        return exec_store(ip, fs, args, ec);

    /* (3) SELECT n | <alias> -- the spine owns the work-area engine. */
    if (fl_ci_eq(verb, "SELECT")) {
        wa_env *env = xb_interp_env(ip);
        /* numeric area? */
        if (args[0] >= '0' && args[0] <= '9') {
            int areanum = 0;
            uint32_t i = 0;
            while (args[i] >= '0' && args[i] <= '9') {
                areanum = areanum * 10 + (args[i] - '0');
                i++;
            }
            rc = wa_select(env, areanum);
        } else {
            char al[16];
            const char *a2;
            (void)split_verb(args, al, sizeof(al), &a2);
            rc = wa_select_alias(env, al);
        }
        if (rc != WA_OK) { if (ec) *ec = 0; return -INTERP_ERR_SYNTAX; }
        return INTERP_OK;
    }

    /* (4) RELEASE ALL (memvar table owner is this step). */
    if (fl_ci_eq(verb, "RELEASE")) {
        char w2[16];
        const char *a2;
        (void)split_verb(args, w2, sizeof(w2), &a2);
        if (fl_ci_eq(w2, "ALL") || w2[0] == '\0') {
            int i;
            for (i = 0; i < fs->nvars; i++)
                fs->vars[i].live = 0;
            fs->nvars = 0;
            fs->arena_used = 0u;
            return INTERP_OK;
        }
        /* RELEASE <namelist> */
        {
            const char *q = args;
            for (;;) {
                char nm[FLOW_MAX_NAME];
                uint32_t n = 0;
                int slot;
                while (*q != '\0' && fl_isspace(*q)) q++;
                while (*q != '\0' && *q != ',' && !fl_isspace(*q)) {
                    if (n < FLOW_MAX_NAME - 1) nm[n++] = fl_up1(*q);
                    q++;
                }
                nm[n] = '\0';
                if (n > 0u) {
                    slot = memvar_find(fs, nm);
                    if (slot >= 0) fs->vars[slot].live = 0;
                }
                while (*q != '\0' && fl_isspace(*q)) q++;
                if (*q == ',') { q++; continue; }
                break;
            }
            return INTERP_OK;
        }
    }

    /* (5) the registered command-hook CHAIN (S5.4..S5.7), tried in order. */
    {
        int h;
        for (h = 0; h < fs->nhooks; h++) {
            int hrc;
            if (!fs->hooks[h])
                continue;
            hrc = fs->hooks[h](fs->hook_users[h], ip, verb, args, ec);
            if (hrc == CMD_OK)
                return INTERP_OK;
            if (hrc < 0)
                return hrc;         /* a real run-time error from the hook */
            /* hrc == CMD_UNKNOWN -> try the next hook in the chain. */
        }
    }

    /* unrecognised verb: fail loud (#16). */
    if (ec) *ec = FLOW_MSG_UNRECOGNIZED;
    return -INTERP_ERR_UNKNOWN_CMD;
}

/* ===================================================================== */
/* The structured executor                                                */
/* ===================================================================== */

/*
 * run_if: execute IF <guard> ... [ELSE ...] ENDIF starting at fp->lines[start]
 * (which is LK_IF). `endif` is the index of the matching ENDIF. *next is set to
 * the line after ENDIF. Propagates LOOP/EXIT signals via *sig.
 */
static int run_if(flow_prog *fp, int start, int endif, int *next,
                  flow_signal *sig, int *ec)
{
    int truth = 0, rc, else_at;
    char buf[FLOW_MAX_LINELEN + 1];

    if (copy_line(&fp->lines[start], buf, sizeof(buf)) != 0)
        return -INTERP_ERR_DEPTH;
    {
        const char *rest;
        (void)line_classify(buf, &rest);     /* re-derive the guard text */
        rc = eval_guard(fp->ip, rest, &truth, ec);
        if (rc != INTERP_OK)
            return rc;
    }

    else_at = find_top_level(fp, start + 1, endif, LK_ELSE, LK_OTHER, LK_OTHER);

    if (truth) {
        int body_to = (else_at >= 0) ? else_at : endif;
        rc = run_block(fp, start + 1, body_to, sig, ec);
    } else if (else_at >= 0) {
        rc = run_block(fp, else_at + 1, endif, sig, ec);
    } else {
        rc = INTERP_OK;            /* false, no ELSE: do nothing */
    }
    *next = endif + 1;
    return rc;
}

/*
 * run_docase: execute DO CASE / CASE / OTHERWISE / ENDCASE. The FIRST CASE whose
 * guard is .T. runs; if none, OTHERWISE runs (if present). dBASE evaluates CASE
 * guards top-to-bottom and stops at the first true one. Each CASE guard must be
 * Logical (#37). `endcase` is the matching ENDCASE; *next = endcase+1.
 */
static int run_docase(flow_prog *fp, int start, int endcase, int *next,
                      flow_signal *sig, int *ec)
{
    int i, rc;
    int otherwise_at = -1;
#ifdef FLOW_MUTATE_DOCASE_ALL
    int ran_any = 0;       /* MUTANT bookkeeping */
#else
    int chosen = -1;       /* index of the chosen CASE/OTHERWISE line */
#endif

    /* Walk top-level CASE / OTHERWISE lines in order. */
    for (i = start + 1; i < endcase; ) {
        line_kind k = fp->kind[i];
        if (k == LK_IF || k == LK_DOWHILE || k == LK_DOCASE) {
            int e2 = match_block_end(fp, i);
            if (e2 < 0) return -INTERP_ERR_STRUCT;
            i = e2 + 1;
            continue;
        }
        if (k == LK_OTHERWISE) {
            otherwise_at = i;
            i++;
            continue;
        }
        if (k == LK_CASE) {
            char buf[FLOW_MAX_LINELEN + 1];
            const char *rest;
            int truth = 0;
            if (copy_line(&fp->lines[i], buf, sizeof(buf)) != 0)
                return -INTERP_ERR_DEPTH;
            (void)line_classify(buf, &rest);
            rc = eval_guard(fp->ip, rest, &truth, ec);
            if (rc != INTERP_OK)
                return rc;
#ifdef FLOW_MUTATE_DOCASE_ALL
            /* MUTANT (Rule 6 -- -DFLOW_MUTATE_DOCASE_ALL): run EVERY true CASE
             * body instead of only the first -> a DO CASE that should pick one
             * branch runs several; the oracle's "first true CASE only" check
             * goes RED. */
            if (truth) {
                int body_end = endcase;
                int j;
                for (j = i + 1; j < endcase; ) {
                    line_kind kk = fp->kind[j];
                    if (kk == LK_IF || kk == LK_DOWHILE || kk == LK_DOCASE) {
                        int e3 = match_block_end(fp, j);
                        if (e3 < 0) return -INTERP_ERR_STRUCT;
                        j = e3 + 1; continue;
                    }
                    if (kk == LK_CASE || kk == LK_OTHERWISE) { body_end = j; break; }
                    j++;
                }
                rc = run_block(fp, i + 1, body_end, sig, ec);
                if (rc != INTERP_OK) return rc;
                if (*sig != SIG_NONE) { *next = endcase + 1; return INTERP_OK; }
                ran_any = 1;
            }
            i++;
            continue;
#else
            if (truth) { chosen = i; break; }   /* first true CASE wins */
            i++;
            continue;
#endif
        }
        /* a plain line between DO CASE and the first CASE is ignored by dBASE. */
        i++;
    }

#ifdef FLOW_MUTATE_DOCASE_ALL
    if (!ran_any && otherwise_at >= 0) {
        int body_end = find_top_level(fp, otherwise_at + 1, endcase,
                                      LK_CASE, LK_OTHERWISE, LK_OTHER);
        if (body_end < 0) body_end = endcase;
        rc = run_block(fp, otherwise_at + 1, body_end, sig, ec);
        if (rc != INTERP_OK) return rc;
    }
    *next = endcase + 1;
    return INTERP_OK;
#else
    if (chosen < 0)
        chosen = otherwise_at;     /* no true CASE: fall to OTHERWISE if present */

    if (chosen >= 0) {
        /* body runs from chosen+1 to the next top-level CASE/OTHERWISE or ENDCASE. */
        int body_end = find_top_level(fp, chosen + 1, endcase,
                                      LK_CASE, LK_OTHERWISE, LK_OTHER);
        if (body_end < 0)
            body_end = endcase;
        rc = run_block(fp, chosen + 1, body_end, sig, ec);
        if (rc != INTERP_OK)
            return rc;
    }
    *next = endcase + 1;
    return INTERP_OK;
#endif
}

/*
 * run_dowhile: execute DO WHILE <guard> ... ENDDO. Re-evaluates the guard each
 * iteration (must be Logical, #37). LOOP restarts; EXIT breaks. `enddo` is the
 * matching ENDDO; *next = enddo+1.
 */
static int run_dowhile(flow_prog *fp, int start, int enddo, int *next, int *ec)
{
    char buf[FLOW_MAX_LINELEN + 1];
    const char *rest;
    int rc;

    if (copy_line(&fp->lines[start], buf, sizeof(buf)) != 0)
        return -INTERP_ERR_DEPTH;
    (void)line_classify(buf, &rest);

    for (;;) {
        int truth = 0;
        flow_signal sig = SIG_NONE;
        rc = eval_guard(fp->ip, rest, &truth, ec);
        if (rc != INTERP_OK)
            return rc;
        if (!truth)
            break;
        rc = run_block(fp, start + 1, enddo, &sig, ec);
        if (rc != INTERP_OK)
            return rc;
        if (sig == SIG_EXIT)
            break;
        /* SIG_LOOP or SIG_NONE: re-test the guard and continue. */
    }
    *next = enddo + 1;
    return INTERP_OK;
}

/*
 * run_block: execute the statement list lines[from..to). On a LOOP/EXIT it sets
 * *sig and returns immediately (the enclosing DO WHILE handles the signal). A
 * structural error or run-time error returns negative.
 */
static int run_block(flow_prog *fp, int from, int to, flow_signal *sig, int *ec)
{
    int i = from, rc;
    char buf[FLOW_MAX_LINELEN + 1];

    while (i < to) {
        line_kind k = fp->kind[i];

        switch (k) {
        case LK_IF: {
            int endif = match_block_end(fp, i);
            int next = i + 1;
            if (endif < 0 || endif >= to) return -INTERP_ERR_STRUCT;
            rc = run_if(fp, i, endif, &next, sig, ec);
            if (rc != INTERP_OK) return rc;
            if (*sig != SIG_NONE) return INTERP_OK;   /* LOOP/EXIT bubbling up */
            i = next;
            break;
        }
        case LK_DOWHILE: {
            int enddo = match_block_end(fp, i);
            int next = i + 1;
            if (enddo < 0 || enddo >= to) return -INTERP_ERR_STRUCT;
            rc = run_dowhile(fp, i, enddo, &next, ec);   /* swallows LOOP/EXIT */
            if (rc != INTERP_OK) return rc;
            i = next;
            break;
        }
        case LK_DOCASE: {
            int endcase = match_block_end(fp, i);
            int next = i + 1;
            if (endcase < 0 || endcase >= to) return -INTERP_ERR_STRUCT;
            rc = run_docase(fp, i, endcase, &next, sig, ec);
            if (rc != INTERP_OK) return rc;
            if (*sig != SIG_NONE) return INTERP_OK;
            i = next;
            break;
        }
        case LK_LOOP:
            *sig = SIG_LOOP;
            return INTERP_OK;
        case LK_EXIT:
            *sig = SIG_EXIT;
            return INTERP_OK;

        /* terminators / mid-block keywords that appear here are unmatched. */
        case LK_ELSE:
        case LK_ENDIF:
        case LK_ENDDO:
        case LK_CASE:
        case LK_OTHERWISE:
        case LK_ENDCASE:
            if (ec) *ec = 0;
            return -INTERP_ERR_STRUCT;   /* fail loud: structure error */

        case LK_OTHER:
        default:
            if (copy_line(&fp->lines[i], buf, sizeof(buf)) != 0)
                return -INTERP_ERR_DEPTH;
            rc = exec_command(fp, buf, ec);
            if (rc != INTERP_OK)
                return rc;
            i++;
            break;
        }
    }
    return INTERP_OK;
}

/* ===================================================================== */
/* Public API                                                             */
/* ===================================================================== */

/*
 * flow_mark_inuse: install the composed resolver onto ip's ctx if it is not
 * already there, capturing the work-area delegate. This marks the flow_state as
 * "in use" (ctx->resolve == flow_resolve) so a subsequent flow_state_for /
 * samir_do does NOT mistake a registered-but-not-yet-run interp for a remade one
 * and RESET it (which would wipe the just-registered command hooks). It is the
 * same install xb_interp_store performs; field resolution is unchanged because
 * flow_resolve delegates to the saved work-area resolver.
 */
static void flow_mark_inuse(xb_interp *ip, flow_state *fs)
{
    xb_ctx *ctx = xb_interp_ctx(ip);
    if (ctx && ctx->resolve != flow_resolve) {
        fs->saved_resolve = ctx->resolve;
        fs->saved_user    = ctx->user;
        ctx->resolve = flow_resolve;
        ctx->user    = fs;
    }
}

void xb_interp_set_cmd_hook(xb_interp *ip, xb_cmd_hook hook, void *user)
{
    flow_state *fs = flow_state_for(ip);
    int i;
    if (!fs)
        return;
    flow_mark_inuse(ip, fs);
    /* Clear the chain, then register `hook` as its sole entry (S5.3 compat). */
    for (i = 0; i < INTERP_MAX_CMD_HOOKS; i++) {
        fs->hooks[i]      = (xb_cmd_hook)0;
        fs->hook_users[i] = (void *)0;
    }
    fs->nhooks = 0;
    if (hook) {
        fs->hooks[0]      = hook;
        fs->hook_users[0] = user;
        fs->nhooks        = 1;
    }
}

int xb_interp_add_cmd_hook(xb_interp *ip, xb_cmd_hook hook, void *user)
{
    flow_state *fs = flow_state_for(ip);
    if (!fs)
        return -INTERP_ERR_NOMEM;
    if (!hook)
        return INTERP_OK;                  /* NULL append is a no-op */
    flow_mark_inuse(ip, fs);               /* keep the chain across the first samir_do */
    if (fs->nhooks >= INTERP_MAX_CMD_HOOKS)
        return -INTERP_ERR_DEPTH;          /* chain full: fail loud (Rule 2) */
    fs->hooks[fs->nhooks]      = hook;
    fs->hook_users[fs->nhooks] = user;
    fs->nhooks++;
    return INTERP_OK;
}

int xb_interp_store(xb_interp *ip, const char *name, const xb_val *v)
{
    flow_state *fs = flow_state_for(ip);
    xb_ctx *ctx;
    if (!fs || !name || !v)
        return -INTERP_ERR_NOMEM;
    /* ensure the composed resolver is installed so the stored memvar is visible
     * to subsequent xb_interp_eval_str / samir_do (delegates to fields). */
    ctx = xb_interp_ctx(ip);
    if (ctx && ctx->resolve != flow_resolve) {
        fs->saved_resolve = ctx->resolve;
        fs->saved_user    = ctx->user;
        ctx->resolve = flow_resolve;
        ctx->user    = fs;
    }
    return memvar_set(fs, name, v);
}

int xb_interp_get_memvar(xb_interp *ip, const char *name, xb_val *out)
{
    flow_state *fs = flow_lookup(ip);
    int slot;
    if (!fs || !name || !out)
        return 1;
    slot = memvar_find(fs, name);
    if (slot < 0)
        return 1;
    *out = fs->vars[slot].val;
    return 0;
}

void xb_interp_release_all(xb_interp *ip)
{
    flow_state *fs = flow_lookup(ip);
    int i;
    if (!fs)
        return;
    for (i = 0; i < fs->nvars; i++)
        fs->vars[i].live = 0;
    fs->nvars = 0;
    fs->arena_used = 0u;
}

int samir_last_error(xb_interp *ip)
{
    flow_state *fs = flow_lookup(ip);
    return fs ? fs->last_error : 0;
}

/*
 * samir_do: the S5.3 entry point. Splits `prg` into lines, classifies each,
 * installs the composed (memvar-aware) resolver onto the interp ctx, runs the
 * statement list, then restores the original resolver. Records the last error.
 */
int samir_do(xb_interp *ip, const char *prg)
{
    /* Line pools sized for FLOW_MAX_LINES, on THIS call's stack (~8 KiB). Kept on
     * the stack -- NOT static -- so samir_do is RE-ENTRANT: S5.7's DO <proc> can
     * recursively run a nested program without clobbering the caller's lines. */
    flow_line  s_lines[FLOW_MAX_LINES];
    line_kind  s_kind[FLOW_MAX_LINES];

    flow_state *fs;
    xb_ctx *ctx;
    flow_prog fp;
    flow_signal sig = SIG_NONE;
    int n, i, rc, ec = 0;

    if (!ip || !prg)
        return -INTERP_ERR_EVAL;

    fs = flow_state_for(ip);
    if (!fs)
        return -INTERP_ERR_NOMEM;
    fs->last_error = 0;

    n = split_lines(prg, s_lines, FLOW_MAX_LINES);
    if (n < 0) {
        fs->last_error = INTERP_ERR_DEPTH;
        return -INTERP_ERR_DEPTH;
    }

    /* classify each line (needs a NUL-terminated working copy). */
    for (i = 0; i < n; i++) {
        char buf[FLOW_MAX_LINELEN + 1];
        const char *rest;
        if (copy_line(&s_lines[i], buf, sizeof(buf)) != 0) {
            fs->last_error = INTERP_ERR_DEPTH;
            return -INTERP_ERR_DEPTH;
        }
        s_kind[i] = line_classify(buf, &rest);
        (void)rest;                    /* guard text is re-derived per use */
    }

    /* Install the COMPOSED resolver: save the work-area resolver, then point the
     * ctx at flow_resolve. PRESERVE every other ctx field (set_exact, scratch,
     * ctx_today, and any cursor hook a parallel lane adds) -- ONLY resolve/user
     * change. We deliberately DO NOT restore on return: flow_resolve delegates to
     * the saved work-area resolver for fields, so field resolution is unchanged,
     * and leaving it installed lets memvars persist across samir_do calls on the
     * same interp AND is the fresh-interp detector used by flow_state_for. We only
     * (re)capture the delegate when it is NOT already flow_resolve, so a second
     * samir_do does not capture flow_resolve as its own delegate (infinite loop). */
    ctx = xb_interp_ctx(ip);
    if (ctx->resolve != flow_resolve) {
        fs->saved_resolve = ctx->resolve;
        fs->saved_user    = ctx->user;
    }
    ctx->resolve = flow_resolve;
    ctx->user    = fs;

    fp.ip     = ip;
    fp.fs     = fs;
    fp.lines  = s_lines;
    fp.kind   = s_kind;
    fp.nlines = n;

    rc = run_block(&fp, 0, n, &sig, &ec);

    /* A LOOP/EXIT that bubbled to the top level (outside any DO WHILE) is a
     * structure error. */
    if (rc == INTERP_OK && sig != SIG_NONE) {
        rc = -INTERP_ERR_STRUCT;
        ec = 0;
    }

    /* NOTE: we leave ctx->resolve == flow_resolve (it delegates to the saved
     * work-area resolver). See the install comment + flow_state_for. All other
     * ctx fields are untouched. */

    if (rc != INTERP_OK) {
        /* record the dBASE catalog ordinal where the eval produced one,
         * else the structural/interp code. */
        if (ec != 0)
            fs->last_error = ec;
        else
            fs->last_error = -rc;   /* the positive interp_err */
        return rc;
    }
    fs->last_error = 0;
    return INTERP_OK;
}
