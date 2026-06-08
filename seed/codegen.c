/*
 * codegen.c -- Step B seed codegen: AST -> nasm Intel-syntax 32-bit x86.
 *
 * beads: initech-znb ("Step B of the InitechOS seed cross-compiler").
 * Ref:   PRD Sec 6.7 -- "Single-pass front end; stack-machine codegen
 *        (expression evaluation to x86, fixed register conventions, NO
 *        optimizer, NO register allocator) -- small and sufficient." This
 *        seed pass is an AST walk (correctness over single-pass purity; the
 *        RESIDENT compiler is the single-pass artifact, PRD Sec 4 /
 *        CLAUDE.md "two compilers, never conflated").
 *        CLAUDE.md Law 1 (cite) / Law 2 (oracle is truth) / Rule 2 (fail
 *        loud) / Rule 11 (deterministic, no timestamps) / Rule 12 (ASCII).
 *
 * ============================================================================
 * FIXED CALLING / STACK-MACHINE CONVENTIONS
 * ============================================================================
 * The compiled program is one nasm function `pas_main`, called by the runtime
 * (seed/rt/start.asm) after it sets up a stack. `pas_main` follows the
 * standard cdecl frame (push ebp / mov ebp,esp / ... / leave / ret) and may
 * clobber eax/ecx/edx freely (caller-saved); the runtime expects nothing back.
 *
 * EXPRESSIONS evaluate onto the x86 data stack, postorder:
 *   - Every expression leaves its 32-bit signed result in EAX.
 *   - For a binary op `lhs OP rhs`:
 *         eval lhs  -> eax
 *         push eax                  ; spill lhs to the stack
 *         eval rhs  -> eax          ; (may itself recurse/push)
 *         mov ecx, eax              ; rhs -> ecx
 *         pop eax                   ; lhs -> eax
 *         <combine eax (lhs), ecx (rhs)> -> eax
 *     This spill-to-stack discipline is the "stack machine": no register
 *     allocator, fixed registers, correct for arbitrary nesting.
 *   - EAX = working/result register; ECX = popped rhs scratch.
 *   - EDX is reserved as the idiv high-word / sign-extension register.
 *
 * OPERATORS:
 *   +   add eax, ecx
 *   -   sub eax, ecx                 (lhs - rhs)
 *   *   imul eax, ecx                (signed 32x32 -> low 32)
 *   div cdq ; idiv ecx -> quotient in eax   (Pascal `div`)
 *   mod cdq ; idiv ecx -> remainder in edx ; mov eax, edx   (Pascal `mod`)
 *   unary -  neg eax
 *
 *   Pascal `div`/`mod` are integer division. The Step-A subset only forms
 *   non-negative operands in the gate corpus, but x86 `idiv` is signed and
 *   TRUNCATES TOWARD ZERO, which matches ISO-Pascal `div` for operands of the
 *   same sign and is the Turbo-Pascal behaviour. (`mod` follows the sign of
 *   the dividend on x86; that, too, is Turbo-Pascal's `mod`. Negative-operand
 *   semantics are documented here but not exercised by the seed gate, which
 *   only divides non-negative values.)
 *
 * VARIABLES: each `integer` var gets a zero-initialised 4-byte slot in .bss
 * labelled `v_<name>` (names are case-insensitive Pascal idents; we lower-case
 * them for a deterministic, collision-free label, and the front end already
 * rejects redeclarations). A var read is `mov eax, [v_name]`; an assignment is
 * `mov [v_name], eax`.
 *
 * STRING LITERALS: each distinct write/writeln string arg is emitted as a
 * NUL-terminated byte array in .rodata labelled `str_<n>` (n = emission order,
 * deterministic). Bytes are emitted as decimal `db` values so any byte is safe
 * and the source stays ASCII-clean (Rule 12).
 *
 * write/writeln LOWERING: each argument is emitted left to right:
 *   - string arg : lea/mov the label into the arg slot, call serial_puts
 *   - int expr   : eval -> eax, call serial_put_int (eax = signed value)
 *   writeln additionally writes a trailing newline (0x0A) via serial_putc.
 *   Helper ABI (defined in the runtime): arg passed in EAX (cdecl-ish but
 *   register-passed for simplicity); callee preserves nothing we rely on, so
 *   no live values straddle a helper call.
 *
 * Determinism (Rule 11): no timestamps, labels are ordinal, output is a pure
 * function of the AST -> the same source always yields byte-identical asm.
 * ============================================================================
 */
#include "codegen.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    FILE *out;
    int   str_count;   /* next .rodata string label ordinal */
    /* Collected .rodata string defs are emitted inline as encountered into a
     * deferred buffer is unnecessary: we emit .text and .rodata in separate
     * passes. To keep a single AST walk we instead emit strings to .rodata at
     * the point of use by buffering. For simplicity and determinism we do a
     * first pass to emit .rodata strings, then a second pass for .text. */
} Cg;

