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
 *
 * B2 addition (beads initech-80iw; ADR-0007 DEC-02): AST_IF and AST_WHILE
 * are the only new NODE KINDS -- the two PRIMITIVE control-flow statements.
 * `for`/`repeat` are sugar (ADR-0007 DEC-02's own text): seed/parser.c
 * desugars them entirely at parse time into AST_ASSIGN/AST_WHILE/AST_BLOCK/
 * AST_UNOP(NOT) nodes that already exist, so codegen never sees a "for" or
 * "repeat" node and needs no new machinery for them (the ADR's explicit
 * point: sugar "desugar[s] to the primitive forms ... rather than requiring
 * new codegen machinery"). `case` is not implemented (optional per DEC-02).
 *
 * B3 addition (beads initech-7mo3; ADR-0007 DEC-02 "const declarations",
 * "char, ord, chr"):
 *   - AST_CONSTDECL: one "NAME = <literal>;" const declaration. Unlike
 *     AST_VARDECL (which names a LIST of variables sharing one .bss slot
 *     per name), a const-decl names exactly ONE constant and carries no
 *     runtime value at all -- the value is folded into a fresh literal AST
 *     node (AST_INTLIT/AST_CHARLIT/AST_BOOLLIT) at every USE SITE by
 *     seed/parser.c (see parse_const_section / parse_factor's TOK_IDENT
 *     branch), so codegen never emits a .bss slot for a const (seed/
 *     codegen.c emit_bss skips AST_CONSTDECL nodes) -- "front-end FOLD to
 *     literals at use sites, no runtime storage" per the bead. The node
 *     still appears in AST_PROGRAM's decls list purely so
 *     seed/typecheck.c's collect_decls can enforce the SAME
 *     case-insensitive one-declaration rule across const AND var names in
 *     one flat namespace (a const can collide with a var, or with another
 *     const, exactly like two colliding var-decls today).
 *   - AST_CHARLIT: a char literal, e.g. 'A'. DISAMBIGUATION (DECISION,
 *     report): the lexer already lexes '...' as TOK_STRING for every
 *     quoted literal regardless of length (write/writeln string args
 *     included). This subset's minimal sound rule: a TOK_STRING token
 *     reached from an EXPRESSION context (parse_factor -- assignments,
 *     conditions, const values, ord/chr arguments, operands of relational
 *     ops, etc.) must be exactly ONE character, and becomes an
 *     AST_CHARLIT; a TOK_STRING reached from a write/writeln ARGUMENT
 *     position (parsed directly by parse_write, which special-cases
 *     TOK_STRING before ever calling parse_expr) becomes an AST_STRLIT of
 *     whatever length it has, unconditionally, exactly as before B3. This
 *     mirrors real Turbo Pascal: a length-1 '...' literal is Char-typed
 *     wherever a Char is expected (ISO 7185 Sec 6.1.7 / Turbo Pascal
 *     Language Guide: "a string-literal enclosed in one pair of quotes
 *     that specifies a single character... is of type Char"), while
 *     `write`/`writeln` treat ANY quoted literal as its string form.
 *     A zero- or multi-character '...' token reaching parse_factor is a
 *     located syntax error (this subset has no general string EXPRESSION
 *     type yet -- fixed/ShortString strings land at B7).
 *   - OP_ORD / OP_CHR (AstOp, below): ord()/chr() reuse AST_UNOP (one
 *     operand, one result) rather than a general function-call AST kind --
 *     this subset has no user-callable functions yet (B4 introduces
 *     procedures/functions); ord/chr are recognized as RESERVED KEYWORDS by
 *     the parser (see token.h's B3 note), each parsing "( expr )" directly.
 *     Both are VALUE NO-OPS at codegen (char/boolean/integer already share
 *     one zero-extended 0..255-or-32-bit representation in eax -- B1); see
 *     seed/codegen.c's OP_ORD/OP_CHR cases for chr()'s 8-bit-truncation
 *     DECISION (out-of-range chr() truncates, it does not trap).
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
    AST_TY_BOOLEAN,
    /* B3 (beads initech-7mo3; ADR-0007 DEC-02 "char"). char is a DISTINCT
     * scalar type -- no implicit coercion to/from integer; ord()/chr() are
     * the only bridges (see typecheck.c). */
    AST_TY_CHAR
} AstVarType;

/* Human-readable name for a semantic type (diagnostics, dumps). */
const char *ast_vartype_name(AstVarType t);

/* ------------------------------------------------------------------ */
/* Node kinds                                                         */
/* ------------------------------------------------------------------ */
typedef enum {
    AST_PROGRAM,   /* program <name>; <block> . */
    AST_VARDECL,   /* one "name1, name2 : integer;" (or ":boolean") group */
    /* B3 (beads initech-7mo3): one "NAME = <literal>;" const declaration.
     * Folded away by codegen (no .bss slot); kept only for typecheck's
     * one-declaration-rule bookkeeping -- see this header's B3 comment. */
    AST_CONSTDECL,
    AST_BLOCK,     /* begin <stmt>* end -- a compound statement */
    AST_ASSIGN,    /* <name> := <expr> */
    /* B2 (beads initech-80iw): the two primitive control-flow statements.
     * for/repeat are sugar, desugared to these + AST_BLOCK/AST_ASSIGN/
     * AST_UNOP(NOT) at parse time -- see parser.c and this header's B2
     * comment above. */
    AST_IF,        /* if <cond> then <then_stmt> [else <else_stmt>] */
    AST_WHILE,     /* while <cond> do <body> */
    AST_WRITE,     /* write(<arg>*)   -- is_newline == 0 */
    AST_WRITELN,   /* writeln(<arg>*) -- is_newline == 1 (shared node kind) */
    AST_BINOP,     /* <lhs> <op> <rhs> */
    AST_UNOP,      /* <op> <operand>  (unary minus, 'not') */
    AST_INTLIT,    /* integer literal */
    AST_BOOLLIT,   /* 'true' / 'false' literal (B1) */
    AST_CHARLIT,   /* char literal, e.g. 'A' (B3; see this header's note) */
    AST_STRLIT,    /* string literal (write/writeln args only) */
    AST_VARREF,    /* reference to a variable by name */
    /* B4 (beads initech-63ce; ADR-0007 DEC-02 "procedures/functions: nested
     * scopes, both value and var parameters, recursion, results", plus the
     * `forward` directive per the DEC-02 ratification amendment) -- THE
     * CODEGEN PIVOT. Four new node kinds; the frame model + calling
     * convention are documented at the top of seed/codegen.c.
     *   AST_PROCDECL / AST_FUNCDECL: a top-level (flat, NOT lexically
     *     nested -- see the DECISION note below) procedure or function
     *     declaration. Both use the `procfunc` union member. A function
     *     (AST_FUNCDECL) additionally carries a result type and returns via
     *     the `name := expr` assignment (the ISO 7185 / Turbo Pascal
     *     result-variable idiom; the TP7 `Result` pseudo-variable is NOT in
     *     this subset -- see the DECISION note below).
     *   AST_PARAM: one formal parameter (name, type, by-ref flag). Parameter
     *     groups like "(a, b: integer)" are FLATTENED into one AST_PARAM per
     *     name at parse time so each parameter owns a deterministic frame
     *     offset [ebp+8+4i] (codegen.c).
     *   AST_CALL: a call by name with an argument list. Used BOTH as an
     *     expression (a function call -- its `type` field is the callee's
     *     result type) and as a statement (a procedure call). Whether an
     *     argument is passed by value or by address is decided at the call
     *     site from the CALLEE's signature (typecheck resolves it; codegen
     *     emits an address for a var parameter, a value for a value
     *     parameter).
     *
     * DECISION (report, ADR ambiguity): ADR-0007 DEC-02 says "nested
     * scopes". This subset reads that as "each procedure/function introduces
     * its OWN scope over its parameters + locals, with the program-level
     * globals visible" -- NOT lexically-nested procedure DECLARATIONS with
     * uplevel/non-local addressing (a static link or display). The ADR never
     * specifies static-link/display machinery, DEC-04 favours "small and
     * sufficient" single-pass stack-machine codegen, and a self-hosting
     * compiler is expressible with flat top-level routines + globals. So
     * procedures/functions are TOP-LEVEL only; a routine's body sees its own
     * params/locals (which may shadow a global) and the program globals.
     *
     * DECISION (report, ADR silent): a function returns its result ONLY via
     * assignment to the function name (`Fact := ...`); the TP7 extended-
     * syntax `Result` pseudo-variable is NOT implemented (the ADR is silent;
     * name-assignment is the ISO 7185 / TP7-default form -- minimal).
     *
     * DECISION (report): a call ALWAYS uses parentheses (`Foo(args)`); a
     * bare identifier is ALWAYS a variable/const/function-result reference.
     * This removes the "bare function name = call vs result variable"
     * ambiguity Pascal otherwise has for param-less routines, for zero
     * self-host cost (mirrors true/false/ord/chr being reserved at B1/B3 for
     * the same minimality reason, ADR-0007 DEC-02/DR-2). */
    AST_PROCDECL,
    AST_FUNCDECL,
    AST_PARAM,
    AST_CALL,
    /*
     * B5 (beads initech-54uu; ADR-0007 DEC-02 "static arrays (array[lo..hi]
     * of T), indexed as both l-value and r-value"):
     *   AST_INDEX: `name[index]` used as an EXPRESSION (r-value) -- reading
     *     an array element, or as a `var`-parameter call argument (its
     *     ELEMENT address is passed -- codegen.c's gen_addr_of/
     *     gen_elem_addr). `name` must resolve to a declared ARRAY (a global
     *     .bss block or a frame-resident local -- see AST_VARDECL's
     *     `is_array`/`lo`/`hi` fields below); `index` is any INTEGER
     *     expression, including a nested call (`a[Compute(i)]`).
     *
     *   INDEXED ASSIGNMENT (l-value) reuses AST_ASSIGN rather than adding a
     *   sibling node kind: `as.assign.index` is NULL for an ordinary scalar
     *   assignment (`name := value`, every pre-B5 use) and non-NULL for an
     *   indexed one (`name[index] := value`) -- every existing AST_ASSIGN
     *   call site just gains an `if (index) ... else ...` branch instead of
     *   a whole new statement kind.
     *
     *   DECISION (report, ADR silent -- the bead's own "decide minimal,
     *   record" point): arrays are NOT first-class parameter types here --
     *   no "var arr: array[1..N] of integer" parameter form (whole-array by
     *   reference or by value). AST_PARAM's `ptype` stays the pre-B5 SCALAR
     *   AstVarType. What IS supported (required by "indexed as ... l-value",
     *   composed with B4's var parameters): passing ONE INDEXED ELEMENT
     *   (`a[i]`) as a `var` parameter argument -- that passes the element's
     *   ADDRESS (AST_INDEX reused as a call argument; typecheck.c's
     *   check_call var-param branch; codegen.c's gen_addr_of). Rationale:
     *   this seed's own routines are flat + operate on globals (B4's
     *   DECISION note above), so a self-hosting compiler never NEEDS to pass
     *   a whole symbol-table array by reference -- it references the global
     *   array directly, or passes one element. Full array parameters
     *   (bounds would need to travel with the argument, or be erased
     *   Pascal-open-array style) are a materially bigger feature, deferred
     *   past B5, not silently absorbed.
     *
     *   DECISION (report): multi-dimensional arrays (`array[1..3,1..3] of
     *   T` or arrays-of-arrays) are NOT implemented. ADR-0007 DEC-02 asks
     *   only for "static arrays (array[lo..hi] of T)" -- one dimension --
     *   and docs/plans/TPS-M7-subset-plan.md's B6 note ("arrays-of-arrays
     *   arrive implicitly via records-of-arrays if needed") places any
     *   nested-container need at B6 (records), not here. This seed's own
     *   bootstrap needs (symbol tables, source buffers -- the bead's own
     *   oracle framing) are naturally one-dimensional. Left OUT, not
     *   silently absorbed -- revisit only if B6/B7 genuinely need it.
     *
     *   DECISION (report, ADR silent -- Turbo Pascal's own default): NO
     *   runtime bounds checking. Real Turbo Pascal range-checks only under
     *   {$R+}; {$R-} (no checking) is the DEFAULT -- the same precedent this
     *   subset already used for chr()'s out-of-range TRUNCATION (codegen.c's
     *   OP_CHR case, cited to the Borland Turbo Pascal 7.0 Language Guide).
     *   TENSION WITH RULE 2 (recorded, not resolved away): an out-of-range
     *   index is a SILENT out-of-bounds memory access -- in this compiler's
     *   own self-hosted future use, a bad symbol-table index would corrupt
     *   an adjacent frame slot or .bss variable with no diagnostic. The
     *   mitigation is a SOURCE DISCIPLINE, not a codegen feature (mirrors
     *   DEC-03's nested-if idiom): self-host-critical Pascal (Turbo
     *   Initech's own source, B9) must use FIXED-CAPACITY arrays sized by
     *   named limit constants plus EXPLICIT guards before every index --
     *   the identical discipline ADR-0007 DEC-02's ratification amendment
     *   already mandates for the compiler's own internal collections
     *   ("capacity overflow is a Rule-2 fail-loud diagnostic ... never a
     *   silent truncation or wraparound"). Recorded here so a future reader
     *   does not mistake the omission for an oversight.
     */
    AST_INDEX
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
    OP_NOT,   /* not (unary) */
    /* B3 (beads initech-7mo3): ord()/chr(), both unary (AST_UNOP). Value
     * no-ops at codegen -- see seed/codegen.c. */
    OP_ORD,   /* ord(x): char|boolean|integer -> integer */
    OP_CHR    /* chr(x): integer -> char (truncates to 8 bits) */
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
        /* vtype: the DECLARED type of this group ("integer"/"boolean"/
         * "char"), set by the parser when it consumes the type keyword --
         * distinct from the generic per-expression `type` field above.
         * B5 (beads initech-54uu): when is_array is set, `vtype` holds the
         * array's ELEMENT type (reused rather than a new field -- one fewer
         * thing to keep in sync) and lo/hi are the declared inclusive
         * bounds (lo <= hi enforced at parse time, parser.c's
         * parse_array_type). is_array == 0 (the ast_new memset default) is
         * every pre-B5 scalar vardecl; lo/hi are unused (0) in that case. */
        struct {
            AstList    names; /* AST_VARREF nodes */
            AstVarType vtype;
            int        is_array;
            long       lo, hi;
        } vardecl;
        /* B3: name + declared type only -- no value (folded away, see the
         * header's B3 comment). */
        struct { char *name; AstVarType ctype; } constdecl;
        struct { AstList stmts; } block;
        /* B5 (beads initech-54uu): `index` is NULL for an ordinary scalar
         * assignment (every pre-B5 use); non-NULL means `name[index] :=
         * value` -- an indexed (array-element) assignment. See this
         * header's B5 AST_INDEX comment above. */
        struct { char *name; AstNode *index; AstNode *value; } assign;
        /* B2: else_stmt is NULL when there is no 'else' clause. */
        struct { AstNode *cond; AstNode *then_stmt; AstNode *else_stmt; } ifstmt;
        struct { AstNode *cond; AstNode *body; } whilestmt;
        struct { AstList args; int is_newline; } write;
        struct { AstOp op; AstNode *lhs; AstNode *rhs; } binop;
        struct { AstOp op; AstNode *operand; } unop;
        struct { long value; } intlit;
        struct { int value; } boollit;                 /* 0 or 1 (B1) */
        struct { int value; } charlit;                 /* 0..255 byte (B3) */
        struct { char *text; size_t length; } strlit; /* decoded, NUL-term */
        struct { char *name; } varref;
        /* B4 (beads initech-63ce). */
        /* One formal parameter. is_var==1 for a `var` (by-reference)
         * parameter (the frame slot holds an ADDRESS), 0 for a value
         * parameter (the frame slot holds a copy). */
        struct { char *name; AstVarType ptype; int is_var; } param;
        /* A procedure (has_result==0) or function (has_result==1)
         * declaration. `params` holds AST_PARAM nodes left-to-right; `decls`
         * holds local AST_VARDECL groups; `body` is the AST_BLOCK (NULL for a
         * `forward` declaration -- is_forward==1). `rettype` is the function
         * result type (unused when has_result==0). */
        struct {
            char      *name;
            AstList    params;
            int        has_result;
            AstVarType rettype;
            AstList    decls;
            AstNode   *body;
            int        is_forward;
        } procfunc;
        /* A call by name. `args` holds the argument expressions in source
         * (left-to-right) order; codegen pushes them right-to-left (cdecl).
         * Used as an expression (function call; `type` = result type) or as
         * a statement (procedure call). */
        struct { char *name; AstList args; } call;
        /* B5 (beads initech-54uu): `name[index]` as an EXPRESSION (r-value)
         * or a `var`-parameter call argument. `name` must resolve to a
         * declared array (checked by typecheck.c, which also sets `type`,
         * ast.h's generic per-expression field, to the array's element
         * type). See this header's B5 AST_INDEX comment above. */
        struct { char *name; AstNode *index; } arrayindex;
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
