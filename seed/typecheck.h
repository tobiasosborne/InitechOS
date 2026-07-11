/*
 * typecheck.h -- minimal static type checker for the seed Pascal subset.
 *
 * beads: initech-f0uc (B1 of docs/plans/TPS-M7-subset-plan.md)
 * Ref:   ADR-0007 DEC-02 (the subset table; integer/boolean scalars),
 *        DEC-03 (complete boolean evaluation), CLAUDE.md Law 1 / Rule 2 /
 *        Rule 12. PRD Sec 6.7 (Turbo Initech: same language as the seed).
 *
 * DECISION (B1, report for the record): ADR-0007 fixes the LANGUAGE subset
 * (DEC-02) and the EVALUATION strategy (DEC-03) but is silent on how strict
 * the seed's own type checking must be -- it says only "a compiler needs
 * SOME typing discipline." This module implements the minimal SOUND rule
 * set the subset needs at B1:
 *
 *   - Every variable name is declared exactly once (case-insensitive, per
 *     Pascal's case-insensitive identifiers -- ADR-0007 DEC-02 dialect
 *     pinning); a duplicate name is a checked error, not silently the last
 *     declaration winning.
 *   - Every variable REFERENCE (assignment target, expression operand)
 *     must resolve to a declared name; there is no implicit declaration.
 *   - Assignment (:=) requires the right-hand expression's type to match
 *     the left-hand variable's declared type EXACTLY -- no implicit
 *     integer<->char<->boolean coercion; `ord()`/`chr()` (B3, beads
 *     initech-7mo3) are the only explicit bridges between integer and
 *     char/boolean.
 *   - Relational operators (= <> < <= > >=) require BOTH operands to be
 *     the SAME type (integer, boolean, or -- as of B3 -- char) and yield
 *     boolean.
 *   - and/or/not require boolean operand(s) and yield boolean.
 *   - + - * div mod (and unary -) require integer operand(s) and yield
 *     integer. (char is deliberately NOT arithmetic: no char+char,
 *     char+int, etc. -- only ord()/chr() move a value between char and
 *     integer, and relational ops compare two chars.)
 *   - A write/writeln expression argument (string args are exempt; they are
 *     never type-checked as expressions) must resolve to integer, boolean,
 *     or char -- codegen picks the print routine from the checked type
 *     (see typecheck_program's contract below and seed/codegen.c gen_write).
 *   - (B2, beads initech-80iw) 'if'/'while' conditions must be boolean.
 *     'for'/'repeat' get the same discipline for free: the parser desugars
 *     both to assign/while/binop/unop(not) nodes before typecheck ever
 *     runs, so a for-loop's bounds go through ordinary assignment type
 *     checking (forcing integer) and a repeat's guard goes through ordinary
 *     'not' checking (forcing boolean) -- see seed/parser.c's desugar
 *     comments and ADR-0007 DEC-02.
 *   - (B3, beads initech-7mo3; ADR-0007 DEC-02 "const declarations", "char,
 *     ord, chr") additions:
 *       * const names resolve through the SAME flat, case-insensitive,
 *         one-declaration symbol table as var names (a const/var name
 *         collision, or a duplicate const, is the identical "duplicate
 *         variable declaration" diagnostic); a const may never be an
 *         assignment TARGET ("cannot assign to a constant").
 *       * char is a type DISTINCT from integer -- no implicit coercion in
 *         either direction.
 *       * ord(x) requires x : char | boolean | integer, yields integer
 *         (TOTAL over every ordinal type this subset has; ord(boolean) is
 *         IMPLEMENTED -- Turbo Pascal allows it, giving 0/1 -- see
 *         check_unop's OP_ORD case for the DECISION note).
 *       * chr(x) requires x : integer, yields char (see seed/codegen.c's
 *         OP_CHR case for the out-of-range TRUNCATION decision -- this
 *         module only enforces the operand TYPE, not its runtime range).
 *
 * This is intentionally NOT full Pascal type inference (no subranges, no
 * real/records/arrays -- those are out of scope for B1-B3 per the DEC-02
 * subset table) -- just enough soundness that a boolean (or now char) can
 * never silently flow where an integer is expected (or vice versa), and
 * every write knows which print routine to call.
 *
 * CONTRACT: typecheck_program must be run (and must succeed) exactly once,
 * after a successful parse_program() and before codegen_emit(). It mutates
 * `program` in place, setting the `type` field (ast.h) of every expression
 * node it visits (AST_INTLIT/AST_BOOLLIT/AST_VARREF/AST_BINOP/AST_UNOP) to
 * its computed AstVarType. Non-expression nodes (AST_PROGRAM, AST_VARDECL,
 * AST_BLOCK, AST_ASSIGN, AST_WRITE/AST_WRITELN, AST_STRLIT) are not
 * assigned a `type` (AST_STRLIT never needs one; write/writeln skip typing
 * string args entirely -- see the DECISION note above).
 *
 * Error-handling strategy (consistent with the lexer/parser, see their
 * headers): single-error. The first type fault is recorded with a location
 * and checking stops; typecheck_program never longjmps and never aborts on
 * a well-formed AST (an AST_VARDECL whose list contains a non-AST_VARREF
 * node, or any other AST-shape invariant break, is an internal contract
 * violation between the parser and this module and IS a loud fatal --
 * Rule 2 -- because it can only mean a bug in this codebase, never bad
 * user input).
 */
#ifndef SEED_TYPECHECK_H
#define SEED_TYPECHECK_H

#include "ast.h"

#define TYPECHECK_ERRMSG_CAP 256

typedef struct {
    int  ok;                          /* 1 on success, 0 on a type error */
    char error[TYPECHECK_ERRMSG_CAP]; /* located diagnostic on failure */
    int  line;                        /* 1-based fault location */
    int  col;
} TypeCheckResult;

/*
 * Type-check (and annotate) `program`, an AST_PROGRAM root from a
 * successful parse_program() call. Returns 0 on success (out->ok == 1,
 * every expression node's `type` field populated), non-zero on the first
 * type error (out->ok == 0, error/line/col located). Never longjmps.
 */
int typecheck_program(AstNode *program, TypeCheckResult *out);

#endif /* SEED_TYPECHECK_H */