/* ---- fatal (Rule 2): internal AST contract violation, never user input ---- */
static void cg_ice(const char *what, const AstNode *n)
{
    fprintf(stderr,
            "initechc: codegen ICE: %s (kind=%d at %d:%d)\n",
            what, n ? (int)n->kind : -1, n ? n->line : 0, n ? n->col : 0);
    abort();
}

/* Emit a lower-cased ident as a label-safe suffix. The lexer guarantees idents
 * are [A-Za-z_][A-Za-z0-9_]*, so a straight lower-case is collision-free. */
static void emit_var_label(FILE *out, const char *name)
{
    for (const char *p = name; *p; p++)
        fputc((char)tolower((unsigned char)*p), out);
}

/* ----- .rodata string pass: assign ordinal labels to each string arg ----- */
/* We walk write/writeln args in source order and emit each string literal as a
 * labelled byte array. To map a string node to its label in the .text pass we
 * re-walk in the SAME order and re-derive the ordinal -- deterministic. */

static void rodata_walk(Cg *cg, const AstNode *n);

static void rodata_emit_string(Cg *cg, const AstNode *s)
{
    FILE *o = cg->out;
    int idx = cg->str_count++;
    fprintf(o, "str_%d:\n", idx);
    fprintf(o, "    db ");
    for (size_t i = 0; i < s->as.strlit.length; i++) {
        fprintf(o, "%u,", (unsigned char)s->as.strlit.text[i]);
    }
    fprintf(o, "0\n");
}

static void rodata_walk(Cg *cg, const AstNode *n)
{
    if (!n)
        return;
    switch (n->kind) {
    case AST_PROGRAM:
        rodata_walk(cg, n->as.program.block);
        break;
    case AST_BLOCK:
        for (size_t i = 0; i < n->as.block.stmts.count; i++)
            rodata_walk(cg, n->as.block.stmts.items[i]);
        break;
    case AST_WRITE:
    case AST_WRITELN:
        for (size_t i = 0; i < n->as.write.args.count; i++) {
            AstNode *arg = n->as.write.args.items[i];
            if (arg->kind == AST_STRLIT)
                rodata_emit_string(cg, arg);
        }
        break;
    case AST_ASSIGN:
        /* assignment value is an integer expr -- no string literals there */
        break;
    default:
        break;
    }
}

/* ----------------------------- .text pass ----------------------------- */

static void gen_expr(Cg *cg, const AstNode *e);

static void gen_binop(Cg *cg, const AstNode *e)
{
    FILE *o = cg->out;
    gen_expr(cg, e->as.binop.lhs);     /* lhs -> eax */
    fprintf(o, "    push eax\n");       /* spill lhs */
    gen_expr(cg, e->as.binop.rhs);     /* rhs -> eax */
    fprintf(o, "    mov ecx, eax\n");   /* rhs -> ecx */
    fprintf(o, "    pop eax\n");        /* lhs -> eax */
    switch (e->as.binop.op) {
    case OP_ADD:
        fprintf(o, "    add eax, ecx\n");
        break;
    case OP_SUB:
        fprintf(o, "    sub eax, ecx\n");
        break;
    case OP_MUL:
        fprintf(o, "    imul eax, ecx\n");
        break;
    case OP_DIV:
        fprintf(o, "    cdq\n");          /* sign-extend eax into edx:eax */
        fprintf(o, "    idiv ecx\n");     /* quotient -> eax */
        break;
    case OP_MOD:
        fprintf(o, "    cdq\n");
        fprintf(o, "    idiv ecx\n");     /* remainder -> edx */
        fprintf(o, "    mov eax, edx\n");
        break;
    case OP_NEG:
        cg_ice("OP_NEG in binop position", e);
        break;
    default:
        cg_ice("unknown binop", e);
        break;
    }
}

static void gen_expr(Cg *cg, const AstNode *e)
{
    FILE *o = cg->out;
    switch (e->kind) {
    case AST_INTLIT:
        /* long literal -> 32-bit immediate. The lexer/parser keep these in
         * range for the subset; emit as a decimal immediate. */
        fprintf(o, "    mov eax, %ld\n", e->as.intlit.value);
        break;
    case AST_VARREF:
        fprintf(o, "    mov eax, [v_");
        emit_var_label(o, e->as.varref.name);
        fprintf(o, "]\n");
        break;
    case AST_BINOP:
        gen_binop(cg, e);
        break;
    case AST_UNOP:
        if (e->as.unop.op != OP_NEG)
            cg_ice("unknown unop", e);
        gen_expr(cg, e->as.unop.operand);
        fprintf(o, "    neg eax\n");
        break;
    default:
        cg_ice("non-expression node in expression context", e);
        break;
    }
}

