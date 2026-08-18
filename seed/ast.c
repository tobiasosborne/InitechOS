/*
 * ast.c -- arena allocator, node constructors, and S-expression dump for the
 *          seed Pascal front-end AST.
 *
 * beads: initech-znb ("Step A of the InitechOS seed cross-compiler")
 * Ref:   PRD Sec 6.7 (the language), PRD Sec 4 (seed targets the same language
 *        as the resident compiler). CLAUDE.md Law 1 / Rule 2 / Rule 12.
 *
 * Ownership: see the header. One bump arena owns every node and string; the
 * whole tree is freed by ast_arena_free(). OOM is a loud process abort
 * (CLAUDE.md Rule 2 -- fail fast, fail loud), so parser code never threads
 * allocation failures.
 *
 * ASCII-clean (Rule 12). No timestamps / nondeterminism (Rule 11): the dump is
 * a pure function of the tree, so test goldens are stable.
 */
#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Arena                                                              */
/* ------------------------------------------------------------------ */
/*
 * The arena is a singly-linked list of blocks. Each block carries a fixed-size
 * payload region; allocations bump a cursor. A request larger than the default
 * block size gets its own exact-fit block. Alignment is to max_align_t.
 */
#define AST_ARENA_BLOCK_PAYLOAD (64u * 1024u)

struct AstArenaBlock {
    AstArenaBlock *next;
    size_t         used;
    size_t         cap;
    /* payload follows immediately after this header */
    char           data[];
};

/* Round n up to a multiple of the platform's max alignment. */
static size_t align_up(size_t n)
{
    const size_t a = sizeof(max_align_t);
    return (n + (a - 1)) & ~(a - 1);
}

static void arena_fatal_oom(size_t want)
{
    /* Rule 2: a build-tool fatal. No recovery path. */
    fprintf(stderr, "initechc: fatal: AST arena out of memory "
            "(requested %zu bytes)\n", want);
    abort();
}

static AstArenaBlock *arena_new_block(size_t payload)
{
    if (payload < AST_ARENA_BLOCK_PAYLOAD)
        payload = AST_ARENA_BLOCK_PAYLOAD;
    AstArenaBlock *b = malloc(sizeof(*b) + payload);
    if (!b)
        arena_fatal_oom(sizeof(*b) + payload);
    b->next = NULL;
    b->used = 0;
    b->cap = payload;
    return b;
}

void ast_arena_init(AstArena *a)
{
    a->head = NULL;
}

void *ast_arena_alloc(AstArena *a, size_t size)
{
    size = align_up(size ? size : 1);

    /* Fit into the current head block if possible. */
    if (a->head && a->head->used + size <= a->head->cap) {
        void *p = a->head->data + a->head->used;
        a->head->used += size;
        return p;
    }

    /* Otherwise push a fresh block big enough for this request. */
    AstArenaBlock *b = arena_new_block(size);
    b->next = a->head;
    a->head = b;
    void *p = b->data;
    b->used = size;
    return p;
}

