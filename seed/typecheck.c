/*
 * typecheck.c -- minimal static type checker for the seed Pascal subset.
 *
 * beads: initech-f0uc (B1 of docs/plans/TPS-M7-subset-plan.md)
 * Ref:   see typecheck.h for the full rule set and the DECISION note on
 *        checking strictness (ADR-0007 is silent on this; this module is
 *        the "minimal sound checking" the bead asked for). CLAUDE.md Law 1
 *        / Rule 2 (fail loud on internal contract breaks) / Rule 12.
 *
 * Two passes over the AST, mirroring the parser's own single-error style
 * (see parser.c fail_at):
 *   1. collect_decls  -- build a flat, insertion-ordered symbol table from
 *      the program's var-sections (name -> declared AstVarType), rejecting
 *      duplicate names.
 *   2. check_stmt / check_expr -- walk the block, resolving every VARREF
 *      against the table, checking assignment/operator/write-arg type
 *      agreement, and annotating every expression node's `type` field.
 *
 * The symbol table is a small fixed-capacity array (TC_MAX_SYMBOLS), not a
 * hash map -- insertion-ordered linear scan, no nondeterministic iteration
 * (consistent with the determinism discipline ADR-0007 DEC-04 states for
 * Turbo Initech; the seed is host tooling, not the artifact, but there is
 * no reason to diverge from a simpler-and-deterministic structure here).
 * Overflow is a located, fail-loud diagnostic (Rule 2), never silent
 * truncation.
 */
#include "typecheck.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define TC_MAX_SYMBOLS 256
#define TC_SYM_NAME_CAP 128
/* B4 (beads initech-63ce): fixed-capacity, fail-loud-on-overflow tables for
 * the routine table and per-routine local scope -- the same "no hashing,
 * insertion-ordered, deterministic" discipline TC_MAX_SYMBOLS already uses
 * (ADR-0007 DEC-04 spirit). */
#define TC_MAX_PROCS   128
#define TC_MAX_PARAMS   32
#define TC_MAX_LOCALS   64

typedef struct {
    char       name[TC_SYM_NAME_CAP]; /* case-folded (lower) declared name */
    AstVarType type;
    /* B3 (beads initech-7mo3): 1 if this name was declared `const`, 0 if
     * `var`. const and var share ONE flat, case-insensitive namespace (the
     * same one-declaration rule) -- see collect_decls -- and is_const is
     * how check_stmt's AST_ASSIGN case rejects "assign to a constant". */
    int        is_const;
} TcSym;

/* B4 (beads initech-63ce): one entry in a routine's local scope. `kind`
 * distinguishes a value parameter, a var (by-reference) parameter, a local
 * variable, and the function-result variable; `is_lvalue` marks everything
 * that may be an assignment target or a var-parameter argument (i.e. not a
 * constant -- but there are no local consts in this subset, so every local
 * is an lvalue). */
typedef enum {
    TC_KIND_VALPARAM,
    TC_KIND_VARPARAM,
    TC_KIND_LOCAL,
    TC_KIND_RESULT
} TcLocalKind;

typedef struct {
    char        name[TC_SYM_NAME_CAP];
    AstVarType  type;
    TcLocalKind kind;
} TcLocal;

/* B4: one procedure/function signature. Params are flattened (one entry per
 * name). `has_result`/`rettype` describe a function; `is_defined` flips from
 * 0 (a `forward` declaration seen) to 1 (the defining occurrence seen). */
typedef struct {
    char       name[TC_SYM_NAME_CAP];
    int        has_result;
    AstVarType rettype;
    struct {
        char       name[TC_SYM_NAME_CAP];
        AstVarType type;
        int        is_var;
    } params[TC_MAX_PARAMS];
    int        nparams;
    int        is_defined;
    int        line, col; /* of the (first) declaration, for diagnostics */
} TcProc;

typedef struct {
    TcSym syms[TC_MAX_SYMBOLS];   /* program-level vars + consts (globals) */
    int   nsyms;

    TcProc procs[TC_MAX_PROCS];   /* B4: routine signatures */
    int    nprocs;

    /* B4: the local scope of the routine body currently being checked.
     * nlocal == 0 (and in_routine == 0) while checking the main program
     * block, so resolution falls through to globals only. */
    TcLocal local[TC_MAX_PARAMS + TC_MAX_LOCALS];
    int     nlocal;
    int     in_routine;

    int   failed;
    char  errmsg[TYPECHECK_ERRMSG_CAP];
    int   errline;
    int   errcol;
} Tc;

