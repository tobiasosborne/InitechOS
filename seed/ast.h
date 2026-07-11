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
 * procedures/functions, records, pointers, char/real types, more builtins.
 * They are intentionally absent so the surface stays small.
 *
 * B1 addition (beads initech-f0uc; ADR-0007 DEC-02/DEC-03): relational
 * operators (= <> < <= > >=), a `boolean` type, and `and`/`or`/`not` under
 * COMPLETE evaluation (both operands of and/or always evaluate -- see
 * seed/codegen.c gen_binop, which has no conditional-branch path for any
 * binop, incl. AND/OR). Every expression node's `type` field is computed by
 * seed/typecheck.c (a separate pass run after a successful parse, before
 * codegen) and consumed by codegen to choose integer vs boolean printing for
 * write/writeln args. AST_VARDECL's `vtype` is set directly by the parser
 * (the declared type keyword is already consumed there); it is distinct from
 * the generic per-expression `type` field.
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
/* Semantic (value) types                                             */
/* ------------------------------------------------------------------ */
/* B1 (beads initech-f0uc; ADR-0007 DEC-02). AST_TY_UNKNOWN (0, the ast_new
 * memset default) marks an expression node whose type has not yet been
 * computed by seed/typecheck.c -- it is never a valid final type for a
 * successfully typechecked program. */
typedef enum {
    AST_TY_UNKNOWN = 0,
    AST_TY_INTEGER,
    AST_TY_BOOLEAN
} AstVarType;

/* Human-readable name for a semantic type (diagnostics, dumps). */
const char *ast_vartype_name(AstVarType t);

/* ------------------------------------------------------------------ */
/* Node kinds                                                         */
/* ------------------------------------------------------------------ */
typedef enum {
    AST_PROGRAM,   /* program <name>; <block> . */
    AST_VARDECL,   /* one "name1, name2 : integer;" (or ":boolean") group */
    AST_BLOCK,     /* begin <stmt>* end -- a compound statement */
    AST_ASSIGN,    /* <name> := <expr> */
    AST_WRITE,     /* write(<arg>*)   -- is_newline == 0 */
    AST_WRITELN,   /* writeln(<arg>*) -- is_newline == 1 (shared node kind) */
    AST_BINOP,     /* <lhs> <op> <rhs> */
    AST_UNOP,      /* <op> <operand>  (unary minus, 'not') */
    AST_INTLIT,    /* integer literal */
    AST_BOOLLIT,   /* 'true' / 'false' literal (B1) */
    AST_STRLIT,    /* string literal (write/writeln args only) */
    AST_VARREF     /* reference to a variable by name */
} AstKind;

typedef enum {
    OP_ADD,   /* + */
    OP_SUB,   /* - */
    OP_MUL,   /* * */
    OP_DIV,   /* div */
    OP_MOD,   /* mod */
    OP_NEG,   /* unary - */
    /* B1 (beads initech-f0uc): relational (non-chaining, lowest precedence)
     * + boolean and/or/not (complete evaluation -- ADR-0007 DEC-03). */
    OP_EQ,    /* = */
    OP_NE,    /* <> */
    OP_LT,    /* < */
    OP_LE,    /* <= */
    OP_GT,    /* > */
    OP_GE,    /* >= */
    OP_AND,   /* and */
    OP_OR,    /* or */
    OP_NOT    /* not (unary) */
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
    AstKind    kind;
    int        line;   /* 1-based source location of the construct */
    int        col;
    /* Semantic type of an EXPRESSION node (BINOP/UNOP/INTLIT/BOOLLIT/VARREF),
     * computed by seed/typecheck.c after a successful parse and consumed by
     * codegen (e.g. to pick integer vs boolean write/writeln printing).
     * AST_TY_UNKNOWN until typecheck runs (B1, beads initech-f0uc). Unused
     * (stays AST_TY_UNKNOWN) on non-expression node kinds. */
    AstVarType type;
    union {
        struct { char *name; AstList decls; AstNode *block; } program;
        /* vtype: the DECLARED type of this group ("integer"/"boolean"),
         * set by the parser when it consumes the type keyword -- distinct
         * from the generic per-expression `type` field above. */
        struct { AstList names; /* AST_VARREF nodes */ AstVarType vtype; } vardecl;
        struct { AstList stmts; } block;
        struct { char *name; AstNode *value; } assign;
        struct { AstList args; int is_newline; } write;
        struct { AstOp op; AstNode *lhs; AstNode *rhs; } binop;
        struct { AstOp op; AstNode *operand; } unop;
        struct { long value; } intlit;
        struct { int value; } boollit;                 /* 0 or 1 (B1) */
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