char *ast_arena_strndup(AstArena *a, const char *s, size_t n)
{
    char *p = ast_arena_alloc(a, n + 1);
    if (n)
        memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

void ast_arena_free(AstArena *a)
{
    AstArenaBlock *b = a->head;
    while (b) {
        AstArenaBlock *next = b->next;
        free(b);
        b = next;
    }
    a->head = NULL;
}

/* ------------------------------------------------------------------ */
/* Node + list constructors                                           */
/* ------------------------------------------------------------------ */
AstNode *ast_new(AstArena *a, AstKind kind, int line, int col)
{
    AstNode *n = ast_arena_alloc(a, sizeof(*n));
    memset(n, 0, sizeof(*n));
    n->kind = kind;
    n->line = line;
    n->col = col;
    /* B7 (beads initech-39k2): -1 = "no string temporary assigned" (the memset
     * default 0 is a valid temp index, so the unused sentinel must be set
     * explicitly). The codegen pre-walk overwrites this for every string-
     * materializing node it visits. */
    n->str_temp = -1;
    return n;
}

void ast_list_init(AstList *l)
{
    l->items = NULL;
    l->count = 0;
    l->cap = 0;
}

void ast_list_push(AstArena *a, AstList *l, AstNode *node)
{
    if (l->count == l->cap) {
        size_t newcap = l->cap ? l->cap * 2 : 4;
        AstNode **items = ast_arena_alloc(a, newcap * sizeof(*items));
        if (l->count)
            memcpy(items, l->items, l->count * sizeof(*items));
        l->items = items;
        l->cap = newcap;
    }
    l->items[l->count++] = node;
}

/* ------------------------------------------------------------------ */
/* Operator names                                                     */
/* ------------------------------------------------------------------ */
const char *ast_op_name(AstOp op)
{
    switch (op) {
    case OP_ADD: return "+";
    case OP_SUB: return "-";
    case OP_MUL: return "*";
    case OP_DIV: return "div";
    case OP_MOD: return "mod";
    case OP_NEG: return "neg";
    case OP_EQ:  return "=";
    case OP_NE:  return "<>";
    case OP_LT:  return "<";
    case OP_LE:  return "<=";
    case OP_GT:  return ">";
    case OP_GE:  return ">=";
    case OP_AND: return "and";
    case OP_OR:  return "or";
    case OP_NOT: return "not";
    case OP_ORD: return "ord";
    case OP_CHR: return "chr";
    case OP_LENGTH: return "length"; /* B7 (beads initech-39k2) */
    }
    return "?";
}

const char *ast_vartype_name(AstVarType t)
{
    switch (t) {
    case AST_TY_UNKNOWN: return "?";
    case AST_TY_INTEGER: return "integer";
    case AST_TY_BOOLEAN: return "boolean";
    case AST_TY_CHAR:    return "char";
    case AST_TY_RECORD:  return "record";
    case AST_TY_STRING:  return "string"; /* B7 (beads initech-39k2) */
    case AST_TY_FILE:    return "file";   /* B8 (beads initech-ogxv) */
    }
    return "?";
}

/* ------------------------------------------------------------------ */
/* S-expression dump                                                  */
/* ------------------------------------------------------------------ */
/*
 * The dump is the parser's testable contract (CLAUDE.md Law 2): test_parser.c
 * asserts exact strings. Keep the format stable. A string literal is emitted
 * as "..." with embedded double-quotes and backslashes escaped so the dump is
 * unambiguous; the subset's source strings are single-quoted, so this only
 * matters for completeness.
 */
static void dump_str_payload(FILE *fp, const char *s, size_t len)
{
    fputc('"', fp);
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (c == '"' || c == '\\')
            fputc('\\', fp);
        fputc(c, fp);
    }
    fputc('"', fp);
}

