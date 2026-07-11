/*
 * parser.h -- recursive-descent parser for the seed Pascal front-end subset.
 *
 * beads: initech-znb ("Step A of the InitechOS seed cross-compiler")
 * Ref:   PRD Sec 6.7 (single-pass front end), PRD Sec 4 (same language as
 *        the resident compiler). CLAUDE.md Law 1 / Rule 12.
 *
 * GRAMMAR IMPLEMENTED (EBNF). Keywords/identifiers are case-insensitive.
 *
 *   program      = "program" ident ";"
 *                  { const-section | var-section | proc-or-func } block "." ;
 *   var-section  = "var" var-decl ";" { var-decl ";" } ;
 *   var-decl     = ident { "," ident } ":" var-type ;
 *   var-type     = "integer" | "boolean" | "char" | array-type ;
 *
 *   B5 (beads initech-54uu; ADR-0007 DEC-02 "static arrays (array[lo..hi] of
 *   T), indexed as both l-value and r-value") -- static arrays, in BOTH
 *   global (var-section) and LOCAL (a routine's own var-section) position --
 *   parse_one_vardecl is the ONE function both paths share, so this is not
 *   two grammars.
 *
 *   array-type   = "array" "[" array-bound ".." array-bound "]" "of"
 *                  ("integer" | "boolean" | "char") ;
 *   array-bound  = [ "-" ] integer | ident ;
 *     Bounds are PARSE-TIME CONSTANTS ONLY (an integer literal, optionally
 *     negated, or an already-registered `const` INTEGER name -- see
 *     parse_array_bound) -- never a general expression, since a static
 *     array's extent must be known for frame-slot/. bss sizing. `lo <= hi`
 *     is enforced HERE, at parse time (fail loud, Rule 2), not left as a
 *     codegen surprise.
 *   index-expr   = ident "[" expr "]" ;
 *     Used as an EXPRESSION (r-value; see `factor` below) or as an
 *     assignment TARGET (l-value; see `assignment` below, which now allows
 *     an optional "[" expr "]" between the identifier and ":="). The index
 *     expression is any INTEGER expression, including a nested call
 *     (`a[Compute(i)]`).
 *   DECISION (report, ADR silent): NO multi-dimensional arrays (one
 *   dimension only -- ADR-0007 DEC-02's own text asks only for
 *   "array[lo..hi] of T"); NO array-typed parameters (whole-array by value
 *   or by reference) -- only ONE INDEXED ELEMENT may be passed as a `var`
 *   parameter argument (its address is passed); NO runtime bounds checking
 *   (Turbo Pascal's own {$R-} default -- see ast.h's B5 AST_INDEX comment
 *   for the full rationale and the Rule-2 tension this creates, mitigated by
 *   a SOURCE discipline: fixed-capacity arrays + explicit guards, not a
 *   compiler feature).
 *   block        = "begin" [ stmt { ";" stmt } ] "end" ;
 *   stmt         = assignment | call | block | write-stmt | if-stmt
 *                | while-stmt | for-stmt | repeat-stmt | (* empty *) ;
 *   assignment   = ident [ "[" expr "]" ] ":=" expr ;
 *     The optional "[" expr "]" (B5, beads initech-54uu) makes the target an
 *     indexed array-element assignment; `ident` must resolve to a declared
 *     array (typecheck.c). Reuses AST_ASSIGN with an optional `index` field
 *     rather than a new statement kind (see ast.h's B5 comment).
 *
 *   B4 (beads initech-63ce; ADR-0007 DEC-02 "procedures/functions: nested
 *   scopes, both value and var parameters, recursion, results" + the
 *   `forward` directive) -- THE CODEGEN PIVOT. TOP-LEVEL (flat) routines
 *   only; see ast.h's B4 DECISION note (a routine has its own scope over its
 *   params + locals plus the program globals; NOT lexically-nested
 *   declarations with uplevel addressing). A function returns via assignment
 *   to its own name (no `Result` pseudo-variable). A call ALWAYS uses
 *   parentheses (a bare identifier is never a call); a var-parameter argument
 *   must be a plain variable (enforced in typecheck.c). Local `const`
 *   sections are DEFERRED (only local `var`); a global const is usable inside
 *   a routine body, and a param/local shadows a same-named const.
 *
 *   proc-or-func = ("procedure" ident [ param-list ] ";"
 *                 | "function"  ident [ param-list ] ":" type ";")
 *                  ( "forward" ";" | [ var-section ] block ";" ) ;
 *   param-list   = "(" [ param-group { ";" param-group } ] ")" ;
 *   param-group  = [ "var" ] ident { "," ident } ":" type ;
 *   type         = "integer" | "boolean" | "char" ;
 *   call         = ident "(" [ expr { "," expr } ] ")" ;
 *     A `forward` declaration carries the full signature and no body; the
 *     defining occurrence must repeat an IDENTICAL signature (the FPC-
 *     compatible form; typecheck.c validates it). Enables mutual recursion.
 *   write-stmt   = ("write" | "writeln") "(" [ write-args ] ")" ;
 *   write-args   = write-arg { "," write-arg } ;
 *   write-arg    = string | expr ;
 *
 *   B2 (beads initech-80iw; ADR-0007 DEC-02) control flow. if/while are
 *   PRIMITIVE; for/repeat are SUGAR, desugared entirely at parse time (see
 *   parser.c) into if/while/assign/block -- codegen never sees a for/repeat
 *   node. `case` is not implemented (optional per DEC-02's own text).
 *
 *   if-stmt      = "if" expr "then" stmt [ "else" stmt ] ;
 *     Dangling-else binds to the NEAREST unmatched "if" (standard: a plain
 *     recursive-descent parse_if checks for a trailing "else" immediately
 *     after parsing its own then-branch, before returning to any enclosing
 *     if's else-check, so the else is consumed by the innermost if that
 *     doesn't already have one -- ISO 7185 / Turbo Pascal canonical
 *     resolution, not a special rule this grammar has to state).
 *   while-stmt   = "while" expr "do" stmt ;
 *   for-stmt     = "for" ident ":=" expr ( "to" | "downto" ) expr "do" stmt ;
 *     `ident` MUST already be a declared `integer` variable (the for-stmt
 *     does not declare it). Both bound expressions are evaluated EXACTLY
 *     ONCE, before the loop starts (ISO 7185 Sec 6.8.3.9 / Turbo Pascal
 *     Language Guide "for statement": ", the values of the initial-value
 *     and final-value expressions are IMPLEMENTATION-evaluated once, at
 *     entry to the loop" -- a for-stmt is desugared here to an explicit
 *     once-evaluated hidden limit variable specifically so the bound
 *     survives even if the loop body reassigns a variable the bound
 *     expression referenced (see control.pas's FORONCE tag for the fixture
 *     that pins this down). The loop-control variable's value once the loop
 *     terminates NORMALLY is left FORMALLY UNDEFINED by both ISO 7185 and
 *     Turbo Pascal -- no fixture in this repo may depend on it.
 *   repeat-stmt  = "repeat" stmt-seq "until" expr ;
 *     stmt-seq     = stmt { ";" stmt } ;   (* NOTE: no begin/end wrapper *)
 *     The body executes AT LEAST once (it runs, then the guard is tested);
 *     desugars to "the body, once, followed by a while-not-guard loop
 *     whose body is the same statement sequence" (see parser.c).
 *
 *   expr         = simple-expr [ relational-op simple-expr ] ;
 *   relational-op = "=" | "<>" | "<" | "<=" | ">" | ">=" ;
 *   simple-expr  = term { ("+" | "-" | "or") term } ;      (* left-assoc *)
 *   term         = factor { ("*" | "div" | "mod" | "and") factor };(* left-assoc *)
 *   factor       = integer | boolean-lit | char-lit | ident | call
 *                | index-expr
 *                | "ord" "(" expr ")" | "chr" "(" expr ")"
 *                | "(" expr ")" | "-" factor | "not" factor ;
 *     index-expr (B5, beads initech-54uu) is `ident "[" expr "]"`, checked
 *     AFTER the const-fold and call-parenthesis checks in parse_factor's
 *     TOK_IDENT branch (a const can never be an array in this subset, so the
 *     ordering never mis-parses a const use as an index).
 *   boolean-lit  = "true" | "false" ;
 *   char-lit     = "'" any-one-character "'" ;
 *
 *   B3 (beads initech-7mo3; ADR-0007 DEC-02 "const declarations", "char,
 *   ord, chr") additions:
 *
 *   const-section = "const" const-decl ";" { const-decl ";" } ;
 *   const-decl    = ident "=" const-literal ;
 *   const-literal = [ "-" ] integer | char-lit | "true" | "false" ;
 *     const/var sections may REPEAT and INTERLEAVE in any order (DECISION,
 *     report: goes past the bead's stated floor of "at least
 *     const-then-var" -- see parser.c's parse_program_root comment for the
 *     citation). const-decls fold to a literal AST node at every USE SITE,
 *     front-end only -- no runtime storage, no .bss slot (see ast.h's B3
 *     comment and seed/codegen.c's emit_bss).
 *
 *   char-lit DISAMBIGUATION (DECISION, report -- ast.h's B3 comment has the
 *   full citation): the lexer already lexes '...' as one TOK_STRING kind
 *   for every quoted literal, used in TWO different grammar positions:
 *   write-arg (parsed directly by parse_write, unconditionally a string of
 *   whatever length) and factor / const-literal (an EXPRESSION-context use,
 *   which must be exactly one character to be a valid char-lit -- a 0- or
 *   multi-character '...' token reaching factor/const-literal is a located
 *   syntax error, since this subset has no string-typed expressions yet).
 *   This is Turbo Pascal's own rule: a length-1 quoted literal is
 *   Char-typed wherever a Char is expected.
 *
 *   ord(expr) / chr(expr): RESERVED keywords (they predate the general
 *   function-call syntax B4 added and stay reserved for minimality), each
 *   parsing one parenthesized argument. ord() accepts char, boolean, or
 *   integer (Ord is total over every ordinal type this subset has -- TP
 *   allows ord(anInteger) as the identity); chr() accepts only integer,
 *   yielding char (TRUNCATED to 8 bits if out of 0..255 -- Borland Turbo
 *   Pascal default {$R-}, no range-check trap; see seed/codegen.c). See
 *   typecheck.h for the exact rules.
 *
 * Precedence (B1, beads initech-f0uc; ISO 7185 / Turbo Pascal canonical
 * order -- ADR-0007 DEC-02, "Relational operators ... a boolean type, and
 * and/or/not"), LOWEST to HIGHEST:
 *   relational (= <> < <= > >=)   <   or, +, -   <   and, *, div, mod   <
 *   unary - / not   <   primary
 * i.e. 'and' joins the MULTIPLYING operators (same level as * div mod);
 * 'or' joins the ADDING operators (same level as + -); relational is the
 * OUTERMOST (lowest-precedence) level and is NON-CHAINING: `expr` parses
 * AT MOST one relational operator -- "a < b = c" is a syntax error (Pascal
 * relations do not chain; parenthesize + combine with and/or instead), and
 * the parser fails loud with a located, specific diagnostic rather than a
 * generic "expected ..." message (see parse_expr in parser.c).
 * "writeln" with an empty arg list (writeln) and with "()" are both accepted.
 *
 * TYPE CHECKING is a SEPARATE pass (seed/typecheck.c), run by the driver
 * after a successful parse and before codegen -- it is not part of this
 * grammar. See typecheck.h for the rules (assignment/operator/write-arg
 * type agreement; B2 adds: if/while/repeat guards must be boolean, for-loop
 * bounds must be integer -- enforced for free by desugaring into ordinary
 * assign/relational/while nodes that already carry those rules).
 *
 * DEFERRED (later steps, intentionally not parsed): case, real type,
 * records, pointers, typed consts, general string expressions,
 * lexically-nested routines, local const sections, the `Result`
 * pseudo-variable, multi-dimensional/array-of-array arrays, array-typed
 * parameters (B5, beads initech-54uu -- see the array-type grammar note
 * above for the full DECISION list).
 *
 * Error-handling strategy (DECIDED, consistent with the lexer): the parser is
 * single-error. On the first syntax (or lexical) fault it records a located
 * message in ParseResult.error and stops; it does NOT longjmp and does NOT
 * abort. parse_program() returns 0 on success (ast set, ok==1) or non-zero on
 * error (ast==NULL, ok==0, error/line/col populated). The only fatal path is
 * arena OOM, which the arena turns into a loud process abort (Rule 2).
 */
#ifndef SEED_PARSER_H
#define SEED_PARSER_H

#include "ast.h"

#define PARSE_ERRMSG_CAP 256

typedef struct {
    int      ok;                       /* 1 on success, 0 on error */
    AstNode *ast;                      /* the AST_PROGRAM root, or NULL */
    char     error[PARSE_ERRMSG_CAP];  /* located diagnostic on failure */
    int      line;                     /* 1-based fault location */
    int      col;
} ParseResult;

/*
 * Parse [src, src+len) as a program. Nodes are allocated from `arena`, which
 * the caller must have initialised (ast_arena_init) and must free
 * (ast_arena_free) when done with the returned AST. Returns 0 on success,
 * non-zero on a syntax/lex error (details in *out). Never longjmps.
 */
int parse_program(const char *src, size_t len, AstArena *arena,
                  ParseResult *out);

#endif /* SEED_PARSER_H */
