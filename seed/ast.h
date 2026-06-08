/*
 * ast.h -- AST node types for the seed Pascal front-end scaffold subset.
 *
 * beads: initech-znb ("Step A of the InitechOS seed cross-compiler")
 * Ref:   PRD Sec 6.7 (the language), PRD Sec 4 (seed targets the same
 *        language as the resident compiler). CLAUDE.md Law 1 / Rule 12.
 *
 * OWNERSHIP STORY (DECIDED):
 * All AST nodes and the strings they reference are allocated from a single
 * bump/arena allocator (AstArena). The parser allocates exclusively from the
 * arena. To free the whole tree you call ast_arena_free(arena) ONCE -- there
 * is no per-node free, no reference counting, no aliasing hazard. This matches
 * a single-pass compiler's lifetime model (the AST lives for one compilation)
 * and keeps the parser allocation-error story trivial: an arena OOM aborts the
 * process via the arena (a build-tool fatal, CLAUDE.md Rule 2 fail-loud), so
 * parser code never has to thread allocation failures.
 *
 * Strings (identifiers, decoded string literals) are copied into the arena and
 * NUL-terminated, so nodes do not alias the (possibly transient) lexer buffer.
 *
 * Scope NOTE: minimal scaffold only. Deferred (later steps): if/while/for,
 * procedures/functions, records, pointers, boolean/char/real types, more
 * builtins. They are intentionally absent so the surface stays small.
 */
#ifndef SEED_AST_H
#define SEED_AST_H

#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Arena                                                              */
/* ------------------------------------------------------------------ */
typedef struct AstArenaBlock AstArenaBlock;

typedef struct {
    AstArenaBlock *head;  /* singly-linked list of allocation blocks */
} AstArena;

void  ast_arena_init(AstArena *a);
void *ast_arena_alloc(AstArena *a, size_t size);     /* aligned; aborts on OOM */
char *ast_arena_strndup(AstArena *a, const char *s, size_t n);
void  ast_arena_free(AstArena *a);                   /* frees the whole tree */

/* ------------------------------------------------------------------ */
/* Node kinds                                                         */
/* ------------------------------------------------------------------ */
typedef enum {
    AST_PROGRAM,   /* program <name>; <block> . */
    AST_VARDECL,   /* one "name1, name2 : integer;" group */
    AST_BLOCK,     /* begin <stmt>* end -- a compound statement */
    AST_ASSIGN,    /* <name> := <expr> */
    AST_WRITE,     /* write(<arg>*)   -- is_newline == 0 */
    AST_WRITELN,   /* writeln(<arg>*) -- is_newline == 1 (shared node kind) */
    AST_BINOP,     /* <lhs> <op> <rhs> */
    AST_UNOP,      /* <op> <operand>  (unary minus only, for now) */
    AST_INTLIT,    /* integer literal */
    AST_STRLIT,    /* string literal (write/writeln args only) */
    AST_VARREF     /* reference to a variable by name */
} AstKind;

typedef enum {
    OP_ADD,   /* + */
    OP_SUB,   /* - */
    OP_MUL,   /* * */
    OP_DIV,   /* div */
    OP_MOD,   /* mod */
    OP_NEG    /* unary - */
} AstOp;

typedef struct AstNode AstNode;

/* A simple growable node-pointer list (arena-backed). Used for the var-decl
 * list, a block's statements, a vardecl's names, and a write's arguments. */
typedef struct {
    AstNode **items;
    size_t    count;
    size_t    cap;
} AstList;

struct AstNode {
    AstKind kind;
    int     line;   /* 1-based source location of the construct */
    int     col;
    union {
        struct { char *name; AstList decls; AstNode *block; } program;
        struct { AstList names; /* AST_VARREF nodes */ } vardecl;
        struct { AstList stmts; } block;
        struct { char *name; AstNode *value; } assign;
        struct { AstList args; int is_newline; } write;
        struct { AstOp op; AstNode *lhs; AstNode *rhs; } binop;
        struct { AstOp op; AstNode *operand; } unop;
        struct { long value; } intlit;
        struct { char *text; size_t length; } strlit; /* decoded, NUL-term */
        struct { char *name; } varref;
    } as;
};

/* Node constructors (allocate from the arena). */
AstNode *ast_new(AstArena *a, AstKind kind, int line, int col);

/* List helpers (arena-backed; abort on OOM). */
void ast_list_init(AstList *l);
void ast_list_push(AstArena *a, AstList *l, AstNode *node);

/* Pretty-print the tree as a compact S-expression to a FILE. */
void ast_dump(const AstNode *node, void *fp /* FILE* */);

/* Name of an operator, for dumps ("+", "div", "neg", ...). */
const char *ast_op_name(AstOp op);

#endif /* SEED_AST_H */