static void dump(const AstNode *n, FILE *fp)
{
    if (!n) {
        fputs("(nil)", fp);
        return;
    }
    switch (n->kind) {
    case AST_PROGRAM:
        fprintf(fp, "(program %s", n->as.program.name);
        for (size_t i = 0; i < n->as.program.decls.count; i++) {
            fputc(' ', fp);
            dump(n->as.program.decls.items[i], fp);
        }
        fputc(' ', fp);
        dump(n->as.program.block, fp);
        fputc(')', fp);
        break;
    case AST_VARDECL:
        fputs("(var", fp);
        for (size_t i = 0; i < n->as.vardecl.names.count; i++) {
            fputc(' ', fp);
            dump(n->as.vardecl.names.items[i], fp);
            /* B5 (beads initech-54uu): an array group dumps its bounds +
             * element type instead of a bare scalar type. B6 (beads
             * initech-rug7): a RECORD-typed group (scalar or array element)
             * dumps the record type NAME instead of the bare "record" tag,
             * so the S-expression distinguishes two different record types. */
            const char *tyname = n->as.vardecl.rectype ? n->as.vardecl.rectype
                                : ast_vartype_name(n->as.vardecl.vtype);
            if (n->as.vardecl.is_array)
                fprintf(fp, ":array[%ld..%ld] of %s", n->as.vardecl.lo,
                        n->as.vardecl.hi, tyname);
            else if (n->as.vardecl.vtype == AST_TY_STRING)
                /* B7 (beads initech-39k2): a string var dumps its capacity. */
                fprintf(fp, ":string[%d]", n->as.vardecl.strcap);
            else
                fprintf(fp, ":%s", tyname);
        }
        fputc(')', fp);
        break;
    case AST_CONSTDECL:
        fprintf(fp, "(const %s:%s)", n->as.constdecl.name,
                ast_vartype_name(n->as.constdecl.ctype));
        break;
    case AST_BLOCK:
        fputs("(block", fp);
        for (size_t i = 0; i < n->as.block.stmts.count; i++) {
            fputc(' ', fp);
            dump(n->as.block.stmts.items[i], fp);
        }
        fputc(')', fp);
        break;
    case AST_ASSIGN:
        /* B5 (beads initech-54uu): an indexed assignment dumps the index
         * expression too, so the S-expression contract distinguishes
         * "a := v" from "a[i] := v". B6 (beads initech-rug7): a FIELD
         * assignment additionally dumps ".field"; a WHOLE-RECORD assignment
         * (rec_fields > 0) is tagged "assign-record" so the contract
         * distinguishes it from an ordinary scalar/indexed assignment. */
        if (n->as.assign.field) {
            if (n->as.assign.index) {
                fprintf(fp, "(assign-index-field %s ", n->as.assign.name);
                dump(n->as.assign.index, fp);
                fprintf(fp, " %s ", n->as.assign.field);
            } else {
                fprintf(fp, "(assign-field %s %s ", n->as.assign.name,
                        n->as.assign.field);
            }
            dump(n->as.assign.value, fp);
        } else if (n->as.assign.rec_fields > 0) {
            if (n->as.assign.index) {
                fprintf(fp, "(assign-record-index %s ", n->as.assign.name);
                dump(n->as.assign.index, fp);
                fputc(' ', fp);
            } else {
                fprintf(fp, "(assign-record %s ", n->as.assign.name);
            }
            dump(n->as.assign.value, fp);
        } else if (n->as.assign.index) {
            fprintf(fp, "(assign-index %s ", n->as.assign.name);
            dump(n->as.assign.index, fp);
            fputc(' ', fp);
            dump(n->as.assign.value, fp);
        } else {
            fprintf(fp, "(assign %s ", n->as.assign.name);
            dump(n->as.assign.value, fp);
        }
        fputc(')', fp);
        break;
    case AST_IF:
        fputs("(if ", fp);
        dump(n->as.ifstmt.cond, fp);
        fputc(' ', fp);
        dump(n->as.ifstmt.then_stmt, fp);
        if (n->as.ifstmt.else_stmt) {
            fputc(' ', fp);
            dump(n->as.ifstmt.else_stmt, fp);
        }
        fputc(')', fp);
        break;
    case AST_WHILE:
        fputs("(while ", fp);
        dump(n->as.whilestmt.cond, fp);
        fputc(' ', fp);
        dump(n->as.whilestmt.body, fp);
        fputc(')', fp);
        break;
    case AST_WRITE:
    case AST_WRITELN:
        fputs(n->as.write.is_newline ? "(writeln" : "(write", fp);
        for (size_t i = 0; i < n->as.write.args.count; i++) {
            fputc(' ', fp);
            dump(n->as.write.args.items[i], fp);
        }
        fputc(')', fp);
        break;
    case AST_BINOP:
        fprintf(fp, "(%s ", ast_op_name(n->as.binop.op));
        dump(n->as.binop.lhs, fp);
        fputc(' ', fp);
        dump(n->as.binop.rhs, fp);
        fputc(')', fp);
        break;
    case AST_UNOP:
        fprintf(fp, "(%s ", ast_op_name(n->as.unop.op));
        dump(n->as.unop.operand, fp);
        fputc(')', fp);
        break;
    case AST_INTLIT:
        fprintf(fp, "(int %ld)", n->as.intlit.value);
        break;
    case AST_BOOLLIT:
        fprintf(fp, "(bool %s)", n->as.boollit.value ? "true" : "false");
        break;
    case AST_CHARLIT:
        fprintf(fp, "(char %d)", n->as.charlit.value);
        break;
    case AST_STRLIT:
        fputs("(str ", fp);
        dump_str_payload(fp, n->as.strlit.text, n->as.strlit.length);
        fputc(')', fp);
        break;
    case AST_VARREF:
        fprintf(fp, "(varref %s)", n->as.varref.name);
        break;
    /* B4 (beads initech-63ce). */
    case AST_PARAM:
        /* B7 (beads initech-39k2): a `var string` parameter dumps its
         * capacity (always 255 -- the bare-string-only formal rule). */
        if (n->as.param.ptype == AST_TY_STRING)
            fprintf(fp, "(param %s%s:string[%d])",
                    n->as.param.is_var ? "var " : "", n->as.param.name,
                    n->as.param.strcap);
        else
            fprintf(fp, "(param %s%s:%s)", n->as.param.is_var ? "var " : "",
                    n->as.param.name,
                    n->as.param.rectype ? n->as.param.rectype
                                        : ast_vartype_name(n->as.param.ptype));
        break;
    case AST_PROCDECL:
    case AST_FUNCDECL:
        if (n->as.procfunc.has_result)
            fprintf(fp, "(func %s:%s", n->as.procfunc.name,
                    ast_vartype_name(n->as.procfunc.rettype));
        else
            fprintf(fp, "(proc %s", n->as.procfunc.name);
        fputs(" (params", fp);
        for (size_t i = 0; i < n->as.procfunc.params.count; i++) {
            fputc(' ', fp);
            dump(n->as.procfunc.params.items[i], fp);
        }
        fputc(')', fp);
        fputs(" (decls", fp);
        for (size_t i = 0; i < n->as.procfunc.decls.count; i++) {
            fputc(' ', fp);
            dump(n->as.procfunc.decls.items[i], fp);
        }
        fputc(')', fp);
        if (n->as.procfunc.is_forward) {
            fputs(" forward", fp);
        } else {
            fputc(' ', fp);
            dump(n->as.procfunc.body, fp);
        }
        fputc(')', fp);
        break;
    case AST_CALL:
        fprintf(fp, "(call %s", n->as.call.name);
        for (size_t i = 0; i < n->as.call.args.count; i++) {
            fputc(' ', fp);
            dump(n->as.call.args.items[i], fp);
        }
        fputc(')', fp);
        break;
    /* B5 (beads initech-54uu). */
    case AST_INDEX:
        fprintf(fp, "(index %s ", n->as.arrayindex.name);
        dump(n->as.arrayindex.index, fp);
        fputc(')', fp);
        break;
    /* B6 (beads initech-rug7). */
    case AST_TYPEDECL:
        fprintf(fp, "(type %s (fields", n->as.typedecl.name);
        for (size_t i = 0; i < n->as.typedecl.fields.count; i++) {
            fputc(' ', fp);
            dump(n->as.typedecl.fields.items[i], fp);
        }
        fputs("))", fp);
        break;
    case AST_FIELD:
        fputs("(field ", fp);
        dump(n->as.field.base, fp);
        fprintf(fp, " %s)", n->as.field.field);
        break;
    }
}

void ast_dump(const AstNode *node, void *fp)
{
    dump(node, (FILE *)fp);
}
