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
    AST_TY_CHAR,
    /* B6 (beads initech-rug7; ADR-0007 DEC-02 "records (record ... end),
     * field access, arrays of records, with a deterministic field layout").
     * A NAMED record type declared in a top-level `type` section (AST_TYPEDECL
     * below) -- the ONLY type-constructor in this subset (DECISION, report:
     * the ADR/plan do not mention a `type` section explicitly; a named
     * record type is the natural, minimal carrier since the compiler's own
     * future source needs NAMED record types (Token, Symbol, AstNode) and an
     * array-of-record var only needs to NAME its element record -- see
     * parser.h's B6 grammar note). Anonymous inline records in a var-section
     * (TP allows `var r: record ... end;`) are DELIBERATELY NOT supported --
     * minimality; every record var/param/array-element names a `type`
     * section's record. WHEREVER a node's static type is AST_TY_RECORD, a
     * companion field (this struct's own `rectype` for expression nodes;
     * `vardecl.rectype` / `param.rectype` for declarations) names WHICH
     * record type, case-preserved-original-spelling, arena-owned -- see this
     * header's AST_TYPEDECL/AST_FIELD comments below. */
    AST_TY_RECORD,
    /* B7 (beads initech-39k2; ADR-0007 DEC-02 "minimal fixed/ShortString-style
     * strings (length/index/compare/concat) -- NOT dynamic/heap strings").
     * A fixed-capacity Turbo-Pascal ShortString: `string` (= string[255]) or
     * `string[N]`, 1 <= N <= 255. See this header's B7 block below for the
     * layout rule, the coercion table, the rejections, and the named idioms. */
    AST_TY_STRING
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
    AST_INDEX,
    /*
     * B6 (beads initech-rug7; ADR-0007 DEC-02 "records (record ... end),
     * field access, arrays of records, with a deterministic field layout").
     *
     *   AST_TYPEDECL: one "type NAME = record f1,f2:T1; f3:T2; end;"
     *     declaration (a top-level-only `type` section, mirroring the B3
     *     const-section's local-declaration deferral -- ast.h's B3 comment).
     *     `fields` holds AST_VARDECL-SHAPED field-GROUP nodes (same "name
     *     list + one shared vtype" shape parse_one_vardecl already builds for
     *     an ordinary var-decl group -- reused rather than inventing a
     *     parallel node shape). DECISION (report, minimality): a field's type
     *     is SCALAR ONLY (integer/boolean/char) -- no nested records, no
     *     array-typed fields, enforced by the PARSER (parse_type_section
     *     only accepts the three scalar keywords for a field's type, never
     *     `array`/a record-type name). This keeps "sizeof(record) = 4 *
     *     field-count" a flat, uniform fact with no recursive layout
     *     question -- the compiler's own Token/Symbol/AstNode records
     *     (integer/char/boolean fields) need nothing more.
     *
     *   AST_FIELD: `base.field` -- a field access used as an r-value
     *     expression (check_expr), as an l-value TARGET (reusing AST_ASSIGN's
     *     new `field`/`field_index` members below, exactly as AST_INDEX's
     *     l-value case reuses AST_ASSIGN's `index` rather than adding a
     *     sibling statement kind), or as a `var`-parameter call argument (its
     *     field's ADDRESS is passed -- codegen.c's gen_addr_of). `base` is
     *     ALWAYS AST_VARREF (a plain record variable) or AST_INDEX (one
     *     element of an array-of-record) -- NEVER another AST_FIELD, because
     *     a field's type is always scalar (no record-of-record in this
     *     subset, see AST_TYPEDECL's note above), so nesting stops at depth
     *     1. `field_index` is the field's 0-based DECLARATION-ORDER index
     *     within its record type, resolved by seed/typecheck.c and consumed
     *     by codegen (mirrors this header's generic per-expression `type`
     *     field: computed once by typecheck, never re-derived by codegen).
     *
     * LAYOUT RULE (binding, Rule 11 / ADR-0007 DEC-04's documented-
     * deterministic-layout spirit, applied to records): fields occupy
     * CONSECUTIVE UNIFORM 4-byte slots in DECLARATION ORDER -- field k is at
     * byte offset 4*k from the record's base address, and
     * sizeof(record) = 4 * field-count. This is the IDENTICAL "one
     * zero-extended dword per scalar, no packing" convention B1
     * (booleans)/B3 (chars)/B5 (array elements) already use, extended one
     * level: NO padding, NO per-field-type width, and (the bead's own
     * "deep-bug intersection") a record ELEMENT inside an array-of-record
     * uses the record's TOTAL SIZE (4*field-count) as the element STRIDE --
     * gen_elem_addr's existing "base + (index-lo)*stride" formula (B5)
     * composes UNCHANGED, just fed a bigger stride; a field access on top of
     * that adds a further fixed 4*field_index (see seed/codegen.c's
     * gen_field_addr). See seed/codegen.c's file-header comment for the
     * full worked addressing story and both this bead's mutation hooks
     * (SEED_MUT_CODEGEN_FIELD_OFF4, SEED_MUT_CODEGEN_REC_STRIDE).
     *
     * SCOPE DECISIONS (report):
     *   - Anonymous inline records (`var r: record ... end;`, TP allows this)
     *     are NOT supported -- minimality; every record names a `type`.
     *   - Value (non-`var`) parameters of RECORD type are REJECTED LOUDLY
     *     (parse time, seed/parser.c's parse_param_list) -- TP would copy the
     *     whole record on every call, which is expensive and unnecessary
     *     here since the self-host source can always use `var`. `var`
     *     parameters of record type ARE supported (the record's address is
     *     passed) -- needed for symbol-table-style helpers that mutate a
     *     caller's record in place.
     *   - Function RESULTS of record type are NOT supported (out of scope;
     *     no self-host need identified, and returning an aggregate through
     *     EAX alone does not generalize the way a scalar result does).
     *   - Whole-record ASSIGNMENT (`r1 := r2`, TP allows it) IS supported,
     *     for two designators of the IDENTICAL named record type (checked by
     *     TYPE NAME, not structural layout -- two different record types
     *     with the same field shape are NOT assignment-compatible).
     *     Implemented as a fully UNROLLED, compile-time member-wise word
     *     copy (field count is always a compile-time constant here, so no
     *     runtime loop/label is needed -- deterministic, Rule 11) -- see
     *     AST_ASSIGN's `rec_fields` member below and codegen.c's
     *     gen_record_copy.
     *   - Record COMPARISON (`=`, `<>`, ...) is REJECTED LOUDLY
     *     (seed/typecheck.c check_binop) -- Turbo Pascal does not define
     *     relational operators on records either.
     */
    AST_TYPEDECL,
    AST_FIELD
} AstKind;

