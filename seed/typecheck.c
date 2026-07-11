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

typedef struct {
    char       name[TC_SYM_NAME_CAP]; /* case-folded (lower) declared name */
    AstVarType type;
} TcSym;

typedef struct {
    TcSym syms[TC_MAX_SYMBOLS];
    int   nsyms;

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

/* ------------------------------------------------------------------ */
/* Pass 1: collect declarations                                       */
/* ------------------------------------------------------------------ */
static void collect_decls(Tc *tc, const AstList *decls)
{
    for (size_t i = 0; i < decls->count && !tc->failed; i++) {
        const AstNode *vd = decls->items[i];
        if (vd->kind != AST_VARDECL) {
            fail_at(tc, vd->line, vd->col,
                    "internal: non-vardecl in program decls (typecheck)");
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
            tc->nsyms++;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Pass 2: expressions + statements                                    */
/* ------------------------------------------------------------------ */
static AstVarType check_expr(Tc *tc, AstNode *e);

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
        if (lt == AST_TY_UNKNOWN || rt == AST_TY_UNKNOWN || lt != rt) {
            fail_at(tc, e->line, e->col,
                    "relational operands must be the same type "
                    "(both integer or both boolean)");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_BOOLEAN;
        return e->type;
    case OP_NEG: case OP_NOT:
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
    case AST_VARREF: {
        char lc[TC_SYM_NAME_CAP];
        lower_copy(lc, sizeof(lc), e->as.varref.name);
        int idx = sym_find(tc, lc);
        if (idx < 0) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg), "undeclared variable: %s",
                     e->as.varref.name);
            fail_at(tc, e->line, e->col, msg);
            return AST_TY_UNKNOWN;
        }
        e->type = tc->syms[idx].type;
        return e->type;
    }
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
        char lc[TC_SYM_NAME_CAP];
        lower_copy(lc, sizeof(lc), n->as.assign.name);
        int idx = sym_find(tc, lc);
        if (idx < 0) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg), "undeclared variable: %s",
                     n->as.assign.name);
            fail_at(tc, n->line, n->col, msg);
            return;
        }
        AstVarType vt = tc->syms[idx].type;
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
    case AST_WRITE:
    case AST_WRITELN:
        for (size_t i = 0; i < n->as.write.args.count && !tc->failed; i++) {
            AstNode *arg = n->as.write.args.items[i];
            if (arg->kind == AST_STRLIT)
                continue; /* string args are never type-checked as exprs */
            AstVarType t = check_expr(tc, arg);
            if (tc->failed)
                return;
            if (t != AST_TY_INTEGER && t != AST_TY_BOOLEAN) {
                fail_at(tc, arg->line, arg->col,
                        "write/writeln argument must be integer or "
                        "boolean");
                return;
            }
        }
        return;
    default:
        fail_at(tc, n->line, n->col,
                "internal: unexpected statement node in typecheck");
        return;
    }
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

    collect_decls(&tc, &program->as.program.decls);
    if (!tc.failed)
        check_stmt(&tc, program->as.program.block);

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