/* Record the first error only (parser.c fail_at idiom: keep the most
 * relevant, earliest-encountered location). */
static void fail_at(Tc *tc, int line, int col, const char *msg)
{
    if (tc->failed)
        return;
    tc->failed = 1;
    tc->errline = line;
    tc->errcol = col;
    snprintf(tc->errmsg, sizeof(tc->errmsg), "%s", msg);
}

/* Case-fold `src` into `dst` (NUL-terminated, truncated at cap-1 bytes --
 * Pascal identifiers this short in the seed's own bootstrap corpus are not
 * expected to collide post-truncation; a real collision would surface as a
 * spurious "duplicate variable declaration" or "undeclared variable",
 * which is a fail-loud symptom, never a silent misresolution). */
static void lower_copy(char *dst, size_t cap, const char *src)
{
    size_t i = 0;
    for (; src[i] != '\0' && i + 1 < cap; i++)
        dst[i] = (char)tolower((unsigned char)src[i]);
    dst[i] = '\0';
}

static int sym_find(const Tc *tc, const char *name_lc)
{
    for (int i = 0; i < tc->nsyms; i++)
        if (strcmp(tc->syms[i].name, name_lc) == 0)
            return i;
    return -1;
}

/* B4 (beads initech-63ce): find a name in the ACTIVE routine's local scope
 * (params + locals + result). -1 if not present (or no routine is active). */
static int local_find(const Tc *tc, const char *name_lc)
{
    for (int i = 0; i < tc->nlocal; i++)
        if (strcmp(tc->local[i].name, name_lc) == 0)
            return i;
    return -1;
}

/* B4: find a routine by (case-folded) name. -1 if not declared. */
static int proc_find(const Tc *tc, const char *name_lc)
{
    for (int i = 0; i < tc->nprocs; i++)
        if (strcmp(tc->procs[i].name, name_lc) == 0)
            return i;
    return -1;
}

/*
 * B4: resolve a name to its type, checking the active routine's local scope
 * FIRST (so a param/local/result shadows a global), then globals. Returns the
 * type via *out_type and (for globals) whether it is a constant via
 * *out_is_const / whether it is an lvalue via *out_is_lvalue. Returns 1 on
 * success, 0 if the name is undeclared (no error recorded here -- the caller
 * words the diagnostic for its context). A local is always an lvalue (no
 * local consts in this subset); a global var is an lvalue, a global const is
 * not.
 */
