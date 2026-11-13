/*
 * codegen.h -- Step B of the seed Pascal cross-compiler: x86 code emission.
 *
 * beads: initech-znb ("Step B of the InitechOS seed cross-compiler").
 * Ref:   PRD Sec 6.7 ("stack-machine codegen ... fixed register conventions,
 *        NO optimizer, NO register allocator -- small and sufficient");
 *        PRD Sec 4 (the SEED compiler is C-hosted, targets freestanding x86).
 *        CLAUDE.md Law 1 (cite sources) / Rule 2 (fail loud) / Rule 12 (ASCII).
 *
 * This pass walks the Step-A AST (seed/ast.h) and emits nasm Intel-syntax
 * 32-bit assembly to a FILE. It is a straightforward stack-machine walk -- the
 * resident compiler (Turbo Initech) is the single-pass artifact; the seed only
 * needs to be correct (PRD Sec 4 / CLAUDE.md "two compilers, never conflated").
 *
 * ----------------------------------------------------------------------------
 * FIXED CONVENTIONS (documented in full at the top of codegen.c):
 *   - Expressions evaluate ONTO the x86 stack. Each subexpression leaves its
 *     32-bit signed result in EAX; binops push the lhs, evaluate the rhs into
 *     EAX, pop the lhs into a scratch reg (ECX/EBX), combine into EAX.
 *   - EAX = working register. EDX is clobbered by idiv (sign extension).
 *   - div/mod via `cdq` + `idiv ecx` (signed, truncates toward zero for the
 *     non-negative operands in this subset; documented in codegen.c).
 *   - B1 (beads initech-f0uc): relational ops via cmp+setcc+movzx (0/1 in
 *     EAX); and/or are bitwise `and`/`or` on already-0/1 values; not is
 *     `xor eax, 1`. COMPLETE evaluation (ADR-0007 DEC-03): every binop,
 *     including and/or, shares ONE branch-free code path that always
 *     evaluates both operands -- see the codegen.c file header.
 *   - Integer/boolean variables get named 4-byte slots in .bss (v_<name>).
 *   - String literals get labelled byte arrays in .rodata.
 *   - write/writeln call runtime helpers (serial_puts / serial_put_int);
 *     writeln appends a trailing newline (0x0A). A boolean write/writeln
 *     argument prints "TRUE"/"FALSE" (Turbo Pascal semantics) via
 *     serial_puts, selected at runtime from the value in EAX.
 * ----------------------------------------------------------------------------
 *
 * CONTRACT (B1): codegen_emit requires `program` to have already been
 * type-checked by seed/typecheck.c (typecheck_program, called by the driver
 * after a successful parse and before codegen_emit) -- specifically, every
 * write/writeln expression argument's `type` field (ast.h) must be
 * populated so gen_write can choose integer vs boolean printing. Calling
 * codegen_emit on an AST that skipped typechecking is safe only for
 * programs with no boolean write/writeln arguments (an unset AST_TY_UNKNOWN
 * type falls through to the integer print path, never boolean).
 *
 * The emitted module defines a single global `pas_main` (the program body),
 * which the runtime (seed/rt/start.asm) calls after stack setup. The runtime
 * owns _start and the helpers; codegen emits only the program.
 */
#ifndef SEED_CODEGEN_H
#define SEED_CODEGEN_H

#include <stdio.h>

#include "ast.h"

/*
 * Emit nasm Intel-syntax 32-bit assembly for `program` (an AST_PROGRAM root)
 * to `out`. Returns 0 on success, non-zero on a codegen error (which it also
 * reports to stderr). Aborts loudly (Rule 2) only on an internal invariant
 * violation (an unexpected node kind), which would indicate a front-end/AST
 * contract break, never user input.
 *
 * B7 (beads initech-39k2): `program` is non-const -- codegen's single rodata
 * pass stamps each string literal's `lit_ord` and the per-routine temp pre-walk
 * stamps each string-materializing node's `str_temp` (the same "annotate the
 * mutable AST" model typecheck already uses). Requires program.uses_strings to
 * have been set by typecheck (it gates the __str_* intrinsic prelude).
 */
int codegen_emit(AstNode *program, FILE *out);

#endif /* SEED_CODEGEN_H */