/*
 * ============================================================================
 * B7 -- fixed/ShortString strings (beads initech-39k2; B7 committee 2026-07-14
 * (3 seats + chair synthesis), bead initech-39k2; ADR-0007 DEC-02 "minimal
 * fixed/ShortString-style strings (length/index/compare/concat) -- NOT
 * dynamic/heap strings", DEC-04 deterministic codegen, DEC-05 "the RTL stays
 * byte/block I/O only ... integer-to-decimal-string is hand-written in-subset
 * Pascal (div/mod + chr + concatenation)").
 *
 * TYPE SURFACE (D1). `string` (= string[255]) and `string[N]`, 1 <= N <= 255
 * (N an integer literal or a folded integer const; out of range = a located
 * parse error). Allowed as: GLOBAL vars, frame-resident LOCALS, and `var`
 * parameters (the formal must be the BARE `string`, cap 255 -- see D6). String
 * CONST declarations (`const Msg = 'text';`) are supported: they fold to a
 * string literal at each use site (the B3 const-fold precedent; the parser's
 * const table gains a text slot). '' as a const or literal value is legal.
 *
 * REJECTED LOUDLY (parse/typecheck, B6 precedent -- each message NAMES the
 * construct): array-of-string (`array[..] of string[N]`), string RECORD
 * fields, VALUE string parameters, string FUNCTION results, a `string[N<255]`
 * VAR formal, and string as an array element type anywhere.
 *
 * ARRAY-OF-STRING DEFERRAL (D1, named consequence): array-of-string is NOT in
 * this subset. Turbo Initech's symbol-table NAMES therefore use the char-pool
 * + integer-offset idiom (ADR-0007 DEC-02's own static index-arena idiom) OR a
 * follow-up bead adds array-of-string before B9 -- decided at B9 sizing, not
 * here.
 *
 * REPRESENTATION (D2, the binding layout rule). A TP ShortString: byte 0 = the
 * current length (unsigned 0..N), bytes 1..N = content, bytes N+1..W-1 =
 * padding never read. W = round4(N+1) = ((N+1)+3)/4*4 bytes (string[255] ->
 * 256B; string[3] -> 4B; string[8] -> 12B). Every string designator (global,
 * local, `var`-param deref) yields an ASCENDING byte pointer to byte 0;
 * capacity is ALWAYS a compile-time immediate from the declared type -- there
 * is NO runtime capacity field.
 *   - GLOBAL: `v_<name>: resb W` in .bss (align 4 held -- W is a dword
 *     multiple); zero-init .bss => an empty string.
 *   - LOCAL: W/4 CONTIGUOUS frame dword slots; the designator's base is the
 *     LOWEST address of the block, byte i at base+i (ascending, exactly like a
 *     global). For W=4 this reduces to the existing scalar cg_local_offset.
 *     NOTE (DECISION): this deliberately DIFFERS from B5's array-local
 *     element-0-at-highest-address convention -- strings are byte-addressed
 *     through the intrinsics, so every storage class shares ONE flat ascending
 *     pointer model. LOCAL string SAFETY (DECISION, a small TP divergence,
 *     differential-invisible): the routine prologue zeroes each local string's
 *     length byte, so length()/write of an unassigned local is a well-defined
 *     empty string (Rule 2 spirit); fixtures still always assign before
 *     reading, so the divergence is documented, not exercised.
 *
 * ASSIGNMENT + INDEXING (D5).
 *   - `s := <string-or-char rhs>`: silent truncation to the compile-time
 *     cap(dst). Whole-string := is the ONLY aggregate string op.
 *   - `s[i]`: 1-based; address = base + i (byte 0 is the length prefix, so NO
 *     -1 adjustment). Read = `movzx eax, byte [base+i]` (a char r-value);
 *     write `s[i] := <char>` = `mov [base+i], al` and does NOT touch the
 *     length byte. NO bounds check ({$R-}, the B5 precedent) -- s[0]/s[>len]
 *     is UNCHECKED. RULE-2 TENSION (recorded verbatim, the B5 pattern): an
 *     out-of-range string index is a SILENT out-of-bounds byte access with no
 *     diagnostic; the mitigation is a SOURCE DISCIPLINE (index only within
 *     1..length before every s[i]), not a codegen feature -- the identical
 *     discipline ADR-0007 DEC-02's amendment mandates for the compiler's own
 *     collections. Recorded so a future reader does not mistake the omission
 *     for an oversight.
 *   - AST reuse (HIGHEST-RISK overload, flagged): a string byte read reuses
 *     AST_INDEX (with arrayindex.is_string) and a string byte write reuses
 *     AST_ASSIGN's `index` (with assign.strcap>0), disambiguated from arrays
 *     by the base designator's TYPE at typecheck. A mix-up would byte-load an
 *     array or dword-load a string, so string.pas covers BOTH array[i] and
 *     s[i] in one program (the IDXA cross-talk guard).
 *
 * COERCION TABLE (D7, binding).
 *   - Multi-char '...' in EXPRESSION context: a string-typed r-value (revises
 *     the B3 "one char only" error -- see AST_CHARLIT's B3 note). Emitted as a
 *     length-prefixed .rodata blob (strlit_<n>), a DISTINCT label family from
 *     the NUL-terminated write-literal str_<n>, BOTH drawing ordinals from the
 *     ONE shared counter/walk order (DEC-04's anti-pattern note; the existing
 *     write-literal emission stays byte-identical).
 *   - '' : legal ONLY in string contexts (empty string, len 0); still a
 *     located error in char context (fpc rejects char := '').
 *   - Single-char '...': stays CHAR (B3). A char is materialized into a len-1
 *     string in EXACTLY three contexts: (1) assignment RHS to a string
 *     l-value; (2) a concat operand when the OTHER operand is string-typed;
 *     (3) a comparison operand when the OTHER operand is string-typed. Nowhere
 *     else (never a var-arg).
 *   - `+` = concat IFF >= 1 operand is string-typed (typecheck overloads
 *     OP_ADD; gen_binop dispatches on operand type). char + char (no string
 *     operand) = a LOCATED type error -- a deliberate TP/fpc divergence (fpc
 *     concatenates); the fpc fixture keeps >= 1 string operand in every concat.
 *   - Relops: the string path IFF >= 1 operand is string-typed (the char side
 *     coerces); both-char stays the existing B1/B3 ordinal compare.
 *   - integer <-> string: NO bridge, both directions located errors (DEC-05).
 *   - string -> char: REJECTED (`c := s` illegal even if len 1); a char comes
 *     from a string only via s[i].
 *   - concat intermediate truncates at 255 silently.
 *
 * length() (D9): `length` is a RESERVED keyword (ord/chr B3 precedent),
 * parsed as length(expr) -> AST_UNOP OP_LENGTH; the argument must be
 * string-typed; codegen emits an inline `movzx eax, byte [base]`. Result
 * integer. setlength is OUT; s[0]-as-length-API is unsanctioned.
 *
 * PARAMS + RESULTS + IDIOMS (D6). `var string` params are IN (the bare
 * `string` cap-255 formal only; the address passes, the callee derefs -- the
 * B4 var-param model). Value string params, string function results, and a
 * `string[N<255]` var formal are all rejected loudly. Named idioms (for the
 * self-host source): stage-then-pass a literal (`tmp := 'begin'; P(tmp)`), and
 * IntToStr as `procedure IntToStr(v: integer; var s: string)` built via
 * `s := chr(d) + s` (keeps DEC-05's in-subset int-to-decimal-string sentence
 * expressible without an RTL service).
 *
 * TEMPORARIES (D4, the deep-bug locus) + THE CHAIR'S CORRECTION. Concat/
 * coercion/compare materialize into FRAME-RESIDENT temporaries (256 bytes = 64
 * dwords each, allocated AFTER locals; a deterministic per-routine source-
 * order pre-walk assigns each site a temp INDEX, reset per statement; the
 * routine reserves max-live-count temps -- pas_main gets its own temp region
 * too). In-place LEFT-DEEP accumulation uses ONE temp (a+b+c => T0:=a;
 * T0+=b; T0+=c). A RIGHT-NESTED operand a+(b+c) forces a SECOND
 * simultaneously-live temp (T0:=a; T1:=b; T1+=c; T0+=T1). THE CHAIR'S
 * CORRECTION (binding): in THIS subset NO string temp is ever live across a
 * call (string results are rejected and a concat is never a var-arg, so no
 * call occurs inside a string expression), so the temp-lifetime deep bug is
 * TWO-LIVE-TEMPS-IN-ONE-STATEMENT (right-nested concat), NOT clobber-across-
 * recursion; string.pas's RNEST clause is that load-bearing bite (a recursive
 * clause is belt, not the bite). ALIASING (s := s + 'x'; s := 'a' + s): the
 * RHS is evaluated FULLY into a temp, then copied to dst with cap(dst) -- dst
 * is never written while it is a live source. `s := <char/lit>` with NO concat
 * may store len + byte(s) inline (the fast path). See codegen.c's B7 header
 * for the intrinsic ABI, the full temp model, and the worked sequence.
 * ============================================================================
 */

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
    OP_CHR,   /* chr(x): integer -> char (truncates to 8 bits) */
    /* B7 (beads initech-39k2): length(s): string -> integer. Unary
     * (AST_UNOP), a reserved keyword like ord/chr (see token.h's B7 note);
     * codegen emits an inline `movzx eax, byte [base]` reading the
     * ShortString's length prefix -- no intrinsic. */
    OP_LENGTH
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
    /* B7 (beads initech-39k2): the frame-resident string-TEMPORARY index this
     * string-materializing expression node evaluates into, assigned by
     * codegen.c's deterministic per-routine pre-walk (plan_string_temps) in
     * source order and read back during emission -- the ONE numeric source
     * for a temp's frame slot, so the SEED_MUT_CODEGEN_STR_TEMP_CLOBBER mutant
     * (collapse every string temp to index 0) has a single choke point. Set
     * ONLY on a concat node (AST_BINOP OP_ADD, string) and on a char-coercion
     * operand that materializes a len-1 string; -1 (unused) everywhere else,
     * including a plain string designator/literal (which needs no temp). See
     * codegen.c's B7 header for the temp model + the worked s := a + (b + c)
     * sequence. */
    int str_temp;
    union {
        /* B7 (beads initech-39k2): `uses_strings` is set by seed/typecheck.c
         * (during its existing walk -- no new pass) iff the program references
         * any string type; codegen.c reads it to decide whether to emit the
         * fixed __str_* intrinsic prelude at all, so a stringless program's
         * emitted .s stays BYTE-IDENTICAL to pre-B7 (the repro corpus's
         * byte-identity guard). A pure function of the AST (deterministic). */
        struct { char *name; AstList decls; AstNode *block; int uses_strings; } program;
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
            /* B6 (beads initech-rug7): non-NULL iff vtype == AST_TY_RECORD --
             * names WHICH record type (arena-owned, original spelling; the
             * parser only accepts an already-registered `type` name here, so
             * this always resolves). When is_array is ALSO set, this is the
             * ARRAY ELEMENT's record type (array-of-record); when is_array is
             * 0, this is the scalar variable's own record type. NULL for
             * every scalar (non-record) vardecl. */
            char      *rectype;
            /* B7 (beads initech-39k2): the declared ShortString CAPACITY (N,
             * 1..255) when vtype == AST_TY_STRING, else 0. `string` folds to
             * strcap == 255. A string vardecl is never an array (array-of-
             * string is rejected loudly at parse time -- ast.h's B7 block),
             * so is_array and strcap are never both set. */
            int        strcap;
        } vardecl;
        /* B3: name + declared type only -- no value (folded away, see the
         * header's B3 comment). */
        struct { char *name; AstVarType ctype; } constdecl;
        struct { AstList stmts; } block;
        /* B5 (beads initech-54uu): `index` is NULL for an ordinary scalar
         * assignment (every pre-B5 use); non-NULL means `name[index] :=
         * value` -- an indexed (array-element) assignment. See this
         * header's B5 AST_INDEX comment above.
         *
         * B6 (beads initech-rug7): `field` is NULL for every non-field
         * assignment (scalar or whole-array-element); non-NULL means
         * `name[index?].field := value` -- a FIELD assignment (index may
         * additionally be NULL for `name.field := value` or non-NULL for
         * `name[index].field := value`, composing with the array case).
         * `field_index` is the field's resolved 0-based declaration-order
         * index (set by typecheck.c), meaningful iff `field` is non-NULL.
         * `rec_fields` is 0 for every ordinary (scalar or field) assignment
         * and > 0 (the record type's field count) for a WHOLE-RECORD
         * assignment (`r1 := r2` or `arr[i] := r`, `field` NULL, `value` a
         * plain record-typed designator) -- set by typecheck.c, consumed by
         * codegen.c's gen_record_copy (a fully unrolled member-wise copy).
         *
         * B7 (beads initech-39k2): `strcap` is 0 for every non-string target
         * and > 0 (the target's declared ShortString capacity, 1..255) when
         * `name` resolves to a string variable/`var` parameter -- set by
         * typecheck.c, consumed by codegen.c to route the assignment through
         * the string path. `index` NULL => a WHOLE-string assign (`s := rhs`,
         * silent truncation to `strcap`); `index` non-NULL => an s[i] BYTE
         * write (`s[i] := <char>`, no length change, `strcap` only marks that
         * `s` is a string). `field` is always NULL for a string target
         * (records have no string fields in this subset). */
        struct {
            char    *name;
            AstNode *index;
            char    *field;
            int      field_index;
            int      rec_fields;
            int      strcap;
            AstNode *value;
        } assign;
        /* B2: else_stmt is NULL when there is no 'else' clause. */
        struct { AstNode *cond; AstNode *then_stmt; AstNode *else_stmt; } ifstmt;
        struct { AstNode *cond; AstNode *body; } whilestmt;
        struct { AstList args; int is_newline; } write;
        struct { AstOp op; AstNode *lhs; AstNode *rhs; } binop;
        struct { AstOp op; AstNode *operand; } unop;
        struct { long value; } intlit;
        struct { int value; } boollit;                 /* 0 or 1 (B1) */
        struct { int value; } charlit;                 /* 0..255 byte (B3) */
        /* B7 (beads initech-39k2): `lit_ord` is the .rodata ordinal assigned
         * to THIS literal by codegen.c's single rodata pass (the ONE shared
         * str_count counter, DEC-04) and STORED here, so the .text pass reads
         * it directly rather than re-deriving it in a second walk (no
         * divergence risk). A write-argument literal emits `str_<ord>` (NUL-
         * terminated, the pre-B7 shape -- byte-identical); an EXPRESSION-
         * context literal emits `strlit_<ord>` (length-prefixed). */
        struct { char *text; size_t length; int lit_ord; } strlit; /* decoded */
        struct { char *name; } varref;
        /* B4 (beads initech-63ce). */
        /* One formal parameter. is_var==1 for a `var` (by-reference)
         * parameter (the frame slot holds an ADDRESS), 0 for a value
         * parameter (the frame slot holds a copy).
         * B6 (beads initech-rug7): `rectype` is non-NULL iff ptype ==
         * AST_TY_RECORD (names which record type). A value (is_var==0)
         * parameter of record type is REJECTED LOUDLY at parse time
         * (parser.c's parse_param_list) -- this field is therefore only ever
         * non-NULL in practice on a `var` parameter (is_var==1). */
        /* B7 (beads initech-39k2): `strcap` is the ShortString capacity (255)
         * when ptype == AST_TY_STRING, else 0. Only a `var` parameter may be
         * string-typed and its formal MUST be the bare `string` (cap 255) --
         * a value string parameter and a `string[N<255]` var formal are both
         * rejected loudly at parse time (parser.c's parse_param_list;
         * ast.h's B7 block). */
        struct { char *name; AstVarType ptype; int is_var; char *rectype; int strcap; } param;
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
        /* B7 (beads initech-39k2): `is_string` is 1 iff `name` resolves to a
         * STRING (not an array) -- then `name[index]` is a 1-based BYTE read
         * (a char r-value, `movzx eax, byte [base+index]`), NOT an array
         * element load. Set by typecheck.c (which disambiguates array-vs-
         * string from the base designator's type -- ast.h's B7 block flags
         * this AST_INDEX/assign-index reuse as the highest-risk overload).
         * 0 for every ordinary array index. */
        struct { char *name; AstNode *index; int is_string; } arrayindex;
        /* B6 (beads initech-rug7): one "type NAME = record ... end;"
         * declaration. `fields` holds AST_VARDECL-shaped field-GROUP nodes
         * (scalar-only vtype; is_array/rectype always 0/NULL on a field
         * group -- see this header's B6 AST_TYPEDECL comment above). */
        struct { char *name; AstList fields; } typedecl;
        /* B6 (beads initech-rug7): `base.field` as an r-value expression (or,
         * via AST_ASSIGN's own `field`/`field_index`, an l-value/var-param
         * address target -- this node kind itself is only ever built for the
         * R-VALUE/argument position; see parser.c). `base` is AST_VARREF or
         * AST_INDEX (never AST_FIELD -- fields are scalar-only, no nesting).
         * `field_index` is resolved by typecheck.c (0-based declaration
         * order within the base's record type); this node's generic `type`
         * (ast.h, above) is the FIELD's scalar type once typechecked. */
        struct { AstNode *base; char *field; int field_index; } field;
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