/* re-derive a string arg's label ordinal by counting strings emitted before it
 * in the SAME source-order walk used by the rodata pass. */
static void gen_write(Cg *cg, const AstNode *n, int *str_idx)
{
    FILE *o = cg->out;
    for (size_t i = 0; i < n->as.write.args.count; i++) {
        AstNode *arg = n->as.write.args.items[i];
        if (arg->kind == AST_STRLIT) {
            fprintf(o, "    mov eax, str_%d\n", (*str_idx)++);
            fprintf(o, "    call serial_puts\n");
        } else {
            gen_expr(cg, arg);                 /* value -> eax */
            fprintf(o, "    call serial_put_int\n");
        }
    }
    if (n->as.write.is_newline) {
        fprintf(o, "    mov al, 10\n");          /* '\n' */
        fprintf(o, "    call serial_putc\n");
    }
}

static void gen_stmt(Cg *cg, const AstNode *n, int *str_idx)
{
    FILE *o = cg->out;
    switch (n->kind) {
    case AST_BLOCK:
        for (size_t i = 0; i < n->as.block.stmts.count; i++)
            gen_stmt(cg, n->as.block.stmts.items[i], str_idx);
        break;
    case AST_ASSIGN:
        gen_expr(cg, n->as.assign.value);       /* value -> eax */
        fprintf(o, "    mov [v_");
        emit_var_label(o, n->as.assign.name);
        fprintf(o, "], eax\n");
        break;
    case AST_WRITE:
    case AST_WRITELN:
        gen_write(cg, n, str_idx);
        break;
    default:
        cg_ice("unexpected statement node", n);
        break;
    }
}

/* Collect declared variable names (AST_VARREF leaves under vardecls). */
static void emit_bss(Cg *cg, const AstNode *program)
{
    FILE *o = cg->out;
    const AstList *decls = &program->as.program.decls;
    if (decls->count == 0)
        return;
    fprintf(o, "section .bss\n");
    fprintf(o, "align 4\n");
    for (size_t i = 0; i < decls->count; i++) {
        const AstNode *vd = decls->items[i];
        if (vd->kind != AST_VARDECL)
            cg_ice("non-vardecl in program decls", vd);
        for (size_t j = 0; j < vd->as.vardecl.names.count; j++) {
            const AstNode *vr = vd->as.vardecl.names.items[j];
            if (vr->kind != AST_VARREF)
                cg_ice("non-varref in vardecl names", vr);
            fprintf(o, "v_");
            emit_var_label(o, vr->as.varref.name);
            fprintf(o, ": resd 1\n");
        }
    }
    fprintf(o, "\n");
}

int codegen_emit(const AstNode *program, FILE *out)
{
    if (!program || program->kind != AST_PROGRAM) {
        fprintf(stderr, "initechc: codegen: root is not a program\n");
        return 1;
    }

    Cg cg;
    cg.out = out;
    cg.str_count = 0;

    /* File banner (deterministic; cites the PRD per Law 1). */
    fprintf(out,
        "; Generated by initechc (seed cross-compiler, beads initech-znb).\n"
        "; Ref: PRD Sec 6.7 (stack-machine codegen, fixed registers, no\n"
        ";      optimizer/allocator). DO NOT EDIT -- emit is deterministic.\n"
        "; ABI: pas_main is called by the runtime (seed/rt/start.asm).\n"
        ";      Expressions evaluate onto the x86 stack; eax = result.\n"
        "bits 32\n\n");

    /* externs: runtime serial helpers. */
    fprintf(out, "extern serial_putc\n");
    fprintf(out, "extern serial_puts\n");
    fprintf(out, "extern serial_put_int\n\n");

    /* .rodata: string literals, in source order. */
    fprintf(out, "section .rodata\n");
    rodata_walk(&cg, program);
    fprintf(out, "\n");

    /* .bss: variable slots. */
    emit_bss(&cg, program);

    /* .text: the program body as pas_main. */
    fprintf(out, "section .text\n");
    fprintf(out, "global pas_main\n");
    fprintf(out, "pas_main:\n");
    fprintf(out, "    push ebp\n");
    fprintf(out, "    mov ebp, esp\n");

    int str_idx = 0;
    gen_stmt(&cg, program->as.program.block, &str_idx);

    fprintf(out, "    leave\n");
    fprintf(out, "    ret\n");

    if (ferror(out)) {
        fprintf(stderr, "initechc: codegen: write error\n");
        return 1;
    }
    return 0;
}