static int resolve_name(const Tc *tc, const char *name_lc,
                        AstVarType *out_type, int *out_is_lvalue)
{
    int li = local_find(tc, name_lc);
    if (li >= 0) {
        *out_type = tc->local[li].type;
        *out_is_lvalue = 1; /* every local (param/local/result) is writable */
        return 1;
    }
    int si = sym_find(tc, name_lc);
    if (si >= 0) {
        *out_type = tc->syms[si].type;
        *out_is_lvalue = !tc->syms[si].is_const;
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Pass 1: collect declarations                                       */
/* ------------------------------------------------------------------ */
static void collect_decls(Tc *tc, const AstList *decls)
{
    for (size_t i = 0; i < decls->count && !tc->failed; i++) {
        const AstNode *vd = decls->items[i];

        /* B3 (beads initech-7mo3): a const-decl contributes exactly ONE
         * name (unlike AST_VARDECL, which names a whole list). It shares
         * the SAME case-insensitive one-declaration rule as vars (this is
         * the requirement typecheck.h's B3 contract states): a const
         * colliding with an existing var name -- or with another const --
         * is the identical "duplicate variable declaration" diagnostic
         * used for two colliding var-decls. */
        if (vd->kind == AST_CONSTDECL) {
            char lc[TC_SYM_NAME_CAP];
            lower_copy(lc, sizeof(lc), vd->as.constdecl.name);
            if (sym_find(tc, lc) >= 0) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "duplicate variable declaration: %s",
                         vd->as.constdecl.name);
                fail_at(tc, vd->line, vd->col, msg);
                return;
            }
            if (tc->nsyms >= TC_MAX_SYMBOLS) {
                fail_at(tc, vd->line, vd->col,
                        "too many variable declarations "
                        "(TC_MAX_SYMBOLS exceeded)");
                return;
            }
            lower_copy(tc->syms[tc->nsyms].name,
                       sizeof(tc->syms[tc->nsyms].name),
                       vd->as.constdecl.name);
            tc->syms[tc->nsyms].type = vd->as.constdecl.ctype;
            tc->syms[tc->nsyms].is_const = 1;
            tc->nsyms++;
            continue;
        }

        /* B4 (beads initech-63ce): procedure/function declarations live in
         * the SAME decls list (see parser.c) but are collected separately by
         * collect_procs; skip them in the globals pass. */
        if (vd->kind == AST_PROCDECL || vd->kind == AST_FUNCDECL)
            continue;

        if (vd->kind != AST_VARDECL) {
            fail_at(tc, vd->line, vd->col,
                    "internal: non-vardecl/constdecl in program decls "
                    "(typecheck)");
            return;
        }
        for (size_t j = 0; j < vd->as.vardecl.names.count && !tc->failed; j++) {
            const AstNode *vr = vd->as.vardecl.names.items[j];
            if (vr->kind != AST_VARREF) {
                fail_at(tc, vr->line, vr->col,
                        "internal: non-varref in vardecl names (typecheck)");
                return;
            }
            char lc[TC_SYM_NAME_CAP];
            lower_copy(lc, sizeof(lc), vr->as.varref.name);
            if (sym_find(tc, lc) >= 0) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "duplicate variable declaration: %s",
                         vr->as.varref.name);
                fail_at(tc, vr->line, vr->col, msg);
                return;
            }
            if (tc->nsyms >= TC_MAX_SYMBOLS) {
                fail_at(tc, vr->line, vr->col,
                        "too many variable declarations "
                        "(TC_MAX_SYMBOLS exceeded)");
                return;
            }
            lower_copy(tc->syms[tc->nsyms].name,
                       sizeof(tc->syms[tc->nsyms].name),
                       vr->as.varref.name);
            tc->syms[tc->nsyms].type = vd->as.vardecl.vtype;
            tc->syms[tc->nsyms].is_const = 0;
            tc->nsyms++;
        }
    }
}

/* ------------------------------------------------------------------ */
/* B4 (beads initech-63ce): collect procedure/function signatures        */
/* ------------------------------------------------------------------ */
/* Flatten one AST_PROCDECL/AST_FUNCDECL's parameter list into `out`,
 * rejecting duplicate parameter names within the routine and capacity
 * overflow (Rule 2). Returns 1 on success, 0 (with an error recorded) on
 * failure. */
static int flatten_signature(Tc *tc, const AstNode *pf, TcProc *out)
{
    lower_copy(out->name, sizeof(out->name), pf->as.procfunc.name);
    out->has_result = pf->as.procfunc.has_result;
    out->rettype = pf->as.procfunc.rettype;
    out->nparams = 0;
    out->line = pf->line;
    out->col = pf->col;
    out->is_defined = 0;

    for (size_t i = 0; i < pf->as.procfunc.params.count; i++) {
        const AstNode *pn = pf->as.procfunc.params.items[i];
        if (pn->kind != AST_PARAM) {
            fail_at(tc, pn->line, pn->col,
                    "internal: non-param in routine parameter list");
            return 0;
        }
        if (out->nparams >= TC_MAX_PARAMS) {
            fail_at(tc, pn->line, pn->col,
                    "too many parameters (TC_MAX_PARAMS exceeded)");
            return 0;
        }
        char plc[TC_SYM_NAME_CAP];
        lower_copy(plc, sizeof(plc), pn->as.param.name);
        for (int k = 0; k < out->nparams; k++) {
            if (strcmp(out->params[k].name, plc) == 0) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "duplicate parameter name: %s", pn->as.param.name);
                fail_at(tc, pn->line, pn->col, msg);
                return 0;
            }
        }
        lower_copy(out->params[out->nparams].name,
                   sizeof(out->params[out->nparams].name), pn->as.param.name);
        out->params[out->nparams].type = pn->as.param.ptype;
        out->params[out->nparams].is_var = pn->as.param.is_var;
        out->nparams++;
    }
    return 1;
}

/* Do two signatures agree on the parts that matter for a `forward` <->
 * defining match: result-ness, result type, arity, and each parameter's type
 * and by-reference flag? (Parameter NAMES may differ -- only the interface
 * shape is load-bearing.) */
