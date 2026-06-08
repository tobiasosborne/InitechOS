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
            fputs(":integer", fp);
        }
        fputc(')', fp);
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
        fprintf(fp, "(assign %s ", n->as.assign.name);
        dump(n->as.assign.value, fp);
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
    case AST_STRLIT:
        fputs("(str ", fp);
        dump_str_payload(fp, n->as.strlit.text, n->as.strlit.length);
        fputc(')', fp);
        break;
    case AST_VARREF:
        fprintf(fp, "(varref %s)", n->as.varref.name);
        break;
    }
}

void ast_dump(const AstNode *node, void *fp)
{
    dump(node, (FILE *)fp);
}
