/*
 * parser.h -- recursive-descent parser for the seed Pascal front-end subset.
 *
 * beads: initech-znb ("Step A of the InitechOS seed cross-compiler")
 * Ref:   PRD Sec 6.7 (single-pass front end), PRD Sec 4 (same language as
 *        the resident compiler). CLAUDE.md Law 1 / Rule 12.
 *
 * GRAMMAR IMPLEMENTED (EBNF). Keywords/identifiers are case-insensitive.
 *
 *   program      = "program" ident ";" [ var-section ] block "." ;
 *   var-section  = "var" var-decl ";" { var-decl ";" } ;
 *   var-decl     = ident { "," ident } ":" ("integer" | "boolean") ;
 *   block        = "begin" [ stmt { ";" stmt } ] "end" ;
 *   stmt         = assignment | block | write-stmt | if-stmt | while-stmt
 *                | for-stmt | repeat-stmt | (* empty *) ;
 *   assignment   = ident ":=" expr ;
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
 *   factor       = integer | boolean-lit | ident | "(" expr ")"
 *                | "-" factor | "not" factor ;
 *   boolean-lit  = "true" | "false" ;
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
 * DEFERRED (later steps, intentionally not parsed): case, procedures/
 * functions, const sections, char/real types, records, pointers, arrays.
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