static int signatures_match(const TcProc *a, const TcProc *b)
{
    if (a->has_result != b->has_result)
        return 0;
    if (a->has_result && a->rettype != b->rettype)
        return 0;
    if (a->nparams != b->nparams)
        return 0;
    for (int i = 0; i < a->nparams; i++) {
        if (a->params[i].type != b->params[i].type)
            return 0;
        if (a->params[i].is_var != b->params[i].is_var)
            return 0;
    }
    return 1;
}

static void collect_procs(Tc *tc, const AstList *decls)
{
    for (size_t i = 0; i < decls->count && !tc->failed; i++) {
        const AstNode *pf = decls->items[i];
        if (pf->kind != AST_PROCDECL && pf->kind != AST_FUNCDECL)
            continue;

        TcProc sig;
        memset(&sig, 0, sizeof(sig));
        if (!flatten_signature(tc, pf, &sig))
            return;

        /* A routine name may not collide with a program-level var/const
         * (they would be indistinguishable at a bare-identifier use site). */
        if (sym_find(tc, sig.name) >= 0) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "procedure/function name collides with a variable or "
                     "constant: %s", pf->as.procfunc.name);
            fail_at(tc, pf->line, pf->col, msg);
            return;
        }

        int idx = proc_find(tc, sig.name);
        if (idx < 0) {
            if (tc->nprocs >= TC_MAX_PROCS) {
                fail_at(tc, pf->line, pf->col,
                        "too many procedures/functions "
                        "(TC_MAX_PROCS exceeded)");
                return;
            }
            sig.is_defined = pf->as.procfunc.is_forward ? 0 : 1;
            tc->procs[tc->nprocs++] = sig;
            continue;
        }

        /* A second declaration of an already-known name. The only legal case
         * is: an earlier `forward` now being DEFINED with a matching
         * signature. */
        TcProc *ex = &tc->procs[idx];
        if (pf->as.procfunc.is_forward) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "duplicate declaration of %s", pf->as.procfunc.name);
            fail_at(tc, pf->line, pf->col, msg);
            return;
        }
        if (ex->is_defined) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "duplicate definition of %s", pf->as.procfunc.name);
            fail_at(tc, pf->line, pf->col, msg);
            return;
        }
        if (!signatures_match(ex, &sig)) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "defining declaration of %s does not match its "
                     "forward declaration (parameter types/count or result "
                     "type differ)", pf->as.procfunc.name);
            fail_at(tc, pf->line, pf->col, msg);
            return;
        }
        ex->is_defined = 1;
    }
}

/* ------------------------------------------------------------------ */
/* Pass 2: expressions + statements                                    */
/* ------------------------------------------------------------------ */
static AstVarType check_expr(Tc *tc, AstNode *e);

/* B4: check one call node against the callee's signature. `want_value` is 1
 * in an expression context (the callee must be a FUNCTION and the call yields
 * its result type) and 0 in a statement context (the callee must be a
 * PROCEDURE). Returns the result type in an expression context, else
 * AST_TY_UNKNOWN. */
static AstVarType check_call(Tc *tc, AstNode *call, int want_value)
{
    char lc[TC_SYM_NAME_CAP];
    lower_copy(lc, sizeof(lc), call->as.call.name);
    int idx = proc_find(tc, lc);
    if (idx < 0) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg), "call to undeclared procedure/function: %s",
                 call->as.call.name);
        fail_at(tc, call->line, call->col, msg);
        return AST_TY_UNKNOWN;
    }
    TcProc *pr = &tc->procs[idx];

    if (want_value && !pr->has_result) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "procedure %s has no result and cannot be used in an "
                 "expression", call->as.call.name);
        fail_at(tc, call->line, call->col, msg);
        return AST_TY_UNKNOWN;
    }
    if (!want_value && pr->has_result) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "function %s must be called in an expression; its result "
                 "may not be discarded in this subset", call->as.call.name);
        fail_at(tc, call->line, call->col, msg);
        return AST_TY_UNKNOWN;
    }

    if ((int)call->as.call.args.count != pr->nparams) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "%s expects %d argument(s) but got %d", call->as.call.name,
                 pr->nparams, (int)call->as.call.args.count);
        fail_at(tc, call->line, call->col, msg);
        return AST_TY_UNKNOWN;
    }

    for (int i = 0; i < pr->nparams && !tc->failed; i++) {
        AstNode *arg = call->as.call.args.items[i];
        /* A var (by-reference) parameter requires a VARIABLE argument (an
         * lvalue) -- never a literal, constant, or compound expression. Check
         * this BEFORE type-checking the arg so the diagnostic is specific
         * (the DEEP-BUG locus: a var-param argument that is not a variable
         * would otherwise silently take the address of a temporary). */
        if (pr->params[i].is_var) {
            AstVarType at;
            int is_lvalue = 0;
            int resolved = 0;
            if (arg->kind == AST_VARREF) {
                char alc[TC_SYM_NAME_CAP];
                lower_copy(alc, sizeof(alc), arg->as.varref.name);
                resolved = resolve_name(tc, alc, &at, &is_lvalue);
            }
            if (arg->kind != AST_VARREF || !resolved || !is_lvalue) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "argument %d of %s is a `var` parameter and requires "
                         "a variable (not a literal, constant, or expression)",
                         i + 1, call->as.call.name);
                fail_at(tc, arg->line, arg->col, msg);
                return AST_TY_UNKNOWN;
            }
            arg->type = at; /* annotate the VARREF so codegen knows its type */
        }
        AstVarType at = check_expr(tc, arg);
        if (tc->failed)
            return AST_TY_UNKNOWN;
        if (at != pr->params[i].type) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "argument %d of %s has type %s but parameter expects %s",
                     i + 1, call->as.call.name, ast_vartype_name(at),
                     ast_vartype_name(pr->params[i].type));
            fail_at(tc, arg->line, arg->col, msg);
            return AST_TY_UNKNOWN;
        }
    }

    if (pr->has_result) {
        call->type = pr->rettype;
        return call->type;
    }
    return AST_TY_UNKNOWN;
}

static AstVarType check_binop(Tc *tc, AstNode *e)
{
    AstVarType lt = check_expr(tc, e->as.binop.lhs);
    AstVarType rt = check_expr(tc, e->as.binop.rhs);
    if (tc->failed)
        return AST_TY_UNKNOWN;

    switch (e->as.binop.op) {
    case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: case OP_MOD:
        if (lt != AST_TY_INTEGER || rt != AST_TY_INTEGER) {
            fail_at(tc, e->line, e->col,
                    "arithmetic operator requires integer operands");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_INTEGER;
        return e->type;
    case OP_AND: case OP_OR:
        if (lt != AST_TY_BOOLEAN || rt != AST_TY_BOOLEAN) {
            fail_at(tc, e->line, e->col,
                    "'and'/'or' require boolean operands (complete "
                    "evaluation -- ADR-0007 DEC-03; both sides always "
                    "evaluate, so both must already be boolean)");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_BOOLEAN;
        return e->type;
    case OP_EQ: case OP_NE: case OP_LT: case OP_LE: case OP_GT: case OP_GE:
        /* B3 (beads initech-7mo3): char joins integer/boolean here for
         * free -- the rule is already generic ("both sides the same
         * type"), so char==char / char<char / etc. Just Work the moment
         * AST_TY_CHAR exists as a real type; no relational-specific change
         * was needed beyond this diagnostic wording. */
        if (lt == AST_TY_UNKNOWN || rt == AST_TY_UNKNOWN || lt != rt) {
            fail_at(tc, e->line, e->col,
                    "relational operands must be the same type "
                    "(both integer, both boolean, or both char)");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_BOOLEAN;
        return e->type;
    case OP_NEG: case OP_NOT: case OP_ORD: case OP_CHR:
        fail_at(tc, e->line, e->col,
                "internal: unary operator reached check_binop");
        return AST_TY_UNKNOWN;
    }
    fail_at(tc, e->line, e->col, "internal: unknown binary operator");
    return AST_TY_UNKNOWN;
}

static AstVarType check_unop(Tc *tc, AstNode *e)
{
    AstVarType t = check_expr(tc, e->as.unop.operand);
    if (tc->failed)
        return AST_TY_UNKNOWN;

    switch (e->as.unop.op) {
    case OP_NEG:
        if (t != AST_TY_INTEGER) {
            fail_at(tc, e->line, e->col,
                    "unary '-' requires an integer operand");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_INTEGER;
        return e->type;
    case OP_NOT:
        if (t != AST_TY_BOOLEAN) {
            fail_at(tc, e->line, e->col,
                    "'not' requires a boolean operand");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_BOOLEAN;
        return e->type;
    /* B3 (beads initech-7mo3; ADR-0007 DEC-02 "ord, chr"). */
    case OP_ORD:
        /* DECISION (report): ord() is TOTAL over every ordinal type this
         * subset has -- char, boolean, AND integer. ord(anInteger) is the
         * identity (real Pascal: Integer already IS an ordinal type, so
         * Ord(anInteger) = anInteger -- Borland Turbo Pascal 7.0 Language
         * Guide, "Ord": "Ord returns a value of type Longint that is the
         * ordinal number of X" for any ordinal X). ord(boolean) is
         * IMPLEMENTED, not rejected (report per the bead): TP allows it and
         * gives 0/1, and the runtime already represents boolean as 0/1 in
         * eax (B1), so ord(boolean) costs zero extra codegen -- exactly the
         * same "value no-op" as ord(char)/ord(integer) (see codegen.c's
         * OP_ORD case). */
        if (t != AST_TY_CHAR && t != AST_TY_BOOLEAN && t != AST_TY_INTEGER) {
            fail_at(tc, e->line, e->col,
                    "'ord' requires a char, boolean, or integer operand");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_INTEGER;
        return e->type;
    case OP_CHR:
        /* chr() is defined only on an integer argument (ISO 7185 / Turbo
         * Pascal; there is no chr(char) or chr(boolean)). Out-of-range
         * (0..255) behaviour: see codegen.c's OP_CHR case for the 8-bit
         * TRUNCATION decision -- this pass only enforces the operand type,
         * not the range (a compile-time-unknown value can't be range-
         * checked here anyway; only a runtime concern). */
        if (t != AST_TY_INTEGER) {
            fail_at(tc, e->line, e->col, "'chr' requires an integer operand");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_CHAR;
        return e->type;
    default:
        fail_at(tc, e->line, e->col, "internal: unknown unary operator");
        return AST_TY_UNKNOWN;
    }
}

static AstVarType check_expr(Tc *tc, AstNode *e)
{
    if (tc->failed || !e)
        return AST_TY_UNKNOWN;

    switch (e->kind) {
    case AST_INTLIT:
        e->type = AST_TY_INTEGER;
        return e->type;
    case AST_BOOLLIT:
        e->type = AST_TY_BOOLEAN;
        return e->type;
    case AST_CHARLIT:
        /* B3 (beads initech-7mo3). */
        e->type = AST_TY_CHAR;
        return e->type;
    case AST_VARREF: {
        /* B4 (beads initech-63ce): resolve in the active routine's local
         * scope first (a param/local/result shadows a global), then globals.
         * A bare identifier is ALWAYS a variable/const/result reference -- a
         * call always has parentheses (parser.c). */
        char lc[TC_SYM_NAME_CAP];
        lower_copy(lc, sizeof(lc), e->as.varref.name);
        AstVarType t;
        int is_lvalue;
        if (!resolve_name(tc, lc, &t, &is_lvalue)) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg), "undeclared variable: %s",
                     e->as.varref.name);
            fail_at(tc, e->line, e->col, msg);
            return AST_TY_UNKNOWN;
        }
        e->type = t;
        return e->type;
    }
    case AST_CALL:
        /* B4: a function call used as an expression value. */
        return check_call(tc, e, /*want_value=*/1);
    case AST_BINOP:
        return check_binop(tc, e);
    case AST_UNOP:
        return check_unop(tc, e);
    default:
        fail_at(tc, e->line, e->col,
                "internal: non-expression node in expression context "
                "(typecheck)");
        return AST_TY_UNKNOWN;
    }
}

static void check_stmt(Tc *tc, AstNode *n)
{
    if (tc->failed || !n)
        return;

    switch (n->kind) {
    case AST_BLOCK:
        for (size_t i = 0; i < n->as.block.stmts.count && !tc->failed; i++)
            check_stmt(tc, n->as.block.stmts.items[i]);
        return;
    case AST_ASSIGN: {
        /* B4 (beads initech-63ce): resolve the target in local-then-global
         * scope. Assigning to a function's own name writes its RESULT
         * variable (a local of kind TC_KIND_RESULT); a `var` parameter,
         * value parameter, and local are all writable. A constant is not. */
        char lc[TC_SYM_NAME_CAP];
        lower_copy(lc, sizeof(lc), n->as.assign.name);
        AstVarType vt;
        int is_lvalue;
        if (!resolve_name(tc, lc, &vt, &is_lvalue)) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg), "undeclared variable: %s",
                     n->as.assign.name);
            fail_at(tc, n->line, n->col, msg);
            return;
        }
        /* B3 (beads initech-7mo3): a const shares the var namespace but may
         * never be an assignment TARGET -- "cannot assign to a constant" is
         * the same diagnostic real Pascal compilers give. */
        if (!is_lvalue) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg), "cannot assign to a constant: %s",
                     n->as.assign.name);
            fail_at(tc, n->line, n->col, msg);
            return;
        }
        AstVarType et = check_expr(tc, n->as.assign.value);
        if (tc->failed)
            return;
        if (et != vt) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "type mismatch in assignment to %s: expected %s, "
                     "got %s (no implicit coercion in this subset)",
                     n->as.assign.name, ast_vartype_name(vt),
                     ast_vartype_name(et));
            fail_at(tc, n->line, n->col, msg);
        }
        return;
    }
    case AST_CALL:
        /* B4: a procedure call used as a statement. */
        check_call(tc, n, /*want_value=*/0);
        return;
    case AST_WRITE:
    case AST_WRITELN:
        for (size_t i = 0; i < n->as.write.args.count && !tc->failed; i++) {
            AstNode *arg = n->as.write.args.items[i];
            if (arg->kind == AST_STRLIT)
                continue; /* string args are never type-checked as exprs */
            AstVarType t = check_expr(tc, arg);
            if (tc->failed)
                return;
            /* B3 (beads initech-7mo3): char joins integer/boolean as a
             * valid write/writeln expression-argument type -- codegen's
             * gen_write picks the byte-emitting path (serial_putc) for it,
             * never decimal formatting. */
            if (t != AST_TY_INTEGER && t != AST_TY_BOOLEAN
                && t != AST_TY_CHAR) {
                fail_at(tc, arg->line, arg->col,
                        "write/writeln argument must be integer, boolean, "
                        "or char");
                return;
            }
        }
        return;
    /* B2 (beads initech-80iw; ADR-0007 DEC-02). if/while are the only new
     * statement kinds -- for/repeat are desugared by the parser into
     * assign/while/block/not nodes BEFORE typecheck ever runs, so their
     * "guard must be boolean" (repeat, via the desugared 'not') and
     * "bounds must be integer" (for, via the desugared assignments and the
     * relational guard) requirements are enforced by the ordinary
     * AST_ASSIGN/AST_BINOP/AST_UNOP rules above -- no extra cases needed. */
    case AST_IF: {
        AstNode *cond = n->as.ifstmt.cond;
        AstVarType ct = check_expr(tc, cond);
        if (tc->failed)
            return;
        if (ct != AST_TY_BOOLEAN) {
            fail_at(tc, cond->line, cond->col,
                    "'if' condition must be boolean");
            return;
        }
        check_stmt(tc, n->as.ifstmt.then_stmt);
        if (tc->failed)
            return;
        check_stmt(tc, n->as.ifstmt.else_stmt);
        return;
    }
    case AST_WHILE: {
        AstNode *cond = n->as.whilestmt.cond;
        AstVarType ct = check_expr(tc, cond);
        if (tc->failed)
            return;
        if (ct != AST_TY_BOOLEAN) {
            fail_at(tc, cond->line, cond->col,
                    "'while' condition must be boolean");
            return;
        }
        check_stmt(tc, n->as.whilestmt.body);
        return;
    }
    default:
        fail_at(tc, n->line, n->col,
                "internal: unexpected statement node in typecheck");
        return;
    }
}

/* ------------------------------------------------------------------ */
/* B4 (beads initech-63ce): per-routine body checking                   */
/* ------------------------------------------------------------------ */
/* Add one entry to the active local scope, rejecting a duplicate WITHIN the
 * routine (a param/local/result colliding with another) and capacity
 * overflow (Rule 2). A local may freely shadow a global (not checked here). */
static void add_local(Tc *tc, int line, int col, const char *name_lc,
                      AstVarType type, TcLocalKind kind, const char *what)
{
    if (tc->failed)
        return;
    if (local_find(tc, name_lc) >= 0) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "duplicate %s in a routine scope: %s", what, name_lc);
        fail_at(tc, line, col, msg);
        return;
    }
    if (tc->nlocal >= (int)(sizeof(tc->local) / sizeof(tc->local[0]))) {
        fail_at(tc, line, col,
                "too many parameters/locals in one routine "
                "(scope table exceeded)");
        return;
    }
    lower_copy(tc->local[tc->nlocal].name,
               sizeof(tc->local[tc->nlocal].name), name_lc);
    tc->local[tc->nlocal].type = type;
    tc->local[tc->nlocal].kind = kind;
    tc->nlocal++;
}

/* Build the routine's local scope (params, then the function result
 * variable, then locals) and type-check its body. The main program block is
 * checked with an empty local scope (globals only). */
static void check_routine(Tc *tc, const AstNode *pf)
{
    if (pf->as.procfunc.is_forward)
        return; /* a forward declaration has no body to check */

    tc->nlocal = 0;
    tc->in_routine = 1;

    for (size_t i = 0; i < pf->as.procfunc.params.count && !tc->failed; i++) {
        const AstNode *pn = pf->as.procfunc.params.items[i];
        char plc[TC_SYM_NAME_CAP];
        lower_copy(plc, sizeof(plc), pn->as.param.name);
        add_local(tc, pn->line, pn->col, plc, pn->as.param.ptype,
                  pn->as.param.is_var ? TC_KIND_VARPARAM : TC_KIND_VALPARAM,
                  "parameter");
    }

    if (!tc->failed && pf->as.procfunc.has_result) {
        char rlc[TC_SYM_NAME_CAP];
        lower_copy(rlc, sizeof(rlc), pf->as.procfunc.name);
        add_local(tc, pf->line, pf->col, rlc, pf->as.procfunc.rettype,
                  TC_KIND_RESULT, "name (a parameter shadows the result)");
    }

    for (size_t i = 0; i < pf->as.procfunc.decls.count && !tc->failed; i++) {
        const AstNode *vd = pf->as.procfunc.decls.items[i];
        if (vd->kind != AST_VARDECL) {
            fail_at(tc, vd->line, vd->col,
                    "internal: non-vardecl in routine locals (typecheck)");
            break;
        }
        for (size_t j = 0; j < vd->as.vardecl.names.count && !tc->failed; j++) {
            const AstNode *vr = vd->as.vardecl.names.items[j];
            char vlc[TC_SYM_NAME_CAP];
            lower_copy(vlc, sizeof(vlc), vr->as.varref.name);
            add_local(tc, vr->line, vr->col, vlc, vd->as.vardecl.vtype,
                      TC_KIND_LOCAL, "local variable");
        }
    }

    if (!tc->failed)
        check_stmt(tc, pf->as.procfunc.body);

    tc->nlocal = 0;
    tc->in_routine = 0;
}

/* ------------------------------------------------------------------ */
/* Public entry                                                        */
/* ------------------------------------------------------------------ */
int typecheck_program(AstNode *program, TypeCheckResult *out)
{
    Tc tc;
    memset(&tc, 0, sizeof(tc));

    if (!program || program->kind != AST_PROGRAM) {
        out->ok = 0;
        snprintf(out->error, sizeof(out->error),
                 "internal: typecheck root is not a program");
        out->line = 0;
        out->col = 0;
        return 1;
    }

    /* Globals first, then routine signatures (so a routine body -- checked
     * next -- can see every global and can call any routine, including ones
     * declared later, which is why mutual recursion needs the `forward`
     * declaration to register the signature ahead of the call site). */
    collect_decls(&tc, &program->as.program.decls);
    if (!tc.failed)
        collect_procs(&tc, &program->as.program.decls);

    /* Each routine body, in source order, in its own local scope. */
    for (size_t i = 0; i < program->as.program.decls.count && !tc.failed; i++) {
        AstNode *d = program->as.program.decls.items[i];
        if (d->kind == AST_PROCDECL || d->kind == AST_FUNCDECL)
            check_routine(&tc, d);
    }

    /* The main program block, with an empty local scope (globals only). */
    if (!tc.failed) {
        tc.nlocal = 0;
        tc.in_routine = 0;
        check_stmt(&tc, program->as.program.block);
    }

    /* Every `forward` declaration must have been defined (Rule 2: a missing
     * definition is a link-time undefined symbol, caught here as a located
     * source error instead). */
    for (int i = 0; i < tc.nprocs && !tc.failed; i++) {
        if (!tc.procs[i].is_defined) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "forward declaration never defined: %s", tc.procs[i].name);
            fail_at(&tc, tc.procs[i].line, tc.procs[i].col, msg);
        }
    }

    if (tc.failed) {
        out->ok = 0;
        snprintf(out->error, sizeof(out->error), "%s", tc.errmsg);
        out->line = tc.errline;
        out->col = tc.errcol;
        return 1;
    }

    out->ok = 1;
    out->error[0] = '\0';
    out->line = 0;
    out->col = 0;
    return 0;
}
